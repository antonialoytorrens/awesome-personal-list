/*
 * awesome-personal-list - Software Heritage API client.
 */

#include <curl/curl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "app.h"
#include "helpers.h"
#include "strutil.h"
#include "swh.h"
#include "wbuf.h"
#include "xmalloc.h"

#define SWH_API_BASE	"https://archive.softwareheritage.org/api/1"

static size_t
curl_discard(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	(void)ptr;
	(void)userdata;
	return (size * nmemb);
}

static size_t
curl_header_cb(char *buffer, size_t size, size_t nitems, void *userdata)
{
	struct swh_result	*out = userdata;
	size_t			 len = size * nitems;
	char			 line[256];
	int			 err;

	if (len >= sizeof(line))
		return (len);

	memcpy(line, buffer, len);
	line[len] = '\0';

	if (!strncasecmp(line, "x-ratelimit-remaining:", 22)) {
		out->rate_remaining = (long)str_tonum(line + 22, 10, 0,
		    1000000, &err);
		if (err != STR_OK)
			out->rate_remaining = -1;
	} else if (!strncasecmp(line, "x-ratelimit-reset:", 18)) {
		out->rate_reset = (long)str_tonum(line + 18, 10, 0,
		    4102444800LL, &err);
		if (err != STR_OK)
			out->rate_reset = 0;
	}

	return (len);
}

/* Reused across every call: a fresh handle per request would mean a fresh
 * TCP+TLS handshake to the same SWH host every time. */
static CURL *swh_curl;

int
swh_init(void)
{
	if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK)
		return (-1);

	return ((swh_curl = curl_easy_init()) != NULL ? 0 : -1);
}

void
swh_cleanup(void)
{
	if (swh_curl != NULL)
		curl_easy_cleanup(swh_curl);
	swh_curl = NULL;
	curl_global_cleanup();
}

int
swh_check_origin(const char *original_url, const char *bearer_token,
    struct swh_result *out)
{
	struct curl_slist	*headers = NULL;
	char			*encoded;
	char			 url[URL_MAX_LEN * 3];
	char			 authhdr[512];
	long			 http_status = 0;
	CURLcode		 rc;

	out->status = SWH_STATUS_UNKNOWN;
	out->rate_remaining = -1;
	out->rate_reset = 0;

	encoded = curl_easy_escape(swh_curl, original_url, 0);
	if (encoded == NULL)
		return (-1);

	if (snprintf(url, sizeof(url), "%s/origin/%s/get/", SWH_API_BASE,
	    encoded) >= (int)sizeof(url)) {
		curl_free(encoded);
		return (-1);
	}
	curl_free(encoded);

	if (bearer_token != NULL && bearer_token[0] != '\0') {
		snprintf(authhdr, sizeof(authhdr), "Authorization: Bearer %s",
		    bearer_token);
		headers = curl_slist_append(headers, authhdr);
	}

	curl_easy_setopt(swh_curl, CURLOPT_URL, url);
	curl_easy_setopt(swh_curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(swh_curl, CURLOPT_WRITEFUNCTION, curl_discard);
	curl_easy_setopt(swh_curl, CURLOPT_HEADERFUNCTION, curl_header_cb);
	curl_easy_setopt(swh_curl, CURLOPT_HEADERDATA, out);
	curl_easy_setopt(swh_curl, CURLOPT_USERAGENT, APP_NAME "/" APP_VERSION);
	curl_easy_setopt(swh_curl, CURLOPT_TIMEOUT, 30L);
	curl_easy_setopt(swh_curl, CURLOPT_FOLLOWLOCATION, 1L);

	rc = curl_easy_perform(swh_curl);

	if (rc == CURLE_OK)
		curl_easy_getinfo(swh_curl, CURLINFO_RESPONSE_CODE, &http_status);

	curl_slist_free_all(headers);

	if (rc != CURLE_OK)
		return (-1);

	if (http_status == 200) {
		out->status = SWH_STATUS_ARCHIVED;
		return (0);
	}

	if (http_status == 404) {
		out->status = SWH_STATUS_NOT_FOUND;
		return (0);
	}

	/* 429, 5xx, etc: no definitive answer, caller retries later. */
	return (-1);
}
