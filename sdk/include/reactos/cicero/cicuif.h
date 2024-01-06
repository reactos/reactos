/*
 * PROJECT:     ReactOS Cicero
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Cicero UI interface
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#include "cicarray.h"

struct CUIFTheme;
    class CUIFObject;
        class CUIFWindow;
class CUIFObjectArray;
class CUIFColorTable;
    class CUIFColorTableSys;
    class CUIFColorTableOff10;
class CUIFBitmapDC;
class CUIFScheme;

/////////////////////////////////////////////////////////////////////////////

#include <uxtheme.h>

// uxtheme.dll
using FN_OpenThemeData = decltype(&OpenThemeData);
using FN_CloseThemeData = decltype(&CloseThemeData);
using FN_DrawThemeBackground = decltype(&DrawThemeBackground);
using FN_DrawThemeParentBackground = decltype(&DrawThemeParentBackground);
using FN_DrawThemeText = decltype(&DrawThemeText);
using FN_DrawThemeIcon = decltype(&DrawThemeIcon);
using FN_GetThemeBackgroundExtent = decltype(&GetThemeBackgroundExtent);
using FN_GetThemeBackgroundContentRect = decltype(&GetThemeBackgroundContentRect);
using FN_GetThemeTextExtent = decltype(&GetThemeTextExtent);
using FN_GetThemePartSize = decltype(&GetThemePartSize);
using FN_DrawThemeEdge = decltype(&DrawThemeEdge);
using FN_GetThemeColor = decltype(&GetThemeColor);
using FN_GetThemeMargins = decltype(&GetThemeMargins);
using FN_GetThemeFont = decltype(&GetThemeFont);
using FN_GetThemeSysColor = decltype(&GetThemeSysColor);
using FN_GetThemeSysSize = decltype(&GetThemeSysSize);

/////////////////////////////////////////////////////////////////////////////

struct CUIFTheme
{
    LPCWSTR m_pszClassList;
    INT m_iPartId;
    DWORD m_dwUnknown2; //FIXME: name and type
    HTHEME m_hTheme;
    static HINSTANCE s_hUXTHEME;
    static FN_OpenThemeData s_fnOpenThemeData;
    static FN_CloseThemeData s_fnCloseThemeData;
    static FN_DrawThemeBackground s_fnDrawThemeBackground;
    static FN_DrawThemeParentBackground s_fnDrawThemeParentBackground;
    static FN_DrawThemeText s_fnDrawThemeText;
    static FN_DrawThemeIcon s_fnDrawThemeIcon;
    static FN_GetThemeBackgroundExtent s_fnGetThemeBackgroundExtent;
    static FN_GetThemeBackgroundContentRect s_fnGetThemeBackgroundContentRect;
    static FN_GetThemeTextExtent s_fnGetThemeTextExtent;
    static FN_GetThemePartSize s_fnGetThemePartSize;
    static FN_DrawThemeEdge s_fnDrawThemeEdge;
    static FN_GetThemeColor s_fnGetThemeColor;
    static FN_GetThemeMargins s_fnGetThemeMargins;
    static FN_GetThemeFont s_fnGetThemeFont;
    static FN_GetThemeSysColor s_fnGetThemeSysColor;
    static FN_GetThemeSysSize s_fnGetThemeSysSize;

    HRESULT InternalOpenThemeData(HWND hWnd);
    HRESULT EnsureThemeData(HWND hWnd);
    HRESULT CloseThemeData();

    STDMETHOD(DrawThemeBackground)(HDC hDC, int iStateId, LPCRECT pRect, LPCRECT pClipRect);
    STDMETHOD(DrawThemeParentBackground)(HWND hwnd, HDC hDC, LPRECT prc);
    STDMETHOD(DrawThemeText)(HDC hDC, int iStateId, LPCWSTR pszText, int cchText, DWORD dwTextFlags, DWORD dwTextFlags2, LPCRECT pRect);
    STDMETHOD(DrawThemeIcon)(HDC hDC, int iStateId, LPCRECT pRect, HIMAGELIST himl, int iImageIndex);
    STDMETHOD(GetThemeBackgroundExtent)(HDC hDC, int iStateId, LPCRECT pContentRect, LPRECT pExtentRect);
    STDMETHOD(GetThemeBackgroundContentRect)(HDC hDC, int iStateId, LPCRECT pBoundingRect, LPRECT pContentRect);
    STDMETHOD(GetThemeTextExtent)(HDC hDC, int iStateId, LPCWSTR pszText, int cchCharCount, DWORD dwTextFlags, LPCRECT pBoundingRect, LPRECT pExtentRect);
    STDMETHOD(GetThemePartSize)(HDC hDC, int iStateId, LPRECT prc, THEMESIZE eSize, SIZE *psz);
    STDMETHOD(DrawThemeEdge)(HDC hDC, int iStateId, LPCRECT pDestRect, UINT uEdge, UINT uFlags, LPRECT pContentRect);
    STDMETHOD(GetThemeColor)(int iStateId, int iPropId, COLORREF *pColor);
    STDMETHOD(GetThemeMargins)(HDC hDC, int iStateId, int iPropId, LPRECT prc, MARGINS *pMargins);
    STDMETHOD(GetThemeFont)(HDC hDC, int iStateId, int iPropId, LOGFONTW *pFont);
    STDMETHOD_(COLORREF, GetThemeSysColor)(INT iColorId);
    STDMETHOD_(int, GetThemeSysSize)(int iSizeId);
    STDMETHOD_(void, SetActiveTheme)(LPCWSTR pszClassList, INT iPartId, DWORD dwUnknown2);
};

/////////////////////////////////////////////////////////////////////////////

class CUIFObjectArray : public CicArray<CUIFObject*>
{
public:
    CUIFObjectArray() { }

    BOOL Add(CUIFObject *pObject)
    {
        if (!pObject || Find(pObject) >= 0)
            return FALSE;

        CUIFObject **ppNew = Append(1);
        if (!ppNew)
            return FALSE;

        *ppNew = pObject;
        return TRUE;
    }

    BOOL Remove(CUIFObject *pObject)
    {
        if (!pObject)
            return FALSE;

        ssize_t iItem = Find(pObject);
        if (iItem < 0)
            return FALSE;

        if (size_t(iItem) + 1 < size())
            MoveMemory(&data()[iItem], &data()[iItem + 1],
                       (size() - (iItem + 1)) * sizeof(CUIFObject*));

        --m_cItems;
        return TRUE;
    }

    CUIFObject *GetLast() const
    {
        if (empty())
            return NULL;
        return (*this)[size() - 1];
    }
};

/////////////////////////////////////////////////////////////////////////////

class CUIFObject : public CUIFTheme
{
protected:
    CUIFObject *m_pParent;
    CUIFWindow *m_pWindow;
    CUIFScheme *m_pScheme;
    CUIFObjectArray m_ObjectArray;
    DWORD m_dwUnknown3; //FIXME: name and type
    DWORD m_style;
    RECT m_rc;
    BOOL m_bEnable;
    BOOL m_bVisible;
    HFONT m_hFont;
    BOOL m_bHasCustomFont;
    LPWSTR m_pszToolTip;
    DWORD m_dwUnknown4[2]; //FIXME: name and type

public:
    CUIFObject(CUIFObject *pParent, DWORD dwUnknown3, LPRECT prc, DWORD style);
    virtual ~CUIFObject();

    STDMETHOD_(void, Initialize)();
    STDMETHOD_(void, OnPaint)(HWND hWnd);
    STDMETHOD_(void, OnHideToolTip)() { } // FIXME: name
    STDMETHOD_(void, OnLButtonDown)(POINT pt) { }
    STDMETHOD_(void, OnMButtonDown)(POINT pt) { }
    STDMETHOD_(void, OnRButtonDown)(POINT pt) { }
    STDMETHOD_(void, OnLButtonUp)(POINT pt) { }
    STDMETHOD_(void, OnMButtonUp)(POINT pt) { }
    STDMETHOD_(void, OnRButtonUp)(POINT pt) { }
    STDMETHOD_(void, OnMouseMove)(POINT pt) { }
    STDMETHOD_(void, OnRButtonDblClk)(POINT pt) { } //FIXME: name
    STDMETHOD_(void, OnRButtonUp2)(POINT pt) { } //FIXME: name
    STDMETHOD_(void, OnUnknown)(DWORD x1, DWORD x2, DWORD x3); //FIXME: name and type
    STDMETHOD_(void, GetRect)(LPRECT prc);
    STDMETHOD_(void, SetRect)(LPCRECT prc);
    STDMETHOD_(BOOL, PtInObject)(POINT pt);
    STDMETHOD_(void, PaintObject)(HWND hWnd, LPCRECT prc);
    STDMETHOD_(void, CallOnPaint)();
    STDMETHOD_(void, Enable)(BOOL bEnable);
    STDMETHOD_(void, Show)(BOOL bVisible);
    STDMETHOD_(void, SetFontToThis)(HFONT hFont);
    STDMETHOD_(void, SetFont)(HFONT hFont);
    STDMETHOD_(void, SetStyle)(DWORD style);
    STDMETHOD_(void, AddUIObj)(CUIFObject *pObject);
    STDMETHOD_(void, RemoveUIObj)(CUIFObject *pObject);
    STDMETHOD_(LRESULT, OnObjectNotify)(CUIFObject *pObject, WPARAM wParam, LPARAM lParam);
    STDMETHOD_(void, SetToolTip)(LPCWSTR pszToolTip);
    STDMETHOD_(LPCWSTR, GetToolTip)();
    STDMETHOD_(LRESULT, OnShowToolTip)();
    STDMETHOD_(void, OnHideToolTip2)() { } // FIXME: name
    STDMETHOD_(void, DetachWndObj)();
    STDMETHOD_(void, ClearWndObj)();
    STDMETHOD_(LRESULT, OnPaintTheme)(HWND hWnd);
    STDMETHOD_(void, OnSetFocus)(HWND hWnd);
    STDMETHOD_(void, ClearTheme)();
};

/////////////////////////////////////////////////////////////////////////////

class CUIFColorTable
{
public:
    CUIFColorTable() { }
    virtual ~CUIFColorTable() { }

    STDMETHOD_(void, InitColor)() = 0;
    STDMETHOD_(void, DoneColor)() { }
    STDMETHOD_(void, InitBrush)() = 0;
    STDMETHOD_(void, DoneBrush)() = 0;

    void Update()
    {
        DoneColor();
        DoneBrush();
        InitColor();
        InitBrush();
    }
};

class CUIFColorTableSys : public CUIFColorTable
{
protected:
    COLORREF m_rgbColors[16];
    HBRUSH m_hBrushes[16];

public:
    CUIFColorTableSys() { }

    HBRUSH GetBrush(INT iColor);

    STDMETHOD_(void, InitColor)() override;
    STDMETHOD_(void, InitBrush)() override;
    STDMETHOD_(void, DoneBrush)() override;
};

class CUIFColorTableOff10 : public CUIFColorTable
{
protected:
    COLORREF m_rgbColors[32];
    HBRUSH m_hBrushes[32];

public:
    CUIFColorTableOff10() { }

    HBRUSH GetBrush(INT iColor);

    STDMETHOD_(void, InitColor)() override;
    STDMETHOD_(void, InitBrush)() override;
    STDMETHOD_(void, DoneBrush)() override;
};

/////////////////////////////////////////////////////////////////////////////

class CUIFBitmapDC
{
protected:
    HBITMAP m_hBitmap;
    HGDIOBJ m_hOldBitmap;
    HGDIOBJ m_hOldObject;
    HDC m_hDC;

public:
    static BOOL s_fInitBitmapDCs;
    static CUIFBitmapDC *s_phdcSrc;
    static CUIFBitmapDC *s_phdcMask;
    static CUIFBitmapDC *s_phdcDst;

    CUIFBitmapDC(BOOL bMemory);
    ~CUIFBitmapDC();

    void Uninit(BOOL bKeep);

    BOOL SetBitmap(HBITMAP hBitmap);
    BOOL SetBitmap(LONG cx, LONG cy, WORD cPlanes, WORD cBitCount);
    BOOL SetDIB(LONG cx, LONG cy, WORD cPlanes, WORD cBitCount);
};

DECLSPEC_SELECTANY BOOL CUIFBitmapDC::s_fInitBitmapDCs = FALSE;
DECLSPEC_SELECTANY CUIFBitmapDC *CUIFBitmapDC::s_phdcSrc = NULL;
DECLSPEC_SELECTANY CUIFBitmapDC *CUIFBitmapDC::s_phdcMask = NULL;
DECLSPEC_SELECTANY CUIFBitmapDC *CUIFBitmapDC::s_phdcDst = NULL;

void cicInitUIFUtil(void);
void cicDoneUIFUtil(void);

/////////////////////////////////////////////////////////////////////////////

class CUIFScheme
{
public:
    static CUIFColorTableSys *s_pColorTableSys;
    static CUIFColorTableOff10 *s_pColorTableOff10;
};

DECLSPEC_SELECTANY CUIFColorTableSys *CUIFScheme::s_pColorTableSys = NULL;
DECLSPEC_SELECTANY CUIFColorTableOff10 *CUIFScheme::s_pColorTableOff10 = NULL;

void cicInitUIFScheme(void);
void cicUpdateUIFScheme(void);
void cicDoneUIFScheme(void);

/////////////////////////////////////////////////////////////////////////////

class CUIFWindow : public CUIFObject
{
    //FIXME
};

/////////////////////////////////////////////////////////////////////////////

// static members
DECLSPEC_SELECTANY HINSTANCE CUIFTheme::s_hUXTHEME = NULL;
DECLSPEC_SELECTANY FN_OpenThemeData CUIFTheme::s_fnOpenThemeData = NULL;
DECLSPEC_SELECTANY FN_CloseThemeData CUIFTheme::s_fnCloseThemeData = NULL;
DECLSPEC_SELECTANY FN_DrawThemeBackground CUIFTheme::s_fnDrawThemeBackground = NULL;
DECLSPEC_SELECTANY FN_DrawThemeParentBackground CUIFTheme::s_fnDrawThemeParentBackground = NULL;
DECLSPEC_SELECTANY FN_DrawThemeText CUIFTheme::s_fnDrawThemeText = NULL;
DECLSPEC_SELECTANY FN_DrawThemeIcon CUIFTheme::s_fnDrawThemeIcon = NULL;
DECLSPEC_SELECTANY FN_GetThemeBackgroundExtent CUIFTheme::s_fnGetThemeBackgroundExtent = NULL;
DECLSPEC_SELECTANY FN_GetThemeBackgroundContentRect CUIFTheme::s_fnGetThemeBackgroundContentRect = NULL;
DECLSPEC_SELECTANY FN_GetThemeTextExtent CUIFTheme::s_fnGetThemeTextExtent = NULL;
DECLSPEC_SELECTANY FN_GetThemePartSize CUIFTheme::s_fnGetThemePartSize = NULL;
DECLSPEC_SELECTANY FN_DrawThemeEdge CUIFTheme::s_fnDrawThemeEdge = NULL;
DECLSPEC_SELECTANY FN_GetThemeColor CUIFTheme::s_fnGetThemeColor = NULL;
DECLSPEC_SELECTANY FN_GetThemeMargins CUIFTheme::s_fnGetThemeMargins = NULL;
DECLSPEC_SELECTANY FN_GetThemeFont CUIFTheme::s_fnGetThemeFont = NULL;
DECLSPEC_SELECTANY FN_GetThemeSysColor CUIFTheme::s_fnGetThemeSysColor = NULL;
DECLSPEC_SELECTANY FN_GetThemeSysSize CUIFTheme::s_fnGetThemeSysSize = NULL;

/////////////////////////////////////////////////////////////////////////////

inline HRESULT CUIFTheme::InternalOpenThemeData(HWND hWnd)
{
    if (!hWnd || !m_pszClassList)
        return E_FAIL;

    if (!cicGetFN(s_hUXTHEME, s_fnOpenThemeData, TEXT("uxtheme.dll"), "OpenThemeData"))
        return E_FAIL;
    m_hTheme = s_fnOpenThemeData(hWnd, m_pszClassList);
    return (m_hTheme ? S_OK : E_FAIL);
}

inline HRESULT CUIFTheme::EnsureThemeData(HWND hWnd)
{
    if (m_hTheme)
        return S_OK;
    return InternalOpenThemeData(hWnd);
}

inline HRESULT CUIFTheme::CloseThemeData()
{
    if (!m_hTheme)
        return S_OK;

    if (!cicGetFN(s_hUXTHEME, s_fnCloseThemeData, TEXT("uxtheme.dll"), "CloseThemeData"))
        return E_FAIL;

    HRESULT hr = s_fnCloseThemeData(m_hTheme);
    m_hTheme = NULL;
    return hr;
}

inline STDMETHODIMP
CUIFTheme::DrawThemeBackground(HDC hDC, int iStateId, LPCRECT pRect, LPCRECT pClipRect)
{
    if (!cicGetFN(s_hUXTHEME, s_fnDrawThemeBackground, TEXT("uxtheme.dll"), "DrawThemeBackground"))
        return E_FAIL;
    return s_fnDrawThemeBackground(m_hTheme, hDC, m_iPartId, iStateId, pRect, pClipRect);
}

inline STDMETHODIMP
CUIFTheme::DrawThemeParentBackground(HWND hwnd, HDC hDC, LPRECT prc)
{
    if (!cicGetFN(s_hUXTHEME, s_fnDrawThemeParentBackground, TEXT("uxtheme.dll"), "DrawThemeParentBackground"))
        return E_FAIL;
    return s_fnDrawThemeParentBackground(hwnd, hDC, prc);
}

inline STDMETHODIMP
CUIFTheme::DrawThemeText(HDC hDC, int iStateId, LPCWSTR pszText, int cchText, DWORD dwTextFlags, DWORD dwTextFlags2, LPCRECT pRect)
{
    if (!cicGetFN(s_hUXTHEME, s_fnDrawThemeText, TEXT("uxtheme.dll"), "DrawThemeText"))
        return E_FAIL;
    return s_fnDrawThemeText(m_hTheme, hDC, m_iPartId, iStateId, pszText, cchText, dwTextFlags, dwTextFlags2, pRect);
}

inline STDMETHODIMP
CUIFTheme::DrawThemeIcon(HDC hDC, int iStateId, LPCRECT pRect, HIMAGELIST himl, int iImageIndex)
{
    if (!cicGetFN(s_hUXTHEME, s_fnDrawThemeIcon, TEXT("uxtheme.dll"), "DrawThemeIcon"))
        return E_FAIL;
    return s_fnDrawThemeIcon(m_hTheme, hDC, m_iPartId, iStateId, pRect, himl, iImageIndex);
}

inline STDMETHODIMP
CUIFTheme::GetThemeBackgroundExtent(HDC hDC, int iStateId, LPCRECT pContentRect, LPRECT pExtentRect)
{
    if (!cicGetFN(s_hUXTHEME, s_fnGetThemeBackgroundExtent, TEXT("uxtheme.dll"), "GetThemeBackgroundExtent"))
        return E_FAIL;
    return s_fnGetThemeBackgroundExtent(m_hTheme, hDC, m_iPartId, iStateId, pContentRect, pExtentRect);
}

inline STDMETHODIMP
CUIFTheme::GetThemeBackgroundContentRect(HDC hDC, int iStateId, LPCRECT pBoundingRect, LPRECT pContentRect)
{
    if (!cicGetFN(s_hUXTHEME, s_fnGetThemeBackgroundContentRect, TEXT("uxtheme.dll"), "GetThemeBackgroundContentRect"))
        return E_FAIL;
    return s_fnGetThemeBackgroundContentRect(m_hTheme, hDC, m_iPartId, iStateId, pBoundingRect, pContentRect);
}

inline STDMETHODIMP
CUIFTheme::GetThemeTextExtent(HDC hDC, int iStateId, LPCWSTR pszText, int cchCharCount, DWORD dwTextFlags, LPCRECT pBoundingRect, LPRECT pExtentRect)
{
    if (!cicGetFN(s_hUXTHEME, s_fnGetThemeTextExtent, TEXT("uxtheme.dll"), "GetThemeTextExtent"))
        return E_FAIL;
    return s_fnGetThemeTextExtent(m_hTheme, hDC, m_iPartId, iStateId, pszText, cchCharCount, dwTextFlags, pBoundingRect, pExtentRect);
}

inline STDMETHODIMP
CUIFTheme::GetThemePartSize(HDC hDC, int iStateId, LPRECT prc, THEMESIZE eSize, SIZE *psz)
{
    if (!cicGetFN(s_hUXTHEME, s_fnGetThemePartSize, TEXT("uxtheme.dll"), "GetThemePartSize"))
        return E_FAIL;
    return s_fnGetThemePartSize(m_hTheme, hDC, m_iPartId, iStateId, prc, eSize, psz);
}

inline STDMETHODIMP
CUIFTheme::DrawThemeEdge(HDC hDC, int iStateId, LPCRECT pDestRect, UINT uEdge, UINT uFlags, LPRECT pContentRect)
{
    if (!cicGetFN(s_hUXTHEME, s_fnDrawThemeEdge, TEXT("uxtheme.dll"), "DrawThemeEdge"))
        return E_FAIL;
    return s_fnDrawThemeEdge(m_hTheme, hDC, m_iPartId, iStateId, pDestRect, uEdge, uFlags, pContentRect);
}

inline STDMETHODIMP
CUIFTheme::GetThemeColor(int iStateId, int iPropId, COLORREF *pColor)
{
    if (!cicGetFN(s_hUXTHEME, s_fnGetThemeColor, TEXT("uxtheme.dll"), "GetThemeColor"))
        return E_FAIL;
    return s_fnGetThemeColor(m_hTheme, m_iPartId, iStateId, iPropId, pColor);
}

inline STDMETHODIMP
CUIFTheme::GetThemeMargins(HDC hDC, int iStateId, int iPropId, LPRECT prc, MARGINS *pMargins)
{
    if (!cicGetFN(s_hUXTHEME, s_fnGetThemeMargins, TEXT("uxtheme.dll"), "GetThemeMargins"))
        return E_FAIL;
    return s_fnGetThemeMargins(m_hTheme, hDC, m_iPartId, iStateId, iPropId, prc, pMargins);
}

inline STDMETHODIMP
CUIFTheme::GetThemeFont(HDC hDC, int iStateId, int iPropId, LOGFONTW *pFont)
{
    if (!cicGetFN(s_hUXTHEME, s_fnGetThemeFont, TEXT("uxtheme.dll"), "GetThemeFont"))
        return E_FAIL;
    return s_fnGetThemeFont(m_hTheme, hDC, m_iPartId, iStateId, iPropId, pFont);
}

inline STDMETHODIMP_(COLORREF)
CUIFTheme::GetThemeSysColor(INT iColorId)
{
    if (!cicGetFN(s_hUXTHEME, s_fnGetThemeSysColor, TEXT("uxtheme.dll"), "GetThemeSysColor"))
        return RGB(0, 0, 0);
    return s_fnGetThemeSysColor(m_hTheme, iColorId);
}

inline STDMETHODIMP_(int)
CUIFTheme::GetThemeSysSize(int iSizeId)
{
    if (!cicGetFN(s_hUXTHEME, s_fnGetThemeSysSize, TEXT("uxtheme.dll"), "GetThemeSysSize"))
        return 0;
    return s_fnGetThemeSysSize(m_hTheme, iSizeId);
}

inline STDMETHODIMP_(void)
CUIFTheme::SetActiveTheme(LPCWSTR pszClassList, INT iPartId, DWORD dwUnknown2)
{
    m_iPartId = iPartId;
    m_dwUnknown2 = dwUnknown2; //FIXME: name and type
    m_pszClassList = pszClassList;
}

/////////////////////////////////////////////////////////////////////////////

/// @unimplemented
inline
CUIFObject::CUIFObject(CUIFObject *pParent, DWORD dwUnknown3, LPRECT prc, DWORD style)
{
    m_pszClassList = NULL;
    m_hTheme = NULL;
    m_pParent = pParent;
    m_dwUnknown3 = dwUnknown3; //FIXME: name and type
    m_style = style;

    if (prc)
        m_rc = *prc;
    else
        ::SetRect(&m_rc, 0, 0, 0, 0);

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

    m_dwUnknown4[0] = -1; //FIXME
    m_dwUnknown4[1] = -1; //FIXME
}

/// @unimplemented
inline
CUIFObject::~CUIFObject()
{
    if (m_pWindow)
    {
        //FIXME
    }

    if (m_pszToolTip)
    {
        delete[] m_pszToolTip;
        m_pszToolTip = NULL;
    }

    for (;;)
    {
        CUIFObject *pLast = m_ObjectArray.GetLast();
        if (!pLast)
            break;

        m_ObjectArray.Remove(pLast);
        delete pLast;
    }

    if (m_pWindow)
        m_pWindow->RemoveUIObj(this);

    CloseThemeData();
}

inline STDMETHODIMP_(void) CUIFObject::Initialize()
{
}

inline STDMETHODIMP_(void) CUIFObject::OnPaint(HWND hWnd)
{
    if (!(m_pWindow->m_style & 0x80000000) || !OnPaintTheme(hWnd))
        OnSetFocus(hWnd);
}

inline STDMETHODIMP_(void) CUIFObject::OnUnknown(DWORD x1, DWORD x2, DWORD x3)
{
}

inline STDMETHODIMP_(void) CUIFObject::GetRect(LPRECT prc)
{
    *prc = m_rc;
}

/// @unimplemented
inline STDMETHODIMP_(void) CUIFObject::SetRect(LPCRECT prc)
{
    m_rc = *prc;
    // FIXME
    CallOnPaint();
}

inline STDMETHODIMP_(BOOL) CUIFObject::PtInObject(POINT pt)
{
    return m_bVisible && ::PtInRect(&m_rc, pt);
}

inline STDMETHODIMP_(void) CUIFObject::PaintObject(HWND hWnd, LPCRECT prc)
{
    if (!m_bVisible)
        return;

    if (!prc)
        prc = &m_rc;

    OnPaint(hWnd);

    for (size_t iItem = 0; iItem < m_ObjectArray.size(); ++iItem)
    {
        CUIFObject *pObject = m_ObjectArray[iItem];
        RECT rc;
        if (::IntersectRect(&rc, prc, &pObject->m_rc))
            pObject->PaintObject(hWnd, &rc);
    }
}

/// @unimplemented
inline STDMETHODIMP_(void) CUIFObject::CallOnPaint()
{
    //FIXME
}

inline STDMETHODIMP_(void) CUIFObject::Enable(BOOL bEnable)
{
    if (m_bEnable == bEnable)
        return;

    m_bEnable = bEnable;
    for (size_t iItem = 0; iItem < m_ObjectArray.size(); ++iItem)
        m_ObjectArray[iItem]->Enable(bEnable);

    CallOnPaint();
}

inline STDMETHODIMP_(void) CUIFObject::Show(BOOL bVisible)
{
    if (m_bVisible == bVisible)
        return;

    m_bVisible = bVisible;
    for (size_t iItem = 0; iItem < m_ObjectArray.size(); ++iItem)
        m_ObjectArray[iItem]->Show(bVisible);

    if (m_bVisible || m_pParent)
        m_pParent->CallOnPaint();
}

inline STDMETHODIMP_(void) CUIFObject::SetFontToThis(HFONT hFont)
{
    m_bHasCustomFont = !!hFont;
    if (!hFont)
        hFont = (HFONT)::GetStockObject(DEFAULT_GUI_FONT);
    m_hFont = hFont;
}

inline STDMETHODIMP_(void) CUIFObject::SetFont(HFONT hFont)
{
    SetFontToThis(hFont);

    for (size_t iItem = 0; iItem < m_ObjectArray.size(); ++iItem)
        m_ObjectArray[iItem]->SetFont(hFont);

    CallOnPaint();
}

inline STDMETHODIMP_(void) CUIFObject::SetStyle(DWORD style)
{
    m_style = style;
}

inline STDMETHODIMP_(void) CUIFObject::AddUIObj(CUIFObject *pObject)
{
    m_ObjectArray.Add(pObject);
    CallOnPaint();
}

inline STDMETHODIMP_(void) CUIFObject::RemoveUIObj(CUIFObject *pObject)
{
    if (m_ObjectArray.Remove(pObject))
        CallOnPaint();
}

inline STDMETHODIMP_(LRESULT) CUIFObject::OnObjectNotify(CUIFObject *pObject, WPARAM wParam, LPARAM lParam)
{
    if (m_pParent)
        return m_pParent->OnObjectNotify(pObject, wParam, lParam);
    return 0;
}

inline STDMETHODIMP_(void) CUIFObject::SetToolTip(LPCWSTR pszToolTip)
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
            wcscpy(m_pszToolTip, pszToolTip);
    }
}

inline STDMETHODIMP_(LPCWSTR) CUIFObject::GetToolTip()
{
    return m_pszToolTip;
}

inline STDMETHODIMP_(LRESULT) CUIFObject::OnShowToolTip()
{
    return 0;
}

/// @unimplemented
inline STDMETHODIMP_(void) CUIFObject::DetachWndObj()
{
    //FIXME
    m_pWindow = NULL;
}

inline STDMETHODIMP_(void) CUIFObject::ClearWndObj()
{
    m_pWindow = NULL;
    for (size_t iItem = 0; iItem < m_ObjectArray.size(); ++iItem)
        m_ObjectArray[iItem]->ClearWndObj();
}

inline STDMETHODIMP_(LRESULT) CUIFObject::OnPaintTheme(HWND hWnd)
{
    return 0;
}

inline STDMETHODIMP_(void) CUIFObject::OnSetFocus(HWND hWnd)
{
}

inline STDMETHODIMP_(void) CUIFObject::ClearTheme()
{
    CloseThemeData();
    for (size_t iItem = 0; iItem < m_ObjectArray.size(); ++iItem)
        m_ObjectArray[iItem]->ClearTheme();
}

/////////////////////////////////////////////////////////////////////////////

inline STDMETHODIMP_(void) CUIFColorTableSys::InitColor()
{
    m_rgbColors[0] = GetSysColor(COLOR_BTNFACE);
    m_rgbColors[1] = GetSysColor(COLOR_BTNSHADOW);
    m_rgbColors[2] = GetSysColor(COLOR_ACTIVEBORDER);
    m_rgbColors[3] = GetSysColor(COLOR_ACTIVECAPTION);
    m_rgbColors[4] = GetSysColor(COLOR_BTNFACE);
    m_rgbColors[5] = GetSysColor(COLOR_BTNSHADOW);
    m_rgbColors[6] = GetSysColor(COLOR_BTNTEXT);
    m_rgbColors[7] = GetSysColor(COLOR_CAPTIONTEXT);
    m_rgbColors[8] = GetSysColor(COLOR_GRAYTEXT);
    m_rgbColors[9] = GetSysColor(COLOR_HIGHLIGHT);
    m_rgbColors[10] = GetSysColor(COLOR_HIGHLIGHTTEXT);
    m_rgbColors[11] = GetSysColor(COLOR_INACTIVECAPTION);
    m_rgbColors[12] = GetSysColor(COLOR_INACTIVECAPTIONTEXT);
    m_rgbColors[13] = GetSysColor(COLOR_MENUTEXT);
    m_rgbColors[14] = GetSysColor(COLOR_WINDOW);
    m_rgbColors[15] = GetSysColor(COLOR_WINDOWTEXT);
}

inline STDMETHODIMP_(void) CUIFColorTableSys::InitBrush()
{
    ZeroMemory(m_hBrushes, sizeof(m_hBrushes));
}

inline STDMETHODIMP_(void) CUIFColorTableSys::DoneBrush()
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

inline HBRUSH CUIFColorTableSys::GetBrush(INT iColor)
{
    if (!m_hBrushes[iColor])
        m_hBrushes[iColor] = ::CreateSolidBrush(m_rgbColors[iColor]);
    return m_hBrushes[iColor];
}

inline HBRUSH CUIFColorTableOff10::GetBrush(INT iColor)
{
    if (!m_hBrushes[iColor])
        m_hBrushes[iColor] = ::CreateSolidBrush(m_rgbColors[iColor]);
    return m_hBrushes[iColor];
}

/// @unimplemented
inline STDMETHODIMP_(void) CUIFColorTableOff10::InitColor()
{
    m_rgbColors[0] = GetSysColor(COLOR_BTNFACE);
    m_rgbColors[1] = GetSysColor(COLOR_WINDOW);
    m_rgbColors[2] = GetSysColor(COLOR_WINDOW);
    m_rgbColors[3] = GetSysColor(COLOR_BTNFACE);
    m_rgbColors[4] = GetSysColor(COLOR_BTNSHADOW);
    m_rgbColors[5] = GetSysColor(COLOR_WINDOW);
    m_rgbColors[6] = GetSysColor(COLOR_HIGHLIGHT);
    m_rgbColors[7] = GetSysColor(COLOR_WINDOWTEXT);
    m_rgbColors[8] = GetSysColor(COLOR_HIGHLIGHT);
    m_rgbColors[9] = GetSysColor(COLOR_HIGHLIGHT);
    m_rgbColors[10] = GetSysColor(COLOR_HIGHLIGHTTEXT);
    m_rgbColors[11] = GetSysColor(COLOR_BTNFACE);
    m_rgbColors[12] = GetSysColor(COLOR_BTNTEXT);
    m_rgbColors[13] = GetSysColor(COLOR_BTNSHADOW);
    m_rgbColors[14] = GetSysColor(COLOR_BTNFACE);
    m_rgbColors[15] = GetSysColor(COLOR_WINDOW);
    m_rgbColors[16] = GetSysColor(COLOR_HIGHLIGHT);
    m_rgbColors[17] = GetSysColor(COLOR_BTNTEXT);
    m_rgbColors[18] = GetSysColor(COLOR_WINDOW);
    m_rgbColors[19] = GetSysColor(COLOR_BTNSHADOW);
    m_rgbColors[20] = GetSysColor(COLOR_BTNFACE);
    m_rgbColors[21] = GetSysColor(COLOR_BTNSHADOW);
    m_rgbColors[22] = GetSysColor(COLOR_BTNFACE);
    m_rgbColors[23] = GetSysColor(COLOR_BTNSHADOW);
    m_rgbColors[24] = GetSysColor(COLOR_CAPTIONTEXT);
    m_rgbColors[25] = GetSysColor(COLOR_HIGHLIGHT);
    m_rgbColors[26] = GetSysColor(COLOR_HIGHLIGHTTEXT);
    m_rgbColors[27] = GetSysColor(COLOR_BTNFACE);
    m_rgbColors[28] = GetSysColor(COLOR_BTNTEXT);
    m_rgbColors[29] = GetSysColor(COLOR_BTNSHADOW);
    m_rgbColors[30] = GetSysColor(COLOR_BTNTEXT);
    m_rgbColors[31] = GetSysColor(COLOR_WINDOWTEXT);
}

inline STDMETHODIMP_(void) CUIFColorTableOff10::InitBrush()
{
    ZeroMemory(m_hBrushes, sizeof(m_hBrushes));
}

inline STDMETHODIMP_(void) CUIFColorTableOff10::DoneBrush()
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

inline void cicInitUIFScheme(void)
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

inline void cicUpdateUIFScheme(void)
{
    if (CUIFScheme::s_pColorTableSys)
        CUIFScheme::s_pColorTableSys->Update();
    if (CUIFScheme::s_pColorTableOff10)
        CUIFScheme::s_pColorTableOff10->Update();
}

inline void cicDoneUIFScheme(void)
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

inline CUIFBitmapDC::CUIFBitmapDC(BOOL bMemory)
{
    m_hBitmap = NULL;
    m_hOldBitmap = NULL;
    m_hOldObject = NULL;
    if (bMemory)
        m_hDC = ::CreateCompatibleDC(NULL);
    else
        m_hDC = ::CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);
}

inline CUIFBitmapDC::~CUIFBitmapDC()
{
    Uninit(FALSE);
    ::DeleteDC(m_hDC);
}

inline void CUIFBitmapDC::Uninit(BOOL bKeep)
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

inline BOOL CUIFBitmapDC::SetBitmap(HBITMAP hBitmap)
{
    if (m_hDC)
        m_hOldBitmap = ::SelectObject(m_hDC, hBitmap);
    return TRUE;
}

inline BOOL CUIFBitmapDC::SetBitmap(LONG cx, LONG cy, WORD cPlanes, WORD cBitCount)
{
    m_hBitmap = ::CreateBitmap(cx, cy, cPlanes, cBitCount, 0);
    m_hOldBitmap = ::SelectObject(m_hDC, m_hBitmap);
    return TRUE;
}

inline BOOL CUIFBitmapDC::SetDIB(LONG cx, LONG cy, WORD cPlanes, WORD cBitCount)
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

inline void cicInitUIFUtil(void)
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

inline void cicDoneUIFUtil(void)
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
