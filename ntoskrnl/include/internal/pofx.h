/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Internal header for the Power Manager Framework (PoFx)
 * COPYRIGHT:   Copyright 2023 George Bișoc <george.bisoc@reactos.org>
 */

//
// Define this if you want debugging support
//
#define _POFX_DEBUG_                                    0x00

//
// These define the Debug Masks Supported
//
#define POFX_DEVICE_DEBUG                               0x01
#define POFX_COMPONENT_DEBUG                            0x02
#define POFX_PEP_DEBUG                                  0x04
#define POFX_DEPENDENT_DEBUG                            0x06
#define POFX_CALLBACKS_DEBUG                            0x08
#define POFX_IDLE_STATE_DEBUG                           0x10

//
// Debug/Tracing support
//
#if _POFX_DEBUG_
#ifdef NEW_DEBUG_SYSTEM_IMPLEMENTED // enable when Debug Filters are implemented
#define POFXTRACE DbgPrintEx
#else
#define POFXTRACE(x, ...)                                 \
    if (x & PopFxTraceLevel) DbgPrint(__VA_ARGS__)
#endif
#else
#define POFXTRACE(x, fmt, ...) DPRINT(fmt, ##__VA_ARGS__)
#endif

//
// PoFx component flags
//
typedef union _POP_FX_COMPONENT_FLAGS
{
    struct
    {
        LONG Value;
        LONG Value2;
    };
    ULONG RefCount:30;
    ULONG Idling:1;
    struct
    {
        ULONG Active:1;
        ULONG CriticalIdleOverride:1;
        ULONG ResidentOverride:1;
        ULONG CompleteIdleStatePending:1;
    };
    ULONG Reserved:29;
} POP_FX_COMPONENT_FLAGS, *PPOP_FX_COMPONENT_FLAGS;

//
// PoFx device status
//
typedef union _POP_FX_DEVICE_STATUS
{
    LONG Value;
    ULONG SystemTransition:1;
    ULONG PepD0Notify:1;
    ULONG IdleTimerOn:1;
    ULONG IgnoreIdleTimeout:1;
    ULONG IrpInUse:1;
    ULONG IrpPending:1;
    ULONG DPNRDeviceNotified:1;
    ULONG DPNRReceivedFromPep:1;
    ULONG Reserved:24;
} POP_FX_DEVICE_STATUS, *PPOP_FX_DEVICE_STATUS;

//
// PoFx device and component accounting
//
typedef struct _POP_FX_ACCOUNTING
{
    ULONG Lock;
    BOOLEAN Active;
    ULONG DripsRequiredState;
    LONG Level;
    LONGLONG ActiveStamp;
    ULONGLONG CsActiveTime;
    LONGLONG CriticalActiveTime;
} POP_FX_ACCOUNTING, *PPOP_FX_ACCOUNTING;

//
// PoFx dependents
//
typedef struct _POP_FX_DEPENDENT
{
    ULONG Index;
    ULONG ProviderIndex;
} POP_FX_DEPENDENT, *PPOP_FX_DEPENDENT;

//
// PoFx providers
//
typedef struct _POP_FX_PROVIDER
{
    ULONG Index;
    BOOLEAN Activating;
} POP_FX_PROVIDER, *PPOP_FX_PROVIDER;

//
// PoFx power idle state
//
typedef struct _POP_FX_IDLE_STATE
{
    ULONGLONG TransitionLatency;
    ULONGLONG ResidencyRequirement;
    ULONG NominalPower;
} POP_FX_IDLE_STATE, *PPOP_FX_IDLE_STATE;

//
// PoFx driver callbacks
//
typedef struct _POP_FX_DRIVER_CALLBACKS
{
    VOID (*ComponentActive)(PVOID arg1, ULONG arg2);
    VOID (*ComponentIdle)(PVOID arg1, ULONG arg2);
    VOID (*ComponentIdleState)(PVOID arg1, ULONG arg2, ULONG arg3);
    VOID (*DevicePowerRequired)(PVOID arg1);
    VOID (*DevicePowerNotRequired)(PVOID arg1);
    LONG (*PowerControl)(PVOID arg1, PGUID arg2, PVOID arg3, ULONG arg4, PVOID arg5, ULONG arg6, PULONG arg7);
    VOID (*ComponentCriticalTransition)(PVOID arg1, ULONG arg2, UCHAR arg3);
} POP_FX_DRIVER_CALLBACKS, *PPOP_FX_DRIVER_CALLBACKS;

