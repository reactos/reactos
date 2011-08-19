#include "test_tcp_oos.h"

#include "lwip/tcp.h"
#include "lwip/stats.h"
#include "tcp_helper.h"

#if !LWIP_STATS || !TCP_STATS || !MEMP_STATS
#error "This tests needs TCP- and MEMP-statistics enabled"
#endif
#if !TCP_QUEUE_OOSEQ
#error "This tests needs TCP_QUEUE_OOSEQ enabled"
#endif

/** CHECK_SEGMENTS_ON_OOSEQ:
 * 1: check count, seqno and len of segments on pcb->ooseq (strict)
 * 0: only check that bytes are received in correct order (less strict) */
#define CHECK_SEGMENTS_ON_OOSEQ 1

#if CHECK_SEGMENTS_ON_OOSEQ
#define EXPECT_OOSEQ(x) EXPECT(x)
#else
#define EXPECT_OOSEQ(x)
#endif

/* helper functions */

/** Get the numbers of segments on the ooseq list */
static int tcp_oos_count(struct tcp_pcb* pcb)
{
  int num = 0;
  struct tcp_seg* seg = pcb->ooseq;
  while(seg != NULL) {
    num++;
    seg = seg->next;
  }
  return num;
}

/** Get the seqno of a segment (by index) on the ooseq list
 *
 * @param pcb the pcb to check for ooseq segments
 * @param seg_index index of the segment on the ooseq list
 * @return seqno of the segment
 */
static u32_t
tcp_oos_seg_seqno(struct tcp_pcb* pcb, int seg_index)
{
  int num = 0;
  struct tcp_seg* seg = pcb->ooseq;

  /* then check the actual segment */
  while(seg != NULL) {
    if(num == seg_index) {
      return seg->tcphdr->seqno;
    }
    num++;
    seg = seg->next;
  }
  fail();
  return 0;
}

/** Get the tcplen of a segment (by index) on the ooseq list
 *
 * @param pcb the pcb to check for ooseq segments
 * @param seg_index index of the segment on the ooseq list
 * @return tcplen of the segment
 */
static int
tcp_oos_seg_tcplen(struct tcp_pcb* pcb, int seg_index)
{
  int num = 0;
  struct tcp_seg* seg = pcb->ooseq;

  /* then check the actual segment */
  while(seg != NULL) {
    if(num == seg_index) {
      return TCP_TCPLEN(seg);
    }
    num++;
    seg = seg->next;
  }
  fail();
  return -1;
}

/* Setup/teardown functions */

static void
tcp_oos_setup(void)
{
  tcp_remove_all();
}

static void
tcp_oos_teardown(void)
{
  tcp_remove_all();
}



/* Test functions */

/** create multiple segments and pass them to tcp_input in a wrong
 * order to see if ooseq-caching works correctly
 * FIN is received in out-of-sequence segments only */
