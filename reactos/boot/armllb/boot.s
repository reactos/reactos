/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/armllb/boot.s
 * PURPOSE:         Implements the entry point for ARM machines
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

    .title "ARM LLB Entry Point"
    .include "ntoskrnl/include/internal/arm/kxarm.h"
    .include "ntoskrnl/include/internal/arm/ksarm.h"

    NESTED_ENTRY _start
    PROLOG_END _start
    
#ifdef _OMAP3_
    /*
     * On OMAP3, the boot is directly from TI BootROM that reads NAND flash.
     * First word is size of program to load.
     * Second word is load address of program. Since DDR is not initialized,
     * we load to SDRAM at 40200000h. Max 64K.
     */
    .word 0x8000
    .word 0x40200000
#endif

    /* Load C entrypoint and setup LLB stack */
    ldr lr, L_LlbStartup
    ldr sp, L_BootStackEnd
    bx lr
    ENTRY_END _start

L_BootStackEnd:
    .long 0x2000000
        
L_LlbStartup:
    .long LlbStartup

/* EOF */
