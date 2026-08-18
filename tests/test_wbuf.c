/* Tests for src/lib/wbuf.c */

#include <string.h>
#include <stdlib.h>
#include "unity.h"
#include "wbuf.h"

void setUp(void) {}
void tearDown(void) {}

/* wbuf_init / wbuf_cleanup */

static void test_init_hint_zero_defaults(void)
{
	struct wbuf b;
	wbuf_init(&b, 0);
	TEST_ASSERT_NOT_NULL(b.data);
	TEST_ASSERT_GREATER_OR_EQUAL(64, b.length);
	TEST_ASSERT_EQUAL_size_t(0, b.offset);
	wbuf_cleanup(&b);
}

static void test_init_with_hint(void)
{
	struct wbuf b;
	wbuf_init(&b, 128);
	TEST_ASSERT_NOT_NULL(b.data);
	TEST_ASSERT_EQUAL_size_t(128, b.length);
	wbuf_cleanup(&b);
}

static void test_cleanup_zeroes_fields(void)
{
	struct wbuf b;
	wbuf_init(&b, 64);
	wbuf_cleanup(&b);
	TEST_ASSERT_NULL(b.data);
	TEST_ASSERT_EQUAL_size_t(0, b.length);
	TEST_ASSERT_EQUAL_size_t(0, b.offset);
}

/* wbuf_alloc / wbuf_free */

static void test_alloc_free_lifecycle(void)
{
	struct wbuf *b = wbuf_alloc(32);
	TEST_ASSERT_NOT_NULL(b);
	TEST_ASSERT_NOT_NULL(b->data);
	wbuf_free(b); /* must not crash */
}

static void test_free_null_is_safe(void)
{
	wbuf_free(NULL); /* must not crash */
}

/* wbuf_append */

static void test_append_single(void)
{
	struct wbuf b;
	wbuf_init(&b, 8);
	wbuf_append(&b, "hello", 5);
	TEST_ASSERT_EQUAL_size_t(5, b.offset);
	TEST_ASSERT_EQUAL_MEMORY("hello", b.data, 5);
	wbuf_cleanup(&b);
}

static void test_append_multiple(void)
{
	struct wbuf b;
	wbuf_init(&b, 4);
	wbuf_append(&b, "ab", 2);
	wbuf_append(&b, "cd", 2);
	TEST_ASSERT_EQUAL_size_t(4, b.offset);
	TEST_ASSERT_EQUAL_MEMORY("abcd", b.data, 4);
	wbuf_cleanup(&b);
}

static void test_append_grows_beyond_hint(void)
{
	struct wbuf b;
	const char *chunk = "0123456789";
	int i;
	wbuf_init(&b, 8);
	for (i = 0; i < 20; i++)
		wbuf_append(&b, chunk, 10);
	TEST_ASSERT_EQUAL_size_t(200, b.offset);
	wbuf_cleanup(&b);
}

/* wbuf_appendf */

static void test_appendf_integer(void)
{
	struct wbuf b;
	wbuf_init(&b, 16);
	wbuf_appendf(&b, "%d", 42);
	TEST_ASSERT_EQUAL_STRING("42", wbuf_stringify(&b, NULL));
	wbuf_cleanup(&b);
}

static void test_appendf_long_string_exceeds_stack_buf(void)
{
	struct wbuf b;
	/* Generate a format string result longer than 512 bytes */
	char big[600];
	size_t len;
	memset(big, 'x', sizeof(big) - 1);
	big[sizeof(big) - 1] = '\0';
	wbuf_init(&b, 16);
	wbuf_appendf(&b, "%s", big);
	TEST_ASSERT_EQUAL_size_t(sizeof(big) - 1, b.offset);
	TEST_ASSERT_EQUAL_STRING(big, wbuf_stringify(&b, &len));
	TEST_ASSERT_EQUAL_size_t(sizeof(big) - 1, len);
	wbuf_cleanup(&b);
}

/* wbuf_reset */

static void test_reset_clears_offset(void)
{
	struct wbuf b;
	wbuf_init(&b, 16);
	wbuf_append(&b, "hello", 5);
	wbuf_reset(&b);
	TEST_ASSERT_EQUAL_size_t(0, b.offset);
	wbuf_cleanup(&b);
}

static void test_reset_then_append(void)
{
	struct wbuf b;
	wbuf_init(&b, 16);
	wbuf_append(&b, "old", 3);
	wbuf_reset(&b);
	wbuf_append(&b, "new", 3);
	TEST_ASSERT_EQUAL_STRING("new", wbuf_stringify(&b, NULL));
	wbuf_cleanup(&b);
}

