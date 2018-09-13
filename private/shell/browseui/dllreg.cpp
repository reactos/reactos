// dllreg.c -- autmatic registration and unregistration
//
#include "priv.h"
#include <advpub.h>
#include <comcat.h>
#include <winineti.h>
#include "resource.h"
#include "regkeys.h"

#ifdef UNIX
#include "unixstuff.h"
#endif

void AddNotepadToOpenWithList();

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
//#define RegCreate(hk, psz, phk) if (ERROR_SUCCESS != RegCreateKeyEx((hk), psz, 0, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_READ|KEY_WRITE, NULL, (phk), &dwDummy)) goto CleanUp
//#define RegSetStr(hk, psz) if (ERROR_SUCCESS != RegSetValueEx((hk), NULL, 0, REG_SZ, (BYTE*)(psz), lstrlen(psz)+1)) goto CleanUp
//#define RegSetStrValue(hk, pszStr, psz)    if(ERROR_SUCCESS != RegSetValueEx((hk), (const char *)(pszStr), 0, REG_SZ, (BYTE*)(psz), lstrlen(psz)+1)) goto CleanUp
//#define RegCloseK(hk) RegCloseKey(hk); hk = NULL
#define RegOpenK(hk, psz, phk) if (ERROR_SUCCESS != RegOpenKeyEx(hk, psz, 0, KEY_READ|KEY_WRITE, phk)) return FALSE


//=--------------------------------------------------------------------------=
// UnregisterTypeLibrary
//=--------------------------------------------------------------------------=
// blows away the type library keys for a given libid.
//
// Parameters:
//    REFCLSID        - [in] libid to blow away.
//
// Output:
//    BOOL            - TRUE OK, FALSE bad.
//
// Notes:
//    - WARNING: this function just blows away the entire type library section,
//      including all localized versions of the type library.  mildly anti-
//      social, but not killer.
//
#ifdef ATL_ENABLED
BOOL UnregisterTypeLibrary
(
    const CLSID* piidLibrary
)
{
    TCHAR szScratch[GUID_STR_LEN];
    HKEY hk;
    BOOL f = FALSE;

    // convert the libid into a string.
    //
    SHStringFromGUID(*piidLibrary, szScratch, ARRAYSIZE(szScratch));
    if (ERROR_SUCCESS == RegOpenKeyExA(HKEY_CLASSES_ROOT, "TypeLib", 0, KEY_READ|KEY_WRITE, &hk))
    {
        f = SHDeleteKey(hk, szScratch);
        ASSERT(f == ERROR_SUCCESS);
        RegCloseKey(hk);
    }

    return f;
}
#endif

#ifdef ATL_ENABLED
HRESULT SHRegisterTypeLib(void)
{
    HRESULT hr = S_OK;
    ITypeLib *pTypeLib;
    DWORD dwPathLen;
    WCHAR wzModuleName[MAX_PATH];

    // Load and register our type library.
    //
    dwPathLen = GetModuleFileName(MLGetHinst(), wzModuleName, ARRAYSIZE(wzModuleName));

#ifdef UNIX
    dwPathLen = ConvertModuleNameToUnix( wzModuleName );
#endif

    hr = LoadTypeLib(wzModuleName, &pTypeLib);

    if (SUCCEEDED(hr))
    {
        // call the unregister type library as we had some old junk that
        // was registered by a previous version of OleAut32, which is now causing
        // the current version to not work on NT...
        UnregisterTypeLibrary(&LIBID_BrowseUI);
        hr = RegisterTypeLib(pTypeLib, wzModuleName, NULL);

        if (FAILED(hr))
        {
            TraceMsg(DM_WARNING, "sccls: RegisterTypeLib failed (%x)", hr);
        }
        pTypeLib->Release();
    }
    else
    {
        TraceMsg(DM_WARNING, "sccls: LoadTypeLib failed (%x)", hr);
    }

    return hr;
}
#endif


