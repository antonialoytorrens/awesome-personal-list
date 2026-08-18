/* awesome-personal-list - view layer: $VAR$ template engine standing in for res.render(). */

#ifndef AWESOME_PERSONAL_LIST_VIEWS_H
#define AWESOME_PERSONAL_LIST_VIEWS_H

#include "ecewo.h"

#include "app.h"
#include "helpers.h"
#include "wbuf.h"

int		 templates_load(ecewo_app_t *, const char *);
const char	*template_get(const char *, size_t *);

#define VIEW_MAX_RAW	8

struct raw_slot {
	char	*key;
	uint8_t	*val;
	size_t	 len;
};

struct view {
	struct wbuf	*body;
	char		*title;
	struct raw_slot	 raw[VIEW_MAX_RAW];
	size_t		 nraw;
};

struct view	*view_new(const char *);
void		 view_free(struct view *);

#define view_set_lit(v, k, s)	view_set_raw(v, k, s, sizeof(s) - 1)

void	tpl_set(struct wbuf *, const char *, const char *);

void	view_set(struct view *, const char *, const char *);
void	view_set_raw(struct view *, const char *, const void *, size_t);
void	view_setf(struct view *, const char *, const char *, ...)
	    __attribute__((format (printf, 3, 4)));
void	view_title(struct view *, const char *);

void	view_render(ecewo_request_t *, ecewo_response_t *, int, struct view *);
void	view_error(ecewo_request_t *, ecewo_response_t *, int, const char *);
void	view_redirect(ecewo_response_t *, const char *);

#endif /* !AWESOME_PERSONAL_LIST_VIEWS_H */
