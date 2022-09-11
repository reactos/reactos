/*		DirectInput Keyboard device
 *
 * Copyright 1998 Marcus Meissner
 * Copyright 1998,1999 Lionel Ulmer
 * Copyright 2000-2001 TransGaming Technologies Inc.
 * Copyright 2005 Raphael Junqueira
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
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winerror.h"
#include "dinput.h"

#include "dinput_private.h"
#include "device_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dinput);

static const struct dinput_device_vtbl keyboard_vtbl;

struct keyboard
{
    struct dinput_device base;
};

static inline struct keyboard *impl_from_IDirectInputDevice8W( IDirectInputDevice8W *iface )
{
    return CONTAINING_RECORD( CONTAINING_RECORD( iface, struct dinput_device, IDirectInputDevice8W_iface ),
                              struct keyboard, base );
}

static BYTE map_dik_code(DWORD scanCode, DWORD vkCode, DWORD subType, DWORD version)
{
    if (!scanCode && version < 0x0800)
        scanCode = MapVirtualKeyW(vkCode, MAPVK_VK_TO_VSC);

    if (subType == DIDEVTYPEKEYBOARD_JAPAN106)
    {
        switch (scanCode)
        {
        case 0x0d: /* ^ */
            scanCode = DIK_CIRCUMFLEX;
            break;
        case 0x1a: /* @ */
            scanCode = DIK_AT;
            break;
        case 0x1b: /* [ */
            scanCode = DIK_LBRACKET;
            break;
        case 0x28: /* : */
            scanCode = DIK_COLON;
            break;
        case 0x29: /* Hankaku/Zenkaku */
            scanCode = DIK_KANJI;
            break;
        case 0x2b: /* ] */
            scanCode = DIK_RBRACKET;
            break;
        case 0x73: /* \ */
            scanCode = DIK_BACKSLASH;
            break;
        }
    }
    if (scanCode & 0x100) scanCode |= 0x80;
    return (BYTE)scanCode;
}

int dinput_keyboard_hook( IDirectInputDevice8W *iface, WPARAM wparam, LPARAM lparam )
{
    struct keyboard *impl = impl_from_IDirectInputDevice8W( iface );
    BYTE new_diks, subtype = GET_DIDEVICE_SUBTYPE( impl->base.instance.dwDevType );
    int dik_code, ret = impl->base.dwCoopLevel & DISCL_EXCLUSIVE;
    KBDLLHOOKSTRUCT *hook = (KBDLLHOOKSTRUCT *)lparam;
    DWORD scan_code;

    if (wparam != WM_KEYDOWN && wparam != WM_KEYUP &&
        wparam != WM_SYSKEYDOWN && wparam != WM_SYSKEYUP)
        return 0;

    TRACE( "iface %p, wparam %#Ix, lparam %#Ix, vkCode %#lx, scanCode %#lx.\n", iface, wparam,
           lparam, hook->vkCode, hook->scanCode );

    switch (hook->vkCode)
    {
        /* R-Shift is special - it is an extended key with separate scan code */
        case VK_RSHIFT  : dik_code = DIK_RSHIFT; break;
        case VK_PAUSE   : dik_code = DIK_PAUSE; break;
        case VK_NUMLOCK : dik_code = DIK_NUMLOCK; break;
        case VK_SUBTRACT: dik_code = DIK_SUBTRACT; break;
        default:
            scan_code = hook->scanCode & 0xff;
            if (hook->flags & LLKHF_EXTENDED) scan_code |= 0x100;
            dik_code = map_dik_code( scan_code, hook->vkCode, subtype, impl->base.dinput->dwVersion );
    }
    new_diks = hook->flags & LLKHF_UP ? 0 : 0x80;

    /* returns now if key event already known */
    if (new_diks == impl->base.device_state[dik_code]) return ret;

    impl->base.device_state[dik_code] = new_diks;
    TRACE( " setting key %02x to %02x\n", dik_code, impl->base.device_state[dik_code] );

    EnterCriticalSection( &impl->base.crit );
    queue_event( iface, DIDFT_MAKEINSTANCE( dik_code ) | DIDFT_PSHBUTTON, new_diks,
                 GetCurrentTime(), impl->base.dinput->evsequence++ );
    if (impl->base.hEvent) SetEvent( impl->base.hEvent );
    LeaveCriticalSection( &impl->base.crit );

    return ret;
}

static DWORD get_keyboard_subtype(void)
{
    INT kbd_type, kbd_subtype, dev_subtype;
    kbd_type = GetKeyboardType(0);
    kbd_subtype = GetKeyboardType(1);

    if (kbd_type == 4 || (kbd_type == 7 && kbd_subtype == 0))
        dev_subtype = DIDEVTYPEKEYBOARD_PCENH;
    else if (kbd_type == 7 && kbd_subtype == 2)
        dev_subtype = DIDEVTYPEKEYBOARD_JAPAN106;
    else {
        FIXME( "Unknown keyboard type %d, subtype %d\n", kbd_type, kbd_subtype );
        dev_subtype = DIDEVTYPEKEYBOARD_PCENH;
    }
    return dev_subtype;
}

