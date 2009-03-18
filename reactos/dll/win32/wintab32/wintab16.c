/*
 * Tablet Win16
 *
 * Copyright 2002 Patrik Stridvall
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"

#include "wintab.h"

#include "wine/windef16.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wintab);

/**********************************************************************/

#define DECLARE_HANDLE16(a) \
	typedef HANDLE16 a##16; \
	typedef a##16 *P##a##16; \
	typedef a##16 *NP##a##16; \
	typedef a##16 *LP##a##16

DECLARE_HANDLE16(HMGR);     /* manager handle */
DECLARE_HANDLE16(HCTX);     /* context handle */
DECLARE_HANDLE16(HWTHOOK);  /* hook handle */

/**********************************************************************/

typedef struct tagLOGCONTEXT16 {
    char    lcName[LCNAMELEN];
    UINT16  lcOptions;
    UINT16  lcStatus;
    UINT16  lcLocks;
    UINT16  lcMsgBase;
    UINT16  lcDevice;
    UINT16  lcPktRate;
    WTPKT   lcPktData;
    WTPKT   lcPktMode;
    WTPKT   lcMoveMask;
    DWORD   lcBtnDnMask;
    DWORD   lcBtnUpMask;
    LONG    lcInOrgX;
    LONG    lcInOrgY;
    LONG    lcInOrgZ;
    LONG    lcInExtX;
    LONG    lcInExtY;
    LONG    lcInExtZ;
    LONG    lcOutOrgX;
    LONG    lcOutOrgY;
    LONG    lcOutOrgZ;
    LONG    lcOutExtX;
    LONG    lcOutExtY;
    LONG    lcOutExtZ;
    FIX32   lcSensX;
    FIX32   lcSensY;
    FIX32   lcSensZ;
    BOOL16  lcSysMode;
    INT16   lcSysOrgX;
    INT16   lcSysOrgY;
    INT16   lcSysExtX;
    INT16   lcSysExtY;
    FIX32   lcSysSensX;
    FIX32   lcSysSensY;
} LOGCONTEXT16, *PLOGCONTEXT16, *NPLOGCONTEXT16, *LPLOGCONTEXT16;

/**********************************************************************/

typedef BOOL16 (WINAPI * WTENUMPROC16)(HCTX16, LPARAM);  /* changed CALLBACK->WINAPI, 1.1 */
typedef BOOL16 (WINAPI * WTCONFIGPROC16)(HCTX16, HWND16);
typedef LRESULT (WINAPI * WTHOOKPROC16)(INT16, WPARAM16, LPARAM);
typedef WTHOOKPROC16 *LPWTHOOKPROC16;

/***********************************************************************
 *		WTInfo (WINTAB.20)
 */
UINT16 WINAPI WTInfo16(UINT16 wCategory, UINT16 nIndex, LPVOID lpOutput)
{
    FIXME("(%hu, %hu, %p): stub\n", wCategory, nIndex, lpOutput);

    return 0;
}

/***********************************************************************
 *		WTOpen (WINTAB.21)
 */
HCTX16 WINAPI WTOpen16(HWND16 hWnd, LPLOGCONTEXT16 lpLogCtx, BOOL16 fEnable)
{
    FIXME("(0x%04hx, %p, %hu): stub\n", hWnd, lpLogCtx, fEnable);

    return 0;
}

/***********************************************************************
 *		WTClose (WINTAB.22)
 */
BOOL16 WINAPI WTClose16(HCTX16 hCtx)
{
    FIXME("(0x%04hx): stub\n", hCtx);

    return TRUE;
}

/***********************************************************************
 *		WTPacketsGet (WINTAB.23)
 */
INT16 WINAPI WTPacketsGet16(HCTX16 hCtx, INT16 cMaxPkts, LPVOID lpPkts)
{
    FIXME("(0x%04hx, %hd, %p): stub\n", hCtx, cMaxPkts, lpPkts);

    return 0;
}

/***********************************************************************
 *		WTPacket (WINTAB.24)
 */
BOOL16 WINAPI WTPacket16(HCTX16 hCtx, UINT16 wSerial, LPVOID lpPkt)
{
    FIXME("(0x%04hx, %hd, %p): stub\n", hCtx, wSerial, lpPkt);

    return FALSE;
}

/***********************************************************************
 *		WTEnable (WINTAB.40)
 */
BOOL16 WINAPI WTEnable16(HCTX16 hCtx, BOOL16 fEnable)
{
    FIXME("(0x%04hx, %hu): stub\n", hCtx, fEnable);

    return FALSE;
}

