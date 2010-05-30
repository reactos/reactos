/* Unit test suite for property sheet control.
 *
 * Copyright 2006 Huw Davies
 * Copyright 2009 Jan de Mooij
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

#include "resources.h"

#include "wine/test.h"

static HWND parent;
static HWND sheethwnd;

static LONG active_page = -1;

#define IDC_APPLY_BUTTON 12321

static int CALLBACK sheet_callback(HWND hwnd, UINT msg, LPARAM lparam)
{
    switch(msg)
    {
    case PSCB_INITIALIZED:
      {
        char caption[256];
        GetWindowTextA(hwnd, caption, sizeof(caption));
        ok(!strcmp(caption,"test caption"), "caption: %s\n", caption);
        sheethwnd = hwnd;
        return 0;
      }
    }
    return 0;
}

static INT_PTR CALLBACK page_dlg_proc(HWND hwnd, UINT msg, WPARAM wparam,
                                      LPARAM lparam)
{
    switch(msg)
    {
    case WM_INITDIALOG:
      {
        HWND sheet = GetParent(hwnd);
        char caption[256];
        GetWindowTextA(sheet, caption, sizeof(caption));
        ok(!strcmp(caption,"test caption"), "caption: %s\n", caption);
        return TRUE;
      }

    case WM_NOTIFY:
      {
        NMHDR *nmhdr = (NMHDR *)lparam;
        switch(nmhdr->code)
        {
        case PSN_APPLY:
            return TRUE;
        default:
            return FALSE;
        }
      }
    case WM_NCDESTROY:
        ok(!SendMessageA(sheethwnd, PSM_INDEXTOHWND, 400, 0),"Should always be 0\n");
        return TRUE;

    default:
        return FALSE;
    }
}

static void test_title(void)
{
    HPROPSHEETPAGE hpsp[1];
    PROPSHEETPAGEA psp;
    PROPSHEETHEADERA psh;
    HWND hdlg;

    memset(&psp, 0, sizeof(psp));
    psp.dwSize = sizeof(psp);
    psp.dwFlags = 0;
    psp.hInstance = GetModuleHandleA(NULL);
    U(psp).pszTemplate = "prop_page1";
    U2(psp).pszIcon = NULL;
    psp.pfnDlgProc = page_dlg_proc;
    psp.lParam = 0;

    hpsp[0] = CreatePropertySheetPageA(&psp);

    memset(&psh, 0, sizeof(psh));
    psh.dwSize = sizeof(psh);
    psh.dwFlags = PSH_MODELESS | PSH_USECALLBACK;
    psh.pszCaption = "test caption";
    psh.nPages = 1;
    psh.hwndParent = GetDesktopWindow();
    U3(psh).phpage = hpsp;
    psh.pfnCallback = sheet_callback;

    hdlg = (HWND)PropertySheetA(&psh);
    if (hdlg == INVALID_HANDLE_VALUE)
    {
        win_skip("comctl32 4.70 needs dwSize adjustment\n");
        psh.dwSize = sizeof(psh) - sizeof(HBITMAP) - sizeof(HPALETTE) - sizeof(HBITMAP);
        hdlg = (HWND)PropertySheetA(&psh);
    }
    DestroyWindow(hdlg);
}

static void test_nopage(void)
{
    HPROPSHEETPAGE hpsp[1];
    PROPSHEETPAGEA psp;
    PROPSHEETHEADERA psh;
    HWND hdlg;

    memset(&psp, 0, sizeof(psp));
    psp.dwSize = sizeof(psp);
    psp.dwFlags = 0;
    psp.hInstance = GetModuleHandleA(NULL);
    U(psp).pszTemplate = "prop_page1";
    U2(psp).pszIcon = NULL;
    psp.pfnDlgProc = page_dlg_proc;
    psp.lParam = 0;

    hpsp[0] = CreatePropertySheetPageA(&psp);

    memset(&psh, 0, sizeof(psh));
    psh.dwSize = sizeof(psh);
    psh.dwFlags = PSH_MODELESS | PSH_USECALLBACK;
    psh.pszCaption = "test caption";
    psh.nPages = 1;
    psh.hwndParent = GetDesktopWindow();
    U3(psh).phpage = hpsp;
    psh.pfnCallback = sheet_callback;

    hdlg = (HWND)PropertySheetA(&psh);
    if (hdlg == INVALID_HANDLE_VALUE)
    {
        win_skip("comctl32 4.70 needs dwSize adjustment\n");
        psh.dwSize = sizeof(psh) - sizeof(HBITMAP) - sizeof(HPALETTE) - sizeof(HBITMAP);
        hdlg = (HWND)PropertySheetA(&psh);
    }
    ShowWindow(hdlg,SW_NORMAL);
    SendMessage(hdlg, PSM_REMOVEPAGE, 0, 0);
    RedrawWindow(hdlg,NULL,NULL,RDW_UPDATENOW|RDW_ERASENOW);
    DestroyWindow(hdlg);
}

static int CALLBACK disableowner_callback(HWND hwnd, UINT msg, LPARAM lparam)
{
    switch(msg)
    {
    case PSCB_INITIALIZED:
      {
        ok(IsWindowEnabled(parent) == 0, "parent window should be disabled\n");
        PostQuitMessage(0);
        return FALSE;
      }
    }
    return FALSE;
}

static void register_parent_wnd_class(void)
{
    WNDCLASSA cls;

    cls.style = 0;
    cls.lpfnWndProc = DefWindowProcA;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(NULL);
    cls.hIcon = 0;
    cls.hCursor = LoadCursorA(0, IDC_ARROW);
    cls.hbrBackground = GetStockObject(WHITE_BRUSH);
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "parent class";
    RegisterClassA(&cls);
}

static void test_disableowner(void)
{
    HPROPSHEETPAGE hpsp[1];
    PROPSHEETPAGEA psp;
    PROPSHEETHEADERA psh;
    INT_PTR p;

    register_parent_wnd_class();
    parent = CreateWindowA("parent class", "", WS_CAPTION | WS_SYSMENU | WS_VISIBLE, 100, 100, 100, 100, GetDesktopWindow(), NULL, GetModuleHandleA(NULL), 0);

    memset(&psp, 0, sizeof(psp));
    psp.dwSize = sizeof(psp);
    psp.dwFlags = 0;
    psp.hInstance = GetModuleHandleA(NULL);
    U(psp).pszTemplate = "prop_page1";
    U2(psp).pszIcon = NULL;
    psp.pfnDlgProc = NULL;
    psp.lParam = 0;

    hpsp[0] = CreatePropertySheetPageA(&psp);

    memset(&psh, 0, sizeof(psh));
    psh.dwSize = sizeof(psh);
    psh.dwFlags = PSH_USECALLBACK;
    psh.pszCaption = "test caption";
    psh.nPages = 1;
    psh.hwndParent = parent;
    U3(psh).phpage = hpsp;
    psh.pfnCallback = disableowner_callback;

    p = PropertySheetA(&psh);
    todo_wine
    ok(p == 0, "Expected 0, got %ld\n", p);
    ok(IsWindowEnabled(parent) != 0, "parent window should be enabled\n");
    DestroyWindow(parent);
}

static INT_PTR CALLBACK nav_page_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch(msg){
    case WM_NOTIFY:
        {
            LPNMHDR hdr = (LPNMHDR)lparam;
            switch(hdr->code){
            case PSN_SETACTIVE:
                active_page = PropSheet_HwndToIndex(hdr->hwndFrom, hwnd);
                return TRUE;
            case PSN_KILLACTIVE:
                /* prevent navigation away from the fourth page */
                if(active_page == 3){
                    SetWindowLongPtr(hwnd, DWLP_MSGRESULT, TRUE);
                    return TRUE;
                }
            }
            break;
        }
    }
    return FALSE;
}

