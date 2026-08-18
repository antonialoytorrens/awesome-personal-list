/*
 * awesome-personal-list - categories.
 * data/categories/<slug>.json. Full CRUD resource, not a hardcoded enum.
 * "uncategorized" is seeded on first run for its name/color metadata, but
 * is never actually assigned to a source -- see source.c. It's the one
 * category that can't be deleted or manually assigned.
 */

#include <sys/types.h>

#include <cjson/cJSON.h>

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "app.h"
#include "models.h"
#include "strutil.h"
#include "validate.h"
#include "wbuf.h"
#include "xmalloc.h"

static void
json_str(cJSON *root, const char *key, char *out, size_t outlen)
{
	cJSON	*v;

	v = cJSON_GetObjectItemCaseSensitive(root, key);
	if (cJSON_IsString(v) && v->valuestring != NULL)
		str_lcpy(out, v->valuestring, outlen);
	else
		out[0] = '\0';
}

int
category_read(const char *slug, struct category *out)
{
	char		 path[PATH_MAX_LEN];
	struct wbuf	 buf;
	cJSON		*root;

	if (store_path(path, sizeof(path), CATEGORIES_DIR, slug, ".json") == -1)
		return (-1);

	wbuf_init(&buf, 512);
	if (store_read(path, &buf) == -1) {
		wbuf_cleanup(&buf);
		return (-1);
	}

	root = cJSON_ParseWithLength((const char *)buf.data, buf.offset);
	wbuf_cleanup(&buf);

	if (root == NULL)
		return (-1);

	memset(out, 0, sizeof(*out));
	json_str(root, "slug", out->slug, sizeof(out->slug));
	json_str(root, "name", out->name, sizeof(out->name));
	json_str(root, "description", out->description,
	    sizeof(out->description));
	json_str(root, "color", out->color, sizeof(out->color));
	if (!v_color(out->color))
		str_lcpy(out->color, CATEGORY_DEFAULT_COLOR, sizeof(out->color));

	cJSON_Delete(root);

	return (out->slug[0] == '\0' ? -1 : 0);
}

int
category_write(const struct category *c)
{
	char		 path[PATH_MAX_LEN];
	cJSON		*root;
	char		*text;
	int		 ret;

	if (store_path(path, sizeof(path), CATEGORIES_DIR, c->slug,
	    ".json") == -1)
		return (-1);

	root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "slug", c->slug);
	cJSON_AddStringToObject(root, "name", c->name);
	cJSON_AddStringToObject(root, "description", c->description);
	cJSON_AddStringToObject(root, "color",
	    v_color(c->color) ? c->color : CATEGORY_DEFAULT_COLOR);

	text = cJSON_Print(root);
	cJSON_Delete(root);

	if (text == NULL)
		return (-1);

	ret = store_write(path, text, strlen(text));
	free(text);

	return (ret);
}

/*
 * Deleting a category unassigns it from every source that has it, rather
 * than refusing while it's "in use" -- UNCATEGORIZED_SLUG is the only
 * category that can't be deleted.
 */
int
category_delete(const char *slug)
{
	char		 path[PATH_MAX_LEN];
	struct source	*srcs;
	size_t		 i, n;

	if (!strcmp(slug, UNCATEGORIZED_SLUG))
		return (-1);

	if (store_path(path, sizeof(path), CATEGORIES_DIR, slug, ".json") == -1)
		return (-1);

	if (source_list(&srcs, &n) == 0) {
		for (i = 0; i < n; i++) {
			if (!source_remove_category(&srcs[i], slug))
				continue;
			srcs[i].updated_at = time(NULL);
			source_write(&srcs[i]);
		}
		source_list_free(srcs);
	}

	return (store_unlink(path));
}

int
category_list(struct category **out, size_t *count)
{
	struct store_list	list;
	struct category		*cats;
	size_t			i, n;

	*out = NULL;
	*count = 0;

	if (store_list(CATEGORIES_DIR, ".json", &list) == -1)
		return (-1);

	cats = xcalloc(list.count ? list.count : 1, sizeof(*cats));
	n = 0;

	for (i = 0; i < list.count; i++) {
		if (category_read(list.ents[i].name, &cats[n]) == 0)
			n++;
	}

	store_list_free(&list);

	*out = cats;
	*count = n;
	return (0);
}

void
category_list_free(struct category *cats)
{
	free(cats);
}

int
category_seed_defaults(void)
{
	struct category	c;

	if (category_read(UNCATEGORIZED_SLUG, &c) == 0)
		return (0);

	memset(&c, 0, sizeof(c));
	str_lcpy(c.slug, UNCATEGORIZED_SLUG, sizeof(c.slug));
	str_lcpy(c.name, "Uncategorized", sizeof(c.name));
	str_lcpy(c.description,
	    "New imports land here until you assign a real category.",
	    sizeof(c.description));
	str_lcpy(c.color, CATEGORY_DEFAULT_COLOR, sizeof(c.color));

	return (category_write(&c));
}
