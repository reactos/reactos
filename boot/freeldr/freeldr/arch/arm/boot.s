/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/freeldr/arch/arm/boot.s
 * PURPOSE:         Implements the entry point for ARM machines
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

    .title "ARM FreeLDR Entry Point"
    .include "ntoskrnl/include/internal/arm/kxarm.h"
    .include "ntoskrnl/include/internal/arm/ksarm.h"

    .section startup
    NESTED_ENTRY _start
    PROLOG_END _start
    
    //
    // C entrypoint
    //
    ldr lr, L_ArmInit

    //
    // Turn off FIQs and IRQs
    // FreeLDR runs without interrupts, and without paging, just like on x86
    //
    mrs r1, cpsr
    orr r1, r1, #CPSR_IRQ_DISABLE | CPSR_FIQ_DISABLE
    msr cpsr, r1
    
    //
    // Turn off caches and the MMU
    //
    mrc p15, 0, r1, c1, c0, 0
    bic r1, r1, #C1_DCACHE_CONTROL
    bic r1, r1, #C1_ICACHE_CONTROL
    bic r1, r1, #C1_MMU_CONTROL
    mcr p15, 0, r1, c1, c0, 0
    
    //
    // Flush everything away
    //
    mov r1, #0
    mcr p15, 0, r1, c7, c7, 0
    
    //
    // Okay, now give us a stack
    //
    ldr sp, L_BootStackEnd

    //
    // Go ahead and call the C initialization code
    // r0 contains the ARM_BOARD_CONFIGURATION_DATA structure
    //
    bx lr
    ENTRY_END _start

L_BootStackEnd:
    .long BootStackEnd

L_ArmInit:
    .long ArmInit

	.align 4
.global BootStack
BootStack:
	.space 0x4000
BootStackEnd:
    .long 0

.section pagedata
.global TranslationTableStart
TranslationTableStart:

.global ArmTranslationTable
ArmTranslationTable:
    .space 0x4000 // 0x00000000->0xFFFFFFFF

.global BootTranslationTable
BootTranslationTable:
    .space 0x0400 // 0x00000000->0x800FFFFF
    .space 0x0C00 // PADDING FOR 4KB GRANULARITY
    .space 0x0400 // 0x00100000->0x801FFFFF
    .space 0x0C00 // PADDING FOR 4KB GRANULARITY
    .space 0x0400 // 0x00200000->0x802FFFFF
    .space 0x0C00 // PADDING FOR 4KB GRANULARITY
    .space 0x0400 // 0x00300000->0x803FFFFF
    .space 0x0C00 // PADDING FOR 4KB GRANULARITY
    .space 0x0400 // 0x00400000->0x804FFFFF
    .space 0x0C00 // PADDING FOR 4KB GRANULARITY
    .space 0x0400 // 0x00500000->0x805FFFFF
    .space 0x0C00 // PADDING FOR 4KB GRANULARITY
    .space 0x0400 // 0x00600000->0x806FFFFF
    .space 0x0C00 // PADDING FOR 4KB GRANULARITY
    .space 0x0400 // 0x00700000->0x807FFFFF
    .space 0x0C00 // PADDING FOR 4KB GRANULARITY
    
.global KernelTranslationTable
KernelTranslationTable:
    .space 0x0400 // 0x00800000->0x808FFFFF
    .space 0x0C00 // PADDING FOR 4KB GRANULARITY
    .space 0x0400 // 0x00900000->0x809FFFFF
    .space 0x0C00 // PADDING FOR 4KB GRANULARITY
    .space 0x0400 // 0x00A00000->0x80AFFFFF
    .space 0x0C00 // PADDING FOR 4KB GRANULARITY
    .space 0x0400 // 0x00B00000->0x80BFFFFF
    .space 0x0C00 // PADDING FOR 4KB GRANULARITY
    .space 0x0400 // 0x00C00000->0x80CFFFFF
    .space 0x0C00 // PADDING FOR 4KB GRANULARITY
    .space 0x0400 // 0x00D00000->0x80DFFFFF
    .space 0x0C00 // PADDING FOR 4KB GRANULARITY
    
.global FlatMapTranslationTable
FlatMapTranslationTable:
    .space 0x0400 // 0xYYYYYYYY->0xC00FFFFF
    .space 0x0C00 // PADDING FOR 4KB GRANULARITY
    .space 0x0400 // 0xYYYYYYYY->0xC01FFFFF
    .space 0x0C00 // PADDING FOR 4KB GRANULARITY
    .space 0x0400 // 0xYYYYYYYY->0xC02FFFFF
    .space 0x0C00 // PADDING FOR 4KB GRANULARITY
    .space 0x0400 // 0xYYYYYYYY->0xC03FFFFF
    .space 0x0C00 // PADDING FOR 4KB GRANULARITY
    .space 0x0400 // 0xYYYYYYYY->0xC04FFFFF
    .space 0x0C00 // PADDING FOR 4KB GRANULARITY
    .space 0x0400 // 0xYYYYYYYY->0xC05FFFFF
    .space 0x0C00 // PADDING FOR 4KB GRANULARITY
    .space 0x0400 // 0xYYYYYYYY->0xC06FFFFF
    .space 0x0C00 // PADDING FOR 4KB GRANULARITY
    .space 0x0400 // 0xYYYYYYYY->0xC07FFFFF
    .space 0x0C00 // PADDING FOR 4KB GRANULARITY
    .space 0x0400 // 0xYYYYYYYY->0xC08FFFFF
    .space 0x0C00 // PADDING FOR 4KB GRANULARITY
    .space 0x0400 // 0xYYYYYYYY->0xC09FFFFF
    .space 0x0C00 // PADDING FOR 4KB GRANULARITY
    .space 0x0400 // 0xYYYYYYYY->0xC0AFFFFF
    .space 0x0C00 // PADDING FOR 4KB GRANULARITY
    .space 0x0400 // 0xYYYYYYYY->0xC0BFFFFF
    .space 0x0C00 // PADDING FOR 4KB GRANULARITY
    .space 0x0400 // 0xYYYYYYYY->0xC0CFFFFF
    .space 0x0C00 // PADDING FOR 4KB GRANULARITY
    .space 0x0400 // 0xYYYYYYYY->0xC0DFFFFF
    .space 0x0C00 // PADDING FOR 4KB GRANULARITY
    .space 0x0400 // 0xYYYYYYYY->0xC0EFFFFF
    .space 0x0C00 // PADDING FOR 4KB GRANULARITY
    .space 0x0400 // 0xYYYYYYYY->0xC0FFFFFF
    .space 0x0C00 // PADDING FOR 4KB GRANULARITY
    
.global MasterTranslationTable
MasterTranslationTable:
    .space 0x0400 // 0xYYYYYYYY->0xC10FFFFF
    .space 0x0C00 // PADDING FOR 4KB GRANULARITY

.global HyperSpaceTranslationTable
HyperSpaceTranslationTable:
    .space 0x0400 // 0xYYYYYYYY->0xC10FFFFF
    .space 0x0C00 // PADDING FOR 4KB GRANULARITY

.global TranslationTableEnd
TranslationTableEnd:
