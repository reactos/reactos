/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ARM64 cache operations - Enhanced with U-Boot optimizations
 * COPYRIGHT:   Copyright 2024 ReactOS Team
 */

#include <asm.inc>

.text

/*
 * void __asm_dcache_level(level, invalidate_only)
 *
 * flush or invalidate one level cache.
 *
 * x0: cache level
 * x1: 0 clean & invalidate, 1 invalidate only
 * x16: FEAT_CCIDX support flag
 * x2~x9: clobbered
 */
PUBLIC __asm_dcache_level
__asm_dcache_level:
    lsl x12, x0, #1
    msr csselr_el1, x12         /* select cache level */
    isb                         /* sync change of cssidr_el1 */
    mrs x6, ccsidr_el1          /* read the new cssidr_el1 */
    ubfx x2, x6, #0, #3         /* x2 <- log2(cache line size)-4 */
    
    /* Check for FEAT_CCIDX support */
    cbz x16, 3f
    ubfx x3, x6, #3, #21        /* x3 <- number of cache ways - 1 (FEAT_CCIDX) */
    ubfx x4, x6, #32, #24       /* x4 <- number of cache sets - 1 (FEAT_CCIDX) */
    b 4f
3:
    ubfx x3, x6, #3, #10        /* x3 <- number of cache ways - 1 */
    ubfx x4, x6, #13, #15       /* x4 <- number of cache sets - 1 */
4:
    add x2, x2, #4              /* x2 <- log2(cache line size) */
    clz w5, w3                  /* bit position of #ways */
    
    /* x12 <- cache level << 1 */
    /* x2 <- line length offset */
    /* x3 <- number of cache ways - 1 */
    /* x4 <- number of cache sets - 1 */
    /* x5 <- bit position of #ways */

loop_set:
    mov x6, x3                  /* x6 <- working copy of #ways */
loop_way:
    lsl x7, x6, x5
    orr x9, x12, x7             /* map way and level to cisw value */
    lsl x7, x4, x2
    orr x9, x9, x7              /* map set number to cisw value */
    tbz w1, #0, 1f
    dc isw, x9                  /* invalidate by set/way */
    b 2f
1:  dc cisw, x9                 /* clean & invalidate by set/way */
2:  subs x6, x6, #1            /* decrement the way */
    b.ge loop_way
    subs x4, x4, #1             /* decrement the set */
    b.ge loop_set

    ret

/*
 * void __asm_dcache_all(int invalidate_only)
 *
 * x0: 0 clean & invalidate, 1 invalidate only
 *
 * flush or invalidate all data cache by SET/WAY.
 */
PUBLIC __asm_dcache_all
__asm_dcache_all:
    mov x1, x0
    dsb sy
    mrs x10, clidr_el1          /* read clidr_el1 */
    ubfx x11, x10, #24, #3      /* x11 <- loc */
    cbz x11, finished           /* if loc is 0, exit */
    mov x15, lr
    
    /* Check for FEAT_CCIDX support */
    mrs x16, id_aa64mmfr2_el1
    ubfx x16, x16, #20, #4      /* save FEAT_CCIDX identifier in x16 */
    
    mov x0, #0                  /* start flush at cache level 0 */
    /* x0  <- cache level */
    /* x10 <- clidr_el1 */
    /* x11 <- loc */
    /* x15 <- return address */

loop_level:
    add x12, x0, x0, lsl #1     /* x12 <- tripled cache level */
    lsr x12, x10, x12
    and x12, x12, #7            /* x12 <- cache type */
    cmp x12, #2
    b.lt skip                   /* skip if no cache or icache */
    bl __asm_dcache_level       /* x1 = 0 flush, 1 invalidate */
skip:
    add x0, x0, #1              /* increment cache level */
    cmp x11, x0
    b.gt loop_level
    mov x0, #0
    msr csselr_el1, x0          /* restore csselr_el1 */
    dsb sy
    isb
    mov lr, x15
finished:
    ret

/*
 * void __asm_invalidate_icache_all(void)
 *
 * invalidate all instruction caches to PoU
 */
PUBLIC __asm_invalidate_icache_all
__asm_invalidate_icache_all:
    ic iallu                    /* invalidate all instruction caches to PoU */
    dsb sy
    isb
    ret

/*
 * Enhanced cache operations based on U-Boot patterns
 */

/* Flush all data caches */
PUBLIC Arm64FlushDataCacheAll
Arm64FlushDataCacheAll:
    mov x0, #0                  /* clean & invalidate */
    b __asm_dcache_all

/* Invalidate all data caches */
PUBLIC Arm64InvalidateDataCacheAll
Arm64InvalidateDataCacheAll:
    mov x0, #1                  /* invalidate only */
    b __asm_dcache_all

