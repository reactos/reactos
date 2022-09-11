/*		DirectInput Device
 *
 * Copyright 1998 Marcus Meissner
 * Copyright 1998,1999 Lionel Ulmer
 *
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

/* This file contains all the Device specific functions that can be used as stubs
   by real device implementations.

   It also contains all the helper functions.
*/

#include <stdarg.h>
#include <string.h>
#include <math.h>

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winuser.h"
#include "winerror.h"
#include "dinput.h"
#include "dinputd.h"
#include "hidusage.h"

#include "initguid.h"
#include "device_private.h"
#include "dinput_private.h"

#include "wine/debug.h"

#define WM_WINE_NOTIFY_ACTIVITY WM_USER

WINE_DEFAULT_DEBUG_CHANNEL(dinput);

/* Windows uses this GUID for guidProduct on non-keyboard/mouse devices.
 * Data1 contains the device VID (low word) and PID (high word).
 * Data4 ends with the ASCII bytes "PIDVID".
 */
DEFINE_GUID( dinput_pidvid_guid, 0x00000000, 0x0000, 0x0000, 0x00, 0x00, 'P', 'I', 'D', 'V', 'I', 'D' );

static inline struct dinput_device *impl_from_IDirectInputDevice8W( IDirectInputDevice8W *iface )
{
    return CONTAINING_RECORD( iface, struct dinput_device, IDirectInputDevice8W_iface );
}

static inline IDirectInputDevice8A *IDirectInputDevice8A_from_impl( struct dinput_device *This )
{
    return &This->IDirectInputDevice8A_iface;
}
static inline IDirectInputDevice8W *IDirectInputDevice8W_from_impl( struct dinput_device *This )
{
    return &This->IDirectInputDevice8W_iface;
}

static inline const char *debugstr_didataformat( const DIDATAFORMAT *data )
{
    if (!data) return "(null)";
    return wine_dbg_sprintf( "%p dwSize %lu, dwObjsize %lu, dwFlags %#lx, dwDataSize %lu, dwNumObjs %lu, rgodf %p",
                             data, data->dwSize, data->dwObjSize, data->dwFlags, data->dwDataSize, data->dwNumObjs, data->rgodf );
}

static inline const char *debugstr_diobjectdataformat( const DIOBJECTDATAFORMAT *data )
{
    if (!data) return "(null)";
    return wine_dbg_sprintf( "%p pguid %s, dwOfs %#lx, dwType %#lx, dwFlags %#lx", data,
                             debugstr_guid( data->pguid ), data->dwOfs, data->dwType, data->dwFlags );
}

static inline BOOL is_exclusively_acquired( struct dinput_device *device )
{
    return device->status == STATUS_ACQUIRED && (device->dwCoopLevel & DISCL_EXCLUSIVE);
}

/******************************************************************************
 *	Various debugging tools
 */
static void _dump_cooperativelevel_DI(DWORD dwFlags) {
    if (TRACE_ON(dinput)) {
	unsigned int   i;
	static const struct {
	    DWORD       mask;
	    const char  *name;
	} flags[] = {
#define FE(x) { x, #x}
	    FE(DISCL_BACKGROUND),
	    FE(DISCL_EXCLUSIVE),
	    FE(DISCL_FOREGROUND),
	    FE(DISCL_NONEXCLUSIVE),
	    FE(DISCL_NOWINKEY)
#undef FE
	};
	TRACE(" cooperative level : ");
	for (i = 0; i < ARRAY_SIZE(flags); i++)
	    if (flags[i].mask & dwFlags)
		TRACE("%s ",flags[i].name);
	TRACE("\n");
    }
}

/******************************************************************************
 * Get the default and the app-specific config keys.
 */
BOOL get_app_key(HKEY *defkey, HKEY *appkey)
{
    char buffer[MAX_PATH+16];
    DWORD len;

    *appkey = 0;

    /* @@ Wine registry key: HKCU\Software\Wine\DirectInput */
    if (RegOpenKeyA(HKEY_CURRENT_USER, "Software\\Wine\\DirectInput", defkey))
        *defkey = 0;

    len = GetModuleFileNameA(0, buffer, MAX_PATH);
    if (len && len < MAX_PATH)
    {
        HKEY tmpkey;

        /* @@ Wine registry key: HKCU\Software\Wine\AppDefaults\app.exe\DirectInput */
        if (!RegOpenKeyA(HKEY_CURRENT_USER, "Software\\Wine\\AppDefaults", &tmpkey))
        {
            char *p, *appname = buffer;
            if ((p = strrchr(appname, '/'))) appname = p + 1;
            if ((p = strrchr(appname, '\\'))) appname = p + 1;
            strcat(appname, "\\DirectInput");

            if (RegOpenKeyA(tmpkey, appname, appkey)) *appkey = 0;
            RegCloseKey(tmpkey);
        }
    }

    return *defkey || *appkey;
}

/******************************************************************************
 * Get a config key from either the app-specific or the default config
 */
DWORD get_config_key( HKEY defkey, HKEY appkey, const WCHAR *name, WCHAR *buffer, DWORD size )
{
    if (appkey && !RegQueryValueExW( appkey, name, 0, NULL, (LPBYTE)buffer, &size )) return 0;

    if (defkey && !RegQueryValueExW( defkey, name, 0, NULL, (LPBYTE)buffer, &size )) return 0;

    return ERROR_FILE_NOT_FOUND;
}

BOOL device_instance_is_disabled( DIDEVICEINSTANCEW *instance, BOOL *override )
{
    static const WCHAR disabled_str[] = {'d', 'i', 's', 'a', 'b', 'l', 'e', 'd', 0};
    static const WCHAR override_str[] = {'o', 'v', 'e', 'r', 'r', 'i', 'd', 'e', 0};
    static const WCHAR joystick_key[] = {'J', 'o', 'y', 's', 't', 'i', 'c', 'k', 's', 0};
    WCHAR buffer[MAX_PATH];
    HKEY hkey, appkey, temp;
    BOOL disable = FALSE;

    get_app_key( &hkey, &appkey );
    if (override) *override = FALSE;

    /* Joystick settings are in the 'joysticks' subkey */
    if (appkey)
    {
        if (RegOpenKeyW( appkey, joystick_key, &temp )) temp = 0;
        RegCloseKey( appkey );
        appkey = temp;
    }

    if (hkey)
    {
        if (RegOpenKeyW( hkey, joystick_key, &temp )) temp = 0;
        RegCloseKey( hkey );
        hkey = temp;
    }

    /* Look for the "controllername"="disabled" key */
    if (!get_config_key( hkey, appkey, instance->tszInstanceName, buffer, sizeof(buffer) ))
    {
        if (!wcscmp( disabled_str, buffer ))
        {
            TRACE( "Disabling joystick '%s' based on registry key.\n", debugstr_w(instance->tszInstanceName) );
            disable = TRUE;
        }
        else if (override && !wcscmp( override_str, buffer ))
        {
            TRACE( "Force enabling joystick '%s' based on registry key.\n", debugstr_w(instance->tszInstanceName) );
            *override = TRUE;
        }
    }

    if (appkey) RegCloseKey( appkey );
    if (hkey) RegCloseKey( hkey );

    return disable;
}

static void dinput_device_release_user_format( struct dinput_device *impl )
{
    if (impl->user_format) free( impl->user_format->rgodf );
    free( impl->user_format );
    impl->user_format = NULL;
}

static inline LPDIOBJECTDATAFORMAT dataformat_to_odf(LPCDIDATAFORMAT df, int idx)
{
    if (idx < 0 || idx >= df->dwNumObjs) return NULL;
    return (LPDIOBJECTDATAFORMAT)((LPBYTE)df->rgodf + idx * df->dwObjSize);
}

/* dataformat_to_odf_by_type
 *  Find the Nth object of the selected type in the DataFormat
 */
LPDIOBJECTDATAFORMAT dataformat_to_odf_by_type(LPCDIDATAFORMAT df, int n, DWORD type)
{
    int i, nfound = 0;

    for (i=0; i < df->dwNumObjs; i++)
    {
        LPDIOBJECTDATAFORMAT odf = dataformat_to_odf(df, i);

        if (odf->dwType & type)
        {
            if (n == nfound)
                return odf;

            nfound++;
        }
    }

    return NULL;
}

static BOOL match_device_object( DIDATAFORMAT *device_format, DIDATAFORMAT *user_format,
                                 const DIDATAFORMAT *format, const DIOBJECTDATAFORMAT *match_obj, DWORD version )
{
    DWORD i, device_instance, instance = DIDFT_GETINSTANCE( match_obj->dwType );
    DIOBJECTDATAFORMAT *device_obj, *user_obj;

    if (version < 0x0700 && instance == 0xff) instance = 0xffff;

    for (i = 0; i < device_format->dwNumObjs; i++)
    {
        user_obj = user_format->rgodf + i;
        device_obj = device_format->rgodf + i;
        device_instance = DIDFT_GETINSTANCE( device_obj->dwType );

        if (!(user_obj->dwType & DIDFT_OPTIONAL)) continue; /* already matched */
        if (match_obj->pguid && device_obj->pguid && !IsEqualGUID( device_obj->pguid, match_obj->pguid )) continue;
        if (instance != DIDFT_GETINSTANCE( DIDFT_ANYINSTANCE ) && instance != device_instance) continue;
        if (!(DIDFT_GETTYPE( match_obj->dwType ) & DIDFT_GETTYPE( device_obj->dwType ))) continue;

        TRACE( "match %s with device %s\n", debugstr_diobjectdataformat( match_obj ),
               debugstr_diobjectdataformat( device_obj ) );

        *user_obj = *device_obj;
        user_obj->dwOfs = match_obj->dwOfs;
        return TRUE;
    }

    return FALSE;
}

static HRESULT dinput_device_init_user_format( struct dinput_device *impl, const DIDATAFORMAT *format )
{
    DIDATAFORMAT *user_format, *device_format = impl->device_format;
    DIOBJECTDATAFORMAT *user_obj, *match_obj;
    DWORD i;

    if (!device_format) return DIERR_INVALIDPARAM;
    if (!(user_format = malloc( sizeof(DIDATAFORMAT) ))) return DIERR_OUTOFMEMORY;
    *user_format = *device_format;
    user_format->dwFlags = format->dwFlags;
    user_format->dwDataSize = format->dwDataSize;
    user_format->dwNumObjs += format->dwNumObjs;
    if (!(user_format->rgodf = calloc( user_format->dwNumObjs, sizeof(DIOBJECTDATAFORMAT) )))
    {
        free( user_format );
        return DIERR_OUTOFMEMORY;
    }

    user_obj = user_format->rgodf + user_format->dwNumObjs;
    while (user_obj-- > user_format->rgodf) user_obj->dwType |= DIDFT_OPTIONAL;

    for (i = 0; i < format->dwNumObjs; ++i)
    {
        match_obj = format->rgodf + i;

        if (!match_device_object( device_format, user_format, format, match_obj, impl->dinput->dwVersion ))
        {
            WARN( "object %s not found\n", debugstr_diobjectdataformat( match_obj ) );
            if (!(match_obj->dwType & DIDFT_OPTIONAL)) goto failed;
            user_obj = user_format->rgodf + device_format->dwNumObjs + i;
            *user_obj = *match_obj;
        }
    }

    user_obj = user_format->rgodf + user_format->dwNumObjs;
    while (user_obj-- > user_format->rgodf) user_obj->dwType &= ~DIDFT_OPTIONAL;

    impl->user_format = user_format;
    return DI_OK;

failed:
    free( user_format->rgodf );
    free( user_format );
    return DIERR_INVALIDPARAM;
}

