/*
 * Implementation of the Active Directory Service Interface
 *
 * Copyright 2005 Detlef Riekenberg
 *
 * This file contains only stubs to get the printui.dll up and running
 * activeds.dll is much much more than this
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

#define COBJMACROS
#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "winuser.h"

#include "objbase.h"
#include "iads.h"
#include "adshlp.h"

#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(activeds);

/*****************************************************
 * DllMain
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("(%p, %d, %p)\n",hinstDLL, fdwReason, lpvReserved);

    switch(fdwReason)
    {
    case DLL_WINE_PREATTACH:
        return FALSE;  /* prefer native version */
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls( hinstDLL );
        break;
    }
    return TRUE;
}

/*****************************************************
 * ADsGetObject     [ACTIVEDS.3]
 */
HRESULT WINAPI ADsGetObject(LPCWSTR lpszPathName, REFIID riid, VOID** ppObject)
{
    FIXME("(%s)->(%s,%p)!stub\n",debugstr_w(lpszPathName), debugstr_guid(riid), ppObject);
    return E_NOTIMPL;
}

/*****************************************************
 * ADsBuildEnumerator    [ACTIVEDS.4]
 */
HRESULT WINAPI ADsBuildEnumerator(IADsContainer * pADsContainer, IEnumVARIANT** ppEnumVariant)
{
    FIXME("(%p)->(%p)!stub\n",pADsContainer, ppEnumVariant);
    return E_NOTIMPL;
}

/*****************************************************
 * ADsEnumerateNext     [ACTIVEDS.6]
 */
HRESULT WINAPI ADsEnumerateNext(IEnumVARIANT* pEnumVariant, ULONG cElements, VARIANT* pvar, ULONG * pcElementsFetched)
{
    FIXME("(%p)->(%u, %p, %p)!stub\n",pEnumVariant, cElements, pvar, pcElementsFetched);
    return E_NOTIMPL;
}

/*****************************************************
 * ADsOpenObject     [ACTIVEDS.9]
 */
HRESULT WINAPI ADsOpenObject(LPCWSTR lpszPathName, LPCWSTR lpszUserName, LPCWSTR lpszPassword, DWORD dwReserved, REFIID riid, VOID** ppObject)
{
    FIXME("(%s,%s,%u,%p,%p)!stub\n", debugstr_w(lpszPathName),
          debugstr_w(lpszUserName), dwReserved, debugstr_guid(riid), ppObject);
    return E_NOTIMPL;
}

/*****************************************************
 * FreeADsMem             [ACTIVEDS.15]
 */
BOOL WINAPI FreeADsMem(LPVOID pMem)
{
    FIXME("(%p)!stub\n",pMem);
    return FALSE;
}
