

HEADER("Pointer size"),
SIZE(SizeofPointer, PVOID),

HEADER("Bug Check Codes"),
CONSTANT(APC_INDEX_MISMATCH),
CONSTANT(INVALID_AFFINITY_SET),
CONSTANT(INVALID_DATA_ACCESS_TRAP),
CONSTANT(IRQL_NOT_GREATER_OR_EQUAL),
CONSTANT(IRQL_NOT_LESS_OR_EQUAL), // 0x0a
CONSTANT(NO_USER_MODE_CONTEXT), // 0x0e
CONSTANT(SPIN_LOCK_ALREADY_OWNED), // 0x0f
CONSTANT(SPIN_LOCK_NOT_OWNED), // 0x10
CONSTANT(THREAD_NOT_MUTEX_OWNER), // 0x11
CONSTANT(TRAP_CAUSE_UNKNOWN), // 0x12
CONSTANT(KMODE_EXCEPTION_NOT_HANDLED), // 0x1e
CONSTANT(KERNEL_APC_PENDING_DURING_EXIT), // 0x20
CONSTANT(PANIC_STACK_SWITCH), // 0x2b
CONSTANT(DATA_BUS_ERROR), // 0x2e
CONSTANT(INSTRUCTION_BUS_ERROR), // 0x2f
CONSTANT(SYSTEM_EXIT_OWNED_MUTEX), // 0x39
//CONSTANT(SYSTEM_UNWIND_PREVIOUS_USER), // 0x3a
//CONSTANT(SYSTEM_SERVICE_EXCEPTION), // 0x3b
//CONSTANT(INTERRUPT_UNWIND_ATTEMPTED), // 0x3c
//CONSTANT(INTERRUPT_EXCEPTION_NOT_HANDLED), // 0x3d
CONSTANT(PAGE_FAULT_WITH_INTERRUPTS_OFF), // 0x49
CONSTANT(IRQL_GT_ZERO_AT_SYSTEM_SERVICE), // 0x4a
CONSTANT(DATA_COHERENCY_EXCEPTION), // 0x55
CONSTANT(INSTRUCTION_COHERENCY_EXCEPTION), // 0x56
CONSTANT(HAL1_INITIALIZATION_FAILED), // 0x61
CONSTANT(UNEXPECTED_KERNEL_MODE_TRAP), // 0x7f
CONSTANT(NMI_HARDWARE_FAILURE), // 0x80
CONSTANT(SPIN_LOCK_INIT_FAILURE), // 0x81
CONSTANT(ATTEMPTED_SWITCH_FROM_DPC), // 0xb8
//CONSTANT(MUTEX_ALREADY_OWNED), // 0xbf
//CONSTANT(HARDWARE_INTERRUPT_STORM), // 0xf2
//CONSTANT(RECURSIVE_MACHINE_CHECK), // 0xfb
//CONSTANT(RECURSIVE_NMI), // 0x111
CONSTANT(KERNEL_SECURITY_CHECK_FAILURE), // 0x139
//CONSTANT(UNSUPPORTED_INSTRUCTION_MODE), // 0x151
//CONSTANT(BUGCHECK_CONTEXT_MODIFIER), // 0x80000000

HEADER("Breakpoints"),
CONSTANT(BREAKPOINT_BREAK),
CONSTANT(BREAKPOINT_PRINT),
CONSTANT(BREAKPOINT_PROMPT),
CONSTANT(BREAKPOINT_LOAD_SYMBOLS),
CONSTANT(BREAKPOINT_UNLOAD_SYMBOLS),
CONSTANT(BREAKPOINT_COMMAND_STRING),

HEADER("Context Frame Flags"),
CONSTANT(CONTEXT_FULL),
CONSTANT(CONTEXT_CONTROL),
CONSTANT(CONTEXT_INTEGER),
CONSTANT(CONTEXT_FLOATING_POINT),
CONSTANT(CONTEXT_DEBUG_REGISTERS),
#if defined(_M_IX86) || defined(_M_AMD64)
CONSTANT(CONTEXT_SEGMENTS),
#endif

HEADER("Exception flags"),
CONSTANT(EXCEPTION_NONCONTINUABLE),
CONSTANT(EXCEPTION_UNWINDING),
CONSTANT(EXCEPTION_EXIT_UNWIND),
CONSTANT(EXCEPTION_STACK_INVALID),
CONSTANT(EXCEPTION_NESTED_CALL),
CONSTANT(EXCEPTION_TARGET_UNWIND),
CONSTANT(EXCEPTION_COLLIDED_UNWIND),
CONSTANT(EXCEPTION_UNWIND),
CONSTANT(EXCEPTION_EXECUTE_HANDLER),
CONSTANT(EXCEPTION_CONTINUE_SEARCH),
CONSTANT(EXCEPTION_CONTINUE_EXECUTION),
#ifdef _X86_
CONSTANT(EXCEPTION_CHAIN_END),
//CONSTANT(FIXED_NTVDMSTATE_LINEAR), /// FIXME ???
#endif

HEADER("Exception types"),
CONSTANT(ExceptionContinueExecution),
CONSTANT(ExceptionContinueSearch),
CONSTANT(ExceptionNestedException),
CONSTANT(ExceptionCollidedUnwind),

HEADER("Fast Fail Constants"),
CONSTANT(FAST_FAIL_GUARD_ICALL_CHECK_FAILURE),
//CONSTANT(FAST_FAIL_INVALID_BUFFER_ACCESS),
#ifdef _M_ASM64
CONSTANT(FAST_FAIL_INVALID_JUMP_BUFFER),
CONSTANT(FAST_FAIL_INVALID_SET_OF_CONTEXT),
#endif // _M_ASM64

HEADER("Interrupt object types"),
CONSTANTX(InLevelSensitive, LevelSensitive),
CONSTANTX(InLatched, Latched),

HEADER("IPI"),
#ifndef _M_AMD64
CONSTANT(IPI_APC),
CONSTANT(IPI_DPC),
CONSTANT(IPI_FREEZE),
CONSTANT(IPI_PACKET_READY),
#endif // _M_AMD64
#ifdef _M_IX86
CONSTANT(IPI_SYNCH_REQUEST),
#endif // _M_IX86

HEADER("IRQL"),
CONSTANT(PASSIVE_LEVEL),
CONSTANT(APC_LEVEL),
CONSTANT(DISPATCH_LEVEL),
#ifdef _M_AMD64
CONSTANT(CLOCK_LEVEL),
#elif defined(_M_IX86)
CONSTANT(CLOCK1_LEVEL),
CONSTANT(CLOCK2_LEVEL),
#endif
CONSTANT(IPI_LEVEL),
CONSTANT(POWER_LEVEL),
CONSTANT(PROFILE_LEVEL),
CONSTANT(HIGH_LEVEL),
RAW("#ifdef NT_UP"),
{TYPE_CONSTANT, "SYNCH_LEVEL", DISPATCH_LEVEL},
RAW("#else"),
{TYPE_CONSTANT, "SYNCH_LEVEL", (IPI_LEVEL - 2)},
RAW("#endif"),

