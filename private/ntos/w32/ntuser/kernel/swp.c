/****************************** Module Header ******************************\
* Module Name: swp.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Contains the xxxSetWindowPos API and related functions.
*
* History:
* 20-Oct-1990 DarrinM   Created.
* 25-Jan-1991 IanJa     added window revalidation
* 11-Jul-1991 DarrinM   Replaced everything with re-ported Win 3.1 code.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

#define CTM_NOCHANGE        0
#define CTM_TOPMOST         1
#define CTM_NOTOPMOST       2

void
FixBogusSWP(PWND pwnd, int * px, int * py, int cx, int cy, UINT flags);
void PreventInterMonitorBlts(PCVR pcvr);

/***************************************************************************\
* DBGCheskSMWP
*
* SMWP can be a HM object, a cached structure or just a pool allocation
*
* History:
* 05/21/98 GerardoB     Created.
\***************************************************************************/
#if DBG
void DBGCheskSMWP (PSMWP psmwp)
{
    if (psmwp->bHandle) {
        UserAssert(psmwp->head.h != NULL);
        UserAssert(psmwp == HtoPqCat(PtoHq(psmwp)));
        UserAssert(psmwp != &gSMWP);
    } else {
        UserAssert((psmwp->head.h == NULL) && (psmwp->head.cLockObj == 0));
        if (psmwp == &gSMWP) {
            UserAssert(TEST_PUDF(PUDF_GSMWPINUSE));
        }
    }

    UserAssert(psmwp->ccvr <= psmwp->ccvrAlloc);
    UserAssert(psmwp->acvr != NULL);

}
#else
#define DBGCheskSMWP(psmwp)
#endif
/***************************************************************************\
* DestroySMWP
*
* Destroys an SMWP object.
*
* History:
* 24-Feb-1997 adams     Created.
\***************************************************************************/

void
DestroySMWP(PSMWP psmwp)
{
    BOOL fFree;

    CheckCritIn();

    DBGCheskSMWP(psmwp);
    /*
     * First mark the object for destruction.  This tells the locking code
     * that we want to destroy this object when the lock count goes to 0.
     * If this returns FALSE, we can't destroy the object yet.
     */
    if (psmwp->bHandle) {
        if (!HMMarkObjectDestroy(psmwp)) {
            return;
        }
        fFree = TRUE;
    } else {
        /*
         * Is this the global cached structure?
         */
        fFree = (psmwp != &gSMWP);
    }

    if (psmwp->acvr) {

        /*
         * Free any hrgnInterMonitor stuff we accumulated.
         */
        PCVR pcvr;
        int ccvr;

        for (pcvr = psmwp->acvr, ccvr = psmwp->ccvr; --ccvr >= 0; pcvr++) {
            if (pcvr->hrgnInterMonitor != NULL) {
                GreDeleteObject(pcvr->hrgnInterMonitor);
            }
        }

        if (fFree) {
            UserFreePool(psmwp->acvr);
        }
    }

    /*
     * Ok to destroy...  Free the handle (which will free the object
     * and the handle).
     */
    if (psmwp->bHandle) {
        HMFreeObject(psmwp);
    } else if (fFree) {
        UserFreePool(psmwp);
    } else {
        UserAssert(TEST_PUDF(PUDF_GSMWPINUSE));
        CLEAR_PUDF(PUDF_GSMWPINUSE);
        /*
         * If acvr grew too much, shrink it.
         * Don't use realloc since we don't care about the left over data
         */
        if (psmwp->ccvrAlloc > 8) {
            PCVR pcvr = UserAllocPool(4 * sizeof(CVR), TAG_SWP);
            if (pcvr != NULL) {
                UserFreePool(psmwp->acvr);
                psmwp->acvr = pcvr;
                psmwp->ccvrAlloc = 4;
            }
        }
    }
}

/***************************************************************************\
* MoveWindow (API)
*
*
* History:
* 25-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

#define MW_FLAGS_REDRAW   (SWP_NOZORDER | SWP_NOACTIVATE)
#define MW_FLAGS_NOREDRAW (SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW)

BOOL xxxMoveWindow(
    PWND pwnd,
    int  x,
    int  y,
    int  cx,
    int  cy,
    BOOL fRedraw)
{
    CheckLock(pwnd);

    if ((pwnd == PWNDDESKTOP(pwnd)) ||
        TestWF(pwnd, WFWIN31COMPAT) ||
        (pwnd->spwndParent != PWNDDESKTOP(pwnd))) {

        return xxxSetWindowPos(
                pwnd,
                NULL,
                x,
                y,
                cx,
                cy,
                (fRedraw ? MW_FLAGS_REDRAW : MW_FLAGS_NOREDRAW));
    } else {

        /*
         * BACKWARD COMPATIBILITY CODE FOR WIN 3.00 AND BELOW
         *
         * Everyone and their brother seems to depend on this behavior for
         * top-level windows.  Specific examples are:
         *
         *  AfterDark help window animation
         *  Finale Speedy Note Editing
         *
         * If the window is a top-level window and fRedraw is FALSE,
         * we must call SetWindowPos with SWP_NOREDRAW CLEAR anyway so that
         * the frame and window background get drawn.  We then validate the
         * entire client rectangle to avoid repainting that.
         */
        BOOL fResult = xxxSetWindowPos(pwnd,
                                       NULL,
                                       x,
                                       y,
                                       cx,
                                       cy,
                                       MW_FLAGS_REDRAW);

        if (!fRedraw)
            xxxValidateRect(pwnd, NULL);

        return fResult;
    }
}

/***************************************************************************\
* AllocateCvr
*
* History:
* 05/20/98  GerardoB    Extracted from old _BeginDeferWindowPos
\***************************************************************************/
BOOL AllocateCvr (PSMWP psmwp, int cwndHint)
{
    PCVR  acvr;

    UserAssert(cwndHint != 0);
    acvr = (PCVR)UserAllocPoolWithQuota(sizeof(CVR) * cwndHint, TAG_SWP);

    if (acvr == NULL) {
        return FALSE;
    }

    /*
     * Initialize psmwp related fields.
     * CVR array is initialized by _DeferWindowPos
     */

    psmwp->acvr      = acvr;
    psmwp->ccvrAlloc = cwndHint;
    psmwp->ccvr      = 0;
    return TRUE;
}
/***************************************************************************\
* InternalBeginDeferWindowPos
*
* History:
* 05/20/98  GerardoB    Created
\***************************************************************************/

PSMWP InternalBeginDeferWindowPos(int cwndHint)
{
    PSMWP psmwp;

    CheckCritIn();

    /*
     * If gSMWP in being used, allocate one.
     * Note that SMWP is zero init but CVR is not; _DeferWindowPos initializes it
     */
    if (TEST_PUDF(PUDF_GSMWPINUSE) || (cwndHint > gSMWP.ccvrAlloc)) {
        psmwp = (PSMWP)UserAllocPoolWithQuotaZInit(sizeof(SMWP), TAG_SWP);
        if (psmwp == NULL) {
            return NULL;
        }
        if (!AllocateCvr(psmwp, cwndHint)) {
            UserFreePool(psmwp);
            return NULL;
        }
    } else {
        SET_PUDF(PUDF_GSMWPINUSE);
        psmwp = &gSMWP;
        RtlZeroMemory(&gSMWP, FIELD_OFFSET(SMWP, ccvrAlloc));
        UserAssert(gSMWP.ccvr == 0);
        UserAssert(gSMWP.acvr != NULL);
    }

    DBGCheskSMWP(psmwp);
    return psmwp;
}
/***************************************************************************\
* BeginDeferWindowPos (API)
*
* This must be called from the client side only. Internally we should
*  call InternalBeginDeferWindowPos to avoid going through the handle table
*  and perhaps even use the cached strucuture.
*
* History:
* 11-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

PSMWP _BeginDeferWindowPos(int cwndHint)
{
    PSMWP psmwp;

    psmwp = (PSMWP)HMAllocObject(PtiCurrent(), NULL, TYPE_SETWINDOWPOS, sizeof(SMWP));
    if (psmwp == NULL) {
        return NULL;
    }

    if (cwndHint == 0) {
        cwndHint = 8;
    }

    if (!AllocateCvr(psmwp, cwndHint)) {
        HMFreeObject(psmwp);
        return NULL;
    }

    psmwp->bHandle = TRUE;
    DBGCheskSMWP(psmwp);
    return psmwp;
}

/***************************************************************************\
* PWInsertAfter
* HWInsertAfter
* History:
* 04-Mar-1992 MikeKe    From win31
\***************************************************************************/
PWND PWInsertAfter(
   HWND hwnd)
{
    PWND pwnd;

    /*
     * HWND_GROUPTOTOP and HWND_TOPMOST are the same thing.
     */
    switch ((ULONG_PTR)hwnd) {
    case (ULONG_PTR)HWND_TOP:
    case (ULONG_PTR)HWND_BOTTOM:
    case (ULONG_PTR)HWND_TOPMOST:
    case (ULONG_PTR)HWND_NOTOPMOST:
        return (PWND)hwnd;

    default:

        /*
         * Don't insert after a destroyed window!  It will cause the
         * window being z-ordered to become unlinked from it's siblings.
         */
        if (pwnd = RevalidateHwnd(hwnd)) {

            /*
             * Do not insert after a destroyed window.  Put it at the
             * bottom of the list, if it is z-ordered at all.
             */
            if (TestWF(pwnd, WFDESTROYED) || pwnd->spwndParent == NULL)
                return NULL;

            UserAssert(_IsDescendant(pwnd->spwndParent, pwnd));
            return pwnd;
        }

        return NULL;
    }
}

HWND HWInsertAfter(
    PWND pwnd)
{
    /*
     * HWND_GROUPTOTOP and HWND_TOPMOST are the same thing.
     */
    switch ((ULONG_PTR)pwnd) {
    case (ULONG_PTR)HWND_TOP:
    case (ULONG_PTR)HWND_BOTTOM:
    case (ULONG_PTR)HWND_TOPMOST:
    case (ULONG_PTR)HWND_NOTOPMOST:
        return (HWND)pwnd;

    default:
        return HW(pwnd);
    }
}

/***************************************************************************\
* DeferWindowPos (API)
*
*
* History:
* 07-11-91 darrinm      Ported from Win 3.1 sources.
\***************************************************************************/

PSMWP _DeferWindowPos(
    PSMWP psmwp,
    PWND  pwnd,
    PWND  pwndInsertAfter,
    int   x,
    int   y,
    int   cx,
    int   cy,
    UINT  flags)
{
    PWINDOWPOS ppos;
    PCVR       pcvr;

    DBGCheskSMWP(psmwp);
    if (psmwp->ccvr + 1 > psmwp->ccvrAlloc) {

        /*
         * Make space for 4 more windows
         */
        DWORD dwNewAlloc = psmwp->ccvrAlloc + 4;

        pcvr = (PCVR)UserReAllocPoolWithQuota(psmwp->acvr,
                                              psmwp->ccvrAlloc * sizeof(CVR),
                                              sizeof(CVR) * dwNewAlloc,
                                              TAG_SWP);

        if (pcvr == NULL) {
            DestroySMWP(psmwp);
            return NULL;
        }

        psmwp->acvr = pcvr;
        psmwp->ccvrAlloc = dwNewAlloc;
    }

    pcvr = &psmwp->acvr[psmwp->ccvr++];
    ppos = &pcvr->pos;

    ppos->hwnd            = HWq(pwnd);
    ppos->hwndInsertAfter = (TestWF(pwnd, WFBOTTOMMOST)) ?
                                HWND_BOTTOM : HWInsertAfter(pwndInsertAfter);
    ppos->x               = x;
    ppos->y               = y;
    ppos->cx              = cx;
    ppos->cy              = cy;
    ppos->flags           = flags;

    pcvr->hrgnClip = NULL;
    pcvr->hrgnInterMonitor = NULL;

    return psmwp;
}
/***************************************************************************\
* ValidateWindowPos
*
* checks validity of SWP structure
*
* NOTE: For performance reasons, this routine is only called
*       in the DEBUG version of USER.
*
* History:
* 10-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

BOOL ValidateWindowPos(PCVR pcvr, PWND pwndParent)
{
    PWND pwnd;
    PWND pwndInsertAfter;
    HWND hwndInsertAfter;

    if ((pwnd = RevalidateHwnd(pcvr->pos.hwnd)) == NULL)
        return FALSE;

    /*
     * Save the pti
     */
    pcvr->pti = GETPTI(pwnd);


    /*
     * If the SWP_NOZORDER bit is not set, validate the Insert behind window.
     */
    if (!(pcvr->pos.flags & SWP_NOZORDER)) {
        BOOL fTopLevel = (pwnd->spwndParent == PWNDDESKTOP(pwnd));
        /*
         * Do not z-order destroyed windows
         */
        if (TestWF(pwnd, WFDESTROYED))
            return FALSE;

        hwndInsertAfter = pcvr->pos.hwndInsertAfter;
        /*
         * If pwndParent is provided, we're about to link this window so we
         *  need to validate LinkWindow assumptions.
         * We have to do this since we callback after determining hwndInsertAfter.
         */

        if ((hwndInsertAfter == HWND_TOPMOST) ||
            (hwndInsertAfter == HWND_NOTOPMOST)) {

            if (!fTopLevel) {
                return FALSE;
            }
        } else if (hwndInsertAfter == HWND_TOP) {
            /*
             * if pwnd is not topmost, the first child must not be topmost.
             */
            if ((pwndParent != NULL) && fTopLevel
                    && !FSwpTopmost(pwnd)
                    && (pwndParent->spwndChild != NULL)
                    && FSwpTopmost(pwndParent->spwndChild)) {

                RIPMSG2(RIP_WARNING, "ValidateWindowPos: pwnd is not SWPTopMost."
                                     " pwnd:%#p. hwndInsertAfter:%#p",
                                      pwnd, hwndInsertAfter);
                return FALSE;
            }
        } else if (hwndInsertAfter != HWND_BOTTOM) {

            /*
             * Ensure pwndInsertAfter is valid
             */
            if (((pwndInsertAfter = RevalidateHwnd(hwndInsertAfter)) == NULL) ||
                    TestWF(pwndInsertAfter, WFDESTROYED)) {

                RIPERR1(ERROR_INVALID_HANDLE, RIP_WARNING, "Invalid hwndInsertAfter (%#p)", hwndInsertAfter);

                return FALSE;
            }

            /*
             * Ensure that pwndInsertAfter is a sibling of pwnd
             */
            if (pwnd == pwndInsertAfter ||
                    pwnd->spwndParent != pwndInsertAfter->spwndParent) {
                RIPMSG2(RIP_WARNING, "hwndInsertAfter (%#p) is not a sibling "
                        "of hwnd (%#p)", hwndInsertAfter, pcvr->pos.hwnd);
                return FALSE;
            }
            /*
             * Ensure proper topmost/nontopmost insert position
             */
            if ((pwndParent != NULL) && fTopLevel) {
                if (FSwpTopmost(pwnd)) {
                    /*
                     * Check if we're trying to insert a topmost window after a non-topmost one.
                     */
                    if (!FSwpTopmost(pwndInsertAfter)) {
                        RIPMSG2(RIP_WARNING, "ValidateWindowPos: pwndInsertAfter is not SWPTopMost."
                                             " pwnd:%#p. pwndInsertAfter:%#p",
                                              pwnd, pwndInsertAfter);
                        return FALSE;
                    }
                } else {
                    /*
                     * Check if we're trying to insert a non-top most window between two
                     *  top-most ones.
                     */
                    if ((pwndInsertAfter->spwndNext != NULL)
                            && FSwpTopmost(pwndInsertAfter->spwndNext)) {

                        RIPMSG2(RIP_WARNING, "ValidateWindowPos: pwndInsertAfter->spwndNext is SWPTopMost."
                                             " pwnd:%#p. pwndInsertAfter:%#p",
                                              pwnd, pwndInsertAfter);
                        return FALSE;
                    }
                }

            }

        } /* if (hwndInsertAfter != HWND_TOP && hwndInsertAfter != HWND_BOTTOM) */

        /*
         * Check that the parent hasn't changed.
         */
        if (pwndParent != NULL) {
            if (pwndParent != pwnd->spwndParent) {
                RIPMSG3(RIP_WARNING, "ValidateWindowPos: parent has changed."
                                     " pwnd:%#p. Old Parent:%#p. Current Parent:%#p",
                                      pwnd, pwndParent, pwnd->spwndParent);
                return FALSE;
            }
        }

    } /* if (!(pcvr->pos.flags & SWP_NOZORDER)) */

    return TRUE;
}

/***************************************************************************\
* IsStillWindowC
*
* Checks if window is valid HWNDC still, and child of proper dude.
*
* History:
\***************************************************************************/

BOOL IsStillWindowC(
    HWND hwndc)
{
    switch ((ULONG_PTR)hwndc) {
    case (ULONG_PTR)HWND_TOP:
    case (ULONG_PTR)HWND_BOTTOM:
    case (ULONG_PTR)HWND_TOPMOST:
    case (ULONG_PTR)HWND_NOTOPMOST:
        return TRUE;

    default:
        /*
         * Make sure we're going to insert after a window that's
         *  (1) Valid
         *  (2) Peer
         */
        return (RevalidateHwnd(hwndc) != 0);
    }
}

/***************************************************************************\
* ValidateSmwp
*
* Validate the SMWP and figure out which window should get activated,
*
* History:
* 10-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

BOOL ValidateSmwp(
    PSMWP psmwp,
    BOOL  *pfSyncPaint)
{
    PCVR pcvr;
    PWND pwndParent;
    PWND pwndT;
    int  ccvr;

    *pfSyncPaint = TRUE;

    pwndT = RevalidateHwnd(psmwp->acvr[0].pos.hwnd);

    if (pwndT == NULL)
        return FALSE;

    pwndParent = pwndT->spwndParent;

    /*
     * Validate the passed-in WINDOWPOS structs, and find a window to activate.
     */
    for (pcvr = psmwp->acvr, ccvr = psmwp->ccvr; --ccvr >= 0; pcvr++) {

        if (!ValidateWindowPos(pcvr, NULL)) {
            pcvr->pos.hwnd = NULL;
            continue;
        }

        /*
         * All windows in the pos list must have the same parent.
         * If not, yell and return FALSE.
         */
        UserAssert(IsStillWindowC(pcvr->pos.hwnd));

        UserAssert(PW(pcvr->pos.hwnd));
        if (PW(pcvr->pos.hwnd)->spwndParent != pwndParent) {
            RIPERR0(ERROR_HWNDS_HAVE_DIFF_PARENT, RIP_VERBOSE, "");
            return FALSE;
        }

        /*
         * If SWP_DEFERDRAWING is set for any of the windows, suppress
         * DoSyncPaint() call later.
         */
        if (pcvr->pos.flags & SWP_DEFERDRAWING)
            *pfSyncPaint = FALSE;
    }

    return TRUE;
}

/***************************************************************************\
* FindValidWindowPos
*
* Some of the windows in the SMWP list may be NULL at ths point (removed
* because they'll be handled by their creator's thread) so we've got to
* look for the first non-NULL window and return it.
*
* History:
* 10-Sep-1991 DarrinM    Created.
\***************************************************************************/

PWINDOWPOS FindValidWindowPos(
    PSMWP psmwp)
{
    int i;

    for (i = 0; i < psmwp->ccvr; i++) {

        if (psmwp->acvr[i].pos.hwnd != NULL)
            return &psmwp->acvr[i].pos;
    }

    return NULL;
}

/***************************************************************************\
*
*  GetLastNonBottomMostWindow()
*
*  Returns the last non bottom-most window in the z-order, NULL if
*  there isn't one.  When figuring out whom to insert after, we want to
*  skip ourself.  But when figuring out if we're already in place, we don't
*  want to skip ourself on enum.
*
* History:
\***************************************************************************/

PWND GetLastNonBottomMostWindow(
    PWND pwnd,
    BOOL fSkipSelf)
{
    PWND pwndT;
    PWND pwndLast = NULL;

    for (pwndT = pwnd->spwndParent->spwndChild;
         pwndT && !TestWF(pwndT, WFBOTTOMMOST);
         pwndT = pwndT->spwndNext) {

        if (!fSkipSelf || (pwnd != pwndT))
            pwndLast = pwndT;
    }

    return pwndLast;
}

/***************************************************************************\
* ValidateZorder
*
* Checks to see if the specified window is already in the specified Z order
* position, by comparing the current Z position with the specified
* pwndInsertAfter.
*
* History:
* 11-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

BOOL ValidateZorder(
    PCVR pcvr)
{
    PWND pwnd;
    PWND pwndPrev;
    PWND pwndInsertAfter;
    BYTE bTopmost;

    /*
     * Validate just to make sure this routine doesn't do anything bogus.
     * Its caller will actually redetect and handle the error.
     */
    UserAssert(RevalidateCatHwnd(pcvr->pos.hwnd));
    pwnd = PWCat(pcvr->pos.hwnd);      // Known to be valid at this point.

    /*
     * Don't z-order a destroyed window
     */
    if (TestWF(pwnd, WFDESTROYED))
        return TRUE;

    UserAssert((HMPheFromObject(pwnd)->bFlags & HANDLEF_DESTROY) == 0);

    pwndInsertAfter = PWInsertAfter(pcvr->pos.hwndInsertAfter);
    if (pcvr->pos.hwndInsertAfter != NULL && pwndInsertAfter == NULL)
        return TRUE;

    if (pwndInsertAfter == PWND_BOTTOM) {

        if (TestWF(pwnd, WFBOTTOMMOST))
            return(pwnd->spwndNext == NULL);
        else
            return(pwnd == GetLastNonBottomMostWindow(pwnd, FALSE));
    }

    pwndPrev = pwnd->spwndParent->spwndChild;
    if (pwndInsertAfter == PWND_TOP)
        return pwndPrev == pwnd;

    if (TestWF(pwndInsertAfter, WFDESTROYED))
        return TRUE;

    /*
     * When we compare the state of the window, we must use
     * the EVENTUAL state of the window that is moving, but
     * the CURRENT state of the window it's inserted behind.
     *
     * Prevent nonbottommost windows from going behind the bottommost one
     */
    if (TestWF(pwndInsertAfter, WFBOTTOMMOST)) {
        pcvr->pos.hwndInsertAfter = HWInsertAfter(GetLastNonBottomMostWindow(pwnd, TRUE));
        return FALSE;
    }

    /*
     * If we are not topmost, but pwndInsertAfter is, OR
     * if we are topmost, but pwndInsertAfter is not,
     * we need to adjust pwndInsertAfter to be the last of
     * the topmost windows.
     */
    bTopmost = TestWF(pwnd, WEFTOPMOST);

    if (TestWF(pwnd, WFTOGGLETOPMOST))
        bTopmost ^= LOBYTE(WEFTOPMOST);

    if (bTopmost != (BYTE)TestWF(pwndInsertAfter, WEFTOPMOST)) {

        pwndInsertAfter = GetLastTopMostWindow();

        /*
         * We're correctly positioned if we're already at the bottom
         */
        if (pwndInsertAfter == pwnd)
            return TRUE;

        pcvr->pos.hwndInsertAfter = HW(pwndInsertAfter);
    }

    /*
     * Look for our previous window in the list...
     */
    if (pwndPrev != pwnd) {

        for ( ; pwndPrev != NULL; pwndPrev = pwndPrev->spwndNext) {

            if (pwndPrev->spwndNext == pwnd)
                return pwndInsertAfter == pwndPrev;
        }

        /*
         * If we get to here, pwnd is not in the sibling list.
         * REALLY BAD NEWS!
         */
        UserAssert(FALSE);
        return TRUE;
    }

    return FALSE;
}

