// dllmain.cpp : Defines the entry point for the DLL application.




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


BOOL APIENTRY DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID fImpLoad)
{
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
		gModule.Init(ObjectMap, hInstance, NULL);
        DisableThreadLibraryCalls(hInstance);
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		gModule.Term();
		break;
	}
	return TRUE;
}



STDAPI DllCanUnloadNow()
{
	return gModule.DllCanUnloadNow();
}


STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
	return gModule.DllGetClassObject(rclsid, riid, ppv);
}

STDAPI DllRegisterServer()
{
	return gModule.DllRegisterServer(FALSE);
}

STDAPI DllUnregisterServer()
{
	return gModule.DllUnregisterServer(FALSE);
}
