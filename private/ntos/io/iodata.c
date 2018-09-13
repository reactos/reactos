/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    iodata.c

Abstract:

    This module contains the global read/write data for the I/O system.

Author:

    Darryl E. Havens (darrylh) April 27, 1989

Revision History:


--*/

#include "iop.h"

//
// Define the global read/write data for the I/O system.
//
// The following lock is used to guard access to the CancelRoutine address
// in IRPs.  It must be locked to set the address of a routine, clear the
// address of a routine, when a cancel routine is invoked, or when
// manipulating any structure that will set a cancel routine address in
// a packet.
//

extern KSPIN_LOCK IopCancelSpinLock;

//
// The following lock is used to guard access to VPB data structures.  It
// must be held each time the reference count, mount flag, or device object
// fields of a VPB are manipulated.
//

extern KSPIN_LOCK IopVpbSpinLock;

//
// The following lock is used to guard access to the I/O system database for
// unloading drivers.  It must be locked to increment or decrement device
// reference counts and to set the unload pending flag in a device object.
// The lock is allocated by the I/O system during phase 1 initialization.
//
// This lock is also used to decrement the count of Associated IRPs for a
// given Master IRP.
//

extern KSPIN_LOCK IopDatabaseLock;

//
// The following resource is used to control access to the I/O system's
// database.  It allows exclusive access to the file system queue for
// registering a file system as well as shared access to the same when
// searching for a file system to mount a volume on some media.  The resource
// is initialized by the I/O system initialization code during phase 1
// initialization.
//

ERESOURCE IopDatabaseResource;

//
// The following resource is used to control access to security descriptors
// on devices.  It allows multiple readers to perform security checks and
// queries on device security, but only a single writer to modify the security
// on a device at a time.
//

ERESOURCE IopSecurityResource;

//
// The following queue header contains the list of disk file systems currently
// loaded into the system.  The list actually contains the device objects
// for each of the file systems in the system.  Access to this queue is
// protected using the IopDatabaseResource for exclusive (write) or shared
// (read) access locks.
//

LIST_ENTRY IopDiskFileSystemQueueHead;

//
// The following queue header contains the list of CD ROM file systems currently
// loaded into the system.  The list actually contains the device objects
// for each of the file systems in the system.  Access to this queue is
// protected using the IopDatabaseResource for exclusive (write) or shared
// (read) access locks.
//

LIST_ENTRY IopCdRomFileSystemQueueHead;

//
// The following queue header contains the list of network file systems
// (redirectors) currently loaded into the system.  The list actually
// contains the device objects for each of the network file systems in the
// system.  Access to this queue is protected using the IopDatabaseResource
// for exclusive (write) or shared (read) access locks.
//

LIST_ENTRY IopNetworkFileSystemQueueHead;

//
// The following queue header contains the list of tape file systems currently
// loaded into the system.  The list actually contains the device objects
// for each of the file systems in the system.  Access to this queue is
// protected using the IopDatabaseResource for exclusive (write) or shared
// (read) access locks.
//

LIST_ENTRY IopTapeFileSystemQueueHead;

//
// The following queue header contains the list of boot drivers that have
// registered for a call back once all devices have been enumerated.
//

LIST_ENTRY IopBootDriverReinitializeQueueHead;

//
// The following queue header contains the list of drivers that have
// registered reinitialization routines.
//

LIST_ENTRY IopDriverReinitializeQueueHead;

//
// The following queue headers contain the lists of the drivers that have
// registered shutdown notification routines.
//

LIST_ENTRY IopNotifyShutdownQueueHead;
LIST_ENTRY IopNotifyLastChanceShutdownQueueHead;

//
// The following queue header contains the list of the driver that have
// registered to be notified when a file system registers or unregisters itself
// as an active file system.
//

LIST_ENTRY IopFsNotifyChangeQueueHead;

//
// The following are the lookaside lists used to keep track of the two I/O
// Request Packet (IRP), the Memory Descriptor List (MDL) Lookaside list, and
// the I/O Completion List (ICP) Lookaside list.
//
// The "large" IRP contains 4 stack locations, the maximum in the SDK, and the
// "small" IRP contains a single entry, the most common case for devices other
// than disks and network devices.
//

NPAGED_LOOKASIDE_LIST IopCompletionLookasideList;
NPAGED_LOOKASIDE_LIST IopLargeIrpLookasideList;
NPAGED_LOOKASIDE_LIST IopSmallIrpLookasideList;
NPAGED_LOOKASIDE_LIST IopMdlLookasideList;
ULONG IopLargeIrpStackLocations;


