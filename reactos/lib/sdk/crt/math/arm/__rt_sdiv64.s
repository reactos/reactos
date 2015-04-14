/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
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
    stmdb sp!,{r0,r1,r2,r3,lr}
    PROLOG_END

    /* Load pointer to stack structure into R0 */
    mov r0, sp

    /* Call the C worker function */
    adr lr, Return
    b __rt_sdiv64_worker

Return
    /* Move result data into the appropriate registers and return */
    ldmia sp!,{r0,r1,r2,r3,pc}
    ENTRY_END __rt_sdiv64

    END
/* EOF */
