#ifndef __NTOSKRNL_INCLUDE_INTERNAL_IO_H
#define __NTOSKRNL_INCLUDE_INTERNAL_IO_H

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
    IN GUID* Event,
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
    PULONG BufferLength
);

NTSTATUS
STDCALL
IopQueryNameFile(
    PVOID ObjectBody,
    POBJECT_NAME_INFORMATION ObjectNameInfo,
    ULONG Length,
    PULONG ReturnLength
);

VOID
STDCALL
IopCloseFile(
    PVOID ObjectBody,
    ULONG HandleCount
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

#endif
