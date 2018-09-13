/****************************** Module Header ******************************\
* Module Name: multimon.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Multimonitor APIs.
*
* History:
* 27-Sep-1996 adams     Stub implementation for NT 5.
* 20-Feb-1997 adams     Port from NT4 SP3.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/* last monitor the cursor was clipped to */
PMONITOR gpMonitorMouse;

/***************************************************************************\
* ClipPointToDesktop
*
* Clips the point the nearest monitor on the desktop.
*
* Arguments:
*     lppt - The point to clip.
*
* History:
* 22-Sep-1996 adams     Created.
* 04-Sep-1998 MCostea   Use _MonitorFromPoint()
\***************************************************************************/

void
ClipPointToDesktop(LPPOINT lppt)
{
    PMONITOR        pMonitor;

    UserAssert(!gpDispInfo->fDesktopIsRect &&
               "You shouldn't call this function if the desktop is a rectangle.\n"
               "Just clip to gpsi->rcScreen instead.");

    /*
     * Optimization: The cursor is likely to be on the monitor it was last on,
     * so check for that case.
     */
    if (gpMonitorMouse != NULL && PtInRect(&gpMonitorMouse->rcMonitor, *lppt)) {
        return;
    }

    pMonitor = _MonitorFromPoint(*lppt, MONITOR_DEFAULTTONEAREST);

    /*
     * Remember the monitor the cursor is on.
     */
    gpMonitorMouse = pMonitor;

    if (lppt->x < pMonitor->rcMonitor.left) {
        lppt->x = pMonitor->rcMonitor.left;
    } else if (lppt->x >= pMonitor->rcMonitor.right) {
        lppt->x = pMonitor->rcMonitor.right-1;
    }
    if (lppt->y < pMonitor->rcMonitor.top) {
        lppt->y = pMonitor->rcMonitor.top;
    } else if (lppt->y >= pMonitor->rcMonitor.bottom) {
        lppt->y = pMonitor->rcMonitor.bottom-1;
    }
}

/***************************************************************************\
* xxxEnumDisplayMonitors
*
* Enumerates the monitors in a display.
*
* Arguments:
*     hdcPaint  - An HDC with a particular visible region. The HDC
*         passed to lpfnEnum will have the capabilities of that monitor,
*         with its visible region clipped to the monitor and hdcPaint.
*         If hdcPaint is NULL, the hdcMonitor passed to lpfnEnum will be NULL.
*
*     lprcClip  - A rectangle to clip the area to. If hdcPaint is non-NULL,
*         the coordinates have the origin of hdcPaint. If hdcPaint is NULL,
*         the coordinates are virtual screen coordinates. If lprcClip is NULL,
*         no clipping is performed.
*
*     lpfnEnum  - The enumeration function.
*
*     dwData    - Application-defined data that is passed through to the
*         enumeration function.
*
*     fInternal - TRUE if the callback is in the kernel, FALSE otherwise.
*
* BUGBUG: This function is only a skeleton for the real thing. It needs
* to handle different displays and possibly different bit depths.
*
* History:
* 22-Sep-1996 adams     Created.
\***************************************************************************/

