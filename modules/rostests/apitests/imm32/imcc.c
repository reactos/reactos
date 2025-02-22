/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Test for imm32 IMCC
 * COPYRIGHT:   Copyright 2021 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "precomp.h"

START_TEST(imcc)
{
    HIMCC hImcc;
    DWORD i;

    for (i = 0; i < 4; ++i)
    {
        hImcc = ImmCreateIMCC(i);
        ok_long(LocalSize(hImcc), 4);
        ok_long(ImmGetIMCCSize(hImcc), 4);
        ok_long(ImmGetIMCCLockCount(hImcc), (LocalFlags(hImcc) & LMEM_LOCKCOUNT));
        ImmDestroyIMCC(hImcc);
    }

    hImcc = ImmCreateIMCC(5);
    ok_long(LocalSize(hImcc), 5);
    ok_long(ImmGetIMCCSize(hImcc), 5);
    ok_long(ImmGetIMCCLockCount(hImcc), (LocalFlags(hImcc) & LMEM_LOCKCOUNT));
    ImmDestroyIMCC(hImcc);
}
