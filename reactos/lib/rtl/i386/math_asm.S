/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Run-Time Library
 * FILE:              lib/rtl/i386/math.S
 * PROGRAMER:         Alex Ionescu (alex@relsoft.net)
 *                    Eric Kohl (ekohl@rz-online.de)
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
 
/* GLOBALS ****************************************************************/

.globl __ftol
.globl __aullshr
.globl __allrem
.globl __aulldiv
.globl __allshr
.globl __allshl
.globl __aullrem
.globl __allmul
.globl __alldiv
.globl __aulldvrm
.globl __alldvrm
.globl _atan
.globl _ceil
.globl _cos
.globl _fabs
.globl _floor
.globl _log
.globl _pow
.globl _sin
.globl _sqrt
.globl _tan
.globl __fltused

/* DATA ********************************************************************/

fzero:
        .long   0                       // Floating point zero
        .long   0                       // Floating point zero

__fltused:
        .long 0x9875

.intel_syntax noprefix

/* FUNCTIONS ***************************************************************/

//
// lldiv - signed long divide
//
// Purpose:
//       Does a signed long divide of the arguments.  Arguments are
//       not changed.
//
// Entry:
//       Arguments are passed on the stack:
//               1st pushed: divisor (QWORD)
//               2nd pushed: dividend (QWORD)
//
// Exit:
//       EDX:EAX contains the quotient (dividend/divisor)
//       NOTE: this routine removes the parameters from the stack.
//
// Uses:
//       ECX
//

__alldiv:

        push    edi
        push    esi
        push    ebx

// Set up the local stack and save the index registers.  When this is done
// the stack frame will look as follows (assuming that the expression a/b will
// generate a call to lldiv(a, b)):
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
//               |      EDI      |
//               |---------------|
//               |      ESI      |
//               |---------------|
//       ESP---->|      EBX      |
//               -----------------
//

#define DVNDLO  [esp + 16]       // stack address of dividend (a)
#define DVNDHI  [esp + 20]       // stack address of dividend (a)
#define DVSRLO  [esp + 24]      // stack address of divisor (b)
#define DVSRHI  [esp + 28]      // stack address of divisor (b)

// Determine sign of the result (edi = 0 if result is positive, non-zero
// otherwise) and make operands positive.

        xor     edi,edi         // result sign assumed positive

        mov     eax,DVNDHI // hi word of a
        or      eax,eax         // test to see if signed
        jge     short L1        // skip rest if a is already positive
        inc     edi             // complement result sign flag
        mov     edx,DVNDLO // lo word of a
        neg     eax             // make a positive
        neg     edx
        sbb     eax,0
        mov     DVNDHI,eax // save positive value
        mov     DVNDLO,edx
L1:
        mov     eax,DVSRHI // hi word of b
        or      eax,eax         // test to see if signed
        jge     short L2        // skip rest if b is already positive
        inc     edi             // complement the result sign flag
        mov     edx,DVSRLO // lo word of a
        neg     eax             // make b positive
        neg     edx
        sbb     eax,0
        mov     DVSRHI,eax // save positive value
        mov     DVSRLO,edx
L2:

//
// Now do the divide.  First look to see if the divisor is less than 4194304K.
// If so, then we can use a simple algorithm with word divides, otherwise
// things get a little more complex.
//
// NOTE - eax currently contains the high order word of DVSR
//

        or      eax,eax         // check to see if divisor < 4194304K
        jnz     short L3        // nope, gotta do this the hard way
        mov     ecx,DVSRLO // load divisor
        mov     eax,DVNDHI // load high word of dividend
        xor     edx,edx
        div     ecx             // eax <- high order bits of quotient
        mov     ebx,eax         // save high bits of quotient
        mov     eax,DVNDLO // edx:eax <- remainder:lo word of dividend
        div     ecx             // eax <- low order bits of quotient
        mov     edx,ebx         // edx:eax <- quotient
        jmp     short L4        // set sign, restore stack and return

//
// Here we do it the hard way.  Remember, eax contains the high word of DVSR
//

L3:
        mov     ebx,eax         // ebx:ecx <- divisor
        mov     ecx,DVSRLO
        mov     edx,DVNDHI // edx:eax <- dividend
        mov     eax,DVNDLO
