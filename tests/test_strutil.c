/* Tests for src/lib/strutil.c */

#include <string.h>
#include "unity.h"
#include "strutil.h"

void setUp(void) {}
void tearDown(void) {}

/* str_lcpy */

static void test_lcpy_normal(void)
{
	char buf[16];
	size_t ret = str_lcpy(buf, "hello", sizeof(buf));
	TEST_ASSERT_EQUAL_STRING("hello", buf);
	TEST_ASSERT_EQUAL_size_t(5, ret);
}

static void test_lcpy_truncation(void)
{
	char buf[4];
	size_t ret = str_lcpy(buf, "hello", sizeof(buf));
	TEST_ASSERT_EQUAL_STRING("hel", buf);
	TEST_ASSERT_EQUAL_size_t(5, ret); /* srclen, not written */
}

static void test_lcpy_dstsize_one(void)
{
	char buf[1];
	str_lcpy(buf, "hello", sizeof(buf));
	TEST_ASSERT_EQUAL_CHAR('\0', buf[0]);
}

static void test_lcpy_empty_src(void)
{
	char buf[8] = "garbage";
	size_t ret = str_lcpy(buf, "", sizeof(buf));
	TEST_ASSERT_EQUAL_STRING("", buf);
	TEST_ASSERT_EQUAL_size_t(0, ret);
}

/* str_split */

static void test_split_single_token(void)
{
	char input[] = "hello";
	char *out[4];
	int n = str_split(input, ",", out, 4);
	TEST_ASSERT_EQUAL_INT(1, n);
	TEST_ASSERT_EQUAL_STRING("hello", out[0]);
	TEST_ASSERT_NULL(out[1]);
}

static void test_split_three_tokens(void)
{
	char input[] = "a,b,c";
	char *out[4];
	int n = str_split(input, ",", out, 4);
	TEST_ASSERT_EQUAL_INT(3, n);
	TEST_ASSERT_EQUAL_STRING("a", out[0]);
	TEST_ASSERT_EQUAL_STRING("b", out[1]);
	TEST_ASSERT_EQUAL_STRING("c", out[2]);
}

static void test_split_consecutive_separators_skipped(void)
{
	char input[] = "a,,b";
	char *out[4];
	int n = str_split(input, ",", out, 4);
	TEST_ASSERT_EQUAL_INT(2, n);
	TEST_ASSERT_EQUAL_STRING("a", out[0]);
	TEST_ASSERT_EQUAL_STRING("b", out[1]);
}

static void test_split_max_exceeded(void)
{
	char input[] = "a,b,c,d";
	char *out[3];
	int n = str_split(input, ",", out, 3);
	TEST_ASSERT_EQUAL_INT(3, n);
	TEST_ASSERT_EQUAL_STRING("a", out[0]);
	TEST_ASSERT_EQUAL_STRING("b", out[1]);
	TEST_ASSERT_EQUAL_STRING("c", out[2]);
}

/* str_tonum */

static void test_tonum_valid_decimal(void)
{
	int err;
	long long v = str_tonum("42", 10, 0, 100, &err);
	TEST_ASSERT_EQUAL_INT(STR_OK, err);
	TEST_ASSERT_EQUAL_INT64(42, v);
}

static void test_tonum_valid_hex(void)
{
	int err;
	long long v = str_tonum("ff", 16, 0, 300, &err);
	TEST_ASSERT_EQUAL_INT(STR_OK, err);
	TEST_ASSERT_EQUAL_INT64(255, v);
}

static void test_tonum_out_of_range_low(void)
{
	int err;
	str_tonum("-1", 10, 0, 100, &err);
	TEST_ASSERT_NOT_EQUAL(STR_OK, err);
}

static void test_tonum_out_of_range_high(void)
{
	int err;
	str_tonum("101", 10, 0, 100, &err);
	TEST_ASSERT_NOT_EQUAL(STR_OK, err);
}

static void test_tonum_non_numeric(void)
{
	int err;
	str_tonum("abc", 10, 0, 100, &err);
	TEST_ASSERT_NOT_EQUAL(STR_OK, err);
}

static void test_tonum_empty_string(void)
{
	int err;
	str_tonum("", 10, 0, 100, &err);
	TEST_ASSERT_NOT_EQUAL(STR_OK, err);
}

static void test_tonum_null(void)
{
	int err;
	str_tonum(NULL, 10, 0, 100, &err);
	TEST_ASSERT_NOT_EQUAL(STR_OK, err);
}

/* str_hex */

static void test_hex_known_input(void)
{
	unsigned char in[] = { 0xde, 0xad, 0xbe, 0xef };
	char out[9];
	str_hex(in, sizeof(in), out, sizeof(out));
	TEST_ASSERT_EQUAL_STRING("deadbeef", out);
}

