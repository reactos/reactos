//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1993-1994
//
// File: thread.c
//
//  This files contains the thread creation routines.  (These routines
//  were swiped from the shell.)
//
//  Public API is RunDLLThread().
//
//---------------------------------------------------------------------------

#include "brfprv.h"

#pragma data_seg(DATASEG_READONLY)
static const TCHAR c_szStubWindowClass[] = TEXT("BrfStubWindow32");
#pragma data_seg()

// SHOpenPropSheet uses these messages.  It is unclear if
// we need to support this or not.  
#define STUBM_SETDATA   (WM_USER)
#define STUBM_GETDATA   (WM_USER+1)

typedef void (WINAPI * NEWTHREADPROC)(HWND hwndStub,                      
        HINSTANCE hAppInstance,                                           
        LPTSTR lpszCmdLine, int nCmdShow);                                 

typedef struct  // dlle
    {
    HINSTANCE  hinst;
    NEWTHREADPROC lpfn;
    } DLLENTRY;

typedef struct // rdlltp
    {
    HINSTANCE  hinstThisDll;
    TCHAR szCmdLine[1];
    } RDLLThreadParam;


typedef struct _NEWTHREAD_NOTIFY                                             
    {                                                                         
    NMHDR   hdr;                                                      
    HICON   hIcon;                                                    
    LPCTSTR  lpszTitle;                                                
    } NEWTHREAD_NOTIFY;                                                          
                                                                          


/*----------------------------------------------------------
Purpose: Parses a command line entry into a DLLENTRY struct.

Returns: TRUE on success
Cond:    --
*/
BOOL PRIVATE InitializeDLLEntry(
    LPTSTR lpszCmdLine, 
    DLLENTRY * pdlle)
    {
    LPTSTR lpStart, lpEnd, lpFunction;

    TRACE_MSG(TF_GENERAL, TEXT("RunDLLThread(%s)"), lpszCmdLine);

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
    CHAR szFunction[MAX_PATH] = "";
    WideCharToMultiByte(CP_ACP, 0, lpFunction, -1, szFunction, MAX_PATH, NULL, NULL);
    pdlle->lpfn = (NEWTHREADPROC)GetProcAddress(pdlle->hinst, szFunction);
}
#else
{
    pdlle->lpfn = (NEWTHREADPROC)GetProcAddress(pdlle->hinst, lpFunction);
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


/*----------------------------------------------------------
Purpose: Handles WM_NOTIFY messages for stub window
Returns: varies
Cond:    --
*/
LRESULT PRIVATE StubNotify(
    HWND hWnd, 
    WPARAM wParam, 
    NEWTHREAD_NOTIFY  *lpn)
    {
    switch (lpn->hdr.code)
        {
#ifdef COOLICON
    case RDN_TASKINFO:
        SetWindowText(hWnd, lpn->lpszTitle ? lpn->lpszTitle : TEXT(""));
        g_hIcon = lpn->hIcon ? lpn->hIcon :
                LoadIcon(hInst, MAKEINTRESOURCE(IDI_DEFAULT));
        break;
#endif

    default:
        return(DefWindowProc(hWnd, WM_NOTIFY, wParam, (LPARAM)lpn));
        }
    }


/*----------------------------------------------------------
Purpose: Window proc for thread

Returns: varies
Cond:    --
*/
LRESULT CALLBACK WndProc(
    HWND hWnd, 
    UINT iMessage, 
    WPARAM wParam, 
    LPARAM lParam)
    {
    switch(iMessage)
        {
    case WM_CREATE:
#ifdef COOLICON
        g_hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_DEFAULT));
#endif
        break;

    case WM_DESTROY:
        break;

    case WM_NOTIFY:
        return(StubNotify(hWnd, wParam, (NEWTHREAD_NOTIFY  *)lParam));

#ifdef COOLICON
    case WM_QUERYDRAGICON:
        return(MAKELRESULT(g_hIcon, 0));
#endif
        
    case STUBM_SETDATA:
        SetWindowLongPtr(hWnd, 0, wParam);
        SetWindowLongPtr(hWnd, sizeof(WPARAM), lParam);
        break;
        
    case  STUBM_GETDATA:
        *((LPARAM *)lParam) = GetWindowLong(hWnd, sizeof(WPARAM));
        return GetWindowLongPtr(hWnd, 0);

    default:
        return DefWindowProc(hWnd, iMessage, wParam, lParam) ;
        break;
        }

    return 0L;
    }


