/* 
 * IDxDiagContainer Implementation
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
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(dxdiag);

/* IDxDiagContainer IUnknown parts follow: */
HRESULT WINAPI IDxDiagContainerImpl_QueryInterface(PDXDIAGCONTAINER iface, REFIID riid, LPVOID *ppobj)
{
    IDxDiagContainerImpl *This = (IDxDiagContainerImpl *)iface;

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDxDiagContainer)) {
        IDxDiagContainerImpl_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
    return E_NOINTERFACE;
}

ULONG WINAPI IDxDiagContainerImpl_AddRef(PDXDIAGCONTAINER iface) {
    IDxDiagContainerImpl *This = (IDxDiagContainerImpl *)iface;
    TRACE("(%p) : AddRef from %ld\n", This, This->ref);
    return ++(This->ref);
}

ULONG WINAPI IDxDiagContainerImpl_Release(PDXDIAGCONTAINER iface) {
    IDxDiagContainerImpl *This = (IDxDiagContainerImpl *)iface;
    ULONG ref = --This->ref;
    TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
    if (ref == 0) {
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* IDxDiagContainer Interface follow: */
HRESULT WINAPI IDxDiagContainerImpl_GetNumberOfChildContainers(PDXDIAGCONTAINER iface, DWORD* pdwCount) {
  IDxDiagContainerImpl *This = (IDxDiagContainerImpl *)iface;
  TRACE("(%p)\n", iface);
  if (NULL == pdwCount) {
    return E_INVALIDARG;
  }
  *pdwCount = This->nSubContainers;
  return S_OK;
}

HRESULT WINAPI IDxDiagContainerImpl_EnumChildContainerNames(PDXDIAGCONTAINER iface, DWORD dwIndex, LPWSTR pwszContainer, DWORD cchContainer) {
  IDxDiagContainerImpl_SubContainer* p = NULL;
  DWORD i = 0;
  IDxDiagContainerImpl *This = (IDxDiagContainerImpl *)iface;
  
  TRACE("(%p, %lu, %s, %lu)\n", iface, dwIndex, debugstr_w(pwszContainer), cchContainer);

  if (NULL == pwszContainer) {
    return E_INVALIDARG;
  }
  if (256 > cchContainer) {
    return DXDIAG_E_INSUFFICIENT_BUFFER;
  }
  
  p = This->subContainers;
  while (NULL != p) {
    if (dwIndex == i) {  
      if (cchContainer <= strlenW(p->contName)) {
	return DXDIAG_E_INSUFFICIENT_BUFFER;
      }
      lstrcpynW(pwszContainer, p->contName, cchContainer);
      return S_OK;
    }
    p = p->next;
    ++i;
  }  
  return E_INVALIDARG;
}

HRESULT WINAPI IDxDiagContainerImpl_GetChildContainer(PDXDIAGCONTAINER iface, LPCWSTR pwszContainer, IDxDiagContainer** ppInstance) {
  IDxDiagContainerImpl_SubContainer* p = NULL;
  IDxDiagContainerImpl *This = (IDxDiagContainerImpl *)iface;

  FIXME("(%p, %s, %p)\n", iface, debugstr_w(pwszContainer), ppInstance);

  if (NULL == ppInstance || NULL == pwszContainer) {
    return E_INVALIDARG;
  }

  p = This->subContainers;
  while (NULL != p) {
    if (0 == lstrcmpW(p->contName, pwszContainer)) {      
      IDxDiagContainerImpl_QueryInterface((PDXDIAGCONTAINER)p, &IID_IDxDiagContainer, (void**) ppInstance);
      return S_OK;
    }
    p = p->next;
  }

  return E_INVALIDARG;
}

HRESULT WINAPI IDxDiagContainerImpl_GetNumberOfProps(PDXDIAGCONTAINER iface, DWORD* pdwCount) {
  /* IDxDiagContainerImpl *This = (IDxDiagContainerImpl *)iface; */
  FIXME("(%p, %p): stub\n", iface, pdwCount);
  return S_OK;
}

HRESULT WINAPI IDxDiagContainerImpl_EnumPropNames(PDXDIAGCONTAINER iface, DWORD dwIndex, LPWSTR pwszPropName, DWORD cchPropName) {
  /* IDxDiagContainerImpl *This = (IDxDiagContainerImpl *)iface; */
  FIXME("(%p, %lu, %s, %lu): stub\n", iface, dwIndex, debugstr_w(pwszPropName), cchPropName);
  return S_OK;
}

HRESULT WINAPI IDxDiagContainerImpl_GetProp(PDXDIAGCONTAINER iface, LPCWSTR pwszPropName, VARIANT* pvarProp) {
  /* IDxDiagContainerImpl *This = (IDxDiagContainerImpl *)iface; */
  FIXME("(%p, %s, %p): stub\n", iface, debugstr_w(pwszPropName), pvarProp);
  return S_OK;
}


IDxDiagContainerVtbl DxDiagContainer_Vtbl =
{
    IDxDiagContainerImpl_QueryInterface,
    IDxDiagContainerImpl_AddRef,
    IDxDiagContainerImpl_Release,
    IDxDiagContainerImpl_GetNumberOfChildContainers,
    IDxDiagContainerImpl_EnumChildContainerNames,
    IDxDiagContainerImpl_GetChildContainer,
    IDxDiagContainerImpl_GetNumberOfProps,
    IDxDiagContainerImpl_EnumPropNames,
    IDxDiagContainerImpl_GetProp
};


HRESULT DXDiag_CreateDXDiagContainer(REFIID riid, LPVOID *ppobj) {
  IDxDiagContainerImpl* container;

  TRACE("(%p, %p)\n", debugstr_guid(riid), ppobj);
  
  container = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDxDiagContainerImpl));
  if (NULL == container) {
    *ppobj = NULL;
    return E_OUTOFMEMORY;
  }
  container->lpVtbl = &DxDiagContainer_Vtbl;
  container->ref = 0; /* will be inited with QueryInterface */
  return IDxDiagContainerImpl_QueryInterface((PDXDIAGCONTAINER)container, riid, ppobj);
}
