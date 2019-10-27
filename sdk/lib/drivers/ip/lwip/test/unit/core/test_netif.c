#include "test_netif.h"

#include "lwip/netif.h"
#include "lwip/stats.h"
#include "lwip/etharp.h"
#include "netif/ethernet.h"

#if !LWIP_NETIF_EXT_STATUS_CALLBACK
#error "This tests needs LWIP_NETIF_EXT_STATUS_CALLBACK enabled"
#endif

struct netif net_test;


/* Setups/teardown functions */

static void
netif_setup(void)
{
  lwip_check_ensure_no_alloc(SKIP_POOL(MEMP_SYS_TIMEOUT));
}

static void
netif_teardown(void)
{
  lwip_check_ensure_no_alloc(SKIP_POOL(MEMP_SYS_TIMEOUT));
}

/* test helper functions */

static err_t
testif_tx_func(struct netif *netif, struct pbuf *p)
{
  LWIP_UNUSED_ARG(netif);
  LWIP_UNUSED_ARG(p);
  return ERR_OK;
}

static err_t
testif_init(struct netif *netif)
{
  netif->name[0] = 'c';
  netif->name[1] = 'h';
  netif->output = etharp_output;
  netif->linkoutput = testif_tx_func;
  netif->mtu = 1500;
  netif->hwaddr_len = 6;
  netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET | NETIF_FLAG_IGMP | NETIF_FLAG_MLD6;

  netif->hwaddr[0] = 0x02;
  netif->hwaddr[1] = 0x03;
  netif->hwaddr[2] = 0x04;
  netif->hwaddr[3] = 0x05;
  netif->hwaddr[4] = 0x06;
  netif->hwaddr[5] = 0x07;

  return ERR_OK;
}

#define MAX_NSC_REASON_IDX 10
static netif_nsc_reason_t expected_reasons;
static int callback_ctr;

static int dummy_active;

static void
test_netif_ext_callback_dummy(struct netif* netif, netif_nsc_reason_t reason, const netif_ext_callback_args_t* args)
{
  LWIP_UNUSED_ARG(netif);
  LWIP_UNUSED_ARG(reason);
  LWIP_UNUSED_ARG(args);

  fail_unless(dummy_active);
}

static void
test_netif_ext_callback(struct netif* netif, netif_nsc_reason_t reason, const netif_ext_callback_args_t* args)
{
  LWIP_UNUSED_ARG(args); /* @todo */
  callback_ctr++;

  fail_unless(netif == &net_test);

  fail_unless(expected_reasons == reason);
}

/* Test functions */

NETIF_DECLARE_EXT_CALLBACK(netif_callback_1)
NETIF_DECLARE_EXT_CALLBACK(netif_callback_2)
NETIF_DECLARE_EXT_CALLBACK(netif_callback_3)

