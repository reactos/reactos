/*              DirectShow Base Functions (QUARTZ.DLL)
 *
 * Copyright 2002 Lionel Ulmer
 *
 * This file contains the (internal) driver registration functions,
 * driver enumeration APIs and DirectDraw creation functions.
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

#include "config.h"
#include "wine/debug.h"

#include "quartz_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

static DWORD dll_ref = 0;

/* For the moment, do nothing here. */
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
    { &CLSID_VideoRendererDefault, VideoRendererDefault_create },
    { &CLSID_DSoundRender, DSoundRender_create },
    { &CLSID_AudioRender, DSoundRender_create },
    { &CLSID_AVIDec, AVIDec_create },
    { &CLSID_SystemClock, QUARTZ_CreateSystemClock },
    { &CLSID_ACMWrapper, ACMWrapper_create },
    { &CLSID_WAVEParser, WAVEParser_create }
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


static HRESULT WINAPI DSCF_CreateInstance(LPCLASSFACTORY iface, LPUNKNOWN pOuter,
                      REFIID riid, LPVOID *ppobj)
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

static HRESULT WINAPI DSCF_LockServer(LPCLASSFACTORY iface,BOOL dolock)
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

/***********************************************************************
 *              DllCanUnloadNow (QUARTZ.@)
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    return dll_ref != 0 ? S_FALSE : S_OK;
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
 *              qzdebugstr_guid (internal)
 *
 * Gives a text version of DirectShow GUIDs
 */
const char * qzdebugstr_guid( const GUID * id )
{
    int i;
    char * name = NULL;

    for (i=0;InterfaceDesc[i].name && !name;i++) {
        if (IsEqualGUID(&InterfaceDesc[i].riid, id)) return InterfaceDesc[i].name;
    }
    return debugstr_guid(id);
}

/***********************************************************************
 *              qzdebugstr_State (internal)
 *
 * Gives a text version of the FILTER_STATE enumeration
 */
const char * qzdebugstr_State(FILTER_STATE state)
{
    switch (state)
    {
    case State_Stopped:
        return "State_Stopped";
    case State_Running:
        return "State_Running";
    case State_Paused:
        return "State_Paused";
    default:
        return "State_Unknown";
    }
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
    int len;
    static const char format[] = "Error: 0x%x";
    char error[MAX_ERROR_TEXT_LEN];

    FIXME("(%x,%p,%d) stub\n", hr, buffer, maxlen);

    if (!buffer) return 0;
    wsprintfA(error, format, hr);
    if ((len = lstrlenA(error)) >= maxlen) return 0; 
    lstrcpyA(buffer, error);
    return len;
}

/***********************************************************************
 *              AMGetErrorTextW (QUARTZ.@)
 */
DWORD WINAPI AMGetErrorTextW(HRESULT hr, LPWSTR buffer, DWORD maxlen)
{
    int len;
    static const WCHAR format[] = {'E','r','r','o','r',':',' ','0','x','%','l','x',0};
    WCHAR error[MAX_ERROR_TEXT_LEN];

    FIXME("(%x,%p,%d) stub\n", hr, buffer, maxlen);

    if (!buffer) return 0;
    wsprintfW(error, format, hr);
    if ((len = lstrlenW(error)) >= maxlen) return 0; 
    lstrcpyW(buffer, error);
    return len;
}
