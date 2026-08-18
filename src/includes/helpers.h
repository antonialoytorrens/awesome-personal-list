/*
 * awesome-personal-list - HTML/URL escaping and date formatting.
 *
 * Kept separate from views.h, which pulls in ecewo: the model layer and the
 * checker binary use these and have no business linking the web framework.
 */

#ifndef AWESOME_PERSONAL_LIST_HELPERS_H
#define AWESOME_PERSONAL_LIST_HELPERS_H

#include <time.h>

#include "wbuf.h"

void	 html_escape(struct wbuf *, const char *, size_t);
void	 url_encode(struct wbuf *, const char *, size_t);

/* Caller frees the result. */
char	*html_escape_str(const char *);
char	*url_encode_str(const char *);

void	 fmt_date(time_t, char *, size_t);

/* Last non-empty path segment of a URL, as a display name. */
const char	*repo_display_name(const char *);
/* Skip a leading scheme://; returns url unchanged if none. */
const char	*url_host_path(const char *);

/* Same-origin only: "/x" is fine, "//evil.com/x" is protocol-relative and
 * would send the browser off-site right after a redirect. */
int		 url_is_safe_next(const char *);

#endif /* !AWESOME_PERSONAL_LIST_HELPERS_H */
