/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint/sizebox.cpp
 * PURPOSE:     Window procedure of the size boxes
 * PROGRAMMERS: Benedikt Freisen
 *              Katayama Hirofumi MZ
 */

/* INCLUDES *********************************************************/

#include "precomp.h"

/* FUNCTIONS ********************************************************/

BOOL getSizeBoxRect(LPRECT prc, SIZEBOX_HITTEST sht, LPCRECT prcBase, BOOL bSetCursor)
{
    switch (sht)
    {
        case SIZEBOX_UPPER_LEFT:
            prc->left = prcBase->left;
            prc->top = prcBase->top;
            if (bSetCursor)
                ::SetCursor(::LoadCursor(NULL, IDC_SIZENWSE));
            break;
        case SIZEBOX_UPPER_CENTER:
            prc->left = (prcBase->left + prcBase->right - GRIP_SIZE) / 2;
            prc->top = prcBase->top;
            if (bSetCursor)
                ::SetCursor(::LoadCursor(NULL, IDC_SIZENS));
            break;
        case SIZEBOX_UPPER_RIGHT:
            prc->left = prcBase->right - GRIP_SIZE;
            prc->top = prcBase->top;
            if (bSetCursor)
                ::SetCursor(::LoadCursor(NULL, IDC_SIZENESW));
            break;
        case SIZEBOX_MIDDLE_LEFT:
            prc->left = prcBase->left;
            prc->top = (prcBase->top + prcBase->bottom - GRIP_SIZE) / 2;
            if (bSetCursor)
                ::SetCursor(::LoadCursor(NULL, IDC_SIZEWE));
            break;
        case SIZEBOX_MIDDLE_RIGHT:
            prc->left = prcBase->right - GRIP_SIZE;
            prc->top = (prcBase->top + prcBase->bottom - GRIP_SIZE) / 2;
            if (bSetCursor)
                ::SetCursor(::LoadCursor(NULL, IDC_SIZEWE));
            break;
        case SIZEBOX_LOWER_LEFT:
            prc->left = prcBase->left;
            prc->top = prcBase->bottom - GRIP_SIZE;
            if (bSetCursor)
                ::SetCursor(::LoadCursor(NULL, IDC_SIZENESW));
            break;
        case SIZEBOX_LOWER_CENTER:
            prc->left = (prcBase->left + prcBase->right - GRIP_SIZE) / 2;
            prc->top = prcBase->bottom - GRIP_SIZE;
            if (bSetCursor)
                ::SetCursor(::LoadCursor(NULL, IDC_SIZENS));
            break;
        case SIZEBOX_LOWER_RIGHT:
            prc->left = prcBase->right - GRIP_SIZE;
            prc->top = prcBase->bottom - GRIP_SIZE;
            if (bSetCursor)
                ::SetCursor(::LoadCursor(NULL, IDC_SIZENWSE));
            break;
        default:
            ::SetRectEmpty(prc);
            return FALSE;
    }

    prc->right = prc->left + GRIP_SIZE;
    prc->bottom = prc->top + GRIP_SIZE;
    return TRUE;
}

SIZEBOX_HITTEST getSizeBoxHitTest(POINT pt, LPCRECT prcBase, BOOL bSetCursor)
{
    RECT rc;
    for (INT i = SIZEBOX_UPPER_LEFT; i <= SIZEBOX_MAX; ++i)
    {
        SIZEBOX_HITTEST sht = (SIZEBOX_HITTEST)i;
        getSizeBoxRect(&rc, sht, prcBase, bSetCursor);
        if (::PtInRect(&rc, pt))
            return sht;
    }

    if (::PtInRect(prcBase, pt))
        return SIZEBOX_CONTENTS;

    return SIZEBOX_NONE;
}

VOID drawSizeBoxes(HDC hdc, LPCRECT prcBase, BOOL bDrawFrame, LPCRECT prcPaint)
{
    CRect rc, rcIntersect;

    if (prcPaint && !::IntersectRect(&rcIntersect, prcPaint, prcBase))
        return;

    if (bDrawFrame)
    {
        HGDIOBJ oldPen, oldBrush;
        LOGBRUSH logBrush = { BS_HOLLOW, 0, 0 };
        rc = *prcBase;
        ::InflateRect(&rc, -GRIP_SIZE / 2, -GRIP_SIZE / 2);

        oldPen = ::SelectObject(hdc, ::CreatePen(PS_DOT, 1, ::GetSysColor(COLOR_HIGHLIGHT)));
        oldBrush = ::SelectObject(hdc, ::CreateBrushIndirect(&logBrush));
        ::Rectangle(hdc, rc.left, rc.top, rc.Width(), rc.Height());
        ::DeleteObject(::SelectObject(hdc, oldBrush));
        ::DeleteObject(::SelectObject(hdc, oldPen));
    }

    for (INT i = SIZEBOX_UPPER_LEFT; i <= SIZEBOX_MAX; ++i)
    {
        SIZEBOX_HITTEST sht = (SIZEBOX_HITTEST)i;
        getSizeBoxRect(&rc, sht, prcBase, FALSE);
        if (::IntersectRect(&rcIntersect, &rc, prcPaint))
            ::FillRect(hdc, &rc, (HBRUSH)(COLOR_HIGHLIGHT + 1));
    }
}
