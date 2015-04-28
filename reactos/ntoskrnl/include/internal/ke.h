#pragma once

/* INCLUDES *****************************************************************/

#include "arch/ke.h"

/* INTERNAL KERNEL TYPES ****************************************************/

typedef struct _WOW64_PROCESS
{
  PVOID Wow64;
} WOW64_PROCESS, *PWOW64_PROCESS;

typedef struct _KPROFILE_SOURCE_OBJECT
{
    KPROFILE_SOURCE Source;
    LIST_ENTRY ListEntry;
} KPROFILE_SOURCE_OBJECT, *PKPROFILE_SOURCE_OBJECT;

typedef enum _CONNECT_TYPE
{
    NoConnect,
    NormalConnect,
    ChainConnect,
    UnknownConnect
} CONNECT_TYPE, *PCONNECT_TYPE;

typedef struct _DISPATCH_INFO
{
    CONNECT_TYPE Type;
    PKINTERRUPT Interrupt;
    PKINTERRUPT_ROUTINE NoDispatch;
    PKINTERRUPT_ROUTINE InterruptDispatch;
    PKINTERRUPT_ROUTINE FloatingDispatch;
    PKINTERRUPT_ROUTINE ChainedDispatch;
    PKINTERRUPT_ROUTINE *FlatDispatch;
} DISPATCH_INFO, *PDISPATCH_INFO;

typedef struct _DEFERRED_REVERSE_BARRIER
{
    ULONG Barrier;
    ULONG TotalProcessors;
} DEFERRED_REVERSE_BARRIER, *PDEFERRED_REVERSE_BARRIER;

typedef struct _KI_SAMPLE_MAP
{
    LARGE_INTEGER PerfStart;
    LARGE_INTEGER PerfEnd;
    LONGLONG PerfDelta;
    LARGE_INTEGER PerfFreq;
    LONGLONG TSCStart;
    LONGLONG TSCEnd;
    LONGLONG TSCDelta;
    ULONG MHz;
} KI_SAMPLE_MAP, *PKI_SAMPLE_MAP;

typedef struct _KTIMER_TABLE_ENTRY
{
    LIST_ENTRY Entry;
    ULARGE_INTEGER Time;
} KTIMER_TABLE_ENTRY, *PKTIMER_TABLE_ENTRY;

#define MAX_TIMER_DPCS                      16

typedef struct _DPC_QUEUE_ENTRY
{
    PKDPC Dpc;
    PKDEFERRED_ROUTINE Routine;
    PVOID Context;
} DPC_QUEUE_ENTRY, *PDPC_QUEUE_ENTRY;

typedef struct _KNMI_HANDLER_CALLBACK
{
    struct _KNMI_HANDLER_CALLBACK* Next;
    PNMI_CALLBACK Callback;
    PVOID Context;
    PVOID Handle;
} KNMI_HANDLER_CALLBACK, *PKNMI_HANDLER_CALLBACK;

typedef PCHAR
(NTAPI *PKE_BUGCHECK_UNICODE_TO_ANSI)(
    IN PUNICODE_STRING Unicode,
    IN PCHAR Ansi,
    IN ULONG Length
);

