/****************************** Module Header ******************************\
* Module Name: update.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains the APIs used to invalidate, validate, and force
* updating of windows.
*
* History:
* 27-Oct-1990 DarrinM   Created.
* 25-Jan-1991 IanJa     Revalidation added
* 16-Jul-1991 DarrinM   Recreated from Win 3.1 sources.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/*
 * Local Constants.
 */
#define UW_ENUMCHILDREN 0x0001
#define UW_RECURSED     0x0004

#define RIR_OUTSIDE     0
#define RIR_INTERSECT   1
#define RIR_INSIDE      2

#define RDW_IGNOREUPDATEDIRTY 0x8000


/***************************************************************************\
* xxxInvalidateRect (API)
*
*
* History:
* 16-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

BOOL xxxInvalidateRect(
    PWND   pwnd,
    LPRECT lprcInvalid,
    BOOL   fErase)
{
    CheckLock(pwnd);

    /*
     * BACKWARD COMPATIBILITY HACK
     *
     * In Windows 3.0 and less, ValidateRect/InvalidateRect() call with
     * hwnd == NULL always INVALIDATED and ERASED the entire desktop, and
     * synchronously sent WM_ERASEBKGND and WM_NCPAINT messages before
     * returning.  The Rgn() calls did not have this behavior.
     */
    if (pwnd == NULL) {
        return xxxRedrawWindow(
                pwnd,
                NULL,
                NULL,
                RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_ERASE | RDW_ERASENOW);
    } else {
        return xxxRedrawWindow(
                pwnd,
                lprcInvalid,
                NULL,
                fErase ? RDW_INVALIDATE | RDW_ERASE : RDW_INVALIDATE);
    }
}

/***************************************************************************\
* xxxValidateRect (API)
*
*
* History:
* 16-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

BOOL xxxValidateRect(
    PWND   pwnd,
    LPRECT lprcValid)
{
    CheckLock(pwnd);

    /*
     * BACKWARD COMPATIBILITY HACK
     *
     * In Windows 3.0 and less, ValidateRect/InvalidateRect() call with
     * hwnd == NULL always INVALIDATED and ERASED the entire desktop, and
     * synchronously sent WM_ERASEBKGND and WM_NCPAINT messages before
     * returning.  The Rgn() calls did not have this behavior.
     */
    if (pwnd == NULL) {
        return xxxRedrawWindow(
                pwnd,
                NULL,
                NULL,
                RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_ERASE | RDW_ERASENOW);
    } else {
        return xxxRedrawWindow(pwnd, lprcValid, NULL, RDW_VALIDATE);
    }
}

/***************************************************************************\
* xxxInvalidateRgn (API)
*
*
* History:
* 16-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

BOOL xxxInvalidateRgn(
    PWND pwnd,
    HRGN hrgnInvalid,
    BOOL fErase)
{
    CheckLock(pwnd);

    return xxxRedrawWindow(
            pwnd,
            NULL,
            hrgnInvalid,
            fErase ? RDW_INVALIDATE | RDW_ERASE : RDW_INVALIDATE);
}

/***************************************************************************\
* xxxValidateRgn (API)
*
*
* History:
* 16-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

BOOL xxxValidateRgn(
    PWND pwnd,
    HRGN hrgnValid)
{
    CheckLock(pwnd);

    return xxxRedrawWindow(pwnd, NULL, hrgnValid, RDW_VALIDATE);
}

/***************************************************************************\
* SmartRectInRegion
*
* This routine is similar to RectInRegion, except that it also determines
* whether or not *lprc is completely within hrgn or not.
*
* RIR_OUTSIDE   - no intersection
* RIR_INTERSECT - *lprc intersects hrgn, but not completely inside
* RIR_INSIDE    - *lprc is completely within hrgn.
*
* LATER:
* It would be MUCH faster to put this functionality into GDI's RectInRegion
* call (a la PM's RectInRegion)
*
* History:
* 16-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

UINT SmartRectInRegion(
    HRGN   hrgn,
    LPRECT lprc)
{
    RECT rc;

    if (!GreRectInRegion(hrgn, lprc))
        return RIR_OUTSIDE;

    /*
     * Algorithm: if the intersection of hrgn and *lprc is the
     * same as *lprc, then *lprc is completely within hrgn.
     *
     * If the region is a rectangular one, then do it the easy way.
     */
    if (GreGetRgnBox(hrgn, &rc) == SIMPLEREGION) {

        if (!IntersectRect(&rc, &rc, lprc))
            return RIR_OUTSIDE;

        if (EqualRect(lprc, &rc))
            return RIR_INSIDE;

    } else {

        SetRectRgnIndirect(ghrgnInv2, lprc);

        switch (IntersectRgn(ghrgnInv2, ghrgnInv2, hrgn)) {

        case SIMPLEREGION:
            GreGetRgnBox(ghrgnInv2, &rc);
            if (EqualRect(lprc, &rc))
                return RIR_INSIDE;
            break;

#define RECTINREGION_BUG
#ifdef RECTINREGION_BUG

        /*
         * NOTE: RectInRegion has a BUG, where it sometimes returns TRUE
         * even if the rectangles of a region touch only on the edges
         * with no overlap.  This will result in an empty region after
         * the combination above.
         */
        case NULLREGION:
            return RIR_OUTSIDE;
            break;
#endif

        default:
            break;
        }
    }

    return RIR_INTERSECT;
}

/***************************************************************************\
* PixieHack
*
* BACKWARD COMPATIBILITY HACK
*
* In 3.0, WM_NCPAINT messages would be sent to any child window that was
* inside the bounding rectangle of a window management operation involving
* any other child, even if the intersection of that region with the child
* is empty.
*
* Some apps such as Pixie 2.3 and CA Cricket Presents rely on this to ensure
* that their tool windows stay on top of other child windows.  When the tool
* window gets a WM_NCPAINT, it brings itself to the top of the pile.
*
* Borland ObjectVision depends on getting the WM_NCPAINT after an
* invalidation of its parent window in an area that include the non-client
* area of the child.  When it recieves the WM_NCPAINT, it must get a
* clip region of HRGN_FULL, or nothing gets drawn.
*
* History:
* 02-Mar-1992 MikeKe    Ported from Win 3.1 sources.
\***************************************************************************/

