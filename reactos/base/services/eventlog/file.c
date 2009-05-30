/*
 * PROJECT:          ReactOS kernel
 * LICENSE:          GPL - See COPYING in the top level directory
 * FILE:             services/eventlog/file.c
 * PURPOSE:          Event logging service
 * COPYRIGHT:        Copyright 2005 Saveliy Tretiakov
                     Michael Martin
 */

/* INCLUDES *****************************************************************/

#include "eventlog.h"

/* GLOBALS ******************************************************************/

static LIST_ENTRY LogFileListHead;
static CRITICAL_SECTION LogFileListCs;

/* FUNCTIONS ****************************************************************/

BOOL LogfInitializeNew(PLOGFILE LogFile)
{
    DWORD dwWritten;
    EVENTLOGEOF EofRec;

    ZeroMemory(&LogFile->Header, sizeof(EVENTLOGHEADER));
    SetFilePointer(LogFile->hFile, 0, NULL, FILE_BEGIN);
    SetEndOfFile(LogFile->hFile);

    LogFile->Header.HeaderSize = sizeof(EVENTLOGHEADER);
    LogFile->Header.EndHeaderSize = sizeof(EVENTLOGHEADER);
    LogFile->Header.StartOffset = sizeof(EVENTLOGHEADER);
    LogFile->Header.EndOffset = sizeof(EVENTLOGHEADER);
    LogFile->Header.MajorVersion = MAJORVER;
    LogFile->Header.MinorVersion = MINORVER;
    LogFile->Header.CurrentRecordNumber = 1;
    /* FIXME: Read MaxSize from registry for this LogFile.
       But for now limit EventLog size to just under 5K. */
    LogFile->Header.MaxSize = 5000;
    LogFile->Header.Signature = LOGFILE_SIGNATURE;
    if (!WriteFile(LogFile->hFile,
                   &LogFile->Header,
                   sizeof(EVENTLOGHEADER),
                   &dwWritten,
                   NULL))
    {
        DPRINT1("WriteFile failed:%d!\n", GetLastError());
        return FALSE;
    }

    EofRec.Ones = 0x11111111;
    EofRec.Twos = 0x22222222;
    EofRec.Threes = 0x33333333;
    EofRec.Fours = 0x44444444;
    EofRec.RecordSizeBeginning = sizeof(EVENTLOGEOF);
    EofRec.RecordSizeEnd = sizeof(EVENTLOGEOF);
    EofRec.CurrentRecordNumber = LogFile->Header.CurrentRecordNumber;
    EofRec.OldestRecordNumber = LogFile->Header.OldestRecordNumber;
    EofRec.BeginRecord = LogFile->Header.StartOffset;
    EofRec.EndRecord = LogFile->Header.EndOffset;

    if (!WriteFile(LogFile->hFile,
                   &EofRec,
                   sizeof(EVENTLOGEOF),
                   &dwWritten,
                   NULL))
    {
        DPRINT1("WriteFile failed:%d!\n", GetLastError());
        return FALSE;
    }

    if (!FlushFileBuffers(LogFile->hFile))
    {
        DPRINT1("FlushFileBuffers failed:%d!\n", GetLastError());
        return FALSE;
    }

    return TRUE;
}

