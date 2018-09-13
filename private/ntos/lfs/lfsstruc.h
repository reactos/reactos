/*++ BUILD Version: 0000    // Increment this if a change has global effects

Copyright (c) 1989  Microsoft Corporation

Module Name:

    LfsStruc.h

Abstract:

    This module defines the data structures that make up the major internal
    part of the Log File Service.

Author:

    Brian Andrew    [BrianAn]   13-June-1991

Revision History:

--*/

#ifndef _LFSSTRUC_
#define _LFSSTRUC_

typedef PVOID PBCB;     //**** Bcb's are now part of the cache module


//
//  Log Context Block.  A pointer to this structure is returned to the user
//  when a client is reading a particular set of log records from the log
//  file.
//

typedef struct _LCB {

    //
    //  The type and size of this record (must be LFS_NTC_LCB)
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  Log record header.  This is the mapped log record header and bcb
    //  for the record header of the current Lsn.
    //

    struct _LFS_RECORD_HEADER *RecordHeader;
    PBCB RecordHeaderBcb;

    //
    //  Context Mode.  This is the mode governing the log record lookup.  We
    //  can look backwards via the ClientUndoNextLsn or ClientPreviousLsn.
    //  We can also look forwards by walking through all the log records and
    //  comparing ClientId fields.
    //

    LFS_CONTEXT_MODE ContextMode;

    //
    //  Client Id.  This is the client ID for the log records being returned.
    //

    LFS_CLIENT_ID ClientId;

    //
    //  Log record pointer.  This is the address returned to the user as the
    //  log record referred to by CurrentLsn.  If we allocated a buffer to
    //  hold the record, we need to deallocate it as necessary.
    //
    //  This field is either the actual mapped log record or a pointer to
    //  an auxilary buffer allocated by the Lfs.
    //

    PVOID CurrentLogRecord;
    BOOLEAN AuxilaryBuffer;

} LCB, *PLCB;


//
//  Lfcb synchronization.  This is the synchronization structure used by the Lfcb.
//

typedef struct _LFCB_SYNC {

    //
    //  Principal Lfcb Resource.
    //

    ERESOURCE Resource;

    //
    //  Notification Event.  This event is set to the Signalled state when
    //  pages are flushed to the cache file.  Any waiters will then check
    //  to see if the Lsn they're waiting for made it to disk.
    //

    KEVENT Event;

    //
    //  User Count.  Number of clients using this structure.  We will deallocate
    //  when all clients are gone.
    //

    ULONG UserCount;

    //
    //  Mutant to guard Lcb spare list
    //

    FAST_MUTEX SpareListMutex; 

} LFCB_SYNC, *PLFCB_SYNC;


//
//  Log Client Structure.  The Lfs allocates one of these for each active
//  client.  The address of this structure will be returned to the user
//  as a log handle.
//

typedef struct _LCH {

    //
    //  The type and size of this record (must be LFS_NTC_LCH)
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  Links for all the client handles on an Lfcb.
    //

    LIST_ENTRY LchLinks;

    //
    //  Log File Control Block.  This is the log file for this log handle.
    //

    struct _LFCB *Lfcb;

    //
    //  Client Id.  This refers to the client record for this client in the
    //  Lfs restart area.
    //

    LFS_CLIENT_ID ClientId;

    //
    //  The following is the number of bytes this client has asked to
    //  have reserved in the log file.  It includes the space
    //  for the log record headers.
    //

    LONGLONG ClientUndoCommitment;

    //
    //  Byte offset in the client array.
    //

    ULONG ClientArrayByteOffset;

    //
    //  Pointer to the resource in the Lfcb.  We access the resource with
    //  this pointer for the times when the lfcb has been deleted.
    //

    PLFCB_SYNC Sync;

} LCH, *PLCH;


//
//  Log Buffer Control Block.  A buffer control block is associated with
//  each of the log buffers.  They are used to serialize access to the
//  log file.
//

