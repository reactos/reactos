/*
 * PROJECT:          ReactOS kernel
 * LICENSE:          GPL - See COPYING in the top level directory
 * FILE:             services/eventlog/eventlog.h
 * PURPOSE:          Event logging service
 * COPYRIGHT:        Copyright 2005 Saveliy Tretiakov
 */

#ifndef __EVENTLOG_H__
#define __EVENTLOG_H__

#define NDEBUG
#define WIN32_NO_STATUS

#include <windows.h>
#include <lpctypes.h>
#include <lpcfuncs.h>
#include <rtlfuncs.h>
#include <obfuncs.h>
#include <iotypes.h>
#include <debug.h>
#include "eventlogrpc_s.h"

typedef struct _IO_ERROR_LPC
{
    PORT_MESSAGE Header;
    IO_ERROR_LOG_MESSAGE Message;
} IO_ERROR_LPC, *PIO_ERROR_LPC;

#define MAJORVER 1
#define MINORVER 1

/*
 *  Our file format will be compatible with NT's
 */
#define LOGFILE_SIGNATURE 0x654c664c

/*
 *  Flags used in logfile header
 */
#define ELF_LOGFILE_HEADER_DIRTY 1
#define ELF_LOGFILE_HEADER_WRAP 2
#define ELF_LOGGFILE_LOGFULL_WRITTEN 4
#define ELF_LOGFILE_ARCHIVE_SET 8

/* FIXME: MSDN reads that the following two structs are in winnt.h. Are they? */
typedef struct _EVENTLOGHEADER {
    ULONG HeaderSize;
    ULONG Signature;
    ULONG MajorVersion;
    ULONG MinorVersion;
    ULONG StartOffset;
    ULONG EndOffset;
    ULONG CurrentRecordNumber;
    ULONG OldestRecordNumber;
    ULONG MaxSize;
    ULONG Flags;
    ULONG Retention;
    ULONG EndHeaderSize;
} EVENTLOGHEADER, *PEVENTLOGHEADER;

typedef struct _EVENTLOGEOF {
    ULONG RecordSizeBeginning;
    ULONG Ones;
    ULONG Twos;
    ULONG Threes;
    ULONG Fours;
    ULONG BeginRecord;
    ULONG EndRecord;
    ULONG CurrentRecordNumber;
    ULONG OldestRecordNumber;
    ULONG RecordSizeEnd;
} EVENTLOGEOF, *PEVENTLOGEOF;

typedef struct
{
    ULONG EventNumber;
    ULONG EventOffset;
} EVENT_OFFSET_INFO, *PEVENT_OFFSET_INFO;

typedef struct
{
    HANDLE hFile;
    EVENTLOGHEADER Header;
    WCHAR *LogName;
    WCHAR *FileName;
    CRITICAL_SECTION cs;
    PEVENT_OFFSET_INFO OffsetInfo;
    ULONG OffsetInfoSize;
    ULONG OffsetInfoNext;
    LIST_ENTRY ListEntry;
} LOGFILE, *PLOGFILE;

typedef struct
{
    PLOGFILE LogFile;
    WCHAR *Name;
} EVENTSOURCE, *PEVENTSOURCE;

/* file.c */
VOID LogfListInitialize(VOID);

PLOGFILE LogfListHead(VOID);

INT LogfListItemCount(VOID);

PLOGFILE LogfListItemByIndex(INT Index);

PLOGFILE LogfListItemByName(WCHAR * Name);

INT LogfListItemIndexByName(WCHAR * Name);

VOID LogfListAddItem(PLOGFILE Item);

VOID LogfListRemoveItem(PLOGFILE Item);

BOOL LogfReadEvent(PLOGFILE LogFile,
                   DWORD Flags,
                   DWORD RecordNumber,
                   DWORD BufSize,
                   PBYTE Buffer,
                   DWORD * BytesRead,
                   DWORD * BytesNeeded);

BOOL LogfWriteData(PLOGFILE LogFile,
                   DWORD BufSize,
                   PBYTE Buffer);

PLOGFILE LogfCreate(WCHAR * LogName,
                    WCHAR * FileName);

VOID LogfClose(PLOGFILE LogFile);

VOID LogfCloseAll(VOID);

BOOL LogfInitializeNew(PLOGFILE LogFile);

BOOL LogfInitializeExisting(PLOGFILE LogFile);

DWORD LogfGetOldestRecord(PLOGFILE LogFile);

ULONG LogfOffsetByNumber(PLOGFILE LogFile,
                         DWORD RecordNumber);

BOOL LogfAddOffsetInformation(PLOGFILE LogFile,
                              ULONG ulNumber,
                              ULONG ulOffset);

PBYTE LogfAllocAndBuildNewRecord(LPDWORD lpRecSize,
                                 DWORD dwRecordNumber,
                                 WORD wType,
                                 WORD wCategory,
                                 DWORD dwEventId,
                                 LPCWSTR SourceName,
                                 LPCWSTR ComputerName,
                                 DWORD dwSidLength,
                                 PSID lpUserSid,
                                 WORD wNumStrings,
                                 WCHAR * lpStrings,
                                 DWORD dwDataSize,
                                 LPVOID lpRawData);

/* eventlog.c */
extern HANDLE MyHeap;

VOID PRINT_HEADER(PEVENTLOGHEADER header);

VOID PRINT_RECORD(PEVENTLOGRECORD pRec);

VOID EventTimeToSystemTime(DWORD EventTime,
                           SYSTEMTIME * SystemTime);

VOID SystemTimeToEventTime(SYSTEMTIME * pSystemTime,
                           DWORD * pEventTime);

/* logport.c */
NTSTATUS WINAPI PortThreadRoutine(PVOID Param);

NTSTATUS InitLogPort(VOID);

NTSTATUS ProcessPortMessage(VOID);

/* rpc.c */
DWORD WINAPI RpcThreadRoutine(LPVOID lpParameter);

static __inline void LogfFreeRecord(LPVOID Rec)
{
    HeapFree(MyHeap, 0, Rec);
}

#endif  /* __EVENTLOG_H__ */
