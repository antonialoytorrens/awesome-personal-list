/*
 * awesome-personal-list - auto-classification by programming language.
 * Tags a repo with its language category (e.g. "C" -> lang-c) once the
 * language is known -- typed by hand at creation, or filled in later by
 * the background language checker. Nothing here is final -- the owner
 * edits the suggestion before saving.
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
int	classify_source(const char *language, struct classified_category *out,
	    size_t max);

/* Canonical display name for a case/alias variant (e.g. "c++", "cpp",
 * "golang"), or NULL if raw is empty or not a recognized language -- caller
 * keeps the original text as typed/detected. */
const char *classify_canonical_language(const char *raw);

#endif /* !AWESOME_PERSONAL_LIST_CLASSIFY_H */
