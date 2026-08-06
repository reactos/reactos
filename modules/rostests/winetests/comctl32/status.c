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

#include <windows.h>
#include <commctrl.h>

#include "wine/test.h"

#define SUBCLASS_NAME "MyStatusBar"

#define expect(expected,got) expect_(__LINE__, expected, got)
static inline void expect_(unsigned line, DWORD expected, DWORD got)
{
    ok_(__FILE__, line)(expected == got, "Expected %ld, got %ld\n", expected, got);
}

#define expect_rect(_left,_top,_right,_bottom,got) do { \
        RECT exp = {abs(got.left - _left), abs(got.top - _top), \
                    abs(got.right - _right), abs(got.bottom - _bottom)}; \
        ok(exp.left <= 2 && exp.top <= 2 && exp.right <= 2 && exp.bottom <= 2, \
           "Expected rect (%d,%d)-(%d,%d), got %s\n", _left, _top, _right, _bottom, \
           wine_dbgstr_rect(&(got))); } while (0)

static HINSTANCE hinst;
static WNDPROC g_status_wndproc;
static RECT g_rcCreated;
static HWND g_hMainWnd;
static int g_wmsize_count = 0;
static INT g_ysize;
static INT g_dpisize;
static int g_wmdrawitm_ctr;
static WNDPROC g_wndproc_saved;

static BOOL (WINAPI *pInitCommonControlsEx)(const INITCOMMONCONTROLSEX*);

static HWND create_status_control(DWORD style, DWORD exstyle)
{
    HWND hWndStatus;

    /* make the control */
    hWndStatus = CreateWindowExA(exstyle, STATUSCLASSNAMEA, NULL, style,
        /* placement */
        0, 0, 300, 20,
        /* parent, etc */
        NULL, NULL, hinst, NULL);
    ok(hWndStatus != NULL, "failed to create status wnd\n");
    return hWndStatus;
}

static LRESULT WINAPI create_test_wndproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LRESULT ret;

    if (msg == WM_CREATE)
    {
        CREATESTRUCTA *cs = (CREATESTRUCTA *)lParam;
        ret = CallWindowProcA(g_status_wndproc, hwnd, msg, wParam, lParam);
        GetWindowRect(hwnd, &g_rcCreated);
        MapWindowPoints(HWND_DESKTOP, g_hMainWnd, (LPPOINT)&g_rcCreated, 2);
        ok(cs->x == g_rcCreated.left, "CREATESTRUCT.x modified\n");
        ok(cs->y == g_rcCreated.top, "CREATESTRUCT.y modified\n");
    } else if (msg == WM_SIZE)
    {
        g_wmsize_count++;
        ret = CallWindowProcA(g_status_wndproc, hwnd, msg, wParam, lParam);
    }
    else
        ret = CallWindowProcA(g_status_wndproc, hwnd, msg, wParam, lParam);

    return ret;
}

static void register_subclass(void)
{
    WNDCLASSEXA cls;

    cls.cbSize = sizeof(WNDCLASSEXA);
    GetClassInfoExA(NULL, STATUSCLASSNAMEA, &cls);
    g_status_wndproc = cls.lpfnWndProc;
    cls.lpfnWndProc = create_test_wndproc;
    cls.lpszClassName = SUBCLASS_NAME;
    cls.hInstance = NULL;
    ok(RegisterClassExA(&cls), "RegisterClassEx failed\n");
}

static void test_create(void)
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

static int CALLBACK check_height_font_enumproc(ENUMLOGFONTEXA *enumlf, NEWTEXTMETRICEXA *ntm, DWORD type, LPARAM lParam)
{
    HWND hwndStatus = (HWND)lParam;
    HDC hdc = GetDC(NULL);
    static const int sizes[] = { 6,  7,  8,  9, 10, 11, 12, 13, 15, 16,
                                20, 22, 28, 36, 48, 72};
    DWORD i;
    INT y;
    LPSTR facename = (CHAR *)enumlf->elfFullName;

    /* on win9x, enumlf->elfFullName is only valid for truetype fonts */
    if (type != TRUETYPE_FONTTYPE)
        facename = enumlf->elfLogFont.lfFaceName;

    for (i = 0; i < ARRAY_SIZE(sizes); i++)
    {
        HFONT hFont;
        TEXTMETRICA tm;
        HFONT hCtrlFont;
        HFONT hOldFont;
        RECT rcCtrl;

        enumlf->elfLogFont.lfHeight = sizes[i];
        hFont = CreateFontIndirectA(&enumlf->elfLogFont);
        hCtrlFont = (HFONT)SendMessageA(hwndStatus, WM_SETFONT, (WPARAM)hFont, TRUE);
        hOldFont = SelectObject(hdc, hFont);

        GetClientRect(hwndStatus, &rcCtrl);
        GetTextMetricsA(hdc, &tm);
        y = tm.tmHeight + (tm.tmInternalLeading ? tm.tmInternalLeading : 2) + 4;

        ok( (rcCtrl.bottom == max(y, g_ysize)) || (rcCtrl.bottom == max(y, g_dpisize)),
            "got %ld (expected %d or %d) for %s #%d\n",
            rcCtrl.bottom, max(y, g_ysize), max(y, g_dpisize), facename, sizes[i]);

        SelectObject(hdc, hOldFont);
        SendMessageA(hwndStatus, WM_SETFONT, (WPARAM)hCtrlFont, TRUE);
        DeleteObject(hFont);
    }
    ReleaseDC(NULL, hdc);
    return 1;
}