/***************************************************************************\
* xxxCalcValidRects
*
* Based on the WINDOWPOS flags in the fs parameter in each WINDOWPOS structure,
* this routine calcs the new position and size of each window, determines if
* its changing Z order, or whether its showing or hiding.  Any redundant
* flags are AND'ed out of the fs parameter.  If no redrawing is needed,
* SWP_NOREDRAW is OR'ed into the flags.  This is called from EndDeferWindowPos.
*
* History:
* 10-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

BOOL xxxCalcValidRects(
    PSMWP psmwp,
    HWND  *phwndNewActive)
{
    PCVR              pcvr;
    PWND              pwnd;
    PWND              pwndParent;
    HWND              hwnd;
    HWND              hwndNewActive = NULL;
    PWINDOWPOS        ppos;
    BOOL              fNoZorder;
    BOOL              fForceNCCalcSize;
    NCCALCSIZE_PARAMS params;
    int               cxSrc;
    int               cySrc;
    int               cxDst;
    int               cyDst;
    int               cmd;
    int               ccvr;
    int               xClientOld;
    int               yClientOld;
    int               cxClientOld;
    int               cyClientOld;
    int               xWindowOld;
    int               yWindowOld;
    int               cxWindowOld;
    int               cyWindowOld;
    TL                tlpwndParent;
    TL                tlpwnd;

    /*
     * Some of the windows in the SMWP list may be NULL at ths point
     * (removed because they'll be handled by their creator's thread)
     * so we've got to look for the first non-NULL window before we can
     * execute some of the tests below.  FindValidWindowPos returns NULL if
     * the list has no valid windows in it.
     */
    if ((ppos = FindValidWindowPos(psmwp)) == NULL)
        return FALSE;

    UserAssert(PW(ppos->hwnd));
    pwndParent = PW(ppos->hwnd)->spwndParent;

    UserAssert(HMRevalidateCatHandle(PtoH(pwndParent)));

    ThreadLock(pwndParent, &tlpwndParent);

    fNoZorder = TRUE;

    /*
     * Go through the SMWP list, enumerating each WINDOWPOS, and compute
     * its new window and client rectangles.
     */
    for (pcvr = psmwp->acvr, ccvr = psmwp->ccvr; --ccvr >= 0; pcvr++) {

        /*
         * This loop may leave the critsect during each iteration so
         * we revalidate pos.hwnd before use.
         */
        if ((hwnd = pcvr->pos.hwnd) == NULL)
            continue;

        pwnd = RevalidateHwnd(hwnd);

        if ((pwnd == NULL) || !IsStillWindowC(pcvr->pos.hwndInsertAfter)) {
            pcvr->pos.hwnd  = NULL;
            pcvr->pos.flags = SWP_NOREDRAW | SWP_NOCHANGE;
            continue;
        }

        ThreadLockAlways(pwnd, &tlpwnd);

        /*
         * Used for 3.0 compatibility.  3.0 sent the NCCALCSIZE message even if
         * the size of the window wasn't changing.
         */
        fForceNCCalcSize = FALSE;

        if (!hwndNewActive && !(pcvr->pos.flags & SWP_NOACTIVATE))
            hwndNewActive = HWq(pwnd);

        if (!(pcvr->pos.flags & SWP_NOSENDCHANGING)) {

            PWND pwndT;

            xxxSendMessage(pwnd, WM_WINDOWPOSCHANGING, 0, (LPARAM)&pcvr->pos);


            /*
             * Don't let them change pcvr->pos.hwnd. It doesn't make sense
             *  plus it'll mess us up.
             * I'm making this RIP_ERROR because we're too close to RTM (7/11/96)
             *  just to make sure that we won't break anyone. This should be
             *  changed to a RIP_WARNING after we ship. Use LOWORD to ignore
             *  "changes" by NTVDM
             */
#if DBG
            if (LOWORD(pcvr->pos.hwnd) != LOWORD(hwnd)) {
                RIPMSG0(RIP_ERROR,
                        "xxxCalcValidRects: Ignoring pcvr->pos.hwnd change by WM_WINDOWPOSCHANGING");
            }
#endif
            pcvr->pos.hwnd = hwnd;

            /*
             * If the window sets again 'hwndInsertAfter' to HWND_NOTOPMOST
             * or HWND_TOPMOST, we need to set this member appropriately.
             * See CheckTopmost for details.
             */
            if (pcvr->pos.hwndInsertAfter == HWND_NOTOPMOST) {
                if (TestWF(pwnd, WEFTOPMOST)) {

                    pwndT = GetLastTopMostWindow();
                    pcvr->pos.hwndInsertAfter = HW(pwndT);

                    if (pcvr->pos.hwndInsertAfter == pcvr->pos.hwnd) {
                        pwndT = _GetWindow(pwnd, GW_HWNDPREV);
                        pcvr->pos.hwndInsertAfter = HW(pwndT);
                    }
                } else {
                    pwndT = _GetWindow(pwnd, GW_HWNDPREV);
                    pcvr->pos.hwndInsertAfter = HW(pwndT);
                }
            } else if (pcvr->pos.hwndInsertAfter == HWND_TOPMOST) {
                pcvr->pos.hwndInsertAfter = HWND_TOP;
            }
        }
        /*
         * make sure the rectangle still matches the window's region
         *
         * Remember the old window rectangle in parent coordinates
         */
        xWindowOld  = pwnd->rcWindow.left;
        yWindowOld  = pwnd->rcWindow.top;
        if (pwndParent != PWNDDESKTOP(pwnd)) {
            xWindowOld -= pwndParent->rcClient.left;
            yWindowOld -= pwndParent->rcClient.top;
        }


        cxWindowOld = pwnd->rcWindow.right - pwnd->rcWindow.left;
        cyWindowOld = pwnd->rcWindow.bottom - pwnd->rcWindow.top;

        /*
         * Assume the client is not moving or sizing
         */
        pcvr->pos.flags |= SWP_NOCLIENTSIZE | SWP_NOCLIENTMOVE;

        if (!(pcvr->pos.flags & SWP_NOMOVE)) {

            if (pcvr->pos.x == xWindowOld && pcvr->pos.y == yWindowOld)
                pcvr->pos.flags |= SWP_NOMOVE;


#ifdef USE_MIRRORING
            /*
             * Since we are comparing client coordinates to see whether
             * to move the window or not :
             * We've to check if the parent of the window-to-be-positioned is RTL
             * mirrored, then let's measure the client old x-coordinate from
             * the right visual edge. This is because the position of a window
             * is mirrored in the parent client area if the parent is RTL mirrored.
             */
            if (TestWF(pwndParent, WEFLAYOUTRTL) && TestwndChild(pwnd) &&
                (pwndParent->rcClient.right - pwnd->rcWindow.right + 1) != pcvr->pos.x)
                pcvr->pos.flags &= ~SWP_NOMOVE;
#endif

            if (TestWF(pwnd, WFMINIMIZED) && IsTrayWindow(pwnd)) {
                pcvr->pos.x = WHERE_NOONE_CAN_SEE_ME;
                pcvr->pos.y = WHERE_NOONE_CAN_SEE_ME;
            }

        } else {
            pcvr->pos.x = xWindowOld;
            pcvr->pos.y = yWindowOld;
        }

        if (!(pcvr->pos.flags & SWP_NOSIZE)) {

            /*
             * Don't allow an invalid window rectangle.
             * BOGUS HACK: For Norton Antivirus, they call
             * MoveWindow at WM_CREATE Time EVEN though
             * the window is minimzed, but they assume its
             * restored at WM_CREATE time.... B#11185, t-arthb
             */
            if (TestWF(pwnd, WFMINIMIZED) &&
                _GetProp(pwnd, PROP_CHECKPOINT, PROPF_INTERNAL)) {

                pcvr->pos.cx = SYSMET(CXMINIMIZED);
                pcvr->pos.cy = SYSMET(CYMINIMIZED);

            } else {
                if (pcvr->pos.cx < 0)
                    pcvr->pos.cx = 0;

                if (pcvr->pos.cy < 0)
                    pcvr->pos.cy = 0;
            }

            if (pcvr->pos.cx == cxWindowOld && pcvr->pos.cy == cyWindowOld) {
                pcvr->pos.flags |= SWP_NOSIZE;
                if (!TestWF(pwnd, WFWIN31COMPAT))
                    fForceNCCalcSize = TRUE;
            }
        } else {
            pcvr->pos.cx = cxWindowOld;
            pcvr->pos.cy = cyWindowOld;
        }

#ifdef USE_MIRRORING
        if (TestWF(pwndParent, WEFLAYOUTRTL) && TestwndChild(pwnd) && (pwndParent != PWNDDESKTOP(pwnd))) {
            if (!(pcvr->pos.flags & SWP_NOMOVE)) {
                pcvr->pos.x = (pwndParent->rcClient.right - pwndParent->rcClient.left) - (pcvr->pos.x + pcvr->pos.cx);
            } else {
                if (!(pcvr->pos.flags & SWP_NOSIZE)) {
                    pcvr->pos.x -= (pcvr->pos.cx - cxWindowOld);
                }
            }
        }
#endif

        /*
         * If showing and already visible, or hiding and already hidden,
         * turn off the appropriate bit.
         */
        if (TestWF(pwnd, WFVISIBLE)) {
            pcvr->pos.flags &= ~SWP_SHOWWINDOW;
        } else {
            pcvr->pos.flags &= ~SWP_HIDEWINDOW;

            /*
             * If hidden, and we're NOT showing, then we won't be drawing,
             * no matter what else is going on.
             */
            if (!(pcvr->pos.flags & SWP_SHOWWINDOW))
                pcvr->pos.flags |= SWP_NOREDRAW;
        }

        /*
         * Muck with the zorder for bottommost windows, again
         * See comment in DeferWindowPos
         */
        if (TestWF(pwnd, WFBOTTOMMOST)) {
            pcvr->pos.flags &= ~SWP_NOZORDER;
            pcvr->pos.hwndInsertAfter = HWND_BOTTOM;
        }

        /*
         * If we're Z-ordering, we can try to remove the Z order
         * bit, as long as all previous windows in the WINDOWPOS list
         * have SWP_NOZORDER set.
         *
         * The reason we don't do this for each window individually
         * is that a window's eventual Z order depends on changes that
         * may have occured on windows earlier in the WINDOWPOS list,
         * so we can only call ValidateZorder if none of the previous
         * windows have changed.
         */
        if (fNoZorder && !(pcvr->pos.flags & SWP_NOZORDER)) {

            /*
             * If the TOPMOST bit is changing, the Z order is "changing",
             * so don't clear the bit even if it's in the right place in the
             * list.
             */
            fNoZorder = FALSE;
            if (!TestWF(pwnd, WFTOGGLETOPMOST) && ValidateZorder(pcvr)) {
                fNoZorder = TRUE;
                pcvr->pos.flags |= SWP_NOZORDER;
            }
        }

        /*
         * If no change is occuring, or if a parent is invisible,
         * we won't be redrawing.
         */
        if (!(pcvr->pos.flags & SWP_NOREDRAW)) {
            if ((pcvr->pos.flags & SWP_CHANGEMASK) == SWP_NOCHANGE ||
                    !_FChildVisible(pwnd)) {
                pcvr->pos.flags |= SWP_NOREDRAW;
            }
        }

        /*
         * BACKWARD COMPATIBILITY HACK
         *
         * In 3.0, if a window was moving but not sizing, we'd send the
         * WM_NCCALCSIZE message anyhow.  Lotus Notes 2.1 depends on this
         * in order to move its "navigation bar" when the main window moves.
         */
        if (!(pcvr->pos.flags & SWP_NOMOVE) &&
            !TestWF(pwnd, WFWIN31COMPAT) &&
            (GetAppCompatFlags(NULL) & GACF_NCCALCSIZEONMOVE)) {

            fForceNCCalcSize = TRUE;
        }

        /*
         * If the window rect is sizing, or if the frame has changed,
         * send the WM_NCCALCSIZE message and deal with valid areas.
         */
        if (((pcvr->pos.flags & (SWP_NOSIZE | SWP_FRAMECHANGED)) != SWP_NOSIZE) ||
            fForceNCCalcSize) {

            WINDOWPOS pos;

            /*
             * check for full screen main app window
             */
            if (!TestWF(pwnd, WFCHILD) && !TestWF(pwnd, WEFTOOLWINDOW)) {
                xxxCheckFullScreen(pwnd, (PSIZERECT)&pcvr->pos.x);
            }

            /*
             * Set up NCCALCSIZE message parameters (in parent coords)
             * wParam = fClientOnly = TRUE
             * lParam = &params
             */
            pos = pcvr->pos;     // Make a local stack copy
            params.lppos = &pos;

            /*
             * params.rgrc[0] = rcWindowNew = New window rectangle
             * params.rgrc[1] = rcWindowOld = Old window rectangle
             * params.rgrc[2] = rcClientOld = Old client rectangle
             */
            #define rcWindowNew params.rgrc[0]
            #define rcWindowOld params.rgrc[1]
            #define rcClientOld params.rgrc[2]

            /*
             * Set up rcWindowNew in parent relative coordinates
             */
            rcWindowNew.left   = pcvr->pos.x;
            rcWindowNew.right  = rcWindowNew.left + pcvr->pos.cx;
            rcWindowNew.top    = pcvr->pos.y;
            rcWindowNew.bottom = rcWindowNew.top + pcvr->pos.cy;

            /*
             * Set up rcWindowOld in parent relative coordinates
             */
            GetRect(pwnd, &rcWindowOld, GRECT_WINDOW | GRECT_PARENTCOORDS);

            /*
             * Set up rcClientOld in parent relative coordinates
             */
            GetRect(pwnd, &rcClientOld, GRECT_CLIENT | GRECT_PARENTCOORDS);

            /*
             * Keep around a copy of the old client position
             */
            xClientOld  = rcClientOld.left;
            cxClientOld = rcClientOld.right - rcClientOld.left;
            yClientOld  = rcClientOld.top;
            cyClientOld = rcClientOld.bottom - rcClientOld.top;

            cmd = (UINT)xxxSendMessage(pwnd, WM_NCCALCSIZE, TRUE, (LPARAM)&params);

            if (!IsStillWindowC(pcvr->pos.hwndInsertAfter)) {
                ThreadUnlock(&tlpwnd);
                ThreadUnlock(&tlpwndParent);
                return FALSE;
            }

            /*
             * Upon return from NCCALCSIZE:
             *
             * params.rgrc[0] = rcClientNew = New client rect
             * params.rgrc[1] = rcValidDst  = Destination valid rectangle
             * params.rgrc[2] = rcValidSrc  = Source valid rectangle
             */
            #undef rcWindowNew
            #undef rcWindowOld
            #undef rcClientOld

            #define rcClientNew params.rgrc[0]
            #define rcValidDst  params.rgrc[1]
            #define rcValidSrc  params.rgrc[2]

            /*
             * Calculate the distance the window contents are
             * moving.  If 0 or an invalid value was returned
             * from the WM_NCCALCSIZE message, assume the
             * entire client area is valid and top-left aligned.
             */
            if (cmd < WVR_MINVALID || cmd > WVR_MAXVALID) {

                /*
                 * We don't need to copy rcValidSrc to rcClientOld,
                 * because it's already stored in rgrc[2].
                 *
                 * rcValidSrc = rcClientOld
                 */
                rcValidDst = rcClientNew;

                cmd = WVR_ALIGNTOP | WVR_ALIGNLEFT;
            }

            /*
             * Calculate the distance we'll be shifting bits...
             */
#ifdef USE_MIRRORING
            if (TestWF(pwnd, WEFLAYOUTRTL)) {
                pcvr->dxBlt = rcValidDst.right - rcValidSrc.right;
            } else
#endif
            {
                pcvr->dxBlt = rcValidDst.left - rcValidSrc.left;
            }
            pcvr->dyBlt = rcValidDst.top - rcValidSrc.top;

            /*
             * Calculate new client rect size and position
             */
            pcvr->xClientNew = rcClientNew.left;
            pcvr->yClientNew = rcClientNew.top;

            pcvr->cxClientNew = rcClientNew.right - rcClientNew.left;
            pcvr->cyClientNew = rcClientNew.bottom - rcClientNew.top;

            /*
             * Figure out whether the client rectangle is moving or sizing,
             * and diddle the appropriate bit if not.
             */
            if (xClientOld != rcClientNew.left || yClientOld != rcClientNew.top)
                pcvr->pos.flags &= ~SWP_NOCLIENTMOVE;

            if (cxClientOld != pcvr->cxClientNew || cyClientOld != pcvr->cyClientNew) {
                pcvr->pos.flags &= ~SWP_NOCLIENTSIZE;
            }

            /*
             * If the caller doesn't want us to save any bits, then don't.
             */
            if (pcvr->pos.flags & SWP_NOCOPYBITS) {
AllInvalid:

                /*
                 * The entire window is invalid: Set the blt rectangle
                 * to empty, to ensure nothing gets bltted.
                 */
                SetRectEmpty(&pcvr->rcBlt);
                ThreadUnlock(&tlpwnd);
                continue;
            }

            /*
             * If this is a transparent window, be sure to invalidate
             * everything, because only some of the window's bits are
             * blittable.
             */
            if (TestWF(pwnd, WEFTRANSPARENT))
                goto AllInvalid;

            /*
             * We never want to try to copy screen bits for a redirected or a
             * layered window. For child windows we may be able to do the
             * copy in our redirection bitmap, but that would add a bunch of
             * work in zzzBltValidBits. We should consider that for later.
             */
            if (GetLayeredWindow(pwnd) != NULL)
                goto AllInvalid;

            /*
             * If both client and window did not change size, the frame didn't
             * change, and the blt rectangle moved the same distance as the
             * rectangle, then the entire window area is valid.
             */
            if (((pcvr->pos.flags &
                    (SWP_NOSIZE | SWP_NOCLIENTSIZE | SWP_FRAMECHANGED))
                    == (SWP_NOSIZE | SWP_NOCLIENTSIZE)) &&
                    pcvr->dxBlt == (pcvr->pos.x - xWindowOld) &&
                    pcvr->dyBlt == (pcvr->pos.y - yWindowOld)) {

                goto AllValid;
            }

            /*
             * Now compute the valid blt rectangle.
             *
             * Check for horz or vert client size changes
             *
             * NOTE: Assumes WVR_REDRAW == WVR_HREDRAW | WVR_VREDRAW
             */
            if (cxClientOld != pcvr->cxClientNew) {

                if ((cmd & WVR_HREDRAW) || TestCF(pwnd, CFHREDRAW))
                    goto AllInvalid;
            }

            if (cyClientOld != pcvr->cyClientNew) {

                if ((cmd & WVR_VREDRAW) || TestCF(pwnd, CFVREDRAW))
                    goto AllInvalid;
            }

            cxSrc = rcValidSrc.right - rcValidSrc.left;
            cySrc = rcValidSrc.bottom - rcValidSrc.top;

            cxDst = rcValidDst.right - rcValidDst.left;
            cyDst = rcValidDst.bottom - rcValidDst.top;

#ifdef USE_MIRRORING
            if ((!!(cmd & WVR_ALIGNRIGHT)) ^ (!!TestWF(pwnd, WEFLAYOUTRTL)))
                rcValidDst.left += ((TestWF(pwnd, WEFLAYOUTRTL) && (cxSrc > cxDst)) ? (cxSrc-cxDst) : (cxDst - cxSrc));
#else
            if (cmd & WVR_ALIGNRIGHT)
                rcValidDst.left += (cxDst - cxSrc);
#endif

            if (cmd & WVR_ALIGNBOTTOM)
                rcValidDst.top += (cyDst - cySrc);

            /*
             * Superimpose the source on the destination, and intersect
             * the rectangles.  This is done by looking at the
             * extent of the rectangles, and pinning as appropriate.
             */

            if (cxSrc < cxDst)
                rcValidDst.right = rcValidDst.left + cxSrc;

            if (cySrc < cyDst)
                rcValidDst.bottom = rcValidDst.top + cySrc;

            /*
             * Finally map the blt rectangle to screen coordinates.
             */
            pcvr->rcBlt = rcValidDst;
            if (pwndParent != PWNDDESKTOP(pwnd)) {

                OffsetRect(
                        &pcvr->rcBlt,
                        pwndParent->rcClient.left,
                        pwndParent->rcClient.top);
            }
        } else {       // if !SWP_NOSIZE or SWP_FRAMECHANGED

AllValid:

            /*
             * No client size change: Blt the entire window,
             * including the frame.  Offset everything by
             * the distance the window rect changed.
             */
            if (pcvr->pos.flags & SWP_NOCOPYBITS) {
                SetRectEmpty(&pcvr->rcBlt);
            } else {
                pcvr->rcBlt.left   = pcvr->pos.x;
                pcvr->rcBlt.top    = pcvr->pos.y;

                if (pwndParent != PWNDDESKTOP(pwnd)) {
                    pcvr->rcBlt.left += pwndParent->rcClient.left;
                    pcvr->rcBlt.top += pwndParent->rcClient.top;
                }

                pcvr->rcBlt.right  = pcvr->rcBlt.left + pcvr->pos.cx;
                pcvr->rcBlt.bottom = pcvr->rcBlt.top + pcvr->pos.cy;
            }

            /*
             * Offset everything by the distance the window moved.
             */
#ifdef USE_MIRRORING
            if (TestWF(pwnd, WEFLAYOUTRTL)) {
                pcvr->dxBlt = (pcvr->pos.x + pcvr->pos.cx) - (xWindowOld + cxWindowOld);
            } else
#endif
            {
                pcvr->dxBlt = pcvr->pos.x - xWindowOld;
            }

            pcvr->dyBlt = pcvr->pos.y - yWindowOld;

            /*
             * If we're moving, we need to set up the client.
             */
            if (!(pcvr->pos.flags & SWP_NOMOVE)) {
                pcvr->pos.flags &= ~SWP_NOCLIENTMOVE;

                pcvr->xClientNew = pwnd->rcClient.left + pcvr->dxBlt;
                pcvr->yClientNew = pwnd->rcClient.top + pcvr->dyBlt;
                if (pwndParent != PWNDDESKTOP(pwnd)) {
                    pcvr->xClientNew -= pwndParent->rcClient.left;
                    pcvr->yClientNew -= pwndParent->rcClient.top;
                }

                pcvr->cxClientNew = pwnd->rcClient.right - pwnd->rcClient.left;
                pcvr->cyClientNew = pwnd->rcClient.bottom - pwnd->rcClient.top;
            }
        }

        ThreadUnlock(&tlpwnd);

    }   // for (... pcvr ...)

    ThreadUnlock(&tlpwndParent);
    *phwndNewActive = hwndNewActive;

    return TRUE;
}

/***************************************************************************\
* GetLastTopMostWindow
*
* Returns the last topmost window in the window list.  Returns NULL if no
* topmost windows.  Used so that we can fill in the pwndInsertAfter field
* in various SWP calls.
*
* History:
* 11-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

PWND GetLastTopMostWindow(VOID)
{
    PWND     pwndT;
    PDESKTOP pdesk = PtiCurrent()->rpdesk;

    if (pdesk == NULL)
        return NULL;

    pwndT = pdesk->pDeskInfo->spwnd->spwndChild;

    if (!pwndT || !TestWF(pwndT, WEFTOPMOST))
        return NULL;

    while (pwndT->spwndNext) {

        if (!TestWF(pwndT->spwndNext, WEFTOPMOST))
            break;

        pwndT = pwndT->spwndNext;
    }

    return pwndT;
}

/***************************************************************************\
* SetWindowPos (API)
*
*
* History:
* 11-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

BOOL xxxSetWindowPos(
    PWND pwnd,
    PWND pwndInsertAfter,
    int  x,
    int  y,
    int  cx,
    int  cy,
    UINT flags)
{
    PSMWP psmwp;
    BOOL  fInval = FALSE;

#if DBG
    CheckLock(pwnd);

    switch((ULONG_PTR)pwndInsertAfter) {
    case 0x0000FFFF:
    case (ULONG_PTR)HWND_TOPMOST:
    case (ULONG_PTR)HWND_NOTOPMOST:
    case (ULONG_PTR)HWND_TOP:
    case (ULONG_PTR)HWND_BOTTOM:
        break;

    default:
        CheckLock(pwndInsertAfter);
        break;
    }
#endif

    /*
     * BACKWARD COMPATIBILITY HACKS
     *
     * Hack 1: For Win 3.0 and below, SetWindowPos() must ignore the
     * move and size flags if SWP_SHOWWINDOW or SWP_HIDEWINDOW
     * is specified.  KnowledgePro is one application that depends on
     * this behavior for the positioning of its MDI icons.
     *
     * Hack 2: In 3.0, if SetWindowPos() is called with SWP_SHOWWINDOW
     * and the window is already visible, then the window was
     * completely invalidated anyway.  So, we do that here too.
     *
     * NOTE: The placement of the invalidation AFTER the EndDeferWindowPos()
     * call means that if the guy is Z-ordering and showing a 3.0 window,
     * it may flash, because EndDefer calls DoSyncPaint, and we invalidate
     * again after that.  Could be fixed with some major hackery in EndDefer,
     * and it's probably not worth the trouble.
     */
    if (flags & (SWP_SHOWWINDOW | SWP_HIDEWINDOW)) {

        if (!TestWF(pwnd, WFWIN31COMPAT)) {

            flags |= SWP_NOMOVE | SWP_NOSIZE;
            if ((flags & SWP_SHOWWINDOW) && TestWF(pwnd, WFVISIBLE))
                fInval = TRUE;
        }
    }

    /*
     * MULTIMONITOR HACKS
     *
     * if a app is centering or cliping a hidden owned window
     * to the primary monitor we should center the window to the owner
     *
     * this makes apps that center/position their own dialogs
     * work when the app is on a secondary monitor.
     */
    if (    !TestWF(pwnd, WFWIN50COMPAT) &&
            gpDispInfo->cMonitors > 1 &&
            !(flags & SWP_NOMOVE) &&
            !TestWF(pwnd, WFCHILD) &&
            !TestWF(pwnd, WFVISIBLE) &&
            (TestWF(pwnd, WFBORDERMASK) == LOBYTE(WFCAPTION)) &&
            pwnd->spwndOwner &&
            TestWF(pwnd->spwndOwner, WFVISIBLE) &&
            !IsRectEmpty(&pwnd->spwndOwner->rcWindow)) {

        FixBogusSWP(pwnd, &x, &y, cx, cy, flags);

    }

    if (!(psmwp = InternalBeginDeferWindowPos(1)) ||
        !(psmwp = _DeferWindowPos(psmwp,
                                  pwnd,
                                  pwndInsertAfter,
                                  x,
                                  y,
                                  cx,
                                  cy,
                                  flags))) {

        return FALSE;
    }


    if (xxxEndDeferWindowPosEx(psmwp, flags & SWP_ASYNCWINDOWPOS)) {

        if (fInval) {
            xxxRedrawWindow(
                    pwnd,
                    NULL,
                    NULL,
                    RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN);
        }

        return TRUE;
    }

    return FALSE;
}

