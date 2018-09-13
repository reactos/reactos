/*++ BUILD Version: 0000    // Increment this if a change has global effects

Copyright (c) 1989  Microsoft Corporation

Module Name:

    LfsDisk.h

Abstract:

    This module defines the on-disk structures present in the log file.

Author:

    Brian Andrew    [BrianAn]   13-June-1991

Revision History:

IMPORTANT NOTE:

    The Log File Service will by used on systems that require that on-disk
    structures guarantee the natural alignment of all arithmetic quantities
    up to and including quad-word (64-bit) numbers.  Therefore, all Lfs
    on-disk structures are quad-word aligned, etc.

--*/

#ifndef _LFSDISK_
#define _LFSDISK_

#define MINIMUM_LFS_PAGES               0x00000030
#define MINIMUM_LFS_CLIENTS             1

//
//  The following macros are used to set and query with respect to the
//  update sequence arrays.
//

#define UpdateSequenceStructureSize( MSH )              \
    ((((PMULTI_SECTOR_HEADER) (MSH))->UpdateSequenceArraySize - 1) * SEQUENCE_NUMBER_STRIDE)

#define UpdateSequenceArraySize( STRUCT_SIZE )          \
    ((STRUCT_SIZE) / SEQUENCE_NUMBER_STRIDE + 1)

#define FIRST_STRIDE                                    \
    (SEQUENCE_NUMBER_STRIDE - sizeof( UPDATE_SEQUENCE_NUMBER ))


//
//  Log client ID.  This is used to uniquely identify a client for a
//  particular log file.
//

typedef struct _LFS_CLIENT_ID {

    USHORT SeqNumber;
    USHORT ClientIndex;

} LFS_CLIENT_ID, *PLFS_CLIENT_ID;


//
//  Log Record Header.  This is the header that begins every Log Record in
//  the log file.
//

typedef struct _LFS_RECORD_HEADER {

    //
    //  Log File Sequence Number of this log record.
    //

    LSN ThisLsn;

    //
    //  The following fields are used to back link Lsn's.  The ClientPrevious
    //  and ClientUndoNextLsn fields are used by a client to link his log
    //  records.
    //

    LSN ClientPreviousLsn;
    LSN ClientUndoNextLsn;

    //
    //  The following field is the size of data area for this record.  The
    //  log record header will be padded if necessary to fill to a 64-bit
    //  boundary, so the client data will begin on a 64-bit boundary to
    //  insure that all of his data is 64-bit aligned.  The below value
    //  has not been padded to 64 bits however.
    //

    ULONG ClientDataLength;

    //
    //  Client ID.  This identifies the owner of this log record.  The owner
    //  is uniquely identified by his offset in the client array and the
    //  sequence number associated with that client record.
    //

    LFS_CLIENT_ID ClientId;

    //
    //  This the Log Record type.  This could be a commit protocol record,
    //  a client restart area or a client update record.
    //

    LFS_RECORD_TYPE RecordType;

    //
    //  Transaction ID.  This is used externally by a client (Transaction
    //  Manager) to group log file entries.
    //

    TRANSACTION_ID TransactionId;

    //
    //  Log record flags.
    //

    USHORT Flags;

    //
    //  Alignment field.
    //

    USHORT AlignWord;

} LFS_RECORD_HEADER, *PLFS_RECORD_HEADER;

#define LOG_RECORD_MULTI_PAGE           (0x0001)

#define LFS_RECORD_HEADER_SIZE          QuadAlign( sizeof( LFS_RECORD_HEADER ))


//
//  Following are the version specific fields in the record page header.
//

typedef struct _LFS_UNPACKED_RECORD_PAGE {

    //
    //  This gives us the offset of the free space in the page.
    //

    USHORT NextRecordOffset;

    USHORT WordAlign;

    //
    //  Reserved.  The following array is reserved for possible future use.
    //

    USHORT Reserved;

    //
    //  Update Sequence Array.  Used to protect the page block.
    //

    UPDATE_SEQUENCE_ARRAY UpdateSequenceArray;

} LFS_UNPACKED_RECORD_PAGE, *PLFS_UNPACKED_RECORD_PAGE;

typedef struct _LFS_PACKED_RECORD_PAGE {

    //
    //  This gives us the offset of the free space in the page.
    //

    USHORT NextRecordOffset;

    USHORT WordAlign;

    ULONG DWordAlign;

    //
    //  The following is the Lsn for the last log record which ends on the page.
    //

    LSN LastEndLsn;

    //
    //  Update Sequence Array.  Used to protect the page block.
    //

    UPDATE_SEQUENCE_ARRAY UpdateSequenceArray;

} LFS_PACKED_RECORD_PAGE, *PLFS_PACKED_RECORD_PAGE;


//
//  Log Record Page Header.  This structure is present at the beginning of each
//  log file page in the client record section.
//

