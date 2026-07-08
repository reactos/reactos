/*
 * PROJECT:     ReactOS Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     User32 Control Gallery
 * COPYRIGHT:   Copyright 2026 Mohammad Amin Mollazadeh <madamin@pm.me>
 */

#include "../controls.h"

/*
 * +----------------------------------------+
 * | [  Normal  ]          O Radiobutton 1  |
 * | [  Default ]          O Radiobutton 2  |
 * | [ Disabled ]          O Radiobutton 3  |
 * |                                        |
 * | [ ] Checkbox         [ Image Button ]  |
 * | [x] Auto Checkbox    [ Icon Button  ]  |
 * | [o] Three-state                        |
 * +----------------------------------------+
 */

LRESULT
ButtonPageProc(PPAGE_HOST PageHost, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return FALSE;
}
