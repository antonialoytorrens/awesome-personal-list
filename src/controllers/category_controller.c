/* awesome-personal-list - categories: full CRUD, user-managed (no hardcoded list). */

#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "ecewo.h"

#include "app.h"
#include "controllers.h"
#include "form.h"
#include "helpers.h"
#include "i18n.h"
#include "middleware.h"
#include "models.h"
#include "strutil.h"
#include "validate.h"
#include "views.h"

void
route_categories_list(ecewo_request_t *req, ecewo_response_t *res)
{
	struct category	*cats;
	size_t		 n, i;
	struct wbuf	 rows;
	struct view	*v;
	const char	*lang;

	if (!require_auth(req, res))
		return;

	lang = ctx_lang(req);

	category_list(&cats, &n);

	/* name/description are free text, only length-checked at creation --
	 * escaped here rather than trusted. */
	wbuf_init(&rows, 2048);
	for (i = 0; i < n; i++) {
		buf_lit(&rows, "<article class=\"star\">\n");
		buf_lit(&rows, "<h3 class=\"star-name\"><a href=\"/categories/");
		html_escape(&rows, cats[i].slug, strlen(cats[i].slug));
		buf_lit(&rows, "\">");
		if (cats[i].color[0] != '\0') {
			buf_lit(&rows, "<span class=\"dot\" data-cat=\"");
			html_escape(&rows, cats[i].slug, strlen(cats[i].slug));
			buf_lit(&rows, "\"></span> ");
		}
		html_escape(&rows, cats[i].name, strlen(cats[i].name));
		buf_lit(&rows, "</a></h3>\n");
		if (cats[i].description[0] != '\0') {
			buf_lit(&rows, "<p class=\"star-desc\">");
			html_escape(&rows, cats[i].description,
			    strlen(cats[i].description));
			buf_lit(&rows, "</p>\n");
		}
		buf_lit(&rows, "</article>\n");
	}
	if (n == 0) {
		const char	*empty = i18n_t(lang, "categories.empty");

		buf_lit(&rows, "<p class=\"empty\">");
		html_escape(&rows, empty, strlen(empty));
		buf_lit(&rows, "</p>\n");
	}
	category_list_free(cats);

	v = view_new("categories.html");
	view_title(v, i18n_t(lang, "categories.title"));
	view_set_raw(v, "ROWS", rows.data, rows.offset);
	wbuf_cleanup(&rows);
	view_render(req, res, ECEWO_OK, v);
}

void
route_category_create(ecewo_request_t *req, ecewo_response_t *res)
{
	struct form	 form;
	struct category	 c;
	const char	*name, *lang;

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

	name = form_get(&form, "name");
	if (!v_category_name(name)) {
		form_free(&form);
		view_error(req, res, ECEWO_BAD_REQUEST,
		    i18n_t(lang, "errors.bad_category_name"));
		return;
	}

	memset(&c, 0, sizeof(c));
	str_lcpy(c.name, name, sizeof(c.name));

	if (store_slugify_url(name, c.slug, sizeof(c.slug)) == -1) {
		form_free(&form);
		view_error(req, res, ECEWO_BAD_REQUEST,
		    i18n_t(lang, "errors.unusable_name"));
		return;
	}

	/* UNCATEGORIZED_SLUG is a reserved, system-managed bucket -- see
	 * category.c -- never a category a user can create or repurpose. */
	if (!strcmp(c.slug, UNCATEGORIZED_SLUG)) {
		form_free(&form);
		view_error(req, res, ECEWO_BAD_REQUEST,
		    i18n_t(lang, "errors.bad_category_name"));
		return;
	}

	if (v_text(form_get(&form, "description"), TEXT_MAX_LEN))
		str_lcpy(c.description, form_get(&form, "description"),
		    sizeof(c.description));

	str_lcpy(c.color, v_color(form_get(&form, "color")) ?
	    form_get(&form, "color") : CATEGORY_DEFAULT_COLOR,
	    sizeof(c.color));

	category_write(&c);
	form_free(&form);
	view_redirect(res, "/categories");
}

