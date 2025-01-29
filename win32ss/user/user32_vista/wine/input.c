/*
 * USER Input processing
 *
 * Copyright 1993 Bob Amstadt
 * Copyright 1996 Albrecht Kleine
 * Copyright 1997 David Faure
 * Copyright 1998 Morten Welinder
 * Copyright 1998 Ulrich Weigand
 * Copyright 2012 Henri Verbeet
 * Copyright 2018 Zebediah Figura for CodeWeavers
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

#include "user_private.h"
#include "dbt.h"
#include "wine/debug.h"
#include "wine/plugplay.h"

WINE_DEFAULT_DEBUG_CHANNEL(win);
WINE_DECLARE_DEBUG_CHANNEL(keyboard);

/***********************************************************************
 *           get_locale_kbd_layout
 */
static HKL get_locale_kbd_layout(void)
{
    ULONG_PTR layout;

    /* FIXME:
     *
     * layout = main_key_tab[kbd_layout].lcid;
     *
     * Winword uses return value of GetKeyboardLayout as a codepage
     * to translate ANSI keyboard messages to unicode. But we have
     * a problem with it: for instance Polish keyboard layout is
     * identical to the US one, and therefore instead of the Polish
     * locale id we return the US one.
     */

    layout = GetUserDefaultLCID();
    layout = MAKELONG( layout, layout );
    return (HKL)layout;
}


/***********************************************************************
 *		keybd_event (USER32.@)
 */
void WINAPI keybd_event( BYTE bVk, BYTE bScan,
                         DWORD dwFlags, ULONG_PTR dwExtraInfo )
{
    INPUT input;

    input.type = INPUT_KEYBOARD;
    input.ki.wVk = bVk;
    input.ki.wScan = bScan;
    input.ki.dwFlags = dwFlags;
    input.ki.time = 0;
    input.ki.dwExtraInfo = dwExtraInfo;
    NtUserSendInput( 1, &input, sizeof(input) );
}


/***********************************************************************
 *		mouse_event (USER32.@)
 */
void WINAPI mouse_event( DWORD dwFlags, DWORD dx, DWORD dy,
                         DWORD dwData, ULONG_PTR dwExtraInfo )
{
    INPUT input;

    input.type = INPUT_MOUSE;
    input.mi.dx = dx;
    input.mi.dy = dy;
    input.mi.mouseData = dwData;
    input.mi.dwFlags = dwFlags;
    input.mi.time = 0;
    input.mi.dwExtraInfo = dwExtraInfo;
    NtUserSendInput( 1, &input, sizeof(input) );
}


/***********************************************************************
 *		GetCursorPos (USER32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetCursorPos( POINT *pt )
{
    return NtUserGetCursorPos( pt );
}


/**********************************************************************
 *		ReleaseCapture (USER32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH ReleaseCapture(void)
{
    return NtUserReleaseCapture();
}


/**********************************************************************
 *		GetCapture (USER32.@)
 */
HWND WINAPI GetCapture(void)
{
    GUITHREADINFO info;
    info.cbSize = sizeof(info);
    return NtUserGetGUIThreadInfo( GetCurrentThreadId(), &info ) ? info.hwndCapture : 0;
}


/*****************************************************************
 *		DestroyCaret (USER32.@)
 */
BOOL WINAPI DestroyCaret(void)
{
    return NtUserDestroyCaret();
}


/*****************************************************************
 *		SetCaretPos (USER32.@)
 */
BOOL WINAPI SetCaretPos( int x, int y )
{
    return NtUserSetCaretPos( x, y );
}


/*****************************************************************
 *		SetCaretBlinkTime (USER32.@)
 */
BOOL WINAPI SetCaretBlinkTime( unsigned int time )
{
    return NtUserSetCaretBlinkTime( time );
}


/***********************************************************************
 *		GetInputState   (USER32.@)
 */
BOOL WINAPI GetInputState(void)
{
    return NtUserGetInputState();
}


/******************************************************************
 *              GetLastInputInfo (USER32.@)
 */
BOOL WINAPI GetLastInputInfo(PLASTINPUTINFO plii)
{
    TRACE("%p\n", plii);

    if (plii->cbSize != sizeof (*plii) )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    plii->dwTime = NtUserGetLastInputTime();
    return TRUE;
}


