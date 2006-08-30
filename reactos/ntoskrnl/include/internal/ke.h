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
extern PKNODE KeNodeBlock[1];
extern UCHAR KeNumberNodes;
extern UCHAR KeProcessNodeSeed;
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

/* MACROS *************************************************************************/

/*
 * On UP machines, we don't actually have a spinlock, we merely raise
 * IRQL to DPC level.
 */
#ifdef CONFIG_SMP
#define KeInitializeDispatcher() KeInitializeSpinLock(&DispatcherDatabaseLock);
#define KeAcquireDispatcherDatabaseLock() KfAcquireSpinLock(&DispatcherDatabaseLock);
#define KeAcquireDispatcherDatabaseLockAtDpcLevel() \
    KeAcquireSpinLockAtDpcLevel (&DispatcherDatabaseLock);
#define KeReleaseDispatcherDatabaseLockFromDpcLevel() \
    KeReleaseSpinLockFromDpcLevel(&DispatcherDatabaseLock);
#define KeReleaseDispatcherDatabaseLock(OldIrql) \
    KiExitDispatcher(OldIrql);
#else
#define KeInitializeDispatcher()
#define KeAcquireDispatcherDatabaseLock() KeRaiseIrqlToDpcLevel();
#define KeReleaseDispatcherDatabaseLock(OldIrql) KiExitDispatcher(OldIrql);
#define KeAcquireDispatcherDatabaseLockAtDpcLevel()
#define KeReleaseDispatcherDatabaseLockFromDpcLevel()
#endif

#define AFFINITY_MASK(Id) KiMask32Array[Id]

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

extern KSPIN_LOCK DispatcherDatabaseLock;

#define KeEnterCriticalRegion()                                             \
{                                                                           \
    PKTHREAD _Thread = KeGetCurrentThread();                                \
    if (_Thread) _Thread->KernelApcDisable--;                               \
}

#define KeLeaveCriticalRegion()                                             \
{                                                                           \
    PKTHREAD _Thread = KeGetCurrentThread();                                \
    if((_Thread) && (++_Thread->KernelApcDisable == 0))                     \
    {                                                                       \
        if (!IsListEmpty(&_Thread->ApcState.ApcListHead[KernelMode]) &&     \
            (_Thread->SpecialApcDisable == 0))                              \
        {                                                                   \
            KiCheckForKernelApcDelivery();                                  \
        }                                                                   \
    }                                                                       \
}

#define KEBUGCHECKWITHTF(a,b,c,d,e,f) \
    DbgPrint("KeBugCheckWithTf at %s:%i\n",__FILE__,__LINE__), \
             KeBugCheckWithTf(a,b,c,d,e,f)

/* Tells us if the Timer or Event is a Syncronization or Notification Object */
#define TIMER_OR_EVENT_TYPE 0x7L

/* One of the Reserved Wait Blocks, this one is for the Thread's Timer */
#define TIMER_WAIT_BLOCK 0x3L

#define IOPM_OFFSET FIELD_OFFSET(KTSS, IoMaps[0].IoMap)

#define SIZE_OF_FX_REGISTERS 32

/* INTERNAL KERNEL FUNCTIONS ************************************************/

/* threadsch.c ********************************************************************/

/* Thread Scheduler Functions */

/* Readies a Thread for Execution. */
BOOLEAN
STDCALL
KiDispatchThreadNoLock(ULONG NewThreadStatus);

/* Readies a Thread for Execution. */
VOID
STDCALL
KiDispatchThread(ULONG NewThreadStatus);

/* Finds a new thread to run */
NTSTATUS
NTAPI
KiSwapThread(
    VOID
);

VOID
NTAPI
KiReadyThread(IN PKTHREAD Thread);

NTSTATUS
STDCALL
KeSuspendThread(PKTHREAD Thread);

NTSTATUS
FASTCALL
KiSwapContext(
    IN PKTHREAD CurrentThread,
    IN PKTHREAD NewThread
);

VOID
STDCALL
KiAdjustQuantumThread(IN PKTHREAD Thread);

VOID
FASTCALL
KiExitDispatcher(KIRQL OldIrql);

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
STDCALL
DbgBreakPointNoBugCheck(VOID);

VOID
STDCALL
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
STDCALL
KeStartProfile(
    struct _KPROFILE* Profile,
    PVOID Buffer
);

BOOLEAN
STDCALL
KeStopProfile(struct _KPROFILE* Profile);

ULONG
STDCALL
KeQueryIntervalProfile(KPROFILE_SOURCE ProfileSource);

