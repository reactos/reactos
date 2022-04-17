/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Run-Time Library
 * FILE:              lib/sdk/crt/math/i386/allmul_asm.s
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

PUBLIC __allmul

/* FUNCTIONS ***************************************************************/
.code

//
// llmul - long multiply routine
//
// Purpose:
//       Does a long multiply (same for signed/unsigned)
//       Parameters are not changed.
//
// Entry:
//       Parameters are passed on the stack:
//               1st pushed: multiplier (QWORD)
//               2nd pushed: multiplicand (QWORD)
//
// Exit:
//       EDX:EAX - product of multiplier and multiplicand
//       NOTE: parameters are removed from the stack
//
// Uses:
//       ECX
//

__allmul:

#define ALO  [esp + 4]       // stack address of a
#define AHI  [esp + 8]       // stack address of a
#define BLO  [esp + 12]      // stack address of b
#define BHI  [esp + 16]      // stack address of b

//
//       AHI, BHI : upper 32 bits of A and B
//       ALO, BLO : lower 32 bits of A and B
//
//             ALO * BLO
//       ALO * BHI
// +     BLO * AHI
// ---------------------
//

        mov     eax,AHI
        mov     ecx,BHI
        or      ecx,eax         //test for both hiwords zero.
        mov     ecx,BLO
        jnz     short hard      //both are zero, just mult ALO and BLO

        mov     eax,ALO
        mul     ecx

        ret     16              // callee restores the stack

hard:
        push    ebx

// must redefine A and B since esp has been altered

#define A2LO  [esp + 8]       // stack address of a
#define A2HI  [esp + 12]       // stack address of a
#define B2LO  [esp + 16]      // stack address of b
#define B2HI  [esp + 20]      // stack address of b

        mul     ecx             //eax has AHI, ecx has BLO, so AHI * BLO
        mov     ebx,eax         //save result

        mov     eax,A2LO
        mul     dword ptr B2HI //ALO * BHI
        add     ebx,eax         //ebx = ((ALO * BHI) + (AHI * BLO))

        mov     eax,A2LO  //ecx = BLO
        mul     ecx             //so edx:eax = ALO*BLO
        add     edx,ebx         //now edx has all the LO*HI stuff

        pop     ebx

        ret     16              // callee restores the stack

END
