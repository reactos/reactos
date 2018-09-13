/****************************** Module Header ******************************\
* Module Name: winmgrc.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains
*
* History:
* 20-Feb-1992 DarrinM   Pulled functions from user\server.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

#define CONSOLE_WINDOW_CLASS (L"ConsoleWindowClass")

/***************************************************************************\
* GetWindowWord (API)
*
* Return a window word.  Positive index values return application window words
* while negative index values return system window words.  The negative
* indices are published in WINDOWS.H.
*
* History:
* 20-Feb-1992 DarrinM   Wrote.
\***************************************************************************/

WORD GetWindowWord(
    HWND hwnd,
    int  index)
{
    PWND pwnd;

    pwnd = ValidateHwnd(hwnd);

    if (pwnd == NULL)
        return 0;

    /*
     * If it's a dialog window the window data is on the server side
     * We just call the "long" routine instead of have two thunks.
     * We know there is enough data if its DWLP_USER so we won't fault.
     */
    if (TestWF(pwnd, WFDIALOGWINDOW) && (index == DWLP_USER)) {
        return (WORD)_GetWindowLong(pwnd, index, FALSE);
    }

    return _GetWindowWord(pwnd, index);
}


BOOL FChildVisible(
    HWND hwnd)
{
    PWND pwnd;

    pwnd = ValidateHwnd(hwnd);

    if (pwnd == NULL)
        return 0;

    return (_FChildVisible(pwnd));
}

BOOL WINAPI AdjustWindowRectEx(
    LPRECT lpRect,
    DWORD dwStyle,
    BOOL bMenu,
    DWORD dwExStyle)
{
    ConnectIfNecessary();

    return _AdjustWindowRectEx(lpRect, dwStyle, bMenu, dwExStyle);
}


int WINAPI GetClassNameW(
    HWND hwnd,
    LPWSTR lpClassName,
    int nMaxCount)
{
    UNICODE_STRING strClassName;

    strClassName.MaximumLength = (USHORT)(nMaxCount * sizeof(WCHAR));
    strClassName.Buffer = lpClassName;
    return NtUserGetClassName(hwnd, FALSE, &strClassName);
}


HWND GetFocus(VOID)
{
    return (HWND)NtUserGetThreadState(UserThreadStateFocusWindow);
}


HWND GetCapture(VOID)
{
    /*
     * If no captures are currently taking place, just return NULL.
     */
    if (gpsi->cCaptures == 0) {
        return NULL;
    }
    return (HWND)NtUserGetThreadState(UserThreadStateCaptureWindow);
}

/***************************************************************************\
* AnyPopup (API)
*
*
*
* History:
* 12-Nov-1990 DarrinM   Ported.
\***************************************************************************/

BOOL AnyPopup(VOID)
{
    PWND pwnd = _GetDesktopWindow();

    for (pwnd = REBASEPWND(pwnd, spwndChild); pwnd; pwnd = REBASEPWND(pwnd, spwndNext)) {

        if ((pwnd->spwndOwner != NULL) && TestWF(pwnd, WFVISIBLE))
            return TRUE;
    }

    return FALSE;
}

/***************************************************************************\
* GetInputState
*
*
*
* History:
\***************************************************************************/

BOOL GetInputState(VOID)
{
    PCLIENTTHREADINFO pcti = GetClientInfo()->pClientThreadInfo;

    if ((pcti == NULL) || (pcti->fsChangeBits & (QS_MOUSEBUTTON | QS_KEY)))
        return (BOOL)NtUserGetThreadState(UserThreadStateInputState);

    return FALSE;
}

/***************************************************************************\
* MapWindowPoints
*
*
*
* History:
\***************************************************************************/

int MapWindowPoints(
    HWND    hwndFrom,
    HWND    hwndTo,
    LPPOINT lppt,
    UINT    cpt)
{
    PWND pwndFrom;
    PWND pwndTo;

    if (hwndFrom != NULL) {

        if ((pwndFrom = ValidateHwnd(hwndFrom)) == NULL)
            return 0;

    } else {

        pwndFrom = NULL;
    }

    if (hwndTo != NULL) {


        if ((pwndTo = ValidateHwnd(hwndTo)) == NULL)
            return 0;

    } else {

        pwndTo = NULL;
    }

    return _MapWindowPoints(pwndFrom, pwndTo, lppt, cpt);
}

/***************************************************************************\
* GetLastActivePopup
*
*
*
* History:
\***************************************************************************/

HWND GetLastActivePopup(
    HWND hwnd)
{
    PWND pwnd = ValidateHwnd(hwnd);

    if (pwnd == NULL)
        return NULL;

    pwnd = _GetLastActivePopup(pwnd);

    return HW(pwnd);
}

/**************************************************************************\
* PtiWindow
*
* Gets the PTHREADINFO of window or NULL if not valid.
*
* 12-Feb-1997 JerrySh   Created.
\**************************************************************************/

PTHREADINFO PtiWindow(
    HWND hwnd)
{
    PHE phe;
    DWORD dw;
    WORD uniq;

    dw = HMIndexFromHandle(hwnd);
    if (dw < gpsi->cHandleEntries) {
        phe = &gSharedInfo.aheList[dw];
        if ((phe->bType == TYPE_WINDOW) && !(phe->bFlags & HANDLEF_DESTROY)) {
            uniq = HMUniqFromHandle(hwnd);
            if (   uniq == phe->wUniq
#if !defined(_WIN64) && !defined(BUILD_WOW6432)
                || uniq == 0
                || uniq == HMUNIQBITS
#endif
                ) {
                return phe->pOwner;
            }
        }
    }
    UserSetLastError(ERROR_INVALID_WINDOW_HANDLE);
    return NULL;
}

/***************************************************************************\
* GetWindowThreadProcessId
*
* Get's windows process and thread ids.
*
* 24-Jun-1991 ScottLu   Created.
\***************************************************************************/