BOOL LogfInitializeExisting(PLOGFILE LogFile)
{
    DWORD dwRead;
    DWORD dwRecordsNumber = 0;
    DWORD dwRecSize, dwRecSign, dwFilePointer;
    PDWORD pdwRecSize2;
    PEVENTLOGRECORD RecBuf;
    BOOL OvewrWrittenRecords = FALSE;

    DPRINT("Initializing LogFile %S\n",LogFile->LogName);

    if (SetFilePointer(LogFile->hFile, 0, NULL, FILE_BEGIN) ==
        INVALID_SET_FILE_POINTER)
    {
        DPRINT1("SetFilePointer failed! %d\n", GetLastError());
        return FALSE;
    }

    if (!ReadFile(LogFile->hFile,
                  &LogFile->Header,
                  sizeof(EVENTLOGHEADER),
                  &dwRead,
                  NULL))
    {
        DPRINT1("ReadFile failed! %d\n", GetLastError());
        return FALSE;
    }

    if (dwRead != sizeof(EVENTLOGHEADER))
    {
        DPRINT("EventLog: Invalid file %S.\n", LogFile->FileName);
        return LogfInitializeNew(LogFile);
    }

    if (LogFile->Header.HeaderSize != sizeof(EVENTLOGHEADER) ||
        LogFile->Header.EndHeaderSize != sizeof(EVENTLOGHEADER))
    {
        DPRINT("EventLog: Invalid header size in %S.\n", LogFile->FileName);
        return LogfInitializeNew(LogFile);
    }

    if (LogFile->Header.Signature != LOGFILE_SIGNATURE)
    {
        DPRINT("EventLog: Invalid signature %x in %S.\n",
               LogFile->Header.Signature, LogFile->FileName);
        return LogfInitializeNew(LogFile);
    }

    if (LogFile->Header.EndOffset > GetFileSize(LogFile->hFile, NULL) + 1)
    {
        DPRINT("EventLog: Invalid eof offset %x in %S.\n",
               LogFile->Header.EndOffset, LogFile->FileName);
        return LogfInitializeNew(LogFile);
    }

    /* Set the read location to the oldest record */
    dwFilePointer = SetFilePointer(LogFile->hFile, LogFile->Header.StartOffset, NULL, FILE_BEGIN);
    if (dwFilePointer == INVALID_SET_FILE_POINTER)
    {
        DPRINT1("SetFilePointer failed! %d\n", GetLastError());
        return FALSE;
    }

    for (;;)
    {
        dwFilePointer = SetFilePointer(LogFile->hFile, 0, NULL, FILE_CURRENT);

        if (dwFilePointer == INVALID_SET_FILE_POINTER)
        {
            DPRINT1("SetFilePointer failed! %d\n", GetLastError());
            return FALSE;
        }

        /* If the EVENTLOGEOF info has been reached and the oldest record was not immediately after the Header */
        if ((dwFilePointer == LogFile->Header.EndOffset) && (LogFile->Header.StartOffset != sizeof(EVENTLOGHEADER)))
        {
            OvewrWrittenRecords = TRUE;
            /* The file has records that overwrote old ones so read them */
            dwFilePointer = SetFilePointer(LogFile->hFile, sizeof(EVENTLOGHEADER), NULL, FILE_BEGIN);
        }

        if (!ReadFile(LogFile->hFile,
                      &dwRecSize,
                      sizeof(dwRecSize),
                      &dwRead,
                      NULL))
        {
            DPRINT1("ReadFile failed! %d\n", GetLastError());
            return FALSE;
        }

        if (dwRead != sizeof(dwRecSize))
            break;

        if (!ReadFile(LogFile->hFile,
                      &dwRecSign,
                      sizeof(dwRecSign),
                      &dwRead,
                      NULL))
        {
            DPRINT1("ReadFile() failed! %d\n", GetLastError());
            return FALSE;
        }

        if (dwRead != sizeof(dwRecSize))
            break;

        if (dwRecSign != LOGFILE_SIGNATURE ||
            dwRecSize + dwFilePointer > GetFileSize(LogFile->hFile, NULL) + 1 ||
            dwRecSize < sizeof(EVENTLOGRECORD))
        {
            break;
        }

        if (SetFilePointer(LogFile->hFile,
                           -((LONG) sizeof(DWORD) * 2),
                           NULL,
                           FILE_CURRENT) == INVALID_SET_FILE_POINTER)
        {
            DPRINT1("SetFilePointer() failed! %d", GetLastError());
            return FALSE;
        }

        RecBuf = (PEVENTLOGRECORD) HeapAlloc(MyHeap, 0, dwRecSize);

        if (!RecBuf)
        {
            DPRINT1("Can't allocate heap!\n");
            return FALSE;
        }

        if (!ReadFile(LogFile->hFile, RecBuf, dwRecSize, &dwRead, NULL))
        {
            DPRINT1("ReadFile() failed! %d\n", GetLastError());
            HeapFree(MyHeap, 0, RecBuf);
            return FALSE;
        }

        if (dwRead != dwRecSize)
        {
            HeapFree(MyHeap, 0, RecBuf);
            break;
        }

        /* if OvewrWrittenRecords is TRUE and this record has already been read */
        if ((OvewrWrittenRecords == TRUE) && (RecBuf->RecordNumber == LogFile->Header.OldestRecordNumber))
        {
            HeapFree(MyHeap, 0, RecBuf);
            break;
        }

        pdwRecSize2 = (PDWORD) (((PBYTE) RecBuf) + dwRecSize - 4);

        if (*pdwRecSize2 != dwRecSize)
        {
            DPRINT1("Invalid RecordSizeEnd of record %d (%x) in %S\n",
                    dwRecordsNumber, *pdwRecSize2, LogFile->LogName);
            HeapFree(MyHeap, 0, RecBuf);
            break;
        }

        dwRecordsNumber++;

        if (!LogfAddOffsetInformation(LogFile,
                                      RecBuf->RecordNumber,
                                      dwFilePointer))
        {
            DPRINT1("LogfAddOffsetInformation() failed!\n");
            HeapFree(MyHeap, 0, RecBuf);
            return FALSE;
        }

        HeapFree(MyHeap, 0, RecBuf);
    }

    LogFile->Header.CurrentRecordNumber = dwRecordsNumber + LogFile->Header.OldestRecordNumber;
    if (LogFile->Header.CurrentRecordNumber == 0)
        LogFile->Header.CurrentRecordNumber = 1;

    /* FIXME: Read MaxSize from registry for this LogFile.
       But for now limit EventLog size to just under 5K. */
    LogFile->Header.MaxSize = 5000;

    if (!SetFilePointer(LogFile->hFile, 0, NULL, FILE_BEGIN) ==
        INVALID_SET_FILE_POINTER)
    {
        DPRINT1("SetFilePointer() failed! %d\n", GetLastError());
        return FALSE;
    }

    if (!WriteFile(LogFile->hFile,
                   &LogFile->Header,
                   sizeof(EVENTLOGHEADER),
                   &dwRead,
                   NULL))
    {
        DPRINT1("WriteFile failed! %d\n", GetLastError());
        return FALSE;
    }

    if (!FlushFileBuffers(LogFile->hFile))
    {
        DPRINT1("FlushFileBuffers failed! %d\n", GetLastError());
        return FALSE;
    }

    return TRUE;
}

