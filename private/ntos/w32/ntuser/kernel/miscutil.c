/****************************************************************************\
* Module Name: minmax.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Misc util functions
*
* 10-25-90 MikeHar      Ported from Windows.
* 14-Feb-1991 mikeke    Added Revalidation code (None)
\****************************************************************************/

#include "precomp.h"
#pragma hdrstop


VOID ZapActiveAndFocus(VOID)
{
   PQ pq = PtiCurrent()->pq;

   Unlock(&pq->spwndActive);
   Unlock(&pq->spwndFocus);
}

VOID SetDialogPointer(PWND pwnd, LONG_PTR lPtr) {

    if ((pwnd->cbwndExtra < DLGWINDOWEXTRA)
            || TestWF(pwnd, WFSERVERSIDEPROC)
            || (PpiCurrent() != GETPTI(pwnd)->ppi)) {
        RIPMSG1(RIP_WARNING, "SetDialogPointer: Unexpected pwnd:%#p", pwnd);
        return;
    }

    ((PDIALOG)pwnd)->pdlg = (PDLG)lPtr;

    if (lPtr == 0) {
        pwnd->fnid |= FNID_CLEANEDUP_BIT;
        ClrWF(pwnd, WFDIALOGWINDOW);
    } else {
        if (pwnd->fnid == 0) {
            pwnd->fnid = FNID_DIALOG;
        }
        SetWF(pwnd, WFDIALOGWINDOW);
    }


}

BOOL _SetProgmanWindow(PWND pwnd) {

    PDESKTOPINFO pdeskinfo = GETDESKINFO(PtiCurrent());

    if (pwnd != NULL) {
        // Fail the call if another shell window exists
        if (pdeskinfo->spwndProgman != NULL)
            return(FALSE);
    }

    Lock(&pdeskinfo->spwndProgman, pwnd);

    return(TRUE);
}

BOOL _SetTaskmanWindow(PWND pwnd) {

    PDESKTOPINFO pdeskinfo = GETDESKINFO(PtiCurrent());

    if (pwnd != NULL) {
        // Fail the call if another shell window exists
        if (pdeskinfo->spwndTaskman != NULL)
            return(FALSE);
    }

    Lock(&pdeskinfo->spwndTaskman, pwnd);

    return(TRUE);
}

/***************************************************************************\
*
*  SetShellWindow()
*
*  Returns true if shell window is successfully set.  Note that we return
*  FALSE if a shell window already exists.  I.E., this works on a first
*  come, first serve basis.
*
*  We also do NOT allow child windows to be shell windows.  Other than that,
*  it's up to the caller to size her window appropriately.
*
*  The pwndBkGnd is provided for the explorer shell.  Since the shellwnd
*  and the window which does the drawing of background wallpapers are
*  different, we need to provide means by which we can draw directly on
*  the background window during hung-app drawing.  The pwnd and pwndBkGnd
*  will be identical if called through the SetShellWindow() api.
*
*
\***************************************************************************/
BOOL xxxSetShellWindow(PWND pwnd, PWND pwndBkGnd)
{
    PTHREADINFO  ptiCurrent = PtiCurrent();
    PDESKTOPINFO pdeskinfo = GETDESKINFO(ptiCurrent);

    PPROCESSINFO ppiShellProcess;

    UserAssert(pwnd);

    /*
     * Fail the call if another shell window exists
     */
    if (pdeskinfo->spwndShell != NULL)
        return(FALSE);

    /*
     * The shell window must be
     *      (1) Top-level
     *      (2) Unowned
     *      (3) Not topmost
     */
    if (TestwndChild(pwnd)             ||
            (pwnd->spwndOwner != NULL) ||
            TestWF(pwnd, WEFTOPMOST)) {

        RIPMSG0(RIP_WARNING, "xxxSetShellWindow: Invalid type of window");
        return(FALSE);
    }

    /*
     * Chicago has a totally different input model which has special code
     * that checks for Ctrl-Esc and sends it to the shell.  We can get
     * the same functionality, without totally re-writing our input model
     * by just automatically installing the Ctrl-Esc as a hotkey for the
     * shell window.  The hotkey delivery code has a special case which
     * turns this into a WM_SYSCOMMAND message instead of a WM_HOTKEY
     * message.
     *
     * We don't both checking for failure.  Somebody could already have
     * a Ctrl-Esc handler installed.
     */
    _RegisterHotKey(pwnd,SC_TASKLIST,MOD_CONTROL,VK_ESCAPE);

    /*
     * This is the shell window wright.
     * So get the process id for the shell.
     */
    ppiShellProcess = GETPTI(pwnd)->ppi;

    /*
     * Set the shell process id to the desktop only if it's the first instance
     */
    if ((ppiShellProcess != NULL) && (pdeskinfo->ppiShellProcess == NULL)) {
        pdeskinfo->ppiShellProcess = ppiShellProcess;
    }

    Lock(&pdeskinfo->spwndShell, pwnd);
    Lock(&pdeskinfo->spwndBkGnd, pwndBkGnd);

    /*
     * Push window to bottom of stack.
     */
    SetWF(pdeskinfo->spwndShell, WFBOTTOMMOST);
    xxxSetWindowPos(pdeskinfo->spwndShell,
                    PWND_BOTTOM,
                    0,
                    0,
                    0,
                    0,
                    SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

    return(TRUE);
}




/***************************************************************************\
* _InitPwSB()
*
* History:
* 10-23-90 MikeHar Ported from WaWaWaWindows.
* 11-28-90 JimA    Changed to int *
* 01-21-91 IanJa   Prefix '_' denoting exported function (although not API)
\***************************************************************************/

PSBINFO _InitPwSB(
    PWND pwnd)
{
    if (pwnd->pSBInfo) {

        /*
         * If memory is already allocated, don't bother to do it again.
         */
        return pwnd->pSBInfo;
    }

    pwnd->pSBInfo = (PSBINFO)DesktopAlloc(pwnd->head.rpdesk,
                                          sizeof(SBINFO),
                                          DTAG_SBINFO);

    if (pwnd->pSBInfo != NULL) {

        /*
         *  rgw[0] = 0;  */  /* LPTR zeros all 6 words
         */

        /*
         *  rgw[1] = 0;
         */

        /*
         *  rgw[3] = 0;
         */

        /*
         *  rgw[4] = 0;
         */
        pwnd->pSBInfo->Vert.posMax = 100;
        pwnd->pSBInfo->Horz.posMax = 100;
    }

    return pwnd->pSBInfo;
}

