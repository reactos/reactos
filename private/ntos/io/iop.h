/*++ BUILD Version: 0002

Copyright (c) 1989  Microsoft Corporation

Module Name:

    iop.h

Abstract:

    This module contains the private structure definitions and APIs used by
    the NT I/O system.

Author:

    Darryl E. Havens (darrylh) 17-Apr-1989


Revision History:


--*/

#ifndef _IOP_
#define _IOP_

#ifndef FAR
#define FAR
#endif

#include "ntos.h"
#include "ntdddisk.h"
#include "ntddscsi.h"
#include "mountmgr.h"
#include "ntiodump.h"
#include "ntiolog.h"
#include "ntiologc.h"
#include "ntseapi.h"
#include "zwapi.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "fsrtl.h"
#include "pnpiop.h"
#include "ioverifier.h"

#ifdef POOL_TAGGING
#undef ExAllocatePool
#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,'  oI')
#undef ExAllocatePoolWithQuota
#define ExAllocatePoolWithQuota(a,b) ExAllocatePoolWithQuotaTag(a,b,'  oI')
#endif


#include "safeboot.h"

extern ULONG BreakDiskByteOffset;
extern ULONG BreakPfn;

//
// Define Information fields values for the return value from popups when a
// volume mount is in progress but failed.
//

#define IOP_ABORT                       1

//
// Define transfer types for process counters.
//

typedef enum _TRANSFER_TYPE {
    ReadTransfer,
    WriteTransfer,
    OtherTransfer
} TRANSFER_TYPE, *PTRANSFER_TYPE;

//
// Define the maximum amount of memory that can be allocated for all
// outstanding error log packets.
//

#define IOP_MAXIMUM_LOG_ALLOCATION PAGE_SIZE

//
// Define an error log entry.
//

typedef struct _ERROR_LOG_ENTRY {
    USHORT Type;
    USHORT Size;
    LIST_ENTRY ListEntry;
    PDEVICE_OBJECT DeviceObject;
    PDRIVER_OBJECT DriverObject;
    LARGE_INTEGER TimeStamp;
} ERROR_LOG_ENTRY, *PERROR_LOG_ENTRY;

//
//  Define both the global IOP_HARD_ERROR_QUEUE and IOP_HARD_ERROR_PACKET
//  structures.   Also set the maximum number of outstanding hard error
//  packets allowed.
//

typedef struct _IOP_HARD_ERROR_QUEUE {
    WORK_QUEUE_ITEM ExWorkItem;
    LIST_ENTRY WorkQueue;
    KSPIN_LOCK WorkQueueSpinLock;
    KSEMAPHORE WorkQueueSemaphore;
    BOOLEAN ThreadStarted;
    ULONG   NumPendingApcPopups;
} IOP_HARD_ERROR_QUEUE, *PIOP_HARD_ERROR_QUEUE;

typedef struct _IOP_HARD_ERROR_PACKET {
    LIST_ENTRY WorkQueueLinks;
    NTSTATUS ErrorStatus;
    UNICODE_STRING String;
} IOP_HARD_ERROR_PACKET, *PIOP_HARD_ERROR_PACKET;

typedef struct _IOP_APC_HARD_ERROR_PACKET {
    WORK_QUEUE_ITEM Item;
    PIRP Irp;
    PVPB Vpb;
    PDEVICE_OBJECT RealDeviceObject;
} IOP_APC_HARD_ERROR_PACKET, *PIOP_APC_HARD_ERROR_PACKET;