L5:
        shr     ebx,1           // shift divisor right one bit
        rcr     ecx,1
        shr     edx,1           // shift dividend right one bit
        rcr     eax,1
        or      ebx,ebx
        jnz     short L5        // loop until divisor < 4194304K
        div     ecx             // now divide, ignore remainder
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
        jc      short L6        // carry means Quotient is off by 1

//
// do long compare here between original dividend and the result of the
// multiply in edx:eax.  If original is larger or equal, we are ok, otherwise
// subtract one (1) from the quotient.
//

        cmp     edx,DVNDHI // compare hi words of result and original
        ja      short L6        // if result > original, do subtract
        jb      short L7        // if result < original, we are ok
        cmp     eax,DVNDLO // hi words are equal, compare lo words
        jbe     short L7        // if less or equal we are ok, else subtract
L6:
        dec     esi             // subtract 1 from quotient
L7:
        xor     edx,edx         // edx:eax <- quotient
        mov     eax,esi

//
// Just the cleanup left to do.  edx:eax contains the quotient.  Set the sign
// according to the save value, cleanup the stack, and return.
//

L4:
        dec     edi             // check to see if result is negative
        jnz     short L8        // if EDI == 0, result should be negative
        neg     edx             // otherwise, negate the result
        neg     eax
        sbb     edx,0

//
// Restore the saved registers and return.
//

L8:
        pop     ebx
        pop     esi
        pop     edi

        ret     16

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

        mov     eax,AHI
        mul     ecx

        ret     16              // callee restores the stack

hard:
        push    ebx

// must redefine A and B since esp has been altered

#define A2LO  [esp + 4]       // stack address of a
#define A2HI  [esp + 8]       // stack address of a
#define B2LO  [esp + 12]      // stack address of b
#define B2HI  [esp + 16]      // stack address of b

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

//
// llrem - signed long remainder
//
// Purpose:
//       Does a signed long remainder of the arguments.  Arguments are
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

__allrem :

        push    ebx
        push    edi

// Set up the local stack and save the index registers.  When this is done
// the stack frame will look as follows (assuming that the expression a%b will
// generate a call to lrem(a, b)):
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
//               |       EBX     |
//               |---------------|
//       ESP---->|       EDI     |
//               -----------------
//

#undef DVNDLO
#undef DVNDHI
#undef DVSRLO
#undef DVSRHI
#define DVNDLO  [esp + 12]       // stack address of dividend (a)
#define DVNDHI  [esp + 16]       // stack address of dividend (a)
#define DVSRLO  [esp + 20]      // stack address of divisor (b)
#define DVSRHI  [esp + 24]      // stack address of divisor (b)

// Determine sign of the result (edi = 0 if result is positive, non-zero
// otherwise) and make operands positive.

        xor     edi,edi         // result sign assumed positive

        mov     eax,DVNDHI // hi word of a
        or      eax,eax         // test to see if signed
        jge     short .L1        // skip rest if a is already positive
        inc     edi             // complement result sign flag bit
        mov     edx,DVNDLO // lo word of a
        neg     eax             // make a positive
        neg     edx
        sbb     eax,0
        mov     DVNDHI,eax // save positive value
        mov     DVNDLO,edx
.L1:
        mov     eax,DVSRHI // hi word of b
        or      eax,eax         // test to see if signed
        jge     short .L2        // skip rest if b is already positive
        mov     edx,DVSRLO // lo word of b
        neg     eax             // make b positive
        neg     edx
        sbb     eax,0
        mov     DVSRHI,eax // save positive value
        mov     DVSRLO,edx
.L2:

//
// Now do the divide.  First look to see if the divisor is less than 4194304K.
// If so, then we can use a simple algorithm with word divides, otherwise
// things get a little more complex.
//
// NOTE - eax currently contains the high order word of DVSR
//

        or      eax,eax         // check to see if divisor < 4194304K
        jnz     short .L3        // nope, gotta do this the hard way
        mov     ecx,DVSRLO // load divisor
        mov     eax,DVNDHI // load high word of dividend
        xor     edx,edx
        div     ecx             // edx <- remainder
        mov     eax,DVNDLO // edx:eax <- remainder:lo word of dividend
        div     ecx             // edx <- final remainder
        mov     eax,edx         // edx:eax <- remainder
        xor     edx,edx
        dec     edi             // check result sign flag
        jns     short .L4        // negate result, restore stack and return
        jmp     short .L8        // result sign ok, restore stack and return

//
// Here we do it the hard way.  Remember, eax contains the high word of DVSR
//

