/*
 * PROJECT:     ReactOS CRT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Floating point conversion routines
 * COPYRIGHT:   Copyright 2024 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <asm.inc>

EXTERN __ftol2:PROC

/* FUNCTIONS ***************************************************************/
.code

__real@43e0000000000000:
    .quad HEX(43e0000000000000)

PUBLIC __ftoul2_legacy
__ftoul2_legacy:

    /* Compare the fp number, passed in st(0), against (LLONG_MAX + 1)
       aka 9223372036854775808.0 (which is 0x43e0000000000000 in double format).
       If it is smaller, it fits into an __int64, so we can pass it to _ftol2.
       After this the original fp value has moved to st(1) */
    fld qword ptr [__real@43e0000000000000]
    fcom

    /* Put the comparison result bits into ax */
    fnstsw ax

    /* Here we test the bits for c0 (0x01) and c3 (0x40).
       We check the parity bit after the test. If it is set,
       an even number of bits were set.
       If both are 0, st(1) < st(0), i.e. our value is ok.
       If both are 1, the value is NaN/Inf and we let _ftol2 handle it. */
    test ah, HEX(41)
    jnp __ftoul2_legacy2

    /* Clean up the fp stack and forward to _ftol2 */
    fstp st(0)
    jmp __ftol2

__ftoul2_legacy2:

    /* Subtract (LLONG_MAX + 1) from the given fp value and put the result in st(1).
       st(0) = 9223372036854775808.0
       st(1) = original fp value - 9223372036854775808.0 */
    fsub st(1), st(0)

    /* Compare the result to (LLONG_MAX + 1) again and pop the fp stack.
       Here we check, whether c0 and c3 are both 0, indicating that st(0) > st(1),
       i.e. fp - (LLONG_MAX + 1) < (LLONG_MAX + 1) */
    fcomp
    fnstsw ax
    test ah, HEX(41)
    jnz __ftoul2_legacy3

    /* We have established that fp - (LLONG_MAX + 1) fits into an __int64,
       so pass that to _ftol2 and manually add the difference to the result */
    call __ftol2
    add edx, HEX(80000000)
    ret

__ftoul2_legacy3:

    /* The value is too large, just return the error value */
    xor eax, eax
    mov edx, HEX(80000000)
    ret

END