VOID PixieHack(
    PWND   pwnd,
    LPRECT prcBounds)
{
    /*
     * If a child intersects the update region, and it isn't already
     * getting an NCPAINT, then make sure it gets one later.
     *
     * Don't apply this hack to top level windows.
     */
    if ((pwnd != _GetDesktopWindow()) &&
        TestWF(pwnd, WFCLIPCHILDREN)  &&
        !TestWF(pwnd, WFMINIMIZED)) {

        RECT rc;

        for (pwnd = pwnd->spwndChild; pwnd; pwnd = pwnd->spwndNext) {

            /*
             * If the window isn't already getting an NCPAINT message,
             * and it has a caption, and it's inside the bounding rect,
             * make sure it gets a WM_NCPAINT with wParam == HRGN_FULL.
             */
            if (!TestWF(pwnd, WFSENDNCPAINT)                      &&
                (TestWF(pwnd, WFBORDERMASK) == LOBYTE(WFCAPTION)) &&
                IntersectRect(&rc, prcBounds, &pwnd->rcWindow)) {

                /*
                 * Sync paint count is incremented when
                 * (senderasebkgnd | sendncpaint) goes from 0 to != 0.
                 * (should make a routine out of this!)
                 */
                SetWF(pwnd, WFSENDNCPAINT);

                /*
                 * Force HRGN_FULL clip rgn.
                 */
                SetWF(pwnd, WFPIXIEHACK);
            }
        }
    }
}

/***************************************************************************\
* xxxRedrawWindow (API)
*
* Forward to xxxInvalidateWindow if the window is visible.
*
* BACKWARD COMPATIBILITY HACK
*
* In Windows 3.0 and less, ValidateRect/InvalidateRect() call with pwnd == NULL
* always INVALIDATED and ERASED all windows, and synchronously sent
* WM_ERASEBKGND and WM_NCPAINT messages before returning.  The Rgn() calls
* did not have this behavior. This case is handled in
* InvalidateRect/ValidateRect.
*
* History:
* 16-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

BOOL xxxRedrawWindow(
    PWND   pwnd,
    LPRECT lprcUpdate,
    HRGN   hrgnUpdate,
    DWORD  flags)
{
    CheckLock(pwnd);

    /*
     * Always map NULL to the desktop.
     */
    if (pwnd == NULL) {
        pwnd = PtiCurrent()->rpdesk->pDeskInfo->spwnd;
    }

    UserAssert(pwnd != NULL);

    if (IsVisible(pwnd)) {

        TL   tlpwnd;
        HRGN hrgn = hrgnUpdate;

        if (flags & (RDW_VALIDATE | RDW_INVALIDATE)) {

            /*
             * Create a (in)validate region in client window coordinates.
             */
            if (hrgn == NULL) {
                if (!lprcUpdate) {
                    hrgn = HRGN_FULL;
                } else {
                    hrgn = ghrgnInv0;

#ifdef USE_MIRRORING
                    if (TestWF(pwnd, WEFLAYOUTRTL)) {
                        MirrorRect(pwnd, lprcUpdate);
                    }
#endif

                    if (pwnd == PWNDDESKTOP(pwnd)) {
                        SetRectRgnIndirect(hrgn, lprcUpdate);
                    } else {
                        GreSetRectRgn(
                                hrgn,
                                lprcUpdate->left + pwnd->rcClient.left,
                                lprcUpdate->top + pwnd->rcClient.top,
                                lprcUpdate->right + pwnd->rcClient.left,
                                lprcUpdate->bottom + pwnd->rcClient.top);
                    }
                }
            } else {
                /*
                 * If necessary, make a copy of the passed-in region, because
                 * we'll be trashing it...
                 */
                if (hrgn != HRGN_FULL) {
                    CopyRgn(ghrgnInv0, hrgn);

#ifdef USE_MIRRORING
                    MirrorRegion(pwnd, ghrgnInv0, TRUE);
#endif

                    hrgn = ghrgnInv0;
                }

                if (pwnd != PWNDDESKTOP(pwnd)) {
                    GreOffsetRgn(hrgn, pwnd->rcClient.left, pwnd->rcClient.top);
                }
            }
        }

        ThreadLock(pwnd, &tlpwnd);
        xxxInternalInvalidate(pwnd, hrgn, flags | RDW_REDRAWWINDOW);
        ThreadUnlock(&tlpwnd);
    }

    return TRUE;
}

