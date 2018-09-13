/****************************** Module Header ******************************\
* Module Name: dc.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains User's DC APIs and related functions.
*
* History:
* 23-Oct-1990 DarrinM   Created.
* 07-Feb-1991 MikeKe    Added Revalidation code (None).
* 17-Jul-1991 DarrinM   Recreated from Win 3.1 source.
* 21-Jan-1992 IanJa     ANSI/Unicode neutral (null op).
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/*
 * DBG Related Information.
 */
#if DBG
BOOL fDisableCache;                 // TRUE to disable DC cache.
#endif

/***************************************************************************\
* DecrementFreeDCECount
*
\***************************************************************************/

__inline VOID DecrementFreeDCECount(VOID)
{
    gnDCECount--;
    UserAssert(gnDCECount >= 0);
}

/***************************************************************************\
* IncrementFreeDCECount
*
\***************************************************************************/

__inline VOID IncrementFreeDCECount(VOID)
{
    gnDCECount++;
    UserAssert(gnDCECount >= 0);
}

/***************************************************************************\
* SetMonitorRegion
*
* The region is in meta dc coordinates, so convert to monitor coords.
\***************************************************************************/

void SetMonitorRegion(PMONITOR pMonitor, HRGN hrgnDst, HRGN hrgnSrc)
{
    if (IntersectRgn(hrgnDst, hrgnSrc, pMonitor->hrgnMonitor) == ERROR) {
        GreSetRectRgn(hrgnDst, 0, 0, 0, 0);
        return;
    }

    GreOffsetRgn(hrgnDst, -pMonitor->rcMonitor.left, -pMonitor->rcMonitor.top);
}

/***************************************************************************\
* ResetOrg
*
* Resets the origin of the DC associated with *pdce, and selects
* a new visrgn.
*
* History:
* 17-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

VOID ResetOrg(
    HRGN hrgn,
    PDCE pdce,
    BOOL fSetVisRgn)
{
    RECT  rc;
    PWND  pwndLayer;

    /*
     * For compatibility purposes, make sure that the DC's for the
     * desktop windows originate at the primary monitor, i.e. (0,0).
     */
    if (GETFNID(pdce->pwndOrg) == FNID_DESKTOP) {
        rc.left = rc.top = 0;
        rc.right = SYSMET(CXVIRTUALSCREEN);
        rc.bottom = SYSMET(CYVIRTUALSCREEN);
    } else if (pdce->DCX_flags & DCX_WINDOW) {
        rc = pdce->pwndOrg->rcWindow;
    } else {
        rc = pdce->pwndOrg->rcClient;
    }

    if (pdce->pMonitor != NULL) {
        OffsetRect(&rc, -pdce->pMonitor->rcMonitor.left,
                -pdce->pMonitor->rcMonitor.top);

        if (hrgn != NULL) {
            SetMonitorRegion(pdce->pMonitor, hrgn, hrgn);
        }
    }

    if ((pwndLayer = GetLayeredWindow(pdce->pwndOrg)) != NULL) {
        if (pdce->DCX_flags & DCX_LAYERED) {
            int x = pwndLayer->rcWindow.left;
            int y = pwndLayer->rcWindow.top;

            /*
             * For layered redirection DCs, the surface origin is the
             * window origin, so offset both the rectangle and the
             * region appropriately.
             */
            OffsetRect(&rc, -x, -y);
            if (hrgn != NULL) {
                GreOffsetRgn(hrgn, -x, -y);
            }
        } else {
            /*
             * Layered windows can only draw to the screen via the redirection
             * DCs or UpdateLayeredWindow, so select an empty visrgn into this
             * screen DC.
             */
            if (hrgn != NULL) {
                GreSetRectRgn(hrgn, 0, 0, 0, 0);
            }
        }
    } else {
        UserAssert(!(pdce->DCX_flags & DCX_LAYERED));
    }

    GreSetDCOrg(pdce->hdc, rc.left, rc.top, (PRECTL)&rc);

    if (fSetVisRgn) {
        GreSelectVisRgn(pdce->hdc, hrgn, SVR_DELETEOLD);
    }
}

/***************************************************************************\
* GetDC (API)
*
* Standard call to GetDC().
*
* History:
* 17-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

HDC _GetDC(
    PWND pwnd)
{
    /*
     * Special case for NULL: For backward compatibility we want to return
     * a window DC for the desktop that does not exclude its children.
     */
    if (pwnd == NULL) {

        PDESKTOP pdesk = PtiCurrent()->rpdesk;

        if (pdesk) {
            return _GetDCEx(pdesk->pDeskInfo->spwnd,
                            NULL,
                            DCX_WINDOW | DCX_CACHE);
        }

        /*
         * The thread has no desktop.  Fail the call.
         */
        return NULL;
    }

    return _GetDCEx(pwnd, NULL, DCX_USESTYLE);
}

/***************************************************************************\
* _ReleaseDC (API)
*
* Release the DC retrieved from GetDC().
*
* History:
* 17-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

BOOL _ReleaseDC(
    HDC hdc)
{
    CheckCritIn();

    return (ReleaseCacheDC(hdc, FALSE) == DCE_NORELEASE ? FALSE : TRUE);
}

/***************************************************************************\
* _GetWindowDC (API)
*
* Retrive a DC for the window.
*
* History:
* 17-Jul-1991 DarrinM   Ported from Win 3.1 sources.
* 25-Jan-1996 ChrisWil  Allow rgnClip so that WM_NCACTIVATE can clip.
\***************************************************************************/

HDC _GetWindowDC(
    PWND pwnd)
{

#if 0

    /*
     * For WIN31 and previous apps, we want to actually return back a
     * client DC.  Before WIN40, the window rect and client rect were the
     * same, and there was this terrible hack to grab the window dc when
     * painting because window DCs never clip anything.  Otherwise the
     * children of the minimized window would be clipped out of the fake
     * client area.  So apps would call GetWindowDC() to redraw their icons,
     * since GetDC() would clip empty if the window had a class icon.
     */
    if (TestWF(pwnd, WFMINIMIZED) && !TestWF(pwnd, WFWIN40COMPAT))
        return(_GetDCEx(pwnd, hrgnClip, DCX_INTERNAL | DCX_CACHE | DCX_USESTYLE));
#endif

    return _GetDCEx(pwnd, NULL, DCX_WINDOW | DCX_USESTYLE);
}

/***************************************************************************\
* UserSetDCVisRgn
*
* Set the visrgn for the DCE.  If the window has a (hrgnClipPublic), we use
* that instead of the (hrgnClip) since it's a public-object.  The other is
* created and owned by the user-thread and can't be used if say we're in the
* hung-app-drawing (different process).  Both regions should be equalent in
* data.
*
* History:
* 10-Nov-1992 DavidPe   Created.
* 20-Dec-1995 ChrisWil  Added (hrgnClipPublic) entry.
\***************************************************************************/

VOID UserSetDCVisRgn(
    PDCE pdce)
{
    HRGN hrgn = NULL;

    /*
     * If the visrgn calculated is empt, set the flag DCX_PWNDORGINVISIBLE,
     * otherwise clear it (it could've been set earlier on).
     */
    if (!CalcVisRgn(&hrgn, pdce->pwndOrg, pdce->pwndClip, pdce->DCX_flags)) {
        pdce->DCX_flags |= DCX_PWNDORGINVISIBLE;
    } else {
        pdce->DCX_flags &= ~DCX_PWNDORGINVISIBLE;
    }

    /*
     * Deal with INTERSECTRGN and EXCLUDERGN.
     */
    if (pdce->DCX_flags & DCX_INTERSECTRGN) {

        UserAssert(pdce->hrgnClipPublic != HRGN_FULL);

        if (pdce->hrgnClipPublic == NULL) {
            SetEmptyRgn(hrgn);
        } else {
            IntersectRgn(hrgn, hrgn, pdce->hrgnClipPublic);
        }

    } else if (pdce->DCX_flags & DCX_EXCLUDERGN) {

        UserAssert(pdce->hrgnClipPublic != NULL);

        if (pdce->hrgnClipPublic == HRGN_FULL) {
            SetEmptyRgn(hrgn);
        } else {
            SubtractRgn(hrgn, hrgn, pdce->hrgnClipPublic);
        }
    }

    ResetOrg(hrgn, pdce, TRUE);
}

/***************************************************************************\
* UserGetClientRgn
*
* Return a copy of the client region and rectangle for the given hwnd.
*
* The caller must enter the user critical section before calling this function.
*
* History:
* 27-Sep-1993 WendyWu   Created.
\***************************************************************************/

HRGN UserGetClientRgn(
    HWND   hwnd,
    LPRECT lprc,
    BOOL   bWindowInsteadOfClient)
{
    HRGN hrgnClient = (HRGN)NULL;
    PWND pwnd;

    /*
     * Must be in critical section.
     */
    CheckCritIn();

    if (pwnd = ValidateHwnd(hwnd)) {

        if (bWindowInsteadOfClient) {

            /*
             * Never clip children for WO_RGN_WINDOW so that NetMeeting
             * gets the unioned window area:
             */

            CalcVisRgn(&hrgnClient,
                       pwnd,
                       pwnd,
                       DCX_WINDOW |
                       (TestWF(pwnd, WFCLIPSIBLINGS) ? DCX_CLIPSIBLINGS : 0));
        } else {
            CalcVisRgn(&hrgnClient,
                       pwnd,
                       pwnd,
                       DCX_CLIPCHILDREN | DCX_CLIPSIBLINGS);
        }

        *lprc = pwnd->rcClient;
    }

    return hrgnClient;
}

