/*++ NDK Version: 0098

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    lpctypes.h

Abstract:

    Type definitions for the Loader.

Author:

    Alex Ionescu (alexi@tinykrnl.org) - Updated - 27-Feb-2006

--*/

#ifndef _KETYPES_H
#define _KETYPES_H

//
// Dependencies
//
#include <umtypes.h>
#ifndef NTOS_MODE_USER
#include <haltypes.h>
#include <potypes.h>
#include <ifssupp.h>
#endif

//
// A system call ID is formatted as such:
// .________________________________________________________________.
// | 14 | 13 | 12 | 11 | 10 | 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
// |--------------|-------------------------------------------------|
// | TABLE NUMBER |                  TABLE OFFSET                   |
// \----------------------------------------------------------------/
//
// The table number is then used as an index into the service descriptor table.
#define TABLE_NUMBER_BITS 1
#define TABLE_OFFSET_BITS 12

//
// There are 2 tables (kernel and shadow, used by Win32K)
//
#define NUMBER_SERVICE_TABLES 2
#define NTOS_SERVICE_INDEX   0
#define WIN32K_SERVICE_INDEX 1

//
// NB. From assembly code, the table number must be computed as an offset into
//     the service descriptor table.
//
//     Each entry into the table is 16 bytes long on 32-bit architectures, and
//     32 bytes long on 64-bit architectures.
//
//     Thus, Table Number 1 is offset 16 (0x10) on x86, and offset 32 (0x20) on
//     x64.
//
#ifdef _WIN64
#define BITS_PER_ENTRY 5 // (1 << 5) = 32 bytes
#else
#define BITS_PER_ENTRY 4 // (1 << 4) = 16 bytes
#endif

//
// We want the table number, but leave some extra bits to we can have the offset
// into the descriptor table.
//
#define SERVICE_TABLE_SHIFT (12 - BITS_PER_ENTRY)

//
// Now the table number (as an offset) is corrupted with part of the table offset
// This mask will remove the extra unwanted bits, and give us the offset into the
// descriptor table proper.
//
#define SERVICE_TABLE_MASK  (((1 << TABLE_NUMBER_BITS) - 1) << BITS_PER_ENTRY)

//
// To get the table offset (ie: the service call number), just keep the 12 bits
//
#define SERVICE_NUMBER_MASK ((1 << TABLE_OFFSET_BITS) - 1)

//
// We'll often need to check if this is a graphics call. This is done by comparing
// the table number offset with the known Win32K table number offset.
// This is usually index 1, so table number offset 0x10 (x86) or 0x20 (x64)
//
#define SERVICE_TABLE_TEST  (WIN32K_SERVICE_INDEX << BITS_PER_ENTRY)

//
// Context Record Flags
//
#define CONTEXT_DEBUGGER                (CONTEXT_FULL | CONTEXT_FLOATING_POINT)

//
// Maximum System Descriptor Table Entries
//
#define SSDT_MAX_ENTRIES                2

//
// Processor Architectures
//
#define PROCESSOR_ARCHITECTURE_INTEL    0
#define PROCESSOR_ARCHITECTURE_MIPS     1
#define PROCESSOR_ARCHITECTURE_ALPHA    2
#define PROCESSOR_ARCHITECTURE_PPC      3
#define PROCESSOR_ARCHITECTURE_SHX      4
#define PROCESSOR_ARCHITECTURE_ARM      5
#define PROCESSOR_ARCHITECTURE_IA64     6
#define PROCESSOR_ARCHITECTURE_ALPHA64  7
#define PROCESSOR_ARCHITECTURE_MSIL     8
#define PROCESSOR_ARCHITECTURE_AMD64    9
#define PROCESSOR_ARCHITECTURE_UNKNOWN  0xFFFF

//
// Object Type Mask for Kernel Dispatcher Objects
//
#define KOBJECT_TYPE_MASK               0x7F
#define KOBJECT_LOCK_BIT                0x80

//
// Dispatcher Priority increments
//
#define THREAD_ALERT_INCREMENT          2

//
// Physical memory offset of KUSER_SHARED_DATA
//
#define KI_USER_SHARED_DATA_PHYSICAL    0x41000

//
// Quantum values and decrements
//
#define MAX_QUANTUM                     0x7F
#define WAIT_QUANTUM_DECREMENT          1
#define CLOCK_QUANTUM_DECREMENT         3

//
// Kernel Feature Bits
//
#define KF_V86_VIS                      0x00000001
#define KF_RDTSC                        0x00000002
#define KF_CR4                          0x00000004
#define KF_CMOV                         0x00000008
#define KF_GLOBAL_PAGE                  0x00000010
#define KF_LARGE_PAGE                   0x00000020
#define KF_MTRR                         0x00000040
#define KF_CMPXCHG8B                    0x00000080
#define KF_MMX                          0x00000100
#define KF_WORKING_PTE                  0x00000200
#define KF_PAT                          0x00000400
#define KF_FXSR                         0x00000800
#define KF_FAST_SYSCALL                 0x00001000
#define KF_XMMI                         0x00002000
#define KF_3DNOW                        0x00004000
#define KF_AMDK6MTRR                    0x00008000
#define KF_XMMI64                       0x00010000
#define KF_DTS                          0x00020000
#define KF_NX_BIT                       0x20000000
#define KF_NX_DISABLED                  0x40000000
#define KF_NX_ENABLED                   0x80000000

