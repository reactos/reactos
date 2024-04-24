/*
 * PROJECT:    PAINT for ReactOS
 * LICENSE:    LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:    Text editor and font chooser for the text tool
 * COPYRIGHT:  Copyright 2015 Benedikt Freisen <b.freisen@gmx.net>
 */

#include "precomp.h"

#define CXY_GRIP 3

CTextEditWindow textEditWindow;

/* FUNCTIONS ********************************************************/

CTextEditWindow::CTextEditWindow()
    : m_hFont(NULL)
    , m_hFontZoomed(NULL)
{
    SetRectEmpty(&m_rc);
}

INT CTextEditWindow::DoHitTest(RECT& rc, POINT pt)
{
    switch (getSizeBoxHitTest(pt, &rc))
    {
        case HIT_NONE:          return HTNOWHERE;
        case HIT_UPPER_LEFT:    return HTTOPLEFT;
        case HIT_UPPER_CENTER:  return HTTOP;
        case HIT_UPPER_RIGHT:   return HTTOPRIGHT;
        case HIT_MIDDLE_LEFT:   return HTLEFT;
        case HIT_MIDDLE_RIGHT:  return HTRIGHT;
        case HIT_LOWER_LEFT:    return HTBOTTOMLEFT;
        case HIT_LOWER_CENTER:  return HTBOTTOM;
        case HIT_LOWER_RIGHT:   return HTBOTTOMRIGHT;
        case HIT_BORDER:        return HTCAPTION; // Enable drag move
        case HIT_INNER:         return HTCLIENT;
    }
    return HTNOWHERE;
}

void CTextEditWindow::DrawGrip(HDC hDC, RECT& rc)
{
    drawSizeBoxes(hDC, &rc, TRUE, NULL);
}

void CTextEditWindow::FixEditPos(LPCWSTR pszOldText)
{
    CStringW szText;
    GetWindowText(szText);

    RECT rcParent;
    ::GetWindowRect(m_hwndParent, &rcParent);

    CRect rc, rcWnd, rcText;
    GetWindowRect(&rcWnd);
    rcText = rcWnd;

    HDC hDC = GetDC();
    if (hDC)
    {
        SelectObject(hDC, m_hFontZoomed);
        TEXTMETRIC tm;
        GetTextMetrics(hDC, &tm);
        szText += L"x"; // This is a trick to enable the g_ptEnd newlines
        const UINT uFormat = DT_LEFT | DT_TOP | DT_EDITCONTROL | DT_NOPREFIX | DT_NOCLIP |
                             DT_EXPANDTABS | DT_WORDBREAK;
        DrawTextW(hDC, szText, -1, &rcText, uFormat | DT_CALCRECT);
        if (tm.tmDescent > 0)
            rcText.bottom += tm.tmDescent;
        ReleaseDC(hDC);
    }

    UnionRect(&rc, &rcText, &rcWnd);
    ::MapWindowPoints(NULL, m_hwndParent, (LPPOINT)&rc, 2);

    rcWnd = rc;
    ::GetClientRect(m_hwndParent, &rcParent);
    rc.IntersectRect(&rcParent, &rcWnd);

    MoveWindow(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, FALSE);

    DefWindowProc(WM_HSCROLL, SB_LEFT, 0);
    DefWindowProc(WM_VSCROLL, SB_TOP, 0);

    ::InvalidateRect(m_hwndParent, &rc, TRUE);
}

LRESULT CTextEditWindow::OnChar(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (wParam == VK_TAB)
        return 0; // FIXME: Tabs

    CStringW szText;
    GetWindowText(szText);

    LRESULT ret = DefWindowProc(nMsg, wParam, lParam);
    FixEditPos(szText);

    return ret;
}

LRESULT CTextEditWindow::OnKeyDown(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (wParam == VK_ESCAPE)
    {
        toolsModel.OnEndDraw(TRUE);
        return 0;
    }

    CStringW szText;
    GetWindowText(szText);

    LRESULT ret = DefWindowProc(nMsg, wParam, lParam);
    FixEditPos(szText);
    return ret;
}