/***********************************************************************
 *		WTOverlap (WINTAB.41)
 */
BOOL16 WINAPI WTOverlap16(HCTX16 hCtx, BOOL16 fToTop)
{
    FIXME("(0x%04hx, %hu): stub\n", hCtx, fToTop);

    return FALSE;
}

/***********************************************************************
 *		WTConfig (WINTAB.60)
 */
BOOL16 WINAPI WTConfig16(HCTX16 hCtx, HWND16 hWnd)
{
    FIXME("(0x%04hx, 0x%04hx): stub\n", hCtx, hWnd);

    return FALSE;
}

/***********************************************************************
 *		WTGet (WINTAB.61)
 */
BOOL16 WINAPI WTGet16(HCTX16 hCtx, LPLOGCONTEXT16 lpLogCtx)
{
    FIXME("(0x%04hx, %p): stub\n", hCtx, lpLogCtx);

    return FALSE;
}

/***********************************************************************
 *		WTSet (WINTAB.62)
 */
BOOL16 WINAPI WTSet16(HCTX16 hCtx, LPLOGCONTEXT16 lpLogCtx)
{
    FIXME("(0x%04hx, %p): stub\n", hCtx, lpLogCtx);

    return FALSE;
}

/***********************************************************************
 *		WTExtGet (WINTAB.63)
 */
BOOL16 WINAPI WTExtGet16(HCTX16 hCtx, UINT16 wExt, LPVOID lpData)
{
    FIXME("(0x%04hx, %hu, %p): stub\n", hCtx, wExt, lpData);

    return FALSE;
}

/***********************************************************************
 *		WTExtSet (WINTAB.64)
 */
BOOL16 WINAPI WTExtSet16(HCTX16 hCtx, UINT16 wExt, LPVOID lpData)
{
    FIXME("(0x%04hx, %hu, %p): stub\n", hCtx, wExt, lpData);

    return FALSE;
}

/***********************************************************************
 *		WTSave (WINTAB.65)
 */
BOOL16 WINAPI WTSave16(HCTX16 hCtx, LPVOID lpSaveInfo)
{
    FIXME("(0x%04hx, %p): stub\n", hCtx, lpSaveInfo);

    return FALSE;
}

/***********************************************************************
 *		WTRestore (WINTAB.66)
 */
HCTX16 WINAPI WTRestore16(HWND16 hWnd, LPVOID lpSaveInfo, BOOL16 fEnable)
{
    FIXME("(0x%04hx, %p, %hu): stub\n", hWnd, lpSaveInfo, fEnable);

    return 0;
}

/***********************************************************************
 *		WTPacketsPeek (WINTAB.80)
 */
INT16 WINAPI WTPacketsPeek16(HCTX16 hCtx, INT16 cMaxPkts, LPVOID lpPkts)
{
    FIXME("(0x%04hx, %hd, %p): stub\n", hCtx, cMaxPkts, lpPkts);

    return 0;
}

/***********************************************************************
 *		WTDataGet (WINTAB.81)
 */
INT16 WINAPI WTDataGet16(HCTX16 hCtx, UINT16 wBegin, UINT16 wEnd,
			 INT16 cMaxPkts, LPVOID lpPkts, LPINT16 lpNPkts)
{
    FIXME("(0x%04hx, %hu, %hu, %hd, %p, %p): stub\n",
	  hCtx, wBegin, wEnd, cMaxPkts, lpPkts, lpNPkts);

    return 0;
}

/***********************************************************************
 *		WTDataPeek (WINTAB.82)
 */
INT16 WINAPI WTDataPeek16(HCTX16 hCtx, UINT16 wBegin, UINT16 wEnd,
			  INT16 cMaxPkts, LPVOID lpPkts, LPINT16 lpNPkts)
{
    FIXME("(0x%04hx, %hu, %hu, %hd, %p, %p): stub\n",
	  hCtx, wBegin, wEnd, cMaxPkts, lpPkts, lpNPkts);

    return 0;
}

/***********************************************************************
 *		WTQueuePackets (WINTAB.83)
 *
 * OBSOLETE IN WIN32!
 */
DWORD WINAPI WTQueuePackets16(HCTX16 hCtx)
{
    FIXME("(0x%04hx): stub\n", hCtx);

    return 0;
}

/***********************************************************************
 *		WTQueuePacketsEx (WINTAB.200)
 */
BOOL16 WINAPI WTQueuePacketsEx16(HCTX16 hCtx, UINT16 *lpOld, UINT16 *lpNew)
{
    FIXME("(0x%04hx, %p, %p): stub\n", hCtx, lpOld, lpNew);

    return TRUE;
}

