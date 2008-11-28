/*
 * Unit tests for menus
 *
 * Copyright 2005 Robert Shearman
 * Copyright 2007 Dmitry Timoshkov
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

#define _WIN32_WINNT 0x0501

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define OEMRESOURCE         /* For OBM_MNARROW */

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"

#include "wine/test.h"

static ATOM atomMenuCheckClass;

static BOOL (WINAPI *pGetMenuInfo)(HMENU,LPCMENUINFO);
static UINT (WINAPI *pSendInput)(UINT, INPUT*, size_t);
static BOOL (WINAPI *pSetMenuInfo)(HMENU,LPCMENUINFO);

static void init_function_pointers(void)
{
    HMODULE hdll = GetModuleHandleA("user32");

#define GET_PROC(func) \
    p ## func = (void*)GetProcAddress(hdll, #func); \
    if(!p ## func) \
      trace("GetProcAddress(%s) failed\n", #func);

    GET_PROC(GetMenuInfo)
    GET_PROC(SendInput)
    GET_PROC(SetMenuInfo)

#undef GET_PROC
}

static BOOL correct_behavior(void)
{
    HMENU hmenu;
    MENUITEMINFO info;
    BOOL rc;

    hmenu = CreateMenu();

    memset(&info, 0, sizeof(MENUITEMINFO));
    info.cbSize= sizeof(MENUITEMINFO);
    SetLastError(0xdeadbeef);
    rc = GetMenuItemInfo(hmenu, 0, TRUE, &info);
    /* Win9x  : 0xdeadbeef
     * NT4    : ERROR_INVALID_PARAMETER
     * >= W2K : ERROR_MENU_ITEM_NOT_FOUND
     */
    if (!rc && GetLastError() != ERROR_MENU_ITEM_NOT_FOUND)
    {
        win_skip("NT4 and below can't handle a bigger MENUITEMINFO struct\n");
        DestroyMenu(hmenu);
        return FALSE;
    }

    DestroyMenu(hmenu);
    return TRUE;
}

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

/* The MSVC headers ignore our NONAMELESSUNION requests so we have to define
 * our own type */
typedef struct
{
    DWORD type;
    union
    {
        MOUSEINPUT      mi;
        KEYBDINPUT      ki;
        HARDWAREINPUT   hi;
    } u;
} TEST_INPUT;

/* globals to communicate between test and wndproc */

static BOOL bMenuVisible;
static HMENU hMenus[4];

#define MOD_SIZE 10
#define MOD_NRMENUS 8

 /* menu texts with their sizes */
static struct {
    LPCSTR text;
    SIZE size; /* size of text up to any \t */
    SIZE sc_size; /* size of the short-cut */
} MOD_txtsizes[] = {
        { "Pinot &Noir" },
        { "&Merlot\bF4" },
        { "Shira&z\tAlt+S" },
        { "" },
        { NULL }
};

static unsigned int MOD_maxid;
static RECT MOD_rc[MOD_NRMENUS];
static int MOD_avec, MOD_hic;
static int MOD_odheight;
static SIZE MODsizes[MOD_NRMENUS]= { {MOD_SIZE, MOD_SIZE},{MOD_SIZE, MOD_SIZE},
    {MOD_SIZE, MOD_SIZE},{MOD_SIZE, MOD_SIZE}};
static int MOD_GotDrawItemMsg = FALSE;
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
                    trace("WM_MEASUREITEM received data %lx size %dx%d\n",
                            pmis->itemData, pmis->itemWidth, pmis->itemHeight);
                MOD_odheight = pmis->itemHeight;
                pmis->itemWidth = MODsizes[pmis->itemData].cx;
                pmis->itemHeight = MODsizes[pmis->itemData].cy;
                return TRUE;
            }
        case WM_DRAWITEM:
            {
                DRAWITEMSTRUCT * pdis;
                TEXTMETRIC tm;
                HPEN oldpen;
                char chrs[]="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
                SIZE sz;
                int i;
                pdis = (DRAWITEMSTRUCT *) lparam;
                if( winetest_debug) {
                    RECT rc;
                    GetMenuItemRect( hwnd, (HMENU)pdis->hwndItem, pdis->itemData ,&rc);
                    trace("WM_DRAWITEM received hwnd %p hmenu %p itemdata %ld item %d rc %d,%d-%d,%d itemrc:  %d,%d-%d,%d\n",
                            hwnd, (HMENU)pdis->hwndItem, pdis->itemData,
                            pdis->itemID, pdis->rcItem.left, pdis->rcItem.top,
                            pdis->rcItem.right,pdis->rcItem.bottom,
                            rc.left,rc.top,rc.right,rc.bottom);
                    oldpen=SelectObject( pdis->hDC, GetStockObject(
                                pdis->itemState & ODS_SELECTED ? WHITE_PEN :BLACK_PEN));
                    Rectangle( pdis->hDC, pdis->rcItem.left,pdis->rcItem.top,
                            pdis->rcItem.right,pdis->rcItem.bottom );
                    SelectObject( pdis->hDC, oldpen);
                }
                /* calculate widths of some menu texts */
                if( ! MOD_txtsizes[0].size.cx)
                    for(i = 0; MOD_txtsizes[i].text; i++) {
                        char buf[100], *p;
                        RECT rc={0,0,0,0};
                        strcpy( buf, MOD_txtsizes[i].text);
                        if( ( p = strchr( buf, '\t'))) {
                            *p = '\0';
                            DrawText( pdis->hDC, p + 1, -1, &rc,
                                    DT_SINGLELINE|DT_CALCRECT);
                            MOD_txtsizes[i].sc_size.cx= rc.right - rc.left;
                            MOD_txtsizes[i].sc_size.cy= rc.bottom - rc.top;
                        }
                        DrawText( pdis->hDC, buf, -1, &rc,
                                DT_SINGLELINE|DT_CALCRECT);
                        MOD_txtsizes[i].size.cx= rc.right - rc.left;
                        MOD_txtsizes[i].size.cy= rc.bottom - rc.top;
                    }

                if( pdis->itemData > MOD_maxid) return TRUE;
                /* store the rectangl */
                MOD_rc[pdis->itemData] = pdis->rcItem;
                /* calculate average character width */
                GetTextExtentPoint( pdis->hDC, chrs, 52, &sz );
                MOD_avec = (sz.cx + 26)/52;
                GetTextMetrics( pdis->hDC, &tm);
                MOD_hic = tm.tmHeight;
                MOD_GotDrawItemMsg = TRUE;
                return TRUE;
            }
        case WM_ENTERIDLE:
            {
                PostMessage(hwnd, WM_CANCELMODE, 0, 0);
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

/* demonstrates that windows locks the menu object so that it is still valid
 * even after a client calls DestroyMenu on it */
static void test_menu_locked_by_window(void)
{
    BOOL ret;
    HMENU hmenu;
    HWND hwnd = CreateWindowEx(0, MAKEINTATOM(atomMenuCheckClass), NULL,
                               WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 200, 200,
                               NULL, NULL, NULL, NULL);
    ok(hwnd != NULL, "CreateWindowEx failed with error %d\n", GetLastError());
    hmenu = CreateMenu();
    ok(hmenu != NULL, "CreateMenu failed with error %d\n", GetLastError());
    ret = InsertMenu(hmenu, 0, MF_STRING, 0, TEXT("&Test"));
    ok(ret, "InsertMenu failed with error %d\n", GetLastError());
    ret = SetMenu(hwnd, hmenu);
    ok(ret, "SetMenu failed with error %d\n", GetLastError());
    ret = DestroyMenu(hmenu);
    ok(ret, "DestroyMenu failed with error %d\n", GetLastError());

    ret = DrawMenuBar(hwnd);
    todo_wine {
    ok(ret, "DrawMenuBar failed with error %d\n", GetLastError());
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
                               WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 200, 200,
                               NULL, NULL, NULL, NULL);
    ok(hwnd != NULL, "CreateWindowEx failed with error %d\n", GetLastError());
    if( !hwnd) return;
    SetWindowLongPtr( hwnd, GWLP_WNDPROC, (LONG)menu_ownerdraw_wnd_proc);
    hmenu = CreatePopupMenu();
    ok(hmenu != NULL, "CreateMenu failed with error %d\n", GetLastError());
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
            "menu item has wrong height: %d should be %d\n",
            MOD_rc[0].bottom - MOD_rc[0].top, MOD_SIZE);
    /* no gaps between the rows */
    ok( MOD_rc[0].bottom - MOD_rc[1].top == 0,
            "There should not be a space between the rows, gap is %d\n",
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
            "columns should be 4 pixels to the left (actual %d).\n",
            leftcol - MOD_rc[0].left);
    /* test width */
    ok( MOD_rc[0].right - MOD_rc[0].left == 2 * MOD_avec + MOD_SIZE,
            "width of owner drawn menu item is wrong. Got %d expected %d\n",
            MOD_rc[0].right - MOD_rc[0].left , 2*MOD_avec + MOD_SIZE);
    /* and height */
    ok( MOD_rc[0].bottom - MOD_rc[0].top == MOD_SIZE,
            "Height is incorrect. Got %d expected %d\n",
            MOD_rc[0].bottom - MOD_rc[0].top, MOD_SIZE);

    /* test width/height of an ownerdraw menu bar as well */
    ret = DestroyMenu(hmenu);
    ok(ret, "DestroyMenu failed with error %d\n", GetLastError());
    hmenu = CreateMenu();
    ok(hmenu != NULL, "CreateMenu failed with error %d\n", GetLastError());
    if( !hmenu) { DestroyWindow(hwnd);return;}
    MOD_maxid=1;
    for(i=0;i<2;i++) { 
        ret = AppendMenu( hmenu, MF_OWNERDRAW , i, 0);
        ok( ret, "AppendMenu failed for %d\n", i);
    }
    ret = SetMenu( hwnd, hmenu);
    UpdateWindow( hwnd); /* hack for wine to draw the window + menu */
    ok(ret, "SetMenu failed with error %d\n", GetLastError());
    /* test width */
    ok( MOD_rc[0].right - MOD_rc[0].left == 2 * MOD_avec + MOD_SIZE,
            "width of owner drawn menu item is wrong. Got %d expected %d\n",
            MOD_rc[0].right - MOD_rc[0].left , 2*MOD_avec + MOD_SIZE);
    /* test hight */
    ok( MOD_rc[0].bottom - MOD_rc[0].top == GetSystemMetrics( SM_CYMENU) - 1,
            "Height of owner drawn menu item is wrong. Got %d expected %d\n",
            MOD_rc[0].bottom - MOD_rc[0].top, GetSystemMetrics( SM_CYMENU) - 1);

    /* clean up */
    ret = DestroyMenu(hmenu);
    ok(ret, "DestroyMenu failed with error %d\n", GetLastError());
    DestroyWindow(hwnd);
}

/* helper for test_menu_bmp_and_string() */
static void test_mbs_help( int ispop, int hassub, int mnuopt,
        HWND hwnd, int arrowwidth, int count, HBITMAP hbmp,
        SIZE bmpsize, LPCSTR text, SIZE size, SIZE sc_size)
{
    BOOL ret;
    HMENU hmenu, submenu;
    MENUITEMINFO mii={ sizeof( MENUITEMINFO )};
    MENUINFO mi;
    RECT rc;
    CHAR text_copy[16];
    int hastab,  expect;
    int failed = 0;

    MOD_GotDrawItemMsg = FALSE;
    mii.fMask = MIIM_FTYPE | MIIM_DATA | MIIM_STATE;
    mii.fType = 0;
    /* check the menu item unless MNS_CHECKORBMP is set */
    mii.fState = (mnuopt != 2 ? MFS_CHECKED : MFS_UNCHECKED);
    mii.dwItemData =0;
    MODsizes[0] = bmpsize;
    hastab = 0;
    if( text ) {
        char *p;
        mii.fMask |= MIIM_STRING;
        strcpy(text_copy, text);
        mii.dwTypeData = text_copy; /* structure member declared non-const */
        if( ( p = strchr( text, '\t'))) {
            hastab = *(p + 1) ? 2 : 1;
        }
    }
    /* tabs don't make sense in menubars */
    if(hastab && !ispop) return;
    if( hbmp) {
        mii.fMask |= MIIM_BITMAP;
        mii.hbmpItem = hbmp;
    }
    submenu = CreateMenu();
    ok( submenu != 0, "CreateMenu failed with error %d\n", GetLastError());
    if( ispop)
        hmenu = CreatePopupMenu();
    else
        hmenu = CreateMenu();
    ok( hmenu != 0, "Create{Popup}Menu failed with error %d\n", GetLastError());
    if( hassub) {
        mii.fMask |= MIIM_SUBMENU;
        mii.hSubMenu = submenu;
    }
    if( mnuopt) {
        mi.cbSize = sizeof(mi);
        mi.fMask = MIM_STYLE;
        pGetMenuInfo( hmenu, &mi);
        mi.dwStyle |= mnuopt == 1 ? MNS_NOCHECK : MNS_CHECKORBMP;
        ret = pSetMenuInfo( hmenu, &mi);
        ok( ret, "SetMenuInfo failed with error %d\n", GetLastError());
    }
    ret = InsertMenuItem( hmenu, 0, FALSE, &mii);
    ok( ret, "InsertMenuItem failed with error %d\n", GetLastError());
    failed = !ret;
    if( winetest_debug) {
        HDC hdc=GetDC(hwnd);
        RECT rc = {100, 50, 400, 70};
        char buf[100];

        sprintf( buf,"%d text \"%s\" mnuopt %d", count, text ? text: "(nil)", mnuopt);
        FillRect( hdc, &rc, (HBRUSH) COLOR_WINDOW);
        TextOut( hdc, 100, 50, buf, strlen( buf));
        ReleaseDC( hwnd, hdc);
    }
    if(ispop)
        ret = TrackPopupMenu( hmenu, 0x100, 100,100, 0, hwnd, NULL);
    else {
        ret = SetMenu( hwnd, hmenu);
        ok(ret, "SetMenu failed with error %d\n", GetLastError());
        DrawMenuBar( hwnd);
    }
    ret = GetMenuItemRect( hwnd, hmenu, 0, &rc);
    /* check menu width */
    if( ispop)
        expect = ( text || hbmp ?
                4 + (mnuopt != 1 ? GetSystemMetrics(SM_CXMENUCHECK) : 0)
                : 0) +
            arrowwidth  + MOD_avec + (hbmp ? bmpsize.cx + 2 : 0) +
            (text && hastab ? /* TAB space */
             MOD_avec + ( hastab==2 ? sc_size.cx : 0) : 0) +
            (text ?  2 + (text[0] ? size.cx :0): 0) ;
    else
        expect = !(text || hbmp) ? 0 :
            ( hbmp ? (text ? 2:0) + bmpsize.cx  : 0 ) +
            (text ? 2 * MOD_avec + (text[0] ? size.cx :0): 0) ;
    ok( rc.right - rc.left == expect,
            "menu width wrong, got %d expected %d\n", rc.right - rc.left, expect);
    failed = failed || !(rc.right - rc.left == expect);
    /* check menu height */
    if( ispop)
        expect = max( ( !(text || hbmp) ? GetSystemMetrics( SM_CYMENUSIZE)/2 : 0),
                max( (text ? max( 2 + size.cy, MOD_hic + 4) : 0),
                    (hbmp ? bmpsize.cy + 2 : 0)));
    else
        expect = ( !(text || hbmp) ? GetSystemMetrics( SM_CYMENUSIZE)/2 :
                max( GetSystemMetrics( SM_CYMENU) - 1, (hbmp ? bmpsize.cy : 0)));
    ok( rc.bottom - rc.top == expect,
            "menu height wrong, got %d expected %d (%d)\n",
            rc.bottom - rc.top, expect, GetSystemMetrics( SM_CYMENU));
    failed = failed || !(rc.bottom - rc.top == expect);
    if( hbmp == HBMMENU_CALLBACK && MOD_GotDrawItemMsg) {
        /* check the position of the bitmap */
        /* horizontal */
        if (!ispop)
            expect = 3;
        else if (mnuopt == 0)
            expect = 4 + GetSystemMetrics(SM_CXMENUCHECK);
        else if (mnuopt == 1)
            expect = 4;
        else /* mnuopt == 2 */
            expect = 2;
        ok( expect == MOD_rc[0].left,
                "bitmap left is %d expected %d\n", MOD_rc[0].left, expect);
        failed = failed || !(expect == MOD_rc[0].left);
        /* vertical */
        expect = (rc.bottom - rc.top - MOD_rc[0].bottom + MOD_rc[0].top) / 2;
        ok( expect == MOD_rc[0].top,
                "bitmap top is %d expected %d\n", MOD_rc[0].top, expect);
        failed = failed || !(expect == MOD_rc[0].top);
    }
    /* if there was a failure, report details */
    if( failed) {
        trace("*** count %d text \"%s\" bitmap %p bmsize %d,%d textsize %d+%d,%d mnuopt %d hastab %d\n",
                count, text ? text: "(nil)", hbmp, bmpsize.cx, bmpsize.cy,
                size.cx, size.cy, sc_size.cx, mnuopt, hastab);
        trace("    check %d,%d arrow %d avechar %d\n",
                GetSystemMetrics(SM_CXMENUCHECK ),
                GetSystemMetrics(SM_CYMENUCHECK ),arrowwidth, MOD_avec);
        if( hbmp == HBMMENU_CALLBACK)
            trace( "    rc %d,%d-%d,%d bmp.rc %d,%d-%d,%d\n",
                rc.left, rc.top, rc.top, rc.bottom, MOD_rc[0].left,
                MOD_rc[0].top,MOD_rc[0].right, MOD_rc[0].bottom);
    }
    /* clean up */
    ret = DestroyMenu(submenu);
    ok(ret, "DestroyMenu failed with error %d\n", GetLastError());
    ret = DestroyMenu(hmenu);
    ok(ret, "DestroyMenu failed with error %d\n", GetLastError());
}


static void test_menu_bmp_and_string(void)
{
    BYTE bmfill[300];
    HBITMAP hbm_arrow;
    BITMAP bm;
    INT arrowwidth;
    HWND hwnd;
    int count, szidx, txtidx, bmpidx, hassub, mnuopt, ispop;

    if( !pGetMenuInfo)
    {
        skip("GetMenuInfo is not available\n");
        return;
    }

    memset( bmfill, 0xcc, sizeof( bmfill));
    hwnd = CreateWindowEx(0, MAKEINTATOM(atomMenuCheckClass), NULL,
                          WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 200, 200,
                          NULL, NULL, NULL, NULL);
    hbm_arrow=LoadBitmap( 0, (CHAR*)OBM_MNARROW);
    GetObject( hbm_arrow, sizeof(bm), &bm);
    arrowwidth = bm.bmWidth;

    ok(hwnd != NULL, "CreateWindowEx failed with error %d\n", GetLastError());
    if( !hwnd) return;
    SetWindowLongPtr( hwnd, GWLP_WNDPROC, (LONG)menu_ownerdraw_wnd_proc);

    if( winetest_debug)
        trace("    check %d,%d arrow %d avechar %d\n",
                GetSystemMetrics(SM_CXMENUCHECK ),
                GetSystemMetrics(SM_CYMENUCHECK ),arrowwidth, MOD_avec);
    count = 0;
    MOD_maxid = 0;
    for( ispop=1; ispop >= 0; ispop--){
        static SIZE bmsizes[]= {
            {10,10},{38,38},{1,30},{55,5}};
        for( szidx=0; szidx < sizeof( bmsizes) / sizeof( SIZE); szidx++) {
            HBITMAP hbm = CreateBitmap( bmsizes[szidx].cx, bmsizes[szidx].cy,1,1,bmfill);
            HBITMAP bitmaps[] = { HBMMENU_CALLBACK, hbm, NULL  };
            ok( (int)hbm, "CreateBitmap failed err %d\n", GetLastError());
            for( txtidx = 0; txtidx < sizeof(MOD_txtsizes)/sizeof(MOD_txtsizes[0]); txtidx++) {
                for( hassub = 0; hassub < 2 ; hassub++) { /* add submenu item */
                    for( mnuopt = 0; mnuopt < 3 ; mnuopt++){ /* test MNS_NOCHECK/MNS_CHECKORBMP */
                        for( bmpidx = 0; bmpidx <sizeof(bitmaps)/sizeof(HBITMAP); bmpidx++) {
                            /* no need to test NULL bitmaps of several sizes */
                            if( !bitmaps[bmpidx] && szidx > 0) continue;
                            if( !ispop && hassub) continue;
                            test_mbs_help( ispop, hassub, mnuopt,
                                    hwnd, arrowwidth, ++count,
                                    bitmaps[bmpidx],
                                    bmsizes[szidx],
                                    MOD_txtsizes[txtidx].text,
                                    MOD_txtsizes[txtidx].size,
                                    MOD_txtsizes[txtidx].sc_size);
                        }
                    }
                }
            }
            DeleteObject( hbm);
        }
    }
    /* clean up */
    DestroyWindow(hwnd);
}

static void test_menu_add_string( void )
{
    HMENU hmenu;
    MENUITEMINFO info;
    BOOL rc;
    int ret;

    char string[0x80];
    char string2[0x80];

    char strback[0x80];
    WCHAR strbackW[0x80];
    static CHAR blah[] = "blah";
    static const WCHAR expectedString[] = {'D','u','m','m','y',' ','s','t','r','i','n','g', 0};

    hmenu = CreateMenu();

    memset( &info, 0, sizeof info );
    info.cbSize = sizeof info;
    info.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_STATE | MIIM_ID;
    info.dwTypeData = blah;
    info.cch = 6;
    info.dwItemData = 0;
    info.wID = 1;
    info.fState = 0;
    InsertMenuItem(hmenu, 0, TRUE, &info );

    memset( &info, 0, sizeof info );
    info.cbSize = sizeof info;
    info.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_STATE | MIIM_DATA | MIIM_ID;
    info.dwTypeData = string;
    info.cch = sizeof string;
    string[0] = 0;
    GetMenuItemInfo( hmenu, 0, TRUE, &info );

    ok( !strcmp( string, "blah" ), "menu item name differed\n");

    /* Test combination of ownerdraw and strings with GetMenuItemString(A/W) */
    strcpy(string, "Dummy string");
    memset(&info, 0x00, sizeof(info));
    info.cbSize= sizeof(MENUITEMINFO); 
    info.fMask= MIIM_FTYPE | MIIM_STRING; /* Set OwnerDraw + typeData */
    info.fType= MFT_OWNERDRAW;
    info.dwTypeData= string; 
    rc = InsertMenuItem( hmenu, 0, TRUE, &info );
    ok (rc, "InsertMenuItem failed\n");

    strcpy(string,"Garbage");
    ok (GetMenuString( hmenu, 0, strback, 99, MF_BYPOSITION), "GetMenuString on ownerdraw entry failed\n");
    ok (!strcmp( strback, "Dummy string" ), "Menu text from Ansi version incorrect\n");

    SetLastError(0xdeadbeef);
    ret = GetMenuStringW( hmenu, 0, (WCHAR *)strbackW, 99, MF_BYPOSITION);
    if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
        skip("GetMenuStringW is not implemented\n");
    else
    {
        ok (ret, "GetMenuStringW on ownerdraw entry failed\n");
        ok (!lstrcmpW( strbackW, expectedString ), "Menu text from Unicode version incorrect\n");
    }

    /* Just change ftype to string and see what text is stored */
    memset(&info, 0x00, sizeof(info));
    info.cbSize= sizeof(MENUITEMINFO); 
    info.fMask= MIIM_FTYPE; /* Set string type */
    info.fType= MFT_STRING;
    info.dwTypeData= (char *)0xdeadbeef; 
    rc = SetMenuItemInfo( hmenu, 0, TRUE, &info );
    ok (rc, "SetMenuItemInfo failed\n");

    /* Did we keep the old dwTypeData? */
    ok (GetMenuString( hmenu, 0, strback, 99, MF_BYPOSITION), "GetMenuString on ownerdraw entry failed\n");
    ok (!strcmp( strback, "Dummy string" ), "Menu text from Ansi version incorrect\n");

    /* Ensure change to bitmap type fails */
    memset(&info, 0x00, sizeof(info));
    info.cbSize= sizeof(MENUITEMINFO); 
    info.fMask= MIIM_FTYPE; /* Set as bitmap type */
    info.fType= MFT_BITMAP;
    info.dwTypeData= (char *)0xdeadbee2; 
    rc = SetMenuItemInfo( hmenu, 0, TRUE, &info );
    ok (!rc, "SetMenuItemInfo unexpectedly worked\n");

    /* Just change ftype back and ensure data hasn't been freed */
    info.fType= MFT_OWNERDRAW; /* Set as ownerdraw type */
    info.dwTypeData= (char *)0xdeadbee3; 
    rc = SetMenuItemInfo( hmenu, 0, TRUE, &info );
    ok (rc, "SetMenuItemInfo failed\n");
    
    /* Did we keep the old dwTypeData? */
    ok (GetMenuString( hmenu, 0, strback, 99, MF_BYPOSITION), "GetMenuString on ownerdraw entry failed\n");
    ok (!strcmp( strback, "Dummy string" ), "Menu text from Ansi version incorrect\n");

    /* Just change string value (not type) */
    memset(&info, 0x00, sizeof(info));
    info.cbSize= sizeof(MENUITEMINFO); 
    info.fMask= MIIM_STRING; /* Set typeData */
    strcpy(string2, "string2");
    info.dwTypeData= string2; 
    rc = SetMenuItemInfo( hmenu, 0, TRUE, &info );
    ok (rc, "SetMenuItemInfo failed\n");

    ok (GetMenuString( hmenu, 0, strback, 99, MF_BYPOSITION), "GetMenuString on ownerdraw entry failed\n");
    ok (!strcmp( strback, "string2" ), "Menu text from Ansi version incorrect\n");

    /*  crashes with wine 0.9.5 */
    memset(&info, 0x00, sizeof(info));
    info.cbSize= sizeof(MENUITEMINFO); 
    info.fMask= MIIM_FTYPE | MIIM_STRING; /* Set OwnerDraw + typeData */
    info.fType= MFT_OWNERDRAW;
    rc = InsertMenuItem( hmenu, 0, TRUE, &info );
    ok (rc, "InsertMenuItem failed\n");
    ok (!GetMenuString( hmenu, 0, NULL, 0, MF_BYPOSITION),
            "GetMenuString on ownerdraw entry succeeded.\n");
    SetLastError(0xdeadbeef);
    ret = GetMenuStringW( hmenu, 0, NULL, 0, MF_BYPOSITION);
    if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
        skip("GetMenuStringW is not implemented\n");
    else
        ok (!ret, "GetMenuStringW on ownerdraw entry succeeded.\n");

    DestroyMenu( hmenu );
}

/* define building blocks for the menu item info tests */
static int strncmpW( const WCHAR *str1, const WCHAR *str2, int n )
{
    if (n <= 0) return 0;
    while ((--n > 0) && *str1 && (*str1 == *str2)) { str1++; str2++; }
    return *str1 - *str2;
}

static  WCHAR *strcpyW( WCHAR *dst, const WCHAR *src )
{
    WCHAR *p = dst;
    while ((*p++ = *src++));
    return dst;
}


#define DMIINFF( i, e, field)\
    ok((int)((i)->field)==(int)((e)->field) || (int)((i)->field)==(0xffff & (int)((e)->field)), \
    "%s got 0x%x expected 0x%x\n", #field, (int)((i)->field), (int)((e)->field));

#define DUMPMIINF(s,i,e)\
{\
    DMIINFF( i, e, fMask)\
    DMIINFF( i, e, fType)\
    DMIINFF( i, e, fState)\
    DMIINFF( i, e, wID)\
    DMIINFF( i, e, hSubMenu)\
    DMIINFF( i, e, hbmpChecked)\
    DMIINFF( i, e, hbmpUnchecked)\
    DMIINFF( i, e, dwItemData)\
    DMIINFF( i, e, dwTypeData)\
    DMIINFF( i, e, cch)\
    if( s==sizeof(MENUITEMINFOA)) DMIINFF( i, e, hbmpItem)\
}    

/* insert menu item */
#define TMII_INSMI( a1,b1,c1,d1,e1,f1,g1,h1,i1,j1,k1,l1,m1,n1,\
    eret1)\
{\
    MENUITEMINFOA info1=a1 b1,c1,d1,e1,f1,(void*)g1,(void*)h1,(void*)i1,j1,(void*)k1,l1,(void*)m1 n1;\
    HMENU hmenu = CreateMenu();\
    BOOL ret, stop = FALSE;\
    SetLastError( 0xdeadbeef);\
    if(ansi)strcpy( string, init);\
    else strcpyW( (WCHAR*)string, (WCHAR*)init);\
    if( ansi) ret = InsertMenuItemA(hmenu, 0, TRUE, &info1 );\
    else ret = InsertMenuItemW(hmenu, 0, TRUE, (MENUITEMINFOW*)&info1 );\
    if( GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)\
    {\
        skip("InsertMenuItem%s not implemented\n", ansi ? "A" : "W");\
        break;\
    }\
    if( !(eret1)) { ok( (eret1)==ret,"InsertMenuItem should have failed.\n");\
        stop = TRUE;\
    } else ok( (eret1)==ret,"InsertMenuItem failed, err %d\n",GetLastError());\


/* GetMenuItemInfo + GetMenuString  */
#define TMII_GMII( a2,b2,c2,d2,e2,f2,g2,h2,i2,j2,k2,l2,m2,n2,\
    a3,b3,c3,d3,e3,f3,g3,h3,i3,j3,k3,l3,m3,n3,\
    expname, eret2, eret3)\
{\
  MENUITEMINFOA info2A=a2 b2,c2,d2,e2,f2,(void*)g2,(void*)h2,(void*)i2,j2,(void*)k2,l2,(void*)m2 n2;\
  MENUITEMINFOA einfoA=a3 b3,c3,d3,e3,f3,(void*)g3,(void*)h3,(void*)i3,j3,(void*)k3,l3,(void*)m3 n3;\
  MENUITEMINFOA *info2 = &info2A;\
  MENUITEMINFOA *einfo = &einfoA;\
  MENUITEMINFOW *info2W = (MENUITEMINFOW *)&info2A;\
  if( !stop) {\
    SetLastError( 0xdeadbeef);\
    ret = ansi ? GetMenuItemInfoA( hmenu, 0, TRUE, info2 ) :\
        GetMenuItemInfoW( hmenu, 0, TRUE, info2W );\
    if( GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)\
    {\
        skip("GetMenuItemInfo%s not implemented\n", ansi ? "A" : "W");\
        break;\
    }\
    if( !(eret2)) ok( (eret2)==ret,"GetMenuItemInfo should have failed.\n");\
    else { \
      ok( (eret2)==ret,"GetMenuItemInfo failed, err %d\n",GetLastError());\
      ret = memcmp( info2, einfo, sizeof einfoA);\
    /*  ok( ret==0, "Got wrong menu item info data\n");*/\
      if( ret) DUMPMIINF(info2A.cbSize, &info2A, &einfoA)\
      if( einfo->dwTypeData == string) {\
        if(ansi) ok( !strncmp( expname, info2->dwTypeData, einfo->cch ), "menu item name differed \"%s\"\n",\
            einfo->dwTypeData ? einfo->dwTypeData: "");\
        else ok( !strncmpW( (WCHAR*)expname, (WCHAR*)info2->dwTypeData, einfo->cch ), "menu item name differed \"%s\"\n",\
            einfo->dwTypeData ? einfo->dwTypeData: "");\
        ret = ansi ? GetMenuStringA( hmenu, 0, string, 80, MF_BYPOSITION) :\
            GetMenuStringW( hmenu, 0, string, 80, MF_BYPOSITION);\
        if( (eret3)){\
            ok( ret, "GetMenuString failed, err %d\n",GetLastError());\
        }else\
            ok( !ret, "GetMenuString should have failed\n");\
      }\
    }\
  }\
}

#define TMII_DONE \
    RemoveMenu(hmenu, 0, TRUE );\
    DestroyMenu( hmenu );\
    DestroyMenu( submenu );\
submenu = CreateMenu();\
}
/* modify menu */
#define TMII_MODM( flags, id, data, eret  )\
if( !stop) {\
    SetLastError( 0xdeadbeef);\
    if(ansi)ret = ModifyMenuA( hmenu, 0, flags, (UINT_PTR)id, (char*)data);\
    else ret = ModifyMenuW( hmenu, 0, flags, (UINT_PTR)id, (WCHAR*)data);\
    if( GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)\
    {\
        skip("ModifyMenu%s not implemented\n", ansi ? "A" : "W");\
        break;\
    }\
    if( !(eret)) ok( (eret)==ret,"ModifyMenuA should have failed.\n");\
    else  ok( (eret)==ret,"ModifyMenuA failed, err %d\n",GetLastError());\
}

