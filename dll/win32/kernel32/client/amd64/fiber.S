/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        dll/win32//kernel32/client/amd64/fiber.S
 * PURPOSE:     Fiber context switch code for the amd64 architecture
 * PROGRAMMER:
 */

#include <asm.inc>

PUBLIC BaseFiberStartup
.PROC BaseFiberStartup
    UNIMPLEMENTED BaseFiberStartup
    /* FIXME: TODO */
    ret

.ENDP BaseFiberStartup


PUBLIC SwitchToFiber

SwitchToFiber:
    UNIMPLEMENTED BaseFiberStartup
    /* FIXME: TODO */
    ret

END
