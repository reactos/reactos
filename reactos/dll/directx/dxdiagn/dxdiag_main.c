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

#define INITGUID

DWORD g_cComponents;
DWORD g_cServerLocks;

WINE_DEFAULT_DEBUG_CHANNEL(dxdiag);

IClassFactoryVtbl DXDiagCF_Vtbl = 
{
  DXDiagCF_QueryInterface,
  DXDiagCF_AddRef,
  DXDiagCF_Release,
  DXDiagCF_CreateInstance,
  DXDiagCF_LockServer
};

IDxDiagProviderVtbl DxDiagProvider_Vtbl =
{
    IDxDiagProviderImpl_QueryInterface,
    IDxDiagProviderImpl_AddRef,
    IDxDiagProviderImpl_Release,
    IDxDiagProviderImpl_Initialize,
    IDxDiagProviderImpl_GetRootContainer
};

IDxDiagProviderPrivateVtbl DxDiagProvider_PrivateVtbl =
{
   IDxDiagProviderImpl_QueryInterface,
   IDxDiagProviderImpl_AddRef,
   IDxDiagProviderImpl_Release,
   IDxDiagProviderImpl_ExecMethod
};

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

HRESULT 
WINAPI 
DXDiagCF_QueryInterface(LPCLASSFACTORY iface, REFIID riid, LPVOID *ppobj) 
{
	LPICLASSFACTORY_INT This = (LPICLASSFACTORY_INT) iface;
	HRESULT retValue = S_OK;	

	if (ppobj == NULL)
	{
		retValue = E_INVALIDARG;
	}
	else
	{				
		if ( ( IsEqualGUID( riid, &IID_IUnknown) ) ||
			( IsEqualGUID( riid, &IID_IClassFactory) ) )
		{			
			*ppobj = This;	
			IClassFactory_AddRef( iface );
		}
		else
		{
			*ppobj = NULL;
			retValue = E_NOINTERFACE;
		}
	}
	return retValue;
}

ULONG 
WINAPI 
DXDiagCF_AddRef(LPCLASSFACTORY iface) 
{
	LPICLASSFACTORY_INT This = (LPICLASSFACTORY_INT) iface;
		
	return (ULONG) InterlockedIncrement( (LONG *) &This->RefCount );
}

ULONG 
WINAPI 
DXDiagCF_Release(LPCLASSFACTORY iface) 
{
	LPICLASSFACTORY_INT This = (LPICLASSFACTORY_INT) iface;
	ULONG RefCount = 0;

	RefCount = InterlockedDecrement( (LONG *) &This->RefCount );

	if (RefCount <= 0)
	{
		if (This->RefCount == 0)
		{		
			HeapFree(GetProcessHeap(), 0, This);
		}

		RefCount = 0;
	}
	
	return RefCount;
}


HRESULT 
WINAPI 
DXDiagCF_CreateInstance(LPCLASSFACTORY iface, LPUNKNOWN pOuter, REFIID riid, LPVOID *ppobj) 
{
	HRESULT retValue = S_OK;
	
	if ( *ppobj != NULL )
	{
		*ppobj = NULL;				
	}

	if ( pOuter == NULL )
	{		
		if ( IsEqualGUID( riid, &CLSID_DxDiagProvider ) )
		{		
			
			LPDXDIAGPROVIDER_INT myDxDiagProvider_int = (LPDXDIAGPROVIDER_INT) HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(DXDIAGPROVIDER_INT));
	
			if ( myDxDiagProvider_int == NULL)
			{
				*ppobj = NULL;
				retValue = E_OUTOFMEMORY;				
			}
			else
			{							
				myDxDiagProvider_int->lpVbl = (LPVOID) &DxDiagProvider_Vtbl;
				myDxDiagProvider_int->lpVbl_private = (LPVOID) &DxDiagProvider_PrivateVtbl;				
				IDxDiagProviderImpl_AddRef( (LPDXDIAGPROVIDER) myDxDiagProvider_int );
				InterlockedIncrement( (LONG *)&g_cComponents);
				retValue = IDxDiagProvider_QueryInterface( (LPDXDIAGPROVIDER) myDxDiagProvider_int, riid, ppobj );							
				IDxDiagProvider_Release( (LPDXDIAGPROVIDER) myDxDiagProvider_int );				
				*ppobj = (LPVOID) myDxDiagProvider_int;			
			}			
		}
		else
		{
			retValue = CLASS_E_CLASSNOTAVAILABLE;
		}
	}

	return retValue;
}

HRESULT 
WINAPI 
DXDiagCF_LockServer(LPCLASSFACTORY iface,BOOL dolock) 
{

	if (dolock == TRUE)
	{ 
		InterlockedIncrement( (LONG *)&g_cServerLocks);
	}

	return S_OK;
}




/***********************************************************************
 *             DllCanUnloadNow (DXDIAGN.@)
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
  return S_OK;
}

/***********************************************************************
 *		DllGetClassObject (DXDIAGN.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
	HRESULT retValue = S_OK;
	LPICLASSFACTORY_INT myIClassFactory_int;
	LPCLASSFACTORY This;
	

	if ( IsEqualGUID( rclsid, &CLSID_DxDiagProvider) )	
	{
		myIClassFactory_int = (LPICLASSFACTORY_INT) HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(ICLASSFACTORY_INT));
				
		if (myIClassFactory_int != NULL)
		{					
			myIClassFactory_int->lpVbl = (LPVOID) &DXDiagCF_Vtbl;

			This = (LPCLASSFACTORY) myIClassFactory_int;
						
			DXDiagCF_AddRef( This );
									
			retValue = IClassFactory_QueryInterface(  This, riid, ppv );	

			IClassFactory_AddRef( This ); 

			*ppv = (LPVOID) This;
		}
		else
		{
			retValue = E_OUTOFMEMORY;
			*ppv = NULL;
		}

	}
	else
	{
		retValue = CLASS_E_CLASSNOTAVAILABLE;
		*ppv = NULL;
	}

	return retValue;

}
