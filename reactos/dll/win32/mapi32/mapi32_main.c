/*
 *             MAPI basics
 *
 * Copyright 2001 CodeWeavers Inc.
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "objbase.h"
#include "mapix.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mapi);

LONG MAPI_ObjectCount = 0;

/***********************************************************************
 *              DllMain (MAPI32.init)
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID fImpLoad)
{
    TRACE("(%p,%ld,%p)\n", hinstDLL, fdwReason, fImpLoad);

    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinstDLL);
        break;
    case DLL_PROCESS_DETACH:
	TRACE("DLL_PROCESS_DETACH: %ld objects remaining\n", MAPI_ObjectCount);
	break;
    }
    return TRUE;
}

/***********************************************************************
 * DllCanUnloadNow (MAPI32.28)
 *
 * Determine if this dll can be unloaded from the callers address space.
 *
 * PARAMS
 *  None.
 *
 * RETURNS
 *  S_OK, if the dll can be unloaded,
 *  S_FALSE, otherwise.
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    return MAPI_ObjectCount == 0 ? S_OK : S_FALSE;
}

HRESULT WINAPI MAPIInitialize ( LPVOID lpMapiInit )
{
    ERR("Stub\n");
    return MAPI_E_NOT_INITIALIZED;
}

ULONG WINAPI MAPILogon(ULONG ulUIParam, LPSTR lpszProfileName, LPSTR
lpszPassword, FLAGS flFlags, ULONG ulReserver, LPLHANDLE lplhSession)
{
    ERR("Stub\n");
    return MAPI_E_LOGON_FAILED;
}

HRESULT WINAPI MAPILogonEx(ULONG_PTR ulUIParam, LPWSTR lpszProfileName,
                           LPWSTR lpszPassword, ULONG flFlags,
                           LPMAPISESSION *lppSession)
{
    ERR("Stub\n");
    return MAPI_E_LOGON_FAILED;
}

VOID WINAPI MAPIUninitialize(void)
{
    ERR("Stub\n");
}
