/*
 * PROJECT:     ReactOS Cicero
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Cicero UI interface
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#include "cicbase.h"

/////////////////////////////////////////////////////////////////////////////

#include <uxtheme.h>

// uxtheme.dll
typedef HRESULT (WINAPI *FN_DrawThemeBackground)(
    HTHEME  hTheme,
    HDC     hdc,
    int     iPartId,
    int     iStateId,
    LPCRECT pRect,
    LPCRECT pClipRect);
typedef HRESULT (WINAPI *FN_DrawThemeParentBackground)(
    HWND       hwnd,
    HDC        hdc,
    const RECT *prc);
typedef HRESULT (WINAPI *FN_DrawThemeText)(
    HTHEME  hTheme,
    HDC     hdc,
    int     iPartId,
    int     iStateId,
    LPCWSTR pszText,
    int     cchText,
    DWORD   dwTextFlags,
    DWORD   dwTextFlags2,
    LPCRECT pRect);
typedef HRESULT (WINAPI *FN_DrawThemeIcon)(
    HTHEME     hTheme,
    HDC        hdc,
    int        iPartId,
    int        iStateId,
    LPCRECT    pRect,
    HIMAGELIST himl,
    int        iImageIndex);
typedef HRESULT (WINAPI *FN_GetThemeBackgroundExtent)(
    HTHEME  hTheme,
    HDC     hdc,
    int     iPartId,
    int     iStateId,
    LPCRECT pContentRect,
    LPRECT  pExtentRect);
typedef HRESULT (WINAPI *FN_GetThemeBackgroundContentRect)(
    HTHEME  hTheme,
    HDC     hdc,
    int     iPartId,
    int     iStateId,
    LPCRECT pBoundingRect,
    LPRECT  pContentRect);
typedef HRESULT (WINAPI *FN_GetThemeTextExtent)(
    HTHEME  hTheme,
    HDC     hdc,
    int     iPartId,
    int     iStateId,
    LPCWSTR pszText,
    int     cchCharCount,
    DWORD   dwTextFlags,
    LPCRECT pBoundingRect,
    LPRECT  pExtentRect);
typedef HRESULT (WINAPI *FN_GetThemePartSize)(
    HTHEME    hTheme,
    HDC       hdc,
    int       iPartId,
    int       iStateId,
    LPCRECT   prc,
    THEMESIZE eSize,
    SIZE      *psz);
typedef HRESULT (WINAPI *FN_DrawThemeEdge)(
    HTHEME  hTheme,
    HDC     hdc,
    int     iPartId,
    int     iStateId,
    LPCRECT pDestRect,
    UINT    uEdge,
    UINT    uFlags,
    LPRECT  pContentRect);
typedef HRESULT (WINAPI *FN_GetThemeColor)(
    HTHEME   hTheme,
    int      iPartId,
    int      iStateId,
    int      iPropId,
    COLORREF *pColor);
typedef HRESULT (WINAPI *FN_GetThemeMargins)(
    HTHEME  hTheme,
    HDC     hdc,
    int     iPartId,
    int     iStateId,
    int     iPropId,
    LPCRECT prc,
    MARGINS *pMargins);
typedef HRESULT (WINAPI *FN_GetThemeFont)(
    HTHEME   hTheme,
    HDC      hdc,
    int      iPartId,
    int      iStateId,
    int      iPropId,
    LOGFONTW *pFont);
typedef COLORREF (WINAPI *FN_GetThemeSysColor)(
    HTHEME hTheme,
    int    iColorId);
typedef int (WINAPI *FN_GetThemeSysSize)(
    HTHEME hTheme,
    int    iSizeId);

/////////////////////////////////////////////////////////////////////////////

struct CUIFTheme
{
protected:
    LPCWSTR m_pszClassList;
    INT m_iPartId;
    DWORD m_dwUnknown2;
    HTHEME m_hTheme;
    static HINSTANCE s_hUXTHEME;
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

public:
    STDMETHOD(DrawThemeBackground)(HDC hDC, int iStateId, LPCRECT pRect, LPCRECT pClipRect);
    STDMETHOD(DrawThemeParentBackground)(HWND hwnd, HDC hDC, LPCRECT prc);
    STDMETHOD(DrawThemeText)(HDC hDC, int iStateId, LPCWSTR pszText, int cchText, DWORD dwTextFlags, DWORD dwTextFlags2, LPCRECT pRect);
    STDMETHOD(DrawThemeIcon)(HDC hDC, int iStateId, LPCRECT pRect, HIMAGELIST himl, int iImageIndex);
    STDMETHOD(GetThemeBackgroundExtent)(HDC hDC, int iStateId, LPCRECT pContentRect, LPRECT pExtentRect);
    STDMETHOD(GetThemeBackgroundContentRect)(HDC hDC, int iStateId, LPCRECT pBoundingRect, LPRECT pContentRect);
    STDMETHOD(GetThemeTextExtent)(HDC hDC, int iStateId, LPCWSTR pszText, int cchCharCount, DWORD dwTextFlags, LPCRECT pBoundingRect, LPRECT pExtentRect);
    STDMETHOD(GetThemePartSize)(HDC hDC, int iStateId, LPCRECT prc, THEMESIZE eSize, SIZE *psz);
    STDMETHOD(DrawThemeEdge)(HDC hDC, int iStateId, LPCRECT pDestRect, UINT uEdge, UINT uFlags, LPRECT pContentRect);
    STDMETHOD(GetThemeColor)(int iStateId, int iPropId, COLORREF *pColor);
    STDMETHOD(GetThemeMargins)(HDC hDC, int iStateId, int iPropId, LPCRECT prc, MARGINS *pMargins);
    STDMETHOD(GetThemeFont)(HDC hDC, int iStateId, int iPropId, LOGFONTW *pFont);
    STDMETHOD_(COLORREF, GetThemeSysColor)(INT iColorId);
    STDMETHOD_(int, GetThemeSysSize)(int iSizeId);
    virtual void SetActiveTheme(LPCWSTR pszClassList, INT iPartId, DWORD dwUnknown2);
};

/////////////////////////////////////////////////////////////////////////////

// static members
DECLSPEC_SELECTANY HINSTANCE CUIFTheme::s_hUXTHEME = NULL;
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

inline STDMETHODIMP
CUIFTheme::DrawThemeBackground(HDC hDC, int iStateId, LPCRECT pRect, LPCRECT pClipRect)
{
    if (!cicGetFN(s_hUXTHEME, s_fnDrawThemeBackground, TEXT("uxtheme.dll"), "DrawThemeBackground"))
        return E_FAIL;
    return s_fnDrawThemeBackground(m_hTheme, hDC, m_iPartId, iStateId, pRect, pClipRect);
}

inline STDMETHODIMP
CUIFTheme::DrawThemeParentBackground(HWND hwnd, HDC hDC, LPCRECT prc)
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
CUIFTheme::GetThemePartSize(HDC hDC, int iStateId, LPCRECT prc, THEMESIZE eSize, SIZE *psz)
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
CUIFTheme::GetThemeMargins(HDC hDC, int iStateId, int iPropId, LPCRECT prc, MARGINS *pMargins)
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
        return E_FAIL;
    return s_fnGetThemeSysColor(m_hTheme, iColorId);
}

inline STDMETHODIMP_(int)
CUIFTheme::GetThemeSysSize(int iSizeId)
{
    if (!cicGetFN(s_hUXTHEME, s_fnGetThemeSysSize, TEXT("uxtheme.dll"), "GetThemeSysSize"))
        return E_FAIL;
    return s_fnGetThemeSysSize(m_hTheme, iSizeId);
}

inline void
CUIFTheme::SetActiveTheme(LPCWSTR pszClassList, INT iPartId, DWORD dwUnknown2)
{
    m_iPartId = iPartId;
    m_dwUnknown2 = dwUnknown2;
    m_pszClassList = pszClassList;
}
