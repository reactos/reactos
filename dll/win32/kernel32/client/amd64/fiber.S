/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        dll/win32/kernel32/client/amd64/fiber.S
 * PURPOSE:     Fiber context switch code for the amd64 architecture
 * PROGRAMMER:
 */

#include <asm.inc>

.code64

PUBLIC BaseFiberStartup
FUNC BaseFiberStartup
    .ENDPROLOG
    int 3
    /* FIXME: TODO */
    ret

ENDFUNC


PUBLIC SwitchToFiber

FUNC SwitchToFiber
    .ENDPROLOG
    int 3
    /* FIXME: TODO */
    ret
ENDFUNC

END
