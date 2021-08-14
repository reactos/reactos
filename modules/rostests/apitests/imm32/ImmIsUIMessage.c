/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Test for ImmIsUIMessage
 * COPYRIGHT:   Copyright 2021 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "precomp.h"

START_TEST(ImmIsUIMessage)
{
    UINT uMsg;
    BOOL ret;
    for (uMsg = 0x100; uMsg < 0x300; ++uMsg)
    {
        ret = ImmIsUIMessageA(NULL, uMsg, 0, 0);
        switch (uMsg)
        {
            case WM_IME_STARTCOMPOSITION: case WM_IME_ENDCOMPOSITION:
            case WM_IME_COMPOSITION: case WM_IME_SETCONTEXT: case WM_IME_NOTIFY:
            case WM_IME_COMPOSITIONFULL: case WM_IME_SELECT: case 0x287:
                ok_int(ret, TRUE);
                break;
            default:
                ok_int(ret, FALSE);
                break;
        }
    }
}
