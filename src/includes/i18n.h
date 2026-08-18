/*
 * awesome-personal-list - translation catalogs.
 * Read once at startup from the locale directory's *.properties files, the
 * same "read from disk once, restart to pick up an edit" model as
 * src/views/templates.c.
 * Framework-free (no ecewo.h), like helpers.h, so the checker binary has no
 * business linking the web framework just because it links this file.
 */

#ifndef AWESOME_PERSONAL_LIST_I18N_H
#define AWESOME_PERSONAL_LIST_I18N_H

#include <stddef.h>

#include "wbuf.h"

int		 i18n_load(const char *dir);

/* Falls back to "en", then to the bare key, so a missing translation is
 * visibly broken rather than silently blank. */
const char	*i18n_t(const char *lang, const char *key);
int		 i18n_supported(const char *lang);

/* Sorted list of loaded locale codes, for building the <select>. */
const char	*const *i18n_locales(size_t *count);
const char	*i18n_native_name(const char *code);

/* First Accept-Language subtag that matches a loaded locale, else NULL. */
const char	*i18n_negotiate(const char *accept_language);

/* Rewrites every [[key]] marker in buf with i18n_t(lang, key), HTML-escaped.
 * A different delimiter than the $VAR$ engine's on purpose: these are
 * literal bytes in trusted template source, never a runtime-substituted
 * value, so they can't collide with tpl_set()'s '$' escaping. */
void		 i18n_translate(struct wbuf *buf, const char *lang);

#endif /* !AWESOME_PERSONAL_LIST_I18N_H */
