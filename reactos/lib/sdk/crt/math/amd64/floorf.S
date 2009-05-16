/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Implementation of tan
 * FILE:              lib/sdk/crt/math/amd64/floorf.S
 * PROGRAMMER:        Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ndk/amd64/asm.h>
#include <ndk/amd64/asmmacro.S>

.intel_syntax noprefix


.proc floorf
    /* Put parameter on the stack */
    movss [rsp - 0x10], xmm0
    fld   dword ptr [rsp]

    /* Change fpu control word to round down */
    fstcw [rsp - 0x10]
    mov   eax, [rsp - 0x10]
    or    eax, 0x00400
    and   eax, 0x0f7ff
    mov   [rsp - 0x08], eax
    fldcw [rsp - 0x08]

    /* Round to integer */
    frndint

    /* Restore fpu control word */
    fldcw [rsp - 0x10]

    fstp  dword ptr [rsp - 0x10]
    movss xmm0, [rsp - 0x10]
    ret

.endproc
