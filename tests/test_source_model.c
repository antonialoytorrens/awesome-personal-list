/* Tests for src/models/source.c (requires tmpdir fixture) */

#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include <string.h>
#include <time.h>
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
	strcpy(g_tmpdir, "/tmp/ms-src-XXXXXX");
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

static void make_source(struct source *s, const char *url,
    const char *description, const char *language)
{
	memset(s, 0, sizeof(*s));
	store_slugify_url(url, s->slug, sizeof(s->slug));
	str_lcpy(s->original_url, url, sizeof(s->original_url));
	str_lcpy(s->description, description, sizeof(s->description));
	str_lcpy(s->language, language, sizeof(s->language));
	s->created_at = 1000000;
	s->updated_at = 1000000;
}

/* write + read roundtrip */

static void test_source_write_read_roundtrip(void)
{
	struct source in, out;
	make_source(&in, "https://github.com/user/repo", "A test repo", "C");
	in.lang_checked_at = 1700000000;
	in.lang_check_count = 3;
	in.language_locked = 1;

	TEST_ASSERT_EQUAL_INT(0, source_write(&in));

	memset(&out, 0, sizeof(out));
	TEST_ASSERT_EQUAL_INT(0, source_read(in.slug, &out));

	TEST_ASSERT_EQUAL_STRING(in.slug, out.slug);
	TEST_ASSERT_EQUAL_STRING(in.original_url, out.original_url);
	TEST_ASSERT_EQUAL_STRING(in.description, out.description);
	TEST_ASSERT_EQUAL_STRING(in.language, out.language);
	TEST_ASSERT_EQUAL_INT64((long long)in.lang_checked_at,
	    (long long)out.lang_checked_at);
	TEST_ASSERT_EQUAL_UINT(in.lang_check_count, out.lang_check_count);
	TEST_ASSERT_EQUAL_INT(in.language_locked, out.language_locked);
}

/* source_list */

static void test_source_list_empty(void)
{
	struct source *srcs;
	size_t count;
	TEST_ASSERT_EQUAL_INT(0, source_list(&srcs, &count));
	TEST_ASSERT_EQUAL_size_t(0, count);
	source_list_free(srcs);
}

static void test_source_list_two_sources(void)
{
	struct source a, b, *srcs;
	size_t count;
	make_source(&a, "https://github.com/user/alpha", "Alpha", "C");
	make_source(&b, "https://github.com/user/beta", "Beta", "Python");
	source_write(&a);
	source_write(&b);

	TEST_ASSERT_EQUAL_INT(0, source_list(&srcs, &count));
	TEST_ASSERT_EQUAL_size_t(2, count);
	source_list_free(srcs);
}

/* source_delete */

static void test_source_delete_removes_file(void)
{
	struct source s, out;
	make_source(&s, "https://github.com/user/todelete", "Delete me", "");
	source_write(&s);
	TEST_ASSERT_EQUAL_INT(0, source_delete(s.slug));
	TEST_ASSERT_EQUAL_INT(-1, source_read(s.slug, &out));
}

/* source_exists_for_url */

static void test_source_exists_for_url_found(void)
{
	struct source s;
	make_source(&s, "https://github.com/user/exists", "Exists", "Go");
	source_write(&s);
	TEST_ASSERT_EQUAL_INT(0,
	    source_exists_for_url("https://github.com/user/exists", NULL));
}

static void test_source_exists_for_url_not_found(void)
{
	TEST_ASSERT_EQUAL_INT(-1,
	    source_exists_for_url("https://github.com/user/nope", NULL));
}

/* in-memory category operations */

static void test_source_has_category_present(void)
{
	struct source s;
	memset(&s, 0, sizeof(s));
	str_lcpy(s.categories[0], "lang-c", NAME_MAX_LEN);
	s.category_count = 1;
	TEST_ASSERT_TRUE(source_has_category(&s, "lang-c"));
}

static void test_source_has_category_absent(void)
{
	struct source s;
	memset(&s, 0, sizeof(s));
	TEST_ASSERT_FALSE(source_has_category(&s, "lang-c"));
}

static void test_source_add_category_normal(void)
{
	struct source s;
	memset(&s, 0, sizeof(s));
	source_add_category(&s, "lang-c");
	TEST_ASSERT_EQUAL_INT(1, s.category_count);
	TEST_ASSERT_EQUAL_STRING("lang-c", s.categories[0]);
}