PLOGFILE LogfCreate(WCHAR * LogName, WCHAR * FileName)
{
    PLOGFILE LogFile;
    BOOL bResult, bCreateNew = FALSE;

    LogFile = (LOGFILE *) HeapAlloc(MyHeap, HEAP_ZERO_MEMORY, sizeof(LOGFILE));
    if (!LogFile)
    {
        DPRINT1("Can't allocate heap!\n");
        return NULL;
    }

    LogFile->hFile = CreateFile(FileName,
                                GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ,
                                NULL,
                                OPEN_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS,
                                NULL);

    if (LogFile->hFile == INVALID_HANDLE_VALUE)
    {
        DPRINT1("Can't create file %S.\n", FileName);
        HeapFree(MyHeap, 0, LogFile);
        return NULL;
    }

    bCreateNew = (GetLastError() == ERROR_ALREADY_EXISTS) ? FALSE : TRUE;

    LogFile->LogName =
        (WCHAR *) HeapAlloc(MyHeap,
                            HEAP_ZERO_MEMORY,
                            (lstrlenW(LogName) + 1) * sizeof(WCHAR));

    if (LogFile->LogName)
        lstrcpyW(LogFile->LogName, LogName);
    else
    {
        DPRINT1("Can't allocate heap\n");
        HeapFree(MyHeap, 0, LogFile);
        return NULL;
    }

    LogFile->FileName =
        (WCHAR *) HeapAlloc(MyHeap,
                            HEAP_ZERO_MEMORY,
                            (lstrlenW(FileName) + 1) * sizeof(WCHAR));

    if (LogFile->FileName)
        lstrcpyW(LogFile->FileName, FileName);
    else
    {
        DPRINT1("Can't allocate heap\n");
        goto fail;
    }

    LogFile->OffsetInfo =
        (PEVENT_OFFSET_INFO) HeapAlloc(MyHeap,
                                       HEAP_ZERO_MEMORY,
                                       sizeof(EVENT_OFFSET_INFO) * 64);

    if (!LogFile->OffsetInfo)
    {
        DPRINT1("Can't allocate heap\n");
        goto fail;
    }

    LogFile->OffsetInfoSize = 64;

    if (bCreateNew)
        bResult = LogfInitializeNew(LogFile);
    else
        bResult = LogfInitializeExisting(LogFile);

    if (!bResult)
        goto fail;

    InitializeCriticalSection(&LogFile->cs);
    LogfListAddItem(LogFile);
    return LogFile;

  fail:
    if (LogFile)
    {
        if (LogFile->OffsetInfo)
            HeapFree(MyHeap, 0, LogFile->OffsetInfo);

        if (LogFile->FileName)
            HeapFree(MyHeap, 0, LogFile->FileName);

        if (LogFile->LogName)
            HeapFree(MyHeap, 0, LogFile->LogName);

        HeapFree(MyHeap, 0, LogFile);
    }

    return NULL;
}