//
// Internal Exception Codes
//
#define KI_EXCEPTION_INTERNAL           0x10000000
#define KI_EXCEPTION_ACCESS_VIOLATION   (KI_EXCEPTION_INTERNAL | 0x04)

#ifndef NTOS_MODE_USER
//
// Number of dispatch codes supported by KINTERRUPT
//
#ifdef _M_AMD64
#define DISPATCH_LENGTH                 4
#elif (NTDDI_VERSION >= NTDDI_LONGHORN)
#define DISPATCH_LENGTH                 135
#else
#define DISPATCH_LENGTH                 106
#endif

#else

//
// KPROCESSOR_MODE Type
//
typedef CCHAR KPROCESSOR_MODE;

//
// Dereferencable pointer to KUSER_SHARED_DATA in User-Mode
//
#define SharedUserData                  ((KUSER_SHARED_DATA *)USER_SHARED_DATA)

//
// Maximum WOW64 Entries in KUSER_SHARED_DATA
//
#define MAX_WOW64_SHARED_ENTRIES        16

//
// Maximum Processor Features supported in KUSER_SHARED_DATA
//
#define PROCESSOR_FEATURE_MAX           64

//
// Event Types
//
typedef enum _EVENT_TYPE
{
    NotificationEvent,
    SynchronizationEvent
} EVENT_TYPE;

//
// Timer Types
//
typedef enum _TIMER_TYPE
{
    NotificationTimer,
    SynchronizationTimer
} TIMER_TYPE;

//
// Wait Types
//
typedef enum _WAIT_TYPE
{
    WaitAll,
    WaitAny
} WAIT_TYPE;

//
// Processor Execution Modes
//
typedef enum _MODE
{
    KernelMode,
    UserMode,
    MaximumMode
} MODE;

//
// Wait Reasons
//
typedef enum _KWAIT_REASON
{
    Executive,
    FreePage,
    PageIn,
    PoolAllocation,
    DelayExecution,
    Suspended,
    UserRequest,
    WrExecutive,
    WrFreePage,
    WrPageIn,
    WrPoolAllocation,
    WrDelayExecution,
    WrSuspended,
    WrUserRequest,
    WrEventPair,
    WrQueue,
    WrLpcReceive,
    WrLpcReply,
    WrVirtualMemory,
    WrPageOut,
    WrRendezvous,
    Spare2,
    WrGuardedMutex,
    Spare4,
    Spare5,
    Spare6,
    WrKernel,
    WrResource,
    WrPushLock,
    WrMutex,
    WrQuantumEnd,
    WrDispatchInt,
    WrPreempted,
    WrYieldExecution,
    MaximumWaitReason
} KWAIT_REASON;

//
// Profiling Sources
//
typedef enum _KPROFILE_SOURCE
{
    ProfileTime,
    ProfileAlignmentFixup,
    ProfileTotalIssues,
    ProfilePipelineDry,
    ProfileLoadInstructions,
    ProfilePipelineFrozen,
    ProfileBranchInstructions,
    ProfileTotalNonissues,
    ProfileDcacheMisses,
    ProfileIcacheMisses,
    ProfileCacheMisses,
    ProfileBranchMispredictions,
    ProfileStoreInstructions,
    ProfileFpInstructions,
    ProfileIntegerInstructions,
    Profile2Issue,
    Profile3Issue,
    Profile4Issue,
    ProfileSpecialInstructions,
    ProfileTotalCycles,
    ProfileIcacheIssues,
    ProfileDcacheAccesses,
    ProfileMemoryBarrierCycles,
    ProfileLoadLinkedIssues,
    ProfileMaximum
} KPROFILE_SOURCE;

//
// NT Product and Architecture Types
//
typedef enum _NT_PRODUCT_TYPE
{
    NtProductWinNt = 1,
    NtProductLanManNt,
    NtProductServer
} NT_PRODUCT_TYPE, *PNT_PRODUCT_TYPE;

typedef enum _ALTERNATIVE_ARCHITECTURE_TYPE
{
    StandardDesign,
    NEC98x86,
    EndAlternatives
} ALTERNATIVE_ARCHITECTURE_TYPE;

#endif

//
// Thread States
//
typedef enum _KTHREAD_STATE
{
    Initialized,
    Ready,
    Running,
    Standby,
    Terminated,
    Waiting,
    Transition,
    DeferredReady,
#if (NTDDI_VERSION >= NTDDI_WS03)
    GateWait
#endif
} KTHREAD_STATE, *PKTHREAD_STATE;

//
// Kernel Object Types
//
typedef enum _KOBJECTS
{
    EventNotificationObject = 0,
    EventSynchronizationObject = 1,
    MutantObject = 2,
    ProcessObject = 3,
    QueueObject = 4,
    SemaphoreObject = 5,
    ThreadObject = 6,
    GateObject = 7,
    TimerNotificationObject = 8,
    TimerSynchronizationObject = 9,
    Spare2Object = 10,
    Spare3Object = 11,
    Spare4Object = 12,
    Spare5Object = 13,
    Spare6Object = 14,
    Spare7Object = 15,
    Spare8Object = 16,
    Spare9Object = 17,
    ApcObject = 18,
    DpcObject = 19,
    DeviceQueueObject = 20,
    EventPairObject = 21,
    InterruptObject = 22,
    ProfileObject = 23,
    ThreadedDpcObject = 24,
    MaximumKernelObject = 25
} KOBJECTS;