/* SetMenuItemInfo */
#define TMII_SMII( a1,b1,c1,d1,e1,f1,g1,h1,i1,j1,k1,l1,m1,n1,\
    eret1)\
if( !stop) {\
    MENUITEMINFOA info1=a1 b1,c1,d1,e1,f1,(void*)g1,(void*)h1,(void*)i1,j1,(void*)k1,l1,(void*)m1 n1;\
    SetLastError( 0xdeadbeef);\
    if(ansi)strcpy( string, init);\
    else strcpyW( (WCHAR*)string, (WCHAR*)init);\
    if( ansi) ret = SetMenuItemInfoA(hmenu, 0, TRUE, &info1 );\
    else ret = SetMenuItemInfoW(hmenu, 0, TRUE, (MENUITEMINFOW*)&info1 );\
    if( GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)\
    {\
        skip("SetMenuItemInfo%s not implemented\n", ansi ? "A" : "W");\
        break;\
    }\
    if( !(eret1)) { ok( (eret1)==ret,"InsertMenuItem should have failed.\n");\
        stop = TRUE;\
    } else ok( (eret1)==ret,"InsertMenuItem failed, err %d\n",GetLastError());\
}



#define OK 1
#define ER 0


static void test_menu_iteminfo( void )
{
  int S=sizeof( MENUITEMINFOA);
  int ansi = TRUE;
  char txtA[]="wine";
  char initA[]="XYZ";
  char emptyA[]="";
  WCHAR txtW[]={'W','i','n','e',0};
  WCHAR initW[]={'X','Y','Z',0};
  WCHAR emptyW[]={0};
  void *txt, *init, *empty, *string;
  HBITMAP hbm = CreateBitmap(1,1,1,1,NULL);
  char stringA[0x80];
  HMENU submenu=CreateMenu();

  do {
    if( ansi) {txt=txtA;init=initA;empty=emptyA;string=stringA;}
    else {txt=txtW;init=initW;empty=emptyW;string=stringA;}
    trace( "%s string %p hbm %p txt %p\n", ansi ?  "ANSI tests:   " : "Unicode tests:", string, hbm, txt);
    /* test all combinations of MFT_STRING, MFT_OWNERDRAW and MFT_BITMAP */
    /* (since MFT_STRING is zero, there are four of them) */
    TMII_INSMI( {, S, MIIM_TYPE, MFT_STRING, 0, 0, 0, 0, 0, 0, txt, 0, 0, }, OK)
    TMII_GMII ( {, S, MIIM_TYPE, -9, -9, -9, -9, -9, -9, -9, string, 80, -9, },
        {, S, MIIM_TYPE, MFT_STRING, -9, -9, 0, -9, -9, -9, string, 4, 0, },
        txt, OK, OK )
    TMII_DONE
    TMII_INSMI( {, S, MIIM_TYPE, MFT_STRING|MFT_OWNERDRAW, -1, -1, -1, -1, -1, -1, txt, 0, -1, }, OK)
    TMII_GMII ( {, S, MIIM_TYPE, -9, -9, -9, -9, -9, -9, -9, string, 80, -9, },
        {, S, MIIM_TYPE, MFT_STRING|MFT_OWNERDRAW, -9, -9, 0, -9, -9, -9, 0, 0, 0, },
        empty, OK, ER )
    TMII_DONE
    TMII_INSMI( {, S, MIIM_TYPE, MFT_BITMAP, -1, -1, -1, -1, -1, -1, hbm, 6, -1, }, OK)
    TMII_GMII ( {, S, MIIM_TYPE, -9, -9, -9, -9, -9, -9, -9, string, 80, -9, },
        {, S, MIIM_TYPE, MFT_BITMAP, -9, -9, 0, -9, -9, -9, hbm, 0, hbm, },
        empty, OK, ER )
    TMII_DONE
    TMII_INSMI( {, S, MIIM_TYPE, MFT_BITMAP|MFT_OWNERDRAW, -1, -1, -1, -1, -1, -1, hbm, 6, -1, }, OK)
    TMII_GMII ( {, S, MIIM_TYPE, -9, -9, -9, -9, -9, -9, -9, string, 80, -9, },
        {, S, MIIM_TYPE, MFT_BITMAP|MFT_OWNERDRAW, -9, -9, 0, -9, -9, -9, hbm, 0, hbm, },
        empty, OK, ER )
    TMII_DONE
    /* not enough space for name*/
    TMII_INSMI( {, S, MIIM_TYPE, MFT_STRING, -1, -1, -1, -1, -1, -1, txt, 6, -1, }, OK)
    TMII_GMII ( {, S, MIIM_TYPE, -9, -9, -9, -9, -9, -9, -9, NULL, 0, -9, },
        {, S, MIIM_TYPE, MFT_STRING, -9, -9, 0, -9, -9, -9, NULL, 4, 0, },
        empty, OK, OK )
    TMII_DONE
    TMII_INSMI( {, S, MIIM_TYPE, MFT_STRING, -1, -1, -1, -1, -1, -1, txt, 6, -1, }, OK)
    TMII_GMII ( {, S, MIIM_TYPE, -9, -9, -9, -9, -9, -9, -9, string, 5, -9, },
        {, S, MIIM_TYPE, MFT_STRING, -9, -9, 0, -9, -9, -9, string, 4, 0, },
        txt, OK, OK )
    TMII_DONE
    TMII_INSMI( {, S, MIIM_TYPE, MFT_STRING, -1, -1, -1, -1, -1, -1, txt, 6, -1, }, OK)
    TMII_GMII ( {, S, MIIM_TYPE, -9, -9, -9, -9, -9, -9, -9, string, 4, -9, },
        {, S, MIIM_TYPE, MFT_STRING, -9, -9, 0, -9, -9, -9, string, 3, 0, },
        txt, OK, OK )
    TMII_DONE
    TMII_INSMI( {, S, MIIM_FTYPE|MIIM_STRING, MFT_OWNERDRAW, -1, -1, -1, -1, -1, -1, NULL, 0, -1, }, OK)
    TMII_GMII ( {, S, MIIM_TYPE, -9, -9, -9, -9, -9, -9, -9, NULL, 0, -9, },
        {, S, MIIM_TYPE, MFT_OWNERDRAW, -9, -9, 0, -9, -9, -9, NULL, 0, 0, },
        empty, OK, ER )
    TMII_DONE
    /* cannot combine MIIM_TYPE with some other flags */
    TMII_INSMI( {, S, MIIM_TYPE|MIIM_STRING, MFT_STRING, -1, -1, -1, -1, -1, -1, txt, 6, -1, }, ER)
    TMII_GMII ( {, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
        {, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
        empty, OK, OK )
    TMII_DONE
    TMII_INSMI( {, S, MIIM_TYPE, MFT_STRING, -1, -1, -1, -1, -1, -1, txt, 6, -1, }, OK)
    TMII_GMII ( {, S, MIIM_TYPE|MIIM_STRING, -9, -9, -9, -9, -9, -9, -9, string, 80, -9, },
        {, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
        empty, ER, OK )
    TMII_DONE
    TMII_INSMI( {, S, MIIM_TYPE|MIIM_FTYPE, MFT_STRING, -1, -1, -1, -1, -1, -1, txt, 6, -1, }, ER)
    TMII_GMII ( {, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
        {, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
        empty, OK, OK )
    TMII_DONE
    TMII_INSMI( {, S, MIIM_TYPE, MFT_STRING, -1, -1, -1, -1, -1, -1, txt, 6, -1, }, OK)
    TMII_GMII ( {, S, MIIM_TYPE|MIIM_FTYPE, -9, -9, -9, -9, -9, -9, -9, string, 80, -9, },
        {, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
        empty, ER, OK )
    TMII_DONE
    TMII_INSMI( {, S, MIIM_TYPE|MIIM_BITMAP, MFT_BITMAP, -1, -1, -1, -1, -1, -1, hbm, 6, hbm, }, ER)
    TMII_GMII ( {, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
        {, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
        empty, OK, OK )
    TMII_DONE
        /* but succeeds with some others */
    TMII_INSMI( {, S, MIIM_TYPE, MFT_STRING, -1, -1, -1, -1, -1, -1, txt, 6, -1, }, OK)
    TMII_GMII ( {, S, MIIM_TYPE|MIIM_SUBMENU, -9, -9, -9, -9, -9, -9, -9, string, 80, -9, },
        {, S, MIIM_TYPE|MIIM_SUBMENU, MFT_STRING, -9, -9, 0, -9, -9, -9, string, 4, 0, },
        txt, OK, OK )
    TMII_DONE
    TMII_INSMI( {, S, MIIM_TYPE, MFT_STRING, -1, -1, -1, -1, -1, -1, txt, 6, -1, }, OK)
    TMII_GMII ( {, S, MIIM_TYPE|MIIM_STATE, -9, -9, -9, -9, -9, -9, -9, string, 80, -9, },
        {, S, MIIM_TYPE|MIIM_STATE, MFT_STRING, 0, -9, 0, -9, -9, -9, string, 4, 0, },
        txt, OK, OK )
    TMII_DONE
    TMII_INSMI( {, S, MIIM_TYPE|MIIM_ID, MFT_STRING, -1, 888, -1, -1, -1, -1, txt, 6, -1, }, OK)
    TMII_GMII ( {, S, MIIM_TYPE|MIIM_ID, -9, -9, -9, -9, -9, -9, -9, string, 80, -9, },
        {, S, MIIM_TYPE|MIIM_ID, MFT_STRING, -9, 888, 0, -9, -9, -9, string, 4, 0, },
        txt, OK, OK )
    TMII_DONE
    TMII_INSMI( {, S, MIIM_TYPE|MIIM_DATA, MFT_STRING, -1, -1, -1, -1, -1, 999, txt, 6, -1, }, OK)
    TMII_GMII ( {, S, MIIM_TYPE|MIIM_DATA, -9, -9, -9, -9, -9, -9, -9, string, 80, -9, },
        {, S, MIIM_TYPE|MIIM_DATA, MFT_STRING, -9, -9, 0, -9, -9, 999, string, 4, 0, },
        txt, OK, OK )
    TMII_DONE
    /* to be continued */
    /* set text with MIIM_TYPE and retrieve with MIIM_STRING */ 
    TMII_INSMI( {, S, MIIM_TYPE, MFT_STRING, -1, -1, -1, -1, -1, -1, txt, 6, -1, }, OK)
    TMII_GMII ( {, S, MIIM_STRING|MIIM_FTYPE, -9, -9, -9, -9, -9, -9, -9, string, 80, -9, },
        {, S, MIIM_STRING|MIIM_FTYPE, MFT_STRING, -9, -9, 0, -9, -9, -9, string, 4, -9, },
        txt, OK, OK )
    TMII_DONE
    /* set text with MIIM_TYPE and retrieve with MIIM_STRING; MFT_OWNERDRAW causes an empty string */ 
    TMII_INSMI( {, S, MIIM_TYPE, MFT_STRING|MFT_OWNERDRAW, -1, -1, -1, -1, -1, -1, txt, 6, -1, }, OK)
    TMII_GMII ( {, S, MIIM_STRING|MIIM_FTYPE, -9, -9, -9, -9, -9, -9, -9, string, 80, -9, },
        {, S, MIIM_STRING|MIIM_FTYPE, MFT_STRING|MFT_OWNERDRAW, -9, -9, 0, -9, -9, -9, string, 0, -9, },
        empty, OK, ER )
    TMII_DONE
    TMII_INSMI( {, S, MIIM_TYPE, MFT_STRING|MFT_OWNERDRAW, -1, -1, -1, -1, -1, -1, NULL, 0, -1, }, OK)
    TMII_GMII ( {, S, MIIM_STRING|MIIM_FTYPE, -9, -9, -9, -9, -9, -9, -9, string, 80, -9, },
        {, S, MIIM_STRING|MIIM_FTYPE, MFT_OWNERDRAW, -9, -9, 0, -9, -9, -9, string, 0, -9, },
        empty, OK, ER )
    TMII_DONE
    TMII_INSMI( {, S, MIIM_TYPE, MFT_STRING|MFT_OWNERDRAW, -1, -1, -1, -1, -1, -1, NULL, 0, -1, }, OK)
    TMII_GMII ( {, S, MIIM_FTYPE, -9, -9, -9, -9, -9, -9, -9, string, 80, -9, },
        {, S, MIIM_FTYPE, MFT_OWNERDRAW, -9, -9, 0, -9, -9, -9, string, 80, -9, },
        init, OK, ER )
    TMII_DONE
    TMII_INSMI( {, S, MIIM_TYPE, MFT_STRING, -1, -1, -1, -1, -1, -1, txt, 0, -1, }, OK)
    TMII_GMII ( {, S, 0, -9, -9, -9, -9, -9, -9, -9, string, 80, -9, },
        {, S, 0, -9, -9, -9, 0, -9, -9, -9, string, 80, -9, },
        init, OK, OK )
    TMII_DONE
    /* contrary to MIIM_TYPE,you can set the text for an owner draw menu */ 
    TMII_INSMI( {, S, MIIM_STRING|MIIM_FTYPE, MFT_STRING|MFT_OWNERDRAW, -1, -1, -1, -1, -1, -1, txt, 0, -1, }, OK)
    TMII_GMII ( {, S, MIIM_STRING|MIIM_FTYPE, -9, -9, -9, -9, -9, -9, -9, string, 80, -9, },
        {, S, MIIM_STRING|MIIM_FTYPE, MFT_OWNERDRAW, -9, -9, 0, -9, -9, -9, string, 4, -9, },
        txt, OK, OK )
    TMII_DONE
    /* same but retrieve with MIIM_TYPE */ 
    TMII_INSMI( {, S, MIIM_STRING|MIIM_FTYPE, MFT_STRING|MFT_OWNERDRAW, -1, -1, -1, -1, -1, -1, txt, 0, -1, }, OK)
    TMII_GMII ( {, S, MIIM_TYPE, -9, -9, -9, -9, -9, -9, -9, string, 80, -9, },
        {, S, MIIM_TYPE, MFT_OWNERDRAW, -9, -9, 0, -9, -9, -9, NULL, 4, NULL, },
        txt, OK, OK )
    TMII_DONE
    TMII_INSMI( {, S, MIIM_STRING|MIIM_FTYPE, MFT_STRING|MFT_OWNERDRAW, -1, -1, -1, -1, -1, -1, NULL, 0, -1, }, OK)
    TMII_GMII ( {, S, MIIM_STRING|MIIM_FTYPE, -9, -9, -9, -9, -9, -9, -9, string, 80, -9, },
        {, S, MIIM_STRING|MIIM_FTYPE, MFT_OWNERDRAW, -9, -9, 0, -9, -9, -9, string, 0, -9, },
        empty, OK, ER )
    TMII_DONE
    TMII_INSMI( {, S, MIIM_STRING|MIIM_FTYPE, MFT_STRING, -1, -1, -1, -1, -1, -1, NULL, 0, -1, }, OK)
    TMII_GMII ( {, S, MIIM_STRING|MIIM_FTYPE, -9, -9, -9, -9, -9, -9, -9, string, 80, -9, },
        {, S, MIIM_STRING|MIIM_FTYPE, MFT_SEPARATOR, -9, -9, 0, -9, -9, -9, string, 0, -9, },
        empty, OK, ER )
    TMII_DONE

    /* How is that with bitmaps? */ 
    TMII_INSMI( {, S, MIIM_BITMAP, -1, -1, -1, -1, -1, -1, -1, -1, -1, hbm, }, OK)
    TMII_GMII ( {, S, MIIM_TYPE, -9, -9, -9, -9, -9, -9, -9, string, 80, -9, },
        {, S, MIIM_TYPE, MFT_BITMAP, -9, -9, 0, -9, -9, -9, hbm, 0, hbm, },
        empty, OK, ER )
    TMII_DONE
    TMII_INSMI( {, S, MIIM_BITMAP, -1, -1, -1, -1, -1, -1, -1, -1, -1, hbm, }, OK)
    TMII_GMII ( {, S, MIIM_BITMAP|MIIM_FTYPE, -9, -9, -9, -9, -9, -9, -9, string, 80, -9, },
        {, S, MIIM_BITMAP|MIIM_FTYPE, 0, -9, -9, 0, -9, -9, -9, string, 80, hbm, },
        init, OK, ER )
    TMII_DONE
        /* MIIM_BITMAP does not like MFT_BITMAP */
    TMII_INSMI( {, S, MIIM_BITMAP|MIIM_FTYPE, MFT_BITMAP, -1, -1, -1, -1, -1, -1, -1, -1, hbm, }, ER)
    TMII_GMII ( {, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
        {, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
        init, OK, OK )
    TMII_DONE
        /* no problem with OWNERDRAWN */
    TMII_INSMI( {, S, MIIM_BITMAP|MIIM_FTYPE, MFT_OWNERDRAW, -1, -1, -1, -1, -1, -1, -1, -1, hbm, }, OK)
    TMII_GMII ( {, S, MIIM_BITMAP|MIIM_FTYPE, -9, -9, -9, -9, -9, -9, -9, string, 80, -9, },
        {, S, MIIM_BITMAP|MIIM_FTYPE, MFT_OWNERDRAW, -9, -9, 0, -9, -9, -9, string, 80, hbm, },
        init, OK, ER )
    TMII_DONE
        /* setting MFT_BITMAP with MFT_FTYPE fails anyway */
    TMII_INSMI( {, S, MIIM_FTYPE, MFT_BITMAP, -1, -1, -1, -1, -1, -1, -1, -1, -1, }, ER)
    TMII_GMII ( {, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
        {, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
        empty, OK, OK )
    TMII_DONE

    /* menu with submenu */
    TMII_INSMI( {, S, MIIM_SUBMENU|MIIM_FTYPE, MFT_STRING, -1, -1, submenu, -1, -1, -1, txt, 0, -1, }, OK)
    TMII_GMII ( {, S, MIIM_SUBMENU, -9, -9, -9, -9, -9, -9, -9, string, 80, -9, },
        {, S, MIIM_SUBMENU, -9, -9, -9, submenu, -9, -9, -9, string, 80, -9, },
        init, OK, ER )
    TMII_DONE
    TMII_INSMI( {, S, MIIM_SUBMENU|MIIM_FTYPE, MFT_STRING, -1, -1, submenu, -1, -1, -1, empty, 0, -1, }, OK)
    TMII_GMII ( {, S, MIIM_SUBMENU, -9, -9, -9, -9, -9, -9, -9, string, 80, -9, },
        {, S, MIIM_SUBMENU, -9, -9, -9, submenu, -9, -9, -9, string, 80, -9, },
        init, OK, ER )
    TMII_DONE
    /* menu with submenu, without MIIM_SUBMENU the submenufield is cleared */
    TMII_INSMI( {, S, MIIM_SUBMENU|MIIM_FTYPE, MFT_STRING, -1, -1, submenu, -1, -1, -1, txt, 0, -1, }, OK)
    TMII_GMII ( {, S, MIIM_STRING|MIIM_FTYPE, -9, -9, -9, -9, -9, -9, -9, string, 80, -9, },
        {, S, MIIM_STRING|MIIM_FTYPE, MFT_STRING|MFT_SEPARATOR, -9, -9, 0, -9, -9, -9, string, 0, -9, },
        empty, OK, ER )
    TMII_GMII ( {, S, MIIM_SUBMENU|MIIM_FTYPE, -9, -9, -9, -9, -9, -9, -9, string, 80, -9, },
        {, S, MIIM_SUBMENU|MIIM_FTYPE, MFT_SEPARATOR, -9, -9, submenu, -9, -9, -9, string, 80, -9, },
        empty, OK, ER )
    TMII_DONE
    /* menu with invalid submenu */
    TMII_INSMI( {, S, MIIM_SUBMENU|MIIM_FTYPE, MFT_STRING, -1, -1, 999, -1, -1, -1, txt, 0, -1, }, ER)
    TMII_GMII ( {, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
        {, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
        init, OK, ER )
    TMII_DONE
    /* Separator */
    TMII_INSMI( {, S, MIIM_TYPE, MFT_SEPARATOR, 0, 0, 0, 0, 0, 0, txt, 0, 0, }, OK)
    TMII_GMII ( {, S, MIIM_TYPE, -9, -9, -9, -9, -9, -9, -9, string, 80, -9, },
        {, S, MIIM_TYPE, MFT_SEPARATOR, -9, -9, 0, -9, -9, -9, 0, 0, 0, },
        empty, OK, ER )
    TMII_DONE
    TMII_INSMI( {, S, MIIM_TYPE, MFT_BITMAP|MFT_SEPARATOR, -1, -1, -1, -1, -1, -1, hbm, 6, -1, }, OK)
    TMII_GMII ( {, S, MIIM_TYPE, -9, -9, -9, -9, -9, -9, -9, string, 80, -9, },
        {, S, MIIM_TYPE, MFT_BITMAP|MFT_SEPARATOR, -9, -9, 0, -9, -9, -9, hbm, 0, hbm, },
        empty, OK, ER )
    TMII_DONE
     /* SEPARATOR and STRING go well together */
    /* BITMAP and STRING go well together */
    TMII_INSMI( {, S, MIIM_STRING|MIIM_BITMAP, -1, -1, -1, -1, -1, -1, -1, txt, 6, hbm, }, OK)
    TMII_GMII ( {, S, MIIM_FTYPE|MIIM_STRING|MIIM_BITMAP, -9, -9, -9, -9, -9, -9, -9, string, 80, -9, },
        {, S, MIIM_FTYPE|MIIM_STRING|MIIM_BITMAP, MFT_STRING, -9, -9, 0, -9, -9, -9, string, 4, hbm, },
        txt, OK, OK )
    TMII_DONE
     /* BITMAP, SEPARATOR and STRING go well together */
    TMII_INSMI( {, S, MIIM_FTYPE|MIIM_STRING|MIIM_BITMAP, MFT_SEPARATOR, -1, -1, -1, -1, -1, -1, txt, 6, hbm, }, OK)
    TMII_GMII ( {, S, MIIM_FTYPE|MIIM_STRING|MIIM_BITMAP, -9, -9, -9, -9, -9, -9, -9, string, 80, -9, },
        {, S, MIIM_FTYPE|MIIM_STRING|MIIM_BITMAP, MFT_SEPARATOR, -9, -9, 0, -9, -9, -9, string, 4, hbm, },
        txt, OK, OK )
    TMII_DONE
     /* last two tests, but use MIIM_TYPE to retrieve info */
    TMII_INSMI( {, S, MIIM_FTYPE|MIIM_STRING, MFT_SEPARATOR, -1, -1, -1, -1, -1, -1, txt, 6, -1, }, OK)
    TMII_GMII ( {, S, MIIM_TYPE, -9, -9, -9, -9, -9, -9, -9, string, 80, -9, },
        {, S, MIIM_TYPE, MFT_SEPARATOR, -9, -9, 0, -9, -9, -9, NULL, 4, NULL, },
        txt, OK, OK )
    TMII_DONE
    TMII_INSMI( {, S, MIIM_STRING|MIIM_BITMAP, -1, -1, -1, -1, -1, -1, -1, txt, 6, hbm, }, OK)
    TMII_GMII ( {, S, MIIM_TYPE, -9, -9, -9, -9, -9, -9, -9, string, 80, -9, },
        {, S, MIIM_TYPE, MFT_BITMAP, -9, -9, 0, -9, -9, -9, hbm, 4, hbm, },
        txt, OK, OK )
    TMII_DONE
    TMII_INSMI( {, S, MIIM_FTYPE|MIIM_STRING|MIIM_BITMAP, MFT_SEPARATOR, -1, -1, -1, -1, -1, -1, txt, 6, hbm, }, OK)
    TMII_GMII ( {, S, MIIM_TYPE, -9, -9, -9, -9, -9, -9, -9, string, 80, -9, },
        {, S, MIIM_TYPE, MFT_SEPARATOR|MFT_BITMAP, -9, -9, 0, -9, -9, -9, hbm, 4, hbm, },
        txt, OK, OK )
    TMII_DONE
     /* same three with MFT_OWNERDRAW */
    TMII_INSMI( {, S, MIIM_FTYPE|MIIM_STRING, MFT_SEPARATOR|MFT_OWNERDRAW, -1, -1, -1, -1, -1, -1, txt, 6, -1, }, OK)
    TMII_GMII ( {, S, MIIM_TYPE, -9, -9, -9, -9, -9, -9, -9, string, 80, -9, },
        {, S, MIIM_TYPE, MFT_SEPARATOR|MFT_OWNERDRAW, -9, -9, 0, -9, -9, -9, NULL, 4, NULL, },
        txt, OK, OK )
    TMII_DONE
    TMII_INSMI( {, S, MIIM_FTYPE|MIIM_STRING|MIIM_BITMAP, MFT_OWNERDRAW, -1, -1, -1, -1, -1, -1, txt, 6, hbm, }, OK)
    TMII_GMII ( {, S, MIIM_TYPE, -9, -9, -9, -9, -9, -9, -9, string, 80, -9, },
        {, S, MIIM_TYPE, MFT_BITMAP|MFT_OWNERDRAW, -9, -9, 0, -9, -9, -9, hbm, 4, hbm, },
        txt, OK, OK )
    TMII_DONE
    TMII_INSMI( {, S, MIIM_FTYPE|MIIM_STRING|MIIM_BITMAP, MFT_SEPARATOR|MFT_OWNERDRAW, -1, -1, -1, -1, -1, -1, txt, 6, hbm, }, OK)
    TMII_GMII ( {, S, MIIM_TYPE, -9, -9, -9, -9, -9, -9, -9, string, 80, -9, },
        {, S, MIIM_TYPE, MFT_SEPARATOR|MFT_BITMAP|MFT_OWNERDRAW, -9, -9, 0, -9, -9, -9, hbm, 4, hbm, },
        txt, OK, OK )
    TMII_DONE

    TMII_INSMI( {, S, MIIM_STRING|MIIM_FTYPE|MIIM_ID, MFT_STRING|MFT_OWNERDRAW, -1, -1, -1, -1, -1, -1, txt, 0, -1, }, OK)
    TMII_GMII ( {, S, MIIM_TYPE, -9, -9, -9, -9, -9, -9, -9, string, 80, -9, },
        {, S, MIIM_TYPE, MFT_OWNERDRAW, -9, -9, 0, -9, -9, -9, NULL, 4, NULL, },
        txt,  OK, OK )
    TMII_DONE
    /* test with modifymenu: string is preserved after setting OWNERDRAW */
    TMII_INSMI( {, S, MIIM_STRING, MFT_STRING, -1, -1, -1, -1, -1, -1, txt, 0, -1, }, OK)
    TMII_MODM( MFT_OWNERDRAW, -1, 787, OK)
    TMII_GMII ( {, S, MIIM_FTYPE|MIIM_STRING|MIIM_DATA, -9, -9, -9, -9, -9, -9, -9, string, 80, -9, },
        {, S, MIIM_FTYPE|MIIM_STRING|MIIM_DATA, MFT_OWNERDRAW, -9, -9, 0, -9, -9, 787, string, 4, -9, },
        txt,  OK, OK )
    TMII_DONE
    /* same with bitmap: now the text is cleared */
    TMII_INSMI( {, S, MIIM_STRING, MFT_STRING, -1, -1, -1, -1, -1, -1, txt, 0, -1, }, OK)
    TMII_MODM( MFT_BITMAP, 545, hbm, OK)
    TMII_GMII ( {, S, MIIM_FTYPE|MIIM_STRING|MIIM_BITMAP|MIIM_ID, -9, -9, -9, -9, -9, -9, -9, string, 80, -9, },
        {, S, MIIM_FTYPE|MIIM_STRING|MIIM_BITMAP|MIIM_ID, MFT_BITMAP, -9, 545, 0, -9, -9, -9, string, 0, hbm, },
        empty,  OK, ER )
    TMII_DONE
    /* start with bitmap: now setting text clears it (though he flag is raised) */
    TMII_INSMI( {, S, MIIM_BITMAP, MFT_STRING, -1, -1, -1, -1, -1, -1, -1, -1, hbm, }, OK)
    TMII_GMII ( {, S, MIIM_FTYPE|MIIM_STRING|MIIM_BITMAP|MIIM_ID, -9, -9, -9, -9, -9, -9, -9, string, 80, -9, },
        {, S, MIIM_FTYPE|MIIM_STRING|MIIM_BITMAP|MIIM_ID, MFT_STRING, -9, 0, 0, -9, -9, -9, string, 0, hbm, },
        empty,  OK, ER )
    TMII_MODM( MFT_STRING, 545, txt, OK)
    TMII_GMII ( {, S, MIIM_FTYPE|MIIM_STRING|MIIM_BITMAP|MIIM_ID, -9, -9, -9, -9, -9, -9, -9, string, 80, -9, },
        {, S, MIIM_FTYPE|MIIM_STRING|MIIM_BITMAP|MIIM_ID, MFT_STRING, -9, 545, 0, -9, -9, -9, string, 4, 0, },
        txt,  OK, OK )
    TMII_DONE
    /*repeat with text NULL */
    TMII_INSMI( {, S, MIIM_BITMAP, MFT_STRING, -1, -1, -1, -1, -1, -1, -1, -1, hbm, }, OK)
    TMII_MODM( MFT_STRING, 545, NULL, OK)
    TMII_GMII ( {, S, MIIM_FTYPE|MIIM_STRING|MIIM_BITMAP|MIIM_ID, -9, -9, -9, -9, -9, -9, -9, string, 80, -9, },
        {, S, MIIM_FTYPE|MIIM_STRING|MIIM_BITMAP|MIIM_ID, MFT_SEPARATOR, -9, 545, 0, -9, -9, -9, string, 0, 0, },
        empty,  OK, ER )
    TMII_DONE
    /* repeat with text "" */
    TMII_INSMI( {, S, MIIM_BITMAP, -1 , -1, -1, -1, -1, -1, -1, -1, -1, hbm, }, OK)
    TMII_MODM( MFT_STRING, 545, empty, OK)
    TMII_GMII ( {, S, MIIM_FTYPE|MIIM_STRING|MIIM_BITMAP|MIIM_ID, -9, -9, -9, -9, -9, -9, -9, string, 80, -9, },
        {, S, MIIM_FTYPE|MIIM_STRING|MIIM_BITMAP|MIIM_ID, MFT_STRING, -9, 545, 0, -9, -9, -9, string, 0, 0, },
        empty,  OK, ER )
    TMII_DONE
    /* start with bitmap: set ownerdraw */
    TMII_INSMI( {, S, MIIM_BITMAP, -1, -1, -1, -1, -1, -1, -1, -1, -1, hbm, }, OK)
    TMII_MODM( MFT_OWNERDRAW, -1, 232, OK)
    TMII_GMII ( {, S, MIIM_FTYPE|MIIM_STRING|MIIM_BITMAP|MIIM_DATA, -9, -9, -9, -9, -9, -9, -9, string, 80, -9, },
        {, S, MIIM_FTYPE|MIIM_STRING|MIIM_BITMAP|MIIM_DATA, MFT_OWNERDRAW, -9, -9, 0, -9, -9, 232, string, 0, hbm, },
        empty,  OK, ER )
    TMII_DONE
    /* ask nothing */
    TMII_INSMI( {, S, MIIM_FTYPE|MIIM_STRING|MIIM_BITMAP, MFT_SEPARATOR, -1, -1, -1, -1, -1, -1, txt, 6, hbm, }, OK)
    TMII_GMII ( {, S, 0, -9, -9, -9, -9, -9, -9, -9, string, 80, -9, },
                {, S, 0, -9, -9, -9,  0, -9, -9, -9, string, 80, -9, },
        init, OK, OK )
    TMII_DONE
    /* some tests with small cbSize: the hbmpItem is to be ignored */ 
    TMII_INSMI( {, S - 4, MIIM_BITMAP, -1, -1, -1, -1, -1, -1, -1, -1, -1, hbm, }, OK)
    TMII_GMII ( {, S, MIIM_TYPE, -9, -9, -9, -9, -9, -9, -9, string, 80, -9, },
        {, S, MIIM_TYPE, MFT_SEPARATOR, -9, -9, 0, -9, -9, -9, NULL, 0, NULL, },
        empty, OK, ER )
    TMII_DONE
    TMII_INSMI( {, S - 4, MIIM_BITMAP, -1, -1, -1, -1, -1, -1, -1, -1, -1, hbm, }, OK)
    TMII_GMII ( {, S, MIIM_BITMAP|MIIM_FTYPE, -9, -9, -9, -9, -9, -9, -9, string, 80, -9, },
        {, S, MIIM_BITMAP|MIIM_FTYPE, MFT_SEPARATOR, -9, -9, 0, -9, -9, -9, string, 80, NULL, },
        init, OK, ER )
    TMII_DONE
    TMII_INSMI( {, S - 4, MIIM_STRING|MIIM_BITMAP, -1, -1, -1, -1, -1, -1, -1, txt, 6, hbm, }, OK)
    TMII_GMII ( {, S, MIIM_TYPE, -9, -9, -9, -9, -9, -9, -9, string, 80, -9, },
        {, S, MIIM_TYPE, MFT_STRING, -9, -9, 0, -9, -9, -9, string, 4, NULL, },
        txt, OK, OK )
    TMII_DONE
    TMII_INSMI( {, S - 4, MIIM_FTYPE|MIIM_STRING|MIIM_BITMAP, MFT_SEPARATOR, -1, -1, -1, -1, -1, -1, txt, 6, hbm, }, OK)
    TMII_GMII ( {, S, MIIM_TYPE, -9, -9, -9, -9, -9, -9, -9, string, 80, -9, },
        {, S, MIIM_TYPE, MFT_SEPARATOR, -9, -9, 0, -9, -9, -9, NULL, 4, NULL, },
        txt, OK, OK )
    TMII_DONE
    TMII_INSMI( {, S - 4, MIIM_FTYPE|MIIM_STRING|MIIM_BITMAP, MFT_OWNERDRAW, -1, -1, -1, -1, -1, -1, txt, 6, hbm, }, OK)
    TMII_GMII ( {, S, MIIM_TYPE, -9, -9, -9, -9, -9, -9, -9, string, 80, -9, },
        {, S, MIIM_TYPE, MFT_OWNERDRAW, -9, -9, 0, -9, -9, -9, NULL, 4, NULL, },
        txt, OK, OK )
    TMII_DONE
    TMII_INSMI( {, S - 4, MIIM_FTYPE|MIIM_STRING|MIIM_BITMAP, MFT_SEPARATOR|MFT_OWNERDRAW, -1, -1, -1, -1, -1, -1, txt, 6, hbm, }, OK)
    TMII_GMII ( {, S, MIIM_TYPE, -9, -9, -9, -9, -9, -9, -9, string, 80, -9, },
        {, S, MIIM_TYPE, MFT_SEPARATOR|MFT_OWNERDRAW, -9, -9, 0, -9, -9, -9, NULL, 4, NULL, },
        txt, OK, OK )
    TMII_DONE
    /* MIIM_TYPE by itself does not get/set the dwItemData for OwnerDrawn menus  */
    TMII_INSMI( {, S, MIIM_TYPE|MIIM_DATA, MFT_STRING|MFT_OWNERDRAW, -1, -1, -1, -1, -1, 343, txt, 0, -1, }, OK)
    TMII_GMII ( {, S, MIIM_TYPE|MIIM_DATA, -9, -9, -9, -9, -9, -9, -9, string, 80, -9, },
        {, S, MIIM_TYPE|MIIM_DATA, MFT_STRING|MFT_OWNERDRAW, -9, -9, 0, -9, -9, 343, 0, 0, 0, },
        empty, OK, ER )
    TMII_DONE
    TMII_INSMI( {, S, MIIM_TYPE|MIIM_DATA, MFT_STRING|MFT_OWNERDRAW, -1, -1, -1, -1, -1, 343, txt, 0, -1, }, OK)
    TMII_GMII ( {, S, MIIM_TYPE, -9, -9, -9, -9, -9, -9, -9, string, 80, -9, },
        {, S, MIIM_TYPE, MFT_STRING|MFT_OWNERDRAW, -9, -9, 0, -9, -9, -9, 0, 0, 0, },
        empty, OK, ER )
    TMII_DONE
    TMII_INSMI( {, S, MIIM_TYPE, MFT_STRING|MFT_OWNERDRAW, -1, -1, -1, -1, -1, 343, txt, 0, -1, }, OK)
    TMII_GMII ( {, S, MIIM_TYPE|MIIM_DATA, -9, -9, -9, -9, -9, -9, -9, string, 80, -9, },
        {, S, MIIM_TYPE|MIIM_DATA, MFT_STRING|MFT_OWNERDRAW, -9, -9, 0, -9, -9, 0, 0, 0, 0, },
        empty, OK, ER )
    TMII_DONE
    /* set a string menu to ownerdraw with MIIM_TYPE */
    TMII_INSMI( {, S, MIIM_TYPE, MFT_STRING, -2, -2, -2, -2, -2, -2, txt, -2, -2, }, OK)
    TMII_SMII( {, S, MIIM_TYPE, MFT_OWNERDRAW, -1, -1, -1, -1, -1, -1, -1, -1, -1, }, OK)
    TMII_GMII ( {, S, MIIM_STRING|MIIM_FTYPE, -9, -9, -9, -9, -9, -9, -9, string, 80, -9, },
        {, S, MIIM_STRING|MIIM_FTYPE, MFT_OWNERDRAW, -9, -9, 0, -9, -9, -9, string, 4, -9, },
        txt, OK, OK )
    TMII_DONE
    /* test with modifymenu add submenu */
    TMII_INSMI( {, S, MIIM_STRING, MFT_STRING, -1, -1, -1, -1, -1, -1, txt, 0, -1, }, OK)
    TMII_MODM( MF_POPUP, submenu, txt, OK)
    TMII_GMII ( {, S, MIIM_FTYPE|MIIM_STRING|MIIM_SUBMENU, -9, -9, -9, -9, -9, -9, -9, string, 80, -9, },
        {, S, MIIM_FTYPE|MIIM_STRING|MIIM_SUBMENU, MFT_STRING, -9, -9, submenu, -9, -9, -9, string, 4, -9, },
        txt,  OK, OK )
    TMII_GMII ( {, S, MIIM_TYPE, -9, -9, -9, -9, -9, -9, -9, string, 80, -9, },
        {, S, MIIM_TYPE, MFT_STRING, -9, -9, 0, -9, -9, -9, string, 4, 0, },
        txt,  OK, OK )
    TMII_DONE
    /* MFT_SEPARATOR bit is kept when the text is added */
    TMII_INSMI( {, S, MIIM_STRING|MIIM_FTYPE, MFT_STRING, -1, -1, -1, -1, -1, -1, NULL, 0, -1, }, OK)
    TMII_SMII( {, S, MIIM_STRING, -1, -1, -1, -1, -1, -1, -1, txt, -1, -1, }, OK)
    TMII_GMII ( {, S, MIIM_STRING|MIIM_FTYPE, -9, -9, -9, -9, -9, -9, -9, string, 80, -9, },
        {, S, MIIM_STRING|MIIM_FTYPE, MFT_SEPARATOR, -9, -9, 0, -9, -9, -9, string, 4, -9, },
        txt, OK, OK )
    TMII_DONE
    /* MFT_SEPARATOR bit is kept when bitmap is added */
    TMII_INSMI( {, S, MIIM_STRING|MIIM_FTYPE, MFT_STRING, -1, -1, -1, -1, -1, -1, NULL, 0, -1, }, OK)
    TMII_SMII( {, S, MIIM_BITMAP, -1, -1, -1, -1, -1, -1, -1, -1, -1, hbm, }, OK)
    TMII_GMII ( {, S, MIIM_BITMAP|MIIM_FTYPE, -9, -9, -9, -9, -9, -9, -9, string, 80, -9, },
        {, S, MIIM_BITMAP|MIIM_FTYPE, MFT_SEPARATOR, -9, -9, 0, -9, -9, -9, string, 80, hbm, },
        init, OK, ER )
    TMII_DONE
    /* Bitmaps inserted with MIIM_TYPE and MFT_BITMAP:
       Only the low word of the dwTypeData is used.
       Use a magic bitmap here (Word 95 uses this to create its MDI menu buttons) */
    TMII_INSMI( {, S, MIIM_TYPE, MFT_BITMAP | MFT_RIGHTJUSTIFY, -1, -1, -1, -1, -1, -1, MAKELONG(HBMMENU_MBAR_CLOSE, 0x1234), -1, -1, }, OK)
    TMII_GMII ( {, S, MIIM_TYPE, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, },
        {, S, MIIM_TYPE, MFT_BITMAP | MFT_RIGHTJUSTIFY, -9, -9, 0, -9, -9, -9, HBMMENU_MBAR_CLOSE, 0, HBMMENU_MBAR_CLOSE, },
        empty, OK, OK )
    TMII_DONE
    /* Type flags */
    TMII_INSMI( {, S, MIIM_TYPE, MFT_BITMAP | MFT_MENUBARBREAK | MFT_RADIOCHECK | MFT_RIGHTJUSTIFY | MFT_RIGHTORDER, -1, -1, -1, -1, -1, -1, hbm, -1, -1, }, OK)
    TMII_GMII ( {, S, MIIM_TYPE, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, },
        {, S, MIIM_TYPE, MFT_BITMAP | MFT_MENUBARBREAK | MFT_RADIOCHECK | MFT_RIGHTJUSTIFY | MFT_RIGHTORDER, -9, -9, 0, -9, -9, -9, hbm, 0, hbm, },
        empty, OK, OK )
    TMII_DONE
    /* State flags */
    TMII_INSMI( {, S, MIIM_TYPE, MFT_BITMAP, -1, -1, -1, -1, -1, -1, hbm, -1, -1, }, OK)
    TMII_SMII( {, S, MIIM_STATE, -1, MFS_CHECKED | MFS_DEFAULT | MFS_GRAYED | MFS_HILITE, -1, -1, -1, -1, -1, -1, -1, -1, }, OK)
    TMII_GMII ( {, S, MIIM_STATE, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, },
        {, S, MIIM_STATE, -9, MFS_CHECKED | MFS_DEFAULT | MFS_GRAYED | MFS_HILITE, -9, 0, -9, -9, -9, -9, -9, -9, },
        empty, OK, OK )
    TMII_DONE
    /* The style MFT_RADIOCHECK cannot be set with MIIM_CHECKMARKS only */
    TMII_INSMI( {, S, MIIM_TYPE, MFT_BITMAP, -1, -1, -1, -1, -1, -1, hbm, -1, -1, }, OK)
    TMII_SMII( {, S, MIIM_CHECKMARKS, MFT_RADIOCHECK, -1, -1, -1, hbm, hbm, -1, -1, -1, -1, }, OK)
    TMII_GMII ( {, S, MIIM_CHECKMARKS | MIIM_TYPE, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, },
        {, S, MIIM_CHECKMARKS | MIIM_TYPE, MFT_BITMAP, -9, -9, 0, hbm, hbm, -9, hbm, 0, hbm, },
        empty, OK, OK )
    TMII_DONE
    /* MFT_BITMAP is added automatically by GetMenuItemInfo() for MIIM_TYPE */
    TMII_INSMI( {, S, MIIM_TYPE, MFT_BITMAP, -1, -1, -1, -1, -1, -1, hbm, -1, -1, }, OK)
    TMII_SMII( {, S, MIIM_FTYPE, MFT_OWNERDRAW, -1, -1, -1, -1, -1, -1, 0x1234, -1, -1, }, OK)
    TMII_GMII ( {, S, MIIM_FTYPE, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, },
        {, S, MIIM_FTYPE, MFT_OWNERDRAW, -9, -9, 0, -9, -9, -9, -9, -9, -9, },
        empty, OK, OK )
    TMII_GMII ( {, S, MIIM_TYPE, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, },
        {, S, MIIM_TYPE, MFT_BITMAP | MFT_OWNERDRAW, -9, -9, 0, -9, -9, -9, hbm, 0, hbm, },
        empty, OK, OK )
    TMII_GMII ( {, S, MIIM_FTYPE, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, },
        {, S, MIIM_FTYPE, MFT_OWNERDRAW, -9, -9, 0, -9, -9, -9, -9, -9, -9, },
        empty, OK, OK )
    TMII_SMII( {, S, MIIM_BITMAP, -1, -1, -1, -1, -1, -1, -1, -1, -1, NULL, }, OK)
    TMII_GMII ( {, S, MIIM_TYPE, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, },
        {, S, MIIM_TYPE, MFT_OWNERDRAW, -9, -9, 0, -9, -9, -9, NULL, 0, NULL, },
        empty, OK, OK )
    TMII_DONE
    /* Bitmaps inserted with MIIM_TYPE and MFT_BITMAP:
       Only the low word of the dwTypeData is used.
       Use a magic bitmap here (Word 95 uses this to create its MDI menu buttons) */
    TMII_INSMI( {, S, MIIM_TYPE, MFT_BITMAP | MFT_RIGHTJUSTIFY, -1, -1, -1, -1, -1, -1, MAKELONG(HBMMENU_MBAR_CLOSE, 0x1234), -1, -1, }, OK)
    TMII_GMII ( {, S, MIIM_TYPE, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, },
        {, S, MIIM_TYPE, MFT_BITMAP | MFT_RIGHTJUSTIFY, -9, -9, 0, -9, -9, -9, HBMMENU_MBAR_CLOSE, 0, HBMMENU_MBAR_CLOSE, },
        empty, OK, OK )
    TMII_DONE
    /* Type flags */
    TMII_INSMI( {, S, MIIM_TYPE, MFT_BITMAP | MFT_MENUBARBREAK | MFT_RADIOCHECK | MFT_RIGHTJUSTIFY | MFT_RIGHTORDER, -1, -1, -1, -1, -1, -1, hbm, -1, -1, }, OK)
    TMII_GMII ( {, S, MIIM_TYPE, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, },
        {, S, MIIM_TYPE, MFT_BITMAP | MFT_MENUBARBREAK | MFT_RADIOCHECK | MFT_RIGHTJUSTIFY | MFT_RIGHTORDER, -9, -9, 0, -9, -9, -9, hbm, 0, hbm, },
        empty, OK, OK )
    TMII_DONE
    /* State flags */
    TMII_INSMI( {, S, MIIM_TYPE, MFT_BITMAP, -1, -1, -1, -1, -1, -1, hbm, -1, -1, }, OK)
    TMII_SMII( {, S, MIIM_STATE, -1, MFS_CHECKED | MFS_DEFAULT | MFS_GRAYED | MFS_HILITE, -1, -1, -1, -1, -1, -1, -1, -1, }, OK)
    TMII_GMII ( {, S, MIIM_STATE, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, },
        {, S, MIIM_STATE, -9, MFS_CHECKED | MFS_DEFAULT | MFS_GRAYED | MFS_HILITE, -9, 0, -9, -9, -9, -9, -9, -9, },
        empty, OK, OK )
    TMII_DONE
    /* The style MFT_RADIOCHECK cannot be set with MIIM_CHECKMARKS only */
    TMII_INSMI( {, S, MIIM_TYPE, MFT_BITMAP, -1, -1, -1, -1, -1, -1, hbm, -1, -1, }, OK)
    TMII_SMII( {, S, MIIM_CHECKMARKS, MFT_RADIOCHECK, -1, -1, -1, hbm, hbm, -1, -1, -1, -1, }, OK)
    TMII_GMII ( {, S, MIIM_CHECKMARKS | MIIM_TYPE, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, },
        {, S, MIIM_CHECKMARKS | MIIM_TYPE, MFT_BITMAP, -9, -9, 0, hbm, hbm, -9, hbm, 0, hbm, },
        empty, OK, OK )
    TMII_DONE
    /* MFT_BITMAP is added automatically by GetMenuItemInfo() for MIIM_TYPE */
    TMII_INSMI( {, S, MIIM_TYPE, MFT_BITMAP, -1, -1, -1, -1, -1, -1, hbm, -1, -1, }, OK)
    TMII_SMII( {, S, MIIM_FTYPE, MFT_OWNERDRAW, -1, -1, -1, -1, -1, -1, 0x1234, -1, -1, }, OK)
    TMII_GMII ( {, S, MIIM_FTYPE, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, },
        {, S, MIIM_FTYPE, MFT_OWNERDRAW, -9, -9, 0, -9, -9, -9, -9, -9, -9, },
        empty, OK, OK )
    TMII_GMII ( {, S, MIIM_TYPE, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, },
        {, S, MIIM_TYPE, MFT_BITMAP | MFT_OWNERDRAW, -9, -9, 0, -9, -9, -9, hbm, 0, hbm, },
        empty, OK, OK )
    TMII_GMII ( {, S, MIIM_FTYPE, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, },
        {, S, MIIM_FTYPE, MFT_OWNERDRAW, -9, -9, 0, -9, -9, -9, -9, -9, -9, },
        empty, OK, OK )
    TMII_SMII( {, S, MIIM_BITMAP, -1, -1, -1, -1, -1, -1, -1, -1, -1, NULL, }, OK)
    TMII_GMII ( {, S, MIIM_TYPE, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, },
        {, S, MIIM_TYPE, MFT_OWNERDRAW, -9, -9, 0, -9, -9, -9, NULL, 0, NULL, },
        empty, OK, OK )
    TMII_DONE
  } while( !(ansi = !ansi) );
  DeleteObject( hbm);
}

/* 
   The following tests try to confirm the algorithm used to return the menu items
   when there is a collision between a menu item and a popup menu
 */
static void test_menu_search_bycommand( void )
{
    HMENU        hmenu, hmenuSub, hmenuSub2;
    MENUITEMINFO info;
    BOOL         rc;
    UINT         id;
    char         strback[0x80];
    char         strIn[0x80];
    static CHAR menuitem[]  = "MenuItem",
                menuitem2[] = "MenuItem 2";

    /* Case 1: Menu containing a menu item */
    hmenu = CreateMenu();
    
    memset( &info, 0, sizeof info );
    info.cbSize = sizeof info;
    info.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_ID;
    info.fType = MFT_STRING;
    strcpy(strIn, "Case 1 MenuItem");
    info.dwTypeData = strIn;
    info.wID = (UINT) 0x1234;
    
    rc = InsertMenuItem(hmenu, 0, TRUE, &info );
    ok (rc, "Inserting the menuitem failed\n");

    id = GetMenuItemID(hmenu, 0);
    ok (id == 0x1234, "Getting the menuitem id failed(gave %x)\n", id);

    /* Confirm the menuitem was given the id supplied (getting by position) */
    memset( &info, 0, sizeof info );
    strback[0] = 0x00;
    info.cbSize = sizeof(MENUITEMINFO);
    info.fMask = MIIM_FTYPE | MIIM_ID | MIIM_STRING;
    info.dwTypeData = strback;
    info.cch = sizeof(strback);

    rc = GetMenuItemInfo(hmenu, 0, TRUE, &info); /* Get by position */
    ok (rc, "Getting the menu items info failed\n");
    ok (info.wID == 0x1234, "IDs differ for the menuitem\n");
    ok (!strcmp(info.dwTypeData, "Case 1 MenuItem"), "Returned item has wrong label\n");

    /* Search by id - Should return the item */
    memset( &info, 0, sizeof info );
    strback[0] = 0x00;
    info.cbSize = sizeof(MENUITEMINFO);
    info.fMask = MIIM_FTYPE | MIIM_ID | MIIM_STRING;
    info.dwTypeData = strback;
    info.cch = sizeof(strback);
    rc = GetMenuItemInfo(hmenu, 0x1234, FALSE, &info); /* Get by ID */

    ok (rc, "Getting the menu items info failed\n");
    ok (info.wID == 0x1234, "IDs differ for the menuitem\n");
    ok (!strcmp(info.dwTypeData, "Case 1 MenuItem"), "Returned item has wrong label\n");

    DestroyMenu( hmenu );

    /* Case 2: Menu containing a popup menu */
    hmenu = CreateMenu();
    hmenuSub = CreateMenu();
    
    strcpy(strIn, "Case 2 SubMenu");
    rc = InsertMenu(hmenu, 0, MF_BYPOSITION | MF_POPUP | MF_STRING, (UINT_PTR)hmenuSub, strIn);
    ok (rc, "Inserting the popup menu into the main menu failed\n");

    id = GetMenuItemID(hmenu, 0);
    ok (id == -1, "Getting the menuitem id unexpectedly worked (gave %x)\n", id);

    /* Confirm the menuitem itself was given an id the same as the HMENU, (getting by position) */
    memset( &info, 0, sizeof info );
    strback[0] = 0x00;
    info.cbSize = sizeof(MENUITEMINFO);
    info.fMask = MIIM_FTYPE | MIIM_ID | MIIM_STRING;
    info.dwTypeData = strback;
    info.cch = sizeof(strback);
    info.wID = 0xdeadbeef;

    rc = GetMenuItemInfo(hmenu, 0, TRUE, &info); /* Get by position */
    ok (rc, "Getting the menu items info failed\n");
    ok (info.wID == (UINT_PTR)hmenuSub, "IDs differ for the menuitem\n");
    ok (!strcmp(info.dwTypeData, "Case 2 SubMenu"), "Returned item has wrong label\n");

    /* Search by id - returns the popup menu itself */
    memset( &info, 0, sizeof info );
    strback[0] = 0x00;
    info.cbSize = sizeof(MENUITEMINFO);
    info.fMask = MIIM_FTYPE | MIIM_ID | MIIM_STRING;
    info.dwTypeData = strback;
    info.cch = sizeof(strback);
    rc = GetMenuItemInfo(hmenu, (UINT_PTR)hmenuSub, FALSE, &info); /* Get by ID */

    ok (rc, "Getting the menu items info failed\n");
    ok (info.wID == (UINT_PTR)hmenuSub, "IDs differ for the popup menu\n");
    ok (!strcmp(info.dwTypeData, "Case 2 SubMenu"), "Returned item has wrong label\n");

    /* 
        Now add an item after it with the same id
     */
    memset( &info, 0, sizeof info );
    info.cbSize = sizeof info;
    info.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_ID;
    info.fType = MFT_STRING;
    strcpy(strIn, "Case 2 MenuItem 1");
    info.dwTypeData = strIn;
    info.wID = (UINT_PTR) hmenuSub;
    rc = InsertMenuItem(hmenu, -1, TRUE, &info );
    ok (rc, "Inserting the menuitem failed\n");

    /* Search by id - returns the item which follows the popup menu */
    memset( &info, 0, sizeof info );
    strback[0] = 0x00;
    info.cbSize = sizeof(MENUITEMINFO);
    info.fMask = MIIM_FTYPE | MIIM_ID | MIIM_STRING;
    info.dwTypeData = strback;
    info.cch = sizeof(strback);
    rc = GetMenuItemInfo(hmenu, (UINT_PTR)hmenuSub, FALSE, &info); /* Get by ID */

    ok (rc, "Getting the menu items info failed\n");
    ok (info.wID == (UINT_PTR)hmenuSub, "IDs differ for the popup menu\n");
    ok (!strcmp(info.dwTypeData, "Case 2 MenuItem 1"), "Returned item has wrong label (%s)\n", info.dwTypeData);

    /* 
        Now add an item before the popup (with the same id)
     */
    memset( &info, 0, sizeof info );
    info.cbSize = sizeof info;
    info.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_ID;
    info.fType = MFT_STRING;
    strcpy(strIn, "Case 2 MenuItem 2");
    info.dwTypeData = strIn;
    info.wID = (UINT_PTR) hmenuSub;
    rc = InsertMenuItem(hmenu, 0, TRUE, &info );
    ok (rc, "Inserting the menuitem failed\n");

    /* Search by id - returns the item which precedes the popup menu */
    memset( &info, 0, sizeof info );
    strback[0] = 0x00;
    info.cbSize = sizeof(MENUITEMINFO);
    info.fMask = MIIM_FTYPE | MIIM_ID | MIIM_STRING;
    info.dwTypeData = strback;
    info.cch = sizeof(strback);
    rc = GetMenuItemInfo(hmenu, (UINT_PTR)hmenuSub, FALSE, &info); /* Get by ID */

    ok (rc, "Getting the menu items info failed\n");
    ok (info.wID == (UINT_PTR)hmenuSub, "IDs differ for the popup menu\n");
    ok (!strcmp(info.dwTypeData, "Case 2 MenuItem 2"), "Returned item has wrong label (%s)\n", info.dwTypeData);

    DestroyMenu( hmenu );
    DestroyMenu( hmenuSub );

    /* 
        Case 3: Menu containing a popup menu which in turn 
           contains 2 items with the same id as the popup itself
     */

    hmenu = CreateMenu();
    hmenuSub = CreateMenu();

    memset( &info, 0, sizeof info );
    info.cbSize = sizeof info;
    info.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_ID;
    info.fType = MFT_STRING;
    info.dwTypeData = menuitem;
    info.wID = (UINT_PTR) hmenuSub; /* Enforce id collisions with the hmenu of the popup submenu*/

    rc = InsertMenu(hmenu, 0, MF_BYPOSITION | MF_POPUP | MF_STRING, (UINT_PTR)hmenuSub, "Submenu");
    ok (rc, "Inserting the popup menu into the main menu failed\n");

    rc = InsertMenuItem(hmenuSub, 0, TRUE, &info );
    ok (rc, "Inserting the sub menu menuitem failed\n");

    memset( &info, 0, sizeof info );
    info.cbSize = sizeof info;
    info.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_ID;
    info.fType = MFT_STRING;
    info.dwTypeData = menuitem2;
    info.wID = (UINT_PTR) hmenuSub; /* Enforce id collisions with the hmenu of the popup submenu*/

    rc = InsertMenuItem(hmenuSub, 1, TRUE, &info );
    ok (rc, "Inserting the sub menu menuitem 2 failed\n");

    /* Prove that you can't query the id of a popup directly (By position) */
    id = GetMenuItemID(hmenu, 0);
    ok (id == -1, "Getting the sub menu id should have failed because its a popup (gave %x)\n", id);

    /* Prove getting the item info via ID returns the first item (not the popup or 2nd item)*/
    memset( &info, 0, sizeof info );
    strback[0] = 0x00;
    info.cbSize = sizeof(MENUITEMINFO);
    info.fMask = MIIM_STRING | MIIM_ID;
    info.dwTypeData = strback;
    info.cch = sizeof(strback);

    rc = GetMenuItemInfo(hmenu, (UINT_PTR)hmenuSub, FALSE, &info);
    ok (rc, "Getting the menus info failed\n");
    ok (info.wID == (UINT_PTR)hmenuSub, "IDs differ for popup menu\n");
    ok (!strcmp(info.dwTypeData, "MenuItem"), "Returned item has wrong label (%s)\n", info.dwTypeData);
    DestroyMenu( hmenu );
    DestroyMenu( hmenuSub );

    /* 
        Case 4: Menu containing 2 popup menus, the second
           contains 2 items with the same id as the first popup menu
     */
    hmenu = CreateMenu();
    hmenuSub = CreateMenu();
    hmenuSub2 = CreateMenu();
    
    rc = InsertMenu(hmenu, 0, MF_BYPOSITION | MF_POPUP | MF_STRING, (UINT_PTR)hmenuSub, "Submenu");
    ok (rc, "Inserting the popup menu into the main menu failed\n");
    
    rc = InsertMenu(hmenu, 1, MF_BYPOSITION | MF_POPUP | MF_STRING, (UINT_PTR)hmenuSub2, "Submenu2");
    ok (rc, "Inserting the popup menu into the main menu failed\n");

    memset( &info, 0, sizeof info );
    info.cbSize = sizeof info;
    info.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_ID;
    info.fType = MFT_STRING;
    info.dwTypeData = menuitem;
    info.wID = (UINT_PTR) hmenuSub; /* Enforce id collisions with the hmenu of the popup submenu*/

    rc = InsertMenuItem(hmenuSub2, 0, TRUE, &info );
    ok (rc, "Inserting the sub menu menuitem failed\n");

    memset( &info, 0, sizeof info );
    info.cbSize = sizeof info;
    info.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_ID;
    info.fType = MFT_STRING;
    info.dwTypeData = menuitem2;
    info.wID = (UINT_PTR) hmenuSub; /* Enforce id collisions with the hmenu of the popup submenu*/

    rc = InsertMenuItem(hmenuSub2, 1, TRUE, &info );
    ok (rc, "Inserting the sub menu menuitem 2 failed\n");

    /* Prove getting the item info via ID returns the first item (not the popup or 2nd item)*/
    memset( &info, 0, sizeof info );
    strback[0] = 0x00;
    info.cbSize = sizeof(MENUITEMINFO);
    info.fMask = MIIM_STRING | MIIM_ID;
    info.dwTypeData = strback;
    info.cch = sizeof(strback);

    rc = GetMenuItemInfo(hmenu, (UINT_PTR)hmenuSub, FALSE, &info);
    ok (rc, "Getting the menus info failed\n");
    ok (info.wID == (UINT_PTR)hmenuSub, "IDs differ for popup menu\n");
    ok (!strcmp(info.dwTypeData, "MenuItem"), "Returned item has wrong label (%s)\n", info.dwTypeData);

    memset( &info, 0, sizeof info );
    strback[0] = 0x00;
    info.cbSize = sizeof(MENUITEMINFO);
    info.fMask = MIIM_STRING | MIIM_ID;
    info.dwTypeData = strback;
    info.cch = sizeof(strback);

    rc = GetMenuItemInfo(hmenu, (UINT_PTR)hmenuSub2, FALSE, &info);
    ok (rc, "Getting the menus info failed\n");
    ok (info.wID == (UINT)hmenuSub2, "IDs differ for popup menu\n");
    ok (!strcmp(info.dwTypeData, "Submenu2"), "Returned item has wrong label (%s)\n", info.dwTypeData);

    DestroyMenu( hmenu );
    DestroyMenu( hmenuSub );
    DestroyMenu( hmenuSub2 );


    /* 
        Case 5: Menu containing a popup menu which in turn
           contains an item with a different id than the popup menu.
           This tests the fallback to a popup menu ID.
     */

    hmenu = CreateMenu();
    hmenuSub = CreateMenu();

    rc = AppendMenu(hmenu, MF_POPUP | MF_STRING, (UINT_PTR)hmenuSub, "Submenu");
    ok (rc, "Appending the popup menu to the main menu failed\n");

    rc = AppendMenu(hmenuSub, MF_STRING, 102, "Item");
    ok (rc, "Appending the item to the popup menu failed\n");

    /* Set the ID for hmenuSub */
    info.cbSize = sizeof(info);
    info.fMask = MIIM_ID;
    info.wID = 101;

    rc = SetMenuItemInfo(hmenu, 0, TRUE, &info);
    ok(rc, "Setting the ID for the popup menu failed\n");

    /* Check if the ID has been set */
    info.wID = 0;
    rc = GetMenuItemInfo(hmenu, 0, TRUE, &info);
    ok(rc, "Getting the ID for the popup menu failed\n");
    ok(info.wID == 101, "The ID for the popup menu has not been set\n");

    /* Prove getting the item info via ID returns the popup menu */
    memset( &info, 0, sizeof(info));
    strback[0] = 0x00;
    info.cbSize = sizeof(MENUITEMINFO);
    info.fMask = MIIM_STRING | MIIM_ID;
    info.dwTypeData = strback;
    info.cch = sizeof(strback);

    rc = GetMenuItemInfo(hmenu, 101, FALSE, &info);
    ok (rc, "Getting the menu info failed\n");
    ok (info.wID == 101, "IDs differ\n");
    ok (!strcmp(info.dwTypeData, "Submenu"), "Returned item has wrong label (%s)\n", info.dwTypeData);

    /* Also look for the menu item  */
    memset( &info, 0, sizeof(info));
    strback[0] = 0x00;
    info.cbSize = sizeof(MENUITEMINFO);
    info.fMask = MIIM_STRING | MIIM_ID;
    info.dwTypeData = strback;
    info.cch = sizeof(strback);

    rc = GetMenuItemInfo(hmenu, 102, FALSE, &info);
    ok (rc, "Getting the menu info failed\n");
    ok (info.wID == 102, "IDs differ\n");
    ok (!strcmp(info.dwTypeData, "Item"), "Returned item has wrong label (%s)\n", info.dwTypeData);

    DestroyMenu(hmenu);
    DestroyMenu(hmenuSub);
}

struct menu_item_pair_s {
    UINT uMenu; /* 1 - top level menu, [0-Menu 1-Enabled 2-Disabled]
                 * 2 - 2nd level menu, [0-Popup 1-Enabled 2-Disabled]
                 * 3 - 3rd level menu, [0-Enabled 1-Disabled] */
    UINT uItem;
};

static struct menu_mouse_tests_s {
    DWORD type;
    struct menu_item_pair_s menu_item_pairs[5]; /* for mousing */
    WORD wVk[5]; /* keys */
    BOOL bMenuVisible;
    BOOL _todo_wine;
} menu_tests[] = {
    /* for each test, send keys or clicks and check for menu visibility */
    { INPUT_KEYBOARD, {{0}}, {VK_LMENU, 0}, TRUE, FALSE }, /* test 0 */
    { INPUT_KEYBOARD, {{0}}, {VK_ESCAPE, 0}, FALSE, FALSE },
    { INPUT_KEYBOARD, {{0}}, {VK_LMENU, 0}, TRUE, FALSE },
    { INPUT_KEYBOARD, {{0}}, {'D', 0}, FALSE, FALSE },
    { INPUT_KEYBOARD, {{0}}, {VK_LMENU, 0}, TRUE, FALSE },
    { INPUT_KEYBOARD, {{0}}, {'E', 0}, FALSE, FALSE },
    { INPUT_KEYBOARD, {{0}}, {VK_LMENU, 'M', 0}, TRUE, FALSE },
    { INPUT_KEYBOARD, {{0}}, {VK_ESCAPE, VK_ESCAPE, 0}, FALSE, FALSE },
    { INPUT_KEYBOARD, {{0}}, {VK_LMENU, 'M', VK_ESCAPE, 0}, TRUE, FALSE },
    { INPUT_KEYBOARD, {{0}}, {VK_ESCAPE, 0}, FALSE, FALSE },
    { INPUT_KEYBOARD, {{0}}, {VK_LMENU, 'M', 0}, TRUE, FALSE },
    { INPUT_KEYBOARD, {{0}}, {'D', 0}, FALSE, FALSE },
    { INPUT_KEYBOARD, {{0}}, {VK_LMENU, 'M', 0}, TRUE, FALSE },
    { INPUT_KEYBOARD, {{0}}, {'E', 0}, FALSE, FALSE },
    { INPUT_KEYBOARD, {{0}}, {VK_LMENU, 'M', 'P', 0}, TRUE, FALSE },
    { INPUT_KEYBOARD, {{0}}, {'D', 0}, FALSE, FALSE },
    { INPUT_KEYBOARD, {{0}}, {VK_LMENU, 'M', 'P', 0}, TRUE, FALSE },
    { INPUT_KEYBOARD, {{0}}, {'E', 0}, FALSE, FALSE },

    { INPUT_MOUSE, {{1, 2}, {0}}, {0}, TRUE, TRUE }, /* test 18 */
    { INPUT_MOUSE, {{1, 1}, {0}}, {0}, FALSE, FALSE },
    { INPUT_MOUSE, {{1, 0}, {0}}, {0}, TRUE, TRUE },
    { INPUT_MOUSE, {{1, 1}, {0}}, {0}, FALSE, FALSE },
    { INPUT_MOUSE, {{1, 0}, {2, 2}, {0}}, {0}, TRUE, TRUE },
    { INPUT_MOUSE, {{2, 1}, {0}}, {0}, FALSE, FALSE },
    { INPUT_MOUSE, {{1, 0}, {2, 0}, {0}}, {0}, TRUE, TRUE },
    { INPUT_MOUSE, {{3, 0}, {0}}, {0}, FALSE, FALSE },
    { INPUT_MOUSE, {{1, 0}, {2, 0}, {0}}, {0}, TRUE, TRUE },
    { INPUT_MOUSE, {{3, 1}, {0}}, {0}, TRUE, TRUE },
    { INPUT_MOUSE, {{1, 1}, {0}}, {0}, FALSE, FALSE },
    { -1 }
};

static void send_key(WORD wVk)
{
    TEST_INPUT i[2];
    memset(i, 0, sizeof(i));
    i[0].type = i[1].type = INPUT_KEYBOARD;
    i[0].u.ki.wVk = i[1].u.ki.wVk = wVk;
    i[1].u.ki.dwFlags = KEYEVENTF_KEYUP;
    pSendInput(2, (INPUT *) i, sizeof(INPUT));
}

static void click_menu(HANDLE hWnd, struct menu_item_pair_s *mi)
{
    HMENU hMenu = hMenus[mi->uMenu];
    TEST_INPUT i[3];
    MSG msg;
    RECT r;
    int screen_w = GetSystemMetrics(SM_CXSCREEN);
    int screen_h = GetSystemMetrics(SM_CYSCREEN);
    BOOL ret = GetMenuItemRect(mi->uMenu > 2 ? NULL : hWnd, hMenu, mi->uItem, &r);
    if(!ret) return;

    memset(i, 0, sizeof(i));
    i[0].type = i[1].type = i[2].type = INPUT_MOUSE;
    i[0].u.mi.dx = i[1].u.mi.dx = i[2].u.mi.dx
            = ((r.left + 5) * 65535) / screen_w;
    i[0].u.mi.dy = i[1].u.mi.dy = i[2].u.mi.dy
            = ((r.top + 5) * 65535) / screen_h;
    i[0].u.mi.dwFlags = i[1].u.mi.dwFlags = i[2].u.mi.dwFlags
            = MOUSEEVENTF_ABSOLUTE;
    i[0].u.mi.dwFlags |= MOUSEEVENTF_MOVE;
    i[1].u.mi.dwFlags |= MOUSEEVENTF_LEFTDOWN;
    i[2].u.mi.dwFlags |= MOUSEEVENTF_LEFTUP;
    pSendInput(3, (INPUT *) i, sizeof(INPUT));

    /* hack to prevent mouse message buildup in Wine */
    while (PeekMessage( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageA( &msg );
}

static DWORD WINAPI test_menu_input_thread(LPVOID lpParameter)
{
    int i, j;
    HANDLE hWnd = lpParameter;

    Sleep(500);
    /* mixed keyboard/mouse test */
    for (i = 0; menu_tests[i].type != -1; i++)
    {
        int elapsed = 0;

        if (menu_tests[i].type == INPUT_KEYBOARD)
            for (j = 0; menu_tests[i].wVk[j] != 0; j++)
                send_key(menu_tests[i].wVk[j]);
        else
            for (j = 0; menu_tests[i].menu_item_pairs[j].uMenu != 0; j++)
                click_menu(hWnd, &menu_tests[i].menu_item_pairs[j]);

        while (menu_tests[i].bMenuVisible != bMenuVisible)
        {
            if (elapsed > 200)
                break;
            elapsed += 20;
            Sleep(20);
        }

        if (menu_tests[i]._todo_wine)
        {
            todo_wine {
                ok(menu_tests[i].bMenuVisible == bMenuVisible, "test %d\n", i);
            }
        }
        else
            ok(menu_tests[i].bMenuVisible == bMenuVisible, "test %d\n", i);
    }
    return 0;
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam,
        LPARAM lParam)
{
    switch (msg) {
        case WM_ENTERMENULOOP:
            bMenuVisible = TRUE;
            break;
        case WM_EXITMENULOOP:
            bMenuVisible = FALSE;
            break;
        default:
            return( DefWindowProcA( hWnd, msg, wParam, lParam ) );
    }
    return 0;
}

static void test_menu_input(void) {
    MSG msg;
    WNDCLASSA  wclass;
    HINSTANCE hInstance = GetModuleHandleA( NULL );
    HANDLE hThread, hWnd;
    DWORD tid;

    wclass.lpszClassName = "MenuTestClass";
    wclass.style         = CS_HREDRAW | CS_VREDRAW;
    wclass.lpfnWndProc   = WndProc;
    wclass.hInstance     = hInstance;
    wclass.hIcon         = LoadIconA( 0, (LPSTR)IDI_APPLICATION );
    wclass.hCursor       = LoadCursorA( NULL, (LPSTR)IDC_ARROW);
    wclass.hbrBackground = (HBRUSH)( COLOR_WINDOW + 1);
    wclass.lpszMenuName  = 0;
    wclass.cbClsExtra    = 0;
    wclass.cbWndExtra    = 0;
    assert (RegisterClassA( &wclass ));
    assert (hWnd = CreateWindowA( wclass.lpszClassName, "MenuTest",
                                  WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0,
                                  400, 200, NULL, NULL, hInstance, NULL) );

    /* fixed menus */
    hMenus[3] = CreatePopupMenu();
    AppendMenu(hMenus[3], MF_STRING, 0, "&Enabled");
    AppendMenu(hMenus[3], MF_STRING|MF_DISABLED, 0, "&Disabled");

    hMenus[2] = CreatePopupMenu();
    AppendMenu(hMenus[2], MF_STRING|MF_POPUP, (UINT_PTR) hMenus[3], "&Popup");
    AppendMenu(hMenus[2], MF_STRING, 0, "&Enabled");
    AppendMenu(hMenus[2], MF_STRING|MF_DISABLED, 0, "&Disabled");

    hMenus[1] = CreateMenu();
    AppendMenu(hMenus[1], MF_STRING|MF_POPUP, (UINT_PTR) hMenus[2], "&Menu");
    AppendMenu(hMenus[1], MF_STRING, 0, "&Enabled");
    AppendMenu(hMenus[1], MF_STRING|MF_DISABLED, 0, "&Disabled");

    SetMenu(hWnd, hMenus[1]);
    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);

    hThread = CreateThread(NULL, 0, test_menu_input_thread, hWnd, 0, &tid);
    while(1)
    {
        if (WAIT_TIMEOUT != WaitForSingleObject(hThread, 50))
            break;
        while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
    }
    DestroyWindow(hWnd);
}

static void test_menu_flags( void )
{
    HMENU hMenu, hPopupMenu;

    hMenu = CreateMenu();
    hPopupMenu = CreatePopupMenu();

    AppendMenu(hMenu, MF_POPUP | MF_STRING, (UINT)hPopupMenu, "Popup");

    AppendMenu(hPopupMenu, MF_STRING | MF_HILITE | MF_DEFAULT, 101, "Item 1");
    InsertMenu(hPopupMenu, 1, MF_BYPOSITION | MF_STRING | MF_HILITE | MF_DEFAULT, 102, "Item 2");
    AppendMenu(hPopupMenu, MF_STRING, 103, "Item 3");
    ModifyMenu(hPopupMenu, 2, MF_BYPOSITION | MF_STRING | MF_HILITE | MF_DEFAULT, 103, "Item 3");

    ok(GetMenuState(hPopupMenu, 0, MF_BYPOSITION) & MF_HILITE,
      "AppendMenu should accept MF_HILITE\n");
    ok(GetMenuState(hPopupMenu, 1, MF_BYPOSITION) & MF_HILITE,
      "InsertMenu should accept MF_HILITE\n");
    ok(GetMenuState(hPopupMenu, 2, MF_BYPOSITION) & MF_HILITE,
      "ModifyMenu should accept MF_HILITE\n");

    ok(!(GetMenuState(hPopupMenu, 0, MF_BYPOSITION) & MF_DEFAULT),
      "AppendMenu must not accept MF_DEFAULT\n");
    ok(!(GetMenuState(hPopupMenu, 1, MF_BYPOSITION) & MF_DEFAULT),
      "InsertMenu must not accept MF_DEFAULT\n");
    ok(!(GetMenuState(hPopupMenu, 2, MF_BYPOSITION) & MF_DEFAULT),
      "ModifyMenu must not accept MF_DEFAULT\n");

    DestroyMenu(hMenu);
}

static void test_menu_hilitemenuitem( void )
{
    HMENU hMenu, hPopupMenu;
    WNDCLASSA wclass;
    HWND hWnd;

    wclass.lpszClassName = "HiliteMenuTestClass";
    wclass.style         = CS_HREDRAW | CS_VREDRAW;
    wclass.lpfnWndProc   = WndProc;
    wclass.hInstance     = GetModuleHandleA( NULL );
    wclass.hIcon         = LoadIconA( 0, (LPSTR)IDI_APPLICATION );
    wclass.hCursor       = LoadCursorA( NULL, (LPSTR)IDC_ARROW);
    wclass.hbrBackground = (HBRUSH)( COLOR_WINDOW + 1);
    wclass.lpszMenuName  = 0;
    wclass.cbClsExtra    = 0;
    wclass.cbWndExtra    = 0;
    assert (RegisterClassA( &wclass ));
    assert (hWnd = CreateWindowA( wclass.lpszClassName, "HiliteMenuTest",
                                  WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0,
                                  400, 200, NULL, NULL, wclass.hInstance, NULL) );

    hMenu = CreateMenu();
    hPopupMenu = CreatePopupMenu();

    AppendMenu(hMenu, MF_POPUP | MF_STRING, (UINT)hPopupMenu, "Popup");

    AppendMenu(hPopupMenu, MF_STRING, 101, "Item 1");
    AppendMenu(hPopupMenu, MF_STRING, 102, "Item 2");
    AppendMenu(hPopupMenu, MF_STRING, 103, "Item 3");

    SetMenu(hWnd, hMenu);

    /* test invalid arguments */

    ok(!(GetMenuState(hPopupMenu, 1, MF_BYPOSITION) & MF_HILITE),
      "HiliteMenuItem: Item 2 is hilited\n");

    SetLastError(0xdeadbeef);
    todo_wine
    {
    ok(!HiliteMenuItem(NULL, hPopupMenu, 1, MF_HILITE | MF_BYPOSITION),
      "HiliteMenuItem: call should have failed.\n");
    }
    ok(GetLastError() == 0xdeadbeef || /* 9x */
       GetLastError() == ERROR_INVALID_WINDOW_HANDLE /* NT */,
      "HiliteMenuItem: expected error ERROR_INVALID_WINDOW_HANDLE, got: %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ok(!HiliteMenuItem(hWnd, NULL, 1, MF_HILITE | MF_BYPOSITION),
      "HiliteMenuItem: call should have failed.\n");
    ok(GetLastError() == 0xdeadbeef || /* 9x */
       GetLastError() == ERROR_INVALID_MENU_HANDLE /* NT */,
      "HiliteMenuItem: expected error ERROR_INVALID_MENU_HANDLE, got: %d\n", GetLastError());

    ok(!(GetMenuState(hPopupMenu, 1, MF_BYPOSITION) & MF_HILITE),
      "HiliteMenuItem: Item 2 is hilited\n");

    /* either MF_HILITE or MF_UNHILITE *and* MF_BYCOMMAND or MF_BYPOSITION need to be set */

    SetLastError(0xdeadbeef);
    ok(HiliteMenuItem(hWnd, hPopupMenu, 1, MF_BYPOSITION),
      "HiliteMenuItem: call should have succeeded.\n");
    ok(GetLastError() == 0xdeadbeef,
      "HiliteMenuItem: expected error 0xdeadbeef, got: %d\n", GetLastError());

    ok(!(GetMenuState(hPopupMenu, 1, MF_BYPOSITION) & MF_HILITE),
      "HiliteMenuItem: Item 2 is hilited\n");

    SetLastError(0xdeadbeef);
    todo_wine
    {
    ok(HiliteMenuItem(hWnd, hPopupMenu, 1, MF_HILITE),
      "HiliteMenuItem: call should have succeeded.\n");
    }
    ok(GetLastError() == 0xdeadbeef,
      "HiliteMenuItem: expected error 0xdeadbeef, got: %d\n", GetLastError());

    ok(!(GetMenuState(hPopupMenu, 1, MF_BYPOSITION) & MF_HILITE),
      "HiliteMenuItem: Item 2 is hilited\n");

    /* hilite a menu item (by position) */

    SetLastError(0xdeadbeef);
    ok(HiliteMenuItem(hWnd, hPopupMenu, 1, MF_HILITE | MF_BYPOSITION),
      "HiliteMenuItem: call should not have failed.\n");
    ok(GetLastError() == 0xdeadbeef,
      "HiliteMenuItem: expected error 0xdeadbeef, got: %d\n", GetLastError());

    todo_wine
    {
    ok(GetMenuState(hPopupMenu, 1, MF_BYPOSITION) & MF_HILITE,
      "HiliteMenuItem: Item 2 is not hilited\n");
    }

    /* unhilite a menu item (by position) */

    SetLastError(0xdeadbeef);
    ok(HiliteMenuItem(hWnd, hPopupMenu, 1, MF_UNHILITE | MF_BYPOSITION),
      "HiliteMenuItem: call should not have failed.\n");
    ok(GetLastError() == 0xdeadbeef,
      "HiliteMenuItem: expected error 0xdeadbeef, got: %d\n", GetLastError());

    ok(!(GetMenuState(hPopupMenu, 1, MF_BYPOSITION) & MF_HILITE),
      "HiliteMenuItem: Item 2 is hilited\n");

    /* hilite a menu item (by command) */

    SetLastError(0xdeadbeef);
    ok(HiliteMenuItem(hWnd, hPopupMenu, 103, MF_HILITE | MF_BYCOMMAND),
      "HiliteMenuItem: call should not have failed.\n");
    ok(GetLastError() == 0xdeadbeef,
      "HiliteMenuItem: expected error 0xdeadbeef, got: %d\n", GetLastError());

    todo_wine
    {
    ok(GetMenuState(hPopupMenu, 2, MF_BYPOSITION) & MF_HILITE,
      "HiliteMenuItem: Item 3 is not hilited\n");
    }

    /* unhilite a menu item (by command) */

    SetLastError(0xdeadbeef);
    ok(HiliteMenuItem(hWnd, hPopupMenu, 103, MF_UNHILITE | MF_BYCOMMAND),
      "HiliteMenuItem: call should not have failed.\n");
    ok(GetLastError() == 0xdeadbeef,
      "HiliteMenuItem: expected error 0xdeadbeef, got: %d\n", GetLastError());

    ok(!(GetMenuState(hPopupMenu, 2, MF_BYPOSITION) & MF_HILITE),
      "HiliteMenuItem: Item 3 is hilited\n");

    DestroyWindow(hWnd);
}

static void check_menu_items(HMENU hmenu, UINT checked_cmd, UINT checked_type,
                             UINT checked_state)
{
    INT i, count;

    count = GetMenuItemCount(hmenu);
    ok (count != -1, "GetMenuItemCount returned -1\n");

    for (i = 0; i < count; i++)
    {
        BOOL ret;
        MENUITEMINFO mii;

        memset(&mii, 0, sizeof(mii));
        mii.cbSize = sizeof(mii);
        mii.fMask  = MIIM_FTYPE | MIIM_STATE | MIIM_ID | MIIM_SUBMENU;
        ret = GetMenuItemInfo(hmenu, i, TRUE, &mii);
        ok(ret, "GetMenuItemInfo(%u) failed\n", i);
#if 0
        trace("item #%u: fType %04x, fState %04x, wID %u, hSubMenu %p\n",
               i, mii.fType, mii.fState, mii.wID, mii.hSubMenu);
#endif
        if (mii.hSubMenu)
        {
            ok((HMENU)mii.wID == mii.hSubMenu, "id %u: wID should be equal to hSubMenu\n", checked_cmd);
            check_menu_items(mii.hSubMenu, checked_cmd, checked_type, checked_state);
        }
        else
        {
            if (mii.wID == checked_cmd)
            {
                ok(mii.fType == checked_type, "id %u: expected fType %04x, got %04x\n", checked_cmd, checked_type, mii.fType);
                ok(mii.fState == checked_state, "id %u: expected fState %04x, got %04x\n", checked_cmd, checked_state, mii.fState);
                ok(mii.wID != 0, "id %u: not expected wID 0\n", checked_cmd);
            }
            else
            {
                ok(mii.fType != MFT_RADIOCHECK, "id %u: not expected fType MFT_RADIOCHECK on cmd %u\n", checked_cmd, mii.wID);

                if (mii.fType == MFT_SEPARATOR)
                {
                    ok(mii.fState == MFS_GRAYED, "id %u: expected fState MFS_GRAYED, got %04x\n", checked_cmd, mii.fState);
                    ok(mii.wID == 0, "id %u: expected wID 0, got %u\n", checked_cmd, mii.wID);
                }
                else
                {
                    ok(mii.fState == 0, "id %u: expected fState 0, got %04x\n", checked_cmd, mii.fState);
                    ok(mii.wID != 0, "id %u: not expected wID 0\n", checked_cmd);
                }
            }
        }
    }
}

static void clear_ftype_and_state(HMENU hmenu, UINT id, UINT flags)
{
    BOOL ret;
    MENUITEMINFO mii;

    memset(&mii, 0, sizeof(mii));
    mii.cbSize = sizeof(mii);
    mii.fMask  = MIIM_FTYPE | MIIM_STATE;
    ret = SetMenuItemInfo(hmenu, id, (flags & MF_BYPOSITION) != 0, &mii);
    ok(ret, "SetMenuItemInfo(%u) failed\n", id);
}

static void test_CheckMenuRadioItem(void)
{
    BOOL ret;
    HMENU hmenu;

    hmenu = LoadMenu(GetModuleHandle(0), MAKEINTRESOURCE(1));
    assert(hmenu != 0);

    check_menu_items(hmenu, -1, 0, 0);

    ret = CheckMenuRadioItem(hmenu, 100, 100, 100, MF_BYCOMMAND);
    ok(ret, "CheckMenuRadioItem failed\n");
    check_menu_items(hmenu, 100, MFT_RADIOCHECK, MFS_CHECKED);

    /* MSDN is wrong, Windows does not remove MFT_RADIOCHECK */
    ret = CheckMenuRadioItem(hmenu, 100, 100, -1, MF_BYCOMMAND);
    ok(!ret, "CheckMenuRadioItem should return FALSE\n");
    check_menu_items(hmenu, 100, MFT_RADIOCHECK, 0);

    /* clear check */
    clear_ftype_and_state(hmenu, 100, MF_BYCOMMAND);
    check_menu_items(hmenu, -1, 0, 0);

    /* first and checked items are on different menus */
    ret = CheckMenuRadioItem(hmenu, 0, 300, 202, MF_BYCOMMAND);
    ok(!ret, "CheckMenuRadioItem should return FALSE\n");
    check_menu_items(hmenu, -1, 0, 0);

    ret = CheckMenuRadioItem(hmenu, 200, 300, 202, MF_BYCOMMAND);
    ok(ret, "CheckMenuRadioItem failed\n");
    check_menu_items(hmenu, 202, MFT_RADIOCHECK, MFS_CHECKED);

    /* MSDN is wrong, Windows does not remove MFT_RADIOCHECK */
    ret = CheckMenuRadioItem(hmenu, 202, 202, -1, MF_BYCOMMAND);
    ok(!ret, "CheckMenuRadioItem should return FALSE\n");
    check_menu_items(hmenu, 202, MFT_RADIOCHECK, 0);

    /* clear check */
    clear_ftype_and_state(hmenu, 202, MF_BYCOMMAND);
    check_menu_items(hmenu, -1, 0, 0);

    /* just for fun, try to check separator */
    ret = CheckMenuRadioItem(hmenu, 0, 300, 0, MF_BYCOMMAND);
    ok(!ret, "CheckMenuRadioItem should return FALSE\n");
    check_menu_items(hmenu, -1, 0, 0);
}

static void test_menu_resource_layout(void)
{
    static const struct
    {
        MENUITEMTEMPLATEHEADER mith;
        WORD data[14];
    } menu_template =
    {
        { 0, 0 }, /* versionNumber, offset */
        {
            /* mtOption, mtID, mtString[] '\0' terminated */
            MF_STRING, 1, 'F', 0,
            MF_STRING, 2, 0,
            MF_SEPARATOR, 3, 0,
            /* MF_SEPARATOR, 4, 'S', 0, FIXME: Wine ignores 'S' */
            MF_STRING|MF_GRAYED|MF_END, 5, 'E', 0
        }
    };
    static const struct
    {
        UINT type, state, id;
        const char *str;
    } menu_data[] =
    {
        { MF_STRING, MF_ENABLED, 1, "F" },
        { MF_SEPARATOR, MF_GRAYED|MF_DISABLED, 2, "" },
        { MF_SEPARATOR, MF_GRAYED|MF_DISABLED, 3, "" },
        /*{ MF_SEPARATOR, MF_GRAYED|MF_DISABLED, 4, "S" }, FIXME: Wine ignores 'S'*/
        { MF_STRING, MF_GRAYED, 5, "E" },
        { MF_SEPARATOR, MF_GRAYED|MF_DISABLED, 6, "" },
        { MF_STRING, MF_ENABLED, 7, "" },
        { MF_SEPARATOR, MF_GRAYED|MF_DISABLED, 8, "" }
    };
    HMENU hmenu;
    INT count, i;
    BOOL ret;

    hmenu = LoadMenuIndirect(&menu_template);
    ok(hmenu != 0, "LoadMenuIndirect error %u\n", GetLastError());

    ret = AppendMenu(hmenu, MF_STRING, 6, NULL);
    ok(ret, "AppendMenu failed\n");
    ret = AppendMenu(hmenu, MF_STRING, 7, "\0");
    ok(ret, "AppendMenu failed\n");
    ret = AppendMenu(hmenu, MF_SEPARATOR, 8, "separator");
    ok(ret, "AppendMenu failed\n");

    count = GetMenuItemCount(hmenu);
    ok(count == sizeof(menu_data)/sizeof(menu_data[0]),
       "expected %u menu items, got %u\n",
       (UINT)(sizeof(menu_data)/sizeof(menu_data[0])), count);

    for (i = 0; i < count; i++)
    {
        char buf[20];
        MENUITEMINFO mii;

        memset(&mii, 0, sizeof(mii));
        mii.cbSize = sizeof(mii);
        mii.dwTypeData = buf;
        mii.cch = sizeof(buf);
        mii.fMask  = MIIM_FTYPE | MIIM_STATE | MIIM_ID | MIIM_STRING;
        ret = GetMenuItemInfo(hmenu, i, TRUE, &mii);
        ok(ret, "GetMenuItemInfo(%u) failed\n", i);
#if 0
        trace("item #%u: fType %04x, fState %04x, wID %u, dwTypeData %s\n",
               i, mii.fType, mii.fState, mii.wID, (LPCSTR)mii.dwTypeData);
#endif
        ok(mii.fType == menu_data[i].type,
           "%u: expected fType %04x, got %04x\n", i, menu_data[i].type, mii.fType);
        ok(mii.fState == menu_data[i].state,
           "%u: expected fState %04x, got %04x\n", i, menu_data[i].state, mii.fState);
        ok(mii.wID == menu_data[i].id,
           "%u: expected wID %04x, got %04x\n", i, menu_data[i].id, mii.wID);
        ok(mii.cch == strlen(menu_data[i].str),
           "%u: expected cch %u, got %u\n", i, (UINT)strlen(menu_data[i].str), mii.cch);
        ok(!strcmp((LPCSTR)mii.dwTypeData, menu_data[i].str),
           "%u: expected dwTypeData %s, got %s\n", i, menu_data[i].str, (LPCSTR)mii.dwTypeData);
    }

    DestroyMenu(hmenu);
}

struct menu_data
{
    UINT type, id;
    const char *str;
};

static HMENU create_menu_from_data(const struct menu_data *item, INT item_count)
{
    HMENU hmenu;
    INT i;
    BOOL ret;

    hmenu = CreateMenu();
    assert(hmenu != 0);

    for (i = 0; i < item_count; i++)
    {
        SetLastError(0xdeadbeef);
        ret = AppendMenu(hmenu, item[i].type, item[i].id, item[i].str);
        ok(ret, "%d: AppendMenu(%04x, %04x, %p) error %u\n",
           i, item[i].type, item[i].id, item[i].str, GetLastError());
    }
    return hmenu;
}

static void compare_menu_data(HMENU hmenu, const struct menu_data *item, INT item_count)
{
    INT count, i;
    BOOL ret;

    count = GetMenuItemCount(hmenu);
    ok(count == item_count, "expected %d, got %d menu items\n", count, item_count);

    for (i = 0; i < count; i++)
    {
        char buf[20];
        MENUITEMINFO mii;

        memset(&mii, 0, sizeof(mii));
        mii.cbSize = sizeof(mii);
        mii.dwTypeData = buf;
        mii.cch = sizeof(buf);
        mii.fMask  = MIIM_FTYPE | MIIM_ID | MIIM_STRING | MIIM_BITMAP;
        ret = GetMenuItemInfo(hmenu, i, TRUE, &mii);
        ok(ret, "GetMenuItemInfo(%u) failed\n", i);
#if 0
        trace("item #%u: fType %04x, fState %04x, wID %04x, hbmp %p\n",
               i, mii.fType, mii.fState, mii.wID, mii.hbmpItem);
#endif
        ok(mii.fType == item[i].type,
           "%u: expected fType %04x, got %04x\n", i, item[i].type, mii.fType);
        ok(mii.wID == item[i].id,
           "%u: expected wID %04x, got %04x\n", i, item[i].id, mii.wID);
        if (item[i].type & (MF_BITMAP | MF_SEPARATOR))
        {
            /* For some reason Windows sets high word to not 0 for
             * not "magic" ids.
             */
            ok(LOWORD(mii.hbmpItem) == LOWORD(item[i].str),
               "%u: expected hbmpItem %p, got %p\n", i, item[i].str, mii.hbmpItem);
        }
        else
        {
            ok(mii.cch == strlen(item[i].str),
               "%u: expected cch %u, got %u\n", i, (UINT)strlen(item[i].str), mii.cch);
            ok(!strcmp((LPCSTR)mii.dwTypeData, item[i].str),
               "%u: expected dwTypeData %s, got %s\n", i, item[i].str, (LPCSTR)mii.dwTypeData);
        }
    }
}

static void test_InsertMenu(void)
{
    /* Note: XP treats only bitmap handles 1 - 6 as "magic" ones
     * regardless of their id.
     */
    static const struct menu_data in1[] =
    {
        { MF_STRING, 1, "File" },
        { MF_BITMAP|MF_HELP, SC_CLOSE, MAKEINTRESOURCE(1) },
        { MF_STRING|MF_HELP, 2, "Help" }
    };
    static const struct menu_data out1[] =
    {
        { MF_STRING, 1, "File" },
        { MF_STRING|MF_HELP, 2, "Help" },
        { MF_BITMAP|MF_HELP, SC_CLOSE, MAKEINTRESOURCE(1) }
    };
    static const struct menu_data in2[] =
    {
        { MF_STRING, 1, "File" },
        { MF_BITMAP|MF_HELP, SC_CLOSE, MAKEINTRESOURCE(100) },
        { MF_STRING|MF_HELP, 2, "Help" }
    };
    static const struct menu_data out2[] =
    {
        { MF_STRING, 1, "File" },
        { MF_BITMAP|MF_HELP, SC_CLOSE, MAKEINTRESOURCE(100) },
        { MF_STRING|MF_HELP, 2, "Help" }
    };
    static const struct menu_data in3[] =
    {
        { MF_STRING, 1, "File" },
        { MF_SEPARATOR|MF_HELP, SC_CLOSE, MAKEINTRESOURCE(1) },
        { MF_STRING|MF_HELP, 2, "Help" }
    };
    static const struct menu_data out3[] =
    {
        { MF_STRING, 1, "File" },
        { MF_SEPARATOR|MF_HELP, SC_CLOSE, MAKEINTRESOURCE(0) },
        { MF_STRING|MF_HELP, 2, "Help" },
    };
    static const struct menu_data in4[] =
    {
        { MF_STRING, 1, "File" },
        { MF_BITMAP|MF_HELP, 1, MAKEINTRESOURCE(1) },
        { MF_STRING|MF_HELP, 2, "Help" }
    };
    static const struct menu_data out4[] =
    {
        { MF_STRING, 1, "File" },
        { MF_STRING|MF_HELP, 2, "Help" },
        { MF_BITMAP|MF_HELP, 1, MAKEINTRESOURCE(1) }
    };
    HMENU hmenu;

#define create_menu(a) create_menu_from_data((a), sizeof(a)/sizeof((a)[0]))
#define compare_menu(h, a) compare_menu_data((h), (a), sizeof(a)/sizeof((a)[0]))

    hmenu = create_menu(in1);
    compare_menu(hmenu, out1);
    DestroyMenu(hmenu);

    hmenu = create_menu(in2);
    compare_menu(hmenu, out2);
    DestroyMenu(hmenu);

    hmenu = create_menu(in3);
    compare_menu(hmenu, out3);
    DestroyMenu(hmenu);

    hmenu = create_menu(in4);
    compare_menu(hmenu, out4);
    DestroyMenu(hmenu);

#undef create_menu
#undef compare_menu
}

START_TEST(menu)
{
    init_function_pointers();

    /* Wine defines MENUITEMINFO for W2K and above. NT4 and below can't
     * handle that.
     */
    if (correct_behavior())
    {
        test_menu_add_string();
        test_menu_iteminfo();
        test_menu_search_bycommand();
        test_CheckMenuRadioItem();
        test_menu_resource_layout();
        test_InsertMenu();
    }

    register_menu_check_class();

    test_menu_locked_by_window();
    test_menu_ownerdraw();
    test_menu_bmp_and_string();

    if( !pSendInput)
        skip("SendInput is not available\n");
    else
        test_menu_input();
    test_menu_flags();

    test_menu_hilitemenuitem();
}
