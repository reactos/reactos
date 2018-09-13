/****************************** Module Header ******************************\
* Module Name: loadbits.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Loads and creates icons / cursors / bitmaps. All 3 functions can either
* load from a client resource file, load from user's resource file, or
* load from the display's resource file. Beware that hmodules are not
* unique across processes!
*
* 05-Apr-1991 ScottLu   Rewrote to work with client/server
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop
#include <wchar.h>

/***************************************************************************\
* _CreateEmptyCursorObject
*
* Creates a cursor object and links it into a cursor list.
*
* 08-Feb-92 ScottLu     Created.
\***************************************************************************/

HCURSOR _CreateEmptyCursorObject(
    BOOL fPublic)
{
    PCURSOR pcurT;

    /*
     * Create the cursor object.
     */
    pcurT = (PCURSOR)HMAllocObject(PtiCurrent(),
                                   NULL,
                                   TYPE_CURSOR,
                                   max(sizeof(CURSOR),
                                   sizeof(ACON)));

    if (fPublic && (pcurT != NULL)) {
        pcurT->head.ppi = NULL;
        UserAssert(PtiCurrent()->TIF_flags & (TIF_CSRSSTHREAD | TIF_SYSTEMTHREAD));
    }

    return (HCURSOR)PtoH(pcurT);
}

/***************************************************************************\
* DestroyEmptyCursorObject
* UnlinkCursor
*
* Destroys an empty cursor object (structure holds nothing that needs
* destroying).
*
* 08-Feb-1992 ScottLu   Created.
\***************************************************************************/
VOID UnlinkCursor(
    PCURSOR pcur)
{
    PCURSOR *ppcurT;
    BOOL    fTriedPublicCache;
    BOOL    fTriedThisProcessCache = FALSE;

    /*
     * First unlink this cursor object from the cursor list (it will be the
     * first one in the list, so this'll be fast...  but just in case, make
     * it a loop).
     */
    if (fTriedPublicCache = (pcur->head.ppi == NULL)) {
        ppcurT = &gpcurFirst;
    } else {
        ppcurT = &pcur->head.ppi->pCursorCache;
    }

LookAgain:

    for (; *ppcurT != NULL; ppcurT = &((*ppcurT)->pcurNext)) {
        if (*ppcurT == pcur) {
            *ppcurT = pcur->pcurNext;
FreeIt:
            pcur->pcurNext = NULL;
            pcur->CURSORF_flags &= ~CURSORF_LINKED;
            return;
        }
    }

    /*
     * If we get here, it means that the cursor used to be public but
     * got assigned to the current thread due to being unlocked.  We
     * have to look for it in the public cache.
     */
    if (!fTriedPublicCache) {
        ppcurT = &gpcurFirst;
        fTriedPublicCache = TRUE;
        goto LookAgain;
    }

    /*
     * If we got here, it means that it was locked during process
     * cleanup and got assigned to no owner.  Try the current process
     * cache.
     */
    if (!fTriedThisProcessCache) {
        ppcurT = &PpiCurrent()->pCursorCache;
        fTriedThisProcessCache = TRUE;
        goto LookAgain;
    }

    /*
     * Getting Desperate here...  Look through every cursor and process
     * cache for it.
     */
    {
        PHE pheMax, pheT;

        pheMax = &gSharedInfo.aheList[giheLast];
        for (pheT = gSharedInfo.aheList; pheT <= pheMax; pheT++) {
            if (pheT->bType == TYPE_CURSOR) {
                if (((PCURSOR)pheT->phead)->pcurNext == pcur) {
                    ((PCURSOR)pheT->phead)->pcurNext = pcur->pcurNext;
                    goto FreeIt;
                } else if (pheT->pOwner && ((PPROCESSINFO)pheT->pOwner)->pCursorCache == pcur) {
                    ((PPROCESSINFO)pheT->pOwner)->pCursorCache = pcur->pcurNext;
                    goto FreeIt;
                }
            }
        }
    }

    UserAssert(FALSE);
}

/***************************************************************************\
* DestroyEmptyCursorObject
*
\***************************************************************************/

VOID DestroyEmptyCursorObject(
    PCURSOR pcur)
{
    if (pcur->CURSORF_flags & CURSORF_LINKED) {
        UnlinkCursor(pcur);
    }

    HMFreeObject(pcur);
}