DWORD GetWindowThreadProcessId(
    HWND    hwnd,
    LPDWORD lpdwProcessId)
{
    PTHREADINFO ptiWindow;
    DWORD dwThreadId;

    if ((ptiWindow = PtiWindow(hwnd)) == NULL)
        return 0;

    /*
     * For non-system threads get the info from the thread info structure
     */
    if (ptiWindow == PtiCurrent()) {

        if (lpdwProcessId != NULL)
            *lpdwProcessId = HandleToUlong(NtCurrentTeb()->ClientId.UniqueProcess);
        dwThreadId = HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread);

    } else {

        /*
         * Make this better later on.
         */
        if (lpdwProcessId != NULL)
            *lpdwProcessId = HandleToUlong(NtUserQueryWindow(hwnd, WindowProcess));
        dwThreadId = HandleToUlong(NtUserQueryWindow(hwnd, WindowThread));
    }

    return dwThreadId;
}

/***************************************************************************\
* GetScrollPos
*
* Returns the current position of a scroll bar
*
* !!! WARNING a similiar copy of this code is in server\sbapi.c
*
* History:
\***************************************************************************/

int GetScrollPos(
    HWND hwnd,
    int  code)
{
    PWND pwnd;

    if ((pwnd = ValidateHwnd(hwnd)) == NULL)
        return 0;

    switch (code) {
    case SB_CTL:
        return (int)SendMessageWorker(pwnd, SBM_GETPOS, 0, 0, FALSE);

    case SB_HORZ:
    case SB_VERT:
        if (pwnd->pSBInfo != NULL) {
            PSBINFO pSBInfo = (PSBINFO)(REBASEALWAYS(pwnd, pSBInfo));
            return (code == SB_VERT) ? pSBInfo->Vert.pos : pSBInfo->Horz.pos;
        } else {
            RIPERR0(ERROR_NO_SCROLLBARS, RIP_VERBOSE, "");
        }
        break;

    default:
        /*
         * Win3.1 validation layer code.
         */
        RIPERR0(ERROR_INVALID_PARAMETER, RIP_VERBOSE, "");
    }

    return 0;
}

/***************************************************************************\
* GetScrollRange
*
* !!! WARNING a similiar copy of this code is in server\sbapi.c
*
* History:
* 16-May-1991 mikeke    Changed to return BOOL
\***************************************************************************/

BOOL GetScrollRange(
    HWND  hwnd,
    int   code,
    LPINT lpposMin,
    LPINT lpposMax)
{
    PSBINFO pSBInfo;
    PWND    pwnd;

    if ((pwnd = ValidateHwnd(hwnd)) == NULL)
        return FALSE;

    switch (code) {
    case SB_CTL:
        SendMessageWorker(pwnd, SBM_GETRANGE, (WPARAM)lpposMin, (LPARAM)lpposMax, FALSE);
        return TRUE;

    case SB_VERT:
    case SB_HORZ:
        if (pSBInfo = REBASE(pwnd, pSBInfo)) {
            PSBDATA pSBData;
            pSBData = (code == SB_VERT) ? &pSBInfo->Vert : &pSBInfo->Horz;
            *lpposMin = pSBData->posMin;
            *lpposMax = pSBData->posMax;
        } else {
            RIPERR0(ERROR_NO_SCROLLBARS, RIP_VERBOSE, "");
            *lpposMin = 0;
            *lpposMax = 0;
        }

        return TRUE;

    default:
        /*
         * Win3.1 validation layer code.
         */
        RIPERR0(ERROR_INVALID_PARAMETER, RIP_VERBOSE, "");
        return FALSE;
    }
}

/***************************************************************************\
* GetScrollInfo
*
* !!! WARNING a similiar copy of this code is in server\winmgrc.c
*
\***************************************************************************/

BOOL GetScrollInfo(
    HWND         hwnd,
    int          code,
    LPSCROLLINFO lpsi)
{
    PWND    pwnd;
    PSBINFO pSBInfo;
    PSBDATA pSBData;

    if (lpsi->cbSize != sizeof(SCROLLINFO)) {

        if (lpsi->cbSize != sizeof(SCROLLINFO) - 4) {
            RIPMSG0(RIP_WARNING, "SCROLLINFO: Invalid cbSize");
            return FALSE;

        } else {
            RIPMSG0(RIP_WARNING, "SCROLLINFO: Invalid cbSize");
        }
    }

    if (lpsi->fMask & ~SIF_MASK) {
        RIPMSG0(RIP_WARNING, "SCROLLINFO: Invalid fMask");
        return FALSE;
    }

    if ((pwnd = ValidateHwnd(hwnd)) == NULL)
        return FALSE;

    switch (code) {
    case SB_CTL:
        SendMessageWorker(pwnd, SBM_GETSCROLLINFO, 0, (LPARAM)lpsi, FALSE);
        return TRUE;

    case SB_HORZ:
    case SB_VERT:
        if (pwnd->pSBInfo == NULL) {
            RIPERR0(ERROR_NO_SCROLLBARS, RIP_VERBOSE, "");
            return FALSE;
        }

        /*
         * Rebase rgwScroll so probing will work
         */
        pSBInfo = (PSBINFO)REBASEALWAYS(pwnd, pSBInfo);

        pSBData = (code == SB_VERT) ? &pSBInfo->Vert : &pSBInfo->Horz;

        return(NtUserSBGetParms(hwnd, code, pSBData, lpsi));

    default:
        /*
         * Win3.1 validation layer code.
         */
        RIPERR0(ERROR_INVALID_PARAMETER, RIP_VERBOSE, "");
        return FALSE;
    }
}

