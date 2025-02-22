/*
 * COPYRIGHT:         BSD - See COPYING.ARM in the top level directory
 * PROJECT:           ReactOS CRT librariy
 * PURPOSE:           Implementation of _abnormal_termination
 * PROGRAMMER:        Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <kxarm.h>

/* CODE **********************************************************************/
    TEXTAREA

    LEAF_ENTRY _abnormal_termination
    __assertfail
    bx lr
    LEAF_END _abnormal_termination

    END
/* EOF */
