/****************************** Module Header ******************************\
* Module Name: clipbrd.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Clipboard code.
*
* History:
* 18-Nov-1990 ScottLu   Ported from Win3.
* 18-Nov-1990 ScottLu   Added revalidation code
* 11-Feb-1991 JimA      Added access checks
* 20-Jun-1995 ChrisWil  Merged Chicago functionality.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

#undef DUMMY_TEXT_HANDLE
#define DUMMY_TEXT_HANDLE       (HANDLE)0x0001        // must be first dummy
#define DUMMY_DIB_HANDLE        (HANDLE)0x0002
#define DUMMY_METARENDER_HANDLE (HANDLE)0x0003
#define DUMMY_METACLONE_HANDLE  (HANDLE)0x0004
#define DUMMY_MAX_HANDLE        (HANDLE)0x0004        // must be last dummy

#define PRIVATEFORMAT       0
#define GDIFORMAT           1
#define HANDLEFORMAT        2
#define METAFILEFORMAT      3

#define IsTextHandle(fmt, hdata)       \
    (((hdata) != DUMMY_TEXT_HANDLE) && \
     (((fmt) == CF_TEXT) || ((fmt) == CF_OEMTEXT) || ((fmt) == CF_UNICODETEXT)))

#define IsDibHandle(fmt, hdata)      \
    (((fmt) == CF_DIB) && ((hdata) != DUMMY_DIB_HANDLE))
#define IsMetaDummyHandle(hdata)     \
    ((hdata == DUMMY_METACLONE_HANDLE) || (hdata == DUMMY_METARENDER_HANDLE))

/**************************************************************************\
* CheckClipboardAccess
*
* Perform access check on the clipboard.  Special case CSRSS threads
* so that console windows on multiple windowstations will have
* the correct access.
*
* 04-Jul-1995 JimA  Created
\**************************************************************************/

PWINDOWSTATION CheckClipboardAccess(void)
{
    NTSTATUS Status;
    PWINDOWSTATION pwinsta;
    BOOL fUseDesktop;
    PTHREADINFO pti;

    pti = PtiCurrentShared();

    /*
     * CSR process use to have NULL pwinsta. Now that it's assigned to
     * the services windowstation we have to explicitly use the desktop
     * for checking the access.
     */
    fUseDesktop = (pti->TIF_flags & TIF_CSRSSTHREAD) ? TRUE : FALSE;

    Status =  ReferenceWindowStation(PsGetCurrentThread(),
                                     NULL,
                                     WINSTA_ACCESSCLIPBOARD,
                                     &pwinsta,
                                     fUseDesktop);
    if (!NT_SUCCESS(Status)) {
        RIPNTERR0(Status, RIP_WARNING,"Access to clipboard denied.");
        return NULL;
    }

    return pwinsta;
}

/**************************************************************************\
* ConvertMemHandle
*
* Converts data to a clipboard-memory-handle.  This special handle
* contains the size-of-data in the first DWORD.  The second DWORD points
* back to the block.
*
* History:
\**************************************************************************/

HANDLE _ConvertMemHandle(
    LPBYTE ccxlpData,
    int    cbData)
{
    PCLIPDATA pClipData;

    pClipData = HMAllocObject(NULL,
                              NULL,
                              TYPE_CLIPDATA,
                              FIELD_OFFSET(CLIPDATA, abData) + cbData);

    if (pClipData == NULL)
        return NULL;

    pClipData->cbData = cbData;

    try {
        RtlCopyMemory(&pClipData->abData, ccxlpData, cbData);
    } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
        HMFreeObject(pClipData);
        return NULL;
    }

    return PtoHq(pClipData);
}

/***************************************************************************\
* _xxxOpenClipboard (API)
*
* External routine. Opens the clipboard for reading/writing, etc.
*
* History:
* 18-Nov-1990 ScottLu   Ported from Win3.
* 11-Feb-1991 JimA      Added access checks.
\***************************************************************************/

BOOL xxxOpenClipboard(
    PWND   pwnd,
    LPBOOL lpfEmptyClient)
{
    PTHREADINFO    pti;
    PWINDOWSTATION pwinsta;

    CheckLock(pwnd);

    if (lpfEmptyClient != NULL)
        *lpfEmptyClient = FALSE;

    /*
     * Blow it off is the caller does not have the proper access rights
     */
    if ((pwinsta = CheckClipboardAccess()) == NULL)
        return FALSE;

    pti = PtiCurrent();

    /*
     * If this thread already has the clipboard open, then there's no
     * need to proceed further.
     */
    if ((pwnd == pwinsta->spwndClipOpen) && (pti == pwinsta->ptiClipLock))
        return TRUE;

    if ((pwnd != pwinsta->spwndClipOpen) && (pwinsta->ptiClipLock != NULL)) {

#if DBG
        /*
         * Only rip if the current-thread doesn't have the clipboard
         * open.
         */
        if (pti != pwinsta->ptiClipLock) {

            RIPMSG0(RIP_VERBOSE,
                  "Clipboard: OpenClipboard already out by another thread");
        }
#endif
        return FALSE;
    }

    /*
     * If the window is already destroyed, then the clipboard will not
     * get disowned when the window is finally unlocked.  FritzS
     */

    UserAssert((pwnd == NULL ) || (!TestWF(pwnd, WFDESTROYED)));

    Lock(&pwinsta->spwndClipOpen, pwnd);
    pwinsta->ptiClipLock = pti;

    /*
     * The client side clipboard cache needs to be emptied if this thread
     * doesn't own the data in the clipboard.
     * Note: We only empty the 16bit clipboard if a 32bit guy owns the
     * clipboard.
     * Harvard graphics uses a handle put into the clipboard
     * by another app, and it expects that handle to still be good after the
     * clipboard has opened and closed mutilple times
     * There may be a problem here if app A puts in format foo and app B opens
     * the clipboard for format foo and then closes it and opens it again
     * format foo client side handle may not be valid.  We may need some
     * sort of uniqueness counter to tell if the client side handle is
     * in sync with the server and always call the server or put the data
     * in share memory with some semaphore.
     *
     * pwinsta->spwndClipOwner: window that last called EmptyClipboard
     * pwinsta->ptiClipLock   : thread that currently has the clipboard open
     */
    if (lpfEmptyClient != NULL) {

        if (!(pti->TIF_flags & TIF_16BIT) ||
            (pti->ppi->iClipSerialNumber != pwinsta->iClipSerialNumber)) {

            *lpfEmptyClient = (pwinsta->spwndClipOwner == NULL) ||
                    (pwinsta->ptiClipLock->ppi !=
                    GETPTI(pwinsta->spwndClipOwner)->ppi);

            pti->ppi->iClipSerialNumber = pwinsta->iClipSerialNumber;
        }
    }

    return TRUE;
}

/***************************************************************************\
* xxxDrawClipboard
*
* Tells the clipboard viewers to redraw.
*
* History:
* 18-Nov-1990 ScottLu   Ported from Win3.
\***************************************************************************/

VOID xxxDrawClipboard(
    PWINDOWSTATION pwinsta)
{
    /*
     * This is what pwinsta->fClipboardChanged is for - to tell us to
     * update the clipboard viewers.
     */
    pwinsta->fClipboardChanged = FALSE;

    if (pwinsta->ptiDrawingClipboard == NULL && pwinsta->spwndClipViewer != NULL) {

        TL tlpwndClipViewer;

        /*
         * Send the message that causes clipboard viewers to redraw.
         * Remember that we're sending this message so we don't send
         * this message twice.
         */
        pwinsta->ptiDrawingClipboard = PtiCurrent();
        ThreadLockAlways(pwinsta->spwndClipViewer, &tlpwndClipViewer);

        if (!(PtiCurrent()->TIF_flags & TIF_16BIT)) {
            /*
             * Desynchronize 32 bit apps.
             */
            xxxSendNotifyMessage(pwinsta->spwndClipViewer,
                                 WM_DRAWCLIPBOARD,
                                 (WPARAM)HW(pwinsta->spwndClipOwner),
                                 0L);
        } else {
            xxxSendMessage(pwinsta->spwndClipViewer,
                           WM_DRAWCLIPBOARD,
                           (WPARAM)HW(pwinsta->spwndClipOwner),
                           0L);
        }

        ThreadUnlock(&tlpwndClipViewer);
        pwinsta->ptiDrawingClipboard = NULL;
    }
}

/***************************************************************************\
* PasteScreenPalette
*
* Creates temp palette with all colors of screen, and sticks it on
* clipboard.
*
* 20-Jun-1995 ChrisWil  Ported from Chicago.
\***************************************************************************/

VOID PasteScreenPalette(
    PWINDOWSTATION pwinsta)
{
    int          irgb;
    int          crgbPal;
    LPLOGPALETTE lppal;
    HPALETTE     hpal = NULL;
    int          crgbFixed;

    UserAssert(TEST_PUSIF(PUSIF_PALETTEDISPLAY));

    /*
     * Use current state of screen.
     */
    crgbPal = GreGetDeviceCaps(gpDispInfo->hdcScreen, SIZEPALETTE);

    if (GreGetSystemPaletteUse(gpDispInfo->hdcScreen) == SYSPAL_STATIC) {
        crgbFixed = GreGetDeviceCaps(gpDispInfo->hdcScreen, NUMRESERVED) / 2;
    } else {
        crgbFixed = 1;
    }

    lppal = (LPLOGPALETTE)UserAllocPool(sizeof(LOGPALETTE) +
                                              (sizeof(PALETTEENTRY) * crgbPal),
                                              TAG_CLIPBOARD);

    if (lppal == NULL)
        return;

    lppal->palVersion    = 0x300;
    lppal->palNumEntries = (WORD)crgbPal;

    if (GreGetSystemPaletteEntries(gpDispInfo->hdcScreen, 0, crgbPal, lppal->palPalEntry)) {

        crgbPal -= crgbFixed;

        for (irgb = crgbFixed; irgb < crgbPal; irgb++) {

            /*
             * Any non-system palette entries need to have PC_NOCOLLAPSE
             * flag set.
             */
            lppal->palPalEntry[irgb].peFlags = PC_NOCOLLAPSE;
        }

        hpal = GreCreatePalette(lppal);
    }

    UserFreePool((HANDLE)lppal);

    if (hpal) {
        InternalSetClipboardData(pwinsta, CF_PALETTE, hpal, FALSE, TRUE);
        GreSetPaletteOwner(hpal, OBJECT_OWNER_PUBLIC);
    }
}

