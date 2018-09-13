/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    genxx.inc

Abstract:

    This file contains common code to generate assembler definitions.

Author:

    David N. Cutler (davec) 9-Aug-1995

Revision History:

    Forrest C. Foltz (forrestf) 26-Jan-1998

        Modified for use with genxx.exe utility

--*/

    //
    // Size of a pointer in bytes.
    //

    genCom("Pointer size in bytes")

    genVal(SizeofPointer, sizeof(PVOID))

    //
    // Process state enumerated type definitions.
    //

    genCom("Process State Enumerated Type Values")

    genVal(ProcessInMemory, ProcessInMemory)
    genVal(ProcessOutOfMemory, ProcessOutOfMemory)
    genVal(ProcessInTransition, ProcessInTransition)

    //
    // Thread state enumerated type definitions.
    //

    genCom("Thread State Enumerated Type Values")

    genVal(Initialized, Initialized)
    genVal(Ready, Ready)
    genVal(Running, Running)
    genVal(Standby, Standby)
    genVal(Terminated, Terminated)
    genVal(Waiting, Waiting)

    //
    // Wait reason and wait type enumerated type definitions.
    //

    EnableInc(HAL)

    genCom("Wait Reason and Wait Type Enumerated Type Values")

    genVal(WrExecutive, Executive)

    DisableInc(HAL)

    genVal(WrEventPair, WrEventPair)
    genVal(WaitAny, WaitAny)
    genVal(WaitAll, WaitAll)

    //
    // APC state structure offset definitions.
    //

    genCom("Apc State Structure Offset Definitions")

    genDef(As, KAPC_STATE, ApcListHead)
    genDef(As, KAPC_STATE, Process)
    genDef(As, KAPC_STATE, KernelApcInProgress)
    genDef(As, KAPC_STATE, KernelApcPending)
    genDef(As, KAPC_STATE, UserApcPending)

    //
    // Bug check code definitions
    //

    EnableInc(HAL)

    genCom("Bug Check Code Definitions")

    genVal(APC_INDEX_MISMATCH, APC_INDEX_MISMATCH)
    genVal(ATTEMPTED_SWITCH_FROM_DPC, ATTEMPTED_SWITCH_FROM_DPC)
    genVal(DATA_BUS_ERROR, DATA_BUS_ERROR)
    genVal(DATA_COHERENCY_EXCEPTION, DATA_COHERENCY_EXCEPTION)
    genVal(HAL1_INITIALIZATION_FAILED, HAL1_INITIALIZATION_FAILED)
    genVal(INSTRUCTION_BUS_ERROR, INSTRUCTION_BUS_ERROR)
    genVal(INSTRUCTION_COHERENCY_EXCEPTION, INSTRUCTION_COHERENCY_EXCEPTION)
    genVal(INTERRUPT_EXCEPTION_NOT_HANDLED, INTERRUPT_EXCEPTION_NOT_HANDLED)
    genVal(INTERRUPT_UNWIND_ATTEMPTED, INTERRUPT_UNWIND_ATTEMPTED)
    genVal(INVALID_AFFINITY_SET, INVALID_AFFINITY_SET)
    genVal(INVALID_DATA_ACCESS_TRAP, INVALID_DATA_ACCESS_TRAP)
    genVal(IRQL_GT_ZERO_AT_SYSTEM_SERVICE, IRQL_GT_ZERO_AT_SYSTEM_SERVICE)
    genVal(IRQL_NOT_LESS_OR_EQUAL, IRQL_NOT_LESS_OR_EQUAL)
    genVal(KMODE_EXCEPTION_NOT_HANDLED, KMODE_EXCEPTION_NOT_HANDLED)
    genVal(NMI_HARDWARE_FAILURE, NMI_HARDWARE_FAILURE)
    genVal(NO_USER_MODE_CONTEXT, NO_USER_MODE_CONTEXT)
    genVal(PAGE_FAULT_WITH_INTERRUPTS_OFF, PAGE_FAULT_WITH_INTERRUPTS_OFF)
    genVal(PANIC_STACK_SWITCH, PANIC_STACK_SWITCH)
    genVal(SPIN_LOCK_INIT_FAILURE, SPIN_LOCK_INIT_FAILURE)
    genVal(SYSTEM_EXIT_OWNED_MUTEX, SYSTEM_EXIT_OWNED_MUTEX)
    genVal(SYSTEM_SERVICE_EXCEPTION, SYSTEM_SERVICE_EXCEPTION)
    genVal(SYSTEM_UNWIND_PREVIOUS_USER, SYSTEM_UNWIND_PREVIOUS_USER)
    genVal(TRAP_CAUSE_UNKNOWN, TRAP_CAUSE_UNKNOWN)
    genVal(UNEXPECTED_KERNEL_MODE_TRAP, UNEXPECTED_KERNEL_MODE_TRAP)

    DisableInc(HAL)

    //
    // Breakpoint types
    //

    EnableInc(HAL)

    genCom("Breakpoint type definitions")

    genVal(DBG_STATUS_CONTROL_C, DBG_STATUS_CONTROL_C)

    DisableInc(HAL)

    //
    // Client Id structure offset definitions.
    //

    genCom("Client Id Structure Offset Definitions")

    genDef(Cid, CLIENT_ID, UniqueProcess)
    genDef(Cid, CLIENT_ID, UniqueThread)

    //
    // Critical section structure offset definitions.
    //

    genCom("Critical Section Structure Offset Definitions")

    genDef(Cs, RTL_CRITICAL_SECTION, DebugInfo)
    genDef(Cs, RTL_CRITICAL_SECTION, LockCount)
    genDef(Cs, RTL_CRITICAL_SECTION, RecursionCount)
    genDef(Cs, RTL_CRITICAL_SECTION, OwningThread)
    genDef(Cs, RTL_CRITICAL_SECTION, LockSemaphore)
    genDef(Cs, RTL_CRITICAL_SECTION, SpinCount)

    //
    // Critical section debug information structure offset definitions.
    //

    genCom("Critical Section Debug Information Structure Offset Definitions")

    genDef(Cs, RTL_CRITICAL_SECTION_DEBUG, Type)
    genDef(Cs, RTL_CRITICAL_SECTION_DEBUG, CreatorBackTraceIndex)
    genDef(Cs, RTL_CRITICAL_SECTION_DEBUG, CriticalSection)
    genDef(Cs, RTL_CRITICAL_SECTION_DEBUG, ProcessLocksList)
    genDef(Cs, RTL_CRITICAL_SECTION_DEBUG, EntryCount)
    genDef(Cs, RTL_CRITICAL_SECTION_DEBUG, ContentionCount)

    //
    // Exception dispatcher context structure offset definitions.
    //