/***************************************************************************\
* UserGetHwnd
*
* Return a hwnd and the associated pwo for the given display hdc.
*
* It returns FALSE if no hwnd corresponds to the hdc is found or if the
* hwnd has incorrect styles for a device format window.
*
* The caller must enter the user critical section before calling this function.
*
* History:
* 27-Sep-1993 WendyWu   Created.
\***************************************************************************/

BOOL UserGetHwnd(
    HDC   hdc,
    HWND  *phwnd,
    PVOID *ppwo,
    BOOL  bCheckStyle)
{
    PWND pwnd;
    PDCE pdce;

    /*
     * Must be in critical section.
     */
    CheckCritIn();

    /*
     * Find pdce and pwnd for this DC.
     *
     * Note: the SAMEHANDLE macro strips out the user defined bits in the
     * handle before doing the comparison.  This is important because when
     * GRE calls this function, it may have lost track of the OWNDC bit.
     */
    for (pdce = gpDispInfo->pdceFirst; pdce != NULL; pdce = pdce->pdceNext) {

        if (pdce->hdc == hdc) // this should be undone once SAMEHANDLE is fixed for kmode
            break;
    }

    /*
     * Return FALSE If it is not in the pdce list.
     */
    if ((pdce == NULL) || (pdce->pwndOrg == NULL))
        return FALSE;

    pwnd = pdce->pwndOrg;

    /*
     * The window style must be clipchildren and clipsiblings.
     * the window's class must not be parentdc
     */
    if (bCheckStyle) {

        if (    !TestWF(pwnd, WFCLIPCHILDREN) ||
                !TestWF(pwnd, WFCLIPSIBLINGS) ||
                TestCF(pwnd, CFPARENTDC)) {

            RIPMSG0(RIP_WARNING, "UserGetHwnd: Bad OpenGL window style or class");
            return FALSE;
        }
    }

    /*
     * Return the hwnd with the correct styles for a device format window.
     */
    *phwnd = HW(pwnd);
    *ppwo  = _GetProp(pwnd, PROP_WNDOBJ, TRUE);

    return TRUE;
}

/***************************************************************************\
* UserAssociateHwnd
*
* Associate a gdi WNDOBJ with hwnd.  The caller must enter the user
* critical section before calling this function.
*
* If 'pwo' is NULL, the association is removed.
*
* History:
* 13-Jan-1994 HockL     Created.
\***************************************************************************/

VOID UserAssociateHwnd(
    HWND  hwnd,
    PVOID pwo)
{
    PWND pwnd;

    /*
     * Must be in critical section.
     */
    CheckCritIn();

    if (pwnd = ValidateHwnd(hwnd)) {

        if (pwo != NULL) {
            if (InternalSetProp(pwnd, PROP_WNDOBJ, pwo, PROPF_INTERNAL | PROPF_NOPOOL))
                gcountPWO++;
        } else {
            if (InternalRemoveProp(pwnd, PROP_WNDOBJ, TRUE))
                gcountPWO--;
        }
    }
}

/***************************************************************************\
* UserReleaseDC
*
* Enter's the critical section and calls _ReleaseDC.
*
* History:
* 25-Jan-1996 ChrisWil  Created comment block.
\***************************************************************************/

BOOL UserReleaseDC(
    HDC hdc)
{
    BOOL b;

    EnterCrit();
    b = _ReleaseDC(hdc);
    LeaveCrit();

    return b;
}

/***************************************************************************\
* InvalidateDce
*
* If the DCE is not in use, removes all information and marks it invalid.
* Otherwise, it resets the DCE flags based on the window styles and
* recalculates the vis rgn.
*
* History:
* 17-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

VOID InvalidateDce(
    PDCE pdce)
{
    GreLockDisplay(gpDispInfo->hDev);

    if (!(pdce->DCX_flags & DCX_INUSE)) {

        /*
         * Accumulate any bounds for this CE
         * since we're about to mark it invalid.
         */
        SpbCheckDce(pdce);

        MarkDCEInvalid(pdce);

        pdce->pwndOrg        = NULL;
        pdce->pwndClip       = NULL;
        pdce->hrgnClip       = NULL;
        pdce->hrgnClipPublic = NULL;

        /*
         * Remove the vis rgn since it is still owned - if we did not,
         * gdi would not be able to clean up properly if the app that
         * owns this vis rgn exist while the vis rgn is still selected.
         */
        GreSelectVisRgn(pdce->hdc, NULL, SVR_DELETEOLD);

    } else {

        PWND pwndOrg  = pdce->pwndOrg;
        PWND pwndClip = pdce->pwndClip;

        /*
         * In case the window's clipping style bits changed,
         * reset the DCE flags from the window style bits.
         * Note that minimized windows never exclude their children.
         */
        pdce->DCX_flags &= ~(DCX_CLIPCHILDREN | DCX_CLIPSIBLINGS);

        /*
         * Chicago stuff...
         */
        if (TestCF(pwndOrg, CFPARENTDC) &&
            (TestWF(pwndOrg, WFWIN31COMPAT) || !TestWF(pwndClip, WFCLIPCHILDREN)) &&
            (TestWF(pwndOrg, WFVISIBLE) == TestWF(pwndClip, WFVISIBLE))) {

            if (TestWF(pwndClip, WFCLIPSIBLINGS))
                pdce->DCX_flags |= DCX_CLIPSIBLINGS;

        } else {

            if (TestWF(pwndOrg, WFCLIPCHILDREN) && !TestWF(pwndOrg, WFMINIMIZED))
                pdce->DCX_flags |= DCX_CLIPCHILDREN;

            if (TestWF(pwndOrg, WFCLIPSIBLINGS))
                pdce->DCX_flags |= DCX_CLIPSIBLINGS;
        }

        /*
         * Mark that any saved visrgn needs to be recomputed.
         */
        pdce->DCX_flags |= DCX_SAVEDRGNINVALID;

        UserSetDCVisRgn(pdce);
    }

    GreUnlockDisplay(gpDispInfo->hDev);
}

/***************************************************************************\
* DeleteHrgnClip
*
* Deletes the clipping regions in the DCE, restores the saved visrgn,
* and invalidates the DCE if saved visrgn is invalid.
*
* History:
* 17-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

VOID DeleteHrgnClip(
    PDCE pdce)
{
    /*
     * Clear these flags first in case we get a DCHook() callback...
     */
    pdce->DCX_flags &= ~(DCX_EXCLUDERGN | DCX_INTERSECTRGN);

    /*
     * Blow away pdce->hrgnClip and clear the associated flags.
     * Do not delete hrgnClip if DCX_NODELETERGN is set!
     */
    if (!(pdce->DCX_flags & DCX_NODELETERGN)) {
        DeleteMaybeSpecialRgn(pdce->hrgnClip);
    } else {
        pdce->DCX_flags &= ~DCX_NODELETERGN;
    }

    DeleteMaybeSpecialRgn(pdce->hrgnClipPublic);

    pdce->hrgnClip       = NULL;
    pdce->hrgnClipPublic = NULL;

    /*
     * If the saved visrgn was invalidated by an InvalidateDce()
     * while we had it checked out, then invalidate the entry now.
     */
    if (pdce->DCX_flags & DCX_SAVEDRGNINVALID) {
        InvalidateDce(pdce);

        /*
         * We've just gone through InvalidateDce, so the visrgn in the
         * DC has been properly reset. Simply nuke the old saved visrgn.
         */
        if (pdce->hrgnSavedVis != NULL) {
            GreDeleteObject(pdce->hrgnSavedVis);
            pdce->hrgnSavedVis = NULL;
        }
    } else {
        /*
         * The saved visrgn is still valid, select it back into the
         * DC so the entry may be re-used without recomputing.
         */
        if (pdce->hrgnSavedVis != NULL) {
            GreSelectVisRgn(pdce->hdc, pdce->hrgnSavedVis, SVR_DELETEOLD);
            pdce->hrgnSavedVis = NULL;
        }
    }
}

/***************************************************************************\
* GetDCEx (API)
*
*
* History:
* 17-Jul-1991 DarrinM   Ported from Win 3.1 sources.
* 20-Dec-1995 ChrisWil  Added (hrgnClipPublic) entry.
\***************************************************************************/

