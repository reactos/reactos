#include "test_dns.h"

#include "lwip/dns.h"

/* Setups/teardown functions */

static void
dns_setup(void)
{
}

static void
dns_teardown(void)
{
}

/* Test functions */

START_TEST(test_dns_set_get_server)
{
  int n;
  LWIP_UNUSED_ARG(_i);

  for (n = 0; n < 256; n++) {
    u8_t i = (u8_t)n;
    ip_addr_t server;
    /* Should return a zeroed address for any index */
    fail_unless(dns_getserver(i));
    fail_unless(ip_addr_isany(dns_getserver(i)));

    /* Should accept setting address for any index, and ignore if out of range */
    IP_ADDR4(&server, 10, 0, 0, i);
    dns_setserver(i, &server);
    fail_unless(dns_getserver(i));
    if (i < DNS_MAX_SERVERS) {
      fail_unless(ip_addr_eq(dns_getserver(i), &server) == 1);
    } else {
      fail_unless(ip_addr_isany(dns_getserver(i)));
    }
  }
}
END_TEST

/** Create the suite including all tests for this module */
Suite *
dns_suite(void)
{
  testfunc tests[] = {
    TESTFUNC(test_dns_set_get_server)
  };
  return create_suite("DNS", tests, sizeof(tests)/sizeof(testfunc), dns_setup, dns_teardown);
}
