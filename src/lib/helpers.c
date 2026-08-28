/*
 * awesome-personal-list - HTML/URL escaping and date formatting.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "helpers.h"
#include "xmalloc.h"

void
html_escape(struct wbuf *b, const char *s, size_t len)
{
	size_t	i;

	for (i = 0; i < len; i++) {
		switch (s[i]) {
		case '&':
			buf_lit(b, "&amp;");
			break;
		case '<':
			buf_lit(b, "&lt;");
			break;
		case '>':
			buf_lit(b, "&gt;");
			break;
		case '"':
			buf_lit(b, "&quot;");
			break;
		case '\'':
			buf_lit(b, "&#39;");
			break;
		default:
			wbuf_append(b, s + i, 1);
		}
	}
}

char *
html_escape_str(const char *s)
{
	struct wbuf	b;
	char		*out;

	wbuf_init(&b, strlen(s) + 16);
	html_escape(&b, s, strlen(s));
	out = xstrdup(wbuf_stringify(&b, NULL));
	wbuf_cleanup(&b);

	return (out);
}

static const char unreserved[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.~";

void
url_encode(struct wbuf *b, const char *s, size_t len)
{
	static const char	hex[] = "0123456789ABCDEF";
	size_t			i;
	unsigned char		c;
	char			enc[3];

	for (i = 0; i < len; i++) {
		c = (unsigned char)s[i];

		if (strchr(unreserved, c) != NULL) {
			wbuf_append(b, &c, 1);
			continue;
		}

		enc[0] = '%';
		enc[1] = hex[c >> 4];
		enc[2] = hex[c & 0x0f];
		wbuf_append(b, enc, 3);
	}
}

char *
url_encode_str(const char *s)
{
	struct wbuf	b;
	char		*out;

	wbuf_init(&b, strlen(s) + 16);
	url_encode(&b, s, strlen(s));
	out = xstrdup(wbuf_stringify(&b, NULL));
	wbuf_cleanup(&b);

	return (out);
}

void
fmt_date(time_t t, char *out, size_t len)
{
	if (t == 0) {
		snprintf(out, len, "never");
		return;
	}

	strftime(out, len, "%Y-%m-%d", gmtime(&t));
}

const char *
repo_display_name(const char *url)
{
	const char	*slash;

	if (url == NULL || *url == '\0')
		return ("");

	slash = strrchr(url, '/');
	if (slash == NULL || slash[1] == '\0')
		return (url);

	return (slash + 1);
}

const char *
url_host_path(const char *url)
{
	const char	*p;

	if (url == NULL)
		return ("");

	p = strstr(url, "://");
	return (p != NULL ? p + 3 : url);
}

int
url_is_safe_next(const char *next)
{
	return (next != NULL && next[0] == '/' && next[1] != '/');
}

void
swh_browse_url(struct wbuf *out, const char *original_url)
{
	char	*enc;

	enc = url_encode_str(original_url);
	wbuf_appendf(out, "https://archive.softwareheritage.org/browse/origin/"
	    "?origin_url=%s", enc);
	free(enc);
}
