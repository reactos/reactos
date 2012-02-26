#include "tcp_helper.h"

#include "lwip/tcp_impl.h"
#include "lwip/stats.h"
#include "lwip/pbuf.h"
#include "lwip/inet_chksum.h"

#if !LWIP_STATS || !TCP_STATS || !MEMP_STATS
#error "This tests needs TCP- and MEMP-statistics enabled"
#endif

/** Remove all pcbs on the given list. */
static void
tcp_remove(struct tcp_pcb* pcb_list)
{
  struct tcp_pcb *pcb = pcb_list;
  struct tcp_pcb *pcb2;

  while(pcb != NULL) {
    pcb2 = pcb;
    pcb = pcb->next;
    tcp_abort(pcb2);
  }
}

/** Remove all pcbs on listen-, active- and time-wait-list (bound- isn't exported). */
void
tcp_remove_all(void)
{
  tcp_remove(tcp_listen_pcbs.pcbs);
  tcp_remove(tcp_active_pcbs);
  tcp_remove(tcp_tw_pcbs);
  fail_unless(lwip_stats.memp[MEMP_TCP_PCB].used == 0);
  fail_unless(lwip_stats.memp[MEMP_TCP_PCB_LISTEN].used == 0);
  fail_unless(lwip_stats.memp[MEMP_TCP_SEG].used == 0);
  fail_unless(lwip_stats.memp[MEMP_PBUF_POOL].used == 0);
}

/** Create a TCP segment usable for passing to tcp_input
 * - IP-addresses, ports, seqno and ackno are taken from pcb
 * - seqno and ackno can be altered with an offset
 */
struct pbuf*
tcp_create_rx_segment(struct tcp_pcb* pcb, void* data, size_t data_len, u32_t seqno_offset,
                      u32_t ackno_offset, u8_t headerflags)
{
  return tcp_create_segment(&pcb->remote_ip, &pcb->local_ip, pcb->remote_port, pcb->local_port,
    data, data_len, pcb->rcv_nxt + seqno_offset, pcb->snd_nxt + ackno_offset, headerflags);
}

/** Create a TCP segment usable for passing to tcp_input */
struct pbuf*
tcp_create_segment(ip_addr_t* src_ip, ip_addr_t* dst_ip,
                   u16_t src_port, u16_t dst_port, void* data, size_t data_len,
                   u32_t seqno, u32_t ackno, u8_t headerflags)
{
  struct pbuf* p;
  struct ip_hdr* iphdr;
  struct tcp_hdr* tcphdr;
  u16_t pbuf_len = (u16_t)(sizeof(struct ip_hdr) + sizeof(struct tcp_hdr) + data_len);

  p = pbuf_alloc(PBUF_RAW, pbuf_len, PBUF_POOL);
  EXPECT_RETNULL(p != NULL);
  EXPECT_RETNULL(p->next == NULL);

  memset(p->payload, 0, p->len);

  iphdr = p->payload;
  /* fill IP header */
  iphdr->dest.addr = dst_ip->addr;
  iphdr->src.addr = src_ip->addr;
  IPH_VHLTOS_SET(iphdr, 4, IP_HLEN / 4, 0);
  IPH_LEN_SET(iphdr, htons(p->tot_len));
  IPH_CHKSUM_SET(iphdr, inet_chksum(iphdr, IP_HLEN));

  pbuf_header(p, -(s16_t)sizeof(struct ip_hdr));

  tcphdr = p->payload;
  tcphdr->src   = htons(src_port);
  tcphdr->dest  = htons(dst_port);
  tcphdr->seqno = htonl(seqno);
  tcphdr->ackno = htonl(ackno);
  TCPH_HDRLEN_SET(tcphdr, sizeof(struct tcp_hdr)/4);
  TCPH_FLAGS_SET(tcphdr, headerflags);
  tcphdr->wnd   = htons(TCP_WND);

  /* copy data */
  memcpy((char*)tcphdr + sizeof(struct tcp_hdr), data, data_len);

  /* calculate checksum */

  tcphdr->chksum = inet_chksum_pseudo(p, src_ip, dst_ip,
          IP_PROTO_TCP, p->tot_len);

  pbuf_header(p, sizeof(struct ip_hdr));

  return p;
}

