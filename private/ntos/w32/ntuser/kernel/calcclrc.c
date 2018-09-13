/****************************** Module Header ******************************\
* Module Name: calcclrc.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* History:
* 10-22-90 MikeHar      Ported functions from Win 3.0 sources.
* 01-Feb-1991 mikeke    Added Revalidation code
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop


/****************************** Module Header ******************************\
* xxxCalcClientRect
*
* 10-22-90 MikeHar      Ported functions from Win 3.0 sources.
\****************************** Module Header ******************************/

void xxxCalcClientRect(
    PWND pwnd,
    LPRECT lprc,
    BOOL fHungRedraw)
{
    int cxFrame, yTopOld;
    RECT rcTemp;
    PMENU pMenu;
    TL tlpmenu;
    int     cBorders;
    BOOL    fEmptyClient;
    BYTE    bFramePresent;

    CheckLock(pwnd);
    UserAssert(IsWinEventNotifyDeferredOK());

    bFramePresent = TestWF(pwnd, WFFRAMEPRESENTMASK);

    /*
     * Clear all the frame bits.  NOTE: The HIBYTE of all these #defines
     * must stay the same for this line to work.
     */
    ClrWF(pwnd, WFFRAMEPRESENTMASK);

    //
    // We need to clear the client border bits also. Otherwise, when the
    // window gets really small, the client border will draw over the menu
    // and caption.
    //
    ClrWF(pwnd, WFCEPRESENT);

    /*
     * If the window is iconic, the client area is empty.
     */
    if (TestWF(pwnd, WFMINIMIZED)) {
        //  SetRectEmpty(lprc);
      // We must make it an empty rectangle.
      // But, that empty rectangle should be at the top left corner of the
      // window rect. Else, ScreenToClient() will return bad values.
        lprc->right = lprc->left;
        lprc->bottom = lprc->top;
        goto CalcClientDone;
    }

    // Save rect into rcTemp for easy local calculations.
    CopyRect(&rcTemp, lprc);

    // Save the top so we'll know how tall the caption was
    yTopOld = rcTemp.top;

    // Adjustment for the caption
    if (TestWF(pwnd, WFBORDERMASK) == LOBYTE(WFCAPTION))
    {
        SetWF(pwnd, WFCPRESENT);

        rcTemp.top += GetCaptionHeight(pwnd);
    }

    // Subtract out window borders
    cBorders = GetWindowBorders(pwnd->style, pwnd->ExStyle, TRUE, FALSE);
    cxFrame = cBorders * SYSMETFROMPROCESS(CXBORDER);
    InflateRect(&rcTemp, -cxFrame, -cBorders * SYSMETFROMPROCESS(CYBORDER));

    if (!TestwndChild(pwnd) && (pMenu = pwnd->spmenu)) {
        SetWF(pwnd, WFMPRESENT);
        if (!fHungRedraw) {
            ThreadLockAlways(pMenu, &tlpmenu);
            rcTemp.top += xxxMenuBarCompute(pMenu, pwnd, rcTemp.top - yTopOld,
                    cxFrame, rcTemp.right - rcTemp.left);
            ThreadUnlock(&tlpmenu);
        }
    }
    /*
     * We should have cleared WFMPRESENT in the else case here. Win9x doesn't do
     *  it either. Any code checking this flag will do the wrong thing...
     * It seems that it's pretty unsual for apps to remove the menu....
     * No code checking this flag can assume that pwnd->spmenu is not NULL -- we
     *  would need to clear it way earlier (at unlock time) for such assumption to hold true.
     */

    //
    // Fix for B#1425 -- Sizing window really small used to move children's
    // rects because the client calculations were wrong.  So we make the
    // bottom of the client match up with the top (the bottom of the menu
    // bar).
    //
    fEmptyClient = FALSE;

    if (rcTemp.top >= rcTemp.bottom) {
        rcTemp.bottom = rcTemp.top;
        fEmptyClient = TRUE;
    }

    //
    // BOGUS BOGUS BOGUS
    // Hack for Central Point PC Tools.
    // Possibly for M5 only.
    // B#8445
    //
    // They check for div-by-zero all over, but they jump to the wrong place
    // if a zero divisor is encountered, and end up faulting anyway.  So this
    // code path was never tested basically.  There's a period when starting
    // up where the window rect of their drives ribbon is empty.  In Win3.x,
    // the client would be shrunk to account for the border it had, and it
    // would look like it wasn't empty because the width would be negative,
    // signed!  So we version-switch this code, since other apps have
    // reported the non-emptiness as an annoying bug.
    //
    if (TestWF(pwnd, WFWIN40COMPAT) && (rcTemp.left >= rcTemp.right)) {
        rcTemp.right = rcTemp.left;
        fEmptyClient = TRUE;
    }

    if (fEmptyClient)
        goto ClientCalcEnd;

    //
    // Subtract client edge if we have space
    //
    if (    TestWF(pwnd, WEFCLIENTEDGE) &&
            (rcTemp.right - rcTemp.left >= (2 * SYSMETFROMPROCESS(CXEDGE))) &&
            (rcTemp.bottom - rcTemp.top >= (2 * SYSMETFROMPROCESS(CYEDGE))) ) {
        SetWF(pwnd, WFCEPRESENT);
        InflateRect(&rcTemp, -SYSMETFROMPROCESS(CXEDGE), -SYSMETFROMPROCESS(CYEDGE));
    }

    //
    // Subtract scrollbars
    // Note compatibility with 3.1:
    //      * You don't get a horizontal scrollbar unless you have MORE
    //  space (> ) in your client than you need for one.
    //      * You get a vertical scrollbar if you have AT LEAST ENOUGH
    //  space (>=) in your client for one.
    //
    if (TestWF(pwnd, WFHSCROLL) && (rcTemp.bottom - rcTemp.top > SYSMETFROMPROCESS(CYHSCROLL))) {
        SetWF(pwnd, WFHPRESENT);
        if (!fHungRedraw) {
            rcTemp.bottom -= SYSMETFROMPROCESS(CYHSCROLL);
        }
    }

    if (TestWF(pwnd, WFVSCROLL) && (rcTemp.right - rcTemp.left >= SYSMETFROMPROCESS(CXVSCROLL))) {
        SetWF(pwnd, WFVPRESENT);
        if (!fHungRedraw) {
#ifdef USE_MIRRORING
            if ((!!TestWF(pwnd, WEFLEFTSCROLL)) ^ (!!TestWF(pwnd, WEFLAYOUTRTL)))
#else
            if (TestWF(pwnd, WEFLEFTSCROLL))
#endif
                rcTemp.left += SYSMETFROMPROCESS(CXVSCROLL);
            else
                rcTemp.right -= SYSMETFROMPROCESS(CXVSCROLL);
        }
    }

ClientCalcEnd:

    CopyRect(lprc, &rcTemp);

CalcClientDone:
    if (FWINABLE() && (bFramePresent != TestWF(pwnd, WFFRAMEPRESENTMASK))) {
        xxxWindowEvent(EVENT_OBJECT_REORDER, pwnd, OBJID_WINDOW, 0, WEF_USEPWNDTHREAD);
    }
}

/***************************************************************************\
*
*  UpdateClientRect()
*
*  Make sure the client rect reflects the window styles correctly
*
\***************************************************************************/

void xxxUpdateClientRect(
    PWND pwnd)
{
    RECT rc;

    CopyRect(&rc, &pwnd->rcWindow);
    xxxCalcClientRect(pwnd, &rc, FALSE);
    CopyRect(&pwnd->rcClient, &rc);
}