static int id_to_offset( struct dinput_device *impl, int id )
{
    DIDATAFORMAT *device_format = impl->device_format, *user_format = impl->user_format;
    DIOBJECTDATAFORMAT *user_obj;

    if (!user_format) return -1;

    user_obj = user_format->rgodf + device_format->dwNumObjs;
    while (user_obj-- > user_format->rgodf)
    {
        if (!user_obj->dwType) continue;
        if ((user_obj->dwType & 0x00ffffff) == (id & 0x00ffffff)) return user_obj->dwOfs;
    }

    return -1;
}

static DWORD semantic_to_obj_id( struct dinput_device *This, DWORD dwSemantic )
{
    DWORD type = (0x0000ff00 & dwSemantic) >> 8;
    BOOL byofs = (dwSemantic & 0x80000000) != 0;
    DWORD value = (dwSemantic & 0x000000ff);
    BOOL found = FALSE;
    DWORD instance;
    int i;

    for (i = 0; i < This->device_format->dwNumObjs && !found; i++)
    {
        LPDIOBJECTDATAFORMAT odf = dataformat_to_odf( This->device_format, i );

        if (byofs && value != odf->dwOfs) continue;
        if (!byofs && value != DIDFT_GETINSTANCE(odf->dwType)) continue;
        instance = DIDFT_GETINSTANCE(odf->dwType);
        found = TRUE;
    }

    if (!found) return 0;

    if (type & DIDFT_AXIS)   type = DIDFT_RELAXIS;
    if (type & DIDFT_BUTTON) type = DIDFT_PSHBUTTON;

    return type | (0x0000ff00 & (instance << 8));
}

/*
 * get_mapping_key
 * Retrieves an open registry key to save the mapping, parametrized for an username,
 * specific device and specific action mapping guid.
 */
static HKEY get_mapping_key(const WCHAR *device, const WCHAR *username, const WCHAR *guid)
{
    static const WCHAR *subkey = L"Software\\Wine\\DirectInput\\Mappings\\%s\\%s\\%s";
    HKEY hkey;
    WCHAR *keyname;

    SIZE_T len = wcslen( subkey ) + wcslen( username ) + wcslen( device ) + wcslen( guid ) + 1;
    keyname = malloc( sizeof(WCHAR) * len );
    swprintf( keyname, len, subkey, username, device, guid );

    /* The key used is HKCU\Software\Wine\DirectInput\Mappings\[username]\[device]\[mapping_guid] */
    if (RegCreateKeyW(HKEY_CURRENT_USER, keyname, &hkey))
        hkey = 0;

    free( keyname );

    return hkey;
}

static HRESULT save_mapping_settings(IDirectInputDevice8W *iface, LPDIACTIONFORMATW lpdiaf, LPCWSTR lpszUsername)
{
    WCHAR *guid_str = NULL;
    DIDEVICEINSTANCEW didev;
    HKEY hkey;
    int i;

    didev.dwSize = sizeof(didev);
    IDirectInputDevice8_GetDeviceInfo(iface, &didev);

    if (StringFromCLSID(&lpdiaf->guidActionMap, &guid_str) != S_OK)
        return DI_SETTINGSNOTSAVED;

    hkey = get_mapping_key(didev.tszInstanceName, lpszUsername, guid_str);

    if (!hkey)
    {
        CoTaskMemFree(guid_str);
        return DI_SETTINGSNOTSAVED;
    }

    /* Write each of the actions mapped for this device.
       Format is "dwSemantic"="dwObjID" and key is of type REG_DWORD
    */
    for (i = 0; i < lpdiaf->dwNumActions; i++)
    {
        WCHAR label[9];

        if (IsEqualGUID(&didev.guidInstance, &lpdiaf->rgoAction[i].guidInstance) &&
            lpdiaf->rgoAction[i].dwHow != DIAH_UNMAPPED)
        {
            swprintf( label, 9, L"%x", lpdiaf->rgoAction[i].dwSemantic );
            RegSetValueExW( hkey, label, 0, REG_DWORD, (const BYTE *)&lpdiaf->rgoAction[i].dwObjID,
                            sizeof(DWORD) );
        }
    }

    RegCloseKey(hkey);
    CoTaskMemFree(guid_str);

    return DI_OK;
}

static BOOL load_mapping_settings( struct dinput_device *This, LPDIACTIONFORMATW lpdiaf, const WCHAR *username )
{
    HKEY hkey;
    WCHAR *guid_str;
    DIDEVICEINSTANCEW didev;
    int i, mapped = 0;

    didev.dwSize = sizeof(didev);
    IDirectInputDevice8_GetDeviceInfo(&This->IDirectInputDevice8W_iface, &didev);

    if (StringFromCLSID(&lpdiaf->guidActionMap, &guid_str) != S_OK)
        return FALSE;

    hkey = get_mapping_key(didev.tszInstanceName, username, guid_str);

    if (!hkey)
    {
        CoTaskMemFree(guid_str);
        return FALSE;
    }

    /* Try to read each action in the DIACTIONFORMAT from registry */
    for (i = 0; i < lpdiaf->dwNumActions; i++)
    {
        DWORD id, size = sizeof(DWORD);
        WCHAR label[9];

        swprintf( label, 9, L"%x", lpdiaf->rgoAction[i].dwSemantic );

        if (!RegQueryValueExW(hkey, label, 0, NULL, (LPBYTE) &id, &size))
        {
            lpdiaf->rgoAction[i].dwObjID = id;
            lpdiaf->rgoAction[i].guidInstance = didev.guidInstance;
            lpdiaf->rgoAction[i].dwHow = DIAH_DEFAULT;
            mapped += 1;
        }
    }

    RegCloseKey(hkey);
    CoTaskMemFree(guid_str);

    return mapped > 0;
}

static BOOL set_app_data( struct dinput_device *dev, int offset, UINT_PTR app_data )
{
    int num_actions = dev->num_actions;
    ActionMap *action_map = dev->action_map, *target_map = NULL;

    if (num_actions == 0)
    {
        num_actions = 1;
        action_map = malloc( sizeof(ActionMap) );
        if (!action_map) return FALSE;
        target_map = &action_map[0];
    }
    else
    {
        int i;
        for (i = 0; i < num_actions; i++)
        {
            if (dev->action_map[i].offset != offset) continue;
            target_map = &dev->action_map[i];
            break;
        }

        if (!target_map)
        {
            num_actions++;
            action_map = realloc( action_map, sizeof(ActionMap) * num_actions );
            if (!action_map) return FALSE;
            target_map = &action_map[num_actions-1];
        }
    }

    target_map->offset = offset;
    target_map->uAppData = app_data;

    dev->action_map = action_map;
    dev->num_actions = num_actions;

    return TRUE;
}

/******************************************************************************
 *	queue_event - add new event to the ring queue
 */

void queue_event( IDirectInputDevice8W *iface, int inst_id, DWORD data, DWORD time, DWORD seq )
{
    static ULONGLONG notify_ms = 0;
    struct dinput_device *This = impl_from_IDirectInputDevice8W( iface );
    int next_pos, ofs = id_to_offset( This, inst_id );
    ULONGLONG time_ms = GetTickCount64();

    if (time_ms - notify_ms > 1000)
    {
        PostMessageW(GetDesktopWindow(), WM_WINE_NOTIFY_ACTIVITY, 0, 0);
        notify_ms = time_ms;
    }

    if (!This->queue_len || This->overflow || ofs < 0) return;

    next_pos = (This->queue_head + 1) % This->queue_len;
    if (next_pos == This->queue_tail)
    {
        TRACE(" queue overflowed\n");
        This->overflow = TRUE;
        return;
    }

    TRACE( " queueing %lu at offset %u (queue head %u / size %u)\n", data, ofs, This->queue_head, This->queue_len );

    This->data_queue[This->queue_head].dwOfs       = ofs;
    This->data_queue[This->queue_head].dwData      = data;
    This->data_queue[This->queue_head].dwTimeStamp = time;
    This->data_queue[This->queue_head].dwSequence  = seq;
    This->data_queue[This->queue_head].uAppData    = -1;

    /* Set uAppData by means of action mapping */
    if (This->num_actions > 0)
    {
        int i;
        for (i=0; i < This->num_actions; i++)
        {
            if (This->action_map[i].offset == ofs)
            {
                TRACE( "Offset %d mapped to uAppData %#Ix\n", ofs, This->action_map[i].uAppData );
                This->data_queue[This->queue_head].uAppData = This->action_map[i].uAppData;
                break;
            }
        }
    }

    This->queue_head = next_pos;
    /* Send event if asked */
}

/******************************************************************************
 *	Acquire
 */

static HRESULT WINAPI dinput_device_Acquire( IDirectInputDevice8W *iface )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8W( iface );
    HRESULT hr = DI_OK;

    TRACE( "iface %p.\n", iface );

    EnterCriticalSection( &impl->crit );
    if (impl->status == STATUS_ACQUIRED)
        hr = DI_NOEFFECT;
    else if (!impl->user_format)
        hr = DIERR_INVALIDPARAM;
    else if ((impl->dwCoopLevel & DISCL_FOREGROUND) && impl->win != GetForegroundWindow())
        hr = DIERR_OTHERAPPHASPRIO;
    else
    {
        impl->status = STATUS_ACQUIRED;
        if (FAILED(hr = impl->vtbl->acquire( iface ))) impl->status = STATUS_UNACQUIRED;
    }
    LeaveCriticalSection( &impl->crit );
    if (hr != DI_OK) return hr;

    dinput_hooks_acquire_device( iface );
    check_dinput_hooks( iface, TRUE );

    return hr;
}

/******************************************************************************
 *	Unacquire
 */

static HRESULT WINAPI dinput_device_Unacquire( IDirectInputDevice8W *iface )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8W( iface );
    HRESULT hr = DI_OK;

    TRACE( "iface %p.\n", iface );

    EnterCriticalSection( &impl->crit );
    if (impl->status != STATUS_ACQUIRED) hr = DI_NOEFFECT;
    else hr = impl->vtbl->unacquire( iface );
    impl->status = STATUS_UNACQUIRED;
    LeaveCriticalSection( &impl->crit );
    if (hr != DI_OK) return hr;

    dinput_hooks_unacquire_device( iface );
    check_dinput_hooks( iface, FALSE );

    return hr;
}

/******************************************************************************
 *	IDirectInputDeviceA
 */

static HRESULT WINAPI dinput_device_SetDataFormat( IDirectInputDevice8W *iface, const DIDATAFORMAT *format )
{
    struct dinput_device *This = impl_from_IDirectInputDevice8W( iface );
    HRESULT res = DI_OK;
    ULONG i;

    TRACE( "iface %p, format %p.\n", iface, format );

    if (!format) return E_POINTER;
    if (TRACE_ON( dinput ))
    {
        TRACE( "user format %s\n", debugstr_didataformat( format ) );
        for (i = 0; i < format->dwNumObjs; ++i) TRACE( "  %lu: object %s\n", i, debugstr_diobjectdataformat( format->rgodf + i ) );
    }

    if (format->dwSize != sizeof(DIDATAFORMAT)) return DIERR_INVALIDPARAM;
    if (format->dwObjSize != sizeof(DIOBJECTDATAFORMAT)) return DIERR_INVALIDPARAM;
    if (This->status == STATUS_ACQUIRED) return DIERR_ACQUIRED;

    EnterCriticalSection(&This->crit);

    free( This->action_map );
    This->action_map = NULL;
    This->num_actions = 0;

    dinput_device_release_user_format( This );
    res = dinput_device_init_user_format( This, format );

    LeaveCriticalSection(&This->crit);
    return res;
}