extern PKNMI_HANDLER_CALLBACK KiNmiCallbackListHead;
extern KSPIN_LOCK KiNmiCallbackListLock;
extern PVOID KeUserApcDispatcher;
extern PVOID KeUserCallbackDispatcher;
extern PVOID KeUserExceptionDispatcher;
extern PVOID KeRaiseUserExceptionDispatcher;
extern LARGE_INTEGER KeBootTime;
extern ULONGLONG KeBootTimeBias;
extern BOOLEAN ExCmosClockIsSane;
extern USHORT KeProcessorArchitecture;
extern USHORT KeProcessorLevel;
extern USHORT KeProcessorRevision;
extern ULONG KeFeatureBits;
extern KNODE KiNode0;
extern PKNODE KeNodeBlock[1];
extern UCHAR KeNumberNodes;
extern UCHAR KeProcessNodeSeed;
extern ETHREAD KiInitialThread;
extern EPROCESS KiInitialProcess;
extern PULONG KiInterruptTemplateObject;
extern PULONG KiInterruptTemplateDispatch;
extern PULONG KiInterruptTemplate2ndDispatch;
extern ULONG KiUnexpectedEntrySize;
extern ULONG_PTR KiDoubleFaultStack;
extern EX_PUSH_LOCK KernelAddressSpaceLock;
extern ULONG KiMaximumDpcQueueDepth;
extern ULONG KiMinimumDpcRate;
extern ULONG KiAdjustDpcThreshold;
extern ULONG KiIdealDpcRate;
extern BOOLEAN KeThreadDpcEnable;
extern LARGE_INTEGER KiTimeIncrementReciprocal;
extern UCHAR KiTimeIncrementShiftCount;
extern ULONG KiTimeLimitIsrMicroseconds;
extern ULONG KiServiceLimit;
extern LIST_ENTRY KeBugcheckCallbackListHead, KeBugcheckReasonCallbackListHead;
extern KSPIN_LOCK BugCheckCallbackLock;
extern KDPC KiTimerExpireDpc;
extern KTIMER_TABLE_ENTRY KiTimerTableListHead[TIMER_TABLE_SIZE];
extern FAST_MUTEX KiGenericCallDpcMutex;
extern LIST_ENTRY KiProfileListHead, KiProfileSourceListHead;
extern KSPIN_LOCK KiProfileLock;
extern LIST_ENTRY KiProcessListHead;
extern LIST_ENTRY KiProcessInSwapListHead, KiProcessOutSwapListHead;
extern LIST_ENTRY KiStackInSwapListHead;
extern KEVENT KiSwapEvent;
extern PKPRCB KiProcessorBlock[];
extern ULONG KiMask32Array[MAXIMUM_PRIORITY];
extern ULONG_PTR KiIdleSummary;
extern PVOID KeUserApcDispatcher;
extern PVOID KeUserCallbackDispatcher;
extern PVOID KeUserExceptionDispatcher;
extern PVOID KeRaiseUserExceptionDispatcher;
extern ULONG KeTimeIncrement;
extern ULONG KeTimeAdjustment;
extern BOOLEAN KiTimeAdjustmentEnabled;
extern LONG KiTickOffset;
extern ULONG_PTR KiBugCheckData[5];
extern ULONG KiFreezeFlag;
extern ULONG KiDPCTimeout;
extern PGDI_BATCHFLUSH_ROUTINE KeGdiFlushUserBatch;
extern ULONGLONG BootCycles, BootCyclesEnd;
extern ULONG ProcessCount;
extern VOID __cdecl KiInterruptTemplate(VOID);

/* MACROS *************************************************************************/

#define AFFINITY_MASK(Id) KiMask32Array[Id]
#define PRIORITY_MASK(Id) KiMask32Array[Id]

/* Tells us if the Timer or Event is a Syncronization or Notification Object */
#define TIMER_OR_EVENT_TYPE 0x7L

/* One of the Reserved Wait Blocks, this one is for the Thread's Timer */
#define TIMER_WAIT_BLOCK 0x3L

#define KTS_SYSCALL_BIT (((KTRAP_STATE_BITS) { { .SystemCall   = TRUE } }).Bits)
#define KTS_PM_BIT      (((KTRAP_STATE_BITS) { { .PreviousMode   = TRUE } }).Bits)
#define KTS_SEG_BIT     (((KTRAP_STATE_BITS) { { .Segments  = TRUE } }).Bits)
#define KTS_VOL_BIT     (((KTRAP_STATE_BITS) { { .Volatiles = TRUE } }).Bits)
#define KTS_FULL_BIT    (((KTRAP_STATE_BITS) { { .Full = TRUE } }).Bits)

