/*
 * awesome-personal-list - sources: list/edit/delete, search, and the manual
 * "New source" flow (paste a URL, describe it, pick categories).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>

#include "ecewo.h"

#include "app.h"
#include "classify.h"
#include "controllers.h"
#include "form.h"
#include "helpers.h"
#include "i18n.h"
#include "middleware.h"
#include "models.h"
#include "strutil.h"
#include "swh.h"
#include "validate.h"
#include "views.h"
#include "xmalloc.h"

void
route_home(ecewo_request_t *req, ecewo_response_t *res)
{
	(void)req;
	view_redirect(res, "/sources");
}

static const char *
category_name(const struct category *cats, size_t n, const char *slug)
{
	size_t	i;

	for (i = 0; i < n; i++) {
		if (!strcmp(cats[i].slug, slug))
			return (cats[i].name);
	}
	return (slug);
}

static int
cmp_category_name(const void *a, const void *b)
{
	const struct category	*ca = a, *cb = b;

	return (strcasecmp(ca->name, cb->name));
}

/* qsort comparator for an array of char[NAME_MAX_LEN] -- each element
 * pointer is already the string itself, not a pointer-to-pointer. */
static int
cmp_lang(const void *a, const void *b)
{
	return (strcasecmp((const char *)a, (const char *)b));
}

/*
 * Sorted, de-duplicated list of every non-empty language in use, always
 * computed from the full (unfiltered) source list -- like the category
 * sidebar's counts -- so the option set doesn't shift as other filters
 * are applied. Returns the count; *out is heap-allocated, caller frees.
 */
static size_t
collect_languages(const struct source *srcs, size_t n, char (**out)[NAME_MAX_LEN])
{
	char	(*langs)[NAME_MAX_LEN];
	size_t	 i, j, count = 0;

	langs = xcalloc(n ? n : 1, sizeof(*langs));

	for (i = 0; i < n; i++) {
		if (srcs[i].language[0] == '\0')
			continue;

		for (j = 0; j < count; j++) {
			if (!strcmp(langs[j], srcs[i].language))
				break;
		}
		if (j == count)
			str_lcpy(langs[count++], srcs[i].language, NAME_MAX_LEN);
	}

	qsort(langs, count, sizeof(*langs), cmp_lang);

	*out = langs;
	return (count);
}

static size_t
count_in_category(const struct source *srcs, size_t n, const char *slug)
{
	size_t	i, c = 0;

	for (i = 0; i < n; i++) {
		if (source_in_category(&srcs[i], slug))
			c++;
	}
	return (c);
}

static void
render_swh_badge(struct wbuf *rows, const struct source *s, const char *lang)
{
	int		status;
	time_t		checked_at;
	const char	*cls, *label;

	if (swh_cache_get(s->original_url, &status, &checked_at) == -1)
		status = s->swh_status;
	(void)checked_at;

	switch (status) {
	case SWH_STATUS_ARCHIVED:
		cls = "badge-ok";
		label = i18n_t(lang, "swh.badge_archived");
		break;
	case SWH_STATUS_NOT_FOUND:
		cls = "badge-warn";
		label = i18n_t(lang, "swh.badge_not_found");
		break;
	default:
		cls = "badge-muted";
		label = i18n_t(lang, "swh.badge_unchecked");
		break;
	}

	buf_lit(rows, "<a class=\"badge ");
	wbuf_append(rows, cls, strlen(cls));
	buf_lit(rows, "\" href=\"");
	swh_browse_url(rows, s->original_url);
	buf_lit(rows, "\">");
	html_escape(rows, label, strlen(label));
	buf_lit(rows, "</a>");
}

/*
 * original_url is only ever checked with v_url() (scheme/length, not
 * character content) and slug/categories come from user-editable text too,
 * so all of it is HTML-escaped here rather than trusted as already-safe.
 */
