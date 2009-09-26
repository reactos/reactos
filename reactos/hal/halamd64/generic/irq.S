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
    .ascii "Sorry, asm function "
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

.global _HalEnableInterrupt
.func _HalEnableInterrupt
_HalEnableInterrupt:
    UNIMPLEMENTED "HalEnableInterrupt"
.endfunc

.global _HalDisableInterrupt
.func _HalDisableInterrupt
_HalDisableInterrupt:
    UNIMPLEMENTED "HalDisableInterrupt"
.endfunc

.global _HalRequestSoftwareInterrupt
.func _HalRequestSoftwareInterrupt
_HalRequestSoftwareInterrupt:
    UNIMPLEMENTED "HalRequestSoftwareInterrupt"
.endfunc

.global _HalSendSoftwareInterrupt
.func _HalSendSoftwareInterrupt
_HalSendSoftwareInterrupt:
    UNIMPLEMENTED "HalSendSoftwareInterrupt"
.endfunc

.global _HalEndSystemInterrupt
.func _HalEndSystemInterrupt
_HalEndSystemInterrupt:
    UNIMPLEMENTED "HalEndSystemInterrupt"
.endfunc


.globl _HalClearSoftwareInterrupt
.func _HalClearSoftwareInterrupt
_HalClearSoftwareInterrupt:
    UNIMPLEMENTED "HalClearSoftwareInterrupt"

    /* Get IRR mask */
    mov eax, 1
    shl eax, cl
    not eax

    /* Set IRR */
//    and gs:[KPCR_IRR], eax
    ret
.endfunc

.globl _HalBeginSystemInterrupt
.func _HalBeginSystemInterrupt
_HalBeginSystemInterrupt:
    UNIMPLEMENTED "HalBeginSystemInterrupt"
.endfunc