LRESULT CTextEditWindow::OnLButtonDown(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LRESULT ret = DefWindowProc(nMsg, wParam, lParam);
    DefWindowProc(WM_HSCROLL, SB_LEFT, 0);
    DefWindowProc(WM_VSCROLL, SB_TOP, 0);
    return ret;
}

LRESULT CTextEditWindow::OnEraseBkGnd(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HDC hDC = (HDC)wParam;
    if (!toolsModel.IsBackgroundTransparent())
    {
        RECT rc;
        GetClientRect(&rc);
        HBRUSH hbr = CreateSolidBrush(paletteModel.GetBgColor());
        FillRect(hDC, &rc, hbr);
        DeleteObject(hbr);
    }
    ::SetTextColor(hDC, paletteModel.GetFgColor());
    return TRUE;
}

LRESULT CTextEditWindow::OnPaint(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    RECT rc;
    GetClientRect(&rc);

    DefWindowProc(nMsg, wParam, lParam);

    HDC hDC = GetDC();
    if (hDC)
    {
        DrawGrip(hDC, rc);
        ReleaseDC(hDC);
    }

    return 0;
}

LRESULT CTextEditWindow::OnNCPaint(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    CRect rc;
    GetWindowRect(&rc);

    HDC hDC = GetDCEx(NULL, DCX_WINDOW | DCX_PARENTCLIP);
    if (hDC)
    {
        rc.OffsetRect(-rc.left, -rc.top);
        DrawGrip(hDC, rc);
        ReleaseDC(hDC);
    }

    return 0;
}

LRESULT CTextEditWindow::OnNCCalcSize(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    return 0; // No frame.
}

LRESULT CTextEditWindow::OnNCHitTest(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
    RECT rc;
    GetWindowRect(&rc);
    return DoHitTest(rc, pt);
}

LRESULT CTextEditWindow::OnSetCursor(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (CWaitCursor::IsWaiting())
    {
        bHandled = FALSE;
        return 0;
    }

    UINT nHitTest = LOWORD(lParam);
    if (nHitTest == HTCAPTION)
    {
        ::SetCursor(::LoadCursorW(NULL, (LPCWSTR)IDC_SIZEALL)); // Enable drag move
        return FALSE;
    }
    return DefWindowProc(nMsg, wParam, lParam);
}

LRESULT CTextEditWindow::OnMove(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LRESULT ret = DefWindowProc(nMsg, wParam, lParam);
    return ret;
}

LRESULT CTextEditWindow::OnSize(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LRESULT ret = DefWindowProc(nMsg, wParam, lParam);

    RECT rc;
    GetClientRect(&rc);
    SendMessage(EM_SETRECTNP, 0, (LPARAM)&rc);
    SendMessage(EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(0, 0));

    return ret;
}

// Hack: Use DECLARE_WND_SUPERCLASS instead!
HWND CTextEditWindow::Create(HWND hwndParent)
{
    m_hwndParent = hwndParent;

    const DWORD style = ES_LEFT | ES_MULTILINE | ES_WANTRETURN | ES_AUTOVSCROLL |
                        WS_CHILD | WS_THICKFRAME;
    HWND hwnd = ::CreateWindowEx(0, WC_EDIT, NULL, style, 0, 0, 0, 0,
                                 hwndParent, NULL, g_hinstExe, NULL);
    if (hwnd)
    {
#undef SubclassWindow // Don't use this macro
        SubclassWindow(hwnd);

        UpdateFont();

        PostMessage(WM_SIZE, 0, 0);
    }

    return m_hWnd;
}