static void
render_source_row(struct wbuf *rows, const struct source *s,
    const struct category *cats, size_t ncat, const char *lang)
{
	const char	*name, *host, *cname, *edit;
	int		 i;

	name = source_display_name(s);
	host = url_host_path(s->original_url);
	edit = i18n_t(lang, "actions.edit");

	buf_lit(rows, "<article class=\"star\">\n<div class=\"star-head\">\n");
	buf_lit(rows, "<input type=\"checkbox\" class=\"star-check\" name=\"slugs\" value=\"");
	html_escape(rows, s->slug, strlen(s->slug));
	buf_lit(rows, "\">\n");
	buf_lit(rows, "<h3 class=\"star-name\"><a href=\"/sources/");
	html_escape(rows, s->slug, strlen(s->slug));
	buf_lit(rows, "\">");
	html_escape(rows, name, strlen(name));
	buf_lit(rows, "</a></h3>\n<a class=\"btn btn-sm btn-ghost\" href=\"/sources/");
	html_escape(rows, s->slug, strlen(s->slug));
	buf_lit(rows, "\">");
	html_escape(rows, edit, strlen(edit));
	buf_lit(rows, "</a>\n</div>\n<p class=\"star-path\">");
	html_escape(rows, host, strlen(host));
	buf_lit(rows, "</p>\n");

	if (s->description[0] != '\0') {
		buf_lit(rows, "<p class=\"star-desc\">");
		html_escape(rows, s->description, strlen(s->description));
		buf_lit(rows, "</p>\n");
	}

	buf_lit(rows, "<div class=\"star-meta\">\n");
	if (s->language[0] != '\0') {
		buf_lit(rows, "<span class=\"lang\"><span class=\"lang-dot\" data-lang=\"");
		html_escape(rows, s->language, strlen(s->language));
		buf_lit(rows, "\"></span> ");
		html_escape(rows, s->language, strlen(s->language));
		buf_lit(rows, "</span>\n");
	}
	for (i = 0; i < s->category_count; i++) {
		cname = category_name(cats, ncat, s->categories[i]);
		buf_lit(rows, "<a class=\"topic\" href=\"/sources?category=");
		html_escape(rows, s->categories[i], strlen(s->categories[i]));
		buf_lit(rows, "\"><span class=\"dot\" data-cat=\"");
		html_escape(rows, s->categories[i], strlen(s->categories[i]));
		buf_lit(rows, "\"></span>");
		html_escape(rows, cname, strlen(cname));
		buf_lit(rows, "</a>\n");
	}
	render_swh_badge(rows, s, lang);
	buf_lit(rows, "\n</div>\n</article>\n");
}

static void
render_category_sidebar(struct wbuf *out, const struct category *cats,
    size_t ncat, const struct source *srcs, size_t nsrc, const char *filter,
    const char *lang)
{
	size_t		i, n;
	int		active_all;
	const char	*all;

	active_all = (filter == NULL || *filter == '\0');
	all = i18n_t(lang, "sources.all_categories");

	buf_lit(out, "<a class=\"side-item");
	if (active_all)
		buf_lit(out, " is-active");
	buf_lit(out, "\" href=\"/sources\"><span>");
	html_escape(out, all, strlen(all));
	wbuf_appendf(out, "</span><span class=\"count\">%zu</span></a>\n", nsrc);

	for (i = 0; i < ncat; i++) {
		n = count_in_category(srcs, nsrc, cats[i].slug);
		buf_lit(out, "<a class=\"side-item");
		if (!active_all && !strcmp(filter, cats[i].slug))
			buf_lit(out, " is-active");
		buf_lit(out, "\" href=\"/sources?category=");
		html_escape(out, cats[i].slug, strlen(cats[i].slug));
		buf_lit(out, "\"><span><span class=\"dot\" data-cat=\"");
		html_escape(out, cats[i].slug, strlen(cats[i].slug));
		buf_lit(out, "\"></span>");
		html_escape(out, cats[i].name, strlen(cats[i].name));
		wbuf_appendf(out, "</span><span class=\"count\">%zu</span></a>\n", n);
	}
}

/* Reads every value for a repeated form field, e.g. several "categories". */
static int
form_get_all(const struct form *form, const char *key, const char **out, int max)
{
	size_t	i;
	int	n = 0;

	for (i = 0; i < form->count && n < max; i++) {
		if (!strcmp(form->fields[i].key, key))
			out[n++] = form->fields[i].value;
	}

	return (n);
}

/* Renders one checkbox per known category, checked if its slug is in `selected`.
 * name/description are free text (only length-checked at creation), so
 * escaped here rather than trusted. */
static void
render_category_checkboxes(struct wbuf *out, const struct category *cats,
    size_t ncat, const char *const *selected, int nselected)
{
	size_t	i;
	int	j, checked;

	for (i = 0; i < ncat; i++) {
		if (!strcmp(cats[i].slug, UNCATEGORIZED_SLUG))
			continue;
		checked = 0;
		for (j = 0; j < nselected; j++) {
			if (!strcmp(cats[i].slug, selected[j])) {
				checked = 1;
				break;
			}
		}

		buf_lit(out, "<label class=\"cat\"><input type=\"checkbox\" "
		    "name=\"categories\" value=\"");
		html_escape(out, cats[i].slug, strlen(cats[i].slug));
		wbuf_appendf(out, "\"%s> ", checked ? " checked" : "");
		html_escape(out, cats[i].name, strlen(cats[i].name));
		buf_lit(out, "</label>\n");
	}
}

