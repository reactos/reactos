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
 *
 * Based on XenoLinux arch/xen/kernel/evtchn.c
 * Copyright (c) 2002-2004, K A Fraser
 * 
 * This file may be distributed separately from the Linux kernel, or
 * incorporated into other software packages, subject to the following license:
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this source file (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "freeldr.h"
#include "machxen.h"

#include <rosxen.h>
#include <xen.h>
#include <ctrl_if.h>
#include <evtchn.h>

#if 2 == XEN_VER
#define TX_FULL(_c)   \
    (((_c)->tx_req_prod - XenCtrlIfTxRespCons) == CONTROL_RING_SIZE)
static control_if_t *XenCtrlIf;
static CONTROL_RING_IDX XenCtrlIfTxRespCons;
static CONTROL_RING_IDX XenCtrlIfRxReqCons;
#define RING_IDX CONTROL_RING_IDX
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

/* Primary message type -> message handler. */
static ctrl_msg_handler_t XenCtrlIfRxmsgHandler[256];


static void
XenCtrlIfNotifyController(void)
{
  notify_via_evtchn(XenCtrlIfEvtchn);
}

static void
XenCtrlIfRxmsgDefaultHandler(ctrl_msg_t *Msg, unsigned long Id)
{
  Msg->length = 0;
  XenCtrlIfSendResponse(Msg);
}

void
XenCtrlIfInit()
{
  unsigned i;
#if 2 != XEN_VER
  control_if_t *CtrlIf;
#endif /* XEN_VER */

  /*
   * Setup control interface
   */
  XenCtrlIfEvtchn = XenStartInfo->domain_controller_evtchn;

  for (i = 0;
       i < sizeof(XenCtrlIfRxmsgHandler) / sizeof(XenCtrlIfRxmsgHandler[0]);
       i++)
    {
      XenCtrlIfRxmsgHandler[i] = XenCtrlIfRxmsgDefaultHandler;
    }

#if 2 == XEN_VER
  XenCtrlIf = ((control_if_t *)((char *)XenSharedInfo + 2048));

  /* Sync up with shared indexes. */
  XenCtrlIfTxRespCons = XenCtrlIf->tx_resp_prod;
  XenCtrlIfRxReqCons  = XenCtrlIf->rx_resp_prod;
#else /* XEN_VER */
  CtrlIf = ((control_if_t *)((char *)XenSharedInfo + 2048));

  /* Sync up with shared indexes. */
  FRONT_RING_ATTACH(&XenCtrlIfTxRing, &CtrlIf->tx_ring, CONTROL_RING_MEM);
  BACK_RING_ATTACH(&XenCtrlIfRxRing, &CtrlIf->rx_ring, CONTROL_RING_MEM);
#endif /* XEN_VER */

  XenEvtchnRegisterCtrlIf(XenCtrlIfEvtchn);
}

BOOL
XenCtrlIfSendMessageNoblock(ctrl_msg_t *Msg)
{
  ctrl_msg_t *Dest;

#if 2 == XEN_VER
  if (TX_FULL(XenCtrlIf))
#else /* XEN_VER */
  if (RING_FULL(&XenCtrlIfTxRing))
#endif /* XEN_VER */
    {
      return FALSE;
    }

#if 2 == XEN_VER
  Dest = XenCtrlIf->tx_ring + MASK_CONTROL_IDX(XenCtrlIf->tx_req_prod);
#else /* XEN_VER */
  Dest = RING_GET_REQUEST(&XenCtrlIfTxRing,
                          XenCtrlIfTxRing.req_prod_pvt);
#endif /* XEN_VER */
  memcpy(Dest, Msg, sizeof(ctrl_msg_t));
#if 2 == XEN_VER
  XenCtrlIf->tx_req_prod++;
#else /* XEN_VER */
  XenCtrlIfTxRing.req_prod_pvt++;
  RING_PUSH_REQUESTS(&XenCtrlIfTxRing);
#endif /* XEN_VER */
  XenCtrlIfNotifyController();

  return TRUE;
}

VOID
XenCtrlIfSendMessageBlock(ctrl_msg_t *Msg)
{
  while (1)
    {
      XenEvtchnDisableEvents();
      if (XenCtrlIfSendMessageNoblock(Msg))
        {
          XenEvtchnEnableEvents();
          return;
        }
      HYPERVISOR_block();
    }
}

VOID
XenCtrlIfDiscardResponses()
{
#if 2 == XEN_VER
  XenCtrlIfTxRespCons = XenCtrlIf->tx_resp_prod;
#else /* XEN_VER */
  RING_DROP_PENDING_RESPONSES(&XenCtrlIfTxRing);
#endif /* XEN_VER */
}

BOOL
XenCtrlIfTransmitterEmpty()
{
#if 2 == XEN_VER
  return (XenCtrlIf->tx_req_prod == XenCtrlIfTxRespCons);
#else /* XEN_VER */
  return XenCtrlIfTxRing.sring->req_prod == XenCtrlIfTxRing.rsp_cons;
#endif /* XEN_VER */
}

VOID
XenCtrlIfSendResponse(ctrl_msg_t *Msg)
{
  ctrl_msg_t   *Dmsg;

  /*
   * NB. The response may the original request message, modified in-place.
   * In this situation we may have src==dst, so no copying is required.
   */

#if 2 == XEN_VER
  Dmsg = &XenCtrlIf->rx_ring[MASK_CONTROL_IDX(XenCtrlIf->rx_resp_prod)];
#else /* XEN_VER */
  Dmsg = RING_GET_RESPONSE(&XenCtrlIfRxRing, XenCtrlIfRxRing.rsp_prod_pvt);
#endif /* XEN_VER */
  if (Dmsg != Msg )
    {
      memcpy(Dmsg, Msg, sizeof(ctrl_msg_t));
    }

  wmb(); /* Write the message before letting the controller peek at it. */
#if 2 == XEN_VER
  XenCtrlIf->rx_resp_prod++;
#else /* XEN_VER */
  XenCtrlIfRxRing.rsp_prod_pvt++;
  RING_PUSH_RESPONSES(&XenCtrlIfRxRing);
#endif /* XEN_VER */

  XenCtrlIfNotifyController();
}

VOID
XenCtrlIfRegisterReceiver(u8 Type, ctrl_msg_handler_t Hnd)
{
  XenCtrlIfRxmsgHandler[Type] = Hnd;
}

static VOID
XenHandleResponses()
{
#if 2 == XEN_VER
  XenCtrlIfTxRespCons = XenCtrlIf->tx_resp_prod;
#else /* XEN_VER */
  XenCtrlIfTxRing.rsp_cons = XenCtrlIfTxRing.sring->rsp_prod;
#endif /* XEN_VER */
}

static VOID
XenHandleRequests()
{
  ctrl_msg_t Msg, *Pmsg;
  RING_IDX Rp, i;

#if 2 == XEN_VER
  i  = XenCtrlIfRxReqCons;
  Rp = XenCtrlIf->rx_req_prod;
#else /* XEN_VER */
  i  = XenCtrlIfRxRing.req_cons;
  Rp = XenCtrlIfRxRing.sring->req_prod;
#endif /* XEN_VER */
 
  for ( ; i != Rp; i++) 
    {
#if 2 == XEN_VER
      Pmsg = &XenCtrlIf->rx_ring[MASK_CONTROL_IDX(XenCtrlIfRxReqCons++)];
#else /* XEN_VER */
      Pmsg = RING_GET_REQUEST(&XenCtrlIfRxRing, i);
#endif /* XEN_VER */
      memcpy(&Msg, Pmsg, offsetof(ctrl_msg_t, msg));

      if (sizeof(Msg.msg) < Msg.length)
        {
          Msg.length = sizeof(Msg.msg);
        }
        
      if (0 != Msg.length)
        {
          memcpy(Msg.msg, Pmsg->msg, Msg.length);
        }

      (*XenCtrlIfRxmsgHandler[Msg.type])(&Msg, 0);
    }

#if 2 == XEN_VER
  XenCtrlIfRxReqCons = i;
#else /* XEN_VER */
  XenCtrlIfRxRing.req_cons = i;
#endif /* XEN_VER */
}

VOID
XenCtrlIfHandleEvent()
{
#if 2 == XEN_VER
  if (XenCtrlIfTxRespCons != XenCtrlIf->tx_resp_prod)
#else /* XEN_VER */
  if (RING_HAS_UNCONSUMED_RESPONSES(&XenCtrlIfTxRing))
#endif /* XEN_VER */
    {
      XenHandleResponses();
    }

#if 2 == XEN_VER
  if (XenCtrlIfRxReqCons != XenCtrlIf->rx_req_prod )
#else /* XEN_VER */
  if (RING_HAS_UNCONSUMED_REQUESTS(&XenCtrlIfRxRing))
#endif /* XEN_VER */
    {
      XenHandleRequests();
    }
}

/* EOF */
