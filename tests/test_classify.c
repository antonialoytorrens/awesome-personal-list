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
	int n = classify_source("", "C", cats, SOURCE_MAX_CATEGORIES);
	TEST_ASSERT_EQUAL_INT(1, n);
	TEST_ASSERT_EQUAL_STRING("lang-c", cats[0].slug);
	TEST_ASSERT_EQUAL_STRING("C", cats[0].name);
}

static void test_python_language(void)
{
	int n = classify_source("", "Python", cats, SOURCE_MAX_CATEGORIES);
	TEST_ASSERT_GREATER_OR_EQUAL(1, n);
	TEST_ASSERT_TRUE(find_slug(cats, n, "lang-python"));
}

static void test_cpp_lowercase(void)
{
	int n = classify_source("", "c++", cats, SOURCE_MAX_CATEGORIES);
	TEST_ASSERT_GREATER_OR_EQUAL(1, n);
	TEST_ASSERT_TRUE(find_slug(cats, n, "lang-cpp"));
}

static void test_sdl_in_description(void)
{
	int n = classify_source("Uses SDL for rendering", "", cats,
	    SOURCE_MAX_CATEGORIES);
	TEST_ASSERT_GREATER_OR_EQUAL(1, n);
	TEST_ASSERT_TRUE(find_slug(cats, n, "sdl"));
}

static void test_c_language_and_gtk_description(void)
{
	int n = classify_source("GTK application", "C", cats,
	    SOURCE_MAX_CATEGORIES);
	TEST_ASSERT_GREATER_OR_EQUAL(2, n);
	TEST_ASSERT_TRUE(find_slug(cats, n, "lang-c"));
	TEST_ASSERT_TRUE(find_slug(cats, n, "gtk"));
}

static void test_homebrew_keyword(void)
{
	int n = classify_source("A homebrew game for consoles", "", cats,
	    SOURCE_MAX_CATEGORIES);
	TEST_ASSERT_GREATER_OR_EQUAL(1, n);
	TEST_ASSERT_TRUE(find_slug(cats, n, "homebrew-console"));
}

static void test_nintendo_ds_no_duplicate_console_category(void)
{
	int n = classify_source("Nintendo DS homebrew game", "", cats,
	    SOURCE_MAX_CATEGORIES);
	int count = 0, i;
	for (i = 0; i < n; i++)
		if (!strcmp(cats[i].slug, "homebrew-console"))
			count++;
	TEST_ASSERT_EQUAL_INT(1, count);
}

static void test_unknown_language_no_keywords(void)
{
	int n = classify_source("A simple utility", "CobolExtreme", cats,
	    SOURCE_MAX_CATEGORIES);
	TEST_ASSERT_EQUAL_INT(0, n);
}

static void test_result_capped_at_max(void)
{
	/* SDL + Qt + GTK + OpenGL + Vulkan + Box2D + Godot + Unity (8 hits) */
	const char *desc = "SDL Qt GTK OpenGL Vulkan Box2D Godot Unity";
	int n = classify_source(desc, "", cats, 3);
	TEST_ASSERT_EQUAL_INT(3, n);
}

static void test_react_in_description(void)
{
	int n = classify_source("A React frontend", "", cats,
	    SOURCE_MAX_CATEGORIES);
	TEST_ASSERT_TRUE(find_slug(cats, n, "react"));
}

static void test_ffmpeg_keyword(void)
{
	int n = classify_source("Video processing with FFmpeg", "", cats,
	    SOURCE_MAX_CATEGORIES);
	TEST_ASSERT_TRUE(find_slug(cats, n, "ffmpeg"));
}

int main(void)
{
	UNITY_BEGIN();

	RUN_TEST(test_c_language);
	RUN_TEST(test_python_language);
	RUN_TEST(test_cpp_lowercase);
	RUN_TEST(test_sdl_in_description);
	RUN_TEST(test_c_language_and_gtk_description);
	RUN_TEST(test_homebrew_keyword);
	RUN_TEST(test_nintendo_ds_no_duplicate_console_category);
	RUN_TEST(test_unknown_language_no_keywords);
	RUN_TEST(test_result_capped_at_max);
	RUN_TEST(test_react_in_description);
	RUN_TEST(test_ffmpeg_keyword);

	return UNITY_END();
}
