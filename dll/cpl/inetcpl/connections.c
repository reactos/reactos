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
#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(inetcpl);

static const WCHAR auto_config_url[] = {'A','u','t','o','C','o','n','f','i','g','U','R','L',0};
static const WCHAR internet_settings[] = {'S','o','f','t','w','a','r','e','\\',
    'M','i','c','r','o','s','o','f','t','\\','W','i','n','d','o','w','s','\\',
    'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
    'I','n','t','e','r','n','e','t',' ','S','e','t','t','i','n','g','s',0};
static const WCHAR proxy_enable[] = {'P','r','o','x','y','E','n','a','b','l','e',0};
static const WCHAR proxy_server[] = {'P','r','o','x','y','S','e','r','v','e','r',0};
static const WCHAR connections[] = {'C','o','n','n','e','c','t','i','o','n','s',0};
static const WCHAR default_connection_settings[] = {'D','e','f','a','u','l','t',
    'C','o','n','n','e','c','t','i','o','n','S','e','t','t','i','n','g','s',0};

static BOOL initdialog_done;

#define CONNECTION_SETTINGS_VERSION 0x46
#define CONNECTION_SETTINGS_MANUAL_PROXY 0x2
#define CONNECTION_SETTINGS_PAC_SCRIPT 0x4
#define CONNECTION_SETTINGS_WPAD 0x8

typedef struct {
    DWORD version;
    DWORD id;
    DWORD flags;
    BYTE data[1];
    /* DWORD proxy_server_len; */
    /* UTF8 proxy_server[proxy_server_len]; */
    /* DWORD bypass_list_len; */
    /* UTF8 bypass_list[bypass_list_len]; */
    /* DWORD configuration_script_len; */
    /* UTF8 configuration_script[configuration_script_len]; */
    /* DWORD unk[8]; set to 0 */
} connection_settings;

static DWORD create_connection_settings(BOOL manual_proxy, const WCHAR *proxy_server,
        BOOL use_wpad, BOOL use_pac_script, const WCHAR *pac_url, connection_settings **ret)
{
    DWORD size = FIELD_OFFSET(connection_settings, data), pos;
    DWORD proxy_server_len;
    DWORD pac_url_len;

    size += sizeof(DWORD);
    if(proxy_server)
    {
        proxy_server_len = WideCharToMultiByte(CP_UTF8, 0, proxy_server, -1,
                NULL, 0, NULL, NULL);
        if(!proxy_server_len) return 0;
        proxy_server_len--;
    }
    else
        proxy_server_len = 0;
    size += proxy_server_len;
    if(pac_url)
    {
        pac_url_len = WideCharToMultiByte(CP_UTF8, 0, pac_url, -1,
                NULL, 0, NULL, NULL);
        if(!pac_url_len) return 0;
        pac_url_len--;
    }
    else
        pac_url_len = 0;
    size += sizeof(DWORD)*10;

    *ret = heap_alloc_zero(size);
    if(!*ret) return 0;

    (*ret)->version = CONNECTION_SETTINGS_VERSION;
    (*ret)->flags = 1;
    if(manual_proxy) (*ret)->flags |= CONNECTION_SETTINGS_MANUAL_PROXY;
    if(use_pac_script) (*ret)->flags |= CONNECTION_SETTINGS_PAC_SCRIPT;
    if(use_wpad) (*ret)->flags |= CONNECTION_SETTINGS_WPAD;
    ((DWORD*)(*ret)->data)[0] = proxy_server_len;
    pos = sizeof(DWORD);
    if(proxy_server_len)
    {
        WideCharToMultiByte(CP_UTF8, 0, proxy_server, -1,
                (char*)(*ret)->data+pos, proxy_server_len, NULL, NULL);
        pos += proxy_server_len;
    }
    pos += sizeof(DWORD); /* skip proxy bypass list */
    ((DWORD*)((*ret)->data+pos))[0] = pac_url_len;
    pos += sizeof(DWORD);
    if(pac_url_len)
    {
        WideCharToMultiByte(CP_UTF8, 0, pac_url, -1,
                (char*)(*ret)->data+pos, pac_url_len, NULL, NULL);
        pos += pac_url_len;
    }
    return size;
}

