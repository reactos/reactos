#include "test_udp.h"

#include "lwip/udp.h"
#include "lwip/stats.h"
#include "lwip/inet_chksum.h"

#if !LWIP_STATS || !UDP_STATS || !MEMP_STATS
#error "This tests needs UDP- and MEMP-statistics enabled"
#endif

struct test_udp_rxdata {
  u32_t rx_cnt;
  u32_t rx_bytes;
  struct udp_pcb *pcb;
};

static struct netif test_netif1, test_netif2;
static ip4_addr_t test_gw1, test_ipaddr1, test_netmask1;
static ip4_addr_t test_gw2, test_ipaddr2, test_netmask2;
static int output_ctr, linkoutput_ctr;

/* Helper functions */
static void
udp_remove_all(void)
{
  struct udp_pcb *pcb = udp_pcbs;
  struct udp_pcb *pcb2;

  while(pcb != NULL) {
    pcb2 = pcb;
    pcb = pcb->next;
    udp_remove(pcb2);
  }
  fail_unless(MEMP_STATS_GET(used, MEMP_UDP_PCB) == 0);
}

static err_t
default_netif_output(struct netif *netif, struct pbuf *p, const ip4_addr_t *ipaddr)
{
  fail_unless((netif == &test_netif1) || (netif == &test_netif2));
  fail_unless(p != NULL);
  fail_unless(ipaddr != NULL);
  output_ctr++;
  return ERR_OK;
}

static err_t
default_netif_linkoutput(struct netif *netif, struct pbuf *p)
{
  fail_unless((netif == &test_netif1) || (netif == &test_netif2));
  fail_unless(p != NULL);
  linkoutput_ctr++;
  return ERR_OK;
}

static err_t
default_netif_init(struct netif *netif)
{
  fail_unless(netif != NULL);
  netif->output = default_netif_output;
  netif->linkoutput = default_netif_linkoutput;
  netif->mtu = 1500;
  netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;
  netif->hwaddr_len = 6;
  return ERR_OK;
}

static void
default_netif_add(void)
{
  struct netif *n;

#if LWIP_HAVE_LOOPIF
  fail_unless(netif_list != NULL); /* the loopif */
  fail_unless(netif_list->next == NULL);
#else
  fail_unless(netif_list == NULL);
#endif
  fail_unless(netif_default == NULL);

  IP4_ADDR(&test_ipaddr1, 192,168,0,1);
  IP4_ADDR(&test_netmask1, 255,255,255,0);
  IP4_ADDR(&test_gw1, 192,168,0,254);
  n = netif_add(&test_netif1, &test_ipaddr1, &test_netmask1,
                &test_gw1, NULL, default_netif_init, NULL);
  fail_unless(n == &test_netif1);

  IP4_ADDR(&test_ipaddr2, 192,168,1,1);
  IP4_ADDR(&test_netmask2, 255,255,255,0);
  IP4_ADDR(&test_gw2, 192,168,1,254);
  n = netif_add(&test_netif2, &test_ipaddr2, &test_netmask2,
                &test_gw2, NULL, default_netif_init, NULL);
  fail_unless(n == &test_netif2);

  netif_set_default(&test_netif1);
  netif_set_up(&test_netif1);
  netif_set_up(&test_netif2);
}

static void
default_netif_remove(void)
{
  fail_unless(netif_default == &test_netif1);
  netif_remove(&test_netif1);
  netif_remove(&test_netif2);
  fail_unless(netif_default == NULL);
#if LWIP_HAVE_LOOPIF
  fail_unless(netif_list != NULL); /* the loopif */
  fail_unless(netif_list->next == NULL);
#else
  fail_unless(netif_list == NULL);
#endif
}
/* Setups/teardown functions */

static void
udp_setup(void)
{
  udp_remove_all();
  default_netif_add();
  lwip_check_ensure_no_alloc(SKIP_POOL(MEMP_SYS_TIMEOUT));
}

static void
udp_teardown(void)
{
  udp_remove_all();
  default_netif_remove();
  lwip_check_ensure_no_alloc(SKIP_POOL(MEMP_SYS_TIMEOUT));
}


/* Test functions */

