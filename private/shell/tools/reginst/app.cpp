
#include "priv.h"       

const TCHAR c_szAppName[]   = TEXT("reginst");
const TCHAR c_szDllReg[]    = TEXT("DllRegisterServer");
const TCHAR c_szDllUnreg[]  = TEXT("DllUnregisterServer");

typedef HRESULT (CALLBACK* DLLREGPROC)(void);
typedef HRESULT (CALLBACK* DLLINSTALLPROC)(BOOL bInstall, LPCWSTR pszCmdLine);

HINSTANCE g_hinst;

#define RIF_QUIET       0x00000001
#define RIF_UNINSTALL   0x00000002
#define RIF_INSTALLONLY 0x00000004
#define RIF_REGONLY     0x00000008
#define RIF_HELP        0x00000010

// Don't link to shlwapi.dll so this is a stand-alone tool

/*----------------------------------------------------------
Purpose: If a path is contained in quotes then remove them.

Returns: --
Cond:    --
*/
void
PathUnquoteSpaces(
    LPTSTR lpsz)
{
    int cch;

    cch = lstrlen(lpsz);

    // Are the first and last chars quotes?
    if (lpsz[0] == TEXT('"') && lpsz[cch-1] == TEXT('"'))
    {
        // Yep, remove them.
        lpsz[cch-1] = TEXT('\0');
        hmemcpy(lpsz, lpsz+1, (cch-1) * SIZEOF(TCHAR));
    }
}


// returns a pointer to the arguments in a cmd type path or pointer to
// NULL if no args exist
//
// "foo.exe bar.txt"    -> "bar.txt"
// "foo.exe"            -> ""
//
// Spaces in filenames must be quoted.
// " "A long name.txt" bar.txt " -> "bar.txt"

LPTSTR
PathGetArgs(
    LPCTSTR pszPath)
{
    BOOL fInQuotes = FALSE;

    if (!pszPath)
            return NULL;

    while (*pszPath)
    {
        if (*pszPath == TEXT('"'))
            fInQuotes = !fInQuotes;
        else if (!fInQuotes && *pszPath == TEXT(' '))
            return (LPTSTR)pszPath+1;
        pszPath = CharNext(pszPath);
    }

    return (LPTSTR)pszPath;
}


__inline BOOL ChrCmpA_inline(WORD w1, WORD wMatch)
{
    /* Most of the time this won't match, so test it first for speed.
    */
    if (LOBYTE(w1) == LOBYTE(wMatch))
    {
        if (IsDBCSLeadByte(LOBYTE(w1)))
        {
            return(w1 != wMatch);
        }
        return FALSE;
    }
    return TRUE;
}


LPSTR FAR PASCAL StrChrA(LPCSTR lpStart, WORD wMatch)
{
    for ( ; *lpStart; lpStart = AnsiNext(lpStart))
    {
        if (!ChrCmpA_inline(*(UNALIGNED WORD FAR *)lpStart, wMatch))
            return((LPSTR)lpStart);
    }
    return (NULL);
}

BOOL
StrTrim(
    IN OUT LPSTR  pszTrimMe,
    IN     LPCSTR pszTrimChars)
    {
    BOOL bRet = FALSE;
    LPSTR psz;
    LPSTR pszStartMeat;
    LPSTR pszMark = NULL;

    ASSERT(IS_VALID_STRING_PTRA(pszTrimMe, -1));
    ASSERT(IS_VALID_STRING_PTRA(pszTrimChars, -1));

    if (pszTrimMe)
    {
        /* Trim leading characters. */

        psz = pszTrimMe;

        while (*psz && StrChrA(pszTrimChars, *psz))
            psz = CharNextA(psz);

        pszStartMeat = psz;

        /* Trim trailing characters. */

        // (The old algorithm used to start from the end and go
        // backwards, but that is piggy because DBCS version of
        // CharPrev iterates from the beginning of the string
        // on every call.)

        while (*psz)
            {
            if (StrChrA(pszTrimChars, *psz))
                {
                pszMark = psz;
                }
            else
                {
                pszMark = NULL;
                }
            psz = CharNextA(psz);
            }

        // Any trailing characters to clip?
        if (pszMark)
            {
            // Yes
            *pszMark = '\0';
            bRet = TRUE;
            }

        /* Relocate stripped string. */

        if (pszStartMeat > pszTrimMe)
        {
            /* (+ 1) for null terminator. */
            MoveMemory(pszTrimMe, pszStartMeat, CbFromCchA(lstrlenA(pszStartMeat) + 1));
            bRet = TRUE;
        }
        else
            ASSERT(pszStartMeat == pszTrimMe);

        ASSERT(IS_VALID_STRING_PTRA(pszTrimMe, -1));
    }

    return bRet;
    }


