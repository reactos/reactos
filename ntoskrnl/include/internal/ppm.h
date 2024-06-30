/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Internal header for the Processor Power Management (PPM)
 * COPYRIGHT:   Copyright 2023 George Bișoc <george.bisoc@reactos.org>
 */

//
// Define this if you want debugging support
//
#define _PPM_DEBUG_                                     0x00

//
// These define the Debug Masks Supported
//
#define PPM_VETO_DEBUG                                  0x01
#define PPM_HETERO_DEBUG                                0x02
#define PPM_CORE_PARK_DEBUG                             0x04
#define PPM_PERF_DEBUG                                  0x06
#define PPM_INIT_SUBSYSTEM_DEBUG                        0x08

//
// Debug/Tracing support
//
#if _PPM_DEBUG_
#ifdef NEW_DEBUG_SYSTEM_IMPLEMENTED // enable when Debug Filters are implemented
#define PPMTRACE DbgPrintEx
#else
#define PPMTRACE(x, ...)                                 \
    if (x & PpmTraceLevel) DbgPrint(__VA_ARGS__)
#endif
#else
#define PPMTRACE(x, fmt, ...) DPRINT(fmt, ##__VA_ARGS__)
#endif

typedef enum _PPM_PROCESSOR_CSTATE_TYPES
{
    ProcessorC0State,
    ProcessorC1State,
    ProcessorC2State,
    ProcessorC3State
} PPM_PROCESSOR_CSTATE_TYPES;

#if 0
//
// Idle synchronization state
//
typedef union _PPM_IDLE_SYNCHRONIZATION_STATE
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
} PPM_IDLE_SYNCHRONIZATION_STATE, *PPPM_IDLE_SYNCHRONIZATION_STATE;

//
// Idle bucket time type
//
typedef enum _PPM_IDLE_BUCKET_TIME_TYPE
{
    PpmIdleBucketTimeInQpc,
    PpmIdleBucketTimeIn100ns,
    PpmIdleBucketTimeMaximum
} PPM_IDLE_BUCKET_TIME_TYPE;

//
// Performance state selection
//
typedef struct _PERFINFO_PPM_STATE_SELECTION
{
    ULONG SelectedState;
    ULONG VetoedStates;
    ULONG VetoReason[1];
} PERFINFO_PPM_STATE_SELECTION, *PPERFINFO_PPM_STATE_SELECTION;

//
// Concurrency accounting
//
typedef struct _PPM_CONCURRENCY_ACCOUNTING
{
    ULONG Lock;
    ULONG Processors;
    ULONG ActiveProcessors;
    ULONGLONG LastUpdateTime;
    ULONGLONG TotalTime;
    ULONGLONG AccumulatedTime[1];
} PPM_CONCURRENCY_ACCOUNTING, *PPPM_CONCURRENCY_ACCOUNTING;

//
// Fixed Function Hardware (FFH) throttle state info
//
typedef struct _PPM_FFH_THROTTLE_STATE_INFO
{
    BOOLEAN EnableLogging;
    ULONG MismatchCount;
    UCHAR Initialized;
    ULONGLONG LastValue;
    LARGE_INTEGER LastLogTickCount;
} PPM_FFH_THROTTLE_STATE_INFO, *PPPM_FFH_THROTTLE_STATE_INFO;

//
// Selection statistics
//
typedef struct _PPM_SELECTION_STATISTICS
{
    ULONGLONG PlatformOnlyCount;
    ULONGLONG PreVetoCount;
    ULONGLONG VetoCount;
    ULONGLONG IdleDurationCount;
    ULONGLONG LatencyCount;
    ULONGLONG InterruptibleCount;
    ULONGLONG DeviceDependencyCount;
    ULONGLONG ProcessorDependencyCount;
    ULONGLONG WrongProcessorCount;
    ULONGLONG LegacyOverrideCount;
    ULONGLONG CstateCheckCount;
    ULONGLONG NoCStateCount;
    ULONGLONG SelectedCount;
} PPM_SELECTION_STATISTICS, *PPPM_SELECTION_STATISTICS;

//
// Veto accounting
//
typedef struct _PPM_VETO_ACCOUNTING
{
    volatile LONG VetoPresent;
    LIST_ENTRY VetoListHead;
} PPM_VETO_ACCOUNTING, *PPPM_VETO_ACCOUNTING;

//
// Power processor idle state
//
typedef struct _PPM_IDLE_STATE
{
    KAFFINITY_EX DomainMembers;
    ULONG Latency;
    ULONG BreakEvenDuration;
    ULONG Power;
    ULONG StateFlags;
    PPM_VETO_ACCOUNTING VetoAccounting;
    UCHAR StateType;
    BOOLEAN InterruptsEnabled;
    UCHAR Interruptible;
    BOOLEAN ContextRetained;
    BOOLEAN CacheCoherent;
    BOOLEAN WakesSpuriously;
    BOOLEAN PlatformOnly;
    BOOLEAN NoCState;
} PPM_IDLE_STATE, *PPPM_IDLE_STATE;
#endif

//
// Processor P/C states
//
typedef struct _PPM_PROCESSOR_STATES
{
    PPM_PROCESSOR_CSTATE_TYPES CurrentCState;
    PPM_PROCESSOR_CSTATE_TYPES TargetCstate;
    // PPM_PROCESSOR_PSTATE PState;
} PPM_PROCESSOR_STATES, *PPPM_PROCESSOR_STATES;


//
// Initialization routines
//
CODE_SEG("INIT")
NTSTATUS
NTAPI
PpmInitialize(
    _In_ BOOLEAN EarlyPhase);

//
// Processor idle functions
//
VOID
FASTCALL
PpmIdle(
    _In_ PPROCESSOR_POWER_STATE PowerState);

//
// Processor performance functions
//
VOID
NTAPI
PpmPerfIdleDpcRoutine(
    _In_ PKDPC Dpc,
    _In_ PVOID DeferredContext,
    _In_ PVOID SystemArgument1,
    _In_ PVOID SystemArgument2);

/* EOF */
