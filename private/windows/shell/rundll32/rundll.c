#include "rundll.h"
#include "tchar.h"

#ifndef WIN32
#include <w32sys.h>             // for IsPEFormat definition
#endif

#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))
#define Reference(x) ((x)=(x))

void WINAPI RunDllErrMsg(HWND hwnd, UINT idStr, LPCTSTR pszTitle, LPCTSTR psz1, LPCTSTR psz2);
int PASCAL WinMainT(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpszCmdLine, int nCmdShow);

TCHAR const g_szAppName [] = TEXT("RunDLL");
TCHAR const s_szRunDLL32[] = TEXT("RUNDLL32.EXE ");
TCHAR const c_szNULL[] = TEXT("");
TCHAR const c_szLocalizeMe[] = TEXT("RUNDLL");  // BUGBUG

HINSTANCE g_hinst;
HICON g_hIcon;

HINSTANCE g_hModule;
HWND g_hwndStub;

#ifdef WX86

#include <wx86dll.h>

WX86LOADX86DLL_ROUTINE pWx86LoadX86Dll = NULL;
WX86THUNKPROC_ROUTINE pWx86ThunkProc = NULL;
HMODULE g_hWx86Dll = NULL;

#endif

RUNDLLPROC g_lpfnCommand;
#ifdef UNICODE
BOOL g_fCmdIsANSI;   // TRUE if g_lpfnCommand() expects ANSI strings
#endif


#ifndef WIN32
void WINAPI WinExecError(HWND hwnd, int err, LPCTSTR lpstrFileName, LPCTSTR lpstrTitle)
{
    RunDllErrMsg(hwnd, err+IDS_LOADERR, lpstrTitle, lpstrFileName, NULL);
}
#endif


LPTSTR NEAR PASCAL StringChr(LPCTSTR lpStart, TCHAR ch)
{
    for (; *lpStart; lpStart = CharNext(lpStart))
    {
        if (*lpStart == ch)
            return (LPTSTR)lpStart;
    }
    return NULL;
}

#ifdef WIN32
// stolen from the CRT, used to shirink our code

int _stdcall ModuleEntry(void)
{
    int i;
    STARTUPINFO si;
    LPTSTR pszCmdLine = GetCommandLine();

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
    GetStartupInfo(&si);

    i = WinMainT(GetModuleHandle(NULL), NULL, pszCmdLine,
                   si.dwFlags & STARTF_USESHOWWINDOW ? si.wShowWindow : SW_SHOWDEFAULT);
    ExitProcess(i);
    return i;   // We never comes here.
}

#endif // WIN32


