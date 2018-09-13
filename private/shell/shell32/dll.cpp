/***************************************************************************
 *  dll.c
 *
 *  Standard DLL entry-point functions 
 *
 ***************************************************************************/

#include "shellprv.h"

#include <ntverp.h>
#include <advpub.h>         // For REGINSTALL
#include "fstreex.h"
#include "ids.h"

#ifdef WINNT

#define INSTALL_WEBFOLDERS 1
#include <msi.h>            // For OfficeWebFolders_Install()
void OfficeWebFolders_Install(void);

#endif // WINNT
extern "C" STDAPI_(void) Control_FillCache_RunDLL( HWND hwndStub, HINSTANCE hAppInstance, LPSTR pszCmdLine, int nCmdShow );

// DllGetVersion - New for IE 4.0 shell integrated mode
//
// All we have to do is declare this puppy and CCDllGetVersion does the rest
//
DLLVER_DUALBINARY(VER_PRODUCTVERSION_DW, VER_PRODUCTBUILD_QFE);

HRESULT CallRegInstall(LPCSTR szSection)
{
    HRESULT hr = E_FAIL;
    HINSTANCE hinstAdvPack = LoadLibrary(TEXT("ADVPACK.DLL"));
    if (hinstAdvPack)
    {
        REGINSTALL pfnri = (REGINSTALL)GetProcAddress(hinstAdvPack, "RegInstall");
        if (pfnri)
        {
            char szShdocvwPath[MAX_PATH];
            STRENTRY seReg[] = {
                { "SHDOCVW_PATH", szShdocvwPath },
#ifdef WINNT                
                { "25", "%SystemRoot%" },
                { "11", "%SystemRoot%\\system32" },
#endif
            };
            STRTABLE stReg = { ARRAYSIZE(seReg), seReg };

            // Get the location of shdocvw.dll
#ifdef WINNT
            lstrcpyA(szShdocvwPath, "%SystemRoot%\\system32");
#else
            GetSystemDirectory(szShdocvwPath, SIZECHARS(szShdocvwPath));
#endif
            PathAppendA(szShdocvwPath, "shdocvw.dll");
          
            hr = pfnri(g_hinst, szSection, &stReg);
        }
        // since we only do this from DllInstall() don't load and unload advpack over and over
        // FreeLibrary(hinstAdvPack);
    }
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

STDAPI CreateShowDesktopOnQuickLaunch()
{
    // delete the "_Current Item" key used for tip rotation in welcome.exe on every upgrade
    HKEY hkey;
    if ( ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Setup\\Welcome"), 0, MAXIMUM_ALLOWED, &hkey ) )
    {
       RegDeleteValue(hkey, TEXT("_Current Item"));
       RegCloseKey(hkey);
    }

    // create the "Show Desktop" icon in the quick launch tray
    TCHAR szPath[MAX_PATH];
    if ( SHGetSpecialFolderPath(NULL, szPath, CSIDL_APPDATA, TRUE) )
    {
        TCHAR szQuickLaunch[MAX_PATH];
        LoadString(g_hinst, IDS_QUICKLAUNCH, szQuickLaunch, ARRAYSIZE(szQuickLaunch));

        if ( PathAppend( szPath, szQuickLaunch ) )
        {
            WritePrivateProfileSection( TEXT("Shell"), TEXT("Command=2\0IconFile=explorer.exe,3\0"), szPath );
            WritePrivateProfileSection( TEXT("Taskbar"), TEXT("Command=ToggleDesktop\0"), szPath );

            return S_OK;
        }
    }

    return E_FAIL;
}

STDAPI Favorites_Install(BOOL bInstall);
STDAPI RecentDocs_Install(BOOL bInstall);
STDAPI CDeskHtmlProp_RegUnReg(BOOL bInstall);
#ifdef WINNT
STDAPI_(BOOL) ApplyRegistrySecurity();
#endif
STDAPI_(void) FixPlusIcons();
STDAPI_(void) CleanupFileSystem();

#define KEEP_FAILURE(hrSum, hrLast) if (FAILED(hrLast)) hrSum = hrLast;

STDAPI DllInstall(BOOL bInstall, LPCWSTR pszCmdLine)
{
    HRESULT hrTemp, hr = S_OK;

    // 99/05/03 vtan: If you're reading this section then you are considering
    // adding code to the registration/installation of shell32.dll. There are
    // now 3 schemes to accomplish this task:

    // 1. IE4UINIT.EXE
    //      Check HKLM\Software\Microsoft\Active Setup\Installed Components\{89820200-ECBD-11cf-8B85-00AA005B4383}
    //      This says that if there is a new version of IE5 to launch ie4uinit.exe.
    //      You can add code to this executable if you wish. You need to enlist in
    //      the setup project on \\trango\slmadd using "ieenlist setup"

    // 2. REGSVR32.EXE /n /i:U shell32.dll
    //      Check HKLM\Software\Microsoft\Active Setup\Installed Components\{89820200-ECBD-11cf-8B85-00AA005B4340}
    //      This is executed using the same scheme as IE4UINIT.EXE except that the
    //      executable used is regsvr32.exe with a command line passed to
    //      shell32!DllInstall. Add your code in the section for "U" below.
    //      If you put the code in the section which is NOT "U" then your
    //      code is executed at GUI setup and any changes you make to HKCU
    //      go into the default user (template). Be careful when putting
    //      things here as winlogon.exe (or some other process) may put
    //      your changes into a user profile unconditionally.

    // 3. HIVEUSD.INX
    //      Checks NT build numbers and does command based on the previous build
    //      number and the current build number only executing the changes between
    //      the builds. If you wish to add something using this method, currently
    //      you have to enlist in the setup project on \\rastaman\ntwin using
    //      "enlist -fgs \\rastaman\ntwin -p setup". To find hiveusd.inx go to
    //      nt\private\setup\inf\win4\inf. Add the build number which the delta
    //      is required and a command to launch %SystemRoot%\System32\shmgrate.exe
    //      with one or two parameters. The first parameter tells what command to
    //      execute. The second parameter is optional. shmgrate.exe then finds
    //      shell32.dll and calls shell32!FirstUserLogon. Code here is for upgrading
    //      HKCU user profiles from one NT build to another NT build.
    //      Code is executed in the process context shmgrate.exe and is executed
    //      at a time where no UI is possible. Always use HKLM\Software\Microsoft
    //      \Windows NT\CurrentVersion\Image File Execution Options\Debugger with
    //      "-d".

    // Schemes 1 and 2 work on either Win9x or WinNT but have the sometimes
    // unwanted side effect of ALWAYS getting executed on version upgrades.
    // Scheme 3 only gets executed on build number deltas. Because schemes 1
    // and 2 are always executed, if a user changes (or deletes) that setting
    // it will always get put back. Not so with scheme 3.

    // Ideally, the best solution is have an internal shell32 build number
    // delta scheme which determines the from build and the to build and does
    // a similar mechanism to what hiveusd.inx and shmgrate.exe do. This
    // would probably involve either a common installation function (such as
    // FirstUserLogon()) which is called differently from Win9x and WinNT or
    // common functions to do the upgrade and two entry points (such as
    // FirstUserLogonNT() and FirstUserLogonWin9X().

    if (bInstall)
    {
#ifdef WINNT
        NT_PRODUCT_TYPE type = NtProductWinNt;
        RtlGetNtProductType(&type);
#endif

        // "U" means it's the per user install call
        if (!StrCmpIW(pszCmdLine, L"U"))
        {
            // NOTE: Code in this segment get run during first login.  We want first
            // login to be as quick as possible so try to minimize this section.

            // Put per-user install stuff here.  Any HKCU registration
            // done here is suspect.  (If you are setting defaults, do
            // so in HKLM and use the SHRegXXXUSValue functions.)

            // WARNING: we get called by the ie4unit.exe (ieunit.inf) scheme:
            //      %11%\shell32.dll,NI,U
            // this happens per user, to test this code "regsvr32 /n /i:U shell32.dll"

#ifdef INSTALL_WEBFOLDERS
            // Install the Office WebFolders shell namespace extension per user.
            OfficeWebFolders_Install();
#endif

            // do the work to Install/Uninstall the favorites directory shellext...
            hrTemp = Favorites_Install(bInstall);
            KEEP_FAILURE(hrTemp, hr);

            // do the work to Install/Uninstall the Recent Docs Folder
            hrTemp = RecentDocs_Install(bInstall);
            KEEP_FAILURE(hrTemp, hr);

            hrTemp = CreateShowDesktopOnQuickLaunch();
            KEEP_FAILURE(hrTemp, hr);
            
            // populate the control panel cache
            Control_FillCache_RunDLL(NULL, NULL, NULL, SW_HIDE);

        }
        else
        {
            // Delete any old registration entries, then add the new ones.
            // Keep ADVPACK.DLL loaded across multiple calls to RegInstall.
            // (The inf engine doesn't guarantee DelReg/AddReg order, that's
            // why we explicitly unreg and reg here.)
            //
            hrTemp = CallRegInstall("RegDll");
            KEEP_FAILURE(hrTemp, hr);

#ifdef WINNT
            // I suppose we should call out NT-only registrations, just in case
            // we ever have to ship a win9x based shell again
            hrTemp = CallRegInstall("RegDllNT");
            KEEP_FAILURE(hrTemp, hr);

            // If we are on NT server, do additional stuff
            if (type != NtProductWinNt)
            {
                hrTemp = CallRegInstall("RegDllNTServer");
                KEEP_FAILURE(hrTemp, hr);
            }
#endif

            // This is apparently the only way to get setup to remove all the registry backup
            // for old names no longer in use...
            hrTemp = CallRegInstall("CleanupOldRollback1");
            KEEP_FAILURE(hrTemp, hr);

            hrTemp = CallRegInstall("CleanupOldRollback2");
            KEEP_FAILURE(hrTemp, hr);

            // REVIEW (ToddB): Move this to DllRegisterServer.
            hrTemp = Shell32RegTypeLib();
            KEEP_FAILURE(hrTemp, hr);
#ifdef WINNT
            ApplyRegistrySecurity();
#endif
            FixPlusIcons();
            CleanupFileSystem();
        }
    }
    else
    {
        // We only need one unreg call since all our sections share
        // the same backup information
        hrTemp = CallRegInstall("UnregDll");
        KEEP_FAILURE(hrTemp, hr);
        UnregisterTypeLibrary(&LIBID_Shell32);
    }

    CDeskHtmlProp_RegUnReg(bInstall);
    
    return hr;
}


STDAPI DllRegisterServer(void)
{
    // NT5 setup calls this so it is now safe to put code here.
    return S_OK;
}


STDAPI DllUnregisterServer(void)
{
    return S_OK;
}

//  Migration/Upgrade functions put here for want of a better place
//  See DllInstall() for an explanation of usage.

BOOL    Clone (LPCTSTR pszSourcePath, LPCTSTR pszTargetPath, LPCTSTR pszName)

{
    TCHAR   szSourcePath[MAX_PATH], szTargetPath[MAX_PATH];

    lstrcpy(szSourcePath, pszSourcePath);
    lstrcpy(szTargetPath, pszTargetPath);
    PathAppend(szSourcePath, pszName);
    PathAppend(szTargetPath, pszName);
    return(CopyFile(szSourcePath, szTargetPath, TRUE));
}

STDAPI  CopySampleJPG (void)

{
    TCHAR   szCurrentUserMyPicturesPath[MAX_PATH],
            szDefaultUserMyPicturesPath[MAX_PATH];

    // Here's the logic for copying the "Sample.jpg" file on upgrade.

    // 1. Get the CSIDL_MYPICUTRES path by using shell32!SHGetFolderPath passing (HANDLE)-1
    //    as the hToken meaning "Default User".
    // 2. Copy the file

    // mydocs!PerUserInit is invoked to setup the desktop.ini and do all the other work
    // required to make this correct.

    if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_MYPICTURES, NULL, SHGFP_TYPE_CURRENT, szCurrentUserMyPicturesPath)) &&
        SUCCEEDED(SHGetFolderPath(NULL, CSIDL_MYPICTURES, (HANDLE)-1, SHGFP_TYPE_CURRENT, szDefaultUserMyPicturesPath)))
    {
        TCHAR   szMyDocsDLLPath[MAX_PATH];

        Clone(szDefaultUserMyPicturesPath, szCurrentUserMyPicturesPath, TEXT("Sample.jpg"));
        if (GetSystemDirectory(szMyDocsDLLPath, ARRAYSIZE(szMyDocsDLLPath)) != 0)
        {
            HINSTANCE   hInstMyDocs;

            PathAppend(szMyDocsDLLPath, TEXT("mydocs.dll"));
            hInstMyDocs = LoadLibrary(szMyDocsDLLPath);
            if (hInstMyDocs != NULL)
            {
                typedef void    (*PFNPerUserInit) (void);

                PFNPerUserInit  pfnPerUserInit;

                pfnPerUserInit = (PFNPerUserInit)GetProcAddress(hInstMyDocs, "PerUserInit");
                if (pfnPerUserInit != NULL)
                    pfnPerUserInit();
                FreeLibrary(hInstMyDocs);
            }
        }
    }
    return(S_OK);
}

