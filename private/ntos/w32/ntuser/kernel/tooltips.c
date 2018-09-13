/****************************** Module Header ******************************\
* Module Name: tooltips.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Implements system tooltips.
*
* History:
* 25-Aug-1996 vadimg    created
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

#define TT_XOFFSET          2
#define TT_YOFFSET          1
#define TTT_SHOW            1
#define TTT_HIDE            2
#define TTT_ANIMATE         3
#define TT_ANIMATEDELAY     20

#define TTF_POSITIVE        0x00000001

#define bitsizeof(x) (sizeof(x) * 8)

UINT MNItemHitTest(PMENU, PWND, POINT);
HANDLE NtGdiGetDCObject(HDC, int);
LONG GreGetBitmapBits(HBITMAP, ULONG, PBYTE, PLONG);
DWORD CalcCaptionButton(PWND pwnd, int hit, LPWORD pcmd, LPRECT prcBtn, LPWORD pbm);
int HitTestScrollBar(PWND pwnd, int ht, POINT pt);
BOOL xxxHotTrackSB(PWND pwnd, int htEx, BOOL fDraw);

__inline void ZeroTooltip(PTOOLTIPWND pttwnd)
{
    RtlZeroMemory((PBYTE)pttwnd + (sizeof(TOOLTIPWND) - sizeof(TOOLTIP)),
            sizeof(TOOLTIP));
}

/***************************************************************************\
* GetTooltipDC
*
* 2/3/1998   vadimg          created
\***************************************************************************/

HDC GetTooltipDC(PTOOLTIPWND pttwnd)
{
    HDC hdc = _GetDCEx((PWND)pttwnd, NULL, DCX_WINDOW | DCX_CACHE |
            DCX_USESTYLE);

    if (hdc == NULL)
        return NULL;

    GreSelectFont(hdc, ghStatusFont);
    return hdc;
}

/***************************************************************************\
* InitTooltipAnimation
*
* Creates memory bitmap and DC for use by system tooltips. Gets the screen
* DC used throughout.
*
* 12-Sep-96 vadimg      created
\***************************************************************************/

void InitTooltipAnimation(PTOOLTIPWND pttwnd)
{
    HDC hdc = GetTooltipDC(pttwnd);

    if ((pttwnd->hdcMem = GreCreateCompatibleDC(hdc)) == NULL) {
        return;
    }
    _ReleaseDC(hdc);
    GreSetDCOwner(pttwnd->hdcMem, OBJECT_OWNER_PUBLIC);
}

/***************************************************************************\
* DestroyTooltipBitmap
*
\***************************************************************************/

void DestroyTooltipBitmap(PTOOLTIPWND pttwnd)
{
    if (pttwnd->hbmMem == NULL)
        return;

    GreSelectBitmap(pttwnd->hdcMem, GreGetStockObject(PRIV_STOCK_BITMAP));
    GreDeleteObject(pttwnd->hbmMem);
    pttwnd->hbmMem = NULL;
}

/***************************************************************************\
* CreateTooltipBitmap
*
\***************************************************************************/

BOOL CreateTooltipBitmap(PTOOLTIPWND pttwnd, UINT cx, UINT cy)
{
    HDC hdc;

    if (pttwnd->hdcMem == NULL) {
        RIPMSG0(RIP_WARNING, "CreateTooltipBitmap: pttwnd->hdcMem is NULL");
        return FALSE;
    }

    DestroyTooltipBitmap(pttwnd);

    hdc = GetTooltipDC(pttwnd);
    pttwnd->hbmMem = GreCreateCompatibleBitmap(hdc, cx, cy);
    _ReleaseDC(hdc);

    if (pttwnd->hbmMem == NULL) {
        RIPMSG0(RIP_WARNING, "CreateTooltipBitmap: hbmMem is NULL");
        return FALSE;
    }
    GreSelectBitmap(pttwnd->hdcMem, pttwnd->hbmMem);
    return TRUE;
}

/***************************************************************************\
* CleanupTooltipAnimation
*
* Deletes memory bitmap and DC for use by system tooltips. Release the
* screen DC.
*
* 12-Sep-96 vadimg      created
\***************************************************************************/

void CleanupTooltipAnimation(PTOOLTIPWND pttwnd)
{
    DestroyTooltipBitmap(pttwnd);

    if (pttwnd->hdcMem != NULL) {
        GreSetDCOwner(pttwnd->hdcMem, OBJECT_OWNER_CURRENT);
        GreDeleteDC(pttwnd->hdcMem);
    }
}

/***************************************************************************\
* TooltipAnimate
*
* Perform one frame of window animation. Just a simplified version of
* the AnimateWindow API.
*
* 12-Sep-96 vadimg      created
\***************************************************************************/