VOID LogfClose(PLOGFILE LogFile)
{
    if (LogFile == NULL)
        return;

    EnterCriticalSection(&LogFile->cs);

    FlushFileBuffers(LogFile->hFile);
    CloseHandle(LogFile->hFile);
    LogfListRemoveItem(LogFile);

    DeleteCriticalSection(&LogFile->cs);

    HeapFree(MyHeap, 0, LogFile->LogName);
    HeapFree(MyHeap, 0, LogFile->FileName);
    HeapFree(MyHeap, 0, LogFile->OffsetInfo);
    HeapFree(MyHeap, 0, LogFile);

    return;
}

VOID LogfCloseAll(VOID)
{
    while (!IsListEmpty(&LogFileListHead))
    {
        LogfClose(LogfListHead());
    }

    DeleteCriticalSection(&LogFileListCs);
}

VOID LogfListInitialize(VOID)
{
    InitializeCriticalSection(&LogFileListCs);
    InitializeListHead(&LogFileListHead);
}

PLOGFILE LogfListHead(VOID)
{
    return CONTAINING_RECORD(LogFileListHead.Flink, LOGFILE, ListEntry);
}

PLOGFILE LogfListItemByName(WCHAR * Name)
{
    PLIST_ENTRY CurrentEntry;
    PLOGFILE Result = NULL;

    EnterCriticalSection(&LogFileListCs);

    CurrentEntry = LogFileListHead.Flink;
    while (CurrentEntry != &LogFileListHead)
    {
        PLOGFILE Item = CONTAINING_RECORD(CurrentEntry,
                                          LOGFILE,
                                          ListEntry);

        if (Item->LogName && !lstrcmpi(Item->LogName, Name))
        {
            Result = Item;
            break;
        }

        CurrentEntry = CurrentEntry->Flink;
    }

    LeaveCriticalSection(&LogFileListCs);
    return Result;
}

/* Index starting from 1 */
INT LogfListItemIndexByName(WCHAR * Name)
{
    PLIST_ENTRY CurrentEntry;
    INT Result = 0;
    INT i = 1;

    EnterCriticalSection(&LogFileListCs);

    CurrentEntry = LogFileListHead.Flink;
    while (CurrentEntry != &LogFileListHead)
    {
        PLOGFILE Item = CONTAINING_RECORD(CurrentEntry,
                                          LOGFILE,
                                          ListEntry);

        if (Item->LogName && !lstrcmpi(Item->LogName, Name))
        {
            Result = i;
            break;
        }

        CurrentEntry = CurrentEntry->Flink;
        i++;
    }

    LeaveCriticalSection(&LogFileListCs);
    return Result;
}

/* Index starting from 1 */
PLOGFILE LogfListItemByIndex(INT Index)
{
    PLIST_ENTRY CurrentEntry;
    PLOGFILE Result = NULL;
    INT i = 1;

    EnterCriticalSection(&LogFileListCs);

    CurrentEntry = LogFileListHead.Flink;
    while (CurrentEntry != &LogFileListHead)
    {
        if (i == Index)
        {
            Result = CONTAINING_RECORD(CurrentEntry, LOGFILE, ListEntry);
            break;
        }

        CurrentEntry = CurrentEntry->Flink;
        i++;
    }

    LeaveCriticalSection(&LogFileListCs);
    return Result;
}

INT LogfListItemCount()
{
    PLIST_ENTRY CurrentEntry;
    INT i = 0;

    EnterCriticalSection(&LogFileListCs);

    CurrentEntry = LogFileListHead.Flink;
    while (CurrentEntry != &LogFileListHead)
    {
        CurrentEntry = CurrentEntry->Flink;
        i++;
    }

    LeaveCriticalSection(&LogFileListCs);
    return i;
}

VOID LogfListAddItem(PLOGFILE Item)
{
    EnterCriticalSection(&LogFileListCs);
    InsertTailList(&LogFileListHead, &Item->ListEntry);
    LeaveCriticalSection(&LogFileListCs);
}