/***********************************************************************
 *		WTQueueSizeGet (WINTAB.84)
 */
INT16 WINAPI WTQueueSizeGet16(HCTX16 hCtx)
{
    FIXME("(0x%04hx): stub\n", hCtx);

    return 0;
}

/***********************************************************************
 *		WTQueueSizeSet (WINTAB.85)
 */
BOOL16 WINAPI WTQueueSizeSet16(HCTX16 hCtx, INT16 nPkts)
{
    FIXME("(0x%04hx, %hd): stub\n", hCtx, nPkts);

    return FALSE;
}

/***********************************************************************
 *		WTMgrOpen (WINTAB.100)
 */
HMGR16 WINAPI WTMgrOpen16(HWND16 hWnd, UINT16 wMsgBase)
{
    FIXME("(0x%04hx, %hu): stub\n", hWnd, wMsgBase);

    return 0;
}

/***********************************************************************
 *		WTMgrClose (WINTAB.101)
 */
BOOL16 WINAPI WTMgrClose16(HMGR16 hMgr)
{
    FIXME("(0x%04hx): stub\n", hMgr);

    return FALSE;
}

/***********************************************************************
 *		WTMgrContextEnum (WINTAB.120)
 */
BOOL16 WINAPI WTMgrContextEnum16(HMGR16 hMgr, WTENUMPROC16 lpEnumFunc, LPARAM lParam)
{
    FIXME("(0x%04hx, %p, %ld): stub\n", hMgr, lpEnumFunc, lParam);

    return FALSE;
}

/***********************************************************************
 *		WTMgrContextOwner (WINTAB.121)
 */
HWND16 WINAPI WTMgrContextOwner16(HMGR16 hMgr, HCTX16 hCtx)
{
    FIXME("(0x%04hx, 0x%04hx): stub\n", hMgr, hCtx);

    return 0;
}

/***********************************************************************
 *		WTMgrDefContext (WINTAB.122)
 */
HCTX16 WINAPI WTMgrDefContext16(HMGR16 hMgr, BOOL16 fSystem)
{
    FIXME("(0x%04hx, %hu): stub\n", hMgr, fSystem);

    return 0;
}

/***********************************************************************
 *		WTMgrDefContextEx (WINTAB.206)
 *
 * 1.1
 */
HCTX16 WINAPI WTMgrDefContextEx16(HMGR16 hMgr, UINT16 wDevice, BOOL16 fSystem)
{
    FIXME("(0x%04hx, %hu, %hu): stub\n", hMgr, wDevice, fSystem);

    return 0;
}

/***********************************************************************
 *		WTMgrDeviceConfig (WINTAB.140)
 */
UINT16 WINAPI WTMgrDeviceConfig16(HMGR16 hMgr, UINT16 wDevice, HWND16 hWnd)
{
    FIXME("(0x%04hx, %hu, 0x%04hx): stub\n", hMgr, wDevice, hWnd);

    return 0;
}

/***********************************************************************
 *		WTMgrConfigReplace (WINTAB.141)
 *
 * OBSOLETE IN WIN32!
 */
BOOL16 WINAPI WTMgrConfigReplace16(HMGR16 hMgr, BOOL16 fInstall,
				   WTCONFIGPROC16 lpConfigProc)
{
    FIXME("(0x%04hx, %hu, %p): stub\n", hMgr, fInstall, lpConfigProc);

    return FALSE;
}

/***********************************************************************
 *		WTMgrConfigReplaceEx (WINTAB.202)
 */
BOOL16 WINAPI WTMgrConfigReplaceEx16(HMGR16 hMgr, BOOL16 fInstall,
				     LPSTR lpszModule, LPSTR lpszCfgProc)
{
    FIXME("(0x%04hx, %hu, %s, %s): stub\n", hMgr, fInstall,
	  debugstr_a(lpszModule), debugstr_a(lpszCfgProc));

    return FALSE;
}

/***********************************************************************
 *		WTMgrPacketHook (WINTAB.160)
 *
 * OBSOLETE IN WIN32!
 */
WTHOOKPROC16 WINAPI WTMgrPacketHook16(HMGR16 hMgr, BOOL16 fInstall,
				      INT16 nType, WTHOOKPROC16 lpFunc)
{
    FIXME("(0x%04hx, %hu, %hd, %p): stub\n", hMgr, fInstall, nType, lpFunc);

    return 0;
}

/***********************************************************************
 *		WTMgrPacketHookEx (WINTAB.203)
 */