#if (NTDDI_VERSION >= NTDDI_VISTA)
HEADER("Entropy Timing Constants"),
CONSTANT(KENTROPY_TIMING_INTERRUPTS_PER_BUFFER),
CONSTANT(KENTROPY_TIMING_BUFFER_MASK),
CONSTANT(KENTROPY_TIMING_ANALYSIS),
#endif

HEADER("Lock Queue"),
CONSTANT(LOCK_QUEUE_WAIT),
CONSTANT(LOCK_QUEUE_OWNER),
CONSTANT(LockQueueDispatcherLock), /// FIXE: obsolete

//HEADER("Performance Definitions"),
//CONSTANT(PERF_CONTEXTSWAP_OFFSET),
//CONSTANT(PERF_CONTEXTSWAP_FLAG),
//CONSTANT(PERF_INTERRUPT_OFFSET),
//CONSTANT(PERF_INTERRUPT_FLAG),
//CONSTANT(PERF_SYSCALL_OFFSET),
//CONSTANT(PERF_SYSCALL_FLAG),
#ifndef _M_ARM
//CONSTANT(PERF_PROFILE_OFFSET), /// FIXE: obsolete
//CONSTANT(PERF_PROFILE_FLAG), /// FIXE: obsolete
//CONSTANT(PERF_SPINLOCK_OFFSET), /// FIXE: obsolete
//CONSTANT(PERF_SPINLOCK_FLAG), /// FIXE: obsolete
#endif
#ifdef _M_IX86
//CONSTANT(PERF_IPI_OFFSET), // 00008H
//CONSTANT(PERF_IPI_FLAG), // 0400000H
//CONSTANT(PERF_IPI), // 040400000H
//CONSTANT(PERF_INTERRUPT), // 020004000H
#endif
//CONSTANT(NTOS_YIELD_MACRO),

HEADER("Process states"),
CONSTANT(ProcessInMemory),
CONSTANT(ProcessOutOfMemory),
CONSTANT(ProcessInTransition),

HEADER("Processor mode"),
CONSTANT(KernelMode),
CONSTANT(UserMode),

HEADER("Service Table Constants"),
CONSTANT(NUMBER_SERVICE_TABLES),
CONSTANT(SERVICE_NUMBER_MASK),
CONSTANT(SERVICE_TABLE_SHIFT),
CONSTANT(SERVICE_TABLE_MASK),
CONSTANT(SERVICE_TABLE_TEST),

HEADER("Status codes"),
CONSTANT(STATUS_ACCESS_VIOLATION),
CONSTANT(STATUS_ASSERTION_FAILURE),
CONSTANT(STATUS_ARRAY_BOUNDS_EXCEEDED),
CONSTANT(STATUS_BAD_COMPRESSION_BUFFER),
CONSTANT(STATUS_BREAKPOINT),
CONSTANT(STATUS_CALLBACK_POP_STACK),
CONSTANT(STATUS_DATATYPE_MISALIGNMENT),
CONSTANT(STATUS_FLOAT_DENORMAL_OPERAND),
CONSTANT(STATUS_FLOAT_DIVIDE_BY_ZERO),
CONSTANT(STATUS_FLOAT_INEXACT_RESULT),
CONSTANT(STATUS_FLOAT_INVALID_OPERATION),
CONSTANT(STATUS_FLOAT_OVERFLOW),
CONSTANT(STATUS_FLOAT_STACK_CHECK),
CONSTANT(STATUS_FLOAT_UNDERFLOW),
CONSTANT(STATUS_FLOAT_MULTIPLE_FAULTS),
CONSTANT(STATUS_FLOAT_MULTIPLE_TRAPS),
CONSTANT(STATUS_GUARD_PAGE_VIOLATION),
CONSTANT(STATUS_ILLEGAL_FLOAT_CONTEXT),
CONSTANT(STATUS_ILLEGAL_INSTRUCTION),
CONSTANT(STATUS_INSTRUCTION_MISALIGNMENT),
CONSTANT(STATUS_INVALID_HANDLE),
CONSTANT(STATUS_INVALID_LOCK_SEQUENCE),
CONSTANT(STATUS_INVALID_OWNER),
CONSTANT(STATUS_INVALID_PARAMETER),
CONSTANT(STATUS_INVALID_PARAMETER_1),
CONSTANT(STATUS_INVALID_SYSTEM_SERVICE),
//CONSTANT(STATUS_INVALID_THREAD),
CONSTANT(STATUS_INTEGER_DIVIDE_BY_ZERO),
CONSTANT(STATUS_INTEGER_OVERFLOW),
CONSTANT(STATUS_IN_PAGE_ERROR),
CONSTANT(STATUS_KERNEL_APC),
CONSTANT(STATUS_LONGJUMP),
CONSTANT(STATUS_NO_CALLBACK_ACTIVE),
#ifndef _M_ARM
CONSTANT(STATUS_NO_EVENT_PAIR), /// FIXME: obsolete
#endif
CONSTANT(STATUS_PRIVILEGED_INSTRUCTION),
CONSTANT(STATUS_SINGLE_STEP),
CONSTANT(STATUS_STACK_BUFFER_OVERRUN),
CONSTANT(STATUS_STACK_OVERFLOW),
CONSTANT(STATUS_SUCCESS),
CONSTANT(STATUS_THREAD_IS_TERMINATING),
CONSTANT(STATUS_TIMEOUT),
CONSTANT(STATUS_UNWIND),
CONSTANT(STATUS_UNWIND_CONSOLIDATE),
CONSTANT(STATUS_USER_APC),
CONSTANT(STATUS_WAKE_SYSTEM),
CONSTANT(STATUS_WAKE_SYSTEM_DEBUGGER),

//HEADER("Thread flags"),
//CONSTANT(THREAD_FLAGS_CYCLE_PROFILING),
//CONSTANT(THREAD_FLAGS_CYCLE_PROFILING_LOCK_BIT),
//CONSTANT(THREAD_FLAGS_CYCLE_PROFILING_LOCK),
//CONSTANT(THREAD_FLAGS_COUNTER_PROFILING),
//CONSTANT(THREAD_FLAGS_COUNTER_PROFILING_LOCK_BIT),
//CONSTANT(THREAD_FLAGS_COUNTER_PROFILING_LOCK),
//CONSTANT(THREAD_FLAGS_CPU_THROTTLED), /// FIXME: obsolete
//CONSTANT(THREAD_FLAGS_CPU_THROTTLED_BIT), /// FIXME: obsolete
//CONSTANT(THREAD_FLAGS_ACCOUNTING_CSWITCH),
//CONSTANT(THREAD_FLAGS_ACCOUNTING_INTERRUPT),
//CONSTANT(THREAD_FLAGS_ACCOUNTING_ANY),
//CONSTANT(THREAD_FLAGS_GROUP_SCHEDULING),
//CONSTANT(THREAD_FLAGS_AFFINITY_SET),
#ifdef _M_IX86
//CONSTANT(THREAD_FLAGS_INSTRUMENTED), // 0x0040
//CONSTANT(THREAD_FLAGS_INSTRUMENTED_PROFILING), // 0x0041
#endif // _M_IX86

