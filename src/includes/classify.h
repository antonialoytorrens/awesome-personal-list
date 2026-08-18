/*
 * awesome-personal-list - auto-classification for the "New source" form.
 * Tags a repo by programming language and known library/framework/console-
 * homebrew keywords, from whatever description/language the owner typed in.
 * A repo can match several (e.g. a C+SDL project is both); nothing here is
 * final -- the owner edits the suggestion before saving.
 */

#ifndef AWESOME_PERSONAL_LIST_CLASSIFY_H
#define AWESOME_PERSONAL_LIST_CLASSIFY_H

#include "app.h"

struct classified_category {
	char	slug[NAME_MAX_LEN];
	char	name[NAME_MAX_LEN];
};

#define CLASSIFY_MAX_CATEGORIES	8

/* Returns the count written to out (0 if nothing matched confidently). */
int	classify_source(const char *description, const char *language,
	    struct classified_category *out, size_t max);

#endif /* !AWESOME_PERSONAL_LIST_CLASSIFY_H */
