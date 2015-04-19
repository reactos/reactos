/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Implementation of _global_unwind2
 * FILE:              lib/sdk/crt/except/arm/_global_unwind2.s
 * PROGRAMMER:        Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <kxarm.h>

/* CODE **********************************************************************/
    TEXTAREA

    LEAF_ENTRY _global_unwind2
    DCD 0xdefc // __assertfail
    bx lr
    LEAF_END _global_unwind2

    END
/* EOF */