/******************************************************************************
  *     SetCooperativeLevel
  *
  *  Set cooperative level and the source window for the events.
  */
static HRESULT WINAPI dinput_device_SetCooperativeLevel( IDirectInputDevice8W *iface, HWND hwnd, DWORD flags )
{
    struct dinput_device *This = impl_from_IDirectInputDevice8W( iface );
    HRESULT hr;

    TRACE( "iface %p, hwnd %p, flags %#lx.\n", iface, hwnd, flags );

    _dump_cooperativelevel_DI( flags );

    if ((flags & (DISCL_EXCLUSIVE | DISCL_NONEXCLUSIVE)) == 0 ||
        (flags & (DISCL_EXCLUSIVE | DISCL_NONEXCLUSIVE)) == (DISCL_EXCLUSIVE | DISCL_NONEXCLUSIVE) ||
        (flags & (DISCL_FOREGROUND | DISCL_BACKGROUND)) == 0 ||
        (flags & (DISCL_FOREGROUND | DISCL_BACKGROUND)) == (DISCL_FOREGROUND | DISCL_BACKGROUND))
        return DIERR_INVALIDPARAM;

    if (hwnd && GetWindowLongW(hwnd, GWL_STYLE) & WS_CHILD) return E_HANDLE;

    if (!hwnd && flags == (DISCL_NONEXCLUSIVE | DISCL_BACKGROUND)) hwnd = GetDesktopWindow();

    if (!IsWindow(hwnd)) return E_HANDLE;

    /* For security reasons native does not allow exclusive background level
       for mouse and keyboard only */
    if (flags & DISCL_EXCLUSIVE && flags & DISCL_BACKGROUND &&
        (IsEqualGUID( &This->guid, &GUID_SysMouse ) || IsEqualGUID( &This->guid, &GUID_SysKeyboard )))
        return DIERR_UNSUPPORTED;

    /* Store the window which asks for the mouse */
    EnterCriticalSection(&This->crit);
    if (This->status == STATUS_ACQUIRED) hr = DIERR_ACQUIRED;
    else
    {
        This->win = hwnd;
        This->dwCoopLevel = flags;
        hr = DI_OK;
    }
    LeaveCriticalSection(&This->crit);

    return hr;
}

static HRESULT WINAPI dinput_device_GetDeviceInfo( IDirectInputDevice8W *iface, DIDEVICEINSTANCEW *instance )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8W( iface );
    DWORD size;

    TRACE( "iface %p, instance %p.\n", iface, instance );

    if (!instance) return E_POINTER;
    if (instance->dwSize != sizeof(DIDEVICEINSTANCE_DX3W) &&
        instance->dwSize != sizeof(DIDEVICEINSTANCEW))
        return DIERR_INVALIDPARAM;

    size = instance->dwSize;
    memcpy( instance, &impl->instance, size );
    instance->dwSize = size;

    return S_OK;
}

/******************************************************************************
  *     SetEventNotification : specifies event to be sent on state change
  */
static HRESULT WINAPI dinput_device_SetEventNotification( IDirectInputDevice8W *iface, HANDLE event )
{
    struct dinput_device *This = impl_from_IDirectInputDevice8W( iface );

    TRACE( "iface %p, event %p.\n", iface, event );

    EnterCriticalSection(&This->crit);
    This->hEvent = event;
    LeaveCriticalSection(&This->crit);
    return DI_OK;
}

void dinput_device_destroy( IDirectInputDevice8W *iface )
{
    struct dinput_device *This = impl_from_IDirectInputDevice8W( iface );

    TRACE( "iface %p.\n", iface );

    free( This->object_properties );
    free( This->data_queue );

    /* Free data format */
    free( This->device_format->rgodf );
    free( This->device_format );
    dinput_device_release_user_format( This );

    /* Free action mapping */
    free( This->action_map );

    IDirectInput_Release(&This->dinput->IDirectInput7A_iface);
    This->crit.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&This->crit);

    free( This );
}

static ULONG WINAPI dinput_device_Release( IDirectInputDevice8W *iface )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8W( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );

    TRACE( "iface %p, ref %lu.\n", iface, ref );

    if (!ref)
    {
        IDirectInputDevice_Unacquire( iface );
        if (impl->vtbl->release) impl->vtbl->release( iface );
        else dinput_device_destroy( iface );
    }

    return ref;
}

static HRESULT WINAPI dinput_device_GetCapabilities( IDirectInputDevice8W *iface, DIDEVCAPS *caps )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8W( iface );
    DWORD size;

    TRACE( "iface %p, caps %p.\n", iface, caps );

    if (!caps) return E_POINTER;
    if (caps->dwSize != sizeof(DIDEVCAPS_DX3) &&
        caps->dwSize != sizeof(DIDEVCAPS))
        return DIERR_INVALIDPARAM;

    size = caps->dwSize;
    memcpy( caps, &impl->caps, size );
    caps->dwSize = size;

    return DI_OK;
}