.L3:
        mov     ebx,eax         // ebx:ecx <- divisor
        mov     ecx,DVSRLO
        mov     edx,DVNDHI // edx:eax <- dividend
        mov     eax,DVNDLO
.L5:
        shr     ebx,1           // shift divisor right one bit
        rcr     ecx,1
        shr     edx,1           // shift dividend right one bit
        rcr     eax,1
        or      ebx,ebx
        jnz     short .L5        // loop until divisor < 4194304K
        div     ecx             // now divide, ignore remainder

//
// We may be off by one, so to check, we will multiply the quotient
// by the divisor and check the result against the orignal dividend
// Note that we must also check for overflow, which can occur if the
// dividend is close to 2**64 and the quotient is off by 1.
//

        mov     ecx,eax         // save a copy of quotient in ECX
        mul     dword ptr DVSRHI
        xchg    ecx,eax         // save product, get quotient in EAX
        mul     dword ptr DVSRLO
        add     edx,ecx         // EDX:EAX = QUOT * DVSR
        jc      short .L6        // carry means Quotient is off by 1

//
// do long compare here between original dividend and the result of the
// multiply in edx:eax.  If original is larger or equal, we are ok, otherwise
// subtract the original divisor from the result.
//

        cmp     edx,DVNDHI // compare hi words of result and original
        ja      short .L6        // if result > original, do subtract
        jb      short .L7        // if result < original, we are ok
        cmp     eax,DVNDLO // hi words are equal, compare lo words
        jbe     short .L7        // if less or equal we are ok, else subtract
.L6:
        sub     eax,DVSRLO // subtract divisor from result
        sbb     edx,DVSRHI
.L7:

//
// Calculate remainder by subtracting the result from the original dividend.
// Since the result is already in a register, we will do the subtract in the
// opposite direction and negate the result if necessary.
//

        sub     eax,DVNDLO // subtract dividend from result
        sbb     edx,DVNDHI

//
// Now check the result sign flag to see if the result is supposed to be positive
// or negative.  It is currently negated (because we subtracted in the 'wrong'
// direction), so if the sign flag is set we are done, otherwise we must negate
// the result to make it positive again.
//

        dec     edi             // check result sign flag
        jns     short .L8        // result is ok, restore stack and return
.L4:
        neg     edx             // otherwise, negate the result
        neg     eax
        sbb     edx,0

//
// Just the cleanup left to do.  edx:eax contains the quotient.
// Restore the saved registers and return.
//

.L8:
        pop     edi
        pop     ebx

        ret     16

//
// llshl - long shift left
//
// Purpose:
//       Does a Long Shift Left (signed and unsigned are identical)
//       Shifts a long left any number of bits.
//
// Entry:
//       EDX:EAX - long value to be shifted
//       CL      - number of bits to shift by
//
// Exit:
//       EDX:EAX - shifted value
//
// Uses:
//       CL is destroyed.
//

__allshl:

//
// Handle shifts of 64 or more bits (all get 0)
//
        cmp     cl, 64
        jae     short RETZERO

//
// Handle shifts of between 0 and 31 bits
//
        cmp     cl, 32
        jae     short MORE32
        shld    edx,eax,cl
        shl     eax,cl
        ret

//
// Handle shifts of between 32 and 63 bits
//
MORE32:
        mov     edx,eax
        xor     eax,eax
        and     cl,31
        shl     edx,cl
        ret

//
// return 0 in edx:eax
//
RETZERO:
        xor     eax,eax
        xor     edx,edx
        ret

//
// llshr - long shift right
//
// Purpose:
//       Does a signed Long Shift Right
//       Shifts a long right any number of bits.
//
// Entry:
//       EDX:EAX - long value to be shifted
//       CL      - number of bits to shift by
//
// Exit:
//       EDX:EAX - shifted value
//
// Uses:
//       CL is destroyed.
//

__allshr:

//
// Handle shifts of 64 bits or more (if shifting 64 bits or more, the result
// depends only on the high order bit of edx).
//
        cmp     cl,64
        jae     short .RETSIGN

//
// Handle shifts of between 0 and 31 bits
//
        cmp     cl, 32
        jae     short .MORE32
        shrd    eax,edx,cl
        sar     edx,cl
        ret

//
// Handle shifts of between 32 and 63 bits
//
.MORE32:
        mov     eax,edx
        sar     edx,31
        and     cl,31
        sar     eax,cl
        ret

