/*
 * FILE:            hal/halx86/generic/irq.S
 * COPYRIGHT:       See COPYING in the top level directory
 * PURPOSE:         Software, System and Hardware IRQ Management
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

/* Enable this (and the define in spinlock.c) to make UP HAL work for MP Kernel */
/* #define CONFIG_SMP */

#include <asm.h>
#include <internal/i386/asmmacro.S>
.intel_syntax noprefix

/* GLOBALS *******************************************************************/

_UnhandledMsg:
    .asciz "\n\x7\x7!!! Unhandled or Unexpected Code at line: %lx!!!\n"

/* FUNCTIONS *****************************************************************/

.globl _HalpApcInterrupt
.func HalpApcInterrupt
TRAP_FIXUPS hapc_a, hapc_t, DoFixupV86, DoFixupAbios
_HalpApcInterrupt:

    /* Create fake interrupt stack */
    pop eax
    pushf
    push cs
    push eax

    /* Enter interrupt */
    INT_PROLOG hapc_a, hapc_t, DoPushFakeErrorCode
.endfunc

.globl _HalpApcInterrupt2ndEntry
.func HalpApcInterrupt2ndEntry
_HalpApcInterrupt2ndEntry:

    /* Save current IRQL and set to APC level */
    push PCR[KPCR_IRQL]
    mov dword ptr PCR[KPCR_IRQL], APC_LEVEL
    and dword ptr PCR[KPCR_IRR], ~(1 << APC_LEVEL)

    /* Enable interrupts and check if we came from User/V86 mode */
    sti
    mov eax, [ebp+KTRAP_FRAME_CS]
    and eax, MODE_MASK
    test dword ptr [ebp+KTRAP_FRAME_EFLAGS], EFLAGS_V86_MASK
    jz DeliverApc

    /* Set user mode delivery */
    or eax, UserMode

DeliverApc:

    /* Deliver the APCs */
    push ebp
    push 0
    push eax
    call _KiDeliverApc@12

    /* Disable interrupts and end it */
    cli
    call _HalpEndSoftwareInterrupt@4
    jmp _Kei386EoiHelper@0
.endfunc

.globl _HalpDispatchInterrupt
.func HalpDispatchInterrupt
TRAP_FIXUPS hdpc_a, hdpc_t, DoFixupV86, DoFixupAbios
_HalpDispatchInterrupt:

    /* Create fake interrupt stack */
    pop eax
    pushf
    push cs
    push eax

    /* Enter interrupt */
    INT_PROLOG hdpc_a, hdpc_t, DoPushFakeErrorCode
.endfunc

.globl _HalpDispatchInterrupt2ndEntry
.func HalpDispatchInterrupt2ndEntry
_HalpDispatchInterrupt2ndEntry:

    /* Save current IRQL and set to DPC level */
    push PCR[KPCR_IRQL]
    mov dword ptr PCR[KPCR_IRQL], DISPATCH_LEVEL
    and dword ptr PCR[KPCR_IRR], ~(1 << DISPATCH_LEVEL)

    /* Enable interrupts and let the kernel handle this */
    sti
    call _KiDispatchInterrupt@0

    /* Disable interrupts and end it */
    cli
    call _HalpEndSoftwareInterrupt@4
    jmp _Kei386EoiHelper@0
.endfunc
