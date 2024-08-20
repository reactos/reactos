/*
 * Copyright 2018 Piotr Caban for CodeWeavers
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

#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include <wininet.h>
#include <winuser.h>
#include <winreg.h>

#include "inetcpl.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(inetcpl);

static BOOL initdialog_done;

static void connections_on_initdialog(HWND hwnd)
{
    INTERNET_PER_CONN_OPTION_LISTW list;
    INTERNET_PER_CONN_OPTIONW options[3];
    WCHAR *address, *port, *pac_url;
    DWORD size, flags;

    SendMessageW(GetDlgItem(hwnd, IDC_EDIT_PAC_SCRIPT),
            EM_LIMITTEXT, INTERNET_MAX_URL_LENGTH, 0);
    SendMessageW(GetDlgItem(hwnd, IDC_EDIT_PROXY_SERVER),
            EM_LIMITTEXT, INTERNET_MAX_URL_LENGTH-10, 0);
    SendMessageW(GetDlgItem(hwnd, IDC_EDIT_PROXY_PORT), EM_LIMITTEXT, 8, 0);

    list.dwSize = sizeof(list);
    list.pszConnection = NULL;
    list.dwOptionCount = ARRAY_SIZE(options);
    list.pOptions = options;

    options[0].dwOption = INTERNET_PER_CONN_FLAGS;
    options[1].dwOption = INTERNET_PER_CONN_PROXY_SERVER;
    options[2].dwOption = INTERNET_PER_CONN_AUTOCONFIG_URL;
    size = sizeof(list);
    if(!InternetQueryOptionW(NULL, INTERNET_OPTION_PER_CONNECTION_OPTION, &list, &size))
        return;

    flags = options[0].Value.dwValue;
    address = options[1].Value.pszValue;
    pac_url = options[2].Value.pszValue;

    TRACE("flags = %lx\n", flags);
    TRACE("ProxyServer = %s\n", wine_dbgstr_w(address));
    TRACE("AutoConfigURL = %s\n", wine_dbgstr_w(pac_url));

    if (flags & PROXY_TYPE_AUTO_DETECT)
        CheckDlgButton(hwnd, IDC_USE_WPAD, BST_CHECKED);

    if(flags & PROXY_TYPE_PROXY)
    {
        CheckDlgButton(hwnd, IDC_USE_PROXY_SERVER, BST_CHECKED);
        EnableWindow(GetDlgItem(hwnd, IDC_EDIT_PROXY_SERVER), TRUE);
        EnableWindow(GetDlgItem(hwnd, IDC_EDIT_PROXY_PORT), TRUE);
    }

    if(address)
    {
        port = wcschr(address, ':');
        if(port)
        {
            *port = 0;
            port++;
        }
        SetDlgItemTextW(hwnd, IDC_EDIT_PROXY_SERVER, address);
        if(port)
            SetDlgItemTextW(hwnd, IDC_EDIT_PROXY_PORT, port);
    }

    if(flags & PROXY_TYPE_AUTO_PROXY_URL)
    {
        CheckDlgButton(hwnd, IDC_USE_PAC_SCRIPT, BST_CHECKED);
        EnableWindow(GetDlgItem(hwnd, IDC_EDIT_PAC_SCRIPT), TRUE);
    }
    if(pac_url)
        SetDlgItemTextW(hwnd, IDC_EDIT_PAC_SCRIPT, pac_url);

    GlobalFree(address);
    GlobalFree(pac_url);
    return;
}

static INT_PTR connections_on_command(HWND hwnd, WPARAM wparam)
{
    BOOL checked;

    switch (wparam)
    {
        case IDC_USE_PAC_SCRIPT:
            checked = IsDlgButtonChecked(hwnd, IDC_USE_PAC_SCRIPT);
            EnableWindow(GetDlgItem(hwnd, IDC_EDIT_PAC_SCRIPT), checked);
            break;
        case IDC_USE_PROXY_SERVER:
            checked = IsDlgButtonChecked(hwnd, IDC_USE_PROXY_SERVER);
            EnableWindow(GetDlgItem(hwnd, IDC_EDIT_PROXY_SERVER), checked);
            EnableWindow(GetDlgItem(hwnd, IDC_EDIT_PROXY_PORT), checked);
    }

    switch (wparam)
    {
        case IDC_USE_WPAD:
        case IDC_USE_PAC_SCRIPT:
        case IDC_USE_PROXY_SERVER:
        case MAKEWPARAM(IDC_EDIT_PAC_SCRIPT, EN_CHANGE):
        case MAKEWPARAM(IDC_EDIT_PROXY_SERVER, EN_CHANGE):
        case MAKEWPARAM(IDC_EDIT_PROXY_PORT, EN_CHANGE):
            if(initdialog_done)
                SendMessageW(GetParent(hwnd), PSM_CHANGED, (WPARAM)hwnd, 0);
            return TRUE;
    }

    return FALSE;
}

static INT_PTR connections_on_notify(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
    WCHAR proxy[INTERNET_MAX_URL_LENGTH];
    WCHAR pac_script[INTERNET_MAX_URL_LENGTH];
    PSHNOTIFY *psn = (PSHNOTIFY*)lparam;
    DWORD proxy_len, port_len, pac_script_len;
    INTERNET_PER_CONN_OPTION_LISTW list;
    INTERNET_PER_CONN_OPTIONW options[3];
    DWORD flags;

    if(psn->hdr.code != PSN_APPLY)
        return FALSE;

    flags = IsDlgButtonChecked(hwnd, IDC_USE_PROXY_SERVER) ? PROXY_TYPE_PROXY : PROXY_TYPE_DIRECT;

    proxy_len = GetDlgItemTextW(hwnd, IDC_EDIT_PROXY_SERVER, proxy, ARRAY_SIZE(proxy));
    if(proxy_len)
    {
        proxy[proxy_len++] = ':';
        port_len = GetDlgItemTextW(hwnd, IDC_EDIT_PROXY_PORT, proxy+proxy_len,
                ARRAY_SIZE(proxy)-proxy_len);
        if(!port_len)
        {
            proxy[proxy_len++] = '8';
            proxy[proxy_len++] = '0';
            proxy[proxy_len] = 0;
        }
    }
    else
    {
        flags = PROXY_TYPE_DIRECT;
    }

    pac_script_len = GetDlgItemTextW(hwnd, IDC_EDIT_PAC_SCRIPT,
            pac_script, ARRAY_SIZE(pac_script));
    if(pac_script_len && IsDlgButtonChecked(hwnd, IDC_USE_PAC_SCRIPT))
        flags |= PROXY_TYPE_AUTO_PROXY_URL;

    if(IsDlgButtonChecked(hwnd, IDC_USE_WPAD))
        flags |= PROXY_TYPE_AUTO_DETECT;

    TRACE("flags = %lx\n", flags);
    TRACE("ProxyServer = %s\n", wine_dbgstr_w(proxy));
    TRACE("AutoConfigURL = %s\n", wine_dbgstr_w(pac_script));

    list.dwSize = sizeof(list);
    list.pszConnection = NULL;
    list.dwOptionCount = ARRAY_SIZE(options);
    list.pOptions = options;

    options[0].dwOption = INTERNET_PER_CONN_FLAGS;
    options[0].Value.dwValue = flags;
    options[1].dwOption = INTERNET_PER_CONN_PROXY_SERVER;
    options[1].Value.pszValue = proxy;
    options[2].dwOption = INTERNET_PER_CONN_AUTOCONFIG_URL;
    options[2].Value.pszValue = pac_script;
    return InternetSetOptionW(NULL, INTERNET_OPTION_PER_CONNECTION_OPTION, &list, sizeof(list));
}

INT_PTR CALLBACK connections_dlgproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
        case WM_INITDIALOG:
            connections_on_initdialog(hwnd);
            initdialog_done = TRUE;
            break;
        case WM_COMMAND:
            return connections_on_command(hwnd, wparam);
        case WM_NOTIFY:
            return connections_on_notify(hwnd, wparam, lparam);
    }
    return FALSE;
}
