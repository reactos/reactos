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

static LRESULT WINAPI menu_check_wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_ENTERMENULOOP:
        /* mark window as having entered menu loop */
        SetWindowLongPtrA(hwnd, GWLP_USERDATA, TRUE);
        /* exit menu modal loop
         * ( A SendMessage does not work on NT3.51 here ) */
        return PostMessageA(hwnd, WM_CANCELMODE, 0, 0);
    }
    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

/* globals to communicate between test and wndproc */

static BOOL bMenuVisible;
static INT popmenu;
static BOOL got_input;
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
static BOOL MOD_GotDrawItemMsg = FALSE;
static int  gflag_initmenupopup,
            gflag_entermenuloop,
            gflag_initmenu,
            gflag_enteridle;
static WPARAM selectitem_wp;
static LPARAM selectitem_lp;

/* wndproc used by test_menu_ownerdraw() */
static LRESULT WINAPI menu_ownerdraw_wnd_proc(HWND hwnd, UINT msg,
        WPARAM wparam, LPARAM lparam)
{
    static HMENU hmenupopup;
    switch (msg)
    {
        case WM_INITMENUPOPUP:
            gflag_initmenupopup++;
            hmenupopup = (HMENU) wparam;
            break;
        case WM_ENTERMENULOOP:
            gflag_entermenuloop++;
            break;
        case WM_INITMENU:
            gflag_initmenu++;
            break;
        case WM_MEASUREITEM:
            {
                MEASUREITEMSTRUCT* pmis = (MEASUREITEMSTRUCT*)lparam;
                if (winetest_debug > 1)
                    trace("WM_MEASUREITEM received data %Ix size %dx%d\n",
                            pmis->itemData, pmis->itemWidth, pmis->itemHeight);
                ok( !wparam, "wrong wparam %Ix\n", wparam );
                ok( pmis->CtlType == ODT_MENU, "wrong type %x\n", pmis->CtlType );
                MOD_odheight = pmis->itemHeight;
                pmis->itemWidth = MODsizes[pmis->itemData].cx;
                pmis->itemHeight = MODsizes[pmis->itemData].cy;
                return TRUE;
            }
        case WM_DRAWITEM:
            {
                DRAWITEMSTRUCT * pdis;
                TEXTMETRICA tm;
                HPEN oldpen;
                char chrs[]="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
                SIZE sz;
                int i;
                pdis = (DRAWITEMSTRUCT *) lparam;
                if (winetest_debug > 1) {
                    RECT rc;
                    GetMenuItemRect( hwnd, (HMENU)pdis->hwndItem, pdis->itemData ,&rc);
                    trace("WM_DRAWITEM received hwnd %p hmenu %p itemdata %Id item %d rc %s itemrc:  %s\n",
                            hwnd, pdis->hwndItem, pdis->itemData, pdis->itemID,
                            wine_dbgstr_rect(&pdis->rcItem), wine_dbgstr_rect(&rc));
                    oldpen=SelectObject( pdis->hDC, GetStockObject(
                                pdis->itemState & ODS_SELECTED ? WHITE_PEN :BLACK_PEN));
                    Rectangle( pdis->hDC, pdis->rcItem.left,pdis->rcItem.top,
                            pdis->rcItem.right,pdis->rcItem.bottom );
                    SelectObject( pdis->hDC, oldpen);
                }
                ok( !wparam, "wrong wparam %Ix\n", wparam );
                ok( pdis->CtlType == ODT_MENU, "wrong type %x\n", pdis->CtlType );
                /* calculate widths of some menu texts */
                if( ! MOD_txtsizes[0].size.cx)
                    for(i = 0; MOD_txtsizes[i].text; i++) {
                        char buf[100], *p;
                        RECT rc={0,0,0,0};
                        strcpy( buf, MOD_txtsizes[i].text);
                        if( ( p = strchr( buf, '\t'))) {
                            *p = '\0';
                            DrawTextA( pdis->hDC, p + 1, -1, &rc,
                                    DT_SINGLELINE|DT_CALCRECT);
                            MOD_txtsizes[i].sc_size.cx= rc.right - rc.left;
                            MOD_txtsizes[i].sc_size.cy= rc.bottom - rc.top;
                        }
                        DrawTextA( pdis->hDC, buf, -1, &rc,
                                DT_SINGLELINE|DT_CALCRECT);
                        MOD_txtsizes[i].size.cx= rc.right - rc.left;
                        MOD_txtsizes[i].size.cy= rc.bottom - rc.top;
                    }

                if( pdis->itemData > MOD_maxid) return TRUE;
                /* store the rectangle */
                MOD_rc[pdis->itemData] = pdis->rcItem;
                /* calculate average character width */
                GetTextExtentPointA( pdis->hDC, chrs, 52, &sz );
                MOD_avec = (sz.cx + 26)/52;
                GetTextMetricsA( pdis->hDC, &tm);
                MOD_hic = tm.tmHeight;
                MOD_GotDrawItemMsg = TRUE;
                return TRUE;
            }
        case WM_ENTERIDLE:
            {
                gflag_enteridle++;
                ok( lparam || broken(!lparam), /* win9x, nt4 */
                    "Menu window handle is NULL!\n");
                if( lparam) {
                    HMENU hmenu = (HMENU)SendMessageA( (HWND)lparam, MN_GETHMENU, 0, 0);
                    ok( hmenupopup == hmenu, "MN_GETHMENU returns %p expected %p\n",
                        hmenu, hmenupopup);
                }
                PostMessageA(hwnd, WM_CANCELMODE, 0, 0);
                return TRUE;
            }
        case WM_MENUSELECT:
            selectitem_wp = wparam;
            selectitem_lp = lparam;
            break;
    }
    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

static void register_menu_check_class(void)
{
    WNDCLASSA wc =
    {
        0,
        menu_check_wnd_proc,
        0,
        0,
        GetModuleHandleA(NULL),
        NULL,
        LoadCursorA(NULL, (LPCSTR)IDC_ARROW),
        (HBRUSH)(COLOR_BTNFACE+1),
        NULL,
        "WineMenuCheck",
    };

    atomMenuCheckClass = RegisterClassA(&wc);
}

static void test_getmenubarinfo(void)
{
    BOOL ret;
    HMENU hmenu;
    MENUBARINFO mbi;
    RECT rcw, rci;
    HWND hwnd;
    INT err;

    mbi.cbSize = sizeof(MENUBARINFO);

    hwnd = CreateWindowExA(0, (LPCSTR)MAKEINTATOM(atomMenuCheckClass), NULL,
            WS_SYSMENU | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 100, 100,
            NULL, NULL, NULL, NULL);
    ok(hwnd != NULL, "CreateWindowEx failed with error %ld\n", GetLastError());

    /* no menu: getmenubarinfo should fail */
    SetLastError(0xdeadbeef);
    ret = GetMenuBarInfo(hwnd, OBJID_MENU, 0, &mbi);
    err = GetLastError();
    ok(ret == FALSE, "GetMenuBarInfo should not have been successful\n");
    ok(err == 0xdeadbeef, "err = %d\n", err);

    /* create menubar, no items yet */
    hmenu = CreateMenu();
    ok(hmenu != NULL, "CreateMenu failed with error %ld\n", GetLastError());

    ret = SetMenu(hwnd, hmenu);
    ok(ret, "SetMenu failed with error %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = GetMenuBarInfo(NULL, OBJID_CLIENT, 0, &mbi);
    err = GetLastError();
    ok(!ret, "GetMenuBarInfo succeeded\n");
    ok(err == ERROR_INVALID_WINDOW_HANDLE, "err = %d\n", err);

    SetLastError(0xdeadbeef);
    ret = GetMenuBarInfo(hwnd, OBJID_CLIENT, 0, &mbi);
    err = GetLastError();
    ok(!ret, "GetMenuBarInfo succeeded\n");
    ok(err==ERROR_INVALID_MENU_HANDLE || broken(err==0xdeadbeef) /* NT, W2K, XP */, "err = %d\n", err);

    SetLastError(0xdeadbeef);
    ret = GetMenuBarInfo(hwnd, OBJID_MENU, -1, &mbi);
    err = GetLastError();
    ok(ret == FALSE, "GetMenuBarInfo should have failed\n");
    ok(err == 0xdeadbeef, "err = %d\n", err);

    mbi.cbSize = 1000;
    SetLastError(0xdeadbeef);
    ret = GetMenuBarInfo(hwnd, OBJID_MENU, 0, &mbi);
    err = GetLastError();
    ok(ret == FALSE, "GetMenuBarInfo should have failed\n");
    ok(err == ERROR_INVALID_PARAMETER, "err = %d\n", err);
    mbi.cbSize = sizeof(MENUBARINFO);

    SetLastError(0xdeadbeef);
    ret = GetMenuBarInfo(hwnd, 123, 0, &mbi);
    err = GetLastError();
    ok(ret == FALSE, "GetMenuBarInfo should have failed\n");
    ok(err == 0xdeadbeef, "err = %d\n", err);

    ret = GetMenuBarInfo(hwnd, OBJID_MENU, 0, &mbi);
    ok(ret, "GetMenuBarInfo failed with error %ld\n", GetLastError());

    ok(mbi.rcBar.left == 0 && mbi.rcBar.top == 0 && mbi.rcBar.bottom == 0 && mbi.rcBar.right == 0,
            "rcBar: Expected (0,0)-(0,0), got: %s\n", wine_dbgstr_rect(&mbi.rcBar));
    ok(mbi.hMenu == hmenu, "hMenu: Got %p instead of %p\n",
            mbi.hMenu, hmenu);
    ok(mbi.fBarFocused == 0, "fBarFocused: Got %d instead of 0.\n", mbi.fBarFocused);
    ok(mbi.fFocused == 0, "fFocused: Got %d instead of 0.\n", mbi.fFocused);

    /* add some items */
    ret = AppendMenuA(hmenu, MF_STRING , 100, "item 1");
    ok(ret, "AppendMenu failed.\n");
    ret = AppendMenuA(hmenu, MF_STRING , 101, "item 2");
    ok(ret, "AppendMenu failed.\n");
    ret = SetMenu(hwnd, hmenu);
    ok(ret, "SetMenu failed with error %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = GetMenuBarInfo(hwnd, OBJID_MENU, 200, &mbi);
    err = GetLastError();
    ok(ret == FALSE, "GetMenuBarInfo should have failed\n");
    ok(err == 0xdeadbeef, "err = %d\n", err);

    /* get info for the whole menu */
    ret = GetMenuBarInfo(hwnd, OBJID_MENU, 0, &mbi);
    ok(ret, "GetMenuBarInfo failed with error %ld\n", GetLastError());

    /* calculate menu rectangle, from window rectangle and the position of the first item  */
    ret = GetWindowRect(hwnd, &rcw);
    ok(ret, "GetWindowRect failed.\n");
    ret = GetMenuItemRect(hwnd, hmenu, 0, &rci);
    ok(ret, "GetMenuItemRect failed.\n");
    todo_wine ok(mbi.rcBar.left == rci.left && mbi.rcBar.top == rci.top &&
            mbi.rcBar.bottom == rci.bottom && mbi.rcBar.right == rcw.right - rci.left + rcw.left,
            "rcBar: Got %s instead of (%ld,%ld)-(%ld,%ld)\n", wine_dbgstr_rect(&mbi.rcBar),
            rci.left, rci.top, rcw.right - rci.left + rcw.left, rci.bottom);
    ok(mbi.hMenu == hmenu, "hMenu: Got %p instead of %p\n", mbi.hMenu, hmenu);
    ok(mbi.fBarFocused == 0, "fBarFocused: got %d instead of 0\n", mbi.fBarFocused);
    ok(mbi.fFocused == 0, "fFocused: got %d instead of 0\n", mbi.fFocused);

    /* get info for item nr.2 */
    ret = GetMenuBarInfo(hwnd, OBJID_MENU, 2, &mbi);
    ok(ret, "GetMenuBarInfo failed with error %ld\n", GetLastError());
    ret = GetMenuItemRect(hwnd, hmenu, 1, &rci);
    ok(ret, "GetMenuItemRect failed.\n");
    ok(EqualRect(&mbi.rcBar, &rci), "rcBar: Got %s instead of %s\n", wine_dbgstr_rect(&mbi.rcBar),
            wine_dbgstr_rect(&rci));
    ok(mbi.hMenu == hmenu, "hMenu: Got %p instead of %p\n", mbi.hMenu, hmenu);
    ok(mbi.fBarFocused == 0, "fBarFocused: got %d instead of 0\n", mbi.fBarFocused);
    ok(mbi.fFocused == 0, "fFocused: got %d instead of 0\n", mbi.fFocused);

    DestroyWindow(hwnd);
}

static void test_GetMenuItemRect(void)
{
    HWND hwnd;
    HMENU hmenu;
    HMENU popup_hmenu;
    RECT window_rect;
    RECT item_rect;
    POINT client_top_left;
    INT caption_height;
    BOOL ret;

    hwnd = CreateWindowW((LPCWSTR)MAKEINTATOM(atomMenuCheckClass), NULL, WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, NULL,
                         NULL, NULL, NULL);
    ok(hwnd != NULL, "CreateWindow failed with error %ld\n", GetLastError());
    hmenu = CreateMenu();
    ok(hmenu != NULL, "CreateMenu failed with error %ld\n", GetLastError());
    popup_hmenu = CreatePopupMenu();
    ok(popup_hmenu != NULL, "CreatePopupMenu failed with error %ld\n", GetLastError());
    ret = AppendMenuA(popup_hmenu, MF_STRING, 0, "Popup");
    ok(ret, "AppendMenu failed with error %ld\n", GetLastError());
    ret = AppendMenuA(hmenu, MF_STRING | MF_POPUP, (UINT_PTR)popup_hmenu, "Menu");
    ok(ret, "AppendMenu failed with error %ld\n", GetLastError());
    ret = SetMenu(hwnd, hmenu);
    ok(ret, "SetMenu failed with error %ld\n", GetLastError());

    /* Get the menu item rectangle of the displayed sysmenu item */
    ret = GetMenuItemRect(hwnd, hmenu, 0, &item_rect);
    ok(ret, "GetMenuItemRect failed with error %ld\n", GetLastError());
    GetWindowRect(hwnd, &window_rect);
    /* Get the screen coordinate of the left top corner of the client rectangle */
    client_top_left.x = 0;
    client_top_left.y = 0;
    MapWindowPoints(hwnd, 0, &client_top_left, 1);
    caption_height = GetSystemMetrics(SM_CYFRAME) + GetSystemMetrics(SM_CYCAPTION);

    ok(item_rect.left == client_top_left.x, "Expect item_rect.left %ld == %ld\n", item_rect.left, client_top_left.x);
    ok(item_rect.right <= window_rect.right, "Expect item_rect.right %ld <= %ld\n", item_rect.right, window_rect.right);
    /* A gap of 1 pixel is added deliberately in commit 75f9e64, so using equal operator would fail on Wine.
     * Check that top and bottom are correct with 1 pixel margin tolerance */
    ok(item_rect.top - (window_rect.top + caption_height) <= 1, "Expect item_rect.top %ld - %ld <= 1\n", item_rect.top,
       window_rect.top + caption_height);
    ok(item_rect.bottom - (client_top_left.y - 1) <= 1, "Expect item_rect.bottom %ld - %ld <= 1\n", item_rect.bottom,
       client_top_left.y - 1);

    /* Get the item rectangle of the not yet displayed popup menu item. */
    ret = GetMenuItemRect(hwnd, popup_hmenu, 0, &item_rect);
    ok(ret, "GetMenuItemRect failed with error %ld\n", GetLastError());
    ok(item_rect.left == client_top_left.x, "Expect item_rect.left %ld == %ld\n", item_rect.left, client_top_left.x);
    ok(item_rect.right == client_top_left.x, "Expect item_rect.right %ld == %ld\n", item_rect.right, client_top_left.x);
    ok(item_rect.top == client_top_left.y, "Expect item_rect.top %ld == %ld\n", item_rect.top, client_top_left.y);
    ok(item_rect.bottom == client_top_left.y, "Expect item_rect.bottom %ld == %ld\n", item_rect.bottom,
       client_top_left.y);

    DestroyWindow(hwnd);
}

static void test_system_menu(void)
{
    WCHAR testW[] = {'t','e','s','t',0};
    BOOL ret;
    HMENU menu;
    HWND hwnd;
    MENUITEMINFOA info;
    MENUITEMINFOW infoW;
    char buffer[80];
    char found[0x200];
    int i, res;

    hwnd = CreateWindowExA(0, (LPCSTR)MAKEINTATOM(atomMenuCheckClass), NULL,
                           WS_SYSMENU | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 100, 100,
                           NULL, NULL, NULL, NULL);
    ok(hwnd != NULL, "CreateWindowEx failed with error %ld\n", GetLastError());
    menu = GetSystemMenu( hwnd, FALSE );
    ok( menu != NULL, "no system menu\n" );

    for (i = 0xf000; i < 0xf200; i++)
    {
        memset( &info, 0xcc, sizeof(info) );
        info.cbSize = sizeof(info);
        info.fMask = MIIM_STRING | MIIM_FTYPE | MIIM_ID;
        info.dwTypeData = buffer;
        info.cch = sizeof( buffer );
        ret = GetMenuItemInfoA( menu, i, FALSE, &info );
        if (ret) trace( "found %x: '%s'\n", i, buffer );
        switch (i)
        {
        case SC_RESTORE:
        case SC_SIZE:
        case SC_MOVE:
        case SC_MINIMIZE:
        case SC_MAXIMIZE:
        case SC_CLOSE:
            ok( ret, "%x menu item not found\n", i );
            break;
        case SC_SCREENSAVE+1:  /* used for the 'About Wine' entry, don't test */
            break;
        default:
            ok( !ret, "%x menu item found\n", i );
            break;
        }
        found[i - 0xf000] = ret;
    }

    for (i = 0xf000; i < 0xf200; i++)
    {
        res = CheckMenuItem( menu, i, 0 );
        if (res == -1) ok( !found[i - 0xf000], "could not check existent item %x\n", i );
        else ok( found[i - 0xf000], "could check non-existent item %x\n", i );

        res = EnableMenuItem( menu, i, 0 );
        if (res == -1) ok( !found[i - 0xf000], "could not enable existent item %x\n", i );
        else ok( found[i - 0xf000], "could enable non-existent item %x\n", i );

        res = GetMenuState( menu, i, 0 );
        if (res == -1) ok( !found[i - 0xf000], "could not get state existent item %x\n", i );
        else ok( found[i - 0xf000], "could get state of non-existent item %x\n", i );

        if (!found[i - 0xf000])  /* don't remove the existing ones */
        {
            ret = RemoveMenu( menu, i, 0 );
            ok( !ret, "could remove non-existent item %x\n", i );
        }

        ret = ModifyMenuA( menu, i, 0, i, "test" );
        if (i == SC_TASKLIST) ok( ret, "failed to modify SC_TASKLIST\n" );
        else if (!ret) ok( !found[i - 0xf000], "could not modify existent item %x\n", i );
        else ok( found[i - 0xf000], "could modify non-existent item %x\n", i );

        ret = ModifyMenuW( menu, i, 0, i, testW );
        if (i == SC_TASKLIST) ok( ret, "failed to modify SC_TASKLIST\n" );
        else if (!ret) ok( !found[i - 0xf000], "could not modify existent item %x\n", i );
        else ok( found[i - 0xf000], "could modify non-existent item %x\n", i );

        ret = ModifyMenuA( menu, i, MF_BYPOSITION, i, "test" );
        ok( !ret, "could modify non-existent item %x\n", i );

        strcpy( buffer, "test" );
        memset( &info, 0xcc, sizeof(info) );
        info.cbSize = sizeof(info);
        info.fMask = MIIM_STRING | MIIM_ID;
        info.wID = i;
        info.dwTypeData = buffer;
        info.cch = strlen( buffer );
        ret = SetMenuItemInfoA( menu, i, FALSE, &info );
        if (i == SC_TASKLIST) ok( ret, "failed to set SC_TASKLIST\n" );
        else if (!ret) ok( !found[i - 0xf000], "could not set existent item %x\n", i );
        else ok( found[i - 0xf000], "could set non-existent item %x\n", i );
        ret = SetMenuItemInfoA( menu, i, TRUE, &info );
        ok( !ret, "could modify non-existent item %x\n", i );

        memset( &infoW, 0xcc, sizeof(infoW) );
        infoW.cbSize = sizeof(infoW);
        infoW.fMask = MIIM_STRING | MIIM_ID;
        infoW.wID = i;
        infoW.dwTypeData = testW;
        infoW.cch = lstrlenW( testW );
        ret = SetMenuItemInfoW( menu, i, FALSE, &infoW );
        if (i == SC_TASKLIST) ok( ret, "failed to set SC_TASKLIST\n" );
        else if (!ret) ok( !found[i - 0xf000], "could not set existent item %x\n", i );
        else ok( found[i - 0xf000], "could set non-existent item %x\n", i );
        ret = SetMenuItemInfoW( menu, i, TRUE, &infoW );
        ok( !ret, "could modify non-existent item %x\n", i );
    }

    /* confirm that SC_TASKLIST still does not exist */
    for (i = 0xf000; i < 0xf200; i++)
    {
        memset( &info, 0xcc, sizeof(info) );
        info.cbSize = sizeof(info);
        info.fMask = MIIM_STRING | MIIM_FTYPE | MIIM_ID;
        info.dwTypeData = buffer;
        info.cch = sizeof( buffer );
        ret = GetMenuItemInfoA( menu, i, FALSE, &info );
        switch (i)
        {
        case SC_RESTORE:
        case SC_SIZE:
        case SC_MOVE:
        case SC_MINIMIZE:
        case SC_MAXIMIZE:
        case SC_CLOSE:
            ok( ret, "%x menu item not found\n", i );
            break;
        case SC_SCREENSAVE+1:  /* used for the 'About Wine' entry, don't test */
            break;
        default:
            ok( !ret, "%x menu item found\n", i );
            break;
        }
    }

    /* now a normal (non-system) menu */

    menu = CreateMenu();
    ok( menu != NULL, "CreateMenu failed with error %ld\n", GetLastError() );

    res = CheckMenuItem( menu, SC_TASKLIST, 0 );
    ok( res == -1, "CheckMenuItem succeeded\n" );
    res = EnableMenuItem( menu, SC_TASKLIST, 0 );
    ok( res == -1, "EnableMenuItem succeeded\n" );
    res = GetMenuState( menu, SC_TASKLIST, 0 );
    ok( res == -1, "GetMenuState succeeded\n" );
    ret = RemoveMenu( menu, SC_TASKLIST, 0 );
    ok( !ret, "RemoveMenu succeeded\n" );
    ret = ModifyMenuA( menu, SC_TASKLIST, 0, SC_TASKLIST, "test" );
    ok( ret, "ModifyMenuA failed err %ld\n", GetLastError() );
    ret = ModifyMenuW( menu, SC_TASKLIST, 0, SC_TASKLIST, testW );
    ok( ret, "ModifyMenuW failed err %ld\n", GetLastError() );
    ret = ModifyMenuA( menu, SC_TASKLIST-1, 0, SC_TASKLIST, "test" );
    ok( !ret, "ModifyMenu succeeded on SC_TASKLIST-1\n" );
    strcpy( buffer, "test" );
    memset( &info, 0xcc, sizeof(info) );
    info.cbSize = sizeof(info);
    info.fMask = MIIM_STRING | MIIM_ID;
    info.wID = SC_TASKLIST;
    info.dwTypeData = buffer;
    info.cch = strlen( buffer );
    ret = SetMenuItemInfoA( menu, SC_TASKLIST, FALSE, &info );
    ok( ret, "failed to set SC_TASKLIST\n" );
    ret = SetMenuItemInfoA( menu, SC_TASKLIST+1, FALSE, &info );
    ok( !ret, "succeeded setting SC_TASKLIST+1\n" );
    ret = SetMenuItemInfoA( menu, SC_TASKLIST, TRUE, &info );
    ok( !ret, "succeeded setting by position\n" );

    memset( &infoW, 0xcc, sizeof(infoW) );
    infoW.cbSize = sizeof(infoW);
    infoW.fMask = MIIM_STRING | MIIM_ID;
    infoW.wID = SC_TASKLIST;
    infoW.dwTypeData = testW;
    infoW.cch = lstrlenW( testW );
    ret = SetMenuItemInfoW( menu, SC_TASKLIST, FALSE, &infoW );
    ok( ret, "failed to set SC_TASKLIST\n" );
    ret = SetMenuItemInfoW( menu, SC_TASKLIST+1, FALSE, &infoW );
    ok( !ret, "succeeded setting SC_TASKLIST+1\n" );
    ret = SetMenuItemInfoW( menu, SC_TASKLIST, TRUE, &infoW );
    ok( !ret, "succeeded setting by position\n" );

    DestroyMenu( menu );
    DestroyWindow( hwnd );
}

/* demonstrates that windows locks the menu object so that it is still valid
 * even after a client calls DestroyMenu on it */
static void test_menu_locked_by_window(void)
{
    BOOL ret;
    HMENU hmenu;
    HWND hwnd = CreateWindowExA(0, (LPCSTR)MAKEINTATOM(atomMenuCheckClass), NULL,
                               WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 200, 200,
                               NULL, NULL, NULL, NULL);
    ok(hwnd != NULL, "CreateWindowEx failed with error %ld\n", GetLastError());
    hmenu = CreateMenu();
    ok(hmenu != NULL, "CreateMenu failed with error %ld\n", GetLastError());
    ret = InsertMenuA(hmenu, 0, MF_STRING, 0, "&Test");
    ok(ret, "InsertMenu failed with error %ld\n", GetLastError());
    ret = SetMenu(hwnd, hmenu);
    ok(ret, "SetMenu failed with error %ld\n", GetLastError());
    ret = DestroyMenu(hmenu);
    ok(ret, "DestroyMenu failed with error %ld\n", GetLastError());

    ret = DrawMenuBar(hwnd);
    ok(ret, "DrawMenuBar failed with error %ld\n", GetLastError());
    ret = IsMenu(GetMenu(hwnd));
    ok(!ret || broken(ret) /* nt4 */, "Menu handle should have been destroyed\n");

    SendMessageA(hwnd, WM_SYSCOMMAND, SC_KEYMENU, 0);
    /* did we process the WM_INITMENU message? */
    ret = GetWindowLongPtrA(hwnd, GWLP_USERDATA);
    todo_wine {
    ok(ret, "WM_INITMENU should have been sent\n");
    }

    DestroyWindow(hwnd);
}

/* demonstrates that subpopup's are locked
 * even after a client calls DestroyMenu on it */
static LRESULT WINAPI subpopuplocked_wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    HWND hwndmenu;
    switch (msg)
    {
    case WM_ENTERIDLE:
        hwndmenu = GetCapture();
        if( hwndmenu) {
            PostMessageA( hwndmenu, WM_KEYDOWN, VK_DOWN, 0);
            PostMessageA( hwndmenu, WM_KEYDOWN, VK_RIGHT, 0);
            PostMessageA( hwndmenu, WM_KEYDOWN, VK_RETURN, 0);
        }
    }
    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

static void test_subpopup_locked_by_menu(void)
{
    BOOL ret;
    HMENU hmenu, hsubmenu;
    MENUINFO mi = { sizeof( MENUINFO)};
    MENUITEMINFOA mii = { sizeof( MENUITEMINFOA)};
    HWND hwnd;
    const int itemid = 0x1234567;

    /* create window, popupmenu with one subpopup */
    hwnd = CreateWindowExA(0, (LPCSTR)MAKEINTATOM(atomMenuCheckClass), NULL,
            WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 200, 200,
            NULL, NULL, NULL, NULL);
    ok(hwnd != NULL, "CreateWindowEx failed with error %ld\n", GetLastError());
    SetWindowLongPtrA( hwnd, GWLP_WNDPROC, (LONG_PTR) subpopuplocked_wnd_proc);
    hmenu = CreatePopupMenu();
    ok(hmenu != NULL, "CreateMenu failed with error %ld\n", GetLastError());
    hsubmenu = CreatePopupMenu();
    ok(hsubmenu != NULL, "CreateMenu failed with error %ld\n", GetLastError());
    ret = InsertMenuA(hmenu, 0, MF_BYPOSITION | MF_POPUP | MF_STRING, (UINT_PTR)hsubmenu,
            "PopUpLockTest");
    ok(ret, "InsertMenu failed with error %ld\n", GetLastError());
    ret = InsertMenuA(hsubmenu, 0, MF_BYPOSITION | MF_STRING, itemid, "PopUpMenu");
    ok(ret, "InsertMenu failed with error %ld\n", GetLastError());
    /* first some tests that all this functions properly */
    mii.fMask = MIIM_SUBMENU;
    ret = GetMenuItemInfoA( hmenu, 0, TRUE, &mii);
    ok( ret, "GetMenuItemInfo failed error %ld\n", GetLastError());
    ok( mii.hSubMenu == hsubmenu, "submenu is %p\n", mii.hSubMenu);
    mi.fMask |= MIM_STYLE;
    ret = GetMenuInfo( hsubmenu, &mi);
    ok( ret , "GetMenuInfo returned 0 with error %ld\n", GetLastError());
    ret = IsMenu( hsubmenu);
    ok( ret , "Menu handle is not valid\n");

    ret = TrackPopupMenu( hmenu, TPM_RETURNCMD, 100,100, 0, hwnd, NULL);
    ok( ret == itemid , "TrackPopupMenu returned %d error is %ld\n", ret, GetLastError());

    /* then destroy the sub-popup */
    ret = DestroyMenu( hsubmenu);
    ok(ret, "DestroyMenu failed with error %ld\n", GetLastError());
    /* and repeat the tests */
    mii.fMask = MIIM_SUBMENU;
    ret = GetMenuItemInfoA( hmenu, 0, TRUE, &mii);
    ok( ret, "GetMenuItemInfo failed error %ld\n", GetLastError());
    /* GetMenuInfo fails now */
    ok( mii.hSubMenu == hsubmenu, "submenu is %p\n", mii.hSubMenu);
    mi.fMask |= MIM_STYLE;
    ret = GetMenuInfo( hsubmenu, &mi);
    ok( !ret , "GetMenuInfo should have failed\n");
    /* IsMenu says it is not */
    ret = IsMenu( hsubmenu);
    ok( !ret , "Menu handle should be invalid\n");

    /* but TrackPopupMenu still works! */
    ret = TrackPopupMenu( hmenu, TPM_RETURNCMD, 100,100, 0, hwnd, NULL);
    todo_wine {
        ok( ret == itemid , "TrackPopupMenu returned %d error is %ld\n", ret, GetLastError());
    }

    /* clean up */
    DestroyMenu( hmenu);
    DestroyWindow(hwnd);
}

static void test_menu_ownerdraw(void)
{
    int i,j,k;
    BOOL ret;
    HMENU hmenu;
    MENUITEMINFOA mii;
    LONG leftcol;
    HWND hwnd = CreateWindowExA(0, (LPCSTR)MAKEINTATOM(atomMenuCheckClass), NULL,
                               WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 200, 200,
                               NULL, NULL, NULL, NULL);
    ok(hwnd != NULL, "CreateWindowEx failed with error %ld\n", GetLastError());
    if( !hwnd) return;
    SetWindowLongPtrA( hwnd, GWLP_WNDPROC, (LONG_PTR)menu_ownerdraw_wnd_proc);
    hmenu = CreatePopupMenu();
    ok(hmenu != NULL, "CreateMenu failed with error %ld\n", GetLastError());
    if( !hmenu) { DestroyWindow(hwnd);return;}
    k=0;
    for( j=0;j<2;j++) /* create columns */
        for(i=0;i<2;i++) { /* create rows */
            ret = AppendMenuA( hmenu, MF_OWNERDRAW |
                              (i==0 ? MF_MENUBREAK : 0), k, (LPCSTR)MAKEINTRESOURCE(k));
            k++;
            ok( ret, "AppendMenu failed for %d\n", k-1);
        }
    MOD_maxid = k-1;
    assert( k <= ARRAY_SIZE(MOD_rc));
    /* display the menu */
    TrackPopupMenu( hmenu, TPM_RETURNCMD, 100,100, 0, hwnd, NULL);

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
    ModifyMenuA( hmenu, 0, MF_BYCOMMAND| MF_OWNERDRAW| MF_SEPARATOR, 0, 0);
    /* display the menu */
    TrackPopupMenu( hmenu, TPM_RETURNCMD, 100,100, 0, hwnd, NULL);
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

    /* test owner-drawn callback bitmap */
    ModifyMenuA( hmenu, 1, MF_BYPOSITION | MFT_BITMAP, 1, (LPCSTR)HBMMENU_CALLBACK );
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_BITMAP | MIIM_FTYPE | MIIM_ID;
    if (GetMenuItemInfoA( hmenu, 1, TRUE, &mii ))
    {
        ok( mii.fType == MFT_BITMAP, "wrong type %x\n", mii.fType );
        ok( mii.wID == 1, "wrong id %x\n", mii.wID );
        ok( mii.hbmpItem == HBMMENU_CALLBACK, "wrong data %p\n", mii.hbmpItem );
    }
    TrackPopupMenu( hmenu, TPM_RETURNCMD, 100,100, 0, hwnd, NULL);

    /* test width/height of an ownerdraw menu bar as well */
    ret = DestroyMenu(hmenu);
    ok(ret, "DestroyMenu failed with error %ld\n", GetLastError());
    hmenu = CreateMenu();
    ok(hmenu != NULL, "CreateMenu failed with error %ld\n", GetLastError());
    if( !hmenu) { DestroyWindow(hwnd);return;}
    MOD_maxid=1;
    for(i=0;i<2;i++) { 
        ret = AppendMenuA( hmenu, MF_OWNERDRAW, i, 0 );
        ok( ret, "AppendMenu failed for %d\n", i);
    }
    ret = SetMenu( hwnd, hmenu);
    UpdateWindow( hwnd); /* hack for wine to draw the window + menu */
    ok(ret, "SetMenu failed with error %ld\n", GetLastError());
    /* test width */
    ok( MOD_rc[0].right - MOD_rc[0].left == 2 * MOD_avec + MOD_SIZE,
            "width of owner drawn menu item is wrong. Got %ld expected %d\n",
            MOD_rc[0].right - MOD_rc[0].left , 2*MOD_avec + MOD_SIZE);
    /* test height */
    ok( MOD_rc[0].bottom - MOD_rc[0].top == GetSystemMetrics( SM_CYMENU) - 1,
            "Height of owner drawn menu item is wrong. Got %ld expected %d\n",
            MOD_rc[0].bottom - MOD_rc[0].top, GetSystemMetrics( SM_CYMENU) - 1);

    /* clean up */
    ret = DestroyMenu(hmenu);
    ok(ret, "DestroyMenu failed with error %ld\n", GetLastError());
    DestroyWindow(hwnd);
}

/* helper for test_menu_bmp_and_string() */
static void test_mbs_help( int ispop, int hassub, int mnuopt,
        HWND hwnd, int arrowwidth, int count, HBITMAP hbmp,
        SIZE bmpsize, LPCSTR text, SIZE size, SIZE sc_size)
{
    BOOL ret;
    HMENU hmenu, submenu;
    MENUITEMINFOA mii={ sizeof( MENUITEMINFOA )};
    MENUINFO mi;
    RECT rc;
    CHAR text_copy[16];
    int hastab,  expect;
    BOOL failed = FALSE;

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
    ok( submenu != 0, "CreateMenu failed with error %ld\n", GetLastError());
    if( ispop)
        hmenu = CreatePopupMenu();
    else
        hmenu = CreateMenu();
    ok( hmenu != 0, "Create{Popup}Menu failed with error %ld\n", GetLastError());
    if( hassub) {
        mii.fMask |= MIIM_SUBMENU;
        mii.hSubMenu = submenu;
    }
    if( mnuopt) {
        mi.cbSize = sizeof(mi);
        mi.fMask = MIM_STYLE;
        GetMenuInfo( hmenu, &mi);
        if( mnuopt) mi.dwStyle |= mnuopt == 1 ? MNS_NOCHECK : MNS_CHECKORBMP;
        ret = SetMenuInfo( hmenu, &mi);
        ok( ret, "SetMenuInfo failed with error %ld\n", GetLastError());
    }
    ret = InsertMenuItemA( hmenu, 0, FALSE, &mii);
    ok( ret, "InsertMenuItem failed with error %ld\n", GetLastError());
    failed = !ret;
    if( winetest_debug) {
        HDC hdc=GetDC(hwnd);
        RECT rc = {100, 50, 400, 70};
        char buf[100];

        sprintf( buf,"%d text \"%s\" mnuopt %d", count, text ? text: "(nil)", mnuopt);
        FillRect( hdc, &rc, (HBRUSH) COLOR_WINDOW);
        TextOutA( hdc, 10, 50, buf, strlen( buf));
        ReleaseDC( hwnd, hdc);
    }
    if(ispop)
        TrackPopupMenu( hmenu, TPM_RETURNCMD, 100,100, 0, hwnd, NULL);
    else {
        ret = SetMenu( hwnd, hmenu);
        ok(ret, "SetMenu failed with error %ld\n", GetLastError());
        DrawMenuBar( hwnd);
    }
    ret = GetMenuItemRect( hwnd, hmenu, 0, &rc);
    ok(ret, "GetMenuItemRect failed with error %ld\n", GetLastError());

    if (0)  /* comment out menu size checks, behavior is different in almost every Windows version */
            /* the tests should however succeed on win2000, XP and Wine (at least up to 1.1.15) */
            /* with a variety of dpis and desktop font sizes */
    {
        /* check menu width */
        if( ispop)
            expect = ( text || hbmp ?
                       4 + (mnuopt != 1 ? GetSystemMetrics(SM_CXMENUCHECK) : 0)
                       : 0) +
                       arrowwidth  + MOD_avec + (hbmp ?
                                    ((INT_PTR)hbmp<0||(INT_PTR)hbmp>12 ? bmpsize.cx + 2 : GetSystemMetrics( SM_CXMENUSIZE) + 2)
                                    : 0) +
                (text && hastab ? /* TAB space */
                 MOD_avec + ( hastab==2 ? sc_size.cx : 0) : 0) +
                (text ?  2 + (text[0] ? size.cx :0): 0) ;
        else
            expect = !(text || hbmp) ? 0 :
                ( hbmp ? (text ? 2:0) + bmpsize.cx  : 0 ) +
                (text ? 2 * MOD_avec + (text[0] ? size.cx :0): 0) ;
        ok( rc.right - rc.left == expect,
            "menu width wrong, got %ld expected %d\n", rc.right - rc.left, expect);
        failed = failed || !(rc.right - rc.left == expect);
        /* check menu height */
        if( ispop)
            expect = max( ( !(text || hbmp) ? GetSystemMetrics( SM_CYMENUSIZE)/2 : 0),
                          max( (text ? max( 2 + size.cy, MOD_hic + 4) : 0),
                               (hbmp ?
                                   ((INT_PTR)hbmp<0||(INT_PTR)hbmp>12 ?
                                       bmpsize.cy + 2
                                     : GetSystemMetrics( SM_CYMENUSIZE) + 2)
                                 : 0)));
        else
            expect = ( !(text || hbmp) ? GetSystemMetrics( SM_CYMENUSIZE)/2 :
                       max( GetSystemMetrics( SM_CYMENU) - 1, (hbmp ? bmpsize.cy : 0)));
        ok( rc.bottom - rc.top == expect,
            "menu height wrong, got %ld expected %d (%d)\n",
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
                "bitmap left is %ld expected %d\n", MOD_rc[0].left, expect);
            failed = failed || !(expect == MOD_rc[0].left);
            /* vertical */
            expect = (rc.bottom - rc.top - MOD_rc[0].bottom + MOD_rc[0].top) / 2;
            ok( expect == MOD_rc[0].top,
                "bitmap top is %ld expected %d\n", MOD_rc[0].top, expect);
            failed = failed || !(expect == MOD_rc[0].top);
        }
    }
    /* if there was a failure, report details */
    if( failed) {
        trace("*** count %d %s text \"%s\" bitmap %p bmsize %ld,%ld textsize %ld+%ld,%ld mnuopt %d hastab %d\n",
                count, (ispop? "POPUP": "MENUBAR"),text ? text: "(nil)", hbmp, bmpsize.cx, bmpsize.cy,
                size.cx, size.cy, sc_size.cx, mnuopt, hastab);
        trace("    check %d,%d arrow %d avechar %d\n",
                GetSystemMetrics(SM_CXMENUCHECK ),
                GetSystemMetrics(SM_CYMENUCHECK ),arrowwidth, MOD_avec);
        if( hbmp == HBMMENU_CALLBACK)
            trace( "    rc %s bmp.rc %s\n", wine_dbgstr_rect(&rc), wine_dbgstr_rect(&MOD_rc[0]));
    }
    /* clean up */
    ret = DestroyMenu(submenu);
    ok(ret, "DestroyMenu failed with error %ld\n", GetLastError());
    ret = DestroyMenu(hmenu);
    ok(ret, "DestroyMenu failed with error %ld\n", GetLastError());
}


static void test_menu_bmp_and_string(void)
{
    BYTE bmfill[300];
    HBITMAP hbm_arrow;
    BITMAP bm;
    INT arrowwidth;
    HWND hwnd;
    HMENU hsysmenu;
    MENUINFO mi= {sizeof(MENUINFO)};
    MENUITEMINFOA mii= {sizeof(MENUITEMINFOA)};
    int count, szidx, txtidx, bmpidx, hassub, mnuopt, ispop;
    BOOL got;

    memset( bmfill, 0xcc, sizeof( bmfill));
    hwnd = CreateWindowExA(0, (LPCSTR)MAKEINTATOM(atomMenuCheckClass), NULL, WS_SYSMENU |
                          WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 200, 200,
                          NULL, NULL, NULL, NULL);
    hbm_arrow = LoadBitmapA( 0, (LPCSTR)OBM_MNARROW);
    GetObjectA( hbm_arrow, sizeof(bm), &bm);
    arrowwidth = bm.bmWidth;
    ok(hwnd != NULL, "CreateWindowEx failed with error %ld\n", GetLastError());
    if( !hwnd) return;
    /* test system menu */
    hsysmenu = GetSystemMenu( hwnd, FALSE);
    ok( hsysmenu != NULL, "GetSystemMenu failed with error %ld\n", GetLastError());
    mi.fMask = MIM_STYLE;
    mi.dwStyle = 0;
    got = GetMenuInfo( hsysmenu, &mi);
    ok( got, "GetMenuInfo failed gle=%ld\n", GetLastError());
    ok( MNS_CHECKORBMP == mi.dwStyle, "System Menu Style is %08lx, without the bit %08x\n",
        mi.dwStyle, MNS_CHECKORBMP);
    mii.fMask = MIIM_BITMAP;
    mii.hbmpItem = NULL;
    got = GetMenuItemInfoA( hsysmenu, SC_CLOSE, FALSE, &mii);
    ok( got, "GetMenuItemInfoA failed gle=%ld\n", GetLastError());
    ok( HBMMENU_POPUP_CLOSE == mii.hbmpItem, "Item info did not get the right hbitmap: got %p  expected %p\n",
        mii.hbmpItem, HBMMENU_POPUP_CLOSE);

    memset(&mii, 0x81, sizeof(mii));
    mii.cbSize = sizeof(mii);
    mii.fMask  = MIIM_STATE | MIIM_ID | MIIM_TYPE | MIIM_DATA;
    mii.dwTypeData = (LPSTR)bmfill;
    mii.cch = sizeof(bmfill);
    mii.dwItemData = 0x81818181;
    got = GetMenuItemInfoA(hsysmenu, SC_RESTORE, FALSE, &mii);
    ok(got, "GetMenuItemInfo failed\n");
    ok((mii.fType & ~(MFT_RIGHTJUSTIFY|MFT_RIGHTORDER)) == MFT_STRING, "expected MFT_STRING, got %#x\n", mii.fType);
    ok(mii.fState == MF_ENABLED, "expected MF_ENABLED, got %#x\n", mii.fState);
    ok(mii.wID == SC_RESTORE, "expected SC_RESTORE, got %#x\n", mii.wID);
    ok(mii.hSubMenu == 0, "expected 0, got %p\n", mii.hSubMenu);
    ok(mii.dwItemData == 0, "expected 0, got %#Ix\n", mii.dwItemData);
    ok(mii.dwTypeData == (LPSTR)bmfill, "expected %p, got %p\n", bmfill, mii.dwTypeData);
    ok(mii.cch != 0, "cch should not be 0\n");
    ok(mii.hbmpItem == HBMMENU_POPUP_RESTORE, "expected HBMMENU_POPUP_RESTORE, got %p\n", mii.hbmpItem);

    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_TYPE;
    mii.hbmpItem = (HBITMAP)0x81818181;
    got = GetMenuItemInfoA(hsysmenu, SC_CLOSE, FALSE, &mii);
    ok(got, "GetMenuItemInfo failed\n");
    ok((mii.fType & ~(MFT_RIGHTJUSTIFY|MFT_RIGHTORDER)) == MFT_STRING, "expected MFT_STRING, got %#x\n", mii.fType);
    ok(mii.fState == MF_ENABLED, "expected MF_ENABLED, got %#x\n", mii.fState);
    ok(mii.wID == SC_RESTORE, "expected SC_RESTORE, got %#x\n", mii.wID);
    ok(mii.hSubMenu == 0, "expected 0, got %p\n", mii.hSubMenu);
    ok(mii.dwItemData == 0, "expected 0, got %#Ix\n", mii.dwItemData);
    ok(mii.dwTypeData == (LPSTR)bmfill, "expected %p, got %p\n", bmfill, mii.dwTypeData);
    ok(mii.cch != 0, "cch should not be 0\n");
    ok(mii.hbmpItem == HBMMENU_POPUP_CLOSE, "expected HBMMENU_POPUP_CLOSE, got %p\n", mii.hbmpItem);

    SetWindowLongPtrA( hwnd, GWLP_WNDPROC, (LONG_PTR)menu_ownerdraw_wnd_proc);

    if( winetest_debug)
        trace("    check %d,%d arrow %d avechar %d\n",
                GetSystemMetrics(SM_CXMENUCHECK ),
                GetSystemMetrics(SM_CYMENUCHECK ),arrowwidth, MOD_avec);
    count = 0;
    MOD_maxid = 0;
    for( ispop=1; ispop >= 0; ispop--){
        static SIZE bmsizes[]= {
            {10,10},{38,38},{1,30},{55,5}};
        for( szidx=0; szidx < ARRAY_SIZE(bmsizes); szidx++) {
            HBITMAP hbm = CreateBitmap( bmsizes[szidx].cx, bmsizes[szidx].cy,1,1,bmfill);
            HBITMAP bitmaps[] = { HBMMENU_CALLBACK, hbm, HBMMENU_POPUP_CLOSE, NULL  };
            ok( hbm != 0, "CreateBitmap failed err %ld\n", GetLastError());
            for( txtidx = 0; txtidx < ARRAY_SIZE(MOD_txtsizes); txtidx++) {
                for( hassub = 0; hassub < 2 ; hassub++) { /* add submenu item */
                    for( mnuopt = 0; mnuopt < 3 ; mnuopt++){ /* test MNS_NOCHECK/MNS_CHECKORBMP */
                        for( bmpidx = 0; bmpidx <ARRAY_SIZE(bitmaps); bmpidx++) {
                            /* no need to test NULL bitmaps of several sizes */
                            if( !bitmaps[bmpidx] && szidx > 0) continue;
                            /* the HBMMENU_POPUP not to test for menu bars */
                            if( !ispop &&
                                bitmaps[bmpidx] >= HBMMENU_POPUP_CLOSE &&
                                bitmaps[bmpidx] <= HBMMENU_POPUP_MINIMIZE) continue;
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
    MENUITEMINFOA info;
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
    InsertMenuItemA(hmenu, 0, TRUE, &info );

    memset( &info, 0, sizeof info );
    info.cbSize = sizeof info;
    info.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_STATE | MIIM_DATA | MIIM_ID;
    info.dwTypeData = string;
    info.cch = sizeof string;
    string[0] = 0;
    GetMenuItemInfoA( hmenu, 0, TRUE, &info );

    ok( !strcmp( string, "blah" ), "menu item name differed\n");

    /* Test combination of ownerdraw and strings with GetMenuItemString(A/W) */
    strcpy(string, "Dummy string");
    memset(&info, 0x00, sizeof(info));
    info.cbSize= sizeof(MENUITEMINFOA);
    info.fMask= MIIM_FTYPE | MIIM_STRING; /* Set OwnerDraw + typeData */
    info.fType= MFT_OWNERDRAW;
    info.dwTypeData= string; 
    rc = InsertMenuItemA( hmenu, 0, TRUE, &info );
    ok (rc, "InsertMenuItem failed\n");

    strcpy(string,"Garbage");
    ok (GetMenuStringA( hmenu, 0, strback, 99, MF_BYPOSITION), "GetMenuString on ownerdraw entry failed\n");
    ok (!strcmp( strback, "Dummy string" ), "Menu text from Ansi version incorrect\n");

    ret = GetMenuStringW( hmenu, 0, strbackW, 99, MF_BYPOSITION );
    ok (ret, "GetMenuStringW on ownerdraw entry failed\n");
    ok (!lstrcmpW( strbackW, expectedString ), "Menu text from Unicode version incorrect\n");

    /* Just try some invalid parameter tests */
    SetLastError(0xdeadbeef);
    rc = SetMenuItemInfoA( hmenu, 0, TRUE, NULL );
    ret = GetLastError();
    ok (!rc, "SetMenuItemInfoA succeeded unexpectedly\n");
    ok (ret == ERROR_INVALID_PARAMETER, "Expected 87, got %d\n", ret);

    SetLastError(0xdeadbeef);
    rc = SetMenuItemInfoA( hmenu, 0, FALSE, NULL );
    ret = GetLastError();
    ok (!rc, "SetMenuItemInfoA succeeded unexpectedly\n");
    ok (ret == ERROR_INVALID_PARAMETER, "Expected 87, got %d\n", ret);

    /* Just change ftype to string and see what text is stored */
    memset(&info, 0x00, sizeof(info));
    info.cbSize= sizeof(MENUITEMINFOA);
    info.fMask= MIIM_FTYPE; /* Set string type */
    info.fType= MFT_STRING;
    info.dwTypeData= (char *)0xdeadbeef;
    rc = SetMenuItemInfoA( hmenu, 0, TRUE, &info );
    ok (rc, "SetMenuItemInfo failed\n");

    /* Did we keep the old dwTypeData? */
    ok (GetMenuStringA( hmenu, 0, strback, 99, MF_BYPOSITION), "GetMenuString on ownerdraw entry failed\n");
    ok (!strcmp( strback, "Dummy string" ), "Menu text from Ansi version incorrect\n");

    /* Ensure change to bitmap type fails */
    memset(&info, 0x00, sizeof(info));
    info.cbSize= sizeof(MENUITEMINFOA);
    info.fMask= MIIM_FTYPE; /* Set as bitmap type */
    info.fType= MFT_BITMAP;
    info.dwTypeData= (char *)0xdeadbee2; 
    rc = SetMenuItemInfoA( hmenu, 0, TRUE, &info );
    ok (!rc, "SetMenuItemInfo unexpectedly worked\n");

    /* Just change ftype back and ensure data hasn't been freed */
    info.fType= MFT_OWNERDRAW; /* Set as ownerdraw type */
    info.dwTypeData= (char *)0xdeadbee3; 
    rc = SetMenuItemInfoA( hmenu, 0, TRUE, &info );
    ok (rc, "SetMenuItemInfo failed\n");

    /* Did we keep the old dwTypeData? */
    ok (GetMenuStringA( hmenu, 0, strback, 99, MF_BYPOSITION), "GetMenuString on ownerdraw entry failed\n");
    ok (!strcmp( strback, "Dummy string" ), "Menu text from Ansi version incorrect\n");

    /* Just change string value (not type) */
    memset(&info, 0x00, sizeof(info));
    info.cbSize= sizeof(MENUITEMINFOA);
    info.fMask= MIIM_STRING; /* Set typeData */
    strcpy(string2, "string2");
    info.dwTypeData= string2;
    rc = SetMenuItemInfoA( hmenu, 0, TRUE, &info );
    ok (rc, "SetMenuItemInfo failed\n");

    ok (GetMenuStringA( hmenu, 0, strback, 99, MF_BYPOSITION), "GetMenuString on ownerdraw entry failed\n");
    ok (!strcmp( strback, "string2" ), "Menu text from Ansi version incorrect\n");

    /*  crashes with wine 0.9.5 */
    memset(&info, 0x00, sizeof(info));
    info.cbSize= sizeof(MENUITEMINFOA);
    info.fMask= MIIM_FTYPE | MIIM_STRING; /* Set OwnerDraw + typeData */
    info.fType= MFT_OWNERDRAW;
    rc = InsertMenuItemA( hmenu, 0, TRUE, &info );
    ok (rc, "InsertMenuItem failed\n");
    ok (!GetMenuStringA( hmenu, 0, NULL, 0, MF_BYPOSITION),
            "GetMenuString on ownerdraw entry succeeded.\n");
    ret = GetMenuStringW( hmenu, 0, NULL, 0, MF_BYPOSITION);
    ok (!ret, "GetMenuStringW on ownerdraw entry succeeded.\n");

    DestroyMenu( hmenu );
}

static void insert_menu_item( int line, HMENU hmenu, BOOL ansi, UINT mask, UINT type, UINT state, UINT id,
                              HMENU submenu, HBITMAP checked, HBITMAP unchecked, ULONG_PTR data,
                              void *type_data, UINT len, HBITMAP item, BOOL expect )
{
    MENUITEMINFOA info;
    BOOL ret;

    /* magic bitmap handle to test smaller cbSize */
    if (item == (HBITMAP)(ULONG_PTR)0xdeadbeef)
        info.cbSize = FIELD_OFFSET(MENUITEMINFOA,hbmpItem);
    else
        info.cbSize = sizeof(info);
    info.fMask = mask;
    info.fType = type;
    info.fState = state;
    info.wID = id;
    info.hSubMenu = submenu;
    info.hbmpChecked = checked;
    info.hbmpUnchecked = unchecked;
    info.dwItemData = data;
    info.dwTypeData = type_data;
    info.cch = len;
    info.hbmpItem = item;
    SetLastError( 0xdeadbeef );
    if (ansi) ret = InsertMenuItemA( hmenu, 0, TRUE, &info );
    else ret = InsertMenuItemW( hmenu, 0, TRUE, (MENUITEMINFOW*)&info );
    if (!expect) ok_(__FILE__, line)( !ret, "InsertMenuItem should have failed.\n" );
    else ok_(__FILE__, line)( ret, "InsertMenuItem failed, err %lu\n", GetLastError());
}

static void check_menu_item_info( int line, HMENU hmenu, BOOL ansi, UINT mask, UINT type, UINT state,
                                  UINT id, HMENU submenu, HBITMAP checked, HBITMAP unchecked,
                                  ULONG_PTR data, void *type_data, UINT in_len, UINT out_len,
                                  HBITMAP item, LPCSTR expname, BOOL expect, BOOL expstring )
{
    MENUITEMINFOA info;
    BOOL ret;
    WCHAR buffer[80];

    SetLastError( 0xdeadbeef );
    memset( &info, 0xcc, sizeof(info) );
    info.cbSize = sizeof(info);
    info.fMask = mask;
    info.dwTypeData = type_data;
    info.cch = in_len;

    ret = ansi ? GetMenuItemInfoA( hmenu, 0, TRUE, &info ) :
                 GetMenuItemInfoW( hmenu, 0, TRUE, (MENUITEMINFOW *)&info );
    if (!expect)
    {
        ok_(__FILE__, line)( !ret, "GetMenuItemInfo should have failed.\n" );
        return;
    }
    ok_(__FILE__, line)( ret, "GetMenuItemInfo failed, err %lu\n", GetLastError());
    if (mask & MIIM_TYPE)
        ok_(__FILE__, line)( info.fType == type || info.fType == LOWORD(type),
                             "wrong type %x/%x\n", info.fType, type );
    if (mask & MIIM_STATE)
        ok_(__FILE__, line)( info.fState == state || info.fState == LOWORD(state),
                             "wrong state %x/%x\n", info.fState, state );
    if (mask & MIIM_ID)
        ok_(__FILE__, line)( info.wID == id || info.wID == LOWORD(id),
                             "wrong id %x/%x\n", info.wID, id );
    if (mask & MIIM_SUBMENU)
        ok_(__FILE__, line)( info.hSubMenu == submenu || (ULONG_PTR)info.hSubMenu == LOWORD(submenu),
                             "wrong submenu %p/%p\n", info.hSubMenu, submenu );
    if (mask & MIIM_CHECKMARKS)
    {
        ok_(__FILE__, line)( info.hbmpChecked == checked || (ULONG_PTR)info.hbmpChecked == LOWORD(checked),
                             "wrong bmpchecked %p/%p\n", info.hbmpChecked, checked );
        ok_(__FILE__, line)( info.hbmpUnchecked == unchecked || (ULONG_PTR)info.hbmpUnchecked == LOWORD(unchecked),
                             "wrong bmpunchecked %p/%p\n", info.hbmpUnchecked, unchecked );
    }
    if (mask & MIIM_DATA)
        ok_(__FILE__, line)( info.dwItemData == data || info.dwItemData == LOWORD(data),
                             "wrong item data %Ix/%Ix\n", info.dwItemData, data );
    if (mask & MIIM_BITMAP)
        ok_(__FILE__, line)( info.hbmpItem == item || (ULONG_PTR)info.hbmpItem == LOWORD(item),
                             "wrong bmpitem %p/%p\n", info.hbmpItem, item );
    ok_(__FILE__, line)( info.dwTypeData == type_data || (ULONG_PTR)info.dwTypeData == LOWORD(type_data),
                         "wrong type data %p/%p\n", info.dwTypeData, type_data );
    ok_(__FILE__, line)( info.cch == out_len ||
                         broken(! ansi && info.cch == 2 * out_len) /* East-Asian */,
                         "wrong len %x/%x\n", info.cch, out_len );
    if (expname)
    {
        if(ansi)
            ok_(__FILE__, line)( !strncmp( expname, info.dwTypeData, out_len ),
                                 "menu item name differed from '%s' '%s'\n", expname, info.dwTypeData );
        else
            ok_(__FILE__, line)( !wcsncmp( (WCHAR *)expname, (WCHAR *)info.dwTypeData, out_len ),
                                 "menu item name wrong\n" );

        SetLastError( 0xdeadbeef );
        ret = ansi ? GetMenuStringA( hmenu, 0, (char *)buffer, 80, MF_BYPOSITION ) :
            GetMenuStringW( hmenu, 0, buffer, 80, MF_BYPOSITION );
        if (expstring)
            ok_(__FILE__, line)( ret, "GetMenuString failed, err %lu\n", GetLastError());
        else
            ok_(__FILE__, line)( !ret, "GetMenuString should have failed\n" );
    }
}

static void modify_menu( int line, HMENU hmenu, BOOL ansi, UINT flags, UINT_PTR id, void *data )
{
    BOOL ret;

    SetLastError( 0xdeadbeef );
    if (ansi) ret = ModifyMenuA( hmenu, 0, flags, id, data );
    else ret = ModifyMenuW( hmenu, 0, flags, id, data );
    ok_(__FILE__,line)( ret, "ModifyMenuA failed, err %lu\n", GetLastError());
}

static void set_menu_item_info( int line, HMENU hmenu, BOOL ansi, UINT mask, UINT type, UINT state,
                                UINT id, HMENU submenu, HBITMAP checked, HBITMAP unchecked, ULONG_PTR data,
                                void *type_data, UINT len, HBITMAP item )

{
    MENUITEMINFOA info;
    BOOL ret;

    /* magic bitmap handle to test smaller cbSize */
    if (item == (HBITMAP)(ULONG_PTR)0xdeadbeef)
        info.cbSize = FIELD_OFFSET(MENUITEMINFOA,hbmpItem);
    else
        info.cbSize = sizeof(info);
    info.fMask = mask;
    info.fType = type;
    info.fState = state;
    info.wID = id;
    info.hSubMenu = submenu;
    info.hbmpChecked = checked;
    info.hbmpUnchecked = unchecked;
    info.dwItemData = data;
    info.dwTypeData = type_data;
    info.cch = len;
    info.hbmpItem = item;
    SetLastError( 0xdeadbeef );
    if (ansi) ret = SetMenuItemInfoA( hmenu, 0, TRUE, &info );
    else ret = SetMenuItemInfoW( hmenu, 0, TRUE, (MENUITEMINFOW*)&info );
    ok_(__FILE__, line)( ret, "SetMenuItemInfo failed, err %lu\n", GetLastError());
}

#define TMII_INSMI( c1,d1,e1,f1,g1,h1,i1,j1,k1,l1,m1,eret1 )\
    hmenu = CreateMenu();\
    submenu = CreateMenu();\
    if(ansi)strcpy( string, init );\
    else wcscpy( string, init );\
    insert_menu_item( __LINE__, hmenu, ansi, c1, d1, e1, f1, g1, h1, i1, j1, k1, l1, m1, eret1 )

/* GetMenuItemInfo + GetMenuString  */
#define TMII_GMII( c2,l2,\
    d3,e3,f3,g3,h3,i3,j3,k3,l3,m3,\
    expname, eret2, eret3)\
    check_menu_item_info( __LINE__, hmenu, ansi, c2, d3, e3, f3, g3, h3, i3, j3, k3, l2, l3, m3, \
                          expname, eret2, eret3 )

#define TMII_DONE \
    RemoveMenu(hmenu, 0, TRUE );\
    DestroyMenu( hmenu );\
    DestroyMenu( submenu );

/* modify menu */
#define TMII_MODM( flags, id, data ) \
    modify_menu( __LINE__, hmenu, ansi, flags, id, data )

/* SetMenuItemInfo */
#define TMII_SMII( c1,d1,e1,f1,g1,h1,i1,j1,k1,l1,m1 ) \
    set_menu_item_info( __LINE__, hmenu, ansi, c1, d1, e1, f1, g1, h1, i1, j1, k1, l1, m1 )


#define OK 1
#define ER 0


static void test_menu_iteminfo( void )
{
  BOOL ansi = TRUE;
  char txtA[]="wine";
  char initA[]="XYZ";
  char emptyA[]="";
  WCHAR txtW[]={'W','i','n','e',0};
  WCHAR initW[]={'X','Y','Z',0};
  WCHAR emptyW[]={0};
  void *txt, *init, *empty, *string;
  HBITMAP hbm = CreateBitmap(1,1,1,1,NULL);
  char stringA[0x80];
  HMENU hmenu, submenu;
  HBITMAP dummy_hbm = (HBITMAP)(ULONG_PTR)0xdeadbeef;

  do {
    if( ansi) {txt=txtA;init=initA;empty=emptyA;string=stringA;}
    else {txt=txtW;init=initW;empty=emptyW;string=stringA;}
    trace( "%s string %p hbm %p txt %p\n", ansi ?  "ANSI tests:   " : "Unicode tests:", string, hbm, txt);
    /* test all combinations of MFT_STRING, MFT_OWNERDRAW and MFT_BITMAP */
    /* (since MFT_STRING is zero, there are four of them) */
    TMII_INSMI( MIIM_TYPE, MFT_STRING, 0, 0, 0, 0, 0, 0, txt, 0, 0, OK );
    TMII_GMII ( MIIM_TYPE, 80,
        MFT_STRING, 0, 0, 0, 0, 0, 0, string, 4, 0,
        txt, OK, OK );
    TMII_DONE
    TMII_INSMI( MIIM_TYPE, MFT_STRING|MFT_OWNERDRAW, -1, -1, 0, 0, 0, -1, txt, 0, 0, OK );
    TMII_GMII ( MIIM_TYPE, 80,
        MFT_STRING|MFT_OWNERDRAW, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        NULL, OK, ER );
    TMII_DONE
    TMII_INSMI( MIIM_TYPE, MFT_BITMAP, -1, -1, 0, 0, 0, -1, hbm, 6, 0, OK );
    TMII_GMII ( MIIM_TYPE, 80,
        MFT_BITMAP, 0, 0, 0, 0, 0, 0, hbm, 0, hbm,
        NULL, OK, ER );
    TMII_DONE
    TMII_INSMI( MIIM_TYPE, MFT_BITMAP|MFT_OWNERDRAW, -1, -1, 0, 0, 0, -1, hbm, 6, 0, OK );
    TMII_GMII ( MIIM_TYPE, 80,
        MFT_BITMAP|MFT_OWNERDRAW, 0, 0, 0, 0, 0, 0, hbm, 0, hbm,
        NULL, OK, ER );
    TMII_DONE
    /* not enough space for name*/
    TMII_INSMI( MIIM_TYPE, MFT_STRING, -1, -1, 0, 0, 0, -1, txt, 6, 0, OK );
    TMII_GMII ( MIIM_TYPE, 0,
        MFT_STRING, 0, 0, 0, 0, 0, 0, NULL, 4, 0,
        NULL, OK, OK );
    TMII_DONE
    TMII_INSMI( MIIM_TYPE, MFT_STRING, -1, -1, 0, 0, 0, -1, txt, 6, 0, OK );
    TMII_GMII ( MIIM_TYPE, 5,
        MFT_STRING, 0, 0, 0, 0, 0, 0, string, 4, 0,
        txt, OK, OK );
    TMII_DONE
    TMII_INSMI( MIIM_TYPE, MFT_STRING, -1, -1, 0, 0, 0, -1, txt, 6, 0, OK );
    TMII_GMII ( MIIM_TYPE, 4,
        MFT_STRING, 0, 0, 0, 0, 0, 0, string, 3, 0,
        txt, OK, OK );
    TMII_DONE
    TMII_INSMI( MIIM_FTYPE|MIIM_STRING, MFT_OWNERDRAW, -1, -1, 0, 0, 0, -1, NULL, 0, 0, OK );
    TMII_GMII ( MIIM_TYPE, 0,
        MFT_OWNERDRAW, 0, 0, 0, 0, 0, 0, NULL, 0, 0,
        NULL, OK, ER );
    TMII_DONE
    /* cannot combine MIIM_TYPE with some other flags */
    TMII_INSMI( MIIM_TYPE|MIIM_STRING, MFT_STRING, -1, -1, 0, 0, 0, -1, txt, 6, 0, ER );
    TMII_DONE
    TMII_INSMI( MIIM_TYPE, MFT_STRING, -1, -1, 0, 0, 0, -1, txt, 6, 0, OK );
    TMII_GMII ( MIIM_TYPE|MIIM_STRING, 80,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        NULL, ER, OK );
    TMII_DONE
    TMII_INSMI( MIIM_TYPE|MIIM_FTYPE, MFT_STRING, -1, -1, 0, 0, 0, -1, txt, 6, 0, ER );
    TMII_DONE
    TMII_INSMI( MIIM_TYPE, MFT_STRING, -1, -1, 0, 0, 0, -1, txt, 6, 0, OK );
    TMII_GMII ( MIIM_TYPE|MIIM_FTYPE, 80,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        NULL, ER, OK );
    TMII_DONE
    TMII_INSMI( MIIM_TYPE|MIIM_BITMAP, MFT_BITMAP, -1, -1, 0, 0, 0, -1, hbm, 6, hbm, ER );
    TMII_DONE
        /* but succeeds with some others */
    TMII_INSMI( MIIM_TYPE, MFT_STRING, -1, -1, 0, 0, 0, -1, txt, 6, 0, OK );
    TMII_GMII ( MIIM_TYPE|MIIM_SUBMENU, 80,
        MFT_STRING, 0, 0, 0, 0, 0, 0, string, 4, 0,
        txt, OK, OK );
    TMII_DONE
    TMII_INSMI( MIIM_TYPE, MFT_STRING, -1, -1, 0, 0, 0, -1, txt, 6, 0, OK );
    TMII_GMII ( MIIM_TYPE|MIIM_STATE, 80,
        MFT_STRING, 0, 0, 0, 0, 0, 0, string, 4, 0,
        txt, OK, OK );
    TMII_DONE
    TMII_INSMI( MIIM_TYPE|MIIM_ID, MFT_STRING, -1, 888, 0, 0, 0, -1, txt, 6, 0, OK );
    TMII_GMII ( MIIM_TYPE|MIIM_ID, 80,
        MFT_STRING, 0, 888, 0, 0, 0, 0, string, 4, 0,
        txt, OK, OK );
    TMII_DONE
    TMII_INSMI( MIIM_TYPE|MIIM_DATA, MFT_STRING, -1, -1, 0, 0, 0, 999, txt, 6, 0, OK );
    TMII_GMII ( MIIM_TYPE|MIIM_DATA, 80,
        MFT_STRING, 0, 0, 0, 0, 0, 999, string, 4, 0,
        txt, OK, OK );
    TMII_DONE
    /* to be continued */
    /* set text with MIIM_TYPE and retrieve with MIIM_STRING */ 
    TMII_INSMI( MIIM_TYPE, MFT_STRING, -1, -1, 0, 0, 0, -1, txt, 6, 0, OK );
    TMII_GMII ( MIIM_STRING|MIIM_FTYPE, 80,
        MFT_STRING, 0, 0, 0, 0, 0, 0, string, 4, 0,
        txt, OK, OK );
    TMII_DONE
    /* set text with MIIM_TYPE and retrieve with MIIM_STRING; MFT_OWNERDRAW causes an empty string */ 
    TMII_INSMI( MIIM_TYPE, MFT_STRING|MFT_OWNERDRAW, -1, -1, 0, 0, 0, -1, txt, 6, 0, OK );
    TMII_GMII ( MIIM_STRING|MIIM_FTYPE, 80,
        MFT_STRING|MFT_OWNERDRAW, 0, 0, 0, 0, 0, 0, string, 0, 0,
        empty, OK, ER );
    TMII_DONE
    TMII_INSMI( MIIM_TYPE, MFT_STRING|MFT_OWNERDRAW, -1, -1, 0, 0, 0, -1, NULL, 0, 0, OK );
    TMII_GMII ( MIIM_STRING|MIIM_FTYPE, 80,
        MFT_OWNERDRAW, 0, 0, 0, 0, 0, 0, string, 0, 0,
        empty, OK, ER );
    TMII_DONE
    TMII_INSMI( MIIM_TYPE, MFT_STRING|MFT_OWNERDRAW, -1, -1, 0, 0, 0, -1, NULL, 0, 0, OK );
    TMII_GMII ( MIIM_FTYPE, 80,
        MFT_OWNERDRAW, 0, 0, 0, 0, 0, 0, string, 80, 0,
        init, OK, ER );
    TMII_DONE
    TMII_INSMI( MIIM_TYPE, MFT_STRING, -1, -1, 0, 0, 0, -1, txt, 0, 0, OK );
    TMII_GMII ( 0, 80,
        0, 0, 0, 0, 0, 0, 0, string, 80, 0,
        init, OK, OK );
    TMII_DONE
    /* contrary to MIIM_TYPE,you can set the text for an owner draw menu */ 
    TMII_INSMI( MIIM_STRING|MIIM_FTYPE, MFT_STRING|MFT_OWNERDRAW, -1, -1, 0, 0, 0, -1, txt, 0, 0, OK );
    TMII_GMII ( MIIM_STRING|MIIM_FTYPE, 80,
        MFT_OWNERDRAW, 0, 0, 0, 0, 0, 0, string, 4, 0,
        txt, OK, OK );
    TMII_DONE
    /* same but retrieve with MIIM_TYPE */ 
    TMII_INSMI( MIIM_STRING|MIIM_FTYPE, MFT_STRING|MFT_OWNERDRAW, -1, -1, 0, 0, 0, -1, txt, 0, 0, OK );
    TMII_GMII ( MIIM_TYPE, 80,
        MFT_OWNERDRAW, 0, 0, 0, 0, 0, 0, NULL, 4, NULL,
        NULL, OK, OK );
    TMII_DONE
    TMII_INSMI( MIIM_STRING|MIIM_FTYPE, MFT_STRING|MFT_OWNERDRAW, -1, -1, 0, 0, 0, -1, NULL, 0, 0, OK );
    TMII_GMII ( MIIM_STRING|MIIM_FTYPE, 80,
        MFT_OWNERDRAW, 0, 0, 0, 0, 0, 0, string, 0, 0,
        empty, OK, ER );
    TMII_DONE
    TMII_INSMI( MIIM_STRING|MIIM_FTYPE, MFT_STRING, -1, -1, 0, 0, 0, -1, NULL, 0, 0, OK );
    TMII_GMII ( MIIM_STRING|MIIM_FTYPE, 80,
        MFT_SEPARATOR, 0, 0, 0, 0, 0, 0, string, 0, 0,
        empty, OK, ER );
    TMII_DONE
    TMII_INSMI( MIIM_STRING|MIIM_FTYPE, MFT_STRING, -1, -1, 0, 0, 0, -1, NULL, 0, 0, OK );
    TMII_GMII ( MIIM_STRING|MIIM_FTYPE, 80,
        MFT_SEPARATOR, 0, 0, 0, 0, 0, 0, NULL, 0, 0,
        NULL, OK, ER );
    TMII_DONE

    /* How is that with bitmaps? */ 
    TMII_INSMI( MIIM_BITMAP, -1, -1, -1, 0, 0, 0, -1, 0, -1, hbm, OK );
    TMII_GMII ( MIIM_TYPE, 80,
        MFT_BITMAP, 0, 0, 0, 0, 0, 0, hbm, 0, hbm,
        NULL, OK, ER );
    TMII_DONE
    TMII_INSMI( MIIM_BITMAP, -1, -1, -1, 0, 0, 0, -1, 0, -1, hbm, OK );
    TMII_GMII ( MIIM_BITMAP|MIIM_FTYPE, 80,
        0, 0, 0, 0, 0, 0, 0, string, 80, hbm,
        init, OK, ER );
    TMII_DONE
        /* MIIM_BITMAP does not like MFT_BITMAP */
    TMII_INSMI( MIIM_BITMAP|MIIM_FTYPE, MFT_BITMAP, -1, -1, 0, 0, 0, -1, 0, -1, hbm, ER );
    TMII_DONE
        /* no problem with OWNERDRAWN */
    TMII_INSMI( MIIM_BITMAP|MIIM_FTYPE, MFT_OWNERDRAW, -1, -1, 0, 0, 0, -1, 0, -1, hbm, OK );
    TMII_GMII ( MIIM_BITMAP|MIIM_FTYPE, 80,
        MFT_OWNERDRAW, 0, 0, 0, 0, 0, 0, string, 80, hbm,
        init, OK, ER );
    TMII_DONE
        /* setting MFT_BITMAP with MFT_FTYPE fails anyway */
    TMII_INSMI( MIIM_FTYPE, MFT_BITMAP, -1, -1, 0, 0, 0, -1, 0, -1, 0, ER );
    TMII_DONE

    /* menu with submenu */
    TMII_INSMI( MIIM_SUBMENU|MIIM_FTYPE, MFT_STRING, -1, -1, submenu, 0, 0, -1, txt, 0, 0, OK );
    TMII_GMII ( MIIM_SUBMENU, 80,
        0, 0, 0, submenu, 0, 0, 0, string, 80, 0,
        init, OK, ER );
    TMII_DONE
    TMII_INSMI( MIIM_SUBMENU|MIIM_FTYPE, MFT_STRING, -1, -1, submenu, 0, 0, -1, empty, 0, 0, OK );
    TMII_GMII ( MIIM_SUBMENU, 80,
        0, 0, 0, submenu, 0, 0, 0, string, 80, 0,
        init, OK, ER );
    TMII_DONE
    /* menu with submenu, without MIIM_SUBMENU the submenufield is cleared */
    TMII_INSMI( MIIM_SUBMENU|MIIM_FTYPE, MFT_STRING, -1, -1, submenu, 0, 0, -1, txt, 0, 0, OK );
    TMII_GMII ( MIIM_STRING|MIIM_FTYPE, 80,
        MFT_STRING|MFT_SEPARATOR, 0, 0, 0, 0, 0, 0, string, 0, 0,
        empty, OK, ER );
    TMII_GMII ( MIIM_SUBMENU|MIIM_FTYPE, 80,
        MFT_SEPARATOR, 0, 0, submenu, 0, 0, 0, string, 80, 0,
        empty, OK, ER );
    TMII_DONE
    /* menu with invalid submenu */
    TMII_INSMI( MIIM_SUBMENU|MIIM_FTYPE, MFT_STRING, -1, -1, (HMENU)999, 0, 0, -1, txt, 0, 0, ER );
    TMII_DONE
    /* Separator */
    TMII_INSMI( MIIM_TYPE, MFT_SEPARATOR, 0, 0, 0, 0, 0, 0, txt, 0, 0, OK );
    TMII_GMII ( MIIM_TYPE, 80,
        MFT_SEPARATOR, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        NULL, OK, ER );
    TMII_DONE
    TMII_INSMI( MIIM_TYPE, MFT_BITMAP|MFT_SEPARATOR, -1, -1, 0, 0, 0, -1, hbm, 6, 0, OK );
    TMII_GMII ( MIIM_TYPE, 80,
        MFT_BITMAP|MFT_SEPARATOR, 0, 0, 0, 0, 0, 0, hbm, 0, hbm,
        NULL, OK, ER );
    TMII_DONE
     /* SEPARATOR and STRING go well together */
    /* BITMAP and STRING go well together */
    TMII_INSMI( MIIM_STRING|MIIM_BITMAP, -1, -1, -1, 0, 0, 0, -1, txt, 6, hbm, OK );
    TMII_GMII ( MIIM_FTYPE|MIIM_STRING|MIIM_BITMAP, 80,
        MFT_STRING, 0, 0, 0, 0, 0, 0, string, 4, hbm,
        txt, OK, OK );
    TMII_DONE
     /* BITMAP, SEPARATOR and STRING go well together */
    TMII_INSMI( MIIM_FTYPE|MIIM_STRING|MIIM_BITMAP, MFT_SEPARATOR, -1, -1, 0, 0, 0, -1, txt, 6, hbm, OK );
    TMII_GMII ( MIIM_FTYPE|MIIM_STRING|MIIM_BITMAP, 80,
        MFT_SEPARATOR, 0, 0, 0, 0, 0, 0, string, 4, hbm,
        txt, OK, OK );
    TMII_DONE
     /* last two tests, but use MIIM_TYPE to retrieve info */
    TMII_INSMI( MIIM_FTYPE|MIIM_STRING, MFT_SEPARATOR, -1, -1, 0, 0, 0, -1, txt, 6, 0, OK );
    TMII_GMII ( MIIM_TYPE, 80,
        MFT_SEPARATOR, 0, 0, 0, 0, 0, 0, NULL, 4, NULL,
        NULL, OK, OK );
    TMII_DONE
    TMII_INSMI( MIIM_STRING|MIIM_BITMAP, -1, -1, -1, 0, 0, 0, -1, txt, 6, hbm, OK );
    TMII_GMII ( MIIM_TYPE, 80,
        MFT_BITMAP, 0, 0, 0, 0, 0, 0, hbm, 4, hbm,
        NULL, OK, OK );
    TMII_DONE
    TMII_INSMI( MIIM_FTYPE|MIIM_STRING|MIIM_BITMAP, MFT_SEPARATOR, -1, -1, 0, 0, 0, -1, txt, 6, hbm, OK );
    TMII_GMII ( MIIM_TYPE, 80,
        MFT_SEPARATOR|MFT_BITMAP, 0, 0, 0, 0, 0, 0, hbm, 4, hbm,
        NULL, OK, OK );
    TMII_DONE
     /* same three with MFT_OWNERDRAW */
    TMII_INSMI( MIIM_FTYPE|MIIM_STRING, MFT_SEPARATOR|MFT_OWNERDRAW, -1, -1, 0, 0, 0, -1, txt, 6, 0, OK );
    TMII_GMII ( MIIM_TYPE, 80,
        MFT_SEPARATOR|MFT_OWNERDRAW, 0, 0, 0, 0, 0, 0, NULL, 4, NULL,
        NULL, OK, OK );
    TMII_DONE
    TMII_INSMI( MIIM_FTYPE|MIIM_STRING|MIIM_BITMAP, MFT_OWNERDRAW, -1, -1, 0, 0, 0, -1, txt, 6, hbm, OK );
    TMII_GMII ( MIIM_TYPE, 80,
        MFT_BITMAP|MFT_OWNERDRAW, 0, 0, 0, 0, 0, 0, hbm, 4, hbm,
        NULL, OK, OK );
    TMII_DONE
    TMII_INSMI( MIIM_FTYPE|MIIM_STRING|MIIM_BITMAP, MFT_SEPARATOR|MFT_OWNERDRAW, -1, -1, 0, 0, 0, -1, txt, 6, hbm, OK );
    TMII_GMII ( MIIM_TYPE, 80,
        MFT_SEPARATOR|MFT_BITMAP|MFT_OWNERDRAW, 0, 0, 0, 0, 0, 0, hbm, 4, hbm,
        NULL, OK, OK );
    TMII_DONE

    TMII_INSMI( MIIM_STRING|MIIM_FTYPE|MIIM_ID, MFT_STRING|MFT_OWNERDRAW, -1, -1, 0, 0, 0, -1, txt, 0, 0, OK );
    TMII_GMII ( MIIM_TYPE, 80,
        MFT_OWNERDRAW, 0, 0, 0, 0, 0, 0, NULL, 4, NULL,
        NULL,  OK, OK );
    TMII_DONE
    /* test with modifymenu: string is preserved after setting OWNERDRAW */
    TMII_INSMI( MIIM_STRING, MFT_STRING, -1, -1, 0, 0, 0, -1, txt, 0, 0, OK );
    TMII_MODM( MFT_OWNERDRAW, -1, (void*)787 );
    TMII_GMII ( MIIM_FTYPE|MIIM_STRING|MIIM_DATA, 80,
        MFT_OWNERDRAW, 0, 0, 0, 0, 0, 787, string, 4, 0,
        txt,  OK, OK );
    TMII_DONE
    /* same with bitmap: now the text is cleared */
    TMII_INSMI( MIIM_STRING, MFT_STRING, -1, -1, 0, 0, 0, -1, txt, 0, 0, OK );
    TMII_MODM( MFT_BITMAP, 545, hbm );
    TMII_GMII ( MIIM_FTYPE|MIIM_STRING|MIIM_BITMAP|MIIM_ID, 80,
        MFT_BITMAP, 0, 545, 0, 0, 0, 0, string, 0, hbm,
        empty,  OK, ER );
    TMII_DONE
    /* start with bitmap: now setting text clears it (though he flag is raised) */
    TMII_INSMI( MIIM_BITMAP, MFT_STRING, -1, -1, 0, 0, 0, -1, 0, -1, hbm, OK );
    TMII_GMII ( MIIM_FTYPE|MIIM_STRING|MIIM_BITMAP|MIIM_ID, 80,
        MFT_STRING, 0, 0, 0, 0, 0, 0, string, 0, hbm,
        empty,  OK, ER );
    TMII_MODM( MFT_STRING, 545, txt );
    TMII_GMII ( MIIM_FTYPE|MIIM_STRING|MIIM_BITMAP|MIIM_ID, 80,
        MFT_STRING, 0, 545, 0, 0, 0, 0, string, 4, 0,
        txt,  OK, OK );
    TMII_DONE
    /*repeat with text NULL */
    TMII_INSMI( MIIM_BITMAP, MFT_STRING, -1, -1, 0, 0, 0, -1, 0, -1, hbm, OK );
    TMII_MODM( MFT_STRING, 545, NULL );
    TMII_GMII ( MIIM_FTYPE|MIIM_STRING|MIIM_BITMAP|MIIM_ID, 80,
        MFT_SEPARATOR, 0, 545, 0, 0, 0, 0, string, 0, 0,
        empty,  OK, ER );
    TMII_DONE
    /* repeat with text "" */
    TMII_INSMI( MIIM_BITMAP, -1 , -1, -1, 0, 0, 0, -1, 0, -1, hbm, OK );
    TMII_MODM( MFT_STRING, 545, empty );
    TMII_GMII ( MIIM_FTYPE|MIIM_STRING|MIIM_BITMAP|MIIM_ID, 80,
        MFT_STRING, 0, 545, 0, 0, 0, 0, string, 0, 0,
        empty,  OK, ER );
    TMII_DONE
    /* start with bitmap: set ownerdraw */
    TMII_INSMI( MIIM_BITMAP, -1, -1, -1, 0, 0, 0, -1, 0, -1, hbm, OK );
    TMII_MODM( MFT_OWNERDRAW, -1, (void *)232 );
    TMII_GMII ( MIIM_FTYPE|MIIM_STRING|MIIM_BITMAP|MIIM_DATA, 80,
        MFT_OWNERDRAW, 0, 0, 0, 0, 0, 232, string, 0, hbm,
        empty,  OK, ER );
    TMII_DONE
    /* ask nothing */
    TMII_INSMI( MIIM_FTYPE|MIIM_STRING|MIIM_BITMAP, MFT_SEPARATOR, -1, -1, 0, 0, 0, -1, txt, 6, hbm, OK );
    TMII_GMII ( 0, 80,
                0, 0, 0,  0, 0, 0, 0, string, 80, 0,
        init, OK, OK );
    TMII_DONE
    /* some tests with small cbSize: the hbmpItem is to be ignored */
    TMII_INSMI( MIIM_BITMAP, -1, -1, -1, 0, 0, 0, -1, 0, -1, dummy_hbm, OK );
    TMII_GMII ( MIIM_TYPE, 80,
        MFT_SEPARATOR, 0, 0, 0, 0, 0, 0, NULL, 0, NULL,
        NULL, OK, ER );
    TMII_DONE
    TMII_INSMI( MIIM_BITMAP, -1, -1, -1, 0, 0, 0, -1, 0, -1, dummy_hbm, OK );
    TMII_GMII ( MIIM_BITMAP|MIIM_FTYPE, 80,
        MFT_SEPARATOR, 0, 0, 0, 0, 0, 0, string, 80, NULL,
        init, OK, ER );
    TMII_DONE
    TMII_INSMI( MIIM_STRING|MIIM_BITMAP, -1, -1, -1, 0, 0, 0, -1, txt, 6, dummy_hbm, OK );
    TMII_GMII ( MIIM_TYPE, 80,
        MFT_STRING, 0, 0, 0, 0, 0, 0, string, 4, NULL,
        txt, OK, OK );
    TMII_DONE
    TMII_INSMI( MIIM_FTYPE|MIIM_STRING|MIIM_BITMAP, MFT_SEPARATOR, -1, -1, 0, 0, 0, -1, txt, 6, dummy_hbm, OK );
    TMII_GMII ( MIIM_TYPE, 80,
        MFT_SEPARATOR, 0, 0, 0, 0, 0, 0, NULL, 4, NULL,
        NULL, OK, OK );
    TMII_DONE
    TMII_INSMI( MIIM_FTYPE|MIIM_STRING|MIIM_BITMAP, MFT_OWNERDRAW, -1, -1, 0, 0, 0, -1, txt, 6, dummy_hbm, OK );
    TMII_GMII ( MIIM_TYPE, 80,
        MFT_OWNERDRAW, 0, 0, 0, 0, 0, 0, NULL, 4, NULL,
        NULL, OK, OK );
    TMII_DONE
    TMII_INSMI( MIIM_FTYPE|MIIM_STRING|MIIM_BITMAP, MFT_SEPARATOR|MFT_OWNERDRAW, -1, -1, 0, 0, 0, -1, txt, 6, dummy_hbm, OK );
    TMII_GMII ( MIIM_TYPE, 80,
        MFT_SEPARATOR|MFT_OWNERDRAW, 0, 0, 0, 0, 0, 0, NULL, 4, NULL,
        NULL, OK, OK );
    TMII_DONE
    /* MIIM_TYPE by itself does not get/set the dwItemData for OwnerDrawn menus  */
    TMII_INSMI( MIIM_TYPE|MIIM_DATA, MFT_STRING|MFT_OWNERDRAW, -1, -1, 0, 0, 0, 343, txt, 0, 0, OK );
    TMII_GMII ( MIIM_TYPE|MIIM_DATA, 80,
        MFT_STRING|MFT_OWNERDRAW, 0, 0, 0, 0, 0, 343, 0, 0, 0,
        NULL, OK, ER );
    TMII_DONE
    TMII_INSMI( MIIM_TYPE|MIIM_DATA, MFT_STRING|MFT_OWNERDRAW, -1, -1, 0, 0, 0, 343, txt, 0, 0, OK );
    TMII_GMII ( MIIM_TYPE, 80,
        MFT_STRING|MFT_OWNERDRAW, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        NULL, OK, ER );
    TMII_DONE
    TMII_INSMI( MIIM_TYPE, MFT_STRING|MFT_OWNERDRAW, -1, -1, 0, 0, 0, 343, txt, 0, 0, OK );
    TMII_GMII ( MIIM_TYPE|MIIM_DATA, 80,
        MFT_STRING|MFT_OWNERDRAW, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        NULL, OK, ER );
    TMII_DONE
    /* set a string menu to ownerdraw with MIIM_TYPE */
    TMII_INSMI( MIIM_TYPE, MFT_STRING, -2, -2, 0, 0, 0, -2, txt, -2, 0, OK );
    TMII_SMII ( MIIM_TYPE, MFT_OWNERDRAW, 0, 0, 0, 0, 0, 0, 0, 0, 0 );
    TMII_GMII ( MIIM_STRING|MIIM_FTYPE, 80,
        MFT_OWNERDRAW, 0, 0, 0, 0, 0, 0, string, 4, 0,
        txt, OK, OK );
    TMII_DONE
    /* test with modifymenu add submenu */
    TMII_INSMI( MIIM_STRING, MFT_STRING, -1, -1, 0, 0, 0, -1, txt, 0, 0, OK );
    TMII_MODM( MF_POPUP, (UINT_PTR)submenu, txt );
    TMII_GMII ( MIIM_FTYPE|MIIM_STRING|MIIM_SUBMENU, 80,
        MFT_STRING, 0, 0, submenu, 0, 0, 0, string, 4, 0,
        txt,  OK, OK );
    TMII_GMII ( MIIM_TYPE, 80,
        MFT_STRING, 0, 0, 0, 0, 0, 0, string, 4, 0,
        txt,  OK, OK );
    TMII_DONE
    /* MFT_SEPARATOR bit is kept when the text is added */
    TMII_INSMI( MIIM_STRING|MIIM_FTYPE, MFT_STRING, -1, -1, 0, 0, 0, -1, NULL, 0, 0, OK );
    TMII_SMII( MIIM_STRING, 0, 0, 0, 0, 0, 0, 0, txt, 0, 0 );
    TMII_GMII ( MIIM_STRING|MIIM_FTYPE, 80,
        MFT_SEPARATOR, 0, 0, 0, 0, 0, 0, string, 4, 0,
        txt, OK, OK );
    TMII_DONE
    /* MFT_SEPARATOR bit is kept when bitmap is added */
    TMII_INSMI( MIIM_STRING|MIIM_FTYPE, MFT_STRING, -1, -1, 0, 0, 0, -1, NULL, 0, 0, OK );
    TMII_SMII( MIIM_BITMAP, 0, 0, 0, 0, 0, 0, 0, 0, 0, hbm );
    TMII_GMII ( MIIM_BITMAP|MIIM_FTYPE, 80,
        MFT_SEPARATOR, 0, 0, 0, 0, 0, 0, string, 80, hbm,
        init, OK, ER );
    TMII_DONE
    /* Bitmaps inserted with MIIM_TYPE and MFT_BITMAP:
       Only the low word of the dwTypeData is used.
       Use a magic bitmap here (Word 95 uses this to create its MDI menu buttons) */
    TMII_INSMI( MIIM_TYPE, MFT_BITMAP | MFT_RIGHTJUSTIFY, -1, -1, 0, 0, 0, -1,
                (HMENU)MAKELPARAM(HBMMENU_MBAR_CLOSE, 0x1234), -1, 0, OK );
    TMII_GMII ( MIIM_TYPE, 80,
        MFT_BITMAP | MFT_RIGHTJUSTIFY, 0, 0, 0, 0, 0, 0, HBMMENU_MBAR_CLOSE, 0, HBMMENU_MBAR_CLOSE,
        NULL, OK, OK );
    TMII_DONE
    /* Type flags */
    TMII_INSMI( MIIM_TYPE, MFT_BITMAP | MFT_MENUBARBREAK | MFT_RADIOCHECK | MFT_RIGHTJUSTIFY | MFT_RIGHTORDER, -1, -1, 0, 0, 0, -1, hbm, -1, 0, OK );
    TMII_GMII ( MIIM_TYPE, 80,
        MFT_BITMAP | MFT_MENUBARBREAK | MFT_RADIOCHECK | MFT_RIGHTJUSTIFY | MFT_RIGHTORDER, 0, 0, 0, 0, 0, 0, hbm, 0, hbm,
        NULL, OK, OK );
    TMII_DONE
    /* State flags */
    TMII_INSMI( MIIM_TYPE, MFT_BITMAP, -1, -1, 0, 0, 0, -1, hbm, -1, 0, OK );
    TMII_SMII( MIIM_STATE, -1, MFS_CHECKED | MFS_DEFAULT | MFS_GRAYED | MFS_HILITE, 0, 0, 0, 0, 0, 0, 0, 0 );
    TMII_GMII ( MIIM_STATE, 80,
        0, MFS_CHECKED | MFS_DEFAULT | MFS_GRAYED | MFS_HILITE, 0, 0, 0, 0, 0, 0, 80, 0,
        NULL, OK, OK );
    TMII_DONE
    /* The style MFT_RADIOCHECK cannot be set with MIIM_CHECKMARKS only */
    TMII_INSMI( MIIM_TYPE, MFT_BITMAP, -1, -1, 0, 0, 0, -1, hbm, -1, 0, OK );
    TMII_SMII( MIIM_CHECKMARKS, MFT_RADIOCHECK, 0, 0, 0, hbm, hbm, 0, 0, 0, 0 );
    TMII_GMII ( MIIM_CHECKMARKS | MIIM_TYPE, 80,
        MFT_BITMAP, 0, 0, 0, hbm, hbm, 0, hbm, 0, hbm,
        NULL, OK, OK );
    TMII_DONE
    /* MFT_BITMAP is added automatically by GetMenuItemInfo() for MIIM_TYPE */
    TMII_INSMI( MIIM_TYPE, MFT_BITMAP, -1, -1, 0, 0, 0, -1, hbm, -1, 0, OK );
    TMII_SMII( MIIM_FTYPE, MFT_OWNERDRAW, 0, 0, 0, 0, 0, 0, (HBITMAP)0x1234, 0, 0 );
    TMII_GMII ( MIIM_FTYPE, 80,
        MFT_OWNERDRAW, 0, 0, 0, 0, 0, 0, 0, 80, 0,
        NULL, OK, OK );
    TMII_GMII ( MIIM_TYPE, 80,
        MFT_BITMAP | MFT_OWNERDRAW, 0, 0, 0, 0, 0, 0, hbm, 0, hbm,
        NULL, OK, OK );
    TMII_GMII ( MIIM_FTYPE, 80,
        MFT_OWNERDRAW, 0, 0, 0, 0, 0, 0, 0, 80, 0,
        NULL, OK, OK );
    TMII_SMII( MIIM_BITMAP, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL );
    TMII_GMII ( MIIM_TYPE, 80,
        MFT_OWNERDRAW, 0, 0, 0, 0, 0, 0, NULL, 0, NULL,
        NULL, OK, OK );
    TMII_DONE
    /* Bitmaps inserted with MIIM_TYPE and MFT_BITMAP:
       Only the low word of the dwTypeData is used.
       Use a magic bitmap here (Word 95 uses this to create its MDI menu buttons) */
    TMII_INSMI( MIIM_TYPE, MFT_BITMAP | MFT_RIGHTJUSTIFY, -1, -1, 0, 0, 0, -1,
                (HMENU)MAKELPARAM(HBMMENU_MBAR_CLOSE, 0x1234), -1, 0, OK );
    TMII_GMII ( MIIM_TYPE, 80,
        MFT_BITMAP | MFT_RIGHTJUSTIFY, 0, 0, 0, 0, 0, 0, HBMMENU_MBAR_CLOSE, 0, HBMMENU_MBAR_CLOSE,
        NULL, OK, OK );
    TMII_DONE
    /* Type flags */
    TMII_INSMI( MIIM_TYPE, MFT_BITMAP | MFT_MENUBARBREAK | MFT_RADIOCHECK | MFT_RIGHTJUSTIFY | MFT_RIGHTORDER, -1, -1, 0, 0, 0, -1, hbm, -1, 0, OK );
    TMII_GMII ( MIIM_TYPE, 80,
        MFT_BITMAP | MFT_MENUBARBREAK | MFT_RADIOCHECK | MFT_RIGHTJUSTIFY | MFT_RIGHTORDER, 0, 0, 0, 0, 0, 0, hbm, 0, hbm,
        NULL, OK, OK );
    TMII_DONE
    /* State flags */
    TMII_INSMI( MIIM_TYPE, MFT_BITMAP, -1, -1, 0, 0, 0, -1, hbm, -1, 0, OK );
    TMII_SMII( MIIM_STATE, -1, MFS_CHECKED | MFS_DEFAULT | MFS_GRAYED | MFS_HILITE, 0, 0, 0, 0, 0, 0, 0, 0 );
    TMII_GMII ( MIIM_STATE, 80,
        0, MFS_CHECKED | MFS_DEFAULT | MFS_GRAYED | MFS_HILITE, 0, 0, 0, 0, 0, 0, 80, 0,
        NULL, OK, OK );
    TMII_DONE
    /* The style MFT_RADIOCHECK cannot be set with MIIM_CHECKMARKS only */
    TMII_INSMI( MIIM_TYPE, MFT_BITMAP, -1, -1, 0, 0, 0, -1, hbm, -1, 0, OK );
    TMII_SMII( MIIM_CHECKMARKS, MFT_RADIOCHECK, 0, 0, 0, hbm, hbm, 0, 0, 0, 0 );
    TMII_GMII ( MIIM_CHECKMARKS | MIIM_TYPE, 80,
        MFT_BITMAP, 0, 0, 0, hbm, hbm, 0, hbm, 0, hbm,
        NULL, OK, OK );
    TMII_DONE
    /* MFT_BITMAP is added automatically by GetMenuItemInfo() for MIIM_TYPE */
    TMII_INSMI( MIIM_TYPE, MFT_BITMAP, -1, -1, 0, 0, 0, -1, hbm, -1, 0, OK );
    TMII_SMII( MIIM_FTYPE, MFT_OWNERDRAW, 0, 0, 0, 0, 0, 0, (HBITMAP)0x1234, 0, 0 );
    TMII_GMII ( MIIM_FTYPE, 80,
        MFT_OWNERDRAW, 0, 0, 0, 0, 0, 0, 0, 80, 0,
        NULL, OK, OK );
    TMII_GMII ( MIIM_TYPE, 80,
        MFT_BITMAP | MFT_OWNERDRAW, 0, 0, 0, 0, 0, 0, hbm, 0, hbm,
        NULL, OK, OK );
    TMII_GMII ( MIIM_FTYPE, 80,
        MFT_OWNERDRAW, 0, 0, 0, 0, 0, 0, 0, 80, 0,
        NULL, OK, OK );
    TMII_SMII( MIIM_BITMAP, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL );
    TMII_GMII ( MIIM_TYPE, 80,
        MFT_OWNERDRAW, 0, 0, 0, 0, 0, 0, NULL, 0, NULL,
        NULL, OK, OK );
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
    MENUITEMINFOA info;
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

    rc = InsertMenuItemA(hmenu, 0, TRUE, &info );
    ok (rc, "Inserting the menuitem failed\n");

    id = GetMenuItemID(hmenu, 0);
    ok (id == 0x1234, "Getting the menuitem id failed(gave %x)\n", id);

    /* Confirm the menuitem was given the id supplied (getting by position) */
    memset( &info, 0, sizeof info );
    strback[0] = 0x00;
    info.cbSize = sizeof(MENUITEMINFOA);
    info.fMask = MIIM_FTYPE | MIIM_ID | MIIM_STRING;
    info.dwTypeData = strback;
    info.cch = sizeof(strback);

    rc = GetMenuItemInfoA(hmenu, 0, TRUE, &info); /* Get by position */
    ok (rc, "Getting the menu items info failed\n");
    ok (info.wID == 0x1234, "IDs differ for the menuitem\n");
    ok (!strcmp(info.dwTypeData, "Case 1 MenuItem"), "Returned item has wrong label\n");

    /* Search by id - Should return the item */
    memset( &info, 0, sizeof info );
    strback[0] = 0x00;
    info.cbSize = sizeof(MENUITEMINFOA);
    info.fMask = MIIM_FTYPE | MIIM_ID | MIIM_STRING;
    info.dwTypeData = strback;
    info.cch = sizeof(strback);
    rc = GetMenuItemInfoA(hmenu, 0x1234, FALSE, &info); /* Get by ID */

    ok (rc, "Getting the menu items info failed\n");
    ok (info.wID == 0x1234, "IDs differ for the menuitem\n");
    ok (!strcmp(info.dwTypeData, "Case 1 MenuItem"), "Returned item has wrong label\n");

    DestroyMenu( hmenu );

    /* Case 2: Menu containing a popup menu */
    hmenu = CreateMenu();
    hmenuSub = CreateMenu();

    strcpy(strIn, "Case 2 SubMenu");
    rc = InsertMenuA(hmenu, 0, MF_BYPOSITION | MF_POPUP | MF_STRING, (UINT_PTR)hmenuSub, strIn);
    ok (rc, "Inserting the popup menu into the main menu failed\n");

    id = GetMenuItemID(hmenu, 0);
    ok (id == -1, "Getting the menuitem id unexpectedly worked (gave %x)\n", id);

    /* Confirm the menuitem itself was given an id the same as the HMENU, (getting by position) */
    memset( &info, 0, sizeof info );
    strback[0] = 0x00;
    info.cbSize = sizeof(MENUITEMINFOA);
    info.fMask = MIIM_FTYPE | MIIM_ID | MIIM_STRING;
    info.dwTypeData = strback;
    info.cch = sizeof(strback);
    info.wID = 0xdeadbeef;

    rc = GetMenuItemInfoA(hmenu, 0, TRUE, &info); /* Get by position */
    ok (rc, "Getting the menu items info failed\n");
    ok (info.wID == (UINT_PTR)hmenuSub, "IDs differ for the menuitem\n");
    ok (!strcmp(info.dwTypeData, "Case 2 SubMenu"), "Returned item has wrong label\n");

    /* Search by id - returns the popup menu itself */
    memset( &info, 0, sizeof info );
    strback[0] = 0x00;
    info.cbSize = sizeof(MENUITEMINFOA);
    info.fMask = MIIM_FTYPE | MIIM_ID | MIIM_STRING;
    info.dwTypeData = strback;
    info.cch = sizeof(strback);
    rc = GetMenuItemInfoA(hmenu, (UINT_PTR)hmenuSub, FALSE, &info); /* Get by ID */

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
    rc = InsertMenuItemA(hmenu, -1, TRUE, &info );
    ok (rc, "Inserting the menuitem failed\n");

    /* Search by id - returns the item which follows the popup menu */
    memset( &info, 0, sizeof info );
    strback[0] = 0x00;
    info.cbSize = sizeof(MENUITEMINFOA);
    info.fMask = MIIM_FTYPE | MIIM_ID | MIIM_STRING;
    info.dwTypeData = strback;
    info.cch = sizeof(strback);
    rc = GetMenuItemInfoA(hmenu, (UINT_PTR)hmenuSub, FALSE, &info); /* Get by ID */

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
    rc = InsertMenuItemA(hmenu, 0, TRUE, &info );
    ok (rc, "Inserting the menuitem failed\n");

    /* Search by id - returns the item which precedes the popup menu */
    memset( &info, 0, sizeof info );
    strback[0] = 0x00;
    info.cbSize = sizeof(MENUITEMINFOA);
    info.fMask = MIIM_FTYPE | MIIM_ID | MIIM_STRING;
    info.dwTypeData = strback;
    info.cch = sizeof(strback);
    rc = GetMenuItemInfoA(hmenu, (UINT_PTR)hmenuSub, FALSE, &info); /* Get by ID */

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

    rc = InsertMenuA(hmenu, 0, MF_BYPOSITION | MF_POPUP | MF_STRING, (UINT_PTR)hmenuSub, "Submenu");
    ok (rc, "Inserting the popup menu into the main menu failed\n");

    rc = InsertMenuItemA(hmenuSub, 0, TRUE, &info );
    ok (rc, "Inserting the sub menu menuitem failed\n");

    memset( &info, 0, sizeof info );
    info.cbSize = sizeof info;
    info.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_ID;
    info.fType = MFT_STRING;
    info.dwTypeData = menuitem2;
    info.wID = (UINT_PTR) hmenuSub; /* Enforce id collisions with the hmenu of the popup submenu*/

    rc = InsertMenuItemA(hmenuSub, 1, TRUE, &info );
    ok (rc, "Inserting the sub menu menuitem 2 failed\n");

    /* Prove that you can't query the id of a popup directly (By position) */
    id = GetMenuItemID(hmenu, 0);
    ok (id == -1, "Getting the sub menu id should have failed because it's a popup (gave %x)\n", id);

    /* Prove getting the item info via ID returns the first item (not the popup or 2nd item)*/
    memset( &info, 0, sizeof info );
    strback[0] = 0x00;
    info.cbSize = sizeof(MENUITEMINFOA);
    info.fMask = MIIM_STRING | MIIM_ID;
    info.dwTypeData = strback;
    info.cch = sizeof(strback);

    rc = GetMenuItemInfoA(hmenu, (UINT_PTR)hmenuSub, FALSE, &info);
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

    rc = InsertMenuA(hmenu, 0, MF_BYPOSITION | MF_POPUP | MF_STRING, (UINT_PTR)hmenuSub, "Submenu");
    ok (rc, "Inserting the popup menu into the main menu failed\n");

    rc = InsertMenuA(hmenu, 1, MF_BYPOSITION | MF_POPUP | MF_STRING, (UINT_PTR)hmenuSub2, "Submenu2");
    ok (rc, "Inserting the popup menu into the main menu failed\n");

    memset( &info, 0, sizeof info );
    info.cbSize = sizeof info;
    info.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_ID;
    info.fType = MFT_STRING;
    info.dwTypeData = menuitem;
    info.wID = (UINT_PTR) hmenuSub; /* Enforce id collisions with the hmenu of the popup submenu*/

    rc = InsertMenuItemA(hmenuSub2, 0, TRUE, &info );
    ok (rc, "Inserting the sub menu menuitem failed\n");

    memset( &info, 0, sizeof info );
    info.cbSize = sizeof info;
    info.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_ID;
    info.fType = MFT_STRING;
    info.dwTypeData = menuitem2;
    info.wID = (UINT_PTR) hmenuSub; /* Enforce id collisions with the hmenu of the popup submenu*/

    rc = InsertMenuItemA(hmenuSub2, 1, TRUE, &info );
    ok (rc, "Inserting the sub menu menuitem 2 failed\n");

    /* Prove getting the item info via ID returns the first item (not the popup or 2nd item)*/
    memset( &info, 0, sizeof info );
    strback[0] = 0x00;
    info.cbSize = sizeof(MENUITEMINFOA);
    info.fMask = MIIM_STRING | MIIM_ID;
    info.dwTypeData = strback;
    info.cch = sizeof(strback);

    rc = GetMenuItemInfoA(hmenu, (UINT_PTR)hmenuSub, FALSE, &info);
    ok (rc, "Getting the menus info failed\n");
    ok (info.wID == (UINT_PTR)hmenuSub, "IDs differ for popup menu\n");
    ok (!strcmp(info.dwTypeData, "MenuItem"), "Returned item has wrong label (%s)\n", info.dwTypeData);

    memset( &info, 0, sizeof info );
    strback[0] = 0x00;
    info.cbSize = sizeof(MENUITEMINFOA);
    info.fMask = MIIM_STRING | MIIM_ID;
    info.dwTypeData = strback;
    info.cch = sizeof(strback);

    rc = GetMenuItemInfoA(hmenu, (UINT_PTR)hmenuSub2, FALSE, &info);
    ok (rc, "Getting the menus info failed\n");
    ok (info.wID == (UINT_PTR)hmenuSub2, "IDs differ for popup menu\n");
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

    rc = AppendMenuA(hmenu, MF_POPUP | MF_STRING, (UINT_PTR)hmenuSub, "Submenu");
    ok (rc, "Appending the popup menu to the main menu failed\n");

    rc = AppendMenuA(hmenuSub, MF_STRING, 102, "Item");
    ok (rc, "Appending the item to the popup menu failed\n");

    /* Set the ID for hmenuSub */
    info.cbSize = sizeof(info);
    info.fMask = MIIM_ID;
    info.wID = 101;

    rc = SetMenuItemInfoA(hmenu, 0, TRUE, &info);
    ok(rc, "Setting the ID for the popup menu failed\n");

    /* Check if the ID has been set */
    info.wID = 0;
    rc = GetMenuItemInfoA(hmenu, 0, TRUE, &info);
    ok(rc, "Getting the ID for the popup menu failed\n");
    ok(info.wID == 101, "The ID for the popup menu has not been set\n");

    /* Prove getting the item info via ID returns the popup menu */
    memset( &info, 0, sizeof(info));
    strback[0] = 0x00;
    info.cbSize = sizeof(MENUITEMINFOA);
    info.fMask = MIIM_STRING | MIIM_ID;
    info.dwTypeData = strback;
    info.cch = sizeof(strback);

    rc = GetMenuItemInfoA(hmenu, 101, FALSE, &info);
    ok (rc, "Getting the menu info failed\n");
    ok (info.wID == 101, "IDs differ\n");
    ok (!strcmp(info.dwTypeData, "Submenu"), "Returned item has wrong label (%s)\n", info.dwTypeData);

    /* Also look for the menu item  */
    memset( &info, 0, sizeof(info));
    strback[0] = 0x00;
    info.cbSize = sizeof(MENUITEMINFOA);
    info.fMask = MIIM_STRING | MIIM_ID;
    info.dwTypeData = strback;
    info.cch = sizeof(strback);

    rc = GetMenuItemInfoA(hmenu, 102, FALSE, &info);
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
    { INPUT_KEYBOARD, {{0}}, {VK_MENU, 0}, TRUE, FALSE }, /* test 0 */
    { INPUT_KEYBOARD, {{0}}, {VK_ESCAPE, 0}, FALSE, FALSE },
    { INPUT_KEYBOARD, {{0}}, {VK_MENU, 0}, TRUE, FALSE },
    { INPUT_KEYBOARD, {{0}}, {'D', 0}, FALSE, FALSE },
    { INPUT_KEYBOARD, {{0}}, {VK_MENU, 0}, TRUE, FALSE },
    { INPUT_KEYBOARD, {{0}}, {'E', 0}, FALSE, FALSE },
    { INPUT_KEYBOARD, {{0}}, {VK_MENU, 'M', 0}, TRUE, FALSE },
    { INPUT_KEYBOARD, {{0}}, {VK_ESCAPE, VK_ESCAPE, 0}, FALSE, FALSE },
    { INPUT_KEYBOARD, {{0}}, {VK_MENU, 'M', VK_ESCAPE, 0}, TRUE, FALSE },
    { INPUT_KEYBOARD, {{0}}, {VK_ESCAPE, 0}, FALSE, FALSE },
    { INPUT_KEYBOARD, {{0}}, {VK_MENU, 'M', 0}, TRUE, FALSE },
    { INPUT_KEYBOARD, {{0}}, {'D', 0}, FALSE, FALSE },
    { INPUT_KEYBOARD, {{0}}, {VK_MENU, 'M', 0}, TRUE, FALSE },
    { INPUT_KEYBOARD, {{0}}, {'E', 0}, FALSE, FALSE },
    { INPUT_KEYBOARD, {{0}}, {VK_MENU, 'M', 'P', 0}, TRUE, FALSE },
    { INPUT_KEYBOARD, {{0}}, {'D', 0}, FALSE, FALSE },
    { INPUT_KEYBOARD, {{0}}, {VK_MENU, 'M', 'P', 0}, TRUE, FALSE },
    { INPUT_KEYBOARD, {{0}}, {'E', 0}, FALSE, FALSE },
    { INPUT_KEYBOARD, {{0}}, {VK_F10, 0}, TRUE, FALSE },
    { INPUT_KEYBOARD, {{0}}, {VK_F10, 0}, FALSE, FALSE },

    { INPUT_MOUSE, {{1, 2}, {0}}, {0}, TRUE, FALSE }, /* test 20 */
    { INPUT_MOUSE, {{1, 1}, {0}}, {0}, FALSE, FALSE },
    { INPUT_MOUSE, {{1, 0}, {0}}, {0}, TRUE, FALSE },
    { INPUT_MOUSE, {{1, 1}, {0}}, {0}, FALSE, FALSE },
    { INPUT_MOUSE, {{1, 0}, {2, 2}, {0}}, {0}, TRUE, FALSE },
    { INPUT_MOUSE, {{2, 1}, {0}}, {0}, FALSE, FALSE },
    { INPUT_MOUSE, {{1, 0}, {2, 0}, {0}}, {0}, TRUE, FALSE },
    { INPUT_MOUSE, {{3, 0}, {0}}, {0}, FALSE, FALSE },
    { INPUT_MOUSE, {{1, 0}, {2, 0}, {0}}, {0}, TRUE, FALSE },
    { INPUT_MOUSE, {{3, 1}, {0}}, {0}, TRUE, FALSE },
    { INPUT_MOUSE, {{1, 1}, {0}}, {0}, FALSE, FALSE },
    { -1 }
};

static void send_key(WORD wVk)
{
    INPUT i[2];
    memset(i, 0, sizeof(i));
    i[0].type = i[1].type = INPUT_KEYBOARD;
    i[0].ki.wVk = i[1].ki.wVk = wVk;
    i[1].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(2, i, sizeof(INPUT));
}

static BOOL click_menu(HANDLE hWnd, struct menu_item_pair_s *mi)
{
    HMENU hMenu = hMenus[mi->uMenu];
    INPUT i[3];
    MSG msg;
    RECT r;
    int screen_w = GetSystemMetrics(SM_CXSCREEN);
    int screen_h = GetSystemMetrics(SM_CYSCREEN);
    BOOL ret = GetMenuItemRect(mi->uMenu > 2 ? NULL : hWnd, hMenu, mi->uItem, &r);
    if(!ret) return FALSE;

    memset(i, 0, sizeof(i));
    i[0].type = i[1].type = i[2].type = INPUT_MOUSE;
    i[0].mi.dx = i[1].mi.dx = i[2].mi.dx
            = ((r.left + 5) * 65535) / screen_w;
    i[0].mi.dy = i[1].mi.dy = i[2].mi.dy
            = ((r.top + 5) * 65535) / screen_h;
    i[0].mi.dwFlags = i[1].mi.dwFlags = i[2].mi.dwFlags
            = MOUSEEVENTF_ABSOLUTE;
    i[0].mi.dwFlags |= MOUSEEVENTF_MOVE;
    i[1].mi.dwFlags |= MOUSEEVENTF_LEFTDOWN;
    i[2].mi.dwFlags |= MOUSEEVENTF_LEFTUP;
    ret = SendInput(3, i, sizeof(INPUT));

    /* hack to prevent mouse message buildup in Wine */
    while (PeekMessageA( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageA( &msg );
    return ret;
}

static DWORD WINAPI test_menu_input_thread(LPVOID lpParameter)
{
    int i, j;
    HANDLE hWnd = lpParameter;

    Sleep(500);
    /* mixed keyboard/mouse test */
    for (i = 0; menu_tests[i].type != -1; i++)
    {
        BOOL ret = TRUE;
        int elapsed;

        got_input = i && menu_tests[i-1].bMenuVisible;

        if (menu_tests[i].type == INPUT_KEYBOARD)
            for (j = 0; menu_tests[i].wVk[j] != 0; j++)
                send_key(menu_tests[i].wVk[j]);
        else
            for (j = 0; menu_tests[i].menu_item_pairs[j].uMenu != 0; j++)
            {
                /* Maybe clicking too fast before menu is initialized. Sleep 100 ms and retry */
                elapsed = 0;
                while (!(ret = click_menu(hWnd, &menu_tests[i].menu_item_pairs[j])))
                {
                    if (elapsed > 1000) break;
                    elapsed += 100;
                    Sleep(100);
                }
            }

        if (!ret)
        {
            skip( "test %u: failed to send input\n", i );
            PostMessageA( hWnd, WM_CANCELMODE, 0, 0 );
            return 0;
        }

        elapsed = 0;
        while (menu_tests[i].bMenuVisible != bMenuVisible)
        {
            if (elapsed > 200)
                break;
            elapsed += 20;
            Sleep(20);
        }

        if (!got_input)
        {
            skip( "test %u: didn't receive input\n", i );
            PostMessageA( hWnd, WM_CANCELMODE, 0, 0 );
            return 0;
        }

        todo_wine_if (menu_tests[i]._todo_wine)
            ok(menu_tests[i].bMenuVisible == bMenuVisible, "test %d\n", i);
    }
    return 0;
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam,
        LPARAM lParam)
{
    MENUBARINFO mbi;
    HMENU hmenu;
    UINT state;
    BOOL br;

    switch (msg) {
        case WM_ENTERMENULOOP:
            bMenuVisible = TRUE;
            break;
        case WM_INITMENUPOPUP:
        case WM_UNINITMENUPOPUP:
        case WM_EXITMENULOOP:
        case WM_MENUSELECT:
            break;

        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_NCMOUSEMOVE:
        case WM_NCLBUTTONDOWN:
        case WM_NCLBUTTONUP:
            got_input = TRUE;
            /* fall through */
        default:
            return( DefWindowProcA( hWnd, msg, wParam, lParam ) );
    }

    mbi.cbSize = sizeof(MENUBARINFO);

    /* get info for the menu */
    br = GetMenuBarInfo(hWnd, OBJID_MENU, 0, &mbi);
    ok(br, "msg %x: GetMenuBarInfo failed\n", msg);
    hmenu = GetMenu(hWnd);
    ok(!mbi.hwndMenu, "msg %x: GetMenuBarInfo.hwndMenu wrong: %p expected NULL\n",
            msg, mbi.hwndMenu);
    ok(mbi.hMenu == hmenu, "msg %x: GetMenuBarInfo got wrong menu: %p expected %p\n",
            msg, mbi.hMenu, hmenu);
    ok(!bMenuVisible == !mbi.fBarFocused, "msg %x: GetMenuBarInfo.fBarFocused (%d) is wrong\n",
            msg, mbi.fBarFocused != 0);
    ok(!bMenuVisible == !mbi.fFocused, "msg %x: GetMenuBarInfo.fFocused (%d) is wrong\n",
            msg, mbi.fFocused != 0);

    /* get info for the menu's first item */
    br = GetMenuBarInfo(hWnd, OBJID_MENU, 1, &mbi);
    ok(br, "msg %x: GetMenuBarInfo failed\n", msg);
    state = GetMenuState(hmenu, 0, MF_BYPOSITION);
    /* Native returns handle to destroyed window */
    todo_wine_if (msg==WM_UNINITMENUPOPUP && popmenu==1)
        ok(!mbi.hwndMenu == !popmenu,
            "msg %x: GetMenuBarInfo.hwndMenu wrong: %p expected %sNULL\n",
            msg, mbi.hwndMenu, popmenu ? "not " : "");
    ok(mbi.hMenu == hmenu, "msg %x: GetMenuBarInfo got wrong menu: %p expected %p\n",
            msg, mbi.hMenu, hmenu);
    ok(!bMenuVisible == !mbi.fBarFocused, "nsg %x: GetMenuBarInfo.fBarFocused (%d) is wrong\n",
            msg, mbi.fBarFocused != 0);
    ok(!(bMenuVisible && (state & MF_HILITE)) == !mbi.fFocused,
            "msg %x: GetMenuBarInfo.fFocused (%d) is wrong\n", msg, mbi.fFocused != 0);

    if (msg == WM_EXITMENULOOP)
        bMenuVisible = FALSE;
    else if (msg == WM_INITMENUPOPUP)
        popmenu++;
    else if (msg == WM_UNINITMENUPOPUP)
        popmenu--;
    return 0;
}

static void test_menu_input(void) {
    MSG msg;
    WNDCLASSA  wclass;
    HINSTANCE hInstance = GetModuleHandleA( NULL );
    HANDLE hThread, hWnd;
    DWORD tid;
    ATOM aclass;
    POINT orig_pos;

    wclass.lpszClassName = "MenuTestClass";
    wclass.style         = CS_HREDRAW | CS_VREDRAW;
    wclass.lpfnWndProc   = WndProc;
    wclass.hInstance     = hInstance;
    wclass.hIcon         = LoadIconA( 0, (LPCSTR)IDI_APPLICATION );
    wclass.hCursor       = LoadCursorA( 0, (LPCSTR)IDC_ARROW );
    wclass.hbrBackground = (HBRUSH)( COLOR_WINDOW + 1 );
    wclass.lpszMenuName  = 0;
    wclass.cbClsExtra    = 0;
    wclass.cbWndExtra    = 0;
    aclass = RegisterClassA( &wclass );
    ok (aclass, "MenuTest class not created\n");
    if (!aclass) return;
    hWnd = CreateWindowA( wclass.lpszClassName, "MenuTest",
                          WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0,
                          400, 200, NULL, NULL, hInstance, NULL);
    ok (hWnd != NULL, "MenuTest window not created\n");
    if (!hWnd) return;
    /* fixed menus */
    hMenus[3] = CreatePopupMenu();
    AppendMenuA(hMenus[3], MF_STRING, 0, "&Enabled");
    AppendMenuA(hMenus[3], MF_STRING|MF_DISABLED, 0, "&Disabled");

    hMenus[2] = CreatePopupMenu();
    AppendMenuA(hMenus[2], MF_STRING|MF_POPUP, (UINT_PTR) hMenus[3], "&Popup");
    AppendMenuA(hMenus[2], MF_STRING, 0, "&Enabled");
    AppendMenuA(hMenus[2], MF_STRING|MF_DISABLED, 0, "&Disabled");

    hMenus[1] = CreateMenu();
    AppendMenuA(hMenus[1], MF_STRING|MF_POPUP, (UINT_PTR) hMenus[2], "&Menu");
    AppendMenuA(hMenus[1], MF_STRING, 0, "&Enabled");
    AppendMenuA(hMenus[1], MF_STRING|MF_DISABLED, 0, "&Disabled");

    SetMenu(hWnd, hMenus[1]);
    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);

    GetCursorPos(&orig_pos);

    hThread = CreateThread(NULL, 0, test_menu_input_thread, hWnd, 0, &tid);
    while(1)
    {
        if (WAIT_TIMEOUT != WaitForSingleObject(hThread, 50))
            break;
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
    }
    SetCursorPos(orig_pos.x, orig_pos.y);
    DestroyWindow(hWnd);
}

static void test_menu_flags( void )
{
    HMENU hMenu, hPopupMenu;

    hMenu = CreateMenu();
    hPopupMenu = CreatePopupMenu();

    AppendMenuA(hMenu, MF_POPUP | MF_STRING, (UINT_PTR)hPopupMenu, "Popup");

    AppendMenuA(hPopupMenu, MF_STRING | MF_HILITE | MF_DEFAULT, 101, "Item 1");
    InsertMenuA(hPopupMenu, 1, MF_BYPOSITION | MF_STRING | MF_HILITE | MF_DEFAULT, 102, "Item 2");
    AppendMenuA(hPopupMenu, MF_STRING, 103, "Item 3");
    ModifyMenuA(hPopupMenu, 2, MF_BYPOSITION | MF_STRING | MF_HILITE | MF_DEFAULT, 103, "Item 3");

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
    ATOM aclass;

    wclass.lpszClassName = "HiliteMenuTestClass";
    wclass.style         = CS_HREDRAW | CS_VREDRAW;
    wclass.lpfnWndProc   = WndProc;
    wclass.hInstance     = GetModuleHandleA( NULL );
    wclass.hIcon         = LoadIconA( 0, (LPCSTR)IDI_APPLICATION );
    wclass.hCursor       = LoadCursorA( 0, (LPCSTR)IDC_ARROW );
    wclass.hbrBackground = (HBRUSH)( COLOR_WINDOW + 1 );
    wclass.lpszMenuName  = 0;
    wclass.cbClsExtra    = 0;
    wclass.cbWndExtra    = 0;
    aclass = RegisterClassA( &wclass );
    ok (aclass, "HiliteMenuTest class could not be created\n");
    if (!aclass) return;
    hWnd = CreateWindowA( wclass.lpszClassName, "HiliteMenuTest",
                          WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0,
                          400, 200, NULL, NULL, wclass.hInstance, NULL);
    ok (hWnd != NULL, "HiliteMenuTest window could not be created\n");
    if (!hWnd) return;

    hMenu = CreateMenu();
    hPopupMenu = CreatePopupMenu();

    AppendMenuA(hMenu, MF_POPUP | MF_STRING, (UINT_PTR)hPopupMenu, "Popup");

    AppendMenuA(hPopupMenu, MF_STRING, 101, "Item 1");
    AppendMenuA(hPopupMenu, MF_STRING, 102, "Item 2");
    AppendMenuA(hPopupMenu, MF_STRING, 103, "Item 3");

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
      "HiliteMenuItem: expected error ERROR_INVALID_WINDOW_HANDLE, got: %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ok(!HiliteMenuItem(hWnd, NULL, 1, MF_HILITE | MF_BYPOSITION),
      "HiliteMenuItem: call should have failed.\n");
    ok(GetLastError() == 0xdeadbeef || /* 9x */
       GetLastError() == ERROR_INVALID_MENU_HANDLE /* NT */,
      "HiliteMenuItem: expected error ERROR_INVALID_MENU_HANDLE, got: %ld\n", GetLastError());

    ok(!(GetMenuState(hPopupMenu, 1, MF_BYPOSITION) & MF_HILITE),
      "HiliteMenuItem: Item 2 is hilited\n");

    /* either MF_HILITE or MF_UNHILITE *and* MF_BYCOMMAND or MF_BYPOSITION need to be set */

    SetLastError(0xdeadbeef);
    ok(HiliteMenuItem(hWnd, hPopupMenu, 1, MF_BYPOSITION),
      "HiliteMenuItem: call should have succeeded.\n");
    ok(GetLastError() == 0xdeadbeef,
      "HiliteMenuItem: expected error 0xdeadbeef, got: %ld\n", GetLastError());

    ok(!(GetMenuState(hPopupMenu, 1, MF_BYPOSITION) & MF_HILITE),
      "HiliteMenuItem: Item 2 is hilited\n");

    SetLastError(0xdeadbeef);
    todo_wine
    {
    ok(HiliteMenuItem(hWnd, hPopupMenu, 1, MF_HILITE),
      "HiliteMenuItem: call should have succeeded.\n");
    }
    ok(GetLastError() == 0xdeadbeef,
      "HiliteMenuItem: expected error 0xdeadbeef, got: %ld\n", GetLastError());

    ok(!(GetMenuState(hPopupMenu, 1, MF_BYPOSITION) & MF_HILITE),
      "HiliteMenuItem: Item 2 is hilited\n");

    /* hilite a menu item (by position) */

    SetLastError(0xdeadbeef);
    ok(HiliteMenuItem(hWnd, hPopupMenu, 1, MF_HILITE | MF_BYPOSITION),
      "HiliteMenuItem: call should not have failed.\n");
    ok(GetLastError() == 0xdeadbeef,
      "HiliteMenuItem: expected error 0xdeadbeef, got: %ld\n", GetLastError());

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
      "HiliteMenuItem: expected error 0xdeadbeef, got: %ld\n", GetLastError());

    ok(!(GetMenuState(hPopupMenu, 1, MF_BYPOSITION) & MF_HILITE),
      "HiliteMenuItem: Item 2 is hilited\n");

    /* hilite a menu item (by command) */

    SetLastError(0xdeadbeef);
    ok(HiliteMenuItem(hWnd, hPopupMenu, 103, MF_HILITE | MF_BYCOMMAND),
      "HiliteMenuItem: call should not have failed.\n");
    ok(GetLastError() == 0xdeadbeef,
      "HiliteMenuItem: expected error 0xdeadbeef, got: %ld\n", GetLastError());

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
      "HiliteMenuItem: expected error 0xdeadbeef, got: %ld\n", GetLastError());

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
        MENUITEMINFOA mii;

        memset(&mii, 0, sizeof(mii));
        mii.cbSize = sizeof(mii);
        mii.fMask  = MIIM_FTYPE | MIIM_STATE | MIIM_ID | MIIM_SUBMENU;
        ret = GetMenuItemInfoA(hmenu, i, TRUE, &mii);
        ok(ret, "GetMenuItemInfo(%u) failed\n", i);

        if (winetest_debug > 1)
            trace("item #%u: fType %04x, fState %04x, wID %u, hSubMenu %p\n",
                  i, mii.fType, mii.fState, mii.wID, mii.hSubMenu);

        if (mii.hSubMenu)
        {
            ok(mii.wID == (UINT_PTR)mii.hSubMenu, "id %u: wID %x should be equal to hSubMenu %p\n",
               checked_cmd, mii.wID, mii.hSubMenu);
            if (!GetMenuItemCount(mii.hSubMenu))
            {
                ok(mii.fType == checked_type, "id %u: expected fType %04x, got %04x\n", checked_cmd, checked_type, mii.fType);
                ok(mii.fState == checked_state, "id %u: expected fState %04x, got %04x\n", checked_cmd, checked_state, mii.fState);
            }
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
    MENUITEMINFOA mii;

    memset(&mii, 0, sizeof(mii));
    mii.cbSize = sizeof(mii);
    mii.fMask  = MIIM_FTYPE | MIIM_STATE;
    ret = SetMenuItemInfoA(hmenu, id, (flags & MF_BYPOSITION) != 0, &mii);
    ok(ret, "SetMenuItemInfo(%u) failed\n", id);
}

static void test_CheckMenuRadioItem(void)
{
    BOOL ret;
    HMENU hmenu;

    hmenu = LoadMenuA(GetModuleHandleA(NULL), MAKEINTRESOURCEA(1));
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

    hmenu = LoadMenuIndirectA(&menu_template);
    ok(hmenu != 0, "LoadMenuIndirect error %lu\n", GetLastError());

    ret = AppendMenuA(hmenu, MF_STRING, 6, NULL);
    ok(ret, "AppendMenu failed\n");
    ret = AppendMenuA(hmenu, MF_STRING, 7, "\0");
    ok(ret, "AppendMenu failed\n");
    ret = AppendMenuA(hmenu, MF_SEPARATOR, 8, "separator");
    ok(ret, "AppendMenu failed\n");

    count = GetMenuItemCount(hmenu);
    ok(count == ARRAY_SIZE(menu_data), "expected %u menu items, got %u\n",
       (UINT) ARRAY_SIZE(menu_data), count);

    for (i = 0; i < count; i++)
    {
        char buf[20];
        MENUITEMINFOA mii;

        memset(&mii, 0, sizeof(mii));
        mii.cbSize = sizeof(mii);
        mii.dwTypeData = buf;
        mii.cch = sizeof(buf);
        mii.fMask  = MIIM_FTYPE | MIIM_STATE | MIIM_ID | MIIM_STRING;
        ret = GetMenuItemInfoA(hmenu, i, TRUE, &mii);
        ok(ret, "GetMenuItemInfo(%u) failed\n", i);
        if (winetest_debug > 1)
            trace("item #%u: fType %04x, fState %04x, wID %u, dwTypeData %s\n",
                  i, mii.fType, mii.fState, mii.wID, (LPCSTR)mii.dwTypeData);

        ok(mii.fType == menu_data[i].type,
           "%u: expected fType %04x, got %04x\n", i, menu_data[i].type, mii.fType);
        ok(mii.fState == menu_data[i].state,
           "%u: expected fState %04x, got %04x\n", i, menu_data[i].state, mii.fState);
        ok(mii.wID == menu_data[i].id,
           "%u: expected wID %04x, got %04x\n", i, menu_data[i].id, mii.wID);
        ok(mii.cch == strlen(menu_data[i].str),
           "%u: expected cch %u, got %u\n", i, (UINT)strlen(menu_data[i].str), mii.cch);
        ok(!strcmp(mii.dwTypeData, menu_data[i].str),
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
        ret = AppendMenuA(hmenu, item[i].type, item[i].id, item[i].str);
        ok(ret, "%d: AppendMenu(%04x, %04x, %p) error %lu\n",
           i, item[i].type, item[i].id, item[i].str, GetLastError());
    }
    return hmenu;
}

/* use InsertMenuItem: does not set the MFT_BITMAP flag,
 * and does not accept non-magic bitmaps with invalid
 * bitmap handles */
static HMENU create_menuitem_from_data(const struct menu_data *item, INT item_count)
{
    HMENU hmenu;
    INT i;
    BOOL ret;
    MENUITEMINFOA mii = { sizeof( MENUITEMINFOA) };

    hmenu = CreateMenu();
    assert(hmenu != 0);

    for (i = 0; i < item_count; i++)
    {
        SetLastError(0xdeadbeef);

        mii.fMask = MIIM_FTYPE | MIIM_ID | MIIM_STATE;
        mii.fType = 0;
        if(  item[i].type & MFT_BITMAP)
        {
            mii.fMask |= MIIM_BITMAP;
            mii.hbmpItem =  (HBITMAP)item[i].str;
        }
        else if(  item[i].type & MFT_SEPARATOR)
            mii.fType = MFT_SEPARATOR;
        else
        {
            mii.fMask |= MIIM_STRING;
            mii.dwTypeData =  (LPSTR)item[i].str;
            mii.cch = strlen(  item[i].str);
        }
        mii.fState = 0;
        if(  item[i].type & MF_HELP) mii.fType |= MF_HELP;
        mii.wID = item[i].id;
        ret = InsertMenuItemA( hmenu, -1, TRUE, &mii);
        ok(ret, "%d: InsertMenuItem(%04x, %04x, %p) error %lu\n",
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
        MENUITEMINFOA mii;

        memset(&mii, 0, sizeof(mii));
        mii.cbSize = sizeof(mii);
        mii.dwTypeData = buf;
        mii.cch = sizeof(buf);
        mii.fMask  = MIIM_FTYPE | MIIM_ID | MIIM_STRING | MIIM_BITMAP;
        ret = GetMenuItemInfoA(hmenu, i, TRUE, &mii);
        ok(ret, "GetMenuItemInfo(%u) failed\n", i);

        if (winetest_debug > 1)
            trace("item #%u: fType %04x, fState %04x, wID %04x, hbmp %p\n",
                  i, mii.fType, mii.fState, mii.wID, mii.hbmpItem);

        ok(mii.fType == item[i].type,
           "%u: expected fType %04x, got %04x\n", i, item[i].type, mii.fType);
        ok(mii.wID == item[i].id,
           "%u: expected wID %04x, got %04x\n", i, item[i].id, mii.wID);
        if (mii.hbmpItem || !item[i].str)
            /* For some reason Windows sets high word to not 0 for
             * not "magic" ids.
             */
            ok(LOWORD(mii.hbmpItem) == LOWORD(item[i].str),
               "%u: expected hbmpItem %p, got %p\n", i, item[i].str, mii.hbmpItem);
        else
        {
            ok(mii.cch == strlen(item[i].str),
               "%u: expected cch %u, got %u\n", i, (UINT)strlen(item[i].str), mii.cch);
            ok(!strcmp(mii.dwTypeData, item[i].str),
               "%u: expected dwTypeData %s, got %s\n", i, item[i].str, (LPCSTR)mii.dwTypeData);
        }
    }
}

static void test_InsertMenu(void)
{
    HBITMAP hbm = CreateBitmap(1,1,1,1,NULL);
    /* Note: XP treats only bitmap handles 1 - 6 as "magic" ones
     * regardless of their id.
     */
    static const struct menu_data in1[] =
    {
        { MF_STRING, 1, "File" },
        { MF_BITMAP|MF_HELP, SC_CLOSE, MAKEINTRESOURCEA(1) },
        { MF_STRING|MF_HELP, 2, "Help" }
    };
    static const struct menu_data out1[] =
    {
        { MF_STRING, 1, "File" },
        { MF_STRING|MF_HELP, 2, "Help" },
        { MF_BITMAP|MF_HELP, SC_CLOSE, MAKEINTRESOURCEA(1) }
    };
    static const struct menu_data out1a[] =
    {
        { MF_STRING, 1, "File" },
        { MF_STRING|MF_HELP, 2, "Help" },
        { MF_HELP, SC_CLOSE, MAKEINTRESOURCEA(1) }
    };
    const struct menu_data in2[] =
    {
        { MF_STRING, 1, "File" },
        { MF_BITMAP|MF_HELP, SC_CLOSE, (char*)hbm },
        { MF_STRING|MF_HELP, 2, "Help" }
    };
    const struct menu_data out2[] =
    {
        { MF_STRING, 1, "File" },
        { MF_BITMAP|MF_HELP, SC_CLOSE, (char*)hbm },
        { MF_STRING|MF_HELP, 2, "Help" }
    };
    const struct menu_data out2a[] =
    {
        { MF_STRING, 1, "File" },
        { MF_HELP, SC_CLOSE, (char*)hbm },
        { MF_STRING|MF_HELP, 2, "Help" }
    };
    static const struct menu_data in3[] =
    {
        { MF_STRING, 1, "File" },
        { MF_SEPARATOR|MF_HELP, SC_CLOSE, MAKEINTRESOURCEA(1) },
        { MF_STRING|MF_HELP, 2, "Help" }
    };
    static const struct menu_data out3[] =
    {
        { MF_STRING, 1, "File" },
        { MF_SEPARATOR|MF_HELP, SC_CLOSE, MAKEINTRESOURCEA(0) },
        { MF_STRING|MF_HELP, 2, "Help" },
    };
    static const struct menu_data in4[] =
    {
        { MF_STRING, 1, "File" },
        { MF_BITMAP|MF_HELP, 1, MAKEINTRESOURCEA(1) },
        { MF_STRING|MF_HELP, 2, "Help" }
    };
    static const struct menu_data out4[] =
    {
        { MF_STRING, 1, "File" },
        { MF_STRING|MF_HELP, 2, "Help" },
        { MF_BITMAP|MF_HELP, 1, MAKEINTRESOURCEA(1) }
    };
    static const struct menu_data out4a[] =
    {
        { MF_STRING, 1, "File" },
        { MF_STRING|MF_HELP, 2, "Help" },
        { MF_HELP, 1, MAKEINTRESOURCEA(1) }
    };
    HMENU hmenu;
    BOOL ret;

#define create_menu(a) create_menu_from_data((a), ARRAY_SIZE(a))
#define create_menuitem(a) create_menuitem_from_data((a), ARRAY_SIZE(a))
#define compare_menu(h, a) compare_menu_data((h), (a), ARRAY_SIZE(a))

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

    /* now using InsertMenuItemInfo */
    hmenu = create_menuitem(in1);
    compare_menu(hmenu, out1a);
    DestroyMenu(hmenu);

    hmenu = create_menuitem(in2);
    compare_menu(hmenu, out2a);
    DestroyMenu(hmenu);

    hmenu = create_menuitem(in3);
    compare_menu(hmenu, out3);
    DestroyMenu(hmenu);

    hmenu = create_menuitem(in4);
    compare_menu(hmenu, out4a);
    DestroyMenu(hmenu);

#undef create_menu
#undef create_menuitem
#undef compare_menu

    hmenu = CreateMenu();

    SetLastError(0xdeadbeef);
    ret = InsertMenuW(hmenu, -1, MF_BYPOSITION | MF_POPUP, 0xdeadbeef, L"test");
    ok(ret && GetLastError() == ERROR_INVALID_MENU_HANDLE,
       "InsertMenuW returned %x %lu\n", ret, GetLastError());
    ok(GetMenuItemCount(hmenu) == 1, "GetMenuItemCount() = %d\n", GetMenuItemCount(hmenu));

    DestroyMenu(hmenu);
}

static void test_menu_getmenuinfo(void)
{
    HMENU hmenu;
    MENUINFO mi = {0};
    BOOL ret;
    DWORD gle;

    /* create a menu */
    hmenu = CreateMenu();
    assert( hmenu);
    /* test some parameter errors */
    SetLastError(0xdeadbeef);
    ret = GetMenuInfo( hmenu, NULL);
    gle= GetLastError();
    ok( !ret, "GetMenuInfo() should have failed\n");
    ok( gle == ERROR_INVALID_PARAMETER ||
        broken(gle == 0xdeadbeef), /* Win98, WinME */
        "GetMenuInfo() error got %lu expected %u\n", gle, ERROR_INVALID_PARAMETER);
    SetLastError(0xdeadbeef);
    mi.cbSize = 0;
    ret = GetMenuInfo( hmenu, &mi);
    gle= GetLastError();
    ok( !ret, "GetMenuInfo() should have failed\n");
    ok( gle == ERROR_INVALID_PARAMETER ||
        broken(gle == 0xdeadbeef), /* Win98, WinME */
        "GetMenuInfo() error got %lu expected %u\n", gle, ERROR_INVALID_PARAMETER);
    SetLastError(0xdeadbeef);
    mi.cbSize = sizeof( MENUINFO);
    ret = GetMenuInfo( hmenu, &mi);
    gle= GetLastError();
    ok( ret, "GetMenuInfo() should have succeeded\n");
    ok( gle == 0xdeadbeef, "GetMenuInfo() error got %lu\n", gle);
    SetLastError(0xdeadbeef);
    mi.cbSize = 0;
    ret = GetMenuInfo( NULL, &mi);
    gle= GetLastError();
    ok( !ret, "GetMenuInfo() should have failed\n");
    ok( gle == ERROR_INVALID_PARAMETER ||
        broken(gle == 0xdeadbeef), /* Win98, WinME */
        "GetMenuInfo() error got %lu expected %u\n", gle, ERROR_INVALID_PARAMETER);
    /* clean up */
    DestroyMenu( hmenu);
    return;
}

static void test_menu_setmenuinfo(void)
{
    HMENU hmenu, hsubmenu;
    MENUINFO mi = {0};
    MENUITEMINFOA mii = { sizeof(MENUITEMINFOA) };
    BOOL ret;
    DWORD gle;
    HBRUSH brush;

    /* create a menu with a submenu */
    hmenu = CreateMenu();
    hsubmenu = CreateMenu();
    assert( hmenu && hsubmenu);
    mii.fMask = MIIM_SUBMENU;
    mii.hSubMenu = hsubmenu;
    ret = InsertMenuItemA( hmenu, 0, FALSE, &mii);
    ok( ret, "InsertMenuItem failed with error %ld\n", GetLastError());
    /* test some parameter errors */
    SetLastError(0xdeadbeef);
    ret = SetMenuInfo( hmenu, NULL);
    gle= GetLastError();
    ok( !ret, "SetMenuInfo() should have failed\n");
    ok( gle == ERROR_INVALID_PARAMETER ||
        broken(gle == 0xdeadbeef), /* Win98, WinME */
        "SetMenuInfo() error got %lu expected %u\n", gle, ERROR_INVALID_PARAMETER);
    SetLastError(0xdeadbeef);
    mi.cbSize = 0;
    ret = SetMenuInfo( hmenu, &mi);
    gle= GetLastError();
    ok( !ret, "SetMenuInfo() should have failed\n");
    ok( gle == ERROR_INVALID_PARAMETER ||
        broken(gle == 0xdeadbeef), /* Win98, WinME */
        "SetMenuInfo() error got %lu expected %u\n", gle, ERROR_INVALID_PARAMETER);
    SetLastError(0xdeadbeef);
    mi.cbSize = sizeof( MENUINFO);
    ret = SetMenuInfo( hmenu, &mi);
    gle= GetLastError();
    ok( ret, "SetMenuInfo() should have succeeded\n");
    ok( gle == 0xdeadbeef, "SetMenuInfo() error got %lu\n", gle);
    SetLastError(0xdeadbeef);
    mi.cbSize = 0;
    ret = SetMenuInfo( NULL, &mi);
    gle= GetLastError();
    ok( !ret, "SetMenuInfo() should have failed\n");
    ok( gle == ERROR_INVALID_PARAMETER ||
        broken(gle == 0xdeadbeef), /* Win98, WinME */
        "SetMenuInfo() error got %lu expected %u\n", gle, ERROR_INVALID_PARAMETER);
    /* functional tests */
    /* menu and submenu should have the CHECKORBMP style bit cleared */
    SetLastError(0xdeadbeef);
    mi.cbSize = sizeof( MENUINFO);
    mi.fMask = MIM_STYLE;
    ret = GetMenuInfo( hmenu, &mi);
    gle= GetLastError();
    ok( ret, "GetMenuInfo() should have succeeded\n");
    ok( gle == 0xdeadbeef, "GetMenuInfo() error got %lu\n", gle);
    ok( !(mi.dwStyle & MNS_CHECKORBMP), "menustyle was not expected to have the MNS_CHECKORBMP flag\n");
    SetLastError(0xdeadbeef);
    mi.cbSize = sizeof( MENUINFO);
    mi.fMask = MIM_STYLE;
    ret = GetMenuInfo( hsubmenu, &mi);
    gle= GetLastError();
    ok( ret, "GetMenuInfo() should have succeeded\n");
    ok( gle == 0xdeadbeef, "GetMenuInfo() error got %lu\n", gle);
    ok( !(mi.dwStyle & MNS_CHECKORBMP), "menustyle was not expected to have the MNS_CHECKORBMP flag\n");
    /* SetMenuInfo() */
    SetLastError(0xdeadbeef);
    mi.cbSize = sizeof( MENUINFO);
    mi.fMask = MIM_STYLE | MIM_APPLYTOSUBMENUS;
    mi.dwStyle = MNS_CHECKORBMP;
    ret = SetMenuInfo( hmenu, &mi);
    gle= GetLastError();
    ok( ret, "SetMenuInfo() should have succeeded\n");
    ok( gle == 0xdeadbeef, "SetMenuInfo() error got %lu\n", gle);
    /* Now both menus should have the MNS_CHECKORBMP style bit set */
    SetLastError(0xdeadbeef);
    mi.cbSize = sizeof( MENUINFO);
    mi.fMask = MIM_STYLE;
    ret = GetMenuInfo( hmenu, &mi);
    gle= GetLastError();
    ok( ret, "GetMenuInfo() should have succeeded\n");
    ok( gle == 0xdeadbeef, "GetMenuInfo() error got %lu\n", gle);
    ok( mi.dwStyle & MNS_CHECKORBMP, "menustyle was expected to have the MNS_CHECKORBMP flag\n");
    SetLastError(0xdeadbeef);
    mi.cbSize = sizeof( MENUINFO);
    mi.fMask = MIM_STYLE;
    ret = GetMenuInfo( hsubmenu, &mi);
    gle= GetLastError();
    ok( ret, "GetMenuInfo() should have succeeded\n");
    ok( gle == 0xdeadbeef, "GetMenuInfo() error got %lu\n", gle);
    ok( mi.dwStyle & MNS_CHECKORBMP, "menustyle was expected to have the MNS_CHECKORBMP flag\n");
    /* now repeat that without the APPLYTOSUBMENUS flag and another style bit */
    SetLastError(0xdeadbeef);
    mi.cbSize = sizeof( MENUINFO);
    mi.fMask = MIM_STYLE ;
    mi.dwStyle = MNS_NOCHECK;
    ret = SetMenuInfo( hmenu, &mi);
    gle= GetLastError();
    ok( ret, "SetMenuInfo() should have succeeded\n");
    ok( gle == 0xdeadbeef, "SetMenuInfo() error got %lu\n", gle);
    /* Now only the top menu should have the MNS_NOCHECK style bit set */
    SetLastError(0xdeadbeef);
    mi.cbSize = sizeof( MENUINFO);
    mi.fMask = MIM_STYLE;
    ret = GetMenuInfo( hmenu, &mi);
    gle= GetLastError();
    ok( ret, "GetMenuInfo() should have succeeded\n");
    ok( gle == 0xdeadbeef, "GetMenuInfo() error got %lu\n", gle);
    ok( mi.dwStyle & MNS_NOCHECK, "menustyle was expected to have the MNS_NOCHECK flag\n");
    SetLastError(0xdeadbeef);
    mi.cbSize = sizeof( MENUINFO);
    mi.fMask = MIM_STYLE;
    ret = GetMenuInfo( hsubmenu, &mi);
    gle= GetLastError();
    ok( ret, "GetMenuInfo() should have succeeded\n");
    ok( gle == 0xdeadbeef, "GetMenuInfo() error got %lu\n", gle);
    ok( !(mi.dwStyle & MNS_NOCHECK), "menustyle was not expected to have the MNS_NOCHECK flag\n");

    /* test background brush */
    mi.cbSize = sizeof(mi);
    mi.fMask = MIM_BACKGROUND;
    ret = GetMenuInfo( hmenu, &mi );
    ok( ret, "GetMenuInfo() should have succeeded\n" );
    ok( mi.hbrBack == NULL, "got %p\n", mi.hbrBack );

    brush = CreateSolidBrush( RGB(0xff, 0, 0) );
    mi.hbrBack = brush;
    ret = SetMenuInfo( hmenu, &mi );
    ok( ret, "SetMenuInfo() should have succeeded\n" );
    mi.hbrBack = NULL;
    ret = GetMenuInfo( hmenu, &mi );
    ok( ret, "GetMenuInfo() should have succeeded\n" );
    ok( mi.hbrBack == brush, "got %p original %p\n", mi.hbrBack, brush );

    mi.hbrBack = NULL;
    ret = SetMenuInfo( hmenu, &mi );
    ok( ret, "SetMenuInfo() should have succeeded\n" );
    ret = GetMenuInfo( hmenu, &mi );
    ok( ret, "GetMenuInfo() should have succeeded\n" );
    ok( mi.hbrBack == NULL, "got %p\n", mi.hbrBack );
    DeleteObject( brush );

    /* clean up */
    DestroyMenu( hsubmenu);
    DestroyMenu( hmenu);
    return;
}

/* little func to easy switch either TrackPopupMenu() or TrackPopupMenuEx() */
static DWORD MyTrackPopupMenu( int ex, HMENU hmenu, UINT flags, INT x, INT y, HWND hwnd, LPTPMPARAMS ptpm)
{
    return ex
        ? TrackPopupMenuEx( hmenu, flags, x, y, hwnd, ptpm)
        : TrackPopupMenu( hmenu, flags, x, y, 0, hwnd, NULL);
}

/* some TrackPopupMenu and TrackPopupMenuEx tests */
/* the LastError values differ between NO_ERROR and invalid handle */
/* between all windows versions tested. The first value is that valid on XP  */
/* Vista was the only that made returned different error values */
/* between the TrackPopupMenu and TrackPopupMenuEx functions */
static void test_menu_trackpopupmenu(void)
{
    BOOL ret;
    HMENU hmenu;
    DWORD gle;
    int Ex;
    HWND hwnd = CreateWindowExA(0, (LPCSTR)MAKEINTATOM(atomMenuCheckClass), NULL,
            WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 200, 200,
            NULL, NULL, NULL, NULL);
    ok(hwnd != NULL, "CreateWindowEx failed with error %ld\n", GetLastError());
    if (!hwnd) return;
    SetWindowLongPtrA( hwnd, GWLP_WNDPROC, (LONG_PTR)menu_ownerdraw_wnd_proc);
    for( Ex = 0; Ex < 2; Ex++)
    {
        hmenu = CreatePopupMenu();
        ok(hmenu != NULL, "CreateMenu failed with error %ld\n", GetLastError());
        if (!hmenu)
        {
            DestroyWindow(hwnd);
            return;
        }
        /* display the menu */
        /* start with an invalid menu handle */
        SetLastError(0xdeadbeef);

        gflag_initmenupopup = gflag_entermenuloop = gflag_initmenu = 0;
        ret = MyTrackPopupMenu( Ex, NULL, TPM_RETURNCMD, 100,100, hwnd, NULL);
        gle = GetLastError();
        ok( !ret, "TrackPopupMenu%s should have failed\n", Ex ? "Ex" : "");
        ok( gle == ERROR_INVALID_MENU_HANDLE
            || broken (gle == 0xdeadbeef) /* win95 */
            || broken (gle == NO_ERROR) /* win98/ME */
            ,"TrackPopupMenu%s error got %lu expected %u\n",
            Ex ? "Ex" : "", gle, ERROR_INVALID_MENU_HANDLE);
        ok( !(gflag_initmenupopup || gflag_entermenuloop || gflag_initmenu),
                "got unexpected message(s)%s%s%s\n",
                gflag_initmenupopup ? " WM_INITMENUPOPUP ": " ",
                gflag_entermenuloop ? "WM_INITMENULOOP ": "",
                gflag_initmenu ? "WM_INITMENU": "");
        /* another one but not NULL */
        SetLastError(0xdeadbeef);
        gflag_initmenupopup = gflag_entermenuloop = gflag_initmenu = 0;
        ret = MyTrackPopupMenu( Ex, (HMENU)hwnd, TPM_RETURNCMD, 100,100, hwnd, NULL);
        gle = GetLastError();
        ok( !ret, "TrackPopupMenu%s should have failed\n", Ex ? "Ex" : "");
        ok( gle == ERROR_INVALID_MENU_HANDLE
            || broken (gle == 0xdeadbeef) /* win95 */
            || broken (gle == NO_ERROR) /* win98/ME */
            ,"TrackPopupMenu%s error got %lu expected %u\n",
            Ex ? "Ex" : "", gle, ERROR_INVALID_MENU_HANDLE);
        ok( !(gflag_initmenupopup || gflag_entermenuloop || gflag_initmenu),
                "got unexpected message(s)%s%s%s\n",
                gflag_initmenupopup ? " WM_INITMENUPOPUP ": " ",
                gflag_entermenuloop ? "WM_INITMENULOOP ": "",
                gflag_initmenu ? "WM_INITMENU": "");

        /* invalid window */
        SetLastError(0xdeadbeef);
        gflag_initmenupopup = gflag_entermenuloop = gflag_initmenu = 0;
        ret = MyTrackPopupMenu( Ex, hmenu, TPM_RETURNCMD, 100,100, 0, NULL);
        gle = GetLastError();
        ok( !ret, "TrackPopupMenu%s should have failed\n", Ex ? "Ex" : "");
        ok( gle == ERROR_INVALID_WINDOW_HANDLE, "TrackPopupMenu%s error got %lu\n", Ex ? "Ex" : "", gle );
        ok( !(gflag_initmenupopup || gflag_entermenuloop || gflag_initmenu),
                "got unexpected message(s)%s%s%s\n",
                gflag_initmenupopup ? " WM_INITMENUPOPUP ": " ",
                gflag_entermenuloop ? "WM_INITMENULOOP ": "",
                gflag_initmenu ? "WM_INITMENU": "");

        /* now a somewhat successful call */
        SetLastError(0xdeadbeef);
        gflag_initmenupopup = gflag_entermenuloop = gflag_initmenu = 0;
        ret = MyTrackPopupMenu( Ex, hmenu, TPM_RETURNCMD, 100,100, hwnd, NULL);
        gle = GetLastError();
        ok( ret == 0, "TrackPopupMenu%s returned %d expected zero\n", Ex ? "Ex" : "", ret);
        ok( gle == NO_ERROR
            || gle == ERROR_INVALID_MENU_HANDLE /* NT4, win2k */
            || broken (gle == 0xdeadbeef) /* win95 */
            ,"TrackPopupMenu%s error got %lu expected %u or %u\n",
            Ex ? "Ex" : "", gle, NO_ERROR, ERROR_INVALID_MENU_HANDLE);
        ok( gflag_initmenupopup && gflag_entermenuloop && gflag_initmenu,
                "missed expected message(s)%s%s%s\n",
                !gflag_initmenupopup ? " WM_INITMENUPOPUP ": " ",
                !gflag_entermenuloop ? "WM_INITMENULOOP ": "",
                !gflag_initmenu ? "WM_INITMENU": "");
        /* and another */
        ret = AppendMenuA( hmenu, MF_STRING, 1, "winetest");
        ok( ret, "AppendMenA has failed!\n");
        SetLastError(0xdeadbeef);
        gflag_initmenupopup = gflag_entermenuloop = gflag_initmenu = 0;
        ret = MyTrackPopupMenu( Ex, hmenu, TPM_RETURNCMD, 100,100, hwnd, NULL);
        gle = GetLastError();
        ok( ret == 0, "TrackPopupMenu%s returned %d expected zero\n", Ex ? "Ex" : "", ret);
        ok( gle == NO_ERROR
            || gle == ERROR_INVALID_MENU_HANDLE /* NT4, win2k and Vista in the TrackPopupMenuEx case */
            || broken (gle == 0xdeadbeef) /* win95 */
            ,"TrackPopupMenu%s error got %lu expected %u or %u\n",
            Ex ? "Ex" : "", gle, NO_ERROR, ERROR_INVALID_MENU_HANDLE);
        ok( gflag_initmenupopup && gflag_entermenuloop && gflag_initmenu,
                "missed expected message(s)%s%s%s\n",
                !gflag_initmenupopup ? " WM_INITMENUPOPUP ": " ",
                !gflag_entermenuloop ? "WM_INITMENULOOP ": "",
                !gflag_initmenu ? "WM_INITMENU": "");
        DestroyMenu(hmenu);
    }
    /* clean up */
    DestroyWindow(hwnd);
}

static HMENU g_hmenu;

static LRESULT WINAPI menu_track_again_wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_ENTERMENULOOP:
    {
        BOOL ret;

        /* try a recursive call */
        SetLastError(0xdeadbeef);
        ret = TrackPopupMenu(g_hmenu, 0, 100, 100, 0, hwnd, NULL);
        ok(ret == FALSE, "got %d\n", ret);
        ok(GetLastError() == ERROR_POPUP_ALREADY_ACTIVE ||
           broken(GetLastError() == 0xdeadbeef) /* W9x */, "got %ld\n", GetLastError());

        /* exit menu modal loop
         * ( A SendMessage does not work on NT3.51 here ) */
        return PostMessageA(hwnd, WM_CANCELMODE, 0, 0);
    }
    }
    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

static void test_menu_trackagain(void)
{
    HWND hwnd;
    BOOL ret;

    hwnd = CreateWindowExA(0, (LPCSTR)MAKEINTATOM(atomMenuCheckClass), NULL,
            WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 200, 200,
            NULL, NULL, NULL, NULL);
    ok(hwnd != NULL, "CreateWindowEx failed with error %ld\n", GetLastError());
    if (!hwnd) return;
    SetWindowLongPtrA( hwnd, GWLP_WNDPROC, (LONG_PTR)menu_track_again_wnd_proc);

    g_hmenu = CreatePopupMenu();
    ok(g_hmenu != NULL, "CreateMenu failed with error %ld\n", GetLastError());

    ret = AppendMenuA(g_hmenu, MF_STRING , 100, "item 1");
    ok(ret, "AppendMenu failed.\n");
    ret = AppendMenuA(g_hmenu, MF_STRING , 101, "item 2");
    ok(ret, "AppendMenu failed.\n");

    ret = TrackPopupMenu( g_hmenu, 0, 100, 100, 0, hwnd, NULL);
    ok(ret == TRUE, "got %d\n", ret);

    DestroyMenu(g_hmenu);
    DestroyWindow(hwnd);
}

/* test handling of WM_CANCELMODE messages */
static int g_got_enteridle;
static HWND g_hwndtosend;
static LRESULT WINAPI menu_cancelmode_wnd_proc(HWND hwnd, UINT msg,
        WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
        case WM_ENTERMENULOOP:
            g_got_enteridle = 0;
            return SendMessageA( g_hwndtosend, WM_CANCELMODE, 0, 0);
        case WM_ENTERIDLE:
            {
                if( g_got_enteridle++ == 0) {
                    /* little hack to get another WM_ENTERIDLE message */
                    PostMessageA( hwnd, WM_MOUSEMOVE, 0, 0);
                    return SendMessageA( g_hwndtosend, WM_CANCELMODE, 0, 0);
                }
                EndMenu();
                return TRUE;
            }
    }
    return DefWindowProcA( hwnd, msg, wparam, lparam);
}

static void test_menu_cancelmode(void)
{
    DWORD ret;
    HWND hwnd, hwndchild;
    HMENU menu, menubar;
    MSG msg;

    hwnd = CreateWindowExA( 0, (LPCSTR)MAKEINTATOM(atomMenuCheckClass), NULL,
            WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 200, 200,
            NULL, NULL, NULL, NULL);
    hwndchild = CreateWindowExA( 0, (LPCSTR)MAKEINTATOM(atomMenuCheckClass), NULL,
            WS_VISIBLE | WS_CHILD, 10, 10, 20, 20,
            hwnd, NULL, NULL, NULL);
    ok( hwnd != NULL && hwndchild != NULL,
            "CreateWindowEx failed with error %ld\n", GetLastError());
    g_hwndtosend = hwnd;
    SetWindowLongPtrA( hwnd, GWLP_WNDPROC, (LONG_PTR)menu_cancelmode_wnd_proc);
    SetWindowLongPtrA( hwndchild, GWLP_WNDPROC, (LONG_PTR)menu_cancelmode_wnd_proc);
    menu = CreatePopupMenu();
    ok( menu != NULL, "CreatePopupMenu failed with error %ld\n", GetLastError());
    ret = AppendMenuA( menu, MF_STRING, 1, "winetest");
    ok( ret, "Functie failed lasterror is %lu\n", GetLastError());
    /* seems to be needed only on wine :( */
    while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
        DispatchMessageA(&msg);
    /* test the effect of sending a WM_CANCELMODE message in the WM_INITMENULOOP
     * handler of the menu owner */
    /* test results is extracted from variable g_got_enteridle. Possible values:
     * 0 : complete conformance. Sending WM_CANCELMODE cancels a menu initializing tracking
     * 1 : Sending WM_CANCELMODE cancels a menu that is in tracking state
     * 2 : Sending WM_CANCELMODE does not work
     */
    /* menu owner is top level window */
    g_hwndtosend = hwnd;
    TrackPopupMenu( menu, TPM_RETURNCMD, 100,100, 0, hwnd, NULL);
    ok( g_got_enteridle == 0, "received %d WM_ENTERIDLE messages, none expected\n", g_got_enteridle);
    /* menu owner is child window */
    g_hwndtosend = hwndchild;
    TrackPopupMenu( menu, TPM_RETURNCMD, 100,100, 0, hwndchild, NULL);
    ok(g_got_enteridle == 0, "received %d WM_ENTERIDLE messages, none expected\n", g_got_enteridle);
    /* now send the WM_CANCELMODE messages to the WRONG window */
    /* those should fail ( to have any effect) */
    g_hwndtosend = hwnd;
    TrackPopupMenu( menu, TPM_RETURNCMD, 100,100, 0, hwndchild, NULL);
    ok( g_got_enteridle == 2, "received %d WM_ENTERIDLE messages, should be 2\n", g_got_enteridle);

    /* test canceling tracking in a window's menu bar */
    menubar = CreateMenu();
    ok( menubar != NULL, "CreateMenu failed with error %ld\n", GetLastError());
    ret = AppendMenuA( menubar, MF_POPUP|MF_STRING, (UINT_PTR)menu, "winetest");
    ok( ret, "AppendMenuA failed lasterror is %lu\n", GetLastError());
    ret = SetMenu( hwnd, menubar );
    ok( ret, "SetMenu failed lasterror is %lu\n", GetLastError());
    /* initiate tracking */
    g_hwndtosend = hwnd;
    ret = SendMessageA( hwnd, WM_SYSCOMMAND, SC_KEYMENU, 0 );
    ok( ret == 0, "Sending WM_SYSCOMMAND/SC_KEYMENU failed lasterror is %lu\n", GetLastError());
    while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
        DispatchMessageA(&msg);
    ok(g_got_enteridle == 0, "received %d WM_ENTERIDLE messages, none expected\n", g_got_enteridle);

    /* cleanup */
    DestroyMenu( menubar );
    DestroyMenu( menu);
    DestroyWindow( hwndchild);
    DestroyWindow( hwnd);
}

/* show menu trees have a maximum depth */
static void test_menu_maxdepth(void)
{
#define NR_MENUS 100
    HMENU hmenus[ NR_MENUS];
    int i;
    DWORD ret;

    SetLastError(12345678);
    for( i = 0; i < NR_MENUS; i++) {
        hmenus[i] = CreatePopupMenu();
        if( !hmenus[i]) break;
    }
    ok( i == NR_MENUS, "could not create more than %d menu's\n", i);
    for( i = 1; i < NR_MENUS; i++) {
        ret = AppendMenuA( hmenus[i], MF_POPUP, (UINT_PTR)hmenus[i-1],"test");
        if( !ret) break;
    }
    trace("Maximum depth is %d\n", i);
    ok( GetLastError() == 12345678, "unexpected error %ld\n",  GetLastError());
    ok( i < NR_MENUS ||
           broken( i == NR_MENUS), /* win98, NT */
           "no ( or very large) limit on menu depth!\n");

    for( i = 0; i < NR_MENUS; i++)
        DestroyMenu( hmenus[i]);
}

/* bug #12171 */
static void test_menu_circref(void)
{
    HMENU menu1, menu2;
    DWORD ret;

    menu1 = CreatePopupMenu();
    menu2 = CreatePopupMenu();
    ok( menu1 && menu2, "error creating menus.\n");
    ret = AppendMenuA( menu1, MF_POPUP, (UINT_PTR)menu2, "winetest");
    ok( ret, "AppendMenu failed, error is %ld\n", GetLastError());
    ret = AppendMenuA( menu1, MF_STRING | MF_HILITE, 123, "winetest");
    ok( ret, "AppendMenu failed, error is %ld\n", GetLastError());
    /* app chooses an id that happens to clash with its own hmenu */
    ret = AppendMenuA( menu2, MF_STRING, (UINT_PTR)menu2, "winetest");
    ok( ret, "AppendMenu failed, error is %ld\n", GetLastError());
    /* now attempt to change the string of the first item of menu1 */
    ret = ModifyMenuA( menu1, (UINT_PTR)menu2, MF_POPUP, (UINT_PTR)menu2, "menu 2");
    ok( !ret ||
            broken( ret), /* win98, NT */
            "ModifyMenu should have failed.\n");
    if( !ret) { /* will probably stack fault if the ModifyMenu succeeded */
        ret = GetMenuState( menu1, 123, 0);
        ok( ret == MF_HILITE, "GetMenuState returned %lx\n",ret);
    }
    DestroyMenu( menu2);
    DestroyMenu( menu1);
}

/* test how the menu texts are aligned when the menu items have
 * different combinations of text and bitmaps (bug #13350) */
static void test_menualign(void)
{
    BYTE bmfill[300];
    HMENU menu;
    HBITMAP hbm1, hbm2, hbm3;
    MENUITEMINFOA mii = { sizeof(MENUITEMINFOA) };
    DWORD ret;
    HWND hwnd;
    MENUINFO mi = { sizeof( MENUINFO)};

    if( !winetest_interactive) {
        skip( "interactive alignment tests.\n");
        return;
    }
    hwnd = CreateWindowExA(0,
            "STATIC",
            "Menu text alignment Test\nPlease make a selection.",
            WS_OVERLAPPEDWINDOW,
            100, 100,
            300, 300,
            NULL, NULL, 0, NULL);
    ShowWindow( hwnd, SW_SHOW);
    /* create bitmaps */
    memset( bmfill, 0xcc, sizeof( bmfill));
    hbm1 = CreateBitmap( 10,10,1,1,bmfill);
    hbm2 = CreateBitmap( 20,20,1,1,bmfill);
    hbm3 = CreateBitmap( 50,6,1,1,bmfill);
    ok( hbm1 && hbm2 && hbm3, "Creating bitmaps failed\n");
    menu = CreatePopupMenu();
    ok( menu != NULL, "CreatePopupMenu() failed\n");

    mi.fMask = MIM_STYLE;
    ret = GetMenuInfo( menu, &mi);
    ok( ret, "GetMenuInfo failed: %ld\n", GetLastError());
    ok( menu != NULL, "GetMenuInfo() failed\n");
    ok( 0 == mi.dwStyle, "menuinfo style is %lx\n", mi.dwStyle);

    /* test 1 */
    mii.fMask = MIIM_BITMAP | MIIM_STRING | MIIM_ID;
    mii.wID = 1;
    mii.hbmpItem = hbm1;
    mii.dwTypeData = (LPSTR) " OK: menu texts are correctly left-aligned.";
    ret = InsertMenuItemA( menu, -1, TRUE, &mii);
    ok( ret, "InsertMenuItem() failed\n");
    mii.fMask = MIIM_BITMAP | MIIM_STRING | MIIM_ID ;
    mii.wID = 2;
    mii.hbmpItem = hbm2;
    mii.dwTypeData = (LPSTR) " FAIL: menu texts are NOT left-aligned.";
    ret = InsertMenuItemA( menu, -1, TRUE, &mii);
    ok( ret, "InsertMenuItem() failed\n");
    ret = TrackPopupMenu( menu, TPM_RETURNCMD, 110, 200, 0, hwnd, NULL);
    ok( ret != 2, "User indicated that menu text alignment test 1 failed %ld\n", ret);
    /* test 2*/
    mii.fMask = MIIM_BITMAP | MIIM_STRING | MIIM_ID;
    mii.wID = 3;
    mii.hbmpItem = hbm3;
    mii.dwTypeData = NULL;
    ret = InsertMenuItemA( menu, 0, TRUE, &mii);
    ok( ret, "InsertMenuItem() failed\n");
    mii.fMask = MIIM_BITMAP | MIIM_STRING | MIIM_ID;
    mii.wID = 1;
    mii.hbmpItem = hbm1;
    /* make the text a bit longer, to keep it readable */
    /* this bug is on winXP and reproduced on wine */
    mii.dwTypeData = (LPSTR) " OK: menu texts are to the right of the bitmaps........";
    ret = SetMenuItemInfoA( menu, 1, TRUE, &mii);
    ok( ret, "SetMenuItemInfo() failed\n");
    mii.wID = 2;
    mii.hbmpItem = hbm2;
    mii.dwTypeData = (LPSTR) " FAIL: menu texts are below the first bitmap.  ";
    ret = SetMenuItemInfoA( menu, 2, TRUE, &mii);
    ok( ret, "SetMenuItemInfo() failed\n");
    ret = TrackPopupMenu( menu, TPM_RETURNCMD, 110, 200, 0, hwnd, NULL);
    ok( ret != 2, "User indicated that menu text alignment test 2 failed %ld\n", ret);
    /* test 3 */
    mii.fMask = MIIM_TYPE | MIIM_ID;
    mii.wID = 3;
    mii.fType = MFT_BITMAP;
    mii.dwTypeData = (LPSTR) hbm3;
    ret = SetMenuItemInfoA( menu, 0, TRUE, &mii);
    ok( ret, "SetMenuItemInfo() failed\n");
    mii.fMask = MIIM_BITMAP | MIIM_STRING | MIIM_ID;
    mii.wID = 1;
    mii.hbmpItem = NULL;
    mii.dwTypeData = (LPSTR) " OK: menu texts are below the bitmap.";
    ret = SetMenuItemInfoA( menu, 1, TRUE, &mii);
    ok( ret, "SetMenuItemInfo() failed\n");
    mii.wID = 2;
    mii.hbmpItem = NULL;
    mii.dwTypeData = (LPSTR) " FAIL: menu texts are NOT below the bitmap.";
    ret = SetMenuItemInfoA( menu, 2, TRUE, &mii);
    ok( ret, "SetMenuItemInfo() failed\n");
    ret = TrackPopupMenu( menu, TPM_RETURNCMD, 110, 200, 0, hwnd, NULL);
    ok( ret != 2, "User indicated that menu text alignment test 3 failed %ld\n", ret);
    /* cleanup */
    DeleteObject( hbm1);
    DeleteObject( hbm2);
    DeleteObject( hbm3);
    DestroyMenu( menu);
    DestroyWindow( hwnd);
}

static LRESULT WINAPI menu_fill_in_init(HWND hwnd, UINT msg,
        WPARAM wparam, LPARAM lparam)
{
    HMENU hmenupopup;
    BOOL ret;
    switch (msg)
    {
        case WM_INITMENUPOPUP:
            gflag_initmenupopup++;
            hmenupopup = (HMENU) wparam;
            ret = AppendMenuA(hmenupopup, MF_STRING , 100, "item 1");
            ok(ret, "AppendMenu failed.\n");
            ret = AppendMenuA(hmenupopup, MF_STRING , 101, "item 2");
            ok(ret, "AppendMenu failed.\n");
            break;
        case WM_ENTERMENULOOP:
            gflag_entermenuloop++;
            break;
        case WM_INITMENU:
            gflag_initmenu++;
            break;
        case WM_ENTERIDLE:
            gflag_enteridle++;
            PostMessageA(hwnd, WM_CANCELMODE, 0, 0);
            return TRUE;
        case WM_MENUSELECT:
            selectitem_wp = wparam;
            selectitem_lp = lparam;
            break;
    }
    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

static void test_emptypopup(void)
{
    BOOL ret;
    HMENU hmenu;

    HWND hwnd = CreateWindowExA(0, (LPCSTR)MAKEINTATOM(atomMenuCheckClass), NULL,
                               WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 200, 200,
                               NULL, NULL, NULL, NULL);
    ok(hwnd != NULL, "CreateWindowEx failed with error %ld\n", GetLastError());
    SetWindowLongPtrA( hwnd, GWLP_WNDPROC, (LONG_PTR)menu_ownerdraw_wnd_proc);

    hmenu = CreatePopupMenu();
    ok(hmenu != NULL, "CreateMenu failed with error %ld\n", GetLastError());

    gflag_initmenupopup = gflag_entermenuloop = gflag_initmenu = gflag_enteridle = 0;
    selectitem_wp = 0xdeadbeef;
    selectitem_lp = 0xdeadbeef;

    ret = TrackPopupMenu( hmenu, TPM_RETURNCMD, 100,100, 0, hwnd, NULL);
    ok(ret == 0, "got %i\n", ret);

    ok(gflag_initmenupopup == 1, "got %i\n", gflag_initmenupopup);
    ok(gflag_entermenuloop == 1, "got %i\n", gflag_entermenuloop);
    ok(gflag_initmenu == 1, "got %i\n", gflag_initmenu);
    ok(gflag_enteridle == 0, "got %i\n", gflag_initmenu);

    ok(selectitem_wp == 0xdeadbeef, "got %Ix\n", selectitem_wp);
    ok(selectitem_lp == 0xdeadbeef, "got %Ix\n", selectitem_lp);

    gflag_initmenupopup = gflag_entermenuloop = gflag_initmenu = gflag_enteridle = 0;
    selectitem_wp = 0xdeadbeef;
    selectitem_lp = 0xdeadbeef;

    ret = TrackPopupMenu( hmenu, 0, 100,100, 0, hwnd, NULL);
    ok(ret == 0, "got %i\n", ret);

    ok(gflag_initmenupopup == 1, "got %i\n", gflag_initmenupopup);
    ok(gflag_entermenuloop == 1, "got %i\n", gflag_entermenuloop);
    ok(gflag_initmenu == 1, "got %i\n", gflag_initmenu);
    ok(gflag_enteridle == 0, "got %i\n", gflag_initmenu);

    ok(selectitem_wp == 0xdeadbeef, "got %Ix\n", selectitem_wp);
    ok(selectitem_lp == 0xdeadbeef, "got %Ix\n", selectitem_lp);

    SetWindowLongPtrA( hwnd, GWLP_WNDPROC, (LONG_PTR)menu_fill_in_init);

    gflag_initmenupopup = gflag_entermenuloop = gflag_initmenu = gflag_enteridle = 0;
    selectitem_wp = 0xdeadbeef;
    selectitem_lp = 0xdeadbeef;

    ret = TrackPopupMenu( hmenu, 0, 100,100, 0, hwnd, NULL);
    ok(ret == 1, "got %i\n", ret);

    ok(gflag_initmenupopup == 1, "got %i\n", gflag_initmenupopup);
    ok(gflag_entermenuloop == 1, "got %i\n", gflag_entermenuloop);
    ok(gflag_initmenu == 1, "got %i\n", gflag_initmenu);
    ok(gflag_enteridle == 1, "got %i\n", gflag_initmenu);

    ok(selectitem_wp == 0xffff0000, "got %Ix\n", selectitem_wp);
    ok(selectitem_lp == 0, "got %Ix\n", selectitem_lp);

    DestroyWindow(hwnd);

    ret = DestroyMenu(hmenu);
    ok(ret, "DestroyMenu failed with error %ld\n", GetLastError());
}

static HMENU get_bad_hmenu( UINT_PTR id )
{
    while (IsMenu( (HMENU)id )) id++;
    return (HMENU)id;
}

static void test_AppendMenu(void)
{
    static char string[] = "string";
    MENUITEMINFOA mii;
    HMENU hmenu, hsubmenu;
    HBITMAP hbmp;
    char buf[16];
    BOOL ret;

    hmenu = CreateMenu();
    ok(hmenu != 0, "CreateMenu failed\n");
    ret = AppendMenuA(hmenu, MF_OWNERDRAW, 201, "item 1");
    ok(ret, "AppendMenu failed\n");
    check_menu_items(hmenu, 201, MF_OWNERDRAW, 0);
    DestroyMenu(hmenu);

    hmenu = CreateMenu();
    ok(hmenu != 0, "CreateMenu failed\n");
    hsubmenu = get_bad_hmenu( 202 );
    ret = AppendMenuA(hmenu, MF_POPUP, (UINT_PTR)hsubmenu, "item 1");
    ok(ret, "AppendMenu failed\n");
    check_menu_items(hmenu, (UINT_PTR)hsubmenu, MF_STRING, 0);
    DestroyMenu(hmenu);

    hmenu = CreateMenu();
    ok(hmenu != 0, "CreateMenu failed\n");
    hsubmenu = get_bad_hmenu( 203 );
    ret = AppendMenuA(hmenu, MF_OWNERDRAW | MF_POPUP, (UINT_PTR)hsubmenu, "item 1");
    ok(ret, "AppendMenu failed\n");
    check_menu_items(hmenu, (UINT_PTR)hsubmenu, MF_OWNERDRAW, 0);
    DestroyMenu(hmenu);

    hmenu = CreateMenu();
    ok(hmenu != 0, "CreateMenu failed\n");
    hsubmenu = CreateMenu();
    ok(hsubmenu != 0, "CreateMenu failed\n");
    ret = AppendMenuA(hmenu, MF_OWNERDRAW, (UINT_PTR)hsubmenu, "item 1");
    ok(ret, "AppendMenu failed\n");
    check_menu_items(hmenu, (UINT_PTR)hsubmenu, MF_OWNERDRAW, 0);
    DestroyMenu(hmenu);
    DestroyMenu(hsubmenu);

    hmenu = CreateMenu();
    ok(hmenu != 0, "CreateMenu failed\n");
    hsubmenu = CreateMenu();
    ok(hsubmenu != 0, "CreateMenu failed\n");
    ret = AppendMenuA(hmenu, MF_POPUP, (UINT_PTR)hsubmenu, "item 1");
    ok(ret, "AppendMenu failed\n");
    check_menu_items(hmenu, (UINT_PTR)hsubmenu, MF_STRING, 0);
    DestroyMenu(hmenu);
    DestroyMenu(hsubmenu);

    hmenu = CreateMenu();
    ok(hmenu != 0, "CreateMenu failed\n");
    hsubmenu = CreateMenu();
    ok(hsubmenu != 0, "CreateMenu failed\n");
    ret = AppendMenuA(hmenu, MF_OWNERDRAW | MF_POPUP, (UINT_PTR)hsubmenu, "item 1");
    ok(ret, "AppendMenu failed\n");
    check_menu_items(hmenu, (UINT_PTR)hsubmenu, MF_OWNERDRAW, 0);
    DestroyMenu(hmenu);
    DestroyMenu(hsubmenu);

    hmenu = CreateMenu();
    ok(hmenu != 0, "CreateMenu failed\n");
    hsubmenu = CreatePopupMenu();
    ok(hsubmenu != 0, "CreatePopupMenu failed\n");
    ret = AppendMenuA(hmenu, MF_OWNERDRAW, (UINT_PTR)hsubmenu, "item 1");
    ok(ret, "AppendMenu failed\n");
    check_menu_items(hmenu, (UINT_PTR)hsubmenu, MF_OWNERDRAW, 0);
    DestroyMenu(hmenu);
    DestroyMenu(hsubmenu);

    hmenu = CreateMenu();
    ok(hmenu != 0, "CreateMenu failed\n");
    hsubmenu = CreatePopupMenu();
    ok(hsubmenu != 0, "CreatePopupMenu failed\n");
    ret = AppendMenuA(hmenu, MF_POPUP, (UINT_PTR)hsubmenu, "item 1");
    ok(ret, "AppendMenu failed\n");
    check_menu_items(hmenu, (UINT_PTR)hsubmenu, MF_STRING, 0);
    DestroyMenu(hmenu);
    DestroyMenu(hsubmenu);

    hmenu = CreateMenu();
    ok(hmenu != 0, "CreateMenu failed\n");
    hsubmenu = CreatePopupMenu();
    ok(hsubmenu != 0, "CreatePopupMenu failed\n");
    ret = AppendMenuA(hmenu, MF_OWNERDRAW | MF_POPUP, (UINT_PTR)hsubmenu, "item 1");
    ok(ret, "AppendMenu failed\n");
    check_menu_items(hmenu, (UINT_PTR)hsubmenu, MF_OWNERDRAW, 0);
    DestroyMenu(hmenu);
    DestroyMenu(hsubmenu);

    hmenu = CreateMenu();
    ok(hmenu != 0, "CreateMenu failed\n");
    ret = AppendMenuA(hmenu, MF_STRING, 204, "item 1");
    ok(ret, "AppendMenu failed\n");
    check_menu_items(hmenu, 204, MF_STRING, 0);
    hsubmenu = get_bad_hmenu( 205 );
    ret = ModifyMenuA(hmenu, 0, MF_POPUP | MF_BYPOSITION, (UINT_PTR)hsubmenu, "item 2");
    ok(ret, "ModifyMenu failed\n");
    check_menu_items(hmenu, (UINT_PTR)hsubmenu, MF_STRING, 0);
    memset(&mii, 0, sizeof(mii));
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_SUBMENU;
    mii.hSubMenu = get_bad_hmenu( 204 );
    ret = InsertMenuItemA(hmenu, 0, TRUE, &mii);
    ok(!ret, "InsertMenuItem should fail\n");
    ret = SetMenuItemInfoA(hmenu, 0, TRUE, &mii);
    ok(!ret, "SetMenuItemInfo should fail\n");
    mii.fMask = MIIM_ID;
    mii.wID = 206;
    ret = InsertMenuItemA(hmenu, 0, TRUE, &mii);
    ok(ret, "InsertMenuItem failed\n");
if (0) /* FIXME: uncomment once Wine is fixed */ {
    check_menu_items(hmenu, 206, MF_SEPARATOR, MFS_GRAYED);
}
    mii.wID = 207;
    ret = SetMenuItemInfoA(hmenu, 0, TRUE, &mii);
    ok(ret, "SetMenuItemInfo failed\n");
if (0) /* FIXME: uncomment once Wine is fixed */ {
    check_menu_items(hmenu, 207, MF_SEPARATOR, MFS_GRAYED);
}
    DestroyMenu(hmenu);

    hbmp = CreateBitmap(1, 1, 1, 1, NULL);
    hmenu = CreateMenu();
    ok(hmenu != 0, "CreateMenu failed\n");

    /* menu item with a string and a bitmap */
    memset(&mii, 0, sizeof(mii));
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_STRING | MIIM_BITMAP;
    mii.dwTypeData = string;
    mii.cch = strlen(string);
    mii.hbmpItem = hbmp;
    ret = InsertMenuItemA(hmenu, 0, TRUE, &mii);
    ok(ret, "InsertMenuItem failed\n");
    memset(&mii, 0x81, sizeof(mii));
    memset(buf, 0x81, sizeof(buf));
    mii.cbSize = sizeof(mii);
    mii.fMask  = MIIM_FTYPE | MIIM_STATE | MIIM_ID | MIIM_SUBMENU | MIIM_STRING | MIIM_BITMAP;
    mii.dwTypeData = buf;
    mii.cch = sizeof(buf);
    mii.dwItemData = 0x81818181;
    ret = GetMenuItemInfoA(hmenu, 0, TRUE, &mii);
    ok(ret, "GetMenuItemInfo failed\n");
    ok(mii.fType == MF_STRING, "expected MF_STRING, got %#x\n", mii.fType);
    ok(mii.fState == MF_ENABLED, "expected MF_ENABLED, got %#x\n", mii.fState);
    ok(mii.wID == 0, "expected 0, got %#x\n", mii.wID);
    ok(mii.hSubMenu == 0, "expected 0, got %p\n", mii.hSubMenu);
    ok(mii.dwItemData == 0x81818181, "expected 0x81818181, got %#Ix\n", mii.dwItemData);
    ok(mii.dwTypeData == buf, "expected %p, got %p\n", buf, mii.dwTypeData);
    ok(mii.cch == 6, "expected 6, got %u\n", mii.cch);
    ok(!strcmp(buf, string), "expected %s, got %s\n", string, buf);
    ok(mii.hbmpItem == hbmp, "expected %p, got %p\n", hbmp, mii.hbmpItem);

    memset(&mii, 0x81, sizeof(mii));
    mii.cbSize = sizeof(mii);
    mii.fMask  = MIIM_TYPE;
    mii.dwItemData = 0x81818181;
    ret = GetMenuItemInfoA(hmenu, 0, TRUE, &mii);
    ok(ret, "GetMenuItemInfo failed\n");
    ok(mii.fType == MF_BITMAP, "expected MF_BITMAP, got %#x\n", mii.fType);
    ok(mii.fState == 0x81818181, "expected 0x81818181, got %#x\n", mii.fState);
    ok(mii.wID == 0x81818181, "expected 0x81818181, got %#x\n", mii.wID);
    ok(mii.hSubMenu == 0, "expected 0, got %p\n", mii.hSubMenu);
    ok(mii.dwItemData == 0x81818181, "expected 0x81818181, got %#Ix\n", mii.dwItemData);
    ok(mii.dwTypeData == (LPSTR)hbmp, "expected %p, got %p\n", hbmp, mii.dwTypeData);
    ok(mii.cch == 6, "expected 6, got %u\n", mii.cch);
    ok(mii.hbmpItem == hbmp, "expected %p, got %p\n", hbmp, mii.hbmpItem);

    memset(&mii, 0x81, sizeof(mii));
    memset(buf, 0x81, sizeof(buf));
    mii.cbSize = sizeof(mii);
    mii.fMask  = MIIM_STATE | MIIM_ID | MIIM_TYPE | MIIM_DATA;
    mii.dwTypeData = buf;
    mii.cch = sizeof(buf);
    mii.dwItemData = 0x81818181;
    ret = GetMenuItemInfoA(hmenu, 0, TRUE, &mii);
    ok(ret, "GetMenuItemInfo failed\n");
    ok(mii.fType == MF_BITMAP, "expected MF_BITMAP, got %#x\n", mii.fType);
    ok(mii.fState == MF_ENABLED, "expected MF_ENABLED, got %#x\n", mii.fState);
    ok(mii.wID == 0, "expected 0, got %#x\n", mii.wID);
    ok(mii.hSubMenu == 0, "expected 0, got %p\n", mii.hSubMenu);
    ok(mii.dwItemData == 0, "expected 0, got %#Ix\n", mii.dwItemData);
    ok(mii.dwTypeData == (LPSTR)hbmp, "expected %p, got %p\n", hbmp, mii.dwTypeData);
    ok(mii.cch == 6, "expected 6, got %u\n", mii.cch);
    ok(mii.hbmpItem == hbmp, "expected %p, got %p\n", hbmp, mii.hbmpItem);

    DestroyMenu(hmenu);
    DeleteObject(hbmp);

    hmenu = CreateMenu();
    ok(hmenu != 0, "CreateMenu failed\n");

    /* menu item with a string and a "magic" bitmap */
    memset(&mii, 0, sizeof(mii));
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_STRING | MIIM_BITMAP;
    mii.dwTypeData = string;
    mii.cch = strlen(string);
    mii.hbmpItem = HBMMENU_POPUP_RESTORE;
    ret = InsertMenuItemA(hmenu, 0, TRUE, &mii);
    ok(ret, "InsertMenuItem failed\n");
    memset(&mii, 0x81, sizeof(mii));
    memset(buf, 0x81, sizeof(buf));
    mii.cbSize = sizeof(mii);
    mii.fMask  = MIIM_FTYPE | MIIM_STATE | MIIM_ID | MIIM_SUBMENU | MIIM_STRING | MIIM_BITMAP;
    mii.dwTypeData = buf;
    mii.cch = sizeof(buf);
    mii.dwItemData = 0x81818181;
    ret = GetMenuItemInfoA(hmenu, 0, TRUE, &mii);
    ok(ret, "GetMenuItemInfo failed\n");
    ok(mii.fType == MF_STRING, "expected MF_STRING, got %#x\n", mii.fType);
    ok(mii.fState == MF_ENABLED, "expected MF_ENABLED, got %#x\n", mii.fState);
    ok(mii.wID == 0, "expected 0, got %#x\n", mii.wID);
    ok(mii.hSubMenu == 0, "expected 0, got %p\n", mii.hSubMenu);
    ok(mii.dwItemData == 0x81818181, "expected 0x81818181, got %#Ix\n", mii.dwItemData);
    ok(mii.dwTypeData == buf, "expected %p, got %p\n", buf, mii.dwTypeData);
    ok(mii.cch == 6, "expected 6, got %u\n", mii.cch);
    ok(!strcmp(buf, string), "expected %s, got %s\n", string, buf);
    ok(mii.hbmpItem == HBMMENU_POPUP_RESTORE, "expected HBMMENU_POPUP_RESTORE, got %p\n", mii.hbmpItem);

    memset(&mii, 0x81, sizeof(mii));
    mii.cbSize = sizeof(mii);
    mii.fMask  = MIIM_TYPE;
    mii.dwTypeData = buf;
    mii.cch = sizeof(buf);
    mii.dwItemData = 0x81818181;
    ret = GetMenuItemInfoA(hmenu, 0, TRUE, &mii);
    ok(ret, "GetMenuItemInfo failed\n");
    ok(mii.fType == MF_STRING, "expected MF_STRING, got %#x\n", mii.fType);
    ok(mii.fState == 0x81818181, "expected 0x81818181, got %#x\n", mii.fState);
    ok(mii.wID == 0x81818181, "expected 0x81818181, got %#x\n", mii.wID);
    ok(mii.hSubMenu == 0, "expected 0, got %p\n", mii.hSubMenu);
    ok(mii.dwItemData == 0x81818181, "expected 0x81818181, got %#Ix\n", mii.dwItemData);
    ok(mii.dwTypeData == buf, "expected %p, got %p\n", buf, mii.dwTypeData);
    ok(mii.cch == 6, "expected 6, got %u\n", mii.cch);
    ok(!strcmp(buf, string), "expected %s, got %s\n", string, buf);
    ok(mii.hbmpItem == HBMMENU_POPUP_RESTORE, "expected HBMMENU_POPUP_RESTORE, got %p\n", mii.hbmpItem);

    memset(&mii, 0x81, sizeof(mii));
    memset(buf, 0x81, sizeof(buf));
    mii.cbSize = sizeof(mii);
    mii.fMask  = MIIM_STATE | MIIM_ID | MIIM_TYPE | MIIM_DATA;
    mii.dwTypeData = buf;
    mii.cch = sizeof(buf);
    mii.dwItemData = 0x81818181;
    ret = GetMenuItemInfoA(hmenu, 0, TRUE, &mii);
    ok(ret, "GetMenuItemInfo failed\n");
    ok(mii.fType == MF_STRING, "expected MF_STRING, got %#x\n", mii.fType);
    ok(mii.fState == MF_ENABLED, "expected MF_ENABLED, got %#x\n", mii.fState);
    ok(mii.wID == 0, "expected 0, got %#x\n", mii.wID);
    ok(mii.hSubMenu == 0, "expected 0, got %p\n", mii.hSubMenu);
    ok(mii.dwItemData == 0, "expected 0, got %#Ix\n", mii.dwItemData);
    ok(mii.dwTypeData == buf, "expected %p, got %p\n", buf, mii.dwTypeData);
    ok(mii.cch == 6, "expected 6, got %u\n", mii.cch);
    ok(!strcmp(buf, string), "expected %s, got %s\n", string, buf);
    ok(mii.hbmpItem == HBMMENU_POPUP_RESTORE, "expected HBMMENU_POPUP_RESTORE, got %p\n", mii.hbmpItem);

    DestroyMenu(hmenu);

    hbmp = CreateBitmap(1, 1, 1, 1, NULL);
    hmenu = CreateMenu();
    ok(hmenu != 0, "CreateMenu failed\n");

    /* menu item with a string */
    memset(&mii, 0, sizeof(mii));
    mii.cbSize = sizeof(mii);
    mii.fMask  = MIIM_STATE | MIIM_ID | MIIM_TYPE | MIIM_DATA;
    mii.dwItemData = (ULONG_PTR)hbmp;
    mii.dwTypeData = string;
    mii.cch = strlen(string);
    mii.hbmpItem = hbmp;
    ret = InsertMenuItemA(hmenu, 0, TRUE, &mii);
    ok(ret, "InsertMenuItem failed\n");
    memset(&mii, 0x81, sizeof(mii));
    memset(buf, 0x81, sizeof(buf));
    mii.cbSize = sizeof(mii);
    mii.fMask  = MIIM_STATE | MIIM_ID | MIIM_TYPE | MIIM_DATA;
    mii.dwTypeData = buf;
    mii.cch = sizeof(buf);
    mii.dwItemData = 0x81818181;
    ret = GetMenuItemInfoA(hmenu, 0, TRUE, &mii);
    ok(ret, "GetMenuItemInfo failed\n");
    ok(mii.fType == MF_STRING, "expected MF_STRING, got %#x\n", mii.fType);
    ok(mii.fState == MF_ENABLED, "expected MF_ENABLED, got %#x\n", mii.fState);
    ok(mii.wID == 0, "expected 0, got %#x\n", mii.wID);
    ok(mii.hSubMenu == 0, "expected 0, got %p\n", mii.hSubMenu);
    ok(mii.dwItemData == (ULONG_PTR)hbmp, "expected %p, got %#Ix\n", hbmp, mii.dwItemData);
    ok(mii.dwTypeData == buf, "expected %p, got %p\n", buf, mii.dwTypeData);
    ok(mii.cch == 6, "expected 6, got %u\n", mii.cch);
    ok(!strcmp(buf, string), "expected %s, got %s\n", string, buf);
    ok(mii.hbmpItem == 0, "expected 0, got %p\n", mii.hbmpItem);

    memset(&mii, 0x81, sizeof(mii));
    memset(buf, 0x81, sizeof(buf));
    mii.cbSize = sizeof(mii);
    mii.fMask  = MIIM_TYPE;
    mii.dwTypeData = buf;
    mii.cch = sizeof(buf);
    mii.dwItemData = 0x81818181;
    ret = GetMenuItemInfoA(hmenu, 0, TRUE, &mii);
    ok(ret, "GetMenuItemInfo failed\n");
    ok(mii.fType == MF_STRING, "expected MF_STRING, got %#x\n", mii.fType);
    ok(mii.fState == 0x81818181, "expected 0x81818181, got %#x\n", mii.fState);
    ok(mii.wID == 0x81818181, "expected 0x81818181, got %#x\n", mii.wID);
    ok(mii.hSubMenu == 0, "expected 0, got %p\n", mii.hSubMenu);
    ok(mii.dwItemData == 0x81818181, "expected 0x81818181, got %#Ix\n", mii.dwItemData);
    ok(mii.dwTypeData == buf, "expected %p, got %p\n", buf, mii.dwTypeData);
    ok(mii.cch == 6, "expected 6, got %u\n", mii.cch);
    ok(!strcmp(buf, string), "expected %s, got %s\n", string, buf);
    ok(mii.hbmpItem == 0, "expected 0, got %p\n", mii.hbmpItem);

    DestroyMenu(hmenu);
    DeleteObject(hbmp);

    /* menu item with a string */
    hbmp = CreateBitmap(1, 1, 1, 1, NULL);
    hmenu = CreateMenu();
    ok(hmenu != 0, "CreateMenu failed\n");
    memset(&mii, 0, sizeof(mii));
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_STRING;
    mii.dwTypeData = string;
    mii.cch = strlen(string);
    ret = InsertMenuItemA(hmenu, 0, TRUE, &mii);
    ok(ret, "InsertMenuItem failed\n");
    memset(&mii, 0x81, sizeof(mii));
    memset(buf, 0x81, sizeof(buf));
    mii.cbSize = sizeof(mii);
    mii.fMask  = MIIM_FTYPE | MIIM_STATE | MIIM_ID | MIIM_SUBMENU | MIIM_STRING | MIIM_BITMAP;
    mii.dwTypeData = buf;
    mii.cch = sizeof(buf);
    mii.dwItemData = 0x81818181;
    ret = GetMenuItemInfoA(hmenu, 0, TRUE, &mii);
    ok(ret, "GetMenuItemInfo failed\n");
    ok(mii.fType == MF_STRING, "expected MF_STRING, got %#x\n", mii.fType);
    ok(mii.fState == MF_ENABLED, "expected MF_ENABLED, got %#x\n", mii.fState);
    ok(mii.wID == 0, "expected 0, got %#x\n", mii.wID);
    ok(mii.hSubMenu == 0, "expected 0, got %p\n", mii.hSubMenu);
    ok(mii.dwItemData == 0x81818181, "expected 0x81818181, got %#Ix\n", mii.dwItemData);
    ok(mii.dwTypeData == buf, "expected %p, got %p\n", buf, mii.dwTypeData);
    ok(mii.cch == 6, "expected 6, got %u\n", mii.cch);
    ok(!strcmp(buf, string), "expected %s, got %s\n", string, buf);
    ok(mii.hbmpItem == 0, "expected 0, got %p\n", mii.hbmpItem);

    /* add "magic" bitmap to a menu item */
    mii.fMask = MIIM_BITMAP;
    mii.hbmpItem = HBMMENU_POPUP_RESTORE;
    ret = SetMenuItemInfoA(hmenu, 0, TRUE, &mii);
    ok(ret, "SetMenuItemInfo failed\n");
    memset(&mii, 0x81, sizeof(mii));
    memset(buf, 0x81, sizeof(buf));
    mii.cbSize = sizeof(mii);
    mii.fMask  = MIIM_FTYPE | MIIM_STATE | MIIM_ID | MIIM_SUBMENU | MIIM_STRING | MIIM_BITMAP;
    mii.dwTypeData = buf;
    mii.cch = sizeof(buf);
    mii.dwItemData = 0x81818181;
    ret = GetMenuItemInfoA(hmenu, 0, TRUE, &mii);
    ok(ret, "GetMenuItemInfo failed\n");
    ok(mii.fType == MF_STRING, "expected MF_STRING, got %#x\n", mii.fType);
    ok(mii.fState == MF_ENABLED, "expected MF_ENABLED, got %#x\n", mii.fState);
    ok(mii.wID == 0, "expected 0, got %#x\n", mii.wID);
    ok(mii.hSubMenu == 0, "expected 0, got %p\n", mii.hSubMenu);
    ok(mii.dwItemData == 0x81818181, "expected 0x81818181, got %#Ix\n", mii.dwItemData);
    ok(mii.dwTypeData == buf, "expected %p, got %p\n", buf, mii.dwTypeData);
    ok(mii.cch == 6, "expected 6, got %u\n", mii.cch);
    ok(!strcmp(buf, string), "expected %s, got %s\n", string, buf);
    ok(mii.hbmpItem == HBMMENU_POPUP_RESTORE, "expected HBMMENU_POPUP_RESTORE, got %p\n", mii.hbmpItem);

    memset(&mii, 0x81, sizeof(mii));
    memset(buf, 0x81, sizeof(buf));
    mii.cbSize = sizeof(mii);
    mii.fMask  = MIIM_TYPE;
    mii.dwTypeData = buf;
    mii.cch = sizeof(buf);
    mii.dwItemData = 0x81818181;
    ret = GetMenuItemInfoA(hmenu, 0, TRUE, &mii);
    ok(ret, "GetMenuItemInfo failed\n");
    ok(mii.fType == MF_STRING, "expected MF_STRING, got %#x\n", mii.fType);
    ok(mii.fState == 0x81818181, "expected 0x81818181, got %#x\n", mii.fState);
    ok(mii.wID == 0x81818181, "expected 0x81818181, got %#x\n", mii.wID);
    ok(mii.hSubMenu == 0, "expected 0, got %p\n", mii.hSubMenu);
    ok(mii.dwItemData == 0x81818181, "expected 0x81818181, got %#Ix\n", mii.dwItemData);
    ok(mii.dwTypeData == buf, "expected %p, got %p\n", buf, mii.dwTypeData);
    ok(mii.cch == 6, "expected 6, got %u\n", mii.cch);
    ok(!strcmp(buf, string), "expected %s, got %s\n", string, buf);
    ok(mii.hbmpItem == HBMMENU_POPUP_RESTORE, "expected HBMMENU_POPUP_RESTORE, got %p\n", mii.hbmpItem);

    /* replace "magic" bitmap by a normal one */
    mii.fMask = MIIM_BITMAP;
    mii.hbmpItem = hbmp;
    ret = SetMenuItemInfoA(hmenu, 0, TRUE, &mii);
    ok(ret, "SetMenuItemInfo failed\n");
    memset(&mii, 0x81, sizeof(mii));
    memset(buf, 0x81, sizeof(buf));
    mii.cbSize = sizeof(mii);
    mii.fMask  = MIIM_FTYPE | MIIM_STATE | MIIM_ID | MIIM_SUBMENU | MIIM_STRING | MIIM_BITMAP;
    mii.dwTypeData = buf;
    mii.cch = sizeof(buf);
    mii.dwItemData = 0x81818181;
    ret = GetMenuItemInfoA(hmenu, 0, TRUE, &mii);
    ok(ret, "GetMenuItemInfo failed\n");
    ok(mii.fType == MF_STRING, "expected MF_STRING, got %#x\n", mii.fType);
    ok(mii.fState == MF_ENABLED, "expected MF_ENABLED, got %#x\n", mii.fState);
    ok(mii.wID == 0, "expected 0, got %#x\n", mii.wID);
    ok(mii.hSubMenu == 0, "expected 0, got %p\n", mii.hSubMenu);
    ok(mii.dwItemData == 0x81818181, "expected 0x81818181, got %#Ix\n", mii.dwItemData);
    ok(mii.dwTypeData == buf, "expected %p, got %p\n", buf, mii.dwTypeData);
    ok(mii.cch == 6, "expected 6, got %u\n", mii.cch);
    ok(!strcmp(buf, string), "expected %s, got %s\n", string, buf);
    ok(mii.hbmpItem == hbmp, "expected %p, got %p\n", hbmp, mii.hbmpItem);

    memset(&mii, 0x81, sizeof(mii));
    memset(buf, 0x81, sizeof(buf));
    mii.cbSize = sizeof(mii);
    mii.fMask  = MIIM_TYPE;
    mii.dwTypeData = buf;
    mii.cch = sizeof(buf);
    mii.dwItemData = 0x81818181;
    ret = GetMenuItemInfoA(hmenu, 0, TRUE, &mii);
    ok(ret, "GetMenuItemInfo failed\n");
    ok(mii.fType == MF_BITMAP, "expected MF_BITMAP, got %#x\n", mii.fType);
    ok(mii.fState == 0x81818181, "expected 0x81818181, got %#x\n", mii.fState);
    ok(mii.wID == 0x81818181, "expected 0x81818181, got %#x\n", mii.wID);
    ok(mii.hSubMenu == 0, "expected 0, got %p\n", mii.hSubMenu);
    ok(mii.dwItemData == 0x81818181, "expected 0x81818181, got %#Ix\n", mii.dwItemData);
    ok(mii.dwTypeData == (LPSTR)hbmp, "expected %p, got %p\n", hbmp, mii.dwTypeData);
    ok(mii.cch == 6, "expected 6, got %u\n", mii.cch);
    ok(mii.hbmpItem == hbmp, "expected %p, got %p\n", hbmp, mii.hbmpItem);

    DestroyMenu(hmenu);
    DeleteObject(hbmp);
}

START_TEST(menu)
{
    register_menu_check_class();

    test_menu_add_string();
    test_menu_iteminfo();
    test_menu_search_bycommand();
    test_CheckMenuRadioItem();
    test_menu_resource_layout();
    test_InsertMenu();
    test_menualign();
    test_system_menu();

    test_menu_locked_by_window();
    test_subpopup_locked_by_menu();
    test_menu_ownerdraw();
    test_getmenubarinfo();
    test_GetMenuItemRect();
    test_menu_bmp_and_string();
    test_menu_getmenuinfo();
    test_menu_setmenuinfo();
    test_menu_input();
    test_menu_flags();

    test_menu_hilitemenuitem();
    test_menu_trackpopupmenu();
    test_menu_trackagain();
    test_menu_cancelmode();
    test_menu_maxdepth();
    test_menu_circref();
    test_emptypopup();
    test_AppendMenu();
}