static HRESULT WINAPI dinput_device_QueryInterface( IDirectInputDevice8W *iface, const GUID *iid, void **out )
{
    struct dinput_device *This = impl_from_IDirectInputDevice8W( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( &IID_IDirectInputDeviceA, iid ) ||
        IsEqualGUID( &IID_IDirectInputDevice2A, iid ) ||
        IsEqualGUID( &IID_IDirectInputDevice7A, iid ) ||
        IsEqualGUID( &IID_IDirectInputDevice8A, iid ))
    {
        IDirectInputDevice2_AddRef(iface);
        *out = IDirectInputDevice8A_from_impl( This );
        return DI_OK;
    }

    if (IsEqualGUID( &IID_IUnknown, iid ) ||
        IsEqualGUID( &IID_IDirectInputDeviceW, iid ) ||
        IsEqualGUID( &IID_IDirectInputDevice2W, iid ) ||
        IsEqualGUID( &IID_IDirectInputDevice7W, iid ) ||
        IsEqualGUID( &IID_IDirectInputDevice8W, iid ))
    {
        IDirectInputDevice2_AddRef(iface);
        *out = IDirectInputDevice8W_from_impl( This );
        return DI_OK;
    }

    WARN( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    return E_NOINTERFACE;
}

static ULONG WINAPI dinput_device_AddRef( IDirectInputDevice8W *iface )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8W( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p, ref %lu.\n", iface, ref );
    return ref;
}

static HRESULT WINAPI dinput_device_EnumObjects( IDirectInputDevice8W *iface,
                                                 LPDIENUMDEVICEOBJECTSCALLBACKW callback,
                                                 void *context, DWORD flags )
{
    static const DIPROPHEADER filter =
    {
        .dwSize = sizeof(filter),
        .dwHeaderSize = sizeof(filter),
        .dwHow = DIPH_DEVICE,
    };
    struct dinput_device *impl = impl_from_IDirectInputDevice8W( iface );
    HRESULT hr;

    TRACE( "iface %p, callback %p, context %p, flags %#lx.\n", iface, callback, context, flags );

    if (!callback) return DIERR_INVALIDPARAM;
    if (flags & ~(DIDFT_AXIS | DIDFT_POV | DIDFT_BUTTON | DIDFT_NODATA | DIDFT_COLLECTION))
        return DIERR_INVALIDPARAM;

    if (flags == DIDFT_ALL || (flags & DIDFT_AXIS))
    {
        hr = impl->vtbl->enum_objects( iface, &filter, DIDFT_AXIS, callback, context );
        if (FAILED(hr)) return hr;
        if (hr != DIENUM_CONTINUE) return DI_OK;
    }
    if (flags == DIDFT_ALL || (flags & DIDFT_POV))
    {
        hr = impl->vtbl->enum_objects( iface, &filter, DIDFT_POV, callback, context );
        if (FAILED(hr)) return hr;
        if (hr != DIENUM_CONTINUE) return DI_OK;
    }
    if (flags == DIDFT_ALL || (flags & DIDFT_BUTTON))
    {
        hr = impl->vtbl->enum_objects( iface, &filter, DIDFT_BUTTON, callback, context );
        if (FAILED(hr)) return hr;
        if (hr != DIENUM_CONTINUE) return DI_OK;
    }
    if (flags == DIDFT_ALL || (flags & (DIDFT_NODATA | DIDFT_COLLECTION)))
    {
        hr = impl->vtbl->enum_objects( iface, &filter, DIDFT_NODATA, callback, context );
        if (FAILED(hr)) return hr;
        if (hr != DIENUM_CONTINUE) return DI_OK;
    }

    return DI_OK;
}

static HRESULT enum_object_filter_init( struct dinput_device *impl, DIPROPHEADER *filter )
{
    DIDATAFORMAT *device_format = impl->device_format, *user_format = impl->user_format;
    DIOBJECTDATAFORMAT *device_obj, *user_obj;

    if (filter->dwHow > DIPH_BYUSAGE) return DIERR_INVALIDPARAM;
    if (filter->dwHow == DIPH_BYUSAGE && !(impl->instance.dwDevType & DIDEVTYPE_HID)) return DIERR_UNSUPPORTED;
    if (filter->dwHow != DIPH_BYOFFSET) return DI_OK;

    if (!impl->user_format) return DIERR_NOTFOUND;

    user_obj = user_format->rgodf + device_format->dwNumObjs;
    device_obj = device_format->rgodf + device_format->dwNumObjs;
    while (user_obj-- > user_format->rgodf && device_obj-- > device_format->rgodf)
    {
        if (!user_obj->dwType) continue;
        if (user_obj->dwOfs == filter->dwObj) break;
    }
    if (user_obj < user_format->rgodf) return DIERR_NOTFOUND;

    filter->dwObj = device_obj->dwOfs;
    return DI_OK;
}

static HRESULT check_property( struct dinput_device *impl, const GUID *guid, const DIPROPHEADER *header, BOOL set )
{
    switch (LOWORD( guid ))
    {
    case (DWORD_PTR)DIPROP_VIDPID:
    case (DWORD_PTR)DIPROP_TYPENAME:
    case (DWORD_PTR)DIPROP_USERNAME:
    case (DWORD_PTR)DIPROP_KEYNAME:
    case (DWORD_PTR)DIPROP_LOGICALRANGE:
    case (DWORD_PTR)DIPROP_PHYSICALRANGE:
    case (DWORD_PTR)DIPROP_APPDATA:
        if (impl->dinput->dwVersion < 0x0800) return DIERR_UNSUPPORTED;
        break;
    }

    switch (LOWORD( guid ))
    {
    case (DWORD_PTR)DIPROP_INSTANCENAME:
    case (DWORD_PTR)DIPROP_KEYNAME:
    case (DWORD_PTR)DIPROP_PRODUCTNAME:
    case (DWORD_PTR)DIPROP_TYPENAME:
    case (DWORD_PTR)DIPROP_USERNAME:
        if (header->dwSize != sizeof(DIPROPSTRING)) return DIERR_INVALIDPARAM;
        break;

    case (DWORD_PTR)DIPROP_AUTOCENTER:
    case (DWORD_PTR)DIPROP_AXISMODE:
    case (DWORD_PTR)DIPROP_BUFFERSIZE:
    case (DWORD_PTR)DIPROP_CALIBRATIONMODE:
    case (DWORD_PTR)DIPROP_DEADZONE:
    case (DWORD_PTR)DIPROP_FFGAIN:
    case (DWORD_PTR)DIPROP_FFLOAD:
    case (DWORD_PTR)DIPROP_GRANULARITY:
    case (DWORD_PTR)DIPROP_JOYSTICKID:
    case (DWORD_PTR)DIPROP_SATURATION:
    case (DWORD_PTR)DIPROP_SCANCODE:
    case (DWORD_PTR)DIPROP_VIDPID:
        if (header->dwSize != sizeof(DIPROPDWORD)) return DIERR_INVALIDPARAM;
        break;

    case (DWORD_PTR)DIPROP_APPDATA:
        if (header->dwSize != sizeof(DIPROPPOINTER)) return DIERR_INVALIDPARAM;
        break;

    case (DWORD_PTR)DIPROP_PHYSICALRANGE:
    case (DWORD_PTR)DIPROP_LOGICALRANGE:
    case (DWORD_PTR)DIPROP_RANGE:
        if (header->dwSize != sizeof(DIPROPRANGE)) return DIERR_INVALIDPARAM;
        break;

    case (DWORD_PTR)DIPROP_GUIDANDPATH:
        if (header->dwSize != sizeof(DIPROPGUIDANDPATH)) return DIERR_INVALIDPARAM;
        break;
    }

    switch (LOWORD( guid ))
    {
    case (DWORD_PTR)DIPROP_PRODUCTNAME:
    case (DWORD_PTR)DIPROP_INSTANCENAME:
    case (DWORD_PTR)DIPROP_VIDPID:
    case (DWORD_PTR)DIPROP_JOYSTICKID:
    case (DWORD_PTR)DIPROP_GUIDANDPATH:
    case (DWORD_PTR)DIPROP_BUFFERSIZE:
    case (DWORD_PTR)DIPROP_FFGAIN:
    case (DWORD_PTR)DIPROP_TYPENAME:
    case (DWORD_PTR)DIPROP_USERNAME:
    case (DWORD_PTR)DIPROP_AUTOCENTER:
    case (DWORD_PTR)DIPROP_AXISMODE:
    case (DWORD_PTR)DIPROP_FFLOAD:
        if (header->dwHow != DIPH_DEVICE) return DIERR_UNSUPPORTED;
        if (header->dwObj) return DIERR_INVALIDPARAM;
        break;

    case (DWORD_PTR)DIPROP_PHYSICALRANGE:
    case (DWORD_PTR)DIPROP_LOGICALRANGE:
    case (DWORD_PTR)DIPROP_RANGE:
    case (DWORD_PTR)DIPROP_DEADZONE:
    case (DWORD_PTR)DIPROP_SATURATION:
    case (DWORD_PTR)DIPROP_GRANULARITY:
    case (DWORD_PTR)DIPROP_CALIBRATIONMODE:
        if (header->dwHow == DIPH_DEVICE && !set) return DIERR_UNSUPPORTED;
        break;

    case (DWORD_PTR)DIPROP_KEYNAME:
        if (header->dwHow == DIPH_DEVICE) return DIERR_INVALIDPARAM;
        break;

    case (DWORD_PTR)DIPROP_SCANCODE:
    case (DWORD_PTR)DIPROP_APPDATA:
        if (header->dwHow == DIPH_DEVICE) return DIERR_UNSUPPORTED;
        break;
    }

    if (set)
    {
        switch (LOWORD( guid ))
        {
        case (DWORD_PTR)DIPROP_AUTOCENTER:
            if (impl->status == STATUS_ACQUIRED && !is_exclusively_acquired( impl )) return DIERR_ACQUIRED;
            break;
        case (DWORD_PTR)DIPROP_AXISMODE:
        case (DWORD_PTR)DIPROP_BUFFERSIZE:
        case (DWORD_PTR)DIPROP_PHYSICALRANGE:
        case (DWORD_PTR)DIPROP_LOGICALRANGE:
            if (impl->status == STATUS_ACQUIRED) return DIERR_ACQUIRED;
            break;
        case (DWORD_PTR)DIPROP_FFLOAD:
        case (DWORD_PTR)DIPROP_GRANULARITY:
        case (DWORD_PTR)DIPROP_VIDPID:
        case (DWORD_PTR)DIPROP_TYPENAME:
        case (DWORD_PTR)DIPROP_USERNAME:
        case (DWORD_PTR)DIPROP_GUIDANDPATH:
            return DIERR_READONLY;
        }

        switch (LOWORD( guid ))
        {
        case (DWORD_PTR)DIPROP_RANGE:
        {
            const DIPROPRANGE *value = (const DIPROPRANGE *)header;
            if (value->lMin > value->lMax) return DIERR_INVALIDPARAM;
            break;
        }
        case (DWORD_PTR)DIPROP_DEADZONE:
        case (DWORD_PTR)DIPROP_SATURATION:
        case (DWORD_PTR)DIPROP_FFGAIN:
        {
            const DIPROPDWORD *value = (const DIPROPDWORD *)header;
            if (value->dwData > 10000) return DIERR_INVALIDPARAM;
            break;
        }
        case (DWORD_PTR)DIPROP_AUTOCENTER:
        case (DWORD_PTR)DIPROP_AXISMODE:
        case (DWORD_PTR)DIPROP_CALIBRATIONMODE:
        {
            const DIPROPDWORD *value = (const DIPROPDWORD *)header;
            if (value->dwData > 1) return DIERR_INVALIDPARAM;
            break;
        }
        case (DWORD_PTR)DIPROP_PHYSICALRANGE:
        case (DWORD_PTR)DIPROP_LOGICALRANGE:
            return DIERR_UNSUPPORTED;
        }
    }
    else
    {
        switch (LOWORD( guid ))
        {
        case (DWORD_PTR)DIPROP_RANGE:
        case (DWORD_PTR)DIPROP_GRANULARITY:
            if (!impl->caps.dwAxes) return DIERR_UNSUPPORTED;
            break;

        case (DWORD_PTR)DIPROP_KEYNAME:
            /* not supported on the mouse */
            if (impl->caps.dwAxes && !(impl->caps.dwDevType & DIDEVTYPE_HID)) return DIERR_UNSUPPORTED;
            break;

        case (DWORD_PTR)DIPROP_PHYSICALRANGE:
        case (DWORD_PTR)DIPROP_LOGICALRANGE:
        case (DWORD_PTR)DIPROP_DEADZONE:
        case (DWORD_PTR)DIPROP_SATURATION:
        case (DWORD_PTR)DIPROP_CALIBRATIONMODE:
            if (!impl->object_properties) return DIERR_UNSUPPORTED;
            break;

        case (DWORD_PTR)DIPROP_FFLOAD:
            if (!(impl->caps.dwFlags & DIDC_FORCEFEEDBACK)) return DIERR_UNSUPPORTED;
            if (!is_exclusively_acquired( impl )) return DIERR_NOTEXCLUSIVEACQUIRED;
            /* fallthrough */
        case (DWORD_PTR)DIPROP_PRODUCTNAME:
        case (DWORD_PTR)DIPROP_INSTANCENAME:
        case (DWORD_PTR)DIPROP_VIDPID:
        case (DWORD_PTR)DIPROP_JOYSTICKID:
        case (DWORD_PTR)DIPROP_GUIDANDPATH:
            if (!impl->vtbl->get_property) return DIERR_UNSUPPORTED;
            break;
        }
    }

    return DI_OK;
}

static BOOL CALLBACK find_object( const DIDEVICEOBJECTINSTANCEW *instance, void *context )
{
    *(DIDEVICEOBJECTINSTANCEW *)context = *instance;
    return DIENUM_STOP;
}

struct get_object_property_params
{
    IDirectInputDevice8W *iface;
    DIPROPHEADER *header;
    DWORD property;
};

static BOOL CALLBACK get_object_property( const DIDEVICEOBJECTINSTANCEW *instance, void *context )
{
    static const struct object_properties default_properties =
    {
        .range_min = DIPROPRANGE_NOMIN,
        .range_max = DIPROPRANGE_NOMAX,
    };
    struct get_object_property_params *params = context;
    struct dinput_device *impl = impl_from_IDirectInputDevice8W( params->iface );
    const struct object_properties *properties = NULL;

    if (!impl->object_properties) properties = &default_properties;
    else properties = impl->object_properties + instance->dwOfs / sizeof(LONG);

    switch (params->property)
    {
    case (DWORD_PTR)DIPROP_PHYSICALRANGE:
    {
        DIPROPRANGE *value = (DIPROPRANGE *)params->header;
        value->lMin = properties->physical_min;
        value->lMax = properties->physical_max;
        return DI_OK;
    }
    case (DWORD_PTR)DIPROP_LOGICALRANGE:
    {
        DIPROPRANGE *value = (DIPROPRANGE *)params->header;
        value->lMin = properties->logical_min;
        value->lMax = properties->logical_max;
        return DI_OK;
    }
    case (DWORD_PTR)DIPROP_RANGE:
    {
        DIPROPRANGE *value = (DIPROPRANGE *)params->header;
        value->lMin = properties->range_min;
        value->lMax = properties->range_max;
        return DIENUM_STOP;
    }
    case (DWORD_PTR)DIPROP_DEADZONE:
    {
        DIPROPDWORD *value = (DIPROPDWORD *)params->header;
        value->dwData = properties->deadzone;
        return DIENUM_STOP;
    }
    case (DWORD_PTR)DIPROP_SATURATION:
    {
        DIPROPDWORD *value = (DIPROPDWORD *)params->header;
        value->dwData = properties->saturation;
        return DIENUM_STOP;
    }
    case (DWORD_PTR)DIPROP_CALIBRATIONMODE:
    {
        DIPROPDWORD *value = (DIPROPDWORD *)params->header;
        value->dwData = properties->calibration_mode;
        return DI_OK;
    }
    case (DWORD_PTR)DIPROP_GRANULARITY:
    {
        DIPROPDWORD *value = (DIPROPDWORD *)params->header;
        value->dwData = 1;
        return DIENUM_STOP;
    }
    case (DWORD_PTR)DIPROP_KEYNAME:
    {
        DIPROPSTRING *value = (DIPROPSTRING *)params->header;
        lstrcpynW( value->wsz, instance->tszName, ARRAY_SIZE(value->wsz) );
        return DIENUM_STOP;
    }
    }

    return DIENUM_STOP;
}

static HRESULT dinput_device_get_property( IDirectInputDevice8W *iface, const GUID *guid, DIPROPHEADER *header )
{
    struct get_object_property_params params = {.iface = iface, .header = header, .property = LOWORD( guid )};
    struct dinput_device *impl = impl_from_IDirectInputDevice8W( iface );
    DWORD object_mask = DIDFT_AXIS | DIDFT_BUTTON | DIDFT_POV;
    DIPROPHEADER filter;
    HRESULT hr;

    filter = *header;
    if (FAILED(hr = enum_object_filter_init( impl, &filter ))) return hr;
    if (FAILED(hr = check_property( impl, guid, header, FALSE ))) return hr;

    switch (LOWORD( guid ))
    {
    case (DWORD_PTR)DIPROP_PRODUCTNAME:
    case (DWORD_PTR)DIPROP_INSTANCENAME:
    case (DWORD_PTR)DIPROP_VIDPID:
    case (DWORD_PTR)DIPROP_JOYSTICKID:
    case (DWORD_PTR)DIPROP_GUIDANDPATH:
    case (DWORD_PTR)DIPROP_FFLOAD:
        return impl->vtbl->get_property( iface, LOWORD( guid ), header, NULL );

    case (DWORD_PTR)DIPROP_RANGE:
    case (DWORD_PTR)DIPROP_PHYSICALRANGE:
    case (DWORD_PTR)DIPROP_LOGICALRANGE:
    case (DWORD_PTR)DIPROP_DEADZONE:
    case (DWORD_PTR)DIPROP_SATURATION:
    case (DWORD_PTR)DIPROP_GRANULARITY:
    case (DWORD_PTR)DIPROP_KEYNAME:
    case (DWORD_PTR)DIPROP_CALIBRATIONMODE:
        hr = impl->vtbl->enum_objects( iface, &filter, object_mask, get_object_property, &params );
        if (FAILED(hr)) return hr;
        if (hr == DIENUM_CONTINUE) return DIERR_NOTFOUND;
        return DI_OK;

    case (DWORD_PTR)DIPROP_AUTOCENTER:
    {
        DIPROPDWORD *value = (DIPROPDWORD *)header;
        if (!(impl->caps.dwFlags & DIDC_FORCEFEEDBACK)) return DIERR_UNSUPPORTED;
        value->dwData = impl->autocenter;
        return DI_OK;
    }
    case (DWORD_PTR)DIPROP_BUFFERSIZE:
    {
        DIPROPDWORD *value = (DIPROPDWORD *)header;
        value->dwData = impl->buffersize;
        return DI_OK;
    }
    case (DWORD_PTR)DIPROP_USERNAME:
    {
        DIPROPSTRING *value = (DIPROPSTRING *)header;
        struct DevicePlayer *device_player;
        LIST_FOR_EACH_ENTRY( device_player, &impl->dinput->device_players, struct DevicePlayer, entry )
        {
            if (IsEqualGUID( &device_player->instance_guid, &impl->guid ))
            {
                if (!*device_player->username) break;
                lstrcpynW( value->wsz, device_player->username, ARRAY_SIZE(value->wsz) );
                return DI_OK;
            }
        }
        return S_FALSE;
    }
    case (DWORD_PTR)DIPROP_FFGAIN:
    {
        DIPROPDWORD *value = (DIPROPDWORD *)header;
        value->dwData = impl->device_gain;
        return DI_OK;
    }
    case (DWORD_PTR)DIPROP_CALIBRATION:
        return DIERR_INVALIDPARAM;
    default:
        FIXME( "Unknown property %s\n", debugstr_guid( guid ) );
        return DIERR_UNSUPPORTED;
    }

    return DI_OK;
}

static HRESULT WINAPI dinput_device_GetProperty( IDirectInputDevice8W *iface, const GUID *guid, DIPROPHEADER *header )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8W( iface );
    HRESULT hr;

    TRACE( "iface %p, guid %s, header %p\n", iface, debugstr_guid( guid ), header );

    if (!header) return DIERR_INVALIDPARAM;
    if (header->dwHeaderSize != sizeof(DIPROPHEADER)) return DIERR_INVALIDPARAM;
    if (!IS_DIPROP( guid )) return DI_OK;

    EnterCriticalSection( &impl->crit );
    hr = dinput_device_get_property( iface, guid, header );
    LeaveCriticalSection( &impl->crit );

    return hr;
}