/***************************************************************************\
* xxxSwpActivate
*
*
* History:
* 11-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

BOOL xxxSwpActivate(
    PWND pwndNewActive)
{
    PTHREADINFO pti;

    CheckLock(pwndNewActive);

    if (pwndNewActive == NULL)
        return FALSE;

    pti = PtiCurrent();

    if (TestwndChild(pwndNewActive)) {

        xxxSendMessage(pwndNewActive, WM_CHILDACTIVATE, 0, 0L);

    } else if (pti->pq->spwndActive != pwndNewActive) {

        /*
         * Remember if this window wants to be active. We are either setting
         * our own window active (most likely), or setting a window of
         * another thread active on purpose. If so that means this thread is
         * controlling this window and will probably want to set itself
         * active and foreground really soon (for example, a setup
         * program doing dde to progman). Allow this thread and the target
         * thread to do forground activates.
         *
         * Let's stop doing this for NT5 in an effort to close the number
         *  of ways applications can force a foreground change. This is not
         *  quite needed anyway, because:
         * -If the current thread is already in the foreground, then it doesn't need
         *  the TIF_ALLOWFOREGROUNDACTIVATE to make a foreground change.
         * -Since FRemoveForegroundActive removes this bit, the current thread
         *  will lose it anyway during the xxxActivateWindow call.
         * -But xxxActivateWindow will set it back anyway because we're activating
         *   a window from a different queue.
         * -The destination window/thread will take the foreground
         *  as a result of the xxxActivateWindow call, hence it doesn't
         *  need the bit on (if you're in the foreground, you don't need it).
         */
         #ifdef DONTDOTHISANYMORE
         if ((pti->pq == gpqForeground) && (pti != GETPTI(pwndNewActive))) {
            /*
             * Allow foreground activate on the source and dest.
             */
            pti->TIF_flags |= TIF_ALLOWFOREGROUNDACTIVATE;
            TAGMSG1(DBGTAG_FOREGROUND, "xxxSwpActivate set TIF %#p", pti);
            GETPTI(pwndNewActive)->TIF_flags |= TIF_ALLOWFOREGROUNDACTIVATE;
            TAGMSG1(DBGTAG_FOREGROUND, "xxxSwpActivate set TIF %#p", GETPTI(pwndNewActive));
         }
         #endif

        if (!xxxActivateWindow(pwndNewActive, AW_USE))
            return FALSE;

        /*
         * HACK ALERT: We set these bits to prevent
         * the frames from redrawing themselves in
         * the later call to DoSyncPaint().
         *
         * Prevent these captions from being repainted during
         * the DoSyncPaint().  (bobgu 6/10/87)
         */
        if (pti->pq->spwndActive != NULL)
            SetWF(pti->pq->spwndActive, WFNONCPAINT);

        if (pti->pq->spwndActivePrev != NULL)
            SetWF(pti->pq->spwndActivePrev, WFNONCPAINT);

        return TRUE;    // Indicate that we diddled these bits
    }

    return FALSE;
}
/***************************************************************************\
* xxxSendChangedMsgs
*
* Send WM_WINDOWPOSCHANGED messages as needed
*
* History:
* 10-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

VOID xxxSendChangedMsgs(
    PSMWP psmwp)
{
    PWND pwnd;
    PCVR pcvr;
    int  ccvr;
    TL   tlpwnd;

    /*
     * Send all the messages that need to be sent...
     */
    for (pcvr = psmwp->acvr, ccvr = psmwp->ccvr; --ccvr >= 0; pcvr++) {

        if (pcvr->pos.hwnd == NULL)
            continue;

        /*
         * If the window's state didn't change, don't send the message.
         */
        if ((pcvr->pos.flags & SWP_CHANGEMASK) == SWP_NOCHANGE)
            continue;

        if ((pwnd = RevalidateHwnd(pcvr->pos.hwnd)) == NULL) {
            RIPMSG0(RIP_WARNING, "xxxSendChangedMsgs: window went away in middle");
            pcvr->pos.flags = SWP_NOREDRAW | SWP_NOCHANGE;
            pcvr->pos.hwnd  = NULL;
            continue;
        }

        if (!IsStillWindowC(pcvr->pos.hwndInsertAfter)) {
            pcvr->pos.hwnd = NULL;
            continue;
        }

        /*
         * Send the WM_WINDOWPOSCHANGED message...
         *
         * Make a frame copy of the WINDOWPOS, because the pcvr
         * info may get reused if SetWindowPos()
         * is called by the message handler: see the comments in
         * AllocSmwp().
         *
         * WM_SIZE, WM_MOVE and WM_SHOW messages are sent by the
         * DefWindowProc() WM_WINDOWPOSCHANGED message processing.
         *
         * Note: It's okay to destroy the window while processing this
         * message, since this is the last call made by the window manager
         * with the window handle before returning from SetWindowPos().
         * This also means we don't have to revalidate the pwnd.
         */
        ThreadLockAlways(pwnd, &tlpwnd);
        xxxSendMessage(pwnd, WM_WINDOWPOSCHANGED, 0, (LPARAM)&pcvr->pos);

        /*
         * Only send a shape change when moving/sizing/minimizing/restoring/
         * maximizing (or framechange for NetMeeting to detect SetWindowRgn)
         */
        if (FWINABLE() &&
            (!(pcvr->pos.flags & SWP_NOCLIENTMOVE) ||
             !(pcvr->pos.flags & SWP_NOCLIENTSIZE) ||
              (pcvr->pos.flags & SWP_STATECHANGE) ||
              (pcvr->pos.flags & SWP_FRAMECHANGED))) {
            xxxWindowEvent(EVENT_OBJECT_LOCATIONCHANGE, pwnd, OBJID_WINDOW, INDEXID_CONTAINER, WEF_USEPWNDTHREAD);
        }
        ThreadUnlock(&tlpwnd);
    }   // for (... pcvr ...)


}

/***************************************************************************\
* AsyncWindowPos
*
* This functions pulls from the passed-in SMWP all windows not owned by the
* current thread and passes them off to their owners to be handled.  This
* eliminates synchronization where thread B won't get a chance to paint
* until thread A has completed painting (or at least returned from handling
* painting-related messages).  Such synchronizations are bad because they
* can cause threads of unrelated process to hang each other.
*
* History:
* 09-10-91 darrinm      Created.
\***************************************************************************/

void AsyncWindowPos(
    PSMWP psmwp)
{
    BOOL        fFinished;
    PCVR        pcvrFirst;
    PCVR        pcvr;
    PCVR        pcvrT;
    int         ccvrRemaining;
    int         ccvr;
    int         chwnd;
    PTHREADINFO ptiT;
    PTHREADINFO ptiCurrent;
    PSMWP       psmwpNew;

    pcvrFirst = psmwp->acvr;
    ccvrRemaining = psmwp->ccvr;

    ptiCurrent = PtiCurrent();

    while (TRUE) {

        fFinished = TRUE;

        /*
         * Loop through all windows in the SMWP list searching for windows
         * owned by other threads.  Return if none are found.
         */
        for (; ccvrRemaining != 0; pcvrFirst++, ccvrRemaining--) {

            if (pcvrFirst->pos.hwnd == NULL)
                continue;

            ptiT = pcvrFirst->pti;
            if (ptiT->pq != ptiCurrent->pq) {
                fFinished = FALSE;
                break;
            }
        }

        if (fFinished) {
            return;
        }

        /*
         * We've found a window of another thread.  Count how many other
         * windows in the list are owned by the same thread so we can
         * allocate a CVR array for them.
         */
        chwnd = 0;

        for (pcvr = pcvrFirst, ccvr = ccvrRemaining; --ccvr >= 0; pcvr++) {

            if (pcvr->pos.hwnd == NULL)
                continue;

            if (pcvr->pti->pq == ptiT->pq)
                chwnd++;
        }

        /*
         * Allocate temp SMWP/CVR structure to be passed to the other thread.
         */
        psmwpNew = (PSMWP)UserAllocPool(sizeof(SMWP) + (sizeof(CVR) * chwnd),
                                        TAG_SWP);

        /*
         * Even if we can't allocate memory to pass the SMWP to another
         * thread we still need to remove its windows from the current list.
         */
        if (psmwpNew == NULL) {

            for (pcvr = pcvrFirst; chwnd != 0; pcvr++) {

                if (pcvr->pti->pq == ptiT->pq) {
                    pcvr->pos.hwnd = NULL;
                    chwnd--;
                }
            }

            continue;
        }

        psmwpNew->ccvr = chwnd;
        psmwpNew->acvr = (PCVR)((PBYTE)psmwpNew + sizeof(SMWP));

        for (pcvr = pcvrFirst, pcvrT = psmwpNew->acvr; chwnd != 0; pcvr++) {

            if (pcvr->pos.hwnd == NULL)
                continue;

            /*
             * Copy the appropriate CVR structs into our temp array.
             */
            if (pcvr->pti->pq == ptiT->pq) {

                *pcvrT++ = *pcvr;
                chwnd--;

                /*
                 * Remove this window from the list of windows to be handled
                 * by the current thread.
                 */
                pcvr->pos.hwnd = NULL;
            }
        }

        /*
         * This lets the other thread know it needs to do some windowposing.
         * The other thread is responsible for freeing the temp SMWP/CVR array.
         */
        if (!PostEventMessage(ptiT, ptiT->pq, QEVENT_SETWINDOWPOS, NULL, 0,
                (WPARAM)psmwpNew, (LPARAM)ptiT)) {
            // IANJA RIP only to catch what was previously a bug: psmwpNew not freed
            RIPMSG1(RIP_WARNING, "PostEventMessage swp to pti %#p failed", ptiT);
            UserFreePool(psmwpNew);
        }
    }

}

/***************************************************************************\
* xxxProcessSetWindowPosEvent
*
* This function is called from xxxProcessEvent (QUEUE.C) to respond to
* posted QEVENT_SETWINDOWPOS events which originate at the AsyncWindowPos
* function above.
*
* History:
* 10-Sep-1991 DarrinM   Created.
\***************************************************************************/

VOID xxxProcessSetWindowPosEvent(
    PSMWP psmwpT)
{
    PSMWP psmwp;

    /*
     * Create a bonafide SMWP/CVR array that xxxEndDeferWindowPos can use
     * and later free.
     */
    if ((psmwp = InternalBeginDeferWindowPos(psmwpT->ccvr)) == NULL) {
        UserFreePool(psmwpT);
        return;
    }

    /*
     * Copy the contents of the temp SMWP/CVR array into the real one.
     */
    RtlCopyMemory(psmwp->acvr, psmwpT->acvr, sizeof(CVR) * psmwpT->ccvr);
    psmwp->ccvr = psmwpT->ccvr;

    /*
     * Complete the MultWindowPos operation now that we're on the correct
     * context.
     */
    xxxEndDeferWindowPosEx(psmwp, FALSE);

    /*
     * Free the temp SMWP/CVR array.
     */
    UserFreePool(psmwpT);
}

#define SWP_BOZO ( SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOREDRAW | SWP_NOACTIVATE )

/***************************************************************************\
* DBGValidateSibblingZOrder
*
* History:
* 04/01/98 GerardoB Created
\***************************************************************************/
#if DBG
void DBGValidateSibblingZOrder(PWND pwndParent)
{
    PWND pwndT = pwndParent->spwndChild;
    /*
     * Check that the sibbling list looks OK right now
     * We don't really care about the z-order of message windows.
     */
    if ((pwndT != NULL) && (pwndParent != PWNDMESSAGE(pwndParent))) {
        BOOL fFoundNonTopMost = !TestWF(pwndT, WEFTOPMOST);
        while (pwndT != NULL) {
            if (TestWF(pwndT, WEFTOPMOST)) {
                UserAssert(!fFoundNonTopMost);
            } else {
                fFoundNonTopMost = TRUE;
            }
            pwndT = pwndT->spwndNext;
        }
    }
}
#else
#define DBGValidateSibblingZOrder(pwndParent)
#endif

/***************************************************************************\
* zzzChangeStates
*
* History:
* 10-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

VOID zzzChangeStates(
    PWND     pwndParent,
    PSMWP    psmwp)
{
    int  ccvr;
    PCVR pcvr;
    PWND pwnd;
    TL tlpwnd;
    TL tlpwndParent;
    int czorder = 0;

    BEGINATOMICCHECK();
    ThreadLockAlways(pwndParent, &tlpwndParent);

    /*
     * Check that the sibbling list looks OK right now
     *
     * Here's the reason why this DBG code is commented out:
     * Owned windows are always expected to be on top of the owner.
     * However, an app can call SetWindowPos and insert the ownee after
     * the owner. IME somehow does this too.
     * This causes us to have A to be inserted after B and later in the
     *  windowpos array, B to be inserted somewhere else. Hence, A won't be in the
     *  expected position, because B will be moved after A is inserted.
     * In other words, a window in hwndInsertAfter must not appear later
     * as a hwnd to be z-ordered. Ownees below owners cause this situation.
     */
  //  DBGValidateSibblingZOrder(pwndParent);


    /*
     * Now change the window states
     */
    for (pcvr = psmwp->acvr, ccvr = psmwp->ccvr; --ccvr >= 0; pcvr++) {

        if (pcvr->pos.hwnd == NULL)
            continue;

        UserAssert(0 == (pcvr->pos.flags & SWP_NOTIFYALL));

        pwnd = RevalidateHwnd(pcvr->pos.hwnd);

        if ((pwnd == NULL) || !IsStillWindowC(pcvr->pos.hwndInsertAfter)) {
            RIPMSG0(RIP_WARNING, "zzzChangeStates: Window went away in middle");
            pcvr->pos.flags = SWP_NOREDRAW | SWP_NOCHANGE;
            pcvr->pos.hwnd  = NULL;
        }

#if DBG
        /*
         * This can happen when we get re-entered during a callback or mulitple
         *  threads are z-ordering the same window. The tray does stuff like this.
         * We would need to keep the toggle state in the windowpos structure to
         *  have each call have its own state.
         */
        if (TestWF(pwnd, WFTOGGLETOPMOST) && (pcvr->pos.flags & SWP_NOZORDER))
            RIPMSG0(RIP_WARNING, "zzzChangeState: WFTOGGLETOPMOST should not be set");

//        UserAssert(!(TestWF(pwnd, WFTOGGLETOPMOST) && (pcvr->pos.flags & SWP_NOZORDER)));
#endif

        /*
         * Check to se if there is any state to change.  If not, just
         * continue.
         */
        if ((pcvr->pos.flags & SWP_CHANGEMASK) == SWP_NOCHANGE) {

            pcvr->pos.flags |= SWP_NOREDRAW;
            continue;
        }

        /*
         * Change the window region if needed
         *
         * Before we do anything, check to see if we're only Z-ordering.
         * If so, then check to see if we're already in the right place,
         * and if so, clear the ZORDER flag.
         *
         * We have to make this test in the state-change loop if previous
         * windows in the WINDOWPOS list were Z-ordered, since the test depends
         * on any ordering that may have happened previously.
         *
         * We don't bother to do this redundancy check if there are
         * other bits set, because the amount of time saved in that
         * case is about as much as the amount of time it takes to
         * test for redundancy.
         */
        if (((pcvr->pos.flags & SWP_CHANGEMASK) ==
             (SWP_NOCHANGE & ~SWP_NOZORDER))) {

            /*
             * If the window's Z order won't be changing, then
             * we can clear the ZORDER bit and set NOREDRAW.
             */
            if ((!TestWF(pwnd, WFTOGGLETOPMOST)) && ValidateZorder(pcvr)) {

                /*
                 * The window's already in the right place:
                 * Set SWP_NOZORDER bit, set SWP_NOREDRAW,
                 * and destroy the visrgn that we created earlier.
                 */
                pcvr->pos.flags |= SWP_NOZORDER | SWP_NOREDRAW;

                if (pcvr->hrgnVisOld) {
                    GreDeleteObject(pcvr->hrgnVisOld);
                    pcvr->hrgnVisOld = NULL;
                }
                continue;
            }
        }

        /*
         * Change the window state, as appropriate...
         */
        if ((pcvr->pos.flags &
            (SWP_NOMOVE | SWP_NOSIZE | SWP_NOCLIENTSIZE | SWP_NOCLIENTMOVE)) !=
            (SWP_NOMOVE | SWP_NOSIZE | SWP_NOCLIENTSIZE | SWP_NOCLIENTMOVE)) {

            PCARET pcaret = &PtiCurrent()->pq->caret;
            BOOL fRecreateRedirectionBitmap = FALSE;

            if (FLayeredOrRedirected(pwnd)) {
                int cx = pwnd->rcWindow.right - pwnd->rcWindow.left;
                int cy = pwnd->rcWindow.bottom - pwnd->rcWindow.top;

                if (cx != pcvr->pos.cx || cy != pcvr->pos.cy) {
                    fRecreateRedirectionBitmap = TRUE;
                }
            }

            /*
             * Set up the new window and client rectangles.
             */
            pwnd->rcWindow.left   = pcvr->pos.x;
            pwnd->rcWindow.top    = pcvr->pos.y;
            if (pwndParent != PWNDDESKTOP(pwnd)) {
                pwnd->rcWindow.left += pwndParent->rcClient.left;
                pwnd->rcWindow.top += pwndParent->rcClient.top;
            }

            pwnd->rcWindow.right  = pwnd->rcWindow.left + pcvr->pos.cx;
            pwnd->rcWindow.bottom = pwnd->rcWindow.top + pcvr->pos.cy;

            if (pwnd->rcWindow.right < pwnd->rcWindow.left) {
                RIPMSG1(RIP_WARNING, "SWP: cx changed for pwnd %#p", pwnd);
                pwnd->rcWindow.right = pwnd->rcWindow.left;
            }

            if (pwnd->rcWindow.bottom < pwnd->rcWindow.top) {
                RIPMSG1(RIP_WARNING, "SWP: cy changed for pwnd %#p", pwnd);
                pwnd->rcWindow.bottom = pwnd->rcWindow.top;
            }

            /*
             * If the client moved relative to its parent,
             * offset the caret by the amount that rcBlt moved
             * relative to the client rect.
             */
            if (pwnd == pcaret->spwnd) {

                /*
                 * Calculate the distance the contents of the client area
                 * is moving, in client-relative coordinates.
                 *
                 * Calculates dBlt + (old position - new position)
                 */
                int dx = pcvr->dxBlt + pwnd->rcClient.left - pcvr->xClientNew;
                int dy = pcvr->dyBlt + pwnd->rcClient.top - pcvr->yClientNew;

                if (pwndParent != PWNDDESKTOP(pwnd))
                {
                    dx -= pwndParent->rcClient.left;
                    dy -= pwndParent->rcClient.top;
                }

                if ((dx | dy) != 0) {
                    pcaret->x += dx;
                    pcaret->y += dy;
                }
            }

            /*
             * Set up the new client rect
             * coordinates provided.
             */
            pwnd->rcClient.left   = pcvr->xClientNew;
            pwnd->rcClient.top    = pcvr->yClientNew;
            if (pwndParent != PWNDDESKTOP(pwnd))
            {
                pwnd->rcClient.left += pwndParent->rcClient.left;
                pwnd->rcClient.top += pwndParent->rcClient.top;
            }

            pwnd->rcClient.right  = pwnd->rcClient.left + pcvr->cxClientNew;
            pwnd->rcClient.bottom = pwnd->rcClient.top + pcvr->cyClientNew;

            /*
             * If the window becomes smaller than the monitor, the system
             * allows it to be moved (see SetSysMenu) and so we must remove
             * the monitor region.
             */
            if (TestWF(pwnd, WFMAXFAKEREGIONAL) && IsSmallerThanScreen(pwnd)) {
                SelectWindowRgn(pwnd, NULL);
            }

            /*
             * If the layered window is resizing, try to resize the
             * redirection bitmap associated with it.
             */
            if (fRecreateRedirectionBitmap) {
                RecreateRedirectionBitmap(pwnd);
            }

            /*
             * Offset the absolute positions of the window's update region,
             * and the position and update regions of its children.
             */
            if ((pcvr->dxBlt | pcvr->dyBlt) != 0) {
                /*
                 * Change position of window region, if it has one
                 * and it isn't a monitor region for a maximized window
                 */
                if (    pwnd->hrgnClip > HRGN_FULL &&
                        !TestWF(pwnd, WFMAXFAKEREGIONAL)) {
                    GreOffsetRgn(pwnd->hrgnClip, pcvr->dxBlt, pcvr->dyBlt);
                }

                if (pwnd->hrgnUpdate > HRGN_FULL) {
                    GreOffsetRgn(pwnd->hrgnUpdate, pcvr->dxBlt, pcvr->dyBlt);
                }
                OffsetChildren(pwnd, pcvr->dxBlt, pcvr->dyBlt, NULL);

                /*
                 * Change the position of the sprite associated with
                 * this window.
                 */
                if (TestWF(pwnd, WEFLAYERED)) {
                    POINT ptPos = {pcvr->pos.x, pcvr->pos.y};

                    GreUpdateSprite(gpDispInfo->hDev, PtoHq(pwnd), NULL, NULL,
                            &ptPos, NULL, NULL, NULL, 0, NULL, 0, NULL);
                }
            }
        }

        /*
         * Change the Z order if the flag is set.  Revalidate
         * hwndInsertAfter to make sure that it is still valid
         */
        if (!(pcvr->pos.flags & SWP_NOZORDER)) {

            if (ValidateWindowPos(pcvr, pwndParent)) {

                UnlinkWindow(pwnd, pwndParent);

                LinkWindow(pwnd,
                           PWInsertAfter(pcvr->pos.hwndInsertAfter),
                           pwndParent);
                czorder++;

                /*
                 * HACK ALERT MERGE
                 *
                 * ValidateZOrder() depends on rational, consistent setting of the
                 * WEFTOPMOST bit in order for it to work properly.  What this means
                 * is that we can't set or clear these bits ahead of time based on
                 * where the window is moving to: instead we have to change the bit
                 * after we've moved it.  Enter the WFTOGGLETOPMOST bit: that bit
                 * is set in ZOrderByOwner() based on what the topmost bit will
                 * eventually be set to.  To maintain a consistent state, we make
                 * any changes AFTER the window has been Z-ordered.
                 */
                if (TestWF(pwnd, WFTOGGLETOPMOST)) {
                    PBYTE pb;

                    ClrWF(pwnd, WFTOGGLETOPMOST);
                    pb = ((BYTE *)&pwnd->state);
                    pb[HIBYTE(WEFTOPMOST)] ^= LOBYTE(WEFTOPMOST);
                }
            } else {
                pcvr->pos.flags |= SWP_NOZORDER;
                ClrWF(pwnd, WFTOGGLETOPMOST);
            }
        }


        /*
         * Handle SWP_HIDEWINDOW and SWP_SHOWWINDOW, by clearing or setting
         * the WS_VISIBLE bit.
         */
        UserAssert(pwndParent != NULL);
        ThreadLockAlways(pwnd, &tlpwnd);
        if (pcvr->pos.flags & SWP_SHOWWINDOW) {

            /*
             * Window is showing. If this app is still in startup mode,
             * (still starting), give the the app starting cursor 5 more
             * seconds.
             */
            if (GETPTI(pwnd)->ppi->W32PF_Flags & W32PF_APPSTARTING)
                zzzCalcStartCursorHide((PW32PROCESS)GETPTI(pwnd)->ppi, 5000);

            /*
             * Set the WS_VISIBLE bit.
             */
            SetVisible(pwnd, SV_SET);

            if (FWINABLE())
                zzzWindowEvent(EVENT_OBJECT_SHOW, pwnd, OBJID_WINDOW, INDEXID_CONTAINER, WEF_USEPWNDTHREAD);

            if (IsTrayWindow(pwnd)) {
                psmwp->bShellNotify = TRUE;
                pcvr->pos.flags |= TestWF(pwnd, WFFRAMEON) ? SWP_NOTIFYACTIVATE|SWP_NOTIFYCREATE: SWP_NOTIFYCREATE;

                /*
                 * This Chicago code is replaced by the preceding code,
                 * which exits the critical section only after the Gre
                 * lock is removed.  Fritz
                 *
                 * xxxCallHook(HSHELL_WINDOWCREATED,
                 *             (WPARAM)HWq(pwnd),
                 *             (LPARAM)0,
                 *             WH_SHELL);
                 *
                 * PostShellHookMessages(HSHELL_WINDOWCREATED, pwnd);
                 *
                 * if (TestWF(pwnd, WFFRAMEON)) {
                 *
                 * xxxSetTrayWindow(pwnd);
                 */

            } else if (TestWF(pwnd, WFFULLSCREEN)) {
                /*
                 * Wake up the tray so it can notice that there is now
                 * a fullscreen visible window.  This deals with bugs
                 * 32164, 141563, and 150217.  FritzS
                 */
                psmwp->bShellNotify = TRUE;
                pcvr->pos.flags |= SWP_NOTIFYFS;
            }

            /*
             * If we're redrawing, create an SPB for this window if
             * needed.
             */
            if (!(pcvr->pos.flags & SWP_NOREDRAW) ||
                    (pcvr->pos.flags & SWP_CREATESPB)) {

                /*
                 * ONLY create an SPB if this window happens to be
                 * on TOP of all others.  NOTE: We could optimize this by
                 * passing in the new vis rgn to CreateSpb() so that the
                 * non-visible part of the window is automatically
                 * invalid in the SPB.
                 */
                /*
                 * Make sure this window's desktop is on top !
                 */
                if (TestCF(pwnd, CFSAVEBITS) &&
                        pwnd->head.rpdesk == grpdeskRitInput) {

                    /*
                     * If this window is the topmost VISIBLE window,
                     * then we can create an SPB.
                     */
                    PWND pwndT;
                    RECT rcT;

                    for (pwndT = pwnd->spwndParent->spwndChild ;
                         pwndT;
                         pwndT = pwndT->spwndNext) {

                        if (pwndT == pwnd) {
                            CreateSpb(pwnd, FALSE, gpDispInfo->hdcScreen);
                            break;
                        }

                        if (TestWF(pwndT, WFVISIBLE)) {

                            /*
                             * Does this window intersect the SAVEBITS
                             * window at all?  If so, bail out.
                             */
                            if (IntersectRect(&rcT,
                                              &pwnd->rcWindow,
                                              &pwndT->rcWindow)) {
                                break;
                            }
                        }
                    }
                }
            }

        } else if (pcvr->pos.flags & SWP_HIDEWINDOW) {

            /*
             * for idiots like MS-Access 2.0 who SetWindowPos( SWP_BOZO
             * and blow away themselves on the shell, then lets
             * just ignore their plea to be removed from the tray
             */
            if (((pcvr->pos.flags & SWP_BOZO ) != SWP_BOZO) &&
                IsTrayWindow(pwnd)) {

                psmwp->bShellNotify = TRUE;
                pcvr->pos.flags |= SWP_NOTIFYDESTROY;

                /*
                 * This Chicago code is replaced by the preceding code,
                 * which exits the critical section only after the Gre
                 * lock is removed.  Fritz
                 *
                 * xxxCallHook(HSHELL_WINDOWDESTROYED,
                 *             (WPARAM)HWq(pwnd),
                 *             (LPARAM)0,
                 *             WH_SHELL);
                 *
                 * PostShellHookMessages(HSHELL_WINDOWDESTROYED, pwnd);
                 */
            }

            /*
             * Clear the WS_VISIBLE bit.
             */
            SetVisible(pwnd, SV_UNSET | SV_CLRFTRUEVIS);

            if (FWINABLE())
                zzzWindowEvent(EVENT_OBJECT_HIDE, pwnd, OBJID_WINDOW, INDEXID_CONTAINER, WEF_USEPWNDTHREAD);
        }

        /*
         * BACKWARD COMPATIBILITY HACK
         *
         * Under 3.0, window frames were always redrawn, even if
         * SWP_NOREDRAW was specified.  If we've gotten this far
         * and we're visible, and SWP_NOREDRAW was specified, set
         * the WFSENDNCPAINT bit.
         *
         * Apps such as ABC Flowcharter and 123W assume this.
         * Typical offending code is MoveWindow(pwnd, ..., FALSE);
         * followed by InvalidateRect(pwnd, NULL, TRUE);
         */
        if (TestWF(pwnd, WFVISIBLE)) {
            if ((pcvr->pos.flags & SWP_STATECHANGE) ||
                (!TestWF(pwnd, WFWIN31COMPAT) && (pcvr->pos.flags & SWP_NOREDRAW))) {

                SetWF(pwnd, WFSENDNCPAINT);
            }
        }

        /*
         * If this window has a clipping region set it now
         */
        if (pcvr->hrgnClip != NULL) {
            SelectWindowRgn(pwnd, pcvr->hrgnClip);
        }
        ThreadUnlock(&tlpwnd);
    }

    /*
     * Check that the sibbling list looks OK now that we're done
     */
  //  DBGValidateSibblingZOrder(pwndParent);

    if (FWINABLE() && czorder)
        zzzWindowEvent(EVENT_OBJECT_REORDER, pwndParent, OBJID_CLIENT, INDEXID_CONTAINER, 0);
    ThreadUnlock(&tlpwndParent);

    ENDATOMICCHECK();
}

