/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS CRT library
 * PURPOSE:           Implementation of __jmp_unwind
 * PROGRAMMER:        Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <kxarm.h>

/* CODE **********************************************************************/
    TEXTAREA

    LEAF_ENTRY __jmp_unwind
    __assertfail
    bx lr
    LEAF_END __jmp_unwind

    END
/* EOF */