void    CopyRegistryValues (HKEY hKeyBaseSource, LPCTSTR pszSource, HKEY hKeyBaseTarget, LPCTSTR pszTarget)

{
    DWORD   dwDisposition, dwMaxValueNameSize, dwMaxValueDataSize;
    HKEY    hKeySource, hKeyTarget;

    hKeySource = hKeyTarget = NULL;
    if ((ERROR_SUCCESS == RegOpenKeyEx(hKeyBaseSource,
                                       pszSource,
                                       0,
                                       KEY_READ,
                                       &hKeySource)) &&
        (ERROR_SUCCESS == RegCreateKeyEx(hKeyBaseTarget,
                                         pszTarget,
                                         0,
                                         TEXT(""),
                                         REG_OPTION_NON_VOLATILE,
                                         KEY_ALL_ACCESS,
                                         NULL,
                                         &hKeyTarget,
                                         &dwDisposition)) &&
        (ERROR_SUCCESS == RegQueryInfoKey(hKeySource,
                                          NULL,
                                          NULL,
                                          NULL,
                                          NULL,
                                          NULL,
                                          NULL,
                                          NULL,
                                          &dwMaxValueNameSize,
                                          &dwMaxValueDataSize,
                                          NULL,
                                          NULL)))
    {
        TCHAR   *pszValueName;
        void    *pValueData;

        pszValueName = reinterpret_cast<TCHAR*>(LocalAlloc(LMEM_FIXED, ++dwMaxValueNameSize * sizeof(TCHAR)));
        if (pszValueName != NULL)
        {
            pValueData = LocalAlloc(LMEM_FIXED, dwMaxValueDataSize);
            if (pValueData != NULL)
            {
                DWORD   dwIndex, dwType, dwValueNameSize, dwValueDataSize;

                dwIndex = 0;
                dwValueNameSize = dwMaxValueNameSize;
                dwValueDataSize = dwMaxValueDataSize;
                while (ERROR_SUCCESS == RegEnumValue(hKeySource,
                                                     dwIndex,
                                                     pszValueName,
                                                     &dwValueNameSize,
                                                     NULL,
                                                     &dwType,
                                                     reinterpret_cast<LPBYTE>(pValueData),
                                                     &dwValueDataSize))
                {
                    RegSetValueEx(hKeyTarget,
                                  pszValueName,
                                  0,
                                  dwType,
                                  reinterpret_cast<LPBYTE>(pValueData),
                                  dwValueDataSize);
                    ++dwIndex;
                    dwValueNameSize = dwMaxValueNameSize;
                    dwValueDataSize = dwMaxValueDataSize;
                }
                LocalFree(pValueData);
            }
            LocalFree(pszValueName);
        }
    }
}

