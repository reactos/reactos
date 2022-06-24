/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS user32.dll
 * FILE:            win32ss/user/user32/windows/window.c
 * PURPOSE:         Window management
 * PROGRAMMERS:     Casper S. Hornstrup (chorns@users.sourceforge.net)
 *                  Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 * UPDATE HISTORY:
 *      06-06-2001  CSH  Created
 */

#define DEBUG
#include <user32.h>

WINE_DEFAULT_DEBUG_CHANNEL(user32);

void MDI_CalcDefaultChildPos( HWND hwndClient, INT total, LPPOINT lpPos, INT delta, UINT *id );
extern LPCWSTR FASTCALL ClassNameToVersion(const void *lpszClass, LPCWSTR lpszMenuName, LPCWSTR *plpLibFileName, HANDLE *pContext, BOOL bAnsi);

/* FUNCTIONS *****************************************************************/


NTSTATUS WINAPI
User32CallSendAsyncProcForKernel(PVOID Arguments, ULONG ArgumentLength)
{
    PSENDASYNCPROC_CALLBACK_ARGUMENTS CallbackArgs;

    TRACE("User32CallSendAsyncProcKernel()\n");

    CallbackArgs = (PSENDASYNCPROC_CALLBACK_ARGUMENTS)Arguments;

    if (ArgumentLength != sizeof(SENDASYNCPROC_CALLBACK_ARGUMENTS))
    {
        return(STATUS_INFO_LENGTH_MISMATCH);
    }

    CallbackArgs->Callback(CallbackArgs->Wnd,
                           CallbackArgs->Msg,
                           CallbackArgs->Context,
                           CallbackArgs->Result);
    return(STATUS_SUCCESS);
}


/*
 * @implemented
 */
BOOL WINAPI
AllowSetForegroundWindow(DWORD dwProcessId)
{
    return NtUserxAllowSetForegroundWindow(dwProcessId);
}


/*
 * @unimplemented
 */
HDWP WINAPI
BeginDeferWindowPos(int nNumWindows)
{
    return NtUserxBeginDeferWindowPos(nNumWindows);
}


/*
 * @implemented
 */
BOOL WINAPI
BringWindowToTop(HWND hWnd)
{
    return NtUserSetWindowPos(hWnd,
                              HWND_TOP,
                              0,
                              0,
                              0,
                              0,
                              SWP_NOSIZE | SWP_NOMOVE);
}


VOID WINAPI
SwitchToThisWindow(HWND hwnd, BOOL fAltTab)
{
    NtUserxSwitchToThisWindow(hwnd, fAltTab);
}


/*
 * @implemented
 */
HWND WINAPI
ChildWindowFromPoint(HWND hWndParent,
                     POINT Point)
{
    return (HWND) NtUserChildWindowFromPointEx(hWndParent, Point.x, Point.y, 0);
}


/*
 * @implemented
 */
HWND WINAPI
ChildWindowFromPointEx(HWND hwndParent,
                       POINT pt,
                       UINT uFlags)
{
    return (HWND) NtUserChildWindowFromPointEx(hwndParent, pt.x, pt.y, uFlags);
}


/*
 * @implemented
 */
BOOL WINAPI
CloseWindow(HWND hWnd)
{
    /* NOTE: CloseWindow does minimizes, and doesn't close. */
    SetActiveWindow(hWnd);
    return ShowWindow(hWnd, SW_SHOWMINIMIZED);
}

FORCEINLINE
VOID
RtlInitLargeString(
    OUT PLARGE_STRING plstr,
    LPCVOID psz,
    BOOL bUnicode)
{
    if(bUnicode)
    {
        RtlInitLargeUnicodeString((PLARGE_UNICODE_STRING)plstr, (PWSTR)psz, 0);
    }
    else
    {
        RtlInitLargeAnsiString((PLARGE_ANSI_STRING)plstr, (PSTR)psz, 0);
    }
}

VOID
NTAPI
RtlFreeLargeString(
    IN PLARGE_STRING LargeString)
{
    if (LargeString->Buffer)
    {
        RtlFreeHeap(GetProcessHeap(), 0, LargeString->Buffer);
        RtlZeroMemory(LargeString, sizeof(LARGE_STRING));
    }
}

DWORD
FASTCALL
RtlGetExpWinVer(HMODULE hModule)
{
    DWORD dwMajorVersion = 3;  // Set default to Windows 3.10.
    DWORD dwMinorVersion = 10;
    PIMAGE_NT_HEADERS pinth;

    if (hModule && !LOWORD((ULONG_PTR)hModule))
    {
        pinth = RtlImageNtHeader(hModule);
        if (pinth)
        {
            dwMajorVersion = pinth->OptionalHeader.MajorSubsystemVersion;

            if (dwMajorVersion == 1)
            {
                dwMajorVersion = 3;
            }
            else
            {
                dwMinorVersion = pinth->OptionalHeader.MinorSubsystemVersion;
            }
        }
    }
    return MAKELONG(MAKEWORD(dwMinorVersion, dwMajorVersion), 0);
}

