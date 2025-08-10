/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ARM64 miscellaneous assembly functions
 * COPYRIGHT:   Copyright 2024 ReactOS Team
 */

#include <asm.inc>

.text

/* ARM64 specific utility functions */

/* Legacy compatibility functions - not applicable to ARM64 but needed for linking */
PUBLIC DiskStopFloppyMotor
DiskStopFloppyMotor:
    /* No floppy motor to stop on ARM64 */
    ret

PUBLIC Int386
Int386:
    /* Return failure - no BIOS interrupts on ARM64 */
    mov w0, #1  /* Return non-zero to indicate failure */
    ret

PUBLIC EnableA20
EnableA20:
    /* A20 line not relevant on ARM64 */
    ret

PUBLIC ChainLoadBiosBootSectorCode
ChainLoadBiosBootSectorCode:
    /* Chainloading not supported on ARM64 UEFI */
    ret

PUBLIC Relocator16Boot
Relocator16Boot:
    /* 16-bit mode not available on ARM64 */
    ret

/* ARM64 specific memory and system functions */

/* Enable/Disable interrupts */
PUBLIC Arm64EnableInterrupts
Arm64EnableInterrupts:
    msr daifclr, #0xF  /* Clear DAIF flags (enable all interrupts) */
    ret

PUBLIC Arm64DisableInterrupts
Arm64DisableInterrupts:
    msr daifset, #0xF  /* Set DAIF flags (disable all interrupts) */
    ret

/* Get/Set interrupt state */
PUBLIC Arm64SaveAndDisableInterrupts
Arm64SaveAndDisableInterrupts:
    mrs x0, daif       /* Read current interrupt state */
    msr daifset, #0xF  /* Disable interrupts */
    ret                /* Return previous state in x0 */

PUBLIC Arm64RestoreInterrupts
Arm64RestoreInterrupts:
    msr daif, x0       /* Restore interrupt state from x0 */
    ret

/* Advanced ARM64 cache operations */
PUBLIC FlushDataCache
FlushDataCache:
    /* x0 = address to flush */
    dc civac, x0       /* Clean and invalidate by address */
    dsb sy
    ret

PUBLIC InvalidateInstructionCache
InvalidateInstructionCache:
    /* Invalidate entire instruction cache */
    ic iallu
    dsb sy
    isb
    ret

PUBLIC FlushTlb
FlushTlb:
    /* Flush entire TLB */
    tlbi vmalle1
    dsb sy
    isb
    ret

/* Comprehensive cache management */
PUBLIC Arm64CleanDataCacheRange
Arm64CleanDataCacheRange:
    /* x0 = start address, x1 = end address */
    mrs x3, ctr_el0
    ubfx x3, x3, #16, #4    /* Extract DminLine */
    mov x2, #4
    lsl x2, x2, x3          /* Cache line size */
    
    sub x3, x2, #1
    bic x0, x0, x3          /* Align start to cache line */
    
1:  dc cvac, x0            /* Clean by address */
    add x0, x0, x2
    cmp x0, x1
    b.lo 1b
    
    dsb sy
    ret

PUBLIC Arm64InvalidateDataCacheRange
Arm64InvalidateDataCacheRange:
    /* x0 = start address, x1 = end address */
    mrs x3, ctr_el0
    ubfx x3, x3, #16, #4    /* Extract DminLine */
    mov x2, #4
    lsl x2, x2, x3          /* Cache line size */
    
    sub x3, x2, #1
    bic x0, x0, x3          /* Align start to cache line */
    
1:  dc ivac, x0            /* Invalidate by address */
    add x0, x0, x2
    cmp x0, x1
    b.lo 1b
    
    dsb sy
    ret

PUBLIC Arm64CleanInvalidateDataCacheRange
Arm64CleanInvalidateDataCacheRange:
    /* x0 = start address, x1 = end address */
    mrs x3, ctr_el0
    ubfx x3, x3, #16, #4    /* Extract DminLine */
    mov x2, #4
    lsl x2, x2, x3          /* Cache line size */
    
    sub x3, x2, #1
    bic x0, x0, x3          /* Align start to cache line */
    
1:  dc civac, x0           /* Clean and invalidate by address */
    add x0, x0, x2
    cmp x0, x1
    b.lo 1b
    
    dsb sy
    ret