STDAPI  MergeDesktopAndNormalStreams (void)

{
    static  const   TCHAR   scszBaseRegistryLocation[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer");
    static  const   int     sciMaximumStreams = 128;
    static  const   TCHAR   sccOldMRUListBase = TEXT('a');

    // Upgrade from NT4 (classic shell) to Windows 2000 (integrated shell)

    // This involves TWO major changes and one minor change:
    //    1. Merging DesktopStreamMRU and StreamMRU
    //    2. Upgrading the MRUList to MRUListEx
    //    3. Leaving the old settings alone for the roaming user profile scenario

    // This also involves special casing the users desktop PIDL because this is
    // stored as an absolute path PIDL in DesktopStream and needs to be stored
    // in Streams\Desktop instead.

    // The conversion is performed in-situ and simultaneously.

    // 1. Open all the keys we are going to need to do the conversion.

    HKEY    hKeyBase, hKeyDesktopStreamMRU, hKeyDesktopStreams, hKeyStreamMRU, hKeyStreams;

    hKeyBase = hKeyDesktopStreamMRU = hKeyDesktopStreams = hKeyStreamMRU = hKeyStreams = NULL;
    if ((ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER,
                                       scszBaseRegistryLocation,
                                       0,
                                       KEY_ALL_ACCESS,
                                       &hKeyBase)) &&
        (ERROR_SUCCESS == RegOpenKeyEx(hKeyBase,
                                       TEXT("DesktopStreamMRU"),
                                       0,
                                       KEY_ALL_ACCESS,
                                       &hKeyDesktopStreamMRU)) &&
        (ERROR_SUCCESS == RegOpenKeyEx(hKeyBase,
                                       TEXT("DesktopStreams"),
                                       0,
                                       KEY_ALL_ACCESS,
                                       &hKeyDesktopStreams)) &&
        (ERROR_SUCCESS == RegOpenKeyEx(hKeyBase,
                                       TEXT("StreamMRU"),
                                       0,
                                       KEY_ALL_ACCESS,
                                       &hKeyStreamMRU)) &&
        (ERROR_SUCCESS == RegOpenKeyEx(hKeyBase,
                                       TEXT("Streams"),
                                       0,
                                       KEY_ALL_ACCESS,
                                       &hKeyStreams)) &&

    // 2. Determine whether this upgrade is needed at all. If the presence of
    // StreamMRU\MRUListEx is detected then stop.

        (ERROR_SUCCESS != RegQueryValueEx(hKeyStreamMRU,
                                         TEXT("MRUListEx"),
                                         NULL,
                                         NULL,
                                         NULL,
                                         NULL)))
    {
        DWORD   *pdwMRUListEx, *pdwMRUListExBase;

        pdwMRUListExBase = pdwMRUListEx = reinterpret_cast<DWORD*>(LocalAlloc(LPTR, sciMaximumStreams * sizeof(DWORD) * 2));
        if (pdwMRUListEx != NULL)
        {
            DWORD   dwLastFreeSlot, dwMRUListSize, dwType;
            TCHAR   *pszMRUList, szMRUList[sciMaximumStreams];

            // 3. Read the StreamMRU\MRUList, iterate thru this list
            // and convert as we go.

            dwLastFreeSlot = 0;
            dwMRUListSize = sizeof(szMRUList);
            if (ERROR_SUCCESS == RegQueryValueEx(hKeyStreamMRU,
                                                 TEXT("MRUList"),
                                                 NULL,
                                                 &dwType,
                                                 reinterpret_cast<LPBYTE>(szMRUList),
                                                 &dwMRUListSize))
            {
                pszMRUList = szMRUList;
                while (*pszMRUList != TEXT('\0'))
                {
                    DWORD   dwValueDataSize;
                    TCHAR   szValue[16];

                    // Read the PIDL information based on the letter in
                    // the MRUList.

                    szValue[0] = *pszMRUList++;
                    szValue[1] = TEXT('\0');
                    if (ERROR_SUCCESS == RegQueryValueEx(hKeyStreamMRU,
                                                         szValue,
                                                         NULL,
                                                         NULL,
                                                         NULL,
                                                         &dwValueDataSize))
                    {
                        DWORD   dwValueType;
                        void    *pValueData;

                        pValueData = LocalAlloc(LMEM_FIXED, dwValueDataSize);
                        if ((pValueData != NULL) &&
                            (ERROR_SUCCESS == RegQueryValueEx(hKeyStreamMRU,
                                                              szValue,
                                                              NULL,
                                                              &dwValueType,
                                                              reinterpret_cast<LPBYTE>(pValueData),
                                                              &dwValueDataSize)))
                        {

                            // Allocate a new number in the MRUListEx for the PIDL.

                            *pdwMRUListEx = szValue[0] - sccOldMRUListBase;
                            wsprintf(szValue, TEXT("%d"), *pdwMRUListEx++);
                            ++dwLastFreeSlot;
                            RegSetValueEx(hKeyStreamMRU,
                                          szValue,
                                          NULL,
                                          dwValueType,
                                          reinterpret_cast<LPBYTE>(pValueData),
                                          dwValueDataSize);
                            LocalFree(pValueData);
                        }
                    }
                }
            }

            // 4. Read the DesktopStreamMRU\MRUList, iterate thru this
            // this and append to the new MRUListEx that is being
            // created as well as copying both the PIDL in DesktopStreamMRU
            // and the view information in DesktopStreams.

            dwMRUListSize = sizeof(szMRUList);
            if (ERROR_SUCCESS == RegQueryValueEx(hKeyDesktopStreamMRU,
                                                 TEXT("MRUList"),
                                                 NULL,
                                                 &dwType,
                                                 reinterpret_cast<LPBYTE>(szMRUList),
                                                 &dwMRUListSize))
            {
                bool    fConvertedEmptyPIDL;
                TCHAR   szDesktopDirectoryPath[MAX_PATH];

                fConvertedEmptyPIDL = false;
                SHGetFolderPath(NULL, CSIDL_DESKTOPDIRECTORY, NULL, SHGFP_TYPE_CURRENT, szDesktopDirectoryPath);
                pszMRUList = szMRUList;
                while (*pszMRUList != TEXT('\0'))
                {
                    DWORD   dwValueDataSize;
                    TCHAR   szSource[16];

                    // Read the PIDL information based on the letter in
                    // the MRUList.

                    szSource[0] = *pszMRUList++;
                    szSource[1] = TEXT('\0');
                    if (ERROR_SUCCESS == RegQueryValueEx(hKeyDesktopStreamMRU,
                                                         szSource,
                                                         NULL,
                                                         NULL,
                                                         NULL,
                                                         &dwValueDataSize))
                    {
                        DWORD   dwValueType;
                        void    *pValueData;

                        pValueData = LocalAlloc(LMEM_FIXED, dwValueDataSize);
                        if ((pValueData != NULL) &&
                            (ERROR_SUCCESS == RegQueryValueEx(hKeyDesktopStreamMRU,
                                                              szSource,
                                                              NULL,
                                                              &dwValueType,
                                                              reinterpret_cast<LPBYTE>(pValueData),
                                                              &dwValueDataSize)))
                        {
                            TCHAR   szTarget[16], szStreamPath[MAX_PATH];

                            if ((SHGetPathFromIDList(reinterpret_cast<LPCITEMIDLIST>(pValueData), szStreamPath) != 0) &&
                                (0 == lstrcmpi(szStreamPath, szDesktopDirectoryPath)))
                            {
                                if (!fConvertedEmptyPIDL)
                                {

                                    // 99/05/24 #343721 vtan: Prefer the desktop relative PIDL
                                    // (empty PIDL) when given a choice of two PIDLs that refer
                                    // to the desktop. The old absolute PIDL is from SP3 and
                                    // earlier days. The new relative PIDL is from SP4 and
                                    // later days. An upgraded SP3 -> SP4 -> SPx -> Windows
                                    // 2000 system will possibly have old absolute PIDLs.
                                    // Check for the empty PIDL. If this is encountered already
                                    // then don't process this stream.

                                    fConvertedEmptyPIDL = ILIsEmpty(reinterpret_cast<LPCITEMIDLIST>(pValueData));
                                    wsprintf(szSource, TEXT("%d"), szSource[0] - sccOldMRUListBase);
                                    CopyRegistryValues(hKeyDesktopStreams, szSource, hKeyStreams, TEXT("Desktop"));
                                }
                            }
                            else
                            {

                                // Allocate a new number in the MRUListEx for the PIDL.

                                *pdwMRUListEx++ = dwLastFreeSlot;
                                wsprintf(szTarget, TEXT("%d"), dwLastFreeSlot++);
                                if (ERROR_SUCCESS == RegSetValueEx(hKeyStreamMRU,
                                                                   szTarget,
                                                                   NULL,
                                                                   dwValueType,
                                                                   reinterpret_cast<LPBYTE>(pValueData),
                                                                   dwValueDataSize))
                                {

                                    // Copy the view information from DesktopStreams to Streams

                                    wsprintf(szSource, TEXT("%d"), szSource[0] - sccOldMRUListBase);
                                    CopyRegistryValues(hKeyDesktopStreams, szSource, hKeyStreams, szTarget);
                                }
                            }
                            LocalFree(pValueData);
                        }
                    }
                }
            }
            *pdwMRUListEx++ = static_cast<DWORD>(-1);
            RegSetValueEx(hKeyStreamMRU,
                          TEXT("MRUListEx"),
                          NULL,
                          REG_BINARY,
                          reinterpret_cast<LPCBYTE>(pdwMRUListExBase),
                          ++dwLastFreeSlot * sizeof(DWORD));
            LocalFree(reinterpret_cast<HLOCAL>(pdwMRUListExBase));
        }
    }
    if (hKeyStreams != NULL)
        RegCloseKey(hKeyStreams);
    if (hKeyStreamMRU != NULL)
        RegCloseKey(hKeyStreamMRU);
    if (hKeyDesktopStreams != NULL)
        RegCloseKey(hKeyDesktopStreams);
    if (hKeyDesktopStreamMRU != NULL)
        RegCloseKey(hKeyDesktopStreamMRU);
    if (hKeyBase != NULL)
        RegCloseKey(hKeyBase);
    return(S_OK);
}