void SetBrowseNewProcess(void)
// We want to enable browse new process by default on high capacity
// machines.  We do this in the per user section so that people can
// disable it if they want.
{
    static const TCHAR c_szBrowseNewProcessReg[] = REGSTR_PATH_EXPLORER TEXT("\\BrowseNewProcess");
    static const TCHAR c_szBrowseNewProcess[] = TEXT("BrowseNewProcess");
    
    // no way if less than ~30 meg (allow some room for debuggers, checked build etc)
    MEMORYSTATUS ms;
    SYSTEM_INFO  si;

    ms.dwLength=sizeof(MEMORYSTATUS);
    GlobalMemoryStatus(&ms);
    GetSystemInfo(&si);

    if (!g_fRunningOnNT && ((si.dwProcessorType == PROCESSOR_INTEL_486) ||
                            (si.dwProcessorType == PROCESSOR_INTEL_386)))
    {
        // Bail if Win9x and 386 or 486 cpu
        return;
    }
        

    if (ms.dwTotalPhys < 30*1024*1024)
        return;
    
    SHRegSetUSValue(c_szBrowseNewProcessReg, c_szBrowseNewProcess, REG_SZ, TEXT("yes"), SIZEOF(TEXT("yes")), SHREGSET_FORCE_HKCU);
}


/*----------------------------------------------------------
Purpose: Queries the registry for the location of the path
         of Internet Explorer and returns it in pszBuf.

Returns: TRUE on success
         FALSE if path cannot be determined

Cond:    --
*/

#define SIZE_FLAG   sizeof(" -nohome")

BOOL
GetIEPath(
    OUT LPSTR pszBuf,
    IN  DWORD cchBuf,
    IN  BOOL  bInsertQuotes)
{
    BOOL bRet = FALSE;
    HKEY hkey;

    *pszBuf = '\0';

    // Get the path of Internet Explorer 
    if (NO_ERROR != RegOpenKeyExA(HKEY_LOCAL_MACHINE, SZ_REGKEY_IEXPLOREA, 0, KEY_READ|KEY_WRITE, &hkey))  
    {
        TraceMsg(TF_ERROR, "InstallRegSet(): RegOpenKey( %s ) Failed", c_szIexploreKey) ;
    }
    else
    {
        DWORD cbBrowser;
        DWORD dwType;

        if (bInsertQuotes)
            lstrcatA(pszBuf, "\"");

        cbBrowser = CbFromCchA(cchBuf - SIZE_FLAG - 4);
        if (NO_ERROR != RegQueryValueExA(hkey, "", NULL, &dwType, 
                                         (LPBYTE)&pszBuf[bInsertQuotes?1:0], &cbBrowser))
        {
            TraceMsg(TF_ERROR, "InstallRegSet(): RegQueryValueEx() for Iexplore path failed");
        }
        else
        {
            bRet = TRUE;
        }

        if (bInsertQuotes)
            lstrcatA(pszBuf, "\"");

        RegCloseKey(hkey);
    }

    return bRet;
}


//
// The actual functions called
//


/*----------------------------------------------------------
Purpose: Calls the ADVPACK entry-point which executes an inf
         file section.
*/
HRESULT 
CallRegInstall(
    LPSTR pszSection,
    BOOL bUninstall)
{
    HRESULT hr = E_FAIL;
    HINSTANCE hinstAdvPack = LoadLibraryA("ADVPACK.DLL");

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
            if ( !EVAL(GetIEPath(szIEPath, SIZECHARS(szIEPath), TRUE)) )
            {
#ifndef UNIX
                // Failed, just say "iexplore"
                lstrcpyA(szIEPath, "iexplore.exe");
#else
                lstrcpyA(szIEPath, "iexplorer");
#endif
            }

            if (g_fRunningOnNT)
            {
                // If on NT, we want custom action for %25% %11%
                // so that it uses %SystemRoot% in writing the
                // path to the registry.
                stReg.cEntries += 2;
            }

            hr = pfnri(g_hinst, pszSection, &stReg);
            if (bUninstall)
            {
                // ADVPACK will return E_UNEXPECTED if you try to uninstall 
                // (which does a registry restore) on an INF section that was 
                // never installed.  We uninstall sections that may never have
                // been installed, so ignore this error
                hr = ((E_UNEXPECTED == hr) ? S_OK : hr);
            }
        }
        else
            TraceMsg(DM_ERROR, "DLLREG CallRegInstall() calling GetProcAddress(hinstAdvPack, \"RegInstall\") failed");

        FreeLibrary(hinstAdvPack);
    }
    else
        TraceMsg(DM_ERROR, "DLLREG CallRegInstall() Failed to load ADVPACK.DLL");

    return hr;
}


