/*
 * PROJECT:     ReactOS CRT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     ARM64 implementation of _clearfp
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include "fpcr.h"

unsigned int __cdecl _clearfp(void)
{
    unsigned int Fpsr = Arm64ReadFpsr();

    /* The condition flags and QC stay as they are */
    Arm64WriteFpsr(Fpsr & ~ARM64_FP_ALL);
    return Arm64FlagsToStatus(Fpsr);
}