/***************************************************************************\
* SwpCalcVisRgn
*
* This routine calculates a non-clipchildren visrgn for pwnd into hrgn.
*
* History:
* 10-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

BOOL SwpCalcVisRgn(
    PWND pwnd,
    HRGN hrgn)
{
    /*
     * If this window is invisible, then
     * the visrgn will be empty, so return FALSE.
     */
    if (!TestWF(pwnd, WFVISIBLE))
        return FALSE;

    /*
     * Otherwise do it the hard way...
     */
    return CalcVisRgn(&hrgn,
                      pwnd,
                      pwnd,
                      (TestWF(pwnd, WFCLIPSIBLINGS) ?
                          (DCX_CLIPSIBLINGS | DCX_WINDOW) : (DCX_WINDOW)));
}

/***************************************************************************\
* CombineOldNewVis
*
* ORs or DIFFs hrgnOldVis and hrgnNewVis, depending on crgn, and the
* RE_* bits of fsRgnEmpty.  Basically, this routine handles the optimization
* where if either region is empty, the other can be copied.  Returns FALSE
* if the result is empty.
*
* History:
* 10-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

BOOL CombineOldNewVis(
    HRGN hrgn,
    HRGN hrgnVisOld,
    HRGN hrgnVisNew,
    UINT crgn,
    UINT fsRgnEmpty)
{
    switch (fsRgnEmpty & (RE_VISOLD | RE_VISNEW)) {
    case RE_VISOLD:

        /*
         * If we're calculating old - new and old is empty, then result is
         * empty.  Otherwise, result is new.
         */
        if (crgn == RGN_DIFF)
            return FALSE;

        CopyRgn(hrgn, hrgnVisNew);
        break;

    case RE_VISNEW:

        /*
         * New is empty: result will be the old.
         */
        CopyRgn(hrgn, hrgnVisOld);
        break;

    case RE_VISNEW | RE_VISOLD:

        /*
         * Both empty: so's the result.
         */
        return FALSE;

    case 0:

        /*
         * Neither are empty: do the real combine.
         */
        switch (GreCombineRgn(hrgn, hrgnVisOld, hrgnVisNew, crgn)) {
        case NULLREGION:
        case ERROR:
            return FALSE;

        default:
            break;
        }
        break;
    }

    return TRUE;
}

/***************************************************************************\
* BltValidInit
*
*
* History:
* 10-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

int BltValidInit(
    PSMWP psmwp)
{
    int  ccvr;
    int  cIter = 0;
    PCVR pcvr;
    PWND pwnd;
    BOOL fChangeState = FALSE;

    /*
     * Before we change any window state, calculate the old visrgn
     */
    for (pcvr = psmwp->acvr, ccvr = psmwp->ccvr; --ccvr >= 0; pcvr++) {

        UINT flags = pcvr->pos.flags;

        /*
         * Make sure this is initialized to NULL; we may be sticking something
         * in it, and we want to know later if we need to free that thing.
         */
        pcvr->hrgnVisOld = NULL;

        if (pcvr->pos.hwnd == NULL)
            continue;

        pwnd = RevalidateHwnd(pcvr->pos.hwnd);

        if ((pwnd == NULL) || !IsStillWindowC(pcvr->pos.hwndInsertAfter)) {
            pcvr->pos.hwnd  = NULL;
            pcvr->pos.flags = SWP_NOREDRAW | SWP_NOCHANGE;
            continue;
        }

        /*
         * Before we change any window's state, ensure that any SPBs
         * over the window's old location are invalidated if necessary.
         * This must be done because no WM_PAINT messages will be
         * sent to anyone for the covered area if the area is obscured
         * by other windows.
         */
        if (AnySpbs() && !(flags & SWP_NOREDRAW))
            SpbCheckRect(pwnd, &pwnd->rcWindow, DCX_WINDOW);

        /*
         * Count the number of passes through the loop
         */
        cIter++;

        /*
         * Remember if any SWPs need their state changed.
         */
        if ((flags & SWP_CHANGEMASK) != SWP_NOCHANGE)
            fChangeState = TRUE;

        /*
         * If we're not redrawing, no need to calculate visrgn
         */
        if (pcvr->pos.flags & SWP_NOREDRAW)
            continue;

        if (!SYSMET(SAMEDISPLAYFORMAT))
            PreventInterMonitorBlts(pcvr);

        pcvr->fsRE       = 0;
        pcvr->hrgnVisOld = CreateEmptyRgn();

        if (pcvr->hrgnVisOld == NULL ||
            !SwpCalcVisRgn(pwnd, pcvr->hrgnVisOld)) {

            pcvr->fsRE = RE_VISOLD;
        }
    }

    return (fChangeState ? cIter : 0);
}

/***************************************************************************\
* zzzBltValidBits
*
* NOTE: Although zzzBltValidBits calls 'xxxInternalInvalidate' it does not
* specify any of the flags that will cause immediate updating.  This means
* that it does not actually leave the critsect and therefore is not an 'xxx'
* routine and doesn't have to bother with revalidation.
*
* This is the routine that blts the windows around on the screen, taking
* into account SPBs.
*
* Here is the basic algebra going on here:
*
* ASSUMES: - rcBlt is aligned to the DESTINATION
*          - offset() offsets from source to destination
*
* 1. hrgnSrc = offset(rcBlt) & hrgnVisOld
*
*    Source region is the blt rectangle aligned with the old visrgn,
*    intersected with the old visrgn.
*
* 2. hrgnDst = rcBlt & hrgnVisNew
*
*    Dest region is the blt rectangle intersected with the new visrgn.
*
* 3. ghrgnValid = offset(hrgnSrc) & hrgnDst
*
*    Valid area is the intersection of the destination with the source
*    superimposed on the destination.
*
* 3.1 ghrgnValid = ghrgnValid - hrgnInterMonitor
*
*    Subtract out any pieces that are moving across monitors.
*
* 4. ghrgnValid -= ghrgnValidSum
*
*    This step takes into account the possibility that another window's
*    valid bits were bltted on top of this windows valid bits.  So, as we
*    blt a window's bits, we accumulate where it went, and subtract it
*    from subsequent window's valid area.
*
* 5. ghrgnInvalid = (hrgnSrc | hrgnDst) - ghrgnValid
*
* 6. ghrgnInvalid += RestoreSpb(ghrgnInvalid) (sort of)
*
*    This is the wild part, because of the grungy way that the device
*    driver SaveBits() routine works.  We call RestoreSpb() with
*    a copy of ghrgnInvalid.  If the SPB valid region doesn't intersect
*    ghrgnInvalid, RestoreSpb() does nothing.  But if it DOES intersect,
*    it blts down the ENTIRE saved SPB bitmap, which may include area
*    of the old window position that IS NOT part of ghrgnValid!
*
*    To correct for this, ghrgnValid is adjusted by subtracting off
*    the ghrgnInvalid computed by RestoreSpb, if it modified it.
*
* 7. ghrgnInvalidSum |= ghrgnInvalid
*
*    We save up the sum of all the invalid areas, and invalidate the
*    whole schmear in one fell swoop at the end.
*
* 8. ghrgnValidSum |= ghrgnValid
*
*    We keep track of the valid areas so far, which are subtracted
*    in step 4.
*
* The actual steps occur in a slightly different order than above, and
* there are numerous optimizations that are taken advantage of (the
* most important having to do with hiding and showing, and complete
* SPB restoration).
*
* Returns TRUE if some drawing was done, FALSE otherwise.
*
* History:
* 10-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

BOOL zzzBltValidBits(
    PSMWP    psmwp)
{
    int        ccvr;
    int        cIter;
    PCVR       pcvr;
    PWND       pwnd;
    PWND       pwndParent;
    PWND       pwndT;
    PWINDOWPOS ppos;
    HRGN       hrgnInvalidate;
    UINT       fsRgnEmpty;
    UINT       fsSumEmpty;
    int        cwndShowing;
    BOOL       fSyncPaint = FALSE;
    BOOL       fInvalidateLayers = FALSE;
    HDC        hdcScreen = NULL;

    DeferWinEventNotify();
    GreLockDisplay(gpDispInfo->hDev);

    /*
     * Compute old visrgns and count total CVRs in list.  A side-effect of
     * BltValidInit is that revalidates all the windows in the SMWP array.
     */


    if ((cIter = BltValidInit(psmwp)) == 0) {

CleanupAndExit:

        /*
         * Go through the cvr list and free the regions that BltValidInit()
         * created.
         */
        for (pcvr = psmwp->acvr, ccvr = psmwp->ccvr; --ccvr >= 0; pcvr++) {

            if (pcvr->hrgnVisOld) {
                GreDeleteObject(pcvr->hrgnVisOld);
                pcvr->hrgnVisOld = NULL;
            }
        }

        goto UnlockAndExit;
    }

    /*
     * We left the crit sect since last time we validated the smwp. Validate
     * it again, and find the first WINDOWPOS structure with a non-NULL
     * hwnd in it.
     */
    ppos = NULL;
    for (pcvr = psmwp->acvr, ccvr = psmwp->ccvr; --ccvr >= 0; pcvr++) {

        /*
         * Revalidate window and if it's invalid, NULL it out in the WINDOWPOS
         * struct.
         */
        pwnd = RevalidateHwnd(pcvr->pos.hwnd);

        if ((pwnd == NULL)              ||
            (pwnd->spwndParent == NULL) ||
            !IsStillWindowC(pcvr->pos.hwndInsertAfter)) {

            pcvr->pos.hwnd  = NULL;
            pcvr->pos.flags = SWP_NOREDRAW | SWP_NOCHANGE;
            continue;
        }

        /*
         * Remember the first WINDOWPOS structure that has a non-NULL
         * hwnd.
         */
        if (ppos == NULL)
            ppos = &pcvr->pos;
    }

    if (ppos == NULL)
        goto CleanupAndExit;

    UserAssert(PW(ppos->hwnd));
    pwndParent = PW(ppos->hwnd)->spwndParent;
    UserAssert(pwndParent);

    /*
     * Go account for any dirty DCs at this point, to ensure that:
     *      - any drawing done before we create an SPB will not
     *        later invalidate that SPB
     *      - the SPB regions reflect the true state of the screen,
     *        so that we don't validate parts of windows that are dirty.
     *
     * We must make this call before we change any window state.
     */
    if (AnySpbs())
        SpbCheck();

    /*
     * Change the window states
     */
    zzzChangeStates(pwndParent, psmwp);

    /*
     * move window bits around
     *
     * Invalidate the DCs for the siblings of this window.
     *
     * If our parent is not clipchildren, then we don't need to
     * invalidate its DCs.  If it IS clipchildren, its client visrgn
     * will be changing, so we must invalidate it too.
     *
     * Note, because IDC_MOVEBLT is set, final completion of WNDOBJ
     * notification is delayed until GreClientRgnDone is called.
     * This final notification does not happen until after the
     * window move blts have completed.
     */
    zzzInvalidateDCCache(pwndParent,
                      IDC_MOVEBLT |
                      (TestWF(pwndParent, WFCLIPCHILDREN) ?
                          IDC_CLIENTONLY : IDC_CHILDRENONLY));

    /*
     * Now, do the bltting or whatever that is required.
     */
    fsSumEmpty = RE_VALIDSUM | RE_INVALIDSUM;
    hrgnInvalidate = ghrgnInvalidSum;

    /*
     * Init count of windows being shown with SWP_SHOWWINDOW
     * for our backward compatibility hack later.
     */
    cwndShowing = 0;

    for (pcvr = psmwp->acvr, ccvr = psmwp->ccvr; --ccvr >= 0; pcvr++) {

        /*
         * Decrement loop count.  When cIter is 0, then
         * we're on the last pass through the loop.
         */
        cIter--;

        if (pcvr->pos.hwnd == NULL)
            continue;

        /*
         * If we're not redrawing, try the next one.
         */
        if (pcvr->pos.flags & SWP_NOREDRAW)
            continue;

        /*
         * Some drawing has been done
         */
        fSyncPaint = TRUE;

        pwnd = PW(pcvr->pos.hwnd);

        fsRgnEmpty = pcvr->fsRE;

        /*
         * Sprites should not be invalidated or cause invalidation.
         */
        if (FLayeredOrRedirected(pwnd)) {
            if (_GetProp(pwnd, PROP_LAYER, TRUE) == NULL)
                goto InvalidEmpty;

            /*
             * Sizing or showing uncovers new bits for a window, so
             * do the normal invalidation in this case. When sizing makes a
             * window smaller setting fInvalidateLayers to TRUE has the side
             * effect of allowing other layered windows to be invalidated.
             * Ideally, it should only allow invlalidating just the windows
             * that resized or showed. This would be a bunch of work, but we
             * should consider it for later.
             */
            if ((pcvr->pos.flags & SWP_NOSIZE) &&
                    !(pcvr->pos.flags & SWP_SHOWWINDOW)) {
                goto InvalidEmpty;
            } else {
                fInvalidateLayers = TRUE;
            }
        }

        /*
         * Calculate the new visrgn
         */
        if (!SwpCalcVisRgn(pwnd, ghrgnVisNew))
            fsRgnEmpty |= RE_VISNEW;

        /*
         * If the window is obscured by another window with an SPB,
         * we have to ensure that that SPB gets invalidated properly
         * since the app may not be getting a WM_PAINT msg or anything
         * to invalidate the bits.
         */
        if (AnySpbs())
            SpbCheckRect(pwnd, &pwnd->rcWindow, DCX_WINDOW);

        /*
         * Calculate ghrgnValid:
         *
         * ghrgnValid = OffsetRgn(rcBlt, -dxBlt, -dyBlt) & hrgnVisOld
         * ghrgnValid = ghrgnValid - ghrgnValidSum
         * OffsetRgn(ghrgnValid, dxBlt, dyBlt);
         * ghrgnValid = ghrgnValid - hrgnUpdate
         * ghrgnValid = ghrgnValid & hrgnVisNew;
         *
         * If either the old or new visrgns are empty, there
         * can be no valid bits...
         */
        if (fsRgnEmpty & (RE_VISOLD | RE_VISNEW))
            goto ValidEmpty;

        /*
         * If the entire window is already completely invalid, blow out.
         */
        if (pwnd->hrgnUpdate == HRGN_FULL)
            goto ValidEmpty;

        /*
         * If the blt rectangle is empty, there can be no valid bits.
         */
        if ((pcvr->rcBlt.right <= pcvr->rcBlt.left) ||
            (pcvr->rcBlt.bottom <= pcvr->rcBlt.top)) {

            goto ValidEmpty;
        }

        GreSetRectRgn(ghrgnSWP1,
                      pcvr->rcBlt.left - pcvr->dxBlt,
                      pcvr->rcBlt.top - pcvr->dyBlt,
                      pcvr->rcBlt.right - pcvr->dxBlt,
                      pcvr->rcBlt.bottom - pcvr->dyBlt);

        switch (IntersectRgn(ghrgnValid, ghrgnSWP1, pcvr->hrgnVisOld)) {
        case NULLREGION:
        case ERROR:
            goto ValidEmpty;
            break;
        }

        if (!(fsSumEmpty & RE_VALIDSUM)) {
            switch (SubtractRgn(ghrgnValid, ghrgnValid, ghrgnValidSum)) {
            case NULLREGION:
            case ERROR:
                goto ValidEmpty;
                break;
            }
        }

        if ((pcvr->dxBlt | pcvr->dyBlt) != 0)
            GreOffsetRgn(ghrgnValid, pcvr->dxBlt, pcvr->dyBlt);

        /*
         * Now subtract off the update regions of ourself and any
         * non-clipchildren parents...
         */
        pwndT = pwnd;

        do {

            if (pwndT->hrgnUpdate == HRGN_FULL)
                goto ValidEmpty;

            if (pwndT->hrgnUpdate != NULL) {
                switch (SubtractRgn(ghrgnValid, ghrgnValid, pwndT->hrgnUpdate)) {
                case NULLREGION:
                case ERROR:
                    goto ValidEmpty;
                    break;
                }
            }

            pwndT = pwndT->spwndParent;

        } while (pwndT && !TestWF(pwndT, WFCLIPCHILDREN));

        /*
         * Subtract out the intermonitor blt pieces.
         */
        if (pcvr->hrgnInterMonitor != NULL) {
            switch (SubtractRgn(ghrgnValid, ghrgnValid, pcvr->hrgnInterMonitor)) {
                case NULLREGION:
                case ERROR:
                    goto ValidEmpty;
            }
        }

        switch (IntersectRgn(ghrgnValid, ghrgnValid, ghrgnVisNew)) {
        case NULLREGION:
        case ERROR:

ValidEmpty:

            fsRgnEmpty |= RE_VALID;
            break;
        }

        /*
         * Before we restore the restore bits over part of our
         * image, we need to first copy any valid bits to their
         * final destination.
         */
        if (!(fsRgnEmpty & RE_VALID) && ((pcvr->dxBlt | pcvr->dyBlt) != 0)) {

            if (hdcScreen == NULL)
                hdcScreen = gpDispInfo->hdcScreen;

            GreSelectVisRgn(hdcScreen, ghrgnValid, SVR_COPYNEW);

#ifdef _WINDOWBLT_NOTIFICATION_
/*
 * Define _WINDOWBLT_NOTIFICATION_ to turn on Window BLT notification.
 * This notification will set a special flag in the SURFOBJ passed to
 * drivers when the DrvCopyBits operation is called to move a window.
 *
 * See also:
 *      ntgdi\gre\maskblt.cxx
 */
            NtGdiBitBlt(hdcScreen,
                        pcvr->rcBlt.left,
                        pcvr->rcBlt.top,
                        pcvr->rcBlt.right - pcvr->rcBlt.left,
                        pcvr->rcBlt.bottom - pcvr->rcBlt.top,
                        hdcScreen,
                        pcvr->rcBlt.left - pcvr->dxBlt,
                        pcvr->rcBlt.top - pcvr->dyBlt,
                        SRCCOPY,
                        0,
                        GBB_WINDOWBLT);
#else
            GreBitBlt(hdcScreen,
                      pcvr->rcBlt.left,
                      pcvr->rcBlt.top,
                      pcvr->rcBlt.right - pcvr->rcBlt.left,
                      pcvr->rcBlt.bottom - pcvr->rcBlt.top,
                      hdcScreen,
                      pcvr->rcBlt.left - pcvr->dxBlt,
                      pcvr->rcBlt.top - pcvr->dyBlt,
                      SRCCOPY,
                      0);
#endif
        }

        /*
         * Now take care of any SPB bit restoration we need to do.
         *
         * Calculate the region to clip the RestoreSpb() output to:
         *
         * ghrgnInvalid = hrgnVisOld - hrgnVisNew
         */
        if (TestWF(pwnd, WFHASSPB)    &&
            !(fsRgnEmpty & RE_VISOLD) &&
            CombineOldNewVis(ghrgnInvalid, pcvr->hrgnVisOld, ghrgnVisNew, RGN_DIFF, fsRgnEmpty)) {

            UINT retRSPB;

            /*
             * Perform SPB bits restore.  We pass RestoreSpb() the region of
             * the part of the SPB that got uncovered by this window rearrangement.
             * It tries to restore as much of this area as it can from the SPB,
             * and returns the area that could not be restored from the SPB.
             *
             * The device driver's SaveBitmap() function does not clip at all
             * when it restores bits, which means that it might write bits
             * in an otherwise valid area.  This means that the invalid area
             * returned by RestoreSpb() may actually be LARGER than the original
             * hrgnSpb passed in.
             *
             * RestoreSpb() returns TRUE if some part of ghrgnInvalid needs
             * to be invalidated.
             */
            if ((retRSPB = RestoreSpb(pwnd, ghrgnInvalid, &hdcScreen)) == RSPB_NO_INVALIDATE) {

                /*
                 * If hrgnVisNew is empty, then we know the whole invalid
                 * area is empty.
                 */
                if (fsRgnEmpty & RE_VISNEW)
                    goto InvalidEmpty;

            } else if (retRSPB == RSPB_INVALIDATE_SSB) {

                /*
                 * If RestoreSpb actually invalidated some area and we already
                 * have a ghrgnValidSum then subtract the newly invalidated area
                 * Warning this region subtract is not in the Win 3.1 code but
                 * they probably did not have the problem as severe because their
                 * drivers were limited to one level of SaveScreenBits
                 */
                if (!(fsSumEmpty & RE_VALIDSUM))
                    SubtractRgn(ghrgnValidSum, ghrgnValidSum, ghrgnInvalid);
            }

            /*
             * ghrgnInvalid += hrgnVisNew
             */
            if (!(fsRgnEmpty & RE_VISNEW))
                UnionRgn(ghrgnInvalid, ghrgnInvalid, ghrgnVisNew);

            /*
             * Some of the area we calculated as valid may have gotten
             * obliterated by the SPB restore.  To ensure this isn't
             * the case, subtract off the ghrgnInvalid returned by RestoreSpb.
             */
            // LATER mikeke VALIDSUM / ghrgnValid mismatch
            if (!(fsRgnEmpty & RE_VALIDSUM)) {
                switch (SubtractRgn(ghrgnValid, ghrgnValid, ghrgnInvalid)) {
                case NULLREGION:
                case ERROR:
                    fsRgnEmpty |= RE_VALIDSUM;
                    break;
                }
            }

        } else {

            /*
             * No SPB.  Simple ghrgnInvalid calculation is:
             *
             * ghrgnInvalid = hrgnVisNew + hrgnVisOld;
             */
            if (pcvr->hrgnVisOld == NULL) {

                /*
                 * If we couldn't create hrgnVisOld, then
                 * invalidate the entire parent
                 */
                SetRectRgnIndirect(ghrgnInvalid, &pwndParent->rcWindow);
            } else {

                if (!CombineOldNewVis(ghrgnInvalid,
                                      pcvr->hrgnVisOld,
                                      ghrgnVisNew,
                                      RGN_OR,
                                      fsRgnEmpty)) {

                    goto InvalidEmpty;
                }
            }
        }

        /*
         * Update ghrgnValidSum
         *
         * ghrgnValidSum += ghrgnValid
         */
        if (!(fsRgnEmpty & RE_VALID)) {

            /*
             * If the sum region is empty, then COPY instead of OR
             */
            if (fsSumEmpty & RE_VALIDSUM)
                CopyRgn(ghrgnValidSum, ghrgnValid);
            else
                UnionRgn(ghrgnValidSum, ghrgnValid, ghrgnValidSum);
            fsSumEmpty &= ~RE_VALIDSUM;
        }

        /*
         * Subtract ghrgnValidSum from ghrgnInvalid if non-empty,
         * otherwise use ghrgnValid.  Note, ghrgnValid has been OR'ed
         * into ghrgnValidSum already.
         */
        if (!(fsSumEmpty & RE_VALIDSUM) || !(fsRgnEmpty & RE_VALID)) {
            switch (SubtractRgn(ghrgnInvalid, ghrgnInvalid,
                    !(fsSumEmpty & RE_VALIDSUM) ? ghrgnValidSum : ghrgnValid)) {
            case NULLREGION:
            case ERROR:
InvalidEmpty:
                fsRgnEmpty |= RE_INVALID;
                break;
            }
        }

        /*
         * If there are any SPB bits left over, it wasn't just created
         * (SWP_SHOWWINDOW), and an operation occured that invalidates
         * the spb bits, get rid of the spb.  A move, size, hide, or
         * zorder operation will invalidate the bits.  Note that we do this
         * outside of the SWP_NOREDRAW case in case the guy set that flag
         * when he had some SPB bits lying around.
         */
        if (TestWF(pwnd, WFHASSPB) && !(pcvr->pos.flags & SWP_SHOWWINDOW) &&
                (pcvr->pos.flags &
                (SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_HIDEWINDOW))
                != (SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER)) {

            FreeSpb(FindSpb(pwnd));
        }

        /*
         * Finally, free up hrgnVisOld.
         */
        if (pcvr->hrgnVisOld) {
            GreDeleteObject(pcvr->hrgnVisOld);
            pcvr->hrgnVisOld = NULL;
        }

        /*
         * BACKWARD COMPATIBILITY HACK
         *
         * In 3.0, a ShowWindow() NEVER invalidated any of the children.
         * It would invalidate the parent and the window being shown, but
         * no others.
         *
         * We only apply hack (a) to 3.0 apps when all the windows involved
         * are doing a SWP_SHOWWINDOW: if any aren't, then we have to make
         * sure the siblings are invalidated too.  So, we count the windows
         * doing a SHOWWINDOW and compare it to the total count in the CVR.
         */
        if (!TestWF(pwnd, WFWIN31COMPAT) && (pcvr->pos.flags & SWP_SHOWWINDOW))
            cwndShowing++;

        /*
         * Update ghrgnInvalidSum:
         *
         * ghrgnInvalidSum += ghrgnInvalid
         */
        if (!(fsRgnEmpty & RE_INVALID)) {

            /*
             * BACKWARD COMPATIBILITY HACK
             *
             * In many cases including ShowWindow, CS_V/HREDRAW,
             * SWP_NOCOPYBITS, etc, 3.0 always invalidated the window with
             * (HRGN)1, regardless of how it was clipped by children, siblings,
             * or parents.  Besides being more efficient, this caused child
             * windows that would otherwise not get update regions to get
             * invalidated -- see the hack notes in InternalInvalidate2.
             *
             * This is a performance hack (usually) because (HRGN)1 can save
             * a lot of region calculations in the normal case.  So, we do this
             * for 3.1 apps as well as 3.0 apps.
             *
             * We detect the case as follows: invalid area not empty,
             * valid area empty, and new visrgn not empty.
             */
            if ((fsRgnEmpty & RE_VALID) && !(fsRgnEmpty & RE_VISNEW)) {

                /*
                 * With the parameters we use InternalInvalidate() does
                 * not leave the critical section
                 */
                BEGINATOMICCHECK();
                xxxInternalInvalidate(pwnd,
                                     HRGN_FULL,
                                     RDW_INVALIDATE |
                                     RDW_FRAME      |
                                     RDW_ERASE      |
                                     RDW_ALLCHILDREN);
                ENDATOMICCHECK();
            }

            /*
             * If the sum region is empty, then COPY instead of OR
             */
            if (fsSumEmpty & RE_INVALIDSUM) {

                /*
                 * HACK ALERT:
                 * If this is the last pass through the loop (cIter == 0)
                 * and ghrgnInvalidSum is currently empty,
                 * then instead of copying ghrgnInvalid to ghrgnInvalidSum,
                 * just set hrgnInvalidate to ghrgnInvalid.  This saves
                 * a region copy in the single-window case.
                 */
                if (cIter == 0) {
                    hrgnInvalidate = ghrgnInvalid;
                } else {
                    CopyRgn(ghrgnInvalidSum, ghrgnInvalid);
                }

            } else {

                UnionRgn(ghrgnInvalidSum, ghrgnInvalid, ghrgnInvalidSum);
            }

            fsSumEmpty &= ~RE_INVALIDSUM;
        }
    } // for (... pcvr ...)

    /*
     * Now, invalidate as needed.
     */
    if (!(fsSumEmpty & RE_INVALIDSUM)) {

        /*
         * BACKWARD COMPATIBILITY HACK (see above)
         *
         * If all the windows involved were being shown, then
         * invalidate the parent ONLY -- don't enumerate any children.
         * (the windows involved have already been invalidated above).
         * This hack is only applied to 3.0 apps (see above).
         */

        /*
         * More hack-o-rama. On Win3.1, the desktop paint would only
         * repaint those portions inside the rect returned from GetClipBox().
         * Dialogs with spbs outside the rect returned from GetClipBox() would
         * not get their spbs invalidated until later, when you clicked on
         * them to make them active. The only dialog that wouldn't really loose
         * its bits is the control panel desktop dialog, which would restore
         * its bad bits when it went away (in certain configurations). On
         * NT, the desktop would repaint and then the dialog would go away.
         * On Win3.1, the dialog would go away and then the desktop would
         * repaint. On NT, because of preemption and little differences in
         * painting order between applications, there was an opportunity to
         * put bad bits on the screen, on Win3.1 there wasn't.
         *
         * Now... the below code that passes RDW_NOCHILDREN only gets executed
         * if the app is marked as a win3.0 app (latest CorelDraw, also wep
         * freecell demonstrates the same problem). This code would get
         * executed when a dialog got shown. So for a win3.0 app, spb would get
         * saved, the dialog would get shown, the desktop invalidated, the
         * desktop would paint, the spb would get clobbered. In short, when
         * a win3.0 app would put up a dialog, all spbs would get freed because
         * of the new (and correct) way the desktop repaints.
         *
         * So the desktop check hack will counter-act the invalidate
         * RDW_NOCHILDREN case if all windows are hiding / showing and the
         * desktop is being invalidated. Note that in the no RDW_NOCHILDREN
         * case, the invalid area gets distributed to the children first (in
         * this case, children of the desktop), so if the children cover the
         * desktop, the desktop won't get any invalid region, which is what
         * we want. - scottlu
         */

        /*
         * With the parameters we use InternalInvalidate() does not leave
         * the critical section
         */

        DWORD dwFlags = RDW_INVALIDATE | RDW_ERASE;
        if (cwndShowing == psmwp->ccvr &&
                pwndParent != PWNDDESKTOP(pwndParent)) {
            dwFlags |= RDW_NOCHILDREN;
        } else {
            dwFlags |= RDW_ALLCHILDREN;
        }
        if (fInvalidateLayers) {
            dwFlags |= RDW_INVALIDATELAYERS;
        }

        BEGINATOMICCHECK();
        xxxInternalInvalidate(pwndParent, hrgnInvalidate, dwFlags);
        ENDATOMICCHECK();
    }

    /*
     * Since zzzInvalidateDCCache was called with IDC_MOVEBLT specified,
     * we must complete the WNDOBJ notification with a call to
     * GreClientRgnDone.
     *
     * Note: in zzzInvalidateDCCache, it is necessary to call
     * GreClientRgnUpdated even if gcountPWO is 0.  However,
     * GreClientRgnDone only does something if gcountPWO is non-zero,
     * so we can optimize slightly.
     */
    if (gcountPWO != 0) {
        GreClientRgnDone(GCR_WNDOBJEXISTS);
    }