static int CALLBACK check_height_family_enumproc(ENUMLOGFONTEXA *enumlf, NEWTEXTMETRICEXA *ntm, DWORD type, LPARAM lParam)
{
    HDC hdc = GetDC(NULL);
    enumlf->elfLogFont.lfHeight = 0;
    EnumFontFamiliesExA(hdc, &enumlf->elfLogFont, (FONTENUMPROCA)check_height_font_enumproc, lParam, 0);
    ReleaseDC(NULL, hdc);
    return 1;
}

static void test_height(void)
{
    LOGFONTA lf;
    HFONT hFont, hFontSm;
    RECT rc1, rc2;
    HWND hwndStatus = CreateWindowA(SUBCLASS_NAME, NULL, WS_CHILD|WS_VISIBLE,
        0, 0, 300, 20, g_hMainWnd, NULL, NULL, NULL);
    HDC hdc;

    GetClientRect(hwndStatus, &rc1);
    hFont = CreateFontA(32, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE, "Tahoma");

    g_wmsize_count = 0;
    SendMessageA(hwndStatus, WM_SETFONT, (WPARAM)hFont, TRUE);
    if (!g_wmsize_count)
    {
        skip("Status control not resized in win95, skipping broken tests.\n");
        return;
    }
    ok(g_wmsize_count > 0, "WM_SETFONT should issue WM_SIZE\n");

    GetClientRect(hwndStatus, &rc2);
    expect_rect(0, 0, 672, 42, rc2); /* GetTextMetrics returns invalid tmInternalLeading for this font */

    g_wmsize_count = 0;
    SendMessageA(hwndStatus, WM_SETFONT, (WPARAM)hFont, TRUE);
    ok(g_wmsize_count > 0, "WM_SETFONT should issue WM_SIZE\n");

    GetClientRect(hwndStatus, &rc2);
    expect_rect(0, 0, 672, 42, rc2);

    /* minheight < fontsize - no effects*/
    SendMessageA(hwndStatus, SB_SETMINHEIGHT, 12, 0);
    SendMessageA(hwndStatus, WM_SIZE, 0, 0);
    GetClientRect(hwndStatus, &rc2);
    expect_rect(0, 0, 672, 42, rc2);

    /* minheight > fontsize - has an effect after WM_SIZE */
    SendMessageA(hwndStatus, SB_SETMINHEIGHT, 60, 0);
    GetClientRect(hwndStatus, &rc2);
    expect_rect(0, 0, 672, 42, rc2);
    SendMessageA(hwndStatus, WM_SIZE, 0, 0);
    GetClientRect(hwndStatus, &rc2);
    expect_rect(0, 0, 672, 62, rc2);

    /* font changed to smaller than minheight - has an effect */
    SendMessageA(hwndStatus, SB_SETMINHEIGHT, 30, 0);
    expect_rect(0, 0, 672, 62, rc2);
    SendMessageA(hwndStatus, WM_SIZE, 0, 0);
    GetClientRect(hwndStatus, &rc2);
    expect_rect(0, 0, 672, 42, rc2);
    hFontSm = CreateFontA(9, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE, "Tahoma");
    SendMessageA(hwndStatus, WM_SETFONT, (WPARAM)hFontSm, TRUE);
    GetClientRect(hwndStatus, &rc2);
    expect_rect(0, 0, 672, 32, rc2);

    /* test the height formula */
    ZeroMemory(&lf, sizeof(lf));
    SendMessageA(hwndStatus, SB_SETMINHEIGHT, 0, 0);
    hdc = GetDC(NULL);

    /* used only for some fonts (tahoma as example) */
    g_ysize = GetSystemMetrics(SM_CYSIZE) + 2;
    if (g_ysize & 1) g_ysize--;     /* The min height is always even */

    g_dpisize = MulDiv(18, GetDeviceCaps(hdc, LOGPIXELSY), 96) + 2;
    if (g_dpisize & 1) g_dpisize--; /* The min height is always even */


    trace("dpi=%d (min height: %d or %d) SM_CYSIZE: %d\n",
            GetDeviceCaps(hdc, LOGPIXELSY), g_ysize, g_dpisize,
            GetSystemMetrics(SM_CYSIZE));

    EnumFontFamiliesExA(hdc, &lf, (FONTENUMPROCA)check_height_family_enumproc, (LPARAM)hwndStatus, 0);
    ReleaseDC(NULL, hdc);

    DestroyWindow(hwndStatus);
    DeleteObject(hFont);
    DeleteObject(hFontSm);
}

