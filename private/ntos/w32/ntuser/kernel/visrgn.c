/****************************** Module Header ******************************\
* Module Name: visrgn.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains User's visible region ('visrgn') manipulation
* functions.
*
* History:
* 23-Oct-1990 DarrinM   Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/*
 * Globals used to keep track of pwnds which
 * need to be excluded from the visrgns.
 */
#define CEXCLUDERECTSMAX 30
#define CEXCLUDEPWNDSMAX 30


BOOL  gfVisAlloc;
int   gcrcVisExclude;
int   gcrcVisExcludeMax;
PWND *gapwndVisExclude;
PWND *gapwndVisDefault;

/***************************************************************************\
* SetRectRgnIndirect
*
* Sets a rect region from a rectangle.
*
* History:
* 26-Sep-1996 adams     Created.
\***************************************************************************/

BOOL
SetRectRgnIndirect(HRGN hrgn, LPCRECT lprc)
{
    return GreSetRectRgn(hrgn, lprc->left, lprc->top, lprc->right, lprc->bottom);
}



/***************************************************************************\
* CreateEmptyRgn
*
* Creates an empty region.
*
* History:
* 24-Sep-1996 adams     Created.
\***************************************************************************/

HRGN
CreateEmptyRgn(void)
{
    return GreCreateRectRgnIndirect(PZERO(RECT));
}



/***************************************************************************\
* CreateEmptyRgnPublic
*
* Creates an empty region and make it public.
*
* History:
* 24-Sep-1996 adams     Created.
\***************************************************************************/

HRGN
CreateEmptyRgnPublic(void)
{
    HRGN hrgn;

    if (hrgn = CreateEmptyRgn()) {
        UserVerify(GreSetRegionOwner(hrgn, OBJECT_OWNER_PUBLIC));
    }

    return hrgn;
}



/***************************************************************************\
* SetEmptyRgn
*
* Sets an empty region.
*
* History:
* 26-Sep-1996 adams     Created.
\***************************************************************************/

BOOL
SetEmptyRgn(HRGN hrgn)
{
    return SetRectRgnIndirect(hrgn, PZERO(RECT));
}



/***************************************************************************\
* SetOrCreateRectRgnIndirectPublic
*
* Sets a region to a rectangle, creating it and making it public
* if it is not already there.
*
* History:
* 01-Oct-1996 adams     Created.
\***************************************************************************/

HRGN
SetOrCreateRectRgnIndirectPublic(HRGN * phrgn, LPCRECT lprc)
{
    if (*phrgn) {
        UserVerify(SetRectRgnIndirect(*phrgn, lprc));
    } else if (*phrgn = GreCreateRectRgnIndirect((LPRECT) lprc)) {
        UserVerify(GreSetRegionOwner(*phrgn, OBJECT_OWNER_PUBLIC));
    }

    return *phrgn;
}


/***************************************************************************\
* ResizeVisExcludeMemory
*
*   This routine is used to resize the vis-rgn memory buffer if the count
*   is exceeded.
*
*
* History:
* 22-Oct-1994 ChrisWil  Created
* 27-Feb-1997 adams     Removed call to UserReallocPool, since the pool
*                       allocator doesn't support realloc.
\***************************************************************************/

BOOL ResizeVisExcludeMemory(VOID)
{
    int     crcNew;
    PWND    apwndNew;

    /*
     * Note (adams): a previous version of the code called UserReallocPool
     * if memory had already been allocated. Unfortunately, UserReallocPool
     * just has to allocate more memory and copy the contents, since Rtl
     * doesn't have a realloc function. If Rtl later gains a Realloc function,
     * this code should be changed to the previous version.
     */

    crcNew = gcrcVisExcludeMax + CEXCLUDEPWNDSMAX;
    apwndNew = (PWND)UserAllocPool(
            crcNew * sizeof(PWND), TAG_VISRGN);

    if (!apwndNew)
        return FALSE;

    UserAssert(gcrcVisExcludeMax == gcrcVisExclude);
    RtlCopyMemory(apwndNew, gapwndVisExclude, gcrcVisExcludeMax * sizeof(PWND));
    if (gfVisAlloc) {
        UserFreePool(gapwndVisExclude);
    } else {
        gfVisAlloc = TRUE;
    }

    gcrcVisExcludeMax = crcNew;
    gapwndVisExclude = (PWND *)apwndNew;
    return TRUE;
}



