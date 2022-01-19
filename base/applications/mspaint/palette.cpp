/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint/palette.cpp
 * PURPOSE:     Window procedure of the palette window
 * PROGRAMMERS: Benedikt Freisen
 */

/* INCLUDES *********************************************************/

#include "precomp.h"

/* FUNCTIONS ********************************************************/

LRESULT CPaletteWindow::OnPaint(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    RECT rc = { 0, 0, 31, 32 };
    HPEN oldPen;
    HBRUSH oldBrush;
    int i, a, b;

    DefWindowProc(WM_PAINT, wParam, lParam);

    HDC hDC = GetDC();
    for(b = 2; b < 30; b++)
        for(a = 2; a < 29; a++)
            if ((a + b) % 2 == 1)
                SetPixel(hDC, a, b, GetSysColor(COLOR_BTNHILIGHT));

    DrawEdge(hDC, &rc, EDGE_RAISED, BF_TOPLEFT);
    DrawEdge(hDC, &rc, BDR_SUNKENOUTER, BF_TOPLEFT | BF_BOTTOMRIGHT);
    SetRect(&rc, 11, 12, 26, 27);
    DrawEdge(hDC, &rc, BDR_RAISEDINNER, BF_RECT | BF_MIDDLE);
    oldPen = (HPEN) SelectObject(hDC, CreatePen(PS_NULL, 0, 0));
    oldBrush = (HBRUSH) SelectObject(hDC, CreateSolidBrush(paletteModel.GetBgColor()));
    Rectangle(hDC, rc.left, rc.top + 2, rc.right - 1, rc.bottom - 1);
    DeleteObject(SelectObject(hDC, oldBrush));
    SetRect(&rc, 4, 5, 19, 20);
    DrawEdge(hDC, &rc, BDR_RAISEDINNER, BF_RECT | BF_MIDDLE);
    oldBrush = (HBRUSH) SelectObject(hDC, CreateSolidBrush(paletteModel.GetFgColor()));
    Rectangle(hDC, rc.left + 2, rc.top + 2, rc.right - 1, rc.bottom - 1);
    DeleteObject(SelectObject(hDC, oldBrush));
    DeleteObject(SelectObject(hDC, oldPen));

    for(i = 0; i < 28; i++)
    {
        SetRect(&rc, 31 + (i % 14) * 16,
                0 + (i / 14) * 16, 16 + 31 + (i % 14) * 16, 16 + 0 + (i / 14) * 16);
        DrawEdge(hDC, &rc, EDGE_RAISED, BF_TOPLEFT);
        DrawEdge(hDC, &rc, BDR_SUNKENOUTER, BF_RECT);
        oldPen = (HPEN) SelectObject(hDC, CreatePen(PS_NULL, 0, 0));
        oldBrush = (HBRUSH) SelectObject(hDC, CreateSolidBrush(paletteModel.GetColor(i)));
        Rectangle(hDC, rc.left + 2, rc.top + 2, rc.right - 1, rc.bottom - 1);
        DeleteObject(SelectObject(hDC, oldBrush));
        DeleteObject(SelectObject(hDC, oldPen));
    }
    ReleaseDC(hDC);
    return 0;
}

LRESULT CPaletteWindow::OnLButtonDown(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (GET_X_LPARAM(lParam) >= 31)
        paletteModel.SetFgColor(paletteModel.GetColor((GET_X_LPARAM(lParam) - 31) / 16 + (GET_Y_LPARAM(lParam) / 16) * 14));
    return 0;
}

LRESULT CPaletteWindow::OnRButtonDown(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (GET_X_LPARAM(lParam) >= 31)
        paletteModel.SetBgColor(paletteModel.GetColor((GET_X_LPARAM(lParam) - 31) / 16 + (GET_Y_LPARAM(lParam) / 16) * 14));
    return 0;
}

LRESULT CPaletteWindow::OnLButtonDblClk(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (GET_X_LPARAM(lParam) >= 31)
        if (ChooseColor(&choosecolor))
        {
            paletteModel.SetColor((GET_X_LPARAM(lParam) - 31) / 16 + (GET_Y_LPARAM(lParam) / 16) * 14,
                choosecolor.rgbResult);
            paletteModel.SetFgColor(choosecolor.rgbResult);
        }
    return 0;
}

LRESULT CPaletteWindow::OnRButtonDblClk(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (GET_X_LPARAM(lParam) >= 31)
        if (ChooseColor(&choosecolor))
        {
            paletteModel.SetColor((GET_X_LPARAM(lParam) - 31) / 16 + (GET_Y_LPARAM(lParam) / 16) * 14,
                choosecolor.rgbResult);
            paletteModel.SetBgColor(choosecolor.rgbResult);
        }
    return 0;
}

LRESULT CPaletteWindow::OnPaletteModelColorChanged(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    InvalidateRect(NULL, FALSE);
    return 0;
}

LRESULT CPaletteWindow::OnPaletteModelPaletteChanged(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    InvalidateRect(NULL, FALSE);
    return 0;
}
