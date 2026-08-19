/*
 * awesome-personal-list - curated sources.
 * data/sources/<slug>.json, one file per entry, slug derived from
 * original_url.
 */

#include <sys/types.h>

#include <cjson/cJSON.h>

#include <stdlib.h>
#include <string.h>

#include "app.h"
#include "helpers.h"
#include "models.h"
#include "strutil.h"
#include "wbuf.h"
#include "xmalloc.h"

static cJSON	*source_to_json(const struct source *);
static int	 source_from_json(cJSON *, struct source *);

static cJSON *
source_to_json(const struct source *s)
{
	cJSON	*root, *cats;
	int	 i;

	root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "slug", s->slug);
	cJSON_AddStringToObject(root, "name", s->name);
	cJSON_AddStringToObject(root, "original_url", s->original_url);
	cJSON_AddStringToObject(root, "description", s->description);
	cJSON_AddStringToObject(root, "language", s->language);

	cats = cJSON_AddArrayToObject(root, "categories");
	for (i = 0; i < s->category_count; i++)
		cJSON_AddItemToArray(cats, cJSON_CreateString(s->categories[i]));

	cJSON_AddStringToObject(root, "notes", s->notes);
	cJSON_AddNumberToObject(root, "swh_status", s->swh_status);
	cJSON_AddNumberToObject(root, "swh_checked_at",
	    (double)s->swh_checked_at);
	cJSON_AddNumberToObject(root, "created_at", (double)s->created_at);
	cJSON_AddNumberToObject(root, "updated_at", (double)s->updated_at);

	return (root);
}

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

static double
json_num(cJSON *root, const char *key)
{
	cJSON	*v;

	v = cJSON_GetObjectItemCaseSensitive(root, key);
	return (cJSON_IsNumber(v) ? v->valuedouble : 0);
}

static int
source_from_json(cJSON *root, struct source *out)
{
	cJSON	*cats, *item;

	memset(out, 0, sizeof(*out));

	json_str(root, "slug", out->slug, sizeof(out->slug));
	json_str(root, "name", out->name, sizeof(out->name));
	json_str(root, "original_url", out->original_url,
	    sizeof(out->original_url));
	json_str(root, "description", out->description,
	    sizeof(out->description));
	json_str(root, "language", out->language, sizeof(out->language));
	json_str(root, "notes", out->notes, sizeof(out->notes));

	/*
	 * UNCATEGORIZED_SLUG is never actually stored -- "uncategorized"
	 * means category_count == 0, computed wherever it's needed rather
	 * than persisted. Older data written before this may still carry
	 * the literal tag; skip it here so those sources self-heal to
	 * category_count == 0 the moment they're next read.
	 */
	cats = cJSON_GetObjectItemCaseSensitive(root, "categories");
	if (cJSON_IsArray(cats)) {
		cJSON_ArrayForEach(item, cats) {
			if (out->category_count == SOURCE_MAX_CATEGORIES)
				break;
			if (cJSON_IsString(item) && item->valuestring != NULL &&
			    item->valuestring[0] != '\0' &&
			    strcmp(item->valuestring, UNCATEGORIZED_SLUG) != 0) {
				str_lcpy(out->categories[out->category_count],
				    item->valuestring, NAME_MAX_LEN);
				out->category_count++;
			}
		}
	}

	out->swh_status = (int)json_num(root, "swh_status");
	out->swh_checked_at = (time_t)json_num(root, "swh_checked_at");
	out->created_at = (time_t)json_num(root, "created_at");
	out->updated_at = (time_t)json_num(root, "updated_at");

	if (out->slug[0] == '\0' || out->original_url[0] == '\0')
		return (-1);

	return (0);
}

int
source_read(const char *slug, struct source *out)
{
	char		 path[PATH_MAX_LEN];
	struct wbuf	 buf;
	cJSON		*root;
	int		 ret;

	if (store_path(path, sizeof(path), SOURCES_DIR, slug, ".json") == -1)
		return (-1);

	wbuf_init(&buf, 1024);
	if (store_read(path, &buf) == -1) {
		wbuf_cleanup(&buf);
		return (-1);
	}

	root = cJSON_ParseWithLength((const char *)buf.data, buf.offset);
	wbuf_cleanup(&buf);

	if (root == NULL)
		return (-1);

	ret = source_from_json(root, out);
	cJSON_Delete(root);

	return (ret);
}