BOOL TooltipAnimate(PTOOLTIPWND pttwnd)
{
    int y, yMem, yReal, ny, iy, cx, cy;
    DWORD dwElapsed;
    HDC hdc;
    BOOL fRet = FALSE;

    if (pttwnd->pstr == NULL)
        return TRUE;

    hdc = GetTooltipDC(pttwnd);
    cx = pttwnd->rcWindow.right - pttwnd->rcWindow.left;
    cy = pttwnd->rcWindow.bottom - pttwnd->rcWindow.top;
    dwElapsed = NtGetTickCount() - pttwnd->dwAnimStart;
    iy = MultDiv(cy, dwElapsed, CMS_TOOLTIP);

    if (dwElapsed > CMS_TOOLTIP || iy == cy) {
        GreBitBlt(hdc, 0, 0, cx, cy, pttwnd->hdcMem, 0, 0, SRCCOPY | NOMIRRORBITMAP, 0);
        fRet = TRUE;
        goto Cleanup;
    } else if (pttwnd->iyAnim == iy) {
        goto Cleanup;
    }

    if (pttwnd->dwFlags & TTF_POSITIVE) {
        y = 0;
        ny = 0;
    } else {
        y = cy;
        ny = -1;
    }

    yReal = y + ny * iy;
    yMem = (pttwnd->dwFlags & TTF_POSITIVE) ? cy - iy : 0;
    pttwnd->iyAnim = iy;

    GreBitBlt(hdc, 0, yReal, cx, iy, pttwnd->hdcMem, 0, yMem, SRCCOPY | NOMIRRORBITMAP, 0);

Cleanup:
    _ReleaseDC(hdc);
    return fRet;
}

/***************************************************************************\
* GetCursorHeight
*
* This is tricky. We need to get the actual cursor size from the hotspot
* down to the end. There is no API in windows to do this, CYCURSOR is
* just the metric for the size of the bitmap, the cursor starts at the top
* of the bitmap and may be smaller than the bitmap itself.
*
* 12-Sep-96 vadimg      ported from common controls
\***************************************************************************/

int GetCursorHeight(void)
{
    int iAnd, iXor, dy = 16;
    WORD wMask[128];
    ICONINFO ii;
    BITMAP bm;
    PCURSOR pcur;
    long lOffset = 0;

    if ((pcur = PtiCurrent()->pq->spcurCurrent) == NULL) {
        return dy;
    }

    if (!_InternalGetIconInfo(pcur, &ii, NULL, NULL, NULL, FALSE)) {
        return dy;
    }

    if (!GreExtGetObjectW(ii.hbmMask, sizeof(bm), (LPSTR)&bm)) {
        goto Bail;
    }

    /*
     * Use the AND mask to get the cursor height if the XOR mask is there.
     */
    if (!GreGetBitmapBits(ii.hbmMask, sizeof(wMask), (BYTE*)wMask, &lOffset)) {
        goto Bail;
    }

    iAnd = (int)(bm.bmWidth * bm.bmHeight / bitsizeof(WORD));

    if (ii.hbmColor == NULL) {
        /*
         * if no color (XOR) bitmap, then the hbmMask is a double height bitmap
         * with the cursor and the mask stacked.
         */

        iXor = iAnd - 1;
        iAnd /= 2;
    } else {
        iXor = 0;
    }

    if (iAnd >= sizeof(wMask)) {
        iAnd = sizeof(wMask) - 1;
    }

    if (iXor >= sizeof(wMask)) {
        iXor = 0;
    }

    for (iAnd--; iAnd >= 0; iAnd--) {
        if ((iXor != 0 && wMask[iXor--] != 0) || wMask[iAnd] != 0xFFFF) {
            break;
        }
    }

    /*
     * Compute the distance between the pointer's lowest point and hotspot.
     */
    dy = (iAnd + 1) * bitsizeof(WORD) / (int)bm.bmWidth - (int)ii.yHotspot;

Bail:
    if (ii.hbmColor) {
        GreDeleteObject(ii.hbmColor);
    }

    if (ii.hbmMask) {
        GreDeleteObject(ii.hbmMask);
    }

    return dy;
}

/***************************************************************************\
* TooltipGetPosition
*
* Get the tooltip position on the screen taking into account the size of
* the tooltip and the screen. The TTF_POSITIVE flag determines if positive
* or negative animation is used.
*
* 12-Sep-96 vadimg      created
\***************************************************************************/

void TooltipGetPosition(PTOOLTIPWND pttwnd, SIZE *psize, POINT *ppt)
{
    PMONITOR    pMonitor;

    *ppt = gpsi->ptCursor;
    pMonitor = _MonitorFromPoint(*ppt, MONITOR_DEFAULTTONULL);
    UserAssert(pMonitor);

    if (ppt->y + psize->cy >= pMonitor->rcMonitor.bottom) {
        ppt->y = ppt->y - psize->cy;
        pttwnd->dwFlags &= ~TTF_POSITIVE;
    } else {
        ppt->y += GetCursorHeight();
        pttwnd->dwFlags |= TTF_POSITIVE;
    }

    if (ppt->x + psize->cx >= pMonitor->rcMonitor.right) {
        ppt->x = pMonitor->rcMonitor.right - psize->cx;
    }

    if (ppt->x < pMonitor->rcMonitor.left) {
        ppt->x = pMonitor->rcMonitor.left;
    }
}