HRESULT keyboard_enum_device( DWORD type, DWORD flags, DIDEVICEINSTANCEW *instance, DWORD version )
{
    BYTE subtype = get_keyboard_subtype();
    DWORD size;

    TRACE( "type %#lx, flags %#lx, instance %p, version %#lx.\n", type, flags, instance, version );

    size = instance->dwSize;
    memset( instance, 0, size );
    instance->dwSize = size;
    instance->guidInstance = GUID_SysKeyboard;
    instance->guidProduct = GUID_SysKeyboard;
    if (version >= 0x0800) instance->dwDevType = DI8DEVTYPE_KEYBOARD | (subtype << 8);
    else instance->dwDevType = DIDEVTYPE_KEYBOARD | (subtype << 8);
    MultiByteToWideChar( CP_ACP, 0, "Keyboard", -1, instance->tszInstanceName, MAX_PATH );
    MultiByteToWideChar( CP_ACP, 0, "Wine Keyboard", -1, instance->tszProductName, MAX_PATH );

    return DI_OK;
}

HRESULT keyboard_create_device( struct dinput *dinput, const GUID *guid, IDirectInputDevice8W **out )
{
    struct keyboard *impl;
    HRESULT hr;

    TRACE( "dinput %p, guid %s, out %p.\n", dinput, debugstr_guid( guid ), out );

    *out = NULL;
    if (!IsEqualGUID( &GUID_SysKeyboard, guid )) return DIERR_DEVICENOTREG;

    if (FAILED(hr = dinput_device_alloc( sizeof(struct keyboard), &keyboard_vtbl, guid, dinput, (void **)&impl )))
        return hr;
    impl->base.crit.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": struct keyboard*->base.crit");

    keyboard_enum_device( 0, 0, &impl->base.instance, dinput->dwVersion );
    impl->base.caps.dwDevType = impl->base.instance.dwDevType;
    impl->base.caps.dwFirmwareRevision = 100;
    impl->base.caps.dwHardwareRevision = 100;

    if (FAILED(hr = dinput_device_init( &impl->base.IDirectInputDevice8W_iface )))
    {
        IDirectInputDevice_Release( &impl->base.IDirectInputDevice8W_iface );
        return hr;
    }

    *out = &impl->base.IDirectInputDevice8W_iface;
    return DI_OK;
}

static HRESULT keyboard_poll( IDirectInputDevice8W *iface )
{
    check_dinput_events();
    return DI_OK;
}

static HRESULT keyboard_acquire( IDirectInputDevice8W *iface )
{
    return DI_OK;
}

static HRESULT keyboard_unacquire( IDirectInputDevice8W *iface )
{
    struct keyboard *impl = impl_from_IDirectInputDevice8W( iface );
    memset( impl->base.device_state, 0, sizeof(impl->base.device_state) );
    return DI_OK;
}

static BOOL try_enum_object( const DIPROPHEADER *filter, DWORD flags, LPDIENUMDEVICEOBJECTSCALLBACKW callback,
                             DIDEVICEOBJECTINSTANCEW *instance, void *data )
{
    if (flags != DIDFT_ALL && !(flags & DIDFT_GETTYPE( instance->dwType ))) return DIENUM_CONTINUE;

    switch (filter->dwHow)
    {
    case DIPH_DEVICE:
        return callback( instance, data );
    case DIPH_BYOFFSET:
        if (filter->dwObj != instance->dwOfs) return DIENUM_CONTINUE;
        return callback( instance, data );
    case DIPH_BYID:
        if ((filter->dwObj & 0x00ffffff) != (instance->dwType & 0x00ffffff)) return DIENUM_CONTINUE;
        return callback( instance, data );
    }

    return DIENUM_CONTINUE;
}

static HRESULT keyboard_enum_objects( IDirectInputDevice8W *iface, const DIPROPHEADER *filter,
                                      DWORD flags, LPDIENUMDEVICEOBJECTSCALLBACKW callback, void *context )
{
    struct keyboard *impl = impl_from_IDirectInputDevice8W( iface );
    BYTE subtype = GET_DIDEVICE_SUBTYPE( impl->base.instance.dwDevType );
    DIDEVICEOBJECTINSTANCEW instance =
    {
        .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
        .guidType = GUID_Key,
        .dwOfs = DIK_ESCAPE,
        .dwType = DIDFT_PSHBUTTON | DIDFT_MAKEINSTANCE( DIK_ESCAPE ),
    };
    DWORD i, dik;
    BOOL ret;

    for (i = 0; i < 512; ++i)
    {
        if (!GetKeyNameTextW( i << 16, instance.tszName, ARRAY_SIZE(instance.tszName) )) continue;
        if (!(dik = map_dik_code( i, 0, subtype, impl->base.dinput->dwVersion ))) continue;
        instance.dwOfs = dik;
        instance.dwType = DIDFT_PSHBUTTON | DIDFT_MAKEINSTANCE( dik );
        ret = try_enum_object( filter, flags, callback, &instance, context );
        if (ret != DIENUM_CONTINUE) return DIENUM_STOP;
    }

    return DIENUM_CONTINUE;
}

static HRESULT keyboard_get_property( IDirectInputDevice8W *iface, DWORD property,
                                      DIPROPHEADER *header, const DIDEVICEOBJECTINSTANCEW *instance )
{
    switch (property)
    {
    case (DWORD_PTR)DIPROP_KEYNAME:
    {
        DIPROPSTRING *value = (DIPROPSTRING *)header;
        lstrcpynW( value->wsz, instance->tszName, ARRAY_SIZE(value->wsz) );
        return DI_OK;
    }
    }
    return DIERR_UNSUPPORTED;
}

static const struct dinput_device_vtbl keyboard_vtbl =
{
    NULL,
    keyboard_poll,
    NULL,
    keyboard_acquire,
    keyboard_unacquire,
    keyboard_enum_objects,
    keyboard_get_property,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
};