//
// The following spinlock is used to control access to the I/O system's error
// log database.  It is initialized by the I/O system initialization code when
// the system is being initialized.  This lock must be owned in order to insert
// or remove entries from either the free or entry queue.
//

extern KSPIN_LOCK IopErrorLogLock;

//
// The following is the list head for all error log entries in the system which
// have not yet been sent to the error log process.  Entries are written placed
// onto the list by the IoWriteElEntry procedure.
//

LIST_ENTRY IopErrorLogListHead;

//
// The following is used to track how much memory is allocated to I/O error log
// packets.  The spinlock is used to protect this variable.
//

ULONG IopErrorLogAllocation;
extern KSPIN_LOCK IopErrorLogAllocationLock;

//
// The following spinlock is used by the I/O system to synchronize examining
// the thread field of an I/O Request Packet so that the request can be
// queued as a special kernel APC to the thread.  The reason that the
// spinlock must be used is for cases when the request times out, and so
// the thread has been permitted to possibly exit.
//

extern KSPIN_LOCK IopCompletionLock;

//
// The following global contains the queue of informational hard error
// pop-ups.
//

IOP_HARD_ERROR_QUEUE IopHardError;

//
// The following global is non-null when there is a pop-up on the screen
// waiting for user action.  It points to that packet.
//

PIOP_HARD_ERROR_PACKET IopCurrentHardError;

//
// The following are used to implement the I/O system's one second timer.
// The lock protects access to the queue, the queue contains an entry for
// each driver that needs to be invoked, and the timer and DPC data
// structures are used to actually get the internal timer routine invoked
// once every second.  The count is used to maintain the number of timer
// entries that actually indicate that the driver is to be invoked.
//

extern KSPIN_LOCK IopTimerLock;
LIST_ENTRY IopTimerQueueHead;
KDPC IopTimerDpc;
KTIMER IopTimer;
ULONG IopTimerCount;

//
// The following are the global pointers for the Object Type Descriptors that
// are created when each of the I/O specific object types are created.
//

POBJECT_TYPE IoAdapterObjectType;
POBJECT_TYPE IoControllerObjectType;
POBJECT_TYPE IoCompletionObjectType;
POBJECT_TYPE IoDeviceObjectType;
POBJECT_TYPE IoDriverObjectType;
POBJECT_TYPE IoDeviceHandlerObjectType;
POBJECT_TYPE IoFileObjectType;
ULONG        IoDeviceHandlerObjectSize;

//
// The following is a global lock and counters for I/O operations requested
// on a system-wide basis.  The first three counters simply track the number
// of read, write, and other types of operations that have been requested.
// The latter three counters track the actual number of bytes that have been
// transferred throughout the system.
//

extern KSPIN_LOCK IoStatisticsLock;
ULONG IoReadOperationCount;
ULONG IoWriteOperationCount;
ULONG IoOtherOperationCount;
LARGE_INTEGER IoReadTransferCount;
LARGE_INTEGER IoWriteTransferCount;
LARGE_INTEGER IoOtherTransferCount;

//
// The following is the base pointer for the crash dump control block that is
// used to control dumping all of physical memory to the paging file after a
// system crash.  And, the checksum for the dump control block is also declared.
//

PDUMP_CONTROL_BLOCK IopDumpControlBlock;
ULONG IopDumpControlBlockChecksum;

//
// The following are the spin lock and event that allow the I/O system to
// implement fast file object locks.
//

KEVENT IopFastLockEvent;

//
// The following is a monotonically increasing number (retrieved via
// InterlockedIncrement) that is used by IoCreateDevice to automatically
// generate a device object name when the FILE_AUTOGENERATED_DEVICE_NAME
// device characteristic is specified.
//

LONG IopUniqueDeviceObjectNumber;

//
// IoRemoteBootClient indicates whether the system was booted as a remote
// boot client.
//

BOOLEAN IoRemoteBootClient;

#if defined(REMOTE_BOOT)
//
// The following indicates whether or not the Client Side Caching subsystem
// was successfully initialized.
//

BOOLEAN IoCscInitializationFailed;
#endif

//
// The following are used to synchronize with the link tracking service while establishing a connection.
//

KEVENT IopLinkTrackingPortObject;
LINK_TRACKING_PACKET IopLinkTrackingPacket;