static void test_source_add_category_uncategorized_noop(void)
{
	struct source s;
	memset(&s, 0, sizeof(s));
	source_add_category(&s, UNCATEGORIZED_SLUG);
	TEST_ASSERT_EQUAL_INT(0, s.category_count);
}

static void test_source_add_category_duplicate_noop(void)
{
	struct source s;
	memset(&s, 0, sizeof(s));
	source_add_category(&s, "lang-c");
	source_add_category(&s, "lang-c");
	TEST_ASSERT_EQUAL_INT(1, s.category_count);
}

static void test_source_add_category_at_max_noop(void)
{
	struct source s;
	int i;
	memset(&s, 0, sizeof(s));
	for (i = 0; i < SOURCE_MAX_CATEGORIES; i++) {
		char slug[NAME_MAX_LEN];
		snprintf(slug, sizeof(slug), "cat-%d", i);
		source_add_category(&s, slug);
	}
	TEST_ASSERT_EQUAL_INT(SOURCE_MAX_CATEGORIES, s.category_count);
	source_add_category(&s, "overflow");
	TEST_ASSERT_EQUAL_INT(SOURCE_MAX_CATEGORIES, s.category_count);
}

static void test_source_remove_category_returns_1(void)
{
	struct source s;
	memset(&s, 0, sizeof(s));
	source_add_category(&s, "lang-c");
	source_add_category(&s, "gtk");
	TEST_ASSERT_EQUAL_INT(1, source_remove_category(&s, "lang-c"));
	TEST_ASSERT_EQUAL_INT(1, s.category_count);
	TEST_ASSERT_EQUAL_STRING("gtk", s.categories[0]);
}

static void test_source_remove_category_missing_returns_0(void)
{
	struct source s;
	memset(&s, 0, sizeof(s));
	TEST_ASSERT_EQUAL_INT(0, source_remove_category(&s, "nope"));
}

static void test_source_categories_persisted(void)
{
	struct source in, out;
	make_source(&in, "https://github.com/user/cats", "Cat test", "C");
	source_add_category(&in, "lang-c");
	source_add_category(&in, "sdl");
	source_write(&in);

	memset(&out, 0, sizeof(out));
	source_read(in.slug, &out);
	TEST_ASSERT_EQUAL_INT(2, out.category_count);
	TEST_ASSERT_TRUE(source_has_category(&out, "lang-c"));
	TEST_ASSERT_TRUE(source_has_category(&out, "sdl"));
}

static void test_source_display_name_falls_back_when_unset(void)
{
	struct source	s;

	make_source(&s, "https://github.com/user/repo", "", "");
	TEST_ASSERT_EQUAL_STRING("repo", source_display_name(&s));
}

static void test_source_display_name_uses_override(void)
{
	struct source	s;

	make_source(&s, "https://github.com/user/repo", "", "");
	str_lcpy(s.name, "My Cool Project", sizeof(s.name));
	TEST_ASSERT_EQUAL_STRING("My Cool Project", source_display_name(&s));
}

static void test_source_name_persisted(void)
{
	struct source	in, out;

	make_source(&in, "https://github.com/user/named", "", "");
	str_lcpy(in.name, "Custom Name", sizeof(in.name));
	source_write(&in);

	memset(&out, 0, sizeof(out));
	source_read(in.slug, &out);
	TEST_ASSERT_EQUAL_STRING("Custom Name", out.name);
}

int main(void)
{
	UNITY_BEGIN();

	RUN_TEST(test_source_write_read_roundtrip);
	RUN_TEST(test_source_display_name_falls_back_when_unset);
	RUN_TEST(test_source_display_name_uses_override);
	RUN_TEST(test_source_name_persisted);

	RUN_TEST(test_source_list_empty);
	RUN_TEST(test_source_list_two_sources);

	RUN_TEST(test_source_delete_removes_file);

	RUN_TEST(test_source_exists_for_url_found);
	RUN_TEST(test_source_exists_for_url_not_found);

	RUN_TEST(test_source_has_category_present);
	RUN_TEST(test_source_has_category_absent);
	RUN_TEST(test_source_add_category_normal);
	RUN_TEST(test_source_add_category_uncategorized_noop);
	RUN_TEST(test_source_add_category_duplicate_noop);
	RUN_TEST(test_source_add_category_at_max_noop);
	RUN_TEST(test_source_remove_category_returns_1);
	RUN_TEST(test_source_remove_category_missing_returns_0);
	RUN_TEST(test_source_categories_persisted);

	return UNITY_END();
}
