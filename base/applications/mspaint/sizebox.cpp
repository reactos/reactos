/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint/sizebox.cpp
 * PURPOSE:     Window procedure of the size boxes
 * PROGRAMMERS: Benedikt Freisen
 *              Katayama Hirofumi MZ
 */

#include "precomp.h"

static LPCWSTR s_cursor_shapes[] =
{
    IDC_SIZENWSE, IDC_SIZENS, IDC_SIZENESW,
    IDC_SIZEWE,               IDC_SIZEWE,
    IDC_SIZENESW, IDC_SIZENS, IDC_SIZENWSE,
};

/* FUNCTIONS ********************************************************/

BOOL setCursorOnSizeBox(CANVAS_HITTEST hit)
{
    if (HIT_UPPER_LEFT <= hit && hit <= HIT_LOWER_RIGHT)
    {
        ::SetCursor(::LoadCursor(NULL, s_cursor_shapes[hit - HIT_UPPER_LEFT]));
        return TRUE;
    }
    return FALSE;
}

BOOL getSizeBoxRect(LPRECT prc, CANVAS_HITTEST hit, LPCRECT prcBase)
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

CANVAS_HITTEST getSizeBoxHitTest(POINT pt, LPCRECT prcBase)
{
    RECT rc;

    if (!::PtInRect(prcBase, pt))
        return HIT_NONE;

    rc = *prcBase;
    ::InflateRect(&rc, -GRIP_SIZE, -GRIP_SIZE);
    if (::PtInRect(&rc, pt))
        return HIT_INNER;

    for (INT i = HIT_UPPER_LEFT; i <= HIT_LOWER_RIGHT; ++i)
    {
        CANVAS_HITTEST hit = (CANVAS_HITTEST)i;
        getSizeBoxRect(&rc, hit, prcBase);
        if (::PtInRect(&rc, pt))
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
        ::InflateRect(&rc, -GRIP_SIZE / 2, -GRIP_SIZE / 2);

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
        getSizeBoxRect(&rc, (CANVAS_HITTEST)i, prcBase);
        if (!prcPaint || ::IntersectRect(&rcIntersect, &rc, prcPaint))
            ::FillRect(hdc, &rc, (HBRUSH)(COLOR_HIGHLIGHT + 1));
    }
}
