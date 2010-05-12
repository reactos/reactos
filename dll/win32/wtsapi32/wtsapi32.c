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

#define WIN32_NO_STATUS
#include "config.h"
#include <stdarg.h>
#include "windef.h"

#include "winbase.h"
#include "wtsapi32.h"
#include "winnls.h"
#include "aclapi.h"
#include "debug.h"

#if defined(_MSC_VER)
    #include "ntstatus.h"
#endif

#include "ketypes.h"
#include "extypes.h"
#include "exfuncs.h"
#include "rtlfuncs.h"

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

static PVOID WTSMallocMemory(SIZE_T nSize);

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
*                QueryProcesses
* Helper function for getting processes list from NtQuerySystemInformation
*/
static PSYSTEM_PROCESS_INFORMATION QueryProcesses()
{
    PSYSTEM_PROCESS_INFORMATION SysProcessesInfo = NULL;
    NTSTATUS Status;
    ULONG BufferSize = 0x8000;
    ULONG ReturnedBufferSize = 0;
    do
    {
        /* free the buffer, and reallocate it to the new size. RATIONALE: since we
        ignore the buffer's contents at this point, there's no point in a realloc()
        that could end up copying a large chunk of data we'd discard anyway */
        WTSFreeMemory(SysProcessesInfo);
        SysProcessesInfo = (PSYSTEM_PROCESS_INFORMATION)WTSMallocMemory(BufferSize);

        if (SysProcessesInfo == NULL)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }

        /* query the information */
        Status = NtQuerySystemInformation(SystemProcessInformation,
                                          SysProcessesInfo,
                                          BufferSize,
                                          &ReturnedBufferSize);

        /* adjust necessary buffer size with returned value or double its size */
        BufferSize = ReturnedBufferSize;
    }
    while (Status == STATUS_INFO_LENGTH_MISMATCH);
    return SysProcessesInfo;
}

/************************************************************
*                GetNextProcess
* Helper function for iterating NtQuerySystemInformation response
*/
static PSYSTEM_PROCESS_INFORMATION GetNextProcess(PSYSTEM_PROCESS_INFORMATION Process)
{
    if (Process->NextEntryOffset == 0)
    {
        return NULL;
    }
    return (PSYSTEM_PROCESS_INFORMATION)((LPBYTE)Process + Process->NextEntryOffset);
}

/************************************************************
*                CountProcesses
* Helper function for calculating process count
* Also calculates necessary space for ImageName unicode strings
*/
static DWORD CountProcesses(IN PSYSTEM_PROCESS_INFORMATION Process, OUT PDWORD pImageNameLength)
{
    DWORD ProcessCount = 0;
    DWORD Length = 0;
    *pImageNameLength = 0;
    while (Process != NULL)
    {
        ++ProcessCount;
        Length = Process->ImageName.Length + sizeof(WCHAR);
        *pImageNameLength += ALIGN_UP(Length, 8);
        Process = GetNextProcess(Process);
    }
    return ProcessCount;
}
/************************************************************
*                GetProcessOwner
* Helper function for getting owner SID for process
*/
static BOOL GetProcessOwner(DWORD ProcessId, PSID pSid, DWORD BufferSize)
{
    BOOL Success = FALSE;
    HANDLE hProcess = NULL;
    PSID ProcessUser = NULL;
    PSECURITY_DESCRIPTOR ProcessSD = NULL;
    DWORD Error;
    hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | READ_CONTROL, FALSE, ProcessId);
    if (hProcess != NULL)
    {
        ProcessUser = NULL;
        ProcessSD = NULL;
        Error = GetSecurityInfo(hProcess,
                                SE_KERNEL_OBJECT,
                                OWNER_SECURITY_INFORMATION,
                                &ProcessUser,
                                NULL,
                                NULL,
                                NULL,
                                &ProcessSD);
        if (!Error)
        {
            if (ProcessUser != NULL)
            {
                Success = !RtlCopySid(BufferSize, pSid, ProcessUser);
            }
            LocalFree(ProcessSD);
        }
        CloseHandle(hProcess);
    }
    return Success;
}

/************************************************************
 *                WTSEnumerateProcessesA  (WTSAPI32.@)
 */