/***************************************************************************\
* ExcludeWindowRects
*   This routine checks to see if the pwnd needs to be added to the list
*   of excluded-clip-rects.  If so, it appends the pwnd to the array.  They
*   do not need to be sorted, since GreSubtractRgnRectList() sorts them
*   internally.
*
*
* History:
* 05-Nov-1992 DavidPe   Created.
* 21-Oct-1994 ChrisWil  Removed pwnd->pwndNextYX.  No longer sorts pwnds.
\***************************************************************************/

#define CheckIntersectRect(prc1, prc2)        \
    (   prc1->left < prc2->right              \
     && prc2->left < prc1->right              \
     && prc1->top < prc2->bottom              \
     && prc2->top < prc1->bottom)

#define EmptyRect(prc)                        \
    (   prc->left >= prc->right               \
     || prc->top >= prc->bottom)

BOOL ExcludeWindowRects(
    PWND   pwnd    ,
    PWND   pwndStop,
    LPRECT lprcIntersect)
{
    PRECT prc;

#if DBG
    if (pwnd != NULL && pwndStop != NULL &&
            pwnd->spwndParent != pwndStop->spwndParent) {
        RIPMSG0(RIP_ERROR, "ExcludeWindowRects: bad windows passed in");
    }
#endif

    while ((pwnd != NULL) && (pwnd != pwndStop)) {
        UserAssert(pwnd);
        prc = &pwnd->rcWindow;
        if (       TestWF(pwnd, WFVISIBLE)
                && !FLayeredOrRedirected(pwnd)
                && (TestWF(pwnd, WEFTRANSPARENT) == 0)
                && CheckIntersectRect(lprcIntersect, prc)
                && !EmptyRect(prc)) {

            UserAssert(gcrcVisExclude <= gcrcVisExcludeMax);
            if (gcrcVisExclude == gcrcVisExcludeMax) {
                if (!ResizeVisExcludeMemory()) {
                    return FALSE;
                }
            }

            gapwndVisExclude[gcrcVisExclude++] = pwnd;
        }

        pwnd = pwnd->spwndNext;
    }

    return TRUE;
}



/***************************************************************************\
* CalcWindowVisRgn
*
*   This routine performs the work of calculating the VisRgn for a window.
*
*
* History:
* 02-Nov-1992 DavidPe   Created.
* 21-Oct-1992 ChrisWil  Removed pwnd->pwndNextYX.  No longer sorts pwnds.
\***************************************************************************/

