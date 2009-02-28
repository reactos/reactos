/*
 * COM proxy/stub factory (CStdPSFactory) implementation
 *
 * Copyright 2001 Ove Kåven, TransGaming Technologies
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
#include "wine/port.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"

#include "objbase.h"

#include "rpcproxy.h"

#include "wine/unicode.h"
#include "wine/debug.h"

#include "cpsf.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

static void format_clsid( WCHAR *buffer, const CLSID *clsid )
{
    static const WCHAR clsid_formatW[] = {'{','%','0','8','X','-','%','0','4','X','-','%','0','4','X','-',
                                    '%','0','2','X','%','0','2','X','-','%','0','2','X','%','0','2','X',
                                    '%','0','2','X','%','0','2','X','%','0','2','X','%','0','2','X','}',0};

    sprintfW( buffer, clsid_formatW, clsid->Data1, clsid->Data2, clsid->Data3,
              clsid->Data4[0], clsid->Data4[1], clsid->Data4[2], clsid->Data4[3],
              clsid->Data4[4], clsid->Data4[5], clsid->Data4[6], clsid->Data4[7] );

}

static BOOL FindProxyInfo(const ProxyFileInfo **pProxyFileList, REFIID riid, const ProxyFileInfo **pProxyInfo, int *pIndex)
{
  while (*pProxyFileList) {
    if ((*pProxyFileList)->pIIDLookupRtn(riid, pIndex)) {
      *pProxyInfo = *pProxyFileList;
      TRACE("found: ProxyInfo %p Index %d\n", *pProxyInfo, *pIndex);
      return TRUE;
    }
    pProxyFileList++;
  }
  TRACE("not found\n");
  return FALSE;
}

static HRESULT WINAPI CStdPSFactory_QueryInterface(LPPSFACTORYBUFFER iface,
                                                  REFIID riid,
                                                  LPVOID *obj)
{
  CStdPSFactoryBuffer *This = (CStdPSFactoryBuffer *)iface;
  TRACE("(%p)->QueryInterface(%s,%p)\n",iface,debugstr_guid(riid),obj);
  if (IsEqualGUID(&IID_IUnknown,riid) ||
      IsEqualGUID(&IID_IPSFactoryBuffer,riid)) {
    *obj = This;
    InterlockedIncrement( &This->RefCount );
    return S_OK;
  }
  return E_NOINTERFACE;
}

static ULONG WINAPI CStdPSFactory_AddRef(LPPSFACTORYBUFFER iface)
{
  CStdPSFactoryBuffer *This = (CStdPSFactoryBuffer *)iface;
  TRACE("(%p)->AddRef()\n",iface);
  return InterlockedIncrement( &This->RefCount );
}

static ULONG WINAPI CStdPSFactory_Release(LPPSFACTORYBUFFER iface)
{
  CStdPSFactoryBuffer *This = (CStdPSFactoryBuffer *)iface;
  TRACE("(%p)->Release()\n",iface);
  return InterlockedDecrement( &This->RefCount );
}

static HRESULT WINAPI CStdPSFactory_CreateProxy(LPPSFACTORYBUFFER iface,
                                               LPUNKNOWN pUnkOuter,
                                               REFIID riid,
                                               LPRPCPROXYBUFFER *ppProxy,
                                               LPVOID *ppv)
{
  CStdPSFactoryBuffer *This = (CStdPSFactoryBuffer *)iface;
  const ProxyFileInfo *ProxyInfo;
  int Index;
  TRACE("(%p)->CreateProxy(%p,%s,%p,%p)\n",iface,pUnkOuter,
       debugstr_guid(riid),ppProxy,ppv);
  if (!FindProxyInfo(This->pProxyFileList,riid,&ProxyInfo,&Index))
    return E_NOINTERFACE;
  return StdProxy_Construct(riid, pUnkOuter, ProxyInfo, Index, iface, ppProxy, ppv);
}

static HRESULT WINAPI CStdPSFactory_CreateStub(LPPSFACTORYBUFFER iface,
                                              REFIID riid,
                                              LPUNKNOWN pUnkServer,
                                              LPRPCSTUBBUFFER *ppStub)
{
  CStdPSFactoryBuffer *This = (CStdPSFactoryBuffer *)iface;
  const ProxyFileInfo *ProxyInfo;
  int Index;
  TRACE("(%p)->CreateStub(%s,%p,%p)\n",iface,debugstr_guid(riid),
       pUnkServer,ppStub);
  if (!FindProxyInfo(This->pProxyFileList,riid,&ProxyInfo,&Index))
    return E_NOINTERFACE;

  if(ProxyInfo->pDelegatedIIDs && ProxyInfo->pDelegatedIIDs[Index])
    return  CStdStubBuffer_Delegating_Construct(riid, pUnkServer, ProxyInfo->pNamesArray[Index],
                                                ProxyInfo->pStubVtblList[Index], ProxyInfo->pDelegatedIIDs[Index],
                                                iface, ppStub);

  return CStdStubBuffer_Construct(riid, pUnkServer, ProxyInfo->pNamesArray[Index],
                                  ProxyInfo->pStubVtblList[Index], iface, ppStub);
}

static const IPSFactoryBufferVtbl CStdPSFactory_Vtbl =
{
  CStdPSFactory_QueryInterface,
  CStdPSFactory_AddRef,
  CStdPSFactory_Release,
  CStdPSFactory_CreateProxy,
  CStdPSFactory_CreateStub
};

/***********************************************************************
 *           NdrDllGetClassObject [RPCRT4.@]
 */
