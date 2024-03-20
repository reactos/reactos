/*
 * PROJECT:     ReactOS Cicero
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Cicero UIF Library
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"
#include "cicuif.h"

/////////////////////////////////////////////////////////////////////////////
// static members

HINSTANCE CUIFTheme::s_hUXTHEME = NULL;
FN_OpenThemeData CUIFTheme::s_fnOpenThemeData = NULL;
FN_CloseThemeData CUIFTheme::s_fnCloseThemeData = NULL;
FN_DrawThemeBackground CUIFTheme::s_fnDrawThemeBackground = NULL;
FN_DrawThemeParentBackground CUIFTheme::s_fnDrawThemeParentBackground = NULL;
FN_DrawThemeText CUIFTheme::s_fnDrawThemeText = NULL;
FN_DrawThemeIcon CUIFTheme::s_fnDrawThemeIcon = NULL;
FN_GetThemeBackgroundExtent CUIFTheme::s_fnGetThemeBackgroundExtent = NULL;
FN_GetThemeBackgroundContentRect CUIFTheme::s_fnGetThemeBackgroundContentRect = NULL;
FN_GetThemeTextExtent CUIFTheme::s_fnGetThemeTextExtent = NULL;
FN_GetThemePartSize CUIFTheme::s_fnGetThemePartSize = NULL;
FN_DrawThemeEdge CUIFTheme::s_fnDrawThemeEdge = NULL;
FN_GetThemeColor CUIFTheme::s_fnGetThemeColor = NULL;
FN_GetThemeMargins CUIFTheme::s_fnGetThemeMargins = NULL;
FN_GetThemeFont CUIFTheme::s_fnGetThemeFont = NULL;
FN_GetThemeSysColor CUIFTheme::s_fnGetThemeSysColor = NULL;
FN_GetThemeSysSize CUIFTheme::s_fnGetThemeSysSize = NULL;

CUIFSystemInfo *CUIFSystemInfo::s_pSystemInfo = NULL;

CUIFColorTableSys *CUIFScheme::s_pColorTableSys = NULL;
CUIFColorTableOff10 *CUIFScheme::s_pColorTableOff10 = NULL;

/////////////////////////////////////////////////////////////////////////////

void CUIFSystemInfo::GetSystemMetrics()
{
    HDC hDC = ::GetDC(NULL);
    m_cBitsPixels = ::GetDeviceCaps(hDC, BITSPIXEL);
    ::ReleaseDC(NULL, hDC);

    HIGHCONTRAST HighContrast = { sizeof(HighContrast) };
    ::SystemParametersInfo(SPI_GETHIGHCONTRAST, sizeof(HighContrast), &HighContrast, 0);
    m_bHighContrast1 = !!(HighContrast.dwFlags & HCF_HIGHCONTRASTON);
    COLORREF rgbBtnText = ::GetSysColor(COLOR_BTNTEXT);
    COLORREF rgbBtnFace = ::GetSysColor(COLOR_BTNFACE);
    const COLORREF black = RGB(0, 0, 0), white = RGB(255, 255, 255);
    m_bHighContrast2 = (m_bHighContrast1 ||
                        (rgbBtnText == black && rgbBtnFace == white) ||
                        (rgbBtnText == white && rgbBtnFace == black));
}

void CUIFSystemInfo::Initialize()
{
    dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    ::GetVersionEx(this);
    GetSystemMetrics();
}

/////////////////////////////////////////////////////////////////////////////
// CUIFTheme

HRESULT CUIFTheme::InternalOpenThemeData(HWND hWnd)
{
    if (!hWnd || !m_pszClassList)
        return E_FAIL;

    if (!cicGetFN(s_hUXTHEME, s_fnOpenThemeData, TEXT("uxtheme.dll"), "OpenThemeData"))
        return E_FAIL;
    m_hTheme = s_fnOpenThemeData(hWnd, m_pszClassList);
    return (m_hTheme ? S_OK : E_FAIL);
}

HRESULT CUIFTheme::EnsureThemeData(HWND hWnd)
{
    if (m_hTheme)
        return S_OK;
    return InternalOpenThemeData(hWnd);
}

HRESULT CUIFTheme::CloseThemeData()
{
    if (!m_hTheme)
        return S_OK;

    if (!cicGetFN(s_hUXTHEME, s_fnCloseThemeData, TEXT("uxtheme.dll"), "CloseThemeData"))
        return E_FAIL;

    HRESULT hr = s_fnCloseThemeData(m_hTheme);
    m_hTheme = NULL;
    return hr;
}

STDMETHODIMP
CUIFTheme::DrawThemeBackground(HDC hDC, int iStateId, LPCRECT pRect, LPCRECT pClipRect)
{
    if (!cicGetFN(s_hUXTHEME, s_fnDrawThemeBackground, TEXT("uxtheme.dll"), "DrawThemeBackground"))
        return E_FAIL;
    return s_fnDrawThemeBackground(m_hTheme, hDC, m_iPartId, iStateId, pRect, pClipRect);
}

STDMETHODIMP
CUIFTheme::DrawThemeParentBackground(HWND hwnd, HDC hDC, LPRECT prc)
{
    if (!cicGetFN(s_hUXTHEME, s_fnDrawThemeParentBackground, TEXT("uxtheme.dll"), "DrawThemeParentBackground"))
        return E_FAIL;
    return s_fnDrawThemeParentBackground(hwnd, hDC, prc);
}

STDMETHODIMP
CUIFTheme::DrawThemeText(HDC hDC, int iStateId, LPCWSTR pszText, int cchText, DWORD dwTextFlags, DWORD dwTextFlags2, LPCRECT pRect)
{
    if (!cicGetFN(s_hUXTHEME, s_fnDrawThemeText, TEXT("uxtheme.dll"), "DrawThemeText"))
        return E_FAIL;
    return s_fnDrawThemeText(m_hTheme, hDC, m_iPartId, iStateId, pszText, cchText, dwTextFlags, dwTextFlags2, pRect);
}

STDMETHODIMP
CUIFTheme::DrawThemeIcon(HDC hDC, int iStateId, LPCRECT pRect, HIMAGELIST himl, int iImageIndex)
{
    if (!cicGetFN(s_hUXTHEME, s_fnDrawThemeIcon, TEXT("uxtheme.dll"), "DrawThemeIcon"))
        return E_FAIL;
    return s_fnDrawThemeIcon(m_hTheme, hDC, m_iPartId, iStateId, pRect, himl, iImageIndex);
}

STDMETHODIMP
CUIFTheme::GetThemeBackgroundExtent(HDC hDC, int iStateId, LPCRECT pContentRect, LPRECT pExtentRect)
{
    if (!cicGetFN(s_hUXTHEME, s_fnGetThemeBackgroundExtent, TEXT("uxtheme.dll"), "GetThemeBackgroundExtent"))
        return E_FAIL;
    return s_fnGetThemeBackgroundExtent(m_hTheme, hDC, m_iPartId, iStateId, pContentRect, pExtentRect);
}

STDMETHODIMP
CUIFTheme::GetThemeBackgroundContentRect(HDC hDC, int iStateId, LPCRECT pBoundingRect, LPRECT pContentRect)
{
    if (!cicGetFN(s_hUXTHEME, s_fnGetThemeBackgroundContentRect, TEXT("uxtheme.dll"), "GetThemeBackgroundContentRect"))
        return E_FAIL;
    return s_fnGetThemeBackgroundContentRect(m_hTheme, hDC, m_iPartId, iStateId, pBoundingRect, pContentRect);
}

STDMETHODIMP
CUIFTheme::GetThemeTextExtent(HDC hDC, int iStateId, LPCWSTR pszText, int cchCharCount, DWORD dwTextFlags, LPCRECT pBoundingRect, LPRECT pExtentRect)
{
    if (!cicGetFN(s_hUXTHEME, s_fnGetThemeTextExtent, TEXT("uxtheme.dll"), "GetThemeTextExtent"))
        return E_FAIL;
    return s_fnGetThemeTextExtent(m_hTheme, hDC, m_iPartId, iStateId, pszText, cchCharCount, dwTextFlags, pBoundingRect, pExtentRect);
}

STDMETHODIMP
CUIFTheme::GetThemePartSize(HDC hDC, int iStateId, LPRECT prc, THEMESIZE eSize, SIZE *psz)
{
    if (!cicGetFN(s_hUXTHEME, s_fnGetThemePartSize, TEXT("uxtheme.dll"), "GetThemePartSize"))
        return E_FAIL;
    return s_fnGetThemePartSize(m_hTheme, hDC, m_iPartId, iStateId, prc, eSize, psz);
}

STDMETHODIMP
CUIFTheme::DrawThemeEdge(HDC hDC, int iStateId, LPCRECT pDestRect, UINT uEdge, UINT uFlags, LPRECT pContentRect)
{
    if (!cicGetFN(s_hUXTHEME, s_fnDrawThemeEdge, TEXT("uxtheme.dll"), "DrawThemeEdge"))
        return E_FAIL;
    return s_fnDrawThemeEdge(m_hTheme, hDC, m_iPartId, iStateId, pDestRect, uEdge, uFlags, pContentRect);
}

STDMETHODIMP
CUIFTheme::GetThemeColor(int iStateId, int iPropId, COLORREF *pColor)
{
    if (!cicGetFN(s_hUXTHEME, s_fnGetThemeColor, TEXT("uxtheme.dll"), "GetThemeColor"))
        return E_FAIL;
    return s_fnGetThemeColor(m_hTheme, m_iPartId, iStateId, iPropId, pColor);
}

STDMETHODIMP
CUIFTheme::GetThemeMargins(HDC hDC, int iStateId, int iPropId, LPRECT prc, MARGINS *pMargins)
{
    if (!cicGetFN(s_hUXTHEME, s_fnGetThemeMargins, TEXT("uxtheme.dll"), "GetThemeMargins"))
        return E_FAIL;
    return s_fnGetThemeMargins(m_hTheme, hDC, m_iPartId, iStateId, iPropId, prc, pMargins);
}

STDMETHODIMP
CUIFTheme::GetThemeFont(HDC hDC, int iStateId, int iPropId, LOGFONTW *pFont)
{
    if (!cicGetFN(s_hUXTHEME, s_fnGetThemeFont, TEXT("uxtheme.dll"), "GetThemeFont"))
        return E_FAIL;
    return s_fnGetThemeFont(m_hTheme, hDC, m_iPartId, iStateId, iPropId, pFont);
}

STDMETHODIMP_(COLORREF)
CUIFTheme::GetThemeSysColor(INT iColorId)
{
    if (!cicGetFN(s_hUXTHEME, s_fnGetThemeSysColor, TEXT("uxtheme.dll"), "GetThemeSysColor"))
        return RGB(0, 0, 0);
    return s_fnGetThemeSysColor(m_hTheme, iColorId);
}

STDMETHODIMP_(int)
CUIFTheme::GetThemeSysSize(int iSizeId)
{
    if (!cicGetFN(s_hUXTHEME, s_fnGetThemeSysSize, TEXT("uxtheme.dll"), "GetThemeSysSize"))
        return 0;
    return s_fnGetThemeSysSize(m_hTheme, iSizeId);
}

STDMETHODIMP_(void)
CUIFTheme::SetActiveTheme(LPCWSTR pszClassList, INT iPartId, INT iStateId)
{
    m_iPartId = iPartId;
    m_iStateId = iStateId;
    m_pszClassList = pszClassList;
}

/////////////////////////////////////////////////////////////////////////////
// CUIFObject

/// @unimplemented
CUIFObject::CUIFObject(CUIFObject *pParent, DWORD nObjectID, LPCRECT prc, DWORD style)
{
    m_pszClassList = NULL;
    m_hTheme = NULL;
    m_pParent = pParent;
    m_nObjectID = nObjectID;
    m_style = style;

    if (prc)
        m_rc = *prc;
    else
        m_rc = { 0, 0, 0, 0 };

    if (m_pParent)
    {
        m_pWindow = m_pParent->m_pWindow;
        m_pScheme = m_pParent->m_pScheme;
    }
    else
    {
        m_pWindow = NULL;
        m_pScheme = NULL;
    }

    m_bEnable = m_bVisible = TRUE;

    m_hFont = (HFONT)::GetStockObject(DEFAULT_GUI_FONT);
    m_bHasCustomFont = FALSE;

    m_pszToolTip = NULL;

    m_dwUnknown4[0] = -1; //FIXME: name
    m_dwUnknown4[1] = -1; //FIXME: name
}

CUIFObject::~CUIFObject()
{
    if (m_pWindow)
    {
        CUIFToolTip *pToolTip = m_pWindow->m_pToolTip;
        if (pToolTip && pToolTip->m_pToolTipTarget == this)
            pToolTip->m_pToolTipTarget = NULL;
    }

    if (m_pszToolTip)
    {
        delete[] m_pszToolTip;
        m_pszToolTip = NULL;
    }

    for (size_t iObj = m_ObjectArray.size(); iObj > 0; )
    {
        --iObj;
        delete m_ObjectArray[iObj];
    }
    m_ObjectArray.clear();

    if (m_pWindow)
        m_pWindow->RemoveUIObj(this);

    CloseThemeData();
}

STDMETHODIMP_(void) CUIFObject::OnPaint(HDC hDC)
{
    if (!(m_pWindow->m_style & UIF_WINDOW_ENABLETHEMED) || !OnPaintTheme(hDC))
        OnPaintNoTheme(hDC);
}

STDMETHODIMP_(BOOL) CUIFObject::OnSetCursor(UINT uMsg, LONG x, LONG y)
{
    return FALSE;
}

STDMETHODIMP_(void) CUIFObject::GetRect(LPRECT prc)
{
    *prc = m_rc;
}

STDMETHODIMP_(BOOL) CUIFObject::PtInObject(POINT pt)
{
    return m_bVisible && ::PtInRect(&m_rc, pt);
}

STDMETHODIMP_(void) CUIFObject::PaintObject(HDC hDC, LPCRECT prc)
{
    if (!m_bVisible)
        return;

    if (!prc)
        prc = &m_rc;

    OnPaint(hDC);

    for (size_t iItem = 0; iItem < m_ObjectArray.size(); ++iItem)
    {
        CUIFObject *pObject = m_ObjectArray[iItem];
        RECT rc;
        if (::IntersectRect(&rc, prc, &pObject->m_rc))
            pObject->PaintObject(hDC, &rc);
    }
}

STDMETHODIMP_(void) CUIFObject::CallOnPaint()
{
    if (m_pWindow)
        m_pWindow->UpdateUI(&m_rc);
}

STDMETHODIMP_(void) CUIFObject::Enable(BOOL bEnable)
{
    if (m_bEnable == bEnable)
        return;

    m_bEnable = bEnable;
    for (size_t iItem = 0; iItem < m_ObjectArray.size(); ++iItem)
        m_ObjectArray[iItem]->Enable(bEnable);

    CallOnPaint();
}

STDMETHODIMP_(void) CUIFObject::Show(BOOL bVisible)
{
    if (m_bVisible == bVisible)
        return;

    m_bVisible = bVisible;
    for (size_t iItem = 0; iItem < m_ObjectArray.size(); ++iItem)
        m_ObjectArray[iItem]->Show(bVisible);

    if (m_bVisible || m_pParent)
        m_pParent->CallOnPaint();
}

STDMETHODIMP_(void) CUIFObject::SetFontToThis(HFONT hFont)
{
    m_bHasCustomFont = !!hFont;
    if (!hFont)
        hFont = (HFONT)::GetStockObject(DEFAULT_GUI_FONT);
    m_hFont = hFont;
}

STDMETHODIMP_(void) CUIFObject::SetFont(HFONT hFont)
{
    SetFontToThis(hFont);

    for (size_t iItem = 0; iItem < m_ObjectArray.size(); ++iItem)
        m_ObjectArray[iItem]->SetFont(hFont);

    CallOnPaint();
}

STDMETHODIMP_(void) CUIFObject::SetStyle(DWORD style)
{
    m_style = style;
}

STDMETHODIMP_(void) CUIFObject::AddUIObj(CUIFObject *pObject)
{
    m_ObjectArray.Add(pObject);
    CallOnPaint();
}

STDMETHODIMP_(void) CUIFObject::RemoveUIObj(CUIFObject *pObject)
{
    if (m_ObjectArray.Remove(pObject))
        CallOnPaint();
}

STDMETHODIMP_(LRESULT) CUIFObject::OnObjectNotify(CUIFObject *pObject, WPARAM wParam, LPARAM lParam)
{
    if (m_pParent)
        return m_pParent->OnObjectNotify(pObject, wParam, lParam);
    return 0;
}

STDMETHODIMP_(void) CUIFObject::SetToolTip(LPCWSTR pszToolTip)
{
    if (m_pszToolTip)
    {
        delete[] m_pszToolTip;
        m_pszToolTip = NULL;
    }

    if (pszToolTip)
    {
        size_t cch = wcslen(pszToolTip);
        m_pszToolTip = new(cicNoThrow) WCHAR[cch + 1];
        if (m_pszToolTip)
            lstrcpynW(m_pszToolTip, pszToolTip, cch + 1);
    }
}

STDMETHODIMP_(void) CUIFObject::ClearWndObj()
{
    m_pWindow = NULL;
    for (size_t iItem = 0; iItem < m_ObjectArray.size(); ++iItem)
        m_ObjectArray[iItem]->ClearWndObj();
}

STDMETHODIMP_(void) CUIFObject::ClearTheme()
{
    CloseThemeData();
    for (size_t iItem = 0; iItem < m_ObjectArray.size(); ++iItem)
        m_ObjectArray[iItem]->ClearTheme();
}

void CUIFObject::StartCapture()
{
    if (m_pWindow)
        m_pWindow->SetCaptureObject(this);
}

void CUIFObject::EndCapture()
{
    if (m_pWindow)
        m_pWindow->SetCaptureObject(NULL);
}

BOOL CUIFObject::IsCapture()
{
    return m_pWindow && (m_pWindow->m_pCaptured == this);
}

void CUIFObject::SetRect(LPCRECT prc)
{
    m_rc = *prc;
    if (m_pWindow)
        m_pWindow->OnObjectMoved(this);
    CallOnPaint();
}

LRESULT CUIFObject::NotifyCommand(WPARAM wParam, LPARAM lParam)
{
    if (m_pParent)
        return m_pParent->OnObjectNotify(this, wParam, lParam);
    return 0;
}

void CUIFObject::DetachWndObj()
{
    if (m_pWindow)
    {
        CUIFToolTip *pToolTip = m_pWindow->m_pToolTip;
        if (pToolTip && pToolTip->m_pToolTipTarget == this)
            pToolTip->m_pToolTipTarget = NULL;

        m_pWindow->RemoveUIObj(this);
        m_pWindow = NULL;
    }
}

BOOL CUIFObject::IsRTL()
{
    if (!m_pWindow)
        return FALSE;
    return !!(m_pWindow->m_style & UIF_WINDOW_LAYOUTRTL);
}

CUIFObject* CUIFObject::ObjectFromPoint(POINT pt)
{
    if (!PtInObject(pt))
        return NULL;

    CUIFObject *pFound = this;
    for (size_t i = 0; i < m_ObjectArray.size(); ++i)
    {
        CUIFObject *pObject = m_ObjectArray[i]->ObjectFromPoint(pt);
        if (pObject)
            pFound = pObject;
    }
    return pFound;
}

void CUIFObject::SetScheme(CUIFScheme *scheme)
{
    m_pScheme = scheme;
    for (size_t i = 0; i < m_ObjectArray.size(); ++i)
    {
        m_ObjectArray[i]->SetScheme(scheme);
    }
}

void CUIFObject::StartTimer(WPARAM wParam)
{
    if (m_pWindow)
        m_pWindow->SetTimerObject(this, wParam);
}

void CUIFObject::EndTimer()
{
    if (m_pWindow)
        m_pWindow->SetTimerObject(NULL, 0);
}

/////////////////////////////////////////////////////////////////////////////
// CUIFColorTable...

STDMETHODIMP_(void) CUIFColorTableSys::InitColor()
{
    m_rgbColors[0] = ::GetSysColor(COLOR_BTNFACE);
    m_rgbColors[1] = ::GetSysColor(COLOR_BTNSHADOW);
    m_rgbColors[2] = ::GetSysColor(COLOR_ACTIVEBORDER);
    m_rgbColors[3] = ::GetSysColor(COLOR_ACTIVECAPTION);
    m_rgbColors[4] = ::GetSysColor(COLOR_BTNFACE);
    m_rgbColors[5] = ::GetSysColor(COLOR_BTNSHADOW);
    m_rgbColors[6] = ::GetSysColor(COLOR_BTNTEXT);
    m_rgbColors[7] = ::GetSysColor(COLOR_CAPTIONTEXT);
    m_rgbColors[8] = ::GetSysColor(COLOR_GRAYTEXT);
    m_rgbColors[9] = ::GetSysColor(COLOR_HIGHLIGHT);
    m_rgbColors[10] = ::GetSysColor(COLOR_HIGHLIGHTTEXT);
    m_rgbColors[11] = ::GetSysColor(COLOR_INACTIVECAPTION);
    m_rgbColors[12] = ::GetSysColor(COLOR_INACTIVECAPTIONTEXT);
    m_rgbColors[13] = ::GetSysColor(COLOR_MENUTEXT);
    m_rgbColors[14] = ::GetSysColor(COLOR_WINDOW);
    m_rgbColors[15] = ::GetSysColor(COLOR_WINDOWTEXT);
}

STDMETHODIMP_(void) CUIFColorTableSys::InitBrush()
{
    ZeroMemory(m_hBrushes, sizeof(m_hBrushes));
}

STDMETHODIMP_(void) CUIFColorTableSys::DoneBrush()
{
    for (size_t i = 0; i < _countof(m_hBrushes); ++i)
    {
        if (m_hBrushes[i])
        {
            ::DeleteObject(m_hBrushes[i]);
            m_hBrushes[i] = NULL;
        }
    }
}

HBRUSH CUIFColorTableSys::GetBrush(INT iColor)
{
    if (!m_hBrushes[iColor])
        m_hBrushes[iColor] = ::CreateSolidBrush(m_rgbColors[iColor]);
    return m_hBrushes[iColor];
}

HBRUSH CUIFColorTableOff10::GetBrush(INT iColor)
{
    if (!m_hBrushes[iColor])
        m_hBrushes[iColor] = ::CreateSolidBrush(m_rgbColors[iColor]);
    return m_hBrushes[iColor];
}

/// @unimplemented
STDMETHODIMP_(void) CUIFColorTableOff10::InitColor()
{
    m_rgbColors[0] = ::GetSysColor(COLOR_BTNFACE);
    m_rgbColors[1] = ::GetSysColor(COLOR_WINDOW);
    m_rgbColors[2] = ::GetSysColor(COLOR_WINDOW);
    m_rgbColors[3] = ::GetSysColor(COLOR_BTNFACE);
    m_rgbColors[4] = ::GetSysColor(COLOR_BTNSHADOW);
    m_rgbColors[5] = ::GetSysColor(COLOR_WINDOW);
    m_rgbColors[6] = ::GetSysColor(COLOR_HIGHLIGHT);
    m_rgbColors[7] = ::GetSysColor(COLOR_WINDOWTEXT);
    m_rgbColors[8] = ::GetSysColor(COLOR_HIGHLIGHT);
    m_rgbColors[9] = ::GetSysColor(COLOR_HIGHLIGHT);
    m_rgbColors[10] = ::GetSysColor(COLOR_HIGHLIGHTTEXT);
    m_rgbColors[11] = ::GetSysColor(COLOR_BTNFACE);
    m_rgbColors[12] = ::GetSysColor(COLOR_BTNTEXT);
    m_rgbColors[13] = ::GetSysColor(COLOR_BTNSHADOW);
    m_rgbColors[14] = ::GetSysColor(COLOR_BTNFACE);
    m_rgbColors[15] = ::GetSysColor(COLOR_WINDOW);
    m_rgbColors[16] = ::GetSysColor(COLOR_HIGHLIGHT);
    m_rgbColors[17] = ::GetSysColor(COLOR_BTNTEXT);
    m_rgbColors[18] = ::GetSysColor(COLOR_WINDOW);
    m_rgbColors[19] = ::GetSysColor(COLOR_BTNSHADOW);
    m_rgbColors[20] = ::GetSysColor(COLOR_BTNFACE);
    m_rgbColors[21] = ::GetSysColor(COLOR_BTNSHADOW);
    m_rgbColors[22] = ::GetSysColor(COLOR_BTNFACE);
    m_rgbColors[23] = ::GetSysColor(COLOR_BTNSHADOW);
    m_rgbColors[24] = ::GetSysColor(COLOR_CAPTIONTEXT);
    m_rgbColors[25] = ::GetSysColor(COLOR_HIGHLIGHT);
    m_rgbColors[26] = ::GetSysColor(COLOR_HIGHLIGHTTEXT);
    m_rgbColors[27] = ::GetSysColor(COLOR_BTNFACE);
    m_rgbColors[28] = ::GetSysColor(COLOR_BTNTEXT);
    m_rgbColors[29] = ::GetSysColor(COLOR_BTNSHADOW);
    m_rgbColors[30] = ::GetSysColor(COLOR_BTNTEXT);
    m_rgbColors[31] = ::GetSysColor(COLOR_WINDOWTEXT);
}

STDMETHODIMP_(void) CUIFColorTableOff10::InitBrush()
{
    ZeroMemory(m_hBrushes, sizeof(m_hBrushes));
}

STDMETHODIMP_(void) CUIFColorTableOff10::DoneBrush()
{
    for (size_t i = 0; i < _countof(m_hBrushes); ++i)
    {
        if (m_hBrushes[i])
        {
            ::DeleteObject(m_hBrushes[i]);
            m_hBrushes[i] = NULL;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
// CUIFScheme

/// @unimplemented
CUIFScheme *cicCreateUIFScheme(DWORD type)
{
#if 1
    return new(cicNoThrow) CUIFSchemeDef(type);
#else
    switch (type)
    {
        case 1:  return new(cicNoThrow) CUIFSchemeOff10(1);
        case 2:  return new(cicNoThrow) CUIFSchemeOff10(2);
        case 3:  return new(cicNoThrow) CUIFSchemeOff10(3);
        default: return new(cicNoThrow) CUIFSchemeDef(type);
    }
#endif
}

STDMETHODIMP_(DWORD) CUIFSchemeDef::GetType()
{
    return m_dwType;
}

STDMETHODIMP_(COLORREF) CUIFSchemeDef::GetColor(INT iColor)
{
    return s_pColorTableSys->GetColor(iColor);
}

STDMETHODIMP_(HBRUSH) CUIFSchemeDef::GetBrush(INT iColor)
{
    return s_pColorTableSys->GetBrush(iColor);
}

STDMETHODIMP_(INT) CUIFSchemeDef::CyMenuItem(INT cyText)
{
    return cyText + 2;
}

STDMETHODIMP_(INT) CUIFSchemeDef::CxSizeFrame()
{
    return ::GetSystemMetrics(SM_CXSIZEFRAME);
}

STDMETHODIMP_(INT) CUIFSchemeDef::CySizeFrame()
{
    return ::GetSystemMetrics(SM_CYSIZEFRAME);
}

STDMETHODIMP_(void) CUIFScheme::FillRect(HDC hDC, LPCRECT prc, INT iColor)
{
    ::FillRect(hDC, prc, GetBrush(iColor));
}

STDMETHODIMP_(void) CUIFScheme::FrameRect(HDC hDC, LPCRECT prc, INT iColor)
{
    ::FrameRect(hDC, prc, GetBrush(iColor));
}

STDMETHODIMP_(void) CUIFSchemeDef::DrawSelectionRect(HDC hDC, LPCRECT prc, int)
{
    ::FillRect(hDC, prc, GetBrush(6));
}

STDMETHODIMP_(void)
CUIFSchemeDef::GetCtrlFaceOffset(DWORD dwUnknownFlags, DWORD dwDrawFlags, LPSIZE pSize)
{
    if (!(dwDrawFlags & UIF_DRAW_PRESSED))
    {
        if (dwDrawFlags & 0x2)
        {
            if (dwUnknownFlags & 0x10)
            {
                pSize->cx = pSize->cy = -1;
                return;
            }
            pSize->cx = pSize->cy = !!(dwUnknownFlags & 0x20);
        }
        else
        {
            if (!(dwDrawFlags & 0x1))
            {
                pSize->cx = pSize->cy = -((dwUnknownFlags & 1) != 0);
                return;
            }
            if (dwUnknownFlags & 0x4)
            {
                pSize->cx = pSize->cy = -1;
                return;
            }
            pSize->cx = pSize->cy = !!(dwUnknownFlags & 0x8);
        }
        return;
    }

    if (!(dwUnknownFlags & 0x40))
    {
        pSize->cx = pSize->cy = !!(dwUnknownFlags & 0x80);
        return;
    }

    pSize->cx = pSize->cy = -1;
}

STDMETHODIMP_(void)
CUIFSchemeDef::DrawCtrlBkgd(HDC hDC, LPCRECT prc, DWORD dwUnknownFlags, DWORD dwDrawFlags)
{
    ::FillRect(hDC, prc, GetBrush(9));

    if (!(dwDrawFlags & UIF_DRAW_PRESSED) && (dwDrawFlags & 0x2))
        return;

    HBRUSH hbrDither = cicCreateDitherBrush();
    if (!hbrDither)
        return;

    COLORREF rgbOldText = ::SetTextColor(hDC, ::GetSysColor(COLOR_BTNFACE));
    COLORREF rgbOldBack = ::SetBkColor(hDC, ::GetSysColor(COLOR_BTNHIGHLIGHT));

    RECT rc = *prc;
    ::InflateRect(&rc, -2, -2);
    ::FillRect(hDC, &rc, hbrDither);
    ::SetTextColor(hDC, rgbOldText);
    ::SetBkColor(hDC, rgbOldBack);
    ::DeleteObject(hbrDither);
}

STDMETHODIMP_(void)
CUIFSchemeDef::DrawCtrlEdge(
    HDC hDC,
    LPCRECT prc,
    DWORD dwUnknownFlags,
    DWORD dwDrawFlags)
{
    UINT uEdge = BDR_RAISEDINNER;

    if (dwDrawFlags & 0x10)
    {
        if (!(dwUnknownFlags & 0x40))
        {
            if (dwUnknownFlags & 0x80)
                uEdge = BDR_SUNKENOUTER;
            else
                return;
        }
    }
    else if (dwDrawFlags & 0x2)
    {
        if (!(dwUnknownFlags & 0x10))
        {
            if (dwUnknownFlags & 0x20)
                uEdge = BDR_SUNKENOUTER;
            else
                return;
        }
    }
    else if (dwDrawFlags & 0x1)
    {
        if (!(dwUnknownFlags & 0x4))
        {
            if (dwUnknownFlags & 0x8)
                uEdge = BDR_SUNKENOUTER;
            else
                return;
        }
    }
    else if (!(dwUnknownFlags & 0x1))
    {
        return;
    }

    RECT rc = *prc;
    ::DrawEdge(hDC, &rc, uEdge, BF_RECT);
}

STDMETHODIMP_(void)
CUIFSchemeDef::DrawCtrlText(
    HDC hDC,
    LPCRECT prc,
    LPCWSTR pszText,
    INT cchText,
    DWORD dwDrawFlags,
    BOOL bRight)
{
    COLORREF rgbOldText = ::GetTextColor(hDC);
    INT OldBkMode = ::SetBkMode(hDC, TRANSPARENT);

    if (cchText == -1)
        cchText = lstrlenW(pszText);

    RECT rc = *prc;
    if (dwDrawFlags & UIF_DRAW_DISABLED)
    {
        ::OffsetRect(&rc, 1, 1);
        ::SetTextColor(hDC, ::GetSysColor(COLOR_BTNHIGHLIGHT));
        ::ExtTextOutW(hDC, (bRight ? rc.right : rc.left), rc.top, ETO_CLIPPED, &rc,
                      pszText, cchText, NULL);
        ::OffsetRect(&rc, -1, -1);
    }

    ::SetTextColor(hDC, ::GetSysColor(COLOR_BTNTEXT));
    ::ExtTextOutW(hDC, (bRight ? rc.right : rc.left), rc.top, ETO_CLIPPED, &rc,
                  pszText, cchText, NULL);

    ::SetTextColor(hDC, rgbOldText);
    ::SetBkMode(hDC, OldBkMode);
}

STDMETHODIMP_(void)
CUIFSchemeDef::DrawCtrlIcon(HDC hDC, LPCRECT prc, HICON hIcon, DWORD dwDrawFlags, LPSIZE pSize)
{
    if (m_bMirroring)
    {
        HBITMAP hbm1, hbm2;
        if (cicGetIconBitmaps(hIcon, &hbm1, &hbm2, pSize))
        {
            DrawCtrlBitmap(hDC, prc, hbm1, hbm2, dwDrawFlags);
            ::DeleteObject(hbm1);
            ::DeleteObject(hbm2);
        }
    }
    else
    {
        UINT uFlags = DST_PREFIXTEXT | DST_TEXT;
        if (dwDrawFlags & UIF_DRAW_DISABLED)
            uFlags |= (DSS_MONO | DSS_DISABLED);
        ::DrawState(hDC, 0, 0, (LPARAM)hIcon, 0, prc->left, prc->top, 0, 0, uFlags);
    }
}

STDMETHODIMP_(void)
CUIFSchemeDef::DrawCtrlBitmap(HDC hDC, LPCRECT prc, HBITMAP hbm1, HBITMAP hbm2, DWORD dwDrawFlags)
{
    if (m_bMirroring)
    {
        hbm1 = cicMirrorBitmap(hbm1, GetBrush(9));
        hbm2 = cicMirrorBitmap(hbm2, (HBRUSH)GetStockObject(BLACK_BRUSH));
    }

    HBRUSH hBrush = (HBRUSH)UlongToHandle(COLOR_BTNFACE + 1);
    BOOL bBrushCreated = FALSE;
    if (hbm2)
    {
        HBITMAP hBitmap = NULL;
        if (dwDrawFlags & UIF_DRAW_DISABLED)
        {
            hBitmap = cicCreateDisabledBitmap(prc, hbm2, GetBrush(9), GetBrush(11), TRUE);
        }
        else
        {
            if ((dwDrawFlags & UIF_DRAW_PRESSED) && !(dwDrawFlags & 0x2))
            {
                hBrush = cicCreateDitherBrush();
                bBrushCreated = TRUE;
            }

            COLORREF rgbFace = ::GetSysColor(COLOR_BTNFACE);
            COLORREF rgbHighlight = ::GetSysColor(COLOR_BTNHIGHLIGHT);
            hBitmap = cicCreateMaskBmp(prc, hbm1, hbm2, hBrush, rgbFace, rgbHighlight);
        }

        if (hBitmap)
        {
            ::DrawState(hDC, NULL, NULL, (LPARAM)hBitmap, 0,
                        prc->left, prc->top,
                        prc->right - prc->left, prc->bottom - prc->top,
                        DST_BITMAP);
            ::DeleteObject(hBitmap);
        }
    }
    else
    {
        UINT uFlags = DST_BITMAP;
        if (dwDrawFlags & UIF_DRAW_DISABLED)
            uFlags |= (DSS_MONO | DSS_DISABLED);

        ::DrawState(hDC, NULL, NULL, (LPARAM)hbm1, 0,
                    prc->left, prc->top,
                    prc->right - prc->left, prc->bottom - prc->top,
                    uFlags);
    }

    if (bBrushCreated)
        ::DeleteObject(hBrush);

    if (m_bMirroring)
    {
        ::DeleteObject(hbm1);
        ::DeleteObject(hbm2);
    }
}

STDMETHODIMP_(void)
CUIFSchemeDef::DrawMenuBitmap(HDC hDC, LPCRECT prc, HBITMAP hbm1, HBITMAP hbm2, DWORD dwDrawFlags)
{
    DrawCtrlBitmap(hDC, prc, hbm1, hbm2, dwDrawFlags);
}

STDMETHODIMP_(void)
CUIFSchemeDef::DrawMenuSeparator(HDC hDC, LPCRECT prc)
{
    RECT rc = *prc;
    rc.bottom = rc.top + (rc.bottom - rc.top) / 2;
    ::FillRect(hDC, &rc, (HBRUSH)UlongToHandle(COLOR_BTNSHADOW + 1));

    rc = *prc;
    rc.top += (rc.bottom - rc.top) / 2;
    ::FillRect(hDC, &rc, (HBRUSH)UlongToHandle(COLOR_BTNHIGHLIGHT + 1));
}

STDMETHODIMP_(void)
CUIFSchemeDef::DrawFrameCtrlBkgd(HDC hDC, LPCRECT prc, DWORD dwUnknownFlags, DWORD dwDrawFlags)
{
    DrawCtrlBkgd(hDC, prc, dwUnknownFlags, dwDrawFlags);
}

STDMETHODIMP_(void)
CUIFSchemeDef::DrawFrameCtrlEdge(HDC hDC, LPCRECT prc, DWORD dwUnknownFlags, DWORD dwDrawFlags)
{
    DrawCtrlEdge(hDC, prc, dwUnknownFlags, dwDrawFlags);
}

STDMETHODIMP_(void)
CUIFSchemeDef::DrawFrameCtrlIcon(HDC hDC, LPCRECT prc, HICON hIcon, DWORD dwDrawFlags, LPSIZE pSize)
{
    DrawCtrlIcon(hDC, prc, hIcon, dwDrawFlags, pSize);
}

STDMETHODIMP_(void)
CUIFSchemeDef::DrawFrameCtrlBitmap(HDC hDC, LPCRECT prc, HBITMAP hbm1, HBITMAP hbm2, DWORD dwDrawFlags)
{
    DrawCtrlBitmap(hDC, prc, hbm1, hbm2, dwDrawFlags);
}

STDMETHODIMP_(void)
CUIFSchemeDef::DrawWndFrame(HDC hDC, LPCRECT prc, DWORD type, DWORD unused1, DWORD unused2)
{
    RECT rc = *prc;
    if (type && type <= 2)
        ::DrawEdge(hDC, &rc, BDR_RAISED, BF_RECT);
    else
        FrameRect(hDC, &rc, 14);
}

STDMETHODIMP_(void)
CUIFSchemeDef::DrawDragHandle(HDC hDC, LPCRECT prc, BOOL bVertical)
{
    RECT rc;
    if (bVertical)
        rc = { prc->left, prc->top + 1, prc->right, prc->top + 4 };
    else
        rc = { prc->left + 1, prc->top, prc->left + 4, prc->bottom };
    ::DrawEdge(hDC, &rc, BDR_RAISEDINNER, BF_RECT);
}

STDMETHODIMP_(void)
CUIFSchemeDef::DrawSeparator(HDC hDC, LPCRECT prc, BOOL bVertical)
{
    HPEN hLightPen = ::CreatePen(PS_SOLID, 0, ::GetSysColor(COLOR_BTNHIGHLIGHT));
    if (!hLightPen)
        return;

    HPEN hShadowPen = CreatePen(PS_SOLID, 0, GetSysColor(COLOR_BTNSHADOW));
    if (!hShadowPen)
    {
        ::DeleteObject(hLightPen);
        return;
    }

    HGDIOBJ hPenOld = ::SelectObject(hDC, hShadowPen);
    if (bVertical)
    {
        ::MoveToEx(hDC, prc->left, prc->top + 1, NULL);
        ::LineTo(hDC, prc->right, prc->top + 1);
        ::SelectObject(hDC, hLightPen);
        ::MoveToEx(hDC, prc->left, prc->top + 2, NULL);
        ::LineTo(hDC, prc->right, prc->top + 2);
    }
    else
    {
        ::MoveToEx(hDC, prc->left + 1, prc->top, NULL);
        ::LineTo(hDC, prc->left + 1, prc->bottom);
        ::SelectObject(hDC, hLightPen);
        ::MoveToEx(hDC, prc->left + 2, prc->top, NULL);
        ::LineTo(hDC, prc->left + 2, prc->bottom);
    }
    ::SelectObject(hDC, hPenOld);

    ::DeleteObject(hShadowPen);
    ::DeleteObject(hLightPen);
}

/////////////////////////////////////////////////////////////////////////////
// CUIFIcon

HIMAGELIST CUIFIcon::GetImageList(BOOL bMirror)
{
    if (!m_hImageList)
        return NULL;

    if (m_hIcon)
    {
        SIZE iconSize;
        cicGetIconSize(m_hIcon, &iconSize);

        UINT uFlags = ILC_COLOR32 | ILC_MASK;
        if (bMirror)
            uFlags |= ILC_MIRROR;

        m_hImageList = ImageList_Create(iconSize.cx, iconSize.cy, uFlags, 1, 0);
        if (m_hImageList)
            ImageList_ReplaceIcon(m_hImageList, -1, m_hIcon);

        return m_hImageList;
    }

    return NULL;
}

/////////////////////////////////////////////////////////////////////////////
// CUIFBitmapDC

CUIFBitmapDC::CUIFBitmapDC(BOOL bMemory)
{
    m_hBitmap = NULL;
    m_hOldBitmap = NULL;
    m_hOldObject = NULL;
    if (bMemory)
        m_hDC = ::CreateCompatibleDC(NULL);
    else
        m_hDC = ::CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);
}

CUIFBitmapDC::~CUIFBitmapDC()
{
    Uninit();
    ::DeleteDC(m_hDC);
}

void CUIFBitmapDC::Uninit(BOOL bKeep)
{
    if (m_hOldBitmap)
    {
        ::SelectObject(m_hDC, m_hOldBitmap);
        m_hOldBitmap = NULL;
    }

    if (m_hOldObject)
    {
        ::SelectObject(m_hDC, m_hOldObject);
        m_hOldObject = NULL;
    }

    if (!bKeep)
    {
        if (m_hBitmap)
        {
            ::DeleteObject(m_hBitmap);
            m_hBitmap = NULL;
        }
    }
}

BOOL CUIFBitmapDC::SetBitmap(HBITMAP hBitmap)
{
    if (m_hDC)
        m_hOldBitmap = ::SelectObject(m_hDC, hBitmap);
    return TRUE;
}

BOOL CUIFBitmapDC::SetBitmap(LONG cx, LONG cy, WORD cPlanes, WORD cBitCount)
{
    m_hBitmap = ::CreateBitmap(cx, cy, cPlanes, cBitCount, 0);
    m_hOldBitmap = ::SelectObject(m_hDC, m_hBitmap);
    return TRUE;
}

BOOL CUIFBitmapDC::SetDIB(LONG cx, LONG cy, WORD cPlanes, WORD cBitCount)
{
    BITMAPINFO bmi;
    ZeroMemory(&bmi, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth = cx;
    bmi.bmiHeader.biHeight = cy;
    bmi.bmiHeader.biPlanes = cPlanes;
    bmi.bmiHeader.biBitCount = cBitCount;
    bmi.bmiHeader.biCompression = BI_RGB;
    m_hBitmap = ::CreateDIBSection(m_hDC, &bmi, DIB_RGB_COLORS, NULL, NULL, 0);
    m_hOldBitmap = ::SelectObject(m_hDC, m_hBitmap);
    return TRUE;
}

void cicInitUIFUtil(void)
{
    if (!CUIFBitmapDC::s_phdcSrc)
        CUIFBitmapDC::s_phdcSrc = new(cicNoThrow) CUIFBitmapDC(TRUE);

    if (!CUIFBitmapDC::s_phdcMask)
        CUIFBitmapDC::s_phdcMask = new(cicNoThrow) CUIFBitmapDC(TRUE);

    if (!CUIFBitmapDC::s_phdcDst)
        CUIFBitmapDC::s_phdcDst = new(cicNoThrow) CUIFBitmapDC(TRUE);

    if (CUIFBitmapDC::s_phdcSrc && CUIFBitmapDC::s_phdcMask && CUIFBitmapDC::s_phdcDst)
        CUIFBitmapDC::s_fInitBitmapDCs = TRUE;
}

void cicDoneUIFUtil(void)
{
    if (CUIFBitmapDC::s_phdcSrc)
    {
        delete CUIFBitmapDC::s_phdcSrc;
        CUIFBitmapDC::s_phdcSrc = NULL;
    }

    if (CUIFBitmapDC::s_phdcMask)
    {
        delete CUIFBitmapDC::s_phdcMask;
        CUIFBitmapDC::s_phdcMask = NULL;
    }

    if (CUIFBitmapDC::s_phdcDst)
    {
        delete CUIFBitmapDC::s_phdcDst;
        CUIFBitmapDC::s_phdcDst = NULL;
    }

    CUIFBitmapDC::s_fInitBitmapDCs = FALSE;
}

HBITMAP cicMirrorBitmap(HBITMAP hBitmap, HBRUSH hbrBack)
{
    BITMAP bm;
    if (!CUIFBitmapDC::s_fInitBitmapDCs || !::GetObject(hBitmap, sizeof(bm), &bm))
        return NULL;

    CUIFBitmapDC::s_phdcSrc->SetBitmap(hBitmap);
    CUIFBitmapDC::s_phdcDst->SetDIB(bm.bmWidth, bm.bmHeight, 1, 32);
    CUIFBitmapDC::s_phdcMask->SetDIB(bm.bmWidth, bm.bmHeight, 1, 32);

    RECT rc = { 0, 0, bm.bmWidth, bm.bmHeight };
    FillRect(*CUIFBitmapDC::s_phdcDst, &rc, hbrBack);

    ::SetLayout(*CUIFBitmapDC::s_phdcMask, LAYOUT_RTL);

    ::BitBlt(*CUIFBitmapDC::s_phdcMask, 0, 0, bm.bmWidth, bm.bmHeight, *CUIFBitmapDC::s_phdcSrc, 0, 0, SRCCOPY);

    ::SetLayout(*CUIFBitmapDC::s_phdcMask, 0);

    ::BitBlt(*CUIFBitmapDC::s_phdcDst, 0, 0, bm.bmWidth, bm.bmHeight, *CUIFBitmapDC::s_phdcMask, 1, 0, SRCCOPY);

    CUIFBitmapDC::s_phdcSrc->Uninit();
    CUIFBitmapDC::s_phdcMask->Uninit();
    CUIFBitmapDC::s_phdcDst->Uninit(TRUE);
    return CUIFBitmapDC::s_phdcDst->DetachBitmap();
}

HBRUSH cicCreateDitherBrush(VOID)
{
    BYTE bytes[16];
    ZeroMemory(&bytes, sizeof(bytes));
    bytes[0] = bytes[4] = bytes[8] = bytes[12] = 0x55;
    bytes[2] = bytes[6] = bytes[10] = bytes[14] = 0xAA;
    HBITMAP hBitmap = ::CreateBitmap(8, 8, 1, 1, bytes);
    if (!hBitmap)
        return NULL;

    LOGBRUSH lb;
    lb.lbHatch = (ULONG_PTR)hBitmap;
    lb.lbStyle = BS_PATTERN;
    HBRUSH hbr = ::CreateBrushIndirect(&lb);
    ::DeleteObject(hBitmap);
    return hbr;
}

HBITMAP
cicCreateDisabledBitmap(LPCRECT prc, HBITMAP hbmMask, HBRUSH hbr1, HBRUSH hbr2, BOOL bPressed)
{
    if (!CUIFBitmapDC::s_fInitBitmapDCs)
        return NULL;

    LONG width = prc->right - prc->left, height = prc->bottom - prc->top;

    CUIFBitmapDC::s_phdcDst->SetDIB(width, height, 1, 32);
    CUIFBitmapDC::s_phdcMask->SetBitmap(hbmMask);
    CUIFBitmapDC::s_phdcSrc->SetDIB(width, height, 1, 32);

    RECT rc = { 0, 0, width, height };
    ::FillRect(*CUIFBitmapDC::s_phdcDst, &rc, hbr1);

    HBRUSH hbrWhite = (HBRUSH)GetStockObject(WHITE_BRUSH);
    ::FillRect(*CUIFBitmapDC::s_phdcSrc, &rc, hbrWhite);

    ::BitBlt(*CUIFBitmapDC::s_phdcSrc, 0, 0, width, height, *CUIFBitmapDC::s_phdcMask, 0, 0, SRCINVERT);
    if (bPressed)
        ::BitBlt(*CUIFBitmapDC::s_phdcDst, 1, 1, width, height,
                 *CUIFBitmapDC::s_phdcSrc, 0, 0, SRCPAINT);
    else
        ::BitBlt(*CUIFBitmapDC::s_phdcDst, 0, 0, width, height,
                 *CUIFBitmapDC::s_phdcSrc, 0, 0, SRCPAINT);

    ::FillRect(*CUIFBitmapDC::s_phdcSrc, &rc, hbr2);

    ::BitBlt(*CUIFBitmapDC::s_phdcSrc, 0, 0, width, height, *CUIFBitmapDC::s_phdcMask, 0, 0, SRCPAINT);
    ::BitBlt(*CUIFBitmapDC::s_phdcDst, 0, 0, width, height, *CUIFBitmapDC::s_phdcSrc, 0, 0, SRCAND);

    CUIFBitmapDC::s_phdcSrc->Uninit();
    CUIFBitmapDC::s_phdcMask->Uninit();
    CUIFBitmapDC::s_phdcDst->Uninit(TRUE);
    return CUIFBitmapDC::s_phdcDst->DetachBitmap();
}

HBITMAP
cicCreateShadowMaskBmp(LPRECT prc, HBITMAP hbm1, HBITMAP hbm2, HBRUSH hbr1, HBRUSH hbr2)
{
    if (!CUIFBitmapDC::s_fInitBitmapDCs)
        return NULL;

    --prc->left;
    --prc->top;

    LONG width = prc->right - prc->left;
    LONG height = prc->bottom - prc->top;

    CUIFBitmapDC bitmapDC(TRUE);

    CUIFBitmapDC::s_phdcDst->SetDIB(width, height, 1, 32);
    CUIFBitmapDC::s_phdcSrc->SetBitmap(hbm1);
    CUIFBitmapDC::s_phdcMask->SetBitmap(hbm2);
    bitmapDC.SetDIB(width, height, 1, 32);

    RECT rc = { 0, 0, width, height };

    ::FillRect(*CUIFBitmapDC::s_phdcDst, &rc, hbr1);
    ::FillRect(bitmapDC, &rc, hbr2);

    ::BitBlt(bitmapDC, 0, 0, width, height, *CUIFBitmapDC::s_phdcMask, 0, 0, SRCPAINT);
    ::BitBlt(*CUIFBitmapDC::s_phdcDst, 2, 2, width, height, bitmapDC, 0, 0, SRCAND);
    ::BitBlt(*CUIFBitmapDC::s_phdcDst, 0, 0, width, height, *CUIFBitmapDC::s_phdcMask, 0, 0, SRCAND);
    ::BitBlt(*CUIFBitmapDC::s_phdcDst, 0, 0, width, height, *CUIFBitmapDC::s_phdcSrc, 0, 0, SRCINVERT);

    CUIFBitmapDC::s_phdcSrc->Uninit();
    CUIFBitmapDC::s_phdcMask->Uninit();
    CUIFBitmapDC::s_phdcDst->Uninit(TRUE);
    return CUIFBitmapDC::s_phdcDst->DetachBitmap();
}

HBITMAP
cicChangeBitmapColor(LPCRECT prc, HBITMAP hbm, COLORREF rgbBack, COLORREF rgbFore)
{
    if (!CUIFBitmapDC::s_fInitBitmapDCs)
        return NULL;

    INT width = prc->right - prc->left;
    INT height = prc->bottom - prc->top;

    CUIFSolidBrush brush(rgbFore);

    CUIFBitmapDC::s_phdcDst->SetDIB(width, height, 1, 32);
    CUIFBitmapDC::s_phdcSrc->SetBitmap(hbm);
    CUIFBitmapDC::s_phdcMask->SetBitmap(width, height, 1, 1);

    ::BitBlt(*CUIFBitmapDC::s_phdcDst, 0, 0, width, height, *CUIFBitmapDC::s_phdcSrc, 0, 0, SRCCOPY);
    ::SelectObject(*CUIFBitmapDC::s_phdcDst, (HBRUSH)brush);
    ::SetBkColor(*CUIFBitmapDC::s_phdcDst, rgbBack);

    ::BitBlt(*CUIFBitmapDC::s_phdcMask, 0, 0, width, height, *CUIFBitmapDC::s_phdcDst, 0, 0, MERGECOPY);
    ::SetBkColor(*CUIFBitmapDC::s_phdcDst, RGB(255, 255, 255));
    ::SetTextColor(*CUIFBitmapDC::s_phdcDst, RGB(0, 0, 0));
    ::BitBlt(*CUIFBitmapDC::s_phdcDst, 0, 0, width, height, *CUIFBitmapDC::s_phdcMask, 0, 0, 0xE20746u);

    CUIFBitmapDC::s_phdcSrc->Uninit();
    CUIFBitmapDC::s_phdcMask->Uninit();
    CUIFBitmapDC::s_phdcDst->Uninit(TRUE);
    return CUIFBitmapDC::s_phdcDst->DetachBitmap();
}

HBITMAP
cicConvertBlackBKGBitmap(LPCRECT prc, HBITMAP hbm1, HBITMAP hbm2, HBRUSH hBrush)
{
    if (!CUIFBitmapDC::s_fInitBitmapDCs)
        return NULL;

    if (IS_INTRESOURCE(hBrush))
        hBrush = ::GetSysColorBrush(HandleToLong(hBrush) - 1);

    LOGBRUSH lb;
    ::GetObject(hBrush, sizeof(lb), &lb);
    if (lb.lbStyle || lb.lbColor)
        return NULL;

    INT width = prc->right - prc->left;
    INT height = prc->bottom - prc->top;

    HBITMAP hBitmap = cicChangeBitmapColor(prc, hbm1, 0, RGB(255, 255, 255));
    if ( !hBitmap )
        return NULL;

    CUIFBitmapDC::s_phdcDst->SetDIB(width, height, 1, 32);
    CUIFBitmapDC::s_phdcSrc->SetBitmap(hBitmap);
    CUIFBitmapDC::s_phdcMask->SetBitmap(hbm2);

    RECT rc = { 0, 0, width, height };

    HBRUSH hbrWhite = (HBRUSH)GetStockObject(WHITE_BRUSH);
    ::FillRect(*CUIFBitmapDC::s_phdcDst, &rc, hbrWhite);

    ::BitBlt(*CUIFBitmapDC::s_phdcDst, 0, 0, width, height, *CUIFBitmapDC::s_phdcMask, 0, 0, 0x660046u);
    ::BitBlt(*CUIFBitmapDC::s_phdcDst, 0, 0, width, height, *CUIFBitmapDC::s_phdcSrc, 0, 0, 0x8800C6u);

    CUIFBitmapDC::s_phdcSrc->Uninit();
    CUIFBitmapDC::s_phdcMask->Uninit();
    CUIFBitmapDC::s_phdcDst->Uninit(TRUE);
    ::DeleteObject(hBitmap);
    return CUIFBitmapDC::s_phdcDst->DetachBitmap();
}

HBITMAP
cicCreateMaskBmp(LPCRECT prc, HBITMAP hbm1, HBITMAP hbm2,
                 HBRUSH hbr, COLORREF rgbColor, COLORREF rgbBack)
{
    if (!CUIFBitmapDC::s_fInitBitmapDCs)
        return NULL;

    INT width = prc->right - prc->left;
    INT height = prc->bottom - prc->top;
    HBITMAP hBitmap = cicConvertBlackBKGBitmap(prc, hbm1, hbm2, hbr);
    if (hBitmap)
        return hBitmap;

    CUIFBitmapDC::s_phdcDst->SetDIB(width, height, 1, 32);
    CUIFBitmapDC::s_phdcSrc->SetBitmap(hbm1);
    CUIFBitmapDC::s_phdcMask->SetBitmap(hbm2);

    RECT rc = { 0, 0, width, height };

    COLORREF OldTextColor = ::SetTextColor(*CUIFBitmapDC::s_phdcDst, rgbColor);
    COLORREF OldBkColor = ::SetBkColor(*CUIFBitmapDC::s_phdcDst, rgbBack);
    ::FillRect(*CUIFBitmapDC::s_phdcDst, &rc, hbr);
    ::SetTextColor(*CUIFBitmapDC::s_phdcDst, OldTextColor);
    ::SetBkColor(*CUIFBitmapDC::s_phdcDst, OldBkColor);

    ::BitBlt(*CUIFBitmapDC::s_phdcDst, 0, 0, width, height, *CUIFBitmapDC::s_phdcMask, 0, 0, SRCAND);
    ::BitBlt(*CUIFBitmapDC::s_phdcDst, 0, 0, width, height, *CUIFBitmapDC::s_phdcSrc, 0, 0, SRCINVERT);
    CUIFBitmapDC::s_phdcSrc->Uninit();
    CUIFBitmapDC::s_phdcMask->Uninit();
    CUIFBitmapDC::s_phdcDst->Uninit(TRUE);

    return CUIFBitmapDC::s_phdcDst->DetachBitmap();
}

BOOL cicGetIconBitmaps(HICON hIcon, HBITMAP *hbm1, HBITMAP *hbm2, const SIZE *pSize)
{
    if (!CUIFBitmapDC::s_fInitBitmapDCs)
        return FALSE;

    SIZE size;
    if (pSize)
    {
        size = *pSize;
    }
    else
    {
        if (!cicGetIconSize(hIcon, &size))
            return FALSE;
    }

    CUIFBitmapDC::s_phdcSrc->SetDIB(size.cx, size.cy, 1, 32);
    CUIFBitmapDC::s_phdcMask->SetBitmap(size.cx, size.cy, 1, 1);

    RECT rc = { 0, 0, size.cx, size.cy };
    ::FillRect(*CUIFBitmapDC::s_phdcSrc, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH));

    ::DrawIconEx(*CUIFBitmapDC::s_phdcSrc, 0, 0, hIcon, size.cx, size.cy, 0, 0, DI_NORMAL);
    ::DrawIconEx(*CUIFBitmapDC::s_phdcMask, 0, 0, hIcon, size.cx, size.cy, 0, 0, DI_MASK);

    CUIFBitmapDC::s_phdcSrc->Uninit(TRUE);
    CUIFBitmapDC::s_phdcMask->Uninit(TRUE);
    *hbm1 = CUIFBitmapDC::s_phdcSrc->DetachBitmap();
    *hbm2 = CUIFBitmapDC::s_phdcMask->DetachBitmap();
    return TRUE;
}

void cicDrawMaskBmpOnDC(HDC hDC, LPCRECT prc, HBITMAP hbmp, HBITMAP hbmpMask)
{
    if (!CUIFBitmapDC::s_fInitBitmapDCs)
        return;

    LONG cx = prc->right - prc->left, cy = prc->bottom - prc->top;
    CUIFBitmapDC::s_phdcDst->SetDIB(cx, cy, 1, 32);
    CUIFBitmapDC::s_phdcSrc->SetBitmap(hbmp);
    CUIFBitmapDC::s_phdcMask->SetBitmap(hbmpMask);
    ::BitBlt(*CUIFBitmapDC::s_phdcDst, 0, 0, cx, cy, hDC, prc->left, prc->top, SRCCOPY);
    ::BitBlt(*CUIFBitmapDC::s_phdcDst, 0, 0, cx, cy, *CUIFBitmapDC::s_phdcMask, 0, 0, SRCAND);
    ::BitBlt(*CUIFBitmapDC::s_phdcDst, 0, 0, cx, cy, *CUIFBitmapDC::s_phdcSrc, 0, 0, SRCINVERT);
    ::BitBlt(hDC, prc->left, prc->top, cx, cy, *CUIFBitmapDC::s_phdcDst, 0, 0, SRCCOPY);
    CUIFBitmapDC::s_phdcSrc->Uninit(FALSE);
    CUIFBitmapDC::s_phdcMask->Uninit(FALSE);
    CUIFBitmapDC::s_phdcDst->Uninit(FALSE);
}

/////////////////////////////////////////////////////////////////////////////
// CUIFWindow

CUIFWindow::CUIFWindow(HINSTANCE hInst, DWORD style)
    : CUIFObject(NULL, 0, NULL, style)
{
    m_hInst = hInst;
    m_nLeft = 200;
    m_nTop = 200;
    m_nWidth = 200;
    m_nHeight = 200;
    m_hWnd = 0;
    m_pWindow = this;
    m_pCaptured = NULL;
    m_pTimerObject = NULL;
    m_pPointed = NULL;
    m_bPointing = FALSE;
    m_pToolTip = NULL;
    m_pShadow = NULL;
    m_bShowShadow = TRUE;
    m_pBehindModal = NULL;
    CUIFWindow::CreateScheme();
}

CUIFWindow::~CUIFWindow()
{
    if (m_pToolTip)
    {
        delete m_pToolTip;
        m_pToolTip = NULL;
    }
    if (m_pShadow)
    {
        delete m_pShadow;
        m_pShadow = NULL;
    }
    for (size_t i = m_ObjectArray.size(); i > 0; )
    {
        --i;
        CUIFObject *pObject = m_ObjectArray[i];
        m_ObjectArray[i] = NULL;
        m_ObjectArray.Remove(pObject);
        delete pObject;
    }
    if (m_pScheme)
    {
        delete m_pScheme;
        m_pScheme = NULL;
    }
}

STDMETHODIMP_(BOOL)
CUIFWindow::Initialize()
{
    LPCTSTR pszClass = GetClassName();

    WNDCLASSEX wcx;
    ZeroMemory(&wcx, sizeof(wcx));
    wcx.cbSize = sizeof(WNDCLASSEXW);
    if (!::GetClassInfoEx(m_hInst, pszClass, &wcx))
    {
        ZeroMemory(&wcx, sizeof(wcx));
        wcx.cbSize = sizeof(wcx);
        wcx.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
        wcx.lpfnWndProc = CUIFWindow::WindowProcedure;
        wcx.cbClsExtra = 0;
        wcx.cbWndExtra = sizeof(LONG_PTR) * 2;
        wcx.hInstance = m_hInst;
        wcx.hIcon = NULL;
        wcx.hCursor = ::LoadCursor(NULL, IDC_ARROW);
        wcx.lpszClassName = pszClass;
        wcx.hbrBackground = NULL;
        wcx.lpszMenuName = NULL;
        wcx.hIconSm = NULL;
        ::RegisterClassEx(&wcx);
    }

    cicUpdateUIFSys();
    cicUpdateUIFScheme();

    if (m_style & UIF_WINDOW_TOOLTIP)
    {
        DWORD style = (m_style & UIF_WINDOW_LAYOUTRTL) | UIF_WINDOW_TOPMOST | 0x10;
        m_pToolTip = new(cicNoThrow) CUIFToolTip(m_hInst, style, this);
        if (m_pToolTip)
            m_pToolTip->Initialize();
    }

    if (m_style & UIF_WINDOW_SHADOW)
    {
        m_pShadow = new(cicNoThrow) CUIFShadow(m_hInst, UIF_WINDOW_TOPMOST, this);
        if (m_pShadow)
            m_pShadow->Initialize();
    }

    return CUIFObject::Initialize();
}

STDMETHODIMP_(LRESULT)
CUIFWindow::OnSettingChange(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void CUIFWindow::UpdateUI(LPCRECT prc)
{
    if (::IsWindow(m_hWnd))
        ::InvalidateRect(m_hWnd, prc, FALSE);
}

CUIFWindow*
CUIFWindow::GetThis(HWND hWnd)
{
    return (CUIFWindow*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
}

void
CUIFWindow::SetThis(HWND hWnd, LONG_PTR dwNewLong)
{
    ::SetWindowLongPtr(hWnd, GWLP_USERDATA, dwNewLong);
}

void CUIFWindow::CreateScheme()
{
    if (m_pScheme)
    {
        delete m_pScheme;
        m_pScheme = NULL;
    }

    INT iScheme = 0;
    if (m_style & UIF_WINDOW_USESCHEME1)
        iScheme = 1;
    else if (m_style & UIF_WINDOW_USESCHEME2)
        iScheme = 2;
    else if (m_style & UIF_WINDOW_USESCHEME3)
        iScheme = 3;

    m_pScheme = cicCreateUIFScheme(iScheme);
    SetScheme(m_pScheme);
}

STDMETHODIMP_(DWORD)
CUIFWindow::GetWndStyle()
{
    DWORD ret;

    if (m_style & UIF_WINDOW_CHILD)
        ret = WS_CHILD | WS_CLIPSIBLINGS;
    else
        ret = WS_POPUP | WS_DISABLED;

    if (m_style & UIF_WINDOW_USESCHEME1)
        ret |= WS_BORDER;
    else if (m_style & UIF_WINDOW_DLGFRAME)
        ret |= WS_DLGFRAME;
    else if ((m_style & UIF_WINDOW_USESCHEME2) || (m_style & 0x10))
        ret |= WS_BORDER;

    return ret;
}

STDMETHODIMP_(DWORD)
CUIFWindow::GetWndStyleEx()
{
    DWORD ret = 0;
    if (m_style & UIF_WINDOW_TOPMOST)
        ret |= WS_EX_TOPMOST;
    if (m_style & UIF_WINDOW_TOOLWINDOW)
        ret |= WS_EX_TOOLWINDOW;
    if (m_style & UIF_WINDOW_LAYOUTRTL)
        ret |= WS_EX_LAYOUTRTL;
    return ret;
}

STDMETHODIMP_(HWND)
CUIFWindow::CreateWnd(HWND hwndParent)
{
    HWND hWnd = CreateWindowEx(GetWndStyleEx(), GetClassName(), GetWndTitle(), GetWndStyle(),
                               m_nLeft, m_nTop, m_nWidth, m_nHeight,
                               hwndParent, NULL, m_hInst, this);
    if (m_pToolTip)
        m_pToolTip->CreateWnd(hWnd);
    if (m_pShadow)
        m_pShadow->CreateWnd(hWnd);
    return hWnd;
}

void CUIFWindow::Show(BOOL bVisible)
{
    if (!IsWindow(m_hWnd))
        return;

    if (bVisible && (m_style & UIF_WINDOW_TOPMOST))
        ::SetWindowPos(m_hWnd, NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);

    m_bVisible = bVisible;
    ::ShowWindow(m_hWnd, (bVisible ? SW_SHOWNOACTIVATE : 0));
}

STDMETHODIMP_(BOOL)
CUIFWindow::AnimateWnd(DWORD dwTime, DWORD dwFlags)
{
    if (!::IsWindow(m_hWnd))
        return FALSE;

    BOOL bVisible = !(dwFlags & 0x10000);
    OnAnimationStart();
    BOOL ret = ::AnimateWindow(m_hWnd, dwTime, dwFlags);
    if (!ret)
        m_bVisible = bVisible;
    OnAnimationEnd();
    return ret;
}

void CUIFWindow::SetCaptureObject(CUIFObject *pCaptured)
{
    if (pCaptured)
    {
        m_pCaptured = pCaptured;
        SetCapture(TRUE);
    }
    else
    {
        m_pCaptured = NULL;
        SetCapture(FALSE);
    }
}

STDMETHODIMP_(void)
CUIFWindow::SetCapture(BOOL bSet)
{
    if (bSet)
        ::SetCapture(m_hWnd);
    else
        ::ReleaseCapture();
}

void CUIFWindow::SetObjectPointed(CUIFObject *pPointed, POINT pt)
{
    if (pPointed == m_pPointed)
        return;

    if (m_pCaptured)
    {
        if (m_pCaptured == m_pPointed && m_pPointed->m_bEnable)
            m_pPointed->OnMouseOut(pt.x, pt.y);
    }
    else if (m_pPointed && m_pPointed->m_bEnable)
    {
        m_pPointed->OnMouseOut(pt.x, pt.y);
    }

    m_pPointed = pPointed;

    if (m_pCaptured)
    {
        if (m_pCaptured == m_pPointed && m_pPointed->m_bEnable)
            m_pPointed->OnMouseIn(pt.x, pt.y);
    }
    else if (m_pPointed && m_pPointed->m_bEnable)
    {
        m_pPointed->OnMouseIn(pt.x, pt.y);
    }
}

STDMETHODIMP_(void)
CUIFWindow::OnObjectMoved(CUIFObject *pObject)
{
    if (!::IsWindow(m_hWnd))
        return;

    POINT pt;
    ::GetCursorPos(&pt);
    ::ScreenToClient(m_hWnd, &pt);
    POINT pt2 = pt;
    CUIFObject *pFound = ObjectFromPoint(pt);
    SetObjectPointed(pFound, pt2);
}

STDMETHODIMP_(void)
CUIFWindow::SetRect(LPCRECT prc)
{
    RECT Rect = { 0, 0, 0, 0 };

    if (::IsWindow(m_hWnd))
        ::GetClientRect(m_hWnd, &Rect);

    CUIFObject::SetRect(&Rect);
}

STDMETHODIMP_(void)
CUIFWindow::ClientRectToWindowRect(LPRECT lpRect)
{
    DWORD style, exstyle;
    if (::IsWindow(m_hWnd))
    {
        style = ::GetWindowLongPtr(m_hWnd, GWL_STYLE);
        exstyle = ::GetWindowLongPtr(m_hWnd, GWL_EXSTYLE);
    }
    else
    {
        style = GetWndStyle();
        exstyle = GetWndStyleEx();
    }
    ::AdjustWindowRectEx(lpRect, style, FALSE, exstyle);
}

STDMETHODIMP_(void)
CUIFWindow::GetWindowFrameSize(LPSIZE pSize)
{
    RECT rc = { 0, 0, 0, 0 };

    ClientRectToWindowRect(&rc);
    pSize->cx = (rc.right - rc.left) / 2;
    pSize->cy = (rc.bottom - rc.top) / 2;
}

STDMETHODIMP_(void)
CUIFWindow::OnAnimationEnd()
{
    if (m_pShadow && m_bShowShadow)
        m_pShadow->Show(m_bVisible);
}

STDMETHODIMP_(void)
CUIFWindow::OnThemeChanged(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    ClearTheme();
}

LRESULT
CUIFWindow::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_GETOBJECT:
            return OnGetObject(hWnd, WM_GETOBJECT, wParam, lParam);

        case WM_SYSCOLORCHANGE:
            cicUpdateUIFScheme();
            OnSysColorChange();
            return 0;

        case WM_ENDSESSION:
            OnEndSession(hWnd, wParam, lParam);
            return 0;

        case WM_SHOWWINDOW:
            if (m_pShadow && m_bShowShadow)
                m_pShadow->Show(wParam);
            return OnShowWindow(hWnd, WM_SHOWWINDOW, wParam, lParam);

        case WM_SETTINGCHANGE:
            cicUpdateUIFSys();
            cicUpdateUIFScheme();
            return OnSettingChange(hWnd, WM_SETTINGCHANGE, wParam, lParam);
        case WM_SETCURSOR:
        {
            POINT Point;
            ::GetCursorPos(&Point);
            ::ScreenToClient(m_hWnd, &Point);

            if (m_pBehindModal)
            {
                m_pBehindModal->ModalMouseNotify(HIWORD(lParam), Point.x, Point.y);
                return TRUE;
            }

            if (!m_bPointing)
            {
                ::SetTimer(m_hWnd, POINTING_TIMER_ID, 1000, NULL);
                m_bPointing = TRUE;
            }

            if (m_pToolTip)
            {
                MSG msg = { m_hWnd, HIWORD(lParam), 0, MAKELPARAM(Point.x, Point.y) };
                m_pToolTip->RelayEvent(&msg);
            }

            if (!(m_style & UIF_WINDOW_NOMOUSEMSG))
                HandleMouseMsg(HIWORD(lParam), Point.x, Point.y);

            return TRUE;
        }
        case WM_MOUSEACTIVATE:
            return MA_NOACTIVATE;
        case WM_ERASEBKGND:
            return OnEraseBkGnd(hWnd, WM_ERASEBKGND, wParam, lParam);
        case WM_CREATE:
            SetRect(NULL);
            OnCreate(hWnd);
            return 0;
        case WM_DESTROY:
            if (m_pToolTip && ::IsWindow(*m_pToolTip))
                ::DestroyWindow(*m_pToolTip);
            if (m_pShadow && ::IsWindow(*m_pShadow))
                ::DestroyWindow(*m_pShadow);
            OnDestroy(hWnd);
            return 0;
        case WM_SIZE:
            SetRect(NULL);
            return 0;
        case WM_ACTIVATE:
            return OnActivate(hWnd, WM_ACTIVATE, wParam, lParam);
        case WM_SETFOCUS:
            OnSetFocus(hWnd);
            return 0;
        case WM_KILLFOCUS:
            OnKillFocus(hWnd);
            return 0;
        case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            HDC hDC = ::BeginPaint(hWnd, &Paint);
            PaintObject(hDC, &Paint.rcPaint);
            ::EndPaint(hWnd, &Paint);
            return 0;
        }
        case WM_PRINTCLIENT:
        {
            PaintObject((HDC)wParam, NULL);
            return 0;
        }
        case WM_THEMECHANGED:
        {
            OnThemeChanged(hWnd, wParam, lParam);
            return 0;
        }
        case WM_COMMAND:
        {
            return 0;
        }
        case WM_TIMER:
        {
            switch (wParam)
            {
                case USER_TIMER_ID:
                {
                    if (m_pTimerObject)
                        m_pTimerObject->OnTimer();
                    break;
                }
                case POINTING_TIMER_ID:
                {
                    POINT pt;
                    ::GetCursorPos(&pt);

                    POINT pt2 = pt;
                    ::ScreenToClient(m_hWnd, &pt2);

                    RECT rc;
                    ::GetWindowRect(m_hWnd, &rc);

                    if (::PtInRect(&rc, pt) && ::WindowFromPoint(pt) == m_hWnd)
                    {
                        m_pBehindModal->ModalMouseNotify(WM_MOUSEMOVE, pt2.x, pt2.y);
                    }
                    else
                    {
                        ::KillTimer(m_hWnd, POINTING_TIMER_ID);
                        m_bPointing = FALSE;
                        SetObjectPointed(NULL, pt2);
                        OnMouseOutFromWindow(pt2.x, pt2.y);
                    }

                    if (m_pToolTip)
                    {
                        MSG msg = { m_hWnd, WM_MOUSEMOVE, 0, MAKELPARAM(pt2.x, pt2.y) };
                        m_pToolTip->RelayEvent(&msg);
                    }
                    break;
                }
                default:
                {
                    OnTimer(wParam);
                    break;
                }
            }
            break;
        }
        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
        case WM_LBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONDBLCLK:
        case WM_MBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONDBLCLK:
        case WM_RBUTTONUP:
        {
            if (m_pBehindModal)
                m_pBehindModal->ModalMouseNotify(uMsg, (SHORT)LOWORD(lParam), (SHORT)HIWORD(lParam));
            else
                HandleMouseMsg(uMsg, (SHORT)LOWORD(lParam), (SHORT)HIWORD(lParam));
            break;
        }
        case WM_KEYUP:
        {
            OnKeyUp(hWnd, wParam, lParam);
            break;
        }
        case WM_WINDOWPOSCHANGING:
        {
            WINDOWPOS *pwp = (WINDOWPOS *)lParam;
            if (m_pShadow && (pwp->flags & SWP_HIDEWINDOW))
                m_pShadow->Show(FALSE);
            if (!(pwp->flags & SWP_NOZORDER) && pwp->hwndInsertAfter == m_pShadow->m_hWnd)
                pwp->flags |= SWP_NOZORDER;
            m_pShadow->OnOwnerWndMoved(!(pwp->flags & SWP_NOSIZE));
            return OnWindowPosChanging(hWnd, WM_WINDOWPOSCHANGING, wParam, lParam);
        }
        case WM_WINDOWPOSCHANGED:
        {
            WINDOWPOS *pwp = (WINDOWPOS *)lParam;
            if (m_pShadow)
                m_pShadow->OnOwnerWndMoved(!(pwp->flags & SWP_NOSIZE));
            return OnWindowPosChanged(hWnd, WM_WINDOWPOSCHANGED, wParam, lParam);
        }
        case WM_NOTIFY:
            OnNotify(hWnd, wParam, lParam);
            return 0;
        case WM_NOTIFYFORMAT:
            return OnNotifyFormat(hWnd, wParam, lParam);
        case WM_DISPLAYCHANGE:
            cicUpdateUIFSys();
            cicUpdateUIFScheme();
            return OnDisplayChange(hWnd, WM_DISPLAYCHANGE, wParam, lParam);
        case WM_NCDESTROY:
            OnNCDestroy(hWnd);
            return 0;
        case WM_KEYDOWN:
            OnKeyDown(hWnd, wParam, lParam);
            return 0;
        default:
        {
            if (uMsg >= WM_USER)
            {
                CUIFWindow *pThis = CUIFWindow::GetThis(hWnd);
                pThis->OnUser(hWnd, uMsg, wParam, lParam);
                break;
            }
            return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
        }
    }

    return 0;
}

LRESULT CALLBACK
CUIFWindow::WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CUIFWindow *This;

    if (uMsg == WM_NCCREATE)
    {
        This = (CUIFWindow*)((CREATESTRUCT*)lParam)->lpCreateParams;
        CUIFWindow::SetThis(hWnd, (LONG_PTR)This);
        This->m_hWnd = hWnd;
    }
    else
    {
        This = CUIFWindow::GetThis(hWnd);
    }

    if (uMsg == WM_GETMINMAXINFO)
    {
        if (This)
            return This->WindowProc(hWnd, uMsg, wParam, lParam);
        else
            return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    if (!This)
        return 0;

    if (uMsg == WM_NCDESTROY)
    {
        This->m_hWnd = NULL;
        ::SetWindowLongPtr(hWnd, GWLP_USERDATA, 0);
    }

    return This->WindowProc(hWnd, uMsg, wParam, lParam);
}

BOOL
CUIFWindow::GetWorkArea(LPCRECT prcWnd, LPRECT prcWorkArea)
{
    if (!(m_style & (UIF_WINDOW_WORKAREA | UIF_WINDOW_MONITOR)))
        return FALSE;

    HMONITOR hMon = ::MonitorFromRect(prcWnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi;
    mi.cbSize = sizeof(MONITORINFO);
    if (!hMon || !::GetMonitorInfo(hMon, &mi))
    {
        if (m_style & UIF_WINDOW_WORKAREA)
            return ::SystemParametersInfo(SPI_GETWORKAREA, 0, prcWorkArea, 0);

        prcWorkArea->left = prcWorkArea->top = 0;
        prcWorkArea->right = ::GetSystemMetrics(SM_CXSCREEN);
        prcWorkArea->bottom = ::GetSystemMetrics(SM_CYSCREEN);
        return TRUE;
    }

    if (m_style & UIF_WINDOW_WORKAREA)
    {
        *prcWorkArea = mi.rcWork;
        return TRUE;
    }

    *prcWorkArea = mi.rcMonitor;
    return TRUE;
}

void
CUIFWindow::AdjustWindowPosition()
{
    RECT rc;
    rc.left = m_nLeft;
    rc.right = m_nLeft + m_nWidth;
    rc.top = m_nTop;
    rc.bottom = m_nTop + m_nHeight;

    RECT rcWorkArea;
    if (!GetWorkArea(&rc, &rcWorkArea))
        return;

    if (m_nLeft < rcWorkArea.left)
        m_nLeft = rcWorkArea.left;
    if (m_nTop < rcWorkArea.top)
        m_nTop = rcWorkArea.top;
    if (m_nLeft + m_nWidth >= rcWorkArea.right)
        m_nLeft = rcWorkArea.right - m_nWidth;
    if (m_nTop + m_nHeight >= rcWorkArea.bottom)
        m_nTop = rcWorkArea.bottom - m_nHeight;
}

void CUIFWindow::SetBehindModal(CUIFWindow *pBehindModal)
{
    m_pBehindModal = pBehindModal;
}

void CUIFWindow::SetTimerObject(CUIFObject *pTimerObject, UINT uElapse)
{
    if (pTimerObject)
    {
        m_pTimerObject = pTimerObject;
        ::SetTimer(m_hWnd, USER_TIMER_ID, uElapse, NULL);
    }
    else
    {
        m_pTimerObject = NULL;
        ::KillTimer(m_hWnd, USER_TIMER_ID);
    }
}

STDMETHODIMP_(void)
CUIFWindow::PaintObject(HDC hDC, LPCRECT prc)
{
    BOOL bGotDC = FALSE;
    if (!hDC)
    {
        hDC = ::GetDC(m_hWnd);
        bGotDC = TRUE;
    }

    if (!prc)
        prc = &m_rc;

    HDC hMemDC = ::CreateCompatibleDC(hDC);
    HBITMAP hbmMem = ::CreateCompatibleBitmap(hDC, prc->right - prc->left, prc->bottom - prc->top);

    HGDIOBJ hbmOld = ::SelectObject(hMemDC, hbmMem);
    ::SetViewportOrgEx(hMemDC, -prc->left, -prc->top, NULL);
    if (FAILED(EnsureThemeData(m_hWnd)) ||
        ((!(m_style & UIF_WINDOW_CHILD) || FAILED(DrawThemeParentBackground(m_hWnd, hMemDC, &m_rc))) &&
         FAILED(DrawThemeBackground(hMemDC, m_iStateId, &m_rc, NULL))))
    {
        if (m_pScheme)
            m_pScheme->FillRect(hMemDC, prc, 22);
    }

    CUIFObject::PaintObject(hMemDC, prc);
    ::BitBlt(hDC, prc->left, prc->top,
                  prc->right - prc->left, prc->bottom - prc->top,
             hMemDC, prc->left, prc->top, SRCCOPY);
    ::SelectObject(hMemDC, hbmOld);
    ::DeleteObject(hbmMem);
    ::DeleteDC(hMemDC);

    if (bGotDC)
        ::ReleaseDC(m_hWnd, hDC);
}

STDMETHODIMP_(void)
CUIFWindow::Move(INT x, INT y, INT nWidth, INT nHeight)
{
    m_nLeft = x;
    m_nTop = y;
    if (nWidth >= 0)
        m_nWidth = nWidth;
    if (nHeight >= 0)
        m_nHeight = nHeight;
    if (::IsWindow(m_hWnd))
    {
        AdjustWindowPosition();
        ::MoveWindow(m_hWnd, m_nLeft, m_nTop, m_nWidth, m_nHeight, TRUE);
    }
}

STDMETHODIMP_(void)
CUIFWindow::RemoveUIObj(CUIFObject *pRemove)
{
    if (pRemove == m_pCaptured)
        SetCaptureObject(NULL);

    if (pRemove == m_pTimerObject)
    {
        m_pTimerObject = NULL;
        ::KillTimer(m_hWnd, USER_TIMER_ID);
    }

    if (pRemove == m_pPointed)
        m_pPointed = NULL;

    CUIFObject::RemoveUIObj(pRemove);
}

STDMETHODIMP_(void)
CUIFWindow::HandleMouseMsg(UINT uMsg, LONG x, LONG y)
{
    POINT pt = { x, y };

    CUIFObject *pFound = (CUIFWindow *)ObjectFromPoint(pt);

    SetObjectPointed(pFound, pt);

    if (m_pCaptured)
        pFound = m_pCaptured;

    if (!pFound || OnSetCursor(uMsg, pt.x, pt.y))
    {
        HCURSOR hCursor = ::LoadCursor(NULL, IDC_ARROW);
        ::SetCursor(hCursor);
    }

    if (pFound && pFound->m_bEnable)
    {
        switch (uMsg)
        {
            case WM_MOUSEMOVE:
                pFound->OnMouseMove(pt.x, pt.y);
                break;
            case WM_LBUTTONDOWN:
                pFound->OnLButtonDown(pt.x, pt.y);
                break;
            case WM_LBUTTONUP:
                pFound->OnLButtonUp(pt.x, pt.y);
                break;
            case WM_RBUTTONDOWN:
                pFound->OnRButtonDown(pt.x, pt.y);
                break;
            case WM_RBUTTONUP:
                pFound->OnRButtonUp(pt.x, pt.y);
                break;
            case WM_MBUTTONDOWN:
                pFound->OnMButtonDown(pt.x, pt.y);
                break;
            case WM_MBUTTONUP:
                pFound->OnMButtonUp(pt.x, pt.y);
                break;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
// CUIFShadow

/// @unimplemented
CUIFShadow::CUIFShadow(HINSTANCE hInst, DWORD style, CUIFWindow *pShadowOwner)
    : CUIFWindow(hInst, (style | UIF_WINDOW_TOOLWINDOW))
{
    m_pShadowOwner = pShadowOwner;
    m_rgbShadowColor = RGB(0, 0, 0);
    m_dwUnknown11[0] = 0;
    m_dwUnknown11[1] = 0;
    m_xShadowDelta = m_yShadowDelta = 0;
    m_bLayerAvailable = FALSE;
}

CUIFShadow::~CUIFShadow()
{
    if (m_pShadowOwner)
        m_pShadowOwner->m_pShadow = NULL;
}

/// @unimplemented
void CUIFShadow::InitSettings()
{
    m_bLayerAvailable = FALSE;
    m_rgbShadowColor = RGB(128, 128, 128);
    m_xShadowDelta = m_yShadowDelta = 2;
}

/// @unimplemented
void CUIFShadow::InitShadow()
{
    if (m_bLayerAvailable)
    {
        //FIXME
    }
}

void CUIFShadow::AdjustWindowPos()
{
    HWND hwndOwner = *m_pShadowOwner;
    if (!::IsWindow(m_hWnd))
        return;

    RECT rc;
    ::GetWindowRect(hwndOwner, &rc);
    ::SetWindowPos(m_hWnd, hwndOwner,
                   rc.left + m_xShadowDelta,
                   rc.top + m_yShadowDelta,
                   rc.right - rc.left,
                   rc.bottom - rc.top,
                   SWP_NOOWNERZORDER | SWP_NOACTIVATE);
}

void CUIFShadow::OnOwnerWndMoved(BOOL bDoSize)
{
    if (::IsWindow(m_hWnd) && ::IsWindowVisible(m_hWnd))
    {
        AdjustWindowPos();
        if (bDoSize)
            InitShadow();
    }
}

STDMETHODIMP_(BOOL)
CUIFShadow::Initialize()
{
    InitSettings();
    return CUIFWindow::Initialize();
}

STDMETHODIMP_(DWORD)
CUIFShadow::GetWndStyleEx()
{
    DWORD exstyle = CUIFWindow::GetWndStyleEx();
    if (m_bLayerAvailable)
        exstyle |= WS_EX_LAYERED;
    return exstyle;
}

STDMETHODIMP_(void)
CUIFShadow::OnPaint(HDC hDC)
{
    RECT rc = m_rc;
    HBRUSH hBrush = ::CreateSolidBrush(m_rgbShadowColor);
    ::FillRect(hDC, &rc, hBrush);
    ::DeleteObject(hBrush);
}

STDMETHODIMP_(LRESULT)
CUIFShadow::OnWindowPosChanging(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    WINDOWPOS *wp = (WINDOWPOS *)lParam;
    wp->hwndInsertAfter = *m_pShadowOwner;
    return ::DefWindowProc(hWnd, Msg, wParam, lParam);
}

STDMETHODIMP_(LRESULT)
CUIFShadow::OnSettingChange(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    InitSettings();

    DWORD exstyle;
    if (m_bLayerAvailable)
        exstyle = ::GetWindowLongPtr(m_hWnd, GWL_EXSTYLE) | WS_EX_LAYERED;
    else
        exstyle = ::GetWindowLongPtr(m_hWnd, GWL_EXSTYLE) & ~WS_EX_LAYERED;

    ::SetWindowLongPtr(m_hWnd, GWL_EXSTYLE, exstyle);

    AdjustWindowPos();
    InitShadow();

    return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
}

STDMETHODIMP_(void)
CUIFShadow::Show(BOOL bVisible)
{
    if (bVisible && ::IsWindow(m_hWnd) && !::IsWindowVisible(m_hWnd))
    {
        AdjustWindowPos();
        InitShadow();
    }

    if (::IsWindow(m_hWnd))
    {
        m_bVisible = bVisible;
        ::ShowWindow(m_hWnd, (bVisible ? SW_SHOWNOACTIVATE : SW_HIDE));
    }
}

/////////////////////////////////////////////////////////////////////////////
// CUIFToolTip

CUIFToolTip::CUIFToolTip(HINSTANCE hInst, DWORD style, CUIFWindow *pToolTipOwner)
    : CUIFWindow(hInst, style)
{
    m_pToolTipOwner = pToolTipOwner;
    m_rcToolTipMargin.left = 2;
    m_rcToolTipMargin.top = 2;
    m_rcToolTipMargin.right = 2;
    m_rcToolTipMargin.bottom = 2;
    m_pToolTipTarget = NULL;
    m_pszToolTipText = NULL;
    m_dwUnknown10 = 0; //FIXME: name and type
    m_nDelayTimeType2 = -1;
    m_nDelayTimeType3 = -1;
    m_nDelayTimeType1 = -1;
    m_cxToolTipWidth = -1;
    m_bToolTipHasBkColor = 0;
    m_bToolTipHasTextColor = 0;
    m_rgbToolTipBkColor = 0;
    m_rgbToolTipTextColor = 0;
}

CUIFToolTip::~CUIFToolTip()
{
    if (m_pToolTipOwner)
        m_pToolTipOwner->m_pToolTip = NULL;
    if (m_pszToolTipText)
        delete[] m_pszToolTipText;
}

LONG
CUIFToolTip::GetDelayTime(UINT uType)
{
    LONG nDelayTime;
    switch (uType)
    {
        case 1:
        {
            nDelayTime = m_nDelayTimeType1;
            if (nDelayTime == -1)
                return ::GetDoubleClickTime() / 5;
            return nDelayTime;
        }
        case 2:
        {
            nDelayTime = m_nDelayTimeType2;
            if (nDelayTime == -1)
                return 10 * ::GetDoubleClickTime();
            return nDelayTime;
        }
        case 3:
        {
            nDelayTime = m_nDelayTimeType3;
            if (nDelayTime == -1)
                return ::GetDoubleClickTime();
            return nDelayTime;
        }
        default:
        {
            return 0;
        }
    }
}

void CUIFToolTip::GetMargin(LPRECT prc)
{
    if (prc)
        *prc = m_rcToolTipMargin;
}

COLORREF
CUIFToolTip::GetTipBkColor()
{
    if (m_bToolTipHasBkColor)
        return m_rgbToolTipBkColor;
    return ::GetSysColor(COLOR_INFOBK);
}

COLORREF
CUIFToolTip::GetTipTextColor()
{
    if (m_bToolTipHasTextColor)
        return m_rgbToolTipTextColor;
    return ::GetSysColor(COLOR_INFOTEXT);
}

CUIFObject*
CUIFToolTip::FindObject(HWND hWnd, POINT pt)
{
    if (hWnd == *m_pToolTipOwner)
        return m_pToolTipOwner->ObjectFromPoint(pt);
    return NULL;
}

void
CUIFToolTip::ShowTip()
{
    ::KillTimer(m_hWnd, TOOLTIP_TIMER_ID);

    if (!m_pToolTipTarget)
        return;

    LPCWSTR pszText = m_pToolTipTarget->GetToolTip();
    if (!pszText)
        return;

    if (!m_pToolTipTarget || m_pToolTipTarget->OnShowToolTip())
        return;

    POINT Point;
    ::GetCursorPos(&Point);
    ::ScreenToClient(*m_pToolTipTarget->m_pWindow, &Point);

    RECT rc;
    m_pToolTipTarget->GetRect(&rc);
    if (!::PtInRect(&rc, Point))
        return;

    size_t cchText = wcslen(pszText);
    m_pszToolTipText = new(cicNoThrow) WCHAR[cchText + 1];
    if (!m_pszToolTipText)
        return;

    lstrcpynW(m_pszToolTipText, pszText, cchText + 1);

    SIZE size;
    GetTipWindowSize(&size);

    RECT rc2 = rc;
    ::ClientToScreen(*m_pToolTipTarget->m_pWindow, (LPPOINT)&rc);
    ::ClientToScreen(*m_pToolTipTarget->m_pWindow, (LPPOINT)&rc.right);
    GetTipWindowRect(&rc2, size, &rc);

    m_bShowToolTip = TRUE;
    Move(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
    Show(TRUE);
}

void
CUIFToolTip::HideTip()
{
    ::KillTimer(m_hWnd, TOOLTIP_TIMER_ID);
    m_bShowToolTip = FALSE;

    if (m_pToolTipTarget)
        m_pToolTipTarget->OnHideToolTip();

    if (m_bVisible)
    {
        if (m_pszToolTipText)
        {
            delete[] m_pszToolTipText;
            m_pszToolTipText = NULL;
        }
        Show(FALSE);
    }
}

void
CUIFToolTip::GetTipWindowSize(LPSIZE pSize)
{
    if (!m_pszToolTipText)
        return;

    HDC hDC = ::GetDC(m_hWnd);
    HGDIOBJ hFontOld = ::SelectObject(hDC, m_hFont);

    RECT rcText = { 0, 0, 0, 0 };
    INT cyText;
    if (m_cxToolTipWidth <= 0)
    {
        cyText = ::DrawTextW(hDC, m_pszToolTipText, -1, &rcText, DT_CALCRECT | DT_SINGLELINE);
    }
    else
    {
        rcText.right = m_cxToolTipWidth;
        cyText = ::DrawTextW(hDC, m_pszToolTipText, -1, &rcText, DT_CALCRECT | DT_WORDBREAK);
    }

    RECT rcMargin;
    GetMargin(&rcMargin);

    RECT rc;
    rc.left     = rcText.left - rcMargin.left;
    rc.top      = rcText.top - rcMargin.top;
    rc.right    = rcText.right + rcMargin.right;
    rc.bottom   = rcText.top + cyText + rcMargin.bottom;
    ClientRectToWindowRect(&rc);

    pSize->cx = rc.right - rc.left;
    pSize->cy = rc.bottom - rc.top;

    ::SelectObject(hDC, hFontOld);
    ::ReleaseDC(m_hWnd, hDC);
}

void
CUIFToolTip::GetTipWindowRect(LPRECT pRect, SIZE toolTipSize, LPCRECT prc)
{
    POINT Point;
    GetCursorPos(&Point);

    HCURSOR hCursor = ::GetCursor();
    ICONINFO IconInfo;
    INT yHotspot = 0;
    INT cyCursor = ::GetSystemMetrics(SM_CYCURSOR);
    if (hCursor && ::GetIconInfo(hCursor, &IconInfo))
    {
        BITMAP bm;
        ::GetObject(IconInfo.hbmMask, sizeof(bm), &bm);
        if (!IconInfo.fIcon)
        {
            cyCursor = bm.bmHeight;
            yHotspot = IconInfo.yHotspot;
            if (!IconInfo.hbmColor)
                cyCursor = bm.bmHeight / 2;
        }
        if (IconInfo.hbmColor)
            ::DeleteObject(IconInfo.hbmColor);
        if (IconInfo.hbmMask)
            ::DeleteObject(IconInfo.hbmMask);
    }

    RECT rcMonitor;
    rcMonitor.left = 0;
    rcMonitor.top = 0;
    rcMonitor.right = GetSystemMetrics(SM_CXSCREEN);
    rcMonitor.bottom = GetSystemMetrics(SM_CYSCREEN);

    HMONITOR hMon = ::MonitorFromPoint(Point, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi;
    if (hMon)
    {
        mi.cbSize = sizeof(MONITORINFO);
        if (::GetMonitorInfo(hMon, &mi))
            rcMonitor = mi.rcMonitor;
    }

    pRect->left   = Point.x;
    pRect->right  = pRect->left + toolTipSize.cx;
    pRect->top    = Point.y + cyCursor - yHotspot;
    pRect->bottom = pRect->top + toolTipSize.cy;

    if (rcMonitor.right < pRect->right)
    {
        pRect->left = rcMonitor.right - toolTipSize.cx;
        pRect->right = rcMonitor.right;
    }
    if (pRect->left < rcMonitor.left)
    {
        pRect->left = rcMonitor.left;
        pRect->right = rcMonitor.left + toolTipSize.cx;
    }
    if (rcMonitor.bottom < pRect->bottom)
    {
        pRect->top = rcMonitor.bottom - toolTipSize.cy;
        pRect->bottom = rcMonitor.bottom;
    }
    if (pRect->top < rcMonitor.top)
    {
        pRect->top = rcMonitor.top;
        pRect->bottom = rcMonitor.top + toolTipSize.cy;
    }
}

void
CUIFToolTip::RelayEvent(LPMSG pMsg)
{
    if (!pMsg)
        return;

    switch (pMsg->message)
    {
        case WM_MOUSEMOVE:
        {
            if (m_bEnable &&
                ::GetKeyState(VK_LBUTTON) >= 0 &&
                ::GetKeyState(VK_MBUTTON) >= 0 &&
                ::GetKeyState(VK_RBUTTON) >= 0)
            {
                POINT pt = { (SHORT)LOWORD(pMsg->lParam), (SHORT)HIWORD(pMsg->lParam) };
                CUIFObject *pFound = CUIFToolTip::FindObject(pMsg->hwnd, pt);
                if (pFound)
                {
                    if (m_pToolTipTarget != pFound)
                    {
                        HideTip();

                        LONG DelayTime;
                        if (!m_bVisible)
                            DelayTime = GetDelayTime(3);
                        else
                            DelayTime = GetDelayTime(1);
                        ::SetTimer(m_hWnd, TOOLTIP_TIMER_ID, DelayTime, NULL);
                    }
                }
                else
                {
                    HideTip();
                }
                m_pToolTipTarget = pFound;
            }
            break;
        }
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
        {
            HideTip();
            break;
        }
    }
}

STDMETHODIMP_(void) CUIFToolTip::OnPaint(HDC hDC)
{
    HGDIOBJ hFontOld = ::SelectObject(hDC, m_hFont);
    INT iBkModeOld = ::SetBkMode(hDC, TRANSPARENT);

    COLORREF rgbTextColor = GetTipTextColor();
    COLORREF rgbOldTextColor = ::SetTextColor(hDC, rgbTextColor);

    COLORREF rgbBkColor = GetTipBkColor();
    HBRUSH hbrBack = ::CreateSolidBrush(rgbBkColor);
    RECT rc = m_rc;
    if (hbrBack)
    {
        ::FillRect(hDC, &rc, hbrBack);
        ::DeleteObject(hbrBack);
    }

    RECT rcMargin;
    GetMargin(&rcMargin);

    rc.left += rcMargin.left;
    rc.top += rcMargin.top;
    rc.right -= rcMargin.right;
    rc.bottom -= rcMargin.bottom;

    if (m_cxToolTipWidth <= 0)
        ::DrawTextW(hDC, m_pszToolTipText, -1, &rc, DT_SINGLELINE);
    else
        ::DrawTextW(hDC, m_pszToolTipText, -1, &rc, DT_WORDBREAK);

    ::SetTextColor(hDC, rgbOldTextColor);
    ::SetBkMode(hDC, iBkModeOld);
    ::SelectObject(hDC, hFontOld);
}

STDMETHODIMP_(void) CUIFToolTip::Enable(BOOL bEnable)
{
    if (!bEnable)
        HideTip();
    CUIFObject::Enable(bEnable);
}

STDMETHODIMP_(void) CUIFToolTip::OnTimer(WPARAM wParam)
{
    if (wParam == TOOLTIP_TIMER_ID)
        ShowTip();
}

/////////////////////////////////////////////////////////////////////////////
// CUIFButton

CUIFButton::CUIFButton(
    CUIFObject *pParent,
    DWORD nObjectID,
    LPCRECT prc,
    DWORD style) : CUIFObject(pParent, nObjectID, prc, style)
{
    m_ButtonIcon.m_hIcon = NULL;
    m_ButtonIcon.m_hImageList = NULL;
    m_dwUnknown9 = 0;
    m_uButtonStatus = 0;
    m_bPressed = FALSE;
    m_hbmButton1 = NULL;
    m_hbmButton2 = NULL;
    m_pszButtonText = NULL;
}

CUIFButton::~CUIFButton()
{
    if (m_pszButtonText)
    {
        delete[] m_pszButtonText;
        m_pszButtonText = NULL;
    }

    if (m_ButtonIcon.m_hImageList)
        ImageList_Destroy(m_ButtonIcon.m_hImageList);
}

void
CUIFButton::DrawBitmapProc(HDC hDC, LPCRECT prc, BOOL bPressed)
{
    INT width = m_rc.right - m_rc.left;
    INT height = m_rc.bottom - m_rc.top;
    if (m_hbmButton2)
    {
        HBITMAP hbmMask = cicCreateMaskBmp(&m_rc, m_hbmButton1, m_hbmButton2,
                                           (HBRUSH)UlongToHandle(COLOR_BTNFACE + 1), 0, 0);
        ::DrawState(hDC, NULL, NULL, (LPARAM)hbmMask, 0,
                    prc->left + bPressed, prc->top + bPressed,
                    width - bPressed, height - bPressed,
                    DST_BITMAP | (m_bEnable ? 0 : (DSS_MONO | DSS_DISABLED)));
        ::DeleteObject(hbmMask);
    }
    else
    {
        ::DrawState(hDC, NULL, NULL, (LPARAM)m_hbmButton1, 0,
                    prc->left + bPressed, prc->top + bPressed,
                    width - bPressed, height - bPressed,
                    DST_BITMAP | (m_bEnable ? 0 : (DSS_MONO | DSS_DISABLED)));
    }
}

void
CUIFButton::DrawEdgeProc(HDC hDC, LPCRECT prc, BOOL bPressed)
{
    RECT rc = *prc;
    if (bPressed)
        ::DrawEdge(hDC, &rc, BDR_SUNKENOUTER, BF_RECT);
    else
        ::DrawEdge(hDC, &rc, BDR_RAISEDINNER, BF_RECT);
}

void CUIFButton::DrawIconProc(HDC hDC, LPRECT prc, BOOL bPressed)
{
    INT width = prc->right - prc->left;
    INT height = prc->bottom - prc->top;
    RECT rc = { 0, 0, width, height };

    HDC hMemDC = ::CreateCompatibleDC(hDC);
    if (!hMemDC)
        return;

    HBITMAP hbmMem = ::CreateCompatibleBitmap(hDC, width, height);
    if (!hbmMem)
    {
        ::DeleteDC(hMemDC);
        return;
    }

    HGDIOBJ hbmOld = ::SelectObject(hMemDC, hbmMem);
    if (m_bEnable)
    {
        ::BitBlt(hMemDC, rc.left, rc.top, width, height, hDC, prc->left, prc->top, SRCCOPY);
    }
    else
    {
        HBRUSH hbrWhite = (HBRUSH)::GetStockObject(WHITE_BRUSH);
        ::FillRect(hMemDC, &rc, hbrWhite);
    }

    if (m_style & UIF_BUTTON_LARGE_ICON)
    {
        ::DrawIconEx(hMemDC,
                     2 + bPressed, 2 + bPressed,
                     m_ButtonIcon.m_hIcon,
                     width - 4, height - 4,
                     0, NULL, DI_NORMAL);
    }
    else
    {
        ::DrawIconEx(hMemDC,
                     (width - 16) / 2 + bPressed,
                     (height - 16) / 2 + bPressed,
                     m_ButtonIcon.m_hIcon,
                     16, 16,
                     0, NULL, DI_NORMAL);
    }

    ::SelectObject(hMemDC, hbmOld);
    ::DrawState(hDC, NULL, NULL, (LPARAM)hbmMem, 0,
                prc->left, prc->top, width, height,
                DST_BITMAP | (m_bEnable ? 0 : (DSS_MONO | DSS_DISABLED)));
    ::DeleteObject(hbmMem);
    ::DeleteDC(hMemDC);
}

void
CUIFButton::DrawTextProc(HDC hDC, LPCRECT prc, BOOL bPressed)
{
    if (!m_pszButtonText)
        return;

    HGDIOBJ hFontOld = ::SelectObject(hDC, m_hFont);
    INT cchText = lstrlenW(m_pszButtonText);
    SIZE textSize;
    ::GetTextExtentPoint32W(hDC, m_pszButtonText, cchText, &textSize);

    INT xText, yText;
    if ((m_style & UIF_BUTTON_H_ALIGN_MASK) == UIF_BUTTON_H_ALIGN_CENTER)
        xText = (m_rc.right - m_rc.left - textSize.cx) / 2;
    else if ((m_style & UIF_BUTTON_H_ALIGN_MASK) == UIF_BUTTON_H_ALIGN_RIGHT)
        xText = m_rc.right - m_rc.left - textSize.cx;
    else
        xText = 0;

    if ((m_style & UIF_BUTTON_V_ALIGN_MASK) == UIF_BUTTON_V_ALIGN_MIDDLE)
        yText = (m_rc.bottom - m_rc.top - textSize.cy) / 2;
    else if ((m_style & UIF_BUTTON_V_ALIGN_MASK) == UIF_BUTTON_V_ALIGN_BOTTOM)
        yText = m_rc.bottom - m_rc.top - textSize.cy;
    else
        yText = 0;

    ::SetBkMode(hDC, TRANSPARENT);

    if (m_bEnable)
    {
        ::SetTextColor(hDC, ::GetSysColor(COLOR_BTNTEXT));
        ::ExtTextOutW(hDC,
                      xText + prc->left + bPressed, yText + prc->top + bPressed,
                      ETO_CLIPPED, prc,
                      m_pszButtonText, cchText, NULL);
    }
    else
    {
        ::SetTextColor(hDC, ::GetSysColor(COLOR_BTNHILIGHT));
        ::ExtTextOutW(hDC,
                      xText + prc->left + bPressed + 1, yText + prc->top + bPressed + 1,
                      ETO_CLIPPED, prc,
                      m_pszButtonText, cchText, NULL);

        ::SetTextColor(hDC, ::GetSysColor(COLOR_BTNSHADOW));
        ::ExtTextOutW(hDC,
                      xText + prc->left + bPressed, yText + prc->top + bPressed,
                      ETO_CLIPPED, prc,
                      m_pszButtonText, cchText, NULL);
    }

    ::SelectObject(hDC, hFontOld);
}

STDMETHODIMP_(void)
CUIFButton::Enable(BOOL bEnable)
{
    CUIFObject::Enable(bEnable);
    if (!m_bEnable)
    {
        SetStatus(0);
        if (IsCapture())
            CUIFObject::EndCapture();
    }
}

void
CUIFButton::GetIconSize(HICON hIcon, LPSIZE pSize)
{
    ICONINFO IconInfo;
    if (::GetIconInfo(hIcon, &IconInfo))
    {
        BITMAP bm;
        ::GetObject(IconInfo.hbmColor, sizeof(bm), &bm);
        ::DeleteObject(IconInfo.hbmColor);
        ::DeleteObject(IconInfo.hbmMask);
        pSize->cx = bm.bmWidth;
        pSize->cy = bm.bmHeight;
    }
    else
    {
        pSize->cx = ::GetSystemMetrics(SM_CXSMICON);
        pSize->cy = ::GetSystemMetrics(SM_CYSMICON);
    }
}

void
CUIFButton::GetTextSize(LPCWSTR pszText, LPSIZE pSize)
{
    HDC hDC = ::GetDC(NULL);
    INT cchText = lstrlenW(pszText);
    HGDIOBJ hFontOld = ::SelectObject(hDC, m_hFont);

    if (!m_bHasCustomFont && SUCCEEDED(EnsureThemeData(*m_pWindow)))
    {
        RECT rc;
        GetThemeTextExtent(hDC, 0, pszText, cchText, 0, NULL, &rc);
        pSize->cx = rc.right;
        pSize->cy = rc.bottom;
    }
    else
    {
        ::GetTextExtentPoint32W(hDC, pszText, cchText, pSize);
    }

    if (m_style & UIF_BUTTON_VERTICAL)
    {
        INT tmp = pSize->cx;
        pSize->cx = pSize->cy;
        pSize->cy = tmp;
    }

    ::SelectObject(hDC, hFontOld);
    ::ReleaseDC(NULL, hDC);
}

STDMETHODIMP_(void)
CUIFButton::OnLButtonDown(LONG x, LONG y)
{
    SetStatus(1);
    StartCapture();
    if ((m_style & 0x30) == 0x20)
        NotifyCommand(1, 0);
}

/// @unimplemented
STDMETHODIMP_(void)
CUIFButton::OnLButtonUp(LONG x, LONG y)
{
    POINT pt = { x, y };
    BOOL bCapture = IsCapture();
    if (bCapture)
        EndCapture();

    BOOL bNotInObject = (m_style & 0x30) == 0x20;
    if ((m_style & 0x30) != 0x10)
    {
        bNotInObject = !PtInObject(pt);
        if (bNotInObject)
        {
            SetStatus(0);
            return;
        }
    }
    else
    {
        if (!bNotInObject)
        {
            bNotInObject = !PtInObject(pt);
            if (!bNotInObject)
            {
                SetStatus(2);
                NotifyCommand(1, 0);
                return;
            }
        }
        SetStatus(0);
        return;
    }

    SetStatus(2);

    if (bCapture)
    {
        m_bPressed = !m_bPressed;
        NotifyCommand(1, 0);
    }
}

void CUIFButton::OnMouseIn(LONG x, LONG y)
{
    if ((m_style & 0x30) == 0x20)
    {
        if (IsCapture())
            SetStatus(0);
        else
            SetStatus(2);
    }
    else
    {
        if (IsCapture())
            SetStatus(1);
        else
            SetStatus(2);
    }
}

void CUIFButton::OnMouseOut(LONG x, LONG y)
{
    if ((m_style & 0x30) == 0x20)
        SetStatus(0);
    else if (IsCapture())
        SetStatus(3);
    else
        SetStatus(0);
}

STDMETHODIMP_(void)
CUIFButton::OnPaintNoTheme(HDC hDC)
{
    ::FillRect(hDC, &m_rc, (HBRUSH)UlongToHandle(COLOR_BTNFACE + 1));

    if (m_bPressed && ((m_uButtonStatus == 0) || (m_uButtonStatus == 3)))
    {
        HBRUSH hbr = cicCreateDitherBrush();
        if (hbr)
        {
            COLORREF OldTextColor = ::SetTextColor(hDC, ::GetSysColor(COLOR_BTNFACE));
            COLORREF OldBkColor = ::SetBkColor(hDC, ::GetSysColor(COLOR_BTNHIGHLIGHT));
            RECT rc = m_rc;
            ::InflateRect(&rc, -2, -2);
            ::FillRect(hDC, &rc, hbr);
            ::SetTextColor(hDC, OldTextColor);
            ::SetBkColor(hDC, OldBkColor);
            ::DeleteObject(hbr);
        }
    }

    BOOL bPressed = (m_bPressed || (m_uButtonStatus == 1));
    if (m_hbmButton1)
        DrawBitmapProc(hDC, &m_rc, bPressed);
    else if (m_ButtonIcon.m_hIcon)
        DrawIconProc(hDC, &m_rc, bPressed);
    else
        DrawTextProc(hDC, &m_rc, bPressed);

    if (m_bPressed || (m_uButtonStatus == 1))
        DrawEdgeProc(hDC, &m_rc, TRUE);
    else if (2 <= m_uButtonStatus && m_uButtonStatus <= 3)
        DrawEdgeProc(hDC, &m_rc, FALSE);
}

void CUIFButton::SetIcon(HICON hIcon)
{
    m_ButtonIcon = hIcon;

    if (m_ButtonIcon.m_hIcon)
        GetIconSize(m_ButtonIcon.m_hIcon, &m_IconSize);
    else
        m_IconSize.cx = m_IconSize.cy = 0;

    CallOnPaint();
}

void CUIFButton::SetStatus(UINT uStatus)
{
    if (uStatus != m_uButtonStatus)
    {
        m_uButtonStatus = uStatus;
        CallOnPaint();
    }
}

void CUIFButton::SetText(LPCWSTR pszText)
{
    if (m_pszButtonText)
    {
        delete[] m_pszButtonText;
        m_pszButtonText = NULL;
    }

    m_TextSize.cx = m_TextSize.cy = 0;

    if (pszText)
    {
        INT cch = lstrlenW(pszText);
        m_pszButtonText = new(cicNoThrow) WCHAR[cch + 1];
        if (!m_pszButtonText)
            return;

        lstrcpynW(m_pszButtonText, pszText, cch + 1);
        GetTextSize(m_pszButtonText, &m_TextSize);
    }

    CallOnPaint();
}

/////////////////////////////////////////////////////////////////////////////
// CUIFButton2

CUIFButton2::CUIFButton2(
    CUIFObject *pParent,
    DWORD nObjectID,
    LPCRECT prc,
    DWORD style) : CUIFButton(pParent, nObjectID, prc, style)
{
    m_iStateId = 0;
    m_iPartId = BP_PUSHBUTTON;
    m_pszClassList = L"TOOLBAR";
}

CUIFButton2::~CUIFButton2()
{
    CloseThemeData();
}

DWORD CUIFButton2::MakeDrawFlag()
{
    DWORD dwDrawFlags = 0;
    if (m_bPressed)
        dwDrawFlags |= UIF_DRAW_PRESSED;
    if (m_uButtonStatus == 1)
        dwDrawFlags |= 0x2;
    else if (2 <= m_uButtonStatus && m_uButtonStatus <= 3)
        dwDrawFlags |= 0x1;
    if (!m_bEnable)
        dwDrawFlags |= UIF_DRAW_DISABLED;
    return dwDrawFlags;
}

/// @unimplemented
STDMETHODIMP_(BOOL)
CUIFButton2::OnPaintTheme(HDC hDC)
{
    //FIXME
    return FALSE;
}

STDMETHODIMP_(void)
CUIFButton2::OnPaintNoTheme(HDC hDC)
{
    if (!m_pScheme)
       return;

    INT width = m_rc.right - m_rc.left;
    INT height = m_rc.bottom - m_rc.top;
    HDC hdcMem = ::CreateCompatibleDC(hDC);
    if (!hdcMem)
        return;

    HBITMAP hbmMem = ::CreateCompatibleBitmap(hDC, width, height);
    if ( !hbmMem )
    {
        ::DeleteDC(hdcMem);
        return;
    }

    HGDIOBJ hbmOld = ::SelectObject(hdcMem, hbmMem);
    HGDIOBJ hFontOld = ::SelectObject(hdcMem, m_hFont);
    RECT rcBack = { 0, 0, width, height };

    INT cxText = 0, cyText = 0, cxContent = 0, cyContent = 0;
    INT cxyBorders, cxButton, cyButton;
    if (m_pszButtonText)
    {
        cxText = m_TextSize.cx;
        cyText = m_TextSize.cy;
    }

    if (m_ButtonIcon.m_hIcon)
    {
        cxContent = m_IconSize.cx;
        cyContent = m_IconSize.cy;
    }
    else if (m_hbmButton1)
    {
        cxContent = m_BitmapSize.cx;
        cyContent = m_BitmapSize.cy;
    }

    if (m_style & UIF_BUTTON_VERTICAL)
    {
        cxyBorders = ((cyText && cyContent) ? 2 : 0);

        cxButton = cxContent;
        cyButton = cyText + cyContent + cxyBorders;
        if (cxText > cxContent)
            cxButton = cxText;
    }
    else
    {
        cxyBorders = ((cxText && cxContent) ? 2 : 0);

        cxButton = cxText + cxContent + cxyBorders;
        cyButton = cyContent;
        if (cyText > cyButton)
            cyButton = cyText;
    }

    INT xOffset, yOffset;
    if ((m_style & UIF_BUTTON_H_ALIGN_MASK) == UIF_BUTTON_H_ALIGN_CENTER)
        xOffset = (rcBack.left + rcBack.right - cxButton) / 2;
    else if ((m_style & UIF_BUTTON_H_ALIGN_MASK) == UIF_BUTTON_H_ALIGN_RIGHT)
        xOffset = rcBack.right - cxText - 2;
    else
        xOffset = rcBack.left + 2;


    if ((m_style & UIF_BUTTON_V_ALIGN_MASK) == UIF_BUTTON_V_ALIGN_MIDDLE)
        yOffset = (rcBack.top + rcBack.bottom - cyButton) / 2;
    else if ((m_style & UIF_BUTTON_V_ALIGN_MASK) == UIF_BUTTON_V_ALIGN_BOTTOM)
        yOffset = rcBack.bottom - cyButton - 2;
    else
        yOffset = rcBack.top + 2;

    RECT rc = { xOffset, yOffset, xOffset + cxButton, cyButton + yOffset };
    SIZE offsetSize = { 0, 0 };
    DWORD dwDrawFlags = MakeDrawFlag();
    m_pScheme->GetCtrlFaceOffset(((m_style & 0x200) ? 0xA5 : 0x54),
                                 dwDrawFlags,
                                 &offsetSize);
    ::OffsetRect(&rc, offsetSize.cx, offsetSize.cy);

    RECT rcImage, rcText;
    if (m_style & UIF_BUTTON_VERTICAL)
    {
        rcImage.left    = (rc.left + rc.right - cxContent) / 2;
        rcImage.top     = rc.top;
        rcImage.right   = rcImage.left + cxContent;
        rcImage.bottom  = rc.top + cyContent;
        rcText.left     = (rc.left + rc.right - cxText) / 2;
        rcText.top      = rc.bottom - cyText;
        rcText.right    = rcText.left + cxText;
        rcText.bottom   = rc.bottom;
    }
    else
    {
        rcImage.left    = rc.left;
        rcImage.top     = (rc.top + rc.bottom - cyContent) / 2;
        rcImage.bottom  = rcImage.top + cyContent;
        rcImage.right   = rc.left + cxContent;
        rcText.left     = rc.right - cxText;
        rcText.top      = (rc.top + rc.bottom - cyText) / 2;
        rcText.right    = rc.right;
        rcText.bottom   = rcText.top + cyText;
    }

    if (IsRTL())
        m_pScheme->m_bMirroring = TRUE;

    m_pScheme->DrawCtrlBkgd(hdcMem,
                            &rcBack,
                            ((m_style & 0x200) ? 0xA5 : 0x54),
                            dwDrawFlags);
    if (m_pszButtonText)
    {
        m_pScheme->DrawCtrlText(hdcMem, &rcText, m_pszButtonText, -1, dwDrawFlags,
                                !!(m_style & UIF_BUTTON_VERTICAL));
    }

    if (m_ButtonIcon.m_hIcon)
        m_pScheme->DrawCtrlIcon(hdcMem, &rcImage, m_ButtonIcon.m_hIcon, dwDrawFlags, &m_IconSize);
    else if (m_hbmButton1)
        m_pScheme->DrawCtrlBitmap(hdcMem, &rcImage, m_hbmButton1, m_hbmButton2, dwDrawFlags);

    if (IsRTL())
        m_pScheme->m_bMirroring = FALSE;

    m_pScheme->DrawCtrlEdge(hdcMem,
                            &rcBack,
                            ((m_style & 0x200) ? 0xA5 : 0x54),
                            dwDrawFlags);

    ::BitBlt(hDC, m_rc.left, m_rc.top, width, height, hdcMem, 0, 0, SRCCOPY);
    ::SelectObject(hdcMem, hFontOld);
    ::SelectObject(hdcMem, hbmOld);
    ::DeleteObject(hbmMem);
}

/////////////////////////////////////////////////////////////////////////////
// CUIFGripper

CUIFGripper::CUIFGripper(CUIFObject *pParent, LPCRECT prc, DWORD style)
    : CUIFObject(pParent, 0, prc, style)
{
    m_iStateId = 0;
    m_pszClassList = L"REBAR";
    if (m_style & UIF_GRIPPER_VERTICAL)
        m_iPartId = RP_GRIPPERVERT;
    else
        m_iPartId = RP_GRIPPER;
}

CUIFGripper::~CUIFGripper()
{
}

STDMETHODIMP_(void)
CUIFGripper::OnMouseMove(LONG x, LONG y)
{
    if (IsCapture())
    {
        POINT pt;
        ::GetCursorPos(&pt);
        m_pWindow->Move(pt.x - m_ptGripper.x, pt.y - m_ptGripper.y, -1, -1);
    }
}

STDMETHODIMP_(void)
CUIFGripper::OnLButtonDown(LONG x, LONG y)
{
    StartCapture();
    m_ptGripper.x = x;
    m_ptGripper.y = y;
    ::ScreenToClient(*m_pWindow, &m_ptGripper);
    RECT rc;
    ::GetWindowRect(*m_pWindow, &rc);
    m_ptGripper.x -= rc.left;
    m_ptGripper.y -= rc.top;
}

STDMETHODIMP_(void)
CUIFGripper::OnLButtonUp(LONG x, LONG y)
{
    if (IsCapture())
        EndCapture();
}

STDMETHODIMP_(BOOL)
CUIFGripper::OnPaintTheme(HDC hDC)
{
    if (FAILED(EnsureThemeData(*m_pWindow)))
        return FALSE;

    if (m_style & UIF_GRIPPER_VERTICAL)
    {
        m_rc.top += 2;
        m_rc.bottom -= 2;
    }
    else
    {
        m_rc.left += 2;
        m_rc.right -= 2;
    }

    if (FAILED(DrawThemeBackground(hDC, 1, &m_rc, 0)))
        return FALSE;

    return TRUE;
}

STDMETHODIMP_(void)
CUIFGripper::OnPaintNoTheme(HDC hDC)
{
    if (m_pScheme)
    {
        m_pScheme->DrawDragHandle(hDC, &m_rc, !!(m_style & UIF_GRIPPER_VERTICAL));
        return;
    }

    RECT rc;
    if (m_style & UIF_GRIPPER_VERTICAL)
        rc = { m_rc.left, m_rc.top + 1, m_rc.right, m_rc.top + 4 };
    else
        rc = { m_rc.left + 1, m_rc.top, m_rc.left + 4, m_rc.bottom };

    ::DrawEdge(hDC, &rc, BDR_RAISEDINNER, BF_RECT);
}

STDMETHODIMP_(BOOL)
CUIFGripper::OnSetCursor(UINT uMsg, LONG x, LONG y)
{
    HCURSOR hCursor = ::LoadCursor(NULL, IDC_SIZEALL);
    ::SetCursor(hCursor);
    return TRUE;
}

STDMETHODIMP_(void)
CUIFGripper::SetStyle(DWORD style)
{
    m_style = style;
    if (m_style & UIF_GRIPPER_VERTICAL)
        SetActiveTheme(L"REBAR", RP_GRIPPERVERT, 0);
    else
        SetActiveTheme(L"REBAR", RP_GRIPPER, 0);
}

/////////////////////////////////////////////////////////////////////////////
// CUIFToolbarMenuButton

CUIFToolbarMenuButton::CUIFToolbarMenuButton(
    CUIFToolbarButton *pParent,
    DWORD nObjectID,
    LPCRECT prc,
    DWORD style) : CUIFButton2(pParent, nObjectID, prc, style)
{
    m_pToolbarButton = pParent;

    HFONT hFont = ::CreateFontW(8, 8, 0, 0, FW_NORMAL, 0, 0, 0, SYMBOL_CHARSET,
                                0, 0, 0, 0, L"Marlett");
    SetFont(hFont);
    SetText(L"u"); // downward triangle
}

CUIFToolbarMenuButton::~CUIFToolbarMenuButton()
{
    ::DeleteObject(m_hFont);
    SetFont(NULL);
}

STDMETHODIMP_(void)
CUIFToolbarMenuButton::OnLButtonUp(LONG x, LONG y)
{
    CUIFButton::OnLButtonUp(x, y);
    m_pToolbarButton->OnUnknownMouse0();
}

STDMETHODIMP_(BOOL)
CUIFToolbarMenuButton::OnSetCursor(UINT uMsg, LONG x, LONG y)
{
    m_pToolbarButton->OnSetCursor(uMsg, x, y);
    return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// CUIFToolbarButtonElement

CUIFToolbarButtonElement::CUIFToolbarButtonElement(
    CUIFToolbarButton *pParent,
    DWORD nObjectID,
    LPCRECT prc,
    DWORD style) : CUIFButton2(pParent, nObjectID, prc, style)
{
    m_pToolbarButton = pParent;
}

STDMETHODIMP_(LPCWSTR)
CUIFToolbarButtonElement::GetToolTip()
{
    if (m_pToolbarButton)
        return m_pToolbarButton->GetToolTip();
    return NULL;
}

STDMETHODIMP_(void)
CUIFToolbarButtonElement::OnLButtonUp(LONG x, LONG y)
{
    CUIFButton::OnLButtonUp(x, y);
    if ((m_pToolbarButton->m_dwToolbarButtonFlags & 0x30000) == 0x20000)
        m_pToolbarButton->OnUnknownMouse0();
    else
        m_pToolbarButton->OnLeftClick();
}

STDMETHODIMP_(void)
CUIFToolbarButtonElement::OnRButtonUp(LONG x, LONG y)
{
    if ((m_pToolbarButton->m_dwToolbarButtonFlags & 0x30000) != 0x20000)
        m_pToolbarButton->OnRightClick();
}

/////////////////////////////////////////////////////////////////////////////
// CUIFToolbarButton

CUIFToolbarButton::CUIFToolbarButton(
    CUIFObject *pParent,
    DWORD nObjectID,
    LPCRECT prc,
    DWORD style,
    DWORD dwToolbarButtonFlags,
    LPCWSTR pszUnknownText) : CUIFObject(pParent, nObjectID, prc, style)
{
    m_dwToolbarButtonFlags = dwToolbarButtonFlags;
    m_pszUnknownText = pszUnknownText;
}

BOOL CUIFToolbarButton::Init()
{
    RECT rc1, rc2;
    rc1 = rc2 = m_rc;

    if ((m_dwToolbarButtonFlags & 0x30000) == 0x30000)
    {
        rc1.right -= 12;
        rc2.left = rc1.right + 1;
    }

    DWORD style = UIF_BUTTON_LARGE_ICON | UIF_BUTTON_V_ALIGN_MIDDLE | UIF_BUTTON_H_ALIGN_CENTER;
    if (m_dwToolbarButtonFlags & 0x2000)
        style |= 0x10;
    if (m_dwToolbarButtonFlags & 0x80000)
        style |= UIF_BUTTON_VERTICAL;
    m_pToolbarButtonElement = new(cicNoThrow) CUIFToolbarButtonElement(this, m_nObjectID, &rc1, style);
    if (!m_pToolbarButtonElement)
        return FALSE;

    m_pToolbarButtonElement->Initialize();
    AddUIObj(m_pToolbarButtonElement);

    if (!m_bEnable)
        m_pToolbarButtonElement->Enable(FALSE);

    if ((m_dwToolbarButtonFlags & 0x30000) == 0x30000)
    {
        style = UIF_BUTTON_LARGE_ICON | UIF_BUTTON_H_ALIGN_CENTER;
        if (m_dwToolbarButtonFlags & 0x80000)
            style |= UIF_BUTTON_VERTICAL;

        m_pToolbarMenuButton = new(cicNoThrow) CUIFToolbarMenuButton(this, 0, &rc2, style);
        if (!m_pToolbarMenuButton)
            return FALSE;

        m_pToolbarMenuButton->Initialize();
        AddUIObj(m_pToolbarMenuButton);

        if (!m_bEnable)
            m_pToolbarMenuButton->Enable(FALSE);
    }

    return TRUE;
}

HICON CUIFToolbarButton::GetIcon()
{
    return m_pToolbarButtonElement->m_ButtonIcon.m_hIcon;
}

void CUIFToolbarButton::SetIcon(HICON hIcon)
{
    m_pToolbarButtonElement->SetIcon(hIcon);
}

STDMETHODIMP_(void)
CUIFToolbarButton::ClearWndObj()
{
    if (m_pToolbarButtonElement)
        m_pToolbarButtonElement->ClearWndObj();
    if (m_pToolbarMenuButton)
        m_pToolbarMenuButton->ClearWndObj();

    CUIFObject::ClearWndObj();
}

STDMETHODIMP_(void)
CUIFToolbarButton::DetachWndObj()
{
    if (m_pToolbarButtonElement)
        m_pToolbarButtonElement->DetachWndObj();
    if (m_pToolbarMenuButton)
        m_pToolbarMenuButton->DetachWndObj();

    CUIFObject::DetachWndObj();
}

STDMETHODIMP_(void)
CUIFToolbarButton::Enable(BOOL bEnable)
{
    CUIFObject::Enable(bEnable);
    if (m_pToolbarButtonElement)
        m_pToolbarButtonElement->Enable(bEnable);
    if (m_pToolbarMenuButton)
        m_pToolbarMenuButton->Enable(bEnable);
}

STDMETHODIMP_(LPCWSTR)
CUIFToolbarButton::GetToolTip()
{
    return CUIFObject::GetToolTip();
}

STDMETHODIMP_(void)
CUIFToolbarButton::SetActiveTheme(LPCWSTR pszClassList, INT iPartId, INT iStateId)
{
    if (m_pToolbarButtonElement)
        m_pToolbarButtonElement->SetActiveTheme(pszClassList, iPartId, iStateId);
    if (m_pToolbarMenuButton)
        m_pToolbarMenuButton->SetActiveTheme(pszClassList, iPartId, iStateId);

    m_pszClassList = pszClassList;
    m_iPartId = iPartId;
    m_iStateId = iStateId;
}

STDMETHODIMP_(void)
CUIFToolbarButton::SetFont(HFONT hFont)
{
    m_pToolbarButtonElement->SetFont(hFont);
}

STDMETHODIMP_(void)
CUIFToolbarButton::SetRect(LPCRECT prc)
{
    CUIFObject::SetRect(prc);

    RECT rc1, rc2;
    rc1 = rc2 = m_rc;

    if ((m_dwToolbarButtonFlags & 0x30000) == 0x30000)
    {
        rc1.right -= 12;
        rc2.left = rc1.right + 1;
    }

    if (m_pToolbarButtonElement)
        m_pToolbarButtonElement->SetRect(&rc1);
    if (m_pToolbarMenuButton)
        m_pToolbarMenuButton->SetRect(&rc2);
}

STDMETHODIMP_(void)
CUIFToolbarButton::SetToolTip(LPCWSTR pszToolTip)
{
    CUIFObject::SetToolTip(pszToolTip);
    if (m_pToolbarButtonElement)
        m_pToolbarButtonElement->SetToolTip(pszToolTip);
    if (m_pToolbarMenuButton)
        m_pToolbarMenuButton->SetToolTip(pszToolTip);
}

/////////////////////////////////////////////////////////////////////////////
// CUIFWndFrame

CUIFWndFrame::CUIFWndFrame(
    CUIFObject *pParent,
    LPCRECT prc,
    DWORD style) : CUIFObject(pParent, 0, prc, style)
{
    m_iPartId = 7;
    m_iStateId = 0;
    m_pszClassList = L"WINDOW";
    m_dwHitTest = 0;
    m_cxFrame = m_cyFrame = 0;

    if (m_pScheme)
    {
        if ((m_style & 0xF) && (m_style & 0xF) <= 2)
        {
            m_cxFrame = m_pScheme->CxSizeFrame();
            m_cyFrame = m_pScheme->CySizeFrame();
        }
        else
        {
            m_cxFrame = m_pScheme->CxWndBorder();
            m_cyFrame = m_pScheme->CyWndBorder();
        }
    }

    m_cxMin = GetSystemMetrics(SM_CXMIN);
    m_cyMin = GetSystemMetrics(SM_CYMIN);
}

void CUIFWndFrame::GetFrameSize(LPSIZE pSize)
{
    pSize->cx = m_cxFrame;
    pSize->cy = m_cyFrame;
}

DWORD CUIFWndFrame::HitTest(LONG x, LONG y)
{
    DWORD dwFlags = 0;
    if ( m_rc.left <= x && x < m_rc.left + m_cxFrame)
        dwFlags |= 0x10;
    if (m_rc.top <= y && y < m_rc.top + m_cyFrame )
        dwFlags |= 0x20;
    if (m_rc.right - m_cxFrame <= x && x < m_rc.right)
        dwFlags |= 0x40;
    if (m_rc.bottom - m_cyFrame <= y && y < m_rc.bottom)
        dwFlags |= 0x80;
    return dwFlags;
}

STDMETHODIMP_(void)
CUIFWndFrame::OnMouseMove(LONG x, LONG y)
{
    if (!IsCapture())
        return;

    POINT Point;
    ::ClientToScreen(*m_pWindow, &Point);

    RECT rc = m_rcWnd;

    if (m_dwHitTest & 0x10)
        rc.left = Point.x + m_rcWnd.left - m_ptHit.x;

    if (m_dwHitTest & 0x20)
        rc.top = Point.y + m_rcWnd.top - m_ptHit.y;

    if (m_dwHitTest & 0x40)
    {
        rc.right = Point.x + m_rcWnd.right - m_ptHit.x;
        if (rc.right <= rc.left + m_cxMin)
            rc.right = rc.left + m_cxMin;
    }

    if (m_dwHitTest & 0x80)
    {
        rc.bottom = Point.y + m_rcWnd.bottom - m_ptHit.y;
        if (rc.bottom <= rc.top + m_cyMin)
            rc.bottom = rc.top + m_cyMin;
    }

    m_pWindow->Move(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
}

STDMETHODIMP_(void)
CUIFWndFrame::OnLButtonDown(LONG x, LONG y)
{
    POINT Point = { x, y };
    DWORD hitTest = m_style & HitTest(x, y);
    if (!hitTest)
        return;

    ::ClientToScreen(*m_pWindow, &Point);
    m_ptHit = Point;
    m_pWindow = m_pWindow;
    m_dwHitTest = hitTest;
    ::GetWindowRect(*m_pWindow, &m_rcWnd);
    StartCapture();
}

STDMETHODIMP_(void)
CUIFWndFrame::OnLButtonUp(LONG x, LONG y)
{
    if (IsCapture())
        EndCapture();
}

STDMETHODIMP_(BOOL)
CUIFWndFrame::OnPaintTheme(HDC hDC)
{
    if (FAILED(EnsureThemeData(*m_pWindow)))
        return FALSE;

    RECT rc = m_rc;
    rc.right = m_cxFrame;
    if (FAILED(DrawThemeEdge(hDC, 0, &rc, 5, 1, NULL)))
        return FALSE;

    rc = m_rc;
    rc.left = rc.right - m_cxFrame;
    if (FAILED(DrawThemeEdge(hDC, 0, &rc, 10, 4, NULL)))
        return FALSE;

    rc = m_rc;
    rc.bottom = m_cyFrame;
    if (FAILED(DrawThemeEdge(hDC, 0, &rc, 5, 2, NULL)))
        return FALSE;

    rc = m_rc;
    rc.top = rc.bottom - m_cyFrame;
    if (FAILED(DrawThemeEdge(hDC, 0, &rc, 10, 8, NULL)))
        return FALSE;

    return TRUE;
}

STDMETHODIMP_(void)
CUIFWndFrame::OnPaintNoTheme(HDC hDC)
{
    if (!m_pScheme)
        return;

    DWORD type = 0;
    if ((m_style & 0xF) == 1)
        type = 1;
    else if ( (m_style & 0xF) == 2 )
        type = 2;

    m_pScheme->DrawWndFrame(hDC, &m_rc, type, m_cxFrame, m_cyFrame);
}

STDMETHODIMP_(BOOL)
CUIFWndFrame::OnSetCursor(UINT uMsg, LONG x, LONG y)
{
    DWORD dwHitTest = m_dwHitTest;
    if (!IsCapture())
        dwHitTest = m_style & HitTest(x, y);

    LPTSTR pszCursor = NULL;
    switch (dwHitTest)
    {
        case 0x30:
        case 0xC0:
            pszCursor = IDC_SIZENWSE;
            break;
        case 0x90:
        case 0x60:
            pszCursor = IDC_SIZENESW;
            break;
        case 0x10:
        case 0x40:
            pszCursor = IDC_SIZEWE;
            break;
        case 0x20:
        case 0x80:
            pszCursor = IDC_SIZENS;
            break;
        default:
            return FALSE;
    }

    HCURSOR hCursor = ::LoadCursor(NULL, pszCursor);
    ::SetCursor(hCursor);
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CUIFBalloonButton

CUIFBalloonButton::CUIFBalloonButton(
    CUIFObject *pParent,
    DWORD nObjectID,
    LPCRECT prc,
    DWORD style) : CUIFButton(pParent, nObjectID, prc, style)
{
    m_nCommandID = 0;
}

STDMETHODIMP_(void)
CUIFBalloonButton::OnPaint(HDC hDC)
{
    RECT rc = m_rc;
    ::OffsetRect(&rc, -rc.left, -rc.top);

    HDC hMemDC = ::CreateCompatibleDC(hDC);
    HBITMAP hbmMem = ::CreateCompatibleBitmap(hDC, rc.right, rc.bottom);
    HGDIOBJ hbmOld = ::SelectObject(hMemDC, hbmMem);

    BOOL bPressed;
    COLORREF rgbShadow, rgbBorder;
    if (m_uButtonStatus == 1)
    {
        bPressed = TRUE;
        rgbShadow = ::GetSysColor(COLOR_BTNSHADOW);
        rgbBorder = ::GetSysColor(COLOR_BTNHIGHLIGHT);
    }
    else
    {
        bPressed = FALSE;
        if (m_uButtonStatus < 4)
        {
            rgbShadow = ::GetSysColor(COLOR_BTNHIGHLIGHT);
            rgbBorder = ::GetSysColor(COLOR_BTNSHADOW);
        }
        else
        {
            rgbShadow = ::GetSysColor(COLOR_INFOBK);
            rgbBorder = ::GetSysColor(COLOR_INFOBK);
        }
    }

    COLORREF rgbInfoBk = ::GetSysColor(COLOR_INFOBK);
    HBRUSH hbrBack = ::CreateSolidBrush(rgbInfoBk);
    ::FillRect(hMemDC, &rc, hbrBack);
    ::DeleteObject(hbrBack);

    DrawTextProc(hMemDC, &rc, bPressed);

    HBRUSH hNullBrush = (HBRUSH)::GetStockObject(NULL_BRUSH);
    HGDIOBJ hbrOld = ::SelectObject(hMemDC, hNullBrush);

    HPEN hPen = ::CreatePen(PS_SOLID, 0, rgbShadow);
    HGDIOBJ hPenOld = ::SelectObject(hMemDC, hPen);
    ::RoundRect(hMemDC, rc.left, rc.top, rc.right - 1, rc.bottom - 1, 6, 6);
    ::SelectObject(hMemDC, hPenOld);
    ::DeleteObject(hPen);

    hPen = ::CreatePen(PS_SOLID, 0, rgbBorder);
    hPenOld = ::SelectObject(hMemDC, hPen);
    ::RoundRect(hMemDC, rc.left + 1, rc.top + 1, rc.right, rc.bottom, 6, 6);
    ::SelectObject(hMemDC, hPenOld);
    ::DeleteObject(hPen);

    hPen = ::CreatePen(PS_SOLID, 0, ::GetSysColor(COLOR_BTNFACE));
    hPenOld = ::SelectObject(hMemDC, hPen);
    ::RoundRect(hMemDC, rc.left + 1, rc.top + 1, rc.right - 1, rc.bottom - 1, 6, 6);
    ::SelectObject(hMemDC, hPenOld);
    ::DeleteObject(hPen);

    ::SelectObject(hMemDC, hbrOld);
    ::BitBlt(hDC, m_rc.left, m_rc.top, m_rc.right - m_rc.left, m_rc.bottom - m_rc.top,
             hMemDC, rc.left, rc.top, SRCCOPY);
    ::SelectObject(hMemDC, hbmOld);
    ::DeleteObject(hbmMem);
    ::DeleteDC(hMemDC);
}

void
CUIFBalloonButton::DrawTextProc(HDC hDC, LPCRECT prc, BOOL bPressed)
{
    if (!m_pszButtonText)
        return;

    UINT uFlags = DT_SINGLELINE;

    if ((m_style & UIF_BUTTON_H_ALIGN_MASK) == UIF_BUTTON_H_ALIGN_CENTER)
        uFlags |= DT_CENTER;
    else if ((m_style & UIF_BUTTON_H_ALIGN_MASK) == UIF_BUTTON_H_ALIGN_RIGHT)
        uFlags |= DT_RIGHT;

    if ((m_style & UIF_BUTTON_V_ALIGN_MASK) == UIF_BUTTON_V_ALIGN_MIDDLE)
        uFlags |= DT_VCENTER;
    else if ((m_style & UIF_BUTTON_V_ALIGN_MASK) == UIF_BUTTON_V_ALIGN_BOTTOM)
        uFlags |= DT_BOTTOM;

    COLORREF rgbOldColor = ::SetTextColor(hDC, ::GetSysColor(COLOR_BTNTEXT));
    INT nOldBkMode = ::SetBkMode(hDC, TRANSPARENT);

    RECT rc = *prc;
    if (bPressed)
        ::OffsetRect(&rc, 1, 1);

    HGDIOBJ hFontOld = ::SelectObject(hDC, m_hFont);
    ::DrawTextW(hDC, m_pszButtonText, -1, &rc, uFlags);
    ::SelectObject(hDC, hFontOld);

    ::SetBkMode(hDC, nOldBkMode);
    ::SetTextColor(hDC, rgbOldColor);
}

/////////////////////////////////////////////////////////////////////////////
// CUIFBalloonWindow

CUIFBalloonWindow::CUIFBalloonWindow(HINSTANCE hInst, DWORD style) : CUIFWindow(hInst, style)
{
    m_dwUnknown6 = -1;
    m_nActionID = -1;
    m_hRgn = NULL;
    m_pszBalloonText = NULL;
    m_bHasBkColor = m_bHasTextColor = FALSE;
    m_rgbBkColor = 0;
    m_rgbTextColor = 0;
    m_ptTarget.x = m_ptTarget.y = 0;
    ZeroMemory(&m_rcExclude, sizeof(m_rcExclude));
    m_dwUnknown7 = 0;
    m_nBalloonType = 0;
    m_dwUnknown8[0] = 0;
    m_dwUnknown8[1] = 0;
    m_ptBalloon.x = m_ptBalloon.y = 0;
    m_cButtons = 0;
    m_hwndNotif = NULL;
    m_uNotifMsg = 0;
    m_rcMargin.left = 8;
    m_rcMargin.top = 8;
    m_rcMargin.right = 8;
    m_rcMargin.bottom = 8;
}

CUIFBalloonWindow::~CUIFBalloonWindow()
{
    if (m_pszBalloonText)
    {
        delete[] m_pszBalloonText;
        m_pszBalloonText = NULL;
    }
}

STDMETHODIMP_(BOOL)
CUIFBalloonWindow::Initialize()
{
    CUIFWindow::Initialize();

    if ((m_style & UIF_BALLOON_WINDOW_TYPE_MASK) == UIF_BALLOON_WINDOW_OK)
    {
        AddButton(IDOK);
    }
    else if ((m_style & UIF_BALLOON_WINDOW_TYPE_MASK) == UIF_BALLOON_WINDOW_YESNO)
    {
        AddButton(IDYES);
        AddButton(IDNO);
    }

    return TRUE;
}

STDMETHODIMP_(void)
CUIFBalloonWindow::OnCreate(HWND hWnd)
{
    m_nActionID = -1;
    AdjustPos();
}

STDMETHODIMP_(void)
CUIFBalloonWindow::OnDestroy(HWND hWnd)
{
    SendNotification(m_nActionID);
    DoneWindowRegion();
}

STDMETHODIMP_(void)
CUIFBalloonWindow::OnKeyDown(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    CUIFBalloonButton *pButton = NULL;

    switch (wParam)
    {
        case VK_RETURN:
            pButton = (CUIFBalloonButton *)FindUIObject(0);
            break;
        case VK_ESCAPE:
            m_nActionID = -1;
            ::DestroyWindow(m_hWnd);
            return;
        case TEXT('Y'):
            pButton = FindButton(IDYES);
            break;
        case TEXT('N'):
            pButton = FindButton(IDNO);
            break;
    }

    if (!pButton)
        return;

    m_nActionID = pButton->m_nCommandID;
    ::DestroyWindow(m_hWnd);
}

STDMETHODIMP_(LRESULT)
CUIFBalloonWindow::OnObjectNotify(CUIFObject *pObject, WPARAM wParam, LPARAM lParam)
{
    CUIFBalloonButton *pButton = (CUIFBalloonButton *)pObject;
    m_nActionID = pButton->m_nCommandID;
    ::DestroyWindow(m_hWnd);
    return 0;
}

STDMETHODIMP_(void)
CUIFBalloonWindow::OnPaint(HDC hDC)
{
    RECT rc;
    GetRect(&rc);
    PaintFrameProc(hDC, &rc);

    switch (m_nBalloonType)
    {
        case 1:
            rc.top += 16;
            break;
        case 2:
            rc.right -= 16;
            break;
        case 3:
            rc.left += 16;
            break;
        default:
            rc.bottom -= 16;
            break;
    }

    RECT rcMargin;
    GetMargin(&rcMargin);

    rc.left += rcMargin.left;
    rc.top += rcMargin.top;
    rc.right -= rcMargin.right;
    rc.bottom -= rcMargin.bottom;

    PaintMessageProc(hDC, &rc, m_pszBalloonText);
}

void
CUIFBalloonWindow::AddButton(UINT nCommandId)
{
    RECT rc = { 0, 0, 0, 0 };
    if (!((IDOK <= nCommandId) && (nCommandId <= IDNO)))
        return;

    CUIFBalloonButton *pButton = new(cicNoThrow) CUIFBalloonButton(this, m_cButtons, &rc, 5);
    if (!pButton)
        return;

    pButton->Initialize();
    pButton->m_nCommandID = nCommandId;

    LPCWSTR pszText;
#ifdef IDS_OK
    extern HINSTANCE g_hInst;
    WCHAR szText[64];
    ::LoadStringW(g_hInst, IDS_OK + (nCommandId - IDOK), szText, _countof(szText));
    pszText = szText;
#else
    switch (nCommandId)
    {
        case IDOK:      pszText = L"OK";      break;
        case IDCANCEL:  pszText = L"Cancel";  break;
        case IDABORT:   pszText = L"&Abort";  break;
        case IDRETRY:   pszText = L"&Retry";  break;
        case IDIGNORE:  pszText = L"&Ignore"; break;
        case IDYES:     pszText = L"&Yes";    break;
        default:        pszText = L"&No";     break;
    }
#endif

    pButton->SetText(pszText);

    AddUIObj(pButton);
    ++m_cButtons;
}

/// @unimplemented
void
CUIFBalloonWindow::AdjustPos()
{
    //FIXME
}

HRGN
CUIFBalloonWindow::CreateRegion(LPCRECT prc)
{
    POINT Points[4];
    HRGN hRgn;
    BOOL bFlag = FALSE;
    LONG x, y;

    switch (m_nBalloonType)
    {
        case 1:
            hRgn = ::CreateRoundRectRgn(prc->left, prc->top + 16, prc->right, prc->bottom, 16, 16);
            y = prc->top + 16;
            bFlag = TRUE;
            break;

        case 2:
            hRgn = ::CreateRoundRectRgn(prc->left, prc->top, prc->right - 16, prc->bottom, 16, 16);
            x = prc->right - 17;
            break;

        case 3:
            hRgn = ::CreateRoundRectRgn(prc->left + 16, prc->top, prc->right, prc->bottom, 16, 16);
            x = prc->left + 16;
            break;

        default:
            hRgn = ::CreateRoundRectRgn(prc->left, prc->top, prc->right, prc->bottom - 16, 16, 16);
            y = prc->bottom - 17;
            bFlag = TRUE;
            break;
    }

    if (bFlag)
    {
        x = Points[1].x = m_ptBalloon.x;
        Points[1].y = m_ptBalloon.y;
        Points[0].y = Points[2].y = Points[3].y = y;
        Points[2].x = x + 10 * (2 * (m_dwUnknown8[0] == 0) - 1);
    }
    else
    {
        Points[2].x = x;
        y = Points[0].y = Points[1].y = Points[3].y = m_ptBalloon.y;
        Points[1].x = m_ptBalloon.x;
        Points[2].y = y + 10 * (2 * (m_dwUnknown8[0] == 2) - 1);
    }

    Points[0].x = Points[3].x = x;

    HRGN hPolygonRgn = ::CreatePolygonRgn(Points, _countof(Points), WINDING);
    ::CombineRgn(hRgn, hRgn, hPolygonRgn, RGN_OR);
    ::DeleteObject(hPolygonRgn);
    return hRgn;
}

void
CUIFBalloonWindow::DoneWindowRegion()
{
    if (m_hRgn)
    {
        ::SetWindowRgn(m_hWnd, NULL, TRUE);
        ::DeleteObject(m_hRgn);
        m_hRgn = NULL;
    }
}

CUIFBalloonButton *
CUIFBalloonWindow::FindButton(UINT nCommandID)
{
    for (UINT iButton = 0; iButton < m_cButtons; ++iButton)
    {
        CUIFBalloonButton *pButton = (CUIFBalloonButton *)FindUIObject(iButton);
        if (pButton && (pButton->m_nCommandID == nCommandID))
            return pButton;
    }
    return NULL;
}

CUIFObject *
CUIFBalloonWindow::FindUIObject(UINT nObjectID)
{
    for (size_t iItem = 0; iItem < m_ObjectArray.size(); ++iItem)
    {
        CUIFObject *pObject = m_ObjectArray[iItem];
        if (pObject->m_nObjectID == nObjectID)
            return pObject;
    }
    return NULL;
}

COLORREF
CUIFBalloonWindow::GetBalloonBkColor()
{
    if (m_bHasBkColor)
        return m_rgbBkColor;
    else
        return ::GetSysColor(COLOR_INFOBK);
}

COLORREF
CUIFBalloonWindow::GetBalloonTextColor()
{
    if (m_bHasTextColor)
        return m_rgbTextColor;
    else
        return ::GetSysColor(COLOR_INFOTEXT);
}

void
CUIFBalloonWindow::GetButtonSize(LPSIZE pSize)
{
    HDC hDisplayDC = ::CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);

    TEXTMETRIC tm;
    HGDIOBJ hFontOld = ::SelectObject(hDisplayDC, m_hFont);
    ::GetTextMetrics(hDisplayDC, &tm);
    ::SelectObject(hDisplayDC, hFontOld);
    ::DeleteDC(hDisplayDC);

    pSize->cx = 16 * tm.tmAveCharWidth;
    pSize->cy = tm.tmHeight + 10;
}

void
CUIFBalloonWindow::GetMargin(LPRECT prcMargin)
{
    if (prcMargin)
        *prcMargin = m_rcMargin;
}

void
CUIFBalloonWindow::SetExcludeRect(LPCRECT prcExclude)
{
    m_rcExclude = *prcExclude;
    AdjustPos();
}

void
CUIFBalloonWindow::SetTargetPos(POINT ptTarget)
{
    m_ptTarget = ptTarget;
    AdjustPos();
}

void
CUIFBalloonWindow::SetText(LPCWSTR pszText)
{
    if (m_pszBalloonText)
    {
        delete[] m_pszBalloonText;
        m_pszBalloonText = NULL;
    }

    if (pszText == NULL)
        pszText = L"";

    size_t cch = wcslen(pszText);
    m_pszBalloonText = new(cicNoThrow) WCHAR[cch + 1];
    if (m_pszBalloonText)
        lstrcpynW(m_pszBalloonText, pszText, cch + 1);

    AdjustPos();
}

void
CUIFBalloonWindow::InitWindowRegion()
{
    RECT rc;
    GetRect(&rc);
    m_hRgn = CreateRegion(&rc);
    if (m_hRgn)
        ::SetWindowRgn(m_hWnd, m_hRgn, TRUE);
}

void
CUIFBalloonWindow::LayoutObject()
{
    SIZE size;
    GetButtonSize(&size);

    RECT rc;
    GetRect(&rc);

    switch (m_nBalloonType)
    {
        case 1:
            rc.top += 16;
            break;
        case 2:
            rc.right -= 16;
            break;
        case 3:
            rc.left += 16;
            break;
        default:
            rc.bottom -= 16;
            break;
    }

    RECT rcMargin;
    GetMargin(&rcMargin);
    rc.left += rcMargin.left;
    rc.top += rcMargin.top;
    rc.right -= rcMargin.right;
    rc.bottom -= rcMargin.bottom;

    LONG xLeft = (rc.left + rc.right - size.cx * (((m_cButtons - 1) / 2) - m_cButtons)) / 2;
    for (UINT iButton = 0; iButton < m_cButtons; ++iButton)
    {
        CUIFObject *UIObject = FindUIObject(iButton);
        if (!UIObject)
            continue;

        rcMargin.left = xLeft + iButton * (size.cx * 3 / 2);
        rcMargin.top = rc.bottom - size.cy;
        rcMargin.right = rcMargin.left + size.cx;
        rcMargin.bottom = rc.bottom;

        UIObject->SetRect(&rcMargin);
        UIObject->Show(TRUE);
    }
}

void
CUIFBalloonWindow::PaintFrameProc(HDC hDC, LPCRECT prc)
{
    HRGN hRgn = CreateRegion(prc);
    HBRUSH hbrBack = ::CreateSolidBrush(GetBalloonBkColor());
    HBRUSH hbrFrame = ::CreateSolidBrush(::GetSysColor(COLOR_WINDOWFRAME));
    ::FillRgn(hDC, hRgn, hbrBack);
    ::FrameRgn(hDC, hRgn, hbrFrame, 1, 1);
    ::DeleteObject(hbrBack);
    ::DeleteObject(hbrFrame);
    ::DeleteObject(hRgn);
}

void
CUIFBalloonWindow::PaintMessageProc(HDC hDC, LPRECT prc, LPCWSTR pszText)
{
    HGDIOBJ hFontOld = ::SelectObject(hDC, m_hFont);
    COLORREF rgbOldColor = ::SetTextColor(hDC, GetBalloonTextColor());
    INT nOldBkMode = ::SetBkMode(hDC, TRANSPARENT);
    ::DrawTextW(hDC, pszText, -1, prc, DT_WORDBREAK);
    ::SelectObject(hDC, hFontOld);
    ::SetTextColor(hDC, rgbOldColor);
    ::SetBkMode(hDC, nOldBkMode);
}

void
CUIFBalloonWindow::SendNotification(WPARAM wParam)
{
    if (m_hwndNotif)
        ::PostMessage(m_hwndNotif, m_uNotifMsg, wParam, 0);
}

/////////////////////////////////////////////////////////////////////////////
// CUIFMenu

CUIFMenu::CUIFMenu(
    HINSTANCE hInst,
    DWORD style,
    DWORD dwUnknown14) : CUIFWindow(hInst, style)
{
    m_nSelectedID = -1;
    m_dwUnknown14 = dwUnknown14;
    SetMenuFont();
}

CUIFMenu::~CUIFMenu()
{
    for (size_t iItem = 0; iItem < m_MenuItems.size(); ++iItem)
        delete m_MenuItems[iItem];

    ::DeleteObject(m_hMenuFont);
    ClearMenuFont();
}

void CUIFMenu::CancelMenu()
{
    if (m_pVisibleSubMenu)
    {
        UninitShow();
    }
    else if (m_bModal)
    {
        SetSelectedId(0xFFFFFFFF);
        ::PostMessage(m_hWnd, WM_NULL, 0, 0);
    }
}

void CUIFMenu::ClearMenuFont()
{
    SetFont(NULL);
    ::DeleteObject(m_hFont);
}

CUIFMenuItem*
CUIFMenu::GetNextItem(CUIFMenuItem *pItem)
{
    size_t iItem, cItems = m_MenuItems.size();

    if (cItems == 0)
        return NULL;

    if (!m_pSelectedItem)
        return m_MenuItems[0];

    for (iItem = 0; iItem < cItems; )
    {
        if (m_MenuItems[iItem++] == pItem)
            break;
    }

    for (;;)
    {
        if (iItem == cItems)
            iItem = 0;
        if (!m_MenuItems[iItem] || !m_MenuItems[iItem]->m_bMenuItemDisabled)
            break;
        ++iItem;
    }

    return m_MenuItems[iItem];
}

CUIFMenuItem*
CUIFMenu::GetPrevItem(CUIFMenuItem *pItem)
{
    ptrdiff_t iItem, cItems = m_MenuItems.size();

    if (cItems == 0)
        return NULL;

    if (!m_pSelectedItem)
        return m_MenuItems[cItems - 1];

    for (iItem = cItems - 1; iItem >= 0; )
    {
        if (m_MenuItems[iItem--] == pItem)
            break;
    }

    for (;;)
    {
        if (iItem < 0)
            iItem = cItems - 1;
        if (!m_MenuItems[iItem] || !m_MenuItems[iItem]->m_bMenuItemDisabled)
            break;
        --iItem;
    }

    return m_MenuItems[iItem];
}

CUIFMenu*
CUIFMenu::GetTopSubMenu()
{
    CUIFMenu *pMenu;
    for (pMenu = this; pMenu->m_pParentMenu; pMenu = pMenu->m_pParentMenu)
        ;
    return pMenu;
}

void CUIFMenu::HandleMouseMsg(UINT uMsg, LONG x, LONG y)
{
    POINT pt = { x, y };
    if (!::PtInRect(&m_rc, pt) &&
        (uMsg == WM_LBUTTONDOWN || uMsg == WM_RBUTTONDOWN || uMsg == WM_LBUTTONUP || uMsg == WM_RBUTTONUP))
    {
        SetSelectedId(-1);
        PostMessage(m_hWnd, WM_NULL, 0, 0);
    }
    CUIFWindow::HandleMouseMsg(uMsg, x, y);
}

STDMETHODIMP_(BOOL)
CUIFMenu::InitShow(CUIFWindow *pWindow, LPCRECT prc, BOOL bFlag, BOOL bDoAnimation)
{
    HWND hWnd = NULL;
    if (pWindow)
        hWnd = *pWindow;

    CreateWnd(hWnd);

    m_bHasMargin = FALSE;

    for (size_t iItem = 0; iItem < m_MenuItems.size(); ++iItem)
    {
        if (m_MenuItems[iItem])
            m_MenuItems[iItem]->InitMenuExtent();
    }

    INT cxMax = 0;

    m_cxMenuExtent = 0;
    for (size_t iItem = 0; iItem < m_MenuItems.size(); ++iItem)
    {
        CUIFMenuItem *pItem = m_MenuItems[iItem];
        if (!pItem)
            continue;

        INT cxItem = m_cxyMargin + pItem->m_MenuLeftExtent.cx;
        if (cxMax < cxItem)
            cxMax = cxItem;
        m_cxMenuExtent = max(m_cxMenuExtent, pItem->m_MenuRightExtent.cx);
        if (!m_bHasMargin && pItem->m_hbmColor && pItem->IsCheck())
            m_bHasMargin = TRUE;
    }

    RECT rcItem = { 0, 0, 0, 0 };
    for (size_t iItem = 0; iItem < m_MenuItems.size(); ++iItem)
    {
        CUIFMenuItem *pItem = m_MenuItems[iItem];
        if (!pItem)
            continue;

        INT cyItem = pItem->m_MenuLeftExtent.cy;
        rcItem.right = rcItem.left + cxMax + m_cxMenuExtent;
        rcItem.bottom = rcItem.top + cyItem;
        pItem->SetRect(&rcItem);
        rcItem.top += cyItem;
        AddUIObj(pItem);
    }

    rcItem.top = 0;
    DWORD style = ::GetWindowLongPtr(hWnd, GWL_STYLE);
    cxMax = rcItem.right;
    INT cyMax = rcItem.bottom;
    if (style & WS_DLGFRAME)
    {
        cxMax = rcItem.right + 2 * ::GetSystemMetrics(SM_CXDLGFRAME);
        cyMax += 2 * ::GetSystemMetrics(SM_CYDLGFRAME);
    }
    else if (style & WS_BORDER)
    {
        cxMax = rcItem.right + 2 * ::GetSystemMetrics(SM_CXBORDER);
        cyMax += 2 * ::GetSystemMetrics(SM_CYBORDER);
    }

    RECT rc = { 0, 0, ::GetSystemMetrics(SM_CXSCREEN), ::GetSystemMetrics(SM_CYSCREEN) };

    RECT rc3 = *prc;
    HMONITOR hMon = ::MonitorFromRect(&rc3, MONITOR_DEFAULTTONEAREST);
    if (hMon)
    {
        MONITORINFO mi = { sizeof(mi) };
        if (::GetMonitorInfo(hMon, &mi))
            rc = mi.rcMonitor;
    }

    if (m_style & 0x200)
        rcItem.left -= cxMax;

    INT x, y;
    DWORD dwFlags2 = 0;

    if (bFlag)
    {
        INT bottom = prc->bottom;
        x = rcItem.left + prc->left;
        if (rcItem.top + bottom + cyMax > rc.bottom)
        {
            bottom = prc->top - cyMax;
            dwFlags2 = 8;
        }
        else
        {
            dwFlags2 = 4;
        }

        y = rcItem.top + bottom;

        if (rcItem.left + cxMax + prc->right > rc.right)
            x = rc.right - cxMax;
    }
    else
    {
        y = rcItem.top + prc->top;
        if (rcItem.left + prc->right + cxMax > rc.right)
        {
            x = rcItem.left + prc->left - cxMax;
            dwFlags2 = 2;
        }
        else
        {
            x = rcItem.left + prc->right;
            dwFlags2 = 1;
        }
        if (rcItem.top + cyMax + prc->bottom > rc.bottom)
            y = rc.bottom - cyMax;
    }

    if (x > rc.right - cxMax)
        x = rc.right - cxMax;
    if (x < rc.left)
        x = rc.left;
    if (y > rc.bottom - cyMax)
        y = rc.bottom - cyMax;
    if (y < rc.top)
        y = rc.top;

    Move(x, y, cxMax, -1);

    SetRect(NULL);

    BOOL bAnimation = FALSE;
    if (bDoAnimation &&
        ::SystemParametersInfo(SPI_GETMENUANIMATION, 0, &bAnimation, 0) && bAnimation)
    {
        BOOL bMenuFade = FALSE;
        if (!::SystemParametersInfoA(SPI_GETMENUFADE, 0, &bMenuFade, 0))
            bMenuFade = FALSE;

        DWORD dwFlags = (bMenuFade ? 0x80000 : dwFlags2) | 0x40000;
        if (!AnimateWnd(200, dwFlags))
            Show(TRUE);
    }
    else
    {
        Show(TRUE);
    }

    if (m_pVisibleSubMenu)
        m_pVisibleSubMenu->m_pParentMenu = this;

    return TRUE;
}

BOOL CUIFMenu::InsertItem(CUIFMenuItem *pItem)
{
    if (!m_MenuItems.Add(pItem))
        return FALSE;

    pItem->SetFont(m_hFont);
    return TRUE;
}

BOOL CUIFMenu::InsertSeparator()
{
    CUIFMenuItemSeparator *pSep = new(cicNoThrow) CUIFMenuItemSeparator(this);
    if (!pSep)
        return FALSE;

    if (!m_MenuItems.Add(pSep))
    {
        delete pSep;
        return FALSE;
    }

    pSep->Initialize();
    return TRUE;
}

STDMETHODIMP_(void)
CUIFMenu::ModalMessageLoop()
{
    MSG msg;

    while (::GetMessage(&msg, 0, 0, 0) && msg.message != WM_NULL &&
           (msg.hwnd == m_hWnd || msg.message <= WM_MOUSEFIRST || WM_MOUSELAST <= msg.message))
    {
        if (!msg.hwnd && WM_KEYFIRST <= msg.message && msg.message <= WM_KEYLAST)
            msg.hwnd = GetTopSubMenu()->m_hWnd;
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
    }
}

STDMETHODIMP_(void)
CUIFMenu::ModalMouseNotify(UINT uMsg, LONG x, LONG y)
{
    if (uMsg == WM_LBUTTONDOWN || uMsg == WM_RBUTTONDOWN)
        CancelMenu();
}

STDMETHODIMP_(void)
CUIFMenu::OnKeyDown(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    CUIFMenuItem *pTargetItem;

    BYTE vKey = (BYTE)wParam;

    switch (vKey)
    {
        case VK_ESCAPE:
            CancelMenu();
            return;

        case VK_LEFT:
            if (!m_pVisibleSubMenu)
                return;

            CancelMenu();
            return;

        case VK_RIGHT:
            if (m_pSelectedItem && m_pSelectedItem->m_pSubMenu)
            {
                m_pSelectedItem->ShowSubPopup();
                CUIFMenu *pSubMenu = m_pSelectedItem->m_pSubMenu;
                pTargetItem = pSubMenu->GetNextItem(NULL);
                pSubMenu->SetSelectedItem(pTargetItem);
            }
            return;

        case VK_UP:
            pTargetItem = GetPrevItem(m_pSelectedItem);
            SetSelectedItem(pTargetItem);
            return;

        case VK_DOWN:
            pTargetItem = GetNextItem(m_pSelectedItem);
            SetSelectedItem(pTargetItem);
            return;

        case VK_RETURN:
            break;

        default:
        {
            if (!(('A' <= vKey && vKey <= 'Z') || ('0' <= vKey && vKey <= '9')))
                return;

            size_t iItem;
            for (iItem = 0; iItem < m_MenuItems.size(); ++iItem)
            {
                CUIFMenuItem *pItem = m_MenuItems[iItem];
                if (pItem->m_nMenuItemVKey == vKey)
                {
                    SetSelectedItem(pItem);
                    break;
                }
            }

            if (iItem == m_MenuItems.size())
                return;
        }
    }

    if (m_pSelectedItem && !m_pSelectedItem->m_bMenuItemGrayed)
    {
        CUIFMenu *pSubMenu = m_pSelectedItem->m_pSubMenu;
        if (pSubMenu)
        {
            m_pSelectedItem->ShowSubPopup();
            pTargetItem = pSubMenu->GetNextItem(NULL);
            pSubMenu->SetSelectedItem(pTargetItem);
        }
        else
        {
            SetSelectedId(m_pSelectedItem->m_nMenuItemID);
            ::PostMessage(m_hWnd, WM_NULL, 0, 0);
        }
    }
}

void CUIFMenu::PostKey(BOOL bUp, WPARAM wParam, LPARAM lParam)
{
    if (m_bModal)
    {
        // NOTE: hWnd parameter will be populated in CUIFMenu::ModalMessageLoop.
        if (bUp)
            ::PostMessage(NULL, WM_KEYUP, wParam, lParam);
        else
            ::PostMessage(NULL, WM_KEYDOWN, wParam, lParam);
    }
}

void CUIFMenu::SetMenuFont()
{
    LONG height = 14;

    NONCLIENTMETRICS ncm = { sizeof(ncm) };
    if (::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0))
    {
        HFONT hFont = ::CreateFontIndirect(&ncm.lfMenuFont);
        SetFont(hFont);

        LONG lfHeight = ncm.lfMenuFont.lfHeight;
        if (lfHeight < 0)
            lfHeight = -lfHeight;
        height = (ncm.iMenuHeight + lfHeight) / 2;
    }

    m_hMenuFont = ::CreateFontW(height, 0, 0, 0, FW_NORMAL, 0, 0, 0, SYMBOL_CHARSET,
                                0, 0, 0, 0, L"Marlett");
    INT cxSmallIcon = ::GetSystemMetrics(SM_CXSMICON);
    m_cxyMargin = max(height, cxSmallIcon) + 2;
}

void CUIFMenu::SetSelectedId(UINT nSelectID)
{
    CUIFMenu *pMenu = this;

    while (pMenu->m_pVisibleSubMenu)
        pMenu = pMenu->m_pVisibleSubMenu;

    pMenu->m_nSelectedID = nSelectID;
}

void CUIFMenu::SetSelectedItem(CUIFMenuItem *pItem)
{
    CUIFMenuItem *pOldItem = m_pSelectedItem;
    if (pOldItem == pItem)
        return;

    m_pSelectedItem = pItem;
    if (pOldItem)
        pOldItem->CallOnPaint();
    if (m_pSelectedItem)
        m_pSelectedItem->CallOnPaint();
}

UINT CUIFMenu::ShowModalPopup(CUIFWindow *pWindow, LPCRECT prc, BOOL bFlag)
{
    CUIFObject *pCaptured = NULL;
    if (pWindow)
    {
        pCaptured = pWindow->m_pCaptured;
        CUIFWindow::SetCaptureObject(NULL);
    }

    UINT nSelectedID = -1;
    if (InitShow(pWindow, prc, bFlag, TRUE))
    {
        m_bModal = TRUE;
        pWindow->SetBehindModal(this);
        ModalMessageLoop();
        nSelectedID = m_nSelectedID;
        pWindow->SetBehindModal(NULL);
        m_bModal = FALSE;
    }

    UninitShow();

    if (pWindow)
        pWindow->SetCaptureObject(pCaptured);

    return nSelectedID;
}

void CUIFMenu::ShowSubPopup(CUIFMenu *pSubMenu, LPCRECT prc, BOOL bFlag)
{
    m_pVisibleSubMenu = pSubMenu;
    InitShow(pSubMenu, prc, bFlag, TRUE);
}

STDMETHODIMP_(BOOL)
CUIFMenu::UninitShow()
{
    if (m_pParentMenu)
        m_pParentMenu->UninitShow();

    Show(FALSE);

    if (m_pVisibleSubMenu)
        m_pVisibleSubMenu->m_pParentMenu = NULL;

    for (size_t iItem = 0; iItem < m_MenuItems.size(); ++iItem)
        RemoveUIObj(m_MenuItems[iItem]);

    ::DestroyWindow(m_hWnd);
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CUIFMenuItem

CUIFMenuItem::CUIFMenuItem(
    CUIFMenu *pMenu,
    BOOL bDisabled) : CUIFObject(pMenu, 0, NULL, 0)
{
    m_ichMenuItemPrefix = -1;
    m_nMenuItemID = 0;
    m_pszMenuItemLeft = m_pszMenuItemRight = NULL;
    m_cchMenuItemLeft = m_cchMenuItemRight = 0;
    m_nMenuItemVKey = 0;
    m_hbmColor = m_hbmMask = NULL;
    m_bMenuItemChecked = m_bMenuItemGrayed = FALSE;
    m_bMenuItemDisabled = bDisabled;
    m_pMenu = pMenu;
}

CUIFMenuItem::~CUIFMenuItem()
{
    if (m_pszMenuItemLeft)
    {
        delete[] m_pszMenuItemLeft;
        m_pszMenuItemLeft = NULL;
    }

    if (m_pszMenuItemRight)
    {
        delete[] m_pszMenuItemRight;
        m_pszMenuItemRight = NULL;
    }

    if (m_pSubMenu)
    {
        delete m_pSubMenu;
        m_pSubMenu = NULL;
    }
}

BOOL CUIFMenuItem::Init(UINT nMenuItemID, LPCWSTR pszText)
{
    m_nMenuItemID = nMenuItemID;

    if (!pszText)
    {
        m_pszMenuItemLeft = NULL;
        m_cchMenuItemLeft = 0;
        return TRUE;
    }

    INT cch = lstrlenW(pszText);
    m_pszMenuItemLeft = new(cicNoThrow) WCHAR[cch + 1];
    if (!m_pszMenuItemLeft)
        return FALSE;

    const WCHAR *pch0 = pszText;
    INT ich1, ich2;
    for (ich1 = 0; *pch0 && *pch0 != L'\t'; ++ich1, ++pch0)
    {
        if (*pch0 == L'&' && *++pch0 != L'&')
        {
            m_nMenuItemVKey = ::VkKeyScanW(*pch0);
            if (!m_nMenuItemVKey)
                m_nMenuItemVKey = (BYTE)VkKeyScanA(*(BYTE*)pch0);
            m_ichMenuItemPrefix = ich1;
        }
        m_pszMenuItemLeft[ich1] = *pch0;
    }
    m_pszMenuItemLeft[ich1] = 0;
    m_cchMenuItemLeft = lstrlenW(m_pszMenuItemLeft);

    if (*pch0 == L'\t')
    {
        m_cchMenuItemRight = 0;
        m_pszMenuItemRight = new(cicNoThrow) WCHAR[cch + 1];
        if (m_pszMenuItemRight)
        {
            ++pch0;
            WCHAR wch = *pch0;
            for (ich2 = 0; *pch0; ++ich2)
            {
                m_pszMenuItemRight[ich2] = wch;
                wch = *++pch0;
            }
            m_pszMenuItemRight[ich2] = 0;
            m_cchMenuItemRight = lstrlenW(m_pszMenuItemRight);
        }
    }

    return TRUE;
}

STDMETHODIMP_(void)
CUIFMenuItem::InitMenuExtent()
{
    if (!m_pszMenuItemLeft)
    {
        if (m_hbmColor)
        {
            BITMAP bm;
            ::GetObject(m_hbmColor, sizeof(bm), &bm);
            m_MenuLeftExtent.cx = bm.bmWidth + 2;
            m_MenuLeftExtent.cy = bm.bmHeight + 4;
        }
        else
        {
            m_MenuLeftExtent.cx = m_MenuLeftExtent.cy = 0;
        }
        return;
    }

    HDC hDC = ::GetDC(*m_pWindow);

    HGDIOBJ hFontOld = ::SelectObject(hDC, m_hFont);
    ::GetTextExtentPoint32W(hDC, m_pszMenuItemLeft, m_cchMenuItemLeft, &m_MenuLeftExtent);
    m_MenuLeftExtent.cx += 16;
    m_MenuLeftExtent.cy += 8;
    if (m_pszMenuItemRight)
    {
        ::GetTextExtentPoint32W(hDC, m_pszMenuItemRight, m_cchMenuItemRight, &m_MenuRightExtent);
        m_MenuRightExtent.cy += 8;
    }
    ::SelectObject(hDC, hFontOld);

    if (m_pSubMenu)
        m_MenuLeftExtent.cx += m_MenuLeftExtent.cy + 2;
    if (m_pMenu->m_style & UIF_MENU_USE_OFF10)
        m_MenuLeftExtent.cx += 24;

    ::ReleaseDC(*m_pWindow, hDC);
}

BOOL CUIFMenuItem::IsCheck()
{
    return m_bMenuItemChecked || m_bMenuItemForceChecked;
}

void CUIFMenuItem::DrawArrow(HDC hDC, INT xLeft, INT yTop)
{
    if (!m_pSubMenu)
        return;

    HGDIOBJ hFontOld = ::SelectObject(hDC, m_pMenu->m_hMenuFont);
    ::TextOutW(hDC, xLeft, yTop, L"4", 1); // rightward triangle
    ::SelectObject(hDC, hFontOld);
}

void CUIFMenuItem::DrawBitmapProc(HDC hDC, INT xLeft, INT yTop)
{
    if (!m_pScheme || !m_hbmColor)
        return;

    BITMAP bm;
    ::GetObject(m_hbmColor, sizeof(bm), &bm);

    INT width = m_pMenu->m_cxyMargin, height = m_rc.bottom - m_rc.top;
    if (width > bm.bmWidth)
    {
        width = bm.bmWidth;
        xLeft += (width - bm.bmWidth) / 2;
    }
    if (height > bm.bmHeight)
    {
        height = bm.bmHeight;
        yTop += (height - bm.bmHeight) / 2;
    }

    RECT rc = { xLeft, yTop, xLeft + width, yTop + height };

    if (IsRTL())
        m_pScheme->m_bMirroring = TRUE;

    if (m_pMenu->m_pSelectedItem != this || m_bMenuItemDisabled)
        m_pScheme->DrawFrameCtrlBitmap(hDC, &rc, m_hbmColor, m_hbmMask, 0);
    else
        m_pScheme->DrawFrameCtrlBitmap(hDC, &rc, m_hbmColor, m_hbmMask, UIF_DRAW_PRESSED);

    if (IsRTL())
        m_pScheme->m_bMirroring = FALSE;
}

void CUIFMenuItem::DrawCheck(HDC hDC, INT xLeft, INT yTop)
{
    if (!IsCheck())
        return;

    HGDIOBJ hFontOld = ::SelectObject(hDC, m_pMenu->m_hMenuFont);
    WCHAR wch = (m_bMenuItemChecked ? L'a' : L'h'); // checkmark or bullet
    ::TextOutW(hDC, xLeft, yTop, &wch, 1);
    ::SelectObject(hDC, hFontOld);
}

void
CUIFMenuItem::DrawUnderline(HDC hDC, INT xText, INT yText, HBRUSH hbr)
{
    if (m_ichMenuItemPrefix > m_cchMenuItemLeft)
        return;

    SIZE PrePrefixSize, PostPrefixSize;
    ::GetTextExtentPoint32W(hDC, m_pszMenuItemLeft, m_ichMenuItemPrefix, &PrePrefixSize);
    ::GetTextExtentPoint32W(hDC, m_pszMenuItemLeft, m_ichMenuItemPrefix + 1, &PostPrefixSize);

    BOOL bHeadPrefix = (m_ichMenuItemPrefix == 0);

    RECT rc;
    rc.left   = xText + PrePrefixSize.cx + !bHeadPrefix;
    rc.right  = xText + PostPrefixSize.cx;
    rc.top    = (yText + PostPrefixSize.cy) - 1;
    rc.bottom = yText + PostPrefixSize.cy;
    ::FillRect(hDC, &rc, hbr);
}

STDMETHODIMP_(void)
CUIFMenuItem::OnLButtonUp(LONG x, LONG y)
{
    if (!m_bMenuItemGrayed && !m_bMenuItemDisabled && !m_pSubMenu)
    {
        m_pMenu->SetSelectedId(m_nMenuItemID);
        ::PostMessage(*m_pWindow, WM_NULL, 0, 0);
    }
}

STDMETHODIMP_(void)
CUIFMenuItem::OnMouseIn(LONG x, LONG y)
{
    if (m_pMenu->m_pParentMenu)
        m_pMenu->m_pParentMenu->CancelMenu();

    if (m_pSubMenu)
    {
        DWORD Delay;
        if (!::SystemParametersInfo(SPI_GETMENUSHOWDELAY, 0, &Delay, 0))
            Delay = 300;

        CUIFObject::StartTimer(Delay);
    }

    m_pMenu->SetSelectedItem(this);
}

STDMETHODIMP_(void)
CUIFMenuItem::OnPaint(HDC hDC)
{
    if (m_pMenu->m_style & UIF_MENU_USE_OFF10)
        OnPaintO10(hDC);
    else
        OnPaintDef(hDC);
}

STDMETHODIMP_(void)
CUIFMenuItem::OnPaintO10(HDC hDC)
{
    if (!m_pScheme)
        return;

    HGDIOBJ hFontOld = ::SelectObject(hDC, m_hFont);

    SIZE textSize;
    ::GetTextExtentPoint32W(hDC, m_pszMenuItemLeft, m_cchMenuItemLeft, &textSize);

    LONG cySpace = m_rc.bottom - m_rc.top - textSize.cy;
    LONG xCheck = m_rc.left, yCheck = m_rc.top + cySpace / 2;
    LONG cxyMargin = (m_pMenu->m_bHasMargin ? m_pMenu->m_cxyMargin : 0);

    LONG xBitmap = m_rc.left + cxyMargin, yBitmap = m_rc.top;
    LONG xText = m_rc.left + m_pMenu->m_cxyMargin + cxyMargin + 8;
    LONG yText = m_rc.top + cySpace / 2;
    LONG xArrow = m_rc.left - textSize.cy + m_rc.right - 2;
    LONG xRightText = m_rc.right - m_pMenu->m_cxMenuExtent - 8;

    RECT rc;
    GetRect(&rc);

    if (m_bMenuItemDisabled || m_pMenu->m_pSelectedItem != this)
    {
        rc.right = m_pMenu->m_cxyMargin + rc.left + 2;
        if (m_pMenu->m_bHasMargin)
            rc.right += m_pMenu->m_cxyMargin;

        ::FillRect(hDC, &rc, m_pScheme->GetBrush(9));
    }
    else
    {
        m_pScheme->DrawCtrlBkgd(hDC, &rc, 0, UIF_DRAW_PRESSED);
        m_pScheme->DrawCtrlEdge(hDC, &rc, 0, UIF_DRAW_PRESSED);
    }

    ::SetBkMode(hDC, TRANSPARENT);

    if (m_bMenuItemGrayed)
    {
        ::SetTextColor(hDC, m_pScheme->GetColor(11));
        ::ExtTextOutW(hDC, xText, yText, ETO_CLIPPED, &m_rc, m_pszMenuItemLeft,
                      m_cchMenuItemLeft, NULL);
    }
    else if (m_bMenuItemDisabled || m_pMenu->m_pSelectedItem != this)
    {
        ::SetTextColor(hDC, m_pScheme->GetColor(10));
        ::ExtTextOutW(hDC, xText, yText, ETO_CLIPPED, &m_rc, m_pszMenuItemLeft,
                      m_cchMenuItemLeft, NULL);
    }
    else
    {
        ::SetTextColor(hDC, m_pScheme->GetColor(5));
        ::ExtTextOutW(hDC, xText, yText, ETO_CLIPPED, &m_rc, m_pszMenuItemLeft,
                      m_cchMenuItemLeft, NULL);
    }

    DrawUnderline(hDC, xText, yText, m_pScheme->GetBrush(5));

    if (m_pszMenuItemRight)
    {
        ::ExtTextOutW(hDC, xRightText, yText, ETO_CLIPPED, &m_rc, m_pszMenuItemRight,
                      m_cchMenuItemRight, NULL);
    }

    DrawCheck(hDC, xCheck, yCheck);
    DrawBitmapProc(hDC, xBitmap, yBitmap);
    DrawArrow(hDC, xArrow, yText);

    ::SelectObject(hDC, hFontOld);
}

STDMETHODIMP_(void)
CUIFMenuItem::OnPaintDef(HDC hDC)
{
    HGDIOBJ hFontOld = ::SelectObject(hDC, m_hFont);

    SIZE textSize;
    ::GetTextExtentPoint32W(hDC, m_pszMenuItemLeft, m_cchMenuItemLeft, &textSize);

    LONG cxyMargin = (m_pMenu->m_bHasMargin ? m_pMenu->m_cxyMargin : 0);

    LONG cySpace = m_rc.bottom - m_rc.top - textSize.cy;
    LONG xCheck = m_rc.left, yCheck = m_rc.top + cySpace / 2;
    LONG xBitmap = m_rc.left + cxyMargin, yBitmap = m_rc.top;
    LONG xText = m_rc.left + cxyMargin + m_pMenu->m_cxyMargin + 2;
    LONG yText = m_rc.top + cySpace / 2;

    LONG xArrow = m_rc.right + m_rc.left - 10;

    ::SetBkMode(hDC, TRANSPARENT);

    if (m_bMenuItemGrayed)
    {
        UINT uOptions = ETO_CLIPPED;
        if (m_bMenuItemDisabled || m_pMenu->m_pSelectedItem != this)
        {
            ::SetTextColor(hDC, ::GetSysColor(COLOR_BTNHIGHLIGHT));
            ::ExtTextOutW(hDC, xText + 1, yText + 1, ETO_CLIPPED, &m_rc, m_pszMenuItemLeft,
                          m_cchMenuItemLeft, NULL);
            DrawCheck(hDC, xCheck + 1, yCheck + 1);
            DrawBitmapProc(hDC, xBitmap + 1, yBitmap + 1);
            DrawArrow(hDC, xArrow + 1, yText + 1);
        }
        else
        {
            ::SetBkColor(hDC, ::GetSysColor(COLOR_HIGHLIGHT));
            uOptions = ETO_CLIPPED | ETO_OPAQUE;
        }
        ::SetTextColor(hDC, ::GetSysColor(COLOR_BTNSHADOW));
        ::ExtTextOutW(hDC, xText, yText, uOptions, &m_rc, m_pszMenuItemLeft,
                      m_cchMenuItemLeft, NULL);
        DrawUnderline(hDC, xText, yText, (HBRUSH)UlongToHandle(COLOR_BTNSHADOW + 1));
    }
    else if (m_bMenuItemDisabled || m_pMenu->m_pSelectedItem != this)
    {
        ::SetTextColor(hDC, ::GetSysColor(COLOR_MENUTEXT));
        ::ExtTextOutW(hDC, xText, yText, ETO_CLIPPED, &m_rc, m_pszMenuItemLeft,
                      m_cchMenuItemLeft, NULL);
        DrawUnderline(hDC, xText, yText, (HBRUSH)UlongToHandle(COLOR_MENUTEXT + 1));
    }
    else
    {
        ::SetTextColor(hDC, ::GetSysColor(COLOR_HIGHLIGHTTEXT));
        ::SetBkColor(hDC, ::GetSysColor(COLOR_HIGHLIGHT));
        ::ExtTextOutW(hDC, xText, yText, ETO_CLIPPED | ETO_OPAQUE, &m_rc,
                      m_pszMenuItemLeft, m_cchMenuItemLeft, NULL);
        DrawUnderline(hDC, xText, yText, (HBRUSH)UlongToHandle(COLOR_HIGHLIGHTTEXT + 1));
    }

    DrawCheck(hDC, xCheck, yCheck);
    DrawBitmapProc(hDC, xBitmap, yBitmap);
    DrawArrow(hDC, xArrow, yText);

    ::SelectObject(hDC, hFontOld);
}

STDMETHODIMP_(void)
CUIFMenuItem::OnTimer()
{
    CUIFObject::EndTimer();
    if (m_pMenu->m_pPointed == this)
        ShowSubPopup();
}

void CUIFMenuItem::SetBitmapMask(HBITMAP hbmMask)
{
    m_hbmMask = hbmMask;
    CallOnPaint();
}

void CUIFMenuItem::ShowSubPopup()
{
    RECT rc = m_rc;
    ::ClientToScreen(*m_pWindow, (LPPOINT)&rc);
    ::ClientToScreen(*m_pWindow, (LPPOINT)&rc.right);
    m_pSubMenu->ShowSubPopup(m_pMenu, &rc, FALSE);
}

/////////////////////////////////////////////////////////////////////////////
// CUIFMenuItemSeparator

CUIFMenuItemSeparator::CUIFMenuItemSeparator(CUIFMenu *pMenu) : CUIFMenuItem(pMenu, TRUE)
{
    m_nMenuItemID = -1;
}

STDMETHODIMP_(void)
CUIFMenuItemSeparator::InitMenuExtent()
{
    m_MenuLeftExtent.cx = 0;
    m_MenuLeftExtent.cy = 6;
}

STDMETHODIMP_(void)
CUIFMenuItemSeparator::OnPaintDef(HDC hDC)
{
    if (!m_pScheme)
        return;

    RECT rc;
    rc.left   = m_rc.left + 2;
    rc.top    = m_rc.top + (m_rc.bottom - m_rc.top - 2) / 2;
    rc.right  = m_rc.right - 2;
    rc.bottom = rc.top + 2;
    m_pScheme->DrawMenuSeparator(hDC, &rc);
}

STDMETHODIMP_(void)
CUIFMenuItemSeparator::OnPaintO10(HDC hDC)
{
    if (!m_pScheme)
        return;

    LONG cx = m_rc.right - m_rc.left - 4;
    LONG cy = (m_rc.bottom - m_rc.top - 2) / 2;

    RECT rc;
    GetRect(&rc);

    rc.right = rc.left + m_pMenu->m_cxyMargin + 2;
    if (m_pMenu->m_bHasMargin)
        rc.right += m_pMenu->m_cxyMargin;

    HBRUSH hBrush = m_pScheme->GetBrush(9);
    ::FillRect(hDC, &rc, hBrush);
    rc = {
        m_rc.left + m_pMenu->m_cxyMargin + 4,
        m_rc.top + cy,
        m_rc.left + cx + 2,
        m_rc.top + cy + 1
    };
    m_pScheme->DrawMenuSeparator(hDC, &rc);
}

/////////////////////////////////////////////////////////////////////////////

void cicGetWorkAreaRect(POINT pt, LPRECT prc)
{
    ::SystemParametersInfo(SPI_GETWORKAREA, 0, prc, 0);

    HMONITOR hMon = ::MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
    if (hMon)
    {
        MONITORINFO mi = { sizeof(mi) };
        if (::GetMonitorInfo(hMon, &mi))
            *prc = mi.rcWork;
    }
}

void cicGetScreenRect(POINT pt, LPRECT prc)
{
    *prc = { 0, 0, ::GetSystemMetrics(SM_CXSCREEN), ::GetSystemMetrics(SM_CYSCREEN) };
    HMONITOR hMon = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
    if (hMon)
    {
        MONITORINFO mi = { sizeof(mi) };
        GetMonitorInfo(hMon, &mi);
            *prc = mi.rcMonitor;
    }
}

BOOL cicIsFullScreenSize(HWND hWnd)
{
    RECT rc;

    ::GetWindowRect(hWnd, &rc);
    return (rc.left <= 0) && (rc.top <= 0) &&
           (rc.right >= GetSystemMetrics(SM_CXFULLSCREEN)) &&
           (rc.bottom >= GetSystemMetrics(SM_CYFULLSCREEN));
}

BOOL cicGetIconSize(HICON hIcon, LPSIZE pSize)
{
    ICONINFO IconInfo;
    if (!GetIconInfo(hIcon, &IconInfo))
        return FALSE;

    BITMAP bm;
    ::GetObject(IconInfo.hbmColor, sizeof(bm), &bm);
    ::DeleteObject(IconInfo.hbmColor);
    ::DeleteObject(IconInfo.hbmMask);
    pSize->cx = bm.bmWidth;
    pSize->cy = bm.bmHeight;
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////

void cicInitUIFSys(void)
{
    CUIFSystemInfo::s_pSystemInfo = new(cicNoThrow) CUIFSystemInfo();
    if (CUIFSystemInfo::s_pSystemInfo)
        CUIFSystemInfo::s_pSystemInfo->Initialize();
}

void cicDoneUIFSys(void)
{
    if (CUIFSystemInfo::s_pSystemInfo)
    {
        delete CUIFSystemInfo::s_pSystemInfo;
        CUIFSystemInfo::s_pSystemInfo = NULL;
    }
}

void cicUpdateUIFSys(void)
{
    if (CUIFSystemInfo::s_pSystemInfo)
        CUIFSystemInfo::s_pSystemInfo->GetSystemMetrics();
}

/////////////////////////////////////////////////////////////////////////////

void cicInitUIFScheme(void)
{
    CUIFColorTable *pColorTable;

    pColorTable = CUIFScheme::s_pColorTableSys = new(cicNoThrow) CUIFColorTableSys();
    if (pColorTable)
    {
        pColorTable->InitColor();
        pColorTable->InitBrush();
    }

    pColorTable = CUIFScheme::s_pColorTableOff10 = new(cicNoThrow) CUIFColorTableOff10();
    if (pColorTable)
    {
        pColorTable->InitColor();
        pColorTable->InitBrush();
    }
}

void cicUpdateUIFScheme(void)
{
    if (CUIFScheme::s_pColorTableSys)
        CUIFScheme::s_pColorTableSys->Update();
    if (CUIFScheme::s_pColorTableOff10)
        CUIFScheme::s_pColorTableOff10->Update();
}

void cicDoneUIFScheme(void)
{
    if (CUIFScheme::s_pColorTableSys)
    {
        delete CUIFScheme::s_pColorTableSys;
        CUIFScheme::s_pColorTableSys = NULL;
    }
    if (CUIFScheme::s_pColorTableOff10)
    {
        delete CUIFScheme::s_pColorTableOff10;
        CUIFScheme::s_pColorTableOff10 = NULL;
    }
}

/////////////////////////////////////////////////////////////////////////////

void cicInitUIFLib(void)
{
    cicInitUIFSys();
    cicInitUIFScheme();
    cicInitUIFUtil();
}

void cicDoneUIFLib(void)
{
    cicDoneUIFScheme();
    cicDoneUIFSys();
    cicDoneUIFUtil();
}

/////////////////////////////////////////////////////////////////////////////
