/*
 * Window stations and desktops
 *
 * Copyright 2002 Alexandre Julliard
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

#include "ntstatus.h"
#define WIN32_NO_STATUS

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winerror.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/winternl.h"
#include "wine/server.h"
#include "wine/unicode.h"
#include "wine/debug.h"
#include "user_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(winstation);


/* callback for enumeration functions */
struct enum_proc_lparam
{
    NAMEENUMPROCA func;
    LPARAM        lparam;
};

static BOOL CALLBACK enum_names_WtoA( LPWSTR name, LPARAM lparam )
{
    struct enum_proc_lparam *data = (struct enum_proc_lparam *)lparam;
    char buffer[MAX_PATH];

    if (!WideCharToMultiByte( CP_ACP, 0, name, -1, buffer, sizeof(buffer), NULL, NULL ))
        return FALSE;
    return data->func( buffer, data->lparam );
}


/***********************************************************************
 *              CreateWindowStationA  (USER32.@)
 */
HWINSTA WINAPI CreateWindowStationA( LPCSTR name, DWORD reserved, ACCESS_MASK access,
                                     LPSECURITY_ATTRIBUTES sa )
{
    WCHAR buffer[MAX_PATH];

    if (!name) return CreateWindowStationW( NULL, reserved, access, sa );

    if (!MultiByteToWideChar( CP_ACP, 0, name, -1, buffer, MAX_PATH ))
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    return CreateWindowStationW( buffer, reserved, access, sa );
}


/***********************************************************************
 *              CreateWindowStationW  (USER32.@)
 */
