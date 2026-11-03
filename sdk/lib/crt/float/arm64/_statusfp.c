/*
 * PROJECT:     ReactOS CRT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     ARM64 implementation of _statusfp
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include "fpcr.h"

unsigned int __cdecl _statusfp(void)
{
    return Arm64FlagsToStatus(Arm64ReadFpsr());
}