HEADER("TLS defines"),
CONSTANT(TLS_MINIMUM_AVAILABLE),
CONSTANT(TLS_EXPANSION_SLOTS),

HEADER("Thread states"),
CONSTANT(Initialized),
CONSTANT(Ready),
CONSTANT(Running),
CONSTANT(Standby),
CONSTANT(Terminated),
CONSTANT(Waiting),
#ifdef _M_ARM
CONSTANT(Transition),
CONSTANT(DeferredReady),
//CONSTANT(GateWaitObsolete),
#endif // _M_ARM

HEADER("Wait type / reason"),
CONSTANT(WrExecutive),
CONSTANT(WrMutex), /// FIXME: Obsolete
CONSTANT(WrDispatchInt),
CONSTANT(WrQuantumEnd), /// FIXME: Obsolete
CONSTANT(WrEventPair), /// FIXME: Obsolete
CONSTANT(WaitAny),
CONSTANT(WaitAll),

HEADER("Stack sizes"),
CONSTANT(KERNEL_STACK_SIZE), /// FIXME: Obsolete
CONSTANT(KERNEL_LARGE_STACK_SIZE),
CONSTANT(KERNEL_LARGE_STACK_COMMIT), /// FIXME: Obsolete
//CONSTANT(DOUBLE_FAULT_STACK_SIZE),
#ifdef _M_AMD64
CONSTANT(KERNEL_MCA_EXCEPTION_STACK_SIZE),
CONSTANT(NMI_STACK_SIZE),
CONSTANT(ISR_STACK_SIZE),
#endif

//CONSTANT(KTHREAD_AUTO_ALIGNMENT_BIT),
//CONSTANT(KTHREAD_GUI_THREAD_MASK),
//CONSTANT(KTHREAD_SYSTEM_THREAD_BIT),
//CONSTANT(KTHREAD_QUEUE_DEFER_PREEMPTION_BIT),

HEADER("Miscellaneous Definitions"),
CONSTANT(TRUE),
CONSTANT(FALSE),
CONSTANT(PAGE_SIZE),
CONSTANT(Executive),
//CONSTANT(BASE_PRIORITY_THRESHOLD),
//CONSTANT(EVENT_PAIR_INCREMENT), /// FIXME: obsolete
CONSTANT(LOW_REALTIME_PRIORITY),
CONSTANT(CLOCK_QUANTUM_DECREMENT),
//CONSTANT(READY_SKIP_QUANTUM),
//CONSTANT(THREAD_QUANTUM),
CONSTANT(WAIT_QUANTUM_DECREMENT),
//CONSTANT(ROUND_TRIP_DECREMENT_COUNT),
CONSTANT(MAXIMUM_PROCESSORS),
CONSTANT(INITIAL_STALL_COUNT),
//CONSTANT(EXCEPTION_EXECUTE_FAULT), // amd64
//CONSTANT(KCACHE_ERRATA_MONITOR_FLAGS), // not arm
//CONSTANT(KI_DPC_ALL_FLAGS),
//CONSTANT(KI_DPC_ANY_DPC_ACTIVE),
//CONSTANT(KI_DPC_INTERRUPT_FLAGS), // 0x2f arm and x64
//CONSTANT(KI_EXCEPTION_GP_FAULT), // not i386
//CONSTANT(KI_EXCEPTION_INVALID_OP), // not i386
//CONSTANT(KI_EXCEPTION_INTEGER_DIVIDE_BY_ZERO), // amd64
CONSTANT(KI_EXCEPTION_ACCESS_VIOLATION),
//CONSTANT(KINTERRUPT_STATE_DISABLED_BIT),
//CONSTANT(KINTERRUPT_STATE_DISABLED),
//CONSTANT(TARGET_FREEZE), // amd64
//CONSTANT(BlackHole), // FIXME: obsolete
CONSTANT(DBG_STATUS_CONTROL_C),
//CONSTANTPTR(USER_SHARED_DATA), // FIXME: we need the kernel mode address here!
//CONSTANT(MM_SHARED_USER_DATA_VA),
//CONSTANT(KERNEL_STACK_CONTROL_LARGE_STACK), // FIXME: obsolete
//CONSTANT(DISPATCH_LENGTH), // FIXME: obsolete
//CONSTANT(MAXIMUM_PRIMARY_VECTOR), // not arm
//CONSTANT(KI_SLIST_FAULT_COUNT_MAXIMUM), // i386
//CONSTANTUSER_CALLBACK_FILTER),

#ifndef _M_ARM
CONSTANT(MAXIMUM_IDTVECTOR),
//CONSTANT(MAXIMUM_PRIMARY_VECTOR),
CONSTANT(PRIMARY_VECTOR_BASE),
CONSTANT(RPL_MASK),
CONSTANT(MODE_MASK),
//MODE_BIT equ 00000H amd64
//LDT_MASK equ 00004H amd64
#endif


/* STRUCTURE OFFSETS *********************************************************/

//HEADER("KAFFINITY_EX"),
//OFFSET(AfCount, KAFFINITY_EX, Count),
//OFFSET(AfBitmap, KAFFINITY_EX, Bitmap),

//HEADER("Aligned Affinity"),
//OFFSET(AfsCpuSet, ???, CpuSet), // FIXME: obsolete

HEADER("KAPC"),
OFFSET(ApType, KAPC, Type),
OFFSET(ApSize, KAPC, Size),
OFFSET(ApThread, KAPC, Thread),
OFFSET(ApApcListEntry, KAPC, ApcListEntry),
OFFSET(ApKernelRoutine, KAPC, KernelRoutine),
OFFSET(ApRundownRoutine, KAPC, RundownRoutine),
OFFSET(ApNormalRoutine, KAPC, NormalRoutine),
OFFSET(ApNormalContext, KAPC, NormalContext),
OFFSET(ApSystemArgument1, KAPC, SystemArgument1),
OFFSET(ApSystemArgument2, KAPC, SystemArgument2),
OFFSET(ApApcStateIndex, KAPC, ApcStateIndex),
OFFSET(ApApcMode, KAPC, ApcMode),
OFFSET(ApInserted, KAPC, Inserted),
SIZE(ApcObjectLength, KAPC),

HEADER("KAPC offsets (relative to NormalRoutine)"),
RELOFFSET(ArNormalRoutine, KAPC, NormalRoutine, NormalRoutine),
RELOFFSET(ArNormalContext, KAPC, NormalContext, NormalRoutine),
RELOFFSET(ArSystemArgument1, KAPC, SystemArgument1, NormalRoutine),
RELOFFSET(ArSystemArgument2, KAPC, SystemArgument2, NormalRoutine),
CONSTANTX(ApcRecordLength, 4 * sizeof(PVOID)),

