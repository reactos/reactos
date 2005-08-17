/*
 * PROJECT:         ReactOS Native Headers
 * FILE:            include/ndk/ketypes.h
 * PURPOSE:         Definitions for Kernel Types not defined in DDK/IFS
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 06/10/04
 */
#ifndef _KETYPES_H
#define _KETYPES_H

/* DEPENDENCIES **************************************************************/
#ifndef NTOS_MODE_USER
#include <arc/arc.h>
#include "arch/ketypes.h"
#endif

/* CONSTANTS *****************************************************************/
#define SSDT_MAX_ENTRIES 4
#define PROCESSOR_FEATURE_MAX 64
#define CONTEXT_DEBUGGER (CONTEXT_FULL | CONTEXT_FLOATING_POINT)
#define THREAD_WAIT_OBJECTS 4
#define THREAD_ALERT 0x4

#ifdef NTOS_MODE_USER
#define SharedUserData ((KUSER_SHARED_DATA * CONST) USER_SHARED_DATA)
#endif

/* EXPORTED DATA *************************************************************/
#ifndef NTOS_MODE_USER
extern CHAR NTOSAPI KeNumberProcessors;
extern LOADER_PARAMETER_BLOCK NTOSAPI KeLoaderBlock;
extern ULONG NTOSAPI KeDcacheFlushCount;
extern ULONG NTOSAPI KeIcacheFlushCount;
extern KAFFINITY NTOSAPI KeActiveProcessors;
extern ULONG NTOSAPI KiDmaIoCoherency; /* RISC Architectures only */
extern ULONG NTOSAPI KeMaximumIncrement;
extern ULONG NTOSAPI KeMinimumIncrement;
extern ULONG NTOSAPI NtBuildNumber;
extern SSDT_ENTRY NTOSAPI KeServiceDescriptorTable[SSDT_MAX_ENTRIES];
extern SSDT_ENTRY NTOSAPI KeServiceDescriptorTableShadow[SSDT_MAX_ENTRIES];
#endif

/* ENUMERATIONS **************************************************************/

#ifdef NTOS_MODE_USER
typedef enum _EVENT_TYPE
{
    NotificationEvent,
    SynchronizationEvent
} EVENT_TYPE;

typedef enum _TIMER_TYPE
{
    NotificationTimer,
    SynchronizationTimer
} TIMER_TYPE;

typedef enum _WAIT_TYPE
{
    WaitAll,
    WaitAny
} WAIT_TYPE;

typedef enum _MODE
{
    KernelMode,
    UserMode,
    MaximumMode
} MODE;

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

/* FUNCTION TYPES ************************************************************/

typedef VOID
(NTAPI *PKNORMAL_ROUTINE)(
    IN PVOID  NormalContext,
    IN PVOID  SystemArgument1,
    IN PVOID  SystemArgument2);

typedef VOID
(NTAPI *PTIMER_APC_ROUTINE)(
    IN PVOID  TimerContext,
    IN ULONG  TimerLowValue,
    IN LONG  TimerHighValue);

/* TYPES *********************************************************************/

typedef LONG KPRIORITY;

#ifdef NTOS_MODE_USER
typedef CCHAR KPROCESSOR_MODE;

typedef struct _KSYSTEM_TIME
{
    ULONG LowPart;
    LONG High1Time;
    LONG High2Time;
} KSYSTEM_TIME, *PKSYSTEM_TIME;

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
    ULONGLONG Fill0;
    ULONGLONG SystemCall[4];
    union {
        volatile KSYSTEM_TIME TickCount;
        volatile ULONG64 TickCountQuad;
    };
} KUSER_SHARED_DATA, *PKUSER_SHARED_DATA;
#endif

#ifndef NTOS_MODE_USER
typedef struct _CONFIGURATION_COMPONENT_DATA
{
    struct _CONFIGURATION_COMPONENT_DATA *Parent;
    struct _CONFIGURATION_COMPONENT_DATA *Child;
    struct _CONFIGURATION_COMPONENT_DATA *Sibling;
    CONFIGURATION_COMPONENT Component;
} CONFIGURATION_COMPONENT_DATA, *PCONFIGURATION_COMPONENT_DATA;

typedef enum _KAPC_ENVIRONMENT
{
    OriginalApcEnvironment,
    AttachedApcEnvironment,
    CurrentApcEnvironment
} KAPC_ENVIRONMENT;

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

typedef struct _KPROFILE
{
    CSHORT Type;
    CSHORT Size;
    LIST_ENTRY ListEntry;
    PVOID RegionStart;
    PVOID RegionEnd;
    ULONG BucketShift;
    PVOID Buffer;
    KPROFILE_SOURCE Source;
    ULONG Affinity;
    BOOLEAN Active;
    struct _KPROCESS *Process;
} KPROFILE, *PKPROFILE;

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

