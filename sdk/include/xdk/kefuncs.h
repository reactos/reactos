/******************************************************************************
 *                              Kernel Functions                              *
 ******************************************************************************/
$if (_WDMDDK_)
#if defined(_M_IX86)
$include(x86/ke.h)
#elif defined(_M_AMD64)
$include(amd64/ke.h)
#elif defined(_M_IA64)
$include(ia64/ke.h)
#elif defined(_M_PPC)
$include(ppc/ke.h)
#elif defined(_M_MIPS)
$include(mips/ke.h)
#elif defined(_M_ARM)
$include(arm/ke.h)
#else
#error Unknown Architecture
#endif

NTKERNELAPI
VOID
NTAPI
KeInitializeEvent(
  _Out_ PRKEVENT Event,
  _In_ EVENT_TYPE Type,
  _In_ BOOLEAN State);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
NTAPI
KeClearEvent(
  _Inout_ PRKEVENT Event);
$endif (_WDMDDK_)
$if (_NTDDK_)

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
FASTCALL
KeInvalidateRangeAllCaches(
  _In_ PVOID BaseAddress,
  _In_ ULONG Length);
$endif (_NTDDK_)

#if (NTDDI_VERSION >= NTDDI_WIN2K)

$if (_WDMDDK_)
#if defined(_NTDDK_) || defined(_NTIFS_)
_Maybe_raises_SEH_exception_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
ProbeForRead(
  __in_data_source(USER_MODE) _In_reads_bytes_(Length) CONST VOID *Address, /* CONST is added */
  _In_ SIZE_T Length,
  _In_ ULONG Alignment);
#endif /* defined(_NTDDK_) || defined(_NTIFS_) */

_Maybe_raises_SEH_exception_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
ProbeForWrite(
  __in_data_source(USER_MODE) _Out_writes_bytes_(Length) PVOID Address,
  _In_ SIZE_T Length,
  _In_ ULONG Alignment);

$endif (_WDMDDK_)
$if (_NTDDK_)
NTKERNELAPI
VOID
NTAPI
KeSetImportanceDpc(
  _Inout_ PRKDPC Dpc,
  _In_ KDPC_IMPORTANCE Importance);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
LONG
NTAPI
KePulseEvent(
  _Inout_ PRKEVENT Event,
  _In_ KPRIORITY Increment,
  _In_ BOOLEAN Wait);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
LONG
NTAPI
KeSetBasePriorityThread(
  _Inout_ PRKTHREAD Thread,
  _In_ LONG Increment);

_Acquires_lock_(_Global_critical_region_)
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
KeEnterCriticalRegion(VOID);

_Releases_lock_(_Global_critical_region_)
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
KeLeaveCriticalRegion(VOID);

NTKERNELAPI
DECLSPEC_NORETURN
VOID
NTAPI
KeBugCheck(
  _In_ ULONG BugCheckCode);
$endif(_NTDDK_)
$if (_WDMDDK_ || _NTDDK_)
#if defined(SINGLE_GROUP_LEGACY_API)
$endif (_WDMDDK_ || _NTDDK_)

$if (_WDMDDK_)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
KeRevertToUserAffinityThread(VOID);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
KeSetSystemAffinityThread(
  _In_ KAFFINITY Affinity);

NTKERNELAPI
VOID
NTAPI
KeSetTargetProcessorDpc(
  _Inout_ PRKDPC Dpc,
  _In_ CCHAR Number);

NTKERNELAPI
KAFFINITY
NTAPI
KeQueryActiveProcessors(VOID);
$endif (_WDMDDK_)
$if (_NTDDK_)

NTKERNELAPI
VOID
NTAPI
KeSetTargetProcessorDpc(
  _Inout_ PRKDPC Dpc,
  _In_ CCHAR Number);

NTKERNELAPI
KAFFINITY
NTAPI
KeQueryActiveProcessors(VOID);
$endif (_NTDDK_)
$if (_WDMDDK_ || _NTDDK_)
#endif /* defined(SINGLE_GROUP_LEGACY_API) */
$endif (_WDMDDK_ || _NTDDK_)

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
  _Out_ PLARGE_INTEGER CurrentTime);
#endif /* !_M_AMD64 */

#if !defined(_X86_) && !defined(_M_ARM)
_Requires_lock_not_held_(*SpinLock)
_Acquires_lock_(*SpinLock)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_saves_
_IRQL_raises_(DISPATCH_LEVEL)
NTKERNELAPI
KIRQL
NTAPI
KeAcquireSpinLockRaiseToDpc(
  _Inout_ PKSPIN_LOCK SpinLock);

#define KeAcquireSpinLock(SpinLock, OldIrql) \
    *(OldIrql) = KeAcquireSpinLockRaiseToDpc(SpinLock)

_Requires_lock_not_held_(*SpinLock)
_Acquires_lock_(*SpinLock)
_IRQL_requires_min_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
NTAPI
KeAcquireSpinLockAtDpcLevel(
  _Inout_ PKSPIN_LOCK SpinLock);

