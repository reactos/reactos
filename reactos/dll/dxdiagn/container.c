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
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(ref before=%lu)\n", This, refCount - 1);

    DXDIAGN_LockModule();

    return refCount;
}

ULONG WINAPI IDxDiagContainerImpl_Release(PDXDIAGCONTAINER iface) {
    IDxDiagContainerImpl *This = (IDxDiagContainerImpl *)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(ref before=%lu)\n", This, refCount + 1);

    if (!refCount) {
        HeapFree(GetProcessHeap(), 0, This);
    }

    DXDIAGN_UnlockModule();
    
    return refCount;
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
  IDxDiagContainerImpl *This = (IDxDiagContainerImpl *)iface;
  IDxDiagContainerImpl_SubContainer* p = NULL;
  DWORD i = 0;
  
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

HRESULT WINAPI IDxDiagContainerImpl_GetChildContainerInternal(PDXDIAGCONTAINER iface, LPCWSTR pwszContainer, IDxDiagContainer** ppInstance) {
  IDxDiagContainerImpl *This = (IDxDiagContainerImpl *)iface;
  IDxDiagContainerImpl_SubContainer* p = NULL;

  p = This->subContainers;
  while (NULL != p) {
    if (0 == lstrcmpW(p->contName, pwszContainer)) {      
      *ppInstance = (PDXDIAGCONTAINER)p->pCont;
      return S_OK;
    }
    p = p->next;
  }
  return E_INVALIDARG;
}

HRESULT WINAPI IDxDiagContainerImpl_GetChildContainer(PDXDIAGCONTAINER iface, LPCWSTR pwszContainer, IDxDiagContainer** ppInstance) {
  IDxDiagContainerImpl *This = (IDxDiagContainerImpl *)iface;
  IDxDiagContainer* pContainer = NULL;
  LPWSTR tmp, orig_tmp;
  INT tmp_len;
  WCHAR* cur;
  HRESULT hr = E_INVALIDARG;

  FIXME("(%p, %s, %p)\n", iface, debugstr_w(pwszContainer), ppInstance);

  if (NULL == ppInstance || NULL == pwszContainer) {
    return E_INVALIDARG;
  }

  pContainer = (PDXDIAGCONTAINER) This;

  tmp_len = strlenW(pwszContainer) + 1;
  orig_tmp = tmp = HeapAlloc(GetProcessHeap(), 0, tmp_len * sizeof(WCHAR));
  if (NULL == tmp) return E_FAIL;
  lstrcpynW(tmp, pwszContainer, tmp_len);

  cur = strchrW(tmp, '.');
  while (NULL != cur) {
    *cur = '\0'; /* cut tmp string to '.' */
    hr = IDxDiagContainerImpl_GetChildContainerInternal(pContainer, tmp, &pContainer);
    if (!SUCCEEDED(hr) || NULL == pContainer)
      goto on_error;
    *cur++; /* go after '.' (just replaced by \0) */
    tmp = cur;
    cur = strchrW(tmp, '.');
  }

  hr = IDxDiagContainerImpl_GetChildContainerInternal(pContainer, tmp, ppInstance);
  if (SUCCEEDED(hr)) {
    IDxDiagContainerImpl_AddRef((PDXDIAGCONTAINER)*ppInstance);
  }

on_error:
  HeapFree(GetProcessHeap(), 0, orig_tmp);
  return hr;
}

HRESULT WINAPI IDxDiagContainerImpl_GetNumberOfProps(PDXDIAGCONTAINER iface, DWORD* pdwCount) {
  IDxDiagContainerImpl *This = (IDxDiagContainerImpl *)iface;
  TRACE("(%p)\n", iface);
  if (NULL == pdwCount) {
    return E_INVALIDARG;
  }
  *pdwCount = This->nProperties;
  return S_OK;
}

HRESULT WINAPI IDxDiagContainerImpl_EnumPropNames(PDXDIAGCONTAINER iface, DWORD dwIndex, LPWSTR pwszPropName, DWORD cchPropName) {
  IDxDiagContainerImpl *This = (IDxDiagContainerImpl *)iface;
  IDxDiagContainerImpl_Property* p = NULL;
  DWORD i = 0;
  
  FIXME("(%p, %lu, %s, %lu)\n", iface, dwIndex, debugstr_w(pwszPropName), cchPropName);

  if (NULL == pwszPropName) {
    return E_INVALIDARG;
  }
  if (256 > cchPropName) {
    return DXDIAG_E_INSUFFICIENT_BUFFER;
  }
  
  p = This->properties;
  while (NULL != p) {
    if (dwIndex == i) {  
      if (cchPropName <= lstrlenW(p->vName)) {
	return DXDIAG_E_INSUFFICIENT_BUFFER;
      }
      lstrcpynW(pwszPropName, p->vName, cchPropName);
      return S_OK;
    }
    p = p->next;
    ++i;
  }  
  return E_INVALIDARG;
}

HRESULT WINAPI IDxDiagContainerImpl_GetProp(PDXDIAGCONTAINER iface, LPCWSTR pwszPropName, VARIANT* pvarProp) {
  IDxDiagContainerImpl *This = (IDxDiagContainerImpl *)iface;
  IDxDiagContainerImpl_Property* p = NULL;
  FIXME("(%p, %s, %p)\n", iface, debugstr_w(pwszPropName), pvarProp);

  if (NULL == pvarProp || NULL == pwszPropName) {
    return E_INVALIDARG;
  }

  p = This->properties;
  while (NULL != p) {
    if (0 == lstrcmpW(p->vName, pwszPropName)) {      
      VariantCopy(pvarProp, &p->v);
      return S_OK;
    }
    p = p->next;
  }
  return S_OK;
}

HRESULT WINAPI IDxDiagContainerImpl_AddProp(PDXDIAGCONTAINER iface, LPCWSTR pwszPropName, VARIANT* pVarProp) {
  IDxDiagContainerImpl *This = (IDxDiagContainerImpl *)iface;
  IDxDiagContainerImpl_Property* p = NULL;
  IDxDiagContainerImpl_Property* pNew = NULL;

  FIXME("(%p, %s, %p)\n", iface, debugstr_w(pwszPropName), pVarProp);

  if (NULL == pVarProp || NULL == pwszPropName) {
    return E_INVALIDARG;
  }

  pNew =  HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDxDiagContainerImpl_Property));
  if (NULL == pNew) {
    return E_OUTOFMEMORY;
  }
  VariantInit(&pNew->v);
  VariantCopy(&pNew->v, pVarProp);
  pNew->vName = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (lstrlenW(pwszPropName) + 1) * sizeof(WCHAR));
  lstrcpyW(pNew->vName, pwszPropName);
  pNew->next = NULL;

  p = This->properties;
  if (NULL == p) {
    This->properties = pNew;
  } else {
    while (NULL != p->next) {
      p = p->next;
    }
    p->next = pNew;
  }
  ++This->nProperties;
  return S_OK;
}

