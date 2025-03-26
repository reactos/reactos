#pragma once

#include <section_attribs.h>



#ifdef _NTOSKRNL_

#ifndef _ARM_
#define KeGetCurrentThread  _KeGetCurrentThread
#define KeGetPreviousMode   _KeGetPreviousMode
#endif
#undef  PsGetCurrentProcess
#define PsGetCurrentProcess _PsGetCurrentProcess

#define RVA(m, b) ((PVOID)((ULONG_PTR)(b) + (ULONG_PTR)(m)))

//
// We are very lazy on ARM -- we just import intrinsics
// Question: Why wasn't this done for x86 too? (see fastintrlck.asm)
//
#define InterlockedDecrement         _InterlockedDecrement
#define InterlockedDecrement16       _InterlockedDecrement16
#define InterlockedIncrement         _InterlockedIncrement
#define InterlockedIncrement16       _InterlockedIncrement16
#define InterlockedCompareExchange   _InterlockedCompareExchange
#define InterlockedCompareExchange16 _InterlockedCompareExchange16
#define InterlockedCompareExchange64 _InterlockedCompareExchange64
#define InterlockedExchange          _InterlockedExchange
#define InterlockedExchangeAdd       _InterlockedExchangeAdd
#define InterlockedOr                _InterlockedOr
#define InterlockedAnd               _InterlockedAnd

//
// Use inlined versions of fast/guarded mutex routines
//
#define ExEnterCriticalRegionAndAcquireFastMutexUnsafe _ExEnterCriticalRegionAndAcquireFastMutexUnsafe
#define ExReleaseFastMutexUnsafeAndLeaveCriticalRegion _ExReleaseFastMutexUnsafeAndLeaveCriticalRegion
#define ExAcquireFastMutex _ExAcquireFastMutex
#define ExReleaseFastMutex _ExReleaseFastMutex
#define ExAcquireFastMutexUnsafe _ExAcquireFastMutexUnsafe
#define ExReleaseFastMutexUnsafe _ExReleaseFastMutexUnsafe
#define ExTryToAcquireFastMutex _ExTryToAcquireFastMutex

#define KeInitializeGuardedMutex _KeInitializeGuardedMutex
#define KeAcquireGuardedMutex _KeAcquireGuardedMutex
#define KeReleaseGuardedMutex _KeReleaseGuardedMutex
#define KeAcquireGuardedMutexUnsafe _KeAcquireGuardedMutexUnsafe
#define KeReleaseGuardedMutexUnsafe _KeReleaseGuardedMutexUnsafe
#define KeTryToAcquireGuardedMutex _KeTryToAcquireGuardedMutex

#include "tag.h"
#include "ke.h"
#include "ob.h"
#include "mm.h"
#include "ex.h"
#include "cm.h"
#include "ps.h"
#include "cc.h"
#include "io.h"
#include "po.h"
#include "se.h"
#include "ldr.h"
#ifndef _WINKD_
#include "kd.h"
#endif
#include "kd64.h"
#include "fsrtl.h"
#include "lpc.h"
#include "rtl.h"
#include "dbgk.h"
#include "spinlock.h"
#include "test.h"
#include "inbv.h"
#include "vdm.h"
#include "hal.h"
#include "hdl.h"
#include "icif.h"
#include "arch/intrin_i.h"
#include <arbiter.h>

/*
 * Use IsPointerOffset to test whether a pointer should be interpreted as an offset
 * or as a pointer
 */
#if defined(_X86_) || defined(_M_AMD64) || defined(_MIPS_) || defined(_PPC_) || defined(_ARM_)

/* for x86 and x86-64 the MSB is 1 so we can simply test on that */
#define IsPointerOffset(Ptr) ((LONG_PTR)(Ptr) >= 0)

#elif defined(_IA64_)

/* on Itanium if the 24 most significant bits are set, we're not dealing with
   offsets anymore. */
#define IsPointerOffset(Ptr)  (((ULONG_PTR)(Ptr) & 0xFFFFFF0000000000ULL) == 0)

#else
#error IsPointerOffset() needs to be defined for this architecture
#endif

#endif

#ifndef _WIN64
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, SystemCall) == 0x300);