BOOL NEAR PASCAL ParseCommand(LPTSTR lpszCmdLine, int nCmdShow)
{
    LPTSTR lpStart, lpEnd, lpFunction;
#ifndef WIN32
    LPTSTR pCopy;

    // build a rundll32-based winexec string just in case
    pCopy = (LPTSTR)LocalAlloc(LPTR, lstrlen(lpszCmdLine)*sizeof(TCHAR) + sizeof(s_szRunDLL32));
    if (!pCopy)
            return FALSE;
    lstrcpy(pCopy, s_szRunDLL32);
    lstrcat(pCopy, lpszCmdLine);
#endif

#ifdef DEBUG
OutputDebugString(TEXT("RUNDLL: Command: "));
OutputDebugString(lpszCmdLine);
OutputDebugString(TEXT("\r\n"));
#endif
        for (lpStart=lpszCmdLine; ; )
        {
                // Skip leading blanks
                //
                while (*lpStart == TEXT(' '))
                {
                        ++lpStart;
                }

                // Check if there are any switches
                //
                if (*lpStart != TEXT('/'))
                {
                        break;
                }

                // Look at all the switches; ignore unknown ones
                //
                for (++lpStart; ; ++lpStart)
                {
                        switch (*lpStart)
                        {
                        case TEXT(' '):
                        case TEXT('\0'):
                                goto EndSwitches;
                                break;

                        // Put any switches we care about here
                        //

                        default:
                                break;
                        }
                }
EndSwitches:
                ;
        }

        // If the path is double-quoted, search for the next
        // quote, otherwise, look for a space 
                
        lpEnd = lpStart;
        if ( *lpStart == TEXT('\"') ) {
            
            // Skip opening quote

            lpStart++;
                        
            // Scan, and skip over, subsequent characters until
            // another double-quote or a null is encountered.
             
            while ( *++lpEnd && (*lpEnd != TEXT('\"')) )
                NULL;
            if (!*lpEnd)
                return FALSE;
                            
            *lpEnd++ = TEXT('\0');
        }
        else
        {
            // No quotes, so run until a space or a comma
            while ( *lpEnd && (*lpEnd != TEXT(' ')) && (*lpEnd != TEXT(',')))
                lpEnd++;
            if (!*lpEnd)
                return FALSE;

            *lpEnd++ = TEXT('\0');
        }

        // At this point we're just past the terminated dll path.   We
        // then skip spaces and commas, which should take us to the start of the 
        // entry point (lpFunction)

        while ( *lpEnd && ((*lpEnd == TEXT(' ')) || (*lpEnd == TEXT(','))))
            lpEnd++;
        if (!*lpEnd)
            return FALSE;

        lpFunction = lpEnd;

        // If there's a space after the function name, we need to terminate 
        // the function name and move the end pointer, because that's where
        // the arguments to the function live.

        lpEnd = StringChr(lpFunction, TEXT(' '));
        if (lpEnd)
            *lpEnd++ = TEXT('\0');

        // Load the library and get the procedure address
        // Note that we try to get a module handle first, so we don't need
        // to pass full file names around
        //

#ifndef WIN32
        // if loading a 32 bit DLL out of 16bit rundll, exec the
        // 32 bit version of rundll and return
        if (IsPEFormat(lpStart, NULL))
        {
                int err = (int)WinExec(pCopy, nCmdShow);
                WinExecError(NULL, err, lpStart, c_szLocalizeMe);
                LocalFree((HLOCAL)pCopy);
                return FALSE;
        }
        else
        {
                LocalFree((HLOCAL)pCopy);
        }
#endif

        g_hModule = LoadLibrary(lpStart);
#ifdef WIN32

#ifdef WX86

        //
        // If the load fails try it thru wx86, since it might be an
        // x86 on risc binary
        //

        if (g_hModule==NULL) {

            g_hWx86Dll = LoadLibrary(TEXT("wx86.dll"));
            if (g_hWx86Dll) {
                pWx86LoadX86Dll = (PVOID)GetProcAddress(g_hWx86Dll, "Wx86LoadX86Dll");
                pWx86ThunkProc  = (PVOID)GetProcAddress(g_hWx86Dll, "Wx86ThunkProc");
                if (pWx86LoadX86Dll && pWx86ThunkProc) {
                    g_hModule = pWx86LoadX86Dll(lpStart, 0);
                    }
                }

            if (!g_hModule) {
                if (g_hWx86Dll) {
                    FreeLibrary(g_hWx86Dll);
                    g_hWx86Dll = NULL;
                    }
                }
            }
#endif


        if (g_hModule==NULL)
        {
            TCHAR szSysErrMsg[MAX_PATH];
            BOOL fSuccess = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                              NULL, GetLastError(), 0, szSysErrMsg, ARRAYSIZE(szSysErrMsg), NULL);
            if (fSuccess)
            {
                RunDllErrMsg(NULL, IDS_CANTLOADDLL, c_szLocalizeMe, lpStart, szSysErrMsg);
            }
            return FALSE;
        }
#else
        if ((UINT)g_hModule <= 32)
        {
            WinExecError(NULL, (int)g_hModule, lpStart, c_szLocalizeMe);
            return(FALSE);
        }
#endif

#ifdef WINNT        // REVIEW: May need this on Nashville too...
        //
        // Check whether we need to run as a different windows version
        //
        // BUGBUG - Stolen from ntos\mm\procsup.c
        //
        //
        {
            PPEB Peb = NtCurrentPeb();
            PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)g_hModule;
            PIMAGE_NT_HEADERS pHeader = (PIMAGE_NT_HEADERS)((DWORD_PTR)g_hModule + pDosHeader->e_lfanew);
            PIMAGE_LOAD_CONFIG_DIRECTORY ImageConfigData;
            ULONG ReturnedSize;

            if (pHeader->FileHeader.SizeOfOptionalHeader != 0 &&
                pHeader->OptionalHeader.Win32VersionValue != 0)
            {
                Peb->OSMajorVersion = pHeader->OptionalHeader.Win32VersionValue & 0xFF;
                Peb->OSMinorVersion = (pHeader->OptionalHeader.Win32VersionValue >> 8) & 0xFF;
                Peb->OSBuildNumber  = (USHORT)((pHeader->OptionalHeader.Win32VersionValue >> 16) & 0x3FFF);
                Peb->OSPlatformId   = (pHeader->OptionalHeader.Win32VersionValue >> 30) ^ 0x2;
            }

            ImageConfigData = ImageDirectoryEntryToData( Peb->ImageBaseAddress,
                                                         TRUE,
                                                         IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG,
                                                         &ReturnedSize
                                                   );
            if (ImageConfigData != NULL && ImageConfigData->CSDVersion != 0)
            {
                Peb->OSCSDVersion = ImageConfigData->CSDVersion;
            }
        }
