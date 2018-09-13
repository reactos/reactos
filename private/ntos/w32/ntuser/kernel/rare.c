/****************************** Module Header ******************************\
* Module Name: rare.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* History:
* 06-28-91 MikeHar      Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/*
 * MetricsRecalc flags
 */
#define CALC_RESIZE         0x0001
#define CALC_FRAME          0x0002
#define CALC_MINIMIZE       0x0004

/*
 * NormalizeRect flags
 */
#define NORMALIZERECT_NORMAL        0
#define NORMALIZERECT_MAXIMIZED     1
#define NORMALIZERECT_FULLSCREEN    2

/***************************************************************************\
* SnapshotMonitorRects
*
*   This is called from ResetDisplay to memorize the monitor positions so
* DesktopRecalcEx will know where to move stuff.
*
* Returns the MONITORRECTS if succeeded, NULL otherwise.
*
* History:
* 09-Dec-1996 adams     Created.
\***************************************************************************/

PMONITORRECTS
SnapshotMonitorRects(void)
{
    PMONITOR        pMonitor;
    PMONITORRECTS   pmr;
    PMONITORPOS     pmp;
#if DBG
    ULONG           cVisMon = 0;
#endif

    pmr = UserAllocPool(
            sizeof(MONITORRECTS) + sizeof(MONITORPOS) * (gpDispInfo->cMonitors - 1),
            TAG_MONITORRECTS);

    if (!pmr) {
        RIPERR0(ERROR_OUTOFMEMORY, RIP_WARNING, "Out of memory in SnapshotMonitorRects");
        return NULL;
    }

    pmp = pmr->amp;
    for (   pMonitor = gpDispInfo->pMonitorFirst;
            pMonitor;
            pMonitor = pMonitor->pMonitorNext) {

        if (!(pMonitor->dwMONFlags & MONF_VISIBLE))
            continue;
#if DBG
        cVisMon++;
#endif

        CopyRect(&pmp->rcMonitor, &pMonitor->rcMonitor);
        CopyRect(&pmp->rcWork,    &pMonitor->rcWork);

        /*
         * If the device for this monitor object is not active, don't store
         * the pointer to it in the list.  This way the windows on the inactive
         * monitor will be later moved to the default one.
         */
        if (HdevFromMonitor(pMonitor) == -1) {
            pmp->pMonitor = NULL;
        } else {
            pmp->pMonitor = pMonitor;
        }

        pmp++;
    }
    UserAssert(cVisMon == gpDispInfo->cMonitors);

    pmr->cMonitor = (int)(pmp - pmr->amp);

    return pmr;
}

/***************************************************************************\
* NormalizeRect
*
* Adjusts a window rectangle when the working area changes.  This can be
* because of a tray move, with the resolution staying the same, or
* because of a dynamic resolution change, with the tray staying the same
* relatively.
*
* History:
\***************************************************************************/

PMONITOR NormalizeRect(
    LPRECT          lprcDest,
    LPRECT          lprcSrc,
    PMONITORRECTS   pmrOld,
    int             iOldMonitor,
    int             codeFullScreen,
    DWORD           style)
{
    LPCRECT     lprcOldMonitor;
    LPCRECT     lprcOldWork;
    LPRECT      lprcNewWork;
    PMONITOR    pMonitor;
    int         cxOldMonitor;
    int         cyOldMonitor;
    int         cxNewMonitor;
    int         cyNewMonitor;
    int         dxOrg, dyOrg;

    /*
     * Track the window so it stays in the same place on the same monitor.
     * If the old monitor is no longer active then pick a default.
     */
    if ((pMonitor = pmrOld->amp[iOldMonitor].pMonitor) == NULL) {
        pMonitor = GetPrimaryMonitor();
    }

    lprcOldMonitor = &pmrOld->amp[iOldMonitor].rcMonitor;
    lprcOldWork = &pmrOld->amp[iOldMonitor].rcWork;

    /*
     * If is a fullscreen app just make it fullscreen at the new location.
     */
    if (codeFullScreen != NORMALIZERECT_NORMAL) {
        LPCRECT lprcOldSnap, lprcNewSnap;

        /*
         * If it is a maximized window snap it to the work area.
         * Otherwise it is a rude app so snap it to the screen.
         */
        if (codeFullScreen == NORMALIZERECT_MAXIMIZED) {
            lprcOldSnap = lprcOldWork;
            lprcNewSnap = &pMonitor->rcWork;
        } else {
            lprcOldSnap = lprcOldMonitor;
            lprcNewSnap = &pMonitor->rcMonitor;
        }

        lprcDest->left = lprcSrc->left +
            lprcNewSnap->left - lprcOldSnap->left;

        lprcDest->top = lprcSrc->top +
            lprcNewSnap->top - lprcOldSnap->top;

        lprcDest->right = lprcSrc->right +
            lprcNewSnap->right - lprcOldSnap->right;

        lprcDest->bottom = lprcSrc->bottom +
            lprcNewSnap->bottom - lprcOldSnap->bottom;

        goto AllDone;
    }

    /*
     * Offset the window by the change in desktop origin.
     */
    dxOrg = pMonitor->rcMonitor.left - lprcOldMonitor->left;
    dyOrg = pMonitor->rcMonitor.top - lprcOldMonitor->top;

    /*
     * Calculate the change in screen size (we need it in more than one place).
     */
    cxOldMonitor = lprcOldMonitor->right - lprcOldMonitor->left;
    cyOldMonitor = lprcOldMonitor->bottom - lprcOldMonitor->top;
    cxNewMonitor = pMonitor->rcMonitor.right - pMonitor->rcMonitor.left;
    cyNewMonitor = pMonitor->rcMonitor.bottom - pMonitor->rcMonitor.top;

    /*
     * If the monitor resolution has changed (or we moved to a new monitor)
     * then factor in the size change.
     */
    if (cxNewMonitor != cxOldMonitor || cyNewMonitor != cyOldMonitor) {
        int xWnd = lprcSrc->left - lprcOldMonitor->left;
        int yWnd = lprcSrc->top - lprcOldMonitor->top;

        dxOrg += MultDiv(xWnd, cxNewMonitor - cxOldMonitor, cxOldMonitor);
        dyOrg += MultDiv(yWnd, cyNewMonitor - cyOldMonitor, cyOldMonitor);
    }

    /*
     * Compute the initial new position.
     */
    CopyOffsetRect(lprcDest, lprcSrc, dxOrg, dyOrg);
    lprcNewWork = &pMonitor->rcWork;

    /*
     * Fit horizontally.  Try to fit so that the window isn't out of the
     * working area horizontally.  Keep left edge visible always.
     */
    if (lprcDest->right > lprcNewWork->right) {
        OffsetRect(lprcDest, lprcNewWork->right - lprcDest->right, 0);
    }

    if (lprcDest->left < lprcNewWork->left) {
        OffsetRect(lprcDest, lprcNewWork->left - lprcDest->left, 0);
    }

    /*
     * Fit vertically.  Try to fit so that the window isn't out of the
     * working area vertically.  Keep top edge visible always.
     */
    if (lprcDest->bottom > lprcNewWork->bottom) {
        OffsetRect(lprcDest, 0, lprcNewWork->bottom - lprcDest->bottom);
    }

    if (lprcDest->top < lprcNewWork->top) {
        OffsetRect(lprcDest, 0, lprcNewWork->top - lprcDest->top);
    }

    /*
     * If the window is sizeable then shrink it if necessary.
     */
    if (style & WS_THICKFRAME)
    {
        int cSnap = 0;

        if (lprcDest->right > lprcNewWork->right) {
            lprcDest->right = lprcNewWork->right;
            cSnap++;
        }

        if (lprcDest->bottom > lprcNewWork->bottom) {
            lprcDest->bottom = lprcNewWork->bottom;
            cSnap++;
        }


        /*
         * Now make sure we didn't turn this normal window into a
         * fullscreen window.  This is a complete hack but it is much
         * better than changing from 800x600 to 640x480 and ending up with
         * a bunch of fullscreen apps...
         */
        if (cSnap == 2) {
            InflateRect(lprcDest, -1, -1);
        }
    }

AllDone:
    return pMonitor;
}


/***************************************************************************\
* _SetRipFlags
*
* Sets the debug rip flags
*
* History:
* 16-Aug-1996 adams     Created.
\***************************************************************************/

VOID _SetRipFlags(
    DWORD dwRipFlags, DWORD dwPID)
{
    if (gpsi) {
        if ((dwRipFlags != (DWORD)-1) && !(dwRipFlags & ~RIPF_VALIDUSERFLAGS)) {
            gpsi->wRIPFlags = (WORD)((gpsi->wRIPFlags & ~RIPF_VALIDUSERFLAGS) | dwRipFlags);
        }
        if (dwPID != (DWORD)-1) {
            gpsi->wRIPPID = (WORD)dwPID;
        }
    }
}

/***************************************************************************\
* _SetDbgTag
*
* Sets debugging level for a tag.
*
* History:
* 16-Aug-1996 adams     Created.
\***************************************************************************/

void
_SetDbgTag(int tag, DWORD dwDBGTAGFlags)
{
#if DEBUGTAGS
    if (gpsi && tag < DBGTAG_Max && !(dwDBGTAGFlags & ~DBGTAG_VALIDUSERFLAGS)) {
        COPY_FLAG(gpsi->adwDBGTAGFlags[tag], dwDBGTAGFlags, DBGTAG_VALIDUSERFLAGS);
    }
#else
    UNREFERENCED_PARAMETER(tag);
    UNREFERENCED_PARAMETER(dwDBGTAGFlags);
#endif
}


/***************************************************************************\
* UpdateWinIniInt
*
* History:
* 18-Apr-1994 mikeke     Created
\***************************************************************************/
BOOL UpdateWinIniInt(PUNICODE_STRING pProfileUserName,
    UINT         idSection,
    UINT         wKeyNameId,
    int          value
    )
{
    WCHAR szTemp[40];
    WCHAR            szKeyName[40];
    swprintf(szTemp, L"%d", value);

    ServerLoadString(hModuleWin,
                         wKeyNameId,
                         szKeyName,
                         sizeof(szKeyName) / sizeof(WCHAR));

    return FastWriteProfileStringW(pProfileUserName,
                                   idSection,
                                   szKeyName,
                                   szTemp);


}

/***************************************************************************\
* SetDesktopMetrics
*
* History:
* 31-Jan-1994 mikeke    Ported
\***************************************************************************/

void SetDesktopMetrics()
{
    LPRECT      lprcWork;

    lprcWork = &GetPrimaryMonitor()->rcWork;

    SYSMET(CXFULLSCREEN)      = lprcWork->right - lprcWork->left;
    SYSMET(CXMAXIMIZED)       = lprcWork->right - lprcWork->left + 2*SYSMET(CXSIZEFRAME);

    SYSMET(CYFULLSCREEN)      = lprcWork->bottom - lprcWork->top - SYSMET(CYCAPTION);
    SYSMET(CYMAXIMIZED)       = lprcWork->bottom - lprcWork->top + 2*SYSMET(CYSIZEFRAME);
}

/***************************************************************************\
* xxxMetricsRecalc (Win95: MetricsRecalc)
*
* Does work to size/position all minimized or nonminimized
* windows.  Called when frame metrics or min metrics are changed.
*
* Note that you can NOT do DeferWindowPos() with this function.  SWP doesn't
* work when you do parents and children at the same time--it's only for
* peer windows.  Thus we must do SetWindowPos() for each window.
*
* History:
* 06-28-91 MikeHar      Ported.
\***************************************************************************/
void xxxMetricsRecalc(
    UINT wFlags,
    int  dx,
    int  dy,
    int  dyCaption,
    int  dyMenu)
{
    PHWND       phwnd;
    PWND        pwnd;
    RECT        rc;
    PCHECKPOINT pcp;
    TL          tlpwnd;
    BOOL        fResized;
    PBWL        pbwl;
    PTHREADINFO ptiCurrent;
    int         c;

    ptiCurrent = PtiCurrent();
    pbwl = BuildHwndList(
            GETDESKINFO(ptiCurrent)->spwnd->spwndChild,
            BWL_ENUMLIST | BWL_ENUMCHILDREN,
            NULL);

    if (!pbwl)
        return;

    UserAssert(*pbwl->phwndNext == (HWND) 1);
    for (   c = (int)(pbwl->phwndNext - pbwl->rghwnd), phwnd = pbwl->rghwnd;
            c > 0;
            c--, phwnd++) {

        pwnd = RevalidateHwnd(*phwnd);
        if (!pwnd)
            continue;

        ThreadLockAlwaysWithPti(ptiCurrent, pwnd, &tlpwnd);

        fResized = FALSE;

        if ((wFlags & CALC_MINIMIZE) && TestWF(pwnd, WFMINIMIZED)) {
            /*
             * We're changing the minimized window dimensions.  We need to
             * resize.  Note that we do NOT move.
             */
            CopyRect(&rc, (&pwnd->rcWindow));
            rc.right += dx;
            rc.bottom += dy;

            goto PositionWnd;
        }

        /*
         * We're changing the size of the window because the sizing border
         * changed.
         */
        if ((wFlags & CALC_RESIZE) && TestWF(pwnd, WFSIZEBOX)) {

            pcp = (CHECKPOINT *)_GetProp(pwnd, PROP_CHECKPOINT, PROPF_INTERNAL);

            /*
             * Update maximized position to account for sizing border
             * We do this for DOS box also.  This way client of max'ed windows
             * stays in same relative position.
             */
            if (pcp && (pcp->fMaxInitialized)) {
                pcp->ptMax.x -= dx;
                pcp->ptMax.y -= dy;
            }

            if (TestWF(pwnd, WFMINIMIZED)) {
                if (pcp)
                    InflateRect(&pcp->rcNormal, dx, dy);
            } else {
                CopyInflateRect(&rc, (&pwnd->rcWindow), dx, dy);
                if (TestWF(pwnd, WFCPRESENT))
                    rc.bottom += dyCaption;
                if (TestWF(pwnd, WFMPRESENT))
                    rc.bottom += dyMenu;

PositionWnd:
                fResized = TRUE;

                /*
                 * Remember SWP expects values in PARENT CLIENT coordinates.
                 */
                if (pwnd->spwndParent != PWNDDESKTOP(pwnd)) {
                    OffsetRect(&rc,
                        -pwnd->spwndParent->rcClient.left,
                        -pwnd->spwndParent->rcClient.top);
                }

                xxxSetWindowPos(pwnd,
                                PWND_TOP,
                                rc.left,
                                rc.top,
                                rc.right-rc.left,
                                rc.bottom-rc.top,

#if 0 // Win95 flags
                                SWP_NOZORDER | SWP_NOACTIVATE | SWP_DEFERDRAWING | SWP_FRAMECHANGED);
#else
                                SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_FRAMECHANGED | SWP_NOREDRAW);
#endif
            }
        }

        /*
         * We're changing the nonclient widgets, so recalculate the
         * client.
         */
        if (wFlags & CALC_FRAME) {

            /*
             * Delete any cached small icons...
             */
            if (dyCaption)
                 xxxSendMessage(pwnd, WM_SETICON, ICON_RECREATE, 0);

            if (!TestWF(pwnd, WFMINIMIZED) && !fResized) {

                CopyRect(&rc, &(pwnd->rcWindow));
                if (TestWF(pwnd, WFMPRESENT))
                    rc.bottom += dyMenu;

                if (TestWF(pwnd, WFCPRESENT)) {
                    rc.bottom += dyCaption;
                    /*
                     * Maximized MDI child windows position their caption
                     *  outside their parent's client area (negative y).
                     *  If the caption has changed, they need to be
                     *  repositioned.
                     */
                    if (TestWF(pwnd, WFMAXIMIZED)
                            && TestWF(pwnd, WFCHILD)
                            && (GETFNID(pwnd->spwndParent) == FNID_MDICLIENT)) {

                        xxxSetWindowPos(pwnd,
                                        PWND_TOP,
                                        rc.left  - pwnd->spwndParent->rcWindow.left,
                                        rc.top   - pwnd->spwndParent->rcWindow.top - dyCaption,
                                        rc.right - rc.left,
                                        rc.bottom- rc.top, SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED | SWP_NOREDRAW);
                        goto LoopCleanup;
                    }
                }

                xxxSetWindowPos(pwnd,
                                PWND_TOP,
                                0,
                                0,
                                rc.right-rc.left,
                                rc.bottom-rc.top,
#if 0 // Win95 flags
                                SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED | SWP_NOREDRAW);
#else
                                SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED | SWP_NOCOPYBITS | SWP_NOREDRAW);
#endif
            }
        }

LoopCleanup:
        ThreadUnlock(&tlpwnd);
    }

    FreeHwndList(pbwl);
}

/***************************************************************************\
* FindOldMonitor
*
* Returns the index of the monitor in "pmr" which has the greatest
* overlap with a rectangle. This function is used to determine which
* monitor a window was on after one or more monitor rectangles have
* changed.
*
* History:
* 11-Sep-1996 adams     Created.
\***************************************************************************/

int
FindOldMonitor(LPCRECT lprc, PMONITORRECTS pmr)
{
    DWORD       dwClosest;
    int         iClosest, i;
    int         cxRect, cyRect;
    PMONITORPOS pmp;

    iClosest = -1;
    dwClosest = 0;

    cxRect = (lprc->right - lprc->left);
    cyRect = (lprc->bottom - lprc->top);

    for (i = 0, pmp = pmr->amp; i < pmr->cMonitor; pmp++, i++)
    {
        RECT rcT;

        if (IntersectRect(&rcT, lprc, &pmp->rcMonitor))
        {
            DWORD dwT;

            //
            // convert to width/height
            //
            rcT.right -= rcT.left;
            rcT.bottom -= rcT.top;

            //
            // if fully enclosed, we're done
            //
            if ((rcT.right == cxRect) && (rcT.bottom == cyRect))
                return i;

            //
            // use largest area
            //
            dwT = (DWORD)rcT.right * (DWORD)rcT.bottom;
            if (dwT > dwClosest)
            {
                dwClosest = dwT;
                iClosest = i;
            }
        }
    }

    return iClosest;
}



