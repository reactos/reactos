/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Run-Time Library
 * FILE:              lib/sdk/crt/math/i386/ftol_asm.s
 * PROGRAMER:         Alex Ionescu (alex@relsoft.net)
 *
 * Copyright (C) 2002 Michael Ringgaard.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.

 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES// LOSS OF USE, DATA, OR PROFITS// OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <asm.inc>

PUBLIC __ftol

/* FUNCTIONS ***************************************************************/
.code

/*
 * This routine is called by MSVC-generated code to convert from floating point
 * to integer representation. The floating point number to be converted is
 * on the top of the floating point stack.
 */
__ftol:
    /* Set up stack frame */
    push ebp
    mov ebp, esp

    /* Set "round towards zero" mode */
    fstcw [ebp-2]
    wait
    mov ax, [ebp-2]
    or ah, 12
    mov [ebp-4], ax
    fldcw [ebp-4]

    /* Do the conversion */
    fistp qword ptr [ebp-12]

    /* Restore rounding mode */
    fldcw [ebp-2]

    /* Return value */
    mov eax, [ebp-12]
    mov edx, [ebp-8]

    /* Remove stack frame and return*/
    leave
    ret

END