/*
 * Pagination for /sources -- it can list thousands of rows, and rendering
 * all of them on every request is what was making the page slow.
 */
static const size_t PAGE_SIZES[] = { 10, 20, 30, 50, 100, 200 };
#define PAGE_SIZES_COUNT (sizeof(PAGE_SIZES) / sizeof(PAGE_SIZES[0]))
#define DEFAULT_PAGE_SIZE 50

struct page_window {
	size_t	start, end;	/* slice bounds into the filtered set, end exclusive */
	size_t	per_page;	/* always one of PAGE_SIZES */
	size_t	page;		/* 1-indexed */
	size_t	total_pages;
};

static struct page_window
compute_page_window(ecewo_request_t *req, size_t total)
{
	struct page_window	w = { 0 };
	const char		*pp = ecewo_query(req, "per_page");
	const char		*pg = ecewo_query(req, "page");
	size_t			i;
	int			err;
	long			v;

	/* Anything not in PAGE_SIZES -- including the retired "all" -- falls
	 * back to the default rather than erroring, so old bookmarks still
	 * resolve. The cap also keeps a full page of bulk checkboxes inside
	 * FORM_MAX_FIELDS. */
	w.per_page = DEFAULT_PAGE_SIZE;
	if (pp != NULL) {
		v = (long)str_tonum(pp, 10, 1, 100000, &err);
		if (err == STR_OK) {
			for (i = 0; i < PAGE_SIZES_COUNT; i++) {
				if ((size_t)v == PAGE_SIZES[i]) {
					w.per_page = (size_t)v;
					break;
				}
			}
		}
	}

	w.total_pages = (total == 0) ? 1 :
	    (total + w.per_page - 1) / w.per_page;

	w.page = 1;
	if (pg != NULL) {
		v = (long)str_tonum(pg, 10, 1, 1000000, &err);
		if (err == STR_OK)
			w.page = (size_t)v;
	}
	if (w.page > w.total_pages)
		w.page = w.total_pages;

	w.start = (w.page - 1) * w.per_page;
	w.end = w.start + w.per_page;
	if (w.end > total)
		w.end = total;
	if (w.start > total)
		w.start = total;

	return (w);
}

/* extra_qs: already-encoded "key=value" to keep across page links (e.g. a
 * category filter), or NULL. */
static void
render_pagination(struct wbuf *out, const char *path, const char *extra_qs,
    struct page_window w, const char *lang)
{
	size_t		i;
	const char	*prev = i18n_t(lang, "pager.prev");
	const char	*next = i18n_t(lang, "pager.next");

	buf_lit(out, "<div class=\"pager\">\n<div class=\"pager-sizes\">");
	for (i = 0; i < PAGE_SIZES_COUNT; i++) {
		wbuf_appendf(out, "<a class=\"pager-size%s\" href=\"%s?%s%s"
		    "per_page=%zu\">%zu</a>",
		    (w.per_page == PAGE_SIZES[i]) ? " is-active" : "",
		    path, extra_qs != NULL ? extra_qs : "",
		    extra_qs != NULL ? "&" : "", PAGE_SIZES[i], PAGE_SIZES[i]);
	}
	buf_lit(out, "</div>\n");

	if (w.total_pages > 1) {
		buf_lit(out, "<div class=\"pager-nav\">");
		if (w.page > 1) {
			wbuf_appendf(out, "<a href=\"%s?%s%sper_page=%zu&page=%zu\">",
			    path, extra_qs != NULL ? extra_qs : "",
			    extra_qs != NULL ? "&" : "", w.per_page, w.page - 1);
			html_escape(out, prev, strlen(prev));
			buf_lit(out, "</a>");
		}
		wbuf_appendf(out, "<span class=\"pager-status\">%zu / %zu</span>",
		    w.page, w.total_pages);
		if (w.page < w.total_pages) {
			wbuf_appendf(out, "<a href=\"%s?%s%sper_page=%zu&page=%zu\">",
			    path, extra_qs != NULL ? extra_qs : "",
			    extra_qs != NULL ? "&" : "", w.per_page, w.page + 1);
			html_escape(out, next, strlen(next));
			buf_lit(out, "</a>");
		}
		buf_lit(out, "</div>\n");
	}
	buf_lit(out, "</div>\n");
}