//
// Function pointers of key IO routines.
// The functions need to be in their own cache lines as they are readonly and 
// never modified after boot.
//

#define CACHE_SIZE      128
UCHAR                   IopPrePadding[CACHE_SIZE] = {0};
PIO_CALL_DRIVER         pIofCallDriver = 0;
PIO_COMPLETE_REQUEST    pIofCompleteRequest = 0;
PIO_ALLOCATE_IRP        pIoAllocateIrp = 0;
PIO_FREE_IRP            pIoFreeIrp = 0;
UCHAR                   IopPostPadding[CACHE_SIZE] = {0};

//*********
//
// Note:  All of the following data is potentially pageable, depending on the
//        target platform.
//
//*********

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("PAGE")
#endif

//
// The following semaphore is used by the IO system when it reports resource
// usage to the configuration registry on behalf of a driver.  This semaphore
// is initialized by the I/O system initialization code when the system is
// started.
//

KSEMAPHORE IopRegistrySemaphore;

//
// The following are used to store the handle and a pointer to the referenced

// whenever a file is moved across systems.
//

PVOID IopLinkTrackingServiceObject;
PKEVENT IopLinkTrackingServiceEvent;


//
// The following array specifies the minimum length of the FileInformation
// buffer for an NtQueryInformationFile service.
//
// WARNING:  This array depends on the order of the values in the
//           FileInformationClass enumerated type.  Note that the
//           enumerated type is one-based and the array is zero-based.
//

UCHAR IopQueryOperationLength[] =
          {
            0,
            0,                                         //  1 FileDirectoryInformation
            0,                                         //  2 FileFullDirectoryInformation
            0,                                         //  3 FileBothDirectoryInformation
            sizeof( FILE_BASIC_INFORMATION ),          //  4 FileBasicInformation
            sizeof( FILE_STANDARD_INFORMATION ),       //  5 FileStandardInformation
            sizeof( FILE_INTERNAL_INFORMATION ),       //  6 FileInternalInformation
            sizeof( FILE_EA_INFORMATION ),             //  7 FileEaInformation
            sizeof( FILE_ACCESS_INFORMATION ),         //  8 FileAccessInformation
            sizeof( FILE_NAME_INFORMATION ),           //  9 FileNameInformation
            0,                                         // 10 FileRenameInformation
            0,                                         // 11 FileLinkInformation
            0,                                         // 12 FileNamesInformation
            0,                                         // 13 FileDispositionInformation
            sizeof( FILE_POSITION_INFORMATION ),       // 14 FilePositionInformation
            0,                                         // 15 FileFullEaInformation
            sizeof( FILE_MODE_INFORMATION ),           // 16 FileModeInformation
            sizeof( FILE_ALIGNMENT_INFORMATION ),      // 17 FileAlignmentInformation
            sizeof( FILE_ALL_INFORMATION ),            // 18 FileAllInformation
            0,                                         // 19 FileAllocationInformation
            0,                                         // 20 FileEndOfFileInformation
            sizeof( FILE_NAME_INFORMATION ),           // 21 FileAlternateNameInformation
            sizeof( FILE_STREAM_INFORMATION ),         // 22 FileStreamInformation
            sizeof( FILE_PIPE_INFORMATION ),           // 23 FilePipeInformation
            sizeof( FILE_PIPE_LOCAL_INFORMATION ),     // 24 FilePipeLocalInformation
            sizeof( FILE_PIPE_REMOTE_INFORMATION ),    // 25 FilePipeRemoteInformation
            sizeof( FILE_MAILSLOT_QUERY_INFORMATION ), // 26 FileMailslotQueryInformation
            0,                                         // 27 FileMailslotSetInformation
            sizeof( FILE_COMPRESSION_INFORMATION ),    // 28 FileCompressionInformation
            sizeof( FILE_OBJECTID_INFORMATION ),       // 29 FileObjectIdInformation
            0,                                         // 30 FileCompletionInformation
            0,                                         // 31 FileMoveClusterInformation
            sizeof( FILE_QUOTA_INFORMATION ),          // 32 FileQuotaInformation
            sizeof( FILE_REPARSE_POINT_INFORMATION ),  // 33 FileReparsePointInformation
            sizeof( FILE_NETWORK_OPEN_INFORMATION),    // 34 FileNetworkOpenInformation
            sizeof( FILE_ATTRIBUTE_TAG_INFORMATION),   // 35 FileAttributeTagInformation
            0,                                         // 36 FileTrackingInformation
            0xff                                       //    FileMaximumInformation
          };

