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
#define NDEBUG
#include <debug.h>


typedef struct _LOG_INFO
{
    RPC_BINDING_HANDLE BindingHandle;
    LOGHANDLE LogHandle;
    BOOL bLocal;
} LOG_INFO, *PLOG_INFO;

/******************************************************************************
 * BackupEventLogA [ADVAPI32.@]
 */
BOOL WINAPI
BackupEventLogA(
    IN HANDLE hEventLog,
    IN LPCSTR lpBackupFileName)
{
    PLOG_INFO pLog;
    NTSTATUS Status;
    ANSI_STRING BackupFileName;

    DPRINT("%p, %s\n", hEventLog, lpBackupFileName);

    RtlInitAnsiString(&BackupFileName, lpBackupFileName);

    pLog = (PLOG_INFO)hEventLog;
    if (!pLog)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    Status = ElfrBackupELFA(
        pLog->BindingHandle,
        pLog->LogHandle,
        BackupFileName.Buffer);

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
BackupEventLogW(
    IN HANDLE hEventLog,
    IN LPCWSTR lpBackupFileName)
{
    PLOG_INFO pLog;
    NTSTATUS Status;
    UNICODE_STRING BackupFileName;

    DPRINT("%p, %s\n", hEventLog, lpBackupFileName);

    RtlInitUnicodeString(&BackupFileName, lpBackupFileName);

    pLog = (PLOG_INFO)hEventLog;
    if (!pLog)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    Status = ElfrBackupELFW(
        pLog->BindingHandle,
        pLog->LogHandle,
        BackupFileName.Buffer);

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
ClearEventLogA(
    IN HANDLE hEventLog,
    IN LPCSTR lpBackupFileName)
{
    PLOG_INFO pLog;
    NTSTATUS Status;
    ANSI_STRING BackupFileName;

    DPRINT("%p, %s\n", hEventLog, lpBackupFileName);

    RtlInitAnsiString(&BackupFileName, lpBackupFileName);

    pLog = (PLOG_INFO)hEventLog;
    if (!pLog)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    Status = ElfrClearELFA(
        pLog->BindingHandle,
        pLog->LogHandle,
        BackupFileName.Buffer);

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
ClearEventLogW(
    IN HANDLE hEventLog,
    IN LPCWSTR lpBackupFileName)
{
    PLOG_INFO pLog;
    NTSTATUS Status;
    UNICODE_STRING BackupFileName;

    DPRINT("%p, %s\n", hEventLog, lpBackupFileName);

    RtlInitUnicodeString(&BackupFileName, lpBackupFileName);

    pLog = (PLOG_INFO)hEventLog;
    if (!pLog)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    Status = ElfrClearELFW(
        pLog->BindingHandle,
        pLog->LogHandle,
        BackupFileName.Buffer);

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
CloseEventLog(
    IN HANDLE hEventLog)
{
    PLOG_INFO pLog;
    NTSTATUS Status;

    DPRINT("%p", hEventLog);

    pLog = (PLOG_INFO)hEventLog;
    if (!pLog)
        return TRUE;

    if (pLog->bLocal == FALSE)
    {
        if (!EvtUnbindRpc(pLog->BindingHandle))
        {
            SetLastError(ERROR_ACCESS_DENIED);
            return FALSE;
        }
    }
    else
    {
        Status = ElfrCloseEL(
            pLog->BindingHandle,
            &pLog->LogHandle);
        if (!NT_SUCCESS(Status))
        {
            SetLastError(RtlNtStatusToDosError(Status));
            return FALSE;
        }
    }

    HeapFree(GetProcessHeap(), 0, pLog);

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
DeregisterEventSource(
    IN HANDLE hEventLog)
{
    PLOG_INFO pLog;
    NTSTATUS Status;

    DPRINT("%p\n", hEventLog);

    pLog = (PLOG_INFO)hEventLog;
    if (!pLog)
        return TRUE;

    Status = ElfrDeregisterEventSource(
        pLog->BindingHandle,
        &pLog->LogHandle);
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
GetNumberOfEventLogRecords(
    IN HANDLE hEventLog,
    OUT PDWORD NumberOfRecords)
{
    PLOG_INFO pLog;
    NTSTATUS Status;
    long Records;

    DPRINT("%p, %p\n", hEventLog, NumberOfRecords);

    pLog = (PLOG_INFO)hEventLog;
    if (!pLog)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    Status = ElfrNumberOfRecords(
        pLog->BindingHandle,
        pLog->LogHandle,
        &Records);
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
GetOldestEventLogRecord(
    IN HANDLE hEventLog,
    OUT PDWORD OldestRecord)
{
    PLOG_INFO pLog;
    NTSTATUS Status;
    long Oldest;

    DPRINT("%p, %p\n", hEventLog, OldestRecord);

    pLog = (PLOG_INFO)hEventLog;
    if (!pLog)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    Status = ElfrOldestRecord(
        pLog->BindingHandle,
        pLog->LogHandle,
        &Oldest);
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
NotifyChangeEventLog(
    IN HANDLE hEventLog,
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
OpenBackupEventLogA(
    IN LPCSTR lpUNCServerName,
    IN LPCSTR lpFileName)
{
    UNICODE_STRING UNCServerName;
    UNICODE_STRING FileName;
    HANDLE Handle;

    DPRINT("%s, %s\n", lpUNCServerName, lpFileName);

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

    Handle = OpenBackupEventLogW(
        UNCServerName.Buffer,
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
OpenBackupEventLogW(
    IN LPCWSTR lpUNCServerName,
    IN LPCWSTR lpFileName)
{
    PLOG_INFO pLog;
    NTSTATUS Status;
    UNICODE_STRING UNCServerName;
    UNICODE_STRING FileName;

    DPRINT("%s, %s\n", lpUNCServerName, lpFileName);

    RtlInitUnicodeString(&UNCServerName, lpUNCServerName);
    RtlInitUnicodeString(&FileName, lpFileName);

    pLog = HeapAlloc(GetProcessHeap(), 0, sizeof(LOG_INFO));
    if (!pLog)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }
    ZeroMemory(pLog, sizeof(LOG_INFO));

    if (lpUNCServerName == NULL || *lpUNCServerName == 0)
    {
        pLog->bLocal = TRUE;

        if (!EvtGetLocalHandle(&pLog->BindingHandle))
        {
            HeapFree(GetProcessHeap(), 0, pLog);
            SetLastError(ERROR_GEN_FAILURE);
            return NULL;
        }
    }
    else
    {
        pLog->bLocal = FALSE;

        if (!EvtBindRpc(lpUNCServerName, &pLog->BindingHandle))
        {
            HeapFree(GetProcessHeap(), 0, pLog);
            SetLastError(ERROR_INVALID_COMPUTERNAME);
            return NULL;
        }
    }

    Status = ElfrOpenBELW(
        pLog->BindingHandle,
        UNCServerName.Buffer,
        FileName.Buffer,
        0,
        0,
        &pLog->LogHandle);

    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        HeapFree(GetProcessHeap(), 0, pLog);
        return NULL;
    }
    return pLog;
}


/******************************************************************************
 * OpenEventLogA [ADVAPI32.@]
 */
HANDLE WINAPI
OpenEventLogA(
    IN LPCSTR lpUNCServerName,
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

    Handle = OpenEventLogW(UNCServerName.Buffer, SourceName.Buffer);

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
OpenEventLogW(
    IN LPCWSTR lpUNCServerName,
    IN LPCWSTR lpSourceName)
{
    PLOG_INFO pLog;
    NTSTATUS Status;
    UNICODE_STRING UNCServerName;
    UNICODE_STRING SourceName;

    DPRINT("%s, %s\n", lpUNCServerName, lpSourceName);

    RtlInitUnicodeString(&UNCServerName, lpUNCServerName);
    RtlInitUnicodeString(&SourceName, lpSourceName);

    pLog = HeapAlloc(GetProcessHeap(), 0, sizeof(LOG_INFO));
    if (!pLog)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }
    ZeroMemory(pLog, sizeof(LOG_INFO));

    if (lpUNCServerName == NULL || *lpUNCServerName == 0)
    {
        pLog->bLocal = TRUE;

        if (!EvtGetLocalHandle(&pLog->BindingHandle))
        {
            HeapFree(GetProcessHeap(), 0, pLog);
            SetLastError(ERROR_GEN_FAILURE);
            return NULL;
        }
    }
    else
    {
        pLog->bLocal = FALSE;

        if (!EvtBindRpc(lpUNCServerName, &pLog->BindingHandle))
        {
            HeapFree(GetProcessHeap(), 0, pLog);
            SetLastError(ERROR_INVALID_COMPUTERNAME);
            return NULL;
        }
    }

    Status = ElfrOpenELW(
        pLog->BindingHandle,
        UNCServerName.Buffer,
        SourceName.Buffer,
        NULL,
        0,
        0,
        &pLog->LogHandle);

    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        HeapFree(GetProcessHeap(), 0, pLog);
        return NULL;
    }
    return pLog;
}


/******************************************************************************
 * ReadEventLogA [ADVAPI32.@]
 */
BOOL WINAPI
ReadEventLogA(
    IN HANDLE hEventLog,
    IN DWORD dwReadFlags,
    IN DWORD dwRecordOffset,
    OUT LPVOID lpBuffer,
    IN DWORD nNumberOfBytesToRead,
    OUT DWORD *pnBytesRead,
    OUT DWORD *pnMinNumberOfBytesNeeded)
{
    PLOG_INFO pLog;
    NTSTATUS Status;
    long bytesRead, minNumberOfBytesNeeded;

    DPRINT("%p, %lu, %lu, %p, %lu, %p, %p\n",
        hEventLog, dwReadFlags, dwRecordOffset, lpBuffer,
        nNumberOfBytesToRead, pnBytesRead, pnMinNumberOfBytesNeeded);

    pLog = (PLOG_INFO)hEventLog;
    if (!pLog)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    Status = ElfrReadELA(
        pLog->BindingHandle,
        pLog->LogHandle,
        dwReadFlags,
        dwRecordOffset,
        nNumberOfBytesToRead,
        lpBuffer,
        &bytesRead,
        &minNumberOfBytesNeeded);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    *pnBytesRead = (DWORD)bytesRead;
    *pnMinNumberOfBytesNeeded = (DWORD)minNumberOfBytesNeeded;
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
ReadEventLogW(
    IN HANDLE hEventLog,
    IN DWORD dwReadFlags,
    IN DWORD dwRecordOffset,
    OUT LPVOID lpBuffer,
    IN DWORD nNumberOfBytesToRead,
    OUT DWORD *pnBytesRead,
    OUT DWORD *pnMinNumberOfBytesNeeded)
{
    PLOG_INFO pLog;
    NTSTATUS Status;
    long bytesRead, minNumberOfBytesNeeded;

    DPRINT("%p, %lu, %lu, %p, %lu, %p, %p\n",
        hEventLog, dwReadFlags, dwRecordOffset, lpBuffer,
        nNumberOfBytesToRead, pnBytesRead, pnMinNumberOfBytesNeeded);

    pLog = (PLOG_INFO)hEventLog;
    if (!pLog)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    Status = ElfrReadELW(
        pLog->BindingHandle,
        pLog->LogHandle,
        dwReadFlags,
        dwRecordOffset,
        nNumberOfBytesToRead,
        lpBuffer,
        &bytesRead,
        &minNumberOfBytesNeeded);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    *pnBytesRead = (DWORD)bytesRead;
    *pnMinNumberOfBytesNeeded = (DWORD)minNumberOfBytesNeeded;
    return TRUE;
}


/******************************************************************************
 * RegisterEventSourceA [ADVAPI32.@]
 */
HANDLE WINAPI
RegisterEventSourceA(
    IN LPCSTR lpUNCServerName,
    IN LPCSTR lpSourceName)
{
    UNICODE_STRING UNCServerName;
    UNICODE_STRING SourceName;
    HANDLE Handle;

    DPRINT("%s, %s\n", lpUNCServerName, lpSourceName);

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

    Handle = RegisterEventSourceW(UNCServerName.Buffer, SourceName.Buffer);

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
RegisterEventSourceW(
    IN LPCWSTR lpUNCServerName,
    IN LPCWSTR lpSourceName)
{
    PLOG_INFO pLog;
    NTSTATUS Status;
    UNICODE_STRING UNCServerName;
    UNICODE_STRING SourceName;

    DPRINT("%s, %s\n", lpUNCServerName, lpSourceName);

    RtlInitUnicodeString(&UNCServerName, lpUNCServerName);
    RtlInitUnicodeString(&SourceName, lpSourceName);

    pLog = HeapAlloc(GetProcessHeap(), 0, sizeof(LOG_INFO));
    if (!pLog)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }
    ZeroMemory(pLog, sizeof(LOG_INFO));

    if (lpUNCServerName == NULL || *lpUNCServerName == 0)
    {
        pLog->bLocal = TRUE;

        if (!EvtGetLocalHandle(&pLog->BindingHandle))
        {
            HeapFree(GetProcessHeap(), 0, pLog);
            SetLastError(ERROR_GEN_FAILURE);
            return NULL;
        }
    }
    else
    {
        pLog->bLocal = FALSE;

        if (!EvtBindRpc(lpUNCServerName, &pLog->BindingHandle))
        {
            HeapFree(GetProcessHeap(), 0, pLog);
            SetLastError(ERROR_INVALID_COMPUTERNAME);
            return NULL;
        }
    }

    Status = ElfrRegisterEventSourceW(
        pLog->BindingHandle,
        UNCServerName.Buffer,
        SourceName.Buffer,
        L"",
        0,
        0,
        &pLog->LogHandle);

    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        HeapFree(GetProcessHeap(), 0, pLog);
        return NULL;
    }
    return pLog;
}


/******************************************************************************
 * ReportEventA [ADVAPI32.@]
 */
BOOL WINAPI
ReportEventA(
    IN HANDLE hEventLog,
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

    wideStrArray = HeapAlloc(
        GetProcessHeap(),
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
        ret = ReportEventW(
            hEventLog,
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
            HeapFree(
                GetProcessHeap(),
                0,
                (PVOID)wideStrArray[i]);
        }
    }

    HeapFree(
        GetProcessHeap(),
        0,
        wideStrArray);

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
ReportEventW(
    IN HANDLE hEventLog,
    IN WORD wType,
    IN WORD wCategory,
    IN DWORD dwEventID,
    IN PSID lpUserSid,
    IN WORD wNumStrings,
    IN DWORD dwDataSize,
    IN LPCWSTR *lpStrings,
    IN LPVOID lpRawData)
{
#if 0
    PLOG_INFO pLog;
    NTSTATUS Status;
    UNICODE_STRING *Strings;
    WORD i;

    DPRINT("%p, %u, %u, %lu, %p, %u, %lu, %p, %p\n",
        hEventLog, wType, wCategory, dwEventID, lpUserSid,
        wNumStrings, dwDataSize, lpStrings, lpRawData);

    pLog = (PLOG_INFO)hEventLog;
    if (!pLog)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    Strings = HeapAlloc(
        GetProcessHeap(),
        0,
        wNumStrings * sizeof(UNICODE_STRING));
    if (!Strings)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    for (i = 0; i < wNumStrings; i++)
        RtlInitUnicodeString(&Strings[i], lpStrings[i]);

    Status = ElfrReportEventW(
        pLog->BindingHandle,
        pLog->LogHandle,
        0, /* FIXME: Time */
        wType,
        wCategory,
        dwEventID,
        wNumStrings,
        dwDataSize,
        L"", /* FIXME: ComputerName */
        lpUserSid,
        (LPWSTR *)lpStrings, /* FIXME: should be Strings */
        lpRawData,
        0,
        NULL,
        NULL);
    HeapFree(GetProcessHeap(), 0, Strings);

    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }
    return TRUE;
#else
  int i;

    /* partial stub */

  if (wNumStrings == 0)
    return TRUE;

  if (lpStrings == NULL)
    return TRUE;

  for (i = 0; i < wNumStrings; i++)
    {
      switch (wType)
        {
        case EVENTLOG_SUCCESS:
            DPRINT1("Success: %S\n", lpStrings[i]);
            break;

        case EVENTLOG_ERROR_TYPE:
            DPRINT1("Error: %S\n", lpStrings[i]);
            break;

        case EVENTLOG_WARNING_TYPE:
            DPRINT1("Warning: %S\n", lpStrings[i]);
            break;

        case EVENTLOG_INFORMATION_TYPE:
            DPRINT1("Info: %S\n", lpStrings[i]);
            break;

        default:
            DPRINT1("Type %hu: %S\n", wType, lpStrings[i]);
            break;
        }
    }

  return TRUE;
#endif
}
