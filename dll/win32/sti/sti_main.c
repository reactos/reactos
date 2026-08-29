/*
 * Copyright (C) 2002 Aric Stewart for CodeWeavers
 * Copyright (C) 2009 Damjan Jovanovic
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

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winerror.h"
#include "objbase.h"
#include "initguid.h"
#include "wia_lh.h"
#include "sti.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(sti);

extern HRESULT WINAPI STI_DllGetClassObject(REFCLSID, REFIID, LPVOID *);
extern BOOL WINAPI STI_DllMain(HINSTANCE, DWORD, LPVOID);
extern HRESULT WINAPI STI_DllRegisterServer(void);
extern HRESULT WINAPI STI_DllUnregisterServer(void);

typedef HRESULT (*fnCreateInstance)(REFIID riid, IUnknown *pUnkOuter, LPVOID *ppObj);

typedef struct
{
    IClassFactory IClassFactory_iface;
    fnCreateInstance pfnCreateInstance;
} sti_cf;

static inline sti_cf *impl_from_IClassFactory( IClassFactory *iface )
{
    return CONTAINING_RECORD(iface, sti_cf, IClassFactory_iface);
}

static HRESULT sti_create( REFIID riid, IUnknown *pUnkOuter, LPVOID *ppObj )
{
    if (pUnkOuter != NULL && !IsEqualIID(riid, &IID_IUnknown))
        return CLASS_E_NOAGGREGATION;

    if (IsEqualGUID(riid, &IID_IUnknown))
        return StiCreateInstanceW(GetCurrentProcess(), STI_VERSION_REAL | STI_VERSION_FLAG_UNICODE, (PSTIW*) ppObj, pUnkOuter);
    else if (IsEqualGUID(riid, &IID_IStillImageW))
        return StiCreateInstanceW(GetCurrentProcess(), STI_VERSION_REAL | STI_VERSION_FLAG_UNICODE, (PSTIW*) ppObj, NULL);
    else if (IsEqualGUID(riid, &IID_IStillImageA))
        return StiCreateInstanceA(GetCurrentProcess(), STI_VERSION_REAL, (PSTIA*) ppObj, NULL);
    else
    {
        FIXME("no interface %s\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }
}

static HRESULT WINAPI sti_cf_QueryInterface( IClassFactory *iface, REFIID riid, LPVOID *ppobj )
{
    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IClassFactory))
    {
        IClassFactory_AddRef( iface );
        *ppobj = iface;
        return S_OK;
    }
    FIXME("interface %s not implemented\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI sti_cf_AddRef( IClassFactory *iface )
{
    return 2;
}

static ULONG WINAPI sti_cf_Release( IClassFactory *iface )
{
    return 1;
}

static HRESULT WINAPI sti_cf_CreateInstance( IClassFactory *iface, LPUNKNOWN pOuter,
                                             REFIID riid, LPVOID *ppobj )
{
    sti_cf *This = impl_from_IClassFactory( iface );
    HRESULT r;
    IUnknown *punk;

    TRACE("%p %s %p\n", pOuter, debugstr_guid(riid), ppobj);

    *ppobj = NULL;

    r = This->pfnCreateInstance( riid, pOuter, (LPVOID *)&punk );
    if (FAILED(r))
        return r;

    r = IUnknown_QueryInterface( punk, riid, ppobj );
    if (FAILED(r))
        return r;

    IUnknown_Release( punk );
    return r;
}

static HRESULT WINAPI sti_cf_LockServer( IClassFactory *iface, BOOL dolock )
{
    FIXME("(%p)->(%d)\n", iface, dolock);
    return S_OK;
}

static const struct IClassFactoryVtbl sti_cf_vtbl =
{
    sti_cf_QueryInterface,
    sti_cf_AddRef,
    sti_cf_Release,
    sti_cf_CreateInstance,
    sti_cf_LockServer
};

static sti_cf the_sti_cf = { { &sti_cf_vtbl }, sti_create };

BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("(0x%p, %ld, %p)\n",hInstDLL,fdwReason,lpvReserved);

    return STI_DllMain(hInstDLL, fdwReason, lpvReserved);
}

/******************************************************************************
 *           DllGetClassObject   (STI.@)
 */
HRESULT WINAPI DllGetClassObject( REFCLSID rclsid, REFIID iid, LPVOID *ppv )
{
    IClassFactory *cf = NULL;

    TRACE("%s %s %p\n", debugstr_guid(rclsid), debugstr_guid(iid), ppv);

    if (IsEqualGUID( rclsid, &CLSID_Sti ))
    {
       cf = &the_sti_cf.IClassFactory_iface;
    }

    if (cf)
        return IClassFactory_QueryInterface( cf, iid, ppv );
    return STI_DllGetClassObject( rclsid, iid, ppv );
}

/***********************************************************************
 *           DllRegisterServer (STI.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    return STI_DllRegisterServer();
}

/***********************************************************************
 *           DllUnRegisterServer (STI.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    return STI_DllUnregisterServer();
}