//
// The following array specifies the minimum length of the FileInformation
// buffer for an NtSetInformationFile service.
//
// WARNING:  This array depends on the order of the values in the
//           FileInformationClass enumerated type.  Note that the
//           enumerated type is one-based and the array is zero-based.
//

UCHAR IopSetOperationLength[] =
          {
            0,
            0,                                       //  1 FileDirectoryInformation
            0,                                       //  2 FileFullDirectoryInformation
            0,                                       //  3 FileBothDirectoryInformation
            sizeof( FILE_BASIC_INFORMATION ),        //  4 FileBasicInformation
            0,                                       //  5 FileStandardInformation
            0,                                       //  6 FileInternalInformation
            0,                                       //  7 FileEaInformation
            0,                                       //  8 FileAccessInformation
            0,                                       //  9 FileNameInformation
            sizeof( FILE_RENAME_INFORMATION ),       // 10 FileRenameInformation
            sizeof( FILE_LINK_INFORMATION ),         // 11 FileLinkInformation
            0,                                       // 12 FileNamesInformation
            sizeof( FILE_DISPOSITION_INFORMATION ),  // 13 FileDispositionInformation
            sizeof( FILE_POSITION_INFORMATION ),     // 14 FilePositionInformation
            0,                                       // 15 FileFullEaInformation
            sizeof( FILE_MODE_INFORMATION ),         // 16 FileModeInformation
            0,                                       // 17 FileAlignmentInformation
            0,                                       // 18 FileAllInformation
            sizeof( FILE_ALLOCATION_INFORMATION ),   // 19 FileAllocationInformation
            sizeof( FILE_END_OF_FILE_INFORMATION ),  // 20 FileEndOfFileInformation
            0,                                       // 21 FileAlternateNameInformation
            0,                                       // 22 FileStreamInformation
            sizeof( FILE_PIPE_INFORMATION ),         // 23 FilePipeInformation
            0,                                       // 24 FilePipeLocalInformation
            sizeof( FILE_PIPE_REMOTE_INFORMATION ),  // 25 FilePipeRemoteInformation
            0,                                       // 26 FileMailslotQueryInformation
            sizeof( FILE_MAILSLOT_SET_INFORMATION ), // 27 FileMailslotSetInformation
            0,                                       // 28 FileCompressionInformation
            sizeof( FILE_OBJECTID_INFORMATION ),     // 29 FileObjectIdInformation
            sizeof( FILE_COMPLETION_INFORMATION ),   // 30 FileCompletionInformation
            sizeof( FILE_MOVE_CLUSTER_INFORMATION ), // 31 FileMoveClusterInformation
            sizeof( FILE_QUOTA_INFORMATION ),        // 32 FileQuotaInformation
            0,                                       // 33 FileReparsePointInformation
            0,                                       // 34 FileNetworkOpenInformation
            0,                                       // 35 FileAttributeTagInformation
            sizeof( FILE_TRACKING_INFORMATION ),     // 36 FileTrackingInformation
            0xff                                     //    FileMaximumInformation
          };

//
// The following array specifies the alignment requirement of both all query
// and set operations, including directory operations, but not FS operations.
//
// WARNING:  This array depends on the order of the values in the
//           FileInformationClass enumerated type.  Note that the
//           enumerated type is one-based and the array is zero-based.
//