/* INTERNAL KERNEL FUNCTIONS ************************************************/

VOID
NTAPI
CPUID(
    IN ULONG InfoType,
    OUT PULONG CpuInfoEax,
    OUT PULONG CpuInfoEbx,
    OUT PULONG CpuInfoEcx,
    OUT PULONG CpuInfoEdx
);

LONGLONG
FASTCALL
RDMSR(
    IN ULONG Register
);

VOID
NTAPI
WRMSR(
    IN ULONG Register,
    IN LONGLONG Value
);

/* Finds a new thread to run */
LONG_PTR
FASTCALL
KiSwapThread(
    IN PKTHREAD Thread,
    IN PKPRCB Prcb
);

VOID
NTAPI
KeReadyThread(
    IN PKTHREAD Thread
);

BOOLEAN
NTAPI
KeSetDisableBoostThread(
    IN OUT PKTHREAD Thread,
    IN BOOLEAN Disable
);

BOOLEAN
NTAPI
KeSetDisableBoostProcess(
    IN PKPROCESS Process,
    IN BOOLEAN Disable
);

BOOLEAN
NTAPI
KeSetAutoAlignmentProcess(
    IN PKPROCESS Process,
    IN BOOLEAN Enable
);

KAFFINITY
NTAPI
KeSetAffinityProcess(
    IN PKPROCESS Process,
    IN KAFFINITY Affinity
);

VOID
NTAPI
KeBoostPriorityThread(
    IN PKTHREAD Thread,
    IN KPRIORITY Increment
);

VOID
NTAPI
KeBalanceSetManager(IN PVOID Context);

VOID
NTAPI
KiReadyThread(IN PKTHREAD Thread);

ULONG
NTAPI
KeSuspendThread(PKTHREAD Thread);

BOOLEAN
NTAPI
KeReadStateThread(IN PKTHREAD Thread);

BOOLEAN
FASTCALL
KiSwapContext(
    IN KIRQL WaitIrql,
    IN PKTHREAD CurrentThread
);

VOID
NTAPI
KiAdjustQuantumThread(IN PKTHREAD Thread);

VOID
FASTCALL
KiExitDispatcher(KIRQL OldIrql);

VOID
FASTCALL
KiDeferredReadyThread(IN PKTHREAD Thread);

PKTHREAD
FASTCALL
KiIdleSchedule(
    IN PKPRCB Prcb
);

VOID
FASTCALL
KiProcessDeferredReadyList(
    IN PKPRCB Prcb
);

KAFFINITY
FASTCALL
KiSetAffinityThread(
    IN PKTHREAD Thread,
    IN KAFFINITY Affinity
);

PKTHREAD
FASTCALL
KiSelectNextThread(
    IN PKPRCB Prcb
);

BOOLEAN
FASTCALL
KiInsertTimerTable(
    IN PKTIMER Timer,
    IN ULONG Hand
);

VOID
FASTCALL
KiTimerListExpire(
    IN PLIST_ENTRY ExpiredListHead,
    IN KIRQL OldIrql
);

BOOLEAN
FASTCALL
KiInsertTreeTimer(
    IN PKTIMER Timer,
    IN LARGE_INTEGER Interval
);

VOID
FASTCALL
KiCompleteTimer(
    IN PKTIMER Timer,
    IN PKSPIN_LOCK_QUEUE LockQueue
);

/* gmutex.c ********************************************************************/

VOID
FASTCALL
KiAcquireGuardedMutex(
    IN OUT PKGUARDED_MUTEX GuardedMutex
);

VOID
FASTCALL
KiAcquireFastMutex(
    IN PFAST_MUTEX FastMutex
);

/* gate.c **********************************************************************/

VOID
FASTCALL
KeInitializeGate(PKGATE Gate);

VOID
FASTCALL
KeSignalGateBoostPriority(PKGATE Gate);

