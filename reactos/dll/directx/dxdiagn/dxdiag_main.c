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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "config.h"
#include "dxdiag_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dxdiag);

LONG DXDIAGN_refCount = 0;

/* At process attach */
BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
  TRACE("%p,%lx,%p\n", hInstDLL, fdwReason, lpvReserved);
  if (fdwReason == DLL_PROCESS_ATTACH) {
    DisableThreadLibraryCalls(hInstDLL);    
  }
  return TRUE;
}

/*******************************************************************************
 * DXDiag ClassFactory
 */
typedef struct {
  const IClassFactoryVtbl *lpVtbl;
  REFCLSID   rclsid;
  HRESULT   (*pfnCreateInstanceFactory)(LPCLASSFACTORY iface, LPUNKNOWN punkOuter, REFIID riid, LPVOID *ppobj);
} IClassFactoryImpl;

static HRESULT WINAPI DXDiagCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj) {
  FIXME("- no interface\n\tIID:\t%s\n", debugstr_guid(riid));

  if (ppobj == NULL) return E_POINTER;
  
  return E_NOINTERFACE;
}

static ULONG WINAPI DXDiagCF_AddRef(LPCLASSFACTORY iface) {
  DXDIAGN_LockModule();

  return 2; /* non-heap based object */
}

static ULONG WINAPI DXDiagCF_Release(LPCLASSFACTORY iface) {
  DXDIAGN_UnlockModule();

  return 1; /* non-heap based object */
}

static HRESULT WINAPI DXDiagCF_CreateInstance(LPCLASSFACTORY iface,LPUNKNOWN pOuter,REFIID riid,LPVOID *ppobj) {
  IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
  TRACE("(%p)->(%p,%s,%p)\n",This,pOuter,debugstr_guid(riid),ppobj);
  
  return This->pfnCreateInstanceFactory(iface, pOuter, riid, ppobj);
}

static HRESULT WINAPI DXDiagCF_LockServer(LPCLASSFACTORY iface,BOOL dolock) {
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

static IClassFactoryImpl DXDiag_CFS[] = {
  { &DXDiagCF_Vtbl, &CLSID_DxDiagProvider, DXDiag_CreateDXDiagProvider },
  { NULL, NULL, NULL }
};

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
    int i = 0;

    TRACE("(%p,%p,%p)\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
    while (NULL != DXDiag_CFS[i].rclsid) {
      if (IsEqualGUID(rclsid, DXDiag_CFS[i].rclsid)) {
	      DXDiagCF_AddRef((IClassFactory*) &DXDiag_CFS[i]);
	      *ppv = &DXDiag_CFS[i];
	      return S_OK;
      }
      ++i;
    }

    FIXME("(%p,%p,%p): no interface found.\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
    return CLASS_E_CLASSNOTAVAILABLE;
}
