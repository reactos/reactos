/*++

Copyright (c) 1994-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    indicdll.c

Abstract:

    This module implements the dll for handling the shell hooks for the
    multilingual language indicator.  It is also used for the on-screen
    keyboard, but it MUST be loaded by internat.exe.

Revision History:

--*/



//
//  Include Files.
//

#include "indicdll.h"


#ifdef WINNT
#define CONSOLE_WINDOW_CLASS      TEXT("ConsoleWindowClass")
#define CONSOLE_IME_WINDOW_CLASS  TEXT("ConsoleIMEClass")
#endif

#define INTERNATSHDATANAME        TEXT("InternatSHData")


LPVOID lpvSharedMem = NULL;              // pointer to shared memory
LPSHAREDDATA lpvSHDataHead = NULL;       // header pointer of SH data
HANDLE hMapObject = NULL;                // handle to file mapping


//
//  Function Prototypes.
//

LRESULT CALLBACK
IndicDll_ShellHookProc(
    int nCode,
    WPARAM wParam,
    LPARAM lParam);

LRESULT CALLBACK
IndicDll_KeyboardHookProc(
    int nCode,
    WPARAM wParam,
    LPARAM lParam);

LRESULT CALLBACK
IndicDll_CBTProc(
    int nCode,
    WPARAM wParam,
    LPARAM lParam);





////////////////////////////////////////////////////////////////////////////
//
//  IndicDll_DllMain
//
////////////////////////////////////////////////////////////////////////////

BOOL WINAPI IndicDll_DllMain(
    HINSTANCE hInstance,
    DWORD dwReason,
    LPVOID lpvReserved)
{
    switch (dwReason)
    {
        case ( DLL_PROCESS_ATTACH ) :
        {
            BOOL fInit = FALSE;

            ASSERT(NULL == hMapObject);
            ASSERT(NULL == lpvSharedMem);
            ASSERT(NULL == lpvSHDataHead);

            hMapObject = CreateFileMapping(
                            INVALID_HANDLE_VALUE,   // use paging file
                            NULL,                   // no security attributes
                            PAGE_READWRITE,         // read/write access
                            0,                      // size: high 32-bits
                            sizeof(SHAREDDATA),     // size: low 32-bits
                            INTERNATSHDATANAME);    // name of map object

            if (hMapObject == NULL)
                return FALSE;

            // The first process to attach initializes memory.

            fInit = (GetLastError() != ERROR_ALREADY_EXISTS);

            // Get a pointer to the file-mapped shared memory.

            lpvSharedMem = MapViewOfFile(
                              hMapObject,     // object to map view of
                              FILE_MAP_WRITE, // read/write access
                              0,              // high offset:  map from
                              0,              // low offset:   beginning
                              0);             // default: map entire file

            if (lpvSharedMem == NULL)
                return FALSE;

            lpvSHDataHead = (LPSHAREDDATA) lpvSharedMem;

            if (fInit)
            {
                memset(lpvSharedMem, '\0', sizeof(SHAREDDATA));

                lpvSHDataHead->hinstDLL = hInstance;
                lpvSHDataHead->iShellActive  = 0;
            }

            break;
        }

        case ( DLL_PROCESS_DETACH ) :
        {
            // Unmap shared memory from the process's address space.
            if (lpvSharedMem != NULL)
            {
                UnmapViewOfFile(lpvSharedMem);
                lpvSHDataHead = lpvSharedMem = NULL;
            }

            // Close the process's handle to the file-mapping object.
            if (hMapObject != NULL)
            {
                CloseHandle(hMapObject);
                hMapObject = NULL;
            }

            break;
        }
    }
    return (TRUE);

    UNREFERENCED_PARAMETER(lpvReserved);
}

