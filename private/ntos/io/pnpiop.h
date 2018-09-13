/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    pnpsubs.c

Abstract:

    This module contains the plug-and-play macros and constants.

Author:

    Shie-Lin Tzong (shielint) 29-Jan-1995
    Andrew Thornton (andrewth) 5-Sept-1996

Environment:

    Kernel mode


Revision History:


--*/

#include "..\pnp\pnpi.h"
#include "arbiter.h"
#include "dockintf.h"
#include "pnprlist.h"

//
// Pool tags
//

#define IOP_DNOD_TAG    'donD'
#define IOP_DNDT_TAG    'tdnD'
#define IOP_DPWR_TAG    'rwPD'

//
// Dbg scope
//

#define DBG_SCOPE 1     // Enable SOME DBG stuff on ALL builds
//#define DBG_SCOPE DBG // Enable only on DBG build

//
// The DEVICE_NODE is really just some extra stuff that we'd like to keep around
// for each physical device object.
// It is seperated from DEVOBJ_EXTENSION because these fields only apply to
// PDO.
//

typedef enum {

    DOCK_NOTDOCKDEVICE,
    DOCK_QUIESCENT,
    DOCK_ARRIVING,
    DOCK_DEPARTING,
    DOCK_EJECTIRP_COMPLETED

} PROFILE_STATUS;

typedef enum {

    PROFILE_IN_PNPEVENT,
    PROFILE_NOT_IN_PNPEVENT,
    PROFILE_PERHAPS_IN_PNPEVENT

} PROFILE_NOTIFICATION_TIME;

typedef struct _PENDING_SET_INTERFACE_STATE
{
    LIST_ENTRY      List;
    UNICODE_STRING  LinkName;
} PENDING_SET_INTERFACE_STATE, *PPENDING_SET_INTERFACE_STATE;


typedef enum _UNLOCK_UNLINK_ACTION {
    UnlinkRemovedDeviceNodes,
    UnlinkAllDeviceNodesPendingClose,
    UnlinkOnlyChildDeviceNodesPendingClose
}   UNLOCK_UNLINK_ACTION, *PUNLOCK_UNLINK_ACTION;

typedef struct _DEVICE_NODE *PDEVICE_NODE;
typedef struct _DEVICE_NODE {

    //
    // Pointer to another DEVICE_NODE with the same parent as this one.
    //

    PDEVICE_NODE Sibling;

    //
    // Pointer to the first child of this DEVICE_NODE.
    //

    PDEVICE_NODE Child;

    //
    // Pointer to this DEVICE_NODE's parent.
    //

    PDEVICE_NODE Parent;

    //
    // Pointer to the last child of the device node
    //

    PDEVICE_NODE LastChild;

    //
    // Depth of DEVICE_NODE in the tree, root is 0
    //

    ULONG Level;

    //
    // Power notification order list entry for this device node
    //

    PPO_DEVICE_NOTIFY Notify;

    //
    // General flags.
    //

    ULONG Flags;

    //
    // Flags used by user-mode for volatile state which should go away on a
    // reboot or when the device is removed.
    //

    ULONG UserFlags;

    //
    // Problem.  This is set if DNF_HAS_PROBLEM is set in Flags.  Indicates
    // which problem exists and uses the same values as the config manager
    // CM_PROB_*
    //

    ULONG Problem;

    //
    // Pointer to the physical device object that this DEVICE_NODE is associated
    // with.
    //

    PDEVICE_OBJECT PhysicalDeviceObject;

    //
    // Pointer to the list of resources assigned to the PhysicalDeviceObject.
    // This is the Resource list which is passed to driver's start routine.
    // Note, PDO contains ListOfAssignedResources which described the resources
    // owned by the PDO. But, it is not in the format we will pass to driver or
    // write to registry.
    //

    PCM_RESOURCE_LIST ResourceList;

    PCM_RESOURCE_LIST ResourceListTranslated;

    //
    // EnumerationMutex ensures that a given DEVICE_NODE doesn't get enumerated
    // on multiple threads at the same time.
    //

    KEVENT EnumerationMutex;

    //
    // InstancePath is the path of the instance node in the registry,
    // i.e. <EnumBus>\<DeviceId>\<uniqueid>
    //

    UNICODE_STRING InstancePath;

    //
    // ServiceName is the name of the driver who controls the device. (Not the
    // driver who enumerates/creates the PDO.)  This field is mainly for
    // convenience.
    //

    UNICODE_STRING ServiceName;

    //
    // DuplicatePDO - if the flags have DNF_DUPLICATE set then this fields indicates
    // the duplicate PDO which is enumerated by a bus driver.  N.B. It is possible
    // that DNF_DUPLICATE is set but this field is NULL.  In this case, it means that
    // we know the device is a duplicate of another device and we have not enumerated
    // the DuplicatePDO yet.
    //

    PDEVICE_OBJECT DuplicatePDO;

    //
    // ResourceRequirements
    //

    PIO_RESOURCE_REQUIREMENTS_LIST ResourceRequirements;

    //
    // Information queried from the LEGACY_BUS_INFORMATION irp.
    //

    INTERFACE_TYPE InterfaceType;
    ULONG BusNumber;

    //
    // Information queried from the BUS_INFORMATION irp.
    //

    INTERFACE_TYPE ChildInterfaceType;
    ULONG ChildBusNumber;
    USHORT ChildBusTypeIndex;

    //
    // Information queried from the PNP_BUS_INFORMATION irp. The BusTypeIndex
    // value is actually an index into a global table of known bus type guids.
    //

    USHORT Reserved;        // padding, someone else can use this field

    //
    // Linked list of entries that represent each driver that has registered
    // for notification on this devnode. Note: drivers (and user-mode) actually
    // register based on a FILE_OBJECT handle, which is translated into a PDO
    // by sending an IRP_MN_QUERY_DEVICE_RELATIONS for TargetDeviceRelation.
    //

    LIST_ENTRY TargetDeviceNotify;

    //
    // DeviceArbiterList - A list of arbiters registered for this physical device object
    // Note: The Arbiters must be dereferenced when the device node is going away.
    //

    LIST_ENTRY DeviceArbiterList;

    //
    // DeviceTranslatorList - A list of translator for this physical device object
    // NOTE: the Translator must be dereferenced when the devic node is going away.
    //

    LIST_ENTRY DeviceTranslatorList;

    //
    // NoTranslatorMask - the bit position corresponds to resource type
    //   if bit is set, there is no translator for the resource type in this devnode
    //

    USHORT NoTranslatorMask;

    //
    // QueryTranslatorMask - The bit position corresponds to resource type.
    //   if bit is set, the translator for the resource type is queried.
    //

    USHORT QueryTranslatorMask;

    //
    // NoArbiterMask - the bit position corresponds to resource type
    //   if bit is set, there is no arbiter for the resource type in this devnode
    //

    USHORT NoArbiterMask;

    //
    // QueryArbiterMask - The bit position corresponds to resource type.
    //   if bit is set, the arbiter for the resource type is queried.
    //

    USHORT QueryArbiterMask;

    //
    // The following fields are used to track  legacy resource allocation
    // LegacyDeviceNode - The real legacy device node.
    // NextResourceDeviceNode - link all the made-up device nodes which own part of
    //   the resources from LegacyDeviceNode.
    //

    union {
        PDEVICE_NODE LegacyDeviceNode;
        PDEVICE_RELATIONS PendingDeviceRelations;
    } OverUsed1;

    union {
        PDEVICE_NODE NextResourceDeviceNode;
    } OverUsed2;

    //
    // Remember the BootResources for the device
    //

    PCM_RESOURCE_LIST BootResources;

    //
    // Lock Count used to keep track of multiple ejects
    //
    ULONG LockCount;

    //
    // If this devnode has been QueryRemoved but the original target of the
    // QueryRemove is still physically present then this will point to the
    // relation list used to process the original QueryRemove.
    //
    PRELATION_LIST RelationList;

    //
    // When Capabilities have been queried for a device (twice, once before
    // start and once after start) the flags are stored here in the same format
    // as the query capabilities IRP - use IopDeviceNodeFlagsToCapabilities to
    // access.
    //
    ULONG CapabilityFlags;

    //
    // Maintain a list of current dock devices and their SerialNumbers
    //
    struct {
        PROFILE_STATUS  DockStatus;
        LIST_ENTRY      ListEntry;
        PWCHAR          SerialNumber;
    } DockInfo;

    //
    // Maintain a count to determine if either ourselves or any of
    // our children are stopping us from being disableable
    // count = myself (DNUF_NOT_DISABLEABLE) + 1 for each immediate
    // child that has DisableableDepends > 0
    //
    ULONG DisableableDepends;

    //
    // List of pended IoSetDeviceInterfaceState calls.
    // IoSetDeviceInterfaceState adds an entry to this list whenever it is
    // called and we haven't been started yet.  Once we do the start we'll
    // run down the list.
    //
    LIST_ENTRY PendedSetInterfaceState;

#if DBG_SCOPE
    ULONG FailureStatus;
    PCM_RESOURCE_LIST PreviousResourceList;
    PIO_RESOURCE_REQUIREMENTS_LIST PreviousResourceRequirements;
#endif

} DEVICE_NODE;


//
// A device Object is a PDO iff it has a non NULL device node (aka set by
// plug and play during a query device relations.
//

