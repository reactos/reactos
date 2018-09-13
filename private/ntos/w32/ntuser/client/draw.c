
/****************************** Module Header ******************************\
* Module Name: draw.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains the DrawFrameControl API
*
* History:
* 12-12-93  FritzS  Ported from Chicago
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* PaintRect
*
* History:
* 11-15-90 DarrinM  Ported from Win 3.0 sources.
* 01-21-91 IanJa    Prefix '_' denoting exported function (although not API)
* 12-12-94 JerrySh  Copied from server - make sure to keep in sync
\***************************************************************************/

BOOL PaintRect(
    HWND hwndBrush,
    HWND hwndPaint,
    HDC hdc,
    HBRUSH hbr,
    LPRECT lprc)
{
    POINT ptOrg;
    PWND pwndBrush;
    PWND pwndPaint;
    HWND    hwndDesktop;

    hwndDesktop = GetDesktopWindow();
    if (hwndBrush == NULL) {
        hwndBrush = hwndDesktop;
    }

    if (hwndBrush != hwndPaint) {
        pwndBrush = ValidateHwnd(hwndBrush);
        if (pwndBrush == NULL) {
            RIPMSG1(RIP_WARNING, "PaintRect: invalid Brush window %lX", hwndBrush);
            return FALSE;
        }

        pwndPaint = ValidateHwnd(hwndPaint);
        if (pwndPaint == NULL) {
            RIPMSG1(RIP_WARNING, "PaintRect: invalid Paint window %lX", hwndBrush);
            return FALSE;
        }


        if (hwndBrush != hwndDesktop) {
            SetBrushOrgEx(
                    hdc,
                    pwndBrush->rcClient.left - pwndPaint->rcClient.left,
                    pwndBrush->rcClient.top - pwndPaint->rcClient.top,
                    &ptOrg);
        } else {
            SetBrushOrgEx(hdc, 0, 0, &ptOrg);
        }
    }

    /*
     * If hbr < CTLCOLOR_MAX, it isn't really a brush but is one of our
     * special color values.  Translate it to the appropriate WM_CTLCOLOR
     * message and send it off to get back a real brush.  The translation
     * process assumes the CTLCOLOR*** and WM_CTLCOLOR*** values map directly.
     */
    if (hbr < (HBRUSH)CTLCOLOR_MAX) {
        hbr = GetControlColor(hwndBrush, hwndPaint, hdc,
                HandleToUlong(hbr) + WM_CTLCOLORMSGBOX);
    }

    FillRect(hdc, lprc, hbr);

    if (hwndBrush != hwndPaint) {
        SetBrushOrgEx(hdc, ptOrg.x, ptOrg.y, NULL);
    }

    return TRUE;
}
