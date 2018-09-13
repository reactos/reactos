/****************************** Module Header ******************************\
* Module Name: mmrtl.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Multimonitor APIs.
*
* History:
* 29-Mar-1997 adams     Created.
\***************************************************************************/

#define Int32x32To32(x, y) ((x) * (y))

/*
 * There is no object locking in the client, so the monitor object can
 * go away at any time. Therefore to be safe, we need an exception handler.
 */
#ifdef _USERK_
#define BEGIN_EXCEPTION_HANDLER
#define END_EXCEPTION_HANDLER
#define END_EXCEPTION_HANDLER_EMPTY
#else // _USERK_
#define BEGIN_EXCEPTION_HANDLER try {
#define END_EXCEPTION_HANDLER \
    } except (W32ExceptionHandler(TRUE, RIP_WARNING)) { \
        pMonitorResult = NULL; \
    }
#define END_EXCEPTION_HANDLER_EMPTY \
    } except (W32ExceptionHandler(TRUE, RIP_WARNING)) { \
    }
#endif // _USERK_


/***************************************************************************\
* _MonitorFromPoint
*
* Calculate the monitor that a point is in or is nearest to.
*
* Arguments:
*     pt      - The point.
*     dwFlags - One of:
*         MONITOR_DEFAULTTONULL - If the point isn't in a monitor,
*             return NULL.
*
*         MONITOR_DEFAULTTOPRIMARY - If the point isn't in a monitor,
*             return the primary monitor.
*
*         MONITOR_DEFAULTTONEAREST - Return the monitor nearest the point.
*
* History:
* 22-Sep-1996 adams     Created.
* 29-Mar-1997 adams     Moved to rtl.
\***************************************************************************/