/***************************************************************************\
* xxxDesktopRecalc
*
* Moves all top-level nonpopup windows into free desktop area,
* attempting to keep them in the same position relative to the monitor
* they were on.  Also resets minimized info (so that when a window is
* subsequently minimized it will go to the correct location).
*
* History:
* 11-Sep-1996 adams     Created.
\***************************************************************************/

void
xxxDesktopRecalc(PMONITORRECTS pmrOld)
{
    PWND            pwndDesktop;
    PSMWP           psmwp;
    PHWND           phwnd;
    PBWL            pbwl;
    PWND            pwnd;
    CHECKPOINT *    pcp;
    int             iOldMonitor;
    int             codeFullScreen;

    UserVerify(pwndDesktop = _GetDesktopWindow());
    if ((pbwl = BuildHwndList(pwndDesktop->spwndChild, BWL_ENUMLIST, NULL)) == NULL)
        return;

    if ((psmwp = InternalBeginDeferWindowPos(4)) != NULL) {
        for (phwnd = pbwl->rghwnd; *phwnd != (HWND)1 && psmwp; phwnd++) {
            /*
             * Make sure this hwnd is still around.
             */
            if (    (pwnd = RevalidateHwnd(*phwnd)) == NULL ||
                    TestWF(pwnd, WEFTOOLWINDOW)) {

                continue;
            }

            codeFullScreen = TestWF(pwnd, WFFULLSCREEN) ?
                NORMALIZERECT_FULLSCREEN : NORMALIZERECT_NORMAL;

            pcp = (CHECKPOINT *)_GetProp(pwnd, PROP_CHECKPOINT, PROPF_INTERNAL);
            if (pcp) {

                /*
                 * We don't need to blow away saved maximized positions
                 * anymore, since the max position is always (for top level
                 * windows) relative to the origin of the monitor's working
                 * area.  And for child windows, we shouldn't do it period
                 * anyway.
                 */
                pcp->fMinInitialized = FALSE;

                /*
                 * Figure out which monitor the position was on before things
                 * got shuffled around and try to keep it on that monitor.  If
                 * it was never visible on a monitor then leave it alone.
                 */
                iOldMonitor = FindOldMonitor(&pcp->rcNormal, pmrOld);
                if (iOldMonitor != (UINT)-1) {
                    NormalizeRect(
                            &pcp->rcNormal,
                            &pcp->rcNormal,
                            pmrOld,
                            iOldMonitor,
                            codeFullScreen,
                            pwnd->style);
                }
            }

            /*
             * Figure out which monitor the position was on before things got
             * shuffled around and try to keep it on that monitor.  If it
             * was never visible on a monitor then leave it alone.
             */

            iOldMonitor = FindOldMonitor(&pwnd->rcWindow, pmrOld);
            if (iOldMonitor != -1) {

                PMONITOR pMonitorDst;
                RECT rc;

                /*
                 * Check for maximized apps that are truly maximized.
                 * (As opposed to apps that manage their owm maximized rect.)
                 */
                if (TestWF(pwnd, WFMAXIMIZED)) {
                    LPRECT lprcOldWork = &pmrOld->amp[iOldMonitor].rcWork;

                    if (    (pwnd->rcWindow.right - pwnd->rcWindow.left >=
                                lprcOldWork->right - lprcOldWork->left)
                            &&
                            (pwnd->rcWindow.bottom - pwnd->rcWindow.top >=
                                lprcOldWork->bottom - lprcOldWork->top)) {

                        codeFullScreen = NORMALIZERECT_MAXIMIZED;
                    }
                }

                pMonitorDst = NormalizeRect(
                        &rc,
                        &pwnd->rcWindow,
                        pmrOld,
                        iOldMonitor,
                        codeFullScreen,
                        pwnd->style);

                if (TestWF(pwnd, WFMAXFAKEREGIONAL)) {
                    UserAssert(pMonitorDst->hrgnMonitor);
                    pwnd->hrgnClip = pMonitorDst->hrgnMonitor;
                }

                psmwp = _DeferWindowPos(
                        psmwp,
                        pwnd,
                        (PWND)HWND_TOP,
                        rc.left,
                        rc.top,
                        rc.right - rc.left,
                        rc.bottom - rc.top,
                        SWP_NOACTIVATE | SWP_NOZORDER);
            }
        }

        if (psmwp) {
            xxxEndDeferWindowPosEx(psmwp, TRUE);
        }
    }

    FreeHwndList(pbwl);
}



/***************************************************************************\
* SetWindowMetricInt
*
* History:
* 25-Feb-96 BradG       Added Pixel -> TWIPS conversion
\***************************************************************************/

BOOL SetWindowMetricInt(PUNICODE_STRING pProfileUserName,
        WORD wKeyNameId,
        int iIniValue
        )
{
    /*
     * If you change the below list of STR_* make sure you make a corresponding
     * change in FastGetProfileIntFromID (profile.c)
     */
    switch (wKeyNameId) {
    case STR_BORDERWIDTH:
    case STR_SCROLLWIDTH:
    case STR_SCROLLHEIGHT:
    case STR_CAPTIONWIDTH:
    case STR_CAPTIONHEIGHT:
    case STR_SMCAPTIONWIDTH:
    case STR_SMCAPTIONHEIGHT:
    case STR_MENUWIDTH:
    case STR_MENUHEIGHT:
    case STR_ICONHORZSPACING:
    case STR_ICONVERTSPACING:
    case STR_MINWIDTH:
    case STR_MINHORZGAP:
    case STR_MINVERTGAP:
      /*
       * Always store window metrics in TWIPS
       */
        iIniValue = -MultDiv(iIniValue, 72*20, gpsi->dmLogPixels);
        break;
    }

      return UpdateWinIniInt(pProfileUserName,
                             PMAP_METRICS,
                             wKeyNameId,
                             iIniValue);
}

/***************************************************************************\
* SetWindowMetricFont
*
* History:
\***************************************************************************/

BOOL SetWindowMetricFont(PUNICODE_STRING pProfileUserName,
    UINT idKey,
    LPLOGFONT lplf
    )
{
    return FastWriteProfileValue(
            pProfileUserName,
            PMAP_METRICS,
            (LPWSTR)UIntToPtr( idKey ),
            REG_BINARY,
            (LPBYTE)lplf,
            sizeof(LOGFONTW)
            );
}


/***************************************************************************\
* SetAndDrawNCMetrics
*
* History:
\***************************************************************************/

BOOL xxxSetAndDrawNCMetrics(PUNICODE_STRING pProfileUserName,
    int clNewBorder,
    LPNONCLIENTMETRICS lpnc)
{
    int dl;
    int dxMinOld;
    int dyMinOld;
    int cxBorder;
    int cyBorder;
    int dyCaption;
    int dyMenu;

    dl = clNewBorder - gpsi->gclBorder;
    dxMinOld = SYSMET(CXMINIMIZED);
    dyMinOld = SYSMET(CYMINIMIZED);
    cxBorder = SYSMET(CXBORDER);
    cyBorder = SYSMET(CYBORDER);


    /*
     * Do we need to recalculate?
     */
    if ((lpnc == NULL) && !dl)
        return(FALSE);

    if (lpnc) {
        dyCaption = (int)lpnc->iCaptionHeight - SYSMET(CYSIZE);
        dyMenu = (int)lpnc->iMenuHeight - SYSMET(CYMENUSIZE);
    } else {
        dyCaption = dyMenu = 0;
    }


    /*
     * Recalculate the system metrics
     */
    xxxSetWindowNCMetrics(pProfileUserName, lpnc, TRUE, clNewBorder);

    /*
     * Reset our saved menu size/position info
     */
    MenuRecalc();

    /*
     * Reset window sized, positions, frames
     */
    xxxMetricsRecalc(
            CALC_FRAME | (dl ? CALC_RESIZE : 0),
            dl*cxBorder,
            dl*cyBorder,
            dyCaption,
            dyMenu);

    dxMinOld = SYSMET(CXMINIMIZED) - dxMinOld;
    dyMinOld = SYSMET(CYMINIMIZED) - dyMinOld;
    if (dxMinOld || dyMinOld) {
        xxxMetricsRecalc(CALC_MINIMIZE, dxMinOld, dyMinOld, 0, 0);
    }

    xxxRedrawScreen();


    return TRUE;
}

/***************************************************************************\
* xxxSetAndDrawMinMetrics
*
* History:
* 13-May-1994 mikeke     mikeke     Ported
\***************************************************************************/
BOOL xxxSetAndDrawMinMetrics(PUNICODE_STRING pProfileUserName,
        LPMINIMIZEDMETRICS lpmin
        )
{
    /*
     * Save minimized window dimensions
     */
    int dxMinOld = SYSMET(CXMINIMIZED);
    int dyMinOld = SYSMET(CYMINIMIZED);


    SetMinMetrics(pProfileUserName,lpmin);

    /*
     * Do we need to adjust minimized size?
     */
    dxMinOld = SYSMET(CXMINIMIZED) - dxMinOld;
    dyMinOld = SYSMET(CYMINIMIZED) - dyMinOld;

    if (dxMinOld || dyMinOld) {
        xxxMetricsRecalc(CALC_MINIMIZE, dxMinOld, dyMinOld, 0, 0);
    }

    xxxRedrawScreen();


    return TRUE;
}


/***************************************************************************\
* xxxSPISetNCMetrics
*
* History:
* 13-May-1994 mikeke     mikeke     Ported
\***************************************************************************/

BOOL xxxSPISetNCMetrics(PUNICODE_STRING pProfileUserName,
        LPNONCLIENTMETRICS lpnc,
        BOOL fAlterWinIni
        )
{
    BOOL fWriteAllowed = !fAlterWinIni;
    BOOL fChanged = FALSE;

    if (fAlterWinIni) {
        fChanged  = SetWindowMetricInt(pProfileUserName,STR_BORDERWIDTH,     (int) lpnc->iBorderWidth        );
        fChanged &= SetWindowMetricInt(pProfileUserName,STR_SCROLLWIDTH,     (int) lpnc->iScrollWidth        );
        fChanged &= SetWindowMetricInt(pProfileUserName,STR_SCROLLHEIGHT,    (int) lpnc->iScrollHeight       );
        fChanged &= SetWindowMetricInt(pProfileUserName,STR_CAPTIONWIDTH,    (int) lpnc->iCaptionWidth       );
        fChanged &= SetWindowMetricInt(pProfileUserName,STR_CAPTIONHEIGHT,   (int) lpnc->iCaptionHeight      );
        fChanged &= SetWindowMetricInt(pProfileUserName,STR_SMCAPTIONWIDTH,  (int) lpnc->iSmCaptionWidth     );
        fChanged &= SetWindowMetricInt(pProfileUserName,STR_SMCAPTIONHEIGHT, (int) lpnc->iSmCaptionHeight    );
        fChanged &= SetWindowMetricInt(pProfileUserName,STR_MENUWIDTH,       (int) lpnc->iMenuWidth          );
        fChanged &= SetWindowMetricInt(pProfileUserName,STR_MENUHEIGHT,      (int) lpnc->iMenuHeight         );

        fChanged &= SetWindowMetricFont(pProfileUserName,STR_CAPTIONFONT,    &lpnc->lfCaptionFont            );
        fChanged &= SetWindowMetricFont(pProfileUserName,STR_SMCAPTIONFONT,  &lpnc->lfSmCaptionFont          );
        fChanged &= SetWindowMetricFont(pProfileUserName,STR_MENUFONT,       &lpnc->lfMenuFont               );
        fChanged &= SetWindowMetricFont(pProfileUserName,STR_STATUSFONT,     &lpnc->lfStatusFont             );
        fChanged &= SetWindowMetricFont(pProfileUserName,STR_MESSAGEFONT,    &lpnc->lfMessageFont            );

        fWriteAllowed = fChanged;
    }

    if (fWriteAllowed)
        xxxSetAndDrawNCMetrics(pProfileUserName,(int) lpnc->iBorderWidth, lpnc);

    return fChanged;
}

/***************************************************************************\
* xxxSPISetMinMetrics
*
* History:
* 13-May-1994 mikeke     mikeke     Ported
\***************************************************************************/

BOOL xxxSPISetMinMetrics(PUNICODE_STRING pProfileUserName,
    LPMINIMIZEDMETRICS lpmin,
    BOOL fAlterWinIni
    )
{
    BOOL fWriteAllowed = !fAlterWinIni;
    BOOL fChanged = FALSE;

    if (fAlterWinIni) {
        fChanged  = SetWindowMetricInt(pProfileUserName,STR_MINWIDTH,   (int) lpmin->iWidth      );
        fChanged &= SetWindowMetricInt(pProfileUserName,STR_MINHORZGAP, (int) lpmin->iHorzGap    );
        fChanged &= SetWindowMetricInt(pProfileUserName,STR_MINVERTGAP, (int) lpmin->iVertGap    );
        fChanged &= SetWindowMetricInt(pProfileUserName,STR_MINARRANGE, (int) lpmin->iArrange    );

        fWriteAllowed = fChanged;
    }

    if (fWriteAllowed) {
        xxxSetAndDrawMinMetrics(pProfileUserName,lpmin);
    }

    return fChanged;
}


/***************************************************************************\
* SPISetIconMetrics
*
* History:
* 13-May-1994 mikeke     mikeke     Ported
\***************************************************************************/
BOOL SPISetIconMetrics(PUNICODE_STRING pProfileUserName,
    LPICONMETRICS lpicon,
    BOOL          fAlterWinIni
    )
{
    BOOL fWriteAllowed = !fAlterWinIni;
    BOOL fChanged = FALSE;

    if (fAlterWinIni) {
        fChanged  = SetWindowMetricInt(pProfileUserName,STR_ICONHORZSPACING, (int) lpicon->iHorzSpacing);
        fChanged &= SetWindowMetricInt(pProfileUserName,STR_ICONVERTSPACING, (int) lpicon->iVertSpacing);
        fChanged &= SetWindowMetricInt(pProfileUserName,STR_ICONTITLEWRAP,   (int) lpicon->iTitleWrap);
        fChanged &= SetWindowMetricFont(pProfileUserName,STR_ICONFONT,             &lpicon->lfFont);

        fWriteAllowed = fChanged;
    }

    if (fWriteAllowed) {

        SetIconMetrics(pProfileUserName,lpicon);

        xxxRedrawScreen();
    }

    return fChanged;
}


/***************************************************************************\
* SPISetIconTitleFont
*
* History:
* 13-May-1994 mikeke     mikeke     Ported
\***************************************************************************/

BOOL SPISetIconTitleFont(PUNICODE_STRING pProfileUserName,
    LPLOGFONT lplf,
    BOOL      fAlterWinIni
    )
{
    HFONT hfnT;
    BOOL  fWriteAllowed = !fAlterWinIni;
    BOOL  fWinIniChanged = FALSE;

    if (hfnT = CreateFontFromWinIni(pProfileUserName,lplf, STR_ICONFONT)) {
        if (fAlterWinIni) {

            if (lplf) {
                LOGFONT lf;

                GreExtGetObjectW(hfnT, sizeof(LOGFONTW), &lf);
                fWinIniChanged = SetWindowMetricFont(pProfileUserName,STR_ICONFONT, &lf);
            } else {
                /*
                 * !lParam so go back to current win.ini settings so
                 */
                fWinIniChanged = TRUE;
            }

            fWriteAllowed = fWinIniChanged;
        }

        if (fWriteAllowed) {

            if (ghIconFont) {
                GreMarkDeletableFont(ghIconFont);
                GreDeleteObject(ghIconFont);
            }

            ghIconFont = hfnT;

        } else {
            GreMarkDeletableFont(hfnT);
            GreDeleteObject(hfnT);
        }
    }

    return fWinIniChanged;
}

/***************************************************************************\
* xxxSetSPIMetrics
*
* History:
* 13-May-1994 mikeke     mikeke     Ported
\***************************************************************************/

BOOL
xxxSetSPIMetrics(PUNICODE_STRING pProfileUserName,
        DWORD wFlag,
        LPVOID lParam,
        BOOL fAlterWinIni
        )
{
    BOOL fWinIniChanged;

    switch (wFlag) {
    case SPI_SETANIMATION:
        if (fAlterWinIni) {
            fWinIniChanged = SetWindowMetricInt(pProfileUserName,
                    STR_MINANIMATE,
                    (int) ((LPANIMATIONINFO) lParam)->iMinAnimate
                    );

            if (!fWinIniChanged) {
                return FALSE;
            }
        } else {
            fWinIniChanged = FALSE;
        }

        SET_OR_CLEAR_PUDF(PUDF_ANIMATE, ((LPANIMATIONINFO) lParam)->iMinAnimate);
        return fWinIniChanged;

    case SPI_SETNONCLIENTMETRICS:
        return xxxSPISetNCMetrics(pProfileUserName,(LPNONCLIENTMETRICS) lParam, fAlterWinIni);

    case SPI_SETICONMETRICS:
        return SPISetIconMetrics(pProfileUserName,(LPICONMETRICS) lParam, fAlterWinIni);

    case SPI_SETMINIMIZEDMETRICS:
        return xxxSPISetMinMetrics(pProfileUserName,(LPMINIMIZEDMETRICS) lParam, fAlterWinIni);

    case SPI_SETICONTITLELOGFONT:
        return SPISetIconTitleFont(pProfileUserName,(LPLOGFONT) lParam, fAlterWinIni);

    default:
        RIPERR1(ERROR_INVALID_PARAMETER, RIP_WARNING, "SetSPIMetrics. Invalid wFlag: %#lx", wFlag);
        return FALSE;
    }
}

/***************************************************************************\
* SetFilterKeys
*
* History:
* 10-12-94 JimA         Created.
\***************************************************************************/

