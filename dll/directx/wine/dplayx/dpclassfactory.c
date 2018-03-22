/* COM class factory for direct play lobby interfaces.
 *
 * Copyright 1999, 2000 Peter Hunnisett
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

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "objbase.h"
#include "winerror.h"
#include "wine/debug.h"
#include "dplay.h"
#include "dplobby.h"
#include "initguid.h"
#include "dplay_global.h"

WINE_DEFAULT_DEBUG_CHANNEL(dplay);


typedef struct
{
    IClassFactory IClassFactory_iface;
    HRESULT (*createinstance)(REFIID riid, void **ppv);
} IClassFactoryImpl;

static inline IClassFactoryImpl *impl_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, IClassFactoryImpl, IClassFactory_iface);
}

static HRESULT WINAPI IClassFactoryImpl_QueryInterface(IClassFactory *iface, REFIID riid,
        void **ppv)
{
    TRACE("(%p)->(%s %p)\n", iface, debugstr_guid(riid), ppv);

    if (IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IClassFactory, riid))
    {
        *ppv = iface;
        IClassFactory_AddRef(iface);
        return S_OK;
    }

    *ppv = NULL;
    WARN("no interface for %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI IClassFactoryImpl_AddRef(IClassFactory *iface)
{
    return 2; /* non-heap based object */
}

static ULONG WINAPI IClassFactoryImpl_Release(IClassFactory *iface)
{
    return 1; /* non-heap based object */
}

static HRESULT WINAPI IClassFactoryImpl_CreateInstance(IClassFactory *iface, IUnknown *pOuter,
        REFIID riid, void **ppv)
{
    IClassFactoryImpl *This = impl_from_IClassFactory(iface);

    TRACE("(%p)->(%p,%s,%p)\n", iface, pOuter, debugstr_guid(riid), ppv);

    if (pOuter)
        return CLASS_E_NOAGGREGATION;

    return This->createinstance(riid, ppv);
}

static HRESULT WINAPI IClassFactoryImpl_LockServer(IClassFactory *iface, BOOL dolock)
{
    FIXME("(%p)->(%d),stub!\n", iface, dolock);
    return S_OK;
}

static const IClassFactoryVtbl cf_vt = {
    IClassFactoryImpl_QueryInterface,
    IClassFactoryImpl_AddRef,
    IClassFactoryImpl_Release,
    IClassFactoryImpl_CreateInstance,
    IClassFactoryImpl_LockServer
};

static IClassFactoryImpl dplay_cf = {{&cf_vt}, dplay_create};
static IClassFactoryImpl dplaylobby_cf = {{&cf_vt}, dplobby_create};


/*******************************************************************************
 * DllGetClassObject [DPLAYX.@]
 * Retrieves DP or DPL class object from a DLL object
 *
 * NOTES
 *    Docs say returns STDAPI
 *
 * PARAMS
 *    rclsid [I] CLSID for the class object
 *    riid   [I] Reference to identifier of interface for class object
 *    ppv    [O] Address of variable to receive interface pointer for riid
 *
 * RETURNS
 *    Success: S_OK
 *    Failure: CLASS_E_CLASSNOTAVAILABLE, E_OUTOFMEMORY, E_INVALIDARG,
 *             E_UNEXPECTED
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    TRACE("(%s,%s,%p)\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);

    if (IsEqualCLSID(&CLSID_DirectPlay, rclsid))
        return IClassFactory_QueryInterface(&dplay_cf.IClassFactory_iface, riid, ppv);

    if (IsEqualCLSID(&CLSID_DirectPlayLobby, rclsid))
        return IClassFactory_QueryInterface(&dplaylobby_cf.IClassFactory_iface, riid, ppv);

    FIXME("(%s,%s,%p): no class found.\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
    return CLASS_E_CLASSNOTAVAILABLE;
}