/***************************************************************************\
* TooltipGetSize
*
* Estimate the size of the tooltip window based on the size of the text.
*
* 12-Sep-96 vadimg      created
\***************************************************************************/

void TooltipGetSize(PTOOLTIPWND pttwnd, SIZE *psize)
{
    HDC hdc = GetTooltipDC(pttwnd);
    GreGetTextExtentW(hdc, pttwnd->pstr, wcslen(pttwnd->pstr),
            psize, GGTE_WIN3_EXTENT);
    _ReleaseDC(hdc);
    psize->cx += SYSMET(CXEDGE) + 2 * SYSMET(CXBORDER) * TT_XOFFSET;
    psize->cy += SYSMET(CYEDGE) + 2 * SYSMET(CYBORDER) * TT_YOFFSET;
}

/***************************************************************************\
* TooltipRender
*
* Render the tooltip window into the provided DC.
*
* 12-Sep-96 vadimg      created
\***************************************************************************/

void TooltipRender(PTOOLTIPWND pttwnd, HDC hdc)
{
    COLORREF crBk;
    UINT uFlags;
    RECT rc;

    if (pttwnd->pstr == NULL)
        return;

    GreSelectFont(hdc, ghStatusFont);
    GreSetTextColor(hdc, gpsi->argbSystem[COLOR_INFOTEXT]);
    crBk = gpsi->argbSystem[COLOR_INFOBK];

    CopyOffsetRect(&rc, &pttwnd->rcClient, -pttwnd->rcClient.left,
           -pttwnd->rcClient.top);

    /*
     * We don't want dithered colors, so FillRect with the nearest color.
     */
    if (crBk == GreGetNearestColor(hdc, crBk)) {
        GreSetBkColor(hdc, crBk);
        uFlags = ETO_OPAQUE;
    } else {
        FillRect(hdc, &rc, SYSHBR(INFOBK));
        GreSetBkMode(hdc, TRANSPARENT);
        uFlags = ETO_CLIPPED;
    }

    GreExtTextOutW(hdc, SYSMET(CXBORDER) * TT_XOFFSET,
            SYSMET(CYBORDER) * TT_YOFFSET, uFlags, &rc, pttwnd->pstr,
            wcslen(pttwnd->pstr), NULL);
}

/***************************************************************************\
* FindNcHitEx
*
* 12-Sep-96 vadimg      created
\***************************************************************************/

int FindNCHitEx(PWND pwnd, int ht, POINT pt)
{
    /*
     * Bug 263057 joejo
     * It seems that pwnd->spmenu can be released and set to null,
     * without the WFMPRESENT flag being cleared. Make sure that
     * we have a good pwnd->spmenu before continuing.
     */
    if (ht == HTMENU && pwnd->spmenu && TestWF(pwnd, WFMPRESENT)) {
        PMENU spmenu = pwnd->spmenu;
        PITEM pitem;
        int nItem;

        nItem = MNItemHitTest(spmenu, pwnd, pt);
        if (nItem >= 0) {
            pitem = (PITEM)&spmenu->rgItems[nItem];
            switch ((ULONG_PTR)pitem->hbmp) {
            case (ULONG_PTR)HBMMENU_SYSTEM:
                ht = HTMDISYSMENU;
                break;
            case (ULONG_PTR)HBMMENU_MBAR_RESTORE:
                ht = HTMDIMAXBUTTON;
                break;
            case (ULONG_PTR)HBMMENU_MBAR_MINIMIZE:
            case (ULONG_PTR)HBMMENU_MBAR_MINIMIZE_D:
                ht = HTMDIMINBUTTON;
                break;
            case (ULONG_PTR)HBMMENU_MBAR_CLOSE:
            case (ULONG_PTR)HBMMENU_MBAR_CLOSE_D:
                ht = HTMDICLOSE;
                break;
            case (ULONG_PTR)HBMMENU_CALLBACK:
                ht = HTERROR;
                break;
            default:
                ht = HTMENUITEM;
                break;
            }
        }
        return MAKELONG(ht, nItem);
    } else if (ht == HTVSCROLL && TestWF(pwnd, WFVPRESENT)) {
        return MAKELONG(HitTestScrollBar(pwnd, TRUE, pt), 1);
    } else if (ht == HTHSCROLL && TestWF(pwnd, WFHPRESENT)) {
        return MAKELONG(HitTestScrollBar(pwnd, FALSE, pt), 0);
    }

    return ht;
}

/***************************************************************************\
* KillTooltipTimer
*
* Kill the timer and zero out the timer id.
\***************************************************************************/
void KillTooltipTimer (PTOOLTIPWND pttwnd)
{
    UINT uTID = pttwnd->uTID;
    if (uTID != 0) {
        pttwnd->uTID = 0;
        _KillTimer((PWND)pttwnd, uTID);
    }
}
/***************************************************************************\
* SetTooltipTimer
*
\***************************************************************************/
void SetTooltipTimer (PTOOLTIPWND pttwnd, UINT uTID, UINT uDelay)
{
    KillTooltipTimer(pttwnd);
    pttwnd->uTID = uTID;
    InternalSetTimer((PWND)pttwnd, uTID, uDelay, NULL, 0);
}
/***************************************************************************\
* xxxResetTooltip
*
* Hide the tooltip, kill the timer, and zero out most of the struct members.
\***************************************************************************/