//
// Adjust reasons
//
typedef enum _ADJUST_REASON
{
    AdjustNone = 0,
    AdjustUnwait = 1,
    AdjustBoost = 2
} ADJUST_REASON;

//
// Continue Status
//
typedef enum _KCONTINUE_STATUS
{
    ContinueError = 0,
    ContinueSuccess,
    ContinueProcessorReselected,
    ContinueNextProcessor
} KCONTINUE_STATUS;

//
// Process States
//
typedef enum _KPROCESS_STATE
{
    ProcessInMemory,
    ProcessOutOfMemory,
    ProcessInTransition,
    ProcessInSwap,
    ProcessOutSwap,
} KPROCESS_STATE, *PKPROCESS_STATE;

//
// NtVdmControl Classes
//
typedef enum _VDMSERVICECLASS
{
   VdmStartExecution = 0,
   VdmQueueInterrupt = 1,
   VdmDelayInterrupt = 2,
   VdmInitialize = 3,
   VdmFeatures = 4,
   VdmSetInt21Handler = 5,
   VdmQueryDir = 6,
   VdmPrinterDirectIoOpen = 7,
   VdmPrinterDirectIoClose = 8,
   VdmPrinterInitialize = 9,
   VdmSetLdtEntries = 10,
   VdmSetProcessLdtInfo = 11,
   VdmAdlibEmulation = 12,
   VdmPMCliControl = 13,
   VdmQueryVdmProcess = 14,
} VDMSERVICECLASS;

#ifdef NTOS_MODE_USER

//
// APC Normal Routine
//
typedef VOID
(NTAPI *PKNORMAL_ROUTINE)(
    _In_ PVOID NormalContext,
    _In_ PVOID SystemArgument1,
    _In_ PVOID SystemArgument2
);

//
// Timer Routine
//
typedef VOID
(NTAPI *PTIMER_APC_ROUTINE)(
    _In_ PVOID TimerContext,
    _In_ ULONG TimerLowValue,
    _In_ LONG TimerHighValue
);

//
// System Time Structure
//
typedef struct _KSYSTEM_TIME
{
    ULONG LowPart;
    LONG High1Time;
    LONG High2Time;
} KSYSTEM_TIME, *PKSYSTEM_TIME;

//
// Shared Kernel User Data
//
typedef struct _KUSER_SHARED_DATA
{
    ULONG TickCountLowDeprecated;
    ULONG TickCountMultiplier;
    volatile KSYSTEM_TIME InterruptTime;
    volatile KSYSTEM_TIME SystemTime;
    volatile KSYSTEM_TIME TimeZoneBias;
    USHORT ImageNumberLow;
    USHORT ImageNumberHigh;
    WCHAR NtSystemRoot[260];
    ULONG MaxStackTraceDepth;
    ULONG CryptoExponent;
    ULONG TimeZoneId;
    ULONG LargePageMinimum;
    ULONG Reserved2[7];
    NT_PRODUCT_TYPE NtProductType;
    BOOLEAN ProductTypeIsValid;
    ULONG NtMajorVersion;
    ULONG NtMinorVersion;
    BOOLEAN ProcessorFeatures[PROCESSOR_FEATURE_MAX];
    ULONG Reserved1;
    ULONG Reserved3;
    volatile ULONG TimeSlip;
    ALTERNATIVE_ARCHITECTURE_TYPE AlternativeArchitecture;
    LARGE_INTEGER SystemExpirationDate;
    ULONG SuiteMask;
    BOOLEAN KdDebuggerEnabled;
#if (NTDDI_VERSION >= NTDDI_WINXPSP2)
    UCHAR NXSupportPolicy;
#endif
    volatile ULONG ActiveConsoleId;
    volatile ULONG DismountCount;
    ULONG ComPlusPackage;
    ULONG LastSystemRITEventTickCount;
    ULONG NumberOfPhysicalPages;
    BOOLEAN SafeBootMode;
    ULONG TraceLogging;
    ULONG Fill0;
    ULONGLONG TestRetInstruction;
    ULONG SystemCall;
    ULONG SystemCallReturn;
    ULONGLONG SystemCallPad[3];
    union {
        volatile KSYSTEM_TIME TickCount;
        volatile ULONG64 TickCountQuad;
    };
    ULONG Cookie;
#if (NTDDI_VERSION >= NTDDI_WS03)
    LONGLONG ConsoleSessionForegroundProcessId;
    ULONG Wow64SharedInformation[MAX_WOW64_SHARED_ENTRIES];
#endif
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    USHORT UserModeGlobalLogger[8];
    ULONG HeapTracingPid[2];
    ULONG CritSecTracingPid[2];
    union
    {
        ULONG SharedDataFlags;
        struct
        {
            ULONG DbgErrorPortPresent:1;
            ULONG DbgElevationEnabled:1;
            ULONG DbgVirtEnabled:1;
            ULONG DbgInstallerDetectEnabled:1;
            ULONG SpareBits:28;
        };
    };
    ULONG ImageFileExecutionOptions;
    KAFFINITY ActiveProcessorAffinity;
#endif
} KUSER_SHARED_DATA, *PKUSER_SHARED_DATA;

