/******************************************************************************
 *                              Kernel Functions                              *
 ******************************************************************************/
$if (_NTDDK_)
NTKERNELAPI
VOID
FASTCALL
KeInvalidateRangeAllCaches(
  IN PVOID BaseAddress,
  IN ULONG Length);
$endif

$if (_WDMDDK_)
NTKERNELAPI
VOID
NTAPI
KeInitializeEvent(
  OUT PRKEVENT Event,
  IN EVENT_TYPE Type,
  IN BOOLEAN State);

NTKERNELAPI
VOID
NTAPI
KeClearEvent(
  IN OUT PRKEVENT Event);
$endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)

$if (_NTDDK_)
NTKERNELAPI
VOID
NTAPI
KeSetImportanceDpc(
  IN OUT PRKDPC Dpc,
  IN KDPC_IMPORTANCE Importance);

NTKERNELAPI
LONG
NTAPI
KePulseEvent(
  IN OUT PRKEVENT Event,
  IN KPRIORITY Increment,
  IN BOOLEAN Wait);

NTKERNELAPI
LONG
NTAPI
KeSetBasePriorityThread(
  IN OUT PRKTHREAD Thread,
  IN LONG Increment);

NTKERNELAPI
VOID
NTAPI
KeEnterCriticalRegion(VOID);

NTKERNELAPI
VOID
NTAPI
KeLeaveCriticalRegion(VOID);

NTKERNELAPI
DECLSPEC_NORETURN
VOID
NTAPI
KeBugCheck(
  IN ULONG BugCheckCode);
$endif /* _NTDDK_ */

$if (_WDMDDK_)
#if defined(_NTDDK_) || defined(_NTIFS_)
NTKERNELAPI
VOID
NTAPI
ProbeForRead(
  IN CONST VOID *Address, /* CONST is added */
  IN SIZE_T Length,
  IN ULONG Alignment);
#endif /* defined(_NTDDK_) || defined(_NTIFS_) */

NTKERNELAPI
VOID
NTAPI
ProbeForWrite(
  IN PVOID Address,
  IN SIZE_T Length,
  IN ULONG Alignment);

$endif /* _WDMDDK_ */

#if defined(SINGLE_GROUP_LEGACY_API)

$if (_WDMDDK_)
NTKERNELAPI
VOID
NTAPI
KeRevertToUserAffinityThread(VOID);

NTKERNELAPI
VOID
NTAPI
KeSetSystemAffinityThread(
  IN KAFFINITY Affinity);

NTKERNELAPI
VOID
NTAPI
KeSetTargetProcessorDpc(
  IN OUT PRKDPC Dpc,
  IN CCHAR Number);

NTKERNELAPI
KAFFINITY
NTAPI
KeQueryActiveProcessors(VOID);
$endif

$if (_NTDDK_)
NTKERNELAPI
VOID
NTAPI
KeSetTargetProcessorDpc(
  IN OUT PRKDPC Dpc,
  IN CCHAR Number);

NTKERNELAPI
KAFFINITY
NTAPI
KeQueryActiveProcessors(VOID);
$endif

#endif /* defined(SINGLE_GROUP_LEGACY_API) */

$if (_WDMDDK_)
#if !defined(_M_AMD64)
NTKERNELAPI
ULONGLONG
NTAPI
KeQueryInterruptTime(VOID);

NTKERNELAPI
VOID
NTAPI
KeQuerySystemTime(
  OUT PLARGE_INTEGER CurrentTime);
#endif /* !_M_AMD64 */

#if !defined(_X86_)
NTKERNELAPI
KIRQL
NTAPI
KeAcquireSpinLockRaiseToDpc(
  IN OUT PKSPIN_LOCK SpinLock);

#define KeAcquireSpinLock(SpinLock, OldIrql) \
    *(OldIrql) = KeAcquireSpinLockRaiseToDpc(SpinLock)

NTKERNELAPI
VOID
NTAPI
KeAcquireSpinLockAtDpcLevel(
  IN OUT PKSPIN_LOCK SpinLock);

NTKERNELAPI
VOID
NTAPI
KeReleaseSpinLock(
  IN OUT PKSPIN_LOCK SpinLock,
  IN KIRQL NewIrql);

