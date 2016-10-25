/*
 * PROJECT:         ReactOS EventLog Service
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/services/eventlog/eventlog.h
 * PURPOSE:         Precompiled Header for the Event logging service
 * COPYRIGHT:       Copyright 2005 Saveliy Tretiakov
 */

#ifndef __EVENTLOG_H__
#define __EVENTLOG_H__

#include <stdarg.h>

/* PSDK/NDK Headers */
#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>

#define NTOS_MODE_USER
#include <ndk/rtlfuncs.h>
#include <ndk/obfuncs.h>

#define ROUND_DOWN(n, align) (((ULONG)n) & ~((align) - 1l))
#define ROUND_UP(n, align) ROUND_DOWN(((ULONG)n) + (align) - 1, (align))

#include <eventlogrpc_s.h>
#include <strsafe.h>

typedef struct _IO_ERROR_LPC
{
    PORT_MESSAGE Header;
    IO_ERROR_LOG_MESSAGE Message;
} IO_ERROR_LPC, *PIO_ERROR_LPC;


/*
 *  Our file format will be compatible with NT's
 */
#define MAJORVER 1
#define MINORVER 1
#define LOGFILE_SIGNATURE 0x654c664c

/*
 *  Flags used in logfile header
 */
#define ELF_LOGFILE_HEADER_DIRTY    1
#define ELF_LOGFILE_HEADER_WRAP     2
#define ELF_LOGFILE_LOGFULL_WRITTEN 4
#define ELF_LOGFILE_ARCHIVE_SET     8

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

#define EVENTLOGEOF_SIZE_FIXED  (5 * sizeof(ULONG))
C_ASSERT(EVENTLOGEOF_SIZE_FIXED == FIELD_OFFSET(EVENTLOGEOF, BeginRecord));

typedef struct _EVENT_OFFSET_INFO
{
    ULONG EventNumber;
    ULONG EventOffset;
} EVENT_OFFSET_INFO, *PEVENT_OFFSET_INFO;

typedef struct _LOGFILE
{
    HANDLE hFile;
    EVENTLOGHEADER Header;
    ULONG CurrentSize;  /* Equivalent to the file size, is <= MaxSize and can be extended to MaxSize if needed */
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


/* eventlog.c */
extern PEVENTSOURCE EventLogSource;

VOID PRINT_HEADER(PEVENTLOGHEADER header);
VOID PRINT_RECORD(PEVENTLOGRECORD pRec);


/* eventsource.c */
VOID InitEventSourceList(VOID);

BOOL
LoadEventSources(HKEY hKey,
                 PLOGFILE pLogFile);

PEVENTSOURCE
GetEventSourceByName(LPCWSTR Name);


/* file.c */
VOID LogfListInitialize(VOID);
DWORD LogfListItemCount(VOID);
PLOGFILE LogfListItemByIndex(DWORD Index);
PLOGFILE LogfListItemByName(LPCWSTR Name);
// DWORD LogfListItemIndexByName(WCHAR * Name);



NTSTATUS
LogfReadEvents(PLOGFILE LogFile,
               ULONG    Flags,
               PULONG   RecordNumber,
               ULONG    BufSize,
               PBYTE    Buffer,
               PULONG   BytesRead,
               PULONG   BytesNeeded,
               BOOLEAN  Ansi);

NTSTATUS
LogfWriteRecord(PLOGFILE LogFile,
                ULONG BufSize, // SIZE_T
                PEVENTLOGRECORD Record);

NTSTATUS
LogfClearFile(PLOGFILE LogFile,
              PUNICODE_STRING BackupFileName);

NTSTATUS
LogfBackupFile(PLOGFILE LogFile,
               PUNICODE_STRING BackupFileName);

NTSTATUS
LogfCreate(PLOGFILE* LogFile,
           PCWSTR    LogName,
           PUNICODE_STRING FileName,
           ULONG     ulMaxSize,
           ULONG     ulRetention,
           BOOLEAN   Permanent,
           BOOLEAN   Backup);

VOID
LogfClose(PLOGFILE LogFile,
          BOOLEAN  ForceClose);

VOID LogfCloseAll(VOID);

PEVENTLOGRECORD
LogfAllocAndBuildNewRecord(PSIZE_T pRecSize,
                           ULONG   Time,
                           USHORT  wType,
                           USHORT  wCategory,
                           ULONG   dwEventId,
                           PCWSTR  SourceName,
                           PCWSTR  ComputerName,
                           ULONG   dwSidLength,
                           PSID    pUserSid,
                           USHORT  wNumStrings,
                           PWSTR   pStrings,
                           ULONG   dwDataSize,
                           PVOID   pRawData);

static __inline void LogfFreeRecord(PEVENTLOGRECORD Record)
{
    HeapFree(GetProcessHeap(), 0, Record);
}

VOID
LogfReportEvent(USHORT wType,
                USHORT wCategory,
                ULONG  dwEventId,
                USHORT wNumStrings,
                PWSTR  pStrings,
                ULONG  dwDataSize,
                PVOID  pRawData);


/* logport.c */
NTSTATUS WINAPI PortThreadRoutine(PVOID Param);

NTSTATUS InitLogPort(VOID);

NTSTATUS ProcessPortMessage(VOID);

/* rpc.c */
DWORD WINAPI RpcThreadRoutine(LPVOID lpParameter);

#endif  /* __EVENTLOG_H__ */