//
// VDM Structures
//
#include "pshpack1.h"
typedef struct _VdmVirtualIca
{
    LONG ica_count[8];
    LONG ica_int_line;
    LONG ica_cpu_int;
    USHORT ica_base;
    USHORT ica_hipiri;
    USHORT ica_mode;
    UCHAR ica_master;
    UCHAR ica_irr;
    UCHAR ica_isr;
    UCHAR ica_imr;
    UCHAR ica_ssr;
} VDMVIRTUALICA, *PVDMVIRTUALICA;
#include "poppack.h"

typedef struct _VdmIcaUserData
{
    PVOID pIcaLock;
    PVDMVIRTUALICA pIcaMaster;
    PVDMVIRTUALICA pIcaSlave;
    PULONG pDelayIrq;
    PULONG pUndelayIrq;
    PULONG pDelayIret;
    PULONG pIretHooked;
    PULONG pAddrIretBopTable;
    PHANDLE phWowIdleEvent;
    PLARGE_INTEGER pIcaTimeout;
    PHANDLE phMainThreadSuspended;
} VDMICAUSERDATA, *PVDMICAUSERDATA;

typedef struct _VDM_INITIALIZE_DATA
{
    PVOID TrapcHandler;
    PVDMICAUSERDATA IcaUserData;
} VDM_INITIALIZE_DATA, *PVDM_INITIALIZE_DATA;

#else

//
// System Thread Start Routine
//
typedef
VOID
(NTAPI *PKSYSTEM_ROUTINE)(
    PKSTART_ROUTINE StartRoutine,
    PVOID StartContext
);

#ifndef _NTSYSTEM_
typedef VOID
(NTAPI *PKNORMAL_ROUTINE)(
  IN PVOID NormalContext OPTIONAL,
  IN PVOID SystemArgument1 OPTIONAL,
  IN PVOID SystemArgument2 OPTIONAL);

typedef VOID
(NTAPI *PKRUNDOWN_ROUTINE)(
  IN struct _KAPC *Apc);

typedef VOID
(NTAPI *PKKERNEL_ROUTINE)(
  IN struct _KAPC *Apc,
  IN OUT PKNORMAL_ROUTINE *NormalRoutine OPTIONAL,
  IN OUT PVOID *NormalContext OPTIONAL,
  IN OUT PVOID *SystemArgument1 OPTIONAL,
  IN OUT PVOID *SystemArgument2 OPTIONAL);
#endif

//
// APC Environment Types
//
typedef enum _KAPC_ENVIRONMENT
{
    OriginalApcEnvironment,
    AttachedApcEnvironment,
    CurrentApcEnvironment,
    InsertApcEnvironment
} KAPC_ENVIRONMENT;

//
// PRCB DPC Data
//
typedef struct _KDPC_DATA
{
    LIST_ENTRY DpcListHead;
    ULONG_PTR DpcLock;
#ifdef _M_AMD64
    volatile LONG DpcQueueDepth;
#else
    volatile ULONG DpcQueueDepth;
#endif
    ULONG DpcCount;
} KDPC_DATA, *PKDPC_DATA;

//
// Per-Processor Lookaside List
//
typedef struct _PP_LOOKASIDE_LIST
{
    struct _GENERAL_LOOKASIDE *P;
    struct _GENERAL_LOOKASIDE *L;
} PP_LOOKASIDE_LIST, *PPP_LOOKASIDE_LIST;

//
// Architectural Types
//
#include <arch/ketypes.h>

//
// Kernel Memory Node
//
#include <pshpack1.h>
typedef struct _KNODE
{
    SLIST_HEADER DeadStackList;
    SLIST_HEADER PfnDereferenceSListHead;
    KAFFINITY ProcessorMask;
    ULONG Color;
    UCHAR Seed;
    UCHAR NodeNumber;
    ULONG Flags;
    ULONG MmShiftedColor;
    ULONG FreeCount[2];
    struct _SINGLE_LIST_ENTRY *PfnDeferredList;
} KNODE, *PKNODE;
#include <poppack.h>

//
// Kernel Profile Object
//
typedef struct _KPROFILE
{
    CSHORT Type;
    CSHORT Size;
    LIST_ENTRY ProfileListEntry;
    struct _KPROCESS *Process;
    PVOID RangeBase;
    PVOID RangeLimit;
    ULONG BucketShift;
    PVOID Buffer;
    ULONG_PTR Segment;
    KAFFINITY Affinity;
    KPROFILE_SOURCE Source;
    BOOLEAN Started;
} KPROFILE, *PKPROFILE;

//
// Kernel Interrupt Object
//
typedef struct _KINTERRUPT
{
    CSHORT Type;
    CSHORT Size;
    LIST_ENTRY InterruptListEntry;
    PKSERVICE_ROUTINE ServiceRoutine;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    PKSERVICE_ROUTINE MessageServiceRoutine;
    ULONG MessageIndex;
#endif
    PVOID ServiceContext;
    KSPIN_LOCK SpinLock;
    ULONG TickCount;
    PKSPIN_LOCK ActualLock;
    PKINTERRUPT_ROUTINE DispatchAddress;
    ULONG Vector;
    KIRQL Irql;
    KIRQL SynchronizeIrql;
    BOOLEAN FloatingSave;
    BOOLEAN Connected;
    CCHAR Number;
    BOOLEAN ShareVector;
    KINTERRUPT_MODE Mode;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    KINTERRUPT_POLARITY Polarity;
#endif
    ULONG ServiceCount;
    ULONG DispatchCount;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    ULONGLONG Rsvd1;
#endif
#ifdef _M_AMD64
    PKTRAP_FRAME TrapFrame;
    PVOID Reserved;
#endif
    ULONG DispatchCode[DISPATCH_LENGTH];
} KINTERRUPT;

