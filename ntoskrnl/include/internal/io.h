/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/include/internal/io.h
 * PURPOSE:         Internal header for the I/O Manager
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

#include "ntdddisk.h"

//
// Define this if you want debugging support
//
#define _IO_DEBUG_                                      0x00

//
// These define the Debug Masks Supported
//
#define IO_IRP_DEBUG                                    0x01
#define IO_FILE_DEBUG                                   0x02
#define IO_API_DEBUG                                    0x04
#define IO_CTL_DEBUG                                    0x08

//
// Debug/Tracing support
//
#if _IO_DEBUG_
#ifdef NEW_DEBUG_SYSTEM_IMPLEMENTED // enable when Debug Filters are implemented
#define IOTRACE(x, ...)                                     \
    {                                                       \
        DbgPrintEx("%s [%.16s] - ",                         \
                   __FUNCTION__,                            \
                   PsGetCurrentProcess()->ImageFileName);   \
        DbgPrintEx(__VA_ARGS__);                            \
    }
#else
#define IOTRACE(x, ...)                                     \
    if (x & IopTraceLevel)                                  \
    {                                                       \
        DbgPrint("%s [%.16s] - ",                           \
                 __FUNCTION__,                              \
                 PsGetCurrentProcess()->ImageFileName);     \
        DbgPrint(__VA_ARGS__);                              \
    }
#endif
#else
#define IOTRACE(x, fmt, ...) DPRINT(fmt, ##__VA_ARGS__)
#endif

//
// Registry path to the enumeration root key
//
#define ENUM_ROOT L"\\Registry\\Machine\\System\\CurrentControlSet\\Enum"

//
// Returns the type of METHOD_ used in this IOCTL
//
#define IO_METHOD_FROM_CTL_CODE(c)                      (c & 0x00000003)

//
// Bugcheck codes for RAM disk booting
//
//
// No LoaderXIPRom descriptor was found in the loader memory list
//
#define RD_NO_XIPROM_DESCRIPTOR  1
//
// Unable to open the RAM disk driver (ramdisk.sys or \Device\Ramdisk)
//
#define RD_NO_RAMDISK_DRIVER     2
//
// FSCTL_CREATE_RAM_DISK failed
//
#define RD_FSCTL_FAILED          3
//
// Unable to create GUID string from binary GUID
//
#define RD_GUID_CONVERT_FAILED   4
//
// Unable to create symbolic link pointing to the RAM disk device
//
#define RD_SYMLINK_CREATE_FAILED 5
//
// Unable to create system root path when creating the RAM disk
//
#define RD_SYSROOT_INIT_FAILED 6

//
// Max traversal of reparse points for a single open in IoParseDevice
//
#define IOP_MAX_REPARSE_TRAVERSAL 0x20

//
// Private flags for IoCreateFile / IoParseDevice
//
#define IOP_USE_TOP_LEVEL_DEVICE_HINT       0x01
#define IOP_CREATE_FILE_OBJECT_EXTENSION    0x02


typedef struct _FILE_OBJECT_EXTENSION
{
    PDEVICE_OBJECT TopDeviceObjectHint;
    PVOID FilterContext;

} FILE_OBJECT_EXTENSION, *PFILE_OBJECT_EXTENSION;



//
// We can call the Ob Inlined API, it's the same thing
//
#define IopAllocateMdlFromLookaside                     \
    ObpAllocateObjectCreateInfoBuffer
#define IopFreeMdlFromLookaside                         \
    ObpFreeCapturedAttributes

//
// Determines if the IRP is Synchronous
//
#define IsIrpSynchronous(Irp, FileObject)               \
    ((Irp->Flags & IRP_SYNCHRONOUS_API)  ||             \
     (!(FileObject) ?                                   \
        FALSE :                                         \
        FileObject->Flags & FO_SYNCHRONOUS_IO))         \

//
// Returns the internal Device Object Extension
//
#define IoGetDevObjExtension(DeviceObject)              \
    ((PEXTENDED_DEVOBJ_EXTENSION)                       \
     (DeviceObject->DeviceObjectExtension))             \

//
// Returns the internal Driver Object Extension
//
#define IoGetDrvObjExtension(DriverObject)              \
    ((PEXTENDED_DRIVER_EXTENSION)                       \
     (DriverObject->DriverExtension))                   \

/*
 * VOID
 * IopDeviceNodeSetFlag(
 *   PDEVICE_NODE DeviceNode,
 *   ULONG Flag);
 */
#define IopDeviceNodeSetFlag(DeviceNode, Flag)          \
    ((DeviceNode)->Flags |= (Flag))

/*
 * VOID
 * IopDeviceNodeClearFlag(
 *   PDEVICE_NODE DeviceNode,
 *   ULONG Flag);
 */
#define IopDeviceNodeClearFlag(DeviceNode, Flag)        \
    ((DeviceNode)->Flags &= ~(Flag))

/*
 * BOOLEAN
 * IopDeviceNodeHasFlag(
 *   PDEVICE_NODE DeviceNode,
 *   ULONG Flag);
 */
