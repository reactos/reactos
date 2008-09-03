/*
 * FILE:            hal/halx86/generic/irq.S
 * COPYRIGHT:       See COPYING in the top level directory
 * PURPOSE:         Software, System and Hardware IRQ Management
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include <asm.h>
#include <internal/i386/asmmacro.S>
.intel_syntax noprefix

.macro UNIMPLEMENTED func
    jmp 2f
1:
    .ascii "Sory, asm function "
    .ascii func
    .ascii " is unimplemented!\n\0"
2:
    movabs rcx, offset 1b
    call _DbgPrint
    ret
.endm



/* GLOBALS *******************************************************************/



/* FUNCTIONS *****************************************************************/

.globl _HalpInitPICs
.func _HalpInitPICs
_HalpInitPICs:
UNIMPLEMENTED "HalpInitPICs"

.endfunc

.globl _HalClearSoftwareInterrupt
.func _HalClearSoftwareInterrupt
_HalClearSoftwareInterrupt:

    /* Get IRR mask */
    mov eax, 1
    shl eax, cl
    not eax

    /* Set IRR */
//    and gs:[KPCR_IRR], eax
    ret
.endfunc

.globl _HalRequestSoftwareInterrupt
.func _HalRequestSoftwareInterrupt
_HalRequestSoftwareInterrupt:
UNIMPLEMENTED "HalRequestSoftwareInterrupt"
.endfunc

.globl _HalBeginSystemInterrupt
.func _HalBeginSystemInterrupt
_HalBeginSystemInterrupt:
UNIMPLEMENTED "HalBeginSystemInterrupt"
.endfunc

.globl _HalpApcInterrupt
.func _HalpApcInterrupt
//TRAP_FIXUPS hapc_a, hapc_t, DoFixupV86, DoFixupAbios
_HalpApcInterrupt:
UNIMPLEMENTED "HalpApcInterrupt"
.endfunc

.globl _HalpApcInterrupt2ndEntry
.func _HalpApcInterrupt2ndEntry
_HalpApcInterrupt2ndEntry:
UNIMPLEMENTED "HalpApcInterrupt2ndEntry"
.endfunc

.globl _HalpDispatchInterrupt
.func _HalpDispatchInterrupt
//TRAP_FIXUPS hdpc_a, hdpc_t, DoFixupV86, DoFixupAbios
_HalpDispatchInterrupt:
UNIMPLEMENTED "HalpDispatchInterrupt"
.endfunc


.globl _HalpDispatchInterrupt2ndEntry
.func _HalpDispatchInterrupt2ndEntry
_HalpDispatchInterrupt2ndEntry:
UNIMPLEMENTED "HalpDispatchInterrupt2ndEntry"
.endfunc