//
// Kernel Event Pair Object
//
typedef struct _KEVENT_PAIR
{
    CSHORT Type;
    CSHORT Size;
    KEVENT LowEvent;
    KEVENT HighEvent;
} KEVENT_PAIR, *PKEVENT_PAIR;

//
// Kernel No Execute Options
//
typedef struct _KEXECUTE_OPTIONS
{
    UCHAR ExecuteDisable:1;
    UCHAR ExecuteEnable:1;
    UCHAR DisableThunkEmulation:1;
    UCHAR Permanent:1;
    UCHAR ExecuteDispatchEnable:1;
    UCHAR ImageDispatchEnable:1;
    UCHAR Spare:2;
} KEXECUTE_OPTIONS, *PKEXECUTE_OPTIONS;

#if (NTDDI_VERSION >= NTDDI_WIN7)
typedef union _KWAIT_STATUS_REGISTER
{
    UCHAR Flags;
    struct
    {
        UCHAR State:2;
        UCHAR Affinity:1;
        UCHAR Priority:1;
        UCHAR Apc:1;
        UCHAR UserApc:1;
        UCHAR Alert:1;
        UCHAR Unused:1;
    };
} KWAIT_STATUS_REGISTER, *PKWAIT_STATUS_REGISTER;

typedef struct _COUNTER_READING
{
    enum _HARDWARE_COUNTER_TYPE Type;
    ULONG Index;
    ULONG64 Start;
    ULONG64 Total;
}COUNTER_READING, *PCOUNTER_READING;

typedef struct _KTHREAD_COUNTERS
{
    ULONG64 WaitReasonBitMap;
    struct _THREAD_PERFORMANCE_DATA* UserData;
    ULONG Flags;
    ULONG ContextSwitches;
    ULONG64 CycleTimeBias;
    ULONG64 HardwareCounters;
    COUNTER_READING HwCounter[16];
}KTHREAD_COUNTERS, *PKTHREAD_COUNTERS;
#endif

//
// Kernel Thread (KTHREAD)
//
typedef struct _KTHREAD
{
    DISPATCHER_HEADER Header;
#if (NTDDI_VERSION >= NTDDI_LONGHORN) // [
    ULONGLONG CycleTime;
#ifndef _WIN64 // [
    ULONG HighCycleTime;
#endif // ]
    ULONGLONG QuantumTarget;
#else // ][
    LIST_ENTRY MutantListHead;
#endif // ]
    PVOID InitialStack;
    ULONG_PTR StackLimit; // FIXME: PVOID
    PVOID KernelStack;
    KSPIN_LOCK ThreadLock;
#if (NTDDI_VERSION >= NTDDI_WIN7) // [
    KWAIT_STATUS_REGISTER WaitRegister;
    BOOLEAN Running;
    BOOLEAN Alerted[2];
    union
    {
        struct
        {
            ULONG KernelStackResident:1;
            ULONG ReadyTransition:1;
            ULONG ProcessReadyQueue:1;
            ULONG WaitNext:1;
            ULONG SystemAffinityActive:1;
            ULONG Alertable:1;
            ULONG GdiFlushActive:1;
            ULONG UserStackWalkActive:1;
            ULONG ApcInterruptRequest:1;
            ULONG ForceDeferSchedule:1;
            ULONG QuantumEndMigrate:1;
            ULONG UmsDirectedSwitchEnable:1;
            ULONG TimerActive:1;
            ULONG Reserved:19;
        };
        LONG MiscFlags;
    };
#endif // ]
    union
    {
        KAPC_STATE ApcState;
        struct
        {
            UCHAR ApcStateFill[FIELD_OFFSET(KAPC_STATE, UserApcPending) + 1];
#if (NTDDI_VERSION >= NTDDI_LONGHORN) // [
            SCHAR Priority;
#if (NTDDI_VERSION >= NTDDI_WIN7) // [
            /* On x86, the following members "fall out" of the union */
            volatile ULONG NextProcessor;
            volatile ULONG DeferredProcessor;
#else // ][
            /* On x86, the following members "fall out" of the union */
            volatile USHORT NextProcessor;
            volatile USHORT DeferredProcessor;
#endif // ]
#else // ][
            UCHAR ApcQueueable;
            /* On x86, the following members "fall out" of the union */
            volatile UCHAR NextProcessor;
            volatile UCHAR DeferredProcessor;
            UCHAR AdjustReason;
            SCHAR AdjustIncrement;
#endif // ]
        };
    };
    KSPIN_LOCK ApcQueueLock;
#ifndef _M_AMD64 // [
    ULONG ContextSwitches;
    volatile UCHAR State;
    UCHAR NpxState;
    KIRQL WaitIrql;
    KPROCESSOR_MODE WaitMode;
#endif // ]
    LONG_PTR WaitStatus;
#if (NTDDI_VERSION >= NTDDI_WIN7) // [
    PKWAIT_BLOCK WaitBlockList;
#else // ][
    union
    {
        PKWAIT_BLOCK WaitBlockList;
        PKGATE GateObject;
    };
#if (NTDDI_VERSION >= NTDDI_LONGHORN) // [
    union
    {
        struct
        {
            ULONG KernelStackResident:1;
            ULONG ReadyTransition:1;
            ULONG ProcessReadyQueue:1;
            ULONG WaitNext:1;
            ULONG SystemAffinityActive:1;
            ULONG Alertable:1;
            ULONG GdiFlushActive:1;
            ULONG Reserved:25;
        };
        LONG MiscFlags;
    };
#else // ][
    BOOLEAN Alertable;
    BOOLEAN WaitNext;
#endif // ]
    UCHAR WaitReason;
#if (NTDDI_VERSION < NTDDI_LONGHORN)
    SCHAR Priority;
    BOOLEAN EnableStackSwap;
#endif // ]
    volatile UCHAR SwapBusy;
    BOOLEAN Alerted[MaximumMode];
#endif // ]
    union
    {
        LIST_ENTRY WaitListEntry;
        SINGLE_LIST_ENTRY SwapListEntry;
    };
    PKQUEUE Queue;
#ifndef _M_AMD64 // [
    ULONG WaitTime;
    union
    {
        struct
        {
            SHORT KernelApcDisable;
            SHORT SpecialApcDisable;
        };
        ULONG CombinedApcDisable;
    };
#endif // ]
    struct _TEB *Teb;

#if (NTDDI_VERSION >= NTDDI_WIN7) // [
    KTIMER Timer;
#else // ][
    union
    {
        KTIMER Timer;
        struct
        {
            UCHAR TimerFill[FIELD_OFFSET(KTIMER, Period) + sizeof(LONG)];
#if !defined(_WIN64) // [
        };
    };
#endif // ]
#endif // ]
            union
            {
                struct
                {
                    ULONG AutoAlignment:1;
                    ULONG DisableBoost:1;
#if (NTDDI_VERSION >= NTDDI_LONGHORN) // [
                    ULONG EtwStackTraceApc1Inserted:1;
                    ULONG EtwStackTraceApc2Inserted:1;
                    ULONG CycleChargePending:1;
                    ULONG CalloutActive:1;
                    ULONG ApcQueueable:1;
                    ULONG EnableStackSwap:1;
                    ULONG GuiThread:1;
                    ULONG ReservedFlags:23;
#else // ][
                    LONG ReservedFlags:30;
#endif // ]
                };
                LONG ThreadFlags;
            };
#if defined(_WIN64) && (NTDDI_VERSION < NTDDI_WIN7) // [
        };
    };