/*----------------------------------------------------------
Purpose: Creates a stub window to handle the thread

Returns: handle to window
Cond:    --
*/
HWND PRIVATE CreateStubWindow(void)
    {
    WNDCLASS wndclass;

    if (!GetClassInfo(g_hinst, c_szStubWindowClass, &wndclass))
        {
        wndclass.style         = 0 ;
        wndclass.lpfnWndProc   = WndProc ;
        wndclass.cbClsExtra    = 0 ;
        wndclass.cbWndExtra    = sizeof(DWORD_PTR) * 2 ;
        wndclass.hInstance     = g_hinst;
        wndclass.hIcon         = NULL ;
        wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW) ;
        wndclass.hbrBackground = GetStockObject (WHITE_BRUSH) ;
        wndclass.lpszMenuName  = NULL ;
        wndclass.lpszClassName = c_szStubWindowClass ;

        if (!RegisterClass(&wndclass))
            {
            return NULL;
            }
        }

    return CreateWindowEx(WS_EX_TOOLWINDOW, c_szStubWindowClass, c_szNULL, 
                        0, 0, 0, 0, 0, NULL, NULL, g_hinst, NULL);
    }


/*----------------------------------------------------------
Purpose: Initializes a new thread in our DLL

Returns: 0
Cond:    --
*/
DWORD PRIVATE ThreadInitDLL(
    RDLLThreadParam *prdlltp)
    {
    DLLENTRY dlle;
    HINSTANCE hinstThisDll = prdlltp->hinstThisDll;

    if (InitializeDLLEntry((LPTSTR)prdlltp->szCmdLine, &dlle))
        {
        HWND hwndStub = CreateStubWindow();
        if (hwndStub)
            {
            SetForegroundWindow(hwndStub);
            dlle.lpfn(hwndStub, g_hinst, prdlltp->szCmdLine, SW_NORMAL);
            DestroyWindow(hwndStub);
            }
        FreeLibrary(dlle.hinst);
        }

    GFree(prdlltp);

    // Free off our extra reference to this dll
    FreeLibraryAndExitThread(hinstThisDll, 0);
    return 0;   // will never get here, but to keep from getting compiler warnings...
    }


/*----------------------------------------------------------
Purpose: Start a new thread

         This code was swiped from the shell.

Returns: TRUE if the thread was created successfully
Cond:    --
*/
BOOL PUBLIC RunDLLThread(
    HWND hwnd, 
    LPCTSTR pszCmdLine, 
    int nCmdShow)
    {
    BOOL fSuccess=FALSE; // assume error
    RDLLThreadParam *prdlltp = GAlloc(sizeof(RDLLThreadParam) + CbFromCch(lstrlen(pszCmdLine)));

    if (prdlltp)
        {
        DWORD idThread;
        HANDLE hthread;
        lstrcpy(prdlltp->szCmdLine, pszCmdLine);

        // Need to update reference count to this dll incase someone unloads us from under...
        prdlltp->hinstThisDll = LoadLibrary(TEXT("SYNCUI.DLL"));

        if (hthread=CreateThread(NULL, 0, ThreadInitDLL, prdlltp, 0, &idThread))
            {
            // We don't need to communicate with this thread any more.
            // Close the handle and let it run and terminate itself.
            //
            // Notes: In this case, prdlltp will be freed by the thread.
            //
            CloseHandle(hthread);
            fSuccess=TRUE;
            }
        else
            {
            // Thread creation failed, we should free the buffer.
            FreeLibrary(prdlltp->hinstThisDll);
            GFree(prdlltp);
            }
        }

    return fSuccess;
    }
