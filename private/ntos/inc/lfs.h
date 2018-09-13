/*++ BUILD Version: 0000    // Increment this if a change has global effects

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Lfs.h

Abstract:

    This module contains the public data structures and procedure
    prototypes for the Log File Service.

Author:
    Brian Andrew    [BrianAn]   20-June-1991


Revision History:

--*/

#ifndef _LFS_
#define _LFS_

//
// The Multi-Sector Header and Update Sequence Array provide detection of
// incomplete multi-sector transfers for devices which either have a
// physical sector size equal to the Sequence Number Stride or greater, or
// which do not transfer sectors out of order.  If a device exists which has
// a sector size smaller than the Sequence Number Stride *and* it sometimes
// transfers sectors out of order, then the Update Sequence Array will not
// provide absolute detection of incomplete transfers.  The Sequence Number
// Stride is set to a small enough number to provide absolute protection for
// all known hard disks.  It is not set any smaller, in order to avoid
// excessive run time and space overhead.
//
// The Multi-Sector Header contains space for a four-byte signature for the
// convenience of its user.  It then provides the offset to and length of the
// the Update Sequence Array.  The Update Sequence Array consists of an array
// of n saved USHORTs, where n is the size of the structure being protected
// divided by the sequence number stride.  (The size of structure being
// protected must be a nonzero power of 2 times the Sequence Number Stride,
// and less than or equal to the physical page size of the machine.)  The
// first word of the Update Sequence Array contains the Update Sequence Number,
// which is a cyclical counter (however 0 is not used) of the number of times
// the containing structure has been written to disk.  Following the Update
// Sequence Number are the n saved USHORTs which were overwritten by the
// Update Sequence Number the last time the containing structure was
// written to disk.
//
// In detail, just prior to each time the protected structure is written to
// disk, the last word in each Sequence Number Stride is saved to its
// respective position in the Sequence Number Array, and then it is overwritten
// with the next Update Sequence Number.  Just after this write, or whenever
// reading the structure, the saved word from the Sequence Number Array is
// restored to its actual position in the structure.  Before restoring the
// saved words on reads, all of the sequence numbers at the end of each
// stride are compared with the actual sequence number at the start of the
// array.  If any of these compares come up not equal, then a failed
// multi-sector transfer has been detected.
//
// The size of the array is determined by the size of the containing structure.
// As a C detail, the array is declared here with a size of 1, since its
// actual size can only be determined at runtime.
//
// The Update Sequence Array should be included at the end of the header of
// the structure it is protecting, since it is variable size.  Its user must
// insure that the correct size is reserved for it, namely:
//
//      (sizeof-structure / SEQUENCE_NUMBER_STRIDE + 1) * sizeof(USHORT)
//

#define SEQUENCE_NUMBER_STRIDE           (512)

typedef USHORT UPDATE_SEQUENCE_NUMBER, *PUPDATE_SEQUENCE_NUMBER;

//
// This structure must be allocated at the start of the structure being
// protected.
//

#if !defined( _AUTOCHECK_ )

typedef struct _MULTI_SECTOR_HEADER {

    //
    // Space for a four-character signature
    //

    UCHAR Signature[4];

    //
    // Offset to Update Sequence Array, from start of structure.  The Update
    // Sequence Array must end before the last USHORT in the first "sector"
    // of size SEQUENCE_NUMBER_STRIDE.  (I.e., with the current constants,
    // the sum of the next two fields must be <= 510.)
    //

    USHORT UpdateSequenceArrayOffset;

    //
    // Size of Update Sequence Array (from above formula)
    //

    USHORT UpdateSequenceArraySize;

} MULTI_SECTOR_HEADER, *PMULTI_SECTOR_HEADER;

#endif

//
// This array must be present at the offset described above.
//

typedef UPDATE_SEQUENCE_NUMBER UPDATE_SEQUENCE_ARRAY[1];

typedef UPDATE_SEQUENCE_ARRAY *PUPDATE_SEQUENCE_ARRAY;

//
//  The following structure is allocated in the file system's Vcb and
//  its address is passed to Lfs during log file initialization.  It
//  contains the offset of the current write as well as the system
//  page size being used by Lfs.
//