static  const   int     s_ciMaximumNumericString = 32;

int     GetRegistryStringValueAsInteger (HKEY hKey, LPCTSTR pszValue, int iDefaultValue)

{
    int     iResult;
    DWORD   dwType, dwStringSize;
    TCHAR   szString[s_ciMaximumNumericString];

    dwStringSize = sizeof(szString);
    if (ERROR_SUCCESS == RegQueryValueEx(hKey,
                                         pszValue,
                                         NULL,
                                         &dwType,
                                         reinterpret_cast<LPBYTE>(szString),
                                         &dwStringSize) && (dwType == REG_SZ))
    {
        iResult = StrToInt(szString);
    }
    else
    {
        iResult = iDefaultValue;
    }
    return(iResult);
}

void    SetRegistryIntegerAsStringValue (HKEY hKey, LPCTSTR pszValue, int iValue)

{
    TCHAR   szString[s_ciMaximumNumericString];

    wnsprintf(szString, ARRAYSIZE(szString), TEXT("%d"), iValue);
    TW32(RegSetValueEx(hKey,
                       pszValue,
                       0,
                       REG_SZ,
                       reinterpret_cast<LPBYTE>(szString),
                       (lstrlen(szString) + sizeof('\0')) * sizeof(TCHAR)));
}

STDAPI  MoveAndAdjustIconMetrics (void)