/***************************************************************************\
* InternalInvalidate2
*
* (In)validates hrgn in pwnd and in child windows of pwnd. Child windows
* also subtract their visible region from hrgnSubtract.
*
* pwnd         - The window to (in)validate.
* hrng         - The region to (in)validate.
* hrgnSubtract - The region to subtract the visible region of
*                 child windows from.
* prcParents   - Contains the intersection of pwnd's client or window rect
*                 with the client rectangles of its parents. May just be
*                 the window's client or window rect.
*
* flags        - RDW_ flags.
*
* Returns FALSE if hrgnSubtract becomes a NULLREGION, TRUE otherwise.
*
* History:
* 16-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

BOOL InternalInvalidate2(
    PWND   pwnd,
    HRGN   hrgn,
    HRGN   hrgnSubtract,
    LPRECT prcParents,
    DWORD  flags)
{
    /*
     * NOTE: Uses ghrgnInv2
     */
    RECT  rcOurShare;
    DWORD flagsChildren;
    PWND  pwndT;

    /*
     * This routine is called recursively down the parent/child chain.
     * Remember if on the way one of the windows has a clipping region.
     * This info is used later on to optimize out a loop in the common
     * case.
     */
    if (pwnd->hrgnClip != NULL) {
        flags |= RDW_HASWINDOWRGN;
    }

    /*
     * If we recurse, make sure our children subtract themselves off.
     */
    flagsChildren = flags | RDW_SUBTRACTSELF;
    CopyRect(&rcOurShare, &pwnd->rcWindow);

    /*
     * If we're invalidating, we only want to deal with the part of
     * our window rectangle that intersects our parents.
     * This way, we don't end up validating or invalidating more than our
     * fair share. If we're completely obscured by our parents, then there is
     * nothing to do.
     *
     * We don't perform this intersection if we're validating, because there
     * are cases where a child and its update region may exist but be obscured
     * by parents, and we want to make sure validation will work in these
     * cases.  ScrollWindow() can cause this when children are offset, as can
     * various 3.0 compatibility hacks.
     */

    if (flags & RDW_INVALIDATE) {
        /*
         * Don't subtract out any sprite windows from the invalid region.
         * Behave as if it's not there.  However, always allow layered window
         * invalidation when RDW_INVALIDATELAYERS is passed in.
         */
        if (FLayeredOrRedirected(pwnd) && !(flags & RDW_INVALIDATELAYERS))
            return TRUE;

        if (!IntersectRect(&rcOurShare, &rcOurShare, prcParents)) {

            /*
             * BACKWARD COMPATIBILITY HACK: If hrgn is (HRGN)1, we need to
             * invalidate ALL child windows, even if they're not visible.  This
             * is a bummer, because it'll result in all sorts of repaints that
             * aren't necessary.
             *
             * Various apps, including WordStar for Windows and WaveEdit,
             * depend on this behavior.  Here's how WaveEdit relies on this: it
             * has a CS_HDREDRAW | CS_VREDRAW window, that moves its children
             * around with MoveWindow( ..., fRedraw = FALSE).  The windows
             * not part of the new client area didn't get invalidated.
             */
            if (!TestWF(pwnd, WFWIN31COMPAT) && (hrgn == HRGN_FULL)) {

                /*
                 * For purposes of hit-testing, our share is our window
                 * rectangle.  However, we don't want to diddle the region
                 * passed to us, because by rights we're really clipped out!
                 */
                flags &= ~RDW_SUBTRACTSELF;
                flagsChildren &= ~RDW_SUBTRACTSELF;

            } else {
                return TRUE;
            }
        }

        /*
         * If our window rect doesn't intersect the valid/invalid region,
         * nothing further to do.
         */
        if (hrgn > HRGN_FULL) {

            switch (SmartRectInRegion(hrgn, &rcOurShare)) {
            case RIR_OUTSIDE:
                return TRUE;

            case RIR_INTERSECT:

                /*
                 * The update region can be within the window rect but not
                 * touch the window region; in this case we don't want this
                 * update region to be distributed to this window. If this
                 * is the case, return TRUE as if RIR_OUTSIDE.
                 *
                 * If RDW_HASWINDOWRGN is set, either this window or
                 * one of its parents has a window clipping region. This
                 * flag is just an optimization so that this loop isn't
                 * executed all the time.
                 *
                 * A future optimization may be to calculate this parent
                 * clipped region as part of recursion like prcParents is
                 * calculated. It is not super important though because this
                 * case rarely happens (a window with a region), and even
                 * more rare, a regional window that is a child of a regional
                 * window parent.
                 */
                if (flags & RDW_HASWINDOWRGN) {

                    /*
                     * Clip to the window's clipping region and parents!
                     * If we don't clip to parents, we may get a case where
                     * a child clips out some update region that was meant to
                     * go to a sibling of the parent.
                     */
                    SetRectRgnIndirect(ghrgnInv2, &rcOurShare);
                    for (pwndT = pwnd; pwndT != NULL; pwndT = pwndT->spwndParent) {

                        if (pwndT->hrgnClip != NULL) {

                            /*
                             * An error at this stage would possibly result
                             * in more being subtracted out of the clipping
                             * region that we'd like.
                             */
                            IntersectRgn(ghrgnInv2, ghrgnInv2, pwndT->hrgnClip);
                        }
                    }

                    if (IntersectRgn(ghrgnInv2, ghrgnInv2, hrgn) == NULLREGION)
                        return TRUE;
                }
                break;

            case RIR_INSIDE:
                /*
                 * If the rectangle is completely within hrgn, then we can use
                 * HRGN_FULL, which is much faster and easier to deal with.
                 *
                 * COMPAT HACK:  There are some apps (PP, MSDRAW) that depend
                 * on some weirdities of the 3.0 GetUpdateRect in order to
                 * paint properly.  Since this stuff hinges on whether the
                 * update region is 1 or a real region, we need to simulate
                 * when 3.0 would generate a HRGN(1) update region.  The
                 * following optimization was not made in 3.0, so we yank it
                 * in 3.1 for these apps.  (win31 bugs 8235,10380)
                 */
                if (!(GetAppCompatFlags(GETPTI(pwnd)) & GACF_NOHRGN1))
                    hrgn = HRGN_FULL;
                break;
            }
        }
    }

    /*
     * If not CLIPCHILDREN, go diddle the update region BEFORE our clipped
     * children have done their thing to hrgnSubtract. Otherwise,
     * we'll diddle after we recurse.
     */
    if (!TestWF(pwnd, WFCLIPCHILDREN)) {
        InternalInvalidate3(pwnd, hrgn, flags);
    }

    /*
     * If this is a GACF_ALWAYSSENDNCPAINT app, take care of it...
     */
    if (TestWF(pwnd, WFALWAYSSENDNCPAINT))
        PixieHack(pwnd, &rcOurShare);

    /*
     * Recurse on our children if necessary.
     *
     * By default, our children are enumerated if we are not CLIPCHILDREN.
     * Don't bother with children if we're minimized.
     */
    if ((pwnd->spwndChild != NULL) &&
        !TestWF(pwnd, WFMINIMIZED) &&
        !(flags & RDW_NOCHILDREN)  &&
        ((flags & RDW_ALLCHILDREN) || !TestWF(pwnd, WFCLIPCHILDREN))) {

        RECT rcChildrenShare;
        PWND pwndChild;

        /*
         * If we're invalidating, make sure our children
         * erase and frame themselves. Also, tell children to subtract
         * themselves from hrgnSubtract.
         */
        if (flags & RDW_INVALIDATE) {
            flagsChildren |= RDW_ERASE | RDW_FRAME;
        }

        /*
         * Our children are clipped to our client rect, so reflect
         * that in the rectangle we give them.
         */
        if (IntersectRect(&rcChildrenShare, &rcOurShare, &pwnd->rcClient) ||
            (!TestWF(pwnd, WFWIN31COMPAT) && (hrgn == HRGN_FULL))) {

            for (pwndChild = pwnd->spwndChild; pwndChild != NULL;
                    pwndChild = pwndChild->spwndNext) {

                if (!TestWF(pwndChild, WFVISIBLE))
                    continue;

                if (!InternalInvalidate2(pwndChild,
                                         hrgn,
                                         hrgnSubtract,
                                         &rcChildrenShare,
                                         flagsChildren)) {

                    /*
                     * The children swallowed the region:
                     * If there are no update region related things
                     * to do then we can just return with FALSE
                     */
                    if (!(flags & (RDW_INTERNALPAINT | RDW_NOINTERNALPAINT)))
                        return FALSE;

                    /*
                     * We have to enumerate the rest of the children because
                     * one of the RDW_NO/INTERNALPAINT bits is set.  Since
                     * there's no longer any update region to worry about,
                     * strip out the update region bits from the parent
                     * and child fiags.  Also, tell the children not to
                     * bother subtracting themselves from the region.
                     */
                    flags &= ~(RDW_INVALIDATE |
                               RDW_ERASE      |
                               RDW_FRAME      |
                               RDW_VALIDATE   |
                               RDW_NOERASE    |
                               RDW_NOFRAME);

                    flagsChildren &= ~(RDW_INVALIDATE |
                                       RDW_ERASE      |
                                       RDW_FRAME      |
                                       RDW_VALIDATE   |
                                       RDW_NOERASE    |
                                       RDW_NOFRAME    |
                                       RDW_SUBTRACTSELF);
                }
            }
        }
    }

    /*
     * Go diddle the update region  (AFTER our clipped children may have
     * done their thing to hrgnSubtract)
     */
    if (TestWF(pwnd, WFCLIPCHILDREN))
        InternalInvalidate3(pwnd, hrgn, flags);

    /*
     * If we're invalidating and we're supposed to,
     * try to subtract off our window area from the region.
     *
     * This way our parent and our siblings below us will not
     * get any update region for areas that don't need one.
     */
    if (flags & RDW_SUBTRACTSELF) {

        /*
         * Subtract our visible region from the update rgn only if:
         * a) we're not a transparent window
         * b) we are clipsiblings
         * c) we're validating OR our parent is clipchildren.
         *
         * The check for validation is a backward-compatibility hack: this
         * is what 3.0 did, so this is what we do here.
         *
         * BACKWARD COMPATIBILITY HACK
         *
         * In 3.0, we subtracted this window from the update rgn if it
         * was clipsiblings, even if the parent was not clipchildren.
         * This causes a compatibility problem for Lotus Notes 3.1: it
         * has a combobox dropdown in a dialog that is a WS_CLIPSIBLING
         * sibling of the other dialog controls, which are not WS_CLIPSIBLINGs.
         * The dialog is not WS_CLIPCHILDREN.  What happens is that a listbox
         * underneath the dropdown also gets a paint msg (since we didn't
         * do this subtraction), and, since it's not CLIPSIBLINGS, it
         * obliterates the dropdown.
         *
         * This is a very obscure difference, and it's too late in the
         * project to make this change now, so we're leaving the code as is
         * and using a compatibility hack to enable the 3.0-compatible
         * behavior.  It's quite likely that this code works the way it does
         * for other compatibility reasons.  Sigh (neilk).
         */
        if (!TestWF(pwnd, WEFTRANSPARENT) &&
            TestWF(pwnd, WFCLIPSIBLINGS)  &&
            ((flags & RDW_VALIDATE) ||
                 ((pwnd->spwndParent != NULL) &&
                 (TestWF(pwnd->spwndParent, WFCLIPCHILDREN) ||
                 (GetAppCompatFlags(GETPTI(pwnd)) & GACF_SUBTRACTCLIPSIBS))))) {

            /*
             * Intersect with our visible area.
             *
             * Don't worry about errors: an error will result in more, not less
             * area being invalidated, which is okay.
             */
            SetRectRgnIndirect(ghrgnInv2, &rcOurShare);

            /*
             * If RDW_HASWINDOWRGN is set, either this window or
             * one of its parents has a window clipping region. This
             * flag is just an optimization so that this loop isn't
             * executed all the time.
             */
            if (flags & RDW_HASWINDOWRGN) {

                /*
                 * Clip to the window's clipping region and parents!
                 * If we don't clip to parents, we may get a case where
                 * a child clips out some update region that was meant to
                 * go to a sibling of the parent.
                 */
                for (pwndT = pwnd; pwndT != NULL; pwndT = pwndT->spwndParent) {

                    if (pwndT->hrgnClip != NULL) {

                        /*
                         * An error at this stage would possibly result in more
                         * being subtracted out of the clipping region that
                         * we'd like.
                         */
                        IntersectRgn(ghrgnInv2, ghrgnInv2, pwndT->hrgnClip);
                    }
                }
            }


#if 1
            /*
             * TEMP HACK!!! RE-ENABLE this code when regions work again
             */
            if (SubtractRgn(hrgnSubtract, hrgnSubtract, ghrgnInv2) == NULLREGION)
                return FALSE;
#else
            {
            DWORD iRet;

            iRet = SubtractRgn(hrgnSubtract, hrgnSubtract, ghrgnInv2);

            if (iRet == NULLREGION)
                return FALSE;

            if (iRet == SIMPLEREGION) {
                RECT rcSub;
                GreGetRgnBox(hrgnSubtract, &rcSub);
                if (rcSub.left > rcSub.right)
                    return FALSE;
            }
            }
#endif



        }
    }

    return TRUE;
}