HRESULT WINAPI IDxDiagContainerImpl_AddChildContainer(PDXDIAGCONTAINER iface, LPCWSTR pszContName, PDXDIAGCONTAINER pSubCont) {
  IDxDiagContainerImpl *This = (IDxDiagContainerImpl *)iface;
  IDxDiagContainerImpl_SubContainer* p = NULL;
  IDxDiagContainerImpl_SubContainer* pNew = NULL;

  FIXME("(%p, %s, %p)\n", iface, debugstr_w(pszContName), pSubCont);

  if (NULL == pSubCont || NULL == pszContName) {
    return E_INVALIDARG;
  }

  pNew =  HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDxDiagContainerImpl_SubContainer));
  if (NULL == pNew) {
    return E_OUTOFMEMORY;
  }
  pNew->pCont = pSubCont;
  pNew->contName = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (lstrlenW(pszContName) + 1) * sizeof(WCHAR));
  lstrcpyW(pNew->contName, pszContName);
  pNew->next = NULL;

  p = This->subContainers;
  if (NULL == p) {
    This->subContainers = pNew;
  } else {
    while (NULL != p->next) {
      p = p->next;
    }
    p->next = pNew;
  }
  ++This->nSubContainers;
  return S_OK;
}

static const IDxDiagContainerVtbl DxDiagContainer_Vtbl =
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