//
// PoFx worker watchdog info
//
typedef struct _POP_FX_WORK_ORDER_WATCHDOG_INFO
{
    KTIMER Timer;
    KDPC Dpc;
    struct _POP_FX_WORK_ORDER* WorkOrder;
} POP_FX_WORK_ORDER_WATCHDOG_INFO, *PPOP_FX_WORK_ORDER_WATCHDOG_INFO;

//
// PoFx worker order
//
typedef struct _POP_FX_WORK_ORDER
{
    WORK_QUEUE_ITEM WorkItem;
    LONG WorkCount;
    PVOID Context;
    PPOP_FX_WORK_ORDER_WATCHDOG_INFO WatchdogTimerInfo;
} POP_FX_WORK_ORDER, *PPOP_FX_WORK_ORDER;

//
// PoFx power extension plug-in (PEP)
//
typedef struct _POP_FX_PLUGIN
{
    LIST_ENTRY Link;
    ULONG Version;
    ULONGLONG Flags;
    KQUEUE WorkQueue;
    UCHAR (*AcceptDeviceNotification)(ULONG arg1, PVOID arg2);
    UCHAR (*AcceptProcessorNotification)(PPEPHANDLE__ arg1, ULONG arg2, PVOID arg3);
    ULONG WorkOrderCount;
    POP_FX_WORK_ORDER WorkOrders[1];
} POP_FX_PLUGIN, *PPOP_FX_PLUGIN;

//
// PoFx device
//
typedef struct _POP_FX_DEVICE
{
    LIST_ENTRY Link;
    PIRP Irp;
    struct _POP_IRP_DATA* IrpData;
    volatile POP_FX_DEVICE_STATUS Status;
    LONG PowerReqCall;
    LONG PowerNotReqCall;
    PPOP_FX_PLUGIN Plugin;
    PPEPHANDLE__ PluginHandle;
    PPOP_FX_PLUGIN MiniPlugin;
    PPEPHANDLE__ MiniPluginHandle;
    PDEVICE_NODE DevNode;
    PDEVICE_OBJECT DeviceObject;
    PDEVICE_OBJECT TargetDevice;
    POP_FX_DRIVER_CALLBACKS Callbacks;
    PVOID DriverContext;
    IO_REMOVE_LOCK RemoveLock;
    POP_FX_WORK_ORDER WorkOrder;
    ULONG IdleLock;
    KTIMER IdleTimer;
    KDPC IdleDpc;
    ULONGLONG IdleTimeout;
    ULONGLONG IdleStamp;
    PDEVICE_OBJECT NextIrpDeviceObject;
    POWER_STATE NextIrpPowerState;
    VOID (*NextIrpCallerCompletion)(PDEVICE_OBJECT arg1, UCHAR arg2, POWER_STATE arg3, PVOID arg4, PIO_STATUS_BLOCK arg5);
    PVOID NextIrpCallerContext;
    KEVENT IrpCompleteEvent;
    UCHAR (*PowerOnDumpDeviceCallback)(PPEP_CRASHDUMP_INFORMATION arg1);
    POP_FX_ACCOUNTING Accounting;
    ULONG ComponentCount;
    struct _POP_FX_COMPONENT* Components[1];
} POP_FX_DEVICE, *PPOP_FX_DEVICE;

//
// PoFx component
//
typedef struct _POP_FX_COMPONENT
{
    GUID Id;
    ULONG Index;
    POP_FX_WORK_ORDER WorkOrder;
    PPOP_FX_DEVICE Device;
    volatile POP_FX_COMPONENT_FLAGS Flags;
    volatile LONG Resident;
    KEVENT ActiveEvent;
    ULONG IdleLock;
    volatile LONG IdleConditionComplete;
    volatile LONG IdleStateComplete;
    ULONGLONG IdleStamp;
    volatile ULONG CurrentIdleState;
    ULONG IdleStateCount;
    PPOP_FX_IDLE_STATE IdleStates;
    ULONG DeepestWakeableIdleState;
    ULONG ProviderCount;
    PPOP_FX_PROVIDER Providers;
    ULONG IdleProviderCount;
    ULONG DependentCount;
    PPOP_FX_DEPENDENT Dependents;
    POP_FX_ACCOUNTING Accounting;
} POP_FX_COMPONENT, *PPOP_FX_COMPONENT;

/* EOF */
