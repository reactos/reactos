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

//
// Number of dispatch codes supported by KINTERRUPT
//
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
#define KINTERRUPT_DISPATCH_CODES       135
#else
#define KINTERRUPT_DISPATCH_CODES       106
#endif

#ifdef NTOS_MODE_USER

//
// KPROCESSOR_MODE Type
//
typedef CCHAR KPROCESSOR_MODE;

//
// Dereferencable pointer to KUSER_SHARED_DATA in User-Mode
//
#define SharedUserData                  ((KUSER_SHARED_DATA *CONST)USER_SHARED_DATA)

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
    IN PVOID NormalContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
);

//
// Timer Routine
//
typedef VOID
(NTAPI *PTIMER_APC_ROUTINE)(
    IN PVOID TimerContext,
    IN ULONG TimerLowValue,
    IN LONG TimerHighValue
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
    volatile ULONG DpcQueueDepth;
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
    PVOID Segment;
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
    ULONG DispatchCode[KINTERRUPT_DISPATCH_CODES];
} KINTERRUPT, *PKINTERRUPT;

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

//
// Kernel Thread (KTHREAD)
//
typedef struct _KTHREAD
{
    DISPATCHER_HEADER DispatcherHeader;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    ULONGLONG CycleTime;
    ULONG HighCycleTime;
    ULONGLONG QuantumTarget;
#else
    LIST_ENTRY MutantListHead;
#endif
    PVOID InitialStack;
    ULONG_PTR StackLimit;
    PVOID KernelStack;
    KSPIN_LOCK ThreadLock;
    union
    {
        KAPC_STATE ApcState;
        struct
        {
            UCHAR ApcStateFill[23];
            UCHAR ApcQueueable;
            volatile UCHAR NextProcessor;
            volatile UCHAR DeferredProcessor;
            UCHAR AdjustReason;
            SCHAR AdjustIncrement;
        };
    };
    KSPIN_LOCK ApcQueueLock;
    ULONG ContextSwitches;
    volatile UCHAR State;
    UCHAR NpxState;
    KIRQL WaitIrql;
    KPROCESSOR_MODE WaitMode;
    LONG_PTR WaitStatus;
    union
    {
        PKWAIT_BLOCK WaitBlockList;
        PKGATE GateObject;
    };
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
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
#else
    BOOLEAN Alertable;
    BOOLEAN WaitNext;
#endif
    UCHAR WaitReason;
    SCHAR Priority;
    BOOLEAN EnableStackSwap;
    volatile UCHAR SwapBusy;
    BOOLEAN Alerted[MaximumMode];
    union
    {
        LIST_ENTRY WaitListEntry;
        SINGLE_LIST_ENTRY SwapListEntry;
    };
    PKQUEUE Queue;
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
    struct _TEB *Teb;
    union
    {
        KTIMER Timer;
        struct
        {
            UCHAR TimerFill[40];
            union
            {
                struct
                {
                    LONG AutoAlignment:1;
                    LONG DisableBoost:1;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
                    LONG EtwStackTrace1ApcInserted:1;
                    LONG EtwStackTrace2ApcInserted:1;
                    LONG CycleChargePending:1;
                    LONG ReservedFlags:27;
#else
                    LONG ReservedFlags:30;
#endif
                };
                LONG ThreadFlags;
            };
        };
    };
    union
    {
        KWAIT_BLOCK WaitBlock[THREAD_WAIT_OBJECTS + 1];
        struct
        {
            UCHAR WaitBlockFill0[23];
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
            UCHAR IdealProcessor;
#else
            BOOLEAN SystemAffinityActive;
#endif
        };
        struct
        {
            UCHAR WaitBlockFill1[47];
            CCHAR PreviousMode;
        };
        struct
        {
            UCHAR WaitBlockFill2[71];
            UCHAR ResourceIndex;
        };
        struct
        {
            UCHAR WaitBlockFill3[95];
            UCHAR LargeStack;
        };
    };
    LIST_ENTRY QueueListEntry;
    PKTRAP_FRAME TrapFrame;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    PVOID FirstArgument;
#endif
    PVOID CallbackStack;
    PVOID ServiceTable;
    UCHAR ApcStateIndex;
#if (NTDDI_VERSION < NTDDI_LONGHORN)
    UCHAR IdealProcessor;
#endif
    BOOLEAN Preempted;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    BOOLEAN CalloutActive;
#else
    BOOLEAN ProcessReadyQueue;
    BOOLEAN KernelStackResident;
#endif
    SCHAR BasePriority;
    SCHAR PriorityDecrement;
    CHAR Saturation;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    ULONG SystemCallNumber;
    ULONG Spare2;
#endif
    KAFFINITY UserAffinity;
    struct _KPROCESS *Process;
    KAFFINITY Affinity;
    PKAPC_STATE ApcStatePointer[2];
    union
    {
        KAPC_STATE SavedApcState;
        struct
        {
            UCHAR SavedApcStateFill[23];
            CCHAR FreezeCount;
            CCHAR SuspendCount;
            UCHAR UserIdealProcessor;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
            union
            {
                struct
                {
                    UCHAR ReservedBits0:1;
                    UCHAR SegmentsPresent:1;
                    UCHAR Reservedbits1:1;
                };
                UCHAR NestedStateFlags;
            };
#else
            UCHAR CalloutActive;
#endif
            UCHAR Iopl;
        };
    };
    PVOID Win32Thread;
    PVOID StackBase;
    union
    {
        KAPC SuspendApc;
        struct
        {
            UCHAR SuspendApcFill0[1];
            SCHAR Quantum;
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
            UCHAR SuspendApcFill3[36];
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
            PKPRCB WaitPrcb;
#else
            PVOID TlsArray;
#endif
        };
        struct
        {
            UCHAR SuspendApcFill4[40];
            PVOID LegoData;
        };
        struct
        {
            UCHAR SuspendApcFill5[47];
            UCHAR PowerState;
            ULONG UserTime;
        };
    };
    union
    {
        KSEMAPHORE SuspendSemaphore;
        struct
        {
            UCHAR SuspendSemaphorefill[20];
            ULONG SListFaultCount;
        };
    };
    LIST_ENTRY ThreadListEntry;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    LIST_ENTRY MutantListHead;
#endif
    PVOID SListFaultAddress;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    PVOID MdlForLockedteb;
#endif
} KTHREAD, *PKTHREAD;

#define ASSERT_THREAD(object) \
    ASSERT((((object)->DispatcherHeader.Type & KOBJECT_TYPE_MASK) == ThreadObject))

//
// Kernel Process (KPROCESS)
//
typedef struct _KPROCESS
{
    DISPATCHER_HEADER Header;
    LIST_ENTRY ProfileListHead;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    ULONG DirectoryTableBase;
    ULONG Unused0;
#else
    ULONG DirectoryTableBase[2];
#endif
#if defined(_M_IX86)
    KGDTENTRY LdtDescriptor;
    KIDTENTRY Int21Descriptor;
    USHORT IopmOffset;
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
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    ULONGLONG CycleTime;
#endif
} KPROCESS, *PKPROCESS;

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

//
// Exported System Service Descriptor Tables
//
extern KSERVICE_TABLE_DESCRIPTOR NTSYSAPI KeServiceDescriptorTable[SSDT_MAX_ENTRIES];
extern KSERVICE_TABLE_DESCRIPTOR NTSYSAPI KeServiceDescriptorTableShadow[SSDT_MAX_ENTRIES];

#endif // !NTOS_MODE_USER

#endif // _KETYPES_H
