/*
 * COPYRIGHT:         BSD - See COPYING.ARM in the top level directory
 * PROJECT:           ReactOS CRT library
 * PURPOSE:           Implementation of __rt_sdiv64
 * PROGRAMMER:        Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <kxarm.h>

    IMPORT __rt_sdiv64_worker

/* CODE **********************************************************************/

    TEXTAREA

    NESTED_ENTRY __rt_sdiv64

    /* Allocate stack space and store parameters there */
    push {r0,r1,r2,r3,r4,lr}
    PROLOG_END

    /* Call the C worker function */
    mov r4,sp
    push {r4}

    bl __rt_sdiv64_worker

    pop {r4}
    /* Move result data into the appropriate registers and return */
    pop {r0,r1,r2,r3,r4,pc}
    NESTED_END __rt_sdiv64

    END
/* EOF */
