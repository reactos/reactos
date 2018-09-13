// GIFFilter.cpp : Implementation of DLL Exports.

// You will need the NT SUR Beta 2 SDK or VC 4.2 in order to build this 
// project.  This is because you will need MIDL 3.00.15 or higher and new
// headers and libs.  If you have VC 4.2 installed, then everything should
// already be configured correctly.

// Note: Proxy/Stub Information
//		To build a separate proxy/stub DLL, 
//		run nmake -f GIFFilterps.mak in the project directory.

// Disable "unreferenced local function removed" warning

#include "stdafx.h"
#include "resource.h"
#include "atlimpl.cpp"
#include "initguid.h"
#include "Include\GIFFilt.h"
#include "CGIFFilt.H"
#include <advpub.h>

#define IID_DEFINED
#include "Include\GIFFilt.ic"

#pragma warning( disable: 4505 )

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_CoGIFFilter, CGIFFilter)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
      ATLTRACE( "GIFFilt.dll attach\n" );
		_Module.Init(ObjectMap, hInstance);
		DisableThreadLibraryCalls(hInstance);
	}
	else if (dwReason == DLL_PROCESS_DETACH)
   {
		_Module.Term();
      ATLTRACE( "GIFFilt.dll detach\n" );
   }

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

/*
STDAPI DllRegisterServer(void)
{
	// registers object, typelib and all interfaces in typelib
	return _Module.RegisterServer(FALSE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
	_Module.UnregisterServer();
	return S_OK;
}
*/

static HINSTANCE hAdvPackLib;

REGINSTALL GetRegInstallFn(void)
{
    hAdvPackLib = LoadLibraryA("advpack.dll");
    if (!hAdvPackLib)
		return NULL;

    return (REGINSTALL)GetProcAddress(hAdvPackLib, achREGINSTALL);
}

inline void UnloadAdvPack(void)
{
    FreeLibrary(hAdvPackLib);
}

STDAPI DllRegisterServer(void)
{
    REGINSTALL pfnReg = GetRegInstallFn();
	HRESULT hr;

	if (pfnReg == NULL)
		return E_FAIL;
		
    // Delete any old registration entries, then add the new ones.
    hr = (*pfnReg)(_Module.GetResourceInstance(), "UnReg", NULL);
    if (SUCCEEDED(hr))
    	hr = (*pfnReg)(_Module.GetResourceInstance(), "Reg", NULL);

    UnloadAdvPack();
	
    return hr;
}

STDAPI
DllUnregisterServer(void)
{
    REGINSTALL pfnReg = GetRegInstallFn();
	HRESULT hr;
	
	if (pfnReg == NULL)
		return E_FAIL;

    hr = (*pfnReg)( _Module.GetResourceInstance(), "UnReg", NULL);

    UnloadAdvPack();

    return hr;
}


