/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            boot/freeldr/arch/arm/boot.s
 * PURPOSE:         Implements the entry point for ARM machines
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

//#include <kxarm.h>

#define CPSR_IRQ_DISABLE        0x80
#define CPSR_FIQ_DISABLE        0x40
#define CPSR_THUMB_ENABLE       0x20

#define C1_MMU_CONTROL          0x01
#define C1_ALIGNMENT_CONTROL    0x02
#define C1_DCACHE_CONTROL       0x04
#define C1_ICACHE_CONTROL       0x1000
#define C1_VECTOR_CONTROL       0x2000

/* GLOBALS ********************************************************************/

.global _start
.global ArmTranslationTable
.section startup
   
/* BOOT CODE ******************************************************************/
   
_start:
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
    
/* BOOT STACK *****************************************************************/

L_BootStackEnd:
    .long BootStackEnd

L_ArmInit:
    .long ArmInit

	.align 4
BootStack:
	.space 0x4000
BootStackEnd:
    .long 0
    
/* INITIAL PAGE TABLE *********************************************************/

.section pagedata
ArmTranslationTable:
    .space 0x4000
