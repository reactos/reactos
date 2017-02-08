/*
 * Internet control panel applet: general propsheet
 *
 * Copyright 2010 Detlef Riekenberg
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
 *
 */

#include "inetcpl.h"

#include <wininet.h>
#include <shlobj.h>

static const WCHAR about_blank[] = {'a','b','o','u','t',':','b','l','a','n','k',0};
#ifdef __REACTOS__
static const WCHAR default_home[] = {'h','t','t','p',':','/','/','w','w','w','.','r','e','a','c','t','o','s','.','o','r','g',0};
#else
static const WCHAR default_home[] = {'h','t','t','p',':','/','/','w','w','w','.','w','i','n','e','h','q','.','o','r','g',0};
#endif
static const WCHAR start_page[] = {'S','t','a','r','t',' ','P','a','g','e',0};
static const WCHAR reg_ie_main[] = {'S','o','f','t','w','a','r','e','\\',
                                    'M','i','c','r','o','s','o','f','t','\\',
                                    'I','n','t','e','r','n','e','t',' ','E','x','p','l','o','r','e','r','\\',
                                    'M','a','i','n',0};

/* list of unimplemented buttons */
static DWORD disabled_general_buttons[] = {IDC_HOME_CURRENT,
                                           IDC_HISTORY_SETTINGS,
                                           0};
static DWORD disabled_delhist_buttons[] = {IDC_DELETE_FORM_DATA,
                                           IDC_DELETE_PASSWORDS,
                                           0};

/*********************************************************************
 * delhist_on_command [internal]
 *
 * handle WM_COMMAND in Delete browsing history dialog
 *
 */
static INT_PTR delhist_on_command(HWND hdlg, WPARAM wparam)
{
    switch (wparam)
    {
        case MAKEWPARAM(IDOK, BN_CLICKED):
            if (IsDlgButtonChecked(hdlg, IDC_DELETE_TEMP_FILES))
                FreeUrlCacheSpaceW(NULL, 100, 0);

            if (IsDlgButtonChecked(hdlg, IDC_DELETE_COOKIES))
            {
                WCHAR pathW[MAX_PATH];

                if(SHGetSpecialFolderPathW(NULL, pathW, CSIDL_COOKIES, TRUE))
                    FreeUrlCacheSpaceW(pathW, 100, 0);
            }

            if (IsDlgButtonChecked(hdlg, IDC_DELETE_HISTORY))
            {
                WCHAR pathW[MAX_PATH];

                if(SHGetSpecialFolderPathW(NULL, pathW, CSIDL_HISTORY, TRUE))
                    FreeUrlCacheSpaceW(pathW, 100, 0);
            }

            EndDialog(hdlg, IDOK);
            return TRUE;

        case MAKEWPARAM(IDCANCEL, BN_CLICKED):
            EndDialog(hdlg, IDCANCEL);
            return TRUE;

        case MAKEWPARAM(IDC_DELETE_TEMP_FILES, BN_CLICKED):
        case MAKEWPARAM(IDC_DELETE_COOKIES, BN_CLICKED):
        case MAKEWPARAM(IDC_DELETE_HISTORY, BN_CLICKED):
        case MAKEWPARAM(IDC_DELETE_FORM_DATA, BN_CLICKED):
        case MAKEWPARAM(IDC_DELETE_PASSWORDS, BN_CLICKED):
        {
            BOOL any = IsDlgButtonChecked(hdlg, IDC_DELETE_TEMP_FILES) ||
                       IsDlgButtonChecked(hdlg, IDC_DELETE_COOKIES) ||
                       IsDlgButtonChecked(hdlg, IDC_DELETE_HISTORY) ||
                       IsDlgButtonChecked(hdlg, IDC_DELETE_FORM_DATA) ||
                       IsDlgButtonChecked(hdlg, IDC_DELETE_PASSWORDS);
            EnableWindow(GetDlgItem(hdlg, IDOK), any);
            break;
        }

        default:
            break;
    }
    return FALSE;
}


/*********************************************************************
 * delhist_dlgproc [internal]
 *
 * Delete browsing history dialog procedure
 *
 */
static INT_PTR CALLBACK delhist_dlgproc(HWND hdlg, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
        case WM_COMMAND:
            return delhist_on_command(hdlg, wparam);

        case WM_INITDIALOG:
        {
            DWORD *ptr = disabled_delhist_buttons;
            while (*ptr)
            {
                EnableWindow(GetDlgItem(hdlg, *ptr), FALSE);
                ptr++;
            }
            CheckDlgButton(hdlg, IDC_DELETE_TEMP_FILES, BST_CHECKED);
            break;
        }

        default:
            break;
    }
    return FALSE;
}

/*********************************************************************
 * parse_url_from_outside [internal]
 *
 * Filter an URL, add a usable scheme, when needed
 *
 */
static DWORD parse_url_from_outside(LPCWSTR url, LPWSTR out, DWORD maxlen)
{
    HMODULE hdll;
    DWORD (WINAPI *pParseURLFromOutsideSourceW)(LPCWSTR, LPWSTR, LPDWORD, LPDWORD);
    DWORD res;

    hdll = LoadLibraryA("shdocvw.dll");
    pParseURLFromOutsideSourceW = (void *) GetProcAddress(hdll, (LPSTR) 170);

    if (pParseURLFromOutsideSourceW)
    {
        res = pParseURLFromOutsideSourceW(url, out, &maxlen, NULL);
        FreeLibrary(hdll);
        return res;
    }

    ERR("failed to get ordinal 170: %d\n", GetLastError());
    FreeLibrary(hdll);
    return 0;
}