static void test_wiznavigation(void)
{
    HPROPSHEETPAGE hpsp[4];
    PROPSHEETPAGEA psp[4];
    PROPSHEETHEADERA psh;
    HWND hdlg, control;
    LONG_PTR controlID;
    LRESULT defidres;
    BOOL hwndtoindex_supported = TRUE;
    const INT nextID = 12324;
    const INT backID = 12323;

    /* create the property sheet pages */
    memset(psp, 0, sizeof(PROPSHEETPAGEA) * 4);

    psp[0].dwSize = sizeof(PROPSHEETPAGEA);
    psp[0].hInstance = GetModuleHandleA(NULL);
    U(psp[0]).pszTemplate = MAKEINTRESOURCE(IDD_PROP_PAGE_INTRO);
    psp[0].pfnDlgProc = nav_page_proc;
    hpsp[0] = CreatePropertySheetPageA(&psp[0]);

    psp[1].dwSize = sizeof(PROPSHEETPAGEA);
    psp[1].hInstance = GetModuleHandleA(NULL);
    U(psp[1]).pszTemplate = MAKEINTRESOURCE(IDD_PROP_PAGE_EDIT);
    psp[1].pfnDlgProc = nav_page_proc;
    hpsp[1] = CreatePropertySheetPageA(&psp[1]);

    psp[2].dwSize = sizeof(PROPSHEETPAGEA);
    psp[2].hInstance = GetModuleHandleA(NULL);
    U(psp[2]).pszTemplate = MAKEINTRESOURCE(IDD_PROP_PAGE_RADIO);
    psp[2].pfnDlgProc = nav_page_proc;
    hpsp[2] = CreatePropertySheetPageA(&psp[2]);

    psp[3].dwSize = sizeof(PROPSHEETPAGEA);
    psp[3].hInstance = GetModuleHandleA(NULL);
    U(psp[3]).pszTemplate = MAKEINTRESOURCE(IDD_PROP_PAGE_EXIT);
    psp[3].pfnDlgProc = nav_page_proc;
    hpsp[3] = CreatePropertySheetPageA(&psp[3]);

    /* set up the property sheet dialog */
    memset(&psh, 0, sizeof(psh));
    psh.dwSize = sizeof(psh);
    psh.dwFlags = PSH_MODELESS | PSH_WIZARD;
    psh.pszCaption = "A Wizard";
    psh.nPages = 4;
    psh.hwndParent = GetDesktopWindow();
    U3(psh).phpage = hpsp;
    hdlg = (HWND)PropertySheetA(&psh);
    if (hdlg == INVALID_HANDLE_VALUE)
    {
        win_skip("comctl32 4.70 needs dwSize adjustment\n");
        psh.dwSize = sizeof(psh) - sizeof(HBITMAP) - sizeof(HPALETTE) - sizeof(HBITMAP);
        hdlg = (HWND)PropertySheetA(&psh);
    }

    ok(active_page == 0, "Active page should be 0. Is: %d\n", active_page);

    control = GetFocus();
    controlID = GetWindowLongPtr(control, GWLP_ID);
    ok(controlID == nextID, "Focus should have been set to the Next button. Expected: %d, Found: %ld\n", nextID, controlID);

    /* simulate pressing the Next button */
    SendMessage(hdlg, PSM_PRESSBUTTON, PSBTN_NEXT, 0);
    if (!active_page) hwndtoindex_supported = FALSE;
    if (hwndtoindex_supported)
        ok(active_page == 1, "Active page should be 1 after pressing Next. Is: %d\n", active_page);

    control = GetFocus();
    controlID = GetWindowLongPtr(control, GWLP_ID);
    ok(controlID == IDC_PS_EDIT1, "Focus should be set to the first item on the second page. Expected: %d, Found: %ld\n", IDC_PS_EDIT1, controlID);

    defidres = SendMessage(hdlg, DM_GETDEFID, 0, 0);
    ok(defidres == MAKELRESULT(nextID, DC_HASDEFID), "Expected default button ID to be %d, is %d\n", nextID, LOWORD(defidres));

    /* set the focus to the second edit box on this page */
    SetFocus(GetNextDlgTabItem(hdlg, control, FALSE));

    /* press next again */
    SendMessage(hdlg, PSM_PRESSBUTTON, PSBTN_NEXT, 0);
    if (hwndtoindex_supported)
        ok(active_page == 2, "Active page should be 2 after pressing Next. Is: %d\n", active_page);

    control = GetFocus();
    controlID = GetWindowLongPtr(control, GWLP_ID);
    ok(controlID == IDC_PS_RADIO1, "Focus should have been set to item on third page. Expected: %d, Found %ld\n", IDC_PS_RADIO1, controlID);

    /* back button */
    SendMessage(hdlg, PSM_PRESSBUTTON, PSBTN_BACK, 0);
    if (hwndtoindex_supported)
        ok(active_page == 1, "Active page should be 1 after pressing Back. Is: %d\n", active_page);

    control = GetFocus();
    controlID = GetWindowLongPtr(control, GWLP_ID);
    ok(controlID == IDC_PS_EDIT1, "Focus should have been set to the first item on second page. Expected: %d, Found %ld\n", IDC_PS_EDIT1, controlID);

    defidres = SendMessage(hdlg, DM_GETDEFID, 0, 0);
    ok(defidres == MAKELRESULT(backID, DC_HASDEFID), "Expected default button ID to be %d, is %d\n", backID, LOWORD(defidres));

    /* press next twice */
    SendMessage(hdlg, PSM_PRESSBUTTON, PSBTN_NEXT, 0);
    if (hwndtoindex_supported)
        ok(active_page == 2, "Active page should be 2 after pressing Next. Is: %d\n", active_page);
    SendMessage(hdlg, PSM_PRESSBUTTON, PSBTN_NEXT, 0);
    if (hwndtoindex_supported)
        ok(active_page == 3, "Active page should be 3 after pressing Next. Is: %d\n", active_page);
    else
        active_page = 3;

    control = GetFocus();
    controlID = GetWindowLongPtr(control, GWLP_ID);
    ok(controlID == nextID, "Focus should have been set to the Next button. Expected: %d, Found: %ld\n", nextID, controlID);

    /* try to navigate away, but shouldn't be able to */
    SendMessage(hdlg, PSM_PRESSBUTTON, PSBTN_BACK, 0);
    ok(active_page == 3, "Active page should still be 3 after pressing Back. Is: %d\n", active_page);

    defidres = SendMessage(hdlg, DM_GETDEFID, 0, 0);
    ok(defidres == MAKELRESULT(nextID, DC_HASDEFID), "Expected default button ID to be %d, is %d\n", nextID, LOWORD(defidres));

    DestroyWindow(hdlg);
}
static void test_buttons(void)
{
    HPROPSHEETPAGE hpsp[1];
    PROPSHEETPAGEA psp;
    PROPSHEETHEADERA psh;
    HWND hdlg;
    HWND button;
    RECT rc;
    int prevRight, top;

    memset(&psp, 0, sizeof(psp));
    psp.dwSize = sizeof(psp);
    psp.dwFlags = 0;
    psp.hInstance = GetModuleHandleA(NULL);
    U(psp).pszTemplate = "prop_page1";
    U2(psp).pszIcon = NULL;
    psp.pfnDlgProc = page_dlg_proc;
    psp.lParam = 0;

    hpsp[0] = CreatePropertySheetPageA(&psp);

    memset(&psh, 0, sizeof(psh));
    psh.dwSize = sizeof(psh);
    psh.dwFlags = PSH_MODELESS | PSH_USECALLBACK;
    psh.pszCaption = "test caption";
    psh.nPages = 1;
    psh.hwndParent = GetDesktopWindow();
    U3(psh).phpage = hpsp;
    psh.pfnCallback = sheet_callback;

    hdlg = (HWND)PropertySheetA(&psh);
    if (hdlg == INVALID_HANDLE_VALUE)
    {
        win_skip("comctl32 4.70 needs dwSize adjustment\n");
        psh.dwSize = sizeof(psh) - sizeof(HBITMAP) - sizeof(HPALETTE) - sizeof(HBITMAP);
        hdlg = (HWND)PropertySheetA(&psh);
    }

    /* OK button */
    button = GetDlgItem(hdlg, IDOK);
    GetWindowRect(button, &rc);
    prevRight = rc.right;
    top = rc.top;

    /* Cancel button */
    button = GetDlgItem(hdlg, IDCANCEL);
    GetWindowRect(button, &rc);
    ok(rc.top == top, "Cancel button should have same top as OK button\n");
    ok(rc.left > prevRight, "Cancel button should be to the right of OK button\n");
    prevRight = rc.right;

    button = GetDlgItem(hdlg, IDC_APPLY_BUTTON);
    GetWindowRect(button, &rc);
    ok(rc.top == top, "Apply button should have same top as OK button\n");
    ok(rc.left > prevRight, "Apply button should be to the right of Cancel button\n");
    prevRight = rc.right;

    button = GetDlgItem(hdlg, IDHELP);
    GetWindowRect(button, &rc);
    ok(rc.top == top, "Help button should have same top as OK button\n");
    ok(rc.left > prevRight, "Help button should be to the right of Apply button\n");

    DestroyWindow(hdlg);
}

START_TEST(propsheet)
{
    test_title();
    test_nopage();
    test_disableowner();
    test_wiznavigation();
    test_buttons();
}
