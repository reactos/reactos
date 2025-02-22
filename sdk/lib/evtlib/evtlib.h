/*
 * PROJECT:         ReactOS EventLog File Library
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            sdk/lib/evtlib/evtlib.h
 * PURPOSE:         Provides functionality for reading and writing
 *                  EventLog files in the NT <= 5.2 (.evt) format.
 * PROGRAMMERS:     Copyright 2005 Saveliy Tretiakov
 *                  Michael Martin
 *                  Hermes Belusca-Maito
 */

#ifndef __EVTLIB_H__
#define __EVTLIB_H__

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* PSDK/NDK Headers */
// #define WIN32_NO_STATUS
// #include <windef.h>
// #include <winbase.h>
// #include <winnt.h>

#define NTOS_MODE_USER
#include <ndk/rtlfuncs.h>

#ifndef ROUND_DOWN
#define ROUND_DOWN(n, align) (((ULONG)n) & ~((align) - 1l))
#endif

#ifndef ROUND_UP
#define ROUND_UP(n, align) ROUND_DOWN(((ULONG)n) + (align) - 1, (align))
#endif

/*
 * Our file format will be compatible with NT's
 */
#define MAJORVER    1
#define MINORVER    1
#define LOGFILE_SIGNATURE   0x654c664c  // "LfLe"

/*
 * Flags used in the logfile header
 */
#define ELF_LOGFILE_HEADER_DIRTY    1
#define ELF_LOGFILE_HEADER_WRAP     2
#define ELF_LOGFILE_LOGFULL_WRITTEN 4
#define ELF_LOGFILE_ARCHIVE_SET     8

/*
 * On-disk event log structures (log file header, event record and EOF record).
 * NOTE: Contrary to what MSDN claims, both the EVENTLOGHEADER and EVENTLOGEOF
 * structures are absent from winnt.h .
 */

#include <pshpack4.h> // pshpack1

// ELF_LOGFILE_HEADER
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


/* Those flags and structure are defined in winnt.h */
#ifndef _WINNT_

/* EventType flags */
#define EVENTLOG_SUCCESS            0
#define EVENTLOG_ERROR_TYPE         1
#define EVENTLOG_WARNING_TYPE       2
#define EVENTLOG_INFORMATION_TYPE   4
#define EVENTLOG_AUDIT_SUCCESS      8
#define EVENTLOG_AUDIT_FAILURE      16

typedef struct _EVENTLOGRECORD
{
    ULONG  Length;              /* Length of full record, including the data portion */
    ULONG  Reserved;
    ULONG  RecordNumber;
    ULONG  TimeGenerated;
    ULONG  TimeWritten;
    ULONG  EventID;
    USHORT EventType;
    USHORT NumStrings;          /* Number of strings in the 'Strings' array */
    USHORT EventCategory;
    USHORT ReservedFlags;
    ULONG  ClosingRecordNumber;
    ULONG  StringOffset;
    ULONG  UserSidLength;
    ULONG  UserSidOffset;
    ULONG  DataLength;          /* Length of the data portion */
    ULONG  DataOffset;          /* Offset from beginning of record */
/*
 * Length-varying data:
 *
 *  WCHAR SourceName[];
 *  WCHAR ComputerName[];
 *  SID   UserSid;              // Must be aligned on a DWORD boundary
 *  WCHAR Strings[];
 *  BYTE  Data[];
 *  CHAR  Pad[];                // Padding for DWORD boundary
 *  ULONG Length;               // Same as the first 'Length' member at the beginning
 */
} EVENTLOGRECORD, *PEVENTLOGRECORD;

#endif // _WINNT_


// ELF_EOF_RECORD
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

#include <poppack.h>


typedef struct _EVENT_OFFSET_INFO
{
    ULONG EventNumber;
    ULONG EventOffset;
} EVENT_OFFSET_INFO, *PEVENT_OFFSET_INFO;

#define TAG_ELF     ' flE'
#define TAG_ELF_BUF 'BflE'

struct _EVTLOGFILE;

typedef PVOID
(NTAPI *PELF_ALLOCATE_ROUTINE)(
    IN SIZE_T Size,
    IN ULONG Flags,
    IN ULONG Tag
);

typedef VOID
(NTAPI *PELF_FREE_ROUTINE)(
    IN PVOID Ptr,
    IN ULONG Flags,
    IN ULONG Tag
);