VOID
STDCALL
KeSetIntervalProfile(
    KPROFILE_SOURCE ProfileSource,
    ULONG Interval
);

VOID
STDCALL
KeProfileInterrupt(
    PKTRAP_FRAME TrapFrame
);

VOID
STDCALL
KeProfileInterruptWithSource(
    IN PKTRAP_FRAME TrapFrame,
    IN KPROFILE_SOURCE Source
);

BOOLEAN
STDCALL
KiRosPrintAddress(PVOID Address);

VOID
STDCALL
KeUpdateRunTime(
    PKTRAP_FRAME TrapFrame,
    KIRQL Irql
);

VOID
STDCALL
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
STDCALL
KeRundownThread(VOID);

NTSTATUS
NTAPI
KeReleaseThread(PKTHREAD Thread);

LONG
STDCALL
KeQueryBasePriorityThread(IN PKTHREAD Thread);

VOID
STDCALL
KiSetPriorityThread(
    PKTHREAD Thread,
    KPRIORITY Priority,
    PBOOLEAN Released
);

BOOLEAN
NTAPI
KiDispatcherObjectWake(
    DISPATCHER_HEADER* hdr,
    KPRIORITY increment
);

VOID
STDCALL
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
STDCALL
KeInitializeProcess(
    struct _KPROCESS *Process,
    KPRIORITY Priority,
    KAFFINITY Affinity,
    LARGE_INTEGER DirectoryTableBase
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
STDCALL
KeForceResumeThread(IN PKTHREAD Thread);

BOOLEAN
STDCALL
KeDisableThreadApcQueueing(IN PKTHREAD Thread);

BOOLEAN
STDCALL
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
STDCALL
KeContextToTrapFrame(
    PCONTEXT Context,
    PKEXCEPTION_FRAME ExeptionFrame,
    PKTRAP_FRAME TrapFrame,
    ULONG ContextFlags,
    KPROCESSOR_MODE PreviousMode
);

VOID
STDCALL
KiDeliverApc(
    KPROCESSOR_MODE PreviousMode,
    PVOID Reserved,
    PKTRAP_FRAME TrapFrame
);

VOID
STDCALL
KiCheckForKernelApcDelivery(VOID);

LONG
STDCALL
KiInsertQueue(
    IN PKQUEUE Queue,
    IN PLIST_ENTRY Entry,
    BOOLEAN Head
);

ULONG
STDCALL
KeSetProcess(
    struct _KPROCESS* Process,
    KPRIORITY Increment,
    BOOLEAN InWait
);

VOID
STDCALL
KeInitializeEventPair(PKEVENT_PAIR EventPair);

VOID
STDCALL
KiInitializeUserApc(
    IN PKEXCEPTION_FRAME Reserved,
    IN PKTRAP_FRAME TrapFrame,
    IN PKNORMAL_ROUTINE NormalRoutine,
    IN PVOID NormalContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
);

PLIST_ENTRY
STDCALL
KeFlushQueueApc(
    IN PKTHREAD Thread,
    IN KPROCESSOR_MODE PreviousMode
);

VOID
STDCALL
KiAttachProcess(
    struct _KTHREAD *Thread,
    struct _KPROCESS *Process,
    KIRQL ApcLock,
    struct _KAPC_STATE *SavedApcState
);

VOID
STDCALL
KiSwapProcess(
    struct _KPROCESS *NewProcess,
    struct _KPROCESS *OldProcess
);

BOOLEAN
STDCALL
KeTestAlertThread(IN KPROCESSOR_MODE AlertMode);

BOOLEAN
STDCALL
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
KeInitDpc(struct _KPRCB* Prcb);

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
KeInit1(
    PCHAR CommandLine,
    PULONG LastKernelAddress
);

VOID
NTAPI
KeInit2(VOID);

BOOLEAN
NTAPI
KiDeliverUserApc(PKTRAP_FRAME TrapFrame);

VOID
STDCALL
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
STDCALL
KePushAndStackSwitchAndSysRet(
    ULONG Push,
    PVOID NewStack
);

VOID
STDCALL
KeStackSwitchAndRet(PVOID NewStack);

VOID
STDCALL
KeBugCheckWithTf(
    ULONG BugCheckCode,
    ULONG BugCheckParameter1,
    ULONG BugCheckParameter2,
    ULONG BugCheckParameter3,
    ULONG BugCheckParameter4,
    PKTRAP_FRAME Tf
);

VOID
STDCALL
KeFlushCurrentTb(VOID);

VOID
STDCALL
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

#include "ke_x.h"

#endif /* __NTOSKRNL_INCLUDE_INTERNAL_KE_H */
