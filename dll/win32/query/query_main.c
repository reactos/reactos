/*
 *    Query Implementation
 *
 * Copyright 2006 Mike McCormack
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

#define COBJMACROS


#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "ole2.h"
#include "ntquery.h"
#include "cierror.h"
#include "initguid.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(query);

/***********************************************************************
 *             DllGetClassObject (QUERY.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID iid, LPVOID *ppv)
{
    FIXME("%s %s %p\n", debugstr_guid(rclsid), debugstr_guid(iid), ppv);

    return CLASS_E_CLASSNOTAVAILABLE;
}

HRESULT WINAPI CIState( WCHAR const *pwcsCat, WCHAR const *pwcsMachine, CI_STATE *pCiState)
{
    FIXME("%s %s %p\n", debugstr_w(pwcsCat), debugstr_w(pwcsMachine), pCiState);
    return CI_E_NOT_RUNNING;
}

HRESULT WINAPI LocateCatalogsA(CHAR const *pwszScope, ULONG iBm,
                               CHAR *pwszMachine, ULONG *pcMachine,
                               CHAR *pwszCat, ULONG *pcCat)
{

    FIXME("%s %lu %p %p %p %p\n", debugstr_a(pwszScope),
          iBm, pwszMachine, pcMachine, pwszCat, pcCat);
    return CI_E_NOT_RUNNING;
}

HRESULT WINAPI LocateCatalogsW(WCHAR const *pwszScope, ULONG iBm,
                               WCHAR *pwszMachine, ULONG *pcMachine,
                               WCHAR *pwszCat, ULONG *pcCat)
{

    FIXME("%s %lu %p %p %p %p\n", debugstr_w(pwszScope),
          iBm, pwszMachine, pcMachine, pwszCat, pcCat);
    return CI_E_NOT_RUNNING;
}

HRESULT WINAPI LoadIFilter(WCHAR const *pwcsPath, IUnknown *pUnkOuter, void **ppIUnk)
{
    FIXME("%s %p %p\n", debugstr_w(pwcsPath), pUnkOuter, ppIUnk);
    *ppIUnk = NULL;
    return E_NOTIMPL;
}
