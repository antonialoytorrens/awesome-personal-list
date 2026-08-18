/*
 * awesome-personal-list - HTML template loading.
 * Read from disk once at startup into the app-lifetime arena, looked up by
 * basename, so an operator can edit a template and restart without a rebuild.
 */

#include <sys/types.h>
#include <sys/stat.h>

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "app.h"
#include "models.h"
#include "views.h"
#include "wbuf.h"
#include "xmalloc.h"

#define TEMPLATES_MAX	32

struct template {
	char		name[64];
	const char	*data;
	size_t		len;
};

static struct template	templates[TEMPLATES_MAX];
static size_t		template_count;

int
templates_load(ecewo_app_t *app, const char *dir)
{
	DIR		*d;
	struct dirent	*dp;
	struct wbuf	 buf;
	ecewo_arena_t	*arena;
	char		 path[PATH_MAX_LEN];
	size_t		 nlen, blen;
	char		*copy;

	arena = ecewo_app_arena(app);
	template_count = 0;

	if ((d = opendir(dir)) == NULL) {
		app_log(LOG_ERR, "cannot open template directory %s: %s",
		    dir, strerror(errno));
		return (-1);
	}

	while ((dp = readdir(d)) != NULL) {
		if (dp->d_name[0] == '.')
			continue;

		nlen = strlen(dp->d_name);
		if (nlen >= sizeof(templates[0].name))
			continue;

		if (snprintf(path, sizeof(path), "%s/%s", dir,
		    dp->d_name) >= (int)sizeof(path))
			continue;

		wbuf_init(&buf, 4096);
		if (store_read(path, &buf) == -1) {
			wbuf_cleanup(&buf);
			continue;
		}

		if (template_count == TEMPLATES_MAX) {
			app_log(LOG_WARNING, "too many templates, skipping %s",
			    dp->d_name);
			wbuf_cleanup(&buf);
			continue;
		}

		blen = buf.offset;
		copy = ecewo_alloc(arena, blen);
		memcpy(copy, buf.data, blen);
		wbuf_cleanup(&buf);

		memcpy(templates[template_count].name, dp->d_name, nlen);
		templates[template_count].name[nlen] = '\0';
		templates[template_count].data = copy;
		templates[template_count].len = blen;
		template_count++;
	}

	closedir(d);
	return (0);
}

const char *
template_get(const char *name, size_t *len)
{
	size_t	i;

	for (i = 0; i < template_count; i++) {
		if (!strcmp(templates[i].name, name)) {
			if (len != NULL)
				*len = templates[i].len;
			return (templates[i].data);
		}
	}

	return (NULL);
}