HEADER("KAPC_STATE"),
OFFSET(AsApcListHead, KAPC_STATE, ApcListHead),
OFFSET(AsProcess, KAPC_STATE, Process),
OFFSET(AsKernelApcInProgress, KAPC_STATE, KernelApcInProgress), // FIXME: obsolete
OFFSET(AsKernelApcPending, KAPC_STATE, KernelApcPending),
OFFSET(AsUserApcPending, KAPC_STATE, UserApcPending),

HEADER("CLIENT_ID"),
OFFSET(CidUniqueProcess, CLIENT_ID, UniqueProcess),
OFFSET(CidUniqueThread, CLIENT_ID, UniqueThread),

HEADER("RTL_CRITICAL_SECTION"),
OFFSET(CsDebugInfo, RTL_CRITICAL_SECTION, DebugInfo),
OFFSET(CsLockCount, RTL_CRITICAL_SECTION, LockCount),
OFFSET(CsRecursionCount, RTL_CRITICAL_SECTION, RecursionCount),
OFFSET(CsOwningThread, RTL_CRITICAL_SECTION, OwningThread),
OFFSET(CsLockSemaphore, RTL_CRITICAL_SECTION, LockSemaphore),
OFFSET(CsSpinCount, RTL_CRITICAL_SECTION, SpinCount),

HEADER("RTL_CRITICAL_SECTION_DEBUG"),
OFFSET(CsType, RTL_CRITICAL_SECTION_DEBUG, Type),
OFFSET(CsCreatorBackTraceIndex, RTL_CRITICAL_SECTION_DEBUG, CreatorBackTraceIndex),
OFFSET(CsCriticalSection, RTL_CRITICAL_SECTION_DEBUG, CriticalSection),
OFFSET(CsProcessLocksList, RTL_CRITICAL_SECTION_DEBUG, ProcessLocksList),
OFFSET(CsEntryCount, RTL_CRITICAL_SECTION_DEBUG, EntryCount),
OFFSET(CsContentionCount, RTL_CRITICAL_SECTION_DEBUG, ContentionCount),

HEADER("KDEVICE_QUEUE_ENTRY"),
OFFSET(DeDeviceListEntry, KDEVICE_QUEUE_ENTRY, DeviceListEntry),
OFFSET(DeSortKey, KDEVICE_QUEUE_ENTRY, SortKey),
OFFSET(DeInserted, KDEVICE_QUEUE_ENTRY, Inserted),
SIZE(DeviceQueueEntryLength, KDEVICE_QUEUE_ENTRY),

HEADER("KDPC"),
OFFSET(DpType, KDPC, Type),
OFFSET(DpImportance, KDPC, Importance),
OFFSET(DpNumber, KDPC, Number),
OFFSET(DpDpcListEntry, KDPC, DpcListEntry),
OFFSET(DpDeferredRoutine, KDPC, DeferredRoutine),
OFFSET(DpDeferredContext, KDPC, DeferredContext),
OFFSET(DpSystemArgument1, KDPC, SystemArgument1),
OFFSET(DpSystemArgument2, KDPC, SystemArgument2),
OFFSET(DpDpcData, KDPC, DpcData),
SIZE(DpcObjectLength, KDPC),

HEADER("KDEVICE_QUEUE"),
OFFSET(DvType, KDEVICE_QUEUE, Type),
OFFSET(DvSize, KDEVICE_QUEUE, Size),
OFFSET(DvDeviceListHead, KDEVICE_QUEUE, DeviceListHead),
OFFSET(DvSpinLock, KDEVICE_QUEUE, Lock),
OFFSET(DvBusy, KDEVICE_QUEUE, Busy),
SIZE(DeviceQueueObjectLength, KDEVICE_QUEUE),

HEADER("EXCEPTION_RECORD"),
OFFSET(ErExceptionCode, EXCEPTION_RECORD, ExceptionCode),
OFFSET(ErExceptionFlags, EXCEPTION_RECORD, ExceptionFlags),
OFFSET(ErExceptionRecord, EXCEPTION_RECORD, ExceptionRecord),
OFFSET(ErExceptionAddress, EXCEPTION_RECORD, ExceptionAddress),
OFFSET(ErNumberParameters, EXCEPTION_RECORD, NumberParameters),
OFFSET(ErExceptionInformation, EXCEPTION_RECORD, ExceptionInformation),
SIZE(ExceptionRecordLength, EXCEPTION_RECORD),
SIZE(EXCEPTION_RECORD_LENGTH, EXCEPTION_RECORD), // not 1386

HEADER("EPROCESS"),
OFFSET(EpDebugPort, EPROCESS, DebugPort),
#if defined(_M_IX86)
OFFSET(EpVdmObjects, EPROCESS, VdmObjects),
#elif defined(_M_AMD64)
OFFSET(EpWow64Process, EPROCESS, Wow64Process),
#endif
SIZE(ExecutiveProcessObjectLength, EPROCESS),

HEADER("ETHREAD offsets"),
OFFSET(EtCid, ETHREAD, Cid), // 0x364
SIZE(ExecutiveThreadObjectLength, ETHREAD), // 0x418

HEADER("KEVENT"),
OFFSET(EvType, KEVENT, Header.Type),
OFFSET(EvSize, KEVENT, Header.Size),
OFFSET(EvSignalState, KEVENT, Header.SignalState),
OFFSET(EvWaitListHead, KEVENT, Header.WaitListHead),
SIZE(EventObjectLength, KEVENT),

HEADER("FIBER"),
OFFSET(FbFiberData, FIBER, FiberData),
OFFSET(FbExceptionList, FIBER, ExceptionList),
OFFSET(FbStackBase, FIBER, StackBase),
OFFSET(FbStackLimit, FIBER, StackLimit),
OFFSET(FbDeallocationStack, FIBER, DeallocationStack),
OFFSET(FbFiberContext, FIBER, FiberContext),
//OFFSET(FbWx86Tib, FIBER, Wx86Tib),
//OFFSET(FbActivationContextStackPointer, FIBER, ActivationContextStackPointer),
OFFSET(FbFlsData, FIBER, FlsData),
OFFSET(FbGuaranteedStackBytes, FIBER, GuaranteedStackBytes),
//OFFSET(FbTebFlags, FIBER, TebFlags),

HEADER("FAST_MUTEX"),
OFFSET(FmCount, FAST_MUTEX, Count),
OFFSET(FmOwner, FAST_MUTEX, Owner),
OFFSET(FmContention, FAST_MUTEX, Contention),
//OFFSET(FmGate, FAST_MUTEX, Gate), // obsolete
OFFSET(FmOldIrql, FAST_MUTEX, OldIrql),

#ifndef _M_ARM
HEADER("GETSETCONTEXT offsets"), // GET_SET_CTX_CONTEXT
OFFSET(GetSetCtxContextPtr, GETSETCONTEXT, Context),
#endif // _M_ARM

