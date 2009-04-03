/*              DirectShow Editing Services (qedit.dll)
 *
 * Copyright 2008 Google (Lei Zhang)
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

#include "qedit_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(qedit);

BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv)
{
    switch(fdwReason) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hInstDLL);
            break;
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}

/******************************************************************************
 * DirectShow ClassFactory
 */
typedef struct {
    IClassFactory ITF_IClassFactory;

    LONG ref;
    HRESULT (*pfnCreateInstance)(IUnknown *pUnkOuter, LPVOID *ppObj);
} IClassFactoryImpl;

struct object_creation_info
{
    const CLSID *clsid;
    HRESULT (*pfnCreateInstance)(IUnknown *pUnkOuter, LPVOID *ppObj);
};

static const struct object_creation_info object_creation[] =
{
    { &CLSID_MediaDet, MediaDet_create },
};

static HRESULT WINAPI
DSCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj)
{
    IClassFactoryImpl *This = (IClassFactoryImpl *)iface;

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IClassFactory))
    {
        IClassFactory_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }

    *ppobj = NULL;
    WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
    return E_NOINTERFACE;
}

static ULONG WINAPI DSCF_AddRef(LPCLASSFACTORY iface)
{
    IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI DSCF_Release(LPCLASSFACTORY iface)
{
    IClassFactoryImpl *This = (IClassFactoryImpl *)iface;

    ULONG ref = InterlockedDecrement(&This->ref);

    if (ref == 0)
        CoTaskMemFree(This);

    return ref;
}

static HRESULT WINAPI DSCF_CreateInstance(LPCLASSFACTORY iface, LPUNKNOWN pOuter, REFIID riid, LPVOID *ppobj)
{
    IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
    HRESULT hres;
    LPUNKNOWN punk;

    TRACE("(%p)->(%p,%s,%p)\n",This,pOuter,debugstr_guid(riid),ppobj);

    *ppobj = NULL;
    hres = This->pfnCreateInstance(pOuter, (LPVOID *) &punk);
    if (SUCCEEDED(hres)) {
        hres = IUnknown_QueryInterface(punk, riid, ppobj);
        IUnknown_Release(punk);
    }
    return hres;
}

static HRESULT WINAPI DSCF_LockServer(LPCLASSFACTORY iface, BOOL dolock)
{
    IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
    FIXME("(%p)->(%d),stub!\n",This,dolock);
    return S_OK;
}

static const IClassFactoryVtbl DSCF_Vtbl =
{
    DSCF_QueryInterface,
    DSCF_AddRef,
    DSCF_Release,
    DSCF_CreateInstance,
    DSCF_LockServer
};


/***********************************************************************
 *              DllCanUnloadNow (QEDIT.@)
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    return S_OK;
}

/*******************************************************************************
 * DllGetClassObject [QEDIT.@]
 * Retrieves class object from a DLL object
 *
 * PARAMS
 *    rclsid [I] CLSID for the class object
 *    riid   [I] Reference to identifier of interface for class object
 *    ppv    [O] Address of variable to receive interface pointer for riid
 *
 * RETURNS
 *    Success: S_OK
 *    Failure: CLASS_E_CLASSNOTAVAILABLE, E_OUTOFMEMORY, E_INVALIDARG,
 *             E_UNEXPECTED, E_NOINTERFACE
 */

HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    unsigned int i;
    IClassFactoryImpl *factory;

    TRACE("(%s,%s,%p)\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);

    if ( !IsEqualGUID( &IID_IClassFactory, riid )
         && ! IsEqualGUID( &IID_IUnknown, riid) )
        return E_NOINTERFACE;

    for (i=0; i < sizeof(object_creation)/sizeof(object_creation[0]); i++)
    {
        if (IsEqualGUID(object_creation[i].clsid, rclsid))
            break;
    }

    if (i == sizeof(object_creation)/sizeof(object_creation[0]))
    {
        FIXME("%s: no class found.\n", debugstr_guid(rclsid));
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    factory = CoTaskMemAlloc(sizeof(*factory));
    if (factory == NULL) return E_OUTOFMEMORY;

    factory->ITF_IClassFactory.lpVtbl = &DSCF_Vtbl;
    factory->ref = 1;

    factory->pfnCreateInstance = object_creation[i].pfnCreateInstance;

    *ppv = &(factory->ITF_IClassFactory);
    return S_OK;
}
