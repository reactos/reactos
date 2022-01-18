/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint/textedit.cpp
 * PURPOSE:     Text editor and font chooser for the text tool
 * PROGRAMMERS: Benedikt Freisen
 */

/* INCLUDES *********************************************************/

#include "precomp.h"

#define CXY_GRIP 3

/* FUNCTIONS ********************************************************/

CTextEditWindow::CTextEditWindow() : m_hFont(NULL), m_hFontZoomed(NULL), m_nAppIsMovingOrSizing(0)
{
    SetRectEmpty(&m_rc);
}

#define X0 rc.left
#define X1 ((rc.left + rc.right - CXY_GRIP) / 2)
#define X2 (rc.right - CXY_GRIP)
#define Y0 rc.top
#define Y1 ((rc.top + rc.bottom - CXY_GRIP) / 2)
#define Y2 (rc.bottom - CXY_GRIP)
#define RECT0 X0, Y0, X0 + CXY_GRIP, Y0 + CXY_GRIP // Upper Left
#define RECT1 X1, Y0, X1 + CXY_GRIP, Y0 + CXY_GRIP // Top
#define RECT2 X2, Y0, X2 + CXY_GRIP, Y0 + CXY_GRIP // Upper Right
#define RECT3 X0, Y1, X0 + CXY_GRIP, Y1 + CXY_GRIP // Left
#define RECT4 X2, Y1, X2 + CXY_GRIP, Y1 + CXY_GRIP // Right
#define RECT5 X0, Y2, X0 + CXY_GRIP, Y2 + CXY_GRIP // Lower Left
#define RECT6 X1, Y2, X1 + CXY_GRIP, Y2 + CXY_GRIP // Bottom
#define RECT7 X2, Y2, X2 + CXY_GRIP, Y2 + CXY_GRIP // Lower Right

INT CTextEditWindow::DoHitTest(RECT& rc, POINT pt)
{
    RECT rcGrip;

    SetRect(&rcGrip, RECT0);
    if (PtInRect(&rcGrip, pt))
        return HTTOPLEFT;
    SetRect(&rcGrip, RECT1);
    if (PtInRect(&rcGrip, pt))
        return HTTOP;
    SetRect(&rcGrip, RECT2);
    if (PtInRect(&rcGrip, pt))
        return HTTOPRIGHT;

    SetRect(&rcGrip, RECT3);
    if (PtInRect(&rcGrip, pt))
        return HTLEFT;
    SetRect(&rcGrip, RECT4);
    if (PtInRect(&rcGrip, pt))
        return HTRIGHT;

    SetRect(&rcGrip, RECT5);
    if (PtInRect(&rcGrip, pt))
        return HTBOTTOMLEFT;
    SetRect(&rcGrip, RECT6);
    if (PtInRect(&rcGrip, pt))
        return HTBOTTOM;
    SetRect(&rcGrip, RECT7);
    if (PtInRect(&rcGrip, pt))
        return HTBOTTOMRIGHT;

    // On border line?
    RECT rcInner = rc;
    InflateRect(&rcInner, -3, -3);
    if (!PtInRect(&rcInner, pt) && PtInRect(&rc, pt))
        return HTCAPTION;

    return HTCLIENT;
}

void CTextEditWindow::DrawGrip(HDC hDC, RECT& rc)
{
    HGDIOBJ hbrOld = SelectObject(hDC, GetStockObject(NULL_BRUSH));
    HPEN hPen = CreatePen(PS_DOT, 1, GetSysColor(COLOR_HIGHLIGHT));
    HGDIOBJ hPenOld = SelectObject(hDC, hPen);
    InflateRect(&rc, -1, -1);
    Rectangle(hDC, rc.left, rc.top, rc.right, rc.bottom);
    InflateRect(&rc, 1, 1);
    SelectObject(hDC, hPenOld);
    SelectObject(hDC, hbrOld);
    DeleteObject(hPen);

    RECT rcGrip;
    HBRUSH hbrHighlight = GetSysColorBrush(COLOR_HIGHLIGHT);

    SetRect(&rcGrip, RECT0);
    FillRect(hDC, &rcGrip, hbrHighlight);
    SetRect(&rcGrip, RECT1);
    FillRect(hDC, &rcGrip, hbrHighlight);
    SetRect(&rcGrip, RECT2);
    FillRect(hDC, &rcGrip, hbrHighlight);

    SetRect(&rcGrip, RECT3);
    FillRect(hDC, &rcGrip, hbrHighlight);
    SetRect(&rcGrip, RECT4);
    FillRect(hDC, &rcGrip, hbrHighlight);

    SetRect(&rcGrip, RECT5);
    FillRect(hDC, &rcGrip, hbrHighlight);
    SetRect(&rcGrip, RECT6);
    FillRect(hDC, &rcGrip, hbrHighlight);
    SetRect(&rcGrip, RECT7);
    FillRect(hDC, &rcGrip, hbrHighlight);
}