/**********************************************************************
 *		VkKeyScanA (USER32.@)
 *
 * VkKeyScan translates an ANSI character to a virtual-key and shift code
 * for the current keyboard.
 * high-order byte yields :
 *	0	Unshifted
 *	1	Shift
 *	2	Ctrl
 *	3-5	Shift-key combinations that are not used for characters
 *	6	Ctrl-Alt
 *	7	Ctrl-Alt-Shift
 *	I.e. :	Shift = 1, Ctrl = 2, Alt = 4.
 * FIXME : works ok except for dead chars :
 * VkKeyScan '^'(0x5e, 94) ... got keycode 00 ... returning 00
 * VkKeyScan '`'(0x60, 96) ... got keycode 00 ... returning 00
 */
SHORT WINAPI VkKeyScanA(CHAR cChar)
{
    WCHAR wChar;

    if (IsDBCSLeadByte(cChar)) return -1;

    MultiByteToWideChar(CP_ACP, 0, &cChar, 1, &wChar, 1);
    return VkKeyScanW(wChar);
}

/******************************************************************************
 *		VkKeyScanW (USER32.@)
 */
SHORT WINAPI VkKeyScanW(WCHAR cChar)
{
    return NtUserVkKeyScanEx( cChar, NtUserGetKeyboardLayout(0) );
}

/**********************************************************************
 *		VkKeyScanExA (USER32.@)
 */
WORD WINAPI VkKeyScanExA(CHAR cChar, HKL dwhkl)
{
    WCHAR wChar;

    if (IsDBCSLeadByte(cChar)) return -1;

    MultiByteToWideChar(CP_ACP, 0, &cChar, 1, &wChar, 1);
    return NtUserVkKeyScanEx( wChar, dwhkl );
}

/**********************************************************************
 *		OemKeyScan (USER32.@)
 */
DWORD WINAPI OemKeyScan( WORD oem )
{
    WCHAR wchr;
    DWORD vkey, scan;
    char oem_char = LOBYTE( oem );

    if (!OemToCharBuffW( &oem_char, &wchr, 1 ))
        return -1;

    vkey = VkKeyScanW( wchr );
    scan = MapVirtualKeyW( LOBYTE( vkey ), MAPVK_VK_TO_VSC );
    if (!scan) return -1;

    vkey &= 0xff00;
    vkey <<= 8;
    return vkey | scan;
}

/******************************************************************************
 *		GetKeyboardType (USER32.@)
 */
INT WINAPI GetKeyboardType(INT nTypeFlag)
{
    TRACE_(keyboard)("(%d)\n", nTypeFlag);
    if (LOWORD(NtUserGetKeyboardLayout(0)) == MAKELANGID(LANG_JAPANESE, SUBLANG_JAPANESE_JAPAN))
    {
        /* scan code for `_', the key left of r-shift, in Japanese 106 keyboard */
        const UINT JP106_VSC_USCORE = 0x73;

        switch(nTypeFlag)
        {
        case 0:      /* Keyboard type */
            return 7;    /* Japanese keyboard */
        case 1:      /* Keyboard Subtype */
            /* Test keyboard mappings to detect Japanese keyboard */
            if (MapVirtualKeyW(VK_OEM_102, MAPVK_VK_TO_VSC) == JP106_VSC_USCORE
                && MapVirtualKeyW(JP106_VSC_USCORE, MAPVK_VSC_TO_VK) == VK_OEM_102)
                return 2;    /* Japanese 106 */
            else
                return 0;    /* AT-101 */
        case 2:      /* Number of F-keys */
            return 12;   /* It has 12 F-keys */
        }
    }
    else
    {
        switch(nTypeFlag)
        {
        case 0:      /* Keyboard type */
            return 4;    /* AT-101 */
        case 1:      /* Keyboard Subtype */
            return 0;    /* There are no defined subtypes */
        case 2:      /* Number of F-keys */
            return 12;   /* We're doing an 101 for now, so return 12 F-keys */
        }
    }
    WARN_(keyboard)("Unknown type\n");
    return 0;    /* The book says 0 here, so 0 */
}

/******************************************************************************
 *		MapVirtualKeyA (USER32.@)
 */