{
    // 99/06/06 #309198 vtan: The following comes from hiveusd.inx which is
    // where this functionality used to be executed. It used to consist of
    // simple registry deletion and addition. This doesn't work on upgrade
    // when the user has large icons (Shell Icon Size == 48).

    // In this case that metric must be moved and the new values adjusted
    // so that the metric is preserved should the user then decide to turn
    // off large icons.

    // To restore old functionality, remove the entry in hiveusd.inx at
    // build 1500 which is where this function is invoked and copy the
    // old text back in.

/*
    HKR,"1508\Hive\2","Action",0x00010001,3
    HKR,"1508\Hive\2","KeyName",0000000000,"Control Panel\Desktop\WindowMetrics"
    HKR,"1508\Hive\2","Value",0000000000,"75"
    HKR,"1508\Hive\2","ValueName",0000000000,"IconSpacing"
    HKR,"1508\Hive\3","Action",0x00010001,3
    HKR,"1508\Hive\3","KeyName",0000000000,"Control Panel\Desktop\WindowMetrics"
    HKR,"1508\Hive\3","Value",0000000000,"1"
    HKR,"1508\Hive\3","ValueName",0000000000,"IconTitleWrap"
*/

    // Icon metric keys have moved from HKCU\Control Panel\Desktop\Icon*
    // to HKCU\Control Panel\Desktop\WindowMetrics\Icon* but only 3 values
    // should be moved. These are "IconSpacing", "IconTitleWrap" and
    // "IconVerticalSpacing". This code is executed before the deletion
    // entry in hiveusd.inx so that it can get the values before they
    // are deleted. The addition section has been remove (it's above).

    static  const   TCHAR   s_cszIconSpacing[] = TEXT("IconSpacing");
    static  const   TCHAR   s_cszIconTitleWrap[] = TEXT("IconTitleWrap");
    static  const   TCHAR   s_cszIconVerticalSpacing[] = TEXT("IconVerticalSpacing");

    static  const   int     s_ciStandardOldIconSpacing = 75;
    static  const   int     s_ciStandardNewIconSpacing = -1125;

    HKEY    hKeyDesktop, hKeyWindowMetrics;

    hKeyDesktop = hKeyWindowMetrics = NULL;
    if ((ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER,
                                       TEXT("Control Panel\\Desktop"),
                                       0,
                                       KEY_ALL_ACCESS,
                                       &hKeyDesktop)) &&
        (ERROR_SUCCESS == RegOpenKeyEx(hKeyDesktop,
                                       TEXT("WindowMetrics"),
                                       0,
                                       KEY_ALL_ACCESS,
                                       &hKeyWindowMetrics)))
    {
        int     iIconSpacing, iIconTitleWrap, iIconVerticalSpacing;

        // 1. Read the values that we wish the move and adjust.

        iIconSpacing = GetRegistryStringValueAsInteger(hKeyDesktop, s_cszIconSpacing, s_ciStandardOldIconSpacing);
        iIconTitleWrap = GetRegistryStringValueAsInteger(hKeyDesktop, s_cszIconTitleWrap, 1);
        iIconVerticalSpacing = GetRegistryStringValueAsInteger(hKeyDesktop, s_cszIconVerticalSpacing, s_ciStandardOldIconSpacing);

        // 2. Perform the adjustment.

        iIconSpacing = s_ciStandardNewIconSpacing * iIconSpacing / s_ciStandardOldIconSpacing;
        iIconVerticalSpacing = s_ciStandardNewIconSpacing * iIconVerticalSpacing / s_ciStandardOldIconSpacing;

        // 3. Write the values back out in the new (moved) location.

        SetRegistryIntegerAsStringValue(hKeyWindowMetrics, s_cszIconSpacing, iIconSpacing);
        SetRegistryIntegerAsStringValue(hKeyWindowMetrics, s_cszIconTitleWrap, iIconTitleWrap);
        SetRegistryIntegerAsStringValue(hKeyWindowMetrics, s_cszIconVerticalSpacing, iIconVerticalSpacing);

        // 4. Let winlogon continue processing hiveusd.inx and delete the
        // old entries in the process. We already created the new entries
        // and that has been removed from hiveusd.inx.

    }
    if (hKeyWindowMetrics != NULL)
        TW32(RegCloseKey(hKeyWindowMetrics));
    if (hKeyDesktop != NULL)
        TW32(RegCloseKey(hKeyDesktop));
    return(S_OK);
}

