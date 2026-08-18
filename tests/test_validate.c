/* Tests for src/lib/validate.c */

#include <string.h>
#include "unity.h"
#include "validate.h"
#include "app.h"

void setUp(void) {}
void tearDown(void) {}

/* v_url */

static void test_url_http(void)
{
	TEST_ASSERT_TRUE(v_url("http://example.com"));
}

static void test_url_https(void)
{
	TEST_ASSERT_TRUE(v_url("https://github.com/user/repo"));
}

static void test_url_git(void)
{
	TEST_ASSERT_TRUE(v_url("git://github.com/user/repo.git"));
}

static void test_url_no_scheme(void)
{
	TEST_ASSERT_FALSE(v_url("example.com/path"));
}

static void test_url_null(void)
{
	TEST_ASSERT_FALSE(v_url(NULL));
}

static void test_url_empty(void)
{
	TEST_ASSERT_FALSE(v_url(""));
}

static void test_url_too_long(void)
{
	/* URL_MAX_LEN exactly: invalid (must be < URL_MAX_LEN) */
	char buf[URL_MAX_LEN + 1];
	memset(buf, 'a', URL_MAX_LEN);
	buf[URL_MAX_LEN] = '\0';
	TEST_ASSERT_FALSE(v_url(buf));
}

/* v_slug */

static void test_slug_alphanumeric(void)
{
	TEST_ASSERT_TRUE(v_slug("abc123"));
}

static void test_slug_with_hyphen_underscore_dot(void)
{
	TEST_ASSERT_TRUE(v_slug("my-project_v1.0"));
}

static void test_slug_with_space(void)
{
	TEST_ASSERT_FALSE(v_slug("hello world"));
}

static void test_slug_with_slash(void)
{
	TEST_ASSERT_FALSE(v_slug("foo/bar"));
}

static void test_slug_null(void)
{
	TEST_ASSERT_FALSE(v_slug(NULL));
}

static void test_slug_empty(void)
{
	TEST_ASSERT_FALSE(v_slug(""));
}

static void test_slug_at_max_len(void)
{
	char buf[NAME_MAX_LEN + 1];
	memset(buf, 'a', NAME_MAX_LEN);
	buf[NAME_MAX_LEN] = '\0';
	TEST_ASSERT_FALSE(v_slug(buf)); /* len >= NAME_MAX_LEN */
}

static void test_slug_one_below_max_len(void)
{
	char buf[NAME_MAX_LEN];
	memset(buf, 'a', NAME_MAX_LEN - 1);
	buf[NAME_MAX_LEN - 1] = '\0';
	TEST_ASSERT_TRUE(v_slug(buf));
}

/* v_text */

static void test_text_normal(void)
{
	TEST_ASSERT_TRUE(v_text("Hello, world!", TEXT_MAX_LEN));
}

static void test_text_one_below_max(void)
{
	char buf[TEXT_MAX_LEN];
	memset(buf, 'a', TEXT_MAX_LEN - 1);
	buf[TEXT_MAX_LEN - 1] = '\0';
	TEST_ASSERT_TRUE(v_text(buf, TEXT_MAX_LEN)); /* strlen < maxlen */
}

static void test_text_at_max_len(void)
{
	char buf[TEXT_MAX_LEN + 1];
	memset(buf, 'a', TEXT_MAX_LEN);
	buf[TEXT_MAX_LEN] = '\0';
	TEST_ASSERT_FALSE(v_text(buf, TEXT_MAX_LEN)); /* strlen == maxlen: invalid */
}

static void test_text_null(void)
{
	TEST_ASSERT_FALSE(v_text(NULL, TEXT_MAX_LEN));
}

static void test_text_empty_is_valid(void)
{
	TEST_ASSERT_TRUE(v_text("", TEXT_MAX_LEN));
}

/* v_category_name */

static void test_category_name_valid(void)
{
	TEST_ASSERT_TRUE(v_category_name("C Libraries"));
}

