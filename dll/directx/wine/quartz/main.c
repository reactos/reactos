/*              DirectShow Base Functions (QUARTZ.DLL)
 *
 * Copyright 2002 Lionel Ulmer
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

#include "quartz_private.h"

extern HRESULT WINAPI QUARTZ_DllGetClassObject(REFCLSID, REFIID, LPVOID *) DECLSPEC_HIDDEN;
extern HRESULT WINAPI QUARTZ_DllCanUnloadNow(void) DECLSPEC_HIDDEN;
extern BOOL WINAPI QUARTZ_DllMain(HINSTANCE, DWORD, LPVOID) DECLSPEC_HIDDEN;

/* For the moment, do nothing here. */
BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv)
{
    return QUARTZ_DllMain( hInstDLL, fdwReason, lpv );
}

static HRESULT SeekingPassThru_create(IUnknown *pUnkOuter, LPVOID *ppObj)
{
    return PosPassThru_Construct(pUnkOuter,ppObj); /* from strmbase */
}

/******************************************************************************
 * DirectShow ClassFactory
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
    { &CLSID_SeekingPassThru, SeekingPassThru_create },
    { &CLSID_FilterGraph, FilterGraph_create },
    { &CLSID_FilterGraphNoThread, FilterGraphNoThread_create },
    { &CLSID_FilterMapper, FilterMapper_create },
    { &CLSID_FilterMapper2, FilterMapper2_create },
    { &CLSID_AsyncReader, AsyncReader_create },
    { &CLSID_MemoryAllocator, StdMemAllocator_create },
    { &CLSID_AviSplitter, AVISplitter_create },
    { &CLSID_MPEG1Splitter, MPEGSplitter_create },
    { &CLSID_VideoRenderer, VideoRenderer_create },
    { &CLSID_NullRenderer, NullRenderer_create },
    { &CLSID_VideoMixingRenderer, VMR7Impl_create },
    { &CLSID_VideoMixingRenderer9, VMR9Impl_create },
    { &CLSID_VideoRendererDefault, VideoRendererDefault_create },
    { &CLSID_DSoundRender, DSoundRender_create },
    { &CLSID_AudioRender, DSoundRender_create },
    { &CLSID_AVIDec, AVIDec_create },
    { &CLSID_SystemClock, QUARTZ_CreateSystemClock },
    { &CLSID_ACMWrapper, ACMWrapper_create },
    { &CLSID_WAVEParser, WAVEParser_create }
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


static HRESULT WINAPI DSCF_CreateInstance(IClassFactory *iface, IUnknown *pOuter,
                                          REFIID riid, void **ppobj)
{
    IClassFactoryImpl *This = impl_from_IClassFactory(iface);
    HRESULT hres;
    LPUNKNOWN punk;

    TRACE("(%p)->(%p,%s,%p)\n",This,pOuter,debugstr_guid(riid),ppobj);

    *ppobj = NULL;
    if(pOuter && !IsEqualGUID(&IID_IUnknown, riid))
        return E_NOINTERFACE;

    hres = This->pfnCreateInstance(pOuter, (LPVOID *) &punk);
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
 * DllGetClassObject [QUARTZ.@]
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

    TRACE("(%s,%s,%p)\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);

    if (IsEqualGUID( &IID_IClassFactory, riid ) || IsEqualGUID( &IID_IUnknown, riid))
    {
        for (i=0; i < sizeof(object_creation)/sizeof(object_creation[0]); i++)
        {
            if (IsEqualGUID(object_creation[i].clsid, rclsid))
            {
                IClassFactoryImpl *factory = CoTaskMemAlloc(sizeof(*factory));
                if (factory == NULL) return E_OUTOFMEMORY;

                factory->IClassFactory_iface.lpVtbl = &DSCF_Vtbl;
                factory->ref = 1;

                factory->pfnCreateInstance = object_creation[i].pfnCreateInstance;

                *ppv = &factory->IClassFactory_iface;
                return S_OK;
            }
        }
    }
    return QUARTZ_DllGetClassObject( rclsid, riid, ppv );
}

/***********************************************************************
 *              DllCanUnloadNow (QUARTZ.@)
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    return QUARTZ_DllCanUnloadNow();
}


#define OUR_GUID_ENTRY(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    { { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } } , #name },

static const struct {
	const GUID	riid;
	const char 	*name;
} InterfaceDesc[] =
{
#include "uuids.h"
    { { 0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0} }, NULL }
};

/***********************************************************************
 *              proxies
 */
