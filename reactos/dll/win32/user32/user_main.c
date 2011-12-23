/*
 * USER initialization code
 *
 * Copyright 2000 Alexandre Julliard
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
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"

#include "controls.h"
#include "user_private.h"
#include "win.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(graphics);

#define DESKTOP_ALL_ACCESS 0x01ff

HMODULE user32_module = 0;

static CRITICAL_SECTION user_section;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &user_section,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": user_section") }
};
static CRITICAL_SECTION user_section = { &critsect_debug, -1, 0, 0, 0, 0 };

static HPALETTE (WINAPI *pfnGDISelectPalette)( HDC hdc, HPALETTE hpal, WORD bkgnd );
static UINT (WINAPI *pfnGDIRealizePalette)( HDC hdc );
static HPALETTE hPrimaryPalette;

static DWORD exiting_thread_id;

extern void WDML_NotifyThreadDetach(void);


/***********************************************************************
 *           USER_Lock
 */
void USER_Lock(void)
{
    EnterCriticalSection( &user_section );
}


/***********************************************************************
 *           USER_Unlock
 */
void USER_Unlock(void)
{
    LeaveCriticalSection( &user_section );
}


/***********************************************************************
 *           USER_CheckNotLock
 *
 * Make sure that we don't hold the user lock.
 */
void USER_CheckNotLock(void)
{
    if (user_section.OwningThread == ULongToHandle(GetCurrentThreadId()) && user_section.RecursionCount)
    {
        ERR( "BUG: holding USER lock\n" );
        DebugBreak();
    }
}


/***********************************************************************
 *		UserSelectPalette (Not a Windows API)
 */
static HPALETTE WINAPI UserSelectPalette( HDC hDC, HPALETTE hPal, BOOL bForceBackground )
{
    WORD wBkgPalette = 1;

    if (!bForceBackground && (hPal != GetStockObject(DEFAULT_PALETTE)))
    {
        HWND hwnd = WindowFromDC( hDC );
        if (hwnd)
        {
            HWND hForeground = GetForegroundWindow();
            /* set primary palette if it's related to current active */
            if (hForeground == hwnd || IsChild(hForeground,hwnd))
            {
                wBkgPalette = 0;
                hPrimaryPalette = hPal;
            }
        }
    }
    return pfnGDISelectPalette( hDC, hPal, wBkgPalette);
}


/***********************************************************************
 *		UserRealizePalette (USER32.@)
 */
UINT WINAPI UserRealizePalette( HDC hDC )
{
    UINT realized = pfnGDIRealizePalette( hDC );

    /* do not send anything if no colors were changed */
    if (realized && GetCurrentObject( hDC, OBJ_PAL ) == hPrimaryPalette)
    {
        /* send palette change notification */
        HWND hWnd = WindowFromDC( hDC );
        if (hWnd) SendMessageTimeoutW( HWND_BROADCAST, WM_PALETTECHANGED, (WPARAM)hWnd, 0,
                                       SMTO_ABORTIFHUNG, 2000, NULL );
    }
    return realized;
}


/***********************************************************************
 *           palette_init
 *
 * Patch the function pointers in GDI for SelectPalette and RealizePalette
 */
static void palette_init(void)
{
    void **ptr;
    HMODULE module = GetModuleHandleA( "gdi32" );
    if (!module)
    {
        ERR( "cannot get GDI32 handle\n" );
        return;
    }
    if ((ptr = (void**)GetProcAddress( module, "pfnSelectPalette" )))
        pfnGDISelectPalette = InterlockedExchangePointer( ptr, UserSelectPalette );
    else ERR( "cannot find pfnSelectPalette in GDI32\n" );
    if ((ptr = (void**)GetProcAddress( module, "pfnRealizePalette" )))
        pfnGDIRealizePalette = InterlockedExchangePointer( ptr, UserRealizePalette );
    else ERR( "cannot find pfnRealizePalette in GDI32\n" );
}


/***********************************************************************
 *           get_default_desktop
 *
 * Get the name of the desktop to use for this app if not specified explicitly.
 */
static const WCHAR *get_default_desktop(void)
{
    static const WCHAR defaultW[] = {'D','e','f','a','u','l','t',0};
#if 0
    static const WCHAR desktopW[] = {'D','e','s','k','t','o','p',0};
#endif
    static const WCHAR explorerW[] = {'\\','E','x','p','l','o','r','e','r',0};
    static const WCHAR app_defaultsW[] = {'S','o','f','t','w','a','r','e','\\',
                                          'W','i','n','e','\\',
                                          'A','p','p','D','e','f','a','u','l','t','s',0};
    static WCHAR buffer[MAX_PATH + sizeof(explorerW)/sizeof(WCHAR)];
    WCHAR *p, *appname = buffer;
    const WCHAR *ret = defaultW;
    DWORD len;
#if 0
    HKEY tmpkey, appkey;
#endif

    len = (GetModuleFileNameW( 0, buffer, MAX_PATH ));
    if (!len || len >= MAX_PATH) return ret;
    if ((p = strrchrW( appname, '/' ))) appname = p + 1;
    if ((p = strrchrW( appname, '\\' ))) appname = p + 1;
    p = appname + strlenW(appname);
    strcpyW( p, explorerW );

    /* @@ Wine registry key: HKCU\Software\Wine\AppDefaults\app.exe\Explorer */
#if 0
    if (!RegOpenKeyW( HKEY_CURRENT_USER, app_defaultsW, &tmpkey ))
    {
        if (RegOpenKeyW( tmpkey, appname, &appkey )) appkey = 0;
        RegCloseKey( tmpkey );
        if (appkey)
        {
            len = sizeof(buffer);
            if (!RegQueryValueExW( appkey, desktopW, 0, NULL, (LPBYTE)buffer, &len )) ret = buffer;
            RegCloseKey( appkey );
            if (ret && strcmpiW( ret, defaultW )) return ret;
            ret = defaultW;
        }
    }
#endif
    memcpy( buffer, app_defaultsW, 13 * sizeof(WCHAR) );  /* copy only software\\wine */
    strcpyW( buffer + 13, explorerW );

    /* @@ Wine registry key: HKCU\Software\Wine\Explorer */
#if 0
    if (!RegOpenKeyW( HKEY_CURRENT_USER, buffer, &appkey ))
    {
        len = sizeof(buffer);
        if (!RegQueryValueExW( appkey, desktopW, 0, NULL, (LPBYTE)buffer, &len )) ret = buffer;
        RegCloseKey( appkey );
    }
#endif
    return ret;
}