typedef struct _LBCB {

    //
    //  The type and size of this record (must be LFS_NTC_LBCB)
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  Buffer Block Links.  These fields are used to link the buffer blocks
    //  together.
    //

    LIST_ENTRY WorkqueLinks;
    LIST_ENTRY ActiveLinks;

    //
    //  Log file position and length.  This is the location in the log file to write
    //  out this buffer.
    //

    LONGLONG FileOffset;
    LONGLONG Length;

    //
    //  Sequence number.  This is the sequence number for log records which
    //  begin on this page.
    //

    LONGLONG SeqNumber;

    //
    //  Next Offset.  This is the next offset to write a log record in the
    //  this log page.  Stored as a large integer to facilitate large
    //  integer operations.
    //

    LONGLONG BufferOffset;

    //
    //  Buffer.  This field points to the buffer containing the log page
    //  for this block.  For a log record page this is a pointer to
    //  a pinned cache buffer, for a log restart page, this is a pointer
    //  to an auxilary buffer.
    //

    PVOID PageHeader;

    //
    //  Bcb for Log Page Block.  This is the Bcb for the pinned data.
    //  If this buffer block describes an Lfs restart area, this field is NULL.
    //

    PBCB LogPageBcb;

    //
    //  Last Lsn.  This is the Lsn for the last log record on this page.  We delay
    //  writing it until the page is flushed, storing it here instead.
    //

    LSN LastLsn;

    //
    //  Last complete Lsn.  This is the Lsn for the last log record which ends
    //  on this page.
    //

    LSN LastEndLsn;

    //
    //  Page Flags.  These are the flags associated with this log page.
    //  We store them in the Lbcb until the page is written.  They flags
    //  to use are the same as in the log record page header.
    //
    //      LOG_PAGE_LOG_RECORD_END     -   Page contains the end of a log record
    //      LOG_PAGE_PACKED             -   Page contains packed log records
    //      LOG_PAGE_TAIL_COPY          -   Page is a copy of the log file end
    //

    ULONG Flags;

    //
    //  Lbcb flags.  These are flags used to describe this Lbcb.
    //
    //      LBCB_LOG_WRAPPED            -   Lbcb has wrapped the log file
    //      LBCB_ON_ACTIVE_QUEUE        -   Lbcb is on the active queue
    //      LBCB_NOT_EMPTY              -   Page has existing log record
    //      LBCB_FLUSH_COPY             -   Write copy of this page first
    //      LBCB_RESTART_LBCB           -   This Lbcb contains a restart page
    //

    ULONG LbcbFlags;

    //
    //  This is the thread which has locked the log page.
    //

    ERESOURCE_THREAD ResourceThread;

} LBCB, *PLBCB;

#define LBCB_LOG_WRAPPED                        (0x00000001)
#define LBCB_ON_ACTIVE_QUEUE                    (0x00000002)
#define LBCB_NOT_EMPTY                          (0x00000004)
#define LBCB_FLUSH_COPY                         (0x00000008)
#define LBCB_RESTART_LBCB                       (0x00000020)


//
//  Log file data.  This data structure is used on a per-log file basis.
//

typedef enum _LFS_IO_STATE {

    LfsNoIoInProgress = 0,
    LfsClientThreadIo

} LFS_IO_STATE;

