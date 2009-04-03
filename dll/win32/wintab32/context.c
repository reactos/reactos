/*
 * Tablet Context
 *
 * Copyright 2002 Patrik Stridvall
 * Copyright 2003 CodeWeavers, Aric Stewart
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "windef.h"
#include "winerror.h"
#include "winbase.h"
#include "winuser.h"
#include "winnls.h"

#include "wintab.h"
#include "wintab_internal.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wintab32);

/*
 * Documentation found at
 * http://www.csl.sony.co.jp/projects/ar/restricted/wintabl.html
 */

static BOOL gLoaded;
static LPOPENCONTEXT gOpenContexts;
static HCTX gTopContext = (HCTX)0xc00;

static void LOGCONTEXTAtoW(const LOGCONTEXTA *in, LOGCONTEXTW *out)
{
    MultiByteToWideChar(CP_ACP, 0, in->lcName, -1, out->lcName, LCNAMELEN);
    out->lcName[LCNAMELEN - 1] = 0;
    /* we use the fact that the fields after lcName are the same in LOGCONTEXTA and W */
    memcpy(&out->lcOptions, &in->lcOptions, sizeof(LOGCONTEXTA) - FIELD_OFFSET(LOGCONTEXTA, lcOptions));
}

static void LOGCONTEXTWtoA(const LOGCONTEXTW *in, LOGCONTEXTA *out)
{
    WideCharToMultiByte(CP_ACP, 0, in->lcName, LCNAMELEN, out->lcName, LCNAMELEN, NULL, NULL);
    out->lcName[LCNAMELEN - 1] = 0;
    /* we use the fact that the fields after lcName are the same in LOGCONTEXTA and W */
    memcpy(&out->lcOptions, &in->lcOptions, sizeof(LOGCONTEXTW) - FIELD_OFFSET(LOGCONTEXTW, lcOptions));
}

static BOOL is_logcontext_category(UINT wCategory)
{
    return wCategory == WTI_DEFSYSCTX || wCategory == WTI_DEFCONTEXT || wCategory == WTI_DDCTXS;
}

static BOOL is_string_field(UINT wCategory, UINT nIndex)
{
    if (wCategory == WTI_INTERFACE && nIndex == IFC_WINTABID)
        return TRUE;
    if (is_logcontext_category(wCategory) && nIndex == CTX_NAME)
        return TRUE;
    if ((wCategory >= WTI_CURSORS && wCategory <= WTI_CURSORS + 9) &&
            (nIndex == CSR_NAME || nIndex == CSR_BTNNAMES))
        return TRUE;
    if (wCategory == WTI_DEVICES && (nIndex == DVC_NAME || nIndex == DVC_PNPID))
        return TRUE;
    return FALSE;
}

static const char* DUMPBITS(int x)
{
    char buf[200];
    buf[0] = 0;
    if (x&PK_CONTEXT) strcat(buf,"PK_CONTEXT ");
    if (x&PK_STATUS) strcat(buf, "PK_STATUS ");
    if (x&PK_TIME) strcat(buf, "PK_TIME ");
    if (x&PK_CHANGED) strcat(buf, "PK_CHANGED ");
    if (x&PK_SERIAL_NUMBER) strcat(buf, "PK_SERIAL_NUMBER ");
    if (x&PK_CURSOR) strcat(buf, "PK_CURSOR ");
    if (x&PK_BUTTONS) strcat(buf, "PK_BUTTONS ");
    if (x&PK_X) strcat(buf, "PK_X ");
    if (x&PK_Y) strcat(buf, "PK_Y ");
    if (x&PK_Z) strcat(buf, "PK_Z ");
    if (x&PK_NORMAL_PRESSURE) strcat(buf, "PK_NORMAL_PRESSURE ");
    if (x&PK_TANGENT_PRESSURE) strcat(buf, "PK_TANGENT_PRESSURE ");
    if (x&PK_ORIENTATION) strcat(buf, "PK_ORIENTATION ");
    if (x&PK_ROTATION) strcat(buf, "PK_ROTATION ");
    return wine_dbg_sprintf("{%s}",buf);
}

static inline void DUMPPACKET(WTPACKET packet)
{
    TRACE("pkContext: %p pkStatus: 0x%x pkTime : 0x%x pkChanged: 0x%x pkSerialNumber: 0x%x pkCursor : %i pkButtons: %x pkX: %i pkY: %i pkZ: %i pkNormalPressure: %i pkTangentPressure: %i pkOrientation: (%i,%i,%i) pkRotation: (%i,%i,%i)\n",
          packet.pkContext, packet.pkStatus, packet.pkTime, packet.pkChanged, packet.pkSerialNumber,
          packet.pkCursor, packet.pkButtons, packet.pkX, packet.pkY, packet.pkZ,
          packet.pkNormalPressure, packet.pkTangentPressure,
          packet.pkOrientation.orAzimuth, packet.pkOrientation.orAltitude, packet.pkOrientation.orTwist,
          packet.pkRotation.roPitch, packet.pkRotation.roRoll, packet.pkRotation.roYaw);
}

