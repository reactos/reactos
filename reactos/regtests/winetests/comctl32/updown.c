/* Unit test suite for updown control.
 *
 * Copyright 2005 C. Scott Ananian
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
#include <stdio.h>

#include "wine/test.h"

static HDC desktopDC;
static HINSTANCE hinst;

static HWND create_edit_control (DWORD style, DWORD exstyle)
{
    HWND handle;

    handle = CreateWindowEx(exstyle,
			  "EDIT",
			  NULL,
			  ES_AUTOHSCROLL | ES_AUTOVSCROLL | style,
			  10, 10, 300, 300,
			  NULL, NULL, hinst, NULL);
    assert (handle);
    if (winetest_interactive)
	ShowWindow (handle, SW_SHOW);
    return handle;
}

static HWND create_updown_control (HWND hWndEdit)
{
    HWND hWndUpDown;

    /* make the control */
    hWndUpDown = CreateWindowEx
	(0L, UPDOWN_CLASS, NULL,
	 /* window styles */
	 UDS_SETBUDDYINT | UDS_ALIGNRIGHT |
	 UDS_ARROWKEYS | UDS_NOTHOUSANDS,
	 /* placement */
	 0, 0, 8, 8, 
	 /* parent, etc */
	 NULL, NULL, hinst, NULL);
    assert (hWndUpDown);
    /* set the buddy. */
    SendMessage (hWndUpDown, UDM_SETBUDDY, (WPARAM)hWndEdit, 0L );
    /* set the range. */
    SendMessage (hWndUpDown, UDM_SETRANGE, 0L, (LPARAM) MAKELONG(32000, 0));
    /* maybe show it. */
    if (winetest_interactive)
	ShowWindow (hWndUpDown, SW_SHOW);
    return hWndUpDown;
}

static void test_updown_control (void)
{
    HWND hWndUpDown, hWndEdit;
    int num;

    hWndEdit = create_edit_control (ES_AUTOHSCROLL | ES_NUMBER, 0);
    hWndUpDown = create_updown_control (hWndEdit);
    /* before we set a value, it should be '0' */
    num = SendMessage(hWndUpDown, UDM_GETPOS, 0, 0L);
    ok(num == 0, "Expected 0 got %d\n", num);
    /* set a value, check it. */
    SendMessage(hWndUpDown, UDM_SETPOS, 0L, MAKELONG( 1, 0));
    num = SendMessage(hWndUpDown, UDM_GETPOS, 0, 0L);
    ok(num == 1, "Expected 1 got %d\n", num);
    /* okay, done (short set of tests!) */
    DestroyWindow(hWndUpDown);
    DestroyWindow(hWndEdit);
}

START_TEST(updown)
{
    desktopDC=GetDC(NULL);
    hinst = GetModuleHandleA(NULL);

    InitCommonControls();

    test_updown_control();
}
