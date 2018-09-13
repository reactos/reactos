// dllreg.cpp -- autmatic registration and unregistration
//
#include "priv.h"

#include <advpub.h>
#include <comcat.h>


// helper macros

// ADVPACK will return E_UNEXPECTED if you try to uninstall (which does a registry restore)
// on an INF section that was never installed.  We uninstall sections that may never have
// been installed, so this MACRO will quiet these errors.
#define QuietInstallNoOp(hr)   ((E_UNEXPECTED == hr) ? S_OK : hr)

BOOL UnregisterTypeLibrary(const CLSID* piidLibrary)
{
    TCHAR szScratch[GUIDSTR_MAX];
    HKEY hk;
    BOOL fResult = FALSE;

    // convert the libid into a string.
    //
    SHStringFromGUID(*piidLibrary, szScratch, ARRAYSIZE(szScratch));

    if (RegOpenKey(HKEY_CLASSES_ROOT, TEXT("TypeLib"), &hk) == ERROR_SUCCESS) {
        fResult = RegDeleteKey(hk, szScratch);
        RegCloseKey(hk);
    }
    
    return fResult;
}



HRESULT RegisterTypeLib(void)
{
    HRESULT hr = S_OK;
    ITypeLib *pTypeLib;
    DWORD   dwPathLen;
    TCHAR   szTmp[MAX_PATH];
#ifdef UNICODE
    WCHAR   *pwsz = szTmp; 
#else
    WCHAR   pwsz[MAX_PATH];
#endif

    // Load and register our type library.
    //
    dwPathLen = GetModuleFileName(HINST_THISDLL, szTmp, ARRAYSIZE(szTmp));
#ifndef UNICODE
    if (SHAnsiToUnicode(szTmp, pwsz, MAX_PATH)) 
#endif
    {
        hr = LoadTypeLib(pwsz, &pTypeLib);

        if (SUCCEEDED(hr))
        {
            // call the unregister type library as we had some old junk that
            // was registered by a previous version of OleAut32, which is now causing
            // the current version to not work on NT...
            UnregisterTypeLibrary(&LIBID_SrvWizLib);
            hr = RegisterTypeLib(pTypeLib, pwsz, NULL);

            if (FAILED(hr))
            {
            }
            pTypeLib->Release();
        }
        else
        {
        }
    } 
#ifndef UNICODE
    else {
        hr = E_FAIL;
    }
#endif

    return hr;
}


/*----------------------------------------------------------
Purpose: Calls the ADVPACK entry-point which executes an inf
         file section.

Returns: 
Cond:    --
*/
HRESULT CallRegInstall(HINSTANCE hinstSrvWiz, LPSTR szSection)
{
    HRESULT hr = E_FAIL;
    HINSTANCE hinstAdvPack = LoadLibrary(TEXT("ADVPACK.DLL"));

    if (hinstAdvPack)
    {
        REGINSTALL pfnri = (REGINSTALL)GetProcAddress(hinstAdvPack, "RegInstall");

        if (pfnri)
        {
            char szThisDLL[MAX_PATH];

            // Get the location of this DLL from the HINSTANCE
            if ( !GetModuleFileNameA(hinstSrvWiz, szThisDLL, ARRAYSIZE(szThisDLL)) )
            {
                // Failed, just say "srvwiz.dll"
                lstrcpyA(szThisDLL, "srvwiz.dll");
            }

            STRENTRY seReg[] = {
                { "THISDLL", szThisDLL },

                // These two NT-specific entries must be at the end
                { "25", "%SystemRoot%" },
                { "11", "%SystemRoot%\\system32" },
            };
            STRTABLE stReg = { ARRAYSIZE(seReg), seReg };

            hr = pfnri(g_hinst, szSection, &stReg);
        }

        FreeLibrary(hinstAdvPack);
    }

    return hr;
}


STDAPI DllRegisterServer(void)
{
    HRESULT hr;

    // Delete any old registration entries, then add the new ones.
    // Keep ADVPACK.DLL loaded across multiple calls to RegInstall.
    // (The inf engine doesn't guarantee DelReg/AddReg order, that's
    // why we explicitly unreg and reg here.)
    //
    HINSTANCE hinstSrvWiz = GetModuleHandle(TEXT("SRVWIZ.DLL"));
    hr = CallRegInstall(hinstSrvWiz, "RegDll");

    RegisterTypeLib();

    return hr;
}

STDAPI DllUnregisterServer(void)
{
    HRESULT hr;
    HINSTANCE hinstSrvWiz = GetModuleHandle(TEXT("SRVWIZ.DLL"));

    // UnInstall the registry values
    hr = CallRegInstall(hinstSrvWiz, "UnregDll");
    UnregisterTypeLibrary(&LIBID_SrvWizLib);

    return hr;
}


STDAPI DllInstall(BOOL bInstall, LPCWSTR pszCmdLine)
{
    return S_OK;    
}    