NTKERNELAPI
VOID
NTAPI
KeReleaseSpinLockFromDpcLevel(
  IN OUT PKSPIN_LOCK SpinLock);
#endif /* !_X86_ */

#if defined(_X86_) && (defined(_WDM_INCLUDED_) || defined(WIN9X_COMPAT_SPINLOCK))
NTKERNELAPI
VOID
NTAPI
KeInitializeSpinLock(
  IN PKSPIN_LOCK SpinLock);
#else
FORCEINLINE
VOID
KeInitializeSpinLock(IN PKSPIN_LOCK SpinLock)
{
  /* Clear the lock */
  *SpinLock = 0;
}
#endif

NTKERNELAPI
DECLSPEC_NORETURN
VOID
NTAPI
KeBugCheckEx(
  IN ULONG BugCheckCode,
  IN ULONG_PTR BugCheckParameter1,
  IN ULONG_PTR BugCheckParameter2,
  IN ULONG_PTR BugCheckParameter3,
  IN ULONG_PTR BugCheckParameter4);

NTKERNELAPI
BOOLEAN
NTAPI
KeCancelTimer(
  IN OUT PKTIMER);

NTKERNELAPI
NTSTATUS
NTAPI
KeDelayExecutionThread(
  IN KPROCESSOR_MODE WaitMode,
  IN BOOLEAN Alertable,
  IN PLARGE_INTEGER Interval);

NTKERNELAPI
BOOLEAN
NTAPI
KeDeregisterBugCheckCallback(
  IN OUT PKBUGCHECK_CALLBACK_RECORD CallbackRecord);

NTKERNELAPI
VOID
NTAPI
KeEnterCriticalRegion(VOID);

NTKERNELAPI
VOID
NTAPI
KeInitializeDeviceQueue(
  OUT PKDEVICE_QUEUE DeviceQueue);

NTKERNELAPI
VOID
NTAPI
KeInitializeDpc(
  OUT PRKDPC Dpc,
  IN PKDEFERRED_ROUTINE DeferredRoutine,
  IN PVOID DeferredContext OPTIONAL);

NTKERNELAPI
VOID
NTAPI
KeInitializeMutex(
  OUT PRKMUTEX Mutex,
  IN ULONG Level);

NTKERNELAPI
VOID
NTAPI
KeInitializeSemaphore(
  OUT PRKSEMAPHORE Semaphore,
  IN LONG Count,
  IN LONG Limit);

NTKERNELAPI
VOID
NTAPI
KeInitializeTimer(
  OUT PKTIMER Timer);

NTKERNELAPI
VOID
NTAPI
KeInitializeTimerEx(
  OUT PKTIMER Timer,
  IN TIMER_TYPE Type);

NTKERNELAPI
BOOLEAN
NTAPI
KeInsertByKeyDeviceQueue(
  IN OUT PKDEVICE_QUEUE DeviceQueue,
  IN OUT PKDEVICE_QUEUE_ENTRY DeviceQueueEntry,
  IN ULONG SortKey);

NTKERNELAPI
BOOLEAN
NTAPI
KeInsertDeviceQueue(
  IN OUT PKDEVICE_QUEUE DeviceQueue,
  IN OUT PKDEVICE_QUEUE_ENTRY DeviceQueueEntry);

NTKERNELAPI
BOOLEAN
NTAPI
KeInsertQueueDpc(
  IN OUT PRKDPC Dpc,
  IN PVOID SystemArgument1 OPTIONAL,
  IN PVOID SystemArgument2 OPTIONAL);

NTKERNELAPI
VOID
NTAPI
KeLeaveCriticalRegion(VOID);

NTHALAPI
LARGE_INTEGER
NTAPI
KeQueryPerformanceCounter(
  OUT PLARGE_INTEGER PerformanceFrequency OPTIONAL);

NTKERNELAPI
KPRIORITY
NTAPI
KeQueryPriorityThread(
  IN PRKTHREAD Thread);

NTKERNELAPI
ULONG
NTAPI
KeQueryTimeIncrement(VOID);

NTKERNELAPI
LONG
NTAPI
KeReadStateEvent(
  IN PRKEVENT Event);

NTKERNELAPI
LONG
NTAPI
KeReadStateMutex(
  IN PRKMUTEX Mutex);

NTKERNELAPI
LONG
NTAPI
KeReadStateSemaphore(
  IN PRKSEMAPHORE Semaphore);

