/*
 * awesome-personal-list - filesystem primitives.
 * The only module that touches the data directory directly.
 */

#include <sys/types.h>
#include <sys/stat.h>

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include "app.h"
#include "models.h"
#include "strutil.h"
#include "translit.h"
#include "xmalloc.h"

static int	store_cmp_name(const void *, const void *);
static int	mkdir_p(const char *);

static int
mkdir_p(const char *path)
{
	if (mkdir(path, 0750) == -1 && errno != EEXIST)
		return (-1);

	return (0);
}

int
store_verify_dirs(void)
{
	if (mkdir_p(DATA_DIR) == -1)
		return (-1);
	if (mkdir_p(SOURCES_DIR) == -1)
		return (-1);
	if (mkdir_p(CATEGORIES_DIR) == -1)
		return (-1);
	if (mkdir_p(BODY_TMP_DIR) == -1)
		return (-1);

	return (0);
}

/*
 * Slugs are single path components: no separators, no traversal, no
 * dotfiles.
 */
int
store_valid_slug(const char *name)
{
	size_t		len, i;

	if (name == NULL)
		return (0);

	len = strlen(name);
	if (len == 0 || len >= NAME_MAX_LEN)
		return (0);

	if (name[0] == '.' || name[0] == '-')
		return (0);

	for (i = 0; i < len; i++) {
		unsigned char c = (unsigned char)name[i];

		if (!(isalnum(c) || c == '.' || c == '_' || c == '-'))
			return (0);
	}

	if (strstr(name, "..") != NULL)
		return (0);

	return (1);
}

int
store_path(char *out, size_t len, const char *dir, const char *slug,
    const char *ext)
{
	int		ret;

	if (!store_valid_slug(slug))
		return (-1);

	if (ext == NULL)
		ext = "";

	ret = snprintf(out, len, "%s/%s%s", dir, slug, ext);
	if (ret < 0 || (size_t)ret >= len)
		return (-1);

	return (0);
}

int
store_exists(const char *path)
{
	struct stat	st;

	if (stat(path, &st) == -1)
		return (0);

	return (S_ISREG(st.st_mode));
}

int
store_read(const char *path, struct wbuf *out)
{
	int		fd;
	ssize_t		r;
	uint8_t		tmp[8192];

	if ((fd = open(path, O_RDONLY | O_NOFOLLOW)) == -1)
		return (-1);

	for (;;) {
		r = read(fd, tmp, sizeof(tmp));
		if (r == -1) {
			if (errno == EINTR)
				continue;
			close(fd);
			return (-1);
		}
		if (r == 0)
			break;
		wbuf_append(out, tmp, (size_t)r);
	}

	close(fd);
	return (0);
}

/*
 * Write via a temp file plus rename so a reader never sees a half-written
 * record, and a crash mid-save leaves the previous version intact.
 */
int
store_write(const char *path, const void *data, size_t len)
{
	int		fd, ret;
	size_t		off;
	ssize_t		w;
	char		tmp[PATH_MAX_LEN];

	ret = snprintf(tmp, sizeof(tmp), "%s.tmp.%d", path, (int)getpid());
	if (ret < 0 || (size_t)ret >= sizeof(tmp))
		return (-1);

	fd = open(tmp, O_WRONLY | O_CREAT | O_TRUNC | O_NOFOLLOW, 0640);
	if (fd == -1)
		return (-1);

	off = 0;
	while (off < len) {
		w = write(fd, (const uint8_t *)data + off, len - off);
		if (w == -1) {
			if (errno == EINTR)
				continue;
			goto fail;
		}
		off += (size_t)w;
	}

	if (fsync(fd) == -1)
		goto fail;
	if (close(fd) == -1) {
		fd = -1;
		goto fail;
	}

	if (rename(tmp, path) == -1) {
		fd = -1;
		goto fail;
	}

	return (0);

fail:
	if (fd != -1)
		close(fd);
	unlink(tmp);
	return (-1);
}