struct set_object_property_params
{
    IDirectInputDevice8W *iface;
    const DIPROPHEADER *header;
    DWORD property;
};

static BOOL CALLBACK set_object_property( const DIDEVICEOBJECTINSTANCEW *instance, void *context )
{
    struct set_object_property_params *params = context;
    struct dinput_device *impl = impl_from_IDirectInputDevice8W( params->iface );
    struct object_properties *properties = NULL;

    if (!impl->object_properties) return DIENUM_STOP;
    properties = impl->object_properties + instance->dwOfs / sizeof(LONG);

    switch (params->property)
    {
    case (DWORD_PTR)DIPROP_RANGE:
    {
        const DIPROPRANGE *value = (const DIPROPRANGE *)params->header;
        properties->range_min = value->lMin;
        properties->range_max = value->lMax;
        return DIENUM_CONTINUE;
    }
    case (DWORD_PTR)DIPROP_DEADZONE:
    {
        const DIPROPDWORD *value = (const DIPROPDWORD *)params->header;
        properties->deadzone = value->dwData;
        return DIENUM_CONTINUE;
    }
    case (DWORD_PTR)DIPROP_SATURATION:
    {
        const DIPROPDWORD *value = (const DIPROPDWORD *)params->header;
        properties->saturation = value->dwData;
        return DIENUM_CONTINUE;
    }
    case (DWORD_PTR)DIPROP_CALIBRATIONMODE:
    {
        const DIPROPDWORD *value = (const DIPROPDWORD *)params->header;
        properties->calibration_mode = value->dwData;
        return DIENUM_CONTINUE;
    }
    }

    return DIENUM_STOP;
}

static BOOL CALLBACK reset_object_value( const DIDEVICEOBJECTINSTANCEW *instance, void *context )
{
    struct dinput_device *impl = context;
    struct object_properties *properties;
    LONG tmp = -1;

    if (!impl->object_properties) return DIENUM_STOP;
    properties = impl->object_properties + instance->dwOfs / sizeof(LONG);

    if (instance->dwType & DIDFT_AXIS)
    {
        if (!properties->range_min) tmp = properties->range_max / 2;
        else tmp = round( (properties->range_min + properties->range_max) / 2.0 );
    }

    *(LONG *)(impl->device_state + instance->dwOfs) = tmp;
    return DIENUM_CONTINUE;
}

static void reset_device_state( IDirectInputDevice8W *iface )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8W( iface );
    DIPROPHEADER filter =
    {
        .dwHeaderSize = sizeof(DIPROPHEADER),
        .dwSize = sizeof(DIPROPHEADER),
        .dwHow = DIPH_DEVICE,
        .dwObj = 0,
    };

    impl->vtbl->enum_objects( iface, &filter, DIDFT_AXIS | DIDFT_POV, reset_object_value, impl );
}

static HRESULT WINAPI dinput_device_set_property( IDirectInputDevice8W *iface, const GUID *guid,
                                                  const DIPROPHEADER *header )
{
    struct set_object_property_params params = {.iface = iface, .header = header, .property = LOWORD( guid )};
    struct dinput_device *impl = impl_from_IDirectInputDevice8W( iface );
    DWORD object_mask = DIDFT_AXIS | DIDFT_BUTTON | DIDFT_POV;
    DIDEVICEOBJECTINSTANCEW instance;
    DIPROPHEADER filter;
    HRESULT hr;

    filter = *header;
    if (FAILED(hr = enum_object_filter_init( impl, &filter ))) return hr;
    if (FAILED(hr = check_property( impl, guid, header, TRUE ))) return hr;

    switch (LOWORD( guid ))
    {
    case (DWORD_PTR)DIPROP_RANGE:
    case (DWORD_PTR)DIPROP_DEADZONE:
    case (DWORD_PTR)DIPROP_SATURATION:
    {
        hr = impl->vtbl->enum_objects( iface, &filter, DIDFT_AXIS, set_object_property, &params );
        if (FAILED(hr)) return hr;
        reset_device_state( iface );
        return DI_OK;
    }
    case (DWORD_PTR)DIPROP_CALIBRATIONMODE:
    {
        const DIPROPDWORD *value = (const DIPROPDWORD *)header;
        if (value->dwData > DIPROPCALIBRATIONMODE_RAW) return DIERR_INVALIDPARAM;
        hr = impl->vtbl->enum_objects( iface, &filter, DIDFT_AXIS, set_object_property, &params );
        if (FAILED(hr)) return hr;
        reset_device_state( iface );
        return DI_OK;
    }
    case (DWORD_PTR)DIPROP_AUTOCENTER:
    {
        const DIPROPDWORD *value = (const DIPROPDWORD *)header;
        if (!(impl->caps.dwFlags & DIDC_FORCEFEEDBACK)) return DIERR_UNSUPPORTED;

        FIXME( "DIPROP_AUTOCENTER stub!\n" );
        impl->autocenter = value->dwData;
        return DI_OK;
    }
    case (DWORD_PTR)DIPROP_FFGAIN:
    {
        const DIPROPDWORD *value = (const DIPROPDWORD *)header;
        if (!impl->vtbl->send_device_gain) return DIERR_UNSUPPORTED;
        impl->device_gain = value->dwData;
        if (!is_exclusively_acquired( impl )) return DI_OK;
        return impl->vtbl->send_device_gain( iface, impl->device_gain );
    }
    case (DWORD_PTR)DIPROP_AXISMODE:
    {
        const DIPROPDWORD *value = (const DIPROPDWORD *)header;

        TRACE( "Axis mode: %s\n", value->dwData == DIPROPAXISMODE_ABS ? "absolute" : "relative" );
        if (impl->user_format)
        {
            impl->user_format->dwFlags &= ~DIDFT_AXIS;
            impl->user_format->dwFlags |= value->dwData == DIPROPAXISMODE_ABS ? DIDF_ABSAXIS : DIDF_RELAXIS;
        }
        return DI_OK;
    }
    case (DWORD_PTR)DIPROP_BUFFERSIZE:
    {
        const DIPROPDWORD *value = (const DIPROPDWORD *)header;

        TRACE( "buffersize %lu\n", value->dwData );

        impl->buffersize = value->dwData;
        impl->queue_len = min( impl->buffersize, 1024 );
        free( impl->data_queue );

        impl->data_queue = impl->queue_len ? malloc( impl->queue_len * sizeof(DIDEVICEOBJECTDATA) ) : NULL;
        impl->queue_head = impl->queue_tail = impl->overflow = 0;
        return DI_OK;
    }
    case (DWORD_PTR)DIPROP_APPDATA:
    {
        const DIPROPPOINTER *value = (const DIPROPPOINTER *)header;
        int user_offset;
        hr = impl->vtbl->enum_objects( iface, &filter, object_mask, find_object, &instance );
        if (FAILED(hr)) return hr;
        if (hr == DIENUM_CONTINUE) return DIERR_OBJECTNOTFOUND;
        if ((user_offset = id_to_offset( impl, instance.dwType )) < 0) return DIERR_OBJECTNOTFOUND;
        if (!set_app_data( impl, user_offset, value->uData )) return DIERR_OUTOFMEMORY;
        return DI_OK;
    }
    default:
        FIXME( "Unknown property %s\n", debugstr_guid( guid ) );
        return DIERR_UNSUPPORTED;
    }

    return DI_OK;
}

static HRESULT WINAPI dinput_device_SetProperty( IDirectInputDevice8W *iface, const GUID *guid,
                                                 const DIPROPHEADER *header )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8W( iface );
    HRESULT hr;

    TRACE( "iface %p, guid %s, header %p\n", iface, debugstr_guid( guid ), header );

    if (!header) return DIERR_INVALIDPARAM;
    if (header->dwHeaderSize != sizeof(DIPROPHEADER)) return DIERR_INVALIDPARAM;
    if (!IS_DIPROP( guid )) return DI_OK;

    EnterCriticalSection( &impl->crit );
    hr = dinput_device_set_property( iface, guid, header );
    LeaveCriticalSection( &impl->crit );

    return hr;
}

