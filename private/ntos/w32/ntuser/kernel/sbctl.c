/**************************** Module Header ********************************\
* Module Name: sbctl.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Scroll bar internal routines
*
* History:
*   11/21/90 JimA      Created.
*   02-04-91 IanJa     Revalidaion added
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

void CalcSBStuff(
    PWND pwnd,
    PSBCALC pSBCalc,
    BOOL fVert);

#define IsScrollBarControl(h) (GETFNID(h) == FNID_SCROLLBAR)

/*
 * Now it is possible to selectively Enable/Disable just one arrow of a Window
 * scroll bar; Various bits in the 7th word in the rgwScroll array indicates which
 * one of these arrows are disabled; The following masks indicate which bit of the
 * word indicates which arrow;
 */
#define WSB_HORZ_LF  0x0001  // Represents the Left arrow of the horizontal scroll bar.
#define WSB_HORZ_RT  0x0002  // Represents the Right arrow of the horizontal scroll bar.
#define WSB_VERT_UP  0x0004  // Represents the Up arrow of the vert scroll bar.
#define WSB_VERT_DN  0x0008  // Represents the Down arrow of the vert scroll bar.

#define WSB_VERT (WSB_VERT_UP | WSB_VERT_DN)
#define WSB_HORZ   (WSB_HORZ_LF | WSB_HORZ_RT)

void DrawCtlThumb(PSBWND);

/*
 * RETURN_IF_PSBTRACK_INVALID:
 * This macro tests whether the pSBTrack we have is invalid, which can happen
 * if it gets freed during a callback.
 * This protects agains the original pSBTrack being freed and no new one
 * being allocated or a new one being allocated at a different address.
 * This does not protect against the original pSBTrack being freed and a new
 * one being allocated at the same address.
 * If pSBTrack has changed, we assert that there is not already a new one
 * because we are really not expecting this.
 */
#define RETURN_IF_PSBTRACK_INVALID(pSBTrack, pwnd) \
    if ((pSBTrack) != PWNDTOPSBTRACK(pwnd)) {      \
        UserAssert(PWNDTOPSBTRACK(pwnd) == NULL);  \
        return;                                    \
    }

/*
 * REEVALUATE_PSBTRACK
 * This macro just refreshes the local variable pSBTrack, in case it has
 * been changed during a callback.  After performing this operation, pSBTrack
 * should be tested to make sure it is not now NULL.
 */
#if DBG
#define REEVALUATE_PSBTRACK(pSBTrack, pwnd, str)          \
    if ((pSBTrack) != PWNDTOPSBTRACK(pwnd)) {             \
        RIPMSG3(RIP_WARNING,                              \
                "%s: pSBTrack changed from %#p to %#p",   \
                (str), (pSBTrack), PWNDTOPSBTRACK(pwnd)); \
    }                                                     \
    (pSBTrack) = PWNDTOPSBTRACK(pwnd)
#else
#define REEVALUATE_PSBTRACK(pSBTrack, pwnd, str)          \
    (pSBTrack) = PWNDTOPSBTRACK(pwnd)
#endif

/***************************************************************************\
* HitTestScrollBar
*
* 11/15/96      vadimg          ported from Memphis sources
\***************************************************************************/

int HitTestScrollBar(PWND pwnd, BOOL fVert, POINT pt)
{
    UINT wDisable;
    int px;
    BOOL fCtl = IsScrollBarControl(pwnd);
    SBCALC SBCalc, *pSBCalc;

    if (fCtl) {
        wDisable = ((PSBWND)pwnd)->wDisableFlags;
    } else {
#ifdef USE_MIRRORING
        //
        // Reflect the click coordinates on the horizontal
        // scroll bar if the window is mirrored
        //
        if (TestWF(pwnd,WEFLAYOUTRTL) && !fVert) {
            pt.x = pwnd->rcWindow.right - pt.x;
        }
        else
#endif
        pt.x -= pwnd->rcWindow.left;

        pt.y -= pwnd->rcWindow.top;
        wDisable = GetWndSBDisableFlags(pwnd, fVert);
    }

    if ((wDisable & SB_DISABLE_MASK) == SB_DISABLE_MASK) {
        return HTERROR;
    }

    if (fCtl) {
        pSBCalc = &(((PSBWND)pwnd)->SBCalc);
    } else {
        pSBCalc = &SBCalc;
        CalcSBStuff(pwnd, pSBCalc, fVert);
    }

    px = fVert ? pt.y : pt.x;

    if (px < pSBCalc->pxUpArrow) {
        if (wDisable & LTUPFLAG) {
            return HTERROR;
        }
        return HTSCROLLUP;
    } else if (px >= pSBCalc->pxDownArrow) {
        if (wDisable & RTDNFLAG) {
            return HTERROR;
        }
        return HTSCROLLDOWN;
    } else if (px < pSBCalc->pxThumbTop) {
        return HTSCROLLUPPAGE;
    } else if (px < pSBCalc->pxThumbBottom) {
        return HTSCROLLTHUMB;
    } else if (px < pSBCalc->pxDownArrow) {
        return HTSCROLLDOWNPAGE;
    }
    return HTERROR;
}

BOOL _SBGetParms(
    PWND pwnd,
    int code,
    PSBDATA pw,
    LPSCROLLINFO lpsi)
{
    PSBTRACK pSBTrack;

    pSBTrack = PWNDTOPSBTRACK(pwnd);


    if (lpsi->fMask & SIF_RANGE) {
        lpsi->nMin = pw->posMin;
        lpsi->nMax = pw->posMax;
    }

    if (lpsi->fMask & SIF_PAGE)
        lpsi->nPage = pw->page;

    if (lpsi->fMask & SIF_POS) {
        lpsi->nPos = pw->pos;
    }

    if (lpsi->fMask & SIF_TRACKPOS)
    {
        if (pSBTrack && (pSBTrack->nBar == code) && (pSBTrack->spwndTrack == pwnd)) {
            // posNew is in the context of psbiSB's window and bar code
            lpsi->nTrackPos = pSBTrack->posNew;
        } else {
            lpsi->nTrackPos = pw->pos;
        }
    }
    return ((lpsi->fMask & SIF_ALL) ? TRUE : FALSE);
}

/***************************************************************************\
* GetWndSBDisableFlags
*
* This returns the scroll bar Disable flags of the scroll bars of a
*  given Window.
*
*
* History:
*  4-18-91 MikeHar Ported for the 31 merge
\***************************************************************************/

UINT GetWndSBDisableFlags(
    PWND pwnd,  // The window whose scroll bar Disable Flags are to be returned;
    BOOL fVert)  // If this is TRUE, it means Vertical scroll bar.
{
    PSBINFO pw;

    if ((pw = pwnd->pSBInfo) == NULL) {
        RIPERR0(ERROR_NO_SCROLLBARS, RIP_VERBOSE, "");
        return 0;
    }

    return (fVert ? (pw->WSBflags & WSB_VERT) >> 2 : pw->WSBflags & WSB_HORZ);
}


/***************************************************************************\
*  xxxEnableSBCtlArrows()
*
*  This function can be used to selectively Enable/Disable
*     the arrows of a scroll bar Control
*
* History:
* 04-18-91 MikeHar      Ported for the 31 merge
\***************************************************************************/

BOOL xxxEnableSBCtlArrows(
    PWND pwnd,
    UINT wArrows)
{
    UINT wOldFlags;

    CheckLock(pwnd);
    UserAssert(IsWinEventNotifyDeferredOK());

    wOldFlags = ((PSBWND)pwnd)->wDisableFlags; // Get the original status

    if (wArrows == ESB_ENABLE_BOTH) {      // Enable both the arrows
        ((PSBWND)pwnd)->wDisableFlags &= ~SB_DISABLE_MASK;
    } else {
        ((PSBWND)pwnd)->wDisableFlags |= wArrows;
    }

    /*
     * Check if the status has changed because of this call
     */
    if (wOldFlags == ((PSBWND)pwnd)->wDisableFlags)
        return FALSE;

    /*
     * Else, redraw the scroll bar control to reflect the new state
     */
    if (IsVisible(pwnd))
        xxxInvalidateRect(pwnd, NULL, TRUE);

    if (FWINABLE()) {
        UINT wNewFlags = ((PSBWND)pwnd)->wDisableFlags;

        /*
         * state change notifications
         */
        if ((wOldFlags & ESB_DISABLE_UP) != (wNewFlags & ESB_DISABLE_UP)) {
            xxxWindowEvent(EVENT_OBJECT_STATECHANGE, pwnd, OBJID_CLIENT,
                    INDEX_SCROLLBAR_UP, WEF_USEPWNDTHREAD);
        }

        if ((wOldFlags & ESB_DISABLE_DOWN) != (wNewFlags & ESB_DISABLE_DOWN)) {
            xxxWindowEvent(EVENT_OBJECT_STATECHANGE, pwnd, OBJID_CLIENT,
                    INDEX_SCROLLBAR_DOWN, WEF_USEPWNDTHREAD);
        }
    }

    return TRUE;
}


/***************************************************************************\
* xxxEnableWndSBArrows()
*
*  This function can be used to selectively Enable/Disable
*     the arrows of a Window Scroll bar(s)
*
* History:
*  4-18-91 MikeHar      Ported for the 31 merge
\***************************************************************************/

BOOL xxxEnableWndSBArrows(
    PWND pwnd,
    UINT wSBflags,
    UINT wArrows)
{
    INT wOldFlags;
    PSBINFO pw;
    BOOL bRetValue = FALSE;
    HDC hdc;

    CheckLock(pwnd);
    UserAssert(IsWinEventNotifyDeferredOK());

    if ((pw = pwnd->pSBInfo) != NULL) {
        wOldFlags = pw->WSBflags;
    } else {

        /*
         * Originally everything is enabled; Check to see if this function is
         * asked to disable anything; Otherwise, no change in status; So, must
         * return immediately;
         */
        if(!wArrows)
            return FALSE;          // No change in status!

        wOldFlags = 0;    // Both are originally enabled;
        if((pw = _InitPwSB(pwnd)) == NULL)  // Allocate the pSBInfo for hWnd
            return FALSE;
    }


    if((hdc = _GetWindowDC(pwnd)) == NULL)
        return FALSE;

    /*
     *  First Take care of the Horizontal Scroll bar, if one exists.
     */
    if((wSBflags == SB_HORZ) || (wSBflags == SB_BOTH)) {
        if(wArrows == ESB_ENABLE_BOTH)      // Enable both the arrows
            pw->WSBflags &= ~SB_DISABLE_MASK;
        else
            pw->WSBflags |= wArrows;

        /*
         * Update the display of the Horizontal Scroll Bar;
         */
        if(pw->WSBflags != wOldFlags) {
            bRetValue = TRUE;
            wOldFlags = pw->WSBflags;
            if (TestWF(pwnd, WFHPRESENT) && !TestWF(pwnd, WFMINIMIZED) &&
                    IsVisible(pwnd)) {
                xxxDrawScrollBar(pwnd, hdc, FALSE);  // Horizontal Scroll Bar.
            }
        }
        if (FWINABLE()) {
            // Left button
            if ((wOldFlags & ESB_DISABLE_LEFT) != (pw->WSBflags & ESB_DISABLE_LEFT)) {
                xxxWindowEvent(EVENT_OBJECT_STATECHANGE, pwnd, OBJID_HSCROLL,
                        INDEX_SCROLLBAR_UP, WEF_USEPWNDTHREAD);
            }

            // Right button
            if ((wOldFlags & ESB_DISABLE_RIGHT) != (pw->WSBflags & ESB_DISABLE_RIGHT)) {
                xxxWindowEvent(EVENT_OBJECT_STATECHANGE, pwnd, OBJID_HSCROLL,
                        INDEX_SCROLLBAR_DOWN, WEF_USEPWNDTHREAD);
            }
        }
    }

    /*
     *  Then take care of the Vertical Scroll bar, if one exists.
     */
    if((wSBflags == SB_VERT) || (wSBflags == SB_BOTH)) {
        if(wArrows == ESB_ENABLE_BOTH)      // Enable both the arrows
            pw->WSBflags &= ~(SB_DISABLE_MASK << 2);
        else
            pw->WSBflags |= (wArrows << 2);

        /*
         * Update the display of the Vertical Scroll Bar;
         */
        if(pw->WSBflags != wOldFlags) {
            bRetValue = TRUE;
            if (TestWF(pwnd, WFVPRESENT) && !TestWF(pwnd, WFMINIMIZED) &&
                    IsVisible(pwnd)) {
                xxxDrawScrollBar(pwnd, hdc, TRUE);  // Vertical Scroll Bar
            }

            if (FWINABLE()) {
                // Up button
                if ((wOldFlags & (ESB_DISABLE_UP << 2)) != (pw->WSBflags & (ESB_DISABLE_UP << 2))) {
                    xxxWindowEvent(EVENT_OBJECT_STATECHANGE, pwnd, OBJID_VSCROLL,
                            INDEX_SCROLLBAR_UP, WEF_USEPWNDTHREAD);
                }

                // Down button
                if ((wOldFlags & (ESB_DISABLE_DOWN << 2)) != (pw->WSBflags & (ESB_DISABLE_DOWN << 2))) {
                    xxxWindowEvent(EVENT_OBJECT_STATECHANGE, pwnd, OBJID_VSCROLL,
                            INDEX_SCROLLBAR_DOWN, WEF_USEPWNDTHREAD);
                }
            }
        }
    }

    _ReleaseDC(hdc);

    return bRetValue;
}


