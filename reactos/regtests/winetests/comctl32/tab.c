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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <assert.h>
#include <windows.h>
#include <commctrl.h>

#include "wine/test.h"

#undef VISIBLE

#define TAB_DEFAULT_WIDTH 96
#define TAB_PADDING_X 2
#define TAB_PADDING_Y 2

#define TCS_BOTTOM              0x0002

#ifdef VISIBLE
#define WAIT Sleep (1000)
#define REDRAW(hwnd) RedrawWindow (hwnd, NULL, 0, RDW_UPDATENOW)
#define trace_tab(str) trace(str)
#else
#define WAIT
#define REDRAW(hwnd)
#define trace_tab(str)
#endif

static HWND
create_tabcontrol (DWORD style)
{
    HWND handle;
    TCITEM tcNewTab;

    handle = CreateWindow (
	WC_TABCONTROLA,
	"TestTab",
	WS_CLIPSIBLINGS | WS_CLIPCHILDREN | TCS_FOCUSNEVER | style,
        10, 10, 300, 100,
        NULL, NULL, NULL, 0);

    assert (handle);
    
    SetWindowLong(handle, GWL_STYLE, WS_CLIPSIBLINGS | WS_CLIPCHILDREN | TCS_FOCUSNEVER | style);

    tcNewTab.mask = TCIF_TEXT | TCIF_IMAGE;
    tcNewTab.pszText = "Tab 1";
    tcNewTab.iImage = 0;
    SendMessage (handle, TCM_INSERTITEM, 0, (LPARAM) &tcNewTab);
    tcNewTab.pszText = "Wide Tab 2";
    tcNewTab.iImage = 1;
    SendMessage (handle, TCM_INSERTITEM, 1, (LPARAM) &tcNewTab);
    tcNewTab.pszText = "T 3";
    tcNewTab.iImage = 2;
    SendMessage (handle, TCM_INSERTITEM, 2, (LPARAM) &tcNewTab);

#ifdef VISIBLE
    ShowWindow (handle, SW_SHOW);
#endif
    REDRAW(handle);
    WAIT;

    return handle;
}

static void CheckSize(HWND hwnd, INT width, INT height)
{
    RECT rTab, r1;

    r1.left=r1.top=r1.right=r1.bottom=0;
    SendMessage (hwnd, TCM_GETITEMRECT, 0, (LPARAM) &rTab);
    SendMessage (hwnd, TCM_ADJUSTRECT, FALSE, (LPARAM) &r1);
    /* trace ("Got (%ld,%ld)-(%ld,%ld)\n", rTab.left, rTab.top, rTab.right, rTab.bottom); */
    trace (" (%ld,%ld)-(%ld,%ld)\n", r1.left, r1.top, r1.right, r1.bottom);
    if ((width  >= 0) && (height < 0))
	ok (width  == rTab.right  - rTab.left, "Expected [%d] got [%ld]\n",  width,  rTab.right  - rTab.left);
    else if ((height >= 0) && (width  < 0))
	ok (height == rTab.bottom - rTab.top,  "Expected [%d] got [%ld]\n",  height, rTab.bottom - rTab.top);
    else
	ok ((width  == rTab.right  - rTab.left) &&
	    (height == rTab.bottom - rTab.top ),
	    "Expected [%d,%d] got [%ld,%ld]\n", width, height, rTab.right - rTab.left, rTab.bottom - rTab.top);
}

static void TabCheckSetSize(HWND hwnd, INT SetWidth, INT SetHeight, INT ExpWidth, INT ExpHeight)
{
    SendMessage (hwnd, TCM_SETITEMSIZE, 0,
	(LPARAM) MAKELPARAM((SetWidth >= 0) ? SetWidth:0, (SetHeight >= 0) ? SetHeight:0));
    REDRAW(hwnd);
    CheckSize(hwnd, ExpWidth, ExpHeight);
    WAIT;
}

