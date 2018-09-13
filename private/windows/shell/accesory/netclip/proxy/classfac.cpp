#include "windows.h"
#include "ole2.h"
#include "tchar.h"

#ifdef _FEATURE_SHELLEX
#include <shlobj.h>
#include <shlguid.h>
#endif

#include "classfac.h"

#ifdef _FEATURE_SHELLEX
#include "shellex.h"
// {B222AAAA-8813-11cf-A54D-080036F12502}
static const CLSID CLSID_NetClipShellEx = 
{ 0xb222aaaa, 0x8813, 0x11cf, { 0xa5, 0x4d, 0x8, 0x0, 0x36, 0xf1, 0x25, 0x2 } };

ULONG g_dwRefCount=0;

// Create a new database object and return a pointer to it
HRESULT CShellExFactory::CreateInstance(IUnknown *pUnkOuter, REFIID riid, void** ppObject) 
{
	if (pUnkOuter && riid!=IID_IUnknown)
	{
		*ppObject=NULL;
		return E_INVALIDARG;
	}
	CShellEx* pObj=new CShellEx();
	HRESULT hRes=pObj->Initialize(pUnkOuter);
	if (FAILED(hRes)) 
	{
		delete pObj;
		return hRes;
	}
	
	if (pUnkOuter)
	{
		*ppObject=(IUnknown*) (pObj->m_punkInner);
		pObj->m_punkInner->AddRef();
	}
	else if (FAILED(pObj->QueryInterface(riid, ppObject))) 
	{
		delete pObj;
		*ppObject=NULL;
		return E_NOINTERFACE;
	}
	return S_OK;
}

HRESULT	CShellExFactory::LockServer(BOOL fLock) 
{
	if (fLock) 
	{
		InterlockedIncrement((long*) &g_dwRefCount);
	}
	else 
	{
		InterlockedDecrement((long*) &g_dwRefCount);
	}
	return S_OK;
}

CShellExFactory::CShellExFactory() 
{
	m_dwRefCount=0;
}

HRESULT CShellExFactory::QueryInterface(REFIID riid, void** ppObject) 
{
	if (riid==IID_IUnknown || riid==IID_IClassFactory) 
	{
		*ppObject=(IClassFactory*) this;
	}
	else 
	{
		return E_NOINTERFACE;
	}
	AddRef();
	return S_OK;
}

ULONG CShellExFactory::AddRef() 
{
	InterlockedIncrement((long*) &g_dwRefCount);
	InterlockedIncrement((long*) &m_dwRefCount);
	return m_dwRefCount;
}

ULONG CShellExFactory::Release() 
{
    ULONG dwRefCount=m_dwRefCount-1;
	InterlockedDecrement((long*) &g_dwRefCount);
	if (InterlockedDecrement((long*) &m_dwRefCount)==0) 
	{
		delete this;
		return 0;
	}
	return dwRefCount;
}
#endif

// Declarations for PS functions
STDAPI PSDllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppObject);
STDAPI PSDllCanUnloadNow();
STDAPI PSDllRegisterServer(void);
STDAPI PSDllUnregisterServer(void);

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppObject) 
{
#ifdef _FEATURE_SHELLEX
	if (rclsid==CLSID_NetClipShellEx) 
	{
		CShellExFactory *pFactory= new CShellExFactory;
		if (FAILED(pFactory->QueryInterface(riid, ppObject))) 
		{
			delete pFactory;
			*ppObject=NULL;
			return E_INVALIDARG;
		}
	}
	else 
#endif
	{ 
		return PSDllGetClassObject(rclsid, riid, ppObject);
	}
#ifdef _FEATURE_SHELLEX
	return NO_ERROR;
#endif
}

STDAPI DllCanUnloadNow() 
{
	if (g_dwRefCount) 
	{
		return S_FALSE;
	}
	else 
	{
		return PSDllCanUnloadNow();
	}
}

