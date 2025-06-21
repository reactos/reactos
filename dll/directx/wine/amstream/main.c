/*
 *     MultiMedia Streams Base Functions (AMSTREAM.DLL)
 *
 * Copyright 2004 Christian Costa
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
#include "winerror.h"

#include "ole2.h"
#include "rpcproxy.h"

#include "amstream_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

/******************************************************************************
 * Multimedia Streams ClassFactory
 */
typedef struct {
    IClassFactory IClassFactory_iface;
    LONG ref;
    HRESULT (*pfnCreateInstance)(IUnknown *pUnkOuter, LPVOID *ppObj);
} IClassFactoryImpl;

static inline IClassFactoryImpl *impl_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, IClassFactoryImpl, IClassFactory_iface);
}

struct object_creation_info
{
    const CLSID *clsid;
    HRESULT (*pfnCreateInstance)(IUnknown *pUnkOuter, LPVOID *ppObj);
};

static const struct object_creation_info object_creation[] =
{
    { &CLSID_AMMultiMediaStream, multimedia_stream_create },
    { &CLSID_AMDirectDrawStream, ddraw_stream_create },
    { &CLSID_AMAudioStream, audio_stream_create },
    { &CLSID_AMAudioData, AMAudioData_create },
    { &CLSID_MediaStreamFilter, filter_create }
};

static HRESULT WINAPI AMCF_QueryInterface(IClassFactory *iface, REFIID riid, void **ppobj)
{
    if (IsEqualGUID(riid, &IID_IUnknown)
	|| IsEqualGUID(riid, &IID_IClassFactory))
    {
	IClassFactory_AddRef(iface);
        *ppobj = iface;
	return S_OK;
    }

    *ppobj = NULL;
    WARN("(%p)->(%s,%p), not found\n", iface, debugstr_guid(riid), ppobj);
    return E_NOINTERFACE;
}

static ULONG WINAPI AMCF_AddRef(IClassFactory *iface)
{
    IClassFactoryImpl *This = impl_from_IClassFactory(iface);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI AMCF_Release(IClassFactory *iface)
{
    IClassFactoryImpl *This = impl_from_IClassFactory(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    if (ref == 0)
        free(This);

    return ref;
}


static HRESULT WINAPI AMCF_CreateInstance(IClassFactory *iface, IUnknown *pOuter,
                                          REFIID riid, void **ppobj)
{
    IClassFactoryImpl *This = impl_from_IClassFactory(iface);
    HRESULT hres;
    IUnknown *punk;

    TRACE("(%p)->(%p,%s,%p)\n",This,pOuter,debugstr_guid(riid),ppobj);

    *ppobj = NULL;
    hres = This->pfnCreateInstance(pOuter, (LPVOID *) &punk);
    if (SUCCEEDED(hres)) {
        hres = IUnknown_QueryInterface(punk, riid, ppobj);
        IUnknown_Release(punk);
    }
    return hres;
}

static HRESULT WINAPI AMCF_LockServer(IClassFactory *iface, BOOL dolock)
{
    IClassFactoryImpl *This = impl_from_IClassFactory(iface);
    FIXME("(%p)->(%d),stub!\n",This,dolock);
    return S_OK;
}

static const IClassFactoryVtbl DSCF_Vtbl =
{
    AMCF_QueryInterface,
    AMCF_AddRef,
    AMCF_Release,
    AMCF_CreateInstance,
    AMCF_LockServer
};

/*******************************************************************************
 * DllGetClassObject [AMSTREAM.@]
 * Retrieves class object from a DLL object
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
    unsigned int i;
    IClassFactoryImpl *factory;
    
    TRACE("(%s,%s,%p)\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
    
    if ( !IsEqualGUID( &IID_IClassFactory, riid )
	 && ! IsEqualGUID( &IID_IUnknown, riid) )
	return E_NOINTERFACE;

    for (i = 0; i < ARRAY_SIZE(object_creation); i++)
    {
	if (IsEqualGUID(object_creation[i].clsid, rclsid))
	    break;
    }

    if (i == ARRAY_SIZE(object_creation))
    {
	FIXME("%s: no class found.\n", debugstr_guid(rclsid));
	return CLASS_E_CLASSNOTAVAILABLE;
    }

    if (!(factory = calloc(1, sizeof(*factory))))
        return E_OUTOFMEMORY;

    factory->IClassFactory_iface.lpVtbl = &DSCF_Vtbl;
    factory->ref = 1;

    factory->pfnCreateInstance = object_creation[i].pfnCreateInstance;

    *ppv = &factory->IClassFactory_iface;
    return S_OK;
}