static inline void DUMPCONTEXT(LOGCONTEXTW lc)
{
    TRACE("Name: %s, Options: %x, Status: %x, Locks: %x, MsgBase: %x, "
          "Device: %x, PktRate: %x, "
          "%x%s, %x%s, %x%s, "
          "BtnDnMask: %x, BtnUpMask: %x, "
          "InOrgX: %i, InOrgY: %i, InOrgZ: %i, "
          "InExtX: %i, InExtY: %i, InExtZ: %i, "
          "OutOrgX: %i, OutOrgY: %i, OutOrgZ: %i, "
          "OutExtX: %i, OutExtY: %i, OutExtZ: %i, "
          "SensX: %i, SensY: %i, SensZ: %i, "
          "SysMode: %i, "
          "SysOrgX: %i, SysOrgY: %i, "
          "SysExtX: %i, SysExtY: %i, "
          "SysSensX: %i, SysSensY: %i\n",
          wine_dbgstr_w(lc.lcName), lc.lcOptions, lc.lcStatus, lc.lcLocks, lc.lcMsgBase,
          lc.lcDevice, lc.lcPktRate, lc.lcPktData, DUMPBITS(lc.lcPktData),
          lc.lcPktMode, DUMPBITS(lc.lcPktMode), lc.lcMoveMask,
          DUMPBITS(lc.lcMoveMask), lc.lcBtnDnMask, lc.lcBtnUpMask,
          lc.lcInOrgX, lc.lcInOrgY, lc.lcInOrgZ, lc.lcInExtX, lc.lcInExtY,
          lc.lcInExtZ, lc.lcOutOrgX, lc.lcOutOrgY, lc.lcOutOrgZ, lc.lcOutExtX,
          lc.lcOutExtY, lc.lcOutExtZ, lc.lcSensX, lc.lcSensY, lc.lcSensZ, lc.lcSysMode,
          lc.lcSysOrgX, lc.lcSysOrgY, lc.lcSysExtX, lc.lcSysExtY, lc.lcSysSensX,
          lc.lcSysSensY);
}


/* Find an open context given the handle */
static LPOPENCONTEXT TABLET_FindOpenContext(HCTX hCtx)
{
    LPOPENCONTEXT ptr = gOpenContexts;
    while (ptr)
    {
        if (ptr->handle == hCtx) return ptr;
        ptr = ptr->next;
    }
    return NULL;
}

static void LoadTablet(void)
{
    TRACE("Initializing the tablet to hwnd %p\n",hwndDefault);
    gLoaded= TRUE;
    pLoadTabletInfo(hwndDefault);
}

int TABLET_PostTabletMessage(LPOPENCONTEXT newcontext, UINT msg, WPARAM wParam,
                             LPARAM lParam, BOOL send_always)
{
    if ((send_always) || (newcontext->context.lcOptions & CXO_MESSAGES))
    {
        TRACE("Posting message %x to %p\n",msg, newcontext->hwndOwner);
        return PostMessageA(newcontext->hwndOwner, msg, wParam, lParam);
    }
    return 0;
}

static inline DWORD ScaleForContext(DWORD In, LONG InOrg, LONG InExt, LONG OutOrg, LONG OutExt)
{
    if (((InExt > 0 )&&(OutExt > 0)) || ((InExt<0) && (OutExt < 0)))
        return ((In - InOrg) * abs(OutExt) / abs(InExt)) + OutOrg;
    else
        return ((abs(InExt) - (In - InOrg))*abs(OutExt) / abs(InExt)) + OutOrg;
}

