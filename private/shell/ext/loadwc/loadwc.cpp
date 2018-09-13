//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       LoadWC.cpp
//
//  Contents:   exe to load webcheck
//
//  Classes:
//
//  Functions:
//
//  History:    12-12/96    rayen (Raymond Endres)  Created
//
//----------------------------------------------------------------------------

#include <windows.h>
#include <docobj.h>
#include <webcheck.h>
#include <shlguid.h>
#include <shlobj.h>
#include <shellp.h>

//
// Channels are enabled for the IE4 upgrades.
//
#define ENABLE_CHANNELS

#ifdef DEBUG
#define ASSERT(b)   if (!(b)) DebugBreak();
#else
#define ASSERT(b)
#endif

#define MLUI_INIT
#include <mluisupp.h>

//
// NOTE: ActiveSetup relies on our window name and class name
// to shut us down properly in softboot.  Do not change it.
//
const TCHAR c_szClassName[] = TEXT("LoadWC");
const TCHAR c_szWebCheckWindow[] = TEXT("MS_WebcheckMonitor");
const TCHAR c_szWebcheckKey[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Webcheck");

// message send to dynamically start sens/lce (must not conflict with dialmon)
#define WM_START_SENSLCE    (WM_USER+200)

// only used in debug code to grovel with shell service object
#ifdef DEBUG
const TCHAR c_szWebCheck[] = TEXT("WebCheck");
const TCHAR c_szShellReg[] = TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ShellServiceObjectDelayLoad");
#endif

typedef struct {
    HINSTANCE   hInstance;          // handle to current instance
    HWND        hwnd;               // main window handle
    int         nCmdShow;           // hidden or not?
    HINSTANCE   hWebcheck;          // handle to webcheck dll
    BOOL        fUninstallOnly;     // TRUE -> run uninstall stubs only, then quit
    BOOL        fIntShellMode;      // TRUE -> integrated shell mode, else browser-only
    BOOL        fStartSensLce;
} GLOBALS;

GLOBALS g;

// webcheck function we dynaload
typedef HRESULT (WINAPI *PFNSTART)(BOOL fForceExternals);
typedef HRESULT (WINAPI *PFNSTOP)(void);

// Code to run install/uninstall stubs, from shell\inc.

#define HINST_THISDLL   g.hInstance

#include "resource.h"
//#include <stubsup.h>
#include <inststub.h>

int WINAPI WinMainT(HINSTANCE, HINSTANCE, LPSTR, int);
LRESULT APIENTRY WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void vLoadWebCheck(void);
void vUnloadWebCheck(void);
BOOL bParseCommandLine(LPSTR lpCmdLine, int nCmdShow);

//----------------------------------------------------------------------------
// ModuleEntry
//----------------------------------------------------------------------------
extern "C" int _stdcall ModuleEntry(void)
{
    int i;
    STARTUPINFOA si;
    LPTSTR pszCmdLine;

    pszCmdLine = GetCommandLine();

    // g_hProcessHeap = GetProcessHeap();

    //
    // We don't want the "No disk in drive X:" requesters, so we set
    // the critical error mask such that calls will just silently fail
    //
    SetErrorMode(SEM_FAILCRITICALERRORS);

    if ( *pszCmdLine == TEXT('\"') ) {
        /*
         * Scan, and skip over, subsequent characters until
         * another double-quote or a null is encountered.
         */
        while ( *++pszCmdLine && (*pszCmdLine
             != TEXT('\"')) );
        /*
         * If we stopped on a double-quote (usual case), skip
         * over it.
         */
        if ( *pszCmdLine == TEXT('\"') )
            pszCmdLine++;
    }
    else {
        while (*pszCmdLine > TEXT(' '))
            pszCmdLine++;
    }

    /*
     * Skip past any white space preceeding the second token.
     */
    while (*pszCmdLine && (*pszCmdLine <= TEXT(' '))) {
        pszCmdLine++;
    }

    si.dwFlags = 0;
    GetStartupInfoA(&si);

    i = WinMainT(GetModuleHandle(NULL), NULL, pszCmdLine,
                   si.dwFlags & STARTF_USESHOWWINDOW ? si.wShowWindow : SW_SHOWDEFAULT);

    // Since we now have a way for an extension to tell us when it is finished,
    // we will terminate all processes when the main thread goes away.

    ExitProcess(i);

    // DebugMsg(DM_TRACE, TEXT("c.me: Cabinet main thread exiting without ExitProcess."));
    return i;
}

//----------------------------------------------------------------------------
// Registry helper function
//----------------------------------------------------------------------------
BOOL ReadRegValue(HKEY hkeyRoot, const TCHAR *pszKey, const TCHAR *pszValue, 
                   void *pData, DWORD dwBytes)
{
    long    lResult;
    HKEY    hkey;
    DWORD   dwType;

    lResult = RegOpenKey(hkeyRoot, pszKey, &hkey);
    if (lResult != ERROR_SUCCESS) {
        return FALSE;
    }

    lResult = RegQueryValueEx(hkey, pszValue, NULL, &dwType, (BYTE *)pData, 
        &dwBytes);
    RegCloseKey(hkey);

    if (lResult != ERROR_SUCCESS) 
        return FALSE;
    
    return TRUE;
}

BOOL WriteRegValue(HKEY hkeyRoot, const TCHAR *pszKey, const TCHAR *pszValue,
                   DWORD dwType, void *pData, DWORD dwBytes)
{
    HKEY hkey;

    long lResult = RegOpenKey(hkeyRoot, pszKey, &hkey);

    if (ERROR_SUCCESS == lResult)
    {
        lResult = RegSetValueEx(hkey, pszValue, 0, dwType, (BYTE *)pData, dwBytes);
        RegCloseKey(hkey);
    }
    
    return ERROR_SUCCESS == lResult;
}


void MakeWindowsRootPath(LPSTR pszBuffer)
{
    LPSTR pszEnd = NULL;
    if (*pszBuffer == '\\' && *(pszBuffer+1) == '\\') {
        pszEnd = pszBuffer + 2;
        while (*pszEnd && (*pszEnd != '\\'))
            pszEnd++;
        if (*pszEnd) {
            pszEnd++;
            while (*pszEnd && (*pszEnd != '\\'))
                pszEnd++;
            if (*pszEnd)
                pszEnd++;
        }
    }
    else {
        LPSTR pszNext = CharNext(pszBuffer);
        if (*pszNext == ':' && *(pszNext+1) == '\\')
            pszEnd = pszNext + 2;
    }
    if (pszEnd != NULL)
        *pszEnd = '\0';
    else {
        /* ??? Windows dir is neither UNC nor a root path?
         * Just make sure it ends in a backslash.
         */
        LPSTR pszLast = pszBuffer;
        if (*pszBuffer) {
             pszLast = CharPrev(pszBuffer, pszBuffer + lstrlen(pszBuffer));
        }
        if (*pszLast != '\\')
            lstrcat(pszLast, "\\");
    }
}

//----------------------------------------------------------------------------
// InitShellFolders
//
// More of making loadwc.exe a browser-only mode catch-all.  This code makes
// sure that the Shell Folders key in the per-user registry is fully populated
// with the absolute paths to all the special folders for IE, even if
// shell32.dll doesn't understand all of them.
//
// As the shell would do in SHGetSpecialFolderLocation, we check the
// User Shell Folders key for a path, and if that's present we copy it to the
// Shell Folders key, expanding %USERPROFILE% if necessary.  If the value is
// not present under User Shell Folders, we generate the default location
// (usually under the Windows directory) and store that location under
// Shell Folders.
//----------------------------------------------------------------------------
struct FolderDescriptor {
    UINT idsDirName;        /* Resource ID for directory name */
    LPCTSTR pszRegValue;    /* Name of reg value to set path in */
    BOOL fDefaultInRoot : 1;    /* TRUE if default location is root directory */
    BOOL fWriteToUSF : 1;       /* TRUE if we should write to User Shell Folders to work around Win95 bug */
} aFolders[] = {
    { IDS_CSIDL_PERSONAL_L, TEXT("Personal"), TRUE, TRUE } ,
    { IDS_CSIDL_FAVORITES_L, TEXT("Favorites"), FALSE, TRUE },
    { IDS_CSIDL_APPDATA_L, TEXT("AppData"), FALSE, FALSE },
    { IDS_CSIDL_CACHE_L, TEXT("Cache"), FALSE, FALSE },
    { IDS_CSIDL_COOKIES_L, TEXT("Cookies"), FALSE, FALSE },
    { IDS_CSIDL_HISTORY_L, TEXT("History"), FALSE, FALSE },
};

void InitShellFolders(void)
{
    LONG err;
    HKEY hkeySF = NULL;
    HKEY hkeyUSF = NULL;
    TCHAR szDefaultDir[MAX_PATH];
    TCHAR szRootDir[MAX_PATH+1]; // possible extra '\'
    LPSTR pszPathEnd;
    LPSTR pszRootEnd;

    /* Get the windows directory and simulate PathAddBackslash (which we
     * can't get out of shlwapi, because loadwc.exe also needs to be able
     * to load after IE has been uninstalled and shlwapi deleted).
     *
     * Also build the root directory of the drive that the Windows directory
     * is on, so we can put My Documents there if necessary.
     */
    GetWindowsDirectory(szDefaultDir, ARRAYSIZE(szDefaultDir));
    lstrcpy(szRootDir, szDefaultDir);
    MakeWindowsRootPath(szRootDir);
    pszRootEnd = szRootDir + lstrlen(szRootDir);

    pszPathEnd = CharPrev(szDefaultDir, szDefaultDir + lstrlen(szDefaultDir));
    if (*pszPathEnd != '\\') {
        pszPathEnd = CharNext(pszPathEnd);
        *(pszPathEnd++) = '\\';
    }
    // pszPathEnd now points to where we can append the relative path
    UINT cchPathSpace = ARRAYSIZE(szDefaultDir) - (UINT)(pszPathEnd - szDefaultDir);
    UINT cchRootSpace = ARRAYSIZE(szRootDir) - (UINT)(pszRootEnd - szRootDir);

    err = RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",
                       0, KEY_QUERY_VALUE | KEY_SET_VALUE, &hkeySF);
    if (err == ERROR_SUCCESS) {
        err = RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders",
                           0, KEY_QUERY_VALUE, &hkeyUSF);
        if (err == ERROR_SUCCESS) {
            for (UINT i=0; i<ARRAYSIZE(aFolders); i++) {
                TCHAR szRawFolderName[MAX_PATH];
                TCHAR szExpandedFolderName[MAX_PATH];
                DWORD dwType;
                DWORD cbData = sizeof(szRawFolderName);
                LPSTR pszPath;

                err = RegQueryValueEx(hkeyUSF, aFolders[i].pszRegValue,
                                      NULL, &dwType, (LPBYTE)szRawFolderName,
                                      &cbData);
                if (err == ERROR_SUCCESS) {
                    if ((dwType != REG_EXPAND_SZ && dwType != REG_SZ) ||
                        !ExpandEnvironmentStrings(szRawFolderName,
                                                  szExpandedFolderName,
                                                  ARRAYSIZE(szExpandedFolderName)))
                    {
                        continue;
                    }
                    pszPath = szExpandedFolderName;
                }
                else {
                    if (!MLLoadString(aFolders[i].idsDirName,
                                      aFolders[i].fDefaultInRoot ? pszRootEnd : pszPathEnd,
                                      aFolders[i].fDefaultInRoot ? cchRootSpace : cchPathSpace)) {
                        continue;
                    }
                    if (aFolders[i].fDefaultInRoot)
                        pszPath = szRootDir;
                    else
                        pszPath = szDefaultDir;

                    if (GetFileAttributes(pszPath) == 0xffffffff)
                        CreateDirectory(pszPath, NULL);

                    /* The Win95 shell has a bug where for some shell folders,
                     * if there isn't a path recorded under User Shell Folders,
                     * the shell folder is assumed not to exist.  So, for those
                     * folders only, we also write the default path to USF.
                     * We do not do this generically because no value under
                     * USF is supposed to mean "use the one in the Windows
                     * directory", whereas an absolute path means "use that
                     * path";  if there's a path under USF, it will be used
                     * literally, which is a problem if the folder is set up
                     * to use the shared folder location but roams to a machine
                     * with Windows installed in a different directory.
                     */
                    if (aFolders[i].fWriteToUSF) {
                        RegSetValueEx(hkeyUSF, aFolders[i].pszRegValue, 0, REG_SZ,
                                      (LPBYTE)pszPath, lstrlen(pszPath)+1);
                    }
                }
                RegSetValueEx(hkeySF, aFolders[i].pszRegValue, 0, REG_SZ,
                              (LPBYTE)pszPath, lstrlen(pszPath)+1);
            }

            RegCloseKey(hkeyUSF);
        }
        RegCloseKey(hkeySF);
    }
}