#if defined(_ALPHA_) || defined(_MIPS_) || defined(_PPC_) || defined(_IA64_)

    genCom("Dispatcher Context Structure Offset Definitions")

    genDef(Dc, DISPATCHER_CONTEXT, ControlPc)
    genDef(Dc, DISPATCHER_CONTEXT, FunctionEntry)
    genDef(Dc, DISPATCHER_CONTEXT, EstablisherFrame)
    genDef(Dc, DISPATCHER_CONTEXT, ContextRecord)

#endif

    //
    // Exception record offset, flag, and enumerated type definitions.
    //

    EnableInc(HAL)

    genCom("Exception Record Offset, Flag, and Enumerated Type Definitions")

    genVal(EXCEPTION_NONCONTINUABLE, EXCEPTION_NONCONTINUABLE)
    genVal(EXCEPTION_UNWINDING, EXCEPTION_UNWINDING)
    genVal(EXCEPTION_EXIT_UNWIND, EXCEPTION_EXIT_UNWIND)
    genVal(EXCEPTION_STACK_INVALID, EXCEPTION_STACK_INVALID)
    genVal(EXCEPTION_NESTED_CALL, EXCEPTION_NESTED_CALL)
    genVal(EXCEPTION_TARGET_UNWIND, EXCEPTION_TARGET_UNWIND)
    genVal(EXCEPTION_COLLIDED_UNWIND, EXCEPTION_COLLIDED_UNWIND)
    genVal(EXCEPTION_UNWIND, EXCEPTION_UNWIND)
    genVal(EXCEPTION_EXECUTE_HANDLER, EXCEPTION_EXECUTE_HANDLER)
    genVal(EXCEPTION_CONTINUE_SEARCH, EXCEPTION_CONTINUE_SEARCH)
    genVal(EXCEPTION_CONTINUE_EXECUTION, EXCEPTION_CONTINUE_EXECUTION)

#if defined(_X86_)

    genVal(EXCEPTION_CHAIN_END, (ULONG)EXCEPTION_CHAIN_END)

#endif

    genSpc()

    genVal(ExceptionContinueExecution, ExceptionContinueExecution)
    genVal(ExceptionContinueSearch, ExceptionContinueSearch)
    genVal(ExceptionNestedException, ExceptionNestedException)
    genVal(ExceptionCollidedUnwind, ExceptionCollidedUnwind)

    genSpc()

    genDef(Er, EXCEPTION_RECORD, ExceptionCode)
    genDef(Er, EXCEPTION_RECORD, ExceptionFlags)
    genDef(Er, EXCEPTION_RECORD, ExceptionRecord)
    genDef(Er, EXCEPTION_RECORD, ExceptionAddress)
    genDef(Er, EXCEPTION_RECORD, NumberParameters)
    genDef(Er, EXCEPTION_RECORD, ExceptionInformation)
    genVal(ExceptionRecordLength, (sizeof(EXCEPTION_RECORD) + 15) & (~15))

    DisableInc(HAL)

    //
    // Fast Mutex structure offset definitions.
    //

    EnableInc(HAL)

    genCom("Fast Mutex Structure Offset Definitions")

    genDef(Fm, FAST_MUTEX, Count)
    genDef(Fm, FAST_MUTEX, Owner)
    genDef(Fm, FAST_MUTEX, Contention)
    genDef(Fm, FAST_MUTEX, Event)
    genDef(Fm, FAST_MUTEX, OldIrql)

    //
    // Interrupt priority request level definitions
    //

    genCom("Interrupt Priority Request Level Definitions")

    genVal(APC_LEVEL, APC_LEVEL)
    genVal(DISPATCH_LEVEL, DISPATCH_LEVEL)

