/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Helper functions for NtContinue test
 * COPYRIGHT:   Copyright 2022 Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include <asm.inc>
#include <ksamd64.inc>

.code64

EXTERN RtlCaptureContext:PROC
EXTERN check:PROC

PUBLIC continuePoint
.PROC continuePoint

    // Allocate space for a CONTEXT structure
    .ALLOCSTACK CONTEXT_FRAME_LENGTH + 8
    .ENDPROLOG

    // Capture the current CONTEXT
    mov rcx, rsp
    call RtlCaptureContext

    // Call the function that will compare the current context with the expected one
    cld
    mov rcx, rsp
    call check

    // check() must not return
    int 3

.ENDP

// EOF
END
