#ifndef __NTOSKRNL_INCLUDE_INTERNAL_KE_H
#define __NTOSKRNL_INCLUDE_INTERNAL_KE_H

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

/* Cached modules from the loader block */
typedef enum _CACHED_MODULE_TYPE
{
    AnsiCodepage,
    OemCodepage,
    UnicodeCasemap,
    SystemRegistry,
    HardwareRegistry,
    MaximumCachedModuleType,
} CACHED_MODULE_TYPE, *PCACHED_MODULE_TYPE;
extern PLOADER_MODULE CachedModules[MaximumCachedModuleType];

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

typedef struct _KTIMER_TABLE_ENTRY
{
    LIST_ENTRY Entry;
    ULARGE_INTEGER Time;
} KTIMER_TABLE_ENTRY, *PKTIMER_TABLE_ENTRY;

typedef PCHAR
(NTAPI *PKE_BUGCHECK_UNICODE_TO_ANSI)(
    IN PUNICODE_STRING Unicode,
    IN PCHAR Ansi,
    IN ULONG Length
);

struct _KIRQ_TRAPFRAME;
struct _KPCR;
struct _KPRCB;
struct _KEXCEPTION_FRAME;

extern PVOID KeUserApcDispatcher;
extern PVOID KeUserCallbackDispatcher;
extern PVOID KeUserExceptionDispatcher;
extern PVOID KeRaiseUserExceptionDispatcher;
extern LARGE_INTEGER SystemBootTime;
extern ULONG_PTR KERNEL_BASE;
extern ULONG KeI386NpxPresent;
extern ULONG KeI386XMMIPresent;
extern ULONG KeI386FxsrPresent;
extern ULONG KeI386CpuType;
extern ULONG KeI386CpuStep;
extern ULONG KeProcessorArchitecture;
extern ULONG KeProcessorLevel;
extern ULONG KeProcessorRevision;
extern ULONG KeFeatureBits;
extern ULONG Ke386GlobalPagesEnabled;
extern KNODE KiNode0;
extern PKNODE KeNodeBlock[1];
extern UCHAR KeNumberNodes;
extern UCHAR KeProcessNodeSeed;
extern ETHREAD KiInitialThread;
extern EPROCESS KiInitialProcess;
extern ULONG KiInterruptTemplate[KINTERRUPT_DISPATCH_CODES];
extern PULONG KiInterruptTemplateObject;
extern PULONG KiInterruptTemplateDispatch;
extern PULONG KiInterruptTemplate2ndDispatch;
extern ULONG KiUnexpectedEntrySize;
extern PVOID Ki386IopmSaveArea;
extern ULONG KeI386EFlagsAndMaskV86;
extern ULONG KeI386EFlagsOrMaskV86;
extern BOOLEAN KeI386VirtualIntExtensions;
extern KIDTENTRY KiIdt[];
extern KGDTENTRY KiBootGdt[];
extern KDESCRIPTOR KiGdtDescriptor;
extern KDESCRIPTOR KiIdtDescriptor;
extern KTSS KiBootTss;
extern UCHAR P0BootStack[];
extern UCHAR KiDoubleFaultStack[];
extern FAST_MUTEX KernelAddressSpaceLock;
extern ULONG KiMaximumDpcQueueDepth;
extern ULONG KiMinimumDpcRate;
extern ULONG KiAdjustDpcThreshold;
extern ULONG KiIdealDpcRate;
extern LARGE_INTEGER KiTimeIncrementReciprocal;
extern UCHAR KiTimeIncrementShiftCount;
extern LIST_ENTRY BugcheckCallbackListHead, BugcheckReasonCallbackListHead;
extern KSPIN_LOCK BugCheckCallbackLock;
extern KDPC KiExpireTimerDpc;
extern KTIMER_TABLE_ENTRY KiTimerTableListHead[TIMER_TABLE_SIZE];
extern LIST_ENTRY KiTimerListHead;
extern KMUTEX KiGenericCallDpcMutex;
extern LIST_ENTRY KiProfileListHead, KiProfileSourceListHead;
extern KSPIN_LOCK KiProfileLock;
extern LIST_ENTRY KiProcessListHead;
extern LIST_ENTRY KiProcessInSwapListHead, KiProcessOutSwapListHead;
extern LIST_ENTRY KiStackInSwapListHead;
extern KEVENT KiSwapEvent;
extern PKPRCB KiProcessorBlock[];
extern ULONG KiMask32Array[MAXIMUM_PRIORITY];
extern ULONG KiIdleSummary;
extern VOID KiTrap8(VOID);
extern VOID KiTrap2(VOID);

