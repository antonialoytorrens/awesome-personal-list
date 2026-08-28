/* Tests for src/lib/classify.c */

#include <string.h>
#include "unity.h"
#include "classify.h"
#include "app.h"

void setUp(void) {}
void tearDown(void) {}

static struct classified_category cats[SOURCE_MAX_CATEGORIES];

static int find_slug(const struct classified_category *c, int n,
    const char *slug)
{
	int i;
	for (i = 0; i < n; i++)
		if (!strcmp(c[i].slug, slug))
			return 1;
	return 0;
}

static void test_c_language(void)
{
	int n = classify_source("C", cats, SOURCE_MAX_CATEGORIES);
	TEST_ASSERT_EQUAL_INT(1, n);
	TEST_ASSERT_EQUAL_STRING("lang-c", cats[0].slug);
	TEST_ASSERT_EQUAL_STRING("C", cats[0].name);
}

static void test_python_language(void)
{
	int n = classify_source("Python", cats, SOURCE_MAX_CATEGORIES);
	TEST_ASSERT_GREATER_OR_EQUAL(1, n);
	TEST_ASSERT_TRUE(find_slug(cats, n, "lang-python"));
}

static void test_cpp_lowercase(void)
{
	int n = classify_source("c++", cats, SOURCE_MAX_CATEGORIES);
	TEST_ASSERT_GREATER_OR_EQUAL(1, n);
	TEST_ASSERT_TRUE(find_slug(cats, n, "lang-cpp"));
}

static void test_unknown_language(void)
{
	int n = classify_source("CobolExtreme", cats, SOURCE_MAX_CATEGORIES);
	TEST_ASSERT_EQUAL_INT(0, n);
}

static void test_empty_language(void)
{
	int n = classify_source("", cats, SOURCE_MAX_CATEGORIES);
	TEST_ASSERT_EQUAL_INT(0, n);
}

static void test_canonical_language_alias(void)
{
	TEST_ASSERT_EQUAL_STRING("C++", classify_canonical_language("cpp"));
	TEST_ASSERT_EQUAL_STRING("Go", classify_canonical_language("golang"));
	TEST_ASSERT_EQUAL_STRING("C", classify_canonical_language("c"));
}

static void test_canonical_language_unknown(void)
{
	TEST_ASSERT_NULL(classify_canonical_language("CobolExtreme"));
	TEST_ASSERT_NULL(classify_canonical_language(""));
	TEST_ASSERT_NULL(classify_canonical_language(NULL));
}

int main(void)
{
	UNITY_BEGIN();

	RUN_TEST(test_c_language);
	RUN_TEST(test_python_language);
	RUN_TEST(test_cpp_lowercase);
	RUN_TEST(test_unknown_language);
	RUN_TEST(test_empty_language);
	RUN_TEST(test_canonical_language_alias);
	RUN_TEST(test_canonical_language_unknown);

	return UNITY_END();
}
