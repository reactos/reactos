#include "test_ip6.h"

#include "lwip/ethip6.h"
#include "lwip/ip6.h"
#include "lwip/inet_chksum.h"
#include "lwip/nd6.h"
#include "lwip/stats.h"
#include "lwip/prot/ethernet.h"
#include "lwip/prot/ip.h"
#include "lwip/prot/ip6.h"

#include "lwip/tcpip.h"

#if LWIP_IPV6 /* allow to build the unit tests without IPv6 support */

static struct netif test_netif6;
static int linkoutput_ctr;

static err_t
default_netif_linkoutput(struct netif *netif, struct pbuf *p)
{
  fail_unless(netif == &test_netif6);
  fail_unless(p != NULL);
  linkoutput_ctr++;
  return ERR_OK;
}

static err_t
default_netif_init(struct netif *netif)
{
  fail_unless(netif != NULL);
  netif->linkoutput = default_netif_linkoutput;
  netif->output_ip6 = ethip6_output;
  netif->mtu = 1500;
  netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHERNET | NETIF_FLAG_MLD6;
  netif->hwaddr_len = ETH_HWADDR_LEN;
  return ERR_OK;
}

static void
default_netif_add(void)
{
  struct netif *n;
  fail_unless(netif_default == NULL);
  n = netif_add_noaddr(&test_netif6, NULL, default_netif_init, NULL);
  fail_unless(n == &test_netif6);
  netif_set_default(&test_netif6);
}

static void
default_netif_remove(void)
{
  fail_unless(netif_default == &test_netif6);
  netif_remove(&test_netif6);
}

static void
ip6_test_handle_timers(int count)
{
  int i;
  for (i = 0; i < count; i++) {
    nd6_tmr();
  }
}

/* Setups/teardown functions */

static void
ip6_setup(void)
{
  default_netif_add();
  lwip_check_ensure_no_alloc(SKIP_POOL(MEMP_SYS_TIMEOUT));
}

static void
ip6_teardown(void)
{
  if (netif_list->loop_first != NULL) {
    pbuf_free(netif_list->loop_first);
    netif_list->loop_first = NULL;
  }
  netif_list->loop_last = NULL;
  /* poll until all memory is released... */
  tcpip_thread_poll_one();
  default_netif_remove();
  lwip_check_ensure_no_alloc(SKIP_POOL(MEMP_SYS_TIMEOUT));
}


/* Test functions */

static void
test_ip6_ll_addr_iter(int expected_ctr1, int expected_ctr2)
{
  fail_unless(linkoutput_ctr == 0);

  /* test that nothing is sent with link uo but netif down */
  netif_set_link_up(&test_netif6);
  ip6_test_handle_timers(500);
  fail_unless(linkoutput_ctr == 0);
  netif_set_link_down(&test_netif6);
  fail_unless(linkoutput_ctr == 0);

  /* test that nothing is sent with link down but netif up */
  netif_set_up(&test_netif6);
  ip6_test_handle_timers(500);
  fail_unless(linkoutput_ctr == 0);
  netif_set_down(&test_netif6);
  fail_unless(linkoutput_ctr == 0);

  /* test what is sent with link up + netif up */
  netif_set_link_up(&test_netif6);
  netif_set_up(&test_netif6);
  ip6_test_handle_timers(500);
  fail_unless(linkoutput_ctr == expected_ctr1);
  netif_set_down(&test_netif6);
  netif_set_link_down(&test_netif6);
  fail_unless(linkoutput_ctr == expected_ctr1);
  linkoutput_ctr = 0;

  netif_set_up(&test_netif6);
  netif_set_link_up(&test_netif6);
  ip6_test_handle_timers(500);
  fail_unless(linkoutput_ctr == expected_ctr2);
  netif_set_link_down(&test_netif6);
  netif_set_down(&test_netif6);
  fail_unless(linkoutput_ctr == expected_ctr2);
  linkoutput_ctr = 0;
}

START_TEST(test_ip6_ll_addr)
{
  LWIP_UNUSED_ARG(_i);

  /* test without link-local address */
  test_ip6_ll_addr_iter(0, 0);

  /* test with link-local address */
  netif_create_ip6_linklocal_address(&test_netif6, 1);
  test_ip6_ll_addr_iter(3 + LWIP_IPV6_DUP_DETECT_ATTEMPTS + LWIP_IPV6_MLD, 3);
}
END_TEST