typedef NTSTATUS
(NTAPI *PELF_FILE_READ_ROUTINE)(
    IN  struct _EVTLOGFILE* LogFile,
    IN  PLARGE_INTEGER FileOffset,
    OUT PVOID   Buffer,
    IN  SIZE_T  Length,
    OUT PSIZE_T ReadLength OPTIONAL
);

typedef NTSTATUS
(NTAPI *PELF_FILE_WRITE_ROUTINE)(
    IN  struct _EVTLOGFILE* LogFile,
    IN  PLARGE_INTEGER FileOffset,
    IN  PVOID   Buffer,
    IN  SIZE_T  Length,
    OUT PSIZE_T WrittenLength OPTIONAL
);

typedef NTSTATUS
(NTAPI *PELF_FILE_SET_SIZE_ROUTINE)(
    IN struct _EVTLOGFILE* LogFile,
    IN ULONG FileSize,
    IN ULONG OldFileSize
);

typedef NTSTATUS
(NTAPI *PELF_FILE_FLUSH_ROUTINE)(
    IN struct _EVTLOGFILE* LogFile,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length
);

typedef struct _EVTLOGFILE
{
    PELF_ALLOCATE_ROUTINE   Allocate;
    PELF_FREE_ROUTINE       Free;
    PELF_FILE_SET_SIZE_ROUTINE FileSetSize;
    PELF_FILE_WRITE_ROUTINE FileWrite;
    PELF_FILE_READ_ROUTINE  FileRead;
    PELF_FILE_FLUSH_ROUTINE FileFlush;

    EVENTLOGHEADER Header;
    ULONG CurrentSize;  /* Equivalent to the file size, is <= MaxSize and can be extended to MaxSize if needed */
    UNICODE_STRING FileName;
    PEVENT_OFFSET_INFO OffsetInfo;
    ULONG OffsetInfoSize;
    ULONG OffsetInfoNext;
    BOOLEAN ReadOnly;
} EVTLOGFILE, *PEVTLOGFILE;


NTSTATUS
NTAPI
ElfCreateFile(
    IN OUT PEVTLOGFILE LogFile,
    IN PUNICODE_STRING FileName OPTIONAL,
    IN ULONG    FileSize,
    IN ULONG    MaxSize,
    IN ULONG    Retention,
    IN BOOLEAN  CreateNew,
    IN BOOLEAN  ReadOnly,
    IN PELF_ALLOCATE_ROUTINE   Allocate,
    IN PELF_FREE_ROUTINE       Free,
    IN PELF_FILE_SET_SIZE_ROUTINE FileSetSize,
    IN PELF_FILE_WRITE_ROUTINE FileWrite,
    IN PELF_FILE_READ_ROUTINE  FileRead,
    IN PELF_FILE_FLUSH_ROUTINE FileFlush); // What about Seek ??

NTSTATUS
NTAPI
ElfReCreateFile(
    IN PEVTLOGFILE LogFile);

// NTSTATUS
// ElfClearFile(PEVTLOGFILE LogFile);

NTSTATUS
NTAPI
ElfBackupFile(
    IN PEVTLOGFILE LogFile,
    IN PEVTLOGFILE BackupLogFile);

NTSTATUS
NTAPI
ElfFlushFile(
    IN PEVTLOGFILE LogFile);

VOID
NTAPI
ElfCloseFile(  // ElfFree
    IN PEVTLOGFILE LogFile);

NTSTATUS
NTAPI
ElfReadRecord(
    IN  PEVTLOGFILE LogFile,
    IN  ULONG RecordNumber,
    OUT PEVENTLOGRECORD Record,
    IN  SIZE_T  BufSize, // Length
    OUT PSIZE_T BytesRead OPTIONAL,
    OUT PSIZE_T BytesNeeded OPTIONAL);

NTSTATUS
NTAPI
ElfWriteRecord(
    IN PEVTLOGFILE LogFile,
    IN PEVENTLOGRECORD Record,
    IN SIZE_T BufSize);

ULONG
NTAPI
ElfGetOldestRecord(
    IN PEVTLOGFILE LogFile);

ULONG
NTAPI
ElfGetCurrentRecord(
    IN PEVTLOGFILE LogFile);

ULONG
NTAPI
ElfGetFlags(
    IN PEVTLOGFILE LogFile);

#if DBG
VOID PRINT_HEADER(PEVENTLOGHEADER Header);
#endif

#ifdef __cplusplus
}
#endif
#endif  /* __EVTLIB_H__ */
