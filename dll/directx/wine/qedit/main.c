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

#define WINE_NO_NAMELESS_EXTENSION

#include "qedit_private.h"
#include "rpcproxy.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, void *reserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(instance);
    }
    else if (reason == DLL_PROCESS_DETACH && !reserved)
    {
        strmbase_release_typelibs();
    }
    return TRUE;
}

/******************************************************************************
 * DirectShow ClassFactory
 */
typedef struct {
    IClassFactory IClassFactory_iface;
    LONG ref;
    HRESULT (*create_instance)(IUnknown *outer, IUnknown **out);
} IClassFactoryImpl;

static inline IClassFactoryImpl *impl_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, IClassFactoryImpl, IClassFactory_iface);
}

struct object_creation_info
{
    const CLSID *clsid;
    HRESULT (*create_instance)(IUnknown *outer, IUnknown **out);
};

static const struct object_creation_info object_creation[] =
{
    {&CLSID_AMTimeline, timeline_create},
    {&CLSID_MediaDet, media_detector_create},
    {&CLSID_NullRenderer, null_renderer_create},
    {&CLSID_SampleGrabber, sample_grabber_create},
};

static HRESULT WINAPI DSCF_QueryInterface(IClassFactory *iface, REFIID riid, void **ppobj)
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

static ULONG WINAPI DSCF_AddRef(IClassFactory *iface)
{
    IClassFactoryImpl *This = impl_from_IClassFactory(iface);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI DSCF_Release(IClassFactory *iface)
{
    IClassFactoryImpl *This = impl_from_IClassFactory(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    if (ref == 0)
        CoTaskMemFree(This);

    return ref;
}

static HRESULT WINAPI DSCF_CreateInstance(IClassFactory *iface, IUnknown *pOuter, REFIID riid,
        void **ppobj)
{
    IClassFactoryImpl *This = impl_from_IClassFactory(iface);
    HRESULT hres;
    LPUNKNOWN punk;

    TRACE("(%p)->(%p,%s,%p)\n",This,pOuter,debugstr_guid(riid),ppobj);

    *ppobj = NULL;
    if (pOuter && !IsEqualGUID(&IID_IUnknown, riid))
        return E_NOINTERFACE;

    hres = This->create_instance(pOuter, &punk);
    if (SUCCEEDED(hres)) {
        hres = IUnknown_QueryInterface(punk, riid, ppobj);
        IUnknown_Release(punk);
    }
    return hres;
}

static HRESULT WINAPI DSCF_LockServer(IClassFactory *iface, BOOL dolock)
{
    IClassFactoryImpl *This = impl_from_IClassFactory(iface);
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

    factory = CoTaskMemAlloc(sizeof(*factory));
    if (factory == NULL) return E_OUTOFMEMORY;

    factory->IClassFactory_iface.lpVtbl = &DSCF_Vtbl;
    factory->ref = 1;

    factory->create_instance = object_creation[i].create_instance;

    *ppv = &factory->IClassFactory_iface;
    return S_OK;
}

static const REGPINTYPES reg_null_mt = {&GUID_NULL, &GUID_NULL};

static const REGFILTERPINS2 reg_sample_grabber_pins[2] =
{
    {
        .cInstances = 1,
        .nMediaTypes = 1,
        .lpMediaType = &reg_null_mt,
    },
    {
        .dwFlags = REG_PINFLAG_B_OUTPUT,
        .cInstances = 1,
        .nMediaTypes = 1,
        .lpMediaType = &reg_null_mt,
    },
};

static const REGFILTER2 reg_sample_grabber =
{
    .dwVersion = 2,
    .dwMerit = MERIT_DO_NOT_USE,
    .u.s2.cPins2 = 2,
    .u.s2.rgPins2 = reg_sample_grabber_pins,
};

static const REGFILTERPINS2 reg_null_renderer_pins[1] =
{
    {
        .dwFlags = REG_PINFLAG_B_OUTPUT,
        .cInstances = 1,
        .nMediaTypes = 1,
        .lpMediaType = &reg_null_mt,
    },
};

static const REGFILTER2 reg_null_renderer =
{
    .dwVersion = 2,
    .dwMerit = MERIT_DO_NOT_USE,
    .u.s2.cPins2 = 1,
    .u.s2.rgPins2 = reg_null_renderer_pins,
};

/***********************************************************************
 *		DllRegisterServer (QEDIT.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    IFilterMapper2 *mapper;
    HRESULT hr;

    if (FAILED(hr = __wine_register_resources()))
        return hr;

    if (FAILED(hr = CoCreateInstance(&CLSID_FilterMapper2, NULL, CLSCTX_INPROC_SERVER,
            &IID_IFilterMapper2, (void **)&mapper)))
        return hr;

    IFilterMapper2_RegisterFilter(mapper, &CLSID_SampleGrabber, L"SampleGrabber",
            NULL, NULL, NULL, &reg_sample_grabber);
    IFilterMapper2_RegisterFilter(mapper, &CLSID_NullRenderer, L"Null Renderer",
            NULL, NULL, NULL, &reg_null_renderer);

    IFilterMapper2_Release(mapper);
    return S_OK;
}

/***********************************************************************
 *		DllUnregisterServer (QEDIT.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    IFilterMapper2 *mapper;
    HRESULT hr;

    if (FAILED(hr = __wine_unregister_resources()))
        return hr;

    if (FAILED(hr = CoCreateInstance(&CLSID_FilterMapper2, NULL, CLSCTX_INPROC_SERVER,
            &IID_IFilterMapper2, (void **)&mapper)))
        return hr;

    IFilterMapper2_UnregisterFilter(mapper, NULL, NULL, &CLSID_SampleGrabber);
    IFilterMapper2_UnregisterFilter(mapper, NULL, NULL, &CLSID_NullRenderer);

    IFilterMapper2_Release(mapper);
    return S_OK;
}
