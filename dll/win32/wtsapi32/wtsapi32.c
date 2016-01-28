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

#include <config.h>
#include <stdarg.h>
//#include <stdlib.h>
#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <wine/winternl.h>
#include <wtsapi32.h>
#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(wtsapi);


/************************************************************
 *                WTSCloseServer  (WTSAPI32.@)
 */
void WINAPI WTSCloseServer(HANDLE hServer)
{
    FIXME("Stub %p\n", hServer);
}

/************************************************************
 *                WTSConnectSessionA  (WTSAPI32.@)
 */
BOOL WINAPI WTSConnectSessionA(ULONG LogonId, ULONG TargetLogonId, PSTR pPassword, BOOL bWait)
{
   FIXME("Stub %d %d (%s) %d\n", LogonId, TargetLogonId, debugstr_a(pPassword), bWait);
   return TRUE;
}

/************************************************************
 *                WTSConnectSessionW  (WTSAPI32.@)
 */
BOOL WINAPI WTSConnectSessionW(ULONG LogonId, ULONG TargetLogonId, PWSTR pPassword, BOOL bWait)
{
   FIXME("Stub %d %d (%s) %d\n", LogonId, TargetLogonId, debugstr_w(pPassword), bWait);
   return TRUE;
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
 *                WTSEnableChildSessions  (WTSAPI32.@)
 */
BOOL WINAPI WTSEnableChildSessions(BOOL enable)
{
    FIXME("Stub %d\n", enable);
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
    WTS_PROCESS_INFOW *processInfo;
    SYSTEM_PROCESS_INFORMATION *spi;
    ULONG size = 0x4000;
    void *buf = NULL;
    NTSTATUS status;
    DWORD count;
    WCHAR *name;

    if (!ppProcessInfo || !pCount || Reserved != 0 || Version != 1)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (hServer != WTS_CURRENT_SERVER_HANDLE)
    {
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        return FALSE;
    }

    do
    {
        size *= 2;
        HeapFree(GetProcessHeap(), 0, buf);
        buf = HeapAlloc(GetProcessHeap(), 0, size);
        if (!buf)
        {
            SetLastError(ERROR_OUTOFMEMORY);
            return FALSE;
        }
        status = NtQuerySystemInformation(SystemProcessInformation, buf, size, NULL);
    }
    while (status == STATUS_INFO_LENGTH_MISMATCH);

    if (status != STATUS_SUCCESS)
    {
        HeapFree(GetProcessHeap(), 0, buf);
        SetLastError(RtlNtStatusToDosError(status));
        return FALSE;
    }

    spi = buf;
    count = size = 0;
    for (;;)
    {
        size += sizeof(WTS_PROCESS_INFOW) + spi->ProcessName.Length + sizeof(WCHAR);
        count++;
        if (spi->NextEntryOffset == 0) break;
        spi = (SYSTEM_PROCESS_INFORMATION *)(((PCHAR)spi) + spi->NextEntryOffset);
    }

    processInfo = HeapAlloc(GetProcessHeap(), 0, size);
    if (!processInfo)
    {
        HeapFree(GetProcessHeap(), 0, buf);
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }
    name = (WCHAR *)&processInfo[count];

    *ppProcessInfo = processInfo;
    *pCount = count;

    spi = buf;
    while (count--)
    {
        processInfo->SessionId = 0;
        processInfo->ProcessId = HandleToUlong(spi->UniqueProcessId);
        processInfo->pProcessName = name;
        processInfo->pUserSid = NULL;
        memcpy( name, spi->ProcessName.Buffer, spi->ProcessName.Length );
        name[ spi->ProcessName.Length/sizeof(WCHAR) ] = 0;

        processInfo++;
        name += (spi->ProcessName.Length + sizeof(WCHAR))/sizeof(WCHAR);
        spi = (SYSTEM_PROCESS_INFORMATION *)(((PCHAR)spi) + spi->NextEntryOffset);
    }

    HeapFree(GetProcessHeap(), 0, buf);
    return TRUE;
}

/************************************************************
 *                WTSEnumerateServersA  (WTSAPI32.@)
 */
BOOL WINAPI WTSEnumerateServersA(LPSTR pDomainName, DWORD Reserved, DWORD Version, PWTS_SERVER_INFOA *ppServerInfo, DWORD *pCount)
{
    FIXME("Stub %s 0x%08x 0x%08x %p %p\n", debugstr_a(pDomainName), Reserved, Version, ppServerInfo, pCount);
    return FALSE;
}

/************************************************************
 *                WTSEnumerateServersW  (WTSAPI32.@)
 */
BOOL WINAPI WTSEnumerateServersW(LPWSTR pDomainName, DWORD Reserved, DWORD Version, PWTS_SERVER_INFOW *ppServerInfo, DWORD *pCount)
{
    FIXME("Stub %s 0x%08x 0x%08x %p %p\n", debugstr_w(pDomainName), Reserved, Version, ppServerInfo, pCount);
    return FALSE;
}


/************************************************************
 *                WTSEnumerateEnumerateSessionsA  (WTSAPI32.@)
 */
BOOL WINAPI WTSEnumerateSessionsA(HANDLE hServer, DWORD Reserved, DWORD Version,
    PWTS_SESSION_INFOA* ppSessionInfo, DWORD* pCount)
{
    static int once;

    if (!once++) FIXME("Stub %p 0x%08x 0x%08x %p %p\n", hServer, Reserved, Version,
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
    HeapFree(GetProcessHeap(), 0, pMemory);
}

/************************************************************
 *                WTSLogoffSession (WTSAPI32.@)
 */
BOOL WINAPI WTSLogoffSession(HANDLE hserver, DWORD session_id, BOOL bwait)
{
    FIXME("(%p, 0x%x, %d): stub\n", hserver, session_id, bwait);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
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
 *                WTSQueryUserToken (WTSAPI32.@)
 */
BOOL WINAPI WTSQueryUserToken(ULONG session_id, PHANDLE token)
{
    FIXME("%u %p\n", session_id, token);
    return FALSE;
}

/************************************************************
 *                WTSQueryUserConfigA (WTSAPI32.@)
 */
BOOL WINAPI WTSQueryUserConfigA(LPSTR pServerName, LPSTR pUserName, WTS_CONFIG_CLASS WTSConfigClass, LPSTR *ppBuffer, DWORD *pBytesReturned)
{
   FIXME("Stub (%s) (%s) 0x%08x %p %p\n", debugstr_a(pServerName), debugstr_a(pUserName), WTSConfigClass,
        ppBuffer, pBytesReturned);
   return FALSE;
}

/************************************************************
 *                WTSQueryUserConfigW (WTSAPI32.@)
 */
BOOL WINAPI WTSQueryUserConfigW(LPWSTR pServerName, LPWSTR pUserName, WTS_CONFIG_CLASS WTSConfigClass, LPWSTR *ppBuffer, DWORD *pBytesReturned)
{
   FIXME("Stub (%s) (%s) 0x%08x %p %p\n", debugstr_w(pServerName), debugstr_w(pUserName), WTSConfigClass,
        ppBuffer, pBytesReturned);
   return FALSE;
}


/************************************************************
 *                WTSRegisterSessionNotification (WTSAPI32.@)
 */
BOOL WINAPI WTSRegisterSessionNotification(HWND hWnd, DWORD dwFlags)
{
    FIXME("Stub %p 0x%08x\n", hWnd, dwFlags);
    return TRUE;
}

/************************************************************
 *                WTSRegisterSessionNotification (WTSAPI32.@)
 */
BOOL WINAPI WTSRegisterSessionNotificationEx(HANDLE hServer, HWND hWnd, DWORD dwFlags)
{
    FIXME("Stub %p %p 0x%08x\n", hServer, hWnd, dwFlags);
    return FALSE;
}


/************************************************************
 *                WTSSendMessageA (WTSAPI32.@)
 */
BOOL WINAPI WTSSendMessageA(HANDLE hServer, DWORD SessionId, LPSTR pTitle, DWORD TitleLength, LPSTR pMessage,
   DWORD MessageLength, DWORD Style, DWORD Timeout, DWORD *pResponse, BOOL bWait)
{
   FIXME("Stub %p 0x%08x (%s) %d (%s) %d 0x%08x %d %p %d\n", hServer, SessionId, debugstr_a(pTitle), TitleLength, debugstr_a(pMessage), MessageLength, Style, Timeout, pResponse, bWait);
   return FALSE;
}

/************************************************************
 *                WTSSendMessageW (WTSAPI32.@)
 */
BOOL WINAPI WTSSendMessageW(HANDLE hServer, DWORD SessionId, LPWSTR pTitle, DWORD TitleLength, LPWSTR pMessage,
   DWORD MessageLength, DWORD Style, DWORD Timeout, DWORD *pResponse, BOOL bWait)
{
   FIXME("Stub %p 0x%08x (%s) %d (%s) %d 0x%08x %d %p %d\n", hServer, SessionId, debugstr_w(pTitle), TitleLength, debugstr_w(pMessage), MessageLength, Style, Timeout, pResponse, bWait);
   return FALSE;
}

/************************************************************
 *                WTSSetUserConfigA (WTSAPI32.@)
 */
BOOL WINAPI WTSSetUserConfigA(LPSTR pServerName, LPSTR pUserName, WTS_CONFIG_CLASS WTSConfigClass, LPSTR pBuffer, DWORD DataLength)
{
   FIXME("Stub (%s) (%s) 0x%08x %p %d\n", debugstr_a(pServerName), debugstr_a(pUserName), WTSConfigClass,pBuffer, DataLength);
   return FALSE;
}

/************************************************************
 *                WTSSetUserConfigW (WTSAPI32.@)
 */
BOOL WINAPI WTSSetUserConfigW(LPWSTR pServerName, LPWSTR pUserName, WTS_CONFIG_CLASS WTSConfigClass, LPWSTR pBuffer, DWORD DataLength)
{
   FIXME("Stub (%s) (%s) 0x%08x %p %d\n", debugstr_w(pServerName), debugstr_w(pUserName), WTSConfigClass,pBuffer, DataLength);
   return FALSE;
}

/************************************************************
 *                WTSShutdownSystem (WTSAPI32.@)
 */
BOOL WINAPI WTSShutdownSystem(HANDLE hServer, DWORD ShutdownFlag)
{
   FIXME("Stub %p 0x%08x\n", hServer,ShutdownFlag);
   return FALSE;
}

/************************************************************
 *                WTSStartRemoteControlSessionA (WTSAPI32.@)
 */
BOOL WINAPI WTSStartRemoteControlSessionA(LPSTR pTargetServerName, ULONG TargetLogonId, BYTE HotkeyVk, USHORT HotkeyModifiers)
{
   FIXME("Stub (%s) %d %d %d\n", debugstr_a(pTargetServerName), TargetLogonId, HotkeyVk, HotkeyModifiers);
   return FALSE;
}

/************************************************************
 *                WTSStartRemoteControlSessionW (WTSAPI32.@)
 */
BOOL WINAPI WTSStartRemoteControlSessionW(LPWSTR pTargetServerName, ULONG TargetLogonId, BYTE HotkeyVk, USHORT HotkeyModifiers)
{
   FIXME("Stub (%s) %d %d %d\n", debugstr_w(pTargetServerName), TargetLogonId, HotkeyVk, HotkeyModifiers);
   return FALSE;
}

/************************************************************
 *                WTSStopRemoteControlSession (WTSAPI32.@)
 */
BOOL WINAPI WTSStopRemoteControlSession(ULONG LogonId)
{
   FIXME("Stub %d\n",  LogonId);
   return FALSE;
}

/************************************************************
 *                WTSTerminateProcess (WTSAPI32.@)
 */
BOOL WINAPI WTSTerminateProcess(HANDLE hServer, DWORD ProcessId, DWORD ExitCode)
{
   FIXME("Stub %p %d %d\n", hServer, ProcessId, ExitCode);
   return FALSE;
}

/************************************************************
 *                WTSUnRegisterSessionNotification (WTSAPI32.@)
 */
BOOL WINAPI WTSUnRegisterSessionNotification(HWND hWnd)
{
    FIXME("Stub %p\n", hWnd);
    return FALSE;
}

/************************************************************
 *                WTSUnRegisterSessionNotification (WTSAPI32.@)
 */
BOOL WINAPI WTSUnRegisterSessionNotificationEx(HANDLE hServer, HWND hWnd)
{
    FIXME("Stub %p %p\n", hServer, hWnd);
    return FALSE;
}


/************************************************************
 *                WTSVirtualChannelClose (WTSAPI32.@)
 */
BOOL WINAPI WTSVirtualChannelClose(HANDLE hChannelHandle)
{
   FIXME("Stub %p\n", hChannelHandle);
   return FALSE;
}

/************************************************************
 *                WTSVirtualChannelOpen (WTSAPI32.@)
 */
HANDLE WINAPI WTSVirtualChannelOpen(HANDLE hServer, DWORD SessionId, LPSTR pVirtualName)
{
   FIXME("Stub %p %d (%s)\n", hServer, SessionId, debugstr_a(pVirtualName));
   return NULL;
}

/************************************************************
 *                WTSVirtualChannelOpen (WTSAPI32.@)
 */
HANDLE WINAPI WTSVirtualChannelOpenEx(DWORD SessionId, LPSTR pVirtualName, DWORD flags)
{
   FIXME("Stub %d (%s) %d\n",  SessionId, debugstr_a(pVirtualName), flags);
   return NULL;
}

/************************************************************
 *                WTSVirtualChannelPurgeInput (WTSAPI32.@)
 */
BOOL WINAPI WTSVirtualChannelPurgeInput(HANDLE hChannelHandle)
{
   FIXME("Stub %p\n", hChannelHandle);
   return FALSE;
}

/************************************************************
 *                WTSVirtualChannelPurgeOutput (WTSAPI32.@)
 */
BOOL WINAPI WTSVirtualChannelPurgeOutput(HANDLE hChannelHandle)
{
   FIXME("Stub %p\n", hChannelHandle);
   return FALSE;
}


/************************************************************
 *                WTSVirtualChannelQuery (WTSAPI32.@)
 */
BOOL WINAPI WTSVirtualChannelQuery(HANDLE hChannelHandle, WTS_VIRTUAL_CLASS WtsVirtualClass, PVOID *ppBuffer, DWORD *pBytesReturned)
{
   FIXME("Stub %p %d %p %p\n", hChannelHandle, WtsVirtualClass, ppBuffer, pBytesReturned);
   return FALSE;
}

/************************************************************
 *                WTSVirtualChannelRead (WTSAPI32.@)
 */
BOOL WINAPI WTSVirtualChannelRead(HANDLE hChannelHandle, ULONG TimeOut, PCHAR Buffer, ULONG BufferSize, PULONG pBytesRead)
{
   FIXME("Stub %p %d %p %d %p\n", hChannelHandle, TimeOut, Buffer, BufferSize, pBytesRead);
   return FALSE;
}

/************************************************************
 *                WTSVirtualChannelWrite (WTSAPI32.@)
 */
BOOL WINAPI WTSVirtualChannelWrite(HANDLE hChannelHandle, PCHAR Buffer, ULONG Length, PULONG pBytesWritten)
{
   FIXME("Stub %p %p %d %p\n", hChannelHandle, Buffer, Length, pBytesWritten);
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