HDC _GetDCEx(
    PWND  pwnd,
    HRGN  hrgnClip,
    DWORD DCX_flags)
{
    HRGN  hrgn;
    HDC   hdcMatch;
    PWND  pwndClip;
    PWND  pwndOrg;
    PDCE  pdce;
    PDCE  *ppdce;
    PDCE  *ppdceNotInUse;
    DWORD DCX_flagsMatch;
    BOOL  bpwndOrgVisible;
    PWND  pwndLayer;
    HBITMAP hbmLayer;
    BOOL  fVisRgnError = FALSE;

    /*
     * Lock the device while we're playing with visrgns.
     */
    GreLockDisplay(gpDispInfo->hDev);

    if (pwnd == NULL)
        pwnd = PtiCurrent()->rpdesk->pDeskInfo->spwnd;

    hdcMatch = NULL;
    pwndOrg  = pwndClip = pwnd;

    bpwndOrgVisible = IsVisible(pwndOrg);

    if (PpiCurrent()->W32PF_Flags & W32PF_OWNDCCLEANUP) {
        DelayedDestroyCacheDC();
    }

    /*
     * If necessary, compute DCX flags from window style.
     */
    if (DCX_flags & DCX_USESTYLE) {

        DCX_flags &= ~(DCX_CLIPSIBLINGS | DCX_CLIPCHILDREN | DCX_PARENTCLIP);

        if (!(DCX_flags & DCX_WINDOW)) {

            if (TestCF(pwndOrg, CFPARENTDC))
                DCX_flags |= DCX_PARENTCLIP;

            /*
             * If the DCX_CACHE flag is present, override OWNDC/CLASSDC.
             * Otherwise, calculate from appropriate style bits.
             */
            if (!(DCX_flags & DCX_CACHE) && !TestCF(pwndOrg, CFOWNDC)) {
                if (TestCF(pwndOrg, CFCLASSDC)) {
                    /*
                     * Look for a non-cache entry that matches hdc...
                     */
                    if (pwndOrg->pcls->pdce != NULL) {
                        hdcMatch = pwndOrg->pcls->pdce->hdc;
                    }
                } else {
                    DCX_flags |= DCX_CACHE;
                }
            }

            if (TestWF(pwndOrg, WFCLIPCHILDREN))
                DCX_flags |= DCX_CLIPCHILDREN;

            if (TestWF(pwndOrg, WFCLIPSIBLINGS))
                DCX_flags |= DCX_CLIPSIBLINGS;

            /*
             * Minimized windows never exclude their children.
             */
            if (TestWF(pwndOrg, WFMINIMIZED)) {
                DCX_flags &= ~DCX_CLIPCHILDREN;

                if (pwndOrg->pcls->spicn)
                    DCX_flags |= DCX_CACHE;
            }

        } else {
            if (TestWF(pwndClip, WFCLIPSIBLINGS))
                DCX_flags |= DCX_CLIPSIBLINGS;

            DCX_flags |= DCX_CACHE;

            /*
             * Window DCs never exclude children.
             */
        }
    }

    /*
     * Deal with all the Win 3.0-compatible clipping rules:
     *
     * DCX_NOCLIPCHILDREN overrides:
     *      DCX_PARENTCLIP/CS_OWNDC/CS_CLASSDC
     * DCX_PARENTCLIP overrides:
     *      DCX_CLIPSIBLINGS/DCX_CLIPCHILDREN/CS_OWNDC/CS_CLASSDC
     */
    if (DCX_flags & DCX_NOCLIPCHILDREN) {
        DCX_flags &= ~(DCX_PARENTCLIP | DCX_CLIPCHILDREN);
        DCX_flags |= DCX_CACHE;
    }

    /*
     * Deal with layered windows.
     */
    if ((pwndLayer = GetLayeredWindow(pwndOrg)) != NULL &&
            (hbmLayer = _GetProp(pwndLayer, PROP_LAYER, TRUE)) != NULL) {

        /*
         * Get a layered redirection DC.
         */
        DCX_flags |= DCX_LAYERED;

        /*
         * When the window we're getting the DC for is the layered and
         * redirected window, don't allow to clip to its parent, since
         * clipping must not exceed the size of the backing bitmap.
         */
        if (pwndOrg == pwndLayer) {
            DCX_flags &= ~DCX_PARENTCLIP;
        }

        /*
         * Convert hrgnClip from screen to the redirection DC coordinates.
         */
        if (hrgnClip > HRGN_SPECIAL_LAST) {
            GreOffsetRgn(hrgnClip, -pwndLayer->rcWindow.left,
                    -pwndLayer->rcWindow.top);
        }
    } else {
        pwndLayer = NULL;
        hbmLayer = NULL;
    }

    if (DCX_flags & DCX_PARENTCLIP) {

        PWND pwndParent;

        /*
         * If this window has no parent.  This can occur if the app is
         * calling GetDC in response to a CBT_CREATEWND callback.  In this
         * case, the parent is not yet setup.
         */
        if (pwndOrg->spwndParent == NULL)
            pwndParent = PtiCurrent()->rpdesk->pDeskInfo->spwnd;
        else
            pwndParent = pwndOrg->spwndParent;

        /*
         * Always get the DC from the cache.
         */
        DCX_flags |= DCX_CACHE;

        /*
         * We can't use a shared DC if the visibility of the
         * child does not match the parent's, or if a
         * CLIPSIBLINGS or CLIPCHILDREN DC is requested.
         *
         * In 3.1, we pay attention to the CLIPSIBLINGS and CLIPCHILDREN
         * bits of CS_PARENTDC windows, by overriding CS_PARENTDC if
         * either of these flags are requested.
         *
         * BACKWARD COMPATIBILITY HACK
         *
         * If parent is CLIPCHILDREN, get a cache DC, but don't
         * use parent's DC.  Windows PowerPoint depends on this
         * behavior in order to draw the little gray rect between
         * its scroll bars correctly.
         */
        if (!(DCX_flags & (DCX_CLIPSIBLINGS | DCX_CLIPCHILDREN)) &&
                (TestWF(pwndOrg, WFWIN31COMPAT) || !TestWF(pwndParent, WFCLIPCHILDREN)) &&
                TestWF(pwndParent, WFVISIBLE) == TestWF(pwndOrg, WFVISIBLE)) {

            pwndClip = pwndParent;

#if DBG
            if (DCX_flags & DCX_CLIPCHILDREN)
                RIPMSG0(RIP_WARNING, "WS_CLIPCHILDREN overridden by CS_PARENTDC");
            if (DCX_flags & DCX_CLIPSIBLINGS)
                RIPMSG0(RIP_WARNING, "WS_CLIPSIBLINGS overridden by CS_PARENTDC");
#endif
            /*
             * Make sure flags reflect hwndClip rather than hwndOrg.
             * But, we must never clip the children (since that's who
             * wants to do the drawing!)
             */
            DCX_flags &= ~(DCX_CLIPCHILDREN | DCX_CLIPSIBLINGS);
            if (TestWF(pwndClip, WFCLIPSIBLINGS))
                DCX_flags |= DCX_CLIPSIBLINGS;
        }
    }

    /*
     * Make sure we don't return an OWNDC if the calling thread didn't
     * create this window - need to returned cached always in this case.
     *
     * Win95 does not contain this code.  Why?
     */
    if (!(DCX_flags & DCX_CACHE)) {
        if (pwndOrg == NULL || GETPTI(pwndOrg) != PtiCurrent())
            DCX_flags |= DCX_CACHE;
    }

    DCX_flagsMatch = DCX_flags & DCX_MATCHMASK;

    if (!(DCX_flags & DCX_CACHE)) {

        /*
         * Handle CS_OWNDC and CS_CLASSDC cases specially.  Based on the
         * supplied match information, we need to find the appropriate DCE.
         */
        for (ppdce = &gpDispInfo->pdceFirst; (pdce = *ppdce); ppdce = &pdce->pdceNext) {

            if (pdce->DCX_flags & DCX_CACHE)
                continue;

            /*
             * Look for the entry that matches hdcMatch or pwndOrg...
             */
            if (!(pdce->pwndOrg == pwndOrg || pdce->hdc == hdcMatch))
                continue;

            /*
             * NOTE: The "Multiple-BeginPaint()-of-OWNDC-Window" Conundrum
             *
             * There is a situation having to do with OWNDC or CLASSDC window
             * DCs that can theoretically arise that is handled specially
             * here and in ReleaseCacheDC().  These DCs are identified with
             * the DCX_CACHE bit CLEAR.
             *
             * In the case where BeginPaint() (or a similar operation) is
             * called more than once without an intervening EndPaint(), the
             * DCX_INTERSECTRGN (or DCX_EXCLUDERGN) bit may already be set
             * when we get here.
             *
             * Theoretically, the correct thing to do is to save the current
             * hrgnClip, and set up the new one here.  In ReleaseCacheDC, the
             * saved hrgnClip is restored and the visrgn recomputed.
             *
             * All of this is only necessary if BOTH calls involve an
             * hrgnClip that causes the visrgn to be changed (i.e., the
             * simple hrgnClip test clears the INTERSECTRGN or EXCLUDERGN bit
             * fails), which is not at all likely.
             *
             * When this code encounters this multiple-BeginPaint case it
             * punts by honoring the new EXCLUDE/INTERSECTRGN bits, but it
             * first restores the DC to a wide-open visrgn before doing so.
             * This means that the first EndPaint() will restore the visrgn
             * to a wide-open DC, rather than clipped to the first
             * BeginPaint()'s update rgn.  This is a good punt, because worst
             * case an app does a bit more drawing than it should.
             */
            if ((pdce->DCX_flags & (DCX_EXCLUDERGN | DCX_INTERSECTRGN)) &&
                    (DCX_flags & (DCX_EXCLUDERGN | DCX_INTERSECTRGN))) {

                RIPMSG0(RIP_WARNING, "Nested BeginPaint() calls, please fix Your app!");
                DeleteHrgnClip(pdce);
            }

            if (pdce->DCX_flags & DCX_LAYERED) {
                /*
                 * We're giving out the same DC again. Since it may not have
                 * been released, transfer any accumulated bits if needed.
                 */
                UpdateLayeredSprite(pdce);
            
                /*
                 * As this point, the DC may get converted back to a screen
                 * DC, so we must select the screen surface back into the DC.
                 */
                UserVerify(GreSelectRedirectionBitmap(pdce->hdc, NULL));
            }

            /*
             * If we matched exactly, no recomputation necessary
             * (we found a CS_OWNDC or a CS_CLASSDC that is already set up)
             * Otherwise, we have a CS_CLASSDC that needs recomputation.
             */
            if (    pdce->pwndOrg == pwndOrg &&
                    bpwndOrgVisible &&
                    (pdce->DCX_flags & DCX_LAYERED) == (DCX_flags & DCX_LAYERED) &&
                    !(pdce->DCX_flags & DCX_PWNDORGINVISIBLE)) {

                goto HaveComputedEntry;
            }

            goto RecomputeEntry;
        }

        RIPMSG1(RIP_WARNING, "Couldn't find DC for %p - bad code path", pwndOrg);

NullExit:

        GreUnlockDisplay(gpDispInfo->hDev);
        return NULL;

    } else {

        /*
         * Make a quick pass through the cache, looking for an
         * exact match.
         */
SearchAgain:

#if DBG
        if (fDisableCache)
            goto SearchFailed;
#endif

        /*
         * CONSIDER (adams): Put this check into the loop above so we don't
         * touch all these pages twice?
         */
        for (ppdce = &gpDispInfo->pdceFirst; (pdce = *ppdce); ppdce = &pdce->pdceNext) {

            /*
             * If we find an entry that is not in use and whose clip flags
             * and clip window match, we can use it.
             *
             * NOTE: DCX_INTERSECT/EXCLUDERGN cache entries always have
             * DCX_INUSE set, so we'll never erroneously match one here.
             */
            UserAssert(!(pdce->DCX_flags & (DCX_INTERSECTRGN | DCX_EXCLUDERGN)) ||
                       (pdce->DCX_flags & DCX_INUSE));

            if ((pdce->pwndClip == pwndClip) &&
                pdce->pMonitor == NULL &&
                (DCX_flagsMatch == (pdce->DCX_flags & (DCX_MATCHMASK | DCX_INUSE | DCX_INVALID)))) {

                /*
                 * Special case for icon - bug 9103 (win31)
                 */
                if (TestWF(pwndClip, WFMINIMIZED) &&
                    (pdce->pwndOrg != pdce->pwndClip)) {
                    continue;
                }

                /*
                 * If the pwndOrg of the DC we found is not visible and
                 * the pwndOrg we're looking for is visble, then
                 * the visrgn is no good, we can't reuse it so keep
                 * looking.
                 */
                if (bpwndOrgVisible && pdce->DCX_flags & DCX_PWNDORGINVISIBLE) {
                    continue;
                }

                /*
                 * Set INUSE before performing any GDI operations, just
                 * in case DCHook() has a mind to recalculate the visrgn...
                 */
                pdce->DCX_flags |= DCX_INUSE;

                /*
                 * We found an entry with the proper visrgn.
                 * If the origin doesn't match, update the CE and reset it.
                 */
                if (pwndOrg != pdce->pwndOrg) {
                    /*
                     * Need to flush any dirty rectangle stuff now.
                     */
                    SpbCheckDce(pdce);

                    pdce->pwndOrg = pwndOrg;
                    ResetOrg(NULL, pdce, FALSE);
                }

                goto HaveComputedEntry;
            }
        }

#if DBG
SearchFailed:
#endif

        /*
         * Couldn't find an exact match.  Find some invalid or non-inuse
         * entry we can reuse.
         */
        ppdceNotInUse = NULL;
        for (ppdce = &gpDispInfo->pdceFirst; (pdce = *ppdce); ppdce = &pdce->pdceNext) {

            /*
             * Skip non-cache entries
             */
            if (!(pdce->DCX_flags & DCX_CACHE))
                continue;

            /*
             * Skip monitor-specific entires
             */
            if (pdce->pMonitor != NULL)
                continue;

            if (pdce->DCX_flags & DCX_INVALID) {
                break;
            } else if (!(pdce->DCX_flags & DCX_INUSE)) {

                /*
                 * Remember the non-inuse one, but keep looking for an invalid.
                 */
                ppdceNotInUse = ppdce;
            }
        }

        /*
         * If we broke out of the loop, we found an invalid entry to reuse.
         * Otherwise see if we found a non-inuse entry to reuse.
         */
        if (pdce == NULL && ((ppdce = ppdceNotInUse) == NULL)) {

            /*
             * Create another DCE if we need it.
             */
            if (!CreateCacheDC(pwndOrg,
                               DCX_INVALID | DCX_CACHE |
                               (DCX_flags & DCX_LAYERED),
                               NULL)) {
                goto NullExit;
            }

            goto SearchAgain;
        }

        /*
         * We've chosen an entry to reuse: now fill it in and recompute it.
         */
        pdce = *ppdce;

RecomputeEntry:

        /*
         * Any non-invalid entries that we reuse might still have some bounds
         * that need to be used to invalidate SPBs.  Apply them here.
         */
        if (!(pdce->DCX_flags & DCX_INVALID))
            SpbCheckDce(pdce);

        /*
         * We want to compute only the matchable visrgn at first,
         * so we don't set up hrgnClip, or set the EXCLUDERGN or INTERSECTRGN
         * bits yet -- we'll deal with those later.
         */
        pdce->DCX_flags = DCX_flagsMatch | DCX_INUSE;

#if DBG
        /*
         * We're about to select the visrgn into the DC, even though it's
         * not yet completely setup. Turn off the visrgn validation for now.
         * It will be turned on before this function returns.
         */
        GreValidateVisrgn(pdce->hdc, FALSE);
#endif

        /*
         * Now recompute the visrgn (minus any hrgnClip shenanigans)
         */
        hrgn = NULL;
        if (CalcVisRgn(&hrgn, pwndOrg, pwndClip, DCX_flagsMatch) == FALSE) {
            pdce->DCX_flags |= DCX_PWNDORGINVISIBLE;
        }

        pdce->pwndOrg        = pwndOrg;
        pdce->pwndClip       = pwndClip;
        pdce->hrgnClip       = NULL;      // Just in case...
        pdce->hrgnClipPublic = NULL;

        ResetOrg(hrgn, pdce, TRUE);

        if (hrgn == NULL) {
            fVisRgnError = TRUE;
        }

        /*
         * When we arrive here, pdce (and *ppdce) point to
         * a cache entry whose visrgn and origin are set up.
         * All that remains to be done is to deal with EXCLUDE/INTERSECTRGN
         */
HaveComputedEntry:

        /*
         * If the window clipping flags have changed in the window
         * since the last time this dc was invalidated, then recompute
         * this dc entry.
         */
        if ((pdce->DCX_flags & DCX_MATCHMASK) != (DCX_flags & DCX_MATCHMASK))
            goto RecomputeEntry;

        /*
         * Let's check these assertions just in case...
         */
        UserAssert(pdce);
        UserAssert(*ppdce == pdce);
        UserAssert(pdce->DCX_flags & DCX_INUSE);
        UserAssert(!(pdce->DCX_flags & DCX_INVALID));
        UserAssert((pdce->DCX_flags & DCX_MATCHMASK) == (DCX_flags & DCX_MATCHMASK));

        /*
         * Move the dce to the head of the list so it's easy to find later.
         */
        if (pdce != gpDispInfo->pdceFirst) {
            *ppdce = pdce->pdceNext;
            pdce->pdceNext = gpDispInfo->pdceFirst;
            gpDispInfo->pdceFirst = pdce;
        }

#if DBG
        /*
         * We're about to mess with the visrgn in this DC, even though it's
         * not yet completely setup. Turn off the visrgn validation for now.
         * It will be turned on before this function returns.
         */
        GreValidateVisrgn(pdce->hdc, FALSE);
#endif

        /*
         * Time to deal with DCX_INTERSECTRGN or DCX_EXCLUDERGN.
         *
         * We handle these two bits specially, because cache entries
         * with these bits set cannot be reused with the bits set.  This
         * is because the area described in hrgnClip would have to be
         * compared along with the bit, which is a pain, especially since
         * they'd never match very often anyhow.
         *
         * What we do instead is to save the visrgn of the window before
         * applying either of these two flags, which is then restored
         * at ReleaseCacheDC() time, along with the clearing of these bits.
         * This effectively converts a cache entry with either of these
         * bits set into a "normal" cache entry that can be matched.
         */
        if (DCX_flags & DCX_INTERSECTRGN) {

            if (hrgnClip != HRGN_FULL) {

                SetEmptyRgn(ghrgnGDC);

                /*
                 * Save the visrgn for reuse on ReleaseDC().
                 * (do this BEFORE we set hrgnClip & pdce->flag bit,
                 * so that if a DCHook() callback occurs it recalculates
                 * without hrgnClip)
                 */
                UserAssertMsg0(!pdce->hrgnSavedVis,
                               "Nested SaveVisRgn attempt in _GetDCEx");

                /*
                 * get the current vis region into hrgnSavedVis.  Temporarily
                 * store a dummy one in the DC.
                 */

                pdce->hrgnSavedVis = CreateEmptyRgn();

                GreSelectVisRgn(pdce->hdc,pdce->hrgnSavedVis, SVR_SWAP);

                pdce->hrgnClip = hrgnClip;

                if (DCX_flags & DCX_NODELETERGN)
                    pdce->DCX_flags |= DCX_NODELETERGN;

                pdce->DCX_flags |= DCX_INTERSECTRGN;

                if (hrgnClip == NULL) {

                    pdce->hrgnClipPublic = NULL;

                } else {

                    IntersectRgn(ghrgnGDC, pdce->hrgnSavedVis, hrgnClip);

                    /*
                     * Make a copy of the hrgnClip and make it public
                     * so that we can use it in calculations in HungDraw.
                     */
                    pdce->hrgnClipPublic = CreateEmptyRgnPublic();
                    CopyRgn(pdce->hrgnClipPublic, hrgnClip);
                }

                /*
                 * Clear the SAVEDRGNINVALID bit, since we're just
                 * about to set it properly now.  If the dce later
                 * gets invalidated, it'll set this bit so we know
                 * to recompute it when we restore the visrgn.
                 */
                pdce->DCX_flags &= ~DCX_SAVEDRGNINVALID;

                /*
                 * Select in the new region.  we use the SWAP_REGION mode
                 * so that ghrgnGDC always has a valid rgn
                 */

                GreSelectVisRgn(pdce->hdc, ghrgnGDC, SVR_SWAP);
            }
        } else if (DCX_flags & DCX_EXCLUDERGN) {

            if (hrgnClip != NULL) {

                SetEmptyRgn(ghrgnGDC);

                /*
                 * Save the visrgn for reuse on ReleaseDC().
                 * (do this BEFORE we set hrgnClip & pdce->flag bit,
                 * so that if a DCHook() callback occurs it recalculates
                 * without hrgnClip)
                 */
                UserAssertMsg0(!pdce->hrgnSavedVis,
                               "Nested SaveVisRgn attempt in _GetDCEx");

                /*
                 * get the current vis region into hrgnSavedVis.  Temporarily
                 * store a dummy one in the DC.
                 */
                pdce->hrgnSavedVis = CreateEmptyRgn();

                GreSelectVisRgn(pdce->hdc,pdce->hrgnSavedVis, SVR_SWAP);

                pdce->hrgnClip = hrgnClip;

                if (DCX_flags & DCX_NODELETERGN)
                    pdce->DCX_flags |= DCX_NODELETERGN;

                pdce->DCX_flags |= DCX_EXCLUDERGN;

                if (hrgnClip == HRGN_FULL) {

                    pdce->hrgnClipPublic = HRGN_FULL;

                } else {

                    SubtractRgn(ghrgnGDC, pdce->hrgnSavedVis, hrgnClip);

                    /*
                     * Make a copy of the hrgnClip and make it public
                     * so that we can use it in calculations in HungDraw.
                     */
                    pdce->hrgnClipPublic = CreateEmptyRgnPublic();
                    CopyRgn(pdce->hrgnClipPublic, hrgnClip);
                }

                /*
                 * Clear the SAVEDRGNINVALID bit, since we're just
                 * about to set it properly now.  If the dce later
                 * gets invalidated, it'll set this bit so we know
                 * to recompute it when we restore the visrgn.
                 */
                pdce->DCX_flags &= ~DCX_SAVEDRGNINVALID;

                /*
                 * Select in the new region.  we use the SWAP_REGION mode
                 * so that ghrgnGDC always has a valid rgn
                 */

                GreSelectVisRgn(pdce->hdc, ghrgnGDC, SVR_SWAP);
            }
        }
    }

    if (pdce->DCX_flags & DCX_LAYERED) {
        UserAssert(pwndLayer != NULL);
        UserAssert(hbmLayer != NULL);

        UserVerify(GreSelectRedirectionBitmap(pdce->hdc, hbmLayer));

        /*
         * Enable bounds accumulation, so we know if there was any drawing
         * done into that DC and the actual rect we need to update when
         * this DC is released.
         */
        GreGetBounds(pdce->hdc, NULL, GGB_ENABLE_WINMGR);

        /*
         * In case the visrgn couldn't be allocated, clear it in the
         * dc again, since we just selected a new surface.
         */
        if (fVisRgnError) {
            GreSelectVisRgn(pdce->hdc, NULL, SVR_DELETEOLD);
        }
    }

    /*
     * Whew! Set ownership and return the bloody DC.
     * Only set ownership for cache dcs.  Own dcs have already been owned.
     * The reason why we don't want to set the ownership over again is
     * because the console sets its owndcs to PUBLIC so gdisrv can use
     * them without asserting.  We don't want to set the ownership back
     * again.
     */
    if (pdce->DCX_flags & DCX_CACHE) {

        if (!GreSetDCOwner(pdce->hdc, OBJECT_OWNER_CURRENT)) {
            RIPMSG1(RIP_WARNING, "GetDCEx: SetDCOwner Failed %lX", pdce->hdc);
        }

        /*
         * Decrement the Free DCE Count.  This should always be >= 0,
         * since we'll create a new dce if the cache is all in use.
         */
        DecrementFreeDCECount();

        pdce->ptiOwner = PtiCurrent();
    }

#ifdef USE_MIRRORING
    if (TestWF(pwnd, WEFLAYOUTRTL) && !(DCX_flags & DCX_NOMIRROR)) {
        GreSetLayout(pdce->hdc, -1, LAYOUT_RTL);
    }
#endif

#if DBG
    GreValidateVisrgn(pdce->hdc, TRUE);
#endif

    GreUnlockDisplay(gpDispInfo->hDev);

    return pdce->hdc;
}