BOOL CalcWindowVisRgn(
    PWND  pwnd,
    HRGN  *phrgn,
    DWORD flags)
{
    RECT rcWindow;
    PWND pwndParent;
    PWND pwndRoot;
    PWND pwndLoop;
    BOOL fClipSiblings;
    BOOL fRgnParent = FALSE;
    BOOL fResult;
    PWND apwndVisDefault[CEXCLUDEPWNDSMAX];


    /*
     * First get the initial window rectangle which will be used for
     * the basis of exclusion calculations.
     */
    rcWindow = (flags & DCX_WINDOW ? pwnd->rcWindow : pwnd->rcClient);

    /*
     * Get the parent of this window.  We start at the parent and backtrack
     * through the window-parent-list until we reach the end of the parent-
     * list.  This will give us the intersect-rectangle which is used as
     * the basis for checking intersection of the exclusion rects.
     */
    pwndRoot   = pwnd->head.rpdesk->pDeskInfo->spwnd->spwndParent;
    pwndParent = pwnd->spwndParent;

    /*
     * The parent can be NULL in the case when pwnd == pwndRoot. In other
     * cases we should figure why the hell the parent is NULL.
     */
    if (pwndParent == NULL) {
#if DBG
        if (pwnd != pwndRoot) {
            RIPMSG0(RIP_ERROR, "CalcWindowVisRgn: pwndParent is NULL");
        }
#endif
        goto NullRegion;
    }

    while (pwndParent != pwndRoot) {

        /*
         * Don't clip layered DCs to the desktop. The surface of the layered
         * DC is the size of the window and we always want to have the image
         * of the entire window in that surface.
         */
        if ((flags & DCX_LAYERED) && (GETFNID(pwndParent) == FNID_DESKTOP))
            break;

        /*
         * Remember if any of the parents have a window region.
         */
        if (pwndParent->hrgnClip != NULL)
            fRgnParent = TRUE;

        /*
         * Intersect the parent's client rectangle with the window rectangle.
         */
        if (!IntersectRect(&rcWindow, &rcWindow, &pwndParent->rcClient))
            goto NullRegion;

        pwndParent = pwndParent->spwndParent;
    }

    /*
     * Initialize the VisRgn memory-buffer.  This is
     * used to hold the pwnd's.
     */
    gapwndVisDefault  = apwndVisDefault;
    gapwndVisExclude  = gapwndVisDefault;
    gcrcVisExcludeMax = ARRAY_SIZE(apwndVisDefault);
    gcrcVisExclude    = 0;

    /*
     * Build the list of exclude-rects.
     */
    fClipSiblings = (BOOL)(flags & DCX_CLIPSIBLINGS);
    pwndParent    = pwnd->spwndParent;
    pwndLoop      = pwnd;

    while (pwndParent != pwndRoot) {

        /*
         * Don't exclude siblings of a layered window when calculating
         * the virgn of a layered redirection DC.  We must always have
         * a complete image of this window in the redirection bitmap.
         */
        if ((flags & DCX_LAYERED) && FLayeredOrRedirected(pwndLoop)) {
            fClipSiblings = FALSE;
        }

        /*
         * Exclude any siblings if necessary.
         */
        if (fClipSiblings && (pwndParent->spwndChild != pwndLoop)) {

            if (!ExcludeWindowRects(pwndParent->spwndChild,
                                    pwndLoop,
                                    &rcWindow)) {

                goto NullRegion;
            }
        }


        /*
         * Set this flag for next time through the loop...
         */
        fClipSiblings = TestWF(pwndParent, WFCLIPSIBLINGS);

        pwndLoop      = pwndParent;
        pwndParent    = pwndLoop->spwndParent;
    }

    if ((flags & DCX_CLIPCHILDREN) && (pwnd->spwndChild != NULL)) {

        if (!ExcludeWindowRects(pwnd->spwndChild, NULL, &rcWindow)) {
            goto NullRegion;
        }
    }

    /*
     * If there are rectangles to exclude call GDI to create
     * a region excluding them from the window rectangle.  If
     * not simply call GreSetRectRgn().
     */
    if (gcrcVisExclude > 0) {

        RECT  arcVisRects[CEXCLUDERECTSMAX];
        PRECT arcExclude;
        int   i;
        int   ircVisExclude  = 0;
        int   irgnVisExclude = 0;

        /*
         * If we need to exclude more rectangles than fit in
         * the pre-allocated buffer, obviously we have to
         * allocate one that's big enough.
         */

        if (gcrcVisExclude <= CEXCLUDERECTSMAX) {
            arcExclude = arcVisRects;
        } else {
            arcExclude = (PRECT)UserAllocPoolWithQuota(
                    sizeof(RECT) * gcrcVisExclude, TAG_VISRGN);

            if (!arcExclude)
                goto NullRegion;
        }

        /*
         * Now run through the list and put the
         * window rectangles into the array for the call
         * to CombineRgnRectList().
         */
        for (i = 0; i < gcrcVisExclude; i++) {

            /*
             * If the window has a clip-rgn associated with
             * it, then re-use gpwneExcludeList[] entries for
             * storing them.
             */
            if (gapwndVisExclude[i]->hrgnClip != NULL) {

                gapwndVisExclude[irgnVisExclude++] = gapwndVisExclude[i];
                continue;
            }

            /*
             * This window doesn't have a clipping region; remember its
             * rect for clipping purposes.
             */
            arcExclude[ircVisExclude++] = gapwndVisExclude[i]->rcWindow;
        }

        if (*phrgn == NULL)
            *phrgn = CreateEmptyRgn();

        if (ircVisExclude != 0) {
            GreSubtractRgnRectList(*phrgn,
                                   &rcWindow,
                                   arcExclude,
                                   ircVisExclude);
        } else {
            SetRectRgnIndirect(*phrgn, &rcWindow);
        }

        for (i = 0; i < irgnVisExclude; i++) {

            SetRectRgnIndirect(ghrgnInv2, &gapwndVisExclude[i]->rcWindow);
            IntersectRgn(ghrgnInv2, ghrgnInv2, gapwndVisExclude[i]->hrgnClip);

            if (SubtractRgn(*phrgn, *phrgn, ghrgnInv2) == NULLREGION)
                break;
        }

        if (arcExclude != arcVisRects) {
            UserFreePool((HLOCAL)arcExclude);
        }

    } else {

        /*
         * If the window was somehow destroyed, then we will return
         * a null-region.  Emptying the rect will accomplish this.
         */
        if (TestWF(pwnd, WFDESTROYED)) {
            SetRectEmpty(&rcWindow);
        }

        /*
         * If there weren't any rectangles to exclude, simply call
         * GreSetRectRgn() with the window rectangle.
         */
        SetOrCreateRectRgnIndirectPublic(phrgn, &rcWindow);
    }

    /*
     * Clip out this window's window region
     */
    if (pwnd->hrgnClip != NULL) {
        IntersectRgn(*phrgn, *phrgn, pwnd->hrgnClip);
    }

    /*
     * Clip out parent's window regions, if there are any.
     */
    if (fRgnParent) {

        PWND pwndT;

        for (pwndT = pwnd->spwndParent;
                pwndT != pwndRoot;
                pwndT = pwndT->spwndParent) {

            if (pwndT->hrgnClip != NULL) {

                if (IntersectRgn(*phrgn, *phrgn, pwndT->hrgnClip) == NULLREGION)
                    break;
            }
        }
    }

    fResult = TRUE;
    // fall-through

Cleanup:
    if (gfVisAlloc) {
        UserFreePool((HLOCAL)gapwndVisExclude);
        gfVisAlloc = FALSE;
    }

    return fResult;

NullRegion:
    SetOrCreateRectRgnIndirectPublic(phrgn, PZERO(RECT));
    fResult = FALSE;
    goto Cleanup;
}

