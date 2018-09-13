#include "priv.h"
#include <iethread.h>
#include "winlist.h"
#include "htregmng.h"
#include "resource.h"
#include "mttf.h"

#include <mluisupp.h>

//BugBug  inststub uses kernel string funcions and unbounded buffer functions
#undef lstrcpyn
#undef lstrcmp
#undef lstrcpy
#undef lstrcmpi

#undef  StrCpyW
#define lstrcpy    StrCpyW
#define lstrcpyn   StrCpyNW
#define lstrcmp    StrCmpW
#define lstrcmpi   StrCmpIW

#include <inststub.h>

#undef lstrcpyn
#undef lstrcmp
#undef lstrcpy
#undef lstrcmpi

#define lstrcpyn       Do_not_use_lstrcpyn_use_StrCpyN
#define lstrcmp        Do_not_use_lstrcmp_use_StrCmp
#define lstrcpy        Do_not_use_lstrcpy_use_StrCpyN
#define lstrcmpi       Do_not_use_lstrcmpi_use_StrCmpI

#define StrCpyW        Do_not_use_StrCpyW_use_StrCpyNW

#ifdef UNIX
#include "unixstuff.h"
#endif

/* Old install stub API (no parameters) for compatibility for a few builds */
EXTERN_C void RunInstallUninstallStubs(void)
{
    RunInstallUninstallStubs2(NULL);
}


void IERevokeClassFactoryObject(void);

#ifndef POSTPOSTSPLIT
// This value will be initialized to 0 only when we are under IExplorer.exe
UINT g_tidParking = 0;
#endif


#define DM_FAVORITES 0

#ifdef BETA_WARNING
#pragma message("buidling with time bomb enabled")
void DoTimebomb(HWND hwnd)
{
    SYSTEMTIME st;
    GetSystemTime(&st);

    // BUGBUG: Remove after the beta-release!!!
    //
    // Revision History:
    //  End of October, 1996
    //  April, 1997
    //  September, 1997 (for beta-1)
    //  November 15th, 1997 (for beta-2)
    //
    if (st.wYear > 1997 || (st.wYear==1997 && st.wMonth > 11) ||
            (st.wYear==1997 && st.wMonth == 11 && st.wDay > 15))
    {
        TCHAR szTitle[128];
        TCHAR szBeta[512];

        MLLoadShellLangString(IDS_CABINET, szTitle, ARRAYSIZE(szTitle));
        MLLoadShellLangString(IDS_BETAEXPIRED, szBeta, ARRAYSIZE(szBeta));

        MessageBox(hwnd, szBeta, szTitle, MB_OK);
    }
}
#else
#define DoTimebomb(hwnd)
#endif


/*----------------------------------------------------------
Purpose: Initialize the favorites folder if it doesn't exist.

Returns: --

Cond:    As a side-effect, SHGetSpecialFolderPath calls Ole
         functions.  So this function must be called after
         OleInitialize has been called.

Note:    This is only really required on win95 / NT4 in
         browser only mode.  The shell32.dll that ships
         with IE4 can handle CSIDL_FAVORITES with fCreate=TRUE.
*/
void InitFavoritesDir()
{
    TCHAR szPath[MAX_PATH];

    if (!SHGetSpecialFolderPath(NULL, szPath, CSIDL_FAVORITES, TRUE))
    {
        TCHAR szFavorites[80];

        TraceMsg(DM_FAVORITES, "InitFavoritesDir -- no favorites");

        // if this failed, that means we need to create it ourselves
        GetWindowsDirectory(szPath, ARRAYSIZE(szPath));
        MLLoadString(IDS_FAVORITES, szFavorites, ARRAYSIZE(szFavorites));
        PathCombine(szPath, szPath, szFavorites);
        SHCreateDirectory(NULL, szPath);

        HKEY hkExplorer;
        if (RegCreateKey(HKEY_CURRENT_USER, REGSTR_PATH_EXPLORER, &hkExplorer) == ERROR_SUCCESS)
        {
            HKEY hkUSF;

            if (RegCreateKey(hkExplorer, TEXT("User Shell Folders"), &hkUSF) == ERROR_SUCCESS)
            {
                BOOL f;

                TraceMsg(DM_FAVORITES, "InitFavoritesDir -- created in %s", szPath);

                RegSetValueEx(hkUSF, TEXT("Favorites"), 0, REG_SZ, (LPBYTE)szPath, (1 + lstrlen(szPath)) * SIZEOF(TCHAR));
                f = SHGetSpecialFolderPath(NULL, szPath, CSIDL_FAVORITES, TRUE);
                TraceMsg(DM_FAVORITES, "InitFavoritesDir -- cached at %d %s", f, szPath);

                ASSERT(f);
                RegCloseKey(hkUSF);
            }

            RegCloseKey(hkExplorer);
        }
    }
}