UCHAR IopQuerySetAlignmentRequirement[] =
          {
            0,
            sizeof( LONGLONG ), //  1 FileDirectoryInformation
            sizeof( LONGLONG ), //  2 FileFullDirectoryInformation
            sizeof( LONGLONG ), //  3 FileBothDirectoryInformation
            sizeof( LONGLONG ), //  4 FileBasicInformation
            sizeof( LONGLONG ), //  5 FileStandardInformation
            sizeof( LONGLONG ), //  6 FileInternalInformation
            sizeof( LONG ),     //  7 FileEaInformation
            sizeof( LONG ),     //  8 FileAccessInformation
            sizeof( LONG ),     //  9 FileNameInformation
            sizeof( LONG ),     // 10 FileRenameInformation
            sizeof( LONG ),     // 11 FileLinkInformation
            sizeof( LONG ),     // 12 FileNamesInformation
            sizeof( CHAR ),     // 13 FileDispositionInformation
            sizeof( LONGLONG ), // 14 FilePositionInformation
            sizeof( LONG ),     // 15 FileFullEaInformation
            sizeof( LONG ),     // 16 FileModeInformation
            sizeof( LONG ),     // 17 FileAlignmentInformation
            sizeof( LONGLONG ), // 18 FileAllInformation
            sizeof( LONGLONG ), // 19 FileAllocationInformation
            sizeof( LONGLONG ), // 20 FileEndOfFileInformation
            sizeof( LONG ),     // 21 FileAlternateNameInformation
            sizeof( LONGLONG ), // 22 FileStreamInformation
            sizeof( LONG ),     // 23 FilePipeInformation
            sizeof( LONG ),     // 24 FilePipeLocalInformation
            sizeof( LONG ),     // 25 FilePipeRemoteInformation
            sizeof( LONGLONG ), // 26 FileMailslotQueryInformation
            sizeof( LONG ),     // 27 FileMailslotSetInformation
            sizeof( LONGLONG ), // 28 FileCompressionInformation
            sizeof( LONG ),     // 29 FileObjectIdInformation
            sizeof( LONG ),     // 30 FileCompletionInformation
            sizeof( LONG ),     // 31 FileMoveClusterInformation
            sizeof( LONG ),     // 32 FileQuotaInformation
            sizeof( LONG ),     // 33 FileReparsePointInformation
            sizeof( LONGLONG ), // 34 FileNetworkOpenInformation
            sizeof( LONG ),     // 35 FileAttributeTagInformation
            sizeof( LONG ),     // 36 FileTrackingInformation
            0xff                //    FileMaximumInformation
          };

//
// The following array specifies the required access mask for the caller to
// access information in an NtQueryXxxFile service.
//
// WARNING:  This array depends on the order of the values in the
//           FileInformationClass enumerated type.  Note that the
//           enumerated type is one-based and the array is zero-based.
//

ULONG IopQueryOperationAccess[] =
         {
            0,
            0,                    //  1 FileDirectoryInformation
            0,                    //  2 FileFullDirectoryInformation
            0,                    //  3 FileBothDirectoryInformation
            FILE_READ_ATTRIBUTES, //  4 FileBasicInformation
            0,                    //  5 FileStandardInformation
            0,                    //  6 FileInternalInformation
            0,                    //  7 FileEaInformation
            0,                    //  8 FileAccessInformation
            0,                    //  9 FileNameInformation
            0,                    // 10 FileRenameInformation
            0,                    // 11 FileLinkInformation
            0,                    // 12 FileNamesInformation
            0,                    // 13 FileDispositionInformation
            0,                    // 14 FilePositionInformation
            FILE_READ_EA,         // 15 FileFullEaInformation
            0,                    // 16 FileModeInformation
            0,                    // 17 FileAlignmentInformation
            FILE_READ_ATTRIBUTES, // 18 FileAllInformation
            0,                    // 19 FileAllocationInformation
            0,                    // 20 FileEndOfFileInformation
            0,                    // 21 FileAlternateNameInformation
            0,                    // 22 FileStreamInformation
            FILE_READ_ATTRIBUTES, // 23 FilePipeInformation
            FILE_READ_ATTRIBUTES, // 24 FilePipeLocalInformation
            FILE_READ_ATTRIBUTES, // 25 FilePipeRemoteInformation
            0,                    // 26 FileMailslotQueryInformation
            0,                    // 27 FileMailslotSetInformation
            0,                    // 28 FileCompressionInformation
            0,                    // 29 FileObjectIdInformation
            0,                    // 30 FileCompletionInformation
            0,                    // 31 FileMoveClusterInformation
            0,                    // 32 FileQuotaInformation
            0,                    // 33 FileReparsePointInformation
            FILE_READ_ATTRIBUTES, // 34 FileNetworkOpenInformation
            FILE_READ_ATTRIBUTES, // 35 FileAttributeTagInformation
            0,                    // 36 FileTrackingInformation
            0xffffffff            //    FileMaximumInformation
          };

//
// The following array specifies the required access mask for the caller to
// access information in an NtSetXxxFile service.
//
// WARNING:  This array depends on the order of the values in the
//           FILE_INFORMATION_CLASS enumerated type.  Note that the
//           enumerated type is one-based and the array is zero-based.
//