BOOL SetFilterKeys(PUNICODE_STRING pProfileUserName,
    LPFILTERKEYS pFilterKeys)
{
    LPWSTR           pwszd = L"%d";
    BOOL             fWinIniChanged;
    WCHAR            szTemp[40];

    swprintf(szTemp, pwszd, pFilterKeys->dwFlags);
    fWinIniChanged = FastWriteProfileStringW(pProfileUserName,
            PMAP_KEYBOARDRESPONSE,
            L"Flags",
            szTemp
            );

    swprintf(szTemp, pwszd, pFilterKeys->iWaitMSec);
    fWinIniChanged &= FastWriteProfileStringW(pProfileUserName,
            PMAP_KEYBOARDRESPONSE,
            L"DelayBeforeAcceptance",
            szTemp
            );

    swprintf(szTemp, pwszd, pFilterKeys->iDelayMSec);
    fWinIniChanged &= FastWriteProfileStringW(pProfileUserName,
            PMAP_KEYBOARDRESPONSE,
            L"AutoRepeatDelay",
            szTemp
            );

    swprintf(szTemp, pwszd, pFilterKeys->iRepeatMSec);
    fWinIniChanged &= FastWriteProfileStringW(pProfileUserName,
            PMAP_KEYBOARDRESPONSE,
            L"AutoRepeatRate",
            szTemp
            );

    swprintf(szTemp, pwszd, pFilterKeys->iBounceMSec);
    fWinIniChanged &= FastWriteProfileStringW(pProfileUserName,
            PMAP_KEYBOARDRESPONSE,
            L"BounceTime",
            szTemp
            );

    return fWinIniChanged;
}

/***************************************************************************\
* SetMouseKeys
*
* History:
* 10-12-94 JimA         Created.
\***************************************************************************/

BOOL SetMouseKeys(PUNICODE_STRING pProfileUserName,
    LPMOUSEKEYS  pMK)
{
    LPWSTR           pwszd = L"%d";
    BOOL             fWinIniChanged;
    WCHAR            szTemp[40];

    swprintf(szTemp, pwszd, pMK->dwFlags);
    fWinIniChanged = FastWriteProfileStringW(pProfileUserName,
            PMAP_MOUSEKEYS,
            L"Flags",
            szTemp
            );

    swprintf(szTemp, pwszd, pMK->iMaxSpeed);
    fWinIniChanged &= FastWriteProfileStringW(pProfileUserName,
            PMAP_MOUSEKEYS,
            L"MaximumSpeed",
            szTemp
            );

    swprintf(szTemp, pwszd, pMK->iTimeToMaxSpeed);
    fWinIniChanged &= FastWriteProfileStringW(pProfileUserName,
            PMAP_MOUSEKEYS,
            L"TimeToMaximumSpeed",
            szTemp
            );

    return fWinIniChanged;
}

/***************************************************************************\
* SetSoundSentry
*
* History:
* 10-12-94 JimA         Created.
\***************************************************************************/

BOOL SetSoundSentry(PUNICODE_STRING pProfileUserName,
    LPSOUNDSENTRY pSS)
{
    LPWSTR           pwszd = L"%d";
    BOOL             fWinIniChanged;
    WCHAR            szTemp[40];

    swprintf(szTemp, pwszd, pSS->dwFlags);
    fWinIniChanged = FastWriteProfileStringW(pProfileUserName,
            PMAP_SOUNDSENTRY,
            L"Flags",
            szTemp
            );

    swprintf(szTemp, pwszd, pSS->iFSTextEffect);
    fWinIniChanged &= FastWriteProfileStringW(pProfileUserName,
            PMAP_SOUNDSENTRY,
            L"TextEffect",
            szTemp
            );

    swprintf(szTemp, pwszd, pSS->iWindowsEffect);
    fWinIniChanged &= FastWriteProfileStringW(pProfileUserName,
            PMAP_SOUNDSENTRY,
            L"WindowsEffect",
            szTemp
            );

    return fWinIniChanged;
}


/***************************************************************************\
* CalculateMouseSensitivity
*
* History:
* 09-27-96 jparsons     Created.
\***************************************************************************/

LONG CalculateMouseSensitivity(LONG lSens)
/*
 *      The resultant table looks like this...
 *
 *      Sens |  Sensitivity Adjustment
 *       0   |  0           * SENS_SCALAR  ALGORITHM 0<=NUM<=2
 *       1   |  1/32        * SENS_SCALAR   (NUM/32)*SENS_SCALAR
 *       2   |  1/16        * SENS_SCALAR
 *       3   |  1/8         * SENS_SCALAR  ALGORITHM 3<=NUM<=10
 *       4   |  1/4  (2/8)  * SENS_SCALAR   ((NUM-2)/8)*SENS_SCALAR
 *       5   |  3/8         * SENS_SCALAR
 *       6   |  1/2  (4/8)  * SENS_SCALAR
 *       7   |  5/8         * SENS_SCALAR
 *       8   |  3/4  (6/8)  * SENS_SCALAR
 *       9   |  7/8         * SENS_SCALAR
 *       10  |  1           * SENS_SCALAR
 *       11  |  5/4         * SENS_SCALAR  ALGORITHM NUM>=11
 *       12  |  3/2  (6/4)  * SENS_SCALAR   ((NUM-6)/4)*SENS_SCALAR
 *       13  |  7/4         * SENS_SCALAR
 *       14  |  2    (8/4)  * SENS_SCALAR
 *       15  |  9/4         * SENS_SCALAR
 *       16  |  5/2  (10/4) * SENS_SCALAR
 *       17  |  11/4        * SENS_SCALAR
 *       18  |  3    (12/4) * SENS_SCALAR
 *       19  |  13/4        * SENS_SCALAR
 *       20  |  7/2  (14/4) * SENS_SCALAR
 *
 *     COMMENTS:  Sensitivities are constrained to be between 1 and 20.
 */
{
    LONG lSenFactor ;

    if(lSens <= 2)
       lSenFactor=lSens*256/32 ;
    else if(lSens >= 3 && lSens <= 10 )
       lSenFactor=(lSens-2)*256/8 ;
    else
       lSenFactor=(lSens-6)*256/4 ;

    return lSenFactor ;
}


/***************************************************************************\
* xxxSystemParametersInfo
*
* SPI_GETBEEP:   wParam is not used. lParam is long pointer to a boolean which
*                gets true if beep on, false if beep off.
*
* SPI_SETBEEP:   wParam is a bool which sets beep on (true) or off (false).
*                lParam is not used.
*
* SPI_GETMOUSE:  wParam is not used. lParam is long pointer to an integer
*                array where rgw[0] gets xMouseThreshold, rgw[1] gets
*                yMouseThreshold, and rgw[2] gets gMouseSpeed.
*
* SPI_SETMOUSE:  wParam is not used. lParam is long pointer to an integer
*                array as described above.  User's values are set to values
*                in array.
*
* SPI_GETBORDER: wParam is not used. lParam is long pointer to an integer
*                which gets the value of clBorder (border multiplier factor).
*
* SPI_SETBORDER: wParam is an integer which sets gpsi->gclBorder.
*                lParam is not used.
*
* SPI_GETKEYBOARDDELAY: wParam is not used. lParam is a long pointer to an int
*                which gets the current keyboard repeat delay setting.
*
* SPI_SETKEYBOARDDELAY: wParam is the new keyboard delay setting.
*                lParam is not used.
*
* SPI_GETKEYBOARDSPEED: wParam is not used.  lParam is a long pointer
*                to an int which gets the current keyboard repeat
*                speed setting.
*
* SPI_SETKEYBOARDSPEED: wParam is the new keyboard speed setting.
*                lParam is not used.
*
* SPI_KANJIMENU: wParam contains:
*                    1 - Mouse accelerator
*                    2 - ASCII accelerator
*                    3 - Kana accelerator
*                lParam is not used.  The wParam value is stored in the global
*                KanjiMenu for use in accelerator displaying & searching.
*
* SPI_LANGDRIVER: wParam is not used.
*                 lParam contains a LPSTR to the new language driver filename.
*
* SPI_ICONHORIZONTALSPACING: wParam is the width in pixels of an icon cell.
*
* SPI_ICONVERTICALSPACING: wParam is the height in pixels of an icon cell.
*
* SPI_GETSCREENSAVETIMEOUT: wParam is not used
*                lParam is a pointer to an int which gets the screen saver
*                timeout value.
*
* SPI_SETSCREENSAVETIMEOUT: wParam is the time in seconds for the system
*                to be idle before screensaving.
*
* SPI_GETSCREENSAVEACTIVE: lParam is a pointer to a BOOL which gets TRUE
*                if the screensaver is active else gets false.
*
* SPI_SETSCREENSAVEACTIVE: if wParam is TRUE, screensaving is activated
*                else it is deactivated.
*
* SPI_GETLOWPOWERTIMEOUT:
* SPI_GETPOWEROFFTIMEOUT: wParam is not used
*                lParam is a pointer to an int which gets the appropriate
*                power saving screen blanker timeout value.
*
* SPI_SETLOWPOWERTIMEOUT:
* SPI_SETPOWEROFFTIMEOUT: wParam is the time in seconds for the system
*                to be idle before power saving screen blanking.
*
* SPI_GETLOWPOWERACTIVE:
* SPI_GETPOWEROFFACTIVE: lParam is a pointer to a BOOL which gets TRUE
*                if the power saving screen blanker is active else gets false.
*
* SPI_SETLOWPOWERACTIVE:
* SPI_SETPOWEROFFACTIVE: if wParam is TRUE, power saving screen blanking is
*                activated else it is deactivated.
*
* SPI_GETGRIDGRANULARITY: Obsolete. Returns 1 always.
*
* SPI_SETGRIDGRANULARITY: Obsolete.  Does nothing.
*
* SPI_SETDESKWALLPAPER: wParam is not used; lParam is a long ptr to a string
*                that holds the name of the bitmap file to be used as the
*                desktop wall paper.
*
* SPI_SETDESKPATTERN: Both wParam and lParam are not used; USER will read the
*                "pattern=" from WIN.INI and make it as the current desktop
*                 pattern;
*
* SPI_GETICONTITLEWRAP: lParam is LPINT which gets 0 if wrapping if off
*                       else gets 1.
*
* SPI_SETICONTITLEWRAP: wParam specifies TRUE to turn wrapping on else false
*
* SPI_GETMENUDROPALIGNMENT: lParam is LPINT which gets 0 specifies if menus
*                 drop left aligned else 1 if drop right aligned.
*
* SPI_SETMENUDROPALIGNMENT: wParam 0 specifies if menus drop left aligned else
*                 the drop right aligned.
*
* SPI_SETDOUBLECLKWIDTH: wParam specifies the width of the rectangle
*                 within which the second click of a double click must fall
*                 for it to be registered as a double click.
*
* SPI_SETDOUBLECLKHEIGHT: wParam specifies the height of the rectangle
*                 within which the second click of a double click must fall
*                 for it to be registered as a double click.
*
* SPI_GETICONTITLELOGFONT: lParam is a pointer to a LOGFONT struct which
*                 gets the logfont for the current icon title font. wParam
*                 specifies the size of the logfont struct.
*
* SPI_SETDOUBLECLICKTIME: wParm specifies the double click time
*
* SPI_SETMOUSEBUTTONSWAP: if wParam is 1, swap mouse buttons else if wParam
*                 is 0, don't swap buttons
* SPI_SETDRAGFULLWINDOWS: wParam = fSet.
* SPI_GETDRAGFULLWINDOWS: returns fSet.
*
* SPI_GETFILTERKEYS: lParam is a pointer to a FILTERKEYS struct.  wParam
*                 specifies the size of the filterkeys struct.
*
* SPI_SETFILTERKEYS: lParam is a pointer to a FILTERKEYS struct.  wParam
*                 is not used.
*
* SPI_GETSTICKYKEYS: lParam is a pointer to a STICKYKEYS struct.  wParam
*                 specifies the size of the stickykeys struct.
*
* SPI_SETSTICKYKEYS: lParam is a pointer to a STICKYKEYS struct.  wParam
*                 is not used.
*
* SPI_GETMOUSEKEYS: lParam is a pointer to a MOUSEKEYS struct.  wParam
*                 specifies the size of the mousekeys struct.
*
* SPI_SETMOUSEKEYS: lParam is a pointer to a MOUSEKEYS struct.  wParam
*                 is not used.
*
* SPI_GETACCESSTIMEOUT: lParam is a pointer to an ACCESSTIMEOUT struct.
*                 wParam specifies the size of the accesstimeout struct.
*
* SPI_SETACCESSTIMEOUT: lParam is a pointer to a ACCESSTIMEOUT struct.
*                 wParam is not used.
*
* SPI_GETTOGGLEKEYS: lParam is a pointer to a TOGGLEKEYS struct.  wParam
*                 specifies the size of the togglekeys struct.
*
* SPI_SETTOGGLEKEYS: lParam is a pointer to a TOGGLEKEYS struct.  wParam
*                 is not used.
*
* SPI_GETKEYBOARDPREF: lParam is a pointer to a BOOL.
*                 wParam is not used.
*
* SPI_SETKEYBOARDPREF: wParam is a BOOL.
*                 lParam is not used.
*
* SPI_GETSCREENREADER: lParam is a pointer to a BOOL.
*                 wParam is not used.
*
* SPI_SETSCREENREADER: wParam is a BOOL.
*                 lParam is not used.
*
* SPI_GETSHOWSOUNDS: lParam is a pointer to a SHOWSOUNDS struct.  wParam
*                 specifies the size of the showsounds struct.
*
* SPI_SETSHOWSOUNDS: lParam is a pointer to a SHOWSOUNDS struct.  wParam
*                 is not used.
*
* SPI_GETNONCLIENTMETRICS: lParam is a pointer to a NONCLIENTMETRICSW struct.
*                 wPAram is not used.
*
* SPI_GETSNAPTODEFBUTTON: lParam is a pointer to a BOOL which gets TRUE
*                if the snap to default push button is active else gets false.
*
* SPI_SETSNAPTODEFBUTTON: if wParam is TRUE, dialog boxes will snap the mouse
*                pointer to the default push button when created.
*
* SPI_GETFONTSMOOTHING:
*     wParam is unused
*     lParam is LPINT for boolean fFontSmoothing
*
* SPI_SETFONTSMOOTHING:
*     wParam is INT for boolean fFontSmoothing
*
* SPI_GETWHEELSCROLLLINES: lParam is a pointer to a ULONG to receive the
*                 suggested number of lines to scroll when the wheel is
*                 rotated. wParam is unused.
*
* SPI_SETWHEELSCROLLLINES: wParam is a ULONG containing the suggested number
*                 of lines to scroll when the wheel is rotated. lParam is
*                 unused.
*
* SPI_SETSCREENSAVERRUNNING / SPI_SCREENSAVERRUNNING: not supported on NT.
* SPI_GETSCREENSAVERRUNNING: wParam - Not used. lParam a pointer to a BOOL which
*                 will receive TRUE is a screen saver is running or FALSE otherwise.
*
* SPI_SETSHOWIMEUI wParam is TRUE or FALSE
* SPI_GETSHOWIMEUI neither wParam or lParam used
*
* History:
* 06-28-91      MikeHar     Ported.
* 12-8-93       SanfordS    Added SPI_SET/GETDRAGFULLWINDOWS
* 20-May-1996   adams       Added SPI_SET/GETWHEELSCROLLLINES
\***************************************************************************/