typedef
NTSTATUS
(FASTCALL *PIO_CALL_DRIVER) (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

typedef
VOID
(FASTCALL *PIO_COMPLETE_REQUEST) (
    IN PIRP Irp,
    IN CCHAR PriorityBoost
    );

typedef
VOID
(*PIO_FREE_IRP) (
    IN struct _IRP *Irp
    );

typedef
PIRP
(*PIO_ALLOCATE_IRP) (
    IN CCHAR   StackSize,
    IN BOOLEAN ChargeQuota
    );

extern IOP_HARD_ERROR_QUEUE IopHardError;
extern PIOP_HARD_ERROR_PACKET IopCurrentHardError;

#define IOP_MAXIMUM_OUTSTANDING_HARD_ERRORS 25

typedef struct _IO_WORKITEM {
    WORK_QUEUE_ITEM WorkItem;
    PIO_WORKITEM_ROUTINE Routine;
    PDEVICE_OBJECT DeviceObject;
    PVOID Context;
#if DBG
    ULONG Size;
#endif
} IO_WORKITEM;

//
// Define the global data for the error logger and I/O system.
//

extern WORK_QUEUE_ITEM IopErrorLogWorkItem;
extern BOOLEAN IopErrorLogPortPending;
extern BOOLEAN IopErrorLogDisabledThisBoot;
extern KSPIN_LOCK IopErrorLogLock;
extern LIST_ENTRY IopErrorLogListHead;
extern ULONG IopErrorLogAllocation;
extern KSPIN_LOCK IopErrorLogAllocationLock;
extern KSPIN_LOCK IopCancelSpinLock;
extern KSPIN_LOCK IopVpbSpinLock;
extern GENERIC_MAPPING IopFileMapping;
extern GENERIC_MAPPING IopCompletionMapping;

//
// Define a dummy file object for use on stack for fast open operations.
//

typedef struct _DUMMY_FILE_OBJECT {
    OBJECT_HEADER ObjectHeader;
    CHAR FileObjectBody[ sizeof( FILE_OBJECT ) ];
} DUMMY_FILE_OBJECT, *PDUMMY_FILE_OBJECT;

//
// Define the structures private to the I/O system.
//

#define OPEN_PACKET_PATTERN  0xbeaa0251

//
// Define an Open Packet (OP).  An OP is used to communicate information
// between the NtCreateFile service executing in the context of the caller
// and the device object parse routine.  It is the parse routine who actually
// creates the file object for the file.
//

typedef struct _OPEN_PACKET {
    CSHORT Type;
    CSHORT Size;
    PFILE_OBJECT FileObject;
    NTSTATUS FinalStatus;
    ULONG_PTR Information;
    ULONG ParseCheck;
    PFILE_OBJECT RelatedFileObject;

    //
    // The following are the open-specific parameters.  Notice that the desired
    // access field is passed through to the parse routine via the object
    // management architecture, so it does not need to be repeated here.  Also
    // note that the same is true for the file name.
    //

    LARGE_INTEGER AllocationSize;
    ULONG CreateOptions;
    USHORT FileAttributes;
    USHORT ShareAccess;
    PVOID EaBuffer;
    ULONG EaLength;
    ULONG Options;
    ULONG Disposition;

    //
    // The following is used when performing a fast query during open to get
    // back the file attributes for a file.
    //

    PFILE_BASIC_INFORMATION BasicInformation;

    //
    // The following is used when performing a fast network query during open
    // to get back the network file attributes for a file.
    //

    PFILE_NETWORK_OPEN_INFORMATION NetworkInformation;

    //
    // The type of file to create.
    //

    CREATE_FILE_TYPE CreateFileType;

    //
    // The following pointer provides a way of passing the parameters
    // specific to the file type of the file being created to the parse
    // routine.
    //

    PVOID ExtraCreateParameters;

    //
    // The following is used to indicate that an open of a device has been
    // performed and the access check for the device has already been done,
    // but because of a reparse, the I/O system has been called again for
    // the same device.  Since the access check has already been made, the
    // state cannot handle being called again (access was already granted)
    // and it need not anyway since the check has already been made.
    //

    BOOLEAN Override;

    //
    // The following is used to indicate that a file is being opened for the
    // sole purpose of querying its attributes.  This causes a considerable
    // number of shortcuts to be taken in the parse, query, and close paths.
    //

    BOOLEAN QueryOnly;

    //
    // The following is used to indicate that a file is being opened for the
    // sole purpose of deleting it.  This causes a considerable number of
    // shortcurs to be taken in the parse and close paths.
    //

    BOOLEAN DeleteOnly;

    //
    // The following is used to indicate that a file being opened for a query
    // only is being opened to query its network attributes rather than just
    // its FAT file attributes.
    //

    BOOLEAN FullAttributes;

    //
    // The following pointer is used when a fast open operation for a fast
    // delete or fast query attributes call is being made rather than a
    // general file open.  The dummy file object is actually stored on the
    // the caller's stack rather than allocated pool to speed things up.
    //

    PDUMMY_FILE_OBJECT LocalFileObject;

} OPEN_PACKET, *POPEN_PACKET;

//
// Define a Load Packet (LDP).  An LDP is used to communicate load and unload
// driver information between the appropriate system services and the routine
// that actually performs the work.  This is implemented using a packet
// because various drivers need to be initialized in the context of THE
// system process because they create threads within its context which open
// handles to objects that henceforth are only valid in the context of that
// process.
//

typedef struct _LOAD_PACKET {
    WORK_QUEUE_ITEM WorkQueueItem;
    KEVENT Event;
    PDRIVER_OBJECT DriverObject;
    PUNICODE_STRING DriverServiceName;
    NTSTATUS FinalStatus;
} LOAD_PACKET, *PLOAD_PACKET;

//
// Define a Link Tracking Packet.  A link tracking packet is used to open the
// user-mode link tracking service's LPC port so that information about objects
// which have been moved can be tracked.
//

typedef struct _LINK_TRACKING_PACKET {
    WORK_QUEUE_ITEM WorkQueueItem;
    KEVENT Event;
    NTSTATUS FinalStatus;
} LINK_TRACKING_PACKET, *PLINK_TRACKING_PACKET;

//
// Define the type for entries placed on the driver reinitialization queue.
// These entries are entered onto the tail when the driver requests that
// it be reinitialized, and removed from the head by the code that actually
// performs the reinitialization.
//

typedef struct _REINIT_PACKET {
    LIST_ENTRY ListEntry;
    PDRIVER_OBJECT DriverObject;
    PDRIVER_REINITIALIZE DriverReinitializationRoutine;
    PVOID Context;
} REINIT_PACKET, *PREINIT_PACKET;

//
// Define the type for entries placed on the driver shutdown notification queue.
// These entries represent those drivers that would like to be notified that the
// system is begin shutdown before it actually goes down.
//

typedef struct _SHUTDOWN_PACKET {
    LIST_ENTRY ListEntry;
    PDEVICE_OBJECT DeviceObject;
} SHUTDOWN_PACKET, *PSHUTDOWN_PACKET;

//
// Define the type for entries placed on the file system registration change
// notification queue.
//

typedef struct _NOTIFICATION_PACKET {
    LIST_ENTRY ListEntry;
    PDRIVER_OBJECT DriverObject;
    PDRIVER_FS_NOTIFICATION NotificationRoutine;
} NOTIFICATION_PACKET, *PNOTIFICATION_PACKET;

//
// Define I/O completion packet types.
//

typedef enum _COMPLETION_PACKET_TYPE {
    IopCompletionPacketIrp,
    IopCompletionPacketMini,
    IopCompletionPacketQuota
} COMPLETION_PACKET_TYPE, *PCOMPLETION_PACKET_TYPE;

//
// Define the type for completion packets inserted onto completion ports when
// there is no full I/O request packet that was used to perform the I/O
// operation.  This occurs when the fast I/O path is used, and when the user
// directly inserts a completion message.
//
typedef struct _IOP_MINI_COMPLETION_PACKET {

    //
    // The following unnamed structure must be exactly identical
    // to the unnamed structure used in the IRP overlay section used
    // for completion queue entries.
    //

    struct {

        //
        // List entry - used to queue the packet to completion queue, among
        // others.
        //

        LIST_ENTRY ListEntry;

        union {

            //
            // Current stack location - contains a pointer to the current
            // IO_STACK_LOCATION structure in the IRP stack.  This field
            // should never be directly accessed by drivers.  They should
            // use the standard functions.
            //

            struct _IO_STACK_LOCATION *CurrentStackLocation;

            //
            // Minipacket type.
            //

            ULONG PacketType;
        };
    };

    PVOID KeyContext;
    PVOID ApcContext;
    NTSTATUS IoStatus;
    ULONG_PTR IoStatusInformation;
} IOP_MINI_COMPLETION_PACKET, *PIOP_MINI_COMPLETION_PACKET;

//
// Define the type for a dump control block.  This structure is used to describe
// all of the data, drivers, and memory necessary to dump all of physical memory
// to the disk after a bugcheck.
//

typedef struct _MINIPORT_NODE {
    LIST_ENTRY ListEntry;
    PLDR_DATA_TABLE_ENTRY DriverEntry;
    ULONG DriverChecksum;
} MINIPORT_NODE, *PMINIPORT_NODE;

#define IO_TYPE_DCB                     0xff

#define DCB_DUMP_ENABLED                 0x01
#define DCB_SUMMARY_ENABLED              0x02
#define DCB_AUTO_REBOOT                  0x04
#define DCB_DUMP_HEADER_ENABLED          0x10
#define DCB_SUMMARY_DUMP_ENABLED         0x20
#define DCB_TRIAGE_DUMP_ENABLED          0x40
#define DCB_TRIAGE_DUMP_ACT_UPON_ENABLED 0x80

typedef struct _DUMP_CONTROL_BLOCK {
    UCHAR Type;
    CHAR Flags;
    USHORT Size;
    CCHAR NumberProcessors;
    CHAR Reserved;
    USHORT ProcessorArchitecture;
    PDUMP_STACK_CONTEXT DumpStack;
    PPHYSICAL_MEMORY_DESCRIPTOR MemoryDescriptor;
    ULONG MemoryDescriptorLength;
    PLARGE_INTEGER FileDescriptorArray;
    ULONG FileDescriptorSize;
    PULONG HeaderPage;
    PFN_NUMBER HeaderPfn;
    ULONG MajorVersion;
    ULONG MinorVersion;
    ULONG BuildNumber;
    CHAR VersionUser[32];
    ULONG HeaderSize;               // Size of dump header includes summary dump.
    LARGE_INTEGER DumpFileSize;     // Size of dump file.
    ULONG TriageDumpFlags;          // Flags for triage dump.
    PUCHAR TriageDumpBuffer;        // Buffer for triage dump.
    ULONG TriageDumpBufferSize;     // Size of triage dump buffer.
} DUMP_CONTROL_BLOCK, *PDUMP_CONTROL_BLOCK;


//
// Define the global data for the I/O system.
//

#define IOP_FIXED_SIZE_MDL_PFNS        0x17

extern KSPIN_LOCK IopDatabaseLock;
extern ERESOURCE IopDatabaseResource;
extern ERESOURCE IopSecurityResource;
extern LIST_ENTRY IopDiskFileSystemQueueHead;
extern LIST_ENTRY IopCdRomFileSystemQueueHead;
extern LIST_ENTRY IopNetworkFileSystemQueueHead;
extern LIST_ENTRY IopTapeFileSystemQueueHead;
extern LIST_ENTRY IopBootDriverReinitializeQueueHead;
extern LIST_ENTRY IopDriverReinitializeQueueHead;
extern LIST_ENTRY IopNotifyShutdownQueueHead;
extern LIST_ENTRY IopNotifyLastChanceShutdownQueueHead;
extern LIST_ENTRY IopFsNotifyChangeQueueHead;
extern KSPIN_LOCK IoStatisticsLock;
extern KSEMAPHORE IopRegistrySemaphore;
extern KSPIN_LOCK IopTimerLock;
extern LIST_ENTRY IopTimerQueueHead;
extern KDPC IopTimerDpc;
extern KTIMER IopTimer;
extern ULONG IopTimerCount;
extern ULONG IopLargeIrpStackLocations;
extern KSPIN_LOCK IopCompletionLock;

extern POBJECT_TYPE IoAdapterObjectType;
extern POBJECT_TYPE IoCompletionObjectType;
extern POBJECT_TYPE IoControllerObjectType;
extern POBJECT_TYPE IoDeviceObjectType;
extern POBJECT_TYPE IoDriverObjectType;
extern POBJECT_TYPE IoDeviceHandlerObjectType;
extern POBJECT_TYPE IoFileObjectType;
extern ULONG        IoDeviceHandlerObjectSize;

extern NPAGED_LOOKASIDE_LIST IopLargeIrpLookasideList;
extern NPAGED_LOOKASIDE_LIST IopSmallIrpLookasideList;
extern NPAGED_LOOKASIDE_LIST IopMdlLookasideList;
extern NPAGED_LOOKASIDE_LIST IopCompletionLookasideList;

extern UCHAR IopQueryOperationLength[];
extern UCHAR IopSetOperationLength[];
extern ULONG IopQueryOperationAccess[];
extern ULONG IopSetOperationAccess[];
extern UCHAR IopQuerySetAlignmentRequirement[];
extern UCHAR IopQueryFsOperationLength[];
extern UCHAR IopSetFsOperationLength[];
extern ULONG IopQueryFsOperationAccess[];
extern ULONG IopSetFsOperationAccess[];
extern UCHAR IopQuerySetFsAlignmentRequirement[];

extern UNICODE_STRING IoArcBootDeviceName;
extern UNICODE_STRING IoArcHalDeviceName;
extern PUCHAR IoLoaderArcBootDeviceName;
extern PDUMP_CONTROL_BLOCK IopDumpControlBlock;
extern ULONG IopDumpControlBlockChecksum;

extern LONG IopUniqueDeviceObjectNumber;

extern PVOID IopLinkTrackingServiceObject;
extern PKEVENT IopLinkTrackingServiceEvent;
extern KEVENT IopLinkTrackingPortObject;
extern LINK_TRACKING_PACKET IopLinkTrackingPacket;

extern PVOID IopLoaderBlock;

extern BOOLEAN IopRemoteBootCardInitialized;

extern ULONG IopLookasideIrpFloat;
extern ULONG IopLookasideIrpLimit;
extern BOOLEAN  IopVerifierOn;

extern PIO_CALL_DRIVER        pIofCallDriver;
extern PIO_COMPLETE_REQUEST   pIofCompleteRequest;
extern PIO_FREE_IRP           pIoFreeIrp;
extern PIO_ALLOCATE_IRP       pIoAllocateIrp;

//
// The following declaration cannot go in EX.H since POBJECT_TYPE is not defined
// until OB.H, which depends on EX.H.  Hence, it is not exported by the EX
// component at all.
//

extern POBJECT_TYPE ExEventObjectType;

//
// Define routines private to the I/O system.
//

VOID
IopAbortRequest(
    IN PKAPC Apc
    );

//+
//
// BOOLEAN
// IopAcquireFastLock(
//     IN PFILE_OBJECT FileObject
// )
//
// Routine Description:
//
//     This routine is invoked to acquire the fast lock for a file object.
//     This lock protects the busy indicator in the file object resource.
//
// Arguments:
//
//     FileObject - Pointer to the file object to be locked.
//
// Return Values:
//
//      FALSE - the fileobject was not locked (it was busy)
//      TRUE  - the fileobject was locked & the busy flag has been set to TRUE
//
//-

#define IopAcquireFastLock( FileObject )    \
    ( InterlockedExchange( &FileObject->Busy, (ULONG) TRUE ) == FALSE )

#define IopAcquireCancelSpinLockAtDpcLevel()    \
    ExAcquireSpinLockAtDpcLevel (&IopCancelSpinLock)

#define IopReleaseCancelSpinLockFromDpcLevel()  \
    ExReleaseSpinLockFromDpcLevel (&IopCancelSpinLock)

#define IopAllocateIrp(StackSize, ChargeQuota) \
        IoAllocateIrp((StackSize), (ChargeQuota))

#define IsIoVerifierOn()    IopVerifierOn


NTSTATUS
IopAcquireFileObjectLock(
    IN PFILE_OBJECT FileObject,
    IN KPROCESSOR_MODE RequestorMode,
    IN BOOLEAN Alertable,
    OUT PBOOLEAN Interrupted
    );


VOID
IopAllocateIrpCleanup(
    IN PFILE_OBJECT FileObject,
    IN PKEVENT EventObject OPTIONAL
    );

PIRP
IopAllocateIrpMustSucceed(
    IN CCHAR StackSize
    );

VOID
IopApcHardError(
    IN PVOID StartContext
    );

VOID
IopCancelAlertedRequest(
    IN PKEVENT Event,
    IN PIRP Irp
    );

VOID
IopCheckBackupRestorePrivilege(
    IN PACCESS_STATE AccessState,
    IN OUT PULONG CreateOptions,
    IN KPROCESSOR_MODE PreviousMode,
    IN ULONG Disposition
    );

NTSTATUS
IopCheckGetQuotaBufferValidity(
    IN PFILE_GET_QUOTA_INFORMATION QuotaBuffer,
    IN ULONG QuotaLength,
    OUT PULONG_PTR ErrorOffset
    );

VOID
IopCloseFile(
    IN PEPROCESS Process OPTIONAL,
    IN PVOID Object,
    IN ULONG GrantedAccess,
    IN ULONG ProcessHandleCount,
    IN ULONG SystemHandleCount
    );

VOID
IopCompleteUnloadOrDelete(
    IN PDEVICE_OBJECT DeviceObject,
    IN KIRQL Irql
    );

VOID
IopCompletePageWrite(
    IN PKAPC Apc,
    IN PKNORMAL_ROUTINE *NormalRoutine,
    IN PVOID *NormalContext,
    IN PVOID *SystemArgument1,
    IN PVOID *SystemArgument2
    );

VOID
IopCompleteRequest(
    IN PKAPC Apc,
    IN PKNORMAL_ROUTINE *NormalRoutine,
    IN PVOID *NormalContext,
    IN PVOID *SystemArgument1,
    IN PVOID *SystemArgument2
    );

VOID
IopConnectLinkTrackingPort(
    IN PVOID Parameter
    );

VOID
IopCreateVpb (
    IN PDEVICE_OBJECT DeviceObject
    );

VOID
IopDeallocateApc(
    IN PKAPC Apc,
    IN PKNORMAL_ROUTINE *NormalRoutine,
    IN PVOID *NormalContext,
    IN PVOID *SystemArgument1,
    IN PVOID *SystemArgument2
    );

VOID
IopDecrementDeviceObjectRef(
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN AlwaysUnload
    );

VOID
IopDeleteDriver(
    IN PVOID Object
    );

VOID
IopDeleteDevice(
    IN PVOID Object
    );

VOID
IopDeleteFile(
    IN PVOID Object
    );

VOID
IopDeleteIoCompletion(
    IN PVOID Object
    );

//+
//
// VOID
// IopDequeueThreadIrp(
//     IN PIRP Irp
//     )
//
// Routine Description:
//
//     This routine dequeues the specified I/O Request Packet (IRP) from the
//     thread IRP queue which it is currently queued.
//
//     In checked we set Flink == Blink so we can assert free's of queue'd IRPs
//
// Arguments:
//
//     Irp - Specifies the IRP that is dequeued.
//
// Return Value:
//
//     None.
//
//-

#define IopDequeueThreadIrp( Irp ) \
   { \
   RemoveEntryList( &Irp->ThreadListEntry ); \
   InitializeListHead( &Irp->ThreadListEntry ) ; \
   }


#ifdef  _WIN64
#define IopApcRoutinePresent(ApcRoutine)    ARGUMENT_PRESENT((ULONG_PTR)(ApcRoutine) & ~1)
#else
#define IopApcRoutinePresent(ApcRoutine)    ARGUMENT_PRESENT(ApcRoutine)
#endif

VOID
IopDisassociateThreadIrp(
    VOID
    );

BOOLEAN
IopDmaDispatch(
    IN PKINTERRUPT Interrupt,
    IN PVOID ServiceContext
    );

VOID
IopDropIrp(
    IN PIRP Irp,
    IN PFILE_OBJECT FileObject
    );

LONG
IopExceptionFilter(
    IN PEXCEPTION_POINTERS ExceptionPointers,
    OUT PNTSTATUS ExceptionCode
    );

VOID
IopExceptionCleanup(
    IN PFILE_OBJECT FileObject,
    IN PIRP Irp,
    IN PKEVENT EventObject OPTIONAL,
    IN PKEVENT KernelEvent OPTIONAL
    );

VOID
IopErrorLogThread(
    IN PVOID StartContext
    );

VOID
IopFreeIrpAndMdls(
    IN PIRP Irp
    );

PDEVICE_OBJECT
IopGetDeviceAttachmentBase(
    IN PDEVICE_OBJECT DeviceObject
    );

PDEVICE_OBJECT
IopGetDeviceAttachmentBaseRef(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
IopGetDriverNameFromKeyNode(
    IN HANDLE KeyHandle,
    OUT PUNICODE_STRING DriverName
    );

ULONG
IopGetDumpControlBlockCheck (
    IN PDUMP_CONTROL_BLOCK  Dcb
    );

NTSTATUS
IopGetFileName(
    IN PFILE_OBJECT FileObject,
    IN ULONG Length,
    OUT PVOID FileInformation,
    OUT PULONG ReturnedLength
    );

BOOLEAN
IopGetMountFlag(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
IopGetRegistryKeyInformation(
    IN HANDLE KeyHandle,
    OUT PKEY_FULL_INFORMATION *Information
    );

NTSTATUS
IopGetRegistryValue(
    IN HANDLE KeyHandle,
    IN PWSTR  ValueName,
    OUT PKEY_VALUE_FULL_INFORMATION *Information
    );

NTSTATUS
IopGetRegistryValues(
    IN HANDLE KeyHandle,
    IN PKEY_VALUE_FULL_INFORMATION *ValueList
    );

NTSTATUS
IopGetRegistryValues(
    IN HANDLE KeyHandle,
    IN PKEY_VALUE_FULL_INFORMATION *ValueList
    );

NTSTATUS
IopGetSetObjectId(
    IN PFILE_OBJECT FileObject,
    IN OUT PVOID Buffer,
    IN ULONG Length,
    IN ULONG OperationFlags
    );

NTSTATUS
IopGetSetSecurityObject(
    IN PVOID Object,
    IN SECURITY_OPERATION_CODE OperationCode,
    IN PSECURITY_INFORMATION SecurityInformation,
    IN OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN OUT PULONG CapturedLength,
    IN OUT PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
    IN POOL_TYPE PoolType,
    IN PGENERIC_MAPPING GenericMapping
    );

NTSTATUS
IopGetVolumeId(
    IN PFILE_OBJECT FileObject,
    IN OUT PLINK_TRACKING_INFORMATION ObjectId,
    IN ULONG Length
    );

VOID
IopHardErrorThread(
    PVOID StartContext
    );

//++
//
// VOID
// IopInitializeIrp(
//     IN OUT PIRP Irp,
//     IN USHORT PacketSize,
//     IN CCHAR StackSize
//     )
//
// Routine Description:
//
//     Initializes an IRP.
//
// Arguments:
//
//     Irp - a pointer to the IRP to initialize.
//
//     PacketSize - length, in bytes, of the IRP.
//
//     StackSize - Number of stack locations in the IRP.
//
// Return Value:
//
//     None.
//
//--

#define IopInitializeIrp( Irp, PacketSize, StackSize ) {          \
    RtlZeroMemory( (Irp), (PacketSize) );                         \
    (Irp)->Type = (CSHORT) IO_TYPE_IRP;                           \
    (Irp)->Size = (USHORT) ((PacketSize));                        \
    (Irp)->StackCount = (CCHAR) ((StackSize));                    \
    (Irp)->CurrentLocation = (CCHAR) ((StackSize) + 1);           \
    (Irp)->ApcEnvironment = KeGetCurrentApcEnvironment();         \
    InitializeListHead (&(Irp)->ThreadListEntry);                 \
    (Irp)->Tail.Overlay.CurrentStackLocation =                    \
        ((PIO_STACK_LOCATION) ((UCHAR *) (Irp) +                  \
            sizeof( IRP ) +                                       \
            ( (StackSize) * sizeof( IO_STACK_LOCATION )))); }

VOID
IopInitializeResourceMap (
    PLOADER_PARAMETER_BLOCK LoaderBlock
    );

VOID
IopInsertRemoveDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN Insert
    );

NTSTATUS
IopInvalidDeviceRequest(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
IopInvalidateVolumesForDevice(
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
IopIsSameMachine(
    IN PFILE_OBJECT SourceFile,
    IN HANDLE TargetFile
    );

NTSTATUS
IopLoadDriver(
    IN HANDLE KeyHandle,
    IN BOOLEAN CheckForSafeBoot
    );

VOID
IopLoadFileSystemDriver(
    IN PDEVICE_OBJECT DeviceObject
    );

VOID
IopLoadUnloadDriver(
    IN PVOID Parameter
    );

NTSTATUS
IopLookupBusStringFromID (
    IN  HANDLE KeyHandle,
    IN  INTERFACE_TYPE InterfaceType,
    OUT PWCHAR Buffer,
    IN  ULONG Length,
    OUT PULONG BusFlags OPTIONAL
    );

NTSTATUS
IopMountVolume(
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN AllowRawMount,
    IN BOOLEAN DeviceLockAlreadyHeld,
    IN BOOLEAN Alertable
    );

BOOLEAN
IopNotifyPnpWhenChainDereferenced(
    IN PDEVICE_OBJECT *PhysicalDeviceObjects,
    IN ULONG DeviceObjectCount,
    IN BOOLEAN Query,
    OUT PDEVICE_OBJECT *VetoingDevice
    );

NTSTATUS
IopOpenLinkOrRenameTarget(
    OUT PHANDLE TargetHandle,
    IN PIRP Irp,
    IN PVOID RenameBuffer,
    IN PFILE_OBJECT FileObject
    );

NTSTATUS
IopOpenRegistryKey(
    OUT PHANDLE Handle,
    IN HANDLE BaseHandle OPTIONAL,
    IN PUNICODE_STRING KeyName,
    IN ACCESS_MASK DesiredAccess,
    IN BOOLEAN Create
    );

NTSTATUS
IopParseDevice(
    IN PVOID ParseObject,
    IN PVOID ObjectType,
    IN PACCESS_STATE AccessState,
    IN KPROCESSOR_MODE AccessMode,
    IN ULONG Attributes,
    IN OUT PUNICODE_STRING CompleteName,
    IN OUT PUNICODE_STRING RemainingName,
    IN OUT PVOID Context OPTIONAL,
    IN PSECURITY_QUALITY_OF_SERVICE SecurityQos OPTIONAL,
    OUT PVOID *Object
    );

NTSTATUS
IopParseFile(
    IN PVOID ParseObject,
    IN PVOID ObjectType,
    IN PACCESS_STATE AccessState,
    IN KPROCESSOR_MODE AccessMode,
    IN ULONG Attributes,
    IN OUT PUNICODE_STRING CompleteName,
    IN OUT PUNICODE_STRING RemainingName,
    IN OUT PVOID Context OPTIONAL,
    IN PSECURITY_QUALITY_OF_SERVICE SecurityQos OPTIONAL,
    OUT PVOID *Object
    );

BOOLEAN
IopProtectSystemPartition(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

NTSTATUS
IopQueryName(
    IN PVOID Object,
    IN BOOLEAN HasObjectName,
    OUT POBJECT_NAME_INFORMATION ObjectNameInfo,
    IN ULONG Length,
    OUT PULONG ReturnLength
    );

NTSTATUS
IopQueryXxxInformation(
    IN PFILE_OBJECT FileObject,
    IN ULONG InformationClass,
    IN ULONG Length,
    OUT PVOID Information,
    OUT PULONG ReturnedLength,
    IN BOOLEAN FileInformation
    );

//+
// VOID
// IopQueueThreadIrp(
//     IN PIRP Irp
//     )
//
// Routine Description:
//
//     This routine queues the specified I/O Request Packet (IRP) to the thread
//     whose TCB address is stored in the packet.
//
// Arguments:
//
//     Irp - Supplies the IRP to be queued for the specified thread.
//
// Return Value:
//
//     None.
//
//-

#define IopQueueThreadIrp( Irp ) {                      \
    KIRQL irql;                                         \
    KeRaiseIrql( APC_LEVEL, &irql );                    \
    InsertHeadList( &Irp->Tail.Overlay.Thread->IrpList, \
                    &Irp->ThreadListEntry );            \
    KeLowerIrql( irql );                                \
    }

VOID
IopQueueWorkRequest(
    IN PIRP Irp
    );

VOID
IopRaiseHardError(
    IN PVOID NormalContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

VOID
IopRaiseInformationalHardError(
    IN PVOID NormalContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

VOID
IopReadyDeviceObjects(
    IN PDRIVER_OBJECT DriverObject
    );

//+
//
// VOID
// IopReleaseFileObjectLock(
//     IN PFILE_OBJECT FileObject
// )
//
// Routine Description:
//
//     This routine is invoked to release ownership of the file object lock.
//
// Arguments:
//
//     FileObject - Pointer to the file object whose ownership is to be
//         released.
//
// Return Value:
//
//     None.
//
//-

#define IopReleaseFileObjectLock( FileObject ) {    \
    ULONG Result;                                   \
    Result = InterlockedExchange( &FileObject->Busy, FALSE ); \
    ASSERT(Result != FALSE);                        \
    if (FileObject->Waiters != 0) {                 \
        KeSetEvent( &FileObject->Lock, 0, FALSE );  \
    }                                               \
}

#if _WIN32_WINNT >= 0x0500
NTSTATUS
IopSendMessageToTrackService(
    IN PLINK_TRACKING_INFORMATION SourceVolumeId,
    IN PFILE_OBJECTID_BUFFER SourceObjectId,
    IN PFILE_TRACKING_INFORMATION TargetObjectInformation
    );
#endif

NTSTATUS
IopSetEaOrQuotaInformationFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID Buffer,
    IN ULONG Length,
    IN BOOLEAN SetEa
    );

NTSTATUS
IopSetRemoteLink(
    IN PFILE_OBJECT FileObject,
    IN PFILE_OBJECT DestinationFileObject OPTIONAL,
    IN PFILE_TRACKING_INFORMATION FileInformation OPTIONAL
    );

VOID
IopStartApcHardError(
    IN PVOID StartContext
    );

NTSTATUS
IopSynchronousApiServiceTail(
    IN NTSTATUS ReturnedStatus,
    IN PKEVENT Event,
    IN PIRP Irp,
    IN KPROCESSOR_MODE RequestorMode,
    IN PIO_STATUS_BLOCK LocalIoStatus,
    OUT PIO_STATUS_BLOCK IoStatusBlock
    );

NTSTATUS
IopSynchronousServiceTail(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN DeferredIoCompletion,
    IN KPROCESSOR_MODE RequestorMode,
    IN BOOLEAN SynchronousIo,
    IN TRANSFER_TYPE TransferType
    );

VOID
IopTimerDispatch(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

NTSTATUS
IopTrackLink(
    IN PFILE_OBJECT FileObject,
    IN OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PFILE_TRACKING_INFORMATION FileInformation,
    IN ULONG Length,
    IN PKEVENT Event,
    IN KPROCESSOR_MODE RequestorMode
    );


VOID
IopUserCompletion(
    IN PKAPC Apc,
    IN PKNORMAL_ROUTINE *NormalRoutine,
    IN PVOID *NormalContext,
    IN PVOID *SystemArgument1,
    IN PVOID *SystemArgument2
    );

NTSTATUS
IopXxxControlFile(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG IoControlCode,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN BOOLEAN DeviceIoControl
    );

NTSTATUS
IopReportResourceUsage(
    IN PUNICODE_STRING DriverClassName OPTIONAL,
    IN PDRIVER_OBJECT DriverObject,
    IN PCM_RESOURCE_LIST DriverList OPTIONAL,
    IN ULONG DriverListSize OPTIONAL,
    IN PDEVICE_OBJECT DeviceObject OPTIONAL,
    IN PCM_RESOURCE_LIST DeviceList OPTIONAL,
    IN ULONG DeviceListSize OPTIONAL,
    IN BOOLEAN OverrideConflict,
    OUT PBOOLEAN ConflictDetected
    );

NTSTATUS
IopAddRemoteBootValuesToRegistry (
    PLOADER_PARAMETER_BLOCK LoaderBlock
    );

NTSTATUS
IopStartNetworkForRemoteBoot (
    PLOADER_PARAMETER_BLOCK LoaderBlock
    );

#if defined(REMOTE_BOOT)
VOID
IopShutdownCsc (
    VOID
    );
#endif

NTSTATUS
IopStartTcpIpForRemoteBoot (
    PLOADER_PARAMETER_BLOCK LoaderBlock
    );

BOOLEAN
IopIsRemoteBootCard(
    IN PDEVICE_NODE DeviceNode,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN PWCHAR HwIds
    );

NTSTATUS
IopSetupRemoteBootCard(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN HANDLE UniqueIdHandle,
    IN PUNICODE_STRING UnicodeDeviceInstance
    );

BOOLEAN
IopSafebootDriverLoad(
    PUNICODE_STRING DriverId
    );

PSECURITY_DESCRIPTOR
IopCreateDefaultDeviceSecurityDescriptor(
    IN DEVICE_TYPE DeviceType,
    IN ULONG DeviceCharacteristics,
    IN BOOLEAN DeviceHasName,
    IN PUCHAR Buffer,
    OUT PACL *AllocatedAcl,
    OUT PSECURITY_INFORMATION SecurityInformation OPTIONAL
    );

VOID
IopDoNameTransmogrify(
    IN PIRP Irp,
    IN PFILE_OBJECT FileObject,
    IN PREPARSE_DATA_BUFFER ReparseBuffer
    );

NTSTATUS
IopQueryDeviceCapabilities(
    IN PDEVICE_NODE DeviceNode,
    OUT PDEVICE_CAPABILITIES Capabilities
    );

//
// dump support routines
//
//Used for error logging
NTSTATUS
IopLogErrorEvent(
    IN ULONG            SequenceNumber,
    IN ULONG            UniqueErrorValue,
    IN NTSTATUS         FinalStatus,
    IN NTSTATUS         SpecificIOStatus,
    IN ULONG            LengthOfInsert1,
    IN PWCHAR           Insert1,
    IN ULONG            LengthOfInsert2,
    IN PWCHAR           Insert2
    );

BOOLEAN
IopConfigureCrashDump(
    IN HANDLE HandlePagingFile
    );

VOID
IopUpdateOtherOperationCount(
    VOID
    );

VOID
IopUpdateReadOperationCount(
    VOID
    );

VOID
IopUpdateWriteOperationCount(
    VOID
    );

VOID
IopUpdateOtherTransferCount(
    IN ULONG TransferCount
    );

VOID
IopUpdateReadTransferCount(
    IN ULONG TransferCount
    );

VOID
IopUpdateWriteTransferCount(
    IN ULONG TransferCount
    );

NTSTATUS
FASTCALL
IopfCallDriver(
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp
    );

VOID
FASTCALL
IopfCompleteRequest(
    IN  PIRP    Irp,
    IN  CCHAR   PriorityBost
    );


PIRP
IopAllocateIrpPrivate(
    IN  CCHAR   StackSize,
    IN  BOOLEAN ChargeQuota
    );

VOID
IopFreeIrp(
    IN  PIRP    Irp
    );

PVOID
IopAllocateErrorLogEntry(
    IN PDEVICE_OBJECT deviceObject,
    IN PDRIVER_OBJECT driverObject,
    IN UCHAR EntrySize
    );
#endif // _IOP_