typedef struct _LFS_RECORD_PAGE_HEADER {

    //
    //  Cache multisector protection header.
    //

    MULTI_SECTOR_HEADER MultiSectorHeader;

    union {

        //
        //  Highest Lsn in this log file page.  This field is only for
        //  regular log pages.
        //

        LSN LastLsn;

        //
        //  Log file offset.  This is for the tail copies and indicates the
        //  location in the file where the original lays.  In this case the
        //  LastLsn field above can be obtained from the last ending Lsn
        //  field in the PACKED_RECORD_PAGE structure.
        //

        LONGLONG FileOffset;

    } Copy;

    //
    //  Page Header Flags.  These are the same flags that are stored in the
    //  Lbcb->Flags field.
    //
    //      LOG_PAGE_LOG_RECORD_END     -   Page contains the end of a log record
    //

    ULONG Flags;

    //
    //  I/O Page Position.  The following fields are used to determine
    //  where this log page resides within a Lfs I/O transfer.
    //

    USHORT PageCount;
    USHORT PagePosition;

    //
    //  The following is the difference between version 1.1 and earlier.
    //

    union {

        LFS_UNPACKED_RECORD_PAGE Unpacked;
        LFS_PACKED_RECORD_PAGE Packed;

    } Header;

} LFS_RECORD_PAGE_HEADER, *PLFS_RECORD_PAGE_HEADER;

#define LOG_PAGE_LOG_RECORD_END             (0x00000001)

#define LFS_UNPACKED_RECORD_PAGE_HEADER_SIZE        (                               \
    FIELD_OFFSET( LFS_RECORD_PAGE_HEADER, Header.Unpacked.UpdateSequenceArray )     \
)

#define LFS_PACKED_RECORD_PAGE_HEADER_SIZE          (                               \
    FIELD_OFFSET( LFS_RECORD_PAGE_HEADER, Header.Packed.UpdateSequenceArray )       \
)


//
//  Log Restart Page Header.  This structure is at the head of the restart
//  areas in a log file.
//

typedef struct _LFS_RESTART_PAGE_HEADER {

    //
    //  Cache multisector protection header.
    //

    MULTI_SECTOR_HEADER MultiSectorHeader;

    //
    //  This is the last Lsn found by checkdisk for this volume.
    //

    LSN ChkDskLsn;

    //
    //  System page size.  This is the page size of the system which
    //  initialized the log file.  Unless the log file has been gracefully
    //  shutdown (there are no clients with restart areas), it is a fatal
    //  error to attempt to write to a log file on a system with a differen
    //  page size.
    //

    ULONG SystemPageSize;

    //
    //  Log Page Size.  This is the log page size used for this log file.
    //  The entire Lfs restart area must fit on a single log page.
    //

    ULONG LogPageSize;

    //
    //  Lfs restart area offset.  This is the offset from the start of this
    //  structure to the Lfs restart area.
    //

    USHORT RestartOffset;

    //
    //  The indicates major and minor versions.  Note that the pre-release versions
    //  have -1 in both positions.  Major version 0 indicates the transition
    //  from Beta to USA support.
    //
    //      Major Version
    //
    //          -1      Beta Version
    //           0      Transition
    //           1      Update sequence support.
    //

    SHORT MinorVersion;
    SHORT MajorVersion;

    //
    //  Update Sequence Array.  Used to protect the page block.
    //

    UPDATE_SEQUENCE_ARRAY UpdateSequenceArray;

} LFS_RESTART_PAGE_HEADER, *PLFS_RESTART_PAGE_HEADER;

#define LFS_RESTART_PAGE_HEADER_SIZE    (                           \
    FIELD_OFFSET( LFS_RESTART_PAGE_HEADER, UpdateSequenceArray )    \
)

//
//  Id strings for the page headers.
//

#define LFS_SIGNATURE_RESTART_PAGE          "RSTR"
#define LFS_SIGNATURE_RESTART_PAGE_ULONG    0x52545352
#define LFS_SIGNATURE_RECORD_PAGE           "RCRD"
#define LFS_SIGNATURE_RECORD_PAGE_ULONG     0x44524352
#define LFS_SIGNATURE_BAD_USA               "BAAD"
#define LFS_SIGNATURE_BAD_USA_ULONG         0x44414142
#define LFS_SIGNATURE_MODIFIED              "CHKD"
#define LFS_SIGNATURE_MODIFIED_ULONG        0x444b4843
#define LFS_SIGNATURE_UNINITIALIZED         "\377\377\377\377"
#define LFS_SIGNATURE_UNINITIALIZED_ULONG   0xffffffff


//
//  Log Client Record.  A log client record exists for each client user of
//  the log file.  One of these is in each Lfs restart area.
//

#define LFS_NO_CLIENT                           0xffff
#define LFS_CLIENT_NAME_MAX                     64