static void test_category_name_empty(void)
{
	TEST_ASSERT_FALSE(v_category_name(""));
}

/* v_admin_name */

static void test_admin_name_valid(void)
{
	TEST_ASSERT_TRUE(v_admin_name("Antoni Aloy"));
}

static void test_admin_name_empty(void)
{
	TEST_ASSERT_FALSE(v_admin_name(""));
}

static void test_admin_name_null(void)
{
	TEST_ASSERT_FALSE(v_admin_name(NULL));
}

static void test_admin_name_at_max_len(void)
{
	char buf[NAME_MAX_LEN + 1];
	memset(buf, 'a', NAME_MAX_LEN);
	buf[NAME_MAX_LEN] = '\0';
	TEST_ASSERT_FALSE(v_admin_name(buf)); /* len >= NAME_MAX_LEN */
}

static void test_admin_name_one_below_max_len(void)
{
	char buf[NAME_MAX_LEN];
	memset(buf, 'a', NAME_MAX_LEN - 1);
	buf[NAME_MAX_LEN - 1] = '\0';
	TEST_ASSERT_TRUE(v_admin_name(buf));
}

/* v_color */

static void test_color_valid_lowercase(void)
{
	TEST_ASSERT_TRUE(v_color("#aabbcc"));
}

static void test_color_valid_uppercase(void)
{
	TEST_ASSERT_TRUE(v_color("#AABB00"));
}

static void test_color_no_hash(void)
{
	TEST_ASSERT_FALSE(v_color("aabbcc"));
}

static void test_color_six_chars_no_hash(void)
{
	TEST_ASSERT_FALSE(v_color("aabbcc")); /* len=6, no # */
}

static void test_color_non_hex_digit(void)
{
	TEST_ASSERT_FALSE(v_color("#gghhii"));
}

static void test_color_null(void)
{
	TEST_ASSERT_FALSE(v_color(NULL));
}

static void test_color_eight_chars(void)
{
	TEST_ASSERT_FALSE(v_color("#aabbccdd")); /* length 9, wrong */
}

int main(void)
{
	UNITY_BEGIN();

	RUN_TEST(test_url_http);
	RUN_TEST(test_url_https);
	RUN_TEST(test_url_git);
	RUN_TEST(test_url_no_scheme);
	RUN_TEST(test_url_null);
	RUN_TEST(test_url_empty);
	RUN_TEST(test_url_too_long);

	RUN_TEST(test_slug_alphanumeric);
	RUN_TEST(test_slug_with_hyphen_underscore_dot);
	RUN_TEST(test_slug_with_space);
	RUN_TEST(test_slug_with_slash);
	RUN_TEST(test_slug_null);
	RUN_TEST(test_slug_empty);
	RUN_TEST(test_slug_at_max_len);
	RUN_TEST(test_slug_one_below_max_len);

	RUN_TEST(test_text_normal);
	RUN_TEST(test_text_one_below_max);
	RUN_TEST(test_text_at_max_len);
	RUN_TEST(test_text_null);
	RUN_TEST(test_text_empty_is_valid);

	RUN_TEST(test_category_name_valid);
	RUN_TEST(test_category_name_empty);

	RUN_TEST(test_admin_name_valid);
	RUN_TEST(test_admin_name_empty);
	RUN_TEST(test_admin_name_null);
	RUN_TEST(test_admin_name_at_max_len);
	RUN_TEST(test_admin_name_one_below_max_len);

	RUN_TEST(test_color_valid_lowercase);
	RUN_TEST(test_color_valid_uppercase);
	RUN_TEST(test_color_no_hash);
	RUN_TEST(test_color_six_chars_no_hash);
	RUN_TEST(test_color_non_hex_digit);
	RUN_TEST(test_color_null);
	RUN_TEST(test_color_eight_chars);

	return UNITY_END();
}