START_TEST(tab)
{
    HWND hwTab;
    HIMAGELIST himl = ImageList_Create(21, 21, ILC_COLOR, 3, 4);

    InitCommonControls();


    hwTab = create_tabcontrol(TCS_FIXEDWIDTH);

    trace_tab ("Testing TCS_FIXEDWIDTH tabs no icon...\n");
    trace_tab ("  default width...\n");
    CheckSize(hwTab, TAB_DEFAULT_WIDTH, -1);
    trace_tab ("  set size...\n");
    TabCheckSetSize(hwTab, 50, 20, 50, 20);
    WAIT;
    trace_tab ("  min size...\n");
    TabCheckSetSize(hwTab, 0, 1, 0, 1);
    WAIT;

    SendMessage(hwTab, TCM_SETIMAGELIST, 0, (LPARAM)himl);

    trace_tab ("Testing TCS_FIXEDWIDTH tabs with icon...\n");
    trace_tab ("  set size > icon...\n");
    TabCheckSetSize(hwTab, 50, 30, 50, 30);
    trace_tab ("  set size < icon...\n");
    TabCheckSetSize(hwTab, 20, 20, 25, 20);
    trace_tab ("  min size...\n");
    TabCheckSetSize(hwTab, 0, 1, 25, 1);

    DestroyWindow (hwTab);

    trace_tab ("Testing TCS_FIXEDWIDTH buttons no icon...\n");
    hwTab = create_tabcontrol(TCS_FIXEDWIDTH | TCS_BUTTONS);

    trace_tab ("  default width...\n");
    CheckSize(hwTab, TAB_DEFAULT_WIDTH, -1);
    trace_tab ("  set size 1...\n");
    TabCheckSetSize(hwTab, 20, 20, 20, 20);
    trace_tab ("  set size 2...\n");
    TabCheckSetSize(hwTab, 10, 50, 10, 50);
    trace_tab ("  min size...\n");
    TabCheckSetSize(hwTab, 0, 1, 0, 1);

    SendMessage(hwTab, TCM_SETIMAGELIST, 0, (LPARAM)himl);

    trace_tab ("Testing TCS_FIXEDWIDTH buttons with icon...\n");
    trace_tab ("  set size > icon...\n");
    TabCheckSetSize(hwTab, 50, 30, 50, 30);
    trace_tab ("  set size < icon...\n");
    TabCheckSetSize(hwTab, 20, 20, 25, 20);
    trace_tab ("  min size...\n");
    TabCheckSetSize(hwTab, 0, 1, 25, 1);
    trace_tab (" Add padding...\n");
    SendMessage(hwTab, TCM_SETPADDING, 0, MAKELPARAM(4,4));
    trace_tab ("  min size...\n");
    TabCheckSetSize(hwTab, 0, 1, 25, 1);

    DestroyWindow (hwTab);

    hwTab = create_tabcontrol(TCS_FIXEDWIDTH | TCS_BOTTOM);
    trace_tab ("Testing TCS_FIXEDWIDTH | TCS_BOTTOM tabs no icon...\n");

    trace_tab ("  default width...\n");
    CheckSize(hwTab, TAB_DEFAULT_WIDTH, -1);
    trace_tab ("  set size 1...\n");
    TabCheckSetSize(hwTab, 20, 20, 20, 20);
    trace_tab ("  set size 2...\n");
    TabCheckSetSize(hwTab, 10, 50, 10, 50);
    trace_tab ("  min size...\n");
    TabCheckSetSize(hwTab, 0, 1, 0, 1);

    SendMessage(hwTab, TCM_SETIMAGELIST, 0, (LPARAM)himl);
    
    trace_tab ("Testing TCS_FIXEDWIDTH | TCS_BOTTOM tabs with icon...\n");
    trace_tab ("  set size > icon...\n");
    TabCheckSetSize(hwTab, 50, 30, 50, 30);
    trace_tab ("  set size < icon...\n");
    TabCheckSetSize(hwTab, 20, 20, 25, 20);
    trace_tab ("  min size...\n");
    TabCheckSetSize(hwTab, 0, 1, 25, 1);
    trace_tab (" Add padding...\n");
    SendMessage(hwTab, TCM_SETPADDING, 0, MAKELPARAM(4,4));
    trace_tab ("  min size...\n");
    TabCheckSetSize(hwTab, 0, 1, 25, 1);

    DestroyWindow (hwTab);


    ImageList_Destroy(himl);
}