#if defined(_X86_)

    genVal(CLOCK1_LEVEL, CLOCK1_LEVEL)
    genVal(CLOCK2_LEVEL, CLOCK2_LEVEL)

#endif

    genVal(IPI_LEVEL, IPI_LEVEL)
    genVal(POWER_LEVEL, POWER_LEVEL)
    genVal(PROFILE_LEVEL, PROFILE_LEVEL)
    genVal(HIGH_LEVEL, HIGH_LEVEL)
    genVal(SYNCH_LEVEL, SYNCH_LEVEL)

    //
    // Large integer structure offset definitions.
    //

    genCom("Large Integer Structure Offset Definitions")

    genDef(Li, LARGE_INTEGER, LowPart)
    genDef(Li, LARGE_INTEGER, HighPart)

    //
    // List entry structure offset definitions.
    //

    genCom("List Entry Structure Offset Definitions")

    genDef(Ls, LIST_ENTRY, Flink)
    genDef(Ls, LIST_ENTRY, Blink)

    //
    // String structure offset definitions.
    //

    genCom("String Structure Offset Definitions")

    genDef(Str, STRING, Length)
    genDef(Str, STRING, MaximumLength)
    genDef(Str, STRING, Buffer)

    //
    // System time structure offset definitions.
    //

#if defined(_MIPS_) || defined(_PPC_) || defined(_X86_)

    genCom("System Time Structure Offset Definitions")

    genAlt(StLowTime, KSYSTEM_TIME, LowPart)
    genDef(St, KSYSTEM_TIME, High1Time)
    genDef(St, KSYSTEM_TIME, High2Time)

#endif

    //
    // Time structure offset definitions.
    //

    genCom("Time Structure Offset Definitions")

    genAlt(TmLowTime, LARGE_INTEGER, LowPart)
    genAlt(TmHighTime, LARGE_INTEGER , HighPart)

    DisableInc(HAL)

    //
    // Thread switch counter structure offset definitions.
    //

    genCom("Thread Switch Counter Offset Definitions")

    genDef(Tw, KTHREAD_SWITCH_COUNTERS, FindAny)
    genDef(Tw, KTHREAD_SWITCH_COUNTERS, FindIdeal)
    genDef(Tw, KTHREAD_SWITCH_COUNTERS, FindLast)
    genDef(Tw, KTHREAD_SWITCH_COUNTERS, IdleAny)
    genDef(Tw, KTHREAD_SWITCH_COUNTERS, IdleCurrent)
    genDef(Tw, KTHREAD_SWITCH_COUNTERS, IdleIdeal)
    genDef(Tw, KTHREAD_SWITCH_COUNTERS, IdleLast)
    genDef(Tw, KTHREAD_SWITCH_COUNTERS, PreemptAny)
    genDef(Tw, KTHREAD_SWITCH_COUNTERS, PreemptCurrent)
    genDef(Tw, KTHREAD_SWITCH_COUNTERS, PreemptLast)
    genDef(Tw, KTHREAD_SWITCH_COUNTERS, SwitchToIdle)

    //
    // Status code definitions
    //

    genCom("Status Code Definitions")

#if defined(_ALPHA_)

    genVal(STATUS_ALPHA_ARITHMETIC_EXCEPTION, STATUS_ALPHA_ARITHMETIC_EXCEPTION)
    genVal(STATUS_ALPHA_BAD_VIRTUAL_ADDRESS, STATUS_ALPHA_BAD_VIRTUAL_ADDRESS)
    genVal(STATUS_ALPHA_FLOATING_NOT_IMPLEMENTED, STATUS_ALPHA_FLOATING_NOT_IMPLEMENTED)
    genVal(STATUS_ALPHA_GENTRAP, STATUS_ALPHA_GENTRAP)
    genVal(STATUS_ALPHA_MACHINE_CHECK, (DATA_BUS_ERROR | 0xdfff0000))

#endif

#if defined(_IA64_)
    genVal(STATUS_IA64_INVALID_STACK, STATUS_IA64_INVALID_STACK)
