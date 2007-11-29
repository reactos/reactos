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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "objbase.h"
#include "mapix.h"
#include "mapiform.h"
#include "mapi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mapi);

LONG MAPI_ObjectCount = 0;

/***********************************************************************
 *              DllMain (MAPI32.init)
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID fImpLoad)
{
    TRACE("(%p,%d,%p)\n", hinstDLL, fdwReason, fImpLoad);

    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinstDLL);
        break;
    case DLL_PROCESS_DETACH:
	TRACE("DLL_PROCESS_DETACH: %d objects remaining\n", MAPI_ObjectCount);
	break;
    }
    return TRUE;
}

/***********************************************************************
 *		DllGetClassObject (MAPI32.27)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID iid, LPVOID *ppv)
{
    *ppv = NULL;
    FIXME("\n\tCLSID:\t%s,\n\tIID:\t%s\n", debugstr_guid(rclsid), debugstr_guid(iid));
    return CLASS_E_CLASSNOTAVAILABLE;
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

HRESULT WINAPI MAPIInitialize(LPVOID init)
{
    FIXME("(%p) Stub\n", init);
    return SUCCESS_SUCCESS;
}

ULONG WINAPI MAPILogon(ULONG_PTR uiparam, LPSTR profile, LPSTR password,
    FLAGS flags, ULONG reserved, LPLHANDLE session)
{
    FIXME("(0x%08lx %s %p 0x%08lx 0x%08x %p) Stub\n", uiparam,
          debugstr_a(profile), password, flags, reserved, session);

    if (session) *session = 1;
    return SUCCESS_SUCCESS;
}

ULONG WINAPI MAPILogoff(LHANDLE session, ULONG_PTR uiparam, FLAGS flags,
    ULONG reserved )
{
    FIXME("(0x%08lx 0x%08lx 0x%08lx 0x%08x) Stub\n", session,
          uiparam, flags, reserved);
    return SUCCESS_SUCCESS;
}

HRESULT WINAPI MAPILogonEx(ULONG_PTR uiparam, LPWSTR profile,
    LPWSTR password, ULONG flags, LPMAPISESSION *session)
{
    FIXME("(0x%08lx %s %p 0x%08x %p) Stub\n", uiparam,
          debugstr_w(profile), password, flags, session);
    return SUCCESS_SUCCESS;
}

HRESULT WINAPI MAPIOpenLocalFormContainer(LPVOID *ppfcnt)
{
    FIXME("(%p) Stub\n", ppfcnt);
    return E_FAIL;
}

VOID WINAPI MAPIUninitialize(void)
{
    FIXME("Stub\n");
}

HRESULT WINAPI MAPIAdminProfiles(ULONG ulFlags,  LPPROFADMIN *lppProfAdmin)
{
    FIXME("(%u, %p): stub\n", ulFlags, lppProfAdmin);
    *lppProfAdmin = NULL;
    return E_FAIL;
}