/***************************************************************************\
* MungeClipData
*
* When clipboard is closed, we translate data to more independent format
* and pastes dummy handles if necessary.
*
* 20-Jun-1995 ChrisWil  Ported from Chicago.
\***************************************************************************/

VOID MungeClipData(
    PWINDOWSTATION pwinsta)
{
    PCLIP  pOEM;
    PCLIP  pTXT;
    PCLIP  pUNI;
    PCLIP  pBMP;
    PCLIP  pDIB;
    PCLIP  pDV5;

    PCLIP pClip;

    /*
     * If only CF_OEMTEXT, CF_TEXT or CF_UNICODE are available, make the
     * other formats available too.
     */
    pTXT = FindClipFormat(pwinsta, CF_TEXT);
    pOEM = FindClipFormat(pwinsta, CF_OEMTEXT);
    pUNI = FindClipFormat(pwinsta, CF_UNICODETEXT);

    if (pTXT != NULL || pOEM != NULL || pUNI != NULL) {

        /*
         * Make dummy text formats.
         */
        if (!FindClipFormat(pwinsta, CF_LOCALE)) {

            /*
             * CF_LOCALE not currently stored.  Save the locale
             * information while it's still available.
             */
            PTHREADINFO ptiCurrent = PtiCurrent();
            DWORD       lcid;
            DWORD       lang;
            HANDLE      hLocale;

            /*
             * The LOCALE format is an HGLOBAL to a DWORD lcid.  The
             * spklActive->hkl actually stores more than just the locale,
             * so we need to mask the value.
             * (#99321 spklActive is NULL before winlogon loads kbd layouts)
             */
            if (ptiCurrent->spklActive) {
                lang = HandleToUlong(ptiCurrent->spklActive->hkl);

                lcid = MAKELCID(LOWORD(lang), SORT_DEFAULT);

                if (hLocale = _ConvertMemHandle((LPBYTE)&lcid, sizeof(DWORD))) {
                    if (!InternalSetClipboardData(pwinsta,
                                                  CF_LOCALE,
                                                  hLocale,
                                                  FALSE,
                                                  TRUE)) {
                        PVOID pObj;

                        pObj = HMValidateHandleNoRip(hLocale, TYPE_CLIPDATA);
                        if (pObj != NULL) {
                            HMFreeObject(pObj);
                        }
                    }
                }
            }
        }

        if (pTXT == NULL)
            InternalSetClipboardData(pwinsta,
                                     CF_TEXT,
                                     (HANDLE)DUMMY_TEXT_HANDLE,
                                     FALSE,
                                     TRUE);

        if (pOEM == NULL)
            InternalSetClipboardData(pwinsta,
                                     CF_OEMTEXT,
                                     (HANDLE)DUMMY_TEXT_HANDLE,
                                     FALSE,
                                     TRUE);

        if (pUNI == NULL)
            InternalSetClipboardData(pwinsta,
                                     CF_UNICODETEXT,
                                     (HANDLE)DUMMY_TEXT_HANDLE,
                                     FALSE,
                                     TRUE);
    }

    /*
     * For the metafile formats we also want to add its cousin if its
     * not alread present.  We pass the same data because GDI knows
     * how to convert between the two.
     */
    if (!FindClipFormat(pwinsta, CF_METAFILEPICT) &&
        (pClip = FindClipFormat(pwinsta, CF_ENHMETAFILE))) {

        InternalSetClipboardData(pwinsta,
                                CF_METAFILEPICT,
                                pClip->hData ? DUMMY_METACLONE_HANDLE :
                                    DUMMY_METARENDER_HANDLE,
                                FALSE,
                                TRUE);

    } else if (!FindClipFormat(pwinsta, CF_ENHMETAFILE) &&
               (pClip = FindClipFormat(pwinsta, CF_METAFILEPICT))) {

        InternalSetClipboardData(pwinsta,
                                 CF_ENHMETAFILE,
                                 pClip->hData ? DUMMY_METACLONE_HANDLE :
                                     DUMMY_METARENDER_HANDLE,
                                 FALSE,
                                 TRUE);
    }

    /*
     * Convert bitmap formats.
     *
     * If only CF_BITMAP, CF_DIB or CF_DIBV5 are available, make the
     * other formats available too. And check palette if screen is
     * palette managed.
     */
    pBMP = FindClipFormat(pwinsta, CF_BITMAP);
    pDIB = FindClipFormat(pwinsta, CF_DIB);
    pDV5 = FindClipFormat(pwinsta, CF_DIBV5);

    if (pBMP != NULL || pDIB != NULL || pDV5 != NULL) {

        /*
         * If there is no CF_BITMAP, set dummy.
         */
        if (pBMP == NULL) {
            InternalSetClipboardData(pwinsta,
                                     CF_BITMAP,
                                     DUMMY_DIB_HANDLE,
                                     FALSE,
                                     TRUE);
        }

        /*
         * If there is no CF_DIB, set dummy.
         */
        if (pDIB == NULL) {
            InternalSetClipboardData(pwinsta,
                                     CF_DIB,
                                     DUMMY_DIB_HANDLE,
                                     FALSE,
                                     TRUE);
        }

        /*
         * If there is no CF_DIBV5, set dummy.
         */
        if (pDV5 == NULL) {
            InternalSetClipboardData(pwinsta,
                                     CF_DIBV5,
                                     DUMMY_DIB_HANDLE,
                                     FALSE,
                                     TRUE);
        }

        if (TEST_PUSIF(PUSIF_PALETTEDISPLAY) &&
            !FindClipFormat(pwinsta, CF_PALETTE)) {

            /*
             * Displays are palettized and there is no palette data in clipboard, yet.
             */
            if (pDIB != NULL || pDV5 != NULL)
            {
                /*
                 * Store a dummy dib and palette (if one not already there).
                 */
                InternalSetClipboardData(pwinsta,
                                         CF_PALETTE,
                                         DUMMY_DIB_HANDLE,
                                         FALSE,
                                         TRUE);
            } else {
                /*
                 * if only CF_BITMAP is avalilable, perserve Screen palette.
                 */
                PasteScreenPalette(pwinsta);
            }
        }
    }

    return;
}

/***************************************************************************\
* xxxCloseClipboard (API)
*
* External routine. Closes the clipboard.
*
* Note: we do not delete any client side handle at this point.  Many apps,
* WordPerfectWin, incorrectly use handles after they have put them in the
* clipboard.  They also put things in the clipboard without becoming the
* clipboard owner because they want to add RichTextFormat to the normal
* text that is already in the clipboard from another app.
*
* History:
* 18-Nov-1990 ScottLu   Ported from Win3.
* 22-Aug-1991 EichiM    Unicode enabling
* 20-Jun-1995 ChrisWil  Merged Chicago functionality.
\***************************************************************************/

BOOL xxxCloseClipboard(
    PWINDOWSTATION pwinsta)
{
    PTHREADINFO ptiCurrent;
    TL          tlpwinsta;

    if ((pwinsta == NULL) && ((pwinsta = CheckClipboardAccess()) == NULL)) {
        return FALSE;
    }

    /*
     * If the current thread does not have the clipboard open, return
     * FALSE.
     */
    ptiCurrent = PtiCurrent();

    if (pwinsta->ptiClipLock != ptiCurrent) {
        RIPERR0(ERROR_CLIPBOARD_NOT_OPEN, RIP_WARNING, "xxxCloseClipboard not open");
        return FALSE;
    }

    ThreadLockWinSta(ptiCurrent, pwinsta, &tlpwinsta);

    /*
     * Convert data to independent formats.
     */
    if (pwinsta->fClipboardChanged)
        MungeClipData(pwinsta);

    /*
     * Release the clipboard explicitly after we're finished calling
     * SetClipboardData().
     */
    Unlock(&pwinsta->spwndClipOpen);
    pwinsta->ptiClipLock = NULL;

    /*
     * Notify any clipboard viewers that the clipboard contents have
     * changed.
     */
    if (pwinsta->fClipboardChanged)
        xxxDrawClipboard(pwinsta);

    ThreadUnlockWinSta(ptiCurrent, &tlpwinsta);

    return TRUE;
}

/***************************************************************************\
* _EnumClipboardFormats (API)
*
* This routine takes a clipboard format and gives the next format back to
* the application. This should only be called while the clipboard is open
* and locked so the formats don't change around.
*
* History:
* 18-Nov-1990 ScottLu   Ported from Win3.
\***************************************************************************/

UINT _EnumClipboardFormats(
    UINT fmt)
{
    PWINDOWSTATION pwinsta;
    UINT           fmtRet;

    if ((pwinsta = CheckClipboardAccess()) == NULL)
        return 0;

    /*
     * If the current thread doesn't have the clipboard open or if there
     * is no clipboard, return 0 for no formats.
     */
    if (pwinsta->ptiClipLock != PtiCurrent()) {
        RIPERR0(ERROR_CLIPBOARD_NOT_OPEN, RIP_WARNING, "EnumClipboardFormat: clipboard not open");
        return 0;
    }

    fmtRet = 0;

    if (pwinsta->pClipBase != NULL) {

        PCLIP pClip;

        /*
         * Find the next clipboard format.  If the format is 0, start from
         * the beginning.
         */
        if (fmt != 0) {

            /*
             * Find the next clipboard format.  NOTE that this routine locks
             * the clipboard handle and updates pwinsta->pClipBase with the
             * starting address of the clipboard.
             */
            if ((pClip = FindClipFormat(pwinsta, fmt)) != NULL)
                pClip++;

        } else {
            pClip = pwinsta->pClipBase;
        }

        /*
         * Find the new format before unlocking the clipboard.
         */
        if (pClip && (pClip < &pwinsta->pClipBase[pwinsta->cNumClipFormats])) {

            fmtRet = pClip->fmt;
        }
    }

    /*
     * Return the new clipboard format.
     */
    return fmtRet;
}

/***************************************************************************\
* UT_GetFormatType
*
* Given the clipboard format, return the handle type.
*
* Warning:  Private formats, eg CF_PRIVATEFIRST, return PRIVATEFORMAT
* unlike Win 3.1 which has a bug and returns HANDLEFORMAT.  And they
* would incorrectly free the handle.  Also they would NOT free GDIOBJFIRST
* objects.
*
* History:
* 18-Nov-1990 ScottLu   Ported from Win3.
\***************************************************************************/