_Requires_lock_held_(*SpinLock)
_Releases_lock_(*SpinLock)
_IRQL_requires_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
NTAPI
KeReleaseSpinLock(
  _Inout_ PKSPIN_LOCK SpinLock,
  _In_ _IRQL_restores_ KIRQL NewIrql);

_Requires_lock_held_(*SpinLock)
_Releases_lock_(*SpinLock)
_IRQL_requires_min_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
NTAPI
KeReleaseSpinLockFromDpcLevel(
  _Inout_ PKSPIN_LOCK SpinLock);
#endif /* !_X86_ */

#if defined(_X86_) && (defined(_WDM_INCLUDED_) || defined(WIN9X_COMPAT_SPINLOCK))
NTKERNELAPI
VOID
NTAPI
KeInitializeSpinLock(
  _Out_ PKSPIN_LOCK SpinLock);
#else
FORCEINLINE
VOID
KeInitializeSpinLock(_Out_ PKSPIN_LOCK SpinLock)
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
  _In_ ULONG BugCheckCode,
  _In_ ULONG_PTR BugCheckParameter1,
  _In_ ULONG_PTR BugCheckParameter2,
  _In_ ULONG_PTR BugCheckParameter3,
  _In_ ULONG_PTR BugCheckParameter4);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
KeCancelTimer(
  _Inout_ PKTIMER);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
KeDelayExecutionThread(
  _In_ KPROCESSOR_MODE WaitMode,
  _In_ BOOLEAN Alertable,
  _In_ PLARGE_INTEGER Interval);

_Must_inspect_result_
NTKERNELAPI
BOOLEAN
NTAPI
KeDeregisterBugCheckCallback(
  _Inout_ PKBUGCHECK_CALLBACK_RECORD CallbackRecord);

_Acquires_lock_(_Global_critical_region_)
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
KeEnterCriticalRegion(VOID);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
NTAPI
KeInitializeDeviceQueue(
  _Out_ PKDEVICE_QUEUE DeviceQueue);

NTKERNELAPI
VOID
NTAPI
KeInitializeDpc(
  _Out_ __drv_aliasesMem PRKDPC Dpc,
  _In_ PKDEFERRED_ROUTINE DeferredRoutine,
  _In_opt_ __drv_aliasesMem PVOID DeferredContext);

NTKERNELAPI
VOID
NTAPI
KeInitializeMutex(
  _Out_ PRKMUTEX Mutex,
  _In_ ULONG Level);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
VOID
NTAPI
KeInitializeSemaphore(
  _Out_ PRKSEMAPHORE Semaphore,
  _In_ LONG Count,
  _In_ LONG Limit);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
NTAPI
KeInitializeTimer(
  _Out_ PKTIMER Timer);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
NTAPI
KeInitializeTimerEx(
  _Out_ PKTIMER Timer,
  _In_ TIMER_TYPE Type);

