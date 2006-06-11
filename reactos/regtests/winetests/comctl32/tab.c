/* Unit test suite for tab control.
 *
 * Copyright 2003 Vitaliy Margolen
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

#define DEFAULT_MIN_TAB_WIDTH 54
#define TAB_DEFAULT_WIDTH 96
#define TAB_PADDING_X 6
#define EXTRA_ICON_PADDING 3

#define TabWidthPadded(padd_x, num) (DEFAULT_MIN_TAB_WIDTH - (TAB_PADDING_X - (padd_x)) * num)

#define TabCheckSetSize(hwnd, SetWidth, SetHeight, ExpWidth, ExpHeight, Msg)\
    SendMessage (hwnd, TCM_SETITEMSIZE, 0,\
	(LPARAM) MAKELPARAM((SetWidth >= 0) ? SetWidth:0, (SetHeight >= 0) ? SetHeight:0));\
    if (winetest_interactive) RedrawWindow (hwnd, NULL, 0, RDW_UPDATENOW);\
    CheckSize(hwnd, ExpWidth, ExpHeight, Msg);

#define CheckSize(hwnd,width,height,msg)\
    SendMessage (hwnd, TCM_GETITEMRECT, 0, (LPARAM) &rTab);\
    if ((width  >= 0) && (height < 0))\
	ok (width  == rTab.right  - rTab.left, "%s: Expected width [%d] got [%ld]\n",\
        msg, (int)width,  rTab.right  - rTab.left);\
    else if ((height >= 0) && (width  < 0))\
	ok (height == rTab.bottom - rTab.top,  "%s: Expected height [%d] got [%ld]\n",\
        msg, (int)height, rTab.bottom - rTab.top);\
    else\
	ok ((width  == rTab.right  - rTab.left) &&\
	    (height == rTab.bottom - rTab.top ),\
	    "%s: Expected [%d,%d] got [%ld,%ld]\n", msg, (int)width, (int)height,\
            rTab.right - rTab.left, rTab.bottom - rTab.top);

static HFONT hFont = 0;

static HWND
create_tabcontrol (DWORD style, DWORD mask)
{
    HWND handle;
    TCITEM tcNewTab;
    static char text1[] = "Tab 1",
    text2[] = "Wide Tab 2",
    text3[] = "T 3";

    handle = CreateWindow (
	WC_TABCONTROLA,
	"TestTab",
	WS_CLIPSIBLINGS | WS_CLIPCHILDREN | TCS_FOCUSNEVER | style,
        10, 10, 300, 100,
        NULL, NULL, NULL, 0);

    assert (handle);

    SetWindowLong(handle, GWL_STYLE, WS_CLIPSIBLINGS | WS_CLIPCHILDREN | TCS_FOCUSNEVER | style);
    SendMessage (handle, WM_SETFONT, 0, (LPARAM) hFont);

    tcNewTab.mask = mask;
    tcNewTab.pszText = text1;
    tcNewTab.iImage = 0;
    SendMessage (handle, TCM_INSERTITEM, 0, (LPARAM) &tcNewTab);
    tcNewTab.pszText = text2;
    tcNewTab.iImage = 1;
    SendMessage (handle, TCM_INSERTITEM, 1, (LPARAM) &tcNewTab);
    tcNewTab.pszText = text3;
    tcNewTab.iImage = 2;
    SendMessage (handle, TCM_INSERTITEM, 2, (LPARAM) &tcNewTab);

    if (winetest_interactive)
    {
        ShowWindow (handle, SW_SHOW);
        RedrawWindow (handle, NULL, 0, RDW_UPDATENOW);
        Sleep (1000);
    }

    return handle;
}

static void test_tab(INT nMinTabWidth)
{
    HWND hwTab;
    RECT rTab;
    HIMAGELIST himl = ImageList_Create(21, 21, ILC_COLOR, 3, 4);
    SIZE size;
    HDC hdc;
    HFONT hOldFont;
    INT i;

    hwTab = create_tabcontrol(TCS_FIXEDWIDTH, TCIF_TEXT|TCIF_IMAGE);
    SendMessage(hwTab, TCM_SETMINTABWIDTH, 0, nMinTabWidth);

    hdc = GetDC(hwTab);
    hOldFont = SelectObject(hdc, (HFONT)SendMessage(hwTab, WM_GETFONT, 0, 0));
    GetTextExtentPoint32A(hdc, "Tab 1", strlen("Tab 1"), &size);
    trace("Tab1 text size: size.cx=%ld size.cy=%ld\n", size.cx, size.cy);
    SelectObject(hdc, hOldFont);
    ReleaseDC(hwTab, hdc);

    trace ("  TCS_FIXEDWIDTH tabs no icon...\n");
    CheckSize(hwTab, TAB_DEFAULT_WIDTH, -1, "default width");
    TabCheckSetSize(hwTab, 50, 20, 50, 20, "set size");
    TabCheckSetSize(hwTab, 0, 1, 0, 1, "min size");

    SendMessage(hwTab, TCM_SETIMAGELIST, 0, (LPARAM)himl);

    trace ("  TCS_FIXEDWIDTH tabs with icon...\n");
    TabCheckSetSize(hwTab, 50, 30, 50, 30, "set size > icon");
    TabCheckSetSize(hwTab, 20, 20, 25, 20, "set size < icon");
    TabCheckSetSize(hwTab, 0, 1, 25, 1, "min size");

    DestroyWindow (hwTab);

    hwTab = create_tabcontrol(TCS_FIXEDWIDTH | TCS_BUTTONS, TCIF_TEXT|TCIF_IMAGE);
    SendMessage(hwTab, TCM_SETMINTABWIDTH, 0, nMinTabWidth);

    trace ("  TCS_FIXEDWIDTH buttons no icon...\n");
    CheckSize(hwTab, TAB_DEFAULT_WIDTH, -1, "default width");
    TabCheckSetSize(hwTab, 20, 20, 20, 20, "set size 1");
    TabCheckSetSize(hwTab, 10, 50, 10, 50, "set size 2");
    TabCheckSetSize(hwTab, 0, 1, 0, 1, "min size");

    SendMessage(hwTab, TCM_SETIMAGELIST, 0, (LPARAM)himl);

    trace ("  TCS_FIXEDWIDTH buttons with icon...\n");
    TabCheckSetSize(hwTab, 50, 30, 50, 30, "set size > icon");
    TabCheckSetSize(hwTab, 20, 20, 25, 20, "set size < icon");
    TabCheckSetSize(hwTab, 0, 1, 25, 1, "min size");
    SendMessage(hwTab, TCM_SETPADDING, 0, MAKELPARAM(4,4));
    TabCheckSetSize(hwTab, 0, 1, 25, 1, "set padding, min size");

    DestroyWindow (hwTab);

    hwTab = create_tabcontrol(TCS_FIXEDWIDTH | TCS_BOTTOM, TCIF_TEXT|TCIF_IMAGE);
    SendMessage(hwTab, TCM_SETMINTABWIDTH, 0, nMinTabWidth);

    trace ("  TCS_FIXEDWIDTH | TCS_BOTTOM tabs...\n");
    CheckSize(hwTab, TAB_DEFAULT_WIDTH, -1, "no icon, default width");

    TabCheckSetSize(hwTab, 20, 20, 20, 20, "no icon, set size 1");
    TabCheckSetSize(hwTab, 10, 50, 10, 50, "no icon, set size 2");
    TabCheckSetSize(hwTab, 0, 1, 0, 1, "no icon, min size");

    SendMessage(hwTab, TCM_SETIMAGELIST, 0, (LPARAM)himl);

    TabCheckSetSize(hwTab, 50, 30, 50, 30, "with icon, set size > icon");
    TabCheckSetSize(hwTab, 20, 20, 25, 20, "with icon, set size < icon");
    TabCheckSetSize(hwTab, 0, 1, 25, 1, "with icon, min size");
    SendMessage(hwTab, TCM_SETPADDING, 0, MAKELPARAM(4,4));
    TabCheckSetSize(hwTab, 0, 1, 25, 1, "set padding, min size");

    DestroyWindow (hwTab);

    hwTab = create_tabcontrol(0, TCIF_TEXT|TCIF_IMAGE);
    SendMessage(hwTab, TCM_SETMINTABWIDTH, 0, nMinTabWidth);

    trace ("  non fixed width, with text...\n");
    CheckSize(hwTab, max(size.cx +TAB_PADDING_X*2, (nMinTabWidth < 0) ? DEFAULT_MIN_TAB_WIDTH : nMinTabWidth), -1,
              "no icon, default width");
    for (i=0; i<8; i++)
    {
        INT nTabWidth = (nMinTabWidth < 0) ? TabWidthPadded(i, 2) : nMinTabWidth;

        SendMessage(hwTab, TCM_SETIMAGELIST, 0, 0);
        SendMessage(hwTab, TCM_SETPADDING, 0, MAKELPARAM(i,i));

        TabCheckSetSize(hwTab, 50, 20, max(size.cx + i*2, nTabWidth), 20, "no icon, set size");
        TabCheckSetSize(hwTab, 0, 1, max(size.cx + i*2, nTabWidth), 1, "no icon, min size");

        SendMessage(hwTab, TCM_SETIMAGELIST, 0, (LPARAM)himl);
        nTabWidth = (nMinTabWidth < 0) ? TabWidthPadded(i, 3) : nMinTabWidth;

        TabCheckSetSize(hwTab, 50, 30, max(size.cx + 21 + i*3, nTabWidth), 30, "with icon, set size > icon");
        TabCheckSetSize(hwTab, 20, 20, max(size.cx + 21 + i*3, nTabWidth), 20, "with icon, set size < icon");
        TabCheckSetSize(hwTab, 0, 1, max(size.cx + 21 + i*3, nTabWidth), 1, "with icon, min size");
    }
    DestroyWindow (hwTab);

    hwTab = create_tabcontrol(0, TCIF_IMAGE);
    SendMessage(hwTab, TCM_SETMINTABWIDTH, 0, nMinTabWidth);

    trace ("  non fixed width, no text...\n");
    CheckSize(hwTab, (nMinTabWidth < 0) ? DEFAULT_MIN_TAB_WIDTH : nMinTabWidth, -1, "no icon, default width");
    for (i=0; i<8; i++)
    {
        INT nTabWidth = (nMinTabWidth < 0) ? TabWidthPadded(i, 2) : nMinTabWidth;

        SendMessage(hwTab, TCM_SETIMAGELIST, 0, 0);
        SendMessage(hwTab, TCM_SETPADDING, 0, MAKELPARAM(i,i));

        TabCheckSetSize(hwTab, 50, 20, nTabWidth, 20, "no icon, set size");
        TabCheckSetSize(hwTab, 0, 1, nTabWidth, 1, "no icon, min size");

        SendMessage(hwTab, TCM_SETIMAGELIST, 0, (LPARAM)himl);
        if (i > 1 && nMinTabWidth > 0 && nMinTabWidth < DEFAULT_MIN_TAB_WIDTH)
            nTabWidth += EXTRA_ICON_PADDING *(i-1);

        TabCheckSetSize(hwTab, 50, 30, nTabWidth, 30, "with icon, set size > icon");
        TabCheckSetSize(hwTab, 20, 20, nTabWidth, 20, "with icon, set size < icon");
        TabCheckSetSize(hwTab, 0, 1, nTabWidth, 1, "with icon, min size");
    }

    DestroyWindow (hwTab);

    ImageList_Destroy(himl);
    DeleteObject(hFont);
}

START_TEST(tab)
{
    LOGFONTA logfont;

    lstrcpyA(logfont.lfFaceName, "Arial");
    memset(&logfont, 0, sizeof(logfont));
    logfont.lfHeight = -12;
    logfont.lfWeight = FW_NORMAL;
    logfont.lfCharSet = ANSI_CHARSET;
    hFont = CreateFontIndirectA(&logfont);

    InitCommonControls();

    trace ("Testing with default MinWidth\n");
    test_tab(-1);
    trace ("Testing with MinWidth set to -3\n");
    test_tab(-3);
    trace ("Testing with MinWidth set to 24\n");
    test_tab(24);
    trace ("Testing with MinWidth set to 54\n");
    test_tab(54);
    trace ("Testing with MinWidth set to 94\n");
    test_tab(94);
}