UnlockAndExit:

    /*
     * If necessary, release the screen DC
     */
    if (hdcScreen != NULL) {

        /*
         * Reset the visrgn before we go...
         */
        GreSelectVisRgn(hdcScreen, NULL, SVR_DELETEOLD);

        /*
         * Make sure that the drawing we did in this DC does not affect
         * any SPBs.  Clear the dirty rect.
         */
        GreGetBounds(hdcScreen, NULL, 0);     // NULL means reset
    }

    /*
     * All the dirty work is done.  Ok to leave the critsects we entered
     * earlier and dispatch any deferred Window Event notifications.
     */
    GreUnlockDisplay(gpDispInfo->hDev);
    zzzEndDeferWinEventNotify();

    return fSyncPaint;
}

/***************************************************************************\
* xxxHandleWindowPosChanged
*
* DefWindowProc() HandleWindowPosChanged handler.
*
* History:
* 11-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

VOID xxxHandleWindowPosChanged(
    PWND pwnd,
    PWINDOWPOS ppos)
{
    CheckLock(pwnd);

    if (!(ppos->flags & SWP_NOCLIENTMOVE)) {
        POINT   pt;
        PWND    pwndParent;

        pt.x = pwnd->rcClient.left;
        pt.y = pwnd->rcClient.top;

        pwndParent = pwnd->spwndParent;
        UserAssert(pwndParent);

        if (pwndParent != PWNDDESKTOP(pwnd)) {
            pt.x -= pwndParent->rcClient.left;
            pt.y -= pwndParent->rcClient.top;
        }

        xxxSendMessage(
                pwnd,
                WM_MOVE,
                FALSE,
                MAKELONG(pt.x, pt.y));
    }

    if ((ppos->flags & SWP_STATECHANGE) || !(ppos->flags & SWP_NOCLIENTSIZE)) {

        if (TestWF(pwnd, WFMINIMIZED))
            xxxSendSizeMessage(pwnd, SIZEICONIC);
        else if (TestWF(pwnd, WFMAXIMIZED))
            xxxSendSizeMessage(pwnd, SIZEFULLSCREEN);
        else
            xxxSendSizeMessage(pwnd, SIZENORMAL);
    }
}

/***************************************************************************\
* PWND GetRealOwner(pwnd)
*
* Returns the owner of pwnd, normalized so that it shares the same parent
* of pwnd.
*
* History:
* 04-Mar-1992 MikeKe    From win31
\***************************************************************************/

PWND GetRealOwner(
    PWND pwnd)
{
    PWND pwndParent = pwnd->spwndParent;

    /*
     * A frame window owned by itself is "unowned"
     */
    if (pwnd != pwnd->spwndOwner && (pwnd = pwnd->spwndOwner) != NULL) {

        /*
         * The NULL test is in case the owner is higher than the
         * passed in window (e.g.  your owner IS your parent)
         */
        while (pwnd != NULL && pwnd->spwndParent != pwndParent)
            pwnd = pwnd->spwndParent;
    }

    return pwnd;
}

/***************************************************************************\
*
* Starting at pwnd (or pwndParent->spwndChild if pwnd == NULL), find
* next window owned by pwndOwner
*
* History:
* 04-Mar-1992 MikeKe    From win31
\***************************************************************************/

PWND NextOwnedWindow(
    PWND pwnd,
    PWND pwndOwner,
    PWND pwndParent)
{
    if (pwnd == NULL) {
        pwnd = pwndParent->spwndChild;
        goto loop;
    }

    while ((pwnd = pwnd->spwndNext) != NULL) {

loop:

        /*
         * If owner of pwnd is pwndOwner, break out of here...
         */
        if (pwndOwner == GetRealOwner(pwnd))
            break;
    }

    return pwnd;
}

/***************************************************************************\
*
* Recursively enumerate owned windows starting from pwndRoot,
* to set the state of WEFTOPMOST.  Doesn't actually diddle
* this bit yet: the work gets done in zzzChangeStates():
* instead we just set the WFTOGGLETOPMOST bit as appropriate.
*
* We can't diddle the state until the Z order changes are done,
* or else GetTopMostWindow() and the like will do the wrong thing.
*
* History:
* 04-Mar-1992 MikeKe    From win31
\***************************************************************************/

VOID SetTopmost(
    PWND pwndRoot,
    BOOL fTopmost)
{
    PWND pwnd;

    /*
     * If the new state is different than the current state,
     * then set the TOGGLETOPMOST bit, so it'll get toggled
     * in ChangeStates().
     */
    UserAssert((fTopmost == TRUE) || (fTopmost == FALSE));
    if (!!TestWF(pwndRoot, WEFTOPMOST) ^ fTopmost) {
        SetWF(pwndRoot, WFTOGGLETOPMOST);
    } else {
        ClrWF(pwndRoot, WFTOGGLETOPMOST);
    }

    pwnd = NULL;
    while (pwnd = NextOwnedWindow(pwnd, pwndRoot, pwndRoot->spwndParent)) {
                  SetTopmost(pwnd, fTopmost);
    }

}

/*
 * LATER: (hiroyama) #88810
 * The IME code here broke the US regression test, so backing it up until we
 * hit the problem on NT.
 */
#ifdef LATER
/***************************************************************************\
 * IsBottomIMEWindow()
 *
 * returns TRUE if pwnd is IME window and its toplevel window is BOTTOMMOST
 *
 * Ported: 18-Apr-1997 Hiroyama     from Memphis
\***************************************************************************/
BOOL IsBottomIMEWindow(
    PWND pwnd)
{
    if (TestCF(pwnd, CFIME) ||
            (pwnd->pcls->atomClassName == gpsi->atomSysClass[ICLS_IME])) {
        PWND pwndT2 = pwnd;
        PWND pwndTopOwner = pwnd;
        PWND pwndDesktop;

        if (grpdeskRitInput == NULL || grpdeskRitInput->pDeskInfo == NULL) {
            // Desktop is being created or not yet created
            RIPMSG1(RIP_WARNING, "IsBottomIMEWindow: Desktop is being created or not yet created. pwnd=%#p\n",
                    pwnd);
            return FALSE;
        }

        pwndDesktop = grpdeskRitInput->pDeskInfo->spwnd;

        UserAssert(pwndDesktop);

        /*
         * search the toplevel owner window of the IME window.
         */
        while (pwndT2 && (pwndT2 != pwndDesktop)) {
            pwndTopOwner = pwndT2;
            pwndT2 = pwndT2->spwndOwner;
        }
        /*
         * TRUE if the toplevel owner window of the IME window is BOTTOMMOST
         */
        return (BOOL)(TestWF(pwndTopOwner, WFBOTTOMMOST));
    }
    return FALSE;
}

/***************************************************************************\
 * ImeCheckBottomIMEWindow()
 *
 * returns TRUE if pwndT->spwndNext's owner is BOTTOMMOST
 *
 * Ported: 18-Apr-1997 Hiroyama     from Memphis
\***************************************************************************/
BOOL ImeCheckBottomIMEWindow(
    PWND pwndT)
{
    /*
     * pwnd is IME window and its toplevel window is BOTTOMMOST
     */
    PWND pwndDesktop;
    PWND pwndT2 = pwndT->spwndNext;
    PWND pwndTopOwner = pwndT2;

    UserAssert(grpdeskRipInput != NULL && grpdeskRitInput->pDeskInfo != NULL);
    pwndDesktop = grpdeskRitInput->pDeskInfo->spwnd;

    /*
     * check if toplevel owner window of pwnd->spwndNext is bottommost
     */
    while (pwndT2 && (pwndT2 != pwndDesktop)) {
        pwndTopOwner = pwndT2;
        pwndT2 = pwndT2->spwndOwner;
    }

    if (pwndTopOwner && TestWF(pwndTopOwner, WFBOTTOMMOST)) {
        /*
         * yes, pwndT is the last one whose toplevel window is *not* BOTTOMMOST
         */
        return TRUE;
    }

    return FALSE;
}
#endif  // LATER

/***************************************************************************\
* CalcForegroundInsertAfter
*
* Calculates where to zorder a window that doesn't belong to the foreground
* thread and is not topmost but wants to come to the top. This routine
* calculates what "top" means under those conditions.
*
* 14-Sep-1992 ScottLu       Created.
\***************************************************************************/

PWND CalcForegroundInsertAfter(
    PWND pwnd)
{
    PWND        pwndInsertAfter, pwndInsertAfterSave;
    PWND        pwndT;
    PTHREADINFO ptiTop;
#ifdef LATER    // see #88810
    BOOLEAN     fImeOwnerIsBottom = FALSE;
#endif

    /*
     * If we're allowing this application to make this top
     * level window foreground active, then this app may
     * not be foreground yet, but we want any windows it
     * zorders to appear on top because it is probably about
     * to activate them (this is a guess!) If this is the case,
     * let it do what it wants. A good example of this is an
     * application like toolbook that creates a window without a
     * caption, doesn't activate it, and wants that to appear on top.
     */

    if (TestWF(pwnd, WFBOTTOMMOST)) {
        pwndInsertAfter = GetLastNonBottomMostWindow(pwnd, TRUE);
    } else {
        pwndInsertAfter = GetLastTopMostWindow();
#ifdef LATER    // see #88810
        if (IS_IME_ENABLED()) {
            fImeOwnerIsBottom = IsBottomIMEWindow(pwnd);
            if (fImeOwnerIsBottom) {
                for (pwndT = pwndInsertAfter; pwndT; pwndT = pwndT->spwndNext) {
                    if (ImeCheckBottomIMEWindow(pwndT)) {
                        /*
                         * toplevel owner of pwndT->spwndNext is BOTTOMMOST
                         */
                        break;
                    }
                    pwndInsertAfter = pwndT;
                }
            }
        }
#endif  // LATER
    }


    if (!TestwndChild(pwnd)) {
//    if (hwnd->hwndParent == hwndDesktop)  -- Chicago conditional FritzS

        if ((GETPTI(pwnd)->TIF_flags & TIF_ALLOWFOREGROUNDACTIVATE) ||
                (GETPTI(pwnd)->ppi->W32PF_Flags & W32PF_ALLOWFOREGROUNDACTIVATE)) {

            return pwndInsertAfter;
        }
    }

    /*
     * If there is no foreground thread or this pwnd is of the foreground
     * thread, then let it come to the top.
     */
    if (gpqForeground == NULL)
        return pwndInsertAfter;

    if (GETPTI(pwnd)->pq == gpqForeground)
        return pwndInsertAfter;

    /*
     * This thread is not of the foreground queue, so search for a window
     * of this thread to zorder above.
     */
    pwndT = ((pwndInsertAfter == NULL) ?
            pwnd->spwndParent->spwndChild :
            pwndInsertAfter);

    /*
     * Remember the top insert after in case this first loop
     *  fails to find a window
     */
    pwndInsertAfterSave = pwndInsertAfter;

    for (; pwndT != NULL; pwndT = pwndT->spwndNext) {

        /*
         * This window wants to come to the top if possible.
         * If we're passing our own window, get out of this loop:
         * by now we already have pwndInsertAfter set up to the
         * last available window to insert after.
         */
        if ((pwndT == pwnd) || TestWF(pwndT, WFBOTTOMMOST))
            break;

        /*
         * If this window isn't owned by this thread, continue.
         */
        if (GETPTI(pwndT) != GETPTI(pwnd)) {
            pwndInsertAfter = pwndT;
            continue;
        }

        /*
         * Don't want a window zordering below one of its top most windows
         * if it isn't foreground.
         */
        if (TestWF(pwndT, WEFTOPMOST)) {
            pwndInsertAfter = pwndT;
            continue;
        }

#ifdef LATER    // see #88810
        // FE_IME
        if (fImeOwnerIsBottom && ImeCheckBottomIMEWindow(pwndT)) {
            /*
             * owner of pwndT->spwndNext is BOTTOMMOST
             * so pwndT is the last one whose owner is not bottommost.
             */
            pwndInsertAfter = pwndT;
            continue;
        }
        // end FE_IME
#endif

        /*
         * Ok to change zorder of top level windows because of
         * invisible windows laying around, but not children:
         * applications would go nuts if we did this.
         */
        if (!TestwndChild(pwndT)) {
            if (!TestWF(pwndT, WFVISIBLE)) {
                pwndInsertAfter = pwndT;
                continue;
            }
        }

        break;
    }

    /*
     * If we didn't find a window in the previous loop,
     * it means that the thread has no
     * other sibling windows, so we need to put it after the
     * foreground window (foreground thread). Search for the
     * first unowned window of the foreground app to zorder
     * after.
     */
    if ((pwndT == NULL) || TestWF(pwndT, WFBOTTOMMOST)) {
        /*
         * This is our first guess in case nothing works out.
         */
        pwndInsertAfter = pwndInsertAfterSave;

        /*
         * Start below the last topmost or from the top if no
         * topmost windows.
         */
        if ((pwndT = pwndInsertAfter) == NULL)
            pwndT = pwnd->spwndParent->spwndChild;

        /*
         * ptiTop is the pti of the active window in the foreground queue!
         */
        ptiTop = NULL;
        if (gpqForeground->spwndActive != NULL)
            ptiTop = GETPTI(gpqForeground->spwndActive);

        for (; pwndT != NULL; pwndT = pwndT->spwndNext) {

            if (TestWF(pwndT, WFBOTTOMMOST))
                break;

            /*
             * If not the top most thread, continue.
             */
            if (GETPTI(pwndT) != ptiTop)
                continue;

            /*
             * Found one of the foreground thread. Remember this
             * as the next best guess. Try to find an unowned
             * visible window, which would indicate the main
             * window of the foreground thread. If owned,
             * continue.
             */
            if (pwndT->spwndOwner != NULL) {
                pwndInsertAfter = pwndT;
                continue;
            }

            /*
             * Unowned and of the foreground thread. Is it visible?
             * If not, get out of here.
             */
            if (!TestWF(pwndT, WFVISIBLE))
                continue;
#ifdef LATER    // see #88810
            // FE_IME
            if (fImeOwnerIsBottom && ImeCheckBottomIMEWindow(pwndT)) {
                continue;
            }
            // end FE_IME
#endif

            /*
             * Best possible match so far: unowned visible window
             * of the foreground thread.
             */
            pwndInsertAfter = pwndT;
        }
    }

    UserAssert(pwnd != pwndInsertAfter);

    return pwndInsertAfter;
}

