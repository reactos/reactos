#ifndef __NTOSKRNL_INCLUDE_INTERNAL_IO_H
#define __NTOSKRNL_INCLUDE_INTERNAL_IO_H

#include <ntdddisk.h>

/* STRUCTURES ***************************************************************/

typedef struct _DISKENTRY
{
  LIST_ENTRY ListEntry;
  ULONG DiskNumber;
  ULONG Signature;
  ULONG Checksum;
  PDEVICE_OBJECT DeviceObject;
} DISKENTRY, *PDISKENTRY; 

#define  PARTITION_TBL_SIZE 4

#include <pshpack1.h>

typedef struct _PARTITION
{
  unsigned char   BootFlags;					/* bootable?  0=no, 128=yes  */
  unsigned char   StartingHead;					/* beginning head number */
  unsigned char   StartingSector;				/* beginning sector number */
  unsigned char   StartingCylinder;				/* 10 bit nmbr, with high 2 bits put in begsect */
  unsigned char   PartitionType;				/* Operating System type indicator code */
  unsigned char   EndingHead;					/* ending head number */
  unsigned char   EndingSector;					/* ending sector number */
  unsigned char   EndingCylinder;				/* also a 10 bit nmbr, with same high 2 bit trick */
  unsigned int  StartingBlock;					/* first sector relative to start of disk */
  unsigned int  SectorCount;					/* number of sectors in partition */
} PARTITION, *PPARTITION;

typedef struct _PARTITION_SECTOR
{
  UCHAR BootCode[440];				/* 0x000 */
  ULONG Signature;				/* 0x1B8 */
  UCHAR Reserved[2];				/* 0x1BC */
  PARTITION Partition[PARTITION_TBL_SIZE];	/* 0x1BE */
  USHORT Magic;					/* 0x1FE */
} PARTITION_SECTOR, *PPARTITION_SECTOR;

#include <poppack.h>

#define IO_METHOD_FROM_CTL_CODE(ctlCode) (ctlCode&0x00000003)

extern POBJECT_TYPE IoCompletionType;
extern PDEVICE_NODE IopRootDeviceNode;

/* This is like the IRP Overlay so we can optimize its insertion */
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

typedef struct _DUMMY_FILE_OBJECT
{
    OBJECT_HEADER ObjectHeader;
    CHAR FileObjectBody[sizeof(FILE_OBJECT)];
} DUMMY_FILE_OBJECT, *PDUMMY_FILE_OBJECT;

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


/* List of Bus Type GUIDs */
typedef struct _IO_BUS_TYPE_GUID_LIST
{
    ULONG GuidCount;
    FAST_MUTEX Lock;
    GUID Guids[1];
} IO_BUS_TYPE_GUID_LIST, *PIO_BUS_TYPE_GUID_LIST;
extern PIO_BUS_TYPE_GUID_LIST IopBusTypeGuidList;

/* Packet Types */
#define IrpCompletionPacket     0x1
#define IrpMiniCompletionPacket 0x2

/*
 * VOID
 * IopDeviceNodeSetFlag(
 *   PDEVICE_NODE DeviceNode,
 *   ULONG Flag);
 */
#define IopDeviceNodeSetFlag(DeviceNode, Flag) \
    ((DeviceNode)->Flags |= (Flag))

/*
 * VOID
 * IopDeviceNodeClearFlag(
 *   PDEVICE_NODE DeviceNode,
 *   ULONG Flag);
 */
#define IopDeviceNodeClearFlag(DeviceNode, Flag) \
    ((DeviceNode)->Flags &= ~(Flag))

/*
 * BOOLEAN
 * IopDeviceNodeHasFlag(
 *   PDEVICE_NODE DeviceNode,
 *   ULONG Flag);
 */
#define IopDeviceNodeHasFlag(DeviceNode, Flag) \
    (((DeviceNode)->Flags & (Flag)) > 0)

/*
 * VOID
 * IopDeviceNodeSetUserFlag(
 *   PDEVICE_NODE DeviceNode,
 *   ULONG UserFlag);
 */
#define IopDeviceNodeSetUserFlag(DeviceNode, UserFlag) \
    ((DeviceNode)->UserFlags |= (UserFlag))

