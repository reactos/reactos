/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Run-Time Library
 * FILE:              lib/sdk/crt/math/i386/aulldvrm_asm.s
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

PUBLIC __aulldvrm

/* FUNCTIONS ***************************************************************/
.code

__aulldvrm:

// ulldvrm - unsigned long divide and remainder
//
// Purpose:
//       Does a unsigned long divide and remainder of the arguments.  Arguments
//       are not changed.
//
// Entry:
//       Arguments are passed on the stack:
//               1st pushed: divisor (QWORD)
//               2nd pushed: dividend (QWORD)
//
// Exit:
//       EDX:EAX contains the quotient (dividend/divisor)
//       EBX:ECX contains the remainder (divided % divisor)
//       NOTE: this routine removes the parameters from the stack.
//
// Uses:
//       ECX
//
        push    esi

// Set up the local stack and save the index registers.  When this is done
// the stack frame will look as follows (assuming that the expression a/b will
// generate a call to aulldvrm(a, b)):
//
//               -----------------
//               |               |
//               |---------------|
//               |               |
//               |--divisor (b)--|
//               |               |
//               |---------------|
//               |               |
//               |--dividend (a)-|
//               |               |
//               |---------------|
//               | return addr** |
//               |---------------|
//       ESP---->|      ESI      |
//               -----------------
//

#undef DVNDLO
#undef DVNDHI
#undef DVSRLO
#undef DVSRHI
#define DVNDLO  [esp + 8]       // stack address of dividend (a)
#define DVNDHI  [esp + 12]       // stack address of dividend (a)
#define DVSRLO  [esp + 16]      // stack address of divisor (b)
#define DVSRHI  [esp + 20]      // stack address of divisor (b)

//
// Now do the divide.  First look to see if the divisor is less than 4194304K.
// If so, then we can use a simple algorithm with word divides, otherwise
// things get a little more complex.
//

        mov     eax,DVSRHI // check to see if divisor < 4194304K
        or      eax,eax
        jnz     short .L1        // nope, gotta do this the hard way
        mov     ecx,DVSRLO // load divisor
        mov     eax,DVNDHI // load high word of dividend
        xor     edx,edx
        div     ecx             // get high order bits of quotient
        mov     ebx,eax         // save high bits of quotient
        mov     eax,DVNDLO // edx:eax <- remainder:lo word of dividend
        div     ecx             // get low order bits of quotient
        mov     esi,eax         // ebx:esi <- quotient

//
// Now we need to do a multiply so that we can compute the remainder.
//
        mov     eax,ebx         // set up high word of quotient
        mul     dword ptr DVSRLO // HIWORD(QUOT) * DVSR
        mov     ecx,eax         // save the result in ecx
        mov     eax,esi         // set up low word of quotient
        mul     dword ptr DVSRLO // LOWORD(QUOT) * DVSR
        add     edx,ecx         // EDX:EAX = QUOT * DVSR
        jmp     short .L2        // complete remainder calculation

//
// Here we do it the hard way.  Remember, eax contains DVSRHI
//

.L1:
        mov     ecx,eax         // ecx:ebx <- divisor
        mov     ebx,DVSRLO
        mov     edx,DVNDHI // edx:eax <- dividend
        mov     eax,DVNDLO
.L3:
        shr     ecx,1           // shift divisor right one bit// hi bit <- 0
        rcr     ebx,1
        shr     edx,1           // shift dividend right one bit// hi bit <- 0
        rcr     eax,1
        or      ecx,ecx
        jnz     short .L3        // loop until divisor < 4194304K
        div     ebx             // now divide, ignore remainder
        mov     esi,eax         // save quotient

//
// We may be off by one, so to check, we will multiply the quotient
// by the divisor and check the result against the orignal dividend
// Note that we must also check for overflow, which can occur if the
// dividend is close to 2**64 and the quotient is off by 1.
//

        mul     dword ptr DVSRHI // QUOT * DVSRHI
        mov     ecx,eax
        mov     eax,DVSRLO
        mul     esi             // QUOT * DVSRLO
        add     edx,ecx         // EDX:EAX = QUOT * DVSR
        jc      short .L4        // carry means Quotient is off by 1

//
// do long compare here between original dividend and the result of the
// multiply in edx:eax.  If original is larger or equal, we are ok, otherwise
// subtract one (1) from the quotient.
//

        cmp     edx,DVNDHI // compare hi words of result and original
        ja      short .L4        // if result > original, do subtract
        jb      short .L5        // if result < original, we are ok
        cmp     eax,DVNDLO // hi words are equal, compare lo words
        jbe     short .L5        // if less or equal we are ok, else subtract
.L4:
        dec     esi             // subtract 1 from quotient
        sub     eax,DVSRLO // subtract divisor from result
        sbb     edx,DVSRHI
.L5:
        xor     ebx,ebx         // ebx:esi <- quotient

.L2:
//
// Calculate remainder by subtracting the result from the original dividend.
// Since the result is already in a register, we will do the subtract in the
// opposite direction and negate the result.
//

        sub     eax,DVNDLO // subtract dividend from result
        sbb     edx,DVNDHI
        neg     edx             // otherwise, negate the result
        neg     eax
        sbb     edx,0

//
// Now we need to get the quotient into edx:eax and the remainder into ebx:ecx.
//
        mov     ecx,edx
        mov     edx,ebx
        mov     ebx,ecx
        mov     ecx,eax
        mov     eax,esi
//
// Just the cleanup left to do.  edx:eax contains the quotient.
// Restore the saved registers and return.
//

        pop     esi

        ret     16

END