HRESULT WINAPI NdrDllGetClassObject(REFCLSID rclsid, REFIID iid, LPVOID *ppv,
                                   const ProxyFileInfo **pProxyFileList,
                                   const CLSID *pclsid,
                                   CStdPSFactoryBuffer *pPSFactoryBuffer)
{
  TRACE("(%s, %s, %p, %p, %s, %p)\n", debugstr_guid(rclsid),
    debugstr_guid(iid), ppv, pProxyFileList, debugstr_guid(pclsid),
    pPSFactoryBuffer);

  *ppv = NULL;
  if (!pPSFactoryBuffer->lpVtbl) {
    const ProxyFileInfo **pProxyFileList2;
    DWORD max_delegating_vtbl_size = 0;
    pPSFactoryBuffer->lpVtbl = &CStdPSFactory_Vtbl;
    pPSFactoryBuffer->RefCount = 0;
    pPSFactoryBuffer->pProxyFileList = pProxyFileList;
    for (pProxyFileList2 = pProxyFileList; *pProxyFileList2; pProxyFileList2++) {
      int i;
      for (i = 0; i < (*pProxyFileList2)->TableSize; i++) {
        /* FIXME: i think that different vtables should be copied for
         * async interfaces */
        void * const *pSrcRpcStubVtbl = (void * const *)&CStdStubBuffer_Vtbl;
        void **pRpcStubVtbl = (void **)&(*pProxyFileList2)->pStubVtblList[i]->Vtbl;
        unsigned int j;

        if ((*pProxyFileList2)->pDelegatedIIDs && (*pProxyFileList2)->pDelegatedIIDs[i]) {
          pSrcRpcStubVtbl = (void * const *)&CStdStubBuffer_Delegating_Vtbl;
          if ((*pProxyFileList2)->pStubVtblList[i]->header.DispatchTableCount > max_delegating_vtbl_size)
            max_delegating_vtbl_size = (*pProxyFileList2)->pStubVtblList[i]->header.DispatchTableCount;
        }

        for (j = 0; j < sizeof(IRpcStubBufferVtbl)/sizeof(void *); j++)
          if (!pRpcStubVtbl[j])
            pRpcStubVtbl[j] = pSrcRpcStubVtbl[j];
      }
    }
    if(max_delegating_vtbl_size > 0)
      create_delegating_vtbl(max_delegating_vtbl_size);
  }
  if (pclsid && IsEqualGUID(rclsid, pclsid))
    return IPSFactoryBuffer_QueryInterface((LPPSFACTORYBUFFER)pPSFactoryBuffer, iid, ppv);
  else {
    const ProxyFileInfo *info;
    int index;
    /* otherwise, the dll may be using the iid as the clsid, so
     * search for it in the proxy file list */
    if (FindProxyInfo(pProxyFileList, rclsid, &info, &index))
      return IPSFactoryBuffer_QueryInterface((LPPSFACTORYBUFFER)pPSFactoryBuffer, iid, ppv);

    WARN("class %s not available\n", debugstr_guid(rclsid));
    return CLASS_E_CLASSNOTAVAILABLE;
  }
}

/***********************************************************************
 *           NdrDllCanUnloadNow [RPCRT4.@]
 */
HRESULT WINAPI NdrDllCanUnloadNow(CStdPSFactoryBuffer *pPSFactoryBuffer)
{
  return !(pPSFactoryBuffer->RefCount);
}


/***********************************************************************
 *           NdrDllRegisterProxy [RPCRT4.@]
 */
