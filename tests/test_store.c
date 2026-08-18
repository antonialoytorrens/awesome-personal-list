/* Tests for src/lib/store.c (pure functions only -- no filesystem I/O). */

#include <string.h>
#include "unity.h"
#include "models.h"

void setUp(void) {}
void tearDown(void) {}

/* store_valid_slug */

static void test_valid_slug_ok(void)
{
	TEST_ASSERT_TRUE(store_valid_slug("github-com-foo-bar"));
}

static void test_valid_slug_rejects_leading_dot(void)
{
	TEST_ASSERT_FALSE(store_valid_slug(".hidden"));
}

static void test_valid_slug_rejects_traversal(void)
{
	TEST_ASSERT_FALSE(store_valid_slug("a..b"));
}

static void test_valid_slug_rejects_null(void)
{
	TEST_ASSERT_FALSE(store_valid_slug(NULL));
}

/* store_slugify_url */

static void test_slugify_strips_scheme_and_lowercases(void)
{
	char out[128];
	TEST_ASSERT_EQUAL_INT(0,
	    store_slugify_url("https://GitHub.com/Foo/Bar", out, sizeof(out)));
	TEST_ASSERT_EQUAL_STRING("github-com-foo-bar", out);
}

static void test_slugify_http_scheme(void)
{
	char out[128];
	TEST_ASSERT_EQUAL_INT(0,
	    store_slugify_url("http://example.com/x", out, sizeof(out)));
	TEST_ASSERT_EQUAL_STRING("example-com-x", out);
}

static void test_slugify_collapses_runs_of_punctuation(void)
{
	char out[128];
	TEST_ASSERT_EQUAL_INT(0,
	    store_slugify_url("https://example.com//a---b", out, sizeof(out)));
	TEST_ASSERT_EQUAL_STRING("example-com-a-b", out);
}

static void test_slugify_empty_input_fails(void)
{
	char out[128];
	TEST_ASSERT_EQUAL_INT(-1, store_slugify_url("https://", out, sizeof(out)));
}

static void test_slugify_null_input_fails(void)
{
	char out[128];
	TEST_ASSERT_EQUAL_INT(-1, store_slugify_url(NULL, out, sizeof(out)));
}

/* Unicode transliteration -- the bug this file was added to cover:
 * "educacio" was coming out as "educaci" because the accented "o" was
 * dropped instead of transliterated. */

static void test_slugify_transliterates_catalan_accent(void)
{
	char out[128];
	TEST_ASSERT_EQUAL_INT(0,
	    store_slugify_url("educaci\xc3\xb3", out, sizeof(out)));
	TEST_ASSERT_EQUAL_STRING("educacio", out);
}

static void test_slugify_transliterates_multiple_diacritics(void)
{
	char out[128];
	/* "Configuraci\xc3\xb3n R\xc3\xa1pida" = "Configuración Rápida" */
	TEST_ASSERT_EQUAL_INT(0,
	    store_slugify_url("Configuraci\xc3\xb3n R\xc3\xa1pida", out,
	    sizeof(out)));
	TEST_ASSERT_EQUAL_STRING("configuracion-rapida", out);
}

static void test_slugify_transliterates_cedilla_and_sharp_s(void)
{
	char out[128];
	/* "Fran\xc3\xa7ais / Stra\xc3\x9fe" = "Français / Straße" -- each
	 * escape is its own string literal (concatenated at compile time)
	 * so a following hex-digit-looking letter ('a', 'i'...) can't be
	 * swallowed into the \x escape (\x consumes as many hex digits as
	 * follow it). */
	TEST_ASSERT_EQUAL_INT(0,
	    store_slugify_url("Fran" "\xc3\xa7" "ais / Stra" "\xc3\x9f" "e",
	    out, sizeof(out)));
	TEST_ASSERT_EQUAL_STRING("francais-strasse", out);
}

static void test_slugify_unmapped_multibyte_becomes_separator(void)
{
	char out[128];
	/* Euro sign (U+20AC) has no ASCII mapping -- it must not corrupt
	 * the slug, just fall back to being a separator like before. */
	TEST_ASSERT_EQUAL_INT(0,
	    store_slugify_url("Price \xe2\x82\xac 5", out, sizeof(out)));
	TEST_ASSERT_EQUAL_STRING("price-5", out);
}

int main(void)
{
	UNITY_BEGIN();

	RUN_TEST(test_valid_slug_ok);
	RUN_TEST(test_valid_slug_rejects_leading_dot);
	RUN_TEST(test_valid_slug_rejects_traversal);
	RUN_TEST(test_valid_slug_rejects_null);

	RUN_TEST(test_slugify_strips_scheme_and_lowercases);
	RUN_TEST(test_slugify_http_scheme);
	RUN_TEST(test_slugify_collapses_runs_of_punctuation);
	RUN_TEST(test_slugify_empty_input_fails);
	RUN_TEST(test_slugify_null_input_fails);

	RUN_TEST(test_slugify_transliterates_catalan_accent);
	RUN_TEST(test_slugify_transliterates_multiple_diacritics);
	RUN_TEST(test_slugify_transliterates_cedilla_and_sharp_s);
	RUN_TEST(test_slugify_unmapped_multibyte_becomes_separator);

	return UNITY_END();
}