/*********************************************************************
 * general_on_command [internal]
 *
 * handle WM_COMMAND
 *
 */
static INT_PTR general_on_command(HWND hwnd, WPARAM wparam)
{

    switch (wparam)
    {
        case MAKEWPARAM(IDC_HOME_EDIT, EN_CHANGE):
            /* enable apply button */
            SendMessageW(GetParent(hwnd), PSM_CHANGED, (WPARAM)hwnd, 0);
            break;

        case MAKEWPARAM(IDC_HOME_BLANK, BN_CLICKED):
            SetDlgItemTextW(hwnd, IDC_HOME_EDIT, about_blank);
            break;

        case MAKEWPARAM(IDC_HOME_DEFAULT, BN_CLICKED):
            SetDlgItemTextW(hwnd, IDC_HOME_EDIT, default_home);
            break;

        case MAKEWPARAM(IDC_HISTORY_DELETE, BN_CLICKED):
            DialogBoxW(hcpl, MAKEINTRESOURCEW(IDD_DELETE_HISTORY), hwnd,
                       delhist_dlgproc);
            break;

        default:
            TRACE("not implemented for command: %d/%d\n", HIWORD(wparam),  LOWORD(wparam));
            return FALSE;
    }
    return TRUE;
}

/*********************************************************************
 * general_on_initdialog [internal]
 *
 * handle WM_INITDIALOG
 *
 */
static VOID general_on_initdialog(HWND hwnd)
{
    WCHAR buffer[INTERNET_MAX_URL_LENGTH];
    DWORD len;
    DWORD type;
    LONG res;
    DWORD *ptr = disabled_general_buttons;

    /* disable unimplemented buttons */
    while (*ptr)
    {
        EnableWindow(GetDlgItem(hwnd, *ptr), FALSE);
        ptr++;
    }

    /* read current homepage from the registry. Try HCU first, then HKLM */
    *buffer = 0;
    len = sizeof(buffer);
    type = REG_SZ;
    res = SHRegGetUSValueW(reg_ie_main, start_page, &type, buffer, &len, FALSE, (LPBYTE) about_blank, sizeof(about_blank));

    if (!res && (type == REG_SZ))
    {
        SetDlgItemTextW(hwnd, IDC_HOME_EDIT, buffer);
    }
}

/*********************************************************************
 * general_on_notify [internal]
 *
 * handle WM_NOTIFY
 *
 */
static INT_PTR general_on_notify(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
    PSHNOTIFY *psn;
    WCHAR buffer[INTERNET_MAX_URL_LENGTH];
    WCHAR parsed[INTERNET_MAX_URL_LENGTH];
    LONG res;

    psn = (PSHNOTIFY *) lparam;
    TRACE("WM_NOTIFY (%p, 0x%lx, 0x%lx) from %p with code: %d\n", hwnd, wparam, lparam,
            psn->hdr.hwndFrom, psn->hdr.code);

    if (psn->hdr.code == PSN_APPLY)
    {
        *buffer = 0;
        GetDlgItemTextW(hwnd, IDC_HOME_EDIT, buffer, sizeof(buffer)/sizeof(WCHAR));
        TRACE("EDITTEXT has %s\n", debugstr_w(buffer));

        res = parse_url_from_outside(buffer, parsed, sizeof(parsed)/sizeof(WCHAR));
        TRACE("got %d with %s\n", res, debugstr_w(parsed));

        if (res)
        {
            HKEY hkey;

            /* update the dialog, when needed */
            if (lstrcmpW(buffer, parsed))
                SetDlgItemTextW(hwnd, IDC_HOME_EDIT, parsed);

            /* update the registry */
            res = RegOpenKeyW(HKEY_CURRENT_USER, reg_ie_main, &hkey);
            if (!res)
            {
                res = RegSetValueExW(hkey, start_page, 0, REG_SZ, (const BYTE *)parsed,
                                    (lstrlenW(parsed) + 1) * sizeof(WCHAR));
                RegCloseKey(hkey);
                return !res;
            }
        }
    }
    return FALSE;
}

/*********************************************************************
 * general_dlgproc [internal]
 *
 */
INT_PTR CALLBACK general_dlgproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{

    switch (msg)
    {
        case WM_INITDIALOG:
            general_on_initdialog(hwnd);
            return TRUE;

        case WM_COMMAND:
            return general_on_command(hwnd, wparam);

        case WM_NOTIFY:
            return general_on_notify(hwnd, wparam, lparam);

        default:
            /* do not flood the log */
            if ((msg == WM_SETCURSOR) || (msg == WM_NCHITTEST) || (msg == WM_MOUSEMOVE))
                return FALSE;

            TRACE("(%p, 0x%08x/%d, 0x%lx, 0x%lx)\n", hwnd, msg, msg, wparam, lparam);

    }
    return FALSE;
}
