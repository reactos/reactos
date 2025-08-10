/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ARM64 architecture definitions
 * COPYRIGHT:   Copyright 2024 ReactOS Team
 */

#pragma once

/* ARM64 specific definitions */
#define ARM64_PAGE_SIZE     4096
#define ARM64_PAGE_SHIFT    12

/* Function prototypes */
VOID Arm64MachInit(const char *CmdLine);
VOID Arm64InitializeMMU(VOID);
VOID Arm64InitializeExceptions(VOID);
VOID Arm64DisableMMU(VOID);
BOOLEAN Arm64IsMMUEnabled(VOID);
BOOLEAN Arm64MapVirtualMemory(ULONGLONG VirtualAddress, ULONGLONG PhysicalAddress, ULONGLONG Size, ULONG Attributes);
BOOLEAN Arm64UnmapVirtualMemory(ULONGLONG VirtualAddress, ULONGLONG Size);
ULONGLONG Arm64GetPhysicalAddress(ULONGLONG VirtualAddress);
ULONG Arm64GetMemoryAttributes(ULONGLONG Address);
VOID Arm64FlushTlbRange(ULONGLONG VirtualAddress, ULONGLONG Size);

/* Exception and trap handlers */
VOID Arm64HandleSynchronousException(PARM64_CONTEXT Context, ULONGLONG Esr, ULONGLONG FaultAddr, ULONGLONG PC);
VOID Arm64HandleIrq(PARM64_CONTEXT Context);
VOID Arm64HandleFiq(PARM64_CONTEXT Context);
VOID Arm64HandleSerror(PARM64_CONTEXT Context, ULONGLONG Esr);
VOID Arm64DumpContext(PARM64_CONTEXT Context);

/* Assembly functions */
VOID Arm64EnableInterrupts(VOID);
VOID Arm64DisableInterrupts(VOID);
ULONGLONG Arm64SaveAndDisableInterrupts(VOID);
VOID Arm64RestoreInterrupts(ULONGLONG State);
VOID Arm64CleanDataCacheRange(ULONGLONG Start, ULONGLONG End);
VOID Arm64InvalidateDataCacheRange(ULONGLONG Start, ULONGLONG End);
VOID Arm64CleanInvalidateDataCacheRange(ULONGLONG Start, ULONGLONG End);
VOID Arm64InvalidateTlbByAddress(ULONGLONG Address);
ULONGLONG GetCurrentEL(VOID);
ULONGLONG Arm64GetSctlr(VOID);
VOID Arm64SetSctlr(ULONGLONG Value);
ULONGLONG Arm64GetTcr(VOID);
VOID Arm64SetTcr(ULONGLONG Value);
ULONGLONG Arm64GetMair(VOID);
VOID Arm64SetMair(ULONGLONG Value);
ULONGLONG Arm64GetTtbr0(VOID);
VOID Arm64SetTtbr0(ULONGLONG Value);
ULONGLONG Arm64GetTtbr1(VOID);
VOID Arm64SetTtbr1(ULONGLONG Value);
ULONGLONG Arm64GetCycleCount(VOID);
VOID Arm64ResetCycleCount(VOID);
ULONG Arm64AtomicIncrement(PULONG Value);
ULONG Arm64AtomicDecrement(PULONG Value);
BOOLEAN Arm64AtomicCompareExchange(PULONG Destination, ULONG Compare, ULONG Exchange);
VOID Arm64MemoryBarrier(VOID);
VOID Arm64DataMemoryBarrier(VOID);
VOID Arm64InstructionBarrier(VOID);
VOID Arm64Breakpoint(VOID);
VOID Arm64HaltProcessor(VOID);

/* Enhanced cache operations from U-Boot */
VOID __asm_dcache_level(ULONGLONG level, ULONGLONG invalidate_only);
VOID __asm_dcache_all(ULONGLONG invalidate_only);
VOID __asm_invalidate_icache_all(VOID);
VOID Arm64FlushDataCacheAll(VOID);
VOID Arm64InvalidateDataCacheAll(VOID);
VOID Arm64InvalidateInstructionCacheAll(VOID);
VOID Arm64CleanDataCacheToPoC(ULONGLONG address);
VOID Arm64CleanDataCacheToPoU(ULONGLONG address);
VOID Arm64InvalidateInstructionCacheRange(ULONGLONG Start, ULONGLONG End);
VOID Arm64ZeroCacheLine(ULONGLONG address);
ULONGLONG Arm64GetCacheLineSize(VOID);
VOID Arm64CompleteCacheMaintenance(VOID);
VOID Arm64CacheMaintenanceForCode(ULONGLONG Start, ULONGLONG End);

/* Generic Timer functions */
VOID Arm64InitializeTimer(VOID);
ULONGLONG Arm64GetTimerFrequency(VOID);
ULONGLONG Arm64GetTimerCount(VOID);
ULONGLONG Arm64GetTimerFreq(VOID);
ULONGLONG Arm64TimerTicksToMicroseconds(ULONGLONG ticks);
ULONGLONG Arm64MicrosecondsToTimerTicks(ULONGLONG microseconds);
VOID Arm64DelayMicroseconds(ULONG microseconds);
ULONGLONG Arm64GetElapsedMicroseconds(VOID);
BOOLEAN Arm64SetTimerInterrupt(ULONGLONG microseconds);
VOID Arm64DisableTimerInterrupt(VOID);
BOOLEAN Arm64IsTimerInterruptPending(VOID);
VOID Arm64HandleTimerInterrupt(VOID);
ULONG Arm64GetSystemTime(VOID);
ULONGLONG Arm64ReadPerformanceCounter(VOID);
ULONGLONG Arm64GetPerformanceFrequency(VOID);

