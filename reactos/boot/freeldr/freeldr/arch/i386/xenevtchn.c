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
#include "rtl.h"

#define asmlinkage __attribute__((regparm(0)))

static int XenCtrlIfIndex = -1;
static u32 XenCtrlIfBit;
static int XenDiskIndex = -1;
static u32 XenDiskBit;

static void
XenEvtchnSetMask()
{
  unsigned i;

  /* Mask all event channels except the one we're interested in */
  for (i = 0;
       i < sizeof(XenSharedInfo->evtchn_mask)
           / sizeof(XenSharedInfo->evtchn_mask[0]);
       i++)
    {
      if (i == XenCtrlIfIndex)
        {
          if (i == XenDiskIndex)
            {
              XenSharedInfo->evtchn_mask[i] = ~ (XenCtrlIfBit | XenDiskBit);
            }
          else
            {
              XenSharedInfo->evtchn_mask[i] = ~ XenCtrlIfBit;
            }
        }
      else if (i == XenDiskIndex)
        {
          XenSharedInfo->evtchn_mask[i] = ~ XenDiskBit;
        }
      else
        {
          XenSharedInfo->evtchn_mask[i] = ~ 0;
        }
    }
}

VOID
XenEvtchnRegisterCtrlIf(unsigned CtrlIfEvtchn)
{
  XenCtrlIfIndex = CtrlIfEvtchn >> 5;
  XenCtrlIfBit = 1 << (CtrlIfEvtchn & 0x1f);

  XenEvtchnSetMask();
}

VOID
XenEvtchnRegisterDisk(unsigned DiskEvtchn)
{
  XenDiskIndex = DiskEvtchn >> 5;
  XenDiskBit = 1 << (DiskEvtchn & 0x1f);

  XenEvtchnSetMask();
}

/* NB. Event delivery is disabled on entry. */
asmlinkage void
XenEvtchnDoUpcall(struct pt_regs *Regs)
{
  XenSharedInfo->vcpu_data[0].evtchn_upcall_pending = 0;
#if 2 == XEN_VER
  XenSharedInfo->evtchn_pending_sel = 0;
#else /* XEN_VER */
  XenSharedInfo->vcpu_data[0].evtchn_pending_sel = 0;
#endif /* XEN_VER */

  while ((0 <= XenDiskIndex
          && 0 != (XenSharedInfo->evtchn_pending[XenDiskIndex]
                   & ~ XenSharedInfo->evtchn_mask[XenDiskIndex]
                   & XenDiskBit)) ||
         (0 <= XenCtrlIfIndex
          && 0 != (XenSharedInfo->evtchn_pending[XenCtrlIfIndex]
                   & ~ XenSharedInfo->evtchn_mask[XenCtrlIfIndex]
                   & XenCtrlIfBit)))
    {
      if (0 <= XenDiskIndex
          && 0 != (XenSharedInfo->evtchn_pending[XenDiskIndex]
                   & ~ XenSharedInfo->evtchn_mask[XenDiskIndex]
                   & XenDiskBit))
        {
          XenSharedInfo->evtchn_pending[XenDiskIndex] &= ~ XenDiskBit;
          XenDiskHandleEvent();
        }
      if (0 <= XenCtrlIfIndex
          && 0 != (XenSharedInfo->evtchn_pending[XenCtrlIfIndex]
                   & ~ XenSharedInfo->evtchn_mask[XenCtrlIfIndex]
                   & XenCtrlIfBit))
        {
          XenSharedInfo->evtchn_pending[XenCtrlIfIndex] &= ~ XenCtrlIfBit;
          XenCtrlIfHandleEvent();
        }
    }
}

VOID XenEvtchnDisableEvents()
{
  XenSharedInfo->vcpu_data[0].evtchn_upcall_mask = 1;
}

VOID XenEvtchnEnableEvents()
{
  XenSharedInfo->vcpu_data[0].evtchn_upcall_mask = 0;
}

/* EOF */
