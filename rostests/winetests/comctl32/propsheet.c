/* Unit test suite for property sheet control.
 *
 * Copyright 2006 Huw Davies
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

static HWND parent;

static int CALLBACK sheet_callback(HWND hwnd, UINT msg, LPARAM lparam)
{
    switch(msg)
    {
    case PSCB_INITIALIZED:
      {
        char caption[256];
        GetWindowTextA(hwnd, caption, sizeof(caption));
        ok(!strcmp(caption,"test caption"), "caption: %s\n", caption);
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
    psp.hInstance = GetModuleHandleW(NULL);
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
    psp.hInstance = GetModuleHandleW(NULL);
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

    register_parent_wnd_class();
    parent = CreateWindowA("parent class", "", WS_CAPTION | WS_SYSMENU | WS_VISIBLE, 100, 100, 100, 100, GetDesktopWindow(), NULL, GetModuleHandleA(NULL), 0);

    memset(&psp, 0, sizeof(psp));
    psp.dwSize = sizeof(psp);
    psp.dwFlags = 0;
    psp.hInstance = GetModuleHandleW(NULL);
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

    PropertySheetA(&psh);
    ok(IsWindowEnabled(parent) != 0, "parent window should be enabled\n");
    DestroyWindow(parent);
}

START_TEST(propsheet)
{
    test_title();
    test_nopage();
    test_disableowner();
}