UINT WINAPI MapVirtualKeyA(UINT code, UINT maptype)
{
    return MapVirtualKeyExA( code, maptype, NtUserGetKeyboardLayout(0) );
}

/******************************************************************************
 *		MapVirtualKeyW (USER32.@)
 */
UINT WINAPI MapVirtualKeyW(UINT code, UINT maptype)
{
    return NtUserMapVirtualKeyEx( code, maptype, NtUserGetKeyboardLayout(0) );
}

/******************************************************************************
 *		MapVirtualKeyExA (USER32.@)
 */
UINT WINAPI MapVirtualKeyExA(UINT code, UINT maptype, HKL hkl)
{
    UINT ret;

    ret = NtUserMapVirtualKeyEx( code, maptype, hkl );
    if (maptype == MAPVK_VK_TO_CHAR)
    {
        BYTE ch = 0;
        WCHAR wch = ret;

        WideCharToMultiByte( CP_ACP, 0, &wch, 1, (LPSTR)&ch, 1, NULL, NULL );
        ret = ch;
    }
    return ret;
}

/****************************************************************************
 *		GetKBCodePage (USER32.@)
 */
UINT WINAPI GetKBCodePage(void)
{
    return GetOEMCP();
}

/****************************************************************************
 *		GetKeyboardLayoutNameA (USER32.@)
 */
BOOL WINAPI GetKeyboardLayoutNameA(LPSTR pszKLID)
{
    WCHAR buf[KL_NAMELENGTH];

    if (NtUserGetKeyboardLayoutName( buf ))
        return WideCharToMultiByte( CP_ACP, 0, buf, -1, pszKLID, KL_NAMELENGTH, NULL, NULL ) != 0;
    return FALSE;
}

/****************************************************************************
 *		GetKeyNameTextA (USER32.@)
 */
INT WINAPI GetKeyNameTextA(LONG lParam, LPSTR lpBuffer, INT nSize)
{
    WCHAR buf[256];
    INT ret;

    if (!nSize || !NtUserGetKeyNameText( lParam, buf, ARRAYSIZE(buf) ))
    {
        lpBuffer[0] = 0;
        return 0;
    }
    ret = WideCharToMultiByte(CP_ACP, 0, buf, -1, lpBuffer, nSize, NULL, NULL);
    if (!ret && nSize)
    {
        ret = nSize - 1;
        lpBuffer[ret] = 0;
    }
    else ret--;

    return ret;
}

/****************************************************************************
 *		ToUnicode (USER32.@)
 */
INT WINAPI ToUnicode( UINT virt, UINT scan, const BYTE *state, LPWSTR str, int size, UINT flags )
{
    return NtUserToUnicodeEx( virt, scan, state, str, size, flags, NtUserGetKeyboardLayout(0) );
}

/****************************************************************************
 *		ToAscii (USER32.@)
 */
INT WINAPI ToAscii( UINT virtKey, UINT scanCode, const BYTE *lpKeyState,
                    LPWORD lpChar, UINT flags )
{
    return ToAsciiEx( virtKey, scanCode, lpKeyState, lpChar, flags,
                      NtUserGetKeyboardLayout(0) );
}

/****************************************************************************
 *		ToAsciiEx (USER32.@)
 */
INT WINAPI ToAsciiEx( UINT virtKey, UINT scanCode, const BYTE *lpKeyState,
                      LPWORD lpChar, UINT flags, HKL dwhkl )
{
    WCHAR uni_chars[2];
    INT ret, n_ret;

    ret = NtUserToUnicodeEx( virtKey, scanCode, lpKeyState, uni_chars, 2, flags, dwhkl );
    if (ret < 0) n_ret = 1; /* FIXME: make ToUnicode return 2 for dead chars */
    else n_ret = ret;
    WideCharToMultiByte(CP_ACP, 0, uni_chars, n_ret, (LPSTR)lpChar, 2, NULL, NULL);
    return ret;
}

/**********************************************************************
 *		BlockInput (USER32.@)
 */
BOOL WINAPI BlockInput(BOOL fBlockIt)
{
    FIXME_(keyboard)("(%d): stub\n", fBlockIt);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return FALSE;
}


/***********************************************************************
 *		LoadKeyboardLayoutW (USER32.@)
 */