#endif // ]
#if (NTDDI_VERSION >= NTDDI_WIN7) // [
#if defined(_WIN64) // [
    ULONG Spare0;
#else // ][
    PVOID ServiceTable;
#endif // ]
#endif // ]
    union
    {
        DECLSPEC_ALIGN(8) KWAIT_BLOCK WaitBlock[THREAD_WAIT_OBJECTS + 1];
#if (NTDDI_VERSION < NTDDI_WIN7) // [
        struct
        {
            UCHAR WaitBlockFill0[FIELD_OFFSET(KWAIT_BLOCK, SpareByte)]; // 32bit = 23, 64bit = 43
#if (NTDDI_VERSION >= NTDDI_LONGHORN) // [
            UCHAR IdealProcessor;
#else // ][
            BOOLEAN SystemAffinityActive;
#endif // ]
        };
        struct
        {
            UCHAR WaitBlockFill1[1 * sizeof(KWAIT_BLOCK) + FIELD_OFFSET(KWAIT_BLOCK, SpareByte)]; // 47 / 91
            CCHAR PreviousMode;
        };
        struct
        {
            UCHAR WaitBlockFill2[2 * sizeof(KWAIT_BLOCK) + FIELD_OFFSET(KWAIT_BLOCK, SpareByte)]; // 71 / 139
            UCHAR ResourceIndex;
        };
        struct
        {
            UCHAR WaitBlockFill3[3 * sizeof(KWAIT_BLOCK) + FIELD_OFFSET(KWAIT_BLOCK, SpareByte)]; // 95 / 187
            UCHAR LargeStack;
        };
#endif // ]
#ifdef _M_AMD64 // [
        struct
        {
            UCHAR WaitBlockFill4[FIELD_OFFSET(KWAIT_BLOCK, SpareLong)];
            ULONG ContextSwitches;
        };
        struct
        {
            UCHAR WaitBlockFill5[1 * sizeof(KWAIT_BLOCK) + FIELD_OFFSET(KWAIT_BLOCK, SpareLong)];
            UCHAR State;
            UCHAR NpxState;
            UCHAR WaitIrql;
            CHAR WaitMode;
        };
        struct
        {
            UCHAR WaitBlockFill6[2 * sizeof(KWAIT_BLOCK) + FIELD_OFFSET(KWAIT_BLOCK, SpareLong)];
            ULONG WaitTime;
        };
#if (NTDDI_VERSION >= NTDDI_WIN7) // [
        struct
        {
            UCHAR WaitBlockFill7[168];
            PVOID TebMappedLowVa;
            struct _UMS_CONTROL_BLOCK* Ucb;
        };
#endif // ]
        struct
        {
#if (NTDDI_VERSION >= NTDDI_WIN7) // [
            UCHAR WaitBlockFill8[188];
#else // ][
            UCHAR WaitBlockFill7[3 * sizeof(KWAIT_BLOCK) + FIELD_OFFSET(KWAIT_BLOCK, SpareLong)];
#endif // ]
            union
            {
                struct
                {
                    SHORT KernelApcDisable;
                    SHORT SpecialApcDisable;
                };
                ULONG CombinedApcDisable;
            };
        };
#endif // ]
    };
    LIST_ENTRY QueueListEntry;
    PKTRAP_FRAME TrapFrame;