BOOL
xxxEnumDisplayMonitors(
    HDC             hdcPaint,
    LPRECT          lprcPaint,
    MONITORENUMPROC lpfnEnum,
    LPARAM          lData,
    BOOL            fInternal)
{
    RECT            rcPaint;
    POINT           ptOrg;
    RECT            rcMonitorPaint;
    BOOL            fReturn;
    PMONITOR        pMonitor;
    TL              tlpMonitor;
    PTHREADINFO     ptiCurrent = PtiCurrent();
    PDCE            pdcePaint;
    HDC             hdcMonitor;
    PWND            pwndOrg;

    /*
     * Validate the DC passed in.
     */
    if (hdcPaint) {

        if ((pdcePaint = LookupDC(hdcPaint)) == NULL) {
            RIPMSG0(RIP_WARNING, "EnumDisplayMonitors: LookupDC failed");
            return FALSE;
        }

        pwndOrg = pdcePaint->pwndOrg;

        /*
         * Intersect the painting area with the clipbox.  If there
         * isn't anything, bail out now.
         */
        if (GreGetClipBox(hdcPaint, &rcPaint, FALSE) == NULLREGION)
            return TRUE;

        if (lprcPaint && !IntersectRect(&rcPaint, &rcPaint, lprcPaint))
            return TRUE;

        /*
         * rcPaint is in dc coordinates.  We must convert to screen
         * coords so we can intersect with monitors.
         */
        GreGetDCOrg(hdcPaint, &ptOrg);
        OffsetRect(&rcPaint, ptOrg.x, ptOrg.y);
    } else {
        CopyRect(&rcPaint, &gpDispInfo->rcScreen);
        if (lprcPaint && !IntersectRect(&rcPaint, &rcPaint, lprcPaint))
            return TRUE;
    }

    fReturn = TRUE;

    for (pMonitor = gpDispInfo->pMonitorFirst; pMonitor != NULL;
                pMonitor = pMonitor->pMonitorNext) {

        /*
         * Note: the check for MONF_VISIBLE was removed to allow mirror drivers
         * to see monitor specific updates.
         */
        if (!IntersectRect(&rcMonitorPaint, &rcPaint, &pMonitor->rcMonitor)) {
            continue;
        }

        if (hdcPaint) {

            if ((hdcMonitor = GetMonitorDC(pdcePaint, pMonitor)) == NULL) {
                RIPMSG0(RIP_WARNING, "EnumDisplayMonitors: GetMonitorDC failed");
                return FALSE;
            }

            OffsetRect(&rcMonitorPaint, -ptOrg.x, -ptOrg.y);
            GreIntersectClipRect(
                    hdcMonitor,
                    rcMonitorPaint.left,
                    rcMonitorPaint.top,
                    rcMonitorPaint.right,
                    rcMonitorPaint.bottom);
        } else {

            hdcMonitor = NULL;
        }

        ThreadLockAlwaysWithPti(ptiCurrent, pMonitor, &tlpMonitor);

        if (fInternal) {
            fReturn = (*lpfnEnum) (
                    (HMONITOR) pMonitor,
                    hdcMonitor,
                    &rcMonitorPaint,
                    lData);

        } else {
            fReturn = xxxClientMonitorEnumProc(
                    PtoH(pMonitor),
                    hdcMonitor,
                    &rcMonitorPaint,
                    lData,
                    lpfnEnum);
        }

        /*
         * We just called back and the monitor has been freed if
         * ThreadUnlock returns NULL. The entire monitor configuration may
         * have changed, the monitors may have been rearranged, so just stop
         * enumerating at this point.
         */
        if (ThreadUnlock(&tlpMonitor) == NULL) {
            fReturn = FALSE;
        }

        if (hdcMonitor)
            ReleaseCacheDC(hdcMonitor, FALSE);

        if (!fReturn)
            break;

        /*
         * Revalidate hdcPaint, since it could have been messed with
         * in the callback.
         */
        if (hdcPaint) {
            if ((pdcePaint = LookupDC(hdcPaint)) == NULL) {
                RIPMSG0(RIP_WARNING, "EnumDisplayMonitors: LookupDC failed");
                return FALSE;
            }

            if (pdcePaint->pwndOrg != pwndOrg) {
                RIPMSG0(RIP_WARNING, "EnumDisplayMonitors: wrong window");
                return FALSE;
            }
        }
    }

    return fReturn;
}

/***************************************************************************\
* DestroyMonitor
*
* This function doesn't keep track of the visible monitors count because
* it assumes that after it was called the count will be recalculated, such
* as the case during mode changes. For a final unlock the monitor will have
* been removed from the monitor list already, so the count doesn't need to
* be recalculated.
*
* 5-May-1998    vadimg      created
\***************************************************************************/

void DestroyMonitor(PMONITOR pMonitor)
{
    UserAssert(pMonitor);

    /*
     * Remove references to this monitor from the global data.
     */
    if (pMonitor == gpMonitorMouse) {
        gpMonitorMouse = NULL;
    }

    /*
     * Remove from the monitor list.
     */
    REMOVE_FROM_LIST(MONITOR, gpDispInfo->pMonitorFirst, pMonitor, pMonitorNext);

    /*
     * Make sure the primary monitor points to a valid monitor. During the
     * mode changes the primary monitor will be recalculated as appropriate.
     */
    if (pMonitor == gpDispInfo->pMonitorPrimary) {
        gpDispInfo->pMonitorPrimary = gpDispInfo->pMonitorFirst;
    }

    if (HMMarkObjectDestroy(pMonitor)) {

        if (pMonitor->hrgnMonitor) {
           GreDeleteObject(pMonitor->hrgnMonitor);
        }
    
        HMFreeObject(pMonitor);
    }
}
