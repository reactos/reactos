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

/* INCLUDES ******************************************************************/

#include <advapi32.h>

#include <ndk/kefuncs.h>
#include <eventlogrpc_c.h>

WINE_DEFAULT_DEBUG_CHANNEL(advapi);

static RPC_UNICODE_STRING EmptyStringU = { 0, 0, L"" };
static RPC_STRING EmptyStringA = { 0, 0, "" };


/* FUNCTIONS *****************************************************************/

handle_t __RPC_USER
EVENTLOG_HANDLE_A_bind(EVENTLOG_HANDLE_A UNCServerName)
{
    handle_t hBinding = NULL;
    RPC_CSTR pszStringBinding;
    RPC_STATUS status;

    TRACE("EVENTLOG_HANDLE_A_bind() called\n");

    status = RpcStringBindingComposeA(NULL,
                                      (RPC_CSTR)"ncacn_np",
                                      (RPC_CSTR)UNCServerName,
                                      (RPC_CSTR)"\\pipe\\EventLog",
                                      NULL,
                                      &pszStringBinding);
    if (status)
    {
        ERR("RpcStringBindingCompose returned 0x%x\n", status);
        return NULL;
    }

    /* Set the binding handle that will be used to bind to the server. */
    status = RpcBindingFromStringBindingA(pszStringBinding,
                                          &hBinding);
    if (status != RPC_S_OK)
    {
        ERR("RpcBindingFromStringBinding returned 0x%x\n", status);
    }

    status = RpcStringFreeA(&pszStringBinding);
    if (status != RPC_S_OK)
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
    if (status != RPC_S_OK)
    {
        ERR("RpcBindingFree returned 0x%x\n", status);
    }
}


handle_t __RPC_USER
EVENTLOG_HANDLE_W_bind(EVENTLOG_HANDLE_W UNCServerName)
{
    handle_t hBinding = NULL;
    RPC_WSTR pszStringBinding;
    RPC_STATUS status;

    TRACE("EVENTLOG_HANDLE_W_bind() called\n");

    status = RpcStringBindingComposeW(NULL,
                                      L"ncacn_np",
                                      UNCServerName,
                                      L"\\pipe\\EventLog",
                                      NULL,
                                      &pszStringBinding);
    if (status != RPC_S_OK)
    {
        ERR("RpcStringBindingCompose returned 0x%x\n", status);
        return NULL;
    }

    /* Set the binding handle that will be used to bind to the server. */
    status = RpcBindingFromStringBindingW(pszStringBinding,
                                          &hBinding);
    if (status != RPC_S_OK)
    {
        ERR("RpcBindingFromStringBinding returned 0x%x\n", status);
    }

    status = RpcStringFreeW(&pszStringBinding);
    if (status != RPC_S_OK)
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
    if (status != RPC_S_OK)
    {
        ERR("RpcBindingFree returned 0x%x\n", status);
    }
}


/******************************************************************************
 * BackupEventLogA [ADVAPI32.@]
 */