HRESULT CALLBACK ICaptureGraphBuilder_FindInterface_Proxy( ICaptureGraphBuilder *This,
                                                           const GUID *pCategory,
                                                           IBaseFilter *pf,
                                                           REFIID riid,
                                                           void **ppint )
{
    return ICaptureGraphBuilder_RemoteFindInterface_Proxy( This, pCategory, pf,
                                                           riid, (IUnknown **)ppint );
}

HRESULT __RPC_STUB ICaptureGraphBuilder_FindInterface_Stub( ICaptureGraphBuilder *This,
                                                            const GUID *pCategory,
                                                            IBaseFilter *pf,
                                                            REFIID riid,
                                                            IUnknown **ppint )
{
    return ICaptureGraphBuilder_FindInterface( This, pCategory, pf, riid, (void **)ppint );
}

HRESULT CALLBACK ICaptureGraphBuilder2_FindInterface_Proxy( ICaptureGraphBuilder2 *This,
                                                            const GUID *pCategory,
                                                            const GUID *pType,
                                                            IBaseFilter *pf,
                                                            REFIID riid,
                                                            void **ppint )
{
    return ICaptureGraphBuilder2_RemoteFindInterface_Proxy( This, pCategory, pType,
                                                            pf, riid, (IUnknown **)ppint );
}

HRESULT __RPC_STUB ICaptureGraphBuilder2_FindInterface_Stub( ICaptureGraphBuilder2 *This,
                                                             const GUID *pCategory,
                                                             const GUID *pType,
                                                             IBaseFilter *pf,
                                                             REFIID riid,
                                                             IUnknown **ppint )
{
    return ICaptureGraphBuilder2_FindInterface( This, pCategory, pType, pf, riid, (void **)ppint );
}

/***********************************************************************
 *              qzdebugstr_guid (internal)
 *
 * Gives a text version of DirectShow GUIDs
 */
const char * qzdebugstr_guid( const GUID * id )
{
    int i;

    for (i=0; InterfaceDesc[i].name; i++)
        if (IsEqualGUID(&InterfaceDesc[i].riid, id)) return InterfaceDesc[i].name;

    return debugstr_guid(id);
}

LONG WINAPI AmpFactorToDB(LONG ampfactor)
{
    FIXME("(%d) Stub!\n", ampfactor);
    return 0;
}

LONG WINAPI DBToAmpFactor(LONG db)
{
    FIXME("(%d) Stub!\n", db);
    /* Avoid divide by zero (probably during range computation) in Windows Media Player 6.4 */
    if (db < -1000)
	return 0;
    return 100;
}

/***********************************************************************
 *              AMGetErrorTextA (QUARTZ.@)
 */
DWORD WINAPI AMGetErrorTextA(HRESULT hr, LPSTR buffer, DWORD maxlen)
{
    DWORD res;
    WCHAR errorW[MAX_ERROR_TEXT_LEN];

    TRACE("(%x,%p,%d)\n", hr, buffer, maxlen);
    if (!buffer)
        return 0;

    res = AMGetErrorTextW(hr, errorW, sizeof(errorW)/sizeof(*errorW));
    return WideCharToMultiByte(CP_ACP, 0, errorW, res, buffer, maxlen, 0, 0);
}

/***********************************************************************
 *              AMGetErrorTextW (QUARTZ.@)
 */
DWORD WINAPI AMGetErrorTextW(HRESULT hr, LPWSTR buffer, DWORD maxlen)
{
    unsigned int len;
    static const WCHAR format[] = {'E','r','r','o','r',':',' ','0','x','%','l','x',0};
    WCHAR error[MAX_ERROR_TEXT_LEN];

    FIXME("(%x,%p,%d) stub\n", hr, buffer, maxlen);

    if (!buffer) return 0;
    wsprintfW(error, format, hr);
    if ((len = strlenW(error)) >= maxlen) return 0;
    lstrcpyW(buffer, error);
    return len;
}
