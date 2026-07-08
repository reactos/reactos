/*
 * PROJECT:     ReactOS Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     User32 Control Gallery
 * COPYRIGHT:   Copyright 2026 Mohammad Amin Mollazadeh <madamin@pm.me>
 */

#include "../controls.h"

/*
 * +------------------------------------------------+
 * | Single Selection                              |
 * | +--------------------------------------------+ |
 * | | Item 1                                     | |
 * | | Item 2                                     | |
 * | | Item 3                                     | |
 * | +--------------------------------------------+ |
 * |                                                |
 * | Multiple Selection                            |
 * | +--------------------------------------------+ |
 * | | [ ] Item A                                 | |
 * | | [x] Item B                                 | |
 * | | [x] Item C                                 | |
 * | +--------------------------------------------+ |
 * |                                                |
 * | Extended Selection                            |
 * | +--------------------------------------------+ |
 * | | Item 1                                     | |
 * | | Item 2                                     | |
 * | | Item 3                                     | |
 * | +--------------------------------------------+ |
 * +------------------------------------------------+
 */

LRESULT
ListBoxPageProc(PPAGE_HOST PageHost, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return FALSE;
}
