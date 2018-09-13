// RegwizCtrl.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//		To build a separate proxy/stub DLL, 
//		run nmake -f RegwizCtrlps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include "initguid.h"
#include "RegwizCtrl.h"

#include "RegwizCtrl_i.c"
#include "CRWCtrl.h"
#include "cathelp.h"


CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_RegWizCtrl, CRegWizCtrl)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		_Module.Init(ObjectMap, hInstance);
		DisableThreadLibraryCalls(hInstance);
	}
	else if (dwReason == DLL_PROCESS_DETACH)
		_Module.Term();
	return TRUE;    // ok
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
	return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
	return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
	HRESULT hr;
#ifdef _MERGE_PROXYSTUB
	HRESULT hRes = PrxDllRegisterServer();
	if (FAILED(hRes))
		return hRes;
#endif

	// registers object, typelib and all interfaces in typelib
	if ( SUCCEEDED(hr =_Module.RegisterServer(TRUE)) &&
		 SUCCEEDED(hr = CreateComponentCategory(CATID_SafeForScripting, 
						L"Controls that are safely scriptable")) &&
		 SUCCEEDED(hr = CreateComponentCategory(CATID_SafeForInitializing, 
						L"Controls safely initializable from persistent data")) &&
		 SUCCEEDED(hr = RegisterCLSIDInCategory(CLSID_RegWizCtrl, 
												CATID_SafeForScripting)) )
	{
		hr = RegisterCLSIDInCategory(CLSID_RegWizCtrl, CATID_SafeForInitializing);
	}

	return hr;
	
	
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
#ifdef _MERGE_PROXYSTUB
	PrxDllUnregisterServer();
#endif
	_Module.UnregisterServer();

	// Remove CATID information.
    UnRegisterCLSIDInCategory(CLSID_RegWizCtrl, CATID_SafeForScripting);
    UnRegisterCLSIDInCategory(CLSID_RegWizCtrl, CATID_SafeForInitializing);

	return S_OK;
}


