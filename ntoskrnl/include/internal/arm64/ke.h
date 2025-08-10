#pragma once

#include "intrin_i.h"

/* ARM64 Kernel Executive Definitions */

#define KiServiceExit2 KiExceptionExit

#define SYNCH_LEVEL DISPATCH_LEVEL

/* ARM64 PCR (Processor Control Region) */
#define PCR ((KPCR * const)ARM64_PCR_ADDRESS)

/* ARM64 specific addresses */
#define ARM64_PCR_ADDRESS           0xFFFFF80000000000ULL
#define ARM64_SHARED_USER_DATA      0x7FFE0000ULL

/* ARM64 breakpoint definitions */
#define KD_BREAKPOINT_TYPE          ULONG
#define KD_BREAKPOINT_SIZE          sizeof(ULONG) 
#define KD_BREAKPOINT_VALUE         0xD4200000  /* BRK #0 instruction */

/* Maximum interrupt vectors for ARM64 GIC */
#define MAXIMUM_VECTOR              1024
#define MAXIMUM_BUILTIN_VECTOR      32

/* ARM64 page size definitions */
#define PAGE_SIZE                   0x1000      /* 4KB pages */
#define PAGE_SHIFT                  12

/* ARM64 Exception Levels */
#define ARM64_EL0                   0
#define ARM64_EL1                   1
#define ARM64_EL2                   2
#define ARM64_EL3                   3