HEADER("KINTERRUPT"),
OFFSET(InType, KINTERRUPT, Type),
OFFSET(InSize, KINTERRUPT, Size),
OFFSET(InInterruptListEntry, KINTERRUPT, InterruptListEntry),
OFFSET(InServiceRoutine, KINTERRUPT, ServiceRoutine),
OFFSET(InServiceContext, KINTERRUPT, ServiceContext),
OFFSET(InSpinLock, KINTERRUPT, SpinLock),
OFFSET(InTickCount, KINTERRUPT, TickCount),
OFFSET(InActualLock, KINTERRUPT, ActualLock),
OFFSET(InDispatchAddress, KINTERRUPT, DispatchAddress),
OFFSET(InVector, KINTERRUPT, Vector),
OFFSET(InIrql, KINTERRUPT, Irql),
OFFSET(InSynchronizeIrql, KINTERRUPT, SynchronizeIrql),
OFFSET(InFloatingSave, KINTERRUPT, FloatingSave),
OFFSET(InConnected, KINTERRUPT, Connected),
OFFSET(InNumber, KINTERRUPT, Number),
OFFSET(InShareVector, KINTERRUPT, ShareVector),
//OFFSET(InInternalState, KINTERRUPT, InternalState),
OFFSET(InMode, KINTERRUPT, Mode),
OFFSET(InServiceCount, KINTERRUPT, ServiceCount),
OFFSET(InDispatchCount, KINTERRUPT, DispatchCount),
//OFFSET(InTrapFrame, KINTERRUPT, TrapFrame), // amd64
OFFSET(InDispatchCode, KINTERRUPT, DispatchCode), // obsolete
SIZE(InterruptObjectLength, KINTERRUPT),

#ifdef _M_AMD64
HEADER("IO_STATUS_BLOCK"),
OFFSET(IoStatus, IO_STATUS_BLOCK, Status),
OFFSET(IoPointer, IO_STATUS_BLOCK, Pointer),
OFFSET(IoInformation, IO_STATUS_BLOCK, Information),
#endif /* _M_AMD64 */

#if (NTDDI_VERSION >= NTDDI_WIN8)
HEADER("KSTACK_CONTROL"),
OFFSET(KcCurrentBase, KSTACK_CONTROL, StackBase),
OFFSET(KcActualLimit, KSTACK_CONTROL, ActualLimit),
OFFSET(KcPreviousBase, KSTACK_CONTROL, Previous.StackBase),
OFFSET(KcPreviousLimit, KSTACK_CONTROL, Previous.StackLimit),
OFFSET(KcPreviousKernel, KSTACK_CONTROL, Previous.KernelStack),
OFFSET(KcPreviousInitial, KSTACK_CONTROL, Previous.InitialStack),
#ifdef _IX86
OFFSET(KcTrapFrame, KSTACK_CONTROL, PreviousTrapFrame),
OFFSET(KcExceptionList, KSTACK_CONTROL, PreviousExceptionList),
#endif // _IX86
SIZE(KSTACK_CONTROL_LENGTH, KSTACK_CONTROL),
CONSTANT(KSTACK_ACTUAL_LIMIT_EXPANDED), // move somewhere else?
#else
//HEADER("KERNEL_STACK_CONTROL"),
#endif

#if 0 // no longer in win 10, different struct
HEADER("KNODE"),
//OFFSET(KnRight, KNODE, Right),
//OFFSET(KnLeft, KNODE, Left),
OFFSET(KnPfnDereferenceSListHead, KNODE, PfnDereferenceSListHead),
OFFSET(KnProcessorMask, KNODE, ProcessorMask),
OFFSET(KnColor, KNODE, Color),
OFFSET(KnSeed, KNODE, Seed),
OFFSET(KnNodeNumber, KNODE, NodeNumber),
OFFSET(KnFlags, KNODE, Flags),
OFFSET(KnMmShiftedColor, KNODE, MmShiftedColor),
OFFSET(KnFreeCount, KNODE, FreeCount),
OFFSET(KnPfnDeferredList, KNODE, PfnDeferredList),
SIZE(KNODE_SIZE, KNODE),
#endif

HEADER("KSPIN_LOCK_QUEUE"),
OFFSET(LqNext, KSPIN_LOCK_QUEUE, Next),
OFFSET(LqLock, KSPIN_LOCK_QUEUE, Lock),
SIZE(LOCK_QUEUE_HEADER_SIZE, KSPIN_LOCK_QUEUE),

HEADER("KLOCK_QUEUE_HANDLE"),
OFFSET(LqhLockQueue, KLOCK_QUEUE_HANDLE, LockQueue),
OFFSET(LqhNext, KLOCK_QUEUE_HANDLE, LockQueue.Next),
OFFSET(LqhLock, KLOCK_QUEUE_HANDLE, LockQueue.Lock),
OFFSET(LqhOldIrql, KLOCK_QUEUE_HANDLE, OldIrql),

HEADER("LARGE_INTEGER"),
OFFSET(LiLowPart, LARGE_INTEGER, LowPart),
OFFSET(LiHighPart, LARGE_INTEGER, HighPart),

HEADER("LOADER_PARAMETER_BLOCK (rel. to LoadOrderListHead)"),
RELOFFSET(LpbKernelStack, LOADER_PARAMETER_BLOCK, KernelStack, LoadOrderListHead),
RELOFFSET(LpbPrcb, LOADER_PARAMETER_BLOCK, Prcb, LoadOrderListHead),
RELOFFSET(LpbProcess, LOADER_PARAMETER_BLOCK, Process, LoadOrderListHead),
RELOFFSET(LpbThread, LOADER_PARAMETER_BLOCK, Thread, LoadOrderListHead),

HEADER("LIST_ENTRY"),
OFFSET(LsFlink, LIST_ENTRY, Flink),
OFFSET(LsBlink, LIST_ENTRY, Blink),

HEADER("PEB"),
OFFSET(PeBeingDebugged, PEB, BeingDebugged),
OFFSET(PeProcessParameters, PEB, ProcessParameters),
OFFSET(PeKernelCallbackTable, PEB, KernelCallbackTable),
SIZE(ProcessEnvironmentBlockLength, PEB),

HEADER("KPROFILE"),
OFFSET(PfType, KPROFILE, Type),
OFFSET(PfSize, KPROFILE, Size),
OFFSET(PfProfileListEntry, KPROFILE, ProfileListEntry),
OFFSET(PfProcess, KPROFILE, Process),
OFFSET(PfRangeBase, KPROFILE, RangeBase),
OFFSET(PfRangeLimit, KPROFILE, RangeLimit),
OFFSET(PfBucketShift, KPROFILE, BucketShift),
OFFSET(PfBuffer, KPROFILE, Buffer),
OFFSET(PfSegment, KPROFILE, Segment),
OFFSET(PfAffinity, KPROFILE, Affinity),
OFFSET(PfSource, KPROFILE, Source),
OFFSET(PfStarted, KPROFILE, Started),
SIZE(ProfileObjectLength, KPROFILE),

HEADER("PORT_MESSAGE"), // whole thing obsolete in win10
OFFSET(PmLength, PORT_MESSAGE, u1.Length),
OFFSET(PmZeroInit, PORT_MESSAGE, u2.ZeroInit),
OFFSET(PmClientId, PORT_MESSAGE, ClientId),
OFFSET(PmProcess, PORT_MESSAGE, ClientId.UniqueProcess),
OFFSET(PmThread, PORT_MESSAGE, ClientId.UniqueThread),
OFFSET(PmMessageId, PORT_MESSAGE, MessageId),
OFFSET(PmClientViewSize, PORT_MESSAGE, ClientViewSize),
SIZE(PortMessageLength, PORT_MESSAGE),

