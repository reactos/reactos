// BitmapSurfaces.cpp : Implementation of DLL Exports.

// You will need the NT SUR Beta 2 SDK or VC 4.2 in order to build this 
// project.  This is because you will need MIDL 3.00.15 or higher and new
// headers and libs.  If you have VC 4.2 installed, then everything should
// already be configured correctly.

// Note: Proxy/Stub Information
//		To merge the proxy/stub code into the object DLL, add the file 
//		dlldatax.c to the project.  Make sure precompiled headers 
//		are turned off for this file, and add _MERGE_PROXYSTUB to the 
//		defines for the project.  
//
//		Modify the custom build rule for BitmapSurfaces.idl by adding the following 
//		files to the Outputs.  You can select all of the .IDL files by 
//		expanding each project and holding Ctrl while clicking on each of them.
//			BitmapSurfaces_p.c
//			dlldata.c
//		To build a separate proxy/stub DLL, 
//		run nmake -f BitmapSurfacesps.mak in the project directory.

#include "stdafx.h"
#include "resource.h"
#include "atlimpl.cpp"
#include <advpub.h>
#include "initguid.h"
#include "imgutil.h"
#include "csnfstrm.h"
#include "cmimeid.h"
#include "cmapmime.h"
#include "cdithtbl.h"
#include "dithers.h"
#include "cdith8.h"
#include "dlldatax.h"

#ifdef _MERGE_PROXYSTUB
extern "C" HINSTANCE hProxyDll;
#endif

CComModule _Module;

BEGIN_OBJECT_MAP( ObjectMap )
   OBJECT_ENTRY( CLSID_CoSniffStream, CSniffStream )
   OBJECT_ENTRY( CLSID_CoMapMIMEToCLSID, CMapMIMEToCLSID )
#ifndef MINSUPPORT   
   OBJECT_ENTRY( CLSID_CoDitherToRGB8, CDitherToRGB8 )
#endif   
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain( HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved )
{
	lpReserved;
#ifdef _MERGE_PROXYSTUB
	if (!PrxDllMain(hInstance, dwReason, lpReserved))
		return FALSE;
#endif
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		_Module.Init(ObjectMap, hInstance);
		DisableThreadLibraryCalls(hInstance);
      ATLTRACE(_T("ImgUtil.Dll DLL_PROCESS_ATTACH\n"));
      InitMIMEIdentifier();
      InitDefaultMappings();
#ifndef MINSUPPORT      
      CDitherToRGB8::InitTableCache();
#endif      
	}
	else if (dwReason == DLL_PROCESS_DETACH)
   {
      ATLTRACE(_T("ImgUtil.Dll DLL_PROCESS_DETACH\n"));
#ifndef MINSUPPORT      
      CDitherToRGB8::CleanupTableCache();
#endif      
      CleanupDefaultMappings();
		_Module.Term();
   }

	return TRUE;    // ok
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
#ifdef _MERGE_PROXYSTUB
	if (PrxDllCanUnloadNow() != S_OK)
		return S_FALSE;
#endif
	return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
#ifdef _MERGE_PROXYSTUB
	if (PrxDllGetClassObject(rclsid, riid, ppv) == S_OK)
		return S_OK;
#endif
   return( _Module.GetClassObject(rclsid, riid, ppv) );
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI ie3_DllRegisterServer(void)
{
#ifdef _MERGE_PROXYSTUB
	HRESULT hRes = PrxDllRegisterServer();
	if (FAILED(hRes))
		return hRes;
#endif
	// registers object, typelib and all interfaces in typelib
	return _Module.RegisterServer(FALSE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI ie3_DllUnregisterServer(void)
{
#ifdef _MERGE_PROXYSTUB
	PrxDllUnregisterServer();
#endif
	_Module.UnregisterServer();
	return S_OK;
}

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

STDAPI ie4_DllRegisterServer(void)
{
    REGINSTALL pfnReg = GetRegInstallFn();
	HRESULT hr;

#ifdef _MERGE_PROXYSTUB
	HRESULT hRes = PrxDllRegisterServer();
	if (FAILED(hRes))
		return hRes;
#endif

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
ie4_DllUnregisterServer(void)
{
    REGINSTALL pfnReg = GetRegInstallFn();
	HRESULT hr;
	
#ifdef _MERGE_PROXYSTUB
	PrxDllUnregisterServer();
#endif

	if (pfnReg == NULL)
		return E_FAIL;

    hr = (*pfnReg)( _Module.GetResourceInstance(), "UnReg", NULL);

    UnloadAdvPack();

    return hr;
}

STDAPI DllRegisterServer(void)
{
    REGINSTALL pfnReg = GetRegInstallFn();
    UnloadAdvPack();

    if (pfnReg)
        return ie4_DllRegisterServer();
    else
        return ie3_DllRegisterServer();
}

STDAPI
DllUnregisterServer(void)
{
    REGINSTALL pfnReg = GetRegInstallFn();
    UnloadAdvPack();

    if (pfnReg)
        return ie4_DllUnregisterServer();
    else
        return ie3_DllUnregisterServer();
}