/***************************************************************************\
* ReleaseCacheDC
*
* Releases a DC from the cache.
*
* History:
* 17-Jul-1991 DarrinM   Ported from Win 3.1 sources.
* 20-Dec-1995 ChrisWil  Added (hrgnClipPublic) entry.
\***************************************************************************/

UINT ReleaseCacheDC(
    HDC  hdc,
    BOOL fEndPaint)
{
    PDCE pdce;
    PDCE *ppdce;

    for (ppdce = &gpDispInfo->pdceFirst; (pdce = *ppdce); ppdce = &pdce->pdceNext) {

        if (pdce->hdc == hdc) {

            /*
             * Check for redundant releases or release of an invalid entry
             */
            if ((pdce->DCX_flags & (DCX_DESTROYTHIS | DCX_INVALID | DCX_INUSE)) != DCX_INUSE)
                return DCE_NORELEASE;

            /*
             * Lock the display since we may be playing with visrgns.
             */
            GreLockDisplay(gpDispInfo->hDev);

            if (pdce->DCX_flags & DCX_LAYERED) {
                UpdateLayeredSprite(pdce);
            }

            /*
             * If this is a permanent DC, then don't reset its state.
             */
            if (pdce->DCX_flags & DCX_CACHE) {
                /*
                 * Restore the DC state and mark the entry as not in use.
                 * Set owner back to server as well, since it's going back
                 * into the cache.
                 */
                if (!(pdce->DCX_flags & DCX_NORESETATTRS)) {
                    /*
                     * If bSetupDC() failed, the DC is busy (ie. in-use
                     * by another thread), so don't release it.
                     */
                    if ( (!(GreCleanDC(hdc))) ||
                         (!(GreSetDCOwner(hdc, OBJECT_OWNER_NONE))) ) {

                        GreUnlockDisplay(gpDispInfo->hDev);
                        return DCE_NORELEASE;
                    }

                } else if (!GreSetDCOwner(pdce->hdc, OBJECT_OWNER_NONE)) {

                    GreUnlockDisplay(gpDispInfo->hDev);
                    return DCE_NORELEASE;
                }

                pdce->ptiOwner  = NULL;
                pdce->DCX_flags    &= ~DCX_INUSE;

#if DBG
                /*
                 * Turn off checked only surface validation for now, since
                 * we may select a different surface (screen) in this DC that
                 * may not correspond to the visrgn currently in the DC. When
                 * the DC is given out again, it will be revalidated.
                 */
                GreValidateVisrgn(pdce->hdc, FALSE);
#endif

                /*
                 * The DC is no longer in use, so unselect the redirection
                 * bitmap from it.
                 */
                if (pdce->DCX_flags & DCX_LAYERED) {
                    UserVerify(GreSelectRedirectionBitmap(pdce->hdc, NULL));
                }

                /*
                 * Increment the Free DCE count.  This holds the count
                 * of available DCEs.  Check the threshold, and destroy
                 * the dce if it's above the mark.
                 */
                IncrementFreeDCECount();

                if (gnDCECount > DCE_SIZE_CACHETHRESHOLD) {
                    if (DestroyCacheDC(ppdce, pdce->hdc)) {
                        GreUnlockDisplay(gpDispInfo->hDev);
                        return DCE_FREED;
                    }
                }
            }

            /*
             * If we have an EXCLUDERGN or INTERSECTRGN cache entry,
             * convert it back to a "normal" cache entry by restoring
             * the visrgn and blowing away hrgnClip.
             *
             * Note that for non-DCX_CACHE DCs, we only do this if
             * we're being called from EndPaint().
             */
            if ((pdce->DCX_flags & (DCX_EXCLUDERGN | DCX_INTERSECTRGN)) &&
                    ((pdce->DCX_flags & DCX_CACHE) || fEndPaint)) {
                DeleteHrgnClip(pdce);
            }

            GreUnlockDisplay(gpDispInfo->hDev);
            return DCE_RELEASED;
        }
    }

    /*
     * Yell if DC couldn't be found...
     */
    RIPERR1(ERROR_DC_NOT_FOUND, RIP_WARNING,
            "Invalid device context (DC) handle passed to ReleaseCacheDC (0x%08lx)", hdc);

    return DCE_NORELEASE;
}

