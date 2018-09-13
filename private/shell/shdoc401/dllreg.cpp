// dllreg.c -- autmatic registration and unregistration
//
#include "priv.h"
#include <advpub.h>

STDAPI CDeskHtmlProp_RegUnReg(BOOL bReg);

//=--------------------------------------------------------------------------=
// miscellaneous [useful] numerical constants
//=--------------------------------------------------------------------------=
// the length of a guid once printed out with -'s, leading and trailing bracket,
// plus 1 for NULL
//
#define GUID_STR_LEN    40


//
// helper macros
//
#define RegOpenK(hk, psz, phk) if (ERROR_SUCCESS != RegOpenKeyEx(hk, psz, 0, KEY_READ|KEY_WRITE, phk)) return FALSE

// ADVPACK will return E_UNEXPECTED if you try to uninstall (which does a registry restore)
// on an INF section that was never installed.  We uninstall sections that may never have
// been installed, so this MACRO will quiet these errors.
#define QuietInstallNoOp(hr)   ((E_UNEXPECTED == hr) ? S_OK : hr)

//
// The actual functions called
//


/*----------------------------------------------------------
Purpose: Calls the ADVPACK entry-point which executes an inf
         file section.

Returns: 
Cond:    --
*/
HRESULT 
CallRegInstall(
    LPSTR szSection)
{
    HRESULT hr = E_FAIL;
    HINSTANCE hinstAdvPack = LoadLibrary(TEXT("ADVPACK.DLL"));

    if (hinstAdvPack)
    {
        REGINSTALL pfnri = (REGINSTALL)GetProcAddress(hinstAdvPack, "RegInstall");

        if (pfnri)
        {
            static STRENTRY seReg[] = {
                // These two NT-specific entries must be at the end
                { "25", "%SystemRoot%" },
                { "11", "%SystemRoot%\\system32" },
            };
            STRTABLE stReg = { ARRAYSIZE(seReg) - 2, seReg };

            if (g_fRunningOnNT)
            {
                // If on NT, we want custom action for %25% %11%
                // so that it uses %SystemRoot% in writing the
                // path to the registry.
                stReg.cEntries += 2;
            }

            hr = pfnri(g_hinst, szSection, &stReg);
            if (FAILED(QuietInstallNoOp(hr)))
                TraceMsg(DM_ERROR, "DLLREG CallRegInstall(\"%s\") failed.  hr=%lx", szSection, (DWORD) hr);
        }
        else
            TraceMsg(DM_ERROR, "DLLREG CallRegInstall() calling GetProcAddress(hinstAdvPack, \"RegInstall\") failed");

        FreeLibrary(hinstAdvPack);
    }
    else
        TraceMsg(DM_ERROR, "DLLREG CallRegInstall() Failed to load ADVPACK.DLL");

    return hr;
}

BOOL UnregisterTypeLibrary(const CLSID* piidLibrary)
{
    TCHAR szScratch[GUIDSTR_MAX];
    HKEY hk;
    BOOL f = FALSE;

    // convert the libid into a string.
    //
    SHStringFromGUID(*piidLibrary, szScratch, ARRAYSIZE(szScratch));

    if (RegOpenKey(HKEY_CLASSES_ROOT, TEXT("TypeLib"), &hk) == ERROR_SUCCESS)
    {
        f = RegDeleteKey(hk, szScratch);
        RegCloseKey(hk);
    }
    
    return f;
}

HRESULT Shell32RegTypeLib(void)
{
    TCHAR szPath[MAX_PATH];
    WCHAR wszPath[MAX_PATH];

    // Load and register our type library.
    //
    GetModuleFileName(HINST_THISDLL, szPath, ARRAYSIZE(szPath));
    SHTCharToUnicode(szPath, wszPath, ARRAYSIZE(wszPath));

    ITypeLib *pTypeLib;
    HRESULT hr = LoadTypeLib(wszPath, &pTypeLib);
    if (SUCCEEDED(hr))
    {
        // call the unregister type library as we had some old junk that
        // was registered by a previous version of OleAut32, which is now causing
        // the current version to not work on NT...
        UnregisterTypeLibrary(&LIBID_Shell32);
        hr = RegisterTypeLib(pTypeLib, wszPath, NULL);
        if (FAILED(hr))
        {
            TraceMsg(TF_WARNING, "SHELL32: RegisterTypeLib failed (%x)", hr);
        }
        pTypeLib->Release();
    }
    else
    {
        TraceMsg(TF_WARNING, "SHELL32: LoadTypeLib failed (%x)", hr);
    }

    return hr;
}