/***************************************************************************\
* EnableScrollBar()
*
* This function can be used to selectively Enable/Disable
*     the arrows of a scroll bar; It could be used with Windows Scroll
*     bars as well as scroll bar controls
*
* History:
*  4-18-91 MikeHar Ported for the 31 merge
\***************************************************************************/

BOOL xxxEnableScrollBar(
    PWND pwnd,
    UINT wSBflags,  // Whether it is a Window Scroll Bar; if so, HORZ or VERT?
                    // Possible values are SB_HORZ, SB_VERT, SB_CTL or SB_BOTH
    UINT wArrows)   // Which arrows must be enabled/disabled:
                    // ESB_ENABLE_BOTH = > Enable both arrows.
                    // ESB_DISABLE_LTUP = > Disable Left/Up arrow;
                    // ESB_DISABLE_RTDN = > DIsable Right/Down arrow;
                    // ESB_DISABLE_BOTH = > Disable both the arrows;
{
#define ES_NOTHING 0
#define ES_DISABLE 1
#define ES_ENABLE  2
    UINT wOldFlags;
    UINT wEnableWindow;

    CheckLock(pwnd);

    if(wSBflags != SB_CTL) {
        return xxxEnableWndSBArrows(pwnd, wSBflags, wArrows);
    }

    /*
     *  Let us assume that we don't have to call EnableWindow
     */
    wEnableWindow = ES_NOTHING;

    wOldFlags = ((PSBWND)pwnd)->wDisableFlags & (UINT)SB_DISABLE_MASK;

    /*
     * Check if the present state of the arrows is exactly the same
     *  as what the caller wants:
     */
    if (wOldFlags == wArrows)
        return FALSE ;          // If so, nothing needs to be done;

    /*
     * Check if the caller wants to disable both the arrows
     */
    if (wArrows == ESB_DISABLE_BOTH) {
        wEnableWindow = ES_DISABLE;      // Yes! So, disable the whole SB Ctl.
    } else {

        /*
         * Check if the caller wants to enable both the arrows
         */
        if(wArrows == ESB_ENABLE_BOTH) {

            /*
             * We need to enable the SB Ctl only if it was already disabled.
             */
            if(wOldFlags == ESB_DISABLE_BOTH)
                wEnableWindow = ES_ENABLE;// EnableWindow(.., TRUE);
        } else {

            /*
             * Now, Caller wants to disable only one arrow;
             * Check if one of the arrows was already disabled and we want
             * to disable the other;If so, the whole SB Ctl will have to be
             * disabled; Check if this is the case:
             */
            if((wOldFlags | wArrows) == ESB_DISABLE_BOTH)
                wEnableWindow = ES_DISABLE;      // EnableWindow(, FALSE);
         }
    }
    if(wEnableWindow != ES_NOTHING) {

        /*
         * EnableWindow returns old state of the window; We must return
         * TRUE only if the Old state is different from new state.
         */
        if(xxxEnableWindow(pwnd, (BOOL)(wEnableWindow == ES_ENABLE))) {
            return !(TestWF(pwnd, WFDISABLED));
        } else {
            return TestWF(pwnd, WFDISABLED);
        }
    }

    return (BOOL)xxxSendMessage(pwnd, SBM_ENABLE_ARROWS, (DWORD)wArrows, 0);
#undef ES_NOTHING
#undef ES_DISABLE
#undef ES_ENABLE
}

/***************************************************************************\
*
*  DrawSize() -
*
\***************************************************************************/
void FAR DrawSize(PWND pwnd, HDC hdc, int cxFrame,int cyFrame)
{
    int     x, y;
    //HBRUSH  hbrSave;

    if (TestWF(pwnd, WEFLEFTSCROLL)) {
        x = cxFrame;
    } else {
        x = pwnd->rcWindow.right - pwnd->rcWindow.left - cxFrame - SYSMET(CXVSCROLL);
    }
    y = pwnd->rcWindow.bottom - pwnd->rcWindow.top  - cyFrame - SYSMET(CYHSCROLL);

    // If we have a scrollbar control, or the sizebox is not associated with
    // a sizeable window, draw the flat gray sizebox.  Otherwise, use the
    // sizing grip.
    if (IsScrollBarControl(pwnd))
    {
        if (TestWF(pwnd, SBFSIZEGRIP))
            goto DrawSizeGrip;
        else
            goto DrawBox;

    }
    else if (!SizeBoxHwnd(pwnd))
    {
DrawBox:
        {
            //hbrSave = GreSelectBrush(hdc, SYSHBR(3DFACE));
            //GrePatBlt(hdc, x, y, SYSMET(CXVSCROLL), SYSMET(CYHSCROLL), PATCOPY);
            //GreSelectBrush(hdc, hbrSave);

            POLYPATBLT PolyData;

            PolyData.x         = x;
            PolyData.y         = y;
            PolyData.cx        = SYSMET(CXVSCROLL);
            PolyData.cy        = SYSMET(CYHSCROLL);
            PolyData.BrClr.hbr = SYSHBR(3DFACE);

            GrePolyPatBlt(hdc,PATCOPY,&PolyData,1,PPB_BRUSH);

        }
    }
    else
    {
DrawSizeGrip:
        // Blt out the grip bitmap.
        BitBltSysBmp(hdc, x, y, TestWF(pwnd, WEFLEFTSCROLL) ? OBI_NCGRIP_L : OBI_NCGRIP);
    }
}

/***************************************************************************\
* xxxSelectColorObjects
*
*
*
* History:
\***************************************************************************/

HBRUSH xxxGetColorObjects(
    PWND pwnd,
    HDC hdc)
{
    HBRUSH hbrRet;

    CheckLock(pwnd);

    // Use the scrollbar color even if the scrollbar is disabeld.
    if (!IsScrollBarControl(pwnd))
        hbrRet = (HBRUSH)xxxDefWindowProc(pwnd, WM_CTLCOLORSCROLLBAR, (WPARAM)hdc, (LPARAM)HWq(pwnd));
    else {
        // B#12770 - GetControlBrush sends a WM_CTLCOLOR message to the
        // owner.  If the app doesn't process the message, DefWindowProc32
        // will always return the appropriate system brush. If the app.
        // returns an invalid object, GetControlBrush will call DWP for
        // the default brush. Thus hbrRet doesn't need any validation
        // here.
        hbrRet = xxxGetControlBrush(pwnd, hdc, WM_CTLCOLORSCROLLBAR);
    }

    return hbrRet;
}

/***************************************************************************\
*
*  DrawGroove()
*
*  Draws lines & middle of thumb groove
*  Note that pw points into prc.  Moreover, note that both pw & prc are
*  NEAR pointers, so *prc better not be on the stack.
*
\***************************************************************************/
void NEAR DrawGroove(HDC hdc, HBRUSH  hbr, LPRECT prc, BOOL fVert)
{
    if ((hbr == SYSHBR(3DHILIGHT)) || (hbr == gpsi->hbrGray))
        FillRect(hdc, prc, hbr);
    else
    {
        RECT    rc;

    // Draw sides
        CopyRect(&rc, prc);
        DrawEdge(hdc, &rc, EDGE_SUNKEN, BF_ADJUST | BF_FLAT |
            (fVert ? BF_LEFT | BF_RIGHT : BF_TOP | BF_BOTTOM));

    // Fill middle
        FillRect(hdc, &rc, hbr);
    }
}

/***************************************************************************\
* CalcTrackDragRect
*
* Give the rectangle for a scrollbar in pSBTrack->pSBCalc,
* calculate pSBTrack->rcTrack, the rectangle where tracking
* may occur without cancelling the thumbdrag operation.
*
\***************************************************************************/

void CalcTrackDragRect(PSBTRACK pSBTrack) {

    int     cx;
    int     cy;
    LPINT   pwX, pwY;

    //
    // Point pwX and pwY at the parts of the rectangle
    // corresponding to pSBCalc->pxLeft, pxTop, etc.
    //
    // pSBTrack->pSBCalc->pxLeft is the left edge of a vertical
    // scrollbar and the top edge of horizontal one.
    // pSBTrack->pSBCalc->pxTop is the top of a vertical
    // scrollbar and the left of horizontal one.
    // etc...
    //
    // Point pwX and pwY to the corresponding parts
    // of pSBTrack->rcTrack.
    //

    pwX = pwY = (LPINT)&pSBTrack->rcTrack;

    if (pSBTrack->fTrackVert) {
        cy = SYSMET(CYVTHUMB);
        pwY++;
    } else {
        cy = SYSMET(CXHTHUMB);
        pwX++;
    }
    /*
     * Later5.0 GerardoB: People keep complaining about this tracking region
     *  being too narrow so let's make it wider while PM decides what to do
     *  about it.
     * We also used to have some hard coded min and max values but that should
     *  depend on some metric, if at all needed.
     */
    cx = (pSBTrack->pSBCalc->pxRight - pSBTrack->pSBCalc->pxLeft) * 8;
    cy *= 2;

    *(pwX + 0) = pSBTrack->pSBCalc->pxLeft - cx;
    *(pwY + 0) = pSBTrack->pSBCalc->pxTop - cy;
    *(pwX + 2) = pSBTrack->pSBCalc->pxRight + cx;
    *(pwY + 2) = pSBTrack->pSBCalc->pxBottom + cy;
}

void RecalcTrackRect(PSBTRACK pSBTrack) {
    LPINT pwX, pwY;
    RECT rcSB;


    if (!pSBTrack->fCtlSB)
        CalcSBStuff(pSBTrack->spwndTrack, pSBTrack->pSBCalc, pSBTrack->fTrackVert);

    pwX = (LPINT)&rcSB;
    pwY = pwX + 1;
    if (!pSBTrack->fTrackVert)
        pwX = pwY--;

    *(pwX + 0) = pSBTrack->pSBCalc->pxLeft;
    *(pwY + 0) = pSBTrack->pSBCalc->pxTop;
    *(pwX + 2) = pSBTrack->pSBCalc->pxRight;
    *(pwY + 2) = pSBTrack->pSBCalc->pxBottom;

    switch(pSBTrack->cmdSB) {
    case SB_LINEUP:
        *(pwY + 2) = pSBTrack->pSBCalc->pxUpArrow;
        break;
    case SB_LINEDOWN:
        *(pwY + 0) = pSBTrack->pSBCalc->pxDownArrow;
        break;
    case SB_PAGEUP:
        *(pwY + 0) = pSBTrack->pSBCalc->pxUpArrow;
        *(pwY + 2) = pSBTrack->pSBCalc->pxThumbTop;
        break;
    case SB_THUMBPOSITION:
        CalcTrackDragRect(pSBTrack);
        break;
    case SB_PAGEDOWN:
        *(pwY + 0) = pSBTrack->pSBCalc->pxThumbBottom;
        *(pwY + 2) = pSBTrack->pSBCalc->pxDownArrow;
        break;
    }

    if (pSBTrack->cmdSB != SB_THUMBPOSITION) {
        CopyRect(&pSBTrack->rcTrack, &rcSB);
    }
}

/***************************************************************************\
* DrawThumb2
*
*
*
* History:
* 01-03-94  FritzS   Chicago changes
\***************************************************************************/

void DrawThumb2(
    PWND pwnd,
    PSBCALC pSBCalc,
    HDC hdc,
    HBRUSH hbr,
    BOOL fVert,
    UINT wDisable)  /* Disabled flags for the scroll bar */
{
    int    *pLength;
    int    *pWidth;
    RECT   rcSB;
    PSBTRACK pSBTrack;

    //
    // Bail out if the scrollbar has an empty rect
    //
    if ((pSBCalc->pxTop >= pSBCalc->pxBottom) || (pSBCalc->pxLeft >= pSBCalc->pxRight))
        return;
    pLength = (LPINT)&rcSB;
    if (fVert)
        pWidth = pLength++;
    else
        pWidth  = pLength + 1;

    pWidth[0] = pSBCalc->pxLeft;
    pWidth[2] = pSBCalc->pxRight;

    /*
     * If both scroll bar arrows are disabled, then we should not draw
     * the thumb.  So, quit now!
     */
    if (((wDisable & LTUPFLAG) && (wDisable & RTDNFLAG)) ||
        ((pSBCalc->pxDownArrow - pSBCalc->pxUpArrow) < pSBCalc->cpxThumb)) {
        pLength[0] = pSBCalc->pxUpArrow;
        pLength[2] = pSBCalc->pxDownArrow;

        DrawGroove(hdc, hbr, &rcSB, fVert);
        return;
    }

    if (pSBCalc->pxUpArrow < pSBCalc->pxThumbTop) {
        // Fill in space above Thumb
        pLength[0] = pSBCalc->pxUpArrow;
        pLength[2] = pSBCalc->pxThumbTop;

        DrawGroove(hdc, hbr, &rcSB, fVert);
    }

    if (pSBCalc->pxThumbBottom < pSBCalc->pxDownArrow) {
        // Fill in space below Thumb
        pLength[0] = pSBCalc->pxThumbBottom;
        pLength[2] = pSBCalc->pxDownArrow;

        DrawGroove(hdc, hbr, &rcSB, fVert);
    }

    //
    // Draw elevator
    //
    pLength[0] = pSBCalc->pxThumbTop;
    pLength[2] = pSBCalc->pxThumbBottom;

    // Not soft!
    DrawPushButton(hdc, &rcSB, 0, 0);

    /*
     * If we're tracking a page scroll, then we've obliterated the hilite.
     * We need to correct the hiliting rectangle, and rehilite it.
     */
    pSBTrack = PWNDTOPSBTRACK(pwnd);

    if (pSBTrack && (pSBTrack->cmdSB == SB_PAGEUP || pSBTrack->cmdSB == SB_PAGEDOWN) &&
            (pwnd == pSBTrack->spwndTrack) &&
            (BOOL)pSBTrack->fTrackVert == fVert) {

        if (pSBTrack->fTrackRecalc) {
            RecalcTrackRect(pSBTrack);
            pSBTrack->fTrackRecalc = FALSE;
        }

        pLength = (int *)&pSBTrack->rcTrack;

        if (fVert)
            pLength++;

        if (pSBTrack->cmdSB == SB_PAGEUP)
            pLength[2] = pSBCalc->pxThumbTop;
        else
            pLength[0] = pSBCalc->pxThumbBottom;

        if (pLength[0] < pLength[2])
            InvertRect(hdc, &pSBTrack->rcTrack);
    }
}

