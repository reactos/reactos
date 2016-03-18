/*
 * COPYRIGHT:  See COPYING in the top level directory
 * PROJECT:    ReactOS system libraries
 * FILE:       dll/win32/kernel32/client/arm/thread.S
 * PURPOSE:    Thread and process start thunks
 * PROGRAMMER:
 */

#include <ksarm.h>

    IMPORT BaseThreadStartup
    IMPORT BaseProcessStartup

    TEXTAREA

    LEAF_ENTRY BaseThreadStartupThunk
    __assertfail
    bx lr
    LEAF_END BaseThreadStartupThunk

    LEAF_ENTRY BaseProcessStartThunk
    __assertfail
    bx lr
    LEAF_END BaseProcessStartThunk

    END
/* EOF */