static void test_status_control(void)
{
    HWND hWndStatus;
    int r;
    int nParts[] = {50, 150, -1, -1};
    int checkParts[] = {0, 0, 0, 0, 0};
    int borders[] = {0, 0, 0};
    RECT rc;
    CHAR charArray[20];
    HICON hIcon;
    char ch;
    char chstr[10] = "Inval id";
    COLORREF crColor = RGB(0,0,0);
    WCHAR wbuf[20];

    hWndStatus = create_status_control(WS_VISIBLE | SBT_TOOLTIPS, 0);

    /* Divide into parts and set text */
    r = SendMessageA(hWndStatus, SB_SETPARTS, 3, (LPARAM)nParts);
    expect(TRUE,r);
    r = SendMessageA(hWndStatus, SB_SETTEXTA, SBT_POPOUT|0,    (LPARAM)"First");
    expect(TRUE,r);
    r = SendMessageA(hWndStatus, SB_SETTEXTA, SBT_OWNERDRAW|1, (LPARAM)"Second");
    expect(TRUE,r);
    r = SendMessageA(hWndStatus, SB_SETTEXTA, SBT_NOBORDERS|2, (LPARAM)"Third");
    expect(TRUE,r);

    /* Get RECT Information */
    r = SendMessageA(hWndStatus, SB_GETRECT, 0, (LPARAM)&rc);
    expect(TRUE,r);
    expect(2,rc.top);
    /* The rc.bottom test is system dependent
    expect(22,rc.bottom); */
    expect(0,rc.left);
    expect(50,rc.right);
    r = SendMessageA(hWndStatus, SB_GETRECT, -1, (LPARAM)&rc);
    expect(FALSE,r);
    r = SendMessageA(hWndStatus, SB_GETRECT, 3, (LPARAM)&rc);
    expect(FALSE,r);
    /* Get text length and text */
    r = SendMessageA(hWndStatus, SB_GETTEXTLENGTHA, 0, 0);
    expect(5,LOWORD(r));
    expect(SBT_POPOUT,HIWORD(r));
    r = SendMessageW(hWndStatus, WM_GETTEXTLENGTH, 0, 0);
    ok(r == 5, "Expected 5, got %d\n", r);
    r = SendMessageA(hWndStatus, SB_GETTEXTLENGTHA, 1, 0);
    expect(0,LOWORD(r));
    expect(SBT_OWNERDRAW,HIWORD(r));
    r = SendMessageA(hWndStatus, SB_GETTEXTLENGTHA, 2, 0);
    expect(5,LOWORD(r));
    expect(SBT_NOBORDERS,HIWORD(r));
    r = SendMessageA(hWndStatus, SB_GETTEXTA, 2, (LPARAM) charArray);
    ok(strcmp(charArray,"Third") == 0, "Expected Third, got %s\n", charArray);
    expect(5,LOWORD(r));
    expect(SBT_NOBORDERS,HIWORD(r));

    /* Get parts */
    r = SendMessageA(hWndStatus, SB_GETPARTS, 3, (LPARAM)checkParts);
    ok(r == 3, "Expected 3, got %d\n", r);
    expect(50,checkParts[0]);
    expect(150,checkParts[1]);
    expect(-1,checkParts[2]);
    r = SendMessageA(hWndStatus, SB_GETPARTS, 5, (LPARAM)checkParts);
    ok(r == 3, "Expected 3, got %d\n", r);
    expect(50,checkParts[0]);
    expect(150,checkParts[1]);
    expect(-1,checkParts[2]);
    expect(0,checkParts[3]);
    expect(0,checkParts[4]);
    /* Get borders */
    r = SendMessageA(hWndStatus, SB_GETBORDERS, 0, (LPARAM)borders);
    ok(r == TRUE, "Expected TRUE, got %d\n", r);
    expect(0,borders[0]);
    expect(2,borders[1]);
    expect(2,borders[2]);

    /* Test resetting text with different characters */
    r = SendMessageA(hWndStatus, SB_SETPARTS, 4, (LPARAM)nParts);
    expect(TRUE,r);
    r = SendMessageA(hWndStatus, SB_SETTEXTA, 0, (LPARAM)"First@Again");
    expect(TRUE,r);
    r = SendMessageA(hWndStatus, SB_SETTEXTA, 1, (LPARAM)"Invalid\tChars\\7\7");
    expect(TRUE,r);
    r = SendMessageA(hWndStatus, SB_SETTEXTA, 2, (LPARAM)"InvalidChars\\n\n");
    expect(TRUE,r);
    r = SendMessageW(hWndStatus, SB_SETTEXTW, 3, (LPARAM)L"Non printable\x80");
    expect(TRUE,r);

    /* Get text again */
    r = SendMessageA(hWndStatus, SB_GETTEXTA, 0, (LPARAM) charArray);
    ok(strcmp(charArray,"First@Again") == 0, "Expected First@Again, got %s\n", charArray);
    ok(r == 11, "r = %d\n", r);

    r = SendMessageA(hWndStatus, SB_GETTEXTA, 1, (LPARAM) charArray);
    ok(strcmp(charArray,"Invalid\tChars\\7 ") == 0, "Expected Invalid\tChars\\7 , got %s\n", charArray);
    ok(r == 16, "r = %d\n", r);

    r = SendMessageA(hWndStatus, SB_GETTEXTA, 2, (LPARAM) charArray);
    ok(strcmp(charArray,"InvalidChars\\n ") == 0, "Expected InvalidChars\\n , got %s\n", charArray);
    ok(r == 15, "r = %d\n", r);

    r = SendMessageW(hWndStatus, SB_GETTEXTW, 3, (LPARAM) wbuf);
    ok(wcscmp(wbuf, L"Non printable\x80") == 0, "got %s\n", wine_dbgstr_w(wbuf));
    ok(r == 14, "r = %d\n", r);

    /* test more nonprintable chars */
    for(ch = 0x00; ch < 0x7F; ch++) {
        chstr[5] = ch;
        r = SendMessageA(hWndStatus, SB_SETTEXTA, 0, (LPARAM)chstr);
        expect(TRUE,r);
        r = SendMessageA(hWndStatus, SB_GETTEXTA, 0, (LPARAM)charArray);
        ok(r == strlen(charArray), "got %d\n", r);
        /* substitution with single space */
        if (ch > 0x00 && ch < 0x20 && ch != '\t')
            chstr[5] = ' ';
        ok(strcmp(charArray, chstr) == 0, "Expected %s, got %s\n", chstr, charArray);
    }

    /* Set background color */
    crColor = SendMessageA(hWndStatus, SB_SETBKCOLOR , 0, RGB(255,0,0));
    ok(crColor == CLR_DEFAULT ||
       broken(crColor == RGB(0,0,0)), /* win95 */
       "Expected 0x%.8lx, got 0x%.8lx\n", CLR_DEFAULT, crColor);
    crColor = SendMessageA(hWndStatus, SB_SETBKCOLOR , 0, CLR_DEFAULT);
    ok(crColor == RGB(255,0,0) ||
       broken(crColor == RGB(0,0,0)), /* win95 */
       "Expected 0x%.8lx, got 0x%.8lx\n", RGB(255,0,0), crColor);

    /* Add an icon to the status bar */
    hIcon = LoadIconA(NULL, (LPCSTR)IDI_QUESTION);
    r = SendMessageA(hWndStatus, SB_SETICON, 1, 0);
    ok(r != 0 ||
       broken(r == 0), /* win95 */
       "Expected non-zero, got %d\n", r);
    r = SendMessageA(hWndStatus, SB_SETICON, 1, (LPARAM) hIcon);
    ok(r != 0 ||
       broken(r == 0), /* win95 */
       "Expected non-zero, got %d\n", r);
    r = SendMessageA(hWndStatus, SB_SETICON, 1, 0);
    ok(r != 0 ||
       broken(r == 0), /* win95 */
       "Expected non-zero, got %d\n", r);

    /* Set the Unicode format */
    r = SendMessageA(hWndStatus, SB_SETUNICODEFORMAT, FALSE, 0);
    expect(FALSE,r);
    r = SendMessageA(hWndStatus, SB_GETUNICODEFORMAT, 0, 0);
    expect(FALSE,r);
    r = SendMessageA(hWndStatus, SB_SETUNICODEFORMAT, TRUE, 0);
    expect(FALSE,r);
    r = SendMessageA(hWndStatus, SB_GETUNICODEFORMAT, 0, 0);
    ok(r == TRUE ||
       broken(r == FALSE), /* win95 */
       "Expected TRUE, got %d\n", r);

    /* Reset number of parts */
    r = SendMessageA(hWndStatus, SB_SETPARTS, 2, (LPARAM)nParts);
    expect(TRUE,r);
    r = SendMessageA(hWndStatus, SB_GETPARTS, 0, 0);
    ok(r == 2, "Expected 2, got %d\n", r);
    r = SendMessageA(hWndStatus, SB_SETPARTS, 0, 0);
    expect(FALSE,r);
    r = SendMessageA(hWndStatus, SB_GETPARTS, 0, 0);
    ok(r == 2, "Expected 2, got %d\n", r);

    /* Set the minimum height and get rectangle information again */
    SendMessageA(hWndStatus, SB_SETMINHEIGHT, 50, 0);
    r = SendMessageA(hWndStatus, WM_SIZE, 0, 0);
    expect(0,r);
    r = SendMessageA(hWndStatus, SB_GETRECT, 0, (LPARAM)&rc);
    expect(TRUE,r);
    expect(2,rc.top);
    /* The rc.bottom test is system dependent
    expect(22,rc.bottom); */
    expect(0,rc.left);
    expect(50,rc.right);
    r = SendMessageA(hWndStatus, SB_GETRECT, -1, (LPARAM)&rc);
    expect(FALSE,r);
    r = SendMessageA(hWndStatus, SB_GETRECT, 3, (LPARAM)&rc);
    expect(FALSE,r);

    /* Set the ToolTip text */
    SendMessageA(hWndStatus, SB_SETTIPTEXTA, 0,(LPARAM) "Tooltip Text");
    lstrcpyA(charArray, "apple");
    SendMessageA(hWndStatus, SB_GETTIPTEXTA, MAKEWPARAM (0, 20),(LPARAM) charArray);
    ok(strcmp(charArray,"Tooltip Text") == 0 ||
        broken(!strcmp(charArray, "apple")), /* win95 */
        "Expected Tooltip Text, got %s\n", charArray);

    /* Make simple */
    SendMessageA(hWndStatus, SB_SIMPLE, TRUE, 0);
    r = SendMessageA(hWndStatus, SB_ISSIMPLE, 0, 0);
    ok(r == TRUE ||
       broken(r == FALSE), /* win95 */
       "Expected TRUE, got %d\n", r);

    DestroyWindow(hWndStatus);
}