typedef struct _LFCB {

    //
    //  The type and size of this record (must be LFS_NTC_LFCB)
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  Lfcb Links.  The following links the file control blocks to the
    //  global data structure.
    //

    LIST_ENTRY LfcbLinks;

    //
    //  Lch Links.  The following links all of the handles for the Lfcb.
    //

    LIST_ENTRY LchLinks;

    //
    //
    //  File Object.  This is the file object for the log file.
    //

    PFILE_OBJECT FileObject;

    //
    //  Log File Size.  This is the size of the log file.
    //  The second value is the size proposed by this open.
    //

    LONGLONG FileSize;

    //
    //  System page size and masks.
    //

    LONGLONG SystemPageSize;
    ULONG SystemPageMask;
    LONG SystemPageInverseMask;

    //
    //  Log page size, masks and shift count to do multiplication and division
    //  of log pages.
    //

    LONGLONG  LogPageSize;
    ULONG LogPageMask;
    LONG LogPageInverseMask;
    ULONG LogPageShift;

    //
    //  First log page.  This is the offset in the file of the first
    //  log page with log records.
    //

    LONGLONG FirstLogPage;

    //
    //  Next log page offset.  This is the offset of the next log page to use.
    //  If we are reusing this page we store the offset to begin with.
    //

    LONGLONG NextLogPage;
    ULONG ReusePageOffset;

    //
    //  Data Offset.  This is the offset within a log page of the data that
    //  appears on that page.  This will be the actual restart data for
    //  an Lfs restart page, or the beginning of log record data for a log
    //  record page.
    //

    ULONG RestartDataOffset;
    LONGLONG LogPageDataOffset;

    //
    //  Data Size.  This is the amount of data that may be stored on a
    //  log page.  It is included here because it is frequently used.  It
    //  is simply the log page size minus the data offset.
    //

    ULONG RestartDataSize;
    LONGLONG LogPageDataSize;

    //
    //  Record header size.  This is the size to use for the record headers
    //  when reading the log file.
    //

    USHORT RecordHeaderLength;

    //
    //  Sequence number.  This is the number of times we have cycled through
    //  the log file.  The wrap sequence number is used to confirm that we
    //  have gone through the entire file at least once.  When we write a
    //  log record page for an Lsn with this sequence number, then we have
    //  cycled through the file.
    //

    LONGLONG SeqNumber;
    LONGLONG SeqNumberForWrap;
    ULONG SeqNumberBits;
    ULONG FileDataBits;

    //
    //  Buffer Block Links.  The following links the buffer blocks for this
    //  log file.
    //

    LIST_ENTRY LbcbWorkque;
    LIST_ENTRY LbcbActive;

    PLBCB ActiveTail;
    PLBCB PrevTail;

    //
    //  The enumerated type indicates if there is an active write for
    //  this log file and whether it is being done by an Lfs or
    //  client thread.
    //

    LFS_IO_STATE LfsIoState;

    //
    //  Current Restart Area.  The following is the in-memory image of the
    //  next restart area.  We also store a pointer to the client data
    //  array in the restart area.  The client array offset is from the start of
    //  the restart area.
    //

    PLFS_RESTART_AREA RestartArea;
    PLFS_CLIENT_RECORD ClientArray;
    USHORT ClientArrayOffset;
    USHORT ClientNameOffset;

    //
    //  Restart Area size.  This is the usable size of the restart area.
    //

    ULONG RestartAreaSize;
    USHORT LogClients;

    //
    //  Initial Restart area.  If true, then the in-memory restart area is to
    //  be written to the first position on the disk.
    //

    BOOLEAN InitialRestartArea;

    //
    //  The following pseudo Lsn's are used to track when restart areas
    //  are flushed to the disk.
    //

    LSN NextRestartLsn;
    LSN LastFlushedRestartLsn;

    //
    //  The following is the earliest Lsn we will guarantee is still in the
    //  log file.
    //

    LSN OldestLsn;

    //
    //  The following is the file offset of the oldest Lsn in the system.
    //  We redundantly store it in this form since we will be constantly
    //  checking if a new log record will write over part of the file
    //  we are trying to maintain.
    //

    LONGLONG OldestLsnOffset;

    //
    //  Last Flushed Lsn.  The following is the last Lsn guaranteed to
    //  be flushed to the disk.
    //

    LSN LastFlushedLsn;

    //
    //
    //  The following fields are used to track current usage in the log file.
    //
    //      TotalAvailable - is the total number of bytes available for
    //          log records.  It is the number of log pages times the
    //          data size of each page.
    //
    //      TotalAvailInPages - is the total number of bytes in the log
    //          pages for log records.  This is TotalAvailable without
    //          subtracting the size of the page headers.
    //
    //      TotalUndoCommitment - is the number of bytes reserved for
    //          possible abort operations.  This includes space for
    //          log record headers as well.
    //
    //      MaxCurrentAvail - is the maximum available in all pages
    //          subtracting the page header and any reserved tail.
    //
    //      CurrentAvailable - is the total number of bytes available in
    //          unused pages in the log file.
    //
    //      ReservedLogPageSize - is the number of bytes on a page available
    //          for reservation.
    //

    LONGLONG TotalAvailable;
    LONGLONG TotalAvailInPages;
    LONGLONG TotalUndoCommitment;
    LONGLONG MaxCurrentAvail;
    LONGLONG CurrentAvailable;

    LONGLONG ReservedLogPageSize;

    //
    //  The following fields are used to store information about the
    //  update sequence arrays.
    //

    USHORT RestartUsaOffset;
    USHORT RestartUsaArraySize;

    USHORT LogRecordUsaOffset;
    USHORT LogRecordUsaArraySize;

    //
    //  Major and minor version numbers.
    //

    SHORT MajorVersion;
    SHORT MinorVersion;

    //
    //  Log File Flags.
    //
    //      LFCB_LOG_WRAPPED        -   We found an Lbcb which wraps the log file
    //      LFCB_MULTIPLE_PAGE_IO   -   Write multiple pages if possible
    //      LFCB_NO_LAST_LSN        -   There are no log records to return
    //      LFCB_PACK_LOG           -   Pack the records into the pages
    //      LFCB_REUSE_TAIL         -   We will be reusing the tail of the log file after restart
    //      LFCB_NO_OLDEST_LSN      -   There is no oldest page being reserved
    //

    ULONG Flags;

    //
    //  The following are the spare Lbcb's for the volume and a field with
    //  the count for these.
    //

    ULONG SpareLbcbCount;
    LIST_ENTRY SpareLbcbList;

    //
    //  The following are sparse LCB's to be used rather than having to allocate
    //  then when reading log records
    //

    ULONG SpareLcbCount;
    LIST_ENTRY SpareLcbList;

    //
    //  The following structure synchronizes access to this structure.
    //

    PLFCB_SYNC Sync;

    //
    //  Count of waiters wanting access to flush the Lfcb.
    //

    ULONG Waiters;

    //
    //  On-disk value for OpenLogCount.  This is the value we will stuff into
    //  the client handles.
    //

    ULONG CurrentOpenLogCount;

    //
    //  Maintain the flush range for this file.
    //

    PLFS_WRITE_DATA UserWriteData;

    PLBCB PageToDirty;

#ifdef BRIANDBG
    ERESOURCE_THREAD LfsIoThread;
#endif

} LFCB, *PLFCB;

