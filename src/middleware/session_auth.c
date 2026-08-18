/*
 * awesome-personal-list - session middleware, backed by ecewo-session.
 * Global middleware: ecewo_use() runs it for every request, resolving the
 * session once; handlers read the result out of the request context. Login
 * is compulsory everywhere in this app -- there is no public mode and no
 * /register route.
 *
 * Sessions are in-memory (a restart logs the owner out) -- acceptable for a
 * single-user personal tool, and simpler than a hand-rolled sessions file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ecewo.h"
#include "ecewo-cookie.h"
#include "ecewo-session.h"

#include "app.h"
#include "helpers.h"
#include "i18n.h"
#include "middleware.h"
#include "models.h"
#include "strutil.h"
#include "xmalloc.h"

#define CTX_KEY	"awesome-personal-list.ctx"

static struct req_ctx	*ctx_load(ecewo_request_t *);
static ecewo_cookie_options_t *session_cookie_opts(int max_age);

/*
 * Resolves the language to render this request in: the admin's saved
 * preference once logged in (there's exactly one account, so this *is*
 * "the user's" language); before login, the only reachable page is
 * /login, so the Accept-Language header stands in for a saved preference.
 */
static void
ctx_load_lang(ecewo_request_t *req, struct req_ctx *ctx)
{
	const char	*negotiated;

	if (ctx->authed && i18n_supported(ctx->user.language)) {
		str_lcpy(ctx->lang, ctx->user.language, sizeof(ctx->lang));
		return;
	}

	if (!ctx->authed && (negotiated = i18n_negotiate(
	    ecewo_header_get(req, "Accept-Language"))) != NULL) {
		str_lcpy(ctx->lang, negotiated, sizeof(ctx->lang));
		return;
	}

	str_lcpy(ctx->lang, "en", sizeof(ctx->lang));
}

static struct req_ctx *
ctx_load(ecewo_request_t *req)
{
	struct req_ctx	*ctx;
	char		*name;

	if ((ctx = ecewo_context_get(req, CTX_KEY)) != NULL)
		return (ctx);

	ctx = ecewo_alloc(ecewo_req_arena(req), sizeof(*ctx));
	if (ctx == NULL)
		fatal("ctx_load: request arena exhausted");

	memset(ctx, 0, sizeof(*ctx));
	ecewo_context_set(req, CTX_KEY, ctx);

	if ((ctx->sess = (struct ecewo_session_s *)
	    ecewo_session_from_request(req)) != NULL) {
		name = ecewo_session_get((ecewo_session_t *)ctx->sess, "user",
		    ecewo_req_arena(req));

		if (name != NULL && admin_lookup(name, &ctx->user) == 0)
			ctx->authed = 1;
		else
			ctx->sess = NULL;
	}

	ctx_load_lang(req, ctx);

	return (ctx);
}

void
ctx_middleware(ecewo_request_t *req, ecewo_response_t *res, ecewo_next_t next)
{
	(void)ctx_load(req);
	next(req, res);
}

struct req_ctx *
ctx_get(ecewo_request_t *req)
{
	return (ctx_load(req));
}

int
ctx_authed(ecewo_request_t *req)
{
	return (ctx_load(req)->authed);
}

const char *
ctx_user(ecewo_request_t *req)
{
	struct req_ctx	*ctx = ctx_load(req);

	return (ctx->authed ? ctx->user.name : "");
}

const char *
ctx_lang(ecewo_request_t *req)
{
	return (ctx_load(req)->lang);
}

int
require_auth(ecewo_request_t *req, ecewo_response_t *res)
{
	char	 loc[PATH_MAX_LEN + 32];
	char	*enc;

	if (ctx_authed(req))
		return (1);

	enc = url_encode_str(ecewo_req_path(req));
	snprintf(loc, sizeof(loc), "/login?next=%s", enc);
	free(enc);

	ecewo_redirect(res, ECEWO_SEE_OTHER, loc);
	return (0);
}

/* max_age 0 (logout) makes this an expiring Set-Cookie, deleting it client-side. */
static ecewo_cookie_options_t *
session_cookie_opts(int max_age)
{
	ecewo_cookie_options_t	*opts;

	if ((opts = ecewo_cookie_options_new()) == NULL) {
		app_log(LOG_ERR, "ecewo_cookie_options_new failed");
		return (NULL);
	}

	ecewo_cookie_options_set_path(opts, "/");
	ecewo_cookie_options_set_max_age(opts, max_age);
	ecewo_cookie_options_set_http_only(opts, 1);
	ecewo_cookie_options_set_same_site(opts, ECEWO_COOKIE_SAMESITE_LAX);
	ecewo_cookie_options_set_secure(opts, app_cfg.tls_enabled ? 1 : 0);

	return (opts);
}

void
auth_login(ecewo_request_t *req, ecewo_response_t *res, const struct admin *u)
{
	struct req_ctx		*ctx;
	ecewo_session_t		*sess;
	ecewo_cookie_options_t	*opts;
	char			 csrf[TOKEN_HEX_LEN];

	ctx = ctx_load(req);

	sess = ecewo_session_create(ecewo_req_app(req), SESSION_TTL);
	if (sess == NULL) {
		app_log(LOG_ERR, "ecewo_session_create failed for %s", u->name);
		return;
	}

	token_generate(csrf, sizeof(csrf));
	ecewo_session_set(sess, "user", u->name);
	ecewo_session_set(sess, "csrf", csrf);

	ctx->sess = (struct ecewo_session_s *)sess;
	ctx->user = *u;
	ctx->authed = 1;

	if ((opts = session_cookie_opts(SESSION_TTL)) != NULL) {
		ecewo_session_send(res, sess, opts);
		ecewo_cookie_options_free(opts);
	}
}

void
auth_logout(ecewo_request_t *req, ecewo_response_t *res)
{
	struct req_ctx		*ctx;
	ecewo_cookie_options_t	*opts;

	ctx = ctx_load(req);

	if (ctx->sess != NULL && (opts = session_cookie_opts(0)) != NULL) {
		ecewo_session_destroy(res, (ecewo_session_t *)ctx->sess, opts);
		ecewo_cookie_options_free(opts);
	}

	memset(ctx, 0, sizeof(*ctx));
}
