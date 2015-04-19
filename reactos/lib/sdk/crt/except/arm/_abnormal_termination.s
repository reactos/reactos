/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Implementation of _abnormal_termination
 * FILE:              lib/sdk/crt/except/arm/_abnormal_termination.s
 * PROGRAMMER:        Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <kxarm.h>

/* CODE **********************************************************************/
    TEXTAREA

    LEAF_ENTRY _abnormal_termination
    DCD 0xdefc // __assertfail
    bx lr
    LEAF_END _abnormal_termination

    END
/* EOF */