/* Function to determine whether we're in integrated-shell mode or browser-only
 * mode.  The method for doing this (looking for DllGetVersion exported from
 * shell32.dll) is taken from shdocvw.  We don't actually call that entrypoint,
 * we just look for it.
 */
BOOL IsIntegratedShellMode()
{
    FARPROC pfnDllGetVersion = NULL;
    HMODULE hmodShell = LoadLibrary("shell32.dll");
    if (hmodShell != NULL) {
        pfnDllGetVersion = GetProcAddress(hmodShell, "DllGetVersion");
        FreeLibrary(hmodShell);
    }

    return (pfnDllGetVersion != NULL);
}

//
// Convert the string to a DWORD.
//
DWORD StringToDW(LPCTSTR psz)
{
    DWORD dwRet = 0;

    while (*psz >= TEXT('0') && *psz <= TEXT('9'))
    {
        dwRet = dwRet * 10 + *psz - TEXT('0');
        *psz++;
    }

    return dwRet;
}

//
// Is the version string from IE4
//

BOOL IsVersionIE4(LPCTSTR pszVersion)
{
    BOOL fRet = FALSE;

    //
    // IE3.0 is 4.70  and Ie4.0x is >= 4.71.1218.xxxx 
    //

    if (pszVersion[0] == TEXT('4') && pszVersion[1] == TEXT('.'))
    {
        DWORD dw = StringToDW(pszVersion+2);
        
        if (dw > 71 || (dw == 71 && pszVersion[4] == TEXT('.') &&
                        StringToDW(pszVersion+5) >= 1218))
        {
            fRet = TRUE;
        }
    }

    return fRet;
}