typedef struct _LFS_CLIENT_RECORD {

    //
    //  Oldest Lsn.  This is the oldest Lsn that this client requires to
    //  be in the log file.
    //

    LSN OldestLsn;

    //
    //  Client Restart Lsn.  This is the Lsn of the latest client restart
    //  area written to the disk.  A reserved Lsn will indicate that no
    //  restart area exists for this client.
    //

    LSN ClientRestartLsn;

    //
    //
    //  Previous/Next client area.  These are the indexes into an array of
    //  Log Client Records for the previous and next client records.
    //

    USHORT PrevClient;
    USHORT NextClient;

    //
    //  Sequence Number.  Incremented whenever this record is reused.  This
    //  will happen whenever a client opens (reopens) the log file and has
    //  no current restart area.

    USHORT SeqNumber;

    //
    //  Alignment field.
    //

    USHORT AlignWord;

    //
    //  Align the entire record.
    //

    ULONG AlignDWord;

    //
    //  The following fields are used to describe the client name.  A client
    //  name consists of at most 32 Unicode character (64 bytes).  The Log
    //  file service will treat client names as case sensitive.
    //

    ULONG ClientNameLength;

    WCHAR ClientName[LFS_CLIENT_NAME_MAX];

} LFS_CLIENT_RECORD, *PLFS_CLIENT_RECORD;


//
//  Lfs Restart Area.  Two copies of these will exist at the beginning of the
//  log file.
//

typedef struct _LFS_RESTART_AREA {

    //
    //  Current Lsn.  This is periodic snapshot of the current logical end of
    //  log file to facilitate restart.
    //

    LSN CurrentLsn;

    //
    //  Number of Clients.  This is the maximum number of clients supported
    //  for this log file.
    //

    USHORT LogClients;

    //
    //  The following are indexes into the client record arrays.  The client
    //  records are linked into two lists.  A free list of client records and
    //  an in-use list of records.
    //

    USHORT ClientFreeList;
    USHORT ClientInUseList;

    //
    //  Flag field.
    //
    //      RESTART_SINGLE_PAGE_IO      All log pages written 1 by 1
    //

    USHORT Flags;

    //
    //  The following is the number of bits to use for the sequence number.
    //

    ULONG SeqNumberBits;

    //
    //  Length of this restart area.
    //

    USHORT RestartAreaLength;

    //
    //  Offset from the start of this structure to the client array.
    //  Ignored in versions prior to 1.1
    //

    USHORT ClientArrayOffset;

    //
    //  Usable log file size.  We will stop sharing the value in the page header.
    //

    LONGLONG FileSize;

    //
    //  DataLength of last Lsn.  This doesn't include the length of
    //  the Lfs header.
    //

    ULONG LastLsnDataLength;

    //
    //  The following apply to log pages.  This is the log page data offset and
    //  the length of the log record header.  Ignored in versions prior to 1.1
    //

    USHORT RecordHeaderLength;
    USHORT LogPageDataOffset;

    //
    //  Log file open count.  Used to determine if there has been a change to the disk.
    //

    ULONG RestartOpenLogCount;

    //
    //  Keep this structure quadword aligned.
    //

    ULONG AlignDWord;

    //
    //  Client data.
    //

    LFS_CLIENT_RECORD LogClientArray[1];

} LFS_RESTART_AREA, *PLFS_RESTART_AREA;

#define RESTART_SINGLE_PAGE_IO              (0x0001)

#define LFS_RESTART_AREA_SIZE       (FIELD_OFFSET( LFS_RESTART_AREA, LogClientArray ))

//
//  Remember the old size of the restart area when accessing older disks.
//

typedef struct _LFS_OLD_RESTART_AREA {

    //
    //  Current Lsn.  This is periodic snapshot of the current logical end of
    //  log file to facilitate restart.
    //

    LSN CurrentLsn;

    //
    //  Number of Clients.  This is the maximum number of clients supported
    //  for this log file.
    //

    USHORT LogClients;

    //
    //  The following are indexes into the client record arrays.  The client
    //  records are linked into two lists.  A free list of client records and
    //  an in-use list of records.
    //

    USHORT ClientFreeList;
    USHORT ClientInUseList;

    //
    //  Flag field.
    //
    //      RESTART_SINGLE_PAGE_IO      All log pages written 1 by 1
    //

    USHORT Flags;

    //
    //  The following is the number of bits to use for the sequence number.
    //

    ULONG SeqNumberBits;

    //
    //  Length of this restart area.
    //

    USHORT RestartAreaLength;

    //
    //  Offset from the start of this structure to the client array.
    //  Ignored in versions prior to 1.1
    //

    USHORT ClientArrayOffset;

    //
    //  Usable log file size.  We will stop sharing the value in the page header.
    //

    LONGLONG FileSize;

    //
    //  DataLength of last Lsn.  This doesn't include the length of
    //  the Lfs header.
    //

    ULONG LastLsnDataLength;

    //
    //  The following apply to log pages.  This is the log page data offset and
    //  the length of the log record header.  Ignored in versions prior to 1.1
    //

    USHORT RecordHeaderLength;
    USHORT LogPageDataOffset;

    //
    //  Client data.
    //

    LFS_CLIENT_RECORD LogClientArray[1];

} LFS_OLD_RESTART_AREA, *PLFS_OLD_RESTART_AREA;
#endif // _LFSDISK_
