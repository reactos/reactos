/*
 * Window properties
 *
 * Copyright 1995, 1996, 2001 Alexandre Julliard
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

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wine/unicode.h"
#include "wine/winternl.h"
#include "user_private.h"
#include "wine/server.h"

/* size of buffer needed to store an atom string */
#define ATOM_BUFFER_SIZE 256


/***********************************************************************
 *              get_properties
 *
 * Retrieve the list of properties of a given window.
 * Returned buffer must be freed by caller.
 */
static property_data_t *get_properties( HWND hwnd, int *count )
{
    property_data_t *data;
    int total = 32;

    while (total)
    {
        int res = 0;
        if (!(data = HeapAlloc( GetProcessHeap(), 0, total * sizeof(*data) ))) break;
        *count = 0;
        SERVER_START_REQ( get_window_properties )
        {
            req->window = wine_server_user_handle( hwnd );
            wine_server_set_reply( req, data, total * sizeof(*data) );
            if (!wine_server_call( req )) res = reply->total;
        }
        SERVER_END_REQ;
        if (res && res <= total)
        {
            *count = res;
            return data;
        }
        HeapFree( GetProcessHeap(), 0, data );
        total = res;  /* restart with larger buffer */
    }
    return NULL;
}


/***********************************************************************
 *              EnumPropsA_relay
 *
 * relay to call the EnumProps callback function from EnumPropsEx
 */
static BOOL CALLBACK EnumPropsA_relay( HWND hwnd, LPSTR str, HANDLE handle, ULONG_PTR lparam )
{
    PROPENUMPROCA func = (PROPENUMPROCA)lparam;
    return func( hwnd, str, handle );
}


/***********************************************************************
 *              EnumPropsW_relay
 *
 * relay to call the EnumProps callback function from EnumPropsEx
 */
static BOOL CALLBACK EnumPropsW_relay( HWND hwnd, LPWSTR str, HANDLE handle, ULONG_PTR lparam )
{
    PROPENUMPROCW func = (PROPENUMPROCW)lparam;
    return func( hwnd, str, handle );
}


/***********************************************************************
 *              EnumPropsA   (USER32.@)
 */
INT WINAPI EnumPropsA( HWND hwnd, PROPENUMPROCA func )
{
    return EnumPropsExA( hwnd, EnumPropsA_relay, (LPARAM)func );
}


/***********************************************************************
 *              EnumPropsW   (USER32.@)
 */
INT WINAPI EnumPropsW( HWND hwnd, PROPENUMPROCW func )
{
    return EnumPropsExW( hwnd, EnumPropsW_relay, (LPARAM)func );
}


/***********************************************************************
 *              GetPropA   (USER32.@)
 */
HANDLE WINAPI GetPropA( HWND hwnd, LPCSTR str )
{
    WCHAR buffer[ATOM_BUFFER_SIZE];

    if (IS_INTRESOURCE(str)) return GetPropW( hwnd, (LPCWSTR)str );
    if (!MultiByteToWideChar( CP_ACP, 0, str, -1, buffer, ATOM_BUFFER_SIZE )) return 0;
    return GetPropW( hwnd, buffer );
}


/***********************************************************************
 *              GetPropW   (USER32.@)
 */
HANDLE WINAPI GetPropW( HWND hwnd, LPCWSTR str )
{
    ULONG_PTR ret = 0;

    SERVER_START_REQ( get_window_property )
    {
        req->window = wine_server_user_handle( hwnd );
        if (IS_INTRESOURCE(str)) req->atom = LOWORD(str);
        else wine_server_add_data( req, str, strlenW(str) * sizeof(WCHAR) );
        if (!wine_server_call_err( req )) ret = reply->data;
    }
    SERVER_END_REQ;
    return (HANDLE)ret;
}


/***********************************************************************
 *              SetPropA   (USER32.@)
 */
BOOL WINAPI SetPropA( HWND hwnd, LPCSTR str, HANDLE handle )
{
    WCHAR buffer[ATOM_BUFFER_SIZE];

    if (IS_INTRESOURCE(str)) return SetPropW( hwnd, (LPCWSTR)str, handle );
    if (!MultiByteToWideChar( CP_ACP, 0, str, -1, buffer, ATOM_BUFFER_SIZE )) return FALSE;
    return SetPropW( hwnd, buffer, handle );
}


/***********************************************************************
 *              SetPropW   (USER32.@)
 */
BOOL WINAPI SetPropW( HWND hwnd, LPCWSTR str, HANDLE handle )
{
    BOOL ret;

    SERVER_START_REQ( set_window_property )
    {
        req->window = wine_server_user_handle( hwnd );
        req->data   = (ULONG_PTR)handle;
        if (IS_INTRESOURCE(str)) req->atom = LOWORD(str);
        else wine_server_add_data( req, str, strlenW(str) * sizeof(WCHAR) );
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *              RemovePropA   (USER32.@)
 */
HANDLE WINAPI RemovePropA( HWND hwnd, LPCSTR str )
{
    WCHAR buffer[ATOM_BUFFER_SIZE];

    if (IS_INTRESOURCE(str)) return RemovePropW( hwnd, (LPCWSTR)str );
    if (!MultiByteToWideChar( CP_ACP, 0, str, -1, buffer, ATOM_BUFFER_SIZE )) return 0;
    return RemovePropW( hwnd, buffer );
}


/***********************************************************************
 *              RemovePropW   (USER32.@)
 */
HANDLE WINAPI RemovePropW( HWND hwnd, LPCWSTR str )
{
    ULONG_PTR ret = 0;

    SERVER_START_REQ( remove_window_property )
    {
        req->window = wine_server_user_handle( hwnd );
        if (IS_INTRESOURCE(str)) req->atom = LOWORD(str);
        else wine_server_add_data( req, str, strlenW(str) * sizeof(WCHAR) );
        if (!wine_server_call_err( req )) ret = reply->data;
    }
    SERVER_END_REQ;

    return (HANDLE)ret;
}


/***********************************************************************
 *              EnumPropsExA   (USER32.@)
 */
INT WINAPI EnumPropsExA(HWND hwnd, PROPENUMPROCEXA func, LPARAM lParam)
{
    int ret = -1, i, count;
    property_data_t *list = get_properties( hwnd, &count );

    if (list)
    {
        for (i = 0; i < count; i++)
        {
            char string[ATOM_BUFFER_SIZE];
            if (!UserGetAtomNameA( list[i].atom, string, ATOM_BUFFER_SIZE )) continue;
            if (!(ret = func( hwnd, string, (HANDLE)(ULONG_PTR)list[i].data, lParam ))) break;
        }
        HeapFree( GetProcessHeap(), 0, list );
    }
    return ret;
}


/***********************************************************************
 *              EnumPropsExW   (USER32.@)
 */
INT WINAPI EnumPropsExW(HWND hwnd, PROPENUMPROCEXW func, LPARAM lParam)
{
    int ret = -1, i, count;
    property_data_t *list = get_properties( hwnd, &count );

    if (list)
    {
        for (i = 0; i < count; i++)
        {
            WCHAR string[ATOM_BUFFER_SIZE];
            if (!UserGetAtomNameW( list[i].atom, string, ATOM_BUFFER_SIZE )) continue;
            if (!(ret = func( hwnd, string, (HANDLE)(ULONG_PTR)list[i].data, lParam ))) break;
        }
        HeapFree( GetProcessHeap(), 0, list );
    }
    return ret;
}