void CTextEditWindow::DoFillBack(HWND hwnd, HDC hDC)
{
    if (toolsModel.IsBackgroundTransparent())
        return;

    RECT rc;
    SendMessage(EM_GETRECT, 0, (LPARAM)&rc);
    MapWindowPoints(hwnd, (LPPOINT)&rc, 2);

    HBRUSH hbr = CreateSolidBrush(paletteModel.GetBgColor());
    FillRect(hDC, &rc, hbr);
    DeleteObject(hbr);
}

LRESULT CTextEditWindow::OnCreate(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    UpdateFont();
    return 0;
}

LRESULT CTextEditWindow::OnClose(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    ShowWindow(SW_HIDE);
    if (m_hFont)
    {
        DeleteObject(m_hFont);
        m_hFont = NULL;
    }
    if (m_hFontZoomed)
    {
        DeleteObject(m_hFontZoomed);
        m_hFontZoomed = NULL;
    }
    return 0;
}

void CTextEditWindow::InvalidateEditRect()
{
    RECT rc;
    GetWindowRect(&rc);
    ::MapWindowPoints(NULL, m_hwndParent, (LPPOINT)&rc, 2);
    ::InvalidateRect(m_hwndParent, &rc, TRUE);

    GetClientRect(&rc);
    MapWindowPoints(canvasWindow, (LPPOINT)&rc, 2);
    canvasWindow.CanvasToImage(rc);
    m_rc = rc;
}

LRESULT CTextEditWindow::OnPaletteModelColorChanged(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    UpdateFont();
    return 0;
}

LRESULT CTextEditWindow::OnToolsModelSettingsChanged(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    UpdateFont();
    return 0;
}

LRESULT CTextEditWindow::OnToolsModelZoomChanged(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    UpdateFont();
    ValidateEditRect(NULL);
    return 0;
}

LRESULT CTextEditWindow::OnToolsModelToolChanged(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (wParam == TOOL_TEXT)
    {
        UpdateFont();
    }
    else
    {
        ShowWindow(SW_HIDE);
    }
    return 0;
}

void CTextEditWindow::UpdateFont()
{
    if (m_hFont)
    {
        DeleteObject(m_hFont);
        m_hFont = NULL;
    }
    if (m_hFontZoomed)
    {
        DeleteObject(m_hFontZoomed);
        m_hFontZoomed = NULL;
    }

    LOGFONTW lf;
    ZeroMemory(&lf, sizeof(lf));
    lf.lfCharSet = DEFAULT_CHARSET; // registrySettings.CharSet; // Ignore
    lf.lfWeight = (registrySettings.Bold ? FW_BOLD : FW_NORMAL);
    lf.lfItalic = (BYTE)registrySettings.Italic;
    lf.lfUnderline = (BYTE)registrySettings.Underline;
    StringCchCopyW(lf.lfFaceName, _countof(lf.lfFaceName), registrySettings.strFontName);

    HDC hdc = GetDC();
    if (hdc)
    {
        INT nFontSize = registrySettings.PointSize;
        lf.lfHeight = -MulDiv(nFontSize, GetDeviceCaps(hdc, LOGPIXELSY), 72);
        ReleaseDC(hdc);
    }

    m_hFont = ::CreateFontIndirect(&lf);

    lf.lfHeight = Zoomed(lf.lfHeight);
    m_hFontZoomed = ::CreateFontIndirect(&lf);

    SetWindowFont(m_hWnd, m_hFontZoomed, TRUE);
    DefWindowProc(EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(0, 0));

    FixEditPos(NULL);

    Invalidate();
}

LRESULT CTextEditWindow::OnSetSel(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LRESULT ret = DefWindowProc(nMsg, wParam, lParam);
    DefWindowProc(WM_HSCROLL, SB_LEFT, 0);
    DefWindowProc(WM_VSCROLL, SB_TOP, 0);
    InvalidateEditRect();
    return ret;
}

BOOL CTextEditWindow::GetEditRect(LPRECT prc) const
{
    *prc = m_rc;
    return TRUE;
}