VOID
FASTCALL
KeWaitForGate(
    PKGATE Gate,
    KWAIT_REASON WaitReason,
    KPROCESSOR_MODE WaitMode
);

/* ipi.c ********************************************************************/

VOID
FASTCALL
KiIpiSend(
    KAFFINITY TargetSet,
    ULONG IpiRequest
);

VOID
NTAPI
KiIpiSendPacket(
    IN KAFFINITY TargetProcessors,
    IN PKIPI_WORKER WorkerFunction,
    IN PKIPI_BROADCAST_WORKER BroadcastFunction,
    IN ULONG_PTR Context,
    IN PULONG Count
);

VOID
FASTCALL
KiIpiSignalPacketDone(
    IN PKIPI_CONTEXT PacketContext
);

VOID
FASTCALL
KiIpiSignalPacketDoneAndStall(
    IN PKIPI_CONTEXT PacketContext,
    IN volatile PULONG ReverseStall
);

/* next file ***************************************************************/

UCHAR
NTAPI
KeFindNextRightSetAffinity(
    IN UCHAR Number,
    IN ULONG Set
);

VOID
NTAPI
DbgBreakPointNoBugCheck(VOID);

VOID
NTAPI
KeInitializeProfile(
    struct _KPROFILE* Profile,
    struct _KPROCESS* Process,
    PVOID ImageBase,
    SIZE_T ImageSize,
    ULONG BucketSize,
    KPROFILE_SOURCE ProfileSource,
    KAFFINITY Affinity
);

BOOLEAN
NTAPI
KeStartProfile(
    struct _KPROFILE* Profile,
    PVOID Buffer
);

BOOLEAN
NTAPI
KeStopProfile(struct _KPROFILE* Profile);

ULONG
NTAPI
KeQueryIntervalProfile(KPROFILE_SOURCE ProfileSource);

VOID
NTAPI
KeSetIntervalProfile(
    KPROFILE_SOURCE ProfileSource,
    ULONG Interval
);

VOID
NTAPI
KeProfileInterrupt(
    PKTRAP_FRAME TrapFrame
);

VOID
NTAPI
KeProfileInterruptWithSource(
    IN PKTRAP_FRAME TrapFrame,
    IN KPROFILE_SOURCE Source
);

VOID
NTAPI
KeUpdateRunTime(
    PKTRAP_FRAME TrapFrame,
    KIRQL Irql
);

VOID
NTAPI
KiExpireTimers(
    PKDPC Dpc,
    PVOID DeferredContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2
);

VOID
NTAPI
KeInitializeThread(
    IN PKPROCESS Process,
    IN OUT PKTHREAD Thread,
    IN PKSYSTEM_ROUTINE SystemRoutine,
    IN PKSTART_ROUTINE StartRoutine,
    IN PVOID StartContext,
    IN PCONTEXT Context,
    IN PVOID Teb,
    IN PVOID KernelStack
);

VOID
NTAPI
KeUninitThread(
    IN PKTHREAD Thread
);

NTSTATUS
NTAPI
KeInitThread(
    IN OUT PKTHREAD Thread,
    IN PVOID KernelStack,
    IN PKSYSTEM_ROUTINE SystemRoutine,
    IN PKSTART_ROUTINE StartRoutine,
    IN PVOID StartContext,
    IN PCONTEXT Context,
    IN PVOID Teb,
    IN PKPROCESS Process
);

VOID
NTAPI
KiInitializeContextThread(
    PKTHREAD Thread,
    PKSYSTEM_ROUTINE SystemRoutine,
    PKSTART_ROUTINE StartRoutine,
    PVOID StartContext,
    PCONTEXT Context
);

VOID
NTAPI
KeStartThread(
    IN OUT PKTHREAD Thread
);

BOOLEAN
NTAPI
KeAlertThread(
    IN PKTHREAD Thread,
    IN KPROCESSOR_MODE AlertMode
);