/****************************************************************************\
* _GetActiveWindow (API)
*
*
* 23-Oct-1990 MikeHar   Ported from Windows.
* 12-Nov-1990 DarrinM   Moved from getset.c to here.
\****************************************************************************/

HWND GetActiveWindow(VOID)
{
    return (HWND)NtUserGetThreadState(UserThreadStateActiveWindow);
}

/****************************************************************************\
* GetCursor
*
*
* History:
\****************************************************************************/

HCURSOR GetCursor(VOID)
{
    return (HCURSOR)NtUserGetThreadState(UserThreadStateCursor);
}

/***************************************************************************\
* BOOL IsMenu(HMENU);
*
* Verifies that the handle passed in is a menu handle.
*
* Histroy:
* 10-Jul-1992 MikeHar   Created.
\***************************************************************************/

BOOL IsMenu(
   HMENU hMenu)
{
   if (HMValidateHandle(hMenu, TYPE_MENU))
      return TRUE;

   return FALSE;
}

/***************************************************************************\
* GetAppCompatFlags
*
* Compatibility flags for < Win 3.1 apps running on 3.1
*
* History:
* 01-Apr-1992 ScottLu   Created.
* 04-May-1992 DarrinM   Moved to USERRTL.DLL.
\***************************************************************************/

DWORD GetAppCompatFlags(
    PTHREADINFO pti)
{
    UNREFERENCED_PARAMETER(pti);

    ConnectIfNecessary();

    return GetClientInfo()->dwCompatFlags;
}

/***************************************************************************\
* GetAppCompatFlags2
*
* Compatibility flags for <= wVer apps.  Newer apps will get no hacks
* from this DWORD.
*
* History:
* 06-29-98 MCostea      Created.
\***************************************************************************/

DWORD GetAppCompatFlags2(
    WORD wVer)
{
    ConnectIfNecessary();
    /*
     * Newer apps should behave, so they get no hacks
     */
    if (wVer < GETAPPVER()) {
        return 0;
    }
    return GetClientInfo()->dwCompatFlags2;
}

/**************************************************************************\
* IsWindowUnicode
*
* 25-Feb-1992 IanJa     Created
\**************************************************************************/

BOOL IsWindowUnicode(
    IN HWND hwnd)
{
    PWND pwnd;


    if ((pwnd = ValidateHwnd(hwnd)) == NULL)
        return FALSE;

    return !TestWF(pwnd, WFANSIPROC);
}

/**************************************************************************\
* TestWindowProcess
*
* 14-Nov-1994 JimA      Created.
\**************************************************************************/

BOOL TestWindowProcess(
    PWND pwnd)
{
    /*
     * If the threads are the same, don't bother going to the kernel
     * to get the window's process id.
     */
    if (GETPTI(pwnd) == PtiCurrent()) {
        return TRUE;
    }

    return (GetWindowProcess(HW(pwnd)) == GETPROCESSID());
}

/**************************************************************************\
* IsHungAppWindow
*
* 11-14-94 JimA         Created.
\**************************************************************************/

BOOL IsHungAppWindow(
    HWND hwnd)
{
    return (NtUserQueryWindow(hwnd, WindowIsHung) != NULL);
}

/***************************************************************************\
* PtiCurrent
*
* Returns the THREADINFO structure for the current thread.
* LATER: Get DLL_THREAD_ATTACH initialization working right and we won't
*        need this connect code.
*
* History:
* 10-28-90 DavidPe      Created.
\***************************************************************************/

PTHREADINFO PtiCurrent(VOID)
{
    ConnectIfNecessary();
    return (PTHREADINFO)NtCurrentTebShared()->Win32ThreadInfo;
}


/***************************************************************************\
* _AdjustWindowRectEx (API)
*
*
*
* History:
* 10-24-90 darrinm      Ported from Win 3.0.
\***************************************************************************/

BOOL _AdjustWindowRectEx(
    LPRECT lprc,
    LONG style,
    BOOL fMenu,
    DWORD dwExStyle)
{
    //
    // Here we add on the appropriate 3D borders for old and new apps.
    //
    // Rules:
    //   (1) Do nothing for windows that have 3D border styles.
    //   (2) If the window has a dlgframe border (has a caption or is a
    //          a dialog), then add on the window edge style.
    //   (3) We NEVER add on the CLIENT STYLE.  New apps can create
    //          it if they want.  This is because it screws up alignment
    //          when the app doesn't know about it.
    //

    if (NeedsWindowEdge(style, dwExStyle, GETAPPVER() >= VER40))
        dwExStyle |= WS_EX_WINDOWEDGE;
    else
        dwExStyle &= ~WS_EX_WINDOWEDGE;

    //
    // Space for a menu bar
    //
    if (fMenu)
        lprc->top -= SYSMET(CYMENU);

    //
    // Space for a caption bar
    //
    if ((HIWORD(style) & HIWORD(WS_CAPTION)) == HIWORD(WS_CAPTION)) {
        lprc->top -= (dwExStyle & WS_EX_TOOLWINDOW) ? SYSMET(CYSMCAPTION) : SYSMET(CYCAPTION);
    }

    //
    // Space for borders (window AND client)
    //
    {
        int cBorders;

        //
        // Window AND Client borders
        //

        if (cBorders = GetWindowBorders(style, dwExStyle, TRUE, TRUE))
            InflateRect(lprc, cBorders*SYSMET(CXBORDER), cBorders*SYSMET(CYBORDER));
    }

    return TRUE;
}

/***************************************************************************\
* ShowWindowNoRepaint
\***************************************************************************/

void ShowWindowNoRepaint(PWND pwnd)
{
    HWND hwnd = HWq(pwnd);
    PCLS pcls = REBASE(pwnd, pcls);
    NtUserSetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE |
            SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER |
            SWP_NOREDRAW | SWP_SHOWWINDOW | SWP_NOACTIVATE |
            ((pcls->style & CS_SAVEBITS) ? SWP_CREATESPB : 0));
}