STDAPI
DllRegisterServer(void)
{
    HRESULT hr = S_OK;
    HRESULT hrExternal = S_OK;
    TraceMsg(DM_TRACE, "DLLREG DllRegisterServer() Beginning");

#ifdef DEBUG
    if (IsFlagSet(g_dwBreakFlags, BF_ONAPIENTER))
    {
        TraceMsg(TF_ALWAYS, "Stopping in DllRegisterServer");
        DEBUG_BREAK;
    }
#endif

    //
    //  SHDOC401 is used only in the "the shell is in IE 4.01 integrated mode"
    //  case.  We detect this by sniffing the shell32 version.
    //
    //  less than v4 - not integrated; do nothing
    //            v4 - integrated IE4.x; install
    //            v5 - integrated IE5.x or higher; do nothing
    //
    //  We don't use WhichPlatform() because WhichPlatform() does not
    //  to distinguish between IE4 shell32 and IE5 shell32 (it calls them
    //  both "integrated").

    if (g_uiShell32 != 4) {
        TraceMsg(TF_ALWAYS, "Skipping DllRegisterServer because shell32 is not 4.x");
        return S_OK;
    }

    hr = CallRegInstall("RegDll");
    if (SUCCEEDED(hrExternal))
        hrExternal = hr;
    if (FAILED(hr))
        TraceMsg(DM_ERROR, "DLLREG DllRegisterServer() failed calling CallRegInstall(\"RegDll\").  hr=%lx", (DWORD) hr);

    hr = Shell32RegTypeLib();
    if (SUCCEEDED(hrExternal))
        hrExternal = hr;
    if (FAILED(hr))
        TraceMsg(DM_ERROR, "DLLREG DllRegisterServer() failed calling CallRegInstall(\"RegDll\").  hr=%lx", (DWORD) hr);


    if (g_fRunningOnNT)
    {
        hr = CallRegInstall("RegDllNT");
        if (SUCCEEDED(hrExternal))
            hrExternal = hr;
    }
    
    return hrExternal;
}

STDAPI DllUnregisterServer(void)
{
    HRESULT hr;
    TraceMsg(DM_TRACE, "DLLREG DllUnregisterServer() Beginning");

    // UnInstall the registry values
    hr = CallRegInstall("UnregDll");

    // ADVPACK will return E_UNEXPECTED if you try to uninstall
    // (which does a registry restore) on an INF section that was
    // never installed.  We uninstall sections that may never have
    // been installed, so ignore this error 
    hr = (E_UNEXPECTED == hr) ? S_OK : hr;

    ASSERT(SUCCEEDED(hr));  // Make sure uninstall succeeded.

    UnregisterTypeLibrary(&LIBID_Shell32);

    return hr;
}

/*----------------------------------------------------------
Purpose: Install/uninstall user settings

Description: Nothing to do here.
*/
STDAPI DllInstall(BOOL bInstall, LPCWSTR pszCmdLine)
{
    HRESULT hr = S_OK;
    HRESULT hrExternal = S_OK;

    if (bInstall)
    {
        if (g_uiShell32 != 4) {
            TraceMsg(TF_ALWAYS, "Skipping DllInstall because shell32 is not 4.x");
            return S_OK;
        }

        // The "RegDllAlways" section will always be run independent of what
        // platform we are on.
        hr = CallRegInstall("RegDllAlways");
        if (SUCCEEDED(hrExternal))
            hrExternal = hr;
        if (FAILED(hr))
            TraceMsg(DM_ERROR, "DLLREG DllRegisterServer() failed calling CallRegInstall(\"RegDllAlways\").  hr=%lx", (DWORD) hr);
    }

    // We need to do this because this is the code that populates the branch of registry
    // needed for the ActiveDesktop.
    CDeskHtmlProp_RegUnReg(bInstall);  

    return hrExternal;
}
