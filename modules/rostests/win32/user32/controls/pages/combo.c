/*
 * PROJECT:     ReactOS Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     User32 Control Gallery
 * COPYRIGHT:   Copyright 2026 Mohammad Amin Mollazadeh <madamin@pm.me>
 */

#include "../controls.h"

/*
 * +------------------------------------------------+
 * | Drop Down Combo                               |
 * | +--------------------------+v                 |
 * | | Apple                    |                  |
 * | +--------------------------+                  |
 * |                                               |
 * | Drop Down List                                |
 * | +--------------------------+v                 |
 * | | Banana                   |                  |
 * | +--------------------------+                  |
 * |                                               |
 * | Simple Combo                                  |
 * | +--------------------------+                  |
 * | | Item 1                   |                  |
 * | | Item 2                   |                  |
 * | | Item 3                   |                  |
 * | +--------------------------+                  |
 * +------------------------------------------------+
 */

LRESULT
ComboPageProc(PPAGE_HOST PageHost, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return FALSE;
}