#endif

#ifdef UNICODE
        {
            /*
             * Look for a 'W' tagged Unicode function.
             * If it is not there, then look for the 'A' tagged ANSI function
             * if we cant find that one either, then look for an un-tagged function
             */
            LPSTR lpstrFunctionName;
            UINT cchLength;

            cchLength = lstrlen(lpFunction)+1;
            g_fCmdIsANSI = FALSE;

            lpstrFunctionName = (LPSTR)LocalAlloc(LMEM_FIXED, (cchLength+1)*2);    // +1 for "W",  *2 for DBCS

            if (lpstrFunctionName && (WideCharToMultiByte (CP_ACP, 0, lpFunction, cchLength,
                             lpstrFunctionName, cchLength*2, NULL, NULL))) {

                cchLength = lstrlenA(lpstrFunctionName);
                lpstrFunctionName[cchLength] = 'W';        // convert name to Wide version
                lpstrFunctionName[cchLength+1] = '\0';

                g_lpfnCommand = (RUNDLLPROC)GetProcAddress(g_hModule, lpstrFunctionName);

                if (g_lpfnCommand == NULL) {
                    // No UNICODE version, try for ANSI
                    lpstrFunctionName[cchLength] = 'A';        // convert name to ANSI version
                    g_fCmdIsANSI = TRUE;

                    g_lpfnCommand = (RUNDLLPROC)GetProcAddress(g_hModule, lpstrFunctionName);

                    if (g_lpfnCommand == NULL) {
                        // No ANSI version either, try for non-tagged
                        lpstrFunctionName[cchLength] = '\0';        // convert name to ANSI version

                        g_lpfnCommand = (RUNDLLPROC)GetProcAddress(g_hModule, lpstrFunctionName);
                    }
                }
            }
            if (lpstrFunctionName) {
                LocalFree((LPVOID)lpstrFunctionName);
            }
        }
#else
        {
            /*
             * Look for 'A' tagged ANSI version.
             * If it is not there, then look for a non-tagged function.
             */
            LPSTR pszFunction;
            int  cchFunction;

            g_lpfnCommand = NULL;

            cchFunction = lstrlen(lpFunction);

            pszFunction = LocalAlloc(LMEM_FIXED, cchFunction + sizeof(CHAR) * 2);  // string + 'A' + '\0'
            if (pszFunction != NULL)

                CopyMemory(pszFunction, lpFunction, cchFunction);

                pszFunction[cchFunction++] = 'A';
                pszFunction[cchFunction] = '\0';

                g_lpfnCommand = (RUNDLLPROC)GetProcAddress(g_hModule, achFunction);

                LocalFree(pszFunction);
            }

            if (g_lpfnCommand == NULL) {
                // No "A" tagged function, just look for the non tagged name
                g_lpfnCommand = (RUNDLLPROC)GetProcAddress(g_hModule, lpFunction);
            }
        }
#endif

#ifdef WX86
        if (g_lpfnCommand && g_hWx86Dll) {
            g_lpfnCommand = pWx86ThunkProc(g_lpfnCommand, (PVOID)4, TRUE);
            }
#endif

        if (!g_lpfnCommand)
        {
                RunDllErrMsg(NULL, IDS_GETPROCADRERR, c_szLocalizeMe, lpStart, lpFunction);
                FreeLibrary(g_hModule);
                return(FALSE);
        }

        // Copy the rest of the command parameters down
        //
        if (lpEnd)
        {
                lstrcpy(lpszCmdLine, lpEnd);
        }
        else
        {
                *lpszCmdLine = TEXT('\0');
        }

        return(TRUE);
}

#ifdef WINNT

LRESULT NEAR PASCAL StubNotify(HWND hWnd, WPARAM wParam, RUNDLL_NOTIFY FAR *lpn)
{
        switch (lpn->hdr.code)
        {
        case RDN_TASKINFO:
// don't need to set title too
//              SetWindowText(hWnd, lpn->lpszTitle ? lpn->lpszTitle : c_szNULL);
                g_hIcon = lpn->hIcon ? lpn->hIcon :
                        LoadIcon(g_hinst, MAKEINTRESOURCE(IDI_DEFAULT));

                SetClassLongPtr(hWnd, GCLP_HICON, (DWORD_PTR)g_hIcon);

                return 0L;

        default:
                return(DefWindowProc(hWnd, WM_NOTIFY, wParam, (LPARAM)lpn));
        }
}