START_TEST(test_tcp_recv_ooseq_FIN_OOSEQ)
{
  struct test_tcp_counters counters;
  struct tcp_pcb* pcb;
  struct pbuf *p_8_9, *p_4_8, *p_4_10, *p_2_14, *p_fin, *pinseq;
  char data[] = {
     1,  2,  3,  4,
     5,  6,  7,  8,
     9, 10, 11, 12,
    13, 14, 15, 16};
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

  /* create segments */
  /* pinseq is sent as last segment! */
  pinseq = tcp_create_rx_segment(pcb, &data[0],  4, 0, 0, TCP_ACK);
  /* p1: 8 bytes before FIN */
  /*     seqno: 8..16 */
  p_8_9  = tcp_create_rx_segment(pcb, &data[8],  8, 8, 0, TCP_ACK|TCP_FIN);
  /* p2: 4 bytes before p1, including the first 4 bytes of p1 (partly duplicate) */
  /*     seqno: 4..11 */
  p_4_8  = tcp_create_rx_segment(pcb, &data[4],  8, 4, 0, TCP_ACK);
  /* p3: same as p2 but 2 bytes longer */
  /*     seqno: 4..13 */
  p_4_10 = tcp_create_rx_segment(pcb, &data[4], 10, 4, 0, TCP_ACK);
  /* p4: 14 bytes before FIN, includes data from p1 and p2, plus partly from pinseq */
  /*     seqno: 2..15 */
  p_2_14 = tcp_create_rx_segment(pcb, &data[2], 14, 2, 0, TCP_ACK);
  /* FIN, seqno 16 */
  p_fin  = tcp_create_rx_segment(pcb,     NULL,  0,16, 0, TCP_ACK|TCP_FIN);
  EXPECT(pinseq != NULL);
  EXPECT(p_8_9 != NULL);
  EXPECT(p_4_8 != NULL);
  EXPECT(p_4_10 != NULL);
  EXPECT(p_2_14 != NULL);
  EXPECT(p_fin != NULL);
  if ((pinseq != NULL) && (p_8_9 != NULL) && (p_4_8 != NULL) && (p_4_10 != NULL) && (p_2_14 != NULL) && (p_fin != NULL)) {
    /* pass the segment to tcp_input */
    tcp_input(p_8_9, &netif);
    /* check if counters are as expected */
    EXPECT(counters.close_calls == 0);
    EXPECT(counters.recv_calls == 0);
    EXPECT(counters.recved_bytes == 0);
    EXPECT(counters.err_calls == 0);
    /* check ooseq queue */
    EXPECT_OOSEQ(tcp_oos_count(pcb) == 1);
    EXPECT_OOSEQ(tcp_oos_seg_seqno(pcb, 0) == 8);
    EXPECT_OOSEQ(tcp_oos_seg_tcplen(pcb, 0) == 9); /* includes FIN */

    /* pass the segment to tcp_input */
    tcp_input(p_4_8, &netif);
    /* check if counters are as expected */
    EXPECT(counters.close_calls == 0);
    EXPECT(counters.recv_calls == 0);
    EXPECT(counters.recved_bytes == 0);
    EXPECT(counters.err_calls == 0);
    /* check ooseq queue */
    EXPECT_OOSEQ(tcp_oos_count(pcb) == 2);
    EXPECT_OOSEQ(tcp_oos_seg_seqno(pcb, 0) == 4);
    EXPECT_OOSEQ(tcp_oos_seg_tcplen(pcb, 0) == 4);
    EXPECT_OOSEQ(tcp_oos_seg_seqno(pcb, 1) == 8);
    EXPECT_OOSEQ(tcp_oos_seg_tcplen(pcb, 1) == 9); /* includes FIN */

    /* pass the segment to tcp_input */
    tcp_input(p_4_10, &netif);
    /* check if counters are as expected */
    EXPECT(counters.close_calls == 0);
    EXPECT(counters.recv_calls == 0);
    EXPECT(counters.recved_bytes == 0);
    EXPECT(counters.err_calls == 0);
    /* ooseq queue: unchanged */
    EXPECT_OOSEQ(tcp_oos_count(pcb) == 2);
    EXPECT_OOSEQ(tcp_oos_seg_seqno(pcb, 0) == 4);
    EXPECT_OOSEQ(tcp_oos_seg_tcplen(pcb, 0) == 4);
    EXPECT_OOSEQ(tcp_oos_seg_seqno(pcb, 1) == 8);
    EXPECT_OOSEQ(tcp_oos_seg_tcplen(pcb, 1) == 9); /* includes FIN */

    /* pass the segment to tcp_input */
    tcp_input(p_2_14, &netif);
    /* check if counters are as expected */
    EXPECT(counters.close_calls == 0);
    EXPECT(counters.recv_calls == 0);
    EXPECT(counters.recved_bytes == 0);
    EXPECT(counters.err_calls == 0);
    /* check ooseq queue */
    EXPECT_OOSEQ(tcp_oos_count(pcb) == 2);
    EXPECT_OOSEQ(tcp_oos_seg_seqno(pcb, 0) == 2);
    EXPECT_OOSEQ(tcp_oos_seg_tcplen(pcb, 0) == 6);
    EXPECT_OOSEQ(tcp_oos_seg_seqno(pcb, 1) == 8);
    EXPECT_OOSEQ(tcp_oos_seg_tcplen(pcb, 1) == 9); /* includes FIN */

    /* pass the segment to tcp_input */
    tcp_input(p_fin, &netif);
    /* check if counters are as expected */
    EXPECT(counters.close_calls == 0);
    EXPECT(counters.recv_calls == 0);
    EXPECT(counters.recved_bytes == 0);
    EXPECT(counters.err_calls == 0);
    /* ooseq queue: unchanged */
    EXPECT_OOSEQ(tcp_oos_count(pcb) == 2);
    EXPECT_OOSEQ(tcp_oos_seg_seqno(pcb, 0) == 2);
    EXPECT_OOSEQ(tcp_oos_seg_tcplen(pcb, 0) == 6);
    EXPECT_OOSEQ(tcp_oos_seg_seqno(pcb, 1) == 8);
    EXPECT_OOSEQ(tcp_oos_seg_tcplen(pcb, 1) == 9); /* includes FIN */

    /* pass the segment to tcp_input */
    tcp_input(pinseq, &netif);
    /* check if counters are as expected */
    EXPECT(counters.close_calls == 1);
    EXPECT(counters.recv_calls == 1);
    EXPECT(counters.recved_bytes == data_len);
    EXPECT(counters.err_calls == 0);
    EXPECT(pcb->ooseq == NULL);
  }

  /* make sure the pcb is freed */
  EXPECT(lwip_stats.memp[MEMP_TCP_PCB].used == 1);
  tcp_abort(pcb);
  EXPECT(lwip_stats.memp[MEMP_TCP_PCB].used == 0);
}
END_TEST


