/*
 * awesome-personal-list - repo-hosting-provider language detection.
 * Same shape as swh_client.c (one reused CURL handle, a rate-limit header
 * callback), but unlike swh_check_origin() (status code only) this reads
 * the JSON response body, so it also needs a write-to-buffer callback and
 * cJSON parsing.
 */

#include <curl/curl.h>

#include <cjson/cJSON.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "app.h"
#include "helpers.h"
#include "lang.h"
#include "strutil.h"
#include "wbuf.h"
#include "xmalloc.h"

/* Reused across every call, same reasoning as swh_client.c's swh_curl. */
static CURL *lang_curl;

int
lang_init(void)
{
	if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK)
		return (-1);

	return ((lang_curl = curl_easy_init()) != NULL ? 0 : -1);
}

void
lang_cleanup(void)
{
	if (lang_curl != NULL)
		curl_easy_cleanup(lang_curl);
	lang_curl = NULL;
	curl_global_cleanup();
}

static size_t
curl_to_wbuf(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	struct wbuf	*b = userdata;
	size_t		 len = size * nmemb;

	/* Bodies here are a few hundred bytes of JSON; refuse to buffer
	 * anything wildly larger than expected instead of trusting the peer. */
	if (b->offset + len > 65536)
		return (0);

	wbuf_append(b, ptr, len);
	return (len);
}

static long rate_seen = -1;

static size_t
curl_rate_header_cb(char *buffer, size_t size, size_t nitems, void *userdata)
{
	size_t	len = size * nitems;
	char	line[256];
	int	err;

	(void)userdata;

	if (len >= sizeof(line))
		return (len);

	memcpy(line, buffer, len);
	line[len] = '\0';

	if (!strncasecmp(line, "x-ratelimit-remaining:", 22)) {
		rate_seen = (long)str_tonum(line + 22, 10, 0, 1000000, &err);
		if (err != STR_OK)
			rate_seen = -1;
	} else if (!strncasecmp(line, "ratelimit-remaining:", 20)) {
		rate_seen = (long)str_tonum(line + 20, 10, 0, 1000000, &err);
		if (err != STR_OK)
			rate_seen = -1;
	}

	return (len);
}

/* Skips a leading scheme and the given host; NULL if `original_url` isn't
 * on that host. */
static const char *
path_after_host(const char *original_url, const char *host)
{
	const char	*p = url_host_path(original_url);
	size_t		 hlen = strlen(host);

	if (strncasecmp(p, host, hlen) != 0)
		return (NULL);
	p += hlen;
	if (*p != '/')
		return (NULL);
	return (p + 1);
}

/* Trims a trailing "/", "?...", "#..." and a trailing ".git" from a path
 * segment already known to be `len` bytes long. Returns the trimmed length. */
static size_t
trim_repo_segment(const char *seg, size_t len)
{
	if (len > 4 && !strncmp(seg + len - 4, ".git", 4))
		len -= 4;
	return (len);
}