/***************************************************************************\
* ZombieCursor
*
* Unlink the cursor and set its owner to the system process.
*
* 3-Sep-1997    vadimg      created
\***************************************************************************/

VOID ZombieCursor(PCURSOR pcur)
{
    if (pcur->CURSORF_flags & CURSORF_LINKED) {
        UnlinkCursor(pcur);
    }

#if DBG
    if (ISTS()) {
        PHE phe;
        phe = HMPheFromObject(pcur);

        if (phe->pOwner == NULL) {
            RIPMSG2(RIP_ERROR, "NULL owner for cursor %x phe %x\n",
                    pcur, phe);
        }
    }
#endif // DBG

    HMChangeOwnerProcess(pcur, gptiRit);

    RIPMSG1(RIP_WARNING, "ZombieCursor: 0x%08X became a zombie", pcur);
}

/***************************************************************************\
* ResStrCmp
*
* This function compares two strings taking into account that one or both
* of them may be resource IDs.  The function returns a TRUE if the strings
* are equal, instead of the zero lstrcmp() returns.
*
* History:
* 20-Apr-91 DavidPe     Created
\***************************************************************************/

BOOL ResStrCmp(
    PUNICODE_STRING cczpstr1,
    PUNICODE_STRING pstr2)
{
    BOOL retval = FALSE;
    /*
     * pstr1 is a STRING that is in kernel space, but the buffer may
     * be in client space.
     */

    if (cczpstr1->Length == 0) {

        /*
         * pstr1 is a resource ID, so just compare the values.
         */
        if (cczpstr1->Buffer == pstr2->Buffer)
            return TRUE;

    } else {

        try {
        /*
         * pstr1 is a string.  if pstr2 is an actual string compare the
         * string values; if pstr2 is not a string then pstr1 may be an
         * "integer string" of the form "#123456". so convert it to an
         * integer and compare the integers.
         * Before calling lstrcmp(), make sure pstr2 is an actual
         * string, not a resource ID.
         */
            if (pstr2->Length != 0) {

                if (RtlEqualUnicodeString(cczpstr1, pstr2, TRUE))
                    retval = TRUE;

            } else if (cczpstr1->Buffer[0] == '#') {

                UNICODE_STRING strId;
                int            id;

                strId.Length        = cczpstr1->Length - sizeof(WCHAR);
                strId.MaximumLength = strId.Length;
                strId.Buffer        = cczpstr1->Buffer + 1;
                RtlUnicodeStringToInteger(&strId, 10, (PULONG)&id);

                if (id == (LONG_PTR)pstr2->Buffer)
                    retval = TRUE;
            }
        } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
        }
    }

    return retval;
}

/***********************************************************************\
* SearchIconCache
*
* Worker routine for FindExistingCursorIcon().
*
* Returns: pCurFound
*
* 28-Sep-1995 SanfordS  Created.
\***********************************************************************/

PCURSOR SearchIconCache(
    PCURSOR         pCursorCache,
    ATOM            atomModName,
    PUNICODE_STRING cczpstrResName,
    PCURSOR         pcurSrc,
    PCURSORFIND     pcfSearch)
{
    /*
     * Run through the list of 'resource' objects created,
     * and see if the cursor requested has already been loaded.
     * If so just return it.  We do this to be consistent with
     * Win 3.0 where they simply return a pointer to the res-data
     * for a cursor/icon handle.  Many apps count on this and
     * call LoadCursor/Icon() often.
     *
     * LR_SHARED implies:
     *   1) icons never get deleted till process (LATER or WOW module)
     *      goes away.
     *   2) This cache is consulted before trying to load a res.
     */
    for (; pCursorCache != NULL; pCursorCache = pCursorCache->pcurNext) {

        /*
         * If we are given a specific cursor to look for, then
         * search for that first.
         */
        if (pcurSrc && (pCursorCache == pcurSrc))
            return pcurSrc;

        /*
         * No need to look further if the module name doesn't match.
         */
        if (atomModName != pCursorCache->atomModName)
            continue;

        /*
         * We only return images that cannot be destroyed by the app.
         * so we don't have to deal with ref counts.  This is owned
         * by us, but not LR_SHARED.
         */
        if (!(pCursorCache->CURSORF_flags & CURSORF_LRSHARED))
            continue;

        /*
         * Check the other distinguishing search criteria for
         * a match.
         */
        if ((pCursorCache->rt == LOWORD(pcfSearch->rt)) &&
            ResStrCmp(cczpstrResName, &pCursorCache->strName)) {

            /*
             * Acons don't have a size per se because each frame
             * can be a different size.  We always make it a hit
             * on acons so replacement of system icons is possible.
             */
            if (pCursorCache->CURSORF_flags & CURSORF_ACON)
                return pCursorCache;

            /*
             * First hit wins.  Nothing fancy here.  Apps that use
             * LR_SHARED have to watch out for this.
             */
            if ((!pcfSearch->cx || (pCursorCache->cx == pcfSearch->cx))       &&
                (!pcfSearch->cy || ((pCursorCache->cy / 2) == pcfSearch->cy)) &&
                (!pcfSearch->bpp || (pCursorCache->bpp == pcfSearch->bpp))) {

                return pCursorCache;
            }
        }
    }

    return NULL;
}