#endif

    genVal(STATUS_ACCESS_VIOLATION, STATUS_ACCESS_VIOLATION)
    genVal(STATUS_ARRAY_BOUNDS_EXCEEDED, STATUS_ARRAY_BOUNDS_EXCEEDED)
    genVal(STATUS_BAD_COMPRESSION_BUFFER, STATUS_BAD_COMPRESSION_BUFFER)
    genVal(STATUS_BREAKPOINT, STATUS_BREAKPOINT)
    genVal(STATUS_DATATYPE_MISALIGNMENT, STATUS_DATATYPE_MISALIGNMENT)
    genVal(STATUS_FLOAT_DENORMAL_OPERAND, STATUS_FLOAT_DENORMAL_OPERAND)
    genVal(STATUS_FLOAT_DIVIDE_BY_ZERO, STATUS_FLOAT_DIVIDE_BY_ZERO)
    genVal(STATUS_FLOAT_INEXACT_RESULT, STATUS_FLOAT_INEXACT_RESULT)
    genVal(STATUS_FLOAT_INVALID_OPERATION, STATUS_FLOAT_INVALID_OPERATION)
    genVal(STATUS_FLOAT_OVERFLOW, STATUS_FLOAT_OVERFLOW)
    genVal(STATUS_FLOAT_STACK_CHECK, STATUS_FLOAT_STACK_CHECK)
    genVal(STATUS_FLOAT_UNDERFLOW, STATUS_FLOAT_UNDERFLOW)
    genVal(STATUS_FLOAT_MULTIPLE_FAULTS, STATUS_FLOAT_MULTIPLE_FAULTS)
    genVal(STATUS_FLOAT_MULTIPLE_TRAPS, STATUS_FLOAT_MULTIPLE_TRAPS)
    genVal(STATUS_GUARD_PAGE_VIOLATION, STATUS_GUARD_PAGE_VIOLATION)
    genVal(STATUS_ILLEGAL_FLOAT_CONTEXT, STATUS_ILLEGAL_FLOAT_CONTEXT)
    genVal(STATUS_ILLEGAL_INSTRUCTION, STATUS_ILLEGAL_INSTRUCTION)
    genVal(STATUS_INSTRUCTION_MISALIGNMENT, STATUS_INSTRUCTION_MISALIGNMENT)
    genVal(STATUS_INVALID_HANDLE, STATUS_INVALID_HANDLE)
    genVal(STATUS_INVALID_LOCK_SEQUENCE, STATUS_INVALID_LOCK_SEQUENCE)
    genVal(STATUS_INVALID_OWNER, STATUS_INVALID_OWNER)
    genVal(STATUS_INVALID_PARAMETER_1, STATUS_INVALID_PARAMETER_1)
    genVal(STATUS_INVALID_SYSTEM_SERVICE, STATUS_INVALID_SYSTEM_SERVICE)
    genVal(STATUS_INTEGER_DIVIDE_BY_ZERO, STATUS_INTEGER_DIVIDE_BY_ZERO)
    genVal(STATUS_INTEGER_OVERFLOW, STATUS_INTEGER_OVERFLOW)
    genVal(STATUS_IN_PAGE_ERROR, STATUS_IN_PAGE_ERROR)
    genVal(STATUS_KERNEL_APC, STATUS_KERNEL_APC)
    genVal(STATUS_LONGJUMP, STATUS_LONGJUMP)
    genVal(STATUS_NO_CALLBACK_ACTIVE, STATUS_NO_CALLBACK_ACTIVE)
    genVal(STATUS_NO_EVENT_PAIR, STATUS_NO_EVENT_PAIR)
    genVal(STATUS_PRIVILEGED_INSTRUCTION, STATUS_PRIVILEGED_INSTRUCTION)
    genVal(STATUS_SINGLE_STEP, STATUS_SINGLE_STEP)
    genVal(STATUS_STACK_OVERFLOW, STATUS_STACK_OVERFLOW)
    genVal(STATUS_SUCCESS, STATUS_SUCCESS)
    genVal(STATUS_THREAD_IS_TERMINATING, STATUS_THREAD_IS_TERMINATING)
    genVal(STATUS_TIMEOUT, STATUS_TIMEOUT)
    genVal(STATUS_UNWIND, STATUS_UNWIND)
    genVal(STATUS_WAKE_SYSTEM_DEBUGGER, STATUS_WAKE_SYSTEM_DEBUGGER)

    //
    // Define kernel object structure definitions.
    //
    // APC object structure defintions.
    //

    genCom("APC Object Structure Offset Definitions")

    genDef(Ap, KAPC, Type)
    genDef(Ap, KAPC, Size)
    genDef(Ap, KAPC, Thread)
    genDef(Ap, KAPC, ApcListEntry)
    genDef(Ap, KAPC, KernelRoutine)
    genDef(Ap, KAPC, RundownRoutine)
    genDef(Ap, KAPC, NormalRoutine)
    genDef(Ap, KAPC, NormalContext)
    genDef(Ap, KAPC, SystemArgument1)
    genDef(Ap, KAPC, SystemArgument2)
    genDef(Ap, KAPC, ApcStateIndex)
    genDef(Ap, KAPC, ApcMode)
    genDef(Ap, KAPC, Inserted)
    genVal(ApcObjectLength, sizeof(KAPC))

    //
    // DPC Object structure definitions.
    //

    EnableInc(HAL)

    genCom("DPC object Structure Offset Definitions")

    genDef(Dp, KDPC, Type)
    genDef(Dp, KDPC, Number)
    genDef(Dp, KDPC, Importance)
    genDef(Dp, KDPC, DpcListEntry)
    genDef(Dp, KDPC, DeferredRoutine)
    genDef(Dp, KDPC, DeferredContext)
    genDef(Dp, KDPC, SystemArgument1)
    genDef(Dp, KDPC, SystemArgument2)
    genDef(Dp, KDPC, Lock)
    genVal(DpcObjectLength, sizeof(KDPC))

    DisableInc(HAL)

    //
    // Device queue object structure definitions.
    //

    genCom("Device Queue Object Structure Offset Definitions")

    genDef(Dv, KDEVICE_QUEUE, Type)
    genDef(Dv, KDEVICE_QUEUE, Size)
    genDef(Dv, KDEVICE_QUEUE, DeviceListHead)
    genAlt(DvSpinLock, KDEVICE_QUEUE, Lock)
    genDef(Dv, KDEVICE_QUEUE, Busy)
    genVal(DeviceQueueObjectLength, sizeof(KDEVICE_QUEUE))

    //
    // Device object entry structure definitions.
    //

    genCom("Device Queue Entry Structure Offset Definitions")

    genDef(De, KDEVICE_QUEUE_ENTRY, DeviceListEntry)
    genDef(De, KDEVICE_QUEUE_ENTRY, SortKey)
    genDef(De, KDEVICE_QUEUE_ENTRY, Inserted)
    genVal(DeviceQueueEntryLength, sizeof(KDEVICE_QUEUE_ENTRY))

    //
    // Event object structure definitions.
    //

    genCom("Event Object Structure Offset Definitions")

    genDef(Ev, DISPATCHER_HEADER, Type)
    genDef(Ev, DISPATCHER_HEADER, Size)
    genDef(Ev, DISPATCHER_HEADER, SignalState)
    genAlt(EvWaitListHead, KEVENT, Header.WaitListHead)
    genVal(EventObjectLength, sizeof(KEVENT))

    //
    // Event pair object structure definitions.
    //

    genCom("Event Pair Object Structure Offset Definitions")

    genDef(Ep, KEVENT_PAIR, Type)
    genDef(Ep, KEVENT_PAIR, Size)
    genDef(Ep, KEVENT_PAIR, EventLow)
    genDef(Ep, KEVENT_PAIR, EventHigh)

