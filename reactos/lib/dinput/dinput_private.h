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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_DLLS_DINPUT_DINPUT_PRIVATE_H
#define __WINE_DLLS_DINPUT_DINPUT_PRIVATE_H

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "dinput.h"

/* Implementation specification */
typedef struct IDirectInputImpl IDirectInputImpl;
struct IDirectInputImpl
{
   LPVOID lpVtbl;
   DWORD  ref;

   /* Used to have an unique sequence number for all the events */
   DWORD evsequence;

   int version;
};

/* Function called by all devices that Wine supports */
typedef struct dinput_device {
    INT pref;
    const char *name;
    BOOL (*enum_deviceA)(DWORD dwDevType, DWORD dwFlags, LPDIDEVICEINSTANCEA lpddi, int version, int id);
    BOOL (*enum_deviceW)(DWORD dwDevType, DWORD dwFlags, LPDIDEVICEINSTANCEW lpddi, int version, int id);
    HRESULT (*create_deviceA)(IDirectInputImpl *dinput, REFGUID rguid, REFIID riid, LPDIRECTINPUTDEVICEA* pdev);
    HRESULT (*create_deviceW)(IDirectInputImpl *dinput, REFGUID rguid, REFIID riid, LPDIRECTINPUTDEVICEW* pdev);
} dinput_device;

extern void dinput_register_device(dinput_device *device);

extern HINSTANCE DINPUT_instance;

void scan_keyboard();
void scan_mouse();

#endif /* __WINE_DLLS_DINPUT_DINPUT_PRIVATE_H */