#define ASSERT_PDO(d) \
    do { \
        if (    NULL == (d)->DeviceObjectExtension->DeviceNode || \
                (((PDEVICE_NODE)(d)->DeviceObjectExtension->DeviceNode)->Flags & DNF_LEGACY_RESOURCE_DEVICENODE))  { \
            KeBugCheckEx(PNP_DETECTED_FATAL_ERROR, PNP_ERR_INVALID_PDO, (ULONG_PTR)d, 0, 0); \
        } \
    } \
    while (0)

//
// DNF_MAKEUP - this devnode's device is created and owned by PnP manager
//

#define DNF_MADEUP                                  0x00000001

//
// DNF_DUPLICATE - this devnode's device is a duplicate of another enumerate PDO
//

#define DNF_DUPLICATE                               0x00000002

//
// DNF_HAL_NODE - a flag to indicate which device node is the root node created by
// the hal
//

#define DNF_HAL_NODE                                0x00000004

//
// DNF_PROCESSED - indicates if the registry instance key of the device node
//                 was created.
//

#define DNF_PROCESSED                               0x00000008

//
// DNF_ENUMERATED - used to track enumeration in IopEnumerateDevice()
//

#define DNF_ENUMERATED                              0x00000010

//
// Singal that we need to send driver query id irps
//

#define DNF_NEED_QUERY_IDS                          0x00000020

//
// THis device has been added to its controlling driver
//

#define DNF_ADDED                                   0x00000040

//
// DNF_HAS_BOOT_CONFIG - the device has resource assigned by BIOS.  It is considered
//    pseudo-started and need to participate in rebalance.
//

#define DNF_HAS_BOOT_CONFIG                         0x00000080

//
// DNF_BOOT_CONFIG_RESERVED - Indicates the BOOT resources of the device are reserved.
//

#define DNF_BOOT_CONFIG_RESERVED                    0x00000100

//
// DNF_START_REQUEST_PENDING - Indicates the device is being started.
//

#define DNF_START_REQUEST_PENDING                   0x00000200

//
// DNF_NO_RESOURCE_REQUIRED - this devnode's device does not require resource.
//

#define DNF_NO_RESOURCE_REQUIRED                    0x00000400

//
// DNF_RESOURCE_REQUIREMENTS_NEED_FILTERED - to distinguished the
//      DeviceNode->ResourceRequirements is a filtered list or not.
//

#define DNF_RESOURCE_REQUIREMENTS_NEED_FILTERED     0x00000800

//
// Indicates the device's resources are bing assigned (but is not done yet.)
// So don't try assign resource to this device.
//

#define DNF_ASSIGNING_RESOURCES                     0x00001000

//
// DNF_RESOURCE_ASSIGNED - this devnode's resources are assigned by PnP
//

#define DNF_RESOURCE_ASSIGNED                       0x00002000

//
// DNF_RESOURCE_REPORTED - this devnode's resources are reported by PnP
//

#define DNF_RESOURCE_REPORTED                       0x00004000

//
// DNF_RESOURCE_REQUIREMENTS_CHANGED - Indicates the device's resource
//      requirements list has been changed.
//

#define DNF_RESOURCE_REQUIREMENTS_CHANGED           0x00008000

//
// DNF_NON_STOPPED_REBALANC - indicates the device can be restarted with new
//      resources without being stopped.
//

#define DNF_NON_STOPPED_REBALANCE                   0x00010000

//
// DNF_STOPPED - indicates this device is currently stopped for reconfiguration of
//               its resources.
//

#define DNF_STOPPED                                 0x00020000

//
// DNF_STARTED - indicates if the device was started, i.e., its StartDevice
//               irp is processed.
//

#define DNF_STARTED                                 0x00040000

//
// The device's controlling driver is a legacy driver
//

#define DNF_LEGACY_DRIVER                           0x00080000

//
// For the reported detected devices, they are considered started.  We still
// need a flag to indicate we need to enumerate the device.
//

#define DNF_NEED_ENUMERATION_ONLY                   0x00100000

//
// DNF_IO_INVALIDATE_DEVICE_RELATIONS_PENDING - indicate the
//      IoInvalidateDeviceRelations request is pending and therequest needs to
//      be queued after the Query_Device_relation irp is completed.
//

#define DNF_IO_INVALIDATE_DEVICE_RELATIONS_PENDING  0x00200000

//
// Indicates the device is being sent a query device relations irp. So no more
//      q-d-r irp at the same time.
//

#define DNF_BEING_ENUMERATED                        0x00400000

//
// DNF_ENUMERATION_REQUEST_QUEUED - indicate the IoInvalidateDeviceRelations
//      request is queued.  So, new IoInvalidateDeviceRelations can be ignored.
//

#define DNF_ENUMERATION_REQUEST_QUEUED              0x00800000

//
// DNF_ENUMERATION_REQUEST_PENDING - Indicates the QUERY_DEVICE_RELATIONS irp
//      returns pending.
//

#define DNF_ENUMERATION_REQUEST_PENDING             0x01000000

//
// This corresponds to the user-mode CM_PROB_WILL_BE_REMOVED problem value and
// the DN_WILL_BE_REMOVED status flag.
//

#define DNF_HAS_PROBLEM                             0x02000000

//
// DNF_HAS_PRIVATE_PROBLEM - indicates this device reported PNP_DEVICE_FAILED
//  to a IRP_MN_QUERY_PNP_DEVICE_STATE without also reporting
//  PNP_DEVICE_RESOURCE_REQUIREMENTS_CHANGED.
//

#define DNF_HAS_PRIVATE_PROBLEM                     0x04000000

//
// DNF_REMOVE_PENDING_CLOSES is set after a IRP_MN_SURPRISE_REMOVE is sent
// to a device object.  It is an indicator that IRP_MN_REMOVE_DEVICE should
// be sent to the device object as soon as all of the file objects have gone
// away.
//

#define DNF_REMOVE_PENDING_CLOSES                   0x08000000

//
// DNF_DEVICE_GONE is set when a pdo is no longer returned in a query bus
// relations.  It will then be processed as a surprise remove if started.
// This flag is used to better detect when a device is resurrected, and when
// processing surprise remove, to determine if the devnode should be removed
// from the tree.
//

#define DNF_DEVICE_GONE                             0x10000000

//
// DNF_LEGACY_RESOURCE_DEVICENODE is set for device nodes created for legacy
// resource allocation.
//

#define DNF_LEGACY_RESOURCE_DEVICENODE              0x20000000

//
// DNF_NEEDS_REBALANCE is set for device nodes that trigger rebalance.
//

#define DNF_NEEDS_REBALANCE                         0x40000000

//
// DNF_LOCKED_FOR_EJECT is set on device nodes that are being ejected or are
// related to a device being ejected.
//

#define DNF_LOCKED_FOR_EJECT                        0x80000000

//
// This corresponds to the user-mode the DN_WILL_BE_REMOVED status flag.
//

#define DNUF_WILL_BE_REMOVED                        0x00000001

//
// This corresponds to the user-mode DN_NO_SHOW_IN_DM status flag.
//

#define DNUF_DONT_SHOW_IN_UI                        0x00000002

//
// This flag is set when user-mode lets us know that a reboot is required
// for this device.
//

#define DNUF_NEED_RESTART                           0x00000004

//
// This flag is set to let the user-mode know when a device can be disabled
// it is still possible for this to be TRUE, yet disable to fail, as it's
// a polled flag (see also PNP_DEVICE_NOT_DISABLEABLE)
//

#define DNUF_NOT_DISABLEABLE                        0x00000008


#define DNF_ADD_PHASE                 (DNF_HAS_PROBLEM | DNF_HAS_PRIVATE_PROBLEM | DNF_DEVICE_GONE | DNF_REMOVE_PENDING_CLOSES | DNF_ADDED)

#define OK_TO_ADD_DEVICE(_devnode_)                                 \
    ( (_devnode_)->Flags & DNF_PROCESSED     &&                     \
      !((_devnode_)->Flags & DNF_ADD_PHASE) )

#define DNF_START_PHASE               (DNF_HAS_PROBLEM | DNF_HAS_PRIVATE_PROBLEM | DNF_DEVICE_GONE | DNF_REMOVE_PENDING_CLOSES | DNF_STARTED | DNF_START_REQUEST_PENDING)

#define DNF_ASYNC_REQUEST_PENDING     (DNF_START_REQUEST_PENDING | DNF_ENUMERATION_REQUEST_PENDING)

#define DNF_ASSIGN_RESOURCE_PHASE     (DNF_HAS_PROBLEM | DNF_HAS_PRIVATE_PROBLEM | DNF_DEVICE_GONE | DNF_REMOVE_PENDING_CLOSES | DNF_RESOURCE_ASSIGNED | DNF_RESOURCE_REPORTED |  \
                                       DNF_ASSIGNING_RESOURCES | DNF_NO_RESOURCE_REQUIRED)
#define DNF_HAS_RESOURCE              (DNF_RESOURCE_ASSIGNED | DNF_RESOURCE_REPORTED | \
                                       DNF_NO_RESOURCE_REQUIRED)

//
// PNP Bugcheck Subcodes
//
#define PNP_ERR_DUPLICATE_PDO                   1
#define PNP_ERR_INVALID_PDO                     2
#define PNP_ERR_BOGUS_ID                        3
#define PNP_ERR_PDO_ENUMERATED_AFTER_DELETION   4
#define PNP_ERR_ACTIVE_PDO_FREED                5