/***************************************************************************\
* xxxDrawSB2
*
*
*
* History:
\***************************************************************************/

void xxxDrawSB2(
    PWND pwnd,
    PSBCALC pSBCalc,
    HDC hdc,
    BOOL fVert,
    UINT wDisable)
{

    int     cLength;
    int     cWidth;
    int     *pwX;
    int     *pwY;
    HBRUSH hbr;
    HBRUSH hbrSave;
    int cpxArrow;
    RECT    rc, rcSB;
    COLORREF crText, crBk;

    CheckLock(pwnd);

    cLength = (pSBCalc->pxBottom - pSBCalc->pxTop) / 2;
    cWidth = (pSBCalc->pxRight - pSBCalc->pxLeft);

    if ((cLength <= 0) || (cWidth <= 0)) {
        return;
    }
    if (fVert)
        cpxArrow = SYSMET(CYVSCROLL);
    else
        cpxArrow = SYSMET(CXHSCROLL);

    /*
     * Save background and DC color, since they get changed in
     * xxxGetColorObjects. Restore before we return.
     */
    crBk = GreGetBkColor(hdc);
    crText = GreGetTextColor(hdc);

    hbr = xxxGetColorObjects(pwnd, hdc);

    if (cLength > cpxArrow)
        cLength = cpxArrow;
    pwX = (int *)&rcSB;
    pwY = pwX + 1;
    if (!fVert)
        pwX = pwY--;

    pwX[0] = pSBCalc->pxLeft;
    pwY[0] = pSBCalc->pxTop;
    pwX[2] = pSBCalc->pxRight;
    pwY[2] = pSBCalc->pxBottom;

    hbrSave = GreSelectBrush(hdc, SYSHBR(BTNTEXT));

    //
    // BOGUS
    // Draw scrollbar arrows as disabled if the scrollbar itself is
    // disabled OR if the window it is a part of is disabled?
    //
    if (fVert) {
        if ((cLength == SYSMET(CYVSCROLL)) && (cWidth == SYSMET(CXVSCROLL))) {
            BitBltSysBmp(hdc, rcSB.left, rcSB.top, (wDisable & LTUPFLAG) ? OBI_UPARROW_I : OBI_UPARROW);
            BitBltSysBmp(hdc, rcSB.left, rcSB.bottom - cLength, (wDisable & RTDNFLAG) ? OBI_DNARROW_I : OBI_DNARROW);
        } else {
            CopyRect(&rc, &rcSB);
            rc.bottom = rc.top + cLength;
            DrawFrameControl(hdc, &rc, DFC_SCROLL,
                DFCS_SCROLLUP | ((wDisable & LTUPFLAG) ? DFCS_INACTIVE : 0));

            rc.bottom = rcSB.bottom;
            rc.top = rcSB.bottom - cLength;
            DrawFrameControl(hdc, &rc, DFC_SCROLL,
                DFCS_SCROLLDOWN | ((wDisable & RTDNFLAG) ? DFCS_INACTIVE : 0));
        }
    } else {
        if ((cLength == SYSMET(CXHSCROLL)) && (cWidth == SYSMET(CYHSCROLL))) {
            BitBltSysBmp(hdc, rcSB.left, rcSB.top, (wDisable & LTUPFLAG) ? OBI_LFARROW_I : OBI_LFARROW);
            BitBltSysBmp(hdc, rcSB.right - cLength, rcSB.top, (wDisable & RTDNFLAG) ? OBI_RGARROW_I : OBI_RGARROW);
        } else {
            CopyRect(&rc, &rcSB);
            rc.right = rc.left + cLength;
            DrawFrameControl(hdc, &rc, DFC_SCROLL,
                DFCS_SCROLLLEFT | ((wDisable & LTUPFLAG) ? DFCS_INACTIVE : 0));

            rc.right = rcSB.right;
            rc.left = rcSB.right - cLength;
            DrawFrameControl(hdc, &rc, DFC_SCROLL,
                DFCS_SCROLLRIGHT | ((wDisable & RTDNFLAG) ? DFCS_INACTIVE : 0));
        }
    }

    hbrSave = GreSelectBrush(hdc, hbrSave);
    DrawThumb2(pwnd, pSBCalc, hdc, hbr, fVert, wDisable);
    GreSelectBrush(hdc, hbrSave);

    GreSetBkColor(hdc, crBk);
    GreSetTextColor(hdc, crText);
}

/***************************************************************************\
* zzzSetSBCaretPos
*
*
*
* History:
\***************************************************************************/

void zzzSetSBCaretPos(
    PSBWND psbwnd)
{

    if ((PWND)psbwnd == PtiCurrent()->pq->spwndFocus) {
        zzzSetCaretPos((psbwnd->fVert ? psbwnd->SBCalc.pxLeft : psbwnd->SBCalc.pxThumbTop) + SYSMET(CXEDGE),
                (psbwnd->fVert ? psbwnd->SBCalc.pxThumbTop : psbwnd->SBCalc.pxLeft) + SYSMET(CYEDGE));
    }
}

/***************************************************************************\
* CalcSBStuff2
*
*
*
* History:
\***************************************************************************/

void CalcSBStuff2(
    PSBCALC  pSBCalc,
    LPRECT lprc,
    CONST PSBDATA pw,
    BOOL fVert)
{
    int cpx;
    DWORD dwRange;
    int denom;

    if (fVert) {
        pSBCalc->pxTop = lprc->top;
        pSBCalc->pxBottom = lprc->bottom;
        pSBCalc->pxLeft = lprc->left;
        pSBCalc->pxRight = lprc->right;
        pSBCalc->cpxThumb = SYSMET(CYVSCROLL);
    } else {

        /*
         * For horiz scroll bars, "left" & "right" are "top" and "bottom",
         * and vice versa.
         */
        pSBCalc->pxTop = lprc->left;
        pSBCalc->pxBottom = lprc->right;
        pSBCalc->pxLeft = lprc->top;
        pSBCalc->pxRight = lprc->bottom;
        pSBCalc->cpxThumb = SYSMET(CXHSCROLL);
    }

    pSBCalc->pos = pw->pos;
    pSBCalc->page = pw->page;
    pSBCalc->posMin = pw->posMin;
    pSBCalc->posMax = pw->posMax;

    dwRange = ((DWORD)(pSBCalc->posMax - pSBCalc->posMin)) + 1;

    //
    // For the case of short scroll bars that don't have enough
    // room to fit the full-sized up and down arrows, shorten
    // their sizes to make 'em fit
    //
    cpx = min((pSBCalc->pxBottom - pSBCalc->pxTop) / 2, pSBCalc->cpxThumb);

    pSBCalc->pxUpArrow   = pSBCalc->pxTop    + cpx;
    pSBCalc->pxDownArrow = pSBCalc->pxBottom - cpx;

    if ((pw->page != 0) && (dwRange != 0)) {
        // JEFFBOG -- This is the one and only place where we should
        // see 'range'.  Elsewhere it should be 'range - page'.

        /*
         * The minimun thumb size used to depend on the frame/edge metrics.
         * People that increase the scrollbar width/height expect the minimun
         *  to grow with proportianally. So NT5 bases the minimun on
         *  CXH/YVSCROLL, which is set by default in cpxThumb.
         */
        /*
         * i is used to keep the macro "max" from executing EngMulDiv twice.
         */
        int i = EngMulDiv(pSBCalc->pxDownArrow - pSBCalc->pxUpArrow,
                                             pw->page, dwRange);
        pSBCalc->cpxThumb = max(pSBCalc->cpxThumb / 2, i);
    }

    pSBCalc->pxMin = pSBCalc->pxTop + cpx;
    pSBCalc->cpx = pSBCalc->pxBottom - cpx - pSBCalc->cpxThumb - pSBCalc->pxMin;

    denom = dwRange - (pw->page ? pw->page : 1);
    if (denom)
        pSBCalc->pxThumbTop = EngMulDiv(pw->pos - pw->posMin,
            pSBCalc->cpx, denom) +
            pSBCalc->pxMin;
    else
        pSBCalc->pxThumbTop = pSBCalc->pxMin - 1;

    pSBCalc->pxThumbBottom = pSBCalc->pxThumbTop + pSBCalc->cpxThumb;

}

/***************************************************************************\
* SBCtlSetup
*
*
*
* History:
\***************************************************************************/

void SBCtlSetup(
    PSBWND psbwnd)
{
    RECT rc;

    GetRect((PWND)psbwnd, &rc, GRECT_CLIENT | GRECT_CLIENTCOORDS);
    CalcSBStuff2(&psbwnd->SBCalc, &rc, (PSBDATA)&psbwnd->SBCalc, psbwnd->fVert);
}

/***************************************************************************\
* HotTrackSB
*
\***************************************************************************/

#ifdef COLOR_HOTTRACKING

DWORD GetTrackFlags(int ht, BOOL fDraw)
{
    if (fDraw) {
        switch(ht) {
        case HTSCROLLUP:
        case HTSCROLLUPPAGE:
            return LTUPFLAG;

        case HTSCROLLDOWN:
        case HTSCROLLDOWNPAGE:
            return RTDNFLAG;

        case HTSCROLLTHUMB:
            return LTUPFLAG | RTDNFLAG;

        default:
            return 0;
        }
    } else {
        return 0;
    }
}

BOOL xxxHotTrackSB(PWND pwnd, int htEx, BOOL fDraw)
{
    SBCALC SBCalc;
    HDC  hdc;
    BOOL fVert = HIWORD(htEx);
    int ht = LOWORD(htEx);
    DWORD dwTrack = GetTrackFlags(ht, fDraw);

    CheckLock(pwnd);

    /*
     * xxxDrawSB2 does not callback or leave the critical section when it's
     * not a SB control and the window belongs to a different thread. It
     * calls xxxDefWindowProc which simply returns the brush color.
     */
    CalcSBStuff(pwnd, &SBCalc, fVert);
    hdc = _GetDCEx(pwnd, NULL, DCX_WINDOW | DCX_USESTYLE | DCX_CACHE);
    xxxDrawSB2(pwnd, &SBCalc, hdc, fVert, GetWndSBDisableFlags(pwnd, fVert), dwTrack);
    _ReleaseDC(hdc);
    return TRUE;
}

void xxxHotTrackSBCtl(PSBWND psbwnd, int ht, BOOL fDraw)
{
    DWORD dwTrack = GetTrackFlags(ht, fDraw);
    HDC hdc;

    CheckLock(psbwnd);

    SBCtlSetup(psbwnd);
    hdc = _GetDCEx((PWND)psbwnd, NULL, DCX_WINDOW | DCX_USESTYLE | DCX_CACHE);
    xxxDrawSB2((PWND)psbwnd, &psbwnd->SBCalc, hdc, psbwnd->fVert, psbwnd->wDisableFlags, dwTrack);
    _ReleaseDC(hdc);
}
#endif // COLOR_HOTTRACKING

BOOL SBSetParms(PSBDATA pw, LPSCROLLINFO lpsi, LPBOOL lpfScroll, LPLONG lplres)
{
    // pass the struct because we modify the struct but don't want that
    // modified version to get back to the calling app

    BOOL fChanged = FALSE;

    if (lpsi->fMask & SIF_RETURNOLDPOS)
        // save previous position
        *lplres = pw->pos;

    if (lpsi->fMask & SIF_RANGE) {
        // if the range MAX is below the range MIN -- then treat is as a
        // zero range starting at the range MIN.
        if (lpsi->nMax < lpsi->nMin)
            lpsi->nMax = lpsi->nMin;

        if ((pw->posMin != lpsi->nMin) || (pw->posMax != lpsi->nMax)) {
            pw->posMin = lpsi->nMin;
            pw->posMax = lpsi->nMax;

            if (!(lpsi->fMask & SIF_PAGE)) {
                lpsi->fMask |= SIF_PAGE;
                lpsi->nPage = pw->page;
            }

            if (!(lpsi->fMask & SIF_POS)) {
                lpsi->fMask |= SIF_POS;
                lpsi->nPos = pw->pos;
            }

            fChanged = TRUE;
        }
    }

    if (lpsi->fMask & SIF_PAGE) {
        DWORD dwMaxPage = (DWORD) abs(pw->posMax - pw->posMin) + 1;

        // Clip page to 0, posMax - posMin + 1

        if (lpsi->nPage > dwMaxPage)
            lpsi->nPage = dwMaxPage;


        if (pw->page != (int)(lpsi->nPage)) {
            pw->page = lpsi->nPage;

            if (!(lpsi->fMask & SIF_POS)) {
                lpsi->fMask |= SIF_POS;
                lpsi->nPos = pw->pos;
            }

            fChanged = TRUE;
        }
    }

    if (lpsi->fMask & SIF_POS) {
        int iMaxPos = pw->posMax - ((pw->page) ? pw->page - 1 : 0);
        // Clip pos to posMin, posMax - (page - 1).

        if (lpsi->nPos < pw->posMin)
            lpsi->nPos = pw->posMin;
        else if (lpsi->nPos > iMaxPos)
            lpsi->nPos = iMaxPos;


        if (pw->pos != lpsi->nPos) {
            pw->pos = lpsi->nPos;
            fChanged = TRUE;
        }
    }

    if (!(lpsi->fMask & SIF_RETURNOLDPOS)) {
        // Return the new position
        *lplres = pw->pos;
    }

    /*
     * This was added by JimA as Cairo merge but will conflict
     * with the documentation for SetScrollPos
     */
/*
    else if (*lplres == pw->pos)
        *lplres = 0;
*/
    if (lpsi->fMask & SIF_RANGE) {
        if (*lpfScroll = (pw->posMin != pw->posMax))
            goto checkPage;
    } else if (lpsi->fMask & SIF_PAGE)
checkPage:
        *lpfScroll = (pw->page <= (pw->posMax - pw->posMin));

    return fChanged;
}


