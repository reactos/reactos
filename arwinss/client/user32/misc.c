/*
 * Misc USER functions
 *
 * Copyright 1995 Thomas Sandford
 * Copyright 1997 Marcus Meissner
 * Copyright 1998 Turchanov Sergey
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

#include "windef.h"
#include "wine/windef16.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "wine/winternl.h"
#include "user_private.h"

#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(win);

/* USER signal proc flags and codes */
/* See UserSignalProc for comments */
#define USIG_FLAGS_WIN32          0x0001
#define USIG_FLAGS_GUI            0x0002
#define USIG_FLAGS_FEEDBACK       0x0004
#define USIG_FLAGS_FAULT          0x0008

#define USIG_DLL_UNLOAD_WIN16     0x0001
#define USIG_DLL_UNLOAD_WIN32     0x0002
#define USIG_FAULT_DIALOG_PUSH    0x0003
#define USIG_FAULT_DIALOG_POP     0x0004
#define USIG_DLL_UNLOAD_ORPHANS   0x0005
#define USIG_THREAD_INIT          0x0010
#define USIG_THREAD_EXIT          0x0020
#define USIG_PROCESS_CREATE       0x0100
#define USIG_PROCESS_INIT         0x0200
#define USIG_PROCESS_EXIT         0x0300
#define USIG_PROCESS_DESTROY      0x0400
#define USIG_PROCESS_RUNNING      0x0500
#define USIG_PROCESS_LOADED       0x0600

/***********************************************************************
 *		SignalProc32 (USER.391)
 *		UserSignalProc (USER32.@)
 *
 * The exact meaning of the USER signals is undocumented, but this
 * should cover the basic idea:
 *
 * USIG_DLL_UNLOAD_WIN16
 *     This is sent when a 16-bit module is unloaded.
 *
 * USIG_DLL_UNLOAD_WIN32
 *     This is sent when a 32-bit module is unloaded.
 *
 * USIG_DLL_UNLOAD_ORPHANS
 *     This is sent after the last Win3.1 module is unloaded,
 *     to allow removal of orphaned menus.
 *
 * USIG_FAULT_DIALOG_PUSH
 * USIG_FAULT_DIALOG_POP
 *     These are called to allow USER to prepare for displaying a
 *     fault dialog, even though the fault might have happened while
 *     inside a USER critical section.
 *
 * USIG_THREAD_INIT
 *     This is called from the context of a new thread, as soon as it
 *     has started to run.
 *
 * USIG_THREAD_EXIT
 *     This is called, still in its context, just before a thread is
 *     about to terminate.
 *
 * USIG_PROCESS_CREATE
 *     This is called, in the parent process context, after a new process
 *     has been created.
 *
 * USIG_PROCESS_INIT
 *     This is called in the new process context, just after the main thread
 *     has started execution (after the main thread's USIG_THREAD_INIT has
 *     been sent).
 *
 * USIG_PROCESS_LOADED
 *     This is called after the executable file has been loaded into the
 *     new process context.
 *
 * USIG_PROCESS_RUNNING
 *     This is called immediately before the main entry point is called.
 *
 * USIG_PROCESS_EXIT
 *     This is called in the context of a process that is about to
 *     terminate (but before the last thread's USIG_THREAD_EXIT has
 *     been sent).
 *
 * USIG_PROCESS_DESTROY
 *     This is called after a process has terminated.
 *
 *
 * The meaning of the dwFlags bits is as follows:
 *
 * USIG_FLAGS_WIN32
 *     Current process is 32-bit.
 *
 * USIG_FLAGS_GUI
 *     Current process is a (Win32) GUI process.
 *
 * USIG_FLAGS_FEEDBACK
 *     Current process needs 'feedback' (determined from the STARTUPINFO
 *     flags STARTF_FORCEONFEEDBACK / STARTF_FORCEOFFFEEDBACK).
 *
 * USIG_FLAGS_FAULT
 *     The signal is being sent due to a fault.
 */
WORD WINAPI UserSignalProc( UINT uCode, DWORD dwThreadOrProcessID,
                            DWORD dwFlags, HMODULE16 hModule )
{
    FIXME("(%04x, %08x, %04x, %04x)\n",
          uCode, dwThreadOrProcessID, dwFlags, hModule );
    /* FIXME: Should chain to GdiSignalProc now. */
    return 0;
}


