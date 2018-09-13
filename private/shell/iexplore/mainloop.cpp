#include "iexplore.h"
#include "rcids.h"
#include "shlwapi.h"

#include <platform.h>

#ifdef UNIX
#include "unixstuff.h"
#endif

static const TCHAR c_szBrowseNewProcessReg[] = REGSTR_PATH_EXPLORER TEXT("\\BrowseNewProcess");
static const TCHAR c_szBrowseNewProcess[] = TEXT("BrowseNewProcess");

int WinMainT(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpszCmdLine, int nCmdShow);


STDAPI_(int) ModuleEntry(void)
{
    int i;
    STARTUPINFOA si;
#ifdef UNICODE
    LPTSTR pszCmdLine = GetCommandLine();
#else
    // for multibyte should make it unsigned
    BYTE * pszCmdLine = (BYTE *)GetCommandLine();
#endif


#if defined(UNIX) 
    // IEUNIX: On solaris we are getting out of file handles with new code pages added to mlang
    // causing more nls files to be mmapped.
    INCREASE_FILEHANDLE_LIMIT;
#endif

    //
    // We don't want the "No disk in drive X:" requesters, so we set
    // the critical error mask such that calls will just silently fail
    //

    SetErrorMode(SEM_FAILCRITICALERRORS);

    if(StopWatchMode() & SPMODE_BROWSER)  // Used to get the start of browser total download time
    {
        StopWatch_Start(SWID_BROWSER_FRAME, TEXT("Browser Frame Start"), SPMODE_BROWSER | SPMODE_DEBUGOUT);
    }
    
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
    si.cb = sizeof(si);

    GetStartupInfoA(&si);

    i = WinMainT(GetModuleHandle(NULL), NULL, (LPTSTR)pszCmdLine,
                   si.dwFlags & STARTF_USESHOWWINDOW ? si.wShowWindow : SW_SHOWDEFAULT);

#ifndef UNIX  
    ExitThread(i);  // We only come here when we are not the shell...
#else
// there seem to be some desirable side effect calling ExitThread on Windows
    ExitProcess(i); 
#endif
    return i;
}

//
// Create a unique event name
//
HANDLE AppendEvent(COPYDATASTRUCT *pcds)
{
    static DWORD dwNextID = 0;
    TCHAR szEvent[MAX_IEEVENTNAME];

    wsprintf(szEvent, "IE-%08X-%08X", GetCurrentThreadId(), dwNextID++);
    HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, szEvent);
    if (hEvent)
    {
        //
        // Put the (UNICODE) event name at the end of the cds data
        //
        LPWSTR pwszBufferEvent = (LPWSTR)(((BYTE *)pcds->lpData) + pcds->cbData);
#ifdef UNICODE
        lstrcpy(pwszBufferEvent, szEvent);
#else
        MultiByteToWideChar(CP_ACP, 0, szEvent, -1, pwszBufferEvent, ARRAYSIZE(szEvent));
#endif
        pcds->cbData += (lstrlenW(pwszBufferEvent) + 1) * sizeof(WCHAR);
    }

    return hEvent;
}

BOOL IsCommandSwitch(LPTSTR lpszCmdLine, LPTSTR pszSwitch, BOOL fRemoveSwitch)
{
    LPTSTR lpsz;
    
    if ((lpsz=StrStrI(lpszCmdLine, pszSwitch)) && (lpsz == lpszCmdLine))
    {
        int cch = lstrlen(pszSwitch);

        if (*(lpsz+cch) == 0 || *(lpsz+cch) == TEXT(' '))
        {
            while (*(lpsz+cch) == TEXT(' '))
                cch++;

            if (fRemoveSwitch)
            {
                // Remove the switch by copying everything up.
                *lpsz=0;
                lstrcat(lpsz, lpsz+cch);
            }
            return TRUE;
        }
    } 
    return FALSE;
}

BOOL CheckForNeedingAppCompatWindow(void)
{
    // Which I could simply get the Process of who spawned me.  For now
    // try hack to get the foreground window and go from there...
    TCHAR szClassName[80];
    HWND hwnd = GetForegroundWindow();

    if (hwnd && GetClassName(hwnd, szClassName, ARRAYSIZE(szClassName)) > 0)
    {
        if (lstrcmpi(szClassName, TEXT("MauiFrame")) == 0)
            return TRUE;
    }
    return FALSE;
}