/***************************************************************************\
* GetTopMostInsertAfter
*
* We don't want any one to get in front of a hard error box, except menus,
*  screen savers, etc.
*
* Don't call it directly, use the GETTOPMOSTINSERTAFTER macro to avoid
*  the call when there is no hard error box up (gHardErrorHandler.pti == NULL).
*
* 04-25-96 GerardoB Created
\***************************************************************************/
PWND GetTopMostInsertAfter (PWND pwnd)
{
    PWND pwndT;
    PTHREADINFO ptiCurrent;
    PDESKTOP pdesk;
    WORD wfnid;

    /*
     * If you hit this assertion, you're probably not using the
     *  GETTOPMOSTINSERTAFTER macro to make this call.
     */
    UserAssert(gHardErrorHandler.pti != NULL);
    /*
     * pwnd: Menu and switch (ALT-TAB) windows can go on top.
     */
    wfnid = GETFNID(pwnd);
    if ((wfnid == FNID_MENU) || (wfnid == FNID_SWITCH)) {
        return NULL;
    }

    /*
     * pti: If this is the error handler thread, don't bother any longer.
     *      Screen saver can go on top too.
     */
    ptiCurrent = PtiCurrent();
    UserAssert(ptiCurrent != NULL);

    if (ptiCurrent == gHardErrorHandler.pti || (ptiCurrent->ppi->W32PF_Flags & W32PF_SCREENSAVER)) {
        return NULL;
    }

    /*
     * pdesk: Leave the logon desktop alone.
     *        Make sure the hard error box is on this desktop
     */
    pdesk = ptiCurrent->rpdesk;
    UserAssert(pdesk != NULL);
    UserAssert(pdesk->rpwinstaParent);
    UserAssert(pdesk->rpwinstaParent->pTerm);

    if ((pdesk == grpdeskLogon)
            || (pdesk != gHardErrorHandler.pti->rpdesk)) {
        return NULL;
    }

    /*
     * Walk the window list looking for the hard error box.
     * Start searching from the current desktop's first child.
     * Note that the harderror box migth not be created yet.
     */
    UserAssert(pdesk->pDeskInfo);
    UserAssert(pdesk->pDeskInfo->spwnd);

    for (pwndT = pdesk->pDeskInfo->spwnd->spwndChild;
            pwndT != NULL; pwndT = pwndT->spwndNext) {

        /*
         * Hard error boxes are always top most.
         */
        if (!TestWF(pwndT, WEFTOPMOST)) {
            break;
        }

        /*
         * If this window was created by the hard error handler thread,
         *   then this is it
         */
        if (gHardErrorHandler.pti == GETPTI(pwndT)) {
            return pwndT;
        }
    }

    return NULL;
}

/***************************************************************************\
*
* This routine maps the special HWND_* values of ppos->hwndInsertAfter,
* and returns whether or not the window's owner group should be labelled
* TOPMOST or not, or left alone.
*
* Here are the TOPMOST rules.  If pwndInsertAfter is:
*
* 1. HWND_BOTTOM == (HWND)1:
*
*    The group is made non-TOPMOST.
*
* 2. HWND_TOPMOST == (HWND)-1:
*
*    hwndInsertAfter is set to HWND_TOP, and the group is made TOPMOST.
*
* 3. HWND_NOTOPMOST == (HWND)-2:
*
*    Treated same as HWND_TOP, except that the TOPMOST bit is cleared.
*    and the entire group is made non-topmost.
*    Used to make a topmost window non-topmost, but still leave it at
*    the top of the non-topmost pile.
*    The group is not changed if the window is already non-topmost.
*
* 4. HWND_TOP == (HWND)NULL:
*
*    pwndInsertAfter is set to the last TOPMOST window if pwnd
*    is not itself TOPMOST.  If pwnd IS TOPMOST, then pwndInsertAfter
*    remains HWND_TOP.
*
* 5. A TOPMOST window:
*
*    If a window is being inserted among the TOPMOST windows, then
*    the group becomes topmost too, EXCEPT if it's being inserted behind
*    the bottom-most topmost window: in that case the window retains
*    its current TOPMOST bit.
*
* 6. A non-TOPMOST window:
*
*    If a window is being inserted among non-TOPMOST windows, the group is made
*    non-TOPMOST and inserted there.
*
* Whenever a group is made TOPMOST, only that window and its ownees are made
* topmost.  When a group is made NOTOPMOST, the entire window is made non-topmost.
*
* This routine must NOT set SWP_NOZORDER if the topmost state is changing:
* this would prevent the topmost bits from getting toggled in ChangeStates.
*
* History:
* 04-Mar-1992 MikeKe    From win31
\***************************************************************************/

int CheckTopmost(
    PWINDOWPOS ppos)
{
    PWND pwnd, pwndInsertAfter, pwndT;

    /*
     * BACKWARD COMPATIBILITY HACK
     *
     * If we're activating a window and Z-ordering too, we must ignore the
     * specified Z order and bring the window to the top, EXCEPT in the
     * following conditions:
     *
     * 1.  The window is already active (in which case, the activation code
     *    will not be bringing the window to the top)
     *
     * 2.  HWND_TOP or HWND_NOTOPMOST is specified.  This allows us to
     *    activate and move to topmost or nontopmost at the same time.
     *
     * NOTE: It would have been possible to modify ActivateWindow() to
     * take a flag to prevent it from ever doing the BringWindowToTop,
     * thus allowing SetWindowPos() to properly honor pwndInsertBehind
     * AND activation, but this change was considered too late in the
     * game -- there could be problems with existing 3.1 apps, such as
     * PenWin, etc.
     */
    pwnd = PW(ppos->hwnd);
    if (!(ppos->flags & SWP_NOACTIVATE) &&
            !(ppos->flags & SWP_NOZORDER) &&
             (ppos->hwndInsertAfter != HWND_TOPMOST &&
             ppos->hwndInsertAfter != HWND_NOTOPMOST) &&
             (pwnd != GETPTI(pwnd)->pq->spwndActive)) {
        ppos->hwndInsertAfter = HWND_TOP;
    }

    /*
     * If we're not Z-ordering, don't do anything.
     */
    if (ppos->flags & SWP_NOZORDER)
        return CTM_NOCHANGE;


    if (ppos->hwndInsertAfter == HWND_BOTTOM) {

        return CTM_NOTOPMOST;

    } else if (ppos->hwndInsertAfter == HWND_NOTOPMOST) {

        /*
         * If currently topmost, move to top of non-topmost list.
         * Otherwise, no change.
         *
         * Note that we don't set SWP_NOZORDER -- we still need to
         * check the TOGGLETOPMOST bits in ChangeStates()
         */
        if (TestWF(pwnd, WEFTOPMOST)) {

            pwndT = GetLastTopMostWindow();
            ppos->hwndInsertAfter = HW(pwndT);

            if (ppos->hwndInsertAfter == ppos->hwnd) {
                pwndT = _GetWindow(pwnd, GW_HWNDPREV);
                ppos->hwndInsertAfter = HW(pwndT);
            }

        } else {

            pwndT = _GetWindow(pwnd, GW_HWNDPREV);
            ppos->hwndInsertAfter = HW(pwndT);
        }

        return CTM_NOTOPMOST;

    } else if (ppos->hwndInsertAfter == HWND_TOPMOST) {
        pwndT = GETTOPMOSTINSERTAFTER(pwnd);
        if (pwndT != NULL) {
            ppos->hwndInsertAfter = HW(pwndT);
        } else {
            ppos->hwndInsertAfter = HWND_TOP;
        }

        return CTM_TOPMOST;

    } else if (ppos->hwndInsertAfter == HWND_TOP) {

        /*
         * If we're not topmost, position ourself after
         * the last topmost window.
         * Otherwise, make sure that no one gets in front
         *  of a hard error box.
         */
        if (TestWF(pwnd, WEFTOPMOST)) {
            pwndT = GETTOPMOSTINSERTAFTER(pwnd);
            if (pwndT != NULL) {
                ppos->hwndInsertAfter = HW(pwndT);
            }
            return CTM_NOCHANGE;
        }

        /*
         * Calculate the window to zorder after for this window, taking
         * into account foreground status.
         */
        pwndInsertAfter = CalcForegroundInsertAfter(pwnd);
        ppos->hwndInsertAfter = HW(pwndInsertAfter);

        return CTM_NOCHANGE;
    }

    /*
     * If we're being inserted after the last topmost window,
     * then don't change the topmost status.
     */
    pwndT = GetLastTopMostWindow();
    if (ppos->hwndInsertAfter == HW(pwndT))
        return CTM_NOCHANGE;

    /*
     * Otherwise, if we're inserting a TOPMOST among non-TOPMOST,
     * or vice versa, change the status appropriately.
     */
    if (TestWF(PW(ppos->hwndInsertAfter), WEFTOPMOST)) {

        if (!TestWF(pwnd, WEFTOPMOST)) {
            return CTM_TOPMOST;
        }

        pwndT = GETTOPMOSTINSERTAFTER(pwnd);
        if (pwndT != NULL) {
            ppos->hwndInsertAfter = HW(pwndT);
        }

    } else {

        if (TestWF(pwnd, WEFTOPMOST))
            return CTM_NOTOPMOST;
    }

    return CTM_NOCHANGE;
}

/***************************************************************************\
* IsOwnee(pwndOwnee, pwndOwner)
*
* Returns TRUE if pwndOwnee is owned by pwndOwner
*
*
* History:
* 04-Mar-1992 MikeKe    From win31
\***************************************************************************/

BOOL IsOwnee(
    PWND pwndOwnee,
    PWND pwndOwner)
{
    PWND pwnd;

    while (pwndOwnee != NULL) {

        /*
         * See if pwndOwnee is a child of pwndOwner...
         */
        for (pwnd = pwndOwnee; pwnd != NULL; pwnd = pwnd->spwndParent) {
            if (pwnd == pwndOwner)
                return TRUE;
        }

        /*
         * If the window doesn't own itself, then set the owner
         * to itself.
         */
        pwndOwnee = (pwndOwnee->spwndOwner != pwndOwnee ?
                pwndOwnee->spwndOwner : NULL);
    }

    return FALSE;
}

/***************************************************************************\
*
* History:
* 04-Mar-1992 MikeKe    From win31
\***************************************************************************/

BOOL IsBehind(
    PWND pwnd,
    PWND pwndReference)
{

    /*
     * Starting at pwnd, move down until we reach the end of the window
     * list, or until we reach pwndReference.  If we encounter pwndReference,
     * then pwnd is above pwndReference, so return FALSE.  If we get to the
     * end of the window list, pwnd is behind, so return TRUE.
     */
    if (pwndReference == (PWND)HWND_TOP)
        return TRUE;

    if (pwndReference == (PWND)HWND_BOTTOM)
        return FALSE;

    for ( ; pwnd != NULL; pwnd = pwnd->spwndNext) {
        if (pwnd == pwndReference)
            return FALSE;
    }

    return TRUE;
}

/***************************************************************************\
*
* Add pwnd to the SMWP.  pwndChange is the "real" pwnd being repositioned
* and pwndInsertAfter is the place where it's being inserted.
*
* pwndTopInsert is the window handle where the top of the owner tree should be
* inserted.  The special value of (HWND)-2 is used to indicate recursion, in
* which case newly added SWPs are added to the previous element.
*
* History:
* 04-Mar-1992 MikeKe    From win31
\***************************************************************************/

PSMWP AddSelfAndOwnees(
    PSMWP psmwp,
    PWND  pwnd,
    PWND  pwndChange,
    PWND  pwndInsertAfter,
    int   iTop)
{
    PWND pwndChgOwnee;
    PWND pwndT;
    BOOL fChgOwneeInserted;
    CVR  *pcvr;

    /*
     * The general idea here is to first add our ownees, then add ourself.
     * When we add our ownees though, we add them as appropriate based
     * on the pwndInsertAfter parameter.
     *
     * Find out if any of our ownees are on a direct path between pwndChange
     * and the root of the owner tree.  If one is, then its Z order relative
     * to its owner-siblings will be changing.  If none are, then
     * we want to add our ownees to the list in their current order.
     */
    pwndChgOwnee = pwndChange;
    while (pwndChgOwnee != NULL) {

        pwndT = GetRealOwner(pwndChgOwnee);

        if (pwnd == pwndT)
            break;

        pwndChgOwnee = pwndT;
    }

    /*
     * Now enumerate all other ownees, and insert them in their
     * current order.
     */
    fChgOwneeInserted = FALSE;
    pwndT = NULL;
    while ((pwndT = NextOwnedWindow(pwndT, pwnd, pwnd->spwndParent)) != NULL) {

        /*
         * If these siblings are to be reordered, compare the sibling's
         * current Z order with pwndInsertAfter.
         */
        if (pwndChgOwnee == NULL) {

            /*
             * No Z change for our ownees: just add them in their current order
             */
            psmwp = AddSelfAndOwnees(psmwp, pwndT, NULL, NULL, iTop);

        } else {

            /*
             * If we haven't already inserted the ChgOwnee, and the
             * enumerated owner-sibling is behind pwndInsertAfter, then
             * add ChgOwnee.
             */
            if (!fChgOwneeInserted && IsBehind(pwndT, pwndInsertAfter)) {

                psmwp = AddSelfAndOwnees(psmwp,
                                         pwndChgOwnee,
                                         pwndChange,
                                         pwndInsertAfter,
                                         iTop);

                if (psmwp == NULL)
                    return NULL;

                fChgOwneeInserted = TRUE;
            }

            if (pwndT != pwndChgOwnee) {

                /*
                 * Not the change ownee: add it in its current order.
                 */
                psmwp = AddSelfAndOwnees(psmwp, pwndT, NULL, NULL, iTop);
            }
        }

        if (psmwp == NULL)
            return NULL;
    }

    /*
     * If we never added the change ownee in the loop, add it now.
     */
    if ((pwndChgOwnee != NULL) && !fChgOwneeInserted) {

        psmwp = AddSelfAndOwnees(psmwp,
                                 pwndChgOwnee,
                                 pwndChange,
                                 pwndInsertAfter,
                                 iTop);

        if (psmwp == NULL)
            return NULL;
    }

    /*
     * Finally, add this window to the list.
     */
    psmwp = _DeferWindowPos(psmwp, pwnd, NULL,
            0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);

    if (psmwp == NULL)
        return NULL;

    /*
     * If we aren't inserting the topmost entry,
     * link this entry to the previous one.
     * The topmost entry will get set by our caller.
     */
    if (iTop != psmwp->ccvr - 1) {
        pcvr = &psmwp->acvr[psmwp->ccvr - 1];
        pcvr->pos.hwndInsertAfter = (pcvr - 1)->pos.hwnd;
    }
    return psmwp;
}

/***************************************************************************\
*
* ZOrderByOwner2 - Add the current window and all it owns to the SWP list,
* and arrange them in the new Z ordering.  Called only if the Z order of the
* window is changing.
*
* History:
* 04-Mar-1992 MikeKe    From win31
\***************************************************************************/

PSMWP ZOrderByOwner2(
    PSMWP psmwp,
    int   iTop)
{
    PWND       pwndT;
    PWND       pwndOwnerRoot;
    PWND       pwndTopInsert;
    PWINDOWPOS ppos;
    PWND       pwnd;
    PWND       pwndInsertAfter;
    BOOL       fHasOwnees;

    ppos = &psmwp->acvr[iTop].pos;

    /*
     * If inside message box processing, not Z ordering,
     * or if SWP_NOOWNERZORDER specified, all done.
     */
    // LATER 04-Mar-1992 MikeKe
    // do we have a substitue for fMessageBox
    if ((ppos->flags & SWP_NOZORDER) ||
        (ppos->flags & SWP_NOOWNERZORDER)) {

        return psmwp;
    }

    pwnd = PW(ppos->hwnd);
    pwndInsertAfter = PWInsertAfter(ppos->hwndInsertAfter);

    fHasOwnees = (NextOwnedWindow(NULL, pwnd, pwnd->spwndParent) != NULL);

    /*
     * If the window isn't owned, and it doesn't own any other window,
     * do nothing.
     */
    if (!pwnd->spwndOwner && !fHasOwnees)
        return psmwp;

    /*
     * Find the unowned window to start building the tree from.
     * This is easy: just zip upwards until we find a window with no owner.
     */
    pwndOwnerRoot = pwndT = pwnd;
    while ((pwndT = GetRealOwner(pwndT)) != NULL)
        pwndOwnerRoot = pwndT;

    /*
     * We need to calculate what pwndInsertAfter should be for
     * the first (topmost) window of the SWP list.
     *
     * If pwndInsertAfter is part of the owner tree we'll be building,
     * then we want to reorder windows within the owner group, so the
     * entire group should maintain it's relative order.
     *
     * If pwndInsertAfter is part of another owner tree, then we want
     * the whole group relative to that.
     *
     * If pwndInsertAfter is HWND_BOTTOM, then we want the whole
     * group to go to the bottom, so we position it relative to
     * the bottom most window that is not part of the tree.  We also
     * want to put pwnd on the bottom relative to its owner siblings.
     *
     * If pwndInsertAfter is HWND_TOP, then bring the whole group
     * to the top, as well as bringing pwnd to the top relative to its
     * owner siblings.
     *
     * Assume the topmost of group is same as topmost
     * (true for all cases except where rearranging subtree of group)
     */
    pwndTopInsert = pwndInsertAfter;
    if (pwndInsertAfter == (PWND)HWND_TOP) {

        /*
         * Bring the whole group to the top: nothing fancy to do.
         */

    } else if (pwndInsertAfter == (PWND)HWND_BOTTOM) {

        /*
         * Put the whole group on the bottom.  pwndTopInsert should
         * be the bottommost window unowned by pwndOwnerRoot.
         */
        for (pwndT = pwnd->spwndParent->spwndChild;
                (pwndT != NULL) && !TestWF(pwndT, WFBOTTOMMOST); pwndT = pwndT->spwndNext) {

            /*
             * If it's not owned, then this is the bottommost so far.
             */
            if (!IsOwnee(pwndT, pwndOwnerRoot))
                pwndTopInsert = pwndT;
        }

        /*
         * If there were no other windows not in our tree,
         * then there is no Z ordering change to be done.
         */
        if (pwndTopInsert == (PWND)HWND_BOTTOM)
            ppos->flags |= SWP_NOZORDER;

    } else {

        /*
         * pwndInsertAfter is a window.  Compute pwndTopInsert
         */
        if (IsOwnee(pwndInsertAfter, pwndOwnerRoot)) {

            /*
             * SPECIAL CASE: If we do not own any windows, and we're
             * being moved within our owner group in such a way that
             * we remain above our owner, then no other windows will
             * be moving with us, and we can just exit
             * without building our tree.  This can save a LOT of
             * extra work, especially with the MS apps CBT tutorials,
             * which do this kind of thing a lot.
             */
            if (!fHasOwnees) {

                /*
                 * Make sure we will still be above our owner by searching
                 * for our owner starting from pwndInsertAfter.  If we
                 * find our owner, then pwndInsertAfter is above it.
                 */
                for (pwndT = pwndInsertAfter; pwndT != NULL;
                        pwndT = pwndT->spwndNext) {

                    if (pwndT == pwnd->spwndOwner)
                        return psmwp;
                }
            }

            /*
             * Part of same group: Find out which window the topmost
             * of the group is currently inserted behind.
             */
            pwndTopInsert = (PWND)HWND_TOP;
            for (pwndT = pwnd->spwndParent->spwndChild; pwndT != NULL;
                    pwndT = pwndT->spwndNext) {

                if (IsOwnee(pwndT, pwndOwnerRoot))
                    break;

                pwndTopInsert = pwndT;
            }
        }
    }

    /*
     * Okay, now go recursively build the owned window list...
     */
    if (!(ppos->flags & SWP_NOZORDER)) {

        /*
         * First "delete" the last entry (the one we're sorting with)
         */
        psmwp->ccvr--;

        psmwp = AddSelfAndOwnees(psmwp,
                                 pwndOwnerRoot,
                                 pwnd,
                                 pwndInsertAfter,
                                 iTop);

        /*
         * Now set the place where the whole group is going.
         */
        if (psmwp != NULL)
            psmwp->acvr[iTop].pos.hwndInsertAfter = HW(pwndTopInsert);
    }

    return psmwp;
}

/***************************************************************************\
* TrackBackground
*
* Adjust zorder if we're crossing a TOPMOST boundary. Make sure that a
* non-topmost window in a background thread doesn't come in front of
* non-topmost windows in the foreground thread.
\***************************************************************************/

BOOL TrackBackground(WINDOWPOS *ppos, PWND pwndPrev, PWND pwnd)
{
    PWND pwndT;

    if (pwndPrev == NULL)
        return FALSE;

    /*
     * Is this window foreground? If so, let it go. For wow apps,
     * check to see if any thread of the process is foreground.
     */
    if (GETPTI(pwnd)->TIF_flags & TIF_16BIT) {

        if (gptiForeground == NULL)
            return FALSE;

        if (GETPTI(pwnd)->ppi == gptiForeground->ppi)
            return FALSE;

    } else {

        if (GETPTI(pwnd) == gptiForeground)
            return FALSE;
    }

    /*
     * Make sure the previous window is either staying or becoming
     * topmost. If not, continue: no top most boundary.
     */
    if (!FSwpTopmost(pwndPrev))
        return FALSE;

    /*
     * Is the current window already top-most? If so then don't
     * calculate a special insert after. If we don't check for
     * this, then pwnd's insert after may be calculated as what
     * pwnd already is, if pwnd is the last top most window. That
     * would cause window links to get corrupted.
     */
    if (TestWF(pwnd, WEFTOPMOST))
        return FALSE;

    /*
     * Doing this assign prevents this routine from being called
     * twice, since HW() is a conditional macro.
     */
    pwndT = CalcForegroundInsertAfter(pwnd);
    ppos->hwndInsertAfter = HW(pwndT);
    return TRUE;
}

/***************************************************************************\
* TrackZorder, TrackZorderHelper
*
* Set up hwndInsertAfter links to point to the previous window in the
* CVR array and partition them in TOPMOST and non-TOPMOST chains.
*
* 05/16/97      vadimg      created
\***************************************************************************/

void TrackZorderHelper(WINDOWPOS *ppos, HWND *phwnd)
{
    /*
     * phwnd (hwndTopmost or hwndRegular) have been initialized to NULL before
     * the beginning of the scan. This way the first hwndInsertAfter that
     * we process remains with the value that was previously calculated.
     */
    if (*phwnd != NULL) {

#if DBG
    if (ppos->hwndInsertAfter != *phwnd) {
        RIPMSG0(RIP_WARNING, "TrackZorder: modified hwndInsertAfter");
    }
#endif

        ppos->hwndInsertAfter = *phwnd;
    }
    *phwnd = ppos->hwnd;
}

