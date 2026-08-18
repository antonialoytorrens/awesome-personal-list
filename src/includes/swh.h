/*
 * awesome-personal-list - Software Heritage API client (libcurl).
 * A single per-origin check: GET /api/1/origin/<url>/get/. There is no bulk
 * "is this known" endpoint. Rate limit is read back from the response
 * headers rather than assumed, so the caller can throttle adaptively
 * whether it is authenticated or not.
 */

#ifndef AWESOME_PERSONAL_LIST_SWH_H
#define AWESOME_PERSONAL_LIST_SWH_H

struct swh_result {
	int	status;		/* SWH_STATUS_ARCHIVED or SWH_STATUS_NOT_FOUND */
	long	rate_remaining;	/* -1 if the header was absent */
	long	rate_reset;	/* unix time; 0 if the header was absent */
};

int	swh_init(void);
void	swh_cleanup(void);

/*
 * `bearer_token` may be NULL for an unauthenticated request. Returns 0 on
 * any definitive answer (archived or not), -1 on a transport/parse failure
 * (caller should retry later, not record a result).
 */
int	swh_check_origin(const char *original_url, const char *bearer_token,
	    struct swh_result *out);

#endif /* !AWESOME_PERSONAL_LIST_SWH_H */