IndicDll_IsNamedWindow(HWND hwnd, LPTSTR pszClassIn)
{
    int cch;
    TCHAR szClass[32];

    cch = GetClassName(hwnd, szClass, sizeof(szClass) / sizeof(TCHAR) - 1);
    szClass[cch] = 0;
    return (lstrcmp(szClass, pszClassIn) == 0);
}

//***   IndicDll_IsTasksWindow -- is this the 'tasks' band
// NOTES
//  kind of rude, we really shouldn't know about the tray's internals.
// possibly we should make SetNotifyWnd pass this stuff in.
#define TASKS_WINDOW_CLASS      TEXT("MSTaskSwWClass")

BOOL IndicDll_IsTasksWindow(HWND hwnd)
{
    ASSERT(lpvSHDataHead);

    // 1st, try cache
    if (lpvSHDataHead->hwndTasks != NULL) {
        ASSERT((hwnd == lpvSHDataHead->hwndTasks) == IndicDll_IsNamedWindow(hwnd, TASKS_WINDOW_CLASS));
        return (hwnd == lpvSHDataHead->hwndTasks);
    }

    // 2nd, the hard way
    if (IndicDll_IsNamedWindow(hwnd, TASKS_WINDOW_CLASS)) {
        lpvSHDataHead->hwndTasks = hwnd;       // update cache (clear in SetNotifyWnd)
        return TRUE;
    }

    return FALSE;
}

//***   IsExplorerTaskBarWnd -- is this 'special' window
// DESCRIPTION
//  normally when we change to a new thread we change to that thread's
// IME (and update the notify icon).  in fact we usually want to do this
// for the tray too (e.g. if you click in an addr band, we want to use
// whatever IME you set).
//  however we *don't* want to do that when clicking on the notify icon
// itself, since the reason you do that is to bring up a menu *for the
// current app*, so you want the icon to refer to the 'old current'
// (previous) thread not the 'new current' (current) thread.  (got that?).
//  but, clicking on the notify icon causes a SetFocus to the Tray, and
// the Tray propagates that down to the 'tasks' band.
//  so somehow we need to filter these guys out.
//  well, when there was no addr band we could simply filter out *all*
// things below the tray.  not so w/ the addr band.
//  so what to do?  well, 1st we narrow down the old check to ignore the
// notify window (which is what you 'really' want).
//  but because of the SetFocus to the tray (and the propagation down to 
// 'tasks', we 2nd have to filter those out (HACK HACK).  to do that we
// filter out anything below the 'tasks' window (for propagated SetFocus),
// and also filter out the 'tray' if it's top-level (for a 'direct' SetFocus).
// NOTES
//  this still leaves some 'narrow' bugs.  e.g. when you click on the tasks
// band 'for real', we stay in the 'old' language.  ditto for clicking on
// the clock (or anything else we might add to the notify area).  however
// since tasks doesn't take input, and clock launches a separate app for
// input, these bugs should be harmless.
//
// ***  IsWndOnNotifyWnd() for NT2000
// Old code doesn't change any language updating for all TaskBar focus.
// But new code accecpt all language changing request except Notify window area
// by IsWndOnNOtifyWnd().
// In NT2000, there isn't the proper way to filter notify window from TaskBar.
// (while user is clicking the language indicator icon to change language)
// Since there are only TaskBar or SysTabControl32 focus window setting instead
// of Notify window.
//

BOOL IsWndOnNotifyWnd(HWND hwnd)
{
    DWORD dwCurPos;
    POINT pt;
    RECT rcTrayNotify;
    BOOL fTaskBar = FALSE;
    HWND hwndDesktop = GetDesktopWindow();

    ASSERT(lpvSHDataHead);

    if (hwnd == lpvSHDataHead->hwndTaskBar)
    {
        fTaskBar = TRUE;
    }
    else
    {
        while (hwnd != NULL && hwnd != hwndDesktop)
        {
            if (hwnd == lpvSHDataHead->hwndNotify || IndicDll_IsTasksWindow(hwnd))
            {
                fTaskBar = TRUE;
            }
            hwnd = GetParent(hwnd);
        }
    }

    if (fTaskBar)
    {
        // If the current focus is on TaskBar, check the current mouse cursor
        // position on Notify window area. This is for filtering notify window
        // focus.
        dwCurPos = GetMessagePos();
        pt.x = LOWORD(dwCurPos);
        pt.y = HIWORD(dwCurPos);

        GetWindowRect(lpvSHDataHead->hwndNotify, &rcTrayNotify);

        if (PtInRect(&rcTrayNotify, pt))
             return TRUE;
    }

    return FALSE;
}