HKL WINAPI LoadKeyboardLayoutW( const WCHAR *name, UINT flags )
{
    WCHAR layout_path[MAX_PATH], value[5];
    LCID locale = GetUserDefaultLCID();
    DWORD id, value_size, tmp;
    HKEY hkey;
    HKL layout;

    FIXME_(keyboard)( "name %s, flags %x, semi-stub!\n", debugstr_w( name ), flags );

    tmp = wcstoul( name, NULL, 16 );
    if (HIWORD( tmp )) layout = UlongToHandle( tmp );
    else layout = UlongToHandle( MAKELONG( LOWORD( tmp ), LOWORD( tmp ) ) );

    if (!((UINT_PTR)layout >> 28)) id = LOWORD( tmp );
    else id = HIWORD( layout ); /* IME or aliased layout */

    wcscpy( layout_path, L"System\\CurrentControlSet\\Control\\Keyboard Layouts\\" );
    wcscat( layout_path, name );

    if (!RegOpenKeyW( HKEY_LOCAL_MACHINE, layout_path, &hkey ))
    {
        value_size = sizeof(value);
        if (!RegGetValueW( hkey, NULL, L"Layout Id", RRF_RT_REG_SZ, NULL, (void *)&value, &value_size ))
            id = 0xf000 | (wcstoul( value, NULL, 16 ) & 0xfff);

        RegCloseKey( hkey );
    }

    layout = UlongToHandle( MAKELONG( locale, id ) );
    if ((flags & KLF_ACTIVATE) && NtUserActivateKeyboardLayout( layout, 0 )) return layout;

    /* FIXME: semi-stub: returning default layout */
    return get_locale_kbd_layout();
}

/***********************************************************************
 *		LoadKeyboardLayoutA (USER32.@)
 */
HKL WINAPI LoadKeyboardLayoutA(LPCSTR pwszKLID, UINT Flags)
{
    HKL ret;
    UNICODE_STRING pwszKLIDW;

    if (pwszKLID) RtlCreateUnicodeStringFromAsciiz(&pwszKLIDW, pwszKLID);
    else pwszKLIDW.Buffer = NULL;

    ret = LoadKeyboardLayoutW(pwszKLIDW.Buffer, Flags);
    RtlFreeUnicodeString(&pwszKLIDW);
    return ret;
}


/***********************************************************************
 *		LoadKeyboardLayoutEx (USER32.@)
 */
HKL WINAPI LoadKeyboardLayoutEx( HKL layout, const WCHAR *name, UINT flags )
{
    FIXME_(keyboard)( "layout %p, name %s, flags %x, semi-stub!\n", layout, debugstr_w( name ), flags );

    if (!layout) return NULL;
    return LoadKeyboardLayoutW( name, flags );
}

/***********************************************************************
 *		UnloadKeyboardLayout (USER32.@)
 */
BOOL WINAPI UnloadKeyboardLayout( HKL layout )
{
    FIXME_(keyboard)( "layout %p, stub!\n", layout );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}


static DWORD CALLBACK devnotify_window_callbackW(HANDLE handle, DWORD flags, DEV_BROADCAST_HDR *header)
{
    SendMessageTimeoutW(handle, WM_DEVICECHANGE, flags, (LPARAM)header, SMTO_ABORTIFHUNG, 2000, NULL);
    return 0;
}