int UT_GetFormatType(
    PCLIP pClip)
{
    switch (pClip->fmt) {

    case CF_BITMAP:
    case CF_DSPBITMAP:
    case CF_PALETTE:
        return GDIFORMAT;

    case CF_METAFILEPICT:
    case CF_DSPMETAFILEPICT:
    case CF_ENHMETAFILE:
    case CF_DSPENHMETAFILE:
        return METAFILEFORMAT;

    case CF_OWNERDISPLAY:
        return PRIVATEFORMAT;

    default:
        return HANDLEFORMAT;
    }
}

/***************************************************************************\
* UT_FreeCBFormat
*
* Free the data in the pass clipboard structure.
*
* History:
* 18-Nov-1990 ScottLu   Ported from Win3.
\***************************************************************************/

VOID UT_FreeCBFormat(
    PCLIP pClip)
{
    PVOID pObj;

    /*
     * No Data, then no point.
     */
    if (pClip->hData == NULL)
        return;

    /*
     * Free the object given the type.
     */
    switch (UT_GetFormatType(pClip)) {

    case METAFILEFORMAT:

        /*
         * GDI stores the metafile on the server side for the clipboard.
         * Notify the GDI server to free the metafile data.
         */
        if (!IsMetaDummyHandle(pClip->hData)) {
            GreDeleteServerMetaFile(pClip->hData);
        }
        break;

    case HANDLEFORMAT:

        /*
         * It's a simple global object.  Text/Dib handles can be
         * dummy handles, so check for those first.  We need to
         * perform extra-checks on the format since HANDLEFORMATS
         * are the default-type.  We only want to delete those obects
         * we can quarentee are handle-types.
         */
        if ((pClip->hData != DUMMY_TEXT_HANDLE) &&
            (pClip->hData != DUMMY_DIB_HANDLE)) {

            pObj = HMValidateHandleNoRip(pClip->hData, TYPE_CLIPDATA);
            if (pObj) {
                HMFreeObject(pObj);
            }
        }
        break;

    case GDIFORMAT:

        /*
         * Bitmaps can be marked as dummy-handles.
         */
        if (pClip->hData != DUMMY_DIB_HANDLE) {
            GreDeleteObject(pClip->hData);
        }
        break;

    case PRIVATEFORMAT:

        /*
         * Destroy the private data here if it is a global handle: we
         * aren't destroying the client's copy here, only the server's,
         * which nobody wants (including the server!)
         */
        if (pClip->fGlobalHandle) {
            pObj = HMValidateHandleNoRip(pClip->hData, TYPE_CLIPDATA);
            if (pObj) {
                HMFreeObject(pObj);
            }
        }
        break;
    }
}

/***************************************************************************\
* xxxSendClipboardMessage
*
* Helper routine that sends a notification message to the clipboard owner.
*
* History:
* 18-Nov-1990 ScottLu   Ported from Win3.
\***************************************************************************/

VOID xxxSendClipboardMessage(
    PWINDOWSTATION pwinsta,
    UINT           message)
{
    TL      tlpwndClipOwner;
    LONG_PTR dwResult;
    LRESULT lRet;

    if (pwinsta->spwndClipOwner != NULL) {

        PWND    pwndClipOwner = pwinsta->spwndClipOwner;

        ThreadLockAlways(pwndClipOwner, &tlpwndClipOwner);

        /*
         * We use SendNotifyMessage so the apps don't have to synchronize
         * but some 16 bit apps break because of the different message
         * ordering so we allow 16 bit apps to synchronize to other apps
         * Word 6 and Excel 5 with OLE.  Do a copy in Word and then another
         * copy in Excel and Word faults.
         */
        if ((message == WM_DESTROYCLIPBOARD) &&
            !(PtiCurrent()->TIF_flags & TIF_16BIT)) {

            /*
             * Let the app think it's the clipboard owner during
             * the processing of this message by waiting for it
             * to be processed before setting the new owner.
             */
            lRet = xxxSendMessageTimeout(
                    pwndClipOwner,
                    WM_DESTROYCLIPBOARD,
                    0,
                    0L,
                    SMTO_ABORTIFHUNG | SMTO_NORMAL,
                    5 * 1000,
                    &dwResult);

            if (lRet == 0) {

                /*
                 * The message timed out and wasn't sent, so
                 * let the app handle it when it's ready.
                 */
                RIPMSG0(RIP_WARNING, "Sending WM_DESTROYCLIPBOARD timed-out, resending via SendNotifyMessage");
                xxxSendNotifyMessage(pwndClipOwner, WM_DESTROYCLIPBOARD, 0, 0L);
            }

        } else {

            xxxSendMessage(pwndClipOwner, message, 0, 0L);
        }

        ThreadUnlock(&tlpwndClipOwner);
    }

}

/***************************************************************************\
* xxxEmptyClipboard (API)
*
* Empties the clipboard contents if the current thread has the clipboard
* open.
*
* History:
* 18-Nov-1990 ScottLu   Ported from Win3.
\***************************************************************************/

BOOL xxxEmptyClipboard(
    PWINDOWSTATION pwinsta)
{
    TL          tlpwinsta;
    PCLIP       pClip;
    int         cFmts;
    BOOL        fDying;
    PTHREADINFO ptiCurrent = (PTHREADINFO)(W32GetCurrentThread());
    BOOL bInternal = !(pwinsta == NULL);

    /*
     * Check access.
     */
    if ((pwinsta == NULL) && ((pwinsta = CheckClipboardAccess()) == NULL))
        return FALSE;

    /*
     * If the current thread doesn't have the clipboard open, it can't be
     * be emptied!
     */

    if (ptiCurrent == NULL) {
        UserAssert(bInternal);
    }

    if (!bInternal) {
        if (pwinsta->ptiClipLock != ptiCurrent) {
            RIPERR0(ERROR_CLIPBOARD_NOT_OPEN, RIP_WARNING, "xxxEmptyClipboard: clipboard not open");
            return FALSE;
        }
    }

    /*
     * Only send messages at logoff.
     */
    fDying = pwinsta->dwWSF_Flags & WSF_DYING;

    if (!fDying && ptiCurrent) {
        ThreadLockWinSta(ptiCurrent, pwinsta, &tlpwinsta);

        /*
         * Let the clipboard owner know that the clipboard is
         * being destroyed.
         */
        xxxSendClipboardMessage(pwinsta, WM_DESTROYCLIPBOARD);
    }

    if ((pClip = pwinsta->pClipBase) != NULL) {

        /*
         * Loop through all the clipboard entries and free their data
         * objects.  Only call DeleteAtom for real atoms.
         */
        for (cFmts = pwinsta->cNumClipFormats; cFmts-- != 0;) {

            if ((ATOM)pClip->fmt >= MAXINTATOM)
                UserDeleteAtom((ATOM)pClip->fmt);

            UT_FreeCBFormat(pClip++);
        }

        /*
         * Free the clipboard itself.
         */
        UserFreePool((HANDLE)pwinsta->pClipBase);
        pwinsta->pClipBase       = NULL;
        pwinsta->cNumClipFormats = 0;
    }

    /*
     * The "empty" succeeds.  The owner is now the thread that has the
     * clipboard open.  Remember the clipboard has changed; this will
     * cause the viewer to redraw at CloseClipboard time.
     */
    pwinsta->fClipboardChanged = TRUE;
    Lock(&pwinsta->spwndClipOwner, pwinsta->spwndClipOpen);

    /*
     * Change the clipboard serial number so that the client-side
     * clipboard caches of all the processes will get
     * flushed on the next OpenClipboard.
     */
    pwinsta->iClipSerialNumber++;
    pwinsta->iClipSequenceNumber++;
    pwinsta->fInDelayedRendering = FALSE;

    if (!fDying && ptiCurrent)
        ThreadUnlockWinSta(ptiCurrent, &tlpwinsta);

    return TRUE;
}

/***************************************************************************\
* _SetClipboardData
*
* This routine sets data into the clipboard. Does validation against
* DUMMY_TEXT_HANDLE only.
*
* History:
* 18-Nov-1990 ScottLu   Ported from Win3.
\***************************************************************************/

BOOL _SetClipboardData(
    UINT   fmt,
    HANDLE hData,
    BOOL   fGlobalHandle,
    BOOL   fIncSerialNumber)
{
    PWINDOWSTATION pwinsta;
    BOOL fRet;

    if ((pwinsta = CheckClipboardAccess()) == NULL)
        return FALSE;

    /*
     * Check if the Data handle is DUMMY_TEXT_HANDLE; If so, return an
     * error.  DUMMY_TEXT_HANDLE will be used as a valid clipboard handle
     * only by USER.  If any app tries to pass it as a handle, it should
     * get an error!
     */
    if ((hData >= DUMMY_TEXT_HANDLE) && (hData <= DUMMY_MAX_HANDLE)) {
        RIPMSG0(RIP_WARNING, "Clipboard: SetClipboardData called with dummy-handle");
        return FALSE;
    }

    if (fRet = InternalSetClipboardData(pwinsta, fmt, hData, fGlobalHandle, fIncSerialNumber)) {

        /*
         * The set object must remain PUBLIC, so that other processes
         * can view/manipulate the handles when requested.
         */
        switch (fmt) {
        case CF_BITMAP:
            GreSetBitmapOwner(hData, OBJECT_OWNER_PUBLIC);
            break;

        case CF_PALETTE:
            GreSetPaletteOwner(hData, OBJECT_OWNER_PUBLIC);
            break;
        }
    }

    return fRet;
}

/***************************************************************************\
* InternalSetClipboardData
*
* Internal routine to set data into the clipboard.
*
* History:
* 18-Nov-1990 ScottLu   Ported from Win3.
\***************************************************************************/

#define CCHFORMATNAME 256