void
route_sources_list(ecewo_request_t *req, ecewo_response_t *res)
{
	struct source	*srcs;
	struct category	*cats = NULL;
	size_t		 n, ncat = 0, i, nmatched;
	size_t		*matched;
	struct wbuf	 rows, sidebar, pager, qsbuf, bulk_opts, lang_opts;
	struct view	*v;
	const char	*filter_raw, *q_raw, *lang_raw, *page_raw, *per_page_raw, *lang;
	char		 filter[NAME_MAX_LEN];
	char		 q[256];
	char		 language[NAME_MAX_LEN];
	char		(*langs)[NAME_MAX_LEN];
	size_t		 nlangs;
	char		*extra_qs;
	struct page_window w;

	if (!require_auth(req, res))
		return;

	lang = ctx_lang(req);

	filter_raw = ecewo_query(req, "category");
	filter[0] = '\0';
	if (filter_raw != NULL && v_slug(filter_raw))
		str_lcpy(filter, filter_raw, sizeof(filter));

	q_raw = ecewo_query(req, "q");
	q[0] = '\0';
	if (q_raw != NULL)
		str_lcpy(q, q_raw, sizeof(q));

	lang_raw = ecewo_query(req, "language");
	language[0] = '\0';
	if (lang_raw != NULL)
		str_lcpy(language, lang_raw, sizeof(language));

	/* Raw, unvalidated echo-back for the bulk-form hidden fields --
	 * compute_page_window() re-validates them on the next GET anyway. */
	page_raw = ecewo_query(req, "page");
	per_page_raw = ecewo_query(req, "per_page");

	if (source_list(&srcs, &n) == -1) {
		view_error(req, res, ECEWO_INTERNAL_SERVER_ERROR,
		    i18n_t(lang, "errors.list_sources_failed"));
		return;
	}

	if (category_list(&cats, &ncat) == 0 && ncat > 0)
		qsort(cats, ncat, sizeof(*cats), cmp_category_name);

	wbuf_init(&sidebar, 1024);
	render_category_sidebar(&sidebar, cats, ncat, srcs, n, filter, lang);

	nlangs = collect_languages(srcs, n, &langs);

	matched = xmalloc((n ? n : 1) * sizeof(*matched));
	nmatched = 0;
	for (i = 0; i < n; i++) {
		if (source_matches_filter(&srcs[i], filter, language, q))
			matched[nmatched++] = i;
	}

	w = compute_page_window(req, nmatched);

	wbuf_init(&rows, 4096);
	for (i = w.start; i < w.end; i++)
		render_source_row(&rows, &srcs[matched[i]], cats, ncat, lang);
	if (nmatched == 0) {
		const char	*empty = i18n_t(lang, "sources.empty");

		buf_lit(&rows, "<p class=\"empty\">");
		html_escape(&rows, empty, strlen(empty));
		buf_lit(&rows, "</p>\n");
	}
	free(matched);

	/* Both are re-embedded into href query strings on the pager and the
	 * search box's own value -- url_encode_str() (not just html_escape())
	 * so an '&' or '"' in a search term can't inject a bogus query param
	 * or break out of an attribute. */
	wbuf_init(&qsbuf, 128);
	if (filter[0] != '\0') {
		char	*enc = url_encode_str(filter);

		wbuf_appendf(&qsbuf, "category=%s", enc);
		free(enc);
	}
	if (language[0] != '\0') {
		char	*enc = url_encode_str(language);

		wbuf_appendf(&qsbuf, "%slanguage=%s", qsbuf.offset ? "&" : "", enc);
		free(enc);
	}
	if (q[0] != '\0') {
		char	*enc = url_encode_str(q);

		wbuf_appendf(&qsbuf, "%sq=%s", qsbuf.offset ? "&" : "", enc);
		free(enc);
	}
	extra_qs = qsbuf.offset ? (char *)wbuf_stringify(&qsbuf, NULL) : NULL;

	wbuf_init(&pager, 512);
	render_pagination(&pager, "/sources", extra_qs, w, lang);
	wbuf_cleanup(&qsbuf);

	/* Plain <option> list for the bulk-assign <select> -- existing
	 * categories only, bulk actions don't create new ones. */
	wbuf_init(&bulk_opts, 512);
	for (i = 0; i < ncat; i++) {
		if (!strcmp(cats[i].slug, UNCATEGORIZED_SLUG))
			continue;
		buf_lit(&bulk_opts, "<option value=\"");
		html_escape(&bulk_opts, cats[i].slug, strlen(cats[i].slug));
		buf_lit(&bulk_opts, "\">");
		html_escape(&bulk_opts, cats[i].name, strlen(cats[i].name));
		buf_lit(&bulk_opts, "</option>\n");
	}

	/* <option> list for the language filter -- every distinct language
	 * currently in use, value and label are the same free-text string. */
	wbuf_init(&lang_opts, 512);
	for (i = 0; i < nlangs; i++) {
		buf_lit(&lang_opts, "<option value=\"");
		html_escape(&lang_opts, langs[i], strlen(langs[i]));
		wbuf_appendf(&lang_opts, "\"%s>",
		    !strcmp(langs[i], language) ? " selected" : "");
		html_escape(&lang_opts, langs[i], strlen(langs[i]));
		buf_lit(&lang_opts, "</option>\n");
	}
	free(langs);

	source_list_free(srcs);
	if (cats != NULL)
		category_list_free(cats);

	v = view_new("sources.html");
	view_title(v, i18n_t(lang, "sources.title"));
	view_setf(v, "LEDE", i18n_t(lang, "sources.lede_fmt"), nmatched);
	view_setf(v, "BULK_SELECT_ALL_LABEL",
	    i18n_t(lang, "sources.bulk_select_all_matching_fmt"), nmatched);
	view_setf(v, "BULK_ADD_ALL_LABEL",
	    i18n_t(lang, "sources.bulk_add_category_all_fmt"), nmatched);
	view_setf(v, "BULK_DELETE_ALL_LABEL",
	    i18n_t(lang, "sources.bulk_delete_all_fmt"), nmatched);
	view_setf(v, "BULK_DELETE_ALL_CONFIRM",
	    i18n_t(lang, "sources.bulk_confirm_delete_all_fmt"), nmatched);
	view_set(v, "QUERY", q);
	view_set(v, "CATEGORY", filter);
	view_set(v, "LANGUAGE", language);
	view_set(v, "PAGE", page_raw != NULL ? page_raw : "");
	view_set(v, "PER_PAGE", per_page_raw != NULL ? per_page_raw : "");
	view_set_raw(v, "SIDEBAR", sidebar.data, sidebar.offset);
	view_set_raw(v, "ROWS", rows.data, rows.offset);
	view_set_raw(v, "PAGER", pager.data, pager.offset);
	view_set_raw(v, "BULK_CATEGORY_OPTIONS", bulk_opts.data, bulk_opts.offset);
	view_set_raw(v, "LANGUAGE_OPTIONS", lang_opts.data, lang_opts.offset);
	wbuf_cleanup(&sidebar);
	wbuf_cleanup(&rows);
	wbuf_cleanup(&pager);
	wbuf_cleanup(&bulk_opts);
	wbuf_cleanup(&lang_opts);
	view_render(req, res, ECEWO_OK, v);
}