BOOL xxxSystemParametersInfo(
        UINT         wFlag,     // Item to change
        DWORD        wParam,
        PVOID        lParam,
        UINT         flags
        )
{
    PPROCESSINFO         ppi = PpiCurrent();
    int                  clBorderOld;
    int                  clBorderNew;
    LPWSTR               pwszd = L"%d";
    WCHAR                szSection[40];
    WCHAR                szTemp[40];
    WCHAR                szPat[MAX_PATH];
    BOOL                 fWinIniChanged = FALSE;
    BOOL                 fAlterWinIni = ((flags & SPIF_UPDATEINIFILE) != 0);
    BOOL                 fSendWinIniChange = ((flags & SPIF_SENDCHANGE) != 0);
    BOOL                 fWriteAllowed = !fAlterWinIni;
    ACCESS_MASK          amRequest;
    LARGE_UNICODE_STRING strSection;
    int                  *piTimeOut;
    int                  iResID;
    TL tlName;
    PUNICODE_STRING pProfileUserName = NULL;

    UserAssert(IsWinEventNotifyDeferredOK());

    /*
     * CONSIDER(adams) : Many of the SPI_GET* could be implemented
     * on the client side (SnapTo, WheelScrollLines, etc.).
     */

    /*
     * Features not implemented
     */

    switch (wFlag)
    {
        case SPI_TIMEOUTS:
        case SPI_KANJIMENU:
        case SPI_LANGDRIVER:
        case SPI_UNUSED39:
        case SPI_UNUSED40:
        case SPI_SETPENWINDOWS:

        case SPI_GETWINDOWSEXTENSION:
        case SPI_SETSCREENSAVERRUNNING:     // same as SPI_SCREENSAVERRUNNING

        case SPI_GETSERIALKEYS:
        case SPI_SETSERIALKEYS:

        case SPI_SETMOUSETRAILS:
        case SPI_GETMOUSETRAILS:
            RIPERR1(ERROR_INVALID_PARAMETER,
                    RIP_WARNING,
                    "SPI_ 0x%lx parameter not supported", wFlag );

            return FALSE;
    }


    /*
     * Perform access check.  Always grant access to CSR.
     */
    if (ppi->Process != gpepCSRSS) {
        switch (wFlag) {
        case SPI_SETBEEP:
        case SPI_SETMOUSE:
        case SPI_SETBORDER:
        case SPI_SETKEYBOARDSPEED:
        case SPI_SETDEFAULTINPUTLANG:
        case SPI_SETSCREENSAVETIMEOUT:
        case SPI_SETSCREENSAVEACTIVE:
        case SPI_SETLOWPOWERTIMEOUT:
        case SPI_SETPOWEROFFTIMEOUT:
        case SPI_SETLOWPOWERACTIVE:
        case SPI_SETPOWEROFFACTIVE:
        case SPI_SETGRIDGRANULARITY:
        case SPI_SETDESKWALLPAPER:
        case SPI_SETDESKPATTERN:
        case SPI_SETKEYBOARDDELAY:
        case SPI_SETICONTITLEWRAP:
        case SPI_SETMENUDROPALIGNMENT:
        case SPI_SETDOUBLECLKWIDTH:
        case SPI_SETDOUBLECLKHEIGHT:
        case SPI_SETDOUBLECLICKTIME:
        case SPI_SETMOUSEBUTTONSWAP:
        case SPI_SETICONTITLELOGFONT:
        case SPI_SETFASTTASKSWITCH:
        case SPI_SETFILTERKEYS:
        case SPI_SETTOGGLEKEYS:
        case SPI_SETMOUSEKEYS:
        case SPI_SETSHOWSOUNDS:
        case SPI_SETSTICKYKEYS:
        case SPI_SETACCESSTIMEOUT:
        case SPI_SETSOUNDSENTRY:
        case SPI_SETKEYBOARDPREF:
        case SPI_SETSCREENREADER:
        case SPI_SETSNAPTODEFBUTTON:
        case SPI_SETANIMATION:
        case SPI_SETNONCLIENTMETRICS:
        case SPI_SETICONMETRICS:
        case SPI_SETMINIMIZEDMETRICS:
        case SPI_SETWORKAREA:
        case SPI_SETFONTSMOOTHING:
        case SPI_SETMOUSEHOVERWIDTH:
        case SPI_SETMOUSEHOVERHEIGHT:
        case SPI_SETMOUSEHOVERTIME:
        case SPI_SETWHEELSCROLLLINES:
        case SPI_SETMENUSHOWDELAY:
        case SPI_SETHIGHCONTRAST:
        case SPI_SETDRAGFULLWINDOWS:
        case SPI_SETDRAGWIDTH:
        case SPI_SETDRAGHEIGHT:
        case SPI_SETCURSORS:
        case SPI_SETICONS:
        case SPI_SETLANGTOGGLE:
            amRequest = WINSTA_WRITEATTRIBUTES;
            break;

        case SPI_ICONHORIZONTALSPACING:
        case SPI_ICONVERTICALSPACING:
            if (IS_PTR(lParam)) {
                amRequest = WINSTA_READATTRIBUTES;
            } else if (wParam) {
                amRequest = WINSTA_WRITEATTRIBUTES;
            } else
                return TRUE;
            break;

        default:
            if ((wFlag & SPIF_RANGETYPEMASK) && (wFlag & SPIF_SET)) {
                amRequest = WINSTA_WRITEATTRIBUTES;
            } else {
                amRequest = WINSTA_READATTRIBUTES;
            }
            break;
        }

        if (amRequest == WINSTA_READATTRIBUTES) {
            RETURN_IF_ACCESS_DENIED(ppi->amwinsta, amRequest, FALSE);
        } else {
            UserAssert(amRequest == WINSTA_WRITEATTRIBUTES);
            if (!CheckWinstaWriteAttributesAccess()) {
                return FALSE;
            }
        }

        /*
         * If we're reading, then set the write flag to ensure that
         * the return value will be TRUE.
         */
        if (amRequest == WINSTA_READATTRIBUTES)
            fWriteAllowed = TRUE;
    } else {
        fWriteAllowed = TRUE;
    }

    /*
     * Make sure the section buffer is terminated.
     */
    szSection[0] = 0;

    switch (wFlag) {
    case SPI_GETBEEP:
        (*(BOOL *)lParam) = TEST_BOOL_PUDF(PUDF_BEEP);
        break;

    case SPI_SETBEEP:
        if (fAlterWinIni) {
            ServerLoadString(
                    hModuleWin,
                    (UINT)(wParam ? STR_BEEPYES : STR_BEEPNO),
                    (LPWSTR)szTemp, 10);

            fWinIniChanged = FastUpdateWinIni(NULL,
                    PMAP_BEEP,
                    (UINT) STR_BEEP,
                    szTemp
                    );

            fWriteAllowed = fWinIniChanged;
        }

        if (fWriteAllowed) {
            SET_OR_CLEAR_PUDF(PUDF_BEEP, wParam);
        }

        break;


    case SPI_SETMOUSESPEED:
        if (((LONG_PTR) lParam < MOUSE_SENSITIVITY_MIN) || ((LONG_PTR) lParam > MOUSE_SENSITIVITY_MAX)) {
            return FALSE ;
        }

        if (fAlterWinIni) {
            swprintf(szTemp, pwszd, lParam) ;
            fWinIniChanged = FastUpdateWinIni(NULL,
                    PMAP_MOUSE,
                    STR_MOUSESENSITIVITY,
                    szTemp
                    );

            fWriteAllowed = fWinIniChanged;
        }

        if (fWriteAllowed) {
            gMouseSensitivity = PtrToLong(lParam);
            gMouseSensitivityFactor = CalculateMouseSensitivity(PtrToLong(lParam)) ;
        }
        break;

    case SPI_GETMOUSESPEED:
        *((LPINT)lParam) = gMouseSensitivity ;
        break;

    case SPI_GETMOUSE:
        ((LPINT)lParam)[0] = gMouseThresh1;
        ((LPINT)lParam)[1] = gMouseThresh2;
        ((LPINT)lParam)[2] = gMouseSpeed;
        break;

    case SPI_SETMOUSE:
        if (fAlterWinIni) {
            BOOL bWritten1, bWritten2, bWritten3;

            pProfileUserName = CreateProfileUserName(&tlName);
            bWritten1 = UpdateWinIniInt(pProfileUserName, PMAP_MOUSE, STR_MOUSETHRESH1, ((LPINT)lParam)[0]);
            bWritten2 = UpdateWinIniInt(pProfileUserName, PMAP_MOUSE, STR_MOUSETHRESH2, ((LPINT)lParam)[1]);
            bWritten3 = UpdateWinIniInt(pProfileUserName, PMAP_MOUSE, STR_MOUSESPEED,   ((LPINT)lParam)[2]);
            if (bWritten1 && bWritten2 && bWritten3)
                fWinIniChanged = TRUE;
            else {

                /*
                 * Attempt to backout any changes.
                 */
                if (bWritten1) {
                    UpdateWinIniInt(pProfileUserName, PMAP_MOUSE, STR_MOUSETHRESH1, gMouseThresh1);
                }
                if (bWritten2) {
                    UpdateWinIniInt(pProfileUserName, PMAP_MOUSE, STR_MOUSETHRESH2, gMouseThresh2);
                }
                if (bWritten3) {
                    UpdateWinIniInt(pProfileUserName, PMAP_MOUSE, STR_MOUSESPEED,   gMouseSpeed);
                }
            }
            fWriteAllowed = fWinIniChanged;
            FreeProfileUserName(pProfileUserName, &tlName);
        }
        if (fWriteAllowed) {
            gMouseThresh1 = ((LPINT)lParam)[0];
            gMouseThresh2 = ((LPINT)lParam)[1];
            gMouseSpeed = ((LPINT)lParam)[2];
        }
        break;

    case SPI_GETSNAPTODEFBUTTON:
        (*(LPBOOL)lParam) = TEST_BOOL_PUSIF(PUSIF_SNAPTO);
        break;

    case SPI_SETSNAPTODEFBUTTON:
        wParam = (wParam != 0);

        if (fAlterWinIni) {
            fWinIniChanged = UpdateWinIniInt(NULL, PMAP_MOUSE, STR_SNAPTO, wParam);
            fWriteAllowed = fWinIniChanged;
        }

        if (fWriteAllowed) {
            SET_OR_CLEAR_PUSIF(PUSIF_SNAPTO, wParam);
        }

        break;

    case SPI_GETBORDER:
        (*(LPINT)lParam) = gpsi->gclBorder;
        break;

    case SPI_SETBORDER:
        pProfileUserName = CreateProfileUserName(&tlName);
        if (fAlterWinIni) {
            fWinIniChanged = SetWindowMetricInt(pProfileUserName, STR_BORDERWIDTH, wParam);
            fWriteAllowed = fWinIniChanged;
        }
        if (fWriteAllowed) {
            clBorderOld = gpsi->gclBorder;
            clBorderNew = wParam;

            if (clBorderNew < 1)
                clBorderNew = 1;
            else if (clBorderNew > 50)
                clBorderNew = 50;

            if (clBorderOld == clBorderNew) {

                /*
                 * If border size doesn't change, don't waste time.
                 */
                FreeProfileUserName(pProfileUserName, &tlName);
                break;
            }

            xxxSetAndDrawNCMetrics(pProfileUserName, clBorderNew, NULL);

            /*
             * Nice magic number of 3.  So if the border is set to 1, there are actualy
             * 4 pixels in the border
             */

            bSetDevDragWidth(gpDispInfo->hDev, gpsi->gclBorder + BORDER_EXTRA);
        }
        FreeProfileUserName(pProfileUserName, &tlName);
        break;

    case SPI_GETFONTSMOOTHING:
        (*(LPINT)lParam) = !!(GreGetFontEnumeration() & FE_AA_ON);
        break;

    case SPI_SETFONTSMOOTHING:
        wParam = (wParam ? FE_AA_ON : 0);
        if (fAlterWinIni) {
            fWinIniChanged = UpdateWinIniInt(NULL,PMAP_DESKTOP, STR_FONTSMOOTHING, wParam);
            fWriteAllowed = fWinIniChanged;
        }
        if (fWriteAllowed) {
            GreSetFontEnumeration(wParam | FE_SET_AA);
        }
        break;

    case SPI_GETKEYBOARDSPEED:
        (*(int *)lParam) = (gnKeyboardSpeed & KSPEED_MASK);
        break;

    case SPI_SETKEYBOARDSPEED:
        /*
         * Limit the range to max value; SetKeyboardRate takes both speed and delay
         */
        if (wParam > KSPEED_MASK)           // KSPEED_MASK == KSPEED_MAX
            wParam = KSPEED_MASK;
        if (fAlterWinIni) {
            fWinIniChanged = UpdateWinIniInt(NULL,PMAP_KEYBOARD, STR_KEYSPEED, wParam);
            fWriteAllowed = fWinIniChanged;
        }
        if (fWriteAllowed) {
            gnKeyboardSpeed = (gnKeyboardSpeed & ~KSPEED_MASK) | wParam;
            SetKeyboardRate(gnKeyboardSpeed );
        }
        break;

    case SPI_GETKEYBOARDDELAY:
        (*(int *)lParam) = (gnKeyboardSpeed & KDELAY_MASK) >> KDELAY_SHIFT;
        break;

    case SPI_SETKEYBOARDDELAY:
        if (fAlterWinIni) {
            fWinIniChanged = UpdateWinIniInt(NULL,PMAP_KEYBOARD, STR_KEYDELAY, wParam);
            fWriteAllowed = fWinIniChanged;
        }
        if (fWriteAllowed) {
            gnKeyboardSpeed = (gnKeyboardSpeed & ~KDELAY_MASK) | (wParam << KDELAY_SHIFT);
            SetKeyboardRate(gnKeyboardSpeed);
        }
        break;

    case SPI_SETLANGTOGGLE:

        /*
         * wParam unused, lParam unused.  Simply reread the registry setting.
         */

        return GetKbdLangSwitch(NULL);

        break;

    case SPI_GETDEFAULTINPUTLANG:
        /*
         * wParam unused.  lParam is a pointer to buffer to store hkl.
         */
        UserAssert(gspklBaseLayout != NULL);
        (*(HKL *)lParam) = gspklBaseLayout->hkl;
        break;

    case SPI_SETDEFAULTINPUTLANG: {
        PKL pkl;
        /*
         * wParam unused.  lParam is new language of hkl (depending on whether the
         * hiword is set.
         */
        pkl = HKLtoPKL(PtiCurrent(), *(HKL *)lParam);
        if (pkl == NULL) {
            return FALSE;
        }
        if (fWriteAllowed) {
            Lock(&gspklBaseLayout, pkl);
        }
        break;
    }

    case SPI_ICONHORIZONTALSPACING:
        if (IS_PTR(lParam)) {
            *(LPINT)lParam = SYSMET(CXICONSPACING);
        } else if (wParam) {

            /*
             * Make sure icon spacing is reasonable.
             */
            wParam = max(wParam, (DWORD)SYSMET(CXICON));

            if (fAlterWinIni) {
                fWinIniChanged = SetWindowMetricInt(NULL, STR_ICONHORZSPACING, wParam );
                fWriteAllowed = fWinIniChanged;
            }
            if (fWriteAllowed) {
                SYSMET(CXICONSPACING) = (UINT)wParam;
            }
        }
        break;

    case SPI_ICONVERTICALSPACING:
        if (IS_PTR(lParam)) {
            *(LPINT)lParam = SYSMET(CYICONSPACING);
        } else if (wParam) {
            wParam = max(wParam, (DWORD)SYSMET(CYICON));

            if (fAlterWinIni) {
                fWinIniChanged = SetWindowMetricInt(NULL, STR_ICONVERTSPACING, wParam);
                fWriteAllowed = fWinIniChanged;
            }
            if (fWriteAllowed) {
                SYSMET(CYICONSPACING) = (UINT)wParam;
            }
        }
        break;

    case SPI_GETSCREENSAVETIMEOUT:
        piTimeOut = &giScreenSaveTimeOutMs;
        goto HandleGetTimeouts;

    case SPI_GETLOWPOWERTIMEOUT:
        if (!NT_SUCCESS(DrvGetMonitorPowerState(gpDispInfo->pmdev, PowerDeviceD1))) {
            return FALSE;
        }
        piTimeOut = &giLowPowerTimeOutMs;
        goto HandleGetTimeouts;

    case SPI_GETPOWEROFFTIMEOUT:
        if (!NT_SUCCESS(DrvGetMonitorPowerState(gpDispInfo->pmdev, PowerDeviceD3))) {
            return FALSE;
        }
        piTimeOut = &giPowerOffTimeOutMs;

HandleGetTimeouts:
        /*
         * If the screen saver is disabled, I store this fact as a negative
         * time out value.  So, we give the Control Panel the absolute value
         * of the screen save time out. We store this in milliseconds.
         */
        if (*piTimeOut < 0)
            (*(int *)lParam) = -*piTimeOut / 1000;
        else
            (*(int *)lParam) = *piTimeOut / 1000;
        break;

    case SPI_SETSCREENSAVETIMEOUT:
        piTimeOut = &giScreenSaveTimeOutMs;
        iResID = STR_SCREENSAVETIMEOUT;
        goto HandleSetTimeouts;

    case SPI_SETLOWPOWERTIMEOUT:
        if (!NT_SUCCESS(DrvGetMonitorPowerState(gpDispInfo->pmdev, PowerDeviceD1))) {
            return FALSE;
        }
        piTimeOut = &giLowPowerTimeOutMs;
        iResID = STR_LOWPOWERTIMEOUT;
        goto HandleSetTimeouts;

    case SPI_SETPOWEROFFTIMEOUT:
        if (!NT_SUCCESS(DrvGetMonitorPowerState(gpDispInfo->pmdev, PowerDeviceD3))) {
            return FALSE;
        }
        piTimeOut = &giPowerOffTimeOutMs;
        iResID = STR_POWEROFFTIMEOUT;

HandleSetTimeouts:
        /*
         * Maintain the screen save active/inactive state when setting the
         * time out value.  Timeout value is given in seconds but stored
         * in milliseconds
         */
        if (CheckDesktopPolicy(NULL, (PCWSTR)iResID)) {
            fAlterWinIni = FALSE;
            fWriteAllowed = FALSE;
        }
        if (fAlterWinIni) {
            fWinIniChanged = UpdateWinIniInt(NULL,PMAP_DESKTOP, iResID, wParam);
            fWriteAllowed = fWinIniChanged;
        }
        if (fWriteAllowed) {
            if (glinp.dwFlags & LINP_POWERTIMEOUTS) {
                // Call video driver here to exit power down mode.
                KdPrint(("Exit video power down mode\n"));
                DrvSetMonitorPowerState(gpDispInfo->pmdev, PowerDeviceD0);
            }
            glinp.dwFlags &= ~LINP_INPUTTIMEOUTS;
            glinp.timeLastInputMessage = NtGetTickCount();
            if (*piTimeOut < 0) {
                *piTimeOut = -((int)wParam);
            } else {
                *piTimeOut = wParam;
            }
            *piTimeOut *= 1000;
        }
        break;

    case SPI_GETSCREENSAVEACTIVE:
        (*(BOOL *)lParam) = (giScreenSaveTimeOutMs > 0);
        break;

    case SPI_GETLOWPOWERACTIVE:
        if (!NT_SUCCESS(DrvGetMonitorPowerState(gpDispInfo->pmdev, PowerDeviceD1))) {
            return FALSE;
        }
        (*(BOOL *)lParam) = (giLowPowerTimeOutMs > 0);
        break;

    case SPI_GETPOWEROFFACTIVE:
        if (!NT_SUCCESS(DrvGetMonitorPowerState(gpDispInfo->pmdev, PowerDeviceD3))) {
            return FALSE;
        }
        (*(BOOL *)lParam) = (giPowerOffTimeOutMs > 0);
        break;

    case SPI_SETSCREENSAVEACTIVE:
        piTimeOut = &giScreenSaveTimeOutMs;
        iResID = STR_SCREENSAVEACTIVE;
        goto HandleSetActive;

    case SPI_SETLOWPOWERACTIVE:
        if (!NT_SUCCESS(DrvGetMonitorPowerState(gpDispInfo->pmdev, PowerDeviceD1))) {
            return FALSE;
        }
        piTimeOut = &giLowPowerTimeOutMs;
        iResID = STR_LOWPOWERACTIVE;
        goto HandleSetActive;

    case SPI_SETPOWEROFFACTIVE:
        if (!NT_SUCCESS(DrvGetMonitorPowerState(gpDispInfo->pmdev, PowerDeviceD3))) {
            return FALSE;
        }
        piTimeOut = &giPowerOffTimeOutMs;
        iResID = STR_POWEROFFACTIVE;

HandleSetActive:
        wParam = (wParam != 0);

        if (CheckDesktopPolicy(NULL, (PCWSTR)iResID)) {
            fAlterWinIni = FALSE;
            fWriteAllowed = FALSE;
        }
        if (fAlterWinIni) {
            fWinIniChanged = UpdateWinIniInt(NULL,PMAP_DESKTOP, iResID, wParam);
            fWriteAllowed = fWinIniChanged;
        }
        if (fWriteAllowed) {
            if (glinp.dwFlags & LINP_POWERTIMEOUTS) {
                // Call video driver here to exit power down mode.
                KdPrint(("Exit video power down mode\n"));
                DrvSetMonitorPowerState(gpDispInfo->pmdev, PowerDeviceD0);
            }
            glinp.dwFlags &= ~LINP_INPUTTIMEOUTS;
            glinp.timeLastInputMessage = NtGetTickCount();
            if ((*piTimeOut < 0 && wParam) ||
                (*piTimeOut >= 0 && !wParam)) {
                *piTimeOut = -*piTimeOut;
            }
        }
        break;


    case SPI_SETDESKWALLPAPER:
        pProfileUserName = CreateProfileUserName(&tlName);
        if (fAlterWinIni) {

            if (wParam != (WPARAM)-1) {

                /*
                 * Save current wallpaper in case of failure.
                 */
                FastGetProfileStringFromIDW(pProfileUserName,
                       PMAP_DESKTOP,
                       STR_DTBITMAP,
                       TEXT(""),
                       szPat,
                       sizeof(szPat) / sizeof(WCHAR)
                       );

                fWinIniChanged = FastUpdateWinIni(pProfileUserName,
                       PMAP_DESKTOP,
                       STR_DTBITMAP,
                       (LPWSTR)lParam
                       );

                fWriteAllowed = fWinIniChanged;

            } else {
                fWriteAllowed = TRUE;
            }
        }

        if (fWriteAllowed) {

            if (xxxSetDeskWallpaper(pProfileUserName,(LPWSTR)lParam)) {

                if (grpdeskRitInput) {

                    xxxInternalInvalidate(grpdeskRitInput->pDeskInfo->spwnd,
                                          HRGN_FULL,
                                          RDW_INVALIDATE |
                                              RDW_ERASE |
                                              RDW_FRAME |
                                              RDW_ALLCHILDREN);
                }

            } else if (fAlterWinIni && (wParam != 0xFFFFFFFF)) {

                /*
                 * Backout any change to win.ini.
                 */
                FastUpdateWinIni(pProfileUserName,PMAP_DESKTOP, STR_DTBITMAP, szPat);
                fWinIniChanged = FALSE;
                fWriteAllowed = fWinIniChanged;
            } else if (!fAlterWinIni) {
                /*
                 * Bug 304109 - joejo
                 * Make sure we return a 0 retval if we didn't do anything!
                 */
                fWinIniChanged = FALSE;
                fWriteAllowed = fWinIniChanged;
            }
        }
        FreeProfileUserName(pProfileUserName, &tlName);
        break;

    /*
     * Bug 257718 - joejo
     * Add SPI_GETDESKWALLPAPER to SystemParametersInfo
     */
    case SPI_GETDESKWALLPAPER:
        /*
         * Bug 283318 - jojeo
         *
         * Get the string from the gobal var, not the registry,
         * as it's more current.
         */
        if (gpszWall != NULL) {
            /*
             * Copy the global wallpaper name ONLY if nun null
             */
            wcscpy(lParam, gpszWall);
        } else {
            /*
             * Null out the string so no garbage can corrupt the user's
             * buffer.
             */
            (*(LPWSTR)lParam) = (WCHAR)0;
        }
        break;

    case SPI_SETDESKPATTERN: {
            BOOL fRet;

            if (wParam == -1 && lParam != 0)
                return FALSE;

            pProfileUserName = CreateProfileUserName(&tlName);
            if (fAlterWinIni && wParam != -1) {

                /*
                 * Save the current pattern in case of failure.
                 */
                FastGetProfileStringFromIDW(pProfileUserName,
                        PMAP_DESKTOP,
                        STR_DESKPATTERN,
                        TEXT(""),
                        szPat,
                        sizeof(szPat) / sizeof(WCHAR)
                        );

                fWinIniChanged = FastUpdateWinIni(pProfileUserName,
                        PMAP_DESKTOP,
                        STR_DESKPATTERN,
                        (LPWSTR)lParam
                        );

                fWriteAllowed = fWinIniChanged;
            }

            if (fWriteAllowed) {

                fRet = xxxSetDeskPattern(pProfileUserName,
                        wParam == -1 ? (LPWSTR)-1 : (LPWSTR)lParam,
                        FALSE);

                if (!fRet) {

                    /*
                     * Back out any change to win.ini
                     */
                    if (fAlterWinIni && wParam != -1) {

                        FastUpdateWinIni(pProfileUserName,
                                PMAP_DESKTOP,
                                STR_DESKPATTERN,
                                szPat
                                );
                    }

                    FreeProfileUserName(pProfileUserName, &tlName);
                    return FALSE;
                }
            }
        }
        FreeProfileUserName(pProfileUserName, &tlName);
        break;

    case SPI_GETICONTITLEWRAP:
        *((int *)lParam) = TEST_BOOL_PUDF(PUDF_ICONTITLEWRAP);
        break;

    case SPI_SETICONTITLEWRAP:
        wParam = (wParam != 0);
        if (fAlterWinIni) {
            fWinIniChanged = SetWindowMetricInt(NULL, STR_ICONTITLEWRAP, wParam);
            fWriteAllowed = fWinIniChanged;
        }
        if (fWriteAllowed) {
            SET_OR_CLEAR_PUDF(PUDF_ICONTITLEWRAP, wParam);
            xxxMetricsRecalc(CALC_FRAME, 0, 0, 0, 0);
        }
        break;

    case SPI_SETDRAGWIDTH:
        if (fAlterWinIni) {
            fWinIniChanged = UpdateWinIniInt(NULL, PMAP_DESKTOP, STR_DRAGWIDTH, wParam);
            fWriteAllowed = fWinIniChanged;
        }
        if (fWriteAllowed) {
            SYSMET(CXDRAG) = wParam;
        }
        break;

    case SPI_SETDRAGHEIGHT:
        if (fAlterWinIni) {
            fWinIniChanged = UpdateWinIniInt(NULL, PMAP_DESKTOP, STR_DRAGHEIGHT, wParam);
            fWriteAllowed = fWinIniChanged;
        }
        if (fWriteAllowed) {
            SYSMET(CYDRAG) = wParam;
        }
        break;

    case SPI_GETMENUDROPALIGNMENT:
        (*(int *)lParam) = (SYSMET(MENUDROPALIGNMENT));
        break;

    case SPI_SETMENUDROPALIGNMENT:
        if (fAlterWinIni) {
            fWinIniChanged = UpdateWinIniInt(NULL, PMAP_WINDOWSU, STR_MENUDROPALIGNMENT, wParam);
            fWriteAllowed = fWinIniChanged;
        }
        if (fWriteAllowed) {
            SYSMET(MENUDROPALIGNMENT) = (BOOL)(wParam != 0);
        }
        break;

    case SPI_SETDOUBLECLKWIDTH:
        if (fAlterWinIni) {
            fWinIniChanged = UpdateWinIniInt(NULL, PMAP_MOUSE, STR_DOUBLECLICKWIDTH, wParam);
            fWriteAllowed = fWinIniChanged;
        }
        if (fWriteAllowed) {
            SYSMET(CXDOUBLECLK) = wParam;
        }
        break;

    case SPI_SETDOUBLECLKHEIGHT:
        if (fAlterWinIni) {
            fWinIniChanged = UpdateWinIniInt(NULL, PMAP_MOUSE, STR_DOUBLECLICKHEIGHT, wParam);
            fWriteAllowed = fWinIniChanged;
        }
        if (fWriteAllowed) {
            SYSMET(CYDOUBLECLK) = wParam;
        }
        break;

    case SPI_GETICONTITLELOGFONT:
        GreExtGetObjectW(ghIconFont, sizeof(LOGFONTW), lParam);
        break;

    case SPI_SETICONTITLELOGFONT:
    {
        if (lParam != NULL) {
            if (wParam != sizeof(LOGFONTW))
                return FALSE;
        } else if (wParam) {
            return FALSE;
        }

        pProfileUserName = CreateProfileUserName(&tlName);
        fWinIniChanged = xxxSetSPIMetrics(pProfileUserName, wFlag, lParam, fAlterWinIni);
        FreeProfileUserName(pProfileUserName, &tlName);
        if (fAlterWinIni) {
            fWriteAllowed = fWinIniChanged;
        }
        break;
    }

    case SPI_SETDOUBLECLICKTIME:
        if (fAlterWinIni) {
            fWinIniChanged = UpdateWinIniInt(NULL,PMAP_MOUSE, STR_DBLCLKSPEED, wParam);
            fWriteAllowed = fWinIniChanged;
        }
        if (fWriteAllowed) {
            _SetDoubleClickTime((UINT)wParam);
        }
        break;

    case SPI_GETANIMATION: {
        LPANIMATIONINFO lpai = (LPANIMATIONINFO) lParam;

        if (lpai == NULL || (wParam != sizeof(ANIMATIONINFO)))
            return(FALSE);

        lpai->cbSize        = sizeof(ANIMATIONINFO);
        lpai->iMinAnimate   = TEST_BOOL_PUDF(PUDF_ANIMATE);

        break;
    }

    case SPI_GETNONCLIENTMETRICS: {
        LPNONCLIENTMETRICS lpnc = (LPNONCLIENTMETRICS) lParam;
        if (lpnc == NULL)
            return FALSE;

        GetWindowNCMetrics(lpnc);
        break;
    }

    case SPI_GETMINIMIZEDMETRICS: {
        LPMINIMIZEDMETRICS lpmin = (LPMINIMIZEDMETRICS)lParam;

        lpmin->cbSize        = sizeof(MINIMIZEDMETRICS);

            lpmin->iWidth    = SYSMET(CXMINIMIZED) - 2*SYSMET(CXFIXEDFRAME);
            lpmin->iHorzGap  = SYSMET(CXMINSPACING) - SYSMET(CXMINIMIZED);
            lpmin->iVertGap  = SYSMET(CYMINSPACING) - SYSMET(CYMINIMIZED);
            lpmin->iArrange  = SYSMET(ARRANGE);

        break;
    }

    case SPI_GETICONMETRICS: {
        LPICONMETRICS lpicon = (LPICONMETRICS)lParam;

        lpicon->cbSize          = sizeof(ICONMETRICS);

        lpicon->iHorzSpacing    = SYSMET(CXICONSPACING);
        lpicon->iVertSpacing    = SYSMET(CYICONSPACING);
        lpicon->iTitleWrap      = TEST_BOOL_PUDF(PUDF_ICONTITLEWRAP);
        GreExtGetObjectW(ghIconFont, sizeof(LOGFONTW), &(lpicon->lfFont));

        break;
    }

    case SPI_SETANIMATION:
    case SPI_SETNONCLIENTMETRICS:
    case SPI_SETICONMETRICS:
    case SPI_SETMINIMIZEDMETRICS:
    {
        fWinIniChanged = xxxSetSPIMetrics(NULL, wFlag, lParam, fAlterWinIni);
        if (fAlterWinIni) {
            fWriteAllowed = fWinIniChanged;
        }
        ServerLoadString(hModuleWin, STR_METRICS, szSection, ARRAY_SIZE(szSection));
        break;
    }
    case SPI_SETMOUSEBUTTONSWAP:
        if (fAlterWinIni) {
            fWinIniChanged = UpdateWinIniInt(NULL, PMAP_MOUSE, STR_SWAPBUTTONS, wParam);
            fWriteAllowed = fWinIniChanged;
        }
        if (fWriteAllowed) {
            _SwapMouseButton((wParam != 0));
        }
        break;

    case SPI_GETFASTTASKSWITCH:
        *((PINT)lParam) = TRUE;    // do the work so we don't screw anybody

    case SPI_SETFASTTASKSWITCH:
        RIPMSG0(RIP_WARNING,"SPI_SETFASTTASKSWITCH and SPI_GETFASTTASKSWITCH are obsolete actions.");
        break;

    case SPI_GETWORKAREA:
        CopyRect((LPRECT)lParam, &GetPrimaryMonitor()->rcWork);
        break;

    case SPI_SETWORKAREA:
    {
        RECT        rcNewWork;
        LPRECT      lprcNewWork;
        PMONITOR    pMonitorWork;

        lprcNewWork = (LPRECT)lParam;

        /*
         * Validate Rectangle
         */
        if ((lprcNewWork != NULL) &&
            ((lprcNewWork->right < lprcNewWork->left) ||
             (lprcNewWork->bottom < lprcNewWork->top))) {

            RIPMSG0(RIP_WARNING, "Bad work rectangle passed to SystemParametersInfo(SPI_SETWORKAREA, ...)\n");
            return FALSE;
        }

        /*
         * Figure out which monitor has the working area.
         */
        if (!lprcNewWork) {
            pMonitorWork = GetPrimaryMonitor();
            lprcNewWork = &pMonitorWork->rcMonitor;
        } else {
            pMonitorWork = _MonitorFromRect(lprcNewWork, MONITOR_DEFAULTTOPRIMARY);
        }

        /*
         * Get new working area, clipped to monitor of course.
         */
        if (!IntersectRect(&rcNewWork, lprcNewWork, &pMonitorWork->rcMonitor) ||
            !EqualRect(&rcNewWork, lprcNewWork))
        {
            /*
             * Complain.
             */
            RIPERR4(
                    ERROR_INVALID_PARAMETER,
                    RIP_WARNING,
                    "Bad work rectangle passed to SystemParametersInfo(SPI_SETWORKAREA, ...) %d, %d, %d, %d",
                    lprcNewWork->left, lprcNewWork->top, lprcNewWork->right, lprcNewWork->bottom);
            return FALSE;
        }

        if (!EqualRect(&pMonitorWork->rcWork, &rcNewWork))
        {
            PMONITORRECTS   pmr;

            /*
             * If we are going to reposition windows, remember the old
             * monitor positions for xxxDesktopRecalc.
             */
            if (wParam) {
                pmr = SnapshotMonitorRects();
                if (!pmr) {
                    return FALSE;
                }
            }

            pMonitorWork->rcWork = rcNewWork;
            if (pMonitorWork == GetPrimaryMonitor()) {
                SetDesktopMetrics();
            }

            /*
             * Reposition windows
             */

            if (wParam) {

                TL tlPool;

                ThreadLockPool(PtiCurrent(), pmr, &tlPool);
                xxxDesktopRecalc(pmr);
                ThreadUnlockAndFreePool(PtiCurrent(), &tlPool);
            }

            fWinIniChanged = TRUE;
        }

        fWriteAllowed = TRUE;
        break;
    }

    case SPI_SETDRAGFULLWINDOWS:
        wParam = (wParam == 1);
        if (fAlterWinIni) {
            fWinIniChanged = UpdateWinIniInt(NULL, PMAP_DESKTOP, STR_DRAGFULLWINDOWS, wParam);
            fWriteAllowed = fWinIniChanged;
        }
        if (fWriteAllowed) {
            SET_OR_CLEAR_PUDF(PUDF_DRAGFULLWINDOWS, wParam);
        }
        break;

    case SPI_GETDRAGFULLWINDOWS:
        *((PINT)lParam) = TEST_BOOL_PUDF(PUDF_DRAGFULLWINDOWS);
        break;

    case SPI_GETFILTERKEYS:
        {
            LPFILTERKEYS pFK = (LPFILTERKEYS)lParam;
            int cbSkip = sizeof(gFilterKeys.cbSize);

            if ((wParam != 0) && (wParam != sizeof(FILTERKEYS))) {
                return FALSE;
            }
            if (!pFK || (pFK->cbSize != sizeof(FILTERKEYS))) {
                return FALSE;
            }
            /*
             * In the future we may support multiple sizes of this data structure.  Don't
             * change the cbSize field of the data structure passed in.
             */
            RtlCopyMemory((LPVOID)((LPBYTE)pFK + cbSkip),
                          (LPVOID)((LPBYTE)&gFilterKeys + cbSkip),
                          pFK->cbSize - cbSkip);
        }
        break;

    case SPI_SETFILTERKEYS:
        {
            LPFILTERKEYS pFK = (LPFILTERKEYS)lParam;

            if ((wParam != 0) && (wParam != sizeof(FILTERKEYS))) {
                return FALSE;
            }
            if (!pFK || (pFK->cbSize != sizeof(FILTERKEYS)))
                return FALSE;

            /*
             * SlowKeys and BounceKeys cannot both be active simultaneously
             */
            if (pFK->iWaitMSec && pFK->iBounceMSec) {
                return FALSE;
            }

            /*
             * Do some parameter validation.  We will fail on unsupported and
             * undefined bits being set.
             */
            if ((pFK->dwFlags & FKF_VALID) != pFK->dwFlags) {
                return FALSE;
            }
            /*
             * FKF_AVAILABLE can't be set via API.  Use registry value.
             */
            if (TEST_ACCESSFLAG(FilterKeys, FKF_AVAILABLE)) {
                pFK->dwFlags |= FKF_AVAILABLE;
            } else {
                pFK->dwFlags &= ~FKF_AVAILABLE;
            }
            if ((pFK->iWaitMSec > 2000) ||
                (pFK->iDelayMSec > 2000) ||
                (pFK->iRepeatMSec > 2000) ||
                (pFK->iBounceMSec > 2000)) {
                return FALSE;
            }

            if (fAlterWinIni) {
                pProfileUserName = CreateProfileUserName(&tlName);
                fWinIniChanged = SetFilterKeys(pProfileUserName, pFK);
                fWriteAllowed = fWinIniChanged;
                if (!fWinIniChanged) {

                    /*
                     * Back out any changes to win.ini
                     */
                    SetFilterKeys(pProfileUserName, &gFilterKeys);
                }
                FreeProfileUserName(pProfileUserName, &tlName);
            }
            if (fWriteAllowed) {
                RtlCopyMemory(&gFilterKeys, pFK, pFK->cbSize);

                /*
                 * Don't allow user to change cbSize field
                 */
                gFilterKeys.cbSize = sizeof(FILTERKEYS);

                if (!TEST_ACCESSFLAG(FilterKeys, FKF_FILTERKEYSON)) {
                    StopFilterKeysTimers();
                }
                SetAccessEnabledFlag();
                if (FCallHookTray())
                    xxxCallHook(HSHELL_ACCESSIBILITYSTATE,
                                ACCESS_FILTERKEYS,
                                (LONG)0,
                                WH_SHELL);
                PostShellHookMessages(HSHELL_ACCESSIBILITYSTATE, ACCESS_FILTERKEYS);
            }
        }
        break;

    case SPI_GETSTICKYKEYS:
        {
            LPSTICKYKEYS pSK = (LPSTICKYKEYS)lParam;
            int cbSkip = sizeof(gStickyKeys.cbSize);

            if ((wParam != 0) && (wParam != sizeof(STICKYKEYS))) {
                return FALSE;
            }
            if (!pSK || (pSK->cbSize != sizeof(STICKYKEYS))) {
                return FALSE;
            }
            /*
             * In the future we may support multiple sizes of this data structure.  Don't
             * change the cbSize field of the data structure passed in.
             */
            RtlCopyMemory((LPVOID)((LPBYTE)pSK + cbSkip),
                          (LPVOID)((LPBYTE)&gStickyKeys + cbSkip),
                          pSK->cbSize - cbSkip);

            pSK->dwFlags &= ~SKF_STATEINFO;
            pSK->dwFlags |= (gLatchBits&0xff) <<24;

#if SKF_LALTLATCHED != 0x10000000
#error SKF_LALTLATCHED value is incorrect
#endif
#if SKF_LCTLLATCHED != 0x04000000
#error SKF_LCTLLATCHED value is incorrect
#endif
#if SKF_LSHIFTLATCHED != 0x01000000
#error SKF_LSHIFTLATCHED value is incorrect
#endif
#if SKF_RALTLATCHED  !=  0x20000000
#error SKF_RALTLATCHED value is incorrect
#endif
#if  SKF_RCTLLATCHED != 0x08000000
#error SKF_RCTLLATCHED value is incorrect
#endif
#if SKF_RSHIFTLATCHED != 0x02000000
#error SKF_RSHIFTLATCHED value is incorrect
#endif
            pSK->dwFlags |= (gLockBits&0xff) <<16;
#if SKF_LALTLOCKED != 0x00100000
#error SKF_LALTLOCKED value is incorrect
#endif
#if SKF_LCTLLOCKED != 0x00040000
#error SKF_LCTLLOCKED value is incorrect
#endif
#if SKF_LSHIFTLOCKED != 0x00010000
#error SKF_LSHIFTLOCKED value is incorrect
#endif
#if SKF_RALTLOCKED  != 0x00200000
#error SKF_RALTLOCKED value is incorrect
#endif
#if SKF_RCTLLOCKED != 0x00080000
#error SKF_RCTLLOCKED value is incorrect
#endif
#if SKF_RSHIFTLOCKED != 0x00020000
#error SKF_RSHIFTLOCKED value is incorrect
#endif

        }

        break;

    case SPI_SETSTICKYKEYS:
        {
            LPSTICKYKEYS pSK = (LPSTICKYKEYS)lParam;
            BOOL fWasOn;

            fWasOn = TEST_BOOL_ACCESSFLAG(StickyKeys, SKF_STICKYKEYSON);
            if ((wParam != 0) && (wParam != sizeof(STICKYKEYS))) {
                return FALSE;
            }
            if (!pSK || (pSK->cbSize != sizeof(STICKYKEYS)))
                return FALSE;

            pSK->dwFlags &= ~SKF_STATEINFO;  /* Don't penalize them for using
                                              * data from SPI_GETSTICKYKEYS.
                                              */

            /*
             * Do some parameter validation.  We will fail on unsupported and
             * undefined bits being set.
             */
            if ((pSK->dwFlags & SKF_VALID) != pSK->dwFlags) {
                return FALSE;
            }
            /*
             * SKF_AVAILABLE can't be set via API.  Use registry value.
             */
            if (TEST_ACCESSFLAG(StickyKeys, SKF_AVAILABLE)) {
                pSK->dwFlags |= SKF_AVAILABLE;
            } else {
                pSK->dwFlags &= ~SKF_AVAILABLE;
            }

            if (fAlterWinIni) {
                swprintf(szTemp, pwszd, pSK->dwFlags);
                fWinIniChanged = FastWriteProfileStringW(NULL,
                        PMAP_STICKYKEYS,
                        L"Flags",
                        szTemp
                        );
                fWriteAllowed = fWinIniChanged;
            }
            if (fWriteAllowed) {
                RtlCopyMemory(&gStickyKeys, pSK, pSK->cbSize);

                /*
                 * Don't allow user to change cbSize field
                 */
                gStickyKeys.cbSize = sizeof(STICKYKEYS);
                if (!TEST_ACCESSFLAG(StickyKeys, SKF_STICKYKEYSON) && fWasOn) {
                    xxxTurnOffStickyKeys();
                }

                SetAccessEnabledFlag();
                if (FCallHookTray())
                    xxxCallHook(HSHELL_ACCESSIBILITYSTATE,
                                ACCESS_STICKYKEYS,
                                (LONG)0,
                                WH_SHELL);
                PostShellHookMessages(HSHELL_ACCESSIBILITYSTATE, ACCESS_STICKYKEYS);
            }
        }
        break;

    case SPI_GETTOGGLEKEYS:
        {
            LPTOGGLEKEYS pTK = (LPTOGGLEKEYS)lParam;
            int cbSkip = sizeof(gToggleKeys.cbSize);

            if ((wParam != 0) && (wParam != sizeof(TOGGLEKEYS))) {
                return FALSE;
            }
            if (!pTK || (pTK->cbSize != sizeof(TOGGLEKEYS))) {
                return FALSE;
            }
            /*
             * In the future we may support multiple sizes of this data structure.  Don't
             * change the cbSize field of the data structure passed in.
             */
            RtlCopyMemory((LPVOID)((LPBYTE)pTK + cbSkip),
                          (LPVOID)((LPBYTE)&gToggleKeys + cbSkip),
                          pTK->cbSize - cbSkip);
        }
        break;

    case SPI_SETTOGGLEKEYS:
        {
            LPTOGGLEKEYS pTK = (LPTOGGLEKEYS)lParam;

            if ((wParam != 0) && (wParam != sizeof(TOGGLEKEYS))) {
                return FALSE;
            }
            if (!pTK || (pTK->cbSize != sizeof(TOGGLEKEYS)))
                return FALSE;

            /*
             * Do some parameter validation.  We will fail on unsupported and
             * undefined bits being set.
             */
            if ((pTK->dwFlags & TKF_VALID) != pTK->dwFlags) {
                return FALSE;
            }
            /*
             * TKF_AVAILABLE can't be set via API.  Use registry value.
             */
            if (TEST_ACCESSFLAG(ToggleKeys, TKF_AVAILABLE)) {
                pTK->dwFlags |= TKF_AVAILABLE;
            } else {
                pTK->dwFlags &= ~TKF_AVAILABLE;
            }

            if (fAlterWinIni) {
                swprintf(szTemp, pwszd, pTK->dwFlags);
                fWinIniChanged = FastWriteProfileStringW(NULL,
                        PMAP_TOGGLEKEYS,
                        L"Flags",
                        szTemp
                        );
                fWriteAllowed = fWinIniChanged;
            }
            if (fWriteAllowed) {
                RtlCopyMemory(&gToggleKeys, pTK, pTK->cbSize);

                /*
                 * Don't allow user to change cbSize field
                 */
                gToggleKeys.cbSize = sizeof(TOGGLEKEYS);

                SetAccessEnabledFlag();
            }
        }
        break;

    case SPI_GETMOUSEKEYS:
        {
            LPMOUSEKEYS pMK = (LPMOUSEKEYS)lParam;
            int cbSkip = sizeof(gMouseKeys.cbSize);

            if ((wParam != 0) && (wParam != sizeof(MOUSEKEYS))) {
                return FALSE;
            }
            if (!pMK || (pMK->cbSize != sizeof(MOUSEKEYS))) {
                return FALSE;
            }
            /*
             * In the future we may support multiple sizes of this data structure.  Don't
             * change the cbSize field of the data structure passed in.
             */
            RtlCopyMemory((LPVOID)((LPBYTE)pMK + cbSkip),
                          (LPVOID)((LPBYTE)&gMouseKeys + cbSkip),
                          pMK->cbSize - cbSkip);


            pMK->dwFlags &= ~MKF_STATEINFO;

            if (gbMKMouseMode) pMK->dwFlags |= MKF_MOUSEMODE;

            pMK->dwFlags |= (gwMKButtonState & 3) << 24;
#if MOUSE_BUTTON_LEFT != 0x01
#error MOUSE_BUTTON_LEFT value is incorrect
#endif
#if MOUSE_BUTTON_RIGHT != 0x02
#error MOUSE_BUTTON_RIGHT value is incorrect
#endif
#if MKF_LEFTBUTTONDOWN != 0x01000000
#error MKF_LEFTBUTTONDOWN value is incorrect
#endif
#if MKF_RIGHTBUTTONDOWN != 0x02000000
#error MKF_RIGHTBUTTONDOWN value is incorrect
#endif

            pMK->dwFlags |= (gwMKCurrentButton & 3)<< 28;
#if MKF_LEFTBUTTONSEL != 0x10000000
#error MKF_LEFTBUTTONSEL value is incorrect
#endif
#if MKF_RIGHTBUTTONSEL != 0x20000000
#error MKF_RIGHTBUTTONSEL value is incorrect
#endif
        }
        break;

    case SPI_SETMOUSEKEYS: {
            LPMOUSEKEYS pMK = (LPMOUSEKEYS)lParam;

            if ((wParam != 0) && (wParam != sizeof(MOUSEKEYS))) {
                return FALSE;
            }
            if (!pMK || (pMK->cbSize != sizeof(MOUSEKEYS)))
                return FALSE;

            /*
             * Do some parameter validation.  We will fail on unsupported and
             * undefined bits being set.
             */
            pMK->dwFlags &= ~MKF_STATEINFO;  /* Don't penalize them for using
                                              * data from SPI_GETMOUSEKEYS.
                                              */

            if ((pMK->dwFlags & MKF_VALID) != pMK->dwFlags) {
                return FALSE;
            }
            /*
             * MKF_AVAILABLE can't be set via API.  Use registry value.
             */
            if (TEST_ACCESSFLAG(MouseKeys, MKF_AVAILABLE)) {
                pMK->dwFlags |= MKF_AVAILABLE;
            } else {
                pMK->dwFlags &= ~MKF_AVAILABLE;
            }
            if ((pMK->iMaxSpeed < 10) || (pMK->iMaxSpeed > 360)) {
                return FALSE;
            }
            if ((pMK->iTimeToMaxSpeed < 1000) || (pMK->iTimeToMaxSpeed > 5000)) {
                return FALSE;
            }

            if (fAlterWinIni) {
                pProfileUserName = CreateProfileUserName(&tlName);
                fWinIniChanged = SetMouseKeys(pProfileUserName, pMK);
                fWriteAllowed = fWinIniChanged;
                if (!fWinIniChanged) {

                    /*
                     * Back out any changes to win.ini
                     */
                    SetMouseKeys(pProfileUserName, &gMouseKeys);
                }
                FreeProfileUserName(pProfileUserName, &tlName);
            }
            if (fWriteAllowed) {
                RtlCopyMemory(&gMouseKeys, pMK, pMK->cbSize);

                /*
                 * Don't allow user to change cbSize field
                 */
                gMouseKeys.cbSize = sizeof(MOUSEKEYS);

                CalculateMouseTable();

                if (TEST_ACCESSFLAG(MouseKeys, MKF_MOUSEKEYSON)) {
                    if ((TestAsyncKeyStateToggle(gNumLockVk) != 0) ^
                        (TEST_ACCESSFLAG(MouseKeys, MKF_REPLACENUMBERS) != 0))
                        gbMKMouseMode = TRUE;
                    else
                        gbMKMouseMode = FALSE;
                    MKShowMouseCursor();
                } else {
                    MKHideMouseCursor();
                }

                SetAccessEnabledFlag();

                if (FCallHookTray())
                    xxxCallHook(HSHELL_ACCESSIBILITYSTATE,
                                ACCESS_MOUSEKEYS,
                                (LONG)0,
                                WH_SHELL);
                PostShellHookMessages(HSHELL_ACCESSIBILITYSTATE, ACCESS_MOUSEKEYS);
            }
        }
        break;

    case SPI_GETHIGHCONTRAST:
        {
            LPHIGHCONTRAST pHC = (LPHIGHCONTRAST)lParam;

            /*
             * In the future we may support multiple sizes of this data structure.  Don't
             * change the cbSize field of the data structure passed in.
             */

            pHC->dwFlags = gHighContrast.dwFlags;

            /*
             * A hostile app could deallocate the memory using a second thread,
             * so shelter the copy with a try.
             */
            try {
                RtlCopyMemory(pHC->lpszDefaultScheme, gHighContrastDefaultScheme, MAX_SCHEME_NAME_SIZE * sizeof(WCHAR));
            } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
            }
        }

        break;

    case SPI_SETHIGHCONTRAST:
        {
            LPINTERNALSETHIGHCONTRAST pHC = (LPINTERNALSETHIGHCONTRAST)lParam;
            WCHAR wcDefaultScheme[MAX_SCHEME_NAME_SIZE];

            if (pHC->usDefaultScheme.Length >= MAX_SCHEME_NAME_SIZE*sizeof(WCHAR) )
                return FALSE;

            if (pHC->usDefaultScheme.Buffer) {
                /*
                 * Only set the scheme if the user specifies a scheme.  An empty
                 * buffer is ignored.  We do the copy here so that we don't need to
                 * put a try/except around the WriteProfileString code.
                 */

                try {
                    RtlCopyMemory(wcDefaultScheme, pHC->usDefaultScheme.Buffer, pHC->usDefaultScheme.Length);
                } except (W32ExceptionHandler(TRUE, RIP_WARNING)) {
                    return FALSE;
                }
            }
            wcDefaultScheme[pHC->usDefaultScheme.Length / sizeof(WCHAR)] = 0;

            if (fAlterWinIni) {
                pProfileUserName = CreateProfileUserName(&tlName);
                swprintf(szTemp, pwszd, pHC->dwFlags);
                fWinIniChanged = FastWriteProfileStringW(pProfileUserName,
                        PMAP_HIGHCONTRAST,
                        L"Flags",
                        szTemp
                        );

                fWriteAllowed = fWinIniChanged;
                /*
                 * Note -- we do not write anything if there is no default scheme
                 * from the app.  This is consistent with Win95/Win98 behavior.
                 */

                if (pHC->usDefaultScheme.Buffer) {
                    fWinIniChanged |= FastWriteProfileStringW(pProfileUserName,
                        PMAP_HIGHCONTRAST,
                        TEXT("High Contrast Scheme"),
                        wcDefaultScheme
                        );
                }
                FreeProfileUserName(pProfileUserName, &tlName);

            }
            if (fWriteAllowed) {
                DWORD dwFlagsOld = gHighContrast.dwFlags;
                LPARAM lp = fAlterWinIni?0:ACCESS_HIGHCONTRASTNOREG;

#if (ACCESS_HIGHCONTRASTNOREG | ACCESS_HIGHCONTRASTOFF) != ACCESS_HIGHCONTRASTOFFNOREG
#error ACCESS_HIGHCONTRASTOFF value is incorrect
#endif
#if (ACCESS_HIGHCONTRASTNOREG | ACCESS_HIGHCONTRASTON) != ACCESS_HIGHCONTRASTONNOREG
#error ACCESS_HIGHCONTRASTON value is incorrect
#endif
#if (ACCESS_HIGHCONTRASTNOREG | ACCESS_HIGHCONTRASTCHANGE) != ACCESS_HIGHCONTRASTCHANGENOREG
#error ACCESS_HIGHCONTRASTCHANGE value is incorrect
#endif

                /*
                 * If a NULL is specified in the lpszDefaultScheme, then it is
                 * not changed.  This is consistent with Win95/Win98 behavior.
                 */

                if (pHC->usDefaultScheme.Buffer)
                    wcscpy(gHighContrastDefaultScheme, wcDefaultScheme);

                gHighContrast.dwFlags = pHC->dwFlags;

                /*
                 * now, post message to turn high contrast on or off
                 */

                if (pHC->dwFlags & HCF_HIGHCONTRASTON) {
                    _PostMessage(gspwndLogonNotify, WM_LOGONNOTIFY, LOGON_ACCESSNOTIFY,
                        (dwFlagsOld & HCF_HIGHCONTRASTON)? (ACCESS_HIGHCONTRASTCHANGE | lp):
                                                           (ACCESS_HIGHCONTRASTON | lp));
                } else {
                    _PostMessage(gspwndLogonNotify, WM_LOGONNOTIFY, LOGON_ACCESSNOTIFY, ACCESS_HIGHCONTRASTOFF | lp);
                }

            }
        break;
        }

    case SPI_GETACCESSTIMEOUT:
        {
            LPACCESSTIMEOUT pTO = (LPACCESSTIMEOUT)lParam;
            int cbSkip = sizeof(gAccessTimeOut.cbSize);

            if ((wParam != 0) && (wParam != sizeof(ACCESSTIMEOUT))) {
                return FALSE;
            }
            if (!pTO || (pTO->cbSize != sizeof(ACCESSTIMEOUT))) {
                return FALSE;
            }
            /*
             * In the future we may support multiple sizes of this data structure.  Don't
             * change the cbSize field of the data structure passed in.
             */
            RtlCopyMemory((LPVOID)((LPBYTE)pTO + cbSkip),
                          (LPVOID)((LPBYTE)&gAccessTimeOut + cbSkip),
                          pTO->cbSize - cbSkip);
        }
        break;

    case SPI_SETACCESSTIMEOUT:
        {
            LPACCESSTIMEOUT pTO = (LPACCESSTIMEOUT)lParam;

            if ((wParam != 0) && (wParam != sizeof(ACCESSTIMEOUT))) {
                return FALSE;
            }
            if (!pTO || (pTO->cbSize != sizeof(ACCESSTIMEOUT)))
                return FALSE;

            /*
             * Do some parameter validation.  We will fail on unsupported and
             * undefined bits being set.
             */
            if ((pTO->dwFlags & ATF_VALID) != pTO->dwFlags) {
                return FALSE;
            }
            if (pTO->iTimeOutMSec > 3600000) {
                return FALSE;
            }

            if (fAlterWinIni) {
                pProfileUserName = CreateProfileUserName(&tlName);
                swprintf(szTemp, pwszd, pTO->dwFlags);
                fWinIniChanged = FastWriteProfileStringW(pProfileUserName,
                        PMAP_TIMEOUT,
                        L"Flags",
                        szTemp
                        );

                swprintf(szTemp, pwszd, pTO->iTimeOutMSec);
                fWinIniChanged = FastWriteProfileStringW(pProfileUserName,
                        PMAP_TIMEOUT,
                        L"TimeToWait",
                        szTemp
                        );

                fWriteAllowed = fWinIniChanged;
                if (!fWinIniChanged) {

                    /*
                     * Back out any changes to win.ini
                     */
                    swprintf(szTemp, pwszd, gAccessTimeOut.dwFlags);
                    fWinIniChanged = FastWriteProfileStringW(pProfileUserName,
                            PMAP_TIMEOUT,
                            L"Flags",
                            szTemp
                            );

                    swprintf(szTemp, pwszd, gAccessTimeOut.iTimeOutMSec);
                    fWinIniChanged = FastWriteProfileStringW(pProfileUserName,
                            PMAP_TIMEOUT,
                            L"TimeToWait",
                            szTemp
                            );
                }
                FreeProfileUserName(pProfileUserName, &tlName);
            }
            if (fWriteAllowed) {
                RtlCopyMemory(&gAccessTimeOut, pTO, pTO->cbSize);

                /*
                 * Don't allow user to change cbSize field
                 */
                gAccessTimeOut.cbSize = sizeof(ACCESSTIMEOUT);

                SetAccessEnabledFlag();

                AccessTimeOutReset();
            }
        }
        break;

    case SPI_SETSHOWSOUNDS:
        if (fAlterWinIni) {
            swprintf(szTemp, pwszd, (wParam == 1));
            fWinIniChanged = FastWriteProfileStringW(NULL,
                    PMAP_SHOWSOUNDS,
                    L"On",
                    szTemp
                    );

            fWriteAllowed = fWinIniChanged;
        }
        if (fWriteAllowed) {
            SET_OR_CLEAR_ACCF(ACCF_SHOWSOUNDSON, wParam == 1);
            SetAccessEnabledFlag();

            /*
            * Bug 2079.  Update the System Metrics Info.
            */
            SYSMET(SHOWSOUNDS) = TEST_BOOL_ACCF(ACCF_SHOWSOUNDSON);
        }
        break;

    case SPI_GETSHOWSOUNDS: {
            PINT pint = (int *)lParam;

            *pint = TEST_BOOL_ACCF(ACCF_SHOWSOUNDSON);
        }
        break;

    case SPI_GETKEYBOARDPREF:
        {
            PBOOL pfKeyboardPref = (PBOOL)lParam;

            *pfKeyboardPref = TEST_BOOL_ACCF(ACCF_KEYBOARDPREF);
        }
        break;

    case SPI_SETKEYBOARDPREF:
        {
            BOOL fKeyboardPref = (BOOL)wParam;

            if (fAlterWinIni) {
                fWinIniChanged = FastWriteProfileStringW(NULL,
                        PMAP_KEYBOARDPREF,
                        L"On",
                        (fKeyboardPref) ? L"1" : L"0"
                        );

                fWriteAllowed = fWinIniChanged;
            }
            if (fWriteAllowed)
            {
                SET_OR_CLEAR_ACCF(ACCF_KEYBOARDPREF, wParam);
            }
        }
        break;

    case SPI_GETSCREENREADER:
        {
            PBOOL pfScreenReader = (PBOOL)lParam;

            *pfScreenReader = TEST_BOOL_ACCF(ACCF_SCREENREADER);
        }
        break;

    case SPI_SETSCREENREADER:
        {
            BOOL fScreenReader = (BOOL)wParam;

            if (fAlterWinIni) {
                fWinIniChanged = FastWriteProfileStringW(NULL,
                    PMAP_SCREENREADER,
                    L"On",
                    (fScreenReader) ? L"1" : L"0"
                    );
                fWriteAllowed = fWinIniChanged;
            }
            if (fWriteAllowed)
            {
                SET_OR_CLEAR_ACCF(ACCF_SCREENREADER, wParam);
            }
        }
        break;

    case SPI_GETSOUNDSENTRY:
        {
            LPSOUNDSENTRY pSS = (LPSOUNDSENTRY)lParam;
            int cbSkip = sizeof(gSoundSentry.cbSize);

            if ((wParam != 0) && (wParam != sizeof(SOUNDSENTRY))) {
                return FALSE;
            }
            if (!pSS || (pSS->cbSize != sizeof(SOUNDSENTRY))) {
                return FALSE;
            }
            /*
             * In the future we may support multiple sizes of this data structure.  Don't
             * change the cbSize field of the data structure passed in.
             */
            RtlCopyMemory((LPVOID)((LPBYTE)pSS + cbSkip),
                          (LPVOID)((LPBYTE)&gSoundSentry + cbSkip),
                          pSS->cbSize - cbSkip);
        }
        break;

    case SPI_SETSOUNDSENTRY:
        {
            LPSOUNDSENTRY pSS = (LPSOUNDSENTRY)lParam;

            if ((wParam != 0) && (wParam != sizeof(SOUNDSENTRY))) {
                return FALSE;
            }
            if (!pSS || (pSS->cbSize != sizeof(SOUNDSENTRY)))
                return FALSE;

            /*
             * Do some parameter validation.  We will fail on unsupported and
             * undefined bits being set.
             */
            if ((pSS->dwFlags & SSF_VALID) != pSS->dwFlags) {
                return FALSE;
            }
            /*
             * We don't support SSWF_CUSTOM.
             */
            if (pSS->iWindowsEffect > SSWF_DISPLAY) {
                return FALSE;
            }
            /*
             * No support for non-windows apps.
             */
            if (pSS->iFSTextEffect != SSTF_NONE) {
                return FALSE;
            }
            if (pSS->iFSGrafEffect != SSGF_NONE) {
                return FALSE;
            }
            /*
             * SSF_AVAILABLE can't be set via API.  Use registry value.
             */
            if (TEST_ACCESSFLAG(SoundSentry, SSF_AVAILABLE)) {
                pSS->dwFlags |= SSF_AVAILABLE;
            } else {
                pSS->dwFlags &= ~SSF_AVAILABLE;
            }

            if (fAlterWinIni) {
                pProfileUserName = CreateProfileUserName(&tlName);
                fWinIniChanged = SetSoundSentry(pProfileUserName, pSS);
                fWriteAllowed = fWinIniChanged;
                if (!fWinIniChanged) {

                    /*
                     * Back out any changes to win.ini
                     */
                    SetSoundSentry(pProfileUserName, &gSoundSentry);
                }
                FreeProfileUserName(pProfileUserName, &tlName);
            }
            if (fWriteAllowed) {
                RtlCopyMemory(&gSoundSentry, pSS, pSS->cbSize);

                /*
                 * Don't allow user to change cbSize field
                 */
                gSoundSentry.cbSize = sizeof(SOUNDSENTRY);

                SetAccessEnabledFlag();
            }
        }
        break;

    case SPI_SETCURSORS:
            pProfileUserName = CreateProfileUserName(&tlName);
            xxxUpdateSystemCursorsFromRegistry(pProfileUserName);
            FreeProfileUserName(pProfileUserName, &tlName);

            break;

    case SPI_SETICONS:
            pProfileUserName = CreateProfileUserName(&tlName);
            xxxUpdateSystemIconsFromRegistry(pProfileUserName);
            FreeProfileUserName(pProfileUserName, &tlName);

            break;

    case SPI_GETMOUSEHOVERWIDTH:
        *((UINT *)lParam) = gcxMouseHover;
        break;

    case SPI_SETMOUSEHOVERWIDTH:
        if (fAlterWinIni) {
            fWinIniChanged = UpdateWinIniInt(NULL, PMAP_MOUSE, STR_MOUSEHOVERWIDTH, wParam);
            fWriteAllowed = fWinIniChanged;
        }
        if (fWriteAllowed) {
            gcxMouseHover = wParam;
        }
        break;

    case SPI_GETMOUSEHOVERHEIGHT:
        *((UINT *)lParam) = gcyMouseHover;
        break;

    case SPI_SETMOUSEHOVERHEIGHT:
        if (fAlterWinIni) {
            fWinIniChanged = UpdateWinIniInt(NULL, PMAP_MOUSE, STR_MOUSEHOVERHEIGHT, wParam);
            fWriteAllowed = fWinIniChanged;
        }
        if (fWriteAllowed) {
            gcyMouseHover = wParam;
        }
        break;

    case SPI_GETMOUSEHOVERTIME:
        *((UINT *)lParam) = gdtMouseHover;
        break;

    case SPI_SETMOUSEHOVERTIME:
        if (fAlterWinIni) {
            fWinIniChanged = UpdateWinIniInt(NULL, PMAP_MOUSE, STR_MOUSEHOVERTIME, wParam);
            fWriteAllowed = fWinIniChanged;
        }
        if (fWriteAllowed) {
            gdtMouseHover = wParam;
        }
        break;

    case SPI_GETWHEELSCROLLLINES:
        (*(LPDWORD)lParam) = gpsi->ucWheelScrollLines;
        break;

    case SPI_SETWHEELSCROLLLINES:
        if (fAlterWinIni) {
            fWinIniChanged = UpdateWinIniInt(NULL, PMAP_DESKTOP, STR_WHEELSCROLLLINES, wParam);
            fWriteAllowed = fWinIniChanged;
        }

        if (fWriteAllowed)
            gpsi->ucWheelScrollLines = (UINT)wParam;
        break;

    case SPI_GETMENUSHOWDELAY:
        (*(LPDWORD)lParam) = gdtMNDropDown;
        break;

    case SPI_SETMENUSHOWDELAY:
        if (fAlterWinIni) {
            fWinIniChanged = UpdateWinIniInt(NULL, PMAP_DESKTOP, STR_MENUSHOWDELAY, wParam);
            fWriteAllowed = fWinIniChanged;
        }
        if (fWriteAllowed)
            gdtMNDropDown = wParam;
        break;

    case SPI_GETSCREENSAVERRUNNING:
        (*(LPBOOL)lParam) = gppiScreenSaver != NULL;
        break;

    case SPI_SETSHOWIMEUI:
        return xxxSetIMEShowStatus(!!wParam);

    case SPI_GETSHOWIMEUI:
        (*(LPBOOL)lParam) = _GetIMEShowStatus();
        break;

    default:

#define ppvi (UPDWORDPointer(wFlag))
#define uDataRead ((UINT)fWinIniChanged)

        if (wFlag < SPI_MAX) {
            RIPERR1(ERROR_INVALID_SPI_VALUE, RIP_WARNING, "xxxSystemParamtersInfo: Invalid SPI_:%#lx", wFlag);
            return FALSE;
        }

        UserAssert(wFlag & SPIF_RANGETYPEMASK);

        if (!(wFlag & SPIF_SET)) {

            if ((wFlag & SPIF_RANGETYPEMASK) == SPIF_BOOL) {
                BOOL fDisable, fDisableValue;

                UserAssert(UPIsBOOLRange(wFlag));
                /*
                 * Handle settings that can be disabled by additional conditions.
                 */
                fDisable = fDisableValue = FALSE;
                if (wFlag < SPI_GETUIEFFECTS) {
                    if (!TestUP(UIEFFECTS)) {
                        switch (wFlag) {
                        case SPI_GETACTIVEWNDTRKZORDER:
                        case SPI_GETACTIVEWINDOWTRACKING:
                            break;

                        case SPI_GETKEYBOARDCUES:
                            fDisableValue = TRUE;
                            /* Fall Through */

                        default:
                            fDisable = TRUE;
                            break;
                        }
                    } else { /* if (!TestUP(UIEFFECTS) */
                        switch (wFlag) {
                        case SPI_GETKEYBOARDCUES:
                            if (TEST_BOOL_ACCF(ACCF_KEYBOARDPREF)) {
                                fDisableValue = TRUE;
                                fDisable = TRUE;
                            }
                            break;

                        case SPI_GETGRADIENTCAPTIONS:
                        case SPI_GETSELECTIONFADE:
                        case SPI_GETMENUFADE:
                        case SPI_GETTOOLTIPFADE:
                        case SPI_GETCURSORSHADOW:
                            if (gbDisableAlpha) {
                                fDisable = TRUE;
                            }
                            break;
                        }
                    }
                } /* if (wFlag < SPI_GETUIEFFECTS) */
                /*
                 * Give them the disabled value or read the actual one
                 */
                if (fDisable) {
                    *((BOOL *)lParam) = fDisableValue;
                } else {
                    *((BOOL *)lParam) = !!TestUPBOOL(gpdwCPUserPreferencesMask, wFlag);
                }
            } else {
                UserAssert(UPIsDWORDRange(wFlag));
                *((DWORD *)lParam) = UPDWORDValue(wFlag);
            }

        } else { /* if (!(wFlag & SPIF_SET)) */
            pProfileUserName = CreateProfileUserName(&tlName);

            if ((wFlag & SPIF_RANGETYPEMASK) == SPIF_BOOL) {
                DWORD pdwValue [SPI_BOOLMASKDWORDSIZE];

                UserAssert(UPIsBOOLRange(wFlag));
                UserAssert(sizeof(pdwValue) == sizeof(gpdwCPUserPreferencesMask));

                if (fAlterWinIni) {
                    /*
                     * We only need to set/clear the bit passed in, however, we write the whole
                     *  bit mask to the registry. Since the info in gpdwCPUserPreferencesMask
                     *  might not match what it is in the registry, we need to read the registry before
                     *  we write to it.
                     */
                    uDataRead = FastGetProfileValue(pProfileUserName,
                            gpviCPUserPreferences->uSection,
                            gpviCPUserPreferences->pwszKeyName,
                            NULL,
                            (LPBYTE)pdwValue,
                            sizeof(pdwValue)
                            );

                    /*
                     * If some bits are not in the registry, get them from gpdwCPUserPreferencesMask
                     */
                    UserAssert(uDataRead <= sizeof(gpdwCPUserPreferencesMask));
                    RtlCopyMemory(pdwValue + uDataRead,
                                  gpdwCPUserPreferencesMask + uDataRead,
                                  sizeof(gpdwCPUserPreferencesMask) - uDataRead);

                    /*
                     * Set/Clear the new state and write it
                     */
                    if (lParam) {
                        SetUPBOOL(pdwValue, wFlag);
                    } else {
                        ClearUPBOOL(pdwValue, wFlag);
                    }

                    fWinIniChanged = FastWriteProfileValue(pProfileUserName,
                            gpviCPUserPreferences->uSection,
                            gpviCPUserPreferences->pwszKeyName,
                            REG_BINARY,
                            (LPBYTE)pdwValue,
                            sizeof(pdwValue)
                            );

                    fWriteAllowed = fWinIniChanged;
                }

                if (fWriteAllowed) {
                    if (lParam) {
                        SetUPBOOL(gpdwCPUserPreferencesMask, wFlag);
                    } else {
                        ClearUPBOOL(gpdwCPUserPreferencesMask, wFlag);
                    }

                    /*
                     * Propagate gpsi flags
                     */
                    switch (wFlag) {
                    case SPI_SETUIEFFECTS:
                        PropagetUPBOOLTogpsi(UIEFFECTS);
                        SetPointer(TRUE);
                        /*
                         * Fall through
                         */

                    case SPI_SETGRADIENTCAPTIONS:
                        CreateBitmapStrip();
                        xxxRedrawScreen();
                        break;

                    case SPI_SETCOMBOBOXANIMATION:
                        PropagetUPBOOLTogpsi(COMBOBOXANIMATION);
                        break;

                    case SPI_SETLISTBOXSMOOTHSCROLLING:
                        PropagetUPBOOLTogpsi(LISTBOXSMOOTHSCROLLING);
                        break;

                    case SPI_SETKEYBOARDCUES:
                        PropagetUPBOOLTogpsi(KEYBOARDCUES);
                        break;

                    case SPI_SETCURSORSHADOW:
                        SetPointer(TRUE);
                        break;

                    } /* switch (wFlag) */

                } /* if (fWriteAllowed) */

            } else { /* if ((wFlag & SPIF_RANGETYPEMASK) == SPIF_BOOL) */

                UserAssert(UPIsDWORDRange(wFlag));

                if (fAlterWinIni) {
                    fWinIniChanged = FastWriteProfileValue(pProfileUserName,
                            ppvi->uSection,
                            ppvi->pwszKeyName,
                            REG_DWORD,
                            (LPBYTE)&lParam,
                            sizeof(DWORD)
                            );

                    fWriteAllowed = fWinIniChanged;
                }

                if (fWriteAllowed) {

                    ppvi->dwValue = PtrToUlong(lParam);

                    switch(wFlag) {
                    case SPI_SETCARETWIDTH:
                        gpsi->uCaretWidth = ppvi->dwValue;
                        break;
                    default:
                        break;
                    }
                }

            } /* if ((wFlag & SPIF_RANGETYPEMASK) == SPIF_BOOL) */

            FreeProfileUserName(pProfileUserName, &tlName);
        } /* if (!(wFlag & SPIF_SET)) */

        break;
#undef ppvi
#undef uDataRead
    } /* switch (wFlag) */


    if (fWinIniChanged && fSendWinIniChange) {
        ULONG_PTR dwResult;

        /*
         * dwResult is defined so that xxxSendMessageTimeout will really
         * and truly do a timeout.  Yeah, I know, this is a hack, but,
         * it is compatible.
         */

        RtlInitLargeUnicodeString(&strSection, szSection, (UINT)-1);
        xxxSendMessageTimeout(PWND_BROADCAST, WM_SETTINGCHANGE, wFlag, (LPARAM)&strSection,
                SMTO_NORMAL, 100, &dwResult);
    }

    return fWriteAllowed;
}

