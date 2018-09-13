#include "ctlspriv.h"
#include "flat_sb.h"

//  Following interfaces are imported from newwsbctl.c and newsb.c. These
//  functions are for internal use only.

void FlatSB_Internal_CalcSBStuff(WSBState *, BOOL);
void FlatSB_Internal_DoScroll(WSBState *, int, int, BOOL);
void FlatSB_Internal_EndScroll(WSBState *, BOOL);
void FlatSB_Internal_DrawArrow(WSBState *, HDC, CONST RECT *, int, int);
void FlatSB_Internal_DrawElevator(WSBState *, HDC, LPRECT, BOOL);
void FlatSB_Internal_DrawGroove(WSBState *, HDC, LPRECT, BOOL);
void FlatSB_Internal_DrawSize(WSBState *, HDC, int, int);
void FlatSB_Internal_DrawScrollBar(WSBState *, HDC, BOOL, BOOL);
void FlatSB_Internal_DrawThumb(WSBState *, BOOL);
void FlatSB_Internal_DrawThumb2(WSBState *, HDC, BOOL, UINT);
UINT FlatSB_Internal_GetSBFlags(WSBState *, BOOL);
BOOL FlatSB_Internal_EnableScrollBar(WSBState *, int, UINT);
WSBState * FlatSB_Internal_InitPwSB(HWND);
void FlatSB_Internal_RedrawScrollBar(WSBState *, BOOL);
void FlatSB_Internal_SBTrackInit(WSBState *, HWND, LPARAM, int, BOOL);
void FlatSB_Internal_TrackBox(WSBState *, int message, WPARAM, LPARAM);
void FlatSB_Internal_TrackThumb(WSBState *, int message, WPARAM, LPARAM);
BOOL FlatSB_Internal_IsSizeBox(HWND);

LRESULT FlatSB_Internal_SetScrollBar(WSBState *, int, LPSCROLLINFO, BOOL);
LRESULT CALLBACK FlatSB_SubclassWndProc(HWND, UINT, WPARAM, LPARAM, WPARAM, ULONG_PTR);

void FlatSB_Internal_NotifyWinEvent(WSBState *pWState, UINT event, LONG_PTR idChild)
{
    MyNotifyWinEvent(event, pWState->sbHwnd,
                     pWState->fTrackVert ? OBJID_VSCROLL : OBJID_HSCROLL,
                     idChild);
}

#define IsHottrackable(STYLE)   ((STYLE == FSB_FLAT_MODE) || (STYLE == FSB_ENCARTA_MODE))

HRESULT WINAPI UninitializeFlatSB(HWND hwnd)
{
    SCROLLINFO hsi, vsi;
    WSBState * pWState;
    int style, vFlags, hFlags;
    BOOL hValid = FALSE, vValid = FALSE;

    GetWindowSubclass(hwnd, FlatSB_SubclassWndProc, 0, (ULONG_PTR *)&pWState);
    if (pWState == (WSBState *)NULL)
        return S_FALSE;
    else if (pWState == WSB_UNINIT_HANDLE)   {
        RemoveWindowSubclass(hwnd, FlatSB_SubclassWndProc, 0);
        return S_FALSE;
    }

    if (pWState->fTracking)
        return E_FAIL;          //  Can't do this!

    style = pWState->style;
    vsi.cbSize = hsi.cbSize = sizeof(SCROLLINFO);
    vsi.fMask = hsi.fMask = SIF_ALL | SIF_DISABLENOSCROLL;

    hValid = FlatSB_GetScrollInfo(hwnd, SB_HORZ, &hsi);
    hFlags = FlatSB_Internal_GetSBFlags(pWState, SB_HORZ);
    vValid = FlatSB_GetScrollInfo(hwnd, SB_VERT, &vsi);
    vFlags = FlatSB_Internal_GetSBFlags(pWState, SB_VERT);

    DeleteObject(pWState->hbm_Bkg);
    DeleteObject(pWState->hbr_Bkg);
    LocalFree((HLOCAL)pWState);
    RemoveWindowSubclass(hwnd, FlatSB_SubclassWndProc, 0);

    if (vValid) {
        SetScrollInfo(hwnd, SB_VERT, &vsi, FALSE);
        EnableScrollBar(hwnd, SB_VERT, vFlags);
    }

    if (hValid) {
        SetScrollInfo(hwnd, SB_HORZ, &hsi, FALSE);
        EnableScrollBar(hwnd, SB_HORZ, hFlags);
    }

    SetWindowBits(hwnd, GWL_STYLE, WS_HSCROLL | WS_VSCROLL, style & (WS_HSCROLL | WS_VSCROLL));

    //  Force the WM_NCCALCSIZE/WM_NCPAINT to be sent.
    CCInvalidateFrame(hwnd);
    return S_OK;
}

//
//  For accessibility - We keep the original USER scrollbars around and
//  keep USER's view of the scrollbar in sync with the flat view.  This
//  means keeping the WS_[HV]SCROLL styles on the window, forwarding
//  all scrollbar APIs into USER, etc.  That way, when OLEACC asks USER
//  for the scrollbar state, USER returns values that match the flat_sb
//  values.
//
//  Even though the styles are enabled, the UI isn't affected since we
//  take over all nonclient painting and hit-testing so USER never gets
//  a chance to paint or hit-test the scrollbars that we took over.
//
BOOL WINAPI InitializeFlatSB(HWND hwnd)
{
    int newStyle, style;
    SCROLLINFO hsi, vsi, siTmp;
    WSBState * pWState;
    BOOL hValid = FALSE, vValid = FALSE;

    style = GetWindowLong(hwnd, GWL_STYLE);
    siTmp.cbSize = vsi.cbSize = hsi.cbSize = sizeof(SCROLLINFO);
    vsi.fMask = hsi.fMask = SIF_ALL | SIF_DISABLENOSCROLL;

    if (style & WS_HSCROLL)
        hValid = GetScrollInfo(hwnd, SB_HORZ, &hsi);

    if (style & WS_VSCROLL)
        vValid = GetScrollInfo(hwnd, SB_VERT, &vsi);

    newStyle = style & (WS_VSCROLL | WS_HSCROLL);
    style &= ~(WS_VSCROLL | WS_HSCROLL);

    GetWindowSubclass(hwnd, FlatSB_SubclassWndProc, 0, (ULONG_PTR *)&pWState);
    if (!vValid && !hValid)   {
        if (NULL == pWState)    {
            if (!SetWindowSubclass(hwnd, FlatSB_SubclassWndProc, 0, (ULONG_PTR)WSB_UNINIT_HANDLE))
                return FALSE;
        } else  {
            //  It seems to me unreasonable to do nothing while the caller wants
            //  to init again the flat SB we are already using.
        }
        return TRUE;
    }

    if ((NULL == pWState) || (WSB_UNINIT_HANDLE == pWState))    {
        pWState = FlatSB_Internal_InitPwSB(hwnd);
        if ((WSBState *)NULL == pWState)
            return FALSE;

        if (!SetWindowSubclass(hwnd,FlatSB_SubclassWndProc, 0,(ULONG_PTR)pWState)) {
            DeleteObject(pWState->hbm_Bkg);
            DeleteObject(pWState->hbr_Bkg);
            LocalFree((HLOCAL)pWState);
            return FALSE;
        }
    }

    pWState->style = newStyle;
    if (hValid)
        FlatSB_Internal_SetScrollBar(pWState, SB_HORZ, &hsi, FALSE);

    if (vValid)
        FlatSB_Internal_SetScrollBar(pWState, SB_VERT, &vsi, FALSE);

    //  Force the WM_NCCALCSIZE/WM_NCPAINT to be sent.
    CCInvalidateFrame(hwnd);

    return TRUE;
}


LRESULT FlatSB_NCDestroyProc(WSBState * pWState, HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    ASSERT(pWState);

    if (pWState != WSB_UNINIT_HANDLE)   {
        DeleteObject(pWState->hbm_Bkg);
        DeleteObject(pWState->hbr_Bkg);
        LocalFree((HLOCAL)pWState);
    }

    RemoveWindowSubclass(hwnd, FlatSB_SubclassWndProc, 0);
    return DefSubclassProc(hwnd, WM_NCDESTROY, wParam, lParam);
}

LRESULT FlatSB_NCCalcProc(WSBState * pWState, HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    RECT * rc = (RECT *) lParam;
    NCCALCSIZE_PARAMS * pnc = (NCCALCSIZE_PARAMS *)lParam;
    RECT    rcClient, rcWin;
    LRESULT lres;
    DWORD dwStyle;

    //  ZDC:
    //
    //  Note:
    //      It's said that if wParam is true, new rgrc[1|2] are also
    //      computed. Since I didn't see the implementation in the 'user'
    //      code, I leave it unimplemented.


    if ((BOOL)wParam == TRUE)
        CopyRect(&rcWin, &(pnc->rgrc[0]));
    else
        CopyRect(&rcWin, rc);

    dwStyle = SetWindowBits(hwnd, GWL_STYLE, WS_VSCROLL | WS_HSCROLL, 0);

    // Save pnc->rgrc[0] to keep USER happy (see below)
    CopyRect(&rcClient, &pnc->rgrc[0]);

    lres = DefSubclassProc(hwnd, WM_NCCALCSIZE, wParam, lParam);

    SetWindowBits(hwnd, GWL_STYLE, WS_VSCROLL | WS_HSCROLL, dwStyle);

    // USER does funky internal state munging during the WM_NCCALCSIZE
    // and we want USER's internal state to see the scrollbars even though
    // we're drawing them.  So give USER one last look at the original
    // values so he will think the scroll bars are really there.  This
    // sets internal WFVPRESENT and WFHPRESENT flags that OLEACC secretly
    // looks at via the undocumented GetScrollBarInfo().
    DefSubclassProc(hwnd, WM_NCCALCSIZE, FALSE, (LPARAM)&rcClient);

    if ((BOOL)wParam == TRUE)
        CopyRect(&rcClient, &(pnc->rgrc[0]));
    else
        CopyRect(&rcClient, rc);

    pWState->style &= ~(WFVPRESENT | WFHPRESENT);
    if (TestSTYLE(pWState->style, WS_VSCROLL)
        && (rcClient.right - rcClient.left >= pWState->x_VSBArrow)) {
        pWState->style |= WFVPRESENT;
        rcClient.right -= pWState->x_VSBArrow;
    }

    if (TestSTYLE(pWState->style, WS_HSCROLL)
        && (rcClient.bottom - rcClient.top > pWState->y_HSBArrow)) {
        pWState->style |= WFHPRESENT;
        rcClient.bottom -= pWState->y_HSBArrow;
    }

    if ((BOOL)wParam == TRUE)
        CopyRect(&(pnc->rgrc[0]), &rcClient);
    else
        CopyRect(rc, &rcClient);

    pWState->rcClient.top = rcClient.top - rcWin.top;
    pWState->rcClient.bottom = rcClient.bottom - rcWin.top;
    pWState->rcClient.left = rcClient.left - rcWin.left;
    pWState->rcClient.right = rcClient.right - rcWin.left;

    return lres;
}


LRESULT FlatSB_NCPaintProc(WSBState * pWState, HWND hwnd, WPARAM wParam,  LPARAM lParam)
{
    HDC     hdc;
    int     oldLoc, newLoc;
    LRESULT lres;
    DWORD dwStyle;
    RECT rcClient;

    ASSERT(pWState);
    ASSERT(pWState != WSB_UNINIT_HANDLE);

    //
    //  DefWindowProc(WM_NCPAINT) is going to try to draw USER's scrollbars,
    //  and will draw them in the wrong place if our scrollbar width is
    //  different from the system default width.  (Argh.)
    //
    //  So remove the scrollbar styles, do the paint, then put them back.
    //
    dwStyle = SetWindowBits(hwnd, GWL_STYLE, WS_VSCROLL | WS_HSCROLL, 0);

    GetWindowRect(hwnd, &rcClient);
    DefSubclassProc(hwnd, WM_NCCALCSIZE, FALSE, (LPARAM)&rcClient);

    lres = DefSubclassProc(hwnd, WM_NCPAINT, wParam, lParam);

    SetWindowBits(hwnd, GWL_STYLE, WS_VSCROLL | WS_HSCROLL, dwStyle);

    GetWindowRect(hwnd, &rcClient);
    DefSubclassProc(hwnd, WM_NCCALCSIZE, FALSE, (LPARAM)&rcClient);

//  hdc = GetDCEx(hwnd, (HRGN) wParam, DCX_WINDOW |
//                DCX_INTERSECTRGN | DCX_LOCKWINDOWUPDATE);

    //  ZDC:
    //
    //  Note:
    //      For some reason(wParam == 1) the statements above did not give
    //      the result we expected. I am not sure if it's the only case that
    //      GetDCEx will disappoint us.

    hdc = GetWindowDC(hwnd);
    newLoc = WSB_MOUSELOC_OUTSIDE;
    oldLoc = pWState->locMouse;

    if (TestSTYLE(pWState->style, WFHPRESENT)
        && TestSTYLE(pWState->style, WFVPRESENT))   {
        int cxFrame, cyFrame;

        cxFrame = pWState->rcClient.right;
        cyFrame = pWState->rcClient.bottom;
        FlatSB_Internal_DrawSize(pWState, hdc, cxFrame, cyFrame);
    }

    if (TestSTYLE(pWState->style, WFHPRESENT))  {
        FlatSB_Internal_DrawScrollBar(pWState, hdc, FALSE, FALSE);
        if (pWState->fHActive)
            newLoc = pWState->locMouse;
    }

    if (TestSTYLE(pWState->style, WFVPRESENT))  {
        pWState->locMouse = oldLoc;
        FlatSB_Internal_DrawScrollBar(pWState, hdc, TRUE, FALSE);
        if (pWState->fVActive)
            newLoc = pWState->locMouse;
    }
    pWState->locMouse = newLoc;

    ReleaseDC(hwnd, hdc);

    return lres;
}

