/*
 *  FreeLoader
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "freeldr.h"
#include "machxen.h"

#include <rosxen.h>
#include <xen.h>
#include <ctrl_if.h>
#include <evtchn.h>

#if XEN_VER == 2
#define TX_FULL(_c)   \
    (((_c)->tx_req_prod - XenCtrlIfTxRespCons) == CONTROL_RING_SIZE)
static control_if_t *XenCtrlIf;
static CONTROL_RING_IDX XenCtrlIfTxRespCons;
static CONTROL_RING_IDX XenCtrlIfRxReqCons;
#else /* XEN_VER */
/*
 * Extra ring macros to sync a consumer index up to the public producer index.
 * Generally UNSAFE, but we use it for recovery and shutdown in some cases.
 */
#define RING_DROP_PENDING_RESPONSES(_r)                                 \
    do {                                                                \
        (_r)->rsp_cons = (_r)->sring->rsp_prod;                         \
    } while (0)

static ctrl_front_ring_t XenCtrlIfTxRing;
static ctrl_back_ring_t XenCtrlIfRxRing;
#endif /* XEN_VER */
static int XenCtrlIfEvtchn;

void
XenCtrlIfInit()
{
#if XEN_VER != 2
  control_if_t *CtrlIf;
#endif /* XEN_VER */

  /*
   * Setup control interface
   */
  XenCtrlIfEvtchn = XenStartInfo->domain_controller_evtchn;

#if XEN_VER == 2
  XenCtrlIf = ((control_if_t *)((char *)XenSharedInfo + 2048));

  /* Sync up with shared indexes. */
  XenCtrlIfTxRespCons = XenCtrlIf->tx_resp_prod;
  XenCtrlIfRxReqCons  = XenCtrlIf->rx_resp_prod;
#else /* XEN_VER */
  CtrlIf = ((control_if_t *)((char *)XenSharedInfo + 2048));

  /* Sync up with shared indexes. */
  FRONT_RING_ATTACH(&XenCtrlIfTxRing, &CtrlIf->tx_ring);
  BACK_RING_ATTACH(&XenCtrlIfRxRing, &CtrlIf->rx_ring);
#endif /* XEN_VER */
}

BOOL
XenCtrlIfSendMessageNoblock(ctrl_msg_t *Msg)
{
  ctrl_msg_t *Dest;

#if XEN_VER == 2
  if (TX_FULL(XenCtrlIf))
#else /* XEN_VER */
  if (RING_FULL(&XenCtrlIfTxRing))
#endif /* XEN_VER */
    {
      return FALSE;
    }

#if XEN_VER == 2
  Dest = XenCtrlIf->tx_ring + MASK_CONTROL_IDX(XenCtrlIf->tx_req_prod);
#else /* XEN_VER */
  Dest = RING_GET_REQUEST(&XenCtrlIfTxRing,
                          XenCtrlIfTxRing.req_prod_pvt);
#endif /* XEN_VER */
  memcpy(Dest, Msg, sizeof(ctrl_msg_t));
#if XEN_VER == 2
  XenCtrlIf->tx_req_prod++;
#else /* XEN_VER */
  XenCtrlIfTxRing.req_prod_pvt++;
  RING_PUSH_REQUESTS(&XenCtrlIfTxRing);
#endif /* XEN_VER */
  notify_via_evtchn(XenCtrlIfEvtchn);

  return TRUE;
}

VOID
XenCtrlIfDiscardResponses()
{
#if XEN_VER == 2
  XenCtrlIfTxRespCons = XenCtrlIf->tx_resp_prod;
#else /* XEN_VER */
  RING_DROP_PENDING_RESPONSES(&XenCtrlIfTxRing);
#endif /* XEN_VER */
}

BOOL
XenCtrlIfTransmitterEmpty()
{
#if XEN_VER == 2
  return (XenCtrlIf->tx_req_prod == XenCtrlIfTxRespCons);
#else /* XEN_VER */
  return XenCtrlIfTxRing.sring->req_prod == XenCtrlIfTxRing.rsp_cons;
#endif /* XEN_VER */
}

/* EOF */