BOOL InternalSetClipboardData(
    PWINDOWSTATION pwinsta,
    UINT           fmt,
    HANDLE         hData,
    BOOL           fGlobalHandle,
    BOOL           fIncSerialNumber)
{
    PCLIP pClip;
    WCHAR achFormatName[CCHFORMATNAME];

    /*
     * Just check for pwinsta->ptiClipLock being NULL instead of checking
     * against PtiCurrent because an app needs to call SetClipboardData if
     * he's rendering data while another app has the clipboard open.
     */
    if ((pwinsta->ptiClipLock == NULL) || (fmt == 0)) {
        RIPERR0(ERROR_CLIPBOARD_NOT_OPEN, RIP_WARNING, "SetClipboardData: Clipboard not open");
        return FALSE;
    }

    if ((pClip = FindClipFormat(pwinsta, fmt)) != NULL) {

        /*
         * If data already exists, free it before we replace it.
         */
        UT_FreeCBFormat(pClip);

    } else {

        if (pwinsta->pClipBase == NULL) {
            pClip = (PCLIP)UserAllocPool(sizeof(CLIP), TAG_CLIPBOARD);
        } else {
            DWORD dwSize = sizeof(CLIP) * pwinsta->cNumClipFormats;

            pClip = (PCLIP)UserReAllocPool(pwinsta->pClipBase,
                                           dwSize,
                                           dwSize + sizeof(CLIP),
                                           TAG_CLIPBOARD);
        }

        /*
         * Out of memory...  return.
         */
        if (pClip == NULL) {
            RIPMSG0(RIP_WARNING, "SetClipboardData: Out of memory");
            return FALSE;
        }

        /*
         * Just in case the data moved
         */
        pwinsta->pClipBase = pClip;

        /*
         * Increment the reference count of this atom format so that if
         * the application frees this atom we don't get stuck with a
         * bogus atom. We call DeleteAtom in the EmptyClipboard() code,
         * which decrements this count when we're done with this clipboard
         * data.
         */
        if (UserGetAtomName((ATOM)fmt, achFormatName, CCHFORMATNAME) != 0)
            UserAddAtom(achFormatName, FALSE);

        /*
         * Point to the new entry in the clipboard.
         */
        pClip += pwinsta->cNumClipFormats++;
        pClip->fmt = fmt;
    }

    /*
     * Start updating the new entry in the clipboard.
     */
    pClip->hData         = hData;
    pClip->fGlobalHandle = fGlobalHandle;

    if (fIncSerialNumber)
        pwinsta->fClipboardChanged = TRUE;

    if (fIncSerialNumber && !pwinsta->fInDelayedRendering) {
        pwinsta->iClipSequenceNumber++;
    }

    /*
     * If the thread didn't bother emptying the clipboard before
     * writing to it, change the clipboard serial number
     * so that the client-side clipboard caches of all the
     * processes will get flushed on the next OpenClipboard.
     */
    if ((pwinsta->spwndClipOwner == NULL) ||
        (GETPTI(pwinsta->spwndClipOwner) != PtiCurrent())) {

        RIPMSG0(RIP_VERBOSE, "Clipboard: SetClipboardData called without emptying clipboard");

        if (fIncSerialNumber)
            pwinsta->iClipSerialNumber++;
    }

    return TRUE;
}

/***************************************************************************\
* CreateScreenBitmap
*
*
\***************************************************************************/

HBITMAP CreateScreenBitmap(
    int  cx,
    int  cy,
    UINT bpp)
{
    if (bpp == 1)
        return GreCreateBitmap(cx, cy, 1, 1, NULL);

    return GreCreateCompatibleBitmap(gpDispInfo->hdcScreen, cx, cy);
}

/***************************************************************************\
* SizeOfDib
*
* Returns the size of a packed-dib.
*
\***************************************************************************/

DWORD SizeOfDib(
    LPBITMAPINFOHEADER lpDib)
{
    DWORD dwColor;
    DWORD dwBits;

    /*
     * Calculate size of bitmap bits.
     */
    dwBits = WIDTHBYTES(lpDib->biWidth * lpDib->biBitCount) * abs(lpDib->biHeight);

    /*
     * Calculate size of color table.
     */
    if ((lpDib->biCompression == BI_RGB) ||
        (lpDib->biCompression == BI_BITFIELDS)) {

        if (lpDib->biClrUsed) {
            dwColor = lpDib->biClrUsed * sizeof(DWORD);
        } else {
            if (lpDib->biBitCount <= 8) {
                dwColor = (1 << lpDib->biBitCount) * sizeof(RGBQUAD);
            } else if ((lpDib->biBitCount == 16) || (lpDib->biBitCount == 32)) {
                if (lpDib->biCompression == BI_BITFIELDS) {
                    dwColor = (3 * sizeof(DWORD));
                } else {
                    dwColor = 0;
                }
            } else {
                dwColor = 0;
            }
        }

    } else if (lpDib->biCompression == BI_RLE4) {

        dwColor = 16 * sizeof(DWORD);

    } else if (lpDib->biCompression == BI_RLE8) {

        dwColor = 256 * sizeof(DWORD);

    } else {

        dwColor = 0;

    }

    return (lpDib->biSize + dwColor + dwBits);
}

/***************************************************************************\
* DIBtoBMP
*
* Creates a bitmap from a DIB spec.
*
\***************************************************************************/

HBITMAP DIBtoBMP(
    LPBITMAPINFOHEADER lpbih,
    HPALETTE           hpal)
{
    HDC      hdc;
    int      cx;
    int      cy;
    int      bpp;
    LPSTR    lpbits;
    HBITMAP  hbmp;

    #define lpbch ((LPBITMAPCOREHEADER)lpbih)

    /*
     * Gather the dib-info for the convert.
     */
    if (lpbih->biSize == sizeof(BITMAPINFOHEADER)) {

        cx  = (int)lpbih->biWidth;
        cy  = (int)lpbih->biHeight;
        bpp = (int)lpbih->biBitCount;

        lpbits = ((PBYTE)lpbih) + sizeof(BITMAPINFOHEADER);

        if (lpbih->biClrUsed) {
            lpbits += (lpbih->biClrUsed * sizeof(RGBQUAD));
        } else if (bpp <= 8) {
            lpbits += ((1 << bpp) * sizeof(RGBQUAD));
        } else if ((bpp == 16) || (bpp == 32)) {
            lpbits += (3 * sizeof(RGBQUAD));
        }

    } else if (lpbch->bcSize == sizeof(BITMAPCOREHEADER)) {

        cx  = (int)lpbch->bcWidth;
        cy  = (int)lpbch->bcHeight;
        bpp = (int)lpbch->bcBitCount;

        lpbits = ((PBYTE)lpbch) + sizeof(BITMAPCOREHEADER);

        if (lpbch->bcBitCount <= 8)
            lpbits += (1 << bpp);

    } else {
        return NULL;
    }

    hbmp = NULL;

    if (hdc = GreCreateCompatibleDC(gpDispInfo->hdcScreen)) {

        if (hbmp = CreateScreenBitmap(cx, cy, bpp)) {

            HBITMAP  hbmT;
            HPALETTE hpalT = NULL;

            hbmT = GreSelectBitmap(hdc, hbmp);

            if (hpal) {
                hpalT = _SelectPalette(hdc, hpal, FALSE);
                xxxRealizePalette(hdc);
            }

            GreSetDIBits(hdc,
                         hbmp,
                         0,
                         cy,
                         lpbits,
                         (LPBITMAPINFO)lpbih,
                         DIB_RGB_COLORS);

            if (hpalT) {
                _SelectPalette(hdc, hpalT, FALSE);
                xxxRealizePalette(hdc);
            }

            GreSelectBitmap(hdc, hbmT);
        }

        GreDeleteDC(hdc);
    }

    #undef lpbch

    return hbmp;
}

/***************************************************************************\
* BMPtoDIB
*
* Creates a memory block with DIB information from a physical bitmap tagged
* to a specific DC.
*
* A DIB block consists of a BITMAPINFOHEADER + RGB colors + DIB bits.
*
\***************************************************************************/

LPBITMAPINFOHEADER BMPtoDIB(
    HBITMAP  hbmp,
    HPALETTE hpal,
    DWORD*   pcbSize)
{
    BITMAP             bmp;
    BITMAPINFOHEADER   bi;
    LPBITMAPINFOHEADER lpbi;
    DWORD              cbBits;
    DWORD              cbPalette;
    DWORD              cbTotal;
    WORD               cBits;
    HDC                hdc;

    UserAssert(hbmp);

    /*
     * Get physical information
     */
    if (!GreExtGetObjectW(hbmp, sizeof(BITMAP), &bmp)) {
        UserAssert(FALSE);
        return NULL;
    }

    /*
     * Adjust the bit count since we only allow DIBS with 1,4,8,16,24 and
     * 32 bits.
     */
    cBits = ((WORD)bmp.bmPlanes * (WORD)bmp.bmBitsPixel);

    if (cBits <= 1) {

        cBits = 1;

    } else if (cBits <= 4) {

        cBits = 4;

    } else if (cBits <= 8) {

        cBits = 8;

    } else {

        /*
         * We're not going to recognize 16/32bpp formats for
         * apps that are not 4.00 or greater.  Paint-Shop has
         * a bug in it where they only recognize (1,4,8,24).  This
         * really stinks that we need to do this type of thing as
         * not to break them bad-apps.
         */
        if (LOWORD(PtiCurrent()->dwExpWinVer) >= VER40) {

            if (cBits <= 16)
                cBits = 16;
            else if (cBits <= 24)
                cBits = 24;
            else
                cBits = 32;

        } else {
            cBits = 24;
        }
    }

    /*
     * Fill in BITMAPINFOHEADER with DIB data
     */
    RtlZeroMemory(&bi, sizeof(bi));

    bi.biSize        = sizeof(bi);
    bi.biWidth       = bmp.bmWidth;
    bi.biHeight      = bmp.bmHeight;
    bi.biPlanes      = 1;
    bi.biBitCount    = cBits;
    bi.biCompression = BI_RGB;

    /*
     * DWORD align the bits-size since dibs must be so.
     */
    cbBits = (DWORD)WIDTHBYTES((WORD)bi.biWidth * cBits) * (DWORD)bi.biHeight;

    /*
     * How big is the palette color table?
     */
    cbPalette = 0;

    if (cBits <= 8) {

        cbPalette = (1 << cBits) * sizeof(RGBQUAD);

    } else if ((cBits == 16) || (cBits == 32)) {

        cbPalette = (3 * sizeof(DWORD));
        bi.biCompression = BI_BITFIELDS;
    }

    /*
     * How much space do we need for the entire DIB?
     */
    cbTotal = bi.biSize + cbPalette + cbBits;

    lpbi = (LPBITMAPINFOHEADER)UserAllocPool(cbTotal, TAG_CLIPBOARD);
    if (lpbi == NULL) {
        return NULL;
    }

    /*
     * Have the total allocated size returned in pcbSize
     */
    if (pcbSize != NULL) {
        *pcbSize = cbTotal;
    }

    /*
     * Setup DIB header
     */
    memcpy(lpbi, &bi, sizeof(bi));

    if (hdc = GreCreateCompatibleDC(gpDispInfo->hdcScreen)) {
        HPALETTE           hpalT = NULL;
        TL tlPool;

        ThreadLockPool(PtiCurrent(), lpbi, &tlPool);

        if (hpal) {
            hpalT = _SelectPalette(hdc, hpal, FALSE);
            xxxRealizePalette(hdc);
        }

        /*
         * Get old bitmap's DIB bits, using the current DC.
         */
        GreGetDIBitsInternal(hdc,
                             hbmp,
                             0,
                             (WORD)bi.biHeight,
                             (LPSTR)((LPSTR)lpbi + lpbi->biSize + cbPalette),
                             (LPBITMAPINFO)lpbi,
                             DIB_RGB_COLORS,
                             cbBits,
                             lpbi->biSize + cbPalette);


        if (hpalT) {
            _SelectPalette(hdc, hpalT, FALSE);
            xxxRealizePalette(hdc);
        }

        GreDeleteDC(hdc);

        ThreadUnlockPool(PtiCurrent(), &tlPool);
    }

    return lpbi;
}