#ifdef ENABLE_CHANNELS
//
// Copy ChanBarSetAutoLaunchRegValue from browseui.
//
//extern void ChanBarSetAutoLaunchRegValue(BOOL fAutoLaunch);
void ChanBarSetAutoLaunchRegValue(BOOL fAutoLaunch)
{
    SHRegSetUSValue(TEXT("Software\\Microsoft\\Internet Explorer\\Main"), 
                    TEXT("Show_ChannelBand"), REG_SZ, 
                    fAutoLaunch ? TEXT("yes") : TEXT("no"),
                    sizeof(fAutoLaunch ? TEXT("yes") : TEXT("no")), 
                    SHREGSET_HKCU | SHREGSET_FORCE_HKCU);
}

#endif  // ENABLE_CHANNELS

STDAPI SHCreateSplashScreen(ISplashScreen ** pSplash);
typedef BOOL (*PFNISDEBUGGERPRESENT)(void);
void CUrlHistory_CleanUp();

//
// Mean Time To Failure check routines
//
#define REG_STR_IE    TEXT("SOFTWARE\\Microsoft\\Internet Explorer\\Main")
#define REG_STR_CLEAN TEXT("CleanBrowserShutdown")

LONG GetSessionCount();

void WriteCleanBrowserShutdown(BOOL fClean)
{
    HKEY hkey;
    DWORD dwValue = (DWORD)fClean;

    //
    // Only do MTTF on the first browser session
    //
    if (GetSessionCount() != 0)
        return;
    
    if (RegOpenKeyEx(HKEY_CURRENT_USER, REG_STR_IE, 0, KEY_WRITE, &hkey) 
        == ERROR_SUCCESS)
    {
        RegSetValueEx(hkey, REG_STR_CLEAN, 
            0, REG_DWORD, (LPBYTE)&dwValue, sizeof(dwValue));
        RegCloseKey(hkey);
    }
}

BOOL ReadCleanBrowserShutdown()
{
    HKEY hkey;
    DWORD dwValue = 1; // default to clean shutdown
    DWORD dwSize;

    //
    // Only do MTTF on the first browser session
    //
    if (GetSessionCount() != 0)
        return TRUE;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, REG_STR_IE, 0, KEY_READ, &hkey) 
        == ERROR_SUCCESS)
    {
        RegQueryValueEx(hkey, REG_STR_CLEAN, NULL, NULL, (LPBYTE)&dwValue, &dwSize);
        RegCloseKey(hkey);
    }
    return (BOOL)dwValue;
}

//
// Post a message to the MTTF window if it is present
// The MTTF tool tracks this process being up
//
#define STR_MTTF_WINDOW_CLASS TEXT("HARVEYCAT")

void PostMTTFMessage(WPARAM mttfMsg)
{
#ifndef UNIX
    HWND hwndVerFind = FindWindow(STR_MTTF_WINDOW_CLASS, NULL);
    if (hwndVerFind)
    {
        PostMessage(hwndVerFind, WM_COMMAND, 
                    (WPARAM) mttfMsg, 
                    (LPARAM) GetCurrentProcessId());
    }
#endif
}

void _TweakCurrentDirectory()
{
#ifndef UNIX
    TCHAR szPath[MAX_PATH];
    if (SHGetSpecialFolderPath(NULL, szPath, CSIDL_DESKTOPDIRECTORY, FALSE))
        SetCurrentDirectory(szPath);
#endif
}

BOOL _IsDebuggerPresent()
{
    static BOOL bDebugger = -1;
    if (bDebugger == -1)
    {
        bDebugger = FALSE;
        // See if a debugger is present and bail on splash screen
        // so we don't get in the way of people... This api is only
        // present on NT...
        if (g_fRunningOnNT)
        {
            PFNISDEBUGGERPRESENT pfndebugger = (PFNISDEBUGGERPRESENT)GetProcAddress(GetModuleHandle(TEXT("KERNEL32")), "IsDebuggerPresent");
            if (pfndebugger)
                bDebugger = pfndebugger();
        }
    }
    return bDebugger;
}

BOOL g_fBrowserOnlyProcess = FALSE;

