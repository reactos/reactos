/*
 * PROJECT:          ReactOS kernel
 * LICENSE:          GPL - See COPYING in the top level directory
 * FILE:             services/eventlog/file.c
 * PURPOSE:          Event logging service
 * COPYRIGHT:        Copyright 2005 Saveliy Tretiakov
 */

/* INCLUDES *****************************************************************/

#include "eventlog.h"

/* GLOBALS ******************************************************************/

static LIST_ENTRY LogFileListHead;
static CRITICAL_SECTION LogFileListCs;
extern HANDLE MyHeap;

/* FUNCTIONS ****************************************************************/

BOOL LogfInitializeNew(PLOGFILE LogFile)
{
    DWORD dwWritten;
    EOF_RECORD EofRec;

    ZeroMemory(&LogFile->Header, sizeof(FILE_HEADER));
    SetFilePointer(LogFile->hFile, 0, NULL, FILE_BEGIN);
    SetEndOfFile(LogFile->hFile);

    LogFile->Header.SizeOfHeader = sizeof(FILE_HEADER);
    LogFile->Header.SizeOfHeader2 = sizeof(FILE_HEADER);
    LogFile->Header.FirstRecordOffset = sizeof(FILE_HEADER);
    LogFile->Header.EofOffset = sizeof(FILE_HEADER);
    LogFile->Header.MajorVersion = MAJORVER;
    LogFile->Header.MinorVersion = MINORVER;
    LogFile->Header.NextRecord = 1;

    LogFile->Header.Signature = LOGFILE_SIGNATURE;
    if (!WriteFile(LogFile->hFile,
                   &LogFile->Header,
                   sizeof(FILE_HEADER),
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
    EofRec.Size1 = sizeof(EOF_RECORD);
    EofRec.Size2 = sizeof(EOF_RECORD);
    EofRec.NextRecordNumber = LogFile->Header.NextRecord;
    EofRec.OldestRecordNumber = LogFile->Header.OldestRecord;
    EofRec.StartOffset = LogFile->Header.FirstRecordOffset;
    EofRec.EndOffset = LogFile->Header.EofOffset;

    if (!WriteFile(LogFile->hFile,
                   &EofRec,
                   sizeof(EOF_RECORD),
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

    if (SetFilePointer(LogFile->hFile, 0, NULL, FILE_BEGIN) ==
        INVALID_SET_FILE_POINTER)
    {
        DPRINT1("SetFilePointer failed! %d\n", GetLastError());
        return FALSE;
    }

    if (!ReadFile(LogFile->hFile,
                  &LogFile->Header,
                  sizeof(FILE_HEADER),
                  &dwRead,
                  NULL))
    {
        DPRINT1("ReadFile failed! %d\n", GetLastError());
        return FALSE;
    }

    if (dwRead != sizeof(FILE_HEADER))
    {
        DPRINT("EventLog: Invalid file %S.\n", LogFile->FileName);
        return LogfInitializeNew(LogFile);
    }

    if (LogFile->Header.SizeOfHeader != sizeof(FILE_HEADER) ||
        LogFile->Header.SizeOfHeader2 != sizeof(FILE_HEADER))
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

    if (LogFile->Header.EofOffset > GetFileSize(LogFile->hFile, NULL) + 1)
    {
        DPRINT("EventLog: Invalid eof offset %x in %S.\n",
               LogFile->Header.EofOffset, LogFile->FileName);
        return LogfInitializeNew(LogFile);
    }

    for (;;)
    {
        dwFilePointer = SetFilePointer(LogFile->hFile, 0, NULL, FILE_CURRENT);

        if (dwFilePointer == INVALID_SET_FILE_POINTER)
        {
            DPRINT1("SetFilePointer failed! %d\n", GetLastError());
            return FALSE;
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

        pdwRecSize2 = (PDWORD) (((PBYTE) RecBuf) + dwRecSize - 4);

        if (*pdwRecSize2 != dwRecSize)
        {
            DPRINT1("Invalid size2 of record %d (%x) in %s\n",
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
    }  // for(;;)

    LogFile->Header.NextRecord = dwRecordsNumber + 1;
    LogFile->Header.OldestRecord = dwRecordsNumber ? 1 : 0;  // FIXME

    if (!SetFilePointer(LogFile->hFile, 0, NULL, FILE_CURRENT) ==
        INVALID_SET_FILE_POINTER)
    {
        DPRINT1("SetFilePointer() failed! %d\n", GetLastError());
        return FALSE;
    }

    if (!WriteFile(LogFile->hFile,
                   &LogFile->Header,
                   sizeof(FILE_HEADER),
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

BOOL LogfReadEvent(PLOGFILE LogFile,
                   DWORD Flags,
                   DWORD RecordNumber,
                   DWORD BufSize,
                   PBYTE Buffer,
                   DWORD * BytesRead,
                   DWORD * BytesNeeded)
{
    DWORD dwOffset, dwRead, dwRecSize;
    DWORD dwBufferUsage = 0, dwRecNum;

    if (Flags & EVENTLOG_FORWARDS_READ && Flags & EVENTLOG_BACKWARDS_READ)
        return FALSE;

    if (!(Flags & EVENTLOG_FORWARDS_READ) && !(Flags & EVENTLOG_BACKWARDS_READ))
        return FALSE;

    if (!Buffer || !BytesRead || !BytesNeeded)
        return FALSE;

    dwRecNum = RecordNumber;
    EnterCriticalSection(&LogFile->cs);
    dwOffset = LogfOffsetByNumber(LogFile, dwRecNum);

    if (!dwOffset)
    {
        LeaveCriticalSection(&LogFile->cs);
        return FALSE;
    }

    if (SetFilePointer(LogFile->hFile, dwOffset, NULL, FILE_BEGIN) ==
        INVALID_SET_FILE_POINTER)
    {
        DPRINT1("SetFilePointer() failed! %d\n", GetLastError());
        LeaveCriticalSection(&LogFile->cs);
        return FALSE;
    }

    if (!ReadFile(LogFile->hFile, &dwRecSize, sizeof(DWORD), &dwRead, NULL))
    {
        DPRINT1("ReadFile() failed! %d\n", GetLastError());
        LeaveCriticalSection(&LogFile->cs);
        return FALSE;
    }

    if (dwRecSize > BufSize)
    {
        *BytesRead = 0;
        *BytesNeeded = dwRecSize;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        LeaveCriticalSection(&LogFile->cs);
        return FALSE;
    }

    if (SetFilePointer(LogFile->hFile,
                       -((LONG) sizeof(DWORD)),
                       NULL,
                       FILE_CURRENT) == INVALID_SET_FILE_POINTER)
    {
        DPRINT1("SetFilePointer() failed! %d\n", GetLastError());
        LeaveCriticalSection(&LogFile->cs);
        return FALSE;
    }

    if (!ReadFile(LogFile->hFile, Buffer, dwRecSize, &dwRead, NULL))
    {
        DPRINT1("ReadFile() failed! %d\n", GetLastError());
        LeaveCriticalSection(&LogFile->cs);
        return FALSE;
    }

    dwBufferUsage += dwRead;

    while (dwBufferUsage < BufSize)
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
            DPRINT1("SetFilePointer() failed! %d\n", GetLastError());
            LeaveCriticalSection(&LogFile->cs);
            return FALSE;
        }

        if (!ReadFile(LogFile->hFile,
                      &dwRecSize,
                      sizeof(DWORD),
                      &dwRead,
                      NULL))
        {
            DPRINT1("ReadFile() failed! %d\n", GetLastError());
            LeaveCriticalSection(&LogFile->cs);
            return FALSE;
        }

        if (dwBufferUsage + dwRecSize > BufSize)
            break;

        if (SetFilePointer(LogFile->hFile,
                           -((LONG) sizeof(DWORD)),
                           NULL,
                           FILE_CURRENT) == INVALID_SET_FILE_POINTER)
        {
            DPRINT1("SetFilePointer() failed! %d\n", GetLastError());
            LeaveCriticalSection(&LogFile->cs);
            return FALSE;
        }

        if (!ReadFile(LogFile->hFile,
                      Buffer + dwBufferUsage,
                      dwRecSize,
                      &dwRead,
                      NULL))
        {
            DPRINT1("ReadFile() failed! %d\n", GetLastError());
            LeaveCriticalSection(&LogFile->cs);
            return FALSE;
        }

        dwBufferUsage += dwRead;
    }

    *BytesRead = dwBufferUsage;
    LeaveCriticalSection(&LogFile->cs);
    return TRUE;
}

BOOL LogfWriteData(PLOGFILE LogFile, DWORD BufSize, PBYTE Buffer)
{
    DWORD dwWritten;
    SYSTEMTIME st;
    EOF_RECORD EofRec;

    if (!Buffer)
        return FALSE;

    GetSystemTime(&st);
    SystemTimeToEventTime(&st, &((PEVENTLOGRECORD) Buffer)->TimeWritten);

    EnterCriticalSection(&LogFile->cs);

    if (SetFilePointer(LogFile->hFile,
                       LogFile->Header.EofOffset,
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
                                  LogFile->Header.NextRecord,
                                  LogFile->Header.EofOffset))
    {
        LeaveCriticalSection(&LogFile->cs);
        return FALSE;
    }

    LogFile->Header.NextRecord++;
    LogFile->Header.EofOffset += dwWritten;

    if (LogFile->Header.OldestRecord == 0)
        LogFile->Header.OldestRecord = 1;

    EofRec.Ones = 0x11111111;
    EofRec.Twos = 0x22222222;
    EofRec.Threes = 0x33333333;
    EofRec.Fours = 0x44444444;
    EofRec.Size1 = sizeof(EOF_RECORD);
    EofRec.Size2 = sizeof(EOF_RECORD);
    EofRec.NextRecordNumber = LogFile->Header.NextRecord;
    EofRec.OldestRecordNumber = LogFile->Header.OldestRecord;
    EofRec.StartOffset = LogFile->Header.FirstRecordOffset;
    EofRec.EndOffset = LogFile->Header.EofOffset;

    if (!WriteFile(LogFile->hFile,
                   &EofRec,
                   sizeof(EOF_RECORD),
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
                   sizeof(FILE_HEADER),
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
    return LogFile->Header.OldestRecord;
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

void __inline LogfFreeRecord(LPVOID Rec)
{
    HeapFree(MyHeap, 0, Rec);
}
