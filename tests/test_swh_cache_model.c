/* Tests for src/models/swh_cache.c (requires tmpdir fixture) */

#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "unity.h"
#include "app.h"
#include "models.h"

static char g_orig_cwd[4096];
static char g_tmpdir[64];

void setUp(void)
{
	getcwd(g_orig_cwd, sizeof(g_orig_cwd));
	strcpy(g_tmpdir, "/tmp/ms-swh-XXXXXX");
	mkdtemp(g_tmpdir);
	chdir(g_tmpdir);
	store_verify_dirs();
}

void tearDown(void)
{
	char cmd[128];
	chdir(g_orig_cwd);
	snprintf(cmd, sizeof(cmd), "rm -rf %s", g_tmpdir);
	system(cmd);
}

/* swh_cache_get before cache file exists */

static void test_swh_get_no_file(void)
{
	int status = SWH_STATUS_ARCHIVED; /* pre-set to non-UNKNOWN */
	time_t checked_at = 99;
	TEST_ASSERT_EQUAL_INT(-1,
	    swh_cache_get("https://example.com", &status, &checked_at));
	TEST_ASSERT_EQUAL_INT(SWH_STATUS_UNKNOWN, status);
	TEST_ASSERT_EQUAL_INT64(0, (long long)checked_at);
}

/* set then get */

static void test_swh_set_then_get(void)
{
	int status;
	time_t checked_at;
	const char *url = "https://github.com/user/repo";
	time_t ts = 1700000000;

	TEST_ASSERT_EQUAL_INT(0,
	    swh_cache_set(url, SWH_STATUS_ARCHIVED, ts));
	TEST_ASSERT_EQUAL_INT(0,
	    swh_cache_get(url, &status, &checked_at));
	TEST_ASSERT_EQUAL_INT(SWH_STATUS_ARCHIVED, status);
	TEST_ASSERT_EQUAL_INT64((long long)ts, (long long)checked_at);
}

/* get for URL not in cache */

static void test_swh_get_url_not_in_cache(void)
{
	int status;
	time_t checked_at;
	swh_cache_set("https://other.com", SWH_STATUS_ARCHIVED, 1000);
	TEST_ASSERT_EQUAL_INT(-1,
	    swh_cache_get("https://nothere.com", &status, &checked_at));
}

/* set updates existing entry — no duplicate rows */

static void test_swh_set_updates_existing(void)
{
	int status;
	time_t checked_at;
	const char *url = "https://github.com/user/repo";

	swh_cache_set(url, SWH_STATUS_NOT_FOUND, 1000);
	swh_cache_set(url, SWH_STATUS_ARCHIVED, 2000); /* update */

	TEST_ASSERT_EQUAL_INT(0, swh_cache_get(url, &status, &checked_at));
	TEST_ASSERT_EQUAL_INT(SWH_STATUS_ARCHIVED, status);
	TEST_ASSERT_EQUAL_INT64(2000, (long long)checked_at);
}

/* multiple URLs coexist independently */

static void test_swh_multiple_urls_independent(void)
{
	int s1, s2;
	time_t t1, t2;
	const char *url1 = "https://github.com/u/repo1";
	const char *url2 = "https://gitlab.com/u/repo2";

	swh_cache_set(url1, SWH_STATUS_ARCHIVED, 1111);
	swh_cache_set(url2, SWH_STATUS_NOT_FOUND, 2222);

	swh_cache_get(url1, &s1, &t1);
	swh_cache_get(url2, &s2, &t2);

	TEST_ASSERT_EQUAL_INT(SWH_STATUS_ARCHIVED, s1);
	TEST_ASSERT_EQUAL_INT64(1111, (long long)t1);
	TEST_ASSERT_EQUAL_INT(SWH_STATUS_NOT_FOUND, s2);
	TEST_ASSERT_EQUAL_INT64(2222, (long long)t2);
}

int main(void)
{
	UNITY_BEGIN();

	RUN_TEST(test_swh_get_no_file);
	RUN_TEST(test_swh_set_then_get);
	RUN_TEST(test_swh_get_url_not_in_cache);
	RUN_TEST(test_swh_set_updates_existing);
	RUN_TEST(test_swh_multiple_urls_independent);

	return UNITY_END();
}