#define PNP_ERR_DEVICE_MISSING_FROM_EJECT_LIST  6
#define PNP_ERR_UNEXPECTED_ADD_RELATION_ERR     7



typedef NTSTATUS (*PENUM_CALLBACK)(
    IN PDEVICE_NODE DeviceNode,
    IN PVOID Context
    );

//
// Define workitem for add/start new device
//

typedef struct _NEW_DEVICE_WORK_ITEM {
    WORK_QUEUE_ITEM WorkItem;
    PDEVICE_OBJECT DeviceObject;
    PDRIVER_OBJECT DriverObject;
} NEW_DEVICE_WORK_ITEM, *PNEW_DEVICE_WORK_ITEM;

//
// Define callback routine for IopApplyFunctionToSubKeys &
// IopApplyFunctionToServiceInstances
//
typedef BOOLEAN (*PIOP_SUBKEY_CALLBACK_ROUTINE) (
    IN     HANDLE,
    IN     PUNICODE_STRING,
    IN OUT PVOID
    );

//
// Define context structures for Start and Add device services
//

#define NO_MORE_GROUP ((USHORT) -1)
#define SETUP_RESERVED_GROUP      0
#define BUS_DRIVER_GROUP          1

typedef struct _ADD_CONTEXT {
    USHORT GroupsToStart;
    USHORT GroupToStartNext;
    ULONG DriverStartType;
} ADD_CONTEXT, *PADD_CONTEXT;

typedef struct _START_CONTEXT {
    BOOLEAN LoadDriver;
    BOOLEAN NewDevice;
    ADD_CONTEXT AddContext;
} START_CONTEXT, *PSTART_CONTEXT;

//
// Resource translation and allocation related structures
//

typedef enum _RESOURCE_HANDLER_TYPE {
    ResourceHandlerNull,
    ResourceTranslator,
    ResourceArbiter,
    ResourceLegacyDeviceDetection
} RESOURCE_HANDLER_TYPE;

#define PI_MAXIMUM_RESOURCE_TYPE_TRACKED 15

//
// Internal Arbiters tracking structures
// Note the first three fields of PI_RESOURCE_ARBITER_ENTRY and PI_RESOURCE_TRANSLATOR_ENTRY
// must be the same.
//

typedef struct _PI_RESOURCE_ARBITER_ENTRY {
    LIST_ENTRY DeviceArbiterList;         // Link all the arbiters of a PDO.
    UCHAR ResourceType;
    PARBITER_INTERFACE ArbiterInterface;
    LIST_ENTRY ResourceList;
    LIST_ENTRY BestResourceList;
    LIST_ENTRY BestConfig;                // Link all the arbiters which produces the best logconf
    LIST_ENTRY ActiveArbiterList;         // Link all the arbiters under testing
    UCHAR State;
    BOOLEAN ResourcesChanged;
} PI_RESOURCE_ARBITER_ENTRY, *PPI_RESOURCE_ARBITER_ENTRY;

//
// Define PI_RESOURCE_ARBITER_ENTRY state
//

#define PI_ARBITER_HAS_SOMETHING 1
#define PI_ARBITER_TEST_FAILED   2

//
// Internal Translator tracking structures
//

typedef struct _PI_RESOURCE_TRANSLATOR_ENTRY {
    LIST_ENTRY DeviceTranslatorList;
    UCHAR ResourceType;
    PTRANSLATOR_INTERFACE TranslatorInterface;
    PDEVICE_NODE DeviceNode;
} PI_RESOURCE_TRANSLATOR_ENTRY, *PPI_RESOURCE_TRANSLATOR_ENTRY;

//
// IOP_RESOURCE_REQUEST
//

#define QUERY_RESOURCE_LIST                0
#define QUERY_RESOURCE_REQUIREMENTS        1

#define REGISTRY_ALLOC_CONFIG              1
#define REGISTRY_FORCED_CONFIG             2
#define REGISTRY_BOOT_CONFIG               4
#define REGISTRY_OVERRIDE_CONFIGVECTOR     1
#define REGISTRY_BASIC_CONFIGVECTOR        2

//
// An array of IOP_RESOURCE_REQUEST structures is used to anchor all the
// devices for which resource rerquirement is being attempted.
//

#define IOP_ASSIGN_RETRY              0x00000008    // Retry resource allocation later
#define IOP_ASSIGN_EXCLUDE            0x00000010    // internal IopAssign flag
#define IOP_ASSIGN_IGNORE             0x00000020    // ignore this request
#define IOP_ASSIGN_NO_REBALANCE       0x00000080    // no rebal if assign fails
#define IOP_ASSIGN_RESOURCES_RELEASED 0x00000100    // resources are released for rebalancing
#define IOP_ASSIGN_KEEP_CURRENT_CONFIG 0x00000200   // Indicate non-stopped rebalance.  We need to
                                                    //   preserved the current config.
#define IOP_ASSIGN_CLEAR_RESOURCE_REQUIREMENTS_CHANGE_FLAG \
                                      0x00000400

typedef struct _IOP_RESOURCE_REQUEST {
    PDEVICE_OBJECT                 PhysicalDevice;
    ULONG                          Flags;
    ARBITER_REQUEST_SOURCE         AllocationType;
    ULONG                          Priority;                   // 0 is highest priority
    PIO_RESOURCE_REQUIREMENTS_LIST ResourceRequirements;
    PVOID                          ReqList;                    // PREQ_LIST
    PCM_RESOURCE_LIST              ResourceAssignment;
    PCM_RESOURCE_LIST              TranslatedResourceAssignment;
    NTSTATUS                       Status;
} IOP_RESOURCE_REQUEST, *PIOP_RESOURCE_REQUEST;

//
// Misc
//

//
// Enumeration request type
//

typedef enum _DEVICE_REQUEST_TYPE {
    ReenumerateDeviceTree,
    ReenumerateDeviceOnly,
    ReenumerateBootDevices,
    RestartEnumeration,
    AssignResources,
    ResourceRequirementsChanged,
    StartDevice,
    ReenumerateRootDevices
} DEVICE_REQUEST_TYPE;

typedef struct _PI_DEVICE_REQUEST {
    LIST_ENTRY ListEntry;
    PDEVICE_OBJECT DeviceObject;
    DEVICE_REQUEST_TYPE RequestType;
    PKEVENT CompletionEvent;
    PNTSTATUS CompletionStatus;
} PI_DEVICE_REQUEST, *PPI_DEVICE_REQUEST;

#define CmResourceTypeReserved  0xf0



//
// This macro returns the pointer to the beginning of the data
// area of KEY_VALUE_FULL_INFORMATION structure.
// In the macro, k is a pointer to KEY_VALUE_FULL_INFORMATION structure.
//

#define KEY_VALUE_DATA(k) ((PCHAR)(k) + (k)->DataOffset)

//
// Save failure status info.
//

#if DBG_SCOPE
#define SAVE_FAILURE_INFO(DeviceNode, Status) (DeviceNode)->FailureStatus = (Status)
#else
#define SAVE_FAILURE_INFO(DeviceNode, Status)
#endif

#define IopDoesDevNodeHaveProblem(devnode)                          \
        ((devnode)->Flags & (DNF_HAS_PROBLEM | DNF_HAS_PRIVATE_PROBLEM))

#define IopIsDevNodeProblem(devnode, problem)                       \
        (((devnode)->Flags & DNF_HAS_PROBLEM) && (devnode)->Problem == (problem))

#define IopClearDevNodeProblem(devnode)                             \
        (devnode)->Flags &= ~DNF_HAS_PROBLEM;                       \
        (devnode)->Problem = 0;

#define IopSetDevNodeProblem(devnode, problem)                      \
        ASSERT(((devnode)->Flags & DNF_PROCESSED) || !((devnode)->Flags & DNF_ENUMERATED)); \
        ASSERT(!((devnode)->Flags & (DNF_STARTED | DNF_HAS_PROBLEM | DNF_HAS_PRIVATE_PROBLEM))); \
        ASSERT(problem != 0); \
        (devnode)->Flags |= DNF_HAS_PROBLEM;                        \
        (devnode)->Problem = (problem);

#define IopIsProblemReadonly(problem)            \
    ((problem) != CM_PROB_FAILED_INSTALL      && \
     (problem) != CM_PROB_FAILED_ADD          && \
     (problem) != CM_PROB_FAILED_START        && \
     (problem) != CM_PROB_NOT_CONFIGURED      && \
     (problem) != CM_PROB_NEED_RESTART        && \
     (problem) != CM_PROB_REINSTALL           && \
     (problem) != CM_PROB_REGISTRY            && \
     (problem) != CM_PROB_DISABLED)

//++
//
// VOID
// IopRegistryDataToUnicodeString(
//     OUT PUNICODE_STRING u,
//     IN  PWCHAR p,
//     IN  ULONG l
//     )
//
//--
#define IopRegistryDataToUnicodeString(u, p, l)  \
    {                                            \
        ULONG len;                               \
                                                 \
        PiRegSzToString((p), (l), &len, NULL);   \
        (u)->Length = (USHORT)len;               \
        (u)->MaximumLength = (USHORT)(l);        \
        (u)->Buffer = (p);                       \
    }

//
// Title Index to set registry key value
//

#define TITLE_INDEX_VALUE 0