NTKERNELAPI
BOOLEAN
NTAPI
KeReadStateTimer(
  IN PKTIMER Timer);

NTKERNELAPI
BOOLEAN
NTAPI
KeRegisterBugCheckCallback(
  OUT PKBUGCHECK_CALLBACK_RECORD CallbackRecord,
  IN PKBUGCHECK_CALLBACK_ROUTINE CallbackRoutine,
  IN PVOID Buffer,
  IN ULONG Length,
  IN PUCHAR Component);

NTKERNELAPI
LONG
NTAPI
KeReleaseMutex(
  IN OUT PRKMUTEX Mutex,
  IN BOOLEAN Wait);

NTKERNELAPI
LONG
NTAPI
KeReleaseSemaphore(
  IN OUT PRKSEMAPHORE Semaphore,
  IN KPRIORITY Increment,
  IN LONG Adjustment,
  IN BOOLEAN Wait);

NTKERNELAPI
PKDEVICE_QUEUE_ENTRY
NTAPI
KeRemoveByKeyDeviceQueue(
  IN OUT PKDEVICE_QUEUE DeviceQueue,
  IN ULONG SortKey);

NTKERNELAPI
PKDEVICE_QUEUE_ENTRY
NTAPI
KeRemoveDeviceQueue(
  IN OUT PKDEVICE_QUEUE DeviceQueue);

NTKERNELAPI
BOOLEAN
NTAPI
KeRemoveEntryDeviceQueue(
  IN OUT PKDEVICE_QUEUE DeviceQueue,
  IN OUT PKDEVICE_QUEUE_ENTRY DeviceQueueEntry);

NTKERNELAPI
BOOLEAN
NTAPI
KeRemoveQueueDpc(
  IN OUT PRKDPC Dpc);

NTKERNELAPI
LONG
NTAPI
KeResetEvent(
  IN OUT PRKEVENT Event);

NTKERNELAPI
LONG
NTAPI
KeSetEvent(
  IN OUT PRKEVENT Event,
  IN KPRIORITY Increment,
  IN BOOLEAN Wait);

NTKERNELAPI
VOID
NTAPI
KeSetImportanceDpc(
  IN OUT PRKDPC Dpc,
  IN KDPC_IMPORTANCE Importance);

NTKERNELAPI
KPRIORITY
NTAPI
KeSetPriorityThread(
  IN OUT PKTHREAD Thread,
  IN KPRIORITY Priority);

NTKERNELAPI
BOOLEAN
NTAPI
KeSetTimer(
  IN OUT PKTIMER Timer,
  IN LARGE_INTEGER DueTime,
  IN PKDPC Dpc OPTIONAL);

NTKERNELAPI
BOOLEAN
NTAPI
KeSetTimerEx(
  IN OUT PKTIMER Timer,
  IN LARGE_INTEGER DueTime,
  IN LONG Period OPTIONAL,
  IN PKDPC Dpc OPTIONAL);

NTHALAPI
VOID
NTAPI
KeStallExecutionProcessor(
  IN ULONG MicroSeconds);

NTKERNELAPI
BOOLEAN
NTAPI
KeSynchronizeExecution(
  IN OUT PKINTERRUPT Interrupt,
  IN PKSYNCHRONIZE_ROUTINE SynchronizeRoutine,
  IN PVOID SynchronizeContext OPTIONAL);

NTKERNELAPI
NTSTATUS
NTAPI
KeWaitForMultipleObjects(
  IN ULONG Count,
  IN PVOID Object[],
  IN WAIT_TYPE WaitType,
  IN KWAIT_REASON WaitReason,
  IN KPROCESSOR_MODE WaitMode,
  IN BOOLEAN Alertable,
  IN PLARGE_INTEGER Timeout OPTIONAL,
  OUT PKWAIT_BLOCK WaitBlockArray OPTIONAL);

#define KeWaitForMutexObject KeWaitForSingleObject

NTKERNELAPI
NTSTATUS
NTAPI
KeWaitForSingleObject(
  IN PVOID Object,
  IN KWAIT_REASON WaitReason,
  IN KPROCESSOR_MODE WaitMode,
  IN BOOLEAN Alertable,
  IN PLARGE_INTEGER Timeout OPTIONAL);
$endif