/***********************************************************************
 *           winstation_init
 *
 * Connect to the process window station and desktop.
 */
static void winstation_init(void)
{
    static const WCHAR WinSta0[] = {'W','i','n','S','t','a','0',0};

    STARTUPINFOW info;
    WCHAR *winstation = NULL, *desktop = NULL, *buffer = NULL;
    HANDLE handle;

    GetStartupInfoW( &info );
    if (info.lpDesktop && *info.lpDesktop)
    {
        buffer = HeapAlloc( GetProcessHeap(), 0, (strlenW(info.lpDesktop) + 1) * sizeof(WCHAR) );
        strcpyW( buffer, info.lpDesktop );
        if ((desktop = strchrW( buffer, '\\' )))
        {
            *desktop++ = 0;
            winstation = buffer;
        }
        else desktop = buffer;
    }

    /* set winstation if explicitly specified, or if we don't have one yet */
    if (buffer || !GetProcessWindowStation())
    {
        handle = OpenWindowStationW( winstation ? winstation : WinSta0, FALSE, WINSTA_ALL_ACCESS );

        if (handle)
        {
            SetProcessWindowStation( handle );
            /* only WinSta0 is visible */
            if (!winstation || !strcmpiW( winstation, WinSta0 ))
            {
                USEROBJECTFLAGS flags;
                flags.fInherit  = FALSE;
                flags.fReserved = FALSE;
                flags.dwFlags   = WSF_VISIBLE;
                SetUserObjectInformationW( handle, UOI_FLAGS, &flags, sizeof(flags) );
            }
        }
    }
    if (buffer || !GetThreadDesktop( GetCurrentThreadId() ))
    {
        handle = OpenDesktopW(desktop ? desktop : get_default_desktop(),
                            0, FALSE, DESKTOP_ALL_ACCESS);

        if (handle) SetThreadDesktop( handle );
    }
    HeapFree( GetProcessHeap(), 0, buffer );
}


/***********************************************************************
 *           USER initialisation routine
 */
static BOOL process_attach(void)
{
    winstation_init();

    /* Initialize system colors and metrics */
    SYSPARAMS_Init();

    /* Setup palette function pointers */
    palette_init();

    /* Initialize built-in window classes */
    CLASS_RegisterBuiltinClasses();

    /* Initialize message spying */
    if (!SPY_Init()) return FALSE;

    return TRUE;
}


/**********************************************************************
 *           USER_IsExitingThread
 */
BOOL USER_IsExitingThread( DWORD tid )
{
    return (tid == exiting_thread_id);
}


/**********************************************************************
 *           thread_detach
 */
static void thread_detach(void)
{
    struct user_thread_info *thread_info = get_user_thread_info();

    exiting_thread_id = GetCurrentThreadId();

    WDML_NotifyThreadDetach();

    if (thread_info->top_window) WIN_DestroyThreadWindows( thread_info->top_window );
    if (thread_info->msg_window) WIN_DestroyThreadWindows( thread_info->msg_window );
    CloseHandle( thread_info->server_queue );
    HeapFree( GetProcessHeap(), 0, thread_info->wmchar_data );

    exiting_thread_id = 0;
}


/***********************************************************************
 *           UserClientDllInitialize  (USER32.@)
 *
 * USER dll initialisation routine (exported as UserClientDllInitialize for compatibility).
 */
BOOL WINAPI DllMain( HINSTANCE inst, DWORD reason, LPVOID reserved )
{
    BOOL ret = TRUE;
    switch(reason)
    {
    case DLL_PROCESS_ATTACH:
        user32_module = inst;
        ret = process_attach();
        break;
    case DLL_THREAD_DETACH:
        thread_detach();
        break;
    case DLL_PROCESS_DETACH:
        USER_unload_driver();
        break;
    }
    return ret;
}


/***********************************************************************
 *		LockWorkStation (USER32.@)
 */
BOOL WINAPI LockWorkStation(void)
{
    TRACE(": stub\n");
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}


BOOL WINAPI ClientThreadSetup()
{
    TRACE(": stub\n");
    return TRUE;
}

BOOL WINAPI UnregisterUserApiHook(VOID)
{
    return TRUE;
}
