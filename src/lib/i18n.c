/*
 * awesome-personal-list - translation catalogs: the locale directory's .properties
 * files, loaded once at startup, kept for the life of the process (never
 * freed -- same lifetime as the compiled-in templates).
 */

#include <sys/types.h>

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app.h"
#include "helpers.h"
#include "i18n.h"
#include "models.h"
#include "strutil.h"
#include "xmalloc.h"

#define I18N_LOCALES_MAX	8
#define I18N_ENTRIES_MAX	400
#define I18N_KEY_MAX		80
#define I18N_EXT		".properties"

struct i18n_entry {
	char	 key[I18N_KEY_MAX];
	char	*value;
};

struct i18n_locale {
	char			code[8];
	struct i18n_entry	entries[I18N_ENTRIES_MAX];
	size_t			count;
};

static struct i18n_locale	locales[I18N_LOCALES_MAX];
static size_t			locale_count;
static const char		*locale_codes[I18N_LOCALES_MAX];

static const struct {
	const char	*code;
	const char	*name;
} native_names[] = {
	{ "en", "English" },
	{ "es", "Español" },
	{ "ca", "Català" },
};

static int
cmp_entry_key(const void *a, const void *b)
{
	const struct i18n_entry	*ea = a, *eb = b;

	return (strcmp(ea->key, eb->key));
}

static int
cmp_key_entry(const void *keyp, const void *elemp)
{
	const char			*key = keyp;
	const struct i18n_entry	*e = elemp;

	return (strcmp(key, e->key));
}

static int
cmp_locale_code(const void *a, const void *b)
{
	const struct i18n_locale	*la = a, *lb = b;

	return (strcmp(la->code, lb->code));
}

static struct i18n_locale *
find_locale(const char *code)
{
	size_t	i;

	if (code == NULL)
		return (NULL);

	for (i = 0; i < locale_count; i++) {
		if (!strcmp(locales[i].code, code))
			return (&locales[i]);
	}

	return (NULL);
}

/*
 * key=value, UTF-8, one per line. '#'/'!' comments and blank lines are
 * skipped (Java properties convention, minus \ line continuations and
 * \uXXXX escapes -- not needed for short UI strings). Split on the first
 * '=' only, so a value may itself contain '='. Destructive: writes NULs
 * into `data`, which must be a private, already-NUL-terminated copy (see
 * wbuf_stringify()).
 */
static void
parse_properties(char *data, size_t len, struct i18n_locale *loc)
{
	char	*p, *end, *line, *nl, *eq, *key, *val, *kend;
	size_t	 linelen;

	p = data;
	end = data + len;

	while (p < end && loc->count < I18N_ENTRIES_MAX) {
		line = p;
		nl = memchr(p, '\n', (size_t)(end - p));
		linelen = nl ? (size_t)(nl - p) : (size_t)(end - p);
		p = nl ? nl + 1 : end;

		if (linelen > 0 && line[linelen - 1] == '\r')
			linelen--;
		line[linelen] = '\0';

		while (*line == ' ' || *line == '\t')
			line++;

		if (*line == '\0' || *line == '#' || *line == '!')
			continue;

		if ((eq = strchr(line, '=')) == NULL)
			continue;

		*eq = '\0';
		key = line;
		val = eq + 1;

		kend = eq - 1;
		while (kend >= key && (*kend == ' ' || *kend == '\t')) {
			*kend = '\0';
			kend--;
		}
		while (*val == ' ' || *val == '\t')
			val++;

		if (key[0] == '\0')
			continue;

		if (str_lcpy(loc->entries[loc->count].key, key,
		    sizeof(loc->entries[0].key)) >= sizeof(loc->entries[0].key))
			continue;

		loc->entries[loc->count].value = xstrdup(val);
		loc->count++;
	}
}