LPOPENCONTEXT AddPacketToContextQueue(LPWTPACKET packet, HWND hwnd)
{
    LPOPENCONTEXT ptr=NULL;

    EnterCriticalSection(&csTablet);

    ptr = gOpenContexts;
    while (ptr)
    {
        TRACE("Trying Queue %p (%p %p)\n", ptr->handle, hwnd, ptr->hwndOwner);

        if (ptr->hwndOwner == hwnd)
        {
            int tgt;
            if (!ptr->enabled)
            {
                ptr = ptr->next;
                continue;
            }

            tgt = ptr->PacketsQueued;

            packet->pkContext = ptr->handle;

            /* translate packet data to the context */

            /* Scale  as per documentation */
            packet->pkY = ScaleForContext(packet->pkY, ptr->context.lcInOrgY,
                                ptr->context.lcInExtY, ptr->context.lcOutOrgY,
                                ptr->context.lcOutExtY);

            packet->pkX = ScaleForContext(packet->pkX, ptr->context.lcInOrgX,
                                ptr->context.lcInExtX, ptr->context.lcOutOrgX,
                                ptr->context.lcOutExtX);

            /* flip the Y axis */
            if (ptr->context.lcOutExtY > 0)
                packet->pkY = ptr->context.lcOutExtY - packet->pkY;
            else if (ptr->context.lcOutExtY < 0)
                packet->pkY = abs(ptr->context.lcOutExtY + packet->pkY);

            DUMPPACKET(*packet);

            if (tgt == ptr->QueueSize)
            {
                TRACE("Queue Overflow %p\n",ptr->handle);
                ptr->PacketQueue[tgt-1].pkStatus |= TPS_QUEUE_ERR;
            }
            else
            {
                TRACE("Placed in queue %p index %i\n",ptr->handle,tgt);
                ptr->PacketQueue[tgt] = *packet;
                ptr->PacketsQueued++;

                if (ptr->ActiveCursor != packet->pkCursor)
                {
                    ptr->ActiveCursor = packet->pkCursor;
                    if (ptr->context.lcOptions & CXO_CSRMESSAGES)
                        TABLET_PostTabletMessage(ptr, _WT_CSRCHANGE(ptr->context.lcMsgBase),
                          (WPARAM)packet->pkSerialNumber, (LPARAM)ptr->handle,
                            FALSE);
                }
            }
            break;
         }
        ptr = ptr->next;
    }
    LeaveCriticalSection(&csTablet);
    TRACE("Done (%p)\n",ptr);
    return ptr;
}

/*
 * Flushes all packets from the queue.
 */
static inline void TABLET_FlushQueue(LPOPENCONTEXT context)
{
    context->PacketsQueued = 0;
}

static inline int CopyTabletData(LPVOID target, LPVOID src, INT size)
{
    memcpy(target,src,size);
    return(size);
}

static INT TABLET_FindPacket(LPOPENCONTEXT context, UINT wSerial,
                                LPWTPACKET *pkt)
{
    int loop;
    int index  = -1;
    for (loop = 0; loop < context->PacketsQueued; loop++)
        if (context->PacketQueue[loop].pkSerialNumber == wSerial)
        {
            index = loop;
            *pkt = &context->PacketQueue[loop];
            break;
        }

    TRACE("%i .. %i\n",context->PacketsQueued,index);

    return index;
}


static LPVOID TABLET_CopyPacketData(LPOPENCONTEXT context, LPVOID lpPkt,
                                    LPWTPACKET wtp)
{
    LPBYTE ptr;

    ptr = lpPkt;
    TRACE("Packet Bits %s\n",DUMPBITS(context->context.lcPktData));

    if (context->context.lcPktData & PK_CONTEXT)
        ptr+=CopyTabletData(ptr,&wtp->pkContext,sizeof(HCTX));
    if (context->context.lcPktData & PK_STATUS)
        ptr+=CopyTabletData(ptr,&wtp->pkStatus,sizeof(UINT));
    if (context->context.lcPktData & PK_TIME)
        ptr+=CopyTabletData(ptr,&wtp->pkTime,sizeof(LONG));
    if (context->context.lcPktData & PK_CHANGED)
        ptr+=CopyTabletData(ptr,&wtp->pkChanged,sizeof(WTPKT));
    if (context->context.lcPktData & PK_SERIAL_NUMBER)
        ptr+=CopyTabletData(ptr,&wtp->pkChanged,sizeof(UINT));
    if (context->context.lcPktData & PK_CURSOR)
        ptr+=CopyTabletData(ptr,&wtp->pkCursor,sizeof(UINT));
    if (context->context.lcPktData & PK_BUTTONS)
        ptr+=CopyTabletData(ptr,&wtp->pkButtons,sizeof(DWORD));
    if (context->context.lcPktData & PK_X)
        ptr+=CopyTabletData(ptr,&wtp->pkX,sizeof(DWORD));
    if (context->context.lcPktData & PK_Y)
        ptr+=CopyTabletData(ptr,&wtp->pkY,sizeof(DWORD));
    if (context->context.lcPktData & PK_Z)
        ptr+=CopyTabletData(ptr,&wtp->pkZ,sizeof(DWORD));
    if (context->context.lcPktData & PK_NORMAL_PRESSURE)
        ptr+=CopyTabletData(ptr,&wtp->pkNormalPressure,sizeof(UINT));
    if (context->context.lcPktData & PK_TANGENT_PRESSURE)
        ptr+=CopyTabletData(ptr,&wtp->pkTangentPressure,sizeof(UINT));
    if (context->context.lcPktData & PK_ORIENTATION)
        ptr+=CopyTabletData(ptr,&wtp->pkOrientation,sizeof(ORIENTATION));
    if (context->context.lcPktData & PK_ROTATION)
        ptr+=CopyTabletData(ptr,&wtp->pkRotation,sizeof(ROTATION));

    /*TRACE("Copied %i bytes\n",(INT)ptr - (INT)lpPkt); */
    return ptr;
}