//
// Determine if this is an IE4 upgrade.
//
BOOL IsIE4Upgrade()
{
    BOOL fRet = FALSE;

    TCHAR szVersion[MAX_PATH];

    if (ReadRegValue(HKEY_LOCAL_MACHINE,
                     TEXT("Software\\Microsoft\\IE Setup\\Setup"),
                     TEXT("PreviousIESysFile"), (void *)szVersion,
                     sizeof(szVersion)))
    {
        fRet = IsVersionIE4(szVersion);
    }

    return fRet;
}

//
// This is an IE5 or later function.  If the user's machine is running IE4 they
// have uninstalled back to IE4.
//
BOOL IsUninstallToIE4()
{
    BOOL fRet = FALSE;

    TCHAR szVersion[MAX_PATH];

    if (ReadRegValue(HKEY_LOCAL_MACHINE,
                     TEXT("Software\\Microsoft\\Internet Explorer"),
                     TEXT("Version"), (void *)szVersion,
                     sizeof(szVersion)))
    {
        fRet = IsVersionIE4(szVersion);
    }

    return fRet;
}

/* Function to launch miscellaneous applications for browser only mode.
 * We run IEXPLORE.EXE -channelband, looking in the registry for the path
 * to IEXPLORE, and WELCOME.EXE /f, located in the same directory.
 */

