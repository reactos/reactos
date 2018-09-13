// vsengine.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//		To build a separate proxy/stub DLL, 
//		run nmake -f vsengineps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include "initguid.h"
#include "vsengine.h"

#include "vsengine_i.c"
#include "c:\nt\public\sdk\inc\vrsscan.h"
#include "Provider.h"

#define szVSEngine      "Microsoft VSEngine Test Driver"
#define wszVSEngine     L"Microsoft VSEngine Test Driver"

#define szCLSIDProvider "68E721E0-CD58-11D0-BD3D-00AA00B92AF1"
#define szCookie        "Cookie"


#define szVendorContactInfo         "VendorContactInfo"
#define szVendorContactInfoValue    "Microsoft Corp., Redmond, WA"

#define szVendorDescription         "VendorDescription"
#define szVendorDescriptionValue    "Microsoft VSEngine Test Driver"

#define szVendorFlags               "VendorFlags"
#define szVendorFlagsValue          15

#define szVendorIcon                "VendorIcon"
#define szVendorIconValue           "vsengine.ico"

#define WriteToRegistryStr(szKey,szValue)  \
if (RegSetValueEx(hkReg, (const char *)szKey, 0, REG_SZ, (BYTE *)szValue, lstrlen((const char *)szValue)+1) != ERROR_SUCCESS) {     \
        MessageBox(NULL,"Unable to write to registry (String).",szVSEngine,MB_OK); goto Exit; }

#define WriteToRegistryDword(szKey,dwValue)  \
dwLocalValue = dwValue;  \
if (RegSetValueEx(hkReg, (const char *)szKey, 0, REG_DWORD, (BYTE *)&dwLocalValue, sizeof(DWORD)) != ERROR_SUCCESS) {     \
        MessageBox(NULL,"Unable to write to registry (Dword).",szVSEngine,MB_OK); goto Exit; }

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_Provider, Provider)
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
	// registers object, typelib and all interfaces in typelib
	HRESULT hr, hr2 = _Module.RegisterServer(TRUE);
    HKEY hkReg = NULL;
    char RegName[2048];
    DWORD dwLocalValue;

    IUnknown *pUnk = NULL;
    IRegisterVirusScanEngine *prvs = NULL;

    if (hr2 != S_OK) {
        char tmp[2048];
        wsprintf(tmp,"Unable to register server (%08X)",hr2);
        MessageBox(NULL,tmp,szVSEngine,MB_OK);
        goto Exit;
    }
  
    wsprintf(RegName,"CLSID\\{%s}\\VirusScanner",szCLSIDProvider);

    if (RegOpenKeyEx(HKEY_CLASSES_ROOT,RegName,0,KEY_ALL_ACCESS,&hkReg) != ERROR_SUCCESS) {

        DWORD dwDisp = 0;
        if (RegCreateKeyEx(HKEY_CLASSES_ROOT, RegName, 0, NULL, REG_OPTION_NON_VOLATILE, 
            KEY_ALL_ACCESS, NULL, &hkReg, &dwDisp) != ERROR_SUCCESS) {

            MessageBox(NULL,"Unable open registry key 'VirusScanner'.",szVSEngine,MB_OK);
            goto Exit;

        }
    }


    WriteToRegistryStr(szVendorDescription, szVendorDescriptionValue);
    WriteToRegistryStr(szVendorContactInfo, szVendorContactInfoValue);
    WriteToRegistryDword(szVendorFlags, szVendorFlagsValue);
    WriteToRegistryStr(szVendorIcon, szVendorIconValue);

    hr = CoCreateInstance(CLSID_VirusScan, NULL, CLSCTX_INPROC_SERVER, IID_IUnknown, (void **)&pUnk);
    if (FAILED(hr)) {
        MessageBox(NULL,"Unable to register with CLSID_VirusScan",szVSEngine,MB_OK);
        DllUnregisterServer();
        return E_UNEXPECTED;
    }

    hr = pUnk->QueryInterface(IID_IRegisterVirusScanEngine, (void **)&prvs);
    pUnk->Release();
    
    if (FAILED(hr)) {
        MessageBox(NULL,"Unable to acquire IID_IRegisterVirusScanner",szVSEngine,MB_OK);
        DllUnregisterServer();
        return E_UNEXPECTED;
    }

    DWORD dwCookie;
    hr = prvs->RegisterScanEngine(CLSID_Provider,wszVSEngine,15,0,&dwCookie);
    
    // Release virus scan object
    prvs->Release();

    if (FAILED(hr)) {
        MessageBox(NULL,"CLSID_VirusScan failed to register CLSID_Provider",szVSEngine,MB_OK);
        DllUnregisterServer();
        return E_UNEXPECTED;
    }

    // Add cookie to registry

    WriteToRegistryDword(szCookie,dwCookie);

Exit:
    if (hkReg != NULL)
        RegCloseKey(hkReg);

    return hr2;
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
	_Module.UnregisterServer();

    // Don't bother cleaning up registry entries.

    return S_OK;
}


