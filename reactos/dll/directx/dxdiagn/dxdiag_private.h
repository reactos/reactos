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

#define COBJMACROS

#include <windows.h>
#if defined(_WIN32) && !defined(_NO_COM )
#define COM_NO_WINDOWS_H

#include <objbase.h>
#else
#define IUnknown void
#if !defined(NT_BUILD_ENVIRONMENT) && !defined(WINNT)
        #define CO_E_NOTINITIALIZED 0x800401F0
#endif
#endif







#include <dxdiag.h>

/* DXDiag Interfaces: */
typedef struct IDxDiagProviderImpl  IDxDiagProviderImpl;
typedef struct IDxDiagContainerImpl IDxDiagContainerImpl;

/* ---------------- */
/* IDxDiagProvider  */
/* ---------------- */

/*****************************************************************************
 * IDxDiagProvider implementation structure
 */

typedef struct _DXDIAGPROVIDER_INT
{
	/*0x00 */ LPVOID lpVbl;
	/*0x04 */ LPVOID lpVbl_private;
	/*0x08 */ DWORD Unkonwn1;
	/*0x0C */ DWORD Unkonwn2;
	/*0x10 */ DWORD RefCount;
	/*0x0C */ DWORD Unkonwn3;
	/*0x20 */ CHAR Unkonwn4;
	/*0x21 */ CHAR Unkonwn5;
	/*0x22 */ CHAR Unkonwn6;
	/*0x23 */ CHAR Unkonwn7;
	/*0x24 */ CHAR Unkonwn8;
	/*0x25 */ CHAR Unkonwn9;
	/*0x26 */ CHAR Unkonwn10;
	/*0x27 */ CHAR Unkonwn11;
	/*0x28 */ CHAR Unkonwn12;
	/*0x29 */ CHAR Unkonwn13;	
	/*0x2A */ CHAR Unkonwn14;	
	/*0x2C */ CHAR Unkonwn15;	
	/* wine */
	/* IDxDiagProvider fields */
	BOOL        init;
	DXDIAG_INIT_PARAMS params;
	IDxDiagContainer* pRootContainer;
} DXDIAGPROVIDER_INT, *LPDXDIAGPROVIDER_INT;

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

typedef struct IDxDiagContainerImpl_Property {
  LPWSTR vName;
  VARIANT v;
  struct IDxDiagContainerImpl_Property* next;
} IDxDiagContainerImpl_Property;


/*****************************************************************************
 * IDxDiagContainer implementation structure
 */
struct IDxDiagContainerImpl {
  /* IUnknown fields */
  const IDxDiagContainerVtbl *lpVtbl;
  LONG        ref;
  /* IDxDiagContainer fields */
  IDxDiagContainerImpl_Property* properties;
  IDxDiagContainerImpl_SubContainer* subContainers;
  DWORD nProperties;
  DWORD nSubContainers;
};

/* IUnknown: */
extern HRESULT WINAPI IDxDiagContainerImpl_QueryInterface(PDXDIAGCONTAINER iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI IDxDiagContainerImpl_AddRef(PDXDIAGCONTAINER iface);

/** Internal */
extern HRESULT WINAPI IDxDiagContainerImpl_AddProp(PDXDIAGCONTAINER iface, LPCWSTR pwszPropName, VARIANT* pVarProp);
extern HRESULT WINAPI IDxDiagContainerImpl_AddChildContainer(PDXDIAGCONTAINER iface, LPCWSTR pszContName, PDXDIAGCONTAINER pSubCont);


/** internal factory */
extern HRESULT DXDiag_CreateDXDiagContainer(REFIID riid, LPVOID *ppobj);
extern HRESULT DXDiag_InitRootDXDiagContainer(IDxDiagContainer* pRootCont);

/**********************************************************************
 * Dll lifetime tracking declaration for dxdiagn.dll
 */




/*****************************************************************************
 * IClassFactoryImpl implementation structure
 */


HRESULT WINAPI DXDiagCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj); 
ULONG WINAPI DXDiagCF_AddRef(LPCLASSFACTORY iface);
ULONG WINAPI DXDiagCF_Release(LPCLASSFACTORY iface);
HRESULT WINAPI DXDiagCF_CreateInstance(LPCLASSFACTORY iface,LPUNKNOWN pOuter,REFIID riid,LPVOID *ppobj);
HRESULT WINAPI DXDiagCF_LockServer(LPCLASSFACTORY iface,BOOL dolock);



typedef struct _ICLASSFACTORY_INT 
{
  LPVOID lpVbl;
  DWORD RefCount;  
  LPVOID Unknown1;
  LPVOID Unknown2;
} ICLASSFACTORY_INT, *LPICLASSFACTORY_INT;



ULONG WINAPI IDxDiagProviderImpl_ExecMethod(PDXDIAGPROVIDER iface);

#if defined( _WIN32 ) && !defined( _NO_COM )
    #undef INTERFACE
    #define INTERFACE IDxDiagProviderPrivate

    DECLARE_INTERFACE_( IDxDiagProviderPrivate, IUnknown )
    {
      STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
      STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
      STDMETHOD_(ULONG,Release) (THIS) PURE;
	  STDMETHOD_(ULONG,ExecMethod) (THIS) PURE;
	  
    };
     #if !defined(__cplusplus) || defined(CINTERFACE)
         #define IDxDiagProviderPrivate_QueryInterface(p, a, b)         (p)->lpVtbl->QueryInterface(p, a, b)
         #define IDxDiagProviderPrivate_AddRef(p)                       (p)->lpVtbl->AddRef(p)
         #define IDxDiagProviderPrivate_Release(p)                      (p)->lpVtbl->Release(p)
		 #define IDxDiagProviderPrivate_ExecMethod(p)                   (p)->lpVtbl->ExecMethod(p)
     #else
         #define IDxDiagProviderPrivate_QueryInterface(p, a, b)         (p)->QueryInterface(a, b)
         #define IDxDiagProviderPrivate_AddRef(p)                       (p)->AddRef()
         #define IDxDiagProviderPrivate_Release(p)                      (p)->Release()
		 #define IDxDiagProviderPrivate_ExecMethod(p)                   (p)->ExecMethod()
     #endif
#endif

typedef struct IDxDiagProviderPrivateVtbl DXDIAGPROVIDERPRIVARECALLBACK;
typedef DXDIAGPROVIDERPRIVARECALLBACK  *LPDXDIAGPROVIDERPRIVARECALLBACK;

#endif
