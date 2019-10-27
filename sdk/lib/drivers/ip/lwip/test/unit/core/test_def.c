#include "test_def.h"

#include "lwip/def.h"

#define MAGIC_UNTOUCHED_BYTE  0x7a
#define TEST_BUFSIZE          32
#define GUARD_SIZE            4

/* Setups/teardown functions */

static void
def_setup(void)
{
}

static void
def_teardown(void)
{
}

static void
def_check_range_untouched(const char *buf, size_t len)
{
  size_t i;

  for (i = 0; i < len; i++) {
    fail_unless(buf[i] == (char)MAGIC_UNTOUCHED_BYTE);
  }
}

static void test_def_itoa(int number, const char *expected)
{
  char buf[TEST_BUFSIZE];
  char *test_buf = &buf[GUARD_SIZE];

  size_t exp_len = strlen(expected);
  fail_unless(exp_len + 4 < (TEST_BUFSIZE - (2 * GUARD_SIZE)));

  memset(buf, MAGIC_UNTOUCHED_BYTE, sizeof(buf));
  lwip_itoa(test_buf, exp_len + 1, number);
  def_check_range_untouched(buf, GUARD_SIZE);
  fail_unless(test_buf[exp_len] == 0);
  fail_unless(!memcmp(test_buf, expected, exp_len));
  def_check_range_untouched(&test_buf[exp_len + 1], TEST_BUFSIZE - GUARD_SIZE - exp_len - 1);

  /* check with too small buffer */
  memset(buf, MAGIC_UNTOUCHED_BYTE, sizeof(buf));
  lwip_itoa(test_buf, exp_len, number);
  def_check_range_untouched(buf, GUARD_SIZE);
  def_check_range_untouched(&test_buf[exp_len + 1], TEST_BUFSIZE - GUARD_SIZE - exp_len - 1);

  /* check with too large buffer */
  memset(buf, MAGIC_UNTOUCHED_BYTE, sizeof(buf));
  lwip_itoa(test_buf, exp_len + 4, number);
  def_check_range_untouched(buf, GUARD_SIZE);
  fail_unless(test_buf[exp_len] == 0);
  fail_unless(!memcmp(test_buf, expected, exp_len));
  def_check_range_untouched(&test_buf[exp_len + 4], TEST_BUFSIZE - GUARD_SIZE - exp_len - 4);
}

START_TEST(test_def_lwip_itoa)
{
  LWIP_UNUSED_ARG(_i);

  test_def_itoa(0, "0");
  test_def_itoa(1, "1");
  test_def_itoa(-1, "-1");
  test_def_itoa(15, "15");
  test_def_itoa(-15, "-15");
  test_def_itoa(156, "156");
  test_def_itoa(1192, "1192");
  test_def_itoa(-156, "-156");
}
END_TEST

/** Create the suite including all tests for this module */
Suite *
def_suite(void)
{
  testfunc tests[] = {
    TESTFUNC(test_def_lwip_itoa)
  };
  return create_suite("DEF", tests, sizeof(tests)/sizeof(testfunc), def_setup, def_teardown);
}
