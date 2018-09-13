/****************************************************************************\
*
*  MDIWIN.C -
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
*      MDI Child Windows Support
*
* History
* 11-14-90 MikeHar     Ported from windows
* 14-Feb-1991 mikeke   Added Revalidation code
\****************************************************************************/

#include "precomp.h"
#pragma hdrstop

#define TITLE_EXTRA 5
#define MAX_TITLE_LEN 160


/***************************************************************************\
* xxxSetFrameTitle
*
* if lpch == 1, we redraw the whole frame. If 2, we don't do any redraw. Any
* other value, and we redraw just the caption of the frame.
*
* History:
* 11-14-90 MikeHar Ported from windows
* 04-16-91 MikeHar Win31 Merge
\***************************************************************************/

void xxxSetFrameTitle(
    PWND pwndFrame,
    PWND pwndMDI,
    LPWSTR lpch)
{
    PWND pwnd;
    PMDI pmdi;
    WCHAR sz[MAX_TITLE_LEN];
    HWND  hwndFrame = HW(pwndFrame);

    CheckLock(pwndFrame);
    CheckLock(pwndMDI);

    /*
     * Get a pointer to the MDI structure
     */
    pmdi = ((PMDIWND)pwndMDI)->pmdi;

    if (IS_PTR(lpch) || lpch == NULL) {
        if (HTITLE(pmdi)) {
            UserLocalFree(HTITLE(pmdi));
        }
        HTITLE(pmdi) = TextAlloc(lpch);
    }

    if (HTITLE(pmdi)) {
        LARGE_UNICODE_STRING str;
        int cch;

        RtlInitLargeUnicodeString(&str, HTITLE(pmdi), (UINT)-1);
        TextCopy(&str, sz, sizeof(sz)/sizeof(WCHAR));

        if (MAXED(pmdi) && (pwnd = ValidateHwnd(MAXED(pmdi))) && pwnd->strName.Length) {

            cch = MAX_TITLE_LEN - ((str.Length / sizeof(WCHAR)) + TITLE_EXTRA);
            if (cch > 0) {
                wcscat(sz, TEXT(" - ["));
                wcsncat(sz, REBASE(pwnd, strName.Buffer), cch - 1);
                wcscat(sz, TEXT("]"));
            }
        }
    } else {
        sz[0] = 0;
    }

    _DefSetText(hwndFrame, sz, FALSE);

    if (lpch == (LPWSTR)1L)
        NtUserRedrawFrameAndHook(hwndFrame);

    else if (lpch != (LPWSTR)2L) {
        if (!NtUserRedrawTitle(hwndFrame, DC_TEXT))
            NtUserRedrawFrame(hwndFrame);
    }
}


/***************************************************************************\
* TranslateMDISysAccel
*
* History:
* 11-14-90 MikeHar Ported from windows
\***************************************************************************/

BOOL TranslateMDISysAccel(
    HWND hwnd,
    LPMSG lpMsg)
{
    PWND pwnd;
    PMDI pmdi;
    int event;

    /*
     * Is this a message we care about?
     */
    if (lpMsg->message != WM_KEYDOWN && lpMsg->message != WM_SYSKEYDOWN) {
        return FALSE;
    }

    /*
     * This is called within a message loop. If the window gets destroyed,
     * there still may be other messages in the queue that get returned
     * after the window is destroyed. The app will call TranslateAccelerator()
     * on every one of these, causing RIPs.... Make it nice so it just
     * returns FALSE.
     */
    if ((pwnd = ValidateHwndNoRip(hwnd)) == NULL) {
        RIPERR0(ERROR_INVALID_WINDOW_HANDLE, RIP_VERBOSE, "");
        return FALSE;
    }

    CheckLock(pwnd);

    /*
     * Make sure this is really an MDIClient window. Harvard Graphics 2.0
     * calls this function with a different window class and caused us
     * to get an access violation.
     */
    if (GETFNID(pwnd) != FNID_MDICLIENT) {
        RIPMSG0(RIP_WARNING, "Window not of MDIClient class");
        return FALSE;
    }

    /*
     * Get a pointer to the MDI structure
     */
    pmdi = ((PMDIWND)pwnd)->pmdi;

    if (!ACTIVE(pmdi))
        return FALSE;

    if (!IsWindowEnabled(ACTIVE(pmdi)))
        return FALSE;

    switch (lpMsg->wParam) {
    case VK_F4:
        event = SC_CLOSE;
        break;
    case VK_F6:
    case VK_TAB:
        if (GetKeyState(VK_SHIFT) < 0)
            event = SC_PREVWINDOW;
        else
            event = SC_NEXTWINDOW;
        break;
    default:
        return FALSE;
    }

    /*
     * All of these have the control key down
     */
    if (GetKeyState(VK_CONTROL) >= 0)
        return FALSE;

    if (GetKeyState(VK_MENU) < 0)
        return FALSE;

    SendMessage(ACTIVE(pmdi), WM_SYSCOMMAND, event, MAKELONG(lpMsg->wParam, 0));

    return TRUE;
}

/***************************************************************************\
*
*  CalcClientScrolling()
*
\***************************************************************************/

#define SBJ_HORZ  HAS_SBHORZ
#define SBJ_VERT  HAS_SBVERT
#define SBJ_BOTH  (SBJ_HORZ | SBJ_VERT)

void ByteOutsetRect(LPRECT lprc)
{
    int FAR *pi;
    int i;

    for (i = 0, pi = (int FAR *) lprc; i < 4; i++, pi++) {
        if (*pi > 0)
            *pi += 7;
        else if (*pi < 0)
            *pi -= 7;

        *pi /= 8;
    }
}

void CalcClientScrolling(HWND hwnd, UINT sbj, BOOL fIgnoreMin)
{
    PWND pwnd;
    RECT rcScroll;
    RECT rcClient;
    RECT rcRange;
    RECT rcT;
    PWND pwndT;
    BOOL fVert;
    BOOL fHorz;
    BYTE fHadVert, fHadHorz;
    BOOL fCheckVert;
    BOOL fCheckHorz;
    BOOL fNeedScrolls;
    SCROLLINFO si;

    if ((pwnd = ValidateHwnd(hwnd)) == NULL) {
        return;
    }
    CheckLock(pwnd);

    UserAssert(GETFNID(pwnd) != FNID_DESKTOP);

    // do nothing if the parent is iconic.  This way, we don't add invisible
    // scrollbars which will paint and unpaint when restoring...
    if (TestWF(pwnd, WFMINIMIZED))
        return;

    fVert = FALSE;
    fHorz = FALSE;
    fNeedScrolls=FALSE;

    fCheckHorz = (sbj & SBJ_HORZ);
    fCheckVert = (sbj & SBJ_VERT);

    // find client area without scroll bars
    CopyRect(&rcClient, &pwnd->rcClient);

    fHadVert = TestWF(pwnd, WFVSCROLL);
    if (fCheckVert && fHadVert)
        rcClient.right += SYSMET(CXVSCROLL);

    fHadHorz = TestWF(pwnd, WFHSCROLL);
    if (fCheckHorz && fHadHorz)
        rcClient.bottom += SYSMET(CYHSCROLL);

    // find the rectangle that bounds all visible child windows
    SetRectEmpty(&rcScroll);

    for (pwndT = REBASEPWND(pwnd, spwndChild); pwndT;
            pwndT = REBASEPWND(pwndT, spwndNext)) {
        if (fIgnoreMin && TestWF(pwndT, WFMINIMIZED))
            continue;

        if (TestWF(pwndT,WFVISIBLE)) {
            if (TestWF(pwndT, WFMAXIMIZED)) {
                fNeedScrolls = FALSE;
                break;
            }

            /*
             * add this window to the area that has to be visible
             */
            UnionRect(&rcScroll, &rcScroll, &pwndT->rcWindow);

            /*
             * add scroll bars if its not contained in the
             * client area
             */
            UnionRect(&rcT, &rcClient, &pwndT->rcWindow);
            if (!EqualRect(&rcClient, &rcT)) {
                fNeedScrolls = TRUE;
            }
        }
    }

    SetRectEmpty(&rcRange);

    // offset rectangles such that rcClient's top & left are both 0
    // making rcClient's right & bottom be the page size
    OffsetRect(&rcScroll, -rcClient.left, -rcClient.top);
    OffsetRect(&rcClient, -rcClient.left, -rcClient.top);

    if (!fNeedScrolls)
        rcClient.bottom = rcClient.right = 0;
    else do
    {
            /*
             * the range is the union of the parent client with all of its
             * children
             */
        CopyRect(&rcT, &rcRange);
        UnionRect(&rcRange, &rcScroll, &rcClient);

        if (fCheckVert) {
            // subtract off space for the vertical scroll if we need it
            if (((rcRange.bottom - rcRange.top) > rcClient.bottom) && !fVert) {
                fVert = TRUE;
                rcClient.right -= SYSMET(CXVSCROLL);
            }
        }

        if (fCheckHorz) {
            // subtract off space for the horizontal scroll if we need it
            if (((rcRange.right - rcRange.left) > rcClient.right) && !fHorz) {
                fHorz = TRUE;
                rcClient.bottom -= SYSMET(CYHSCROLL);
            }
        }
    }
    while (!EqualRect(&rcRange, &rcT));

    if (fNeedScrolls) {
        // HACK of death beginning
        if (rcRange.right == rcClient.right)
            rcRange.right -= 8;

        if (rcRange.bottom == rcClient.bottom)
            rcRange.bottom -= 8;
        // HACK of death ending
    }

    if (fCheckVert) {

        /*
         * check to see if we are changing the presence of the vertical
         * scrollbar
         */
        if ((rcRange.bottom - rcRange.top) <= rcClient.bottom) {
            ClearWindowState(pwnd, WFVSCROLL);
        } else {
            SetWindowState(pwnd, WFVSCROLL);
       }
    }

    if (fCheckHorz) {

        /*
         * same for horizontal scroll
         */
        if ((rcRange.right - rcRange.left) <= rcClient.right) {
            ClearWindowState(pwnd, WFHSCROLL);
        } else {
            SetWindowState(pwnd, WFHSCROLL);
        }
    }

    if (fNeedScrolls) {
        ByteOutsetRect(&rcClient);
        ByteOutsetRect(&rcRange);
    }

    si.cbSize   = sizeof(SCROLLINFO);
    si.fMask    = SIF_ALL;
    si.nPos     = 0;

    si.nMin     = rcRange.left;
    si.nMax     = rcRange.right;
    si.nPage    = rcClient.right;

    NtUserSetScrollInfo(hwnd, SB_HORZ, &si, FALSE);

    si.nMin     = rcRange.top;
    si.nMax     = rcRange.bottom;
    si.nPage    = rcClient.bottom;

    NtUserSetScrollInfo(hwnd, SB_VERT, &si, FALSE);

    if ((fHadVert != TestWF(pwnd, WFVSCROLL)) ||
        (fHadHorz != TestWF(pwnd, WFHSCROLL)))
    NtUserRedrawFrame(hwnd);
}


/***************************************************************************\
* ScrollChildren
*
*  Handles WM_VSCROLL and WM_HSCROLL messages
*
* History:
* 11-14-90 MikeHar Ported from windows
\***************************************************************************/