/***************************************************************************\
* DIBtoDIBV5
*
* History:
* 18-Dec-1997 HideyukN  Created.
\***************************************************************************/

LPBITMAPV5HEADER DIBtoDIBV5(
    LPBITMAPINFOHEADER lpDib,
    DWORD              cbSize)
{
    LPBITMAPV5HEADER lpV5h;
    ULONG            cjBits;
    ULONG            cjColor, cjColorV5;
    BOOL             bBitMasks = FALSE;

    if (cbSize < sizeof(BITMAPINFOHEADER)) {
        RIPMSG2(RIP_WARNING, "DIBtoDIBV5: buffer %d too small for header %d",
                cbSize, sizeof(BITMAPINFOHEADER));
        return NULL;
    }

    /*
     * Support only convert from BITMAPINFOHEADER
     */
    if (lpDib->biSize != sizeof(BITMAPINFOHEADER))
        return NULL;

    /*
     * Calculate size of bitmap bits.
     */
    cjBits = WIDTHBYTES(lpDib->biWidth * lpDib->biBitCount) * abs(lpDib->biHeight);

    /*
     * Calculate size of color table.
     */
    if (lpDib->biCompression == BI_BITFIELDS) {

        if ((lpDib->biBitCount == 16) || (lpDib->biBitCount == 32)) {
            /*
             * Color bit masks for R,G,B.
             */
            cjColor   = (3 * sizeof(DWORD));

            /*
             * Bit fields data is a part of BITMAPV5HEADER
             */
            cjColorV5 = 0;
            bBitMasks = TRUE;
        } else {
            /*
             * Actually, thi shouldn't happen.
             */
            RIPMSG0(RIP_ERROR,"Bad BITMAPINFOHEADER.");
            cjColor = cjColorV5 = 0;
        }

    } else if (lpDib->biCompression == BI_RGB) {

        if (lpDib->biClrUsed) {
            cjColor = cjColorV5 = lpDib->biClrUsed * sizeof(DWORD);
        } else {
            if (lpDib->biBitCount <= 8) {
                cjColor = cjColorV5 = (1 << lpDib->biBitCount) * sizeof(RGBQUAD);
            } else {
                cjColor = cjColorV5 = 0;
            }
        }

    } else if (lpDib->biCompression == BI_RLE4) {

        cjColor = cjColorV5 = 16 * sizeof(DWORD);

    } else if (lpDib->biCompression == BI_RLE8) {

        cjColor = cjColorV5 = 256 * sizeof(DWORD);

    } else {

        cjColor = cjColorV5 = 0;

    }

    if (cbSize < sizeof(BITMAPINFOHEADER) + cjColorV5 + cjBits) {
        RIPMSG5(RIP_WARNING, "DIBtoDIBV5: buffer %d too small for bitmap %d Header"
                             " %d cjColorV5 %d cjBits %d",
                cbSize,
                sizeof(BITMAPINFOHEADER) + cjColorV5 + cjBits,
                sizeof(BITMAPINFOHEADER),
                cjColorV5,
                cjBits);
        return NULL;
    }

    /*
     * Allocate memory for BITMAPV5HEADER
     */
    lpV5h = (LPBITMAPV5HEADER)UserAllocPool(sizeof(BITMAPV5HEADER) + cjColorV5 + cjBits,
                                            TAG_CLIPBOARD);

    if (lpV5h == NULL)
        return NULL;

    /*
     * fill allocated memory with zero.
     */
    RtlZeroMemory((PVOID)lpV5h,sizeof(BITMAPV5HEADER));

    try {

        /*
         * Copy BITMAPINFOHEADER to BITMAPV5HEADER
         */
        RtlCopyMemory((PVOID)lpV5h,(PVOID)lpDib,sizeof(BITMAPINFOHEADER));

    } except (W32ExceptionHandler(FALSE, RIP_ERROR)) {
        UserFreePool(lpV5h);
        return NULL;
    }

    /*
     * Adjust the header size to BITMAPV5HEADER.
     */
    lpV5h->bV5Size = sizeof(BITMAPV5HEADER);

    /*
     * Bitmap is in sRGB color space.
     */
    lpV5h->bV5CSType = LCS_sRGB;

    /*
     * Set rendering intent.
     */
    lpV5h->bV5Intent = LCS_GM_IMAGES;

    if (bBitMasks) {

        /*
         * If there is bitfields mask, copy it to BITMAPV5HEADER.
         */
        lpV5h->bV5RedMask = *(DWORD *)&(((BITMAPINFO *)lpDib)->bmiColors[0]);
        lpV5h->bV5GreenMask = *(DWORD *)&(((BITMAPINFO *)lpDib)->bmiColors[1]);
        lpV5h->bV5BlueMask = *(DWORD *)&(((BITMAPINFO *)lpDib)->bmiColors[2]);

    } else {

        /*
         * Otherwise, copy color table if there is.
         */
        if (cjColorV5) {
            RtlCopyMemory((BYTE *)lpV5h + sizeof(BITMAPV5HEADER),
                          (BYTE *)lpDib + sizeof(BITMAPINFOHEADER),
                          cjColorV5);
        }
    }

    /*
     * Copy bitmap bits
     */
    RtlCopyMemory((BYTE *)lpV5h + sizeof(BITMAPV5HEADER) + cjColorV5,
                  (BYTE *)lpDib + sizeof(BITMAPINFOHEADER) + cjColorV5,
                  cjBits);

    return lpV5h;
}

/***************************************************************************\
* BMPtoDIBV5
*
* History:
* 18-Dec-1997 HideyukN  Created.
\***************************************************************************/

LPBITMAPV5HEADER BMPtoDIBV5(
    HBITMAP  hbmp,
    HPALETTE hpal)
{
    LPBITMAPV5HEADER   lpV5h;
    LPBITMAPINFOHEADER lpbih;
    DWORD              cbSize;

    /*
     * Convert bitmap handle to BITMAPINFOHEADER first.
     */
    lpbih = BMPtoDIB(hbmp, hpal, &cbSize);

    if (lpbih) {

        /*
         * Then, convert BITMAPINFOHEADER to BITMAPV5HEADER.
         */
        lpV5h = DIBtoDIBV5(lpbih, cbSize);

        /*
         * Free memory which contains BITMAPINFOHEADER temporary.
         */
        UserFreePool(lpbih);

        return (lpV5h);
    } else {

        RIPMSG0(RIP_ERROR,"Failed on BMPtoDIB(), Why ??");
        return NULL;
    }
}

/***************************************************************************\
* xxxGetDummyBitmap
*
* Returns a real-bitmap from a dummy-format.
*
* History:
* 24-Oct-1995 ChrisWil  Created.
\***************************************************************************/

HANDLE xxxGetDummyBitmap(
    PWINDOWSTATION pwinsta,
    PGETCLIPBDATA  pgcd)
{
    HANDLE             hData = NULL;
    PCLIPDATA          pData;
    HBITMAP            hBitmap;
    LPBITMAPINFOHEADER lpbih;
    ULONG              cjBitmap;
    HPALETTE           hPal = NULL;

    PCLIP              pClipT;

    /*
     * If palette display, then first attempt to get the palette
     * for this bitmap.
     */
    if (TEST_PUSIF(PUSIF_PALETTEDISPLAY)) {
        hPal = xxxGetClipboardData(pwinsta, CF_PALETTE, pgcd);
    }

    /*
     * The convertion priority is CF_DIBV5 and then CF_DIB,
     * so, check we have CF_DIBV5 first.
     */
    pClipT = FindClipFormat(pwinsta, CF_DIBV5);
    if (pClipT && (pClipT->hData != DUMMY_DIB_HANDLE)) {

        /*
         * Ok, we have *real* CF_DIBV5 data, At this moment, just
         * go back to client side, then create bitmap handle for
         * CF_BITMAP. Since color conversion only can do it on
         * user-mode.
         */
        if (hData = xxxGetClipboardData(pwinsta, CF_DIBV5, pgcd)) {

            /*
             * Return the type of the returned data.
             * Again, convertion will happen in client side.
             */
            pgcd->uFmtRet  = CF_DIBV5;
            pgcd->hPalette = hPal;

            return (hData);
        }
    }

    /*
     * If the bitmap is a dummy, then we have a problem.  We can't
     * retrieve a bitmap if we only have dummys to work with.
     */
    pClipT = FindClipFormat(pwinsta, CF_DIB);
    if (pClipT && (pClipT->hData != DUMMY_DIB_HANDLE)) {
        hData = xxxGetClipboardData(pwinsta, CF_DIB, pgcd);

    }
    if (hData == NULL)
        return NULL;

    /*
     * Since dibs (memory-handles) are stored in a special
     * format (size,base,data), we need to offet the pointer
     * to the right offset (2 uints).
     */
    if (pData = (PCLIPDATA)HMValidateHandleNoRip(hData, TYPE_CLIPDATA)) {
        lpbih = (LPBITMAPINFOHEADER)&pData->abData;
        cjBitmap = pData->cbData;
    } else {
        UserAssert(pData != NULL);
        return NULL;
    }

    /*
     * Convert the dib to a bitmap.
     */
    /*
     * The buffer size for bitmap should be larger than
     * bitmap header + color table + bitmap bits data.
     */
    if ((cjBitmap >= sizeof(BITMAPCOREHEADER)) &&
        (cjBitmap >= (GreGetBitmapSize((CONST BITMAPINFO *)lpbih,DIB_RGB_COLORS) +
                      GreGetBitmapBitsSize((CONST BITMAPINFO *)lpbih)))) {

        if (hBitmap = DIBtoBMP(lpbih, hPal)) {
            /*
             * Once, we create *real* bitmap, overwrite dummy handle.
             */

            pClipT = FindClipFormat(pwinsta, CF_BITMAP);
            if (pClipT) {
                UT_FreeCBFormat(pClipT);
                pClipT->hData = hBitmap;
                GreSetBitmapOwner(hBitmap, OBJECT_OWNER_PUBLIC);

                /*
                 * Let callee know we can obtain CF_BITMAP
                 */
                pgcd->uFmtRet = CF_BITMAP;
            } else {
                /*
                 * Bleh -- now we can't find the BITMAP entry anymore.  Bail.
                 */
                RIPMSG0(RIP_WARNING,
                      "Clipboard: CF_BITMAP format not available");
                GreDeleteObject(hBitmap);
                hBitmap = NULL;
            }
        }
        return (HANDLE)hBitmap;
    } else {
        RIPMSG0(RIP_WARNING, "GetClipboardData, bad DIB format\n");
        return NULL;
    }

}

