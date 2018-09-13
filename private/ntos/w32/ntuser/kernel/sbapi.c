/**************************** Module Header ********************************\
* Module Name:
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Scroll bar public APIs
*
* History:
*   11/21/90 JimA      Created.
*   01-31-91 IanJa     Revalidaion added
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* xxxShowScrollBar
*
* Shows and hides standard scroll bars or scroll bar controls. If wBar is
* SB_HORZ, SB_VERT, or SB_BOTH, pwnd is assumed to be the handle of the window
* which has the standard scroll bars as styles. If wBar is SB_CTL, pwnd is
* assumed to be the handle of the scroll bar control.
*
* It does not destroy pwnd->rgwScroll like xxxSetScrollBar() does, so that the
* app can hide a standard scroll bar and then show it, without having to reset
* the range and thumbposition.
*
* History:
* 16-May-1991 mikeke    Changed to return BOOL
\***************************************************************************/

BOOL xxxShowScrollBar(
    PWND pwnd,
    UINT wBar,      /* SB_HORZ, SB_VERT, SB_BOTH , SB_CTL */
    BOOL fShow)     /* Show or Hide. */
{
    BOOL fChanged = FALSE;
    DWORD   dwStyle;

    CheckLock(pwnd);

    switch (wBar)
    {
    case SB_CTL:
        {

            xxxShowWindow(
                    pwnd,
                    (fShow ? SHOW_OPENWINDOW : HIDE_WINDOW) | TEST_PUDF(PUDF_ANIMATE));

            return(TRUE);
        }

    case SB_HORZ:
        dwStyle = WS_HSCROLL;
        break;

    case SB_VERT:
        dwStyle = WS_VSCROLL;
        break;

    case SB_BOTH:
        dwStyle = WS_HSCROLL | WS_VSCROLL;
        break;
    }

    if (!fShow)
    {
        if (pwnd->style & dwStyle)
        {
            fChanged = TRUE;
            pwnd->style &= ~dwStyle;
        }
    } else {
        if ((pwnd->style & dwStyle) != dwStyle)
        {
            fChanged = TRUE;
            pwnd->style |= dwStyle;
        }

        /*
         * Make sure that pwsb is initialized.
         */
        if (pwnd->pSBInfo == NULL)
            _InitPwSB(pwnd);
    }

    /*
     * If the state changed, redraw the frame and force WM_NCPAINT.
     */
    if (fChanged) {

        /*
         * We always redraw even if minimized or hidden...  Otherwise, it seems
         * the scroll bars aren't properly hidden/shown when we become
         * visible
         */
        xxxRedrawFrame(pwnd);
    }
    return TRUE;
}