/***************************************************************************\
* AnimateBlend
*
* 6-Mar-1997    vadimg      created
\***************************************************************************/

#define ALPHASTART 40
#define ONEFRAME 10

BOOL AnimateBlend(PWND pwnd, HDC hdcScreen, HDC hdcImage, DWORD dwTime, BOOL fHide)
{
    HWND hwnd = HWq(pwnd);
    SIZE size;
    POINT ptSrc = {0, 0}, ptDst;
    BLENDFUNCTION blend;
    DWORD dwElapsed;
    BYTE bAlpha = ALPHASTART;
    LARGE_INTEGER liFreq, liStart, liDiff;
    LARGE_INTEGER liIter;
    DWORD dwIter;

    if (QueryPerformanceFrequency(&liFreq) == 0)
        return FALSE;

    if (SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) |
            WS_EX_LAYERED) == 0)
        return FALSE;

    if (fHide) {
        /*
         * Give up the time slice and sleep just a touch to allow windows
         * below invalidated by the SetWindowLong(WS_EX_LAYERED) call to
         * repaint enough for the sprite to get good background image.
         */
        Sleep(10);
    }

    ptDst.x = pwnd->rcWindow.left;
    ptDst.y = pwnd->rcWindow.top;
    size.cx = pwnd->rcWindow.right - pwnd->rcWindow.left;
    size.cy = pwnd->rcWindow.bottom - pwnd->rcWindow.top;

    blend.BlendOp     = AC_SRC_OVER;
    blend.BlendFlags  = 0;
    blend.AlphaFormat = 0;
    blend.SourceConstantAlpha = fHide ? (255 - bAlpha) : bAlpha;

    /*
     * Copy the initial image with the initial alpha.
     */
    NtUserUpdateLayeredWindow(hwnd, NULL, &ptDst, &size, hdcImage, &ptSrc, 0,
            &blend, ULW_ALPHA);

    if (!fHide) {
        ShowWindowNoRepaint(pwnd);
    }

    /*
     * Time and start the animation cycle.
     */
    dwElapsed = (dwTime * ALPHASTART + 255) / 255 + 10;
    QueryPerformanceCounter(&liStart);
    liStart.QuadPart = liStart.QuadPart - dwElapsed * liFreq.QuadPart / 1000;

    while (dwElapsed < dwTime) {

        if (fHide) {
            blend.SourceConstantAlpha = (BYTE)((255 * (dwTime - dwElapsed)) / dwTime);
        } else {
            blend.SourceConstantAlpha = (BYTE)((255 * dwElapsed) / dwTime);
        }

        QueryPerformanceCounter(&liIter);

        NtUserUpdateLayeredWindow(hwnd, NULL, NULL, NULL, NULL, NULL, 0,
                &blend, ULW_ALPHA);

        QueryPerformanceCounter(&liDiff);

        /*
         * Calculate how long in ms the previous frame took.
         */
        liIter.QuadPart = liDiff.QuadPart - liIter.QuadPart;
        dwIter = (DWORD)((liIter.QuadPart * 1000) / liFreq.QuadPart);

        if (dwIter < ONEFRAME) {
            Sleep(ONEFRAME - dwIter);
        }

        liDiff.QuadPart -= liStart.QuadPart;
        dwElapsed = (DWORD)((liDiff.QuadPart * 1000) / liFreq.QuadPart);
    }

    /*
     * Hide the window before removing the layered bit to make sure that
     * the bits for the window are not left on the screen.
     */
    if (fHide) {
        NtUserSetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_HIDEWINDOW |
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }

    SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) &
            ~WS_EX_LAYERED);

    if (!fHide) {
        BitBlt(hdcScreen, 0, 0, size.cx, size.cy, hdcImage, 0, 0, SRCCOPY | NOMIRRORBITMAP);
    }

    return TRUE;
}

/***************************************************************************\
* AnimateWindow (API)
*
* Hide animations are done by updating a la full-drag.  Uses window's window
* region to do some of the magic.
*
* We have to put in the CLIPCHILDREN hack to work around a bug with the
* DC cache resetting attributes even if DCX_USESTYLE is not used whe
* the DC cache is invalidated.
*
* History:
* 9-Sep-1996    vadimg      created
\***************************************************************************/

#define AW_HOR          (AW_HOR_POSITIVE | AW_HOR_NEGATIVE | AW_CENTER)
#define AW_VER          (AW_VER_POSITIVE | AW_VER_NEGATIVE | AW_CENTER)

__inline int AnimInc(int x, int y, int z)
{
    return MultDiv(x, y, z);
}

__inline int AnimDec(int x, int y, int z)
{
    return x - AnimInc(x, y, z);
}