/***************************************************************************\
* CalcSBStuff
*
*
*
* History:
\***************************************************************************/

void CalcSBStuff(
    PWND pwnd,
    PSBCALC pSBCalc,
    BOOL fVert)
{
    RECT    rcT;
    RECT    rcClient;
#ifdef USE_MIRRORING
    int     cx, iTemp;
#endif

    //
    // Get client rectangle.  We know that scrollbars always align to the right
    // and to the bottom of the client area.
    //
    GetRect(pwnd, &rcClient, GRECT_CLIENT | GRECT_WINDOWCOORDS);
#ifdef USE_MIRRORING
    if (TestWF(pwnd, WEFLAYOUTRTL)) {
        cx             = pwnd->rcWindow.right - pwnd->rcWindow.left;
        iTemp          = rcClient.left;
        rcClient.left  = cx - rcClient.right;
        rcClient.right = cx - iTemp;
    }
#endif

    if (fVert) {
         // Only add on space if vertical scrollbar is really there.
        if (TestWF(pwnd, WEFLEFTSCROLL)) {
            rcT.right = rcT.left = rcClient.left;
            if (TestWF(pwnd, WFVPRESENT))
                rcT.left -= SYSMET(CXVSCROLL);
        } else {
            rcT.right = rcT.left = rcClient.right;
            if (TestWF(pwnd, WFVPRESENT))
                rcT.right += SYSMET(CXVSCROLL);
        }

        rcT.top = rcClient.top;
        rcT.bottom = rcClient.bottom;
    } else {
        // Only add on space if horizontal scrollbar is really there.
        rcT.bottom = rcT.top = rcClient.bottom;
        if (TestWF(pwnd, WFHPRESENT))
            rcT.bottom += SYSMET(CYHSCROLL);

        rcT.left = rcClient.left;
        rcT.right = rcClient.right;
    }

    // If InitPwSB stuff fails (due to our heap being full) there isn't anything reasonable
    // we can do here, so just let it go through.  We won't fault but the scrollbar won't work
    // properly either...
    if (_InitPwSB(pwnd))
        CalcSBStuff2(pSBCalc, &rcT, (fVert) ? &pwnd->pSBInfo->Vert :  &pwnd->pSBInfo->Horz, fVert);

}

/***************************************************************************\
*
*  DrawCtlThumb()
*
\***************************************************************************/
void DrawCtlThumb(PSBWND psb)
{
    HBRUSH  hbr, hbrSave;
    HDC     hdc = (HDC) _GetWindowDC((PWND) psb);

    SBCtlSetup(psb);

    hbrSave = GreSelectBrush(hdc, hbr = xxxGetColorObjects((PWND) psb, hdc));

    DrawThumb2((PWND) psb, &psb->SBCalc, hdc, hbr, psb->fVert, psb->wDisableFlags);

    GreSelectBrush(hdc, hbrSave);
    _ReleaseDC(hdc);
}


/***************************************************************************\
* xxxDrawThumb
*
*
*
* History:
\***************************************************************************/

void xxxDrawThumb(
    PWND pwnd,
    PSBCALC pSBCalc,
    BOOL fVert)
{
    HBRUSH hbr, hbrSave;
    HDC hdc;
    UINT wDisableFlags;
    SBCALC SBCalc;

    CheckLock(pwnd);

    if (!pSBCalc) pSBCalc = &SBCalc;
    hdc = (HDC)_GetWindowDC(pwnd);

    CalcSBStuff(pwnd, &SBCalc, fVert);
    wDisableFlags = GetWndSBDisableFlags(pwnd, fVert);

    hbrSave = GreSelectBrush(hdc, hbr = xxxGetColorObjects(pwnd, hdc));

    DrawThumb2(pwnd, &SBCalc, hdc, hbr, fVert, wDisableFlags);

    GreSelectBrush(hdc, hbrSave);

    /*
     * Won't hurt even if DC is already released (which happens automatically
     * if window is destroyed during xxxSelectColorObjects)
     */
    _ReleaseDC(hdc);
}

/***************************************************************************\
* xxxSetScrollBar
*
*
*
* History:
\***************************************************************************/

LONG xxxSetScrollBar(
    PWND pwnd,
    int code,
    LPSCROLLINFO lpsi,
    BOOL fRedraw)
{
    BOOL        fVert;
    PSBDATA pw;
    PSBINFO pSBInfo;
    BOOL fOldScroll;
    BOOL fScroll;
    WORD        wfScroll;
    LONG     lres;
    BOOL        fNewScroll;

    CheckLock(pwnd);
    UserAssert(IsWinEventNotifyDeferredOK());

    if (fRedraw)
        // window must be visible to redraw
        fRedraw = IsVisible(pwnd);

    if (code == SB_CTL)
#ifdef FE_SB // xxxSetScrollBar()
        // scroll bar control; send the control a message
        if(GETPTI(pwnd)->TIF_flags & TIF_16BIT) {
            //
            // If the target application is 16bit apps, we don't pass win40's message.
            // This fix for Ichitaro v6.3. It eats the message. It never forwards
            // the un-processed messages to original windows procedure via
            // CallWindowProc().
            //
            // Is this from xxxSetScrollPos() ?
            if(lpsi->fMask == (SIF_POS|SIF_RETURNOLDPOS)) {
                return (int)xxxSendMessage(pwnd, SBM_SETPOS, lpsi->nPos, fRedraw);
            // Is this from xxxSetScrollRange() ?
            } else if(lpsi->fMask == SIF_RANGE) {
                xxxSendMessage(pwnd, SBM_SETRANGE, lpsi->nMin, lpsi->nMax);
                return TRUE;
            // Others...
            } else {
                return (LONG)xxxSendMessage(pwnd, SBM_SETSCROLLINFO, (WPARAM) fRedraw, (LPARAM) lpsi);
            }
        } else {
            return (LONG)xxxSendMessage(pwnd, SBM_SETSCROLLINFO, (WPARAM) fRedraw, (LPARAM) lpsi);
        }
#else
        // scroll bar control; send the control a message
        return (LONG)xxxSendMessage(pwnd, SBM_SETSCROLLINFO, (WPARAM) fRedraw, (LPARAM) lpsi);
#endif // FE_SB

    fVert = (code != SB_HORZ);

    wfScroll = (fVert) ? WFVSCROLL : WFHSCROLL;

    fScroll = fOldScroll = (TestWF(pwnd, wfScroll)) ? TRUE : FALSE;

    /*
     * Don't do anything if we're setting position of a nonexistent scroll bar.
     */
    if (!(lpsi->fMask & SIF_RANGE) && !fOldScroll && (pwnd->pSBInfo == NULL)) {
        RIPERR0(ERROR_NO_SCROLLBARS, RIP_VERBOSE, "");
        return 0;
    }

    if (fNewScroll = !(pSBInfo = pwnd->pSBInfo)) {
        if ((pSBInfo = _InitPwSB(pwnd)) == NULL)
            return 0;
    }

    pw = (fVert) ? &(pSBInfo->Vert) : &(pSBInfo->Horz);

    if (!SBSetParms(pw, lpsi, &fScroll, &lres) && !fNewScroll) {
        // no change -- but if REDRAW is specified and there's a scrollbar,
        // redraw the thumb
        if (fOldScroll && fRedraw)
            goto redrawAfterSet;

        return lres;
    }

    ClrWF(pwnd, wfScroll);

    if (fScroll)
        SetWF(pwnd, wfScroll);
    else if (!TestWF(pwnd, (WFHSCROLL | WFVSCROLL))) {
        // if neither scroll bar is set and both ranges are 0, then free up the
        // scroll info

        pSBInfo = pwnd->pSBInfo;

        if ((pSBInfo->Horz.posMin == pSBInfo->Horz.posMax) &&
            (pSBInfo->Vert.posMin == pSBInfo->Vert.posMax)) {
            DesktopFree(pwnd->head.rpdesk, (HANDLE)(pwnd->pSBInfo));
            pwnd->pSBInfo = NULL;
        }
    }

    if (lpsi->fMask & SIF_DISABLENOSCROLL) {
        if (fOldScroll) {
            SetWF(pwnd, wfScroll);
            xxxEnableWndSBArrows(pwnd, code, (fScroll) ? ESB_ENABLE_BOTH : ESB_DISABLE_BOTH);
        }
    } else if (fOldScroll ^ fScroll) {
        PSBTRACK pSBTrack = PWNDTOPSBTRACK(pwnd);
        if (pSBTrack && (pwnd == pSBTrack->spwndTrack)) {
            pSBTrack->fTrackRecalc = TRUE;
        }
        xxxRedrawFrame(pwnd);
        // Note: after xxx, pSBTrack may no longer be valid (but we return now)
        return lres;
    }

    if (fScroll && fRedraw && (fVert ? TestWF(pwnd, WFVPRESENT) : TestWF(pwnd, WFHPRESENT))) {
        PSBTRACK pSBTrack;
redrawAfterSet:
        if (FWINABLE()) {
            xxxWindowEvent(EVENT_OBJECT_VALUECHANGE, pwnd, (fVert ? OBJID_VSCROLL : OBJID_HSCROLL),
                    INDEX_SCROLLBAR_SELF, WEF_USEPWNDTHREAD);
        }
        pSBTrack = PWNDTOPSBTRACK(pwnd);
        // Bail out if the caller is trying to change the position of
        // a scrollbar that is in the middle of tracking.  We'll hose
        // TrackThumb() otherwise.

        if (pSBTrack && (pwnd == pSBTrack->spwndTrack) &&
                ((BOOL)(pSBTrack->fTrackVert) == fVert) &&
                (pSBTrack->xxxpfnSB == xxxTrackThumb)) {
            return lres;
        }

        xxxDrawThumb(pwnd, NULL, fVert);
        // Note: after xxx, pSBTrack may no longer be valid (but we return now)
    }

    return lres;
}



/***************************************************************************\
* xxxDrawScrollBar
*
*
*
* History:
\***************************************************************************/

void xxxDrawScrollBar(
    PWND pwnd,
    HDC hdc,
    BOOL fVert)
{
    SBCALC SBCalc;
    PSBCALC pSBCalc;
    PSBTRACK pSBTrack = PWNDTOPSBTRACK(pwnd);

    CheckLock(pwnd);
    if (pSBTrack && (pwnd == pSBTrack->spwndTrack) && (pSBTrack->fCtlSB == FALSE)
         && (fVert == (BOOL)pSBTrack->fTrackVert)) {
        pSBCalc = pSBTrack->pSBCalc;
    } else {
        pSBCalc = &SBCalc;
    }
    CalcSBStuff(pwnd, pSBCalc, fVert);

    xxxDrawSB2(pwnd, pSBCalc, hdc, fVert, GetWndSBDisableFlags(pwnd, fVert));
}

/***************************************************************************\
* SBPosFromPx
*
* Compute scroll bar position from pixel location
*
* History:
\***************************************************************************/

int SBPosFromPx(
    PSBCALC  pSBCalc,
    int px)
{
    if (px < pSBCalc->pxMin) {
        return pSBCalc->posMin;
    }
    if (px >= pSBCalc->pxMin + pSBCalc->cpx) {
        return (pSBCalc->posMax - (pSBCalc->page ? pSBCalc->page - 1 : 0));
    }
    if (pSBCalc->cpx)
        return (pSBCalc->posMin + EngMulDiv(pSBCalc->posMax - pSBCalc->posMin -
            (pSBCalc->page ? pSBCalc->page - 1 : 0),
            px - pSBCalc->pxMin, pSBCalc->cpx));
    else
        return (pSBCalc->posMin - 1);
}

/***************************************************************************\
* InvertScrollHilite
*
*
*
* History:
\***************************************************************************/

void InvertScrollHilite(
    PWND pwnd,
    PSBTRACK pSBTrack)
{
    HDC hdc;

    /*
     * Don't invert if the thumb is all the way at the top or bottom
     * or you will end up inverting the line between the arrow and the thumb.
     */
    if (!IsRectEmpty(&pSBTrack->rcTrack)) {
        if (pSBTrack->fTrackRecalc) {
            RecalcTrackRect(pSBTrack);
            pSBTrack->fTrackRecalc = FALSE;
        }

        hdc = (HDC)_GetWindowDC(pwnd);
        InvertRect(hdc, &pSBTrack->rcTrack);
        _ReleaseDC(hdc);
    }
}

/***************************************************************************\
* xxxDoScroll
*
* Sends scroll notification to the scroll bar owner
*
* History:
\***************************************************************************/