typedef struct _LFS_WRITE_DATA {

    LONGLONG FileOffset;
    ULONG Length;
    ULONG LfsStructureSize;
    PVOID Lfcb;

} LFS_WRITE_DATA, *PLFS_WRITE_DATA;

//
//  The following structure is used to identify a log record by a log
//  sequence number.
//

typedef LARGE_INTEGER LSN, *PLSN;

//
//  The following Lsn will never occur in a file, it is used to indicate
//  a non-lsn.
//

extern LSN LfsZeroLsn;

//
//  We set the default page size to 4K
//

#define LFS_DEFAULT_LOG_PAGE_SIZE           (0x1000)

//
//  The following type defines the different log record types.
//

typedef enum _LFS_RECORD_TYPE {

    LfsClientRecord = 1,
    LfsClientRestart

} LFS_RECORD_TYPE, *PLFS_RECORD_TYPE;

//
//  The following search modes are supported.
//

typedef enum _LFS_CONTEXT_MODE {

    LfsContextUndoNext = 1,
    LfsContextPrevious,
    LfsContextForward

} LFS_CONTEXT_MODE, *PLFS_CONTEXT_MODE;

typedef ULONG TRANSACTION_ID, *PTRANSACTION_ID;

typedef enum _TRANSACTION_STATE {

    TransactionUninitialized = 0,
    TransactionActive,
    TransactionPrepared,
    TransactionCommitted

} TRANSACTION_STATE, *PTRANSACTION_STATE;

typedef enum _LFS_INFO {

    LfsUseUsa = 1,
    LfsPackLog,
    LfsFixedPageSize

} LFS_INFO, *PLFS_INFO;

typedef PVOID LFS_LOG_HANDLE, *PLFS_LOG_HANDLE;

typedef PVOID LFS_LOG_CONTEXT, *PLFS_LOG_CONTEXT;

//
//  Write Entry for LfsWrite and LfsForceWrite.  The interface to these
//  routines takes a pointer to a Write Entry along with a count of how
//  many Write Entries to expect to describe pieces of the caller's buffer
//  which are supposed to be copied in sequence to the log file.
//

typedef struct _LFS_WRITE_ENTRY {

    PVOID Buffer;
    ULONG ByteLength;

} LFS_WRITE_ENTRY, *PLFS_WRITE_ENTRY;


//
// Global Maintenance routines
//

BOOLEAN
LfsInitializeLogFileService (
    );

//
//  Log File Registration routines
//

typedef struct _LOG_FILE_INFORMATION {

    //
    //  This is the total useable space in the log file after space for
    //  headers and Lfs Restart Areas.
    //

    LONGLONG TotalAvailable;

    //
    //  This is the useable space in the log file from the current position
    //  in the log file to the lowest BaseLsn.  This total as returned is not
    //  yet reduced for undo commitments, returned separately below.
    //

    LONGLONG CurrentAvailable;

    //
    //  This is the total undo commitment for all clients of the log file.
    //  LfsWrite requests are refused when the sum of the write size of the
    //  request plus the UndoRequirement for the request plus the TotalUndoCommitment
    //  are greater than the CurrentAvailable.
    //

    LONGLONG TotalUndoCommitment;

    //
    //  This is the total undo commitment for this client.
    //

    LONGLONG ClientUndoCommitment;

    //
    //  Current system Lsn's.  Includes the Oldest, LastFlushed and current
    //  Lsn.
    //

    LSN OldestLsn;
    LSN LastFlushedLsn;
    LSN LastLsn;

} LOG_FILE_INFORMATION, *PLOG_FILE_INFORMATION;

VOID
LfsInitializeLogFile (
    IN PFILE_OBJECT LogFile,
    IN USHORT MaximumClients,
    IN ULONG LogPageSize OPTIONAL,
    IN LONGLONG FileSize,
    OUT PLFS_WRITE_DATA WriteData
    );

ULONG
LfsOpenLogFile (
    IN PFILE_OBJECT LogFile,
    IN UNICODE_STRING ClientName,
    IN USHORT MaximumClients,
    IN ULONG LogPageSize OPTIONAL,
    IN LONGLONG FileSize,
    IN OUT PLFS_INFO LfsInfo,
    OUT PLFS_LOG_HANDLE LogHandle,
    OUT PLFS_WRITE_DATA WriteData
    );