static LRESULT WINAPI ownerdraw_test_wndproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LRESULT ret;
    if (msg == WM_DRAWITEM)
        g_wmdrawitm_ctr++;
    ret = CallWindowProcA(g_wndproc_saved, hwnd, msg, wParam, lParam);
    return ret;
}

static void test_status_ownerdraw(void)
{
    HWND hWndStatus;
    int r;
    const char* statustext = "STATUS TEXT";
    LONG oldstyle;

    /* subclass the main window and make sure it is visible */
    g_wndproc_saved = (WNDPROC) SetWindowLongPtrA( g_hMainWnd, GWLP_WNDPROC,
                                                  (LONG_PTR)ownerdraw_test_wndproc );
    ok( g_wndproc_saved != 0, "failed to set the WndProc\n");
    SetWindowPos( g_hMainWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE);
    oldstyle = GetWindowLongA( g_hMainWnd, GWL_STYLE);
    SetWindowLongA( g_hMainWnd, GWL_STYLE, oldstyle | WS_VISIBLE);
    /* create a status child window */
    ok((hWndStatus = CreateWindowA(SUBCLASS_NAME, "", WS_CHILD|WS_VISIBLE, 0, 0, 100, 100,
                    g_hMainWnd, NULL, NULL, 0)) != NULL, "CreateWindowA failed\n");
    /* set text */
    g_wmdrawitm_ctr = 0;
    r = SendMessageA(hWndStatus, SB_SETTEXTA, 0, (LPARAM)statustext);
    ok( r == TRUE, "Sendmessage returned %d, expected 1\n", r);
    ok( 0 == g_wmdrawitm_ctr, "got %d drawitem messages expected none\n", g_wmdrawitm_ctr);
    /* set same text, with ownerdraw flag */
    g_wmdrawitm_ctr = 0;
    r = SendMessageA(hWndStatus, SB_SETTEXTA, SBT_OWNERDRAW, (LPARAM)statustext);
    ok( r == TRUE, "Sendmessage returned %d, expected 1\n", r);
    ok( 1 == g_wmdrawitm_ctr, "got %d drawitem messages expected 1\n", g_wmdrawitm_ctr);
    /* and again */
    g_wmdrawitm_ctr = 0;
    r = SendMessageA(hWndStatus, SB_SETTEXTA, SBT_OWNERDRAW, (LPARAM)statustext);
    ok( r == TRUE, "Sendmessage returned %d, expected 1\n", r);
    ok( 1 == g_wmdrawitm_ctr, "got %d drawitem messages expected 1\n", g_wmdrawitm_ctr);
    /* clean up */
    DestroyWindow(hWndStatus);
    SetWindowLongA( g_hMainWnd, GWL_STYLE, oldstyle);
    SetWindowLongPtrA( g_hMainWnd, GWLP_WNDPROC, (LONG_PTR)g_wndproc_saved );
}

