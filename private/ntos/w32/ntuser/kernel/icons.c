/****************************** Module Header ******************************\
* Module Name: icons.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains routines having to do with icons.
*
* History:
* 11-14-90 DarrinM      Created.
* 13-Feb-1991 mikeke    Added Revalidation code
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

#define DX_GAP      (SYSMET(CXMINSPACING) - SYSMET(CXMINIMIZED))
#define DY_GAP      (SYSMET(CYMINSPACING) - SYSMET(CYMINIMIZED))

/***************************************************************************\
* xxxArrangeIconicWindows
*
* Function to arrange all icons for a particular window.  Does this by
* Returns 0 if no icons or the height of one
* icon row if there are any icons.
*
* History:
* 11-14-90 darrinm      Ported from Win 3.0 sources.
*  4-17-91 mikehar      Win31 Merge
\***************************************************************************/

UINT xxxArrangeIconicWindows(
    PWND pwnd)
{
    PBWL pbwl;
    PSMWP psmwp;
    PWND pwndTest, pwndSort, pwndSwitch;
    HWND *phwnd, *phwndSort;
    CHECKPOINT *pcp, *pcpSort;
    POINT ptSort, ptSrc;
    WORD  nIcons = 0;
    RECT  rc;
    POINT ptMin;
    int   xOrg, yOrg;
    int   dx, dy;
    int   dxSlot, dySlot;
    int   cIconsPerPass, iIconPass;
    BOOL  fHorizontal, fBreak;
    TL tlpwndTest;
    BOOL fHideMe;

    CheckLock(pwnd);

    /*
     * Create a window list of all children of pwnd
     */
    if ((pbwl = BuildHwndList(pwnd->spwndChild, BWL_ENUMLIST, NULL)) == NULL)
        return 0;

    fHideMe = IsTrayWindow(pwnd->spwndChild);

    //
    // Put these into local vars for efficiency (see ParkIcon())
    //
    dxSlot = SYSMET(CXMINSPACING);
    dySlot = SYSMET(CYMINSPACING);

    //
    // We need to adjust the client rectangle if the parent has scrollbars.
    //
    GetRealClientRect(pwnd, &rc, GRC_SCROLLS, NULL);

    /*
     * find all icons
     */
    pwndSwitch = RevalidateHwnd(ghwndSwitch);
    for (phwnd = pbwl->rghwnd; *phwnd != (HWND)1; phwnd++) {
        if (((pwndTest = RevalidateHwnd(*phwnd)) == NULL) ||
                !TestWF(pwndTest , WFVISIBLE) ||
                pwndTest == pwndSwitch ||
                (pcp = (CHECKPOINT *)_GetProp(pwndTest, PROP_CHECKPOINT,
                        PROPF_INTERNAL)) == NULL) {
            *phwnd = NULL;
            continue;
        }

        if (!TestWF(pwndTest, WFMINIMIZED)) {
            pcp->fMinInitialized = FALSE;
            pcp->ptMin.x = pcp->ptMin.y = -1;
            *phwnd = NULL;
            continue;
        }

        /*
         * inc count of icons
         */
        nIcons++;

        /*
         * we will park in default position again...
         */
        pcp->fDragged = FALSE;

        /*
         * ensure the original position is up to date
         */
        pcp->ptMin.x = pwndTest->rcWindow.left;
        pcp->ptMin.y = pwndTest->rcWindow.top;
        _ScreenToClient(pwnd, &pcp->ptMin);

        // Slide into the nearest row or column
        switch (SYSMET(ARRANGE) & ~ARW_HIDE) {
            case ARW_TOPLEFT | ARW_RIGHT:
            case ARW_TOPRIGHT | ARW_LEFT:
                // Slide into top row
                pcp->ptMin.y += dySlot / 2;
                pcp->ptMin.y -= pcp->ptMin.y % dySlot;
                break;

            case ARW_TOPLEFT | ARW_DOWN:
            case ARW_BOTTOMLEFT | ARW_UP:
                // Slide into left column
                pcp->ptMin.x += dxSlot / 2;
                pcp->ptMin.x -= pcp->ptMin.x % dxSlot;
                break;

            case ARW_BOTTOMLEFT | ARW_RIGHT:
            case ARW_BOTTOMRIGHT | ARW_LEFT:
                // Slide into bottom row
                pcp->ptMin.y = rc.bottom - pcp->ptMin.y;
                pcp->ptMin.y += dySlot / 2;
                pcp->ptMin.y -= pcp->ptMin.y % dySlot;
                pcp->ptMin.y = rc.bottom - pcp->ptMin.y;
                break;

            case ARW_BOTTOMRIGHT | ARW_UP:
            case ARW_TOPRIGHT | ARW_DOWN:
                // Slide into right column
                pcp->ptMin.x = rc.right - pcp->ptMin.x;
                pcp->ptMin.x += dxSlot / 2;
                pcp->ptMin.x -= pcp->ptMin.x % dxSlot;
                pcp->ptMin.x = rc.right - pcp->ptMin.x;
                break;
        }
    }

    if (nIcons == 0) {

        /*
         * no icons were found...  break out
         */
        FreeHwndList(pbwl);
        return 0;
    }

    if (fHideMe) {
        ptMin.x = WHERE_NOONE_CAN_SEE_ME;
        ptMin.y = WHERE_NOONE_CAN_SEE_ME;
        goto JustParkEm;
    }

    //
    // Get gravity && move vars
    //
    if (SYSMET(ARRANGE) & ARW_STARTRIGHT) {
        // Starting on right side
        ptMin.x = xOrg = rc.right - dxSlot;
        dx = -dxSlot;
    } else {
        // Starting on left
        ptMin.x = xOrg = rc.left + DX_GAP;
        dx = dxSlot;
    }

    if (SYSMET(ARRANGE) & ARW_STARTTOP) {
        // Starting on top
        ptMin.y = yOrg = rc.top + DY_GAP;
        dy = dySlot;
    } else {
        // Starting on bottom
        ptMin.y = yOrg = rc.bottom - dySlot;
        dy = -dySlot;
    }

    //
    // Get arrange dir
    //
    fHorizontal = ( (SYSMET(ARRANGE) & ARW_DOWN) ? FALSE : TRUE );

    iIconPass = fHorizontal ? (rc.right / dxSlot) : (rc.bottom / dySlot);
    cIconsPerPass = iIconPass = max(1, iIconPass);

    /*
     * insertion sort of windows by y, and by x within a row.
     */
    for (phwnd = pbwl->rghwnd; *phwnd != (HWND)1; phwnd++) {

        /*
         * Check for 0 (window was not icon) and
         * Check for invalid HWND (window has been destroyed)
         */
        if (*phwnd == NULL || (pwndTest = RevalidateHwnd(*phwnd)) == NULL)
            continue;

        pcp = (CHECKPOINT *)_GetProp(pwndTest, PROP_CHECKPOINT,
                PROPF_INTERNAL);
        ptSrc = pcp->ptMin;

        fBreak = FALSE;
        for (phwndSort = pbwl->rghwnd; phwndSort < phwnd; phwndSort++) {
            if (*phwndSort == NULL ||
                    (pwndSort = RevalidateHwnd(*phwndSort)) == NULL)
                continue;

            pcpSort = (CHECKPOINT*)_GetProp(pwndSort, PROP_CHECKPOINT,
                    PROPF_INTERNAL);

            ptSort = pcpSort->ptMin;

            //
            // Is this the position in which to sort this min window?
            //
            switch (SYSMET(ARRANGE) & ~ARW_HIDE) {
                case ARW_BOTTOMLEFT | ARW_RIGHT:
                    // Lower left, moving horizontally
                    if (((ptSort.y == ptSrc.y) && (ptSort.x > ptSrc.x)) ||
                        (ptSort.y < ptSrc.y))
                        fBreak = TRUE;
                    break;

                case ARW_BOTTOMLEFT | ARW_UP:
                    // Lower left, moving vertically
                    if (((ptSort.x == ptSrc.x) && (ptSort.y < ptSrc.y)) ||
                        (ptSort.x > ptSrc.x))
                        fBreak = TRUE;
                    break;

                case ARW_BOTTOMRIGHT | ARW_LEFT:
                    // Lower right, moving horizontally
                    if (((ptSort.y == ptSrc.y) && (ptSort.x < ptSrc.x)) ||
                        (ptSort.y < ptSrc.y))
                        fBreak = TRUE;
                    break;

                case ARW_BOTTOMRIGHT | ARW_UP:
                    // Lower right, moving vertically
                    if (((ptSort.x == ptSrc.x) && (ptSort.y < ptSrc.y)) ||
                        (ptSort.x < ptSrc.x))
                        fBreak = TRUE;
                    break;

                case ARW_TOPLEFT | ARW_RIGHT:
                    // Top left, moving horizontally
                    if (((ptSort.y == ptSrc.y) && (ptSort.x > ptSrc.x)) ||
                        (ptSort.y > ptSrc.y))
                        fBreak = TRUE;
                    break;

                case ARW_TOPLEFT | ARW_DOWN:
                    // Top left, moving vertically
                    if (((ptSort.x == ptSrc.x) && (ptSort.y > ptSrc.y)) ||
                        (ptSort.x > ptSrc.x))
                        fBreak = TRUE;
                    break;

                case ARW_TOPRIGHT | ARW_LEFT:
                    // Top right, moving horizontally
                    if (((ptSort.y == ptSrc.y) && (ptSort.x < ptSrc.x)) ||
                        (ptSort.y > ptSrc.y))
                        fBreak = TRUE;
                    break;

                case ARW_TOPRIGHT | ARW_DOWN:
                    // Top right, moving vertically
                    if (((ptSort.x == ptSrc.x) && (ptSort.y > ptSrc.y)) ||
                        (ptSort.x < ptSrc.x))
                        fBreak = TRUE;
                    break;
            }

            if (fBreak)
                break;
        }

        /*
         * insert the window at this position by sliding the rest up.
         * LATER IanJa, use hwnd intermediate variables, avoid PW() & HW()
         */
        while (phwndSort < phwnd) {
            pwndSort = PW(*phwndSort);
            *phwndSort = HW(pwndTest);
            pwndTest = pwndSort;
            phwndSort++;
        }

        /*
         * replace the window handle in the original position
         */
        *phwnd = HW(pwndTest);
    }

    //
    // Now park the icons.
    //

JustParkEm:

    for (phwnd = pbwl->rghwnd; *phwnd != (HWND)1; phwnd++) {
        if (*phwnd == NULL || (pwndTest = RevalidateHwnd(*phwnd)) == NULL)
            continue;

        pcp = (CHECKPOINT *)_GetProp(pwndTest, PROP_CHECKPOINT,
                PROPF_INTERNAL);
        if (pcp != NULL) {
            pcp->fMinInitialized = TRUE;
            pcp->ptMin = ptMin;
        }

        if (fHideMe) {
            continue;
        }

        // Setup to process the next position
        if (--iIconPass <= 0) {
            // Need to setup next pass
            iIconPass = cIconsPerPass;

            if (fHorizontal) {
                ptMin.x = xOrg;
                ptMin.y += dy;
            } else {
                ptMin.x += dx;
                ptMin.y = yOrg;
            }
        } else {
            // Same pass
            if (fHorizontal)
                ptMin.x += dx;
            else
                ptMin.y += dy;
        }
    }

    psmwp = InternalBeginDeferWindowPos(2 * nIcons);
    if (psmwp == NULL)
        goto ParkExit;

    for (phwnd = pbwl->rghwnd; *phwnd != (HWND)1; phwnd++) {

        /*
         * Check for a NULL (window has gone away)
         */
        if (*phwnd == NULL || (pwndTest = RevalidateHwnd(*phwnd)) == NULL)
            continue;

        pcp = (CHECKPOINT *)_GetProp(pwndTest, PROP_CHECKPOINT,
                PROPF_INTERNAL);


        ThreadLockAlways(pwndTest, &tlpwndTest);

        psmwp = _DeferWindowPos(
                psmwp,
                pwndTest,
                NULL,
                pcp->ptMin.x,
                pcp->ptMin.y,
                SYSMET(CXMINIMIZED),
                SYSMET(CYMINIMIZED),
                SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOCOPYBITS);

        ThreadUnlock(&tlpwndTest);

        if (psmwp == NULL)
            break;
    }
    if (psmwp != NULL) {
        /*
         * Make the swp async so we don't hang waiting for hung apps.
         */
        xxxEndDeferWindowPosEx(psmwp, TRUE);
    }

ParkExit:
    FreeHwndList(pbwl);
    return nIcons;
}
