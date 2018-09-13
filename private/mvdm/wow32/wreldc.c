//*****************************************************************************
//
// DC Cacheing -
//
//     Support for misbehaved apps - which continue to use a DC that has been
//     Released. Well the problem is WIN30 allows it, so we need to be
//     compatible.
//
//
// 03-Feb-92  NanduriR   Created.
//
//*****************************************************************************

#include "precomp.h"
#pragma hdrstop

MODNAME(wreldc.c);

BOOL GdiCleanCacheDC (HDC hdc16);

//*****************************************************************************
// count of currently cached DCs so that we can quickly check whether any
// ReleasedDCs are pending.
//
//*****************************************************************************

INT  iReleasedDCs = 0;


//*****************************************************************************
// The head of the linked list of the DCs Info.
//
//*****************************************************************************

LPDCCACHE lpDCCache = NULL;


//*****************************************************************************
// ReleaseCachedDCs -
//        ReleaseDC's a cached DC if it meets the 'search criterion'.
//        The Search flag indicates which input arguments will be used.
//        Unused arguments can be NULL or pure garbage.
//
//        NOTE: this does not free the memory that has been allocated for
//              the list.
//
//              We reset the flag 'flState' and thus will be able to
//              reuse the structure.
//
//        Returns TRUE
//*****************************************************************************

BOOL ReleaseCachedDCs(HAND16 htask16, HAND16 hwnd16, HAND16 hdc16,
                            HWND hwnd32, UINT flSearch)
{
    HAND16 hdcTemp;
    LPDCCACHE lpT;

    UNREFERENCED_PARAMETER(hdc16);

    if (iReleasedDCs) {
        for (lpT = lpDCCache; lpT != NULL; lpT = lpT->lpNext) {

             if (!(lpT->flState & DCCACHE_STATE_RELPENDING))
                 continue;

             hdcTemp = (HAND16)NULL;

             if (flSearch & SRCHDC_TASK16_HWND16) {

                 if (lpT->htask16 == htask16 &&
                         lpT->hwnd16 == hwnd16)
                     hdcTemp = lpT->hdc16;

             }
             else if (flSearch & SRCHDC_TASK16) {

                 if (lpT->htask16 == htask16)
                     hdcTemp = lpT->hdc16;

             }
             else {
                 LOGDEBUG(0, ("ReleaseCachedDCs:Invalid Search Flag\n"));
             }

             if (hdcTemp) {
                 if (ReleaseDC(lpT->hwnd32, HDC32(hdcTemp))) {
                     LOGDEBUG(6,
                         ("ReleaseCachedDCs: success hdc16 %04x - count %04x\n",
                                                   hdcTemp, (iReleasedDCs-1)));
                 }
                 else {
                     LOGDEBUG(7, ("ReleaseCachedDCs: FAILED hdc16 %04x\n",
                                                                    hdcTemp));
                 }

                 // reset the state evenif ReleaseDC failed

                 lpT->flState = 0;
                 if (!(--iReleasedDCs))
                     break;
             }
        }
    }

    return TRUE;
}


//*****************************************************************************
// FreeCachedDCs -
//        ReleaseDC's a cached DC - Normally called during taskexit.
//
//        NOTE: this does not free the memory that has been allocated for
//              the list.
//
//              We reset the flag 'flState' and thus will be able to
//              reuse the structure.
//
//        Returns TRUE
//*****************************************************************************

BOOL FreeCachedDCs(HAND16 htask16)
{
    HAND16 hdcTemp;
    LPDCCACHE lpT;

    for (lpT = lpDCCache; lpT != NULL; lpT = lpT->lpNext) {

         if ((lpT->flState & DCCACHE_STATE_INUSE) &&
                                               lpT->htask16 == htask16) {

             hdcTemp = lpT->hdc16;
             if (lpT->flState & DCCACHE_STATE_RELPENDING) {

                 if (ReleaseDC(lpT->hwnd32, HDC32(hdcTemp))) {
                     LOGDEBUG(6,
                         ("FreeCachedDCs: success hdc16 %04x - task %04x\n",
                                                               hdcTemp, htask16));
                 }
                 else {
                     LOGDEBUG(7, ("FreeCachedDCs: FAILED hdc16 %04x - task %04x\n",
                                                             hdcTemp, htask16));
                 }

                 WOW32ASSERT(iReleasedDCs != 0);
                 --iReleasedDCs;
             }

             lpT->flState = 0;
         }
    }

    return TRUE;
}


