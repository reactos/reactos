/*++ NDK Version: 0095

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    lpctypes.h

Abstract:

    Type definitions for the Loader.

Author:

    Alex Ionescu (alex.ionescu@reactos.com)   06-Oct-2004

--*/

#ifndef _KETYPES_H
#define _KETYPES_H

//
// Dependencies
//
#include <umtypes.h>
#ifndef NTOS_MODE_USER
#include <arc/arc.h>
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
#define SSDT_MAX_ENTRIES                4

//
// Maximum number of times a thread can be suspended
//
#define MAXIMUM_SUSPEND_COUNT           0x7F

//
// Dispatcher Priority increments
//
#define THREAD_ALERT_INCREMENT          2

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
} KTHREAD_STATE, *PKTHREAD_STATE;

//
// Process States
//
typedef enum _KPROCESS_STATE
{
    ProcessInMemory,
    ProcessOutOfMemory,
    ProcessInTransition,
} KPROCESS_STATE, *PKPROCESS_STATE;

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
    LONGLONG ConsoleSessionForegroundProcessId;
    ULONG Wow64SharedInformation[MAX_WOW64_SHARED_ENTRIES];
    ULONG UserModeGlobalLogging;
} KUSER_SHARED_DATA, *PKUSER_SHARED_DATA;

#else

//
// APC Environment Types
//
typedef enum _KAPC_ENVIRONMENT
{
    OriginalApcEnvironment,
    AttachedApcEnvironment,
    CurrentApcEnvironment
} KAPC_ENVIRONMENT;