static DWORD CALLBACK devnotify_window_callbackA(HANDLE handle, DWORD flags, DEV_BROADCAST_HDR *header)
{
    if (flags & 0x8000)
    {
        switch (header->dbch_devicetype)
        {
        case DBT_DEVTYP_DEVICEINTERFACE:
        {
            const DEV_BROADCAST_DEVICEINTERFACE_W *ifaceW = (const DEV_BROADCAST_DEVICEINTERFACE_W *)header;
            size_t lenW = wcslen( ifaceW->dbcc_name );
            DEV_BROADCAST_DEVICEINTERFACE_A *ifaceA;
            DWORD lenA;

            if (!(ifaceA = malloc( offsetof(DEV_BROADCAST_DEVICEINTERFACE_A, dbcc_name[lenW * 3 + 1]) )))
                return 0;
            lenA = WideCharToMultiByte( CP_ACP, 0, ifaceW->dbcc_name, lenW + 1,
                                        ifaceA->dbcc_name, lenW * 3 + 1, NULL, NULL );

            ifaceA->dbcc_size = offsetof(DEV_BROADCAST_DEVICEINTERFACE_A, dbcc_name[lenA + 1]);
            ifaceA->dbcc_devicetype = ifaceW->dbcc_devicetype;
            ifaceA->dbcc_reserved = ifaceW->dbcc_reserved;
            ifaceA->dbcc_classguid = ifaceW->dbcc_classguid;
            SendMessageTimeoutA( handle, WM_DEVICECHANGE, flags, (LPARAM)ifaceA, SMTO_ABORTIFHUNG, 2000, NULL );
            free( ifaceA );
            return 0;
        }

        case DBT_DEVTYP_HANDLE:
        {
            const DEV_BROADCAST_HANDLE *handleW = (const DEV_BROADCAST_HANDLE *)header;
            UINT sizeW = handleW->dbch_size - offsetof(DEV_BROADCAST_HANDLE, dbch_data[0]), len, offset;
            DEV_BROADCAST_HANDLE *handleA;

            if (!(handleA = malloc( offsetof(DEV_BROADCAST_HANDLE, dbch_data[sizeW * 3 + 1]) ))) return 0;
            memcpy( handleA, handleW, offsetof(DEV_BROADCAST_HANDLE, dbch_data[0]) );
            offset = min( sizeW, handleW->dbch_nameoffset );

            memcpy( handleA->dbch_data, handleW->dbch_data, offset );
            len = WideCharToMultiByte( CP_ACP, 0, (WCHAR *)(handleW->dbch_data + offset), (sizeW - offset) / sizeof(WCHAR),
                                       (char *)handleA->dbch_data + offset, sizeW * 3 + 1 - offset, NULL, NULL );
            handleA->dbch_size = offsetof(DEV_BROADCAST_HANDLE, dbch_data[offset + len + 1]);

            SendMessageTimeoutA( handle, WM_DEVICECHANGE, flags, (LPARAM)handleA, SMTO_ABORTIFHUNG, 2000, NULL );
            free( handleA );
            return 0;
        }

        case DBT_DEVTYP_OEM:
            break;
        default:
            FIXME( "unimplemented W to A mapping for %#lx\n", header->dbch_devicetype );
        }
    }

    SendMessageTimeoutA( handle, WM_DEVICECHANGE, flags, (LPARAM)header, SMTO_ABORTIFHUNG, 2000, NULL );
    return 0;
}

static DWORD CALLBACK devnotify_service_callback(HANDLE handle, DWORD flags, DEV_BROADCAST_HDR *header)
{
    FIXME("Support for service handles is not yet implemented!\n");
    return 0;
}

/***********************************************************************
 *		RegisterDeviceNotificationA (USER32.@)
 *
 * See RegisterDeviceNotificationW.
 */
HDEVNOTIFY WINAPI RegisterDeviceNotificationA( HANDLE handle, void *filter, DWORD flags )
{
    return RegisterDeviceNotificationW( handle, filter, flags );
}

/***********************************************************************
 *		RegisterDeviceNotificationW (USER32.@)
 */
HDEVNOTIFY WINAPI RegisterDeviceNotificationW( HANDLE handle, void *filter, DWORD flags )
{
    DEV_BROADCAST_HDR *header = filter;
    device_notify_callback callback;

    TRACE("handle %p, filter %p, flags %#lx\n", handle, filter, flags);

    if (flags & ~(DEVICE_NOTIFY_SERVICE_HANDLE | DEVICE_NOTIFY_ALL_INTERFACE_CLASSES))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }

    if (!(flags & DEVICE_NOTIFY_SERVICE_HANDLE) && !IsWindow( handle ))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }

    if (flags & DEVICE_NOTIFY_SERVICE_HANDLE)
        callback = devnotify_service_callback;
    else if (IsWindowUnicode( handle ))
        callback = devnotify_window_callbackW;
    else
        callback = devnotify_window_callbackA;

    if (!header)
    {
        DEV_BROADCAST_HDR dummy = {0};
        return I_ScRegisterDeviceNotification( handle, &dummy, callback );
    }
    if (header->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE)
    {
        DEV_BROADCAST_DEVICEINTERFACE_W iface = *(DEV_BROADCAST_DEVICEINTERFACE_W *)header;

        if (flags & DEVICE_NOTIFY_ALL_INTERFACE_CLASSES)
            iface.dbcc_size = offsetof( DEV_BROADCAST_DEVICEINTERFACE_W, dbcc_classguid );
        else
            iface.dbcc_size = offsetof( DEV_BROADCAST_DEVICEINTERFACE_W, dbcc_name );

        return I_ScRegisterDeviceNotification( handle, (DEV_BROADCAST_HDR *)&iface, callback );
    }
    if (header->dbch_devicetype == DBT_DEVTYP_HANDLE)
    {
        FIXME( "DBT_DEVTYP_HANDLE not implemented\n" );
        return I_ScRegisterDeviceNotification( handle, header, callback );
    }

    FIXME( "type %#lx not implemented\n", header->dbch_devicetype );
    SetLastError( ERROR_INVALID_DATA );
    return NULL;
}

