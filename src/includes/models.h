/*
 * awesome-personal-list - storage layer.
 * Nothing here knows about HTTP; everything is plain C over the data
 * directory. No database: sources/categories are one JSON file per record,
 * admin/swh_cache are TSV. Sessions are ecewo-session, not a flat file.
 * Single-threaded caller, so no locking.
 */

#ifndef AWESOME_PERSONAL_LIST_MODELS_H
#define AWESOME_PERSONAL_LIST_MODELS_H

#include <sys/types.h>
#include <time.h>

#include "app.h"
#include "wbuf.h"

/* ---------------------------------------------------------------- store */

struct store_ent {
	char		name[NAME_MAX_LEN];
	time_t		mtime;
};

struct store_list {
	struct store_ent	*ents;
	size_t			 count;
};

int	store_verify_dirs(void);
int	store_valid_slug(const char *);
int	store_path(char *, size_t, const char *, const char *, const char *);
int	store_exists(const char *);
int	store_read(const char *, struct wbuf *);
int	store_write(const char *, const void *, size_t);
int	store_unlink(const char *);
int	store_list(const char *, const char *, struct store_list *);
void	store_list_free(struct store_list *);

/* Lowercases, strips scheme/host punctuation into a safe slug. */
int	store_slugify_url(const char *, char *, size_t);

/* -------------------------------------------------------------- sources */

int	source_read(const char *slug, struct source *);
int	source_write(const struct source *);
int	source_delete(const char *slug);
int	source_list(struct source **, size_t *);
void	source_list_free(struct source *);
/* O(1) slug lookup, not a directory scan -- see source.c. -1 if absent. */
int	source_exists_for_url(const char *url, struct source *);
/* The owner's name override if set, else derived from original_url. */
const char *source_display_name(const struct source *);

int	source_has_category(const struct source *, const char *slug);
/* Adds slug if there's room and it's not already present. No-op for
 * UNCATEGORIZED_SLUG, which is never actually stored -- see source.c. */
void	source_add_category(struct source *, const char *slug);
/* Removes slug if present. Returns 1 if it was, 0 otherwise. */
int	source_remove_category(struct source *, const char *slug);

/* slug == UNCATEGORIZED_SLUG matches sources with no category at all. */
int	source_in_category(const struct source *, const char *slug);
/* Case-insensitive substring match against display name or description. */
int	source_matches_query(const struct source *, const char *q);
/* AND of the three -- an empty filter/language/q is always satisfied. */
int	source_matches_filter(const struct source *, const char *filter,
	    const char *language, const char *q);

/* ------------------------------------------------------------ categories */

int	category_read(const char *slug, struct category *);
int	category_write(const struct category *);
/* Unassigns slug from every source that has it, then deletes the category.
 * Fails only for UNCATEGORIZED_SLUG, which can't be deleted. */
int	category_delete(const char *slug);
int	category_list(struct category **, size_t *);
void	category_list_free(struct category *);
int	category_seed_defaults(void);

/* ----------------------------------------------------------------- admin */

int	admin_exists(void);
int	admin_lookup(const char *name, struct admin *);
int	admin_create(const char *name, const char *password);
int	admin_verify(const char *name, const char *password, struct admin *);
int	admin_update_language(const char *name, const char *lang);
int	admin_update_name(const char *old_name, const char *new_name);

/* Random hex token, TOKEN_HEX_LEN bytes including the NUL. Used for argon2
 * salts and, by the session middleware, for CSRF tokens. */
void	token_generate(char *, size_t);

/* --------------------------------------------------------------- swh_cache */

/* checked_at == 0 and status == SWH_STATUS_UNKNOWN when never checked. */
int	swh_cache_get(const char *original_url, int *status, time_t *checked_at);
int	swh_cache_set(const char *original_url, int status, time_t checked_at);

#endif /* !AWESOME_PERSONAL_LIST_MODELS_H */