void xxxDoScroll(
    PWND pwnd,
    PWND pwndNotify,
    int cmd,
    int pos,
    BOOL fVert
)
{
    TL tlpwndNotify;

    /*
     * Special case!!!! this routine is always passed pwnds that are
     * not thread locked, so they need to be thread locked here.  The
     * callers always know that by the time DoScroll() returns,
     * pwnd and pwndNotify could be invalid.
     */
    ThreadLock(pwndNotify, &tlpwndNotify);
    xxxSendMessage(pwndNotify, (UINT)(fVert ? WM_VSCROLL : WM_HSCROLL),
            MAKELONG(cmd, pos), (LPARAM)HW(pwnd));

    ThreadUnlock(&tlpwndNotify);
}

// -------------------------------------------------------------------------
//
//  CheckScrollRecalc()
//
// -------------------------------------------------------------------------
//void CheckScrollRecalc(PWND pwnd, PSBSTATE pSBState, PSBCALC pSBCalc)
//{
//    if ((pSBState->pwndCalc != pwnd) || ((pSBState->nBar != SB_CTL) && (pSBState->nBar != ((pSBState->fVertSB) ? SB_VERT : SB_HORZ))))
//    {
//        // Calculate SB stuff based on whether it's a control or in a window
//        if (pSBState->fCtlSB)
//            SBCtlSetup((PSBWND) pwnd);
//        else
//            CalcSBStuff(pwnd, pSBCalc, pSBState->fVertSB);
//    }
//}


/***************************************************************************\
* xxxMoveThumb
*
* History:
\***************************************************************************/

void xxxMoveThumb(
    PWND pwnd,
    PSBCALC  pSBCalc,
    int px)
{
    HBRUSH  hbr, hbrSave;
    HDC     hdc;
    PSBTRACK pSBTrack;

    CheckLock(pwnd);

    pSBTrack = PWNDTOPSBTRACK(pwnd);

    if ((pSBTrack == NULL) || (px == pSBTrack->pxOld))
        return;

pxReCalc:

    pSBTrack->posNew = SBPosFromPx(pSBCalc, px);

    /* Tentative position changed -- notify the guy. */
    if (pSBTrack->posNew != pSBTrack->posOld) {
        if (pSBTrack->spwndSBNotify != NULL) {
            xxxDoScroll(pSBTrack->spwndSB, pSBTrack->spwndSBNotify, SB_THUMBTRACK, pSBTrack->posNew, pSBTrack->fTrackVert
            );

        }
        // After xxxDoScroll, re-evaluate pSBTrack
        REEVALUATE_PSBTRACK(pSBTrack, pwnd, "xxxMoveThumb(1)");
        if ((pSBTrack == NULL) || (pSBTrack->xxxpfnSB == NULL))
            return;

        pSBTrack->posOld = pSBTrack->posNew;

        //
        // Anything can happen after the SendMessage above!
        // Make sure that the SBINFO structure contains data for the
        // window being tracked -- if not, recalculate data in SBINFO
        //
//        CheckScrollRecalc(pwnd, pSBState, pSBCalc);
        // when we yield, our range can get messed with
        // so make sure we handle this

        if (px >= pSBCalc->pxMin + pSBCalc->cpx)
        {
            px = pSBCalc->pxMin + pSBCalc->cpx;
            goto pxReCalc;
        }

    }

    hdc = _GetWindowDC(pwnd);

    pSBCalc->pxThumbTop = px;
    pSBCalc->pxThumbBottom = pSBCalc->pxThumbTop + pSBCalc->cpxThumb;

    // at this point, the disable flags are always going to be 0 --
    // we're in the middle of tracking.
    hbrSave = GreSelectBrush(hdc, hbr = xxxGetColorObjects(pwnd, hdc));

    // After xxxGetColorObjects, re-evaluate pSBTrack
    REEVALUATE_PSBTRACK(pSBTrack, pwnd, "xxxMoveThumb(2)");
    if (pSBTrack == NULL) {
        RIPMSG1(RIP_ERROR, "Did we use to leak hdc %#p?", hdc) ;
        _ReleaseDC(hdc);
        return;
    }
    DrawThumb2(pwnd, pSBCalc, hdc, hbr, pSBTrack->fTrackVert, 0);
    GreSelectBrush(hdc, hbrSave);
    _ReleaseDC(hdc);

    pSBTrack->pxOld = px;
}

/***************************************************************************\
* zzzDrawInvertScrollArea
*
*
*
* History:
\***************************************************************************/

void zzzDrawInvertScrollArea(
    PWND pwnd,
    PSBTRACK pSBTrack,
    BOOL fHit,
    UINT cmd)
{
    HDC hdc;
    RECT rcTemp;
    int cx, cy;
    UINT bm;

    if ((cmd != SB_LINEUP) && (cmd != SB_LINEDOWN)) {
        // not hitting on arrow -- just invert the area and return
        InvertScrollHilite(pwnd, pSBTrack);

        if (cmd == SB_PAGEUP) {
            if (fHit)
                SetWF(pwnd, WFPAGEUPBUTTONDOWN);
            else
                ClrWF(pwnd, WFPAGEUPBUTTONDOWN);
        } else {
            if (fHit)
                SetWF(pwnd, WFPAGEDNBUTTONDOWN);
            else
                ClrWF(pwnd, WFPAGEDNBUTTONDOWN);
        }

        if (FWINABLE()) {
            zzzWindowEvent(EVENT_OBJECT_STATECHANGE, pwnd,
                    (pSBTrack->fCtlSB ? OBJID_CLIENT : (pSBTrack->fTrackVert ? OBJID_VSCROLL : OBJID_HSCROLL)),
                    ((cmd == SB_PAGEUP) ? INDEX_SCROLLBAR_UPPAGE : INDEX_SCROLLBAR_DOWNPAGE),
                    WEF_USEPWNDTHREAD);
            // Note: after zzz, pSBTrack may no longer be valid (but we return now)
        }
        return;
    }

    if (pSBTrack->fTrackRecalc) {
        RecalcTrackRect(pSBTrack);
        pSBTrack->fTrackRecalc = FALSE;
    }

    CopyRect(&rcTemp, &pSBTrack->rcTrack);

    hdc = _GetWindowDC(pwnd);

    if (pSBTrack->fTrackVert) {
        cx = SYSMET(CXVSCROLL);
        cy = SYSMET(CYVSCROLL);
    } else {
        cx = SYSMET(CXHSCROLL);
        cy = SYSMET(CYHSCROLL);
    }

    if ((cx == (rcTemp.right - rcTemp.left)) &&
        (cy == (rcTemp.bottom - rcTemp.top))) {
        if (cmd == SB_LINEUP)
            bm = (pSBTrack->fTrackVert) ? OBI_UPARROW : OBI_LFARROW;
        else // SB_LINEDOWN
            bm = (pSBTrack->fTrackVert) ? OBI_DNARROW : OBI_RGARROW;

        if (fHit)
            bm += DOBI_PUSHED;

        BitBltSysBmp(hdc, rcTemp.left, rcTemp.top, bm);
    } else {
        DrawFrameControl(hdc, &rcTemp, DFC_SCROLL,
            ((pSBTrack->fTrackVert) ? DFCS_SCROLLVERT : DFCS_SCROLLHORZ) |
            ((fHit) ? DFCS_PUSHED | DFCS_FLAT : 0) |
            ((cmd == SB_LINEUP) ? DFCS_SCROLLMIN : DFCS_SCROLLMAX));
    }

    _ReleaseDC(hdc);


    if (cmd == SB_LINEUP) {
        if (fHit)
            SetWF(pwnd, WFLINEUPBUTTONDOWN);
        else
            ClrWF(pwnd, WFLINEUPBUTTONDOWN);
    } else {
        if (fHit)
            SetWF(pwnd, WFLINEDNBUTTONDOWN);
        else
            ClrWF(pwnd, WFLINEDNBUTTONDOWN);
    }

    if (FWINABLE()) {
        zzzWindowEvent(EVENT_OBJECT_STATECHANGE, pwnd,
                (pSBTrack->fCtlSB ? OBJID_CLIENT : (pSBTrack->fTrackVert ? OBJID_VSCROLL : OBJID_HSCROLL)),
                (cmd == SB_LINEUP ? INDEX_SCROLLBAR_UP : INDEX_SCROLLBAR_DOWN),
                WEF_USEPWNDTHREAD);
        // Note: after zzz, pSBTrack may no longer be valid (but we return now)
    }
}

/***************************************************************************\
* xxxEndScroll
*
*
*
* History:
\***************************************************************************/

void xxxEndScroll(
    PWND pwnd,
    BOOL fCancel)
{
    UINT oldcmd;
    PSBTRACK pSBTrack;
    CheckLock(pwnd);
    UserAssert(!IsWinEventNotifyDeferred());

    pSBTrack = PWNDTOPSBTRACK(pwnd);
    if (pSBTrack && PtiCurrent()->pq->spwndCapture == pwnd && pSBTrack->xxxpfnSB != NULL) {

        oldcmd = pSBTrack->cmdSB;
        pSBTrack->cmdSB = 0;
        xxxReleaseCapture();

        // After xxxReleaseCapture, revalidate pSBTrack
        RETURN_IF_PSBTRACK_INVALID(pSBTrack, pwnd);

        if (pSBTrack->xxxpfnSB == xxxTrackThumb) {

            if (fCancel) {
                pSBTrack->posOld = pSBTrack->pSBCalc->pos;
            }

            /*
             * DoScroll does thread locking on these two pwnds -
             * this is ok since they are not used after this
             * call.
             */
            if (pSBTrack->spwndSBNotify != NULL) {
                xxxDoScroll(pSBTrack->spwndSB, pSBTrack->spwndSBNotify,
                        SB_THUMBPOSITION, pSBTrack->posOld, pSBTrack->fTrackVert
                );
                // After xxxDoScroll, revalidate pSBTrack
                RETURN_IF_PSBTRACK_INVALID(pSBTrack, pwnd);
            }

            if (pSBTrack->fCtlSB) {
                DrawCtlThumb((PSBWND) pwnd);
            } else {
                xxxDrawThumb(pwnd, pSBTrack->pSBCalc, pSBTrack->fTrackVert);
                // Note: after xxx, pSBTrack may no longer be valid
            }

        } else if (pSBTrack->xxxpfnSB == xxxTrackBox) {
            DWORD lParam;
            POINT ptMsg;

            if (pSBTrack->hTimerSB != 0) {
                _KillSystemTimer(pwnd, IDSYS_SCROLL);
                pSBTrack->hTimerSB = 0;
            }
            lParam = _GetMessagePos();
#ifdef USE_MIRRORING
            if (TestWF(pwnd, WEFLAYOUTRTL)) {
                ptMsg.x = pwnd->rcWindow.right - GET_X_LPARAM(lParam);
            } else
#endif
            {
                ptMsg.x = GET_X_LPARAM(lParam) - pwnd->rcWindow.left;
            }
            ptMsg.y = GET_Y_LPARAM(lParam) - pwnd->rcWindow.top;
            if (PtInRect(&pSBTrack->rcTrack, ptMsg)) {
                zzzDrawInvertScrollArea(pwnd, pSBTrack, FALSE, oldcmd);
                // Note: after zzz, pSBTrack may no longer be valid
            }
        }

        /*
         * Always send SB_ENDSCROLL message.
         *
         * DoScroll does thread locking on these two pwnds -
         * this is ok since they are not used after this
         * call.
         */

        // After xxxDrawThumb or zzzDrawInvertScrollArea, revalidate pSBTrack
        RETURN_IF_PSBTRACK_INVALID(pSBTrack, pwnd);

        if (pSBTrack->spwndSBNotify != NULL) {
            xxxDoScroll(pSBTrack->spwndSB, pSBTrack->spwndSBNotify,
                    SB_ENDSCROLL, 0, pSBTrack->fTrackVert);
            // After xxxDoScroll, revalidate pSBTrack
            RETURN_IF_PSBTRACK_INVALID(pSBTrack, pwnd);
        }

        ClrWF(pwnd, WFSCROLLBUTTONDOWN);
        ClrWF(pwnd, WFVERTSCROLLTRACK);

        if (FWINABLE()) {
            xxxWindowEvent(EVENT_SYSTEM_SCROLLINGEND, pwnd,
                    (pSBTrack->fCtlSB ? OBJID_CLIENT :
                            (pSBTrack->fTrackVert ? OBJID_VSCROLL : OBJID_HSCROLL)),
                    INDEXID_CONTAINER, 0);
            // After xxxWindowEvent, revalidate pSBTrack
            RETURN_IF_PSBTRACK_INVALID(pSBTrack, pwnd);
        }

        /*
         * If this is a Scroll Bar Control, turn the caret back on.
         */
        if (pSBTrack->spwndSB != NULL) {
            zzzShowCaret(pSBTrack->spwndSB);
            // After zzz, revalidate pSBTrack
            RETURN_IF_PSBTRACK_INVALID(pSBTrack, pwnd);
        }


        pSBTrack->xxxpfnSB = NULL;

        /*
         * Unlock structure members so they are no longer holding down windows.
         */
        Unlock(&pSBTrack->spwndSB);
        Unlock(&pSBTrack->spwndSBNotify);
        Unlock(&pSBTrack->spwndTrack);
        UserFreePool(pSBTrack);
        PWNDTOPSBTRACK(pwnd) = NULL;
    }
}


/***************************************************************************\
* xxxContScroll
*
*
*
* History:
\***************************************************************************/

