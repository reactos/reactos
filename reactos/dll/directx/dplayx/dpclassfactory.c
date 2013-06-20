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

//#include <stdarg.h>
//#include <string.h>

#define COBJMACROS

//#include "windef.h"
//#include "winbase.h"
//#include "objbase.h"
//#include "winerror.h"
#include <wine/debug.h>
//#include "dplay.h"
//#include "dplobby.h"
//#include "initguid.h"
#include "dpinit.h"

WINE_DEFAULT_DEBUG_CHANNEL(dplay);


/*******************************************************************************
 * DirectPlayLobby ClassFactory
 */

typedef struct
{
    /* IUnknown fields */
    IClassFactory IClassFactory_iface;
    LONG          ref;
} IClassFactoryImpl;

static inline IClassFactoryImpl *impl_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, IClassFactoryImpl, IClassFactory_iface);
}

static HRESULT WINAPI
DP_and_DPL_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj) {
        IClassFactoryImpl *This = impl_from_IClassFactory(iface);

        FIXME("(%p)->(%s,%p),stub!\n",This,debugstr_guid(riid),ppobj);

        return E_NOINTERFACE;
}

static ULONG WINAPI
DP_and_DPL_AddRef(LPCLASSFACTORY iface) {
        IClassFactoryImpl *This = impl_from_IClassFactory(iface);
        return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI DP_and_DPL_Release(LPCLASSFACTORY iface) {
        IClassFactoryImpl *This = impl_from_IClassFactory(iface);
        /* static class (reference starts @ 1), won't ever be freed */
        return InterlockedDecrement(&This->ref);
}

static HRESULT WINAPI DP_and_DPL_CreateInstance(
        LPCLASSFACTORY iface,LPUNKNOWN pOuter,REFIID riid,LPVOID *ppobj
) {
        IClassFactoryImpl *This = impl_from_IClassFactory(iface);

        TRACE("(%p)->(%p,%s,%p)\n",This,pOuter,debugstr_guid(riid),ppobj);

        if ( DPL_CreateInterface( riid, ppobj ) == S_OK )
        {
           return S_OK;
        }
        else if ( DP_CreateInterface( riid, ppobj ) == S_OK )
        {
           return S_OK;
        }

        return E_NOINTERFACE;
}

static HRESULT WINAPI DP_and_DPL_LockServer(LPCLASSFACTORY iface,BOOL dolock) {
        IClassFactoryImpl *This = impl_from_IClassFactory(iface);
        FIXME("(%p)->(%d),stub!\n",This,dolock);
        return S_OK;
}

static const IClassFactoryVtbl DP_and_DPL_Vtbl = {
        DP_and_DPL_QueryInterface,
        DP_and_DPL_AddRef,
        DP_and_DPL_Release,
        DP_and_DPL_CreateInstance,
        DP_and_DPL_LockServer
};

static IClassFactoryImpl DP_and_DPL_CF = {{&DP_and_DPL_Vtbl}, 1 };


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

    if ( IsEqualCLSID( riid, &IID_IClassFactory ) )
    {
        *ppv = &DP_and_DPL_CF;
        IClassFactory_AddRef( (IClassFactory*)*ppv );

        return S_OK;
    }

    ERR("(%s,%s,%p): no interface found.\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
    return CLASS_E_CLASSNOTAVAILABLE;
}