#endif /* (NTDDI_VERSION >= NTDDI_WIN2K) */

#if (NTDDI_VERSION >= NTDDI_WINXP)
$if (_NTDDK_)
NTKERNELAPI
BOOLEAN
NTAPI
KeAreApcsDisabled(VOID);
$endif

$if (_WDMDDK_)
_DECL_HAL_KE_IMPORT
VOID
FASTCALL
KeAcquireInStackQueuedSpinLock(
  IN OUT PKSPIN_LOCK SpinLock,
  OUT PKLOCK_QUEUE_HANDLE LockHandle);

NTKERNELAPI
VOID
FASTCALL
KeAcquireInStackQueuedSpinLockAtDpcLevel(
  IN OUT PKSPIN_LOCK SpinLock,
  OUT PKLOCK_QUEUE_HANDLE LockHandle);

NTKERNELAPI
KIRQL
NTAPI
KeAcquireInterruptSpinLock(
  IN OUT PKINTERRUPT Interrupt);

NTKERNELAPI
BOOLEAN
NTAPI
KeAreApcsDisabled(VOID);

NTKERNELAPI
ULONG
NTAPI
KeGetRecommendedSharedDataAlignment(VOID);

NTKERNELAPI
ULONG
NTAPI
KeQueryRuntimeThread(
  IN PKTHREAD Thread,
  OUT PULONG UserTime);

NTKERNELAPI
VOID
FASTCALL
KeReleaseInStackQueuedSpinLockFromDpcLevel(
  IN PKLOCK_QUEUE_HANDLE LockHandle);

NTKERNELAPI
VOID
NTAPI
KeReleaseInterruptSpinLock(
  IN OUT PKINTERRUPT Interrupt,
  IN KIRQL OldIrql);

NTKERNELAPI
PKDEVICE_QUEUE_ENTRY
NTAPI
KeRemoveByKeyDeviceQueueIfBusy(
  IN OUT PKDEVICE_QUEUE DeviceQueue,
  IN ULONG SortKey);

_DECL_HAL_KE_IMPORT
VOID
FASTCALL
KeReleaseInStackQueuedSpinLock(
  IN PKLOCK_QUEUE_HANDLE LockHandle);
$endif

#endif /* (NTDDI_VERSION >= NTDDI_WINXP) */

$if (_WDMDDK_)
#if (NTDDI_VERSION >= NTDDI_WINXPSP1)

NTKERNELAPI
BOOLEAN
NTAPI
KeDeregisterBugCheckReasonCallback(
  IN OUT PKBUGCHECK_REASON_CALLBACK_RECORD CallbackRecord);

NTKERNELAPI
BOOLEAN
NTAPI
KeRegisterBugCheckReasonCallback(
  OUT PKBUGCHECK_REASON_CALLBACK_RECORD CallbackRecord,
  IN PKBUGCHECK_REASON_CALLBACK_ROUTINE CallbackRoutine,
  IN KBUGCHECK_CALLBACK_REASON Reason,
  IN PUCHAR Component);

#endif /* (NTDDI_VERSION >= NTDDI_WINXPSP1) */

#if (NTDDI_VERSION >= NTDDI_WINXPSP2)
NTKERNELAPI
VOID
NTAPI
KeFlushQueuedDpcs(VOID);
#endif /* (NTDDI_VERSION >= NTDDI_WINXPSP2) */
$endif /* _WDMDDK_ */

#if (NTDDI_VERSION >= NTDDI_WS03)

$if (_WDMDDK_)
NTKERNELAPI
PVOID
NTAPI
KeRegisterNmiCallback(
  IN PNMI_CALLBACK CallbackRoutine,
  IN PVOID Context OPTIONAL);

NTKERNELAPI
NTSTATUS
NTAPI
KeDeregisterNmiCallback(
  IN PVOID Handle);

NTKERNELAPI
VOID
NTAPI
KeInitializeThreadedDpc(
  OUT PRKDPC Dpc,
  IN PKDEFERRED_ROUTINE DeferredRoutine,
  IN PVOID DeferredContext OPTIONAL);

NTKERNELAPI
ULONG_PTR
NTAPI
KeIpiGenericCall(
  IN PKIPI_BROADCAST_WORKER BroadcastFunction,
  IN ULONG_PTR Context);

