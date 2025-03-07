#include "test_tcp_state.h"

#include "lwip/priv/tcp_priv.h"
#include "lwip/stats.h"
#include "tcp_helper.h"
#include "lwip/inet_chksum.h"

#ifdef _MSC_VER
#pragma warning(disable: 4307) /* we explicitly wrap around TCP seqnos */
#endif

#if !LWIP_STATS || !TCP_STATS || !MEMP_STATS
#error "This tests needs TCP- and MEMP-statistics enabled"
#endif

static struct netif test_netif = {0};
static struct test_tcp_txcounters test_txcounters = {0};

#define SEQNO1 (0xFFFFFF00 - TCP_MSS)
#define ISS    6510
static u8_t test_tcp_timer;

/* our own version of tcp_tmr so we can reset fast/slow timer state */
static void
test_tcp_tmr(void)
{
  tcp_fasttmr();
  if (++test_tcp_timer & 1) {
    tcp_slowtmr();
  }
}

/* Get TCP flags from packets */
static u8_t 
get_tcp_flags_from_packet(struct pbuf *p, u16_t tcp_hdr_offset)
{
  struct tcp_hdr tcphdr;
  u16_t ret;
  EXPECT_RETX(p != NULL, 0);
  EXPECT_RETX(p->len >= tcp_hdr_offset + sizeof(struct tcp_hdr), 0);
  ret = pbuf_copy_partial(p, &tcphdr, sizeof(struct tcp_hdr), tcp_hdr_offset);
  EXPECT(ret == sizeof(struct tcp_hdr));
  return TCPH_FLAGS(&tcphdr);
}

/* Create listening tcp_pcb */
static struct tcp_pcb_listen *
create_listening_pcb(u16_t local_port, struct test_tcp_counters *counters)
{
  struct tcp_pcb *pcb;
  struct tcp_pcb_listen *lpcb=NULL;
  err_t err;
  u16_t port = local_port?local_port:1234;

  if (counters) {
    pcb = test_tcp_new_counters_pcb(counters);
  } else {
    pcb = tcp_new();
  }
  EXPECT(pcb != NULL);

  if (pcb) {
    err = tcp_bind(pcb, &test_netif.ip_addr, port);
    EXPECT(err == ERR_OK);
    lpcb = (struct tcp_pcb_listen *)tcp_listen(pcb);
  }

  return lpcb;
}

/* Setup/teardown functions */
static struct netif* old_netif_list;
static struct netif* old_netif_default;

static void
tcp_state_setup(void)
{
  struct tcp_pcb dummy_pcb; /* we need this for tcp_next_iss() only */

  /* reset iss to default (6510) */
  tcp_ticks = 0;
  tcp_ticks = 0 - (tcp_next_iss(&dummy_pcb) - 6510);
  tcp_next_iss(&dummy_pcb);
  tcp_ticks = 0;

  test_tcp_timer = 0;

  old_netif_list = netif_list;
  old_netif_default = netif_default;
  netif_list = NULL;
  netif_default = NULL;
  tcp_remove_all();
  test_tcp_init_netif(&test_netif, &test_txcounters, &test_local_ip, &test_netmask);
  lwip_check_ensure_no_alloc(SKIP_POOL(MEMP_SYS_TIMEOUT));
}

static void
tcp_state_teardown(void)
{
  netif_list = NULL;
  netif_default = NULL;
  tcp_remove_all();
  /* restore netif_list for next tests (e.g. loopif) */
  netif_list = old_netif_list;
  netif_default = old_netif_default;
  lwip_check_ensure_no_alloc(SKIP_POOL(MEMP_SYS_TIMEOUT));
}

/* helper functions */

static void 
test_rst_generation_with_incoming_packet(struct pbuf *p,
  struct netif *netif, struct test_tcp_txcounters *tx_counters)
{
  u16_t tcp_flags;
  EXPECT_RET(p != NULL);
  memset(tx_counters, 0, sizeof(struct test_tcp_txcounters));
  /* pass the segment to tcp_input */
  tx_counters->copy_tx_packets = 1;
  test_tcp_input(p, netif);
  tx_counters->copy_tx_packets = 0;
  /* check if packets are as expected */
  EXPECT(tx_counters->tx_packets != NULL);
  if (tx_counters->tx_packets) {
    tcp_flags = get_tcp_flags_from_packet(tx_counters->tx_packets, 20);
    EXPECT(tcp_flags & TCP_RST);
    pbuf_free(tx_counters->tx_packets);
    tx_counters->tx_packets = NULL;
  }
}