/*
 * VOID
 * IopDeviceNodeClearUserFlag(
 *   PDEVICE_NODE DeviceNode,
 *   ULONG UserFlag);
 */
#define IopDeviceNodeClearUserFlag(DeviceNode, UserFlag) \
    ((DeviceNode)->UserFlags &= ~(UserFlag))

/*
 * BOOLEAN
 * IopDeviceNodeHasUserFlag(
 *   PDEVICE_NODE DeviceNode,
 *   ULONG UserFlag);
 */
#define IopDeviceNodeHasUserFlag(DeviceNode, UserFlag) \
    (((DeviceNode)->UserFlags & (UserFlag)) > 0)

 /*
 * VOID
 * IopDeviceNodeSetProblem(
 *   PDEVICE_NODE DeviceNode,
 *   ULONG Problem);
 */
#define IopDeviceNodeSetProblem(DeviceNode, Problem) \
    ((DeviceNode)->Problem |= (Problem))

/*
 * VOID
 * IopDeviceNodeClearProblem(
 *   PDEVICE_NODE DeviceNode,
 *   ULONG Problem);
 */
#define IopDeviceNodeClearProblem(DeviceNode, Problem) \
    ((DeviceNode)->Problem &= ~(Problem))

/*
 * BOOLEAN
 * IopDeviceNodeHasProblem(
 *   PDEVICE_NODE DeviceNode,
 *   ULONG Problem);
 */
#define IopDeviceNodeHasProblem(DeviceNode, Problem) \
    (((DeviceNode)->Problem & (Problem)) > 0)


/*
   Called on every visit of a node during a preorder-traversal of the device
   node tree.
   If the routine returns STATUS_UNSUCCESSFUL the traversal will stop and
   STATUS_SUCCESS is returned to the caller who initiated the tree traversal.
   Any other returned status code will be returned to the caller. If a status
   code that indicates an error (other than STATUS_UNSUCCESSFUL) is returned,
   the traversal is stopped immediately and the status code is returned to
   the caller.
 */
typedef NTSTATUS (*DEVICETREE_TRAVERSE_ROUTINE)(
    PDEVICE_NODE DeviceNode,
    PVOID Context
);

/* Context information for traversing the device tree */
typedef struct _DEVICETREE_TRAVERSE_CONTEXT
{
    /* Current device node during a traversal */
    PDEVICE_NODE DeviceNode;
    /* Initial device node where we start the traversal */
    PDEVICE_NODE FirstDeviceNode;
    /* Action routine to be called for every device node */
    DEVICETREE_TRAVERSE_ROUTINE Action;
    /* Context passed to the action routine */
    PVOID Context;
} DEVICETREE_TRAVERSE_CONTEXT, *PDEVICETREE_TRAVERSE_CONTEXT;

/*
 * VOID
 * IopInitDeviceTreeTraverseContext(
 *   PDEVICETREE_TRAVERSE_CONTEXT DeviceTreeTraverseContext,
 *   PDEVICE_NODE DeviceNode,
 *   DEVICETREE_TRAVERSE_ROUTINE Action,
 *   PVOID Context);
 */
#define IopInitDeviceTreeTraverseContext( \
  _DeviceTreeTraverseContext, _DeviceNode, _Action, _Context) { \
  (_DeviceTreeTraverseContext)->FirstDeviceNode = (_DeviceNode); \
  (_DeviceTreeTraverseContext)->Action = (_Action); \
  (_DeviceTreeTraverseContext)->Context = (_Context); }

VOID
PnpInit(VOID);

VOID
PnpInit2(VOID);

VOID
IopInitDriverImplementation(VOID);

VOID
IopInitPnpNotificationImplementation(VOID);

VOID
IopNotifyPlugPlayNotification(
    IN PDEVICE_OBJECT DeviceObject,
    IN IO_NOTIFICATION_EVENT_CATEGORY EventCategory,
    IN LPCGUID Event,
    IN PVOID EventCategoryData1,
    IN PVOID EventCategoryData2
);

NTSTATUS
IopGetSystemPowerDeviceObject(PDEVICE_OBJECT *DeviceObject);

NTSTATUS
IopCreateDeviceNode(
    PDEVICE_NODE ParentNode,
    PDEVICE_OBJECT PhysicalDeviceObject,
    PDEVICE_NODE *DeviceNode
);

