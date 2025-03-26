/*
 * PROJECT:    PAINT for ReactOS
 * LICENSE:    LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:    Window procedure of the palette window
 * COPYRIGHT:  Copyright 2015 Benedikt Freisen <b.freisen@gmx.net>
 *             Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

/* The private metrics */
#define CXY_SELECTEDBOX     15 /* width / height of a selected color box */
#define X_MARGIN 4 /* horizontal margin */
#define Y_MARGIN ((rcClient.bottom / 2) - CXY_COLORBOX) /* center position minus one color box */
#define X_COLORBOX_OFFSET   (X_MARGIN + CXY_BIGBOX + X_MARGIN)
#define COLOR_COUNT         28
#define HALF_COLOR_COUNT    (COLOR_COUNT / 2)

CPaletteWindow paletteWindow;

/* FUNCTIONS ********************************************************/

CPaletteWindow::CPaletteWindow()
    : m_hbmCached(NULL)
{
}

CPaletteWindow::~CPaletteWindow()
{
    if (m_hbmCached)
        ::DeleteObject(m_hbmCached);
}

static VOID drawColorBox(HDC hDC, LPCRECT prc, COLORREF rgbColor, UINT nBorder)
{
    RECT rc = *prc;
    ::FillRect(hDC, &rc, (HBRUSH)(COLOR_3DFACE + 1));
    ::DrawEdge(hDC, &rc, nBorder, BF_RECT | BF_ADJUST);

    HBRUSH hbr = ::CreateSolidBrush(rgbColor);
    ::FillRect(hDC, &rc, hbr);
    ::DeleteObject(hbr);
}

static VOID getColorBoxRect(LPRECT prc, const RECT& rcClient, INT iColor)
{
    INT dx = (iColor % HALF_COLOR_COUNT) * CXY_COLORBOX; /* delta x */
    INT dy = (iColor / HALF_COLOR_COUNT) * CXY_COLORBOX; /* delta y */
    prc->left   = X_COLORBOX_OFFSET + dx;
    prc->right  = prc->left + CXY_COLORBOX;
    prc->top    = Y_MARGIN + dy;
    prc->bottom = prc->top + CXY_COLORBOX;
}

INT CPaletteWindow::DoHitTest(INT xPos, INT yPos) const
{
    RECT rcClient;
    GetClientRect(&rcClient);

    /* delta x and y */
    INT dx = (xPos - X_COLORBOX_OFFSET), dy = (yPos - Y_MARGIN);

    /* horizontal and vertical indexes */
    INT ix = (dx / CXY_COLORBOX), iy = (dy / CXY_COLORBOX);

    /* Is it inside of a color box? */
    if (0 <= ix && ix < HALF_COLOR_COUNT && 0 <= iy && iy < 2)
        return ix + (iy * HALF_COLOR_COUNT); /* return the color index */

    return -1; /* Not found */
}

LRESULT CPaletteWindow::OnEraseBkgnd(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    return TRUE; /* Avoid flickering */
}

