/* awesome-personal-list - the admin's own profile page: account info + language. */

#include <string.h>

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
route_profile_get(ecewo_request_t *req, ecewo_response_t *res)
{
	struct req_ctx	*ctx;
	struct view	*v;
	char		 created[32];

	if (!require_auth(req, res))
		return;

	ctx = ctx_get(req);
	fmt_date(ctx->user.created, created, sizeof(created));

	v = view_new("profile.html");
	view_title(v, i18n_t(ctx_lang(req), "profile.title"));
	view_set(v, "NAME", ctx->user.name);
	view_set(v, "CREATED", created);
	view_render(req, res, ECEWO_OK, v);
}

void
route_profile_post(ecewo_request_t *req, ecewo_response_t *res)
{
	struct form	 form;
	struct req_ctx	*ctx;
	const char	*name, *lang;

	if (!require_auth(req, res))
		return;

	if (form_parse(&form, ecewo_req_body(req), ecewo_req_body_len(req)) == -1 ||
	    !csrf_verify(req, &form)) {
		form_free(&form);
		view_error(req, res, ECEWO_FORBIDDEN,
		    i18n_t(ctx_lang(req), "errors.bad_request"));
		return;
	}

	ctx = ctx_get(req);
	name = form_get(&form, "name");
	lang = form_get(&form, "lang");

	if (name != NULL && v_admin_name(name) &&
	    admin_update_name(ctx->user.name, name) == 0)
		str_lcpy(ctx->user.name, name, sizeof(ctx->user.name));

	if (lang != NULL && i18n_supported(lang) &&
	    admin_update_language(ctx->user.name, lang) == 0) {
		str_lcpy(ctx->user.language, lang, sizeof(ctx->user.language));
		str_lcpy(ctx->lang, lang, sizeof(ctx->lang));
	}

	form_free(&form);
	view_redirect(res, "/profile");
}