/* Test functions */

/* Call tcp_new() and test memp stats (max number) */
START_TEST(test_tcp_new_max_num)
{
  struct tcp_pcb* pcb[MEMP_NUM_TCP_PCB + 1];
  int i;
  LWIP_UNUSED_ARG(_i);

  fail_unless(MEMP_STATS_GET(used, MEMP_TCP_PCB) == 0);

  for(i = 0;i < MEMP_NUM_TCP_PCB; i++) {
    pcb[i] = tcp_new();
    fail_unless(MEMP_STATS_GET(used, MEMP_TCP_PCB) == (i + 1));
  }
  fail_unless(MEMP_STATS_GET(used, MEMP_TCP_PCB) == MEMP_NUM_TCP_PCB);
  /* Trying to remove the oldest pcb in TIME_WAIT,LAST_ACK,CLOSING state when pcb full */
  pcb[MEMP_NUM_TCP_PCB] = tcp_new();
  fail_unless(pcb[MEMP_NUM_TCP_PCB] == NULL);
  tcp_set_state(pcb[0], TIME_WAIT, &test_local_ip, &test_remote_ip, TEST_LOCAL_PORT, TEST_REMOTE_PORT);
  pcb[MEMP_NUM_TCP_PCB] = tcp_new();
  fail_unless(pcb[MEMP_NUM_TCP_PCB] != NULL);
  fail_unless(MEMP_STATS_GET(used, MEMP_TCP_PCB) == MEMP_NUM_TCP_PCB);

  for (i = 1; i <= MEMP_NUM_TCP_PCB; i++)
  {
    tcp_abort(pcb[i]);
  }
  fail_unless(MEMP_STATS_GET(used, MEMP_TCP_PCB) == 0);
}
END_TEST


/* pcbs in TIME_WAIT state will be deleted when creating new pcb reach the max number */
START_TEST(test_tcp_new_max_num_remove_TIME_WAIT)
{
  struct tcp_pcb* pcb;
  struct tcp_pcb* pcb_list[MEMP_NUM_TCP_PCB + 1];
  int i;
  LWIP_UNUSED_ARG(_i);

  /* create a pcb in TIME_WAIT state */
  pcb = tcp_new();
  EXPECT_RET(pcb != NULL);
  tcp_set_state(pcb, TIME_WAIT, &test_local_ip, &test_remote_ip, TEST_LOCAL_PORT, TEST_REMOTE_PORT);
  EXPECT_RET(MEMP_STATS_GET(used, MEMP_TCP_PCB) == 1);
  EXPECT_RET(pcb->state == TIME_WAIT);

  /* Create max number pcbs */
  for(i = 0;i < MEMP_NUM_TCP_PCB-1; i++) {
    pcb_list[i] = tcp_new();
    EXPECT(MEMP_STATS_GET(used, MEMP_TCP_PCB) == (i + 2));
  }
  EXPECT(MEMP_STATS_GET(used, MEMP_TCP_PCB) == MEMP_NUM_TCP_PCB);

  /* Create one more pcb, and expect that the pcb in the TIME_WAIT state is deleted */
  pcb_list[MEMP_NUM_TCP_PCB-1] = tcp_new();
  EXPECT_RET(pcb_list[MEMP_NUM_TCP_PCB-1] != NULL);
  EXPECT_RET(MEMP_STATS_GET(used, MEMP_TCP_PCB) == MEMP_NUM_TCP_PCB);

  for (i = 0; i <= MEMP_NUM_TCP_PCB-1; i++)
  {
    tcp_abort(pcb_list[i]);
  }
  EXPECT(MEMP_STATS_GET(used, MEMP_TCP_PCB) == 0);

}
END_TEST