void
route_source_get(ecewo_request_t *req, ecewo_response_t *res)
{
	struct source	 s;
	struct category	*cats;
	struct wbuf	 opts, badge;
	size_t		 n;
	int		 i;
	const char	*sel[SOURCE_MAX_CATEGORIES];
	struct view	*v;
	const char	*slug, *lang;

	if (!require_auth(req, res))
		return;

	lang = ctx_lang(req);

	slug = ecewo_param(req, "slug");
	if (slug == NULL || source_read(slug, &s) == -1) {
		view_error(req, res, ECEWO_NOT_FOUND,
		    i18n_t(lang, "errors.no_such_source"));
		return;
	}

	category_list(&cats, &n);
	for (i = 0; i < s.category_count; i++)
		sel[i] = s.categories[i];
	wbuf_init(&opts, 512);
	render_category_checkboxes(&opts, cats, n, sel, s.category_count);
	category_list_free(cats);

	v = view_new("source_form.html");
	view_title(v, i18n_t(lang, "source_form.title"));
	view_set(v, "SLUG", s.slug);
	view_set(v, "NAME", source_display_name(&s));
	view_set(v, "NAME_INPUT", s.name);
	view_set(v, "NAME_DEFAULT", repo_display_name(s.original_url));
	view_set(v, "ORIGINAL_URL", s.original_url);
	view_set(v, "DESCRIPTION", s.description);
	view_set(v, "LANGUAGE", s.language);
	view_set(v, "NOTES", s.notes);
	view_set_raw(v, "CATEGORY_OPTIONS", opts.data, opts.offset);
	wbuf_cleanup(&opts);

	wbuf_init(&badge, 128);
	render_swh_badge(&badge, &s, lang);
	view_set_raw(v, "SWH_BADGE", badge.data, badge.offset);
	wbuf_cleanup(&badge);

	view_render(req, res, ECEWO_OK, v);
}