#define LFCB_LOG_WRAPPED                (0x00000001)
#define LFCB_MULTIPLE_PAGE_IO           (0x00000002)
#define LFCB_NO_LAST_LSN                (0x00000004)
#define LFCB_PACK_LOG                   (0x00000008)
#define LFCB_REUSE_TAIL                 (0x00000010)
#define LFCB_NO_OLDEST_LSN              (0x00000020)
#define LFCB_LOG_FILE_CORRUPT           (0x00000040)
#define LFCB_FINAL_SHUTDOWN             (0x00000080)
#define LFCB_READ_FIRST_RESTART         (0x00000100)
#define LFCB_READ_SECOND_RESTART        (0x00000200)

#define LFCB_RESERVE_LBCB_COUNT         (5)
#define LFCB_MAX_LBCB_COUNT             (25)

#define LFCB_RESERVE_LCB_COUNT          (5)
#define LFCB_MAX_LCB_COUNT              (25)


//
//  Global Log Data.  The following structure has only one instance and
//  maintains global information for the entire logging service.
//

typedef struct _LFS_DATA {

    //
    //  The type and size of this record (must be LFS_NTC_DATA)
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  The following field links all of the Log File Control Blocks for
    //  the logging system.
    //

    LIST_ENTRY LfcbLinks;

    //
    //  Flag field.
    //

    ULONG Flags;

    //
    //  The following mutex controls access to this structure.
    //

    FAST_MUTEX LfsDataLock;

    //
    //  Allocated buffers for reading spanning log records in low memory case.
    //  Flags indicate which buffers owned.
    //      LFS_BUFFER1_OWNED
    //      LFS_BUFFER2_OWNED
    //

    PVOID Buffer1;
    PVOID Buffer2;
    ERESOURCE_THREAD BufferOwner;
    ULONG BufferFlags;

    FAST_MUTEX BufferLock;
    KEVENT BufferNotification;

} LFS_DATA, *PLFS_DATA;

#define LFS_DATA_INIT_FAILED                (0x00000001)
#define LFS_DATA_INITIALIZED                (0x00000002)

#define LFS_BUFFER1_OWNED                   (0x00000001)
#define LFS_BUFFER2_OWNED                   (0x00000002)

#define LFS_BUFFER_SIZE                     (0x10000)
#endif // _LFSSTRUC_