C_ASSERT(FIELD_OFFSET(KTHREAD, InitialStack) == KTHREAD_INITIAL_STACK);
C_ASSERT(FIELD_OFFSET(KTHREAD, KernelStack) == KTHREAD_KERNEL_STACK);
C_ASSERT(FIELD_OFFSET(KTHREAD, SystemAffinityActive) == FIELD_OFFSET(KTHREAD, WaitBlock) + FIELD_OFFSET(KWAIT_BLOCK, SpareByte));
C_ASSERT(FIELD_OFFSET(KTHREAD, ApcState.Process) == KTHREAD_APCSTATE_PROCESS);
C_ASSERT(FIELD_OFFSET(KTHREAD, ApcQueueable) == FIELD_OFFSET(KTHREAD, ApcState.UserApcPending) + 1);
C_ASSERT(FIELD_OFFSET(KTHREAD, ApcQueueable) == 0x3F);
C_ASSERT(FIELD_OFFSET(KTHREAD, NextProcessor) == 0x40);
C_ASSERT(FIELD_OFFSET(KTHREAD, DeferredProcessor) == 0x41);
C_ASSERT(FIELD_OFFSET(KTHREAD, AdjustReason) == 0x42);
C_ASSERT(FIELD_OFFSET(KTHREAD, NpxState) == KTHREAD_NPX_STATE);
C_ASSERT(FIELD_OFFSET(KTHREAD, Alertable) == 0x58);
C_ASSERT(FIELD_OFFSET(KTHREAD, SwapBusy) == 0x05D);
C_ASSERT(FIELD_OFFSET(KTHREAD, Teb) == KTHREAD_TEB);
C_ASSERT(FIELD_OFFSET(KTHREAD, Timer) == 0x078);
C_ASSERT(FIELD_OFFSET(KTHREAD, ThreadFlags) == 0x0A0);
C_ASSERT(FIELD_OFFSET(KTHREAD, WaitBlock) == 0x0A8);
C_ASSERT(FIELD_OFFSET(KTHREAD, WaitBlockFill0) == 0x0A8);
C_ASSERT(FIELD_OFFSET(KTHREAD, QueueListEntry) == 0x108);
C_ASSERT(FIELD_OFFSET(KTHREAD, PreviousMode) == KTHREAD_PREVIOUS_MODE);
C_ASSERT(FIELD_OFFSET(KTHREAD, PreviousMode) == FIELD_OFFSET(KTHREAD, WaitBlock) + sizeof(KWAIT_BLOCK) + FIELD_OFFSET(KWAIT_BLOCK, SpareByte));
C_ASSERT(FIELD_OFFSET(KTHREAD, ResourceIndex) == FIELD_OFFSET(KTHREAD, WaitBlock) + 2*sizeof(KWAIT_BLOCK) + FIELD_OFFSET(KWAIT_BLOCK, SpareByte));
C_ASSERT(FIELD_OFFSET(KTHREAD, LargeStack) == FIELD_OFFSET(KTHREAD, WaitBlock) + 3*sizeof(KWAIT_BLOCK) + FIELD_OFFSET(KWAIT_BLOCK, SpareByte));
C_ASSERT(FIELD_OFFSET(KTHREAD, TrapFrame) == KTHREAD_TRAP_FRAME);
C_ASSERT(FIELD_OFFSET(KTHREAD, CallbackStack) == KTHREAD_CALLBACK_STACK);
C_ASSERT(FIELD_OFFSET(KTHREAD, ServiceTable) == KTHREAD_SERVICE_TABLE);
C_ASSERT(FIELD_OFFSET(KTHREAD, FreezeCount) == FIELD_OFFSET(KTHREAD, SavedApcState.UserApcPending) + 1);
C_ASSERT(FIELD_OFFSET(KTHREAD, Quantum) == FIELD_OFFSET(KTHREAD, SuspendApc.SpareByte0));
C_ASSERT(FIELD_OFFSET(KTHREAD, QuantumReset) == FIELD_OFFSET(KTHREAD, SuspendApc.SpareByte1));
C_ASSERT(FIELD_OFFSET(KTHREAD, KernelTime) == FIELD_OFFSET(KTHREAD, SuspendApc.SpareLong0));
C_ASSERT(FIELD_OFFSET(KTHREAD, TlsArray) == FIELD_OFFSET(KTHREAD, SuspendApc.SystemArgument1));
C_ASSERT(FIELD_OFFSET(KTHREAD, LegoData) == FIELD_OFFSET(KTHREAD, SuspendApc.SystemArgument2));
C_ASSERT(FIELD_OFFSET(KTHREAD, PowerState) == FIELD_OFFSET(KTHREAD, SuspendApc.Inserted) + 1);
C_ASSERT(sizeof(KTHREAD) == 0x1B8);

C_ASSERT(FIELD_OFFSET(KPROCESS, DirectoryTableBase) == KPROCESS_DIRECTORY_TABLE_BASE);