/* Call tcp_connect to check active open */
START_TEST(test_tcp_connect_active_open)
{
  struct test_tcp_counters counters;
  struct tcp_pcb *pcb;
  struct pbuf *p;
  err_t err;
  u16_t test_port = 1234;
  u32_t seqno = 0;
  LWIP_UNUSED_ARG(_i);

  /* create and initialize the pcb */
  tcp_ticks = SEQNO1 - ISS;
  pcb = test_tcp_new_counters_pcb(&counters);
  EXPECT_RET(pcb != NULL);
  
  /* Get seqno from SYN packet */
  test_txcounters.copy_tx_packets = 1;
  err = tcp_connect(pcb, &test_remote_ip, test_port, NULL);
  test_txcounters.copy_tx_packets = 0;
  EXPECT(err == ERR_OK);
  EXPECT(pcb->state == SYN_SENT);
  EXPECT(test_txcounters.num_tx_calls == 1);
  EXPECT_RET(test_txcounters.tx_packets != NULL);
  if (test_txcounters.tx_packets != NULL) {
    struct tcp_hdr tcphdr;
    u16_t ret;
    ret = pbuf_copy_partial(test_txcounters.tx_packets, &tcphdr, 20, 20);
    EXPECT(ret == 20);
    EXPECT(TCPH_FLAGS(&tcphdr) & TCP_SYN);
    pbuf_free(test_txcounters.tx_packets);
    test_txcounters.tx_packets = NULL;
    seqno = lwip_htonl(tcphdr.seqno);
    EXPECT(seqno == pcb->lastack);
  }

  /* check correct syn packet */
  p = tcp_create_segment(&pcb->remote_ip, &pcb->local_ip, test_port,
    pcb->local_port, NULL, 0, 12345, seqno + 1, TCP_SYN|TCP_ACK);
  EXPECT_RET(p != NULL);
  test_tcp_input(p, &test_netif);
  EXPECT_RET(pcb->state == ESTABLISHED);
  EXPECT_RET(test_txcounters.num_tx_calls == 2);

  /* make sure the pcb is freed */
  EXPECT(MEMP_STATS_GET(used, MEMP_TCP_PCB) == 1);
  tcp_abort(pcb);
  EXPECT(MEMP_STATS_GET(used, MEMP_TCP_PCB) == 0);
}
END_TEST

START_TEST(test_tcp_active_close)
{
  struct tcp_pcb *pcb, *pcbl;
  struct test_tcp_counters counters;
  struct pbuf *p;
  err_t err;
  u32_t i;
  LWIP_UNUSED_ARG(_i);

  /* create TCP in LISTEN state */
  memset(&counters, 0, sizeof(counters));
  pcb = test_tcp_new_counters_pcb(&counters);
  EXPECT_RET(pcb != NULL);
  EXPECT_RET(MEMP_STATS_GET(used, MEMP_TCP_PCB) == 1);
  err = tcp_bind(pcb, &test_netif.ip_addr, 1234);
  EXPECT_RET(err == ERR_OK);
  pcbl = tcp_listen(pcb);
  EXPECT_RET(pcbl != NULL);
  EXPECT_RET(pcbl->state == LISTEN);
  EXPECT_RET(MEMP_STATS_GET(used, MEMP_TCP_PCB) == 0);
  EXPECT_RET(MEMP_STATS_GET(used, MEMP_TCP_PCB_LISTEN) == 1);

  memset(&test_txcounters, 0, sizeof(test_txcounters));
  err = tcp_close(pcbl);
  EXPECT_RET(err == ERR_OK);
  EXPECT_RET(MEMP_STATS_GET(used, MEMP_TCP_PCB) == 0);
  EXPECT_RET(MEMP_STATS_GET(used, MEMP_TCP_PCB_LISTEN) == 0);
  EXPECT(test_txcounters.num_tx_calls == 0);

  /* close TCP in SYN_SENT state */
  memset(&counters, 0, sizeof(counters));
  pcb = test_tcp_new_counters_pcb(&counters);
  EXPECT_RET(pcb != NULL);
  err = tcp_connect(pcb, &test_netif.gw, 1234, NULL);
  EXPECT_RET(err == ERR_OK);
  EXPECT_RET(pcb->state == SYN_SENT);
  EXPECT(test_txcounters.num_tx_calls == 1);
  EXPECT_RET(MEMP_STATS_GET(used, MEMP_TCP_PCB) == 1);

  memset(&test_txcounters, 0, sizeof(test_txcounters));
  err = tcp_close(pcb);
  EXPECT_RET(err == ERR_OK);
  EXPECT_RET(MEMP_STATS_GET(used, MEMP_TCP_PCB) == 0);
  EXPECT(test_txcounters.num_tx_calls == 0);

  /* close TCP in ESTABLISHED state */
  memset(&counters, 0, sizeof(counters));
  pcb = test_tcp_new_counters_pcb(&counters);
  EXPECT_RET(pcb != NULL);
  tcp_set_state(pcb, ESTABLISHED, &test_local_ip, &test_remote_ip, TEST_LOCAL_PORT, TEST_REMOTE_PORT);
  EXPECT_RET(MEMP_STATS_GET(used, MEMP_TCP_PCB) == 1);

  memset(&test_txcounters, 0, sizeof(test_txcounters));
  err = tcp_close(pcb);
  EXPECT_RET(err == ERR_OK);
  EXPECT_RET(pcb->state == FIN_WAIT_1);
  EXPECT_RET(MEMP_STATS_GET(used, MEMP_TCP_PCB) == 1);
  /* test_tcp_tmr(); */
  EXPECT(test_txcounters.num_tx_calls == 1);
  /* create a segment ACK and pass it to tcp_input */
  p = tcp_create_rx_segment(pcb, NULL, 0, 0, 1, TCP_ACK);
  EXPECT_RET(p != NULL);
  test_tcp_input(p, &test_netif);
  EXPECT_RET(pcb->state == FIN_WAIT_2);
  /* create a segment FIN and pass it to tcp_input */
  p = tcp_create_rx_segment(pcb, NULL, 0, 0, 0, TCP_FIN);
  EXPECT_RET(p != NULL);
  test_tcp_input(p, &test_netif);
  EXPECT_RET(pcb->state == TIME_WAIT);
  EXPECT_RET(MEMP_STATS_GET(used, MEMP_TCP_PCB) == 1);
  for (i = 0; i < 2 * TCP_MSL / TCP_TMR_INTERVAL + 1; i++) {
    test_tcp_tmr();
  }
  EXPECT_RET(MEMP_STATS_GET(used, MEMP_TCP_PCB) == 0);
}
END_TEST