HWTHOOK16 WINAPI WTMgrPacketHookEx16(HMGR16 hMgr, INT16 nType,
				     LPSTR lpszModule, LPSTR lpszHookProc)
{
    FIXME("(0x%04hx, %hd, %s, %s): stub\n", hMgr, nType,
	  debugstr_a(lpszModule), debugstr_a(lpszHookProc));

    return 0;
}

/***********************************************************************
 *		WTMgrPacketUnhook (WINTAB.204)
 */
BOOL16 WINAPI WTMgrPacketUnhook16(HWTHOOK16 hHook)
{
    FIXME("(0x%04hx): stub\n", hHook);

    return FALSE;
}

/***********************************************************************
 *		WTMgrPacketHookDefProc (WINTAB.161)
 *
 * OBSOLETE IN WIN32!
 */
LRESULT WINAPI WTMgrPacketHookDefProc16(INT16 nCode, WPARAM16 wParam,
					LPARAM lParam, LPWTHOOKPROC16 lplpFunc)
{
    FIXME("(%hd, %hu, %lu, %p): stub\n", nCode, wParam, lParam, lplpFunc);

    return 0;
}

/***********************************************************************
 *		WTMgrPacketHookNext (WINTAB.205)
 */
LRESULT WINAPI WTMgrPacketHookNext16(HWTHOOK16 hHook, INT16 nCode,
				     WPARAM16 wParam, LPARAM lParam)
{
    FIXME("(0x%04hx, %hd, %hu, %lu): stub\n", hHook, nCode, wParam, lParam);

    return 0;
}


/***********************************************************************
 *		WTMgrExt (WINTAB.180)
 */
BOOL16 WINAPI WTMgrExt16(HMGR16 hMgr, UINT16 wExt, LPVOID lpData)
{
    FIXME("(0x%04hx, %hu, %p): stub\n", hMgr, wExt, lpData);

    return FALSE;
}

/***********************************************************************
 *		WTMgrCsrEnable (WINTAB.181)
 */
BOOL16 WINAPI WTMgrCsrEnable16(HMGR16 hMgr, UINT16 wCursor, BOOL16 fEnable)
{
    FIXME("(0x%04hx, %hu, %hu): stub\n", hMgr, wCursor, fEnable);

    return FALSE;
}

/***********************************************************************
 *		WTMgrCsrButtonMap (WINTAB.182)
 */
BOOL16 WINAPI WTMgrCsrButtonMap16(HMGR16 hMgr, UINT16 wCursor,
				  LPBYTE lpLogBtns, LPBYTE lpSysBtns)
{
    FIXME("(0x%04hx, %hu, %p, %p): stub\n", hMgr, wCursor, lpLogBtns, lpSysBtns);

    return FALSE;
}

/***********************************************************************
 *		WTMgrCsrPressureBtnMarks (WINTAB.183)
 *
 * OBSOLETE IN WIN32! (But only according to documentation)
 */
BOOL16 WINAPI WTMgrCsrPressureBtnMarks16(HMGR16 hMgr, UINT16 wCsr,
					 DWORD dwNMarks, DWORD dwTMarks)
{
    FIXME("(0x%04hx, %hu, %u, %u): stub\n", hMgr, wCsr, dwNMarks, dwTMarks);

    return FALSE;
}

/***********************************************************************
 *		WTMgrCsrPressureBtnMarksEx (WINTAB.201)
 */
BOOL16 WINAPI WTMgrCsrPressureBtnMarksEx16(HMGR16 hMgr, UINT16 wCsr,
					   UINT16 *lpNMarks, UINT16 *lpTMarks)
{
    FIXME("(0x%04hx, %hu, %p, %p): stub\n", hMgr, wCsr, lpNMarks, lpTMarks);

    return FALSE;
}

/***********************************************************************
 *		WTMgrCsrPressureResponse (WINTAB.184)
 */
BOOL16 WINAPI WTMgrCsrPressureResponse16(HMGR16 hMgr, UINT16 wCsr,
					 UINT16 *lpNResp, UINT16 *lpTResp)
{
    FIXME("(0x%04hx, %hu, %p, %p): stub\n", hMgr, wCsr, lpNResp, lpTResp);

    return FALSE;
}

/***********************************************************************
 *		WTMgrCsrExt (WINTAB.185)
 */
BOOL16 WINAPI WTMgrCsrExt16(HMGR16 hMgr, UINT16 wCsr, UINT16 wExt, LPVOID lpData)
{
    FIXME("(0x%04hx, %hu, %hu, %p): stub\n", hMgr, wCsr, wExt, lpData);

    return FALSE;
}