extern  "C"     STDAPI  FirstUserLogon (LPCSTR pcszCommand, LPCSTR pcszOptionalArguments)

{
    enum
    {
        kCommandCopySampleJPG,
        kCommandMergeDesktopAndNormalStreams,
        kCommandMoveAndAdjustIconMetrics
    };

    typedef struct
    {
        LPCSTR  pcszCommand;
        int     iCommand;
    } tCommandElement;

    tCommandElement     sCommands[] =
    {
        { "CopySampleJPG",                  kCommandCopySampleJPG                  },
        { "MergeDesktopAndNormalStreams",   kCommandMergeDesktopAndNormalStreams   },
        { "MoveAndAdjustIconMetrics",       kCommandMoveAndAdjustIconMetrics       },
    };

    HRESULT     hResult;
    int         i;

    // Match what shmgrate.exe passed us and execute the command.
    // Only use the optional argument if required. Note this is
    // done ANSI because the original command line is ANSI from
    // shmgrate.exe.

    for (i = 0; (i < ARRAYSIZE(sCommands)) && (lstrcmpA(pcszCommand, sCommands[i].pcszCommand) != 0); ++i)
        ;
    switch (sCommands[i].iCommand)
    {
        case kCommandCopySampleJPG:
            hResult = CopySampleJPG();
            break;
        case kCommandMergeDesktopAndNormalStreams:
            hResult = MergeDesktopAndNormalStreams();
            break;
        case kCommandMoveAndAdjustIconMetrics:
            hResult = MoveAndAdjustIconMetrics();
            break;
        default:
            hResult = E_FAIL;
            break;
    }
    return(hResult);
}