NTSTATUS
IopFreeDeviceNode(PDEVICE_NODE DeviceNode);

VOID
IoInitCancelHandling(VOID);

VOID
IoInitFileSystemImplementation(VOID);

VOID
IoInitVpbImplementation(VOID);

NTSTATUS
IoMountVolume(
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN AllowRawMount
);

PVOID 
IoOpenSymlink(PVOID SymbolicLink);

PVOID 
IoOpenFileOnDevice(
    PVOID SymbolicLink, 
    PWCHAR Name
);

NTSTATUS
STDCALL
IopCreateDevice(
    PVOID ObjectBody,
    PVOID Parent,
    PWSTR RemainingPath,
    POBJECT_ATTRIBUTES ObjectAttributes
);

NTSTATUS
STDCALL
IopAttachVpb(PDEVICE_OBJECT DeviceObject);

VOID
IoInitShutdownNotification(VOID);

VOID
IoShutdownRegisteredDevices(VOID);

VOID
IoShutdownRegisteredFileSystems(VOID);

NTSTATUS
IoCreateArcNames(VOID);

NTSTATUS
IoCreateSystemRootLink(PCHAR ParameterLine);

NTSTATUS
IopInitiatePnpIrp(
    PDEVICE_OBJECT DeviceObject,
    PIO_STATUS_BLOCK IoStatusBlock,
    ULONG MinorFunction,
    PIO_STACK_LOCATION Stack
);

NTSTATUS
IoCreateDriverList(VOID);

NTSTATUS
IoDestroyDriverList(VOID);

/* bootlog.c */

VOID
IopInitBootLog(BOOLEAN StartBootLog);

VOID
IopStartBootLog(VOID);

VOID
IopStopBootLog(VOID);

VOID
IopBootLog(
    PUNICODE_STRING DriverName, 
    BOOLEAN Success
);

VOID
IopSaveBootLogToFile(VOID);

/* cancel.c */

VOID
STDCALL
IoCancelThreadIo(PETHREAD Thread);

/* errlog.c */

NTSTATUS
IopInitErrorLog(VOID);

/* rawfs.c */

BOOLEAN
RawFsIsRawFileSystemDeviceObject(IN PDEVICE_OBJECT DeviceObject);

NTSTATUS
STDCALL
RawFsDriverEntry(PDRIVER_OBJECT DriverObject,
                 PUNICODE_STRING RegistryPath);


/* pnpmgr.c */

PDEVICE_NODE
FASTCALL
IopGetDeviceNode(PDEVICE_OBJECT DeviceObject);

NTSTATUS
IopActionConfigureChildServices(PDEVICE_NODE DeviceNode,
                                PVOID Context);

NTSTATUS
IopActionInitChildServices(PDEVICE_NODE DeviceNode,
                           PVOID Context,
                           BOOLEAN BootDrivers);


/* pnproot.c */

NTSTATUS
STDCALL
PnpRootDriverEntry(
   IN PDRIVER_OBJECT DriverObject,
   IN PUNICODE_STRING RegistryPath
);

NTSTATUS
PnpRootCreateDevice(PDEVICE_OBJECT *PhysicalDeviceObject);

/* device.c */

NTSTATUS 
FASTCALL
IopInitializeDevice(
    PDEVICE_NODE DeviceNode,
    PDRIVER_OBJECT DriverObject
);

NTSTATUS
IopStartDevice(PDEVICE_NODE DeviceNode);

/* driver.c */

VOID
FASTCALL
IopInitializeBootDrivers(VOID);

VOID
FASTCALL
IopInitializeSystemDrivers(VOID);

NTSTATUS
FASTCALL
IopCreateDriverObject(
    PDRIVER_OBJECT *DriverObject,
    PUNICODE_STRING ServiceName,
    ULONG CreateAttributes,
    BOOLEAN FileSystemDriver,
    PVOID DriverImageStart,
    ULONG DriverImageSize
);

NTSTATUS
FASTCALL
IopGetDriverObject(
    PDRIVER_OBJECT *DriverObject,
    PUNICODE_STRING ServiceName,
    BOOLEAN FileSystem
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
    PDEVICE_NODE DeviceNode,
    BOOLEAN Lower
);