/***************************************************************************\
* _RegisterShellHookWindow
*
* History:
\***************************************************************************/

BOOL _RegisterShellHookWindow(PWND pwnd) {
    PDESKTOPINFO pdeskinfo;

    if (pwnd->head.rpdesk == NULL)
        return FALSE;

    pdeskinfo = pwnd->head.rpdesk->pDeskInfo;

    /*
     * Add pwnd to the desktop's Volatile Window Pointer List (VWPL) of
     * ShellHook windows. If this call initializes the VWPL, set the
     * (re)allocation threshhold to 2 PWNDs (we know we never have more than
     * 2 windows in this list anyway)
     */
    if (VWPLAdd(&(pdeskinfo->pvwplShellHook), pwnd, 2)) {
        SetWF(pwnd, WFSHELLHOOKWND);
        return TRUE;
    }
    return FALSE;
}

/***************************************************************************\
* _DeregisterShellHookWindow
*
* History:
\***************************************************************************/

BOOL _DeregisterShellHookWindow(PWND pwnd) {
    PDESKTOPINFO pdeskinfo;

    if (pwnd->head.rpdesk == NULL)
        return FALSE;

    pdeskinfo = pwnd->head.rpdesk->pDeskInfo;

    if (VWPLRemove(&(pdeskinfo->pvwplShellHook), pwnd)) {
        ClrWF(pwnd, WFSHELLHOOKWND);
    }
    return TRUE;
}