static void dinput_device_set_username( struct dinput_device *impl, const DIPROPSTRING *value )
{
    struct DevicePlayer *device_player;
    BOOL found = FALSE;

    LIST_FOR_EACH_ENTRY( device_player, &impl->dinput->device_players, struct DevicePlayer, entry )
    {
        if (IsEqualGUID( &device_player->instance_guid, &impl->guid ))
        {
            found = TRUE;
            break;
        }
    }
    if (!found && (device_player = malloc( sizeof(struct DevicePlayer) )))
    {
        list_add_tail( &impl->dinput->device_players, &device_player->entry );
        device_player->instance_guid = impl->guid;
    }
    if (device_player)
        lstrcpynW( device_player->username, value->wsz, ARRAY_SIZE(device_player->username) );
}

static BOOL CALLBACK get_object_info( const DIDEVICEOBJECTINSTANCEW *instance, void *data )
{
    DIDEVICEOBJECTINSTANCEW *dest = data;
    DWORD size = dest->dwSize;

    memcpy( dest, instance, size );
    dest->dwSize = size;

    return DIENUM_STOP;
}

static HRESULT WINAPI dinput_device_GetObjectInfo( IDirectInputDevice8W *iface,
                                                   DIDEVICEOBJECTINSTANCEW *instance, DWORD obj, DWORD how )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8W( iface );
    DIPROPHEADER filter =
    {
        .dwSize = sizeof(filter),
        .dwHeaderSize = sizeof(filter),
        .dwHow = how,
        .dwObj = obj
    };
    HRESULT hr;

    TRACE( "iface %p, instance %p, obj %#lx, how %#lx.\n", iface, instance, obj, how );

    if (!instance) return E_POINTER;
    if (instance->dwSize != sizeof(DIDEVICEOBJECTINSTANCE_DX3W) && instance->dwSize != sizeof(DIDEVICEOBJECTINSTANCEW))
        return DIERR_INVALIDPARAM;
    if (how == DIPH_DEVICE) return DIERR_INVALIDPARAM;
    if (FAILED(hr = enum_object_filter_init( impl, &filter ))) return hr;

    hr = impl->vtbl->enum_objects( iface, &filter, DIDFT_ALL, get_object_info, instance );
    if (FAILED(hr)) return hr;
    if (hr == DIENUM_CONTINUE) return DIERR_NOTFOUND;
    return DI_OK;
}

static HRESULT WINAPI dinput_device_GetDeviceState( IDirectInputDevice8W *iface, DWORD size, void *data )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8W( iface );
    DIDATAFORMAT *device_format = impl->device_format, *user_format;
    DIOBJECTDATAFORMAT *device_obj, *user_obj;
    BYTE *user_state = data;
    DIPROPHEADER filter =
    {
        .dwSize = sizeof(filter),
        .dwHeaderSize = sizeof(filter),
        .dwHow = DIPH_DEVICE,
        .dwObj = 0,
    };
    HRESULT hr;

    TRACE( "iface %p, size %lu, data %p.\n", iface, size, data );

    if (!data) return DIERR_INVALIDPARAM;

    IDirectInputDevice2_Poll( iface );

    EnterCriticalSection( &impl->crit );
    if (impl->status == STATUS_UNPLUGGED)
        hr = DIERR_INPUTLOST;
    else if (impl->status != STATUS_ACQUIRED)
        hr = DIERR_NOTACQUIRED;
    else if (!(user_format = impl->user_format))
        hr = DIERR_INVALIDPARAM;
    else if (size != user_format->dwDataSize)
        hr = DIERR_INVALIDPARAM;
    else
    {
        memset( user_state, 0, size );

        user_obj = user_format->rgodf + device_format->dwNumObjs;
        device_obj = device_format->rgodf + device_format->dwNumObjs;
        while (user_obj-- > user_format->rgodf && device_obj-- > device_format->rgodf)
        {
            if (user_obj->dwType & DIDFT_BUTTON)
                user_state[user_obj->dwOfs] = impl->device_state[device_obj->dwOfs];
        }

        /* reset optional POVs to their default */
        user_obj = user_format->rgodf + user_format->dwNumObjs;
        while (user_obj-- > user_format->rgodf + device_format->dwNumObjs)
            if (user_obj->dwType & DIDFT_POV) *(ULONG *)(user_state + user_obj->dwOfs) = 0xffffffff;

        user_obj = user_format->rgodf + device_format->dwNumObjs;
        device_obj = device_format->rgodf + device_format->dwNumObjs;
        while (user_obj-- > user_format->rgodf && device_obj-- > device_format->rgodf)
        {
            if (user_obj->dwType & (DIDFT_POV | DIDFT_AXIS))
                *(ULONG *)(user_state + user_obj->dwOfs) = *(ULONG *)(impl->device_state + device_obj->dwOfs);
            if (!(user_format->dwFlags & DIDF_ABSAXIS) && (device_obj->dwType & DIDFT_RELAXIS))
                *(ULONG *)(impl->device_state + device_obj->dwOfs) = 0;
        }

        hr = DI_OK;
    }
    LeaveCriticalSection( &impl->crit );

    return hr;
}

static HRESULT WINAPI dinput_device_GetDeviceData( IDirectInputDevice8W *iface, DWORD size, DIDEVICEOBJECTDATA *data,
                                                   DWORD *count, DWORD flags )
{
    struct dinput_device *This = impl_from_IDirectInputDevice8W( iface );
    HRESULT ret = DI_OK;
    int len;

    TRACE( "iface %p, size %lu, data %p, count %p, flags %#lx.\n", iface, size, data, count, flags );

    if (This->dinput->dwVersion == 0x0800 || size == sizeof(DIDEVICEOBJECTDATA_DX3))
    {
        if (!This->queue_len) return DIERR_NOTBUFFERED;
        if (This->status == STATUS_UNPLUGGED) return DIERR_INPUTLOST;
        if (This->status != STATUS_ACQUIRED) return DIERR_NOTACQUIRED;
    }

    if (!This->queue_len)
        return DI_OK;
    if (size < sizeof(DIDEVICEOBJECTDATA_DX3)) return DIERR_INVALIDPARAM;

    IDirectInputDevice2_Poll(iface);
    EnterCriticalSection(&This->crit);

    len = This->queue_head - This->queue_tail;
    if (len < 0) len += This->queue_len;

    if ((*count != INFINITE) && (len > *count)) len = *count;

    if (data)
    {
        int i;
        for (i = 0; i < len; i++)
        {
            int n = (This->queue_tail + i) % This->queue_len;
            memcpy( (char *)data + size * i, This->data_queue + n, size );
        }
    }
    *count = len;

    if (This->overflow && This->dinput->dwVersion == 0x0800)
        ret = DI_BUFFEROVERFLOW;

    if (!(flags & DIGDD_PEEK))
    {
        /* Advance reading position */
        This->queue_tail = (This->queue_tail + len) % This->queue_len;
        This->overflow = FALSE;
    }

    LeaveCriticalSection(&This->crit);

    TRACE( "Returning %lu events queued\n", *count );
    return ret;
}

static HRESULT WINAPI dinput_device_RunControlPanel( IDirectInputDevice8W *iface, HWND hwnd, DWORD flags )
{
    FIXME( "iface %p, hwnd %p, flags %#lx stub!\n", iface, hwnd, flags );
    return DI_OK;
}

static HRESULT WINAPI dinput_device_Initialize( IDirectInputDevice8W *iface, HINSTANCE instance,
                                                DWORD version, const GUID *guid )
{
    FIXME( "iface %p, instance %p, version %#lx, guid %s stub!\n", iface, instance, version,
           debugstr_guid( guid ) );
    return DI_OK;
}

static HRESULT WINAPI dinput_device_CreateEffect( IDirectInputDevice8W *iface, const GUID *guid,
                                                  const DIEFFECT *params, IDirectInputEffect **out,
                                                  IUnknown *outer )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8W( iface );
    DWORD flags;
    HRESULT hr;

    TRACE( "iface %p, guid %s, params %p, out %p, outer %p\n", iface, debugstr_guid( guid ),
           params, out, outer );

    if (!out) return E_POINTER;
    *out = NULL;

    if (!(impl->caps.dwFlags & DIDC_FORCEFEEDBACK)) return DIERR_UNSUPPORTED;
    if (!impl->vtbl->create_effect) return DIERR_UNSUPPORTED;
    if (FAILED(hr = impl->vtbl->create_effect( iface, out ))) return hr;

    hr = IDirectInputEffect_Initialize( *out, DINPUT_instance, impl->dinput->dwVersion, guid );
    if (FAILED(hr)) goto failed;
    if (!params) return DI_OK;

    flags = params->dwSize == sizeof(DIEFFECT_DX6) ? DIEP_ALLPARAMS : DIEP_ALLPARAMS_DX5;
    if (!is_exclusively_acquired( impl )) flags |= DIEP_NODOWNLOAD;
    hr = IDirectInputEffect_SetParameters( *out, params, flags );
    if (FAILED(hr)) goto failed;
    return DI_OK;

failed:
    IDirectInputEffect_Release( *out );
    *out = NULL;
    return hr;
}

static HRESULT WINAPI dinput_device_EnumEffects( IDirectInputDevice8W *iface, LPDIENUMEFFECTSCALLBACKW callback,
                                                 void *context, DWORD type )
{
    DIEFFECTINFOW info = {.dwSize = sizeof(info)};
    HRESULT hr;

    TRACE( "iface %p, callback %p, context %p, type %#lx.\n", iface, callback, context, type );

    if (!callback) return DIERR_INVALIDPARAM;

    type = DIEFT_GETTYPE( type );

    if (type == DIEFT_ALL || type == DIEFT_CONSTANTFORCE)
    {
        hr = IDirectInputDevice8_GetEffectInfo( iface, &info, &GUID_ConstantForce );
        if (FAILED(hr) && hr != DIERR_DEVICENOTREG) return hr;
        if (hr == DI_OK && callback( &info, context ) == DIENUM_STOP) return DI_OK;
    }

    if (type == DIEFT_ALL || type == DIEFT_RAMPFORCE)
    {
        hr = IDirectInputDevice8_GetEffectInfo( iface, &info, &GUID_RampForce );
        if (FAILED(hr) && hr != DIERR_DEVICENOTREG) return hr;
        if (hr == DI_OK && callback( &info, context ) == DIENUM_STOP) return DI_OK;
    }

    if (type == DIEFT_ALL || type == DIEFT_PERIODIC)
    {
        hr = IDirectInputDevice8_GetEffectInfo( iface, &info, &GUID_Square );
        if (FAILED(hr) && hr != DIERR_DEVICENOTREG) return hr;
        if (hr == DI_OK && callback( &info, context ) == DIENUM_STOP) return DI_OK;

        hr = IDirectInputDevice8_GetEffectInfo( iface, &info, &GUID_Sine );
        if (FAILED(hr) && hr != DIERR_DEVICENOTREG) return hr;
        if (hr == DI_OK && callback( &info, context ) == DIENUM_STOP) return DI_OK;

        hr = IDirectInputDevice8_GetEffectInfo( iface, &info, &GUID_Triangle );
        if (FAILED(hr) && hr != DIERR_DEVICENOTREG) return hr;
        if (hr == DI_OK && callback( &info, context ) == DIENUM_STOP) return DI_OK;

        hr = IDirectInputDevice8_GetEffectInfo( iface, &info, &GUID_SawtoothUp );
        if (FAILED(hr) && hr != DIERR_DEVICENOTREG) return hr;
        if (hr == DI_OK && callback( &info, context ) == DIENUM_STOP) return DI_OK;

        hr = IDirectInputDevice8_GetEffectInfo( iface, &info, &GUID_SawtoothDown );
        if (FAILED(hr) && hr != DIERR_DEVICENOTREG) return hr;
        if (hr == DI_OK && callback( &info, context ) == DIENUM_STOP) return DI_OK;
    }

    if (type == DIEFT_ALL || type == DIEFT_CONDITION)
    {
        hr = IDirectInputDevice8_GetEffectInfo( iface, &info, &GUID_Spring );
        if (FAILED(hr) && hr != DIERR_DEVICENOTREG) return hr;
        if (hr == DI_OK && callback( &info, context ) == DIENUM_STOP) return DI_OK;

        hr = IDirectInputDevice8_GetEffectInfo( iface, &info, &GUID_Damper );
        if (FAILED(hr) && hr != DIERR_DEVICENOTREG) return hr;
        if (hr == DI_OK && callback( &info, context ) == DIENUM_STOP) return DI_OK;

        hr = IDirectInputDevice8_GetEffectInfo( iface, &info, &GUID_Inertia );
        if (FAILED(hr) && hr != DIERR_DEVICENOTREG) return hr;
        if (hr == DI_OK && callback( &info, context ) == DIENUM_STOP) return DI_OK;

        hr = IDirectInputDevice8_GetEffectInfo( iface, &info, &GUID_Friction );
        if (FAILED(hr) && hr != DIERR_DEVICENOTREG) return hr;
        if (hr == DI_OK && callback( &info, context ) == DIENUM_STOP) return DI_OK;
    }

    return DI_OK;
}

