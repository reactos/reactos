/*
 * PROJECT:     ReactOS vcruntime library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _chkesp
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <asm.inc>

.code

EXTERN __chkesp_failed:PROC

PUBLIC __chkesp
__chkesp:
    jnz _failed
    ret

_failed:
    jmp __chkesp_failed

END