/***************************************************************************\
* xxxGetDummyDib
*
* Returns a real-dib (in special clipboard-handle format) from a
* dummy format.
*
* History:
* 24-Oct-1995 ChrisWil  Created.
\***************************************************************************/

HANDLE xxxGetDummyDib(
    PWINDOWSTATION pwinsta,
    PGETCLIPBDATA  pgcd)
{
    HANDLE             hData = NULL;
    HBITMAP            hBitmap = NULL;
    LPBITMAPINFOHEADER lpDib;
    HANDLE             hDib;
    HPALETTE           hPal = NULL;

    PCLIP              pClipT;

    /*
     * If palette display, then first attempt to get the palette
     * for this bitmap.  For palette devices, we must have a palette.
     */
    if (TEST_PUSIF(PUSIF_PALETTEDISPLAY)) {
        hPal = xxxGetClipboardData(pwinsta, CF_PALETTE, pgcd);

        if (hPal == NULL)
            return NULL;
    }

    /*
     * The convertion priority is CF_DIBV5 and then CF_BITMAP,
     * so, check we have CF_DIBV5 first.
     */
    pClipT = FindClipFormat(pwinsta, CF_DIBV5);
    if (pClipT && (pClipT->hData != DUMMY_DIB_HANDLE)) {

        /*
         * Ok, we have *real* CF_DIBV5 data, At this moment, just
         * go back to client side, then create bitmap data for
         * CF_DIB. Since color conversion only can do it on
         * user-mode.
         */
        if (hData = xxxGetClipboardData(pwinsta, CF_DIBV5, pgcd)) {

            /*
             * Return the type of the returned data.
             * Again, convertion will happen in client side.
             */
            pgcd->uFmtRet  = CF_DIBV5;
            pgcd->hPalette = hPal;

            return (hData);
        }
    }

    /*
     * Get the real-bitmap.  We must have one in order to convert
     * to the dib.  If there's no bitmap, then we have something
     * wrong.
     */
    pClipT = FindClipFormat(pwinsta, CF_BITMAP);
    if (pClipT && (pClipT->hData != DUMMY_DIB_HANDLE)) {
        hBitmap = xxxGetClipboardData(pwinsta, CF_BITMAP, pgcd);
    }

    if (hBitmap == NULL) {
        return NULL;
    }

    /*
     * Convert the bitmap to a dib-spec.
     */
    hDib = NULL;
    if (lpDib = BMPtoDIB(hBitmap, hPal, NULL)) {

        DWORD cbData = SizeOfDib(lpDib);;

        /*
         * Convert the dib-spec to the special-clipboard
         * memory-handle (size,base,data).  This so
         * the client is able to convert properly when
         * handled a dib.
         */
        hDib = _ConvertMemHandle((LPBYTE)lpDib, cbData);
        UserFreePool(lpDib);

        if (hDib != NULL) {
            /*
             * Once, we create *real* bitmap, overwrite dummy handle.
             */

            pClipT = FindClipFormat(pwinsta, CF_DIB);
            if (pClipT) {
                UT_FreeCBFormat(pClipT);
                pClipT->hData = hDib;

                /*
                 * Let callee know we can obtain CF_DIB
                 */
                pgcd->uFmtRet = CF_DIB;
            } else {
                PVOID pObj;
                /*
                 * Bleh -- now we can't find the DIB entry anymore.  Bail.
                 */
                RIPMSG0(RIP_WARNING,
                      "Clipboard: CF_PDIB format not available");
                pObj = HMValidateHandleNoRip(hDib, TYPE_CLIPDATA);
                if (pObj) {
                    HMFreeObject(pObj);
                }
                hDib = NULL;
            }
        }
    }

    return hDib;
}

/***************************************************************************\
* xxxGetDummyDibV5
*
* Returns a real-dib (in special clipboard-handle format) from a
* dummy format.
*
* History:
* 18-Dec-1997 HideyukN  Created.
\***************************************************************************/

HANDLE xxxGetDummyDibV5(
    PWINDOWSTATION pwinsta,
    PGETCLIPBDATA  pgcd)
{
    HANDLE             hData;
    PCLIPDATA          pData;
    LPBITMAPV5HEADER   lpDibV5 = NULL;
    HANDLE             hDibV5 = NULL;

    PCLIP              pClipT;

    /*
     * The convertion priority is CF_DIB and then CF_BITMAP,
     * so, check we have CF_DIB first.
     */
    pClipT = FindClipFormat(pwinsta, CF_DIB);
    if (pClipT && (pClipT->hData != DUMMY_DIB_HANDLE)) {

        /*
         * Ok, we have *real* CF_DIB data, get it.
         */
        if (hData = xxxGetClipboardData(pwinsta, CF_DIB, pgcd)) {

            /*
             * Since dibs (memory-handles) are stored in a special
             * format (size,base,data), we need to offet the pointer
             * to the right offset (2 uints).
             */
            if (pData = (PCLIPDATA)HMValidateHandleNoRip(hData, TYPE_CLIPDATA)) {

                LPBITMAPINFOHEADER lpDib = (LPBITMAPINFOHEADER)&pData->abData;

                /*
                 * Convert the BITMAPINFOHEADER to BITMAPV5HEADER.
                 */
                lpDibV5 = DIBtoDIBV5(lpDib, pData->cbData);

            } else {
                UserAssert(pData != NULL);
            }
        }
    }

    if (lpDibV5 == NULL) {

        /*
         * Try CF_BITMAP, here
         */
        pClipT = FindClipFormat(pwinsta, CF_BITMAP);
        if ((pClipT) &&
            (pClipT->hData != DUMMY_DIB_HANDLE) &&
            (hData = xxxGetClipboardData(pwinsta, CF_BITMAP, pgcd))) {

            HPALETTE hPal = NULL;

            /*
             * If palette display, then first attempt to get the palette
             * for this bitmap.  For palette devices, we must have a palette.
             */
            if (TEST_PUSIF(PUSIF_PALETTEDISPLAY)) {

                hPal = xxxGetClipboardData(pwinsta, CF_PALETTE, pgcd);

                if (hPal == NULL)
                    return NULL;
            }

            /*
             * hData is GDI bitmap handle, Convert the bitmap to a dib-spec.
             */
            lpDibV5 = BMPtoDIBV5((HBITMAP)hData, hPal);
        }
    }

    if (lpDibV5 != NULL) {

        DWORD cbData = SizeOfDib((LPBITMAPINFOHEADER)lpDibV5);

        /*
         * Convert the dib-spec to the special-clipboard
         * memory-handle (size,base,data).  This so
         * the client is able to convert properly when
         * handled a dib.
         */
        hDibV5 = _ConvertMemHandle((LPBYTE)lpDibV5, cbData);
        UserFreePool(lpDibV5);

        if (hDibV5 != NULL) {

            /*
             * Once, we create *real* bitmap, overwrite dummy handle.
             */

            pClipT = FindClipFormat(pwinsta, CF_DIBV5);
            if (pClipT) {
                UT_FreeCBFormat(pClipT);
                pClipT->hData = hDibV5;

                /*
                 * Let callee know we can obtain CF_DIBV5
                 */
                pgcd->uFmtRet = CF_DIBV5;
            } else {
                PVOID pObj;
                /*
                 * Bleh -- now we can't find the DIB entry anymore.  Bail.
                 */
                RIPMSG0(RIP_WARNING,
                      "Clipboard: CF_DIBV5 format not available");
                pObj = HMValidateHandleNoRip(hDibV5, TYPE_CLIPDATA);
                if (pObj) {
                    HMFreeObject(pObj);
                }
                hDibV5 = NULL;
            }
        }
    }

    return hDibV5;
}

/***************************************************************************\
* CreateDIBPalette
*
* This creates a palette with PC_NOCOLLAPSE entries since we require the
* palette-entries and bitmap-indexes to map exactly.  Otherwise, we could
* end up selecting a palette where a color collapses to an index not
* where the bitmap thinks it is.  This would cause slower drawing since
* the Blt would go through color translation.
*
* History:
* 31-Jan-1992 MikeKe    From win31
\***************************************************************************/

HPALETTE CreateDIBPalette(
   LPBITMAPINFOHEADER pbmih,
   UINT               colors)
{
    HPALETTE hpal;

    if (colors != 0) {

        int         i;
        BOOL        fOldDIB = (pbmih->biSize == sizeof(BITMAPCOREHEADER));
        RGBTRIPLE   *pColorTable;
        PLOGPALETTE plp;

        /*
         * Allocate memory for palette creation.
         */
        plp = (PLOGPALETTE)UserAllocPoolWithQuota(sizeof(LOGPALETTE) +
                                                  (sizeof(PALETTEENTRY) * 256),
                                                  TAG_CLIPBOARDPALETTE);

        if (plp == NULL) {
            return NULL;
        }

        pColorTable = (RGBTRIPLE *)((LPSTR)pbmih + (WORD)pbmih->biSize);
        plp->palVersion = 0x300;

        if (fOldDIB || (pbmih->biClrUsed == 0)) {
            UserAssert(colors <= 0xFFFF);
            plp->palNumEntries = (WORD)colors;
        } else {
            UserAssert(pbmih->biClrUsed <= 0xFFFF);
            plp->palNumEntries = (WORD)pbmih->biClrUsed;
        }

        for (i = 0; i < (int)(plp->palNumEntries); i++) {

            plp->palPalEntry[i].peRed   = pColorTable->rgbtRed;
            plp->palPalEntry[i].peGreen = pColorTable->rgbtGreen;
            plp->palPalEntry[i].peBlue  = pColorTable->rgbtBlue;
            plp->palPalEntry[i].peFlags = (BYTE)PC_NOCOLLAPSE;

            if (fOldDIB) {
                pColorTable++;
            } else {
                pColorTable = (RGBTRIPLE *)((LPSTR)pColorTable+sizeof(RGBQUAD));
            }
        }

        hpal = GreCreatePalette((LPLOGPALETTE)plp);
        UserFreePool(plp);

    } else {
        hpal = GreCreateHalftonePalette(HDCBITS());
    }

    GreSetPaletteOwner(hpal, OBJECT_OWNER_PUBLIC);

    return hpal;
}

