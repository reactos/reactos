// dllreg.cpp -- autmatic registration and unregistration
//
#include "priv.h"
//#include "installwv.h"

#include <advpub.h>
#include <comcat.h>
//#include <msieftp.h>


// helper macros

// ADVPACK will return E_UNEXPECTED if you try to uninstall (which does a registry restore)
// on an INF section that was never installed.  We uninstall sections that may never have
// been installed, so this MACRO will quiet these errors.
#define QuietInstallNoOp(hr)   ((E_UNEXPECTED == hr) ? S_OK : hr)


const CHAR  c_szIexploreKey[]         = "Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\IEXPLORE.EXE";

/*----------------------------------------------------------
Purpose: Queries the registry for the location of the path
         of Internet Explorer and returns it in pszBuf.

Returns: TRUE on success
         FALSE if path cannot be determined

Cond:    --
*/
BOOL
GetIEPath(
    OUT LPSTR pszBuf,
    IN  DWORD cchBuf)
{
    BOOL bRet = FALSE;
    HKEY hkey;

    *pszBuf = '\0';

    // Get the path of Internet Explorer 
    if (NO_ERROR != RegOpenKeyA(HKEY_LOCAL_MACHINE, c_szIexploreKey, &hkey))  
    {
    }
    else
    {
        DWORD cbBrowser;
        DWORD dwType;

        lstrcatA(pszBuf, "\"");

        cbBrowser = CbFromCchA(cchBuf - lstrlenA(" -nohome") - 4);
        if (NO_ERROR != RegQueryValueExA(hkey, "", NULL, &dwType, 
                                         (LPBYTE)&pszBuf[1], &cbBrowser))
        {
        }
        else
        {
            bRet = TRUE;
        }

        lstrcatA(pszBuf, "\"");

        RegCloseKey(hkey);
    }

    return bRet;
}


/*----------------------------------------------------------
Purpose: Calls the ADVPACK entry-point which executes an inf
         file section.

Returns: 
Cond:    --
*/
HRESULT CallRegInstall(LPSTR szSection)
{
    HRESULT hr = E_FAIL;
    HINSTANCE hinstAdvPack = LoadLibrary(TEXT("ADVPACK.DLL"));

    if (hinstAdvPack)
    {
        REGINSTALL pfnri = (REGINSTALL)GetProcAddress(hinstAdvPack, "RegInstall");

        if (pfnri)
        {
            char szIEPath[MAX_PATH];
            STRENTRY seReg[] = {
                { "MSIEXPLORE", szIEPath },

                // These two NT-specific entries must be at the end
                { "25", "%SystemRoot%" },
                { "11", "%SystemRoot%\\system32" },
            };
            STRTABLE stReg = { ARRAYSIZE(seReg) - 2, seReg };

            // Get the location of iexplore from the registry
            if ( !EVAL(GetIEPath(szIEPath, ARRAYSIZE(szIEPath))) )
            {
                // Failed, just say "iexplore"
                lstrcpyA(szIEPath, "iexplore.exe");
            }

#if 0 //  Disable ---------------------
            if (g_fRunningOnNT)
            {
                // If on NT, we want custom action for %25% %11%
                // so that it uses %SystemRoot% in writing the
                // path to the registry.
                stReg.cEntries += 2;
            }
#endif // 0 

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
    HINSTANCE hinstFTP = LoadLibrary(TEXT("MSIEFTP.DLL"));
    HINSTANCE hinstAdvPack = LoadLibrary(TEXT("ADVPACK.DLL"));
    hr = CallRegInstall("FtpShellExtensionInstall");

    if (hinstAdvPack)
        FreeLibrary(hinstAdvPack);

    if (EVAL(hinstFTP))     // We need hinstFTP or we can't install the webview files.
        FreeLibrary(hinstFTP);

    return hr;
}

STDAPI DllUnregisterServer(void)
{
    HRESULT hr;

    // UnInstall the registry values
    hr = CallRegInstall("FtpShellExtensionUninstall");

    return hr;
}


/*----------------------------------------------------------
Purpose: Install/uninstall user settings

Description: Note that this function has special error handling.
             The function will keep hrExternal with the worse error
             but will only stop executing util the internal error (hr)
             gets really bad.  This is because we need the external
             error to catch incorrectly authored INFs but the internal
             error to be robust in attempting to install other INF sections
             even if one doesn't make it.
*/
STDAPI DllInstall(BOOL bInstall, LPCWSTR pszCmdLine)
{
    return S_OK;    
}    