NTKERNELAPI
KIRQL
FASTCALL
KeAcquireSpinLockForDpc(
  IN OUT PKSPIN_LOCK SpinLock);

NTKERNELAPI
VOID
FASTCALL
KeReleaseSpinLockForDpc(
  IN OUT PKSPIN_LOCK SpinLock,
  IN KIRQL OldIrql);

NTKERNELAPI
BOOLEAN
FASTCALL
KeTestSpinLock(
  IN PKSPIN_LOCK SpinLock);
$endif /* _WDMDDK_ */

$if (_NTDDK_)
NTKERNELAPI
BOOLEAN
NTAPI
KeInvalidateAllCaches(VOID);
$endif /* _NTDDK_ */

#endif /* (NTDDI_VERSION >= NTDDI_WS03) */

#if (NTDDI_VERSION >= NTDDI_WS03SP1)

$if (_NTDDK_)
NTKERNELAPI
NTSTATUS
NTAPI
KeExpandKernelStackAndCallout(
  IN PEXPAND_STACK_CALLOUT Callout,
  IN PVOID Parameter OPTIONAL,
  IN SIZE_T Size);

NTKERNELAPI
VOID
NTAPI
KeEnterGuardedRegion(VOID);

NTKERNELAPI
VOID
NTAPI
KeLeaveGuardedRegion(VOID);
$endif /* _NTDDK_ */

$if (_WDMDDK_)
NTKERNELAPI
BOOLEAN
FASTCALL
KeTryToAcquireSpinLockAtDpcLevel(
  IN OUT PKSPIN_LOCK SpinLock);

NTKERNELAPI
BOOLEAN
NTAPI
KeAreAllApcsDisabled(VOID);

NTKERNELAPI
VOID
FASTCALL
KeAcquireGuardedMutex(
  IN OUT PKGUARDED_MUTEX GuardedMutex);

NTKERNELAPI
VOID
FASTCALL
KeAcquireGuardedMutexUnsafe(
  IN OUT PKGUARDED_MUTEX GuardedMutex);

NTKERNELAPI
VOID
NTAPI
KeEnterGuardedRegion(VOID);

NTKERNELAPI
VOID
NTAPI
KeLeaveGuardedRegion(VOID);

NTKERNELAPI
VOID
FASTCALL
KeInitializeGuardedMutex(
  OUT PKGUARDED_MUTEX GuardedMutex);

NTKERNELAPI
VOID
FASTCALL
KeReleaseGuardedMutexUnsafe(
  IN OUT PKGUARDED_MUTEX GuardedMutex);

NTKERNELAPI
VOID
FASTCALL
KeReleaseGuardedMutex(
  IN OUT PKGUARDED_MUTEX GuardedMutex);

NTKERNELAPI
BOOLEAN
FASTCALL
KeTryToAcquireGuardedMutex(
  IN OUT PKGUARDED_MUTEX GuardedMutex);
$endif /* _WDMDDK_ */

#endif /* (NTDDI_VERSION >= NTDDI_WS03SP1) */

#if (NTDDI_VERSION >= NTDDI_VISTA)

$if (_WDMDDK_)
NTKERNELAPI
VOID
FASTCALL
KeAcquireInStackQueuedSpinLockForDpc(
  IN OUT PKSPIN_LOCK SpinLock,
  OUT PKLOCK_QUEUE_HANDLE LockHandle);

NTKERNELAPI
VOID
FASTCALL
KeReleaseInStackQueuedSpinLockForDpc(
  IN PKLOCK_QUEUE_HANDLE LockHandle);

NTKERNELAPI
NTSTATUS
NTAPI
KeQueryDpcWatchdogInformation(
  OUT PKDPC_WATCHDOG_INFORMATION WatchdogInformation);
$endif /* _WDMDDK_ */

#if defined(SINGLE_GROUP_LEGACY_API)
$if (_NTDDK_)
NTKERNELAPI
ULONG
NTAPI
KeQueryActiveProcessorCount(
  OUT PKAFFINITY ActiveProcessors OPTIONAL);

NTKERNELAPI
ULONG
NTAPI
KeQueryMaximumProcessorCount(VOID);
$endif /* _NTDDK_ */

$if (_WDMDDK_)
NTKERNELAPI
KAFFINITY
NTAPI
KeSetSystemAffinityThreadEx(
  IN KAFFINITY Affinity);

