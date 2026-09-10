/* Tests for src/models/swh_cache.c (requires tmpdir fixture) */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
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
	unsigned check_count = 7;
	TEST_ASSERT_EQUAL_INT(-1,
	    swh_cache_get("https://example.com", &status, &checked_at,
	    &check_count));
	TEST_ASSERT_EQUAL_INT(SWH_STATUS_UNKNOWN, status);
	TEST_ASSERT_EQUAL_INT64(0, (long long)checked_at);
	TEST_ASSERT_EQUAL_UINT(0, check_count);
}

/* set then get */

static void test_swh_set_then_get(void)
{
	int status;
	time_t checked_at;
	unsigned check_count;
	const char *url = "https://github.com/user/repo";
	time_t ts = 1700000000;

	TEST_ASSERT_EQUAL_INT(0,
	    swh_cache_set(url, SWH_STATUS_ARCHIVED, ts, 4));
	TEST_ASSERT_EQUAL_INT(0,
	    swh_cache_get(url, &status, &checked_at, &check_count));
	TEST_ASSERT_EQUAL_INT(SWH_STATUS_ARCHIVED, status);
	TEST_ASSERT_EQUAL_INT64((long long)ts, (long long)checked_at);
	TEST_ASSERT_EQUAL_UINT(4, check_count);
}

/* get for URL not in cache */

static void test_swh_get_url_not_in_cache(void)
{
	int status;
	time_t checked_at;
	unsigned check_count;
	swh_cache_set("https://other.com", SWH_STATUS_ARCHIVED, 1000, 0);
	TEST_ASSERT_EQUAL_INT(-1,
	    swh_cache_get("https://nothere.com", &status, &checked_at,
	    &check_count));
}

/* set updates existing entry — no duplicate rows */

static void test_swh_set_updates_existing(void)
{
	int status;
	time_t checked_at;
	unsigned check_count;
	const char *url = "https://github.com/user/repo";

	swh_cache_set(url, SWH_STATUS_NOT_FOUND, 1000, 1);
	swh_cache_set(url, SWH_STATUS_ARCHIVED, 2000, 0); /* update + reset */

	TEST_ASSERT_EQUAL_INT(0,
	    swh_cache_get(url, &status, &checked_at, &check_count));
	TEST_ASSERT_EQUAL_INT(SWH_STATUS_ARCHIVED, status);
	TEST_ASSERT_EQUAL_INT64(2000, (long long)checked_at);
	TEST_ASSERT_EQUAL_UINT(0, check_count);
}

/* multiple URLs coexist independently */

static void test_swh_multiple_urls_independent(void)
{
	int s1, s2;
	time_t t1, t2;
	unsigned c1, c2;
	const char *url1 = "https://github.com/u/repo1";
	const char *url2 = "https://gitlab.com/u/repo2";

	swh_cache_set(url1, SWH_STATUS_ARCHIVED, 1111, 2);
	swh_cache_set(url2, SWH_STATUS_NOT_FOUND, 2222, 5);

	swh_cache_get(url1, &s1, &t1, &c1);
	swh_cache_get(url2, &s2, &t2, &c2);

	TEST_ASSERT_EQUAL_INT(SWH_STATUS_ARCHIVED, s1);
	TEST_ASSERT_EQUAL_INT64(1111, (long long)t1);
	TEST_ASSERT_EQUAL_UINT(2, c1);
	TEST_ASSERT_EQUAL_INT(SWH_STATUS_NOT_FOUND, s2);
	TEST_ASSERT_EQUAL_INT64(2222, (long long)t2);
	TEST_ASSERT_EQUAL_UINT(5, c2);
}

/* Legacy 3-column rows parse as check_count == 0 */

static void test_swh_legacy_three_column_row(void)
{
	FILE *fp;
	int status;
	time_t checked_at;
	unsigned check_count;
	const char *url = "https://github.com/legacy/repo";

	fp = fopen(SWH_CACHE_FILE, "w");
	TEST_ASSERT_NOT_NULL(fp);
	fprintf(fp, "%s\t%d\t%lld\n", url, SWH_STATUS_NOT_FOUND,
	    (long long)12345);
	fclose(fp);

	TEST_ASSERT_EQUAL_INT(0,
	    swh_cache_get(url, &status, &checked_at, &check_count));
	TEST_ASSERT_EQUAL_INT(SWH_STATUS_NOT_FOUND, status);
	TEST_ASSERT_EQUAL_INT64(12345, (long long)checked_at);
	TEST_ASSERT_EQUAL_UINT(0, check_count);
}

int main(void)
{
	UNITY_BEGIN();

	RUN_TEST(test_swh_get_no_file);
	RUN_TEST(test_swh_set_then_get);
	RUN_TEST(test_swh_get_url_not_in_cache);
	RUN_TEST(test_swh_set_updates_existing);
	RUN_TEST(test_swh_multiple_urls_independent);
	RUN_TEST(test_swh_legacy_three_column_row);

	return UNITY_END();
}
