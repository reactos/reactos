#ifndef __INCLUDE_INTERNAL_NTOSKRNL_H
#define __INCLUDE_INTERNAL_NTOSKRNL_H

/*
 * Use these to place a function in a specific section of the executable
 */
#define PLACE_IN_SECTION(s)	__attribute__((section (s)))
#ifdef __GNUC__
#define INIT_FUNCTION		PLACE_IN_SECTION("INIT")
#define PAGE_LOCKED_FUNCTION	PLACE_IN_SECTION("pagelk")
#define PAGE_UNLOCKED_FUNCTION	PLACE_IN_SECTION("pagepo")
#else
#define INIT_FUNCTION
#define PAGE_LOCKED_FUNCTION
#define PAGE_UNLOCKED_FUNCTION
#endif

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
#else
#include "kd64.h"
#endif
#include "fsrtl.h"
#include "lpc.h"
#include "rtl.h"
#ifdef KDBG
#include "../kdbg/kdb.h"
#endif
#include "dbgk.h"
#include "tag.h"
#include "test.h"
#include "inbv.h"
#include "vdm.h"
#include "hal.h"
#include "arch/intrin_i.h"

/*
 * generic information class probing code
 */

#define ICIF_QUERY               0x1
#define ICIF_SET                 0x2
#define ICIF_QUERY_SIZE_VARIABLE 0x4
#define ICIF_SET_SIZE_VARIABLE   0x8
#define ICIF_SIZE_VARIABLE (ICIF_QUERY_SIZE_VARIABLE | ICIF_SET_SIZE_VARIABLE)

typedef struct _INFORMATION_CLASS_INFO
{
  ULONG RequiredSizeQUERY;
  ULONG RequiredSizeSET;
  ULONG AlignmentSET;
  ULONG AlignmentQUERY;
  ULONG Flags;
} INFORMATION_CLASS_INFO, *PINFORMATION_CLASS_INFO;

#define ICI_SQ_SAME(Type, Alignment, Flags)                                    \
  { Type, Type, Alignment, Alignment, Flags }

#define ICI_SQ(TypeQuery, TypeSet, AlignmentQuery, AlignmentSet, Flags)        \
  { TypeQuery, TypeSet, AlignmentQuery, AlignmentSet, Flags }

//
// TEMPORARY
//
#define IQS_SAME(Type, Alignment, Flags)                                    \
  { sizeof(Type), sizeof(Type), sizeof(Alignment), sizeof(Alignment), Flags }

#define IQS(TypeQuery, TypeSet, AlignmentQuery, AlignmentSet, Flags)        \
  { sizeof(TypeQuery), sizeof(TypeSet), sizeof(AlignmentQuery), sizeof(AlignmentSet), Flags }

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

C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, SystemCall) == 0x300);
C_ASSERT(FIELD_OFFSET(KTHREAD, InitialStack) == KTHREAD_INITIAL_STACK);
C_ASSERT(FIELD_OFFSET(KTHREAD, Teb) == KTHREAD_TEB);
C_ASSERT(FIELD_OFFSET(KTHREAD, KernelStack) == KTHREAD_KERNEL_STACK);
C_ASSERT(FIELD_OFFSET(KTHREAD, NpxState) == KTHREAD_NPX_STATE);
C_ASSERT(FIELD_OFFSET(KTHREAD, ServiceTable) == KTHREAD_SERVICE_TABLE);
C_ASSERT(FIELD_OFFSET(KTHREAD, PreviousMode) == KTHREAD_PREVIOUS_MODE);
C_ASSERT(FIELD_OFFSET(KTHREAD, TrapFrame) == KTHREAD_TRAP_FRAME);
C_ASSERT(FIELD_OFFSET(KTHREAD, CallbackStack) == KTHREAD_CALLBACK_STACK);
C_ASSERT(FIELD_OFFSET(KTHREAD, ApcState.Process) == KTHREAD_APCSTATE_PROCESS);
C_ASSERT(FIELD_OFFSET(KPROCESS, DirectoryTableBase) == KPROCESS_DIRECTORY_TABLE_BASE);
C_ASSERT(FIELD_OFFSET(KPCR, Tib.ExceptionList) == KPCR_EXCEPTION_LIST);

C_ASSERT(FIELD_OFFSET(KPCR, Self) == KPCR_SELF);
#ifdef _M_IX86
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
C_ASSERT(FIELD_OFFSET(KV86M_TRAP_FRAME, SavedExceptionStack) == TF_SAVED_EXCEPTION_STACK);
C_ASSERT(FIELD_OFFSET(KV86M_TRAP_FRAME, regs) == TF_REGS);
C_ASSERT(FIELD_OFFSET(KV86M_TRAP_FRAME, orig_ebp) == TF_ORIG_EBP);
C_ASSERT(FIELD_OFFSET(KTSS, Esp0) == KTSS_ESP0);
C_ASSERT(FIELD_OFFSET(KTSS, IoMapBase) == KTSS_IOMAPBASE);
#endif

#endif /* INCLUDE_INTERNAL_NTOSKRNL_H */