#ifdef ENABLE_CHANNELS
const TCHAR c_szChanBarRegPath[] = TEXT("Software\\Microsoft\\Internet Explorer\\Main");
const TCHAR c_szChanBarKey[] = TEXT("Show_ChannelBand");
#endif

void LaunchBrowserOnlyApps()
{
    TCHAR szPath[MAX_PATH];

    /* Don't launch any of these guys if this is a "redist mode" install,
     * i.e. if a game or something installed browser components silently
     * without the user really realizing it's there.
     */
    if (ReadRegValue(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\IE Setup\\Setup",
                     "InstallMode", (void *)szPath, sizeof(szPath))
        && !lstrcmp(szPath, "R")) {
        return;
    }

    LPTSTR pszPathEnd;
    LONG cbPath = sizeof(szPath);

    /* Get the default value from the App Paths\IEXPLORE.EXE reg key, which
     * is the absolute path to the EXE.
     */
    if (RegQueryValue(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\IEXPLORE.EXE",
                      szPath, &cbPath) != ERROR_SUCCESS) {
        pszPathEnd = szPath;
        lstrcpy(szPath, "IEXPLORE.EXE");    /* can't get from reg, hope it's on the path */
    }
    else {
        /* Find the last backslash in the path.  This is a manual
         * version of strrchr(szPath, '\\').
         */
        LPTSTR pszLastBackslash = NULL;
        for (pszPathEnd = szPath; *pszPathEnd; pszPathEnd = CharNext(pszPathEnd)) {
            if (*pszPathEnd == '\\')
                pszLastBackslash = pszPathEnd;
        }
        if (pszLastBackslash == NULL)
            pszPathEnd = szPath;
        else
            pszPathEnd = pszLastBackslash + 1;      /* point after last backslash */
    }

#ifdef ENABLE_CHANNELS

    /* Don't launch the channel band app if the user doesn't want it.
     * They want it if the reg value is missing, or if it's "yes".
     * On WinNT, we default to "no" for browser-only installs.
     */
    TCHAR szValue[20];
    BOOL  fShowChannelBand=FALSE;

    if (ReadRegValue(HKEY_CURRENT_USER, c_szChanBarRegPath, 
                      c_szChanBarKey, (void *)szValue, sizeof(szValue)))
    {
        if (!lstrcmpi(szValue, "yes"))
        {
            fShowChannelBand=TRUE;
        }
    }
    //
    // In general, don't auto launch the channel bar post IE4.
    //
    // Exception:  Show the channelband if there is no Show_ChannelBand key and
    // the user upgraded over IE4 and this is W95 or W98.  This is required
    // because IE4 would launch a channel bar in this scenario w/o writting the
    // Show_ChannelBand key.  We don't want to turn of the channel bar for these
    // users.
    //
    // Another exception:  Loadwc doesn't get uninstalled when IE is
    // uninstalled.  If the user uninstalls IE5+ and goes back to IE4 we want
    // this version of loadwc to revert to IE4 loadwc behavior.
    //
    else
    {
        OSVERSIONINFO vi;
        vi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        GetVersionEx(&vi);
    
        if (vi.dwPlatformId != VER_PLATFORM_WIN32_NT &&
            (IsIE4Upgrade() || IsUninstallToIE4()))
        {
            fShowChannelBand=TRUE;

            //
            // Set the registry key so this code only runs once and upgrades to
            // IE5 don't have to worry about this scenario.
            //

            WriteRegValue(HKEY_CURRENT_USER, c_szChanBarRegPath, c_szChanBarKey,
                          REG_SZ, TEXT("yes"), sizeof(TEXT("yes")));
        }
        else
        {
            WriteRegValue(HKEY_CURRENT_USER, c_szChanBarRegPath, c_szChanBarKey,
                          REG_SZ, TEXT("no"), sizeof(TEXT("no")));
        }
    }

    if (fShowChannelBand)
    {
        int cLen = lstrlen(szPath);
        lstrcpyn(szPath + cLen, " -channelband", ARRAYSIZE(szPath) - cLen);
        WinExec(szPath, SW_SHOWNORMAL);
    }
#endif

    /* Check the registry to see if the welcome app should be launched.  Again,
     * only launch if value is missing or positive (non-zero dword, this time).
     */
    DWORD dwShow = 0;
    if (!ReadRegValue(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Tips",
                      "ShowIE4", (void *)&dwShow, sizeof(dwShow))
        || dwShow) {
        lstrcpyn(pszPathEnd, "WELCOME.EXE /f", ARRAYSIZE(szPath) - (UINT)(pszPathEnd - szPath));
        WinExec(szPath, SW_SHOWNORMAL);
    }
}