/***************************************************************************\
* xxxSendMinRectMessages
*
* History:
\***************************************************************************/

BOOL xxxSendMinRectMessages(PWND pwnd, RECT *lpRect) {
    BOOL fRet = FALSE;
    HWND hwnd = HW(pwnd);
    PTHREADINFO pti = PtiCurrent();
    PDESKTOPINFO pdeskinfo;
    DWORD nPwndShellHook;
    PWND pwndShellHook;

    if (IsHooked(pti, WHF_SHELL)) {
        xxxCallHook(HSHELL_GETMINRECT, (WPARAM)hwnd,
            (LPARAM)lpRect, WH_SHELL);
        fRet = TRUE;
    }

    pdeskinfo = GETDESKINFO(pti);
    if (pdeskinfo->pvwplShellHook == NULL)
        return fRet;

    nPwndShellHook = 0;
    pwndShellHook = NULL;
    while (pwndShellHook = VWPLNext(pdeskinfo->pvwplShellHook, pwndShellHook, &nPwndShellHook)) {
        TL tlpwnd;
        ULONG_PTR dwRes;

        ThreadLock(pwndShellHook, &tlpwnd);
        if (xxxSendMessageTimeout(pwndShellHook, WM_KLUDGEMINRECT, (WPARAM)(hwnd), (LPARAM)lpRect,
            SMTO_NORMAL, 100, &dwRes))
            fRet = TRUE;

        /*
         * pdeskinfo->pvwplShellHook may have been realloced to a different
         * location and size during the WM_KLUDGEMINRECT callback.
         */
        ThreadUnlock(&tlpwnd);
    }
    return fRet;
}