/***************************************************************************\
* InternalInvalidate3
*
* Adds or subtracts hrgn to the windows update region and sets appropriate
* painting state flags.
*
* pwnd  - The window.
* hrng  - The region to add to the update region.
* flags - RDW_ flags.
*
* History:
* 16-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

VOID InternalInvalidate3(
    PWND  pwnd,
    HRGN  hrgn,
    DWORD flags)
{
    BOOL fNeededPaint;

    fNeededPaint = NEEDSPAINT(pwnd);

    if (flags & (RDW_INVALIDATE | RDW_INTERNALPAINT | RDW_ERASE | RDW_FRAME)) {

        if (flags & RDW_INTERNALPAINT)
            SetWF(pwnd, WFINTERNALPAINT);

        if (flags & RDW_INVALIDATE) {

            /*
             * Make sure that the NONCPAINT bit is cleared
             * to ensure that the caption will redraw when we update.
             */
            ClrWF(pwnd, WFNONCPAINT);

            /*
             * If another app is invalidating this window, then set the
             * UPDATEDIRTY flag.
             *
             * Solves critical section where thread A draws, then validates,
             * but thread B goes and invalidates before A validates.
             * See comments later in RDW_VALIDATE code.
             */
            if (GETPTI(pwnd) != PtiCurrent()) {

                SetWF(pwnd, WFUPDATEDIRTY);

                /*
                 * Paint order problem, see paint.c
                 */
                if (TestWF(pwnd, WFWMPAINTSENT)) {
                    SetWF(pwnd, WFDONTVALIDATE);
                }
            }

            /*
             * BACKWARD COMPATIBILITY HACK
             *
             * In 3.0, InvalidateRect(pwnd, NULL, FALSE) would always
             * clear the WFSENDERASEBKGND flag, even if it was previously
             * set from an InvalidateRect(pwnd, NULL, TRUE).  This is bogus,
             * because it can cause you to "lose" WM_ERASEBKGND messages, but
             * AttachMate Extra (and maybe other apps) depend on this behavior.
             */
            if ((hrgn == HRGN_FULL) && !TestWF(pwnd, WFWIN31COMPAT))
                ClrWF(pwnd, WFSENDERASEBKGND);

            if (flags & RDW_ERASE)
                SetWF(pwnd, WFSENDERASEBKGND);

            if ((flags & (RDW_FRAME | RDW_ERASE)) && !TestWF(pwnd, WEFTRANSPARENT))
                SetHungFlag(pwnd, WFREDRAWIFHUNG);

            if (flags & RDW_FRAME)
                SetWF(pwnd, WFSENDNCPAINT);

            /*
             * If window is already completely invalidated,
             * no need to do any further invalidation.
             */
            if (pwnd->hrgnUpdate != HRGN_FULL) {

                if (hrgn == HRGN_FULL) {
InvalidateAll:
                    DeleteMaybeSpecialRgn(pwnd->hrgnUpdate);
                    pwnd->hrgnUpdate = HRGN_FULL;

                } else {
                    if (pwnd->hrgnUpdate == NULL) {

                        if (!(pwnd->hrgnUpdate = CreateEmptyRgnPublic()))
                            goto InvalidateAll;

                        if (CopyRgn(pwnd->hrgnUpdate, hrgn) == ERROR)
                            goto InvalidateAll;

                    } else {   // pwnd->hrgnUpdate is a region

                        if (UnionRgn(pwnd->hrgnUpdate,
                                     pwnd->hrgnUpdate,
                                     hrgn) == ERROR) {

                            goto InvalidateAll;
                        }
                    }
                }
            }
        }

        if (!fNeededPaint && NEEDSPAINT(pwnd))
            IncPaintCount(pwnd);

    } else if (flags & (RDW_VALIDATE | RDW_NOINTERNALPAINT | RDW_NOERASE | RDW_NOFRAME)) {

        /*
         * Validation:
         *
         * Do not allow validation if this window has been invalidated from
         * another process - because this window may be validating just
         * after another process invalidated, thereby validating invalid
         * bits.
         *
         * Sometimes applications draw stuff, then validate what they drew.
         * If another app invalidated some area during the drawing operation,
         * then it will need another paint message.
         *
         * This wouldn't be necessary if people validated BEFORE they drew.
         */
        if (TestWF(pwnd, WFUPDATEDIRTY) && !(flags & RDW_IGNOREUPDATEDIRTY))
            return;

        if (flags & RDW_NOINTERNALPAINT)
            ClrWF(pwnd, WFINTERNALPAINT);

        if (flags & RDW_VALIDATE) {

            if (flags & RDW_NOERASE)
                ClrWF(pwnd, WFSENDERASEBKGND);

            if (flags & RDW_NOFRAME) {
                ClrWF(pwnd, WFSENDNCPAINT);
                ClrWF(pwnd, WFPIXIEHACK);
            }

            if (flags & (RDW_NOERASE | RDW_NOFRAME))
                ClearHungFlag(pwnd, WFREDRAWIFHUNG);

            if (pwnd->hrgnUpdate != NULL) {

                /*
                 * If WFSENDNCPAINT is set, then all or part of the
                 * window border still needs to be drawn.  This means
                 * that we must subtract off the client rectangle only.
                 * Convert HRGN_FULL to the client region.
                 */
                if (TestWF(pwnd, WFSENDNCPAINT) && (hrgn == HRGN_FULL)) {
                    hrgn = ghrgnInv2;
                    CalcWindowRgn(pwnd, hrgn, TRUE);
                }

                if (hrgn == HRGN_FULL) {
ValidateAll:

                    /*
                     * We're validating the entire window.  Just
                     * blow away the update region.
                     */
                    DeleteMaybeSpecialRgn(pwnd->hrgnUpdate);
                    pwnd->hrgnUpdate = (HRGN)NULL;

                    /*
                     * No need to erase the background...
                     */
                    ClrWF(pwnd, WFSENDERASEBKGND);
                    ClearHungFlag(pwnd, WFREDRAWIFHUNG);

                } else {

                    /*
                     * Subtracting some region from pwnd->hrgnUpdate.
                     * Be sure pwnd->hrgnUpdate is a real region.
                     */
                    if (pwnd->hrgnUpdate == HRGN_FULL) {

                        /*
                         * If the WFSENDNCPAINT bit is set,
                         * the update region must include the entire window
                         * area.  Otherwise it includes only the client.
                         */
                        pwnd->hrgnUpdate = CreateEmptyRgnPublic();

                        /*
                         * If the creation failed, punt by
                         * invalidating the entire window.
                         */
                        if (pwnd->hrgnUpdate == NULL)
                            goto InvalidateAll;

                        if (CalcWindowRgn(pwnd,
                                          pwnd->hrgnUpdate,
                                          !(TestWF(pwnd, WFSENDNCPAINT))) == ERROR) {

                            goto InvalidateAll;
                        }
                    }

                    /*
                     * Subtract off the region.  If we get an error,
                     * punt by invalidating everything.  If the
                     * region becomes empty, then validate everything.
                     */
                    switch (SubtractRgn(pwnd->hrgnUpdate,
                                        pwnd->hrgnUpdate,
                                        hrgn)) {
                    case ERROR:
                        goto InvalidateAll;

                    case NULLREGION:
                        goto ValidateAll;
                    }
                }
            }
        }

        if (fNeededPaint && !NEEDSPAINT(pwnd))
            DecPaintCount(pwnd);
    }
}