//----------------------------------------------------------------------------
// WinMain
//----------------------------------------------------------------------------
int WINAPI WinMainT(
    HINSTANCE hInstance,            // handle to current instance
    HINSTANCE hPrevInstance,        // handle to previous instance
    LPSTR lpCmdLine,                // pointer to command line
    int nCmdShow                        // show state of window
   )
{
    HWND hwndOtherInstance;

    // Save the globals
    g.hInstance = hInstance;
    g.nCmdShow = SW_HIDE;
    g.fUninstallOnly = FALSE;
    g.fStartSensLce = FALSE;

    g.fIntShellMode = IsIntegratedShellMode();
    g.hWebcheck = NULL;

    MLLoadResources(g.hInstance, TEXT("loadwclc.dll"));

    // Parse the command line, for DEBUG options and for uninstall-only switch.
    // Now also sets fStartSensLce
    if (!bParseCommandLine(lpCmdLine, nCmdShow))
        return 0;

    // look for webcheck window.  This is ultimately the guy we need to
    // find to load sens/lce late in the game.
    hwndOtherInstance = FindWindow(c_szWebCheckWindow, c_szWebCheckWindow);

    if(NULL == hwndOtherInstance)
    {
        // can't find webcheck, look for loadwc.  If we find him but not
        // webcheck, we either don't have the MOP or we're still in the 15
        // second delay.  Send the messages to loadwc and he'll take care
        // of it.
        hwndOtherInstance = FindWindow(c_szClassName, c_szClassName);
    }

    if(hwndOtherInstance)
    {
        // an instance is already running.  Tell it about Sens/LCE loading
        // requirements and bail out
        if(g.fStartSensLce)
        {
            PostMessage(hwndOtherInstance, WM_START_SENSLCE, 0, 0);
        }
        return 0;
    }

    // Set up the absolute paths for all the shell folders we care about,
    // in case we're in browser-only mode and the shell doesn't support
    // the new ones.
    if (!g.fUninstallOnly)
        InitShellFolders();

    // Run all install/uninstall stubs for browser-only mode.
    // If IE4 has been uninstalled, we'll be run with the -u switch; this
    // means to run install/uninstall stubs only, no webcheck stuff.
    if (!g.fIntShellMode) {
        RunInstallUninstallStubs2(NULL);
    }

    if (g.fUninstallOnly)
        return 0;

    // Launch the channel bar and welcome apps in browser-only mode.
    if (!g.fIntShellMode) {
        LaunchBrowserOnlyApps();
    }

    // Register the window class for the main window.
    WNDCLASS wc; 
    if (!hPrevInstance)
    { 
        wc.style            = 0; 
        wc.lpfnWndProc      = (WNDPROC) WndProc; 
        wc.cbClsExtra       = 0; 
        wc.cbWndExtra       = 0; 
        wc.hInstance        = hInstance; 
        wc.hIcon            = NULL;
        wc.hCursor          = NULL;
        wc.hbrBackground    = (HBRUSH) (COLOR_WINDOW + 1);
        wc.lpszMenuName     = NULL;
        wc.lpszClassName    = c_szClassName;
 
        if (!RegisterClass(&wc)) 
            return 0; 
    }
 
    // Create the main window.
    g.hwnd = CreateWindow(c_szClassName,
                            c_szClassName,
                            WS_OVERLAPPEDWINDOW,
                            CW_USEDEFAULT,
                            CW_USEDEFAULT,
                            CW_USEDEFAULT,
                            CW_USEDEFAULT,
                            (HWND) NULL,
                            (HMENU) NULL,
                            hInstance,
                            (LPVOID) NULL);
    if (!g.hwnd) 
        return 0; 
 
    // Show the window and paint its contents.
    ShowWindow(g.hwnd, g.nCmdShow); 
 
    // Start the message loop
    MSG msg; 
    while (GetMessage(&msg, (HWND) NULL, 0, 0)) { 
        TranslateMessage(&msg); 
        DispatchMessage(&msg); 
    }

    MLFreeResources(g.hInstance);

    // Return the exit code to Windows
    return (int)msg.wParam;
} 