/***********************************************************************\
* _FindExistingCursorIcon
*
* This routine searches all existing icons for one matching the properties
* given.  This routine will only return cursors/icons that are of
* the type that cannot be destroyed by the app. (CURSORF_LRSHARED or
* unowned) and will take the first hit it finds.
*
* 32bit apps that call LoadImage() will normally not have this cacheing
* feature unless they specify LR_SHARED.  If they do so, it is the apps
* responsability to be careful with how they use the cache since wild
* lookups (ie 0s in cx, cy or bpp) will result in different results
* depending on the history of icon/cursor creation.  It is thus recommended
* that apps only use the LR_SHARED option when they are only working
* with one size/colordepth of icon or when they call LoadImage() with
* specific size and/or color content requested.
*
* For the future it would be nice to have a cacheing scheeme that would
* simply be used to speed up reloading of images.  To do this right,
* you would need ref counts to allow deletes to work properly and would
* have to remember whether the images in the cache had been stretched
* or color munged so you don't allow restretching.
*
* Returns: pcurFound
*
*
* 17-Sep-1995 SanfordS  Created.
\***********************************************************************/

PCURSOR _FindExistingCursorIcon(
    ATOM            atomModName,
    PUNICODE_STRING cczpstrResName,
    PCURSOR         pcurSrc,
    PCURSORFIND     pcfSearch)
{
    PCURSOR pcurT = NULL;

    /*
     * If rt is zero we're doing an indirect create, so matching with
     * a previously loaded cursor/icon would be inappropriate.
     */
    if (pcfSearch->rt && atomModName) {

        pcurT = SearchIconCache(PpiCurrent()->pCursorCache,
                                atomModName,
                                cczpstrResName,
                                pcurSrc,
                                pcfSearch);
        if (pcurT == NULL) {
            pcurT = SearchIconCache(gpcurFirst,
                                    atomModName,
                                    cczpstrResName,
                                    pcurSrc,
                                    pcfSearch);
        }
    }

    return pcurT;
}

/***************************************************************************\
* _InternalGetIconInfo
*
* History:
* 09-Mar-1993 MikeKe    Created.
\***************************************************************************/