//
// Return double precision 0 or -1, depending on the sign of edx
//
.RETSIGN:
        sar     edx,31
        mov     eax,edx
        ret

//
// ulldiv - unsigned long divide
//
// Purpose:
//       Does a unsigned long divide of the arguments.  Arguments are
//       not changed.
//
// Entry:
//       Arguments are passed on the stack:
//               1st pushed: divisor (QWORD)
//               2nd pushed: dividend (QWORD)
//
// Exit:
//       EDX:EAX contains the quotient (dividend/divisor)
//       NOTE: this routine removes the parameters from the stack.
//
// Uses:
//       ECX
//

__aulldiv:

        push    ebx
        push    esi

// Set up the local stack and save the index registers.  When this is done
// the stack frame will look as follows (assuming that the expression a/b will
// generate a call to uldiv(a, b)):
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
//               |      EBX      |
//               |---------------|
//       ESP---->|      ESI      |
//               -----------------
//

#undef DVNDLO
#undef DVNDHI
#undef DVSRLO
#undef DVSRHI
#define DVNDLO  [esp + 12]       // stack address of dividend (a)
#define DVNDHI  [esp + 16]       // stack address of dividend (a)
#define DVSRLO  [esp + 20]      // stack address of divisor (b)
#define DVSRHI  [esp + 24]      // stack address of divisor (b)

//
// Now do the divide.  First look to see if the divisor is less than 4194304K.
// If so, then we can use a simple algorithm with word divides, otherwise
// things get a little more complex.
//

        mov     eax,DVSRHI // check to see if divisor < 4194304K
        or      eax,eax
        jnz     short ..L1        // nope, gotta do this the hard way
        mov     ecx,DVSRLO // load divisor
        mov     eax,DVNDHI // load high word of dividend
        xor     edx,edx
        div     ecx             // get high order bits of quotient
        mov     ebx,eax         // save high bits of quotient
        mov     eax,DVNDLO // edx:eax <- remainder:lo word of dividend
        div     ecx             // get low order bits of quotient
        mov     edx,ebx         // edx:eax <- quotient hi:quotient lo
        jmp     short ..L2        // restore stack and return

//
// Here we do it the hard way.  Remember, eax contains DVSRHI
//

..L1:
        mov     ecx,eax         // ecx:ebx <- divisor
        mov     ebx,DVSRLO
        mov     edx,DVNDHI // edx:eax <- dividend
        mov     eax,DVNDLO
..L3:
        shr     ecx,1           // shift divisor right one bit// hi bit <- 0
        rcr     ebx,1
        shr     edx,1           // shift dividend right one bit// hi bit <- 0
        rcr     eax,1
        or      ecx,ecx
        jnz     short ..L3        // loop until divisor < 4194304K
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
        jc      short ..L4        // carry means Quotient is off by 1

//
// do long compare here between original dividend and the result of the
// multiply in edx:eax.  If original is larger or equal, we are ok, otherwise
// subtract one (1) from the quotient.
//

        cmp     edx,DVNDHI // compare hi words of result and original
        ja      short ..L4        // if result > original, do subtract
        jb      short ..L5        // if result < original, we are ok
        cmp     eax,DVNDLO // hi words are equal, compare lo words
        jbe     short ..L5        // if less or equal we are ok, else subtract
..L4:
        dec     esi             // subtract 1 from quotient
..L5:
        xor     edx,edx         // edx:eax <- quotient
        mov     eax,esi

//
// Just the cleanup left to do.  edx:eax contains the quotient.
// Restore the saved registers and return.
//

..L2:

        pop     esi
        pop     ebx

        ret     16

//
// ullshr - long shift right
//
// Purpose:
//       Does a unsigned Long Shift Right
//       Shifts a long right any number of bits.
//
// Entry:
//       EDX:EAX - long value to be shifted
//       CL      - number of bits to shift by
//
// Exit:
//       EDX:EAX - shifted value
//
// Uses:
//       CL is destroyed.
//

__aullshr:

//
// Handle shifts of 64 bits or more (if shifting 64 bits or more, the result
// depends only on the high order bit of edx).
//
        cmp     cl,64
        jae     short ..RETZERO

//
// Handle shifts of between 0 and 31 bits
//
        cmp     cl, 32
        jae     short ..MORE32
        shrd    eax,edx,cl
        shr     edx,cl
        ret