int
i18n_load(const char *dir)
{
	DIR		*d;
	struct dirent	*dp;
	struct wbuf	 buf;
	char		 path[PATH_MAX_LEN];
	size_t		 nlen, extlen, codelen;
	char		*data;
	size_t		 datalen;

	locale_count = 0;

	if ((d = opendir(dir)) == NULL) {
		app_log(LOG_ERR, "cannot open locale directory %s: %s",
		    dir, strerror(errno));
		return (-1);
	}

	extlen = strlen(I18N_EXT);

	while ((dp = readdir(d)) != NULL) {
		if (dp->d_name[0] == '.')
			continue;

		nlen = strlen(dp->d_name);
		if (nlen <= extlen || strcmp(dp->d_name + nlen - extlen, I18N_EXT))
			continue;

		codelen = nlen - extlen;
		if (codelen == 0 || codelen >= sizeof(locales[0].code))
			continue;

		if (locale_count == I18N_LOCALES_MAX) {
			app_log(LOG_WARNING, "too many locales, skipping %s",
			    dp->d_name);
			continue;
		}

		if (snprintf(path, sizeof(path), "%s/%s", dir,
		    dp->d_name) >= (int)sizeof(path))
			continue;

		wbuf_init(&buf, 4096);
		if (store_read(path, &buf) == -1) {
			wbuf_cleanup(&buf);
			app_log(LOG_WARNING, "cannot read locale file %s", path);
			continue;
		}

		memcpy(locales[locale_count].code, dp->d_name, codelen);
		locales[locale_count].code[codelen] = '\0';
		locales[locale_count].count = 0;

		data = wbuf_stringify(&buf, &datalen);
		parse_properties(data, datalen, &locales[locale_count]);
		wbuf_cleanup(&buf);

		qsort(locales[locale_count].entries, locales[locale_count].count,
		    sizeof(locales[locale_count].entries[0]), cmp_entry_key);

		locale_count++;
	}

	closedir(d);

	qsort(locales, locale_count, sizeof(locales[0]), cmp_locale_code);

	return (0);
}

const char *
i18n_t(const char *lang, const char *key)
{
	struct i18n_locale	*loc;
	struct i18n_entry	*found;

	if ((loc = find_locale(lang)) != NULL) {
		found = bsearch(key, loc->entries, loc->count,
		    sizeof(loc->entries[0]), cmp_key_entry);
		if (found != NULL)
			return (found->value);
	}

	if (loc == NULL || strcmp(lang, "en")) {
		if ((loc = find_locale("en")) != NULL) {
			found = bsearch(key, loc->entries, loc->count,
			    sizeof(loc->entries[0]), cmp_key_entry);
			if (found != NULL)
				return (found->value);
		}
	}

	return (key);
}

int
i18n_supported(const char *lang)
{
	return (find_locale(lang) != NULL);
}

const char *const *
i18n_locales(size_t *count)
{
	size_t	i;

	for (i = 0; i < locale_count; i++)
		locale_codes[i] = locales[i].code;

	if (count != NULL)
		*count = locale_count;

	return (locale_codes);
}

const char *
i18n_native_name(const char *code)
{
	size_t	i;

	for (i = 0; i < sizeof(native_names) / sizeof(native_names[0]); i++) {
		if (!strcmp(native_names[i].code, code))
			return (native_names[i].name);
	}

	return (code);
}

const char *
i18n_negotiate(const char *accept_language)
{
	static char	out[8];
	char		buf[256];
	char		*tags[16];
	char		*tag, *semi, *dash;
	int		n, i;

	if (accept_language == NULL || accept_language[0] == '\0')
		return (NULL);

	str_lcpy(buf, accept_language, sizeof(buf));
	n = str_split(buf, ",", tags, 16);

	for (i = 0; i < n; i++) {
		tag = tags[i];
		while (*tag == ' ')
			tag++;
		if ((semi = strchr(tag, ';')) != NULL)
			*semi = '\0';
		if ((dash = strchr(tag, '-')) != NULL)
			*dash = '\0';
		str_tolower(tag);

		if (i18n_supported(tag)) {
			str_lcpy(out, tag, sizeof(out));
			return (out);
		}
	}

	return (NULL);
}

/*
 * Same "rebuild into a fresh buffer" shape as wbuf_replace_string() (see
 * src/lib/wbuf.c): the token set here is open-ended, keyed by whatever key
 * is between [[ and ]], so a single fixed-needle replace loop doesn't fit.
 */
void
i18n_translate(struct wbuf *buf, const char *lang)
{
	struct wbuf	out;
	size_t		i, j, keylen;
	int		owner_api;
	char		key[I18N_KEY_MAX];
	const char	*val;

	if (buf->offset == 0)
		return;

	owner_api = buf->owner_api;
	wbuf_init(&out, buf->offset);

	i = 0;
	while (i < buf->offset) {
		if (i + 1 < buf->offset && buf->data[i] == '[' &&
		    buf->data[i + 1] == '[') {
			j = i + 2;
			while (j + 1 < buf->offset &&
			    !(buf->data[j] == ']' && buf->data[j + 1] == ']'))
				j++;

			if (j + 1 < buf->offset && buf->data[j] == ']' &&
			    buf->data[j + 1] == ']') {
				keylen = j - (i + 2);
				if (keylen >= sizeof(key))
					keylen = sizeof(key) - 1;
				memcpy(key, buf->data + i + 2, keylen);
				key[keylen] = '\0';

				val = i18n_t(lang, key);
				html_escape(&out, val, strlen(val));

				i = j + 2;
				continue;
			}
		}

		wbuf_append(&out, buf->data + i, 1);
		i++;
	}

	wbuf_cleanup(buf);
	*buf = out;
	buf->owner_api = owner_api;
}
