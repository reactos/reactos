/* Unit test suite for status control.
 *
 * Copyright 2007 Google (Lei Zhang)
 * Copyright 2007 Alex Arazi
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

#define expect(expected,got) ok (expected == got,"Expected %d, got %d\n",expected,got);

static HINSTANCE hinst;

static HWND create_status_control(DWORD style, DWORD exstyle)
{
    HWND hWndStatus;

    /* make the control */
    hWndStatus = CreateWindowEx(exstyle, STATUSCLASSNAME, NULL, style,
        /* placement */
        0, 0, 300, 20,
        /* parent, etc */
        NULL, NULL, hinst, NULL);
    assert (hWndStatus);
    return hWndStatus;
}

static void test_status_control(void)
{
    HWND hWndStatus;
    int r;
    int nParts[] = {50, 150, -1};
    int checkParts[] = {0, 0, 0};
    int borders[] = {0, 0, 0};
    RECT rc;
    CHAR charArray[20];
    HICON hIcon;

    hWndStatus = create_status_control(WS_VISIBLE, 0);

    /* Divide into parts and set text */
    r = SendMessage(hWndStatus, SB_SETPARTS, 3, (long)nParts);
    expect(TRUE,r);
    r = SendMessage(hWndStatus, SB_SETTEXT, 0, (LPARAM)"First");
    expect(TRUE,r);
    r = SendMessage(hWndStatus, SB_SETTEXT, 1, (LPARAM)"Second");
    expect(TRUE,r);
    r = SendMessage(hWndStatus, SB_SETTEXT, 2, (LPARAM)"Third");
    expect(TRUE,r);

    /* Get RECT Information */
    r = SendMessage(hWndStatus, SB_GETRECT, 0, (LPARAM)&rc);
    expect(TRUE,r);
    expect(2,rc.top);
    /* The rc.bottom test is system dependent
    expect(22,rc.bottom); */
    expect(0,rc.left);
    expect(50,rc.right);
    r = SendMessage(hWndStatus, SB_GETRECT, -1, (LPARAM)&rc);
    expect(FALSE,r);
    r = SendMessage(hWndStatus, SB_GETRECT, 3, (LPARAM)&rc);
    expect(FALSE,r);
    /* Get text length and text */
    r = SendMessage(hWndStatus, SB_GETTEXTLENGTH, 2, 0);
    expect(5,LOWORD(r));
    expect(0,HIWORD(r));
    r = SendMessage(hWndStatus, SB_GETTEXT, 2, (LPARAM) charArray);
    ok(strcmp(charArray,"Third") == 0, "Expected Third, got %s\n", charArray);
    expect(5,LOWORD(r));
    expect(0,HIWORD(r));

    /* Get parts and borders */
    r = SendMessage(hWndStatus, SB_GETPARTS, 3, (long)checkParts);
    ok(r == 3, "Expected 3, got %d\n", r);
    expect(50,checkParts[0]);
    expect(150,checkParts[1]);
    expect(-1,checkParts[2]);
    r = SendMessage(hWndStatus, SB_GETBORDERS, 0, (long)borders);
    ok(r == TRUE, "Expected TRUE, got %d\n", r);
    expect(0,borders[0]);
    expect(2,borders[1]);
    expect(2,borders[2]);

    /* Test resetting text with different characters */
    r = SendMessage(hWndStatus, SB_SETTEXT, 0, (LPARAM)"First@Again");
    expect(TRUE,r);
    r = SendMessage(hWndStatus, SB_SETTEXT, 1, (LPARAM)"InvalidChars\\7\7");
        expect(TRUE,r);
    r = SendMessage(hWndStatus, SB_SETTEXT, 2, (LPARAM)"InvalidChars\\n\n");
        expect(TRUE,r);

    /* Get text again */
    r = SendMessage(hWndStatus, SB_GETTEXT, 0, (LPARAM) charArray);
    ok(strcmp(charArray,"First@Again") == 0, "Expected First@Again, got %s\n", charArray);
    expect(11,LOWORD(r));
    expect(0,HIWORD(r));
    r = SendMessage(hWndStatus, SB_GETTEXT, 1, (LPARAM) charArray);
    todo_wine
    {
        ok(strcmp(charArray,"InvalidChars\\7 ") == 0, "Expected InvalidChars\\7 , got %s\n", charArray);
    }
    expect(15,LOWORD(r));
    expect(0,HIWORD(r));
    r = SendMessage(hWndStatus, SB_GETTEXT, 2, (LPARAM) charArray);
    todo_wine
    {
        ok(strcmp(charArray,"InvalidChars\\n ") == 0, "Expected InvalidChars\\n , got %s\n", charArray);
    }
    expect(15,LOWORD(r));
    expect(0,HIWORD(r));

    /* Set background color */
    r = SendMessage(hWndStatus, SB_SETBKCOLOR , 0, RGB(255,0,0));
    expect(CLR_DEFAULT,r);
    r = SendMessage(hWndStatus, SB_SETBKCOLOR , 0, CLR_DEFAULT);
    expect(RGB(255,0,0),r);

    /* Add an icon to the status bar */
    hIcon = LoadIcon(NULL, IDI_QUESTION);
    r = SendMessage(hWndStatus, SB_SETICON, 1, (LPARAM) NULL);
    ok(r != 0, "Expected non-zero, got %d\n", r);
    r = SendMessage(hWndStatus, SB_SETICON, 1, (LPARAM) hIcon);
    ok(r != 0, "Expected non-zero, got %d\n", r);
    r = SendMessage(hWndStatus, SB_SETICON, 1, (LPARAM) NULL);
    ok(r != 0, "Expected non-zero, got %d\n", r);

    /* Set the Unicode format */
    r = SendMessage(hWndStatus, SB_SETUNICODEFORMAT, FALSE, 0);
    r = SendMessage(hWndStatus, SB_GETUNICODEFORMAT, 0, 0);
    expect(FALSE,r);
    r = SendMessage(hWndStatus, SB_SETUNICODEFORMAT, TRUE, 0);
    expect(FALSE,r);
    r = SendMessage(hWndStatus, SB_GETUNICODEFORMAT, 0, 0);
    expect(TRUE,r);

    /* Reset number of parts */
    r = SendMessage(hWndStatus, SB_SETPARTS, 2, (long)nParts);
    expect(TRUE,r);

    /* Set the minimum height and get rectangle information again */
    SendMessage(hWndStatus, SB_SETMINHEIGHT, 50, (LPARAM) 0);
    r = SendMessage(hWndStatus, WM_SIZE, 0, (LPARAM) 0);
    expect(0,r);
    r = SendMessage(hWndStatus, SB_GETRECT, 0, (LPARAM)&rc);
    expect(TRUE,r);
    expect(2,rc.top);
    /* The rc.bottom test is system dependent
    expect(22,rc.bottom); */
    expect(0,rc.left);
    expect(50,rc.right);
    r = SendMessage(hWndStatus, SB_GETRECT, -1, (LPARAM)&rc);
    expect(FALSE,r);
    r = SendMessage(hWndStatus, SB_GETRECT, 3, (LPARAM)&rc);
    expect(FALSE,r);

    /* Set the ToolTip text */
    todo_wine
    {
        SendMessage(hWndStatus, SB_SETTIPTEXT, 0,(LPARAM) "Tooltip Text");
        SendMessage(hWndStatus, SB_GETTIPTEXT, MAKEWPARAM (0, 20),(LPARAM) charArray);
        ok(strcmp(charArray,"Tooltip Text") == 0, "Expected Tooltip Text, got %s\n", charArray);
    }

    /* Make simple */
    SendMessage(hWndStatus, SB_SIMPLE, TRUE, 0);
    r = SendMessage(hWndStatus, SB_ISSIMPLE, 0, 0);
    expect(TRUE,r);

    DestroyWindow(hWndStatus);
}

START_TEST(status)
{
    hinst = GetModuleHandleA(NULL);

    InitCommonControls();

    test_status_control();
}
