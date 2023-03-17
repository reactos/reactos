/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint/sizebox.h
 * PURPOSE:     Window procedure of the size boxes
 * PROGRAMMERS: Benedikt Freisen
 *              Katayama Hirofumi MZ
 */

#pragma once

enum SIZEBOX_HITTEST
{
    SIZEBOX_NONE = 0,
    SIZEBOX_UPPER_LEFT,
    SIZEBOX_UPPER_CENTER,
    SIZEBOX_UPPER_RIGHT,
    SIZEBOX_MIDDLE_LEFT,
    SIZEBOX_MIDDLE_RIGHT,
    SIZEBOX_LOWER_LEFT,
    SIZEBOX_LOWER_CENTER,
    SIZEBOX_LOWER_RIGHT,
    SIZEBOX_MAX = SIZEBOX_LOWER_RIGHT,
    SIZEBOX_CONTENTS
};

BOOL getSizeBoxRect(LPRECT prc, SIZEBOX_HITTEST sht, LPCRECT prcBase, BOOL bSetCursor);
SIZEBOX_HITTEST getSizeBoxHitTest(POINT pt, LPCRECT prcBase, BOOL bSetCursor);
VOID drawSizeBoxes(HDC hdc, LPCRECT prcBase, BOOL bDrawFrame = FALSE, LPCRECT prcPaint = NULL);