#define IopDeviceNodeHasFlag(DeviceNode, Flag)          \
    (((DeviceNode)->Flags & (Flag)) > 0)

/*
 * VOID
 * IopDeviceNodeSetUserFlag(
 *   PDEVICE_NODE DeviceNode,
 *   ULONG UserFlag);
 */
#define IopDeviceNodeSetUserFlag(DeviceNode, UserFlag)  \
    ((DeviceNode)->UserFlags |= (UserFlag))

/*
 * VOID
 * IopDeviceNodeClearUserFlag(
 *   PDEVICE_NODE DeviceNode,
 *   ULONG UserFlag);
 */
#define IopDeviceNodeClearUserFlag(DeviceNode, UserFlag)\
    ((DeviceNode)->UserFlags &= ~(UserFlag))

/*
 * BOOLEAN
 * IopDeviceNodeHasUserFlag(
 *   PDEVICE_NODE DeviceNode,
 *   ULONG UserFlag);
 */
#define IopDeviceNodeHasUserFlag(DeviceNode, UserFlag)  \
    (((DeviceNode)->UserFlags & (UserFlag)) > 0)

 /*
 * VOID
 * IopDeviceNodeSetProblem(
 *   PDEVICE_NODE DeviceNode,
 *   ULONG Problem);
 */
#define IopDeviceNodeSetProblem(DeviceNode, Problem)    \
    ((DeviceNode)->Problem |= (Problem))

/*
 * VOID
 * IopDeviceNodeClearProblem(
 *   PDEVICE_NODE DeviceNode,
 *   ULONG Problem);
 */
#define IopDeviceNodeClearProblem(DeviceNode, Problem)  \
    ((DeviceNode)->Problem &= ~(Problem))

/*
 * BOOLEAN
 * IopDeviceNodeHasProblem(
 *   PDEVICE_NODE DeviceNode,
 *   ULONG Problem);
 */
#define IopDeviceNodeHasProblem(DeviceNode, Problem)    \
    (((DeviceNode)->Problem & (Problem)) > 0)

/*
 * VOID
 * IopInitDeviceTreeTraverseContext(
 *   PDEVICETREE_TRAVERSE_CONTEXT DeviceTreeTraverseContext,
 *   PDEVICE_NODE DeviceNode,
 *   DEVICETREE_TRAVERSE_ROUTINE Action,
 *   PVOID Context);
 */
#define IopInitDeviceTreeTraverseContext(               \
    _DeviceTreeTraverseContext, _DeviceNode, _Action,   \
    _Context) {                                         \
    (_DeviceTreeTraverseContext)->FirstDeviceNode =     \
        (_DeviceNode);                                  \
    (_DeviceTreeTraverseContext)->Action = (_Action);   \
    (_DeviceTreeTraverseContext)->Context = (_Context); }

/*
 * BOOLEAN
 * IopIsValidPhysicalDeviceObject(
 *   IN PDEVICE_OBJECT PhysicalDeviceObject);
 */
#define IopIsValidPhysicalDeviceObject(PhysicalDeviceObject)                                                            \
        (((PEXTENDED_DEVOBJ_EXTENSION)PhysicalDeviceObject) &&                                                          \
        (((PEXTENDED_DEVOBJ_EXTENSION)PhysicalDeviceObject->DeviceObjectExtension)->DeviceNode) &&                      \
        (((PEXTENDED_DEVOBJ_EXTENSION)PhysicalDeviceObject->DeviceObjectExtension)->DeviceNode->Flags & DNF_ENUMERATED))

//
// Device List Operations
//
typedef enum _IOP_DEVICE_LIST_OPERATION
{
    IopRemove,
    IopAdd
} IOP_DEVICE_LIST_OPERATION, *PIOP_DEVICE_LIST_OPERATION;

//
// Transfer statistics
//
typedef enum _IOP_TRANSFER_TYPE
{
    IopReadTransfer,
    IopWriteTransfer,
    IopOtherTransfer
} IOP_TRANSFER_TYPE, *PIOP_TRANSFER_TYPE;

//
// Packet Types when piggybacking on the IRP Overlay
//
typedef enum _COMPLETION_PACKET_TYPE
    {
    IopCompletionPacketIrp,
    IopCompletionPacketMini,
    IopCompletionPacketQuota
} COMPLETION_PACKET_TYPE, *PCOMPLETION_PACKET_TYPE;

//
// Special version of the IRP Overlay used to optimize I/O completion
// by not using up a separate structure.
//
typedef struct _IOP_MINI_COMPLETION_PACKET
{
    struct
    {
        LIST_ENTRY ListEntry;
        union
        {
            struct _IO_STACK_LOCATION *CurrentStackLocation;
            ULONG PacketType;
        };
    };
    PVOID KeyContext;
    PVOID ApcContext;
    NTSTATUS IoStatus;
    ULONG_PTR IoStatusInformation;
} IOP_MINI_COMPLETION_PACKET, *PIOP_MINI_COMPLETION_PACKET;

