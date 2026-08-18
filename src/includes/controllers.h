/* awesome-personal-list - route handlers. */

#ifndef AWESOME_PERSONAL_LIST_CONTROLLERS_H
#define AWESOME_PERSONAL_LIST_CONTROLLERS_H

#include "ecewo.h"

/* auth */
void	route_login_get(ecewo_request_t *, ecewo_response_t *);
void	route_login_post(ecewo_request_t *, ecewo_response_t *);
void	route_logout(ecewo_request_t *, ecewo_response_t *);

/* profile + language */
void	route_profile_get(ecewo_request_t *, ecewo_response_t *);
void	route_profile_post(ecewo_request_t *, ecewo_response_t *);
void	route_lang_set(ecewo_request_t *, ecewo_response_t *);

/* sources */
void	route_home(ecewo_request_t *, ecewo_response_t *);
void	route_sources_list(ecewo_request_t *, ecewo_response_t *);
void	route_source_get(ecewo_request_t *, ecewo_response_t *);
void	route_source_post(ecewo_request_t *, ecewo_response_t *);
void	route_source_new_get(ecewo_request_t *, ecewo_response_t *);
void	route_source_new_post(ecewo_request_t *, ecewo_response_t *);

/* categories */
void	route_categories_list(ecewo_request_t *, ecewo_response_t *);
void	route_category_create(ecewo_request_t *, ecewo_response_t *);
void	route_category_get(ecewo_request_t *, ecewo_response_t *);
void	route_category_post(ecewo_request_t *, ecewo_response_t *);
void	route_categories_css(ecewo_request_t *, ecewo_response_t *);

/* sources bulk actions */
void	route_sources_bulk(ecewo_request_t *, ecewo_response_t *);

/* README */
void	route_readme(ecewo_request_t *, ecewo_response_t *);

#endif /* !AWESOME_PERSONAL_LIST_CONTROLLERS_H */
