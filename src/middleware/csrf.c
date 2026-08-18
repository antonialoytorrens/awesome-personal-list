/* awesome-personal-list - CSRF tokens: one per session, embedded in every form. */

#include <string.h>

#include "ecewo.h"
#include "ecewo-session.h"

#include "form.h"
#include "middleware.h"
#include "models.h"
#include "strutil.h"

const char *
csrf_token(ecewo_request_t *req)
{
	struct req_ctx	*ctx = ctx_get(req);
	char		*token;

	if (!ctx->authed)
		return ("");

	token = ecewo_session_get((ecewo_session_t *)ctx->sess, "csrf",
	    ecewo_req_arena(req));

	return (token != NULL ? token : "");
}

int
csrf_verify(ecewo_request_t *req, const struct form *form)
{
	const char	*expected, *supplied;

	if ((supplied = form_get(form, "csrf")) == NULL)
		return (0);

	expected = csrf_token(req);

	return (expected[0] != '\0' && str_timingsafe_equal(expected, supplied));
}