LRESULT FlatSB_NCHitTestProc(WSBState *pWState, HWND hwnd, WPARAM wParam, LPARAM lParam, BOOL fTrack);

VOID CALLBACK TimerMouseLeave(
    HWND hwnd,  // handle of window for timer messages
    UINT uMsg,  // WM_TIMER message
    UINT_PTR idEvent,  // timer identifier
    DWORD dwTime   // current system time
)
{
    WSBState * pWState;

    if (idEvent != IDWSB_TRACK)
        return;

    GetWindowSubclass(hwnd, FlatSB_SubclassWndProc, 0, (ULONG_PTR *)&pWState);
    if ((pWState == (WSBState *)NULL) || (pWState == WSB_UNINIT_HANDLE))    {
        KillTimer(hwnd, IDWSB_TRACK);
        return;
    }

    if (pWState->fTracking) {
        return;
    }

    FlatSB_NCHitTestProc(pWState, hwnd, 0, 0, TRUE);
    return;
}

LRESULT FlatSB_NCHitTestProc(WSBState *pWState, HWND hwnd, WPARAM wParam, LPARAM lParam, BOOL fTTrack)
{
    LRESULT lres, lHTCode=HTBOTTOMRIGHT;
    RECT    rcTest, rcWindow;
    POINT   pt;
    BOOL    fVChanged = FALSE, fHChanged = FALSE;
    BOOL    fWinActive = ChildOfActiveWindow(hwnd);
    int     newLoc, oldLoc;

    ASSERT(pWState);
    ASSERT(pWState != WSB_UNINIT_HANDLE);

    GetWindowRect(hwnd, &rcWindow);
    if (fTTrack) {
        lres = HTNOWHERE;
        if (fWinActive)
            GetCursorPos(&pt);
        else    {
            pt.x = rcWindow.left - 1;      //  NOWHERE --- to fool CalcSBtuff2
            pt.y = rcWindow.top - 1;
        }
    } else    {
        lres = DefSubclassProc(hwnd, WM_NCHITTEST, wParam, lParam);
        pt.x = GET_X_LPARAM(lParam);
        pt.y = GET_Y_LPARAM(lParam);
    }

    //
    // If this is a RTL mirrored window, then measure
    // the client coordinates from the visual right edge.
    // [samera]
    //
    if (IS_WINDOW_RTL_MIRRORED(hwnd)) {
        pt.x = rcWindow.right - pt.x;
        lHTCode = HTBOTTOMLEFT;
    } else {
        pt.x -= rcWindow.left;
    }
    pt.y -= rcWindow.top;

    if (fTTrack && fWinActive && (pt.x == pWState->ptMouse.x) && (pt.y == pWState->ptMouse.y))
        return lres /* Meaningless result*/;

    //  We shouldn't get HTVSCROLL / HTHSCROLL for system scrollbar here.
    if (lres != HTNOWHERE)  {
        goto Redraw;
    }

    if (TestSTYLE(pWState->style, WFVPRESENT))  {
        rcTest.left = pWState->rcClient.right;
        rcTest.right = pWState->rcClient.right + pWState->x_VSBArrow;
        rcTest.top = pWState->rcClient.top;
        rcTest.bottom = pWState->rcClient.bottom;

        if (PtInRect(&rcTest, pt))  {
            lres = HTVSCROLL;
            goto Redraw;
        }
    }

    if (TestSTYLE(pWState->style, WFHPRESENT))  {
        rcTest.left = pWState->rcClient.left;
        rcTest.right = pWState->rcClient.right;
        rcTest.top = pWState->rcClient.bottom;
        rcTest.bottom = pWState->rcClient.bottom + pWState->y_HSBArrow;

        if (PtInRect(&rcTest, pt))  {
            lres = HTHSCROLL;
            goto Redraw;
        }
    }

    if (TestSTYLE(pWState->style, WFHPRESENT) && TestSTYLE(pWState->style, WFVPRESENT))
    {
        rcTest.left = pWState->rcClient.right;
        rcTest.right = pWState->rcClient.right + pWState->x_VSBArrow;
        rcTest.top = pWState->rcClient.bottom;
        rcTest.bottom = pWState->rcClient.bottom + pWState->y_HSBArrow;

        if (PtInRect(&rcTest, pt))  {
            if (!FlatSB_Internal_IsSizeBox(hwnd))
                lres = HTSIZE;
            else
                lres = lHTCode;
            goto Redraw;
        }
    }

    lres = HTNOWHERE;

Redraw:
    if(pWState->fTracking)
        return lres;

    if (!fWinActive) {
        fVChanged = pWState->fVActive; pWState->fVActive = FALSE;
        fHChanged = pWState->fHActive; pWState->fHActive = FALSE;
    } else  {
        switch (lres)   {
        case HTVSCROLL:
            fVChanged = TRUE; pWState->fVActive = TRUE;
            fHChanged = pWState->fHActive; pWState->fHActive = FALSE;
            break;
        case HTHSCROLL:
            fVChanged = pWState->fVActive; pWState->fVActive = FALSE;
            fHChanged = TRUE; pWState->fHActive = TRUE;
            break;
        default:
            fVChanged = pWState->fVActive; pWState->fVActive = FALSE;
            fHChanged = pWState->fHActive; pWState->fHActive = FALSE;
            break;
        }
    }

    pWState->ptMouse.x = pt.x;
    pWState->ptMouse.y = pt.y;

    newLoc = WSB_MOUSELOC_OUTSIDE;
    oldLoc = pWState->locMouse;
    if (fVChanged && IsHottrackable(pWState->vStyle))  {

        FlatSB_Internal_RedrawScrollBar(pWState, TRUE);
        if (pWState->fVActive)
            newLoc = pWState->locMouse;
    }

    if (fHChanged && IsHottrackable(pWState->hStyle))  {
        pWState->locMouse = oldLoc;
        FlatSB_Internal_RedrawScrollBar(pWState, FALSE);
        if (pWState->fHActive)
            newLoc = pWState->locMouse;
    }
    pWState->locMouse = newLoc;

    if (pWState->fVActive || pWState->fHActive) {
        if (pWState->hTrackSB == 0)
            pWState->hTrackSB = SetTimer(hwnd, IDWSB_TRACK,
                        GetDoubleClickTime()/2,
                        TimerMouseLeave);
    } else  {
        if (pWState->hTrackSB)  {
            KillTimer(hwnd, IDWSB_TRACK);
            pWState->hTrackSB = 0;
        }
    }

    return lres;
}

LRESULT FlatSB_SysCommandProc(WSBState * pWState, HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    LRESULT lres;
    unsigned uCmdType;
    int     hitArea;

    ASSERT(pWState);
    ASSERT(pWState != WSB_UNINIT_HANDLE);

    uCmdType = (unsigned) wParam & 0xFFF0;        // type of system command requested
    hitArea = (int) wParam & 0x000F;
    if (uCmdType != SC_HSCROLL && uCmdType != SC_VSCROLL)
        return DefSubclassProc(hwnd, WM_SYSCOMMAND, wParam, lParam);
    else
        //  There are some initialization we may need.
#define SC_INVALID 0
        lres = DefSubclassProc(hwnd, WM_SYSCOMMAND, (WPARAM)SC_INVALID, lParam);
#undef  SC_INVALID

    FlatSB_Internal_SBTrackInit(pWState, hwnd, lParam, hitArea, GetKeyState(VK_SHIFT) < 0);
    return 0;
}

LRESULT FlatSB_CancelModeProc(WSBState * pWState, HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    LRESULT lres;

    ASSERT(pWState);
    ASSERT(pWState != WSB_UNINIT_HANDLE);

    lres = DefSubclassProc(hwnd, WM_CANCELMODE, wParam, lParam);

    //  A good citizen of Subclass, we have to wait the DefSubclassProc
    //  release capture first!

    if (pWState->pfnSB)
        FlatSB_Internal_EndScroll(pWState, TRUE);

    return lres;
}

//
//  This updates the system metrics and points pPWState->pwmet at the
//  application metrics or system metrics, depending on whether a
//  screenreader is running.
//
void FlatSB_InitWSBMetrics(WSBState *pWState)
{
    BOOL fScreenRead;

    pWState->metSys.cxHSBThumb = GetSystemMetrics(SM_CXHTHUMB);
    pWState->metSys.cyVSBThumb = GetSystemMetrics(SM_CYVTHUMB);
    pWState->metSys.cxVSBArrow = GetSystemMetrics(SM_CXVSCROLL);
    pWState->metSys.cyVSBArrow = GetSystemMetrics(SM_CYVSCROLL);
    pWState->metSys.cxHSBArrow = GetSystemMetrics(SM_CXHSCROLL);
    pWState->metSys.cyHSBArrow = GetSystemMetrics(SM_CYHSCROLL);

    fScreenRead = FALSE;
    SystemParametersInfo(SPI_GETSCREENREADER, 0, &fScreenRead, 0);

    // If a screen reader is running, then the active metrics are the
    // system metrics; otherwise, it's the app metrics.
    pWState->pmet = fScreenRead ? &pWState->metSys : &pWState->metApp;

}

LRESULT FlatSB_OnSettingChangeProc(WSBState *pWState, HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    ASSERT(pWState);
    ASSERT(pWState != WSB_UNINIT_HANDLE);

    FlatSB_InitWSBMetrics(pWState);

    // These new metrics will most likely have altered our frame, so
    // recompute our frame stuff too
    CCInvalidateFrame(hwnd);

    return DefSubclassProc(hwnd, WM_SETTINGCHANGE, wParam, lParam);
}

