/* 
 * DXDiag
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

#define COBJMACROS

#include <stdarg.h>

#include "initguid.h"
#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "oleauto.h"
#include "oleidl.h"
#include "rpcproxy.h"
#include "dxdiag_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dxdiag);

HINSTANCE dxdiagn_instance = 0;

LONG DXDIAGN_refCount = 0;

/* At process attach */
BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
  TRACE("%p,%lx,%p\n", hInstDLL, fdwReason, lpvReserved);
  if (fdwReason == DLL_PROCESS_ATTACH) {
      dxdiagn_instance = hInstDLL;
      DisableThreadLibraryCalls(hInstDLL);
  }
  return TRUE;
}

/*******************************************************************************
 * DXDiag ClassFactory
 */
typedef struct {
  IClassFactory IClassFactory_iface;
} IClassFactoryImpl;

static HRESULT WINAPI DXDiagCF_QueryInterface(IClassFactory *iface, REFIID riid, void **ppv)
{
  if (ppv == NULL)
    return E_POINTER;

  if (IsEqualGUID(&IID_IUnknown, riid))
    TRACE("(%p)->(IID_IUnknown %p)\n", iface, ppv);
  else if (IsEqualGUID(&IID_IClassFactory, riid))
    TRACE("(%p)->(IID_IClassFactory %p)\n", iface, ppv);
  else if (IsEqualGUID(&IID_IExternalConnection, riid) ||
           IsEqualGUID(&IID_IMarshal, riid)) {
    TRACE("(%p)->(%s) ignoring\n", iface, debugstr_guid(riid));
    *ppv = NULL;
    return E_NOINTERFACE;
  }
  else {
    FIXME("(%p)->(%s %p)\n", iface, debugstr_guid(riid), ppv);
    *ppv = NULL;
    return E_NOINTERFACE;
  }

  *ppv = iface;
  IClassFactory_AddRef(iface);
  return S_OK;
}

static ULONG WINAPI DXDiagCF_AddRef(IClassFactory *iface)
{
  DXDIAGN_LockModule();

  return 2; /* non-heap based object */
}

static ULONG WINAPI DXDiagCF_Release(IClassFactory * iface)
{
  DXDIAGN_UnlockModule();

  return 1; /* non-heap based object */
}

static HRESULT WINAPI DXDiagCF_CreateInstance(IClassFactory *iface, IUnknown *pOuter, REFIID riid,
        void **ppv)
{
  TRACE("(%p)->(%p,%s,%p)\n", iface, pOuter, debugstr_guid(riid), ppv);

  return DXDiag_CreateDXDiagProvider(iface, pOuter, riid, ppv);
}

static HRESULT WINAPI DXDiagCF_LockServer(IClassFactory *iface, BOOL dolock)
{
  TRACE("(%d)\n", dolock);

  if (dolock)
    DXDIAGN_LockModule();
  else
    DXDIAGN_UnlockModule();
  
  return S_OK;
}

static const IClassFactoryVtbl DXDiagCF_Vtbl = {
  DXDiagCF_QueryInterface,
  DXDiagCF_AddRef,
  DXDiagCF_Release,
  DXDiagCF_CreateInstance,
  DXDiagCF_LockServer
};

static IClassFactoryImpl DXDiag_CF = { { &DXDiagCF_Vtbl } };

/***********************************************************************
 *             DllCanUnloadNow (DXDIAGN.@)
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
  return DXDIAGN_refCount != 0 ? S_FALSE : S_OK;
}

/***********************************************************************
 *		DllGetClassObject (DXDIAGN.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    TRACE("(%s,%s,%p)\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);

    if (IsEqualGUID(rclsid, &CLSID_DxDiagProvider)) {
      IClassFactory_AddRef(&DXDiag_CF.IClassFactory_iface);
      *ppv = &DXDiag_CF.IClassFactory_iface;
      return S_OK;
    }

    FIXME("(%s,%s,%p): no interface found.\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
    return CLASS_E_CLASSNOTAVAILABLE;
}
