/*
 * Main DLL interface to Queue Manager (BITS)
 *
 * Background Intelligent Transfer Service (BITS) interface.  Dll is named
 * qmgr for backwards compatibility with early versions of BITS.
 *
 * Copyright 2007 Google (Roy Shea)
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

#include <stdio.h>

#define COBJMACROS
#include "objbase.h"
#include "winuser.h"
#include "winreg.h"
#include "advpub.h"
#include "olectl.h"
#include "rpcproxy.h"
#include "winsvc.h"
#include "qmgr.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(qmgr);

/* Handle to the base address of this DLL */
static HINSTANCE hInst;

/* Entry point for DLL */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("(%p, %d, %p)\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;  /* prefer native version */
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            hInst = hinstDLL;
            break;
    }

    return TRUE;
}

HRESULT WINAPI DllRegisterServer(void)
{
    return __wine_register_resources(hInst);
}

HRESULT WINAPI DllUnregisterServer(void)
{
    return __wine_unregister_resources(hInst);
}