/* wbuf_stringify */

static void test_stringify_nul_terminated(void)
{
	struct wbuf b;
	wbuf_init(&b, 8);
	wbuf_append(&b, "hi", 2);
	const char *s = wbuf_stringify(&b, NULL);
	TEST_ASSERT_EQUAL_STRING("hi", s);
	wbuf_cleanup(&b);
}

static void test_stringify_length_param(void)
{
	struct wbuf b;
	size_t len = 0;
	wbuf_init(&b, 8);
	wbuf_append(&b, "hello", 5);
	wbuf_stringify(&b, &len);
	TEST_ASSERT_EQUAL_size_t(5, len);
	wbuf_cleanup(&b);
}

/* wbuf_replace_string */

static void test_replace_single_occurrence(void)
{
	struct wbuf b;
	wbuf_init(&b, 32);
	wbuf_append(&b, "hello $NAME$!", 13);
	wbuf_replace_string(&b, "$NAME$", "world", 5);
	TEST_ASSERT_EQUAL_STRING("hello world!", wbuf_stringify(&b, NULL));
	wbuf_cleanup(&b);
}

static void test_replace_multiple_occurrences(void)
{
	struct wbuf b;
	wbuf_init(&b, 32);
	wbuf_append(&b, "a-b-a", 5);
	wbuf_replace_string(&b, "a", "X", 1);
	TEST_ASSERT_EQUAL_STRING("X-b-X", wbuf_stringify(&b, NULL));
	wbuf_cleanup(&b);
}

static void test_replace_no_match(void)
{
	struct wbuf b;
	wbuf_init(&b, 16);
	wbuf_append(&b, "hello", 5);
	wbuf_replace_string(&b, "xyz", "Q", 1);
	TEST_ASSERT_EQUAL_STRING("hello", wbuf_stringify(&b, NULL));
	wbuf_cleanup(&b);
}

static void test_replace_longer_than_needle(void)
{
	struct wbuf b;
	wbuf_init(&b, 16);
	wbuf_append(&b, "aXb", 3);
	wbuf_replace_string(&b, "X", "LONG", 4);
	TEST_ASSERT_EQUAL_STRING("aLONGb", wbuf_stringify(&b, NULL));
	wbuf_cleanup(&b);
}

static void test_replace_shorter_than_needle(void)
{
	struct wbuf b;
	wbuf_init(&b, 16);
	wbuf_append(&b, "aLONGb", 6);
	wbuf_replace_string(&b, "LONG", "X", 1);
	TEST_ASSERT_EQUAL_STRING("aXb", wbuf_stringify(&b, NULL));
	wbuf_cleanup(&b);
}

static void test_replace_preserves_owner_api_flag(void)
{
	struct wbuf *b = wbuf_alloc(16);
	TEST_ASSERT_EQUAL_INT(1, b->owner_api);
	wbuf_append(b, "hello", 5);
	wbuf_replace_string(b, "hello", "bye", 3);
	TEST_ASSERT_EQUAL_INT(1, b->owner_api); /* must survive the replace */
	wbuf_free(b);
}

int main(void)
{
	UNITY_BEGIN();

	RUN_TEST(test_init_hint_zero_defaults);
	RUN_TEST(test_init_with_hint);
	RUN_TEST(test_cleanup_zeroes_fields);

	RUN_TEST(test_alloc_free_lifecycle);
	RUN_TEST(test_free_null_is_safe);

	RUN_TEST(test_append_single);
	RUN_TEST(test_append_multiple);
	RUN_TEST(test_append_grows_beyond_hint);

	RUN_TEST(test_appendf_integer);
	RUN_TEST(test_appendf_long_string_exceeds_stack_buf);

	RUN_TEST(test_reset_clears_offset);
	RUN_TEST(test_reset_then_append);

	RUN_TEST(test_stringify_nul_terminated);
	RUN_TEST(test_stringify_length_param);

	RUN_TEST(test_replace_single_occurrence);
	RUN_TEST(test_replace_multiple_occurrences);
	RUN_TEST(test_replace_no_match);
	RUN_TEST(test_replace_longer_than_needle);
	RUN_TEST(test_replace_shorter_than_needle);
	RUN_TEST(test_replace_preserves_owner_api_flag);

	return UNITY_END();
}
