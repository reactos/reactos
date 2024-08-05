#ifndef LWIP_HDR_TEST_PPPOS_H
#define LWIP_HDR_TEST_PPPOS_H

#include "../lwip_check.h"
#include "netif/ppp/ppp.h"

#if PPP_SUPPORT && PPPOS_SUPPORT

Suite* pppos_suite(void);

#endif /* PPP_SUPPORT && PPPOS_SUPPORT */

#endif /* LWIP_HDR_TEST_PPPOS_H */