static int
do_get(const char *url, struct curl_slist *headers, struct wbuf *body,
    long *http_status)
{
	CURLcode	rc;

	*http_status = 0;
	rate_seen = -1;

	curl_easy_setopt(lang_curl, CURLOPT_URL, url);
	curl_easy_setopt(lang_curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(lang_curl, CURLOPT_WRITEFUNCTION, curl_to_wbuf);
	curl_easy_setopt(lang_curl, CURLOPT_WRITEDATA, body);
	curl_easy_setopt(lang_curl, CURLOPT_HEADERFUNCTION, curl_rate_header_cb);
	curl_easy_setopt(lang_curl, CURLOPT_HEADERDATA, NULL);
	curl_easy_setopt(lang_curl, CURLOPT_USERAGENT, APP_NAME "/" APP_VERSION);
	curl_easy_setopt(lang_curl, CURLOPT_TIMEOUT, 30L);
	curl_easy_setopt(lang_curl, CURLOPT_FOLLOWLOCATION, 1L);

	rc = curl_easy_perform(lang_curl);
	if (rc == CURLE_OK)
		curl_easy_getinfo(lang_curl, CURLINFO_RESPONSE_CODE, http_status);

	return (rc == CURLE_OK ? 0 : -1);
}

static int
github_detect(const char *original_url, const char *token, char *out,
    size_t outlen)
{
	const char		*path, *slash, *repo_end;
	char			 owner[NAME_MAX_LEN], repo[NAME_MAX_LEN];
	char			 url[URL_MAX_LEN];
	char			 authhdr[512];
	struct curl_slist	*headers = NULL;
	struct wbuf		 body;
	cJSON			*root, *lang;
	long			 http_status;
	size_t			 n;
	int			 ret = -1;

	if ((path = path_after_host(original_url, "github.com")) == NULL)
		return (-1);
	if ((slash = strchr(path, '/')) == NULL)
		return (-1);

	n = (size_t)(slash - path);
	if (n == 0 || n >= sizeof(owner))
		return (-1);
	memcpy(owner, path, n);
	owner[n] = '\0';

	path = slash + 1;
	repo_end = path + strcspn(path, "/?#");
	n = trim_repo_segment(path, (size_t)(repo_end - path));
	if (n == 0 || n >= sizeof(repo))
		return (-1);
	memcpy(repo, path, n);
	repo[n] = '\0';

	if (snprintf(url, sizeof(url), "https://api.github.com/repos/%s/%s",
	    owner, repo) >= (int)sizeof(url))
		return (-1);

	headers = curl_slist_append(headers,
	    "Accept: application/vnd.github+json");
	if (token != NULL && token[0] != '\0') {
		snprintf(authhdr, sizeof(authhdr), "Authorization: Bearer %s",
		    token);
		headers = curl_slist_append(headers, authhdr);
	}

	wbuf_init(&body, 4096);

	if (do_get(url, headers, &body, &http_status) == 0 &&
	    http_status == 200) {
		root = cJSON_ParseWithLength((const char *)body.data, body.offset);
		if (root != NULL) {
			lang = cJSON_GetObjectItemCaseSensitive(root, "language");
			if (cJSON_IsString(lang) && lang->valuestring != NULL &&
			    lang->valuestring[0] != '\0') {
				str_lcpy(out, lang->valuestring, outlen);
				ret = 0;
			}
			cJSON_Delete(root);
		}
	}

	wbuf_cleanup(&body);
	curl_slist_free_all(headers);

	return (ret);
}

static int
gitlab_detect(const char *original_url, const char *token, char *out,
    size_t outlen)
{
	const char		*path, *path_end;
	char			 project[URL_MAX_LEN];
	char			*encoded;
	char			 url[URL_MAX_LEN * 3];
	char			 authhdr[512];
	struct curl_slist	*headers = NULL;
	struct wbuf		 body;
	cJSON			*root, *item;
	const char		*best_name = NULL;
	double			 best_pct = -1;
	long			 http_status;
	size_t			 n;
	int			 ret = -1;

	if ((path = path_after_host(original_url, "gitlab.com")) == NULL)
		return (-1);

	path_end = path + strcspn(path, "?#");
	while (path_end > path && path_end[-1] == '/')
		path_end--;
	n = trim_repo_segment(path, (size_t)(path_end - path));
	if (n == 0 || n >= sizeof(project))
		return (-1);
	memcpy(project, path, n);
	project[n] = '\0';

	encoded = curl_easy_escape(lang_curl, project, 0);
	if (encoded == NULL)
		return (-1);

	if (snprintf(url, sizeof(url),
	    "https://gitlab.com/api/v4/projects/%s/languages", encoded) >=
	    (int)sizeof(url)) {
		curl_free(encoded);
		return (-1);
	}
	curl_free(encoded);

	if (token != NULL && token[0] != '\0') {
		snprintf(authhdr, sizeof(authhdr), "PRIVATE-TOKEN: %s", token);
		headers = curl_slist_append(headers, authhdr);
	}

	wbuf_init(&body, 4096);

	if (do_get(url, headers, &body, &http_status) == 0 &&
	    http_status == 200) {
		root = cJSON_ParseWithLength((const char *)body.data, body.offset);
		if (root != NULL) {
			cJSON_ArrayForEach(item, root) {
				if (cJSON_IsNumber(item) &&
				    item->valuedouble > best_pct) {
					best_pct = item->valuedouble;
					best_name = item->string;
				}
			}
			if (best_name != NULL && best_name[0] != '\0') {
				str_lcpy(out, best_name, outlen);
				ret = 0;
			}
			cJSON_Delete(root);
		}
	}

	wbuf_cleanup(&body);
	curl_slist_free_all(headers);

	return (ret);
}

struct lang_provider {
	const char	*host;
	int		(*detect)(const char *original_url, const char *token,
			    char *out, size_t outlen);
};

static const struct lang_provider providers[] = {
	{ "github.com", github_detect },
	{ "gitlab.com", gitlab_detect },
};

int
lang_detect(const char *original_url, const char *github_token,
    const char *gitlab_token, char *out, size_t outlen, long *rate_remaining)
{
	const char	*host = url_host_path(original_url);
	size_t		 i;
	int		 rc;
	const char	*token;

	*rate_remaining = -1;

	for (i = 0; i < sizeof(providers) / sizeof(providers[0]); i++) {
		if (strncasecmp(host, providers[i].host,
		    strlen(providers[i].host)) != 0)
			continue;

		token = !strcmp(providers[i].host, "github.com") ?
		    github_token : gitlab_token;
		rc = providers[i].detect(original_url, token, out, outlen);
		*rate_remaining = rate_seen;
		return (rc);
	}

	return (-1);
}