BOOL _InternalGetIconInfo(
    IN  PCURSOR                  pcur,
    OUT PICONINFO                ccxpiconinfo,
    OUT OPTIONAL PUNICODE_STRING pstrInstanceName,
    OUT OPTIONAL PUNICODE_STRING pstrResName,
    OUT OPTIONAL LPDWORD         ccxpbpp,
    IN  BOOL                     fInternalCursor)
{
    HBITMAP hbmBitsT;
    HBITMAP hbmDstT;
    HBITMAP hbmMask;
    HBITMAP hbmColor;

    /*
     * Note -- while the STRING structures are in kernel mode memory, the
     * buffers are in user-mode memory.  So all use of the buffers should
     * be protected bytry blocks.
     */

    /*
     * If this is an animated cursor, just grab the first frame and return
     * the info for it.
     */
    if (pcur->CURSORF_flags & CURSORF_ACON)
        pcur = ((PACON)pcur)->aspcur[0];

    /*
     * Make copies of the bitmaps
     *
     * If the color bitmap is around, then there is no XOR mask in the
     * hbmMask bitmap.
     */
    hbmMask = GreCreateBitmap(
            pcur->cx,
            (pcur->hbmColor && !fInternalCursor) ? pcur->cy / 2 : pcur->cy,
            1,
            1,
            NULL);

    if (hbmMask == NULL)
        return FALSE;


    hbmColor = NULL;

    if (pcur->hbmColor != NULL) {

        hbmColor = GreCreateCompatibleBitmap(HDCBITS(),
                                             pcur->cx,
                                             pcur->cy / 2);

        if (hbmColor == NULL) {
            GreDeleteObject(hbmMask);
            return FALSE;
        }
    }

    hbmBitsT = GreSelectBitmap(ghdcMem2, pcur->hbmMask);
    hbmDstT  = GreSelectBitmap(ghdcMem, hbmMask);

    GreBitBlt(ghdcMem,
              0,
              0,
              pcur->cx,
              (pcur->hbmColor && !fInternalCursor) ? pcur->cy / 2 : pcur->cy,
              ghdcMem2,
              0,
              0,
              SRCCOPY,
              0x00ffffff);

    if (hbmColor != NULL) {

        GreSelectBitmap(ghdcMem2, pcur->hbmColor);
        GreSelectBitmap(ghdcMem, hbmColor);

        GreBitBlt(ghdcMem,
                  0,
                  0,
                  pcur->cx,
                  pcur->cy / 2,
                  ghdcMem2,
                  0,
                  0,
                  SRCCOPY,
                  0);
    }

    GreSelectBitmap(ghdcMem2, hbmBitsT);
    GreSelectBitmap(ghdcMem, hbmDstT);

    /*
     * Fill in the iconinfo structure.  make copies of the bitmaps.
     */
    try {

        ccxpiconinfo->fIcon = (pcur->rt == PTR_TO_ID(RT_ICON));
        ccxpiconinfo->xHotspot = pcur->xHotspot;
        ccxpiconinfo->yHotspot = pcur->yHotspot;
        ccxpiconinfo->hbmMask  = hbmMask;
        ccxpiconinfo->hbmColor = hbmColor;

        if (pstrInstanceName != NULL) {

            if (pcur->atomModName) {
                pstrInstanceName->Length = (USHORT)
                        UserGetAtomName(pcur->atomModName,
                                        pstrInstanceName->Buffer,
                                        (int) (pstrInstanceName->MaximumLength / sizeof(WCHAR))
                                        * sizeof(WCHAR));
            } else {
                pstrInstanceName->Length = 0;
            }
        }

        if (pstrResName != NULL) {

            if (IS_PTR(pcur->strName.Buffer)) {
                RtlCopyUnicodeString(pstrResName, &pcur->strName);
            } else {
                *pstrResName = pcur->strName;
            }
        }

        if (ccxpbpp)
            *ccxpbpp = pcur->bpp;

    } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
        GreDeleteObject(hbmMask);
        GreDeleteObject(hbmColor);
        return FALSE;
    }

    return TRUE;
}

/***************************************************************************\
* _DestroyCursor
*
* History:
* 25-Apr-1991 DavidPe       Created.
* 04-Aug-1992 DarrinM       Now destroys ACONs as well.
\***************************************************************************/

