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

#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include <windows.h>
#include <commctrl.h>

#include "wine/test.h"

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
    psp.u.pszTemplate = "prop_page1";
    psp.u2.pszIcon = NULL;
    psp.pfnDlgProc = page_dlg_proc;
    psp.lParam = 0;

    hpsp[0] = CreatePropertySheetPageA(&psp);

    memset(&psh, 0, sizeof(psh));
    psh.dwSize = sizeof(psh);
    psh.dwFlags = PSH_MODELESS | PSH_USECALLBACK;
    psh.pszCaption = "test caption";
    psh.nPages = 1;
    psh.hwndParent = GetDesktopWindow();
    psh.u3.phpage = hpsp;
    psh.pfnCallback = sheet_callback;

    hdlg = (HWND)PropertySheetA(&psh);
    DestroyWindow(hdlg);
}

START_TEST(propsheet)
{
    test_title();
}