START_TEST(test_udp_new_remove)
{
  struct udp_pcb* pcb;
  LWIP_UNUSED_ARG(_i);

  fail_unless(MEMP_STATS_GET(used, MEMP_UDP_PCB) == 0);

  pcb = udp_new();
  fail_unless(pcb != NULL);
  if (pcb != NULL) {
    fail_unless(MEMP_STATS_GET(used, MEMP_UDP_PCB) == 1);
    udp_remove(pcb);
    fail_unless(MEMP_STATS_GET(used, MEMP_UDP_PCB) == 0);
  }
}
END_TEST

static void test_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p,
    const ip_addr_t *addr, u16_t port)
{
  struct test_udp_rxdata *ctr = (struct test_udp_rxdata *)arg;

  LWIP_UNUSED_ARG(addr);
  LWIP_UNUSED_ARG(port);

  fail_unless(arg != NULL);
  fail_unless(ctr->pcb == pcb);

  ctr->rx_cnt++;
  ctr->rx_bytes += p->tot_len;

  if (p != NULL) {
    pbuf_free(p);
  }
}

static struct pbuf *
test_udp_create_test_packet(u16_t length, u16_t port, u32_t dst_addr)
{
  err_t err;
  u8_t ret;
  struct udp_hdr *uh;
  struct ip_hdr *ih;
  struct pbuf *p;
  const u8_t test_data[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf};

  p = pbuf_alloc(PBUF_TRANSPORT, length, PBUF_POOL);
  fail_unless(p != NULL);
  if (p == NULL) {
    return NULL;
  }
  fail_unless(p->next == NULL);
  err = pbuf_take(p, test_data, length);
  fail_unless(err == ERR_OK);

  /* add UDP header */
  ret = pbuf_add_header(p, sizeof(struct udp_hdr));
  fail_unless(!ret);
  uh = (struct udp_hdr *)p->payload;
  uh->chksum = 0;
  uh->dest = uh->src = lwip_htons(port);
  uh->len = lwip_htons(p->tot_len);
  /* add IPv4 header */
  ret = pbuf_add_header(p, sizeof(struct ip_hdr));
  fail_unless(!ret);
  ih = (struct ip_hdr *)p->payload;
  memset(ih, 0, sizeof(*ih));
  ih->dest.addr = dst_addr;
  ih->_len = lwip_htons(p->tot_len);
  ih->_ttl = 32;
  ih->_proto = IP_PROTO_UDP;
  IPH_VHL_SET(ih, 4, sizeof(struct ip_hdr) / 4);
  IPH_CHKSUM_SET(ih, inet_chksum(ih, sizeof(struct ip_hdr)));
  return p;
}