//
// I/O Completion Context for IoSetIoCompletionRoutineEx
//
typedef struct _IO_UNLOAD_SAFE_COMPLETION_CONTEXT
{
    PDEVICE_OBJECT DeviceObject;
    PVOID Context;
    PIO_COMPLETION_ROUTINE CompletionRoutine;
} IO_UNLOAD_SAFE_COMPLETION_CONTEXT, *PIO_UNLOAD_SAFE_COMPLETION_CONTEXT;

//
// I/O Wrapper around the Executive Work Item
//
typedef struct _IO_WORKITEM
{
    WORK_QUEUE_ITEM Item;
    PDEVICE_OBJECT DeviceObject;
    PIO_WORKITEM_ROUTINE WorkerRoutine;
    PVOID Context;
} IO_WORKITEM;

//
// I/O Wrapper around the Kernel Interrupt
//
typedef struct _IO_INTERRUPT
{
    KINTERRUPT FirstInterrupt;
    PKINTERRUPT Interrupt[MAXIMUM_PROCESSORS];
    KSPIN_LOCK SpinLock;
} IO_INTERRUPT, *PIO_INTERRUPT;

//
// I/O Error Log Packet Header
//
typedef struct _ERROR_LOG_ENTRY
{
    CSHORT Type;
    CSHORT Size;
    LIST_ENTRY ListEntry;
    PDEVICE_OBJECT DeviceObject;
    PDRIVER_OBJECT DriverObject;
    LARGE_INTEGER TimeStamp;
} ERROR_LOG_ENTRY, *PERROR_LOG_ENTRY;

//
// To simplify matters, the kernel is made to support both the checked and free
// version of the I/O Remove Lock in the same binary. This structure includes
// both, since the DDK has the structure with a compile-time #ifdef.
//
typedef struct _EXTENDED_IO_REMOVE_LOCK
{
    IO_REMOVE_LOCK_COMMON_BLOCK Common;
    IO_REMOVE_LOCK_DBG_BLOCK Dbg;
} EXTENDED_IO_REMOVE_LOCK, *PEXTENDED_IO_REMOVE_LOCK;

//
// Dummy File Object used inside the Open Packet so that OB knows how to
// deal with the Object Pointer even though it's not a real file.
//
typedef struct _DUMMY_FILE_OBJECT
{
    OBJECT_HEADER ObjectHeader;
    CHAR FileObjectBody[sizeof(FILE_OBJECT)];
} DUMMY_FILE_OBJECT, *PDUMMY_FILE_OBJECT;

//
// Open packet used as a context for Device/File parsing so that the parse
// routine can know what operation is being requested.
//
typedef struct _OPEN_PACKET
{
    CSHORT Type;
    CSHORT Size;
    PFILE_OBJECT FileObject;
    NTSTATUS FinalStatus;
    ULONG_PTR Information;
    ULONG ParseCheck;
    PFILE_OBJECT RelatedFileObject;
    OBJECT_ATTRIBUTES OriginalAttributes;
    LARGE_INTEGER AllocationSize;
    ULONG CreateOptions;
    USHORT FileAttributes;
    USHORT ShareAccess;
    PVOID EaBuffer;
    ULONG EaLength;
    ULONG Options;
    ULONG Disposition;
    PFILE_BASIC_INFORMATION BasicInformation;
    PFILE_NETWORK_OPEN_INFORMATION NetworkInformation;
    CREATE_FILE_TYPE CreateFileType;
    PVOID ExtraCreateParameters;
    BOOLEAN Override;
    BOOLEAN QueryOnly;
    BOOLEAN DeleteOnly;
    BOOLEAN FullAttributes;
    PDUMMY_FILE_OBJECT LocalFileObject;
    BOOLEAN TraversedMountPoint;
    ULONG InternalFlags;
    PDEVICE_OBJECT TopDeviceObjectHint;
} OPEN_PACKET, *POPEN_PACKET;

//
// Boot Driver List Entry
//
typedef struct _DRIVER_INFORMATION
{
    LIST_ENTRY Link;
    PDRIVER_OBJECT DriverObject;
    PBOOT_DRIVER_LIST_ENTRY DataTableEntry;
    HANDLE ServiceHandle;
    USHORT TagPosition;
    ULONG Failed;
    ULONG Processed;
    NTSTATUS Status;
} DRIVER_INFORMATION, *PDRIVER_INFORMATION;

//
// List of Bus Type GUIDs
//
typedef struct _IO_BUS_TYPE_GUID_LIST
{
    ULONG GuidCount;
    FAST_MUTEX Lock;
    GUID Guids[1];
} IO_BUS_TYPE_GUID_LIST, *PIO_BUS_TYPE_GUID_LIST;
extern PIO_BUS_TYPE_GUID_LIST IopBusTypeGuidList;

//
// Shutdown entry for registed devices
//
typedef struct _SHUTDOWN_ENTRY
{
    LIST_ENTRY ShutdownList;
    PDEVICE_OBJECT DeviceObject;
} SHUTDOWN_ENTRY, *PSHUTDOWN_ENTRY;

