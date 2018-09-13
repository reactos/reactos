#include "precomp.hxx"
#pragma hdrstop

#include <shguidp.h>
#include <advpub.h>     // RegInstall stuff

#include "util.h"
#include "resource.h"
#include "version.h"

// {ECF03A32-103D-11d2-854D-006008059367}   CLSID_MyDocsDropTarget
const CLSID CLSID_MyDocsDropTarget = { 0xecf03a32, 0x103d, 0x11d2, { 0x85, 0x4d, 0x0, 0x60, 0x8, 0x5, 0x93, 0x67 } };
// {ECF03A33-103D-11d2-854D-006008059367}   CLSID_MyDocsCopyHook
const CLSID CLSID_MyDocsCopyHook = { 0xecf03a33, 0x103d, 0x11d2, { 0x85, 0x4d, 0x0, 0x60, 0x8, 0x5, 0x93, 0x67 } };
// {4a7ded0a-ad25-11d0-98a8-0800361b1103}   CLSID_MyDocsProp
const CLSID CLSID_MyDocsProp = {0x4a7ded0a, 0xad25, 0x11d0, 0x98, 0xa8, 0x08, 0x00, 0x36, 0x1b, 0x11, 0x03};

HINSTANCE g_hInstance = 0;
LONG g_cRefThisDll = 0;          // DLL global reference count

STDAPI_(void) DllAddRef(void)
{
    InterlockedIncrement(&g_cRefThisDll);
}

STDAPI_(void) DllRelease(void)
{
    InterlockedDecrement(&g_cRefThisDll);
}

STDAPI DllCanUnloadNow(void)
{
    return g_cRefThisDll ? S_FALSE : S_OK;
}

HRESULT CMyDocuments_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi);
HRESULT CMyDocsCopyHook_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi);
HRESULT CMyDocsDropTarget_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi);
HRESULT CMyDocsProp_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi);


CF_TABLE_BEGIN( g_ObjectInfo )

    CF_TABLE_ENTRY( &CLSID_MyDocuments,         CMyDocuments_CreateInstance,      COCREATEONLY),
    CF_TABLE_ENTRY( &CLSID_MyDocsCopyHook,      CMyDocsCopyHook_CreateInstance,   COCREATEONLY),
    CF_TABLE_ENTRY( &CLSID_MyDocsDropTarget,    CMyDocsDropTarget_CreateInstance, COCREATEONLY),
    CF_TABLE_ENTRY( &CLSID_MyDocsProp,          CMyDocsProp_CreateInstance, COCREATEONLY),

CF_TABLE_END( g_ObjectInfo )


STDMETHODIMP CClassFactory::QueryInterface(REFIID riid,void**ppvObj)
{
    if (IsEqualIID(riid, IID_IClassFactory) || IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = (void *)GET_ICLASSFACTORY(this);
        DllAddRef();
        return NOERROR;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CClassFactory::AddRef()
{
    DllAddRef();
    return 2;
}

STDMETHODIMP_(ULONG) CClassFactory::Release()
{
    DllRelease();
    return 1;
}

STDMETHODIMP CClassFactory::CreateInstance(IUnknown *punkOuter, REFIID riid,void**ppv)
{
    *ppv = NULL;

    if (punkOuter && !IsEqualIID(riid, IID_IUnknown))
    {
        return CLASS_E_NOAGGREGATION;
    }
    else
    {
        LPOBJECTINFO pthisobj = (LPOBJECTINFO)this;
       
        if (punkOuter) // && !(pthisobj->dwClassFactFlags & OIF_ALLOWAGGREGATION))
            return CLASS_E_NOAGGREGATION;

        IUnknown *punk;
        HRESULT hr = pthisobj->pfnCreateInstance(punkOuter, &punk, pthisobj);
        if (SUCCEEDED(hr))
        {
            hr = punk->QueryInterface(riid, ppv);
            punk->Release();
        }
    
        return hr;
    }
}

STDMETHODIMP CClassFactory::LockServer(BOOL fLock)
{
    if (fLock)
        DllAddRef();
    else
        DllRelease();
    return S_OK;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid,void**ppv)
{
    if (IsEqualIID(riid, IID_IClassFactory) || IsEqualIID(riid, IID_IUnknown))
    {
        for (LPCOBJECTINFO pcls = g_ObjectInfo; pcls->pclsid; pcls++)
        {
            if (IsEqualGUID(rclsid, *(pcls->pclsid)))
            {
                *ppv = (void*)pcls; 
                DllAddRef();        // class factory holds DLL ref count
                return NOERROR;
            }
        }
    }
    *ppv = NULL;
    return CLASS_E_CLASSNOTAVAILABLE;
}


// Call ADVPACK for the given section of our resource based INF>
//   hInstance = resource instance to get REGINST section from
//   szSection = section name to invoke
HRESULT CallRegInstall(HINSTANCE hInstance, LPCSTR szSection)
{
    HRESULT hr = E_FAIL;
    HINSTANCE hinstAdvPack = LoadLibrary(TEXT("ADVPACK.DLL"));
    if (hinstAdvPack)
    {
        REGINSTALL pfnri = (REGINSTALL)GetProcAddress(hinstAdvPack, "RegInstall");
        if ( pfnri )
        {
#ifdef WINNT
            STRENTRY seReg[] =
            {
                // These two NT-specific entries must be at the end
                { "25", "%SystemRoot%" },
                { "11", "%SystemRoot%\\system32" },
            };
            STRTABLE stReg = { ARRAYSIZE(seReg), seReg };
            hr = pfnri(hInstance, szSection, &stReg);
#else
            hr = pfnri(hInstance, szSection, NULL);
#endif
        }
        FreeLibrary(hinstAdvPack);
    }
    return hr;
}

// Make sure the per-user settings are correct
void _CreateMyDocsFolder()
{
    TCHAR szPath[MAX_PATH];
    HRESULT hr = SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, szPath);
    if (hr != S_OK)
    {
        hr = SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_DEFAULT, szPath);
        if (hr == S_OK)
        {
            hr = ChangePersonalPath(szPath, NULL, NULL);
            if (SUCCEEDED(hr))
            {
                // At this point we're creating the directory.
                SHGetFolderPath(NULL, CSIDL_PERSONAL | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_DEFAULT, szPath);
            }
        }
    }
}

