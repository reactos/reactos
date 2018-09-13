/****************************** Module Header ******************************\
* Module Name: drawfrm.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Window Frame Drawing Routines. (aka wmframe.c)
*
* History:
* 10-22-90 MikeHar    Ported functions from Win 3.0 sources.
* 13-Feb-1991 mikeke    Added Revalidation code
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop


/***************************************************************************\
* BitBltSysBmp
*
\***************************************************************************/

BOOL FAR BitBltSysBmp(
    HDC          hdc,
    int          x,
    int          y,
    UINT         i
    )
{
    BOOL bRet;
    POEMBITMAPINFO pOem = gpsi->oembmi + i;

    bRet = GreBitBlt(hdc,
                     x,
                     y,
                     pOem->cx,
                     pOem->cy,
                     HDCBITS(),
                     pOem->x,
                     pOem->y,
                     SRCCOPY,
                     0);
#ifdef USE_MIRRORING
    /*
     * If the UI language is Hebrew we do not want to mirror the ? mark only
     * Then redraw ? with out the button frame.
     */
    if (HEBREW_UI_LANGID() && MIRRORED_HDC(hdc)) {
        if ((i >= OBI_HELP) && (i <= OBI_HELP_H)) {
            if (i == OBI_HELP_D) {
                x = x + SYSMET(CXEDGE); 
            }
            bRet = GreBitBlt(hdc,
                             x,
                             y+SYSMET(CXEDGE),
                             pOem->cx-SYSMET(CXEDGE)*2,
                             pOem->cy-SYSMET(CXEDGE)*2,
                             HDCBITS(),
                             pOem->x+SYSMET(CXEDGE),
                             pOem->y+SYSMET(CXEDGE),
                             SRCCOPY|NOMIRRORBITMAP,
                             0);

        }
    }
#endif
    return bRet;
}

/***************************************************************************\
* xxxDrawWindowFrame
*
* History:
* 10-24-90 MikeHar      Ported from WaWaWaWindows.
\***************************************************************************/

void xxxDrawWindowFrame(
    PWND pwnd,
    HDC  hdc,
    BOOL fHungRedraw,
    BOOL fActive)
{
    RECT    rcClip;
    int cxFrame, cyFrame;
    UINT    wFlags = DC_NC;

    CheckLock(pwnd);

    /*
     * If we are minimized, or if a parent is minimized or invisible,
     * we've got nothing to draw.
     */
    if (!IsVisible(pwnd) ||
        (TestWF(pwnd, WFNONCPAINT) && !TestWF(pwnd, WFMENUDRAW)) ||
        EqualRect(&pwnd->rcWindow, &pwnd->rcClient)) {
        return;
    }

    /*
     * If the update rgn is not NULL, we may have to invalidate the bits saved.
     */
//    if (TRUE) {
    if (pwnd->hrgnUpdate > NULL || GreGetClipBox(hdc, &rcClip, TRUE) != NULLREGION) {
        RECT rcWindow;
        int  cBorders;

        if (TestWF(pwnd, WFMINIMIZED) && !TestWF(pwnd, WFNONCPAINT)) {
            if (TestWF(pwnd, WFFRAMEON))
                wFlags |= DC_ACTIVE;
            if (fHungRedraw)
                wFlags |= DC_NOSENDMSG;
            xxxDrawCaptionBar(pwnd, hdc, wFlags);
            return;
        }

        cxFrame = cyFrame = cBorders =
            GetWindowBorders(pwnd->style, pwnd->ExStyle, TRUE, FALSE);
        cxFrame *= SYSMET(CXBORDER);
        cyFrame *= SYSMET(CYBORDER);

        GetRect(pwnd, &rcWindow, GRECT_WINDOW | GRECT_WINDOWCOORDS);
        InflateRect(&rcWindow, -cxFrame, -cyFrame);

        /*
         * If the menu style is present, draw it.
         */
        if (TestWF(pwnd, WFMPRESENT) && !fHungRedraw) {
            rcWindow.top += xxxMenuBarDraw(pwnd, hdc, cxFrame, cyFrame);
        }

        /*
         * Draw the title bar if the window has a caption or any window
         * borders.  Punt if the NONCPAINT bit is set, because that means
         * we're going to draw the frame a little bit later.
         */

        if ((TestWF(pwnd, WFBORDERMASK) != 0
                || TestWF(pwnd, WEFDLGMODALFRAME))
                || TestWF(pwnd, WFSIZEBOX)
                || TestWF(pwnd, WEFWINDOWEDGE)
                || TestWF(pwnd, WEFSTATICEDGE)
            && !TestWF(pwnd, WFNONCPAINT))
        {
            if (fHungRedraw)
                wFlags |= DC_NOSENDMSG;
            if (fActive)
                wFlags |= DC_ACTIVE;
            xxxDrawCaptionBar(pwnd, hdc, wFlags | DC_NOVISIBLE);
        }

        //
        // Subtract out caption if present.
        //
        rcWindow.top += GetCaptionHeight(pwnd);

        //
        // Draw client edge
        //
        if (TestWF(pwnd, WFCEPRESENT)) {
            cxFrame += SYSMET(CXEDGE);
            cyFrame += SYSMET(CYEDGE);
            DrawEdge(hdc, &rcWindow, EDGE_SUNKEN, BF_RECT | BF_ADJUST);
        }

        //
        // Since scrolls don't have to use tricks to overlap the window
        // border anymore, we don't have to worry about borders.
        //
        if (TestWF(pwnd, WFVPRESENT) && !fHungRedraw) {
            if (TestWF(pwnd, WFHPRESENT)) {
                // This accounts for client borders.
                DrawSize(pwnd, hdc, cxFrame, cyFrame);
            }

            xxxDrawScrollBar(pwnd, hdc, TRUE);
        }

        if (TestWF(pwnd, WFHPRESENT) && !fHungRedraw)
            xxxDrawScrollBar(pwnd, hdc, FALSE);
    }
}


/***************************************************************************\
* xxxRedrawFrame
*
* Called by scrollbars and menus to redraw a windows scroll bar or menu.
*
* History:
* 10-24-90 MikeHar Ported from WaWaWaWindows.
\***************************************************************************/

void xxxRedrawFrame(
    PWND pwnd)
{
    CheckLock(pwnd);

    /*
     * We always want to call xxxSetWindowPos, even if invisible or iconic,
     * because we need to make sure the WM_NCCALCSIZE message gets sent.
     */
    xxxSetWindowPos(pwnd, NULL, 0, 0, 0, 0, SWP_NOZORDER |
            SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE | SWP_DRAWFRAME);
}

void xxxRedrawFrameAndHook(
    PWND pwnd)
{
    CheckLock(pwnd);

    /*
     * We always want to call xxxSetWindowPos, even if invisible or iconic,
     * because we need to make sure the WM_NCCALCSIZE message gets sent.
     */
    xxxSetWindowPos(pwnd, NULL, 0, 0, 0, 0, SWP_NOZORDER |
            SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE | SWP_DRAWFRAME);
    if ( IsTrayWindow(pwnd) ) {
        HWND hw = HWq(pwnd);
        xxxCallHook(HSHELL_REDRAW, (WPARAM)hw, 0L, WH_SHELL);
        PostShellHookMessages(HSHELL_REDRAW, (LPARAM)hw);

    }
}