HWND WINAPI
User32CreateWindowEx(DWORD dwExStyle,
                     LPCSTR lpClassName,
                     LPCSTR lpWindowName,
                     DWORD dwStyle,
                     int x,
                     int y,
                     int nWidth,
                     int nHeight,
                     HWND hWndParent,
                     HMENU hMenu,
                     HINSTANCE hInstance,
                     LPVOID lpParam,
                     DWORD dwFlags)
{
    LARGE_STRING WindowName;
    LARGE_STRING lstrClassName, *plstrClassName;
    LARGE_STRING lstrClassVersion, *plstrClassVersion;
    UNICODE_STRING ClassName;
    UNICODE_STRING ClassVersion;
    WNDCLASSEXA wceA;
    WNDCLASSEXW wceW;
    HMODULE hLibModule = NULL;
    DWORD dwLastError;
    BOOL Unicode, ClassFound = FALSE;
    HWND Handle = NULL;
    LPCWSTR lpszClsVersion;
    LPCWSTR lpLibFileName = NULL;
    HANDLE pCtx = NULL;
    DWORD dwFlagsVer;

#if 0
    DbgPrint("[window] User32CreateWindowEx style %d, exstyle %d, parent %d\n", dwStyle, dwExStyle, hWndParent);
#endif

    dwFlagsVer = RtlGetExpWinVer( hInstance ? hInstance : GetModuleHandleW(NULL) );
    TRACE("Module Version %x\n",dwFlagsVer);

    if (!RegisterDefaultClasses)
    {
        TRACE("RegisterSystemControls\n");
        RegisterSystemControls();
    }

    Unicode = !(dwFlags & NUCWE_ANSI);

    if (IS_ATOM(lpClassName))
    {
        plstrClassName = (PVOID)lpClassName;
    }
    else
    {
        if (Unicode)
        {
            RtlInitUnicodeString(&ClassName, (PCWSTR)lpClassName);
        }
        else
        {
            if (!RtlCreateUnicodeStringFromAsciiz(&ClassName, (PCSZ)lpClassName))
            {
                SetLastError(ERROR_OUTOFMEMORY);
                return NULL;
            }
        }

        /* Copy it to a LARGE_STRING */
        lstrClassName.Buffer = ClassName.Buffer;
        lstrClassName.Length = ClassName.Length;
        lstrClassName.MaximumLength = ClassName.MaximumLength;
        plstrClassName = &lstrClassName;
    }

    /* Initialize a LARGE_STRING */
    RtlInitLargeString(&WindowName, lpWindowName, Unicode);

    // HACK: The current implementation expects the Window name to be UNICODE
    if (!Unicode)
    {
        NTSTATUS Status;
        PSTR AnsiBuffer = WindowName.Buffer;
        ULONG AnsiLength = WindowName.Length;

        WindowName.Length = 0;
        WindowName.MaximumLength = AnsiLength * sizeof(WCHAR);
        WindowName.Buffer = RtlAllocateHeap(RtlGetProcessHeap(),
                                            0,
                                            WindowName.MaximumLength);
        if (!WindowName.Buffer)
        {
            SetLastError(ERROR_OUTOFMEMORY);
            goto cleanup;
        }

        Status = RtlMultiByteToUnicodeN(WindowName.Buffer,
                                        WindowName.MaximumLength,
                                        &WindowName.Length,
                                        AnsiBuffer,
                                        AnsiLength);
        if (!NT_SUCCESS(Status))
        {
            goto cleanup;
        }
    }

    if (!hMenu && (dwStyle & (WS_OVERLAPPEDWINDOW | WS_POPUP)))
    {
        if (Unicode)
        {
            wceW.cbSize = sizeof(wceW);
            if (GetClassInfoExW(hInstance, (LPCWSTR)lpClassName, &wceW) && wceW.lpszMenuName)
            {
                hMenu = LoadMenuW(hInstance, wceW.lpszMenuName);
            }
        }
        else
        {
            wceA.cbSize = sizeof(wceA);
            if (GetClassInfoExA(hInstance, lpClassName, &wceA) && wceA.lpszMenuName)
            {
                hMenu = LoadMenuA(hInstance, wceA.lpszMenuName);
            }
        }
    }

    if (!Unicode) dwExStyle |= WS_EX_SETANSICREATOR;

    lpszClsVersion = ClassNameToVersion(lpClassName, NULL, &lpLibFileName, &pCtx, !Unicode);
    if (!lpszClsVersion)
    {
        plstrClassVersion = plstrClassName;
    }
    else
    {
        RtlInitUnicodeString(&ClassVersion, lpszClsVersion);
        lstrClassVersion.Buffer = ClassVersion.Buffer;
        lstrClassVersion.Length = ClassVersion.Length;
        lstrClassVersion.MaximumLength = ClassVersion.MaximumLength;
        plstrClassVersion = &lstrClassVersion;
    }

    for (;;)
    {
        Handle = NtUserCreateWindowEx(dwExStyle,
                                      plstrClassName,
                                      plstrClassVersion,
                                      &WindowName,
                                      dwStyle,
                                      x,
                                      y,
                                      nWidth,
                                      nHeight,
                                      hWndParent,
                                      hMenu,
                                      hInstance,
                                      lpParam,
                                      dwFlagsVer,
                                      pCtx );
        if (Handle) break;
        if (!lpLibFileName) break;
        if (!ClassFound)
        {
            dwLastError = GetLastError();
            if (dwLastError == ERROR_CANNOT_FIND_WND_CLASS)
            {
                ClassFound = VersionRegisterClass(ClassName.Buffer, lpLibFileName, pCtx, &hLibModule);
                if (ClassFound) continue;
            }
        }
        if (hLibModule)
        {
            dwLastError = GetLastError();
            FreeLibrary(hLibModule);
            SetLastError(dwLastError);
            hLibModule = NULL;
        }
        break;
    }

#if 0
    DbgPrint("[window] NtUserCreateWindowEx() == %d\n", Handle);
#endif

cleanup:
    if (!Unicode)
    {
        if (!IS_ATOM(lpClassName))
            RtlFreeUnicodeString(&ClassName);

        RtlFreeLargeString(&WindowName);
    }

    return Handle;
}


/*
 * @implemented
 */
