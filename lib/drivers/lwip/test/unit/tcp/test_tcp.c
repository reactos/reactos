#include "test_tcp.h"

#include "lwip/tcp.h"
#include "lwip/stats.h"
#include "tcp_helper.h"

#if !LWIP_STATS || !TCP_STATS || !MEMP_STATS
#error "This tests needs TCP- and MEMP-statistics enabled"
#endif

/* Setups/teardown functions */

static void
tcp_setup(void)
{
  tcp_remove_all();
}

static void
tcp_teardown(void)
{
  tcp_remove_all();
}


/* Test functions */

/** Call tcp_new() and tcp_abort() and test memp stats */
START_TEST(test_tcp_new_abort)
{
  struct tcp_pcb* pcb;
  LWIP_UNUSED_ARG(_i);

  fail_unless(lwip_stats.memp[MEMP_TCP_PCB].used == 0);

  pcb = tcp_new();
  fail_unless(pcb != NULL);
  if (pcb != NULL) {
    fail_unless(lwip_stats.memp[MEMP_TCP_PCB].used == 1);
    tcp_abort(pcb);
    fail_unless(lwip_stats.memp[MEMP_TCP_PCB].used == 0);
  }
}
END_TEST

/** Create an ESTABLISHED pcb and check if receive callback is called */
START_TEST(test_tcp_recv_inseq)
{
  struct test_tcp_counters counters;
  struct tcp_pcb* pcb;
  struct pbuf* p;
  char data[] = {1, 2, 3, 4};
  struct ip_addr remote_ip, local_ip;
  u16_t data_len;
  u16_t remote_port = 0x100, local_port = 0x101;
  struct netif netif;
  LWIP_UNUSED_ARG(_i);

  /* initialize local vars */
  memset(&netif, 0, sizeof(netif));
  IP4_ADDR(&local_ip, 192, 168, 1, 1);
  IP4_ADDR(&remote_ip, 192, 168, 1, 2);
  data_len = sizeof(data);
  /* initialize counter struct */
  memset(&counters, 0, sizeof(counters));
  counters.expected_data_len = data_len;
  counters.expected_data = data;

  /* create and initialize the pcb */
  pcb = test_tcp_new_counters_pcb(&counters);
  EXPECT_RET(pcb != NULL);
  tcp_set_state(pcb, ESTABLISHED, &local_ip, &remote_ip, local_port, remote_port);

  /* create a segment */
  p = tcp_create_rx_segment(pcb, counters.expected_data, data_len, 0, 0, 0);
  EXPECT(p != NULL);
  if (p != NULL) {
    /* pass the segment to tcp_input */
    tcp_input(p, &netif);
    /* check if counters are as expected */
    EXPECT(counters.close_calls == 0);
    EXPECT(counters.recv_calls == 1);
    EXPECT(counters.recved_bytes == data_len);
    EXPECT(counters.err_calls == 0);
  }

  /* make sure the pcb is freed */
  EXPECT(lwip_stats.memp[MEMP_TCP_PCB].used == 1);
  tcp_abort(pcb);
  EXPECT(lwip_stats.memp[MEMP_TCP_PCB].used == 0);
}
END_TEST


/** Create the suite including all tests for this module */
Suite *
tcp_suite(void)
{
  TFun tests[] = {
    test_tcp_new_abort,
    test_tcp_recv_inseq,
  };
  return create_suite("TCP", tests, sizeof(tests)/sizeof(TFun), tcp_setup, tcp_teardown);
}
