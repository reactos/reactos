/*
 * Test co-installer and class installer DLL
 *
 * Copyright 2018 Zebediah Figura
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
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "setupapi.h"

unsigned int callback_count;
DI_FUNCTION last_message;

DWORD WINAPI class_success(DI_FUNCTION function, HDEVINFO set, SP_DEVINFO_DATA *device)
{
    callback_count++;
    last_message = function;
    return ERROR_SUCCESS;
}

DWORD WINAPI ClassInstall(DI_FUNCTION function, HDEVINFO set, SP_DEVINFO_DATA *device)
{
    return class_success(function, set, device);
}

DWORD WINAPI class_default(DI_FUNCTION function, HDEVINFO set, SP_DEVINFO_DATA *device)
{
    return ERROR_DI_DO_DEFAULT;
}

DWORD WINAPI class_error(DI_FUNCTION function, HDEVINFO set, SP_DEVINFO_DATA *device)
{
    return 0xdeadbeef;
}

DWORD WINAPI co_success(DI_FUNCTION function, HDEVINFO set, SP_DEVINFO_DATA *device,
        COINSTALLER_CONTEXT_DATA *context)
{
    callback_count++;
    last_message = function;
    return ERROR_SUCCESS;
}

DWORD WINAPI CoDeviceInstall(DI_FUNCTION function, HDEVINFO set, SP_DEVINFO_DATA *device,
        COINSTALLER_CONTEXT_DATA *context)
{
    return co_success(function, set, device, context);
}

DWORD WINAPI co_error(DI_FUNCTION function, HDEVINFO set, SP_DEVINFO_DATA *device,
        COINSTALLER_CONTEXT_DATA *context)
{
    return 0xdeadbeef;
}