BOOL WINAPI AnimateWindow(HWND hwnd, DWORD dwTime, DWORD dwFlags)
{
    PTHREADINFO ptiCurrent = PtiCurrent();
    HDC hdc = NULL, hdcMem = NULL;
    HRGN hrgnUpdate = NULL, hrgnOld = NULL, hrgnWin;
    HBITMAP hbmMem = NULL, hbmOld;
    BOOL fHide = dwFlags & AW_HIDE, fRet = FALSE, fSlide = dwFlags & AW_SLIDE;
    BOOL fRemoveClipChildren = FALSE;
    int x, y, nx, ny, cx, cy, ix, iy, ixLast, iyLast, xLast, yLast, xWin, yWin;
    int xReal, yReal, xMem, yMem;
    DWORD dwStart, dwElapsed;
    RECT rcOld, rcNew, rcWin;
    PWND pwnd = ValidateHwnd(hwnd);
    UINT uBounds;
    RECT rcBounds;
#ifdef USE_MIRRORING
    DWORD dwOldlayout = GDI_ERROR;
#endif

    if (pwnd == NULL)
        return FALSE;

    /*
     * Nothing to do or the flags didn't validate. Send the jerk to hell.
     */
    if ((dwFlags & ~AW_VALID) != 0 || (dwFlags & (AW_HOR_POSITIVE |
            AW_HOR_NEGATIVE | AW_CENTER | AW_VER_POSITIVE |
            AW_VER_NEGATIVE | AW_BLEND)) == 0)
        return FALSE;

    if (!(dwFlags & AW_BLEND)) {
        if (pwnd->hrgnClip != NULL) {
            return FALSE;
        }
    }

    /*
     * If already hidden and tring to hide, just bail out of here.
     * (or already shown and trying to show...)
     */
    if (!IsWindowVisible(hwnd)) {
        if (fHide) {
            return FALSE;
        }
    } else {
        if (!fHide) {
            return FALSE;
        }
    }


    if ((hdc = GetDCEx(hwnd, NULL, DCX_WINDOW | DCX_USESTYLE |
            DCX_CACHE)) == NULL) {
        return FALSE;
    }

    if (TestWF(pwnd, WFCLIPCHILDREN)) {
        fRemoveClipChildren = TRUE;
        ClearWindowState(pwnd, WFCLIPCHILDREN);
    }

    /*
     * Precreate regions used for calculating paint updates.
     */
    if (fHide) {
        if ((hrgnUpdate = CreateRectRgn(0, 0, 0, 0)) == NULL) {
            goto Cleanup;
        }
        if ((hrgnOld = CreateRectRgn(0, 0, 0, 0)) == NULL) {
            goto Cleanup;
        }
    }

    rcWin = pwnd->rcWindow;
    cx = rcWin.right - rcWin.left;
    cy = rcWin.bottom - rcWin.top;

    /*
     * Set up the offscreen dc.
     */
    if ((hbmMem = CreateCompatibleBitmap(hdc, cx, cy | CCB_NOVIDEOMEMORY)) == NULL) {
        goto Cleanup;
    }
    if ((hdcMem = CreateCompatibleDC(hdc)) == NULL) {
        goto Cleanup;
    }
#ifdef USE_MIRRORING
    /*
     * Turn off Mirroring.
     */
    dwOldlayout = SetLayout(hdcMem, 0);
#endif
    hbmOld = SelectBitmap(hdcMem, hbmMem);

    if (!(dwFlags & AW_BLEND)) {
        /*
         * Set window region to nothing, so that if the window draws during
         * callbacks in WM_PRINT, it doesn't happen on screen.
         */
        if ((hrgnWin = CreateRectRgn(0, 0, 0, 0)) != NULL) {
            SetWindowRgn(hwnd, hrgnWin, FALSE);
        }

        if (!fHide) {
            ShowWindowNoRepaint(pwnd);
        }
    }

    SetBoundsRect(hdcMem, NULL, DCB_RESET | DCB_ENABLE);

    /*
     * Get the actual image. The windows participating here must implement
     * WM_PRINTCLIENT or they will look ugly.
     */
    SendMessage(hwnd, WM_PRINT, (WPARAM)hdcMem, PRF_CLIENT | PRF_NONCLIENT |
            PRF_CHILDREN | PRF_ERASEBKGND);

    /*
     * If the window changes size during callbacks, like RAID does with combo
     * boxes by resizing them on WM_CTLCOLOR from WM_ERASEBKGND, send WM_PRINT
     * again to get the correctly sized image.
     */
    if (!EqualRect(&rcWin, &pwnd->rcWindow)) {
        rcWin = pwnd->rcWindow;
        cx = rcWin.right - rcWin.left;
        cy = rcWin.bottom - rcWin.top;

        SelectObject(hdcMem, hbmOld);
        DeleteObject(hbmMem);

        if ((hbmMem = CreateCompatibleBitmap(hdc, cx, cy)) == NULL) {
            goto Cleanup;
        }

        SelectObject(hdcMem, hbmMem);
        SendMessage(hwnd, WM_PRINT, (WPARAM)hdcMem, PRF_CLIENT |
                 PRF_NONCLIENT | PRF_CHILDREN | PRF_ERASEBKGND);
    }

    /*
     * Check to see if the app painted in our DC.  If not, do not animate the window
     * SetBoundsRect() was called prior to SendMessage(WM_PRINT) for the hdcMem
     * and in xxxLBPaint() (to fix bug#83743)
     */
    uBounds = GetBoundsRect(hdcMem, &rcBounds, 0);
    if ((uBounds & DCB_RESET) && (!(uBounds & DCB_ACCUMULATE))) {
        if (fHide) {
            NtUserShowWindow(hwnd, SW_HIDE);
        } else {
            RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME |
                         RDW_ALLCHILDREN);
        }
        goto Cleanup;
    }

    if (dwTime == 0) {
        dwTime = CMS_QANIMATION;
    }
    if (dwFlags & AW_BLEND) {
        fRet = AnimateBlend(pwnd, hdc, hdcMem, dwTime, fHide);
        goto ShowActivate;
    }

    xWin = rcWin.left;
    yWin = rcWin.top;

    ix = iy = xLast = yLast = 0;
    ixLast = cx;
    iyLast = cy;

    /*
     * Calculate initial coordinates and multiples. x, y are the starting points
     * for xReal, yReal calcs, nx, and ny are the directional multiples.
     */
    if (dwFlags & AW_CENTER) {
        x = cx / 2;
        nx = -1;
        fSlide = FALSE;
    } else if (dwFlags & AW_HOR_POSITIVE) {
        x = fHide ? cx : 0;
        nx = fHide ? -1 : 0;
    } else if (dwFlags & AW_HOR_NEGATIVE) {
        x = fHide ? 0 : cx;
        nx = fHide ? 0 : -1;
    } else {
        x = 0;
        nx = 0;
        ix = cx;
    }

    if (dwFlags & AW_CENTER) {
        y = cy / 2;
        ny = -1;
    } else if (dwFlags & AW_VER_POSITIVE) {
        y = fHide ? cy : 0;
        ny = fHide ? -1 : 0;
    } else if (dwFlags & AW_VER_NEGATIVE) {
        y = fHide ? 0 : cy;
        ny = fHide ? 0 : -1;
    } else {
        y = 0;
        ny = 0;
        iy = cy;
    }

    dwStart = GetTickCount();

    while (TRUE) {

        dwElapsed = GetTickCount() - dwStart;

        if (dwFlags & AW_HOR) {
            if (fHide) {
                ix = AnimDec(cx, dwElapsed, dwTime);
            } else {
                ix = AnimInc(cx, dwElapsed, dwTime);
            }
        }

        if (dwFlags & AW_VER) {
            if (fHide) {
                iy = AnimDec(cy, dwElapsed, dwTime);
            } else {
                iy = AnimInc(cy, dwElapsed, dwTime);
            }
        }

        /*
         * Terminate when we are out of time or we're hiding and either
         * dimenion has reached zero (i.e. the window is not visible) or
         * we're showing and the window is completely visible.
         */
        if (dwElapsed > dwTime ||
                (fHide && (ix == 0 || iy == 0)) ||
                (!fHide && (ix == cx && iy == cy))) {
            break;
        } else if (ixLast == ix && iyLast == iy) {
            Sleep(1);
        } else {
            if (dwFlags & AW_CENTER) {
                xReal = x + nx * (ix / 2);
                yReal = y + ny * (iy / 2);
            } else {
                xReal = x + nx * ix;
                yReal = y + ny * iy;
            }

            /*
             * Calculate new window area and set as the window rgn.
             */
            rcNew.left = xReal;
            rcNew.top = yReal;
            rcNew.right = rcNew.left + ix;
            rcNew.bottom = rcNew.top + iy;

            /*
             * Change the window region accordingly to the new rect to make
             * sure that everything underneath paints properly.  If a region
             * was selected in before, it will be destroyed in SetWindowRgn.
             */
            if ((hrgnWin = CreateRectRgnIndirect(&rcNew)) != NULL) {
                SetWindowRgn(hwnd, hrgnWin, FALSE);
            }

            if (fHide) {
                /*
                 * Calculate the smallest possible update region.
                 */
                rcOld.left = xLast;
                rcOld.top = yLast;
                rcOld.right = rcOld.left + ixLast;
                rcOld.bottom = rcOld.top + iyLast;

                SetRectRgn(hrgnOld, rcOld.left, rcOld.top, rcOld.right, rcOld.bottom);
                SetRectRgn(hrgnUpdate, rcNew.left, rcNew.top, rcNew.right, rcNew.bottom);
                CombineRgn(hrgnUpdate, hrgnOld, hrgnUpdate, RGN_DIFF);

                OffsetRgn(hrgnUpdate, xWin, yWin);
                RedrawWindow(NULL, NULL, hrgnUpdate, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME |
                        RDW_ALLCHILDREN);
                NtUserCallHwndParamLock(hwnd, (ULONG_PTR)hrgnUpdate, SFI_XXXUPDATEWINDOWS);
            }

            xMem = xReal;
            yMem = yReal;

            if (fSlide) {
                if (dwFlags & AW_HOR_POSITIVE) {
                    xMem = fHide ? 0: cx - ix;
                } else if (dwFlags & AW_HOR_NEGATIVE) {
                    xMem = fHide ? cx - ix : 0;
                }
                if (dwFlags & AW_VER_POSITIVE) {
                    yMem = fHide ? 0 : cy - iy;
                } else if (dwFlags & AW_VER_NEGATIVE) {
                    yMem = fHide ? cy - iy : 0;
                }
            }

            BitBlt(hdc, xReal, yReal, ix, iy, hdcMem, xMem, yMem, SRCCOPY | NOMIRRORBITMAP);

            xLast = xReal;
            yLast = yReal;
            ixLast = ix;
            iyLast = iy;
        }
    }

    fRet = TRUE;

    /*
     * One last update to make sure that everything is repainted properly.
     */
    if (fHide) {
        if (fRemoveClipChildren) {
            SetWindowState(pwnd, WFCLIPCHILDREN);
            fRemoveClipChildren = FALSE;
        }
        NtUserSetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE |
                SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER |
                SWP_NOREDRAW | SWP_HIDEWINDOW |
                (dwFlags & AW_ACTIVATE ? 0 : SWP_NOACTIVATE));

        /*
         * This way we won't get a flash, which we would if we allowed to draw
         * in the call above.
         */
        if (ixLast != 0 && iyLast != 0) {
            rcWin.left = xLast + xWin;
            rcWin.top = yLast + yWin;
            rcWin.right = rcWin.left + ixLast;
            rcWin.bottom = rcWin.top + iyLast;
            SetRectRgn(hrgnUpdate, rcWin.left, rcWin.top, rcWin.right, rcWin.bottom);
            RedrawWindow(NULL, NULL, hrgnUpdate, RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_ERASE |
                         RDW_FRAME);
            NtUserCallHwndParamLock(hwnd, (ULONG_PTR)hrgnUpdate, SFI_XXXUPDATEWINDOWS);
        }
    } else {
        if (ixLast != cx || iyLast != cy) {
            if ((hrgnWin = CreateRectRgn(0, 0, cx, cy)) != NULL) {
                SetWindowRgn(hwnd, hrgnWin, FALSE);
            }
            BitBlt(hdc, 0, 0, cx, cy, hdcMem, 0, 0, SRCCOPY | NOMIRRORBITMAP);
        }
        if (fRemoveClipChildren) {
            SetWindowState(pwnd, WFCLIPCHILDREN);
            fRemoveClipChildren = FALSE;
        }

ShowActivate:
        if (dwFlags & AW_ACTIVATE) {
            NtUserSetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE |
                    SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);
        }
    }