START_TEST(test_netif_extcallbacks)
{
  ip4_addr_t addr;
  ip4_addr_t netmask;
  ip4_addr_t gw;
  LWIP_UNUSED_ARG(_i);

  IP4_ADDR(&addr, 0, 0, 0, 0);
  IP4_ADDR(&netmask, 0, 0, 0, 0);
  IP4_ADDR(&gw, 0, 0, 0, 0);

  netif_add_ext_callback(&netif_callback_3, test_netif_ext_callback_dummy);
  netif_add_ext_callback(&netif_callback_2, test_netif_ext_callback);
  netif_add_ext_callback(&netif_callback_1, test_netif_ext_callback_dummy);

  dummy_active = 1;

  /* positive tests: check that single events come as expected */

  expected_reasons = LWIP_NSC_NETIF_ADDED;
  callback_ctr = 0;
  netif_add(&net_test, &addr, &netmask, &gw, &net_test, testif_init, ethernet_input);
  fail_unless(callback_ctr == 1);

  expected_reasons = LWIP_NSC_LINK_CHANGED;
  callback_ctr = 0;
  netif_set_link_up(&net_test);
  fail_unless(callback_ctr == 1);

  expected_reasons = LWIP_NSC_STATUS_CHANGED;
  callback_ctr = 0;
  netif_set_up(&net_test);
  fail_unless(callback_ctr == 1);

  IP4_ADDR(&addr, 1, 2, 3, 4);
  expected_reasons = LWIP_NSC_IPV4_ADDRESS_CHANGED;
  callback_ctr = 0;
  netif_set_ipaddr(&net_test, &addr);
  fail_unless(callback_ctr == 1);

  IP4_ADDR(&netmask, 255, 255, 255, 0);
  expected_reasons = LWIP_NSC_IPV4_NETMASK_CHANGED;
  callback_ctr = 0;
  netif_set_netmask(&net_test, &netmask);
  fail_unless(callback_ctr == 1);

  IP4_ADDR(&gw, 1, 2, 3, 254);
  expected_reasons = LWIP_NSC_IPV4_GATEWAY_CHANGED;
  callback_ctr = 0;
  netif_set_gw(&net_test, &gw);
  fail_unless(callback_ctr == 1);

  IP4_ADDR(&addr, 0, 0, 0, 0);
  expected_reasons = LWIP_NSC_IPV4_ADDRESS_CHANGED;
  callback_ctr = 0;
  netif_set_ipaddr(&net_test, &addr);
  fail_unless(callback_ctr == 1);

  IP4_ADDR(&netmask, 0, 0, 0, 0);
  expected_reasons = LWIP_NSC_IPV4_NETMASK_CHANGED;
  callback_ctr = 0;
  netif_set_netmask(&net_test, &netmask);
  fail_unless(callback_ctr == 1);

  IP4_ADDR(&gw, 0, 0, 0, 0);
  expected_reasons = LWIP_NSC_IPV4_GATEWAY_CHANGED;
  callback_ctr = 0;
  netif_set_gw(&net_test, &gw);
  fail_unless(callback_ctr == 1);

  /* check for multi-events (only one combined callback expected) */

  IP4_ADDR(&addr, 1, 2, 3, 4);
  IP4_ADDR(&netmask, 255, 255, 255, 0);
  IP4_ADDR(&gw, 1, 2, 3, 254);
  expected_reasons = (netif_nsc_reason_t)(LWIP_NSC_IPV4_ADDRESS_CHANGED | LWIP_NSC_IPV4_NETMASK_CHANGED |
                                          LWIP_NSC_IPV4_GATEWAY_CHANGED | LWIP_NSC_IPV4_SETTINGS_CHANGED);
  callback_ctr = 0;
  netif_set_addr(&net_test, &addr, &netmask, &gw);
  fail_unless(callback_ctr == 1);

  /* check that for no-change, no callback is expected */
  expected_reasons = LWIP_NSC_NONE;
  callback_ctr = 0;
  netif_set_ipaddr(&net_test, &addr);
  fail_unless(callback_ctr == 0);

  netif_set_netmask(&net_test, &netmask);
  callback_ctr = 0;
  fail_unless(callback_ctr == 0);

  callback_ctr = 0;
  netif_set_gw(&net_test, &gw);
  fail_unless(callback_ctr == 0);

  callback_ctr = 0;
  netif_set_addr(&net_test, &addr, &netmask, &gw);
  fail_unless(callback_ctr == 0);

  /* check for single-events */
  IP4_ADDR(&addr, 1, 2, 3, 5);
  expected_reasons = (netif_nsc_reason_t)(LWIP_NSC_IPV4_ADDRESS_CHANGED | LWIP_NSC_IPV4_SETTINGS_CHANGED);
  callback_ctr = 0;
  netif_set_addr(&net_test, &addr, &netmask, &gw);
  fail_unless(callback_ctr == 1);

  expected_reasons = LWIP_NSC_STATUS_CHANGED;
  callback_ctr = 0;
  netif_set_down(&net_test);
  fail_unless(callback_ctr == 1);

  expected_reasons = LWIP_NSC_NETIF_REMOVED;
  callback_ctr = 0;
  netif_remove(&net_test);
  fail_unless(callback_ctr == 1);

  expected_reasons = LWIP_NSC_NONE;

  netif_remove_ext_callback(&netif_callback_2);
  netif_remove_ext_callback(&netif_callback_3);
  netif_remove_ext_callback(&netif_callback_1);
  dummy_active = 0;
}
END_TEST


/** Create the suite including all tests for this module */
Suite *
netif_suite(void)
{
  testfunc tests[] = {
    TESTFUNC(test_netif_extcallbacks)
  };
  return create_suite("NETIF", tests, sizeof(tests)/sizeof(testfunc), netif_setup, netif_teardown);
}
