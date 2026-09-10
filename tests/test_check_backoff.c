/* Tests for src/lib/check_backoff.c */

#include <time.h>

#include "unity.h"
#include "check_backoff.h"

void setUp(void) {}
void tearDown(void) {}

static void test_interval_base(void)
{
	TEST_ASSERT_EQUAL_INT64(CHECK_BACKOFF_BASE_SEC,
	    (long long)check_backoff_interval(0));
}

static void test_interval_doubles(void)
{
	TEST_ASSERT_EQUAL_INT64(2 * CHECK_BACKOFF_BASE_SEC,
	    (long long)check_backoff_interval(1));
	TEST_ASSERT_EQUAL_INT64(4 * CHECK_BACKOFF_BASE_SEC,
	    (long long)check_backoff_interval(2));
	TEST_ASSERT_EQUAL_INT64(8 * CHECK_BACKOFF_BASE_SEC,
	    (long long)check_backoff_interval(3));
}

static void test_interval_caps_at_max(void)
{
	TEST_ASSERT_EQUAL_INT64(CHECK_BACKOFF_MAX_SEC,
	    (long long)check_backoff_interval(20));
	TEST_ASSERT_EQUAL_INT64(CHECK_BACKOFF_MAX_SEC,
	    (long long)check_backoff_interval(100));
}

static void test_due_never_checked(void)
{
	TEST_ASSERT_TRUE(check_backoff_due(0, 0, 1000));
	TEST_ASSERT_TRUE(check_backoff_due(0, 5, 1000));
}

static void test_due_before_interval(void)
{
	time_t checked = 1000;

	TEST_ASSERT_FALSE(check_backoff_due(checked, 0,
	    checked + CHECK_BACKOFF_BASE_SEC - 1));
}

static void test_due_at_interval(void)
{
	time_t checked = 1000;

	TEST_ASSERT_TRUE(check_backoff_due(checked, 0,
	    checked + CHECK_BACKOFF_BASE_SEC));
}

int main(void)
{
	UNITY_BEGIN();

	RUN_TEST(test_interval_base);
	RUN_TEST(test_interval_doubles);
	RUN_TEST(test_interval_caps_at_max);
	RUN_TEST(test_due_never_checked);
	RUN_TEST(test_due_before_interval);
	RUN_TEST(test_due_at_interval);

	return UNITY_END();
}
