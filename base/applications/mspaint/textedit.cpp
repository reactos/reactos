/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint/textedit.cpp
 * PURPOSE:     Text editor and font chooser for the text tool
 * PROGRAMMERS: Benedikt Freisen
 */

/* INCLUDES *********************************************************/

#include "precomp.h"

const INT c_cxyGrip = 4;

/* FUNCTIONS ********************************************************/

SIZE CTextEditWindow::DoCalcRect(HDC hDC, LPTSTR pszText, INT cchText, LPRECT prcParent, LPCTSTR pszOldText)
{
    RECT rc;
    GetClientRect(&rc);
    MapWindowPoints(NULL, (LPPOINT)&rc, 2);

    TEXTMETRIC tm;
    GetTextMetrics(hDC, &tm);

    DWORD dwMargin = (DWORD)DefWindowProc(EM_GETMARGINS, 0, 0);
    LONG leftMargin = LOWORD(dwMargin), rightMargin = HIWORD(dwMargin);

    INT x = leftMargin, y = tm.tmHeight;
    INT xMax = x, yMax = y;
    INT ich;
    for (ich = 0; ich < cchText; ++ich)
    {
        if (pszText[ich] == TEXT('\r'))
            continue;

        if (pszText[ich] == TEXT('\n'))
        {
            if (rc.top + yMax + tm.tmHeight > prcParent->bottom)
            {
                pszText[ich] = 0;
                break;
            }
            x = leftMargin;
            y += tm.tmHeight;
            if (yMax < y)
                yMax = y;
            continue;
        }

        SIZE siz;
        GetTextExtentPoint32(hDC, &pszText[ich], 1, &siz);
        x += siz.cx;
        if (xMax < x)
            xMax = x;
    }

    if (ich != cchText && pszOldText)
    {
        DWORD iStart, iEnd;
        SendMessage(EM_GETSEL, (WPARAM)&iStart, (LPARAM)&iEnd);
        SetWindowText(pszOldText);
        SendMessage(EM_SETSEL, iStart, iEnd);
        MessageBeep(0xFFFFFFFF);
    }

    SIZE ret = { xMax + rightMargin, yMax };
    return ret;
}

#define X0 rc.left
#define X1 ((rc.left + rc.right - c_cxyGrip) / 2)
#define X2 (rc.right - c_cxyGrip)
#define Y0 rc.top
#define Y1 ((rc.top + rc.bottom - c_cxyGrip) / 2)
#define Y2 (rc.bottom - c_cxyGrip)
#define RECT0 X0, Y0, X0 + c_cxyGrip, Y0 + c_cxyGrip
#define RECT1 X1, Y0, X1 + c_cxyGrip, Y0 + c_cxyGrip
#define RECT2 X2, Y0, X2 + c_cxyGrip, Y0 + c_cxyGrip
#define RECT3 X0, Y1, X0 + c_cxyGrip, Y1 + c_cxyGrip
#define RECT4 X2, Y1, X2 + c_cxyGrip, Y1 + c_cxyGrip
#define RECT5 X0, Y2, X0 + c_cxyGrip, Y2 + c_cxyGrip
#define RECT6 X1, Y2, X1 + c_cxyGrip, Y2 + c_cxyGrip
#define RECT7 X2, Y2, X2 + c_cxyGrip, Y2 + c_cxyGrip

INT CTextEditWindow::HitTestGrip(RECT& rc, POINT pt)
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
    return HTCLIENT;
}

