/* $Id: machpc.c 12672 2005-01-01 00:42:18Z chorns $
 *
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

/*
 * Extra ring macros to sync a consumer index up to the public producer index.
 * Generally UNSAFE, but we use it for recovery and shutdown in some cases.
 */
#define RING_DROP_PENDING_RESPONSES(_r)                                 \
    do {                                                                \
        (_r)->rsp_cons = (_r)->sring->rsp_prod;                         \
    } while (0)

#define OUTPUT_BUFFER_SIZE 128
static char OutputBuffer[OUTPUT_BUFFER_SIZE];
static unsigned OutputPtr = 0;

static void
FlushOutput()
{
  ctrl_msg_t *msg;

  while (0 != OutputPtr)
    {
      RING_DROP_PENDING_RESPONSES(&XenCtrlIfTxRing);

      if (! RING_FULL(&XenCtrlIfTxRing))
        {
          /*
           * Put message on the control interface ring and trigger virtual
           * console writer.
           */
          msg = RING_GET_REQUEST(&XenCtrlIfTxRing,
                                  XenCtrlIfTxRing.req_prod_pvt);

          msg->type = CMSG_CONSOLE;
          msg->subtype = CMSG_CONSOLE_DATA;
          msg->length = OutputPtr;
          memcpy(msg->msg, OutputBuffer, OutputPtr);
          msg->id      = 0xff;
          XenCtrlIfTxRing.req_prod_pvt++;
          RING_PUSH_REQUESTS(&XenCtrlIfTxRing);
          notify_via_evtchn(XenCtrlIfEvtchn);

          OutputPtr = 0;
        }
    }
}

static VOID
PutCharInBuffer(int Ch)
{
  if (OUTPUT_BUFFER_SIZE <= OutputPtr)
    {
      FlushOutput();
    }
  OutputBuffer[OutputPtr++] = Ch;
}

VOID
XenConsPutChar(int Ch)
{
  if ('\n' == Ch)
    {
      PutCharInBuffer('\r');
      PutCharInBuffer('\n');
      FlushOutput();
    }
  else
    {
      PutCharInBuffer(Ch);
    }
}

/* EOF */
