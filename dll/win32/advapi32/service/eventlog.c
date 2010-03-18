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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <advapi32.h>
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(advapi);

static RPC_UNICODE_STRING EmptyStringU = { 0, 0, L"" };
static RPC_STRING EmptyStringA = { 0, 0, "" };


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
                                      (UCHAR *)"\\pipe\\EventLog",
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
    ANSI_STRING BackupFileName;
    NTSTATUS Status;

    TRACE("%p, %s\n", hEventLog, lpBackupFileName);

    RtlInitAnsiString(&BackupFileName, lpBackupFileName);

    RpcTryExcept
    {
        Status = ElfrBackupELFA(hEventLog,
                                (PRPC_STRING)&BackupFileName);
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
    UNICODE_STRING BackupFileName;
    NTSTATUS Status;

    TRACE("%p, %s\n", hEventLog, debugstr_w(lpBackupFileName));

    RtlInitUnicodeString(&BackupFileName, lpBackupFileName);

    RpcTryExcept
    {
        Status = ElfrBackupELFW(hEventLog,
                                (PRPC_UNICODE_STRING)&BackupFileName);
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
    ANSI_STRING BackupFileName;
    NTSTATUS Status;

    TRACE("%p, %s\n", hEventLog, lpBackupFileName);

    RtlInitAnsiString(&BackupFileName, lpBackupFileName);

    RpcTryExcept
    {
        Status = ElfrClearELFA(hEventLog,
                               (PRPC_STRING)&BackupFileName);
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
    UNICODE_STRING BackupFileName;
    NTSTATUS Status;

    TRACE("%p, %s\n", hEventLog, debugstr_w(lpBackupFileName));

    RtlInitUnicodeString(&BackupFileName,lpBackupFileName);

    RpcTryExcept
    {
        Status = ElfrClearELFW(hEventLog,
                               (PRPC_UNICODE_STRING)&BackupFileName);
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
 * GetEventLogInformation [ADVAPI32.@]
 *
 * PARAMS
 *   hEventLog      [I] Handle to event log
 *   dwInfoLevel    [I] Level of event log information to return
 *   lpBuffer       [O] Buffer that receives the event log information
 *   cbBufSize      [I] Size of the lpBuffer buffer
 *   pcbBytesNeeded [O] Required buffer size
 */
BOOL WINAPI
GetEventLogInformation(IN HANDLE hEventLog,
                       IN DWORD dwInfoLevel,
                       OUT LPVOID lpBuffer,
                       IN DWORD cbBufSize,
                       OUT LPDWORD pcbBytesNeeded)
{
    NTSTATUS Status;

    if (dwInfoLevel != EVENTLOG_FULL_INFO)
    {
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    RpcTryExcept
    {
        Status = ElfrGetLogInformation(hEventLog,
                                       dwInfoLevel,
                                       (LPBYTE)lpBuffer,
                                       cbBufSize,
                                       pcbBytesNeeded);
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

    if(!NumberOfRecords)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

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

    if(!OldestRecord)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

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
    ANSI_STRING FileName;
    IELF_HANDLE LogHandle;
    NTSTATUS Status;

    TRACE("%s, %s\n", lpUNCServerName, lpFileName);

    RtlInitAnsiString(&FileName, lpFileName);

    RpcTryExcept
    {
        Status = ElfrOpenBELA((LPSTR)lpUNCServerName,
                              (PRPC_STRING)&FileName,
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
    UNICODE_STRING FileName;
    IELF_HANDLE LogHandle;
    NTSTATUS Status;

    TRACE("%s, %s\n", debugstr_w(lpUNCServerName), debugstr_w(lpFileName));

    RtlInitUnicodeString(&FileName, lpFileName);

    RpcTryExcept
    {
        Status = ElfrOpenBELW((LPWSTR)lpUNCServerName,
                              (PRPC_UNICODE_STRING)&FileName,
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
 *
 * Opens a handle to the specified event log.
 *
 * PARAMS
 *  lpUNCServerName [I] UNC name of the server on which the event log is
 *                      opened.
 *  lpSourceName    [I] Name of the log.
 *
 * RETURNS
 *  Success: Handle to an event log.
 *  Failure: NULL
 */
HANDLE WINAPI
OpenEventLogA(IN LPCSTR lpUNCServerName,
              IN LPCSTR lpSourceName)
{
    LPSTR UNCServerName;
    ANSI_STRING SourceName;
    IELF_HANDLE LogHandle = NULL;
    NTSTATUS Status;

    TRACE("%s, %s\n", lpUNCServerName, lpSourceName);

    if (lpSourceName == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    if (lpUNCServerName == NULL || *lpUNCServerName == 0)
        UNCServerName = NULL;
    else
        UNCServerName = (LPSTR)lpUNCServerName;

    RtlInitAnsiString(&SourceName, lpSourceName);

    RpcTryExcept
    {
        Status = ElfrOpenELA(UNCServerName,
                             (PRPC_STRING)&SourceName,
                             &EmptyStringA,
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
    LPWSTR UNCServerName;
    UNICODE_STRING SourceName;
    IELF_HANDLE LogHandle;
    NTSTATUS Status;

    TRACE("%s, %s\n", debugstr_w(lpUNCServerName), debugstr_w(lpSourceName));

    if (lpSourceName == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    if (lpUNCServerName == NULL || *lpUNCServerName == 0)
        UNCServerName = NULL;
    else
        UNCServerName = (LPWSTR)lpUNCServerName;

    RtlInitUnicodeString(&SourceName, lpSourceName);

    RpcTryExcept
    {
        Status = ElfrOpenELW(UNCServerName,
                             (PRPC_UNICODE_STRING)&SourceName,
                             &EmptyStringU,
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

    if(!pnBytesRead || !pnMinNumberOfBytesNeeded)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

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

    if(!pnBytesRead || !pnMinNumberOfBytesNeeded)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

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
    ANSI_STRING SourceName;
    IELF_HANDLE LogHandle;
    NTSTATUS Status;

    TRACE("%s, %s\n", lpUNCServerName, lpSourceName);

    RtlInitAnsiString(&SourceName, lpSourceName);

    RpcTryExcept
    {
        Status = ElfrRegisterEventSourceA((LPSTR)lpUNCServerName,
                                          (PRPC_STRING)&SourceName,
                                          &EmptyStringA,
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
    UNICODE_STRING SourceName;
    IELF_HANDLE LogHandle;
    NTSTATUS Status;

    TRACE("%s, %s\n", debugstr_w(lpUNCServerName), debugstr_w(lpSourceName));

    RtlInitUnicodeString(&SourceName, lpSourceName);

    RpcTryExcept
    {
        Status = ElfrRegisterEventSourceW((LPWSTR)lpUNCServerName,
                                          (PRPC_UNICODE_STRING)&SourceName,
                                          &EmptyStringU,
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
    NTSTATUS Status;
    ANSI_STRING *Strings;
    ANSI_STRING ComputerName;
    WORD i;

    TRACE("%p, %u, %u, %lu, %p, %u, %lu, %p, %p\n",
          hEventLog, wType, wCategory, dwEventID, lpUserSid,
          wNumStrings, dwDataSize, lpStrings, lpRawData);

    Strings = HeapAlloc(GetProcessHeap(),
                        0,
                        wNumStrings * sizeof(ANSI_STRING));
    if (!Strings)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    for (i = 0; i < wNumStrings; i++)
        RtlInitAnsiString(&Strings[i], lpStrings[i]);

    /*FIXME: ComputerName */
    RtlInitAnsiString(&ComputerName, "");

    RpcTryExcept
    {
        Status = ElfrReportEventA(hEventLog,
                                  0, /* FIXME: Time */
                                  wType,
                                  wCategory,
                                  dwEventID,
                                  wNumStrings,
                                  dwDataSize,
                                  (PRPC_STRING) &ComputerName,
                                  lpUserSid,
                                  (PRPC_STRING*) &Strings,
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