/***************************************************************************\
* ValidateParents
*
* This routine validates hrgn from the update regions of the parent windows
* between pwnd and its first clip children parent.
* If hrgn is NULL, then the window rect (intersected with all parents)
* is validated.
*
* This routine is called when a window is being drawn in
* UpdateWindow() so that non-CLIPCHILDREN parents
* of windows being redrawn won't draw on their valid children.
*
* Returns FALSE if fRecurse is TRUE and a non-CLIPCHILDREN parent
* has an update region; otherwise, returns TRUE.
*
* History:
* 16-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

BOOL ValidateParents(
    PWND pwnd,
    BOOL fRecurse)
{
    RECT rcParents;
    RECT rc;
    PWND pwndParent = pwnd;
    BOOL fInit = FALSE;

    /*
     * This is checking whether we are in an in-between state, just before
     * a WM_SYNCPAINT is about to arrive.  If not, then ValidateParents()
     * needs to work like it did in Win 3.1.
     */
    while (TestWF(pwndParent, WFCHILD))
        pwndParent = pwndParent->spwndParent;

    if (!TestWF(pwndParent, WFSYNCPAINTPENDING))
        fRecurse = FALSE;

    pwndParent = pwnd;

    while ((pwndParent = pwndParent->spwndParent) != NULL) {

        /*
         * Stop when we find a clipchildren parent
         */
        if (TestWF(pwndParent, WFCLIPCHILDREN))
            break;

        /*
         * Subtract the region from this parent's update region,
         * if it has one.
         */
        if (pwndParent->hrgnUpdate != NULL) {
            if (fRecurse) {
                return FALSE;
            }
            if (!fInit) {
                fInit = TRUE;

                /*
                 * Do initial setup.  If our window rectangle is
                 * completely obscured, get out.
                 */
                rc = pwnd->rcWindow;
                if (!IntersectWithParents(pwnd, &rc))
                    break;

                SetRectRgnIndirect(ghrgnInv1, &rc);

                /*
                 * If this window has a region, make sure the piece being validated
                 * is within this region.
                 */
                if (pwnd->hrgnClip != NULL) {

                    /*
                     * If we get NULLREGION back, there is nothing to validate
                     * against parents, so break out. If ERROR gets returned,
                     * there is not much we can do: the best "wrong" thing
                     * to do is just continue and validate a little more
                     * from the parent.
                     */
                    if (!IntersectRgn(ghrgnInv1, ghrgnInv1, pwnd->hrgnClip))
                        break;
                }
            }

            /*
             * Calculate the rcParents parameter to
             * pass up to InternalInvalidate2.
             */
            rcParents = pwndParent->rcWindow;

            if (!IntersectWithParents(pwndParent, &rcParents))
                break;

            InternalInvalidate2(
                    pwndParent,
                    ghrgnInv1,
                    ghrgnInv1,
                    &rcParents,
                    RDW_VALIDATE | RDW_NOCHILDREN | RDW_IGNOREUPDATEDIRTY);
        }
    }

    return TRUE;
}