VOID
LfsCloseLogFile (
    IN LFS_LOG_HANDLE LogHandle
    );

VOID
LfsDeleteLogHandle (
    IN LFS_LOG_HANDLE LogHandle
    );

VOID
LfsReadLogFileInformation (
    IN LFS_LOG_HANDLE LogHandle,
    IN PLOG_FILE_INFORMATION Buffer,
    IN OUT PULONG Length
    );

BOOLEAN
LfsVerifyLogFile (
    IN LFS_LOG_HANDLE LogHandle,
    IN PVOID LogFileHeader,
    IN ULONG Length
    );

//
//  Log File Client Restart routines
//

NTSTATUS
LfsReadRestartArea (
    IN LFS_LOG_HANDLE LogHandle,
    IN OUT PULONG BufferLength,
    IN PVOID Buffer,
    OUT PLSN Lsn
    );

VOID
LfsWriteRestartArea (
    IN LFS_LOG_HANDLE LogHandle,
    IN ULONG BufferLength,
    IN PVOID Buffer,
    OUT PLSN Lsn
    );

VOID
LfsSetBaseLsn (
    IN LFS_LOG_HANDLE LogHandle,
    IN LSN BaseLsn
    );

//
//  If ResetTotal is positive, then NumberRecords and ResetTotal set the absolute
//  values for the client.  If ResetTotal is negative, then they are adjustments
//  to the totals for this client.
//

VOID
LfsResetUndoTotal (
    IN LFS_LOG_HANDLE LogHandle,
    IN ULONG NumberRecords,
    IN LONG ResetTotal
    );

//
//  Log File Write routines
//

BOOLEAN
LfsWrite (
    IN LFS_LOG_HANDLE LogHandle,
    IN ULONG NumberOfWriteEntries,
    IN PLFS_WRITE_ENTRY WriteEntries,
    IN LFS_RECORD_TYPE RecordType,
    IN TRANSACTION_ID *TransactionId OPTIONAL,
    IN LSN UndoNextLsn,
    IN LSN PreviousLsn,
    IN LONG UndoRequirement,
    OUT PLSN Lsn
    );

BOOLEAN
LfsForceWrite (
    IN LFS_LOG_HANDLE LogHandle,
    IN ULONG NumberOfWriteEntries,
    IN PLFS_WRITE_ENTRY WriteEntries,
    IN LFS_RECORD_TYPE RecordType,
    IN TRANSACTION_ID *TransactionId OPTIONAL,
    IN LSN UndoNextLsn,
    IN LSN PreviousLsn,
    IN LONG UndoRequirement,
    OUT PLSN Lsn
    );

VOID
LfsFlushToLsn (
    IN LFS_LOG_HANDLE LogHandle,
    IN LSN Lsn
    );

VOID
LfsCheckWriteRange (
    IN PLFS_WRITE_DATA WriteData,
    IN OUT PLONGLONG FlushOffset,
    IN OUT PULONG FlushLength
    );

//
//  Log File Query Record routines
//

VOID
LfsReadLogRecord (
    IN LFS_LOG_HANDLE LogHandle,
    IN LSN FirstLsn,
    IN LFS_CONTEXT_MODE ContextMode,
    OUT PLFS_LOG_CONTEXT Context,
    OUT PLFS_RECORD_TYPE RecordType,
    OUT TRANSACTION_ID *TransactionId,
    OUT PLSN UndoNextLsn,
    OUT PLSN PreviousLsn,
    OUT PULONG BufferLength,
    OUT PVOID *Buffer
    );

BOOLEAN
LfsReadNextLogRecord (
    IN LFS_LOG_HANDLE LogHandle,
    IN OUT LFS_LOG_CONTEXT Context,
    OUT PLFS_RECORD_TYPE RecordType,
    OUT TRANSACTION_ID *TransactionId,
    OUT PLSN UndoNextLsn,
    OUT PLSN PreviousLsn,
    OUT PLSN Lsn,
    OUT PULONG BufferLength,
    OUT PVOID *Buffer
    );

VOID
LfsTerminateLogQuery (
    IN LFS_LOG_HANDLE LogHandle,
    IN LFS_LOG_CONTEXT Context
    );

LSN
LfsQueryLastLsn (
    IN LFS_LOG_HANDLE LogHandle
    );

#endif  // LFS

