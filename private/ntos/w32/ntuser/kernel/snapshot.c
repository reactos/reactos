/****************************** Module Header ******************************\
* Module Name: snapshot.c
*
* Screen/Window SnapShotting Routines
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* History:
* 26-Nov-1991 DavidPe   Ported from Win 3.1 sources
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* xxxSnapWindow
*
* Effects: Snaps either the desktop hwnd or the active front most window. If
* any other window is specified, we will snap it but it will be clipped.
*
\***************************************************************************/

BOOL xxxSnapWindow(
    PWND pwnd)
{
    PTHREADINFO    ptiCurrent;
    RECT           rc;
    HDC            hdcScr = NULL;
    HDC            hdcMem = NULL;
    BOOL           fRet;
    HBITMAP        hbmOld;
    HBITMAP        hbm;
    HANDLE         hPal;
    LPLOGPALETTE   lppal;
    int            palsize;
    int            iFixedPaletteEntries;
    BOOL           fSuccess;
    PWND           pwndT;
    TL             tlpwndT;
    PWINDOWSTATION pwinsta;
    TL             tlpwinsta;

    CheckLock(pwnd);
    UserAssert(pwnd);

    ptiCurrent = PtiCurrent();

    /*
     * If this is a thread of winlogon, don't do the snapshot.
     */
    if (GetCurrentProcessId() == gpidLogon)
        return FALSE;

    /*
     * Get the affected windowstation
     */
    if (!NT_SUCCESS(ReferenceWindowStation(
            PsGetCurrentThread(),
            NULL,
            WINSTA_READSCREEN,
            &pwinsta,
            TRUE)) ||
            pwinsta->dwWSF_Flags & WSF_NOIO) {
        return FALSE;
    }

    /*
     * If the window is on another windowstation, do nothing
     */
    if (pwnd->head.rpdesk->rpwinstaParent != pwinsta)
        return FALSE;

    /*
     * Get the parent of any child windows.
     */
    while ((pwnd != NULL) && TestWF(pwnd, WFCHILD)) {
        pwnd = pwnd->spwndParent;
    }

    /*
     * Lock the windowstation before we leave the critical section
     */
    ThreadLockWinSta(ptiCurrent, pwinsta, &tlpwinsta);

    /*
     * Open the clipboard and empty it.
     *
     * pwndDesktop is made the owner of the clipboard, instead of the
     * currently active window; -- SANKAR -- 20th July, 1989 --
     */
    pwndT = ptiCurrent->rpdesk->pDeskInfo->spwnd;
    ThreadLockWithPti(ptiCurrent, pwndT, &tlpwndT);
    fSuccess = xxxOpenClipboard(pwndT, NULL);
    ThreadUnlock(&tlpwndT);

    if (!fSuccess) {
        ThreadUnlockWinSta(ptiCurrent, &tlpwinsta);
        return FALSE;
    }

    xxxEmptyClipboard(pwinsta);

    /*
     * Use the whole window.
     */
    CopyRect(&rc, &pwnd->rcWindow);

    /*
     * Only snap what is on the screen.
     */
    if (!IntersectRect(&rc, &rc, &gpDispInfo->rcScreen)) {
        fRet = FALSE;
        goto SnapExit;
    }

    rc.right -= rc.left;
    rc.bottom -= rc.top;

    /*
     * Figure out how far offset from window origin visible part is
     */
    if (pwnd != PWNDDESKTOP(pwnd)) {
        rc.left -= pwnd->rcWindow.left;
        rc.top -= pwnd->rcWindow.top;
    }

    /*
     * Get the entire window's DC.
     */
    hdcScr = _GetWindowDC(pwnd);
    if (!hdcScr)
        goto MemoryError;

    /*
     * Create the memory DC.
     */
    hdcMem = GreCreateCompatibleDC(hdcScr);
    if (!hdcMem)
        goto MemoryError;

    /*
     * Create the destination bitmap.  If it fails, then attempt
     * to create a monochrome bitmap.
     * Did we have enough memory?
     */

    if (SYSMET(SAMEDISPLAYFORMAT)) {
        hbm = GreCreateCompatibleBitmap(hdcScr, rc.right, rc.bottom);
    } else {
        hbm = GreCreateBitmap(rc.right, rc.bottom, 1, gpDispInfo->BitCountMax, NULL);
    }

    if (!hbm) {
        hbm = GreCreateBitmap(rc.right, rc.bottom, 1, 1, NULL);
        if (!hbm)
            goto MemoryError;
    }

    /*
     * Select the bitmap into the memory DC.
     */
    hbmOld = GreSelectBitmap(hdcMem, hbm);

    /*
     * Snap!!!
     * Check the return value because the process taking the snapshot
     * may not have access to read the screen.
     */
    fRet = GreBitBlt(hdcMem, 0, 0, rc.right, rc.bottom, hdcScr, rc.left, rc.top, SRCCOPY | CAPTUREBLT, 0);

    /*
     * Restore the old bitmap into the memory DC.
     */
    GreSelectBitmap(hdcMem, hbmOld);

    /*
     * If the blt failed, leave now.
     */
    if (!fRet)
        goto SnapExit;

    _SetClipboardData(CF_BITMAP, hbm, FALSE, TRUE);

    /*
     * If this is a palette device, let's throw the current system palette
     * into the clipboard also.  Useful if the user just snapped a window
     * containing palette colors...
     */
    if (TEST_PUSIF(PUSIF_PALETTEDISPLAY)) {

        int i;
        int iPalSize;

        palsize = GreGetDeviceCaps(hdcScr, SIZEPALETTE);

        /*
         * Determine the number of system colors.
         */
        if (GreGetSystemPaletteUse(hdcScr) == SYSPAL_STATIC)
            iFixedPaletteEntries = GreGetDeviceCaps(hdcScr, NUMRESERVED);
        else
            iFixedPaletteEntries = 2;

        lppal = (LPLOGPALETTE)UserAllocPoolWithQuota(
                (LONG)(sizeof(LOGPALETTE) + sizeof(PALETTEENTRY) * palsize),
                TAG_CLIPBOARD);

        if (lppal != NULL) {
            lppal->palVersion = 0x300;
            lppal->palNumEntries = (WORD)palsize;

            if (GreGetSystemPaletteEntries(hdcScr,
                                           0,
                                           palsize,
                                           lppal->palPalEntry)) {

                iPalSize = palsize - iFixedPaletteEntries / 2;

                for (i = iFixedPaletteEntries / 2; i < iPalSize; i++) {

                    /*
                     * Any non system palette enteries need to have the NOCOLLAPSE
                     * flag set otherwise bitmaps containing different palette
                     * indices but same colors get messed up.
                     */
                    lppal->palPalEntry[i].peFlags = PC_NOCOLLAPSE;
                }

                if (hPal = GreCreatePalette(lppal))
                    _SetClipboardData(CF_PALETTE, hPal, FALSE, TRUE);
            }

            UserFreePool(lppal);
        }
    }
    PlayEventSound(USER_SOUND_SNAPSHOT);

    fRet = TRUE;

SnapExit:

    /*
     * Release the window/client DC.
     */
     if (hdcScr) {
         _ReleaseDC(hdcScr);
     }

    xxxCloseClipboard(pwinsta);
    Unlock(&pwinsta->spwndClipOwner);

    /*
     * Delete the memory DC.
     */
    if (hdcMem) {
        GreDeleteDC(hdcMem);
    }

    ThreadUnlockWinSta(ptiCurrent, &tlpwinsta);

    return fRet;

MemoryError:
    /*
     * Display an error message box.
     */
    ClientNoMemoryPopup();
    fRet = FALSE;
    goto SnapExit;
}