//
// F/S Notification entry for registered File Systems
//
typedef struct _FS_CHANGE_NOTIFY_ENTRY
{
    LIST_ENTRY FsChangeNotifyList;
    PDRIVER_OBJECT DriverObject;
    PDRIVER_FS_NOTIFICATION FSDNotificationProc;
} FS_CHANGE_NOTIFY_ENTRY, *PFS_CHANGE_NOTIFY_ENTRY;

//
// Driver (Boot) Re-Initialization Entry
//
typedef struct _DRIVER_REINIT_ITEM
{
    LIST_ENTRY ItemEntry;
    PDRIVER_OBJECT DriverObject;
    PDRIVER_REINITIALIZE ReinitRoutine;
    PVOID Context;
} DRIVER_REINIT_ITEM, *PDRIVER_REINIT_ITEM;

//
// Called on every visit of a node during a preorder-traversal of the device
// node tree.
// If the routine returns STATUS_UNSUCCESSFUL the traversal will stop and
// STATUS_SUCCESS is returned to the caller who initiated the tree traversal.
// Any other returned status code will be returned to the caller. If a status
// code that indicates an error (other than STATUS_UNSUCCESSFUL) is returned,
// the traversal is stopped immediately and the status code is returned to
// the caller.
//
typedef
NTSTATUS
(*DEVICETREE_TRAVERSE_ROUTINE)(
    IN PDEVICE_NODE DeviceNode,
    IN PVOID Context
);

//
// Context information for traversing the device tree
//
typedef struct _DEVICETREE_TRAVERSE_CONTEXT
{
    //
    // Current device node during a traversal
    //
    PDEVICE_NODE DeviceNode;

    //
    // Initial device node where we start the traversal
    //
    PDEVICE_NODE FirstDeviceNode;

    //
    // Action routine to be called for every device node
    //
    DEVICETREE_TRAVERSE_ROUTINE Action;

    //
    // Context passed to the action routine
    //
    PVOID Context;
} DEVICETREE_TRAVERSE_CONTEXT, *PDEVICETREE_TRAVERSE_CONTEXT;

//
// Reserve IRP allocator
// Used for read paging IOs in low-memory situations
//
typedef struct _RESERVE_IRP_ALLOCATOR
{
    PIRP ReserveIrp;
    volatile LONG ReserveIrpInUse;
    KEVENT WaitEvent;
    CCHAR StackSize;
} RESERVE_IRP_ALLOCATOR, *PRESERVE_IRP_ALLOCATOR;

//
// Type selection for IopCreateSecurityDescriptorPerType()
//
typedef enum _SECURITY_DESCRIPTOR_TYPE
{
    RestrictedPublic = 1,
    UnrestrictedPublic,
    RestrictedPublicOpen,
    UnrestrictedPublicOpen,
    SystemDefault,
} SECURITY_DESCRIPTOR_TYPE, *PSECURITY_DESCRIPTOR_TYPE;

//
// Action types and data for PiQueueDeviceAction()
//
typedef enum _DEVICE_ACTION
{
    PiActionEnumDeviceTree,
    PiActionEnumRootDevices,
    PiActionResetDevice,
    PiActionAddBootDevices,
    PiActionStartDevice,
    PiActionQueryState,
} DEVICE_ACTION;

//
// Resource code
//
ULONG
NTAPI
PnpDetermineResourceListSize(IN PCM_RESOURCE_LIST ResourceList);

NTSTATUS
NTAPI
IopAssignDeviceResources(
    IN PDEVICE_NODE DeviceNode
);

NTSTATUS
NTAPI
IopFixupResourceListWithRequirements(
    IN PIO_RESOURCE_REQUIREMENTS_LIST RequirementsList,
    OUT PCM_RESOURCE_LIST *ResourceList
);

NTSTATUS
NTAPI
IopDetectResourceConflict(
     IN PCM_RESOURCE_LIST ResourceList,
     IN BOOLEAN Silent,
     OUT OPTIONAL PCM_PARTIAL_RESOURCE_DESCRIPTOR ConflictingDescriptor
);

//
// PNP Routines
//
NTSTATUS
NTAPI
PipCallDriverAddDevice(
    IN PDEVICE_NODE DeviceNode,
    IN BOOLEAN LoadDriver,
    IN PDRIVER_OBJECT DriverObject
);

CODE_SEG("INIT")
NTSTATUS
NTAPI
IopInitializePlugPlayServices(
    VOID
);

BOOLEAN
NTAPI
PpInitSystem(
    VOID
);

VOID
PnpInit2(
    VOID
);

VOID
IopInitDriverImplementation(
    VOID
);

NTSTATUS
IopGetSystemPowerDeviceObject(
    IN PDEVICE_OBJECT *DeviceObject
);

PDEVICE_NODE
PipAllocateDeviceNode(
    IN PDEVICE_OBJECT PhysicalDeviceObject
);

VOID
PiInsertDevNode(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ PDEVICE_NODE ParentNode);

PNP_DEVNODE_STATE
PiSetDevNodeState(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ PNP_DEVNODE_STATE NewState);

VOID
PiSetDevNodeProblem(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ UINT32 Problem);

VOID
PiClearDevNodeProblem(
    _In_ PDEVICE_NODE DeviceNode);

