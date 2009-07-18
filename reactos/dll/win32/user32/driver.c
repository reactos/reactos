/*
 * USER driver support
 *
 * Copyright 2000, 2005 Alexandre Julliard
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
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wine/debug.h"

#include "user_private.h"

static USER_DRIVER null_driver, lazy_load_driver;

const USER_DRIVER *USER_Driver = &lazy_load_driver;
static DWORD driver_load_error;

/* load the graphics driver */
static const USER_DRIVER *load_driver(void)
{
    char buffer[MAX_PATH], libname[32], *name, *next;
    HKEY hkey;
    void *ptr;
    HMODULE graphics_driver;
    USER_DRIVER *driver, *prev;

#ifndef __REACTOS__
    strcpy( buffer, "x11" );  /* default value */
#else
    strcpy( buffer, "nt" );
#endif
#if 0
    /* @@ Wine registry key: HKCU\Software\Wine\Drivers */
    if (!RegOpenKeyA( HKEY_CURRENT_USER, "Software\\Wine\\Drivers", &hkey ))
    {
        DWORD type, count = sizeof(buffer);
        RegQueryValueExA( hkey, "Graphics", 0, &type, (LPBYTE) buffer, &count );
        RegCloseKey( hkey );
    }
#endif
    name = buffer;
    while (name)
    {
        next = strchr( name, ',' );
        if (next) *next++ = 0;

        _snprintf( libname, sizeof(libname), "wine%s.drv", name );
        if ((graphics_driver = LoadLibraryA( libname )) != 0) break;
        name = next;
    }

    if (!graphics_driver)
        driver_load_error = GetLastError();

    driver = HeapAlloc( GetProcessHeap(), 0, sizeof(*driver) );
    *driver = null_driver;

    if (graphics_driver)
    {
#define GET_USER_FUNC(name) \
    do { if ((ptr = GetProcAddress( graphics_driver, #name ))) driver->p##name = ptr; } while(0)

        GET_USER_FUNC(ActivateKeyboardLayout);
        GET_USER_FUNC(Beep);
        GET_USER_FUNC(GetAsyncKeyState);
        GET_USER_FUNC(GetKeyNameText);
        GET_USER_FUNC(GetKeyboardLayout);
        GET_USER_FUNC(GetKeyboardLayoutName);
        GET_USER_FUNC(LoadKeyboardLayout);
        GET_USER_FUNC(MapVirtualKeyEx);
        GET_USER_FUNC(SendInput);
        GET_USER_FUNC(ToUnicodeEx);
        GET_USER_FUNC(UnloadKeyboardLayout);
        GET_USER_FUNC(VkKeyScanEx);
        GET_USER_FUNC(SetCursor);
        GET_USER_FUNC(GetCursorPos);
        GET_USER_FUNC(SetCursorPos);
        GET_USER_FUNC(ClipCursor);
        GET_USER_FUNC(GetScreenSaveActive);
        GET_USER_FUNC(SetScreenSaveActive);
        GET_USER_FUNC(AcquireClipboard);
        GET_USER_FUNC(EmptyClipboard);
        GET_USER_FUNC(SetClipboardData);
        GET_USER_FUNC(GetClipboardData);
        GET_USER_FUNC(CountClipboardFormats);
        GET_USER_FUNC(EnumClipboardFormats);
        GET_USER_FUNC(IsClipboardFormatAvailable);
        GET_USER_FUNC(RegisterClipboardFormat);
        GET_USER_FUNC(GetClipboardFormatName);
        GET_USER_FUNC(EndClipboardUpdate);
        GET_USER_FUNC(ChangeDisplaySettingsEx);
        GET_USER_FUNC(EnumDisplayMonitors);
        GET_USER_FUNC(EnumDisplaySettingsEx);
        GET_USER_FUNC(GetMonitorInfo);
        GET_USER_FUNC(CreateDesktopWindow);
        GET_USER_FUNC(CreateWindow);
        GET_USER_FUNC(DestroyWindow);
        GET_USER_FUNC(GetDC);
        GET_USER_FUNC(MsgWaitForMultipleObjectsEx);
        GET_USER_FUNC(ReleaseDC);
        GET_USER_FUNC(ScrollDC);
        GET_USER_FUNC(SetCapture);
        GET_USER_FUNC(SetFocus);
        GET_USER_FUNC(SetLayeredWindowAttributes);
        GET_USER_FUNC(SetParent);
        GET_USER_FUNC(SetWindowRgn);
        GET_USER_FUNC(SetWindowIcon);
        GET_USER_FUNC(SetWindowStyle);
        GET_USER_FUNC(SetWindowText);
        GET_USER_FUNC(ShowWindow);
        GET_USER_FUNC(SysCommand);
        GET_USER_FUNC(WindowMessage);
        GET_USER_FUNC(WindowPosChanging);
        GET_USER_FUNC(WindowPosChanged);
#undef GET_USER_FUNC
    }

    prev = InterlockedCompareExchangePointer( (void **)&USER_Driver, driver, &lazy_load_driver );
    if (prev != &lazy_load_driver)
    {
        /* another thread beat us to it */
        HeapFree( GetProcessHeap(), 0, driver );
        FreeLibrary( graphics_driver );
        driver = prev;
    }
    return driver;
}

/* unload the graphics driver on process exit */
void USER_unload_driver(void)
{
    USER_DRIVER *prev;
    /* make sure we don't try to call the driver after it has been detached */
    prev = InterlockedExchangePointer( (void **)&USER_Driver, &null_driver );
    if (prev != &lazy_load_driver && prev != &null_driver)
        HeapFree( GetProcessHeap(), 0, prev );
}


/**********************************************************************
 * Null user driver
 *
 * These are fallbacks for entry points that are not implemented in the real driver.
 */

static HKL CDECL nulldrv_ActivateKeyboardLayout( HKL layout, UINT flags )
{
    return 0;
}

static void CDECL nulldrv_Beep(void)
{
}

static SHORT CDECL nulldrv_GetAsyncKeyState( INT key )
{
    return 0;
}

static INT CDECL nulldrv_GetKeyNameText( LONG lparam, LPWSTR buffer, INT size )
{
    return 0;
}

static HKL CDECL nulldrv_GetKeyboardLayout( DWORD layout )
{
    return 0;
}

static BOOL CDECL nulldrv_GetKeyboardLayoutName( LPWSTR name )
{
    return FALSE;
}

static HKL CDECL nulldrv_LoadKeyboardLayout( LPCWSTR name, UINT flags )
{
    return 0;
}

static UINT CDECL nulldrv_MapVirtualKeyEx( UINT code, UINT type, HKL layout )
{
    return 0;
}

static UINT CDECL nulldrv_SendInput( UINT count, LPINPUT inputs, int size )
{
    return 0;
}

static INT CDECL nulldrv_ToUnicodeEx( UINT virt, UINT scan, const BYTE *state, LPWSTR str,
                                      int size, UINT flags, HKL layout )
{
    return 0;
}

static BOOL CDECL nulldrv_UnloadKeyboardLayout( HKL layout )
{
    return 0;
}

static SHORT CDECL nulldrv_VkKeyScanEx( WCHAR ch, HKL layout )
{
    return -1;
}

static void CDECL nulldrv_SetCursor( struct tagCURSORICONINFO *info )
{
}

static BOOL CDECL nulldrv_GetCursorPos( LPPOINT pt )
{
    return FALSE;
}

static BOOL CDECL nulldrv_SetCursorPos( INT x, INT y )
{
    return FALSE;
}

static BOOL CDECL nulldrv_ClipCursor( LPCRECT clip )
{
    return FALSE;
}

static BOOL CDECL nulldrv_GetScreenSaveActive(void)
{
    return FALSE;
}

static void CDECL nulldrv_SetScreenSaveActive( BOOL on )
{
}

static INT CDECL nulldrv_AcquireClipboard( HWND hwnd )
{
    return 0;
}

static BOOL CDECL nulldrv_CountClipboardFormats(void)
{
    return 0;
}

static void CDECL nulldrv_EmptyClipboard( BOOL keepunowned )
{
}

static void CDECL nulldrv_EndClipboardUpdate(void)
{
}

static UINT CDECL nulldrv_EnumClipboardFormats( UINT format )
{
    return 0;
}

static BOOL CDECL nulldrv_GetClipboardData( UINT format, HANDLE16 *h16, HANDLE *h32 )
{
    return FALSE;
}

static INT CDECL nulldrv_GetClipboardFormatName( UINT format, LPWSTR buffer, UINT len )
{
    return FALSE;
}

static BOOL CDECL nulldrv_IsClipboardFormatAvailable( UINT format )
{
    return FALSE;
}

static UINT CDECL nulldrv_RegisterClipboardFormat( LPCWSTR name )
{
    return 0;
}

static BOOL CDECL nulldrv_SetClipboardData( UINT format, HANDLE16 h16, HANDLE h32, BOOL owner )
{
    return FALSE;
}

static LONG CDECL nulldrv_ChangeDisplaySettingsEx( LPCWSTR name, LPDEVMODEW mode, HWND hwnd,
                                             DWORD flags, LPVOID lparam )
{
    return DISP_CHANGE_FAILED;
}

static BOOL CDECL nulldrv_EnumDisplayMonitors( HDC hdc, LPRECT rect, MONITORENUMPROC proc, LPARAM lp )
{
    return FALSE;
}

static BOOL CDECL nulldrv_EnumDisplaySettingsEx( LPCWSTR name, DWORD num, LPDEVMODEW mode, DWORD flags )
{
    return FALSE;
}

static BOOL CDECL nulldrv_GetMonitorInfo( HMONITOR handle, LPMONITORINFO info )
{
    return FALSE;
}

static BOOL CDECL nulldrv_CreateDesktopWindow( HWND hwnd )
{
    return TRUE;
}

static BOOL CDECL nulldrv_CreateWindow( HWND hwnd )
{
    static int warned;
    if (warned++)
        return FALSE;

    MESSAGE( "Application tried to create a window, but no driver could be loaded.\n");
    switch (driver_load_error)
    {
    case ERROR_MOD_NOT_FOUND:
        MESSAGE( "The X11 driver is missing.  Check your build!\n" );
        break;
    case ERROR_DLL_INIT_FAILED:
        MESSAGE( "Make sure that your X server is running and that $DISPLAY is set correctly.\n" );
        break;
    default:
        MESSAGE( "Unknown error (%d).\n", driver_load_error );
    }

    return FALSE;
}

static void CDECL nulldrv_DestroyWindow( HWND hwnd )
{
}

static void CDECL nulldrv_GetDC( HDC hdc, HWND hwnd, HWND top_win, const RECT *win_rect,
                                 const RECT *top_rect, DWORD flags )
{
}

static DWORD CDECL nulldrv_MsgWaitForMultipleObjectsEx( DWORD count, const HANDLE *handles, DWORD timeout,
                                                        DWORD mask, DWORD flags )
{
    return WaitForMultipleObjectsEx( count, handles, flags & MWMO_WAITALL,
                                     timeout, flags & MWMO_ALERTABLE );
}

static void CDECL nulldrv_ReleaseDC( HWND hwnd, HDC hdc )
{
}

static BOOL CDECL nulldrv_ScrollDC( HDC hdc, INT dx, INT dy, const RECT *scroll, const RECT *clip,
                                    HRGN hrgn, LPRECT update )
{
    return FALSE;
}

static void CDECL nulldrv_SetCapture( HWND hwnd, UINT flags )
{
}

static void CDECL nulldrv_SetFocus( HWND hwnd )
{
}

static void CDECL nulldrv_SetLayeredWindowAttributes( HWND hwnd, COLORREF key, BYTE alpha, DWORD flags )
{
}

static void CDECL nulldrv_SetParent( HWND hwnd, HWND parent, HWND old_parent )
{
}

static int CDECL nulldrv_SetWindowRgn( HWND hwnd, HRGN hrgn, BOOL redraw )
{
    return 1;
}

static void CDECL nulldrv_SetWindowIcon( HWND hwnd, UINT type, HICON icon )
{
}

static void CDECL nulldrv_SetWindowStyle( HWND hwnd, INT offset, STYLESTRUCT *style )
{
}

static void CDECL nulldrv_SetWindowText( HWND hwnd, LPCWSTR text )
{
}

static UINT CDECL nulldrv_ShowWindow( HWND hwnd, INT cmd, RECT *rect, UINT swp )
{
    return swp;
}

static LRESULT CDECL nulldrv_SysCommand( HWND hwnd, WPARAM wparam, LPARAM lparam )
{
    return -1;
}

static LRESULT CDECL nulldrv_WindowMessage( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    return 0;
}

static void CDECL nulldrv_WindowPosChanging( HWND hwnd, HWND insert_after, UINT swp_flags,
                                             const RECT *window_rect, const RECT *client_rect,
                                             RECT *visible_rect )
{
}

static void CDECL nulldrv_WindowPosChanged( HWND hwnd, HWND insert_after, UINT swp_flags,
                                            const RECT *window_rect, const RECT *client_rect,
                                            const RECT *visible_rect, const RECT *valid_rects )
{
}

static USER_DRIVER null_driver =
{
    /* keyboard functions */
    nulldrv_ActivateKeyboardLayout,
    nulldrv_Beep,
    nulldrv_GetAsyncKeyState,
    nulldrv_GetKeyNameText,
    nulldrv_GetKeyboardLayout,
    nulldrv_GetKeyboardLayoutName,
    nulldrv_LoadKeyboardLayout,
    nulldrv_MapVirtualKeyEx,
    nulldrv_SendInput,
    nulldrv_ToUnicodeEx,
    nulldrv_UnloadKeyboardLayout,
    nulldrv_VkKeyScanEx,
    /* mouse functions */
    nulldrv_SetCursor,
    nulldrv_GetCursorPos,
    nulldrv_SetCursorPos,
    nulldrv_ClipCursor,
    /* screen saver functions */
    nulldrv_GetScreenSaveActive,
    nulldrv_SetScreenSaveActive,
    /* clipboard functions */
    nulldrv_AcquireClipboard,
    nulldrv_CountClipboardFormats,
    nulldrv_EmptyClipboard,
    nulldrv_EndClipboardUpdate,
    nulldrv_EnumClipboardFormats,
    nulldrv_GetClipboardData,
    nulldrv_GetClipboardFormatName,
    nulldrv_IsClipboardFormatAvailable,
    nulldrv_RegisterClipboardFormat,
    nulldrv_SetClipboardData,
    /* display modes */
    nulldrv_ChangeDisplaySettingsEx,
    nulldrv_EnumDisplayMonitors,
    nulldrv_EnumDisplaySettingsEx,
    nulldrv_GetMonitorInfo,
    /* windowing functions */
    nulldrv_CreateDesktopWindow,
    nulldrv_CreateWindow,
    nulldrv_DestroyWindow,
    nulldrv_GetDC,
    nulldrv_MsgWaitForMultipleObjectsEx,
    nulldrv_ReleaseDC,
    nulldrv_ScrollDC,
    nulldrv_SetCapture,
    nulldrv_SetFocus,
    nulldrv_SetLayeredWindowAttributes,
    nulldrv_SetParent,
    nulldrv_SetWindowRgn,
    nulldrv_SetWindowIcon,
    nulldrv_SetWindowStyle,
    nulldrv_SetWindowText,
    nulldrv_ShowWindow,
    nulldrv_SysCommand,
    nulldrv_WindowMessage,
    nulldrv_WindowPosChanging,
    nulldrv_WindowPosChanged
};


/**********************************************************************
 * Lazy loading user driver
 *
 * Initial driver used before another driver is loaded.
 * Each entry point simply loads the real driver and chains to it.
 */

static HKL CDECL loaderdrv_ActivateKeyboardLayout( HKL layout, UINT flags )
{
    return load_driver()->pActivateKeyboardLayout( layout, flags );
}

static void CDECL loaderdrv_Beep(void)
{
    load_driver()->pBeep();
}

static SHORT CDECL loaderdrv_GetAsyncKeyState( INT key )
{
    return load_driver()->pGetAsyncKeyState( key );
}

static INT CDECL loaderdrv_GetKeyNameText( LONG lparam, LPWSTR buffer, INT size )
{
    return load_driver()->pGetKeyNameText( lparam, buffer, size );
}

static HKL CDECL loaderdrv_GetKeyboardLayout( DWORD layout )
{
    return load_driver()->pGetKeyboardLayout( layout );
}

static BOOL CDECL loaderdrv_GetKeyboardLayoutName( LPWSTR name )
{
    return load_driver()->pGetKeyboardLayoutName( name );
}

static HKL CDECL loaderdrv_LoadKeyboardLayout( LPCWSTR name, UINT flags )
{
    return load_driver()->pLoadKeyboardLayout( name, flags );
}

static UINT CDECL loaderdrv_MapVirtualKeyEx( UINT code, UINT type, HKL layout )
{
    return load_driver()->pMapVirtualKeyEx( code, type, layout );
}

static UINT CDECL loaderdrv_SendInput( UINT count, LPINPUT inputs, int size )
{
    return load_driver()->pSendInput( count, inputs, size );
}

static INT CDECL loaderdrv_ToUnicodeEx( UINT virt, UINT scan, const BYTE *state, LPWSTR str,
                                  int size, UINT flags, HKL layout )
{
    return load_driver()->pToUnicodeEx( virt, scan, state, str, size, flags, layout );
}

static BOOL CDECL loaderdrv_UnloadKeyboardLayout( HKL layout )
{
    return load_driver()->pUnloadKeyboardLayout( layout );
}

static SHORT CDECL loaderdrv_VkKeyScanEx( WCHAR ch, HKL layout )
{
    return load_driver()->pVkKeyScanEx( ch, layout );
}

static void CDECL loaderdrv_SetCursor( struct tagCURSORICONINFO *info )
{
    load_driver()->pSetCursor( info );
}

static BOOL CDECL loaderdrv_GetCursorPos( LPPOINT pt )
{
    return load_driver()->pGetCursorPos( pt );
}

static BOOL CDECL loaderdrv_SetCursorPos( INT x, INT y )
{
    return load_driver()->pSetCursorPos( x, y );
}

static BOOL CDECL loaderdrv_ClipCursor( LPCRECT clip )
{
    return load_driver()->pClipCursor( clip );
}

static BOOL CDECL loaderdrv_GetScreenSaveActive(void)
{
    return load_driver()->pGetScreenSaveActive();
}

static void CDECL loaderdrv_SetScreenSaveActive( BOOL on )
{
    load_driver()->pSetScreenSaveActive( on );
}

static INT CDECL loaderdrv_AcquireClipboard( HWND hwnd )
{
    return load_driver()->pAcquireClipboard( hwnd );
}

static BOOL CDECL loaderdrv_CountClipboardFormats(void)
{
    return load_driver()->pCountClipboardFormats();
}

static void CDECL loaderdrv_EmptyClipboard( BOOL keepunowned )
{
    load_driver()->pEmptyClipboard( keepunowned );
}

static void CDECL loaderdrv_EndClipboardUpdate(void)
{
    load_driver()->pEndClipboardUpdate();
}

static UINT CDECL loaderdrv_EnumClipboardFormats( UINT format )
{
    return load_driver()->pEnumClipboardFormats( format );
}

static BOOL CDECL loaderdrv_GetClipboardData( UINT format, HANDLE16 *h16, HANDLE *h32 )
{
    return load_driver()->pGetClipboardData( format, h16, h32 );
}

static INT CDECL loaderdrv_GetClipboardFormatName( UINT format, LPWSTR buffer, UINT len )
{
    return load_driver()->pGetClipboardFormatName( format, buffer, len );
}

static BOOL CDECL loaderdrv_IsClipboardFormatAvailable( UINT format )
{
    return load_driver()->pIsClipboardFormatAvailable( format );
}

static UINT CDECL loaderdrv_RegisterClipboardFormat( LPCWSTR name )
{
    return load_driver()->pRegisterClipboardFormat( name );
}

static BOOL CDECL loaderdrv_SetClipboardData( UINT format, HANDLE16 h16, HANDLE h32, BOOL owner )
{
    return load_driver()->pSetClipboardData( format, h16, h32, owner );
}

static LONG CDECL loaderdrv_ChangeDisplaySettingsEx( LPCWSTR name, LPDEVMODEW mode, HWND hwnd,
                                                     DWORD flags, LPVOID lparam )
{
    return load_driver()->pChangeDisplaySettingsEx( name, mode, hwnd, flags, lparam );
}

static BOOL CDECL loaderdrv_EnumDisplayMonitors( HDC hdc, LPRECT rect, MONITORENUMPROC proc, LPARAM lp )
{
    return load_driver()->pEnumDisplayMonitors( hdc, rect, proc, lp );
}

static BOOL CDECL loaderdrv_EnumDisplaySettingsEx( LPCWSTR name, DWORD num, LPDEVMODEW mode, DWORD flags )
{
    return load_driver()->pEnumDisplaySettingsEx( name, num, mode, flags );
}

static BOOL CDECL loaderdrv_GetMonitorInfo( HMONITOR handle, LPMONITORINFO info )
{
    return load_driver()->pGetMonitorInfo( handle, info );
}

static BOOL CDECL loaderdrv_CreateDesktopWindow( HWND hwnd )
{
    return load_driver()->pCreateDesktopWindow( hwnd );
}

static BOOL CDECL loaderdrv_CreateWindow( HWND hwnd )
{
    return load_driver()->pCreateWindow( hwnd );
}

static void CDECL loaderdrv_DestroyWindow( HWND hwnd )
{
    load_driver()->pDestroyWindow( hwnd );
}

static void CDECL loaderdrv_GetDC( HDC hdc, HWND hwnd, HWND top_win, const RECT *win_rect,
                                   const RECT *top_rect, DWORD flags )
{
    load_driver()->pGetDC( hdc, hwnd, top_win, win_rect, top_rect, flags );
}

static DWORD CDECL loaderdrv_MsgWaitForMultipleObjectsEx( DWORD count, const HANDLE *handles, DWORD timeout,
                                                          DWORD mask, DWORD flags )
{
    return load_driver()->pMsgWaitForMultipleObjectsEx( count, handles, timeout, mask, flags );
}

static void CDECL loaderdrv_ReleaseDC( HWND hwnd, HDC hdc )
{
    load_driver()->pReleaseDC( hwnd, hdc );
}

static BOOL CDECL loaderdrv_ScrollDC( HDC hdc, INT dx, INT dy, const RECT *scroll, const RECT *clip,
                                      HRGN hrgn, LPRECT update )
{
    return load_driver()->pScrollDC( hdc, dx, dy, scroll, clip, hrgn, update );
}

static void CDECL loaderdrv_SetCapture( HWND hwnd, UINT flags )
{
    load_driver()->pSetCapture( hwnd, flags );
}

static void CDECL loaderdrv_SetFocus( HWND hwnd )
{
    load_driver()->pSetFocus( hwnd );
}

static void CDECL loaderdrv_SetLayeredWindowAttributes( HWND hwnd, COLORREF key, BYTE alpha, DWORD flags )
{
    load_driver()->pSetLayeredWindowAttributes( hwnd, key, alpha, flags );
}

static void CDECL loaderdrv_SetParent( HWND hwnd, HWND parent, HWND old_parent )
{
    load_driver()->pSetParent( hwnd, parent, old_parent );
}

static int CDECL loaderdrv_SetWindowRgn( HWND hwnd, HRGN hrgn, BOOL redraw )
{
    return load_driver()->pSetWindowRgn( hwnd, hrgn, redraw );
}

static void CDECL loaderdrv_SetWindowIcon( HWND hwnd, UINT type, HICON icon )
{
    load_driver()->pSetWindowIcon( hwnd, type, icon );
}

static void CDECL loaderdrv_SetWindowStyle( HWND hwnd, INT offset, STYLESTRUCT *style )
{
    load_driver()->pSetWindowStyle( hwnd, offset, style );
}

static void CDECL loaderdrv_SetWindowText( HWND hwnd, LPCWSTR text )
{
    load_driver()->pSetWindowText( hwnd, text );
}

static UINT CDECL loaderdrv_ShowWindow( HWND hwnd, INT cmd, RECT *rect, UINT swp )
{
    return load_driver()->pShowWindow( hwnd, cmd, rect, swp );
}

static LRESULT CDECL loaderdrv_SysCommand( HWND hwnd, WPARAM wparam, LPARAM lparam )
{
    return load_driver()->pSysCommand( hwnd, wparam, lparam );
}

static LRESULT CDECL loaderdrv_WindowMessage( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    return load_driver()->pWindowMessage( hwnd, msg, wparam, lparam );
}

static void CDECL loaderdrv_WindowPosChanging( HWND hwnd, HWND insert_after, UINT swp_flags,
                                               const RECT *window_rect, const RECT *client_rect,
                                               RECT *visible_rect )
{
    load_driver()->pWindowPosChanging( hwnd, insert_after, swp_flags,
                                       window_rect, client_rect, visible_rect );
}

static void CDECL loaderdrv_WindowPosChanged( HWND hwnd, HWND insert_after, UINT swp_flags,
                                              const RECT *window_rect, const RECT *client_rect,
                                              const RECT *visible_rect, const RECT *valid_rects )
{
    load_driver()->pWindowPosChanged( hwnd, insert_after, swp_flags, window_rect,
                                      client_rect, visible_rect, valid_rects );
}

static USER_DRIVER lazy_load_driver =
{
    /* keyboard functions */
    loaderdrv_ActivateKeyboardLayout,
    loaderdrv_Beep,
    loaderdrv_GetAsyncKeyState,
    loaderdrv_GetKeyNameText,
    loaderdrv_GetKeyboardLayout,
    loaderdrv_GetKeyboardLayoutName,
    loaderdrv_LoadKeyboardLayout,
    loaderdrv_MapVirtualKeyEx,
    loaderdrv_SendInput,
    loaderdrv_ToUnicodeEx,
    loaderdrv_UnloadKeyboardLayout,
    loaderdrv_VkKeyScanEx,
    /* mouse functions */
    loaderdrv_SetCursor,
    loaderdrv_GetCursorPos,
    loaderdrv_SetCursorPos,
    loaderdrv_ClipCursor,
    /* screen saver functions */
    loaderdrv_GetScreenSaveActive,
    loaderdrv_SetScreenSaveActive,
    /* clipboard functions */
    loaderdrv_AcquireClipboard,
    loaderdrv_CountClipboardFormats,
    loaderdrv_EmptyClipboard,
    loaderdrv_EndClipboardUpdate,
    loaderdrv_EnumClipboardFormats,
    loaderdrv_GetClipboardData,
    loaderdrv_GetClipboardFormatName,
    loaderdrv_IsClipboardFormatAvailable,
    loaderdrv_RegisterClipboardFormat,
    loaderdrv_SetClipboardData,
    /* display modes */
    loaderdrv_ChangeDisplaySettingsEx,
    loaderdrv_EnumDisplayMonitors,
    loaderdrv_EnumDisplaySettingsEx,
    loaderdrv_GetMonitorInfo,
    /* windowing functions */
    loaderdrv_CreateDesktopWindow,
    loaderdrv_CreateWindow,
    loaderdrv_DestroyWindow,
    loaderdrv_GetDC,
    loaderdrv_MsgWaitForMultipleObjectsEx,
    loaderdrv_ReleaseDC,
    loaderdrv_ScrollDC,
    loaderdrv_SetCapture,
    loaderdrv_SetFocus,
    loaderdrv_SetLayeredWindowAttributes,
    loaderdrv_SetParent,
    loaderdrv_SetWindowRgn,
    loaderdrv_SetWindowIcon,
    loaderdrv_SetWindowStyle,
    loaderdrv_SetWindowText,
    loaderdrv_ShowWindow,
    loaderdrv_SysCommand,
    loaderdrv_WindowMessage,
    loaderdrv_WindowPosChanging,
    loaderdrv_WindowPosChanged
};