#ifdef INSTALL_WEBFOLDERS
//
// WebFolders namespace extension installation.
// This is the code that initially installs the Office WebFolders 
// shell namespace extension on the computer.  Code in shmgrate.exe
// (see private\windows\shell\migrate) performs per-user
// web folders registration duties.
//
typedef UINT (WINAPI * PFNMSIINSTALLPRODUCT)(LPCTSTR, LPCTSTR);
typedef INSTALLUILEVEL (WINAPI * PFNMSISETINTERNALUI)(INSTALLUILEVEL, HWND *);

#define GETPROC(var, hmod, ptype, fn)  ptype var = (ptype)GetProcAddress(hmod, fn)

#define API_MSISETINTERNALUI  "MsiSetInternalUI"
#ifdef UNICODE
#   define API_MSIINSTALLPRODUCT "MsiInstallProductW"
#else
#   define API_MSIINSTALLPRODUCT "MsiInstallProductA"
#endif

typedef struct _WEBFOLDER_INSTALL_RETRY_STRUCT {
    HMODULE hmod;
    PFNMSISETINTERNALUI pfnMsiSetInternalUI;
    PFNMSIINSTALLPRODUCT pfnMsiInstallProduct;
} WEBFOLDER_INSTALL_RETRY_STRUCT;
   

void WebFolder_Install_RetryThreadProc(WEBFOLDER_INSTALL_RETRY_STRUCT * pwirs)
{
    ASSERT(pwirs);
    ASSERT(pwirs->hmod);
    ASSERT(pwirs->pfnMsiSetInternalUI);
    ASSERT(pwirs->pfnMsiInstallProduct);

    TCHAR szPath[MAX_PATH];
    UINT uRet = 0;
    INSTALLUILEVEL oldUILevel = pwirs->pfnMsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    do {
        GetSystemDirectory(szPath, ARRAYSIZE(szPath));
        PathAppend(szPath, TEXT("webfldrs.msi"));
        uRet = pwirs->pfnMsiInstallProduct(szPath, TEXT(""));
    } while (uRet == ERROR_INSTALL_ALREADY_RUNNING);

    pwirs->pfnMsiSetInternalUI(oldUILevel, NULL);
    DllRelease();
    FreeLibrary(pwirs->hmod);
    LocalFree(pwirs);
}

