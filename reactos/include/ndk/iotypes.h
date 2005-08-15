/*
 * PROJECT:         ReactOS Native Headers
 * FILE:            include/ndk/iotypes.h
 * PURPOSE:         Definitions for exported I/O Manager Types not defined in DDK/IFS
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 06/10/04
 */

#ifndef _IOTYPES_H
#define _IOTYPES_H

/* DEPENDENCIES **************************************************************/
#include "potypes.h"

/* EXPORTED DATA *************************************************************/
extern POBJECT_TYPE NTOSAPI IoAdapterObjectType;
extern POBJECT_TYPE NTOSAPI IoDeviceHandlerObjectType;
extern POBJECT_TYPE NTOSAPI IoDeviceObjectType;
extern POBJECT_TYPE NTOSAPI IoDriverObjectType;
extern POBJECT_TYPE NTOSAPI IoFileObjectType;

/* CONSTANTS *****************************************************************/

/* Device Object Extension Flags */
#define DOE_UNLOAD_PENDING    0x1
#define DOE_DELETE_PENDING    0x2
#define DOE_REMOVE_PENDING    0x4
#define DOE_REMOVE_PROCESSED  0x8
#define DOE_START_PENDING     0x10

/* Device Node Flags */
#define DNF_PROCESSED                           0x00000001
#define DNF_STARTED                             0x00000002
#define DNF_START_FAILED                        0x00000004
#define DNF_ENUMERATED                          0x00000008
#define DNF_DELETED                             0x00000010
#define DNF_MADEUP                              0x00000020
#define DNF_START_REQUEST_PENDING               0x00000040
#define DNF_NO_RESOURCE_REQUIRED                0x00000080
#define DNF_INSUFFICIENT_RESOURCES              0x00000100
#define DNF_RESOURCE_ASSIGNED                   0x00000200
#define DNF_RESOURCE_REPORTED                   0x00000400
#define DNF_HAL_NODE                            0x00000800 // ???
#define DNF_ADDED                               0x00001000
#define DNF_ADD_FAILED                          0x00002000
#define DNF_LEGACY_DRIVER                       0x00004000
#define DNF_STOPPED                             0x00008000
#define DNF_WILL_BE_REMOVED                     0x00010000
#define DNF_NEED_TO_ENUM                        0x00020000
#define DNF_NOT_CONFIGURED                      0x00040000
#define DNF_REINSTALL                           0x00080000
#define DNF_RESOURCE_REQUIREMENTS_NEED_FILTERED 0x00100000 // ???
#define DNF_DISABLED                            0x00200000
#define DNF_RESTART_OK                          0x00400000
#define DNF_NEED_RESTART                        0x00800000
#define DNF_VISITED                             0x01000000
#define DNF_ASSIGNING_RESOURCES                 0x02000000
#define DNF_BEEING_ENUMERATED                   0x04000000
#define DNF_NEED_ENUMERATION_ONLY               0x08000000
#define DNF_LOCKED                              0x10000000
#define DNF_HAS_BOOT_CONFIG                     0x20000000
#define DNF_BOOT_CONFIG_RESERVED                0x40000000
#define DNF_HAS_PROBLEM                         0x80000000 // ???
/* For UserFlags field */
#define DNUF_DONT_SHOW_IN_UI    0x0002
#define DNUF_NOT_DISABLEABLE    0x0008

/* ENUMERATIONS **************************************************************/
typedef enum _PNP_DEVNODE_STATE
{
    DeviceNodeUnspecified = 0x300,
    DeviceNodeUninitialized = 0x301,
    DeviceNodeInitialized = 0x302,
    DeviceNodeDriversAdded = 0x303,
    DeviceNodeResourcesAssigned = 0x304,
    DeviceNodeStartPending = 0x305,
    DeviceNodeStartCompletion = 0x306,
    DeviceNodeStartPostWork = 0x307,
    DeviceNodeStarted = 0x308,
    DeviceNodeQueryStopped = 0x309,
    DeviceNodeStopped = 0x30a,
    DeviceNodeRestartCompletion = 0x30b,
    DeviceNodeEnumeratePending = 0x30c,
    DeviceNodeEnumerateCompletion = 0x30d,
    DeviceNodeAwaitingQueuedDeletion = 0x30e,
    DeviceNodeAwaitingQueuedRemoval = 0x30f,
    DeviceNodeQueryRemoved = 0x310,
    DeviceNodeRemovePendingCloses = 0x311,
    DeviceNodeRemoved = 0x312,
    DeviceNodeDeletePendingCloses = 0x313,
    DeviceNodeDeleted = 0x314,
    MaxDeviceNodeState = 0x315,
} PNP_DEVNODE_STATE;

/* TYPES *********************************************************************/

typedef struct _MAILSLOT_CREATE_PARAMETERS
{
    ULONG           MailslotQuota;
    ULONG           MaximumMessageSize;
    LARGE_INTEGER   ReadTimeout;
    BOOLEAN         TimeoutSpecified;
} MAILSLOT_CREATE_PARAMETERS, *PMAILSLOT_CREATE_PARAMETERS;

typedef struct _NAMED_PIPE_CREATE_PARAMETERS
{
    ULONG           NamedPipeType;
    ULONG           ReadMode;
    ULONG           CompletionMode;
    ULONG           MaximumInstances;
    ULONG           InboundQuota;
    ULONG           OutboundQuota;
    LARGE_INTEGER   DefaultTimeout;
    BOOLEAN         TimeoutSpecified;
} NAMED_PIPE_CREATE_PARAMETERS, *PNAMED_PIPE_CREATE_PARAMETERS;

