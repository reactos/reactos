/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * PURPOSE:     Fiber context switch code for the amd64 architecture
 * PROGRAMMER:
 */

#include <ksarm.h>

    TEXTAREA

    LEAF_ENTRY BaseFiberStartup
    __assertfail
    bx lr
    LEAF_END BaseFiberStartup

    LEAF_ENTRY SwitchToFiber
    __assertfail
    bx lr
    LEAF_END SwitchToFiber

    END
/* EOF */