/***************************************************************************\
* xxxUpdateWindow2
*
* Sends a WM_PAINT message to the window if it needs painting,
* then sends the message to its children.
*
* Always returns TRUE.
*
* History:
* 16-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

void xxxUpdateWindow2(
    PWND  pwnd,
    DWORD flags)
{
    TL tlpwnd;

    CheckLock(pwnd);

    if (NEEDSPAINT(pwnd)) {

        /*
         * Punch a hole in our parent's update region, if we have one.
         */
        if (pwnd->hrgnUpdate) {
            if (ValidateParents(pwnd, flags & UW_RECURSED) == FALSE) {
                return;
            }
        }

        /*
         * Now that we're sending the message, clear the
         * internal paint bit if it was previously set.
         */
        if (TestWF(pwnd, WFINTERNALPAINT)) {

            ClrWF(pwnd, WFINTERNALPAINT);

            /*
             * If there is no update region, then no further paint messages
             * are pending, so we must dec the paint count.
             */
            if (pwnd->hrgnUpdate == NULL)
                DecPaintCount(pwnd);
        }

        /*
         * Set a flag indicating that a paint message was not processed
         * (but should be).
         */
        SetWF(pwnd, WFPAINTNOTPROCESSED);

        /*
         * Clear this bit, for apps (like MicroLink) that don't call
         * BeginPaint or GetUpdateRect/Rgn (but DO call ValidateRect)
         * when handling their WM_PAINT message.
         */
        ClrWF(pwnd, WFUPDATEDIRTY);

        /*
         * BACKWARD COMPATIBILITY HACK
         *
         * Win 3.0 always sent WM_PAINTICON with wParam == TRUE for no good
         * reason, and Lotus Notes has come to depend on this.
         */
        if (!TestWF(pwnd, WFWIN40COMPAT) &&
            TestWF(pwnd, WFMINIMIZED)    &&
            (pwnd->pcls->spicn != NULL)) {

            xxxSendMessage(pwnd, WM_PAINTICON, TRUE, 0L);

        } else {

            xxxSendMessage(pwnd, WM_PAINT, 0, 0L);
        }

        /*
         * If the guy didn't call BeginPaint/EndPaint(), or GetUpdateRect/Rgn
         * with fErase == TRUE, then we have to clean up for him here.
         */
        if (TestWF(pwnd, WFPAINTNOTPROCESSED)) {

            RIPMSG0(RIP_VERBOSE,
                "App didn't call BeginPaint() or GetUpdateRect/Rgn(fErase == TRUE) in WM_PAINT");

            xxxSimpleDoSyncPaint(pwnd);
        }
    }

    /*
     * For desktop window, do not force the top level window repaint at this
     * this point.  We are calling UpdateWindow() for the desktop before
     * size/move is sent for the top level windows.
     *
     * BUG: The comment above seems a bit random.  Is there really a problem?
     *      If nothing else this has to remain this way because it is
     *      how Win 3.0 worked (neilk)
     */
    if ((flags & UW_ENUMCHILDREN) && (pwnd != PWNDDESKTOP(pwnd))) {

        /*
         * Update any children...
         */
        ThreadLockNever(&tlpwnd);
        pwnd = pwnd->spwndChild;
        while (pwnd != NULL) {

            /*
             * If there is a transparent window that needs painting,
             * skip it if another window below it needs to paint.
             */
            if (TestWF(pwnd, WEFTRANSPARENT) && NEEDSPAINT(pwnd)) {

                PWND pwndT = pwnd;

                while ((pwndT = pwndT->spwndNext) != NULL) {
                    if (NEEDSPAINT(pwndT))
                        break;
                }

                if (pwndT != NULL) {
                    pwnd = pwnd->spwndNext;
                    continue;
                }
            }

            ThreadLockExchangeAlways(pwnd, &tlpwnd);
            xxxUpdateWindow2(pwnd, flags | UW_RECURSED);
            pwnd = pwnd->spwndNext;
        }

        ThreadUnlock(&tlpwnd);
    }

    return;
}

/***************************************************************************\
* xxxInternalUpdateWindow
*
* Sends a WM_PAINT message to the window if it needs painting,
* then sends the message to its children. Won't send WM_PAINT
* if the window is transparent and has siblings that need
* painting.
*
* History:
* 16-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

void xxxInternalUpdateWindow(
    PWND pwnd,
    DWORD flags)
{
    CheckLock(pwnd);

    /*
     * If the passed-in window is transparent and a sibling below
     * needs repainting, don't do anything.
     */
    if (TestWF(pwnd, WEFTRANSPARENT)) {

        PWND         pwndT = pwnd;
        PTHREADINFO ptiCurrent = GETPTI(pwnd);

        while ((pwndT = pwndT->spwndNext) != NULL) {

            /*
             * Make sure sibling window belongs to same app.
             */
            if (GETPTI(pwndT) != ptiCurrent)
                continue;

            if (NEEDSPAINT(pwndT))
                return;
        }
    }

    /*
     * Enumerate pwnd and all its children, sending WM_PAINTs as needed.
     */
    xxxUpdateWindow2(pwnd, flags);
}

