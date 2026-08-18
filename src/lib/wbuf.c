/*
 * awesome-personal-list - growable byte buffer.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wbuf.h"
#include "xmalloc.h"

void
wbuf_init(struct wbuf *b, size_t hint)
{
	if (hint == 0)
		hint = 64;

	b->data = xmalloc(hint);
	b->length = hint;
	b->offset = 0;
	b->owner_api = 0;
}

struct wbuf *
wbuf_alloc(size_t hint)
{
	struct wbuf	*b;

	b = xmalloc(sizeof(*b));
	wbuf_init(b, hint);
	b->owner_api = 1;

	return (b);
}

static void
wbuf_grow(struct wbuf *b, size_t need)
{
	size_t	newlen;

	if (b->offset + need <= b->length)
		return;

	newlen = b->length ? b->length : 64;
	while (newlen < b->offset + need)
		newlen *= 2;

	b->data = xrealloc(b->data, newlen);
	b->length = newlen;
}

void
wbuf_append(struct wbuf *b, const void *data, size_t len)
{
	if (len == 0)
		return;

	wbuf_grow(b, len);
	memcpy(b->data + b->offset, data, len);
	b->offset += len;
}

void
wbuf_appendv(struct wbuf *b, const char *fmt, va_list ap)
{
	va_list		copy;
	int		n;
	char		stackbuf[512];

	va_copy(copy, ap);
	n = vsnprintf(stackbuf, sizeof(stackbuf), fmt, copy);
	va_end(copy);

	if (n < 0)
		return;

	if ((size_t)n < sizeof(stackbuf)) {
		wbuf_append(b, stackbuf, (size_t)n);
		return;
	}

	wbuf_grow(b, (size_t)n + 1);
	vsnprintf((char *)(b->data + b->offset), (size_t)n + 1, fmt, ap);
	b->offset += (size_t)n;
}

void
wbuf_appendf(struct wbuf *b, const char *fmt, ...)
{
	va_list	ap;

	va_start(ap, fmt);
	wbuf_appendv(b, fmt, ap);
	va_end(ap);
}

void
wbuf_reset(struct wbuf *b)
{
	b->offset = 0;
}

void
wbuf_cleanup(struct wbuf *b)
{
	free(b->data);
	b->data = NULL;
	b->length = 0;
	b->offset = 0;
}

void
wbuf_free(struct wbuf *b)
{
	if (b == NULL)
		return;

	wbuf_cleanup(b);

	if (b->owner_api)
		free(b);
}

char *
wbuf_stringify(struct wbuf *b, size_t *len)
{
	wbuf_grow(b, 1);
	b->data[b->offset] = '\0';

	if (len != NULL)
		*len = b->offset;

	return ((char *)b->data);
}

/*
 * Replaces every occurrence of `needle` in `b` with `repl`. Rebuilds into a
 * fresh buffer rather than shifting bytes in place, since replacement text
 * length usually differs from the needle's.
 */
void
wbuf_replace_string(struct wbuf *b, const char *needle, const void *repl,
    size_t repl_len)
{
	struct wbuf	 out;
	size_t		 nlen, i;
	uint8_t		*p;
	int		 owner_api;

	nlen = strlen(needle);
	if (nlen == 0 || b->offset == 0)
		return;

	/* wbuf_init() always sets owner_api=0 on `out`; preserve b's own,
	 * or a wbuf_alloc()'d buffer silently stops being freed by
	 * wbuf_free() after this call -- a leak on every substitution. */
	owner_api = b->owner_api;

	wbuf_init(&out, b->offset);

	i = 0;
	while (i < b->offset) {
		if (i + nlen <= b->offset &&
		    memcmp(b->data + i, needle, nlen) == 0) {
			wbuf_append(&out, repl, repl_len);
			i += nlen;
			continue;
		}

		p = b->data + i;
		wbuf_append(&out, p, 1);
		i++;
	}

	wbuf_cleanup(b);
	*b = out;
	b->owner_api = owner_api;
}
