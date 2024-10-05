#include "test_timers.h"

#include "lwip/def.h"
#include "lwip/timeouts.h"
#include "arch/sys_arch.h"

/* Setups/teardown functions */

static struct sys_timeo* old_list_head;

static void
timers_setup(void)
{
  struct sys_timeo** list_head = sys_timeouts_get_next_timeout();
  old_list_head = *list_head;
  *list_head = NULL;
}

static void
timers_teardown(void)
{
  struct sys_timeo** list_head = sys_timeouts_get_next_timeout();
  *list_head = old_list_head;
  lwip_sys_now = 0;
}

static int fired[3];
static void
dummy_handler(void* arg)
{
  int index = LWIP_PTR_NUMERIC_CAST(int, arg);
  fired[index] = 1;
}

#define HANDLER_EXECUTION_TIME 5
static int cyclic_fired;
static void
dummy_cyclic_handler(void)
{
   cyclic_fired = 1;
   lwip_sys_now += HANDLER_EXECUTION_TIME;
}

struct lwip_cyclic_timer test_cyclic = {10, dummy_cyclic_handler};

static void
do_test_cyclic_timers(u32_t offset)
{
  struct sys_timeo** list_head = sys_timeouts_get_next_timeout();

  /* verify normal timer expiration */
  lwip_sys_now = offset + 0;
  sys_timeout(test_cyclic.interval_ms, lwip_cyclic_timer, &test_cyclic);

  cyclic_fired = 0;
  sys_check_timeouts();
  fail_unless(cyclic_fired == 0);

  lwip_sys_now = offset + test_cyclic.interval_ms;
  sys_check_timeouts();
  fail_unless(cyclic_fired == 1);

  fail_unless((*list_head)->time == (u32_t)(lwip_sys_now + test_cyclic.interval_ms - HANDLER_EXECUTION_TIME));
  
  sys_untimeout(lwip_cyclic_timer, &test_cyclic);


  /* verify "overload" - next cyclic timer execution is already overdue twice */
  lwip_sys_now = offset + 0;
  sys_timeout(test_cyclic.interval_ms, lwip_cyclic_timer, &test_cyclic);

  cyclic_fired = 0;
  sys_check_timeouts();
  fail_unless(cyclic_fired == 0);

  lwip_sys_now = offset + 2*test_cyclic.interval_ms;
  sys_check_timeouts();
  fail_unless(cyclic_fired == 1);

  fail_unless((*list_head)->time == (u32_t)(lwip_sys_now + test_cyclic.interval_ms));
}

START_TEST(test_cyclic_timers)
{
  LWIP_UNUSED_ARG(_i);

  /* check without u32_t wraparound */
  do_test_cyclic_timers(0);

  /* check with u32_t wraparound */
  do_test_cyclic_timers(0xfffffff0);
}
END_TEST

/* reproduce bug #52748: the bug in timeouts.c */
START_TEST(test_bug52748)
{
  LWIP_UNUSED_ARG(_i);

  memset(&fired, 0, sizeof(fired));

  lwip_sys_now = 50;
  sys_timeout(20, dummy_handler, LWIP_PTR_NUMERIC_CAST(void*, 0));
  sys_timeout( 5, dummy_handler, LWIP_PTR_NUMERIC_CAST(void*, 2));

  lwip_sys_now = 55;
  sys_check_timeouts();
  fail_unless(fired[0] == 0);
  fail_unless(fired[1] == 0);
  fail_unless(fired[2] == 1);

  lwip_sys_now = 60;
  sys_timeout(10, dummy_handler, LWIP_PTR_NUMERIC_CAST(void*, 1));
  sys_check_timeouts();
  fail_unless(fired[0] == 0);
  fail_unless(fired[1] == 0);
  fail_unless(fired[2] == 1);

  lwip_sys_now = 70;
  sys_check_timeouts();
  fail_unless(fired[0] == 1);
  fail_unless(fired[1] == 1);
  fail_unless(fired[2] == 1);
}
END_TEST