START_TEST(test_ip6_aton_ipv4mapped)
{
  int ret;
  ip_addr_t addr;
  ip6_addr_t addr6;
  const ip_addr_t addr_expected = IPADDR6_INIT_HOST(0, 0, 0xFFFF, 0xD4CC65D2);
  const char *full_ipv6_addr = "0:0:0:0:0:FFFF:D4CC:65D2";
  const char *shortened_ipv6_addr = "::FFFF:D4CC:65D2";
  const char *full_ipv4_mapped_addr = "0:0:0:0:0:FFFF:212.204.101.210";
  const char *shortened_ipv4_mapped_addr = "::FFFF:212.204.101.210";
  const char *bogus_ipv4_mapped_addr = "::FFFF:212.204.101.2101";
  LWIP_UNUSED_ARG(_i);

  /* check IPv6 representation */
  memset(&addr6, 0, sizeof(addr6));
  ret = ip6addr_aton(full_ipv6_addr, &addr6);
  fail_unless(ret == 1);
  fail_unless(memcmp(&addr6, &addr_expected, 16) == 0);
  memset(&addr, 0, sizeof(addr));
  ret = ipaddr_aton(full_ipv6_addr, &addr);
  fail_unless(ret == 1);
  fail_unless(memcmp(&addr, &addr_expected, 16) == 0);

  /* check shortened IPv6 representation */
  memset(&addr6, 0, sizeof(addr6));
  ret = ip6addr_aton(shortened_ipv6_addr, &addr6);
  fail_unless(ret == 1);
  fail_unless(memcmp(&addr6, &addr_expected, 16) == 0);
  memset(&addr, 0, sizeof(addr));
  ret = ipaddr_aton(shortened_ipv6_addr, &addr);
  fail_unless(ret == 1);
  fail_unless(memcmp(&addr, &addr_expected, 16) == 0);

  /* checked shortened mixed representation */
  memset(&addr6, 0, sizeof(addr6));
  ret = ip6addr_aton(shortened_ipv4_mapped_addr, &addr6);
  fail_unless(ret == 1);
  fail_unless(memcmp(&addr6, &addr_expected, 16) == 0);
  memset(&addr, 0, sizeof(addr));
  ret = ipaddr_aton(shortened_ipv4_mapped_addr, &addr);
  fail_unless(ret == 1);
  fail_unless(memcmp(&addr, &addr_expected, 16) == 0);

  /* checked mixed representation */
  memset(&addr6, 0, sizeof(addr6));
  ret = ip6addr_aton(full_ipv4_mapped_addr, &addr6);
  fail_unless(ret == 1);
  fail_unless(memcmp(&addr6, &addr_expected, 16) == 0);
  memset(&addr, 0, sizeof(addr));
  ret = ipaddr_aton(full_ipv4_mapped_addr, &addr);
  fail_unless(ret == 1);
  fail_unless(memcmp(&addr, &addr_expected, 16) == 0);

  /* checked bogus mixed representation */
  memset(&addr6, 0, sizeof(addr6));
  ret = ip6addr_aton(bogus_ipv4_mapped_addr, &addr6);
  fail_unless(ret == 0);
  memset(&addr, 0, sizeof(addr));
  ret = ipaddr_aton(bogus_ipv4_mapped_addr, &addr);
  fail_unless(ret == 0);
}
END_TEST

START_TEST(test_ip6_ntoa_ipv4mapped)
{
  const ip_addr_t addr = IPADDR6_INIT_HOST(0, 0, 0xFFFF, 0xD4CC65D2);
  char buf[128];
  char *str;
  LWIP_UNUSED_ARG(_i);

  str = ip6addr_ntoa_r(ip_2_ip6(&addr), buf, sizeof(buf));
  fail_unless(str == buf);
  fail_unless(!strcmp(str, "::FFFF:212.204.101.210"));
}
END_TEST

struct test_addr_and_str {
  ip_addr_t addr;
  const char *str;
};

