/*
 * FILE:            hal/halx86/generic/irq.S
 * COPYRIGHT:       See COPYING in the top level directory
 * PURPOSE:         Software, System and Hardware IRQ Management
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include <reactos/asm.h>
#include <ndk/amd64/asm.h>


/* FUNCTIONS *****************************************************************/

PUBLIC HalEnableInterrupt
HalEnableInterrupt:
    UNIMPLEMENTED HalEnableInterrupt
    ret

PUBLIC HalDisableInterrupt
HalDisableInterrupt:
    UNIMPLEMENTED HalDisableInterrupt
    ret

PUBLIC HalRequestSoftwareInterrupt
HalRequestSoftwareInterrupt:
    UNIMPLEMENTED HalRequestSoftwareInterrupt
    ret

PUBLIC HalSendSoftwareInterrupt
HalSendSoftwareInterrupt:
    UNIMPLEMENTED HalSendSoftwareInterrupt
    ret

PUBLIC HalEndSystemInterrupt
HalEndSystemInterrupt:
    UNIMPLEMENTED HalEndSystemInterrupt
    ret

PUBLIC HalClearSoftwareInterrupt
HalClearSoftwareInterrupt:
    UNIMPLEMENTED HalClearSoftwareInterrupt

    /* Get IRR mask */
    mov eax, 1
    shl eax, cl
    not eax

    /* Set IRR */
//    and gs:[KPCR_IRR], eax
    ret

PUBLIC HalBeginSystemInterrupt
HalBeginSystemInterrupt:
    UNIMPLEMENTED HalBeginSystemInterrupt
    ret