static VOID TABLET_BlankPacketData(LPOPENCONTEXT context, LPVOID lpPkt, INT n)
{
    int rc = 0;

    if (context->context.lcPktData & PK_CONTEXT)
        rc +=sizeof(HCTX);
    if (context->context.lcPktData & PK_STATUS)
        rc +=sizeof(UINT);
    if (context->context.lcPktData & PK_TIME)
        rc += sizeof(LONG);
    if (context->context.lcPktData & PK_CHANGED)
        rc += sizeof(WTPKT);
    if (context->context.lcPktData & PK_SERIAL_NUMBER)
        rc += sizeof(UINT);
    if (context->context.lcPktData & PK_CURSOR)
        rc += sizeof(UINT);
    if (context->context.lcPktData & PK_BUTTONS)
        rc += sizeof(DWORD);
    if (context->context.lcPktData & PK_X)
        rc += sizeof(DWORD);
    if (context->context.lcPktData & PK_Y)
        rc += sizeof(DWORD);
    if (context->context.lcPktData & PK_Z)
        rc += sizeof(DWORD);
    if (context->context.lcPktData & PK_NORMAL_PRESSURE)
        rc += sizeof(UINT);
    if (context->context.lcPktData & PK_TANGENT_PRESSURE)
        rc += sizeof(UINT);
    if (context->context.lcPktData & PK_ORIENTATION)
        rc += sizeof(ORIENTATION);
    if (context->context.lcPktData & PK_ROTATION)
        rc += sizeof(ROTATION);

    rc *= n;
    memset(lpPkt,0,rc);
}


static UINT WTInfoT(UINT wCategory, UINT nIndex, LPVOID lpOutput, BOOL bUnicode)
{
    UINT result;

    TRACE("(%d, %d, %p, %d)\n", wCategory, nIndex, lpOutput, bUnicode);
    if (gLoaded == FALSE)
         LoadTablet();

    /*
     *  Handle system extents here, as we can use user32.dll code to set them.
     */
    if(wCategory == WTI_DEFSYSCTX)
    {
        switch(nIndex)
        {
            case CTX_SYSEXTX:
                if(lpOutput != NULL)
                    *(LONG*)lpOutput = GetSystemMetrics(SM_CXSCREEN);
                return sizeof(LONG);
            case CTX_SYSEXTY:
                if(lpOutput != NULL)
                    *(LONG*)lpOutput = GetSystemMetrics(SM_CYSCREEN);
                return sizeof(LONG);
           /* No action, delegate to X11Drv */
        }
    }

    if (is_logcontext_category(wCategory) && nIndex == 0)
    {
        if (lpOutput)
        {
            LOGCONTEXTW buf;
            pWTInfoW(wCategory, nIndex, &buf);

            /*  Handle system extents here, as we can use user32.dll code to set them */
            if(wCategory == WTI_DEFSYSCTX && nIndex == 0)
            {
                buf.lcSysExtX = GetSystemMetrics(SM_CXSCREEN);
                buf.lcSysExtY = GetSystemMetrics(SM_CYSCREEN);
            }

            if (bUnicode)
                memcpy(lpOutput, &buf, sizeof(buf));
            else
                LOGCONTEXTWtoA(&buf, lpOutput);
        }

        result = bUnicode ? sizeof(LOGCONTEXTW) : sizeof(LOGCONTEXTA);
    }
    else if (is_string_field(wCategory, nIndex) && !bUnicode)
    {
        int size = pWTInfoW(wCategory, nIndex, NULL);
        WCHAR *buf = HeapAlloc(GetProcessHeap(), 0, size);
        pWTInfoW(wCategory, nIndex, buf);
        result = WideCharToMultiByte(CP_ACP, 0, buf, size/sizeof(WCHAR), lpOutput, lpOutput ? 2*size : 0, NULL, NULL);
        HeapFree(GetProcessHeap(), 0, buf);
    }
    else
        result =  pWTInfoW(wCategory, nIndex, lpOutput);

    TRACE("returns %d\n", result);
    return result;
}

/***********************************************************************
 *		WTInfoA (WINTAB32.20)
 */
UINT WINAPI WTInfoA(UINT wCategory, UINT nIndex, LPVOID lpOutput)
{
    return WTInfoT(wCategory, nIndex, lpOutput, FALSE);
}


/***********************************************************************
 *		WTInfoW (WINTAB32.1020)
 */
UINT WINAPI WTInfoW(UINT wCategory, UINT nIndex, LPVOID lpOutput)
{
    return WTInfoT(wCategory, nIndex, lpOutput, TRUE);
}

/***********************************************************************
 *		WTOpenW (WINTAB32.2021)
 */