//
// PRCB DPC Data
//
typedef struct _KDPC_DATA
{
    LIST_ENTRY DpcListHead;
    ULONG DpcLock;
    ULONG DpcQueueDepth;
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
typedef struct _KNODE
{
    SLIST_HEADER DeadStackList;
    SLIST_HEADER PfnDereferenceSListHead;
    ULONG ProcessorMask;
    ULONG Color;
    UCHAR Seed;
    UCHAR NodeNumber;
    ULONG Flags;
    ULONG MmShiftedColor;
    ULONG FreeCount[2];
    struct _SINGLE_LIST_ENTRY *PfnDeferredList;
} KNODE, *PKNODE;

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
    ULONG Segment;
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
    PVOID ServiceContext;
    KSPIN_LOCK SpinLock;
    ULONG TickCount;
    PKSPIN_LOCK ActualLock;
    PVOID DispatchAddress;
    ULONG Vector;
    KIRQL Irql;
    KIRQL SynchronizeIrql;
    BOOLEAN FloatingSave;
    BOOLEAN Connected;
    CHAR Number;
    UCHAR ShareVector;
    KINTERRUPT_MODE Mode;
    ULONG ServiceCount;
    ULONG DispatchCount;
    ULONG DispatchCode[106];
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
// Kernel Thread (KTHREAD)
//
#include <pshpack1.h>
typedef struct _KTHREAD
{
    DISPATCHER_HEADER DispatcherHeader;                 /* 00 */
    LIST_ENTRY MutantListHead;                          /* 10 */
    PVOID InitialStack;                                 /* 18 */
    ULONG_PTR StackLimit;                               /* 1C */
    PVOID KernelStack;                                  /* 20 */
    KSPIN_LOCK ThreadLock;                              /* 24 */
    union                                               /* 28 */
    {                                                   /* 28 */
        KAPC_STATE ApcState;                            /* 34 */
        struct                                          /* 28 */
        {                                               /* 28 */
            UCHAR ApcStateFill[23];                     /* 34 */
            UCHAR ApcQueueable;                         /* 3F */
        };                                              /* 3F */
    };                                                  /* 3F */
    UCHAR NextProcessor;                                /* 40 */
    UCHAR DeferredProcessor;                            /* 41 */
    UCHAR AdjustReason;                                 /* 42 */
    UCHAR AdjustIncrement;                              /* 43 */
    KSPIN_LOCK ApcQueueLock;                            /* 44 */
    ULONG ContextSwitches;                              /* 48 */
    UCHAR State;                                        /* 4C */
    UCHAR NpxState;                                     /* 4D */
    UCHAR WaitIrql;                                     /* 4E */
    UCHAR WaitMode;                                     /* 4F */
    LONG WaitStatus;                                    /* 50 */
    union                                               /* 54 */
    {                                                   /* 54 */
        PKWAIT_BLOCK WaitBlockList;                     /* 54 */
        PKGATE GateObject;                              /* 54 */
    };                                                  /* 54 */
    UCHAR Alertable;                                    /* 58 */
    UCHAR WaitNext;                                     /* 59 */
    UCHAR WaitReason;                                   /* 5A */
    UCHAR Priority;                                     /* 5B */
    UCHAR EnableStackSwap;                              /* 5C */
    UCHAR SwapBusy;                                     /* 5D */
    UCHAR Alerted[2];                                   /* 5E */
    union                                               /* 60 */
    {                                                   /* 60 */
        LIST_ENTRY WaitListEntry;                       /* 60 */
        SINGLE_LIST_ENTRY SwapListEntry;                /* 60 */
    };                                                  /* 68 */
    PKQUEUE Queue;                                      /* 68 */
    ULONG WaitTime;                                     /* 6C */
    union                                               /* 70 */
    {                                                   /* 70 */
        struct                                          /* 70 */
        {                                               /* 70 */
            USHORT KernelApcDisable;                    /* 70 */
            USHORT SpecialApcDisable;                   /* 72 */
        };                                              /* 70 */
        ULONG CombinedApcDisable;                       /* 70 */
    };                                                  /* 74 */
    struct _TEB *Teb;                                   /* 74 */
    union                                               /* 78 */
    {                                                   /* 78 */
        KTIMER Timer;                                   /* 78 */
        UCHAR TimerFill[40];                            /* 78 */
    };                                                  /* 78 */
    union                                               /* A0 */
    {                                                   /* A0 */
        struct                                          /* A0 */
        {                                               /* A0 */
            LONG AutoAlignment:1;                       /* A0 */
            LONG DisableBoost:1;                        /* A0 */
            LONG ReservedFlags:30;                      /* A0 */
        };                                              /* A0 */
        LONG ThreadFlags;                               /* A0 */
    };                                                  /* A0 */
    PVOID Padding;                                      /* A4 */
    union                                               /* A8 */
    {                                                   /* A8 */
        KWAIT_BLOCK WaitBlock[4];                       /* A8 */
        union                                           /* A8 */
        {                                               /* A8 */
            struct                                      /* A8 */
            {                                           /* A8 */
                UCHAR WaitBlockFill0[23];               /* A8 */
                UCHAR SystemAffinityActive;             /* BF */
            };                                          /* A8 */
            struct                                      /* A8 */
            {                                           /* A8 */
                UCHAR WaitBlockFill1[47];               /* A8 */
                UCHAR PreviousMode;                     /* D7 */
            };                                          /* A8 */
            struct                                      /* A8 */
            {                                           /* A8 */
                UCHAR WaitBlockFill2[71];               /* A8 */
                UCHAR ResourceIndex;                    /* EF */
            };                                          /* A8 */
            struct                                      /* A8 */
            {                                           /* A8 */
                UCHAR WaitBlockFill3[95];               /* A8 */
                UCHAR LargeStack;                       /* 107 */
            };                                          /* A8 */
        };                                              /* A8 */
    };                                                  /* A8 */
    LIST_ENTRY QueueListEntry;                          /* 108 */
    PKTRAP_FRAME TrapFrame;                             /* 110 */
    PVOID CallbackStack;                                /* 114 */
    PVOID ServiceTable;                                 /* 118 */
    UCHAR ApcStateIndex;                                /* 11C */
    UCHAR IdealProcessor;                               /* 11D */
    UCHAR Preempted;                                    /* 11E */
    UCHAR ProcessReadyQueue;                            /* 11F */
    UCHAR KernelStackResident;                          /* 120 */
    CHAR BasePriority;                                  /* 121 */
    CHAR PriorityDecrement;                             /* 122 */
    CHAR Saturation;                                    /* 123 */
    KAFFINITY UserAffinity;                             /* 124 */
    struct _KPROCESS *Process;                          /* 128 */
    KAFFINITY Affinity;                                 /* 12C */
    PKAPC_STATE ApcStatePointer[2];                     /* 130 */
    union                                               /* 138 */
    {                                                   /* 138 */
        KAPC_STATE SavedApcState;                       /* 138 */
        union                                           /* 138 */
        {                                               /* 138 */
            UCHAR SavedApcStateFill[23];                /* 138 */
            CHAR FreezeCount;                           /* 14F */
        };                                              /* 138 */
    };                                                  /* 138 */
    CHAR SuspendCount;                                  /* 150 */
    UCHAR UserIdealProcessor;                           /* 151 */
    UCHAR CalloutActive;                                /* 152 */
    UCHAR Iopl;                                         /* 153 */
    PVOID Win32Thread;                                  /* 154 */
    PVOID StackBase;                                    /* 158 */
    union                                               /* 15C */
    {                                                   /* 15C */
        KAPC SuspendApc;                                /* 15C */
        union                                           /* 15C */
        {                                               /* 15C */
            UCHAR SuspendApcFill0[1];                   /* 15C */
            CHAR Quantum;                               /* 15D */
        };                                              /* 15C */
        union                                           /* 15C */
        {                                               /* 15C */
            UCHAR SuspendApcFill1[3];                   /* 15C */
            UCHAR QuantumReset;                         /* 15F */
        };                                              /* 15C */
        union                                           /* 15C */
        {                                               /* 15C */
            UCHAR SuspendApcFill2[4];                   /* 15C */
            ULONG KernelTime;                           /* 160 */
        };                                              /* 15C */
        union                                           /* 15C */
        {                                               /* 15C */
            UCHAR SuspendApcFill3[36];                  /* 15C */
            PVOID TlsArray;                             /* 180 */
        };                                              /* 15C */
        union                                           /* 15C */
        {                                               /* 15C */
            UCHAR SuspendApcFill4[40];                  /* 15C */
            PVOID LegoData;                             /* 184 */
        };                                              /* 15C */
        union                                           /* 15C */
        {                                               /* 15C */
            UCHAR SuspendApcFill5[47];                  /* 15C */
            UCHAR PowerState;                           /* 18B */
        };                                              /* 15C */
    };                                                  /* 15C */
    ULONG UserTime;                                     /* 18C */
    union                                               /* 190 */
    {                                                   /* 190 */
        KSEMAPHORE SuspendSemaphore;                    /* 190 */
        UCHAR SuspendSemaphorefill[20];                 /* 190 */
    };                                                  /* 190 */
    ULONG SListFaultCount;                              /* 1A4 */
    LIST_ENTRY ThreadListEntry;                         /* 1A8 */
    PVOID SListFaultAddress;                            /* 1B0 */
} KTHREAD;                                              /* sizeof: 1B4 */
#include <poppack.h>

//
// Kernel Process (KPROCESS)
//
typedef struct _KPROCESS
{
    DISPATCHER_HEADER     Header;                    /* 000 */
    LIST_ENTRY            ProfileListHead;           /* 010 */
    PHYSICAL_ADDRESS      DirectoryTableBase;        /* 018 */
#if defined(_M_IX86)
    KGDTENTRY             LdtDescriptor;             /* 020 */
    KIDTENTRY             Int21Descriptor;           /* 028 */
    USHORT                IopmOffset;                /* 030 */
    UCHAR                 Iopl;                      /* 032 */
    UCHAR                 Unused;                    /* 033 */
#endif
    ULONG                 ActiveProcessors;          /* 034 */
    ULONG                 KernelTime;                /* 038 */
    ULONG                 UserTime;                  /* 03C */
    LIST_ENTRY            ReadyListHead;             /* 040 */
    LIST_ENTRY            SwapListEntry;             /* 048 */
    PVOID                 VdmTrapcHandler;           /* 04C */
    LIST_ENTRY            ThreadListHead;            /* 050 */
    KSPIN_LOCK            ProcessLock;               /* 058 */
    KAFFINITY             Affinity;                  /* 05C */
    union
    {
        struct
        {
            ULONG         AutoAlignment:1;           /* 060.0 */
            ULONG         DisableBoost:1;            /* 060.1 */
            ULONG         DisableQuantum:1;          /* 060.2 */
            ULONG         ReservedFlags:29;          /* 060.3 */
        };
        ULONG             ProcessFlags;              /* 060 */
    };
    CHAR                  BasePriority;              /* 064 */
    CHAR                  QuantumReset;              /* 065 */
    UCHAR                 State;                     /* 066 */
    UCHAR                 ThreadSeed;                /* 067 */
    UCHAR                 PowerState;                /* 068 */
    UCHAR                 IdealNode;                 /* 069 */
    UCHAR                 Visited;                   /* 06A */
    KEXECUTE_OPTIONS      Flags;                     /* 06B */
    ULONG                 StackCount;                /* 06C */
    LIST_ENTRY            ProcessListEntry;          /* 070 */
} KPROCESS, *PKPROCESS;

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
#ifdef _REACTOS_
extern LOADER_PARAMETER_BLOCK NTSYSAPI KeLoaderBlock;
#else
extern PLOADER_PARAMETER_BLOCK NTSYSAPI KeLoaderBlock;
#endif

//
// Exported Hardware Data
//
extern KAFFINITY NTSYSAPI KeActiveProcessors;
extern CHAR NTSYSAPI KeNumberProcessors;
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
