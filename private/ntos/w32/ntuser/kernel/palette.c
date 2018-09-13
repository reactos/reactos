/**************************** Module Header ********************************\
* Module Name: palette.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Palette Handling Routines
*
* History:
* 24-May-1993 MikeKe    From win3.1
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* IsTopmostRealApp
*
* Returns true if current process is the shell process and this window
* is the first non-shell/user one we find in zorder.  If so, we consider
* him to be the "palette foreground".
*
* History:
\***************************************************************************/

BOOL IsTopmostRealApp(
    PWND pwnd)
{
    PTHREADINFO  ptiCurrent = PtiCurrent();
    PDESKTOPINFO pdeskinfo = pwnd->head.rpdesk->pDeskInfo;
    if ((pdeskinfo->spwndShell == NULL) ||
        (GETPTI(pdeskinfo->spwndShell)->pq != gpqForeground)) {

        return FALSE;
    }

    return (pwnd == NextTopWindow(ptiCurrent,
                                  NULL,
                                  NULL,
                                  NTW_IGNORETOOLWINDOW));
}

/***************************************************************************\
* _SelectPalette
*
* Selects palette into DC.  This is a wrapper to gdi where we can perform
* checks to see if it's a foreground dc.
*
* History:
\***************************************************************************/

HPALETTE _SelectPalette(
    HDC      hdc,
    HPALETTE hpal,
    BOOL     fForceBackground)
{
    PWND pwndTop;
    BOOL fBackgroundPalette = TRUE;
    PWND pwnd = NULL;
    /*
     * If we are not forcing palette into background, find out where it does
     * actually belong. Don't ever select the default palette in as a
     * foreground palette because this confuses gdi. Many apps do a
     * (oldPal = SelectPalette) (myPal); Draw; SelectObject(oldPal).
     * and we don't want to allow this to go through.
     */
    if (!fForceBackground     &&
        TEST_PUSIF(PUSIF_PALETTEDISPLAY) &&
        (hpal != GreGetStockObject(DEFAULT_PALETTE))) {

        if (pwnd = WindowFromCacheDC(hdc)) {

            PWND pwndActive;

            /*
             * don't "select" palette unless on a palette device
             */
            pwndTop = GetTopLevelWindow(pwnd);

            if (!TestWF(pwndTop, WFHASPALETTE)) {

                if (pwndTop != _GetDesktopWindow())
                    GETPTI(pwndTop)->TIF_flags |= TIF_PALETTEAWARE;

                SetWF(pwndTop, WFHASPALETTE);
            }

            /*
             * Hack-o-rama:
             * Windows get foreground use of the palette if
             *      * They are the foreground's active window
             *      * The current process is the shell and they are the
             * topmost valid non-toolwindow in the zorder.
             *
             * This makes our tray friendly on palettized displays.
             * Currently, if you run a palette app and click on the tray,
             * the palette app goes all weird.  Broderbund apps go
             * completely black.  This is because they get forced to be
             * background always, even though the shell isn't really
             * palettized.
             *
             * Note: this palette architecture sucks.  Apps get forced to
             * be background palette users even if the foreground thread
             * couldn't care less about the palette.  Should go by zorder
             * if so, but in a more clean way than this.
             *
             * We really only care about the tray && the background.
             * Cabinet dudes don't matter so much.
             */
            pwndActive = (gpqForeground ? gpqForeground->spwndActive : NULL);

#if 0
            if (pwndActive                                            &&
                (pwndTop != pwnd->head.rpdesk->pDeskInfo->spwndShell) &&
                ((pwndActive == pwnd) || _IsChild(pwndActive, pwnd) || IsTopmostRealApp(pwnd)) &&
                !TestWF(pwnd, WEFTOOLWINDOW)) {

                fBackgroundPalette = FALSE;
            }
#else
            if ((pwndTop != pwndTop->head.rpdesk->pDeskInfo->spwnd)      &&
                (pwndTop != pwndTop->head.rpdesk->pDeskInfo->spwndShell) &&
                (pwndActive != NULL)                                     &&
                ((pwndActive == pwnd)          ||
                    _IsChild(pwndActive, pwnd) ||
                    IsTopmostRealApp(pwnd))                              &&
                !TestWF(pwnd, WEFTOOLWINDOW)) {

                fBackgroundPalette = FALSE;
            }
#endif
        }
    }

    return GreSelectPalette(hdc, hpal, fBackgroundPalette);
}