HEADER("KPROCESS"),
OFFSET(PrType, KPROCESS, Header.Type),
OFFSET(PrSize, KPROCESS, Header.Size),
OFFSET(PrSignalState, KPROCESS, Header.SignalState),
OFFSET(PrProfileListHead, KPROCESS, ProfileListHead),
OFFSET(PrDirectoryTableBase, KPROCESS, DirectoryTableBase),
#ifdef _M_ARM
//OFFSET(PrPageDirectory, KPROCESS, PageDirectory),
#elif defined(_M_IX86)
OFFSET(PrLdtDescriptor, KPROCESS, LdtDescriptor),
OFFSET(PrInt21Descriptor, KPROCESS, Int21Descriptor),
#endif
OFFSET(PrThreadListHead, KPROCESS, ThreadListHead),
OFFSET(PrAffinity, KPROCESS, Affinity),
OFFSET(PrReadyListHead, KPROCESS, ReadyListHead),
OFFSET(PrSwapListEntry, KPROCESS, SwapListEntry),
OFFSET(PrActiveProcessors, KPROCESS, ActiveProcessors),
OFFSET(PrProcessFlags, KPROCESS, ProcessFlags),
OFFSET(PrBasePriority, KPROCESS, BasePriority),
OFFSET(PrQuantumReset, KPROCESS, QuantumReset),
#if defined(_M_IX86)
OFFSET(PrIopmOffset, KPROCESS, IopmOffset),
#endif
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
OFFSET(PrCycleTime, KPROCESS, CycleTime),
#endif
OFFSET(PrKernelTime, KPROCESS, KernelTime),
OFFSET(PrUserTime, KPROCESS, UserTime),
#if defined(_M_AMD64) || defined(_M_ARM)
//OFFSET(PrInstrumentationCallback, KPROCESS, InstrumentationCallback),
#elif defined(_M_IX86)
OFFSET(PrVdmTrapcHandler, KPROCESS, VdmTrapcHandler),
//OFFSET(PrVdmObjects, KPROCESS, VdmObjects),
OFFSET(PrFlags, KPROCESS, Flags),
//PrInstrumentationCallback equ 0031CH // ???
#endif
SIZE(KernelProcessObjectLength, KPROCESS),

HEADER("KQUEUE"),
OFFSET(QuType, KQUEUE, Header.Type), // not in win10
OFFSET(QuSize, KQUEUE, Header.Size), // not in win10
OFFSET(QuSignalState, KQUEUE, Header.SignalState),
OFFSET(QuEntryListHead, KQUEUE, EntryListHead),
OFFSET(QuCurrentCount, KQUEUE, CurrentCount),
OFFSET(QuMaximumCount, KQUEUE, MaximumCount),
OFFSET(QuThreadListHead, KQUEUE, ThreadListHead),
SIZE(QueueObjectLength, KQUEUE),

HEADER("KSERVICE_TABLE_DESCRIPTOR offsets"),
OFFSET(SdBase, KSERVICE_TABLE_DESCRIPTOR, Base),
OFFSET(SdCount, KSERVICE_TABLE_DESCRIPTOR, Count), // not in win10
OFFSET(SdLimit, KSERVICE_TABLE_DESCRIPTOR, Limit),
OFFSET(SdNumber, KSERVICE_TABLE_DESCRIPTOR, Number),
SIZE(SdLength, KSERVICE_TABLE_DESCRIPTOR),

HEADER("STRING"),
OFFSET(StrLength, STRING, Length),
OFFSET(StrMaximumLength, STRING, MaximumLength),
OFFSET(StrBuffer, STRING, Buffer),

HEADER("TEB"),
#if defined(_M_IX86)
OFFSET(TeExceptionList, TEB, NtTib.ExceptionList),
#elif defined(_M_AMD64)
OFFSET(TeCmTeb, TEB, NtTib),
#endif
OFFSET(TeStackBase, TEB, NtTib.StackBase),
OFFSET(TeStackLimit, TEB, NtTib.StackLimit),
OFFSET(TeFiberData, TEB, NtTib.FiberData),
OFFSET(TeSelf, TEB, NtTib.Self),
OFFSET(TeEnvironmentPointer, TEB, EnvironmentPointer),
OFFSET(TeClientId, TEB, ClientId),
OFFSET(TeActiveRpcHandle, TEB, ActiveRpcHandle),
OFFSET(TeThreadLocalStoragePointer, TEB, ThreadLocalStoragePointer),
OFFSET(TePeb, TEB, ProcessEnvironmentBlock),
OFFSET(TeLastErrorValue, TEB, LastErrorValue),
OFFSET(TeCountOfOwnedCriticalSections, TEB, CountOfOwnedCriticalSections),
OFFSET(TeCsrClientThread, TEB, CsrClientThread),
OFFSET(TeWOW32Reserved, TEB, WOW32Reserved),
//OFFSET(TeSoftFpcr, TEB, SoftFpcr),
OFFSET(TeExceptionCode, TEB, ExceptionCode),
OFFSET(TeActivationContextStackPointer, TEB, ActivationContextStackPointer),
//#if (NTDDI_VERSION >= NTDDI_WIN10)
//OFFSET(TeInstrumentationCallbackSp, TEB, InstrumentationCallbackSp),
//OFFSET(TeInstrumentationCallbackPreviousPc, TEB, InstrumentationCallbackPreviousPc),
//OFFSET(TeInstrumentationCallbackPreviousSp, TEB, InstrumentationCallbackPreviousSp),
//#endif
OFFSET(TeGdiClientPID, TEB, GdiClientPID),
OFFSET(TeGdiClientTID, TEB, GdiClientTID),
OFFSET(TeGdiThreadLocalInfo, TEB, GdiThreadLocalInfo),
OFFSET(TeglDispatchTable, TEB, glDispatchTable),
OFFSET(TeglReserved1, TEB, glReserved1),
OFFSET(TeglReserved2, TEB, glReserved2),
OFFSET(TeglSectionInfo, TEB, glSectionInfo),
OFFSET(TeglSection, TEB, glSection),
OFFSET(TeglTable, TEB, glTable),
OFFSET(TeglCurrentRC, TEB, glCurrentRC),
OFFSET(TeglContext, TEB, glContext),
OFFSET(TeDeallocationStack, TEB, DeallocationStack),
OFFSET(TeTlsSlots, TEB, TlsSlots),
OFFSET(TeVdm, TEB, Vdm),
OFFSET(TeInstrumentation, TEB, Instrumentation),
OFFSET(TeGdiBatchCount, TEB, GdiBatchCount),
OFFSET(TeGuaranteedStackBytes, TEB, GuaranteedStackBytes),
OFFSET(TeTlsExpansionSlots, TEB, TlsExpansionSlots),
OFFSET(TeFlsData, TEB, FlsData),
SIZE(ThreadEnvironmentBlockLength, TEB),