START_TEST(test_tcp_imultaneous_close)
{
  struct test_tcp_counters counters;
  struct tcp_pcb* pcb;
  struct pbuf* p;
  char data = 0x0f;
  err_t err;
  u32_t i;
  LWIP_UNUSED_ARG(_i);

  /* initialize counter struct */
  memset(&counters, 0, sizeof(counters));
  counters.expected_data_len = 1;
  counters.expected_data = &data;

  /* create and initialize the pcb */
  pcb = test_tcp_new_counters_pcb(&counters);
  EXPECT_RET(pcb != NULL);
  tcp_set_state(pcb, ESTABLISHED, &test_local_ip, &test_remote_ip, TEST_LOCAL_PORT, TEST_REMOTE_PORT);
  err = tcp_close(pcb);
  EXPECT_RET(err == ERR_OK);
  EXPECT_RET(pcb->state == FIN_WAIT_1);
  /* create a FIN segment */
  p = tcp_create_rx_segment(pcb, &data, 0, 0, 0, TCP_FIN);
  EXPECT(p != NULL);
  if (p != NULL) {
    test_tcp_input(p, &test_netif);
  }
  EXPECT_RET(pcb->state == CLOSING);
  /* create an ACK segment */
  p = tcp_create_rx_segment(pcb, &data, 0, 0, 1, TCP_ACK);
  EXPECT(p != NULL);
  if (p != NULL) {
    test_tcp_input(p, &test_netif);
  }
  EXPECT_RET(pcb->state == TIME_WAIT);
  for (i = 0; i < 2 * TCP_MSL / TCP_TMR_INTERVAL + 1; i++) {
    test_tcp_tmr();
  }
  EXPECT_RET(MEMP_STATS_GET(used, MEMP_TCP_PCB) == 0);
}
END_TEST


/* RST was generated when receive any incoming segment in CLOSED state */
START_TEST(test_tcp_gen_rst_in_CLOSED)
{
  struct pbuf *p;
  ip_addr_t src_addr = test_remote_ip;
  ip_addr_t dst_addr = test_local_ip;
  LWIP_UNUSED_ARG(_i);

  /* Do not create any pcb */

  /* create a segment */
  p = tcp_create_segment(&src_addr, &dst_addr, TEST_REMOTE_PORT,
    TEST_LOCAL_PORT, NULL, 0, 12345, 54321, TCP_ACK);
  EXPECT(p != NULL);
  test_rst_generation_with_incoming_packet(p, &test_netif, &test_txcounters);
  EXPECT(test_txcounters.num_tx_calls == 1);

}
END_TEST