int
store_unlink(const char *path)
{
	if (unlink(path) == -1 && errno != ENOENT)
		return (-1);

	return (0);
}

int
store_list(const char *dir, const char *suffix, struct store_list *out)
{
	DIR		*d;
	struct dirent	*dp;
	struct stat	 st;
	size_t		 cap, slen, nlen;
	char		 full[PATH_MAX_LEN];
	struct store_ent *ents, *tmp;

	out->ents = NULL;
	out->count = 0;

	if ((d = opendir(dir)) == NULL)
		return (errno == ENOENT ? 0 : -1);

	cap = 32;
	ents = xcalloc(cap, sizeof(*ents));
	slen = (suffix != NULL) ? strlen(suffix) : 0;

	while ((dp = readdir(d)) != NULL) {
		if (dp->d_name[0] == '.')
			continue;

		nlen = strlen(dp->d_name);

		if (slen > 0) {
			if (nlen <= slen)
				continue;
			if (strcmp(dp->d_name + nlen - slen, suffix))
				continue;
			nlen -= slen;
		}

		if (nlen >= NAME_MAX_LEN)
			continue;

		if (snprintf(full, sizeof(full), "%s/%s", dir,
		    dp->d_name) >= (int)sizeof(full))
			continue;

		if (stat(full, &st) == -1 || !S_ISREG(st.st_mode))
			continue;

		if (out->count == cap) {
			cap *= 2;
			tmp = xcalloc(cap, sizeof(*ents));
			memcpy(tmp, ents, out->count * sizeof(*ents));
			free(ents);
			ents = tmp;
		}

		memcpy(ents[out->count].name, dp->d_name, nlen);
		ents[out->count].name[nlen] = '\0';
		ents[out->count].mtime = st.st_mtime;
		out->count++;
	}

	closedir(d);

	if (out->count > 0)
		qsort(ents, out->count, sizeof(*ents), store_cmp_name);

	out->ents = ents;
	return (0);
}

void
store_list_free(struct store_list *list)
{
	free(list->ents);
	list->ents = NULL;
	list->count = 0;
}

static int
store_cmp_name(const void *a, const void *b)
{
	const struct store_ent	*ea = a, *eb = b;

	return (strcasecmp(ea->name, eb->name));
}

/*
 * Turns a URL (or a free-text name, e.g. a category name) into a
 * filesystem-safe slug: strips the scheme, lowercases, transliterates
 * accented Latin letters to their ASCII base letter (see translit.h), and
 * collapses everything else that is not [a-z0-9] into a single '-'. Good
 * enough as a stable, human-readable filename; collisions (rare -- would
 * need two different inputs to normalize identically) are caught by the
 * caller via store_exists() before use.
 */
int
store_slugify_url(const char *url, char *out, size_t len)
{
	const char	*p;
	size_t		 o;
	int		 dash;

	if (url == NULL || len < 2)
		return (-1);

	p = url;
	if (str_hasprefix(p, "https://"))
		p += 8;
	else if (str_hasprefix(p, "http://"))
		p += 7;

	o = 0;
	dash = 0;

	for (; *p != '\0' && o < len - 1; ) {
		unsigned int	cp = utf8_decode(&p);

		if (cp < 0x80) {
			unsigned char	c = (unsigned char)cp;

			if (isalnum(c)) {
				out[o++] = (char)tolower(c);
				dash = 0;
				continue;
			}
		} else {
			const char	*rep = translit_ascii(cp);

			if (rep != NULL) {
				for (; *rep != '\0' && o < len - 1; rep++)
					out[o++] = *rep;
				dash = 0;
				continue;
			}
		}

		if (o > 0 && !dash) {
			out[o++] = '-';
			dash = 1;
		}
	}

	while (o > 0 && out[o - 1] == '-')
		o--;

	out[o] = '\0';

	return (o == 0 ? -1 : 0);
}