/**********************************************************************
 * SetLastErrorEx [USER32.@]
 *
 * Sets the last-error code.
 *
 * RETURNS
 *    None.
 */
void WINAPI SetLastErrorEx(
    DWORD error, /* [in] Per-thread error code */
    DWORD type)  /* [in] Error type */
{
    TRACE("(0x%08x, 0x%08x)\n", error,type);
    switch(type) {
        case 0:
            break;
        case SLE_ERROR:
        case SLE_MINORERROR:
        case SLE_WARNING:
            /* Fall through for now */
        default:
            FIXME("(error=%08x, type=%08x): Unhandled type\n", error,type);
            break;
    }
    SetLastError( error );
}

/******************************************************************************
 * GetAltTabInfoA [USER32.@]
 */
BOOL WINAPI GetAltTabInfoA(HWND hwnd, int iItem, PALTTABINFO pati, LPSTR pszItemText, UINT cchItemText)
{
    FIXME("(%p, 0x%08x, %p, %p, 0x%08x)\n", hwnd, iItem, pati, pszItemText, cchItemText);
    return FALSE;
}

/******************************************************************************
 * GetAltTabInfoW [USER32.@]
 */
BOOL WINAPI GetAltTabInfoW(HWND hwnd, int iItem, PALTTABINFO pati, LPWSTR pszItemText, UINT cchItemText)
{
    FIXME("(%p, 0x%08x, %p, %p, 0x%08x)\n", hwnd, iItem, pati, pszItemText, cchItemText);
    return FALSE;
}

/******************************************************************************
 * SetDebugErrorLevel [USER32.@]
 * Sets the minimum error level for generating debugging events
 *
 * PARAMS
 *    dwLevel [I] Debugging error level
 *
 * RETURNS
 *    Nothing.
 */
VOID WINAPI SetDebugErrorLevel( DWORD dwLevel )
{
    FIXME("(%d): stub\n", dwLevel);
}


/***********************************************************************
 *		SetWindowStationUser (USER32.@)
 */
DWORD WINAPI SetWindowStationUser(DWORD x1,DWORD x2,DWORD x3,DWORD x4)
{
    FIXME("(0x%08x,0x%08x,0x%08x,0x%08x),stub!\n",x1,x2,x3,x4);
    return 1;
}

static const WCHAR primary_device_name[] = {'\\','\\','.','\\','D','I','S','P','L','A','Y','1',0};
static const WCHAR primary_device_string[] = {'X','1','1',' ','W','i','n','d','o','w','i','n','g',' ',
                                              'S','y','s','t','e','m',0};
static const WCHAR primary_device_deviceid[] = {'P','C','I','\\','V','E','N','_','0','0','0','0','&',
                                                'D','E','V','_','0','0','0','0',0};

/***********************************************************************
 *		EnumDisplayDevicesA (USER32.@)
 */
BOOL WINAPI EnumDisplayDevicesA( LPCSTR lpDevice, DWORD i, LPDISPLAY_DEVICEA lpDispDev,
                                 DWORD dwFlags )
{
    UNICODE_STRING deviceW;
    DISPLAY_DEVICEW ddW;
    BOOL ret;

    if(lpDevice)
        RtlCreateUnicodeStringFromAsciiz(&deviceW, lpDevice); 
    else
        deviceW.Buffer = NULL;

    ddW.cb = sizeof(ddW);
    ret = EnumDisplayDevicesW(deviceW.Buffer, i, &ddW, dwFlags);
    RtlFreeUnicodeString(&deviceW);

    if(!ret) return ret;

    WideCharToMultiByte(CP_ACP, 0, ddW.DeviceName, -1, lpDispDev->DeviceName, sizeof(lpDispDev->DeviceName), NULL, NULL);
    WideCharToMultiByte(CP_ACP, 0, ddW.DeviceString, -1, lpDispDev->DeviceString, sizeof(lpDispDev->DeviceString), NULL, NULL);
    lpDispDev->StateFlags = ddW.StateFlags;

    if(lpDispDev->cb >= offsetof(DISPLAY_DEVICEA, DeviceID) + sizeof(lpDispDev->DeviceID))
        WideCharToMultiByte(CP_ACP, 0, ddW.DeviceID, -1, lpDispDev->DeviceID, sizeof(lpDispDev->DeviceID), NULL, NULL);
    if(lpDispDev->cb >= offsetof(DISPLAY_DEVICEA, DeviceKey) + sizeof(lpDispDev->DeviceKey))
        WideCharToMultiByte(CP_ACP, 0, ddW.DeviceKey, -1, lpDispDev->DeviceKey, sizeof(lpDispDev->DeviceKey), NULL, NULL);

    return TRUE;
}

