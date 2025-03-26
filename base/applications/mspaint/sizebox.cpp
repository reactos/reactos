/*
 * PROJECT:    PAINT for ReactOS
 * LICENSE:    LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:    Window procedure of the size boxes
 * COPYRIGHT:  Copyright 2015 Benedikt Freisen <b.freisen@gmx.net>
 *             Copyright 2017 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

static LPCWSTR s_cursor_shapes[] =
{
    IDC_SIZENWSE, IDC_SIZENS, IDC_SIZENESW,
    IDC_SIZEWE,               IDC_SIZEWE,
    IDC_SIZENESW, IDC_SIZENS, IDC_SIZENWSE,
};

/* FUNCTIONS ********************************************************/

BOOL setCursorOnSizeBox(HITTEST hit)
{
    if (HIT_UPPER_LEFT <= hit && hit <= HIT_LOWER_RIGHT)
    {
        ::SetCursor(::LoadCursorW(NULL, s_cursor_shapes[hit - HIT_UPPER_LEFT]));
        return TRUE;
    }
    return FALSE;
}

BOOL getSizeBoxRect(LPRECT prc, HITTEST hit, LPCRECT prcBase)
{
    switch (hit)
    {
        case HIT_UPPER_LEFT:
            prc->left = prcBase->left;
            prc->top = prcBase->top;
            break;
        case HIT_UPPER_CENTER:
            prc->left = (prcBase->left + prcBase->right - GRIP_SIZE) / 2;
            prc->top = prcBase->top;
            break;
        case HIT_UPPER_RIGHT:
            prc->left = prcBase->right - GRIP_SIZE;
            prc->top = prcBase->top;
            break;
        case HIT_MIDDLE_LEFT:
            prc->left = prcBase->left;
            prc->top = (prcBase->top + prcBase->bottom - GRIP_SIZE) / 2;
            break;
        case HIT_MIDDLE_RIGHT:
            prc->left = prcBase->right - GRIP_SIZE;
            prc->top = (prcBase->top + prcBase->bottom - GRIP_SIZE) / 2;
            break;
        case HIT_LOWER_LEFT:
            prc->left = prcBase->left;
            prc->top = prcBase->bottom - GRIP_SIZE;
            break;
        case HIT_LOWER_CENTER:
            prc->left = (prcBase->left + prcBase->right - GRIP_SIZE) / 2;
            prc->top = prcBase->bottom - GRIP_SIZE;
            break;
        case HIT_LOWER_RIGHT:
            prc->left = prcBase->right - GRIP_SIZE;
            prc->top = prcBase->bottom - GRIP_SIZE;
            break;
        case HIT_INNER:
            *prc = *prcBase;
            ::InflateRect(prc, -GRIP_SIZE, -GRIP_SIZE);
            return TRUE;
        default:
            ::SetRectEmpty(prc);
            return FALSE;
    }

    prc->right = prc->left + GRIP_SIZE;
    prc->bottom = prc->top + GRIP_SIZE;
    return TRUE;
}

HITTEST getSizeBoxHitTest(POINT pt, LPCRECT prcBase)
{
    CRect rc;

    if (!::PtInRect(prcBase, pt))
        return HIT_NONE;

    rc = *prcBase;
    rc.InflateRect(-GRIP_SIZE, -GRIP_SIZE);
    if (rc.PtInRect(pt))
        return HIT_INNER;

    for (INT i = HIT_UPPER_LEFT; i <= HIT_LOWER_RIGHT; ++i)
    {
        HITTEST hit = (HITTEST)i;
        getSizeBoxRect(&rc, hit, prcBase);
        if (rc.PtInRect(pt))
            return hit;
    }

    return HIT_BORDER;
}

VOID drawSizeBoxes(HDC hdc, LPCRECT prcBase, BOOL bDrawFrame, LPCRECT prcPaint)
{
    CRect rc, rcIntersect;

    if (prcPaint && !::IntersectRect(&rcIntersect, prcPaint, prcBase))
        return;

    if (bDrawFrame)
    {
        rc = *prcBase;
        rc.InflateRect(-GRIP_SIZE / 2, -GRIP_SIZE / 2);

        LOGBRUSH logBrush = { BS_HOLLOW, 0, 0 };
        COLORREF rgbHighlight = ::GetSysColor(COLOR_HIGHLIGHT);
        HGDIOBJ oldPen = ::SelectObject(hdc, ::CreatePen(PS_DOT, 1, rgbHighlight));
        HGDIOBJ oldBrush = ::SelectObject(hdc, ::CreateBrushIndirect(&logBrush));
        ::Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
        ::DeleteObject(::SelectObject(hdc, oldBrush));
        ::DeleteObject(::SelectObject(hdc, oldPen));
    }

    for (INT i = HIT_UPPER_LEFT; i <= HIT_LOWER_RIGHT; ++i)
    {
        getSizeBoxRect(&rc, (HITTEST)i, prcBase);
        if (!prcPaint || ::IntersectRect(&rcIntersect, &rc, prcPaint))
            ::FillRect(hdc, &rc, (HBRUSH)(COLOR_HIGHLIGHT + 1));
    }
}
