/*++ NDK Version: 0098

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    potypes.h

Abstract:

    Type definitions for the Power Subystem

Author:

    Alex Ionescu (alexi@tinykrnl.org) - Updated - 27-Feb-2006

--*/

#ifndef _POTYPES_H
#define _POTYPES_H

//
// Dependencies
//
#include <umtypes.h>
#ifndef NTOS_MODE_USER
#include <ntpoapi.h>
#endif

//
// Docking states
//
typedef enum _SYSTEM_DOCK_STATE
{
    SystemDockStateUnknown,
    SystemUndocked,
    SystemDocked
} SYSTEM_DOCK_STATE, *PSYSTEM_DOCK_STATE;

#ifndef NTOS_MODE_USER

//
// Processor Power State Data
//
typedef struct _PROCESSOR_POWER_STATE
{
    PVOID IdleFunction;
    ULONG Idle0KernelTimeLimit;
    ULONG Idle0LastTime;
    PVOID IdleHandlers;
    PVOID IdleState;
    ULONG IdleHandlersCount;
    ULONGLONG LastCheck;
    PROCESSOR_IDLE_TIMES IdleTimes;
    ULONG IdleTime1;
    ULONG PromotionCheck;
    ULONG IdleTime2;
    UCHAR CurrentThrottle;
    UCHAR ThermalThrottleLimit;
    UCHAR CurrentThrottleIndex;
    UCHAR ThermalThrottleIndex;
    ULONG LastKernelUserTime;
    ULONG PerfIdleTime;
    ULONG DebugDelta;
    ULONG DebugCount;
    ULONG LastSysTime;
    ULONG TotalIdleStateTime[3];
    ULONG TotalIdleTransitions[3];
    ULONGLONG PreviousC3StateTime;
    UCHAR KneeThrottleIndex;
    UCHAR ThrottleLimitIndex;
    UCHAR PerfStatesCount;
    UCHAR ProcessorMinThrottle;
    UCHAR ProcessorMaxThrottle;
    UCHAR LastBusyPercentage;
    UCHAR LastC3Percentage;
    UCHAR LastAdjustedBusyPercentage;
    ULONG PromotionCount;
    ULONG DemotionCount;
    ULONG ErrorCount;
    ULONG RetryCount;
    ULONG Flags;
    LARGE_INTEGER PerfCounterFrequency;
    ULONG PerfTickCount;
    KTIMER PerfTimer;
    KDPC PerfDpc;
    PROCESSOR_PERF_STATE *PerfStates;
    PVOID PerfSetThrottle;
    ULONG LastC3KernelUserTime;
    ULONG Spare1[1];
} PROCESSOR_POWER_STATE, *PPROCESSOR_POWER_STATE;

//
// Device Notification Structure
//
typedef struct _PO_DEVICE_NOTIFY
{
    LIST_ENTRY Link;
    PDEVICE_OBJECT TargetDevice;
    UCHAR WakeNeeded;
    UCHAR OrderLevel;
    PDEVICE_OBJECT DeviceObject;
    PVOID Node;
    PUSHORT DeviceName;
    PUSHORT DriverName;
    ULONG ChildCount;
    ULONG ActiveChild;
} PO_DEVICE_NOTIFY, *PPO_DEVICE_NOTIFY;

//
// Power IRP Queue
//
typedef struct _PO_IRP_QUEUE
{
    PIRP CurrentIrp;
    PIRP PendingIrpList;
} PO_IRP_QUEUE, *PPO_IRP_QUEUE;

// Power IRP Manager
typedef struct _PO_IRP_MANAGER
{
    PO_IRP_QUEUE DeviceIrpQueue;
    PO_IRP_QUEUE SystemIrpQueue;
} PO_IRP_MANAGER, *PPO_IRP_MANAGER;

#endif // !NTOS_MODE_USER

#endif // _POTYPES_H