/***************************************************************************\
* xxxGetDummyPalette
*
* Returns a real-palette from a dummy-format.  Derives it from a real-dib.
*
* History:
* 24-Oct-1995 ChrisWil  Created.
\***************************************************************************/

HANDLE xxxGetDummyPalette(
    PWINDOWSTATION pwinsta,
    PGETCLIPBDATA  pgcd)
{
    HANDLE             hData;
    PCLIPDATA          pData;
    LPBITMAPINFOHEADER lpbih;
    HPALETTE           hPal;

    PCLIP              pClipT;

    /*
     * Since CF_DIBV5 has higher priority than CF_DIB, so look into
     * CF_DIBV5 first, to find DIB palette.
     */
    UINT               uFmt = CF_DIBV5;

    if ((pClipT = FindClipFormat(pwinsta, uFmt)) != NULL)
    {
        if (pClipT->hData != DUMMY_DIB_HANDLE) {

            /*
             * Ok, we have real CF_DIBV5, let extract palette from DIBV5.
             */
        } else {

            /*
             * Otherwise, try CF_DIB.
             */
            uFmt = CF_DIB;
        }
    }

    /*
     * Get the DIB by which we derive the palette.  If the DIB comes
     * back as a dummy, then there's something wrong.  Me must have
     * a real dib at this point.
     */
    hData = (HANDLE)xxxGetClipboardData(pwinsta, uFmt, pgcd);
    UserAssert(hData > DUMMY_MAX_HANDLE);

    if (hData == NULL)
        return NULL;

    /*
     * Since dibs (memory-handles) are stored in a special
     * format (size,base,data), we need to offet the pointer
     * to the right offset (2 uints).
     */
    if (pData = (PCLIPDATA)HMValidateHandle(hData, TYPE_CLIPDATA)) {
        lpbih = (LPBITMAPINFOHEADER)&pData->abData;
    } else {
        UserAssert(pData != NULL);
        return NULL;
    }

    if ((pClipT = FindClipFormat(pwinsta, CF_PALETTE)) == NULL) {
        RIPMSG0(RIP_WARNING,
              "Clipboard: CF_PALETTE format not available");
        return NULL;
    }

    /*
     * Note -- if CreateDIBPalette ever changes to leave the crit sect,
     * we will need to move the above FindClipFormat to after the create
     * call and deal with gracefully freeing hPal on failure.  pClipT
     * can change during callbacks.
     */

    hPal = CreateDIBPalette(lpbih, lpbih->biClrUsed);

    if (hPal != NULL) {
        UT_FreeCBFormat(pClipT);
        pClipT->hData = hPal;
        GreSetPaletteOwner(hPal, OBJECT_OWNER_PUBLIC);
    }

    return (HANDLE)hPal;
}

/***************************************************************************\
* xxxGetDummyText
*
* Returns a handle to text from a dummy-format.
*
* History:
* 24-Oct-1995 ChrisWil  Created.
\***************************************************************************/

HANDLE xxxGetDummyText(
    PWINDOWSTATION pwinsta,
    UINT           fmt,
    PGETCLIPBDATA  pgcd)
{
    HANDLE hText;
    PCLIP  pClipT;
    UINT   uFmtMain;
    UINT   uFmtAlt;
    BOOL  bMain = TRUE;

    /*
     * Get the handle of the other text format available.
     */
    switch (fmt) {
    case CF_TEXT:
        uFmtMain = CF_UNICODETEXT;
        uFmtAlt  = CF_OEMTEXT;
        goto GetRealText;

    case CF_OEMTEXT:
        uFmtMain = CF_UNICODETEXT;
        uFmtAlt  = CF_TEXT;
        goto GetRealText;

    case CF_UNICODETEXT:
        uFmtMain = CF_TEXT;
        uFmtAlt  = CF_OEMTEXT;

GetRealText:

        if ((pClipT = FindClipFormat(pwinsta, uFmtMain)) == NULL)
            return NULL;

        if (pClipT->hData != DUMMY_TEXT_HANDLE) {

            if (xxxGetClipboardData(pwinsta, uFmtMain, pgcd))
                break;

            return NULL;
        }

        if ((pClipT = FindClipFormat(pwinsta, uFmtAlt)) == NULL)
            return NULL;

        if (pClipT->hData != DUMMY_TEXT_HANDLE) {
            bMain = FALSE;

            if (xxxGetClipboardData(pwinsta, uFmtAlt, pgcd))
                break;
        }

        /*
         * Fall through to return a dummy handle.
         */

    default:
        return NULL;
    }

    /*
     * Since xxxGetClipboardData leaves the critsect, we need to
     * reacquire pClipT.
     */

    pClipT = FindClipFormat(pwinsta, bMain? uFmtMain:uFmtAlt);

    if (pClipT == NULL) {
        RIPMSG1(RIP_WARNING,
              "Clipboard: GetDummyText, format 0x%lX not available", bMain? uFmtMain:uFmtAlt);
        return NULL;
    }

    /*
     * Return the type of the returned data.
     */
    pgcd->uFmtRet = pClipT->fmt;
    hText         = pClipT->hData;

    /*
     * Set the locale, since the text will need to be
     * converted to another format.
     */
    if(pClipT = FindClipFormat(pwinsta, CF_LOCALE)) {
        pgcd->hLocale = pClipT->hData;
    } else {
        pgcd->hLocale = NULL;
    }

    return hText;
}

/***************************************************************************\
* xxxGetRenderData
*
* Returns a handle to delayed rendered data.  This requires a call to the
* client to supply the data.  This causes us to regenerate our pointer
* to pClip.
*
* History:
* 24-Oct-1995 ChrisWil  Created.
\***************************************************************************/

HANDLE xxxGetRenderData(
    PWINDOWSTATION pwinsta,
    UINT           fmt)
{
    BOOL   fClipboardChangedOld;
    TL     tlpwndClipOwner;
    PCLIP          pClip;

    /*
     * If the handle is NULL, the data is delay rendered.  This means
     * we send a message to the current clipboard owner and have
     * it render the data for us.
     */
    if (pwinsta->spwndClipOwner != NULL) {

        /*
         * Preserve the pwinsta->fClipboardChanged flag before SendMessage
         * and restore the flag later; Thus we ignore the changes
         * done to the pwinsta->fClipboardChanged flag by apps while
         * rendering data in the delayed rendering scheme; This
         * avoids clipboard viewers from painting twice.
         */
        fClipboardChangedOld = pwinsta->fClipboardChanged;
        pwinsta->fInDelayedRendering = TRUE;
        ThreadLockAlways(pwinsta->spwndClipOwner, &tlpwndClipOwner);
        xxxSendMessage(pwinsta->spwndClipOwner, WM_RENDERFORMAT, fmt, 0L);
        ThreadUnlock(&tlpwndClipOwner);
        pwinsta->fClipboardChanged = fClipboardChangedOld;
        pwinsta->fInDelayedRendering = FALSE;

    }

    if ((pClip = FindClipFormat(pwinsta, fmt)) == NULL) {
        RIPMSG1(RIP_WARNING,
              "Clipboard: Meta Render/Clone format 0x%lX not available", fmt);
        return NULL;
    }
    /*
     * We should have the handle now since it has been rendered.
     */
    return pClip->hData;
}

/***************************************************************************\
* xxxGetClipboardData (API)
*
* Grabs a particular data object out of the clipboard.
*
* History:
* 18-Nov-1990 ScottLu   Ported from Win3.
* 20-Aug-1991 EichiM    UNICODE enabling
\***************************************************************************/

HANDLE xxxGetClipboardData(
    PWINDOWSTATION pwinsta,
    UINT           fmt,
    PGETCLIPBDATA  pgcd)
{
    PCLIP  pClip;
    HANDLE hData;

    /*
     * Check the clipboard owner.
     */
    if (pwinsta->ptiClipLock != PtiCurrent()) {
        RIPERR0(ERROR_CLIPBOARD_NOT_OPEN, RIP_VERBOSE, "GetClipboardData: clipboard not open");
        return NULL;
    }

    /*
     * Make sure the format is available.
     */
    if ((pClip = FindClipFormat(pwinsta, fmt)) == NULL) {
        RIPMSG1(RIP_VERBOSE, "Clipboard: Requested format 0x%lX not available", fmt);
        return NULL;
    }

    /*
     * If this is a DUMMY_META*_HANDLE it means that the other
     * metafile format was set in as a delay render format and we should
     * as for that format to get the metafile because the app has not told
     * us they now about this format.
     */
    if (IsMetaDummyHandle(pClip->hData)) {

        if (fmt == CF_ENHMETAFILE) {
            fmt = CF_METAFILEPICT;
        } else if (fmt == CF_METAFILEPICT) {
            fmt = CF_ENHMETAFILE;
        } else {
            RIPMSG0(RIP_WARNING,
                  "Clipboard: Meta Render/Clone expects a metafile type");
        }

        if ((pClip = FindClipFormat(pwinsta, fmt)) == NULL) {
            RIPMSG1(RIP_WARNING,
                  "Clipboard: Meta Render/Clone format 0x%lX not available", fmt);
            return NULL;
        }
    }

    /*
     * This is the data we're returning, unless it's a dummy or
     * render handle.
     */
    hData = pClip->hData;

    /*
     * We are dealing with non-handles.  Retrieve the real data
     * through these inline-routines.  NOTE: these make recursive
     * calls to xxxGetClipboardData().  So care must be taken to
     * assure the pClip is pointing to what we think it's pointing
     * to.
     */
    if ((hData == NULL) || (hData == DUMMY_METARENDER_HANDLE)) {

        hData = xxxGetRenderData(pwinsta, fmt);

    } else if (hData == DUMMY_DIB_HANDLE) {

        switch (fmt) {
        case CF_DIB:
            hData = xxxGetDummyDib(pwinsta, pgcd);
            break;
        case CF_DIBV5:
            hData = xxxGetDummyDibV5(pwinsta, pgcd);
            break;
        case CF_BITMAP:
            hData = xxxGetDummyBitmap(pwinsta, pgcd);
            break;
        case CF_PALETTE:
            hData = xxxGetDummyPalette(pwinsta, pgcd);
            break;
        }

    } else if (hData == DUMMY_TEXT_HANDLE) {

        hData = xxxGetDummyText(pwinsta, fmt, pgcd);
    } else {
        /*
         * This path took no callbacks, so we know pClip is OK.
         */
        if (pgcd)
            pgcd->fGlobalHandle = pClip->fGlobalHandle;

        return hData;
    }

    /*
     * The callbacks for dummy handle resolution have possibly
     * invalidated pClip -- recreate it.
     */

    if ((pClip = FindClipFormat(pwinsta, fmt)) == NULL) {
        RIPMSG1(RIP_VERBOSE, "Clipboard: Requested format 0x%lX not available", fmt);
        return NULL;
    }

    /*
     * Return if this is a global-handle.
     */
    if (pgcd)
        pgcd->fGlobalHandle = pClip->fGlobalHandle;

    return hData;
}