VOID LogfListRemoveItem(PLOGFILE Item)
{
    EnterCriticalSection(&LogFileListCs);
    RemoveEntryList(&Item->ListEntry);
    LeaveCriticalSection(&LogFileListCs);
}

DWORD LogfReadEvent(PLOGFILE LogFile,
                   DWORD Flags,
                   DWORD * RecordNumber,
                   DWORD BufSize,
                   PBYTE Buffer,
                   DWORD * BytesRead,
                   DWORD * BytesNeeded)
{
    DWORD dwOffset, dwRead, dwRecSize;
    DWORD dwBufferUsage = 0, dwRecNum;

    if (Flags & EVENTLOG_FORWARDS_READ && Flags & EVENTLOG_BACKWARDS_READ)
        return ERROR_INVALID_PARAMETER;

    if (!(Flags & EVENTLOG_FORWARDS_READ) && !(Flags & EVENTLOG_BACKWARDS_READ))
        return ERROR_INVALID_PARAMETER;

    if (!Buffer || !BytesRead || !BytesNeeded)
        return ERROR_INVALID_PARAMETER;

    if ((*RecordNumber==0) && !(EVENTLOG_SEQUENTIAL_READ))
    {
        return ERROR_INVALID_PARAMETER;
    }

    dwRecNum = *RecordNumber;
    EnterCriticalSection(&LogFile->cs);

    *BytesRead = 0;
    *BytesNeeded = 0;

    dwOffset = LogfOffsetByNumber(LogFile, dwRecNum);

    if (!dwOffset)
    {
        LeaveCriticalSection(&LogFile->cs);
        return ERROR_HANDLE_EOF;
    }

    if (SetFilePointer(LogFile->hFile, dwOffset, NULL, FILE_BEGIN) ==
        INVALID_SET_FILE_POINTER)
    {
        DPRINT1("SetFilePointer() failed!\n");
        goto Done;
    }

    if (!ReadFile(LogFile->hFile, &dwRecSize, sizeof(DWORD), &dwRead, NULL))
    {
        DPRINT1("ReadFile() failed!\n");
        goto Done;
    }

    if (dwRecSize > BufSize)
    {
        *BytesNeeded = dwRecSize;
        LeaveCriticalSection(&LogFile->cs);
        return ERROR_INSUFFICIENT_BUFFER;
    }

    if (SetFilePointer(LogFile->hFile,
                       -((LONG) sizeof(DWORD)),
                       NULL,
                       FILE_CURRENT) == INVALID_SET_FILE_POINTER)
    {
        DPRINT1("SetFilePointer() failed!\n");
        goto Done;
    }

    if (!ReadFile(LogFile->hFile, Buffer, dwRecSize, &dwRead, NULL))
    {
        DPRINT1("ReadFile() failed!\n");
        goto Done;
    }

    dwBufferUsage += dwRead;

    while (dwBufferUsage <= BufSize)
    {
        if (Flags & EVENTLOG_FORWARDS_READ)
            dwRecNum++;
        else
            dwRecNum--;

        dwOffset = LogfOffsetByNumber(LogFile, dwRecNum);
        if (!dwOffset)
            break;

        if (SetFilePointer(LogFile->hFile, dwOffset, NULL, FILE_BEGIN) ==
            INVALID_SET_FILE_POINTER)
        {
            DPRINT1("SetFilePointer() failed!\n");
            goto Done;
        }

        if (!ReadFile(LogFile->hFile,
                      &dwRecSize,
                      sizeof(DWORD),
                      &dwRead,
                      NULL))
        {
            DPRINT1("ReadFile() failed!\n");
            goto Done;
        }

        if (dwBufferUsage + dwRecSize > BufSize)
            break;

        if (SetFilePointer(LogFile->hFile,
                           -((LONG) sizeof(DWORD)),
                           NULL,
                           FILE_CURRENT) == INVALID_SET_FILE_POINTER)
        {
            DPRINT1("SetFilePointer() failed!\n");
            goto Done;
        }

        if (!ReadFile(LogFile->hFile,
                      Buffer + dwBufferUsage,
                      dwRecSize,
                      &dwRead,
                      NULL))
        {
            DPRINT1("ReadFile() failed!\n");
            goto Done;
        }

        dwBufferUsage += dwRead;
    }

    *BytesRead = dwBufferUsage;
    * RecordNumber = dwRecNum;
    LeaveCriticalSection(&LogFile->cs);
    return ERROR_SUCCESS;

Done:
    DPRINT1("LogfReadEvent failed with %x\n",GetLastError());
    LeaveCriticalSection(&LogFile->cs);
    return GetLastError();
}