HRESULT ParseOption(LPCTSTR * ppsz, LPDWORD pdwFlags)
{
    HRESULT hres = S_FALSE;
    LPCTSTR psz = *ppsz;

    // Skip any leading whitespace
    while (*psz && (' ' == *psz || '\t' == *psz))
        psz++;

    if ('/' == *psz || '-' == *psz)
    {
        hres = S_OK;

        psz++;
        switch (*psz)
        {
        case '?':
            *pdwFlags |= RIF_HELP;
            break;

        case 'q':
            *pdwFlags |= RIF_QUIET;
            break;

        case 'u':
            *pdwFlags |= RIF_UNINSTALL;
            break;

        case 'i':
            *pdwFlags |= RIF_INSTALLONLY;
            break;

        case 'r':
            *pdwFlags |= RIF_REGONLY;
            break;

        default:
            hres = E_FAIL;
            break;
        }

        // Return the new position in the string
        *ppsz = psz+1;
    }
    else
    {
        *ppsz = psz;
    }

    return hres;
}   


HRESULT CallInstall(UINT * pids, DLLREGPROC pfnRegSvr, DLLINSTALLPROC pfnInstall, BOOL bInstall, LPCWSTR psz)
{
    HRESULT hres = S_OK;

    ASSERT(NULL == pfnRegSvr || IS_VALID_CODE_PTR(pfnRegSvr, DLLREGPROC));
    ASSERT(NULL == pfnInstall || IS_VALID_CODE_PTR(pfnInstall, DLLINSTALLPROC));
    ASSERT(IS_VALID_STRING_PTRW(psz, -1));

    _try
    {
        if (pfnRegSvr)
        {
            hres = pfnRegSvr();
            
            if (FAILED(hres))
                *pids = IDS_INSTALLFAILED;
            else
                *pids = IDS_REGSUCCESS;
        }

        if (SUCCEEDED(hres) && pfnInstall)
        {
            hres = pfnInstall(bInstall, psz);
            
            if (FAILED(hres))
                *pids = IDS_INSTALLFAILED;
            else
                *pids = IDS_INSTALLSUCCESS;
            
        }
    }
    _except (EXCEPTION_EXECUTE_HANDLER)
    {
        hres = E_UNEXPECTED;
        *pids = IDS_UNEXPECTED;
    }

    return hres;
}    


/*----------------------------------------------------------
Purpose: Worker function to do the work

         reginst /q /u /i /r foo.dll <cmdline>

            /q  Quiet
            /u  Uninstall

         Calls both DllInstall and DllRegisterServer unless:

            /i  Call DllInstall only
            /r  Call DllRegisterServer/DllUnregisterServer only

         <cmdline> is passed to DllInstall if it exists.

*/
HRESULT
DoWork(HWND hwnd, LPCTSTR pszCmdLine)
{
    TCHAR szDll[MAX_PATH];
    WCHAR wszArgs[MAX_PATH];
    LPCTSTR psz;
    LPCTSTR pszArgs;
    DWORD dwFlags = 0;
    HRESULT hres;
    UINT ids = 0;
    LPCTSTR pszFnError = NULL;


    // Options come first
    psz = PathGetArgs(pszCmdLine);

    while (S_OK == (hres = ParseOption(&psz, &dwFlags)))
        ;  // Loop thru options

    if (dwFlags & RIF_HELP)
    {
        ids = IDS_HELP;
        hres = S_OK;
    }
    else
    {
        // Now psz should point at DLL or null terminator
        lstrcpyn(szDll, psz, SIZECHARS(szDll));

        // Strip off args from the dll name
        LPTSTR pszT = PathGetArgs(szDll);
        if (*pszT)
            *pszT = 0;
        StrTrim(szDll, " \t");
        PathUnquoteSpaces(szDll);

        // Get args to pass to DllInstall
        pszArgs = PathGetArgs(psz);
        MultiByteToWideChar(CP_ACP, 0, pszArgs, -1, wszArgs, SIZECHARS(wszArgs));

        HINSTANCE hinst = LoadLibrary(szDll);
        if (hinst)
        {
            DLLREGPROC pfnRegSvr = NULL;
            DLLINSTALLPROC pfnInstall = NULL;
            
            hres = S_OK;

            if (IsFlagClear(dwFlags, RIF_INSTALLONLY))
            {
                if (dwFlags & RIF_UNINSTALL)
                {
                    pfnRegSvr = (DLLREGPROC)GetProcAddress(hinst, "DllUnregisterServer");
                    pszFnError = c_szDllUnreg;
                }
                else
                {
                    pfnRegSvr = (DLLREGPROC)GetProcAddress(hinst, "DllRegisterServer");
                    pszFnError = c_szDllReg;
                }
            }

            if (IsFlagClear(dwFlags, RIF_REGONLY))
            {
                pfnInstall = (DLLINSTALLPROC)GetProcAddress(hinst, "DllInstall");
                pszFnError = TEXT("DllInstall");
            }

            if (NULL == pfnInstall && NULL == pfnRegSvr)
            {
                ids = IDS_FAILED;
                hres = E_FAIL;
            }
            else
            {
                hres = CallInstall(&ids, pfnRegSvr, pfnInstall, IsFlagClear(dwFlags, RIF_UNINSTALL), wszArgs);

                if (SUCCEEDED(hres))
                {
                    if (pfnRegSvr && pfnInstall)
                        ids = IDS_FULLSUCCESS;
                    
                    if (IsFlagSet(dwFlags, RIF_UNINSTALL))
                    {
                        // refer to "uninstall" msgs instead
                        ids += (IDS_UNREGSUCCESS - IDS_REGSUCCESS);   
                    }
                }
            }

            FreeLibrary(hinst);
        }
        else 
        {
            ids = IDS_LOADFAILED;
            hres = E_FAIL;
        }
    }


    if (0 != ids && IsFlagClear(dwFlags, RIF_QUIET))
    {
        TCHAR szFmt[512];
        TCHAR szMsg[1024];
        TCHAR szT[32];
        LPCTSTR rgpsz[2];
        UINT uFlags = MB_OK;

        rgpsz[0] = szDll;
        rgpsz[1] = pszFnError;

        LoadString(g_hinst, IDS_TITLE, szT, SIZECHARS(szT));
        LoadString(g_hinst, ids, szFmt, SIZECHARS(szFmt));

        FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY, 
                      szFmt, 0, 0, szMsg, SIZECHARS(szMsg), (va_list *)rgpsz);

        switch (ids)
        {
        case IDS_UNEXPECTED:
            uFlags |= MB_ICONERROR;
            break;

        case IDS_FAILED:
        case IDS_LOADFAILED:
        case IDS_INSTALLFAILED:
            uFlags |= MB_ICONWARNING;
            break;

        default:
            uFlags |= MB_ICONINFORMATION;
            break;
        }
        MessageBox(hwnd, szMsg, szT, uFlags);
    }
    return hres;
}