void CTextEditWindow::FixEditPos(LPCTSTR pszOldText)
{
    CString szText;
    GetWindowText(szText);

    RECT rcParent;
    ::GetWindowRect(m_hwndParent, &rcParent);

    RECT rc, rcWnd, rcText;
    GetWindowRect(&rcWnd);
    rcText = rcWnd;

    HDC hDC = GetDC();
    if (hDC)
    {
        SelectObject(hDC, m_hFontZoomed);
        TEXTMETRIC tm;
        GetTextMetrics(hDC, &tm);
        szText += TEXT("x"); // This is a trick to enable the last newlines
        const UINT uFormat = DT_LEFT | DT_TOP | DT_EDITCONTROL | DT_NOPREFIX | DT_NOCLIP |
                             DT_EXPANDTABS | DT_WORDBREAK;
        DrawText(hDC, szText, -1, &rcText, uFormat | DT_CALCRECT);
        if (tm.tmDescent > 0)
            rcText.bottom += tm.tmDescent;
        ReleaseDC(hDC);
    }

    UnionRect(&rc, &rcText, &rcWnd);
    ::MapWindowPoints(NULL, m_hwndParent, (LPPOINT)&rc, 2);

    rcWnd = rc;
    ::GetClientRect(m_hwndParent, &rcParent);
    IntersectRect(&rc, &rcParent, &rcWnd);

    ++m_nAppIsMovingOrSizing;
    MoveWindow(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, FALSE);
    --m_nAppIsMovingOrSizing;

    DefWindowProc(WM_HSCROLL, SB_LEFT, 0);
    DefWindowProc(WM_VSCROLL, SB_TOP, 0);

    ::InvalidateRect(m_hwndParent, &rc, TRUE);
}

LRESULT CTextEditWindow::OnChar(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (wParam == VK_TAB)
        return 0; // FIXME: Tabs

    CString szText;
    GetWindowText(szText);

    LRESULT ret = DefWindowProc(nMsg, wParam, lParam);
    FixEditPos(szText);

    return ret;
}

LRESULT CTextEditWindow::OnKeyDown(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (wParam == VK_ESCAPE)
    {
        toolsModel.OnCancelDraw();
        return 0;
    }

    CString szText;
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
    SetTextColor(hDC, paletteModel.GetFgColor());
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
    RECT rc;
    GetWindowRect(&rc);

    HDC hDC = GetDCEx(NULL, DCX_WINDOW | DCX_PARENTCLIP);
    if (hDC)
    {
        OffsetRect(&rc, -rc.left, -rc.top);
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
    UINT nHitTest = LOWORD(lParam);
    if (nHitTest == HTCAPTION)
    {
        SetCursor(LoadCursor(NULL, IDC_SIZEALL));
        return FALSE;
    }
    return DefWindowProc(nMsg, wParam, lParam);
}

LRESULT CTextEditWindow::OnMove(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LRESULT ret = DefWindowProc(nMsg, wParam, lParam);

    if (m_nAppIsMovingOrSizing == 0)
    {
        Reposition();
        InvalidateEditRect();
    }
    return ret;
}

LRESULT CTextEditWindow::OnSize(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LRESULT ret = DefWindowProc(nMsg, wParam, lParam);

    RECT rc;
    GetClientRect(&rc);
    SendMessage(EM_SETRECTNP, 0, (LPARAM)&rc);
    SendMessage(EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(0, 0));

    if (m_nAppIsMovingOrSizing == 0)
    {
        Reposition();
        InvalidateEditRect();
    }

    return ret;
}

// Hack: Use DECLARE_WND_SUPERCLASS instead!
HWND CTextEditWindow::Create(HWND hwndParent)
{
    m_hwndParent = hwndParent;

    const DWORD style = ES_LEFT | ES_MULTILINE | ES_WANTRETURN | ES_AUTOVSCROLL |
                        WS_CHILD | WS_THICKFRAME;
    m_hWnd = ::CreateWindowEx(0, WC_EDIT, NULL, style, 0, 0, 0, 0,
                              hwndParent, NULL, hProgInstance, NULL);
    if (m_hWnd)
    {
#undef SubclassWindow // Don't use this macro
        SubclassWindow(m_hWnd);

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
    MapWindowPoints(imageArea, (LPPOINT)&rc, 2);
    rc.left = UnZoomed(rc.left);
    rc.top = UnZoomed(rc.top);
    rc.right = UnZoomed(rc.right);
    rc.bottom = UnZoomed(rc.bottom);
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

    LOGFONT lf;
    ZeroMemory(&lf, sizeof(lf));
    lf.lfCharSet = DEFAULT_CHARSET; // registrySettings.CharSet; // Ignore
    lf.lfWeight = (registrySettings.Bold ? FW_BOLD : FW_NORMAL);
    lf.lfItalic = registrySettings.Italic;
    lf.lfUnderline = registrySettings.Underline;
    lstrcpyn(lf.lfFaceName, registrySettings.strFontName, _countof(lf.lfFaceName));

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
    INT x0 = Zoomed(m_rc.left), y0 = Zoomed(m_rc.top);
    INT x1 = Zoomed(m_rc.right), y1 = Zoomed(m_rc.bottom);

    ++m_nAppIsMovingOrSizing;
    MoveWindow(x0, y0, x1 - x0, y1 - y0, TRUE);
    --m_nAppIsMovingOrSizing;
}

void CTextEditWindow::Reposition()
{
    RECT rc, rcImage;
    GetWindowRect(&rc);

    ::MapWindowPoints(NULL, imageArea, (LPPOINT)&rc, 2);
    imageArea.GetClientRect(&rcImage);

    if (rc.bottom > rcImage.bottom)
    {
        rc.top = rcImage.bottom - (rc.bottom - rc.top);
        rc.bottom = rcImage.bottom;
    }

    if (rc.right > rcImage.right)
    {
        rc.left = rcImage.right - (rc.right - rc.left);
        rc.right = rcImage.right;
    }

    if (rc.left < 0)
    {
        rc.right += -rc.left;
        rc.left = 0;
    }

    if (rc.top < 0)
    {
        rc.bottom += -rc.top;
        rc.top = 0;
    }

    ++m_nAppIsMovingOrSizing;
    MoveWindow(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);
    --m_nAppIsMovingOrSizing;
}

LRESULT CTextEditWindow::OnMouseWheel(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    return ::SendMessage(GetParent(), nMsg, wParam, lParam);
}