//
// Handle shifts of between 32 and 63 bits
//
..MORE32:
        mov     eax,edx
        xor     edx,edx
        and     cl,31
        shr     eax,cl
        ret

//
// return 0 in edx:eax
//
..RETZERO:
        xor     eax,eax
        xor     edx,edx
        ret

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
#define DVNDHI  [esp + 8]       // stack address of dividend (a)
#define DVSRLO  [esp + 16]      // stack address of divisor (b)
#define DVSRHI  [esp + 20]      // stack address of divisor (b)

// Now do the divide.  First look to see if the divisor is less than 4194304K.
// If so, then we can use a simple algorithm with word divides, otherwise
// things get a little more complex.
//

        mov     eax,DVSRHI // check to see if divisor < 4194304K
        or      eax,eax
        jnz     short ...L1        // nope, gotta do this the hard way
        mov     ecx,DVSRLO // load divisor
        mov     eax,DVNDHI // load high word of dividend
        xor     edx,edx
        div     ecx             // edx <- remainder, eax <- quotient
        mov     eax,DVNDLO // edx:eax <- remainder:lo word of dividend
        div     ecx             // edx <- final remainder
        mov     eax,edx         // edx:eax <- remainder
        xor     edx,edx
        jmp     short ...L2        // restore stack and return

//
// Here we do it the hard way.  Remember, eax contains DVSRHI
//

...L1:
        mov     ecx,eax         // ecx:ebx <- divisor
        mov     ebx,DVSRLO
        mov     edx,DVNDHI // edx:eax <- dividend
        mov     eax,DVNDLO
...L3:
        shr     ecx,1           // shift divisor right one bit// hi bit <- 0
        rcr     ebx,1
        shr     edx,1           // shift dividend right one bit// hi bit <- 0
        rcr     eax,1
        or      ecx,ecx
        jnz     short ...L3        // loop until divisor < 4194304K
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
        jc      short ...L4        // carry means Quotient is off by 1

//
// do long compare here between original dividend and the result of the
// multiply in edx:eax.  If original is larger or equal, we're ok, otherwise
// subtract the original divisor from the result.
//

        cmp     edx,DVNDHI // compare hi words of result and original
        ja      short ...L4        // if result > original, do subtract
        jb      short ...L5        // if result < original, we're ok
        cmp     eax,DVNDLO // hi words are equal, compare lo words
        jbe     short ...L5        // if less or equal we're ok, else subtract
...L4:
        sub     eax,DVSRLO // subtract divisor from result
        sbb     edx,DVSRHI
...L5:

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

...L2:

        pop     ebx

        ret     16


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
    or ah, 0xC
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

__alldvrm:
        push    edi
        push    esi
        push    ebp

// Set up the local stack and save the index registers.  When this is done
// the stack frame will look as follows (assuming that the expression a/b will
// generate a call to alldvrm(a, b)):
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
//               |      EDI      |
//               |---------------|
//               |      ESI      |
//               |---------------|
//       ESP---->|      EBP      |
//               -----------------
//

#undef DVNDLO
#undef DVNDHI
#undef DVSRLO
#undef DVSRHI
#define DVNDLO  [esp + 16]       // stack address of dividend (a)
#define DVNDHI  [esp + 20]       // stack address of dividend (a)
#define DVSRLO  [esp + 24]      // stack address of divisor (b)
#define DVSRHI  [esp + 28]      // stack address of divisor (b)

// Determine sign of the quotient (edi = 0 if result is positive, non-zero
// otherwise) and make operands positive.
// Sign of the remainder is kept in ebp.

        xor     edi,edi         // result sign assumed positive
        xor     ebp,ebp         // result sign assumed positive

        mov     eax,DVNDHI // hi word of a
        or      eax,eax         // test to see if signed
        jge     short ....L1        // skip rest if a is already positive
        inc     edi             // complement result sign flag
        inc     ebp             // complement result sign flag
        mov     edx,DVNDLO // lo word of a
        neg     eax             // make a positive
        neg     edx
        sbb     eax,0
        mov     DVNDHI,eax // save positive value
        mov     DVNDLO,edx
....L1:
        mov     eax,DVSRHI // hi word of b
        or      eax,eax         // test to see if signed
        jge     short ....L2        // skip rest if b is already positive
        inc     edi             // complement the result sign flag
        mov     edx,DVSRLO // lo word of a
        neg     eax             // make b positive
        neg     edx
        sbb     eax,0
        mov     DVSRHI,eax // save positive value
        mov     DVSRLO,edx