void xxxResetTooltip(PTOOLTIPWND pttwnd)
{
    KillTooltipTimer(pttwnd);

    CheckLock(pttwnd);

    if (TestWF(pttwnd, WFVISIBLE)) {
        PWND spwndMessage;
        TL tlpwnd;

        xxxSetWindowPos((PWND)pttwnd, NULL, 0, 0, 0, 0, SWP_NOSIZE |
                SWP_NOMOVE | SWP_HIDEWINDOW | SWP_NOACTIVATE | SWP_NOZORDER);

        spwndMessage = PWNDMESSAGE(pttwnd);
        ThreadLockAlways(spwndMessage, &tlpwnd);
        xxxSetParent((PWND)pttwnd, spwndMessage);
        ThreadUnlock(&tlpwnd);
    }

    ZeroTooltip(pttwnd);
    pttwnd->head.rpdesk->dwDTFlags &= ~DF_TOOLTIP;
}

/***************************************************************************\
* xxxShowTooltip
*
* Show the tooltip window.
*
* 12-Sep-96 vadimg      created
\***************************************************************************/

void xxxShowTooltip(PTOOLTIPWND pttwnd)
{
    SIZE size;
    POINT pt;
    DWORD dwFlags;

    CheckLock(pttwnd);

    if (pttwnd->pstr == NULL)
        return;

    if (pttwnd->pstr == gszCAPTIONTOOLTIP) {

        PWND pwnd = PtiCurrent()->rpdesk->spwndTrack;
        /*
         * The window text might have changed in callbacks, retrieve it now
         */
        if (TestWF(pwnd, WEFTRUNCATEDCAPTION) && pwnd->strName.Length) {
            wcsncpycch(gszCAPTIONTOOLTIP, pwnd->strName.Buffer, CAPTIONTOOLTIPLEN-1);
            gszCAPTIONTOOLTIP[CAPTIONTOOLTIPLEN-1] = 0;
        } else {
            return;
        }
    }

    TooltipGetSize(pttwnd, &size);
    TooltipGetPosition(pttwnd, &size, &pt);

    dwFlags = SWP_CREATESPB | SWP_SHOWWINDOW | SWP_NOACTIVATE;
    if (TestEffectUP(TOOLTIPANIMATION)) {
        dwFlags |= SWP_NOREDRAW;
    }
    xxxSetWindowPos((PWND)pttwnd, PWND_TOP, pt.x, pt.y, size.cx, size.cy,
            dwFlags);
}

/***************************************************************************\
* xxxTooltipHandleTimer
*
* 12-Sep-96 vadimg      created
\***************************************************************************/

void xxxTooltipHandleTimer(PTOOLTIPWND pttwnd, UINT uTID)
{

    switch(uTID) {
        case TTT_SHOW: {
            /*
             * Move the tooltip window to the desktop so it can
             *  be shown. Then show it.
             */
            PWND pwndDesktop = PWNDDESKTOP(pttwnd);
            TL tlpwnd;
            ThreadLockAlways(pwndDesktop, &tlpwnd);
            xxxSetParent((PWND)pttwnd, pwndDesktop);
            ThreadUnlock(&tlpwnd);

            xxxShowTooltip(pttwnd);
            break;
        }

        case TTT_ANIMATE:
           /*
            * If animation is completed, set timer to hide
            */
           if (TooltipAnimate(pttwnd)) {
               SetTooltipTimer(pttwnd, TTT_HIDE, pttwnd->dwHideDelay);
           }
           break;

        case TTT_HIDE:
           /*
            * Hide it
            */
           xxxResetTooltip(pttwnd);
           break;
    }
}
/***************************************************************************\
* xxxTooltipWndProc
*
* The actual WndProc for the tooltip window.
*
* 12-Sep-96 vadimg      created
\***************************************************************************/