/***************************************************************************\
* xxxInternalInvalidate
*
* (In)validates hrgnUpdate and updates the window.
*
* History:
* 16-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

VOID xxxInternalInvalidate(
    PWND  pwnd,
    HRGN  hrgnUpdate,
    DWORD flags)
{
    RECT rcParents;
    HRGN hrgnSubtract;

#if DBG
    if (flags & (RDW_ERASENOW | RDW_UPDATENOW)) {
        CheckLock(pwnd);
    }
#endif

    /*
     * Allow invalidation of a layered window when someone specifically
     * invalidates it.  This will also prevent invalidation of layered
     * windows during recursive desktop invalidations.
     */
    if (FLayeredOrRedirected(pwnd)) {
        flags |= RDW_INVALIDATELAYERS;
    }

    /*
     * Ensure that hrgnSubtract is a valid region: if it's NULLREGION,
     * use the client region.
     */
    rcParents = (flags & RDW_FRAME ? pwnd->rcWindow : pwnd->rcClient);

    if (flags & (RDW_VALIDATE | RDW_INVALIDATE)) {

        hrgnSubtract = hrgnUpdate;

        if (hrgnSubtract == HRGN_FULL) {

            hrgnSubtract = ghrgnInv1;
            CalcWindowRgn(pwnd,
                          hrgnSubtract,
                          (flags & RDW_FRAME) ? FALSE : TRUE);
        }

        /*
         * Calculate the bounding rectangle of our screen real estate,
         * by intersecting with our parent rectangles.  While we're at
         * it, check the visibility of ourself and our parents.
         *
         * If we're validating we want to skip this, since there
         * are a number of cases where obscured windows may have
         * update regions to be validated -- in particular, after
         * a ScrollWindow() call where a child window was offset
         * by OffsetChildren() to a new, obscured position.  Some of
         * the 3.0 compatibility hacks also can lead to this situation.
         */
        if ((flags & RDW_INVALIDATE) && !IntersectWithParents(pwnd, &rcParents))
            return;

    } else {
        /*
         * hrgnsubtract needs to be a real region even if
         * we are not invalidating or validating.  It really doesn't
         * matter what the region is, but we set it to null so the code
         * has less degrees of freedom.
         */
        hrgnSubtract = ghrgnInv1;
        SetEmptyRgn(hrgnSubtract);
    }

    /*
     * If we're invalidating, and we're being called by the app,
     * we need to invalidate any SPBs that might be affected by
     * drawing in the client area of this window.
     * We have to do this because there is no guarantee that the
     * application will draw in an area that is invalidated
     * (e.g., if the window is completely obscured by another).
     */
    if (    (flags & (RDW_INVALIDATE | RDW_REDRAWWINDOW)) == (RDW_INVALIDATE | RDW_REDRAWWINDOW) &&
            AnySpbs()) {

        RECT rcInvalid;

        /*
         * Intersect the parent's rect with the region bounds...
         */
        GreGetRgnBox(hrgnSubtract, &rcInvalid);
        IntersectRect(&rcInvalid, &rcInvalid, &rcParents);
        SpbCheckRect(pwnd, &rcInvalid, 0);
    }

    /*
     * Now go do the recursive update region calculations...
     */
    InternalInvalidate2(pwnd, hrgnUpdate, hrgnSubtract, &rcParents, flags);

    /*
     * Finally handle any needed drawing.
     *
     * (NOTE: RDW_UPDATENOW implies RDW_ERASENOW)
     */
    if (flags & RDW_UPDATENOW) {

        xxxInternalUpdateWindow(pwnd,
                                flags & RDW_NOCHILDREN ? 0 : UW_ENUMCHILDREN);

    } else if (flags & RDW_ERASENOW) {

        UINT flagsDSP;

        if (flags & RDW_NOCHILDREN) {
            flagsDSP = 0;
        } else if (flags & RDW_ALLCHILDREN) {
            flagsDSP = DSP_ALLCHILDREN;
        } else {
            flagsDSP = DSP_ENUMCLIPPEDCHILDREN;
        }

        xxxDoSyncPaint(pwnd, flagsDSP);
    }
}

/***************************************************************************\
* UpdateWindow (API)
*
* Updates the window and all its children.
*
* History:
* 16-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

BOOL xxxUpdateWindow(
    PWND pwnd)
{
    CheckLock(pwnd);

    xxxInternalUpdateWindow(pwnd, UW_ENUMCHILDREN);

    /*
     * This function needs to return a value, since it is
     * called through NtUserCallHwndLock.
     */
    return TRUE;
}

/***************************************************************************\
* ExcludeUpdateRgn (API)
*
* ENTRY:    hdc - DC to exclude from
*           pwnd - window handle
*
* EXIT:     GDI region type
*
* WARNINGS: The DC is assumed to correspond to the client area of the window.
*
*           The map mode of hdc MUST be text mode (0, 0 is top left corner,
*           one pixel per unit, ascending down and to right) or things won't
*           work.
*
* History:
* 16-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

int _ExcludeUpdateRgn(
    HDC  hdc,
    PWND pwnd)
{
    POINT pt;

    if (pwnd->hrgnUpdate == NULL) {

        RECT rc;

        /*
         * Pass FALSE for fXForm since &rc isn't used.
         */
        return GreGetClipBox(hdc, &rc, FALSE);

    } else if (pwnd->hrgnUpdate == HRGN_FULL) {

        return GreIntersectClipRect(hdc, 0, 0, 0, 0);

    } else {

        /*
         * If no clip rgn exists, then subtract from a device-sized clip rgn.
         * (GetClipRgn returns clip rgn in screen coordinates).
         */
        GreGetDCOrg(hdc, &pt);
        if (GreGetRandomRgn(hdc, ghrgnInv1, 1) != 1) {
            CopyRgn(ghrgnInv1, gpDispInfo->hrgnScreen);
        } else {

            /*
             * Gets returned in dc coords - translate to screen.
             */
            GreOffsetRgn(ghrgnInv1, pt.x, pt.y);
        }

        SubtractRgn(ghrgnInv1, ghrgnInv1, pwnd->hrgnUpdate);

        /*
         * Map to dc coords before select
         */
        GreOffsetRgn(ghrgnInv1, -pt.x, -pt.y);

        return GreExtSelectClipRgn(hdc, ghrgnInv1, RGN_COPY);
    }
}