//*****************************************************************************
// StoredDC -
//
//        Initializes a DCCACHE structure with appropriate values.
//        Uses an empty slot in the linked list if available else
//        allocates a new one and adds to the head of the list.
//
//        Returns TRUE on success, FALSE on failure
//*****************************************************************************

BOOL StoreDC(HAND16 htask16, HAND16 hwnd16, HAND16 hdc16)
{
    HAND16 hdcTemp = (HAND16)NULL;
    LPDCCACHE lpT, lpNew;

    // Check for  an 'inuse' slot that will match the one that will be created
    // or check for an empty slot.
    //
    // An existing 'inuse' slot may match the one that's being created if
    // an app makes multiple calls to GetDC(hwnd) without an intervening
    // ReleaseDC. eg. MathCad.
    //                                                     - Nanduri

    lpNew = (LPDCCACHE)NULL;
    for (lpT = lpDCCache; lpT != NULL; lpT = lpT->lpNext) {
         if (lpT->flState & DCCACHE_STATE_INUSE) {
             if (lpT->hdc16 == hdc16) {
                 if (lpT->hwnd16 == hwnd16 &&
                         lpT->htask16 == htask16  ) {
                     LOGDEBUG(6, ("WOW:An identical second GetDC call without ReleaseDC\n"));
                 }
                 else {
                     LOGDEBUG(6, ("WOW:New DC 0x%04x exists - replacing old cache\n", hdc16));
                 }

                 if (lpT->flState & DCCACHE_STATE_RELPENDING) {
                     WOW32ASSERT(iReleasedDCs != 0);
                     --iReleasedDCs;
                 }

                 lpNew = lpT;
                 break;
             }
         }
         else {
             if (!lpNew)
                 lpNew = lpT;
         }
    }
    lpT = lpNew;

    if (lpT == NULL) {
        lpT = (LPDCCACHE)malloc_w_small(sizeof(DCCACHE));
        if (lpT) {
            lpT->lpNext = lpDCCache;
            lpDCCache = lpT;
        }
        else {
            LOGDEBUG(0, ("StoreDC: malloc_w_small for cache failed\n"));
        }
    }

    if (lpT != NULL) {
        lpT->flState = DCCACHE_STATE_INUSE;
        lpT->htask16 = htask16;
        lpT->hwnd16 = hwnd16;
        lpT->hdc16 = hdc16;
        lpT->hwnd32 = HWND32(hwnd16);
        LOGDEBUG(6, ("StoreDC: Added hdc %04x\n",hdc16));
        return TRUE;
    }
    else
        return FALSE;
}

//*****************************************************************************
// CacheReleasedDC -
//
//        Increments iReleasedDCs to indicate that a ReleaseDC is pending.
//
//        Increments the iReleasedDC only if there was a corresponding GetDC.
//        i.e, only if the DC exists in the DCcache;
//
//        This is to handle the scenrio below:
//                 hdc = BeginPaint(hwnd,..);
//                 ReleaseDC(hwnd, hdc);
//                 EndPaint(hwnd, ..);
//
//
//        Returns TRUE on success, FALSE on failure
//*****************************************************************************

BOOL CacheReleasedDC(HAND16 htask16, HAND16 hwnd16, HAND16 hdc16)
{
    HAND16 hdcTemp = (HAND16)NULL;
    LPDCCACHE lpT;

    for (lpT = lpDCCache; lpT != NULL; lpT = lpT->lpNext) {

         if ((lpT->flState & DCCACHE_STATE_INUSE) &&
                 lpT->htask16 == htask16 &&
                 lpT->hwnd16 == hwnd16 &&
                 lpT->hdc16 == hdc16  ) {


             // the app might do releasedc twice on the same dc by mistake

             if (!(lpT->flState & DCCACHE_STATE_RELPENDING)) {
                 lpT->flState |= DCCACHE_STATE_RELPENDING;
                 iReleasedDCs++;
             }
             LOGDEBUG(6, ("CachedReleasedDC: Pending hdc %04x - count %04x\n",
                                                         hdc16, iReleasedDCs));
             GdiCleanCacheDC (HDC32(hdc16));

             // Fix apps that draw then do lots
             // of disk activity, usually they do
             // a releaseDC.  This flush will syncronize
             // the drawing with the beginning of the
             // disk activity.  Bug #9704 PackRats install program draws text too late

             GdiFlush();

             return TRUE;
         }

    }

    return FALSE;
}