static void connections_on_initdialog(HWND hwnd)
{
    DWORD type, size, enabled;
    WCHAR address[INTERNET_MAX_URL_LENGTH], *port;
    WCHAR pac_url[INTERNET_MAX_URL_LENGTH];
    HKEY hkey, con;
    LONG res;

    SendMessageW(GetDlgItem(hwnd, IDC_EDIT_PAC_SCRIPT),
            EM_LIMITTEXT, INTERNET_MAX_URL_LENGTH, 0);
    SendMessageW(GetDlgItem(hwnd, IDC_EDIT_PROXY_SERVER),
            EM_LIMITTEXT, INTERNET_MAX_URL_LENGTH-10, 0);
    SendMessageW(GetDlgItem(hwnd, IDC_EDIT_PROXY_PORT), EM_LIMITTEXT, 8, 0);

    res = RegOpenKeyW(HKEY_CURRENT_USER, internet_settings, &hkey);
    if(res)
        return;

    size = sizeof(enabled);
    res = RegQueryValueExW(hkey, proxy_enable, NULL, &type, (BYTE*)&enabled, &size);
    if(res || type != REG_DWORD)
        enabled = 0;
    size = sizeof(address);
    res = RegQueryValueExW(hkey, proxy_server, NULL, &type, (BYTE*)address, &size);
    if(res || type != REG_SZ)
        address[0] = 0;
    size = sizeof(pac_url);
    res = RegQueryValueExW(hkey, auto_config_url, NULL, &type, (BYTE*)pac_url, &size);
    if(res || type != REG_SZ)
        pac_url[0] = 0;

    res = RegOpenKeyW(hkey, connections, &con);
    RegCloseKey(hkey);
    if(!res)
    {
        connection_settings *settings = NULL;
        size = 0;

        while((res = RegQueryValueExW(con, default_connection_settings, NULL, &type,
                        (BYTE*)settings, &size)) == ERROR_MORE_DATA || !settings)
        {
            connection_settings *new_settings = heap_realloc(settings, size);
            if(!new_settings)
            {
                RegCloseKey(con);
                heap_free(settings);
                return;
            }
            settings = new_settings;
        }
        RegCloseKey(con);

        if(!res && type == REG_BINARY)
        {
            if(settings->version != CONNECTION_SETTINGS_VERSION)
                FIXME("unexpected structure version (%x)\n", settings->version);
            else if(settings->flags & CONNECTION_SETTINGS_WPAD)
                CheckDlgButton(hwnd, IDC_USE_WPAD, BST_CHECKED);
        }
        heap_free(settings);
    }

    TRACE("ProxyEnable = %x\n", enabled);
    TRACE("ProxyServer = %s\n", wine_dbgstr_w(address));
    TRACE("AutoConfigURL = %s\n", wine_dbgstr_w(auto_config_url));

    if(enabled)
    {
        CheckDlgButton(hwnd, IDC_USE_PROXY_SERVER, BST_CHECKED);
        EnableWindow(GetDlgItem(hwnd, IDC_EDIT_PROXY_SERVER), TRUE);
        EnableWindow(GetDlgItem(hwnd, IDC_EDIT_PROXY_PORT), TRUE);
    }

    port = wcschr(address, ':');
    if(port)
    {
        *port = 0;
        port++;
    }
    SetDlgItemTextW(hwnd, IDC_EDIT_PROXY_SERVER, address);
    if(port)
        SetDlgItemTextW(hwnd, IDC_EDIT_PROXY_PORT, port);

    if(pac_url[0])
    {
        CheckDlgButton(hwnd, IDC_USE_PAC_SCRIPT, BST_CHECKED);
        EnableWindow(GetDlgItem(hwnd, IDC_EDIT_PAC_SCRIPT), TRUE);
        SetDlgItemTextW(hwnd, IDC_EDIT_PAC_SCRIPT, pac_url);
    }

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
    connection_settings *default_connection;
    WCHAR proxy[INTERNET_MAX_URL_LENGTH];
    WCHAR pac_script[INTERNET_MAX_URL_LENGTH];
    PSHNOTIFY *psn = (PSHNOTIFY*)lparam;
    DWORD proxy_len, port_len, pac_script_len;
    DWORD use_proxy, use_pac_script, use_wpad, size;
    LRESULT res;
    HKEY hkey, con;

    if(psn->hdr.code != PSN_APPLY)
        return FALSE;

    res = RegOpenKeyW(HKEY_CURRENT_USER, internet_settings, &hkey);
    if(res)
        return FALSE;

    use_proxy = IsDlgButtonChecked(hwnd, IDC_USE_PROXY_SERVER);
    res = RegSetValueExW(hkey, proxy_enable, 0, REG_DWORD,
            (BYTE*)&use_proxy, sizeof(use_proxy));
    if(res)
    {
        RegCloseKey(hkey);
        return FALSE;
    }
    TRACE("ProxyEnable set to %x\n", use_proxy);

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

        res = RegSetValueExW(hkey, proxy_server, 0, REG_SZ,
                (BYTE*)proxy, (proxy_len+port_len)*sizeof(WCHAR));
    }
    else
    {
        res = RegDeleteValueW(hkey, proxy_server);
        if(res == ERROR_FILE_NOT_FOUND)
            res = ERROR_SUCCESS;
    }
    if(res)
    {
        RegCloseKey(hkey);
        return FALSE;
    }
    TRACE("ProxtServer set to %s\n", wine_dbgstr_w(proxy));

    use_pac_script = IsDlgButtonChecked(hwnd, IDC_USE_PAC_SCRIPT);
    pac_script_len = GetDlgItemTextW(hwnd, IDC_EDIT_PAC_SCRIPT,
            pac_script, ARRAY_SIZE(pac_script));
    if(!pac_script_len) use_pac_script = FALSE;
    if(use_pac_script)
    {
        res = RegSetValueExW(hkey, auto_config_url, 0, REG_SZ,
                (BYTE*)pac_script, pac_script_len*sizeof(WCHAR));
    }
    else
    {
        res = RegDeleteValueW(hkey, auto_config_url);
        if(res == ERROR_FILE_NOT_FOUND)
            res = ERROR_SUCCESS;
    }
    if(res)
    {
        RegCloseKey(hkey);
        return FALSE;
    }
    TRACE("AutoConfigURL set to %s\n", wine_dbgstr_w(use_pac_script ? pac_script : NULL));

    use_wpad = IsDlgButtonChecked(hwnd, IDC_USE_WPAD);

    res = RegCreateKeyExW(hkey, connections, 0, NULL, 0, KEY_WRITE, NULL, &con, NULL);
    RegCloseKey(hkey);
    if(res)
        return FALSE;

    size = create_connection_settings(use_proxy, proxy, use_wpad,
        use_pac_script, pac_script, &default_connection);
    if(!size)
    {
        RegCloseKey(con);
        return FALSE;
    }

    res = RegSetValueExW(con, default_connection_settings, 0, REG_BINARY,
            (BYTE*)default_connection, size);
    heap_free(default_connection);
    RegCloseKey(con);
    return !res;
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