static HRESULT WINAPI dinput_device_GetEffectInfo( IDirectInputDevice8W *iface, DIEFFECTINFOW *info,
                                                   const GUID *guid )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8W( iface );

    TRACE( "iface %p, info %p, guid %s.\n", iface, info, debugstr_guid( guid ) );

    if (!info) return E_POINTER;
    if (info->dwSize != sizeof(DIEFFECTINFOW)) return DIERR_INVALIDPARAM;
    if (!(impl->caps.dwFlags & DIDC_FORCEFEEDBACK)) return DIERR_DEVICENOTREG;
    if (!impl->vtbl->get_effect_info) return DIERR_UNSUPPORTED;
    return impl->vtbl->get_effect_info( iface, info, guid );
}

static HRESULT WINAPI dinput_device_GetForceFeedbackState( IDirectInputDevice8W *iface, DWORD *out )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8W( iface );
    HRESULT hr = DI_OK;

    TRACE( "iface %p, out %p.\n", iface, out );

    if (!out) return E_POINTER;
    *out = 0;

    if (!(impl->caps.dwFlags & DIDC_FORCEFEEDBACK)) return DIERR_UNSUPPORTED;

    EnterCriticalSection( &impl->crit );
    if (!is_exclusively_acquired( impl )) hr = DIERR_NOTEXCLUSIVEACQUIRED;
    else *out = impl->force_feedback_state;
    LeaveCriticalSection( &impl->crit );

    return hr;
}

static HRESULT WINAPI dinput_device_SendForceFeedbackCommand( IDirectInputDevice8W *iface, DWORD command )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8W( iface );
    HRESULT hr;

    TRACE( "iface %p, command %#lx.\n", iface, command );

    switch (command)
    {
    case DISFFC_RESET: break;
    case DISFFC_STOPALL: break;
    case DISFFC_PAUSE: break;
    case DISFFC_CONTINUE: break;
    case DISFFC_SETACTUATORSON: break;
    case DISFFC_SETACTUATORSOFF: break;
    default: return DIERR_INVALIDPARAM;
    }

    if (!(impl->caps.dwFlags & DIDC_FORCEFEEDBACK)) return DIERR_UNSUPPORTED;
    if (!impl->vtbl->send_force_feedback_command) return DIERR_UNSUPPORTED;

    EnterCriticalSection( &impl->crit );
    if (!is_exclusively_acquired( impl )) hr = DIERR_NOTEXCLUSIVEACQUIRED;
    else hr = impl->vtbl->send_force_feedback_command( iface, command, FALSE );
    LeaveCriticalSection( &impl->crit );

    return hr;
}

static HRESULT WINAPI dinput_device_EnumCreatedEffectObjects( IDirectInputDevice8W *iface,
                                                              LPDIENUMCREATEDEFFECTOBJECTSCALLBACK callback,
                                                              void *context, DWORD flags )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8W( iface );

    TRACE( "iface %p, callback %p, context %p, flags %#lx.\n", iface, callback, context, flags );

    if (!callback) return DIERR_INVALIDPARAM;
    if (flags) return DIERR_INVALIDPARAM;
    if (!(impl->caps.dwFlags & DIDC_FORCEFEEDBACK)) return DI_OK;
    if (!impl->vtbl->enum_created_effect_objects) return DIERR_UNSUPPORTED;

    return impl->vtbl->enum_created_effect_objects( iface, callback, context, flags );
}

static HRESULT WINAPI dinput_device_Escape( IDirectInputDevice8W *iface, DIEFFESCAPE *escape )
{
    FIXME( "iface %p, escape %p stub!\n", iface, escape );
    return DI_OK;
}

static HRESULT WINAPI dinput_device_Poll( IDirectInputDevice8W *iface )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8W( iface );
    HRESULT hr = DI_NOEFFECT;

    EnterCriticalSection( &impl->crit );
    if (impl->status == STATUS_UNPLUGGED) hr = DIERR_INPUTLOST;
    else if (impl->status != STATUS_ACQUIRED) hr = DIERR_NOTACQUIRED;
    LeaveCriticalSection( &impl->crit );
    if (FAILED(hr)) return hr;

    if (impl->vtbl->poll) return impl->vtbl->poll( iface );
    return hr;
}

static HRESULT WINAPI dinput_device_SendDeviceData( IDirectInputDevice8W *iface, DWORD size,
                                                    const DIDEVICEOBJECTDATA *data, DWORD *count, DWORD flags )
{
    FIXME( "iface %p, size %lu, data %p, count %p, flags %#lx stub!\n", iface, size, data, count, flags );
    return DI_OK;
}

static HRESULT WINAPI dinput_device_EnumEffectsInFile( IDirectInputDevice8W *iface, const WCHAR *filename,
                                                       LPDIENUMEFFECTSINFILECALLBACK callback,
                                                       void *context, DWORD flags )
{
    FIXME( "iface %p, filename %s, callback %p, context %p, flags %#lx stub!\n", iface,
           debugstr_w(filename), callback, context, flags );
    return DI_OK;
}

static HRESULT WINAPI dinput_device_WriteEffectToFile( IDirectInputDevice8W *iface, const WCHAR *filename,
                                                       DWORD count, DIFILEEFFECT *effects, DWORD flags )
{
    FIXME( "iface %p, filename %s, count %lu, effects %p, flags %#lx stub!\n", iface,
           debugstr_w(filename), count, effects, flags );
    return DI_OK;
}

static HRESULT WINAPI dinput_device_BuildActionMap( IDirectInputDevice8W *iface, DIACTIONFORMATW *format,
                                                    const WCHAR *username, DWORD flags )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8W( iface );
    BOOL load_success = FALSE, has_actions = FALSE;
    DWORD genre, username_len = MAX_PATH;
    WCHAR username_buf[MAX_PATH];
    const DIDATAFORMAT *df;
    DWORD devMask;
    int i;

    FIXME( "iface %p, format %p, username %s, flags %#lx stub!\n", iface, format,
           debugstr_w(username), flags );

    if (!format) return DIERR_INVALIDPARAM;

    switch (GET_DIDEVICE_TYPE( impl->instance.dwDevType ))
    {
    case DIDEVTYPE_KEYBOARD:
    case DI8DEVTYPE_KEYBOARD:
        devMask = DIKEYBOARD_MASK;
        df = &c_dfDIKeyboard;
        break;
    case DIDEVTYPE_MOUSE:
    case DI8DEVTYPE_MOUSE:
        devMask = DIMOUSE_MASK;
        df = &c_dfDIMouse2;
        break;
    default:
        devMask = DIGENRE_ANY;
        df = impl->device_format;
        break;
    }

    /* Unless asked the contrary by these flags, try to load a previous mapping */
    if (!(flags & DIDBAM_HWDEFAULTS))
    {
        /* Retrieve logged user name if necessary */
        if (username == NULL) GetUserNameW( username_buf, &username_len );
        else lstrcpynW( username_buf, username, MAX_PATH );
        load_success = load_mapping_settings( impl, format, username_buf );
    }

    if (load_success) return DI_OK;

    for (i = 0; i < format->dwNumActions; i++)
    {
        /* Don't touch a user configured action */
        if (format->rgoAction[i].dwHow == DIAH_USERCONFIG) continue;

        genre = format->rgoAction[i].dwSemantic & DIGENRE_ANY;
        if (devMask == genre || (devMask == DIGENRE_ANY && genre != DIMOUSE_MASK && genre != DIKEYBOARD_MASK))
        {
            DWORD obj_id = semantic_to_obj_id( impl, format->rgoAction[i].dwSemantic );
            DWORD type = DIDFT_GETTYPE( obj_id );
            DWORD inst = DIDFT_GETINSTANCE( obj_id );

            LPDIOBJECTDATAFORMAT odf;

            if (type == DIDFT_PSHBUTTON) type = DIDFT_BUTTON;
            if (type == DIDFT_RELAXIS) type = DIDFT_AXIS;

            /* Make sure the object exists */
            odf = dataformat_to_odf_by_type( df, inst, type );

            if (odf != NULL)
            {
                format->rgoAction[i].dwObjID = obj_id;
                format->rgoAction[i].guidInstance = impl->guid;
                format->rgoAction[i].dwHow = DIAH_DEFAULT;
                has_actions = TRUE;
            }
        }
        else if (!(flags & DIDBAM_PRESERVE))
        {
            /* We must clear action data belonging to other devices */
            memset( &format->rgoAction[i].guidInstance, 0, sizeof(GUID) );
            format->rgoAction[i].dwHow = DIAH_UNMAPPED;
        }
    }

    if (!has_actions) return DI_NOEFFECT;
    if (flags & (DIDBAM_DEFAULT|DIDBAM_PRESERVE|DIDBAM_INITIALIZE|DIDBAM_HWDEFAULTS))
        FIXME( "Unimplemented flags %#lx\n", flags );
    return DI_OK;
}