/***************************************************************************\
* CreateCacheDC
*
* Creates a DCE and adds it to the cache.
*
* History:
* 17-Jul-1991 DarrinM   Ported from Win 3.1 sources.
* 20-Dec-1995 ChrisWil  Added (hrgnClipPublic) entry.
\***************************************************************************/

HDC CreateCacheDC(
        PWND  pwndOrg,
        DWORD DCX_flags,
        PMONITOR pMonitor
        )
{
    PDCE pdce;
    HDC  hdc;
    HANDLE hDev;

    if ((pdce = (PDCE)UserAllocPool(sizeof(DCE), TAG_DCE)) == NULL)
        return NULL;

    if (pMonitor == NULL) {
        hDev = gpDispInfo->hDev;
    } else {
        hDev = pMonitor->hDev;
    }

    if ((hdc = GreCreateDisplayDC(hDev, DCTYPE_DIRECT, FALSE)) == NULL) {
        UserFreePool(pdce);
        return NULL;
    }

    /*
     * Link this entry into the cache entry list.
     */
    pdce->pdceNext      = gpDispInfo->pdceFirst;
    gpDispInfo->pdceFirst = pdce;

    pdce->hdc            = hdc;
    pdce->DCX_flags      = DCX_flags;
    pdce->pwndOrg        = pwndOrg;
    pdce->pwndClip       = pwndOrg;
    pdce->hrgnClip       = NULL;
    pdce->hrgnClipPublic = NULL;
    pdce->hrgnSavedVis   = NULL;
    pdce->pMonitor       = pMonitor;

    /*
     * Mark it as undeleteable so no application can delete it out of our
     * cache!
     */
    GreMarkUndeletableDC(hdc);

    if (DCX_flags & DCX_OWNDC) {

        /*
         * Set the ownership of owndcs immediately: that way console can set
         * the owernship to PUBLIC when it calls GetDC so that both the input
         * thread and the service threads can use the same owndc.
         */
        GreSetDCOwner(hdc, OBJECT_OWNER_CURRENT);
        pdce->ptiOwner = PtiCurrent();

    } else {

        /*
         * Otherwise it is a cache dc...  set its owner to none - nothing
         * is using it - equivalent of "being in the cache" but unaccessible
         * to other processes.
         */
        GreSetDCOwner(hdc, OBJECT_OWNER_NONE);
        pdce->ptiOwner = NULL;

        /*
         * Increment the available-cacheDC count.  Once this hits our
         * threshold, then we can free-up some of the entries.
         */
        IncrementFreeDCECount();
    }

    /*
     * If we're creating a permanent DC, then compute it now.
     */
    if (!(DCX_flags & DCX_CACHE)) {

        /*
         * Set up the class DC now...
         */
        if (TestCF(pwndOrg, CFCLASSDC))
            pwndOrg->pcls->pdce = pdce;

        /*
         * Finish setting up DCE and force eventual visrgn calculation.
         */
        UserAssert(!(DCX_flags & DCX_WINDOW));

        pdce->DCX_flags |= DCX_INUSE;

        InvalidateDce(pdce);
    }

    /*
     * If there are any spb's around then enable bounds accumulation.
     */
    if (AnySpbs())
        GreGetBounds(pdce->hdc, NULL, DCB_ENABLE | DCB_SET | DCB_WINDOWMGR);

    return pdce->hdc;
}

