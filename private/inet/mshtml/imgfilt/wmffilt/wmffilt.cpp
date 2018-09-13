// WMFFilter.cpp : Implementation of DLL Exports.

// You will need the NT SUR Beta 2 SDK or VC 4.2 in order to build this 
// project.  This is because you will need MIDL 3.00.15 or higher and new
// headers and libs.  If you have VC 4.2 installed, then everything should
// already be configured correctly.

// Note: Proxy/Stub Information
//		To build a separate proxy/stub DLL, 
//		run nmake -f WMFFilterps.mak in the project directory.

// Disable "unreferenced local function removed" warning

#include "stdafx.h"
#include "resource.h"
#include "atlimpl.cpp"
#include "initguid.h"
#include "Include\WMFFilt.h"
#include "CWMFFilt.H"
#include <advpub.h>

#define IID_DEFINED
#include "Include\WMFFilt.ic"

HRESULT WriteMIMEKeys(LPCSTR lpszCLSID, LPSTR lpszMIME, int nBytes, BYTE * pbID);


#pragma warning( disable: 4505 )

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_CoWMFFilter, CWMFFilter)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
      ATLTRACE( "WMFFilt.dll attach\n" );
		_Module.Init(ObjectMap, hInstance);
		DisableThreadLibraryCalls(hInstance);
	}
	else if (dwReason == DLL_PROCESS_DETACH)
   {
		_Module.Term();
      ATLTRACE( "WMFFilt.dll detach\n" );
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

BYTE byID[] = {   0x04, 0x00, 0x00, 0x00,                    // length
                     0xFF,0xFF,0xFF,0xFF,   // mask
                     0xD7,0xCD,0xC6,0x9A   // data
                   };


STDAPI ie3_DllRegisterServer(void)
{
    HRESULT hr;
	// registers object, typelib and all interfaces in typelib
	hr = _Module.RegisterServer(FALSE);
    if (FAILED(hr))
        return hr;

    hr = WriteMIMEKeys("{EC3DB3C6-333E-11D0-B724-00AA006C1A01}", "image/x-wmf", sizeof(byID), byID);

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI ie3_DllUnregisterServer(void)
{
	_Module.UnregisterServer();
	return S_OK;
}



char szDatabase[] = "MIME\\Database\\Content Type\\";
char szBits[] = "Bits";

HRESULT WriteMIMEKeys(LPCSTR lpszCLSID, LPSTR lpszMIME, int nBytes, BYTE * pbID)
{
    char szBuf[MAX_PATH];
    HKEY hkey, hkey2;
    DWORD dw;

    lstrcpy(szBuf, szDatabase);
    lstrcat(szBuf, lpszMIME);
    RegCreateKeyEx(HKEY_CLASSES_ROOT, szBuf, 0, NULL, 0, KEY_WRITE, NULL, &hkey, &dw);
    
    RegSetValueEx(hkey, "Image Filter CLSID", 0, REG_SZ, (LPBYTE)lpszCLSID, lstrlen(lpszCLSID)+1);
    
    RegCreateKeyEx(hkey, szBits, 0, NULL, 0, KEY_WRITE, NULL, &hkey2, &dw);
    RegSetValueEx(hkey2, "0", 0, REG_BINARY, pbID, nBytes);

    RegCloseKey(hkey);
    RegCloseKey(hkey2);

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