/* bind 2 pcbs to specific netif IP and test which one gets broadcasts */
START_TEST(test_udp_broadcast_rx_with_2_netifs)
{
  err_t err;
  struct udp_pcb *pcb1, *pcb2;
  const u16_t port = 12345;
  struct test_udp_rxdata ctr1, ctr2;
  struct pbuf *p;
#if SO_REUSE
  struct udp_pcb *pcb_any;
  struct test_udp_rxdata ctr_any;
#endif
  LWIP_UNUSED_ARG(_i);

  pcb1 = udp_new();
  fail_unless(pcb1 != NULL);
  pcb2 = udp_new();
  fail_unless(pcb2 != NULL);

#if SO_REUSE
  pcb_any = udp_new();
  fail_unless(pcb_any != NULL);

  ip_set_option(pcb1, SOF_REUSEADDR);
  ip_set_option(pcb2, SOF_REUSEADDR);
  ip_set_option(pcb_any, SOF_REUSEADDR);

  err = udp_bind(pcb_any, NULL, port);
  fail_unless(err == ERR_OK);
  memset(&ctr_any, 0, sizeof(ctr_any));
  ctr_any.pcb = pcb_any;
  udp_recv(pcb_any, test_recv, &ctr_any);
#endif

  err = udp_bind(pcb1, &test_netif1.ip_addr, port);
  fail_unless(err == ERR_OK);
  err = udp_bind(pcb2, &test_netif2.ip_addr, port);
  fail_unless(err == ERR_OK);

  memset(&ctr1, 0, sizeof(ctr1));
  ctr1.pcb = pcb1;
  memset(&ctr2, 0, sizeof(ctr2));
  ctr2.pcb = pcb2;

  udp_recv(pcb1, test_recv, &ctr1);
  udp_recv(pcb2, test_recv, &ctr2);

  /* unicast to netif1 */
  p = test_udp_create_test_packet(16, port, test_ipaddr1.addr);
  EXPECT_RET(p != NULL);
  err = ip4_input(p, &test_netif1);
  fail_unless(err == ERR_OK);
  fail_unless(ctr1.rx_cnt == 1);
  fail_unless(ctr1.rx_bytes == 16);
  fail_unless(ctr2.rx_cnt == 0);
#if SO_REUSE
  fail_unless(ctr_any.rx_cnt == 0);
#endif
  ctr1.rx_cnt = ctr1.rx_bytes = 0;

  /* unicast to netif2 */
  p = test_udp_create_test_packet(16, port, test_ipaddr2.addr);
  EXPECT_RET(p != NULL);
  err = ip4_input(p, &test_netif2);
  fail_unless(err == ERR_OK);
  fail_unless(ctr2.rx_cnt == 1);
  fail_unless(ctr2.rx_bytes == 16);
  fail_unless(ctr1.rx_cnt == 0);
#if SO_REUSE
  fail_unless(ctr_any.rx_cnt == 0);
#endif
  ctr2.rx_cnt = ctr2.rx_bytes = 0;

  /* broadcast to netif1-broadcast, input to netif2 */
  p = test_udp_create_test_packet(16, port, test_ipaddr1.addr | ~test_netmask1.addr);
  EXPECT_RET(p != NULL);
  err = ip4_input(p, &test_netif2);
  fail_unless(err == ERR_OK);
  fail_unless(ctr1.rx_cnt == 1);
  fail_unless(ctr1.rx_bytes == 16);
  fail_unless(ctr2.rx_cnt == 0);
#if SO_REUSE
  fail_unless(ctr_any.rx_cnt == 0);
#endif
  ctr1.rx_cnt = ctr1.rx_bytes = 0;

  /* broadcast to netif2-broadcast, input to netif1 */
  p = test_udp_create_test_packet(16, port, test_ipaddr2.addr | ~test_netmask2.addr);
  EXPECT_RET(p != NULL);
  err = ip4_input(p, &test_netif1);
  fail_unless(err == ERR_OK);
  fail_unless(ctr2.rx_cnt == 1);
  fail_unless(ctr2.rx_bytes == 16);
  fail_unless(ctr1.rx_cnt == 0);
#if SO_REUSE
  fail_unless(ctr_any.rx_cnt == 0);
#endif
  ctr2.rx_cnt = ctr2.rx_bytes = 0;

  /* broadcast to global-broadcast, input to netif1 */
  p = test_udp_create_test_packet(16, port, 0xffffffff);
  EXPECT_RET(p != NULL);
  err = ip4_input(p, &test_netif1);
  fail_unless(err == ERR_OK);
  fail_unless(ctr1.rx_cnt == 1);
  fail_unless(ctr1.rx_bytes == 16);
  fail_unless(ctr2.rx_cnt == 0);
#if SO_REUSE
  fail_unless(ctr_any.rx_cnt == 0);
#endif
  ctr1.rx_cnt = ctr1.rx_bytes = 0;

  /* broadcast to global-broadcast, input to netif2 */
  p = test_udp_create_test_packet(16, port, 0xffffffff);
  EXPECT_RET(p != NULL);
  err = ip4_input(p, &test_netif2);
  fail_unless(err == ERR_OK);
  fail_unless(ctr2.rx_cnt == 1);
  fail_unless(ctr2.rx_bytes == 16);
  fail_unless(ctr1.rx_cnt == 0);
#if SO_REUSE
  fail_unless(ctr_any.rx_cnt == 0);
#endif
  ctr2.rx_cnt = ctr2.rx_bytes = 0;
}
END_TEST