/* RST was generated when receive ACK in LISTEN state */
START_TEST(test_tcp_gen_rst_in_LISTEN)
{
  struct tcp_pcb_listen *lpcb;
  struct pbuf *p;
  ip_addr_t src_addr = test_remote_ip;
  LWIP_UNUSED_ARG(_i);

  /* create a pcb in LISTEN state */
  lpcb = create_listening_pcb(TEST_LOCAL_PORT, NULL);
  EXPECT_RET(lpcb != NULL);

  /* create a segment */
  p = tcp_create_segment(&src_addr,&lpcb->local_ip, TEST_REMOTE_PORT,
    lpcb->local_port, NULL, 0, 12345, 54321, TCP_ACK);
  EXPECT(p != NULL);
  test_rst_generation_with_incoming_packet(p, &test_netif, &test_txcounters);
  EXPECT(test_txcounters.num_tx_calls == 1);

  /* the PCB still in LISTEN state */
  EXPECT(MEMP_STATS_GET(used, MEMP_TCP_PCB_LISTEN) == 1);
  if (MEMP_STATS_GET(used, MEMP_TCP_PCB_LISTEN) != 0) {
    /* can not use tcp_abort() */
    tcp_close((struct tcp_pcb *)lpcb);
  }

}
END_TEST


/* RST was generated when receive an SYN in TIME_WAIT state */
START_TEST(test_tcp_gen_rst_in_TIME_WAIT)
{
  struct tcp_pcb *pcb;
  struct pbuf *p;
  LWIP_UNUSED_ARG(_i);

  /* create a pcb in LISTEN state */
  pcb = tcp_new();
  EXPECT_RET(pcb != NULL);
  tcp_set_state(pcb, TIME_WAIT, &test_local_ip, &test_remote_ip, TEST_LOCAL_PORT, TEST_REMOTE_PORT);

  /* create a segment */
  p = tcp_create_rx_segment(pcb, NULL, 0, 0, 0, TCP_SYN);
  EXPECT(p != NULL);
  test_rst_generation_with_incoming_packet(p, &test_netif, &test_txcounters);
  EXPECT(test_txcounters.num_tx_calls == 1);

  EXPECT(MEMP_STATS_GET(used, MEMP_TCP_PCB) == 1);
  EXPECT(pcb->state == TIME_WAIT);
}
END_TEST

/* receive TCP_RST with different seqno */
START_TEST(test_tcp_process_rst_seqno)
{
  struct test_tcp_counters counters;
  struct tcp_pcb *pcb;
  struct pbuf *p;
  err_t err;
  LWIP_UNUSED_ARG(_i);

  /* create and initialize a pcb in SYN_SENT state */
  memset(&counters, 0, sizeof(counters));
  pcb = test_tcp_new_counters_pcb(&counters);
  EXPECT_RET(pcb != NULL);
  err = tcp_connect(pcb, &test_remote_ip, TEST_REMOTE_PORT, NULL);
  EXPECT_RET(err == ERR_OK);

  /* a RST segment with incorrect seqno will not be accepted */
  p = tcp_create_segment(&pcb->remote_ip, &pcb->local_ip, TEST_REMOTE_PORT,
    pcb->local_port, NULL, 0, 12345,  pcb->snd_nxt-10, TCP_RST);
  EXPECT(p != NULL);
  test_tcp_input(p, &test_netif);
  EXPECT(counters.err_calls == 0);

  /* a RST segment with correct seqno will be accepted */
  p = tcp_create_segment(&pcb->remote_ip, &pcb->local_ip, TEST_REMOTE_PORT,
    pcb->local_port, NULL, 0, 12345,  pcb->snd_nxt, TCP_RST);
  EXPECT(p != NULL);
  test_tcp_input(p, &test_netif);
  EXPECT(counters.err_calls == 1);
  counters.err_calls = 0;
  EXPECT(MEMP_STATS_GET(used, MEMP_TCP_PCB) == 0);

  /* create another pcb in ESTABLISHED state */
  memset(&counters, 0, sizeof(counters));
  pcb = test_tcp_new_counters_pcb(&counters);
  EXPECT_RET(pcb != NULL);
  tcp_set_state(pcb, ESTABLISHED, &test_local_ip, &test_remote_ip, TEST_LOCAL_PORT, TEST_REMOTE_PORT);

  /* a RST segment with incorrect seqno will not be accepted */
  p = tcp_create_segment(&pcb->remote_ip, &pcb->local_ip, pcb->remote_port,
    pcb->local_port, NULL, 0, pcb->rcv_nxt-10, 54321, TCP_RST);
  EXPECT(p != NULL);
  test_tcp_input(p, &test_netif);
  EXPECT(counters.err_calls == 0);

  /* a RST segment with correct seqno will be accepted */
  p = tcp_create_segment(&pcb->remote_ip, &pcb->local_ip, TEST_REMOTE_PORT,
    pcb->local_port, NULL, 0, pcb->rcv_nxt, 54321, TCP_RST);
  EXPECT(p != NULL);
  test_tcp_input(p, &test_netif);
  EXPECT(counters.err_calls == 1);
  counters.err_calls = 0;
  EXPECT(MEMP_STATS_GET(used, MEMP_TCP_PCB) == 0);

}
END_TEST