/***************************************************************************\
* WindowFromCacheDC
*
* Returns the window associated with a DC.
*
* History:
* 17-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

PWND WindowFromCacheDC(
    HDC hdc)
{
    PDCE pdce;
    for (pdce = gpDispInfo->pdceFirst; pdce; pdce = pdce->pdceNext) {

        if (pdce->hdc == hdc)
            return (pdce->DCX_flags & DCX_DESTROYTHIS) ? NULL : pdce->pwndOrg;
    }

    return NULL;
}

/***************************************************************************\
* DelayedDestroyCacheDC
*
* Destroys DCE's which have been partially destroyed.
*
* History:
* 16-Jun-1992 DavidPe   Created.
\***************************************************************************/

VOID DelayedDestroyCacheDC(VOID)
{
    PDCE *ppdce;
    PDCE pdce;


    /*
     * Zip through the cache looking for a DCX_DESTROYTHIS hdc.
     */
    for (ppdce = &gpDispInfo->pdceFirst; *ppdce != NULL; ) {

        /*
         * If we found a DCE on this thread that we tried to destroy
         * earlier, try and destroy it again.
         */
        pdce = *ppdce;

        if (pdce->DCX_flags & DCX_DESTROYTHIS)
            DestroyCacheDC(ppdce, pdce->hdc);

        /*
         * Step to the next DC.  If the DC was deleted, there
         * is no need to calculate address of the next entry.
         */
        if (pdce == *ppdce)
            ppdce = &pdce->pdceNext;
    }

    PpiCurrent()->W32PF_Flags &= ~W32PF_OWNDCCLEANUP;
}

/***************************************************************************\
* DestroyCacheDC
*
* Removes a DC from the cache, freeing all resources associated
* with it.
*
* History:
* 17-Jul-1991 DarrinM   Ported from Win 3.1 sources.
* 20-Dec-1995 ChrisWil  Added (hrgnClipPublic) entry.
\***************************************************************************/

BOOL DestroyCacheDC(
    PDCE *ppdce,
    HDC  hdc)
{
    PDCE pdce;

    /*
     * Zip through the cache looking for hdc.
     */
    if (ppdce == NULL) {
        for (ppdce = &gpDispInfo->pdceFirst; (pdce = *ppdce); ppdce = &pdce->pdceNext) {
            if (pdce->hdc == hdc)
                break;
        }
    }

    if (ppdce == NULL)
        return FALSE;

    /*
     * Set this here so we know this DCE is supposed to be deleted.
     */
    pdce = *ppdce;
    pdce->DCX_flags |= DCX_DESTROYTHIS;

    /*
     * Free up the dce object and contents.
     */

    if (!(pdce->DCX_flags & DCX_NODELETERGN)) {
        DeleteMaybeSpecialRgn(pdce->hrgnClip);
        pdce->hrgnClip = NULL;
    }

    if (pdce->hrgnClipPublic != NULL) {
        GreDeleteObject(pdce->hrgnClipPublic);
        pdce->hrgnClipPublic = NULL;
    }

    if (pdce->hrgnSavedVis != NULL) {
        GreDeleteObject(pdce->hrgnSavedVis);
        pdce->hrgnSavedVis = NULL;
    }

    /*
     * If GreSetDCOwner() or GreDeleteDC() fail, the
     * DC is in-use by another thread.  Set
     * W32PF_OWNDCCLEANUP so we know to scan for and
     * delete this DCE later.
     */
    if (!GreSetDCOwner(hdc, OBJECT_OWNER_PUBLIC)) {
        PpiCurrent()->W32PF_Flags |= W32PF_OWNDCCLEANUP;
        return FALSE;
    }

    /*
     * Set the don't rip flag so our routine RipIfCacheDC() doesn't
     * rip (called back from gdi).
     */
#if DBG
    pdce->DCX_flags |= DCX_DONTRIPONDESTROY;
    GreMarkDeletableDC(hdc);    // So GRE doesn't RIP.
#endif

    if (!GreDeleteDC(hdc)) {

#if DBG
        GreMarkUndeletableDC(hdc);
        pdce->DCX_flags &= ~DCX_DONTRIPONDESTROY;
#endif
        PpiCurrent()->W32PF_Flags |= W32PF_OWNDCCLEANUP;
        return FALSE;
    }

    /*
     * Decrement this dc-entry from the free-list count.
     */
    if (pdce->DCX_flags & DCX_CACHE) {

        if (!(pdce->DCX_flags & DCX_INUSE)) {
            DecrementFreeDCECount();
        }
    }

#if DBG
    pdce->pwndOrg  = NULL;
    pdce->pwndClip = NULL;
#endif

    /*
     * Unlink the DCE from the list.
     */
    *ppdce = pdce->pdceNext;

    UserFreePool(pdce);

    return TRUE;
}


/***************************************************************************\
* InvalidateGDIWindows
*
* Recalculates the visrgn of all descendents of pwnd on behalf of GRE.
*
* History:
\***************************************************************************/

VOID InvalidateGDIWindows(
    PWND pwnd)
{
    PVOID pwo;

    if (pwnd != NULL) {

        if ((pwo = _GetProp(pwnd, PROP_WNDOBJ, TRUE)) != NULL) {

            HRGN hrgnClient = NULL;

            if (GreWindowInsteadOfClient(pwo)) {

                /*
                 * Never clip children for WO_RGN_WINDOW so that NetMeeting
                 * gets the unioned window area:
                 */

                CalcVisRgn(&hrgnClient,
                           pwnd,
                           pwnd,
                           DCX_WINDOW |
                           (TestWF(pwnd, WFCLIPSIBLINGS) ? DCX_CLIPSIBLINGS : 0));
            } else {
                CalcVisRgn(&hrgnClient,
                           pwnd,
                           pwnd,
                           DCX_CLIPCHILDREN | DCX_CLIPSIBLINGS);
            }

            GreSetClientRgn(pwo, hrgnClient, &(pwnd->rcClient));
        }

        pwnd = pwnd->spwndChild;
        while (pwnd != NULL) {
            InvalidateGDIWindows(pwnd);
            pwnd = pwnd->spwndNext;
        }
    }
}