/* MACROS *************************************************************************/

#define AFFINITY_MASK(Id) KiMask32Array[Id]
#define PRIORITY_MASK(Id) KiMask32Array[Id]

/* The following macro initializes a dispatcher object's header */
#define KeInitializeDispatcherHeader(Header, t, s, State)                   \
{                                                                           \
    (Header)->Type = t;                                                     \
    (Header)->Absolute = 0;                                                 \
    (Header)->Inserted = 0;                                                 \
    (Header)->Size = s;                                                     \
    (Header)->SignalState = State;                                          \
    InitializeListHead(&((Header)->WaitListHead));                          \
}

#define KEBUGCHECKWITHTF(a,b,c,d,e,f) \
    DbgPrint("KeBugCheckWithTf at %s:%i\n",__FILE__,__LINE__), \
             KeBugCheckWithTf(a,b,c,d,e,f)

/* Tells us if the Timer or Event is a Syncronization or Notification Object */
#define TIMER_OR_EVENT_TYPE 0x7L

/* One of the Reserved Wait Blocks, this one is for the Thread's Timer */
#define TIMER_WAIT_BLOCK 0x3L

/* IOPM Definitions */
#define IO_ACCESS_MAP_NONE 0
#define IOPM_OFFSET FIELD_OFFSET(KTSS, IoMaps[0].IoMap)
#define KiComputeIopmOffset(MapNumber)              \
    (MapNumber == IO_ACCESS_MAP_NONE) ?             \
        (USHORT)(sizeof(KTSS)) :                    \
        (USHORT)(FIELD_OFFSET(KTSS, IoMaps[MapNumber-1].IoMap))

#define SIZE_OF_FX_REGISTERS 32

/* INTERNAL KERNEL FUNCTIONS ************************************************/

/* Readies a Thread for Execution. */
BOOLEAN
NTAPI
KiDispatchThreadNoLock(ULONG NewThreadStatus);

/* Readies a Thread for Execution. */
VOID
NTAPI
KiDispatchThread(ULONG NewThreadStatus);

/* Finds a new thread to run */
NTSTATUS
FASTCALL
KiSwapThread(
    IN PKTHREAD Thread,
    IN PKPRCB Prcb
);

VOID
NTAPI
KiReadyThread(IN PKTHREAD Thread);

NTSTATUS
NTAPI
KeSuspendThread(PKTHREAD Thread);

BOOLEAN
FASTCALL
KiSwapContext(
    IN PKTHREAD CurrentThread,
    IN PKTHREAD NewThread
);

VOID
NTAPI
KiAdjustQuantumThread(IN PKTHREAD Thread);

VOID
FASTCALL
KiExitDispatcher(KIRQL OldIrql);

VOID
NTAPI
KiDeferredReadyThread(IN PKTHREAD Thread);

KAFFINITY
NTAPI
KiSetAffinityThread(
    IN PKTHREAD Thread,
    IN KAFFINITY Affinity,
    IN PBOOLEAN Released // hack
);

PKTHREAD
NTAPI
KiSelectNextThread(
    IN PKPRCB Prcb
);

/* gmutex.c ********************************************************************/

VOID
FASTCALL
KiAcquireGuardedMutexContented(PKGUARDED_MUTEX GuardedMutex);

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
NTAPI
KiIpiSendRequest(
    KAFFINITY TargetSet,
    ULONG IpiRequest
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
    ULONG ImageSize,
    ULONG BucketSize,
    KPROFILE_SOURCE ProfileSource,
    KAFFINITY Affinity
);

VOID
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

BOOLEAN
NTAPI
KiRosPrintAddress(PVOID Address);

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
NTAPI
KiSetPriorityThread(
    IN PKTHREAD Thread,
    IN KPRIORITY Priority,
    IN PBOOLEAN Released // hack
);