_IRQL_requires_(DISPATCH_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
KeInsertByKeyDeviceQueue(
  _Inout_ PKDEVICE_QUEUE DeviceQueue,
  _Inout_ PKDEVICE_QUEUE_ENTRY DeviceQueueEntry,
  _In_ ULONG SortKey);

_IRQL_requires_(DISPATCH_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
KeInsertDeviceQueue(
  _Inout_ PKDEVICE_QUEUE DeviceQueue,
  _Inout_ PKDEVICE_QUEUE_ENTRY DeviceQueueEntry);

NTKERNELAPI
BOOLEAN
NTAPI
KeInsertQueueDpc(
  _Inout_ PRKDPC Dpc,
  _In_opt_ PVOID SystemArgument1,
  _In_opt_ PVOID SystemArgument2);

_Releases_lock_(_Global_critical_region_)
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
KeLeaveCriticalRegion(VOID);

NTHALAPI
LARGE_INTEGER
NTAPI
KeQueryPerformanceCounter(
  _Out_opt_ PLARGE_INTEGER PerformanceFrequency);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
KPRIORITY
NTAPI
KeQueryPriorityThread(
  _In_ PRKTHREAD Thread);

NTKERNELAPI
ULONG
NTAPI
KeQueryTimeIncrement(VOID);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
LONG
NTAPI
KeReadStateEvent(
  _In_ PRKEVENT Event);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
LONG
NTAPI
KeReadStateMutex(
  _In_ PRKMUTEX Mutex);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
LONG
NTAPI
KeReadStateSemaphore(
  _In_ PRKSEMAPHORE Semaphore);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
KeReadStateTimer(
  _In_ PKTIMER Timer);

_Must_inspect_result_
NTKERNELAPI
BOOLEAN
NTAPI
KeRegisterBugCheckCallback(
  _Out_ PKBUGCHECK_CALLBACK_RECORD CallbackRecord,
  _In_ PKBUGCHECK_CALLBACK_ROUTINE CallbackRoutine,
  _In_reads_bytes_opt_(Length) PVOID Buffer,
  _In_ ULONG Length,
  _In_ PUCHAR Component);

_When_(Wait==0, _IRQL_requires_max_(DISPATCH_LEVEL))
_When_(Wait==1, _IRQL_requires_max_(APC_LEVEL))
NTKERNELAPI
LONG
NTAPI
KeReleaseMutex(
  _Inout_ PRKMUTEX Mutex,
  _In_ BOOLEAN Wait);

_When_(Wait==0, _IRQL_requires_max_(DISPATCH_LEVEL))
_When_(Wait==1, _IRQL_requires_max_(APC_LEVEL))
NTKERNELAPI
LONG
NTAPI
KeReleaseSemaphore(
  _Inout_ PRKSEMAPHORE Semaphore,
  _In_ KPRIORITY Increment,
  _In_ LONG Adjustment,
  _In_ _Literal_ BOOLEAN Wait);

_IRQL_requires_(DISPATCH_LEVEL)
NTKERNELAPI
PKDEVICE_QUEUE_ENTRY
NTAPI
KeRemoveByKeyDeviceQueue(
  _Inout_ PKDEVICE_QUEUE DeviceQueue,
  _In_ ULONG SortKey);

_IRQL_requires_(DISPATCH_LEVEL)
NTKERNELAPI
PKDEVICE_QUEUE_ENTRY
NTAPI
KeRemoveDeviceQueue(
  _Inout_ PKDEVICE_QUEUE DeviceQueue);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
KeRemoveEntryDeviceQueue(
  _Inout_ PKDEVICE_QUEUE DeviceQueue,
  _Inout_ PKDEVICE_QUEUE_ENTRY DeviceQueueEntry);

_IRQL_requires_max_(HIGH_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
KeRemoveQueueDpc(
  _Inout_ PRKDPC Dpc);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
LONG
NTAPI
KeResetEvent(
  _Inout_ PRKEVENT Event);

_When_(Wait==0, _IRQL_requires_max_(DISPATCH_LEVEL))
_When_(Wait==1, _IRQL_requires_max_(APC_LEVEL))
NTKERNELAPI
LONG
NTAPI
KeSetEvent(
  _Inout_ PRKEVENT Event,
  _In_ KPRIORITY Increment,
  _In_ _Literal_ BOOLEAN Wait);

NTKERNELAPI
VOID
NTAPI
KeSetImportanceDpc(
  _Inout_ PRKDPC Dpc,
  _In_ KDPC_IMPORTANCE Importance);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
KPRIORITY
NTAPI
KeSetPriorityThread(
  _Inout_ PKTHREAD Thread,
  _In_ KPRIORITY Priority);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
KeSetTimer(
  _Inout_ PKTIMER Timer,
  _In_ LARGE_INTEGER DueTime,
  _In_opt_ PKDPC Dpc);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
KeSetTimerEx(
  _Inout_ PKTIMER Timer,
  _In_ LARGE_INTEGER DueTime,
  _In_ LONG Period OPTIONAL,
  _In_opt_ PKDPC Dpc);

NTHALAPI
VOID
NTAPI
KeStallExecutionProcessor(
  _In_ ULONG MicroSeconds);

_IRQL_requires_max_(HIGH_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
KeSynchronizeExecution(
  _Inout_ PKINTERRUPT Interrupt,
  _In_ PKSYNCHRONIZE_ROUTINE SynchronizeRoutine,
  _In_opt_ __drv_aliasesMem PVOID SynchronizeContext);

_IRQL_requires_min_(PASSIVE_LEVEL)
_When_((Timeout==NULL || Timeout->QuadPart!=0), _IRQL_requires_max_(APC_LEVEL))
_When_((Timeout!=NULL && Timeout->QuadPart==0), _IRQL_requires_max_(DISPATCH_LEVEL))
NTKERNELAPI
NTSTATUS
NTAPI
KeWaitForMultipleObjects(
  _In_ ULONG Count,
  _In_reads_(Count) PVOID Object[],
  _In_ __drv_strictTypeMatch(__drv_typeConst) WAIT_TYPE WaitType,
  _In_ __drv_strictTypeMatch(__drv_typeCond) KWAIT_REASON WaitReason,
  _In_ __drv_strictType(KPROCESSOR_MODE/enum _MODE,__drv_typeConst) KPROCESSOR_MODE WaitMode,
  _In_ BOOLEAN Alertable,
  _In_opt_ PLARGE_INTEGER Timeout,
  _Out_opt_ PKWAIT_BLOCK WaitBlockArray);

#define KeWaitForMutexObject KeWaitForSingleObject

_IRQL_requires_min_(PASSIVE_LEVEL)
_When_((Timeout==NULL || Timeout->QuadPart!=0), _IRQL_requires_max_(APC_LEVEL))
_When_((Timeout!=NULL && Timeout->QuadPart==0), _IRQL_requires_max_(DISPATCH_LEVEL))
NTKERNELAPI
NTSTATUS
NTAPI
KeWaitForSingleObject(
  _In_ _Points_to_data_ PVOID Object,
  _In_ __drv_strictTypeMatch(__drv_typeCond) KWAIT_REASON WaitReason,
  _In_ __drv_strictType(KPROCESSOR_MODE/enum _MODE,__drv_typeConst) KPROCESSOR_MODE WaitMode,
  _In_ BOOLEAN Alertable,
  _In_opt_ PLARGE_INTEGER Timeout);
$endif (_WDMDDK_)
$if (_NTIFS_)

NTKERNELAPI
VOID
NTAPI
KeInitializeMutant(
  _Out_ PRKMUTANT Mutant,
  _In_ BOOLEAN InitialOwner);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
LONG
NTAPI
KeReadStateMutant(
  _In_ PRKMUTANT Mutant);

_When_(Wait==0, _IRQL_requires_max_(DISPATCH_LEVEL))
_When_(Wait==1, _IRQL_requires_max_(APC_LEVEL))
NTKERNELAPI
LONG
NTAPI
KeReleaseMutant(
  _Inout_ PRKMUTANT Mutant,
  _In_ KPRIORITY Increment,
  _In_ BOOLEAN Abandoned,
  _In_ BOOLEAN Wait);

NTKERNELAPI
VOID
NTAPI
KeInitializeQueue(
  _Out_ PRKQUEUE Queue,
  _In_ ULONG Count);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
LONG
NTAPI
KeReadStateQueue(
  _In_ PRKQUEUE Queue);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
LONG
NTAPI
KeInsertQueue(
  _Inout_ PRKQUEUE Queue,
  _Inout_ PLIST_ENTRY Entry);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
LONG
NTAPI
KeInsertHeadQueue(
  _Inout_ PRKQUEUE Queue,
  _Inout_ PLIST_ENTRY Entry);

_IRQL_requires_min_(PASSIVE_LEVEL)
_When_((Timeout==NULL || Timeout->QuadPart!=0), _IRQL_requires_max_(APC_LEVEL))
_When_((Timeout!=NULL && Timeout->QuadPart==0), _IRQL_requires_max_(DISPATCH_LEVEL))
NTKERNELAPI
PLIST_ENTRY
NTAPI
KeRemoveQueue(
  _Inout_ PRKQUEUE Queue,
  _In_ KPROCESSOR_MODE WaitMode,
  _In_opt_ PLARGE_INTEGER Timeout);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
KeAttachProcess(
  _Inout_ PKPROCESS Process);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
KeDetachProcess(VOID);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
PLIST_ENTRY
NTAPI
KeRundownQueue(
  _Inout_ PRKQUEUE Queue);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
KeStackAttachProcess(
  _Inout_ PKPROCESS Process,
  _Out_ PKAPC_STATE ApcState);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
KeUnstackDetachProcess(
  _In_ PKAPC_STATE ApcState);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
UCHAR
NTAPI
KeSetIdealProcessorThread(
  _Inout_ PKTHREAD Thread,
  _In_ UCHAR Processor);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
KeSetKernelStackSwapEnable(
  _In_ BOOLEAN Enable);

#if defined(_X86_)
_Requires_lock_not_held_(*SpinLock)
_Acquires_lock_(*SpinLock)
_IRQL_raises_(SYNCH_LEVEL)
_IRQL_saves_
NTHALAPI
KIRQL
FASTCALL
KeAcquireSpinLockRaiseToSynch(
  _Inout_ PKSPIN_LOCK SpinLock);
#else
_Requires_lock_not_held_(*SpinLock)
_Acquires_lock_(*SpinLock)
_IRQL_raises_(SYNCH_LEVEL)
_IRQL_saves_
NTKERNELAPI
KIRQL
KeAcquireSpinLockRaiseToSynch(
  _Inout_ PKSPIN_LOCK SpinLock);
#endif
$endif (_NTIFS_)

#endif /* (NTDDI_VERSION >= NTDDI_WIN2K) */

#if (NTDDI_VERSION >= NTDDI_WINXP)

$if (_WDMDDK_)
_Requires_lock_not_held_(*LockHandle)
_Acquires_lock_(*LockHandle)
_Post_same_lock_(*SpinLock, *LockHandle)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_saves_global_(QueuedSpinLock,LockHandle)
_IRQL_raises_(DISPATCH_LEVEL)
_DECL_HAL_KE_IMPORT
VOID
FASTCALL
KeAcquireInStackQueuedSpinLock(
  _Inout_ PKSPIN_LOCK SpinLock,
  _Out_ PKLOCK_QUEUE_HANDLE LockHandle);

_Requires_lock_not_held_(*LockHandle)
_Acquires_lock_(*LockHandle)
_Post_same_lock_(*SpinLock, *LockHandle)
_IRQL_requires_min_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
FASTCALL
KeAcquireInStackQueuedSpinLockAtDpcLevel(
  _Inout_ PKSPIN_LOCK SpinLock,
  _Out_ PKLOCK_QUEUE_HANDLE LockHandle);

_Requires_lock_not_held_(*Interrupt->ActualLock)
_Acquires_lock_(*Interrupt->ActualLock)
_IRQL_requires_max_(HIGH_LEVEL)
_IRQL_saves_
_IRQL_raises_(HIGH_LEVEL)
NTKERNELAPI
KIRQL
NTAPI
KeAcquireInterruptSpinLock(
  _Inout_ PKINTERRUPT Interrupt);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
KeAreApcsDisabled(VOID);

NTKERNELAPI
ULONG
NTAPI
KeGetRecommendedSharedDataAlignment(VOID);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
ULONG
NTAPI
KeQueryRuntimeThread(
  _In_ PKTHREAD Thread,
  _Out_ PULONG UserTime);

_Requires_lock_held_(*LockHandle)
_Releases_lock_(*LockHandle)
_IRQL_requires_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
FASTCALL
KeReleaseInStackQueuedSpinLockFromDpcLevel(
  _In_ PKLOCK_QUEUE_HANDLE LockHandle);

_Requires_lock_held_(*Interrupt->ActualLock)
_Releases_lock_(*Interrupt->ActualLock)
_IRQL_requires_(HIGH_LEVEL)
NTKERNELAPI
VOID
NTAPI
KeReleaseInterruptSpinLock(
  _Inout_ PKINTERRUPT Interrupt,
  _In_ _IRQL_restores_ KIRQL OldIrql);

_IRQL_requires_(DISPATCH_LEVEL)
NTKERNELAPI
PKDEVICE_QUEUE_ENTRY
NTAPI
KeRemoveByKeyDeviceQueueIfBusy(
  _Inout_ PKDEVICE_QUEUE DeviceQueue,
  _In_ ULONG SortKey);

_Requires_lock_held_(*LockHandle)
_Releases_lock_(*LockHandle)
_IRQL_requires_(DISPATCH_LEVEL)
_IRQL_restores_global_(QueuedSpinLock,LockHandle)
_DECL_HAL_KE_IMPORT
VOID
FASTCALL
KeReleaseInStackQueuedSpinLock(
  _In_ PKLOCK_QUEUE_HANDLE LockHandle);
$endif (_WDMDDK_)
$if (_NTDDK_)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
KeAreApcsDisabled(VOID);
$endif (_NTDDK_)
$if (_NTIFS_)

_Requires_lock_not_held_(Number)
_Acquires_lock_(Number)
_IRQL_raises_(DISPATCH_LEVEL)
_DECL_HAL_KE_IMPORT
KIRQL
FASTCALL
KeAcquireQueuedSpinLock(
  _In_ KSPIN_LOCK_QUEUE_NUMBER Number);

_Requires_lock_held_(Number)
_Releases_lock_(Number)
_DECL_HAL_KE_IMPORT
VOID
FASTCALL
KeReleaseQueuedSpinLock(
  _In_ KSPIN_LOCK_QUEUE_NUMBER Number,
  _In_ KIRQL OldIrql);

_Must_inspect_result_
_Post_satisfies_(return == 1 || return == 0)
_DECL_HAL_KE_IMPORT
LOGICAL
FASTCALL
KeTryToAcquireQueuedSpinLock(
  _In_ KSPIN_LOCK_QUEUE_NUMBER Number,
  _Out_ _At_(*OldIrql, _IRQL_saves_) PKIRQL OldIrql);
$endif (_NTIFS_)

#endif /* (NTDDI_VERSION >= NTDDI_WINXP) */

$if (_WDMDDK_)
#if (NTDDI_VERSION >= NTDDI_WINXPSP1)

_Must_inspect_result_
NTKERNELAPI
BOOLEAN
NTAPI
KeDeregisterBugCheckReasonCallback(
  _Inout_ PKBUGCHECK_REASON_CALLBACK_RECORD CallbackRecord);

_Must_inspect_result_
NTKERNELAPI
BOOLEAN
NTAPI
KeRegisterBugCheckReasonCallback(
  _Out_ PKBUGCHECK_REASON_CALLBACK_RECORD CallbackRecord,
  _In_ PKBUGCHECK_REASON_CALLBACK_ROUTINE CallbackRoutine,
  _In_ KBUGCHECK_CALLBACK_REASON Reason,
  _In_ PUCHAR Component);

#endif /* (NTDDI_VERSION >= NTDDI_WINXPSP1) */

#if (NTDDI_VERSION >= NTDDI_WINXPSP2)
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
KeFlushQueuedDpcs(VOID);
#endif /* (NTDDI_VERSION >= NTDDI_WINXPSP2) */
$endif (_WDMDDK_)
$if (_WDMDDK_ || _NTDDK_)
#if (NTDDI_VERSION >= NTDDI_WS03)
$endif (_WDMDDK_ || _NTDDK_)

$if (_WDMDDK_)
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
PVOID
NTAPI
KeRegisterNmiCallback(
  _In_ PNMI_CALLBACK CallbackRoutine,
  _In_opt_ PVOID Context);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
KeDeregisterNmiCallback(
  _In_ PVOID Handle);

NTKERNELAPI
VOID
NTAPI
KeInitializeThreadedDpc(
  _Out_ PRKDPC Dpc,
  _In_ PKDEFERRED_ROUTINE DeferredRoutine,
  _In_opt_ PVOID DeferredContext);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(IPI_LEVEL-1)
NTKERNELAPI
ULONG_PTR
NTAPI
KeIpiGenericCall(
  _In_ PKIPI_BROADCAST_WORKER BroadcastFunction,
  _In_ ULONG_PTR Context);

_Requires_lock_not_held_(*SpinLock)
_Acquires_lock_(*SpinLock)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_saves_
NTKERNELAPI
KIRQL
FASTCALL
KeAcquireSpinLockForDpc(
  _Inout_ PKSPIN_LOCK SpinLock);

_Requires_lock_held_(*SpinLock)
_Releases_lock_(*SpinLock)
_IRQL_requires_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
FASTCALL
KeReleaseSpinLockForDpc(
  _Inout_ PKSPIN_LOCK SpinLock,
  _In_ _IRQL_restores_ KIRQL OldIrql);

_Must_inspect_result_
NTKERNELAPI
BOOLEAN
FASTCALL
KeTestSpinLock(
  _In_ PKSPIN_LOCK SpinLock);
$endif (_WDMDDK_)

$if (_NTDDK_)
NTKERNELAPI
BOOLEAN
NTAPI
KeInvalidateAllCaches(VOID);
$endif (_NTDDK_)
$if (_WDMDDK_ || _NTDDK_)
#endif /* (NTDDI_VERSION >= NTDDI_WS03) */
$endif (_WDMDDK_ || _NTDDK_)
$if (_WDMDDK_ || _NTDDK_)
#if (NTDDI_VERSION >= NTDDI_WS03SP1)
$endif (_WDMDDK_ || _NTDDK_)

$if (_WDMDDK_)
_Must_inspect_result_
_IRQL_requires_min_(DISPATCH_LEVEL)
_Post_satisfies_(return == 1 || return == 0)
NTKERNELAPI
BOOLEAN
FASTCALL
KeTryToAcquireSpinLockAtDpcLevel(
  _Inout_ _Requires_lock_not_held_(*_Curr_)
  _When_(return!=0, _Acquires_lock_(*_Curr_))
    PKSPIN_LOCK SpinLock);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
KeAreAllApcsDisabled(VOID);

_Acquires_lock_(_Global_critical_region_)
_Requires_lock_not_held_(*Mutex)
_Acquires_lock_(*Mutex)
_IRQL_requires_max_(APC_LEVEL)
_IRQL_requires_min_(PASSIVE_LEVEL)
NTKERNELAPI
VOID
FASTCALL
KeAcquireGuardedMutex(
  _Inout_ PKGUARDED_MUTEX GuardedMutex);

_Requires_lock_not_held_(*FastMutex)
_Acquires_lock_(*FastMutex)
_IRQL_requires_max_(APC_LEVEL)
_IRQL_requires_min_(PASSIVE_LEVEL)
NTKERNELAPI
VOID
FASTCALL
KeAcquireGuardedMutexUnsafe(
  _Inout_ PKGUARDED_MUTEX GuardedMutex);

_Acquires_lock_(_Global_critical_region_)
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
KeEnterGuardedRegion(VOID);

_Releases_lock_(_Global_critical_region_)
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
KeLeaveGuardedRegion(VOID);

_IRQL_requires_max_(APC_LEVEL)
_IRQL_requires_min_(PASSIVE_LEVEL)
NTKERNELAPI
VOID
FASTCALL
KeInitializeGuardedMutex(
  _Out_ PKGUARDED_MUTEX GuardedMutex);

_Requires_lock_held_(*FastMutex)
_Releases_lock_(*FastMutex)
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
FASTCALL
KeReleaseGuardedMutexUnsafe(
  _Inout_ PKGUARDED_MUTEX GuardedMutex);

_Releases_lock_(_Global_critical_region_)
_Requires_lock_held_(*Mutex)
_Releases_lock_(*Mutex)
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
FASTCALL
KeReleaseGuardedMutex(
  _Inout_ PKGUARDED_MUTEX GuardedMutex);

_Must_inspect_result_
_Success_(return != FALSE)
_IRQL_requires_max_(APC_LEVEL)
_Post_satisfies_(return == 1 || return == 0)
NTKERNELAPI
BOOLEAN
FASTCALL
KeTryToAcquireGuardedMutex(
  _When_ (return, _Requires_lock_not_held_(*_Curr_) _Acquires_exclusive_lock_(*_Curr_)) _Acquires_lock_(_Global_critical_region_)
  _Inout_ PKGUARDED_MUTEX GuardedMutex);
$endif (_WDMDDK_)
$if (_NTDDK_)
_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
KeExpandKernelStackAndCallout(
  _In_ PEXPAND_STACK_CALLOUT Callout,
  _In_opt_ PVOID Parameter,
  _In_ SIZE_T Size);

_Acquires_lock_(_Global_critical_region_)
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
KeEnterGuardedRegion(VOID);

_Releases_lock_(_Global_critical_region_)
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
KeLeaveGuardedRegion(VOID);
$endif (_NTDDK_)
$if (_WDMDDK_ || _NTDDK_)
#endif /* (NTDDI_VERSION >= NTDDI_WS03SP1) */
$endif (_WDMDDK_ || _NTDDK_)

#if (NTDDI_VERSION >= NTDDI_VISTA)
$if (_WDMDDK_)
_Requires_lock_not_held_(*LockHandle)
_Acquires_lock_(*LockHandle)
_Post_same_lock_(*SpinLock, *LockHandle)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_saves_global_(QueuedSpinLock,LockHandle)
NTKERNELAPI
VOID
FASTCALL
KeAcquireInStackQueuedSpinLockForDpc(
  _Inout_ PKSPIN_LOCK SpinLock,
  _Out_ PKLOCK_QUEUE_HANDLE LockHandle);

_Requires_lock_held_(*LockHandle)
_Releases_lock_(*LockHandle)
_IRQL_requires_(DISPATCH_LEVEL)
_IRQL_restores_global_(QueuedSpinLock,LockHandle)
NTKERNELAPI
VOID
FASTCALL
KeReleaseInStackQueuedSpinLockForDpc(
  _In_ PKLOCK_QUEUE_HANDLE LockHandle);

_IRQL_requires_(DISPATCH_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
KeQueryDpcWatchdogInformation(
  _Out_ PKDPC_WATCHDOG_INFORMATION WatchdogInformation);
$endif (_WDMDDK_)
$if (_WDMDDK_ || _NTDDK_)
#if defined(SINGLE_GROUP_LEGACY_API)
$endif (_WDMDDK_ || _NTDDK_)

$if (_WDMDDK_)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
KAFFINITY
NTAPI
KeSetSystemAffinityThreadEx(
  _In_ KAFFINITY Affinity);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
KeRevertToUserAffinityThreadEx(
  _In_ KAFFINITY Affinity);

NTKRNLVISTAAPI
ULONG
NTAPI
KeQueryActiveProcessorCount(
  _Out_opt_ PKAFFINITY ActiveProcessors);

NTKERNELAPI
ULONG
NTAPI
KeQueryMaximumProcessorCount(VOID);
$endif (_WDMDDK_)
$if (_NTDDK_)
NTKRNLVISTAAPI
ULONG
NTAPI
KeQueryActiveProcessorCount(
  _Out_opt_ PKAFFINITY ActiveProcessors);

NTKERNELAPI
ULONG
NTAPI
KeQueryMaximumProcessorCount(VOID);
$endif (_NTDDK_)
$if (_WDMDDK_ || _NTDDK_)
#endif /* SINGLE_GROUP_LEGACY_API */
$endif (_WDMDDK_ || _NTDDK_)
$if (_NTIFS_)
_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
KeQueryOwnerMutant(
  _In_ PKMUTANT Mutant,
  _Out_ PCLIENT_ID ClientId);

_IRQL_requires_min_(PASSIVE_LEVEL)
_When_((Timeout==NULL || *Timeout!=0), _IRQL_requires_max_(APC_LEVEL))
_When_((Timeout!=NULL && *Timeout==0), _IRQL_requires_max_(DISPATCH_LEVEL))
NTKERNELAPI
ULONG
NTAPI
KeRemoveQueueEx(
  _Inout_ PKQUEUE Queue,
  _In_ KPROCESSOR_MODE WaitMode,
  _In_ BOOLEAN Alertable,
  _In_opt_ PLARGE_INTEGER Timeout,
  _Out_writes_to_(Count, return) PLIST_ENTRY *EntryArray,
  _In_ ULONG Count);
$endif (_NTIFS_)

#endif /* (NTDDI_VERSION >= NTDDI_VISTA) */

$if (_WDMDDK_)
#if (NTDDI_VERSION >= NTDDI_WS08)

_IRQL_requires_max_(APC_LEVEL)
PVOID
NTAPI
KeRegisterProcessorChangeCallback(
  _In_ PPROCESSOR_CALLBACK_FUNCTION CallbackFunction,
  _In_opt_ PVOID CallbackContext,
  _In_ ULONG Flags);

_IRQL_requires_max_(APC_LEVEL)
VOID
NTAPI
KeDeregisterProcessorChangeCallback(
  _In_ PVOID CallbackHandle);

#endif /* (NTDDI_VERSION >= NTDDI_WS08) */
$endif (_WDMDDK_)
$if (_WDMDDK_ || _NTDDK_)
#if (NTDDI_VERSION >= NTDDI_WIN7)

NTKERNELAPI
ULONG
NTAPI
KeQueryActiveProcessorCountEx(
  _In_ USHORT GroupNumber);

NTKERNELAPI
ULONG
NTAPI
KeQueryMaximumProcessorCountEx(
  _In_ USHORT GroupNumber);

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
  _In_ USHORT GroupNumber);

NTKERNELAPI
ULONG
NTAPI
KeGetCurrentProcessorNumberEx(
  _Out_opt_ PPROCESSOR_NUMBER ProcNumber);

NTKERNELAPI
VOID
NTAPI
KeQueryNodeActiveAffinity(
  _In_ USHORT NodeNumber,
  _Out_opt_ PGROUP_AFFINITY Affinity,
  _Out_opt_ PUSHORT Count);

NTKERNELAPI
USHORT
NTAPI
KeQueryNodeMaximumProcessorCount(
  _In_ USHORT NodeNumber);

NTKRNLVISTAAPI
USHORT
NTAPI
KeQueryHighestNodeNumber(VOID);

NTKRNLVISTAAPI
USHORT
NTAPI
KeGetCurrentNodeNumber(VOID);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
KeQueryLogicalProcessorRelationship(
  _In_opt_ PPROCESSOR_NUMBER ProcessorNumber OPTIONAL,
  _In_ LOGICAL_PROCESSOR_RELATIONSHIP RelationshipType,
  _Out_writes_bytes_opt_(*Length) PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX Information,
  _Inout_ PULONG Length);

$endif (_WDMDDK_ || _NTDDK_)

$if (_WDMDDK_)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_same_
ULONG64
NTAPI
KeQueryTotalCycleTimeProcess(
  _Inout_ PKPROCESS Process,
  _Out_ PULONG64 CycleTimeStamp);

_IRQL_requires_max_(APC_LEVEL)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_same_
ULONG64
NTAPI
KeQueryTotalCycleTimeThread(
  _Inout_ PKTHREAD Thread,
  _Out_ PULONG64 CycleTimeStamp);

_Must_inspect_result_
NTKERNELAPI
NTSTATUS
NTAPI
KeSetTargetProcessorDpcEx(
  _Inout_ PKDPC Dpc,
  _In_ PPROCESSOR_NUMBER ProcNumber);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
KeSetSystemGroupAffinityThread(
  _In_ PGROUP_AFFINITY Affinity,
  _Out_opt_ PGROUP_AFFINITY PreviousAffinity);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
KeRevertToUserGroupAffinityThread(
  _In_ PGROUP_AFFINITY PreviousAffinity);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKRNLVISTAAPI
BOOLEAN
NTAPI
KeSetCoalescableTimer(
  _Inout_ PKTIMER Timer,
  _In_ LARGE_INTEGER DueTime,
  _In_ ULONG Period,
  _In_ ULONG TolerableDelay,
  _In_opt_ PKDPC Dpc);

NTKERNELAPI
ULONGLONG
NTAPI
KeQueryUnbiasedInterruptTime(VOID);

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
_Ret_range_(<=, 0)
_When_(return==0, _Kernel_float_saved_)
NTKERNELAPI
NTSTATUS
NTAPI
KeSaveExtendedProcessorState(
  _In_ ULONG64 Mask,
  _Out_ _Requires_lock_not_held_(*_Curr_)
  _When_(return==0, _Acquires_lock_(*_Curr_))
    PXSTATE_SAVE XStateSave);

_Kernel_float_restored_
NTKERNELAPI
VOID
NTAPI
KeRestoreExtendedProcessorState(
  _In_ _Requires_lock_held_(*_Curr_) _Releases_lock_(*_Curr_)
    PXSTATE_SAVE XStateSave);

NTSTATUS
NTAPI
KeGetProcessorNumberFromIndex(
  _In_ ULONG ProcIndex,
  _Out_ PPROCESSOR_NUMBER ProcNumber);

ULONG
NTAPI
KeGetProcessorIndexFromNumber(
  _In_ PPROCESSOR_NUMBER ProcNumber);
$endif (_WDMDDK_)
$if (_NTDDK_)
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
KeSetHardwareCounterConfiguration(
  _In_reads_(Count) PHARDWARE_COUNTER CounterArray,
  _In_ ULONG Count);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
KeQueryHardwareCounterConfiguration(
  _Out_writes_to_(MaximumCount, *Count) PHARDWARE_COUNTER CounterArray,
  _In_ ULONG MaximumCount,
  _Out_ PULONG Count);
$endif (_NTDDK_)
$if (_WDMDDK_ || _NTDDK_)
#endif /* (NTDDI_VERSION >= NTDDI_WIN7) */
$endif (_WDMDDK_ || _NTDDK_)
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
  (CallbackRecord)->State = BufferEmpty;

#if defined(_PREFAST_)

void __PREfastPagedCode(void);
void __PREfastPagedCodeLocked(void);
#define PAGED_CODE() __PREfastPagedCode();
#define PAGED_CODE_LOCKED() __PREfastPagedCodeLocked();

#elif DBG

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

#define PAGED_CODE_LOCKED() NOP_FUNCTION;

#else

#define PAGED_CODE() NOP_FUNCTION;
#define PAGED_CODE_LOCKED() NOP_FUNCTION;

#endif /* DBG */

$endif (_WDMDDK_)