/***************************************************************************\
* FindClipFormat
*
* Finds a particular clipboard format in the clipboard, returns a pointer
* to it, or NULL. If a pointer is found, on return the clipboard is locked
* and pwinsta->pClipBase has been updated to point to the beginning of the
* clipboard.
*
* History:
* 18-Nov-1990 ScottLu   Ported from Win3.
\***************************************************************************/

PCLIP FindClipFormat(
    PWINDOWSTATION pwinsta,
    UINT           format)
{
    PCLIP pClip;
    int   iFmt;

    if ((format != 0) && ((pClip = pwinsta->pClipBase) != NULL)) {

        for (iFmt = pwinsta->cNumClipFormats; iFmt-- != 0;) {

            if (pClip->fmt == format)
                return pClip;

            pClip++;
        }
    }

    return NULL;
}

/***************************************************************************\
* _GetPriorityClipboardFormat (API)
*
* This api allows an application to look for any one of a range of
* clipboard formats in a predefined search order.
*
* History:
* 18-Nov-1990 ScottLu   Ported from Win3.
* 11-Feb-1991 JimA      Added access checks.
\***************************************************************************/

int _GetPriorityClipboardFormat(
    PUINT lpPriorityList,
    int   cfmts)
{
    PWINDOWSTATION pwinsta;
    PCLIP          pClip;
    int            iFmt;
    UINT           fmt;

    /*
     * Blow it off is the caller does not have the proper access rights
     */
    if ((pwinsta = CheckClipboardAccess()) == NULL)
        return 0;

    /*
     * If there is no clipboard or no objects in the clipboard, return 0.
     */
    if ((pwinsta->cNumClipFormats == 0) || (pwinsta->pClipBase == NULL))
        return 0;

    /*
     * Look through the list for any of the formats in lpPriorityList.
     */
    while (cfmts-- > 0) {

        fmt = *lpPriorityList;

        if (fmt != 0) {

            pClip = pwinsta->pClipBase;

            for (iFmt = pwinsta->cNumClipFormats; iFmt-- != 0; pClip++) {

                if (pClip->fmt == fmt)
                    return fmt;
            }
        }

        lpPriorityList++;
    }

    /*
     * There is no matching format.  Return -1.
     */
    return -1;
}

/***************************************************************************\
* xxxSetClipboardViewer (API)
*
* Sets the clipboard viewer window.
*
* History:
* 18-Nov-1990 ScottLu   Ported from Win3.
* 11-Feb-1991 JimA      Added access checks.
\***************************************************************************/

PWND xxxSetClipboardViewer(
    PWND pwndClipViewerNew)
{
    TL             tlpwinsta;
    PWINDOWSTATION pwinsta;
    HWND           hwndClipViewerOld;
    PTHREADINFO    ptiCurrent;

    CheckLock(pwndClipViewerNew);

    /*
     * Blow it off is the caller does not have the proper access rights.
     * The NULL return really doesn't indicate an error but the
     * supposed viewer will never receive any clipboard messages, so
     * it shouldn't cause any problems.
     */
    if ((pwinsta = CheckClipboardAccess()) == NULL)
        return NULL;

    ptiCurrent = PtiCurrent();

    ThreadLockWinSta(ptiCurrent, pwinsta, &tlpwinsta);

    hwndClipViewerOld = HW(pwinsta->spwndClipViewer);
    Lock(&pwinsta->spwndClipViewer, pwndClipViewerNew);

    xxxDrawClipboard(pwinsta);

    ThreadUnlockWinSta(ptiCurrent, &tlpwinsta);

    if (hwndClipViewerOld != NULL)
        return RevalidateHwnd(hwndClipViewerOld);

    return NULL;
}

/***************************************************************************\
* xxxChangeClipboardChain (API)
*
* Changes the clipboard viewer chain.
*
* History:
* 18-Nov-1990 ScottLu   Ported from Win3.
* 11-Feb-1991 JimA      Added access checks.
\***************************************************************************/

BOOL xxxChangeClipboardChain(
    PWND pwndRemove,
    PWND pwndNewNext)
{
    TL             tlpwinsta;
    PWINDOWSTATION pwinsta;
    BOOL           result;
    TL             tlpwndClipViewer;
    PTHREADINFO    ptiCurrent;

    CheckLock(pwndRemove);
    CheckLock(pwndNewNext);

    /*
     * Blow it off is the caller does not have the proper access rights.
     */
    if ((pwinsta = CheckClipboardAccess()) == NULL)
        return FALSE;

    /*
     * pwndRemove should be this thread's window, pwndNewNext will
     * either be NULL or another thread's window.
     */
    ptiCurrent = PtiCurrent();

    if (GETPTI(pwndRemove) != ptiCurrent) {
        RIPMSG0(RIP_WARNING, "Clipboard: ChangeClipboardChain will not remove cross threads");
        return FALSE;
    }

    if (pwinsta->spwndClipViewer == NULL) {
        RIPMSG0(RIP_WARNING, "Clipboard: ChangeClipboardChain has no viewer window");
        return FALSE;
    }

    ThreadLockWinSta(ptiCurrent, pwinsta, &tlpwinsta);

    if (pwndRemove == pwinsta->spwndClipViewer) {

        Lock(&pwinsta->spwndClipViewer, pwndNewNext);
        result = TRUE;

    } else {

        ThreadLockAlways(pwinsta->spwndClipViewer, &tlpwndClipViewer);
        result = (BOOL)xxxSendMessage(pwinsta->spwndClipViewer,
                                      WM_CHANGECBCHAIN,
                                      (WPARAM)HW(pwndRemove),
                                      (LPARAM)HW(pwndNewNext));
        ThreadUnlock(&tlpwndClipViewer);
    }

    ThreadUnlockWinSta(ptiCurrent, &tlpwinsta);

    return result;
}

/***************************************************************************\
* DisownClipboard
*
* Disowns the clipboard so someone else can grab it.
*
* pwndClipOwner is the pwnd that is the reason for disowning the clipboard
* when that window is deleted
*
* History:
* 18-Jun-1991 DarrinM   Ported from Win3.
\***************************************************************************/

VOID DisownClipboard(PWND pwndClipOwner)
{
    TL             tlpwinsta;
    PWINDOWSTATION pwinsta;
    int            iFmt;
    int            cFmts;
    PCLIP          pClip;
    PCLIP          pClipOut;
    BOOL           fKeepDummyHandle;
    PTHREADINFO    ptiCurrent;

    if ((pwinsta = CheckClipboardAccess()) == NULL)
        return;

    ptiCurrent = PtiCurrent();

    ThreadLockWinSta(ptiCurrent, pwinsta, &tlpwinsta);

    xxxSendClipboardMessage(pwinsta, WM_RENDERALLFORMATS);

    pClipOut = pClip = pwinsta->pClipBase;
    fKeepDummyHandle = FALSE;

    for (cFmts = 0, iFmt = pwinsta->cNumClipFormats; iFmt-- != 0;) {

        /*
         * We have to remove the Dummy handles also if the corresponding
         * valid handles are NULL; We should not remove the dummy handles if
         * the corresponding valid handles are not NULL;
         * The following code assumes that only one dummy handle is possible
         * and that can appear only after the corresponding valid handle in
         * the pClip linked list;
         * Fix for Bug #???? --SANKAR-- 10-19-89 --OPUS BUG #3252--
         */
        if (pClip->hData != NULL) {

            if ((pClip->hData != DUMMY_TEXT_HANDLE) ||
                ((pClip->hData == DUMMY_TEXT_HANDLE) && fKeepDummyHandle)) {

                cFmts++;
                *pClipOut++ = *pClip;

                if (IsTextHandle(pClip->fmt, pClip->hData)) {
                    fKeepDummyHandle  = TRUE;
                }
            }
        }

        pClip++;
    }

    /*
     * Unlock the clipboard owner if the owner is still the window we were cleaning
     * up for.
     */
    if (pwndClipOwner == pwinsta->spwndClipOwner) {
        Unlock(&pwinsta->spwndClipOwner);
    } else {
        RIPMSG2(RIP_WARNING, "DisownClipboard: pwndClipOwner changed from %#p to %#p",
                pwndClipOwner, pwinsta->spwndClipOwner);
    }

    /*
     * If number of formats changed, redraw.
     */
    if (cFmts != pwinsta->cNumClipFormats) {
        pwinsta->fClipboardChanged = TRUE;
        pwinsta->iClipSequenceNumber++;
    }

    pwinsta->cNumClipFormats = cFmts;

    /*
     * If anything changed, redraw.  And make sure the data type munging is done.  Or else we will lose
     * them when xxxDrawClipboard clears the fClipboardChanged flag
     */
    if (pwinsta->fClipboardChanged) {
        xxxDrawClipboard(pwinsta);
        MungeClipData(pwinsta);
    }

    ThreadUnlockWinSta(ptiCurrent, &tlpwinsta);
}

/***************************************************************************\
* ForceEmptyClipboard
*
* We're logging off. Force the clipboard contents to go away.
*
* 23-Jul-1992 ScottLu   Created.
\***************************************************************************/

VOID ForceEmptyClipboard(
    PWINDOWSTATION pwinsta)
{

    pwinsta->ptiClipLock =  ((PTHREADINFO)(W32GetCurrentThread())); /*
                                                                     * This will be NULL
                                                                     *   for a non-GUI thread.
                                                                     */
    Unlock(&pwinsta->spwndClipOwner);
    Unlock(&pwinsta->spwndClipViewer);
    Unlock(&pwinsta->spwndClipOpen);

    xxxEmptyClipboard(pwinsta);

    /*
     * If the windowstation is dying, don't bother closing
     * the clipboard.
     */
    if (!(pwinsta->dwWSF_Flags & WSF_DYING))
        xxxCloseClipboard(pwinsta);
}