STDAPI 
DllRegisterServer(void)
{
    HRESULT hr = S_OK;
    HRESULT hrExternal = S_OK;  //used to return the first failure
    TraceMsg(DM_TRACE, "DLLREG DllRegisterServer() Beginning");

#ifdef DEBUG
    if (IsFlagSet(g_dwBreakFlags, BF_ONAPIENTER))
    {
        TraceMsg(TF_ALWAYS, "Stopping in DllRegisterServer");
        DEBUG_BREAK;
    }
#endif

    // Delete any old registration entries, then add the new ones.
    // Keep ADVPACK.DLL loaded across multiple calls to RegInstall.
    // (The inf engine doesn't guarantee DelReg/AddReg order, that's
    // why we explicitly unreg and reg here.)
    //
    HINSTANCE hinstAdvPack = LoadLibraryA("ADVPACK.DLL");
    hr = THR(CallRegInstall("InstallControls", FALSE));
    if (SUCCEEDED(hrExternal))
        hrExternal = hr;

    if (hinstAdvPack)
        FreeLibrary(hinstAdvPack);

#ifdef ATL_ENABLED
    // registers object, typelib and all interfaces in typelib
    hr = SHRegisterTypeLib();
    if (SUCCEEDED(hrExternal))
        hrExternal = hr;
#endif

    return hrExternal;
}

STDAPI DllUnregisterServer(void)
{
    HRESULT hr;
    TraceMsg(DM_TRACE, "DLLREG DllUnregisterServer() Beginning");

    // UnInstall the registry values
    hr = THR(CallRegInstall("UnInstallControls", TRUE));

    return hr;
}

void ImportQuickLinks();
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
    HRESULT hr = S_OK;
    HRESULT hrExternal = S_OK;

    CoInitialize(0);
    if (bInstall)
    {
        // "U" means it's the per user install call
        if (!lstrcmpiW(pszCmdLine, L"U"))
        {
            ImportQuickLinks();
            SetBrowseNewProcess();
            if (GetUIVersion() >= 5)
                hr = THR(CallRegInstall("InstallPerUser_BrowseUIShell", FALSE));
        }
        else
        {
            // Backup current associations because InstallPlatformRegItems() may overwrite.
            if (GetUIVersion() < 5)
                hr = THR(CallRegInstall("InstallBrowseUINonShell", FALSE));
            else
                hr = THR(CallRegInstall("InstallBrowseUIShell", FALSE));

            if (SUCCEEDED(hrExternal))
                hrExternal = hr;

#ifdef ATL_ENABLED
            SHRegisterTypeLib();
#endif
        }

        // Add Notepad to the OpenWithList for .htm files
        AddNotepadToOpenWithList();
    }
    else
    {
        hr = THR(CallRegInstall("UnInstallBrowseUI", TRUE));
        if (SUCCEEDED(hrExternal))
            hrExternal = hr;

#ifdef ATL_ENABLED
        UnregisterTypeLibrary(&LIBID_BrowseUI);
#endif
    }

    CoUninitialize();
    return hrExternal;    
}    