START_TEST(test_udp_bind)
{
  struct udp_pcb* pcb1;
  struct udp_pcb* pcb2;
  ip_addr_t ip1;
  ip_addr_t ip2;
  err_t err1;
  err_t err2;
  LWIP_UNUSED_ARG(_i);

  /* bind on same port using different IP address types */
  ip_addr_set_any_val(0, ip1);
  ip_addr_set_any_val(1, ip2);

  pcb1 = udp_new_ip_type(IPADDR_TYPE_V4);
  pcb2 = udp_new_ip_type(IPADDR_TYPE_V6);

  err1 = udp_bind(pcb1, &ip1, 2105);
  err2 = udp_bind(pcb2, &ip2, 2105);

  fail_unless(err1 == ERR_OK);
  fail_unless(err2 == ERR_OK);

  udp_remove(pcb1);
  udp_remove(pcb2);

  /* bind on same port using SAME IPv4 address type */
  ip_addr_set_any_val(0, ip1);
  ip_addr_set_any_val(0, ip2);

  pcb1 = udp_new_ip_type(IPADDR_TYPE_V4);
  pcb2 = udp_new_ip_type(IPADDR_TYPE_V4);

  err1 = udp_bind(pcb1, &ip1, 2105);
  err2 = udp_bind(pcb2, &ip2, 2105);

  fail_unless(err1 == ERR_OK);
  fail_unless(err2 == ERR_USE);

  udp_remove(pcb1);
  udp_remove(pcb2);

  /* bind on same port using SAME IPv6 address type */
  ip_addr_set_any_val(1, ip1);
  ip_addr_set_any_val(1, ip2);

  pcb1 = udp_new_ip_type(IPADDR_TYPE_V6);
  pcb2 = udp_new_ip_type(IPADDR_TYPE_V6);

  err1 = udp_bind(pcb1, &ip1, 2105);
  err2 = udp_bind(pcb2, &ip2, 2105);

  fail_unless(err1 == ERR_OK);
  fail_unless(err2 == ERR_USE);

  udp_remove(pcb1);
  udp_remove(pcb2);

  /* Bind with different IP address type */
  ip_addr_set_any_val(0, ip1);
  ip_addr_set_any_val(1, ip2);

  pcb1 = udp_new_ip_type(IPADDR_TYPE_V6);
  pcb2 = udp_new_ip_type(IPADDR_TYPE_V4);

  err1 = udp_bind(pcb1, &ip1, 2105);
  err2 = udp_bind(pcb2, &ip2, 2105);

  fail_unless(err1 == ERR_OK);
  fail_unless(err2 == ERR_OK);

  udp_remove(pcb1);
  udp_remove(pcb2);

  /* Bind with different IP numbers */
  IP_ADDR4(&ip1, 1, 2, 3, 4);
  IP_ADDR4(&ip2, 4, 3, 2, 1);

  pcb1 = udp_new_ip_type(IPADDR_TYPE_V6);
  pcb2 = udp_new_ip_type(IPADDR_TYPE_V4);

  err1 = udp_bind(pcb1, &ip1, 2105);
  err2 = udp_bind(pcb2, &ip2, 2105);

  fail_unless(err1 == ERR_OK);
  fail_unless(err2 == ERR_OK);

  udp_remove(pcb1);
  udp_remove(pcb2);

  /* Bind with same IP numbers */
  IP_ADDR4(&ip1, 1, 2, 3, 4);
  IP_ADDR4(&ip2, 1, 2, 3, 4);

  pcb1 = udp_new_ip_type(IPADDR_TYPE_V6);
  pcb2 = udp_new_ip_type(IPADDR_TYPE_V4);

  err1 = udp_bind(pcb1, &ip1, 2105);
  err2 = udp_bind(pcb2, &ip2, 2105);

  fail_unless(err1 == ERR_OK);
  fail_unless(err2 == ERR_USE);

  udp_remove(pcb1);
  udp_remove(pcb2);

  /* bind on same port using ANY + IPv4 */
  ip1 = *IP_ANY_TYPE;
  IP_ADDR4(&ip2, 1, 2, 3, 4);

  pcb1 = udp_new_ip_type(IPADDR_TYPE_ANY);
  pcb2 = udp_new_ip_type(IPADDR_TYPE_V4);

  err1 = udp_bind(pcb1, &ip1, 2105);
  err2 = udp_bind(pcb2, &ip2, 2105);

  fail_unless(err1 == ERR_OK);
  fail_unless(err2 == ERR_USE);

  udp_remove(pcb1);
  udp_remove(pcb2);
}
END_TEST

/** Create the suite including all tests for this module */
Suite *
udp_suite(void)
{
  testfunc tests[] = {
    TESTFUNC(test_udp_new_remove),
    TESTFUNC(test_udp_broadcast_rx_with_2_netifs),
    TESTFUNC(test_udp_bind)
  };
  return create_suite("UDP", tests, sizeof(tests)/sizeof(testfunc), udp_setup, udp_teardown);
}