/***********************************************************************
 *		EnumDisplayDevicesW (USER32.@)
 */
BOOL WINAPI EnumDisplayDevicesW( LPCWSTR lpDevice, DWORD i, LPDISPLAY_DEVICEW lpDisplayDevice,
                                 DWORD dwFlags )
{
    FIXME("(%s,%d,%p,0x%08x), stub!\n",debugstr_w(lpDevice),i,lpDisplayDevice,dwFlags);

    if (i)
        return FALSE;

    memcpy(lpDisplayDevice->DeviceName, primary_device_name, sizeof(primary_device_name));
    memcpy(lpDisplayDevice->DeviceString, primary_device_string, sizeof(primary_device_string));
  
    lpDisplayDevice->StateFlags =
        DISPLAY_DEVICE_ATTACHED_TO_DESKTOP |
        DISPLAY_DEVICE_PRIMARY_DEVICE |
        DISPLAY_DEVICE_VGA_COMPATIBLE;

    if(lpDisplayDevice->cb >= offsetof(DISPLAY_DEVICEW, DeviceID) + sizeof(lpDisplayDevice->DeviceID))
        memcpy(lpDisplayDevice->DeviceID, primary_device_deviceid, sizeof(primary_device_deviceid));
    if(lpDisplayDevice->cb >= offsetof(DISPLAY_DEVICEW, DeviceKey) + sizeof(lpDisplayDevice->DeviceKey))
        lpDisplayDevice->DeviceKey[0] = 0;

    return TRUE;
}

struct monitor_enum_info
{
    RECT     rect;
    UINT     max_area;
    UINT     min_distance;
    HMONITOR primary;
    HMONITOR nearest;
    HMONITOR ret;
};

/* helper callback for MonitorFromRect */
static BOOL CALLBACK monitor_enum( HMONITOR monitor, HDC hdc, LPRECT rect, LPARAM lp )
{
    struct monitor_enum_info *info = (struct monitor_enum_info *)lp;
    RECT intersect;

    if (IntersectRect( &intersect, rect, &info->rect ))
    {
        /* check for larger intersecting area */
        UINT area = (intersect.right - intersect.left) * (intersect.bottom - intersect.top);
        if (area > info->max_area)
        {
            info->max_area = area;
            info->ret = monitor;
        }
    }
    else if (!info->max_area)  /* if not intersecting, check for min distance */
    {
        UINT distance;
        INT x, y;

        if (rect->left >= info->rect.right) x = info->rect.right - rect->left;
        else x = rect->right - info->rect.left;
        if (rect->top >= info->rect.bottom) y = info->rect.bottom - rect->top;
        else y = rect->bottom - info->rect.top;
        distance = x * x + y * y;
        if (distance < info->min_distance)
        {
            info->min_distance = distance;
            info->nearest = monitor;
        }
    }
    if (!info->primary)
    {
        MONITORINFO mon_info;
        mon_info.cbSize = sizeof(mon_info);
        GetMonitorInfoW( monitor, &mon_info );
        if (mon_info.dwFlags & MONITORINFOF_PRIMARY) info->primary = monitor;
    }
    return TRUE;
}

/***********************************************************************
 *		MonitorFromRect (USER32.@)
 */
HMONITOR WINAPI MonitorFromRect( LPCRECT rect, DWORD flags )
{
    struct monitor_enum_info info;

    /* make sure the desktop window exists */
    GetDesktopWindow();

    info.rect         = *rect;
    info.max_area     = 0;
    info.min_distance = ~0u;
    info.primary      = 0;
    info.nearest      = 0;
    info.ret          = 0;
    if (!EnumDisplayMonitors( 0, NULL, monitor_enum, (LPARAM)&info )) return 0;
    if (!info.ret)
    {
        if (flags & MONITOR_DEFAULTTOPRIMARY) info.ret = info.primary;
        else if (flags & MONITOR_DEFAULTTONEAREST) info.ret = info.nearest;
    }

    TRACE( "%s flags %x returning %p\n", wine_dbgstr_rect(rect), flags, info.ret );
    return info.ret;
}