C_ASSERT(FIELD_OFFSET(KPCR, NtTib.ExceptionList) == KPCR_EXCEPTION_LIST);
C_ASSERT(FIELD_OFFSET(KPCR, SelfPcr) == KPCR_SELF);
C_ASSERT(FIELD_OFFSET(KPCR, IRR) == KPCR_IRR);
C_ASSERT(FIELD_OFFSET(KPCR, IDR) == KPCR_IDR);
C_ASSERT(FIELD_OFFSET(KPCR, Irql) == KPCR_IRQL);
C_ASSERT(FIELD_OFFSET(KIPCR, PrcbData) + FIELD_OFFSET(KPRCB, CurrentThread) == KPCR_CURRENT_THREAD);
C_ASSERT(FIELD_OFFSET(KIPCR, PrcbData) + FIELD_OFFSET(KPRCB, NextThread) == KPCR_PRCB_NEXT_THREAD);
C_ASSERT(FIELD_OFFSET(KIPCR, PrcbData) + FIELD_OFFSET(KPRCB, NpxThread) == KPCR_NPX_THREAD);
C_ASSERT(FIELD_OFFSET(KIPCR, PrcbData) == KPCR_PRCB_DATA);
C_ASSERT(FIELD_OFFSET(KIPCR, PrcbData) + FIELD_OFFSET(KPRCB, KeSystemCalls) == KPCR_SYSTEM_CALLS);
C_ASSERT(FIELD_OFFSET(KIPCR, PrcbData) + FIELD_OFFSET(KPRCB, DpcData) + FIELD_OFFSET(KDPC_DATA, DpcQueueDepth) == KPCR_PRCB_DPC_QUEUE_DEPTH);
C_ASSERT(FIELD_OFFSET(KIPCR, PrcbData) + FIELD_OFFSET(KPRCB, DpcData) + 16 == KPCR_PRCB_DPC_COUNT);
C_ASSERT(FIELD_OFFSET(KIPCR, PrcbData) + FIELD_OFFSET(KPRCB, DpcStack) == KPCR_PRCB_DPC_STACK);
C_ASSERT(FIELD_OFFSET(KIPCR, PrcbData) + FIELD_OFFSET(KPRCB, TimerRequest) == KPCR_PRCB_TIMER_REQUEST);
C_ASSERT(FIELD_OFFSET(KIPCR, PrcbData) + FIELD_OFFSET(KPRCB, MaximumDpcQueueDepth) == KPCR_PRCB_MAXIMUM_DPC_QUEUE_DEPTH);
C_ASSERT(FIELD_OFFSET(KIPCR, PrcbData) + FIELD_OFFSET(KPRCB, DpcRequestRate) == KPCR_PRCB_DPC_REQUEST_RATE);
C_ASSERT(FIELD_OFFSET(KIPCR, PrcbData) + FIELD_OFFSET(KPRCB, DpcInterruptRequested) == KPCR_PRCB_DPC_INTERRUPT_REQUESTED);
C_ASSERT(FIELD_OFFSET(KIPCR, PrcbData) + FIELD_OFFSET(KPRCB, DpcRoutineActive) == KPCR_PRCB_DPC_ROUTINE_ACTIVE);
C_ASSERT(FIELD_OFFSET(KIPCR, PrcbData) + FIELD_OFFSET(KPRCB, DpcLastCount) == KPCR_PRCB_DPC_LAST_COUNT);
C_ASSERT(FIELD_OFFSET(KIPCR, PrcbData) + FIELD_OFFSET(KPRCB, TimerRequest) == KPCR_PRCB_TIMER_REQUEST);
C_ASSERT(FIELD_OFFSET(KIPCR, PrcbData) + FIELD_OFFSET(KPRCB, QuantumEnd) == KPCR_PRCB_QUANTUM_END);
C_ASSERT(FIELD_OFFSET(KIPCR, PrcbData) + FIELD_OFFSET(KPRCB, DeferredReadyListHead) == KPCR_PRCB_DEFERRED_READY_LIST_HEAD);
C_ASSERT(FIELD_OFFSET(KIPCR, PrcbData) + FIELD_OFFSET(KPRCB, PowerState) == KPCR_PRCB_POWER_STATE_IDLE_FUNCTION);
C_ASSERT(FIELD_OFFSET(KIPCR, PrcbData) + FIELD_OFFSET(KPRCB, PrcbLock) == KPCR_PRCB_PRCB_LOCK);
C_ASSERT(FIELD_OFFSET(KIPCR, PrcbData) + FIELD_OFFSET(KPRCB, DpcStack) == KPCR_PRCB_DPC_STACK);
C_ASSERT(FIELD_OFFSET(KIPCR, PrcbData) + FIELD_OFFSET(KPRCB, IdleSchedule) == KPCR_PRCB_IDLE_SCHEDULE);
C_ASSERT(sizeof(FX_SAVE_AREA) == SIZEOF_FX_SAVE_AREA);

/* Platform specific checks */
C_ASSERT(FIELD_OFFSET(KPROCESS, IopmOffset) == KPROCESS_IOPM_OFFSET);
C_ASSERT(FIELD_OFFSET(KPROCESS, LdtDescriptor) == KPROCESS_LDT_DESCRIPTOR0);
C_ASSERT(FIELD_OFFSET(KTSS, Esp0) == KTSS_ESP0);
C_ASSERT(FIELD_OFFSET(KTSS, IoMapBase) == KTSS_IOMAPBASE);
#endif
