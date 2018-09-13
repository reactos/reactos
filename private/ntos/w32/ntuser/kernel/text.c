/****************************** Module Header ******************************\
* Module Name: text.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains the MessageBox API and related functions.
*
* History:
* 10-01-90 EricK        Created.
* 11-20-90 DarrinM      Merged in User text APIs.
* 02-07-91 DarrinM      Removed TextOut, ExtTextOut, and GetTextExtentPoint stubs.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

BOOL _TextOutW(
    HDC     hdc,
    int     x,
    int     y,
    LPCWSTR lp,
    UINT    cc);

/***************************************************************************\
* xxxPSMTextOut
*
* Outputs the text and puts and _ below the character with an &
* before it. Note that this routine isn't used for menus since menus
* have their own special one so that it is specialized and faster...
*
* NOTE: A very similar routine (UserLpkPSMTextOut) exists on the client
*       side in drawtext.c.  Any non-kernel specific changes to this
*       routine most likely need to be made in UserLpkPSMTextOut as well.
*
* History:
* 11-13-90 JimA         Ported to NT.
* 30-Nov-1992 mikeke    Client side version
* 8-Apr-1998 MCostea    Added dwFlags
\***************************************************************************/

void xxxPSMTextOut(
    HDC hdc,
    int xLeft,
    int yTop,
    LPWSTR lpsz,
    int cch,
    DWORD dwFlags)
{
    int cx;
    LONG textsize, result;
    /*
     * In the kernel we have a limited amount of stack. So it should be a stack
     * variable in user mode and static in kernel mode where it is thread safe
     * since we are in the crit section.
     */
    static WCHAR achWorkBuffer[255];
    WCHAR *pchOut = achWorkBuffer;
    TEXTMETRICW textMetric;
    SIZE size;
    RECT rc;
    COLORREF color;
    PTHREADINFO ptiCurrent = PtiCurrentShared();

    if (CALL_LPK(ptiCurrent)) {
        /*
         * A user mode LPK is installed for layout and shaping.
         * Perform callback and return.
         */
        UNICODE_STRING ustrStr;

        RtlInitUnicodeString(&ustrStr, lpsz);
        xxxClientPSMTextOut(hdc, xLeft, yTop, &ustrStr, cch, dwFlags);
        return;
    }

    if (cch > sizeof(achWorkBuffer)/sizeof(WCHAR)) {
        pchOut = (WCHAR*)UserAllocPool((cch+1) * sizeof(WCHAR), TAG_RTL);
        if (pchOut == NULL)
            return;
    }

    result = GetPrefixCount(lpsz, cch, pchOut, cch);

    if (!(dwFlags & DT_PREFIXONLY)) {
        _TextOutW(hdc, xLeft, yTop, pchOut, cch - HIWORD(result));
    }

    /*
     * Any true prefix characters to underline?
     */
    if (LOWORD(result) == 0xFFFF || dwFlags & DT_HIDEPREFIX) {
        if (pchOut != achWorkBuffer)
            UserFreePool(pchOut);
        return;
    }

    if (!_GetTextMetricsW(hdc, &textMetric)) {
        textMetric.tmOverhang = 0;
        textMetric.tmAscent = 0;
    }

    /*
     * For proportional fonts, find starting point of underline.
     */
    if (LOWORD(result) != 0) {

        /*
         * How far in does underline start (if not at 0th byte.).
         */
        GreGetTextExtentW(hdc, (LPWSTR)pchOut, LOWORD(result), &size, GGTE_WIN3_EXTENT);
        xLeft += size.cx;

        /*
         * Adjust starting point of underline if not at first char and there is
         * an overhang.  (Italics or bold fonts.)
         */
        xLeft = xLeft - textMetric.tmOverhang;
    }

    /*
     * Adjust for proportional font when setting the length of the underline and
     * height of text.
     */
    GreGetTextExtentW(hdc, (LPWSTR)(pchOut + LOWORD(result)), 1, &size, GGTE_WIN3_EXTENT);
    textsize = size.cx;

    /*
     * Find the width of the underline character.  Just subtract out the overhang
     * divided by two so that we look better with italic fonts.  This is not
     * going to effect embolded fonts since their overhang is 1.
     */
    cx = LOWORD(textsize) - textMetric.tmOverhang / 2;

    /*
     * Get height of text so that underline is at bottom.
     */
    yTop += textMetric.tmAscent + 1;

    /*
     * Draw the underline using the foreground color.
     */
    SetRect(&rc, xLeft, yTop, xLeft+cx, yTop+1);
    color = GreSetBkColor(hdc, GreGetTextColor(hdc));
    GreExtTextOutW(hdc, xLeft, yTop, ETO_OPAQUE, &rc, TEXT(""), 0, NULL);
    GreSetBkColor(hdc, color);

    if (pchOut != achWorkBuffer) {
        UserFreePool(pchOut);
    }
}