/* RST was generated when receive an SYN+ACK with incorrect ACK number in SYN_SENT state */
START_TEST(test_tcp_gen_rst_in_SYN_SENT_ackseq)
{
  struct tcp_pcb *pcb;
  struct pbuf *p;
  u16_t test_port = 1234;
  err_t err;
  LWIP_UNUSED_ARG(_i);

  /* create and initialize a pcb in listen state */
  pcb = tcp_new();
  EXPECT_RET(pcb != NULL);
  err = tcp_connect(pcb, &test_remote_ip, test_port, NULL);
  EXPECT_RET(err == ERR_OK);

  /* create a SYN+ACK segment with incorrect seqno */
  p = tcp_create_segment(&pcb->remote_ip, &pcb->local_ip, pcb->remote_port,
    pcb->local_port, NULL, 0, 12345,  pcb->lastack-10, TCP_SYN|TCP_ACK);
  EXPECT(p != NULL);
  test_rst_generation_with_incoming_packet(p, &test_netif, &test_txcounters);

  /* LWIP: send RST then re-send SYN immediately */
  EXPECT(test_txcounters.num_tx_calls == 2);

}
END_TEST

/* RST was generated when receive an ACK without SYN in SYN_SENT state */
START_TEST(test_tcp_gen_rst_in_SYN_SENT_non_syn_ack)
{
  struct tcp_pcb *pcb;
  struct pbuf *p;
  u16_t test_port = 1234;
  err_t err;
  LWIP_UNUSED_ARG(_i);

  /* create and initialize a pcb in listen state */
  pcb = tcp_new();
  EXPECT_RET(pcb != NULL);
  err = tcp_connect(pcb, &test_remote_ip, test_port, NULL);
  EXPECT_RET(err == ERR_OK);

  /* create a SYN+ACK segment with incorrect seqno */
  p = tcp_create_segment(&pcb->remote_ip, &pcb->local_ip, pcb->remote_port,
    pcb->local_port, NULL, 0, 12345,  pcb->lastack, TCP_ACK);
  EXPECT(p != NULL);
  test_rst_generation_with_incoming_packet(p, &test_netif, &test_txcounters);

  /* LWIP: send RST then re-send SYN immediately */
  EXPECT(test_txcounters.num_tx_calls == 2);

}
END_TEST

/* RST was generated when receive an ACK with incorrect seqno in SYN_RCVD state */
START_TEST(test_tcp_gen_rst_in_SYN_RCVD)
{
  struct tcp_pcb_listen *lpcb;
  struct pbuf *p;
  u32_t ack_seqno = 0;
  ip_addr_t src_addr = test_remote_ip;
  LWIP_UNUSED_ARG(_i);

  /* create and initialize a pcb in listen state */
  lpcb = create_listening_pcb(TEST_LOCAL_PORT, NULL);
  EXPECT_RET(lpcb != NULL);

  /* LISTEN -> SYN_RCVD */
  p = tcp_create_segment(&src_addr, &lpcb->local_ip, TEST_REMOTE_PORT,
    lpcb->local_port, NULL, 0, 1000, 54321, TCP_SYN);
  EXPECT(p != NULL);
  memset(&test_txcounters, 0, sizeof(struct test_tcp_txcounters));
  test_tcp_input(p, &test_netif);
  EXPECT(test_txcounters.num_tx_calls == 1);
  EXPECT(MEMP_STATS_GET(used, MEMP_TCP_PCB) == 1);
  if (MEMP_STATS_GET(used, MEMP_TCP_PCB) == 1) {
    ack_seqno = tcp_active_pcbs[0].lastack;
  }

  /* create a ACK segment with incorrect seqno */
  p = tcp_create_segment(&src_addr, &lpcb->local_ip, TEST_REMOTE_PORT,
    lpcb->local_port, NULL, 0, 1001, ack_seqno+1111, TCP_ACK);
  EXPECT(p != NULL);
  test_rst_generation_with_incoming_packet(p, &test_netif, &test_txcounters);
  EXPECT(test_txcounters.num_tx_calls == 1);

  /* the active pcb still exists */
  EXPECT(MEMP_STATS_GET(used, MEMP_TCP_PCB) == 1);
  EXPECT(MEMP_STATS_GET(used, MEMP_TCP_PCB_LISTEN) == 1);
  if (MEMP_STATS_GET(used, MEMP_TCP_PCB_LISTEN) != 0) {
    /* can not use tcp_abort() */
    tcp_close((struct tcp_pcb *)lpcb);
  }
}
END_TEST

