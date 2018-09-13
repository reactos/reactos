#include "init.h"
#include "global.h"
#include <shlwapi.h> // for DllInstall prototype

#define MLUI_INIT
#include <mluisupp.h>

extern HRESULT CanonicalizeModuleUsage(void);

// {88C6C381-2E85-11d0-94DE-444553540000}
const GUID CLSID_ControlFolder = {0x88c6c381, 0x2e85, 0x11d0, 0x94, 0xde, 0x44, 0x45, 0x53, 0x54, 0x0, 0x0};
#define STRING_CLSID_CONTROLFOLDER TEXT("{88C6C381-2E85-11d0-94DE-444553540000}")

// global variables
HINSTANCE   g_hInst = NULL;
LONG        g_cRefDll = 0;
BOOL        g_fAllAccess = FALSE;   // we'll set to true if we can open our keys with KEY_ALL_ACCESS

#define GUID_STR_LEN    40
#define REG_PATH_IE_SETTINGS  TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings")
#define REG_PATH_IE_CACHE_LIST  TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\ActiveX Cache")
#define REG_ACTIVEX_CACHE     TEXT("ActiveXCache")
#define DEFAULT_CACHE_DIRECTORY  TEXT("Occache")

HRESULT CreateShellFolderPath(LPCTSTR pszPath, LPCTSTR pszGUID)
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
        WritePrivateProfileString(TEXT(".ShellClassInfo"), TEXT("CLSID"), pszGUID, szDesktopIni);
        WritePrivateProfileString(NULL, NULL, NULL, szDesktopIni);

        // Hide the desktop.ini since the shell does not selectively
        // hide it.
        SetFileAttributes(szDesktopIni, FILE_ATTRIBUTE_HIDDEN);

        return NOERROR;
    }
    else
    {
        DebugMsg(DM_TRACE, "Cannot make %s a system folder", pszPath);
        return E_FAIL;
    }
}

void CleanupShellFolder(LPCTSTR pszPath)
{
    if (PathFileExists(pszPath))
    {
        TCHAR szDesktopIni[MAX_PATH];

        // make the history a normal folder
        SetFileAttributes(pszPath, FILE_ATTRIBUTE_NORMAL);
        PathCombine(szDesktopIni, pszPath, TEXT("desktop.ini"));

        // If the desktop.ini already exists, make sure it is writable
        if (PathFileExists(szDesktopIni))
        {
            SetFileAttributes(szDesktopIni, FILE_ATTRIBUTE_NORMAL);
            // Get the ini file cache to let go of this file
            WritePrivateProfileString(NULL, NULL, NULL, szDesktopIni);
            DeleteFile(szDesktopIni);
        }

        // remove the history directory
        // RemoveDirectory(pszPath); // don't do this, we haven't uninstalled all the controls therein! 
    }
}

HRESULT CallRegInstall(LPSTR szSection)
{
    HRESULT hr = E_FAIL;
    HINSTANCE hinstAdvPack = LoadLibrary(TEXT("ADVPACK.DLL"));
    if (hinstAdvPack)
    {
        REGINSTALL pfnri = (REGINSTALL)GetProcAddress(hinstAdvPack, achREGINSTALL);
        if (pfnri)
        {
            hr = pfnri(g_hInst, szSection, NULL);
        }

        FreeLibrary(hinstAdvPack);
    }

    return hr;
}


HRESULT GetControlFolderPath(LPTSTR szCacheDir, DWORD cchBuffer )
{
    /*
    LONG lResult = ERROR_SUCCESS;
    HKEY hKeyIntSetting = NULL;

    Assert(lpszDir != NULL);
    if (lpszDir == NULL)
        return HRESULT_FROM_WIN32(ERROR_BAD_ARGUMENTS);

    if ((lResult = RegOpenKeyEx(
                        HKEY_LOCAL_MACHINE,
                        REG_PATH_IE_SETTINGS,
                        0x0,
                        KEY_READ,
                        &hKeyIntSetting)) == ERROR_SUCCESS)
    {
        ULONG ulSize = ulSizeBuf;
        lResult = RegQueryValueEx(
                            hKeyIntSetting,
                            REG_ACTIVEX_CACHE,
                            NULL,
                            NULL,
                            (unsigned char*)lpszDir,
                            &ulSize);
        RegCloseKey(hKeyIntSetting);
    }

    return (lResult == ERROR_SUCCESS ? S_OK : HRESULT_FROM_WIN32(lResult));
    */
       // Compose the default path.
    int len;

    GetWindowsDirectory(szCacheDir, cchBuffer);
    len = lstrlen(szCacheDir);
    if ( len && (szCacheDir[len-1] != '\\'))
        lstrcat(szCacheDir, "\\");
    lstrcat(szCacheDir, REG_OCX_CACHE_DIR);

    return ((len != 0)? S_OK : E_FAIL);
}