/***************************************************************************\
* xxxRealizePalette
*
* Realizes palette to the DC.  This is a wrapper to gdi so that we can
* check for changes prior to sending notifications.
*
* History:
\***************************************************************************/

int xxxRealizePalette(
    HDC hdc)
{
    PWND           pwnd;
    DWORD          dwNumChanged;
    PWINDOWSTATION pwinsta;
    PDESKTOP       pdesk;
    TL             tlpwnd;

    dwNumChanged = GreRealizePalette(hdc);

    if (HIWORD(dwNumChanged) && IsDCCurrentPalette(hdc)) {

        pwnd = WindowFromCacheDC(hdc);

        /*
         * if there is no associated window, don't send the palette change
         * messages since this is a memory hdc.
         */
        if (pwnd != NULL) {
            /*
             * Ok, send WM_PALETTECHANGED message to everyone. The wParam
             * contains a handle to the currently active window.  Send
             * message to the desktop also, so things on the desktop bitmap
             * will paint ok.
             */
             ThreadLock(pwnd, &tlpwnd);
             xxxBroadcastPaletteChanged(pwnd, FALSE);
             ThreadUnlock(&tlpwnd);

            /*
             * Mark all other desktops as needing to send out
             * WM_PALETTECHANGED messages.
             */

            pwinsta = grpWinStaList;

            while (pwinsta != NULL) {
                pdesk = pwinsta->rpdeskList;
                while (pdesk != NULL) {
                    if (pdesk != pwnd->head.rpdesk) {
                        pdesk->dwDTFlags |= DTF_NEEDSPALETTECHANGED;
                    }
                    pdesk = pdesk->rpdeskNext;
                }
                pwinsta = pwinsta->rpwinstaNext;
            }

            GreRealizePalette(hdc);
        }
    }

    /*
     * Walk through the SPB list (the saved bitmaps under windows with the
     * CS_SAVEBITS style) discarding all bitmaps
     */
    if (HIWORD(dwNumChanged)) {
        FreeAllSpbs();
    }

    return LOWORD(dwNumChanged);
}

/***************************************************************************\
* xxxFlushPalette
*
* This resets the palette and lets the next foreground app grab the
* foreground palette.  This is called in such instances when we
* minimize a window.
*
* History:
* 31-Aug-1995  ChrisWil    Created.
\***************************************************************************/

VOID xxxFlushPalette(
    PWND pwnd)
{
    CheckLock(pwnd);
    /*
     * Broadcast the palette changed messages.
     */
    GreRealizeDefaultPalette(gpDispInfo->hdcScreen, TRUE);
    xxxBroadcastPaletteChanged(pwnd, TRUE);
}

/***************************************************************************\
* xxxBroadcastPaletteChanged
*
* RealizePalette passes FALSE for fForceDesktopso so that it does not
* cause a loop in case RealizePalette was called by the desktop window.
* In such a case we don't want to call RealizeDesktop. In all other cases
* we do want to go to RealizeDesktop to give the desktop a chance to
* reailze its palette or possibly just repaint.
*
* 04/22/97      vadimg      created
\***************************************************************************/

VOID xxxBroadcastPaletteChanged(PWND pwnd, BOOL fForceDesktop)
{
    PWND pwndDesk;
    HWND hwnd = HWq(pwnd);

    CheckLock(pwnd);

    pwndDesk = PWNDDESKTOP(pwnd);
    if (fForceDesktop || pwnd != pwndDesk) {
        TL tlpwndDesk;
        ThreadLockAlways(pwndDesk, &tlpwndDesk);
        xxxRealizeDesktop(pwndDesk);
        ThreadUnlock(&tlpwndDesk);
    }

    xxxSendNotifyMessage(PWND_BROADCAST, WM_PALETTECHANGED, (WPARAM)hwnd, 0L);
}
