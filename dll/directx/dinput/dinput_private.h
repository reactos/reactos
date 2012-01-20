/*
 * Copyright 2000 Lionel Ulmer
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

#ifndef __WINE_DLLS_DINPUT_DINPUT_PRIVATE_H
#define __WINE_DLLS_DINPUT_DINPUT_PRIVATE_H

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "dinput.h"
#include "wine/list.h"

/* Implementation specification */
typedef struct IDirectInputImpl IDirectInputImpl;
struct IDirectInputImpl
{
    const IDirectInput7AVtbl   *lpVtbl;
    const IDirectInput7WVtbl   *lpVtbl7w;
    const IDirectInput8AVtbl   *lpVtbl8a;
    const IDirectInput8WVtbl   *lpVtbl8w;

    LONG                        ref;

    CRITICAL_SECTION            crit;
    struct list                 entry;          /* entry into list of all IDirectInputs */

    DWORD                       evsequence;     /* unique sequence number for events */
    DWORD                       dwVersion;      /* direct input version number */
    struct list                 devices_list;   /* list of all created dinput devices */
};

/* Function called by all devices that Wine supports */
struct dinput_device {
    const char *name;
    BOOL (*enum_deviceA)(DWORD dwDevType, DWORD dwFlags, LPDIDEVICEINSTANCEA lpddi, DWORD version, int id);
    BOOL (*enum_deviceW)(DWORD dwDevType, DWORD dwFlags, LPDIDEVICEINSTANCEW lpddi, DWORD version, int id);
    HRESULT (*create_deviceA)(IDirectInputImpl *dinput, REFGUID rguid, REFIID riid, LPDIRECTINPUTDEVICEA* pdev);
    HRESULT (*create_deviceW)(IDirectInputImpl *dinput, REFGUID rguid, REFIID riid, LPDIRECTINPUTDEVICEW* pdev);
};

extern const struct dinput_device mouse_device;
extern const struct dinput_device keyboard_device;
extern const struct dinput_device joystick_linux_device;
extern const struct dinput_device joystick_linuxinput_device;

extern void check_dinput_hooks(LPDIRECTINPUTDEVICE8A);
typedef void (*DI_EVENT_PROC)(LPDIRECTINPUTDEVICE8A, WPARAM, LPARAM);

extern void _dump_diactionformatA(LPDIACTIONFORMATA);

#endif /* __WINE_DLLS_DINPUT_DINPUT_PRIVATE_H */