typedef struct _IO_TIMER
{
    USHORT Type;
    USHORT TimerEnabled;
    LIST_ENTRY IoTimerList;
    PIO_TIMER_ROUTINE TimerRoutine;
    PVOID Context;
    PDEVICE_OBJECT DeviceObject;
} IO_TIMER, *PIO_TIMER;

typedef struct _DEVICE_NODE
{
    /* A tree structure. */
    struct _DEVICE_NODE *Parent;
    struct _DEVICE_NODE *PrevSibling;
    struct _DEVICE_NODE *NextSibling;
    struct _DEVICE_NODE *Child;
    /* The level of deepness in the tree. */
    UINT Level;
    PPO_DEVICE_NOTIFY Notify;
    /* State machine. */
    PNP_DEVNODE_STATE State;
    PNP_DEVNODE_STATE PreviousState;
    PNP_DEVNODE_STATE StateHistory[20];
    UINT StateHistoryEntry;
    /* ? */
    INT CompletionStatus;
    /* ? */
    PIRP PendingIrp;
    /* See DNF_* flags below (WinDBG documentation has WRONG values) */
    ULONG Flags;
    /* See DNUF_* flags below (and IRP_MN_QUERY_PNP_DEVICE_STATE) */
    ULONG UserFlags;
    /* See CM_PROB_* values are defined in cfg.h */
    ULONG Problem;
    /* Pointer to the PDO corresponding to the device node. */
    PDEVICE_OBJECT PhysicalDeviceObject;
    /* Resource list as assigned by the PnP arbiter. See IRP_MN_START_DEVICE
       and ARBITER_INTERFACE (not documented in DDK, but present in headers). */
    PCM_RESOURCE_LIST ResourceList;
    /* Resource list as assigned by the PnP arbiter (translated version). */
    PCM_RESOURCE_LIST ResourceListTranslated;
    /* Instance path relative to the Enum key in registry. */
    UNICODE_STRING InstancePath;
    /* Name of the driver service. */
    UNICODE_STRING ServiceName;
    /* ? */
    PDEVICE_OBJECT DuplicatePDO;
    /* See IRP_MN_QUERY_RESOURCE_REQUIREMENTS. */
    PIO_RESOURCE_REQUIREMENTS_LIST ResourceRequirements;
    /* Information about bus for bus drivers. */
    INTERFACE_TYPE InterfaceType;
    ULONG BusNumber;
    /* Information about underlying bus for child devices. */
    INTERFACE_TYPE ChildInterfaceType;
    ULONG ChildBusNumber;
    USHORT ChildBusTypeIndex;
    /* ? */
    UCHAR RemovalPolicy;
    UCHAR HardwareRemovalPolicy;
    LIST_ENTRY TargetDeviceNotify;
    LIST_ENTRY DeviceArbiterList;
    LIST_ENTRY DeviceTranslatorList;
    USHORT NoTranslatorMask;
    USHORT QueryTranslatorMask;
    USHORT NoArbiterMask;
    USHORT QueryArbiterMask;
    union
    {
        struct _DEVICE_NODE *LegacyDeviceNode;
        PDEVICE_RELATIONS PendingDeviceRelations;
    } OverUsed1;
    union
    {
        struct _DEVICE_NODE *NextResourceDeviceNode;
    } OverUsed2;
    /* See IRP_MN_QUERY_RESOURCES/IRP_MN_FILTER_RESOURCES. */
    PCM_RESOURCE_LIST BootResources;
    /* See the bitfields in DEVICE_CAPABILITIES structure. */
    ULONG CapabilityFlags;
    struct
    {
        ULONG DockStatus;
        LIST_ENTRY ListEntry;
        WCHAR *SerialNumber;
    } DockInfo;
    ULONG DisableableDepends;
    LIST_ENTRY PendedSetInterfaceState;
    LIST_ENTRY LegacyBusListEntry;
    ULONG DriverUnloadRetryCount;
    struct _DEVICE_NODE *PreviousParent;
    ULONG DeletedChidren;
} DEVICE_NODE, *PDEVICE_NODE;

typedef struct _PI_RESOURCE_ARBITER_ENTRY
{
    LIST_ENTRY DeviceArbiterList;
    UCHAR ResourceType;
    PARBITER_INTERFACE ArbiterInterface;
    ULONG Level;
    LIST_ENTRY ResourceList;
    LIST_ENTRY BestResourceList;
    LIST_ENTRY BestConfig;
    LIST_ENTRY ActiveArbiterList;
    UCHAR State;
    UCHAR ResourcesChanged;
} PI_RESOURCE_ARBITER_ENTRY, *PPI_RESOURCE_ARBITER_ENTRY;
  
typedef struct _DEVOBJ_EXTENSION
{
    CSHORT Type;
    USHORT Size;
    PDEVICE_OBJECT DeviceObject;
    ULONG PowerFlags;
    struct DEVICE_OBJECT_POWER_EXTENSION *Dope;
    ULONG ExtensionFlags;
    struct _DEVICE_NODE *DeviceNode;
    PDEVICE_OBJECT AttachedTo;
    LONG StartIoCount;
    LONG StartIoKey;
    ULONG StartIoFlags;
    struct _VPB *Vpb;
} DEVOBJ_EXTENSION, *PDEVOBJ_EXTENSION;

typedef struct _PRIVATE_DRIVER_EXTENSIONS
{
    struct _PRIVATE_DRIVER_EXTENSIONS *Link;
    PVOID ClientIdentificationAddress;
    CHAR Extension[1];
} PRIVATE_DRIVER_EXTENSIONS, *PPRIVATE_DRIVER_EXTENSIONS;

#endif

