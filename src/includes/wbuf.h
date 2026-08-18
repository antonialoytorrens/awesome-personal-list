/*
 * awesome-personal-list - growable byte buffer.
 *
 * Deliberately malloc-backed rather than built on ecewo's arena string
 * builder: the models and the checker binary also run with no request arena
 * to borrow from.
 */

#ifndef AWESOME_PERSONAL_LIST_WBUF_H
#define AWESOME_PERSONAL_LIST_WBUF_H

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

struct wbuf {
	uint8_t		*data;
	size_t		 length;	/* capacity */
	size_t		 offset;	/* bytes in use */
	int		 owner_api;	/* allocated by wbuf_alloc(), free with wbuf_free() */
};

void		 wbuf_init(struct wbuf *, size_t);
struct wbuf	*wbuf_alloc(size_t);
void		 wbuf_append(struct wbuf *, const void *, size_t);
void		 wbuf_appendv(struct wbuf *, const char *, va_list);
void		 wbuf_appendf(struct wbuf *, const char *, ...)
		    __attribute__((format (printf, 2, 3)));
void		 wbuf_reset(struct wbuf *);
void		 wbuf_cleanup(struct wbuf *);
void		 wbuf_free(struct wbuf *);

char		*wbuf_stringify(struct wbuf *, size_t *);

void		 wbuf_replace_string(struct wbuf *, const char *,
		    const void *, size_t);

#define buf_lit(b, s)	wbuf_append(b, s, sizeof(s) - 1)

#endif /* !AWESOME_PERSONAL_LIST_WBUF_H */