LRESULT FlatSB_OnScrollProc(WSBState *pWState, HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (GET_WM_HSCROLL_HWND(wParam, lParam) == NULL && !pWState->fInDoScroll) {
        // Somebody on the outside (probably USER) changed our scroll stuff,
        // so re-sync with the USER values.
        if (GET_WM_HSCROLL_CODE(wParam, lParam) == SB_ENDSCROLL)
            FlatSB_NCPaintProc(pWState, hwnd, (WPARAM)1, 0);
    }
    return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK FlatSB_SubclassWndProc
(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    WPARAM uIdSubclass,
    ULONG_PTR dwRefData
)
{
    WSBState * pWState = (WSBState *)dwRefData;

    ASSERT (dwRefData);

    if (pWState == (WSBState *)NULL)
        return DefSubclassProc(hwnd, uMsg, wParam, lParam);
    else if (pWState == WSB_UNINIT_HANDLE && uMsg != WM_NCDESTROY)
        return DefSubclassProc(hwnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
    case WM_NCDESTROY:
        return FlatSB_NCDestroyProc(pWState, hwnd, wParam, lParam);
    case WM_NCCALCSIZE:
        return FlatSB_NCCalcProc(pWState, hwnd, wParam, lParam);
    case WM_NCPAINT:
        return FlatSB_NCPaintProc(pWState, hwnd, wParam, lParam);
    case WM_NCHITTEST:
        return FlatSB_NCHitTestProc(pWState, hwnd, wParam, lParam, FALSE);
    case WM_SYSCOMMAND:
        return FlatSB_SysCommandProc(pWState, hwnd, wParam, lParam);
    case WM_CANCELMODE:
        return FlatSB_CancelModeProc(pWState, hwnd, wParam, lParam);
    case WM_SETTINGCHANGE:
        return FlatSB_OnSettingChangeProc(pWState, hwnd, wParam, lParam);

    case WM_VSCROLL:
    case WM_HSCROLL:
        return FlatSB_OnScrollProc(pWState, hwnd, uMsg, wParam, lParam);
    }
    return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}


//=------------------------------------------------------------------
//  Start of drawing functions.
//=-------------------------------------------------------------------


#define WSB_BUTTON_UPARROW      DFCS_SCROLLUP
#define WSB_BUTTON_DOWNARROW    DFCS_SCROLLDOWN
#define WSB_BUTTON_LEFTARROW    DFCS_SCROLLLEFT
#define WSB_BUTTON_RIGHTARROW   DFCS_SCROLLRIGHT

#define WSB_RESTING_MODE        0
#define WSB_HOTTRACKED_MODE     1
#define WSB_MOUSEDOWN_MODE      2
#define WSB_DISABLED_MODE       3

void FlatSB_Internal_DrawBox(HDC hdc, CONST RECT * prct, int mode)
{
    HBRUSH hbrOld, hbrEdge, hbrFace;
    int w, h, l, t;

    if (prct->left > prct->right)
        return;
    else if (prct->top > prct->bottom)
        return;

    l = prct->left;
    t = prct->top;
    w = prct->right - prct->left;
    h = prct->bottom - prct->top;

    switch (mode)   {
    case WSB_HOTTRACKED_MODE:
        hbrEdge = GetSysColorBrush(COLOR_3DSHADOW);
        hbrFace = hbrEdge;
        break;
    case WSB_MOUSEDOWN_MODE:
        hbrEdge = GetSysColorBrush(COLOR_3DSHADOW);
        hbrFace = (HBRUSH)GetStockObject(BLACK_BRUSH);
        break;
    case WSB_DISABLED_MODE:
        hbrEdge = GetSysColorBrush(COLOR_3DHILIGHT);
        hbrFace = GetSysColorBrush(COLOR_3DFACE);
        break;
    case WSB_RESTING_MODE:
    default:
        hbrEdge = GetSysColorBrush(COLOR_3DSHADOW);
        hbrFace = GetSysColorBrush(COLOR_3DFACE);
        break;
    }
    hbrOld = SelectObject(hdc, hbrEdge);
    PatBlt(hdc, l, t, w, 1, PATCOPY);
    PatBlt(hdc, l, t, 1, h, PATCOPY);
    PatBlt(hdc, l, t + h - 1, w, 1, PATCOPY);
    PatBlt(hdc, l + w - 1, t, 1, h, PATCOPY);

    SelectObject(hdc, hbrFace);
    PatBlt(hdc, l + 1, t + 1, w - 2, h - 2, PATCOPY);
    SelectObject(hdc, hbrOld);
}

void FlatSB_Internal_DrawEncartaBox(HDC hdc, CONST RECT * prct, int mode)
{
    HBRUSH hbrOld, hbrLite, hbrDark, hbrFace;

    int w, h, l, t;

    if (prct->left > prct->right)
        return;
    else if (prct->top > prct->bottom)
        return;

    l = prct->left;
    t = prct->top;
    w = prct->right - prct->left;
    h = prct->bottom - prct->top;

    switch (mode)   {
    case WSB_HOTTRACKED_MODE:
        hbrLite = GetSysColorBrush(COLOR_3DHILIGHT);
        hbrDark = GetSysColorBrush(COLOR_3DSHADOW);
        break;
    case WSB_MOUSEDOWN_MODE:
        hbrDark = GetSysColorBrush(COLOR_3DHILIGHT);
        hbrLite = GetSysColorBrush(COLOR_3DSHADOW);
        break;
    case WSB_DISABLED_MODE:
        hbrDark = hbrLite = GetSysColorBrush(COLOR_3DHILIGHT);
        break;
    case WSB_RESTING_MODE:
    default:
        hbrDark = hbrLite = GetSysColorBrush(COLOR_3DSHADOW);
        break;
    }

    hbrFace = GetSysColorBrush(COLOR_3DFACE);

    hbrOld = SelectObject(hdc, hbrLite);
    PatBlt(hdc, l, t, w, 1, PATCOPY);
    PatBlt(hdc, l, t, 1, h, PATCOPY);

    SelectObject(hdc, hbrDark);
    PatBlt(hdc, l, t + h - 1, w, 1, PATCOPY);
    PatBlt(hdc, l + w - 1, t, 1, h, PATCOPY);

    SelectObject(hdc, hbrFace);
    PatBlt(hdc, l + 1, t + 1, w - 2, h - 2, PATCOPY);

    SelectObject(hdc, hbrOld);
}

void FlatSB_Internal_DrawArrow(WSBState * pWState, HDC hdc, CONST RECT * rcArrow, int buttonIndex, int extraModeBits)
{
    COLORREF rgb;
    LPCTSTR strIndex;
    HFONT   hFont, hOldFont;
    int     x, y, cx, cy, iOldBk, c;
    BOOL    fDisabled = extraModeBits & DFCS_INACTIVE;
    BOOL    fMouseDown = extraModeBits & DFCS_PUSHED;
    BOOL    fHotTracked;
    int     mode, style;

    if (rcArrow->left >= rcArrow->right)
        return;
    else if (rcArrow->top >= rcArrow->bottom)
        return;

    if (buttonIndex == WSB_BUTTON_LEFTARROW || buttonIndex == WSB_BUTTON_RIGHTARROW)
        style = pWState->hStyle;
    else
        style = pWState->vStyle;

    switch (buttonIndex)    {
    case WSB_BUTTON_LEFTARROW:
        fHotTracked = (pWState->locMouse == WSB_MOUSELOC_ARROWLF);
        strIndex = TEXT("3");
        break;
    case WSB_BUTTON_RIGHTARROW:
        fHotTracked = (pWState->locMouse == WSB_MOUSELOC_ARROWRG);
        strIndex = TEXT("4");
        break;
    case WSB_BUTTON_UPARROW:
        fHotTracked = (pWState->locMouse == WSB_MOUSELOC_ARROWUP);
        strIndex = TEXT("5");
        break;
    case WSB_BUTTON_DOWNARROW:
        fHotTracked = (pWState->locMouse == WSB_MOUSELOC_ARROWDN);
        strIndex = TEXT("6");
        break;
    default:
        return;
    }

    if (!fDisabled && fHotTracked && pWState->fHitOld)
        fMouseDown = TRUE;

    if (style == FSB_REGULAR_MODE) {
        RECT rc;

        CopyRect(&rc, rcArrow);
        if (fDisabled)
            DrawFrameControl(hdc, &rc, DFC_SCROLL, buttonIndex | DFCS_INACTIVE);
        else if (fMouseDown)
            DrawFrameControl(hdc, &rc, DFC_SCROLL, buttonIndex | DFCS_FLAT);
        else
            DrawFrameControl(hdc, &rc, DFC_SCROLL, buttonIndex);
        return;
    }

    if (fDisabled)
        mode = WSB_DISABLED_MODE;
    else if (fMouseDown)
        mode = WSB_MOUSEDOWN_MODE;
    else if (fHotTracked)
        mode = WSB_HOTTRACKED_MODE;
    else
        mode = WSB_RESTING_MODE;

    if (style == FSB_ENCARTA_MODE)  {
        FlatSB_Internal_DrawEncartaBox(hdc, rcArrow, mode);
    } else  {
        FlatSB_Internal_DrawBox(hdc, rcArrow, mode);
    }

    cx = rcArrow->right - rcArrow->left;
    cy = rcArrow->bottom - rcArrow->top;
    c = min(cx, cy);

    if (c < 4)      // Couldn't fill in a char after drawing the edges.
        return;

    x = rcArrow->left + ((cx - c) / 2) + 2;
    y = rcArrow->top + ((cy - c) / 2) + 2;

    c -= 4;

    if (style == FSB_FLAT_MODE) {
        switch (mode)   {
        case WSB_RESTING_MODE:
            rgb = RGB(0, 0, 0);
            break;
        case WSB_HOTTRACKED_MODE:
        case WSB_MOUSEDOWN_MODE:
            rgb = RGB(255, 255, 255);
            break;
        case WSB_DISABLED_MODE:
            rgb = GetSysColor(COLOR_3DSHADOW);
            break;
        default:
            rgb = RGB(0, 0, 0);
            break;
        }
    } else  {   //  FSB_ENCARTA_MODE
        switch (mode)   {
        case WSB_DISABLED_MODE:
            rgb = GetSysColor(COLOR_3DSHADOW);
            break;
        case WSB_RESTING_MODE:
        case WSB_HOTTRACKED_MODE:
        case WSB_MOUSEDOWN_MODE:
        default:
            rgb = RGB(0, 0, 0);
            break;
        }
    }

    hFont = CreateFont(c, 0, 0, 0, FW_NORMAL, 0, 0, 0, SYMBOL_CHARSET, 0, 0, 0, 0, WSB_SYS_FONT);
    iOldBk = SetBkMode(hdc, TRANSPARENT);
    hOldFont = SelectObject(hdc, hFont);

    rgb = SetTextColor(hdc, rgb);
    TextOut(hdc, x, y, strIndex, 1);

    SetBkMode(hdc, iOldBk);
    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);

    return;
}

void FlatSB_Internal_DrawElevator(WSBState * pWState, HDC hdc, LPRECT lprc, BOOL fVert)
{
    BOOL    fHit;
    int     mode;
    int     style;

    fHit = (fVert)?(pWState->locMouse == WSB_MOUSELOC_V_THUMB)
                  :(pWState->locMouse == WSB_MOUSELOC_H_THUMB);

    style = (fVert)?pWState->vStyle:pWState->hStyle;
    switch (style)  {
    case FSB_FLAT_MODE:
    case FSB_ENCARTA_MODE:
        if ((pWState->cmdSB == SB_THUMBPOSITION) && (fVert == pWState->fTrackVert))
            mode = WSB_HOTTRACKED_MODE;
        else
            mode = (fHit)?WSB_HOTTRACKED_MODE:WSB_RESTING_MODE;

        if (style == FSB_FLAT_MODE)
            FlatSB_Internal_DrawBox(hdc, lprc, mode);
        else
            FlatSB_Internal_DrawEncartaBox(hdc, lprc, mode);
        break;
    case FSB_REGULAR_MODE:
    default:
        {
            RECT rc;

            CopyRect(&rc, lprc);
            DrawFrameControl(hdc, &rc, DFC_BUTTON, DFCS_BUTTONPUSH);
        }
        break;
    }
}

//=-------------------------------------------------------------
//  FlatSB_Internal_DrawSize
//      Draw the size grip if needed.
//=-------------------------------------------------------------

void FlatSB_Internal_DrawSize(WSBState * pWState, HDC hdc, int x, int y)
{
    HBRUSH  hbrSave, hbr3DFACE;
    RECT    rcWindow;
    HWND    hwnd = pWState->sbHwnd;
    int     style;

    style = GetWindowLong(hwnd, GWL_STYLE);
    if (!FlatSB_Internal_IsSizeBox(hwnd))
    {
        hbr3DFACE = GetSysColorBrush(COLOR_3DFACE);
        hbrSave = SelectObject(hdc, hbr3DFACE);
        PatBlt(hdc, x, y, pWState->x_VSBArrow, pWState->y_HSBArrow, PATCOPY);
        SelectBrush(hdc, hbrSave);
    }
    else
    {
        rcWindow.left = x;
        rcWindow.right = x + pWState->x_VSBArrow;
        rcWindow.top = y;
        rcWindow.bottom = y + pWState->y_HSBArrow;
        DrawFrameControl(hdc, &rcWindow, DFC_SCROLL, DFCS_SCROLLSIZEGRIP);
    }
}

//=-------------------------------------------------------------
//  FlatSB_Internal_DrawGroove
//      Draw lines & middle of the thumb groove
//=-------------------------------------------------------------

void FlatSB_Internal_DrawGroove(WSBState * pWState, HDC hdc, LPRECT prct, BOOL fVert)
{
    HBRUSH   hbrLight;
    COLORREF cBkg, cFg;
    HPALETTE oldPal = 0;

    if (fVert)  {
        hbrLight = pWState->hbr_VSBBkg;
        cBkg = pWState->col_VSBBkg;
    } else  {
        hbrLight = pWState->hbr_HSBBkg;
        cBkg = pWState->col_HSBBkg;
    }

    if (hbrLight == (HBRUSH)NULL)   {
        hbrLight = GetSysColorBrush(COLOR_3DLIGHT);
        FillRect(hdc, prct, hbrLight);
        return;
    }

    if (pWState->hPalette != (HPALETTE)NULL)    {
        oldPal = SelectPalette(hdc, pWState->hPalette, TRUE);
        RealizePalette(hdc);
    }

    cFg = SetTextColor(hdc, GetSysColor(COLOR_3DFACE));
    cBkg = SetBkColor(hdc, cBkg);
    FillRect(hdc, prct, hbrLight);
    if (oldPal != (HPALETTE)NULL)
        SelectPalette(hdc, oldPal, TRUE);

    SetTextColor(hdc, cFg);
    SetBkColor(hdc, cBkg);
}


//=-------------------------------------------------------------------
//  Following functions are ported from winsbctl.c in user code.
//=-------------------------------------------------------------------


//=-------------------------------------------------------------------------
//  SBPosFromPx() -
//=-------------------------------------------------------------------------

int FlatSB_Internal_SBPosFromPx(WSBState * pWState, int px)
{
    int * pw;

    if (pWState->fTrackVert)
        pw = &(pWState->sbVMinPos);
    else
        pw = &(pWState->sbHMinPos);

    if (px < pWState->pxUpArrow)
        return pw[SBO_MIN];

    if (px >= pWState->pxDownArrow)
            return (pw[SBO_MAX] - (pw[SBO_PAGE]?pw[SBO_PAGE] - 1 : 0));

    return (pw[SBO_MIN] + DMultDiv(pw[SBO_MAX] - pw[SBO_MIN] - (pw[SBO_PAGE]?pw[SBO_PAGE] - 1 : 0),
                                   px - pWState->pxUpArrow,
                                   pWState->cpxSpace)
           );
}

//=-------------------------------------------------------------------------
//  InvertScrollHilite()
//=-------------------------------------------------------------------------

void FlatSB_Internal_InvertScrollHilite(WSBState * pWState)
{
    HWND hwnd = pWState->sbHwnd;
    HDC hdc;

    // Don't invert if the thumb is all the way at the top or bottom
    // or you will end up inverting the line between the arrow and the thumb.
    if (!IsRectEmpty(&(pWState->rcTrack)))
    {
        hdc = GetWindowDC(hwnd);
        InvertRect(hdc, &(pWState->rcTrack));
        ReleaseDC(hwnd, hdc);
    }
}

//=-------------------------------------------------------------------------
//  FlatSB_Internal_MoveThumb()
//=-------------------------------------------------------------------------

void FlatSB_Internal_MoveThumb(WSBState * pWState, int px)
{
    HWND    hwnd = pWState->sbHwnd;
    HDC     hdc;

    if (px == pWState->pxOld)
        return;

pxReCalc:

    pWState->posNew = FlatSB_Internal_SBPosFromPx(pWState, px);

    /* Tentative position changed -- notify the guy. */
    if (pWState->posNew != pWState->posOld) {
        FlatSB_Internal_DoScroll(pWState, SB_THUMBTRACK, pWState->posNew, pWState->fTrackVert);
        if (!pWState->fTracking)
            return;

        pWState->posOld = pWState->posNew;

        //
        // Anything can happen after the SendMessage above in DoScroll!
        // Make sure that the SBINFO structure contains data for the
        // window being tracked -- if not, recalculate data in SBINFO
        // If fVertSB is TRUE, the last CalcSBStuff call is for SB_VERT.
        // If fTrackVert != fVertSB, we got garbage in pWState.
        //

        if (pWState->fTrackVert != pWState->fVertSB)
            FlatSB_Internal_CalcSBStuff(pWState, pWState->fTrackVert);

        // when we yield, our range can get messed with
        // so make sure we handle this

        if (px >= pWState->pxDownArrow - pWState->cpxThumb) {
            px = pWState->pxDownArrow - pWState->cpxThumb;
            goto pxReCalc;
        }
    }

    hdc = GetWindowDC(hwnd);

    pWState->pxThumbTop = px;
    pWState->pxThumbBottom = pWState->pxThumbTop + pWState->cpxThumb;

    //  At this point, the disable flags are always going to be 0 --
    //  we're in the middle of tracking.

    //  We are Okay in this case, since in DrawElevator we decide the mode by
    //  cmd == SB_THUMBPOSITION.
    FlatSB_Internal_DrawThumb2(pWState, hdc, pWState->fTrackVert, 0);
    ReleaseDC(hwnd, hdc);

    pWState->pxOld = px;
}