/* a listen pcb returns to LISTEN from SYN_RCVD when RST received */
START_TEST(test_tcp_receive_rst_SYN_RCVD_to_LISTEN)
{
  struct tcp_pcb_listen *lpcb;
  struct pbuf *p;
  u16_t tcp_flags;
  ip_addr_t src_addr = test_remote_ip;
  LWIP_UNUSED_ARG(_i);

  /* create and initialize a pcb in listen state */
  lpcb = create_listening_pcb(TEST_LOCAL_PORT, NULL);
  EXPECT_RET(lpcb != NULL);

  /* create a SYN segment */
  p = tcp_create_segment(&src_addr, &lpcb->local_ip, TEST_REMOTE_PORT,
    lpcb->local_port, NULL, 0, 1000, 54321, TCP_SYN);
  EXPECT(p != NULL);
  /* pass the segment to tcp_input */
  memset(&test_txcounters, 0, sizeof(struct test_tcp_txcounters));
  test_txcounters.copy_tx_packets = 1;
  test_tcp_input(p, &test_netif);
  test_txcounters.copy_tx_packets = 0;
  /* check if packets are as expected */
  EXPECT(test_txcounters.num_tx_calls == 1);
  tcp_flags = get_tcp_flags_from_packet(test_txcounters.tx_packets, 20);
  pbuf_free(test_txcounters.tx_packets);
  test_txcounters.tx_packets = NULL;
  EXPECT((tcp_flags & TCP_SYN) && (tcp_flags & TCP_ACK));
  EXPECT(MEMP_STATS_GET(used, MEMP_TCP_PCB) == 1);

  /* create a RST segment */
  p = tcp_create_segment(&src_addr, &lpcb->local_ip, TEST_REMOTE_PORT,
    lpcb->local_port, NULL, 0, 1001, 54321, TCP_RST);
  EXPECT(p != NULL);
  test_tcp_input(p, &test_netif);
  EXPECT(MEMP_STATS_GET(used, MEMP_TCP_PCB) == 0);
  EXPECT(MEMP_STATS_GET(used, MEMP_TCP_PCB_LISTEN) == 1);

  if (MEMP_STATS_GET(used, MEMP_TCP_PCB_LISTEN) != 0) {
    /* can not use tcp_abort() */
    tcp_close((struct tcp_pcb *)lpcb);
  }
}
END_TEST

/** Create the suite including all tests for this module */
Suite *
tcp_state_suite(void)
{
  testfunc tests[] = {
    TESTFUNC(test_tcp_new_max_num),
    TESTFUNC(test_tcp_new_max_num_remove_TIME_WAIT),
    TESTFUNC(test_tcp_connect_active_open),
    TESTFUNC(test_tcp_active_close),
    TESTFUNC(test_tcp_imultaneous_close),
    TESTFUNC(test_tcp_gen_rst_in_CLOSED),
    TESTFUNC(test_tcp_gen_rst_in_LISTEN),
    TESTFUNC(test_tcp_gen_rst_in_TIME_WAIT),
    TESTFUNC(test_tcp_process_rst_seqno),
    TESTFUNC(test_tcp_gen_rst_in_SYN_SENT_ackseq),
    TESTFUNC(test_tcp_gen_rst_in_SYN_SENT_non_syn_ack),
    TESTFUNC(test_tcp_gen_rst_in_SYN_RCVD),
    TESTFUNC(test_tcp_receive_rst_SYN_RCVD_to_LISTEN),
  };
  return create_suite("TCP_STATE", tests, sizeof(tests) / sizeof(testfunc), tcp_state_setup, tcp_state_teardown);
}
