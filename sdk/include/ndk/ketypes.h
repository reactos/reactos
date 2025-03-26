/*++ NDK Version: 0098

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    ketypes.h

Abstract:

    Type definitions for the Kernel services.

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
// Internal Exception Codes
//
#define KI_EXCEPTION_INTERNAL           0x10000000
#define KI_EXCEPTION_ACCESS_VIOLATION   (KI_EXCEPTION_INTERNAL | 0x04)

typedef struct _FIBER                                    /* Field offsets:    */
{                                                        /* i386  arm   x64   */
    PVOID FiberData;                                     /* 0x000 0x000 0x000 */
    struct _EXCEPTION_REGISTRATION_RECORD *ExceptionList;/* 0x004 0x004 0x008 */
    PVOID StackBase;                                     /* 0x008 0x008 0x010 */
    PVOID StackLimit;                                    /* 0x00C 0x00C 0x018 */
    PVOID DeallocationStack;                             /* 0x010 0x010 0x020 */
    CONTEXT FiberContext;                                /* 0x014 0x018 0x030 */
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    PVOID Wx86Tib;                                       /* 0x2E0 0x1b8 0x500 */
    struct _ACTIVATION_CONTEXT_STACK *ActivationContextStackPointer; /* 0x2E4 0x1bc 0x508 */
    PVOID FlsData;                                       /* 0x2E8 0x1c0 0x510 */
    ULONG GuaranteedStackBytes;                          /* 0x2EC 0x1c4 0x518 */
    ULONG TebFlags;                                      /* 0x2F0 0x1c8 0x51C */
#else
    ULONG GuaranteedStackBytes;                          /* 0x2E0         */
    PVOID FlsData;                                       /* 0x2E4         */
    struct _ACTIVATION_CONTEXT_STACK *ActivationContextStackPointer;
#endif
} FIBER, *PFIBER;

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

#else // NTOS_MODE_USER

//
// KPROCESSOR_MODE Type
//
typedef CCHAR KPROCESSOR_MODE;

//
// Dereferencable pointer to KUSER_SHARED_DATA in User-Mode
//
#define SharedUserData                  ((KUSER_SHARED_DATA *)USER_SHARED_DATA)

#ifdef _X86_
/* Macros for user-mode run-time checks of X86 system architecture */

#ifndef IsNEC_98
#define IsNEC_98     (SharedUserData->AlternativeArchitecture == NEC98x86)
#endif

#ifndef IsNotNEC_98
#define IsNotNEC_98  (SharedUserData->AlternativeArchitecture != NEC98x86)
#endif

/* User-mode cannot override the architecture */
#ifndef SetNEC_98
#define SetNEC_98
#endif

/* User-mode cannot override the architecture */
#ifndef SetNotNEC_98
#define SetNotNEC_98
#endif

#else // !_X86_
/* Correctly define these run-time definitions for non X86 machines */

#ifndef IsNEC_98
#define IsNEC_98 (FALSE)
#endif

#ifndef IsNotNEC_98
#define IsNotNEC_98 (TRUE)
#endif

#ifndef SetNEC_98
#define SetNEC_98
#endif

#ifndef SetNotNEC_98
#define SetNotNEC_98
#endif

#endif // _X86_

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

//
// Flags for NXSupportPolicy
//
#if (NTDDI_VERSION >= NTDDI_WINXPSP2)
#define NX_SUPPORT_POLICY_ALWAYSOFF 0
#define NX_SUPPORT_POLICY_ALWAYSON  1
#define NX_SUPPORT_POLICY_OPTIN     2
#define NX_SUPPORT_POLICY_OPTOUT    3
#endif

#endif // NTOS_MODE_USER

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

#define MAXIMUM_XSTATE_FEATURES             64

typedef struct _XSTATE_FEATURE
{
    ULONG Offset;
    ULONG Size;
} XSTATE_FEATURE, *PXSTATE_FEATURE;

typedef struct _XSTATE_CONFIGURATION
{
    ULONG64 EnabledFeatures;
#if (NTDDI_VERSION >= NTDDI_WINBLUE)
    ULONG64 EnabledVolatileFeatures;
#endif
    ULONG Size;
    union
    {
        ULONG ControlFlags;
        struct
        {
            ULONG OptimizedSave:1;
            ULONG CompactionEnabled:1; // WIN10+
        };
    };
    XSTATE_FEATURE Features[MAXIMUM_XSTATE_FEATURES];
#if (NTDDI_VERSION >= NTDDI_WIN10)
    ULONG64 EnabledSupervisorFeatures;
    ULONG64 AlignedFeatures;
    ULONG AllFeatureSize;
    ULONG AllFeatures[MAXIMUM_XSTATE_FEATURES];
#endif
#if (NTDDI_VERSION >= NTDDI_WIN10_RS5)
    ULONG64 EnabledUserVisibleSupervisorFeatures;
#endif
#if (NTDDI_VERSION >= NTDDI_WIN11)
    ULONG64 ExtendedFeatureDisableFeatures;
    ULONG AllNonLargeFeatureSize;
    ULONG Spare;
#endif
} XSTATE_CONFIGURATION, *PXSTATE_CONFIGURATION;

