/*
 * awesome-personal-list - the $VAR$ template engine.
 * tpl_set() escapes '$' too, so a substituted value can never introduce a
 * placeholder for a later pass. view_set_raw() defers to render time for
 * values that must carry literal HTML.
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ecewo.h"

#include "app.h"
#include "helpers.h"
#include "i18n.h"
#include "middleware.h"
#include "views.h"
#include "wbuf.h"
#include "xmalloc.h"

static void	view_apply_raw(struct view *);
static void	build_lang_options(struct wbuf *, const char *);
static void	build_current_path(struct wbuf *, ecewo_request_t *);

struct view *
view_new(const char *template)
{
	struct view	*v;
	const char	*data;
	size_t		 len;

	if ((data = template_get(template, &len)) == NULL)
		fatal("view_new: no such template: %s", template);

	v = xcalloc(1, sizeof(*v));
	v->body = wbuf_alloc(len + 1024);
	wbuf_append(v->body, data, len);

	return (v);
}

void
view_free(struct view *v)
{
	size_t	i;

	if (v == NULL)
		return;

	for (i = 0; i < v->nraw; i++) {
		free(v->raw[i].key);
		free(v->raw[i].val);
	}

	wbuf_free(v->body);
	free(v->title);
	free(v);
}

void
tpl_set(struct wbuf *buf, const char *key, const char *value)
{
	char		token[64];
	struct wbuf	esc;
	size_t		i, len;

	if (value == NULL)
		value = "";

	snprintf(token, sizeof(token), "$%s$", key);

	wbuf_init(&esc, strlen(value) + 16);
	len = strlen(value);

	for (i = 0; i < len; i++) {
		switch (value[i]) {
		case '&':	buf_lit(&esc, "&amp;"); break;
		case '<':	buf_lit(&esc, "&lt;"); break;
		case '>':	buf_lit(&esc, "&gt;"); break;
		case '"':	buf_lit(&esc, "&quot;"); break;
		case '\'':	buf_lit(&esc, "&#39;"); break;
		case '$':	buf_lit(&esc, "&#36;"); break;
		default:	wbuf_append(&esc, value + i, 1);
		}
	}

	wbuf_replace_string(buf, token, esc.data, esc.offset);
	wbuf_cleanup(&esc);
}

void
view_set(struct view *v, const char *key, const char *value)
{
	tpl_set(v->body, key, value);
}

void
view_set_raw(struct view *v, const char *key, const void *val, size_t len)
{
	if (v->nraw == VIEW_MAX_RAW)
		fatal("view_set_raw: too many raw slots for %s", key);

	v->raw[v->nraw].key = xstrdup(key);
	v->raw[v->nraw].val = xmalloc(len);
	memcpy(v->raw[v->nraw].val, val, len);
	v->raw[v->nraw].len = len;
	v->nraw++;
}

void
view_setf(struct view *v, const char *key, const char *fmt, ...)
{
	va_list		ap;
	struct wbuf	tmp;

	wbuf_init(&tmp, 256);
	va_start(ap, fmt);
	wbuf_appendv(&tmp, fmt, ap);
	va_end(ap);

	tpl_set(v->body, key, wbuf_stringify(&tmp, NULL));
	wbuf_cleanup(&tmp);
}

void
view_title(struct view *v, const char *title)
{
	free(v->title);
	v->title = xstrdup(title);
}

/* Raw slots go in after every $KEY$ pass, so their content is never rescanned. */
static void
view_apply_raw(struct view *v)
{
	char		token[64];
	size_t		i;

	for (i = 0; i < v->nraw; i++) {
		snprintf(token, sizeof(token), "$%s$", v->raw[i].key);
		wbuf_replace_string(v->body, token, v->raw[i].val,
		    v->raw[i].len);
	}
}

/*
 * The navbar/profile language switcher redirects back to this path so
 * switching languages doesn't lose the page the admin was on. ecewo only
 * exposes the bare path (no raw query string), so this rebuilds it from
 * the handful of query keys the app actually uses (all on /sources);
 * pages with none of these render just the bare path, unchanged.
 */
static void
build_current_path(struct wbuf *out, ecewo_request_t *req)
{
	static const char *const keys[] =
	    { "category", "language", "q", "page", "per_page" };
	const char	*path = ecewo_req_path(req);
	size_t		 i;
	char		 sep = '?';

	wbuf_append(out, path, strlen(path));

	for (i = 0; i < sizeof(keys) / sizeof(keys[0]); i++) {
		const char	*v = ecewo_query(req, keys[i]);
		char		*enc;

		if (v == NULL || v[0] == '\0')
			continue;

		enc = url_encode_str(v);
		wbuf_appendf(out, "%c%s=%s", sep, keys[i], enc);
		free(enc);
		sep = '&';
	}
}

/* <option> per loaded locale, current one marked selected. Native names are
 * a small hardcoded, developer-trusted table (i18n_native_name()), not
 * translated content, so no escaping pass is needed here. */
static void
build_lang_options(struct wbuf *out, const char *current)
{
	const char *const	*codes;
	size_t			 n, i;

	codes = i18n_locales(&n);

	for (i = 0; i < n; i++) {
		wbuf_appendf(out, "<option value=\"%s\"%s>%s</option>",
		    codes[i], !strcmp(codes[i], current) ? " selected" : "",
		    i18n_native_name(codes[i]));
	}
}

void
view_render(ecewo_request_t *req, ecewo_response_t *res, int status,
    struct view *v)
{
	struct wbuf	 layout, opts;
	const char	*data, *lang;
	size_t		 len;
	char		*html;

	view_apply_raw(v);

	if ((data = template_get("layout.html", &len)) == NULL)
		fatal("view_render: layout.html missing");

	wbuf_init(&layout, len + v->body->offset + 256);
	wbuf_append(&layout, data, len);

	lang = ctx_lang(req);

	wbuf_replace_string(&layout, "$BODY$", v->body->data, v->body->offset);
	tpl_set(&layout, "TITLE", v->title != NULL ? v->title : APP_NAME);
	tpl_set(&layout, "USER", ctx_user(req));
	tpl_set(&layout, "CSRF", csrf_token(req));
	tpl_set(&layout, "BODY_CLASS", ctx_authed(req) ? "authed" : "guest");
	tpl_set(&layout, "LANG", lang);

	{
		struct wbuf	curpath;

		wbuf_init(&curpath, 128);
		build_current_path(&curpath, req);
		tpl_set(&layout, "CURRENT_PATH", wbuf_stringify(&curpath, NULL));
		wbuf_cleanup(&curpath);
	}

	wbuf_init(&opts, 128);
	build_lang_options(&opts, lang);
	wbuf_replace_string(&layout, "$LANG_OPTIONS$", opts.data, opts.offset);
	wbuf_cleanup(&opts);

	i18n_translate(&layout, lang);

	html = wbuf_stringify(&layout, NULL);
	ecewo_send_html(res, status, html);

	wbuf_cleanup(&layout);
	view_free(v);
}

void
view_error(ecewo_request_t *req, ecewo_response_t *res, int status,
    const char *message)
{
	struct view	*v;

	v = view_new("error.html");
	view_title(v, "Error");
	view_set(v, "MESSAGE", message);
	view_render(req, res, status, v);
}

void
view_redirect(ecewo_response_t *res, const char *path)
{
	ecewo_redirect(res, ECEWO_SEE_OTHER, path);
}
