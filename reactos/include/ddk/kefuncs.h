#ifndef __INCLUDE_DDK_KEFUNCS_H
#define __INCLUDE_DDK_KEFUNCS_H

/* KERNEL FUNCTIONS ********************************************************/

VOID KeDrainApcQueue(VOID);
VOID KeInitializeApc(PKAPC Apc, PKNORMAL_ROUTINE NormalRoutine,
		     PVOID NormalContext,
		     PKTHREAD TargetThread);
BOOLEAN KeInsertQueueApc(PKAPC Apc);

/*
 * FUNCTION: Acquires a spinlock so the caller can synchronize access to 
 * data
 * ARGUMENTS:
 *         SpinLock = Initialized spinlock
 *         OldIrql (OUT) = Set the previous irql on return 
 */
VOID KeAcquireSpinLock(PKSPIN_LOCK SpinLock, PKIRQL OldIrql);

VOID KeAcquireSpinLockAtDpcLevel(PKSPIN_LOCK SpinLock);
BOOLEAN KeCancelTimer(PKTIMER Timer);
VOID KeClearEvent(PKEVENT Event);
NTSTATUS KeDelayExecutionThread(KPROCESSOR_MODE WaitMode,
				BOOLEAN Alertable,
				PLARGE_INTEGER Internal);
BOOLEAN KeDeregisterBugCheckCallback(PKBUGCHECK_CALLBACK_RECORD
				     CallbackRecord);
VOID KeEnterCriticalRegion(VOID);
VOID KeFlushIoBuffers(PMDL Mdl, BOOLEAN ReadOperation, BOOLEAN DmaOperation);
KIRQL KeGetCurrentIrql(VOID);
ULONG KeGetCurrentProcessorNumber(VOID);
ULONG KeGetDcacheFillSize(VOID);
PKTHREAD KeGetCurrentThread(VOID);
VOID KeInitializeCallbackRecord(PKBUGCHECK_CALLBACK_RECORD CallbackRecord);
VOID KeInitializeDeviceQueue(PKDEVICE_QUEUE DeviceQueue);
VOID KeInitializeDpc(PKDPC Dpc, PKDEFERRED_ROUTINE DeferredRoutine,
		     PVOID DeferredContext);
VOID KeInitializeEvent(PKEVENT Event, EVENT_TYPE Type, BOOLEAN State);
VOID KeInitializeMutex(PKMUTEX Mutex, ULONG Level);
VOID KeInitializeSemaphore(PKSEMAPHORE Semaphore, LONG Count, LONG Limit);
VOID KeInitializeTimer(PKTIMER Timer);
VOID KeInitializeTimerEx(PKTIMER Timer, TIMER_TYPE Type);
BOOLEAN KeInsertByKeyDeviceQueue(PKDEVICE_QUEUE DeviceQueue,
				 PKDEVICE_QUEUE_ENTRY DeviceQueueEntry,
				 ULONG SortKey);
BOOLEAN KeInsertDeviceQueue(PKDEVICE_QUEUE DeviceQueue,
			    PKDEVICE_QUEUE_ENTRY DeviceQueueEntry);
BOOLEAN KeInsertQueueDpc(PKDPC Dpc, PVOID SystemArgument1, 
			 PVOID SystemArgument2);
VOID KeLeaveCriticalRegion(VOID);   
VOID KeLowerIrql(KIRQL NewIrql);
LARGE_INTEGER KeQueryPerformanceCounter(PLARGE_INTEGER PerformanceFrequency);
VOID KeQuerySystemTime(PLARGE_INTEGER CurrentTime);
VOID KeQueryTickCount(PLARGE_INTEGER TickCount);
ULONG KeQueryTimeIncrement(VOID);
VOID KeRaiseIrql(KIRQL NewIrql, PKIRQL OldIrql);
LONG KeReadStateEvent(PKEVENT Event);
LONG KeReadStateMutex(PKMUTEX Mutex);
LONG KeReadStateSemaphore(PKSEMAPHORE Semaphore);
BOOLEAN KeReadStateTimer(PKTIMER Timer);
BOOLEAN KeRegisterBugCheckCallback(PKBUGCHECK_CALLBACK_RECORD CallbackRecord,
				   PKBUGCHECK_CALLBACK_ROUTINE CallbackRoutine,
				   PVOID Buffer,
				   ULONG Length,
				   PUCHAR Component);
