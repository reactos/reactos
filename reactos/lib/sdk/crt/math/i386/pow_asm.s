/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Run-Time Library
 * FILE:              lib/rtl/i386/pow.S
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
 
.globl _pow
 
 /* DATA ********************************************************************/

fzero:
        .long   0                       // Floating point zero
        .long   0                       // Floating point zero

.intel_syntax noprefix

/* FUNCTIONS ***************************************************************/

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
        and     ah,    0b10111010       // Turn off CF, PF and ZF
        or      ah,al                   // Set new  CF, PF and ZF
        sahf                            // Store AH into Flags
        jb      __fpow1                 // Re-direct if x < 0
        ja      __fpow2                 // Re-direct if x > 0
        fxch                            // Swap st, st(1)
        fcom    qword ptr [edi]         // Compare y with zero
        fxch                            // Restore x as top of stack
        fstsw   ax                      // Get the FPU status word
        mov     al,ah                   // Move condition flags to AL
        lahf                            // Load Flags into AH
        and     al,    0b01000101       // Isolate  C0, C2 and C3
        and     ah,    0b10111010       // Turn off CF, PF and ZF
        or      ah,al                   // Set new  CF, PF and ZF
        sahf                            // Store AH into Flags
        jmp     __fpow2                 // Re-direct
__fpow1:        fxch                            // Put y on top of stack
        fld    st                       // Duplicate y as st(1)
        frndint                         // Round to integer
        fxch                            // Put y on top of stack
        fcomp                           // y = int(y) ?
        fstsw   ax                      // Get the FPU status word
        mov     al,ah                   // Move condition flags to AL
        lahf                            // Load Flags into AH
        and     al,    0b01000101       // Isolate  C0, C2 and C3
        and     ah,    0b10111010       // Turn off CF, PF and ZF
        or      ah,al                   // Set new  CF, PF and ZF
        sahf                            // Store AH into Flags
        jne      __fpow4                 // Proceed if y = int(y)
        fist    dword ptr [ebp-12]      // Store y as integer
        and     dword ptr [ebp-12],1    // Set bit if y is odd
        fxch                            // Put x on top of stack
        fabs                            // x = |x|
__fpow2:        fldln2                          // Load log base e of 2
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
        jz      __fpow3                 // No, re-direct
        fchs                            // Negate the result
__fpow3:        fstp    qword ptr [ebp-8]       // Save (double)pow(x, y)
        fld     qword ptr [ebp-8]       // Load (double)pow(x, y)
__fpow4:        pop     eax                     // Restore register eax
        pop     edi                     // Restore register edi
        mov     esp,ebp                 // Deallocate temporary space
        pop     ebp
        ret
