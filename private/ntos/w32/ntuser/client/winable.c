/****************************** Module Header ******************************\
* Module Name: winable.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains
*
* History:
* 20-Feb-1992 DarrinM   Pulled functions from user\server.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
*
* GetWindowInfo()
* PRIVATE
*
* Gets information about a window in one self-consistent big block.
*
\***************************************************************************/
BOOL WINAPI
GetWindowInfo(HWND hwnd, PWINDOWINFO pwi)
{
    PWND pwnd;
    UINT cBorders;
    PCLS pclsT;

    if (pwi->cbSize != sizeof(WINDOWINFO)) {
        RIPERR1(ERROR_INVALID_PARAMETER, RIP_WARNING, "WINDOWINFO.cbSize %d is wrong", pwi->cbSize);
    }
    /*
     * Validate the window
     */
    pwnd = ValidateHwnd(hwnd);

    if (pwnd == NULL) {
        return FALSE;
    }

    try {
        // Window rect
        pwi->rcWindow = pwnd->rcWindow;

        // Client rect
        pwi->rcClient = pwnd->rcClient;

        // Style
        pwi->dwStyle = pwnd->style;
        pwi->dwExStyle = pwnd->ExStyle;
        pwi->dwWindowStatus = 0;
        if (TestWF(pwnd, WFFRAMEON))
            pwi->dwWindowStatus |= WS_ACTIVECAPTION;

        // Borders
        cBorders = GetWindowBorders(pwnd->style, pwnd->ExStyle, TRUE, FALSE);
        pwi->cxWindowBorders = cBorders * SYSMET(CXBORDER);
        pwi->cyWindowBorders = cBorders * SYSMET(CYBORDER);

        // Type
        pclsT = (PCLS)REBASEALWAYS(pwnd, pcls);
        pwi->atomWindowType = pclsT->atomClassName;

        // Version
        if (TestWF(pwnd, WFWIN50COMPAT)) {
            pwi->wCreatorVersion = VER50;
        } else if (TestWF(pwnd, WFWIN40COMPAT)) {
            pwi->wCreatorVersion = VER40;
        } else if (TestWF(pwnd, WFWIN31COMPAT)) {
            pwi->wCreatorVersion = VER31;
        } else {
            pwi->wCreatorVersion = VER30;
        }
    } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
        RIPERR1(ERROR_INVALID_WINDOW_HANDLE,
                RIP_WARNING,
                "Window %x no longer valid",
                hwnd);
        return FALSE;
    }

    return TRUE;
}