HEADER("TIME_FIELDS"),
OFFSET(TfYear, TIME_FIELDS, Year),
OFFSET(TfMonth, TIME_FIELDS, Month),
OFFSET(TfDay, TIME_FIELDS, Day),
OFFSET(TfHour, TIME_FIELDS, Hour),
OFFSET(TfMinute, TIME_FIELDS, Minute),
OFFSET(TfSecond, TIME_FIELDS, Second),
OFFSET(TfMilliseconds, TIME_FIELDS, Milliseconds),
OFFSET(TfWeekday, TIME_FIELDS, Weekday),

HEADER("KTHREAD"),
OFFSET(ThType, KTHREAD, Header.Type),
OFFSET(ThLock, KTHREAD, Header.Lock),
OFFSET(ThSize, KTHREAD, Header.Size),
OFFSET(ThThreadControlFlags, KTHREAD, Header.ThreadControlFlags),
OFFSET(ThDebugActive, KTHREAD, Header.DebugActive),
OFFSET(ThSignalState, KTHREAD, Header.SignalState),
OFFSET(ThInitialStack, KTHREAD, InitialStack),
OFFSET(ThStackLimit, KTHREAD, StackLimit),
OFFSET(ThStackBase, KTHREAD, StackBase),
OFFSET(ThThreadLock, KTHREAD, ThreadLock),
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
OFFSET(ThCycleTime, KTHREAD, CycleTime),
#if defined(_M_IX86)
OFFSET(ThHighCycleTime, KTHREAD, HighCycleTime),
#endif
#endif /* (NTDDI_VERSION >= NTDDI_LONGHORN) */
#if defined(_M_IX86)
OFFSET(ThServiceTable, KTHREAD, ServiceTable),
#endif
//OFFSET(ThCurrentRunTime, KTHREAD, CurrentRunTime),
//OFFSET(ThStateSaveArea, KTHREAD, StateSaveArea), // 0x3C not arm
OFFSET(ThKernelStack, KTHREAD, KernelStack),
#if (NTDDI_VERSION >= NTDDI_WIN7)
OFFSET(ThRunning, KTHREAD, Running),
#endif /* (NTDDI_VERSION >= NTDDI_WIN7) */
OFFSET(ThAlerted, KTHREAD, Alerted),
#if (NTDDI_VERSION >= NTDDI_WIN7)
OFFSET(ThMiscFlags, KTHREAD, MiscFlags),
#endif /* (NTDDI_VERSION >= NTDDI_WIN7) */
OFFSET(ThThreadFlags, KTHREAD, ThreadFlags),
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
OFFSET(ThSystemCallNumber, KTHREAD, SystemCallNumber),
#endif /* (NTDDI_VERSION >= NTDDI_LONGHORN) */
//OFFSET(ThFirstArgument, KTHREAD, FirstArgument),
OFFSET(ThTrapFrame, KTHREAD, TrapFrame),
OFFSET(ThApcState, KTHREAD, ApcState),
OFFSET(ThPriority, KTHREAD, Priority),
OFFSET(ThContextSwitches, KTHREAD, ContextSwitches),
OFFSET(ThState, KTHREAD, State),
OFFSET(ThNpxState, KTHREAD, NpxState),
OFFSET(ThWaitIrql, KTHREAD, WaitIrql),
OFFSET(ThWaitMode, KTHREAD, WaitMode),
OFFSET(ThTeb, KTHREAD, Teb),
OFFSET(ThTimer, KTHREAD, Timer),
OFFSET(ThWin32Thread, KTHREAD, Win32Thread),
OFFSET(ThWaitTime, KTHREAD, WaitTime),
OFFSET(ThCombinedApcDisable, KTHREAD, CombinedApcDisable),
OFFSET(ThKernelApcDisable, KTHREAD, KernelApcDisable),
OFFSET(ThSpecialApcDisable, KTHREAD, SpecialApcDisable),
#if defined(_M_ARM)
//OFFSET(ThVfpState, KTHREAD, VfpState),
#endif
OFFSET(ThNextProcessor, KTHREAD, NextProcessor),
OFFSET(ThProcess, KTHREAD, Process),
OFFSET(ThPreviousMode, KTHREAD, PreviousMode),
OFFSET(ThPriorityDecrement, KTHREAD, PriorityDecrement),
OFFSET(ThAdjustReason, KTHREAD, AdjustReason),
OFFSET(ThAdjustIncrement, KTHREAD, AdjustIncrement),
OFFSET(ThAffinity, KTHREAD, Affinity),
OFFSET(ThApcStateIndex, KTHREAD, ApcStateIndex),
OFFSET(ThIdealProcessor, KTHREAD, IdealProcessor),
OFFSET(ThApcStatePointer, KTHREAD, ApcStatePointer),
OFFSET(ThSavedApcState, KTHREAD, SavedApcState),
OFFSET(ThWaitReason, KTHREAD, WaitReason),
OFFSET(ThSaturation, KTHREAD, Saturation),
OFFSET(ThLegoData, KTHREAD, LegoData),
//#if defined(_M_ARM) && (NTDDI_VERSION >= NTDDI_WIN10)
//#define ThUserRoBase 0x434
//#define ThUserRwBase 0x438
//#endif
#ifdef _M_IX86
OFFSET(ThSListFaultCount, KTHREAD, WaitReason), // 0x18E
OFFSET(ThSListFaultAddress, KTHREAD, WaitReason), // 0x10
#endif // _M_IX86
#if defined(_M_IX86) || defined(_M_AMD64)
OFFSET(ThUserFsBase, KTHREAD, WaitReason), // 0x434
OFFSET(ThUserGsBase, KTHREAD, WaitReason), // 0x438
#endif // defined
SIZE(KernelThreadObjectLength, KTHREAD),

HEADER("KTIMER"),
OFFSET(TiType, KTIMER, Header.Type),
OFFSET(TiSize, KTIMER, Header.Size),
OFFSET(TiInserted, KTIMER, Header.Inserted), // not in win 10
OFFSET(TiSignalState, KTIMER, Header.SignalState),
OFFSET(TiDueTime, KTIMER, DueTime),
OFFSET(TiTimerListEntry, KTIMER, TimerListEntry),
OFFSET(TiDpc, KTIMER, Dpc),
OFFSET(TiPeriod, KTIMER, Period),
SIZE(TimerObjectLength, KTIMER),

HEADER("TIME"),
OFFSET(TmLowTime, TIME, LowTime),
OFFSET(TmHighTime, TIME, HighTime),