void ScrollMDIChildren(
    HWND hwnd,
    int nCtl,
    UINT wCmd,
    int iThumbPos)
{
    SCROLLINFO  si;
    int wInc;
    int wNewPos;
    //SHORT sPos;
    int          x, y;

    wInc = (((nCtl == SB_VERT) ? SYSMET(CYSIZE) : SYSMET(CXSIZE)) + 7) / 8;

    si.cbSize   = sizeof(SCROLLINFO);
    si.fMask    = SIF_ALL;
    GetScrollInfo(hwnd, nCtl, &si);

    si.nPage--;
    si.nMax -= si.nPage;

    switch (wCmd) {
    case SB_BOTTOM:
        wNewPos = si.nMax;
        break;
    case SB_TOP:
        wNewPos = si.nMin;
        break;
    case SB_LINEDOWN:
        wNewPos = si.nPos + wInc;
        break;
    case SB_LINEUP:
        wNewPos = si.nPos - wInc;
        break;
    case SB_PAGEDOWN:
        wNewPos = si.nPos + si.nPage;
        break;
    case SB_PAGEUP:
        wNewPos = si.nPos - si.nPage;
        break;
    case SB_THUMBPOSITION:

        wNewPos = iThumbPos;
        break;
    case SB_ENDSCROLL:
        CalcClientScrolling(hwnd, (nCtl == SB_VERT) ? SBJ_VERT : SBJ_HORZ, FALSE);

    /*
     ** FALL THRU **
     */
    default:
        return;
    }

    if (wNewPos < si.nMin)
        wNewPos = si.nMin;
    else if (wNewPos > si.nMax)
        wNewPos = si.nMax;

    SetScrollPos(hwnd, nCtl, wNewPos, TRUE);

    // the "* 8" is because we need to scroll in bytes.  The scrollbar
    // increments for MDI are bytes (this is due to the fact that we need to
    // not upset the brush origin of the app workspace brush that is used to
    // fill the MDI background)

    x = (si.nPos - wNewPos) * 8;

    if (nCtl == SB_VERT) {
        y = x;
        x = 0;
    } else
        // x is already set properly for this case
        y = 0;

    NtUserScrollWindowEx(hwnd, x, y, NULL, NULL, NULL, NULL,
           SW_SCROLLWINDOW | SW_INVALIDATE | SW_ERASE | SW_SCROLLCHILDREN);
}


VOID ScrollChildren(
    HWND hwnd,
    UINT wMsg,
    DWORD wParam)
{
    ScrollMDIChildren(hwnd,
                      wMsg == WM_VSCROLL ? SB_VERT : SB_HORZ,
                      LOWORD(wParam),
                      (short)(HIWORD(wParam)));
}


/***************************************************************************\
* RecalculateScrollRanges
*
* History:
* 11-14-90 MikeHar Ported from windows
* 04-16-91 MikeHar Win31 Merge
\***************************************************************************/

void RecalculateScrollRanges(
    PWND pwndParent,
    BOOL fIgnoreMin)
{
    PMDI pmdi = ((PMDIWND)pwndParent)->pmdi;

    if (!(SCROLL(pmdi) & (CALCSCROLL | SCROLLCOUNT))) {
        if (PostMessage(HWq(pwndParent), MM_CALCSCROLL, fIgnoreMin, 0L)) {
            SCROLL(pmdi) |= CALCSCROLL;
        }
    }
}


/***************************************************************************\
* GetCascadeWindowPos
*
* History:
* 11-14-90 MikeHar Ported from windows
* 01-12-94 FritzS  Ported from Chicago
\***************************************************************************/

void GetCascadeWindowPos(
    LPCRECT prcClient,  /* area to arrange to */
    int     iWindow,    /* index of this window */
    LPRECT  lprc)       /* resulting rectangle */
{
    int cStack;
    int xStep, yStep;
    int dxClient, dyClient;

    /*
     * Compute the width and breadth of the situation.
     */
    dxClient = prcClient->right - prcClient->left;
    UserAssert(dxClient >= 0);
    dyClient = prcClient->bottom - prcClient->top;
    UserAssert(dyClient >= 0);

    /*
     * Compute the width and breadth of the window steps.
     */
    xStep = SYSMET(CXSIZEFRAME) + SYSMET(CXSIZE);
    yStep = SYSMET(CYSIZEFRAME) + SYSMET(CYSIZE);

    /*
     * How many windows per stack?
     */
    cStack = dyClient / (3 * yStep);

    lprc->right = dxClient - (cStack * xStep);
    lprc->bottom = dyClient - (cStack * yStep);

    /*
     * HACK!: Mod by cStack+1 and make sure no div-by-zero
     * exception happens.
     */
    if (++cStack <= 0) {
        cStack = 1;
    }

    lprc->left = prcClient->left + (iWindow % cStack) * xStep;
    lprc->top = prcClient->top + (iWindow % cStack) * yStep;
}


/***************************************************************************\
* MDICheckCascadeRect
*
* History:
* 11-14-90 MikeHar Ported from windows
* 04-16-91 MikeHar Win31 Merge
\***************************************************************************/

void MDICheckCascadeRect(
    PWND pwndClient,
    LPRECT lprc)
{
    PMDI pmdi;
    RECT rc, rcClient;
    int         iWindow;

    /*
     * Get a pointer to the MDI structure
     */
    pmdi = ((PMDIWND)pwndClient)->pmdi;

    iWindow = ITILELEVEL(pmdi);

    GetRect(pwndClient, &rcClient, GRECT_CLIENT | GRECT_CLIENTCOORDS);
    GetCascadeWindowPos(&rcClient, iWindow, &rc);

    if ((lprc->right == CW_USEDEFAULT || lprc->right == CW2_USEDEFAULT) ||
            !(lprc->right)) {
        lprc->right = rc.right;
    }

    if ((lprc->bottom == CW_USEDEFAULT || lprc->bottom == CW2_USEDEFAULT) ||
            !(lprc->bottom)) {
        lprc->bottom = rc.bottom;
    }

    if (lprc->left == CW_USEDEFAULT || lprc->left == CW2_USEDEFAULT) {
        lprc->left = rc.left;
        lprc->top = rc.top;
    }
}


/***************************************************************************\
* UnMaximizeChildWindows
*
* effects: Helper routine used by TileChildWindows and CascadeChildWindows to
* restore any maximized windows of the given parent. Returns TRUE if a
* maximized window was restored.
*
* History:
* 4-16-91 MikeHar       Win31 Merge
\***************************************************************************/

BOOL UnmaximizeChildWindows(
    HWND hwndParent)
{
    HWND hwndMove;
    PWND pwndMove;
    BOOL fFoundOne = FALSE;
    BOOL fAsync;
    UINT chwnd;
    HWND *phwndList;
    HWND *phwnd;
    HWND hwndChild = GetWindow(hwndParent, GW_CHILD);

    /*
     * Get the hwnd list.  It is returned in a block of memory
     * allocated with LocalAlloc.
     */
    if (hwndChild == NULL ||
            (chwnd = BuildHwndList(NULL, GetWindow(hwndParent, GW_CHILD),
                                   FALSE, 0, &phwndList)) == 0) {
        return FALSE;
    }

    fAsync = (hwndParent == GetDesktopWindow());

    for (phwnd = phwndList; chwnd > 0; chwnd--, phwnd++) {
        if ((hwndMove = *phwnd) == NULL)
            continue;

        if ((pwndMove = ValidateHwnd(hwndMove)) == NULL)
            continue;

// Not in Chicago -- FritzS
//        if (pwndMove->spwndOwner != NULL)
//            continue;

        if (TestWF(pwndMove, WFMAXIMIZED) && TestWF(pwndMove, WFVISIBLE)) {
            //
            // If we haven't done it yet, lock the screen to prevent sending
            // paints for a cleaner update.
            //
            if (!fFoundOne && fAsync)
                NtUserLockWindowUpdate(hwndParent);

            fFoundOne = TRUE;

            if (fAsync)
                NtUserShowWindowAsync(hwndMove, SW_SHOWNOACTIVATE);
            else
                NtUserShowWindow(hwndMove, SW_SHOWNORMAL);
        }
    }

    UserLocalFree(phwndList);

    if (fFoundOne && fAsync) {

        HWND hwndActive = NtUserGetForegroundWindow();
        if (hwndActive != NULL) {

            /*
             * Hack! Since the above showwindows cause zorder changes, we want
             * the active window to be in front.  This makes sure...
             */
            NtUserSetWindowPos(hwndActive, HWND_TOP, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_ASYNCWINDOWPOS);

        }
        NtUserLockWindowUpdate(NULL);
        RedrawWindow(hwndParent, NULL, NULL,
                RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_ERASE | RDW_FRAME);
    }

    return fFoundOne;
}


/***************************************************************************\
*
* ARRANGEWINDOWSDATA: passed to EnumDisplayMonitors enumeration functions.
*
\***************************************************************************/

typedef struct tagARRANGEWINDOWSDATA {
    PWND    pwndParent;
    UINT    flags;
    LPRECT  lprcParent;
    int     chwnd;
    int     chwndReal;
    HWND *  phwnd;
    PWND    pwndDesktop;
    HDWP    hdwp;
    UINT    uGRCFlags;
    int     fVerifyParent;
} ARRANGEWINDOWSDATA, *PARRANGEWINDOWSDATA;



/***************************************************************************\
* ArrangeWindows
*
* Called from CascadeWindows and TileWindows, it performs what
* is common to both functions, and calls out to the enumeration
* function to do the work of window arrangement.
*
* History:
* 10-Jul-1997 adams     Created.
\***************************************************************************/

