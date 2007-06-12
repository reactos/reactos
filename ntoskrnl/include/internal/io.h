/*
* PROJECT:         ReactOS Kernel
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            ntoskrnl/include/io.h
* PURPOSE:         Internal header for the I/O Manager
* PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
*/
#include "ntdddisk.h"

//
// Define this if you want debugging support
//
#define _IO_DEBUG_                                      0x01

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
#define IOTRACE(x, ...) DPRINT(__VA_ARGS__);
#endif

//
// Returns the type of METHOD_ used in this IOCTL
//
#define IO_METHOD_FROM_CTL_CODE(c)                      (c & 0x00000003)

//
// Packet Types when piggybacking on the IRP Overlay
//
#define IrpCompletionPacket                             0x1
#define IrpMiniCompletionPacket                         0x2

//
// We can call the Ob Inlined API, it's the same thing
//
#define IopAllocateMdlFromLookaside                     \
    ObpAllocateCapturedAttributes
#define IopFreeMdlFromLookaside                         \
    ObpFreeCapturedAttributes

//
// Returns the size of a CM_RESOURCE_LIST
//
#define CM_RESOURCE_LIST_SIZE(ResList)                  \
    (ResList->Count == 1) ?                             \
        FIELD_OFFSET(                                   \
            CM_RESOURCE_LIST,                           \
            List[0].PartialResourceList.                \
            PartialDescriptors[(ResList)->              \
                               List[0].                 \
                               PartialResourceList.     \
                               Count])                  \
        :                                               \
        FIELD_OFFSET(CM_RESOURCE_LIST, List)

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
// Special version of the IRP Overlay used to optimize I/O completion
// by not using up a separate structure.
//
typedef struct _IO_COMPLETION_PACKET
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
    PVOID Key;
    PVOID Context;
    IO_STATUS_BLOCK IoStatus;
} IO_COMPLETION_PACKET, *PIO_COMPLETION_PACKET;

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
} IO_WORKITEM, *PIO_WORKITEM;

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
// Event Log LPC Message
//
typedef struct _ELF_API_MSG
{
    PORT_MESSAGE h;
    ULONG Unknown[2];
    IO_ERROR_LOG_MESSAGE IoErrorMessage;
} ELF_API_MSG, *PELF_API_MSG;

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
    PVOID MailslotOrPipeParameters;
    BOOLEAN Override;
    BOOLEAN QueryOnly;
    BOOLEAN DeleteOnly;
    BOOLEAN FullAttributes;
    PDUMMY_FILE_OBJECT DummyFileObject;
    ULONG InternalFlags;
    //PIO_DRIVER_CREATE_CONTEXT DriverCreateContext; Vista only, needs ROS DDK Update
} OPEN_PACKET, *POPEN_PACKET;

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
// PNP Routines
//
VOID
PnpInit(
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

VOID
IopInitPnpNotificationImplementation(
    VOID
);

VOID
IopNotifyPlugPlayNotification(
    IN PDEVICE_OBJECT DeviceObject,
    IN IO_NOTIFICATION_EVENT_CATEGORY EventCategory,
    IN LPCGUID Event,
    IN PVOID EventCategoryData1,
    IN PVOID EventCategoryData2
);

NTSTATUS
IopGetSystemPowerDeviceObject(
    IN PDEVICE_OBJECT *DeviceObject
);

NTSTATUS
IopCreateDeviceNode(
    IN PDEVICE_NODE ParentNode,
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN PUNICODE_STRING ServiceName,
    OUT PDEVICE_NODE *DeviceNode
);

NTSTATUS
IopFreeDeviceNode(
    IN PDEVICE_NODE DeviceNode
);

NTSTATUS
IopInitiatePnpIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG MinorFunction,
    IN PIO_STACK_LOCATION Stack
);

PDEVICE_NODE
FASTCALL
IopGetDeviceNode(
    IN PDEVICE_OBJECT DeviceObject
);

NTSTATUS
IopActionConfigureChildServices(
    IN PDEVICE_NODE DeviceNode,
    IN PVOID Context
);

NTSTATUS
IopActionInitChildServices(
    IN PDEVICE_NODE DeviceNode,
    IN PVOID Context,
    IN BOOLEAN BootDrivers
);

