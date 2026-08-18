/* Tests for src/lib/helpers.c */

#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "unity.h"
#include "helpers.h"
#include "wbuf.h"

void setUp(void) {}
void tearDown(void) {}

/* html_escape (via wbuf) */

static void test_html_escape_ampersand(void)
{
	struct wbuf b;
	wbuf_init(&b, 32);
	html_escape(&b, "&", 1);
	TEST_ASSERT_EQUAL_STRING("&amp;", wbuf_stringify(&b, NULL));
	wbuf_cleanup(&b);
}

static void test_html_escape_lt(void)
{
	struct wbuf b;
	wbuf_init(&b, 32);
	html_escape(&b, "<", 1);
	TEST_ASSERT_EQUAL_STRING("&lt;", wbuf_stringify(&b, NULL));
	wbuf_cleanup(&b);
}

static void test_html_escape_gt(void)
{
	struct wbuf b;
	wbuf_init(&b, 32);
	html_escape(&b, ">", 1);
	TEST_ASSERT_EQUAL_STRING("&gt;", wbuf_stringify(&b, NULL));
	wbuf_cleanup(&b);
}

static void test_html_escape_quote(void)
{
	struct wbuf b;
	wbuf_init(&b, 32);
	html_escape(&b, "\"", 1);
	TEST_ASSERT_EQUAL_STRING("&quot;", wbuf_stringify(&b, NULL));
	wbuf_cleanup(&b);
}

static void test_html_escape_apostrophe(void)
{
	struct wbuf b;
	wbuf_init(&b, 32);
	html_escape(&b, "'", 1);
	TEST_ASSERT_EQUAL_STRING("&#39;", wbuf_stringify(&b, NULL));
	wbuf_cleanup(&b);
}

static void test_html_escape_plain_ascii_unchanged(void)
{
	struct wbuf b;
	wbuf_init(&b, 32);
	html_escape(&b, "hello", 5);
	TEST_ASSERT_EQUAL_STRING("hello", wbuf_stringify(&b, NULL));
	wbuf_cleanup(&b);
}

static void test_html_escape_mixed(void)
{
	struct wbuf b;
	wbuf_init(&b, 64);
	html_escape(&b, "<b>Hi & Bye</b>", 15);
	TEST_ASSERT_EQUAL_STRING("&lt;b&gt;Hi &amp; Bye&lt;/b&gt;",
	    wbuf_stringify(&b, NULL));
	wbuf_cleanup(&b);
}

/* html_escape_str */

static void test_html_escape_str_heap_allocated(void)
{
	char *s = html_escape_str("<script>");
	TEST_ASSERT_EQUAL_STRING("&lt;script&gt;", s);
	free(s);
}

/* url_encode (via wbuf) */

static void test_url_encode_unreserved_passthrough(void)
{
	struct wbuf b;
	wbuf_init(&b, 32);
	url_encode(&b, "abcABC123-_.~", 13);
	TEST_ASSERT_EQUAL_STRING("abcABC123-_.~", wbuf_stringify(&b, NULL));
	wbuf_cleanup(&b);
}

static void test_url_encode_space(void)
{
	struct wbuf b;
	wbuf_init(&b, 16);
	url_encode(&b, " ", 1);
	TEST_ASSERT_EQUAL_STRING("%20", wbuf_stringify(&b, NULL));
	wbuf_cleanup(&b);
}

static void test_url_encode_slash(void)
{
	struct wbuf b;
	wbuf_init(&b, 16);
	url_encode(&b, "/", 1);
	TEST_ASSERT_EQUAL_STRING("%2F", wbuf_stringify(&b, NULL));
	wbuf_cleanup(&b);
}

static void test_url_encode_at(void)
{
	struct wbuf b;
	wbuf_init(&b, 16);
	url_encode(&b, "@", 1);
	TEST_ASSERT_EQUAL_STRING("%40", wbuf_stringify(&b, NULL));
	wbuf_cleanup(&b);
}

/* url_encode_str */

static void test_url_encode_str_heap_allocated(void)
{
	char *s = url_encode_str("hello world");
	TEST_ASSERT_EQUAL_STRING("hello%20world", s);
	free(s);
}