#if DBG
#define PNP_ASSERT(condition, message) \
        if (!(condition)) { \
           DbgPrint((message)); \
           DbgBreakPoint(); \
        }
#else
#define PNP_ASSERT(condition, message)
#endif

//
// Default value of PnpDetectionEnabled
// Only used if the CCS\Control\Pnp key is absent or garbled
//

#define PNP_DETECTION_ENABLED_DEFAULT   TRUE

//
// Size of scratch buffer used in this module.
//

#define PNP_SCRATCH_BUFFER_SIZE 512
#define PNP_LARGE_SCRATCH_BUFFER_SIZE (PNP_SCRATCH_BUFFER_SIZE * 8)

//
// Define Device Instance Flags (used by IoQueryDeviceConfiguration apis)
//

#define DEVINSTANCE_FLAG_HWPROFILE_DISABLED 0x1
#define DEVINSTANCE_FLAG_PNP_ENUMERATED 0x2

//
// Define Enumeration Control Flags (used by IopApplyFunctionToSubKeys)
//

#define FUNCTIONSUBKEY_FLAG_IGNORE_NON_CRITICAL_ERRORS  0x1
#define FUNCTIONSUBKEY_FLAG_DELETE_SUBKEYS              0x2

//
// The following definitions are used in IoOpenDeviceInstanceKey
//

#define PLUGPLAY_REGKEY_DEVICE  1
#define PLUGPLAY_REGKEY_DRIVER  2
#define PLUGPLAY_REGKEY_CURRENT_HWPROFILE 4

//
// Define device extension for devices reported with IoReportDetectedDevice.
//

typedef struct _IOPNP_DEVICE_EXTENSION {
    PWCHAR CompatibleIdList;
    ULONG CompatibleIdListSize;
} IOPNP_DEVICE_EXTENSION, *PIOPNP_DEVICE_EXTENSION;

//
// Reserve Boot Resources
//

typedef struct _IOP_RESERVED_RESOURCES_RECORD IOP_RESERVED_RESOURCES_RECORD, *PIOP_RESERVED_RESOURCES_RECORD;

struct _IOP_RESERVED_RESOURCES_RECORD {
    PIOP_RESERVED_RESOURCES_RECORD  Next;
    PDEVICE_OBJECT                  DeviceObject;
    PCM_RESOURCE_LIST               ReservedResources;
};

//
// External References
//

//
// Init data
//
extern PVOID IopPnpScratchBuffer1;
extern PVOID IopPnpScratchBuffer2;
extern PCM_RESOURCE_LIST IopInitHalResources;
extern PDEVICE_NODE IopInitHalDeviceNode;
extern PIOP_RESERVED_RESOURCES_RECORD IopInitReservedResourceList;

//
// Regular data
//

//
// IopRootDeviceNode - the head of the PnP manager's device node tree.
//

extern PDEVICE_NODE IopRootDeviceNode;

//
// IopPnPDriverObject - the madeup driver object for pnp manager
//

extern PDRIVER_OBJECT IopPnPDriverObject;

//
// IopPnPSpinLock - spinlock for Pnp code.
//

extern KSPIN_LOCK IopPnPSpinLock;

//
// IopPnpDeleteRequestList - a link list of device removal requests to worker thread.
//

extern LIST_ENTRY IopPnpDeleteRequestList;

//
// IopPnpEnumerationRequestList - a link list of device enumeration requests to worker thread.
//

extern LIST_ENTRY IopPnpEnumerationRequestList;

//
// IopDeviceTreeLock - performs syncronization on the whole device node tree.
//      IopAcquireEnumerationLock acquires this lock shared then optionally
//                                acquires an exclusive lock on a devnode.
//      IopAcquireDeviceTreeLock acquires this lock exclusive
//

extern ERESOURCE IopDeviceTreeLock;

//
// PiEventQueueEmpty - Manual reset event which is set when the queue is empty
//

extern KEVENT PiEventQueueEmpty;

//
// PiEnumerationLock - to synchronize IoInvalidateDeviceRelations in boot phase.
//

extern KEVENT PiEnumerationLock;

//
// iopEnumerationCount - indicates how many devices are being enumerated.
//

extern LONG IopEnumerationCount;

//
// IopNumberDeviceNodes - Number of outstanding device nodes in the system
//

extern ULONG IopNumberDeviceNodes;

//
// PnPInitialized - A flag to indicate if PnP initialization is completed.
//

extern BOOLEAN PnPInitialized;

//
// PnPBootDriverInitialied
//

extern BOOLEAN PnPBootDriversInitialized;

//
// PnPBootDriverLoaded
//

extern BOOLEAN PnPBootDriversLoaded;

//
// IopBootConfigsReserved - Indicates whether we have reserved BOOT configs or not.
//

extern BOOLEAN IopBootConfigsReserved;

//
// IopResourcesReleased - a flag to indicate if a device is removed and its resources
//     are freed.  This is for reallocating resources for DNF_INSUFFICIENT_RESOURCES
//     devices.
//

extern BOOLEAN IopResourcesReleased;

//
// PnPDetectionEnabled - A flag to indicate if detection code can be executed
//

extern BOOLEAN PnPDetectionEnabled;

//
// PnpDefaultInterfaceTYpe - Use this if the interface type of resource list is unknown.
//

extern INTERFACE_TYPE PnpDefaultInterfaceType;

//
// PnpStartAsynOk - control how start irp should be handled. Synchronously or Asynchronously?
//

extern BOOLEAN PnpAsyncOk;

//
// IopPendingEjects - List of pending eject requests
//
extern LIST_ENTRY  IopPendingEjects;

//
// IopPendingSurpriseRemovals - List of pending surprise removal requests
//
extern LIST_ENTRY  IopPendingSurpriseRemovals;


NTSTATUS
IopAppendStringToValueKey (
    IN HANDLE Handle,
    IN PWSTR ValueName,
    IN PUNICODE_STRING String,
    IN BOOLEAN Create
    );

NTSTATUS
IopPrepareDriverLoading (
    IN PUNICODE_STRING KeyName,
    IN HANDLE KeyHandle,
    IN PIMAGE_NT_HEADERS Header
    );

NTSTATUS
IopRemoveStringFromValueKey (
    IN HANDLE Handle,
    IN PWSTR ValueName,
    IN PUNICODE_STRING String
    );

BOOLEAN
IopIsDuplicatedDevices(
    IN PCM_RESOURCE_LIST Configuration1,
    IN PCM_RESOURCE_LIST Configuration2,
    IN PHAL_BUS_INFORMATION BusInfo1 OPTIONAL,
    IN PHAL_BUS_INFORMATION BusInfo2 OPTIONAL
    );

NTSTATUS
IopMarkDuplicateDevice(
    IN PUNICODE_STRING TargetKeyName,
    IN ULONG TargetInstance,
    IN PUNICODE_STRING SourceKeyName,
    IN ULONG SourceInstance
    );

BOOLEAN
IopConcatenateUnicodeStrings (
    OUT PUNICODE_STRING Destination,
    IN  PUNICODE_STRING String1,
    IN  PUNICODE_STRING String2  OPTIONAL
    );

NTSTATUS
IopServiceInstanceToDeviceInstance (
    IN  HANDLE ServiceKeyHandle OPTIONAL,
    IN  PUNICODE_STRING ServiceKeyName OPTIONAL,
    IN  ULONG ServiceInstanceOrdinal,
    OUT PUNICODE_STRING DeviceInstanceRegistryPath OPTIONAL,
    OUT PHANDLE DeviceInstanceHandle OPTIONAL,
    IN  ACCESS_MASK DesiredAccess
    );

NTSTATUS
IopCreateRegistryKeyEx(
    OUT PHANDLE Handle,
    IN HANDLE BaseHandle OPTIONAL,
    IN PUNICODE_STRING KeyName,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG CreateOptions,
    OUT PULONG Disposition OPTIONAL
    );

NTSTATUS
IopOpenRegistryKeyEx(
    OUT PHANDLE Handle,
    IN HANDLE BaseHandle OPTIONAL,
    IN PUNICODE_STRING KeyName,
    IN ACCESS_MASK DesiredAccess
    );

NTSTATUS
IopCreateMadeupNode(
    IN PUNICODE_STRING ServiceKeyName,
    OUT PHANDLE ReturnedHandle,
    OUT PUNICODE_STRING KeyName,
    OUT PULONG InstanceOrdinal,
    IN BOOLEAN ResourceOwned
    );

NTSTATUS
IopInitializePlugPlayServices(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN ULONG Phase
    );

NTSTATUS
IopOpenServiceEnumKeys (
    IN PUNICODE_STRING ServiceKeyName,
    IN ACCESS_MASK DesiredAccess,
    OUT PHANDLE ServiceHandle OPTIONAL,
    OUT PHANDLE ServiceEnumHandle OPTIONAL,
    IN BOOLEAN CreateEnum
    );

NTSTATUS
IopOpenCurrentHwProfileDeviceInstanceKey(
    OUT PHANDLE Handle,
    IN  PUNICODE_STRING ServiceKeyName,
    IN  ULONG Instance,
    IN  ACCESS_MASK DesiredAccess,
    IN  BOOLEAN Create
    );

NTSTATUS
IopGetDeviceInstanceCsConfigFlags(
    IN PUNICODE_STRING DeviceInstance,
    OUT PULONG CsConfigFlags
    );