// this entry is used by IEXPLORE.EXE to run the browser in a separate process. this is the
// standard setting, but browsers (IE) can also be run in the same process if BrowseInSeparateProcess
// is turned off (for better perf, worse stability)

STDAPI_(int) IEWinMain(LPSTR pszCmdLine, int nCmdShow)
{
    //  this flag indicates that this
    //  browser is running in its own process
    //  and is not integrated with the shell
    //  even if it is running on an intgrated shell
    g_fBrowserOnlyProcess = TRUE;

    _TweakCurrentDirectory();

    //
    // Mean Time To Failure support
    //
    PostMTTFMessage(MTTF_STARTUP);
        
    if (g_dwProfileCAP & 0x00000001)
        StartCAP();

#ifdef FULL_DEBUG
    // Turn off GDI batching so that paints are performed immediately
    GdiSetBatchLimit(1);
#endif

    ASSERT(g_tidParking == 0);

    g_tidParking = GetCurrentThreadId();

    DebugMemLeak(DML_TYPE_MAIN | DML_BEGIN);

    ISplashScreen *pSplash = NULL;
        
#ifndef UNIX
    // Show splash screen, be simple for beta 1...
    if (!_IsDebuggerPresent())
    {
        if (SUCCEEDED(SHCreateSplashScreen(&pSplash)))
        {
            HWND hSplash;
            pSplash->Show( HINST_THISDLL, -1, -1, &hSplash );
        }
    }            
#endif

    if (SUCCEEDED(OleInitialize(NULL)))
    {
        BOOL fWeOwnWinList = WinList_Init();
        IETHREADPARAM* piei = SHCreateIETHREADPARAM(NULL, nCmdShow, NULL, NULL);
        if (piei) 
        {
            //
            // Create favorites dir (by hand if necessary).
            //
            InitFavoritesDir();

            //
            //  If we are opening IE with no parameter, check if this is
            // the very first open.
            //
            piei->pSplash = pSplash;
            if (pszCmdLine && pszCmdLine[0])
            {
                WCHAR wszBuf[MAX_URL_STRING];
                LPCWSTR pwszCmdLine = wszBuf;
                
                SHAnsiToUnicode(pszCmdLine, wszBuf, ARRAYSIZE(wszBuf));
                SHParseIECommandLine(&pwszCmdLine, piei);
                // If the "-channelband" option is selected, turn it ON by default
#ifdef ENABLE_CHANNELS
                if (piei->fDesktopChannel)
                    ChanBarSetAutoLaunchRegValue(TRUE);
#endif  // ENABLE_CHANNELS
                piei->pszCmdLine = StrDupW(pwszCmdLine);
            }
            else
            {
                piei->fCheckFirstOpen = TRUE;
            }

#ifdef UNIX
            if ( piei->fShouldStart && CheckAndDisplayEULA() )
            {
#ifndef NO_SPLASHSCREEN
                // IEUNIX - should show after parsing the commandline options.
                if (!_IsDebuggerPresent())
                {
                    if (SUCCEEDED(SHCreateSplashScreen(&pSplash)))
                    {
                        HWND hSplash;
                        pSplash->Show( HINST_THISDLL, -1, -1, &hSplash );
                    }
                }            
#endif
#endif
                DoTimebomb(NULL);
                piei->uFlags |= (COF_CREATENEWWINDOW | COF_NOFINDWINDOW | COF_INPROC | COF_IEXPLORE);
                
                SHOpenFolderWindow(piei);
#ifdef UNIX
            } // Start browser only if user accepts the license.
#endif
        }

        IERevokeClassFactoryObject();
        CUrlHistory_CleanUp();

        if (fWeOwnWinList)
            WinList_Terminate();

        InternetSetOption(NULL, INTERNET_OPTION_DIGEST_AUTH_UNLOAD, NULL, 0);

        OleUninitialize();
    }

    ATOMICRELEASE(g_psfInternet);

#ifdef DEBUG
    CoFreeUnusedLibraries();
#endif

    DebugMemLeak(DML_TYPE_MAIN | DML_END);

    TraceMsg(TF_SHDTHREAD, "IEWinMain about to call ExitProcess");

    if (g_dwProfileCAP & 0x00020000)
        StopCAP();

    PostMTTFMessage(MTTF_SHUTDOWN); // Mean Time To Failure Support

#ifdef UNIX
    MwExecuteAtExit();
    if (g_dwStopWatchMode)
        StopWatchFlush();
#endif

    ExitProcess(0);
    return TRUE;
}


