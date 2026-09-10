/* awesome-personal-list - GET /AWESOME.md, always current, generated on request. */

#include <stdlib.h>
#include <string.h>

#include "ecewo.h"

#include "controllers.h"
#include "middleware.h"
#include "readme.h"

void
route_readme(ecewo_request_t *req, ecewo_response_t *res)
{
	char	*md;

	if (!require_auth(req, res))
		return;

	md = readme_render();
	ecewo_header_set(res, "Content-Type", "text/markdown; charset=utf-8");
	ecewo_send(res, ECEWO_OK, md, strlen(md));
	free(md);
}
