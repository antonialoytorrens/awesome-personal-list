/* Tests for src/models/category.c (requires tmpdir fixture) */

#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "unity.h"
#include "app.h"
#include "models.h"
#include "strutil.h"

static char g_orig_cwd[4096];
static char g_tmpdir[64];

void setUp(void)
{
	getcwd(g_orig_cwd, sizeof(g_orig_cwd));
	strcpy(g_tmpdir, "/tmp/ms-cat-XXXXXX");
	mkdtemp(g_tmpdir);
	chdir(g_tmpdir);
	store_verify_dirs();
}

void tearDown(void)
{
	char cmd[128];
	chdir(g_orig_cwd);
	snprintf(cmd, sizeof(cmd), "rm -rf %s", g_tmpdir);
	system(cmd);
}

static void make_category(struct category *c, const char *slug,
    const char *name, const char *color)
{
	memset(c, 0, sizeof(*c));
	str_lcpy(c->slug, slug, sizeof(c->slug));
	str_lcpy(c->name, name, sizeof(c->name));
	str_lcpy(c->color, color, sizeof(c->color));
}

static void make_source_with_cat(const char *url, const char *cat_slug)
{
	struct source s;
	memset(&s, 0, sizeof(s));
	store_slugify_url(url, s.slug, sizeof(s.slug));
	str_lcpy(s.original_url, url, sizeof(s.original_url));
	str_lcpy(s.description, "test", sizeof(s.description));
	source_add_category(&s, cat_slug);
	source_write(&s);
}

/* write + read roundtrip */

static void test_category_write_read_roundtrip(void)
{
	struct category in, out;
	make_category(&in, "lang-c", "C", "#555555");
	str_lcpy(in.description, "C language", sizeof(in.description));

	TEST_ASSERT_EQUAL_INT(0, category_write(&in));

	memset(&out, 0, sizeof(out));
	TEST_ASSERT_EQUAL_INT(0, category_read("lang-c", &out));
	TEST_ASSERT_EQUAL_STRING("lang-c", out.slug);
	TEST_ASSERT_EQUAL_STRING("C", out.name);
	TEST_ASSERT_EQUAL_STRING("#555555", out.color);
	TEST_ASSERT_EQUAL_STRING("C language", out.description);
}

/* name with spaces is allowed */

static void test_category_name_with_spaces(void)
{
	struct category in, out;
	make_category(&in, "c-libs", "C Libraries", "#aabbcc");

	TEST_ASSERT_EQUAL_INT(0, category_write(&in));
	TEST_ASSERT_EQUAL_INT(0, category_read("c-libs", &out));
	TEST_ASSERT_EQUAL_STRING("C Libraries", out.name);
}

/* invalid color falls back to default */

static void test_category_invalid_color_defaults(void)
{
	struct category in, out;
	make_category(&in, "no-color", "No Color", "xxxxxx");

	category_write(&in);
	category_read("no-color", &out);
	TEST_ASSERT_EQUAL_STRING(CATEGORY_DEFAULT_COLOR, out.color);
}

/* category_list */

static void test_category_list_empty(void)
{
	struct category *cats;
	size_t count;
	TEST_ASSERT_EQUAL_INT(0, category_list(&cats, &count));
	TEST_ASSERT_EQUAL_size_t(0, count);
	category_list_free(cats);
}

static void test_category_list_two_categories(void)
{
	struct category a, b, *cats;
	size_t count;
	make_category(&a, "alpha", "Alpha", "#111111");
	make_category(&b, "beta", "Beta", "#222222");
	category_write(&a);
	category_write(&b);

	TEST_ASSERT_EQUAL_INT(0, category_list(&cats, &count));
	TEST_ASSERT_EQUAL_size_t(2, count);
	category_list_free(cats);
}

/* category_seed_defaults */

static void test_category_seed_creates_uncategorized(void)
{
	struct category out;
	TEST_ASSERT_EQUAL_INT(0, category_seed_defaults());
	TEST_ASSERT_EQUAL_INT(0, category_read(UNCATEGORIZED_SLUG, &out));
	TEST_ASSERT_EQUAL_STRING(UNCATEGORIZED_SLUG, out.slug);
}

static void test_category_seed_is_idempotent(void)
{
	category_seed_defaults();
	TEST_ASSERT_EQUAL_INT(0, category_seed_defaults()); /* no-op */
}

/* category_delete — UNCATEGORIZED_SLUG cannot be deleted */

static void test_category_delete_uncategorized_fails(void)
{
	struct category out;
	category_seed_defaults();
	TEST_ASSERT_EQUAL_INT(-1, category_delete(UNCATEGORIZED_SLUG));
	/* File still exists */
	TEST_ASSERT_EQUAL_INT(0, category_read(UNCATEGORIZED_SLUG, &out));
}

/* category_delete — without sources */

static void test_category_delete_without_sources(void)
{
	struct category c, out;
	make_category(&c, "tools", "Tools", "#333333");
	category_write(&c);
	TEST_ASSERT_EQUAL_INT(0, category_delete("tools"));
	TEST_ASSERT_EQUAL_INT(-1, category_read("tools", &out));
}

/* category_delete — unassigns from sources that use it */

static void test_category_delete_unassigns_sources(void)
{
	struct category c;
	struct source s1, s2;
	make_category(&c, "games", "Games", "#ff0000");
	category_write(&c);

	make_source_with_cat("https://github.com/user/game1", "games");
	make_source_with_cat("https://github.com/user/game2", "games");

	TEST_ASSERT_EQUAL_INT(0, category_delete("games"));

	/* Read back both sources and verify "games" is gone */
	store_slugify_url("https://github.com/user/game1", s1.slug,
	    sizeof(s1.slug));
	store_slugify_url("https://github.com/user/game2", s2.slug,
	    sizeof(s2.slug));
	source_read(s1.slug, &s1);
	source_read(s2.slug, &s2);

	TEST_ASSERT_FALSE(source_has_category(&s1, "games"));
	TEST_ASSERT_FALSE(source_has_category(&s2, "games"));
}

/* category_delete preserves other categories on a source */

static void test_category_delete_preserves_other_categories(void)
{
	struct category c;
	struct source s;
	make_category(&c, "sdk", "SDK", "#00ff00");
	category_write(&c);

	/* Source has both "sdk" and "lang-c" */
	memset(&s, 0, sizeof(s));
	store_slugify_url("https://github.com/user/multi", s.slug,
	    sizeof(s.slug));
	str_lcpy(s.original_url, "https://github.com/user/multi",
	    sizeof(s.original_url));
	source_add_category(&s, "sdk");
	source_add_category(&s, "lang-c");
	source_write(&s);

	category_delete("sdk");

	memset(&s, 0, sizeof(s));
	source_read("github-com-user-multi", &s);
	TEST_ASSERT_FALSE(source_has_category(&s, "sdk"));
	TEST_ASSERT_TRUE(source_has_category(&s, "lang-c"));
}

int main(void)
{
	UNITY_BEGIN();

	RUN_TEST(test_category_write_read_roundtrip);
	RUN_TEST(test_category_name_with_spaces);
	RUN_TEST(test_category_invalid_color_defaults);

	RUN_TEST(test_category_list_empty);
	RUN_TEST(test_category_list_two_categories);

	RUN_TEST(test_category_seed_creates_uncategorized);
	RUN_TEST(test_category_seed_is_idempotent);

	RUN_TEST(test_category_delete_uncategorized_fails);
	RUN_TEST(test_category_delete_without_sources);
	RUN_TEST(test_category_delete_unassigns_sources);
	RUN_TEST(test_category_delete_preserves_other_categories);

	return UNITY_END();
}
