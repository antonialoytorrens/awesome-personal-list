/*
 * awesome-personal-list - small string helpers used by the model layer.
 */

#ifndef AWESOME_PERSONAL_LIST_STRUTIL_H
#define AWESOME_PERSONAL_LIST_STRUTIL_H

#include <stddef.h>

#define STR_OK		1
#define STR_ERROR	0

/*
 * Copies src into dst, always NUL-terminating, and returns strlen(src).
 * Callers detect truncation with `>= sizeof(dst)`.
 */
size_t	str_lcpy(char *, const char *, size_t);

/*
 * Field splitter. Empty fields are skipped rather than stored, and `out` is
 * NULL-terminated. Destructive: separators in `input` are overwritten.
 * Returns the number stored.
 */
int	str_split(char *, const char *, char **, size_t);

/* strtoll with range checking. *err is STR_OK or STR_ERROR. */
long long str_tonum(const char *, int, long long, long long, int *);

/* Hex-encodes len bytes into out, which must hold (len * 2) + 1 bytes. */
void	str_hex(const unsigned char *, size_t, char *, size_t);

/* Length-independent comparison; both strings must be NUL-terminated. */
int	str_timingsafe_equal(const char *, const char *);

/* Lowercases in place. */
void	str_tolower(char *);

/* 1 if s starts with prefix. */
int	str_hasprefix(const char *, const char *);

/* Case-insensitive substring search. NULL if not found. */
char	*str_casestr(const char *haystack, const char *needle);

#endif /* !AWESOME_PERSONAL_LIST_STRUTIL_H */