HCTX WINAPI WTOpenW(HWND hWnd, LPLOGCONTEXTW lpLogCtx, BOOL fEnable)
{
    LPOPENCONTEXT newcontext;

    TRACE("hWnd=%p, lpLogCtx=%p, fEnable=%u\n", hWnd, lpLogCtx, fEnable);
    DUMPCONTEXT(*lpLogCtx);

    newcontext = HeapAlloc(GetProcessHeap(), 0 , sizeof(OPENCONTEXT));
    newcontext->context = *lpLogCtx;
    newcontext->hwndOwner = hWnd;
    newcontext->ActiveCursor = -1;
    newcontext->QueueSize = 10;
    newcontext->PacketsQueued = 0;
    newcontext->PacketQueue=HeapAlloc(GetProcessHeap(),0,sizeof(WTPACKET)*10);

    EnterCriticalSection(&csTablet);
    newcontext->handle = gTopContext++;
    newcontext->next = gOpenContexts;
    gOpenContexts = newcontext;
    LeaveCriticalSection(&csTablet);

    pAttachEventQueueToTablet(hWnd);

    TABLET_PostTabletMessage(newcontext, _WT_CTXOPEN(newcontext->context.lcMsgBase), (WPARAM)newcontext->handle,
                      newcontext->context.lcStatus, TRUE);

    if (fEnable)
    {
        newcontext->enabled = TRUE;
        /* TODO: Add to top of overlap order */
        newcontext->context.lcStatus = CXS_ONTOP;
    }
    else
    {
        newcontext->enabled = FALSE;
        newcontext->context.lcStatus = CXS_DISABLED;
    }

    TABLET_PostTabletMessage(newcontext, _WT_CTXOVERLAP(newcontext->context.lcMsgBase),
                            (WPARAM)newcontext->handle,
                            newcontext->context.lcStatus, TRUE);

    return newcontext->handle;
}

/***********************************************************************
 *		WTOpenA (WINTAB32.21)
 */
HCTX WINAPI WTOpenA(HWND hWnd, LPLOGCONTEXTA lpLogCtx, BOOL fEnable)
{
    LOGCONTEXTW logCtxW;

    LOGCONTEXTAtoW(lpLogCtx, &logCtxW);
    return WTOpenW(hWnd, &logCtxW, fEnable);
}

/***********************************************************************
 *		WTClose (WINTAB32.22)
 */
BOOL WINAPI WTClose(HCTX hCtx)
{
    LPOPENCONTEXT context,ptr;

    TRACE("(%p)\n", hCtx);

    EnterCriticalSection(&csTablet);

    ptr = context = gOpenContexts;

    while (context && (context->handle != hCtx))
    {
        ptr = context;
        context = context->next;
    }
    if (!context)
    {
        LeaveCriticalSection(&csTablet);
        return TRUE;
    }

    if (context == gOpenContexts)
        gOpenContexts = context->next;
    else
        ptr->next = context->next;

    LeaveCriticalSection(&csTablet);

    TABLET_PostTabletMessage(context, _WT_CTXCLOSE(context->context.lcMsgBase), (WPARAM)context->handle,
                      context->context.lcStatus,TRUE);

    HeapFree(GetProcessHeap(),0,context->PacketQueue);
    HeapFree(GetProcessHeap(),0,context);

    return TRUE;
}

/***********************************************************************
 *		WTPacketsGet (WINTAB32.23)
 */
int WINAPI WTPacketsGet(HCTX hCtx, int cMaxPkts, LPVOID lpPkts)
{
    int limit;
    LPOPENCONTEXT context;
    LPVOID ptr = lpPkts;

    TRACE("(%p, %d, %p)\n", hCtx, cMaxPkts, lpPkts);

    if (!hCtx)
        return 0;

    EnterCriticalSection(&csTablet);

    context = TABLET_FindOpenContext(hCtx);

    if (lpPkts != NULL)
        TABLET_BlankPacketData(context,lpPkts,cMaxPkts);

    if (context->PacketsQueued == 0)
    {
        LeaveCriticalSection(&csTablet);
        return 0;
    }

    limit = min(cMaxPkts,context->PacketsQueued);

    if(ptr != NULL)
    {
        int i = 0;
        for(i = 0; i < limit; i++)
            ptr=TABLET_CopyPacketData(context ,ptr, &context->PacketQueue[i]);
    }


    if (limit < context->PacketsQueued)
    {
        memmove(context->PacketQueue, &context->PacketQueue[limit],
            (context->PacketsQueued - (limit))*sizeof(WTPACKET));
    }
    context->PacketsQueued -= limit;
    LeaveCriticalSection(&csTablet);

    TRACE("Copied %i packets\n",limit);

    return limit;
}

/***********************************************************************
 *		WTPacket (WINTAB32.24)
 */