//----------------------------------------------------------------------------
// WndProc
//----------------------------------------------------------------------------
LRESULT APIENTRY WndProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{ 
    switch (uMsg) {
        case WM_CREATE: 
            DWORD   dwTime;
            if(!ReadRegValue(HKEY_CURRENT_USER, c_szWebcheckKey, "DelayLoad", &dwTime, sizeof(DWORD)))
                dwTime = 15;
            SetTimer(hwnd, 1, 1000 * dwTime, NULL);
            break;
            
        case WM_START_SENSLCE:
            g.fStartSensLce = TRUE;
            break;

        case WM_TIMER:
            KillTimer(hwnd, 1);
            vLoadWebCheck();
            return 0; 

        case WM_ENDSESSION:
            if (!wParam)    // if not fEndSession, bail
                break;
            // else fall through to WM_DESTROY
 
        case WM_DESTROY:
            vUnloadWebCheck();
            PostQuitMessage(0);
            return 0; 
 
        default: 
            break;
    } 
    return DefWindowProc(hwnd, uMsg, wParam, lParam); 
} 

//----------------------------------------------------------------------------
// vLoadWebCheck
//----------------------------------------------------------------------------
void vLoadWebCheck(void)
{
    if(g.hWebcheck)
        return;
        
    g.hWebcheck = LoadLibrary(TEXT("webcheck.dll"));
    if(g.hWebcheck)
    {
        PFNSTART pfn = (PFNSTART)GetProcAddress(g.hWebcheck, (LPCSTR)7);
        if(pfn)
        {
            pfn(g.fStartSensLce);
        }
        else
        {
            // clean up dll
            FreeLibrary(g.hWebcheck);
            g.hWebcheck = NULL;
        }
    }
}