WORD ArrangeWindows(
    HWND            hwndParent,
    UINT            flags,
    CONST RECT *    lpRect,
    UINT            chwnd,
    CONST HWND *    ahwnd,
    MONITORENUMPROC lpfnEnum)
{
    ARRANGEWINDOWSDATA  awd;
    HWND *              phwnd = NULL;

    /*
     * Get parent window
     */
    awd.pwndDesktop = _GetDesktopWindow();
    if (!hwndParent) {
        hwndParent = HW(awd.pwndDesktop);
        awd.pwndParent = awd.pwndDesktop;
    } else {
        awd.pwndParent = ValidateHwnd(hwndParent);
        if (awd.pwndParent == NULL) {
            return 0;
        }
    }

    UnmaximizeChildWindows(hwndParent);

    /*
     * If the rect passed in contains the desktop window,
     * arrange the windows on the desktop
     */
    if ( lpRect &&
         awd.pwndParent == awd.pwndDesktop   &&
         lpRect->left   <= awd.pwndDesktop->rcClient.left   &&
         lpRect->top    <= awd.pwndDesktop->rcClient.top    &&
         lpRect->right  >= awd.pwndDesktop->rcClient.right  &&
         lpRect->bottom >= awd.pwndDesktop->rcClient.bottom ) {

        lpRect = NULL;
    }

    /*
     * Arrange iconic windows if appropriate, and determine flags
     * for getting the client rectangle if no rect is given.
     */
    if (lpRect == NULL) {
        if (    (   awd.pwndParent != awd.pwndDesktop ||
                    !(SYSMET(ARRANGE) & ARW_HIDE)) &&
                NtUserArrangeIconicWindows(hwndParent) != 0) {

            awd.uGRCFlags = GRC_SCROLLS | GRC_MINWNDS;
        } else {
            awd.uGRCFlags = GRC_SCROLLS;
        }
    }

    /*
     * Get window list
     */
    if (ahwnd == NULL) {
        HWND hwndChild;
        PWND pwndChild;

        pwndChild = REBASEPWND(awd.pwndParent, spwndChild);
        hwndChild = HW(pwndChild);
        if (    hwndChild == NULL ||
                (chwnd = BuildHwndList(NULL, hwndChild, FALSE, 0, &phwnd)) == 0) {
            return 0;
        }
    }

    /*
     * Arrange windows
     */
    awd.hdwp = NtUserBeginDeferWindowPos(chwnd);
    if (awd.hdwp == NULL)
        goto Done;

    awd.flags = flags;
    awd.lprcParent = (LPRECT) lpRect;
    awd.chwnd = chwnd;
    awd.chwndReal = 0;
    awd.phwnd = ahwnd ? (HWND *) ahwnd : phwnd;
    awd.fVerifyParent = (ahwnd != NULL);

    /*
     * If the parent is the desktop and a rectangle is not provided,
     * arrange the windows on each monitor. Otherwise, arrange the
     * windows once by calling the enumeration function directly.
     */
    if (awd.pwndParent == awd.pwndDesktop && lpRect == NULL) {
            NtUserEnumDisplayMonitors(NULL, NULL, lpfnEnum, (LPARAM) &awd);
    } else {
        (*lpfnEnum)(NULL, NULL, NULL, (LPARAM) &awd);
    }

    /* Make this arrangement asynchronous so we don't hang */
    if (awd.hdwp != NULL) {
        NtUserEndDeferWindowPosEx(awd.hdwp, TRUE);
    }

Done:
    if (phwnd) {
        UserLocalFree(phwnd);
    }

    return (awd.hdwp != NULL) ? awd.chwndReal : 0;
}



/***************************************************************************\
* GetParentArrangeRect
*
* Returns the rect passed in pawd if provided, otherwise gets the client
* rect of the parent window.
*
* History:
* 10-Jul-1997 adams     Created.
\***************************************************************************/

void
GetParentArrangeRect(PARRANGEWINDOWSDATA pawd, PMONITOR pMonitor, LPRECT lprc)
{
    UINT    uGRCFlags;

    if (pawd->lprcParent) {
        *lprc = *pawd->lprcParent;
    } else {
        uGRCFlags = pawd->uGRCFlags;

        /*
         * If icons are shown on the desktop, then they are always
         * shown on the primary monitor. So remove the GRC_MINWNDS
         * flag for monitors other than the primary.
         */
        if (pMonitor && pMonitor != GetPrimaryMonitor()) {
            uGRCFlags &= ~GRC_MINWNDS;
        }

        GetRealClientRect(
                pawd->pwndParent, lprc, uGRCFlags, pMonitor);
    }
}



/***************************************************************************\
* ValidatePositionableWindow
*
* Validates and returns a window if it is positionable, and sets
* the appropriate sizing flags.
*
* History:
* 10-Jul-1997 adams     Created.
\***************************************************************************/

PWND
ValidatePositionableWindow(
        HWND        hwndChild,
        PWND        pwndParent,
        PWND        pwndDesktop,
        DWORD       dwMDIFlags,
        PMONITOR    pMonitor,
        DWORD *     pdwSWPFlags)
{
    PWND    pwndChild;

    pwndChild = ValidateHwnd(hwndChild);
    if (pwndChild) {
        if (pwndParent && REBASEPWND(pwndChild, spwndParent) != pwndParent) {
            RIPMSG0(RIP_WARNING, "Cascade/Tile Windows: Windows in list must have same parent");
        } else if (
                /*
                 * mikesch - removed the maximized check since the call
                 * to restore maximized windows in unmaximizechildwindows occurs
                 * asynchronously now.
                 */
                TestWF(pwndChild, WFVISIBLE) &&
                TestWF(pwndChild, WFCPRESENT) &&
                !TestWF(pwndChild, WFMINIMIZED) &&
                !TestWF(pwndChild, WEFTOPMOST) &&
                (!(dwMDIFlags & MDITILE_SKIPDISABLED) || !TestWF(pwndChild, WFDISABLED)) &&
                !TestWF(pwndChild, WEFTOOLWINDOW) &&
                ((pMonitor) ?
                    (pMonitor == _MonitorFromWindow(pwndChild, MONITOR_DEFAULTTONULL)) :
                    (pwndParent != pwndDesktop || _MonitorFromWindow(pwndChild, MONITOR_DEFAULTTONULL)))) {

                    if (pdwSWPFlags) {
                        *pdwSWPFlags = SWP_NOACTIVATE | SWP_NOCOPYBITS;
                        if (!TestWF(pwndChild, WFSIZEBOX)) {
                            *pdwSWPFlags |= SWP_NOSIZE;
                        }
                        if (!(dwMDIFlags & MDITILE_ZORDER)) {
                            *pdwSWPFlags |= SWP_NOZORDER;
                        }
                    }
            return pwndChild;
        }
    }

    return NULL;
}



/***************************************************************************\
* CascadeWindowsEnum
*
* Cascades windows on the monitor.
*
* History:
* 10-Jul-1997 adams     Created.
\***************************************************************************/

BOOL CALLBACK
CascadeWindowsEnum(
        HMONITOR    hmonitor,
        HDC         hdc,
        LPRECT      lprc,
        LPARAM      lparam)
{
    PARRANGEWINDOWSDATA pawd = (PARRANGEWINDOWSDATA)lparam;
    PMONITOR    pMonitor = hmonitor ? VALIDATEHMONITOR(hmonitor) : NULL;
    RECT        rcParent;
    int         i;
    int         chwndReal = 0;
    RECT        rc;
    HWND        * phwnd, * phwndCopy;
    BOOL        fRet = TRUE;

    UNREFERENCED_PARAMETER(hdc);
    UNREFERENCED_PARAMETER(lprc);

    /*
     * Get the parent rectangle if none is given.
     */
    GetParentArrangeRect(pawd, pMonitor, &rcParent);

    /*
     * New for NT5: MDITILE_ZORDER (for the SHELL guys)
     * Sort pawd->phwnd by z-order
     */
    if (pawd->flags & MDITILE_ZORDER) {
        PWND pwndChild;
        HWND * phwndFullList, * phwndNext, * phwndSort, * phwndSearch;
        int chwndFullList, chwndSort, chwndSearch;
        /*
         * Make a copy to leave their array alone (it's supposed to be const)
         */
        phwndCopy = UserLocalAlloc(0, pawd->chwnd * sizeof(HWND));
        if (phwndCopy == NULL) {
            return FALSE;
        }
        RtlCopyMemory(phwndCopy, pawd->phwnd, pawd->chwnd * sizeof(HWND));
        /*
         * Get the sibblings Z-Ordered list.
         */
        pwndChild = REBASEPWND(pawd->pwndParent, spwndChild);
        if (pwndChild == NULL) {
            fRet = FALSE;
            goto CleanUp;
        }
        chwndFullList = BuildHwndList(NULL, HWq(pwndChild), FALSE, 0, &phwndFullList);
        if (chwndFullList == 0) {
            fRet = FALSE;
            goto CleanUp;
        }
        /*
         * Loop through the Z-Ordered list looking for the windows in the array
         */
        for (phwndNext = phwndFullList, chwndSort = pawd->chwnd, phwndSort = phwndCopy;
                (chwndFullList > 0) && (chwndSort > 1);
                chwndFullList--, phwndNext++) {

            for (chwndSearch = chwndSort, phwndSearch = phwndSort;
                    chwndSearch > 0;
                    chwndSearch--, phwndSearch++) {
                /*
                 * If it found a window, move it after the last sorted window.
                 */
                if (*phwndNext == *phwndSearch) {
                    HWND hwndFirst = *phwndSort;
                    *phwndSort = *phwndSearch;
                    *phwndSearch = hwndFirst;
                    phwndSort++;
                    chwndSort--;
                    break;
                }
            }
        }
        UserLocalFree(phwndFullList);
    } else { /* if (pawd->flags & MDITILE_ZORDER) */
        phwndCopy = pawd->phwnd;
    }

    /*
     * Arrange the windows in the list, preserving z-order.
     */
    for (i = pawd->chwnd, phwnd = phwndCopy + i - 1; --i >= 0; phwnd--) {
        HWND    hwndChild;
        PWND    pwndChild = NULL;
        DWORD   dwSWPFlags;

        hwndChild = *phwnd;
        pwndChild = ValidatePositionableWindow(
                hwndChild,
                pawd->fVerifyParent ? pawd->pwndParent : NULL,
                pawd->pwndDesktop,
                pawd->flags,
                pMonitor,
                &dwSWPFlags);

        if (!pwndChild)
            continue;

        GetCascadeWindowPos(&rcParent, chwndReal, &rc);

        pawd->hdwp = NtUserDeferWindowPos(
                pawd->hdwp,
                hwndChild,
                HWND_TOP,
                rc.left,
                rc.top,
                rc.right,
                rc.bottom,
                dwSWPFlags);

        chwndReal++;
        pawd->chwndReal++;
    }

CleanUp:
    if (pawd->flags & MDITILE_ZORDER) {
        UserLocalFree(phwndCopy);
    }

    return fRet && (pawd->hdwp != NULL);
}



/***************************************************************************\
*
*  CascadeWindows()
*
*  Cascades a list of children within a parent, according to the flags and
*  the rectangle passed in.
*
\***************************************************************************/
WORD CascadeWindows(
    HWND hwndParent,
    UINT flags,
    CONST RECT *lpRect,
    UINT chwnd,
    CONST HWND *ahwnd)
{
    return ArrangeWindows(hwndParent, flags, lpRect, chwnd, ahwnd, CascadeWindowsEnum);
}

