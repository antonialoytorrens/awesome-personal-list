/*
 * awesome-personal-list - session/auth and CSRF middleware.
 */

#ifndef AWESOME_PERSONAL_LIST_MIDDLEWARE_H
#define AWESOME_PERSONAL_LIST_MIDDLEWARE_H

#include "ecewo.h"

#include "app.h"
#include "form.h"

/* Global middleware: resolves the session cookie once per request. */
void		 ctx_middleware(ecewo_request_t *, ecewo_response_t *,
		    ecewo_next_t);

struct req_ctx	*ctx_get(ecewo_request_t *);
int		 ctx_authed(ecewo_request_t *);
const char	*ctx_user(ecewo_request_t *);
const char	*ctx_lang(ecewo_request_t *);

/* Login is compulsory for every route in this app -- no public mode. */
int		 require_auth(ecewo_request_t *, ecewo_response_t *);

void		 auth_login(ecewo_request_t *, ecewo_response_t *,
		    const struct admin *);
void		 auth_logout(ecewo_request_t *, ecewo_response_t *);

/* ----------------------------------------------------------------- csrf */

const char	*csrf_token(ecewo_request_t *);
int		 csrf_verify(ecewo_request_t *, const struct form *);

#endif /* !AWESOME_PERSONAL_LIST_MIDDLEWARE_H */