NTSTATUS
IopFreeDeviceNode(
    IN PDEVICE_NODE DeviceNode
);

NTSTATUS
NTAPI
IopQueryDeviceCapabilities(PDEVICE_NODE DeviceNode,
                           PDEVICE_CAPABILITIES DeviceCaps);

NTSTATUS
IopSynchronousCall(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIO_STACK_LOCATION IoStackLocation,
    OUT PVOID *Information
);

NTSTATUS
NTAPI
IopInitiatePnpIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIO_STATUS_BLOCK IoStatusBlock,
    IN UCHAR MinorFunction,
    IN PIO_STACK_LOCATION Stack
);

PDEVICE_NODE
FASTCALL
IopGetDeviceNode(
    IN PDEVICE_OBJECT DeviceObject
);

NTSTATUS
IoCreateDriverList(
    VOID
);

NTSTATUS
IoDestroyDriverList(
    VOID
);

CODE_SEG("INIT")
NTSTATUS
IopInitPlugPlayEvents(VOID);

NTSTATUS
IopQueueDeviceChangeEvent(
    _In_ const GUID *EventGuid,
    _In_ const GUID *InterfaceClassGuid,
    _In_ PUNICODE_STRING SymbolicLinkName);

NTSTATUS
IopQueueTargetDeviceEvent(
    _In_ const GUID *Guid,
    _In_ PUNICODE_STRING DeviceIds);

NTSTATUS
IopQueueDeviceInstallEvent(
    _In_ const GUID *Guid,
    _In_ PUNICODE_STRING DeviceId);

NTSTATUS
NTAPI
IopOpenRegistryKeyEx(
    PHANDLE KeyHandle,
    HANDLE ParentKey,
    PUNICODE_STRING Name,
    ACCESS_MASK DesiredAccess);

NTSTATUS
NTAPI
IopGetRegistryValue(
    IN HANDLE Handle,
    IN PWSTR ValueName,
    OUT PKEY_VALUE_FULL_INFORMATION *Information
);

NTSTATUS
NTAPI
IopCreateRegistryKeyEx(
    OUT PHANDLE Handle,
    IN HANDLE BaseHandle OPTIONAL,
    IN PUNICODE_STRING KeyName,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG CreateOptions,
    OUT PULONG Disposition OPTIONAL
);


NTSTATUS
IopTraverseDeviceTree(
    PDEVICETREE_TRAVERSE_CONTEXT Context);

NTSTATUS
NTAPI
IopCreateDeviceKeyPath(
    IN PCUNICODE_STRING RegistryPath,
    IN ULONG CreateOptions,
    OUT PHANDLE Handle);

//
// PnP Routines
//
CODE_SEG("INIT")
NTSTATUS
NTAPI
IopUpdateRootKey(
    VOID
);

CODE_SEG("INIT")
NTSTATUS
NTAPI
PiInitCacheGroupInformation(
    VOID
);

USHORT
NTAPI
PpInitGetGroupOrderIndex(
    IN HANDLE ServiceHandle
);

USHORT
NTAPI
PipGetDriverTagPriority(
    IN HANDLE ServiceHandle
);

NTSTATUS
NTAPI
PnpRegMultiSzToUnicodeStrings(
    IN PKEY_VALUE_FULL_INFORMATION KeyValueInformation,
    OUT PUNICODE_STRING *UnicodeStringList,
    OUT PULONG UnicodeStringCount
);

BOOLEAN
NTAPI
PnpRegSzToString(
    IN PWCHAR RegSzData,
    IN ULONG RegSzLength,
    OUT PUSHORT StringLength OPTIONAL
);

VOID
PiSetDevNodeText(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ HANDLE InstanceKey);

//
// Initialization Routines
//
CODE_SEG("INIT")
NTSTATUS
NTAPI
IopCreateArcNames(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
);

CODE_SEG("INIT")
NTSTATUS
NTAPI
IopReassignSystemRoot(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    OUT PANSI_STRING NtBootPath
);

CODE_SEG("INIT")
BOOLEAN
NTAPI
IoInitSystem(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
);

BOOLEAN
IopVerifyDiskSignature(
    _In_ PDRIVE_LAYOUT_INFORMATION_EX DriveLayout,
    _In_ PARC_DISK_SIGNATURE ArcDiskSignature,
    _Out_ PULONG Signature);

BOOLEAN
NTAPI
IoInitializeCrashDump(
    IN HANDLE PageFileHandle
);

CODE_SEG("INIT")
VOID
PiInitializeNotifications(
    VOID);

//
// Device/Volume Routines
//
VOID
NTAPI
IopReadyDeviceObjects(
    IN PDRIVER_OBJECT Driver
);

PVPB
NTAPI
IopCheckVpbMounted(
    IN POPEN_PACKET OpenPacket,
    IN PDEVICE_OBJECT DeviceObject,
    IN PUNICODE_STRING RemainingName,
    OUT PNTSTATUS Status
);

NTSTATUS
NTAPI
IopMountVolume(
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN AllowRawMount,
    IN BOOLEAN DeviceIsLocked,
    IN BOOLEAN Alertable,
    OUT PVPB *Vpb
);