void CTextEditWindow::DrawGrip(HDC hDC, RECT& rc)
{
    HGDIOBJ hbrOld = SelectObject(hDC, GetStockObject(NULL_BRUSH));
    HPEN hPen = CreatePen(PS_DOT, 1, GetSysColor(COLOR_HIGHLIGHT));
    HGDIOBJ hPenOld = SelectObject(hDC, hPen);
    Rectangle(hDC, rc.left, rc.top, rc.right, rc.bottom);
    SelectObject(hDC, hPenOld);
    SelectObject(hDC, hbrOld);
    DeleteObject(hPen);

    RECT rcGrip;

    SetRect(&rcGrip, RECT0);
    FillRect(hDC, &rcGrip, GetSysColorBrush(COLOR_HIGHLIGHT));
    SetRect(&rcGrip, RECT1);
    FillRect(hDC, &rcGrip, GetSysColorBrush(COLOR_HIGHLIGHT));
    SetRect(&rcGrip, RECT2);
    FillRect(hDC, &rcGrip, GetSysColorBrush(COLOR_HIGHLIGHT));

    SetRect(&rcGrip, RECT3);
    FillRect(hDC, &rcGrip, GetSysColorBrush(COLOR_HIGHLIGHT));
    SetRect(&rcGrip, RECT4);
    FillRect(hDC, &rcGrip, GetSysColorBrush(COLOR_HIGHLIGHT));

    SetRect(&rcGrip, RECT5);
    FillRect(hDC, &rcGrip, GetSysColorBrush(COLOR_HIGHLIGHT));
    SetRect(&rcGrip, RECT6);
    FillRect(hDC, &rcGrip, GetSysColorBrush(COLOR_HIGHLIGHT));
    SetRect(&rcGrip, RECT7);
    FillRect(hDC, &rcGrip, GetSysColorBrush(COLOR_HIGHLIGHT));
}

void CTextEditWindow::InvalidateEdit(LPTSTR pszText)
{
    TCHAR szText[512];
    INT cchText = GetWindowText(szText, _countof(szText));

    RECT rcParent;
    imageArea.GetWindowRect(&rcParent);

    RECT rc, rcWnd, rcText;
    GetWindowRect(&rcWnd);
    rcText = rcWnd;

    if (HDC hDC = GetDC())
    {
        SelectObject(hDC, (HFONT)SendMessage(WM_GETFONT, 0, 0));
        SIZE siz = DoCalcRect(hDC, szText, cchText, &rcParent, pszText);
        ReleaseDC(hDC);

        rcText.right = rcText.left + siz.cx;
        rcText.bottom = rcText.top + siz.cy;
    }

    UnionRect(&rc, &rcText, &rcWnd);
    ::MapWindowPoints(NULL, imageArea, (LPPOINT)&rc, 2);

    rcWnd = rc;
    imageArea.GetClientRect(&rcParent);
    IntersectRect(&rc, &rcParent, &rcWnd);

    MoveWindow(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);

    DWORD dwMargin = (DWORD)SendMessage(EM_GETMARGINS, 0, 0);
    LONG leftMargin = LOWORD(dwMargin), rightMargin = HIWORD(dwMargin);

    rc.left += leftMargin;
    rc.right -= rightMargin;
    ::MapWindowPoints(imageArea, m_hWnd, (LPPOINT)&rc, 2);
    SendMessage(EM_SETRECT, 0, (LPARAM)&rc);

    DefWindowProc(WM_HSCROLL, SB_LEFT, 0);
    DefWindowProc(WM_VSCROLL, SB_TOP, 0);

    imageArea.InvalidateRect(NULL, TRUE);
}

CTextEditWindow::CTextEditWindow() : m_hFont(NULL)
{
    ZeroMemory(&m_lf, sizeof(m_lf));
}

CTextEditWindow::~CTextEditWindow()
{
}

LRESULT CTextEditWindow::OnChar(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    TCHAR szText[512];
    GetWindowText(szText, _countof(szText));
    LRESULT ret = DefWindowProc(nMsg, wParam, lParam);
    InvalidateEdit(szText);
    return ret;
}

LRESULT CTextEditWindow::OnKeyDown(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    TCHAR szText[512];
    GetWindowText(szText, _countof(szText));
    LRESULT ret = DefWindowProc(nMsg, wParam, lParam);
    InvalidateEdit(szText);
    return ret;
}

LRESULT CTextEditWindow::OnKeyUp(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    TCHAR szText[512];
    GetWindowText(szText, _countof(szText));
    LRESULT ret = DefWindowProc(nMsg, wParam, lParam);
    InvalidateEdit(szText);
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

    if (HDC hDC = GetDC())
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
    if (HDC hDC = GetDCEx(NULL, DCX_WINDOW | DCX_PARENTCLIP))
    {
        OffsetRect(&rc, -rc.left, -rc.top);
        DrawGrip(hDC, rc);
        ReleaseDC(hDC);
    }
    return 0;
}

LRESULT CTextEditWindow::OnNCCalcSize(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    return 0;
}

LRESULT CTextEditWindow::OnNCHitTest(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
    RECT rc;
    GetWindowRect(&rc);
    return HitTestGrip(rc, pt);
}

LRESULT CTextEditWindow::OnMove(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LRESULT ret = DefWindowProc(nMsg, wParam, lParam);
    imageArea.InvalidateRect(NULL, TRUE);
    return ret;
}