BOOL _DestroyCursor(
    PCURSOR pcur,
    DWORD   cmdDestroy)
{
    PPROCESSINFO ppi;
    PPROCESSINFO ppiCursor;
    int          i;
    extern BOOL DestroyAniIcon(PACON pacon);

    if (pcur == NULL) {
        UserAssert(FALSE);
        return(TRUE);
    }
    ppi = PpiCurrent();
    ppiCursor = GETPPI(pcur);

    /*
     * Remove this icon from the caption icon cache.
     */
    for (i = 0; i < CCACHEDCAPTIONS; i++) {
        if (gcachedCaptions[i].spcursor == pcur) {
            Unlock( &(gcachedCaptions[i].spcursor) );
        }
    }

    /*
     * First step in destroying an cursor
     */
    switch (cmdDestroy) {

    case CURSOR_ALWAYSDESTROY:

        /*
         * Always destroy? then don't do any checking...
         */
        break;

    case CURSOR_CALLFROMCLIENT:

        /*
         * Can't destroy public cursors/icons.
         */
        if (ppiCursor == NULL)
            /*
             * Fake success if its a resource loaded icon because
             * this is how win95 responded.
             */
            return !!(pcur->CURSORF_flags & CURSORF_FROMRESOURCE);

        /*
         * If this cursor was loaded from a resource, don't free it till the
         * process exits.  This is the way we stay compatible with win3.0's
         * cursors which were actually resources.  Resources under win3 have
         * reference counting and other "features" like handle values that
         * never change.  Read more in the comment in
         * ServerLoadCreateCursorIcon().
         */
        if (pcur->CURSORF_flags & (CURSORF_LRSHARED | CURSORF_SECRET)) {
            return TRUE;
        }

        /*
         * One thread can't destroy the objects created by another.
         */
        if (ppiCursor != ppi) {
            RIPERR0(ERROR_DESTROY_OBJECT_OF_OTHER_THREAD, RIP_ERROR, "DestroyCursor: cursor belongs to another process");
            return FALSE;
        }

        /*
         * fall through.
         */

    case CURSOR_THREADCLEANUP:

        /*
         * Don't destroy public objects either (pretend it worked though).
         */
        if (ppiCursor == NULL)
            return TRUE;
        break;
    }

    /*
     * First mark the object for destruction.  This tells the locking code that
     * we want to destroy this object when the lock count goes to 0.  If this
     * returns FALSE, we can't destroy the object yet.
     */
    if (!HMMarkObjectDestroy((PHEAD)pcur))
        return FALSE;

    if (pcur->strName.Length != 0) {
        UserFreePool((LPSTR)pcur->strName.Buffer);
    }

    if (pcur->atomModName != 0) {
        UserDeleteAtom(pcur->atomModName);
    }

    /*
     * If this is an ACON call its special routine to destroy it.
     */
    if (pcur->CURSORF_flags & CURSORF_ACON) {
        DestroyAniIcon((PACON)pcur);
    } else {
        if (pcur->hbmMask != NULL) {
            GreDeleteObject(pcur->hbmMask);
            GreDecQuotaCount((PW32PROCESS)(pcur->head.ppi));
        }
        if (pcur->hbmColor != NULL) {
            GreDeleteObject(pcur->hbmColor);
            GreDecQuotaCount((PW32PROCESS)(pcur->head.ppi));
        }
        if (pcur->hbmAlpha != NULL) {
            /*
             * This is an internal GDI object, and so not covered by quota.
             */
            GreDeleteObject(pcur->hbmAlpha);
        }
    }

    /*
     * Ok to destroy...  Free the handle (which will free the object and the
     * handle).
     */
    DestroyEmptyCursorObject(pcur);
    return TRUE;
}



/***************************************************************************\
* DestroyUnlockedCursor
*
* Called when a cursor is destoyed due to an unlock.
*
* History:
* 24-Feb-1997 adams     Created.
\***************************************************************************/

void
DestroyUnlockedCursor(void * pv)
{
    _DestroyCursor((PCURSOR)pv, CURSOR_THREADCLEANUP);
}



/***************************************************************************\
* _SetCursorContents
*
*
* History:
* 27-Apr-1992 ScottLu   Created.
\***************************************************************************/

BOOL _SetCursorContents(
    PCURSOR pcur,
    PCURSOR pcurNew)
{
    HBITMAP hbmpT;

    if (!(pcur->CURSORF_flags & CURSORF_ACON)) {

        /*
         * Swap bitmaps.
         */
        hbmpT = pcur->hbmMask;
        pcur->hbmMask = pcurNew->hbmMask;
        pcurNew->hbmMask = hbmpT;

        hbmpT = pcur->hbmColor;
        pcur->hbmColor = pcurNew->hbmColor;
        pcurNew->hbmColor = hbmpT;

        /*
         * Remember hotspot info and size info
         */
        pcur->xHotspot = pcurNew->xHotspot;
        pcur->yHotspot = pcurNew->yHotspot;
        pcur->cx = pcurNew->cx;
        pcur->cy = pcurNew->cy;
    }

    /*
     * Destroy the cursor we copied from.
     */
    _DestroyCursor(pcurNew, CURSOR_THREADCLEANUP);

    return (BOOL)TRUE;
}