/* ARM64 doesn't have real mode or BIOS calls */
#define FNID_Reboot                 0x00

/* ARM64 register context structure (simplified) */
typedef struct _ARM64_CONTEXT
{
    ULONGLONG X[31];  /* General purpose registers X0-X30 */
    ULONGLONG SP;     /* Stack pointer */
    ULONGLONG PC;     /* Program counter */
    ULONGLONG PSTATE; /* Processor state */
} ARM64_CONTEXT, *PARM64_CONTEXT;

/* ARM64 doesn't use segment selectors */
#define ARM64_USER_CS       0
#define ARM64_USER_DS       0
#define ARM64_KERNEL_CS     0
#define ARM64_KERNEL_DS     0

/* ARM64 cache operations */
#define ARM64_DC_CIVAC(addr) __asm__ volatile ("dc civac, %0" :: "r" (addr) : "memory")
#define ARM64_IC_IALLU()     __asm__ volatile ("ic iallu" ::: "memory")
#define ARM64_DSB_SY()       __asm__ volatile ("dsb sy" ::: "memory")
#define ARM64_ISB()          __asm__ volatile ("isb" ::: "memory")

/* Memory barrier macros */
#define ARM64_DMB_SY()       __asm__ volatile ("dmb sy" ::: "memory")
#define ARM64_DMB_ISH()      __asm__ volatile ("dmb ish" ::: "memory")

/* ARM64 system register access */
#define ARM64_READ_SYSREG(reg) ({ \
    ULONGLONG _val; \
    __asm__ volatile ("mrs %0, " #reg : "=r" (_val)); \
    _val; \
})

#define ARM64_WRITE_SYSREG(reg, val) \
    __asm__ volatile ("msr " #reg ", %0" :: "r" ((ULONGLONG)(val)))

/* Common system registers */
#define ARM64_SCTLR_EL1     "sctlr_el1"
#define ARM64_MAIR_EL1      "mair_el1"
#define ARM64_TCR_EL1       "tcr_el1"
#define ARM64_TTBR0_EL1     "ttbr0_el1"
#define ARM64_TTBR1_EL1     "ttbr1_el1"
#define ARM64_MIDR_EL1      "midr_el1"
#define ARM64_ESR_EL1       "esr_el1"
#define ARM64_FAR_EL1       "far_el1"
#define ARM64_ELR_EL1       "elr_el1"
#define ARM64_SPSR_EL1      "spsr_el1"
#define ARM64_VBAR_EL1      "vbar_el1"
#define ARM64_DAIF          "daif"
#define ARM64_CPACR_EL1     "cpacr_el1"
#define ARM64_PMCR_EL0      "pmcr_el0"
#define ARM64_PMCCNTR_EL0   "pmccntr_el0"
#define ARM64_CTR_EL0       "ctr_el0"
#define ARM64_CCSIDR_EL1    "ccsidr_el1"
#define ARM64_CSSELR_EL1    "csselr_el1"
#define ARM64_CLIDR_EL1     "clidr_el1"
#define ARM64_ID_AA64ISAR0_EL1 "id_aa64isar0_el1"
#define ARM64_ID_AA64PFR0_EL1  "id_aa64pfr0_el1"

/* Exception levels */
#define ARM64_CURRENT_EL    "CurrentEL"
#define ARM64_EL0           0
#define ARM64_EL1           1
#define ARM64_EL2           2
#define ARM64_EL3           3

/* Memory types for MAIR_EL1 */
#define ARM64_MT_DEVICE_nGnRnE  0
#define ARM64_MT_DEVICE_nGnRE   1
#define ARM64_MT_DEVICE_GRE     2
#define ARM64_MT_NORMAL_NC      3
#define ARM64_MT_NORMAL         4
#define ARM64_MT_NORMAL_WT      5

/* Page table entry bits */
#define ARM64_PTE_VALID         (1ULL << 0)
#define ARM64_PTE_TABLE         (1ULL << 1)
#define ARM64_PTE_PAGE          (1ULL << 1)
#define ARM64_PTE_BLOCK         (0ULL << 1)
#define ARM64_PTE_AP_MASK       (3ULL << 6)
#define ARM64_PTE_AP_RW_EL1     (0ULL << 6)
#define ARM64_PTE_AP_RW_ALL     (1ULL << 6)
#define ARM64_PTE_AP_RO_EL1     (2ULL << 6)
#define ARM64_PTE_AP_RO_ALL     (3ULL << 6)
#define ARM64_PTE_SH_MASK       (3ULL << 8)
#define ARM64_PTE_SH_NON        (0ULL << 8)
#define ARM64_PTE_SH_OUTER      (2ULL << 8)
#define ARM64_PTE_SH_INNER      (3ULL << 8)
#define ARM64_PTE_ATTR_MASK     (7ULL << 2)
#define ARM64_PTE_UXN           (1ULL << 54)
#define ARM64_PTE_PXN           (1ULL << 53)