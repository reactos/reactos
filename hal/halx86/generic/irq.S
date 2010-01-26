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

/* GLOBALS *******************************************************************/

/* FUNCTIONS *****************************************************************/

GENERATE_TRAP_HANDLER HalpApcInterrupt, 0, 0, 1
GENERATE_TRAP_HANDLER HalpDispatchInterrupt, 0, 0, 1