PWND TrackZorder(WINDOWPOS* ppos, PWND pwndPrev, HWND *phwndTop, HWND *phwndReg)
{
    PWND pwnd = PW(ppos->hwnd);

    if (pwnd == NULL)
        return NULL;

    if (TrackBackground(ppos, pwndPrev, pwnd)) {
        *phwndReg = ppos->hwnd;
    } else if (FSwpTopmost(pwnd)) {
        TrackZorderHelper(ppos, phwndTop);
    } else {
        TrackZorderHelper(ppos, phwndReg);
    }

    return pwnd;
}

/***************************************************************************\
* ZOrderByOwner
*
* This routine Z-Orders windows by their owners.
*
* LATER
* This code currently assumes that all of the window handles are valid
*
* History:
* 04-Mar-1992 MikeKe      from win31
\***************************************************************************/

PSMWP ZOrderByOwner(
    PSMWP psmwp)
{
    int         i;
    PWND        pwnd;
    PWND        pwndT;
    WINDOWPOS   pos;
    PTHREADINFO ptiT;
    HRGN        hrgnClipSave;

    /*
     * Some of the windows in the SMWP list may be NULL at ths point
     * (removed because they'll be handled by their creator's thread)
     * so we've got to look for the first non-NULL window before we can
     * execute some of the tests below.  FindValidWindowPos returns NULL if
     * the list has no valid windows in it.
     */
    if (FindValidWindowPos(psmwp) == NULL)
        return psmwp;

    /*
     * For each SWP in the array, move it to the end of the array
     * and generate its entire owner tree in sorted order.
     */
    for (i = psmwp->ccvr; i-- != 0; ) {

        int       iScan;
        int       iTop;
        int       code;
        WINDOWPOS *ppos;
        HWND      hwndTopmost;
        HWND      hwndRegular;

        if (psmwp->acvr[0].pos.hwnd == NULL)
            continue;

        code = CheckTopmost(&psmwp->acvr[0].pos);

        /*
         * Make a local copy for later...
         *
         * Why don't we copy all CVR fields? This seems pretty hard to maintain.
         * Perhaps because most of them haven't been used yet....
         *
         */
        pos  = psmwp->acvr[0].pos;
        ptiT = psmwp->acvr[0].pti;
        hrgnClipSave = psmwp->acvr[0].hrgnClip;

        /*
         * Move the CVR to the end (if it isn't already)
         */
        iTop = psmwp->ccvr - 1;

        if (iTop != 0) {

            RtlCopyMemory(&psmwp->acvr[0],
                          &psmwp->acvr[1],
                          iTop * sizeof(CVR));

            psmwp->acvr[iTop].pos = pos;
            psmwp->acvr[iTop].pti = ptiT;
            psmwp->acvr[iTop].hrgnClip = hrgnClipSave;
        }

        if ((psmwp = ZOrderByOwner2(psmwp, iTop)) == NULL)
            break;

        /*
         * Deal with WEFTOPMOST bits.  If we're SETTING the TOPMOST bits,
         * we want to set them for this window and
         * all its owned windows -- the owners stay unchanged.  If we're
         * CLEARING, though, we need to enumerate ALL the windows, since
         * they all need to lose the topmost bit when one loses it.
         *
         * Note that since a status change doesn't necessarily mean that
         * the true Z order of the windows have changed, so ZOrderByOwner2
         * may not have enumerated all of the owned and owner windows.
         * So, we enumerate them separately here.
         */
        if (code != CTM_NOCHANGE) {
            PWND pwndRoot = PW(pos.hwnd);
#if DBG
            PWND pwndOriginal = pwndRoot;
#endif

            /*
             * Make sure we're z-ordering this window. Or settting topmost
             *  is bad.
             */
            UserAssert(!(pos.flags & SWP_NOZORDER));

            /*
             * If we're CLEARING the topmost, then we want to enumerate
             * the owners and ownees, so start our enumeration at the root.
             */
            if (code == CTM_NOTOPMOST) {

                while (pwnd = GetRealOwner(pwndRoot))
                    pwndRoot = pwnd;
            }

#if DBG
            if ((pos.flags & SWP_NOOWNERZORDER)
                && ((pwndOriginal != pwndRoot)
                    || (NextOwnedWindow(NULL, pwndRoot, pwndRoot->spwndParent) != NULL))) {
                /*
                 * We're not doing owner z-order but pwndOriginal has an owner and/or
                 *  owns some windows. The problem is, SetTopMost always affects the whole
                 *  owner/ownee group. So we might end up with WFTOGGLETOPMOST windows
                 *  that won't be z-ordered. It has always been like that.
                 */
                RIPMSG2(RIP_WARNING, "ZOrderByOwner: Topmost change while using SWP_NOOWNERZORDER."
                                     " pwndRoot:%p  pwndOriginal:%p",
                                     pwndRoot, pwndOriginal);
            }
#endif

            SetTopmost(pwndRoot, code == CTM_TOPMOST);
        }

        /*
         * Now scan the list forwards (from the bottom of the
         * owner tree towards the root) looking for the window
         * we were positioning originally (it may have been in
         * the middle of the owner tree somewhere).  Update the
         * window pos structure stored there with the original
         * information (though the z-order info is retained from
         * the sort).
         */
        pwnd = NULL;
        hwndTopmost = hwndRegular = NULL;
        for (iScan = iTop; iScan != psmwp->ccvr; iScan++) {

            ppos = &psmwp->acvr[iScan].pos;

            if (ppos->hwnd == pos.hwnd) {
                ppos->x      = pos.x;
                ppos->y      = pos.y;
                ppos->cx     = pos.cx;
                ppos->cy     = pos.cy;
                ppos->flags ^= ((ppos->flags ^ pos.flags) & ~SWP_NOZORDER);
                psmwp->acvr[iScan].hrgnClip = hrgnClipSave;
            }

            pwndT = pwnd;
            pwnd  = TrackZorder(ppos, pwndT, &hwndTopmost, &hwndRegular);
        }
    }

    return psmwp;
}
/***************************************************************************\
* xxxEndDeferWindowPosEx
*
*
* History:
* 11-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

BOOL xxxEndDeferWindowPosEx(
    PSMWP psmwp,
    BOOL  fAsync)
{
    PWND        pwndNewActive;
    PWND        pwndParent;
    PWND        pwndActive;
    PWND        pwndActivePrev;
    HWND        hwndNewActive;
    PWINDOWPOS  pwp;
    BOOL        fClearBits;
    BOOL        fSyncPaint;
    UINT        cVisWindowsPrev;
    PTHREADINFO ptiCurrent = PtiCurrent();
    TL          tlpwndNewActive;
    TL          tlpwndParent;
    TL          tlcuSMWP;
    BOOL        fForegroundPrev;

    UserAssert(IsWinEventNotifyDeferredOK());

    DBGCheskSMWP(psmwp);
    if (psmwp->bHandle) {
        CheckLock(psmwp);
    }

    /*
     * Validate the window pos structures and find a window to activate.
     */
    if ((psmwp->ccvr != 0) && ValidateSmwp(psmwp, &fSyncPaint)) {

        if ((pwp = FindValidWindowPos(psmwp)) == NULL)
            goto lbFinished;

        /*
         * Make sure to stop at the mother desktop window.  In Win95
         * a SetWindowPos() on a desktop window will have a NULL parent
         * window.  This is not true in NT, but our mother desktop
         * window does have a NULL rpdesk, so check it too.
         */
        UserAssert(PW(pwp->hwnd));
        pwndParent = PW(pwp->hwnd)->spwndParent;
        if (pwndParent == NULL || pwndParent->head.rpdesk == NULL)
            goto lbFinished;

        /*
         * Usually all window positioning happens synchronously across threads.
         * This is because apps expect that behavior now - if it was async,
         * callers could not expect the state to be set once the api returned.
         * This is not the semantics of SetWindowPos(). The downside of this
         * synchronicity is that a SetWindowPos() on an hwnd created by another
         * thread will cause the caller to wait for that thread - even if that
         * thread is hung. That's what you get.
         *
         * We don't want task manager to hang though, no matter who else is
         * hung, so when taskman calls, it calls a special entry point for
         * tiling / cascading, which does SetWindowPos() asynchronously -
         * by posting an event in each thread's queue that makes it set its
         * own window position - that way if the thread is hung, who cares -
         * it doesn't effect taskman.
         *
         * Do async window pos positioning before zorder by owner so that
         * we retain any cross thread ownership relationships synchronously.
         */
        if (fAsync) {
            AsyncWindowPos(psmwp);
        }

        /*
         * If needed, Z order the windows by owner.
         * This may grow the SMWP, if new CVRs are added.
         */
        if (pwndParent == PWNDDESKTOP(pwndParent)) {

            if ((psmwp = ZOrderByOwner(psmwp)) == NULL) {
                return FALSE;
            }
        }

        ThreadLockAlwaysWithPti(ptiCurrent, pwndParent, &tlpwndParent);
        ThreadLockPoolCleanup(ptiCurrent, psmwp, &tlcuSMWP, DestroySMWP);

        /*
         * Calc new window positions.
         */
        if (xxxCalcValidRects(psmwp, &hwndNewActive)) {

            int i;

            pwndNewActive = RevalidateHwnd(hwndNewActive);

            ThreadLockWithPti(ptiCurrent, pwndNewActive, &tlpwndNewActive);

            cVisWindowsPrev = ptiCurrent->cVisWindows;
            fForegroundPrev = (ptiCurrent == gptiForeground);

            /*
             * The call to zzzBltValidBits will leave the critical section
             * if there are any notifications to make.
             */
            UserAssert(IsWinEventNotifyDeferredOK());
            if (!zzzBltValidBits(psmwp))
                fSyncPaint = FALSE;
            UserAssert(IsWinEventNotifyDeferredOK());

            if (psmwp->bShellNotify) {
                for (i = psmwp->ccvr; i-- != 0; ) {
                    /*
                     * Loop through the windows, looking for notifications.
                     */

                    if (0 == (psmwp->acvr[i].pos.flags & SWP_NOTIFYALL))
                        continue;

                    if (psmwp->acvr[i].pos.flags & SWP_NOTIFYCREATE) {
                          PostShellHookMessages(HSHELL_WINDOWCREATED,
                                    (LPARAM)psmwp->acvr[i].pos.hwnd);

                          xxxCallHook(HSHELL_WINDOWCREATED,
                            (WPARAM)psmwp->acvr[i].pos.hwnd,
                            (LPARAM)0,
                            WH_SHELL);
                    }

                    if (psmwp->acvr[i].pos.flags & SWP_NOTIFYDESTROY) {
                        PostShellHookMessages(HSHELL_WINDOWDESTROYED,
                                      (LPARAM)psmwp->acvr[i].pos.hwnd);

                        xxxCallHook(HSHELL_WINDOWDESTROYED,
                            (WPARAM)psmwp->acvr[i].pos.hwnd,
                            (LPARAM)0,
                            WH_SHELL);
                    }

                    if (psmwp->acvr[i].pos.flags & SWP_NOTIFYACTIVATE) {
                        PWND pwnd = RevalidateHwnd(psmwp->acvr[i].pos.hwnd);
                        if (pwnd != NULL){
                            TL tlpwnd;
                            ThreadLockAlwaysWithPti(ptiCurrent, pwnd, &tlpwnd);
                            xxxSetTrayWindow(pwnd->head.rpdesk, pwnd, NULL);
                            ThreadUnlock(&tlpwnd);
                        }
                    }

                    if (psmwp->acvr[i].pos.flags & SWP_NOTIFYFS) {
                        xxxSetTrayWindow(ptiCurrent->rpdesk, STW_SAME, NULL);
                    }
                }
            }


            /*
             * If this process went from some windows to no windows visible
             * and it was in the foreground, then let its next activate
             * come to the foreground.
             */
            if (fForegroundPrev && cVisWindowsPrev && !ptiCurrent->cVisWindows) {

                ptiCurrent->TIF_flags |= TIF_ALLOWFOREGROUNDACTIVATE;
                TAGMSG1(DBGTAG_FOREGROUND, "xxxEndDeferWindowPosEx set TIF %#p", ptiCurrent);

                /*
                 * Also if any apps were in the middle of starting when
                 * this happened, allow them to foreground activate again.
                 */
                RestoreForegroundActivate();
            }

            /*
             * Deal with any activation...
             */
            fClearBits = FALSE;
            if (pwndNewActive != NULL)
                fClearBits = xxxSwpActivate(pwndNewActive);

            /*
             * Now draw frames and erase backgrounds of all the windows
             * involved.
             */
            UserAssert(pwndParent);
            if (fSyncPaint)
                xxxDoSyncPaint(pwndParent, DSP_ENUMCLIPPEDCHILDREN);

            ThreadUnlock(&tlpwndNewActive);

            /*
             * If SwpActivate() set the NONCPAINT bits, clear them now.
             */
            if (fClearBits) {

                if (pwndActive = ptiCurrent->pq->spwndActive)
                    ClrWF(pwndActive, WFNONCPAINT);

                if (pwndActivePrev = ptiCurrent->pq->spwndActivePrev)
                    ClrWF(pwndActivePrev, WFNONCPAINT);
            }

            /*
             * Send WM_WINDOWPOSCHANGED messages
             */
            xxxSendChangedMsgs(psmwp);
        }

        ThreadUnlockPoolCleanup(ptiCurrent, &tlcuSMWP);
        ThreadUnlock(&tlpwndParent);
    }

lbFinished:

    /*
     * All done.  Free everything up and return.
     */
    DestroySMWP(psmwp);
    return TRUE;
}


/***************************************************************************\
* IncVisWindows
* DecVisWindows
*
* These routines deal with incrementing/decrementing the visible windows
* on the thread.
*
\***************************************************************************/
#if DBG

BOOL gfVisVerify = FALSE;

void VerifycVisWindows(PWND pwnd)
{
    BOOL fShowMeTheWindows = FALSE;
    PTHREADINFO pti = GETPTI(pwnd);
    PWND pwndNext;
    UINT uVisWindows = 0;

    if (!gfVisVerify) {
        return;
    }

    /*
     * Make sure the count makes sense
     */
    if ((int)pti->cVisWindows < 0) {
        RIPMSG0(RIP_ERROR, "VerifycVisWindows: pti->cVisWindows underflow!");
        fShowMeTheWindows = TRUE;
    }

    /*
     * This window might be owned by a desktop-less service
     */
    if (pti->rpdesk == NULL || (pti->TIF_flags & TIF_SYSTEMTHREAD)) {
        return;
    }

    /*
     * Child windows don't affect cVisWindows
     */
    if (!FTopLevel(pwnd)) {
        return;
    }

ShowMeTheWindows:
    /*
     * We're going to count all the windows owned by this pti
     *  that should be included in cVisWindows
     */
    pwndNext = pti->rpdesk->pDeskInfo->spwnd;
    /*
     * If this is a top level window, start with the first child.
     * If not, it should be a desktop thread window
     */
    if (pwndNext == pwnd->spwndParent) {
        pwndNext = pwndNext->spwndChild;
    } else if (pwndNext->spwndParent != pwnd->spwndParent) {
        RIPMSG1(RIP_WARNING, "VerifycVisWindows: Non top level window:%#p", pwnd);
        return;
    }

    if (fShowMeTheWindows) {
        RIPMSG1(RIP_WARNING, "VerifycVisWindows: Start window walk at:%#p", pwndNext);
    }

    /*
     * Count the visble-not-minimized windows owned by this pti
     */
    while (pwndNext != NULL) {
        if (pti == GETPTI(pwndNext)) {
            if (fShowMeTheWindows) {
                RIPMSG1(RIP_WARNING, "VerifycVisWindows: pwndNext:%#p", pwndNext);
            }
            if (!TestWF(pwndNext, WFMINIMIZED)
                    && TestWF(pwndNext, WFVISIBLE)) {

                uVisWindows++;

                if (fShowMeTheWindows) {
                    RIPMSG1(RIP_WARNING, "VerifycVisWindows: Counted:%#p", pwndNext);
                }
            }
        }
        pwndNext = pwndNext->spwndNext;
    }

    /*
     * It must match.
     */
    if (pti->cVisWindows != uVisWindows) {
        RIPMSG2(RIP_WARNING, "VerifycVisWindows: pti->cVisWindows:%#lx. uVisWindows:%#lx",
                pti->cVisWindows, uVisWindows);

        /*
         * Disable going through the list and make the error into a warning.
         * There are many loopholes as to how the cVisWindow count may get
         * messed up. See bug 109807.
         */
        fShowMeTheWindows = TRUE;

        if (!fShowMeTheWindows) {
            fShowMeTheWindows = TRUE;
            uVisWindows = 0;
            goto ShowMeTheWindows;
        }
    }
}
#endif

/***************************************************************************\
* FVisCountable
*
* Desktops and top-level i.e. whose parent is the desktop) non-minimized
* windows should be counted in the per-thread visible window counts.
\***************************************************************************/

BOOL FVisCountable(PWND pwnd)
{
    if (!TestWF(pwnd, WFDESTROYED)) {
        if ((GETFNID(pwnd) == FNID_DESKTOP) ||
                (FTopLevel(pwnd) && !TestWF(pwnd, WFMINIMIZED))) {
            return TRUE;
        }
    }
    return FALSE;
}

/***************************************************************************\
* IncVisWindows
*
\***************************************************************************/

VOID IncVisWindows(
    PWND pwnd)
{
    if (FVisCountable(pwnd))
        GETPTI(pwnd)->cVisWindows++;

#if DBG
    if (!ISTS())
        VerifycVisWindows(pwnd);
#endif
}

/***************************************************************************\
* cDecVis
*
* An inline that allows debug code to decrement the vis window count
* without doing verification right away.  Also alled by DecVisWindows
* to do the actual work.
\***************************************************************************/

__inline void cDecVis(PWND pwnd)
{
    UserAssert(pwnd != NULL);

    if (FVisCountable(pwnd))
        GETPTI(pwnd)->cVisWindows--;
}

/***************************************************************************\
* DecVisWindows
*
\***************************************************************************/

VOID DecVisWindows(
    PWND pwnd)
{
    cDecVis(pwnd);

#if DBG
    if (!ISTS())
        VerifycVisWindows(pwnd);
#endif
}

/***************************************************************************\
* SetMiminize
*
* This routine must be used to flip the WS_MIMIMIZE style bit.
* It adjusts cVisWindows count if appropriate.
*
* 06/06/96  GerardoB Created
\***************************************************************************/

VOID SetMinimize(
    PWND pwnd,
    UINT uFlags)
{
    /*
     * Note that Dec and IncVisWindows check the WFMINIMIZED flag, so the order
     *  in which we set/clear the flag and call these functions is important
     * If the window is not WFVISIBLE, cVisWindows must not change.
     */
    if (uFlags & SMIN_SET) {
        UserAssert(!TestWF(pwnd, WFMINIMIZED));
        if (TestWF(pwnd, WFVISIBLE)) {
            /*
             * Decrement the count because the window is not minimized
             *  and visible, and we're about to mark it as minimized
             */

#if DBG
            cDecVis(pwnd);
#else
            DecVisWindows(pwnd);
#endif
        }
        SetWF(pwnd, WFMINIMIZED);

#if DBG
        VerifycVisWindows(pwnd);
#endif
    } else {

        UserAssert(TestWF(pwnd, WFMINIMIZED));
        ClrWF(pwnd, WFMINIMIZED);
        if (TestWF(pwnd, WFVISIBLE)) {
            /*
             * Increment the count because the window is visible
             *  and it's no longer marked as minimized
             */
            IncVisWindows(pwnd);
        }
    }
}
/***************************************************************************\
* SetVisible
*
* This routine must be used to set or clear the WS_VISIBLE style bit.
* It also handles the setting or clearing of the WF_TRUEVIS bit.
*
* Note that we don't check if the window is already in the (in)visible
* state before setting/clearing the WFVISIBLE bit and calling
* Inc/DecVisWindows. If the window is already in the given state and
* someone calls SetVisible to change into the same state, the VisCount
* will get out of sync. This could happen, for example, if someone
* passed two SWP_SHOWWINDOW for the same hwnd CVR's in the same
* EndDeferWindowPos call. It would be ideal to do the check here, but
* most of the time the caller does the check and we don't want to
* penalize everybody just because of the weird cases.
*
* History:
* 11-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

VOID SetVisible(
    PWND pwnd,
    UINT flags)
{
    if (flags & SV_SET) {

#if DBG
        if (TestWF(pwnd, WFINDESTROY)) {
            RIPMSG1(RIP_WARNING, "SetVisible: show INDESTROY %#p", pwnd);
        }
#endif

        if (TestWF(pwnd, WFVISIBLE)) {
            RIPMSG1(RIP_WARNING, "SetVisible: already visible %#p", pwnd);
        } else {
            SetWF(pwnd, WFVISIBLE);
            IncVisWindows(pwnd);
        }
    } else {

        if (flags & SV_CLRFTRUEVIS)
            ClrFTrueVis(pwnd);

#if DBG
        if (TestWF(pwnd, WFDESTROYED)) {
            RIPMSG1(RIP_WARNING, "SetVisible: hide DESTROYED %#p", pwnd);
        }
#endif

        if (TestWF(pwnd, WFVISIBLE)) {
            ClrWF(pwnd, WFVISIBLE);
            DecVisWindows(pwnd);
        } else {
            RIPMSG1(RIP_WARNING, "SetVisible: already hidden %#p", pwnd);
        }
    }
}

/***************************************************************************\
* IsMaxedRect
*
* Determines if a window is "maximizing" to a certain area
*
* History:
\***************************************************************************/

BOOL IsMaxedRect(
    LPRECT      lprcWithin,
    PCSIZERECT  psrcMaybe)
{
    return(psrcMaybe->x <= lprcWithin->left                      &&
           psrcMaybe->y <= lprcWithin->top                       &&
           psrcMaybe->cx >= lprcWithin->right - lprcWithin->left &&
           psrcMaybe->cy >= lprcWithin->bottom - lprcWithin->top);
}

/***************************************************************************\
* xxxCheckFullScreen
*
* Sees if a window is really fullscreen or just a maximized window in
* disguise.  If the latter, it will be forced to the proper maximized
* size.
*
* This is called from both CalcValidRects() and CreateWindowEx().
*
* History:
\***************************************************************************/

