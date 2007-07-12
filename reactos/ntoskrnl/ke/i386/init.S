/*
 * FILE:            ntoskrnl/ke/i386/init.S
 * COPYRIGHT:       See COPYING in the top level directory
 * PURPOSE:         Kernel Initialization
 * PROGRAMMER:      Thomas Weidenmueller <w3seek@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <asm.h>
#include <internal/i386/asmmacro.S>
.intel_syntax noprefix

/* FUNCTIONS ******************************************************************/

.text
.globl _KiSetupStackAndInitializeKernel@24
.func KiSetupStackAndInitializeKernel@24
_KiSetupStackAndInitializeKernel@24:

    mov esi, esp

    /* Setup the new stack */
    mov esp, [esp + 12]
    sub esp, NPX_FRAME_LENGTH + KTRAP_FRAME_ALIGN + KTRAP_FRAME_LENGTH
    push CR0_EM + CR0_TS + CR0_MP

    /* Copy all parameters to the new stack */
    push [esi + 24]
    push [esi + 20]
    push [esi + 16]
    push [esi + 12]
    push [esi + 8]
    push [esi + 4]
    xor ebp, ebp
    call _KiInitializeKernel@24

    jmp _KiSystemStartupFinal@0
.endfunc