LRESULT xxxTooltipWndProc(PWND pwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    PTOOLTIPWND pttwnd;

    CheckLock(pwnd);
    VALIDATECLASSANDSIZE(pwnd, uMsg, wParam, lParam, FNID_TOOLTIP, WM_NCCREATE);
    pttwnd = (PTOOLTIPWND)pwnd;

    switch(uMsg) {
    case WM_TIMER:
        xxxTooltipHandleTimer(pttwnd, (UINT)wParam);
        break;

    case WM_PAINT:
        xxxBeginPaint(pwnd, &ps);
        TooltipRender(pttwnd, ps.hdc);
        xxxEndPaint(pwnd, &ps);
        break;

    case WM_PRINTCLIENT:
        TooltipRender(pttwnd, (HDC)wParam);
        break;

    case WM_ERASEBKGND:
        break;

    case WM_NCCREATE:
        InitTooltipDelay(pttwnd);
        InitTooltipAnimation(pttwnd);
        goto CallDWP;

    case WM_NCDESTROY:
        CleanupTooltipAnimation(pttwnd);
        GETPDESK(pttwnd)->dwDTFlags &= ~DF_TOOLTIP;
        goto CallDWP;

    case WM_WINDOWPOSCHANGED:
        if (((LPWINDOWPOS)lParam)->flags & SWP_SHOWWINDOW) {
            HDC hdc;
            int cx;
            int cy;

            if (!TestEffectUP(TOOLTIPANIMATION)) {
                SetTooltipTimer(pttwnd, TTT_HIDE, pttwnd->dwHideDelay);
                goto CallDWP;
            }

            hdc = NULL;
            cx = pttwnd->rcWindow.right - pttwnd->rcWindow.left;
            cy = pttwnd->rcWindow.bottom - pttwnd->rcWindow.top;

            /*
             * At this point we're sure that the window is showing and the size
             * has been changed and we're in the context of the desktop thread.
             */
            if (TestALPHA(TOOLTIPFADE)) {
                hdc = CreateFade((PWND)pttwnd, NULL, CMS_TOOLTIP,
                        FADE_SHOW | FADE_TOOLTIP);
            } else {
                if (CreateTooltipBitmap(pttwnd, cx, cy)) {
                    hdc = pttwnd->hdcMem;
                }
            }

            if (hdc == NULL) {
                SetTooltipTimer(pttwnd, TTT_HIDE, 0);
                goto CallDWP;
            }

            xxxSendMessage((PWND)pttwnd, WM_PRINT, (WPARAM)hdc,
                    PRF_CLIENT | PRF_NONCLIENT | PRF_CHILDREN | PRF_ERASEBKGND);

            /*
             * Start animation timer
             */

            if (TestFadeFlags(FADE_TOOLTIP)) {
                StartFade();
                SetTooltipTimer(pttwnd, TTT_HIDE, pttwnd->dwHideDelay);
            } else {
                pttwnd->dwAnimStart = NtGetTickCount();
                SetTooltipTimer(pttwnd, TTT_ANIMATE, TT_ANIMATEDELAY);
            }
        } else if (((LPWINDOWPOS)lParam)->flags & SWP_HIDEWINDOW) {
            if (TestFadeFlags(FADE_TOOLTIP)) {
                StopFade();
            } else {
                DestroyTooltipBitmap(pttwnd);
            }
        }
        goto CallDWP;

    default:
CallDWP:
        return xxxDefWindowProc(pwnd, uMsg, wParam, lParam);
    }

    return 0;
}

/***************************************************************************\
* IsTrackedHittest
*
* Should we be tracking this hittest code? Return the track string if yes.
* If on caption returning the window strName.Buffer could
* make the system bugcheck if there is a SetWindowText in the callback.
\***************************************************************************/
LPWSTR IsTooltipHittest(PWND pwnd, UINT ht)
{
    switch (ht) {
    case HTMINBUTTON:
        if (TestWF(pwnd, WFMINBOX)) {
            return (TestWF(pwnd, WFMINIMIZED)) ? gszRESUP : gszMIN;
        }
        break;

    case HTMAXBUTTON:
        if (TestWF(pwnd, WFMAXBOX)) {
            return (TestWF(pwnd, WFMAXIMIZED)) ? gszRESDOWN : gszMAX;
        }
        break;

    case HTCLOSE:
    case HTMDICLOSE:
        return gszSCLOSE;

/*  Commented out due to TandyT ...
    case HTSYSMENU:
    case HTMDISYSMENU:
        return gszSMENU;
*/
    case HTHELP:
        return GETGPSIMBPSTR(SEB_HELP);

    case HTMDIMINBUTTON:
        return gszMIN;

    case HTMDIMAXBUTTON:
        return gszRESDOWN;

    case HTCAPTION:
        /*
         * We only show the caption tooltip if the window text
         * doesn't fit entirely on the caption.  We will fill
         * gszCAPTIONTOOLTIP right before showing the text
         */
        if (TestWF(pwnd, WEFTRUNCATEDCAPTION)) {
            return gszCAPTIONTOOLTIP;
        }
        break;

    default:
        break;
    }
    return NULL;
}