/***********************************************************************
 *		UnregisterDeviceNotification (USER32.@)
 */
BOOL WINAPI UnregisterDeviceNotification( HDEVNOTIFY handle )
{
    TRACE("%p\n", handle);

    return I_ScUnregisterDeviceNotification( handle );
}

/***********************************************************************
 *              GetRawInputDeviceInfoA   (USER32.@)
 */
UINT WINAPI GetRawInputDeviceInfoA( HANDLE device, UINT command, void *data, UINT *size )
{
    TRACE( "device %p, command %#x, data %p, size %p.\n", device, command, data, size );

    /* RIDI_DEVICENAME size is in chars, not bytes */
    if (command == RIDI_DEVICENAME)
    {
        WCHAR *nameW;
        UINT ret, sizeW;

        if (!size) return ~0U;

        sizeW = *size;

        if (data && sizeW > 0)
            nameW = HeapAlloc( GetProcessHeap(), 0, sizeof(WCHAR) * sizeW );
        else
            nameW = NULL;

        ret = NtUserGetRawInputDeviceInfo( device, command, nameW, &sizeW );

        if (ret && ret != ~0U)
            WideCharToMultiByte( CP_ACP, 0, nameW, -1, data, *size, NULL, NULL );

        *size = sizeW;

        HeapFree( GetProcessHeap(), 0, nameW );

        return ret;
    }

    return NtUserGetRawInputDeviceInfo( device, command, data, size );
}

/***********************************************************************
 *              DefRawInputProc   (USER32.@)
 */
LRESULT WINAPI DefRawInputProc( RAWINPUT **data, INT data_count, UINT header_size )
{
    TRACE( "data %p, data_count %d, header_size %u.\n", data, data_count, header_size );

    return header_size == sizeof(RAWINPUTHEADER) ? 0 : -1;
}

/*****************************************************************************
 * CloseTouchInputHandle (USER32.@)
 */