void
route_source_post(ecewo_request_t *req, ecewo_response_t *res)
{
	struct form	 form;
	struct source	 s;
	const char	*slug, *method, *lang;

	if (!require_auth(req, res))
		return;

	lang = ctx_lang(req);
	slug = ecewo_param(req, "slug");

	if (form_parse(&form, ecewo_req_body(req), ecewo_req_body_len(req)) == -1 ||
	    !csrf_verify(req, &form)) {
		form_free(&form);
		view_error(req, res, ECEWO_FORBIDDEN,
		    i18n_t(lang, "errors.bad_request"));
		return;
	}

	if (slug == NULL || source_read(slug, &s) == -1) {
		form_free(&form);
		view_error(req, res, ECEWO_NOT_FOUND,
		    i18n_t(lang, "errors.no_such_source"));
		return;
	}

	method = form_get(&form, "_method");

	if (method != NULL && !strcasecmp(method, "delete")) {
		source_delete(slug);
		form_free(&form);
		view_redirect(res, "/sources");
		return;
	}

	if (v_text(form_get(&form, "name"), NAME_MAX_LEN))
		str_lcpy(s.name, form_get(&form, "name"), sizeof(s.name));
	if (v_text(form_get(&form, "description"), TEXT_MAX_LEN))
		str_lcpy(s.description, form_get(&form, "description"),
		    sizeof(s.description));
	if (v_text(form_get(&form, "language"), NAME_MAX_LEN)) {
		const char	*canon;

		str_lcpy(s.language, form_get(&form, "language"),
		    sizeof(s.language));
		canon = classify_canonical_language(s.language);
		if (canon != NULL)
			str_lcpy(s.language, canon, sizeof(s.language));
	}
	if (v_text(form_get(&form, "notes"), TEXT_MAX_LEN))
		str_lcpy(s.notes, form_get(&form, "notes"), sizeof(s.notes));

	{
		const char	*picked[SOURCE_MAX_CATEGORIES];
		int		 npicked, i;

		npicked = form_get_all(&form, "categories", picked,
		    SOURCE_MAX_CATEGORIES);

		s.category_count = 0;
		for (i = 0; i < npicked; i++) {
			if (v_slug(picked[i]))
				source_add_category(&s, picked[i]);
		}
	}

	{
		const char	*new_url = form_get(&form, "original_url");

		if (new_url != NULL && strcmp(new_url, s.original_url) != 0) {
			char		 newslug[NAME_MAX_LEN];
			struct source	 other;
			struct swh_result swh;

			if (!v_url(new_url)) {
				form_free(&form);
				view_error(req, res, ECEWO_BAD_REQUEST,
				    i18n_t(lang, "errors.bad_original_url"));
				return;
			}
			if (store_slugify_url(new_url, newslug,
			    sizeof(newslug)) == -1) {
				form_free(&form);
				view_error(req, res, ECEWO_BAD_REQUEST,
				    i18n_t(lang, "errors.unusable_url"));
				return;
			}
			/*
			 * Same guard source_exists_for_url() does (slugify,
			 * read, compare) -- inlined because a raw slug
			 * collision must be rejected even for a different
			 * literal URL that happens to slugify the same way,
			 * which source_exists_for_url()'s exact-URL check
			 * alone wouldn't catch.
			 */
			if (strcmp(newslug, s.slug) != 0 &&
			    source_read(newslug, &other) == 0) {
				form_free(&form);
				view_error(req, res, ECEWO_BAD_REQUEST,
				    i18n_t(lang, "errors.duplicate_source_url"));
				return;
			}

			str_lcpy(s.original_url, new_url,
			    sizeof(s.original_url));

			/*
			 * Stale cache entry is keyed on the old URL and
			 * irrelevant now -- check the new URL synchronously,
			 * same as source creation does.
			 */
			if (swh_cache_get(new_url, &s.swh_status,
			    &s.swh_checked_at) == -1 &&
			    swh_check_origin(new_url, app_cfg.swh_token,
			    &swh) == 0) {
				s.swh_status = swh.status;
				s.swh_checked_at = time(NULL);
				swh_cache_set(new_url, s.swh_status,
				    s.swh_checked_at);
			}

			if (strcmp(newslug, s.slug) != 0) {
				char	oldslug[NAME_MAX_LEN];

				str_lcpy(oldslug, s.slug, sizeof(oldslug));
				str_lcpy(s.slug, newslug, sizeof(s.slug));
				s.updated_at = time(NULL);
				source_write(&s);
				source_delete(oldslug);
				form_free(&form);
				view_redirect(res, "/sources");
				return;
			}
		}
	}

	s.updated_at = time(NULL);
	source_write(&s);

	form_free(&form);
	view_redirect(res, "/sources");
}

#define MAX_BULK_SLUGS	200	/* matches the largest per_page choice */

/*
 * Multi-select actions on the sources list: add a category to (never
 * replaces existing ones -- same "add" semantics as editing a source by
 * hand) or delete every checked row. Existing categories only; this
 * doesn't create new ones.
 */