BOOL CALLBACK
TileWindowsEnum(
        HMONITOR    hmonitor,
        HDC         hdc,
        LPRECT      lprc,
        LPARAM      lparam)
{
    PARRANGEWINDOWSDATA pawd = (PARRANGEWINDOWSDATA)lparam;
    PMONITOR    pMonitor = hmonitor ? VALIDATEHMONITOR(hmonitor) : NULL;
    RECT        rcParent;
    int         ihwnd;
    int         chwndReal;
    int         square;
    int         iCol, iRow;
    int         cCol, cRow;
    int         cRem;
    int         dx, dy;
    int         xRes, yRes;

    UNREFERENCED_PARAMETER(hdc);
    UNREFERENCED_PARAMETER(lprc);

    /*
     * Get the parent rectangle if none is given.
     */
    GetParentArrangeRect(pawd, pMonitor, &rcParent);

    /*
     * Now, figure out how many REAL windows we have.
     */
    chwndReal = 0;
    for (ihwnd = pawd->chwnd; --ihwnd >= 0;) {
        if (ValidatePositionableWindow(
                pawd->phwnd[ihwnd],
                pawd->fVerifyParent ? pawd->pwndParent : NULL,
                pawd->pwndDesktop,
                pawd->flags,
                pMonitor,
                NULL)) {

            chwndReal++;
        }
    }

    if (chwndReal == 0)
        return TRUE;

    xRes = rcParent.right - rcParent.left;
    yRes = rcParent.bottom - rcParent.top;
    if (xRes <= 0 || yRes <= 0)
        return TRUE;

    /*
     * Compute nearest least square
     */
    for (square = 2; square * square <= chwndReal; square++) {
        ;
    }

    if (pawd->flags & MDITILE_HORIZONTAL) {
        cCol = square - 1;
        cRow = chwndReal / cCol;
        cRem = chwndReal % cCol;
    } else {
        cRow = square - 1;
        cCol = chwndReal / cRow;
        cRem = chwndReal % cRow;
    }

    chwndReal = 0;
    ihwnd = -1;
    for (iCol = 0; iCol < cCol; iCol++) {
        /*
         * Add one extra row to handle the remainders.
         */
        if (cCol - iCol <= cRem) {
            cRow++;
        }

        for (iRow = 0; iRow < cRow; iRow++) {
            HWND    hwndChild;
            PWND    pwndChild;
            DWORD   dwSWPFlags;

            dx = xRes / cCol;
            dy = yRes / cRow;

NextWindow:
            /*
             * Skip bogus and nonpositionable windows
             */
            ihwnd++;
            if (ihwnd >= pawd->chwnd)
                return TRUE;

            hwndChild = pawd->phwnd[ihwnd];
            pwndChild = ValidatePositionableWindow(
                    hwndChild,
                    pawd->fVerifyParent ? pawd->pwndParent : NULL,
                    pawd->pwndDesktop,
                    pawd->flags,
                    pMonitor,
                    &dwSWPFlags);

            if (!pwndChild)
                goto NextWindow;

            /*
             * Move, size the window
             */
            pawd->hdwp = NtUserDeferWindowPos(
                    pawd->hdwp,
                    hwndChild,
                    HWND_TOP,
                    rcParent.left + iCol*dx,
                    rcParent.top + iRow*dy,
                    dx,
                    dy,
                    dwSWPFlags);

            if (pawd->hdwp == NULL)
                return FALSE;

            chwndReal++;
            pawd->chwndReal++;
        }

        if (cCol - iCol <= cRem) {
            cRow--;
            cRem--;
        }
    }

    return TRUE;
}



/***************************************************************************\
*
*  TileWindows()
*
*  Tiles a list of children within a parent, according to the flags and
*  the rectangle passed in.
*
\***************************************************************************/
WORD TileWindows(
    HWND        hwndParent,
    UINT        flags,
    CONST RECT *lpRect,
    UINT        chwnd,
    CONST HWND *ahwnd)
{
    return ArrangeWindows(hwndParent, flags, lpRect, chwnd, ahwnd, TileWindowsEnum);
}



/***************************************************************************\
* xxxMDIActivate
*
* History:
* 11-14-90 MikeHar Ported from windows
*  4-16-91 MikeHar Win31 Merge
\***************************************************************************/