PMONITOR
_MonitorFromPoint(POINT pt, DWORD dwFlags)
{
    PMONITOR        pMonitor, pMonitorResult;
    int             dx;
    int             dy;

    UserAssert(dwFlags == MONITOR_DEFAULTTONULL ||
               dwFlags == MONITOR_DEFAULTTOPRIMARY ||
               dwFlags == MONITOR_DEFAULTTONEAREST);

    if (GetDispInfo()->cMonitors == 1 && dwFlags != MONITOR_DEFAULTTONULL)
        return GetPrimaryMonitor();

    switch (dwFlags) {
    case MONITOR_DEFAULTTONULL:
    case MONITOR_DEFAULTTOPRIMARY:
        /*
         * Return the monitor the point is in.
         */

        BEGIN_EXCEPTION_HANDLER

        for (   pMonitor = REBASESHAREDPTRALWAYS(GetDispInfo()->pMonitorFirst);
                pMonitor;
                pMonitor = REBASESHAREDPTR(pMonitor->pMonitorNext)) {

            if (!(pMonitor->dwMONFlags & MONF_VISIBLE))
                continue;

            if (PtInRect(&((MONITOR *)pMonitor)->rcMonitor, pt)) {
                return pMonitor;
            }
        }

        END_EXCEPTION_HANDLER_EMPTY

        /*
         * Return what the user wants if it's not found.
         */
        switch (dwFlags) {
        case MONITOR_DEFAULTTONULL:
            return NULL;

        case MONITOR_DEFAULTTOPRIMARY:
            return GetPrimaryMonitor();

        default:
            UserAssertMsg0(FALSE, "Logic error in _MonitorFromPoint");
            break;
        }

    case MONITOR_DEFAULTTONEAREST:

#define MONITORFROMPOINTALGORITHM(SUMSQUARESMAX, SUMSQUARESTYPE, POINTMULTIPLY)     \
        SUMSQUARESTYPE  sumsquare;                                                  \
        SUMSQUARESTYPE  leastsumsquare;                                             \
        leastsumsquare = SUMSQUARESMAX;                                             \
        for (   pMonitor = REBASESHAREDPTRALWAYS(GetDispInfo()->pMonitorFirst);     \
                pMonitor;                                                           \
                pMonitor = REBASESHAREDPTR(pMonitor->pMonitorNext)) {               \
                                                                                    \
            if (!(pMonitor->dwMONFlags & MONF_VISIBLE))                             \
                continue;                                                           \
                                                                                    \
            /*                                                                      \
             * Determine distance from monitor along x axis.                        \
             */                                                                     \
            if (pt.x < pMonitor->rcMonitor.left) {                                  \
                dx = pMonitor->rcMonitor.left - pt.x;                               \
            } else if (pt.x < pMonitor->rcMonitor.right) {                          \
                dx = 0;                                                             \
            } else {                                                                \
                /*                                                                  \
                 * Monitor rectangles do not include the rightmost edge.            \
                 */                                                                 \
                dx = pt.x - (pMonitor->rcMonitor.right - 1);                        \
            }                                                                       \
                                                                                    \
            /*                                                                      \
             * Skip this monitor if dx is greater than dx^2 + dy^2.                 \
             * We do this check to avoid multiplication operations.                 \
             */                                                                     \
            if ((SUMSQUARESTYPE) dx >= leastsumsquare)                              \
                continue;                                                           \
                                                                                    \
            /*                                                                      \
             * Determine distance from monitor along y axis.                        \
             */                                                                     \
            if (pt.y < pMonitor->rcMonitor.top) {                                   \
                dy = pMonitor->rcMonitor.top - pt.y;                                \
            } else if (pt.y < pMonitor->rcMonitor.bottom) {                         \
                /*                                                                  \
                 * The point is in the monitor and we're done                       \
                 * if both dx and dy are zero.                                      \
                 */                                                                 \
                if (dx == 0)                                                        \
                    return pMonitor;                                                \
                                                                                    \
                dy = 0;                                                             \
            } else {                                                                \
                dy = pt.y - (pMonitor->rcMonitor.bottom - 1);                       \
            }                                                                       \
                                                                                    \
            /*                                                                      \
             * Calculate dx^2. Skip this monitor if dx is greater                   \
             * than dx^2 + dy^2. We do this check to avoid                          \
             * multiplication operations.                                           \
             */                                                                     \
            sumsquare = POINTMULTIPLY(dx, dx);                                      \
            if (sumsquare >= leastsumsquare)                                        \
                continue;                                                           \
                                                                                    \
            /*                                                                      \
             * Skip this monitor if dx^2 + y is greater than dx^2 + dy^2.           \
             * We do this check to avoid multiplication operations.                 \
             */                                                                     \
            if (sumsquare + (SUMSQUARESTYPE) dy >= leastsumsquare)                  \
                continue;                                                           \
                                                                                    \
            /*                                                                      \
             * Compute dx^2 + dy^2. Skip this monitor if it's not the least.        \
             */                                                                     \
            sumsquare += (SUMSQUARESTYPE) POINTMULTIPLY(dy, dy);                    \
            if (sumsquare >= leastsumsquare)                                        \
                continue;                                                           \
                                                                                    \
            /*                                                                      \
             * This is the closest monitor so far.                                  \
             */                                                                     \
            leastsumsquare = sumsquare;                                             \
            pMonitorResult = pMonitor;                                              \
        }

#if DBG
        pMonitorResult = (PMONITOR) -1;
#endif

        if (    pt.x < SHRT_MIN || SHRT_MAX < pt.x ||
                pt.y < SHRT_MIN || SHRT_MAX < pt.y) {

            BEGIN_EXCEPTION_HANDLER
            MONITORFROMPOINTALGORITHM(_UI64_MAX, ULONGLONG, Int32x32To64)
            END_EXCEPTION_HANDLER

        } else {

            BEGIN_EXCEPTION_HANDLER
            MONITORFROMPOINTALGORITHM(UINT_MAX, UINT, Int32x32To32)
            END_EXCEPTION_HANDLER

        }

        UserAssert(pMonitorResult != (PMONITOR) -1);
        return pMonitorResult;

    default:
        UserAssert(0 && "Logic error in _MonitorFromPoint, shouldn't have gotten here.");
        break;
    }

    UserAssert(0 && "Logic error in _MonitorFromPoint, shouldn't have gotten here.");
    return NULL;
}