#if defined(_MIPS_) || defined(_PPC_) || defined(_IA64_)

    #define EventOffset (OFFSET(KEVENT_PAIR, EventHigh) - \
                         OFFSET(KEVENT_PAIR, EventLow))

//    if ((EventOffset & (EventOffset - 1)) != 0) {
//        fprintf(stderr, "GENXX: Event offset not log2N\n")
//    }

    genVal(SET_LOW_WAIT_HIGH, - (LONG)(EventOffset * 2))
    genVal(SET_HIGH_WAIT_LOW, - (LONG)EventOffset)
    genVal(SET_EVENT_PAIR_MASK, (LONG)EventOffset)

#endif

    //
    // Interrupt object structure definitions.
    //

#if defined(_ALPHA_) || defined(_MIPS_) || defined(_PPC_) || defined(_IA64_)

    EnableInc(HAL)

#endif

    genCom("Interrupt Object Structure Offset Definitions")

    genVal(InLevelSensitive, LevelSensitive)
    genVal(InLatched, Latched)

    genSpc()

    genDef(In, KINTERRUPT, Type)
    genDef(In, KINTERRUPT, Size)
    genDef(In, KINTERRUPT, InterruptListEntry)
    genDef(In, KINTERRUPT, ServiceRoutine)
    genDef(In, KINTERRUPT, ServiceContext)
    genDef(In, KINTERRUPT, SpinLock)
    genDef(In, KINTERRUPT, ActualLock)
    genDef(In, KINTERRUPT, DispatchAddress)
    genDef(In, KINTERRUPT, Vector)
    genDef(In, KINTERRUPT, Irql)
    genDef(In, KINTERRUPT, SynchronizeIrql)
    genDef(In, KINTERRUPT, FloatingSave)
    genDef(In, KINTERRUPT, Connected)
    genDef(In, KINTERRUPT, Number)
    genDef(In, KINTERRUPT, Mode)
    genDef(In, KINTERRUPT, ShareVector)
    genDef(In, KINTERRUPT, DispatchCode)
    genVal(InterruptObjectLength, sizeof(KINTERRUPT))

#if defined(_X86_)

    genSpc()

    genVal(NORMAL_DISPATCH_LENGTH, NORMAL_DISPATCH_LENGTH * sizeof(ULONG))
    genVal(DISPATCH_LENGTH, DISPATCH_LENGTH * sizeof(ULONG))

#endif

#if defined(_ALPHA_) || defined(_MIPS_) || defined(_PPC_) || defined(_IA64_)

    DisableInc(HAL)

#endif

    //
    // Process object structure offset definitions.
    //

    genCom("Process Object Structure Offset Definitions")

    genDef(Pr, DISPATCHER_HEADER, Type)
    genDef(Pr, DISPATCHER_HEADER, Size)
    genDef(Pr, DISPATCHER_HEADER, SignalState)
    genDef(Pr, KPROCESS, ProfileListHead)
    genDef(Pr, KPROCESS, DirectoryTableBase)