START_TEST(test_ip6_ntoa)
{
  struct test_addr_and_str tests[] = {
    {IPADDR6_INIT_HOST(0xfe800000, 0x00000000, 0xb2a1a2ff, 0xfea3a4a5), "FE80::B2A1:A2FF:FEA3:A4A5"}, /* test shortened zeros */
    {IPADDR6_INIT_HOST(0xfe800000, 0xff000000, 0xb2a1a2ff, 0xfea3a4a5), "FE80:0:FF00:0:B2A1:A2FF:FEA3:A4A5"}, /* don't omit single zero blocks */
    {IPADDR6_INIT_HOST(0xfe800000, 0xff000000, 0xb2000000, 0x0000a4a5), "FE80:0:FF00:0:B200::A4A5"}, /* omit longest zero block */
  };
  char buf[128];
  char *str;
  size_t i;
  LWIP_UNUSED_ARG(_i);

  for (i = 0; i < LWIP_ARRAYSIZE(tests); i++) {
    str = ip6addr_ntoa_r(ip_2_ip6(&tests[i].addr), buf, sizeof(buf));
    fail_unless(str == buf);
    fail_unless(!strcmp(str, tests[i].str));
  }
}
END_TEST

START_TEST(test_ip6_lladdr)
{
  u8_t zeros[128];
  const u8_t test_mac_addr[6] = {0xb0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5};
  const u32_t expected_ip6_addr_1[4] = {PP_HTONL(0xfe800000), 0, PP_HTONL(0xb2a1a2ff), PP_HTONL(0xfea3a4a5)};
  const u32_t expected_ip6_addr_2[4] = {PP_HTONL(0xfe800000), 0, PP_HTONL(0x0000b0a1), PP_HTONL(0xa2a3a4a5)};
  LWIP_UNUSED_ARG(_i);
  memset(zeros, 0, sizeof(zeros));

  fail_unless(test_netif6.hwaddr_len == 6);
  fail_unless(!memcmp(test_netif6.hwaddr, zeros, 6));

  fail_unless(test_netif6.ip6_addr_state[0] == 0);
  fail_unless(!memcmp(netif_ip6_addr(&test_netif6, 0), zeros, sizeof(ip6_addr_t)));

  /* set specific mac addr */
  memcpy(test_netif6.hwaddr, test_mac_addr, 6);

  /* create link-local addr based on mac (EUI-48) */
  netif_create_ip6_linklocal_address(&test_netif6, 1);
  fail_unless(IP_IS_V6(&test_netif6.ip6_addr[0]));
  fail_unless(!memcmp(&netif_ip6_addr(&test_netif6, 0)->addr, expected_ip6_addr_1, 16));
#if LWIP_IPV6_SCOPES
  fail_unless(netif_ip6_addr(&test_netif6, 0)->zone == (test_netif6.num + 1));
#endif
  /* reset address */
  memset(&test_netif6.ip6_addr[0], 0, sizeof(ip6_addr_t));
  test_netif6.ip6_addr_state[0] = 0;

  /* create link-local addr based interface ID */
  netif_create_ip6_linklocal_address(&test_netif6, 0);
  fail_unless(IP_IS_V6(&test_netif6.ip6_addr[0]));
  fail_unless(!memcmp(&netif_ip6_addr(&test_netif6, 0)->addr, expected_ip6_addr_2, 16));
#if LWIP_IPV6_SCOPES
  fail_unless(netif_ip6_addr(&test_netif6, 0)->zone == (test_netif6.num + 1));
#endif
  /* reset address */
  memset(&test_netif6.ip6_addr[0], 0, sizeof(ip6_addr_t));
  test_netif6.ip6_addr_state[0] = 0;

  /* reset mac address */
  memset(&test_netif6.hwaddr, 0, sizeof(test_netif6.hwaddr));
}
END_TEST

/** Create the suite including all tests for this module */
Suite *
ip6_suite(void)
{
  testfunc tests[] = {
    TESTFUNC(test_ip6_ll_addr),
    TESTFUNC(test_ip6_aton_ipv4mapped),
    TESTFUNC(test_ip6_ntoa_ipv4mapped),
    TESTFUNC(test_ip6_ntoa),
    TESTFUNC(test_ip6_lladdr)
  };
  return create_suite("IPv6", tests, sizeof(tests)/sizeof(testfunc), ip6_setup, ip6_teardown);
}

#else /* LWIP_IPV6 */

/* allow to build the unit tests without IPv6 support */
START_TEST(test_ip6_dummy)
{
  LWIP_UNUSED_ARG(_i);
}
END_TEST

Suite *
ip6_suite(void)
{
  testfunc tests[] = {
    TESTFUNC(test_ip6_dummy),
  };
  return create_suite("IPv6", tests, sizeof(tests)/sizeof(testfunc), NULL, NULL);
}
#endif /* LWIP_IPV6 */
