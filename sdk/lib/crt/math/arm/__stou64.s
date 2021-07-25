/*
 * PROJECT:     ReactOS CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of __stou64
 * COPYRIGHT:   Copyright 2015 Timo Kreuzer <timo.kreuzer@reactos.org>
 *              Copyright 2021 Roman Masanin <36927roma@gmail.com>
 */

/* INCLUDES ******************************************************************/

#include <kxarm.h>

    IMPORT __stou64_worker

/* CODE **********************************************************************/

    TEXTAREA

    /*
        IN: s0 = single (float) value
        OUT: r1:r0 = uint64 value
    */
    LEAF_ENTRY __stou64
    /* Allocate stack space and store parameters there */
    push {lr}
    PROLOG_END

    /* Call the C worker function */
    VMOV r0,s0
    bl __stou64_worker

    /* Move result data into the appropriate registers and return */
    pop {pc}
    LEAF_END __stou64

    END
/* EOF */