/***********************************************************************
 *		MonitorFromPoint (USER32.@)
 */
HMONITOR WINAPI MonitorFromPoint( POINT pt, DWORD flags )
{
    RECT rect;

    SetRect( &rect, pt.x, pt.y, pt.x + 1, pt.y + 1 );
    return MonitorFromRect( &rect, flags );
}

/***********************************************************************
 *		MonitorFromWindow (USER32.@)
 */
HMONITOR WINAPI MonitorFromWindow(HWND hWnd, DWORD dwFlags)
{
    RECT rect;
    WINDOWPLACEMENT wp;

    TRACE("(%p, 0x%08x)\n", hWnd, dwFlags);

    if (IsIconic(hWnd) && GetWindowPlacement(hWnd, &wp))
        return MonitorFromRect( &wp.rcNormalPosition, dwFlags );

    if (GetWindowRect( hWnd, &rect ))
        return MonitorFromRect( &rect, dwFlags );

    if (!(dwFlags & (MONITOR_DEFAULTTOPRIMARY|MONITOR_DEFAULTTONEAREST))) return 0;
    /* retrieve the primary */
    SetRect( &rect, 0, 0, 1, 1 );
    return MonitorFromRect( &rect, dwFlags );
}

/***********************************************************************
 *		GetMonitorInfoA (USER32.@)
 */
BOOL WINAPI GetMonitorInfoA(HMONITOR hMonitor, LPMONITORINFO lpMonitorInfo)
{
    MONITORINFOEXW miW;
    MONITORINFOEXA *miA = (MONITORINFOEXA*)lpMonitorInfo;
    BOOL ret;

    miW.cbSize = sizeof(miW);

    ret = GetMonitorInfoW(hMonitor, (MONITORINFO*)&miW);
    if(!ret) return ret;

    miA->rcMonitor = miW.rcMonitor;
    miA->rcWork = miW.rcWork;
    miA->dwFlags = miW.dwFlags;
    if(miA->cbSize >= offsetof(MONITORINFOEXA, szDevice) + sizeof(miA->szDevice))
        WideCharToMultiByte(CP_ACP, 0, miW.szDevice, -1, miA->szDevice, sizeof(miA->szDevice), NULL, NULL);
    return ret;
}

/***********************************************************************
 *		GetMonitorInfoW (USER32.@)
 */
BOOL WINAPI GetMonitorInfoW(HMONITOR hMonitor, LPMONITORINFO lpMonitorInfo)
{
    BOOL ret = USER_Driver->pGetMonitorInfo( hMonitor, lpMonitorInfo );
    if (ret)
        TRACE("flags %04x, monitor %s, work %s\n", lpMonitorInfo->dwFlags,
              wine_dbgstr_rect(&lpMonitorInfo->rcMonitor),
              wine_dbgstr_rect(&lpMonitorInfo->rcWork));
    return ret;
}

/***********************************************************************
 *		EnumDisplayMonitors (USER32.@)
 */
BOOL WINAPI EnumDisplayMonitors( HDC hdc, LPCRECT rect, MONITORENUMPROC proc, LPARAM lp )
{
    return USER_Driver->pEnumDisplayMonitors( hdc, (LPRECT)rect, proc, lp );
}

/***********************************************************************
 *		RegisterSystemThread (USER32.@)
 */
void WINAPI RegisterSystemThread(DWORD flags, DWORD reserved)
{
    FIXME("(%08x, %08x)\n", flags, reserved);
}


/***********************************************************************
 *           RegisterTasklist   			[USER32.@]
 */
DWORD WINAPI RegisterTasklist (DWORD x)
{
    FIXME("0x%08x\n",x);
    return TRUE;
}


/***********************************************************************
 *		RegisterDeviceNotificationA (USER32.@)
 *
 * See RegisterDeviceNotificationW.
 */
HDEVNOTIFY WINAPI RegisterDeviceNotificationA(HANDLE hnd, LPVOID notifyfilter, DWORD flags)
{
    FIXME("(hwnd=%p, filter=%p,flags=0x%08x) returns a fake device notification handle!\n",
          hnd,notifyfilter,flags );
    return (HDEVNOTIFY) 0xcafecafe;
}