LONG KeReleaseMutex(PKMUTEX Mutex, BOOLEAN Wait);
LONG KeReleaseSemaphore(PKSEMAPHORE Semaphore, KPRIORITY Increment,
			LONG Adjustment, BOOLEAN Wait);
VOID KeReleaseSpinLock(PKSPIN_LOCK Spinlock, KIRQL NewIrql);
VOID KeReleaseSpinLockFromDpcLevel(PKSPIN_LOCK Spinlock);
PKDEVICE_QUEUE_ENTRY KeRemoveByKeyDeviceQueue(PKDEVICE_QUEUE DeviceQueue,
					      ULONG SortKey);
PKDEVICE_QUEUE_ENTRY KeRemoveDeviceQueue(PKDEVICE_QUEUE DeviceQueue);
BOOLEAN KeRemoveEntryDeviceQueue(PKDEVICE_QUEUE DeviceQueue,
				 PKDEVICE_QUEUE_ENTRY DeviceQueueEntry);
BOOLEAN KeRemoveQueueDpc(PKDPC Dpc);
LONG KeResetEvent(PKEVENT Event);
LONG KeSetBasePriorityThread(PKTHREAD Thread, LONG Increment);
LONG KeSetEvent(PKEVENT Event, KPRIORITY Increment, BOOLEAN Wait);
KPRIORITY KeSetPriorityThread(PKTHREAD Thread, KPRIORITY Priority);
BOOLEAN KeSetTimer(PKTIMER Timer, LARGE_INTEGER DueTime, PKDPC Dpc);
BOOLEAN KeSetTimerEx(PKTIMER Timer, LARGE_INTEGER DueTime, LONG Period,
		     PKDPC Dpc);
VOID KeStallExecutionProcessor(ULONG MicroSeconds);
BOOLEAN KeSynchronizeExecution(PKINTERRUPT Interrupt, 
			       PKSYNCHRONIZE_ROUTINE SynchronizeRoutine,
			       PVOID SynchronizeContext);
NTSTATUS KeWaitForMultipleObjects(ULONG Count,
				  PVOID Object[],
				  WAIT_TYPE WaitType,
				  KWAIT_REASON WaitReason,
				  KPROCESSOR_MODE WaitMode,
				  BOOLEAN Alertable,
				  PLARGE_INTEGER Timeout,
				  PKWAIT_BLOCK WaitBlockArray);
NTSTATUS KeWaitForMutexObject(PKMUTEX Mutex, KWAIT_REASON WaitReason,
			      KPROCESSOR_MODE WaitMode, BOOLEAN Alertable,
			      PLARGE_INTEGER Timeout);
NTSTATUS KeWaitForSingleObject(PVOID Object, KWAIT_REASON WaitReason,
			       KPROCESSOR_MODE WaitMode,
			       BOOLEAN Alertable, PLARGE_INTEGER Timeout);
   
/*
 * FUNCTION: Initializes a spinlock
 * ARGUMENTS:
 *        SpinLock = Spinlock to initialize
 */
VOID KeInitializeSpinLock(PKSPIN_LOCK SpinLock);
   
/*
 * FUNCTION: Sets the current irql without altering the current processor 
 * state
 * ARGUMENTS:
 *          newlvl = IRQ level to set
 * NOTE: This is for internal use only
 */
VOID KeSetCurrentIrql(KIRQL newlvl);


/*
 * FUNCTION: Brings the system down in a controlled manner when an 
 * inconsistency that might otherwise cause corruption has been detected
 * ARGUMENTS:
 *           BugCheckCode = Specifies the reason for the bug check
 *           BugCheckParameter[1-4] = Additional information about bug
 * RETURNS: Doesn't
 */
VOID KeBugCheckEx(ULONG BugCheckCode,
		  ULONG BugCheckParameter1,
		  ULONG BugCheckParameter2,
		  ULONG BugCheckParameter3,
		  ULONG BugCheckParameter4);

/*
 * FUNCTION: Brings the system down in a controlled manner when an 
 * inconsistency that might otherwise cause corruption has been detected
 * ARGUMENTS:
 *           BugCheckCode = Specifies the reason for the bug check
 * RETURNS: Doesn't
 */
VOID KeBugCheck(ULONG BugCheckCode);

#endif /* __INCLUDE_DDK_KEFUNCS_H */