/***************************************************************************\
* CalcVisRgn
*
* Will return FALSE if the pwndOrg is not visible, TRUE otherwise.
*
* History:
* 17-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

BOOL CalcVisRgn(
    HRGN  *phrgn,
    PWND  pwndOrg,
    PWND  pwndClip,
    DWORD flags)
{
    PDESKTOP    pdesk;

    UserAssert(pwndOrg != NULL);

    /*
     * If the window's not visible or is not an active desktop,
     * or if the clip window is in the process of being destroyed,
     * the visrgn is empty.
     */
    pdesk = pwndOrg->head.rpdesk;
    
    UserAssert(pdesk);
    
    /*
     * Make sure this happens in the IO windowstation
     */
#if DBG
    if (grpdeskRitInput != NULL) {
        UserAssert(pdesk->rpwinstaParent == grpdeskRitInput->rpwinstaParent ||
                   !IsVisible(pwndOrg));
    }
#endif // DBG

    if (!IsVisible(pwndOrg) || pdesk != grpdeskRitInput) {
        goto EmptyRgn;
    }

    /*
     * If LockWindowUpdate() has been called, and this window is a child
     * of the lock window, always return an empty visrgn.
     */
    if ((gspwndLockUpdate != NULL)     &&
        !(flags & DCX_LOCKWINDOWUPDATE)         &&
        _IsDescendant(gspwndLockUpdate, pwndOrg)) {

        goto EmptyRgn;
    }

    /*
     * Now go compute the visrgn for pwndClip.
     */
    return CalcWindowVisRgn(pwndClip, phrgn, flags);

EmptyRgn:
    SetOrCreateRectRgnIndirectPublic(phrgn, PZERO(RECT));
    return FALSE;
}

/***************************************************************************\
* CalcWindowRgn
*
*
* History:
* 17-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

int CalcWindowRgn(
    PWND pwnd,
    HRGN hrgn,
    BOOL fClient)
{
    SetRectRgnIndirect(hrgn, (fClient) ? &pwnd->rcClient : &pwnd->rcWindow);

    /*
     * If the window has a region, then intersect the rectangle region with
     * that. If this is low on memory, it'll propagate ERROR back.
     */
    if (pwnd->hrgnClip != NULL) {
        return IntersectRgn(hrgn, hrgn, pwnd->hrgnClip);
    }

    return SIMPLEREGION;
}