#if defined(_X86_)

    genDef(Pr, KPROCESS, LdtDescriptor)
    genDef(Pr, KPROCESS, Int21Descriptor)
    genDef(Pr, KPROCESS, IopmOffset)
    genDef(Pr, KPROCESS, Iopl)
    genDef(Pr, KPROCESS, VdmFlag)

#endif

#if defined(_IA64_)

    genDef(Pr, KPROCESS, LdtDescriptor)
    genDef(Pr, KPROCESS, Int21Descriptor)
    genDef(Pr, KPROCESS, VdmFlag)

    genDef(Pr, KPROCESS, ProcessRegion)
    genDef(Pr, KPROCESS, SessionRegion)
    genDef(Pr, KPROCESS, SessionMapInfo)
    genDef(Pr, KPROCESS, SessionParentBase)

#endif

    genDef(Pr, KPROCESS, ActiveProcessors)

#if defined(_ALPHA_)

    genDef(Pr, KPROCESS, RunOnProcessors)
    genDef(Pr, KPROCESS, ProcessSequence)
    genDef(Pr, KPROCESS, ProcessAsn)

#endif

    genDef(Pr, KPROCESS, KernelTime)
    genDef(Pr, KPROCESS, UserTime)
    genDef(Pr, KPROCESS, ReadyListHead)
    genDef(Pr, KPROCESS, SwapListEntry)
    genDef(Pr, KPROCESS, ThreadListHead)
    genDef(Pr, KPROCESS, ProcessLock)
    genDef(Pr, KPROCESS, Affinity)
    genDef(Pr, KPROCESS, StackCount)
    genDef(Pr, KPROCESS, BasePriority)
    genDef(Pr, KPROCESS, ThreadQuantum)
    genDef(Pr, KPROCESS, AutoAlignment)
    genDef(Pr, KPROCESS, State)
    genVal(ProcessObjectLength, ((sizeof(KPROCESS) + 15) & ~15))
    genVal(ExtendedProcessObjectLength, ((sizeof(EPROCESS) + 15) & ~15))

    //
    // Profile object structure offset definitions.
    //

    genCom("Profile Object Structure Offset Definitions")

    genDef(Pf, KPROFILE, Type)
    genDef(Pf, KPROFILE, Size)
    genDef(Pf, KPROFILE, ProfileListEntry)
    genDef(Pf, KPROFILE, Process)
    genDef(Pf, KPROFILE, RangeBase)
    genDef(Pf, KPROFILE, RangeLimit)
    genDef(Pf, KPROFILE, BucketShift)
    genDef(Pf, KPROFILE, Buffer)
    genDef(Pf, KPROFILE, Segment)
    genDef(Pf, KPROFILE, Affinity)
    genDef(Pf, KPROFILE, Source)
    genDef(Pf, KPROFILE, Started)
    genVal(ProfileObjectLength, sizeof(KPROFILE))

    //
    // Queue object structure offset definitions.
    //

    genCom("Queue Object Structure Offset Definitions")

    genDef(Qu, DISPATCHER_HEADER, Type)
    genDef(Qu, DISPATCHER_HEADER, Size)
    genDef(Qu, DISPATCHER_HEADER, SignalState)
    genDef(Qu, KQUEUE, EntryListHead)
    genDef(Qu, KQUEUE, CurrentCount)
    genDef(Qu, KQUEUE, MaximumCount)
    genDef(Qu, KQUEUE, ThreadListHead)
    genVal(QueueObjectLength, sizeof(KQUEUE))

    //
    // Thread object structure offset definitions
    //

    genCom("Thread Object Structure Offset Definitions")

    genDef(Ee, EEVENT_PAIR, KernelEventPair)
    genDef(Et, ETHREAD, Cid)
    genDef(Et, ETHREAD, PerformanceCountLow)
    genDef(Et, ETHREAD, PerformanceCountHigh)
    genVal(EtEthreadLength, ((sizeof(ETHREAD) + 15) & ~15))

    genSpc()

    genDef(Th, DISPATCHER_HEADER, Type)
    genDef(Th, DISPATCHER_HEADER, Size)
    genDef(Th, DISPATCHER_HEADER, SignalState)
    genDef(Th, KTHREAD, MutantListHead)
    genDef(Th, KTHREAD, InitialStack)
    genDef(Th, KTHREAD, StackLimit)
    genDef(Th, KTHREAD, Teb)
    genDef(Th, KTHREAD, TlsArray)
    genDef(Th, KTHREAD, KernelStack)
    genDef(Th, KTHREAD, DebugActive)
    genDef(Th, KTHREAD, State)
    genDef(Th, KTHREAD, Alerted)
    genDef(Th, KTHREAD, Iopl)
    genDef(Th, KTHREAD, NpxState)
    genDef(Th, KTHREAD, Saturation)
    genDef(Th, KTHREAD, Priority)
    genDef(Th, KTHREAD, ApcState)
    genDef(Th, KTHREAD, ContextSwitches)
    genDef(Th, KTHREAD, WaitStatus)
    genDef(Th, KTHREAD, WaitIrql)
    genDef(Th, KTHREAD, WaitMode)
    genDef(Th, KTHREAD, WaitNext)
    genDef(Th, KTHREAD, WaitReason)
    genDef(Th, KTHREAD, WaitBlockList)
    genDef(Th, KTHREAD, WaitListEntry)
    genDef(Th, KTHREAD, WaitTime)
    genDef(Th, KTHREAD, BasePriority)
    genDef(Th, KTHREAD, DecrementCount)
    genDef(Th, KTHREAD, PriorityDecrement)
    genDef(Th, KTHREAD, Quantum)
    genDef(Th, KTHREAD, WaitBlock)
    genDef(Th, KTHREAD, KernelApcDisable)
    genDef(Th, KTHREAD, UserAffinity)
    genDef(Th, KTHREAD, SystemAffinityActive)
    genDef(Th, KTHREAD, ServiceTable)