ULONG IopSetOperationAccess[] =
         {
            0,
            0,                     //  1 FileDirectoryInformation
            0,                     //  2 FileFullDirectoryInformation
            0,                     //  3 FileBothDirectoryInformation
            FILE_WRITE_ATTRIBUTES, //  4 FileBasicInformation
            0,                     //  5 FileStandardInformation
            0,                     //  6 FileInternalInformation
            0,                     //  7 FileEaInformation
            0,                     //  8 FileAccessInformation
            0,                     //  9 FileNameInformation
            DELETE,                // 10 FileRenameInformation
            0,                     // 11 FileLinkInformation
            0,                     // 12 FileNamesInformation
            DELETE,                // 13 FileDispositionInformation
            0,                     // 14 FilePositionInformation
            FILE_WRITE_EA,         // 15 FileFullEaInformation
            0,                     // 16 FileModeInformation
            0,                     // 17 FileAlignmentInformation
            0,                     // 18 FileAllInformation
            FILE_WRITE_DATA,       // 19 FileAllocationInformation
            FILE_WRITE_DATA,       // 20 FileEndOfFileInformation
            0,                     // 21 FileAlternateNameInformation
            0,                     // 22 FileStreamInformation
            FILE_WRITE_ATTRIBUTES, // 23 FilePipeInformation
            0,                     // 24 FilePipeLocalInformation
            FILE_WRITE_ATTRIBUTES, // 25 FilePipeRemoteInformation
            0,                     // 26 FileMailslotQueryInformation
            0,                     // 27 FileMailslotSetInformation
            0,                     // 28 FileCompressionInformation
            0,                     // 29 FileObjectIdInformation
            0,                     // 30 FileCompletionInformation
            FILE_WRITE_DATA,       // 31 FileMoveClusterInformation
            0,                     // 32 FileQuotaInformation
            0,                     // 33 FileReparsePointInformation
            0,                     // 34 FileNetworkOpenInformation
            0,                     // 35 FileAttributeTagInformation
            FILE_WRITE_DATA,       // 36 FileTrackingInformation
            0xffffffff             //    FileMaximumInformation
          };

//
// The following array specifies the minimum length of the FsInformation
// buffer for an NtQueryVolumeInformation service.
//
// WARNING:  This array depends on the order of the values in the
//           FS_INFORMATION_CLASS enumerated type.  Note that the
//           enumerated type is one-based and the array is zero-based.
//

UCHAR IopQueryFsOperationLength[] =
          {
            0,
            sizeof( FILE_FS_VOLUME_INFORMATION ),    // 1 FileFsVolumeInformation
            0,                                       // 2 FileFsLabelInformation
            sizeof( FILE_FS_SIZE_INFORMATION ),      // 3 FileFsSizeInformation
            sizeof( FILE_FS_DEVICE_INFORMATION ),    // 4 FileFsDeviceInformation
            sizeof( FILE_FS_ATTRIBUTE_INFORMATION ), // 5 FileFsAttributeInformation
            sizeof( FILE_FS_CONTROL_INFORMATION ),   // 6 FileFsControlInformation
            sizeof( FILE_FS_FULL_SIZE_INFORMATION ), // 7 FileFsFullSizeInformation
            sizeof( FILE_FS_OBJECTID_INFORMATION ),  // 8 FileFsObjectIdInformation
            0xff                                     //   FileFsMaximumInformation
          };

//
// The following array specifies the minimum length of the FsInformation
// buffer for an NtSetVolumeInformation service.
//
// WARNING:  This array depends on the order of the values in the
//           FS_INFORMATION_CLASS enumerated type.  Note that the
//           enumerated type is one-based and the array is zero-based.
//

UCHAR IopSetFsOperationLength[] =
          {
            0,
            0,                                     // 1 FileFsVolumeInformation
            sizeof( FILE_FS_LABEL_INFORMATION ),   // 2 FileFsLabelInformation
            0,                                     // 3 FileFsSizeInformation
            0,                                     // 4 FileFsDeviceInformation
            0,                                     // 5 FileFsAttributeInformation
            sizeof( FILE_FS_CONTROL_INFORMATION ), // 6 FileFsControlInformation
            0,                                     // 7 FileFsFullSizeInformation
            sizeof( FILE_FS_OBJECTID_INFORMATION ),// 8 FileFsObjectIdInformation
            0xff                                   //   FileFsMaximumInformation
          };