#if (NTDDI_VERSION >= NTDDI_LONGHORN) // [
    PVOID FirstArgument;
    union                                               // 2 elements, 0x8 bytes (sizeof)
    {
        PVOID CallbackStack;
        ULONG_PTR CallbackDepth;
    };
#else // ][
    PVOID CallbackStack;
#endif // ]
#if (NTDDI_VERSION < NTDDI_LONGHORN) || ((NTDDI_VERSION < NTDDI_WIN7) && !defined(_WIN64)) // [
    PVOID ServiceTable;
#endif // ]
#if (NTDDI_VERSION < NTDDI_LONGHORN) && defined(_WIN64) // [
    ULONG KernelLimit;
#endif // ]
    UCHAR ApcStateIndex;
#if (NTDDI_VERSION < NTDDI_LONGHORN) // [
    UCHAR IdealProcessor;
    BOOLEAN Preempted;
    BOOLEAN ProcessReadyQueue;
#ifdef _WIN64 // [
    PVOID Win32kTable;
    ULONG Win32kLimit;
#endif // ]
    BOOLEAN KernelStackResident;
#endif // ]
    SCHAR BasePriority;
    SCHAR PriorityDecrement;
#if (NTDDI_VERSION >= NTDDI_LONGHORN) // [
    BOOLEAN Preempted;
    UCHAR AdjustReason;
    CHAR AdjustIncrement;
#if (NTDDI_VERSION >= NTDDI_WIN7)
    UCHAR PreviousMode;
#else
    UCHAR Spare01;
#endif
#endif // ]
    CHAR Saturation;
#if (NTDDI_VERSION >= NTDDI_LONGHORN) // [
    ULONG SystemCallNumber;
#if (NTDDI_VERSION >= NTDDI_WIN7) // [
    ULONG FreezeCount;
#else // ][
    ULONG Spare02;
#endif // ]
#endif // ]
#if (NTDDI_VERSION >= NTDDI_WIN7) // [
    GROUP_AFFINITY UserAffinity;
    struct _KPROCESS *Process;
    GROUP_AFFINITY Affinity;
    ULONG IdealProcessor;
    ULONG UserIdealProcessor;
#else // ][
    KAFFINITY UserAffinity;
    struct _KPROCESS *Process;
    KAFFINITY Affinity;
#endif // ]
    PKAPC_STATE ApcStatePointer[2];
    union
    {
        KAPC_STATE SavedApcState;
        struct
        {
            UCHAR SavedApcStateFill[FIELD_OFFSET(KAPC_STATE, UserApcPending) + 1];
#if (NTDDI_VERSION >= NTDDI_WIN7) // [
            UCHAR WaitReason;
#else // ][
            CCHAR FreezeCount;
#endif // ]
#ifndef _WIN64 // [
        };
    };
#endif // ]
            CCHAR SuspendCount;
#if (NTDDI_VERSION >= NTDDI_WIN7) // [
            CCHAR Spare1;
#else // ][
            UCHAR UserIdealProcessor;
#endif // ]
#if (NTDDI_VERSION >= NTDDI_WIN7) // [
#elif (NTDDI_VERSION >= NTDDI_LONGHORN) // ][
            UCHAR Spare03;
#else // ][
            UCHAR CalloutActive;
#endif // ]
#ifdef _WIN64 // [
            UCHAR CodePatchInProgress;
        };
    };
#endif // ]
#if defined(_M_IX86) // [
#if (NTDDI_VERSION >= NTDDI_LONGHORN) // [
    UCHAR OtherPlatformFill;
#else // ][
    UCHAR Iopl;
#endif // ]
#endif // ]
    PVOID Win32Thread;
    PVOID StackBase;
    union
    {
        KAPC SuspendApc;
        struct
        {
            UCHAR SuspendApcFill0[1];
#if (NTDDI_VERSION >= NTDDI_WIN7) // [
            UCHAR ResourceIndex;
#elif (NTDDI_VERSION >= NTDDI_LONGHORN) // ][
            CHAR Spare04;
#else // ][
            SCHAR Quantum;
#endif // ]
        };
        struct
        {
            UCHAR SuspendApcFill1[3];
            UCHAR QuantumReset;
        };
        struct
        {
            UCHAR SuspendApcFill2[4];
            ULONG KernelTime;
        };
        struct
        {
            UCHAR SuspendApcFill3[FIELD_OFFSET(KAPC, SystemArgument1)];
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
            PKPRCB WaitPrcb;
#else
            PVOID TlsArray;
#endif
        };
        struct
        {
            UCHAR SuspendApcFill4[FIELD_OFFSET(KAPC, SystemArgument2)]; // 40 / 72
            PVOID LegoData;
        };
        struct
        {
            UCHAR SuspendApcFill5[FIELD_OFFSET(KAPC, Inserted) + 1]; // 47 / 83
#if (NTDDI_VERSION >= NTDDI_WIN7) // [
            UCHAR LargeStack;
#else // ][
            UCHAR PowerState;
#endif // ]
#ifdef _WIN64 // [
            ULONG UserTime;
#endif // ]
        };
    };
#ifndef _WIN64 // [
    ULONG UserTime;