BOOL WINAPI WTPacket(HCTX hCtx, UINT wSerial, LPVOID lpPkt)
{
    int rc = 0;
    LPOPENCONTEXT context;
    LPWTPACKET wtp = NULL;

    TRACE("(%p, %d, %p)\n", hCtx, wSerial, lpPkt);

    if (!hCtx)
        return 0;

    EnterCriticalSection(&csTablet);

    context = TABLET_FindOpenContext(hCtx);

    rc = TABLET_FindPacket(context ,wSerial, &wtp);

    if (rc >= 0)
    {
        if (lpPkt)
           TABLET_CopyPacketData(context ,lpPkt, wtp);

        if ((rc+1) < context->QueueSize)
        {
            memmove(context->PacketQueue, &context->PacketQueue[rc+1],
                (context->PacketsQueued - (rc+1))*sizeof(WTPACKET));
        }
        context->PacketsQueued -= (rc+1);
    }
    LeaveCriticalSection(&csTablet);

    TRACE("Returning %i\n",rc+1);
    return rc+1;
}

/***********************************************************************
 *		WTEnable (WINTAB32.40)
 */
BOOL WINAPI WTEnable(HCTX hCtx, BOOL fEnable)
{
    LPOPENCONTEXT context;

    TRACE("hCtx=%p, fEnable=%u\n", hCtx, fEnable);

    if (!hCtx) return FALSE;

    EnterCriticalSection(&csTablet);
    context = TABLET_FindOpenContext(hCtx);
    /* if we want to enable and it is not enabled then */
    if(fEnable && !context->enabled)
    {
        context->enabled = TRUE;
        /* TODO: Add to top of overlap order */
        context->context.lcStatus = CXS_ONTOP;
        TABLET_PostTabletMessage(context,
            _WT_CTXOVERLAP(context->context.lcMsgBase),
            (WPARAM)context->handle,
            context->context.lcStatus, TRUE);
    }
    /* if we want to disable and it is not disabled then */
    else if (!fEnable && context->enabled)
    {
        context->enabled = FALSE;
        /* TODO: Remove from overlap order?? needs a test */
        context->context.lcStatus = CXS_DISABLED;
        TABLET_FlushQueue(context);
        TABLET_PostTabletMessage(context,
            _WT_CTXOVERLAP(context->context.lcMsgBase),
            (WPARAM)context->handle,
            context->context.lcStatus, TRUE);
    }
    LeaveCriticalSection(&csTablet);

    return TRUE;
}

/***********************************************************************
 *		WTOverlap (WINTAB32.41)
 *
 *		Move context to top or bottom of overlap order
 */
BOOL WINAPI WTOverlap(HCTX hCtx, BOOL fToTop)
{
    LPOPENCONTEXT context;

    TRACE("hCtx=%p, fToTop=%u\n", hCtx, fToTop);

    if (!hCtx) return FALSE;

    EnterCriticalSection(&csTablet);
    context = TABLET_FindOpenContext(hCtx);
    if (!context)
    {
        LeaveCriticalSection(&csTablet);
        return FALSE;
    }

    /* if we want to send to top and it's not already there */
    if (fToTop && context->context.lcStatus != CXS_ONTOP)
    {
        /* TODO: Move context to top of overlap order */
        FIXME("Not moving context to top of overlap order\n");
        context->context.lcStatus = CXS_ONTOP;
        TABLET_PostTabletMessage(context,
            _WT_CTXOVERLAP(context->context.lcMsgBase),
            (WPARAM)context->handle,
            context->context.lcStatus, TRUE);
    }
    else if (!fToTop)
    {
        /* TODO: Move context to bottom of overlap order */
        FIXME("Not moving context to bottom of overlap order\n");
        context->context.lcStatus = CXS_OBSCURED;
        TABLET_PostTabletMessage(context,
            _WT_CTXOVERLAP(context->context.lcMsgBase),
            (WPARAM)context->handle,
            context->context.lcStatus, TRUE);
    }
    LeaveCriticalSection(&csTablet);

    return TRUE;
}

/***********************************************************************
 *		WTConfig (WINTAB32.61)
 */
BOOL WINAPI WTConfig(HCTX hCtx, HWND hWnd)
{
    FIXME("(%p, %p): stub\n", hCtx, hWnd);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return FALSE;
}

/***********************************************************************
 *		WTGetA (WINTAB32.61)
 */
BOOL WINAPI WTGetA(HCTX hCtx, LPLOGCONTEXTA lpLogCtx)
{
    LPOPENCONTEXT context;

    TRACE("(%p, %p)\n", hCtx, lpLogCtx);

    if (!hCtx) return 0;

    EnterCriticalSection(&csTablet);
    context = TABLET_FindOpenContext(hCtx);
    LOGCONTEXTWtoA(&context->context, lpLogCtx);
    LeaveCriticalSection(&csTablet);

    return TRUE;
}

/***********************************************************************
 *		WTGetW (WINTAB32.1061)
 */
BOOL WINAPI WTGetW(HCTX hCtx, LPLOGCONTEXTW lpLogCtx)
{
    LPOPENCONTEXT context;

    TRACE("(%p, %p)\n", hCtx, lpLogCtx);

    if (!hCtx) return 0;

    EnterCriticalSection(&csTablet);
    context = TABLET_FindOpenContext(hCtx);
    memmove(lpLogCtx,&context->context,sizeof(LOGCONTEXTW));
    LeaveCriticalSection(&csTablet);

    return TRUE;
}