BOOL WINAPI WTSEnumerateProcessesA(HANDLE hServer, DWORD Reserved, DWORD Version,
                                   PWTS_PROCESS_INFOA* ppProcessInfo, DWORD* pCount)
{
    PSYSTEM_PROCESS_INFORMATION SysProcessInfo = NULL;
    PSYSTEM_PROCESS_INFORMATION SysProcess;
    PBYTE Data;
    PWTS_PROCESS_INFOA Process;
    ULONG BufferSize = 0;
    DWORD ProcessCount = 0;
    DWORD Offset;
    DWORD Length;
    DWORD ProcessId;

    if (!ppProcessInfo || !pCount) return FALSE;

    *pCount = 0;

    if (Version != 1)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (hServer != WTS_CURRENT_SERVER_HANDLE)
    {
        FIXME("Stub %p 0x%08x 0x%08x %p %p\n", hServer, Reserved, Version,
              ppProcessInfo, pCount);
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        return FALSE;
    }

    SysProcessInfo = QueryProcesses();
    if (SysProcessInfo == NULL)
    {
        return FALSE;
    }
    // Calculates buffer size for processes information
    ProcessCount = CountProcesses(SysProcessInfo, &BufferSize);
    // Doubles space for strings (in case of UTF-8 or UTF-7 is used as default code page)
    BufferSize *= 2;
    // And count space for records and SIDs
    BufferSize += ProcessCount * (sizeof(WTS_PROCESS_INFOA) + SECURITY_MAX_SID_SIZE);

    Data = (PBYTE)WTSMallocMemory(BufferSize);
    if (Data == NULL)
    {
        WTSFreeMemory(SysProcessInfo);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    // WTS_PROCESS_INFOW structures are put in beginning of the buffer
    // FileName paths and SIDs should be located later after this part
    // That way user will be able to free buffer memory 
    // with single WTSFreeMemory call
    for (SysProcess = SysProcessInfo, Process = (PWTS_PROCESS_INFOA)Data,
         Offset = ProcessCount * sizeof(WTS_PROCESS_INFOA);
         SysProcess != NULL;
         SysProcess = GetNextProcess(SysProcess), ++Process)
    {
        ProcessId = PtrToUint(SysProcess->UniqueProcessId);
        Process->SessionId = SysProcess->SessionId;
        // Get unique process id
        Process->ProcessId = ProcessId;
        Process->pProcessName = (LPSTR)(Data + Offset);
        RtlUnicodeToMultiByteN(Process->pProcessName, BufferSize - Offset, &Length,
                        SysProcess->ImageName.Buffer, SysProcess->ImageName.Length);
        Process->pProcessName[Length++] = 0;
        Offset += ALIGN_UP(Length, 8);
        Process->pUserSid = NULL;
        if (ProcessId > 0)
        {
            Process->pUserSid = (PSID)(Data + Offset);
            if (GetProcessOwner(ProcessId, Process->pUserSid, BufferSize - Offset))
            {
                Length = RtlLengthSid(Process->pUserSid);
                Offset += ALIGN_UP(Length, 8);
            }
            else
            {
                Process->pUserSid = NULL;
            }
        }
    }

    WTSFreeMemory(SysProcessInfo);

    // Now we may assign output values
    *pCount = ProcessCount;
    *ppProcessInfo = (PWTS_PROCESS_INFOA)Data;

    return TRUE;
}

/************************************************************
 *                WTSEnumerateProcessesW  (WTSAPI32.@)
 */
BOOL WINAPI WTSEnumerateProcessesW(HANDLE hServer, DWORD Reserved, DWORD Version,
                                   PWTS_PROCESS_INFOW* ppProcessInfo, DWORD* pCount)
{
    PSYSTEM_PROCESS_INFORMATION SysProcessInfo = NULL;
    PSYSTEM_PROCESS_INFORMATION SysProcess;
    PBYTE Data;
    PWTS_PROCESS_INFOW Process;
    ULONG BufferSize;
    DWORD ProcessCount = 0;
    DWORD Offset;
    DWORD Length;
    DWORD ProcessId;

    if (!ppProcessInfo || !pCount) return FALSE;

    *pCount = 0;

    if (Version != 1)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (hServer != WTS_CURRENT_SERVER_HANDLE)
    {
        FIXME("Stub %p 0x%08x 0x%08x %p %p\n", hServer, Reserved, Version,
              ppProcessInfo, pCount);
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        return FALSE;
    }

    SysProcessInfo = QueryProcesses();
    if (SysProcessInfo == NULL)
    {
        return FALSE;
    }
    /* Calculating necessary buffer length */
    ProcessCount = CountProcesses(SysProcessInfo, &BufferSize);
    BufferSize += ProcessCount * (sizeof(WTS_PROCESS_INFOW) + SECURITY_MAX_SID_SIZE);

    Data = (PBYTE)WTSMallocMemory(BufferSize);
    if (Data == NULL)
    {
        WTSFreeMemory(SysProcessInfo);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    // WTS_PROCESS_INFOW structures are put in beginning of the buffer
    // FileName paths and SIDs should be located later after this part
    // That way user will be able to free buffer memory 
    // with single WTSFreeMemory call
    for (SysProcess = SysProcessInfo, Process = (PWTS_PROCESS_INFOW)Data,
         Offset = ProcessCount * sizeof(WTS_PROCESS_INFOW);
         SysProcess != NULL;
         SysProcess = GetNextProcess(SysProcess), ++Process)
    {
        ProcessId = PtrToUint(SysProcess->UniqueProcessId);
        Process->SessionId = SysProcess->SessionId;
        // Get unique process id
        Process->ProcessId = ProcessId;
        Process->pProcessName = (LPWSTR)(Data + Offset);
        if (SysProcess->ImageName.Buffer != NULL)
        {
            Length = SysProcess->ImageName.Length + sizeof(WCHAR);
            RtlCopyMemory(Process->pProcessName, SysProcess->ImageName.Buffer, Length);
        }
        else
        {
            *Process->pProcessName = L'\0';
            Length = sizeof(*Process->pProcessName);
        }
        Offset += ALIGN_UP(Length, 8);
        Process->pUserSid = NULL;
        if (ProcessId > 0)
        {
            Process->pUserSid = (PSID)(Data + Offset);
            if (GetProcessOwner(ProcessId, Process->pUserSid, BufferSize - Offset))
            {
                Length = RtlLengthSid(Process->pUserSid);
                Offset += ALIGN_UP(Length, 8);
            }
            else
            {
                Process->pUserSid = NULL;
            }
        }
    }

    WTSFreeMemory(SysProcessInfo);

    // Now we may assign output values
    *pCount = ProcessCount;
    *ppProcessInfo = (PWTS_PROCESS_INFOW)Data;

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
*                WTSMallocMemory
* Complimentary function to WTSFreeMemory from API
*/
static PVOID WTSMallocMemory(SIZE_T nSize)
{
    return HeapAlloc(GetProcessHeap(), 0, nSize);
}

/************************************************************
 *                WTSFreeMemory (WTSAPI32.@)
 */
void WINAPI WTSFreeMemory(PVOID pMemory)
{
    HeapFree(GetProcessHeap(), 0, pMemory);
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
 *
 *   Obtains the primary access token of the logged-on user specified by the session ID.
 *
 * PARAMS
 * SessionId [in] -- RDP session identifier
 * phToken [out] -- pointer to the token handle for the logged-on user
 *
 *
 * RETURNS 
 * - On success - pointer to the primary token of the user
 * - On failure - zero
 *
 *
 * NOTES
 * - token handle should be closed after use with CloseHandle
 * - on Failure, extended error information is available via GetLastError
 * 
 */
BOOL WINAPI WTSQueryUserToken(
    ULONG SessionId,
    PHANDLE phToken)
{
    *phToken = (HANDLE)0;
    SetLastError(ERROR_NO_TOKEN);
    FIXME("Stub %d\n", SessionId);
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

/************************************************************
 *                WTSRegisterSessionNotification (WTSAPI32.@)
 */
BOOL WINAPI WTSRegisterSessionNotification(HWND hWnd, DWORD dwFlags)
{
    FIXME("Stub %p 0x%08x\n", hWnd, dwFlags);
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