PVOID
IoOpenSymlink(
    IN PVOID SymbolicLink
);

PVOID
IoOpenFileOnDevice(
    IN PVOID SymbolicLink,
    IN PWCHAR Name
);

NTSTATUS
NTAPI
IopCreateVpb(
    IN PDEVICE_OBJECT DeviceObject
);

VOID
NTAPI
IopDereferenceVpbAndFree(
    IN PVPB Vpb
);

VOID
NTAPI
IoInitFileSystemImplementation(
    VOID
);

VOID
NTAPI
IoInitVpbImplementation(
    VOID
);

NTSTATUS
NTAPI
IopReferenceDeviceObject(
    IN PDEVICE_OBJECT DeviceObject
);

VOID
NTAPI
IopDereferenceDeviceObject(
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN ForceUnload
);

NTSTATUS
NTAPI
IopGetRelatedTargetDevice(
    IN PFILE_OBJECT FileObject,
    OUT PDEVICE_NODE *DeviceNode);

NTSTATUS
NTAPI
IoGetRelatedTargetDevice(
    IN PFILE_OBJECT FileObject,
    OUT PDEVICE_OBJECT *DeviceObject
);

VOID
NTAPI
IopUnloadDevice(
    IN PDEVICE_OBJECT DeviceObject
);

PDEVICE_OBJECT
NTAPI
IopGetDeviceAttachmentBase(
    IN PDEVICE_OBJECT DeviceObject
);

//
// IRP Routines
//
NTSTATUS
NTAPI
IopCleanupFailedIrp(
    IN PFILE_OBJECT FileObject,
    IN PKEVENT EventObject,
    IN PVOID Buffer OPTIONAL
);

VOID
NTAPI
IopAbortInterruptedIrp(
    IN PKEVENT EventObject,
    IN PIRP Irp
);

PIRP
NTAPI
IopAllocateIrpMustSucceed(
    IN CCHAR StackSize
);

BOOLEAN
NTAPI
IopInitializeReserveIrp(
    IN PRESERVE_IRP_ALLOCATOR ReserveIrpAllocator
);

PIRP
NTAPI
IopAllocateReserveIrp(
    IN CCHAR StackSize
);

//
// Shutdown routines
//
VOID
IoInitShutdownNotification(
    VOID
);

VOID
NTAPI
IoShutdownSystem(
    IN ULONG Phase
);

VOID
NTAPI
IopShutdownBaseFileSystems(
    IN PLIST_ENTRY ListHead
);

//
// Boot logging support
//
CODE_SEG("INIT")
VOID
IopInitBootLog(
    IN BOOLEAN StartBootLog
);

CODE_SEG("INIT")
VOID
IopStartBootLog(
    VOID
);

VOID
IopStopBootLog(
    VOID
);

VOID
IopBootLog(
    IN PUNICODE_STRING DriverName,
    IN BOOLEAN Success
);

VOID
IopSaveBootLogToFile(
    VOID
);

//
// I/O Cancellation Routines
//
VOID
NTAPI
IoCancelThreadIo(
    IN PETHREAD Thread
);

VOID
IoInitCancelHandling(
    VOID
);

//
// I/O Completion
//
VOID
NTAPI
IopCompleteRequest(
    IN PKAPC Apc,
    IN PKNORMAL_ROUTINE* NormalRoutine,
    IN PVOID* NormalContext,
    IN PVOID* SystemArgument1,
    IN PVOID* SystemArgument2
);

//
// Error Logging Routines
//
VOID
NTAPI
IopInitErrorLog(
    VOID
);

VOID
NTAPI
IopLogWorker(
    IN PVOID Parameter
);

//
// Raw File System MiniDriver
//
BOOLEAN
RawFsIsRawFileSystemDeviceObject(
    IN PDEVICE_OBJECT DeviceObject
);

CODE_SEG("INIT")
NTSTATUS
NTAPI
RawFsDriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
);

//
// PnP Root MiniDriver
//
NTSTATUS
NTAPI
PnpRootDriverEntry(
   IN PDRIVER_OBJECT DriverObject,
   IN PUNICODE_STRING RegistryPath
);

NTSTATUS
PnpRootCreateDeviceObject(
    OUT PDEVICE_OBJECT *DeviceObject);

NTSTATUS
PnpRootCreateDevice(
    IN PUNICODE_STRING ServiceName,
    OUT PDEVICE_OBJECT *PhysicalDeviceObject,
    OUT PUNICODE_STRING FullInstancePath
);

NTSTATUS
PnpRootRegisterDevice(
    IN PDEVICE_OBJECT DeviceObject);

VOID
PnpRootInitializeDevExtension(VOID);

//
// Driver Routines
//
CODE_SEG("INIT")
VOID
FASTCALL
IopInitializeBootDrivers(
    VOID
);

CODE_SEG("INIT")
VOID
FASTCALL
IopInitializeSystemDrivers(
    VOID
);

VOID
NTAPI
IopDeleteDriver(
    IN PVOID ObjectBody
);

