/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint/sizebox.h
 * PURPOSE:     Window procedure of the size boxes
 * PROGRAMMERS: Benedikt Freisen
 *              Katayama Hirofumi MZ
 */

#pragma once

BOOL setCursorOnSizeBox(CANVAS_HITTEST hit);
BOOL getSizeBoxRect(LPRECT prc, CANVAS_HITTEST hit, LPCRECT prcBase);
CANVAS_HITTEST getSizeBoxHitTest(POINT pt, LPCRECT prcBase);
VOID drawSizeBoxes(HDC hdc, LPCRECT prcBase, BOOL bDrawFrame = FALSE, LPCRECT prcPaint = NULL);