/*
 * Re-applies the current /sources filter server-side over every source,
 * instead of trusting a per-row "slugs" field -- the only way "select all"
 * can apply to more than fit in one form body (see FORM_MAX_FIELDS).
 */
static void
bulk_apply_all_matching(const struct form *form, const char *action)
{
	struct source	*srcs;
	size_t		 n, i;
	char		 filter[NAME_MAX_LEN];
	char		 language[NAME_MAX_LEN];
	char		 q[256];
	const char	*filter_raw, *language_raw, *q_raw, *bulk_category;

	filter_raw = form_get(form, "category");
	filter[0] = '\0';
	if (filter_raw != NULL && v_slug(filter_raw))
		str_lcpy(filter, filter_raw, sizeof(filter));

	language_raw = form_get(form, "language");
	language[0] = '\0';
	if (language_raw != NULL)
		str_lcpy(language, language_raw, sizeof(language));

	q_raw = form_get(form, "q");
	q[0] = '\0';
	if (q_raw != NULL)
		str_lcpy(q, q_raw, sizeof(q));

	if (source_list(&srcs, &n) == -1)
		return;

	if (action != NULL && !strcmp(action, "delete")) {
		for (i = 0; i < n; i++) {
			if (source_matches_filter(&srcs[i], filter, language, q))
				source_delete(srcs[i].slug);
		}
	} else if (action != NULL && !strcmp(action, "add_category")) {
		struct category	c;

		bulk_category = form_get(form, "bulk_category");
		if (bulk_category != NULL && v_slug(bulk_category) &&
		    category_read(bulk_category, &c) == 0) {
			for (i = 0; i < n; i++) {
				if (!source_matches_filter(&srcs[i], filter,
				    language, q))
					continue;
				source_add_category(&srcs[i], bulk_category);
				srcs[i].updated_at = time(NULL);
				source_write(&srcs[i]);
			}
		}
	}

	source_list_free(srcs);
}

void
route_sources_bulk(ecewo_request_t *req, ecewo_response_t *res)
{
	struct form	 form;
	const char	*slugs[MAX_BULK_SLUGS];
	int		 nslugs, i;
	const char	*action, *bulk_category, *all_matching, *lang;
	struct wbuf	 target;
	const char	*category, *language, *q, *page, *per_page;

	if (!require_auth(req, res))
		return;

	lang = ctx_lang(req);

	if (form_parse(&form, ecewo_req_body(req), ecewo_req_body_len(req)) == -1 ||
	    !csrf_verify(req, &form)) {
		form_free(&form);
		view_error(req, res, ECEWO_FORBIDDEN,
		    i18n_t(lang, "errors.bad_request"));
		return;
	}

	action = form_get(&form, "bulk_action");
	all_matching = form_get(&form, "all_matching");

	if (all_matching != NULL && !strcmp(all_matching, "1")) {
		bulk_apply_all_matching(&form, action);
	} else if (action != NULL && !strcmp(action, "delete")) {
		nslugs = form_get_all(&form, "slugs", slugs, MAX_BULK_SLUGS);
		for (i = 0; i < nslugs; i++) {
			if (v_slug(slugs[i]))
				source_delete(slugs[i]);
		}
	} else if (action != NULL && !strcmp(action, "add_category")) {
		struct category	c;

		nslugs = form_get_all(&form, "slugs", slugs, MAX_BULK_SLUGS);
		bulk_category = form_get(&form, "bulk_category");
		if (bulk_category != NULL && v_slug(bulk_category) &&
		    category_read(bulk_category, &c) == 0) {
			for (i = 0; i < nslugs; i++) {
				struct source	s;

				if (!v_slug(slugs[i]) ||
				    source_read(slugs[i], &s) == -1)
					continue;
				source_add_category(&s, bulk_category);
				s.updated_at = time(NULL);
				source_write(&s);
			}
		}
	}

	/* Preserve the current filter/page across the redirect, same as the
	 * search box already does. */
	category = form_get(&form, "category");
	language = form_get(&form, "language");
	q = form_get(&form, "q");
	page = form_get(&form, "page");
	per_page = form_get(&form, "per_page");

	wbuf_init(&target, 128);
	buf_lit(&target, "/sources");
	if ((category != NULL && category[0] != '\0') ||
	    (language != NULL && language[0] != '\0') ||
	    (q != NULL && q[0] != '\0') ||
	    (page != NULL && page[0] != '\0') ||
	    (per_page != NULL && per_page[0] != '\0')) {
		char	sep = '?';

		if (category != NULL && category[0] != '\0' && v_slug(category)) {
			wbuf_appendf(&target, "%ccategory=%s", sep, category);
			sep = '&';
		}
		if (language != NULL && language[0] != '\0') {
			char	*enc = url_encode_str(language);

			wbuf_appendf(&target, "%clanguage=%s", sep, enc);
			free(enc);
			sep = '&';
		}
		if (q != NULL && q[0] != '\0') {
			char	*enc = url_encode_str(q);

			wbuf_appendf(&target, "%cq=%s", sep, enc);
			free(enc);
			sep = '&';
		}
		if (per_page != NULL && per_page[0] != '\0') {
			char	*enc = url_encode_str(per_page);

			wbuf_appendf(&target, "%cper_page=%s", sep, enc);
			free(enc);
			sep = '&';
		}
		if (page != NULL && page[0] != '\0') {
			char	*enc = url_encode_str(page);

			wbuf_appendf(&target, "%cpage=%s", sep, enc);
			free(enc);
		}
	}

	view_redirect(res, (char *)wbuf_stringify(&target, NULL));
	wbuf_cleanup(&target);
	form_free(&form);
}