....L2:

//
// Now do the divide.  First look to see if the divisor is less than 4194304K.
// If so, then we can use a simple algorithm with word divides, otherwise
// things get a little more complex.
//
// NOTE - eax currently contains the high order word of DVSR
//

        or      eax,eax         // check to see if divisor < 4194304K
        jnz     short ....L3        // nope, gotta do this the hard way
        mov     ecx,DVSRLO // load divisor
        mov     eax,DVNDHI // load high word of dividend
        xor     edx,edx
        div     ecx             // eax <- high order bits of quotient
        mov     ebx,eax         // save high bits of quotient
        mov     eax,DVNDLO // edx:eax <- remainder:lo word of dividend
        div     ecx             // eax <- low order bits of quotient
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
        jmp     short ....L4        // complete remainder calculation

//
// Here we do it the hard way.  Remember, eax contains the high word of DVSR
//

....L3:
        mov     ebx,eax         // ebx:ecx <- divisor
        mov     ecx,DVSRLO
        mov     edx,DVNDHI // edx:eax <- dividend
        mov     eax,DVNDLO
....L5:
        shr     ebx,1           // shift divisor right one bit
        rcr     ecx,1
        shr     edx,1           // shift dividend right one bit
        rcr     eax,1
        or      ebx,ebx
        jnz     short ....L5        // loop until divisor < 4194304K
        div     ecx             // now divide, ignore remainder
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
        jc      short ....L6        // carry means Quotient is off by 1

//
// do long compare here between original dividend and the result of the
// multiply in edx:eax.  If original is larger or equal, we are ok, otherwise
// subtract one (1) from the quotient.
//

        cmp     edx,DVNDHI // compare hi words of result and original
        ja      short ....L6        // if result > original, do subtract
        jb      short ....L7        // if result < original, we are ok
        cmp     eax,DVNDLO // hi words are equal, compare lo words
        jbe     short ....L7        // if less or equal we are ok, else subtract
....L6:
        dec     esi             // subtract 1 from quotient
        sub     eax,DVSRLO // subtract divisor from result
        sbb     edx,DVSRHI
....L7:
        xor     ebx,ebx         // ebx:esi <- quotient

....L4:
//
// Calculate remainder by subtracting the result from the original dividend.
// Since the result is already in a register, we will do the subtract in the
// opposite direction and negate the result if necessary.
//

        sub     eax,DVNDLO // subtract dividend from result
        sbb     edx,DVNDHI

//
// Now check the result sign flag to see if the result is supposed to be positive
// or negative.  It is currently negated (because we subtracted in the 'wrong'
// direction), so if the sign flag is set we are done, otherwise we must negate
// the result to make it positive again.
//

        dec     ebp             // check result sign flag
        jns     short ....L9        // result is ok, set up the quotient
        neg     edx             // otherwise, negate the result
        neg     eax
        sbb     edx,0

//
// Now we need to get the quotient into edx:eax and the remainder into ebx:ecx.
//
....L9:
        mov     ecx,edx
        mov     edx,ebx
        mov     ebx,ecx
        mov     ecx,eax
        mov     eax,esi

//
// Just the cleanup left to do.  edx:eax contains the quotient.  Set the sign
// according to the save value, cleanup the stack, and return.
//

        dec     edi             // check to see if result is negative
        jnz     short ....L8        // if EDI == 0, result should be negative
        neg     edx             // otherwise, negate the result
        neg     eax
        sbb     edx,0

//
// Restore the saved registers and return.
//

....L8:
        pop     ebp
        pop     esi
        pop     edi

        ret     16

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
#define DVNDHI  [esp + 8]       // stack address of dividend (a)
#define DVSRLO  [esp + 16]      // stack address of divisor (b)
#define DVSRHI  [esp + 20]      // stack address of divisor (b)

//
// Now do the divide.  First look to see if the divisor is less than 4194304K.
// If so, then we can use a simple algorithm with word divides, otherwise
// things get a little more complex.
//

        mov     eax,DVSRHI // check to see if divisor < 4194304K
        or      eax,eax
        jnz     short .....L1        // nope, gotta do this the hard way
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
        jmp     short .....L2        // complete remainder calculation

//
// Here we do it the hard way.  Remember, eax contains DVSRHI
//