#endif // ]
    union
    {
        KSEMAPHORE SuspendSemaphore;
        struct
        {
            UCHAR SuspendSemaphorefill[FIELD_OFFSET(KSEMAPHORE, Limit) + 4]; // 20 / 28
#ifdef _WIN64 // [
            ULONG SListFaultCount;
#endif // ]
        };
    };
#ifndef _WIN64 // [
    ULONG SListFaultCount;
#endif // ]
    LIST_ENTRY ThreadListEntry;
#if (NTDDI_VERSION >= NTDDI_LONGHORN) // [
    LIST_ENTRY MutantListHead;
#endif // ]
    PVOID SListFaultAddress;
#ifdef _M_AMD64 // [
    LONG64 ReadOperationCount;
    LONG64 WriteOperationCount;
    LONG64 OtherOperationCount;
    LONG64 ReadTransferCount;
    LONG64 WriteTransferCount;
    LONG64 OtherTransferCount;
#endif // ]
#if (NTDDI_VERSION >= NTDDI_WIN7) // [
    PKTHREAD_COUNTERS ThreadCounters;
    PXSTATE_SAVE XStateSave;
#elif (NTDDI_VERSION >= NTDDI_LONGHORN) // ][
    PVOID MdlForLockedTeb;
#endif // ]
} KTHREAD;

#define ASSERT_THREAD(object) \
    ASSERT((((object)->Header.Type & KOBJECT_TYPE_MASK) == ThreadObject))

//
// Kernel Process (KPROCESS)
//
typedef struct _KPROCESS
{
    DISPATCHER_HEADER Header;
    LIST_ENTRY ProfileListHead;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    ULONG_PTR DirectoryTableBase;
    ULONG Unused0;
#else
    ULONG_PTR DirectoryTableBase[2];
#endif
#if defined(_M_IX86)
    KGDTENTRY LdtDescriptor;
    KIDTENTRY Int21Descriptor;
#endif
    USHORT IopmOffset;
#if defined(_M_IX86)
    UCHAR Iopl;
    UCHAR Unused;
#endif
    volatile ULONG ActiveProcessors;
    ULONG KernelTime;
    ULONG UserTime;
    LIST_ENTRY ReadyListHead;
    SINGLE_LIST_ENTRY SwapListEntry;
    PVOID VdmTrapcHandler;
    LIST_ENTRY ThreadListHead;
    KSPIN_LOCK ProcessLock;
    KAFFINITY Affinity;
    union
    {
        struct
        {
            LONG AutoAlignment:1;
            LONG DisableBoost:1;
            LONG DisableQuantum:1;
            LONG ReservedFlags:29;
        };
        LONG ProcessFlags;
    };
    SCHAR BasePriority;
    SCHAR QuantumReset;
    UCHAR State;
    UCHAR ThreadSeed;
    UCHAR PowerState;
    UCHAR IdealNode;
    UCHAR Visited;
    union
    {
        KEXECUTE_OPTIONS Flags;
        UCHAR ExecuteOptions;
    };
    ULONG StackCount;
    LIST_ENTRY ProcessListEntry;
#if (NTDDI_VERSION >= NTDDI_LONGHORN) // [
    ULONGLONG CycleTime;
#endif // ]
} KPROCESS;

#define ASSERT_PROCESS(object) \
    ASSERT((((object)->Header.Type & KOBJECT_TYPE_MASK) == ProcessObject))

//
// System Service Table Descriptor
//
typedef struct _KSERVICE_TABLE_DESCRIPTOR
{
    PULONG_PTR Base;
    PULONG Count;
    ULONG Limit;
#if defined(_IA64_)
    LONG TableBaseGpOffset;
#endif
    PUCHAR Number;
} KSERVICE_TABLE_DESCRIPTOR, *PKSERVICE_TABLE_DESCRIPTOR;

//
// Exported Loader Parameter Block
//
extern struct _LOADER_PARAMETER_BLOCK NTSYSAPI *KeLoaderBlock;

//
// Exported Hardware Data
//
extern KAFFINITY NTSYSAPI KeActiveProcessors;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
extern volatile CCHAR NTSYSAPI KeNumberProcessors;
#else
#if (NTDDI_VERSION >= NTDDI_WINXP)
extern CCHAR NTSYSAPI KeNumberProcessors;
#else
//extern PCCHAR KeNumberProcessors;
extern NTSYSAPI CCHAR KeNumberProcessors; //FIXME: Note to Alex: I won't fix this atm, since I prefer to discuss this with you first.
#endif
#endif
extern ULONG NTSYSAPI KiDmaIoCoherency;
extern ULONG NTSYSAPI KeMaximumIncrement;
extern ULONG NTSYSAPI KeMinimumIncrement;
extern ULONG NTSYSAPI KeDcacheFlushCount;
extern ULONG NTSYSAPI KeIcacheFlushCount;
extern ULONG_PTR NTSYSAPI KiBugCheckData[];
extern BOOLEAN NTSYSAPI KiEnableTimerWatchdog;

//
// Exported System Service Descriptor Tables
//
extern KSERVICE_TABLE_DESCRIPTOR NTSYSAPI KeServiceDescriptorTable[SSDT_MAX_ENTRIES];
extern KSERVICE_TABLE_DESCRIPTOR NTSYSAPI KeServiceDescriptorTableShadow[SSDT_MAX_ENTRIES];

#endif // !NTOS_MODE_USER

#endif // _KETYPES_H