/***************************************************************************\
* xxxHotTrackMenu
*
* Hot-track a menu item in the menu bar.
\***************************************************************************/
BOOL xxxHotTrackMenu(PWND pwnd, UINT nItem, BOOL fDraw)
{
    PMENU pmenu = pwnd->spmenu;
    PITEM pItem;
    HDC   hdc;
    UINT  oldAlign;
    TL tlpmenu;

    CheckLock(pwnd);

    /*
     * The window may have lied about the hit-test code on
     * WM_NCHITTEST. Make sure it does indeed have a menu.
     */
    if (!TestWF(pwnd, WFMPRESENT) || pmenu == NULL)
        return FALSE;

    if (nItem >= pmenu->cItems) {
        RIPMSG0(RIP_WARNING, "xxxHotTrackMenu: menu too large");
        return FALSE;
    }

    pItem = &pmenu->rgItems[nItem];

    /*
     * Make sure we draw on the right spot
     */
    ThreadLock(pmenu, &tlpmenu);
    xxxMNRecomputeBarIfNeeded(pwnd, pmenu);
    ValidateThreadLocks(NULL, PtiCurrent()->ptl, (ULONG_PTR)&tlpmenu, TRUE);

    if (fDraw) {
        if (TestMFS(pItem, MF_GRAYED)) {
            ThreadUnlock(&tlpmenu);
            return FALSE;
        }
        SetMFS(pItem, MFS_HOTTRACK);
    } else {
        ClearMFS(pItem, MFS_HOTTRACK);
    }

    hdc = _GetDCEx(pwnd, NULL, DCX_WINDOW | DCX_USESTYLE | DCX_CACHE);
    GreSelectBrush(hdc, SYSHBR(MENUTEXT));
    GreSelectFont(hdc, ghMenuFont);

    oldAlign = GreGetTextAlign(hdc);
    if (pmenu->rgItems && TestMFT(pmenu->rgItems, MFT_RIGHTORDER))
        GreSetTextAlign(hdc, oldAlign | TA_RTLREADING);

    /*
     * When the item is not owner draw, xxxDrawMenuItem does not
     * call back and does not leave the critical section.
     */
    xxxDrawMenuItem(hdc, pmenu, pItem, 0);
    GreSetTextAlign(hdc, oldAlign);
    ThreadUnlock(&tlpmenu);

    _ReleaseDC(hdc);
    return TRUE;
}


/***************************************************************************\
* HotTrackCaption
*
* Hot-track a caption button.
\***************************************************************************/

#ifdef COLOR_HOTTRACKING

BOOL xxxHotTrackCaption(PWND pwnd, int ht, BOOL fDraw)
{
    DWORD dwWhere;
    int   x, y;
    WORD  bm, cmd;
    RECT  rcBtn;
    HDC   hdc;

    CheckLock(pwnd);

    if (!TestWF(pwnd, WFCPRESENT))
        return FALSE;

    dwWhere = xxxCalcCaptionButton(pwnd, ht, &cmd, &rcBtn, &bm);
    x = GET_X_LPARAM(dwWhere);
    y = GET_Y_LPARAM(dwWhere);

    if (!cmd)
        return FALSE;

    hdc = _GetDCEx(pwnd, NULL, DCX_WINDOW | DCX_USESTYLE | DCX_CACHE);
    BitBltSysBmp(hdc, x, y, bm + (fDraw ? DOBI_HOT : 0));
    _ReleaseDC(hdc);
    return TRUE;
}

#endif // COLOR_HOTTRACKING

/***************************************************************************\
* xxxHotTrack
*
\***************************************************************************/

BOOL xxxHotTrack(PWND pwnd, int htEx, BOOL fDraw)
{
    int ht = LOWORD(htEx);

    CheckLock(pwnd);

    switch(ht) {
#ifdef COLOR_HOTTRACKING
    case HTMINBUTTON:
    case HTMAXBUTTON:
    case HTHELP:
    case HTCLOSE:
        return xxxHotTrackCaption(pwnd, ht, fDraw);

    case HTSCROLLUP:
    case HTSCROLLDOWN:
    case HTSCROLLUPPAGE:
    case HTSCROLLDOWNPAGE:
    case HTSCROLLTHUMB:
        return xxxHotTrackSB(pwnd, htEx, fDraw);

    case HTMDIMINBUTTON:
    case HTMDIMAXBUTTON:
    case HTMDICLOSE:
#endif // COLOR_HOTTRACKING
    case HTMENUITEM:
        return xxxHotTrackMenu(pwnd, HIWORD(htEx), fDraw);

    }

    return FALSE;
}

/***************************************************************************\
* xxxCreateTooltip
*
* Call this to show a new tooltip with a new string and delay.
\***************************************************************************/

void xxxCreateTooltip(PTOOLTIPWND pttwnd, LPWSTR pstr)
{
    CheckLock(pttwnd);

    /*
     * Store new text
     */
    pttwnd->pstr = pstr;
    /*
     * If already visible, hide it and show it in new place.
     *  Otherwise, set timer to show.
     */
    if (TestWF(pttwnd, WFVISIBLE)) {
        xxxSetWindowPos((PWND)pttwnd, NULL, 0, 0, 0, 0, SWP_NOSIZE |
                SWP_NOMOVE | SWP_HIDEWINDOW | SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSENDCHANGING);
        xxxShowTooltip(pttwnd);
    } else {
        SetTooltipTimer(pttwnd, TTT_SHOW, pttwnd->dwShowDelay);
    }
}

/***************************************************************************\
* xxxTrackMouseMove
*
* This is the entry point for the system tooltips and hot-tracking.
*
* 12-Sep-96 vadimg      created
\***************************************************************************/