STDAPI DllRegisterServer(void) 
{
	PSDllRegisterServer(); // Register ProxyStubs

#ifdef _FEATURE_SHELLEX
    HKEY hKeyCLSID, hKeyInproc32;
    HKEY hKey;
    TCHAR szValue[256];
	DWORD dwDisposition;

	if (RegCreateKeyEx(HKEY_CLASSES_ROOT, 
			_T("CLSID\\{B222AAAA-8813-11cf-A54D-080036F12502}"), 
			NULL, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, 
			&hKeyCLSID, &dwDisposition)!=ERROR_SUCCESS) 
	{
		return E_UNEXPECTED;
	}

	if (RegSetValueEx(hKeyCLSID, _T(""), NULL, REG_SZ, (BYTE*) _T("Remote Clipboard Shell Extension"), sizeof("Remote Clipboard Shell Extension")*sizeof(TCHAR))!=ERROR_SUCCESS) 
	{
		RegCloseKey(hKeyCLSID);
		return E_UNEXPECTED;
	}

	if (RegCreateKeyEx(hKeyCLSID, 
			_T("InprocServer32"), 
			NULL, _T(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, 
			&hKeyInproc32, &dwDisposition)!=ERROR_SUCCESS) 
	{
		RegCloseKey(hKeyCLSID);
		return E_UNEXPECTED;
	}

	HMODULE hModule;
	hModule=GetModuleHandle(_T("nclipps.DLL"));
	if (!hModule) 
	{
		RegCloseKey(hKeyInproc32);
		RegCloseKey(hKeyCLSID);
		return E_UNEXPECTED;
	}
	TCHAR szName[MAX_PATH+64];
	if (GetModuleFileName(hModule, szName, sizeof(szName))==0) 
	{
		RegCloseKey(hKeyInproc32);
		RegCloseKey(hKeyCLSID);
		return E_UNEXPECTED;
	}
	if (RegSetValueEx(hKeyInproc32, _T(""), NULL, REG_SZ, (BYTE*) szName, sizeof(TCHAR)*(lstrlen(szName)+1))!=ERROR_SUCCESS) 
	{
		RegCloseKey(hKeyInproc32);
		RegCloseKey(hKeyCLSID);
		return E_UNEXPECTED;
	}

    lstrcat(szValue, _T("Apartment"));
	if (RegSetValueEx(hKeyInproc32, _T("ThreadingModel"), NULL, REG_SZ, (BYTE*) szValue, sizeof(TCHAR)*(lstrlen(szValue)+1))!=ERROR_SUCCESS) 
	{
		RegCloseKey(hKeyInproc32);
		RegCloseKey(hKeyCLSID);
		return E_UNEXPECTED;
	}
    RegCloseKey(hKeyInproc32);

	if (RegCreateKeyEx(hKeyCLSID, 
			_T("DefaultIcon"), 
			NULL, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, 
			&hKeyInproc32, &dwDisposition)!=ERROR_SUCCESS) 
	{
		RegCloseKey(hKeyCLSID);
		return E_UNEXPECTED;
	}
    lstrcat(szName, _T(",0"));
	if (RegSetValueEx(hKeyInproc32, _T(""), NULL, REG_SZ, (BYTE*) szName, sizeof(TCHAR)*(lstrlen(szName)+1))!=ERROR_SUCCESS) 
	{
		RegCloseKey(hKeyInproc32);
		RegCloseKey(hKeyCLSID);
		return E_UNEXPECTED;
	}

    RegCloseKey(hKeyInproc32);
	RegCloseKey(hKeyCLSID);

    // [HKEY_CLASSES_ROOT\.{B222AAAA-8813-11cf-A54D-080036F12502}]
    //    @="NetClipFile"
	if (RegCreateKeyEx(HKEY_CLASSES_ROOT, 
			_T(".{B222AAAA-8813-11cf-A54D-080036F12502}"), 
			NULL, _T(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, 
			&hKey, &dwDisposition)!=ERROR_SUCCESS) 
	{
		return E_UNEXPECTED;
	}
    lstrcpy(szValue, _T("NetClipFile"));
	if (RegSetValueEx(hKey, _T(""), NULL, REG_SZ, (BYTE*) szValue, sizeof(TCHAR)*(lstrlen(szValue)+1))!=ERROR_SUCCESS) 
	{
		RegCloseKey(hKey);
		return E_UNEXPECTED;
	}
    RegCloseKey(hKey);

    // [HKEY_CLASSES_ROOT\.{B222AAAA-8813-11cf-A54D-080036F12502}\ShellNew]
    //    @="NetClipFile"
	if (RegCreateKeyEx(HKEY_CLASSES_ROOT, 
			_T(".{B222AAAA-8813-11cf-A54D-080036F12502}\\ShellNew"), 
			NULL, _T(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, 
			&hKey, &dwDisposition)!=ERROR_SUCCESS) 
	{
		return E_UNEXPECTED;
	}
    lstrcpy(szValue, _T(""));
	if (RegSetValueEx(hKey, _T("NullFile"), NULL, REG_SZ, (BYTE*) szValue, sizeof(TCHAR)*(lstrlen(szValue)+1))!=ERROR_SUCCESS) 
	{
		RegCloseKey(hKey);
		return E_UNEXPECTED;
	}
    RegCloseKey(hKey);

    // [HKEY_CLASSES_ROOT\NetClipFile]
    //    @="Remote Clipboard"
	if (RegCreateKeyEx(HKEY_CLASSES_ROOT, 
			_T("NetClipFile"), 
			NULL, _T(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, 
			&hKey, &dwDisposition)!=ERROR_SUCCESS) 
	{
		return E_UNEXPECTED;
	}
    lstrcpy(szValue, _T("Remote Clipboard"));
	if (RegSetValueEx(hKey, _T(""), NULL, REG_SZ, (BYTE*) szValue, sizeof(TCHAR)*(lstrlen(szValue)+1))!=ERROR_SUCCESS) 
	{
		RegCloseKey(hKey);
		return E_UNEXPECTED;
	}
    RegCloseKey(hKey);


    // [HKEY_CLASSES_ROOT\NetClipFile\CLSID]
    //    @="{B222AAAA-8813-11cf-A54D-080036F12502}"
	if (RegCreateKeyEx(HKEY_CLASSES_ROOT, 
			_T("NetClipFile\\CLSID"), 
			NULL, _T(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, 
			&hKey, &dwDisposition)!=ERROR_SUCCESS) 
	{
		return E_UNEXPECTED;
	}
    lstrcpy(szValue, _T("{B222AAAA-8813-11cf-A54D-080036F12502}"));
	if (RegSetValueEx(hKey, _T(""), NULL, REG_SZ, (BYTE*) szValue, sizeof(TCHAR)*(lstrlen(szValue)+1))!=ERROR_SUCCESS) 
	{
		RegCloseKey(hKey);
		return E_UNEXPECTED;
	}
    RegCloseKey(hKey);

#ifdef FEATURE_MENUHANDLER
    // [HKEY_CLASSES_ROOT\NetClipFile\shellex\ContextMenuHandlers]
    //    @="NetClipMenu"
	if (RegCreateKeyEx(HKEY_CLASSES_ROOT, 
			_T("NetClipFile\\shellex\\ContextMenuHandlers"), 
			NULL, _T(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, 
			&hKey, &dwDisposition)!=ERROR_SUCCESS) 
	{
		return E_UNEXPECTED;
	}
    lstrcpy(szValue, _T("NetClipMenu"));
	if (RegSetValueEx(hKey, _T(""), NULL, REG_SZ, (BYTE*) szValue, sizeof(TCHAR)*(lstrlen(szValue)+1))!=ERROR_SUCCESS) 
	{
		RegCloseKey(hKey);
		return E_UNEXPECTED;
	}
    RegCloseKey(hKey);

    // [HKEY_CLASSES_ROOT\NetClipFile\shellex\ContextMenuHandlers\NetClipMenu]
    //    @="{B222AAAA-8813-11cf-A54D-080036F12502}"
	if (RegCreateKeyEx(HKEY_CLASSES_ROOT, 
			_T("NetClipFile\\shellex\\ContextMenuHandlers\\NetClipMenu"), 
			NULL, _T(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, 
			&hKey, &dwDisposition)!=ERROR_SUCCESS) 
	{
		return E_UNEXPECTED;
	}
    lstrcpy(szValue, _T("{B222AAAA-8813-11cf-A54D-080036F12502}"));
	if (RegSetValueEx(hKey, _T(""), NULL, REG_SZ, (BYTE*) szValue, sizeof(TCHAR)*(lstrlen(szValue)+1))!=ERROR_SUCCESS) 
	{
		RegCloseKey(hKey);
		return E_UNEXPECTED;
	}
    RegCloseKey(hKey);
    
#endif

    // [HKEY_CLASSES_ROOT\NetClipFile\DefaultIcon]
    //    @="%1"
	if (RegCreateKeyEx(HKEY_CLASSES_ROOT, 
			_T("NetClipFile\\DefaultIcon"), 
			NULL, _T(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, 
			&hKey, &dwDisposition)!=ERROR_SUCCESS) 
	{
		return E_UNEXPECTED;
	}
    lstrcpy(szValue, _T("%1"));
	if (RegSetValueEx(hKey, _T(""), NULL, REG_SZ, (BYTE*) szValue, sizeof(TCHAR)*(lstrlen(szValue)+1))!=ERROR_SUCCESS) 
	{
		RegCloseKey(hKey);
		return E_UNEXPECTED;
	}
    RegCloseKey(hKey);

    // [HKEY_CLASSES_ROOT\NetClipFile\shellex\DropHandler]
    //    @="{B222AAAA-8813-11cf-A54D-080036F12502}"
	if (RegCreateKeyEx(HKEY_CLASSES_ROOT, 
			_T("NetClipFile\\shellex\\DropHandler"), 
			NULL, _T(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, 
			&hKey, &dwDisposition)!=ERROR_SUCCESS) 
	{
		return E_UNEXPECTED;
	}
    lstrcpy(szValue, _T("{B222AAAA-8813-11cf-A54D-080036F12502}"));
	if (RegSetValueEx(hKey, _T(""), NULL, REG_SZ, (BYTE*) szValue, sizeof(TCHAR)*(lstrlen(szValue)+1))!=ERROR_SUCCESS) 
	{
		RegCloseKey(hKey);
		return E_UNEXPECTED;
	}
    RegCloseKey(hKey);

    // [HKEY_CLASSES_ROOT\NetClipFile\shellex\IconHandler]
    //    @="%1"
	if (RegCreateKeyEx(HKEY_CLASSES_ROOT, 
			_T("NetClipFile\\shellex\\IconHandler"), 
			NULL, _T(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, 
			&hKey, &dwDisposition)!=ERROR_SUCCESS) 
	{
		return E_UNEXPECTED;
	}
    lstrcpy(szValue, _T("{B222AAAA-8813-11cf-A54D-080036F12502}"));
	if (RegSetValueEx(hKey, _T(""), NULL, REG_SZ, (BYTE*) szValue, sizeof(TCHAR)*(lstrlen(szValue)+1))!=ERROR_SUCCESS) 
	{
		RegCloseKey(hKey);
		return E_UNEXPECTED;
	}
    RegCloseKey(hKey);


    // [HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Shell Extensions\Approved]
    //    "{B222AAAA-8813-11cf-A54D-080036F12502}"="Remote Clipboard"
	if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, 
			_T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved"), 
			NULL, _T(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, 
			&hKey, &dwDisposition)!=ERROR_SUCCESS) 
	{
		return E_UNEXPECTED;
	}
    lstrcpy(szValue, _T("Remote Clipboard"));
	if (RegSetValueEx(hKey, _T("{B222AAAA-8813-11cf-A54D-080036F12502}"), NULL, REG_SZ, (BYTE*) szValue, sizeof(TCHAR)*(lstrlen(szValue)+1))!=ERROR_SUCCESS) 
	{
		RegCloseKey(hKey);
		return E_UNEXPECTED;
	}
    RegCloseKey(hKey);


    // [HKEY_CLASSES_ROOT\NetClipFile\shell\open\command]
    //    @="netclip.exe"
	if (RegCreateKeyEx(HKEY_CLASSES_ROOT, 
			_T("NetClipFile\\shell\\open\\command"), 
			NULL, _T(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, 
			&hKey, &dwDisposition)!=ERROR_SUCCESS) 
	{
		return E_UNEXPECTED;
	}
    lstrcpy(szValue, _T("netclip.exe"));
	if (RegSetValueEx(hKey, _T(""), NULL, REG_SZ, (BYTE*) szValue, sizeof(TCHAR)*(lstrlen(szValue)+1))!=ERROR_SUCCESS) 
	{
		RegCloseKey(hKey);
		return E_UNEXPECTED;
	}
    RegCloseKey(hKey);
#endif

	return NOERROR;
}

STDAPI DllUnregisterServer(void) 
{
	PSDllUnregisterServer();
#ifdef _FEATURE_SHELLEX
    if (RegDeleteKey(HKEY_CLASSES_ROOT, 
			_T("CLSID\\{B222AAAA-8813-11cf-A54D-080036F12502}\\InprocServer32"))!=ERROR_SUCCESS) 
	{
		return E_UNEXPECTED;
	}
    if (RegDeleteKey(HKEY_CLASSES_ROOT, 
			_T("CLSID\\{B222AAAA-8813-11cf-A54D-080036F12502}\\DefaultIcon"))!=ERROR_SUCCESS) 
	{
		return E_UNEXPECTED;
	}


    if (RegDeleteKey(HKEY_CLASSES_ROOT, 
			_T("CLSID\\{B222AAAA-8813-11cf-A54D-080036F12502}"))!=ERROR_SUCCESS) 
	{
		return E_UNEXPECTED;
	}

    // [HKEY_CLASSES_ROOT\.{B222AAAA-8813-11cf-A54D-080036F12502}\ShellNew]
	if (RegDeleteKey(HKEY_CLASSES_ROOT, 
			_T(".{B222AAAA-8813-11cf-A54D-080036F12502}\\ShellNew"))!=ERROR_SUCCESS) 
	{
		return E_UNEXPECTED;
	}

    // [HKEY_CLASSES_ROOT\.{B222AAAA-8813-11cf-A54D-080036F12502}]
    //    @="NetClipFile"
	if (RegDeleteKey(HKEY_CLASSES_ROOT, 
			_T(".{B222AAAA-8813-11cf-A54D-080036F12502}"))!=ERROR_SUCCESS) 
	{
		return E_UNEXPECTED;
	}

#ifdef FEATURE_MENUHANDLER
    // [HKEY_CLASSES_ROOT\NetClipFile\shellex\ContextMenuHandlers\NetClipMenu]
    //    @="{B222AAAA-8813-11cf-A54D-080036F12502}"
	if (RegDeleteKey(HKEY_CLASSES_ROOT, 
			_T("NetClipFile\\shellex\\ContextMenuHandlers\\NetClipMenu"))!=ERROR_SUCCESS) 
	{
		return E_UNEXPECTED;
	}

    // [HKEY_CLASSES_ROOT\NetClipFile\shellex\ContextMenuHandlers]
    //    @="NetClipMenu"
	if (RegDeleteKey(HKEY_CLASSES_ROOT, 
			_T("NetClipFile\\shellex\\ContextMenuHandlers"))!=ERROR_SUCCESS) 
	{
		return E_UNEXPECTED;
	}
    // [HKEY_CLASSES_ROOT\NetClipFile\shellex]
    //    @="NetClipMenu"
	if (RegDeleteKey(HKEY_CLASSES_ROOT, 
			_T("NetClipFile\\shellex"))!=ERROR_SUCCESS) 
	{
		return E_UNEXPECTED;
	}
#endif

    // [HKEY_CLASSES_ROOT\NetClipFile\CLSID]
    //    @="{B222AAAA-8813-11cf-A54D-080036F12502}"
	if (RegDeleteKey(HKEY_CLASSES_ROOT, 
			_T("NetClipFile\\CLSID"))!=ERROR_SUCCESS) 
	{
		return E_UNEXPECTED;
	}

    // [HKEY_CLASSES_ROOT\NetClipFile\shellex\DropHandler]
    //    @="{B222AAAA-8813-11cf-A54D-080036F12502}"
	if (RegDeleteKey(HKEY_CLASSES_ROOT, 
			_T("NetClipFile\\shellex\\DropHandler"))!=ERROR_SUCCESS) 
	{
		return E_UNEXPECTED;
	}

    // [HKEY_CLASSES_ROOT\NetClipFile\shellex\IconHandler]
    //    @="{B222AAAA-8813-11cf-A54D-080036F12502}"
	if (RegDeleteKey(HKEY_CLASSES_ROOT, 
			_T("NetClipFile\\shellex\\IconHandler"))!=ERROR_SUCCESS) 
	{
		return E_UNEXPECTED;
	}

    // [HKEY_CLASSES_ROOT\NetClipFile\shellex]
    //    @="{B222AAAA-8813-11cf-A54D-080036F12502}"
	if (RegDeleteKey(HKEY_CLASSES_ROOT, 
			_T("NetClipFile\\shellex"))!=ERROR_SUCCESS) 
	{
		return E_UNEXPECTED;
	}

    // [HKEY_CLASSES_ROOT\NetClipFile\DefaultIcon]
	if (RegDeleteKey(HKEY_CLASSES_ROOT, 
			_T("NetClipFile\\DefaultIcon"))!=ERROR_SUCCESS) 
	{
		return E_UNEXPECTED;
	}

    // [HKEY_CLASSES_ROOT\NetClipFile\shell\open\command]
	if (RegDeleteKey(HKEY_CLASSES_ROOT, 
			_T("NetClipFile\\shell\\open\\command"))!=ERROR_SUCCESS) 
	{
		return E_UNEXPECTED;
	}

    // [HKEY_CLASSES_ROOT\NetClipFile\shell\open
	if (RegDeleteKey(HKEY_CLASSES_ROOT, 
			_T("NetClipFile\\shell\\open"))!=ERROR_SUCCESS) 
	{
		return E_UNEXPECTED;
	}

    // [HKEY_CLASSES_ROOT\NetClipFile\shell]
	if (RegDeleteKey(HKEY_CLASSES_ROOT, 
			_T("NetClipFile\\shell"))!=ERROR_SUCCESS) 
	{
		return E_UNEXPECTED;
	}

    // [HKEY_CLASSES_ROOT\NetClipFile]
    //    @="Remote Clipboard Shell Extension file"
	if (RegDeleteKey(HKEY_CLASSES_ROOT, 
			_T("NetClipFile"))!=ERROR_SUCCESS) 
	{
		return E_UNEXPECTED;
	}

    // [HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Shell Extensions\Approved]
    //    "{B222AAAA-8813-11cf-A54D-080036F12502}"="Remote Clipboard Shell Extension"
    HKEY hKey;
	if (RegOpenKey(HKEY_LOCAL_MACHINE, 
			_T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved"), 
			&hKey)!=ERROR_SUCCESS) 
	{
		return E_UNEXPECTED;
	}

    if (RegDeleteValue(hKey, _T("{B222AAAA-8813-11cf-A54D-080036F12502}")) != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return E_UNEXPECTED;
    }
    RegCloseKey(hKey);
#endif

	return NOERROR;
}