/* TLB operations by address */
PUBLIC Arm64InvalidateTlbByAddress
Arm64InvalidateTlbByAddress:
    /* x0 = virtual address */
    lsr x0, x0, #12         /* Convert to page number */
    tlbi vaae1, x0          /* Invalidate by address */
    dsb sy
    isb
    ret

/* Stack and register operations */
PUBLIC GetStackPointer
GetStackPointer:
    mov x0, sp
    ret

PUBLIC SetStackPointer
SetStackPointer:
    mov sp, x0
    ret

PUBLIC GetFramePointer
GetFramePointer:
    mov x0, x29
    ret

/* ARM64 system register access */
PUBLIC GetCpuId
GetCpuId:
    /* Read MIDR_EL1 register */
    mrs x0, midr_el1
    ret

PUBLIC GetCurrentEL
GetCurrentEL:
    /* Get current exception level */
    mrs x0, CurrentEL
    lsr x0, x0, #2          /* Extract EL field */
    ret

PUBLIC Arm64GetSctlr
Arm64GetSctlr:
    mrs x0, sctlr_el1
    ret

PUBLIC Arm64SetSctlr
Arm64SetSctlr:
    msr sctlr_el1, x0
    isb
    ret

PUBLIC Arm64GetTcr
Arm64GetTcr:
    mrs x0, tcr_el1
    ret

PUBLIC Arm64SetTcr
Arm64SetTcr:
    msr tcr_el1, x0
    isb
    ret

PUBLIC Arm64GetMair
Arm64GetMair:
    mrs x0, mair_el1
    ret

PUBLIC Arm64SetMair
Arm64SetMair:
    msr mair_el1, x0
    isb
    ret

PUBLIC Arm64GetTtbr0
Arm64GetTtbr0:
    mrs x0, ttbr0_el1
    ret

PUBLIC Arm64SetTtbr0
Arm64SetTtbr0:
    msr ttbr0_el1, x0
    isb
    ret

PUBLIC Arm64GetTtbr1
Arm64GetTtbr1:
    mrs x0, ttbr1_el1
    ret

PUBLIC Arm64SetTtbr1
Arm64SetTtbr1:
    msr ttbr1_el1, x0
    isb
    ret

/* Performance monitoring */
PUBLIC Arm64GetCycleCount
Arm64GetCycleCount:
    mrs x0, pmccntr_el0
    ret

PUBLIC Arm64ResetCycleCount
Arm64ResetCycleCount:
    mov x0, #0
    msr pmccntr_el0, x0
    ret

/* Atomic operations */
PUBLIC Arm64AtomicIncrement
Arm64AtomicIncrement:
    /* x0 = pointer to value */
1:  ldxr w1, [x0]          /* Load exclusive */
    add w1, w1, #1         /* Increment */
    stxr w2, w1, [x0]      /* Store exclusive */
    cbnz w2, 1b            /* Retry if failed */
    mov w0, w1             /* Return new value */
    ret

PUBLIC Arm64AtomicDecrement
Arm64AtomicDecrement:
    /* x0 = pointer to value */
1:  ldxr w1, [x0]          /* Load exclusive */
    sub w1, w1, #1         /* Decrement */
    stxr w2, w1, [x0]      /* Store exclusive */
    cbnz w2, 1b            /* Retry if failed */
    mov w0, w1             /* Return new value */
    ret

PUBLIC Arm64AtomicCompareExchange
Arm64AtomicCompareExchange:
    /* x0 = pointer, x1 = compare value, x2 = exchange value */
1:  ldxr w3, [x0]          /* Load exclusive */
    cmp w3, w1             /* Compare with expected */
    b.ne 2f                /* Not equal, fail */
    stxr w4, w2, [x0]      /* Store new value */
    cbnz w4, 1b            /* Retry if store failed */
    mov w0, #1             /* Success */
    ret
2:  clrex                 /* Clear exclusive monitor */
    mov w0, #0             /* Failure */
    ret

/* Memory barriers */
PUBLIC Arm64MemoryBarrier
Arm64MemoryBarrier:
    dsb sy
    ret

PUBLIC Arm64DataMemoryBarrier
Arm64DataMemoryBarrier:
    dmb sy
    ret

PUBLIC Arm64InstructionBarrier
Arm64InstructionBarrier:
    isb
    ret

/* Debugging support */
PUBLIC Arm64Breakpoint
Arm64Breakpoint:
    brk #0
    ret

PUBLIC Arm64HaltProcessor
Arm64HaltProcessor:
1:  wfi
    b 1b

END