/*
 * awesome-personal-list - repo-hosting-provider language detection.
 * One HTTP GET + a JSON field per provider (GitHub/GitLab REST APIs), same
 * shape as swh.h's Software Heritage client. Dispatch is a small static
 * table keyed on the URL's host (see lang_client.c) -- adding another
 * provider is one function plus one table row, not a redesign.
 */

#ifndef AWESOME_PERSONAL_LIST_LANG_H
#define AWESOME_PERSONAL_LIST_LANG_H

#include <stddef.h>

int	lang_init(void);
void	lang_cleanup(void);

/*
 * Writes the detected dominant language into `out` (NUL-terminated, up to
 * outlen) and returns 0 on success. Returns -1 if the host isn't a
 * recognized provider, the repo/project couldn't be resolved, or the
 * provider reported no dominant language -- caller leaves the language
 * field as-is, it is not a transport-retry signal like swh_check_origin().
 *
 * `*rate_remaining` is set to the calls-remaining count from whichever
 * provider was just called (-1 if unknown or no provider matched), so a
 * caller looping over many sources can stop early the same way
 * swh_checker_main.c does for Software Heritage.
 */
int	lang_detect(const char *original_url, const char *github_token,
	    const char *gitlab_token, char *out, size_t outlen,
	    long *rate_remaining);

#endif /* !AWESOME_PERSONAL_LIST_LANG_H */
