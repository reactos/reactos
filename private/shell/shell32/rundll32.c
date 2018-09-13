#include "shellprv.h"
#pragma  hdrstop
#include <regstr.h>

TCHAR const c_szRunDll[] = TEXT("rundll32.exe");

//
// Emulate multi-threads with multi-processes.
//
STDAPI_(BOOL) SHRunDLLProcess(HWND hwnd, LPCTSTR pszCmdLine, int nCmdShow, UINT idStr, BOOL fRunAsNewUser)
{
    BOOL bRet;
    HKEY hkey;
    SHELLEXECUTEINFO ExecInfo = {0};
    TCHAR szPath[MAX_PATH];

    // I hate network install. The windows directory is not the windows
    // directory
    szPath[0] = TEXT('\0');
    if (RegOpenKey(HKEY_LOCAL_MACHINE, REGSTR_PATH_SETUP TEXT("\\Setup"), &hkey) == ERROR_SUCCESS)
    {
        DWORD dwType;
        DWORD cbData = SIZEOF(szPath);;
        if (SHQueryValueEx(hkey, TEXT("SharedDir"), NULL, &dwType, (LPBYTE)szPath, &cbData) != ERROR_SUCCESS)
            szPath[0] = TEXT('\0');
        RegCloseKey(hkey);
    }
    PathCombine(szPath, szPath, c_szRunDll);

    DebugMsg(DM_TRACE, TEXT("sh TR - RunDLLProcess (%s)"), pszCmdLine);
    FillExecInfo(ExecInfo, hwnd, NULL, szPath, pszCmdLine, szNULL, nCmdShow);

    // if we want to launch this cpl as a new user, set the verb to be "runas"
    if (fRunAsNewUser)
    {
        ExecInfo.lpVerb = TEXT("runas");
    }
    else
    {
        // normal execute so no ui, we do our own error messages
        ExecInfo.fMask = SEE_MASK_FLAG_NO_UI;
    }

    //
    // We need to put an appropriate message box.
    //
    bRet = ShellExecuteEx(&ExecInfo);

    if (!bRet && !fRunAsNewUser)
    {
        // If we failed and we werent passing fRunAsNewUser, then we put up our own error UI,
        // else, if we were running this as a new user, then we didnt pass SEE_MASK_FLAG_NO_UI
        // so the error is already taken care of for us by shellexec.
        TCHAR szTitle[64];
        DWORD dwErr = GetLastError(); // LoadString can stomp on this (on failure)
        LoadString(HINST_THISDLL, idStr, szTitle, ARRAYSIZE(szTitle));
        ExecInfo.fMask = 0;
        _ShellExecuteError(&ExecInfo, szTitle, dwErr);
    }

    return bRet;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
    case STUBM_SETICONTITLE:
        if (lParam)
            SetWindowText(hWnd, (LPCTSTR)lParam);
        if (wParam)
            SendMessage(hWnd, WM_SETICON, ICON_BIG, wParam);
        break;

    case STUBM_SETDATA:
        SetWindowLongPtr(hWnd, 0, wParam);
        break;
        
    case STUBM_GETDATA:
        return GetWindowLong(hWnd, 0);
        
    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam) ;
    }
    
    return 0;
}


HWND _CreateStubWindow(POINT * ppt, HWND hwndParent)
{
    WNDCLASS wc;
    int cx, cy;
    // If the stub window is parented, then we want it to be a tool window. This prevents activation
    // problems when this is used in multimon for positioning.

    DWORD dwExStyle = hwndParent? WS_EX_TOOLWINDOW : WS_EX_APPWINDOW;
    if (!GetClassInfo(HINST_THISDLL, c_szStubWindowClass, &wc))
    {
        wc.style         = 0;
        wc.lpfnWndProc   = WndProc;
        wc.cbClsExtra    = 0;
        wc.cbWndExtra    = SIZEOF(DWORD) * 2;
        wc.hInstance     = HINST_THISDLL;
        wc.hIcon         = NULL;
        wc.hCursor       = LoadCursor (NULL, IDC_ARROW);
        wc.hbrBackground = GetStockObject (WHITE_BRUSH);
        wc.lpszMenuName  = NULL;
        wc.lpszClassName = c_szStubWindowClass;

        RegisterClass(&wc);
    }

    cx = cy = CW_USEDEFAULT;
    if (ppt)
    {
        cx = (int)ppt->x;
        cy = (int)ppt->y;
    }

    if (IS_BIDI_LOCALIZED_SYSTEM()) 
    {
        dwExStyle |= dwExStyleRTLMirrorWnd;
    }
    
    // WS_EX_APPWINDOW makes this show up in ALT+TAB, but not the tray.
        
    return CreateWindowEx(dwExStyle, c_szStubWindowClass, c_szNULL, hwndParent? WS_POPUP : WS_OVERLAPPED, cx, cy, 0, 0, hwndParent, NULL, HINST_THISDLL, NULL);
}


typedef struct  // dlle
{
    HINSTANCE  hinst;
    RUNDLLPROC lpfn;
    BOOL       fCmdIsANSI;
} DLLENTRY;


