/*
 * Copyright 2005 Dmitry Timoshkov
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <assert.h>
#include <windows.h>
#include <commctrl.h>

#include "wine/test.h"

static void test_create_tooltip(void)
{
    HWND parent, hwnd;
    DWORD style, exp_style;

    parent = CreateWindowEx(0, "static", NULL, WS_POPUP,
                          0, 0, 0, 0,
                          NULL, NULL, NULL, 0);
    assert(parent);

    hwnd = CreateWindowEx(0, TOOLTIPS_CLASS, NULL, 0x7fffffff | WS_POPUP,
                          10, 10, 300, 100,
                          parent, NULL, NULL, 0);
    assert(hwnd);

    style = GetWindowLong(hwnd, GWL_STYLE);
    trace("style = %08lx\n", style);
    exp_style = 0x7fffffff | WS_POPUP;
    exp_style &= ~(WS_CHILD | WS_MAXIMIZE | WS_BORDER | WS_DLGFRAME);
    ok(style == exp_style,"wrong style %08lx/%08lx\n", style, exp_style);

    DestroyWindow(hwnd);

    hwnd = CreateWindowEx(0, TOOLTIPS_CLASS, NULL, 0,
                          10, 10, 300, 100,
                          parent, NULL, NULL, 0);
    assert(hwnd);

    style = GetWindowLong(hwnd, GWL_STYLE);
    trace("style = %08lx\n", style);
    ok(style == (WS_POPUP | WS_CLIPSIBLINGS | WS_BORDER),
       "wrong style %08lx\n", style);

    DestroyWindow(hwnd);

    DestroyWindow(parent);
}

START_TEST(tooltips)
{
    InitCommonControls();

    test_create_tooltip();
}