NTSTATUS
IoCreateDriverList(
    VOID
);

NTSTATUS
IoDestroyDriverList(
    VOID
);

NTSTATUS 
INIT_FUNCTION
IopInitPlugPlayEvents(VOID);

NTSTATUS
IopQueueTargetDeviceEvent(
    const GUID *Guid,
    PUNICODE_STRING DeviceIds
);

NTSTATUS
IopInitializePnpServices(
    IN PDEVICE_NODE DeviceNode,
    IN BOOLEAN BootDrivers)
;

//
// Initialization Routines
//
NTSTATUS
NTAPI
IopCreateArcNames(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
);

NTSTATUS
NTAPI
IopReassignSystemRoot(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    OUT PANSI_STRING NtBootPath
);

BOOLEAN
NTAPI
IoInitSystem(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
);

//
// Device/Volume Routines
//
NTSTATUS
FASTCALL
IopInitializeDevice(
    IN PDEVICE_NODE DeviceNode,
    IN PDRIVER_OBJECT DriverObject
);

NTSTATUS
IopStartDevice(
    IN PDEVICE_NODE DeviceNode
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
IopDereferenceVpb(
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

//
// Shutdown routines
//
VOID
IoInitShutdownNotification(
    VOID
);

VOID
NTAPI
IoShutdownRegisteredDevices(
    VOID
);

VOID
NTAPI
IoShutdownRegisteredFileSystems(
    VOID
);

//
// Boot logging support
//
VOID
IopInitBootLog(
    IN BOOLEAN StartBootLog
);

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
PnpRootCreateDevice(
    IN PUNICODE_STRING ServiceName,
    IN OUT PDEVICE_OBJECT *PhysicalDeviceObject
);

//
// Driver Routines
//
VOID
FASTCALL
IopInitializeBootDrivers(
    VOID
);

VOID
FASTCALL
IopInitializeSystemDrivers(
    VOID
);

NTSTATUS
NTAPI
IopCreateDriver(IN PUNICODE_STRING DriverName OPTIONAL,
                IN PDRIVER_INITIALIZE InitializationFunction,
                IN PUNICODE_STRING RegistryPath,
                IN PVOID DllBase,
                IN ULONG SizeOfImage,
                OUT PDRIVER_OBJECT *pDriverObject);

VOID
NTAPI
IopDeleteDriver(
    IN PVOID ObjectBody
);

NTSTATUS
FASTCALL
IopGetDriverObject(
    OUT PDRIVER_OBJECT *DriverObject,
    IN PUNICODE_STRING ServiceName,
    IN BOOLEAN FileSystem
);

NTSTATUS
FASTCALL
IopLoadServiceModule(
    IN PUNICODE_STRING ServiceName,
    OUT PLDR_DATA_TABLE_ENTRY *ModuleObject
);

NTSTATUS 
FASTCALL
IopInitializeDriverModule(
    IN PDEVICE_NODE DeviceNode,
    IN PLDR_DATA_TABLE_ENTRY ModuleObject,
    IN PUNICODE_STRING ServiceName,
    IN BOOLEAN FileSystemDriver,
    OUT PDRIVER_OBJECT *DriverObject
);

NTSTATUS
FASTCALL
IopAttachFilterDrivers(
    IN PDEVICE_NODE DeviceNode,
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
    IN OUT PVOID Context OPTIONAL,
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
IopSecurityFile(
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
IopQueryNameFile(
    IN PVOID ObjectBody,
    IN BOOLEAN HasName,
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

//
// Global I/O Data
//
extern POBJECT_TYPE IoCompletionType;
extern PDEVICE_NODE IopRootDeviceNode;
extern ULONG IopTraceLevel;
extern NPAGED_LOOKASIDE_LIST IopMdlLookasideList;
extern GENERIC_MAPPING IopCompletionMapping;
extern GENERIC_MAPPING IopFileMapping;
extern POBJECT_TYPE _IoFileObjectType;
extern HAL_DISPATCH _HalDispatchTable;
extern LIST_ENTRY IopErrorLogListHead;

//
// Inlined Functions
//
#include "io_x.h"
