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

#include <ndk/kefuncs.h>
#include <eventlogrpc_c.h>

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
    ANSI_STRING BackupFileNameA;
    UNICODE_STRING BackupFileNameW;
    NTSTATUS Status;
    BOOL Result;

    TRACE("%p, %s\n", hEventLog, lpBackupFileName);

    if (lpBackupFileName == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    RtlInitAnsiString(&BackupFileNameA, lpBackupFileName);

    Status = RtlAnsiStringToUnicodeString(&BackupFileNameW,
                                          &BackupFileNameA,
                                          TRUE);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    Result = BackupEventLogW(hEventLog,
                             BackupFileNameW.Buffer);

    RtlFreeUnicodeString(&BackupFileNameW);

    return(Result);
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
    UNICODE_STRING BackupFileNameW;
    NTSTATUS Status;

    TRACE("%p, %s\n", hEventLog, debugstr_w(lpBackupFileName));

    if (lpBackupFileName == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!RtlDosPathNameToNtPathName_U(lpBackupFileName, &BackupFileNameW,
                                      NULL, NULL))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    RpcTryExcept
    {
        Status = ElfrBackupELFW(hEventLog,
                                (PRPC_UNICODE_STRING)&BackupFileNameW);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    RtlFreeHeap(RtlGetProcessHeap(), 0, BackupFileNameW.Buffer);

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
    ANSI_STRING BackupFileNameA;
    UNICODE_STRING BackupFileNameW;
    NTSTATUS Status;
    BOOL Result;

    TRACE("%p, %s\n", hEventLog, lpBackupFileName);

    if (lpBackupFileName == NULL)
    {
        RtlInitUnicodeString(&BackupFileNameW, NULL);
    }
    else
    {
        RtlInitAnsiString(&BackupFileNameA, lpBackupFileName);

        Status = RtlAnsiStringToUnicodeString(&BackupFileNameW,
                                              &BackupFileNameA,
                                              TRUE);
        if (!NT_SUCCESS(Status))
        {
            SetLastError(RtlNtStatusToDosError(Status));
            return FALSE;
        }
    }

    Result = ClearEventLogW(hEventLog,
                            BackupFileNameW.Buffer);

    RtlFreeUnicodeString(&BackupFileNameW);

    return Result;
}


/******************************************************************************
 * ClearEventLogW [ADVAPI32.@]
 */
BOOL WINAPI
ClearEventLogW(IN HANDLE hEventLog,
               IN LPCWSTR lpBackupFileName)
{
    UNICODE_STRING BackupFileNameW;
    NTSTATUS Status;

    TRACE("%p, %s\n", hEventLog, debugstr_w(lpBackupFileName));

    if (lpBackupFileName == NULL)
    {
        RtlInitUnicodeString(&BackupFileNameW, NULL);
    }
    else
    {
        if (!RtlDosPathNameToNtPathName_U(lpBackupFileName, &BackupFileNameW,
                                          NULL, NULL))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
    }

    RpcTryExcept
    {
        Status = ElfrClearELFW(hEventLog,
                               (PRPC_UNICODE_STRING)&BackupFileNameW);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    if (lpBackupFileName != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, BackupFileNameW.Buffer);

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

    if (NumberOfRecords == NULL)
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

    if (OldestRecord == NULL)
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
    ANSI_STRING UNCServerNameA;
    UNICODE_STRING UNCServerNameW;
    ANSI_STRING FileNameA;
    UNICODE_STRING FileNameW;
    HANDLE LogHandle;
    NTSTATUS Status;

    TRACE("%s, %s\n", lpUNCServerName, lpFileName);

    /* Convert the server name to unicode */
    if (lpUNCServerName == NULL)
    {
        RtlInitUnicodeString(&UNCServerNameW, NULL);
    }
    else
    {
        RtlInitAnsiString(&UNCServerNameA, lpUNCServerName);

        Status = RtlAnsiStringToUnicodeString(&UNCServerNameW,
                                              &UNCServerNameA,
                                              TRUE);
        if (!NT_SUCCESS(Status))
        {
            SetLastError(RtlNtStatusToDosError(Status));
            return NULL;
        }
    }

    /* Convert the file name to unicode */
    if (lpFileName == NULL)
    {
        RtlInitUnicodeString(&FileNameW, NULL);
    }
    else
    {
        RtlInitAnsiString(&FileNameA, lpFileName);

        Status = RtlAnsiStringToUnicodeString(&FileNameW,
                                              &FileNameA,
                                              TRUE);
        if (!NT_SUCCESS(Status))
        {
            RtlFreeUnicodeString(&UNCServerNameW);
            SetLastError(RtlNtStatusToDosError(Status));
            return NULL;
        }
    }

    /* Call the unicode function */
    LogHandle = OpenBackupEventLogW(UNCServerNameW.Buffer,
                                    FileNameW.Buffer);

    /* Free the unicode strings */
    RtlFreeUnicodeString(&UNCServerNameW);
    RtlFreeUnicodeString(&FileNameW);

    return LogHandle;
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
    UNICODE_STRING FileNameW;
    IELF_HANDLE LogHandle;
    NTSTATUS Status;

    TRACE("%s, %s\n", debugstr_w(lpUNCServerName), debugstr_w(lpFileName));

    if (lpFileName == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    if (!RtlDosPathNameToNtPathName_U(lpFileName, &FileNameW,
                                      NULL, NULL))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    RpcTryExcept
    {
        Status = ElfrOpenBELW((LPWSTR)lpUNCServerName,
                              (PRPC_UNICODE_STRING)&FileNameW,
                              1,
                              1,
                              &LogHandle);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    if (FileNameW.Buffer != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, FileNameW.Buffer);

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
    DWORD dwFlags;

    TRACE("%p, %lu, %lu, %p, %lu, %p, %p\n",
        hEventLog, dwReadFlags, dwRecordOffset, lpBuffer,
        nNumberOfBytesToRead, pnBytesRead, pnMinNumberOfBytesNeeded);

    if (lpBuffer == NULL ||
        pnBytesRead == NULL ||
        pnMinNumberOfBytesNeeded == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    dwFlags = dwReadFlags & (EVENTLOG_SEQUENTIAL_READ | EVENTLOG_SEEK_READ);
    if (dwFlags == (EVENTLOG_SEQUENTIAL_READ | EVENTLOG_SEEK_READ))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    dwFlags = dwReadFlags & (EVENTLOG_FORWARDS_READ | EVENTLOG_BACKWARDS_READ);
    if (dwFlags == (EVENTLOG_FORWARDS_READ | EVENTLOG_BACKWARDS_READ))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
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
    DWORD dwFlags;

    TRACE("%p, %lu, %lu, %p, %lu, %p, %p\n",
        hEventLog, dwReadFlags, dwRecordOffset, lpBuffer,
        nNumberOfBytesToRead, pnBytesRead, pnMinNumberOfBytesNeeded);

    if (lpBuffer == NULL ||
        pnBytesRead == NULL ||
        pnMinNumberOfBytesNeeded == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    dwFlags = dwReadFlags & (EVENTLOG_SEQUENTIAL_READ | EVENTLOG_SEEK_READ);
    if (dwFlags == (EVENTLOG_SEQUENTIAL_READ | EVENTLOG_SEEK_READ))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    dwFlags = dwReadFlags & (EVENTLOG_FORWARDS_READ | EVENTLOG_BACKWARDS_READ);
    if (dwFlags == (EVENTLOG_FORWARDS_READ | EVENTLOG_BACKWARDS_READ))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
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
    PANSI_STRING *Strings;
    ANSI_STRING ComputerName;
    WORD i;
    CHAR szComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD dwSize;
    LARGE_INTEGER SystemTime;
    ULONG Seconds;

    TRACE("%p, %u, %u, %lu, %p, %u, %lu, %p, %p\n",
          hEventLog, wType, wCategory, dwEventID, lpUserSid,
          wNumStrings, dwDataSize, lpStrings, lpRawData);

    Strings = HeapAlloc(GetProcessHeap(),
                        HEAP_ZERO_MEMORY,
                        wNumStrings * sizeof(PANSI_STRING));
    if (!Strings)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    for (i = 0; i < wNumStrings; i++)
    {
        Strings[i] = HeapAlloc(GetProcessHeap(),
                               HEAP_ZERO_MEMORY,
                               sizeof(ANSI_STRING));
        if (Strings[i])
        {
            RtlInitAnsiString(Strings[i], lpStrings[i]);
        }
    }

    dwSize = MAX_COMPUTERNAME_LENGTH + 1;
    GetComputerNameA(szComputerName, &dwSize);
    RtlInitAnsiString(&ComputerName, szComputerName);

    NtQuerySystemTime(&SystemTime);
    RtlTimeToSecondsSince1970(&SystemTime, &Seconds);

    RpcTryExcept
    {
        Status = ElfrReportEventA(hEventLog,
                                  Seconds,
                                  wType,
                                  wCategory,
                                  dwEventID,
                                  wNumStrings,
                                  dwDataSize,
                                  (PRPC_STRING)&ComputerName,
                                  lpUserSid,
                                  (PRPC_STRING*)Strings,
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

    for (i = 0; i < wNumStrings; i++)
    {
        if (Strings[i] != NULL)
            HeapFree(GetProcessHeap(), 0, Strings[i]);
    }

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
    PUNICODE_STRING *Strings;
    UNICODE_STRING ComputerName;
    WORD i;
    WCHAR szComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD dwSize;
    LARGE_INTEGER SystemTime;
    ULONG Seconds;

    TRACE("%p, %u, %u, %lu, %p, %u, %lu, %p, %p\n",
          hEventLog, wType, wCategory, dwEventID, lpUserSid,
          wNumStrings, dwDataSize, lpStrings, lpRawData);

    Strings = HeapAlloc(GetProcessHeap(),
                        0,
                        wNumStrings * sizeof(PUNICODE_STRING));
    if (!Strings)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    for (i = 0; i < wNumStrings; i++)
    {
        Strings[i] = HeapAlloc(GetProcessHeap(),
                               HEAP_ZERO_MEMORY,
                               sizeof(ANSI_STRING));
        if (Strings[i])
        {
            RtlInitUnicodeString(Strings[i], lpStrings[i]);
        }
    }

    dwSize = MAX_COMPUTERNAME_LENGTH + 1;
    GetComputerNameW(szComputerName, &dwSize);
    RtlInitUnicodeString(&ComputerName, szComputerName);

    NtQuerySystemTime(&SystemTime);
    RtlTimeToSecondsSince1970(&SystemTime, &Seconds);

    RpcTryExcept
    {
        Status = ElfrReportEventW(hEventLog,
                                  Seconds,
                                  wType,
                                  wCategory,
                                  dwEventID,
                                  wNumStrings,
                                  dwDataSize,
                                  (PRPC_UNICODE_STRING)&ComputerName,
                                  lpUserSid,
                                  (PRPC_UNICODE_STRING*)Strings,
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

    for (i = 0; i < wNumStrings; i++)
    {
        if (Strings[i] != NULL)
            HeapFree(GetProcessHeap(), 0, Strings[i]);
    }

    HeapFree(GetProcessHeap(), 0, Strings);

    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

BOOL WINAPI
ElfReportEventW(DWORD param1,
                DWORD param2,
                DWORD param3,
                DWORD param4,
                DWORD param5,
                DWORD param6,
                DWORD param7,
                DWORD param8,
                DWORD param9,
                DWORD param10,
                DWORD param11,
                DWORD param12)
{
    return TRUE;
}

HANDLE WINAPI
ElfRegisterEventSourceW(DWORD param1,
                        DWORD param2,
                        DWORD param3)
{
    return (HANDLE)1;
}

BOOL WINAPI
ElfDeregisterEventSource(IN HANDLE hEventLog)
{
    return TRUE;
}
