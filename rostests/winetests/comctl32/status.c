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

#define SUBCLASS_NAME "MyStatusBar"

#define expect(expected,got) ok (expected == got,"Expected %d, got %d\n",expected,got)
#define expect_rect(_left,_top,_right,_bottom,got) do { \
        RECT exp = {abs(got.left - _left), abs(got.top - _top), \
                    abs(got.right - _right), abs(got.bottom - _bottom)}; \
        ok(exp.left <= 2 && exp.top <= 2 && exp.right <= 2 && exp.bottom <= 2, \
           "Expected rect {%d,%d, %d,%d}, got {%d,%d, %d,%d}\n", \
           _left, _top, _right, _bottom, \
           (got).left, (got).top, (got).right, (got).bottom); } while (0)

static HINSTANCE hinst;
static WNDPROC g_status_wndproc;
static RECT g_rcCreated;
static HWND g_hMainWnd;
static int g_wmsize_count = 0;

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

static LRESULT WINAPI create_test_wndproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LRESULT ret;

    if (msg == WM_CREATE)
    {
        CREATESTRUCT *cs = (CREATESTRUCT *)lParam;
        ret = CallWindowProc(g_status_wndproc, hwnd, msg, wParam, lParam);
        GetWindowRect(hwnd, &g_rcCreated);
        MapWindowPoints(HWND_DESKTOP, g_hMainWnd, (LPPOINT)&g_rcCreated, 2);
        ok(cs->x == g_rcCreated.left, "CREATESTRUCT.x modified\n");
        ok(cs->y == g_rcCreated.top, "CREATESTRUCT.y modified\n");
    } else if (msg == WM_SIZE)
    {
        g_wmsize_count++;
        ret = CallWindowProc(g_status_wndproc, hwnd, msg, wParam, lParam);
    }
    else
        ret = CallWindowProc(g_status_wndproc, hwnd, msg, wParam, lParam);

    return ret;
}

static void register_subclass()
{
    WNDCLASSEX cls;

    cls.cbSize = sizeof(WNDCLASSEX);
    GetClassInfoEx(NULL, STATUSCLASSNAME, &cls);
    g_status_wndproc = cls.lpfnWndProc;
    cls.lpfnWndProc = create_test_wndproc;
    cls.lpszClassName = SUBCLASS_NAME;
    cls.hInstance = NULL;
    ok(RegisterClassEx(&cls), "RegisterClassEx failed\n");
}

static void test_create()
{
    RECT rc;
    HWND hwnd;

    ok((hwnd = CreateWindowA(SUBCLASS_NAME, "", WS_CHILD|WS_VISIBLE|SBARS_SIZEGRIP, 0, 0, 100, 100,
        g_hMainWnd, NULL, NULL, 0)) != NULL, "CreateWindowA failed\n");
    MapWindowPoints(HWND_DESKTOP, g_hMainWnd, (LPPOINT)&rc, 2);
    GetWindowRect(hwnd, &rc);
    MapWindowPoints(HWND_DESKTOP, g_hMainWnd, (LPPOINT)&rc, 2);
    expect_rect(0, 0, 100, 100, g_rcCreated);
    expect(0, rc.left);
    expect(672, rc.right);
    expect(226, rc.bottom);
    /* we don't check rc.top as this may depend on user font settings */
    DestroyWindow(hwnd);
}

static int CALLBACK check_height_font_enumproc(ENUMLOGFONTEX *enumlf, NEWTEXTMETRICEX *ntm, DWORD type, LPARAM lParam)
{
    HWND hwndStatus = (HWND)lParam;
    HDC hdc = GetDC(NULL);
    static const int sizes[] = {8, 9, 10, 12, 16, 22, 28, 36, 48, 72};
    int i;

    trace("Font %s\n", enumlf->elfFullName);
    for (i = 0; i < sizeof(sizes)/sizeof(sizes[0]); i++)
    {
        HFONT hFont;
        TEXTMETRIC tm;
        HFONT hCtrlFont;
        HFONT hOldFont;
        RECT rcCtrl;

        enumlf->elfLogFont.lfHeight = sizes[i];
        hFont = CreateFontIndirect(&enumlf->elfLogFont);
        hCtrlFont = (HFONT)SendMessage(hwndStatus, WM_SETFONT, (WPARAM)hFont, TRUE);
        hOldFont = SelectObject(hdc, hFont);

        GetClientRect(hwndStatus, &rcCtrl);
        GetTextMetrics(hdc, &tm);
        expect(max(tm.tmHeight + (tm.tmInternalLeading ? tm.tmInternalLeading : 2) + 4, 20), rcCtrl.bottom);

        SelectObject(hdc, hOldFont);
        SendMessage(hwndStatus, WM_SETFONT, (WPARAM)hCtrlFont, TRUE);
        DeleteObject(hFont);
    }
    ReleaseDC(NULL, hdc);
    return 1;
}