/***************************************************************************\
* PostShellHookMessages
*
* History:
\***************************************************************************/

void PostShellHookMessages(UINT message, LPARAM lParam) {
    PDESKTOPINFO pdeskinfo = GETDESKINFO(PtiCurrent());
    DWORD nPwndShellHook;
    PWND pwndShellHook;

    nPwndShellHook = 0;
    pwndShellHook = NULL;

    /*
     * Hack for WM_APPCOMMAND (bug 389210):
     * We want to allow anyone who's listening for these wm_appcommand messages to be able to
     * take the foreground. ie pressing mail will launch outlook AND bring it to the foreground
     * We set the token to null so anyone can steal the foreground - else it isn't clear who should
     * have the right to steal it - only one person gets the right. We let them fight it out to
     * decide who gets foreground if more than one listener will try make a foreground change.
     */
    if (HSHELL_APPCOMMAND == message) {
        TAGMSG0(DBGTAG_FOREGROUND, "PostShellHookMessages cleared last input token - open foreground.");

        glinp.ptiLastWoken = NULL;
    }

    /*
     * Loop through all the windows registered to listen for shell hooks and post the message
     * to them
     */
    while (pwndShellHook = VWPLNext(pdeskinfo->pvwplShellHook, pwndShellHook, &nPwndShellHook)) {
        if (pwndShellHook == pdeskinfo->spwndProgman) {
            switch (message) {
            case HSHELL_WINDOWCREATED:
                _PostMessage(pwndShellHook, gpsi->uiShellMsg, guiOtherWindowCreated, lParam);
                break;
            case HSHELL_WINDOWDESTROYED:
                _PostMessage(pwndShellHook, gpsi->uiShellMsg, guiOtherWindowDestroyed, lParam);
                break;
            }
        } else {
            _PostMessage(pwndShellHook, gpsi->uiShellMsg, message, lParam);
        }
    }

}