HRESULT WINAPI NdrDllRegisterProxy(HMODULE hDll,
                                  const ProxyFileInfo **pProxyFileList,
                                  const CLSID *pclsid)
{
  static const WCHAR bothW[] = {'B','o','t','h',0};
  static const WCHAR clsidW[] = {'C','L','S','I','D','\\',0};
  static const WCHAR clsid32W[] = {'P','r','o','x','y','S','t','u','b','C','l','s','i','d','3','2',0};
  static const WCHAR interfaceW[] = {'I','n','t','e','r','f','a','c','e','\\',0};
  static const WCHAR psfactoryW[] = {'P','S','F','a','c','t','o','r','y','B','u','f','f','e','r',0};
  static const WCHAR numformatW[] = {'%','u',0};
  static const WCHAR nummethodsW[] = {'N','u','m','M','e','t','h','o','d','s',0};
  static const WCHAR inprocserverW[] = {'I','n','P','r','o','c','S','e','r','v','e','r','3','2',0};
  static const WCHAR threadingmodelW[] = {'T','h','r','e','a','d','i','n','g','M','o','d','e','l',0};
  WCHAR clsid[39], keyname[50], module[MAX_PATH];
  HKEY key, subkey;
  DWORD len;

  TRACE("(%p,%p,%s)\n", hDll, pProxyFileList, debugstr_guid(pclsid));
  format_clsid( clsid, pclsid );

  /* register interfaces to point to clsid */
  while (*pProxyFileList) {
    unsigned u;
    for (u=0; u<(*pProxyFileList)->TableSize; u++) {
      CInterfaceStubVtbl *proxy = (*pProxyFileList)->pStubVtblList[u];
      PCInterfaceName name = (*pProxyFileList)->pNamesArray[u];

      TRACE("registering %s %s => %s\n",
            debugstr_a(name), debugstr_guid(proxy->header.piid), debugstr_w(clsid));

      strcpyW( keyname, interfaceW );
      format_clsid( keyname + strlenW(keyname), proxy->header.piid );
      if (RegCreateKeyW(HKEY_CLASSES_ROOT, keyname, &key) == ERROR_SUCCESS) {
        WCHAR num[10];
        if (name)
          RegSetValueExA(key, NULL, 0, REG_SZ, (const BYTE *)name, strlen(name)+1);
        RegSetValueW( key, clsid32W, REG_SZ, clsid, 0 );
        sprintfW(num, numformatW, proxy->header.DispatchTableCount);
        RegSetValueW( key, nummethodsW, REG_SZ, num, 0 );
        RegCloseKey(key);
      }
    }
    pProxyFileList++;
  }

  /* register clsid to point to module */
  strcpyW( keyname, clsidW );
  strcatW( keyname, clsid );
  len = GetModuleFileNameW(hDll, module, sizeof(module)/sizeof(WCHAR));
  if (len && len < sizeof(module)) {
      TRACE("registering CLSID %s => %s\n", debugstr_w(clsid), debugstr_w(module));
      if (RegCreateKeyW(HKEY_CLASSES_ROOT, keyname, &key) == ERROR_SUCCESS) {
          RegSetValueExW(subkey, NULL, 0, REG_SZ, (const BYTE *)psfactoryW, sizeof(psfactoryW));
          if (RegCreateKeyW(key, inprocserverW, &subkey) == ERROR_SUCCESS) {
              RegSetValueExW(subkey, NULL, 0, REG_SZ, (LPBYTE)module, (strlenW(module)+1)*sizeof(WCHAR));
              RegSetValueExW(subkey, threadingmodelW, 0, REG_SZ, (const BYTE *)bothW, sizeof(bothW));
              RegCloseKey(subkey);
          }
          RegCloseKey(key);
      }
  }

  return S_OK;
}

/***********************************************************************
 *           NdrDllUnregisterProxy [RPCRT4.@]
 */
HRESULT WINAPI NdrDllUnregisterProxy(HMODULE hDll,
                                    const ProxyFileInfo **pProxyFileList,
                                    const CLSID *pclsid)
{
  static const WCHAR clsidW[] = {'C','L','S','I','D','\\',0};
  static const WCHAR interfaceW[] = {'I','n','t','e','r','f','a','c','e','\\',0};
  WCHAR keyname[50];

  TRACE("(%p,%p,%s)\n", hDll, pProxyFileList, debugstr_guid(pclsid));

  /* unregister interfaces */
  while (*pProxyFileList) {
    unsigned u;
    for (u=0; u<(*pProxyFileList)->TableSize; u++) {
      CInterfaceStubVtbl *proxy = (*pProxyFileList)->pStubVtblList[u];
      PCInterfaceName name = (*pProxyFileList)->pNamesArray[u];

      TRACE("unregistering %s %s\n", debugstr_a(name), debugstr_guid(proxy->header.piid));

      strcpyW( keyname, interfaceW );
      format_clsid( keyname + strlenW(keyname), proxy->header.piid );
      RegDeleteTreeW(HKEY_CLASSES_ROOT, keyname);
    }
    pProxyFileList++;
  }

  /* unregister clsid */
  strcpyW( keyname, clsidW );
  format_clsid( keyname + strlenW(keyname), pclsid );
  RegDeleteTreeW(HKEY_CLASSES_ROOT, keyname);

  return S_OK;
}
