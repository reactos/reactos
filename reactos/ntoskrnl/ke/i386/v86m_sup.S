/*
 * FILE:            ntoskrnl/ke/i386/v86m_sup.S
 * COPYRIGHT:       See COPYING in the top level directory
 * PURPOSE:         Virtual 8086 (V86) Mode Support
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * NOTE:            See asmmacro.S for the V86 trap code.
 */

/* INCLUDES ******************************************************************/

#include <asm.h>
#include <internal/i386/asmmacro.S>
.intel_syntax noprefix

/* FUNCTIONS *****************************************************************/

.globl _Ki386SetupAndExitToV86Mode@4
.func Ki386SetupAndExitToV86Mode@4
_Ki386SetupAndExitToV86Mode@4:

    /* Enter V8086 mode */
    pushad
    call @KiEnterV86Mode@0
.endfunc

.globl _Ki386BiosCallReturnAddress
.func Ki386BiosCallReturnAddress
_Ki386BiosCallReturnAddress:

    /* Exit V8086 mode */
    mov ecx, ebp
    call @KiExitV86Mode@4
    mov esp, eax
    popad
.endfunc