HEADER("SYSTEM_CONTEXT_SWITCH_INFORMATION (relative to FindAny)"),
RELOFFSET(TwFindAny, SYSTEM_CONTEXT_SWITCH_INFORMATION, FindAny, FindAny),
RELOFFSET(TwFindIdeal, SYSTEM_CONTEXT_SWITCH_INFORMATION, FindIdeal, FindAny),
RELOFFSET(TwFindLast, SYSTEM_CONTEXT_SWITCH_INFORMATION, FindLast, FindAny),
RELOFFSET(TwIdleAny, SYSTEM_CONTEXT_SWITCH_INFORMATION, IdleAny, FindAny),
RELOFFSET(TwIdleCurrent, SYSTEM_CONTEXT_SWITCH_INFORMATION, IdleCurrent, FindAny),
RELOFFSET(TwIdleIdeal, SYSTEM_CONTEXT_SWITCH_INFORMATION, IdleIdeal, FindAny),
RELOFFSET(TwIdleLast, SYSTEM_CONTEXT_SWITCH_INFORMATION, IdleLast, FindAny),
RELOFFSET(TwPreemptAny, SYSTEM_CONTEXT_SWITCH_INFORMATION, PreemptAny, FindAny),
RELOFFSET(TwPreemptCurrent, SYSTEM_CONTEXT_SWITCH_INFORMATION, PreemptCurrent, FindAny),
RELOFFSET(TwPreemptLast, SYSTEM_CONTEXT_SWITCH_INFORMATION, PreemptLast, FindAny),
RELOFFSET(TwSwitchToIdle, SYSTEM_CONTEXT_SWITCH_INFORMATION, SwitchToIdle, FindAny),

HEADER("KUSER_SHARED_DATA"),
OFFSET(UsTickCountMultiplier, KUSER_SHARED_DATA, TickCountMultiplier), // 0x4
OFFSET(UsInterruptTime, KUSER_SHARED_DATA, InterruptTime), // 0x8
OFFSET(UsSystemTime, KUSER_SHARED_DATA, SystemTime), // 0x14
OFFSET(UsTimeZoneBias, KUSER_SHARED_DATA, TimeZoneBias), // 0x20
OFFSET(UsImageNumberLow, KUSER_SHARED_DATA, ImageNumberLow),
OFFSET(UsImageNumberHigh, KUSER_SHARED_DATA, ImageNumberHigh),
OFFSET(UsNtSystemRoot, KUSER_SHARED_DATA, NtSystemRoot),
OFFSET(UsMaxStackTraceDepth, KUSER_SHARED_DATA, MaxStackTraceDepth),
OFFSET(UsCryptoExponent, KUSER_SHARED_DATA, CryptoExponent),
OFFSET(UsTimeZoneId, KUSER_SHARED_DATA, TimeZoneId),
OFFSET(UsLargePageMinimum, KUSER_SHARED_DATA, LargePageMinimum),
//#if (NTDDI_VERSION >= NTDDI_WIN10)
//OFFSET(UsNtBuildNumber, KUSER_SHARED_DATA, NtBuildNumber),
//#else
OFFSET(UsReserved2, KUSER_SHARED_DATA, Reserved2),
//#endif
OFFSET(UsNtProductType, KUSER_SHARED_DATA, NtProductType),
OFFSET(UsProductTypeIsValid, KUSER_SHARED_DATA, ProductTypeIsValid),
OFFSET(UsNtMajorVersion, KUSER_SHARED_DATA, NtMajorVersion),
OFFSET(UsNtMinorVersion, KUSER_SHARED_DATA, NtMinorVersion),
OFFSET(UsProcessorFeatures, KUSER_SHARED_DATA, ProcessorFeatures),
OFFSET(UsReserved1, KUSER_SHARED_DATA, Reserved1),
OFFSET(UsReserved3, KUSER_SHARED_DATA, Reserved3),
OFFSET(UsTimeSlip, KUSER_SHARED_DATA, TimeSlip),
OFFSET(UsAlternativeArchitecture, KUSER_SHARED_DATA, AlternativeArchitecture),
OFFSET(UsSystemExpirationDate, KUSER_SHARED_DATA, SystemExpirationDate), // not arm
OFFSET(UsSuiteMask, KUSER_SHARED_DATA, SuiteMask),
OFFSET(UsKdDebuggerEnabled, KUSER_SHARED_DATA, KdDebuggerEnabled),
OFFSET(UsActiveConsoleId, KUSER_SHARED_DATA, ActiveConsoleId),
OFFSET(UsDismountCount, KUSER_SHARED_DATA, DismountCount),
OFFSET(UsComPlusPackage, KUSER_SHARED_DATA, ComPlusPackage),
OFFSET(UsLastSystemRITEventTickCount, KUSER_SHARED_DATA, LastSystemRITEventTickCount),
OFFSET(UsNumberOfPhysicalPages, KUSER_SHARED_DATA, NumberOfPhysicalPages),
OFFSET(UsSafeBootMode, KUSER_SHARED_DATA, SafeBootMode),
OFFSET(UsTestRetInstruction, KUSER_SHARED_DATA, TestRetInstruction),
OFFSET(UsSystemCall, KUSER_SHARED_DATA, SystemCall), // not in win10
OFFSET(UsSystemCallReturn, KUSER_SHARED_DATA, SystemCallReturn), // not in win10
OFFSET(UsSystemCallPad, KUSER_SHARED_DATA, SystemCallPad),
OFFSET(UsTickCount, KUSER_SHARED_DATA, TickCount),
OFFSET(UsTickCountQuad, KUSER_SHARED_DATA, TickCountQuad),
OFFSET(UsWow64SharedInformation, KUSER_SHARED_DATA, Wow64SharedInformation), // not in win10
//OFFSET(UsXState, KUSER_SHARED_DATA, XState), // win 10

HEADER("KWAIT_BLOCK offsets"),
OFFSET(WbWaitListEntry, KWAIT_BLOCK, WaitListEntry),
OFFSET(WbThread, KWAIT_BLOCK, Thread),
OFFSET(WbObject, KWAIT_BLOCK, Object),
OFFSET(WbNextWaitBlock, KWAIT_BLOCK, NextWaitBlock), // not in win10
OFFSET(WbWaitKey, KWAIT_BLOCK, WaitKey),
OFFSET(WbWaitType, KWAIT_BLOCK, WaitType),


#if 0
//OFFSET(IbCfgBitMap, ????, CfgBitMap),
CONSTANT(Win32BatchFlushCallout 0x7


#define CmThreadEnvironmentBlockOffset 0x1000

;  Process Parameters Block Structure Offset Definitions
#define PpFlags 0x8


// Extended context structure offset definitions
#define CxxLegacyOffset 0x8
#define CxxLegacyLength 0xc
#define CxxXStateOffset 0x10
#define CxxXStateLength 0x14

#ifndef _M_ARM
;  Bounds Callback Status Code Definitions
BoundExceptionContinueSearch equ 00000H
BoundExceptionHandled equ 00001H
BoundExceptionError equ 00002H
#endif

#ifndef _M_ARM
;  Enlightenment structure definitions
HeEnlightenments equ 00000H
HeHypervisorConnected equ 00004H
HeEndOfInterrupt equ 00008H
HeApicWriteIcr equ 0000CH
HeSpinCountMask equ 00014H
HeLongSpinWait equ 00018H
#endif

// KAFFINITY_EX
#define AffinityExLength 0xc // not i386

#endif