STDAPI AddCacheToRegPathList( HKEY hkeyParent, LPCTSTR szCacheDir, DWORD cchCacheDir )
{
    HRESULT hr = E_FAIL;
    LONG    lResult;

    // Check to see if new path already exists in the list of paths under
    // HKLM\...\Windows\CurrentVersion\Internet Settings\ActiveX Cache\Paths.
    // If not, add it.
    HKEY  hkeyCacheList = NULL;

    lResult = RegCreateKey( hkeyParent, REG_OCX_CACHE_SUBKEY, &hkeyCacheList );
    if (lResult == ERROR_SUCCESS) {
        DWORD dwIndex;
        TCHAR szName[MAX_PATH];
        DWORD cbName;
        TCHAR szValue[MAX_PATH];
        DWORD cbValue;
        LONG  lValueIndex = -1;
        BOOL  fFoundValue = FALSE;

        // iterate through the values of the cache subkey of the internet settings key.
        // The values have names which are simple, positive itegers. The idea here is
        // to have collection of values like so:
        // Name         Value                                   Source
        // "1"          "C:\WINNT\OC Cache"                     IE3 legacy controls
        // "2"          "C:\WINNT\Downloaded ActiveXControls"   IE4 PR-1 legacy controls
        // "3"          "C:\WINNT\Downloaded Components"        IE4 controls.
        for ( dwIndex = 0, cbName = sizeof(szName), cbValue = sizeof(szValue); 
              lResult == ERROR_SUCCESS; 
              dwIndex++, cbName = sizeof(szName), cbValue = sizeof(szValue) )
        {
            lResult = RegEnumValue( hkeyCacheList, dwIndex,
                                    szName, &cbName, 
                                    NULL, NULL,
                                    (LPBYTE)szValue, &cbValue );

            if (lResult == ERROR_SUCCESS)
            {
                // for find new unique value name later.
                lValueIndex = max(lValueIndex, StrToInt(szName));

                if ( !fFoundValue )
                    fFoundValue = (lstrcmpi(szCacheDir, szValue) == 0);
                
                // Make sure that we're registered for all the (existing) old cache directories
                if ( !fFoundValue && PathFileExists(szValue) ) {
                    CreateShellFolderPath( szValue, STRING_CLSID_CONTROLFOLDER );
                }
            }
        }

 
        if (lResult == ERROR_NO_MORE_ITEMS)
        {   // we successfully inspected all the values
            if ( !fFoundValue )
            {
                TCHAR szSubKey[20]; // don't foresee  moure than a few billion caches
                // add new path to list of paths
                wsprintf(szSubKey, "%i", ++lValueIndex);
                lResult = RegSetValueEx( hkeyCacheList, szSubKey, 0, REG_SZ, 
                                         (LPBYTE)szCacheDir, cchCacheDir + 1);
                if ( lResult == ERROR_SUCCESS )
                    hr = S_OK;
                else 
                    hr = HRESULT_FROM_WIN32(lResult);
            } else
                hr = S_OK; // it's already there
        } else
            hr = HRESULT_FROM_WIN32(lResult);

        RegCloseKey( hkeyCacheList );
    } else
        hr = HRESULT_FROM_WIN32(lResult);

    return hr;
}