/***********************************************************************
 *		WTSetA (WINTAB32.62)
 */
BOOL WINAPI WTSetA(HCTX hCtx, LPLOGCONTEXTA lpLogCtx)
{
    LPOPENCONTEXT context;

    TRACE("hCtx=%p, lpLogCtx=%p\n", hCtx, lpLogCtx);

    if (!hCtx || !lpLogCtx) return FALSE;

    /* TODO: if cur process not owner of hCtx only modify
     * attribs not locked by owner */

    EnterCriticalSection(&csTablet);
    context = TABLET_FindOpenContext(hCtx);
    if (!context)
    {
        LeaveCriticalSection(&csTablet);
        return FALSE;
    }

    LOGCONTEXTAtoW(lpLogCtx, &context->context);
    LeaveCriticalSection(&csTablet);

    return TRUE;
}

/***********************************************************************
 *		WTSetW (WINTAB32.1062)
 */
BOOL WINAPI WTSetW(HCTX hCtx, LPLOGCONTEXTW lpLogCtx)
{
    LPOPENCONTEXT context;

    TRACE("hCtx=%p, lpLogCtx=%p\n", hCtx, lpLogCtx);

    if (!hCtx || !lpLogCtx) return FALSE;

    /* TODO: if cur process not hCtx owner only modify
     * attribs not locked by owner */

    EnterCriticalSection(&csTablet);
    context = TABLET_FindOpenContext(hCtx);
    if (!context)
    {
        LeaveCriticalSection(&csTablet);
        return FALSE;
    }

    memmove(&context->context, lpLogCtx, sizeof(LOGCONTEXTW));
    LeaveCriticalSection(&csTablet);

    return TRUE;
}

/***********************************************************************
 *		WTExtGet (WINTAB32.63)
 */
BOOL WINAPI WTExtGet(HCTX hCtx, UINT wExt, LPVOID lpData)
{
    FIXME("(%p, %u, %p): stub\n", hCtx, wExt, lpData);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return FALSE;
}

/***********************************************************************
 *		WTExtSet (WINTAB32.64)
 */
BOOL WINAPI WTExtSet(HCTX hCtx, UINT wExt, LPVOID lpData)
{
    FIXME("(%p, %u, %p): stub\n", hCtx, wExt, lpData);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return FALSE;
}

/***********************************************************************
 *		WTSave (WINTAB32.65)
 */
BOOL WINAPI WTSave(HCTX hCtx, LPVOID lpSaveInfo)
{
    FIXME("(%p, %p): stub\n", hCtx, lpSaveInfo);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return FALSE;
}

/***********************************************************************
 *		WTRestore (WINTAB32.66)
 */
HCTX WINAPI WTRestore(HWND hWnd, LPVOID lpSaveInfo, BOOL fEnable)
{
    FIXME("(%p, %p, %u): stub\n", hWnd, lpSaveInfo, fEnable);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return 0;
}

/***********************************************************************
 *		WTPacketsPeek (WINTAB32.80)
 */
int WINAPI WTPacketsPeek(HCTX hCtx, int cMaxPkts, LPVOID lpPkts)
{
    int limit;
    LPOPENCONTEXT context;
    LPVOID ptr = lpPkts;

    TRACE("(%p, %d, %p)\n", hCtx, cMaxPkts, lpPkts);

    if (!hCtx || !lpPkts) return 0;

    EnterCriticalSection(&csTablet);

    context = TABLET_FindOpenContext(hCtx);

    if (context->PacketsQueued == 0)
    {
        LeaveCriticalSection(&csTablet);
        return 0;
    }

    for (limit = 0; limit < cMaxPkts && limit < context->PacketsQueued; limit++)
        ptr = TABLET_CopyPacketData(context ,ptr, &context->PacketQueue[limit]);

    LeaveCriticalSection(&csTablet);
    TRACE("Copied %i packets\n",limit);
    return limit;
}

/***********************************************************************
 *		WTDataGet (WINTAB32.81)
 */