//
// The following array specifies the required access mask for the caller to
// access information in an NtQueryVolumeInformation service.
//
// WARNING:  This array depends on the order of the values in the
//           FS_INFORMATION_CLASS enumerated type.  Note that the
//           enumerated type is one-based and the array is zero-based.
//

ULONG IopQueryFsOperationAccess[] =
         {
            0,
            0,              // 1 FileFsVolumeInformation [any access to file or volume]
            0,              // 2 FileFsLabelInformation [query is invalid]
            0,              // 3 FileFsSizeInformation [any access to file or volume]
            0,              // 4 FileFsDeviceInformation [any access to file or volume]
            0,              // 5 FileFsAttributeInformation [any access to file or vol]
            FILE_READ_DATA, // 6 FileFsControlInformation [vol read access]
            0,              // 7 FileFsFullSizeInformation [any access to file or volume]
            0,              // 8 FileFsObjectIdInformation [any access to file or volume]
            0xffffffff      //   FileFsMaximumInformation
          };

//
// The following array specifies the required access mask for the caller to
// access information in an NtSetVolumeInformation service.
//
// WARNING:  This array depends on the order of the values in the
//           FS_INFORMATION_CLASS enumerated type.  Note that the
//           enumerated type is one-based and the array is zero-based.
//

ULONG IopSetFsOperationAccess[] =
         {
            0,
            0,               // 1 FileFsVolumeInformation [set is invalid]
            FILE_WRITE_DATA, // 2 FileFsLabelInformation [write access to volume]
            0,               // 3 FileFsSizeInformation [set is invalid]
            0,               // 4 FileFsDeviceInformation [set is invalid]
            0,               // 5 FileFsAttributeInformation [set is invalid]
            FILE_WRITE_DATA, // 6 FileFsControlInformation [vol write access]
            0,               // 7 FileFsFullSizeInformation [set is invalid]
            FILE_WRITE_DATA, // 8 FileFsObjectIdInformation [write access to volume]
            0xffffffff       //   FileFsMaximumInformation
          };

//
// The following array specifies the alignment requirements for all FS query
// and set information services.
//
// WARNING:  This array depends on the order of the values in the
//           FS_INFORMATION_CLASS enumerated type.  Note that the
//           enumerated type is one-based and the array is zero-based.
//

UCHAR IopQuerySetFsAlignmentRequirement[] =
         {
            0,
            sizeof( LONGLONG ), // 1 FileFsVolumeInformation
            sizeof( LONG ),     // 2 FileFsLabelInformation
            sizeof( LONGLONG ), // 3 FileFsSizeInformation
            sizeof( LONG ),     // 4 FileFsDeviceInformation
            sizeof( LONG ),     // 5 FileFsAttributeInformation
            sizeof( LONGLONG ), // 6 FileFsControlInformation
            sizeof( LONGLONG ), // 7 FileFsFullSizeInformation
            sizeof( LONGLONG ), // 8 FileFsObjectIdInformation
            0xff                //   FileFsMaximumInformation
          };

PVOID IopLoaderBlock = NULL;

BOOLEAN IopRemoteBootCardInitialized = FALSE;

WCHAR IopWstrRaw[]                  = L".Raw";
WCHAR IopWstrTranslated[]           = L".Translated";
WCHAR IopWstrBusRaw[]               = L".Bus.Raw";
WCHAR IopWstrBusTranslated[]        = L".Bus.Translated";
WCHAR IopWstrOtherDrivers[]         = L"OtherDrivers";

WCHAR IopWstrAssignedResources[]    = L"AssignedSystemResources";
WCHAR IopWstrRequestedResources[]   = L"RequestedSystemResources";
WCHAR IopWstrSystemResources[]      = L"Control\\SystemResources";
WCHAR IopWstrReservedResources[]    = L"ReservedResources";
WCHAR IopWstrAssignmentOrdering[]   = L"AssignmentOrdering";
WCHAR IopWstrBusValues[]            = L"BusValues";
UNICODE_STRING IoArcBootDeviceName  = { 0 };
UNICODE_STRING IoArcHalDeviceName  = { 0 };
PUCHAR IoLoaderArcBootDeviceName = NULL;

//
// Initialization time data
//

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("INIT")
#endif

WCHAR IopWstrHal[]                  = L"Hardware Abstraction Layer";
WCHAR IopWstrSystem[]               = L"System Resources";
WCHAR IopWstrPhysicalMemory[]       = L"Physical Memory";
WCHAR IopWstrSpecialMemory[]        = L"Reserved";

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg()
#endif