VOID xxxContScroll(
    PWND pwnd,
    UINT message,
    UINT_PTR ID,
    LPARAM lParam)
{
    LONG pt;
    PSBTRACK pSBTrack = PWNDTOPSBTRACK(pwnd);

    UNREFERENCED_PARAMETER(message);
    UNREFERENCED_PARAMETER(ID);
    UNREFERENCED_PARAMETER(lParam);

    if (pSBTrack == NULL)
        return;

    CheckLock(pwnd);

    pt = _GetMessagePos();
#ifdef USE_MIRRORING
    if (TestWF(pwnd, WEFLAYOUTRTL)) {
        pt = MAKELONG(pwnd->rcWindow.right - GET_X_LPARAM(pt), GET_Y_LPARAM(pt) - pwnd->rcWindow.top);
    } else
#endif
    {
        pt = MAKELONG( GET_X_LPARAM(pt) - pwnd->rcWindow.left, GET_Y_LPARAM(pt) - pwnd->rcWindow.top);
    }
    xxxTrackBox(pwnd, WM_NULL, 0, pt, NULL);
    // After xxxTrackBox, revalidate pSBTrack
    RETURN_IF_PSBTRACK_INVALID(pSBTrack, pwnd);

    if (pSBTrack->fHitOld) {
        pSBTrack->hTimerSB = _SetSystemTimer(pwnd, IDSYS_SCROLL,
                gpsi->dtScroll / 8, xxxContScroll);

        /*
         * DoScroll does thread locking on these two pwnds -
         * this is ok since they are not used after this
         * call.
         */
        if (pSBTrack->spwndSBNotify != NULL) {
            xxxDoScroll(pSBTrack->spwndSB, pSBTrack->spwndSBNotify,
                    pSBTrack->cmdSB, 0, pSBTrack->fTrackVert);
            // Note: after xxx, pSBTrack may no longer be valid (but we return now)
        }
    }

    return;
}

/***************************************************************************\
* xxxTrackBox
*
*
*
* History:
\***************************************************************************/

void xxxTrackBox(
    PWND pwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    PSBCALC pSBCalc)
{
    BOOL fHit;
    POINT ptHit;
    PSBTRACK pSBTrack = PWNDTOPSBTRACK(pwnd);
    int cmsTimer;

    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(pSBCalc);

    CheckLock(pwnd);
    UserAssert(IsWinEventNotifyDeferredOK());

    if (pSBTrack == NULL)
        return;

    if (message != WM_NULL && HIBYTE(message) != HIBYTE(WM_MOUSEFIRST))
        return;

    if (pSBTrack->fTrackRecalc) {
        RecalcTrackRect(pSBTrack);
        pSBTrack->fTrackRecalc = FALSE;
    }

    ptHit.x = GET_X_LPARAM(lParam);
    ptHit.y = GET_Y_LPARAM(lParam);
    fHit = PtInRect(&pSBTrack->rcTrack, ptHit);

    if (fHit != (BOOL)pSBTrack->fHitOld) {
        zzzDrawInvertScrollArea(pwnd, pSBTrack, fHit, pSBTrack->cmdSB);
        // After zzz, pSBTrack may no longer be valid
        RETURN_IF_PSBTRACK_INVALID(pSBTrack, pwnd);
    }

    cmsTimer = gpsi->dtScroll / 8;

    switch (message) {
    case WM_LBUTTONUP:
        xxxEndScroll(pwnd, FALSE);
        // Note: after xxx, pSBTrack may no longer be valid
        break;

    case WM_LBUTTONDOWN:
        pSBTrack->hTimerSB = 0;
        cmsTimer = gpsi->dtScroll;

        /*
         *** FALL THRU **
         */

    case WM_MOUSEMOVE:
        if (fHit && fHit != (BOOL)pSBTrack->fHitOld) {

            /*
             * We moved back into the normal rectangle: reset timer
             */
            pSBTrack->hTimerSB = _SetSystemTimer(pwnd, IDSYS_SCROLL,
                    cmsTimer, xxxContScroll);

            /*
             * DoScroll does thread locking on these two pwnds -
             * this is ok since they are not used after this
             * call.
             */
            if (pSBTrack->spwndSBNotify != NULL) {
                xxxDoScroll(pSBTrack->spwndSB, pSBTrack->spwndSBNotify,
                        pSBTrack->cmdSB, 0, pSBTrack->fTrackVert);
                // Note: after xxx, pSBTrack may no longer be valid
            }
        }
    }
    // After xxxDoScroll or xxxEndScroll, revalidate pSBTrack
    RETURN_IF_PSBTRACK_INVALID(pSBTrack, pwnd);
    pSBTrack->fHitOld = fHit;
}


/***************************************************************************\
* xxxTrackThumb
*
*
*
* History:
\***************************************************************************/

void xxxTrackThumb(
    PWND pwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    PSBCALC pSBCalc)
{
    int px;
    PSBTRACK pSBTrack = PWNDTOPSBTRACK(pwnd);
    POINT pt;

    UNREFERENCED_PARAMETER(wParam);

    CheckLock(pwnd);

    if (HIBYTE(message) != HIBYTE(WM_MOUSEFIRST))
        return;

    if (pSBTrack == NULL)
        return;

    // Make sure that the SBINFO structure contains data for the
    // window being tracked -- if not, recalculate data in SBINFO
//    CheckScrollRecalc(pwnd, pSBState, pSBCalc);
    if (pSBTrack->fTrackRecalc) {
        RecalcTrackRect(pSBTrack);
        pSBTrack->fTrackRecalc = FALSE;
    }


    pt.y = GET_Y_LPARAM(lParam);
    pt.x = GET_X_LPARAM(lParam);
    if (!PtInRect(&pSBTrack->rcTrack, pt))
        px = pSBCalc->pxStart;
    else {
        px = (pSBTrack->fTrackVert ? pt.y : pt.x) + pSBTrack->dpxThumb;
        if (px < pSBCalc->pxMin)
            px = pSBCalc->pxMin;
        else if (px >= pSBCalc->pxMin + pSBCalc->cpx)
            px = pSBCalc->pxMin + pSBCalc->cpx;
    }

    xxxMoveThumb(pwnd, pSBCalc, px);

    /*
     * We won't get the WM_LBUTTONUP message if we got here through
     * the scroll menu, so test the button state directly.
     */
    if (message == WM_LBUTTONUP || _GetKeyState(VK_LBUTTON) >= 0) {
        xxxEndScroll(pwnd, FALSE);
    }

}

/***************************************************************************\
* xxxSBTrackLoop
*
*
*
* History:
\***************************************************************************/

void xxxSBTrackLoop(
    PWND pwnd,
    LPARAM lParam,
    PSBCALC pSBCalc)
{
    MSG msg;
    UINT cmd;
    PTHREADINFO ptiCurrent;
    VOID (*xxxpfnSB)(PWND, UINT, WPARAM, LPARAM, PSBCALC);
    PSBTRACK pSBTrack;

    CheckLock(pwnd);
    UserAssert(IsWinEventNotifyDeferredOK());

    pSBTrack = PWNDTOPSBTRACK(pwnd);

    if ((pSBTrack == NULL) || (NULL == (xxxpfnSB = pSBTrack->xxxpfnSB)))
        // mode cancelled -- exit track loop
        return;

    if (pSBTrack->fTrackVert)
        SetWF(pwnd, WFVERTSCROLLTRACK);

    if (FWINABLE()) {
        xxxWindowEvent(EVENT_SYSTEM_SCROLLINGSTART, pwnd,
                (pSBTrack->fCtlSB ? OBJID_CLIENT :
                        (pSBTrack->fTrackVert ? OBJID_VSCROLL : OBJID_HSCROLL)),
                INDEXID_CONTAINER, 0);
        // Note: after xxx, pSBTrack may no longer be valid
    }

    (*xxxpfnSB)(pwnd, WM_LBUTTONDOWN, 0, lParam, pSBCalc);
    // Note: after xxx, pSBTrack may no longer be valid

    ptiCurrent = PtiCurrent();

    while (ptiCurrent->pq->spwndCapture == pwnd) {
        if (!xxxGetMessage(&msg, NULL, 0, 0)) {
            // Note: after xxx, pSBTrack may no longer be valid
            break;
        }

        if (!_CallMsgFilter(&msg, MSGF_SCROLLBAR)) {
            cmd = msg.message;

            if (msg.hwnd == HWq(pwnd) && ((cmd >= WM_MOUSEFIRST && cmd <=
                    WM_MOUSELAST) || (cmd >= WM_KEYFIRST &&
                    cmd <= WM_KEYLAST))) {
                cmd = SystoChar(cmd, msg.lParam);

                // After xxxWindowEvent, xxxpfnSB, xxxTranslateMessage or
                // xxxDispatchMessage, re-evaluate pSBTrack.
                REEVALUATE_PSBTRACK(pSBTrack, pwnd, "xxxTrackLoop");
                if ((pSBTrack == NULL) || (NULL == (xxxpfnSB = pSBTrack->xxxpfnSB)))
                    // mode cancelled -- exit track loop
                    return;

                (*xxxpfnSB)(pwnd, cmd, msg.wParam, msg.lParam, pSBCalc);
            } else {
                xxxTranslateMessage(&msg, 0);
                xxxDispatchMessage(&msg);
            }
        }
    }
}


/***************************************************************************\
* xxxSBTrackInit
*
* History:
\***************************************************************************/