VOID
FASTCALL
IopMarkLastReinitializeDriver(VOID);

VOID
FASTCALL
IopReinitializeDrivers(VOID);

/* file.c */

NTSTATUS
STDCALL
IopCreateFile(
    PVOID ObjectBody,
    PVOID Parent,
    PWSTR RemainingPath,
    POBJECT_CREATE_INFORMATION ObjectAttributes
);

VOID
STDCALL
IopDeleteFile(PVOID ObjectBody);

NTSTATUS
STDCALL
IopSecurityFile(
    PVOID ObjectBody,
    SECURITY_OPERATION_CODE OperationCode,
    SECURITY_INFORMATION SecurityInformation,
    PSECURITY_DESCRIPTOR SecurityDescriptor,
    PULONG BufferLength,
	PSECURITY_DESCRIPTOR *OldSecurityDescriptor,    
    POOL_TYPE PoolType,
    PGENERIC_MAPPING GenericMapping
);

NTSTATUS
STDCALL
IopQueryNameFile(
    PVOID ObjectBody,
    IN BOOLEAN HasName,
    POBJECT_NAME_INFORMATION ObjectNameInfo,
    ULONG Length,
    PULONG ReturnLength
);

VOID
STDCALL
IopCloseFile(
    IN PEPROCESS Process OPTIONAL,
    IN PVOID Object,
    IN ACCESS_MASK GrantedAccess,
    IN ULONG ProcessHandleCount,
    IN ULONG SystemHandleCount
);

/* plugplay.c */

NTSTATUS 
INIT_FUNCTION
IopInitPlugPlayEvents(VOID);

NTSTATUS
IopQueueTargetDeviceEvent(
    const GUID *Guid,
    PUNICODE_STRING DeviceIds
);

/* pnpmgr.c */

NTSTATUS
IopInitializePnpServices(
    IN PDEVICE_NODE DeviceNode,
    IN BOOLEAN BootDrivers)
;

NTSTATUS
IopInvalidateDeviceRelations(
    IN PDEVICE_NODE DeviceNode,
    IN DEVICE_RELATION_TYPE Type
);

/* timer.c */
VOID
FASTCALL
IopInitTimerImplementation(VOID);

VOID
STDCALL
IopRemoveTimerFromTimerList(IN PIO_TIMER Timer);

/* iocomp.c */
VOID
FASTCALL
IopInitIoCompletionImplementation(VOID);

#define CM_RESOURCE_LIST_SIZE(ResList) \
  (ResList->Count == 1) ? \
    FIELD_OFFSET(CM_RESOURCE_LIST, List[0].PartialResourceList. \
                 PartialDescriptors[(ResList)->List[0].PartialResourceList.Count]) \
                        : \
    FIELD_OFFSET(CM_RESOURCE_LIST, List)

/* xhal.c */
NTSTATUS
FASTCALL
xHalQueryDriveLayout(
    IN PUNICODE_STRING DeviceName,
    OUT PDRIVE_LAYOUT_INFORMATION *LayoutInfo
);

#undef HalExamineMBR
VOID 
FASTCALL
HalExamineMBR(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN ULONG MBRTypeIdentifier,
    OUT PVOID *Buffer
);

VOID 
FASTCALL
xHalIoAssignDriveLetters(
    IN PROS_LOADER_PARAMETER_BLOCK LoaderBlock,
    IN PSTRING NtDeviceName,
    OUT PUCHAR NtSystemPath,
    OUT PSTRING NtSystemPathString
);

NTSTATUS 
FASTCALL
xHalIoReadPartitionTable(
    PDEVICE_OBJECT DeviceObject,
    ULONG SectorSize,
    BOOLEAN ReturnRecognizedPartitions,
    PDRIVE_LAYOUT_INFORMATION *PartitionBuffer
);

NTSTATUS 
FASTCALL
xHalIoSetPartitionInformation(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN ULONG PartitionNumber,
    IN ULONG PartitionType
);

NTSTATUS 
FASTCALL
xHalIoWritePartitionTable(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN ULONG SectorsPerTrack,
    IN ULONG NumberOfHeads,
    IN PDRIVE_LAYOUT_INFORMATION PartitionBuffer
);

#endif
