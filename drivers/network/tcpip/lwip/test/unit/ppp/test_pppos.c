#include "test_pppos.h"

#include "lwip/netif.h"
#include "netif/ppp/pppos.h"
#include "netif/ppp/ppp.h"

#if PPP_SUPPORT && PPPOS_SUPPORT
static struct netif pppos_netif;
static ppp_pcb *ppp;

static u32_t ppp_output_cb(ppp_pcb *pcb, const void *data, u32_t len, void *ctx)
{
  LWIP_UNUSED_ARG(pcb);
  LWIP_UNUSED_ARG(data);
  LWIP_UNUSED_ARG(len);
  LWIP_UNUSED_ARG(ctx);

  return  0;
}

static void ppp_link_status_cb(ppp_pcb *pcb, int err_code, void *ctx)
{
  LWIP_UNUSED_ARG(pcb);
  LWIP_UNUSED_ARG(err_code);
  LWIP_UNUSED_ARG(ctx);
}

static void pppos_setup(void)
{
  ppp = pppos_create(&pppos_netif, ppp_output_cb, ppp_link_status_cb, NULL);
  fail_if(ppp == NULL);
  ppp_connect(ppp, 0);
}

static void pppos_teardown(void)
{
}

START_TEST(test_pppos_empty_packet_with_valid_fcs)
{
  u8_t two_breaks[] = { 0x7e, 0, 0, 0x7e };
  u8_t other_packet[] = { 0x7e, 0x7d, 0x20, 0x00, 0x7e };
  /* Set internal states of the underlying pcb */
  pppos_pcb *pppos = (pppos_pcb *)ppp->link_ctx_cb;
 
  LWIP_UNUSED_ARG(_i);

  pppos->open = 1;  /* Pretend the connection is open already */
  pppos->in_accm[0] = 0xf0;  /* Make sure 0x0's are not escaped chars */

  pppos_input(ppp, two_breaks, sizeof(two_breaks));
  pppos_input(ppp, other_packet, sizeof(other_packet));

}
END_TEST

/** Create the suite including all tests for this module */
Suite *
pppos_suite(void)
{
  testfunc tests[] = {
    TESTFUNC(test_pppos_empty_packet_with_valid_fcs)
  };
  return create_suite("PPPOS", tests, sizeof(tests)/sizeof(testfunc), pppos_setup, pppos_teardown);
}

#endif /* PPP_SUPPORT && PPPOS_SUPPORT */