void xxxSBTrackInit(
    PWND pwnd,
    LPARAM lParam,
    int curArea,
    UINT uType)
{
    int px;
    LPINT pwX;
    LPINT pwY;
    UINT wDisable;     // Scroll bar disable flags;
    SBCALC SBCalc;
    PSBCALC pSBCalc;
    RECT rcSB;
    PSBTRACK pSBTrack;

    CheckLock(pwnd);


    if (PWNDTOPSBTRACK(pwnd)) {
        RIPMSG1(RIP_WARNING, "xxxSBTrackInit: PWNDTOPSBTRACK(pwnd) == %#p",
                PWNDTOPSBTRACK(pwnd));
        return;
    }

    pSBTrack = (PSBTRACK)UserAllocPoolWithQuota(sizeof(*pSBTrack), TAG_SCROLLTRACK);
    if (pSBTrack == NULL)
        return;

    pSBTrack->hTimerSB = 0;
    pSBTrack->fHitOld = FALSE;

    pSBTrack->xxxpfnSB = xxxTrackBox;

    pSBTrack->spwndTrack = NULL;
    pSBTrack->spwndSB = NULL;
    pSBTrack->spwndSBNotify = NULL;
    Lock(&pSBTrack->spwndTrack, pwnd);
    PWNDTOPSBTRACK(pwnd) = pSBTrack;

    pSBTrack->fCtlSB = (!curArea);
    if (pSBTrack->fCtlSB) {

        /*
         * This is a scroll bar control.
         */
        Lock(&pSBTrack->spwndSB, pwnd);
        pSBTrack->fTrackVert = ((PSBWND)pwnd)->fVert;
        Lock(&pSBTrack->spwndSBNotify, pwnd->spwndParent);
        wDisable = ((PSBWND)pwnd)->wDisableFlags;
        pSBCalc = &((PSBWND)pwnd)->SBCalc;
        pSBTrack->nBar = SB_CTL;
    } else {

        /*
         * This is a scroll bar that is part of the window frame.
         */

#ifdef USE_MIRRORING
        //
        // Mirror the window coord of the scroll bar,
        // if it is a mirrored one
        //
        if (TestWF(pwnd,WEFLAYOUTRTL)) {
            lParam = MAKELONG(
                    pwnd->rcWindow.right - GET_X_LPARAM(lParam),
                    GET_Y_LPARAM(lParam) - pwnd->rcWindow.top);
        }
        else {
#endif
        lParam = MAKELONG(
                GET_X_LPARAM(lParam) - pwnd->rcWindow.left,
                GET_Y_LPARAM(lParam) - pwnd->rcWindow.top);

#ifdef USE_MIRRORING
        }
#endif
        Lock(&pSBTrack->spwndSBNotify, pwnd);
        Lock(&pSBTrack->spwndSB, NULL);
        pSBTrack->fTrackVert = (curArea - HTHSCROLL);
        wDisable = GetWndSBDisableFlags(pwnd, pSBTrack->fTrackVert);
        pSBCalc = &SBCalc;
        pSBTrack->nBar = (curArea - HTHSCROLL) ? SB_VERT : SB_HORZ;
    }

    pSBTrack->pSBCalc = pSBCalc;
    /*
     *  Check if the whole scroll bar is disabled
     */
    if((wDisable & SB_DISABLE_MASK) == SB_DISABLE_MASK) {
        Unlock(&pSBTrack->spwndSBNotify);
        Unlock(&pSBTrack->spwndSB);
        Unlock(&pSBTrack->spwndTrack);
        UserFreePool(pSBTrack);
        PWNDTOPSBTRACK(pwnd) = NULL;
        return;  // It is a disabled scroll bar; So, do not respond.
    }

    if (!pSBTrack->fCtlSB) {
        CalcSBStuff(pwnd, pSBCalc, pSBTrack->fTrackVert);
    }

    pwX = (LPINT)&rcSB;
    pwY = pwX + 1;
    if (!pSBTrack->fTrackVert)
        pwX = pwY--;

    px = (pSBTrack->fTrackVert ? GET_Y_LPARAM(lParam) : GET_X_LPARAM(lParam));

    *(pwX + 0) = pSBCalc->pxLeft;
    *(pwY + 0) = pSBCalc->pxTop;
    *(pwX + 2) = pSBCalc->pxRight;
    *(pwY + 2) = pSBCalc->pxBottom;
    pSBTrack->cmdSB = (UINT)-1;
    if (px < pSBCalc->pxUpArrow) {

        /*
         *  The click occurred on Left/Up arrow; Check if it is disabled
         */
        if(wDisable & LTUPFLAG) {
            if(pSBTrack->fCtlSB) {   // If this is a scroll bar control,
                zzzShowCaret(pSBTrack->spwndSB);  // show the caret before returning;
                // After zzzShowCaret, revalidate pSBTrack
                RETURN_IF_PSBTRACK_INVALID(pSBTrack, pwnd);
            }

            Unlock(&pSBTrack->spwndSBNotify);
            Unlock(&pSBTrack->spwndSB);
            Unlock(&pSBTrack->spwndTrack);
            UserFreePool(pSBTrack);
            PWNDTOPSBTRACK(pwnd) = NULL;
            return;         // Yes! disabled. Do not respond.
        }

        // LINEUP -- make rcSB the Up Arrow's Rectangle
        pSBTrack->cmdSB = SB_LINEUP;
        *(pwY + 2) = pSBCalc->pxUpArrow;
    } else if (px >= pSBCalc->pxDownArrow) {

        /*
         * The click occurred on Right/Down arrow; Check if it is disabled
         */
        if (wDisable & RTDNFLAG) {
            if (pSBTrack->fCtlSB) {    // If this is a scroll bar control,
                zzzShowCaret(pSBTrack->spwndSB);  // show the caret before returning;
                // After zzzShowCaret, revalidate pSBTrack
                RETURN_IF_PSBTRACK_INVALID(pSBTrack, pwnd);
            }

            Unlock(&pSBTrack->spwndSBNotify);
            Unlock(&pSBTrack->spwndSB);
            Unlock(&pSBTrack->spwndTrack);
            UserFreePool(pSBTrack);
            PWNDTOPSBTRACK(pwnd) = NULL;
            return;// Yes! disabled. Do not respond.
        }

        // LINEDOWN -- make rcSB the Down Arrow's Rectangle
        pSBTrack->cmdSB = SB_LINEDOWN;
        *(pwY + 0) = pSBCalc->pxDownArrow;
    } else if (px < pSBCalc->pxThumbTop) {
        // PAGEUP -- make rcSB the rectangle between Up Arrow and Thumb
        pSBTrack->cmdSB = SB_PAGEUP;
        *(pwY + 0) = pSBCalc->pxUpArrow;
        *(pwY + 2) = pSBCalc->pxThumbTop;
    } else if (px < pSBCalc->pxThumbBottom) {

DoThumbPos:
        /*
         * Elevator isn't there if there's no room.
         */
        if (pSBCalc->pxDownArrow - pSBCalc->pxUpArrow <= pSBCalc->cpxThumb) {
            Unlock(&pSBTrack->spwndSBNotify);
            Unlock(&pSBTrack->spwndSB);
            Unlock(&pSBTrack->spwndTrack);
            UserFreePool(pSBTrack);
            PWNDTOPSBTRACK(pwnd) = NULL;
            return;
        }
        // THUMBPOSITION -- we're tracking with the thumb
        pSBTrack->cmdSB = SB_THUMBPOSITION;
        CalcTrackDragRect(pSBTrack);

        pSBTrack->xxxpfnSB = xxxTrackThumb;
        pSBTrack->pxOld = pSBCalc->pxStart = pSBCalc->pxThumbTop;
        pSBTrack->posNew = pSBTrack->posOld = pSBCalc->pos;
        pSBTrack->dpxThumb = pSBCalc->pxStart - px;

        xxxCapture(PtiCurrent(), pwnd, WINDOW_CAPTURE);
        // After xxxCapture, revalidate pSBTrack
        RETURN_IF_PSBTRACK_INVALID(pSBTrack, pwnd);

        /*
         * DoScroll does thread locking on these two pwnds -
         * this is ok since they are not used after this
         * call.
         */
        if (pSBTrack->spwndSBNotify != NULL) {
            xxxDoScroll(pSBTrack->spwndSB, pSBTrack->spwndSBNotify,
                    SB_THUMBTRACK, pSBTrack->posOld, pSBTrack->fTrackVert
            );
            // Note: after xxx, pSBTrack may no longer be valid
        }
    } else if (px < pSBCalc->pxDownArrow) {
        // PAGEDOWN -- make rcSB the rectangle between Thumb and Down Arrow
        pSBTrack->cmdSB = SB_PAGEDOWN;
        *(pwY + 0) = pSBCalc->pxThumbBottom;
        *(pwY + 2) = pSBCalc->pxDownArrow;
    }

    /*
     * If the shift key is down, we'll position the thumb directly so it's
     * centered on the click point.
     */
    if ((uType == SCROLL_DIRECT && pSBTrack->cmdSB != SB_LINEUP && pSBTrack->cmdSB != SB_LINEDOWN) ||
            (uType == SCROLL_MENU)) {
        if (pSBTrack->cmdSB != SB_THUMBPOSITION) {
            goto DoThumbPos;
        }
        pSBTrack->dpxThumb = -(pSBCalc->cpxThumb / 2);
    }

    xxxCapture(PtiCurrent(), pwnd, WINDOW_CAPTURE);
    // After xxxCapture, revalidate pSBTrack
    RETURN_IF_PSBTRACK_INVALID(pSBTrack, pwnd);

    if (pSBTrack->cmdSB != SB_THUMBPOSITION) {
        CopyRect(&pSBTrack->rcTrack, &rcSB);
    }

    xxxSBTrackLoop(pwnd, lParam, pSBCalc);

    // After xxx, re-evaluate pSBTrack
    REEVALUATE_PSBTRACK(pSBTrack, pwnd, "xxxTrackLoop");
    if (pSBTrack) {
        Unlock(&pSBTrack->spwndSBNotify);
        Unlock(&pSBTrack->spwndSB);
        Unlock(&pSBTrack->spwndTrack);
        UserFreePool(pSBTrack);
        PWNDTOPSBTRACK(pwnd) = NULL;
    }
}

/***************************************************************************\
* GetScrollMenu
*
* History:
\***************************************************************************/

PMENU xxxGetScrollMenu(
    PWND pwnd,
    BOOL fVert)
{
    PMENU pMenu;
    PMENU *ppDesktopMenu;

    /*
     * Grab the menu from the desktop.  If the desktop menu
     * has not been loaded and this is not a system thread,
     * load it now.  Callbacks cannot be made from a system
     * thread or when a thread is in cleanup.
     */
    if (fVert) {
        ppDesktopMenu = &pwnd->head.rpdesk->spmenuVScroll;
    } else {
        ppDesktopMenu = &pwnd->head.rpdesk->spmenuHScroll;
    }
    pMenu = *ppDesktopMenu;
    if (pMenu == NULL && !(PtiCurrent()->TIF_flags & (TIF_SYSTEMTHREAD | TIF_INCLEANUP))) {
        UNICODE_STRING strMenuName;

        RtlInitUnicodeStringOrId(&strMenuName,
            fVert ? MAKEINTRESOURCE(ID_VSCROLLMENU) : MAKEINTRESOURCE(ID_HSCROLLMENU));
        pMenu = xxxClientLoadMenu(NULL, &strMenuName);
        LockDesktopMenu(ppDesktopMenu, pMenu);
    }

    /*
     * Return the handle to the scroll menu.
     */
    if (pMenu != NULL) {
        return _GetSubMenu(pMenu, 0);
    }

    return NULL;
}

/***************************************************************************\
* xxxDoScrollMenu
*
* History:
\***************************************************************************/

VOID
xxxDoScrollMenu(
    PWND pwndNotify,
    PWND pwndSB,
    BOOL fVert,
    LPARAM lParam)
{
    PMENU pMenu;
    SBCALC SBCalc, *pSBCalc;
    UINT cmd;
    POINT pt;
    TL tlpmenu;
    UINT wDisable;

    /*
     * Check the compatibility flag.  Word 6.0 AV's when selecting an item
     * in this menu.
     * NOTE: If this hack is to be extended for other apps we should use
     * another bit for GACF_NOSCROLLBARCTXMENU as the current one is re-used
     *  MCostea #119380
     */
    if (GetAppCompatFlags(NULL) & GACF_NOSCROLLBARCTXMENU) {
        return;
    }

    /*
     * Initialize some stuff.
     */
    POINTSTOPOINT(pt, lParam);
    if (pwndSB) {
        SBCtlSetup((PSBWND)pwndSB);
        pSBCalc = &(((PSBWND)pwndSB)->SBCalc);
        wDisable = ((PSBWND)pwndSB)->wDisableFlags;
        pt.x -= pwndSB->rcWindow.left;
        pt.y -= pwndSB->rcWindow.top;
    } else {
        pSBCalc = &SBCalc;
        CalcSBStuff(pwndNotify, pSBCalc, fVert);
        wDisable = GetWndSBDisableFlags(pwndNotify, fVert);
        pt.x -= pwndNotify->rcWindow.left;
        pt.y -= pwndNotify->rcWindow.top;
    }

    /*
     * Make sure the scrollbar isn't disabled.
     */
    if ((wDisable & SB_DISABLE_MASK) == SB_DISABLE_MASK) {
        return;
    }

    /*
     * Put up a menu and scroll accordingly.
     */
    if ((pMenu = xxxGetScrollMenu(pwndNotify, fVert)) != NULL) {
        ThreadLockAlways(pMenu, &tlpmenu);
        cmd = xxxTrackPopupMenuEx(pMenu,
                                  TPM_RIGHTBUTTON | TPM_RETURNCMD | TPM_NONOTIFY,
                                  GET_X_LPARAM(lParam),
                                  GET_Y_LPARAM(lParam),
                                  pwndNotify,
                                  NULL);
        ThreadUnlock(&tlpmenu);
        if (cmd) {
            if ((cmd & 0x00FF) == SB_THUMBPOSITION) {
                if (pwndSB) {
                    xxxSBTrackInit(pwndSB, MAKELPARAM(pt.x, pt.y), 0, SCROLL_MENU);
                } else {
                    xxxSBTrackInit(pwndNotify, lParam, fVert ? HTVSCROLL : HTHSCROLL, SCROLL_MENU);
                }
            } else {
                xxxDoScroll(pwndSB,
                            pwndNotify,
                            cmd & 0x00FF,
                            0,
                            fVert
                );
                xxxDoScroll(pwndSB,
                            pwndNotify,
                            SB_ENDSCROLL,
                            0,
                            fVert
                );
            }
        }
    }
}

/***************************************************************************\
* xxxSBWndProc
*
* History:
* 08-15-95 jparsons Added guard against NULL lParam [51986]
\***************************************************************************/