ULONG
NTAPI
KeAlertResumeThread(
    IN PKTHREAD Thread
);

ULONG
NTAPI
KeResumeThread(
    IN PKTHREAD Thread
);

PVOID
NTAPI
KeSwitchKernelStack(
    IN PVOID StackBase,
    IN PVOID StackLimit
);

VOID
NTAPI
KeRundownThread(VOID);

NTSTATUS
NTAPI
KeReleaseThread(PKTHREAD Thread);

VOID
NTAPI
KiSuspendRundown(
    IN PKAPC Apc
);

VOID
NTAPI
KiSuspendNop(
    IN PKAPC Apc,
    IN PKNORMAL_ROUTINE *NormalRoutine,
    IN PVOID *NormalContext,
    IN PVOID *SystemArgument1,
    IN PVOID *SystemArgument2
);

VOID
NTAPI
KiSuspendThread(
    IN PVOID NormalContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
);

LONG
NTAPI
KeQueryBasePriorityThread(IN PKTHREAD Thread);

VOID
FASTCALL
KiSetPriorityThread(
    IN PKTHREAD Thread,
    IN KPRIORITY Priority
);

VOID
FASTCALL
KiUnlinkThread(
    IN PKTHREAD Thread,
    IN LONG_PTR WaitStatus
);

VOID
NTAPI
KeDumpStackFrames(PULONG Frame);

BOOLEAN
NTAPI
KiTestAlert(VOID);

VOID
FASTCALL
KiUnwaitThread(
    IN PKTHREAD Thread,
    IN LONG_PTR WaitStatus,
    IN KPRIORITY Increment
);

VOID
NTAPI
KeInitializeProcess(
    struct _KPROCESS *Process,
    KPRIORITY Priority,
    KAFFINITY Affinity,
    PULONG_PTR DirectoryTableBase,
    IN BOOLEAN Enable
);

VOID
NTAPI
KeSetQuantumProcess(
    IN PKPROCESS Process,
    IN UCHAR Quantum
);

KPRIORITY
NTAPI
KeSetPriorityAndQuantumProcess(
    IN PKPROCESS Process,
    IN KPRIORITY Priority,
    IN UCHAR Quantum OPTIONAL
);

ULONG
NTAPI
KeForceResumeThread(IN PKTHREAD Thread);

VOID
NTAPI
KeThawAllThreads(
    VOID
);

VOID
NTAPI
KeFreezeAllThreads(
    VOID
);

BOOLEAN
NTAPI
KeDisableThreadApcQueueing(IN PKTHREAD Thread);

VOID
FASTCALL
KiWaitTest(
    PVOID Object,
    KPRIORITY Increment
);

VOID
NTAPI
KeContextToTrapFrame(
    PCONTEXT Context,
    PKEXCEPTION_FRAME ExeptionFrame,
    PKTRAP_FRAME TrapFrame,
    ULONG ContextFlags,
    KPROCESSOR_MODE PreviousMode
);

VOID
NTAPI
Ke386SetIOPL(VOID);

VOID
NTAPI
KiCheckForKernelApcDelivery(VOID);

LONG
NTAPI
KiInsertQueue(
    IN PKQUEUE Queue,
    IN PLIST_ENTRY Entry,
    BOOLEAN Head
);

VOID
NTAPI
KiTimerExpiration(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
);

ULONG
NTAPI
KeSetProcess(
    struct _KPROCESS* Process,
    KPRIORITY Increment,
    BOOLEAN InWait
);

VOID
NTAPI
KeInitializeEventPair(PKEVENT_PAIR EventPair);

VOID
NTAPI
KiInitializeUserApc(
    IN PKEXCEPTION_FRAME Reserved,
    IN PKTRAP_FRAME TrapFrame,
    IN PKNORMAL_ROUTINE NormalRoutine,
    IN PVOID NormalContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
);