/***************************************************************************\
* GetUpdateRect (API)
*
* Returns the bounding rectangle of the update region, or an empty rectangle
* if there is no update region.  Rectangle is in client-relative coordinates.
*
* Returns TRUE if the update region is non-empty, FALSE if there is no
* update region.
*
* lprc may be NULL to query whether or not an update region exists at all
* or not.
*
* History:
* 16-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

BOOL xxxGetUpdateRect(
    PWND   pwnd,
    LPRECT lprc,
    BOOL   fErase)
{
    RECT rc;

    CheckLock(pwnd);

    if (fErase)
        xxxSimpleDoSyncPaint(pwnd);

    /*
     * The app is looking at the update region: okay to allow window
     * validation.
     */
    ClrWF(pwnd, WFUPDATEDIRTY);

    if (pwnd->hrgnUpdate == NULL) {

        if (lprc) {
            SetRectEmpty(lprc);
        }

        return FALSE;

    } else {

        /*
         * We must handle the case where a window has an update region,
         * but it is completely obscured by its parents.  In this case, we
         * must validate the window and all its children, and return FALSE.
         *
         * An OffsetChildren() call resulting from SetWindowPos() or
         * ScrollWindowEx() will cause this to happen.  Update regions are
         * just offset without checking their new positions to see if they
         * are obscured by the parent(s).  This is too painful to check in
         * those cases, so we instead handle it here.
         *
         * BeginPaint() handles this case correctly by returning an empty
         * rectangle, so nothing special need be done there.
         */
        if (pwnd->hrgnUpdate == HRGN_FULL) {

            rc = pwnd->rcClient;

        } else {

            switch (GreGetRgnBox(pwnd->hrgnUpdate, &rc)) {
            case ERROR:
            case NULLREGION:
                SetRectEmpty(&rc);
                break;

            case SIMPLEREGION:
            case COMPLEXREGION:
                break;
            }

            IntersectRect(&rc, &rc, &pwnd->rcClient);
        }

        if (IntersectWithParents(pwnd, &rc)) {

            if (pwnd != PWNDDESKTOP(pwnd)) {
                OffsetRect(&rc, -pwnd->rcClient.left, -pwnd->rcClient.top);
            }

            /*
             * If the window is CS_OWNDC, then we must map the returned
             * rectangle with DPtoLP, to ensure that the rectangle is
             * in the same coordinate system as the rectangle returned
             * by BeginPaint().
             *
             * BUT ONLY IF hwnd->hrgnUpdate != HRGN_FULL! For true
             * compatibility with 3.0.
             */
            if (TestCF(pwnd, CFOWNDC) &&
                (TestWF(pwnd, WFWIN31COMPAT) || pwnd->hrgnUpdate != HRGN_FULL)) {

                PDCE pdce;

                /*
                 * Look up this window's DC in the cache, and use it to
                 * map the returned rectangle.
                 */
                for (pdce = gpDispInfo->pdceFirst; pdce; pdce = pdce->pdceNext) {

                    if (pdce->pwndOrg == pwnd && !(pdce->DCX_flags & DCX_CACHE)) {
                        GreDPtoLP(pdce->hdc, (LPPOINT)&rc, 2);
                        break;
                    }
                }
            }

        } else {
           SetRectEmpty(&rc);
        }
    }

    if (lprc) {
#ifdef USE_MIRRORING
        if (TestWF(pwnd, WEFLAYOUTRTL)) {
            MirrorRect(pwnd, &rc);
        }
#endif
        *lprc = rc;
    }

    /*
     * If we're in the process a dragging a full window, mark the start
     * of the application painting. This is to make sure that if the
     * application calls DefWindowProc on the WM_PAINT after painting, we
     * won't erase the newly painted areas. Visual Slick calls GetUpdateRect
     * and then DefWindowProc.
     * See other comments for xxxBeginPaint and xxxDWP_Paint.
     * 8/3/94 johannec
     *
     * NOTE: This causes other problems in vslick where some controls
     *       won't paint.  Since the app doesn't call BeginPaint/EndPaint
     *       to truly set/clear the STARTPAINT flag, we do not clear this
     *       bit. (6-27-1996 : ChrisWil).
     *
     *
     *  if (TEST_PUDF(PUDF_DRAGGINGFULLWINDOW)) {
     *      SetWF(pwnd, WFSTARTPAINT);
     *  }
     */

    return TRUE;
}

/***************************************************************************\
* GetUpdateRgn (API)
*
*
* History:
* 16-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

int xxxGetUpdateRgn(
    PWND pwnd,
    HRGN hrgn,
    BOOL fErase)
{
    RECT rc;
    int  code;
    BOOL fNotEmpty;


    CheckLock(pwnd);

    if (fErase)
        xxxSimpleDoSyncPaint(pwnd);

    /*
     * The application is looking at the update region: okay to
     * allow validation
     */
    ClrWF(pwnd, WFUPDATEDIRTY);

    if (pwnd->hrgnUpdate == NULL)
        goto ReturnEmpty;

    rc = pwnd->rcClient;
    fNotEmpty = IntersectWithParents(pwnd, &rc);

    if (pwnd->hrgnUpdate == HRGN_FULL) {

        /*
         * Since the update region may be larger than the window
         * rectangle, intersect it with the window rectangle.
         */
        if (!fNotEmpty)
            goto ReturnEmpty;

        code = SIMPLEREGION;

        /*
         * Normalize the rectangle\region relative to the unclipped window
         */
        if (pwnd != PWNDDESKTOP(pwnd)) {
            OffsetRect(&rc, -pwnd->rcClient.left, -pwnd->rcClient.top);
        }

        SetRectRgnIndirect(hrgn, &rc);

    } else {

        SetRectRgnIndirect(ghrgnInv2, &rc);
        code = IntersectRgn(hrgn, ghrgnInv2, pwnd->hrgnUpdate);

        switch (code) {
        case NULLREGION:
        case ERROR:
            goto ReturnEmpty;

        default:
            if (pwnd != PWNDDESKTOP(pwnd)) {
                GreOffsetRgn(hrgn, -pwnd->rcClient.left, -pwnd->rcClient.top);
            }
            break;
        }
    }
    
#ifdef USE_MIRRORING
    MirrorRegion(pwnd, hrgn, TRUE);
#endif
    /*
     * If we're in the process a dragging a full window, mark the start
     * of the application painting. This is to make sure that if the
     * application calls DefWindowProc on the WM_PAINT after painting, we
     * won't erase the newly painted areas.
     * See other comments for xxxBeginPaint and xxxDWP_Paint.
     * 8/3/94 johannec
     *
     * NOTE: This causes other problems in vslick where some controls
     *       won't paint.  Since the app doesn't call BeginPaint/EndPaint
     *       to truly set/clear the STARTPAINT flag, we do not clear this
     *       bit. (6-27-1996 : ChrisWil).
     *
     *  if (TEST(PUDF(PUDF_DRAGGINGFULLWINDOW)) {
     *      SetWF(pwnd, WFSTARTPAINT);
     *  }
     */

    return code;

ReturnEmpty:
    SetEmptyRgn(hrgn);
    return NULLREGION;
}

/***************************************************************************\
* IntersectWithParents
*
* This routine calculates the intersection of a rectangle with the client
* rectangles of all of pwnd's parents.  Returns FALSE if the intersection
* is empty, a window is invisible, or a parent is minimized.
*
* Stop the intesesection if the window itself or any of its parents are
* layered windows, so we always have a complete bitmap of them.
*
* History:
* 16-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

BOOL IntersectWithParents(
    PWND   pwnd,
    LPRECT lprc)
{
    if (FLayeredOrRedirected(pwnd))
        return TRUE;

    while ((pwnd = pwnd->spwndParent) != NULL) {

        if (!TestWF(pwnd, WFVISIBLE) || TestWF(pwnd, WFMINIMIZED))
            return FALSE;

        if (!IntersectRect(lprc, lprc, &pwnd->rcClient))
            return FALSE;

        if (FLayeredOrRedirected(pwnd))
            return TRUE;
    }

    return TRUE;
}
