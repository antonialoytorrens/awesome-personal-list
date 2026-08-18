/* awesome-personal-list - login/logout. No /register route: single seeded admin. */

#include <stdlib.h>
#include <string.h>

#include "ecewo.h"

#include "app.h"
#include "controllers.h"
#include "form.h"
#include "helpers.h"
#include "i18n.h"
#include "middleware.h"
#include "models.h"
#include "views.h"

void
route_login_get(ecewo_request_t *req, ecewo_response_t *res)
{
	struct view	*v;

	if (ctx_authed(req)) {
		view_redirect(res, "/");
		return;
	}

	v = view_new("login.html");
	view_title(v, i18n_t(ctx_lang(req), "login.title"));
	view_set(v, "NEXT", ecewo_query(req, "next"));
	view_set(v, "ERROR", "");
	view_render(req, res, ECEWO_OK, v);
}

void
route_login_post(ecewo_request_t *req, ecewo_response_t *res)
{
	struct form	form;
	struct admin	a;
	struct view	*v;
	const char	*name, *password, *next, *lang;

	lang = ctx_lang(req);

	if (form_parse(&form, ecewo_req_body(req), ecewo_req_body_len(req)) == -1) {
		view_error(req, res, ECEWO_BAD_REQUEST,
		    i18n_t(lang, "errors.malformed_form"));
		return;
	}

	name = form_get(&form, "name");
	password = form_get(&form, "password");
	next = form_get(&form, "next");

	if (name == NULL || password == NULL ||
	    admin_verify(name, password, &a) == -1) {
		form_free(&form);
		v = view_new("login.html");
		view_title(v, i18n_t(lang, "login.title"));
		view_set(v, "NEXT", next);
		view_set(v, "ERROR", i18n_t(lang, "login.error_invalid"));
		view_render(req, res, ECEWO_UNAUTHORIZED, v);
		return;
	}

	auth_login(req, res, &a);

	view_redirect(res, url_is_safe_next(next) ? next : "/");
	form_free(&form);
}

void
route_logout(ecewo_request_t *req, ecewo_response_t *res)
{
	auth_logout(req, res);
	view_redirect(res, "/login");
}
