// PNGFilter.cpp : Implementation of DLL Exports.

// You will need the NT SUR Beta 2 SDK or VC 4.2 in order to build this 
// project.  This is because you will need MIDL 3.00.15 or higher and new
// headers and libs.  If you have VC 4.2 installed, then everything should
// already be configured correctly.

// Note: Proxy/Stub Information
//		To build a separate proxy/stub DLL, 
//		run nmake -f WMFFilterps.mak in the project directory.

#include "stdafx.h"
#include "resource.h"
#include <atlimpl.cpp>
#include "initguid.h"
#ifdef UNIX
#  include "pngfilt.h"
#else
#  include "include\pngfilt.h"
#endif
#include "cpngfilt.h"
#include <advpub.h>

#define IID_DEFINED
#ifdef UNIX
#  include "pngfilt.ic"
#else
#  include "include\pngfilt.ic"
#endif

#pragma warning( disable: 4505 )

HRESULT WriteMIMEKeys(LPCTSTR lpszCLSID, LPTSTR lpszMIME, int nBytes, BYTE * pbID);

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_CoPNGFilter, CPNGFilter)
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
      ATLTRACE(_T("PNGFilt.dll attach\n"));
	}
	else if (dwReason == DLL_PROCESS_DETACH)
   {
      ATLTRACE(_T("PNGFilt.dll detach\n"));
		_Module.Term();
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

BYTE byPNGID[] = {   0x08, 0x00, 0x00, 0x00,                    // length
                     0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,   // mask
                     0x89,0x50,0x4E,0x47,0x0D,0x0A,0x1A,0x0A    // data
                   };


STDAPI ie3_DllRegisterServer(void)
{
    HRESULT hr;
	// registers object, typelib and all interfaces in typelib
	hr = _Module.RegisterServer(FALSE);
    if (FAILED(hr))
        return hr;

    hr = WriteMIMEKeys(_T("{A3CCEDF7-2DE2-11D0-86F4-00A0C913F750}"), _T("image/png"), sizeof(byPNGID), byPNGID);
    if (FAILED(hr))
        return hr;

    hr = WriteMIMEKeys(_T("{A3CCEDF7-2DE2-11D0-86F4-00A0C913F750}"), _T("image/x-png"), sizeof(byPNGID), byPNGID);

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI ie3_DllUnregisterServer(void)
{
    _Module.UnregisterServer();
    return S_OK;
}

TCHAR szDatabase[] = _T("MIME\\Database\\Content Type\\");
TCHAR szBits[] = _T("Bits");

HRESULT WriteMIMEKeys(LPCTSTR lpszCLSID, LPTSTR lpszMIME, int nBytes, BYTE * pbID)
{
    TCHAR szBuf[MAX_PATH];
    HKEY hkey, hkey2;
    DWORD dw;

    lstrcpy(szBuf, szDatabase);
    lstrcat(szBuf, lpszMIME);
    RegCreateKeyEx(HKEY_CLASSES_ROOT, szBuf, 0, NULL, 0, KEY_WRITE, NULL, &hkey, &dw);
    
    RegSetValueEx(hkey, _T("Image Filter CLSID"), 0, REG_SZ, (LPBYTE)lpszCLSID, lstrlen(lpszCLSID)+1);
    
    RegCreateKeyEx(hkey, szBits, 0, NULL, 0, KEY_WRITE, NULL, &hkey2, &dw);
    RegSetValueEx(hkey2, _T("0"), 0, REG_BINARY, pbID, nBytes);

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