/* fmt_date */

static void test_fmt_date_zero_is_never(void)
{
	char buf[32];
	fmt_date(0, buf, sizeof(buf));
	TEST_ASSERT_EQUAL_STRING("never", buf);
}

static void test_fmt_date_known_timestamp(void)
{
	char buf[32];
	/* 2000-01-01 00:00:00 UTC = 946684800 */
	fmt_date(946684800, buf, sizeof(buf));
	TEST_ASSERT_EQUAL_STRING("2000-01-01", buf);
}

/* repo_display_name */

static void test_repo_display_url_with_path(void)
{
	TEST_ASSERT_EQUAL_STRING("repo",
	    repo_display_name("https://github.com/user/repo"));
}

static void test_repo_display_no_slash(void)
{
	TEST_ASSERT_EQUAL_STRING("example.com",
	    repo_display_name("example.com"));
}

static void test_repo_display_null(void)
{
	TEST_ASSERT_EQUAL_STRING("", repo_display_name(NULL));
}

static void test_repo_display_trailing_slash(void)
{
	/* last slash has nothing after it → slash[1] == '\0' → returns full url */
	const char *r = repo_display_name("https://example.com/");
	TEST_ASSERT_EQUAL_STRING("https://example.com/", r);
}

/* url_host_path */

static void test_url_host_path_strips_https(void)
{
	TEST_ASSERT_EQUAL_STRING("github.com/user/repo",
	    url_host_path("https://github.com/user/repo"));
}

static void test_url_host_path_no_scheme(void)
{
	TEST_ASSERT_EQUAL_STRING("example.com",
	    url_host_path("example.com"));
}

static void test_url_host_path_null(void)
{
	TEST_ASSERT_EQUAL_STRING("", url_host_path(NULL));
}

/* url_is_safe_next */

static void test_url_is_safe_next_valid_path(void)
{
	TEST_ASSERT_TRUE(url_is_safe_next("/dashboard"));
}

static void test_url_is_safe_next_double_slash(void)
{
	TEST_ASSERT_FALSE(url_is_safe_next("//evil.com"));
}

static void test_url_is_safe_next_empty(void)
{
	TEST_ASSERT_FALSE(url_is_safe_next(""));
}

static void test_url_is_safe_next_null(void)
{
	TEST_ASSERT_FALSE(url_is_safe_next(NULL));
}

static void test_url_is_safe_next_no_leading_slash(void)
{
	TEST_ASSERT_FALSE(url_is_safe_next("relative/path"));
}

int main(void)
{
	UNITY_BEGIN();

	RUN_TEST(test_html_escape_ampersand);
	RUN_TEST(test_html_escape_lt);
	RUN_TEST(test_html_escape_gt);
	RUN_TEST(test_html_escape_quote);
	RUN_TEST(test_html_escape_apostrophe);
	RUN_TEST(test_html_escape_plain_ascii_unchanged);
	RUN_TEST(test_html_escape_mixed);

	RUN_TEST(test_html_escape_str_heap_allocated);

	RUN_TEST(test_url_encode_unreserved_passthrough);
	RUN_TEST(test_url_encode_space);
	RUN_TEST(test_url_encode_slash);
	RUN_TEST(test_url_encode_at);

	RUN_TEST(test_url_encode_str_heap_allocated);

	RUN_TEST(test_fmt_date_zero_is_never);
	RUN_TEST(test_fmt_date_known_timestamp);

	RUN_TEST(test_repo_display_url_with_path);
	RUN_TEST(test_repo_display_no_slash);
	RUN_TEST(test_repo_display_null);
	RUN_TEST(test_repo_display_trailing_slash);

	RUN_TEST(test_url_host_path_strips_https);
	RUN_TEST(test_url_host_path_no_scheme);
	RUN_TEST(test_url_host_path_null);

	RUN_TEST(test_url_is_safe_next_valid_path);
	RUN_TEST(test_url_is_safe_next_double_slash);
	RUN_TEST(test_url_is_safe_next_empty);
	RUN_TEST(test_url_is_safe_next_null);
	RUN_TEST(test_url_is_safe_next_no_leading_slash);

	return UNITY_END();
}