.....L1:
        mov     ecx,eax         // ecx:ebx <- divisor
        mov     ebx,DVSRLO
        mov     edx,DVNDHI // edx:eax <- dividend
        mov     eax,DVNDLO
.....L3:
        shr     ecx,1           // shift divisor right one bit// hi bit <- 0
        rcr     ebx,1
        shr     edx,1           // shift dividend right one bit// hi bit <- 0
        rcr     eax,1
        or      ecx,ecx
        jnz     short .....L3        // loop until divisor < 4194304K
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
        jc      short .....L4        // carry means Quotient is off by 1

//
// do long compare here between original dividend and the result of the
// multiply in edx:eax.  If original is larger or equal, we are ok, otherwise
// subtract one (1) from the quotient.
//

        cmp     edx,DVNDHI // compare hi words of result and original
        ja      short .....L4        // if result > original, do subtract
        jb      short .....L5        // if result < original, we are ok
        cmp     eax,DVNDLO // hi words are equal, compare lo words
        jbe     short .....L5        // if less or equal we are ok, else subtract
.....L4:
        dec     esi             // subtract 1 from quotient
        sub     eax,DVSRLO // subtract divisor from result
        sbb     edx,DVSRHI
.....L5:
        xor     ebx,ebx         // ebx:esi <- quotient

.....L2:
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

_atan:
        push    ebp
        mov     ebp,esp
        fld     qword ptr [ebp+8]       // Load real from stack
        fld1                            // Load constant 1
        fpatan                          // Take the arctangent
        pop     ebp
        ret

_ceil:
        push    ebp
        mov     ebp,esp
        sub     esp,4                   // Allocate temporary space
        fld     qword ptr [ebp+8]       // Load real from stack
        fstcw   [ebp-2]                 // Save control word
        fclex                           // Clear exceptions
        mov     word ptr [ebp-4],0xb63  // Rounding control word
        fldcw   [ebp-4]                 // Set new rounding control
        frndint                         // Round to integer
        fclex                           // Clear exceptions
        fldcw   [ebp-2]                 // Restore control word
        mov     esp,ebp                 // Deallocate temporary space
        pop     ebp
        ret

_cos:
        push    ebp
        mov     ebp,esp                 // Point to the stack frame
        fld     qword ptr [ebp+8]       // Load real from stack
        fcos                            // Take the cosine
        pop     ebp
        ret

_fabs:
        push    ebp
        mov     ebp,esp
        fld     qword ptr [ebp+8]       // Load real from stack
        fabs                            // Take the absolute value
        pop     ebp
        ret

_floor:
        push    ebp
        mov     ebp,esp
        sub     esp,4                   // Allocate temporary space
        fld     qword ptr [ebp+8]       // Load real from stack
        fstcw   [ebp-2]                 // Save control word
        fclex                           // Clear exceptions
        mov     word ptr [ebp-4],0x763  // Rounding control word
        fldcw   [ebp-4]                 // Set new rounding control
        frndint                         // Round to integer
        fclex                           // Clear exceptions
        fldcw   [ebp-2]                 // Restore control word
        mov     esp,ebp
        pop     ebp
        ret

_log:
        push    ebp
        mov     ebp,esp
        fld     qword ptr [ebp+8]       // Load real from stack
        fldln2                          // Load log base e of 2
        fxch    st(1)                   // Exchange st, st(1)
        fyl2x                           // Compute the natural log(x)
        pop     ebp
        ret

_pow:
        push    ebp
        mov     ebp,esp
        sub     esp,12                  // Allocate temporary space
        push    edi                     // Save register edi
        push    eax                     // Save register eax
        mov     dword ptr [ebp-12],0    // Set negation flag to zero
        fld     qword ptr [ebp+16]      // Load real from stack
        fld     qword ptr [ebp+8]       // Load real from stack
        mov     edi,offset flat:fzero   // Point to real zero
        fcom    qword ptr [edi]         // Compare x with zero
        fstsw   ax                      // Get the FPU status word
        mov     al,ah                   // Move condition flags to AL
        lahf                            // Load Flags into AH
        and     al,    0b01000101       // Isolate  C0, C2 and C3
        and     ah,not 0b01000101       // Turn off CF, PF and ZF
        or      ah,al                   // Set new  CF, PF and ZF
        sahf                            // Store AH into Flags
        jb      __fpow1                 // Re-direct if x < 0
        ja      __fpow3                 // Re-direct if x > 0
        fxch                            // Swap st, st(1)
        fcom    qword ptr [edi]         // Compare y with zero
        fxch                            // Restore x as top of stack
        fstsw   ax                      // Get the FPU status word
        mov     al,ah                   // Move condition flags to AL
        lahf                            // Load Flags into AH
        and     al,    0b01000101       // Isolate  C0, C2 and C3
        and     ah,not 0b01000101       // Turn off CF, PF and ZF
        or      ah,al                   // Set new  CF, PF and ZF
        sahf                            // Store AH into Flags
        ja      __fpow3                 // Re-direct if y > 0
        fstp    st(1)                   // Set new stack top and pop
        mov     eax,1                   // Set domain error (EDOM)
        jmp     __fpow5                 // End of case
