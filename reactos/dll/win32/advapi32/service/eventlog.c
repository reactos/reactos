/*
 * Win32 advapi functions
 *
 * Copyright 1995 Sven Verdoolaege
 * Copyright 1998 Juergen Schmied
 * Copyright 2003 Mike Hearn
 * Copyright 2007 Hervé Poussineau
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <advapi32.h>
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(advapi);

static RPC_UNICODE_STRING EmptyString = { 0, 0, L"" };


handle_t __RPC_USER
EVENTLOG_HANDLE_A_bind(EVENTLOG_HANDLE_A UNCServerName)
{
    handle_t hBinding = NULL;
    UCHAR *pszStringBinding;
    RPC_STATUS status;

    TRACE("EVENTLOG_HANDLE_A_bind() called\n");

    status = RpcStringBindingComposeA(NULL,
                                      (UCHAR *)"ncacn_np",
                                      (UCHAR *)UNCServerName,
                                      (UCHAR *)"\\pipe\\ntsvcs",
                                      NULL,
                                      (UCHAR **)&pszStringBinding);
    if (status)
    {
        ERR("RpcStringBindingCompose returned 0x%x\n", status);
        return NULL;
    }

    /* Set the binding handle that will be used to bind to the server. */
    status = RpcBindingFromStringBindingA(pszStringBinding,
                                          &hBinding);
    if (status)
    {
        ERR("RpcBindingFromStringBinding returned 0x%x\n", status);
    }

    status = RpcStringFreeA(&pszStringBinding);
    if (status)
    {
        ERR("RpcStringFree returned 0x%x\n", status);
    }

    return hBinding;
}


void __RPC_USER
EVENTLOG_HANDLE_A_unbind(EVENTLOG_HANDLE_A UNCServerName,
                         handle_t hBinding)
{
    RPC_STATUS status;

    TRACE("EVENTLOG_HANDLE_A_unbind() called\n");

    status = RpcBindingFree(&hBinding);
    if (status)
    {
        ERR("RpcBindingFree returned 0x%x\n", status);
    }
}


handle_t __RPC_USER
EVENTLOG_HANDLE_W_bind(EVENTLOG_HANDLE_W UNCServerName)
{
    handle_t hBinding = NULL;
    LPWSTR pszStringBinding;
    RPC_STATUS status;

    TRACE("EVENTLOG_HANDLE_W_bind() called\n");

    status = RpcStringBindingComposeW(NULL,
                                      L"ncacn_np",
                                      (LPWSTR)UNCServerName,
                                      L"\\pipe\\EventLog",
                                      NULL,
                                      &pszStringBinding);
    if (status)
    {
        ERR("RpcStringBindingCompose returned 0x%x\n", status);
        return NULL;
    }

    /* Set the binding handle that will be used to bind to the server. */
    status = RpcBindingFromStringBindingW(pszStringBinding,
                                          &hBinding);
    if (status)
    {
        ERR("RpcBindingFromStringBinding returned 0x%x\n", status);
    }

    status = RpcStringFreeW(&pszStringBinding);
    if (status)
    {
        ERR("RpcStringFree returned 0x%x\n", status);
    }

    return hBinding;
}


void __RPC_USER
EVENTLOG_HANDLE_W_unbind(EVENTLOG_HANDLE_W UNCServerName,
                         handle_t hBinding)
{
    RPC_STATUS status;

    TRACE("EVENTLOG_HANDLE_W_unbind() called\n");

    status = RpcBindingFree(&hBinding);
    if (status)
    {
        ERR("RpcBindingFree returned 0x%x\n", status);
    }
}


/******************************************************************************
 * BackupEventLogA [ADVAPI32.@]
 */