BOOL WINAPI CloseTouchInputHandle( HTOUCHINPUT handle )
{
    FIXME( "handle %p stub!\n", handle );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/*****************************************************************************
 * GetTouchInputInfo (USER32.@)
 */
BOOL WINAPI GetTouchInputInfo( HTOUCHINPUT handle, UINT count, TOUCHINPUT *ptr, int size )
{
    FIXME( "handle %p, count %u, ptr %p, size %u stub!\n", handle, count, ptr, size );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/**********************************************************************
 * IsTouchWindow (USER32.@)
 */
BOOL WINAPI IsTouchWindow( HWND hwnd, ULONG *flags )
{
    FIXME( "hwnd %p, flags %p stub!\n", hwnd, flags );
    return FALSE;
}

/*****************************************************************************
 * RegisterTouchWindow (USER32.@)
 */
BOOL WINAPI RegisterTouchWindow( HWND hwnd, ULONG flags )
{
    FIXME( "hwnd %p, flags %#lx stub!\n", hwnd, flags );
    return TRUE;
}

/*****************************************************************************
 * UnregisterTouchWindow (USER32.@)
 */
BOOL WINAPI UnregisterTouchWindow( HWND hwnd )
{
    FIXME( "hwnd %p stub!\n", hwnd );
    return TRUE;
}

/*****************************************************************************
 * GetGestureInfo (USER32.@)
 */
BOOL WINAPI CloseGestureInfoHandle( HGESTUREINFO handle )
{
    FIXME( "handle %p stub!\n", handle );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/*****************************************************************************
 * GetGestureInfo (USER32.@)
 */
BOOL WINAPI GetGestureExtraArgs( HGESTUREINFO handle, UINT count, BYTE *args )
{
    FIXME( "handle %p, count %u, args %p stub!\n", handle, count, args );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/*****************************************************************************
 * GetGestureInfo (USER32.@)
 */
BOOL WINAPI GetGestureInfo( HGESTUREINFO handle, GESTUREINFO *ptr )
{
    FIXME( "handle %p, ptr %p stub!\n", handle, ptr );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/*****************************************************************************
 * GetGestureConfig (USER32.@)
 */
BOOL WINAPI GetGestureConfig( HWND hwnd, DWORD reserved, DWORD flags, UINT *count,
                              GESTURECONFIG *config, UINT size )
{
    FIXME( "handle %p, reserved %#lx, flags %#lx, count %p, config %p, size %u stub!\n",
           hwnd, reserved, flags, count, config, size );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/**********************************************************************
 * SetGestureConfig (USER32.@)
 */
BOOL WINAPI SetGestureConfig( HWND hwnd, DWORD reserved, UINT count,
                              GESTURECONFIG *config, UINT size )
{
    FIXME( "handle %p, reserved %#lx, count %u, config %p, size %u stub!\n",
           hwnd, reserved, count, config, size );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

BOOL WINAPI GetPointerTouchInfo( UINT32 id, POINTER_TOUCH_INFO *info )
{
    FIXME( "id %u, info %p stub!\n", id, info );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

BOOL WINAPI GetPointerTouchInfoHistory( UINT32 id, UINT32 *count, POINTER_TOUCH_INFO *info )
{
    FIXME( "id %u, count %p, info %p stub!\n", id, count, info );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}


/*******************************************************************
 *           SetForegroundWindow  (USER32.@)
 */
BOOL WINAPI SetForegroundWindow( HWND hwnd )
{
    return NtUserSetForegroundWindow( hwnd );
}


/*******************************************************************
 *           GetActiveWindow  (USER32.@)
 */
HWND WINAPI GetActiveWindow(void)
{
    GUITHREADINFO info;
    info.cbSize = sizeof(info);
    return NtUserGetGUIThreadInfo( GetCurrentThreadId(), &info ) ? info.hwndActive : 0;
}


/*****************************************************************
 *           GetFocus  (USER32.@)
 */
HWND WINAPI GetFocus(void)
{
    GUITHREADINFO info;
    info.cbSize = sizeof(info);
    return NtUserGetGUIThreadInfo( GetCurrentThreadId(), &info ) ? info.hwndFocus : 0;
}


/*******************************************************************
 *           SetShellWindow (USER32.@)
 */
BOOL WINAPI SetShellWindow( HWND hwnd )
{
    return NtUserSetShellWindowEx( hwnd, hwnd );
}


/*******************************************************************
 *           GetShellWindow (USER32.@)
 */
HWND WINAPI GetShellWindow(void)
{
    return NtUserGetShellWindow();
}


/***********************************************************************
 *           SetProgmanWindow (USER32.@)
 */
HWND WINAPI SetProgmanWindow( HWND hwnd )
{
    return NtUserSetProgmanWindow( hwnd );
}


/***********************************************************************
 *           GetProgmanWindow (USER32.@)
 */
HWND WINAPI GetProgmanWindow(void)
{
    return NtUserGetProgmanWindow();
}


/***********************************************************************
 *           SetTaskmanWindow (USER32.@)
 */
HWND WINAPI SetTaskmanWindow( HWND hwnd )
{
    return NtUserSetTaskmanWindow( hwnd );
}

/***********************************************************************
 *           GetTaskmanWindow (USER32.@)
 */
HWND WINAPI GetTaskmanWindow(void)
{
    return NtUserGetTaskmanWindow();
}

HSYNTHETICPOINTERDEVICE WINAPI CreateSyntheticPointerDevice(POINTER_INPUT_TYPE type, ULONG max_count, POINTER_FEEDBACK_MODE mode)
{
    FIXME( "type %ld, max_count %ld, mode %d stub!\n", type, max_count, mode);
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return NULL;
}
