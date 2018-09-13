/****************************** Module Header ******************************\
* Module Name: sysmet.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* System metrics APIs and support routines.
*
* History:
* 24-Sep-1990 DarrinM   Generated stubs.
* 12-Feb-1991 JimA      Added access checks
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* _SwapMouseButton (API)
*
* History:
* 24-Sep-1990 DarrinM   Generated stubs.
* 25-Jan-1991 DavidPe   Did the real thing.
* 12-Feb-1991 JimA      Added access check
\***************************************************************************/

BOOL APIENTRY _SwapMouseButton(
    BOOL fSwapButtons)
{
    BOOL            fSwapOld;
    PPROCESSINFO    ppiCurrent = PpiCurrent();

    /*
     * Blow it off if the caller doesn't have the proper access rights
     */
    RETURN_IF_ACCESS_DENIED(ppiCurrent->amwinsta,
                            WINSTA_READATTRIBUTES | WINSTA_WRITEATTRIBUTES,
                            FALSE);

    if (!(ppiCurrent->W32PF_Flags & W32PF_IOWINSTA)) {
        RIPERR0(ERROR_REQUIRES_INTERACTIVE_WINDOWSTATION,
                RIP_WARNING,
                "SwapMouseButton invalid on a non-interactive WindowStation.");

        return FALSE;
    }

    fSwapOld = SYSMET(SWAPBUTTON);
    SYSMET(SWAPBUTTON) = fSwapButtons;

    /*
     * Give xxxButtonEvent a hint that a mouse button event may have to be
     * left/right swapped to correspond with our current async key state.
     * Toggle the global since an even number of SwapMouseButtons has no effect.
     */
    if (fSwapButtons != fSwapOld) {
        gbMouseButtonsRecentlySwapped = !gbMouseButtonsRecentlySwapped;
    }

    /*
     * Return previous state
     */
    return fSwapOld;
}

/***************************************************************************\
* _SetDoubleClickTime (API)
*
* History:
* 24-Sep-1990 DarrinM   Generated stubs.
* 25-Jan-1991 DavidPe   Did the real thing.
* 12-Feb-1991 JimA      Added access check
* 16-May-1991 MikeKe    Changed to return BOOL
\***************************************************************************/

BOOL APIENTRY _SetDoubleClickTime(
    UINT dtTime)
{
    PWINDOWSTATION pwinsta = PpiCurrent()->rpwinsta;

    /*
     * Blow it off if the caller doesn't have the proper access rights
     */
    if (!CheckWinstaWriteAttributesAccess()) {
        return FALSE;
    }

    if (!dtTime) {
        dtTime = 500;
    } else if (dtTime > 5000) {
        dtTime = 5000;
    }

    gdtDblClk         = dtTime;
    gpsi->dtLBSearch = dtTime * 4;            // dtLBSearch   =  4  * gdtDblClk
    gpsi->dtScroll   = gpsi->dtLBSearch / 5;  // dtScroll     = 4/5 * gdtDblClk
    /*
     * This value should be set through SPI_SETMENUSHOWDELAY
     *   gdtMNDropDown     = gpsi->dtScroll;        // gdtMNDropDown = 4/5 * gdtDblClk
     */

    /*
     * Recalculate delays for tooltip windows on all desktops.
     */
    if (pwinsta != NULL) {
        PDESKTOP pdesk;
        for (pdesk = pwinsta->rpdeskList; pdesk; pdesk = pdesk->rpdeskNext) {
            InitTooltipDelay((PTOOLTIPWND)pdesk->spwndTooltip);
        }
    }

    return TRUE;
}