NTSTATUS
IopLoadDriver(
    _In_ HANDLE ServiceHandle,
    _Out_ PDRIVER_OBJECT *DriverObject);

NTSTATUS
IopGetDriverNames(
    _In_ HANDLE ServiceHandle,
    _Out_ PUNICODE_STRING DriverName,
    _Out_opt_ PUNICODE_STRING ServiceName);

NTSTATUS
IopInitializeDriverModule(
    _In_ PLDR_DATA_TABLE_ENTRY ModuleObject,
    _In_ HANDLE ServiceHandle,
    _Out_ PDRIVER_OBJECT *DriverObject,
    _Out_ NTSTATUS *DriverEntryStatus);

NTSTATUS
FASTCALL
IopAttachFilterDrivers(
    IN PDEVICE_NODE DeviceNode,
    IN HANDLE EnumSubKey,
    IN HANDLE ClassKey,
    IN BOOLEAN Lower
);

VOID
NTAPI
IopReinitializeDrivers(
    VOID
);

VOID
NTAPI
IopReinitializeBootDrivers(
    VOID
);

//
// File Routines
//
VOID
NTAPI
IopDeleteDevice(IN PVOID ObjectBody);

NTSTATUS
NTAPI
IopParseDevice(
    IN PVOID ParseObject,
    IN PVOID ObjectType,
    IN OUT PACCESS_STATE AccessState,
    IN KPROCESSOR_MODE AccessMode,
    IN ULONG Attributes,
    IN OUT PUNICODE_STRING CompleteName,
    IN OUT PUNICODE_STRING RemainingName,
    IN OUT PVOID Context,
    IN PSECURITY_QUALITY_OF_SERVICE SecurityQos OPTIONAL,
    OUT PVOID *Object
);

NTSTATUS
NTAPI
IopParseFile(
    IN PVOID ParseObject,
    IN PVOID ObjectType,
    IN OUT PACCESS_STATE AccessState,
    IN KPROCESSOR_MODE AccessMode,
    IN ULONG Attributes,
    IN OUT PUNICODE_STRING CompleteName,
    IN OUT PUNICODE_STRING RemainingName,
    IN OUT PVOID Context OPTIONAL,
    IN PSECURITY_QUALITY_OF_SERVICE SecurityQos OPTIONAL,
    OUT PVOID *Object
);

VOID
NTAPI
IopDeleteFile(
    IN PVOID ObjectBody
);

NTSTATUS
NTAPI
IopGetSetSecurityObject(
    IN PVOID ObjectBody,
    IN SECURITY_OPERATION_CODE OperationCode,
    IN PSECURITY_INFORMATION SecurityInformation,
    IN OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN OUT PULONG BufferLength,
    OUT PSECURITY_DESCRIPTOR *OldSecurityDescriptor,
    IN POOL_TYPE PoolType,
    IN OUT PGENERIC_MAPPING GenericMapping
);

NTSTATUS
NTAPI
IopQueryName(
    IN PVOID ObjectBody,
    IN BOOLEAN HasName,
    OUT POBJECT_NAME_INFORMATION ObjectNameInfo,
    IN ULONG Length,
    OUT PULONG ReturnLength,
    IN KPROCESSOR_MODE PreviousMode
);

NTSTATUS
NTAPI
IopQueryNameInternal(
    IN PVOID ObjectBody,
    IN BOOLEAN HasName,
    IN BOOLEAN QueryDosName,
    OUT POBJECT_NAME_INFORMATION ObjectNameInfo,
    IN ULONG Length,
    OUT PULONG ReturnLength,
    IN KPROCESSOR_MODE PreviousMode
);

VOID
NTAPI
IopCloseFile(
    IN PEPROCESS Process OPTIONAL,
    IN PVOID Object,
    IN ACCESS_MASK GrantedAccess,
    IN ULONG ProcessHandleCount,
    IN ULONG SystemHandleCount
);

NTSTATUS
NTAPI
IopAcquireFileObjectLock(
    _In_ PFILE_OBJECT FileObject,
    _In_ KPROCESSOR_MODE AccessMode,
    _In_ BOOLEAN Alertable,
    _Out_ PBOOLEAN LockFailed
);

PVOID
NTAPI
IoGetFileObjectFilterContext(
    IN PFILE_OBJECT FileObject
);

NTSTATUS
NTAPI
IoChangeFileObjectFilterContext(
    IN PFILE_OBJECT FileObject,
    IN PVOID FilterContext,
    IN BOOLEAN Define
);

VOID
NTAPI
IopDoNameTransmogrify(
    IN PIRP Irp,
    IN PFILE_OBJECT FileObject,
    IN PREPARSE_DATA_BUFFER DataBuffer
);

NTSTATUS
NTAPI
IoComputeDesiredAccessFileObject(
    IN PFILE_OBJECT FileObject,
    IN PACCESS_MASK DesiredAccess
);

NTSTATUS
NTAPI
IopGetFileInformation(
    IN PFILE_OBJECT FileObject,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInfoClass,
    OUT PVOID Buffer,
    OUT PULONG ReturnedLength
);