PLIST_ENTRY
NTAPI
KeFlushQueueApc(
    IN PKTHREAD Thread,
    IN KPROCESSOR_MODE PreviousMode
);

VOID
NTAPI
KiAttachProcess(
    struct _KTHREAD *Thread,
    struct _KPROCESS *Process,
    PKLOCK_QUEUE_HANDLE ApcLock,
    struct _KAPC_STATE *SavedApcState
);

VOID
NTAPI
KiSwapProcess(
    struct _KPROCESS *NewProcess,
    struct _KPROCESS *OldProcess
);

BOOLEAN
NTAPI
KeTestAlertThread(IN KPROCESSOR_MODE AlertMode);

BOOLEAN
NTAPI
KeRemoveQueueApc(PKAPC Apc);

VOID
FASTCALL
KiActivateWaiterQueue(IN PKQUEUE Queue);

ULONG
NTAPI
KeQueryRuntimeProcess(IN PKPROCESS Process,
                      OUT PULONG UserTime);

/* INITIALIZATION FUNCTIONS *************************************************/

BOOLEAN
NTAPI
KeInitSystem(VOID);

VOID
NTAPI
KeInitExceptions(VOID);

VOID
NTAPI
KeInitInterrupts(VOID);

VOID
NTAPI
KiInitializeBugCheck(VOID);

VOID
NTAPI
KiSystemStartup(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
);

BOOLEAN
NTAPI
KiDeliverUserApc(PKTRAP_FRAME TrapFrame);

VOID
NTAPI
KiMoveApcState(
    PKAPC_STATE OldState,
    PKAPC_STATE NewState
);

VOID
NTAPI
KiAddProfileEvent(
    KPROFILE_SOURCE Source,
    ULONG Pc
);

VOID
NTAPI
KiDispatchException(
    PEXCEPTION_RECORD ExceptionRecord,
    PKEXCEPTION_FRAME ExceptionFrame,
    PKTRAP_FRAME Tf,
    KPROCESSOR_MODE PreviousMode,
    BOOLEAN SearchFrames
);

VOID
NTAPI
KeTrapFrameToContext(
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN OUT PCONTEXT Context
);

DECLSPEC_NORETURN
VOID
NTAPI
KeBugCheckWithTf(
    ULONG BugCheckCode,
    ULONG_PTR BugCheckParameter1,
    ULONG_PTR BugCheckParameter2,
    ULONG_PTR BugCheckParameter3,
    ULONG_PTR BugCheckParameter4,
    PKTRAP_FRAME Tf
);

BOOLEAN
NTAPI
KiHandleNmi(VOID);

VOID
NTAPI
KeFlushCurrentTb(VOID);

BOOLEAN
NTAPI
KeInvalidateAllCaches(VOID);

VOID
FASTCALL
KeZeroPages(IN PVOID Address,
            IN ULONG Size);

BOOLEAN
FASTCALL
KeInvalidAccessAllowed(IN PVOID TrapInformation OPTIONAL);

VOID
NTAPI
KeRosDumpStackFrames(
    PULONG_PTR Frame,
    ULONG FrameCount
);

VOID
NTAPI
KeSetSystemTime(
    IN PLARGE_INTEGER NewSystemTime,
    OUT PLARGE_INTEGER OldSystemTime,
    IN BOOLEAN FixInterruptTime,
    IN PLARGE_INTEGER HalTime
);

ULONG
NTAPI
KeV86Exception(
    ULONG ExceptionNr,
    PKTRAP_FRAME Tf,
    ULONG address
);

VOID
NTAPI
KiStartUnexpectedRange(
    VOID
);

VOID
NTAPI
KiEndUnexpectedRange(
    VOID
);

NTSTATUS
NTAPI
KiRaiseException(
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT Context,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PKTRAP_FRAME TrapFrame,
    IN BOOLEAN SearchFrames
);

NTSTATUS
NTAPI
KiContinue(
    IN PCONTEXT Context,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PKTRAP_FRAME TrapFrame
);

