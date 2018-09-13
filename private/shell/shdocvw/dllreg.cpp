// dllreg.c -- autmatic registration and unregistration
//
#include "priv.h"
#include "util.h"
#include "htregmng.h"
#include <advpub.h>
#include <comcat.h>
#include <winineti.h>
#include "resource.h"

#include <mluisupp.h>

#ifdef UNIX
#include "unixstuff.h"
#endif

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
BOOL UnregisterTypeLibrary
(
    const CLSID* piidLibrary
)
{
    TCHAR szScratch[GUID_STR_LEN];
    HKEY hk;
    BOOL f;

    // convert the libid into a string.
    //
    SHStringFromGUID(*piidLibrary, szScratch, ARRAYSIZE(szScratch));
    RegOpenK(HKEY_CLASSES_ROOT, TEXT("TypeLib"), &hk);

    f = SHDeleteKey(hk, szScratch);

    RegCloseKey(hk);
    return f;
}

HRESULT SHRegisterTypeLib(void)
{
    HRESULT hr = S_OK;
    ITypeLib *pTypeLib;
    DWORD   dwPathLen;
    TCHAR   szTmp[MAX_PATH];

    // Load and register our type library.
    //

    dwPathLen = GetModuleFileName(HINST_THISDLL, szTmp, ARRAYSIZE(szTmp));

#ifdef UNIX
    dwPathLen = ConvertModuleNameToUnix( szTmp );
#endif

    hr = LoadTypeLib(szTmp, &pTypeLib);

    if (SUCCEEDED(hr))
    {
        // call the unregister type library as we had some old junk that
        // was registered by a previous version of OleAut32, which is now causing
        // the current version to not work on NT...
        UnregisterTypeLibrary(&LIBID_SHDocVw);
        hr = RegisterTypeLib(pTypeLib, szTmp, NULL);

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
            if ( !EVAL(GetIEPath(szIEPath, SIZECHARS(szIEPath))) )
            {
                // Failed, just say "iexplore"
#ifndef UNIX
                StrCpyNA(szIEPath, "iexplore.exe", ARRAYSIZE(szIEPath));
#else
                StrCpyNA(szIEPath, "iexplorer", ARRAYSIZE(szIEPath));
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
            TraceMsg(TF_ERROR, "DLLREG CallRegInstall() calling GetProcAddress(hinstAdvPack, \"RegInstall\") failed");

        FreeLibrary(hinstAdvPack);
    }
    else
        TraceMsg(TF_ERROR, "DLLREG CallRegInstall() Failed to load ADVPACK.DLL");

    return hr;
}

const CATID * const c_DeskBandClasses[] = 
{
    &CLSID_QuickLinks,
    &CLSID_AddressBand,
    NULL
};

const CATID * const c_OldDeskBandClasses[] = 
{
    &CLSID_QuickLinksOld,
    NULL
};

const CATID * const c_InfoBandClasses[] =
{
    &CLSID_SearchBand,
    &CLSID_FavBand,
#ifdef ENABLE_CHANNELPANE
    &CLSID_ChannelBand,
#endif
    &CLSID_HistBand,
    &CLSID_ExplorerBand,
    NULL
};

#ifndef ENABLE_CHANNELPANE
const CATID * const c_OldInfoBandClasses[] =
{
    &CLSID_ChannelBand,
    NULL
};
#endif

// We don't care about any CLSIDs.
// We just want to register the category so it gets a nice name
const CATID * const c_CommBandClasses[] =
{
    NULL
};

#if 1 // { ALPHA BUGBUG ComCat bug work-around on alpha, nuke this eventually?
//***   HasImplCat -- does "HKCR/CLSID/{clsid}/Implemented Categories" exist
// NOTES
//  used for ComCat bug work-around on alpha
BOOL HasImplCat(const CATID *pclsid)
{
    HKEY hk;
    TCHAR szClass[GUIDSTR_MAX];
    TCHAR szImpl[MAX_PATH];      // "CLSID/{clsid}/Implemented Categories" BUGBUG size?

    // "CLSID/{clsid}/Implemented Categories"
    SHStringFromGUID(*pclsid, szClass, ARRAYSIZE(szClass));
    ASSERT(lstrlen(szClass) == GUIDSTR_MAX - 1);
    wnsprintf(szImpl, ARRAYSIZE(szImpl), TEXT("CLSID\\%s\\Implemented Categories"), szClass);

    if (RegOpenKeyEx(HKEY_CLASSES_ROOT, szImpl, 0, KEY_READ, &hk) == ERROR_SUCCESS) {
        RegCloseKey(hk);
        return TRUE;
    }
    else {
        TraceMsg(DM_WARNING, "HasImplCat: %s: ret 0", szImpl);
        return FALSE;
    }
}
#endif // }

enum { CCR_REG = 1, CCR_UNREG = 0, CCR_UNREGIMP = -1 };

//***   RegisterOneCategory -- [un]register ComCat implementor(s) and category
// ENTRY/EXIT
//  eRegister   CCR_REG, CCR_UNREG, CCR_UNREGIMP
//      CCR_REG, UNREG      reg/unreg implementor(s) and category
//      CCR_UNREGIMP        unreg implementor(s) only
//  pcatidCat   e.g. CATID_DeskBand
//  idResCat    e.g. IDS_CATDESKBAND
//  pcatidImpl  e.g. c_DeskBandClasses
void RegisterOneCategory(const CATID *pcatidCat, UINT idResCat, const CATID * const *pcatidImpl, UINT eRegister)
{
    ICatRegister* pcr;
    HRESULT hres = CoCreateInstance(CLSID_StdComponentCategoriesMgr, NULL,
        CLSCTX_INPROC_SERVER, IID_ICatRegister, (LPVOID*)&pcr);
    
    if (pcr) {
        if (eRegister == CCR_REG) {
            
            // register the category
            CATEGORYINFO catinfo;
            catinfo.catid = *pcatidCat;     // e.g. CATID_DESKBAND
            catinfo.lcid = LOCALE_USER_DEFAULT;
            MLLoadString(idResCat, catinfo.szDescription, ARRAYSIZE(catinfo.szDescription));
            hres = pcr->RegisterCategories(1, &catinfo);
            
            // register the classes that implement categories
            for ( ; *pcatidImpl != NULL; pcatidImpl++) {
                CLSID clsid = **pcatidImpl;
                CATID catid = *pcatidCat;
                hres = pcr->RegisterClassImplCategories(clsid, 1, &catid);
            }
            
        } else {
            
            // unregister the classes that implement categories
            for ( ; *pcatidImpl != NULL; pcatidImpl++) {
                CLSID clsid = **pcatidImpl;
                CATID catid = *pcatidCat;

#if 1 // { ALPHA BUGBUG ComCat bug work-around on alpha, nuke this eventually?
                // workaround comcat/alpha bug
                // n.b. we do this for non-alpha too to reduce testing impact
                // ie40:63004: comcat does RegCloseKey(invalid) on checked
                // nt/alpha if the clsid doesn't exist (e.g. for QuickLinksOld)
                if (!HasImplCat(&clsid))
                    continue;
#endif // }
                hres = pcr->UnRegisterClassImplCategories(clsid, 1, &catid);
            }
            
            if (eRegister == CCR_UNREG) {
                // BUGBUG do we want to do this?  other classes (e.g. 3rd party
                // ones) might still be using the category.  however since we're
                // the component that added (and supports) the category, it
                // seems correct that we should remove it when we unregister.

                // unregister the category
                CATID catid = *pcatidCat;
                hres = pcr->UnRegisterCategories(1, &catid);
            }
            
        }
        pcr->Release();
    }
}

void RegisterCategories(BOOL fRegister)
{
    UINT eRegister = fRegister ? CCR_REG : CCR_UNREG;

    RegisterOneCategory(&CATID_DeskBand, IDS_CATDESKBAND, c_DeskBandClasses, eRegister);
    RegisterOneCategory(&CATID_InfoBand, IDS_CATINFOBAND, c_InfoBandClasses, eRegister);
    if (fRegister) {
        // only nuke the implementor(s), not the category
        RegisterOneCategory(&CATID_DeskBand, IDS_CATDESKBAND, c_OldDeskBandClasses, CCR_UNREGIMP);
#ifndef ENABLE_CHANNELPANE
        RegisterOneCategory(&CATID_InfoBand, IDS_CATDESKBAND, c_OldInfoBandClasses, CCR_UNREGIMP);
#endif  // ENABLE_CHANNELPANE
    }
    RegisterOneCategory(&CATID_CommBand, IDS_CATCOMMBAND, c_CommBandClasses, eRegister);
}


HRESULT CreateShellFolderPath(LPCTSTR pszPath, LPCTSTR pszGUID, BOOL bUICLSID)
{
    if (!PathFileExists(pszPath))
        CreateDirectory(pszPath, NULL);

    // Mark the folder as a system directory
    if (SetFileAttributes(pszPath, FILE_ATTRIBUTE_SYSTEM))
    {
        TCHAR szDesktopIni[MAX_PATH];
        // Write in the desktop.ini the cache folder class ID
        PathCombine(szDesktopIni, pszPath, TEXT("desktop.ini"));

        // If the desktop.ini already exists, make sure it is writable
        if (PathFileExists(szDesktopIni))
            SetFileAttributes(szDesktopIni, FILE_ATTRIBUTE_NORMAL);

        // (First, flush the cache to make sure the desktop.ini
        // file is really created.)
        WritePrivateProfileString(NULL, NULL, NULL, szDesktopIni);
        WritePrivateProfileString(TEXT(".ShellClassInfo"), bUICLSID ? TEXT("UICLSID") : TEXT("CLSID"), pszGUID, szDesktopIni);
        WritePrivateProfileString(NULL, NULL, NULL, szDesktopIni);

        // Hide the desktop.ini since the shell does not selectively
        // hide it.
        SetFileAttributes(szDesktopIni, FILE_ATTRIBUTE_HIDDEN);

        return NOERROR;
    }
    else
    {
        TraceMsg(TF_ERROR, "Cannot make %s a system folder", pszPath);
        return E_FAIL;
    }
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

    // Delete any old registration entries, then add the new ones.
    // Keep ADVPACK.DLL loaded across multiple calls to RegInstall.
    // (The inf engine doesn't guarantee DelReg/AddReg order, that's
    // why we explicitly unreg and reg here.)
    //
    HINSTANCE hinstAdvPack = LoadLibrary(TEXT("ADVPACK.DLL"));
    hr = THR(CallRegInstall("InstallControls", FALSE));
    if (SUCCEEDED(hrExternal))
        hrExternal = hr;

    if (hinstAdvPack)
        FreeLibrary(hinstAdvPack);

    hr = THR(SHRegisterTypeLib());
    if (SUCCEEDED(hrExternal))
        hrExternal = hr;

#ifdef UNIX
    hrExternal = UnixRegisterBrowserInActiveSetup();
#endif /* UNIX */

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


extern HRESULT UpgradeSettings(void);

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
    HINSTANCE hinstAdvPack;

    ASSERT(IS_VALID_STRING_PTRW(pszCmdLine, -1));

    g_bInDllInstall = TRUE;
    if (0 == StrCmpIW(pszCmdLine, TEXTW("ForceAssoc")))
    {
        InstallIEAssociations(IEA_FORCEIE);
        g_bInDllInstall = FALSE;
        return hr;
    }

    hinstAdvPack = LoadLibrary(TEXT("ADVPACK.DLL"));    // Keep ADVPACK.DLL loaded across multiple calls to RegInstall.

#ifdef DEBUG
    if (IsFlagSet(g_dwBreakFlags, BF_ONAPIENTER))
    {
        TraceMsg(TF_ALWAYS, "Stopping in DllInstall");
        DEBUG_BREAK;
    }
#endif

    // Assume we're installing for integrated shell unless otherwise
    // noted.
    BOOL bIntegrated = ((WhichPlatform() == PLATFORM_INTEGRATED) ? TRUE : FALSE);

    TraceMsg(DM_TRACE, "DLLREG DllInstall(bInstall=%lx, pszCmdLine=\"%ls\") bIntegrated=%lx", (DWORD) bInstall, pszCmdLine, (DWORD) bIntegrated);

    CoInitialize(0);
    if (bInstall)
    {
        // Backup current associations because InstallPlatformRegItems() may overwrite.
        hr = THR(CallRegInstall("InstallAssociations", FALSE));
        if (SUCCEEDED(hrExternal))
            hrExternal = hr;

        hr = THR(CallRegInstall("InstallBrowser", FALSE));
        if (SUCCEEDED(hrExternal))
            hrExternal = hr;

        if (bIntegrated)
        {
            // UnInstall settings that cannot be installed with Shell Integration.
            // This will be a NO-OP if it wasn't installed.
            hr = THR(CallRegInstall("UnInstallOnlyBrowser", TRUE));
            if (SUCCEEDED(hrExternal))
                hrExternal = hr;

            // Install IE4 shell components too.
            hr = THR(CallRegInstall("InstallOnlyShell", FALSE));
            if (SUCCEEDED(hrExternal))
                hrExternal = hr;

            if (GetUIVersion() >= 5)
            {
                hr = THR(CallRegInstall("InstallWin2KShell", FALSE));
                if (SUCCEEDED(hrExternal))
                    hrExternal = hr;
            }
            else
            {
                hr = THR(CallRegInstall("InstallPreWin2KShell", FALSE));
                if (SUCCEEDED(hrExternal))
                    hrExternal = hr;
            }
        }
        else
        {
            // UnInstall Shell Integration settings.
            // This will be a NO-OP if it wasn't installed.
            hr = THR(CallRegInstall("UnInstallOnlyShell", TRUE));
            if (SUCCEEDED(hrExternal))
                hrExternal = hr;

            // Install IE4 shell components too.
            hr = THR(CallRegInstall("InstallOnlyBrowser", FALSE));
            if (SUCCEEDED(hrExternal))
                hrExternal = hr;
        }

        UpgradeSettings();
        UninstallCurrentPlatformRegItems();
        InstallIEAssociations(IEA_NORMAL);
        RegisterCategories(TRUE);
        SHRegisterTypeLib();
    }
    else
    {
        // Uninstall browser-only or integrated-browser?
        UninstallPlatformRegItems(bIntegrated);

        // Restore previous association settings that UninstallPlatformRegItems() could
        // have Uninstalled.
        hr = THR(CallRegInstall("UnInstallAssociations", TRUE));
        if (SUCCEEDED(hrExternal))
            hrExternal = hr;

        // UnInstall settings that cannot be installed with Shell Integration.
        // This will be a NO-OP if it wasn't installed.
        hr = THR(CallRegInstall("UnInstallOnlyBrowser", TRUE));
        if (SUCCEEDED(hrExternal))
            hrExternal = hr;

        // UnInstall Shell Integration settings.
        // This will be a NO-OP if it wasn't installed.
        hr = THR(CallRegInstall("UnInstallShell", TRUE));
        if (SUCCEEDED(hrExternal))
            hrExternal = hr;

        hr = THR(CallRegInstall("UnInstallBrowser", TRUE));
        if (SUCCEEDED(hrExternal))
            hrExternal = hr;

        UnregisterTypeLibrary(&LIBID_SHDocVw);
        RegisterCategories(FALSE);
    }


    if (hinstAdvPack)
        FreeLibrary(hinstAdvPack);

    g_bInDllInstall = FALSE;
    CoUninitialize();
    return hrExternal;    
}    


/*----------------------------------------------------------
Purpose: Gets a registry value that is User Specifc.  
         This will open HKEY_CURRENT_USER if it exists,
         otherwise it will open HKEY_LOCAL_MACHINE.  

Returns: DWORD containing success or error code.
Cond:    --
*/
LONG OpenRegUSKey(LPCTSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult)           
{
    DWORD dwRet = RegOpenKeyEx(HKEY_CURRENT_USER, lpSubKey, ulOptions, samDesired, phkResult);

    if (ERROR_SUCCESS != dwRet)
        dwRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, lpSubKey, ulOptions, samDesired, phkResult);

    return dwRet;
}