BOOL _InitializeDLLEntry(LPTSTR lpszCmdLine, DLLENTRY * pdlle)
{
    LPTSTR lpStart, lpEnd, lpFunction;

    DebugMsg(DM_TRACE, TEXT("sh TR - RunDLLThread (%s)"), lpszCmdLine);

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

        // We have found the DLL,FN parameter
        //
    lpEnd = StrChr(lpStart, TEXT(' '));
    if (lpEnd)
    {
        *lpEnd++ = TEXT('\0');
    }

        // There must be a DLL name and a function name
        //
    lpFunction = StrChr(lpStart, TEXT(','));
    if (!lpFunction)
    {
        return(FALSE);
    }
    *lpFunction++ = TEXT('\0');

        // Load the library and get the procedure address
        // Note that we try to get a module handle first, so we don't need
        // to pass full file names around
        //
    pdlle->hinst = GetModuleHandle(lpStart);
    if (pdlle->hinst)
    {
        TCHAR szName[MAXPATHLEN];

        GetModuleFileName(pdlle->hinst, szName, ARRAYSIZE(szName));
        LoadLibrary(szName);
    }
    else
    {
        pdlle->hinst = LoadLibrary(lpStart);
        if (!ISVALIDHINSTANCE(pdlle->hinst))
        {
            return(FALSE);
        }
    }

#ifdef UNICODE
    {
        /*
         * Look for a 'W' tagged Unicode function.
         * If it is not there, then look for the 'A' tagged ANSI function
         * if we cant find that one either, then look for an un-tagged function
         */
        LPSTR pszFunctionName;
        UINT cchLength;

        cchLength = lstrlen(lpFunction)+1;
        pdlle->fCmdIsANSI = FALSE;

        pszFunctionName = (LPSTR)LocalAlloc(LMEM_FIXED, (cchLength+1)*2);    // +1 for "W",  *2 for DBCS

        if (pszFunctionName && (WideCharToMultiByte (CP_ACP, 0, lpFunction, cchLength,
            pszFunctionName, cchLength*2, NULL, NULL))) {

            cchLength = lstrlenA(pszFunctionName);
            pszFunctionName[cchLength] = 'W';        // convert name to Wide version
            pszFunctionName[cchLength+1] = '\0';

            pdlle->lpfn = (RUNDLLPROC)GetProcAddress(pdlle->hinst, pszFunctionName);

            if (pdlle->lpfn == NULL) {
                    // No UNICODE version, try for ANSI
                pszFunctionName[cchLength] = 'A';        // convert name to ANSI version
                pdlle->fCmdIsANSI = TRUE;

                pdlle->lpfn = (RUNDLLPROC)GetProcAddress(pdlle->hinst, pszFunctionName);

                if (pdlle->lpfn == NULL) {
                        // No ANSI version either, try for non-tagged
                    pszFunctionName[cchLength] = '\0';        // convert name to ANSI version

                    pdlle->lpfn = (RUNDLLPROC)GetProcAddress(pdlle->hinst, pszFunctionName);
                }
            }
        }
        if (pszFunctionName) {
            LocalFree((LPVOID)pszFunctionName);
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

        pdlle->lpfn = NULL;

        cchFunction = lstrlen(lpFunction);

        pszFunction = LocalAlloc(LMEM_FIXED, cchFunction + sizeof(CHAR) * 2);  // string + 'A' + '\0'
        if (pszFunction != NULL)
        {
            CopyMemory(pszFunction, lpFunction, cchFunction);

            pszFunction[cchFunction++] = 'A';
            pszFunction[cchFunction] = '\0';

            pdlle->lpfn = (RUNDLLPROC)GetProcAddress(pdlle->hinst, pszFunction);

            LocalFree(pszFunction);
        }
    }

#endif

    if (!pdlle->lpfn)
    {
        FreeLibrary(pdlle->hinst);
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

DWORD WINAPI _ThreadInitDLL(LPVOID pszCmdLine)
{
    DLLENTRY dlle;

    if (_InitializeDLLEntry((LPTSTR)pszCmdLine, &dlle))
    {
        HWND hwndStub=_CreateStubWindow(NULL, NULL);
        if (hwndStub)
        {
            ULONG cchCmdLine = 0;
            SetForegroundWindow(hwndStub);
#ifdef UNICODE
            if (dlle.fCmdIsANSI)
            {
                //
                // If the function is an ANSI version 
                // Change the command line parameter strings to ANSI before we call the function
                // 
                int cchCmdLine = lstrlen((LPTSTR)pszCmdLine);

                LPVOID pszCommand = LocalAlloc(LMEM_FIXED, sizeof(char) * (cchCmdLine + 1));
                if (pszCommand)
                {
                    WideCharToMultiByte(CP_ACP, 0, (LPTSTR)pszCmdLine, -1, pszCommand, cchCmdLine, NULL, NULL);
                }
                dlle.lpfn(hwndStub, g_hinst, pszCommand, SW_NORMAL);

            }
            else
#endif
            {
                dlle.lpfn(hwndStub, g_hinst, pszCmdLine, SW_NORMAL);
            }
            
            DestroyWindow(hwndStub);
        }
        FreeLibrary(dlle.hinst);
    }

    LocalFree((HLOCAL)pszCmdLine);

    return 0;
}

// BUGBUG: We don't pass nCmdShow.

BOOL WINAPI SHRunDLLThread(HWND hwnd, LPCTSTR pszCmdLine, int nCmdShow)
{
    BOOL fSuccess = FALSE; // assume error
    LPTSTR pszCopy = (void*)LocalAlloc(LPTR, (lstrlen(pszCmdLine) + 1) * SIZEOF(TCHAR));

    if (pszCopy)
    {
        DWORD idThread;
        HANDLE hthread;

        lstrcpy(pszCopy, pszCmdLine);

        hthread = CreateThread(NULL, 0, _ThreadInitDLL, pszCopy, 0, &idThread);
        if (hthread)
        {
            // We don't need to communicate with this thread any more.
            // Close the handle and let it run and terminate itself.
            //
            // Notes: In this case, pszCopy will be freed by the thread.
            //
            CloseHandle(hthread);
            fSuccess = TRUE;
        }
        else
        {
            // Thread creation failed, we should free the buffer.
            LocalFree((HLOCAL)pszCopy);
        }
    }

    return fSuccess;
}

