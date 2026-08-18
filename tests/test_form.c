/* Tests for src/lib/form.c */

#include <string.h>
#include "unity.h"
#include "form.h"

void setUp(void) {}
void tearDown(void) {}

/* form_urldecode */

static void test_urldecode_plus_to_space(void)
{
	char s[] = "hello+world";
	form_urldecode(s);
	TEST_ASSERT_EQUAL_STRING("hello world", s);
}

static void test_urldecode_percent20_to_space(void)
{
	char s[] = "hello%20world";
	form_urldecode(s);
	TEST_ASSERT_EQUAL_STRING("hello world", s);
}

static void test_urldecode_percent2F_to_slash(void)
{
	char s[] = "path%2Fto";
	form_urldecode(s);
	TEST_ASSERT_EQUAL_STRING("path/to", s);
}

static void test_urldecode_uppercase_percent(void)
{
	char s[] = "path%2Fto";
	form_urldecode(s);
	TEST_ASSERT_EQUAL_STRING("path/to", s);
}

static void test_urldecode_invalid_percent_left_as_is(void)
{
	char s[] = "bad%GG";
	form_urldecode(s);
	TEST_ASSERT_EQUAL_STRING("bad%GG", s);
}

static void test_urldecode_returns_decoded_length(void)
{
	char s[] = "a+b";
	size_t len = form_urldecode(s);
	TEST_ASSERT_EQUAL_size_t(3, len); /* "a b" is 3 chars */
}

static void test_urldecode_no_encoding(void)
{
	char s[] = "plain";
	form_urldecode(s);
	TEST_ASSERT_EQUAL_STRING("plain", s);
}

/* form_parse + form_get */

static void test_parse_single_field(void)
{
	struct form f;
	const char body[] = "key=value";
	TEST_ASSERT_EQUAL_INT(0, form_parse(&f, body, sizeof(body) - 1));
	TEST_ASSERT_EQUAL_UINT(1, f.count);
	TEST_ASSERT_EQUAL_STRING("value", form_get(&f, "key"));
	form_free(&f);
}

static void test_parse_multiple_fields(void)
{
	struct form f;
	const char body[] = "a=1&b=2&c=3";
	TEST_ASSERT_EQUAL_INT(0, form_parse(&f, body, sizeof(body) - 1));
	TEST_ASSERT_EQUAL_UINT(3, f.count);
	TEST_ASSERT_EQUAL_STRING("1", form_get(&f, "a"));
	TEST_ASSERT_EQUAL_STRING("2", form_get(&f, "b"));
	TEST_ASSERT_EQUAL_STRING("3", form_get(&f, "c"));
	form_free(&f);
}

static void test_parse_urlencoded_value(void)
{
	struct form f;
	const char body[] = "url=https%3A%2F%2Fexample.com";
	TEST_ASSERT_EQUAL_INT(0, form_parse(&f, body, sizeof(body) - 1));
	TEST_ASSERT_EQUAL_STRING("https://example.com", form_get(&f, "url"));
	form_free(&f);
}

static void test_parse_key_without_equals(void)
{
	struct form f;
	const char body[] = "justkey";
	TEST_ASSERT_EQUAL_INT(0, form_parse(&f, body, sizeof(body) - 1));
	TEST_ASSERT_EQUAL_UINT(1, f.count);
	TEST_ASSERT_EQUAL_STRING("", form_get(&f, "justkey"));
	form_free(&f);
}

static void test_parse_nul_byte_in_body_returns_error(void)
{
	struct form f;
	/* \0 (octal) inserts a true NUL; \x00e would be a single 0x0e byte */
	const char body[] = "key=val\0extra";
	/* strlen(scratch) == 7, len == 13: mismatch → return -1 */
	int rc = form_parse(&f, body, sizeof(body) - 1);
	TEST_ASSERT_EQUAL_INT(-1, rc);
}

static void test_parse_empty_body(void)
{
	struct form f;
	TEST_ASSERT_EQUAL_INT(0, form_parse(&f, "", 0));
	TEST_ASSERT_EQUAL_UINT(0, f.count);
	form_free(&f);
}

/* form_get */

static void test_get_missing_key(void)
{
	struct form f;
	const char body[] = "a=1";
	form_parse(&f, body, sizeof(body) - 1);
	TEST_ASSERT_NULL(form_get(&f, "z"));
	form_free(&f);
}

static void test_get_first_of_duplicate_keys(void)
{
	struct form f;
	const char body[] = "k=first&k=second";
	form_parse(&f, body, sizeof(body) - 1);
	TEST_ASSERT_EQUAL_STRING("first", form_get(&f, "k"));
	form_free(&f);
}

/* form_free */

static void test_free_sets_null_and_zero(void)
{
	struct form f;
	const char body[] = "x=1";
	form_parse(&f, body, sizeof(body) - 1);
	form_free(&f);
	TEST_ASSERT_NULL(f.scratch);
	TEST_ASSERT_EQUAL_UINT(0, f.count);
}

int main(void)
{
	UNITY_BEGIN();

	RUN_TEST(test_urldecode_plus_to_space);
	RUN_TEST(test_urldecode_percent20_to_space);
	RUN_TEST(test_urldecode_percent2F_to_slash);
	RUN_TEST(test_urldecode_uppercase_percent);
	RUN_TEST(test_urldecode_invalid_percent_left_as_is);
	RUN_TEST(test_urldecode_returns_decoded_length);
	RUN_TEST(test_urldecode_no_encoding);

	RUN_TEST(test_parse_single_field);
	RUN_TEST(test_parse_multiple_fields);
	RUN_TEST(test_parse_urlencoded_value);
	RUN_TEST(test_parse_key_without_equals);
	RUN_TEST(test_parse_nul_byte_in_body_returns_error);
	RUN_TEST(test_parse_empty_body);

	RUN_TEST(test_get_missing_key);
	RUN_TEST(test_get_first_of_duplicate_keys);

	RUN_TEST(test_free_sets_null_and_zero);

	return UNITY_END();
}
