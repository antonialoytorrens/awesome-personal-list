/* Tests for src/lib/translit.c */

#include <string.h>
#include "unity.h"
#include "translit.h"

void setUp(void) {}
void tearDown(void) {}

/* utf8_decode */

static void test_decode_ascii(void)
{
	char s[] = "A";
	const char *p = s;
	unsigned int cp = utf8_decode(&p);
	TEST_ASSERT_EQUAL_UINT(0x41, cp);
	TEST_ASSERT_EQUAL_PTR(s + 1, p);
}

static void test_decode_two_byte(void)
{
	/* U+00F3 (LATIN SMALL LETTER O WITH ACUTE), UTF-8: 0xC3 0xB3 */
	const char s[] = { (char)0xc3, (char)0xb3, '\0' };
	const char *p = s;
	unsigned int cp = utf8_decode(&p);
	TEST_ASSERT_EQUAL_UINT(0x00f3, cp);
	TEST_ASSERT_EQUAL_PTR(s + 2, p);
}

static void test_decode_three_byte(void)
{
	/* U+20AC (EURO SIGN), UTF-8: 0xE2 0x82 0xAC */
	const char s[] = { (char)0xe2, (char)0x82, (char)0xac, '\0' };
	const char *p = s;
	unsigned int cp = utf8_decode(&p);
	TEST_ASSERT_EQUAL_UINT(0x20ac, cp);
	TEST_ASSERT_EQUAL_PTR(s + 3, p);
}

static void test_decode_walks_full_string(void)
{
	/* "educaci\xc3\xb3" -- 7 ASCII bytes then the 2-byte "o" acute. */
	char s[] = "educaci\xc3\xb3";
	const char *p = s;
	int i;

	for (i = 0; i < 7; i++)
		TEST_ASSERT_EQUAL_UINT((unsigned char)s[i], utf8_decode(&p));

	TEST_ASSERT_EQUAL_UINT(0x00f3, utf8_decode(&p));
	TEST_ASSERT_EQUAL_CHAR('\0', *p);
}

static void test_decode_stray_continuation_byte(void)
{
	const char s[] = { (char)0x80, 'x', '\0' };
	const char *p = s;
	unsigned int cp = utf8_decode(&p);
	TEST_ASSERT_EQUAL_UINT(0xfffd, cp);
	TEST_ASSERT_EQUAL_PTR(p, s + 1); /* consumed exactly one byte */
}

static void test_decode_truncated_at_end_of_string(void)
{
	/* lead byte announces a 2-byte sequence, but the string ends (NUL)
	 * right after -- must not read past the NUL. */
	const char s[] = { (char)0xc3, '\0' };
	const char *p = s;
	unsigned int cp = utf8_decode(&p);
	TEST_ASSERT_EQUAL_UINT(0xfffd, cp);
	TEST_ASSERT_EQUAL_PTR(p, s + 1); /* stopped before the NUL */
}

/* translit_ascii */

static void test_translit_o_acute(void)
{
	TEST_ASSERT_EQUAL_STRING("o", translit_ascii(0x00f3));
}

static void test_translit_o_grave(void)
{
	TEST_ASSERT_EQUAL_STRING("o", translit_ascii(0x00f2));
}

static void test_translit_n_tilde(void)
{
	TEST_ASSERT_EQUAL_STRING("n", translit_ascii(0x00f1));
}

static void test_translit_c_cedilla(void)
{
	TEST_ASSERT_EQUAL_STRING("c", translit_ascii(0x00e7));
}

static void test_translit_ae_ligature(void)
{
	TEST_ASSERT_EQUAL_STRING("ae", translit_ascii(0x00e6));
}

static void test_translit_sharp_s(void)
{
	TEST_ASSERT_EQUAL_STRING("ss", translit_ascii(0x00df));
}

static void test_translit_uppercase_a_grave(void)
{
	TEST_ASSERT_EQUAL_STRING("a", translit_ascii(0x00c0));
}

static void test_translit_unmapped_symbol(void)
{
	TEST_ASSERT_NULL(translit_ascii(0x00d7)); /* multiplication sign */
}

static void test_translit_unmapped_out_of_range(void)
{
	TEST_ASSERT_NULL(translit_ascii(0x20ac)); /* euro sign */
}

int main(void)
{
	UNITY_BEGIN();

	RUN_TEST(test_decode_ascii);
	RUN_TEST(test_decode_two_byte);
	RUN_TEST(test_decode_three_byte);
	RUN_TEST(test_decode_walks_full_string);
	RUN_TEST(test_decode_stray_continuation_byte);
	RUN_TEST(test_decode_truncated_at_end_of_string);

	RUN_TEST(test_translit_o_acute);
	RUN_TEST(test_translit_o_grave);
	RUN_TEST(test_translit_n_tilde);
	RUN_TEST(test_translit_c_cedilla);
	RUN_TEST(test_translit_ae_ligature);
	RUN_TEST(test_translit_sharp_s);
	RUN_TEST(test_translit_uppercase_a_grave);
	RUN_TEST(test_translit_unmapped_symbol);
	RUN_TEST(test_translit_unmapped_out_of_range);

	return UNITY_END();
}