/***************************************************************************\
* _MonitorFromRect
*
* Calculate the monitor that a rect is in or is nearest to.
*
* Arguments:
*     lprc    - The rect.
*     dwFlags - One of:
*         MONITOR_DEFAULTTONULL - If the rect doesn't intersect a monitor,
*             return NULL.
*
*         MONITOR_DEFAULTTOPRIMARY - If the rect doesn't intersect a monitor,
*             return the primary monitor.
*
*         MONITOR_DEFAULTTONEAREST - Return the monitor nearest the rect.
*
* History:
* 22-Sep-1996 adams     Created.
* 29-Mar-1997 adams     Moved to rtl.
\***************************************************************************/

PMONITOR
_MonitorFromRect(LPCRECT lprc, DWORD dwFlags)
{
    PDISPLAYINFO    pDispInfo;
    PMONITOR        pMonitor, pMonitorResult;
    RECT            rc;
    int             area, areaMost;

    UserAssert(dwFlags == MONITOR_DEFAULTTONULL ||
               dwFlags == MONITOR_DEFAULTTOPRIMARY ||
               dwFlags == MONITOR_DEFAULTTONEAREST);

    /*
     * Special case the most common case - 1 monitor.
     */
    pDispInfo = GetDispInfo();
    if (pDispInfo->cMonitors == 1 && dwFlags != MONITOR_DEFAULTTONULL)
        return GetPrimaryMonitor();

    /*
     * If rect is empty, use topleft point.
     */
    if (IsRectEmpty(lprc)) {
        return _MonitorFromPoint(*(LPPOINT)lprc, dwFlags);
    }

    /*
     * Return the primary monitor if the rectangle covers the desktop.
     */
    if (    lprc->left   <= pDispInfo->rcScreen.left &&
            lprc->top    <= pDispInfo->rcScreen.top &&
            lprc->right  >= pDispInfo->rcScreen.right &&
            lprc->bottom >= pDispInfo->rcScreen.bottom) {

        return GetPrimaryMonitor();
    }

    /*
     * Calculate the nearest rectangle by determining which
     * monitor has the greatest intersection with the rectangle.
     */

    BEGIN_EXCEPTION_HANDLER

    areaMost = 0;
    for (   pMonitor = REBASESHAREDPTRALWAYS(GetDispInfo()->pMonitorFirst);
            pMonitor;
            pMonitor = REBASESHAREDPTR(pMonitor->pMonitorNext)) {

        if (!(pMonitor->dwMONFlags & MONF_VISIBLE))
            continue;

        if (IntersectRect(&rc, lprc, &((MONITOR *)pMonitor)->rcMonitor)) {
            if (EqualRect(&rc, lprc))
                return pMonitor;

            /*
             * Calculate the area of the intersection. Note that
             * the intersection must be in 16bit coordinats, since
             * we limit monitor rects to 16bit coordinate space.
             * So the result of any area calculation will fit in
             * in an int.
             */
            area = (rc.right - rc.left) * (rc.bottom - rc.top);
            if (area > areaMost) {
                areaMost = area;
                pMonitorResult = pMonitor;
            }
        }
    }

    END_EXCEPTION_HANDLER

    UserAssert(areaMost >= 0);
    if (areaMost > 0)
        return pMonitorResult;


    switch (dwFlags) {
    case MONITOR_DEFAULTTONULL:
        return NULL;

    case MONITOR_DEFAULTTOPRIMARY:
        return GetPrimaryMonitor();

    case MONITOR_DEFAULTTONEAREST:
        {
            int dx, dy;

#define MONITORFROMRECTALGORITHM(SUMSQUARESMAX, SUMSQUARESTYPE, POINTMULTIPLY)      \
            SUMSQUARESTYPE  sumsquare;                                              \
            SUMSQUARESTYPE  leastsumsquare;                                         \
            leastsumsquare = SUMSQUARESMAX;                                         \
            for (   pMonitor = REBASESHAREDPTRALWAYS(GetDispInfo()->pMonitorFirst); \
                    pMonitor;                                                       \
                    pMonitor = REBASESHAREDPTR(pMonitor->pMonitorNext)) {           \
                                                                                    \
                if (!(pMonitor->dwMONFlags & MONF_VISIBLE))                         \
                    continue;                                                       \
                                                                                    \
                /*                                                                  \
                 * Determine distance from monitor along x axis.                    \
                 */                                                                 \
                if (lprc->right <= pMonitor->rcMonitor.left) {                      \
                    /*                                                              \
                     * Add 1 because rectangles do not include the rightmost edge.  \
                     */                                                             \
                    dx = pMonitor->rcMonitor.left - lprc->right + 1;                \
                } else if (lprc->left < pMonitor->rcMonitor.right) {                \
                    dx = 0;                                                         \
                } else {                                                            \
                    /*                                                              \
                     * Add 1 because rectangles do not include the rightmost edge.  \
                     */                                                             \
                    dx = lprc->left - (pMonitor->rcMonitor.right - 1);              \
                }                                                                   \
                                                                                    \
                /*                                                                  \
                 * Skip this monitor if dx is greater than dx^2 + dy^2.             \
                 * We do this check to avoid multiplication operations.             \
                 */                                                                 \
                if ((SUMSQUARESTYPE) dx >= leastsumsquare)                          \
                    continue;                                                       \
                                                                                    \
                /*                                                                  \
                 * Determine distance from monitor along y axis.                    \
                 */                                                                 \
                if (lprc->bottom <= pMonitor->rcMonitor.top) {                      \
                    /*                                                              \
                     * Add 1 because rectangles do not include the bottommost edge. \
                     */                                                             \
                    dy = pMonitor->rcMonitor.top - lprc->bottom + 1;                \
                } else if (lprc->top < pMonitor->rcMonitor.bottom) {                \
                    UserAssert(dx != 0 && "This rectangle intersects a monitor, so we shouldn't be here."); \
                    dy = 0;                                                         \
                } else {                                                            \
                    /*                                                              \
                     * Add 1 because rectangles do not include the bottommost edge. \
                     */                                                             \
                    dy = lprc->top - pMonitor->rcMonitor.bottom + 1;                \
                }                                                                   \
                                                                                    \
                /*                                                                  \
                 * Calculate dx^2. Skip this monitor if dx is greater               \
                 * than dx^2 + dy^2. We do this check to avoid                      \
                 * multiplication operations.                                       \
                 */                                                                 \
                sumsquare = POINTMULTIPLY(dx, dx);                                  \
                if (sumsquare >= leastsumsquare)                                    \
                    continue;                                                       \
                                                                                    \
                /*                                                                  \
                 * Skip this monitor if dx^2 + y is greater than dx^2 + dy^2.       \
                 * We do this check to avoid multiplication operations.             \
                 */                                                                 \
                if (sumsquare + (SUMSQUARESTYPE) dy >= leastsumsquare)              \
                    continue;                                                       \
                                                                                    \
                /*                                                                  \
                 * Compute dx^2 + dy^2. Skip this monitor if it's not the least.    \
                 */                                                                 \
                sumsquare += (SUMSQUARESTYPE) POINTMULTIPLY(dy, dy);                \
                if (sumsquare >= leastsumsquare)                                    \
                    continue;                                                       \
                                                                                    \
                /*                                                                  \
                 * This is the closest monitor so far.                              \
                 */                                                                 \
                leastsumsquare = sumsquare;                                         \
                pMonitorResult = pMonitor;                                          \
            }

#if DBG
            pMonitorResult = (PMONITOR) -1;
#endif

            if (    lprc->left < SHRT_MIN || SHRT_MAX < lprc->left ||
                    lprc->top < SHRT_MIN || SHRT_MAX < lprc->top ||
                    lprc->right < SHRT_MIN || SHRT_MAX < lprc->right ||
                    lprc->bottom < SHRT_MIN || SHRT_MAX < lprc->bottom) {

                BEGIN_EXCEPTION_HANDLER
                MONITORFROMRECTALGORITHM(_UI64_MAX, ULONGLONG, Int32x32To64)
                END_EXCEPTION_HANDLER

            } else {

                BEGIN_EXCEPTION_HANDLER
                MONITORFROMRECTALGORITHM(UINT_MAX, UINT, Int32x32To32)
                END_EXCEPTION_HANDLER

            }

            UserAssert(pMonitorResult != (PMONITOR) -1);
            return pMonitorResult;
        }

    default:
        UserAssertMsg0(0, "Logic error in _MonitorFromWindow, shouldn't have gotten here.");
        break;
    }

    UserAssertMsg0(0, "Logic error in _MonitorFromWindow, shouldn't have gotten here.");
    return NULL;
}