NTSTATUS
IopGetServiceInstanceCsConfigFlags(
    IN PUNICODE_STRING ServiceKeyName,
    IN ULONG Instance,
    OUT PULONG CsConfigFlags
    );

NTSTATUS
IopSetServiceInstanceCsConfigFlags(
    IN PUNICODE_STRING ServiceKeyName,
    IN ULONG Instance,
    IN ULONG CsConfigFlags
    );

NTSTATUS
IopApplyFunctionToSubKeys(
    IN HANDLE BaseHandle OPTIONAL,
    IN PUNICODE_STRING KeyName,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG Flags,
    IN PIOP_SUBKEY_CALLBACK_ROUTINE SubKeyCallbackRoutine,
    IN OUT PVOID Context
    );

NTSTATUS
IopRegMultiSzToUnicodeStrings(
    IN PKEY_VALUE_FULL_INFORMATION KeyValueInformation,
    IN PUNICODE_STRING *UnicodeStringList,
    OUT PULONG UnicodeStringCount
    );


NTSTATUS
IopApplyFunctionToServiceInstances(
    IN HANDLE ServiceKeyHandle OPTIONAL,
    IN PUNICODE_STRING ServiceKeyName OPTIONAL,
    IN ACCESS_MASK DesiredAccess,
    IN BOOLEAN IgnoreNonCriticalErrors,
    IN PIOP_SUBKEY_CALLBACK_ROUTINE DevInstCallbackRoutine,
    IN OUT PVOID Context,
    OUT PULONG ServiceInstanceOrdinal OPTIONAL
    );

VOID
IopFreeUnicodeStringList(
    IN PUNICODE_STRING UnicodeStringList,
    IN ULONG StringCount
    );

NTSTATUS
IopDriverLoadingFailed(
    IN HANDLE KeyHandle OPTIONAL,
    IN PUNICODE_STRING KeyName OPTIONAL
    );

NTSTATUS
IopReadDeviceConfiguration (
    IN HANDLE Handle,
    IN ULONG Flags,
    OUT PCM_RESOURCE_LIST *CmResource,
    OUT PULONG Length
    );

BOOLEAN
IopIsFirmwareMapperDevicePresent (
    IN HANDLE KeyHandle
    );

#define IopAcquireEnumerationLock(_devnode_)                        \
    ExAcquireResourceShared(&IopDeviceTreeLock, TRUE);              \
    if ((_devnode_)) {                                              \
        KeWaitForSingleObject( &((PDEVICE_NODE)(_devnode_))->EnumerationMutex,      \
                               Executive,                           \
                               KernelMode,                          \
                               FALSE,                               \
                               NULL );                              \
    }

#define IopReleaseEnumerationLock(_devnode_)                        \
    if ((_devnode_)) {                                              \
        KeSetEvent( &((PDEVICE_NODE)(_devnode_))->EnumerationMutex, \
                    0,                                              \
                    FALSE );                                        \
    }                                                               \
    ExReleaseResource(&IopDeviceTreeLock);


#define IopReleaseEnumerationLockForThread(_devnode_, _thread_)     \
    if ((_devnode_)) {                                              \
        KeSetEvent( &((PDEVICE_NODE)(_devnode_))->EnumerationMutex, \
                    0,                                              \
                    FALSE );                                        \
    }                                                               \
    ExReleaseResourceForThreadLite(&IopDeviceTreeLock, _thread_);

#define IopAcquireDeviceTreeLock() ExAcquireResourceExclusive(&IopDeviceTreeLock, TRUE)
#define IopReleaseDeviceTreeLock() ExReleaseResource(&IopDeviceTreeLock)

VOID
IopInsertTreeDeviceNode (
    IN PDEVICE_NODE     ParentNode,
    IN PDEVICE_NODE     DeviceNode
    );

VOID
IopRemoveTreeDeviceNode (
    IN PDEVICE_NODE     DeviceNode
    );

PDEVICE_NODE
IopAllocateDeviceNode(
    IN PDEVICE_OBJECT PhysicalDeviceObject
    );

#if DBG
VOID
IopCheckDeviceNodeTree (
    IN PDEVICE_OBJECT TargetDevice OPTIONAL,
    IN PDEVICE_NODE   TargetNode OPTIONAL
    );
#endif

NTSTATUS
IopForAllDeviceNodes(
    IN PENUM_CALLBACK Callback,
    IN PVOID Context
    );

ULONG
IopDetermineResourceListSize(
    IN PCM_RESOURCE_LIST ResourceList
    );

NTSTATUS
IopQueryDeviceConfigurationVector(
    IN PUNICODE_STRING ServiceKeyName,
    IN ULONG InstanceOrdinal,
    OUT PULONG DeviceInstanceFlags,
    OUT PIO_RESOURCE_REQUIREMENTS_LIST ConfigurationVector,
    IN ULONG BufferSize,
    OUT PULONG ActualBufferSize
    );

PDRIVER_OBJECT
IopReferenceDriverObjectByName (
    IN PUNICODE_STRING DriverName
    );

PDEVICE_OBJECT
IopDeviceObjectFromDeviceInstance (
    IN HANDLE DeviceInstanceHandle      OPTIONAL,
    IN PUNICODE_STRING DeviceInstance   OPTIONAL
    );

NTSTATUS
IopDeviceObjectToDeviceInstance (
    IN PDEVICE_OBJECT DeviceObject,
    IN PHANDLE DeviceInstanceHandle,
    IN  ACCESS_MASK DesiredAccess
    );

BOOLEAN
IopIsDeviceInstanceEnabled(
    IN HANDLE DeviceInstanceHandle,
    IN PUNICODE_STRING DeviceInstance,
    IN BOOLEAN DisableIfEnabled
    );

NTSTATUS
IopAddDevicesToBootDriver (
   IN PDRIVER_OBJECT DriverObject
   );

USHORT
IopProcessAddDevices (
   IN PDEVICE_NODE DeviceNode,
   IN USHORT StartOrder,
   IN ULONG DriverStartType
   );

BOOLEAN
IopProcessAssignResources (
   IN PDEVICE_NODE DeviceNode,
   IN BOOLEAN Reallocation,
   IN BOOLEAN BootConfigsOK
   );

VOID
IopProcessStartDevices (
   IN PDEVICE_NODE DeviceNode,
   IN PSTART_CONTEXT StartContext
   );

VOID
IopStartDevice (
    IN PDEVICE_OBJECT TargetDevice
    );

NTSTATUS
IopEjectDevice(
    IN PDEVICE_OBJECT DeviceObject,
    PPENDING_RELATIONS_LIST_ENTRY PendingEntry
    );

NTSTATUS
IopRemoveDevice (
    IN PDEVICE_OBJECT TargetDevice,
    IN ULONG IrpMinorCode
    );

NTSTATUS
IopQueryDeviceRelations(
    IN DEVICE_RELATION_TYPE Relations,
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN AsyncOk,
    OUT PDEVICE_RELATIONS *DeviceRelations
    );