//----------------------------------------------------------------------------
// vUnloadWebCheck
//----------------------------------------------------------------------------
void vUnloadWebCheck(void)
{
    if (!g.hWebcheck)
        return;

    PFNSTOP pfn = (PFNSTOP)GetProcAddress(g.hWebcheck, (LPCSTR)8);
    if(pfn)
    {
        pfn();
    }

    // [darrenmi] don't bother unloading webcheck.  We only do this in
    // response to a shut down so it's not a big deal.  On NT screen saver
    // proxy has a thread that wakes up after the call to StopService - if
    // we've unloaded the dll before then, we're toast.

    // clean up dll
    //FreeLibrary(g.hWebcheck);
    //g.hWebcheck = NULL;
}

//=--------------------------------------------------------------------------=
// StringFromGuid
// returns an ANSI string from a CLSID or GUID
//
// Parameters:
//    REFIID               - [in]  clsid to make string out of.
//    LPSTR                - [in]  buffer in which to place resultant GUID.
//
// Output:
//    int                  - number of chars written out.
//=--------------------------------------------------------------------------=
#ifdef DEBUG
int StringFromGuid(
    const CLSID*   piid,
    LPTSTR   pszBuf
    )
{
    return wsprintf(pszBuf, TEXT("{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}"), piid->Data1,
            piid->Data2, piid->Data3, piid->Data4[0], piid->Data4[1], piid->Data4[2],
            piid->Data4[3], piid->Data4[4], piid->Data4[5], piid->Data4[6], piid->Data4[7]);
}
#endif // DEBUG