static void test_gettext(void)
{
    HWND hwndStatus = CreateWindowA(SUBCLASS_NAME, NULL, WS_CHILD|WS_VISIBLE,
        0, 0, 300, 20, g_hMainWnd, NULL, NULL, NULL);
    char buf[5];
    int r;

    r = SendMessageA(hwndStatus, SB_SETTEXTA, 0, (LPARAM)"Text");
    expect(TRUE, r);
    r = SendMessageA(hwndStatus, WM_GETTEXTLENGTH, 0, 0);
    expect(4, r);
    /* A size of 0 returns the length of the text */
    r = SendMessageA(hwndStatus, WM_GETTEXT, 0, 0);
    ok( r == 4 || broken(r == 2) /* win8 */, "Expected 4 got %d\n", r );
    /* A size of 1 only stores the NULL terminator */
    buf[0] = 0xa;
    r = SendMessageA(hwndStatus, WM_GETTEXT, 1, (LPARAM)buf);
    ok( r == 0 || broken(r == 4), "Expected 0 got %d\n", r );
    if (!r) ok(!buf[0], "expected empty buffer\n");
    /* A size of 2 returns a length 1 */
    r = SendMessageA(hwndStatus, WM_GETTEXT, 2, (LPARAM)buf);
    ok( r == 1 || broken(r == 4), "Expected 1 got %d\n", r );
    r = SendMessageA(hwndStatus, WM_GETTEXT, sizeof(buf), (LPARAM)buf);
    expect(4, r);
    ok(!strcmp(buf, "Text"), "expected Text, got %s\n", buf);
    DestroyWindow(hwndStatus);
}