STDAPI SetCacheRegEntries( LPCTSTR szCacheDir )
{
    HRESULT hr = E_FAIL;
    LONG    lResult;
    HKEY    hkeyIS;    // reg key for internet settings;

    lResult = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                            REG_PATH_IE_SETTINGS,
                            0x0,
                            KEY_ALL_ACCESS,
                            &hkeyIS );
    if ( lResult == ERROR_SUCCESS)
    {
        // now the key is ours, oh, yes... it is ours...
        // set the value of the internet settings key used by Code Download.
        int cchCacheDir = lstrlen(szCacheDir);
        TCHAR szCacheDirOld[MAX_PATH];
        DWORD dwType = REG_SZ;
        DWORD cbOldCache = MAX_PATH;

        // Don't fail if we can't quite hook up with legacy caches
        hr = S_OK;

        // Add our old cache path, if any, to the cache path list if it differs from our new cache.
        lResult = RegQueryValueEx( hkeyIS, REG_OCX_CACHE_VALUE_NAME, 0, &dwType, (LPBYTE)szCacheDirOld, &cbOldCache );
        if ( lResult == ERROR_SUCCESS && dwType == REG_SZ &&
             lstrcmpi( szCacheDirOld, szCacheDir ) != 0 )
            AddCacheToRegPathList( hkeyIS, szCacheDirOld, cbOldCache - 1 );

                // Under NT, IE3 might not have been able to write the old cache path, so we'll cobble one up
                // and add it if that dir is present.
                if ( SUCCEEDED(GetWindowsDirectory( szCacheDirOld, MAX_PATH )) )
                {
            cbOldCache = lstrlen( szCacheDirOld ); 
            if ( cbOldCache && (szCacheDirOld[cbOldCache-1] != '\\'))
                lstrcat(szCacheDirOld, "\\");
                        cbOldCache = lstrlen(lstrcat( szCacheDirOld, REG_OCX_OLD_CACHE_DIR ));         

                        if (PathFileExists(szCacheDirOld))
                        {
                                // Let's not fail if this doesn't work
                                AddCacheToRegPathList( hkeyIS, szCacheDirOld, cbOldCache );
                CreateShellFolderPath( szCacheDirOld, STRING_CLSID_CONTROLFOLDER );
                        }
                }
         
        if ( SUCCEEDED(hr) )
        {
            lResult = RegSetValueEx( hkeyIS, REG_OCX_CACHE_VALUE_NAME, 0, REG_SZ,
                                     (LPBYTE)szCacheDir, cchCacheDir + 1 ); // need '\0'

            if ( lResult == ERROR_SUCCESS )
            {
                // add the new (?) path to the collection of valid paths which are the
                // values for the cache subkey.
                hr = AddCacheToRegPathList( hkeyIS, szCacheDir, cchCacheDir );
            } else
                hr = HRESULT_FROM_WIN32(lResult);
        }

        RegCloseKey( hkeyIS );

    } else
        hr = HRESULT_FROM_WIN32(lResult);
     
    return hr;
}

STDAPI InitCacheFolder(void)
{
    HRESULT hr = E_FAIL;
    TCHAR szCacheDir[MAX_PATH];

    // Compose the default path.
    GetControlFolderPath(szCacheDir, MAX_PATH);
   
    // Okay, now we know where we want to put things.
    // Create the directory, and/or claim it as our own
    hr  = CreateShellFolderPath( szCacheDir, STRING_CLSID_CONTROLFOLDER );
    if ( SUCCEEDED(hr) )
    {
        hr = SetCacheRegEntries( szCacheDir );
    }

    return hr;
}

STDAPI DllUnregisterServer(void)
{
    // Remove a bunch of stuff from the registry.
    //
    CallRegInstall("Unreg");

    return NOERROR;
}

STDAPI DllRegisterServer(void)
{
    //
    // Add a bunch of stuff to the registry.
    //
    if (FAILED(CallRegInstall("Reg")))
    {
        goto CleanUp;
    }

    return NOERROR;

CleanUp:        // cleanup stuff if any of our reg stuff fails

    DllUnregisterServer();
    return E_FAIL;
}

STDAPI DllInstall(BOOL bInstall, LPCWSTR pszCmdLine)
{
    HRESULT hr = S_OK;

    if ( bInstall )
    {
        hr =InitCacheFolder();
        
        if ( SUCCEEDED(hr) )
            CanonicalizeModuleUsage();
    } 
    else
    {
        LONG  lResult;
        HKEY  hkeyCacheList;

        // Unhook occache as a shell extension for the cache folders.
        lResult = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                REG_PATH_IE_CACHE_LIST,
                                0x0,
                                KEY_ALL_ACCESS,
                                &hkeyCacheList );

        if ( lResult == ERROR_SUCCESS ) {
            DWORD dwIndex;
            TCHAR szName[MAX_PATH];
            DWORD cbName;
            TCHAR szValue[MAX_PATH];
            DWORD cbValue;

            for ( dwIndex = 0, cbName = sizeof(szName), cbValue = sizeof(szValue); 
                  lResult == ERROR_SUCCESS; 
                  dwIndex++, cbName = sizeof(szName), cbValue = sizeof(szValue) )
            {
                lResult = RegEnumValue( hkeyCacheList, dwIndex,
                                        szName, &cbName, 
                                        NULL, NULL,
                                        (LPBYTE)szValue, &cbValue );

                if ( lResult == ERROR_SUCCESS && PathFileExists(szValue) )
                    CleanupShellFolder(szValue);
            }
            // We leave this key in place because it is the only record we have of the
            // cache folders and would be useful to future installations of IE
            RegCloseKey( hkeyCacheList );
        }
    }

    return hr;    
}