//
// AppCompat - Sequel NetPIM execs a browser and then waits forever
// looking for a visible top level window owned by this process.
//
HWND CreateAppCompatWindow(HINSTANCE hinst)
{
    HWND hwnd;
    static const TCHAR c_szClass[] = TEXT("IEDummyFrame");  // IE3 used "IEFrame"

    WNDCLASS wc = { 0, DefWindowProc, 0, 0, hinst, NULL, NULL, NULL, NULL, c_szClass };
    RegisterClass(&wc);

    // Netmanage ECCO Pro asks to get the menu...
    HMENU hmenu = CreateMenu();
    hwnd = CreateWindowEx(WS_EX_TOOLWINDOW, c_szClass, TEXT(""), 0,
                          0x00007FF0, 0x00007FF0, 0, 0,
                          NULL, hmenu, hinst, NULL);
    // Don't open SHOWDEFAULT or this turkey could end up maximized
    ShowWindow(hwnd, SW_SHOWNA);

    return hwnd;
}

#define USERAGENT_POST_PLATFORM_PATH_TO_KEY    TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\User Agent\\Post Platform")
void SetCompatModeUserAgentString(void)
{
    HKEY hkey;
    const char szcompat[]=TEXT("compat");
    
    if (ERROR_SUCCESS == RegCreateKey(HKEY_CURRENT_USER, USERAGENT_POST_PLATFORM_PATH_TO_KEY, &hkey))
    {
        RegSetValueEx( hkey,
                       szcompat,
                       0,
                       REG_BINARY,
                       (LPBYTE)NULL, 0);
        RegCloseKey(hkey);
    }
    
}

// Tell the user they are running in compat mode and not all the features will be available.
#define IECOMPAT_REG_VAL    TEXT("CompatWarningFor")

void WarnCompatMode(HINSTANCE hinst)
{
    TCHAR szFqFilename[MAX_PATH];
    TCHAR szRegVal[MAX_PATH];
    TCHAR szTitle[255];
    TCHAR szMsg[1024];
    LPTSTR szFile;

    GetModuleFileName(NULL, szFqFilename, ARRAYSIZE(szFqFilename));
    szFile = PathFindFileName(szFqFilename);
    

    // Build up string "compatmodewarningfor <exe name>" as value for reg key
    lstrcpy(szRegVal, IECOMPAT_REG_VAL);
    lstrcat(szRegVal, szFile);

    LoadString(hinst, IDS_COMPATMODEWARNINGTITLE, szTitle, ARRAYSIZE(szTitle));
    LoadString(hinst, IDS_COMPATMODEWARNING, szMsg, ARRAYSIZE(szMsg));

    SHMessageBoxCheck(NULL, szMsg, szTitle, MB_OK, FALSE, szRegVal);
}

#ifdef WINNT

// this is the same code that is in explorer.exe (initcab.c)
#define RSA_PATH_TO_KEY    TEXT("Software\\Microsoft\\Cryptography\\Defaults\\Provider\\Microsoft Base Cryptographic Provider v1.0")
#define CSD_REG_PATH       TEXT("System\\CurrentControlSet\\Control\\Windows")
#define CSD_REG_VALUE      TEXT("CSDVersion")


// the signatures we are looking for in the regsitry so that we can patch up 

