/*
 * PROJECT:    PAINT for ReactOS
 * LICENSE:    LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:    Window procedure of the size boxes
 * COPYRIGHT:  Copyright 2015 Benedikt Freisen <b.freisen@gmx.net>
 *             Copyright 2017-2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

BOOL setCursorOnSizeBox(HITTEST hit);
BOOL getSizeBoxRect(LPRECT prc, HITTEST hit, LPCRECT prcBase);
HITTEST getSizeBoxHitTest(POINT pt, LPCRECT prcBase);
VOID drawSizeBoxes(HDC hdc, LPCRECT prcBase, BOOL bDrawFrame = FALSE, LPCRECT prcPaint = NULL);
