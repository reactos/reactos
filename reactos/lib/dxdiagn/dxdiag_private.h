/*
 * DXDiag private include file
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
 */

#ifndef __WINE_DXDIAG_PRIVATE_H
#define __WINE_DXDIAG_PRIVATE_H

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "objbase.h"
#include "oleauto.h"

#include "dxdiag.h"

/* DXDiag Interfaces: */
typedef struct IDxDiagProviderImpl  IDxDiagProviderImpl;
typedef struct IDxDiagContainerImpl IDxDiagContainerImpl;

/* ---------------- */
/* IDxDiagProvider  */
/* ---------------- */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern IDxDiagProviderVtbl DxDiagProvider_Vtbl;

/*****************************************************************************
 * IDxDiagProvider implementation structure
 */
struct IDxDiagProviderImpl {
  /* IUnknown fields */
  IDxDiagProviderVtbl *lpVtbl;
  DWORD       ref;
  /* IDxDiagProvider fields */
  BOOL        init;
  DXDIAG_INIT_PARAMS params;
  IDxDiagContainer* pRootContainer;
};

/* IUnknown: */
extern HRESULT WINAPI IDxDiagProviderImpl_QueryInterface(PDXDIAGPROVIDER iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI IDxDiagProviderImpl_AddRef(PDXDIAGPROVIDER iface);
extern ULONG WINAPI IDxDiagProviderImpl_Release(PDXDIAGPROVIDER iface);

/* IDxDiagProvider: */
extern HRESULT WINAPI IDxDiagProviderImpl_Initialize(PDXDIAGPROVIDER iface, DXDIAG_INIT_PARAMS* pParams);
extern HRESULT WINAPI IDxDiagProviderImpl_GetRootContainer(PDXDIAGPROVIDER iface, IDxDiagContainer** ppInstance);

/* ---------------- */
/* IDxDiagContainer  */
/* ---------------- */

typedef struct IDxDiagContainerImpl_SubContainer {
  IDxDiagContainer* pCont;
  WCHAR* contName;
  struct IDxDiagContainerImpl_SubContainer* next;
} IDxDiagContainerImpl_SubContainer;

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern IDxDiagContainerVtbl DxDiagContainer_Vtbl;

/*****************************************************************************
 * IDxDiagContainer implementation structure
 */
struct IDxDiagContainerImpl {
  /* IUnknown fields */
  IDxDiagContainerVtbl *lpVtbl;
  DWORD       ref;
  /* IDxDiagContainer fields */
  IDxDiagContainerImpl_SubContainer* subContainers;
  DWORD nSubContainers;
};

/* IUnknown: */
extern HRESULT WINAPI IDxDiagContainerImpl_QueryInterface(PDXDIAGCONTAINER iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI IDxDiagContainerImpl_AddRef(PDXDIAGCONTAINER iface);
extern ULONG WINAPI IDxDiagContainerImpl_Release(PDXDIAGCONTAINER iface);

/* IDxDiagContainer: */
extern HRESULT WINAPI IDxDiagContainerImpl_GetNumberOfChildContainers(PDXDIAGCONTAINER iface,  DWORD* pdwCount);
extern HRESULT WINAPI IDxDiagContainerImpl_EnumChildContainerNames(PDXDIAGCONTAINER iface, DWORD dwIndex, LPWSTR pwszContainer, DWORD cchContainer);
extern HRESULT WINAPI IDxDiagContainerImpl_GetChildContainer(PDXDIAGCONTAINER iface, LPCWSTR pwszContainer, IDxDiagContainer** ppInstance);
extern HRESULT WINAPI IDxDiagContainerImpl_GetNumberOfProps(PDXDIAGCONTAINER iface, DWORD* pdwCount);
extern HRESULT WINAPI IDxDiagContainerImpl_EnumPropNames(PDXDIAGCONTAINER iface, DWORD dwIndex, LPWSTR pwszPropName, DWORD cchPropName);
extern HRESULT WINAPI IDxDiagContainerImpl_GetProp(PDXDIAGCONTAINER iface, LPCWSTR pwszPropName, VARIANT* pvarProp);

/**
 * factories
 */
extern HRESULT DXDiag_CreateDXDiagProvider(LPCLASSFACTORY iface, LPUNKNOWN punkOuter, REFIID riid, LPVOID *ppobj);

/** internal factory */
extern HRESULT DXDiag_CreateDXDiagContainer(REFIID riid, LPVOID *ppobj);



#endif