HWND
WINAPI
DECLSPEC_HOTPATCH
CreateWindowExA(DWORD dwExStyle,
                LPCSTR lpClassName,
                LPCSTR lpWindowName,
                DWORD dwStyle,
                int x,
                int y,
                int nWidth,
                int nHeight,
                HWND hWndParent,
                HMENU hMenu,
                HINSTANCE hInstance,
                LPVOID lpParam)
{
    MDICREATESTRUCTA mdi;
    HWND hwnd;

    if (!RegisterDefaultClasses)
    {
       TRACE("CreateWindowExA RegisterSystemControls\n");
       RegisterSystemControls();
    }

    if (dwExStyle & WS_EX_MDICHILD)
    {
        POINT mPos[2];
        UINT id = 0;
        HWND top_child;
        PWND pWndParent;

        pWndParent = ValidateHwnd(hWndParent);

        if (!pWndParent) return NULL;

        if (pWndParent->fnid != FNID_MDICLIENT) // wine uses WIN_ISMDICLIENT
        {
           WARN("WS_EX_MDICHILD, but parent %p is not MDIClient\n", hWndParent);
           goto skip_mdi;
        }

        /* lpParams of WM_[NC]CREATE is different for MDI children.
        * MDICREATESTRUCT members have the originally passed values.
        */
        mdi.szClass = lpClassName;
        mdi.szTitle = lpWindowName;
        mdi.hOwner = hInstance;
        mdi.x = x;
        mdi.y = y;
        mdi.cx = nWidth;
        mdi.cy = nHeight;
        mdi.style = dwStyle;
        mdi.lParam = (LPARAM)lpParam;

        lpParam = (LPVOID)&mdi;

        if (pWndParent->style & MDIS_ALLCHILDSTYLES)
        {
            if (dwStyle & WS_POPUP)
            {
                WARN("WS_POPUP with MDIS_ALLCHILDSTYLES is not allowed\n");
                return(0);
            }
            dwStyle |= (WS_CHILD | WS_CLIPSIBLINGS);
        }
        else
        {
            dwStyle &= ~WS_POPUP;
            dwStyle |= (WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CAPTION |
                WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
        }

        top_child = GetWindow(hWndParent, GW_CHILD);

        if (top_child)
        {
            /* Restore current maximized child */
            if((dwStyle & WS_VISIBLE) && IsZoomed(top_child))
            {
                TRACE("Restoring current maximized child %p\n", top_child);
                SendMessageW( top_child, WM_SETREDRAW, FALSE, 0 );
                ShowWindow(top_child, SW_RESTORE);
                SendMessageW( top_child, WM_SETREDRAW, TRUE, 0 );
            }
        }

        MDI_CalcDefaultChildPos(hWndParent, -1, mPos, 0, &id);

        if (!(dwStyle & WS_POPUP)) hMenu = UlongToHandle(id);

        if (dwStyle & (WS_CHILD | WS_POPUP))
        {
            if (x == CW_USEDEFAULT || x == CW_USEDEFAULT16)
            {
                x = mPos[0].x;
                y = mPos[0].y;
            }
            if (nWidth == CW_USEDEFAULT || nWidth == CW_USEDEFAULT16 || !nWidth)
                nWidth = mPos[1].x;
            if (nHeight == CW_USEDEFAULT || nHeight == CW_USEDEFAULT16 || !nHeight)
                nHeight = mPos[1].y;
        }
    }

skip_mdi:
    hwnd = User32CreateWindowEx(dwExStyle,
                                lpClassName,
                                lpWindowName,
                                dwStyle,
                                x,
                                y,
                                nWidth,
                                nHeight,
                                hWndParent,
                                hMenu,
                                hInstance,
                                lpParam,
                                NUCWE_ANSI);
    return hwnd;
}


/*
 * @implemented
 */
HWND
WINAPI
DECLSPEC_HOTPATCH
CreateWindowExW(DWORD dwExStyle,
                LPCWSTR lpClassName,
                LPCWSTR lpWindowName,
                DWORD dwStyle,
                int x,
                int y,
                int nWidth,
                int nHeight,
                HWND hWndParent,
                HMENU hMenu,
                HINSTANCE hInstance,
                LPVOID lpParam)
{
    MDICREATESTRUCTW mdi;
    HWND hwnd;

    if (!RegisterDefaultClasses)
    {
       ERR("CreateWindowExW RegisterSystemControls\n");
       RegisterSystemControls();
    }

    if (dwExStyle & WS_EX_MDICHILD)
    {
        POINT mPos[2];
        UINT id = 0;
        HWND top_child;
        PWND pWndParent;

        pWndParent = ValidateHwnd(hWndParent);

        if (!pWndParent) return NULL;

        if (pWndParent->fnid != FNID_MDICLIENT)
        {
           WARN("WS_EX_MDICHILD, but parent %p is not MDIClient\n", hWndParent);
           goto skip_mdi;
        }

        /* lpParams of WM_[NC]CREATE is different for MDI children.
        * MDICREATESTRUCT members have the originally passed values.
        */
        mdi.szClass = lpClassName;
        mdi.szTitle = lpWindowName;
        mdi.hOwner = hInstance;
        mdi.x = x;
        mdi.y = y;
        mdi.cx = nWidth;
        mdi.cy = nHeight;
        mdi.style = dwStyle;
        mdi.lParam = (LPARAM)lpParam;

        lpParam = (LPVOID)&mdi;

        if (pWndParent->style & MDIS_ALLCHILDSTYLES)
        {
            if (dwStyle & WS_POPUP)
            {
                WARN("WS_POPUP with MDIS_ALLCHILDSTYLES is not allowed\n");
                return(0);
            }
            dwStyle |= (WS_CHILD | WS_CLIPSIBLINGS);
        }
        else
        {
            dwStyle &= ~WS_POPUP;
            dwStyle |= (WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CAPTION |
                WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
        }

        top_child = GetWindow(hWndParent, GW_CHILD);

        if (top_child)
        {
            /* Restore current maximized child */
            if((dwStyle & WS_VISIBLE) && IsZoomed(top_child))
            {
                TRACE("Restoring current maximized child %p\n", top_child);
                SendMessageW( top_child, WM_SETREDRAW, FALSE, 0 );
                ShowWindow(top_child, SW_RESTORE);
                SendMessageW( top_child, WM_SETREDRAW, TRUE, 0 );
            }
        }

        MDI_CalcDefaultChildPos(hWndParent, -1, mPos, 0, &id);

        if (!(dwStyle & WS_POPUP)) hMenu = UlongToHandle(id);

        if (dwStyle & (WS_CHILD | WS_POPUP))
        {
            if (x == CW_USEDEFAULT || x == CW_USEDEFAULT16)
            {
                x = mPos[0].x;
                y = mPos[0].y;
            }
            if (nWidth == CW_USEDEFAULT || nWidth == CW_USEDEFAULT16 || !nWidth)
                nWidth = mPos[1].x;
            if (nHeight == CW_USEDEFAULT || nHeight == CW_USEDEFAULT16 || !nHeight)
                nHeight = mPos[1].y;
        }
    }

skip_mdi:
    hwnd = User32CreateWindowEx(dwExStyle,
                                (LPCSTR)lpClassName,
                                (LPCSTR)lpWindowName,
                                dwStyle,
                                x,
                                y,
                                nWidth,
                                nHeight,
                                hWndParent,
                                hMenu,
                                hInstance,
                                lpParam,
                                0);
    return hwnd;
}

/*
 * @unimplemented
 */
HDWP WINAPI
DeferWindowPos(HDWP hWinPosInfo,
               HWND hWnd,
               HWND hWndInsertAfter,
               int x,
               int y,
               int cx,
               int cy,
               UINT uFlags)
{
    return NtUserDeferWindowPos(hWinPosInfo, hWnd, hWndInsertAfter, x, y, cx, cy, uFlags);
}


/*
 * @unimplemented
 */
BOOL WINAPI
EndDeferWindowPos(HDWP hWinPosInfo)
{
    return NtUserEndDeferWindowPosEx(hWinPosInfo, FALSE);
}


/*
 * @implemented
 */
HWND WINAPI
GetDesktopWindow(VOID)
{
    PWND Wnd;
    HWND Ret = NULL;

    _SEH2_TRY
    {
        Wnd = GetThreadDesktopWnd();
        if (Wnd != NULL)
            Ret = UserHMGetHandle(Wnd);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Do nothing */
    }
    _SEH2_END;

    return Ret;
}


static BOOL
User32EnumWindows(HDESK hDesktop,
                  HWND hWndparent,
                  WNDENUMPROC lpfn,
                  LPARAM lParam,
                  DWORD dwThreadId,
                  BOOL bChildren)
{
    DWORD i, dwCount = 0;
    HWND* pHwnd = NULL;
    HANDLE hHeap;
    NTSTATUS Status;

    if (!lpfn)
    {
        SetLastError ( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    /* FIXME instead of always making two calls, should we use some
       sort of persistent buffer and only grow it ( requiring a 2nd
       call ) when the buffer wasn't already big enough? */
    /* first get how many window entries there are */
    Status = NtUserBuildHwndList(hDesktop,
                                 hWndparent,
                                 bChildren,
                                 dwThreadId,
                                 dwCount,
                                 NULL,
                                 &dwCount);
    if (!NT_SUCCESS(Status))
        return FALSE;

    if (!dwCount)
    {
       if (!dwThreadId)
          return FALSE;
       else
          return TRUE;
    }

    /* allocate buffer to receive HWND handles */
    hHeap = GetProcessHeap();
    pHwnd = HeapAlloc(hHeap, 0, sizeof(HWND)*(dwCount+1));
    if (!pHwnd)
    {
        SetLastError ( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }

    /* now call kernel again to fill the buffer this time */
    Status = NtUserBuildHwndList(hDesktop,
                                 hWndparent,
                                 bChildren,
                                 dwThreadId,
                                 dwCount,
                                 pHwnd,
                                 &dwCount);
    if (!NT_SUCCESS(Status))
    {
        if (pHwnd)
            HeapFree(hHeap, 0, pHwnd);
        return FALSE;
    }

    /* call the user's callback function until we're done or
       they tell us to quit */
    for ( i = 0; i < dwCount; i++ )
    {
        /* FIXME I'm only getting NULLs from Thread Enumeration, and it's
         * probably because I'm not doing it right in NtUserBuildHwndList.
         * Once that's fixed, we shouldn't have to check for a NULL HWND
         * here
         * This is now fixed in revision 50205. (jt)
         */
        if (!pHwnd[i]) /* don't enumerate a NULL HWND */
            continue;
        if (!(*lpfn)(pHwnd[i], lParam))
        {
            HeapFree ( hHeap, 0, pHwnd );
            return FALSE;
        }
    }
    if (pHwnd)
        HeapFree(hHeap, 0, pHwnd);
    return TRUE;
}


/*
 * @implemented
 */
BOOL WINAPI
EnumChildWindows(HWND hWndParent,
                 WNDENUMPROC lpEnumFunc,
                 LPARAM lParam)
{
    if (!hWndParent)
    {
        return EnumWindows(lpEnumFunc, lParam);
    }
    return User32EnumWindows(NULL, hWndParent, lpEnumFunc, lParam, 0, TRUE);
}


/*
 * @implemented
 */
BOOL WINAPI
EnumThreadWindows(DWORD dwThreadId,
                  WNDENUMPROC lpfn,
                  LPARAM lParam)
{
    if (!dwThreadId)
        dwThreadId = GetCurrentThreadId();
    return User32EnumWindows(NULL, NULL, lpfn, lParam, dwThreadId, FALSE);
}


/*
 * @implemented
 */
BOOL WINAPI
EnumWindows(WNDENUMPROC lpEnumFunc,
            LPARAM lParam)
{
    return User32EnumWindows(NULL, NULL, lpEnumFunc, lParam, 0, FALSE);
}


/*
 * @implemented
 */
BOOL WINAPI
EnumDesktopWindows(HDESK hDesktop,
                   WNDENUMPROC lpfn,
                   LPARAM lParam)
{
    return User32EnumWindows(hDesktop, NULL, lpfn, lParam, 0, FALSE);
}


/*
 * @implemented
 */
HWND WINAPI
FindWindowExA(HWND hwndParent,
              HWND hwndChildAfter,
              LPCSTR lpszClass,
              LPCSTR lpszWindow)
{
    LPWSTR titleW = NULL;
    HWND hwnd = 0;

    if (lpszWindow)
    {
        DWORD len = MultiByteToWideChar( CP_ACP, 0, lpszWindow, -1, NULL, 0 );
        if (!(titleW = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) ))) return 0;
        MultiByteToWideChar( CP_ACP, 0, lpszWindow, -1, titleW, len );
    }

    if (!IS_INTRESOURCE(lpszClass))
    {
        WCHAR classW[256];
        if (MultiByteToWideChar( CP_ACP, 0, lpszClass, -1, classW, sizeof(classW)/sizeof(WCHAR) ))
            hwnd = FindWindowExW( hwndParent, hwndChildAfter, classW, titleW );
    }
    else
    {
        hwnd = FindWindowExW( hwndParent, hwndChildAfter, (LPCWSTR)lpszClass, titleW );
    }

    HeapFree( GetProcessHeap(), 0, titleW );
    return hwnd;
}


/*
 * @implemented
 */
HWND WINAPI
FindWindowExW(HWND hwndParent,
              HWND hwndChildAfter,
              LPCWSTR lpszClass,
              LPCWSTR lpszWindow)
{
    UNICODE_STRING ucClassName, *pucClassName = NULL;
    UNICODE_STRING ucWindowName, *pucWindowName = NULL;

    if (IS_ATOM(lpszClass))
    {
        ucClassName.Length = 0;
        ucClassName.Buffer = (LPWSTR)lpszClass;
        pucClassName = &ucClassName;
    }
    else if (lpszClass != NULL)
    {
        RtlInitUnicodeString(&ucClassName,
                             lpszClass);
        pucClassName = &ucClassName;
    }

    if (lpszWindow != NULL)
    {
        RtlInitUnicodeString(&ucWindowName,
                             lpszWindow);
        pucWindowName = &ucWindowName;
    }

    return NtUserFindWindowEx(hwndParent,
                              hwndChildAfter,
                              pucClassName,
                              pucWindowName,
                              0);
}


/*
 * @implemented
 */
HWND WINAPI
FindWindowA(LPCSTR lpClassName, LPCSTR lpWindowName)
{
    //FIXME: FindWindow does not search children, but FindWindowEx does.
    //       what should we do about this?
    return FindWindowExA (NULL, NULL, lpClassName, lpWindowName);
}


/*
 * @implemented
 */
HWND WINAPI
FindWindowW(LPCWSTR lpClassName, LPCWSTR lpWindowName)
{
    /*

    There was a FIXME here earlier, but I think it is just a documentation unclarity.

    FindWindow only searches top level windows. What they mean is that child
    windows of other windows than the desktop can be searched.
    FindWindowExW never does a recursive search.

    / Joakim
    */

    return FindWindowExW(NULL, NULL, lpClassName, lpWindowName);
}



/*
 * @implemented
 */
BOOL WINAPI
GetAltTabInfoA(HWND hwnd,
               int iItem,
               PALTTABINFO pati,
               LPSTR pszItemText,
               UINT cchItemText)
{
    return NtUserGetAltTabInfo(hwnd,iItem,pati,(LPWSTR)pszItemText,cchItemText,TRUE);
}


/*
 * @implemented
 */
BOOL WINAPI
GetAltTabInfoW(HWND hwnd,
               int iItem,
               PALTTABINFO pati,
               LPWSTR pszItemText,
               UINT cchItemText)
{
    return NtUserGetAltTabInfo(hwnd,iItem,pati,pszItemText,cchItemText,FALSE);
}


/*
 * @implemented
 */
HWND WINAPI
GetAncestor(HWND hwnd, UINT gaFlags)
{
    HWND Ret = NULL;
    PWND Ancestor, Wnd;

    Wnd = ValidateHwnd(hwnd);
    if (!Wnd)
        return NULL;

    _SEH2_TRY
    {
        Ancestor = NULL;
        switch (gaFlags)
        {
            case GA_PARENT:
                if (Wnd->spwndParent != NULL)
                    Ancestor = DesktopPtrToUser(Wnd->spwndParent);
                break;

            default:
                /* FIXME: Call win32k for now */
                Wnd = NULL;
                break;
        }

        if (Ancestor != NULL)
            Ret = UserHMGetHandle(Ancestor);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Do nothing */
    }
    _SEH2_END;

    if (!Wnd) /* Fall back */
        Ret = NtUserGetAncestor(hwnd, gaFlags);

    return Ret;
}


/*
 * @implemented
 */
BOOL WINAPI
GetClientRect(HWND hWnd, LPRECT lpRect)
{
    PWND Wnd = ValidateHwnd(hWnd);

    if (!Wnd) return FALSE;
    if (Wnd->style & WS_MINIMIZED)
    {
       lpRect->left = lpRect->top = 0;
       lpRect->right = GetSystemMetrics(SM_CXMINIMIZED);
       lpRect->bottom = GetSystemMetrics(SM_CYMINIMIZED);
       return TRUE;
    }
    if ( hWnd != GetDesktopWindow()) // Wnd->fnid != FNID_DESKTOP )
    {
/*        lpRect->left = lpRect->top = 0;
        lpRect->right = Wnd->rcClient.right - Wnd->rcClient.left;
        lpRect->bottom = Wnd->rcClient.bottom - Wnd->rcClient.top;
*/
        *lpRect = Wnd->rcClient;
        OffsetRect(lpRect, -Wnd->rcClient.left, -Wnd->rcClient.top);
    }
    else
    {
        lpRect->left = lpRect->top = 0;
        lpRect->right = Wnd->rcClient.right;
        lpRect->bottom = Wnd->rcClient.bottom;
/* Do this until Init bug is fixed. This sets 640x480, see InitMetrics.
        lpRect->right = GetSystemMetrics(SM_CXSCREEN);
        lpRect->bottom = GetSystemMetrics(SM_CYSCREEN);
*/    }
    return TRUE;
}


/*
 * @implemented
 */
HWND WINAPI
GetLastActivePopup(HWND hWnd)
{
    PWND Wnd;
    HWND Ret = hWnd;

    Wnd = ValidateHwnd(hWnd);
    if (Wnd != NULL)
    {
        _SEH2_TRY
        {
            if (Wnd->spwndLastActive)
            {
               PWND LastActive = DesktopPtrToUser(Wnd->spwndLastActive);
               Ret = UserHMGetHandle(LastActive);
            }
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Do nothing */
        }
        _SEH2_END;
    }
    return Ret;
}


/*
 * @implemented
 */
HWND WINAPI
GetParent(HWND hWnd)
{
    PWND Wnd, WndParent;
    HWND Ret = NULL;

    Wnd = ValidateHwnd(hWnd);
    if (Wnd != NULL)
    {
        _SEH2_TRY
        {
            WndParent = NULL;
            if (Wnd->style & WS_POPUP)
            {
                if (Wnd->spwndOwner != NULL)
                    WndParent = DesktopPtrToUser(Wnd->spwndOwner);
            }
            else if (Wnd->style & WS_CHILD)
            {
                if (Wnd->spwndParent != NULL)
                    WndParent = DesktopPtrToUser(Wnd->spwndParent);
            }

            if (WndParent != NULL)
                Ret = UserHMGetHandle(WndParent);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Do nothing */
        }
        _SEH2_END;
    }

    return Ret;
}


/*
 * @implemented
 */
BOOL WINAPI
GetProcessDefaultLayout(DWORD *pdwDefaultLayout)
{
return (BOOL)NtUserCallOneParam( (DWORD_PTR)pdwDefaultLayout, ONEPARAM_ROUTINE_GETPROCDEFLAYOUT);
}


/*
 * @implemented
 */
HWND WINAPI
GetWindow(HWND hWnd,
          UINT uCmd)
{
    PWND Wnd, FoundWnd;
    HWND Ret = NULL;

    Wnd = ValidateHwnd(hWnd);
    if (!Wnd)
        return NULL;

    _SEH2_TRY
    {
        FoundWnd = NULL;
        switch (uCmd)
        {
            case GW_OWNER:
                if (Wnd->spwndOwner != NULL)
                    FoundWnd = DesktopPtrToUser(Wnd->spwndOwner);
                break;

            case GW_HWNDFIRST:
                if(Wnd->spwndParent != NULL)
                {
                    FoundWnd = DesktopPtrToUser(Wnd->spwndParent);
                    if (FoundWnd->spwndChild != NULL)
                        FoundWnd = DesktopPtrToUser(FoundWnd->spwndChild);
                }
                break;
            case GW_HWNDNEXT:
                if (Wnd->spwndNext != NULL)
                    FoundWnd = DesktopPtrToUser(Wnd->spwndNext);
                break;

            case GW_HWNDPREV:
                if (Wnd->spwndPrev != NULL)
                    FoundWnd = DesktopPtrToUser(Wnd->spwndPrev);
                break;

            case GW_CHILD:
                if (Wnd->spwndChild != NULL)
                    FoundWnd = DesktopPtrToUser(Wnd->spwndChild);
                break;

            case GW_HWNDLAST:
                FoundWnd = Wnd;
                while ( FoundWnd->spwndNext != NULL)
                    FoundWnd = DesktopPtrToUser(FoundWnd->spwndNext);
                break;

            default:
                Wnd = NULL;
                break;
        }

        if (FoundWnd != NULL)
            Ret = UserHMGetHandle(FoundWnd);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Do nothing */
    }
    _SEH2_END;

    return Ret;
}


/*
 * @implemented
 */
HWND WINAPI
GetTopWindow(HWND hWnd)
{
    if (!hWnd) hWnd = GetDesktopWindow();
    return GetWindow(hWnd, GW_CHILD);
}


/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
GetWindowInfo(HWND hWnd,
              PWINDOWINFO pwi)
{
    PWND pWnd;
    PCLS pCls = NULL;
    SIZE Size = {0,0};
    BOOL Ret = FALSE;

    if ( !pwi || pwi->cbSize != sizeof(WINDOWINFO))
       SetLastError(ERROR_INVALID_PARAMETER); // Just set the error and go!

    pWnd = ValidateHwnd(hWnd);
    if (!pWnd)
        return Ret;

    UserGetWindowBorders(pWnd->style, pWnd->ExStyle, &Size, FALSE);

    _SEH2_TRY
    {
       pCls = DesktopPtrToUser(pWnd->pcls);
       pwi->rcWindow = pWnd->rcWindow;
       pwi->rcClient = pWnd->rcClient;
       pwi->dwStyle = pWnd->style;
       pwi->dwExStyle = pWnd->ExStyle;
       pwi->cxWindowBorders = Size.cx;
       pwi->cyWindowBorders = Size.cy;
       pwi->dwWindowStatus = 0;
       if (pWnd->state & WNDS_ACTIVEFRAME || (GetActiveWindow() == hWnd))
          pwi->dwWindowStatus = WS_ACTIVECAPTION;
       pwi->atomWindowType = (pCls ? pCls->atomClassName : 0 );

       if ( pWnd->state2 & WNDS2_WIN50COMPAT )
       {
          pwi->wCreatorVersion = 0x500;
       }
       else if ( pWnd->state2 & WNDS2_WIN40COMPAT )
       {
          pwi->wCreatorVersion = 0x400;
       }
       else if ( pWnd->state2 & WNDS2_WIN31COMPAT )
       {
          pwi->wCreatorVersion =  0x30A;
       }
       else
       {
          pwi->wCreatorVersion = 0x300;
       }

       Ret = TRUE;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Do nothing */
    }
    _SEH2_END;

   return Ret;
}


/*
 * @implemented
 */
UINT WINAPI
GetWindowModuleFileNameA(HWND hwnd,
                         LPSTR lpszFileName,
                         UINT cchFileNameMax)
{
    PWND Wnd = ValidateHwnd(hwnd);

    if (!Wnd)
        return 0;

    return GetModuleFileNameA(Wnd->hModule, lpszFileName, cchFileNameMax);
}


/*
 * @implemented
 */
UINT WINAPI
GetWindowModuleFileNameW(HWND hwnd,
                         LPWSTR lpszFileName,
                         UINT cchFileNameMax)
{
       PWND Wnd = ValidateHwnd(hwnd);

    if (!Wnd)
        return 0;

    return GetModuleFileNameW( Wnd->hModule, lpszFileName, cchFileNameMax );
}

/*
 * @implemented
 */
BOOL WINAPI
GetWindowRect(HWND hWnd,
              LPRECT lpRect)
{
    PWND Wnd = ValidateHwnd(hWnd);

    if (!Wnd) return FALSE;
    if ( hWnd != GetDesktopWindow()) // Wnd->fnid != FNID_DESKTOP )
    {
        *lpRect = Wnd->rcWindow;
    }
    else
    {
        lpRect->left = lpRect->top = 0;
        lpRect->right = Wnd->rcWindow.right;
        lpRect->bottom = Wnd->rcWindow.bottom;
/* Do this until Init bug is fixed. This sets 640x480, see InitMetrics.
        lpRect->right = GetSystemMetrics(SM_CXSCREEN);
        lpRect->bottom = GetSystemMetrics(SM_CYSCREEN);
*/    }
    return TRUE;
}

/*
 * @implemented
 */
int WINAPI
GetWindowTextA(HWND hWnd, LPSTR lpString, int nMaxCount)
{
    PWND Wnd;
    INT Length = 0;

    if (lpString == NULL || nMaxCount == 0)
        return 0;

    Wnd = ValidateHwnd(hWnd);
    if (!Wnd)
        return 0;

    lpString[0] = '\0';

    if (!TestWindowProcess(Wnd))
    {
        _SEH2_TRY
        {
            Length = DefWindowProcA(hWnd, WM_GETTEXT, nMaxCount, (LPARAM)lpString);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Length = 0;
        }
        _SEH2_END;
    }
    else
    {
        Length = SendMessageA(hWnd, WM_GETTEXT, nMaxCount, (LPARAM)lpString);
    }
    //ERR("GWTA Len %d : %s\n",Length,lpString);
    return Length;
}

/*
 * @implemented
 */
int WINAPI
GetWindowTextLengthA(HWND hWnd)
{
    PWND Wnd;

    Wnd = ValidateHwnd(hWnd);
    if (!Wnd)
        return 0;

    if (!TestWindowProcess(Wnd))
    {
        return DefWindowProcA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    }
    else
    {
        return SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    }
}

/*
 * @implemented
 */
int WINAPI
GetWindowTextLengthW(HWND hWnd)
{
    PWND Wnd;

    Wnd = ValidateHwnd(hWnd);
    if (!Wnd)
        return 0;

    if (!TestWindowProcess(Wnd))
    {
        return DefWindowProcW(hWnd, WM_GETTEXTLENGTH, 0, 0);
    }
    else
    {
        return SendMessageW(hWnd, WM_GETTEXTLENGTH, 0, 0);
    }
}

/*
 * @implemented
 */
int WINAPI
GetWindowTextW(HWND hWnd, LPWSTR lpString, int nMaxCount)
{
    PWND Wnd;
    INT Length = 0;

    if (lpString == NULL || nMaxCount == 0)
        return 0;

    Wnd = ValidateHwnd(hWnd);
    if (!Wnd)
        return 0;

    lpString[0] = L'\0';

    if (!TestWindowProcess(Wnd))
    {
        _SEH2_TRY
        {
            Length = DefWindowProcW(hWnd, WM_GETTEXT, nMaxCount, (LPARAM)lpString);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Length = 0;
        }
        _SEH2_END;
    }
    else
    {
        Length = SendMessageW(hWnd, WM_GETTEXT, nMaxCount, (LPARAM)lpString);
    }
    //ERR("GWTW Len %d : %S\n",Length,lpString);
    return Length;
}

DWORD WINAPI
GetWindowThreadProcessId(HWND hWnd,
                         LPDWORD lpdwProcessId)
{
    DWORD Ret = 0;
    PTHREADINFO ti;
    PWND pWnd = ValidateHwnd(hWnd);

    if (!pWnd) return Ret;

    ti = pWnd->head.pti;

    if (ti)
    {
        if (ti == GetW32ThreadInfo())
        { // We are current.
          //FIXME("Current!\n");
            if (lpdwProcessId)
                *lpdwProcessId = (DWORD_PTR)NtCurrentTeb()->ClientId.UniqueProcess;
            Ret = (DWORD_PTR)NtCurrentTeb()->ClientId.UniqueThread;
        }
        else
        { // Ask kernel for info.
          //FIXME("Kernel call!\n");
            if (lpdwProcessId)
                *lpdwProcessId = NtUserQueryWindow(hWnd, QUERY_WINDOW_UNIQUE_PROCESS_ID);
            Ret = NtUserQueryWindow(hWnd, QUERY_WINDOW_UNIQUE_THREAD_ID);
        }
    }
    return Ret;
}


/*
 * @implemented
 */
BOOL WINAPI
IsChild(HWND hWndParent,
    HWND hWnd)
{
    PWND WndParent, DesktopWnd,  Wnd;
    BOOL Ret = FALSE;

    WndParent = ValidateHwnd(hWndParent);
    if (!WndParent)
        return FALSE;
    Wnd = ValidateHwnd(hWnd);
    if (!Wnd)
        return FALSE;

    DesktopWnd = GetThreadDesktopWnd();
    if (!DesktopWnd)
        return FALSE;

    _SEH2_TRY
    {
        while (Wnd != NULL && ((Wnd->style & (WS_POPUP|WS_CHILD)) == WS_CHILD))
        {
            if (Wnd->spwndParent != NULL)
            {
                Wnd = DesktopPtrToUser(Wnd->spwndParent);

                if (Wnd == WndParent)
                {
                    Ret = TRUE;
                    break;
                }
            }
            else
                break;
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Do nothing */
    }
    _SEH2_END;

    return Ret;
}


/*
 * @implemented
 */
BOOL WINAPI
IsIconic(HWND hWnd)
{
    PWND Wnd = ValidateHwnd(hWnd);

    if (Wnd != NULL)
        return (Wnd->style & WS_MINIMIZE) != 0;

    return FALSE;
}


/*
 * @implemented
 */
BOOL WINAPI
IsWindow(HWND hWnd)
{
    PWND Wnd = ValidateHwndNoErr(hWnd);
    if (Wnd != NULL)
    {
        if (Wnd->state & WNDS_DESTROYED ||
            Wnd->state2 & WNDS2_INDESTROY)
           return FALSE;
        return TRUE;
    }

    return FALSE;
}


/*
 * @implemented
 */
BOOL WINAPI
IsWindowUnicode(HWND hWnd)
{
    PWND Wnd = ValidateHwnd(hWnd);

    if (Wnd != NULL)
        return Wnd->Unicode;

    return FALSE;
}


/*
 * @implemented
 */
BOOL WINAPI
IsWindowVisible(HWND hWnd)
{
    BOOL Ret = FALSE;
    PWND Wnd = ValidateHwnd(hWnd);

    if (Wnd != NULL)
    {
        _SEH2_TRY
        {
            Ret = TRUE;

            do
            {
                if (!(Wnd->style & WS_VISIBLE))
                {
                    Ret = FALSE;
                    break;
                }

                if (Wnd->spwndParent != NULL)
                    Wnd = DesktopPtrToUser(Wnd->spwndParent);
                else
                    break;

            } while (Wnd != NULL);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Ret = FALSE;
        }
        _SEH2_END;
    }

    return Ret;
}


/*
 * @implemented
 */
BOOL WINAPI
IsWindowEnabled(HWND hWnd)
{
    // AG: I don't know if child windows are affected if the parent is
    // disabled. I think they stop processing messages but stay appearing
    // as enabled.

    return !(GetWindowLongPtrW(hWnd, GWL_STYLE) & WS_DISABLED);
}


/*
 * @implemented
 */
BOOL WINAPI
IsZoomed(HWND hWnd)
{
    return (GetWindowLongPtrW(hWnd, GWL_STYLE) & WS_MAXIMIZE) != 0;
}


/*
 * @implemented
 */
BOOL WINAPI
LockSetForegroundWindow(UINT uLockCode)
{
    return NtUserxLockSetForegroundWindow(uLockCode);
}


/*
 * @implemented
 */
BOOL WINAPI
AnimateWindow(HWND hwnd,
              DWORD dwTime,
              DWORD dwFlags)
{
    /* FIXME Add animation code */

    /* If trying to show/hide and it's already   *
     * shown/hidden or invalid window, fail with *
     * invalid parameter                         */

    BOOL visible;
    visible = IsWindowVisible(hwnd);
    if(!IsWindow(hwnd) ||
       (visible && !(dwFlags & AW_HIDE)) ||
       (!visible && (dwFlags & AW_HIDE)))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    ShowWindow(hwnd, (dwFlags & AW_HIDE) ? SW_HIDE : ((dwFlags & AW_ACTIVATE) ? SW_SHOW : SW_SHOWNA));

    return TRUE;
}


/*
 * @implemented
 */
BOOL WINAPI
OpenIcon(HWND hWnd)
{
    if (!(GetWindowLongPtrW(hWnd, GWL_STYLE) & WS_MINIMIZE))
        return FALSE;

    ShowWindow(hWnd,SW_RESTORE);
    return TRUE;
}


/*
 * @implemented
 */
HWND WINAPI
RealChildWindowFromPoint(HWND hwndParent,
                         POINT ptParentClientCoords)
{
    return NtUserRealChildWindowFromPoint(hwndParent, ptParentClientCoords.x, ptParentClientCoords.y);
}

/*
 * @unimplemented
 */
BOOL WINAPI
SetForegroundWindow(HWND hWnd)
{
    return NtUserxSetForegroundWindow(hWnd);
}


/*
 * @implemented
 */
BOOL WINAPI
SetProcessDefaultLayout(DWORD dwDefaultLayout)
{
return NtUserCallOneParam( (DWORD_PTR)dwDefaultLayout, ONEPARAM_ROUTINE_SETPROCDEFLAYOUT);
}


/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
SetWindowTextA(HWND hWnd,
               LPCSTR lpString)
{
  PWND pwnd;

  pwnd = ValidateHwnd(hWnd);
  if (pwnd)
  {
     if (!TestWindowProcess(pwnd))
     {
        /* do not send WM_GETTEXT messages to other processes */
        return (DefWindowProcA(hWnd, WM_SETTEXT, 0, (LPARAM)lpString) >= 0);
     }
     return (SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)lpString) >= 0);
  }
  return FALSE;
}


/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
SetWindowTextW(HWND hWnd,
               LPCWSTR lpString)
{
  PWND pwnd;

  pwnd = ValidateHwnd(hWnd);
  if (pwnd)
  {
     if (!TestWindowProcess(pwnd))
     {
        /* do not send WM_GETTEXT messages to other processes */
        return (DefWindowProcW(hWnd, WM_SETTEXT, 0, (LPARAM)lpString) >= 0);
     }
     return (SendMessageW(hWnd, WM_SETTEXT, 0, (LPARAM)lpString) >= 0);
  }
  return FALSE;
}


/*
 * @implemented
 */
BOOL WINAPI
ShowOwnedPopups(HWND hWnd, BOOL fShow)
{
    return NtUserxShowOwnedPopups(hWnd, fShow);
}


/*
 * @implemented
 */
BOOL WINAPI
UpdateLayeredWindow( HWND hwnd,
                     HDC hdcDst,
                     POINT *pptDst,
                     SIZE *psize,
                     HDC hdcSrc,
                     POINT *pptSrc,
                     COLORREF crKey,
                     BLENDFUNCTION *pbl,
                     DWORD dwFlags)
{
  if (dwFlags & ULW_EX_NORESIZE)  /* only valid for UpdateLayeredWindowIndirect */
  {
     SetLastError( ERROR_INVALID_PARAMETER );
     return FALSE;
  }
  return NtUserUpdateLayeredWindow( hwnd,
                                    hdcDst,
                                    pptDst,
                                    psize,
                                    hdcSrc,
                                    pptSrc,
                                    crKey,
                                    pbl,
                                    dwFlags,
                                    NULL);
}

/*
 * @implemented
 */
BOOL WINAPI
UpdateLayeredWindowIndirect(HWND hwnd,
                            const UPDATELAYEREDWINDOWINFO *info)
{
  if (info && info->cbSize == sizeof(*info))
  {
     return NtUserUpdateLayeredWindow( hwnd,
                                       info->hdcDst,
                                       (POINT *)info->pptDst,
                                       (SIZE *)info->psize,
                                       info->hdcSrc,
                                       (POINT *)info->pptSrc,
                                       info->crKey,
                                       (BLENDFUNCTION *)info->pblend,
                                       info->dwFlags,
                                       (RECT *)info->prcDirty);
  }
  SetLastError(ERROR_INVALID_PARAMETER);
  return FALSE;
}

/*
 * @implemented
 */
BOOL WINAPI
SetWindowContextHelpId(HWND hwnd,
                       DWORD dwContextHelpId)
{
    return NtUserxSetWindowContextHelpId(hwnd, dwContextHelpId);
}

/*
 * @implemented
 */
DWORD WINAPI
GetWindowContextHelpId(HWND hwnd)
{
    return NtUserxGetWindowContextHelpId(hwnd);
}

/*
 * @implemented
 */
int WINAPI
InternalGetWindowText(HWND hWnd, LPWSTR lpString, int nMaxCount)
{
    INT Ret = NtUserInternalGetWindowText(hWnd, lpString, nMaxCount);
    if (Ret == 0 && lpString)
        *lpString = L'\0';
    return Ret;
}

/*
 * @implemented
 */
BOOL WINAPI
IsHungAppWindow(HWND hwnd)
{
    return !!NtUserQueryWindow(hwnd, QUERY_WINDOW_ISHUNG);
}

/*
 * @implemented
 */
VOID WINAPI
SetLastErrorEx(DWORD dwErrCode, DWORD dwType)
{
    SetLastError(dwErrCode);
}

/*
 * @implemented
 */
HWND WINAPI
GetFocus(VOID)
{
    return (HWND)NtUserGetThreadState(THREADSTATE_FOCUSWINDOW);
}

DWORD WINAPI
GetRealWindowOwner(HWND hwnd)
{
    return NtUserQueryWindow(hwnd, QUERY_WINDOW_REAL_ID);
}

/*
 * @implemented
 */
HWND WINAPI
SetTaskmanWindow(HWND hWnd)
{
    return NtUserxSetTaskmanWindow(hWnd);
}

/*
 * @implemented
 */
HWND WINAPI
SetProgmanWindow(HWND hWnd)
{
    return NtUserxSetProgmanWindow(hWnd);
}

/*
 * @implemented
 */
HWND WINAPI
GetProgmanWindow(VOID)
{
    return (HWND)NtUserGetThreadState(THREADSTATE_PROGMANWINDOW);
}

/*
 * @implemented
 */
HWND WINAPI
GetTaskmanWindow(VOID)
{
    return (HWND)NtUserGetThreadState(THREADSTATE_TASKMANWINDOW);
}

/*
 * @implemented
 */
BOOL WINAPI
ScrollWindow(HWND hWnd,
             int dx,
             int dy,
             CONST RECT *lpRect,
             CONST RECT *prcClip)
{
    return NtUserScrollWindowEx(hWnd,
                                dx,
                                dy,
                                lpRect,
                                prcClip,
                                0,
                                NULL,
                                (lpRect ? 0 : SW_SCROLLCHILDREN) | (SW_ERASE|SW_INVALIDATE|SW_SCROLLWNDDCE)) != ERROR;
}

/* ScrollWindow uses the window DC, ScrollWindowEx doesn't */

/*
 * @implemented
 */
INT WINAPI
ScrollWindowEx(HWND hWnd,
               int dx,
               int dy,
               CONST RECT *prcScroll,
               CONST RECT *prcClip,
               HRGN hrgnUpdate,
               LPRECT prcUpdate,
               UINT flags)
{
    if (flags & SW_SMOOTHSCROLL)
    {
       FIXME("SW_SMOOTHSCROLL not supported.\n");
       // Fall through....
    }
    return NtUserScrollWindowEx(hWnd,
                                dx,
                                dy,
                                prcScroll,
                                prcClip,
                                hrgnUpdate,
                                prcUpdate,
                                flags);
}

/*
 * @implemented
 */
BOOL WINAPI
AnyPopup(VOID)
{
    int i;
    BOOL retvalue;
    HWND *list = WIN_ListChildren( GetDesktopWindow() );

    if (!list) return FALSE;
    for (i = 0; list[i]; i++)
    {
        if (IsWindowVisible( list[i] ) && GetWindow( list[i], GW_OWNER )) break;
    }
    retvalue = (list[i] != 0);
    HeapFree( GetProcessHeap(), 0, list );
    return retvalue;
}

/*
 * @implemented
 */
BOOL WINAPI
IsWindowInDestroy(HWND hWnd)
{
    PWND pwnd;
    pwnd = ValidateHwnd(hWnd);
    if (!pwnd)
       return FALSE;
    return ((pwnd->state2 & WNDS2_INDESTROY) == WNDS2_INDESTROY);
}

/*
 * @implemented
 */
VOID WINAPI
DisableProcessWindowsGhosting(VOID)
{
    NtUserxEnableProcessWindowGhosting(FALSE);
}

/* EOF */

