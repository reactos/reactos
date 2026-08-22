/* 
 * DirectPlay
 * 
 * Copyright 2004 Raphael Junqueira
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
 *
 */


#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "objbase.h"
#include "oleauto.h"
#include "oleidl.h"
#include "rpcproxy.h"
#include "wine/debug.h"

#include "initguid.h"
#include "dpnet_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dpnet);

static BOOL winsock_loaded = FALSE;

static BOOL WINAPI winsock_startup(INIT_ONCE *once, void *param, void **context)
{
    WSADATA wsa_data;
    DWORD res;

    res = WSAStartup(MAKEWORD(1,1), &wsa_data);
    if(res == ERROR_SUCCESS)
        winsock_loaded = TRUE;
    else
        ERR("WSAStartup failed: %lu\n", res);
    return TRUE;
}

void init_winsock(void)
{
    static INIT_ONCE init_once = INIT_ONCE_STATIC_INIT;
    InitOnceExecuteOnce(&init_once, winsock_startup, NULL, NULL);
}

/* At process attach */
BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("%p,%lx,%p\n", hInstDLL, fdwReason, lpvReserved);

    switch(fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hInstDLL);
            break;

        case DLL_PROCESS_DETACH:
            if (lpvReserved) break;
            if(winsock_loaded)
                WSACleanup();
            break;
    }
    return TRUE;
}

/***********************************************************************
 *             DirectPlay8Create (DPNET.@)
 */
HRESULT WINAPI DirectPlay8Create(REFGUID lpGUID, LPVOID *ppvInt, LPUNKNOWN punkOuter)
{
    TRACE("(%s, %p, %p): stub\n", debugstr_guid(lpGUID), ppvInt, punkOuter);
    return S_OK;
}

/*******************************************************************************
 * DirectPlay ClassFactory
 */
typedef struct
{
  /* IUnknown fields */
  IClassFactory IClassFactory_iface;
  LONG          ref;
  REFCLSID      rclsid;
  HRESULT       (*pfnCreateInstanceFactory)(LPCLASSFACTORY iface, LPUNKNOWN punkOuter, REFIID riid, LPVOID *ppobj);
} IClassFactoryImpl;

static inline IClassFactoryImpl *impl_from_IClassFactory(IClassFactory *iface)
{
  return CONTAINING_RECORD(iface, IClassFactoryImpl, IClassFactory_iface);
}

static HRESULT WINAPI DICF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj) {
  IClassFactoryImpl *This = impl_from_IClassFactory(iface);

  FIXME("(%p)->(%s,%p),stub!\n",This,debugstr_guid(riid),ppobj);
  return E_NOINTERFACE;
}

static ULONG WINAPI DICF_AddRef(LPCLASSFACTORY iface) {
  IClassFactoryImpl *This = impl_from_IClassFactory(iface);
  return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI DICF_Release(LPCLASSFACTORY iface) {
  IClassFactoryImpl *This = impl_from_IClassFactory(iface);
  /* static class, won't be  freed */
  return InterlockedDecrement(&This->ref);
}

static HRESULT WINAPI DICF_CreateInstance(LPCLASSFACTORY iface,LPUNKNOWN pOuter,REFIID riid,LPVOID *ppobj) {
  IClassFactoryImpl *This = impl_from_IClassFactory(iface);

  TRACE("(%p)->(%p,%s,%p)\n",This,pOuter,debugstr_guid(riid),ppobj);
  return This->pfnCreateInstanceFactory(iface, pOuter, riid, ppobj);
}

static HRESULT WINAPI DICF_LockServer(LPCLASSFACTORY iface,BOOL dolock) {
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

static IClassFactoryImpl DPNET_CFS[] = {
  { { &DICF_Vtbl }, 1, &CLSID_DirectPlay8Client,  DPNET_CreateDirectPlay8Client },
  { { &DICF_Vtbl }, 1, &CLSID_DirectPlay8Server,  DPNET_CreateDirectPlay8Server },
  { { &DICF_Vtbl }, 1, &CLSID_DirectPlay8Peer,    DPNET_CreateDirectPlay8Peer },
  { { &DICF_Vtbl }, 1, &CLSID_DirectPlay8Address, DPNET_CreateDirectPlay8Address },
  { { &DICF_Vtbl }, 1, &CLSID_DirectPlay8LobbiedApplication, DPNET_CreateDirectPlay8LobbiedApp },
  { { &DICF_Vtbl }, 1, &CLSID_DirectPlay8LobbyClient, DPNET_CreateDirectPlay8LobbyClient },
  { { &DICF_Vtbl }, 1, &CLSID_DirectPlay8ThreadPool, DPNET_CreateDirectPlay8ThreadPool},
  { { NULL }, 0, NULL, NULL }
};

/***********************************************************************
 *		DllGetClassObject (DPNET.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    int i = 0;

    TRACE("(%s,%s,%p)\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
    /*
    if ( IsEqualCLSID( &IID_IClassFactory, riid ) ) {
    	*ppv = (LPVOID)&DPNET_CF;
	IClassFactory_AddRef((IClassFactory*)*ppv);
	return S_OK;
    }
    */
    while (NULL != DPNET_CFS[i].rclsid) {
      if (IsEqualGUID(rclsid, DPNET_CFS[i].rclsid)) {
        DICF_AddRef(&DPNET_CFS[i].IClassFactory_iface);
	*ppv = &DPNET_CFS[i];
	return S_OK;
      }
      ++i;
    }

    FIXME("(%s,%s,%p): no interface found.\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
    return CLASS_E_CLASSNOTAVAILABLE;
}