typedef struct _KEVENT_PAIR
{
    CSHORT Type;
    CSHORT Size;
    KEVENT LowEvent;
    KEVENT HighEvent;
} KEVENT_PAIR, *PKEVENT_PAIR;

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

#include <pshpack1.h>

typedef struct _KTHREAD
{
    DISPATCHER_HEADER DispatcherHeader;    /* 00 */
    LIST_ENTRY        MutantListHead;      /* 10 */
    PVOID             InitialStack;        /* 18 */
    ULONG_PTR         StackLimit;          /* 1C */
    struct _TEB       *Teb;                /* 20 */
    PVOID             TlsArray;            /* 24 */
    PVOID             KernelStack;         /* 28 */
    UCHAR             DebugActive;         /* 2C */
    UCHAR             State;               /* 2D */
    BOOLEAN           Alerted[2];          /* 2E */
    UCHAR             Iopl;                /* 30 */
    UCHAR             NpxState;            /* 31 */
    CHAR              Saturation;          /* 32 */
    CHAR              Priority;            /* 33 */
    KAPC_STATE        ApcState;            /* 34 */
    ULONG             ContextSwitches;     /* 4C */
    LONG              WaitStatus;          /* 50 */
    KIRQL             WaitIrql;            /* 54 */
    CHAR              WaitMode;            /* 55 */
    UCHAR             WaitNext;            /* 56 */
    UCHAR             WaitReason;          /* 57 */
    union                                  /* 58 */
    {
        PKWAIT_BLOCK  WaitBlockList;       /* 58 */
        PKGATE        GateObject;          /* 58 */
    };                                     /* 58 */
    LIST_ENTRY        WaitListEntry;       /* 5C */
    ULONG             WaitTime;            /* 64 */
    CHAR              BasePriority;        /* 68 */
    UCHAR             DecrementCount;      /* 69 */
    UCHAR             PriorityDecrement;   /* 6A */
    CHAR              Quantum;             /* 6B */
    KWAIT_BLOCK       WaitBlock[4];        /* 6C */
    PVOID             LegoData;            /* CC */
    union
    {
        struct
        {
            USHORT    KernelApcDisable;
            USHORT    SpecialApcDisable;
        };
        ULONG         CombinedApcDisable;  /* D0 */
    };
    KAFFINITY         UserAffinity;        /* D4 */
    UCHAR             SystemAffinityActive;/* D8 */
    UCHAR             PowerState;          /* D9 */
    UCHAR             NpxIrql;             /* DA */
    UCHAR             Pad[1];              /* DB */
    PVOID             ServiceTable;        /* DC */
    struct _KQUEUE    *Queue;              /* E0 */
    KSPIN_LOCK        ApcQueueLock;        /* E4 */
    KTIMER            Timer;               /* E8 */
    LIST_ENTRY        QueueListEntry;      /* 110 */
    KAFFINITY         Affinity;            /* 118 */
    UCHAR             Preempted;           /* 11C */
    UCHAR             ProcessReadyQueue;   /* 11D */
    UCHAR             KernelStackResident; /* 11E */
    UCHAR             NextProcessor;       /* 11F */
    PVOID             CallbackStack;       /* 120 */
    struct _W32THREAD *Win32Thread;        /* 124 */
    struct _KTRAP_FRAME *TrapFrame;        /* 128 */
    PKAPC_STATE       ApcStatePointer[2];  /* 12C */
    UCHAR             EnableStackSwap;     /* 134 */
    UCHAR             LargeStack;          /* 135 */
    UCHAR             ResourceIndex;       /* 136 */
    UCHAR             PreviousMode;        /* 137 */
    ULONG             KernelTime;          /* 138 */
    ULONG             UserTime;            /* 13C */
    KAPC_STATE        SavedApcState;       /* 140 */
    UCHAR             Alertable;           /* 158 */
    UCHAR             ApcStateIndex;       /* 159 */
    UCHAR             ApcQueueable;        /* 15A */
    UCHAR             AutoAlignment;       /* 15B */
    PVOID             StackBase;           /* 15C */
    KAPC              SuspendApc;          /* 160 */
    KSEMAPHORE        SuspendSemaphore;    /* 190 */
    LIST_ENTRY        ThreadListEntry;     /* 1A4 */
    CHAR              FreezeCount;         /* 1AC */
    UCHAR             SuspendCount;        /* 1AD */
    UCHAR             IdealProcessor;      /* 1AE */
    UCHAR             DisableBoost;        /* 1AF */
    UCHAR             QuantumReset;        /* 1B0 */
} KTHREAD;

#include <poppack.h>

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
} KPROCESS;
#endif /* !NTOS_MODE_USER */

#endif
