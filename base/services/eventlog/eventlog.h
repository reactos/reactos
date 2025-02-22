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

#include <evtlib.h>

#include <eventlogrpc_s.h>
#include <strsafe.h>

/* Defined in evtlib.h */
// #define LOGFILE_SIGNATURE   0x654c664c  // "LfLe"

typedef struct _LOGFILE
{
    EVTLOGFILE LogFile;
    HANDLE FileHandle;
    WCHAR *LogName;
    RTL_RESOURCE Lock;
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
LogfCreate(PLOGFILE* LogFile,
           PCWSTR    LogName,
           PUNICODE_STRING FileName,
           ULONG     MaxSize,
           ULONG     Retention,
           BOOLEAN   Permanent,
           BOOLEAN   Backup);

VOID
LogfClose(PLOGFILE LogFile,
          BOOLEAN  ForceClose);

VOID LogfCloseAll(VOID);

NTSTATUS
LogfClearFile(PLOGFILE LogFile,
              PUNICODE_STRING BackupFileName);

NTSTATUS
LogfBackupFile(PLOGFILE LogFile,
               PUNICODE_STRING BackupFileName);

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
                PEVENTLOGRECORD Record,
                SIZE_T BufSize);

PEVENTLOGRECORD
LogfAllocAndBuildNewRecord(PSIZE_T pRecSize,
                           ULONG   Time,
                           USHORT  wType,
                           USHORT  wCategory,
                           ULONG   dwEventId,
                           PUNICODE_STRING SourceName,
                           PUNICODE_STRING ComputerName,
                           ULONG   dwSidLength,
                           PSID    pUserSid,
                           USHORT  wNumStrings,
                           PWSTR   pStrings,
                           ULONG   dwDataSize,
                           PVOID   pRawData);

static __inline void LogfFreeRecord(PEVENTLOGRECORD Record)
{
    RtlFreeHeap(GetProcessHeap(), 0, Record);
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
