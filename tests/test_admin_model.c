/* Tests for src/models/admin.c (requires tmpdir fixture) */

#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "unity.h"
#include "app.h"
#include "models.h"

static char g_orig_cwd[4096];
static char g_tmpdir[64];

void setUp(void)
{
	getcwd(g_orig_cwd, sizeof(g_orig_cwd));
	strcpy(g_tmpdir, "/tmp/ms-adm-XXXXXX");
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

/* admin_exists before any account is created */

static void test_admin_not_exists_initially(void)
{
	TEST_ASSERT_FALSE(admin_exists());
}

/* admin_create — success path */

static void test_admin_create_success(void)
{
	TEST_ASSERT_EQUAL_INT(0, admin_create("alice", "securepassword"));
	TEST_ASSERT_TRUE(admin_exists());
}

/* admin_create — null name */

static void test_admin_create_null_name(void)
{
	TEST_ASSERT_EQUAL_INT(-1, admin_create(NULL, "password123"));
}

/* admin_create — empty name */

static void test_admin_create_empty_name(void)
{
	TEST_ASSERT_EQUAL_INT(-1, admin_create("", "password123"));
}

/* admin_create — password too short (< 8 chars) */

static void test_admin_create_short_password(void)
{
	TEST_ASSERT_EQUAL_INT(-1, admin_create("alice", "short"));
}

/* admin_create — second call is refused */

static void test_admin_create_second_call_refused(void)
{
	admin_create("alice", "password123");
	TEST_ASSERT_EQUAL_INT(-2, admin_create("bob", "password456"));
}

/* admin_lookup by name */

static void test_admin_lookup_by_name(void)
{
	struct admin a;
	admin_create("alice", "password123");
	TEST_ASSERT_EQUAL_INT(0, admin_lookup("alice", &a));
	TEST_ASSERT_EQUAL_STRING("alice", a.name);
}

/* admin_lookup with NULL name returns first row */

static void test_admin_lookup_null_name_returns_first(void)
{
	struct admin a;
	admin_create("alice", "password123");
	TEST_ASSERT_EQUAL_INT(0, admin_lookup(NULL, &a));
	TEST_ASSERT_EQUAL_STRING("alice", a.name);
}

/* admin_lookup with wrong name */

static void test_admin_lookup_wrong_name(void)
{
	admin_create("alice", "password123");
	TEST_ASSERT_EQUAL_INT(-1, admin_lookup("nobody", NULL));
}

/* admin_verify — correct credentials */

static void test_admin_verify_correct_password(void)
{
	admin_create("alice", "password123");
	TEST_ASSERT_EQUAL_INT(0, admin_verify("alice", "password123", NULL));
}

/* admin_verify — wrong password */

static void test_admin_verify_wrong_password(void)
{
	admin_create("alice", "password123");
	TEST_ASSERT_EQUAL_INT(-1, admin_verify("alice", "wrongpass", NULL));
}

/* admin_verify — NULL name maps to first admin row */

static void test_admin_verify_null_name(void)
{
	/* admin_lookup(NULL, ...) intentionally returns the first row;
	 * verify with correct password therefore succeeds */
	admin_create("alice", "password123");
	TEST_ASSERT_EQUAL_INT(0, admin_verify(NULL, "password123", NULL));
}

/* admin_verify — non-existent user */

static void test_admin_verify_no_such_user(void)
{
	TEST_ASSERT_EQUAL_INT(-1, admin_verify("ghost", "anything", NULL));
}

/* admin_update_language */

static void test_admin_update_language(void)
{
	struct admin a;
	admin_create("alice", "password123");
	TEST_ASSERT_EQUAL_INT(0, admin_update_language("alice", "ca"));
	admin_lookup("alice", &a);
	TEST_ASSERT_EQUAL_STRING("ca", a.language);
}

/* admin_update_name */

static void test_admin_update_name_success(void)
{
	struct admin a;
	admin_create("alice", "password123");
	TEST_ASSERT_EQUAL_INT(0, admin_update_name("alice", "alice2"));
	TEST_ASSERT_EQUAL_INT(0, admin_lookup("alice2", &a));
	TEST_ASSERT_EQUAL_STRING("alice2", a.name);
}

static void test_admin_update_name_old_name_gone(void)
{
	admin_create("alice", "password123");
	admin_update_name("alice", "alice2");
	TEST_ASSERT_EQUAL_INT(-1, admin_lookup("alice", NULL));
}

static void test_admin_update_name_nonexistent(void)
{
	TEST_ASSERT_EQUAL_INT(-1, admin_update_name("ghost", "newname"));
}

static void test_admin_update_name_too_long(void)
{
	char buf[NAME_MAX_LEN + 1];
	admin_create("alice", "password123");
	memset(buf, 'a', NAME_MAX_LEN);
	buf[NAME_MAX_LEN] = '\0';
	TEST_ASSERT_EQUAL_INT(-1, admin_update_name("alice", buf));
}

static void test_admin_update_name_preserves_password(void)
{
	admin_create("alice", "password123");
	admin_update_name("alice", "alice2");
	TEST_ASSERT_EQUAL_INT(0, admin_verify("alice2", "password123", NULL));
}

static void test_admin_update_name_preserves_language(void)
{
	struct admin a;
	admin_create("alice", "password123");
	admin_update_language("alice", "es");
	admin_update_name("alice", "alice2");
	admin_lookup("alice2", &a);
	TEST_ASSERT_EQUAL_STRING("es", a.language);
}

/* admin_verify returns populated admin struct */

static void test_admin_verify_populates_out(void)
{
	struct admin a;
	admin_create("alice", "password123");
	TEST_ASSERT_EQUAL_INT(0, admin_verify("alice", "password123", &a));
	TEST_ASSERT_EQUAL_STRING("alice", a.name);
}

int main(void)
{
	UNITY_BEGIN();

	RUN_TEST(test_admin_not_exists_initially);
	RUN_TEST(test_admin_create_success);
	RUN_TEST(test_admin_create_null_name);
	RUN_TEST(test_admin_create_empty_name);
	RUN_TEST(test_admin_create_short_password);
	RUN_TEST(test_admin_create_second_call_refused);
	RUN_TEST(test_admin_lookup_by_name);
	RUN_TEST(test_admin_lookup_null_name_returns_first);
	RUN_TEST(test_admin_lookup_wrong_name);
	RUN_TEST(test_admin_verify_correct_password);
	RUN_TEST(test_admin_verify_wrong_password);
	RUN_TEST(test_admin_verify_null_name);
	RUN_TEST(test_admin_verify_no_such_user);
	RUN_TEST(test_admin_update_language);
	RUN_TEST(test_admin_update_name_success);
	RUN_TEST(test_admin_update_name_old_name_gone);
	RUN_TEST(test_admin_update_name_nonexistent);
	RUN_TEST(test_admin_update_name_too_long);
	RUN_TEST(test_admin_update_name_preserves_password);
	RUN_TEST(test_admin_update_name_preserves_language);
	RUN_TEST(test_admin_verify_populates_out);

	return UNITY_END();
}
