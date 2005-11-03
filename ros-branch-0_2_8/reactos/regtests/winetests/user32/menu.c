/*
 * Unit tests for menus
 *
 * Copyright 2005 Robert Shearman
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

#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"

#include "wine/test.h"

static ATOM atomMenuCheckClass;

static LRESULT WINAPI menu_check_wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_ENTERMENULOOP:
        /* mark window as having entered menu loop */
        SetWindowLongPtr(hwnd, GWLP_USERDATA, TRUE);
        /* exit menu modal loop
         * ( A SendMessage does not work on NT3.51 here ) */
        return PostMessage(hwnd, WM_CANCELMODE, 0, 0);
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

/* globals to communicate between test and wndproc */
unsigned int MOD_maxid;
RECT MOD_rc[4];
int MOD_avec, MOD_hic;
int MOD_odheight;
#define MOD_SIZE 10
/* wndproc used by test_menu_ownerdraw() */
static LRESULT WINAPI menu_ownerdraw_wnd_proc(HWND hwnd, UINT msg,
        WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
        case WM_MEASUREITEM:
            {
                MEASUREITEMSTRUCT* pmis = (MEASUREITEMSTRUCT*)lparam;
                if( winetest_debug)
                    trace("WM_MEASUREITEM received %d,%d\n",
                            pmis->itemWidth, pmis->itemHeight);
                MOD_odheight = pmis->itemHeight;
                pmis->itemWidth = MOD_SIZE;
                pmis->itemHeight = MOD_SIZE;
                return TRUE;
            }
        case WM_DRAWITEM:
            {
                DRAWITEMSTRUCT * pdis;
                TEXTMETRIC tm;
                HPEN oldpen;
                char chrs[]="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
                SIZE sz;
                pdis = (DRAWITEMSTRUCT *) lparam;
                if( winetest_debug) {
                    trace("WM_DRAWITEM received itemdata %ld item %d rc %ld,%ld-%ld,%ld \n",
                            pdis->itemData,
                            pdis->itemID, pdis->rcItem.left, pdis->rcItem.top,
                            pdis->rcItem.right,pdis->rcItem.bottom );
                    oldpen=SelectObject( pdis->hDC, GetStockObject(
                                pdis->itemState & ODS_SELECTED ? WHITE_PEN :BLACK_PEN));
                    Rectangle( pdis->hDC, pdis->rcItem.left,pdis->rcItem.top,
                            pdis->rcItem.right,pdis->rcItem.bottom );
                    SelectObject( pdis->hDC, oldpen);
                }
                if( pdis->itemData > MOD_maxid) return TRUE;
                /* store the rectangl */
                MOD_rc[pdis->itemData] = pdis->rcItem;
                /* calculate average character width */
                GetTextExtentPoint( pdis->hDC, chrs, 52, &sz );
                MOD_avec = (sz.cx + 26)/52;
                GetTextMetrics( pdis->hDC, &tm);
                MOD_hic = tm.tmHeight;
                if( pdis->itemData == MOD_maxid) PostMessage(hwnd, WM_CANCELMODE, 0, 0);
                return TRUE;
            }

    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

static void register_menu_check_class(void)
{
    WNDCLASS wc =
    {
        0,
        menu_check_wnd_proc,
        0,
        0,
        GetModuleHandle(NULL),
        NULL,
        LoadCursor(NULL, IDC_ARROW),
        (HBRUSH)(COLOR_BTNFACE+1),
        NULL,
        TEXT("WineMenuCheck"),
    };
    
    atomMenuCheckClass = RegisterClass(&wc);
}

/* demonstrates that windows lock the menu object so that it is still valid
 * even after a client calls DestroyMenu on it */
static void test_menu_locked_by_window(void)
{
    BOOL ret;
    HMENU hmenu;
    HWND hwnd = CreateWindowEx(0, MAKEINTATOM(atomMenuCheckClass), NULL,
        WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL, NULL, NULL, NULL);
    ok(hwnd != NULL, "CreateWindowEx failed with error %ld\n", GetLastError());
    hmenu = CreateMenu();
    ok(hmenu != NULL, "CreateMenu failed with error %ld\n", GetLastError());
    ret = InsertMenu(hmenu, 0, MF_STRING, 0, TEXT("&Test"));
    ok(ret, "InsertMenu failed with error %ld\n", GetLastError());
    ret = SetMenu(hwnd, hmenu);
    ok(ret, "SetMenu failed with error %ld\n", GetLastError());
    ret = DestroyMenu(hmenu);
    ok(ret, "DestroyMenu failed with error %ld\n", GetLastError());

    ret = DrawMenuBar(hwnd);
    todo_wine {
    ok(ret, "DrawMenuBar failed with error %ld\n", GetLastError());
    }
    ret = IsMenu(GetMenu(hwnd));
    ok(!ret, "Menu handle should have been destroyed\n");

    SendMessage(hwnd, WM_SYSCOMMAND, SC_KEYMENU, 0);
    /* did we process the WM_INITMENU message? */
    ret = GetWindowLongPtr(hwnd, GWLP_USERDATA);
    todo_wine {
    ok(ret, "WM_INITMENU should have been sent\n");
    }

    DestroyWindow(hwnd);
}

static void test_menu_ownerdraw(void)
{
    int i,j,k;
    BOOL ret;
    HMENU hmenu;
    LONG leftcol;
    HWND hwnd = CreateWindowEx(0, MAKEINTATOM(atomMenuCheckClass), NULL,
            WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            NULL, NULL, NULL, NULL);
    ok(hwnd != NULL, "CreateWindowEx failed with error %ld\n", GetLastError());
    if( !hwnd) return;
    SetWindowLong( hwnd, GWL_WNDPROC, (LONG)menu_ownerdraw_wnd_proc);
    hmenu = CreatePopupMenu();
    ok(hmenu != NULL, "CreateMenu failed with error %ld\n", GetLastError());
    if( !hmenu) { DestroyWindow(hwnd);return;}
    k=0;
    for( j=0;j<2;j++) /* create columns */
        for(i=0;i<2;i++) { /* create rows */
            ret = AppendMenu( hmenu, MF_OWNERDRAW | 
                    (i==0 ? MF_MENUBREAK : 0), k, (LPCTSTR) k);
            k++;
            ok( ret, "AppendMenu failed for %d\n", k-1);
        }
    MOD_maxid = k-1;
    assert( k <= sizeof(MOD_rc)/sizeof(RECT));
    /* display the menu */
    ret = TrackPopupMenu( hmenu, 0x100, 100,100, 0, hwnd, NULL);

    /* columns have a 4 pixel gap between them */
    ok( MOD_rc[0].right + 4 ==  MOD_rc[2].left,
            "item rectangles are not separated by 4 pixels space\n");
    /* height should be what the MEASUREITEM message has returned */
    ok( MOD_rc[0].bottom - MOD_rc[0].top == MOD_SIZE,
            "menu item has wrong height: %ld should be %d\n",
            MOD_rc[0].bottom - MOD_rc[0].top, MOD_SIZE);
    /* no gaps between the rows */
    ok( MOD_rc[0].bottom - MOD_rc[1].top == 0,
            "There should not be a space between the rows, gap is %ld\n",
            MOD_rc[0].bottom - MOD_rc[1].top);
    /* test the correct value of the item height that was sent
     * by the WM_MEASUREITEM message */
    ok( MOD_odheight == HIWORD( GetDialogBaseUnits()) || /* WinNT,2k,XP */
            MOD_odheight == MOD_hic,                     /* Win95,98,ME */
            "Wrong height field in MEASUREITEMSTRUCT, expected %d or %d actual %d\n",
            HIWORD( GetDialogBaseUnits()), MOD_hic, MOD_odheight);
    /* test what MF_MENUBREAK did at the first position. Also show
     * that an MF_SEPARATOR is ignored in the height calculation. */
    leftcol= MOD_rc[0].left;
    ModifyMenu( hmenu, 0, MF_BYCOMMAND| MF_OWNERDRAW| MF_SEPARATOR, 0, 0); 
    /* display the menu */
    ret = TrackPopupMenu( hmenu, 0x100, 100,100, 0, hwnd, NULL);
    /* left should be 4 pixels less now */
    ok( leftcol == MOD_rc[0].left + 4, 
            "columns should be 4 pixels to the left (actual %ld).\n",
            leftcol - MOD_rc[0].left);
    /* test width */
    ok( MOD_rc[0].right - MOD_rc[0].left == 2 * MOD_avec + MOD_SIZE,
            "width of owner drawn menu item is wrong. Got %ld expected %d\n",
            MOD_rc[0].right - MOD_rc[0].left , 2*MOD_avec + MOD_SIZE);
    /* and height */
    ok( MOD_rc[0].bottom - MOD_rc[0].top == MOD_SIZE,
            "Height is incorrect. Got %ld expected %d\n",
            MOD_rc[0].bottom - MOD_rc[0].top, MOD_SIZE);

    /* test width/height of a OD menu bar as well */
    ret = DestroyMenu(hmenu);
    ok(ret, "DestroyMenu failed with error %ld\n", GetLastError());
    hmenu = CreateMenu();
    ok(hmenu != NULL, "CreateMenu failed with error %ld\n", GetLastError());
    if( !hmenu) { DestroyWindow(hwnd);return;}
    MOD_maxid=1;
    for(i=0;i<2;i++) { 
        ret = AppendMenu( hmenu, MF_OWNERDRAW , i, 0);
        ok( ret, "AppendMenu failed for %d\n", i);
    }
    SetMenu( hwnd, hmenu);
    UpdateWindow( hwnd); /* hack for wine to draw the window + menu */
    ok(ret, "SetMenu failed with error %ld\n", GetLastError());
    /* test width */
    ok( MOD_rc[0].right - MOD_rc[0].left == 2 * MOD_avec + MOD_SIZE,
            "width of owner drawn menu item is wrong. Got %ld expected %d\n",
            MOD_rc[0].right - MOD_rc[0].left , 2*MOD_avec + MOD_SIZE);
    /* test hight */
    ok( MOD_rc[0].bottom - MOD_rc[0].top == GetSystemMetrics( SM_CYMENU) - 1,
            "Height of owner drawn menu item is wrong. Got %ld expected %d\n",
            MOD_rc[0].bottom - MOD_rc[0].top, GetSystemMetrics( SM_CYMENU) - 1);
    /* clean up */
    DestroyWindow(hwnd);
}

START_TEST(menu)
{
    register_menu_check_class();

    test_menu_locked_by_window();
    test_menu_ownerdraw();
}
