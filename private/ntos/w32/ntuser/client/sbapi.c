/**************************** Module Header ********************************\
* Module Name:
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Scroll bar public APIs
*
* History:
*   08-16-95 FritzS
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* SetScrollPos
*
* History:
\***************************************************************************/

int SetScrollPos(
    HWND hwnd,
    int code,
    int pos,
    BOOL fRedraw)
{
    SCROLLINFO si;

    si.fMask = SIF_POS | SIF_RETURNOLDPOS;
    si.nPos = pos;
    si.cbSize = sizeof(SCROLLINFO);

    return((int) NtUserSetScrollInfo(hwnd, code, &si, fRedraw));
}


/***************************************************************************\
* SetScrollRange
*
* History:
* 16-May-1991 mikeke    Changed to return BOOL
\***************************************************************************/

BOOL SetScrollRange(
    HWND hwnd,
    int code,
    int posMin,
    int posMax,
    BOOL fRedraw)
{
    SCROLLINFO si;

    /*
     * Validate the window handle first, because the further call
     * to NtUserSetScrollInfo will return the position of the scrollbar
     * and not FALSE if the hwnd is invalid
     */
    if ( ValidateHwnd((hwnd)) == NULL)
        return FALSE;

    /*
     * Check if the 'Range'(Max - Min) can be represented by an integer;
     * If not, it is an error;
     * Fix for Bug #1089 -- SANKAR -- 20th Sep, 1989 --.
     */
    if ((unsigned int)(posMax - posMin) > MAXLONG) {
        RIPERR0(ERROR_INVALID_SCROLLBAR_RANGE, RIP_VERBOSE, "");
        return FALSE;
    }

    si.fMask  = SIF_RANGE;
    si.nMin   = posMin;
    si.nMax   = posMax;
    si.cbSize = sizeof(SCROLLINFO);

    NtUserSetScrollInfo(hwnd, code, &si, fRedraw);

    return TRUE;
}