STDAPI_(BOOL) DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID dwReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        HKEY hkeyTest;

        g_hInst = hInst;
        DisableThreadLibraryCalls(g_hInst);

        MLLoadResources(g_hInst, TEXT("occachlc.dll"));
        
        // Test to see if we have permissions to modify HKLM subkeys.
        // We'll use this as an early test to see if we can remove controls.
        if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                           REG_PATH_IE_SETTINGS,
                           0x0,
                           KEY_ALL_ACCESS,
                           &hkeyTest ) == ERROR_SUCCESS )
        {
            g_fAllAccess = TRUE;
            RegCloseKey( hkeyTest );
        }
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        MLFreeResources(g_hInst);
    }
    return TRUE;
}

typedef struct {
    const IClassFactoryVtbl *cf;
    const CLSID *pclsid;
    HRESULT (STDMETHODCALLTYPE *pfnCreate)(IUnknown *, REFIID, void **);
} OBJ_ENTRY;

extern const IClassFactoryVtbl c_CFVtbl;        // forward

//
// we always do a linear search here so put your most often used things first
//
const OBJ_ENTRY c_clsmap[] = {
    { &c_CFVtbl, &CLSID_ControlFolder,             ControlFolder_CreateInstance },
    { &c_CFVtbl, &CLSID_EmptyControlVolumeCache,   EmptyControl_CreateInstance },
    // add more entries here
    { NULL, NULL, NULL }
};

// static class factory (no allocs!)

STDMETHODIMP CClassFactory_QueryInterface(IClassFactory *pcf, REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, &IID_IClassFactory) || IsEqualIID(riid, &IID_IUnknown))
    {
        *ppvObj = (void *)pcf;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
    DllAddRef();
    return NOERROR;
}

STDMETHODIMP_(ULONG) CClassFactory_AddRef(IClassFactory *pcf)
{
    DllAddRef();
    return 2;
}

STDMETHODIMP_(ULONG) CClassFactory_Release(IClassFactory *pcf)
{
    DllRelease();
    return 1;
}

STDMETHODIMP CClassFactory_CreateInstance(IClassFactory *pcf, IUnknown *punkOuter, REFIID riid, void **ppvObject)
{
    OBJ_ENTRY *this = IToClass(OBJ_ENTRY, cf, pcf);
    return this->pfnCreate(punkOuter, riid, ppvObject);
}

STDMETHODIMP CClassFactory_LockServer(IClassFactory *pcf, BOOL fLock)
{
    if (fLock)
        DllAddRef();
    else
        DllRelease();
    return S_OK;
}

const IClassFactoryVtbl c_CFVtbl = {
    CClassFactory_QueryInterface, CClassFactory_AddRef, CClassFactory_Release,
    CClassFactory_CreateInstance,
    CClassFactory_LockServer
};

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, &IID_IClassFactory) || IsEqualIID(riid, &IID_IUnknown))
    {
        const OBJ_ENTRY *pcls;
        for (pcls = c_clsmap; pcls->pclsid; pcls++)
        {
            if (IsEqualIID(rclsid, pcls->pclsid))
            {
                *ppv = (void *)&(pcls->cf);
                DllAddRef();    // Class Factory keeps dll in memory
                return NOERROR;
            }
        }
    }
    // failure
    *ppv = NULL;
    return CLASS_E_CLASSNOTAVAILABLE;;
}

STDAPI_(void) DllAddRef()
{
    InterlockedIncrement(&g_cRefDll);
}

STDAPI_(void) DllRelease()
{
    InterlockedDecrement(&g_cRefDll);
}

STDAPI DllCanUnloadNow(void)
{
    return g_cRefDll == 0 ? S_OK : S_FALSE;
}