void xxxTrackMouseMove(PWND pwnd, int htEx, UINT message)
{
    BOOL fNewpwndTrack;
    DWORD dwDTCancel = 0;
    TL tlpwnd;
    LPWSTR pstr;
    PDESKTOP pdesk = PtiCurrent()->rpdesk;
    PTHREADINFO ptiTrack;


#if DBG
    /*
     * Let's warn if this function gets reenterd so we can make sure
     * nothing bad will follow. This should be a rare situation.
     * Look in gptiReEntered to find out who is already here.
     */
    static UINT gcReEntered = 0;
    static PTHREADINFO gptiReEntered;
    if(gcReEntered++ != 0){
      RIPMSG2(RIP_WARNING, "Reentered xxxTrackMouseMove; previous thread was %#p, current thread is %#p", gptiReEntered, PtiCurrent());
    }
    gptiReEntered = PtiCurrent();

    CheckLock(pwnd);

    /*
     * We must be on an interactive window station.
     */
    if (pdesk->rpwinstaParent != NULL &&
            pdesk->rpwinstaParent->dwWSF_Flags & WSF_NOIO) {
        RIPMSG0(RIP_ERROR, "Can't use tooltips on non-interactive winsta");
    }

     {
        static POINT pt = {0, 0};

#ifdef UNDONE
        /*
         * We might have taken a guess on the hit test (see FindNCHitEx)
         *  so if we're at the same point and same window, something
         *  might be fishy
         */
        if ((pt.x == gpsi->ptCursor.x)
                    && (pt.y == gpsi->ptCursor.y)
                    && (pdesk->spwndTrack == pwnd)) {
            RIPMSG1(RIP_WARNING, "xxxTrackMouseMove: Same point & window. %#p", pwnd);
        }
#endif

        /*
         * Something is supposed to have changed or we're wasting time
         */
        UserAssert((pt.x != gpsi->ptCursor.x)
                    || (pt.y != gpsi->ptCursor.y)
                    || (pdesk->spwndTrack != pwnd)
                    || (pdesk->htEx != htEx)
                    || (message != WM_MOUSEMOVE));
        /*
         * Remember last tracked point
         */
        pt = gpsi->ptCursor;
     }
    /*
     * pwnd is supposed to be on the current thread and queue
     */
    UserAssert(PtiCurrent() == GETPTI(pwnd));
    UserAssert(PtiCurrent()->pq == GETPTI(pwnd)->pq);
#endif

    /*
     * Have we switched windows?
     */
    fNewpwndTrack = (pdesk->spwndTrack != pwnd);
    /*
     * If no tracking is taking place, just go set the new
     *  tracking state
     */
    if (!(pdesk->dwDTFlags & DF_MOUSEMOVETRK)) {
        goto SetNewState;
    }
    /*
     * Potentially while we leave the critical section below in
     * xxxCancelMouseMoveTracking, spwndTrack could be destroyed and unlocked
     * and then we go and create the tooltip. This would mean that
     * DF_TOOLTIPACTIVE (part of DF_MOUSEMOVETRK test above) would be set,
     * but pdesk->spwndTrack would be NULL and we can AV dereferencing
     * pdesk->spwndTrack below. Prevent this by making the check here.
     */
    if (pdesk->spwndTrack == NULL) {
        goto SetNewState;
    }

    /*
     * Nuke hottracking and deactivate tooltip state, if any.
     * Do it sychronously if we're tracking on the current queue;
     *  Otherwise, post an event and let it happen later.
     */
    ptiTrack = GETPTI(pdesk->spwndTrack);
    if  (PtiCurrent()->pq == ptiTrack->pq) {
        dwDTCancel |= DF_HOTTRACKING;
    } else if (pdesk->dwDTFlags & (DF_HOTTRACKING | DF_TOOLTIPACTIVE)) {
        PostEventMessage(ptiTrack, ptiTrack->pq,
                        QEVENT_CANCELMOUSEMOVETRK,
                        pdesk->spwndTrack,
                        pdesk->dwDTFlags,
                        pdesk->htEx, DF_HOTTRACKING);
       /*
        * Paranoid assertion. If we're switching queues, we must
        *  be switching windows. Did we just go through
        *  ReattachThreads?
        */
        UserAssert(pwnd != pdesk->spwndTrack);
        pdesk->dwDTFlags &= ~(DF_HOTTRACKING | DF_TOOLTIPACTIVE);
    }
    /*
     * If we're on the client area or the user clicked,
     *  nuke the tooltip (if any).
     * Since we might want to re-show the tooltip, we don't nuke it
     *  now if we swichted windows (we'll nuke it later if needed)
     */
    if ((htEx == HTCLIENT) || (message != WM_MOUSEMOVE)) {
        dwDTCancel |= DF_TOOLTIPACTIVE;
    }
    /*
     * If we switched windows or crossed client/nonclinet boundaries,
     *  end track mouse leave/hover.
     */
    if (fNewpwndTrack || ((pdesk->htEx == HTCLIENT) ^ (htEx == HTCLIENT))) {
        dwDTCancel |= DF_TRACKMOUSEEVENT;
    }
    /*
     * Cancel whatever is active and needs to go away
     */
    ThreadLockAlways(pdesk->spwndTrack, &tlpwnd);
    xxxCancelMouseMoveTracking(pdesk->dwDTFlags,
                           pdesk->spwndTrack,
                           pdesk->htEx,
                           dwDTCancel);
    ThreadUnlock(&tlpwnd);
    pdesk->dwDTFlags &= ~dwDTCancel;



SetNewState:
    /*
     * Hottracking/tooltip on mouse move if on NC hitest and enabled
     */
    if ((htEx != HTCLIENT) && (message == WM_MOUSEMOVE) && TestEffectUP(HOTTRACKING)) {
        /*
         * Hottrack the new hit test area
         */
        if (xxxHotTrack(pwnd, htEx, TRUE)) {
            pdesk->dwDTFlags |= DF_HOTTRACKING;
        }

        /*
         * Remove/set the tool tip.
         * We always do this synchronously because it doesn't mess
         *  with pwnd's or spwnTrack's queue
         */
        if ((pstr = IsTooltipHittest(pwnd, LOWORD(htEx))) != NULL) {
            PTOOLTIPWND pttwnd = (PTOOLTIPWND)pdesk->spwndTooltip;
            ThreadLockAlways(pttwnd, &tlpwnd);
            xxxCreateTooltip(pttwnd, pstr);
            ThreadUnlock(&tlpwnd);
            pdesk->dwDTFlags |= DF_TOOLTIP;
        } else  {
            PTOOLTIPWND pttwnd = (PTOOLTIPWND)pdesk->spwndTooltip;
            ThreadLockAlways(pttwnd, &tlpwnd);
            xxxResetTooltip(pttwnd);
            ThreadUnlock(&tlpwnd);
        }
    } /* if (htEx != HTCLIENT) */


    ValidateThreadLocks(NULL, PtiCurrent()->ptl, (ULONG_PTR)&pwnd, TRUE);

    /*
     * Update new track window if needed.
     */
    if (fNewpwndTrack) {
        PWND pwndActivate;

         Lock(&pdesk->spwndTrack, pwnd);
        /*
         * Active window tracking.
         * If there is non-zero timeout, get the window we're supposed to activate
         *  and set the timer. Otherwise, set the queue flag so
         *  xxxActiveWindowTracking can do its thing.
         */
         if ((message == WM_MOUSEMOVE) && TestUP(ACTIVEWINDOWTRACKING)) {
             if (UP(ACTIVEWNDTRKTIMEOUT) != 0) {
                 pwndActivate = GetActiveTrackPwnd(pwnd, NULL);
                 if (pwndActivate != NULL) {
                     InternalSetTimer(pwndActivate, IDSYS_WNDTRACKING,
                                     UP(ACTIVEWNDTRKTIMEOUT),
                                     xxxSystemTimerProc, TMRF_SYSTEM);
                 }
             } else {
                 PtiCurrent()->pq->QF_flags |= QF_ACTIVEWNDTRACKING;
             } /* if (TestUP(ACTIVEWNDTRKZORDER)) */
         } /* if (TestUP(ACTIVEWINDOWTRACKING)) */

    }

    /*
     * Save new hit test code
     */
    pdesk->htEx = htEx;

#if DBG
    --gcReEntered;
#endif
}