//=-------------------------------------------------------------------------
//  DrawInvertScrollArea() -
//=-------------------------------------------------------------------------

void FlatSB_Internal_DrawInvertScrollArea(WSBState * pWState, BOOL fHit, int cmd)
{
    HWND hwnd = pWState->sbHwnd;
    HDC  hdc;

    if ((cmd != SB_LINEUP) && (cmd != SB_LINEDOWN))
    {
        FlatSB_Internal_InvertScrollHilite(pWState);
        FlatSB_Internal_NotifyWinEvent(pWState, EVENT_OBJECT_STATECHANGE,
                         cmd == SB_PAGEUP ? INDEX_SCROLLBAR_UPPAGE
                                          : INDEX_SCROLLBAR_DOWNPAGE);
        return;
    }

    hdc = GetWindowDC(hwnd);
    if (cmd == SB_LINEUP) {
        if (pWState->fTrackVert)   {
            FlatSB_Internal_DrawArrow(pWState, hdc, &(pWState->rcTrack), DFCS_SCROLLUP, (fHit) ? DFCS_PUSHED : 0);
        } else  {
            FlatSB_Internal_DrawArrow(pWState, hdc, &(pWState->rcTrack), DFCS_SCROLLLEFT, (fHit) ? DFCS_PUSHED : 0);
        }
    } else {
        if (pWState->fTrackVert)   {
            FlatSB_Internal_DrawArrow(pWState, hdc, &(pWState->rcTrack), DFCS_SCROLLDOWN, (fHit) ? DFCS_PUSHED : 0);
        } else  {
            FlatSB_Internal_DrawArrow(pWState, hdc, &(pWState->rcTrack), DFCS_SCROLLRIGHT, (fHit) ? DFCS_PUSHED : 0);
        }
    }

    FlatSB_Internal_NotifyWinEvent(pWState, EVENT_OBJECT_STATECHANGE,
                     cmd == SB_LINEUP ? INDEX_SCROLLBAR_UP : INDEX_SCROLLBAR_DOWN);

    ReleaseDC(hwnd, hdc);

}

//=-------------------------------------------------------------------------
//  FlatSB_Internal_EndScroll() -
//=-------------------------------------------------------------------------

void FlatSB_Internal_EndScroll(WSBState * pWState, BOOL fCancel)
{
    HWND hwnd = pWState->sbHwnd;
    BOOL fVert = pWState->fTrackVert;
    int oldcmd;

    if (pWState->fTracking)
    {
        oldcmd = pWState->cmdSB;
        pWState->cmdSB = 0;

        //  will not have capture if called by CancelModeProc
        if (GetCapture() == hwnd)
            ReleaseCapture();

        if (pWState->pfnSB == FlatSB_Internal_TrackThumb)
        {
            if (fCancel)    {
                pWState->posOld = pWState->posStart;
            }

            FlatSB_Internal_DoScroll(pWState, SB_THUMBPOSITION, pWState->posOld, fVert);
            FlatSB_Internal_DrawThumb(pWState, fVert);
        }
        else if (pWState->pfnSB == FlatSB_Internal_TrackBox)
        {
            DWORD   lpt;
            RECT    rcWindow;
            POINT   pt;

            if (pWState->hTimerSB)
                KillTimer(hwnd, IDSYS_SCROLL);

            lpt = GetMessagePos();

            ASSERT(hwnd != GetDesktopWindow());

            GetWindowRect(hwnd, &rcWindow);
            pt.x = GET_X_LPARAM(lpt) - rcWindow.left;
            pt.y = GET_Y_LPARAM(lpt) - rcWindow.top;

            if (PtInRect(&(pWState->rcTrack), pt))  {
                pWState->fHitOld = FALSE;
                FlatSB_Internal_DrawInvertScrollArea(pWState, FALSE, oldcmd);
            }
        }

        //  Always send SB_ENDSCROLL message.
        pWState->pfnSB = NULL;

        //  Anything can happen here. Client can call GetScrollInfo for THUMBPOSITION, and we
        //  should return 0, so we should set pfnSB to NULL first.
        FlatSB_Internal_DoScroll(pWState, SB_ENDSCROLL, 0, fVert);
        pWState->fTracking = FALSE;
        pWState->fHitOld = FALSE;

        FlatSB_Internal_NotifyWinEvent(pWState, EVENT_SYSTEM_SCROLLINGEND,
                                       INDEXID_CONTAINER);
        //  Redraw the components.
        FlatSB_NCHitTestProc(pWState, hwnd, 0, 0, TRUE);
    }
}

//=-------------------------------------------------------------------------
//  FlatSB_Internal_DoScroll() -
//=-------------------------------------------------------------------------

void FlatSB_Internal_DoScroll(WSBState *pWState, int cmd, int pos, BOOL fVert)
{
    if (pWState->sbHwnd)
    {
        pWState->fInDoScroll++;
        SendMessage(pWState->sbHwnd, (fVert ? WM_VSCROLL : WM_HSCROLL), (WPARAM)(LOWORD(pos) << 16 | (cmd & 0xffff)), (LPARAM)NULL);
        pWState->fInDoScroll--;
    }
}


//=-------------------------------------------------------------------------
//  TimerScroll()
//=--------------------------------------------------------------------------

VOID CALLBACK TimerScroll(HWND hwnd, UINT message, UINT id, DWORD time)
{
    LONG    pos;
    POINT   pt;
    UINT    dblClkTime, dtScroll;
    WSBState * pWState;
    RECT    rcWindow;


    GetWindowSubclass(hwnd, FlatSB_SubclassWndProc, 0, (ULONG_PTR *)&pWState);
    if ((pWState == (WSBState *)NULL) || (pWState == WSB_UNINIT_HANDLE))    {
        KillTimer(hwnd, IDSYS_SCROLL);
        return;
    }

    ASSERT(hwnd != GetDesktopWindow());

    pos = GetMessagePos();
    pt.x = GET_X_LPARAM(pos), pt.y = GET_Y_LPARAM(pos);
    dblClkTime = GetDoubleClickTime();
    dtScroll = (dblClkTime * 4) / 5;
    GetWindowRect(hwnd, &rcWindow);

    pt.x -= rcWindow.left;
    pt.y -= rcWindow.top;

    pos = LOWORD(pt.y) << 16 | LOWORD(pt.x);
    FlatSB_Internal_TrackBox(pWState, WM_NULL, 0, (LPARAM) pos);

    if (pWState->fHitOld)
    {
        pWState->hTimerSB = SetTimer(hwnd, IDSYS_SCROLL, dtScroll / 8, (TIMERPROC)TimerScroll);
        FlatSB_Internal_DoScroll(pWState, pWState->cmdSB, 0, pWState->fTrackVert);
    }
    return;
}

//=-------------------------------------------------------------------------
//  FlatSB_Internal_TrackBox() -
//=-------------------------------------------------------------------------

void FlatSB_Internal_TrackBox(WSBState * pWState, int message, WPARAM wParam, LPARAM lParam)
{
    HWND hwnd = pWState->sbHwnd;
    BOOL fHit, fVert = pWState->fTrackVert;
    BOOL fHitOld = pWState->fHitOld;
    POINT pt;
    int cmsTimer;
    UINT dblClkTime, dtScroll;

    if (message && (message < WM_MOUSEFIRST || message > WM_MOUSELAST))
        return;

    dblClkTime = GetDoubleClickTime();
    dtScroll = (dblClkTime * 4) / 5;

    pt.x = GET_X_LPARAM(lParam);
    pt.y = GET_Y_LPARAM(lParam);

    fHit = PtInRect(&(pWState->rcTrack), pt);

    if (fHit != fHitOld)   {
        pWState->fHitOld = fHit;
        FlatSB_Internal_DrawInvertScrollArea(pWState, fHit, pWState->cmdSB);
    }

    cmsTimer = dtScroll / 8;

    switch (message)
    {
        case WM_LBUTTONUP:
            FlatSB_Internal_EndScroll(pWState, FALSE);
            break;

        case WM_LBUTTONDOWN:
            pWState->hTimerSB = 0;
            cmsTimer = dtScroll;

            /*** FALL THRU ***/

        case WM_MOUSEMOVE:
            if (fHit && (fHit != fHitOld))
            {
                /* We moved back into the normal rectangle: reset timer */
                pWState->hTimerSB = SetTimer(hwnd, IDSYS_SCROLL, cmsTimer, (TIMERPROC)TimerScroll);
                FlatSB_Internal_DoScroll(pWState, pWState->cmdSB, 0, fVert);
            }
    }
}

//=-------------------------------------------------------------------------
//  FlatSB_Internal_TrackThumb() -
//=-------------------------------------------------------------------------

void FlatSB_Internal_TrackThumb(WSBState * pWState, int message, WPARAM wParam, LPARAM lParam)
{
    HWND    hwnd = pWState->sbHwnd;
    BOOL    fVert = pWState->fTrackVert;
    POINT   pt;

    if (message < WM_MOUSEFIRST || message > WM_MOUSELAST)
        return;

    // Make sure that the SBINFO structure contains data for the
    // window being tracked -- if not, recalculate data in SBINFO
    if (pWState->fTrackVert != pWState->fVertSB)
        FlatSB_Internal_CalcSBStuff(pWState, pWState->fTrackVert);

    pt.x = GET_X_LPARAM(lParam);
    pt.y = GET_Y_LPARAM(lParam);
     if (!PtInRect(&(pWState->rcTrack), pt))
        pWState->px = pWState->pxStart;
    else
    {
        pWState->px = (fVert ? GET_Y_LPARAM(lParam) : GET_X_LPARAM(lParam)) + pWState->dpxThumb;
        if (pWState->px < pWState->pxUpArrow)
            pWState->px = pWState->pxUpArrow;
        else if (pWState->px >= (pWState->pxDownArrow - pWState->cpxThumb))
            pWState->px = pWState->pxDownArrow - pWState->cpxThumb;
    }

    FlatSB_Internal_MoveThumb(pWState, pWState->px);

    if (message == WM_LBUTTONUP)
        FlatSB_Internal_EndScroll(pWState, FALSE);
}

//=-------------------------------------------------------------------------
//  FlatSB_Internal_SBTrackLoop() -
//=-------------------------------------------------------------------------