__fpow1:        fxch                            // Put y on top of stack
        fld    st                       // Duplicate y as st(1)
        frndint                         // Round to integer
        fxch                            // Put y on top of stack
        fcomp                           // y = int(y) ?
        fstsw   ax                      // Get the FPU status word
        mov     al,ah                   // Move condition flags to AL
        lahf                            // Load Flags into AH
        and     al,    0b01000101       // Isolate  C0, C2 and C3
        and     ah,not 0b01000101       // Turn off CF, PF and ZF
        or      ah,al                   // Set new  CF, PF and ZF
        sahf                            // Store AH into Flags
        je      __fpow2                 // Proceed if y = int(y)
        fstp    st(1)                   // Set new stack top and pop
        fldz                            // Set result to zero
        fstp    st(1)                   // Set new stack top and pop
        mov     eax,1                   // Set domain error (EDOM)
        jmp     __fpow5                 // End of case
__fpow2:        fist    dword ptr [ebp-12]      // Store y as integer
        and     dword ptr [ebp-12],1    // Set bit if y is odd
        fxch                            // Put x on top of stack
        fabs                            // x = |x|
__fpow3:        fldln2                          // Load log base e of 2
        fxch    st(1)                   // Exchange st, st(1)
        fyl2x                           // Compute the natural log(x)
        fmulp                           // Compute y * ln(x)
        fldl2e                          // Load log base 2(e)
        fmulp   st(1),st                // Multiply x * log base 2(e)
        fst     st(1)                   // Push result
        frndint                         // Round to integer
        fsub    st(1),st                // Subtract
        fxch                            // Exchange st, st(1)
        f2xm1                           // Compute 2 to the (x - 1)
        fld1                            // Load real number 1
        faddp                           // 2 to the x
        fscale                          // Scale by power of 2
        fstp    st(1)                   // Set new stack top and pop
        test    dword ptr [ebp-12],1    // Negation required ?
        jz      __fpow4                 // No, re-direct
        fchs                            // Negate the result
__fpow4:        fstp    qword ptr [ebp-8]       // Save (double)pow(x, y)
        fld     qword ptr [ebp-8]       // Load (double)pow(x, y)
        fxam                            // Examine st
        fstsw   ax                      // Get the FPU status word
        cmp     ah,5                    // Infinity ?
        jne     __fpow6                 // No, end of case
        mov     eax,2                   // Set range error (ERANGE)
                                        // Get errno pointer offset
__fpow5:        int     3
        mov     edi,0                   // TODO: offset flat:__crt_errno
        mov     edi,[edi]               // Get C errno variable pointer
        mov     dword ptr [edi],eax     // Set errno
__fpow6:        pop     eax                     // Restore register eax
        pop     edi                     // Restore register edi
        mov     esp,ebp                 // Deallocate temporary space
        pop     ebp
        ret

_sin:
        push    ebp                     // Save register bp
        mov     ebp,esp                 // Point to the stack frame
        fld     qword ptr [ebp+8]       // Load real from stack
        fsin                            // Take the sine
        pop     ebp                     // Restore register bp
        ret

_sqrt:
        push    ebp
        mov     ebp,esp
        fld     qword ptr [ebp+8]       // Load real from stack
        fsqrt                           // Take the square root
        pop     ebp
        ret

_tan:
        push    ebp
        mov     ebp,esp
        sub     esp,4                   // Allocate temporary space
        fld     qword ptr [ebp+8]       // Load real from stack
        fptan                           // Take the tangent
        fstp    dword ptr [ebp-4]       // Throw away the constant 1
        mov     esp,ebp                 // Deallocate temporary space
        pop     ebp
        ret