BOOL xxxCheckFullScreen(
    PWND        pwnd,
    PSIZERECT   psrc)
{
    BOOL            fYielded = FALSE;
    PMONITOR        pMonitor;
    PMONITOR        pMonitorPrimary;
    TL              tlpMonitor;
    RECT            rc;
    BOOL            fIsPrimary;


    CheckLock(pwnd);

    /*
     * SINCE THIS IS ONLY CALLED IN 2 PLACES, make the checks there
     * instead of the overhead of calling this function in time critical
     * places.
     *
     * If 3 or more places call it, put the child/toolwindow checks here
     */
    UserAssert(!TestWF(pwnd, WFCHILD));
    UserAssert(!TestWF(pwnd, WEFTOOLWINDOW));

    pMonitorPrimary = GetPrimaryMonitor();
    if (gpDispInfo->cMonitors == 1) {
        pMonitor = pMonitorPrimary;
    } else {
        /*
         * In multiple monitor mode, windows that take up the entire
         * virtual screen are not considered 'full screen'.  'Full screen'
         * means full single monitor only.  This detection is so that any
         * docked bars--tray, office'95 tools--can get out of the way for
         * the application.
         *
         * There are only three types of windows that ought to go full
         * virtual screen.  None of them need the tray et al. to get out of
         * the way:
         *      (1) Normal app windows that want a lot of space
         *          * Those guys just activate and deactivate normally.
         *      (2) Desktop windows
         *          * Shell, User desktop sit behind everything else.
         *      (3) Screen savers, demos, etc.
         *          * These guys should be WS_EX_TOPMOST to ensure they sit
         *            over everybody.
         */
        if (IsMaxedRect(&gpDispInfo->rcScreen, psrc))
            return fYielded;

        RECTFromSIZERECT(&rc, psrc);
        pMonitor = _MonitorFromRect(&rc, MONITOR_DEFAULTTOPRIMARY);
    }

    fIsPrimary = (pMonitor == pMonitorPrimary);
    ThreadLockAlways(pMonitor, &tlpMonitor);

    if (IsMaxedRect(&pMonitor->rcWork, psrc)) {
        if (TestWF(pwnd, WFMAXIMIZED)) {
            SetWF(pwnd, WFREALLYMAXIMIZABLE);

            if (gpDispInfo->cMonitors > 1) {
                /*
                 * BUGBUG: Check if app is before 4.1?
                 */

                /*
                 * This is for XL '95 going fullscreen when already maxed.  It
                 * always uses the primary display.  Let's hack them, and any
                 * other old app that tries to move its truly maximized window.
                 * They will be clipped otherwise by our fake regional stuff.
                 */
                PMONITOR pMonitorReal;

                pMonitorReal = _MonitorFromWindow(pwnd, MONITOR_DEFAULTTOPRIMARY);
                if (pMonitorReal != pMonitor && fIsPrimary) {
                    /*
                     * Transfer over the shape to the REAL monitor.
                     */
                    psrc->x += pMonitorReal->rcMonitor.left;
                    psrc->y  += pMonitorReal->rcMonitor.top;
                    psrc->cx -= (pMonitor->rcMonitor.right - pMonitor->rcMonitor.left) +
                        (pMonitorReal->rcMonitor.right - pMonitorReal->rcMonitor.left);

                    psrc->cy -= (pMonitor->rcMonitor.bottom - pMonitor->rcMonitor.top) +
                        (pMonitorReal->rcMonitor.bottom - pMonitorReal->rcMonitor.top);

                    ThreadUnlock(&tlpMonitor);
                    pMonitor = pMonitorReal;
                    fIsPrimary = FALSE;
                    ThreadLockAlways(pMonitor, &tlpMonitor);
                }
            }
        }

        if (    TestWF(pwnd, WFMAXIMIZED) &&
                TestWF(pwnd, WFMAXBOX)    &&
                (TestWF(pwnd, WFBORDERMASK) == LOBYTE(WFCAPTION))) {

            if (    psrc->y + SYSMET(CYCAPTION) <= pMonitor->rcMonitor.top &&
                    psrc->y + psrc->cy >= pMonitor->rcMonitor.bottom) {

                if (!TestWF(pwnd, WFFULLSCREEN)) {
                    /*
                     * Only want to do full screen stuff on the tray
                     * monitor.
                     */
                    fYielded = xxxAddFullScreen(pwnd, pMonitor);
                }
            } else {
                int iRight;
                int iBottom;
                int dxy;

                if (TestWF(pwnd, WFFULLSCREEN)) {
                    fYielded = xxxRemoveFullScreen(pwnd, pMonitor);
                }

                /*
                 * Despite the code in GetMinMaxInfo() to fix up
                 * the max rect, we still have to hack old apps.
                 * Word '95 & XL '95 do weird things when going to/from
                 * full screen when maximized already.
                 *
                 * NOTE:  you can have more than one docked bar on a
                 * monitor.  Win '95 code doesn't work right in that
                 * case.
                 */
                dxy = GetWindowBorders(pwnd->style, pwnd->ExStyle, TRUE, FALSE);
                dxy *= SYSMET(CXBORDER);

                psrc->x = pMonitor->rcWork.left - dxy;
                psrc->y = pMonitor->rcWork.top - dxy;

                dxy *= 2;
                iRight =
                        pMonitor->rcWork.right - pMonitor->rcWork.left + dxy;
                iBottom =
                        pMonitor->rcWork.bottom - pMonitor->rcWork.top + dxy;

                /*
                 * Let console windows maximize smaller than defaults.
                 */
                if (pwnd->pcls->atomClassName == gatomConsoleClass) {
                    psrc->cx = min(iRight, psrc->cx);
                    psrc->cy = min(iBottom, psrc->cy);
                } else {
                    psrc->cx = iRight;

                    /*
                     * B#14012 save QuickLink II that wants 4 pixels hanging off
                     * the screen for every edge except the bottom edge, which
                     * they only want to overhang by 2 pixels -- jeffbog 5/17/95
                     *
                     * BUT THIS CODE DOESN'T WORK FOR MULTIPLE MONITORS, so don't
                     * do it on secondary dudes.  Else, XL '95 flakes out.
                     */
                    if (fIsPrimary && !TestWF(pwnd, WFWIN40COMPAT)) {
                        psrc->cy = min(iBottom, psrc->cy);
                    } else {
                        psrc->cy = iBottom;
                    }
                }
            }

        } else if (IsMaxedRect(&pMonitor->rcMonitor, psrc)) {
            fYielded = xxxAddFullScreen(pwnd, pMonitor);
        }
    } else {
        if (TestWF(pwnd, WFMAXIMIZED)) {
            ClrWF(pwnd, WFREALLYMAXIMIZABLE);
        }

        fYielded = xxxRemoveFullScreen(pwnd, pMonitor);
    }

    ThreadUnlock(&tlpMonitor);
    return fYielded;
}

/***************************************************************************\
* ClrFTrueVis
*
* Called when making a window invisible.  This routine destroys any update
* regions that may exist, and clears the WF_TRUEVIS of all windows below
* the passed in window.
*
* History:
* 11-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

VOID ClrFTrueVis(
    PWND pwnd)
{
    /*
     * Destroy pwnd and its children's update regions.
     * We do this here to guarantee that a hidden window
     * and its children don't have update regions.
     *
     * This fixes bugs when destroying windows that have
     * update regions (SendDestroyMessages) among others
     * and allows us to simplify SetParent().  This was
     * deemed better than hacking DoPaint() and/or
     * DestroyWindow().
     *
     * We can stop recursing when we find a window that doesn't
     * have the visible bit set, because by definition it won't
     * have any update regions below it (this routine will have been called)
     */
    if (NEEDSPAINT(pwnd)) {

        DeleteMaybeSpecialRgn(pwnd->hrgnUpdate);

        ClrWF(pwnd, WFINTERNALPAINT);

        pwnd->hrgnUpdate = NULL;
        DecPaintCount(pwnd);
    }

    for (pwnd = pwnd->spwndChild; pwnd != NULL; pwnd = pwnd->spwndNext) {

        /*
         * pwnd->fs &= ~WF_TRUEVIS;
         */
        if (TestWF(pwnd, WFVISIBLE))
            ClrFTrueVis(pwnd);
    }
}

/***************************************************************************\
* OffsetChildren
*
* Offsets the window and client rects of all children of hwnd.
* Also deals with the children's update regions and SPB rects.
*
* History:
* 22-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

VOID OffsetChildren(
    PWND   pwnd,
    int    dx,
    int    dy,
    LPRECT prcHitTest)
{
    RECT    rc;
    PWND    pwndStop;

    if (!pwnd->spwndChild)
        return;

    pwndStop = pwnd;
    pwnd = pwndStop->spwndChild;
    for (;;) {
        /*
         * Skip windows that don't intersect prcHitTest...
         */
        if (prcHitTest && !IntersectRect(&rc, prcHitTest, &pwnd->rcWindow))
            goto NextWindow;

        pwnd->rcWindow.left   += dx;
        pwnd->rcWindow.right  += dx;
        pwnd->rcWindow.top    += dy;
        pwnd->rcWindow.bottom += dy;

        pwnd->rcClient.left   += dx;
        pwnd->rcClient.right  += dx;
        pwnd->rcClient.top    += dy;
        pwnd->rcClient.bottom += dy;

        if (pwnd->hrgnUpdate > HRGN_FULL && !TestWF(pwnd, WFMAXFAKEREGIONAL)) {
            GreOffsetRgn(pwnd->hrgnUpdate, dx, dy);
        }

        /*
         * Change position of window region, if it has one
         */
        if (pwnd->hrgnClip != NULL)
            GreOffsetRgn(pwnd->hrgnClip, dx, dy);

        if (TestWF(pwnd, WFHASSPB))
            OffsetRect(&(FindSpb(pwnd))->rc, dx, dy);

#ifdef CHILD_LAYERING
        if (TestWF(pwnd, WEFLAYERED)) {
            POINT ptPos = {pwnd->rcWindow.left, pwnd->rcWindow.top};

            GreUpdateSprite(gpDispInfo->hDev, PtoHq(pwnd), NULL, NULL,
                    &ptPos, NULL, NULL, NULL, 0, NULL, 0, NULL);
        }
#endif // CHILD_LAYERING

        /*
         * Recurse into the child tree if there are children.
         */
        if (pwnd->spwndChild) {
            pwnd = pwnd->spwndChild;
            continue;
        }

NextWindow:
        if (pwnd->spwndNext) {
            /*
             * Recurse to the next sibling in the list.
             */
            pwnd = pwnd->spwndNext;
        } else {
            for (;;) {
                /*
                 * We're at the end of the sibling window list.
                 * Go to the parent's next window.
                 */
                pwnd = pwnd->spwndParent;
                if (pwnd == pwndStop)
                    return;

                if (pwnd->spwndNext) {
                    pwnd = pwnd->spwndNext;
                    break;
                }
            }
        }
    }
}

/***************************************************************************\
* SetWindowRgn
*
* Parameters:
*     hwnd    --  Window handle
*     hrgn    --  Region to set into window. NULL can be accepted.
*     fRedraw --  TRUE to go through SetWindowPos() and calculate
*                 update regions correctly. If the window is visible
*                 this will usually be TRUE.
*
* Returns:
*     TRUE for success, FALSE for failure
*
* Comments:
*     This is a very simple routine to set a window region. It goes through
*     SetWindowPos() to get perfect update region calculation, and to deal
*     with other related issues like vis rgn change & dc invalidation,
*     display lock holding, spb invalidation, etc. Also since it sends
*     WM_WINDOWPOSCHANGING & WM_WINDOWPOSCHANGED, we'll be able to expand
*     SetWindowPos() in the future to take hrgns directly for efficient
*     window state change control (like setting the rect and region at
*     the same time, among others) without harming compatibility.
*
*     hrgn is in window rect coordinates (not client rect coordinates).
*     Once set, hrgn is owned by the system. A copy is not made!
*
* 30-Jul-1994 ScottLu   Created.
\***************************************************************************/

#define SWR_FLAGS_REDRAW   (SWP_NOCHANGE | SWP_FRAMECHANGED | SWP_NOACTIVATE)
#define SWR_FLAGS_NOREDRAW (SWP_NOCHANGE | SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOREDRAW)

BOOL xxxSetWindowRgn(
    PWND pwnd,
    HRGN hrgn,
    BOOL fRedraw)
{
    PSMWP psmwp;
    HRGN  hrgnClip = NULL;
    BOOL  bRet = FALSE;

    /*
     * Validate the region handle.  We did this for 3.51, so
     * we better do it for later versions.  Our validation will
     * make a copy of the clip-rgn and send it through to the
     * SetWIndowRgn code.  Once this is set in the kernel, we
     * will return to the client and the old region will be deleted
     * there.
     *
     * If the region passed in is NULL, then we get rid of the
     * current retion.  Map it to HRGN_FULL so that SetWindowPos()
     * can tell this is what the caller wants.
     */
    if (hrgn) {

        if ((hrgnClip = UserValidateCopyRgn(hrgn)) == NULL) {

#if DBG
            RIPMSG0(RIP_WARNING, "xxxSetWindowRgn: Failed to create region!");
#endif
            goto swrClean;
        }
#ifdef USE_MIRRORING
        MirrorRegion(pwnd, hrgnClip, FALSE);
#endif
    } else {

        hrgnClip = HRGN_FULL;
    }

    /*
     * Get a psmwp, and put the region in it, correctly offset.
     * Use SWP_FRAMECHANGED with acts really as a "empty" SetWindowPos
     * that still sends WM_WINDOWPOSCHANGING and CHANGED messages.
     * SWP_NOCHANGE ensures that we don't size, move, activate, zorder.
     */
    if (psmwp = InternalBeginDeferWindowPos(1)) {

        /*
         * psmwp gets freed automatically if this routine fails.
         */
        if (psmwp = _DeferWindowPos(
                psmwp,
                pwnd,
                PWND_TOP,
                0,
                0,
                0,
                0,
                fRedraw ? SWR_FLAGS_REDRAW : SWR_FLAGS_NOREDRAW)) {

            /*
             * Do the operation. Note that hrgn is still in window coordinates.
             * SetWindowPos() will change it to screen coordinates before
             * selecting into the window.
             */
            psmwp->acvr[0].hrgnClip = hrgnClip;
            bRet = xxxEndDeferWindowPosEx(psmwp, FALSE);
        }
    }

    /*
     * If the call failed, then delete our region we created.  A FALSE
     * return means it should've never made it to the xxxSelectWindowRgn
     * call, so everything should be as it was.
     */
    if (!bRet && (hrgnClip != HRGN_FULL)) {

swrClean:

        GreDeleteObject(hrgnClip);
    }

    return bRet;
}

/***************************************************************************\
* SelectWindowRgn
*
* This routine does the work of actually selecting in the window region.
*
* 30-Jul-1994 ScottLu   Created.
\***************************************************************************/

void SelectWindowRgn(
    PWND pwnd,
    HRGN hrgnClip)
{
    /*
     * If there is a region already there, delete it because
     * a new one is being set.  For maximized windows in multiple monitor
     * mode, we always use the monitor HRGN.  We don't make a copy.  This
     * way, when the hrgn changes because of monitor config, the window's
     * monitor region automatically gets updated.  Clever huh?  Also saves
     * memory.
     */
    if (pwnd->hrgnClip != NULL) {
        if (TestWF(pwnd, WFMAXFAKEREGIONAL)) {
            ClrWF(pwnd, WFMAXFAKEREGIONAL);
        } else {
            /*
             * Do NOT select in a monitor region if the window is normally
             * regional.  The MinMaximize code will always pass HRGN_MONITOR
             * to us no matter what.  But when we get here, bail out and
             * don't destroy the app's region if it has one.
             */
            if (hrgnClip == HRGN_MONITOR)
                return;

            GreDeleteObject(pwnd->hrgnClip);
        }

        pwnd->hrgnClip = NULL;
    }

    /*
     * NULL or HRGN_FULL means "set to NULL". If we have a real region,
     * use it. USER needs to own it, and it needs to be in screen
     * coordinates.
     */
    if (hrgnClip > HRGN_FULL) {

        if (hrgnClip == HRGN_MONITOR) {
            PMONITOR pMonitor;

            /*
             * Use the monitor region if the window is really maxed
             * on a monitor.  It's already happened by the time we get here,
             * if so.  And xxxCheckFullScreen will clear the reallymaximed
             * style for a maximized window if it doesn't cover the whole
             * max area.
             */
            UserAssert(pwnd->spwndParent == PWNDDESKTOP(pwnd));

            if (!TestWF(pwnd, WFMAXIMIZED) || !TestWF(pwnd, WFREALLYMAXIMIZABLE))
                return;

            /*
             * Do nothing for windows off screen.
             */
            pMonitor = _MonitorFromWindow(pwnd, MONITOR_DEFAULTTONULL);
            if (!pMonitor)
                return;

            hrgnClip = pMonitor->hrgnMonitor;
            SetWF(pwnd, WFMAXFAKEREGIONAL);
        } else {
            if (pwnd != PWNDDESKTOP(pwnd)) {
                GreOffsetRgn(hrgnClip, pwnd->rcWindow.left, pwnd->rcWindow.top);
            }

            GreSetRegionOwner(hrgnClip, OBJECT_OWNER_PUBLIC);
        }

        pwnd->hrgnClip = hrgnClip;
    }
}


/***************************************************************************\
* TestRectBogus
*
* Returns TRUE if the window rect [x,y,cx,cy] is centered or
* clipped to the monitor or work rect [prc], FALSE otherwise.
*
* History:
* 26-Mar-1997 adams     Created.
\***************************************************************************/

#define SLOP_X 8
#define SLOP_Y 8

BOOL
TestRectBogus(RECT * prc, int x, int y, int cx, int cy)
{
    //
    //  check for a fullscreen (or offscreen) window
    //
    if (    x  <= prc->left &&
            y  <= prc->top &&
            cx >= (prc->right  - prc->left) &&
            cy >= (prc->bottom - prc->top)) {

        // rect is fullscreen
        return FALSE;
    }

    //
    //  check for the window being centered to the work area
    //  use <= for y to catch dialogs centered "high"
    //  (like the network logon dialog)
    //
    if (    abs(x - (prc->right + prc->left - cx) / 2) <= SLOP_X &&
            abs(y - (prc->bottom + prc->top - cy) / 2) <= SLOP_Y ) {

        // rect centered
        return TRUE;
    }

    //
    //  check for the window being cliped to the work area
    //
    if (    x == prc->left ||
            y == prc->top ||
            x == (prc->right - cx) ||
            y == (prc->bottom - cy)) {

        // rect is clipped
        return TRUE;
    }

    return FALSE;
}


/***************************************************************************\
* IsRectBogus
*
* Returns TRUE if the window rect [x,y,cx,cy] is centered or
* clipped to the monitor or work rect of the primary monitor.
*
* History:
* 26-Mar-1997 adams     Created.
\***************************************************************************/

BOOL
IsRectBogus(int x, int y, int cx, int cy)
{
    PMONITOR    pMonitorPrimary = GetPrimaryMonitor();

    return TestRectBogus(&pMonitorPrimary->rcWork, x, y, cx, cy) ||
           TestRectBogus(&pMonitorPrimary->rcMonitor, x, y, cx, cy);
}



/***************************************************************************\
* FixBogusSWP
*
* Detects if a rect is being centered or clipped to the primary monitor,
* and centers it in its owner's window if so. This prevents apps that
* are not multimon aware from having their "main" window displayed on
* one monitor but their dialogs moved to the primary monitor
* because they believe the dialog is offscreen.
*
* History:
* 26-Mar-1997 adams     Created.
\***************************************************************************/

void
FixBogusSWP(PWND pwnd, int * px, int * py, int cx, int cy, UINT flags)
{
    PMONITOR pMonitor;

    pMonitor = _MonitorFromWindow(pwnd->spwndOwner, MONITOR_DEFAULTTONEAREST);

    //
    // only check for a bogus SWP if the owner is not on the primary
    //
    if (pMonitor != GetPrimaryMonitor()) {

        //
        // get the current size if SWP_NOSIZE is set
        //
        if (flags & SWP_NOSIZE) {
            cx = pwnd->rcWindow.right  - pwnd->rcWindow.left;
            cy = pwnd->rcWindow.bottom - pwnd->rcWindow.top;
        }

        //
        // see if the app is trying to center or clip the window
        //
        if (IsRectBogus(*px, *py, cx, cy))
        {
            RECT rc;

#if DBG
            int oldX = *px;
            int oldY = *py;
#endif

            //
            // the app wants to center/clip the window
            // we will have to do it for them.
            //
            // get the window rect of the parent and
            // intersect that with the work area of
            // the owning monitor, then center the
            // window to this rect.
            //
            IntersectRect(&rc, &pMonitor->rcWork, &pwnd->spwndOwner->rcWindow);

            //
            // new multimonior friendly position.
            //
            *px = rc.left + (rc.right  - rc.left - cx) / 2;
            *py = rc.top  + (rc.bottom - rc.top  - cy) / 2;

            //
            // now clip to the work area.
            //
            if (*px + cx > pMonitor->rcWork.right) {
                *px = pMonitor->rcWork.right - cx;
            }

            if (*py + cy > pMonitor->rcWork.bottom) {
                *py = pMonitor->rcWork.bottom - cy;
            }

            if (*px < pMonitor->rcWork.left) {
                *px = pMonitor->rcWork.left;
            }

            if (*py < pMonitor->rcWork.top) {
                *py = pMonitor->rcWork.top;
            }

            RIPMSG0(RIP_WARNING | RIP_THERESMORE,              "SetWindowPos detected that your app is centering or clipping");
            RIPMSG0(RIP_WARNING | RIP_THERESMORE | RIP_NONAME, "a window to the primary monitor when its owner is on a different monitor.");
            RIPMSG0(RIP_WARNING | RIP_THERESMORE | RIP_NONAME, "Consider fixing your app to use the Window Manager Multimonitor APIs.");
            RIPMSG4(RIP_WARNING | RIP_NONAME,                  "SetWindowPos moved the window from (%d,%d) to (%d,%d).\n",
                                                               oldX, oldY, *px, *py);
        }
    }
}

/***************************************************************************\
* PreventInterMonitorBlts()
*
* Prevents monitor-to-monitor blts when they are different caps.  This
* way we redraw the part of a window that moves to a different monitor.
* We try to blt as much as possible.
*
* We look at the source rect and what monitor owns it, and how much that
* monitor also contains of the destination rect.  Then we compare that
* with the destination rect and what monitor owns that, and how much it
* contains of the source rect.  The larger wins.
*
* rcBlt is in screen coordinates and is the DESTINATION.
*
* History:
* 11-11-1997    vadimg      ported from Memphis
\***************************************************************************/

void PreventInterMonitorBlts(PCVR pcvr)
{
    RECT        rcSrc;
    RECT        rcDst;
    RECT        rcSrcT;
    RECT        rcDstT;
    PMONITOR    pMonitor;
    //
    // If the destination is empty do nothing.
    //
    if (IsRectEmpty(&pcvr->rcBlt))
        return;

    //
    // Get the source rect (rcBlt is the destination, dxBlt/dyBlt are the
    // distance moved from the source).
    //
    CopyOffsetRect(&rcSrc, &pcvr->rcBlt, -pcvr->dxBlt, -pcvr->dyBlt);

    //
    // Split up the source into its monitor pieces.  If the source intersects
    // a monitor, then figure out where that part will be in the destination.
    // Intersect the destination part with the same monitor.  The result is
    // the amount we can blt from the source to the dest on that monitor.
    //
    // We do this for each monitor to find the biggest blt rect.  We want
    // the biggest because we want to repaint as little as possible.  We do
    // bail out if both the source and dest are fully contained on the same
    // monitor.
    //
    for (pMonitor = gpDispInfo->pMonitorFirst;
            pMonitor != NULL;
            pMonitor = pMonitor->pMonitorNext) {

        //
        // We're only interested in visible monitors
        //
        if (!(pMonitor->dwMONFlags & MONF_VISIBLE))
            continue;
        //
        // If this monitor doesn't contain a piece of the source, we don't
        // care about it.  We won't be doing a same monitor blt on it for sure.
        //
        if (!IntersectRect(&rcSrcT, &rcSrc, &pMonitor->rcMonitor))
            continue;

        //
        // See where this rect would be in the destination.
        //
        CopyOffsetRect(&rcDst, &rcSrcT, pcvr->dxBlt, pcvr->dyBlt);

        //
        // Intersect this rect with the same monitor rect to see what piece
        // can be safely blted on the same monitor.
        //
        IntersectRect(&rcDstT, &rcDst, &pMonitor->rcMonitor);

        //
        // Is this piece of the source staying on this monitor?
        //
        if (EqualRect(&rcDstT, &rcDst)) {
            //
            // This source piece is staying completely on this monitor when
            // it becomes the destination.  Hence there is nothing to add
            // to our invalid sum, hrgnInterMonitor.
            //
            if (EqualRect(&rcSrcT, &rcSrc)) {
                //
                // The source is completely ON one monitor and moving to
                // a location also completely ON this monitor. Great, no
                // intermonitor blts whatsoever.  We are done.
                //
                UserAssert(pcvr->hrgnInterMonitor == NULL);
                return;
            } else {
                continue;
            }
        }

        //
        // OK, some piece of the source is moving across monitors.  Figure
        // out what it is and where that piece is in the destination.  That
        // piece in the destination must be invalidated and not blted.
        //
        if (pcvr->hrgnInterMonitor == NULL) {
            pcvr->hrgnInterMonitor = CreateEmptyRgn();
        }

        //
        // The difference between the transposed source to the dest, and the
        // real part of the dest that lies on this monitor, is the amount
        // of the source that will move across a monitor boundary.  Add this
        // to our accumulated invalid region.
        //
        // rcDst is the whole source chunk, rcDstT is the part on the same
        // monitor as the source chunk.
        //
        GreSetRectRgn(ghrgnInv2, rcDst.left, rcDst.top, rcDst.right, rcDst.bottom);
        GreSetRectRgn(ghrgnGDC, rcDstT.left, rcDstT.top, rcDstT.right, rcDstT.bottom);
        SubtractRgn(ghrgnInv2, ghrgnInv2, ghrgnGDC);
        UnionRgn(pcvr->hrgnInterMonitor, pcvr->hrgnInterMonitor, ghrgnInv2);
    }
#if DBG
    VerifyVisibleMonitorCount();
#endif
}