BOOLEAN
NTAPI
IopVerifyDeviceObjectOnStack(
    IN PDEVICE_OBJECT BaseDeviceObject,
    IN PDEVICE_OBJECT TopDeviceObjectHint
);

//
// I/O Timer Routines
//
VOID
FASTCALL
IopInitTimerImplementation(
    VOID
);

VOID
NTAPI
IopRemoveTimerFromTimerList(
    IN PIO_TIMER Timer
);

//
// I/O Completion Routines
//
VOID
NTAPI
IopDeleteIoCompletion(
    PVOID ObjectBody
);

NTSTATUS
NTAPI
IoSetIoCompletion(
    IN PVOID IoCompletion,
    IN PVOID KeyContext,
    IN PVOID ApcContext,
    IN NTSTATUS IoStatus,
    IN ULONG_PTR IoStatusInformation,
    IN BOOLEAN Quota
);

//
// Ramdisk Routines
//
CODE_SEG("INIT")
NTSTATUS
NTAPI
IopStartRamdisk(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
);

//
// Configuration Routines
//
NTSTATUS
IopFetchConfigurationInformation(
    _Out_ PWSTR* SymbolicLinkList,
    _In_ GUID Guid,
    _In_ ULONG ExpectedInterfaces,
    _Out_ PULONG Interfaces
);

VOID
IopStoreSystemPartitionInformation(
    _In_ PUNICODE_STRING NtSystemPartitionDeviceName,
    _In_ PUNICODE_STRING OsLoaderPathName
);

//
// Device action
//
VOID
PiQueueDeviceAction(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ DEVICE_ACTION Action,
    _In_opt_ PKEVENT CompletionEvent,
    _Out_opt_ NTSTATUS *CompletionStatus);

NTSTATUS
PiPerformSyncDeviceAction(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ DEVICE_ACTION Action);

//
// PnP notifications
//
CODE_SEG("PAGE")
VOID
PiNotifyDeviceInterfaceChange(
    _In_ LPCGUID Event,
    _In_ LPCGUID InterfaceClassGuid,
    _In_ PUNICODE_STRING SymbolicLinkName);

CODE_SEG("PAGE")
VOID
PiNotifyHardwareProfileChange(
    _In_ LPCGUID Event);

CODE_SEG("PAGE")
VOID
PiNotifyTargetDeviceChange(
    _In_ LPCGUID Event,
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_opt_ PTARGET_DEVICE_CUSTOM_NOTIFICATION CustomNotification);

//
// PnP IRPs
//
NTSTATUS
PiIrpStartDevice(
    _In_ PDEVICE_NODE DeviceNode);

NTSTATUS
PiIrpStopDevice(
    _In_ PDEVICE_NODE DeviceNode);

NTSTATUS
PiIrpQueryStopDevice(
    _In_ PDEVICE_NODE DeviceNode);

NTSTATUS
PiIrpCancelStopDevice(
    _In_ PDEVICE_NODE DeviceNode);

NTSTATUS
PiIrpQueryDeviceRelations(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ DEVICE_RELATION_TYPE Type);

NTSTATUS
PiIrpQueryResources(
    _In_ PDEVICE_NODE DeviceNode,
    _Out_ PCM_RESOURCE_LIST *Resources);

NTSTATUS
PiIrpQueryResourceRequirements(
    _In_ PDEVICE_NODE DeviceNode,
    _Out_ PIO_RESOURCE_REQUIREMENTS_LIST *Resources);

NTSTATUS
PiIrpQueryDeviceText(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ LCID LocaleId,
    _In_ DEVICE_TEXT_TYPE Type,
    _Out_ PWSTR *DeviceText);

NTSTATUS
PiIrpQueryPnPDeviceState(
    _In_ PDEVICE_NODE DeviceNode,
    _Out_ PPNP_DEVICE_STATE DeviceState);

//
// Global I/O Data
//
extern POBJECT_TYPE IoCompletionType;
extern PDEVICE_NODE IopRootDeviceNode;
extern KSPIN_LOCK IopDeviceTreeLock;
extern ULONG IopTraceLevel;
extern GENERAL_LOOKASIDE IopMdlLookasideList;
extern GENERIC_MAPPING IopCompletionMapping;
extern GENERIC_MAPPING IopFileMapping;
extern POBJECT_TYPE _IoFileObjectType;
extern HAL_DISPATCH _HalDispatchTable;
extern LIST_ENTRY IopErrorLogListHead;
extern ULONG IopAutoReboot;
extern ULONG IopNumTriageDumpDataBlocks;
extern PVOID IopTriageDumpDataBlocks[64];
extern PIO_BUS_TYPE_GUID_LIST PnpBusTypeGuidList;
extern PDRIVER_OBJECT IopRootDriverObject;
extern KSPIN_LOCK IopDeviceActionLock;
extern LIST_ENTRY IopDeviceActionRequestList;
extern RESERVE_IRP_ALLOCATOR IopReserveIrpAllocator;
extern BOOLEAN IoRemoteBootClient;

//
// Inlined Functions
//
#include "io_x.h"