/** create multiple segments and pass them to tcp_input in a wrong
 * order to see if ooseq-caching works correctly
 * FIN is received IN-SEQUENCE at the end */
START_TEST(test_tcp_recv_ooseq_FIN_INSEQ)
{
  struct test_tcp_counters counters;
  struct tcp_pcb* pcb;
  struct pbuf *p_1_2, *p_4_8, *p_3_11, *p_2_12, *p_15_1, *p_15_1a, *pinseq, *pinseqFIN;
  char data[] = {
     1,  2,  3,  4,
     5,  6,  7,  8,
     9, 10, 11, 12,
    13, 14, 15, 16};
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

  /* create segments */
  /* p1: 7 bytes - 2 before FIN */
  /*     seqno: 1..2 */
  p_1_2  = tcp_create_rx_segment(pcb, &data[1],  2, 1, 0, TCP_ACK);
  /* p2: 4 bytes before p1, including the first 4 bytes of p1 (partly duplicate) */
  /*     seqno: 4..11 */
  p_4_8  = tcp_create_rx_segment(pcb, &data[4],  8, 4, 0, TCP_ACK);
  /* p3: same as p2 but 2 bytes longer and one byte more at the front */
  /*     seqno: 3..13 */
  p_3_11 = tcp_create_rx_segment(pcb, &data[3], 11, 3, 0, TCP_ACK);
  /* p4: 13 bytes - 2 before FIN - should be ignored as contained in p1 and p3 */
  /*     seqno: 2..13 */
  p_2_12 = tcp_create_rx_segment(pcb, &data[2], 12, 2, 0, TCP_ACK);
  /* pinseq is the first segment that is held back to create ooseq! */
  /*     seqno: 0..3 */
  pinseq = tcp_create_rx_segment(pcb, &data[0],  4, 0, 0, TCP_ACK);
  /* p5: last byte before FIN */
  /*     seqno: 15 */
  p_15_1 = tcp_create_rx_segment(pcb, &data[15], 1, 15, 0, TCP_ACK);
  /* p6: same as p5, should be ignored */
  p_15_1a= tcp_create_rx_segment(pcb, &data[15], 1, 15, 0, TCP_ACK);
  /* pinseqFIN: last 2 bytes plus FIN */
  /*     only segment containing seqno 14 and FIN */
  pinseqFIN = tcp_create_rx_segment(pcb,  &data[14], 2, 14, 0, TCP_ACK|TCP_FIN);
  EXPECT(pinseq != NULL);
  EXPECT(p_1_2 != NULL);
  EXPECT(p_4_8 != NULL);
  EXPECT(p_3_11 != NULL);
  EXPECT(p_2_12 != NULL);
  EXPECT(p_15_1 != NULL);
  EXPECT(p_15_1a != NULL);
  EXPECT(pinseqFIN != NULL);
  if ((pinseq != NULL) && (p_1_2 != NULL) && (p_4_8 != NULL) && (p_3_11 != NULL) && (p_2_12 != NULL)
    && (p_15_1 != NULL) && (p_15_1a != NULL) && (pinseqFIN != NULL)) {
    /* pass the segment to tcp_input */
    tcp_input(p_1_2, &netif);
    /* check if counters are as expected */
    EXPECT(counters.close_calls == 0);
    EXPECT(counters.recv_calls == 0);
    EXPECT(counters.recved_bytes == 0);
    EXPECT(counters.err_calls == 0);
    /* check ooseq queue */
    EXPECT_OOSEQ(tcp_oos_count(pcb) == 1);
    EXPECT_OOSEQ(tcp_oos_seg_seqno(pcb, 0) == 1);
    EXPECT_OOSEQ(tcp_oos_seg_tcplen(pcb, 0) == 2);

    /* pass the segment to tcp_input */
    tcp_input(p_4_8, &netif);
    /* check if counters are as expected */
    EXPECT(counters.close_calls == 0);
    EXPECT(counters.recv_calls == 0);
    EXPECT(counters.recved_bytes == 0);
    EXPECT(counters.err_calls == 0);
    /* check ooseq queue */
    EXPECT_OOSEQ(tcp_oos_count(pcb) == 2);
    EXPECT_OOSEQ(tcp_oos_seg_seqno(pcb, 0) == 1);
    EXPECT_OOSEQ(tcp_oos_seg_tcplen(pcb, 0) == 2);
    EXPECT_OOSEQ(tcp_oos_seg_seqno(pcb, 1) == 4);
    EXPECT_OOSEQ(tcp_oos_seg_tcplen(pcb, 1) == 8);

    /* pass the segment to tcp_input */
    tcp_input(p_3_11, &netif);
    /* check if counters are as expected */
    EXPECT(counters.close_calls == 0);
    EXPECT(counters.recv_calls == 0);
    EXPECT(counters.recved_bytes == 0);
    EXPECT(counters.err_calls == 0);
    /* check ooseq queue */
    EXPECT_OOSEQ(tcp_oos_count(pcb) == 2);
    EXPECT_OOSEQ(tcp_oos_seg_seqno(pcb, 0) == 1);
    EXPECT_OOSEQ(tcp_oos_seg_tcplen(pcb, 0) == 2);
    /* p_3_11 has removed p_4_8 from ooseq */
    EXPECT_OOSEQ(tcp_oos_seg_seqno(pcb, 1) == 3);
    EXPECT_OOSEQ(tcp_oos_seg_tcplen(pcb, 1) == 11);

    /* pass the segment to tcp_input */
    tcp_input(p_2_12, &netif);
    /* check if counters are as expected */
    EXPECT(counters.close_calls == 0);
    EXPECT(counters.recv_calls == 0);
    EXPECT(counters.recved_bytes == 0);
    EXPECT(counters.err_calls == 0);
    /* check ooseq queue */
    EXPECT_OOSEQ(tcp_oos_count(pcb) == 3);
    EXPECT_OOSEQ(tcp_oos_seg_seqno(pcb, 0) == 1);
    EXPECT_OOSEQ(tcp_oos_seg_tcplen(pcb, 0) == 1);
    EXPECT_OOSEQ(tcp_oos_seg_seqno(pcb, 1) == 2);
    EXPECT_OOSEQ(tcp_oos_seg_tcplen(pcb, 1) == 1);
    EXPECT_OOSEQ(tcp_oos_seg_seqno(pcb, 2) == 3);
    EXPECT_OOSEQ(tcp_oos_seg_tcplen(pcb, 2) == 11);

    /* pass the segment to tcp_input */
    tcp_input(pinseq, &netif);
    /* check if counters are as expected */
    EXPECT(counters.close_calls == 0);
    EXPECT(counters.recv_calls == 1);
    EXPECT(counters.recved_bytes == 14);
    EXPECT(counters.err_calls == 0);
    EXPECT(pcb->ooseq == NULL);

    /* pass the segment to tcp_input */
    tcp_input(p_15_1, &netif);
    /* check if counters are as expected */
    EXPECT(counters.close_calls == 0);
    EXPECT(counters.recv_calls == 1);
    EXPECT(counters.recved_bytes == 14);
    EXPECT(counters.err_calls == 0);
    /* check ooseq queue */
    EXPECT_OOSEQ(tcp_oos_count(pcb) == 1);
    EXPECT_OOSEQ(tcp_oos_seg_seqno(pcb, 0) == 15);
    EXPECT_OOSEQ(tcp_oos_seg_tcplen(pcb, 0) == 1);

    /* pass the segment to tcp_input */
    tcp_input(p_15_1a, &netif);
    /* check if counters are as expected */
    EXPECT(counters.close_calls == 0);
    EXPECT(counters.recv_calls == 1);
    EXPECT(counters.recved_bytes == 14);
    EXPECT(counters.err_calls == 0);
    /* check ooseq queue: unchanged */
    EXPECT_OOSEQ(tcp_oos_count(pcb) == 1);
    EXPECT_OOSEQ(tcp_oos_seg_seqno(pcb, 0) == 15);
    EXPECT_OOSEQ(tcp_oos_seg_tcplen(pcb, 0) == 1);

    /* pass the segment to tcp_input */
    tcp_input(pinseqFIN, &netif);
    /* check if counters are as expected */
    EXPECT(counters.close_calls == 1);
    EXPECT(counters.recv_calls == 2);
    EXPECT(counters.recved_bytes == data_len);
    EXPECT(counters.err_calls == 0);
    EXPECT(pcb->ooseq == NULL);
  }

  /* make sure the pcb is freed */
  EXPECT(lwip_stats.memp[MEMP_TCP_PCB].used == 1);
  tcp_abort(pcb);
  EXPECT(lwip_stats.memp[MEMP_TCP_PCB].used == 0);
}
END_TEST

/** Create the suite including all tests for this module */
Suite *
tcp_oos_suite(void)
{
  TFun tests[] = {
    test_tcp_recv_ooseq_FIN_OOSEQ,
    test_tcp_recv_ooseq_FIN_INSEQ,
  };
  return create_suite("TCP_OOS", tests, sizeof(tests)/sizeof(TFun), tcp_oos_setup, tcp_oos_teardown);
}