LRESULT CPaletteWindow::OnPaint(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    RECT rc, rcClient;
    GetClientRect(&rcClient);

    PAINTSTRUCT ps;
    HDC hDC = BeginPaint(&ps);

    /* To avoid flickering, we use a memory bitmap.
       The left and top values are zeros in client rectangle */
    HDC hMemDC = ::CreateCompatibleDC(hDC);
    m_hbmCached = CachedBufferDIB(m_hbmCached, rcClient.right, rcClient.bottom);
    HGDIOBJ hbmOld = ::SelectObject(hMemDC, m_hbmCached);

    /* Fill the background (since WM_ERASEBKGND handling is disabled) */
    ::FillRect(hMemDC, &rcClient, (HBRUSH)(COLOR_3DFACE + 1));

    /* Draw the big box that contains the black box and the white box */
    rc = { X_MARGIN, Y_MARGIN, X_MARGIN + CXY_BIGBOX, Y_MARGIN + CXY_BIGBOX };
    ::DrawEdge(hMemDC, &rc, EDGE_SUNKEN, BF_RECT | BF_ADJUST);
    COLORREF rgbLight = ::GetSysColor(COLOR_3DHIGHLIGHT);
    for (INT y = rc.top; y < rc.bottom; ++y)
    {
        BOOL bLight = (y & 1);
        for (INT x = rc.left; x < rc.right; ++x)
        {
            if (bLight)
                ::SetPixelV(hMemDC, x, y, rgbLight);
            bLight = !bLight;
        }
    }

    /* Draw the white box in the big box, at 5/8 position */
    rc.left = X_MARGIN + (CXY_BIGBOX * 5 / 8) - (CXY_SELECTEDBOX / 2);
    rc.top = Y_MARGIN + (CXY_BIGBOX * 5 / 8) - (CXY_SELECTEDBOX / 2);
    rc.right = rc.left + CXY_SELECTEDBOX;
    rc.bottom = rc.top + CXY_SELECTEDBOX;
    drawColorBox(hMemDC, &rc, paletteModel.GetBgColor(), BDR_RAISEDINNER);

    /* Draw the black box (overlapping the white box), at 3/8 position */
    rc.left = X_MARGIN + (CXY_BIGBOX * 3 / 8) - (CXY_SELECTEDBOX / 2);
    rc.top = Y_MARGIN + (CXY_BIGBOX * 3 / 8) - (CXY_SELECTEDBOX / 2);
    rc.right = rc.left + CXY_SELECTEDBOX;
    rc.bottom = rc.top + CXY_SELECTEDBOX;
    drawColorBox(hMemDC, &rc, paletteModel.GetFgColor(), BDR_RAISEDINNER);

    /* Draw the normal color boxes */
    for (INT i = 0; i < COLOR_COUNT; i++)
    {
        getColorBoxRect(&rc, rcClient, i);
        drawColorBox(hMemDC, &rc, paletteModel.GetColor(i), BDR_SUNKENOUTER);
    }

    /* Transfer bits (hDC <-- hMemDC) */
    ::BitBlt(hDC, 0, 0, rcClient.right, rcClient.bottom, hMemDC, 0, 0, SRCCOPY);

    ::SelectObject(hMemDC, hbmOld);
    ::DeleteDC(hMemDC);
    EndPaint(&ps);
    return 0;
}

LRESULT CPaletteWindow::OnLButtonDown(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    INT iColor = DoHitTest(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
    if (iColor != -1)
        paletteModel.SetFgColor(paletteModel.GetColor(iColor));
    SetCapture();
    return 0;
}

LRESULT CPaletteWindow::OnRButtonDown(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    INT iColor = DoHitTest(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
    if (iColor != -1)
        paletteModel.SetBgColor(paletteModel.GetColor(iColor));
    return 0;
}

LRESULT CPaletteWindow::OnLButtonDblClk(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    INT iColor = DoHitTest(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
    COLORREF rgbColor = paletteModel.GetFgColor();
    if (iColor != -1 && mainWindow.ChooseColor(&rgbColor))
    {
        paletteModel.SetColor(iColor, rgbColor);
        paletteModel.SetFgColor(rgbColor);
    }
    return 0;
}

LRESULT CPaletteWindow::OnRButtonDblClk(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    INT iColor = DoHitTest(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
    COLORREF rgbColor = paletteModel.GetBgColor();
    if (iColor != -1 && mainWindow.ChooseColor(&rgbColor))
    {
        paletteModel.SetColor(iColor, rgbColor);
        paletteModel.SetBgColor(rgbColor);
    }
    return 0;
}

LRESULT CPaletteWindow::OnPaletteModelColorChanged(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    Invalidate(FALSE);
    return 0;
}

LRESULT CPaletteWindow::OnMouseMove(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (::GetCapture() != m_hWnd)
        return 0;

    POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
    ClientToScreen(&pt);

    RECT rc;
    mainWindow.GetWindowRect(&rc);

    POINT ptCenter = { (rc.left + rc.right) / 2, (rc.top + rc.bottom) / 2 };

    DWORD dwExpectedBar1ID = ((pt.y < ptCenter.y) ? BAR1ID_TOP : BAR1ID_BOTTOM);

    if (registrySettings.Bar1ID != dwExpectedBar1ID)
    {
        registrySettings.Bar1ID = dwExpectedBar1ID;
        mainWindow.PostMessage(WM_SIZE, 0, 0);
    }

    return 0;
}

LRESULT CPaletteWindow::OnLButtonUp(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (::GetCapture() != m_hWnd)
        return 0;

    ::ReleaseCapture();
    return 0;
}
