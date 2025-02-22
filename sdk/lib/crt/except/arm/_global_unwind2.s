/*
 * COPYRIGHT:         BSD - See COPYING.ARM in the top level directory
 * PROJECT:           ReactOS CRT librariy
 * PURPOSE:           Implementation of _global_unwind2
 * PROGRAMMER:        Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <kxarm.h>

/* CODE **********************************************************************/
    TEXTAREA

    LEAF_ENTRY _global_unwind2
    __assertfail
    bx lr
    LEAF_END _global_unwind2

    END
/* EOF */
