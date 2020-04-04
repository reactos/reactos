/*
 * SHAppBarMessage implementation
 *
 * Copyright 2008 Vincent Povirk for CodeWeavers
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

//
// Adapted from Wine appbar.c .
//

#include <wine/config.h>

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include <windef.h>
#include <winbase.h>
#include <winerror.h>
#include <shellapi.h>
#include <winuser.h>
#include <shlobj.h>
#include <shlwapi.h>

#include "undocshell.h"

#include <wine/debug.h>
#include <wine/unicode.h>

WINE_DEFAULT_DEBUG_CHANNEL(appbar);

struct appbar_cmd
{
    DWORD  dwMsg;
    ULONG  return_map;
    DWORD  return_process;
    struct _AppBarData abd;
};

struct appbar_response
{
    ULONGLONG result;
    struct _AppBarData abd;
};

/*************************************************************************
 * SHAppBarMessage            [SHELL32.@]
 */
UINT_PTR WINAPI SHAppBarMessage(DWORD msg, PAPPBARDATA data)
{
    struct appbar_cmd command;
    struct appbar_response* response;
    HANDLE return_map;
    LPVOID return_view;
    HWND appbarmsg_window;
    COPYDATASTRUCT cds;

    UINT_PTR ret = 0;

    TRACE("msg=%d, data={cb=%d, hwnd=%p}\n", msg, data->cbSize, data->hWnd);

    /* These members are message dependent */
    switch(msg)
    {
    case ABM_NEW:
        TRACE("callback: %x\n", data->uCallbackMessage);
        break;

    case ABM_GETAUTOHIDEBAR:
        TRACE("edge: %d\n", data->uEdge);
        break;

    case ABM_QUERYPOS:
    case ABM_SETPOS:
        TRACE("edge: %d, rc: %s\n", data->uEdge, wine_dbgstr_rect(&data->rc));
        break;

    case ABM_GETTASKBARPOS:
        TRACE("rc: %s\n", wine_dbgstr_rect(&data->rc));
        break;

    case ABM_SETAUTOHIDEBAR:
        TRACE("edge: %d, lParam: %lx\n", data->uEdge, data->lParam);
        break;

    default:
        FIXME("unknown msg: %d\n", msg);
        break;
    }

    if (data->cbSize < sizeof(APPBARDATA))
    {
        WARN("data at %p is too small\n", data);
        return FALSE;
    }

    command.dwMsg = msg;
    command.abd.hWnd = data->hWnd;
    command.abd.uCallbackMessage = data->uCallbackMessage;
    command.abd.uEdge = data->uEdge;
    command.abd.rc = data->rc;
    command.abd.lParam = data->lParam;

    return_map = CreateFileMappingW(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE, 0, sizeof(struct appbar_response), NULL);
    if (return_map == NULL)
    {
        ERR("couldn't create file mapping\n");
        return 0;
    }
    command.return_map = HandleToUlong( return_map );

    command.return_process = GetCurrentProcessId();

    appbarmsg_window = FindWindowW(L"Shell_TrayWnd", NULL);
    if (appbarmsg_window == NULL)
    {
        ERR("couldn't find appbar window\n");
        CloseHandle(return_map);
        return 0;
    }

    cds.dwData = TABDMC_APPBAR;
    cds.cbData = sizeof(command);
    cds.lpData = &command;

    SendMessageW(appbarmsg_window, WM_COPYDATA, (WPARAM)data->hWnd, (LPARAM)&cds);

    return_view = MapViewOfFile(return_map, FILE_MAP_READ, 0, 0, sizeof(struct appbar_response));
    if (return_view == NULL)
    {
        ERR("MapViewOfFile failed\n");
        CloseHandle(return_map);
        return 0;
    }

    response = return_view;

    ret = response->result;
    if (ret)
    {
        data->hWnd = response->abd.hWnd;
        data->uCallbackMessage = response->abd.uCallbackMessage;
        data->uEdge = response->abd.uEdge;
        data->rc = response->abd.rc;
        data->lParam = response->abd.lParam;
    }
    UnmapViewOfFile(return_view);

    CloseHandle(return_map);

    return ret;
}