static HRESULT WINAPI dinput_device_SetActionMap( IDirectInputDevice8W *iface, DIACTIONFORMATW *format,
                                                  const WCHAR *username, DWORD flags )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8W( iface );
    DIDATAFORMAT data_format;
    DIOBJECTDATAFORMAT *obj_df = NULL;
    DIPROPDWORD dp;
    DIPROPRANGE dpr;
    DIPROPSTRING dps;
    WCHAR username_buf[MAX_PATH];
    DWORD username_len = MAX_PATH;
    int i, action = 0, num_actions = 0;
    unsigned int offset = 0;
    const DIDATAFORMAT *df;
    ActionMap *action_map;

    FIXME( "iface %p, format %p, username %s, flags %#lx stub!\n", iface, format,
           debugstr_w(username), flags );

    if (!format) return DIERR_INVALIDPARAM;

    switch (GET_DIDEVICE_TYPE( impl->instance.dwDevType ))
    {
    case DIDEVTYPE_KEYBOARD:
    case DI8DEVTYPE_KEYBOARD:
        df = &c_dfDIKeyboard;
        break;
    case DIDEVTYPE_MOUSE:
    case DI8DEVTYPE_MOUSE:
        df = &c_dfDIMouse2;
        break;
    default:
        df = impl->device_format;
        break;
    }

    if (impl->status == STATUS_ACQUIRED) return DIERR_ACQUIRED;

    data_format.dwSize = sizeof(data_format);
    data_format.dwObjSize = sizeof(DIOBJECTDATAFORMAT);
    data_format.dwFlags = DIDF_RELAXIS;
    data_format.dwDataSize = format->dwDataSize;

    /* Count the actions */
    for (i = 0; i < format->dwNumActions; i++)
        if (IsEqualGUID( &impl->guid, &format->rgoAction[i].guidInstance ))
            num_actions++;

    if (num_actions == 0) return DI_NOEFFECT;

    /* Construct the dataformat and actionmap */
    obj_df = malloc( sizeof(DIOBJECTDATAFORMAT) * num_actions );
    data_format.rgodf = (LPDIOBJECTDATAFORMAT)obj_df;
    data_format.dwNumObjs = num_actions;

    action_map = malloc( sizeof(ActionMap) * num_actions );

    for (i = 0; i < format->dwNumActions; i++)
    {
        if (IsEqualGUID( &impl->guid, &format->rgoAction[i].guidInstance ))
        {
            DWORD inst = DIDFT_GETINSTANCE( format->rgoAction[i].dwObjID );
            DWORD type = DIDFT_GETTYPE( format->rgoAction[i].dwObjID );
            LPDIOBJECTDATAFORMAT obj;

            if (type == DIDFT_PSHBUTTON) type = DIDFT_BUTTON;
            if (type == DIDFT_RELAXIS) type = DIDFT_AXIS;

            obj = dataformat_to_odf_by_type( df, inst, type );

            memcpy( &obj_df[action], obj, df->dwObjSize );

            action_map[action].uAppData = format->rgoAction[i].uAppData;
            action_map[action].offset = offset;
            obj_df[action].dwOfs = offset;
            offset += (type & DIDFT_BUTTON) ? 1 : 4;

            action++;
        }
    }

    IDirectInputDevice8_SetDataFormat( iface, &data_format );

    impl->action_map = action_map;
    impl->num_actions = num_actions;

    free( obj_df );

    /* Set the device properties according to the action format */
    dpr.diph.dwSize = sizeof(DIPROPRANGE);
    dpr.lMin = format->lAxisMin;
    dpr.lMax = format->lAxisMax;
    dpr.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    dpr.diph.dwObj = 0;
    dpr.diph.dwHow = DIPH_DEVICE;
    IDirectInputDevice8_SetProperty( iface, DIPROP_RANGE, &dpr.diph );

    if (format->dwBufferSize > 0)
    {
        dp.diph.dwSize = sizeof(DIPROPDWORD);
        dp.dwData = format->dwBufferSize;
        dp.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        dp.diph.dwObj = 0;
        dp.diph.dwHow = DIPH_DEVICE;
        IDirectInputDevice8_SetProperty( iface, DIPROP_BUFFERSIZE, &dp.diph );
    }

    /* Retrieve logged user name if necessary */
    if (username == NULL) GetUserNameW( username_buf, &username_len );
    else lstrcpynW( username_buf, username, MAX_PATH );

    dps.diph.dwSize = sizeof(dps);
    dps.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    dps.diph.dwObj = 0;
    dps.diph.dwHow = DIPH_DEVICE;
    if (flags & DIDSAM_NOUSER) dps.wsz[0] = '\0';
    else lstrcpynW( dps.wsz, username_buf, ARRAY_SIZE(dps.wsz) );
    dinput_device_set_username( impl, &dps );

    /* Save the settings to disk */
    save_mapping_settings( iface, format, username_buf );

    return DI_OK;
}

static HRESULT WINAPI dinput_device_GetImageInfo( IDirectInputDevice8W *iface, DIDEVICEIMAGEINFOHEADERW *header )
{
    FIXME( "iface %p, header %p stub!\n", iface, header );
    return DI_OK;
}

extern const IDirectInputDevice8AVtbl dinput_device_a_vtbl;
static const IDirectInputDevice8WVtbl dinput_device_w_vtbl =
{
    /*** IUnknown methods ***/
    dinput_device_QueryInterface,
    dinput_device_AddRef,
    dinput_device_Release,
    /*** IDirectInputDevice methods ***/
    dinput_device_GetCapabilities,
    dinput_device_EnumObjects,
    dinput_device_GetProperty,
    dinput_device_SetProperty,
    dinput_device_Acquire,
    dinput_device_Unacquire,
    dinput_device_GetDeviceState,
    dinput_device_GetDeviceData,
    dinput_device_SetDataFormat,
    dinput_device_SetEventNotification,
    dinput_device_SetCooperativeLevel,
    dinput_device_GetObjectInfo,
    dinput_device_GetDeviceInfo,
    dinput_device_RunControlPanel,
    dinput_device_Initialize,
    /*** IDirectInputDevice2 methods ***/
    dinput_device_CreateEffect,
    dinput_device_EnumEffects,
    dinput_device_GetEffectInfo,
    dinput_device_GetForceFeedbackState,
    dinput_device_SendForceFeedbackCommand,
    dinput_device_EnumCreatedEffectObjects,
    dinput_device_Escape,
    dinput_device_Poll,
    dinput_device_SendDeviceData,
    /*** IDirectInputDevice7 methods ***/
    dinput_device_EnumEffectsInFile,
    dinput_device_WriteEffectToFile,
    /*** IDirectInputDevice8 methods ***/
    dinput_device_BuildActionMap,
    dinput_device_SetActionMap,
    dinput_device_GetImageInfo,
};

HRESULT dinput_device_alloc( SIZE_T size, const struct dinput_device_vtbl *vtbl, const GUID *guid,
                             struct dinput *dinput, void **out )
{
    struct dinput_device *This;
    DIDATAFORMAT *format;

    if (!(This = calloc( 1, size ))) return DIERR_OUTOFMEMORY;
    if (!(format = calloc( 1, sizeof(*format) )))
    {
        free( This );
        return DIERR_OUTOFMEMORY;
    }

    This->IDirectInputDevice8A_iface.lpVtbl = &dinput_device_a_vtbl;
    This->IDirectInputDevice8W_iface.lpVtbl = &dinput_device_w_vtbl;
    This->ref = 1;
    This->guid = *guid;
    This->instance.dwSize = sizeof(DIDEVICEINSTANCEW);
    This->caps.dwSize = sizeof(DIDEVCAPS);
    This->caps.dwFlags = DIDC_ATTACHED | DIDC_EMULATED;
    This->device_format = format;
    This->device_gain = 10000;
    This->force_feedback_state = DIGFFS_STOPPED | DIGFFS_EMPTY;
    InitializeCriticalSection( &This->crit );
    This->dinput = dinput;
    IDirectInput_AddRef( &dinput->IDirectInput7A_iface );
    This->vtbl = vtbl;

    *out = This;
    return DI_OK;
}

static const GUID *object_instance_guid( const DIDEVICEOBJECTINSTANCEW *instance )
{
    if (IsEqualGUID( &instance->guidType, &GUID_XAxis )) return &GUID_XAxis;
    if (IsEqualGUID( &instance->guidType, &GUID_YAxis )) return &GUID_YAxis;
    if (IsEqualGUID( &instance->guidType, &GUID_ZAxis )) return &GUID_ZAxis;
    if (IsEqualGUID( &instance->guidType, &GUID_RxAxis )) return &GUID_RxAxis;
    if (IsEqualGUID( &instance->guidType, &GUID_RyAxis )) return &GUID_RyAxis;
    if (IsEqualGUID( &instance->guidType, &GUID_RzAxis )) return &GUID_RzAxis;
    if (IsEqualGUID( &instance->guidType, &GUID_Slider )) return &GUID_Slider;
    if (IsEqualGUID( &instance->guidType, &GUID_Button )) return &GUID_Button;
    if (IsEqualGUID( &instance->guidType, &GUID_Key )) return &GUID_Key;
    if (IsEqualGUID( &instance->guidType, &GUID_POV )) return &GUID_POV;
    return &GUID_Unknown;
}

static BOOL CALLBACK enum_objects_init( const DIDEVICEOBJECTINSTANCEW *instance, void *data )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8W( data );
    DIDATAFORMAT *format = impl->device_format;
    DIOBJECTDATAFORMAT *obj_format;

    if (!format->rgodf)
    {
        format->dwDataSize = max( format->dwDataSize, instance->dwOfs + sizeof(LONG) );
        if (instance->dwType & DIDFT_BUTTON) impl->caps.dwButtons++;
        if (instance->dwType & DIDFT_AXIS) impl->caps.dwAxes++;
        if (instance->dwType & DIDFT_POV) impl->caps.dwPOVs++;
        if (instance->dwType & (DIDFT_BUTTON|DIDFT_AXIS|DIDFT_POV))
        {
            if (!impl->device_state_report_id)
                impl->device_state_report_id = instance->wReportId;
            else if (impl->device_state_report_id != instance->wReportId)
                FIXME( "multiple device state reports found!\n" );
        }
    }
    else
    {
        obj_format = format->rgodf + format->dwNumObjs;
        obj_format->pguid = object_instance_guid( instance );
        obj_format->dwOfs = instance->dwOfs;
        obj_format->dwType = instance->dwType;
        obj_format->dwFlags = instance->dwFlags;
    }

    if (impl->object_properties && (instance->dwType & (DIDFT_AXIS | DIDFT_POV)))
        reset_object_value( instance, impl );

    format->dwNumObjs++;
    return DIENUM_CONTINUE;
}

HRESULT dinput_device_init( IDirectInputDevice8W *iface )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8W( iface );
    DIDATAFORMAT *format = impl->device_format;
    ULONG i, size;

    IDirectInputDevice8_EnumObjects( iface, enum_objects_init, iface, DIDFT_ALL );
    if (format->dwDataSize > DEVICE_STATE_MAX_SIZE)
    {
        FIXME( "unable to create device, state is too large\n" );
        return DIERR_OUTOFMEMORY;
    }

    size = format->dwNumObjs * sizeof(*format->rgodf);
    if (!(format->rgodf = calloc( 1, size ))) return DIERR_OUTOFMEMORY;

    format->dwSize = sizeof(*format);
    format->dwObjSize = sizeof(*format->rgodf);
    format->dwFlags = DIDF_ABSAXIS;
    format->dwNumObjs = 0;
    IDirectInputDevice8_EnumObjects( iface, enum_objects_init, iface, DIDFT_ALL );

    if (TRACE_ON( dinput ))
    {
        TRACE( "device format %s\n", debugstr_didataformat( format ) );
        for (i = 0; i < format->dwNumObjs; ++i) TRACE( "  %lu: object %s\n", i, debugstr_diobjectdataformat( format->rgodf + i ) );
    }

    return DI_OK;
}