static void
do_test_timers(u32_t offset)
{
  struct sys_timeo** list_head = sys_timeouts_get_next_timeout();
  
  lwip_sys_now = offset + 0;

  sys_timeout(10, dummy_handler, LWIP_PTR_NUMERIC_CAST(void*, 0));
  fail_unless(sys_timeouts_sleeptime() == 10);
  sys_timeout(20, dummy_handler, LWIP_PTR_NUMERIC_CAST(void*, 1));
  fail_unless(sys_timeouts_sleeptime() == 10);
  sys_timeout( 5, dummy_handler, LWIP_PTR_NUMERIC_CAST(void*, 2));
  fail_unless(sys_timeouts_sleeptime() == 5);

  /* linked list correctly sorted? */
  fail_unless((*list_head)->time             == (u32_t)(lwip_sys_now + 5));
  fail_unless((*list_head)->next->time       == (u32_t)(lwip_sys_now + 10));
  fail_unless((*list_head)->next->next->time == (u32_t)(lwip_sys_now + 20));
  
  /* check timers expire in correct order */
  memset(&fired, 0, sizeof(fired));

  lwip_sys_now += 4;
  sys_check_timeouts();
  fail_unless(fired[2] == 0);

  lwip_sys_now += 1;
  sys_check_timeouts();
  fail_unless(fired[2] == 1);

  lwip_sys_now += 4;
  sys_check_timeouts();
  fail_unless(fired[0] == 0);

  lwip_sys_now += 1;
  sys_check_timeouts();
  fail_unless(fired[0] == 1);

  lwip_sys_now += 9;
  sys_check_timeouts();
  fail_unless(fired[1] == 0);

  lwip_sys_now += 1;
  sys_check_timeouts();
  fail_unless(fired[1] == 1);

  sys_untimeout(dummy_handler, LWIP_PTR_NUMERIC_CAST(void*, 0));
  sys_untimeout(dummy_handler, LWIP_PTR_NUMERIC_CAST(void*, 1));
  sys_untimeout(dummy_handler, LWIP_PTR_NUMERIC_CAST(void*, 2));
}

START_TEST(test_timers)
{
  LWIP_UNUSED_ARG(_i);

  /* check without u32_t wraparound */
  do_test_timers(0);

  /* check with u32_t wraparound */
  do_test_timers(0xfffffff0);
}
END_TEST

START_TEST(test_long_timer)
{
  LWIP_UNUSED_ARG(_i);

  memset(&fired, 0, sizeof(fired));
  lwip_sys_now = 0;

  sys_timeout(LWIP_UINT32_MAX / 4, dummy_handler, LWIP_PTR_NUMERIC_CAST(void*, 0));
  fail_unless(sys_timeouts_sleeptime() == LWIP_UINT32_MAX / 4);

  sys_check_timeouts();
  fail_unless(fired[0] == 0);

  lwip_sys_now += LWIP_UINT32_MAX / 8;

  sys_check_timeouts();
  fail_unless(fired[0] == 0);

  lwip_sys_now += LWIP_UINT32_MAX / 8;

  sys_check_timeouts();
  fail_unless(fired[0] == 0);

  lwip_sys_now += 1;

  sys_check_timeouts();
  fail_unless(fired[0] == 1);

  sys_untimeout(dummy_handler, LWIP_PTR_NUMERIC_CAST(void*, 0));
}
END_TEST

/** Create the suite including all tests for this module */
Suite *
timers_suite(void)
{
  testfunc tests[] = {
    TESTFUNC(test_bug52748),
    TESTFUNC(test_cyclic_timers),
    TESTFUNC(test_timers),
    TESTFUNC(test_long_timer),
  };
  return create_suite("TIMERS", tests, LWIP_ARRAYSIZE(tests), timers_setup, timers_teardown);
}