DECLSPEC_NORETURN
VOID
FASTCALL
KiServiceExit(
    IN PKTRAP_FRAME TrapFrame,
    IN NTSTATUS Status
);

DECLSPEC_NORETURN
VOID
FASTCALL
KiServiceExit2(
    IN PKTRAP_FRAME TrapFrame
);

#ifndef _M_AMD64
VOID
FASTCALL
KiInterruptDispatch(
    IN PKTRAP_FRAME TrapFrame,
    IN PKINTERRUPT Interrupt
);
#endif

VOID
FASTCALL
KiChainedDispatch(
    IN PKTRAP_FRAME TrapFrame,
    IN PKINTERRUPT Interrupt
);

VOID
NTAPI
KiInitializeMachineType(
    VOID
);

VOID
NTAPI
KiSetupStackAndInitializeKernel(
    IN PKPROCESS InitProcess,
    IN PKTHREAD InitThread,
    IN PVOID IdleStack,
    IN PKPRCB Prcb,
    IN CCHAR Number,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
);

VOID
NTAPI
KiInitSpinLocks(
    IN PKPRCB Prcb,
    IN CCHAR Number
);

LARGE_INTEGER
NTAPI
KiComputeReciprocal(
    IN LONG Divisor,
    OUT PUCHAR Shift
);

VOID
NTAPI
KiInitSystem(
    VOID
);

VOID
FASTCALL
KiInsertQueueApc(
    IN PKAPC Apc,
    IN KPRIORITY PriorityBoost
);

NTSTATUS
NTAPI
KiCallUserMode(
    IN PVOID *OutputBuffer,
    IN PULONG OutputLength
);

DECLSPEC_NORETURN
VOID
FASTCALL
KiCallbackReturn(
    IN PVOID Stack,
    IN NTSTATUS Status
);

VOID
NTAPI
KiInitMachineDependent(VOID);

BOOLEAN
NTAPI
KeFreezeExecution(IN PKTRAP_FRAME TrapFrame,
                  IN PKEXCEPTION_FRAME ExceptionFrame);

VOID
NTAPI
KeThawExecution(IN BOOLEAN Enable);

VOID
FASTCALL
KeAcquireQueuedSpinLockAtDpcLevel(
    IN OUT PKSPIN_LOCK_QUEUE LockQueue
);

VOID
FASTCALL
KeReleaseQueuedSpinLockFromDpcLevel(
    IN OUT PKSPIN_LOCK_QUEUE LockQueue
);

VOID
NTAPI
KiRestoreProcessorControlState(
    IN PKPROCESSOR_STATE ProcessorState
);

VOID
NTAPI
KiSaveProcessorControlState(
    OUT PKPROCESSOR_STATE ProcessorState
);

VOID
NTAPI
KiSaveProcessorState(
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame
);

VOID
FASTCALL
KiRetireDpcList(
    IN PKPRCB Prcb
);

VOID
NTAPI
KiQuantumEnd(
    VOID
);

VOID
FASTCALL
KiIdleLoop(
    VOID
);

DECLSPEC_NORETURN
VOID
FASTCALL
KiSystemFatalException(
    IN ULONG ExceptionCode,
    IN PKTRAP_FRAME TrapFrame
);

PVOID
NTAPI
KiPcToFileHeader(IN PVOID Eip,
                 OUT PLDR_DATA_TABLE_ENTRY *LdrEntry,
                 IN BOOLEAN DriversOnly,
                 OUT PBOOLEAN InKernel);

PVOID
NTAPI
KiRosPcToUserFileHeader(IN PVOID Eip,
                        OUT PLDR_DATA_TABLE_ENTRY *LdrEntry);

PCHAR
NTAPI
KeBugCheckUnicodeToAnsi(
    IN PUNICODE_STRING Unicode,
    OUT PCHAR Ansi,
    IN ULONG Length
);

#include "ke_x.h"