NTKERNELAPI
VOID
NTAPI
KeRevertToUserAffinityThreadEx(
  IN KAFFINITY Affinity);

NTKERNELAPI
ULONG
NTAPI
KeQueryActiveProcessorCount(
  OUT PKAFFINITY ActiveProcessors OPTIONAL);

NTKERNELAPI
ULONG
NTAPI
KeQueryMaximumProcessorCount(VOID);
$endif /* _WDMDDK_ */
#endif /* SINGLE_GROUP_LEGACY_API */

#endif /* (NTDDI_VERSION >= NTDDI_VISTA) */

$if (_WDMDDK_)
#if (NTDDI_VERSION >= NTDDI_WS08)

PVOID
KeRegisterProcessorChangeCallback(
  IN PPROCESSOR_CALLBACK_FUNCTION CallbackFunction,
  IN PVOID CallbackContext OPTIONAL,
  IN ULONG Flags);

VOID
KeDeregisterProcessorChangeCallback(
  IN PVOID CallbackHandle);

#endif /* (NTDDI_VERSION >= NTDDI_WS08) */
$endif /* _WDMDDK_ */

#if (NTDDI_VERSION >= NTDDI_WIN7)

$if (_NTDDK_)
NTKERNELAPI
ULONG
NTAPI
KeQueryActiveProcessorCountEx(
  IN USHORT GroupNumber);

NTKERNELAPI
ULONG
NTAPI
KeQueryMaximumProcessorCountEx(
  IN USHORT GroupNumber);

NTKERNELAPI
USHORT
NTAPI
KeQueryActiveGroupCount(VOID);

NTKERNELAPI
USHORT
NTAPI
KeQueryMaximumGroupCount(VOID);

NTKERNELAPI
KAFFINITY
NTAPI
KeQueryGroupAffinity(
  IN USHORT GroupNumber);

NTKERNELAPI
ULONG
NTAPI
KeGetCurrentProcessorNumberEx(
  OUT PPROCESSOR_NUMBER ProcNumber OPTIONAL);

NTKERNELAPI
VOID
NTAPI
KeQueryNodeActiveAffinity(
  IN USHORT NodeNumber,
  OUT PGROUP_AFFINITY Affinity OPTIONAL,
  OUT PUSHORT Count OPTIONAL);

NTKERNELAPI
USHORT
NTAPI
KeQueryNodeMaximumProcessorCount(
  IN USHORT NodeNumber);

NTKERNELAPI
USHORT
NTAPI
KeQueryHighestNodeNumber(VOID);

NTKERNELAPI
USHORT
NTAPI
KeGetCurrentNodeNumber(VOID);

NTKERNELAPI
NTSTATUS
NTAPI
KeQueryLogicalProcessorRelationship(
  IN PPROCESSOR_NUMBER ProcessorNumber OPTIONAL,
  IN LOGICAL_PROCESSOR_RELATIONSHIP RelationshipType,
  OUT PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX Information OPTIONAL,
  IN OUT PULONG Length);

NTKERNELAPI
NTSTATUS
NTAPI
KeSetHardwareCounterConfiguration(
  IN PHARDWARE_COUNTER CounterArray,
  IN ULONG Count);

NTKERNELAPI
NTSTATUS
NTAPI
KeQueryHardwareCounterConfiguration(
  OUT PHARDWARE_COUNTER CounterArray,
  IN ULONG MaximumCount,
  OUT PULONG Count);
$endif /* _NTDDK_ */

$if (_WDMDDK_)
ULONG64
NTAPI
KeQueryTotalCycleTimeProcess(
  IN OUT PKPROCESS Process,
  OUT PULONG64 CycleTimeStamp);

ULONG64
NTAPI
KeQueryTotalCycleTimeThread(
  IN OUT PKTHREAD Thread,
  OUT PULONG64 CycleTimeStamp);

NTKERNELAPI
NTSTATUS
NTAPI
KeSetTargetProcessorDpcEx(
  IN OUT PKDPC Dpc,
  IN PPROCESSOR_NUMBER ProcNumber);

NTKERNELAPI
VOID
NTAPI
KeSetSystemGroupAffinityThread(
  IN PGROUP_AFFINITY Affinity,
  OUT PGROUP_AFFINITY PreviousAffinity OPTIONAL);

