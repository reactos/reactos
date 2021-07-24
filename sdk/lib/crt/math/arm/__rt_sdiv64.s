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
    push {lr}
    sub sp,sp,0x10
    mov r12,sp
    push {r12}
    PROLOG_END

    /* Call the C worker function */
    bl __rt_sdiv64_worker
    add sp,sp,0x04

    /* Move result data into the appropriate registers and return */
    pop {r0,r1,r2,r3,pc}
    NESTED_END __rt_sdiv64

    END
/* EOF */