/* Invalidate all instruction caches */
PUBLIC Arm64InvalidateInstructionCacheAll
Arm64InvalidateInstructionCacheAll:
    b __asm_invalidate_icache_all

/* Clean data cache range by address */
PUBLIC Arm64CleanDataCacheRange
Arm64CleanDataCacheRange:
    /* x0 = start address, x1 = end address */
    mrs x3, ctr_el0
    ubfx x3, x3, #16, #4        /* Extract DminLine */
    mov x2, #4
    lsl x2, x2, x3              /* Cache line size */
    
    sub x3, x2, #1
    bic x0, x0, x3              /* Align start to cache line */
    
1:  dc cvac, x0                 /* Clean by address */
    add x0, x0, x2
    cmp x0, x1
    b.lo 1b
    
    dsb sy
    ret

/* Invalidate data cache range by address */
PUBLIC Arm64InvalidateDataCacheRange
Arm64InvalidateDataCacheRange:
    /* x0 = start address, x1 = end address */
    mrs x3, ctr_el0
    ubfx x3, x3, #16, #4        /* Extract DminLine */
    mov x2, #4
    lsl x2, x2, x3              /* Cache line size */
    
    sub x3, x2, #1
    bic x0, x0, x3              /* Align start to cache line */
    
1:  dc ivac, x0                 /* Invalidate by address */
    add x0, x0, x2
    cmp x0, x1
    b.lo 1b
    
    dsb sy
    ret

/* Clean and invalidate data cache range by address */
PUBLIC Arm64CleanInvalidateDataCacheRange
Arm64CleanInvalidateDataCacheRange:
    /* x0 = start address, x1 = end address */
    mrs x3, ctr_el0
    ubfx x3, x3, #16, #4        /* Extract DminLine */
    mov x2, #4
    lsl x2, x2, x3              /* Cache line size */
    
    sub x3, x2, #1
    bic x0, x0, x3              /* Align start to cache line */
    
1:  dc civac, x0                /* Clean and invalidate by address */
    add x0, x0, x2
    cmp x0, x1
    b.lo 1b
    
    dsb sy
    ret

/* Clean data cache to Point of Coherency */
PUBLIC Arm64CleanDataCacheToPoC
Arm64CleanDataCacheToPoC:
    /* x0 = address */
    dc cvac, x0
    dsb sy
    ret

/* Clean data cache to Point of Unification */
PUBLIC Arm64CleanDataCacheToPoU
Arm64CleanDataCacheToPoU:
    /* x0 = address */
    dc cvau, x0
    dsb sy
    ret

/* Invalidate instruction cache range */
PUBLIC Arm64InvalidateInstructionCacheRange
Arm64InvalidateInstructionCacheRange:
    /* x0 = start address, x1 = end address */
    mrs x3, ctr_el0
    and x3, x3, #0xF             /* Extract IminLine */
    mov x2, #4
    lsl x2, x2, x3              /* Cache line size */
    
    sub x3, x2, #1
    bic x0, x0, x3              /* Align start to cache line */
    
1:  ic ivau, x0                 /* Invalidate instruction cache by address */
    add x0, x0, x2
    cmp x0, x1
    b.lo 1b
    
    dsb sy
    isb
    ret

/* Zero cache line */
PUBLIC Arm64ZeroCacheLine
Arm64ZeroCacheLine:
    /* x0 = address */
    dc zva, x0                  /* Zero cache line */
    ret

/* Get cache line size */
PUBLIC Arm64GetCacheLineSize
Arm64GetCacheLineSize:
    mrs x0, ctr_el0
    ubfx x1, x0, #16, #4        /* Extract DminLine (data cache) */
    and x0, x0, #0xF             /* Extract IminLine (instruction cache) */
    cmp x1, x0
    csel x0, x1, x0, gt         /* Return the larger of the two */
    mov x1, #4
    lsl x0, x1, x0              /* Convert to actual size */
    ret

/* Complete cache maintenance - U-Boot style */
PUBLIC Arm64CompleteCacheMaintenance
Arm64CompleteCacheMaintenance:
    stp x29, x30, [sp, #-16]!
    
    /* Clean all data caches */
    bl Arm64FlushDataCacheAll
    
    /* Invalidate all instruction caches */
    bl Arm64InvalidateInstructionCacheAll
    
    /* Ensure all operations complete */
    dsb sy
    isb
    
    ldp x29, x30, [sp], #16
    ret

/* Cache maintenance for code regions */
PUBLIC Arm64CacheMaintenanceForCode
Arm64CacheMaintenanceForCode:
    /* x0 = start address, x1 = end address */
    stp x29, x30, [sp, #-16]!
    
    /* Clean data cache range */
    bl Arm64CleanDataCacheRange
    
    /* Invalidate instruction cache range */
    bl Arm64InvalidateInstructionCacheRange
    
    ldp x29, x30, [sp], #16
    ret

END