static void test_hex_all_zeros(void)
{
	unsigned char in[] = { 0x00, 0x00 };
	char out[5];
	str_hex(in, sizeof(in), out, sizeof(out));
	TEST_ASSERT_EQUAL_STRING("0000", out);
}

static void test_hex_buffer_too_small_no_write(void)
{
	unsigned char in[] = { 0xab };
	char out[2] = "xx";
	str_hex(in, sizeof(in), out, sizeof(out)); /* needs 3, has 2 -- no-op */
	TEST_ASSERT_EQUAL_CHAR('x', out[0]); /* unchanged */
}

/* str_timingsafe_equal */

static void test_timingsafe_equal_same(void)
{
	TEST_ASSERT_TRUE(str_timingsafe_equal("abc", "abc"));
}

static void test_timingsafe_equal_different_same_length(void)
{
	TEST_ASSERT_FALSE(str_timingsafe_equal("abc", "abd"));
}

static void test_timingsafe_equal_different_lengths(void)
{
	TEST_ASSERT_FALSE(str_timingsafe_equal("abc", "ab"));
}

static void test_timingsafe_equal_empty(void)
{
	TEST_ASSERT_TRUE(str_timingsafe_equal("", ""));
}

/* str_tolower */

static void test_tolower_mixed(void)
{
	char s[] = "Hello World";
	str_tolower(s);
	TEST_ASSERT_EQUAL_STRING("hello world", s);
}

static void test_tolower_already_lower(void)
{
	char s[] = "hello";
	str_tolower(s);
	TEST_ASSERT_EQUAL_STRING("hello", s);
}

static void test_tolower_empty(void)
{
	char s[] = "";
	str_tolower(s); /* must not crash */
	TEST_ASSERT_EQUAL_STRING("", s);
}

/* str_hasprefix */

static void test_hasprefix_match(void)
{
	TEST_ASSERT_TRUE(str_hasprefix("https://example.com", "https://"));
}

static void test_hasprefix_no_match(void)
{
	TEST_ASSERT_FALSE(str_hasprefix("http://example.com", "https://"));
}

static void test_hasprefix_empty_prefix(void)
{
	TEST_ASSERT_TRUE(str_hasprefix("anything", ""));
}

/* str_casestr */

static void test_casestr_case_insensitive_match(void)
{
	const char *p = str_casestr("Hello World", "WORLD");
	TEST_ASSERT_NOT_NULL(p);
	TEST_ASSERT_EQUAL_STRING("World", p);
}

static void test_casestr_not_found(void)
{
	TEST_ASSERT_NULL(str_casestr("Hello", "xyz"));
}

static void test_casestr_needle_longer_than_haystack(void)
{
	TEST_ASSERT_NULL(str_casestr("Hi", "Hello World"));
}

static void test_casestr_empty_needle(void)
{
	TEST_ASSERT_NULL(str_casestr("Hello", ""));
}

int main(void)
{
	UNITY_BEGIN();

	RUN_TEST(test_lcpy_normal);
	RUN_TEST(test_lcpy_truncation);
	RUN_TEST(test_lcpy_dstsize_one);
	RUN_TEST(test_lcpy_empty_src);

	RUN_TEST(test_split_single_token);
	RUN_TEST(test_split_three_tokens);
	RUN_TEST(test_split_consecutive_separators_skipped);
	RUN_TEST(test_split_max_exceeded);

	RUN_TEST(test_tonum_valid_decimal);
	RUN_TEST(test_tonum_valid_hex);
	RUN_TEST(test_tonum_out_of_range_low);
	RUN_TEST(test_tonum_out_of_range_high);
	RUN_TEST(test_tonum_non_numeric);
	RUN_TEST(test_tonum_empty_string);
	RUN_TEST(test_tonum_null);

	RUN_TEST(test_hex_known_input);
	RUN_TEST(test_hex_all_zeros);
	RUN_TEST(test_hex_buffer_too_small_no_write);

	RUN_TEST(test_timingsafe_equal_same);
	RUN_TEST(test_timingsafe_equal_different_same_length);
	RUN_TEST(test_timingsafe_equal_different_lengths);
	RUN_TEST(test_timingsafe_equal_empty);

	RUN_TEST(test_tolower_mixed);
	RUN_TEST(test_tolower_already_lower);
	RUN_TEST(test_tolower_empty);

	RUN_TEST(test_hasprefix_match);
	RUN_TEST(test_hasprefix_no_match);
	RUN_TEST(test_hasprefix_empty_prefix);

	RUN_TEST(test_casestr_case_insensitive_match);
	RUN_TEST(test_casestr_not_found);
	RUN_TEST(test_casestr_needle_longer_than_haystack);
	RUN_TEST(test_casestr_empty_needle);

	return UNITY_END();
}