void OfficeWebFolders_Install(void)
{
    HMODULE hmod = LoadLibrary(TEXT("msi.dll"));
    if (hmod)
    {
        BOOL bFreeLib = FALSE;
        GETPROC(pfnMsiSetInternalUI,  hmod, PFNMSISETINTERNALUI,  API_MSISETINTERNALUI);
        GETPROC(pfnMsiInstallProduct, hmod, PFNMSIINSTALLPRODUCT, API_MSIINSTALLPRODUCT);

        if (pfnMsiSetInternalUI && pfnMsiInstallProduct)
        {
            TCHAR szPath[MAX_PATH];
            GetSystemDirectory(szPath, ARRAYSIZE(szPath));
            PathAppend(szPath, TEXT("webfldrs.msi"));

            //
            // Use "silent" install mode.  No UI.
            //
            INSTALLUILEVEL oldUILevel = pfnMsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

            //
            // Install the web folders MSI package.
            //
            if (pfnMsiInstallProduct(szPath, TEXT("")) == ERROR_INSTALL_ALREADY_RUNNING)
            {
                WEBFOLDER_INSTALL_RETRY_STRUCT * pwirs = (WEBFOLDER_INSTALL_RETRY_STRUCT *)LocalAlloc(LPTR, SIZEOF(WEBFOLDER_INSTALL_RETRY_STRUCT));
                if (pwirs)
                {
                    DWORD thid;     // Not used but we have to pass something in

                    pwirs->hmod = hmod;
                    pwirs->pfnMsiSetInternalUI = pfnMsiSetInternalUI;
                    pwirs->pfnMsiInstallProduct = pfnMsiInstallProduct;

                    DllAddRef();
                    HANDLE hthreadWorker = CreateThread(NULL, 0,
                        (LPTHREAD_START_ROUTINE)WebFolder_Install_RetryThreadProc,
                        (LPVOID)pwirs, CREATE_SUSPENDED, &thid);
                    
                    if (hthreadWorker)
                    {
                        // Demote the priority so it doesn't interfere with the
                        SetThreadPriority(hthreadWorker, THREAD_PRIORITY_BELOW_NORMAL);
                        ResumeThread(hthreadWorker);
                        bFreeLib = FALSE;
                    }
                    else
                    {
                        DllRelease();
                        LocalFree(pwirs);
                    }
                }
            }
            
            pfnMsiSetInternalUI(oldUILevel, NULL);

        }

        if (bFreeLib)
            FreeLibrary(hmod);
    }
}

#endif // INSTALL_WEBFOLDERS


#ifdef WINNT
// now is the time on sprockets when we lock down the registry
STDAPI_(BOOL) ApplyRegistrySecurity()
{
    BOOL fSuccess = FALSE;      // assume failure
    SECURITY_DESCRIPTOR* psd;
    SHELL_USER_PERMISSION supEveryone;
    SHELL_USER_PERMISSION supSystem;
    SHELL_USER_PERMISSION supAdministrators;
    PSHELL_USER_PERMISSION aPerms[3] = {&supEveryone, &supSystem, &supAdministrators};


    // we want the "Everyone" to have read access
    supEveryone.susID = susEveryone;
    supEveryone.dwAccessType = ACCESS_ALLOWED_ACE_TYPE;
    supEveryone.dwAccessMask = KEY_READ;
    supEveryone.fInherit = TRUE;
    supEveryone.dwInheritMask = (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);
    supEveryone.dwInheritAccessMask = GENERIC_READ;

    // we want the "SYSTEM" to have full control
    supSystem.susID = susSystem;
    supSystem.dwAccessType = ACCESS_ALLOWED_ACE_TYPE;
    supSystem.dwAccessMask = KEY_ALL_ACCESS;
    supSystem.fInherit = TRUE;
    supSystem.dwInheritMask = (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);
    supSystem.dwInheritAccessMask = GENERIC_ALL;

    // we want the "Administrators" to have full control
    supAdministrators.susID = susAdministrators;
    supAdministrators.dwAccessType = ACCESS_ALLOWED_ACE_TYPE;
    supAdministrators.dwAccessMask = KEY_ALL_ACCESS;
    supAdministrators.fInherit = TRUE;
    supAdministrators.dwInheritMask = (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);
    supAdministrators.dwInheritAccessMask = GENERIC_ALL;


    psd = GetShellSecurityDescriptor(aPerms, ARRAYSIZE(aPerms));

    if (psd)
    {
        HKEY hkLMBitBucket;

        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\BitBucket"), 0, KEY_ALL_ACCESS, &hkLMBitBucket) == ERROR_SUCCESS)
        {
            if (RegSetKeySecurity(hkLMBitBucket, DACL_SECURITY_INFORMATION, psd) == ERROR_SUCCESS)
            {
                // victory is mine!
                fSuccess = TRUE;
            }

            RegCloseKey(hkLMBitBucket);
        }

        LocalFree(psd);
    }

    return fSuccess;
}
#endif // WINNT


CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    // nothing in here, use clsobj.c class table instead
END_OBJECT_MAP()

// ATL DllMain, needed to support our ATL classes that depend on _Module
// REVIEW: confirm that _Module is really needed

STDAPI_(BOOL) ATL_DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        _Module.Init(ObjectMap, hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        _Module.Term();
    }
    return TRUE;    // ok
}
