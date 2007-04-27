/* Copyright 2005 Ulrich Czekalla
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
#include <stdlib.h>
#include "windef.h"
#include "winbase.h"
#include "wtsapi32.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wtsapi);

static HMODULE WTSAPI32_hModule;

BOOL WINAPI DllMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("%p,%x,%p\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
        {
            DisableThreadLibraryCalls(hinstDLL);
            WTSAPI32_hModule = hinstDLL;
            break;
        }
        case DLL_PROCESS_DETACH:
        {
            break;
        }
    }

    return TRUE;
}

/************************************************************
 *                WTSCloseServer  (WTSAPI32.@)
 */
void WINAPI WTSCloseServer(HANDLE hServer)
{
    FIXME("Stub %p\n", hServer);
}

/************************************************************
 *                WTSDisconnectSession  (WTSAPI32.@)
 */
BOOL WINAPI WTSDisconnectSession(HANDLE hServer, DWORD SessionId, BOOL bWait)
{
    FIXME("Stub %p 0x%08x %d\n", hServer, SessionId, bWait);
    return TRUE;
}

/************************************************************
 *                WTSEnumerateProcessesA  (WTSAPI32.@)
 */
BOOL WINAPI WTSEnumerateProcessesA(HANDLE hServer, DWORD Reserved, DWORD Version,
    PWTS_PROCESS_INFOA* ppProcessInfo, DWORD* pCount)
{
    FIXME("Stub %p 0x%08x 0x%08x %p %p\n", hServer, Reserved, Version,
          ppProcessInfo, pCount);

    if (!ppProcessInfo || !pCount) return FALSE;

    *pCount = 0;
    *ppProcessInfo = NULL;

    return TRUE;
}

/************************************************************
 *                WTSEnumerateProcessesW  (WTSAPI32.@)
 */
BOOL WINAPI WTSEnumerateProcessesW(HANDLE hServer, DWORD Reserved, DWORD Version,
    PWTS_PROCESS_INFOW* ppProcessInfo, DWORD* pCount)
{
    FIXME("Stub %p 0x%08x 0x%08x %p %p\n", hServer, Reserved, Version,
          ppProcessInfo, pCount);

    if (!ppProcessInfo || !pCount) return FALSE;

    *pCount = 0;
    *ppProcessInfo = NULL;

    return TRUE;
}

/************************************************************
 *                WTSEnumerateEnumerateSessionsA  (WTSAPI32.@)
 */
BOOL WINAPI WTSEnumerateSessionsA(HANDLE hServer, DWORD Reserved, DWORD Version,
    PWTS_SESSION_INFOA* ppSessionInfo, DWORD* pCount)
{
    FIXME("Stub %p 0x%08x 0x%08x %p %p\n", hServer, Reserved, Version,
          ppSessionInfo, pCount);

    if (!ppSessionInfo || !pCount) return FALSE;

    *pCount = 0;
    *ppSessionInfo = NULL;

    return TRUE;
}

/************************************************************
 *                WTSEnumerateEnumerateSessionsW  (WTSAPI32.@)
 */
BOOL WINAPI WTSEnumerateSessionsW(HANDLE hServer, DWORD Reserved, DWORD Version,
    PWTS_SESSION_INFOW* ppSessionInfo, DWORD* pCount)
{
    FIXME("Stub %p 0x%08x 0x%08x %p %p\n", hServer, Reserved, Version,
          ppSessionInfo, pCount);

    if (!ppSessionInfo || !pCount) return FALSE;

    *pCount = 0;
    *ppSessionInfo = NULL;

    return TRUE;
}

/************************************************************
 *                WTSFreeMemory (WTSAPI32.@)
 */
void WINAPI WTSFreeMemory(PVOID pMemory)
{
    FIXME("Stub %p\n", pMemory);
    return;
}

/************************************************************
 *                WTSOpenServerA (WTSAPI32.@)
 */
HANDLE WINAPI WTSOpenServerA(LPSTR pServerName)
{
    FIXME("(%s) stub\n", debugstr_a(pServerName));
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return NULL;
}

/************************************************************
 *                WTSOpenServerW (WTSAPI32.@)
 */
HANDLE WINAPI WTSOpenServerW(LPWSTR pServerName)
{
    FIXME("(%s) stub\n", debugstr_w(pServerName));
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return NULL;
}

/************************************************************
 *                WTSQuerySessionInformationA  (WTSAPI32.@)
 */
BOOL WINAPI WTSQuerySessionInformationA(
    HANDLE hServer,
    DWORD SessionId,
    WTS_INFO_CLASS WTSInfoClass,
    LPSTR* Buffer,
    DWORD* BytesReturned)
{
    /* FIXME: Forward request to winsta.dll::WinStationQueryInformationA */
    FIXME("Stub %p 0x%08x %d %p %p\n", hServer, SessionId, WTSInfoClass,
        Buffer, BytesReturned);

    return FALSE;
}

/************************************************************
 *                WTSQuerySessionInformationW  (WTSAPI32.@)
 */
BOOL WINAPI WTSQuerySessionInformationW(
    HANDLE hServer,
    DWORD SessionId,
    WTS_INFO_CLASS WTSInfoClass,
    LPWSTR* Buffer,
    DWORD* BytesReturned)
{
    /* FIXME: Forward request to winsta.dll::WinStationQueryInformationW */
    FIXME("Stub %p 0x%08x %d %p %p\n", hServer, SessionId, WTSInfoClass,
        Buffer, BytesReturned);

    return FALSE;
}

/************************************************************
 *                WTSWaitSystemEvent (WTSAPI32.@)
 */
BOOL WINAPI WTSWaitSystemEvent(HANDLE hServer, DWORD Mask, DWORD* Flags)
{
    /* FIXME: Forward request to winsta.dll::WinStationWaitSystemEvent */
    FIXME("Stub %p 0x%08x %p\n", hServer, Mask, Flags);
    return FALSE;
}