//----------------------------------------------------------------------------
// bParseCmdLine
//
// Parse the command line
//      -u  run install/uninstall stubs only, then quit
//  DEBUG options:
//      -v  visible window (easy to shutdown)
//      -a  add webcheck to shell service object
//      -r  remove webcheck from shell service object
//      -s  fix shell folders only
//      -?  these options
//----------------------------------------------------------------------------
BOOL bParseCommandLine(LPSTR lpCmdLine, int nCmdShow)
{
    if (!lpCmdLine)
        return TRUE;

    CharUpper(lpCmdLine);   /* easier to parse */
    while (*lpCmdLine) {
        if (*lpCmdLine != '-' && *lpCmdLine != '/')
            break;

        lpCmdLine++;

        switch (*(lpCmdLine++)) {
        case 'E':
            // ignore 'embedding' command line
            break;
        case 'L':
        case 'M':
            g.fStartSensLce = TRUE;
            break;
        case 'U':   
            g.fUninstallOnly = TRUE;
            break;
#ifdef DEBUG
        case 'V':
            g.nCmdShow = nCmdShow;
            break;
        case 'A':
            {
                HKEY hkey;
                DWORD dwRet;
                if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_LOCAL_MACHINE, c_szShellReg, 0, 0, 0, KEY_READ | KEY_WRITE, NULL, &hkey, &dwRet))
                {
                    // NOTE: Does it matter if we set this as ANSI or Unicode?  No, stored in Unicode.
                    // NOTE: Does sizeof include the terminating null?  Yes.
                    int iRet;
                    TCHAR szCLSID[GUIDSTR_MAX];
                    iRet = StringFromGuid(&CLSID_WebCheck, szCLSID);
                    ASSERT(GUIDSTR_MAX == (iRet + 1));
            
                    if (ERROR_SUCCESS == RegSetValueEx(hkey, c_szWebCheck, 0, REG_SZ, (CONST BYTE*)szCLSID, sizeof(szCLSID)))
                    {
                        MessageBox(NULL, "Webcheck was added to the shell service object list", "LoadWC", MB_OK);
                    }
                    RegCloseKey(hkey);
                }
                return FALSE;
            }
            break;

        case 'R':
            {
                HKEY hkey;
                if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szShellReg, 0, KEY_READ | KEY_WRITE, &hkey))
                {
                    if (ERROR_SUCCESS == RegDeleteValue(hkey, c_szWebCheck))
                        MessageBox(NULL, "Webcheck was removed from the shell service object list", "LoadWC", MB_OK);
                    RegCloseKey(hkey);
                }
                return FALSE;
            }
            break;

        case 'S':
            InitShellFolders();
            return FALSE;

        case '?':
        default:
            MessageBox(NULL, "Command line options:\n-v\tvisible window\n-a\tadd webcheck as shell service\n-r\tremove webcheck as shell service\n-s\tfixup shell folders only", "LoadWC", MB_OK);
            return FALSE;
#endif
        }

        while (*lpCmdLine == ' ' || *lpCmdLine == '\t') {
            lpCmdLine++;
        }
    }

    return TRUE;
}