void
route_category_get(ecewo_request_t *req, ecewo_response_t *res)
{
	struct category	 c;
	struct view	*v;
	const char	*slug, *lang;
	int		 locked;

	if (!require_auth(req, res))
		return;

	lang = ctx_lang(req);

	slug = ecewo_param(req, "slug");
	if (slug == NULL || category_read(slug, &c) == -1) {
		view_error(req, res, ECEWO_NOT_FOUND,
		    i18n_t(lang, "errors.no_such_category"));
		return;
	}

	locked = !strcmp(c.slug, UNCATEGORIZED_SLUG);

	v = view_new("category_form.html");
	view_title(v, i18n_t(lang, "category_form.title"));
	view_set(v, "SLUG", c.slug);
	view_set(v, "NAME", c.name);
	view_set(v, "DESCRIPTION", c.description);
	view_set(v, "DISABLED", locked ? "disabled" : "");

	if (locked) {
		view_set_lit(v, "COLOR_FIELD", "");
		view_set_lit(v, "SAVE_ACTIONS", "");
		view_set_lit(v, "DELETE_FORM", "");
	} else {
		struct wbuf	color, save, del;

		wbuf_init(&color, 256);
		buf_lit(&color,
		    "<label>[[fields.color]]"
		    "<input type=\"color\" name=\"color\" value=\"");
		html_escape(&color, c.color, strlen(c.color));
		buf_lit(&color, "\"></label>\n");
		view_set_raw(v, "COLOR_FIELD", color.data, color.offset);
		wbuf_cleanup(&color);

		wbuf_init(&save, 128);
		buf_lit(&save, "<div class=\"form-actions\">\n"
		    "<button type=\"submit\" class=\"btn btn-primary\">"
		    "[[actions.save]]</button>\n</div>\n");
		view_set_raw(v, "SAVE_ACTIONS", save.data, save.offset);
		wbuf_cleanup(&save);

		/* c.slug is already restricted to [A-Za-z0-9._-] by v_slug()
		 * at creation time; escaped here regardless, on general
		 * principle for anything landing in an href. */
		wbuf_init(&del, 512);
		buf_lit(&del, "<form class=\"form-danger\" method=\"post\" "
		    "action=\"/categories/");
		html_escape(&del, c.slug, strlen(c.slug));
		buf_lit(&del, "\">\n"
		    "<input type=\"hidden\" name=\"csrf\" value=\"$CSRF$\">\n"
		    "<input type=\"hidden\" name=\"_method\" value=\"delete\">\n"
		    "<details class=\"confirm\">\n"
		    "<summary class=\"btn btn-danger\">"
		    "<span class=\"when-closed\">[[actions.delete]]</span>"
		    "<span class=\"when-open\">[[actions.cancel]]</span></summary>\n"
		    "<div class=\"confirm-pop confirm-pop--left\">\n"
		    "<p>[[category_form.confirm_delete]]</p>\n"
		    "<button type=\"submit\" class=\"btn btn-sm btn-danger\">"
		    "[[actions.confirm_delete]]</button>\n"
		    "</div>\n</details>\n</form>\n");
		view_set_raw(v, "DELETE_FORM", del.data, del.offset);
		wbuf_cleanup(&del);
	}

	view_render(req, res, ECEWO_OK, v);
}

void
route_category_post(ecewo_request_t *req, ecewo_response_t *res)
{
	struct form	 form;
	struct category	 c;
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

	if (slug == NULL || category_read(slug, &c) == -1) {
		form_free(&form);
		view_error(req, res, ECEWO_NOT_FOUND,
		    i18n_t(lang, "errors.no_such_category"));
		return;
	}

	/* UNCATEGORIZED_SLUG is system-managed -- see category.c -- neither
	 * editable nor deletable, regardless of _method. */
	if (!strcmp(c.slug, UNCATEGORIZED_SLUG)) {
		form_free(&form);
		view_error(req, res, ECEWO_FORBIDDEN,
		    i18n_t(lang, "errors.category_in_use"));
		return;
	}

	method = form_get(&form, "_method");

	if (method != NULL && !strcasecmp(method, "delete")) {
		if (category_delete(slug) == -1) {
			form_free(&form);
			view_error(req, res, ECEWO_CONFLICT,
			    i18n_t(lang, "errors.category_in_use"));
			return;
		}
		form_free(&form);
		view_redirect(res, "/categories");
		return;
	}

	if (v_category_name(form_get(&form, "name")))
		str_lcpy(c.name, form_get(&form, "name"), sizeof(c.name));

	if (v_text(form_get(&form, "description"), TEXT_MAX_LEN))
		str_lcpy(c.description, form_get(&form, "description"),
		    sizeof(c.description));

	if (v_color(form_get(&form, "color")))
		str_lcpy(c.color, form_get(&form, "color"), sizeof(c.color));

	category_write(&c);
	form_free(&form);
	view_redirect(res, "/categories");
}

/*
 * Category colors are dynamic, user-set data -- not a fixed table like the
 * language dots -- so under the strict CSP (no inline style=) the only
 * clean way to apply them is a real generated stylesheet, not a header
 * that would need 'unsafe-inline'.
 */
void
route_categories_css(ecewo_request_t *req, ecewo_response_t *res)
{
	struct category	*cats;
	size_t		 n, i;
	struct wbuf	 css;
	char		*text;

	if (!require_auth(req, res))
		return;

	category_list(&cats, &n);

	wbuf_init(&css, 1024);
	for (i = 0; i < n; i++) {
		if (!v_color(cats[i].color))
			continue;

		/* slug is already restricted to [A-Za-z0-9._-] by v_slug()
		 * at creation; color is re-checked here regardless. */
		wbuf_appendf(&css, ".dot[data-cat=\"%s\"]{background:%s}\n",
		    cats[i].slug, cats[i].color);
	}
	category_list_free(cats);

	text = (char *)wbuf_stringify(&css, NULL);
	ecewo_header_set(res, "Content-Type", "text/css; charset=utf-8");
	ecewo_send(res, ECEWO_OK, text, strlen(text));
	wbuf_cleanup(&css);
}