/*
 * Manual "New source" flow: paste a URL, type description/notes by hand,
 * pick categories. Language isn't asked for here -- the background
 * language checker (src/lang_checker_main.c) fills it in and runs
 * classify_source() once it's known.
 */
void
route_source_new_get(ecewo_request_t *req, ecewo_response_t *res)
{
	struct category	*cats;
	struct wbuf	 opts;
	size_t		 n;
	struct view	*v;
	const char	*lang;

	if (!require_auth(req, res))
		return;

	lang = ctx_lang(req);

	category_list(&cats, &n);
	wbuf_init(&opts, 512);
	render_category_checkboxes(&opts, cats, n, NULL, 0);
	category_list_free(cats);

	v = view_new("new_source_form.html");
	view_title(v, i18n_t(lang, "new_source_form.title"));
	view_set(v, "ORIGINAL_URL", "");
	view_set(v, "DESCRIPTION", "");
	view_set(v, "NOTES", "");
	view_set_raw(v, "CATEGORY_OPTIONS", opts.data, opts.offset);
	wbuf_cleanup(&opts);
	view_render(req, res, ECEWO_OK, v);
}

void
route_source_new_post(ecewo_request_t *req, ecewo_response_t *res)
{
	struct form	 form;
	struct source	 s;
	struct swh_result swh;
	const char	*url, *lang;
	int		 i;

	if (!require_auth(req, res))
		return;

	lang = ctx_lang(req);

	if (form_parse(&form, ecewo_req_body(req), ecewo_req_body_len(req)) == -1 ||
	    !csrf_verify(req, &form)) {
		form_free(&form);
		view_error(req, res, ECEWO_FORBIDDEN,
		    i18n_t(lang, "errors.bad_request"));
		return;
	}

	url = form_get(&form, "original_url");
	if (!v_url(url)) {
		form_free(&form);
		view_error(req, res, ECEWO_BAD_REQUEST,
		    i18n_t(lang, "errors.bad_original_url"));
		return;
	}

	memset(&s, 0, sizeof(s));
	str_lcpy(s.original_url, url, sizeof(s.original_url));

	if (store_slugify_url(url, s.slug, sizeof(s.slug)) == -1) {
		form_free(&form);
		view_error(req, res, ECEWO_BAD_REQUEST,
		    i18n_t(lang, "errors.unusable_url"));
		return;
	}

	if (v_text(form_get(&form, "description"), TEXT_MAX_LEN))
		str_lcpy(s.description, form_get(&form, "description"),
		    sizeof(s.description));
	if (v_text(form_get(&form, "notes"), TEXT_MAX_LEN))
		str_lcpy(s.notes, form_get(&form, "notes"), sizeof(s.notes));

	{
		const char	*picked[SOURCE_MAX_CATEGORIES];
		int		 npicked;

		npicked = form_get_all(&form, "categories", picked,
		    SOURCE_MAX_CATEGORIES);
		for (i = 0; i < npicked; i++) {
			if (v_slug(picked[i]))
				source_add_category(&s, picked[i]);
		}
	}

	/* language is unset here; the lang-checker fills it in in the
	 * background (src/lang_checker_main.c) and classifies by language
	 * at that point -- nothing to classify yet at creation time. */

	if (swh_cache_get(url, &s.swh_status, &s.swh_checked_at) == -1 &&
	    swh_check_origin(url, app_cfg.swh_token, &swh) == 0) {
		s.swh_status = swh.status;
		s.swh_checked_at = time(NULL);
		swh_cache_set(url, s.swh_status, s.swh_checked_at);
	}

	s.created_at = s.updated_at = time(NULL);
	source_write(&s);

	form_free(&form);
	view_redirect(res, "/sources");
}
