/*
 * PROJECT:     ReactOS CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _statusfp
 * COPYRIGHT:   Copyright 2021 Roman Masanin <36927roma@gmail.com>
 */

#include "fpscr.h"

unsigned int _statusfp(void)
{
    unsigned int flags = 0;
    ARM_FPSCR fpscr;

    fpscr.raw = __getfp();

    if (fpscr.data.exception & ARM_CW_IM) flags |= _SW_INVALID;
    if (fpscr.data.exception & ARM_CW_ZM) flags |= _SW_ZERODIVIDE;
    if (fpscr.data.exception & ARM_CW_OM) flags |= _SW_OVERFLOW;
    if (fpscr.data.exception & ARM_CW_UM) flags |= _SW_UNDERFLOW;
    if (fpscr.data.exception & ARM_CW_PM) flags |= _SW_INEXACT;
    if (fpscr.data.exception & ARM_CW_DM) flags |= _SW_DENORMAL;
    return flags;
}
