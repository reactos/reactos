/*
 * PROJECT:     ReactOS CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _fpreset
 * COPYRIGHT:   Copyright 2021 Roman Masanin <36927roma@gmail.com>
 */

#include "fpscr.h"

void _fpreset(void)
{
    __setfp(0x0);
}