void xxxMDIActivate(
    PWND pwnd,
    PWND pwndActivate)
{
    HWND hwndOld;
    PWND pwndOld;

    PMDI pmdi;
    BOOL fShowActivate;
    UINT nID;
    TL tlpwnd;
    TL tlpwndOld;
    PWND pwndT;
    HWND hwnd = HWq(pwnd);
    HWND hwndActivate = HW(pwndActivate);

    CheckLock(pwnd);
    CheckLock(pwndActivate);

    /*
     * Get a pointer to the MDI structure
     */
    pmdi = ((PMDIWND)pwnd)->pmdi;

    if (ACTIVE(pmdi) == hwndActivate)
        return;

    if ((pwndActivate != NULL) && (TestWF(pwndActivate, WFDISABLED))) {
        /*
         * Don't even try activating disabled or invisible windows.
         */
        return;
    }

    pwndT = REBASEPWND(pwnd, spwndParent);
    fShowActivate = (HW(pwndT) ==
            NtUserQueryWindow(hwnd, WindowActiveWindow));

    hwndOld = ACTIVE(pmdi);
    if (hwndOld && (pwndOld = ValidateHwnd(hwndOld)) == NULL) {
        hwndOld = NULL;
    }
    ThreadLock(pwndOld, &tlpwndOld);

    if (hwndOld) {

        /*
         * Attempt to deactivate the MDI child window.
         * The MDI child window can fail deactivation by returning FALSE.
         * But this only applies if the MDI frame is the active window
         * and the app is a 3.1 app or later
         */
        if (!SendMessage(hwndOld, WM_NCACTIVATE, FALSE, 0L) && fShowActivate) {
            if (TestWF(pwndOld, WFWIN31COMPAT))
                goto UnlockOld;
        }

        if (!TestWF(pwndOld, WFWIN31COMPAT) && TestWF(pwndOld, WFFRAMEON)) {

            /*
             * Error: Quicken for Windows is sort of bogus.  They try to fail
             * the WM_NCACTIVATE of their newly created window by not passing
             * it to DefWindowProc.  Bug 6412.  WM_NCACTIVATE sets/unsets the
             * WFFRAMEON bit if passed to DWP so we can double check things
             * here.
             */
            goto UnlockOld;
        }

        SendMessage(hwndOld, WM_MDIACTIVATE, (WPARAM)hwndOld, (LPARAM)hwndActivate);

        /*
         * Uncheck the old window menu entry.
         */
        if (WINDOW(pmdi))
            CheckMenuItem(WINDOW(pmdi), PtrToUlong(pwndOld->spmenu),
                MF_BYCOMMAND | MF_UNCHECKED);
    }

    //
    // Handle switching to a new (or NO) maximized window.  If NO window is
    // to become maximized, because we're activating NULL or the window to
    // become active doesn't have a WS_MAXIMIZEBOX, restore the old one to
    // it's normal size to clean up the MDI maximized menubar mess
    //
    if (MAXED(pmdi) && MAXED(pmdi) != hwndActivate) {
        HWND hwndMax;
        int  iShowCode;

        // The MAXBOX check is a new thing for 4.0 dudes; it breaks 3.x apps.
        // See comment in the WM_MDIMAXIMIZE handling.

        if (pwndActivate && (TestWF(pwndActivate, WFMAXBOX) || !TestWF(pwndActivate, WFWIN40COMPAT))) {
            hwndMax = hwndActivate;
            iShowCode = SW_SHOWMAXIMIZED;
            Lock(&ACTIVE(pmdi), hwndMax);
        } else {
            hwndMax = MAXED(pmdi);
            iShowCode = SW_SHOWNORMAL;
        }

        // overload WFFULLSCREEN bit -- useless for child windows anyways
        // use it to indicate to min/max code to not animate size change.

        // NO -- no bit overloading, damn it.  FritzS
        NtUserCallHwndParam(hwndMax, WFNOANIMATE, SFI_SETWINDOWSTATE);
        NtUserShowWindow(hwndMax, iShowCode);
        NtUserCallHwndParam(hwndMax, WFNOANIMATE, SFI_CLEARWINDOWSTATE);
    }

    Lock(&ACTIVE(pmdi), hwndActivate);

    /*
     * We may be removing the activation entirely...
     */
    if (!pwndActivate) {
        if (fShowActivate)
            NtUserSetFocus(hwnd);
        goto UnlockOld;
    }

    if (WINDOW(pmdi)) {

        /*
         * Check the new window menu entry.
         */
        nID = GetWindowID(ACTIVE(pmdi));
        if (nID - FIRST(pmdi) < (MAXITEMS - 1)) {
            CheckMenuItem(WINDOW(pmdi), nID, MF_BYCOMMAND | MFS_CHECKED);
        } else {
            /*
             * the item is not in the menu at all!  Swap it with number 9.
             */
            PWND pwndOther = FindPwndChild(pwnd, (UINT)(FIRST(pmdi) + MAXITEMS - 2));

            SetWindowLongPtr(HW(pwndOther), GWLP_ID, PtrToLong(pwndActivate->spmenu));
            SetWindowLongPtr(hwndActivate, GWLP_ID, FIRST(pmdi) + MAXITEMS - 2);

            ModifyMenuItem(pwndActivate);
        }
    }

    /*
     * Bring the window to the top.
     */
    NtUserSetWindowPos(ACTIVE(pmdi), NULL, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

    /*
     * Update the Caption bar.  Don't muck with styles for 3.1.
     */
    if (fShowActivate) {
        SendMessage(ACTIVE(pmdi), WM_NCACTIVATE, TRUE, 0L);

        ThreadLock(pwnd, &tlpwnd);

        if (hwnd == NtUserQueryWindow(hwnd, WindowFocusWindow))
            SendMessage(hwnd, WM_SETFOCUS, (WPARAM)hwnd, 0);
        else
            NtUserSetFocus(hwnd);

        ThreadUnlock(&tlpwnd);
    }

    /*
     * Notify the new active window of his activation.
     */
    SendMessage(ACTIVE(pmdi), WM_MDIACTIVATE, (WPARAM)hwndOld,
                (LPARAM)hwndActivate);

UnlockOld:
    ThreadUnlock(&tlpwndOld);
}


/***************************************************************************\
* xxxMDINext
*
* History:
* 11-14-90 MikeHar Ported from windows
*  4-16-91 MikeHar Win31 Merge
\***************************************************************************/

void xxxMDINext(
    PWND pwndMDI,
    PWND pwnd,
    BOOL fPrevWindow)
{
    PMDI pmdi;
    PWND pwndNextGuy;
    HDWP hdwp;
    BOOL fHack = FALSE;

    CheckLock(pwndMDI);
    CheckLock(pwnd);

    /*
     * Get a pointer to the MDI structure
     */
    pmdi = ((PMDIWND)pwndMDI)->pmdi;

    pwndNextGuy = pwnd;

    while (TRUE) {
        if (fPrevWindow)
            pwndNextGuy = _GetWindow(pwndNextGuy, GW_HWNDPREV);
        else
            pwndNextGuy = REBASEPWND(pwndNextGuy, spwndNext);

        if (!pwndNextGuy) {
            if (fPrevWindow) {
                pwndNextGuy = _GetWindow(pwnd, GW_HWNDLAST);
            } else {
                pwndNextGuy =  REBASEPWND(pwndMDI, spwndChild);
            }
        }

        if (pwndNextGuy == pwnd)
            return;


        //
        // Ignore hidden and disabled windows.
        //
        if (TestWF(pwndNextGuy, WFVISIBLE) && !TestWF(pwndNextGuy, WFDISABLED))
            break;
    }

    if (MAXED(pmdi)) {
        NtUserSetVisible(HWq(pwndMDI), SV_UNSET | SV_CLRFTRUEVIS);
        fHack = TRUE;
    }

    hdwp = NtUserBeginDeferWindowPos(2);

    /*
     * activate the new window (first, in case of maximized windows)
     */
    hdwp = NtUserDeferWindowPos(hdwp, HW(pwndNextGuy), HWND_TOP, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE);

// LATER 30-Mar-1992 mikeke
// this used to be _GetWindow(pwndMDI->spwndChild, GW_HWNDLAST)
// instead of HWND_BOTTOM
   if (hdwp && !fPrevWindow && (pwnd != pwndNextGuy))
       hdwp = NtUserDeferWindowPos(hdwp, HW(pwnd),
            HWND_BOTTOM, 0, 0, 0, 0,
            SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE );

    NtUserEndDeferWindowPosEx(hdwp, FALSE);

    if (fHack) {
        NtUserShowWindow(HWq(pwndMDI), SW_SHOW);
    }
}


HWND
CreateMDIWindowA(
    LPCSTR pClassName,
    LPCSTR pWindowName,
    DWORD dwStyle,
    int x,
    int y,
    int nWidth,
    int nHeight,
    HWND hwndParent,
    HINSTANCE hModule,
    LPARAM lParam)
{
    return CreateWindowExA(WS_EX_MDICHILD, pClassName, pWindowName,
                                 dwStyle, x, y, nWidth, nHeight,
                                 hwndParent, NULL, hModule, (LPVOID)lParam);
}


HWND
CreateMDIWindowW(
    LPCWSTR pClassName,
    LPCWSTR pWindowName,
    DWORD dwStyle,
    int x,
    int y,
    int nWidth,
    int nHeight,
    HWND hwndParent,
    HINSTANCE hModule,
    LPARAM lParam)
{
    return CreateWindowExW(WS_EX_MDICHILD, pClassName, pWindowName,
                                 dwStyle, x, y, nWidth, nHeight,
                                 hwndParent, NULL, hModule, (LPVOID)lParam);
}


/***************************************************************************\
* xxxMDIDestroy
*
* History:
* 11-14-90 MikeHar Ported from windows
*  4-16-91 MikeHar Win31 Merge
\***************************************************************************/

void xxxMDIDestroy(
    PWND pwnd,
    HWND hwndVictim)
{
    PWND pwndVictim;
    TL tlpwndParent;
    PMDI pmdi;
    PWND pwndParent;
    HWND hwnd;

    CheckLock(pwnd);

    if ((pwndVictim = ValidateHwnd(hwndVictim)) == NULL) {
        return;
    }
    CheckLock(pwndVictim);

    /*
     * Get a pointer to the MDI structure
     */
    pmdi = ((PMDIWND)pwnd)->pmdi;

#ifdef NEVER
// don't do this validation - because it sometimes doesn't work! If an
// app passed in idFirstChild (through CLIENTCREATESTRUCT) as -1, this
// code fails because it treats the id comparisons as unsigned compares.
// Change them to signed compares and it still doesn't work. That is because
// when ShiftMenuIDs() is called, you'll shift mdi windows out of the signed
// comparison range and this check won't allow them to be destroyed. This
// is straight win3.1 code.
//
    /*
     * Validate that this is one of the mdi children we are keeping track
     * of. If it isn't don't destroy it because it'll get mdi id tracking
     * code all screwed up.
     */
    if (((UINT)pwndVictim->spmenu) < FIRST(pmdi) ||
            ((UINT)pwndVictim->spmenu) >= (UINT)(FIRST(pmdi) + CKIDS(pmdi)) ||
            pwndVictim->spwndOwner != NULL) {
        RIPERR0(ERROR_NON_MDICHILD_WINDOW, RIP_VERBOSE, "");
        return;
    }
#endif

    ShiftMenuIDs(pwnd, pwndVictim);

    /*
     * First Activate another window.
     */
    if (SAMEWOWHANDLE(hwndVictim, ACTIVE(pmdi))) {
        xxxMDINext(pwnd, pwndVictim, FALSE);

        /*
         * Destroying only child?
         */
        if (SAMEWOWHANDLE(hwndVictim, ACTIVE(pmdi))) {
            NtUserShowWindow(hwndVictim, SW_HIDE);

            /*
             * If the window is maximized, we need to remove his sys menu
             * now otherwise it may get deleted twice.  Once when the child
             * is destroyed and once when the frame is destroyed.
             */
            if (MAXED(pmdi)) {
                pwndParent = REBASEPWND(pwnd, spwndParent);
                MDIRemoveSysMenu(PtoH(REBASE(pwndParent,spmenu)), MAXED(pmdi));
                Unlock(&MAXED(pmdi));
                ThreadLock(pwndParent, &tlpwndParent);
                xxxSetFrameTitle(pwndParent, pwnd, (LPWSTR)1L);

                /*
                 * Redraw frame so menu bar shows the removed sys menu stuff
                 */
                if (TestWF(pwndParent, WFVISIBLE))
                    NtUserRedrawFrame(HWq(pwndParent));
                ThreadUnlock(&tlpwndParent);
            }
            xxxMDIActivate(pwnd, NULL);
        }
    }

    /*
     * Don't ever let this go negative or we'll get caught in long loops.
     */
    CKIDS(pmdi)--;
    if ((int)CKIDS(pmdi) < 0)
        CKIDS(pmdi) = 0;

    hwnd = HWq(pwnd);
    SendMessage(hwnd, WM_MDIREFRESHMENU, 0L, 0L);

    /*
     * Destroy the window.
     */
    NtUserDestroyWindow(hwndVictim);


    /*
     * During the DestroyWindow the parent may also have been deleted
     * Remove revalidate if we get client side locking
     */
    if (ValidateHwnd(hwnd) == NULL)
       return;

    /*
     * Deleting a window can change the scroll ranges.
     */
    RecalculateScrollRanges(pwnd, FALSE);
}

/***************************************************************************\
* MDIClientWndProc
*
* History:
* 11-14-90 MikeHar Ported from windows
\***************************************************************************/

LRESULT MDIClientWndProcWorker(
    PWND pwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    DWORD fAnsi)
{
    HWND hwnd = HWq(pwnd);
    HWND hwndT;
    PWND pwndT;
    TL tlpwndT;
    PMDI pmdi;
    PWND pwndParent;

    CheckLock(pwnd);

    VALIDATECLASSANDSIZE(pwnd, FNID_MDICLIENT);

    /*
     * Get the pmdi for the given window now since we will use it a lot in
     * various handlers. This was stored using SetWindowLong(hwnd,4,pmdi) when
     * we initially created the MDI client window.
     */
    pmdi = ((PMDIWND)pwnd)->pmdi;

    if (pmdi == NULL) {
        switch (message) {
        case WM_MDICREATE:
        case WM_MDIMAXIMIZE:
        case WM_PARENTNOTIFY:
        case WM_CREATE:
            /*
             * These messages are safe to call, even when pmdi has not already
             * been initialized.
             */
            break;
        
        default:
            /*
             * Any message that is not listed above is not safe to call when
             * pmdi has not been initialized.  Instead, just directly call DWP.
             */
            goto CallDWP;
        }
    }

    switch (message) {
    case WM_NCACTIVATE:

        /*
         * We are changing app activation.  Fix the active child's caption.
         */
        if (ACTIVE(pmdi) != NULL) {
            SendMessage(ACTIVE(pmdi), WM_NCACTIVATE, wParam, lParam);
        }
        goto CallDWP;

    case WM_MDIGETACTIVE:
        if (lParam != 0) {
            *((LPBOOL)lParam) = (MAXED(pmdi) != NULL);
        }

        return (LRESULT)ACTIVE(pmdi);

    case WM_MDIACTIVATE:
        hwndT = (HWND)wParam;
        if ((pwndT = ValidateHwnd(hwndT)) == NULL)
            return 0;

        if (SAMEWOWHANDLE(hwndT, ACTIVE(pmdi)))
              break;

        NtUserSetWindowPos(hwndT, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        break;

    case WM_MDICASCADE:
        pmdi->wScroll |= SCROLLSUPPRESS;
        NtUserShowScrollBar(hwnd, SB_BOTH, FALSE);

        /*
         * Unmaximize any maximized window.
         */
#ifdef NEVER  // Not in Chicago -- FritzS
        if (MAXED(pmdi) != NULL) {
            NtUserShowWindow(MAXED(pmdi), SW_SHOWNORMAL);
        }
#endif
        /*
         * Save success/failure code to return to app
         */
        message = (UINT)CascadeWindows(hwnd, (UINT)wParam, NULL, 0, NULL);
        pmdi->wScroll &= ~SCROLLCOUNT;
        return (LONG)message;
        break;

    case WM_VSCROLL:
    case WM_HSCROLL:
        pmdi->wScroll |= SCROLLSUPPRESS;
        ScrollMDIChildren(hwnd, (message == WM_VSCROLL) ? SB_VERT : SB_HORZ,
              LOWORD(wParam), (short)(HIWORD(wParam)));
        pmdi->wScroll &= ~SCROLLCOUNT;
        break;

    case WM_MDICREATE:
        {
        LPMDICREATESTRUCTA lpMCSA = (LPMDICREATESTRUCTA)lParam;
        LPMDICREATESTRUCTW lpMCSW = (LPMDICREATESTRUCTW)lParam;
        DWORD exStyle = WS_EX_MDICHILD;

        /*
         * inherit the right.to.leftness of the parent.
         */
        exStyle |= (pwnd->ExStyle & (WS_EX_RIGHT | WS_EX_RTLREADING | WS_EX_LEFTSCROLLBAR));

        if (fAnsi) {
            hwndT = CreateWindowExA(exStyle, lpMCSA->szClass, lpMCSA->szTitle,
                lpMCSA->style, lpMCSA->x, lpMCSA->y, lpMCSA->cx, lpMCSA->cy,
                hwnd, NULL, lpMCSA->hOwner, (LPSTR)lpMCSA->lParam);
        } else {
            hwndT = CreateWindowExW(exStyle, lpMCSW->szClass, lpMCSW->szTitle,
                lpMCSW->style, lpMCSW->x, lpMCSW->y, lpMCSW->cx, lpMCSW->cy,
                hwnd, NULL, lpMCSW->hOwner, (LPWSTR)lpMCSW->lParam);
        }

        return((LRESULT)hwndT);

        }

    case WM_MDIDESTROY:
        xxxMDIDestroy(pwnd, (HWND)wParam);
        break;

    case WM_MDIMAXIMIZE:
        hwndT = (HWND)wParam;
        if ((pwndT = ValidateHwnd(hwndT)) == NULL)
            return 0;

        // Only maximize children with a MAXBOX.  However, this introduces
        // backwards-compatibility issues with VB apps (see#12211)
        // So, we do this only for WIN40COMPAT apps and beyond.
        //
        if ((TestWF(pwndT, WFMAXBOX)) || !(TestWF(pwndT, WFWIN40COMPAT))) {
            NtUserShowWindow(hwndT, SW_SHOWMAXIMIZED);
        }
        break;

    case WM_MDIRESTORE:
        hwndT = (HWND)wParam;
        if ((pwndT = ValidateHwnd(hwndT)) == NULL)
            return 0;

        NtUserShowWindow(hwndT, SW_SHOWNORMAL);
        break;

    case WM_MDITILE:
        pmdi->wScroll |= SCROLLSUPPRESS;
        NtUserShowScrollBar(hwnd, SB_BOTH, FALSE);

        /*
         * Unmaximize any maximized window.
         */
#ifdef NEVER  //Not in Chicago
        if (MAXED(pmdi) != NULL) {
            NtUserShowWindow(MAXED(pmdi), SW_SHOWNORMAL);
        }
#endif
        /*
         * Save success/failure code to return to app
         */
        message = (UINT)TileWindows(hwnd, (UINT)wParam, NULL, 0, NULL);
        pmdi->wScroll &= ~SCROLLCOUNT;
        return (LONG)message;
        break;

    case WM_MDIICONARRANGE:
        pmdi->wScroll |= SCROLLSUPPRESS;
        NtUserArrangeIconicWindows(hwnd);
        pmdi->wScroll &= ~SCROLLCOUNT;
        RecalculateScrollRanges(pwnd, TRUE);
        break;

    case WM_MDINEXT:
        if (wParam) {
            hwndT = (HWND)wParam;
        } else {
            hwndT = ACTIVE(pmdi);
        }

        if ((pwndT = ValidateHwnd(hwndT)) == NULL) {
            return 0;
        }

        /*
         * If lParam is 1, do a prev window instead of a next window
         */
        ThreadLockAlways(pwndT, &tlpwndT);
        xxxMDINext(pwnd, pwndT, (lParam == 0 ? 0 : 1));
        ThreadUnlock(&tlpwndT);
        break;

    case WM_MDIREFRESHMENU:
            return (LRESULT)MDISetMenu(pwnd, TRUE, NULL, NULL);

    case WM_MDISETMENU:
            return (LRESULT)MDISetMenu(pwnd, FALSE, (HMENU)wParam, (HMENU)lParam);

    case WM_PARENTNOTIFY:
        if (wParam == WM_LBUTTONDOWN) {
            HWND hwndChild;
            POINT pt;

#ifdef USE_MIRRORING
            if ((pwndT = ValidateHwnd(hwnd)) == NULL)
                return 0;
#endif

            /*
             * Activate this child and bring it to the top.
             */
            pt.x = (int)MAKEPOINTS(lParam).x;
            pt.y = (int)MAKEPOINTS(lParam).y;

#ifdef USE_MIRRORING
            /*
             * Since pt is relative to the client MDI window,
             * then the points should be mirrored if the MDI
             * client window is mirrored so that Scrren Coord
             * calculations are done properly in NtUserChildWindowFromPointEx.
             * [samera]
             */
            if (TestWF(pwndT,WEFLAYOUTRTL)) {
                pt.x = (pwndT->rcClient.right-pwndT->rcClient.left)-pt.x;
            }
#endif

            hwndChild = NtUserChildWindowFromPointEx(hwnd, pt,
                CWP_SKIPDISABLED | CWP_SKIPINVISIBLE);

            if ((hwndChild) && (hwndChild != hwnd)) {

                if (hwndChild != ACTIVE(pmdi)) {
                    NtUserSetWindowPos(hwndChild, HWND_TOP, 0, 0, 0, 0,
                                 SWP_NOMOVE | SWP_NOSIZE);
                }
            }
        }
        break;

    case WM_SETFOCUS:
        if (ACTIVE(pmdi) != NULL && !IsIconic(ACTIVE(pmdi))) {
            NtUserSetFocus(ACTIVE(pmdi));
        }
        break;

    case WM_SIZE:
        if (ACTIVE(pmdi) && (pwndT = ValidateHwnd(ACTIVE(pmdi))) &&
            TestWF(pwndT, WFMAXIMIZED)) {

            RECT rc;

            rc.top = rc.left = 0;
            rc.right = (int)MAKEPOINTS(lParam).x;
            rc.bottom = (int)MAKEPOINTS(lParam).y;
            _AdjustWindowRectEx(&rc, pwndT->style, FALSE,
                    pwndT->ExStyle);
            NtUserMoveWindow(ACTIVE(pmdi), rc.left, rc.top,
                    rc.right - rc.left, rc.bottom - rc.top, TRUE);
        } else {
            RecalculateScrollRanges(pwnd, FALSE);
        }
        goto CallDWP;

    case MM_CALCSCROLL: {

        if (SCROLL(pmdi) & SCROLLCOUNT)
            break;

        {
            WORD sbj = pmdi->wScroll & (HAS_SBVERT | HAS_SBHORZ);

            if (sbj)
            {
                CalcClientScrolling(hwnd, sbj, (BOOL) wParam);

                SCROLL(pmdi) &= ~CALCSCROLL;
            }
        }
        break;
    }

    case WM_CREATE: {
        LPCLIENTCREATESTRUCT pccs = ((LPCREATESTRUCT)lParam)->lpCreateParams;

        /*
         * Try to allocate space for the pmdi
         */
        if ((pmdi = (PMDI)UserLocalAlloc(HEAP_ZERO_MEMORY, sizeof(MDI)))) {
            NtUserSetWindowLongPtr(hwnd, GWLP_MDIDATA, (LONG_PTR)pmdi, FALSE);
        } else {
            NtUserSetWindowFNID(hwnd, FNID_CLEANEDUP_BIT);
            break;
        }

        pwndParent = REBASEPWND(pwnd, spwndParent);
        ACTIVE(pmdi) = NULL;
        MAXED(pmdi)  = NULL;
        CKIDS(pmdi)  = 0;
        WINDOW(pmdi) = pccs->hWindowMenu;

        FIRST(pmdi)  = pccs->idFirstChild;
        SCROLL(pmdi) = 0;
        HTITLE(pmdi) = TextAlloc(REBASE(pwndParent, strName.Buffer));

        _DefSetText(HW(pwndParent), NULL, FALSE);

        ThreadLock(pwndParent, &tlpwndT);
        xxxSetFrameTitle(pwndParent, pwnd, (LPWSTR)2L);
        ThreadUnlock(&tlpwndT);

        if (TestWF(pwnd, WFVSCROLL))
            SCROLL(pmdi) |= HAS_SBVERT;
        if (TestWF(pwnd, WFHSCROLL))
            SCROLL(pmdi) |= HAS_SBHORZ;
        if (SCROLL(pmdi)) {
            ClearWindowState(pwnd, WFVSCROLL | WFHSCROLL);
        }

        /*
         * Set this dude's system menu.
         */
        NtUserGetSystemMenu(HW(pwndParent), FALSE);

        /*
         * make sure we have the correct window client area if scrolls are
         * removed...  hack to take care of small progman bug
         */
        if (SCROLL(pmdi)) {
            NtUserUpdateClientRect(hwnd);
        }
        break;
    }

    case WM_DESTROY:
    case WM_FINALDESTROY:
        if (MAXED(pmdi)) {
            PWND pwndParent;
            PMENU pmenu;

            pwndParent = REBASEPWND(pwnd, spwndParent);
            pmenu = REBASE(pwndParent, spmenu);
            MDIRemoveSysMenu(PtoH(pmenu), MAXED(pmdi));
        }

        /*
         * delete the title
         */
        if (HTITLE(pmdi)) {
            UserLocalFree(HTITLE(pmdi));
            HTITLE(pmdi) = NULL;
        }

        /*
         * Delete the menu items of the child windows in the frame.
         * Chances are, this is called by destroying the frame, but
         * one never knows, does one?
         *
         * Increase CKIDS by 1 after checking to delete the separator
         */
        if (IsMenu(WINDOW(pmdi)) && CKIDS(pmdi)++) {
            UINT iPosition;

            if (CKIDS(pmdi) > MAXITEMS + 1)
                CKIDS(pmdi) = MAXITEMS + 1;

            iPosition = GetMenuItemCount(WINDOW(pmdi));
            while (CKIDS(pmdi)--) {
                NtUserDeleteMenu(WINDOW(pmdi), --iPosition, MF_BYPOSITION);
            }
        }

        /*
         * Unlock those objects that are used by the MDI structure.
         */
        Unlock(&MAXED(pmdi));
        Unlock(&ACTIVE(pmdi));
        Unlock(&WINDOW(pmdi));

        /*
         * Free the MDI structure
         */
        UserLocalFree(pmdi);
        NtUserSetWindowFNID(hwnd, FNID_CLEANEDUP_BIT);

        break;

    default:
CallDWP:
        return DefWindowProcWorker(pwnd, message, wParam, lParam, fAnsi);
    }
    return 0L;
}

/***************************************************************************\
*
\***************************************************************************/

LRESULT WINAPI MDIClientWndProcA(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    PWND pwnd;

    if ((pwnd = ValidateHwnd(hwnd)) == NULL) {
        return 0;
    }

    return MDIClientWndProcWorker(pwnd, message, wParam, lParam, TRUE);
}

LRESULT WINAPI MDIClientWndProcW(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    PWND pwnd;

    if ((pwnd = ValidateHwnd(hwnd)) == NULL) {
        return 0;
    }

    return MDIClientWndProcWorker(pwnd, message, wParam, lParam, FALSE);
}


/***************************************************************************\
* DefFrameProc
*
* History:
* 11-14-90 MikeHar      Ported from windows
\***************************************************************************/

LRESULT DefFrameProcWorker(
    HWND hwnd,
    HWND hwndMDI,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam,
    BOOL fAnsi)
{
    PWND pwnd;
    PWND pwndMDI;
    PMDI pmdi;
    TL tlpwndT;
    HWND hwndT;
    PWND pwndT;
    PMDINEXTMENU pmnm;
    WINDOWPLACEMENT wp;

    if ((pwnd = ValidateHwnd(hwnd)) == NULL) {
        return (0L);
    }
    CheckLock(pwnd);

    if (hwndMDI == NULL) {
        goto CallDWP;
    }

    if ((pwndMDI = ValidateHwnd(hwndMDI)) == NULL) {
        return (0L);
    }
    CheckLock(pwndMDI);

    /*
     * Get a pointer to the MDI structure
     */
    pmdi = ((PMDIWND)pwndMDI)->pmdi;

    switch (wMsg) {

    /*
     * If there is a maximized child window, add it's window text...
     */
    case WM_SETTEXT: {
        LPWSTR lpwsz = NULL;

        if (fAnsi && lParam) {
            if (!MBToWCS((LPSTR)lParam, -1, &lpwsz, -1, TRUE))
                return 0;
            lParam = (LPARAM)lpwsz;
        }
        xxxSetFrameTitle(pwnd, pwndMDI, (LPWSTR)lParam);

        if (lpwsz)
            UserLocalFree(lpwsz);
        break;
    }
    case WM_NCACTIVATE:
        SendMessage(hwndMDI, WM_NCACTIVATE, wParam, lParam);
        goto CallDWP;

    case WM_COMMAND:
        if ((UINT)LOWORD(wParam) == (FIRST(pmdi) + MAXITEMS -1)) {

            /*
             * selected the More...  item
             */
            if (fAnsi) {
                wParam = DialogBoxParamA(hmodUser,
                                         MAKEINTRESOURCEA(IDD_MDI_ACTIVATE),
                                         hwnd,
                                         MDIActivateDlgProcA,
                                         (LPARAM)pwndMDI);
            } else {
                wParam = DialogBoxParamW(hmodUser,
                                         MAKEINTRESOURCEW(IDD_MDI_ACTIVATE),
                                         hwnd,
                                         MDIActivateDlgProcW,
                                         (LPARAM)pwndMDI);
            }
            if ((int)wParam >= 0) {
                wParam += FIRST(pmdi);
                goto ActivateTheChild;
            }
        } else if (((UINT)LOWORD(wParam) >= FIRST(pmdi)) &&
                ((UINT)LOWORD(wParam) < FIRST(pmdi) + CKIDS(pmdi))) {
ActivateTheChild:
            pwndT = FindPwndChild(pwndMDI, (UINT)LOWORD(wParam));
            ThreadLock(pwndT, &tlpwndT);

            SendMessage(hwndMDI, WM_MDIACTIVATE, (WPARAM)HW(pwndT), 0L);

            /*
             * if minimized, restore it.
             */
            if (pwndT != NULL && TestWF(pwndT, WFMINIMIZED))
                    //
                    // Fix for B#1510.  Don't restore directly.  Send child
                    // a restore message.
                    //
                SendMessage(HWq(pwndT), WM_SYSCOMMAND, (WPARAM)SC_RESTORE, 0L);
            ThreadUnlock(&tlpwndT);
            break;
        }

        switch (wParam & 0xFFF0) {

        /*
         * System menu commands on a maxed mdi child
         */
        case SC_SIZE:
        case SC_MOVE:
        case SC_RESTORE:
        case SC_CLOSE:
        case SC_NEXTWINDOW:
        case SC_PREVWINDOW:
        case SC_MINIMIZE:
        case SC_MAXIMIZE:
            hwndT = MAXED(pmdi);
            if (hwndT != NULL) {
                PWND pwndT = ValidateHwnd(hwndT);
                if (pwndT == NULL)
                    break;
                if ((wParam & 0xFFF0) == SC_CLOSE) {
                    /*
                     * Since the window is maxed, we've cleared WFSYSMENU (see
                     *  MDIAddSysMenu). We need to set it back here so GetSysMenuHandle
                     *  will do the right thing for _MNCanClose.
                     */
                    BOOL fCanClose;
                    UserAssert(!TestWF(pwndT, WFSYSMENU) && (pwndT->spmenuSys != NULL));
                    SetWindowState(pwndT, WFSYSMENU);
                    fCanClose = xxxMNCanClose(pwndT);
                    ClearWindowState(pwndT, WFSYSMENU);
                    if (!fCanClose) {
                        break;
                    }
                } else if (((wParam & 0xFFF0) == SC_MINIMIZE) && !TestWF(pwndT, WFMINBOX)) {
                    break;
                }

                return SendMessage(hwndT, WM_SYSCOMMAND, wParam, lParam);
            }
        }
        goto CallDWP;

    case WM_SIZE:
        if (wParam != SIZEICONIC) {
            NtUserMoveWindow(hwndMDI, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
        } else {
            wp.length = sizeof(WINDOWPLACEMENT);
            if (GetWindowPlacement(hwnd, &wp)) {
                RECT rcT;
                int  clB;

               /*
                * If frame is iconic, size mdi win to be size of restored
                * frame's client area.  Thus mdi children etc created in here
                * use the proper mdiclient size.
                */
               clB = GetWindowBorders(pwnd->style, pwnd->ExStyle, TRUE, TRUE);

               CopyInflateRect(&rcT, &wp.rcNormalPosition,
                        -clB*SYSMET(CXBORDER), -clB*SYSMET(CYBORDER));

               if (TestWF(pwnd, WFBORDERMASK) == LOBYTE(WFCAPTION))
                       rcT.top += SYSMET(CYCAPTION);
               rcT.top += SYSMET(CYMENU);

               NtUserMoveWindow(hwndMDI, 0, 0, rcT.right-rcT.left,
                       rcT.bottom-rcT.top, TRUE);
            }
        }
        goto CallDWP;

    case WM_SETFOCUS:
        NtUserSetFocus(hwndMDI);
        break;

    case WM_NEXTMENU:
        if (TestWF(pwnd, WFSYSMENU) && !TestWF(pwnd, WFMINIMIZED) &&
            ACTIVE(pmdi) && !MAXED(pmdi))
        {
            PMENU pmenuIn;
            /*
             * Go to child system menu by wrapping to the left from
             * the first popup in the menu bar or to the right from
             * the frame sysmenu.
             */
            pmnm = (PMDINEXTMENU)lParam;
            pmenuIn = RevalidateHmenu(pmnm->hmenuIn);

            if (pmenuIn && ((wParam == VK_LEFT && pmenuIn == REBASE(pwnd, spmenu)) ||
                    (wParam == VK_RIGHT && pmnm->hmenuIn ==
                    NtUserGetSystemMenu(hwnd, FALSE)))) {

                HMENU hmenu;
                PWND pwndActive;

                //
                // Make sure the window is still valid
                //
                if ((pwndActive = ValidateHwnd(ACTIVE(pmdi))) == NULL) {
                    return 0;
                }

                //
                // Make sure the child's system menu items are updated
                // (i.e. the ones are enabled/disabled)
                //
                if (!TestWF(pwndActive,WFMAXIMIZED)) {
                    NtUserSetSysMenu(ACTIVE(pmdi));
                }

                hmenu = NtUserGetSystemMenu(ACTIVE(pmdi), FALSE);
                pmnm->hmenuNext = hmenu;
                pmnm->hwndNext = ACTIVE(pmdi);

                return TRUE;
            }
        }

        /*
         * default behaviour
         */
        return 0L;

    case WM_MENUCHAR:
        if (!TestWF(pwnd, WFMINIMIZED) && LOWORD(wParam) == TEXT('-')) {
            if (MAXED(pmdi))
                return MAKELONG(0, 2);
            else if (ACTIVE(pmdi)) {
              PostMessage(ACTIVE(pmdi), WM_SYSCOMMAND,
                    SC_KEYMENU, MAKELONG(TEXT('-'), 0));
              return MAKELONG(0, 1);
          }
        }

        /*
         ** FALL THRU **
         */

    default:
CallDWP:
        return DefWindowProcWorker(pwnd, wMsg, wParam, lParam, fAnsi);
    }

    return 0L;
}


LRESULT WINAPI DefFrameProcW(
    HWND hwnd,
    HWND hwndMDIClient,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    return DefFrameProcWorker(hwnd, hwndMDIClient, message, wParam, lParam,
                              FALSE);
}

LRESULT WINAPI DefFrameProcA(
    HWND hwnd,
    HWND hwndMDIClient,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    return DefFrameProcWorker(hwnd, hwndMDIClient, message, wParam,
                              lParam, TRUE);
}


/***************************************************************************\
* ChildMinMaxInfo
*
* History:
* 11-14-90 MikeHar Ported from windows
\***************************************************************************/

void ChildMinMaxInfo(
    PWND pwnd,
    PMINMAXINFO pmmi)
{
    PWND pwndParent = REBASEPWND(pwnd, spwndParent);
    RECT rc;
#ifdef USE_MIRRORING
    int SaveLeft;
#endif
    UserAssert(GETFNID(pwnd) != FNID_DESKTOP);

    CopyRect(&rc, &pwndParent->rcClient);
    _ScreenToClient(pwndParent, (LPPOINT)&rc.left);
    _ScreenToClient(pwndParent, (LPPOINT)&rc.right);

#ifdef USE_MIRRORING
    /*
     * Swap the left and right if pwnd is a mirrored window.
     */
    if (TestWF(pwnd,WEFLAYOUTRTL)) {
       SaveLeft = rc.left;
       rc.left  = rc.right;
       rc.right = SaveLeft;
    }
#endif

    _AdjustWindowRectEx(&rc, pwnd->style, FALSE, pwnd->ExStyle);

    /*
     * Position...
     */
    pmmi->ptMaxPosition.x = rc.left;
    pmmi->ptMaxPosition.y = rc.top;
    pmmi->ptMaxSize.x = rc.right - rc.left;
    pmmi->ptMaxSize.y = rc.bottom - rc.top;
}


/***************************************************************************\
* xxxChildResize
*
* History:
* 11-14-90 MikeHar Ported from windows
\***************************************************************************/

void xxxChildResize(
    PWND pwnd,
    UINT wMode)
{
    PWND pwndT;
    PWND pwndMDI = REBASEPWND(pwnd, spwndParent);
    PWND pwndFrame = REBASEPWND(pwndMDI, spwndParent);
    HWND hwndOldActive;
    PMDI pmdi;
    HWND hwndActive;
    TL tlpwndMDI;
    TL tlpwndFrame;
    TL tlpwndT;
    PMENU pmenu;
    HWND hwnd = HWq(pwnd);

    CheckLock(pwnd);

    NtUserSetSysMenu(hwnd);

    ThreadLock(pwndMDI, &tlpwndMDI);
    ThreadLock(pwndFrame, &tlpwndFrame);

    /*
     * Get a pointer to the MDI structure
     */
    pmdi = ((PMDIWND)pwndMDI)->pmdi;
    pmenu = REBASE(pwndFrame, spmenu);

    if (MAXED(pmdi) == hwnd && wMode != SIZEFULLSCREEN) {
        /*
         * Restoring the current maximized window...
         * Remove the system menu from the Frame window.
         */
        if (!(SCROLL(pmdi) & OTHERMAXING)) {
            Unlock(&MAXED(pmdi));
            MDIRemoveSysMenu(PtoH(pmenu), hwnd);
            Unlock(&MAXED(pmdi));
            xxxSetFrameTitle(pwndFrame, pwndMDI, (LPWSTR)1L);
        }
    }

    if (wMode == SIZEFULLSCREEN) {

        /*
         * Already maximized?
         */
        if (hwnd == MAXED(pmdi))
            goto Exit;

        /*
         * Maximizing this window...
         */

        pmdi->wScroll |= OTHERMAXING | SCROLLCOUNT;

        if (hwndOldActive = MAXED(pmdi)) {
            SendMessage(hwndOldActive, WM_SETREDRAW, FALSE, 0L);
            MDIRemoveSysMenu(PtoH(pmenu), hwndOldActive);
            NtUserMinMaximize(hwndOldActive, SW_MDIRESTORE, FALSE);
            SendMessage(hwndOldActive, WM_SETREDRAW, TRUE, 0L);
        }

        Lock(&MAXED(pmdi), hwnd);

        /*
         * Add the system menu to the Frame window.
         */
        MDIAddSysMenu(PtoH(pmenu), hwnd);
        xxxSetFrameTitle(pwndFrame, pwndMDI, (LPWSTR)1L);

        pmdi->wScroll &= ~(OTHERMAXING | SCROLLCOUNT);
    }

    if (wMode == SIZEICONIC) {
        for (pwndT = REBASEPWND(pwndMDI, spwndChild); pwndT;
                pwndT = REBASEPWND(pwndT, spwndNext)) {
            if (!pwndT->spwndOwner && TestWF(pwndT, WFVISIBLE))
                break;
        }

        hwndActive = NtUserQueryWindow(hwnd, WindowActiveWindow);
        if ((pwndT != NULL) && (hwndActive != NULL) &&
                IsChild(hwndActive, HWq(pwndMDI))) {
            ThreadLockAlways(pwndT, &tlpwndT);
            SendMessage(HWq(pwndT), WM_CHILDACTIVATE, 0, 0L);
            ThreadUnlock(&tlpwndT);
        }
    }

    if (!(SCROLL(pmdi) & SCROLLCOUNT))
        RecalculateScrollRanges(pwndMDI, FALSE);

Exit:
    ThreadUnlock(&tlpwndFrame);
    ThreadUnlock(&tlpwndMDI);
}


/***************************************************************************\
* DefMDIChildProc
*
* History:
* 11-14-90 MikeHar Ported from windows
\***************************************************************************/

LRESULT DefMDIChildProcWorker(
    HWND hwnd,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam,
    BOOL fAnsi)
{
    PWND pwnd;
    PWND pwndParent;
    PMDI pmdi;
    PMDINEXTMENU pmnm;
    HWND hwndT;
    PWND pwndT;
    TL tlpwndT;
    TL tlpwndParent;
    LRESULT lRet;

    if ((pwnd = ValidateHwnd(hwnd)) == NULL) {
        return (0L);
    }

    CheckLock(pwnd);

    /*
     * Check to see if this is a real mdi child window
     */
    pwndParent = REBASEPWND(pwnd, spwndParent);
    if (!pwndParent || GETFNID(pwndParent) != FNID_MDICLIENT) {
        RIPERR0(ERROR_NON_MDICHILD_WINDOW, RIP_VERBOSE, "");
        return DefWindowProcWorker(pwnd, wMsg, wParam, lParam, fAnsi);
    }

    /*
     * Get a pointer to the MDI structure, if it still exists
     */
    pmdi = ((PMDIWND)pwndParent)->pmdi;
    if ((ULONG_PTR)pmdi == (ULONG_PTR)-1) {
        goto CallDWP;
    }

    switch (wMsg) {
    case WM_SETFOCUS:
        if (DIFFWOWHANDLE(hwnd, ACTIVE(pmdi))) {
            ThreadLockAlways(pwndParent, &tlpwndParent);
            xxxMDIActivate(pwndParent, pwnd);
            ThreadUnlock(&tlpwndParent);
        }
        goto CallDWP;

    case WM_NEXTMENU:

        /*
         * wrap to the frame menu bar, either left to the system menu,
         * or right to the frame menu bar.
         */
        pmnm = (PMDINEXTMENU)lParam;
        pwndT = REBASEPWND(pwndParent, spwndParent);
        pmnm->hwndNext = HW(pwndT);
        pmnm->hmenuNext = (wParam == VK_LEFT) ?
                NtUserGetSystemMenu(pmnm->hwndNext, FALSE) :
                GetMenu(pmnm->hwndNext);
        return TRUE;
#if 0
             hWnd->hwndParent->hwndParent
        return (LONG)(((wParam == VK_LEFT) ?
                NtUserGetSystemMenu(HW(pwndT), FALSE):
                pwndT->spmenu)
          );
// return MAKELONG(NtUserGetSystemMenu(ACTIVE(pwndMDI), FALSE),
// ACTIVE(pwndMDI));
#endif
    case WM_CLOSE:
        hwndT = GetParent(hwnd);
        if (hwndT != NULL) {
            SendMessage(hwndT, WM_MDIDESTROY, (WPARAM)hwnd, 0L);
        }
        break;

    case WM_MENUCHAR:
        PostMessage(GetParent(GetParent(hwnd)), WM_SYSCOMMAND,
                (DWORD)SC_KEYMENU, (LONG)LOWORD(wParam));
        return 0x10000;

    case WM_SETTEXT:
        DefWindowProcWorker(pwnd, wMsg, wParam, lParam, fAnsi);
        if (WINDOW(pmdi))
            ModifyMenuItem(pwnd);

        if (TestWF(pwnd, WFMAXIMIZED)) {

            /*
             * Add the child's window text to the frame since it is
             * maximized.  But just redraw the caption so pass a 3L.
             */
            pwndT = REBASEPWND(pwndParent, spwndParent);
            ThreadLock(pwndT, &tlpwndT);
            ThreadLock(pwndParent, &tlpwndParent);
            xxxSetFrameTitle(pwndT, pwndParent, (LPWSTR)3L);
            ThreadUnlock(&tlpwndParent);
            ThreadUnlock(&tlpwndT);
        }
        break;

    case WM_GETMINMAXINFO:
        ChildMinMaxInfo(pwnd, (PMINMAXINFO)lParam);
        break;

    case WM_SIZE:
        xxxChildResize(pwnd, (UINT)wParam);
        goto CallDWP;

    case WM_MOVE:
        if (!TestWF(pwnd, WFMAXIMIZED))
            RecalculateScrollRanges(pwndParent, FALSE);
        goto CallDWP;

    case WM_CHILDACTIVATE:
        ThreadLock(pwndParent, &tlpwndParent);
        xxxMDIActivate(pwndParent, pwnd);
        ThreadUnlock(&tlpwndParent);
        break;

    case WM_SYSCOMMAND:
        switch (wParam & 0xFFF0) {
        case SC_NEXTWINDOW:
        case SC_PREVWINDOW:
            hwndT = GetParent(hwnd);
            SendMessage(hwndT, WM_MDINEXT, (WPARAM)hwnd,
                    (DWORD)((wParam & 0xFFF0) == SC_PREVWINDOW));
            break;

        case SC_SIZE:
        case SC_MOVE:
            if (SAMEWOWHANDLE(hwnd, MAXED(pmdi))) {

                /*
                 * If a maxed child gets a size or move message, blow it
                 * off.
                 */
                break;
            } else
                goto CallDWP;

        case SC_MAXIMIZE:
            if (SAMEWOWHANDLE(hwnd, MAXED(pmdi))) {

                /*
                 * If a maxed child gets a maximize message, forward it
                 * to the frame.  Useful if the maximized child has a
                 * size box so that clicking on it then maximizes the
                 * parent.
                 */
                pwndT = REBASEPWND(pwndParent, spwndParent);
                ThreadLock(pwndT, &tlpwndT);
                lRet = SendMessage(HW(pwndT),
                        WM_SYSCOMMAND, SC_MAXIMIZE, lParam);
                ThreadUnlock(&tlpwndT);
                return lRet;
            }

            /*
             * else fall through
             */

        default:
            goto CallDWP;
        }
        break;

    default:
CallDWP:
        return DefWindowProcWorker(pwnd, wMsg, wParam, lParam, fAnsi);
    }

    return 0L;
}


/***************************************************************************\
* DefMDIChildProc
*
* Translates the message, calls DefMDIChildProc on server side.
*
* 04-11-91 ScottLu Created.
\***************************************************************************/

LRESULT WINAPI DefMDIChildProcW(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    return DefMDIChildProcWorker(hwnd, message, wParam, lParam, FALSE);
}

LRESULT WINAPI DefMDIChildProcA(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    return DefMDIChildProcWorker(hwnd, message, wParam, lParam, TRUE);
}

BOOL MDICompleteChildCreation(HWND hwndChild, HMENU hSysMenu, BOOL fVisible, BOOL fDisabled) {
    PWND pwndChild;
    PWND  pwndClient;
    HWND  hwndClient;
    BOOL  fHasOwnSysMenu;
    PMDI pmdi;

    pwndChild = ValidateHwnd(hwndChild);
    pwndClient = REBASEPWND(pwndChild,spwndParent);
    hwndClient = HWq(pwndClient);

    fHasOwnSysMenu = (pwndChild->spmenuSys) ? TRUE : FALSE;

    pmdi = ((PMDIWND)(pwndClient))->pmdi;

    CKIDS(pmdi)++;
    ITILELEVEL(pmdi)++;
    if (ITILELEVEL(pmdi) > 0x7ffe)
        ITILELEVEL(pmdi) = 0;

    // Update "Window" menu if this new window should be on it
    if (fVisible && !fDisabled && (CKIDS(pmdi) <= MAXITEMS))
        SendMessage(hwndClient, WM_MDIREFRESHMENU, 0, 0L);

    //
    // Add the MDI System Menu.  Catch the case of not being able to add a
    // system menu (EG, guy doesn't have WS_SYSMENU style), and delete the
    // menu to avoid buildup in USER's heap.
    //
    if (hSysMenu && (fHasOwnSysMenu || !NtUserSetSystemMenu(hwndChild, hSysMenu)))
        NtUserDestroyMenu(hSysMenu);

    if (fVisible)
    {
        if (!TestWF(pwndChild, WFMINIMIZED) || !ACTIVE(pmdi))
        {
            NtUserSetWindowPos(hwndChild, HWND_TOP, 0, 0, 0, 0,
                    SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);

            if (TestWF(pwndChild, WFMAXIMIZED) && !fHasOwnSysMenu)
            {
                PWND pwndParent = REBASEPWND(pwndClient, spwndParent);
                PMENU pmenu = REBASE(pwndParent, spmenu);
                MDIAddSysMenu(PtoH(pmenu), hwndChild);
                NtUserRedrawFrame(HW(pwndParent));
            }
        }
        else
        {
            NtUserShowWindow(hwndChild, SW_SHOWMINNOACTIVE);
        }
    }


    return TRUE;
}


BOOL
CreateMDIChild(
        PSHORTCREATE        psc,
        LPMDICREATESTRUCT   pmcs,
        DWORD               dwExpWinVerAndFlags,
        HMENU *             phSysMenu,
        PWND                pwndParent)
{
    BOOL fVisible;
    RECT rcT;
    HMENU hSysMenu = NULL;
    HWND hwndPrevMaxed;
    PMDI pmdi;

    /*
     * Get a pointer to the MDI structure
     */
    pmdi = ((PMDIWND)(pwndParent))->pmdi;

    pmcs->style = psc->style;

    // Mask off ignored style bits and add required ones.
    psc->style |= (WS_CHILD | WS_CLIPSIBLINGS);
    if (!(pwndParent->style & MDIS_ALLCHILDSTYLES))
    {
        psc->style &= WS_MDIALLOWED;
        psc->style |= (WS_MDISTYLE | WS_VISIBLE);
    }
    else if (psc->style & WS_POPUP)
    {
        RIPMSG0(RIP_ERROR, "CreateWindowEx: WS_POPUP not allowed on MDI children");
        if (LOWORD(dwExpWinVerAndFlags) >= VER40)
            return FALSE;
    }

    fVisible = ((psc->style & WS_VISIBLE) != 0L);

    //
    // Save ORIGINAL parameters in MDICREATESTRUCT.  This is for
    // compatibility with old WM_MDICREATE.
    //
    pmcs->x   = rcT.left   = psc->x;
    pmcs->y   = rcT.top    = psc->y;
    pmcs->cx  = rcT.right  = psc->cx;
    pmcs->cy  = rcT.bottom = psc->cy;

    MDICheckCascadeRect(pwndParent, &rcT);

    //
    // Setup creation coords
    //
    psc->x       = rcT.left;
    psc->y       = rcT.top;
    psc->cx      = rcT.right;
    psc->cy      = rcT.bottom;

    // Load the system menu
    if (psc->style & WS_SYSMENU) {
        hSysMenu = xxxLoadSysMenu(CHILDSYSMENU);
        if (hSysMenu == NULL) {
            return FALSE;
        }
    }


    // The window got created ok: now restore the current maximized window
    // so we can maximize ourself in its place.
    hwndPrevMaxed = MAXED(pmdi);
    if (fVisible && IsWindow(hwndPrevMaxed))
    {
        if (psc->style & WS_MAXIMIZE)
            SendMessage(hwndPrevMaxed, WM_SETREDRAW, (WPARAM)FALSE, 0L);

        // we could nuke the hwndPrevMaxed during the SendMessage32
        // so recheck just in case, B#11122, [t-arthb]

        if ( IsWindow(hwndPrevMaxed) )
        {
            NtUserMinMaximize(hwndPrevMaxed, SW_SHOWNORMAL, TRUE);

            if ( psc->style & WS_MAXIMIZE )
               SendMessage(hwndPrevMaxed, WM_SETREDRAW, (WPARAM)TRUE, 0L);
        }

    }

    // Set the proper Child Window ID for this MDI child.
    psc->hMenu = (HMENU)UIntToPtr( (FIRST(pmdi) + CKIDS(pmdi)) );

    *phSysMenu = hSysMenu;

    return TRUE;
}
