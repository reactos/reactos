/*
 * PROJECT:     ReactOS Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     User32 Control Gallery
 * COPYRIGHT:   Copyright 2026 Mohammad Amin Mollazadeh <madamin@pm.me>
 */

#include "../controls.h"

/*
 * +------------------------------------------------+
 * | Single Line Edit                               |
 * | +--------------------------------------------+ |
 * | | Hello world                                | |
 * | +--------------------------------------------+ |
 * |                                                |
 * | Password Edit                                  |
 * | +--------------------------------------------+ |
 * | | ********                                   | |
 * | +--------------------------------------------+ |
 * |                                                |
 * | Multiline Edit                                 |
 * | +--------------------------------------------+ |
 * | | Line 1                                     | |
 * | | Line 2                                     | |
 * | |                                            | |
 * | +--------------------------------------------+ |
 * |                                                |
 * | +--------------------------------------------+ |
 * | | Read-only Text                             | |
 * | +--------------------------------------------+ |
 * +------------------------------------------------+
 */

LRESULT
EditPageProc(PPAGE_HOST PageHost, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return FALSE;
}
