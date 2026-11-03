/*
 * PROJECT:     ReactOS CRT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     ARM64 implementation of _fpreset
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include "fpcr.h"

void __cdecl _fpreset(void)
{
    /* Round to nearest, no traps, no flush to zero, no pending exceptions */
    Arm64WriteFpcr(0);
    Arm64WriteFpsr(Arm64ReadFpsr() & ~ARM64_FP_ALL);
}
