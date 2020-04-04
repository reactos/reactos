/*
 * Copyright 2009, Aric Stewart, CodeWeavers
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

#ifndef __WINE_DLLS_DINPUT_JOYSTICK_PRIVATE_H
#define __WINE_DLLS_DINPUT_JOYSTICK_PRIVATE_H

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "dinput.h"
#include "wine/list.h"
#include "wine/unicode.h"
#include "dinput_private.h"
#include "device_private.h"

/* Number of objects in the default data format */
#define MAX_PROPS 164
struct JoystickGenericImpl;

/* Number of buttons for which to allow remapping */
#define MAX_MAP_BUTTONS 32

typedef void joy_polldev_handler(LPDIRECTINPUTDEVICE8A iface);

typedef struct JoystickGenericImpl
{
    struct IDirectInputDeviceImpl base;

    ObjProps    props[MAX_PROPS];
    DIDEVCAPS   devcaps;
    DIJOYSTATE2 js;     /* wine data */
    GUID        guidProduct;
    GUID        guidInstance;
    char        *name;
    int         device_axis_count;      /* Total number of axes in the device */
    int        *axis_map;               /* User axes remapping */
    int         button_map[MAX_MAP_BUTTONS]; /* User button remapping */
    LONG        deadzone;               /* Default dead-zone */

    joy_polldev_handler *joy_polldev;
} JoystickGenericImpl;

LONG joystick_map_axis(ObjProps *props, int val) DECLSPEC_HIDDEN;
HRESULT setup_dinput_options(JoystickGenericImpl *This, const int *default_axis_map) DECLSPEC_HIDDEN;

DWORD joystick_map_pov(const POINTL *p) DECLSPEC_HIDDEN;

BOOL device_disabled_registry(const char* name) DECLSPEC_HIDDEN;

HRESULT WINAPI JoystickWGenericImpl_GetObjectInfo(LPDIRECTINPUTDEVICE8W iface,
        LPDIDEVICEOBJECTINSTANCEW pdidoi, DWORD dwObj, DWORD dwHow) DECLSPEC_HIDDEN;

HRESULT WINAPI JoystickAGenericImpl_GetObjectInfo(LPDIRECTINPUTDEVICE8A iface,
        LPDIDEVICEOBJECTINSTANCEA pdidoi, DWORD dwObj, DWORD dwHow) DECLSPEC_HIDDEN;

HRESULT WINAPI JoystickAGenericImpl_GetProperty(LPDIRECTINPUTDEVICE8A iface, REFGUID rguid, LPDIPROPHEADER pdiph) DECLSPEC_HIDDEN;
HRESULT WINAPI JoystickWGenericImpl_GetProperty(LPDIRECTINPUTDEVICE8W iface, REFGUID rguid, LPDIPROPHEADER pdiph) DECLSPEC_HIDDEN;

HRESULT WINAPI JoystickAGenericImpl_GetCapabilities(LPDIRECTINPUTDEVICE8A iface, LPDIDEVCAPS lpDIDevCaps) DECLSPEC_HIDDEN;
HRESULT WINAPI JoystickWGenericImpl_GetCapabilities(LPDIRECTINPUTDEVICE8W iface, LPDIDEVCAPS lpDIDevCaps) DECLSPEC_HIDDEN;

void _dump_DIDEVCAPS(const DIDEVCAPS *lpDIDevCaps) DECLSPEC_HIDDEN;

HRESULT WINAPI JoystickAGenericImpl_SetProperty(LPDIRECTINPUTDEVICE8A iface, REFGUID rguid, LPCDIPROPHEADER ph) DECLSPEC_HIDDEN;
HRESULT WINAPI JoystickWGenericImpl_SetProperty(LPDIRECTINPUTDEVICE8W iface, REFGUID rguid, LPCDIPROPHEADER ph) DECLSPEC_HIDDEN;

HRESULT WINAPI JoystickAGenericImpl_GetDeviceInfo( LPDIRECTINPUTDEVICE8A iface,
    LPDIDEVICEINSTANCEA pdidi) DECLSPEC_HIDDEN;

HRESULT WINAPI JoystickWGenericImpl_GetDeviceInfo( LPDIRECTINPUTDEVICE8W iface,
    LPDIDEVICEINSTANCEW pdidi) DECLSPEC_HIDDEN;

HRESULT WINAPI JoystickAGenericImpl_Poll(LPDIRECTINPUTDEVICE8A iface) DECLSPEC_HIDDEN;
HRESULT WINAPI JoystickWGenericImpl_Poll(LPDIRECTINPUTDEVICE8W iface) DECLSPEC_HIDDEN;

HRESULT WINAPI JoystickAGenericImpl_GetDeviceState(LPDIRECTINPUTDEVICE8A iface, DWORD len, LPVOID ptr) DECLSPEC_HIDDEN;
HRESULT WINAPI JoystickWGenericImpl_GetDeviceState(LPDIRECTINPUTDEVICE8W iface, DWORD len, LPVOID ptr) DECLSPEC_HIDDEN;

HRESULT WINAPI JoystickAGenericImpl_BuildActionMap(LPDIRECTINPUTDEVICE8A iface, LPDIACTIONFORMATA lpdiaf, LPCSTR lpszUserName, DWORD dwFlags) DECLSPEC_HIDDEN;
HRESULT WINAPI JoystickWGenericImpl_BuildActionMap(LPDIRECTINPUTDEVICE8W iface, LPDIACTIONFORMATW lpdiaf, LPCWSTR lpszUserName, DWORD dwFlags) DECLSPEC_HIDDEN;

HRESULT WINAPI JoystickAGenericImpl_SetActionMap(LPDIRECTINPUTDEVICE8A iface, LPDIACTIONFORMATA lpdiaf, LPCSTR lpszUserName, DWORD dwFlags) DECLSPEC_HIDDEN;
HRESULT WINAPI JoystickWGenericImpl_SetActionMap(LPDIRECTINPUTDEVICE8W iface, LPDIACTIONFORMATW lpdiaf, LPCWSTR lpszUserName, DWORD dwFlags) DECLSPEC_HIDDEN;

DWORD typeFromGUID(REFGUID guid) DECLSPEC_HIDDEN;
void dump_DIEFFECT(LPCDIEFFECT eff, REFGUID guid, DWORD dwFlags) DECLSPEC_HIDDEN;
BOOL is_xinput_device(const DIDEVCAPS *devcaps, WORD vid, WORD pid) DECLSPEC_HIDDEN;

#endif /* __WINE_DLLS_DINPUT_JOYSTICK_PRIVATE_H */