/***************************************************************************\
* zzzInvalidateDCCache
*
* This function is called when the visrgn of a window is changing for
* some reason.  It is responsible for ensuring that all of the cached
* visrgns in the DC cache that are affected by the visrgn change are
* invalidated.
*
* Operations that affect the visrgn of a window (i.e., things that better
* call this routine one way or another:)
*
*   Hiding or showing self or parent
*   Moving, sizing, or Z-order change of self or parent
*   Minimizing or unminimizing self or parent
*   Screen or paint locking of self or parent
*   LockWindowUpdate of self or parent
*
* Invalidates any cache entries associated with pwnd and/or any children of
* pwnd by either recalcing them on the fly if they're in use, or causing
* them to be recalced later.
*
* History:
* 17-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

BOOL zzzInvalidateDCCache(
    PWND  pwndInvalid,
    DWORD flags)
{
    PWND        pwnd;
    PDCE        pdce;
    PTHREADINFO ptiCurrent = PtiCurrent();
    TL          tlpwndInvalid;
    FLONG       fl;

    /*
     * Invalidation implies screen real estate is changing so we must
     * jiggle the mouse, because a different window may be underneath
     * the mouse, which needs to get a mouse move in order to change the
     * mouse pointer.
     *
     * The check for the tracking is added for full-drag-windows.  In doing
     * full-drag, zzzBltValidBits() is called from setting the window-pos.
     * This resulted in an extra-mousemove being queued from this routine.
     * So, when we're tracking, don't queue a mousemove.  This pointer is
     * null when tracking is off, so it won't effect the normal case.
     */
    ThreadLockAlwaysWithPti(ptiCurrent, pwndInvalid, &tlpwndInvalid);
    
    if (!(ptiCurrent->TIF_flags & TIF_MOVESIZETRACKING) &&
            !(flags & IDC_NOMOUSE)) {

#ifdef REDIRECTION
        if (!IsGlobalHooked(ptiCurrent, WHF_FROM_WH(WH_HITTEST)))
#endif // REDIRECTION
        
            zzzSetFMouseMoved();
    }
    
    /*
     * The visrgn of pwnd is changing.  First see if a change to this
     * visrgn will also affect other window's visrgns:
     *
     * 1) if parent is clipchildren, we need to invalidate parent
     * 2) if clipsiblings, we need to invalidate our sibling's visrgns.
     *
     * We don't optimize the case where we're NOT clipsiblings, and our
     * parent is clipchildren: very rare case.
     * We also don't optimize the fact that a clipsiblings window visrgn
     * change only affects the visrgns of windows BELOW it.
     */
    if (flags & IDC_DEFAULT) {

        flags = 0;

        if ((pwndInvalid->spwndParent != NULL) &&
            (pwndInvalid != PWNDDESKTOP(pwndInvalid))) {

            /*
             * If the parent is a clip-children window, then
             * a change to our visrgn will affect his visrgn, and
             * possibly those of our siblings.  So, invalidate starting
             * from our parent.  Note that we don't need to invalidate
             * any window DCs associated with our parent.
             */
            if (TestWF(pwndInvalid->spwndParent, WFCLIPCHILDREN)) {

                flags = IDC_CLIENTONLY;
                pwndInvalid = pwndInvalid->spwndParent;

            } else if (TestWF(pwndInvalid, WFCLIPSIBLINGS)) {

                /*
                 * If we are clip-siblings, chances are that our siblings are
                 * too.  A change to our visrgn might affect our siblings,
                 * so invalidate all of our siblings.
                 *
                 * NOTE! This code assumes that if pwndInvalid is NOT
                 * CLIPSIBLINGs, that either it does not overlap other
                 * CLIPSIBLINGs windows, or that none of the siblings are
                 * CLIPSIBLINGs.  This is a reasonable assumption, because
                 * mixing CLIPSIBLINGs and non CLIPSIBLINGs windows that
                 * overlap is generally unpredictable anyhow.
                 */
                flags = IDC_CHILDRENONLY;
                pwndInvalid = pwndInvalid->spwndParent;
            }
        }
    }

    /*
     * Go through the list of DCE's, looking for any that need to be
     * invalidated or recalculated.  Basically, any DCE that contains
     * a window handle that is equal to pwndInvalid or a child of pwndInvalid
     * needs to be invalidated.
     */
    for (pdce = gpDispInfo->pdceFirst; pdce; pdce = pdce->pdceNext) {

        if (pdce->DCX_flags & (DCX_INVALID | DCX_DESTROYTHIS))
            continue;

        /*
         * HACK ALERT
         *
         * A minimized client DC must never exclude its children, even if
         * its WS_CLIPCHILDREN bit is set.  For CS_OWNDC windows we must
         * update the flags of the DCE to reflect the change in window state
         * when the visrgn is eventually recomputed.
         */
        if (!(pdce->DCX_flags & (DCX_CACHE | DCX_WINDOW))) {

            if (TestWF(pdce->pwndOrg, WFCLIPCHILDREN))
                pdce->DCX_flags |= DCX_CLIPCHILDREN;

            if (TestWF(pdce->pwndOrg, WFMINIMIZED))
                pdce->DCX_flags &= ~DCX_CLIPCHILDREN;
        }

        /*
         * This code assumes that if pdce->pwndClip != pdce->pwndOrg,
         * that pdce->pwndClip == pdce->pwndOrg->spwndParent.  To ensure
         * that both windows are visited, we start the walk upwards from
         * the lower of the two, or pwndOrg.
         */
        UserAssert((pdce->pwndClip == pdce->pwndOrg) ||
                   (pdce->pwndClip == pdce->pwndOrg->spwndParent));

        /*
         * Walk upwards from pdce->pwndOrg, to see if we encounter
         * pwndInvalid.
         */
        for (pwnd = pdce->pwndOrg; pwnd; pwnd = pwnd->spwndParent) {

            if (pwnd == pwndInvalid) {

                if (pwndInvalid == pdce->pwndOrg) {

                    /*
                     * Ignore DCEs for pwndInvalid if IDC_CHILDRENONLY.
                     */
                    if (flags & IDC_CHILDRENONLY)
                        break;

                    /*
                     * Ignore window DCEs for pwndInvalid if IDC_CLIENTONLY
                     */
                    if ((flags & IDC_CLIENTONLY) && (pdce->DCX_flags & DCX_WINDOW))
                        break;
                }

                InvalidateDce(pdce);
                break;
            }
        }
    }

    /*
     * Update WNDOBJs in gdi if they exist.
     */
    GreLockDisplay(gpDispInfo->hDev);

    fl = (flags & IDC_MOVEBLT) ? GCR_DELAYFINALUPDATE : 0;

    if (gcountPWO != 0) {
        InvalidateGDIWindows(pwndInvalid);
        fl |= GCR_WNDOBJEXISTS;
    }

    GreClientRgnUpdated(fl);

    GreUpdateSpriteVisRgn(gpDispInfo->hDev);

    GreUnlockDisplay(gpDispInfo->hDev);

    ThreadUnlock(&tlpwndInvalid);

    return TRUE;
}

/***************************************************************************\
* _WindowFromDC (API)
*
* Takes a dc, returns the window associated with it.
*
* History:
* 23-Jun-1991 ScottLu   Created.
\***************************************************************************/

PWND _WindowFromDC(
    HDC hdc)
{
    PDCE pdce;

    for (pdce = gpDispInfo->pdceFirst; pdce; pdce = pdce->pdceNext) {

        if (!(pdce->DCX_flags & DCX_INUSE) || (pdce->DCX_flags & DCX_CREATEDC))
            continue;

        if (pdce->hdc == hdc)
            return pdce->pwndOrg;
    }

    return NULL;
}

/***************************************************************************\
* FastWindowFromDC
*
* Returns the window associated with a DC, and puts it at the
* front of the list.
*
* History:
* 23-Jun-1991 ScottLu   Created.
\***************************************************************************/

PWND FastWindowFromDC(
    HDC hdc)
{
    PDCE *ppdce;
    PDCE pdceT;

    if ((gpDispInfo->pdceFirst->hdc == hdc) &&
        (gpDispInfo->pdceFirst->DCX_flags & DCX_INUSE)) {

        return gpDispInfo->pdceFirst->pwndOrg;
    }

    for (ppdce = &gpDispInfo->pdceFirst; *ppdce; ppdce = &(*ppdce)->pdceNext) {

        if (((*ppdce)->hdc == hdc) && ((*ppdce)->DCX_flags & DCX_INUSE)) {

            /*
             * Unlink/link to make it first.
             */
            pdceT                 = *ppdce;
            *ppdce                = pdceT->pdceNext;
            pdceT->pdceNext       = gpDispInfo->pdceFirst;
            gpDispInfo->pdceFirst = pdceT;

            return pdceT->pwndOrg;
        }
    }

    return NULL;
}

/***************************************************************************\
* GetDCOrgOnScreen
*
* This function gets the DC origin of a window in screen coordinates. The
* DC origin is always in the surface coordinates. For screen DCs the
* surface is the screen, so their origin is already in the screen
* coordinates. For redirected DCs, GreGetDCOrg will return the origin
* of the DC in the redirected surface coordinates to which we will add
* the origin of the redirected window that the surface is backing.
*
* 11/25/1998        vadimg      created
\***************************************************************************/

BOOL GetDCOrgOnScreen(HDC hdc, LPPOINT ppt)
{
    if (GreGetDCOrg(hdc, ppt)) {
        POINT ptScreen;

        /*
         * Get the origin of the redirected window in screen coordinates.
         */
        if (UserGetRedirectedWindowOrigin(hdc, &ptScreen)) {
            ppt->x += ptScreen.x;
            ppt->y += ptScreen.y;
            return TRUE;
        }
    }
    return FALSE;
}