#endif

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
        switch(iMessage)
        {
        case WM_CREATE:
                g_hIcon = LoadIcon(g_hinst, MAKEINTRESOURCE(IDI_DEFAULT));
                break;

        case WM_DESTROY:
                break;

#ifdef WINNT
        case WM_NOTIFY:
                return(StubNotify(hWnd, wParam, (RUNDLL_NOTIFY *)lParam));
#endif

#ifdef COOLICON
        case WM_QUERYDRAGICON:
                return(MAKELRESULT(g_hIcon, 0));
#endif

        default:
                return DefWindowProc(hWnd, iMessage, wParam, lParam) ;
                break;
        }

        return 0L;
}


BOOL NEAR PASCAL InitStubWindow(HINSTANCE hInst, HINSTANCE hPrevInstance)
{
        WNDCLASS wndclass;

        if (!hPrevInstance)
        {
                wndclass.style         = 0 ;
                wndclass.lpfnWndProc   = (WNDPROC)WndProc ;
                wndclass.cbClsExtra    = 0 ;
                wndclass.cbWndExtra    = 0 ;
                wndclass.hInstance     = hInst ;
#ifdef COOLICON
                wndclass.hIcon         = NULL ;
#else
                wndclass.hIcon         = LoadIcon(hInst, MAKEINTRESOURCE(IDI_DEFAULT)) ;
#endif
                wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW) ;
                wndclass.hbrBackground = GetStockObject (WHITE_BRUSH) ;
                wndclass.lpszMenuName  = NULL ;
                wndclass.lpszClassName = g_szAppName ;

                if (!RegisterClass(&wndclass))
                {
                        return(FALSE);
                }
        }

        g_hwndStub = CreateWindowEx(WS_EX_TOOLWINDOW,
                                    g_szAppName, c_szNULL, WS_OVERLAPPED, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, NULL, NULL, hInst, NULL);

        return(g_hwndStub != NULL);
}


void NEAR PASCAL CleanUp(void)
{
        DestroyWindow(g_hwndStub);

        FreeLibrary(g_hModule);
}


int PASCAL WinMainT (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpszCmdLine, int nCmdShow)
{
        LPTSTR lpszCmdLineCopy;

        g_hinst = hInstance;

        // turn off critical error bullshit
        SetErrorMode(SEM_NOOPENFILEERRORBOX | SEM_FAILCRITICALERRORS);

        // make a copy of lpCmdLine, since ParseCommand modifies the string
        lpszCmdLineCopy = LocalAlloc(LPTR, (_tcslen(lpszCmdLine)+1)*sizeof(TCHAR));
        if (!lpszCmdLineCopy)
        {
            goto Error0;
        }
        _tcscpy(lpszCmdLineCopy, lpszCmdLine);

        if (!ParseCommand(lpszCmdLineCopy, nCmdShow))
        {
                goto Error1;
        }

        if (!InitStubWindow(hInstance, hPrevInstance))
        {
                goto Error2;
        }

        {
            LPVOID pchCmdLine;

            pchCmdLine = lpszCmdLineCopy;

#ifdef UNICODE

            if (g_fCmdIsANSI) {
                int cchCmdLine;

                cchCmdLine = WideCharToMultiByte(CP_ACP, 0, lpszCmdLineCopy, -1, NULL, 0, NULL, NULL);
                pchCmdLine = LocalAlloc( LMEM_FIXED, sizeof(char) * cchCmdLine );
                if (pchCmdLine == NULL) {
                    RunDllErrMsg(NULL, IDS_LOADERR+00, c_szLocalizeMe, lpszCmdLineCopy, NULL);
                    goto Error3;
                }

                WideCharToMultiByte(CP_ACP, 0, lpszCmdLineCopy, -1, pchCmdLine, cchCmdLine, NULL, NULL);
            }
#endif

            try
            {
                g_lpfnCommand(g_hwndStub, hInstance, pchCmdLine, nCmdShow);
            }
            _except (EXCEPTION_EXECUTE_HANDLER)
            {
                RunDllErrMsg(NULL, IDS_LOADERR+17, c_szLocalizeMe, lpszCmdLine, NULL);
            }

#ifdef UNICODE
Error3:
            if (g_fCmdIsANSI) {
                LocalFree(pchCmdLine);
            }
#endif
        }



Error2:
        CleanUp();
Error1:
        LocalFree(lpszCmdLineCopy);
Error0:
        return(FALSE);
}

void WINAPI RunDllErrMsg(HWND hwnd, UINT idStr, LPCTSTR pszTitle, LPCTSTR psz1, LPCTSTR psz2)
{
    TCHAR szTmp[200];
    TCHAR szMsg[200 + MAX_PATH];

    if (LoadString(g_hinst, idStr, szTmp, ARRAYSIZE(szTmp)))
    {
        wsprintf(szMsg, szTmp, psz1, psz2);
        MessageBox(hwnd, szMsg, pszTitle, MB_OK|MB_ICONHAND);
    }
}