#ifdef _M_IX86
static  BYTE  SP3Sig[] = {0xbd, 0x9f, 0x13, 0xc5, 0x92, 0x12, 0x2b, 0x72,
                          0x4a, 0xba, 0xb6, 0x2a, 0xf9, 0xfc, 0x54, 0x46,
                          0x6f, 0xa1, 0xb4, 0xbb, 0x43, 0xa8, 0xfe, 0xf8,
                          0xa8, 0x23, 0x7d, 0xd1, 0x85, 0x84, 0x22, 0x6e,
                          0xb4, 0x58, 0x00, 0x3e, 0x0b, 0x19, 0x83, 0x88,
                          0x6a, 0x8d, 0x64, 0x02, 0xdf, 0x5f, 0x65, 0x7e,
                          0x3b, 0x4d, 0xd4, 0x10, 0x44, 0xb9, 0x46, 0x34,
                          0xf3, 0x40, 0xf4, 0xbc, 0x9f, 0x4b, 0x82, 0x1e,
                          0xcc, 0xa7, 0xd0, 0x2d, 0x22, 0xd7, 0xb1, 0xf0,
                          0x2e, 0xcd, 0x0e, 0x21, 0x52, 0xbc, 0x3e, 0x81,
                          0xb1, 0x1a, 0x86, 0x52, 0x4d, 0x3f, 0xfb, 0xa2,
                          0x9d, 0xae, 0xc6, 0x3d, 0xaa, 0x13, 0x4d, 0x18,
                          0x7c, 0xd2, 0x28, 0xce, 0x72, 0xb1, 0x26, 0x3f,
                          0xba, 0xf8, 0xa6, 0x4b, 0x01, 0xb9, 0xa4, 0x5c,
                          0x43, 0x68, 0xd3, 0x46, 0x81, 0x00, 0x7f, 0x6a,
                          0xd7, 0xd1, 0x69, 0x51, 0x47, 0x25, 0x14, 0x40,
                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
#else // other than _M_IX86
static  BYTE  SP3Sig[] = {0x8a, 0x06, 0x01, 0x6d, 0xc2, 0xb5, 0xa2, 0x66,
                          0x12, 0x1b, 0x9c, 0xe4, 0x58, 0xb1, 0xf8, 0x7d,
                          0xad, 0x17, 0xc1, 0xf9, 0x3f, 0x87, 0xe3, 0x9c,
                          0xdd, 0xeb, 0xcc, 0xa8, 0x6b, 0x62, 0xd0, 0x72,
                          0xe7, 0xf2, 0xec, 0xd6, 0xd6, 0x36, 0xab, 0x2d,
                          0x28, 0xea, 0x74, 0x07, 0x0e, 0x6c, 0x6d, 0xe1,
                          0xf8, 0x17, 0x97, 0x13, 0x8d, 0xb1, 0x8b, 0x0b,
                          0x33, 0x97, 0xc5, 0x46, 0x66, 0x96, 0xb4, 0xf7,
                          0x03, 0xc5, 0x03, 0x98, 0xf7, 0x91, 0xae, 0x9d,
                          0x00, 0x1a, 0xc6, 0x86, 0x30, 0x5c, 0xc8, 0xc7,
                          0x05, 0x47, 0xed, 0x2d, 0xc2, 0x0b, 0x61, 0x4b,
                          0xce, 0xe5, 0xb7, 0xd7, 0x27, 0x0c, 0x9e, 0x2f,
                          0xc5, 0x25, 0xe3, 0x81, 0x13, 0x9d, 0xa2, 0x67,
                          0xb2, 0x26, 0xfc, 0x99, 0x9d, 0xce, 0x0e, 0xaf,
                          0x30, 0xf3, 0x30, 0xec, 0xa3, 0x0a, 0xfe, 0x16,
                          0xb6, 0xda, 0x16, 0x90, 0x9a, 0x9a, 0x74, 0x7a,
                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
#endif      // _M_IX86

void CheckForSP3RSAOverwrite( void )
{
    // check for them having installed NTSP3 over the top of IE4, it nukes
    // the RSABASE reg stuff, so we have to re-do it. (our default platform is NT + SP3, but this
    // problem doesn't occur on NT5, so ignore it.

    OSVERSIONINFO osVer;

    ZeroMemory(&osVer, sizeof(osVer));
    osVer.dwOSVersionInfoSize = sizeof(osVer);

    if( GetVersionEx(&osVer) && (osVer.dwPlatformId == VER_PLATFORM_WIN32_NT) 
        && (osVer.dwMajorVersion == 4))
    {
        // now check to see we are on SP3 ...
        DWORD dwValue = 0;
        DWORD dwSize = sizeof( dwValue );
        
        if ( ERROR_SUCCESS == SHGetValue( HKEY_LOCAL_MACHINE, CSD_REG_PATH, CSD_REG_VALUE, NULL,
             &dwValue, &dwSize) && LOWORD( dwValue ) == 0x300 )
        {
            BYTE rgbSig[136];
            dwSize = sizeof(rgbSig);
            
            if (ERROR_SUCCESS == SHGetValue ( HKEY_LOCAL_MACHINE, RSA_PATH_TO_KEY, TEXT("Signature"), NULL,
                rgbSig, &dwSize))
            {
                if ((dwSize == sizeof(SP3Sig)) && 
                    (0 == memcmp(SP3Sig, rgbSig, sizeof(SP3Sig))))
                {
                    // need to do a DLLRegisterServer on RSABase
                    HINSTANCE hInst = LoadLibrary(TEXT("rsabase.dll"));
                    if ( hInst )
                    {
                        FARPROC pfnDllReg = GetProcAddress( hInst, "DllRegisterServer");
                        if ( pfnDllReg )
                        {
                            __try
                            {
                                pfnDllReg();
                            }
                            __except( EXCEPTION_EXECUTE_HANDLER)
                            {
                            }
                            __endexcept
                        }

                        FreeLibrary( hInst );
                    }
                }
            }
        }           
    }
}
#else
#define CheckForSP3RSAOverwrite() 
#endif


#define TEN_SECONDS (10 * 1000)

//---------------------------------------------------------------------------
int WinMainT(HINSTANCE hinst, HINSTANCE hPrevInstance, LPTSTR lpszCmdLine, int nCmdShow)
{
#ifdef DEBUG
    CcshellGetDebugFlags();
#endif
    int  iRet = TRUE;
    HWND hwndDesktop ;
    BOOL fNowait = FALSE;
    BOOL fInproc = FALSE;
    BOOL fEval   = FALSE;
#ifdef UNIX
    BOOL fRemote = FALSE;
#endif

    while (1) {
#ifdef UNIX
        if (IsCommandSwitch(lpszCmdLine, TEXT("-remote"), TRUE))
        {
            fRemote = TRUE;
        }
#endif
        if (IsCommandSwitch(lpszCmdLine, TEXT("-eval"), TRUE))
        {
            fInproc = TRUE;
            fEval   = TRUE;
        } else if (IsCommandSwitch(lpszCmdLine, TEXT("-new"), TRUE))
        {
            fInproc = TRUE;
        }
        else if (IsCommandSwitch(lpszCmdLine, TEXT("-nowait"), TRUE)) 
        {
            fNowait = TRUE;
        } 
        else
            break;
    }

#ifndef UNIX
    if (!GetModuleHandle(TEXT("IEXPLORE.EXE")))
    {
        // For side by side install auto dection, if IExplore.exe is renamed, assume this is a side by side do dah
        // and we want to run in "evaluation" mode.
        fInproc = TRUE;
        fEval   = TRUE;        
    }
#endif

    
    // Should we run browser in a new process?
    if (fInproc || SHRegGetBoolUSValue(c_szBrowseNewProcessReg, c_szBrowseNewProcess, FALSE, FALSE))
    {
        goto InThisProcess;
    }

#ifdef UNIX
    if (!(fRemote && ConnectRemoteIE(lpszCmdLine, hinst)))
#endif
    
    if (WhichPlatform() == PLATFORM_INTEGRATED && (hwndDesktop = GetShellWindow()))
    {
        //
        // Integrated browser mode - package up a bunch of data into a COPYDATASTRUCT,
        // and send it to the desktop window via SendMessage(WM_COPYDATA).
        //
        COPYDATASTRUCT cds;
        cds.dwData = nCmdShow;

        //
        // First piece of data is a wide string version of the command line params.
        //
        LPWSTR pwszBuffer = (LPWSTR)LocalAlloc(LPTR, (INTERNET_MAX_URL_LENGTH + 2 * MAX_IEEVENTNAME) * sizeof(WCHAR));;
        if (pwszBuffer)
        {
#ifdef UNICODE
            lstrcpy(pwszBuffer, lpszCmdLine);
#else
            int cch = MultiByteToWideChar(CP_ACP, 0, lpszCmdLine, -1, pwszBuffer, INTERNET_MAX_URL_LENGTH);
            Assert(cch);
#endif
            cds.lpData = pwszBuffer;
            cds.cbData = sizeof(WCHAR) * (lstrlenW((LPCWSTR)cds.lpData) + 1);

            //
            // Second piece of data is the event to fire when
            // the browser window reaches WM_CREATE.
            //
            HANDLE hEventReady = AppendEvent(&cds);
            if (hEventReady)
            {
                //
                // Third piece of data is the event to fire when
                // the browser window closes.  This is optional,
                // we only create it (and wait for it) when there
                // are command line parameters.
                //
                HANDLE hEventDead = NULL;

                // The hard part is to figure out when we need the command line and when
                // we don't. For the most part if there is a command line we will assume that
                // we will need it (potentially) we could look for the -nowait flag.  But then
                // there are others like NetManage ECCO Pro who do their equiv of ShellExecute
                // who don't pass a command line...

                if ((*lpszCmdLine || CheckForNeedingAppCompatWindow()) && !fNowait)
                {
                    hEventDead = AppendEvent(&cds);
                }
                
                if (hEventDead || !*lpszCmdLine || fNowait)
                {
                    //
                    // Send that message!
                    //
                    int iRet = (int)SendMessage(hwndDesktop, WM_COPYDATA, (WPARAM)hwndDesktop, (LPARAM)&cds);

                    //
                    // Nobody needs the string anymore.
                    //
                    LocalFree(pwszBuffer);
                    pwszBuffer = NULL;

                    if (iRet)
                    {
                        //
                        // First, we wait for the browser window to hit WM_CREATE.
                        // When this happens, all DDE servers will have been registered.
                        //
                        DWORD dwRet = WaitForSingleObject(hEventReady, TEN_SECONDS);
                        ASSERT(dwRet == WAIT_OBJECT_0);

                        if (hEventDead)
                        {
                            //
                            // Create an offscreen IE-lookalike window
                            // owned by this process for app compat reasons.
                            //
                            HWND hwnd = CreateAppCompatWindow(hinst);

                            do
                            {
                                //
                                // Calling MsgWait... will cause any threads blocked
                                // on WaitForInputIdle(IEXPLORE) to resume execution.
                                // This is fine because the browser has already
                                // registered its DDE servers by now.
                                //
                                dwRet = MsgWaitForMultipleObjects(1, &hEventDead, FALSE, INFINITE, QS_ALLINPUT);

                                if (dwRet == WAIT_OBJECT_0)
                                {
                                    //
                                    // Kill our helper window cleanly too.
                                    //
                                    DestroyWindow(hwnd);
                                }

                                MSG msg;
                                while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
                                {
                                    if (msg.message == WM_QUIT)
                                    {
                                        //
                                        // We got a quit message, drop out.
                                        //
                                        dwRet = WAIT_OBJECT_0;
                                        break;
                                    }

                                    TranslateMessage(&msg);
                                    DispatchMessage(&msg);
                                }
                            }
                            while(dwRet != WAIT_OBJECT_0);
                        }
                    }

                    iRet = !iRet;
                }
                if (hEventDead)
                {
                    CloseHandle(hEventDead);
                }

                CloseHandle(hEventReady);
            }
        }
        if (pwszBuffer)
        {
            LocalFree(pwszBuffer);
            pwszBuffer = NULL;
        }
    }        
    else
    {

InThisProcess:
        // Browser only mode, check the SP3 bug
        CheckForSP3RSAOverwrite();

        if (fEval)
        {
            // Set "compat" mode user agent
            WarnCompatMode(hinst);

            // #75454... let the compat mode setup set useragent in HKLM.
            //SetCompatModeUserAgentString();
            
            // Run in eval mode. So we want everything from this dir.
            LoadLibrary("comctl32.DLL");
            LoadLibrary("browseui.DLL");            
            LoadLibrary("shdocvw.DLL");
            LoadLibrary("wininet.dll");
            LoadLibrary("urlmon.dll");
            LoadLibrary("mlang.dll");
            LoadLibrary("mshtml.dll");
            LoadLibrary("jscript.DLL");            
        }
        
        iRet = IEWinMain(lpszCmdLine, nCmdShow);
    }

    return iRet;
}

// DllGetLCID
//
// this API is for Office to retrieve our UI language
// when they are hosted by iexplore.
//
STDAPI_(LCID) DllGetLCID (IBindCtx * pbc)
{
     return MLGetUILanguage();
}