/***************************************************************************\
* UserGetRedirectedWindowOrigin
*
* The DC origin is in the surface coordinates. For screen DCs, the surface
* is the screen and so their origin is in the screen coordinates. But for
* redirected DCs, the backing surface origin is the same as the window
* being redirected. This function retrieves the screen origin of a redirected
* window corresponding to a redirection DC. It returns FALSE if this isn't
* a valid DC or it's not a redirected DC.
*
* 11/18/1998        vadimg      created
\***************************************************************************/

BOOL UserGetRedirectedWindowOrigin(HDC hdc, LPPOINT ppt)
{
    PWND pwnd;
    PDCE pdce;

    if ((pdce = LookupDC(hdc)) == NULL)
        return FALSE;

    if (!(pdce->DCX_flags & DCX_LAYERED))
        return FALSE;

    pwnd = GetLayeredWindow(pdce->pwndOrg);

    ppt->x = pwnd->rcWindow.left;
    ppt->y = pwnd->rcWindow.top;

    return TRUE;
}

/***************************************************************************\
* LookupDC
*
* Validate a DC by returning a correspnding pdce.
*
* 11/12/1997   vadimg          created
\***************************************************************************/

PDCE LookupDC(HDC hdc)
{
    PDCE pdce;

    for (pdce = gpDispInfo->pdceFirst; pdce != NULL; pdce = pdce->pdceNext) {

        if (pdce->DCX_flags & (DCX_INVALID | DCX_DESTROYTHIS))
            continue;
        
        if (pdce->hdc == hdc && pdce->pMonitor == NULL &&
                (pdce->DCX_flags & DCX_INUSE)) {
            return pdce;
        }
    }
    return NULL;
}

/***************************************************************************\
* GetMonitorDC
*
* 11/06/97      vadimg      ported from Memphis
\***************************************************************************/

#define DCX_LEAVEBITS (DCX_WINDOW | DCX_CLIPCHILDREN | DCX_CLIPSIBLINGS | \
        DCX_PARENTCLIP | DCX_LOCKWINDOWUPDATE | DCX_NOCLIPCHILDREN | \
        DCX_USESTYLE | DCX_EXCLUDEUPDATE | DCX_INTERSECTUPDATE | \
        DCX_EXCLUDERGN | DCX_INTERSECTRGN)

HDC GetMonitorDC(PDCE pdceOrig, PMONITOR pMonitor)
{
    PDCE pdce;
    POINT pt;
    RECT rc;

TryAgain:
    for (pdce = gpDispInfo->pdceFirst; pdce != NULL; pdce = pdce->pdceNext) {
        /*
         * Find an available DC for this monitor.
         */
        if (pdce->DCX_flags & (DCX_INUSE | DCX_DESTROYTHIS))
            continue;

        if (pdce->pMonitor != pMonitor)
            continue;

        if (!(pdce->DCX_flags & DCX_INVALID))
            SpbCheckDce(pdce);

        /*
         * Copy DC properties and style bits.
         */
        GreSetDCOwner(pdce->hdc, OBJECT_OWNER_CURRENT);
        pdce->pwndOrg = pdceOrig->pwndOrg;
        pdce->pwndClip = pdceOrig->pwndClip;
        pdce->ptiOwner = pdceOrig->ptiOwner;
        pdce->DCX_flags = (DCX_INUSE | DCX_CACHE) | 
                (pdceOrig->DCX_flags & DCX_LEAVEBITS);

        if (pdceOrig->hrgnClip > HRGN_FULL) {
            UserAssert(pdce->hrgnClip == NULL);
            UserAssert(pdceOrig->DCX_flags & (DCX_INTERSECTRGN | DCX_EXCLUDERGN));

            pdce->hrgnClip = CreateEmptyRgn();
            SetMonitorRegion(pMonitor, pdce->hrgnClip, pdceOrig->hrgnClip);
        } else {
            pdce->hrgnClip = pdceOrig->hrgnClip;
        }

        /*
         * Setup the visrgn clipped to this monitor.
         */
        GreCopyVisRgn(pdceOrig->hdc, ghrgnGDC);
        SetMonitorRegion(pMonitor, ghrgnGDC, ghrgnGDC);
        GreSelectVisRgn(pdce->hdc, ghrgnGDC, SVR_COPYNEW);

        GreGetDCOrgEx(pdceOrig->hdc, &pt, &rc);
        OffsetRect(&rc, -pMonitor->rcMonitor.left, -pMonitor->rcMonitor.top);
        GreSetDCOrg(pdce->hdc, rc.left, rc.top, (PRECTL)&rc);

        /*
         * Decrement the Free DCE Count.  This should always be >= 0,
         * since we'll create a new dce if the cache is all in use.
         */
        DecrementFreeDCECount();

        return pdce->hdc;
    }

    /*
     * If this call succeeds a new DC will be available in the cache,
     * so the loop will find it and properly set it up.
     */
    if (CreateCacheDC(NULL, DCX_INVALID | DCX_CACHE, pMonitor) == NULL)
        return NULL;

    goto TryAgain;
}

/***************************************************************************\
* RipIfCacheDC
*
* This is called on debug systems by gdi when it is destroying a dc
* to make sure it isn't in the cache.
*
* History:
\***************************************************************************/

#if DBG
VOID RipIfCacheDC(
    HDC hdc)
{
    PDCE pdce;

    /*
     * This is called on debug systems by gdi when it is destroying a dc
     * to make sure it isn't in the cache.
     */
    EnterCrit();

    for (pdce = gpDispInfo->pdceFirst; pdce; pdce = pdce->pdceNext) {

        if (pdce->hdc == hdc && !(pdce->DCX_flags & DCX_DONTRIPONDESTROY)) {

            RIPMSG1(RIP_ERROR,
                  "Deleting DC in DC cache - contact JohnC. hdc == %08lx\n",
                  pdce->hdc);
        }
    }

    LeaveCrit();
}
#endif

#ifdef USE_MIRRORING
/***************************************************************************\
* MirrorRect
*
* Mirror the client window coordinates.
* 
*
* History:
\***************************************************************************/
void MirrorRect(PWND pwnd, LPRECT lprc)
{
    int left, cx;

    cx          = pwnd->rcClient.right - pwnd->rcClient.left;
    left        = lprc->left;
    lprc->left  = cx - lprc->right;
    lprc->right = cx - left;
}

/***************************************************************************\
* OrderRects
*
* Order the rectangles, so that they flow from left to right. This is needed
* when combining a mirrored region (see MirrorRegion)
* 
*
* History:
\***************************************************************************/
void OrderRects(LPRECT lpR, int nRects)
{
    RECT R;
    int i,j;

    //
    // Sort Left to right
    //
    for (i=0; i<nRects; i++){
        for (j=i+1; (j<nRects) && ((lpR+j)->top == (lpR+i)->top); j++){
            if (((lpR+j)->left < (lpR+i)->left)) {
                R = *(lpR+i);
                *(lpR+i) = *(lpR+j); 
                *(lpR+j) = R;
            }
        }
    }

}

/***************************************************************************\
* MirrorRegion
*
* Mirror a region in a window. This is done by mirroring the rects
* that constitute the region. 'bUseClient' param controls whether 
* the region is a client one or not.
*
* History:
\***************************************************************************/
BOOL MirrorRegion(PWND pwnd, HRGN hrgn, BOOL bUseClient)
{
    int        nRects, i, nDataSize, Saveleft, cx;
    HRGN       hrgn2 = NULL;
    RECT       *lpR;
    RGNDATA    *lpRgnData;
    BOOL       bRet = FALSE;

    if (TestWF(pwnd, WEFLAYOUTRTL) && hrgn > HRGN_SPECIAL_LAST) {
        nDataSize = GreGetRegionData(hrgn, 0, NULL);
        if (nDataSize && (lpRgnData = (RGNDATA *)UserAllocPool(nDataSize, TAG_MIRROR))) {
            if (GreGetRegionData(hrgn, nDataSize, lpRgnData)) {
                nRects       = lpRgnData->rdh.nCount; 
                lpR          = (RECT *)lpRgnData->Buffer;
    
                if (bUseClient) {
                    cx = pwnd->rcClient.right - pwnd->rcClient.left;
                } else {
                    cx = pwnd->rcWindow.right - pwnd->rcWindow.left;
                }
    
                Saveleft                     = lpRgnData->rdh.rcBound.left;
                lpRgnData->rdh.rcBound.left  = cx - lpRgnData->rdh.rcBound.right;
                lpRgnData->rdh.rcBound.right = cx - Saveleft;
    
        
                for (i=0; i<nRects; i++){
                    Saveleft   = lpR->left;
                    lpR->left  = cx - lpR->right;
                    lpR->right = cx - Saveleft;
        
                    lpR++;
                }
        
                OrderRects((RECT *)lpRgnData->Buffer, nRects);
                hrgn2 = GreExtCreateRegion(NULL, nDataSize, lpRgnData);
                if (hrgn2) {
                    GreCombineRgn(hrgn, hrgn2, NULL, RGN_COPY);
                    GreDeleteObject((HGDIOBJ)hrgn2);
                    bRet = TRUE;
                }
            }
    
            //Free mem.
            UserFreePool(lpRgnData);
        }
    }

    return bRet;
}

#endif