NTSTATUS
NTAPI
ElfBackupEventLogFileA(IN HANDLE hEventLog,
                       IN PANSI_STRING BackupFileNameA)
{
    NTSTATUS Status;

    if (!BackupFileNameA || (BackupFileNameA->Length == 0))
        return STATUS_INVALID_PARAMETER;

    RpcTryExcept
    {
        Status = ElfrBackupELFA(hEventLog,
                                (PRPC_STRING)BackupFileNameA);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}

BOOL WINAPI
BackupEventLogA(IN HANDLE hEventLog,
                IN LPCSTR lpBackupFileName)
{
    BOOL Success;
    NTSTATUS Status;
    ANSI_STRING BackupFileNameA;
    UNICODE_STRING BackupFileNameW;

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

    Success = BackupEventLogW(hEventLog,
                              BackupFileNameW.Buffer);

    RtlFreeUnicodeString(&BackupFileNameW);

    return Success;
}


/******************************************************************************
 * BackupEventLogW [ADVAPI32.@]
 *
 * PARAMS
 *   hEventLog        []
 *   lpBackupFileName []
 */
NTSTATUS
NTAPI
ElfBackupEventLogFileW(IN HANDLE hEventLog,
                       IN PUNICODE_STRING BackupFileNameU)
{
    NTSTATUS Status;

    if (!BackupFileNameU || (BackupFileNameU->Length == 0))
        return STATUS_INVALID_PARAMETER;

    RpcTryExcept
    {
        Status = ElfrBackupELFW(hEventLog,
                                (PRPC_UNICODE_STRING)BackupFileNameU);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}

BOOL WINAPI
BackupEventLogW(IN HANDLE hEventLog,
                IN LPCWSTR lpBackupFileName)
{
    NTSTATUS Status;
    UNICODE_STRING BackupFileName;

    TRACE("%p, %s\n", hEventLog, debugstr_w(lpBackupFileName));

    if (lpBackupFileName == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!RtlDosPathNameToNtPathName_U(lpBackupFileName, &BackupFileName,
                                      NULL, NULL))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    Status = ElfBackupEventLogFileW(hEventLog, &BackupFileName);

    RtlFreeHeap(RtlGetProcessHeap(), 0, BackupFileName.Buffer);

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
NTSTATUS
NTAPI
ElfClearEventLogFileA(IN HANDLE hEventLog,
                      IN PANSI_STRING BackupFileNameA)
{
    NTSTATUS Status;

    RpcTryExcept
    {
        Status = ElfrClearELFA(hEventLog,
                               (PRPC_STRING)BackupFileNameA);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}

BOOL WINAPI
ClearEventLogA(IN HANDLE hEventLog,
               IN LPCSTR lpBackupFileName)
{
    BOOL Success;
    NTSTATUS Status;
    ANSI_STRING BackupFileNameA;
    UNICODE_STRING BackupFileNameW;

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

    Success = ClearEventLogW(hEventLog,
                             BackupFileNameW.Buffer);

    RtlFreeUnicodeString(&BackupFileNameW);

    return Success;
}


/******************************************************************************
 * ClearEventLogW [ADVAPI32.@]
 */
NTSTATUS
NTAPI
ElfClearEventLogFileW(IN HANDLE hEventLog,
                      IN PUNICODE_STRING BackupFileNameU)
{
    NTSTATUS Status;

    RpcTryExcept
    {
        Status = ElfrClearELFW(hEventLog,
                               (PRPC_UNICODE_STRING)BackupFileNameU);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}

BOOL WINAPI
ClearEventLogW(IN HANDLE hEventLog,
               IN LPCWSTR lpBackupFileName)
{
    NTSTATUS Status;
    UNICODE_STRING BackupFileName;

    TRACE("%p, %s\n", hEventLog, debugstr_w(lpBackupFileName));

    if (lpBackupFileName == NULL)
    {
        RtlInitUnicodeString(&BackupFileName, NULL);
    }
    else
    {
        if (!RtlDosPathNameToNtPathName_U(lpBackupFileName, &BackupFileName,
                                          NULL, NULL))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
    }

    Status = ElfClearEventLogFileW(hEventLog, &BackupFileName);

    if (lpBackupFileName != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, BackupFileName.Buffer);

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
NTSTATUS
NTAPI
ElfCloseEventLog(IN HANDLE hEventLog)
{
    NTSTATUS Status;

    RpcTryExcept
    {
        Status = ElfrCloseEL(&hEventLog);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}

BOOL WINAPI
CloseEventLog(IN HANDLE hEventLog)
{
    NTSTATUS Status;

    TRACE("%p\n", hEventLog);

    Status = ElfCloseEventLog(hEventLog);
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
NTSTATUS
NTAPI
ElfDeregisterEventSource(IN HANDLE hEventLog)
{
    NTSTATUS Status;

    RpcTryExcept
    {
        Status = ElfrDeregisterEventSource(&hEventLog);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}

BOOL WINAPI
DeregisterEventSource(IN HANDLE hEventLog)
{
    NTSTATUS Status;

    TRACE("%p\n", hEventLog);

    Status = ElfDeregisterEventSource(hEventLog);
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
NTSTATUS
NTAPI
ElfNumberOfRecords(IN HANDLE hEventLog,
                   OUT PULONG NumberOfRecords)
{
    NTSTATUS Status;

    if (!NumberOfRecords)
        return STATUS_INVALID_PARAMETER;

    RpcTryExcept
    {
        Status = ElfrNumberOfRecords(hEventLog, NumberOfRecords);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}

BOOL WINAPI
GetNumberOfEventLogRecords(IN HANDLE hEventLog,
                           OUT PDWORD NumberOfRecords)
{
    NTSTATUS Status;

    TRACE("%p, %p\n", hEventLog, NumberOfRecords);

    Status = ElfNumberOfRecords(hEventLog, NumberOfRecords);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}


/******************************************************************************
 * GetOldestEventLogRecord [ADVAPI32.@]
 *
 * PARAMS
 *   hEventLog    []
 *   OldestRecord []
 */
NTSTATUS
NTAPI
ElfOldestRecord(IN HANDLE hEventLog,
                OUT PULONG OldestRecordNumber)
{
    NTSTATUS Status;

    if (!OldestRecordNumber)
        return STATUS_INVALID_PARAMETER;

    RpcTryExcept
    {
        Status = ElfrOldestRecord(hEventLog, OldestRecordNumber);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}

BOOL WINAPI
GetOldestEventLogRecord(IN HANDLE hEventLog,
                        OUT PDWORD OldestRecord)
{
    NTSTATUS Status;

    TRACE("%p, %p\n", hEventLog, OldestRecord);

    Status = ElfOldestRecord(hEventLog, OldestRecord);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}


/******************************************************************************
 * NotifyChangeEventLog [ADVAPI32.@]
 *
 * PARAMS
 *   hEventLog []
 *   hEvent    []
 */
NTSTATUS
NTAPI
ElfChangeNotify(IN HANDLE hEventLog,
                IN HANDLE hEvent)
{
    NTSTATUS Status;
    CLIENT_ID ClientId = NtCurrentTeb()->ClientId;
    RPC_CLIENT_ID RpcClientId;

    RpcClientId.UniqueProcess = HandleToUlong(ClientId.UniqueProcess);
    RpcClientId.UniqueThread  = HandleToUlong(ClientId.UniqueThread);

    RpcTryExcept
    {
        Status = ElfrChangeNotify(hEventLog, RpcClientId, HandleToUlong(hEvent));
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}

BOOL WINAPI
NotifyChangeEventLog(IN HANDLE hEventLog,
                     IN HANDLE hEvent)
{
    NTSTATUS Status;

    TRACE("%p, %p\n", hEventLog, hEvent);

    Status = ElfChangeNotify(hEventLog, hEvent);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}


/******************************************************************************
 * OpenBackupEventLogA [ADVAPI32.@]
 */
NTSTATUS
NTAPI
ElfOpenBackupEventLogA(IN PANSI_STRING UNCServerNameA,
                       IN PANSI_STRING BackupFileNameA,
                       OUT PHANDLE phEventLog)
{
    NTSTATUS Status;
    PSTR pUNCServerName = NULL;

    if (!phEventLog || !BackupFileNameA || (BackupFileNameA->Length == 0))
        return STATUS_INVALID_PARAMETER;

    if (UNCServerNameA && (UNCServerNameA->Length != 0))
        pUNCServerName = UNCServerNameA->Buffer;

    *phEventLog = NULL;

    RpcTryExcept
    {
        Status = ElfrOpenBELA(pUNCServerName,
                              (PRPC_STRING)BackupFileNameA,
                              1, 1,
                              (IELF_HANDLE*)phEventLog);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}

HANDLE WINAPI
OpenBackupEventLogA(IN LPCSTR lpUNCServerName,
                    IN LPCSTR lpFileName)
{
    NTSTATUS Status;
    HANDLE LogHandle;
    ANSI_STRING UNCServerNameA;
    UNICODE_STRING UNCServerNameW;
    ANSI_STRING FileNameA;
    UNICODE_STRING FileNameW;

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
NTSTATUS
NTAPI
ElfOpenBackupEventLogW(IN PUNICODE_STRING UNCServerNameU,
                       IN PUNICODE_STRING BackupFileNameU,
                       OUT PHANDLE phEventLog)
{
    NTSTATUS Status;
    PWSTR pUNCServerName = NULL;

    if (!phEventLog || !BackupFileNameU || (BackupFileNameU->Length == 0))
        return STATUS_INVALID_PARAMETER;

    if (UNCServerNameU && (UNCServerNameU->Length != 0))
        pUNCServerName = UNCServerNameU->Buffer;

    *phEventLog = NULL;

    RpcTryExcept
    {
        Status = ElfrOpenBELW(pUNCServerName,
                              (PRPC_UNICODE_STRING)BackupFileNameU,
                              1,
                              1,
                              (IELF_HANDLE*)phEventLog);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}

HANDLE WINAPI
OpenBackupEventLogW(IN LPCWSTR lpUNCServerName,
                    IN LPCWSTR lpFileName)
{
    NTSTATUS Status;
    HANDLE hEventLog;
    UNICODE_STRING UNCServerName, FileName;

    TRACE("%s, %s\n", debugstr_w(lpUNCServerName), debugstr_w(lpFileName));

    if (lpFileName == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    if (!RtlDosPathNameToNtPathName_U(lpFileName, &FileName,
                                      NULL, NULL))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    RtlInitUnicodeString(&UNCServerName, lpUNCServerName);

    Status = ElfOpenBackupEventLogW(&UNCServerName, &FileName, &hEventLog);

    if (FileName.Buffer != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, FileName.Buffer);

    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return NULL;
    }

    return hEventLog;
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
NTSTATUS
NTAPI
ElfOpenEventLogA(IN PANSI_STRING UNCServerNameA,
                 IN PANSI_STRING SourceNameA,
                 OUT PHANDLE phEventLog)
{
    NTSTATUS Status;
    PSTR pUNCServerName = NULL;

    if (!phEventLog || !SourceNameA || (SourceNameA->Length == 0))
        return STATUS_INVALID_PARAMETER;

    if (UNCServerNameA && (UNCServerNameA->Length != 0))
        pUNCServerName = UNCServerNameA->Buffer;

    *phEventLog = NULL;

    RpcTryExcept
    {
        Status = ElfrOpenELA(pUNCServerName,
                             (PRPC_STRING)SourceNameA,
                             &EmptyStringA,
                             1,
                             1,
                             (IELF_HANDLE*)phEventLog);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}

HANDLE WINAPI
OpenEventLogA(IN LPCSTR lpUNCServerName,
              IN LPCSTR lpSourceName)
{
    NTSTATUS Status;
    HANDLE hEventLog;
    ANSI_STRING UNCServerName, SourceName;

    TRACE("%s, %s\n", lpUNCServerName, lpSourceName);

    RtlInitAnsiString(&UNCServerName, lpUNCServerName);
    RtlInitAnsiString(&SourceName, lpSourceName);

    Status = ElfOpenEventLogA(&UNCServerName, &SourceName, &hEventLog);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return NULL;
    }

    return hEventLog;
}


/******************************************************************************
 * OpenEventLogW [ADVAPI32.@]
 *
 * PARAMS
 *   lpUNCServerName []
 *   lpSourceName    []
 */
NTSTATUS
NTAPI
ElfOpenEventLogW(IN PUNICODE_STRING UNCServerNameU,
                 IN PUNICODE_STRING SourceNameU,
                 OUT PHANDLE phEventLog)
{
    NTSTATUS Status;
    PWSTR pUNCServerName = NULL;

    if (!phEventLog || !SourceNameU || (SourceNameU->Length == 0))
        return STATUS_INVALID_PARAMETER;

    if (UNCServerNameU && (UNCServerNameU->Length != 0))
        pUNCServerName = UNCServerNameU->Buffer;

    *phEventLog = NULL;

    RpcTryExcept
    {
        Status = ElfrOpenELW(pUNCServerName,
                             (PRPC_UNICODE_STRING)SourceNameU,
                             &EmptyStringU,
                             1,
                             1,
                             (IELF_HANDLE*)phEventLog);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}

HANDLE WINAPI
OpenEventLogW(IN LPCWSTR lpUNCServerName,
              IN LPCWSTR lpSourceName)
{
    NTSTATUS Status;
    HANDLE hEventLog;
    UNICODE_STRING UNCServerName, SourceName;

    TRACE("%s, %s\n", debugstr_w(lpUNCServerName), debugstr_w(lpSourceName));

    RtlInitUnicodeString(&UNCServerName, lpUNCServerName);
    RtlInitUnicodeString(&SourceName, lpSourceName);

    Status = ElfOpenEventLogW(&UNCServerName, &SourceName, &hEventLog);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return NULL;
    }

    return hEventLog;
}


/******************************************************************************
 * ReadEventLogA [ADVAPI32.@]
 */
NTSTATUS
NTAPI
ElfReadEventLogA(IN HANDLE hEventLog,
                 IN ULONG ReadFlags,
                 IN ULONG RecordOffset,
                 OUT LPVOID Buffer,
                 IN ULONG NumberOfBytesToRead,
                 OUT PULONG NumberOfBytesRead,
                 OUT PULONG MinNumberOfBytesNeeded)
{
    NTSTATUS Status;
    ULONG Flags;

    if (!Buffer || !NumberOfBytesRead || !MinNumberOfBytesNeeded)
    {
        return STATUS_INVALID_PARAMETER;
    }

    Flags = ReadFlags & (EVENTLOG_SEQUENTIAL_READ | EVENTLOG_SEEK_READ);
    if (Flags == (EVENTLOG_SEQUENTIAL_READ | EVENTLOG_SEEK_READ))
    {
        return STATUS_INVALID_PARAMETER;
    }

    Flags = ReadFlags & (EVENTLOG_FORWARDS_READ | EVENTLOG_BACKWARDS_READ);
    if (Flags == (EVENTLOG_FORWARDS_READ | EVENTLOG_BACKWARDS_READ))
    {
        return STATUS_INVALID_PARAMETER;
    }

    RpcTryExcept
    {
        Status = ElfrReadELA(hEventLog,
                             ReadFlags,
                             RecordOffset,
                             NumberOfBytesToRead,
                             Buffer,
                             NumberOfBytesRead,
                             MinNumberOfBytesNeeded);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}

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

    TRACE("%p, %lu, %lu, %p, %lu, %p, %p\n",
        hEventLog, dwReadFlags, dwRecordOffset, lpBuffer,
        nNumberOfBytesToRead, pnBytesRead, pnMinNumberOfBytesNeeded);

    Status = ElfReadEventLogA(hEventLog,
                              dwReadFlags,
                              dwRecordOffset,
                              lpBuffer,
                              nNumberOfBytesToRead,
                              pnBytesRead,
                              pnMinNumberOfBytesNeeded);
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
NTSTATUS
NTAPI
ElfReadEventLogW(IN HANDLE hEventLog,
                 IN ULONG ReadFlags,
                 IN ULONG RecordOffset,
                 OUT LPVOID Buffer,
                 IN ULONG NumberOfBytesToRead,
                 OUT PULONG NumberOfBytesRead,
                 OUT PULONG MinNumberOfBytesNeeded)
{
    NTSTATUS Status;
    ULONG Flags;

    if (!Buffer || !NumberOfBytesRead || !MinNumberOfBytesNeeded)
    {
        return STATUS_INVALID_PARAMETER;
    }

    Flags = ReadFlags & (EVENTLOG_SEQUENTIAL_READ | EVENTLOG_SEEK_READ);
    if (Flags == (EVENTLOG_SEQUENTIAL_READ | EVENTLOG_SEEK_READ))
    {
        return STATUS_INVALID_PARAMETER;
    }

    Flags = ReadFlags & (EVENTLOG_FORWARDS_READ | EVENTLOG_BACKWARDS_READ);
    if (Flags == (EVENTLOG_FORWARDS_READ | EVENTLOG_BACKWARDS_READ))
    {
        return STATUS_INVALID_PARAMETER;
    }

    RpcTryExcept
    {
        Status = ElfrReadELW(hEventLog,
                             ReadFlags,
                             RecordOffset,
                             NumberOfBytesToRead,
                             Buffer,
                             NumberOfBytesRead,
                             MinNumberOfBytesNeeded);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}

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

    TRACE("%p, %lu, %lu, %p, %lu, %p, %p\n",
        hEventLog, dwReadFlags, dwRecordOffset, lpBuffer,
        nNumberOfBytesToRead, pnBytesRead, pnMinNumberOfBytesNeeded);

    Status = ElfReadEventLogW(hEventLog,
                              dwReadFlags,
                              dwRecordOffset,
                              lpBuffer,
                              nNumberOfBytesToRead,
                              pnBytesRead,
                              pnMinNumberOfBytesNeeded);
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
NTSTATUS
NTAPI
ElfRegisterEventSourceA(IN PANSI_STRING UNCServerNameA,
                        IN PANSI_STRING SourceNameA,
                        OUT PHANDLE phEventLog)
{
    NTSTATUS Status;
    PSTR pUNCServerName = NULL;

    if (!phEventLog || !SourceNameA || (SourceNameA->Length == 0))
        return STATUS_INVALID_PARAMETER;

    if (UNCServerNameA && (UNCServerNameA->Length != 0))
        pUNCServerName = UNCServerNameA->Buffer;

    *phEventLog = NULL;

    RpcTryExcept
    {
        Status = ElfrRegisterEventSourceA(pUNCServerName,
                                          (PRPC_STRING)SourceNameA,
                                          &EmptyStringA,
                                          1,
                                          1,
                                          (IELF_HANDLE*)phEventLog);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}

HANDLE WINAPI
RegisterEventSourceA(IN LPCSTR lpUNCServerName,
                     IN LPCSTR lpSourceName)
{
    NTSTATUS Status;
    HANDLE hEventLog;
    ANSI_STRING UNCServerName, SourceName;

    TRACE("%s, %s\n", lpUNCServerName, lpSourceName);

    RtlInitAnsiString(&UNCServerName, lpUNCServerName);
    RtlInitAnsiString(&SourceName, lpSourceName);

    Status = ElfRegisterEventSourceA(&UNCServerName, &SourceName, &hEventLog);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return NULL;
    }

    return hEventLog;
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
NTSTATUS
NTAPI
ElfRegisterEventSourceW(IN PUNICODE_STRING UNCServerNameU,
                        IN PUNICODE_STRING SourceNameU,
                        OUT PHANDLE phEventLog)
{
    NTSTATUS Status;
    PWSTR pUNCServerName = NULL;

    if (!phEventLog || !SourceNameU || (SourceNameU->Length == 0))
        return STATUS_INVALID_PARAMETER;

    if (UNCServerNameU && (UNCServerNameU->Length != 0))
        pUNCServerName = UNCServerNameU->Buffer;

    *phEventLog = NULL;

    RpcTryExcept
    {
        Status = ElfrRegisterEventSourceW(pUNCServerName,
                                          (PRPC_UNICODE_STRING)SourceNameU,
                                          &EmptyStringU,
                                          1,
                                          1,
                                          (IELF_HANDLE*)phEventLog);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}

HANDLE WINAPI
RegisterEventSourceW(IN LPCWSTR lpUNCServerName,
                     IN LPCWSTR lpSourceName)
{
    NTSTATUS Status;
    HANDLE hEventLog;
    UNICODE_STRING UNCServerName, SourceName;

    TRACE("%s, %s\n", debugstr_w(lpUNCServerName), debugstr_w(lpSourceName));

    RtlInitUnicodeString(&UNCServerName, lpUNCServerName);
    RtlInitUnicodeString(&SourceName, lpSourceName);

    Status = ElfRegisterEventSourceW(&UNCServerName, &SourceName, &hEventLog);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return NULL;
    }

    return hEventLog;
}


/******************************************************************************
 * ReportEventA [ADVAPI32.@]
 */
NTSTATUS
NTAPI
ElfReportEventA(IN HANDLE hEventLog,
                IN USHORT EventType,
                IN USHORT EventCategory,
                IN ULONG EventID,
                IN PSID UserSID,
                IN USHORT NumStrings,
                IN ULONG DataSize,
                IN PANSI_STRING* Strings,
                IN PVOID Data,
                IN USHORT Flags,
                IN OUT PULONG RecordNumber,
                IN OUT PULONG TimeWritten)
{
    NTSTATUS Status;
    LARGE_INTEGER SystemTime;
    ULONG Time;
    ULONG dwSize;
    ANSI_STRING ComputerName;
    CHAR szComputerName[MAX_COMPUTERNAME_LENGTH + 1];

    dwSize = ARRAYSIZE(szComputerName);
    GetComputerNameA(szComputerName, &dwSize);
    RtlInitAnsiString(&ComputerName, szComputerName);

    NtQuerySystemTime(&SystemTime);
    RtlTimeToSecondsSince1970(&SystemTime, &Time);

    RpcTryExcept
    {
        Status = ElfrReportEventA(hEventLog,
                                  Time,
                                  EventType,
                                  EventCategory,
                                  EventID,
                                  NumStrings,
                                  DataSize,
                                  (PRPC_STRING)&ComputerName,
                                  (PRPC_SID)UserSID,
                                  (PRPC_STRING*)Strings,
                                  Data,
                                  Flags,
                                  RecordNumber,
                                  TimeWritten);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}

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
    WORD i;

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

    Status = ElfReportEventA(hEventLog,
                             wType,
                             wCategory,
                             dwEventID,
                             lpUserSid,
                             wNumStrings,
                             dwDataSize,
                             Strings,
                             lpRawData,
                             0,
                             NULL,
                             NULL);

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
NTSTATUS
NTAPI
ElfReportEventW(IN HANDLE hEventLog,
                IN USHORT EventType,
                IN USHORT EventCategory,
                IN ULONG EventID,
                IN PSID UserSID,
                IN USHORT NumStrings,
                IN ULONG DataSize,
                IN PUNICODE_STRING* Strings,
                IN PVOID Data,
                IN USHORT Flags,
                IN OUT PULONG RecordNumber,
                IN OUT PULONG TimeWritten)
{
    NTSTATUS Status;
    LARGE_INTEGER SystemTime;
    ULONG Time;
    ULONG dwSize;
    UNICODE_STRING ComputerName;
    WCHAR szComputerName[MAX_COMPUTERNAME_LENGTH + 1];

    dwSize = ARRAYSIZE(szComputerName);
    GetComputerNameW(szComputerName, &dwSize);
    RtlInitUnicodeString(&ComputerName, szComputerName);

    NtQuerySystemTime(&SystemTime);
    RtlTimeToSecondsSince1970(&SystemTime, &Time);

    RpcTryExcept
    {
        Status = ElfrReportEventW(hEventLog,
                                  Time,
                                  EventType,
                                  EventCategory,
                                  EventID,
                                  NumStrings,
                                  DataSize,
                                  (PRPC_UNICODE_STRING)&ComputerName,
                                  (PRPC_SID)UserSID,
                                  (PRPC_UNICODE_STRING*)Strings,
                                  Data,
                                  Flags,
                                  RecordNumber,
                                  TimeWritten);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}

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
    WORD i;

    TRACE("%p, %u, %u, %lu, %p, %u, %lu, %p, %p\n",
          hEventLog, wType, wCategory, dwEventID, lpUserSid,
          wNumStrings, dwDataSize, lpStrings, lpRawData);

    Strings = HeapAlloc(GetProcessHeap(),
                        HEAP_ZERO_MEMORY,
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
                               sizeof(UNICODE_STRING));
        if (Strings[i])
        {
            RtlInitUnicodeString(Strings[i], lpStrings[i]);
        }
    }

    Status = ElfReportEventW(hEventLog,
                             wType,
                             wCategory,
                             dwEventID,
                             lpUserSid,
                             wNumStrings,
                             dwDataSize,
                             Strings,
                             lpRawData,
                             0,
                             NULL,
                             NULL);

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

NTSTATUS
NTAPI
ElfReportEventAndSourceW(IN HANDLE hEventLog,
                         IN ULONG Time,
                         IN PUNICODE_STRING ComputerName,
                         IN USHORT EventType,
                         IN USHORT EventCategory,
                         IN ULONG EventID,
                         IN PSID UserSID,
                         IN PUNICODE_STRING SourceName,
                         IN USHORT NumStrings,
                         IN ULONG DataSize,
                         IN PUNICODE_STRING* Strings,
                         IN PVOID Data,
                         IN USHORT Flags,
                         IN OUT PULONG RecordNumber,
                         IN OUT PULONG TimeWritten)
{
    NTSTATUS Status;

    RpcTryExcept
    {
        Status = ElfrReportEventAndSourceW(hEventLog,
                                           Time,
                                           EventType,
                                           EventCategory,
                                           EventID,
                                           (PRPC_UNICODE_STRING)SourceName,
                                           NumStrings,
                                           DataSize,
                                           (PRPC_UNICODE_STRING)ComputerName,
                                           (PRPC_SID)UserSID,
                                           (PRPC_UNICODE_STRING*)Strings,
                                           (PBYTE)Data,
                                           Flags,
                                           RecordNumber,
                                           TimeWritten);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}

NTSTATUS
NTAPI
ElfFlushEventLog(IN HANDLE hEventLog)
{
    NTSTATUS Status;

    RpcTryExcept
    {
        Status = ElfrFlushEL(hEventLog);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}