Cleanup:
    if (fRemoveClipChildren) {
        SetWindowState(pwnd, WFCLIPCHILDREN);
    }
    if (hdcMem != NULL) {
#ifdef USE_MIRRORING
        /*
         * Restore old layout value.
         */
        if (dwOldlayout != GDI_ERROR)
            SetLayout(hdcMem, dwOldlayout);
#endif
        DeleteDC(hdcMem);
    }
    if (hbmMem != NULL) {
        DeleteObject(hbmMem);
    }
    if (hdc != NULL) {
        ReleaseDC(hwnd, hdc);
    }
    if (fHide) {
        if (hrgnUpdate != NULL) {
            DeleteObject(hrgnUpdate);
        }
        if (hrgnOld != NULL) {
            DeleteObject(hrgnOld);
        }
    }
    if (!(dwFlags & AW_BLEND)) {
        SetWindowRgn(hwnd, NULL, FALSE);
    }
    return fRet;
}

/***************************************************************************\
* SmoothScrollWindowEx
*
* History:
* 24-Sep-1996   vadimg      wrote
\***************************************************************************/

#define MINSCROLL 10
#define MAXSCROLLTIME 200

int SmoothScrollWindowEx(HWND hwnd, int dx, int dy, CONST RECT *prcScroll,
        CONST RECT *prcClip, HRGN hrgnUpdate, LPRECT prcUpdate, DWORD dwFlags,
        DWORD dwTime)
{
    RECT rc, rcT, rcUpdate;
    int dxStep, dyStep, dxDone, dyDone, xSrc, ySrc, xDst, yDst, dxBlt, dyBlt;
    int nRet = ERROR, nClip;
    BOOL fNegX = FALSE, fNegY = FALSE;
    HDC hdc, hdcMem = NULL;
    HBITMAP hbmMem = NULL, hbmOld;
    DWORD dwSleep;
    BOOL fCalcSubscroll = FALSE;
    PWND pwnd = ValidateHwnd(hwnd);
    HRGN hrgnScroll = NULL, hrgnErase = NULL;
    MSG msg;
    UINT uBounds;
    RECT rcBounds;

    if (pwnd == NULL)
        return ERROR;
    /*
     * Keep track of the signs so we don't have to mess with abs all the time.
     */
    if (dx < 0) {
        fNegX = TRUE;
        dx = -dx;
    }

    if (dy < 0) {
        fNegY = TRUE;
        dy = -dy;
    }

    /*
     * Set up the client rectangle.
     */
    if (prcScroll != NULL) {
        rc = *prcScroll;
    } else {
        rc.left = rc.top = 0;
        rc.right = pwnd->rcClient.right - pwnd->rcClient.left;
        rc.bottom = pwnd->rcClient.bottom - pwnd->rcClient.top;
    }

    /*
     * If they want to scroll less than we can let them, or more than
     * one page, or need repainting send them to the API.
     */
    if (pwnd->hrgnUpdate != NULL || (dx == 0 && dy == 0) ||
        (dx != 0 && dx > rc.right) ||
        (dy != 0 && dy > rc.bottom)) {
        return NtUserScrollWindowEx(hwnd, fNegX ? -dx : dx, fNegY ? -dy : dy,
                prcScroll, prcClip, hrgnUpdate, prcUpdate,
                dwFlags | SW_ERASE | SW_INVALIDATE);
    }

    if ((hdc = GetDCEx(hwnd, NULL, DCX_USESTYLE | DCX_CACHE)) == NULL) {
        return ERROR;
    }

    /*
     * Part of the window may be obscured, which means that more may be
     * invisible and may need to be bltted. Take that into account by
     * gettting the clip box.
     */
    nClip = GetClipBox(hdc, &rcT);
    if (nClip == ERROR || nClip == NULLREGION) {
        goto Cleanup;
    }

    /*
     * Set up the offscreen dc and send WM_PRINT to get the image.
     */
    if ((hbmMem = CreateCompatibleBitmap(hdc, rc.right, rc.bottom)) == NULL) {
        goto Cleanup;
    }
    if ((hdcMem = CreateCompatibleDC(hdc)) == NULL) {
        goto Cleanup;
    }
    hbmOld = SelectBitmap(hdcMem, hbmMem);

    SetBoundsRect(hdcMem, NULL, DCB_RESET | DCB_ENABLE);

    SendMessage(hwnd, WM_PRINT, (WPARAM)hdcMem, PRF_CLIENT |
            PRF_ERASEBKGND | ((dwFlags & SW_SCROLLCHILDREN) ? PRF_CHILDREN : 0));

    /*
     * If the client rect changes during the callback, send WM_PRINT
     * again to get the correctly sized image.
     */
    if (prcScroll == NULL) {
        rcT.left = rcT.top = 0;
        rcT.right = pwnd->rcClient.right - pwnd->rcClient.left;
        rcT.bottom = pwnd->rcClient.bottom - pwnd->rcClient.top;

        if (!EqualRect(&rc, &rcT)) {
            rc = rcT;

            SelectObject(hdcMem, hbmOld);
            DeleteObject(hbmMem);

            if ((hbmMem = CreateCompatibleBitmap(hdc, rc.right, rc.bottom)) == NULL) {
                goto Cleanup;
            }

            SelectObject(hdcMem, hbmMem);
            SendMessage(hwnd, WM_PRINT, (WPARAM)hdcMem, PRF_CLIENT |
                    PRF_ERASEBKGND | ((dwFlags & SW_SCROLLCHILDREN) ? PRF_CHILDREN : 0));
        }
    }

    /*
     * Check to see if the app painted in our DC.
     */
    uBounds = GetBoundsRect(hdcMem, &rcBounds, 0);
    if ((uBounds & DCB_RESET) && (!(uBounds & DCB_ACCUMULATE))) {
        goto Cleanup;
    }

    if ((hrgnScroll = CreateRectRgn(0, 0, 0, 0)) == NULL) {
        goto Cleanup;
    }
    if ((hrgnErase = CreateRectRgn(0, 0, 0, 0)) == NULL) {
        goto Cleanup;
    }
    SetRectEmpty(&rcUpdate);

    /*
     * Start off with MINSCROLL and adjust it based on available time after
     * the first iteration. We should consider adding a NOTIMELIMIT flag.
     */
    xDst = xSrc = 0;
    yDst = ySrc = 0;

    dxBlt = rc.right;
    dyBlt = rc.bottom;

    if (dx == 0) {
        dxDone = rc.right;
        dxStep = 0;
    } else {
        dxDone = 0;
        dxStep = max(dx / MINSCROLL, 1);
    }

    if (dy == 0) {
        dyDone = rc.bottom;
        dyStep = 0;
    } else {
        dyDone = 0;
        dyStep = max(dy / MINSCROLL, 1);
    }

    if (dwTime == 0) {
        dwTime = MAXSCROLLTIME;
    }
    dwSleep = dwTime / MINSCROLL;

    do {

        /*
         * When the dc is scrolled, the part that's revealed cannot be
         * updated properly. We set up the variables to blt just the part that
         * was just uncovered.
         */
        if (dx != 0) {
            if (dxDone + dxStep > dx) {
                dxStep = dx - dxDone;
            }
            dxDone += dxStep;

            xDst = dx - dxDone;
            dxBlt = rc.right - xDst;
            if (!fNegX) {
                xSrc = xDst;
                xDst = 0;
            }
        }

        if (dy != 0) {
            if (dyDone + dyStep > dy) {
                dyStep = dy - dyDone;
            }
            dyDone += dyStep;

            yDst = dy - dyDone;
            dyBlt = rc.bottom - yDst;
            if (!fNegY) {
                ySrc = yDst;
                yDst = 0;
            }
        }

        /*
         * This is a hack for ReaderMode to be smoothly continuous. We'll make an
         * attempt for the scrolling to take as close to dwTime
         * as possible. We'll also dispatch MOUSEMOVEs to the ReaderMode window, so it
         * can update mouse cursor.
         */
        if (MsgWaitForMultipleObjects(0, NULL, FALSE, dwSleep, QS_MOUSEMOVE) == WAIT_OBJECT_0) {
            if (PeekMessage(&msg, NULL, WM_MOUSEMOVE, WM_MOUSEMOVE, MAKELONG(PM_NOREMOVE, QS_INPUT))) {
                PWND pwndPeek = ValidateHwnd(msg.hwnd);
                if (pwndPeek != NULL) {
                    PCLS pcls = (PCLS)REBASEALWAYS(pwndPeek, pcls);
                    if (pcls->atomClassName == gatomReaderMode) {
                        if (PeekMessage(&msg, msg.hwnd, WM_MOUSEMOVE, WM_MOUSEMOVE, MAKELONG(PM_REMOVE, QS_INPUT))) {
                            DispatchMessage(&msg);
                        }
                    }
                }
            }
        }

        if ((nRet = NtUserScrollWindowEx(hwnd, fNegX ? -dxStep : dxStep,
                fNegY ? -dyStep : dyStep, prcScroll, prcClip,
                hrgnScroll, &rcT, dwFlags)) == ERROR)
            goto Cleanup;

        UnionRect(&rcUpdate, &rcUpdate, &rcT);

        /*
         * Blt the uncovered part.
         */
        BitBlt(hdc, xDst, yDst, dxBlt, dyBlt, hdcMem, xSrc, ySrc, SRCCOPY | NOMIRRORBITMAP);

        SetRectRgn(hrgnErase, xDst, yDst, xDst + dxBlt, yDst + dyBlt);
        CombineRgn(hrgnErase, hrgnScroll, hrgnErase, RGN_DIFF);
        RedrawWindow(hwnd, NULL, hrgnErase, RDW_ERASE | RDW_INVALIDATE | RDW_ERASENOW);

    } while (dxDone < dx || dyDone < dy);

    if (prcUpdate != NULL) {
        *prcUpdate = rcUpdate;
    }
    if (hrgnUpdate != NULL) {
        SetRectRgn(hrgnUpdate, rcUpdate.left, rcUpdate.top,
                rcUpdate.right, rcUpdate.bottom);
    }

Cleanup:
    if (hdcMem != NULL) {
        DeleteDC(hdcMem);
    }
    if (hbmMem != NULL) {
        DeleteObject(hbmMem);
    }
    if (hdc != NULL) {
        ReleaseDC(hwnd, hdc);
    }
    if (hrgnErase != NULL) {
        DeleteObject(hrgnErase);
    }
    if (hrgnScroll != NULL) {
        DeleteObject(hrgnScroll);
    }
    return nRet;
}

/***************************************************************************\
* ScrollWindowEx (API)
*
\***************************************************************************/

int ScrollWindowEx(HWND hwnd, int dx, int dy, CONST RECT *prcScroll,
        CONST RECT *prcClip, HRGN hrgnUpdate, LPRECT prcUpdate,
        UINT dwFlags)
{
    if (dwFlags & SW_SMOOTHSCROLL) {
        return SmoothScrollWindowEx(hwnd, dx, dy, prcScroll, prcClip,
                hrgnUpdate, prcUpdate, LOWORD(dwFlags), HIWORD(dwFlags));
    } else {
        return NtUserScrollWindowEx(hwnd, dx, dy, prcScroll, prcClip,
                hrgnUpdate, prcUpdate, dwFlags);
    }
}