int WINAPI WTDataGet(HCTX hCtx, UINT wBegin, UINT wEnd,
		     int cMaxPkts, LPVOID lpPkts, LPINT lpNPkts)
{
    LPOPENCONTEXT context;
    LPVOID ptr = lpPkts;
    INT bgn = 0;
    INT end = 0;
    INT num = 0;

    TRACE("(%p, %u, %u, %d, %p, %p)\n",
	  hCtx, wBegin, wEnd, cMaxPkts, lpPkts, lpNPkts);

    if (!hCtx) return 0;

    EnterCriticalSection(&csTablet);

    context = TABLET_FindOpenContext(hCtx);

    if (context->PacketsQueued == 0)
    {
        LeaveCriticalSection(&csTablet);
        return 0;
    }

    while (bgn < context->PacketsQueued &&
           context->PacketQueue[bgn].pkSerialNumber != wBegin)
        bgn++;

    end = bgn;
    while (end < context->PacketsQueued &&
           context->PacketQueue[end].pkSerialNumber != wEnd)
        end++;

    if ((bgn == end) && (end == context->PacketsQueued))
    {
        LeaveCriticalSection(&csTablet);
        return 0;
    }

    for (num = bgn; num <= end; num++)
        ptr = TABLET_CopyPacketData(context ,ptr, &context->PacketQueue[num]);

    /* remove read packets */
    if ((end+1) < context->PacketsQueued)
        memmove( &context->PacketQueue[bgn], &context->PacketQueue[end+1],
                (context->PacketsQueued - (end+1)) * sizeof (WTPACKET));

    context->PacketsQueued -= ((end-bgn)+1);
    *lpNPkts = ((end-bgn)+1);

    LeaveCriticalSection(&csTablet);
    TRACE("Copied %i packets\n",*lpNPkts);
    return (end - bgn)+1;
}

/***********************************************************************
 *		WTDataPeek (WINTAB32.82)
 */
int WINAPI WTDataPeek(HCTX hCtx, UINT wBegin, UINT wEnd,
		      int cMaxPkts, LPVOID lpPkts, LPINT lpNPkts)
{
    LPOPENCONTEXT context;
    LPVOID ptr = lpPkts;
    INT bgn = 0;
    INT end = 0;
    INT num = 0;

    TRACE("(%p, %u, %u, %d, %p, %p)\n",
	  hCtx, wBegin, wEnd, cMaxPkts, lpPkts, lpNPkts);

    if (!hCtx || !lpPkts) return 0;

    EnterCriticalSection(&csTablet);

    context = TABLET_FindOpenContext(hCtx);

    if (context->PacketsQueued == 0)
    {
        LeaveCriticalSection(&csTablet);
        return 0;
    }

    while (bgn < context->PacketsQueued &&
           context->PacketQueue[bgn].pkSerialNumber != wBegin)
        bgn++;

    end = bgn;
    while (end < context->PacketsQueued &&
           context->PacketQueue[end].pkSerialNumber != wEnd)
        end++;

    if (bgn == context->PacketsQueued ||  end == context->PacketsQueued)
    {
        TRACE("%i %i %i\n", bgn, end, context->PacketsQueued);
        LeaveCriticalSection(&csTablet);
        return 0;
    }

    for (num = bgn; num <= end; num++)
        ptr = TABLET_CopyPacketData(context ,ptr, &context->PacketQueue[num]);

    *lpNPkts = ((end-bgn)+1);
    LeaveCriticalSection(&csTablet);

    TRACE("Copied %i packets\n",*lpNPkts);
    return (end - bgn)+1;
}

/***********************************************************************
 *		WTQueuePacketsEx (WINTAB32.200)
 */
BOOL WINAPI WTQueuePacketsEx(HCTX hCtx, UINT *lpOld, UINT *lpNew)
{
    LPOPENCONTEXT context;

    TRACE("(%p, %p, %p)\n", hCtx, lpOld, lpNew);

    if (!hCtx) return 0;

    EnterCriticalSection(&csTablet);

    context = TABLET_FindOpenContext(hCtx);

    if (context->PacketsQueued)
    {
        *lpOld = context->PacketQueue[0].pkSerialNumber;
        *lpNew = context->PacketQueue[context->PacketsQueued-1].pkSerialNumber;
    }
    else
    {
        TRACE("No packets\n");
        LeaveCriticalSection(&csTablet);
        return FALSE;
    }
    LeaveCriticalSection(&csTablet);

    return TRUE;
}

/***********************************************************************
 *		WTQueueSizeGet (WINTAB32.84)
 */
int WINAPI WTQueueSizeGet(HCTX hCtx)
{
    LPOPENCONTEXT context;
    TRACE("(%p)\n", hCtx);

    if (!hCtx) return 0;

    EnterCriticalSection(&csTablet);
    context = TABLET_FindOpenContext(hCtx);
    LeaveCriticalSection(&csTablet);
    return context->QueueSize;
}

/***********************************************************************
 *		WTQueueSizeSet (WINTAB32.85)
 */
BOOL WINAPI WTQueueSizeSet(HCTX hCtx, int nPkts)
{
    LPOPENCONTEXT context;

    TRACE("(%p, %d)\n", hCtx, nPkts);

    if (!hCtx) return 0;

    EnterCriticalSection(&csTablet);

    context = TABLET_FindOpenContext(hCtx);

    context->PacketQueue = HeapReAlloc(GetProcessHeap(), 0,
                        context->PacketQueue, sizeof(WTPACKET)*nPkts);

    context->QueueSize = nPkts;
    LeaveCriticalSection(&csTablet);

    return nPkts;
}
