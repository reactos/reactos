/*
 * PROJECT:          ReactOS kernel
 * LICENSE:          GPL - See COPYING in the top level directory
 * FILE:             base/services/eventlog/eventlog.h
 * PURPOSE:          Event logging service
 * COPYRIGHT:        Copyright 2005 Saveliy Tretiakov
 */

#ifndef __EVENTLOG_H__
#define __EVENTLOG_H__

#include <stdarg.h>

#define WIN32_NO_STATUS

#include <windef.h>
#include <winbase.h>
#include <ndk/rtlfuncs.h>
#include <ndk/obfuncs.h>
#include <eventlogrpc_s.h>
#include <strsafe.h>

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
#define ELF_LOGFILE_LOGFULL_WRITTEN 4
#define ELF_LOGFILE_ARCHIVE_SET 8

/* FIXME: MSDN reads that the following two structs are in winnt.h. Are they? */
typedef struct _EVENTLOGHEADER
{
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

typedef struct _EVENTLOGEOF
{
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

typedef struct _EVENT_OFFSET_INFO
{
    ULONG EventNumber;
    ULONG EventOffset;
} EVENT_OFFSET_INFO, *PEVENT_OFFSET_INFO;

typedef struct _LOGFILE
{
    HANDLE hFile;
    EVENTLOGHEADER Header;
    WCHAR *LogName;
    WCHAR *FileName;
    RTL_RESOURCE Lock;
    PEVENT_OFFSET_INFO OffsetInfo;
    ULONG OffsetInfoSize;
    ULONG OffsetInfoNext;
    BOOL Permanent;
    LIST_ENTRY ListEntry;
} LOGFILE, *PLOGFILE;

typedef struct _EVENTSOURCE
{
    LIST_ENTRY EventSourceListEntry;
    PLOGFILE LogFile;
    WCHAR szName[1];
} EVENTSOURCE, *PEVENTSOURCE;


/* Log Handle Flags */
#define LOG_HANDLE_BACKUP_FILE 1

typedef struct _LOGHANDLE
{
    LIST_ENTRY LogHandleListEntry;
    PEVENTSOURCE EventSource;
    PLOGFILE LogFile;
    ULONG CurrentRecord;
    ULONG Flags;
    WCHAR szName[1];
} LOGHANDLE, *PLOGHANDLE;

/* file.c */
VOID LogfListInitialize(VOID);

PLOGFILE LogfListHead(VOID);

INT LogfListItemCount(VOID);

PLOGFILE LogfListItemByIndex(INT Index);

PLOGFILE LogfListItemByName(WCHAR * Name);

INT LogfListItemIndexByName(WCHAR * Name);

VOID LogfListAddItem(PLOGFILE Item);

VOID LogfListRemoveItem(PLOGFILE Item);

DWORD LogfReadEvent(PLOGFILE LogFile,
                   DWORD Flags,
                   DWORD * RecordNumber,
                   DWORD BufSize,
                   PBYTE Buffer,
                   DWORD * BytesRead,
                   DWORD * BytesNeeded,
                   BOOL Ansi);

BOOL LogfWriteData(PLOGFILE LogFile,
                   DWORD BufSize,
                   PBYTE Buffer);

NTSTATUS
LogfClearFile(PLOGFILE LogFile,
              PUNICODE_STRING BackupFileName);

NTSTATUS
LogfBackupFile(PLOGFILE LogFile,
               PUNICODE_STRING BackupFileName);

NTSTATUS
LogfCreate(PLOGFILE *Logfile,
           WCHAR * LogName,
           PUNICODE_STRING FileName,
           ULONG ulMaxSize,
           ULONG ulRetention,
           BOOL Permanent,
           BOOL Backup);

VOID
LogfClose(PLOGFILE LogFile,
          BOOL ForceClose);

VOID LogfCloseAll(VOID);

DWORD LogfGetOldestRecord(PLOGFILE LogFile);

DWORD LogfGetCurrentRecord(PLOGFILE LogFile);

ULONG LogfOffsetByNumber(PLOGFILE LogFile,
                         DWORD RecordNumber);

BOOL LogfAddOffsetInformation(PLOGFILE LogFile,
                              ULONG ulNumber,
                              ULONG ulOffset);

BOOL LogfDeleteOffsetInformation(PLOGFILE LogFile,
                              ULONG ulNumber);

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

VOID
LogfReportEvent(WORD wType,
                WORD wCategory,
                DWORD dwEventId,
                WORD wNumStrings,
                WCHAR *lpStrings,
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

/* eventsource.c */
VOID InitEventSourceList(VOID);

BOOL
LoadEventSources(HKEY hKey,
                 PLOGFILE pLogFile);

PEVENTSOURCE
GetEventSourceByName(LPCWSTR Name);


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