/***************************************************************************\
* xxxCancelMouseMoveTracking
*
* History
* 12/07/96 GerardoB  Created
\***************************************************************************/
void xxxCancelMouseMoveTracking (DWORD dwDTFlags, PWND pwndTrack, int htEx, DWORD dwDTCancel)
{

    CheckLock(pwndTrack);
    /*
     * Hottracking
     */
    if ((dwDTFlags & DF_HOTTRACKING) && (dwDTCancel & DF_HOTTRACKING)) {
        /*
         * The current state must be owned by the current queue.
         * Otherwise, we're about to do an inter-queue cancelation.
         */
        UserAssert(PtiCurrent()->pq == GETPTI(pwndTrack)->pq);

        xxxHotTrack(pwndTrack, htEx, FALSE);
    }

    /*
     * Tooltips
     */
    if ((dwDTFlags & DF_TOOLTIPSHOWING) && (dwDTCancel & DF_TOOLTIP)) {
        PTOOLTIPWND pttwnd = (PTOOLTIPWND)PWNDTOOLTIP(pwndTrack);
        TL tlpwnd;

        ThreadLockAlways(pttwnd, &tlpwnd);
        xxxResetTooltip(pttwnd);
        ThreadUnlock(&tlpwnd);
    }

    /*
     * Mouse Leave
     */
    if ((dwDTFlags & DF_TRACKMOUSELEAVE) && (dwDTCancel & DF_TRACKMOUSELEAVE)) {
        _PostMessage(pwndTrack,
                     ((htEx == HTCLIENT) ? WM_MOUSELEAVE : WM_NCMOUSELEAVE),
                     0, 0);
    }

    /*
     * Mouse Hover
     */
    if ((dwDTFlags & DF_TRACKMOUSEHOVER) && (dwDTCancel & DF_TRACKMOUSEHOVER)) {
        _KillSystemTimer(pwndTrack, IDSYS_MOUSEHOVER);
    }
}