BOOL LogfWriteData(PLOGFILE LogFile, DWORD BufSize, PBYTE Buffer)
{
    DWORD dwWritten;
    DWORD dwRead;
    SYSTEMTIME st;
    EVENTLOGEOF EofRec;
    PEVENTLOGRECORD RecBuf;
    LARGE_INTEGER logFileSize;
    ULONG RecOffSet;
    ULONG WriteOffSet;

    if (!Buffer)
        return FALSE;

    GetSystemTime(&st);
    SystemTimeToEventTime(&st, &((PEVENTLOGRECORD) Buffer)->TimeWritten);

    EnterCriticalSection(&LogFile->cs);

    if (!GetFileSizeEx(LogFile->hFile, &logFileSize))
    {
        return FALSE;
    }

    /* If the size of the file is over MaxSize */
    if ((logFileSize.QuadPart + BufSize)> LogFile->Header.MaxSize)
    {
        ULONG OverWriteLength = 0;
        WriteOffSet = LogfOffsetByNumber(LogFile, LogFile->Header.OldestRecordNumber);
        RecBuf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(EVENTLOGRECORD));
        /* Determine how many records need to be overwritten */
        while (TRUE)
        {
            DPRINT("EventLogFile has reached maximume size\n");

            if (!RecBuf)
            {
                DPRINT1("Failed to allocate buffer for OldestRecord!\n");
                HeapFree(GetProcessHeap(), 0, RecBuf);
                LeaveCriticalSection(&LogFile->cs);
                return FALSE;
            }

            /* Get the oldest record data */
            RecOffSet = LogfOffsetByNumber(LogFile, LogFile->Header.OldestRecordNumber);

            if (SetFilePointer(LogFile->hFile,
                               RecOffSet,
                               NULL,
                               FILE_BEGIN) == INVALID_SET_FILE_POINTER)
            {
                DPRINT1("SetFilePointer() failed! %d\n", GetLastError());
                HeapFree(GetProcessHeap(), 0, RecBuf);
                LeaveCriticalSection(&LogFile->cs);
                return FALSE;
            }

            if (!ReadFile(LogFile->hFile, RecBuf, sizeof(EVENTLOGRECORD), &dwRead, NULL))
            {
                DPRINT1("ReadFile() failed!\n");
                HeapFree(GetProcessHeap(), 0, RecBuf);
                LeaveCriticalSection(&LogFile->cs);
                return FALSE;
            }

            if (RecBuf->Reserved != LOGFILE_SIGNATURE)
            {
                DPRINT1("LogFile corrupt!\n");
                return FALSE;
            }

            LogfDeleteOffsetInformation(LogFile,LogFile->Header.OldestRecordNumber);

            LogFile->Header.OldestRecordNumber++;

            OverWriteLength += RecBuf->Length;
            /* Check the size of the record as the record adding may be larger */
            if (OverWriteLength >= BufSize)
            {
                DPRINT("Record will fit. Lenght %d, BufSize %d\n", OverWriteLength, BufSize);
                LogFile->Header.StartOffset = LogfOffsetByNumber(LogFile, LogFile->Header.OldestRecordNumber);
                break;
            }
        }
        HeapFree(GetProcessHeap(), 0, RecBuf);
    }
    else
        WriteOffSet = LogFile->Header.EndOffset;

    if (SetFilePointer(LogFile->hFile,
                       WriteOffSet,
                       NULL,
                       FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        DPRINT1("SetFilePointer() failed! %d\n", GetLastError());
        LeaveCriticalSection(&LogFile->cs);
        return FALSE;
    }

    if (!WriteFile(LogFile->hFile, Buffer, BufSize, &dwWritten, NULL))
    {
        DPRINT1("WriteFile() failed! %d\n", GetLastError());
        LeaveCriticalSection(&LogFile->cs);
        return FALSE;
    }

    if (!LogfAddOffsetInformation(LogFile,
                                  LogFile->Header.CurrentRecordNumber,
                                  WriteOffSet))
    {
        LeaveCriticalSection(&LogFile->cs);
        return FALSE;
    }

    LogFile->Header.CurrentRecordNumber++;

    if (LogFile->Header.OldestRecordNumber == 0)
        LogFile->Header.OldestRecordNumber = 1;

    if (WriteOffSet == LogFile->Header.EndOffset)
    {
        LogFile->Header.EndOffset += dwWritten;
    }
    if (SetFilePointer(LogFile->hFile,
                       LogFile->Header.EndOffset,
                       NULL,
                       FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        DPRINT1("SetFilePointer() failed! %d\n", GetLastError());
        LeaveCriticalSection(&LogFile->cs);
        return FALSE;
    }

    EofRec.Ones = 0x11111111;
    EofRec.Twos = 0x22222222;
    EofRec.Threes = 0x33333333;
    EofRec.Fours = 0x44444444;
    EofRec.RecordSizeBeginning = sizeof(EVENTLOGEOF);
    EofRec.RecordSizeEnd = sizeof(EVENTLOGEOF);
    EofRec.CurrentRecordNumber = LogFile->Header.CurrentRecordNumber;
    EofRec.OldestRecordNumber = LogFile->Header.OldestRecordNumber;
    EofRec.BeginRecord = LogFile->Header.StartOffset;
    EofRec.EndRecord = LogFile->Header.EndOffset;

    if (!WriteFile(LogFile->hFile,
                   &EofRec,
                   sizeof(EVENTLOGEOF),
                   &dwWritten,
                   NULL))
    {
        DPRINT1("WriteFile() failed! %d\n", GetLastError());
        LeaveCriticalSection(&LogFile->cs);
        return FALSE;
    }

    if (SetFilePointer(LogFile->hFile, 0, NULL, FILE_BEGIN) ==
        INVALID_SET_FILE_POINTER)
    {
        DPRINT1("SetFilePointer() failed! %d\n", GetLastError());
        LeaveCriticalSection(&LogFile->cs);
        return FALSE;
    }

    if (!WriteFile(LogFile->hFile,
                   &LogFile->Header,
                   sizeof(EVENTLOGHEADER),
                   &dwWritten,
                   NULL))
    {
        DPRINT1("WriteFile failed! LastError = %d\n", GetLastError());
        LeaveCriticalSection(&LogFile->cs);
        return FALSE;
    }

    if (!FlushFileBuffers(LogFile->hFile))
    {
        LeaveCriticalSection(&LogFile->cs);
        DPRINT1("FlushFileBuffers() failed! %d\n", GetLastError());
        return FALSE;
    }

    LeaveCriticalSection(&LogFile->cs);
    return TRUE;
}

ULONG LogfOffsetByNumber(PLOGFILE LogFile, DWORD RecordNumber)

/* Returns 0 if nothing found. */
{
    DWORD i;

    for (i = 0; i < LogFile->OffsetInfoNext; i++)
    {
        if (LogFile->OffsetInfo[i].EventNumber == RecordNumber)
            return LogFile->OffsetInfo[i].EventOffset;
    }
    return 0;
}

DWORD LogfGetOldestRecord(PLOGFILE LogFile)
{
    return LogFile->Header.OldestRecordNumber;
}

DWORD LogfGetCurrentRecord(PLOGFILE LogFile)
{
    return LogFile->Header.CurrentRecordNumber;
}

BOOL LogfDeleteOffsetInformation(PLOGFILE LogFile, ULONG ulNumber)
{
    int i;

    if (ulNumber != LogFile->OffsetInfo[0].EventNumber)
    {
        return FALSE;
    }

    for (i=0;i<LogFile->OffsetInfoNext-1; i++)
    {
        LogFile->OffsetInfo[i].EventNumber = LogFile->OffsetInfo[i+1].EventNumber;
        LogFile->OffsetInfo[i].EventOffset = LogFile->OffsetInfo[i+1].EventOffset;
    }
    LogFile->OffsetInfoNext--;
    return TRUE;
}

BOOL LogfAddOffsetInformation(PLOGFILE LogFile, ULONG ulNumber, ULONG ulOffset)
{
    LPVOID NewOffsetInfo;

    if (LogFile->OffsetInfoNext == LogFile->OffsetInfoSize)
    {
        NewOffsetInfo = HeapReAlloc(MyHeap,
                                    HEAP_ZERO_MEMORY,
                                    LogFile->OffsetInfo,
                                    (LogFile->OffsetInfoSize + 64) *
                                        sizeof(EVENT_OFFSET_INFO));

        if (!NewOffsetInfo)
        {
            DPRINT1("Can't reallocate heap.\n");
            return FALSE;
        }

        LogFile->OffsetInfo = (PEVENT_OFFSET_INFO) NewOffsetInfo;
        LogFile->OffsetInfoSize += 64;
    }

    LogFile->OffsetInfo[LogFile->OffsetInfoNext].EventNumber = ulNumber;
    LogFile->OffsetInfo[LogFile->OffsetInfoNext].EventOffset = ulOffset;
    LogFile->OffsetInfoNext++;

    return TRUE;
}

PBYTE LogfAllocAndBuildNewRecord(LPDWORD lpRecSize,
                                 DWORD   dwRecordNumber,
                                 WORD    wType,
                                 WORD    wCategory,
                                 DWORD   dwEventId,
                                 LPCWSTR SourceName,
                                 LPCWSTR ComputerName,
                                 DWORD   dwSidLength,
                                 PSID    lpUserSid,
                                 WORD    wNumStrings,
                                 WCHAR   * lpStrings,
                                 DWORD   dwDataSize,
                                 LPVOID  lpRawData)
{
    DWORD dwRecSize;
    PEVENTLOGRECORD pRec;
    SYSTEMTIME SysTime;
    WCHAR *str;
    UINT i, pos, nStrings;
    PBYTE Buffer;

    dwRecSize =
        sizeof(EVENTLOGRECORD) + (lstrlenW(ComputerName) +
                                  lstrlenW(SourceName) + 2) * sizeof(WCHAR);

    if (dwRecSize % 4 != 0)
        dwRecSize += 4 - (dwRecSize % 4);

    dwRecSize += dwSidLength;

    for (i = 0, str = lpStrings; i < wNumStrings; i++)
    {
        dwRecSize += (lstrlenW(str) + 1) * sizeof(WCHAR);
        str += lstrlenW(str) + 1;
    }

    dwRecSize += dwDataSize;
    if (dwRecSize % 4 != 0)
        dwRecSize += 4 - (dwRecSize % 4);

    dwRecSize += 4;

    Buffer = (BYTE *) HeapAlloc(MyHeap, HEAP_ZERO_MEMORY, dwRecSize);

    if (!Buffer)
    {
        DPRINT1("Can't allocate heap!\n");
        return NULL;
    }

    pRec = (PEVENTLOGRECORD) Buffer;
    pRec->Length = dwRecSize;
    pRec->Reserved = LOGFILE_SIGNATURE;
    pRec->RecordNumber = dwRecordNumber;

    GetSystemTime(&SysTime);
    SystemTimeToEventTime(&SysTime, &pRec->TimeGenerated);
    SystemTimeToEventTime(&SysTime, &pRec->TimeWritten);

    pRec->EventID = dwEventId;
    pRec->EventType = wType;
    pRec->NumStrings = wNumStrings;
    pRec->EventCategory = wCategory;

    pos = sizeof(EVENTLOGRECORD);

    lstrcpyW((WCHAR *) (Buffer + pos), SourceName);
    pos += (lstrlenW(SourceName) + 1) * sizeof(WCHAR);
    lstrcpyW((WCHAR *) (Buffer + pos), ComputerName);
    pos += (lstrlenW(ComputerName) + 1) * sizeof(WCHAR);

    pRec->UserSidOffset = pos;
    if (dwSidLength)
    {
        if (pos % 4 != 0)
            pos += 4 - (pos % 4);
        CopyMemory(Buffer + pos, lpUserSid, dwSidLength);
        pRec->UserSidLength = dwSidLength;
        pRec->UserSidOffset = pos;
        pos += dwSidLength;
    }

    pRec->StringOffset = pos;
    for (i = 0, str = lpStrings, nStrings = 0; i < wNumStrings; i++)
    {
        lstrcpyW((WCHAR *) (Buffer + pos), str);
        pos += (lstrlenW(str) + 1) * sizeof(WCHAR);
        str += lstrlenW(str) + 1;
        nStrings++;
    }
    pRec->NumStrings = nStrings;

    pRec->DataOffset = pos;
    if (dwDataSize)
    {
        pRec->DataLength = dwDataSize;
        CopyMemory(Buffer + pos, lpRawData, dwDataSize);
        pos += dwDataSize;
    }

    if (pos % 4 != 0)
        pos += 4 - (pos % 4);

    *((PDWORD) (Buffer + pos)) = dwRecSize;

    *lpRecSize = dwRecSize;
    return Buffer;
}