/***************************************************************************\
* SetSysColor()
*
* Changes the value of a system color, and updates the brush.  Tries to
* recover in case of an error.
*
* History:
\***************************************************************************/
VOID SetSysColor(
    UINT  icol,
    DWORD rgb,
    UINT  uOptions
    )
{

    if ((uOptions & SSCF_SETMAGICCOLORS) && gpDispInfo->fAnyPalette) {
        union {
            DWORD rgb;
            PALETTEENTRY pe;
        } peMagic;

        peMagic.rgb = rgb;

        /*
         *  when any of the 3D colors are changing, call GDI to
         *  set the apropiate "magic" color
         *
         *  the four magic colors are reserved like so
         *
         *  8       - UI color (3D shadow)
         *  9       - UI color (3D face)
         *
         *  F6      - UI color (3D hilight)
         *  F7      - UI color (desktop)
         *
         *  NOTE (3D hilight) inverts to (3D shadow)
         *       (3D face)    inverts to sys gray
         *
         */

        switch (icol)
        {
        case COLOR_3DSHADOW:
            GreSetMagicColors(gpDispInfo->hdcScreen, peMagic.pe, 8);
            break;

        case COLOR_3DFACE:
            GreSetMagicColors(gpDispInfo->hdcScreen, peMagic.pe, 9);
            break;

        case COLOR_3DHILIGHT:
            GreSetMagicColors(gpDispInfo->hdcScreen, peMagic.pe, 246);
            break;

        case COLOR_DESKTOP:
            GreSetMagicColors(gpDispInfo->hdcScreen, peMagic.pe, 247);
            break;
        }
    }

    if (uOptions & SSCF_16COLORS) {
        /*
         * Force solid colors for all elements in 16 color or less modes.
         */
        rgb = GreGetNearestColor(gpDispInfo->hdcScreen, rgb);
    } else if (uOptions & SSCF_FORCESOLIDCOLOR) {
        /*
         * Force solid colors for certain window elements.
         */
        switch (icol) {

        /*
         * These can be dithers
         */
        case COLOR_DESKTOP:
        case COLOR_ACTIVEBORDER:
        case COLOR_INACTIVEBORDER:
        case COLOR_APPWORKSPACE:
        case COLOR_INFOBK:
        case COLOR_GRADIENTACTIVECAPTION:
        case COLOR_GRADIENTINACTIVECAPTION:
            break;

        default:
            rgb = GreGetNearestColor(gpDispInfo->hdcScreen, rgb);
            break;
        }
    }

    gpsi->argbSystem[icol] = rgb;
    if (SYSHBRUSH(icol) == NULL) {
        /*
         * This is the first time we're setting up the system colors.
         * We need to create the brush
         */
        SYSHBRUSH(icol) = GreCreateSolidBrush(rgb);
        GreMarkUndeletableBrush(SYSHBRUSH(icol));
        GreSetBrushOwnerPublic(SYSHBRUSH(icol));
    } else {
        GreSetSolidBrush(SYSHBRUSH(icol), rgb);
    }
}

/***************************************************************************\
* xxxSetSysColors (API)
*
*
* History:
* 12-Feb-1991 JimA      Created stub and added access check
* 22-Apr-1991 DarrinM   Ported from Win 3.1 sources.
* 16-May-1991 MikeKe    Changed to return BOOL
\***************************************************************************/
BOOL APIENTRY xxxSetSysColors(PUNICODE_STRING pProfileUserName,
    int      cicol,
    PUINT    picolor,
    COLORREF *prgb,
    UINT     uOptions
    )
{
    int      i;
    UINT     icol;
    COLORREF rgb;

    /*
     * Blow it off if the caller doesn't have the proper access rights
     */
    if ((uOptions & SSCF_NOTIFY) && !CheckWinstaWriteAttributesAccess()) {
        return FALSE;
    }

    if (GreGetDeviceCaps(gpDispInfo->hdcScreen, NUMCOLORS) <= 16) {
        uOptions |= SSCF_16COLORS;
    }

    if (uOptions & SSCF_SETMAGICCOLORS) {
        /*
         * Set the Magic colors first
         */
        for(i = 0; i < cicol; i++) {
            icol = picolor[i];
            rgb = prgb[i];
            if (    icol == COLOR_3DFACE ||
                    icol == COLOR_3DSHADOW ||
                    icol == COLOR_3DHILIGHT ||
                    icol == COLOR_DESKTOP) {

                SetSysColor(icol, rgb, uOptions);
            }
        }
    }

    for (i = 0; i < cicol; i++) {

        icol = *picolor++;
        rgb  = *prgb++;

        if (icol >= COLOR_MAX)
            continue;

        if ((uOptions & SSCF_SETMAGICCOLORS) &&
               (icol == COLOR_3DFACE ||
                icol == COLOR_3DSHADOW ||
                icol == COLOR_3DHIGHLIGHT ||
                icol == COLOR_DESKTOP)) {
            continue;
        }

        SetSysColor(icol, rgb, uOptions);
    }

    if (uOptions & SSCF_NOTIFY) {

        /*
         * Recolor all the current desktop
         */
        RecolorDeskPattern();

        /*
         * Render the system bitmaps in new colors before we broadcast
         */

        xxxSetWindowNCMetrics(pProfileUserName,NULL, FALSE, -1);


        /*
         * Notify everyone that the colors have changed.
         */
        xxxSendNotifyMessage(PWND_BROADCAST, WM_SYSCOLORCHANGE, 0, 0L);

        /*
         * Just redraw the entire screen.  Trying to just draw the parts
         * that were changed isn't worth it, since Control Panel always
         * resets every color anyway.
         *
         * Anyway, it could get messy, sending apps NCPAINT messages without
         * accumulating update regions too.
         */
        xxxRedrawScreen();
    }

    return TRUE;
}