/***************************************************************************\
* _MonitorFromWindow
*
* Calculate the monitor that a window is in or is nearest to. We use
* the center of the window to determine its location. If the window
* is minimized, use its normal position.
*
* Arguments:
*     pwnd    - The window.
*     dwFlags - One of:
*         MONITOR_DEFAULTTONULL - If the window doesn't intersect a monitor,
*             return NULL.
*
*         MONITOR_DEFAULTTOPRIMARY - If the window doesn't intersect a monitor,
*             return the primary monitor.
*
*         MONITOR_DEFAULTTONEAREST - Return the monitor nearest the window.
*
* History:
* 22-Sep-1996 adams     Created.
* 29-Mar-1997 adams     Moved to rtl.
\***************************************************************************/

PMONITOR
_MonitorFromWindow(PWND pwnd, DWORD dwFlags)
{
    PWND            pwndParent;

    UserAssert(dwFlags == MONITOR_DEFAULTTONULL ||
               dwFlags == MONITOR_DEFAULTTOPRIMARY ||
               dwFlags == MONITOR_DEFAULTTONEAREST);

    if (GetDispInfo()->cMonitors == 1 && dwFlags != MONITOR_DEFAULTTONULL) {
        return GetPrimaryMonitor();
    }

    if (!pwnd)
        goto NoWindow;

    /*
     * Handle minimized windows.
     */
    if (TestWF(pwnd, WFMINIMIZED))
    {
#ifdef _USERK_
        CHECKPOINT *    pcp;

        pcp = (CHECKPOINT *)_GetProp(pwnd, PROP_CHECKPOINT, PROPF_INTERNAL);
        if (pcp) {
            return _MonitorFromRect(&pcp->rcNormal, dwFlags);
        }
#else
        WINDOWPLACEMENT wp;
        HWND            hwnd;

        wp.length = sizeof(wp);
        hwnd = (HWND)PtoH(pwnd);
        if (GetWindowPlacement(hwnd, &wp)) {
            return _MonitorFromRect(&wp.rcNormalPosition, dwFlags);
        }

        /*
         * (adams) If GetWindowPlacement fails, then either there was not enough
         * memory to allocate a CHECKPOINT, or the window was destroyed
         * and the API failed. If the later, the following code my be
         * playing with invalid memory. Although on the client side we
         * can never guarantee that a window is valid, it seems especially
         * likely that it is invalid here. So do another revalidation
         * by calling IsWindow.
         */
        if (!IsWindow(hwnd))
            goto NoWindow;
#endif

        UserAssert(GETFNID(pwnd) != FNID_DESKTOP);
        pwndParent = REBASEPWND(pwnd, spwndParent);
        if (GETFNID(pwndParent) == FNID_DESKTOP) {
            return GetPrimaryMonitor();
        }

        /*
         * Otherwise, if we are a child window, fall thru below to use the
         * window rect, which actually means something for non-toplevel dudes.
         */
    }

    return _MonitorFromRect(&((WND *)pwnd)->rcWindow, dwFlags);

NoWindow:
    if (dwFlags & (MONITOR_DEFAULTTOPRIMARY | MONITOR_DEFAULTTONEAREST)) {
        return GetPrimaryMonitor();
    }

    return NULL;
}