static int CALLBACK check_height_family_enumproc(ENUMLOGFONTEX *enumlf, NEWTEXTMETRICEX *ntm, DWORD type, LPARAM lParam)
{
    HDC hdc = GetDC(NULL);
    enumlf->elfLogFont.lfHeight = 0;
    EnumFontFamiliesEx(hdc, &enumlf->elfLogFont, (FONTENUMPROC)check_height_font_enumproc, lParam, 0);
    ReleaseDC(NULL, hdc);
    return 1;
}

static void test_height(void)
{
    LOGFONT lf;
    HFONT hFont, hFontSm;
    RECT rc1, rc2;
    HWND hwndStatus = CreateWindow(SUBCLASS_NAME, NULL, WS_CHILD|WS_VISIBLE,
        0, 0, 300, 20, g_hMainWnd, NULL, NULL, NULL);
    HDC hdc;

    GetClientRect(hwndStatus, &rc1);
    hFont = CreateFont(32, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE, "Tahoma");

    g_wmsize_count = 0;
    SendMessage(hwndStatus, WM_SETFONT, (WPARAM)hFont, TRUE);
    if (!g_wmsize_count)
    {
        skip("Status control not resized in win95, skipping broken tests.\n");
        return;
    }
    ok(g_wmsize_count > 0, "WM_SETFONT should issue WM_SIZE\n");

    GetClientRect(hwndStatus, &rc2);
    expect_rect(0, 0, 672, 42, rc2); /* GetTextMetrics returns invalid tmInternalLeading for this font */

    g_wmsize_count = 0;
    SendMessage(hwndStatus, WM_SETFONT, (WPARAM)hFont, TRUE);
    ok(g_wmsize_count > 0, "WM_SETFONT should issue WM_SIZE\n");

    GetClientRect(hwndStatus, &rc2);
    expect_rect(0, 0, 672, 42, rc2);

    /* minheight < fontsize - no effects*/
    SendMessage(hwndStatus, SB_SETMINHEIGHT, 12, 0);
    SendMessage(hwndStatus, WM_SIZE, 0, 0);
    GetClientRect(hwndStatus, &rc2);
    expect_rect(0, 0, 672, 42, rc2);

    /* minheight > fontsize - has an effect after WM_SIZE */
    SendMessage(hwndStatus, SB_SETMINHEIGHT, 60, 0);
    GetClientRect(hwndStatus, &rc2);
    expect_rect(0, 0, 672, 42, rc2);
    SendMessage(hwndStatus, WM_SIZE, 0, 0);
    GetClientRect(hwndStatus, &rc2);
    expect_rect(0, 0, 672, 62, rc2);

    /* font changed to smaller than minheight - has an effect */
    SendMessage(hwndStatus, SB_SETMINHEIGHT, 30, 0);
    expect_rect(0, 0, 672, 62, rc2);
    SendMessage(hwndStatus, WM_SIZE, 0, 0);
    GetClientRect(hwndStatus, &rc2);
    expect_rect(0, 0, 672, 42, rc2);
    hFontSm = CreateFont(9, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE, "Tahoma");
    SendMessage(hwndStatus, WM_SETFONT, (WPARAM)hFontSm, TRUE);
    GetClientRect(hwndStatus, &rc2);
    expect_rect(0, 0, 672, 32, rc2);

    /* test the height formula */
    ZeroMemory(&lf, sizeof(lf));
    SendMessage(hwndStatus, SB_SETMINHEIGHT, 0, 0);
    hdc = GetDC(NULL);
    trace("dpi=%d\n", GetDeviceCaps(hdc, LOGPIXELSY));
    EnumFontFamiliesEx(hdc, &lf, (FONTENUMPROC)check_height_family_enumproc, (LPARAM)hwndStatus, 0);
    ReleaseDC(NULL, hdc);

    DestroyWindow(hwndStatus);
    DeleteObject(hFont);
    DeleteObject(hFontSm);
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
    r = SendMessage(hWndStatus, SB_SETPARTS, 3, (LPARAM)nParts);
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
    r = SendMessage(hWndStatus, SB_GETPARTS, 3, (LPARAM)checkParts);
    ok(r == 3, "Expected 3, got %d\n", r);
    expect(50,checkParts[0]);
    expect(150,checkParts[1]);
    expect(-1,checkParts[2]);
    r = SendMessage(hWndStatus, SB_GETBORDERS, 0, (LPARAM)borders);
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
    ok(r == CLR_DEFAULT ||
       broken(r == 0), /* win95 */
       "Expected %d, got %d\n", CLR_DEFAULT, r);
    r = SendMessage(hWndStatus, SB_SETBKCOLOR , 0, CLR_DEFAULT);
    ok(r == RGB(255,0,0) ||
       broken(r == 0), /* win95 */
       "Expected %d, got %d\n", RGB(255,0,0), r);

    /* Add an icon to the status bar */
    hIcon = LoadIcon(NULL, IDI_QUESTION);
    r = SendMessage(hWndStatus, SB_SETICON, 1, 0);
    ok(r != 0 ||
       broken(r == 0), /* win95 */
       "Expected non-zero, got %d\n", r);
    r = SendMessage(hWndStatus, SB_SETICON, 1, (LPARAM) hIcon);
    ok(r != 0 ||
       broken(r == 0), /* win95 */
       "Expected non-zero, got %d\n", r);
    r = SendMessage(hWndStatus, SB_SETICON, 1, 0);
    ok(r != 0 ||
       broken(r == 0), /* win95 */
       "Expected non-zero, got %d\n", r);

    /* Set the Unicode format */
    r = SendMessage(hWndStatus, SB_SETUNICODEFORMAT, FALSE, 0);
    r = SendMessage(hWndStatus, SB_GETUNICODEFORMAT, 0, 0);
    expect(FALSE,r);
    r = SendMessage(hWndStatus, SB_SETUNICODEFORMAT, TRUE, 0);
    expect(FALSE,r);
    r = SendMessage(hWndStatus, SB_GETUNICODEFORMAT, 0, 0);
    ok(r == TRUE ||
       broken(r == FALSE), /* win95 */
       "Expected TRUE, got %d\n", r);

    /* Reset number of parts */
    r = SendMessage(hWndStatus, SB_SETPARTS, 2, (LPARAM)nParts);
    expect(TRUE,r);

    /* Set the minimum height and get rectangle information again */
    SendMessage(hWndStatus, SB_SETMINHEIGHT, 50, 0);
    r = SendMessage(hWndStatus, WM_SIZE, 0, 0);
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
        lstrcpyA(charArray, "apple");
        SendMessage(hWndStatus, SB_GETTIPTEXT, MAKEWPARAM (0, 20),(LPARAM) charArray);
        ok(strcmp(charArray,"Tooltip Text") == 0 ||
           broken(!strcmp(charArray, "apple")), /* win95 */
           "Expected Tooltip Text, got %s\n", charArray);
    }

    /* Make simple */
    SendMessage(hWndStatus, SB_SIMPLE, TRUE, 0);
    r = SendMessage(hWndStatus, SB_ISSIMPLE, 0, 0);
    ok(r == TRUE ||
       broken(r == FALSE), /* win95 */
       "Expected TRUE, got %d\n", r);

    DestroyWindow(hWndStatus);
}

START_TEST(status)
{
    hinst = GetModuleHandleA(NULL);

    g_hMainWnd = CreateWindowExA(0, "static", "", WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, CW_USEDEFAULT, 672+2*GetSystemMetrics(SM_CXSIZEFRAME),
      226+GetSystemMetrics(SM_CYCAPTION)+2*GetSystemMetrics(SM_CYSIZEFRAME),
      NULL, NULL, GetModuleHandleA(NULL), 0);

    InitCommonControls();

    register_subclass();

    test_status_control();
    test_create();
    test_height();
}