LRESULT CTextEditWindow::OnSize(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LRESULT ret = DefWindowProc(nMsg, wParam, lParam);
    RECT rc;
    GetClientRect(&rc);
    SendMessage(EM_SETRECT, 0, (LPARAM)&rc);
    SendMessage(EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(0, 0));
    imageArea.InvalidateRect(NULL, TRUE);
    return ret;
}

// Hack: Use DECLARE_WND_SUPERCLASS instead!
HWND CTextEditWindow::Create(HWND hwndParent)
{
    DWORD style = ES_LEFT | ES_MULTILINE | ES_WANTRETURN | ES_AUTOHSCROLL | ES_AUTOVSCROLL |
                  WS_CHILD | WS_THICKFRAME;
    DWORD exstyle = 0;
    m_hWnd = ::CreateWindowEx(exstyle, WC_EDIT, NULL, style,
                              0, 0, 0, 0,
                              hwndParent, NULL, hProgInstance, NULL);
    if (m_hWnd)
    {
#undef SubclassWindow
        SubclassWindow(m_hWnd);

        if (!m_hFont)
            m_hFont = ::CreateFontIndirect(&m_lf);

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

void CTextEditWindow::DoDraw(HWND hwnd, HDC hDC)
{
    TCHAR szText[512];
    INT cchText = GetWindowText(szText, _countof(szText));

    RECT rc;
    SendMessage(EM_GETRECT, 0, (LPARAM)&rc);
    MapWindowPoints(hwnd, (LPPOINT)&rc, 2);

    HGDIOBJ hFontOld = SelectObject(hDC, m_hFont);
    UINT uFormat = DT_LEFT | DT_TOP | DT_EDITCONTROL | DT_NOPREFIX;

    if (toolsModel.IsBackgroundTransparent())
    {
        SetBkMode(hDC, TRANSPARENT);
    }
    else
    {
        SetBkMode(hDC, OPAQUE);
        SetBkColor(hDC, paletteModel.GetBgColor());

        HBRUSH hbr = CreateSolidBrush(paletteModel.GetBgColor());
        FillRect(hDC, &rc, hbr);
        DeleteObject(hbr);
    }
    SetTextColor(hDC, paletteModel.GetFgColor());
    DrawText(hDC, szText, cchText, &rc, uFormat);
    SelectObject(hDC, hFontOld);
}

LRESULT CTextEditWindow::OnCreate(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
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
    return 0;
}

void CTextEditWindow::InvalidateEdit2()
{
    RECT rc;
    GetWindowRect(&rc);
    ::MapWindowPoints(NULL, imageArea, (LPPOINT)&rc, 2);
    imageArea.InvalidateRect(&rc, TRUE);
}

LRESULT CTextEditWindow::OnPaletteModelColorChanged(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    InvalidateEdit2();
    return 0;
}

LRESULT CTextEditWindow::OnToolsModelSettingsChanged(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    InvalidateEdit2();
    return 0;
}

LRESULT CTextEditWindow::OnToolsModelToolChanged(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (wParam == TOOL_TEXT)
    {
        if (m_hFont)
        {
            DeleteObject(m_hFont);
            m_hFont = NULL;
        }

        lstrcpyn(m_lf.lfFaceName, fontsDialog.GetFontName(), _countof(m_lf.lfFaceName));

        INT nFontSize = fontsDialog.GetFontSize();
        HDC hdc = GetDC();
        m_lf.lfHeight = -MulDiv (nFontSize, GetDeviceCaps(hdc, LOGPIXELSY), 72);
        ReleaseDC(hdc);

        m_lf.lfWeight = (fontsDialog.IsBold() ? FW_BOLD : FW_NORMAL);
        m_lf.lfItalic = fontsDialog.IsItalic();
        m_lf.lfUnderline = fontsDialog.IsUnderline();

        if (!m_hFont)
            m_hFont = ::CreateFontIndirect(&m_lf);

        SetWindowFont(m_hWnd, m_hFont, TRUE);
        InvalidateEdit2();
    }
    else
    {
        ShowWindow(SW_HIDE);
    }
    return 0;
}

LRESULT CTextEditWindow::OnSetSel(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LRESULT ret = DefWindowProc(nMsg, wParam, lParam);
    InvalidateEdit2();
    return ret;
}