LRESULT xxxSBWndProc(
    PSBWND psbwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    LONG l;
    LONG lres;
    int cx;
    int cy;
    UINT cmd;
    UINT uSide;
    HDC hdc;
    RECT rc;
    POINT pt;
    BOOL fSizeReal;
    HBRUSH hbrSave;
    BOOL fSize;
    PAINTSTRUCT ps;
    UINT style;
    TL tlpwndParent;
    SCROLLINFO      si;
    LPSCROLLINFO    lpsi = &si;
    BOOL            fRedraw = FALSE;
    BOOL            fScroll;

    CheckLock(psbwnd);
    UserAssert(IsWinEventNotifyDeferredOK());

    VALIDATECLASSANDSIZE(((PWND)psbwnd), message, wParam, lParam, FNID_SCROLLBAR, WM_CREATE);

    style = LOBYTE(psbwnd->wnd.style);
    fSize = ((style & (SBS_SIZEBOX | SBS_SIZEGRIP)) != 0);

    switch (message) {
    case WM_CREATE:
        /*
         * Guard against lParam being NULL since the thunk allows it [51986]
         */
        if (lParam) {
            rc.right = (rc.left = ((LPCREATESTRUCT)lParam)->x) +
                    ((LPCREATESTRUCT)lParam)->cx;
            rc.bottom = (rc.top = ((LPCREATESTRUCT)lParam)->y) +
                    ((LPCREATESTRUCT)lParam)->cy;
            // This is because we can't just rev CardFile -- we should fix the
            // problem here in case anyone else happened to have some EXTRA
            // scroll styles on their scroll bar controls (jeffbog 03/21/94)
            if (!TestWF((PWND)psbwnd, WFWIN40COMPAT))
                psbwnd->wnd.style &= ~(WS_HSCROLL | WS_VSCROLL);

            if (!fSize) {
                l = PtrToLong(((LPCREATESTRUCT)lParam)->lpCreateParams);
                psbwnd->SBCalc.pos = psbwnd->SBCalc.posMin = LOWORD(l);
                psbwnd->SBCalc.posMax = HIWORD(l);
                psbwnd->fVert = ((LOBYTE(psbwnd->wnd.style) & SBS_VERT) != 0);
                psbwnd->SBCalc.page = 0;
            }

            if (psbwnd->wnd.style & WS_DISABLED)
                psbwnd->wDisableFlags = SB_DISABLE_MASK;

            if (style & (SBS_TOPALIGN | SBS_BOTTOMALIGN)) {
                if (fSize) {
                    if (style & SBS_SIZEBOXBOTTOMRIGHTALIGN) {
                        rc.left = rc.right - SYSMET(CXVSCROLL);
                        rc.top = rc.bottom - SYSMET(CYHSCROLL);
                    }

                    rc.right = rc.left + SYSMET(CXVSCROLL);
                    rc.bottom = rc.top + SYSMET(CYHSCROLL);
                } else {
                    if (style & SBS_VERT) {
                        if (style & SBS_LEFTALIGN)
                            rc.right = rc.left + SYSMET(CXVSCROLL);
                        else
                            rc.left = rc.right - SYSMET(CXVSCROLL);
                    } else {
                        if (style & SBS_TOPALIGN)
                            rc.bottom = rc.top + SYSMET(CYHSCROLL);
                        else
                            rc.top = rc.bottom - SYSMET(CYHSCROLL);
                    }
                }

                xxxMoveWindow((PWND)psbwnd, rc.left, rc.top, rc.right - rc.left,
                         rc.bottom - rc.top, FALSE);
            }
        } /* if */

        else {
            RIPERR0(ERROR_INVALID_PARAMETER, RIP_WARNING,
                    "xxxSBWndProc - NULL lParam for WM_CREATE\n") ;
        } /* else */

        break;

    case WM_SIZE:
        if (PtiCurrent()->pq->spwndFocus != (PWND)psbwnd)
            break;

        // scroll bar has the focus -- recalc it's thumb caret size
        // no need to DeferWinEventNotify() - see xxxCreateCaret below.
        zzzDestroyCaret();

            //   |             |
            //   |  FALL THRU  |
            //   V             V

    case WM_SETFOCUS:
        SBCtlSetup(psbwnd);

        cx = (psbwnd->fVert ? psbwnd->wnd.rcWindow.right - psbwnd->wnd.rcWindow.left
                            : psbwnd->SBCalc.cpxThumb) - 2 * SYSMET(CXEDGE);
        cy = (psbwnd->fVert ? psbwnd->SBCalc.cpxThumb
                            : psbwnd->wnd.rcWindow.bottom - psbwnd->wnd.rcWindow.top) - 2 * SYSMET(CYEDGE);

        xxxCreateCaret((PWND)psbwnd, (HBITMAP)1, cx, cy);
        zzzSetSBCaretPos(psbwnd);
        zzzShowCaret((PWND)psbwnd);
        break;

    case WM_KILLFOCUS:
        zzzDestroyCaret();
        break;

    case WM_ERASEBKGND:

        /*
         * Do nothing, but don't let DefWndProc() do it either.
         * It will be erased when its painted.
         */
        return (LONG)TRUE;

    case WM_PRINTCLIENT:
    case WM_PAINT:
        if ((hdc = (HDC)wParam) == NULL) {
            hdc = xxxBeginPaint((PWND)psbwnd, (LPPAINTSTRUCT)&ps);
        }
        if (!fSize) {
            SBCtlSetup(psbwnd);
            xxxDrawSB2((PWND)psbwnd, &psbwnd->SBCalc, hdc, psbwnd->fVert, psbwnd->wDisableFlags);
        } else {
            fSizeReal = TestWF((PWND)psbwnd, WFSIZEBOX);
            if (!fSizeReal)
                SetWF((PWND)psbwnd, WFSIZEBOX);

            DrawSize((PWND)psbwnd, hdc, 0, 0);

            if (!fSizeReal)
                ClrWF((PWND)psbwnd, WFSIZEBOX);
        }

        if (wParam == 0L)
            xxxEndPaint((PWND)psbwnd, (LPPAINTSTRUCT)&ps);
        break;

    case WM_GETDLGCODE:
        return DLGC_WANTARROWS;

    case WM_CONTEXTMENU:
            ThreadLock(psbwnd->wnd.spwndParent, &tlpwndParent);
            xxxDoScrollMenu(psbwnd->wnd.spwndParent, (PWND)psbwnd, psbwnd->fVert, lParam);
            ThreadUnlock(&tlpwndParent);
        break;

    case WM_NCHITTEST:
        if (style & SBS_SIZEGRIP) {
#ifdef USE_MIRRORING
            /*
             * If the scroll bar is RTL mirrored, then
             * mirror the hittest of the grip location.
             */
            if (TestWF((PWND)psbwnd, WEFLAYOUTRTL))
                return HTBOTTOMLEFT;
            else
#endif
                return HTBOTTOMRIGHT;
        } else {
            goto DoDefault;
        }
        break;

#ifdef COLOR_HOTTRACKING
    case WM_MOUSELEAVE:
        xxxHotTrackSBCtl(psbwnd, 0, FALSE);
        psbwnd->ht = 0;
        break;

    case WM_MOUSEMOVE:
        {
            int ht;

            if (psbwnd->ht == 0) {
                TRACKMOUSEEVENT tme = {sizeof(TRACKMOUSEEVENT), TME_LEAVE, HWq(psbwnd), 0};
                TrackMouseEvent(&tme);
            }

            pt.x = GET_X_LPARAM(lParam);
            pt.y = GET_Y_LPARAM(lParam);
            ht = HitTestScrollBar((PWND)psbwnd, psbwnd->fVert, pt);
            if (psbwnd->ht != ht) {
                xxxHotTrackSBCtl(psbwnd, ht, TRUE);
                psbwnd->ht = ht;
            }
        }
        break;
#endif // COLOR_HOTTRACKING

    case WM_LBUTTONDBLCLK:
        cmd = SC_ZOOM;
        if (fSize)
            goto postmsg;

        /*
         *** FALL THRU **
         */

    case WM_LBUTTONDOWN:
            //
            // Note that SBS_SIZEGRIP guys normally won't ever see button
            // downs.  This is because they return HTBOTTOMRIGHT to
            // WindowHitTest handling.  This will walk up the parent chain
            // to the first sizeable ancestor, bailing out at caption windows
            // of course.  That dude, if he exists, will handle the sizing
            // instead.
            //
        if (!fSize) {
            if (TestWF((PWND)psbwnd, WFTABSTOP)) {
                xxxSetFocus((PWND)psbwnd);
            }

            zzzHideCaret((PWND)psbwnd);
            SBCtlSetup(psbwnd);

            /*
             * SBCtlSetup enters SEM_SB, and xxxSBTrackInit leaves it.
             */
            xxxSBTrackInit((PWND)psbwnd, lParam, 0, (_GetKeyState(VK_SHIFT) < 0) ? SCROLL_DIRECT : SCROLL_NORMAL);
            break;
        } else {
            cmd = SC_SIZE;
postmsg:
            pt.x = GET_X_LPARAM(lParam);
            pt.y = GET_Y_LPARAM(lParam);
            _ClientToScreen((PWND)psbwnd, &pt);
            lParam = MAKELONG(pt.x, pt.y);

            /*
             * convert HT value into a move value.  This is bad,
             * but this is purely temporary.
             */
#ifdef USE_MIRRORING
            if (TestWF(((PWND)psbwnd)->spwndParent,WEFLAYOUTRTL)) {
                uSide = HTBOTTOMLEFT;
            } else 
#endif
            {
                uSide = HTBOTTOMRIGHT;
            }
            ThreadLock(((PWND)psbwnd)->spwndParent, &tlpwndParent);
            xxxSendMessage(((PWND)psbwnd)->spwndParent, WM_SYSCOMMAND,
                    (cmd | (uSide - HTSIZEFIRST + 1)), lParam);
            ThreadUnlock(&tlpwndParent);
        }
        break;

    case WM_KEYUP:
        switch (wParam) {
        case VK_HOME:
        case VK_END:
        case VK_PRIOR:
        case VK_NEXT:
        case VK_LEFT:
        case VK_UP:
        case VK_RIGHT:
        case VK_DOWN:

            /*
             * Send end scroll message when user up clicks on keyboard
             * scrolling.
             *
             * DoScroll does thread locking on these two pwnds -
             * this is ok since they are not used after this
             * call.
             */
            xxxDoScroll((PWND)psbwnd, psbwnd->wnd.spwndParent,
                    SB_ENDSCROLL, 0, psbwnd->fVert
            );
            break;

        default:
            break;
        }
        break;

    case WM_KEYDOWN:
        switch (wParam) {
        case VK_HOME:
            wParam = SB_TOP;
            goto KeyScroll;

        case VK_END:
            wParam = SB_BOTTOM;
            goto KeyScroll;

        case VK_PRIOR:
            wParam = SB_PAGEUP;
            goto KeyScroll;

        case VK_NEXT:
            wParam = SB_PAGEDOWN;
            goto KeyScroll;

        case VK_LEFT:
        case VK_UP:
            wParam = SB_LINEUP;
            goto KeyScroll;

        case VK_RIGHT:
        case VK_DOWN:
            wParam = SB_LINEDOWN;
KeyScroll:

            /*
             * DoScroll does thread locking on these two pwnds -
             * this is ok since they are not used after this
             * call.
             */
            xxxDoScroll((PWND)psbwnd, psbwnd->wnd.spwndParent, (int)wParam,
                    0, psbwnd->fVert
            );
            break;

        default:
            break;
        }
        break;

    case WM_ENABLE:
        return xxxSendMessage((PWND)psbwnd, SBM_ENABLE_ARROWS,
               (wParam ? ESB_ENABLE_BOTH : ESB_DISABLE_BOTH), 0);

    case SBM_ENABLE_ARROWS:

        /*
         * This is used to enable/disable the arrows in a SB ctrl
         */
        return (LONG)xxxEnableSBCtlArrows((PWND)psbwnd, (UINT)wParam);

    case SBM_GETPOS:
        return (LONG)psbwnd->SBCalc.pos;

    case SBM_GETRANGE:
        *((LPINT)wParam) = psbwnd->SBCalc.posMin;
        *((LPINT)lParam) = psbwnd->SBCalc.posMax;
        return MAKELRESULT(LOWORD(psbwnd->SBCalc.posMin), LOWORD(psbwnd->SBCalc.posMax));

    case SBM_GETSCROLLINFO:
        return (LONG)_SBGetParms((PWND)psbwnd, SB_CTL, (PSBDATA)&psbwnd->SBCalc, (LPSCROLLINFO) lParam);

    case SBM_SETRANGEREDRAW:
        fRedraw = TRUE;

    case SBM_SETRANGE:
        // Save the old values of Min and Max for return value
        si.cbSize = sizeof(si);
//        si.nMin = LOWORD(lParam);
//        si.nMax = HIWORD(lParam);
        si.nMin = (int)wParam;
        si.nMax = (int)lParam;
        si.fMask = SIF_RANGE | SIF_RETURNOLDPOS;
        goto SetInfo;

    case SBM_SETPOS:
        fRedraw = (BOOL) lParam;
        si.cbSize = sizeof(si);
        si.fMask = SIF_POS | SIF_RETURNOLDPOS;
        si.nPos  = (int)wParam;
        goto SetInfo;

    case SBM_SETSCROLLINFO:
        lpsi = (LPSCROLLINFO) lParam;
        fRedraw = (BOOL) wParam;
SetInfo:
        fScroll = TRUE;

        if (SBSetParms((PSBDATA)&psbwnd->SBCalc, lpsi, &fScroll, &lres) && FWINABLE()) {
            xxxWindowEvent(EVENT_OBJECT_VALUECHANGE, (PWND)psbwnd, OBJID_CLIENT,
                    INDEX_SCROLLBAR_SELF, WEF_USEPWNDTHREAD);
        }

        if (!fRedraw)
            return lres;


        /*
         * We must set the new position of the caret irrespective of
         * whether the window is visible or not;
         * Still, this will work only if the app has done a xxxSetScrollPos
         * with fRedraw = TRUE;
         * Fix for Bug #5188 --SANKAR-- 10-15-89
         * No need to DeferWinEventNotify since psbwnd is locked.
         */
        zzzHideCaret((PWND)psbwnd);
        SBCtlSetup(psbwnd);
        zzzSetSBCaretPos(psbwnd);

            /*
             ** The following zzzShowCaret() must be done after the DrawThumb2(),
             ** otherwise this caret will be erased by DrawThumb2() resulting
             ** in this bug:
             ** Fix for Bug #9263 --SANKAR-- 02-09-90
             *
             */

            /*
             *********** zzzShowCaret((PWND)psbwnd); ******
             */

        if (_FChildVisible((PWND)psbwnd) && fRedraw) {
            UINT    wDisable;
            HBRUSH  hbrUse;

            if (!fScroll)
                fScroll = !(lpsi->fMask & SIF_DISABLENOSCROLL);

            wDisable = (fScroll) ? ESB_ENABLE_BOTH : ESB_DISABLE_BOTH;
            xxxEnableScrollBar((PWND) psbwnd, SB_CTL, wDisable);

            hdc = _GetWindowDC((PWND)psbwnd);
            hbrSave = GreSelectBrush(hdc, hbrUse = xxxGetColorObjects((PWND)psbwnd, hdc));

                /*
                 * Before we used to only hideshowthumb() if the mesage was
                 * not SBM_SETPOS.  I am not sure why but this case was ever
                 * needed for win 3.x but on NT it resulted in trashing the border
                 * of the scrollbar when the app called SetScrollPos() during
                 * scrollbar tracking.  - mikehar 8/26
                 */
            DrawThumb2((PWND)psbwnd, &psbwnd->SBCalc, hdc, hbrUse, psbwnd->fVert,
                         psbwnd->wDisableFlags);
            GreSelectBrush(hdc, hbrSave);
            _ReleaseDC(hdc);
        }

            /*
             * This zzzShowCaret() has been moved to this place from above
             * Fix for Bug #9263 --SANKAR-- 02-09-90
             */
        zzzShowCaret((PWND)psbwnd);
        return lres;

    default:
DoDefault:
        return xxxDefWindowProc((PWND)psbwnd, message, wParam, lParam);
    }

    return 0L;
}