void FlatSB_Internal_SBTrackLoop(WSBState * pWState, LPARAM lParam)
{
    HWND    hwnd = pWState->sbHwnd;
    MSG     msg;
    int     cmd, newlParam;
    POINT   pt;

    if (!pWState->fTracking)
        return;

    FlatSB_Internal_NotifyWinEvent(pWState, EVENT_SYSTEM_SCROLLINGSTART,
                                   INDEXID_CONTAINER);

    (*(pWState->pfnSB))(pWState, WM_LBUTTONDOWN, 0, lParam);

    while (GetCapture() == hwnd)
    {
        if (!GetMessage(&msg, NULL, 0, 0))
            break;

        if (!CallMsgFilter(&msg, MSGF_SCROLLBAR)) {
            cmd = msg.message;

            if (msg.hwnd == hwnd &&
                ((cmd >= WM_MOUSEFIRST && cmd <= WM_MOUSELAST) ||
                (cmd >= WM_KEYFIRST && cmd <= WM_KEYLAST  )    ))
            {
            // Process Key
#define ALT_PRESSED 0x20000000L
                if (cmd >= WM_SYSKEYDOWN
                    && cmd <= WM_SYSDEADCHAR
                    && msg.lParam & ALT_PRESSED)
                    cmd -= (WM_SYSKEYDOWN - WM_KEYDOWN);
#undef ALT_PRESSED
                if (!pWState->fTracking)
                    return;

                // Change to coordinates according to left-top corner of the window.
                pt.x = GET_X_LPARAM(msg.lParam) + pWState->rcClient.left;
                pt.y = GET_Y_LPARAM(msg.lParam) + pWState->rcClient.top;

                newlParam = LOWORD(pt.y) << 16 | LOWORD(pt.x);

                (*(pWState->pfnSB))(pWState, cmd, msg.wParam, (LPARAM)newlParam);
            } else {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }
}

//=-------------------------------------------------------------------------
//  FlatSB_Internal_SBTrackInit() -
//=-------------------------------------------------------------------------

void FlatSB_Internal_SBTrackInit(WSBState * pWState, HWND hwnd, LPARAM lParam, int hitArea, BOOL fDirect)
{
    int     hitX = GET_X_LPARAM(lParam);
    int     hitY = GET_Y_LPARAM(lParam);
    int     px;
    int    *pwX;
    int    *pwY;
    int     wDisable;   // Scroll bar disable flags;
    RECT    rcWindow;
    BOOL    fVert;
    POINT   pt;

    // hitArea = 0 indicates a scroll bar control
    // otherwise, curArea will have the hit test area

    if (hitArea == HTHSCROLL)
        fVert = FALSE;
    else if (hitArea == HTVSCROLL)
        fVert = TRUE;
    else
        return;

    ASSERT(hwnd != GetDesktopWindow());

    GetWindowRect(hwnd, &rcWindow);
    pt.x = GET_X_LPARAM(lParam) - rcWindow.left;
    pt.y = GET_Y_LPARAM(lParam) - rcWindow.top;
    lParam = LOWORD(pt.y) << 16 | LOWORD(pt.x);

    wDisable = FlatSB_Internal_GetSBFlags(pWState, fVert);

    if ((wDisable & ESB_DISABLE_BOTH) == ESB_DISABLE_BOTH)  {
        // Whole Scroll Bar is disabled -- do not respond
        pWState->pfnSB = NULL;
        pWState->fTracking = FALSE;
        return;
    }

    pWState->hTimerSB = 0;
    pWState->fHitOld = FALSE;
    pWState->fTracking = FALSE;

    //  For the case we click on scroll bar of a nonactive window. The mode is set to FLAT
    //  by HitTestProc. This will work because we set the tracking flag right away.
    if (fVert)  {
        pWState->fVActive = TRUE;   pWState->fHActive = FALSE;
    } else  {
        pWState->fHActive = TRUE;   pWState->fVActive = FALSE;
    }

    //  This will give us the right locMouse. We will keep it till EndScroll.
    FlatSB_Internal_CalcSBStuff(pWState, fVert);

    //  From now till EndScroll, CalcSBStuff won't compute new locMouse.
    pWState->pfnSB = FlatSB_Internal_TrackBox;
    pWState->fTracking = TRUE;

    // Initialize rcSB to the Rectangle of the Entire Scroll Bar
    pwX = (int *)&(pWState->rcSB);
    pwY = pwX + 1;

    if (!fVert)
        pwX = pwY--;

    pwX[0] = pWState->pxLeft;
    pwY[0] = pWState->pxTop;
    pwX[2] = pWState->pxRight;
    pwY[2] = pWState->pxBottom;

    px = (fVert ? pt.y : pt.x);

    pWState->px = px;
    if (px < pWState->pxUpArrow)
    {   // The click occurred on Left/Up arrow
        if(wDisable & LTUPFLAG)
        {   // Disabled -- do not respond
            pWState->pfnSB = NULL;
            pWState->fTracking = FALSE;
            return;
        }

        // LINEUP -- make rcSB the Up Arrow's Rectangle
        pWState->cmdSB = SB_LINEUP;
        pwY[2] = pWState->pxUpArrow;
    }
    else if (px >= pWState->pxDownArrow)
    {   // The click occurred on Right/Down arrow
        if(wDisable & RTDNFLAG)
        {   // Disabled -- do not respond
            pWState->pfnSB = NULL;
            pWState->fTracking = FALSE;
            return;
        }

        // LINEDOWN -- make rcSB the Down Arrow's Rectangle
        pWState->cmdSB = SB_LINEDOWN;
        pwY[0] = pWState->pxDownArrow;
    }
    else if (px < pWState->pxThumbTop)
    {
        // PAGEUP -- make rcSB the rectangle between Up Arrow and Thumb
        pWState->cmdSB = SB_PAGEUP;

        pwY[0] = pWState->pxUpArrow;
        pwY[2] = pWState->pxThumbTop;
    }
    else if (px < pWState->pxThumbBottom)
    {
DoThumbPos:
        if (pWState->pxDownArrow - pWState->pxUpArrow <= pWState->cpxThumb) {
            // Not enough room -- elevator isn't there
            pWState->pfnSB = NULL;
            pWState->fTracking = FALSE;
            return;
        }
        // THUMBPOSITION -- we're tracking with the thumb
        pWState->cmdSB = SB_THUMBPOSITION;
        pWState->fTrackVert = fVert;
        CopyRect(&(pWState->rcTrack), &(pWState->rcSB));

        if (pWState->sbGutter < 0) {
            // Negative gutter means "infinite size"
            pWState->rcTrack.top = MINLONG;
            pWState->rcTrack.left = MINLONG;
            pWState->rcTrack.right = MAXLONG;
            pWState->rcTrack.bottom = MAXLONG;
        } else
        if (fVert)
            InflateRect(&(pWState->rcTrack),
                        (pWState->rcTrack.right - pWState->rcTrack.left) * pWState->sbGutter,
                        pWState->y_VSBThumb * pWState->sbGutter);
        else
            InflateRect(&(pWState->rcTrack),
                        pWState->x_HSBThumb * pWState->sbGutter,
                        (pWState->rcTrack.bottom - pWState->rcTrack.top) * pWState->sbGutter);

        pWState->pfnSB = FlatSB_Internal_TrackThumb;
        pWState->pxOld  = pWState->pxStart  = pWState->pxThumbTop;
        pWState->posOld = pWState->posNew = pWState->posStart = fVert?pWState->sbVThumbPos:pWState->sbHThumbPos;
        pWState->dpxThumb = pWState->pxThumbTop - pWState->px;

        SetCapture(hwnd);
        FlatSB_Internal_DoScroll(pWState, SB_THUMBTRACK, pWState->posOld, fVert);
        FlatSB_Internal_DrawThumb(pWState, fVert);
    }
    else if (px < pWState->pxDownArrow)
    {
        // PAGEDOWN -- make rcSB the rectangle between Thumb and Down Arrow
        pWState->cmdSB = SB_PAGEDOWN;

        pwY[0] = pWState->pxThumbBottom;
        pwY[2] = pWState->pxDownArrow;
    }

    // NT5-style tracking:  Shift+Click = "Go here"
    if (g_bRunOnNT5 && fDirect && pWState->cmdSB != SB_LINEUP && pWState->cmdSB != SB_LINEDOWN) {
        if (pWState->cmdSB != SB_THUMBPOSITION) {
            goto DoThumbPos;
        }
        pWState->dpxThumb = -(pWState->cpxThumb / 2);
    }

    if (pWState->cmdSB != SB_THUMBPOSITION) {
        pWState->fTrackVert = fVert;
        SetCapture(hwnd);
        CopyRect(&(pWState->rcTrack), &(pWState->rcSB));
    }

    FlatSB_Internal_SBTrackLoop(pWState, lParam);
}

//=-------------------------------------------------------------------------
//  GetScroll...() -
//=-------------------------------------------------------------------------

int WINAPI FlatSB_GetScrollPos(HWND hwnd, int code)
{
    WSBState * pWState;

    ASSERT (code != SB_CTL);

    GetWindowSubclass(hwnd, FlatSB_SubclassWndProc, 0, (ULONG_PTR *)&pWState);
    if (pWState == (WSBState *)NULL)    {
        return GetScrollPos(hwnd, code);
    } else if (pWState == WSB_UNINIT_HANDLE) {
        return 0;
    } else if (pWState->sbHwnd != hwnd) {
        return 0;
    } else  {
        return ((code == SB_VERT)?pWState->sbVThumbPos:pWState->sbHThumbPos);
    }
}

BOOL WINAPI FlatSB_GetScrollPropPtr(HWND hwnd, int propIndex, PINT_PTR pValue)
{
    WSBState * pWState;

    if (!pValue)
        return FALSE;
    else
        *pValue = 0;    //  If we can't set it, we reset it.

    GetWindowSubclass(hwnd, FlatSB_SubclassWndProc, 0, (ULONG_PTR *)&pWState);
    if (pWState == (WSBState *)NULL)    {
        return FALSE;
    } else if (pWState == WSB_UNINIT_HANDLE) {
        pWState = FlatSB_Internal_InitPwSB(hwnd);
        if (pWState == (WSBState *)NULL)
            return FALSE;
        else if (!SetWindowSubclass(hwnd, FlatSB_SubclassWndProc, 0,  (ULONG_PTR)pWState)) {
            DeleteObject(pWState->hbm_Bkg);
            DeleteObject(pWState->hbr_Bkg);
            LocalFree((HLOCAL)pWState);
            return FALSE;
        } else  {
        //  Fall through.
        }
    } else if (pWState->sbHwnd != hwnd) {
        return FALSE;
    }

    switch (propIndex)  {
    case WSB_PROP_CYVSCROLL:
        *pValue = pWState->metApp.cyVSBArrow;
        break;
    case WSB_PROP_CXVSCROLL:
        *pValue = pWState->metApp.cxVSBArrow;
        break;
    case WSB_PROP_CYHSCROLL:
        *pValue = pWState->metApp.cyHSBArrow;
        break;
    case WSB_PROP_CXHSCROLL:
        *pValue = pWState->metApp.cxHSBArrow;
        break;
    case WSB_PROP_CXHTHUMB:
        *pValue = pWState->metApp.cxHSBThumb;
        break;
    case WSB_PROP_CYVTHUMB:
        *pValue = pWState->metApp.cyVSBThumb;
        break;
    case WSB_PROP_WINSTYLE:
        //  To check if a scrollbar is present, the WF(HV)PRESENT bits may
        //  be more useful than WS_(HV)SCROLL bits.
        *pValue = pWState->style;
        break;
    case WSB_PROP_HSTYLE:
        *pValue = pWState->hStyle;
        break;
    case WSB_PROP_VSTYLE:
        *pValue = pWState->vStyle;
        break;
    case WSB_PROP_HBKGCOLOR:
        *pValue = pWState->col_HSBBkg;
        break;
    case WSB_PROP_VBKGCOLOR:
        *pValue = pWState->col_VSBBkg;
        break;
    case WSB_PROP_PALETTE:
        *pValue = (INT_PTR)pWState->hPalette;
        break;
    case WSB_PROP_GUTTER:
        *pValue = pWState->sbGutter;
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

#ifdef _WIN64

BOOL WINAPI FlatSB_GetScrollProp(HWND hwnd, int propIndex, LPINT pValue)
{
    INT_PTR iValue;
    BOOL fRc;

    if (!pValue)
        return FALSE;

#ifdef DEBUG
    if (propIndex == WSB_PROP_PALETTE)
    {
        TraceMsg(TF_ERROR, "FlatSB_GetScrollProp(WSB_PROP_PALETTE): Use GetScrollPropPtr for Win64 compat");
    }
#endif

    fRc = FlatSB_GetScrollPropPtr(hwnd, propIndex, &iValue);
    *pValue = (int)iValue;

    return fRc;
}
#endif

BOOL WINAPI FlatSB_GetScrollRange(HWND hwnd, int code, LPINT lpposMin, LPINT lpposMax)
{
    int     *pw;
    WSBState * pWState;

    ASSERT(code != SB_CTL);
    if (!lpposMin || !lpposMax)
        return FALSE;

    GetWindowSubclass(hwnd, FlatSB_SubclassWndProc, 0, (ULONG_PTR *)&pWState);
    if (pWState == (WSBState *)NULL)    {
        return GetScrollRange(hwnd, code, lpposMin, lpposMax);
//        *lpposMin = 0;
//        *lpposMax = 0;
    } else if (pWState == WSB_UNINIT_HANDLE) {
        *lpposMin = 0;
        *lpposMax = 0;
    } else if (pWState->sbHwnd != hwnd) {
        return FALSE;
    } else {
        pw = (code == SB_VERT) ? &(pWState->sbVMinPos) : &(pWState->sbHMinPos);
        *lpposMin = pw[SBO_MIN];
        *lpposMax = pw[SBO_MAX];
    }

    return TRUE;
}

BOOL WINAPI FlatSB_GetScrollInfo(HWND hwnd, int fnBar, LPSCROLLINFO lpsi)
{
    int *pw;
    WSBState * pWState;

    ASSERT(fnBar != SB_CTL);

    //  ZDC@Oct. 10, Detect GP faults here.
    if ((LPSCROLLINFO)NULL == lpsi)
        return FALSE;

    if (lpsi->cbSize < sizeof (SCROLLINFO))
        return FALSE;

    //  ZDC@Oct. 11, Don't zero out buffer anymore.
    GetWindowSubclass(hwnd, FlatSB_SubclassWndProc, 0, (ULONG_PTR *)&pWState);
    if (pWState == (WSBState *)NULL)    {
        return GetScrollInfo(hwnd, fnBar, lpsi);
    } else if (pWState == WSB_UNINIT_HANDLE) {
        return FALSE;
    } else if (pWState->sbHwnd != hwnd) {
        return FALSE;
    } else if (fnBar == SB_VERT)    {
        pw = &(pWState->sbVMinPos);
    } else if (fnBar == SB_HORZ)    {
        pw = &(pWState->sbHMinPos);
    } else {
        return FALSE;
    }

    if (lpsi->fMask & SIF_RANGE)
        lpsi->nMin = pw[SBO_MIN], lpsi->nMax = pw[SBO_MAX];
    if (lpsi->fMask & SIF_POS)
        lpsi->nPos = pw[SBO_POS];
    if (lpsi->fMask & SIF_PAGE)
        lpsi->nPage = pw[SBO_PAGE];
    // ZDC@Oct 9, Add support for SIF_TRACKPOS
    if (lpsi->fMask & SIF_TRACKPOS) {
        //  This is the olny place that pfnSB is used instead of fTracking.
        if (pWState->pfnSB != NULL) {
            if ((fnBar == SB_VERT) && pWState->fTrackVert)
                lpsi->nTrackPos = pWState->posNew;
            else if ((fnBar == SB_HORZ) && !(pWState->fTrackVert))
                lpsi->nTrackPos = pWState->posNew;
            else
                lpsi->nTrackPos = pw[SBO_POS];
        } else
            lpsi->nTrackPos = pw[SBO_POS];
    }

    return TRUE;
}

BOOL WINAPI FlatSB_ShowScrollBar(HWND hwnd, int fnBar, BOOL fShow)
{
    BOOL fChanged = FALSE;
    int newStyle = 0;
    WSBState * pWState;

    ASSERT(fnBar != SB_CTL);

    GetWindowSubclass(hwnd, FlatSB_SubclassWndProc, 0, (ULONG_PTR *)&pWState);
    if (pWState == (WSBState *)NULL)
        return ShowScrollBar(hwnd, fnBar, fShow);

    switch (fnBar) {
    case SB_VERT:
        newStyle = WS_VSCROLL;
        break;
    case SB_HORZ:
        newStyle = WS_HSCROLL;
        break;
    case SB_BOTH:
        newStyle = WS_VSCROLL | WS_HSCROLL;
        break;
    default:
        return FALSE;
    }

    if (pWState == WSB_UNINIT_HANDLE) {
        if (fShow)  {
            pWState = FlatSB_Internal_InitPwSB(hwnd);
            if (pWState == (WSBState *)NULL)
                return FALSE;
            else if (!SetWindowSubclass(hwnd, FlatSB_SubclassWndProc, 0,  (ULONG_PTR)pWState)) {
                DeleteObject(pWState->hbm_Bkg);
                DeleteObject(pWState->hbr_Bkg);
                LocalFree((HLOCAL)pWState);
                return FALSE;
            }
        } else  {
            return FALSE;
        }
    }

    if (!fShow) {
        if (pWState->style & newStyle) {
            fChanged = TRUE;
            pWState->style &= ~newStyle;
        }
    } else {
        if ((pWState->style & newStyle) != newStyle)    {
            fChanged = TRUE;
            pWState->style |= newStyle;
        }
    }

    if (fChanged) {
        // Keep USER scrollbars in sync for accessibility
        ShowScrollBar(hwnd, fnBar, fShow);
        CCInvalidateFrame(hwnd);
    }

    return TRUE;
}

//=------------------------------------------------------------------
//  Following functions are ported from winsb.c in user code.
//=------------------------------------------------------------------

//=--------------------------------------------------------------
// InitPwSB
//     [in]    hwnd
// Note:
//     This function is only a memory allocating func. It won't
//     do any check. On the other hand, this function should be
//     called before any consequent functions are used.
//=--------------------------------------------------------------

WSBState * FlatSB_Internal_InitPwSB(HWND hwnd)
{
    int     patGray[4];
    HBITMAP hbm;
    WSBState * pw;

    pw = (WSBState *)LocalAlloc(LPTR, sizeof(WSBState));
    // The buffer should already be zero-out.

    if (pw == (WSBState *)NULL)
        return pw;

    patGray[0] = 0x005500AA;
    patGray[1] = 0x005500AA;
    patGray[2] = 0x005500AA;
    patGray[3] = 0x005500AA;

    pw->sbVMaxPos = pw->sbHMaxPos = 100;
    pw->sbHwnd = hwnd;

    // We start out with app metrics equal to system metrics
    FlatSB_InitWSBMetrics(pw);
    pw->metApp = pw->metSys;

    //
    //  NT5's gutter is 8; Win9x's and NT4's gutter is 2.
    //
    pw->sbGutter = g_bRunOnNT5 ? 8 : 2;

    // ZDC
    //     make sure get hbm_Bkg and hbr_Bkg deleted.
    hbm = CreateBitmap(8, 8, 1, 1, (LPSTR)patGray);

    if ((HBITMAP)NULL == hbm)   {
        LocalFree((HLOCAL)pw);
        return NULL;
    }

    pw->hbr_VSBBkg = CreatePatternBrush(hbm);
    if ((HBRUSH)NULL == pw->hbr_VSBBkg)  {
        DeleteObject(hbm);
        LocalFree((HLOCAL)pw);
        return NULL;
    }

    pw->hbr_Bkg = pw->hbr_HSBBkg = pw->hbr_VSBBkg;
    pw->col_VSBBkg = pw->col_HSBBkg = RGB(255, 255, 255);
    pw->hbm_Bkg = hbm;
    pw->hStyle = pw->vStyle = FSB_FLAT_MODE;    //  Default state: Flat.
    pw->ptMouse.x = -1;
    pw->ptMouse.y = -1;

    return(pw);
}

void FlatSB_Internal_RedrawScrollBar(WSBState * pWState, BOOL fVert)
{
    HDC hdc;

    hdc = GetWindowDC(pWState->sbHwnd);
    FlatSB_Internal_DrawScrollBar(pWState, hdc, fVert, TRUE);
    ReleaseDC(pWState->sbHwnd, hdc);
}

//=-------------------------------------------------------------
// FlatSB_Internal_GetSBFlags
//=-------------------------------------------------------------

UINT FlatSB_Internal_GetSBFlags(WSBState * pWState, BOOL fVert)
{
    int wFlags;

    if (pWState == (WSBState *)NULL)    {
        return(0);
    }

    wFlags = pWState->sbFlags;

    return(fVert ? (wFlags & WSB_VERT) >> 2 : wFlags & WSB_HORZ);
}

//=--------------------------------------------------------------
//  return TRUE if there is a change.
//=--------------------------------------------------------------

BOOL WINAPI FlatSB_EnableScrollBar(HWND hwnd, int wSBflags, UINT wArrows)
{
    WSBState * pWState;

    GetWindowSubclass(hwnd, FlatSB_SubclassWndProc, 0, (ULONG_PTR *)&pWState);
    if (pWState == (WSBState *)NULL)    {
        return EnableScrollBar(hwnd, wSBflags, wArrows);
    } else if (pWState == WSB_UNINIT_HANDLE) {
        if (wArrows == ESB_ENABLE_BOTH)
            //  Leave it to later calls.
            return FALSE;
        else    {
            pWState = FlatSB_Internal_InitPwSB(hwnd);
            if (pWState == (WSBState *)NULL)
                return FALSE;
            else if (!SetWindowSubclass(hwnd, FlatSB_SubclassWndProc, 0,  (ULONG_PTR)pWState)) {
                DeleteObject(pWState->hbm_Bkg);
                DeleteObject(pWState->hbr_Bkg);
                LocalFree((HLOCAL)pWState);
                return FALSE;
            }
        }
    } else if (hwnd != pWState->sbHwnd) {
        return FALSE;
    }

    return FlatSB_Internal_EnableScrollBar(pWState, wSBflags, wArrows);
}

//=-------------------------------------------------------------
// FlatSB_Internal_EnableScrollBar
//
// Note:
//     The func will simply fail in case of uninitialized pointer
//     pWState is passed.
//     Since we now use WSBState * as handle, we always hope it's
//     valid already.
//
//     The following func is implemented following the comments in
//     winsbctl.c and the comment of the in MSDN library. In
//     access\inc16\windows.h you can find:
//     #define SB_DISABLE_MASK ESB_DISABLE_BOTH    // 0x03
//
//     The sbFlags is slightly different with rgwScroll[SB_FLAGS].
//=-------------------------------------------------------------

BOOL FlatSB_Internal_EnableScrollBar(WSBState * pWState, int wSBflags, UINT wArrows)
{
    int     wOldFlags;
    int     style;
    BOOL    bRetValue = FALSE;
    BOOL    bDrawHBar = FALSE;
    BOOL    bDrawVBar = FALSE;
    HDC     hdc;
    HWND    hwnd;

    ASSERT (wSBflags != SB_CTL);

    wOldFlags = pWState->sbFlags;
    hwnd = pWState->sbHwnd;

    style = GetWindowLong(hwnd, GWL_STYLE);

    switch (wSBflags)   {
    case SB_HORZ:
    case SB_BOTH:
        if (wArrows == ESB_ENABLE_BOTH)
            pWState->sbFlags &= ~WSB_HORZ;
        else
            pWState->sbFlags |= wArrows;

        if (wOldFlags != pWState->sbFlags)
        {
            bRetValue = TRUE;

            if (TestSTYLE(pWState->style, WFHPRESENT)
                && !TestSTYLE(style, WS_MINIMIZE)
                && IsWindowVisible(hwnd))
                bDrawHBar = TRUE;
        }

        if (wSBflags == SB_HORZ)
            break;
        else
            wOldFlags = pWState->sbFlags;       //  Fall through

    case SB_VERT:
        if (wArrows == ESB_ENABLE_BOTH)
            pWState->sbFlags &= ~WSB_VERT;
        else
            pWState->sbFlags |= (wArrows<<2);

        if (wOldFlags != pWState->sbFlags)
        {
            bRetValue = TRUE;

            if (TestSTYLE(pWState->style, WFVPRESENT)
                && !TestSTYLE(style, WS_MINIMIZE)
                && IsWindowVisible(hwnd))
                bDrawVBar = TRUE;
        }
        break;
    default:
        return FALSE;
    }

    if (bDrawVBar || bDrawHBar) {
        int oldLoc = pWState->locMouse;
        int newLoc;

        if (!(hdc = GetWindowDC(hwnd)))
            return(FALSE);

        newLoc = oldLoc;
        if (bDrawHBar)  {
            FlatSB_Internal_DrawScrollBar(pWState, hdc, FALSE, FALSE);
            if (pWState->fHActive)
                newLoc = pWState->locMouse;
        }
        if (bDrawVBar) {
            pWState->locMouse = oldLoc;
            FlatSB_Internal_DrawScrollBar(pWState, hdc, TRUE, FALSE);
            if (pWState->fVActive)
                newLoc = pWState->locMouse;
        }
        pWState->locMouse = newLoc;

        ReleaseDC(hwnd, hdc);
    }

    // Keep USER scrollbar in sync for accessibility
    if (bRetValue)
        EnableScrollBar(hwnd, wSBflags, wArrows);

    return bRetValue;
}

//=-------------------------------------------------------------
// FlatSB_Internal_DrawThumb2
//=-------------------------------------------------------------

void FlatSB_Internal_DrawThumb2(WSBState * pWState, HDC hdc, BOOL fVert, UINT wDisable)
{
    int     *pLength;
    int     *pWidth;
    HWND    hwnd;
    HBRUSH  hbr;

    hwnd = pWState->sbHwnd;
    hbr = (fVert)?pWState->hbr_VSBBkg:pWState->hbr_HSBBkg;

    // Bail out if the scrollbar has an empty rect
    if ((pWState->pxTop >= pWState->pxBottom)
        || (pWState->pxLeft >= pWState->pxRight))
        return;

    pLength = (int *) &(pWState->rcSB);
    if (fVert)
        pWidth = pLength++;
    else
        pWidth = pLength + 1;

    pWidth[0] = pWState->pxLeft;
    pWidth[2] = pWState->pxRight;

    // If both scroll arrows are disabled or if there isn't enough room for
    // the thumb, just erase the whole slide area and return
    if (((wDisable & LTUPFLAG) && (wDisable & RTDNFLAG)) ||
        ((pWState->pxDownArrow - pWState->pxUpArrow) < pWState->cpxThumb))
    {
        pLength[0] = pWState->pxUpArrow;
        pLength[2] = pWState->pxDownArrow;

        FlatSB_Internal_DrawGroove(pWState, hdc, &(pWState->rcSB), fVert);
        return;
    }

    //  UI designers want a at least 1 pixel gap between arrow and thumb.
    //  Have to do this :(
    if (pWState->pxUpArrow <= pWState->pxThumbTop)
    {
        // Fill in space above Thumb
        pLength[0] = pWState->pxUpArrow;
        pLength[2] = pWState->pxThumbTop;

        FlatSB_Internal_DrawGroove(pWState, hdc, &(pWState->rcSB), fVert);
    }

    if (pWState->pxThumbBottom <= pWState->pxDownArrow)
    {
        // Fill in space below Thumb
        pLength[0] = pWState->pxThumbBottom;
        pLength[2] = pWState->pxDownArrow;

        FlatSB_Internal_DrawGroove(pWState, hdc, &(pWState->rcSB), fVert);
    }

    // Draw elevator
    pLength[0] = pWState->pxThumbTop;
    pLength[2] = pWState->pxThumbBottom;

    FlatSB_Internal_DrawElevator(pWState, hdc, &(pWState->rcSB), fVert);

    // If we're tracking a page scroll, then we've obliterated the hilite.
    // We need to correct the hiliting rectangle, and rehilite it.

    if ((pWState->cmdSB == SB_PAGEUP || pWState->cmdSB == SB_PAGEDOWN)
        && pWState->fTrackVert == fVert)
    {
        pLength = (int *) &pWState->rcTrack;

        if (fVert)
            pLength++;

        if (pWState->cmdSB == SB_PAGEUP)
            pLength[2] = pWState->pxThumbTop;
        else
            pLength[0] = pWState->pxThumbBottom;

        if (pLength[0] < pLength[2])
            InvertRect(hdc, &(pWState->rcTrack));
    }
}

//=-------------------------------------------------------------
// DrawSB2
//=-------------------------------------------------------------

void FlatSB_Internal_DrawSB2(WSBState * pWState, HDC hdc, BOOL fVert, BOOL fRedraw, int oldLoc)
{
    int     cLength;
    int     cWidth;
    int     cpxArrow;
    int     *pwX;
    int     *pwY;
    int     newLoc = pWState->locMouse;
    UINT    wDisable = FlatSB_Internal_GetSBFlags(pWState, fVert);
    HBRUSH  hbrSave;
    HWND    hwnd;
    RECT    rc, * prcSB;

    hwnd = pWState->sbHwnd;
    cLength = (pWState->pxBottom - pWState->pxTop) / 2;
    cWidth = (pWState->pxRight - pWState->pxLeft);

    if ((cLength <= 0) || (cWidth <= 0))
        return;

    cpxArrow = (fVert) ? pWState->y_VSBArrow : pWState->x_HSBArrow;

    if (cLength > cpxArrow)
        cLength = cpxArrow;
    prcSB = &(pWState->rcSB);
    pwX = (int *)prcSB;
    pwY = pwX + 1;
    if (!fVert)
        pwX = pwY--;

    pwX[0] = pWState->pxLeft;
    pwY[0] = pWState->pxTop;
    pwX[2] = pWState->pxRight;
    pwY[2] = pWState->pxBottom;

    hbrSave = SelectObject(hdc, GetSysColorBrush(COLOR_BTNTEXT));

    CopyRect(&rc, prcSB);
    if (fVert)
    {
        rc.bottom = rc.top + cLength;
        if (!fRedraw || newLoc == WSB_MOUSELOC_ARROWUP
                     || oldLoc == WSB_MOUSELOC_ARROWUP)
            FlatSB_Internal_DrawArrow(pWState, hdc, &rc, DFCS_SCROLLUP,
               ((wDisable & LTUPFLAG) ? DFCS_INACTIVE : 0));

        rc.bottom = prcSB->bottom;
        rc.top = prcSB->bottom - cLength;
        if (!fRedraw || newLoc == WSB_MOUSELOC_ARROWDN
                     || oldLoc == WSB_MOUSELOC_ARROWDN)
            FlatSB_Internal_DrawArrow(pWState, hdc, &rc, DFCS_SCROLLDOWN,
               ((wDisable & RTDNFLAG) ? DFCS_INACTIVE : 0));
    }
    else
    {
        rc.right = rc.left + cLength;
        if (!fRedraw || newLoc == WSB_MOUSELOC_ARROWLF
                     || oldLoc == WSB_MOUSELOC_ARROWLF)
            FlatSB_Internal_DrawArrow(pWState, hdc, &rc, DFCS_SCROLLLEFT,
                ((wDisable & LTUPFLAG) ? DFCS_INACTIVE : 0));

        rc.right = prcSB->right;
        rc.left = prcSB->right - cLength;
        if (!fRedraw || newLoc == WSB_MOUSELOC_ARROWRG
                     || oldLoc == WSB_MOUSELOC_ARROWRG)
            FlatSB_Internal_DrawArrow(pWState, hdc, &rc, DFCS_SCROLLRIGHT,
                ((wDisable & RTDNFLAG) ? DFCS_INACTIVE : 0));
    }

    SelectObject(hdc, hbrSave);

    if (!fRedraw)
        FlatSB_Internal_DrawThumb2(pWState, hdc, fVert, wDisable);
    else if (!fVert  || newLoc == WSB_MOUSELOC_H_THUMB
                     || oldLoc == WSB_MOUSELOC_H_THUMB)
        FlatSB_Internal_DrawThumb2(pWState, hdc, fVert, wDisable);
    else if (fVert   || newLoc == WSB_MOUSELOC_V_THUMB
                     || oldLoc == WSB_MOUSELOC_V_THUMB)
        FlatSB_Internal_DrawThumb2(pWState, hdc, fVert, wDisable);
    else
        return;
}

//=-------------------------------------------------------------
// FlatSB_Internal_CalcSBStuff2
//=-------------------------------------------------------------

void FlatSB_Internal_CalcSBStuff2(WSBState * pWState, LPRECT lprc, BOOL fVert)
{
    int     cpxThumb;    // Height of (V)scroll bar thumb.
    int     cpxArrow;    // Height of (V)scroll bar arrow.
    int     cpxSpace;    // The space in scroll bar;
    int     pxTop;
    int     pxBottom;
    int     pxLeft;
    int     pxRight;
    int     pxUpArrow;
    int     pxDownArrow;
    int     pxThumbTop;
    int     pxThumbBottom;
    int     pxMouse;
    int     locMouse;
    int     dwRange, page, relPos;
    BOOL    fSBActive;

    if (fVert) {
        pxTop    = lprc->top;
        pxBottom = lprc->bottom;
        pxLeft   = lprc->left;
        pxRight  = lprc->right;
        cpxArrow = pWState->y_VSBArrow;
        cpxThumb = pWState->y_VSBThumb;
        relPos = pWState->sbVThumbPos - pWState->sbVMinPos;
        page = pWState->sbVPage;
        dwRange = pWState->sbVMaxPos - pWState->sbVMinPos + 1;
        pxMouse = pWState->ptMouse.y;
        fSBActive = pWState->fVActive;
    } else {
        // For horiz scroll bars, "left" & "right" are "top" and "bottom",
        // and vice versa.
        pxTop    = lprc->left;
        pxBottom = lprc->right;
        pxLeft   = lprc->top;
        pxRight  = lprc->bottom;
        cpxArrow = pWState->x_HSBArrow;
        cpxThumb = pWState->x_HSBThumb;
        relPos = pWState->sbHThumbPos - pWState->sbHMinPos;
        page = pWState->sbHPage;
        dwRange = pWState->sbHMaxPos - pWState->sbHMinPos + 1;
        pxMouse = pWState->ptMouse.x;
        fSBActive = pWState->fHActive;
    }

    // For the case of short scroll bars that don't have enough
    // room to fit the full-sized up and down arrows, shorten
    // their sizes to make 'em fit

    cpxArrow = min((pxBottom - pxTop) >> 1, cpxArrow);

    pxUpArrow   = pxTop    + cpxArrow;
    pxDownArrow = pxBottom - cpxArrow;

    cpxSpace = pxDownArrow - pxUpArrow;
    if (page)
    {
        // JEFFBOG -- This is the one and only place where we should
        // see 'range'.  Elsewhere it should be 'range - page'.
        cpxThumb = max(DMultDiv(cpxSpace, page, dwRange),
                        min(cpxThumb, MINITHUMBSIZE));
    }
    cpxSpace -= cpxThumb;

    pxThumbTop = DMultDiv(relPos, cpxSpace, dwRange - (page ? page : 1)) + pxUpArrow;
    pxThumbBottom = pxThumbTop + cpxThumb;

    // Save it to local structure
    pWState->pxLeft     = pxLeft;
    pWState->pxRight    = pxRight;
    pWState->pxTop      = pxTop;
    pWState->pxBottom   = pxBottom;
    pWState->pxUpArrow  = pxUpArrow;
    pWState->pxDownArrow    = pxDownArrow;
    pWState->pxThumbTop = pxThumbTop;
    pWState->pxThumbBottom  = pxThumbBottom;
    pWState->cpxArrow   = cpxArrow;
    pWState->cpxThumb   = cpxThumb;
    pWState->cpxSpace   = cpxSpace;
    pWState->fVertSB    = fVert;

    if (pWState->fTracking) {
        return;
    } else if (!fSBActive)  {
        locMouse = WSB_MOUSELOC_OUTSIDE;
    } else if (pxMouse < pxTop) {
        locMouse = WSB_MOUSELOC_OUTSIDE;
    } else if (pxMouse < pxUpArrow) {
        locMouse = WSB_MOUSELOC_ARROWUP;
    } else if (pxMouse < pxThumbTop) {
        locMouse = WSB_MOUSELOC_V_GROOVE;
    } else if (pxMouse >= pxBottom) {
        locMouse = WSB_MOUSELOC_OUTSIDE;
    } else if (pxMouse >= pxDownArrow) {
        locMouse = WSB_MOUSELOC_ARROWDN;
    } else if (pxMouse >= pxThumbBottom) {
        locMouse = WSB_MOUSELOC_V_GROOVE;
    } else    {   //   pxThumbTop <= pxMouse < pxThumbBottom
        if (pxDownArrow - pxUpArrow <= cpxThumb)    {   //  No space for thumnb.
            locMouse = WSB_MOUSELOC_V_GROOVE;
        } else  {
            locMouse = WSB_MOUSELOC_V_THUMB;
        }
    }
    if ((!fVert) && locMouse)
        locMouse += 4;

    pWState->locMouse = locMouse;
}

//=-------------------------------------------------------------
// FlatSB_Internal_CalcSBStuff
//
// Note:
//  We won't call InitPwSB in this func.
//=-------------------------------------------------------------

void FlatSB_Internal_CalcSBStuff(WSBState * pWState, BOOL fVert)
{
    HWND    hwnd;
    RECT    rcT;
    int     style;

    if (pWState == (WSBState *)NULL)
        return;

    hwnd = pWState->sbHwnd;
    style = GetWindowLong(hwnd, GWL_STYLE);

    if (fVert)
    {
        // Only add on space if vertical scrollbar is really there.
        rcT.right = rcT.left = pWState->rcClient.right;
        if (TestSTYLE(pWState->style, WFVPRESENT))
            rcT.right += pWState->x_VSBArrow;
        rcT.top = pWState->rcClient.top;
        rcT.bottom = pWState->rcClient.bottom;
    }
    else
    {
        // Only add on space if horizontal scrollbar is really there.
        rcT.bottom = rcT.top = pWState->rcClient.bottom;
        if (TestSTYLE(pWState->style, WFHPRESENT))
            rcT.bottom += pWState->y_HSBArrow;

        rcT.left = pWState->rcClient.left;
        rcT.right = pWState->rcClient.right;
    }

    FlatSB_Internal_CalcSBStuff2(pWState, &rcT, fVert);
}

//=-------------------------------------------------------------
// FlatSB_Internal_DrawThumb
//=-------------------------------------------------------------

void FlatSB_Internal_DrawThumb(WSBState * pWState, BOOL fVert)
{
    HWND    hwnd = pWState->sbHwnd;
    HDC     hdc;
    UINT    wDisableFlags;

    hdc = (HDC) GetWindowDC(hwnd);
    FlatSB_Internal_CalcSBStuff(pWState, fVert);

    wDisableFlags = FlatSB_Internal_GetSBFlags(pWState, fVert);
    FlatSB_Internal_DrawThumb2(pWState, hdc, fVert, wDisableFlags);
    ReleaseDC(hwnd, hdc);
}

BOOL FlatSB_Internal_SBSetParms(int * pw, SCROLLINFO si, BOOL * lpfScroll, LRESULT * lplres, BOOL bOldPos)
{
    // pass the struct because we modify the struct but don't want that
    // modified version to get back to the calling app

    BOOL fChanged = FALSE;

    if (bOldPos)
        // save previous position
        *lplres = pw[SBO_POS];

    if (si.fMask & SIF_RANGE)
    {
        // if the range MAX is below the range MIN -- then treat is as a
        // zero range starting at the range MIN.
        if (si.nMax < si.nMin)
            si.nMax = si.nMin;

        if ((pw[SBO_MIN] != si.nMin) || (pw[SBO_MAX] != si.nMax))
        {
            pw[SBO_MIN] = si.nMin;
            pw[SBO_MAX] = si.nMax;

            if (!(si.fMask & SIF_PAGE))
            {
                si.fMask |= SIF_PAGE;
                si.nPage = pw[SBO_PAGE];
            }

            if (!(si.fMask & SIF_POS))
            {
                si.fMask |= SIF_POS;
                si.nPos = pw[SBO_POS];
            }

            fChanged = TRUE;
        }
    }

    if (si.fMask & SIF_PAGE)
    {
        unsigned dwMaxPage = abs(pw[SBO_MAX] - pw[SBO_MIN]) + 1;

        if (si.nPage > dwMaxPage)
            si.nPage = dwMaxPage;

        if (pw[SBO_PAGE] != (int) si.nPage)
        {
            pw[SBO_PAGE] = (int) si.nPage;

            if (!(si.fMask & SIF_POS))
            {
                si.fMask |= SIF_POS;
                si.nPos = pw[SBO_POS];
            }

            fChanged = TRUE;
        }
    }

    if (si.fMask & SIF_POS)
    {
        // Clip pos to posMin, posMax - (page - 1).
        int lMaxPos = pw[SBO_MAX] - ((pw[SBO_PAGE]) ? pw[SBO_PAGE] - 1 : 0);

        // * BOGUS -- show this to SIMONK -- the following doesn't generate *
        // * proper code so I had to use the longer form                    *
        // * si.nPos = min(max(si.nPos, pw[SBO_MIN]), lMaxPos);             *

        if (si.nPos < pw[SBO_MIN])
            si.nPos = pw[SBO_MIN];
        else if (si.nPos > lMaxPos)
            si.nPos = lMaxPos;

        if (pw[SBO_POS] != si.nPos)
        {
            pw[SBO_POS] = si.nPos;
            fChanged = TRUE;
        }
    }

    if (!(bOldPos))
        // Return the new position
        *lplres = pw[SBO_POS];

    if (si.fMask & SIF_RANGE)
    {
        if (*lpfScroll = (pw[SBO_MIN] != pw[SBO_MAX]))
            goto checkPage;
    }
    else if (si.fMask & SIF_PAGE)
checkPage:
        *lpfScroll = (pw[SBO_PAGE] <= (pw[SBO_MAX] - pw[SBO_MIN]));

    return(fChanged);
}


//=-------------------------------------------------------------
// FlatSB_Internal_SetScrollBar
//
// Note:
//     This func is called by SetScrollPos/Range/Info. We let
//     the callers take care of checking pWState.
//     Return 0 if failed.
//=-------------------------------------------------------------

LRESULT FlatSB_Internal_SetScrollBar(WSBState *pWState, int code, LPSCROLLINFO lpsi, BOOL fRedraw)
{
    BOOL    fVert;
    int     *pw;
    BOOL    fOldScroll;
    BOOL    fScroll;
    BOOL    bReturnOldPos = TRUE;
    LRESULT lres;
    int     wfScroll;
    HWND    hwnd = pWState->sbHwnd;

    ASSERT (code != SB_CTL);

    // window must be visible to redraw
    if (fRedraw)
        fRedraw = IsWindowVisible(hwnd);

    fVert = (code != SB_HORZ);
    bReturnOldPos = (lpsi->fMask == SIF_POS);

    wfScroll = (fVert) ? WS_VSCROLL : WS_HSCROLL;

    fScroll = fOldScroll = (TestSTYLE(pWState->style, wfScroll)) ? TRUE : FALSE;

    // Don't do anything if we're NOT setting the range and the scroll doesn't
    // exist.
    if (!(lpsi->fMask & SIF_RANGE) && !fOldScroll)
    {
        return(0);
    }

    pw = &(pWState->sbFlags);

    // user.h: SBO_VERT = 5, SBO_HORZ = 1;
    //  pw += (fVert) ? SBO_VERT : SBO_HORZ;
    pw += (fVert)? 5 : 1;

    // Keep USER scrollbars in sync for accessibility
    SetScrollInfo(hwnd, code, lpsi, FALSE);

    if (!FlatSB_Internal_SBSetParms(pw, *lpsi, &fScroll, &lres, bReturnOldPos))
    {
        // no change -- but if REDRAW is specified and there's a scrollbar,
        // redraw the thumb
        if (fOldScroll && fRedraw)
            goto redrawAfterSet;

        return(lres);
    }

    if (fScroll)
        pWState->style |= wfScroll;
    else
        pWState->style &= ~wfScroll;

    // Keep style bits in sync so OLEACC can read them
    SetWindowBits(hwnd, GWL_STYLE, WS_VSCROLL | WS_HSCROLL, pWState->style);

    if (lpsi->fMask & SIF_DISABLENOSCROLL)
    {
        if (fOldScroll)
        {
            pWState->style |= wfScroll;

            // Keep style bits in sync so OLEACC can read them
            SetWindowBits(hwnd, GWL_STYLE, WS_VSCROLL | WS_HSCROLL, pWState->style);

            FlatSB_Internal_EnableScrollBar(pWState, code, (fScroll) ? ESB_ENABLE_BOTH : ESB_DISABLE_BOTH);
        }
    }
    else if (fOldScroll ^ fScroll)
    {
        CCInvalidateFrame(hwnd);
        return(lres);
    }

    if (fScroll && fRedraw && (fVert ? TestSTYLE(pWState->style, WFVPRESENT) : TestSTYLE(pWState->style, WFHPRESENT)))
    {
redrawAfterSet:

        // Don't send this, since USER already sent one for us when we
        // called SetScrollBar.
        // FlatSB_Internal_NotifyWinEvent(pWState, EVENT_OBJECT_VALUECHANGE, INDEX_SCROLLBAR_SELF);

        // Bail out if the caller is trying to change a scrollbar which is
        // in the middle of tracking.  We'll hose FlatSB_Internal_TrackThumb() otherwise.

        // BUGBUG: CalcSBStuff will change locMouse!
        if (pWState->pfnSB == FlatSB_Internal_TrackThumb)
        {
            FlatSB_Internal_CalcSBStuff(pWState, fVert);
            return(lres);
        }
        FlatSB_Internal_DrawThumb(pWState, fVert);
    }

    return(lres);
}

//=-------------------------------------------------------------
//  SetScrollPos()
//=-------------------------------------------------------------

int WINAPI FlatSB_SetScrollPos(HWND hwnd, int code, int pos, BOOL fRedraw)
{
    SCROLLINFO  si;
    WSBState * pWState;

    GetWindowSubclass(hwnd, FlatSB_SubclassWndProc, 0, (ULONG_PTR *)&pWState);
    if (pWState == (WSBState *)NULL)    {
        return SetScrollPos(hwnd, code, pos, fRedraw);
    } else if (pWState == WSB_UNINIT_HANDLE) {
        return 0;
    } else if (hwnd != pWState->sbHwnd) {
        return 0;
    }

    si.cbSize = sizeof(si);
    si.fMask = SIF_POS;
    si.nPos = pos;
    
    return (int)FlatSB_Internal_SetScrollBar(pWState, code, &si, fRedraw);
}

//=-------------------------------------------------------------
//  SetScrollRange()
//=-------------------------------------------------------------

BOOL WINAPI FlatSB_SetScrollRange(HWND hwnd, int code, int nMin, int nMax, BOOL fRedraw)
{
    SCROLLINFO si;
    WSBState * pWState;

    GetWindowSubclass(hwnd, FlatSB_SubclassWndProc, 0, (ULONG_PTR *)&pWState);
    if (pWState == (WSBState *)NULL)    {
        return SetScrollRange(hwnd, code, nMin, nMax, fRedraw);
    } else if (pWState == WSB_UNINIT_HANDLE) {
        pWState = FlatSB_Internal_InitPwSB(hwnd);
        if (pWState == (WSBState *)NULL)
            return FALSE;
        else if (!SetWindowSubclass(hwnd, FlatSB_SubclassWndProc, 0,  (ULONG_PTR)pWState))  {
            DeleteObject(pWState->hbm_Bkg);
            DeleteObject(pWState->hbr_Bkg);
            LocalFree((HLOCAL)pWState);
            return FALSE;
        }
        //  In this case we always need to (re)draw the scrollbar.
        fRedraw = TRUE;
    } else if (hwnd != pWState->sbHwnd) {
        return FALSE;
    }


    //
    // Still need MAXINT check for stupid PackRat 4.  32-bit apps don't
    // go thru this--we wrap 'em to SetScrollInfo() on the 32-bit side,
    // so DWORD precision is preserved.
    //
    if ((UINT)(nMax - nMin) > 0x7FFF)
        return FALSE;

    si.cbSize = sizeof(si);
    si.fMask = SIF_RANGE;
    si.nMin = nMin;
    si.nMax = nMax;

    FlatSB_Internal_SetScrollBar(pWState, code, &si, fRedraw);

    return(TRUE);
}


//=-------------------------------------------------------------
//  SetScrollInfo()
//
//  Note:
//      Inconsistent with 'user' code. Under no circumstance will
//      we create a new scrollbar(by allocate a new buffer).
//=-------------------------------------------------------------

int WINAPI FlatSB_SetScrollInfo(HWND hwnd, int code, LPSCROLLINFO lpsi, BOOL fRedraw)
{
    WSBState * pWState;

    //  ZDC@Oct. 10, Detect GP faults here.
    if ((LPSCROLLINFO)NULL == lpsi)
        return FALSE;

    if (lpsi->cbSize < sizeof (SCROLLINFO))
        return FALSE;

    GetWindowSubclass(hwnd, FlatSB_SubclassWndProc, 0, (ULONG_PTR *)&pWState);
    if (pWState == (WSBState *)NULL)    {
        return SetScrollInfo(hwnd, code, lpsi, fRedraw);
    } else if (pWState == WSB_UNINIT_HANDLE) {
        if (!(lpsi->fMask & SIF_RANGE))
            return 0;
        pWState = FlatSB_Internal_InitPwSB(hwnd);
        if (pWState == (WSBState *)NULL)
            return 0;
        else if (!SetWindowSubclass(hwnd, FlatSB_SubclassWndProc, 0,  (ULONG_PTR)pWState)) {
            DeleteObject(pWState->hbm_Bkg);
            DeleteObject(pWState->hbr_Bkg);
            LocalFree((HLOCAL)pWState);
            return 0;
        }

        //  In this case we always need to (re)draw the scrollbar.
        fRedraw = TRUE;
    } else if (hwnd != pWState->sbHwnd) {
        return 0;
    }

    //  ZDC@Oct 9, We should always return new pos. How ever, if the fMask
    //  is SIF_POS, SetScrollBar returns the old pos.
    if (lpsi->fMask == SIF_POS)
        lpsi->fMask = SIF_POS | SIF_TRACKPOS;

    return (int)FlatSB_Internal_SetScrollBar(pWState, code, lpsi, fRedraw);
}

//=-------------------------------------------------------------
// FlatSB_SetScrollProp
//     This functions shouldn't be called we we are tracking.
//=-------------------------------------------------------------

BOOL WINAPI FlatSB_SetScrollProp(HWND hwnd, UINT index, INT_PTR newValue, BOOL fRedraw)
{
    BOOL    fResize = FALSE;
    BOOL    fVert = FALSE;
    WSBState * pWState;

    GetWindowSubclass(hwnd, FlatSB_SubclassWndProc, 0, (ULONG_PTR *)&pWState);
    if (pWState == (WSBState *)NULL)
        return FALSE;
    else if (pWState == WSB_UNINIT_HANDLE)  {
        pWState = FlatSB_Internal_InitPwSB(hwnd);
        if (pWState == (WSBState *)NULL)
            return 0;
        else if (!SetWindowSubclass(hwnd, FlatSB_SubclassWndProc, 0,  (ULONG_PTR)pWState)) {
            DeleteObject(pWState->hbm_Bkg);
            DeleteObject(pWState->hbr_Bkg);
            LocalFree((HLOCAL)pWState);
            return 0;
        }

        //  In this case we don't want to (re)draw the scrollbar.
        fRedraw = FALSE;
    }

    if (pWState->fTracking)
        return FALSE;

    switch (index) {
    case WSB_PROP_CXVSCROLL:
        if ((int)newValue == pWState->metApp.cxVSBArrow)
            return TRUE;
        pWState->metApp.cxVSBArrow = (int)newValue;
        fResize = TRUE;
        break;

    case WSB_PROP_CXHSCROLL:
        if ((int)newValue == pWState->metApp.cxHSBArrow)
            return TRUE;
        pWState->metApp.cxHSBArrow = (int)newValue;
        fResize = TRUE;
        break;

    case WSB_PROP_CYVSCROLL:
        if ((int)newValue == pWState->metApp.cyVSBArrow)
            return TRUE;
        pWState->metApp.cyVSBArrow = (int)newValue;
        fResize = TRUE;
        break;

    case WSB_PROP_CYHSCROLL:
        if ((int)newValue == pWState->metApp.cyHSBArrow)
            return TRUE;
        pWState->metApp.cyHSBArrow = (int)newValue;
        fResize = TRUE;
        break;

    case WSB_PROP_CXHTHUMB:
        if ((int)newValue == pWState->metApp.cxHSBThumb)
            return TRUE;
        pWState->metApp.cxHSBThumb = (int)newValue;
        fResize = TRUE;
        break;

    case WSB_PROP_CYVTHUMB:
        if ((int)newValue == pWState->metApp.cyVSBThumb)
            return TRUE;
        pWState->metApp.cyVSBThumb = (int)newValue;
        fResize = TRUE;
        break;

    case WSB_PROP_VBKGCOLOR:
        if ((COLORREF)newValue == pWState->col_VSBBkg)
            return TRUE;
        pWState->col_VSBBkg = (COLORREF)newValue;
        fVert = TRUE;
        break;
    case WSB_PROP_HBKGCOLOR:
        if ((COLORREF)newValue == pWState->col_HSBBkg)
            return TRUE;
        pWState->col_HSBBkg = (COLORREF)newValue;
        break;

    case WSB_PROP_PALETTE:
        if ((HPALETTE)newValue == pWState->hPalette)
            return TRUE;
        pWState->hPalette = (HPALETTE)newValue;
        break;
    case WSB_PROP_VSTYLE:
        if ((int)newValue == pWState->vStyle)
            return TRUE;
        pWState->vStyle = (int)newValue;
        fVert = TRUE;
        break;
    case WSB_PROP_HSTYLE:
        if ((int)newValue == pWState->hStyle)
            return TRUE;
        pWState->hStyle = (int)newValue;
        break;
    case WSB_PROP_GUTTER:
        if ((int)newValue == pWState->sbGutter)
            return TRUE;
        pWState->sbGutter = (int)newValue;
        break;

    default:
        return FALSE;
    }

    if (fResize)    {
    // Always redraw after we change the size.
        CCInvalidateFrame(hwnd);
    } else if (fRedraw) {
        HDC hdc;
        int oldLoc = pWState->locMouse;
        int fSBActive = (fVert)?pWState->fVActive:pWState->fHActive;

        hdc = GetWindowDC(hwnd);
        FlatSB_Internal_DrawScrollBar(pWState, hdc, fVert, FALSE /* Not redraw*/);
        if (!fSBActive)
            pWState->locMouse = oldLoc;
        ReleaseDC(hwnd, hdc);
    }
    return TRUE;
}

//=-------------------------------------------------------------
//  FlatSB_Internal_DrawScrollBar()
//=-------------------------------------------------------------

void FlatSB_Internal_DrawScrollBar(WSBState * pWState, HDC hdc, BOOL fVert, BOOL fRedraw)
{
    int oldLoc = pWState->locMouse;

    FlatSB_Internal_CalcSBStuff(pWState, fVert);
    if ((!fRedraw) || oldLoc != pWState->locMouse)
        FlatSB_Internal_DrawSB2(pWState, hdc, fVert, fRedraw, oldLoc);
}

//=------------------------------------------------------------
//  FlatSB_Internal_IsSizeBox
//      It's still an incomplete mimic of SizeBoxWnd in user/winwhere.c
//=------------------------------------------------------------

BOOL FlatSB_Internal_IsSizeBox(HWND hwndStart)
{
    int style;
    HWND hwnd, hwndDesktop;
    int cxEdge, cyEdge;
    RECT rcChild, rcParent;

    ASSERT(hwndStart);
    hwnd = hwndStart;
    hwndDesktop = GetDesktopWindow();

    cxEdge = GetSystemMetrics(SM_CXEDGE);
    cyEdge = GetSystemMetrics(SM_CYEDGE);
    if (!GetWindowRect(hwnd, &rcChild))
        return FALSE;
    do  {
        style = GetWindowStyle(hwnd);
        if (TestSTYLE(style, WS_SIZEBOX))   {
            if (IsZoomed(hwnd))
                return FALSE;
            else    {
                POINT pt;

                GetClientRect(hwnd, &rcParent);

                pt.x = rcParent.right;
                pt.y = rcParent.bottom;

                ClientToScreen(hwnd, &pt);

                if (rcChild.right + cxEdge < pt.x)
                    return FALSE;
                if (rcChild.bottom + cyEdge < pt.y)
                    return FALSE;
                return TRUE;
            }
        } else  {
            hwnd = GetParent(hwnd);
        }
    }

    while ((hwnd) && (hwnd != hwndDesktop));
    return FALSE;
}
