/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Run-Time Library
 * FILE:              lib/sdk/crt/math/i386/allshl_asm.s
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

PUBLIC __allshl

/* FUNCTIONS ***************************************************************/
.code

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

END
