/*
 * PROJECT:    PAINT for ReactOS
 * LICENSE:    LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:    Window procedure of the size boxes
 * COPYRIGHT:  Copyright 2009 Benedikt Freisen <b.freisen@gmx.net>
 *             Copyright 2017-2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

BOOL setCursorOnSizeBox(CANVAS_HITTEST hit);
BOOL getSizeBoxRect(LPRECT prc, CANVAS_HITTEST hit, LPCRECT prcBase);
CANVAS_HITTEST getSizeBoxHitTest(POINT pt, LPCRECT prcBase);
VOID drawSizeBoxes(HDC hdc, LPCRECT prcBase, BOOL bDrawFrame = FALSE, LPCRECT prcPaint = NULL);
