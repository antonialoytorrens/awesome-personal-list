/* Tests for source_in_category / source_matches_query / source_matches_filter
 * in src/models/source.c */

#include <string.h>
#include "unity.h"
#include "app.h"
#include "models.h"
#include "strutil.h"

void setUp(void) {}
void tearDown(void) {}

static void make_source(struct source *s, const char *url,
    const char *description, const char *language)
{
	memset(s, 0, sizeof(*s));
	str_lcpy(s->original_url, url, sizeof(s->original_url));
	str_lcpy(s->description, description, sizeof(s->description));
	str_lcpy(s->language, language, sizeof(s->language));
}

/* source_in_category */

static void test_in_category_assigned(void)
{
	struct source s;
	make_source(&s, "https://github.com/u/r", "", "");
	source_add_category(&s, "lang-c");
	TEST_ASSERT_TRUE(source_in_category(&s, "lang-c"));
}

static void test_in_category_not_assigned(void)
{
	struct source s;
	make_source(&s, "https://github.com/u/r", "", "");
	source_add_category(&s, "lang-c");
	TEST_ASSERT_FALSE(source_in_category(&s, "gtk"));
}

static void test_in_category_uncategorized_matches_empty(void)
{
	struct source s;
	make_source(&s, "https://github.com/u/r", "", "");
	TEST_ASSERT_TRUE(source_in_category(&s, UNCATEGORIZED_SLUG));
}

static void test_in_category_uncategorized_excludes_categorized(void)
{
	struct source s;
	make_source(&s, "https://github.com/u/r", "", "");
	source_add_category(&s, "lang-c");
	TEST_ASSERT_FALSE(source_in_category(&s, UNCATEGORIZED_SLUG));
}

/* source_matches_query */

static void test_matches_query_by_name(void)
{
	struct source s;
	make_source(&s, "https://github.com/user/myrepo", "", "");
	TEST_ASSERT_TRUE(source_matches_query(&s, "myrepo"));
}

static void test_matches_query_by_name_case_insensitive(void)
{
	struct source s;
	make_source(&s, "https://github.com/user/MyRepo", "", "");
	TEST_ASSERT_TRUE(source_matches_query(&s, "myrepo"));
}

static void test_matches_query_by_description(void)
{
	struct source s;
	make_source(&s, "https://github.com/user/repo", "A tiny web server", "");
	TEST_ASSERT_TRUE(source_matches_query(&s, "web server"));
}

static void test_matches_query_no_match(void)
{
	struct source s;
	make_source(&s, "https://github.com/user/repo", "A tiny web server", "");
	TEST_ASSERT_FALSE(source_matches_query(&s, "nonexistent"));
}

static void test_matches_query_empty_description_no_crash(void)
{
	struct source s;
	make_source(&s, "https://github.com/user/repo", "", "");
	TEST_ASSERT_FALSE(source_matches_query(&s, "anything"));
}

/* source_matches_filter */

static void test_matches_filter_no_filters_matches_everything(void)
{
	struct source s;
	make_source(&s, "https://github.com/user/repo", "desc", "C");
	TEST_ASSERT_TRUE(source_matches_filter(&s, "", "", ""));
}

static void test_matches_filter_category_only_match(void)
{
	struct source s;
	make_source(&s, "https://github.com/user/repo", "", "");
	source_add_category(&s, "lang-c");
	TEST_ASSERT_TRUE(source_matches_filter(&s, "lang-c", "", ""));
}

static void test_matches_filter_category_only_mismatch(void)
{
	struct source s;
	make_source(&s, "https://github.com/user/repo", "", "");
	source_add_category(&s, "lang-c");
	TEST_ASSERT_FALSE(source_matches_filter(&s, "gtk", "", ""));
}

static void test_matches_filter_language_only_match(void)
{
	struct source s;
	make_source(&s, "https://github.com/user/repo", "", "Python");
	TEST_ASSERT_TRUE(source_matches_filter(&s, "", "Python", ""));
}

static void test_matches_filter_language_only_mismatch(void)
{
	struct source s;
	make_source(&s, "https://github.com/user/repo", "", "Python");
	TEST_ASSERT_FALSE(source_matches_filter(&s, "", "C", ""));
}

static void test_matches_filter_language_case_sensitive(void)
{
	/* source_matches_filter uses strcmp for language, not
	 * case-insensitive -- documents existing, deliberate behavior. */
	struct source s;
	make_source(&s, "https://github.com/user/repo", "", "Python");
	TEST_ASSERT_FALSE(source_matches_filter(&s, "", "python", ""));
}

static void test_matches_filter_query_only_match(void)
{
	struct source s;
	make_source(&s, "https://github.com/user/myrepo", "", "");
	TEST_ASSERT_TRUE(source_matches_filter(&s, "", "", "myrepo"));
}

static void test_matches_filter_query_only_mismatch(void)
{
	struct source s;
	make_source(&s, "https://github.com/user/myrepo", "", "");
	TEST_ASSERT_FALSE(source_matches_filter(&s, "", "", "nonexistent"));
}

static void test_matches_filter_all_three_all_match(void)
{
	struct source s;
	make_source(&s, "https://github.com/user/myrepo", "A web server", "C");
	source_add_category(&s, "lang-c");
	TEST_ASSERT_TRUE(source_matches_filter(&s, "lang-c", "C", "myrepo"));
}

static void test_matches_filter_all_three_one_mismatches(void)
{
	/* category and query match, language doesn't -- AND semantics,
	 * every clause must pass. */
	struct source s;
	make_source(&s, "https://github.com/user/myrepo", "A web server", "C");
	source_add_category(&s, "lang-c");
	TEST_ASSERT_FALSE(source_matches_filter(&s, "lang-c", "Python", "myrepo"));
}

static void test_matches_filter_uncategorized_filter(void)
{
	struct source s;
	make_source(&s, "https://github.com/user/repo", "", "");
	TEST_ASSERT_TRUE(source_matches_filter(&s, UNCATEGORIZED_SLUG, "", ""));
}

int main(void)
{
	UNITY_BEGIN();

	RUN_TEST(test_in_category_assigned);
	RUN_TEST(test_in_category_not_assigned);
	RUN_TEST(test_in_category_uncategorized_matches_empty);
	RUN_TEST(test_in_category_uncategorized_excludes_categorized);

	RUN_TEST(test_matches_query_by_name);
	RUN_TEST(test_matches_query_by_name_case_insensitive);
	RUN_TEST(test_matches_query_by_description);
	RUN_TEST(test_matches_query_no_match);
	RUN_TEST(test_matches_query_empty_description_no_crash);

	RUN_TEST(test_matches_filter_no_filters_matches_everything);
	RUN_TEST(test_matches_filter_category_only_match);
	RUN_TEST(test_matches_filter_category_only_mismatch);
	RUN_TEST(test_matches_filter_language_only_match);
	RUN_TEST(test_matches_filter_language_only_mismatch);
	RUN_TEST(test_matches_filter_language_case_sensitive);
	RUN_TEST(test_matches_filter_query_only_match);
	RUN_TEST(test_matches_filter_query_only_mismatch);
	RUN_TEST(test_matches_filter_all_three_all_match);
	RUN_TEST(test_matches_filter_all_three_one_mismatches);
	RUN_TEST(test_matches_filter_uncategorized_filter);

	return UNITY_END();
}
