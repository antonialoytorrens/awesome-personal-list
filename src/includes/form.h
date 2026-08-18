/*
 * awesome-personal-list - application/x-www-form-urlencoded body parsing.
 * ecewo exposes ecewo_query() for the URL query string but has no accessor
 * for form bodies, so this provides one.
 */

#ifndef AWESOME_PERSONAL_LIST_FORM_H
#define AWESOME_PERSONAL_LIST_FORM_H

#include <stddef.h>

/*
 * Just needs to cover a manual multi-select on one /sources page (up to
 * PAGE_SIZES' max of 200, plus a handful of fixed fields) -- "select all"
 * bypasses this entirely via the all_matching bulk-action flag, which never
 * enumerates individual rows. See route_sources_bulk().
 */
#define FORM_MAX_FIELDS	256

struct form_field {
	char	*key;
	char	*value;		/* never NULL; "" for a valueless pair */
};

struct form {
	struct form_field	fields[FORM_MAX_FIELDS];
	size_t			count;
	char			*scratch;	/* decoded copy of the body */
};

/*
 * Parses len bytes of urlencoded body. Returns 0 on success, -1 if the body
 * is unusable. Everything hangs off form->scratch; release with form_free().
 */
int		 form_parse(struct form *, const void *, size_t);
void		 form_free(struct form *);

/* Returns NULL when absent. A present-but-empty field returns "". */
const char	*form_get(const struct form *, const char *);

/* Percent/plus decoding in place. Returns the new length. */
size_t		 form_urldecode(char *);

#endif /* !AWESOME_PERSONAL_LIST_FORM_H */