/* Notify events to parent */
static BOOL g_got_dblclk;
static BOOL g_got_click;
static BOOL g_got_rdblclk;
static BOOL g_got_rclick;

/* Messages to parent */
static BOOL g_got_contextmenu;

static LRESULT WINAPI test_notify_parent_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   switch(msg)
   {
       case WM_NOTIFY:
       {
           NMHDR *hdr = ((LPNMHDR)lParam);
           switch(hdr->code)
           {
               case NM_DBLCLK: g_got_dblclk = TRUE; break;
               case NM_CLICK: g_got_click = TRUE; break;
               case NM_RDBLCLK: g_got_rdblclk = TRUE; break;
               case NM_RCLICK: g_got_rclick = TRUE; break;
           }

           /* Return zero to indicate default processing */
           return 0;
       }

       case WM_CONTEXTMENU: g_got_contextmenu = TRUE; return 0;

       default:
            return( DefWindowProcA(hwnd, msg, wParam, lParam));
   }

   return 0;
}

/* Test that WM_NOTIFY messages from the status control works correctly */
static void test_notify(void)
{
    HWND hwndParent;
    HWND hwndStatus;
    ATOM atom;
    WNDCLASSA wclass = {0};
    wclass.lpszClassName = "TestNotifyParentClass";
    wclass.lpfnWndProc   = test_notify_parent_proc;
    atom = RegisterClassA(&wclass);
    ok(atom, "RegisterClass failed\n");

    /* create parent */
    hwndParent = CreateWindowA(wclass.lpszClassName, "parent", WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, 300, 20, NULL, NULL, NULL, NULL);
    ok(hwndParent != NULL, "Parent creation failed!\n");

    /* create status bar */
    hwndStatus = CreateWindowA(STATUSCLASSNAMEA, NULL, WS_VISIBLE | WS_CHILD,
      0, 0, 300, 20, hwndParent, NULL, NULL, NULL);
    ok(hwndStatus != NULL, "Status creation failed!\n");

    /* Send various mouse event, and check that we get them */
    g_got_dblclk = FALSE;
    SendMessageA(hwndStatus, WM_LBUTTONDBLCLK, 0, 0);
    ok(g_got_dblclk, "WM_LBUTTONDBLCLK was not processed correctly!\n");
    g_got_rdblclk = FALSE;
    SendMessageA(hwndStatus, WM_RBUTTONDBLCLK, 0, 0);
    ok(g_got_rdblclk, "WM_RBUTTONDBLCLK was not processed correctly!\n");
    g_got_click = FALSE;
    SendMessageA(hwndStatus, WM_LBUTTONUP, 0, 0);
    ok(g_got_click, "WM_LBUTTONUP was not processed correctly!\n");

    /* For R-UP, check that we also get the context menu from the default processing */
    g_got_contextmenu = FALSE;
    g_got_rclick = FALSE;
    SendMessageA(hwndStatus, WM_RBUTTONUP, 0, 0);
    ok(g_got_rclick, "WM_RBUTTONUP was not processed correctly!\n");
    ok(g_got_contextmenu, "WM_RBUTTONUP did not activate the context menu!\n");
}