HWINSTA WINAPI CreateWindowStationW( LPCWSTR name, DWORD reserved, ACCESS_MASK access,
                                     LPSECURITY_ATTRIBUTES sa )
{
    HANDLE ret;
    DWORD len = name ? strlenW(name) : 0;

    if (len >= MAX_PATH)
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    SERVER_START_REQ( create_winstation )
    {
        req->flags      = 0;
        req->access     = access;
        req->attributes = OBJ_CASE_INSENSITIVE | OBJ_OPENIF |
                          ((sa && sa->bInheritHandle) ? OBJ_INHERIT : 0);
        wine_server_add_data( req, name, len * sizeof(WCHAR) );
        /* it doesn't seem to set last error */
        wine_server_call( req );
        ret = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 *              OpenWindowStationA  (USER32.@)
 */
HWINSTA WINAPI OpenWindowStationA( LPCSTR name, BOOL inherit, ACCESS_MASK access )
{
    WCHAR buffer[MAX_PATH];

    if (!name) return OpenWindowStationW( NULL, inherit, access );

    if (!MultiByteToWideChar( CP_ACP, 0, name, -1, buffer, MAX_PATH ))
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    return OpenWindowStationW( buffer, inherit, access );
}


/******************************************************************************
 *              OpenWindowStationW  (USER32.@)
 */
HWINSTA WINAPI OpenWindowStationW( LPCWSTR name, BOOL inherit, ACCESS_MASK access )
{
    HANDLE ret = 0;
    DWORD len = name ? strlenW(name) : 0;
    if (len >= MAX_PATH)
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    SERVER_START_REQ( open_winstation )
    {
        req->access     = access;
        req->attributes = OBJ_CASE_INSENSITIVE | (inherit ? OBJ_INHERIT : 0);
        wine_server_add_data( req, name, len * sizeof(WCHAR) );
        if (!wine_server_call_err( req )) ret = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *              CloseWindowStation  (USER32.@)
 */
BOOL WINAPI CloseWindowStation( HWINSTA handle )
{
    BOOL ret;
    SERVER_START_REQ( close_winstation )
    {
        req->handle = wine_server_obj_handle( handle );
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 *              GetProcessWindowStation  (USER32.@)
 */
HWINSTA WINAPI GetProcessWindowStation(void)
{
    HWINSTA ret = 0;

    SERVER_START_REQ( get_process_winstation )
    {
        if (!wine_server_call_err( req ))
            ret = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *              SetProcessWindowStation  (USER32.@)
 */
BOOL WINAPI SetProcessWindowStation( HWINSTA handle )
{
    BOOL ret;

    SERVER_START_REQ( set_process_winstation )
    {
        req->handle = wine_server_obj_handle( handle );
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 *              EnumWindowStationsA  (USER32.@)
 */
BOOL WINAPI EnumWindowStationsA( WINSTAENUMPROCA func, LPARAM lparam )
{
    struct enum_proc_lparam data;
    data.func   = func;
    data.lparam = lparam;
    return EnumWindowStationsW( enum_names_WtoA, (LPARAM)&data );
}


/******************************************************************************
 *              EnumWindowStationsW  (USER32.@)
 */
BOOL WINAPI EnumWindowStationsW( WINSTAENUMPROCW func, LPARAM lparam )
{
    unsigned int index = 0;
    WCHAR name[MAX_PATH];
    BOOL ret = TRUE;
    NTSTATUS status;

    while (ret)
    {
        SERVER_START_REQ( enum_winstation )
        {
            req->index = index;
            wine_server_set_reply( req, name, sizeof(name) - sizeof(WCHAR) );
            status = wine_server_call( req );
            name[wine_server_reply_size(reply)/sizeof(WCHAR)] = 0;
            index = reply->next;
        }
        SERVER_END_REQ;
        if (status == STATUS_NO_MORE_ENTRIES)
            break;
        if (status)
        {
            SetLastError( RtlNtStatusToDosError( status ) );
            return FALSE;
        }
        ret = func( name, lparam );
    }
    return ret;
}


/***********************************************************************
 *              CreateDesktopA   (USER32.@)
 */
HDESK WINAPI CreateDesktopA( LPCSTR name, LPCSTR device, LPDEVMODEA devmode,
                             DWORD flags, ACCESS_MASK access, LPSECURITY_ATTRIBUTES sa )
{
    WCHAR buffer[MAX_PATH];

    if (device || devmode)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    if (!name) return CreateDesktopW( NULL, NULL, NULL, flags, access, sa );

    if (!MultiByteToWideChar( CP_ACP, 0, name, -1, buffer, MAX_PATH ))
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    return CreateDesktopW( buffer, NULL, NULL, flags, access, sa );
}


/***********************************************************************
 *              CreateDesktopW   (USER32.@)
 */
HDESK WINAPI CreateDesktopW( LPCWSTR name, LPCWSTR device, LPDEVMODEW devmode,
                             DWORD flags, ACCESS_MASK access, LPSECURITY_ATTRIBUTES sa )
{
    HANDLE ret;
    DWORD len = name ? strlenW(name) : 0;

    if (device || devmode)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    if (len >= MAX_PATH)
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    SERVER_START_REQ( create_desktop )
    {
        req->flags      = flags;
        req->access     = access;
        req->attributes = OBJ_CASE_INSENSITIVE | OBJ_OPENIF |
                          ((sa && sa->bInheritHandle) ? OBJ_INHERIT : 0);
        wine_server_add_data( req, name, len * sizeof(WCHAR) );
        /* it doesn't seem to set last error */
        wine_server_call( req );
        ret = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 *              OpenDesktopA   (USER32.@)
 */
HDESK WINAPI OpenDesktopA( LPCSTR name, DWORD flags, BOOL inherit, ACCESS_MASK access )
{
    WCHAR buffer[MAX_PATH];

    if (!name) return OpenDesktopW( NULL, flags, inherit, access );

    if (!MultiByteToWideChar( CP_ACP, 0, name, -1, buffer, MAX_PATH ))
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    return OpenDesktopW( buffer, flags, inherit, access );
}


HDESK open_winstation_desktop( HWINSTA hwinsta, LPCWSTR name, DWORD flags, BOOL inherit, ACCESS_MASK access )
{
    HANDLE ret = 0;
    DWORD len = name ? strlenW(name) : 0;
    if (len >= MAX_PATH)
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    SERVER_START_REQ( open_desktop )
    {
        req->winsta     = wine_server_obj_handle( hwinsta );
        req->flags      = flags;
        req->access     = access;
        req->attributes = OBJ_CASE_INSENSITIVE | (inherit ? OBJ_INHERIT : 0);
        wine_server_add_data( req, name, len * sizeof(WCHAR) );
        if (!wine_server_call( req )) ret = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 *              OpenDesktopW   (USER32.@)
 */
HDESK WINAPI OpenDesktopW( LPCWSTR name, DWORD flags, BOOL inherit, ACCESS_MASK access )
{
    return open_winstation_desktop( NULL, name, flags, inherit, access );
}


/***********************************************************************
 *              CloseDesktop  (USER32.@)
 */
BOOL WINAPI CloseDesktop( HDESK handle )
{
    BOOL ret;
    SERVER_START_REQ( close_desktop )
    {
        req->handle = wine_server_obj_handle( handle );
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 *              GetThreadDesktop   (USER32.@)
 */
HDESK WINAPI GetThreadDesktop( DWORD thread )
{
    HDESK ret = 0;

    SERVER_START_REQ( get_thread_desktop )
    {
        req->tid = thread;
        if (!wine_server_call_err( req )) ret = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 *              SetThreadDesktop   (USER32.@)
 */
BOOL WINAPI SetThreadDesktop( HDESK handle )
{
    BOOL ret;

    SERVER_START_REQ( set_thread_desktop )
    {
        req->handle = wine_server_obj_handle( handle );
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    if (ret)  /* reset the desktop windows */
    {
        struct user_thread_info *thread_info = get_user_thread_info();
        thread_info->top_window = 0;
        thread_info->msg_window = 0;
    }
    return ret;
}


/******************************************************************************
 *              EnumDesktopsA   (USER32.@)
 */
BOOL WINAPI EnumDesktopsA( HWINSTA winsta, DESKTOPENUMPROCA func, LPARAM lparam )
{
    struct enum_proc_lparam data;
    data.func   = func;
    data.lparam = lparam;
    return EnumDesktopsW( winsta, enum_names_WtoA, (LPARAM)&data );
}


/******************************************************************************
 *              EnumDesktopsW   (USER32.@)
 */
BOOL WINAPI EnumDesktopsW( HWINSTA winsta, DESKTOPENUMPROCW func, LPARAM lparam )
{
    unsigned int index = 0;
    WCHAR name[MAX_PATH];
    BOOL ret = TRUE;
    NTSTATUS status;

    if (!winsta)
        winsta = GetProcessWindowStation();

    while (ret)
    {
        SERVER_START_REQ( enum_desktop )
        {
            req->winstation = wine_server_obj_handle( winsta );
            req->index      = index;
            wine_server_set_reply( req, name, sizeof(name) - sizeof(WCHAR) );
            status = wine_server_call( req );
            name[wine_server_reply_size(reply)/sizeof(WCHAR)] = 0;
            index = reply->next;
        }
        SERVER_END_REQ;
        if (status == STATUS_NO_MORE_ENTRIES)
            break;
        if (status)
        {
            SetLastError( RtlNtStatusToDosError( status ) );
            return FALSE;
        }
        ret = func(name, lparam);
    }
    return ret;
}


/******************************************************************************
 *              OpenInputDesktop   (USER32.@)
 */
HDESK WINAPI OpenInputDesktop( DWORD flags, BOOL inherit, ACCESS_MASK access )
{
    FIXME( "(%x,%i,%x): stub\n", flags, inherit, access );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return 0;
}


/***********************************************************************
 *              GetUserObjectInformationA   (USER32.@)
 */
BOOL WINAPI GetUserObjectInformationA( HANDLE handle, INT index, LPVOID info, DWORD len, LPDWORD needed )
{
    /* check for information types returning strings */
    if (index == UOI_TYPE || index == UOI_NAME)
    {
        WCHAR buffer[MAX_PATH];
        DWORD lenA;

        if (!GetUserObjectInformationW( handle, index, buffer, sizeof(buffer), NULL )) return FALSE;
        lenA = WideCharToMultiByte( CP_ACP, 0, buffer, -1, NULL, 0, NULL, NULL );
        if (needed) *needed = lenA;
        if (lenA > len)
        {
            SetLastError( ERROR_MORE_DATA );
            return FALSE;
        }
        if (info) WideCharToMultiByte( CP_ACP, 0, buffer, -1, info, len, NULL, NULL );
        return TRUE;
    }
    return GetUserObjectInformationW( handle, index, info, len, needed );
}


/***********************************************************************
 *              GetUserObjectInformationW   (USER32.@)
 */
BOOL WINAPI GetUserObjectInformationW( HANDLE handle, INT index, LPVOID info, DWORD len, LPDWORD needed )
{
    static const WCHAR desktopW[] = { 'D','e','s','k','t','o','p',0 };
    static const WCHAR winstationW[] = { 'W','i','n','d','o','w','S','t','a','t','i','o','n',0 };
    BOOL ret;

    switch(index)
    {
    case UOI_FLAGS:
        {
            USEROBJECTFLAGS *obj_flags = info;
            if (needed) *needed = sizeof(*obj_flags);
            if (len < sizeof(*obj_flags))
            {
                SetLastError( ERROR_BUFFER_OVERFLOW );
                return FALSE;
            }
            SERVER_START_REQ( set_user_object_info )
            {
                req->handle    = wine_server_obj_handle( handle );
                req->flags     = 0;
                ret = !wine_server_call_err( req );
                if (ret)
                {
                    /* FIXME: inherit flag */
                    obj_flags->dwFlags = reply->old_obj_flags;
                }
            }
            SERVER_END_REQ;
        }
        return ret;

    case UOI_TYPE:
        SERVER_START_REQ( set_user_object_info )
        {
            req->handle = wine_server_obj_handle( handle );
            req->flags  = 0;
            ret = !wine_server_call_err( req );
            if (ret)
            {
                size_t size = reply->is_desktop ? sizeof(desktopW) : sizeof(winstationW);
                if (needed) *needed = size;
                if (len < size)
                {
                    SetLastError( ERROR_MORE_DATA );
                    ret = FALSE;
                }
                else memcpy( info, reply->is_desktop ? desktopW : winstationW, size );
            }
        }
        SERVER_END_REQ;
        return ret;

    case UOI_NAME:
        {
            WCHAR buffer[MAX_PATH];
            SERVER_START_REQ( set_user_object_info )
            {
                req->handle = wine_server_obj_handle( handle );
                req->flags  = 0;
                wine_server_set_reply( req, buffer, sizeof(buffer) - sizeof(WCHAR) );
                ret = !wine_server_call_err( req );
                if (ret)
                {
                    size_t size = wine_server_reply_size( reply );
                    buffer[size / sizeof(WCHAR)] = 0;
                    size += sizeof(WCHAR);
                    if (needed) *needed = size;
                    if (len < size)
                    {
                        SetLastError( ERROR_MORE_DATA );
                        ret = FALSE;
                    }
                    else memcpy( info, buffer, size );
                }
            }
            SERVER_END_REQ;
        }
        return ret;

    case UOI_USER_SID:
        FIXME( "not supported index %d\n", index );
        /* fall through */
    default:
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
}


/******************************************************************************
 *              SetUserObjectInformationA   (USER32.@)
 */
BOOL WINAPI SetUserObjectInformationA( HANDLE handle, INT index, LPVOID info, DWORD len )
{
    return SetUserObjectInformationW( handle, index, info, len );
}


/******************************************************************************
 *              SetUserObjectInformationW   (USER32.@)
 */
BOOL WINAPI SetUserObjectInformationW( HANDLE handle, INT index, LPVOID info, DWORD len )
{
    BOOL ret;
    const USEROBJECTFLAGS *obj_flags = info;

    if (index != UOI_FLAGS || !info || len < sizeof(*obj_flags))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    /* FIXME: inherit flag */
    SERVER_START_REQ( set_user_object_info )
    {
        req->handle    = wine_server_obj_handle( handle );
        req->flags     = SET_USER_OBJECT_FLAGS;
        req->obj_flags = obj_flags->dwFlags;
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *              GetUserObjectSecurity   (USER32.@)
 */
BOOL WINAPI GetUserObjectSecurity( HANDLE handle, PSECURITY_INFORMATION info,
                                   PSECURITY_DESCRIPTOR sid, DWORD len, LPDWORD needed )
{
    FIXME( "(%p %p %p len=%d %p),stub!\n", handle, info, sid, len, needed );
    if (needed)
        *needed = sizeof(SECURITY_DESCRIPTOR);
    if (len < sizeof(SECURITY_DESCRIPTOR))
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
    }
    return InitializeSecurityDescriptor(sid, SECURITY_DESCRIPTOR_REVISION);
}

/***********************************************************************
 *              SetUserObjectSecurity   (USER32.@)
 */
BOOL WINAPI SetUserObjectSecurity( HANDLE handle, PSECURITY_INFORMATION info,
                                   PSECURITY_DESCRIPTOR sid )
{
    FIXME( "(%p,%p,%p),stub!\n", handle, info, sid );
    return TRUE;
}