BOOL WINAPI
BackupEventLogA(IN HANDLE hEventLog,
                IN LPCSTR lpBackupFileName)
{
    RPC_STRING BackupFileName;
    NTSTATUS Status;

    TRACE("%p, %s\n", hEventLog, lpBackupFileName);

    BackupFileName.Buffer = (LPSTR)lpBackupFileName;
    BackupFileName.Length = BackupFileName.MaximumLength =
        lpBackupFileName ? strlen(lpBackupFileName) : 0;

    RpcTryExcept
    {
        Status = ElfrBackupELFA(hEventLog,
                                &BackupFileName);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

/******************************************************************************
 * BackupEventLogW [ADVAPI32.@]
 *
 * PARAMS
 *   hEventLog        []
 *   lpBackupFileName []
 */
BOOL WINAPI
BackupEventLogW(IN HANDLE hEventLog,
                IN LPCWSTR lpBackupFileName)
{
    RPC_UNICODE_STRING BackupFileName;
    NTSTATUS Status;

    TRACE("%p, %s\n", hEventLog, debugstr_w(lpBackupFileName));

    BackupFileName.Buffer = (LPWSTR)lpBackupFileName;
    BackupFileName.Length = BackupFileName.MaximumLength =
        lpBackupFileName ? wcslen(lpBackupFileName) * sizeof(WCHAR) : 0;

    RpcTryExcept
    {
        Status = ElfrBackupELFW(hEventLog,
                                &BackupFileName);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}


/******************************************************************************
 * ClearEventLogA [ADVAPI32.@]
 */
BOOL WINAPI
ClearEventLogA(IN HANDLE hEventLog,
               IN LPCSTR lpBackupFileName)
{
    RPC_STRING BackupFileName;
    NTSTATUS Status;

    TRACE("%p, %s\n", hEventLog, lpBackupFileName);

    BackupFileName.Buffer = (LPSTR)lpBackupFileName;
    BackupFileName.Length = BackupFileName.MaximumLength =
        lpBackupFileName ? strlen(lpBackupFileName) : 0;

    RpcTryExcept
    {
        Status = ElfrClearELFA(hEventLog,
                               &BackupFileName);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}


/******************************************************************************
 * ClearEventLogW [ADVAPI32.@]
 */
BOOL WINAPI
ClearEventLogW(IN HANDLE hEventLog,
               IN LPCWSTR lpBackupFileName)
{
    RPC_UNICODE_STRING BackupFileName;
    NTSTATUS Status;

    TRACE("%p, %s\n", hEventLog, debugstr_w(lpBackupFileName));

    BackupFileName.Buffer = (LPWSTR)lpBackupFileName;
    BackupFileName.Length = BackupFileName.MaximumLength =
        lpBackupFileName ? wcslen(lpBackupFileName) * sizeof(WCHAR) : 0;

    RpcTryExcept
    {
        Status = ElfrClearELFW(hEventLog,
                               &BackupFileName);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}


/******************************************************************************
 * CloseEventLog [ADVAPI32.@]
 */
BOOL WINAPI
CloseEventLog(IN HANDLE hEventLog)
{
    NTSTATUS Status;

    TRACE("%p\n", hEventLog);

    RpcTryExcept
    {
        Status = ElfrCloseEL(&hEventLog);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}


/******************************************************************************
 * DeregisterEventSource [ADVAPI32.@]
 * Closes a handle to the specified event log
 *
 * PARAMS
 *    hEventLog [I] Handle to event log
 *
 * RETURNS STD
 */
BOOL WINAPI
DeregisterEventSource(IN HANDLE hEventLog)
{
    NTSTATUS Status;

    TRACE("%p\n", hEventLog);

    RpcTryExcept
    {
        Status = ElfrDeregisterEventSource(&hEventLog);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}


/******************************************************************************
 * GetNumberOfEventLogRecords [ADVAPI32.@]
 *
 * PARAMS
 *   hEventLog       []
 *   NumberOfRecords []
 */
BOOL WINAPI
GetNumberOfEventLogRecords(IN HANDLE hEventLog,
                           OUT PDWORD NumberOfRecords)
{
    NTSTATUS Status;
    DWORD Records;

    TRACE("%p, %p\n", hEventLog, NumberOfRecords);

    RpcTryExcept
    {
        Status = ElfrNumberOfRecords(hEventLog,
                                     &Records);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    *NumberOfRecords = Records;

    return TRUE;
}


/******************************************************************************
 * GetOldestEventLogRecord [ADVAPI32.@]
 *
 * PARAMS
 *   hEventLog    []
 *   OldestRecord []
 */
BOOL WINAPI
GetOldestEventLogRecord(IN HANDLE hEventLog,
                        OUT PDWORD OldestRecord)
{
    NTSTATUS Status;
    DWORD Oldest;

    TRACE("%p, %p\n", hEventLog, OldestRecord);

    RpcTryExcept
    {
        Status = ElfrOldestRecord(hEventLog,
                                  &Oldest);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    *OldestRecord = Oldest;

    return TRUE;
}


/******************************************************************************
 * NotifyChangeEventLog [ADVAPI32.@]
 *
 * PARAMS
 *   hEventLog []
 *   hEvent    []
 */
BOOL WINAPI
NotifyChangeEventLog(IN HANDLE hEventLog,
                     IN HANDLE hEvent)
{
    /* Use ElfrChangeNotify */
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/******************************************************************************
 * OpenBackupEventLogA [ADVAPI32.@]
 */
HANDLE WINAPI
OpenBackupEventLogA(IN LPCSTR lpUNCServerName,
                    IN LPCSTR lpFileName)
{
    UNICODE_STRING UNCServerName;
    UNICODE_STRING FileName;
    HANDLE Handle;

    TRACE("%s, %s\n", lpUNCServerName, lpFileName);

    if (!RtlCreateUnicodeStringFromAsciiz(&UNCServerName, lpUNCServerName))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }

    if (!RtlCreateUnicodeStringFromAsciiz(&FileName, lpFileName))
    {
        RtlFreeUnicodeString(&UNCServerName);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }

    Handle = OpenBackupEventLogW(UNCServerName.Buffer,
                                 FileName.Buffer);

    RtlFreeUnicodeString(&UNCServerName);
    RtlFreeUnicodeString(&FileName);

    return Handle;
}


/******************************************************************************
 * OpenBackupEventLogW [ADVAPI32.@]
 *
 * PARAMS
 *   lpUNCServerName []
 *   lpFileName      []
 */
HANDLE WINAPI
OpenBackupEventLogW(IN LPCWSTR lpUNCServerName,
                    IN LPCWSTR lpFileName)
{
    RPC_UNICODE_STRING FileName;
    IELF_HANDLE LogHandle;
    NTSTATUS Status;

    TRACE("%s, %s\n", debugstr_w(lpUNCServerName), debugstr_w(lpFileName));

    FileName.Buffer = (LPWSTR)lpFileName;
    FileName.Length = FileName.MaximumLength =
        lpFileName ? wcslen(lpFileName) * sizeof(WCHAR) : 0;

    RpcTryExcept
    {
        Status = ElfrOpenBELW((LPWSTR)lpUNCServerName,
                              &FileName,
                              1,
                              1,
                              &LogHandle);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return NULL;
    }

    return (HANDLE)LogHandle;
}


/******************************************************************************
 * OpenEventLogA [ADVAPI32.@]
 */
HANDLE WINAPI
OpenEventLogA(IN LPCSTR lpUNCServerName,
              IN LPCSTR lpSourceName)
{
    UNICODE_STRING UNCServerName;
    UNICODE_STRING SourceName;
    HANDLE Handle;

    if (!RtlCreateUnicodeStringFromAsciiz(&UNCServerName, lpUNCServerName))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }

    if (!RtlCreateUnicodeStringFromAsciiz(&SourceName, lpSourceName))
    {
        RtlFreeUnicodeString(&UNCServerName);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }

    Handle = OpenEventLogW(UNCServerName.Buffer,
                           SourceName.Buffer);

    RtlFreeUnicodeString(&UNCServerName);
    RtlFreeUnicodeString(&SourceName);

    return Handle;
}


/******************************************************************************
 * OpenEventLogW [ADVAPI32.@]
 *
 * PARAMS
 *   lpUNCServerName []
 *   lpSourceName    []
 */
HANDLE WINAPI
OpenEventLogW(IN LPCWSTR lpUNCServerName,
              IN LPCWSTR lpSourceName)
{
    RPC_UNICODE_STRING SourceName;
    IELF_HANDLE LogHandle;
    NTSTATUS Status;

    TRACE("%s, %s\n", debugstr_w(lpUNCServerName), debugstr_w(lpSourceName));

    SourceName.Buffer = (LPWSTR)lpSourceName;
    SourceName.Length = SourceName.MaximumLength =
        lpSourceName ? wcslen(lpSourceName) * sizeof(WCHAR) : 0;

    RpcTryExcept
    {
        Status = ElfrOpenELW((LPWSTR)lpUNCServerName,
                             &SourceName,
                             &EmptyString,
                             1,
                             1,
                             &LogHandle);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return NULL;
    }

    return (HANDLE)LogHandle;
}


/******************************************************************************
 * ReadEventLogA [ADVAPI32.@]
 */
BOOL WINAPI
ReadEventLogA(IN HANDLE hEventLog,
              IN DWORD dwReadFlags,
              IN DWORD dwRecordOffset,
              OUT LPVOID lpBuffer,
              IN DWORD nNumberOfBytesToRead,
              OUT DWORD *pnBytesRead,
              OUT DWORD *pnMinNumberOfBytesNeeded)
{
    NTSTATUS Status;
    DWORD bytesRead, minNumberOfBytesNeeded;

    TRACE("%p, %lu, %lu, %p, %lu, %p, %p\n",
        hEventLog, dwReadFlags, dwRecordOffset, lpBuffer,
        nNumberOfBytesToRead, pnBytesRead, pnMinNumberOfBytesNeeded);

    /* If buffer is NULL set nNumberOfBytesToRead to 0 to prevent rpcrt4 from
       trying to access a null pointer */
    if (!lpBuffer)
    {
        nNumberOfBytesToRead = 0;
    }

    RpcTryExcept
    {
        Status = ElfrReadELA(hEventLog,
                             dwReadFlags,
                             dwRecordOffset,
                             nNumberOfBytesToRead,
                             lpBuffer,
                             &bytesRead,
                             &minNumberOfBytesNeeded);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    *pnBytesRead = (DWORD)bytesRead;
    *pnMinNumberOfBytesNeeded = (DWORD)minNumberOfBytesNeeded;

    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}


/******************************************************************************
 * ReadEventLogW [ADVAPI32.@]
 *
 * PARAMS
 *   hEventLog                []
 *   dwReadFlags              []
 *   dwRecordOffset           []
 *   lpBuffer                 []
 *   nNumberOfBytesToRead     []
 *   pnBytesRead              []
 *   pnMinNumberOfBytesNeeded []
 */
BOOL WINAPI
ReadEventLogW(IN HANDLE hEventLog,
              IN DWORD dwReadFlags,
              IN DWORD dwRecordOffset,
              OUT LPVOID lpBuffer,
              IN DWORD nNumberOfBytesToRead,
              OUT DWORD *pnBytesRead,
              OUT DWORD *pnMinNumberOfBytesNeeded)
{
    NTSTATUS Status;
    DWORD bytesRead, minNumberOfBytesNeeded;

    TRACE("%p, %lu, %lu, %p, %lu, %p, %p\n",
        hEventLog, dwReadFlags, dwRecordOffset, lpBuffer,
        nNumberOfBytesToRead, pnBytesRead, pnMinNumberOfBytesNeeded);

    /* If buffer is NULL set nNumberOfBytesToRead to 0 to prevent rpcrt4 from
       trying to access a null pointer */
    if (!lpBuffer)
    {
        nNumberOfBytesToRead = 0;
    }

    RpcTryExcept
    {
        Status = ElfrReadELW(hEventLog,
                             dwReadFlags,
                             dwRecordOffset,
                             nNumberOfBytesToRead,
                             lpBuffer,
                             &bytesRead,
                             &minNumberOfBytesNeeded);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    *pnBytesRead = (DWORD)bytesRead;
    *pnMinNumberOfBytesNeeded = (DWORD)minNumberOfBytesNeeded;

    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}


/******************************************************************************
 * RegisterEventSourceA [ADVAPI32.@]
 */
HANDLE WINAPI
RegisterEventSourceA(IN LPCSTR lpUNCServerName,
                     IN LPCSTR lpSourceName)
{
    UNICODE_STRING UNCServerName;
    UNICODE_STRING SourceName;
    HANDLE Handle;

    TRACE("%s, %s\n", lpUNCServerName, lpSourceName);

    if (!RtlCreateUnicodeStringFromAsciiz(&UNCServerName, lpUNCServerName))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }

    if (!RtlCreateUnicodeStringFromAsciiz(&SourceName, lpSourceName))
    {
        RtlFreeUnicodeString(&UNCServerName);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }

    Handle = RegisterEventSourceW(UNCServerName.Buffer,
                                  SourceName.Buffer);

    RtlFreeUnicodeString(&UNCServerName);
    RtlFreeUnicodeString(&SourceName);

    return Handle;
}


/******************************************************************************
 * RegisterEventSourceW [ADVAPI32.@]
 * Returns a registered handle to an event log
 *
 * PARAMS
 *   lpUNCServerName [I] Server name for source
 *   lpSourceName    [I] Source name for registered handle
 *
 * RETURNS
 *    Success: Handle
 *    Failure: NULL
 */
HANDLE WINAPI
RegisterEventSourceW(IN LPCWSTR lpUNCServerName,
                     IN LPCWSTR lpSourceName)
{
    RPC_UNICODE_STRING SourceName;
    IELF_HANDLE LogHandle;
    NTSTATUS Status;

    TRACE("%s, %s\n", debugstr_w(lpUNCServerName), debugstr_w(lpSourceName));

    SourceName.Buffer = (LPWSTR)lpSourceName;
    SourceName.Length = SourceName.MaximumLength =
        lpSourceName ? wcslen(lpSourceName) * sizeof(WCHAR) : 0;

    RpcTryExcept
    {
        Status = ElfrRegisterEventSourceW((LPWSTR)lpUNCServerName,
                                          &SourceName,
                                          &EmptyString,
                                          1,
                                          1,
                                          &LogHandle);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return NULL;
    }

    return (HANDLE)LogHandle;
}


/******************************************************************************
 * ReportEventA [ADVAPI32.@]
 */
BOOL WINAPI
ReportEventA(IN HANDLE hEventLog,
             IN WORD wType,
             IN WORD wCategory,
             IN DWORD dwEventID,
             IN PSID lpUserSid,
             IN WORD wNumStrings,
             IN DWORD dwDataSize,
             IN LPCSTR *lpStrings,
             IN LPVOID lpRawData)
{
    LPCWSTR *wideStrArray;
    UNICODE_STRING str;
    WORD i;
    BOOL ret;

    if (wNumStrings == 0)
        return TRUE;

    if (lpStrings == NULL)
        return TRUE;

    wideStrArray = HeapAlloc(GetProcessHeap(),
                             HEAP_ZERO_MEMORY,
                             sizeof(LPCWSTR) * wNumStrings);

    for (i = 0; i < wNumStrings; i++)
    {
        if (!RtlCreateUnicodeStringFromAsciiz(&str, (PSTR)lpStrings[i]))
            break;
        wideStrArray[i] = str.Buffer;
    }

    if (i == wNumStrings)
    {
        ret = ReportEventW(hEventLog,
                           wType,
                           wCategory,
                           dwEventID,
                           lpUserSid,
                           wNumStrings,
                           dwDataSize,
                           wideStrArray,
                           lpRawData);
    }
    else
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        ret = FALSE;
    }

    for (i = 0; i < wNumStrings; i++)
    {
        if (wideStrArray[i])
        {
            HeapFree(GetProcessHeap(),
                     0,
                     (PVOID)wideStrArray[i]);
        }
    }

    HeapFree(GetProcessHeap(),
             0,
             (PVOID)wideStrArray);

    return ret;
}


/******************************************************************************
 * ReportEventW [ADVAPI32.@]
 *
 * PARAMS
 *   hEventLog   []
 *   wType       []
 *   wCategory   []
 *   dwEventID   []
 *   lpUserSid   []
 *   wNumStrings []
 *   dwDataSize  []
 *   lpStrings   []
 *   lpRawData   []
 */
BOOL WINAPI
ReportEventW(IN HANDLE hEventLog,
             IN WORD wType,
             IN WORD wCategory,
             IN DWORD dwEventID,
             IN PSID lpUserSid,
             IN WORD wNumStrings,
             IN DWORD dwDataSize,
             IN LPCWSTR *lpStrings,
             IN LPVOID lpRawData)
{
    NTSTATUS Status;
    UNICODE_STRING *Strings;
    UNICODE_STRING ComputerName;
    WORD i;

    TRACE("%p, %u, %u, %lu, %p, %u, %lu, %p, %p\n",
          hEventLog, wType, wCategory, dwEventID, lpUserSid,
          wNumStrings, dwDataSize, lpStrings, lpRawData);

    Strings = HeapAlloc(GetProcessHeap(),
                        0,
                        wNumStrings * sizeof(UNICODE_STRING));
    if (!Strings)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    for (i = 0; i < wNumStrings; i++)
        RtlInitUnicodeString(&Strings[i], lpStrings[i]);

    /*FIXME: ComputerName */
    RtlInitUnicodeString(&ComputerName, L"");

    RpcTryExcept
    {
        Status = ElfrReportEventW(hEventLog,
                                  0, /* FIXME: Time */
                                  wType,
                                  wCategory,
                                  dwEventID,
                                  wNumStrings,
                                  dwDataSize,
                                  (PRPC_UNICODE_STRING) &ComputerName,
                                  lpUserSid,
                                  (PRPC_UNICODE_STRING*) &Strings,
                                  lpRawData,
                                  0,
                                  NULL,
                                  NULL);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    HeapFree(GetProcessHeap(), 0, Strings);

    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}