BOOLEAN
NTAPI
KiDispatcherObjectWake(
    DISPATCHER_HEADER* hdr,
    KPRIORITY increment
);

VOID
NTAPI
KeExpireTimers(
    PKDPC Apc,
    PVOID Arg1,
    PVOID Arg2,
    PVOID Arg3
);

VOID
NTAPI
KeDumpStackFrames(PULONG Frame);

BOOLEAN
NTAPI
KiTestAlert(VOID);

VOID
FASTCALL
KiAbortWaitThread(
    IN PKTHREAD Thread,
    IN NTSTATUS WaitStatus,
    IN KPRIORITY Increment
);

VOID
NTAPI
KeInitializeProcess(
    struct _KPROCESS *Process,
    KPRIORITY Priority,
    KAFFINITY Affinity,
    PLARGE_INTEGER DirectoryTableBase,
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

BOOLEAN
NTAPI
KeDisableThreadApcQueueing(IN PKTHREAD Thread);

BOOLEAN
NTAPI
KiInsertTimer(
    PKTIMER Timer,
    LARGE_INTEGER DueTime
);

VOID
FASTCALL
KiWaitTest(
    PVOID Object,
    KPRIORITY Increment
);

PULONG 
NTAPI
KeGetStackTopThread(struct _ETHREAD* Thread);

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
KiCheckForKernelApcDelivery(VOID);

LONG
NTAPI
KiInsertQueue(
    IN PKQUEUE Queue,
    IN PLIST_ENTRY Entry,
    BOOLEAN Head
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
KiWakeQueue(IN PKQUEUE Queue);

/* INITIALIZATION FUNCTIONS *************************************************/

VOID
NTAPI
KeInitExceptions(VOID);

VOID
NTAPI
KeInitInterrupts(VOID);

VOID
NTAPI
KeInitTimer(VOID);

VOID
NTAPI
KeInitDispatcher(VOID);

VOID
NTAPI
KiInitializeSystemClock(VOID);

VOID
NTAPI
KiInitializeBugCheck(VOID);

VOID
NTAPI
Phase1Initialization(PVOID Context);

VOID
NTAPI
KiSystemStartup(
    IN PROS_LOADER_PARAMETER_BLOCK LoaderBlock
);

VOID
NTAPI
KeInit2(VOID);

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

VOID
NTAPI
KeApplicationProcessorInit(VOID);

VOID
NTAPI
KePrepareForApplicationProcessorInit(ULONG id);

ULONG
NTAPI
KiUserTrapHandler(
    PKTRAP_FRAME Tf,
    ULONG ExceptionNr,
    PVOID Cr2
);

VOID
NTAPI
KePushAndStackSwitchAndSysRet(
    ULONG Push,
    PVOID NewStack
);

VOID
NTAPI
KeStackSwitchAndRet(PVOID NewStack);

VOID
NTAPI
KeBugCheckWithTf(
    ULONG BugCheckCode,
    ULONG BugCheckParameter1,
    ULONG BugCheckParameter2,
    ULONG BugCheckParameter3,
    ULONG BugCheckParameter4,
    PKTRAP_FRAME Tf
);

VOID
NTAPI
KeFlushCurrentTb(VOID);

VOID
NTAPI
KeRosDumpStackFrames(
    PULONG Frame,
    ULONG FrameCount
);

VOID
NTAPI
KiSetSystemTime(PLARGE_INTEGER NewSystemTime);

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

VOID
NTAPI
KiInterruptDispatch(
    VOID
);

VOID
NTAPI
KiChainedDispatch(
    VOID
);

VOID
NTAPI
Ki386AdjustEsp0(
    IN PKTRAP_FRAME TrapFrame
);

VOID
NTAPI
Ki386SetupAndExitToV86Mode(
    OUT PTEB VdmTeb
);

VOID
NTAPI
KeI386VdmInitialize(
    VOID
);

VOID
NTAPI
KiFlushNPXState(
    IN FLOATING_SAVE_AREA *SaveArea
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

PULONG
NTAPI
KiGetUserModeStackAddress(
    VOID
);

#include "ke_x.h"

#endif /* __NTOSKRNL_INCLUDE_INTERNAL_KE_H */