//    genDef(Th, KTHREAD, Channel)
//    genDef(Th, KTHREAD, Section)
//    genDef(Th, KTHREAD, SystemView)
//    genDef(Th, KTHREAD, ThreadView)
    genDef(Th, KTHREAD, Queue)
    genDef(Th, KTHREAD, ApcQueueLock)
    genDef(Th, KTHREAD, Timer)
    genDef(Th, KTHREAD, QueueListEntry)
    genDef(Th, KTHREAD, Affinity)
    genDef(Th, KTHREAD, Preempted)
    genDef(Th, KTHREAD, ProcessReadyQueue)
    genDef(Th, KTHREAD, KernelStackResident)
    genDef(Th, KTHREAD, NextProcessor)
    genDef(Th, KTHREAD, CallbackStack)
    genDef(Th, KTHREAD, Win32Thread)
    genDef(Th, KTHREAD, TrapFrame)
    genDef(Th, KTHREAD, ApcStatePointer)
    genDef(Th, KTHREAD, PreviousMode)
    genDef(Th, KTHREAD, EnableStackSwap)
    genDef(Th, KTHREAD, LargeStack)
    genDef(Th, KTHREAD, KernelTime)
    genDef(Th, KTHREAD, UserTime)
    genDef(Th, KTHREAD, SavedApcState)
    genDef(Th, KTHREAD, Alertable)
    genDef(Th, KTHREAD, ApcStateIndex)
    genDef(Th, KTHREAD, ApcQueueable)
    genDef(Th, KTHREAD, AutoAlignment)
    genDef(Th, KTHREAD, StackBase)
    genDef(Th, KTHREAD, SuspendApc)
    genDef(Th, KTHREAD, SuspendSemaphore)
    genDef(Th, KTHREAD, ThreadListEntry)
    genDef(Th, KTHREAD, FreezeCount)
    genDef(Th, KTHREAD, SuspendCount)
    genDef(Th, KTHREAD, IdealProcessor)
    genDef(Th, KTHREAD, DisableBoost)

#if defined(_IA64_)

    genDef(Th, KTHREAD, InitialBStore)
    genDef(Th, KTHREAD, BStoreLimit)
    genDef(Th, KTHREAD, KernelBStore)
    genDef(Th, KTHREAD, CallbackBStore)

#endif // defined(_IA64_)

    genVal(ThreadObjectLength, ((sizeof(KTHREAD) + 15) & ~15))
    genVal(ExtendedThreadObjectLength, ((sizeof(ETHREAD) + 15) & ~15))

    genSpc()

    genVal(EVENT_WAIT_BLOCK_OFFSET, OFFSET(KTHREAD, WaitBlock) + (sizeof(KWAIT_BLOCK) * EVENT_WAIT_BLOCK))

#if defined(_X86_)

    genVal(NPX_STATE_NOT_LOADED, NPX_STATE_NOT_LOADED)
    genVal(NPX_STATE_LOADED, NPX_STATE_LOADED)

#endif

    //
    // Timer object structure offset definitions
    //

    genCom("Timer object Structure Offset Definitions")

    genDef(Ti, DISPATCHER_HEADER, Type)
    genDef(Ti, DISPATCHER_HEADER, Size)
    genDef(Ti, DISPATCHER_HEADER, Inserted)
    genDef(Ti, DISPATCHER_HEADER, SignalState)
    genDef(Ti, KTIMER, DueTime)
    genDef(Ti, KTIMER, TimerListEntry)
    genDef(Ti, KTIMER, Dpc)
    genDef(Ti, KTIMER, Period)
    genVal(TimerObjectLength, sizeof(KTIMER))

    genSpc()

    genVal(TIMER_TABLE_SIZE, TIMER_TABLE_SIZE)

    //
    // Wait block structure offset definitions
    //

    genCom("Wait Block Structure Offset Definitions")

    genDef(Wb, KWAIT_BLOCK, WaitListEntry)
    genDef(Wb, KWAIT_BLOCK, Thread)
    genDef(Wb, KWAIT_BLOCK, Object)
    genDef(Wb, KWAIT_BLOCK, NextWaitBlock)
    genDef(Wb, KWAIT_BLOCK, WaitKey)
    genDef(Wb, KWAIT_BLOCK, WaitType)

    //
    // Fiber structure offset definitions.
    //

    genCom("Fiber Structure Offset Definitions")

    genDef(Fb, FIBER, FiberData)
    genDef(Fb, FIBER, ExceptionList)
    genDef(Fb, FIBER, StackBase)
    genDef(Fb, FIBER, StackLimit)
    genDef(Fb, FIBER, DeallocationStack)
    genDef(Fb, FIBER, FiberContext)
    genDef(Fb, FIBER, Wx86Tib)