//
// Shared Kernel User Data
// Keep in sync with sdk/include/xdk/ketypes.h
//
typedef struct _KUSER_SHARED_DATA
{
    ULONG TickCountLowDeprecated;                           // 0x0
    ULONG TickCountMultiplier;                              // 0x4
    volatile KSYSTEM_TIME InterruptTime;                    // 0x8
    volatile KSYSTEM_TIME SystemTime;                       // 0x14
    volatile KSYSTEM_TIME TimeZoneBias;                     // 0x20
    USHORT ImageNumberLow;                                  // 0x2c
    USHORT ImageNumberHigh;                                 // 0x2e
    WCHAR NtSystemRoot[260];                                // 0x30
    ULONG MaxStackTraceDepth;                               // 0x238
    ULONG CryptoExponent;                                   // 0x23c
    ULONG TimeZoneId;                                       // 0x240
    ULONG LargePageMinimum;                                 // 0x244
    ULONG Reserved2[7];                                     // 0x248
    NT_PRODUCT_TYPE NtProductType;                          // 0x264
    BOOLEAN ProductTypeIsValid;                             // 0x268
    ULONG NtMajorVersion;                                   // 0x26c
    ULONG NtMinorVersion;                                   // 0x270
    BOOLEAN ProcessorFeatures[PROCESSOR_FEATURE_MAX];       // 0x274
    ULONG Reserved1;                                        // 0x2b4
    ULONG Reserved3;                                        // 0x2b8
    volatile ULONG TimeSlip;                                // 0x2bc
    ALTERNATIVE_ARCHITECTURE_TYPE AlternativeArchitecture;  // 0x2c0
    ULONG AltArchitecturePad[1];                            // 0x2c4
    LARGE_INTEGER SystemExpirationDate;                     // 0x2c8
    ULONG SuiteMask;                                        // 0x2d0
    BOOLEAN KdDebuggerEnabled;                              // 0x2d4
#if (NTDDI_VERSION >= NTDDI_WINXPSP2)
    UCHAR NXSupportPolicy;                                  // 0x2d5
#endif
    volatile ULONG ActiveConsoleId;                         // 0x2d8
    volatile ULONG DismountCount;                           // 0x2dc
    ULONG ComPlusPackage;                                   // 0x2e0
    ULONG LastSystemRITEventTickCount;                      // 0x2e4
    ULONG NumberOfPhysicalPages;                            // 0x2e8
    BOOLEAN SafeBootMode;                                   // 0x2ec
#if (NTDDI_VERSION >= NTDDI_WIN7)
    union
    {
        UCHAR TscQpcData;                                   // 0x2ed
        struct
        {
            UCHAR TscQpcEnabled:1;                          // 0x2ed
            UCHAR TscQpcSpareFlag:1;                        // 0x2ed
            UCHAR TscQpcShift:6;                            // 0x2ed
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    UCHAR TscQpcPad[2];                                     // 0x2ee
#endif
#if (NTDDI_VERSION >= NTDDI_VISTA)
    union
    {
        ULONG SharedDataFlags;                              // 0x2f0
        struct
        {
            ULONG DbgErrorPortPresent:1;                    // 0x2f0
            ULONG DbgElevationEnabled:1;                    // 0x2f0
            ULONG DbgVirtEnabled:1;                         // 0x2f0
            ULONG DbgInstallerDetectEnabled:1;              // 0x2f0
            ULONG DbgSystemDllRelocated:1;                  // 0x2f0
            ULONG DbgDynProcessorEnabled:1;                 // 0x2f0
            ULONG DbgSEHValidationEnabled:1;                // 0x2f0
            ULONG SpareBits:25;                             // 0x2f0
        } DUMMYSTRUCTNAME2;
    } DUMMYUNIONNAME2;
#else
    ULONG TraceLogging;
#endif
    ULONG DataFlagsPad[1];                                  // 0x2f4
    ULONGLONG TestRetInstruction;                           // 0x2f8
    ULONG SystemCall;                                       // 0x300
    ULONG SystemCallReturn;                                 // 0x304
    ULONGLONG SystemCallPad[3];                             // 0x308
    union
    {
        volatile KSYSTEM_TIME TickCount;                    // 0x320
        volatile ULONG64 TickCountQuad;                     // 0x320
        struct
        {
            ULONG ReservedTickCountOverlay[3];              // 0x320
            ULONG TickCountPad[1];                          // 0x32c
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME3;
    ULONG Cookie;                                           // 0x330
#if (NTDDI_VERSION >= NTDDI_VISTA)
    ULONG CookiePad[1];                                     // 0x334
    LONGLONG ConsoleSessionForegroundProcessId;             // 0x338
#endif
#if (NTDDI_VERSION >= NTDDI_WS03)
    ULONG Wow64SharedInformation[MAX_WOW64_SHARED_ENTRIES]; // 2K3: 0x334 / Vista+: 0x340
#endif
#if (NTDDI_VERSION >= NTDDI_VISTA)
#if (NTDDI_VERSION >= NTDDI_WIN7)
    USHORT UserModeGlobalLogger[16];                        // 0x380
#else
    USHORT UserModeGlobalLogger[8];                         // 0x380
    ULONG HeapTracingPid[2];                                // 0x390
    ULONG CritSecTracingPid[2];                             // 0x398
#endif
    ULONG ImageFileExecutionOptions;                        // 0x3a0
#if (NTDDI_VERSION >= NTDDI_VISTASP1)
    ULONG LangGenerationCount;                              // 0x3a4
#else
  /* 4 bytes padding */
#endif
    ULONGLONG Reserved5;                                    // 0x3a8
    volatile ULONG64 InterruptTimeBias;                     // 0x3b0
#endif // NTDDI_VERSION >= NTDDI_VISTA
#if (NTDDI_VERSION >= NTDDI_WIN7)
    volatile ULONG64 TscQpcBias;                            // 0x3b8
    volatile ULONG ActiveProcessorCount;                    // 0x3c0
    volatile USHORT ActiveGroupCount;                       // 0x3c4
    USHORT Reserved4;                                       // 0x3c6
    volatile ULONG AitSamplingValue;                        // 0x3c8
    volatile ULONG AppCompatFlag;                           // 0x3cc
    ULONGLONG SystemDllNativeRelocation;                    // 0x3d0
    ULONG SystemDllWowRelocation;                           // 0x3d8
    ULONG XStatePad[1];                                     // 0x3dc
    XSTATE_CONFIGURATION XState;                            // 0x3e0
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

typedef struct _KTIMER_TABLE_ENTRY
{
#if (NTDDI_VERSION >= NTDDI_LONGHORN) || defined(_M_ARM) || defined(_M_AMD64)
    KSPIN_LOCK Lock;
#endif
    LIST_ENTRY Entry;
    ULARGE_INTEGER Time;
} KTIMER_TABLE_ENTRY, *PKTIMER_TABLE_ENTRY;

typedef struct _KTIMER_TABLE
{
    PKTIMER TimerExpiry[64];
    KTIMER_TABLE_ENTRY TimerEntries[256];
} KTIMER_TABLE, *PKTIMER_TABLE;

typedef struct _KDPC_LIST
{
    SINGLE_LIST_ENTRY ListHead;
    SINGLE_LIST_ENTRY* LastEntry;
} KDPC_LIST, *PKDPC_LIST;

typedef struct _SYNCH_COUNTERS
{
    ULONG SpinLockAcquireCount;
    ULONG SpinLockContentionCount;
    ULONG SpinLockSpinCount;
    ULONG IpiSendRequestBroadcastCount;
    ULONG IpiSendRequestRoutineCount;
    ULONG IpiSendSoftwareInterruptCount;
    ULONG ExInitializeResourceCount;
    ULONG ExReInitializeResourceCount;
    ULONG ExDeleteResourceCount;
    ULONG ExecutiveResourceAcquiresCount;
    ULONG ExecutiveResourceContentionsCount;
    ULONG ExecutiveResourceReleaseExclusiveCount;
    ULONG ExecutiveResourceReleaseSharedCount;
    ULONG ExecutiveResourceConvertsCount;
    ULONG ExAcqResExclusiveAttempts;
    ULONG ExAcqResExclusiveAcquiresExclusive;
    ULONG ExAcqResExclusiveAcquiresExclusiveRecursive;
    ULONG ExAcqResExclusiveWaits;
    ULONG ExAcqResExclusiveNotAcquires;
    ULONG ExAcqResSharedAttempts;
    ULONG ExAcqResSharedAcquiresExclusive;
    ULONG ExAcqResSharedAcquiresShared;
    ULONG ExAcqResSharedAcquiresSharedRecursive;
    ULONG ExAcqResSharedWaits;
    ULONG ExAcqResSharedNotAcquires;
    ULONG ExAcqResSharedStarveExclusiveAttempts;
    ULONG ExAcqResSharedStarveExclusiveAcquiresExclusive;
    ULONG ExAcqResSharedStarveExclusiveAcquiresShared;
    ULONG ExAcqResSharedStarveExclusiveAcquiresSharedRecursive;
    ULONG ExAcqResSharedStarveExclusiveWaits;
    ULONG ExAcqResSharedStarveExclusiveNotAcquires;
    ULONG ExAcqResSharedWaitForExclusiveAttempts;
    ULONG ExAcqResSharedWaitForExclusiveAcquiresExclusive;
    ULONG ExAcqResSharedWaitForExclusiveAcquiresShared;
    ULONG ExAcqResSharedWaitForExclusiveAcquiresSharedRecursive;
    ULONG ExAcqResSharedWaitForExclusiveWaits;
    ULONG ExAcqResSharedWaitForExclusiveNotAcquires;
    ULONG ExSetResOwnerPointerExclusive;
    ULONG ExSetResOwnerPointerSharedNew;
    ULONG ExSetResOwnerPointerSharedOld;
    ULONG ExTryToAcqExclusiveAttempts;
    ULONG ExTryToAcqExclusiveAcquires;
    ULONG ExBoostExclusiveOwner;
    ULONG ExBoostSharedOwners;
    ULONG ExEtwSynchTrackingNotificationsCount;
    ULONG ExEtwSynchTrackingNotificationsAccountedCount;
} SYNCH_COUNTERS, *PSYNCH_COUNTERS;

//
// PRCB DPC Data
//
typedef struct _KDPC_DATA
{
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    KDPC_LIST DpcList;
#else
    LIST_ENTRY DpcListHead;
#endif
    ULONG_PTR DpcLock;
#if defined(_M_AMD64) || defined(_M_ARM)
    volatile LONG DpcQueueDepth;
#else
    volatile ULONG DpcQueueDepth;
#endif
    ULONG DpcCount;
#if (NTDDI_VERSION >= NTDDI_LONGHORN) || defined(_M_ARM)
    PKDPC ActiveDpc;
#endif
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
    KAFFINITY ProcessorMask;
    UCHAR Color;
    UCHAR Seed;
    UCHAR NodeNumber;
    struct _flags {
        UCHAR Removable : 1;
        UCHAR Fill : 7;
    } Flags;
    ULONG MmShiftedColor;
    ULONG_PTR FreeCount[2];
    struct _SINGLE_LIST_ENTRY *PfnDeferredList;
} KNODE, *PKNODE;

//
// Structure for Get/SetContext APC
//
typedef struct _GETSETCONTEXT
{
    KAPC Apc;
    KEVENT Event;
    KPROCESSOR_MODE Mode;
    CONTEXT Context;
} GETSETCONTEXT, *PGETSETCONTEXT;

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

/// FIXME: should move to rtltypes.h, but we can't include it here.
#if (NTDDI_VERSION >= NTDDI_WIN8)
typedef struct _RTL_RB_TREE
{
    PRTL_BALANCED_NODE Root;
    PRTL_BALANCED_NODE Min;
} RTL_RB_TREE, *PRTL_RB_TREE;
#endif

#if (NTDDI_VERSION >= NTDDI_WINBLUE)
typedef struct _KLOCK_ENTRY_LOCK_STATE
{
    union
    {
        struct
        {
#if (NTDDI_VERSION >= NTDDI_WIN10) // since 6.4.9841.0
            ULONG_PTR CrossThreadReleasable : 1;
#else
            ULONG_PTR Waiting : 1;
#endif
            ULONG_PTR Busy : 1;
            ULONG_PTR Reserved : (8 * sizeof(PVOID)) - 3; // previously Spare
            ULONG_PTR InTree : 1;
        };
        PVOID LockState;
    };
    union
    {
        PVOID SessionState;
        struct
        {
            ULONG SessionId;
#ifdef _WIN64
            ULONG SessionPad;
#endif
        };
    };
} KLOCK_ENTRY_LOCK_STATE, *PKLOCK_ENTRY_LOCK_STATE;

typedef struct _KLOCK_ENTRY
{
    union
    {
        RTL_BALANCED_NODE TreeNode;
        SINGLE_LIST_ENTRY FreeListEntry;
    };
#if (NTDDI_VERSION >= NTDDI_WIN10)
    union
    {
        ULONG EntryFlags;
        struct
        {
            UCHAR EntryOffset;
            union
            {
                UCHAR ThreadLocalFlags;
                struct
                {
                    UCHAR WaitingBit : 1;
                    UCHAR Spare0 : 7;
                };
            };
            union
            {
                UCHAR AcquiredByte;
                UCHAR AcquiredBit : 1;
            };
            union
            {
                UCHAR CrossThreadFlags;
                struct
                {
                    UCHAR HeadNodeBit : 1;
                    UCHAR IoPriorityBit : 1;
                    UCHAR IoQoSWaiter : 1; // since TH2
                    UCHAR Spare1 : 5;
                };
            };
        };
        struct
        {
            ULONG StaticState : 8;
            ULONG AllFlags : 24;
        };
    };
#ifdef _WIN64
    ULONG SpareFlags;
#endif
#else
    union
    {
        PVOID ThreadUnsafe;
        struct
        {
            volatile UCHAR HeadNodeByte;
            UCHAR Reserved1[2];
            volatile UCHAR AcquiredByte;
        };
    };
#endif

    union
    {
        KLOCK_ENTRY_LOCK_STATE LockState;
        PVOID LockUnsafe;
        struct
        {
#if (NTDDI_VERSION >= NTDDI_WIN10)
            volatile UCHAR CrossThreadReleasableAndBusyByte;
#else
            volatile UCHAR WaitingAndBusyByte;
#endif
            UCHAR Reserved[sizeof(PVOID) - 2];
            UCHAR InTreeByte;
            union
            {
                PVOID SessionState;
                struct
                {
                    ULONG SessionId;
#ifdef _WIN64
                    ULONG SessionPad;
#endif
                };
            };
        };
    };
    union
    {
        struct
        {
            RTL_RB_TREE OwnerTree;
            RTL_RB_TREE WaiterTree;
        };
        CHAR CpuPriorityKey;
    };
    ULONG_PTR EntryLock;
    union
    {
#if _WIN64
        ULONG AllBoosts : 17;
#else
        USHORT AllBoosts;
#endif
        struct
        {
            struct
            {
                USHORT CpuBoostsBitmap : 15;
                USHORT IoBoost : 1;
            };
            struct
            {
                USHORT IoQoSBoost : 1;
                USHORT IoNormalPriorityWaiterCount : 8;
                USHORT IoQoSWaiterCount : 7;
            };
        };
    };
#if _WIN64
    ULONG SparePad;
#endif
} KLOCK_ENTRY, *PKLOCK_ENTRY;

#endif

//
// Kernel Thread (KTHREAD)
//
#if (NTDDI_VERSION < NTDDI_WIN8)

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
#if !defined(_M_AMD64) && !defined(_M_ARM64) // [
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
#if !defined(_M_AMD64) && !defined(_M_ARM64) // [
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
#ifdef _WIN64 // [
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
    union
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

#else // not (NTDDI_VERSION < NTDDI_WIN8)

#if defined(_WIN64) && (NTDDI_VERSION < 0x06032580) // since WIN 8.1 Update1 6.3.9600.16384
#define NUMBER_OF_LOCK_ENTRIES 5
#else
#define NUMBER_OF_LOCK_ENTRIES 6
#endif

typedef struct _KTHREAD
{
    DISPATCHER_HEADER Header;
    PVOID SListFaultAddress;
    ULONG64 QuantumTarget;
    PVOID InitialStack;
    volatile VOID *StackLimit;
    PVOID StackBase;
    KSPIN_LOCK ThreadLock;
    volatile ULONG64 CycleTime;
#ifndef _WIN64
    volatile ULONG HighCycleTime;
    PVOID ServiceTable;
#endif
    ULONG CurrentRunTime;
    ULONG ExpectedRunTime;
    PVOID KernelStack;
    XSAVE_FORMAT* StateSaveArea;
    struct _KSCHEDULING_GROUP* SchedulingGroup;
    KWAIT_STATUS_REGISTER WaitRegister;
    BOOLEAN Running;
    BOOLEAN Alerted[MaximumMode];

    union
    {
        struct
        {
#if (NTDDI_VERSION < NTDDI_WIN10)
            ULONG KernelStackResident : 1;
#else
            ULONG AutoBoostActive : 1;
#endif
            ULONG ReadyTransition : 1;
#if (NTDDI_VERSION < NTDDI_WIN10TH2)
            ULONG ProcessReadyQueue : 1;
#endif
            ULONG ProcessReadyQueue : 1;
            ULONG WaitNext : 1;
            ULONG SystemAffinityActive : 1;
            ULONG Alertable : 1;
#if (NTDDI_VERSION < NTDDI_WIN81)
            ULONG CodePatchInProgress : 1;
#endif
            ULONG UserStackWalkActive : 1;
            ULONG ApcInterruptRequest : 1;
            ULONG QuantumEndMigrate : 1;
            ULONG UmsDirectedSwitchEnable : 1;
            ULONG TimerActive : 1;
            ULONG SystemThread : 1;
            ULONG ProcessDetachActive : 1;
            ULONG CalloutActive : 1;
            ULONG ScbReadyQueue : 1;
            ULONG ApcQueueable : 1;
            ULONG ReservedStackInUse : 1;
            ULONG UmsPerformingSyscall : 1;
            ULONG DisableStackCheck : 1;
            ULONG Reserved : 12;
        };
        LONG MiscFlags;
    };

    union
    {
        struct
        {
            ULONG AutoAlignment : 1;
            ULONG DisableBoost : 1;
            ULONG UserAffinitySet : 1;
            ULONG AlertedByThreadId : 1;
            ULONG QuantumDonation : 1;
            ULONG EnableStackSwap : 1;
            ULONG GuiThread : 1;
            ULONG DisableQuantum : 1;
            ULONG ChargeOnlyGroup : 1;
            ULONG DeferPreemption : 1;
            ULONG QueueDeferPreemption : 1;
            ULONG ForceDeferSchedule : 1;
            ULONG ExplicitIdealProcessor : 1;
            ULONG FreezeCount : 1;
#if (NTDDI_VERSION >= 0x060324D7) // since 6.3.9431.0
            ULONG TerminationApcRequest : 1;
#endif
#if (NTDDI_VERSION >= 0x06032580) // since 6.3.9600.16384
            ULONG AutoBoostEntriesExhausted : 1;
#endif
#if (NTDDI_VERSION >= 0x06032580) // since 6.3.9600.17031
            ULONG KernelStackResident : 1;
#endif
#if (NTDDI_VERSION >= NTDDI_WIN10)
            ULONG CommitFailTerminateRequest : 1;
            ULONG ProcessStackCountDecremented : 1;
            ULONG ThreadFlagsSpare : 5;
#endif
            ULONG EtwStackTraceApcInserted : 8;
#if (NTDDI_VERSION < NTDDI_WIN10)
            ULONG ReservedFlags : 10;
#endif
        };
        LONG ThreadFlags;
    };

#if (NTDDI_VERSION >= NTDDI_WIN10)
    volatile UCHAR Tag;
    UCHAR SystemHeteroCpuPolicy;
    UCHAR UserHeteroCpuPolicy : 7;
    UCHAR ExplicitSystemHeteroCpuPolicy : 1;
    UCHAR Spare0;
#else
    ULONG Spare0;
#endif
    ULONG SystemCallNumber;
#ifdef _WIN64
    ULONG Spare1; // Win 10: Spare10
#endif
    PVOID FirstArgument;
    PKTRAP_FRAME TrapFrame;

    union
    {
        KAPC_STATE ApcState;
        struct
        {
            UCHAR ApcStateFill[RTL_SIZEOF_THROUGH_FIELD(KAPC_STATE, UserApcPending)]; // 32bit: 23/0x17, 64bit: 43/0x2B
            SCHAR Priority;
            ULONG UserIdealProcessor;
        };
    };

#ifndef _WIN64
    ULONG ContextSwitches;
    volatile UCHAR State;
#if (NTDDI_VERSION >= NTDDI_WIN10) // since 10.0.10074.0
    CHAR Spare12;
#else
    CHAR NpxState;
#endif
    KIRQL WaitIrql;
    KPROCESSOR_MODE WaitMode;
#endif

    volatile INT_PTR WaitStatus;
    PKWAIT_BLOCK WaitBlockList;
    union
    {
        LIST_ENTRY WaitListEntry;
        SINGLE_LIST_ENTRY SwapListEntry;
    };
    PKQUEUE Queue;
    PVOID Teb;
#if (NTDDI_VERSION >= NTDDI_WIN8 /* 0x060223F0 */) // since 6.2.9200.16384
    ULONG64 RelativeTimerBias;
#endif
    KTIMER Timer;

    union
    {
        DECLSPEC_ALIGN(8) KWAIT_BLOCK WaitBlock[THREAD_WAIT_OBJECTS + 1];
#ifdef _WIN64
        struct
        {
            UCHAR WaitBlockFill4[FIELD_OFFSET(KWAIT_BLOCK, SpareLong)]; // 32bit: -, 64bit: 20/0x14
            ULONG ContextSwitches;
        };
        struct
        {
            UCHAR WaitBlockFill5[1 * sizeof(KWAIT_BLOCK) + FIELD_OFFSET(KWAIT_BLOCK, SpareLong)]; // 32bit: -, 64bit: 68/0x44
            UCHAR State;
#if (NTDDI_VERSION >= NTDDI_WIN10)
            CHAR Spare13;
#else
            CHAR NpxState;
#endif
            UCHAR WaitIrql;
            CHAR WaitMode;
        };
        struct
        {
            UCHAR WaitBlockFill6[2 * sizeof(KWAIT_BLOCK) + FIELD_OFFSET(KWAIT_BLOCK, SpareLong)]; // 32bit: -, 64bit: 116/0x74
            ULONG WaitTime;
        };
        struct
        {
            UCHAR WaitBlockFill7[3 * sizeof(KWAIT_BLOCK) + FIELD_OFFSET(KWAIT_BLOCK, SpareLong)]; // 32bit: -, 64bit: 164/0xA4
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
#endif
        struct
        {
            UCHAR WaitBlockFill8[FIELD_OFFSET(KWAIT_BLOCK, SparePtr)]; // 32bit: 20/0x14, 64bit: 40/0x28
            struct _KTHREAD_COUNTERS *ThreadCounters;
        };
        struct
        {
            UCHAR WaitBlockFill9[1 * sizeof(KWAIT_BLOCK) + FIELD_OFFSET(KWAIT_BLOCK, SparePtr)]; // 32bit: 44/0x2C, 64bit: 88/0x58
            PXSTATE_SAVE XStateSave;
        };
        struct
        {
            UCHAR WaitBlockFill10[2 * sizeof(KWAIT_BLOCK) + FIELD_OFFSET(KWAIT_BLOCK, SparePtr)]; // 32bit: 68/0x44, 64bit: 136/0x88
            PVOID Win32Thread;
        };
        struct
        {
            UCHAR WaitBlockFill11[3 * sizeof(KWAIT_BLOCK) + FIELD_OFFSET(KWAIT_BLOCK, Object)]; // 32bit: 88/0x58, 64bit: 176/0xB0
#ifdef _WIN64
            struct _UMS_CONTROL_BLOCK* Ucb;
            struct _KUMS_CONTEXT_HEADER* Uch;
#else
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
#endif
        };
    };

#ifdef _WIN64
    PVOID TebMappedLowVa;
#endif
    LIST_ENTRY QueueListEntry;
#if (NTDDI_VERSION >= 0x060223F0) // since 6.2.9200.16384
    union
    {
        ULONG NextProcessor;
        struct
        {
            ULONG NextProcessorNumber : 31;
            ULONG SharedReadyQueue : 1;
        };
    };
    LONG QueuePriority;
#else
    ULONG NextProcessor;
    ULONG DeferredProcessor;
#endif
    PKPROCESS Process;

    union
    {
        GROUP_AFFINITY UserAffinity;
        struct
        {
            UCHAR UserAffinityFill[FIELD_OFFSET(GROUP_AFFINITY, Reserved)]; // 32bit: 6/0x6, 64bit: 10/0x0A
            CHAR PreviousMode;
            CHAR BasePriority;
            union
            {
                CHAR PriorityDecrement;
                struct
                {
                    UCHAR ForegroundBoost : 4;
                    UCHAR UnusualBoost : 4;
                };
            };
            UCHAR Preempted;
            UCHAR AdjustReason;
            CHAR AdjustIncrement;
        };
    };

#if (NTDDI_VERSION >= NTDDI_WIN10) // since 10.0.10240.16384
    ULONG_PTR AffinityVersion;
#endif
    union
    {
        GROUP_AFFINITY Affinity;
        struct
        {
            UCHAR AffinityFill[FIELD_OFFSET(GROUP_AFFINITY, Reserved)]; // 32bit: 6/0x6, 64bit: 10/0x0A
            UCHAR ApcStateIndex;
            UCHAR WaitBlockCount;
            ULONG IdealProcessor;
        };
    };

#if (NTDDI_VERSION >= NTDDI_WIN10) // since 10.0.10240.16384
#ifdef _WIN64
    ULONG64 NpxState;
#else
    ULONG Spare15;
#endif
#else
    PKAPC_STATE ApcStatePointer[2];
#endif

    union
    {
        KAPC_STATE SavedApcState;
        struct
        {
            UCHAR SavedApcStateFill[FIELD_OFFSET(KAPC_STATE, UserApcPending) + 1]; // 32bit: 23/0x17, 64bit: 43/0x2B
            UCHAR WaitReason;
            CHAR SuspendCount;
            CHAR Saturation;
            SHORT SListFaultCount;
        };
    };

    union
    {
        KAPC SchedulerApc;
        struct
        {
            UCHAR SchedulerApcFill0[FIELD_OFFSET(KAPC, SpareByte0)]; // 32bit:  1/0x01, 64bit: 1/0x01
            UCHAR ResourceIndex;
        };
        struct
        {
            UCHAR SchedulerApcFill1[FIELD_OFFSET(KAPC, SpareByte1)]; // 32bit:  3/0x03, 64bit: 3/0x03
            UCHAR QuantumReset;
        };
        struct
        {
            UCHAR SchedulerApcFill2[FIELD_OFFSET(KAPC, SpareLong0)]; // 32bit:  4/0x04, 64bit: 4/0x04
            ULONG KernelTime;
        };
        struct
        {
            UCHAR SuspendApcFill3[FIELD_OFFSET(KAPC, SystemArgument1)]; // 32 bit:, 64 bit: 64/0x40
            PKPRCB WaitPrcb;
        };
        struct
        {
            UCHAR SchedulerApcFill4[FIELD_OFFSET(KAPC, SystemArgument2)]; // 32 bit:, 64 bit: 72/0x48
            PVOID LegoData;
        };
        struct
        {
            UCHAR SchedulerApcFill5[FIELD_OFFSET(KAPC, Inserted) + 1]; // 32 bit:, 64 bit: 83/0x53
            UCHAR CallbackNestingLevel;
            ULONG UserTime;
        };
    };

    KEVENT SuspendEvent;
    LIST_ENTRY ThreadListEntry;
    LIST_ENTRY MutantListHead;

#if (NTDDI_VERSION >= NTDDI_WIN10)
    UCHAR AbEntrySummary;
    UCHAR AbWaitEntryCount;
    USHORT Spare20;
#if _WIN64
    ULONG SecureThreadCookie;
#endif
#elif (NTDDI_VERSION >= NTDDI_WINBLUE) // 6.3.9431.0
    SINGLE_LIST_ENTRY LockEntriesFreeList;
#endif

#if (NTDDI_VERSION >= NTDDI_WINBLUE /* 0x06032580 */) // since 6.3.9600.16384
    KLOCK_ENTRY LockEntries[NUMBER_OF_LOCK_ENTRIES];
    SINGLE_LIST_ENTRY PropagateBoostsEntry;
    SINGLE_LIST_ENTRY IoSelfBoostsEntry;
    UCHAR PriorityFloorCounts[16];
    ULONG PriorityFloorSummary;
    volatile LONG AbCompletedIoBoostCount;
  #if (NTDDI_VERSION >= NTDDI_WIN10_RS1)
    LONG AbCompletedIoQoSBoostCount;
  #endif

  #if (NTDDI_VERSION >= NTDDI_WIN10) // since 10.0.10240.16384
    volatile SHORT KeReferenceCount;
  #else
    volatile SHORT AbReferenceCount;
  #endif
  #if (NTDDI_VERSION >= 0x06040000) // since 6.4.9841.0
    UCHAR AbOrphanedEntrySummary;
    UCHAR AbOwnedEntryCount;
  #else
    UCHAR AbFreeEntryCount;
    UCHAR AbWaitEntryCount;
  #endif
    ULONG ForegroundLossTime;
    union
    {
        LIST_ENTRY GlobalForegroundListEntry;
        struct
        {
            SINGLE_LIST_ENTRY ForegroundDpcStackListEntry;
            ULONG_PTR InGlobalForegroundList;
        };
    };
#endif

#if _WIN64
    LONG64 ReadOperationCount;
    LONG64 WriteOperationCount;
    LONG64 OtherOperationCount;
    LONG64 ReadTransferCount;
    LONG64 WriteTransferCount;
    LONG64 OtherTransferCount;
#endif
#if (NTDDI_VERSION >= NTDDI_WIN10) // since 10.0.10041.0
    struct _KSCB *QueuedScb;
#ifndef _WIN64
    ULONG64 NpxState;
#endif
#endif
} KTHREAD;

#endif


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
    ULONG_PTR Unused0;
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
    volatile KAFFINITY ActiveProcessors;
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

#if (NTDDI_VERSION >= NTDDI_WIN8)
//
// Entropy Timing State
//
typedef struct _KENTROPY_TIMING_STATE
{
    ULONG EntropyCount;
    ULONG Buffer[64];
    KDPC Dpc;
    ULONG LastDeliveredBuffer;
    PULONG RawDataBuffer;
} KENTROPY_TIMING_STATE, *PKENTROPY_TIMING_STATE;

//
// Constants from ks386.inc, ksamd64.inc and ksarm.h
//
#define KENTROPY_TIMING_INTERRUPTS_PER_BUFFER 0x400
#define KENTROPY_TIMING_BUFFER_MASK 0x7ff
#define KENTROPY_TIMING_ANALYSIS 0x0

#endif /* (NTDDI_VERSION >= NTDDI_WIN8) */

//
// Exported Loader Parameter Block
//
extern struct _LOADER_PARAMETER_BLOCK NTSYSAPI *KeLoaderBlock;

//
// Exported Hardware Data
//
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