/* ARM64 System Register Access Macros */
#define ARM64_READ_SYSREG(reg) \
    ({ \
        ULONGLONG __val; \
        __asm__ volatile("mrs %0, " #reg : "=r" (__val)); \
        __val; \
    })

#define ARM64_WRITE_SYSREG(reg, val) \
    do { \
        __asm__ volatile("msr " #reg ", %0" :: "r" ((ULONGLONG)(val))); \
    } while(0)

/* ARM64 Memory Barrier Macros */
#define ARM64_DMB_SY()      __asm__ volatile("dmb sy" ::: "memory")
#define ARM64_DSB_SY()      __asm__ volatile("dsb sy" ::: "memory")
#define ARM64_ISB()         __asm__ volatile("isb" ::: "memory")

/* Context and trap frame manipulation macros */
#define KeGetContextPc(Context) \
    ((Context)->Pc)

#define KeSetContextPc(Context, ProgramCounter) \
    ((Context)->Pc = (ProgramCounter))

#define KeGetTrapFramePc(TrapFrame) \
    ((TrapFrame)->Pc)

#define KeGetContextReturnRegister(Context) \
    ((Context)->X0)

#define KeSetContextReturnRegister(Context, ReturnValue) \
    ((Context)->X0 = (ReturnValue))

/* Macro to get trap and exception frame from thread stack */
#define KeGetTrapFrame(Thread) \
    (PKTRAP_FRAME)((ULONG_PTR)((Thread)->InitialStack) - \
                   ALIGN_UP(sizeof(KTRAP_FRAME), STACK_ALIGN) - \
                   ALIGN_UP(sizeof(FX_SAVE_AREA), STACK_ALIGN))

#define KeGetExceptionFrame(Thread) \
    (PKEXCEPTION_FRAME)((ULONG_PTR)KeGetTrapFrame(Thread) - \
                        ALIGN_UP(sizeof(KEXCEPTION_FRAME), STACK_ALIGN))

/* ARM64 Thread initialization macro */
#define KeInitializeThread(Thread, KernelStack, SystemRoutine, StartRoutine, \
                          StartContext, ContextFrame, Teb, Process) \
    KiInitializeThread(Thread, KernelStack, SystemRoutine, StartRoutine, \
                      StartContext, ContextFrame, Teb, Process)

/* ARM64 specific CPU features */
#define ARM64_FEATURE_AES           0x00000001
#define ARM64_FEATURE_SHA           0x00000002
#define ARM64_FEATURE_FP            0x00000004
#define ARM64_FEATURE_ASIMD         0x00000008
#define ARM64_FEATURE_CRC32         0x00000010
#define ARM64_FEATURE_ATOMIC        0x00000020
#define ARM64_FEATURE_RAS           0x00000040
#define ARM64_FEATURE_SVE           0x00000080

/* ARM64 cache operations */
#define ARM64_DC_CIVAC(addr)    __asm__ volatile("dc civac, %0" :: "r" (addr) : "memory")
#define ARM64_DC_CVAU(addr)     __asm__ volatile("dc cvau, %0" :: "r" (addr) : "memory")
#define ARM64_DC_CVAC(addr)     __asm__ volatile("dc cvac, %0" :: "r" (addr) : "memory")
#define ARM64_DC_IVAC(addr)     __asm__ volatile("dc ivac, %0" :: "r" (addr) : "memory")
#define ARM64_IC_IVAU(addr)     __asm__ volatile("ic ivau, %0" :: "r" (addr) : "memory")

/* ARM64 TLB operations */
#define ARM64_TLBI_VMALLE1()    __asm__ volatile("tlbi vmalle1" ::: "memory")
#define ARM64_TLBI_VAE1(addr)   __asm__ volatile("tlbi vae1, %0" :: "r" (addr) : "memory")

/* ARM64 interrupt control */
#define ARM64_DISABLE_INTERRUPTS() \
    __asm__ volatile("msr daifset, #0xF" ::: "memory")

#define ARM64_ENABLE_INTERRUPTS() \
    __asm__ volatile("msr daifclr, #0xF" ::: "memory")

#define ARM64_SAVE_DISABLE_INTERRUPTS() \
    ({ \
        ULONGLONG __daif; \
        __asm__ volatile("mrs %0, daif; msr daifset, #0xF" : "=r" (__daif) :: "memory"); \
        __daif; \
    })

#define ARM64_RESTORE_INTERRUPTS(daif) \
    __asm__ volatile("msr daif, %0" :: "r" (daif) : "memory")

/* Stack alignment for ARM64 */
#define STACK_ALIGN                 16

/* ARM64 kernel function declarations */
NTSTATUS
NTAPI
KiInitializeKernel(
    IN PKPROCESS InitProcess,
    IN PKTHREAD InitThread,
    IN PVOID IdleStack,
    IN PKPRCB Prcb,
    IN CCHAR Number,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
);

VOID
NTAPI
KiInitializeGdt(
    IN PKGDTENTRY64 Gdt
);

VOID
NTAPI
KiInitializeTss(
    IN PKTSS64 Tss,
    IN UINT64 Stack
);

VOID
NTAPI 
KiInitializeCpu(
    IN PKIPCR Pcr
);

VOID
NTAPI
KiSetCacheInformation(
    VOID
);

BOOLEAN
NTAPI
KiInitMachineDependent(
    VOID
);

VOID
NTAPI
KiInitializeInterrupts(
    VOID
);

/* ARM64 Exception and interrupt handlers */
VOID KiTrapHandler(VOID);
VOID KiInterruptHandler(VOID);
VOID KiFiqHandler(VOID);
VOID KiSerrorHandler(VOID);
VOID KiSystemCallHandler64(VOID);
VOID KiSystemCallHandler32(VOID);
VOID KiUnexpectedInterrupt(VOID);

/* ARM64 Context switching */
VOID
FASTCALL
KiSwapContext(
    IN PKTHREAD OldThread,
    IN PKTHREAD NewThread
);

VOID
KiThreadStartup(
    VOID
);

/* ARM64 User mode support */
NTSTATUS
NTAPI
KiCallUserMode(
    IN OUT PVOID *OutputBuffer,
    IN OUT PULONG OutputLength
);

/* ARM64 specific PCR access */
#define KeGetPcr() PCR
#define KeGetCurrentPrcb() (PCR->Prcb)

/* ARM64 IRQL manipulation (using GIC priority) */
#define KfLowerIrql(NewIrql) \
    _KfLowerIrql(NewIrql)

#define KfRaiseIrql(NewIrql) \
    _KfRaiseIrql(NewIrql)

/* ARM64 specific memory management helpers */
#define MmIsRecursiveIoFault() FALSE

/* ARM64 spinlock operations */
#define KiAcquireSpinLock(SpinLock) \
    while (__sync_lock_test_and_set(SpinLock, 1)) { \
        while (*(volatile LONG*)(SpinLock)) \
            __asm__ volatile("yield"); \
    }

#define KiReleaseSpinLock(SpinLock) \
    __sync_lock_release(SpinLock)

/* ARM64 atomic operations */
#define InterlockedIncrement64(ptr) __sync_add_and_fetch(ptr, 1)
#define InterlockedDecrement64(ptr) __sync_sub_and_fetch(ptr, 1)
#define InterlockedExchangeAdd64(ptr, val) __sync_fetch_and_add(ptr, val)
#define InterlockedCompareExchange64(ptr, new_val, old_val) \
    __sync_val_compare_and_swap(ptr, old_val, new_val)

/* ARM64 Generic Timer definitions */
#define ARM64_TIMER_FREQ_DEFAULT    62500000ULL  /* 62.5 MHz typical */

/* ARM64 specific debugging */
#define ARM64_BRK()                 __asm__ volatile("brk #0")
#define ARM64_WFI()                 __asm__ volatile("wfi")
#define ARM64_WFE()                 __asm__ volatile("wfe")
#define ARM64_SEV()                 __asm__ volatile("sev")
#define ARM64_SEVL()                __asm__ volatile("sevl")

#endif /* _NTOSKRNL_INCLUDE_INTERNAL_ARM64_KE_H */