// export that ie4unit.exe calls at per user install time
// this lets us execute code instead of depending on the "DefaultUser" template
// that is used to init new accounts. this deals with upgrade cases too, very important

STDAPI_(void) PerUserInit(void)
{
    TCHAR szPath[MAX_PATH];

    // Check special case that someone (notable Dell when setting up OSR2
    // w/Office 97) might have made the Personal shell folder a directory under
    // the windows\desktop directory.  In this case, we want to hide 
    // the desktop reg item icon

    HRESULT hr = SHGetFolderPath(NULL, CSIDL_PERSONAL | CSIDL_FLAG_DONT_VERIFY, NULL, SHGFP_TYPE_CURRENT, szPath);
    if ((hr == S_OK) && IsPathGoodMyDocsPath(NULL, szPath) == PATH_IS_DESKTOP)
    {
        HideMyDocs();   // Hiding My Docs regitem because real folder is on desktop...
    }

    if (!IsMyDocsHidden())
    {
        _CreateMyDocsFolder();
    }

    UpdateSendToFile(FALSE);
    InstallMyPictures();
}

STDAPI DllRegisterServer(void)
{
    TCHAR szPath[MAX_PATH];

    // Do our dll reg from .inf first...
    CallRegInstall(g_hInstance, "RegDll");

    // put localizable things on the HKCR\CLSID\{MyDocs} key...
    if (S_OK == SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_DEFAULT, szPath))
    {
        LPTSTR pszDisplayName = PathFindFileName(szPath);   // localized name setup
        UINT cch = lstrlen(pszDisplayName);
        HKEY hkey;
        if (ERROR_SUCCESS == RegOpenKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\") MYDOCS_CLSID, &hkey))
        {
            // display name
            RegSetValueEx(hkey, NULL, 0, REG_SZ, (LPBYTE)pszDisplayName, (cch + 1) * sizeof(TCHAR));

            // add our info tip
            cch = LoadString(g_hInstance, IDS_INFOTIP_VALUE, szPath, ARRAYSIZE(szPath));
            RegSetValueEx(hkey, TEXT("InfoTip"), 0, REG_SZ, (LPBYTE)szPath, (cch + 1) * sizeof(TCHAR));
            RegCloseKey(hkey);
        }
    }

    PerUserInit();
    return S_OK;
}

STDAPI DllUnregisterServer(void)
{
    CallRegInstall(g_hInstance, "UnregDll");
    return S_OK;
}

STDAPI DllInstall( BOOL bInstall, LPCWSTR pszCmdLine )
{
    if (0 == StrCmpIW(pszCmdLine, L"UseReadOnly"))
    {
        // Add key for system to use read only bit on shell folders...
        HKEY hkey;
        if (ERROR_SUCCESS == RegCreateKey( HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer"), &hkey ))
        {
            if (bInstall)
            {
                DWORD dwValue = 1;
                RegSetValueEx( hkey, TEXT("UseReadOnlyForSystemFolders"), 0, REG_DWORD, (LPBYTE)&dwValue, sizeof(dwValue));
            }
            else
            {
                RegDeleteValue( hkey, TEXT("UseReadOnlyForSystemFolders") );
            }
            RegCloseKey(hkey);
        }
    }
    else if (0 == StrCmpIW(pszCmdLine, L"Restore"))
    {
        RestoreMyDocsFolder();
    }

    return S_OK;
}

STDAPI_(BOOL) DllMain( HINSTANCE hInstance, DWORD dwReason,void*pReserved )
{
    switch ( dwReason )
    {
        case DLL_PROCESS_ATTACH:
        {
            g_hInstance = hInstance;
            DisableThreadLibraryCalls(hInstance);
            break;
        }
    }
    return TRUE;
}

