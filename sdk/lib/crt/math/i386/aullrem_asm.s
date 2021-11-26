/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Run-Time Library
 * FILE:              lib/sdk/crt/math/i386/aullrem_asm.s
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

PUBLIC __aullrem

/* FUNCTIONS ***************************************************************/
.code

//
// ullrem - unsigned long remainder
//
// Purpose:
//       Does a unsigned long remainder of the arguments.  Arguments are
//       not changed.
//
// Entry:
//       Arguments are passed on the stack:
//               1st pushed: divisor (QWORD)
//               2nd pushed: dividend (QWORD)
//
// Exit:
//       EDX:EAX contains the remainder (dividend%divisor)
//       NOTE: this routine removes the parameters from the stack.
//
// Uses:
//       ECX
//

__aullrem:

        push    ebx

// Set up the local stack and save the index registers.  When this is done
// the stack frame will look as follows (assuming that the expression a%b will
// generate a call to ullrem(a, b)):
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
//       ESP---->|      EBX      |
//               -----------------
//

#undef DVNDLO
#undef DVNDHI
#undef DVSRLO
#undef DVSRHI
#define DVNDLO  [esp + 8]       // stack address of dividend (a)
#define DVNDHI  [esp + 12]      // stack address of dividend (a)
#define DVSRLO  [esp + 16]      // stack address of divisor (b)
#define DVSRHI  [esp + 20]      // stack address of divisor (b)

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
        div     ecx             // edx <- remainder, eax <- quotient
        mov     eax,DVNDLO // edx:eax <- remainder:lo word of dividend
        div     ecx             // edx <- final remainder
        mov     eax,edx         // edx:eax <- remainder
        xor     edx,edx
        jmp     short .L2        // restore stack and return

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

//
// We may be off by one, so to check, we will multiply the quotient
// by the divisor and check the result against the orignal dividend
// Note that we must also check for overflow, which can occur if the
// dividend is close to 2**64 and the quotient is off by 1.
//

        mov     ecx,eax         // save a copy of quotient in ECX
        mul     dword ptr DVSRHI
        xchg    ecx,eax         // put partial product in ECX, get quotient in EAX
        mul     dword ptr DVSRLO
        add     edx,ecx         // EDX:EAX = QUOT * DVSR
        jc      short .L4        // carry means Quotient is off by 1

//
// do long compare here between original dividend and the result of the
// multiply in edx:eax.  If original is larger or equal, we're ok, otherwise
// subtract the original divisor from the result.
//

        cmp     edx,DVNDHI // compare hi words of result and original
        ja      short .L4        // if result > original, do subtract
        jb      short .L5        // if result < original, we're ok
        cmp     eax,DVNDLO // hi words are equal, compare lo words
        jbe     short .L5        // if less or equal we're ok, else subtract
.L4:
        sub     eax,DVSRLO // subtract divisor from result
        sbb     edx,DVSRHI
.L5:

//
// Calculate remainder by subtracting the result from the original dividend.
// Since the result is already in a register, we will perform the subtract in
// the opposite direction and negate the result to make it positive.
//

        sub     eax,DVNDLO // subtract original dividend from result
        sbb     edx,DVNDHI
        neg     edx             // and negate it
        neg     eax
        sbb     edx,0

//
// Just the cleanup left to do.  dx:ax contains the remainder.
// Restore the saved registers and return.
//

.L2:

        pop     ebx

        ret     16

END
