/*
 * Tablet Manager
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
#include "winerror.h"

#include "wintab.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wintab32);

/***********************************************************************
 *		WTMgrOpen (WINTAB32.100)
 */
HMGR WINAPI WTMgrOpen(HWND hWnd, UINT wMsgBase)
{
    FIXME("(%p, %u): stub\n", hWnd, wMsgBase);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return NULL;
}

/***********************************************************************
 *		WTMgrClose (WINTAB32.101)
 */
BOOL WINAPI WTMgrClose(HMGR hMgr)
{
    FIXME("(%p): stub\n", hMgr);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return FALSE;
}

/***********************************************************************
 *		WTMgrContextEnum (WINTAB32.120)
 */
BOOL WINAPI WTMgrContextEnum(HMGR hMgr, WTENUMPROC lpEnumFunc, LPARAM lParam)
{
    FIXME("(%p, %p, %ld): stub\n", hMgr, lpEnumFunc, lParam);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return FALSE;
}

/***********************************************************************
 *		WTMgrContextOwner (WINTAB32.121)
 */
HWND WINAPI WTMgrContextOwner(HMGR hMgr, HCTX hCtx)
{
    FIXME("(%p, %p): stub\n", hMgr, hCtx);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return NULL;
}

/***********************************************************************
 *		WTMgrDefContext (WINTAB32.122)
 */
HCTX WINAPI WTMgrDefContext(HMGR hMgr, BOOL fSystem)
{
    FIXME("(%p, %u): stub\n", hMgr, fSystem);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return NULL;
}

/***********************************************************************
 *		WTMgrDefContextEx (WINTAB32.206)
 *
 * 1.1
 */
HCTX WINAPI WTMgrDefContextEx(HMGR hMgr, UINT wDevice, BOOL fSystem)
{
    FIXME("(%p, %hu, %hu): stub\n", hMgr, wDevice, fSystem);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return NULL;
}

/***********************************************************************
 *		WTMgrDeviceConfig (WINTAB32.140)
 */
UINT WINAPI WTMgrDeviceConfig(HMGR hMgr, UINT wDevice, HWND hWnd)
{
    FIXME("(%p, %u, %p): stub\n", hMgr, wDevice, hWnd);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return 0;
}

/***********************************************************************
 *		WTMgrConfigReplaceExA (WINTAB32.202)
 */
BOOL WINAPI WTMgrConfigReplaceExA(HMGR hMgr, BOOL fInstall,
				  LPSTR lpszModule, LPSTR lpszCfgProc)
{
    FIXME("(%p, %u, %s, %s): stub\n", hMgr, fInstall,
	  debugstr_a(lpszModule), debugstr_a(lpszCfgProc));

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return FALSE;
}

/***********************************************************************
 *		WTMgrConfigReplaceExW (WINTAB32.1202)
 */
BOOL WINAPI WTMgrConfigReplaceExW(HMGR hMgr, BOOL fInstall,
				  LPWSTR lpszModule, LPSTR lpszCfgProc)
{
    FIXME("(%p, %u, %s, %s): stub\n", hMgr, fInstall,
	  debugstr_w(lpszModule), debugstr_a(lpszCfgProc));

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return FALSE;
}

/***********************************************************************
 *		WTMgrPacketHookExA (WINTAB32.203)
 */
HWTHOOK WINAPI WTMgrPacketHookExA(HMGR hMgr, int nType,
				  LPSTR lpszModule, LPSTR lpszHookProc)
{
    FIXME("(%p, %d, %s, %s): stub\n", hMgr, nType,
	  debugstr_a(lpszModule), debugstr_a(lpszHookProc));

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return NULL;
}

/***********************************************************************
 *		WTMgrPacketHookExW (WINTAB32.1203)
 */
HWTHOOK WINAPI WTMgrPacketHookExW(HMGR hMgr, int nType,
				  LPWSTR lpszModule, LPSTR lpszHookProc)
{
    FIXME("(%p, %d, %s, %s): stub\n", hMgr, nType,
	  debugstr_w(lpszModule), debugstr_a(lpszHookProc));

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return NULL;
}

/***********************************************************************
 *		WTMgrPacketUnhook (WINTAB32.204)
 */
BOOL WINAPI WTMgrPacketUnhook(HWTHOOK hHook)
{
    FIXME("(%p): stub\n", hHook);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return FALSE;
}

/***********************************************************************
 *		WTMgrPacketHookNext (WINTAB32.205)
 */
LRESULT WINAPI WTMgrPacketHookNext(HWTHOOK hHook, int nCode,
				   WPARAM wParam, LPARAM lParam)
{
    FIXME("(%p, %d, %lu, %lu): stub\n", hHook, nCode, wParam, lParam);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return 0;
}


/***********************************************************************
 *		WTMgrExt (WINTAB32.180)
 */
BOOL WINAPI WTMgrExt(HMGR hMgr, UINT wExt, LPVOID lpData)
{
    FIXME("(%p, %u, %p): stub\n", hMgr, wExt, lpData);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return FALSE;
}

/***********************************************************************
 *		WTMgrCsrEnable (WINTAB32.181)
 */
BOOL WINAPI WTMgrCsrEnable(HMGR hMgr, UINT wCursor, BOOL fEnable)
{
    FIXME("(%p, %u, %u): stub\n", hMgr, wCursor, fEnable);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return FALSE;
}

/***********************************************************************
 *		WTMgrCsrButtonMap (WINTAB32.182)
 */
BOOL WINAPI WTMgrCsrButtonMap(HMGR hMgr, UINT wCursor,
			      LPBYTE lpLogBtns, LPBYTE lpSysBtns)
{
    FIXME("(%p, %u, %p, %p): stub\n", hMgr, wCursor, lpLogBtns, lpSysBtns);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return FALSE;
}

/***********************************************************************
 *		WTMgrCsrPressureBtnMarks (WINTAB32.183)
 *
 * OBSOLETE IN WIN32! (But only according to documentation)
 */
BOOL WINAPI WTMgrCsrPressureBtnMarks(HMGR hMgr, UINT wCsr,
				     DWORD dwNMarks, DWORD dwTMarks)
{
    FIXME("(%p, %u, %u, %u): stub\n", hMgr, wCsr, dwNMarks, dwTMarks);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return FALSE;
}

/***********************************************************************
 *		WTMgrCsrPressureBtnMarksEx (WINTAB32.201)
 */
BOOL WINAPI WTMgrCsrPressureBtnMarksEx(HMGR hMgr, UINT wCsr,
				       UINT *lpNMarks, UINT *lpTMarks)
{
    FIXME("(%p, %u, %p, %p): stub\n", hMgr, wCsr, lpNMarks, lpTMarks);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return FALSE;
}

/***********************************************************************
 *		WTMgrCsrPressureResponse (WINTAB32.184)
 */
BOOL WINAPI WTMgrCsrPressureResponse(HMGR hMgr, UINT wCsr,
				     UINT *lpNResp, UINT *lpTResp)
{
    FIXME("(%p, %u, %p, %p): stub\n", hMgr, wCsr, lpNResp, lpTResp);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return FALSE;
}

/***********************************************************************
 *		WTMgrCsrExt (WINTAB32.185)
 */
BOOL WINAPI WTMgrCsrExt(HMGR hMgr, UINT wCsr, UINT wExt, LPVOID lpData)
{
    FIXME("(%p, %u, %u, %p): stub\n", hMgr, wCsr, wExt, lpData);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return FALSE;
}