NTKERNELAPI
VOID
NTAPI
KeRevertToUserGroupAffinityThread(
  IN PGROUP_AFFINITY PreviousAffinity);

NTKERNELAPI
BOOLEAN
NTAPI
KeSetCoalescableTimer(
  IN OUT PKTIMER Timer,
  IN LARGE_INTEGER DueTime,
  IN ULONG Period,
  IN ULONG TolerableDelay,
  IN PKDPC Dpc OPTIONAL);

NTKERNELAPI
ULONGLONG
NTAPI
KeQueryUnbiasedInterruptTime(VOID);

NTKERNELAPI
ULONG
NTAPI
KeQueryActiveProcessorCountEx(
  IN USHORT GroupNumber);

NTKERNELAPI
ULONG
NTAPI
KeQueryMaximumProcessorCountEx(
  IN USHORT GroupNumber);

NTKERNELAPI
USHORT
NTAPI
KeQueryActiveGroupCount(VOID);

NTKERNELAPI
USHORT
NTAPI
KeQueryMaximumGroupCount(VOID);

NTKERNELAPI
KAFFINITY
NTAPI
KeQueryGroupAffinity(
  IN USHORT GroupNumber);

NTKERNELAPI
ULONG
NTAPI
KeGetCurrentProcessorNumberEx(
  OUT PPROCESSOR_NUMBER ProcNumber OPTIONAL);

NTKERNELAPI
VOID
NTAPI
KeQueryNodeActiveAffinity(
  IN USHORT NodeNumber,
  OUT PGROUP_AFFINITY Affinity OPTIONAL,
  OUT PUSHORT Count OPTIONAL);

NTKERNELAPI
USHORT
NTAPI
KeQueryNodeMaximumProcessorCount(
  IN USHORT NodeNumber);

NTKERNELAPI
USHORT
NTAPI
KeQueryHighestNodeNumber(VOID);

NTKERNELAPI
USHORT
NTAPI
KeGetCurrentNodeNumber(VOID);

NTKERNELAPI
NTSTATUS
NTAPI
KeQueryLogicalProcessorRelationship(
  IN PPROCESSOR_NUMBER ProcessorNumber OPTIONAL,
  IN LOGICAL_PROCESSOR_RELATIONSHIP RelationshipType,
  OUT PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX Information OPTIONAL,
  IN OUT PULONG Length);

NTKERNELAPI
NTSTATUS
NTAPI
KeSaveExtendedProcessorState(
  IN ULONG64 Mask,
  OUT PXSTATE_SAVE XStateSave);

NTKERNELAPI
VOID
NTAPI
KeRestoreExtendedProcessorState(
  IN PXSTATE_SAVE XStateSave);

NTSTATUS
NTAPI
KeGetProcessorNumberFromIndex(
  IN ULONG ProcIndex,
  OUT PPROCESSOR_NUMBER ProcNumber);

ULONG
NTAPI
KeGetProcessorIndexFromNumber(
  IN PPROCESSOR_NUMBER ProcNumber);
$endif

#endif /* (NTDDI_VERSION >= NTDDI_WIN7) */

$if (_WDMDDK_)
#if !defined(_IA64_)
NTHALAPI
VOID
NTAPI
KeFlushWriteBuffer(VOID);
#endif

/* VOID
 * KeInitializeCallbackRecord(
 *   IN PKBUGCHECK_CALLBACK_RECORD  CallbackRecord)
 */
#define KeInitializeCallbackRecord(CallbackRecord) \
  CallbackRecord->State = BufferEmpty;

#if DBG

#if (NTDDI_VERSION >= NTDDI_VISTA)
#define PAGED_ASSERT( exp ) NT_ASSERT( exp )
#else
#define PAGED_ASSERT( exp ) ASSERT( exp )
#endif

#define PAGED_CODE() { \
  if (KeGetCurrentIrql() > APC_LEVEL) { \
    KdPrint( ("NTDDK: Pageable code called at IRQL > APC_LEVEL (%d)\n", KeGetCurrentIrql() )); \
    PAGED_ASSERT(FALSE); \
  } \
}

#else

#define PAGED_CODE()

#endif /* DBG */

#define PAGED_CODE_LOCKED() NOP_FUNCTION;
$endif