// stolen from the CRT, used to shrink our code

int 
_stdcall 
ModuleEntry(void)
{
    int i;
    STARTUPINFO si;
    LPTSTR pszCmdLine = GetCommandLine();

    si.dwFlags = 0;
    GetStartupInfoA(&si);

    i = WinMain(GetModuleHandle(NULL), 
                NULL, 
                pszCmdLine,
                (si.dwFlags & STARTF_USESHOWWINDOW) ? si.wShowWindow : SW_SHOWDEFAULT);

    ExitProcess(i);
    return i;           // We never come here
}


/*----------------------------------------------------------
Purpose: Stub window proc

Returns: 
Cond:    --
*/
LRESULT 
CALLBACK 
WndProc(
    HWND hWnd, 
    UINT iMessage, 
    WPARAM wParam, 
    LPARAM lParam)
{
    switch(iMessage)
    {
    case WM_CREATE:
            break;

    case WM_DESTROY:
            break;

    default:
            return DefWindowProc(hWnd, iMessage, wParam, lParam);
            break;
    }

    return 0L;
}


/*----------------------------------------------------------
Purpose: Initialize a stub window

Returns: 
Cond:    --
*/
BOOL 
InitStubWindow(
    IN  HINSTANCE hInst, 
    IN  HINSTANCE hPrevInstance,
    OUT HWND *    phwnd)
{
    WNDCLASS wndclass;

    if (!hPrevInstance)
    {
        wndclass.style         = 0 ;
        wndclass.lpfnWndProc   = (WNDPROC)WndProc ;
        wndclass.cbClsExtra    = 0 ;
        wndclass.cbWndExtra    = 0 ;
        wndclass.hInstance     = hInst ;
        wndclass.hIcon         = NULL;
        wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW);
        wndclass.hbrBackground  = (HBRUSH)(COLOR_WINDOW + 1);
        wndclass.lpszMenuName  = NULL ;
        wndclass.lpszClassName = c_szAppName;

        if (!RegisterClass(&wndclass))
        {
            return(FALSE);
        }
    }

    *phwnd = CreateWindowEx(WS_EX_TOOLWINDOW,
                            c_szAppName, 
                            TEXT(""), 
                            WS_OVERLAPPED, 
                            CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, 
                            NULL, NULL, hInst, NULL);

    return (NULL != *phwnd);
}


/*----------------------------------------------------------
Purpose: WinMain

Returns: 
Cond:    --
*/
int 
WinMain(
    HINSTANCE hInstance, 
    HINSTANCE hPrevInstance, 
    LPTSTR pszCmdLine, 
    int nCmdShow)
{
    HWND hwndStub;
    int nRet = 0;

    g_hinst = hInstance;

#ifdef DEBUG

    CcshellGetDebugFlags();

    if (IsFlagSet(g_dwBreakFlags, BF_ONOPEN))
        DebugBreak();

#endif

    // turn off critical error stuff
    SetErrorMode(SEM_NOOPENFILEERRORBOX | SEM_FAILCRITICALERRORS);

    if (InitStubWindow(hInstance, hPrevInstance, &hwndStub))
    {
        // Do work here
        nRet = DoWork(hwndStub, pszCmdLine);

        DestroyWindow(hwndStub);
    }

    return nRet;
}