/***********************************************************************
 *		RegisterDeviceNotificationW (USER32.@)
 *
 * Registers a window with the system so that it will receive
 * notifications about a device.
 *
 * PARAMS
 *     hRecepient           [I] Window or service status handle that
 *                              will receive notifications.
 *     pNotificationFilter  [I] DEV_BROADCAST_HDR followed by some
 *                              type-specific data.
 *     dwFlags              [I] See notes
 *
 * RETURNS
 *
 * A handle to the device notification.
 *
 * NOTES
 *
 * The dwFlags parameter can be one of two values:
 *| DEVICE_NOTIFY_WINDOW_HANDLE  - hRecepient is a window handle
 *| DEVICE_NOTIFY_SERVICE_HANDLE - hRecepient is a service status handle
 */
HDEVNOTIFY WINAPI RegisterDeviceNotificationW(HANDLE hRecepient, LPVOID pNotificationFilter, DWORD dwFlags)
{
    FIXME("(hwnd=%p, filter=%p,flags=0x%08x) returns a fake device notification handle!\n",
          hRecepient,pNotificationFilter,dwFlags );
    return (HDEVNOTIFY) 0xcafeaffe;
}

/***********************************************************************
 *		UnregisterDeviceNotification (USER32.@)
 *
 */
BOOL  WINAPI UnregisterDeviceNotification(HDEVNOTIFY hnd)
{
    FIXME("(handle=%p), STUB!\n", hnd);
    return TRUE;
}

/***********************************************************************
 *           GetAppCompatFlags   (USER32.@)
 */
DWORD WINAPI GetAppCompatFlags( HTASK hTask )
{
    FIXME("(%p) stub\n", hTask);
    return 0;
}

/***********************************************************************
 *           GetAppCompatFlags2   (USER32.@)
 */
DWORD WINAPI GetAppCompatFlags2( HTASK hTask )
{
    FIXME("(%p) stub\n", hTask);
    return 0;
}


/***********************************************************************
 *           AlignRects   (USER32.@)
 */
BOOL WINAPI AlignRects(LPRECT rect, DWORD b, DWORD c, DWORD d)
{
    FIXME("(%p, %d, %d, %d): stub\n", rect, b, c, d);
    if (rect)
        FIXME("rect: [[%d, %d], [%d, %d]]\n", rect->left, rect->top, rect->right, rect->bottom);
    /* Calls OffsetRect */
    return FALSE;
}


/***********************************************************************
 *		LoadLocalFonts (USER32.@)
 */
VOID WINAPI LoadLocalFonts(VOID)
{
    /* are loaded. */
    return;
}


/***********************************************************************
 *		User32InitializeImmEntryTable
 */
BOOL WINAPI User32InitializeImmEntryTable(LPVOID ptr)
{
  FIXME("(%p): stub\n", ptr);
  return TRUE;
}

/**********************************************************************
 * WINNLSGetIMEHotkey [USER32.@]
 *
 */
UINT WINAPI WINNLSGetIMEHotkey(HWND hUnknown1)
{
    FIXME("hUnknown1 %p: stub!\n", hUnknown1);
    return 0; /* unknown */
}

/**********************************************************************
 * WINNLSEnableIME [USER32.@]
 *
 */
BOOL WINAPI WINNLSEnableIME(HWND hUnknown1, BOOL bUnknown2)
{
    FIXME("hUnknown1 %p bUnknown2 %d: stub!\n", hUnknown1, bUnknown2);
    return TRUE; /* success (?) */
}

/**********************************************************************
 * WINNLSGetEnableStatus [USER32.@]
 *
 */
BOOL WINAPI WINNLSGetEnableStatus(HWND hUnknown1)
{
    FIXME("hUnknown1 %p: stub!\n", hUnknown1);
    return TRUE; /* success (?) */
}

/**********************************************************************
 * SendIMEMessageExA [USER32.@]
 *
 */
LRESULT WINAPI SendIMEMessageExA(HWND p1, LPARAM p2)
{
  FIXME("(%p,%lx): stub\n", p1, p2);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/**********************************************************************
 * SendIMEMessageExW [USER32.@]
 *
 */
LRESULT WINAPI SendIMEMessageExW(HWND p1, LPARAM p2)
{
  FIXME("(%p,%lx): stub\n", p1, p2);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/**********************************************************************
 * DisableProcessWindowsGhosting [USER32.@]
 *
 */
VOID WINAPI DisableProcessWindowsGhosting(VOID)
{
  FIXME(": stub\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return;
}