/***************************************************************************\
* _ResetDblClk
*
* History:
\***************************************************************************/

VOID _ResetDblClk(VOID)
{
    PtiCurrent()->pq->timeDblClk = 0L;
}

/***************************************************************************\
* SetMsgBox
*
* History:
\***************************************************************************/

void SetMsgBox(PWND pwnd)
{
    pwnd->head.rpdesk->pDeskInfo->cntMBox++;
    SetWF(pwnd, WFMSGBOX);
    return;
}

/***************************************************************************\
* xxxSimulateShiftF10
*
* This routine is called to convert a WM_CONTEXTHELP message back to a
* SHIFT-F10 sequence for old applications.  It is called from the default
* window procedure.
*
* History:
* 22-Aug-95 BradG       Ported from Win95 (rare.asm)
\***************************************************************************/

VOID xxxSimulateShiftF10( VOID )
{
        /*
     *  VK_SHIFT down
     */
    xxxKeyEvent(VK_LSHIFT, 0x2A | SCANCODE_SIMULATED, NtGetTickCount(), 0, FALSE);

    /*
     *  VK_F10 down
     */
    xxxKeyEvent(VK_F10, 0x44 | SCANCODE_SIMULATED, NtGetTickCount(), 0, FALSE);

    /*
     *  VK_F10 up
     */
    xxxKeyEvent(VK_F10 | KBDBREAK, 0x44 | SCANCODE_SIMULATED, NtGetTickCount(), 0, FALSE);

    /*
     *  VK_SHIFT up
     */
    xxxKeyEvent(VK_LSHIFT | KBDBREAK, 0x2A | SCANCODE_SIMULATED, NtGetTickCount(), 0, FALSE);
}

/*
 * VWPL (Volatile Window Pointer List) Implementation details.
 * ===========================================================
 * Volatile Window Pointer Lists are used to keep a list of windows that we want
 * to send messages to, where the list may get altered during each of those send
 * message callbacks.
 *
 * The list is volatile in that it can change its size, contents and location
 * while we continue to traverse the list.
 *
 * Examples of use:
 * - hungapp redraw code in hungapp.c
 * - xxxSendMinRectMessages stuff in rare.c
 *
 * Members of the VWPL struct:
 * cPwnd
 *   The number of pwnds in the list, not including NULLs
 * cElem
 *   The size of the list, including NULLs.
 * cThreshhold
 *   When growing, the number of extra spaces to add to the list.
 *   When (cElem - cPwnd) > cThreshhold, that's when we reallocate to shrink.
 * apwnd[]
 *   An array of pwnds.
 *   The array may have some empty slots, but they will all be at the end.
 *
 * VWPL Internal Invariants:
 * - no pwnd appears more than once.
 * - cPwnd <= cElem
 * - number of unused slots (cElem - cPwnd) < cThreshhold
 * - all unused slots are at the end of the aPwnd[] array
 *
 * Restrictions on use of VWPLs:
 * - NULL pwnd is not allowed (except in unused slots)
 * - all pwnds in the list must be valid: pwnds must be explicitly removed from
 *   the list in their xxxFreeWindow.
 */

#if DBG_VWPL
BOOL DbgCheckVWPL(PVWPL pvwpl)
{
    DWORD ixPwnd;

    if (!pvwpl) {
        return TRUE;
    }

    UserAssert(pvwpl->cElem >= pvwpl->cPwnd);
    // Low memory may have made a shrinking realloc fail, which doesn't
    // really bother us.
    // UserAssert((pvwpl->cElem - pvwpl->cPwnd) < pvwpl->cThreshhold);

    // Check that cElem is not too big
    UserAssert(pvwpl->cElem < 1000);

    // check that the pwnds are all in the first cPwnd slots.
    for (ixPwnd = 0; ixPwnd < pvwpl->cPwnd; ixPwnd++) {
        UserAssert(pvwpl->aPwnd[ixPwnd] != NULL);
    }

#if ZERO_INIT_VWPL
    // check that the NULLs are all in the last few slots.
    for (ixPwnd = pvwpl->cPwnd; ixPwnd < pvwpl->cElem; ixPwnd++) {
        UserAssert(pvwpl->aPwnd[ixPwnd] == NULL);
    }
#endif

#if 0
    // check that all pwnds are valid?
    for (ixPwnd = 0; ixPwnd < pvwpl->cPwnd; ixPwnd++) {
         PWND pwnd = pvwpl->aPwnd[ixPwnd];
         UserAssert(ValidateHwnd(pwnd->head.h) == pwnd);
    }
#endif

    // check that no pwnds appears twice
    for (ixPwnd = 0; ixPwnd < pvwpl->cPwnd; ixPwnd++) {
        DWORD ix2;
        for (ix2 = ixPwnd + 1; ix2 < pvwpl->cPwnd; ix2++) {
            UserAssert(pvwpl->aPwnd[ixPwnd] != pvwpl->aPwnd[ix2]);
        }
    }
}
#else
#define DbgCheckVWPL(foo)
#endif

/*****************************************************************************\
* VWPLAdd
*
* Adds a pwnd to a VWPL (Volatile Window Pointer List).
* Allocates or reallocates memory as required.
*
* History:
* 98-01-30 IanJa   Created.
\*****************************************************************************/
BOOL VWPLAdd(
    PVWPL *ppvwpl,
    PWND pwnd,
    DWORD dwThreshhold)
{
    PVWPL pvwpl;
    DWORD ixPwnd;

    TAGMSG2(DBGTAG_VWPL, "VWPL %#p + %#p", *ppvwpl, pwnd);
    UserAssert(pwnd);

    if (*ppvwpl == NULL) {
        /*
         * Initialize the VWPL
         */
        UserAssert(dwThreshhold >= 2); // could be 1, but that would be silly
        pvwpl = (PVWPL)UserAllocPool(
                sizeof(VWPL) + (sizeof(PWND) * dwThreshhold), TAG_VWPL);
        if (pvwpl == NULL) {
            RIPMSG1(RIP_WARNING,
                    "VWPLAdd fail to allocate initial %lx",
                    sizeof(VWPL) + (sizeof(PWND) * dwThreshhold));
            DbgCheckVWPL(*ppvwpl);
            return FALSE;
        }
        pvwpl->cElem = dwThreshhold;
        pvwpl->cThreshhold = dwThreshhold;
#if ZERO_INIT_VWPL
        RtlZeroMemory(&(pvwpl->aPwnd[0]), (sizeof(PWND) * dwThreshhold));
#endif
        pvwpl->cPwnd = 0;
        *ppvwpl = pvwpl;
        ixPwnd = 0;
        goto AddPwnd;
    } else {
        pvwpl = *ppvwpl;
        for (ixPwnd = 0; ixPwnd < pvwpl->cElem; ixPwnd++) {
            if (pwnd == pvwpl->aPwnd[ixPwnd]) {
                DbgCheckVWPL(*ppvwpl);
                return FALSE; // callers require FALSE this case
            }
        }

        if (pvwpl->cPwnd >= pvwpl->cElem ) {
            /*
             *  didn't find it already there, and no space so grow the VWPL
             */
            DWORD dwSize;
            DWORD dwSizeNew;

            dwSize = sizeof(VWPL) + (sizeof(PWND) * pvwpl->cElem);
            dwSizeNew = dwSize + (sizeof(PWND) * pvwpl->cThreshhold);
            pvwpl = (PVWPL)UserReAllocPool(pvwpl, dwSize, dwSizeNew, TAG_VWPL);
            if (pvwpl == NULL) {
                RIPMSG2(RIP_WARNING,
                        "VWPLAdd fail to reallocate %lx to %lx", dwSize, dwSizeNew);
                DbgCheckVWPL(*ppvwpl);
                return FALSE;
            }
#if ZERO_INIT_VWPL
            RtlZeroMemory(&(pvwpl->aPwnd[pvwpl->cPwnd]), (sizeof(PWND) * dwThreshhold));
#endif
            pvwpl->cElem += pvwpl->cThreshhold;
            *ppvwpl = pvwpl;
        }
    }

AddPwnd:
    ixPwnd = pvwpl->cPwnd;
    pvwpl->aPwnd[ixPwnd] = pwnd;
    pvwpl->cPwnd++;
    DbgCheckVWPL(*ppvwpl);
    return TRUE;
}

/*****************************************************************************\
* VWPLRemove
*
* Removes a pwnd from a VWPL list of pwnds.
* Reallocates memory as required.
*
* Returns FALSE if the pwnd was not found
*
* History:
* 98-01-30 IanJa   Created.
\*****************************************************************************/
BOOL VWPLRemove(
    PVWPL *ppvwpl,
    PWND pwnd)
{
    PVWPL pvwpl = *ppvwpl;
    DWORD ixPwnd;

    TAGMSG2(DBGTAG_VWPL, "VWPL %#p - %#p", *ppvwpl, pwnd);
    UserAssert(pwnd);

    if (!pvwpl) {
        return FALSE;
    }
    for (ixPwnd = 0; ixPwnd < pvwpl->cElem; ixPwnd++) {
        if (pwnd == pvwpl->aPwnd[ixPwnd]) {
            goto PwndIsFound;
        }
    }
    DbgCheckVWPL(*ppvwpl);
    return FALSE;

PwndIsFound:
    pvwpl->aPwnd[ixPwnd] = NULL;
    pvwpl->cPwnd--;

    if (pvwpl->cPwnd == 0) {
        UserFreePool(pvwpl);
        *ppvwpl = NULL;
        return TRUE;
    }

    /*
     * Compact the VWPL to keep all the empty slots at the end.
     * If these free slots exceeds the threshhold, realloc to shrink.
     * It doesn't matter that we change the order.
     */
    pvwpl->aPwnd[ixPwnd] = pvwpl->aPwnd[pvwpl->cPwnd];
#if ZERO_INIT_VWPL
    pvwpl->aPwnd[pvwpl->cPwnd] = NULL;
#endif


    if ((pvwpl->cElem - pvwpl->cPwnd) >= pvwpl->cThreshhold) {
        DWORD dwSize;
        DWORD dwSizeNew;

        // Low memory may have made a shrinking realloc fail, which doesn't
        // really bother us.
        // UserAssert((pvwpl->cElem - pvwpl->cPwnd) == pvwpl->cThreshhold);

        dwSize = sizeof(VWPL) + (sizeof(PWND) * pvwpl->cElem);
        dwSizeNew = sizeof(VWPL) + (sizeof(PWND) * pvwpl->cPwnd);
        pvwpl = (PVWPL)UserReAllocPool(pvwpl, dwSize, dwSizeNew, TAG_VWPL);
        if (pvwpl == NULL) {
            RIPMSG2(RIP_WARNING,
                    "VWPLRemove fail to reallocate %lx to %lx",
                    dwSize, dwSizeNew);
            DbgCheckVWPL(*ppvwpl);
            return TRUE;
        }
        pvwpl->cElem = pvwpl->cPwnd;
        *ppvwpl = pvwpl;
    }

    DbgCheckVWPL(*ppvwpl);
    return TRUE;
}

/*****************************************************************************\
* VWPLNext
*
* Returns the next pwnd from a VWPL (Volatile Window Pointer List).
*
* Setting *pnPrev to 0 will return the first pwnd in the VWPL, and gets a new
* value in *pnPrev which is to be used in a subsequent call to VWPLNext to
* obtain the next pwnd.
* Returns NULL when the last pwnd has been obtained, and sets *pnPrev back to 0
*
* History:
* 98-01-30 IanJa   Created.
\*****************************************************************************/
PWND VWPLNext(PVWPL pvwpl, PWND pwndPrev, DWORD *pnPrev)
{
    DbgCheckVWPL(pvwpl);

    if (!pvwpl) {
        TAGMSG1(DBGTAG_VWPL, "VWPL %#p => NULL (empty)", pvwpl);
        return NULL;
    }

    if (*pnPrev >= pvwpl->cPwnd) {
        goto NoMorePwnds;
    }

    /*
     * If our previous pwnd is still there, advance to the next slot
     * (else it has gone, so return the one now occupying its slot)
     */
    if (pvwpl->aPwnd[*pnPrev] == pwndPrev) {
        (*pnPrev)++;
    }

    if (*pnPrev < pvwpl->cPwnd) {
        UserAssert(pvwpl->aPwnd[*pnPrev] != pwndPrev);
        TAGMSG2(DBGTAG_VWPL, "VWPL %#p => %#p", pvwpl, pvwpl->aPwnd[*pnPrev]);
        return pvwpl->aPwnd[*pnPrev];
    }

    /*
     * We came to the end
     */
NoMorePwnds:
    TAGMSG1(DBGTAG_VWPL, "VWPL %#p => NULL (end)", pvwpl);
    *pnPrev = 0;
    return NULL;
}