#if defined(_IA64_)

    genDef(Fb, FIBER, DeallocationBStore)
    genDef(Fb, FIBER, BStoreLimit)

#endif // defined(_IA64_)

    //
    // Process environment block structure offset definitions.
    //

    genCom("Process Environment Block Structure Offset Definitions")

    genDef(Pe, PEB, KernelCallbackTable)

    //
    // Define System Service Descriptor Table structures.
    //

    genCom("System Service Descriptor Table Structure Definitions")

    genVal(NUMBER_SERVICE_TABLES, NUMBER_SERVICE_TABLES)
    genVal(SERVICE_NUMBER_MASK, SERVICE_NUMBER_MASK)
    genVal(SERVICE_TABLE_SHIFT, SERVICE_TABLE_SHIFT)
    genVal(SERVICE_TABLE_MASK, SERVICE_TABLE_MASK)
    genVal(SERVICE_TABLE_TEST, SERVICE_TABLE_TEST)

    genSpc()

    genDef(Sd, KSERVICE_TABLE_DESCRIPTOR, Base)
    genDef(Sd, KSERVICE_TABLE_DESCRIPTOR, Count)
    genDef(Sd, KSERVICE_TABLE_DESCRIPTOR, Limit)
#if defined(_IA64_)
    genDef(Sd, KSERVICE_TABLE_DESCRIPTOR, TableBaseGpOffset)
#endif
    genDef(Sd, KSERVICE_TABLE_DESCRIPTOR, Number)

    //
    // Common TEB structure offset definitions
    //

    genCom("Thread Environment Block Structure Offset Definitions")

    genDef(Te, NT_TIB, StackBase)
    genDef(Te, NT_TIB, StackLimit)
    genDef(Te, NT_TIB, FiberData)
    genDef(Te, TEB, EnvironmentPointer)
    genDef(Te, TEB, ClientId)
    genDef(Te, TEB, ActiveRpcHandle)
    genDef(Te, TEB, ThreadLocalStoragePointer)
    genDef(Te, TEB, CountOfOwnedCriticalSections)
    genAlt(TePeb, TEB, ProcessEnvironmentBlock)
    genDef(Te, TEB, CsrClientThread)
    genDef(Te, TEB, WOW32Reserved)
    genAlt(TeSoftFpcr, TEB, FpSoftwareStatusRegister)
    genDef(Te, TEB, GdiClientPID)
    genDef(Te, TEB, GdiClientTID)
    genDef(Te, TEB, GdiThreadLocalInfo)
    genDef(Te, TEB, glDispatchTable)
    genDef(Te, TEB, glReserved1)
    genDef(Te, TEB, glReserved2)
    genDef(Te, TEB, glSectionInfo)
    genDef(Te, TEB, glSection)
    genDef(Te, TEB, glTable)
    genDef(Te, TEB, glCurrentRC)
    genDef(Te, TEB, glContext)
    genDef(Te, TEB, DeallocationStack)
    genDef(Te, TEB, TlsSlots)
    genDef(Te, TEB, Vdm)
    genDef(Te, TEB, GdiBatchCount)
    genDef(Te, TEB, Instrumentation)

#if defined(_IA64_)

    genDef(Te, NT_TIB, ExceptionList)
    genDef(Te, TEB, DeallocationBStore)
    genDef(Te, TEB, BStoreLimit)
    genDef(Te, TEB, DbgSsReserved)

#endif // defined(_IA64_)

    genVal(ThreadEnvironmentBlockLength, sizeof(TEB))

    //
    // Lock Queue structure definitions.
    //

#if defined(_X86_)

    EnableInc(HAL)

#endif

    genCom("Lock Queue Structure Offset Definitions")

    genNam(LOCK_QUEUE_WAIT)
    genNam(LOCK_QUEUE_OWNER)
    genVal(LOCK_QUEUE_HEADER_SIZE, sizeof(KSPIN_LOCK_QUEUE))
    genSpc()

    genNam(LockQueueDispatcherLock)
    genNam(LockQueueContextSwapLock)
    genNam(LockQueuePfnLock)
    genSpc()

    genDef(Lq, KSPIN_LOCK_QUEUE, Next)
    genDef(Lq, KSPIN_LOCK_QUEUE, Lock)

#if defined(_X86_)

    DisableInc(HAL)

#endif
