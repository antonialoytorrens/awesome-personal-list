/*
 * awesome-personal-list - POST /lang: the navbar and profile-page language switcher.
 * Only ever reachable while logged in -- .nav-right, where the selector
 * lives, is CSS-hidden for guests -- so this behaves like every other
 * mutating POST in the app: require_auth() + csrf_verify(), no anonymous
 * case to special-case.
 */

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
#include "views.h"

void
route_lang_set(ecewo_request_t *req, ecewo_response_t *res)
{
	struct form	 form;
	struct req_ctx	*ctx;
	const char	*lang, *next;

	if (!require_auth(req, res))
		return;

	if (form_parse(&form, ecewo_req_body(req), ecewo_req_body_len(req)) == -1 ||
	    !csrf_verify(req, &form)) {
		form_free(&form);
		view_error(req, res, ECEWO_FORBIDDEN,
		    i18n_t(ctx_lang(req), "errors.bad_request"));
		return;
	}

	lang = form_get(&form, "lang");
	next = form_get(&form, "next");

	ctx = ctx_get(req);

	if (lang != NULL && i18n_supported(lang) &&
	    admin_update_language(ctx->user.name, lang) == 0) {
		str_lcpy(ctx->user.language, lang, sizeof(ctx->user.language));
		str_lcpy(ctx->lang, lang, sizeof(ctx->lang));
	}

	form_free(&form);
	view_redirect(res, url_is_safe_next(next) ? next : "/");
}
