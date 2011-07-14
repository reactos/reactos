// dllmain.cpp : Defines the entry point for the DLL application.



#include <windows.h>
#include <shlobj.h>
#include <shlobj_undoc.h>
#include <shlguid.h>
#include <shlguid_undoc.h>
#include <shlwapi.h>
#include <tchar.h>
#include <atlbase.h>
#include <atlcom.h>
#include <atlwin.h>
#include <initguid.h>
#include "stdafx.h"
#include "comtest.h"


extern const IID IID_IComTest;

CComModule gModule;


BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_ComTest, ComTest)
END_OBJECT_MAP()



STDAPI_(BOOL) DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID fImpLoad)
{
	OutputDebugStringW(L"DllMain");
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
		OutputDebugStringW(L"DLL_PROCESS_ATTACH");
		gModule.Init(ObjectMap, hInstance, NULL);
        DisableThreadLibraryCalls(hInstance);
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		OutputDebugStringW(L"DLL_PROCESS_DETACH");
		gModule.Term();
		break;
	}
	return TRUE;
}



STDAPI DllCanUnloadNow()
{
	OutputDebugStringW(L"DllCanUnloadNow");
	return gModule.DllCanUnloadNow();
}


STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
	OutputDebugStringW(L"DllGetClassObject");
	return gModule.DllGetClassObject(rclsid, riid, ppv);
}

STDAPI DllRegisterServer()
{
	OutputDebugStringW(L"DllRegisterServer");
	return gModule.DllRegisterServer(FALSE);
}

STDAPI DllUnregisterServer()
{
	OutputDebugStringW(L"DllUnregisterServer");
	return gModule.DllUnregisterServer(FALSE);
}