static void test_sizegrip(void)
{
    HWND hwndStatus;
    LONG style;
    RECT rc, rcClient;
    POINT pt;
    int width, r;

    hwndStatus = CreateWindowA(SUBCLASS_NAME, "", WS_CHILD|WS_VISIBLE|SBARS_SIZEGRIP,
      0, 0, 100, 100, g_hMainWnd, NULL, NULL, NULL);

    style = GetWindowLongPtrA(g_hMainWnd, GWL_STYLE);
    width = GetSystemMetrics(SM_CXVSCROLL);

    GetClientRect(hwndStatus, &rcClient);

    pt.x = rcClient.right;
    pt.y = rcClient.top;
    ClientToScreen(hwndStatus, &pt);
    rc.left = pt.x - width;
    rc.right = pt.x;
    rc.top = pt.y;

    pt.y = rcClient.bottom;
    ClientToScreen(hwndStatus, &pt);
    rc.bottom = pt.y;

    /* check bounds when not maximized */
    r = SendMessageA(hwndStatus, WM_NCHITTEST, 0, MAKELPARAM(rc.left, rc.top));
    expect(HTBOTTOMRIGHT, r);
    r = SendMessageA(hwndStatus, WM_NCHITTEST, 0, MAKELPARAM(rc.left - 1, rc.top));
    expect(HTCLIENT, r);
    r = SendMessageA(hwndStatus, WM_NCHITTEST, 0, MAKELPARAM(rc.left, rc.top - 1));
    expect(HTBOTTOMRIGHT, r);
    r = SendMessageA(hwndStatus, WM_NCHITTEST, 0, MAKELPARAM(rc.right, rc.bottom));
    expect(HTBOTTOMRIGHT, r);
    r = SendMessageA(hwndStatus, WM_NCHITTEST, 0, MAKELPARAM(rc.right + 1, rc.bottom));
    expect(HTBOTTOMRIGHT, r);
    r = SendMessageA(hwndStatus, WM_NCHITTEST, 0, MAKELPARAM(rc.right, rc.bottom + 1));
    expect(HTBOTTOMRIGHT, r);
    r = SendMessageA(hwndStatus, WM_NCHITTEST, 0, MAKELPARAM(rc.right - 1, rc.bottom - 1));
    expect(HTBOTTOMRIGHT, r);

    /* not maximized and right-to-left */
    SetWindowLongA(hwndStatus, GWL_EXSTYLE, WS_EX_LAYOUTRTL);

    pt.x = rcClient.right;
    ClientToScreen(hwndStatus, &pt);
    rc.left = pt.x + width;
    rc.right = pt.x;

    r = SendMessageA(hwndStatus, WM_NCHITTEST, 0, MAKELPARAM(rc.left, rc.top));
    expect(HTBOTTOMLEFT, r);
    r = SendMessageA(hwndStatus, WM_NCHITTEST, 0, MAKELPARAM(rc.left + 1, rc.top));
    expect(HTCLIENT, r);
    r = SendMessageA(hwndStatus, WM_NCHITTEST, 0, MAKELPARAM(rc.left, rc.top - 1));
    expect(HTBOTTOMLEFT, r);
    r = SendMessageA(hwndStatus, WM_NCHITTEST, 0, MAKELPARAM(rc.right, rc.bottom));
    expect(HTBOTTOMLEFT, r);
    r = SendMessageA(hwndStatus, WM_NCHITTEST, 0, MAKELPARAM(rc.right - 1, rc.bottom));
    expect(HTBOTTOMLEFT, r);
    r = SendMessageA(hwndStatus, WM_NCHITTEST, 0, MAKELPARAM(rc.right, rc.bottom + 1));
    expect(HTBOTTOMLEFT, r);
    r = SendMessageA(hwndStatus, WM_NCHITTEST, 0, MAKELPARAM(rc.right + 1, rc.bottom - 1));
    expect(HTBOTTOMLEFT, r);

    /* maximize with left-to-right */
    SetWindowLongA(g_hMainWnd, GWL_STYLE, style|WS_MAXIMIZE);
    SetWindowLongA(hwndStatus, GWL_EXSTYLE, 0);

    GetClientRect(hwndStatus, &rcClient);

    pt.x = rcClient.right;
    pt.y = rcClient.top;
    ClientToScreen(hwndStatus, &pt);
    rc.left = pt.x - width;
    rc.right = pt.x;
    rc.top = pt.y;

    pt.y = rcClient.bottom;
    ClientToScreen(hwndStatus, &pt);
    rc.bottom = pt.y;

    r = SendMessageA(hwndStatus, WM_NCHITTEST, 0, MAKELPARAM(rc.left, rc.top));
    expect(HTCLIENT, r);
    r = SendMessageA(hwndStatus, WM_NCHITTEST, 0, MAKELPARAM(rc.left - 1, rc.top));
    expect(HTCLIENT, r);
    r = SendMessageA(hwndStatus, WM_NCHITTEST, 0, MAKELPARAM(rc.left, rc.top - 1));
    expect(HTNOWHERE, r);
    r = SendMessageA(hwndStatus, WM_NCHITTEST, 0, MAKELPARAM(rc.right, rc.bottom));
    expect(HTNOWHERE, r);
    r = SendMessageA(hwndStatus, WM_NCHITTEST, 0, MAKELPARAM(rc.right + 1, rc.bottom));
    expect(HTNOWHERE, r);
    r = SendMessageA(hwndStatus, WM_NCHITTEST, 0, MAKELPARAM(rc.right, rc.bottom + 1));
    expect(HTNOWHERE, r);
    r = SendMessageA(hwndStatus, WM_NCHITTEST, 0, MAKELPARAM(rc.right - 1, rc.bottom - 1));
    expect(HTCLIENT, r);

    /* maximized with right-to-left */
    SetWindowLongA(hwndStatus, GWL_EXSTYLE, WS_EX_LAYOUTRTL);

    pt.x = rcClient.right;
    ClientToScreen(hwndStatus, &pt);
    rc.left = pt.x + width;
    rc.right = pt.x;

    r = SendMessageA(hwndStatus, WM_NCHITTEST, 0, MAKELPARAM(rc.left, rc.top));
    expect(HTCLIENT, r);
    r = SendMessageA(hwndStatus, WM_NCHITTEST, 0, MAKELPARAM(rc.left + 1, rc.top));
    expect(HTCLIENT, r);
    r = SendMessageA(hwndStatus, WM_NCHITTEST, 0, MAKELPARAM(rc.left, rc.top - 1));
    expect(HTNOWHERE, r);
    r = SendMessageA(hwndStatus, WM_NCHITTEST, 0, MAKELPARAM(rc.right, rc.bottom));
    expect(HTNOWHERE, r);
    r = SendMessageA(hwndStatus, WM_NCHITTEST, 0, MAKELPARAM(rc.right - 1, rc.bottom));
    expect(HTNOWHERE, r);
    r = SendMessageA(hwndStatus, WM_NCHITTEST, 0, MAKELPARAM(rc.right, rc.bottom + 1));
    expect(HTNOWHERE, r);
    r = SendMessageA(hwndStatus, WM_NCHITTEST, 0, MAKELPARAM(rc.right + 1, rc.bottom - 1));
    expect(HTCLIENT, r);

    SetWindowLongA(g_hMainWnd, GWL_STYLE, style);
    DestroyWindow(hwndStatus);
}

static void init_functions(void)
{
    HMODULE hComCtl32 = LoadLibraryA("comctl32.dll");

#define X(f) p##f = (void*)GetProcAddress(hComCtl32, #f);
    X(InitCommonControlsEx);
#undef X
}

START_TEST(status)
{
    INITCOMMONCONTROLSEX iccex;

    init_functions();

    hinst = GetModuleHandleA(NULL);

    iccex.dwSize = sizeof(iccex);
    iccex.dwICC  = ICC_BAR_CLASSES;
    pInitCommonControlsEx(&iccex);

    g_hMainWnd = CreateWindowExA(0, WC_STATICA, "", WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, CW_USEDEFAULT, 672+2*GetSystemMetrics(SM_CXSIZEFRAME),
      226+GetSystemMetrics(SM_CYCAPTION)+2*GetSystemMetrics(SM_CYSIZEFRAME),
      NULL, NULL, GetModuleHandleA(NULL), 0);

    register_subclass();

    test_status_control();
    test_create();
    test_height();
    test_status_ownerdraw();
    test_gettext();
    test_notify();
    test_sizegrip();
}
