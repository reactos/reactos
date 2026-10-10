/*
 * DirectPlay Voice
 *
 * Copyright (C) 2014 Alistair Leslie-Hughes
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "rpcproxy.h"
#include "wine/debug.h"

#include "initguid.h"
#include "dvoice.h"
#include "dvoice_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dpvoice);

typedef struct
{
    IClassFactory IClassFactory_iface;
    LONG          ref;
    REFCLSID      rclsid;
    HRESULT       (*pfnCreateInstanceFactory)(IClassFactory *iface, IUnknown *punkOuter, REFIID riid, void **ppobj);
} IClassFactoryImpl;

static inline IClassFactoryImpl *impl_from_IClassFactory(IClassFactory *iface)
{
  return CONTAINING_RECORD(iface, IClassFactoryImpl, IClassFactory_iface);
}

static HRESULT WINAPI DICF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,void **ppobj)
{
    IClassFactoryImpl *This = impl_from_IClassFactory(iface);

    FIXME("(%p)->(%s,%p),stub!\n",This,debugstr_guid(riid),ppobj);
    return E_NOINTERFACE;
}

static ULONG WINAPI DICF_AddRef(LPCLASSFACTORY iface)
{
    IClassFactoryImpl *This = impl_from_IClassFactory(iface);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI DICF_Release(LPCLASSFACTORY iface)
{
    IClassFactoryImpl *This = impl_from_IClassFactory(iface);
    /* static class, won't be  freed */
    return InterlockedDecrement(&This->ref);
}

static HRESULT WINAPI DICF_CreateInstance(IClassFactory *iface, IUnknown *pOuter, REFIID riid, void **ppobj)
{
    IClassFactoryImpl *This = impl_from_IClassFactory(iface);

    TRACE("(%p)->(%p,%s,%p)\n",This,pOuter,debugstr_guid(riid),ppobj);
    return This->pfnCreateInstanceFactory(iface, pOuter, riid, ppobj);
}

static HRESULT WINAPI DICF_LockServer(IClassFactory *iface,BOOL dolock)
{
    IClassFactoryImpl *This = impl_from_IClassFactory(iface);
    FIXME("(%p)->(%d),stub!\n",This,dolock);
    return S_OK;
}

static const IClassFactoryVtbl DICF_Vtbl = {
    DICF_QueryInterface,
    DICF_AddRef,
    DICF_Release,
    DICF_CreateInstance,
    DICF_LockServer
};

static IClassFactoryImpl DPVOICE_CFS[] =
{
    { { &DICF_Vtbl }, 1, &CLSID_DirectPlayVoiceClient,  DPVOICE_CreateDirectPlayVoiceClient },
    { { &DICF_Vtbl }, 1, &CLSID_DirectPlayVoiceServer,  DPVOICE_CreateDirectPlayVoiceServer },
    { { &DICF_Vtbl }, 1, &CLSID_DirectPlayVoiceTest,    DPVOICE_CreateDirectPlayVoiceTest },
    { { NULL }, 0, NULL, NULL }
};

HRESULT WINAPI DirectPlayVoiceCreate(LPCGUID pIID, void **ppvInterface)
{
    FIXME("(%s, %p) stub\n", debugstr_guid(pIID), ppvInterface);
    return E_NOTIMPL;
}

HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)
{
    int i = 0;

    TRACE("(%s,%s,%p)\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);

    while (NULL != DPVOICE_CFS[i].rclsid)
    {
        if (IsEqualGUID(rclsid, DPVOICE_CFS[i].rclsid))
        {
            DICF_AddRef(&DPVOICE_CFS[i].IClassFactory_iface);
            *ppv = &DPVOICE_CFS[i];
            return S_OK;
        }
        ++i;
    }

    FIXME("(%s,%s,%p): no interface found.\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
    return CLASS_E_CLASSNOTAVAILABLE;
}