NTSTATUS
IopQueryDeviceState(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
IopQueryDeviceSerialNumber (
    IN PDEVICE_OBJECT DeviceObject,
    OUT PWCHAR *SerialNumber
    );

NTSTATUS
IopForAllChildDeviceNodes(
    IN PDEVICE_NODE Parent,
    IN PENUM_CALLBACK Callback,
    IN PVOID Context
    );

NTSTATUS
IopCleanupDeviceRegistryValues (
    IN PUNICODE_STRING InstancePath,
    IN BOOLEAN KeepReference
    );

NTSTATUS
IopQueryDeviceId (
    IN PDEVICE_OBJECT DeviceObject,
    OUT PWCHAR *DeviceId
    );

NTSTATUS
IopQueryUniqueId (
    IN PDEVICE_OBJECT DeviceObject,
    OUT PWCHAR *UniqueId
    );

NTSTATUS
IopMakeGloballyUniqueId(
    IN PDEVICE_OBJECT DeviceObject,
    IN PWCHAR           UniqueId,
    OUT PWCHAR         *GloballyUniqueId
    );


NTSTATUS
IopQueryCompatibleIds (
    IN PDEVICE_OBJECT DeviceObject,
    IN BUS_QUERY_ID_TYPE IdType,
    OUT PWCHAR *CompatibleIds,
    OUT ULONG *Length
    );

NTSTATUS
IopQueryDeviceResources (
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG ResourceType,
    OUT PVOID *Resource,
    OUT ULONG *Length
    );

NTSTATUS
IopGetDeviceResourcesFromRegistry (
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG ResourceType,
    IN ULONG Preference,
    OUT PVOID *Resource,
    OUT PULONG Length
    );

VOID
IopResourceRequirementsChanged(
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN BOOLEAN StopRequired
    );

NTSTATUS
IopReleaseDeviceResources (
    IN PDEVICE_NODE DeviceNode,
    IN BOOLEAN  ReserveResources
    );

NTSTATUS
IopPnPAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
IopProcessCriticalDevice(
    IN PDEVICE_NODE DeviceNode
    );

NTSTATUS
IopPnPDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
IopPowerDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

VOID
IopDestroyDeviceNode (
    PDEVICE_NODE DeviceNode
    );

NTSTATUS
IopStartAndEnumerateDevice(
    IN PDEVICE_NODE DeviceNode,
    IN PSTART_CONTEXT StartContext
    );

NTSTATUS
IopCallDriverAddDevice (
    IN PDEVICE_NODE DeviceNode,
    IN BOOLEAN LoadDriver,
    IN PADD_CONTEXT AddContext
    );

NTSTATUS
IopStartDriverDevices(
    IN PDRIVER_OBJECT DriverObject
    );

BOOLEAN
IopIsLegacyDriver (
    IN PDRIVER_OBJECT DriverObject
    );

VOID
IopNewDevice(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
IopProcessNewDeviceNode(
    IN OUT PDEVICE_NODE DeviceNode
    );

NTSTATUS
IopSynchronousCall(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIO_STACK_LOCATION TopStackLocation,
    OUT PVOID *Information
    );

NTSTATUS
IopFilterResourceRequirementsList (
    IN PIO_RESOURCE_REQUIREMENTS_LIST IoList,
    IN PCM_RESOURCE_LIST CmList,
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST *FilteredList,
    OUT PBOOLEAN ExactMatch
    );

NTSTATUS
IopMergeFilteredResourceRequirementsList (
    IN PIO_RESOURCE_REQUIREMENTS_LIST IoList1,
    IN PIO_RESOURCE_REQUIREMENTS_LIST IoList2,
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST *MergedList
    );

NTSTATUS
IopMergeCmResourceLists (
    IN PCM_RESOURCE_LIST List1,
    IN PCM_RESOURCE_LIST List2,
    IN OUT PCM_RESOURCE_LIST *MergedList
    );

PIO_RESOURCE_REQUIREMENTS_LIST
IopCmResourcesToIoResources (
    IN ULONG SlotNumber,
    IN PCM_RESOURCE_LIST CmResourceList,
    IN ULONG Priority
    );

NTSTATUS
IopReportResourceListToPnp(
    IN PDRIVER_OBJECT DriverObject OPTIONAL,
    IN PDEVICE_OBJECT DeviceObject OPTIONAL,
    IN PCM_RESOURCE_LIST ResourceList,
    IN ULONG ListSize,
    IN BOOLEAN Translated
    );

NTSTATUS
IopAllocateResources(
    IN PULONG DeviceCountP,
    IN OUT PIOP_RESOURCE_REQUEST *AssignTablePP,
    IN BOOLEAN Locked,
    IN BOOLEAN BootConfigsOK
    );

VOID
IopReallocateResources(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
IopWriteResourceList(
    IN HANDLE ResourceMapKey,
    IN PUNICODE_STRING ClassName,
    IN PUNICODE_STRING DriverName,
    IN PUNICODE_STRING DeviceName,
    IN PCM_RESOURCE_LIST ResourceList,
    IN ULONG ResourceListSize
    );

VOID
IopRemoveResourceListFromPnp(
    IN PLIST_ENTRY ResourceList
    );

NTSTATUS
IopWriteAllocatedResourcesToRegistry (
    IN PDEVICE_NODE DeviceNode,
    IN PCM_RESOURCE_LIST ResourceList,
    IN ULONG Length
    );

USHORT
IopGetGroupOrderIndex (
    IN HANDLE ServiceHandle
    );

VOID
IopDeleteLegacyKey(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
IopOpenDeviceParametersSubkey(
    OUT HANDLE *ParamKeyHandle,
    IN  HANDLE ParentKeyHandle,
    IN  PUNICODE_STRING SubKeyString,
    IN  ACCESS_MASK DesiredAccess
    );

NTSTATUS
IopRequestDeviceAction(
    IN PDEVICE_OBJECT DeviceObject              OPTIONAL,
    IN DEVICE_REQUEST_TYPE RequestType,
    IN PKEVENT CompletionEvent                  OPTIONAL,
    IN PNTSTATUS CompletionStatus               OPTIONAL
    );

NTSTATUS
IopRequestDeviceRemoval(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG Problem
    );

NTSTATUS
IopRestartDeviceNode(
    IN PDEVICE_NODE DeviceNode
    );

NTSTATUS
IopDeleteKeyRecursive(
    IN HANDLE SubKeyHandle,
    IN PWCHAR SubKeyName
    );

NTSTATUS
IopQueryPnpBusInformation (
    IN PDEVICE_OBJECT DeviceObject,
    OUT LPGUID InterfaceGuid           OPTIONAL,
    OUT INTERFACE_TYPE *InterfaceType  OPTIONAL,
    OUT ULONG *BusNumber               OPTIONAL
    );

NTSTATUS
IopQueryLegacyBusInformation (
    IN PDEVICE_OBJECT DeviceObject,
    OUT LPGUID InterfaceGuid           OPTIONAL,
    OUT INTERFACE_TYPE *InterfaceType  OPTIONAL,
    OUT ULONG *BusNumber               OPTIONAL
    );

NTSTATUS
IopGetRootDevices (
    PDEVICE_RELATIONS *DeviceRelations
    );

NTSTATUS
IopLockDeviceRemovalRelations(
    IN PDEVICE_OBJECT DeviceObject,
    IN PLUGPLAY_DEVICE_DELETE_TYPE OperationCode,
    OUT PRELATION_LIST *RelationsList,
    IN BOOLEAN IsKernelInitiated
    );

NTSTATUS
IopDeleteLockedDeviceNodes(
    IN PDEVICE_OBJECT DeviceObject,
    IN PRELATION_LIST RelationsList,
    IN PLUGPLAY_DEVICE_DELETE_TYPE OperationCode,
    IN BOOLEAN IsKernelInitiated,
    IN BOOLEAN ProcessIndirectDescendants,
    IN ULONG Problem,
    OUT PDEVICE_OBJECT *VetoingDevice OPTIONAL
    );

NTSTATUS
IopUnlockDeviceRemovalRelations(
    IN PDEVICE_OBJECT       DeviceObject,
    IN PRELATION_LIST       RelationsList,
    IN UNLOCK_UNLINK_ACTION UnlinkAction
    );

NTSTATUS
IopInvalidateRelationsInList(
    PRELATION_LIST RelationsList,
    BOOLEAN OnlyIndirectDescendants,
    BOOLEAN UnlockDevNode,
    BOOLEAN RestartDevNode
    );

VOID
IopChainDereferenceComplete(
    IN PDEVICE_OBJECT PhysicalDeviceObject
    );

BOOLEAN
IopQueuePendingEject(
    PPENDING_RELATIONS_LIST_ENTRY Entry
    );

VOID
IopProcessCompletedEject(
    IN PVOID Context
    );

BOOLEAN
IopQueuePendingSurpriseRemoval(
    IN PDEVICE_OBJECT DeviceObject,
    IN PRELATION_LIST List,
    IN ULONG Problem
    );

BOOLEAN
IopIsAnyDeviceInstanceEnabled(
    IN PUNICODE_STRING ServiceKeyName,
    IN HANDLE ServiceHandle,
    IN BOOLEAN LegacyIncluded
    );

NTSTATUS
IopQueryResourceHandlerInterface(
    IN RESOURCE_HANDLER_TYPE HandlerType,
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR ResourceType,
    IN OUT PVOID *Interface
    );

NTSTATUS
IopQueryReconfiguration(
    IN UCHAR Request,
    IN PDEVICE_OBJECT DeviceObject
    );

PDEVICE_NODE
IopFindBusDeviceNode (
    IN PDEVICE_NODE DeviceNode,
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber
    );

NTSTATUS
IopLegacyResourceAllocation (
    IN ARBITER_REQUEST_SOURCE AllocationType,
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIO_RESOURCE_REQUIREMENTS_LIST ResourceRequirements,
    IN OUT PCM_RESOURCE_LIST *AllocatedResources OPTIONAL
    );

NTSTATUS
IoReportResourceUsageInternal(
    IN ARBITER_REQUEST_SOURCE AllocationType,
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
IopDuplicateDetection (
    IN INTERFACE_TYPE LegacyBusType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    OUT PDEVICE_NODE *DeviceNode
    );

#if 0
NTSTATUS
IopTranslateResourceList(
    IN PDEVICE_NODE DeviceNode   OPTIONAL,
    IN PCM_RESOURCE_LIST ResourceList,
    OUT PCM_RESOURCE_LIST *TranslatedList
    );
#endif

PDRIVER_OBJECT
IopLoadBootFilterDriver (
    IN PUNICODE_STRING DriverName,
    IN ULONG GroupIndex
    );

NTSTATUS
IopDeviceNodeCapabilitiesToRegistry (
    IN PDEVICE_NODE DeviceNode
    );

NTSTATUS
IopDeviceCapabilitiesToRegistry (
    IN PDEVICE_NODE DeviceNode,
    IN PDEVICE_CAPABILITIES Capabilities
    );

NTSTATUS
IopQueryDeviceCapabilities(
    IN PDEVICE_NODE DeviceNode,
    OUT PDEVICE_CAPABILITIES Capabilities
    );

VOID
IopIncDisableableDepends(
    IN OUT PDEVICE_NODE DeviceNode
    );

VOID
IopDecDisableableDepends(
    IN OUT PDEVICE_NODE DeviceNode
    );

NTSTATUS
IopQueryDockRemovalInterface(
    IN      PDEVICE_OBJECT  DeviceObject,
    IN OUT  PDOCK_INTERFACE *DockInterface
    );

#ifndef FIELD_SIZE
#define FIELD_SIZE(type, field) (sizeof(((type *)0)->field))
#endif

#define IopDeviceNodeFlagsToCapabilities(DeviceNode) \
     ((PDEVICE_CAPABILITIES) (((PUCHAR) (&(DeviceNode)->CapabilityFlags)) - \
                              FIELD_OFFSET(DEVICE_CAPABILITIES, Version) - \
                              FIELD_SIZE(DEVICE_CAPABILITIES, Version)))

typedef
NTSTATUS
(*PIO_RESERVE_RESOURCES_ROUTINE) (
    IN ARBITER_REQUEST_SOURCE ArbiterRequestSource,
    IN PDEVICE_OBJECT DeviceObject,
    IN PCM_RESOURCE_LIST BootResources
    );

NTSTATUS
IopReserveBootResources (
    IN ARBITER_REQUEST_SOURCE ArbiterRequestSource,
    IN PDEVICE_OBJECT DeviceObject,
    IN PCM_RESOURCE_LIST BootResources
    );

NTSTATUS
IopAllocateBootResources (
    IN ARBITER_REQUEST_SOURCE ArbiterRequestSource,
    IN PDEVICE_OBJECT DeviceObject,
    IN PCM_RESOURCE_LIST BootResources
    );

NTSTATUS
IopReserveLegacyBootResources(
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG BusNumber
    );

extern PIO_RESERVE_RESOURCES_ROUTINE IopReserveResourcesRoutine;

//
// Conflict detection declarations
//

NTSTATUS
IopQueryConflictList(
    PDEVICE_OBJECT        PhysicalDeviceObject,
    IN PCM_RESOURCE_LIST  ResourceList,
    IN ULONG              ResourceListSize,
    OUT PPLUGPLAY_CONTROL_CONFLICT_LIST ConflictList,
    IN ULONG              ConflictListSize,
    IN ULONG              Flags
    );

//
// Firmware mapper external declarations.
//

VOID
MapperProcessFirmwareTree(
    IN BOOLEAN OnlyProcessSerialPorts
    );

VOID
MapperConstructRootEnumTree(
    IN BOOLEAN CreatePhantomDevices
    );

VOID
MapperFreeList(
    VOID
    );

NTSTATUS
EisaBuildEisaDeviceNode(
    VOID
    );

VOID
MapperPhantomizeDetectedComPorts(
    VOID
    );
//
// BUGBUG- for now we need the following dbg routines in free build also such
//         that we can track resource allocation by enabling a dbg flag.
//#if DBG

VOID
IopDumpCmResourceList (
    IN PCM_RESOURCE_LIST CmList
    );

VOID
IopDumpCmResourceDescriptor (
    IN PUCHAR Indent,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Desc
    );

VOID
IopDumpAllocatedSystemResources(
    IN UCHAR ResourceType
    );

// #endif

//
// General utility macros
//

//
// This macros calculates the size in bytes of a constant string
//
//  ULONG
//  IopConstStringSize(
//      IN CONST PWSTR String
//      );
//

#define IopConstStringSize(String)          ( sizeof(String) - sizeof(UNICODE_NULL) )

//
// This macros calculates the number of characters of a constant string
//
//  ULONG
//  IopConstStringLength(
//      IN CONST PWSTR String
//      );
//

#define IopConstStringLength(String)        ( ( sizeof(String) - sizeof(UNICODE_NULL) ) / sizeof(WCHAR) )

//
// Kernel Mode (KM) to User Mode (UM) SymbolicLinkName Conversion and Vice Versa
// ie.  \??\ <-> \\?\
//

//
//  VOID
//  IopKMToUMSymbolicLinkName(
//      IN UNICODE_STRING String
//      );
//

#define IopKMToUMSymbolicLinkName(String) \
    ASSERT(String) \
    ASSERT(String->Length > 4) \
    String[1] = L'\\'

//
//  VOID
//  IopUMToKMSymbolicLinkName(
//      IN UNICODE_STRING String
//      );
//

#define IopUMToKMSymbolicLinkName(String) \
    ASSERT(String) \
    ASSERT(String->Length > 4) \
    String[1] = L'?'

//
// Kernel mode notification
//

//
// This macros maps a guid to a hash value based on the number of hash
// buckets we are using.  It does this by treating the  guid as an array of
// 4 ULONGs, suming them and MOD by the number of hash buckets we are using.
//
//  ULONG
//  IopHashGuid(
//      LPGUID Guid
//      );
//

#define IopHashGuid(_Guid) \
            ( ( ((PULONG)_Guid)[0] + ((PULONG)_Guid)[1] + ((PULONG)_Guid)[2] \
                + ((PULONG)_Guid)[3]) % NOTIFY_DEVICE_CLASS_HASH_BUCKETS)



//  This macros abstracts
//
//  VOID
//  IopAcquireNotifyLock(
//      PFAST_MUTEX Lock
//      )

#define IopAcquireNotifyLock(Lock)     ExAcquireFastMutex(Lock);

/*
VOID
IopReleaseNotifyLock(
    PFAST_MUTEX Lock
    )
*/
#define IopReleaseNotifyLock(Lock)     ExReleaseFastMutex(Lock);


//  BOOLEAN
//  IopCompareGuid(
//      IN LPGUID guid1,
//      IN LPGUID guid2
//      );

#define IopCompareGuid(g1, g2)  ( (g1) == (g2) \
                                    ? TRUE \
                                    : RtlCompareMemory( (g1), (g2), sizeof(GUID) ) == sizeof(GUID) \
                                    )

VOID
IopInitializePlugPlayNotification(
    VOID
    );

NTSTATUS
IopNotifySetupDeviceArrival(
        PDEVICE_OBJECT PhysicalDeviceObject,    // PDO of the device
        HANDLE EnumEntryKey,                    // Handle into the enum branch of the registry for this device
        BOOLEAN InstallDriver                   // Should setup attempt to install a driver
);

NTSTATUS
IopRequestHwProfileChangeNotification(
    IN   LPGUID                      EventGuid,
    IN   PROFILE_NOTIFICATION_TIME   NotificationTime,
    OUT  PPNP_VETO_TYPE              VetoType           OPTIONAL,
    OUT  PUNICODE_STRING             VetoName           OPTIONAL
    );

NTSTATUS
IopNotifyTargetDeviceChange(
    LPCGUID EventGuid,
    PDEVICE_OBJECT DeviceObject,
    PVOID NotificationStructure,
    PDRIVER_OBJECT *VetoingDriver
    );

NTSTATUS
IopGetRelatedTargetDevice(
    IN PFILE_OBJECT FileObject,
    OUT PDEVICE_NODE *DeviceNode
    );

NTSTATUS
IopNotifyDeviceClassChange(
    LPGUID EventGuid,
    LPGUID ClassGuid,
    PUNICODE_STRING SymbolicLinkName
    );

NTSTATUS
IopRegisterDeviceInterface(
    IN PUNICODE_STRING DeviceInstanceName,
    IN CONST GUID *InterfaceClassGuid,
    IN PUNICODE_STRING ReferenceString      OPTIONAL,
    IN BOOLEAN UserModeFormat,
    OUT PUNICODE_STRING SymbolicLinkName
    );

NTSTATUS
IopUnregisterDeviceInterface(
    IN PUNICODE_STRING SymbolicLinkName
    );

NTSTATUS
IopRemoveDeviceInterfaces(
    IN PUNICODE_STRING DeviceInstancePath
    );

NTSTATUS
IopGetDeviceInterfaces(
    IN CONST GUID *InterfaceClassGuid,
    IN PUNICODE_STRING DevicePath   OPTIONAL,
    IN ULONG Flags,
    IN BOOLEAN UserModeFormat,
    OUT PWSTR *SymbolicLinkList,
    OUT PULONG SymbolicLinkListSize OPTIONAL
    );

NTSTATUS
IopDoDeferredSetInterfaceState(
    IN PDEVICE_NODE DeviceNode
    );

NTSTATUS
IopProcessSetInterfaceState(
    IN PUNICODE_STRING SymbolicLinkName,
    IN BOOLEAN Enable,
    IN BOOLEAN DeferNotStarted
    );

NTSTATUS
IopReplaceSeperatorWithPound(
    OUT PUNICODE_STRING OutString,
    IN PUNICODE_STRING InString
    );

NTSTATUS
IopNotifyHwProfileChange(
    IN  LPGUID           EventGuid,
    OUT PPNP_VETO_TYPE   VetoType    OPTIONAL,
    OUT PUNICODE_STRING  VetoName    OPTIONAL
    );

VOID
IopUncacheInterfaceInformation(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
IopProcessNewProfile(
    VOID
    );

VOID
IopProcessNewProfileWorker(
    IN PVOID Context
    );

NTSTATUS
IopProcessNewProfileStateCallback(
    IN PDEVICE_NODE DeviceNode,
    IN PVOID Context
    );

//
// Notify entry header - all notify entries have these
//

typedef struct _NOTIFY_ENTRY_HEADER {

    //
    // List Entry structure
    //

    LIST_ENTRY ListEntry;

    //
    // Notification event category for this notification entry.
    //

    IO_NOTIFICATION_EVENT_CATEGORY EventCategory;

    //
    // Callback routine passed in at registration
    //

    PDRIVER_NOTIFICATION_CALLBACK_ROUTINE Callback;

    //
    // Context passed in at registration
    //

    PVOID Context;

    //
    // Driver object of the driver that registered for notifications.  Required
    // so we can dereference it when it unregisters
    //

    PDRIVER_OBJECT DriverObject;

    //
    // RefCount is the number of outstanding pointers to the node and avoids
    // deletion while another notification is taking place
    //

    USHORT RefCount;

    //
    // Unregistered is set if this notification has been unregistered but cannot
    // be removed from the list because other entities are using it
    //

    BOOLEAN Unregistered;

    //
    // Lock is a pointer to the fast mutex which is used to synchronise access
    // to the list this node is a member of and is required so that the correct
    // list can be locked during IoUnregisterPlugPlayNotification.  If no locking
    // is required it is NULL
    //

    PFAST_MUTEX Lock;

} NOTIFY_ENTRY_HEADER, *PNOTIFY_ENTRY_HEADER;


//
// Data to store for each target device registration
//

typedef struct _TARGET_DEVICE_NOTIFY_ENTRY {

    //
    // Header entries
    //

    LIST_ENTRY ListEntry;
    IO_NOTIFICATION_EVENT_CATEGORY EventCategory;
    PDRIVER_NOTIFICATION_CALLBACK_ROUTINE Callback;
    PVOID Context;
    PDRIVER_OBJECT DriverObject;
    USHORT RefCount;
    BOOLEAN Unregistered;
    PFAST_MUTEX Lock;

    //
    // FileObject - the file object of the target device we are interested in
    //

    PFILE_OBJECT FileObject;

    //
    // PhysicalDeviceObject -- the PDO upon which this notification is hooked.
    // We need to keep this here, so we can dereference it when the refcount
    // on this notification entry drops to zero.
    //

    PDEVICE_OBJECT PhysicalDeviceObject;

} TARGET_DEVICE_NOTIFY_ENTRY, *PTARGET_DEVICE_NOTIFY_ENTRY;

//
// Data to store for each device class registration
//

typedef struct _DEVICE_CLASS_NOTIFY_ENTRY {

    //
    // Header entries
    //

    LIST_ENTRY ListEntry;
    IO_NOTIFICATION_EVENT_CATEGORY EventCategory;
    PDRIVER_NOTIFICATION_CALLBACK_ROUTINE Callback;
    PVOID Context;
    PDRIVER_OBJECT DriverObject;
    USHORT RefCount;
    BOOLEAN Unregistered;
    PFAST_MUTEX Lock;

    //
    // ClassGuid - the guid of the device class we are interested in
    //

    GUID ClassGuid;

} DEVICE_CLASS_NOTIFY_ENTRY, *PDEVICE_CLASS_NOTIFY_ENTRY;

//
// Data to store for registration of the Reserved (ie setupdd.sys) variety
//

typedef struct _SETUP_NOTIFY_DATA {

    //
    // Header entries
    //

    LIST_ENTRY ListEntry;
    IO_NOTIFICATION_EVENT_CATEGORY EventCategory;
    PDRIVER_NOTIFICATION_CALLBACK_ROUTINE Callback;
    PVOID Context;
    PDRIVER_OBJECT DriverObject;
    USHORT RefCount;
    BOOLEAN Unregistered;
    PFAST_MUTEX Lock;

} SETUP_NOTIFY_DATA, *PSETUP_NOTIFY_DATA;


//
// Data to store for registration for HardwareProfileChange Events
//

typedef struct _HWPROFILE_NOTIFY_ENTRY {

    //
    // Header entries
    //

    LIST_ENTRY ListEntry;
    IO_NOTIFICATION_EVENT_CATEGORY EventCategory;
    PDRIVER_NOTIFICATION_CALLBACK_ROUTINE Callback;
    PVOID Context;
    PDRIVER_OBJECT DriverObject;
    USHORT RefCount;
    BOOLEAN Unregistered;
    PFAST_MUTEX Lock;

} HWPROFILE_NOTIFY_ENTRY, *PHWPROFILE_NOTIFY_ENTRY;

#define PNP_NOTIFICATION_VERSION            1
#define NOTIFY_DEVICE_CLASS_HASH_BUCKETS    13

//
// IopMaxDeviceNodeLevel - Level number of the DeviceNode deepest in the tree
//
extern ULONG       IopMaxDeviceNodeLevel;
extern ULONG       IoDeviceNodeTreeSequence;

//
// Global notification data
//

extern FAST_MUTEX IopDeviceClassNotifyLock;
extern LIST_ENTRY IopDeviceClassNotifyList[];
extern PSETUP_NOTIFY_DATA IopSetupNotifyData;
extern FAST_MUTEX IopTargetDeviceNotifyLock;
extern LIST_ENTRY IopProfileNotifyList;
extern FAST_MUTEX IopHwProfileNotifyLock;

VOID
IopProcessDeferredRegistrations(
    VOID
    );

//
// Generic buffer management
//

typedef struct _BUFFER_INFO {

    //
    // Buffer - pointer to the start of the buffer
    //

    PCHAR Buffer;

    //
    // Current - Pointer to the current position in the buffer
    //

    PCHAR Current;

    //
    // MaxSize - Maximum size of the buffer in bytes
    //

    ULONG MaxSize;

} BUFFER_INFO, *PBUFFER_INFO;

typedef struct _BUS_TYPE_GUID_LIST {

    //
    // Number of allocated guid slots in the table.
    //
    ULONG Count;

    //
    // Number of entries used so far.
    //
    FAST_MUTEX Lock;

    //
    // Array of bus type guids
    //
    GUID Guid[1];

} BUS_TYPE_GUID_LIST, *PBUS_TYPE_GUID_LIST;

//
// List of queried bus type guids
//
extern PBUS_TYPE_GUID_LIST IopBusTypeGuidList;

//
// Arbiter entry points
//

NTSTATUS
IopPortInitialize(
    VOID
    );

NTSTATUS
IopMemInitialize(
    VOID
    );

NTSTATUS
IopIrqInitialize(
    VOID
    );

NTSTATUS
IopDmaInitialize(
    VOID
    );

NTSTATUS
IopBusNumberInitialize(
    VOID
    );

//
// Arbiter state
//

extern ARBITER_INSTANCE IopRootPortArbiter;
extern ARBITER_INSTANCE IopRootMemArbiter;
extern ARBITER_INSTANCE IopRootIrqArbiter;
extern ARBITER_INSTANCE IopRootDmaArbiter;
extern ARBITER_INSTANCE IopRootBusNumberArbiter;

//
// Buffer management routines.
//

NTSTATUS
IopAllocateBuffer(
    IN PBUFFER_INFO Info,
    IN ULONG Size
    );

NTSTATUS
IopResizeBuffer(
    IN PBUFFER_INFO Info,
    IN ULONG NewSize,
    IN BOOLEAN CopyContents
    );

VOID
IopFreeBuffer(
    IN PBUFFER_INFO Info
    );


//
// UnicodeString management routines.
//

NTSTATUS
IopAllocateUnicodeString(
    IN OUT PUNICODE_STRING String,
    IN USHORT Length
    );

VOID
IopFreeAllocatedUnicodeString(
    PUNICODE_STRING String
    );

//
// Misc.
//

NTSTATUS
PnPBiosGetBiosInfo(
    OUT PVOID *BiosInfo,
    OUT ULONG *BiosInfoLength
    );

BOOLEAN
IopFixupDeviceId(
    PWCHAR DeviceId
    );

//
// Hot Docking Profile Support
//

typedef enum _HARDWARE_PROFILE_BUS_TYPE {
    HardwareProfileBusTypeACPI
} HARDWARE_PROFILE_BUS_TYPE, * PHARDWARE_PROFILE_BUS_TYPE;

extern LIST_ENTRY   IopDockDeviceListHead;
extern FAST_MUTEX   IopDockDeviceListLock;
extern ULONG        IopDockDeviceCount;
extern KSEMAPHORE   IopProfileChangeSemaphore;
extern LONG         IopDocksInTransition;

//
// These six functions are exported by Dockhwp to control hardware profile
// changes
//
VOID
IopHardwareProfileBeginTransition(
    IN BOOLEAN SubsumeExistingDeparture
    );

VOID
IopHardwareProfileMarkDock(
    PDEVICE_NODE    DeviceNode,
    PROFILE_STATUS  ChangeInPresence
    );

NTSTATUS
IopHardwareProfileQueryChange(
    IN  BOOLEAN                     SubsumeExistingDeparture,
    IN  PROFILE_NOTIFICATION_TIME   NotificationTime,
    OUT PPNP_VETO_TYPE              VetoType,
    OUT PUNICODE_STRING             VetoName OPTIONAL
    );

VOID
IopHardwareProfileCommitStartedDock(
    IN PDEVICE_NODE DeviceNode
    );

VOID
IopHardwareProfileCommitRemovedDock(
    IN PDEVICE_NODE DeviceNode
    );

VOID
IopHardwareProfileCancelRemovedDock(
    IN PDEVICE_NODE DeviceNode
    );

VOID
IopHardwareProfileCancelTransition(
    VOID
    );

VOID
IopHardwareProfileSetMarkedDocksEjected(
    VOID
    );

VOID
IopOrphanNotification (
    PDEVICE_NODE DeviceNode
    );


//
// Warm eject externs and function prototypes
//
extern KEVENT IopWarmEjectLock;
extern PDEVICE_OBJECT IopWarmEjectPdo;

NTSTATUS
IopWarmEjectDevice(
    IN PDEVICE_OBJECT      DeviceToEject,
    IN SYSTEM_POWER_STATE  LightestSleepState
    );