void CTextEditWindow::ValidateEditRect(LPCRECT prc OPTIONAL)
{
    if (prc)
        m_rc = *prc;

    CRect rc = m_rc;
    canvasWindow.ImageToCanvas(rc);

    MoveWindow(rc.left, rc.top, rc.Width(), rc.Height(), TRUE);
}

LRESULT CTextEditWindow::OnMoving(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    // Restrict the window position to the image area
    LPRECT prcMoving = (LPRECT)lParam;
    CRect rcMoving = *prcMoving;

    CRect rcImage;
    canvasWindow.GetImageRect(rcImage);
    canvasWindow.ImageToCanvas(rcImage);
    canvasWindow.MapWindowPoints(NULL, &rcImage);

    CRect rcWnd;
    GetWindowRect(&rcWnd);
    INT cx = rcWnd.Width(), cy = rcWnd.Height();

    if (rcMoving.left < rcImage.left)
    {
        rcMoving.left = rcImage.left;
        rcMoving.right = rcImage.left + cx;
    }
    else if (rcMoving.right > rcImage.right)
    {
        rcMoving.right = rcImage.right;
        rcMoving.left = rcImage.right - cx;
    }

    if (rcMoving.top < rcImage.top)
    {
        rcMoving.top = rcImage.top;
        rcMoving.bottom = rcImage.top + cy;
    }
    else if (rcMoving.bottom > rcImage.bottom)
    {
        rcMoving.bottom = rcImage.bottom;
        rcMoving.top = rcImage.bottom - cy;
    }

    *prcMoving = rcMoving;
    Invalidate(TRUE);
    return TRUE;
}

LRESULT CTextEditWindow::OnSizing(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    // Restrict the window size to the image area
    LPRECT prcSizing = (LPRECT)lParam;
    CRect rcSizing = *prcSizing;

    CRect rcImage;
    canvasWindow.GetImageRect(rcImage);
    canvasWindow.ImageToCanvas(rcImage);
    canvasWindow.MapWindowPoints(NULL, &rcImage);

    // Horizontally
    switch (wParam)
    {
        case WMSZ_BOTTOMLEFT:
        case WMSZ_LEFT:
        case WMSZ_TOPLEFT:
            if (rcSizing.left < rcImage.left)
                rcSizing.left = rcImage.left;
            break;
        case WMSZ_BOTTOMRIGHT:
        case WMSZ_RIGHT:
        case WMSZ_TOPRIGHT:
            if (rcSizing.right > rcImage.right)
                rcSizing.right = rcImage.right;
            break;
        case WMSZ_TOP:
        case WMSZ_BOTTOM:
        default:
            break;
    }

    // Vertically
    switch (wParam)
    {
        case WMSZ_BOTTOM:
        case WMSZ_BOTTOMLEFT:
        case WMSZ_BOTTOMRIGHT:
            if (rcSizing.bottom > rcImage.bottom)
                rcSizing.bottom = rcImage.bottom;
            break;
        case WMSZ_TOP:
        case WMSZ_TOPLEFT:
        case WMSZ_TOPRIGHT:
            if (rcSizing.top < rcImage.top)
                rcSizing.top = rcImage.top;
            break;
        case WMSZ_LEFT:
        case WMSZ_RIGHT:
        default:
            break;
    }

    *prcSizing = rcSizing;
    Invalidate(TRUE);
    return TRUE;
}

LRESULT CTextEditWindow::OnMouseWheel(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    return ::SendMessageW(GetParent(), nMsg, wParam, lParam);
}

LRESULT CTextEditWindow::OnCut(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LRESULT ret = DefWindowProc(nMsg, wParam, lParam);
    Invalidate(TRUE); // Redraw
    return ret;
}

LRESULT CTextEditWindow::OnPaste(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LRESULT ret = DefWindowProc(nMsg, wParam, lParam);
    FixEditPos(NULL);
    return ret;
}

LRESULT CTextEditWindow::OnClear(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LRESULT ret = DefWindowProc(nMsg, wParam, lParam);
    Invalidate(TRUE); // Redraw
    return ret;
}