int
source_write(const struct source *s)
{
	char		 path[PATH_MAX_LEN];
	cJSON		*root;
	char		*text;
	int		 ret;

	if (store_path(path, sizeof(path), SOURCES_DIR, s->slug, ".json") == -1)
		return (-1);

	root = source_to_json(s);
	text = cJSON_Print(root);
	cJSON_Delete(root);

	if (text == NULL)
		return (-1);

	ret = store_write(path, text, strlen(text));
	free(text);

	return (ret);
}

int
source_delete(const char *slug)
{
	char	path[PATH_MAX_LEN];

	if (store_path(path, sizeof(path), SOURCES_DIR, slug, ".json") == -1)
		return (-1);

	return (store_unlink(path));
}

int
source_list(struct source **out, size_t *count)
{
	struct store_list	list;
	struct source		*srcs;
	size_t			i, n;

	*out = NULL;
	*count = 0;

	if (store_list(SOURCES_DIR, ".json", &list) == -1)
		return (-1);

	srcs = xcalloc(list.count ? list.count : 1, sizeof(*srcs));
	n = 0;

	for (i = 0; i < list.count; i++) {
		if (source_read(list.ents[i].name, &srcs[n]) == 0)
			n++;
	}

	store_list_free(&list);

	*out = srcs;
	*count = n;
	return (0);
}

void
source_list_free(struct source *srcs)
{
	free(srcs);
}

/*
 * O(1): the slug is deterministic from original_url (store_slugify_url()),
 * so this is one direct file lookup rather than a full directory scan.
 * Verifies original_url on the result too, in case of a rare slug
 * collision between two different URLs.
 */
int
source_exists_for_url(const char *url, struct source *out)
{
	char		slug[NAME_MAX_LEN];
	struct source	s;

	if (store_slugify_url(url, slug, sizeof(slug)) == -1)
		return (-1);

	if (source_read(slug, &s) == -1)
		return (-1);

	if (strcmp(s.original_url, url) != 0)
		return (-1);

	if (out != NULL)
		*out = s;

	return (0);
}

/* The name shown for a source everywhere it's listed: the owner's override
 * if they've set one, else derived from the URL as always. */
const char *
source_display_name(const struct source *s)
{
	return (s->name[0] != '\0' ? s->name : repo_display_name(s->original_url));
}

int
source_has_category(const struct source *s, const char *slug)
{
	int	i;

	for (i = 0; i < s->category_count; i++) {
		if (!strcmp(s->categories[i], slug))
			return (1);
	}

	return (0);
}

void
source_add_category(struct source *s, const char *slug)
{
	if (!strcmp(slug, UNCATEGORIZED_SLUG))
		return;
	if (source_has_category(s, slug))
		return;
	if (s->category_count == SOURCE_MAX_CATEGORIES)
		return;

	str_lcpy(s->categories[s->category_count], slug, NAME_MAX_LEN);
	s->category_count++;
}

int
source_remove_category(struct source *s, const char *slug)
{
	int	i;

	for (i = 0; i < s->category_count; i++) {
		if (!strcmp(s->categories[i], slug)) {
			for (; i < s->category_count - 1; i++) {
				str_lcpy(s->categories[i],
				    s->categories[i + 1], NAME_MAX_LEN);
			}
			s->category_count--;
			return (1);
		}
	}

	return (0);
}

/*
 * UNCATEGORIZED_SLUG is never actually stored on a source (see
 * source_from_json() above); it matches sources with no real category
 * assigned at all.
 */
int
source_in_category(const struct source *s, const char *slug)
{
	if (!strcmp(slug, UNCATEGORIZED_SLUG))
		return (s->category_count == 0);
	return (source_has_category(s, slug));
}

/* name or description, case-insensitive substring. */
int
source_matches_query(const struct source *s, const char *q)
{
	const char	*name = repo_display_name(s->original_url);

	return (str_casestr(name, q) != NULL ||
	    str_casestr(s->description, q) != NULL);
}

int
source_matches_filter(const struct source *s, const char *filter,
    const char *language, const char *q)
{
	if (filter[0] != '\0' && !source_in_category(s, filter))
		return (0);
	if (language[0] != '\0' && strcmp(s->language, language) != 0)
		return (0);
	if (q[0] != '\0' && !source_matches_query(s, q))
		return (0);
	return (1);
}