/** Safely bring a tcp_pcb into the requested state */
void
tcp_set_state(struct tcp_pcb* pcb, enum tcp_state state, ip_addr_t* local_ip,
                   ip_addr_t* remote_ip, u16_t local_port, u16_t remote_port)
{
  /* @todo: are these all states? */
  /* @todo: remove from previous list */
  pcb->state = state;
  if (state == ESTABLISHED) {
    TCP_REG(&tcp_active_pcbs, pcb);
    pcb->local_ip.addr = local_ip->addr;
    pcb->local_port = local_port;
    pcb->remote_ip.addr = remote_ip->addr;
    pcb->remote_port = remote_port;
  } else if(state == LISTEN) {
    TCP_REG(&tcp_listen_pcbs.pcbs, pcb);
    pcb->local_ip.addr = local_ip->addr;
    pcb->local_port = local_port;
  } else if(state == TIME_WAIT) {
    TCP_REG(&tcp_tw_pcbs, pcb);
    pcb->local_ip.addr = local_ip->addr;
    pcb->local_port = local_port;
    pcb->remote_ip.addr = remote_ip->addr;
    pcb->remote_port = remote_port;
  } else {
    fail();
  }
}

void
test_tcp_counters_err(void* arg, err_t err)
{
  struct test_tcp_counters* counters = arg;
  EXPECT_RET(arg != NULL);
  counters->err_calls++;
  counters->last_err = err;
}

static void
test_tcp_counters_check_rxdata(struct test_tcp_counters* counters, struct pbuf* p)
{
  struct pbuf* q;
  u32_t i, received;
  if(counters->expected_data == NULL) {
    /* no data to compare */
    return;
  }
  EXPECT_RET(counters->recved_bytes + p->tot_len <= counters->expected_data_len);
  received = counters->recved_bytes;
  for(q = p; q != NULL; q = q->next) {
    char *data = q->payload;
    for(i = 0; i < q->len; i++) {
      EXPECT_RET(data[i] == counters->expected_data[received]);
      received++;
    }
  }
  EXPECT(received == counters->recved_bytes + p->tot_len);
}

err_t
test_tcp_counters_recv(void* arg, struct tcp_pcb* pcb, struct pbuf* p, err_t err)
{
  struct test_tcp_counters* counters = arg;
  EXPECT_RETX(arg != NULL, ERR_OK);
  EXPECT_RETX(pcb != NULL, ERR_OK);
  EXPECT_RETX(err == ERR_OK, ERR_OK);

  if (p != NULL) {
    if (counters->close_calls == 0) {
      counters->recv_calls++;
      test_tcp_counters_check_rxdata(counters, p);
      counters->recved_bytes += p->tot_len;
    } else {
      counters->recv_calls_after_close++;
      counters->recved_bytes_after_close += p->tot_len;
    }
    pbuf_free(p);
  } else {
    counters->close_calls++;
  }
  EXPECT(counters->recv_calls_after_close == 0 && counters->recved_bytes_after_close == 0);
  return ERR_OK;
}

/** Allocate a pcb and set up the test_tcp_counters_* callbacks */
struct tcp_pcb*
test_tcp_new_counters_pcb(struct test_tcp_counters* counters)
{
  struct tcp_pcb* pcb = tcp_new();
  if (pcb != NULL) {
    /* set up args and callbacks */
    tcp_arg(pcb, counters);
    tcp_recv(pcb, test_tcp_counters_recv);
    tcp_err(pcb, test_tcp_counters_err);
  }
  return pcb;
}

/** Calls tcp_input() after adjusting current_iphdr_dest */
void test_tcp_input(struct pbuf *p, struct netif *inp)
{
  struct ip_hdr *iphdr = (struct ip_hdr*)p->payload;
  ip_addr_copy(current_iphdr_dest, iphdr->dest);
  ip_addr_copy(current_iphdr_src, iphdr->src);
  current_netif = inp;
  current_header = iphdr;

  tcp_input(p, inp);

  current_iphdr_dest.addr = 0;
  current_iphdr_src.addr = 0;
  current_netif = NULL;
  current_header = NULL;
}
