/*
 * PROJECT:     ReactOS CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of __rt_udiv64
 * COPYRIGHT:   Copyright 2015 Timo Kreuzer <timo.kreuzer@reactos.org>
 *              Copyright 2021 Roman Masanin <36927roma@gmail.com>
 */

/* INCLUDES ******************************************************************/

#include <kxarm.h>

    IMPORT __rt_udiv64_worker

/* CODE **********************************************************************/

    TEXTAREA

    /*
        IN: r1:r0 = divisor
        IN: r3:r2 = dividend
        OUT: r1:r0 = quotient
        OUT: r3:r2 = remainder
    */
    NESTED_ENTRY __rt_udiv64

    /* Allocate stack space and store parameters there */
    push {lr}
    sub sp,sp,0x10
    mov r12,sp
    push {r2,r3}
    PROLOG_END

    /* r0 = ret*, r2:r3 = divisor, [sp] = dividend */
    mov r3,r1
    mov r2,r0
    mov r0,r12

    /* Call the C worker function */
    bl __rt_udiv64_worker
    add sp,sp,0x08

    /* Move result data into the appropriate registers and return */
    pop {r0,r1,r2,r3,pc}
    NESTED_END __rt_udiv64

    END
/* EOF */
