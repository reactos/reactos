/*
 * FILE:            hal/halx86/generic/timer.S
 * COPYRIGHT:       See COPYING in the top level directory
 * PURPOSE:         System Timer Interrupt and Management
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include <asm.h>
#include <internal/i386/asmmacro.S>
.intel_syntax noprefix

.extern _Kei386EoiHelper@0

/* GLOBALS *******************************************************************/

/* FUNCTIONS *****************************************************************/

.globl _HalpClockInterrupt@0
.func HalpClockInterrupt@0
_HalpClockInterrupt@0:

    jmp $
.endfunc
