/*
 * PROJECT:     ReactOS CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _clearfp
 * COPYRIGHT:   Copyright 2021 Roman Masanin <36927roma@gmail.com>
 */

#include "fpscr.h"

unsigned int _clearfp(void)
{
    ARM_FPSCR fpscr;
    unsigned int status;

    fpscr.raw = __getfp();
    status = _statusfp();

    fpscr.data.exception = fpscr.data.exception & ~ARM_CW_STATUS_MASK;

    __setfp(fpscr.raw);
    return status;
}