////////////////////////////////////////////////////////////////////////////
//
//  IndicDll_RegisterHookSendWindow
//
//  The hwnd can be zero to indicate the app is closing down.
//
////////////////////////////////////////////////////////////////////////////

BOOL IndicDll_RegisterHookSendWindow(
    HWND hwnd,
    BOOL bInternat)
{
    ASSERT(lpvSHDataHead);

    if (bInternat)
    {
        lpvSHDataHead->hwndInternat = hwnd;
    }
    else
    {
        if (hwnd)
        {
            lpvSHDataHead->hookKbd = SetWindowsHookEx( WH_KEYBOARD,
                                                   IndicDll_KeyboardHookProc,
                                                   lpvSHDataHead->hinstDLL,
                                                   0 );
        }
        else
        {
            UnhookWindowsHookEx(lpvSHDataHead->hookKbd);
        }
        lpvSHDataHead->hwndOSK = hwnd;
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  IndicDll_StartShell
//
////////////////////////////////////////////////////////////////////////////

BOOL IndicDll_StartShell()
{
    ASSERT(lpvSHDataHead);

    if (!(lpvSHDataHead->hwndInternat))
    {
        return (FALSE);
    }

    if (!(lpvSHDataHead->iShellActive))
    {
        lpvSHDataHead->hookShell = SetWindowsHookEx( WH_SHELL,
                                                     IndicDll_ShellHookProc,
                                                     lpvSHDataHead->hinstDLL,
                                                     0 );
        lpvSHDataHead->hookCBT   = SetWindowsHookEx( WH_CBT,
                                                     IndicDll_CBTProc,
                                                     lpvSHDataHead->hinstDLL,
                                                     0 );
        lpvSHDataHead->iShellActive = 1;
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  IndicDll_StopShell
//
////////////////////////////////////////////////////////////////////////////

BOOL IndicDll_StopShell()
{
    ASSERT(lpvSHDataHead);

    if (lpvSHDataHead->iShellActive)
    {
        UnhookWindowsHookEx(lpvSHDataHead->hookShell);
        UnhookWindowsHookEx(lpvSHDataHead->hookCBT);
        (lpvSHDataHead->iShellActive)--;
    }

    return (TRUE);
}

BOOL IndicDll_IsConsoleWindow(HWND hwnd)
{
    return IndicDll_IsNamedWindow(hwnd, CONSOLE_WINDOW_CLASS);
}

////////////////////////////////////////////////////////////////////////////
//
//  IndicDll_shellWindowActivated
//
////////////////////////////////////////////////////////////////////////////

void IndicDll_shellWindowActivated(
    WPARAM wParam,
    LPARAM lParam)
{
    HWND hwndFocus;
    DWORD dwTidFocus;
    DWORD dwProcessId;
    HWND hwndActivated = (HWND)wParam;

    ASSERT(lpvSHDataHead);

    if (wParam)
    {
        //
        //  No tray.
        //
        if (lpvSHDataHead->hwndInternat != NULL)
        {
            hwndFocus = GetFocus();
            if (hwndFocus && IsWindow(hwndFocus))
            {
                //
                //  If this hook is called within 16bit task, GetFocus
                //  would return a foreground window in the other task
                //  when the 16bit task doesn't have a focus.
                //
                dwTidFocus = GetWindowThreadProcessId(hwndFocus, &dwProcessId);
                if (dwTidFocus == GetCurrentThreadId())
                {
                    wParam = (LPARAM)hwndFocus;
                }
                else
                {
                    hwndFocus = (HWND)NULL;
                }
            }

#ifdef WINNT
            //
            //  Console window doesn't hook WH_CBT.
            //  Should set console window handle in hwndLastFocus.
            //
            if (IndicDll_IsConsoleWindow((HWND)wParam) == TRUE)
            {
                lpvSHDataHead->hwndLastFocus = (HWND)wParam;
            }
#endif
        }

        if (lpvSHDataHead->hwndOSK != NULL)
        {
            SendMessage(lpvSHDataHead->hwndOSK, WM_MYWINDOWACTIVATED, wParam, lParam);
        }
    }

    //
    //  Try to save the latest status for IME but not for notify window.
    //
    if ((wParam) && IsWindow((HWND)wParam) &&
        !IsWndOnNotifyWnd((HWND)wParam) &&
        ((HWND)wParam != lpvSHDataHead->hwndInternat))
    {
        DWORD dwProcessId;
        HKL hklNew, hklNotify, hklInternat;

        //
        //  Save the last active window because focus window can be destroyed
        //  before internat.exe uses it. This is still bogus because even if
        //  it's not killed, it may have been changed at the time internat
        //  makes use of it for SetForeGroundWindow.
        //
        if (IsWindow(hwndActivated) &&
            (hwndActivated != lpvSHDataHead->hwndInternat) &&
            !IsWndOnNotifyWnd(hwndActivated))
        {
            lpvSHDataHead->hwndLastActive = (HWND)hwndActivated;
        }

        if (hwndFocus && IsWindow(hwndFocus) && !IsWndOnNotifyWnd(hwndFocus))
        {
            IndicDll_SaveIMEStatus((HWND)hwndFocus);

            hklNew = GetKeyboardLayout(dwTidFocus);

            hklNotify = GetKeyboardLayout(
                              GetWindowThreadProcessId( lpvSHDataHead->hwndNotify,
                                                        &dwProcessId ) );
            hklInternat = GetKeyboardLayout(
                              GetWindowThreadProcessId( lpvSHDataHead->hwndInternat,
                                                        &dwProcessId ) );

            //
            //  If the current focus window has the same layout as
            //  hwndInternat, we may lose HSHELL_LANGUAGE for this.
            //  The last HSHELL_LANGUAGE may have been sent to us (but we
            //  have ignored if it's for internat or notify window), so
            //  system will save the next hook callback.
            //
            // [interpretation of the comment above]
            // the system (user) intentionally skip sending HSHELL_LANGUAGE
            // to shell hooks, when a newly focused window has the same
            // keyboard layout as the last focus window.
            // However if the last focus window was one of those we intentionally
            // ignore to respond (like hwndInternat or hwndNotify), we have not
            // switched our indicator icon to the new layout.
            // So the code below intends to make this switching happen when
            // we get HSHELL_WINDOWACTIVATED.
            //
            if ((hklNew == hklInternat) || (hklNew == hklNotify))
            {
#ifndef WINNT
                if (ImmGetAppIMECompatFlags(GetCurrentThreadId()) &
                    IMECOMPAT_NOSENDLANGCHG)
                {
                    PostMessage(lpvSHDataHead->hwndInternat, WM_MYLANGUAGECHECK, 0, 0);
                }
                else
#endif
                {
                    // BugBug#350263 - Update hklLastFocus with new hkl
                    lpvSHDataHead->hklLastFocus = hklNew;

                    SendMessage( lpvSHDataHead->hwndInternat,
                                 WM_MYLANGUAGECHANGE,
                                 wParam,
                                 (LPARAM)hklNew );
                }
            }
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  IndicDll_shellWindowCreated
//
////////////////////////////////////////////////////////////////////////////

void IndicDll_shellWindowCreated(
    WPARAM wParam,
    LPARAM lParam)
{
    ASSERT(lpvSHDataHead);

#ifdef WINNT
    //
    //  Console window doesn't hook WH_CBT.
    //  Should set console window handle in hwndLastFocus.
    //
    if (IndicDll_IsConsoleWindow((HWND)wParam) == TRUE)
    {
        lpvSHDataHead->hwndLastFocus = (HWND)wParam;
    }
#endif

    if (lpvSHDataHead->hwndOSK != NULL)
    {
        SendMessage(lpvSHDataHead->hwndOSK, WM_MYWINDOWCREATED, wParam, lParam);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  IndicDll_GetLastActiveWnd
//
////////////////////////////////////////////////////////////////////////////

HWND IndicDll_GetLastActiveWnd(void)
{
    ASSERT(lpvSHDataHead);

    return (lpvSHDataHead->hwndLastActive);
}


////////////////////////////////////////////////////////////////////////////
//
//  IndicDll_GetLastFocusWnd
//
////////////////////////////////////////////////////////////////////////////

HWND IndicDll_GetLastFocusWnd(void)
{
    ASSERT(lpvSHDataHead);

    return (lpvSHDataHead->hwndLastFocus);
}


////////////////////////////////////////////////////////////////////////////
//
//  IndicDll_SetNotifyWnd
//
////////////////////////////////////////////////////////////////////////////

void IndicDll_SetNotifyWnd(
    const struct NotifyWindows* nw)
{
    ASSERT(lpvSHDataHead);

    if (nw->cbSize != sizeof *nw) {
        return;
    }
    lpvSHDataHead->hwndNotify = nw->hwndNotify;
    lpvSHDataHead->hwndTaskBar = nw->hwndTaskBar;
    lpvSHDataHead->hwndTasks = NULL;           // clear cache

}


////////////////////////////////////////////////////////////////////////////
//
//  IndicDll_GetLayout
//
////////////////////////////////////////////////////////////////////////////

HKL IndicDll_GetLayout(void)
{
    ASSERT(lpvSHDataHead);

    return (lpvSHDataHead->hklLastFocus);
}


////////////////////////////////////////////////////////////////////////////
//
//  IndicDll_GetIMEStatus
//
////////////////////////////////////////////////////////////////////////////

int IndicDll_GetIMEStatus(void)
{
    ASSERT(lpvSHDataHead);

    return (lpvSHDataHead->iIMEStatForLastFocus);
}


////////////////////////////////////////////////////////////////////////////
//
//  IndicDll_GetConsoleImeWnd
//
////////////////////////////////////////////////////////////////////////////

HWND IndicDll_GetConsoleImeWnd(void)
{
    ASSERT(lpvSHDataHead);

    return lpvSHDataHead->hwndConsoleIme;
}

////////////////////////////////////////////////////////////////////////////
//
//  IndicDll_SaveIMEStatus
//
////////////////////////////////////////////////////////////////////////////

void IndicDll_SaveIMEStatus(
    HWND hwnd)
{
    DWORD dwProcessId;
    HIMC himc;
    DWORD dwConvMode, dwSentence;

    ASSERT(lpvSHDataHead);

    if (hwnd && ImmGetDefaultIMEWnd(hwnd) && !IsWndOnNotifyWnd(hwnd))
    {
        GetWindowThreadProcessId(hwnd, &dwProcessId);

        if (dwProcessId != GetCurrentProcessId())
        {
            //
            //  Can't access input context.
            //
            return;
        }

        himc = ImmGetContext(hwnd);

#ifdef WINNT
        //
        //  This hook is called by ImmSetActiveContextConsoleIME on ConIME.
        //  Should not set window handle in hwndLastFocus.
        //
        {
            int cch;
            TCHAR ClassName[32];

            cch = GetClassName( hwnd,
                                ClassName,
                                sizeof(ClassName) / sizeof(TCHAR) - 1 );
            ClassName[cch] = 0;

            if (lstrcmp(ClassName, CONSOLE_IME_WINDOW_CLASS) != 0)
            {
                lpvSHDataHead->hwndLastFocus = hwnd;
            }
            else
            {
                lpvSHDataHead->hwndConsoleIme = hwnd;
            }
        }
#else
        lpvSHDataHead->hwndLastFocus = hwnd;
#endif

        if (himc)
        {
            //
            //  Enabled.
            //
            if (ImmGetOpenStatus(himc))
            {
                lpvSHDataHead->iIMEStatForLastFocus = IMESTAT_OPEN;
            }
            else
            {
                lpvSHDataHead->iIMEStatForLastFocus = IMESTAT_CLOSE;
            }

            //
            //  Currently, only Korean version has an interest in this info.
            //  Because the app's hkl could still be previous locale between
            //  transition of two layouts, we'd like to check system ACP
            //  instead of process hkl.
            //
            if (GetACP() == 949)
            {
                if (ImmGetConversionStatus(himc, &dwConvMode, &dwSentence))
                {
                    if (dwConvMode & IME_CMODE_NATIVE)
                    {
                        lpvSHDataHead->iIMEStatForLastFocus |= IMESTAT_NATIVE;
                    }
                    if (dwConvMode & IME_CMODE_FULLSHAPE)
                    {
                        lpvSHDataHead->iIMEStatForLastFocus |= IMESTAT_FULLSHAPE;
                    }
                }
            }
            ImmReleaseContext(hwnd, himc);
        }
        else
        {
            //
            //  Disabled.
            //
            lpvSHDataHead->iIMEStatForLastFocus = IMESTAT_DISABLED;
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  IndicDll_ShellHookProc
//
////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK IndicDll_ShellHookProc(
    int nCode,
    WPARAM wParam,
    LPARAM lParam)
{
    if (!lpvSHDataHead)
    {
        return E_FAIL;
    }

    switch (nCode)
    {
        case ( HSHELL_LANGUAGE ) :
        {
            //
            //  Try to save the latest status for IME but not for notify wnd.
            //
            if ((HWND)wParam == NULL) {
                // On NT, mostly wParam is NULL.
                HWND hwndFocus = GetFocus();

                // a console window maynot have a focus window
                // but needs language change as well...
                if (!hwndFocus && IndicDll_IsConsoleWindow(lpvSHDataHead->hwndLastActive))
                {
                    hwndFocus = lpvSHDataHead->hwndLastActive;
                }

                if (hwndFocus && IsWindow(hwndFocus))
                {
                    if (hwndFocus == lpvSHDataHead->hwndLastFocus)
                        wParam = (WPARAM)lpvSHDataHead->hwndLastFocus;
                    else
                    {
                        // BugBug#304762 - Some application can't move the real
                        // focus. So if the current focus is the different with
                        // LastFocus, we update it with the current focus.
                        wParam = (WPARAM) hwndFocus;
                    }
                }
            }

            if ((HWND)wParam == lpvSHDataHead->hwndInternat || IsWndOnNotifyWnd((HWND)wParam))
            {
                break;
            }

            if ((HWND)wParam != lpvSHDataHead->hwndInternat)
            {
                IndicDll_SaveIMEStatus((HWND)wParam);
            }

            if ((HWND)wParam == lpvSHDataHead->hwndLastFocus)
            {
                lpvSHDataHead->hklLastFocus = (HKL)lParam;
            }

#ifndef WINNT
            if ((ImmGetAppIMECompatFlags(GetCurrentThreadId()) &
                 IMECOMPAT_NOSENDLANGCHG) &&
                 (lpvSHDataHead->hwndInternat != NULL))
            {
                PostMessage(lpvSHDataHead->hwndInternat, WM_MYLANGUAGECHECK, 0, 0);
            }
            else
#endif
            {
                if (lpvSHDataHead->hwndInternat != NULL)
                {
                    SendMessage(lpvSHDataHead->hwndInternat, WM_MYLANGUAGECHANGE, wParam, lParam);
                }
            }
            if (lpvSHDataHead->hwndOSK != NULL)
            {
                SendMessage(lpvSHDataHead->hwndOSK, WM_MYLANGUAGECHANGE, wParam, lParam);
            }

            break;
        }
        case ( HSHELL_WINDOWACTIVATED ) :
        {
            IndicDll_shellWindowActivated(wParam, lParam);
            break;
        }
        case ( HSHELL_WINDOWCREATED ) :
        {
            IndicDll_shellWindowCreated(wParam, lParam);
            break;
        }
    }

    return (CallNextHookEx(lpvSHDataHead->hookShell, nCode, wParam, lParam));
}


////////////////////////////////////////////////////////////////////////////
//
//  IndicDll_KeyboardHookProc
//
////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK IndicDll_KeyboardHookProc(
    int nCode,
    WPARAM wParam,
    LPARAM lParam)
{
    if (!lpvSHDataHead)
    {
        return E_FAIL;
    }

    if (nCode >= 0)
    {
        SendMessage( lpvSHDataHead->hwndOSK,
                     (lParam & 0x80000000) ? WM_KEYUP : WM_KEYDOWN,
                     wParam,
                     lParam );
    }

    return (CallNextHookEx(lpvSHDataHead->hookKbd, nCode, wParam, lParam));
}


////////////////////////////////////////////////////////////////////////////
//
//  IndicDll_CBTProc
//
////////////////////////////////////////////////////////////////////////////


VOID IndicDll_CBTSetFocus(HWND hwnd)
{
    DWORD dwTidFocus;
    HKL hklFocus;

    ASSERT(lpvSHDataHead);

    // If the window blongs to Internat itself, or if it's a window
    // on the shell's taskbar, just ignore them.
    if (hwnd == lpvSHDataHead->hwndInternat || IsWndOnNotifyWnd(hwnd)) {
        return;
    }

    // Remember the last focused window.
    lpvSHDataHead->hwndLastFocus = hwnd;

    dwTidFocus = GetWindowThreadProcessId(hwnd, NULL);

    // If the thread hasn't changed since last focus change,
    // further processing is not needed.
    if (dwTidFocus == lpvSHDataHead->dwTidLastFocus) {
        return;
    }

    // Remember the last focused thread id.
    lpvSHDataHead->dwTidLastFocus = dwTidFocus;

    // Get the hkl of the focused thread.
    hklFocus = GetKeyboardLayout(dwTidFocus);

    // If the hkl hasn't changed since last focus change,
    // just return here.
    if (hklFocus == lpvSHDataHead->hklLastFocus) {
        return;
    }

    //
    // Remember the new HKL and tell Internat.exe
    // to update the language information.
    //

    lpvSHDataHead->hklLastFocus = hklFocus;

    PostMessage(lpvSHDataHead->hwndInternat, WM_MYLANGUAGECHECK, (WPARAM)hwnd, (LPARAM)hklFocus);
}

LRESULT CALLBACK IndicDll_CBTProc(
    int nCode,
    WPARAM wParam,
    LPARAM lParam)
{
    if (!lpvSHDataHead)
    {
        return E_FAIL;
    }

    if (nCode >= 0) {
        switch (nCode) {
        case HCBT_ACTIVATE:
            if (((HWND)wParam != lpvSHDataHead->hwndInternat) &&
                    !IsWndOnNotifyWnd((HWND)wParam)) {
                lpvSHDataHead->hwndLastActive = (HWND)wParam;
            }
            break;

        case HCBT_SETFOCUS:
            IndicDll_CBTSetFocus((HWND)wParam);
            break;
        }
    }

    return (CallNextHookEx(lpvSHDataHead->hookCBT, nCode, wParam, lParam));
}
