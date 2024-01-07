/*
 * PROJECT:     ReactOS Cicero
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Cicero UI interface
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#include "cicarray.h"

class CUIFSystemInfo;
struct CUIFTheme;
    class CUIFObject;
        class CUIFWindow;
            class CUIFToolTip;
            class CUIFShadow;
class CUIFObjectArray;
class CUIFColorTable;
    class CUIFColorTableSys;
    class CUIFColorTableOff10;
class CUIFBitmapDC;
class CUIFScheme;

/////////////////////////////////////////////////////////////////////////////

class CUIFSystemInfo : OSVERSIONINFO
{
public:
    static CUIFSystemInfo *s_pSystemInfo;
    DWORD m_cBitsPixels;
    BOOL m_bHighContrast1;
    BOOL m_bHighContrast2;

    CUIFSystemInfo();
    void GetSystemMetrics();
    void Initialize();
};

DECLSPEC_SELECTANY CUIFSystemInfo *CUIFSystemInfo::s_pSystemInfo = NULL;

void cicInitUIFSys(void);
void cicDoneUIFSys(void);
void cicUpdateUIFSys(void);

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
    friend class CUIFWindow;

public:
    CUIFObject(CUIFObject *pParent, DWORD dwUnknown3, LPRECT prc, DWORD style);
    virtual ~CUIFObject();

    void StartCapture();
    void EndCapture();
    BOOL IsCapture();
    BOOL IsRTL();
    LRESULT NotifyCommand(WPARAM wParam, LPARAM lParam);
    CUIFObject* ObjectFromPoint(POINT pt);
    void SetScheme(CUIFScheme *scheme);

    STDMETHOD_(void, Initialize)();
    STDMETHOD_(void, OnPaint)(HDC hDC);
    STDMETHOD_(void, OnHideToolTip)() { } // FIXME: name
    STDMETHOD_(void, OnLButtonDown)(LONG x, LONG y) { }
    STDMETHOD_(void, OnMButtonDown)(LONG x, LONG y) { }
    STDMETHOD_(void, OnRButtonDown)(LONG x, LONG y) { }
    STDMETHOD_(void, OnLButtonUp)(LONG x, LONG y) { }
    STDMETHOD_(void, OnMButtonUp)(LONG x, LONG y) { }
    STDMETHOD_(void, OnRButtonUp)(LONG x, LONG y) { }
    STDMETHOD_(void, OnMouseMove)(LONG x, LONG y) { }
    STDMETHOD_(void, OnRButtonDblClk)(LONG x, LONG y) { } //FIXME: name
    STDMETHOD_(void, OnRButtonUp2)(LONG x, LONG y) { } //FIXME: name
    STDMETHOD_(BOOL, OnSetCursor)(UINT uMsg, LONG x, LONG y);
    STDMETHOD_(void, GetRect)(LPRECT prc);
    STDMETHOD_(void, SetRect)(LPCRECT prc);
    STDMETHOD_(BOOL, PtInObject)(POINT pt);
    STDMETHOD_(void, PaintObject)(HDC hDC, LPCRECT prc);
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
    STDMETHOD_(LRESULT, OnPaintTheme)(HDC hDC);
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
    operator HDC() const { return m_hDC; }

    void Uninit(BOOL bKeep);

    BOOL SetBitmap(HBITMAP hBitmap);
    BOOL SetBitmap(LONG cx, LONG cy, WORD cPlanes, WORD cBitCount);
    BOOL SetDIB(LONG cx, LONG cy, WORD cPlanes, WORD cBitCount);

    HBITMAP DetachBitmap()
    {
        HBITMAP hOldBitmap = m_hBitmap;
        m_hBitmap = NULL;
        return hOldBitmap;
    }
};

DECLSPEC_SELECTANY BOOL CUIFBitmapDC::s_fInitBitmapDCs = FALSE;
DECLSPEC_SELECTANY CUIFBitmapDC *CUIFBitmapDC::s_phdcSrc = NULL;
DECLSPEC_SELECTANY CUIFBitmapDC *CUIFBitmapDC::s_phdcMask = NULL;
DECLSPEC_SELECTANY CUIFBitmapDC *CUIFBitmapDC::s_phdcDst = NULL;

void cicInitUIFUtil(void);
void cicDoneUIFUtil(void);

BOOL cicSetLayout(HDC hDC, BOOL bLayout);
HBITMAP cicMirrorBitmap(HBITMAP hBitmap, HBRUSH hbrBack);
HBRUSH cicCreateDitherBrush(VOID);
HBITMAP cicCreateDisabledBitmap(LPCRECT prc, HBITMAP hbmMask, HBRUSH hbr1, HBRUSH hbr2,
                                BOOL bPressed);
HBITMAP cicCreateShadowMaskBmp(LPRECT prc, HBITMAP hbm1, HBITMAP hbm2, HBRUSH hbr1, HBRUSH hbr2);

/////////////////////////////////////////////////////////////////////////////

class CUIFScheme
{
public:
    static CUIFColorTableSys *s_pColorTableSys;
    static CUIFColorTableOff10 *s_pColorTableOff10;

    CUIFScheme(DWORD type);
};

DECLSPEC_SELECTANY CUIFColorTableSys *CUIFScheme::s_pColorTableSys = NULL;
DECLSPEC_SELECTANY CUIFColorTableOff10 *CUIFScheme::s_pColorTableOff10 = NULL;

void cicInitUIFScheme(void);
void cicUpdateUIFScheme(void);
void cicDoneUIFScheme(void);
CUIFScheme *cicCreateUIFScheme(DWORD type);

/////////////////////////////////////////////////////////////////////////////

class CUIFWindow : public CUIFObject
{
protected:
    INT m_nLeft;
    INT m_nTop;
    INT m_nHeight;
    INT m_nWidth;
    HINSTANCE m_hInst;
    HWND m_hWnd;
    CUIFWindow *m_pUnknown7;
    CUIFObject *m_pCaptured;
    CUIFObject *m_pPointed;
    DWORD m_dwUnknown8;
    DWORD m_dwUnknown9;
    CUIFToolTip *m_pToolTip;
    CUIFShadow *m_pShadow;
    BOOL m_bShowShadow;
    DWORD m_dwUnknown10;
    friend class CUIFObject;

public:
    CUIFWindow(HINSTANCE hInst, DWORD style);
    ~CUIFWindow() override;

    static CUIFWindow* GetThis(HWND hWnd);
    static void SetThis(HWND hWnd, LONG_PTR dwNewLong);

    STDMETHOD_(void, Initialize)() override;
    STDMETHOD_(void, Show)(BOOL bVisible) override;
    STDMETHOD_(void, SetRect)(LPCRECT prc) override;
    STDMETHOD_(void, PaintObject)(HDC hDC, LPCRECT prc) override;
    STDMETHOD_(void, RemoveUIObj)(CUIFObject *pRemove) override;

    void SetCaptureObject(CUIFObject *pCaptured);
    void SetObjectPointed(CUIFObject *pPointed, POINT pt);
    void CreateScheme();
    BOOL GetWorkArea(LPCRECT prcWnd, LPRECT prcWorkArea);
    void AdjustWindowPosition();

    static LRESULT CALLBACK WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    STDMETHOD_(LPCTSTR, GetClassName)();
    STDMETHOD_(LPCTSTR, GetWndText)();
    STDMETHOD_(DWORD, GetWndStyle)();
    STDMETHOD_(DWORD, GetWndStyleEx)();
    STDMETHOD_(HWND, CreateWnd)(HWND hwndParent);
    STDMETHOD_(void, Move)(INT x, INT y, INT nWidth, INT nHeight);
    STDMETHOD_(BOOL, AnimateWnd)(DWORD dwTime, DWORD dwFlags);
    STDMETHOD_(void, OnObjectMoved)(CUIFObject *pObject);
    STDMETHOD_(void, OnLButtonDown2)(LONG x, LONG y);
    STDMETHOD_(void, OnCreate)(HWND hWnd);
    STDMETHOD_(void, OnDestroy)(HWND hWnd);
    STDMETHOD_(void, OnNCDestroy)(HWND hWnd);
    STDMETHOD_(void, OnSetFocus)(HWND hWnd);
    STDMETHOD_(void, OnKillFocus)(HWND hWnd);
    STDMETHOD_(void, OnNotify)(HWND, WPARAM wParam, LPARAM lParam);
    STDMETHOD_(void, OnTimer)(HWND);
    STDMETHOD_(void, OnSysColorChange)();
    STDMETHOD_(void, OnEndSession)(HWND, UINT, LONG);
    STDMETHOD_(void, OnKeyDown)(HWND, UINT, LONG);
    STDMETHOD_(void, OnKeyUp)(HWND, UINT, LONG);
    STDMETHOD_(void, OnUser)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    STDMETHOD_(LRESULT, OnActivate)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    STDMETHOD_(LRESULT, OnWindowPosChanged)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    STDMETHOD_(LRESULT, OnWindowPosChanging)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    STDMETHOD_(LRESULT, OnNotifyFormat)(HWND hWnd, WPARAM wParam, LPARAM lParam);
    STDMETHOD_(LRESULT, OnShowWindow)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    STDMETHOD_(LRESULT, OnSettingChange)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    STDMETHOD_(LRESULT, OnDisplayChange)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    STDMETHOD_(LRESULT, OnGetObject)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    STDMETHOD_(LRESULT, WindowProc)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    STDMETHOD_(BOOL, OnEraseBkGnd)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    STDMETHOD_(void, OnThemeChanged)(HWND, UINT, LONG);
    STDMETHOD_(void, UpdateUI)(LPCRECT prc);
    STDMETHOD_(void, SetCapture)(int);
    STDMETHOD_(void, OnSetCapture)(HWND hWnd, UINT, LONG);
    STDMETHOD_(void, OnAnimationStart)();
    STDMETHOD_(void, OnAnimationEnd)();
    STDMETHOD_(void, HandleMouseMsg)(UINT uMsg, LONG x, LONG y);
    STDMETHOD_(void, ClientRectToWindowRect)(LPRECT pRect);
    STDMETHOD_(void, GetWindowFrameSize)(LPSIZE pSize);
};

/////////////////////////////////////////////////////////////////////////////

class CUIFToolTip : public CUIFWindow
{
    //FIXME
};

class CUIFShadow : public CUIFWindow
{
    //FIXME
};

/////////////////////////////////////////////////////////////////////////////

inline void cicInitUIFLib(void)
{
    cicInitUIFSys();
    cicInitUIFScheme();
    cicInitUIFUtil();
}

inline void cicDoneUIFLib(void)
{
    cicDoneUIFScheme();
    cicDoneUIFSys();
    cicDoneUIFUtil();
}

/////////////////////////////////////////////////////////////////////////////

inline CUIFSystemInfo::CUIFSystemInfo()
{
    dwMajorVersion = 4;
    dwMinorVersion = 0;
    dwBuildNumber = 0;
    dwPlatformId = VER_PLATFORM_WIN32_WINDOWS;
    m_cBitsPixels = 8;
    m_bHighContrast1 = m_bHighContrast2 = FALSE;
}

inline void CUIFSystemInfo::GetSystemMetrics()
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

inline void CUIFSystemInfo::Initialize()
{
    dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    ::GetVersionEx(this);
    GetSystemMetrics();
}

inline void cicInitUIFSys(void)
{
    CUIFSystemInfo::s_pSystemInfo = new(cicNoThrow) CUIFSystemInfo();
    if (CUIFSystemInfo::s_pSystemInfo)
        CUIFSystemInfo::s_pSystemInfo->Initialize();
}

inline void cicDoneUIFSys(void)
{
    if (CUIFSystemInfo::s_pSystemInfo)
    {
        delete CUIFSystemInfo::s_pSystemInfo;
        CUIFSystemInfo::s_pSystemInfo = NULL;
    }
}

inline void cicUpdateUIFSys(void)
{
    if (CUIFSystemInfo::s_pSystemInfo)
        CUIFSystemInfo::s_pSystemInfo->GetSystemMetrics();
}

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

/// @unimplemented
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

/// unimplemented
inline STDMETHODIMP_(void) CUIFObject::OnPaint(HDC hDC)
{
    if (!(m_pWindow->m_style & 0x80000000) || !OnPaintTheme(hDC))
        ; //FIXME
}

inline STDMETHODIMP_(BOOL) CUIFObject::OnSetCursor(UINT uMsg, LONG x, LONG y)
{
    return FALSE;
}

inline STDMETHODIMP_(void) CUIFObject::GetRect(LPRECT prc)
{
    *prc = m_rc;
}

inline STDMETHODIMP_(BOOL) CUIFObject::PtInObject(POINT pt)
{
    return m_bVisible && ::PtInRect(&m_rc, pt);
}

inline STDMETHODIMP_(void) CUIFObject::PaintObject(HDC hDC, LPCRECT prc)
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

inline STDMETHODIMP_(void) CUIFObject::CallOnPaint()
{
    if (m_pWindow)
        m_pWindow->UpdateUI(&m_rc);
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

inline STDMETHODIMP_(void) CUIFObject::ClearWndObj()
{
    m_pWindow = NULL;
    for (size_t iItem = 0; iItem < m_ObjectArray.size(); ++iItem)
        m_ObjectArray[iItem]->ClearWndObj();
}

inline STDMETHODIMP_(LRESULT) CUIFObject::OnPaintTheme(HDC hDC)
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

inline void CUIFObject::StartCapture()
{
    if (m_pWindow)
        m_pWindow->SetCaptureObject(this);
}

inline void CUIFObject::EndCapture()
{
    if (m_pWindow)
        m_pWindow->SetCaptureObject(NULL);
}

inline BOOL CUIFObject::IsCapture()
{
    return m_pWindow && (m_pWindow->m_pCaptured == this);
}

inline void CUIFObject::SetRect(LPCRECT prc)
{
    m_rc = *prc;
    if (m_pWindow)
        m_pWindow->OnObjectMoved(this);
    CallOnPaint();
}

inline LRESULT CUIFObject::NotifyCommand(WPARAM wParam, LPARAM lParam)
{
    if (m_pParent)
        return m_pParent->OnObjectNotify(this, wParam, lParam);
    return 0;
}

/// @unimplemented
inline void CUIFObject::DetachWndObj()
{
    if (m_pWindow)
    {
        CUIFToolTip *pToolTip = m_pWindow->m_pToolTip;
        if (pToolTip)
        {
            //FIXME
        }
        m_pWindow->RemoveUIObj(this);
        m_pWindow = NULL;
    }
}

inline BOOL CUIFObject::IsRTL()
{
    if (!m_pWindow)
        return FALSE;
    return !!(m_pWindow->m_style & 0x200);
}

inline CUIFObject* CUIFObject::ObjectFromPoint(POINT pt)
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

inline void CUIFObject::SetScheme(CUIFScheme *scheme)
{
    m_pScheme = scheme;
    for (size_t i = 0; i < m_ObjectArray.size(); ++i)
    {
        m_ObjectArray[i]->SetScheme(scheme);
    }
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

/// @unimplemented
inline CUIFScheme *cicCreateUIFScheme(DWORD type)
{
    return new(cicNoThrow) CUIFScheme(type);
}

/// @unimplemented
inline CUIFScheme::CUIFScheme(DWORD type)
{
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

inline BOOL cicSetLayout(HDC hDC, DWORD dwLayout)
{
    typedef BOOL (WINAPI *FN_SetLayout)(HDC hDC, DWORD dwLayout);
    static HINSTANCE s_hGdi32 = NULL;
    static FN_SetLayout s_fnSetLayout = NULL;

    if (!cicGetFN(s_hGdi32, s_fnSetLayout, TEXT("gdi32.dll"), "SetLayout"))
        return FALSE;

    return s_fnSetLayout(hDC, dwLayout);
}

inline HBITMAP cicMirrorBitmap(HBITMAP hBitmap, HBRUSH hbrBack)
{
    BITMAP bm;
    if (!CUIFBitmapDC::s_fInitBitmapDCs || !::GetObject(hBitmap, sizeof(bm), &bm))
        return NULL;

    CUIFBitmapDC::s_phdcSrc->SetBitmap(hBitmap);
    CUIFBitmapDC::s_phdcDst->SetDIB(bm.bmWidth, bm.bmHeight, 1, 32);
    CUIFBitmapDC::s_phdcMask->SetDIB(bm.bmWidth, bm.bmHeight, 1, 32);

    RECT rc;
    ::SetRect(&rc, 0, 0, bm.bmWidth, bm.bmHeight);
    FillRect(*CUIFBitmapDC::s_phdcDst, &rc, hbrBack);

    cicSetLayout(*CUIFBitmapDC::s_phdcMask, LAYOUT_RTL);

    ::BitBlt(*CUIFBitmapDC::s_phdcMask, 0, 0, bm.bmWidth, bm.bmHeight, *CUIFBitmapDC::s_phdcSrc, 0, 0, SRCCOPY);

    cicSetLayout(*CUIFBitmapDC::s_phdcMask, LAYOUT_LTR);

    ::BitBlt(*CUIFBitmapDC::s_phdcDst, 0, 0, bm.bmWidth, bm.bmHeight, *CUIFBitmapDC::s_phdcMask, 1, 0, SRCCOPY);

    CUIFBitmapDC::s_phdcSrc->Uninit(FALSE);
    CUIFBitmapDC::s_phdcMask->Uninit(FALSE);
    CUIFBitmapDC::s_phdcDst->Uninit(TRUE);
    return CUIFBitmapDC::s_phdcDst->DetachBitmap();
}

inline HBRUSH cicCreateDitherBrush(VOID)
{
    BYTE Bits[16];
    ZeroMemory(&Bits, sizeof(Bits));
    Bits[0] = Bits[4] = Bits[8] = Bits[12] = 'U';
    Bits[2] = Bits[6] = Bits[10] = Bits[14] = 0xAA;
    HBITMAP hBitmap = ::CreateBitmap(8, 8, 1, 1, Bits);
    if (!hBitmap)
        return NULL;

    LOGBRUSH lb;
    lb.lbHatch = (ULONG_PTR)hBitmap;
    lb.lbStyle = BS_PATTERN;
    HBRUSH hbr = ::CreateBrushIndirect(&lb);
    ::DeleteObject(hBitmap);
    return hbr;
}

inline HBITMAP
cicCreateDisabledBitmap(LPCRECT prc, HBITMAP hbmMask, HBRUSH hbr1, HBRUSH hbr2, BOOL bPressed)
{
    if (!CUIFBitmapDC::s_fInitBitmapDCs)
        return NULL;

    LONG width = prc->right - prc->left, height = prc->bottom - prc->top;

    CUIFBitmapDC::s_phdcDst->SetDIB(width, height, 1, 32);
    CUIFBitmapDC::s_phdcMask->SetBitmap(hbmMask);
    CUIFBitmapDC::s_phdcSrc->SetDIB(width, height, 1, 32);

    RECT rc;
    ::SetRect(&rc, 0, 0, width, height);
    ::FillRect(*CUIFBitmapDC::s_phdcDst, &rc, hbr1);

    HBRUSH hbrWhite = (HBRUSH)GetStockObject(WHITE_BRUSH);
    ::FillRect(*CUIFBitmapDC::s_phdcSrc, &rc, hbrWhite);

    ::BitBlt(*CUIFBitmapDC::s_phdcSrc, 0, 0, width, height, *CUIFBitmapDC::s_phdcMask, 0, 0, SRCINVERT);
    if (bPressed)
        BitBlt(*CUIFBitmapDC::s_phdcDst, 1, 1, width, height, *CUIFBitmapDC::s_phdcSrc, 0, 0, SRCPAINT);
    else
        BitBlt(*CUIFBitmapDC::s_phdcDst, 0, 0, width, height, *CUIFBitmapDC::s_phdcSrc, 0, 0, SRCPAINT);

    ::FillRect(*CUIFBitmapDC::s_phdcSrc, &rc, hbr2);

    ::BitBlt(*CUIFBitmapDC::s_phdcSrc, 0, 0, width, height, *CUIFBitmapDC::s_phdcMask, 0, 0, SRCPAINT);
    ::BitBlt(*CUIFBitmapDC::s_phdcDst, 0, 0, width, height, *CUIFBitmapDC::s_phdcSrc, 0, 0, SRCAND);

    CUIFBitmapDC::s_phdcSrc->Uninit(FALSE);
    CUIFBitmapDC::s_phdcMask->Uninit(FALSE);
    CUIFBitmapDC::s_phdcDst->Uninit(TRUE);
    return CUIFBitmapDC::s_phdcDst->DetachBitmap();
}

inline HBITMAP
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

    RECT rc;
    ::SetRect(&rc, 0, 0, width, height);

    ::FillRect(*CUIFBitmapDC::s_phdcDst, &rc, hbr1);
    ::FillRect(bitmapDC, &rc, hbr2);

    ::BitBlt(bitmapDC, 0, 0, width, height, *CUIFBitmapDC::s_phdcMask, 0, 0, SRCPAINT);
    ::BitBlt(*CUIFBitmapDC::s_phdcDst, 2, 2, width, height, bitmapDC, 0, 0, SRCAND);
    ::BitBlt(*CUIFBitmapDC::s_phdcDst, 0, 0, width, height, *CUIFBitmapDC::s_phdcMask, 0, 0, SRCAND);
    ::BitBlt(*CUIFBitmapDC::s_phdcDst, 0, 0, width, height, *CUIFBitmapDC::s_phdcSrc, 0, 0, SRCINVERT);

    CUIFBitmapDC::s_phdcSrc->Uninit(FALSE);
    CUIFBitmapDC::s_phdcMask->Uninit(FALSE);
    CUIFBitmapDC::s_phdcDst->Uninit(TRUE);
    return CUIFBitmapDC::s_phdcDst->DetachBitmap();
}

/////////////////////////////////////////////////////////////////////////////

inline CUIFWindow::CUIFWindow(HINSTANCE hInst, DWORD style)
    : CUIFObject(0, 0, 0, style)
{
    m_hInst = hInst;
    m_nLeft = 200;
    m_nTop = 200;
    m_nWidth = 200;
    m_nHeight = 200;
    m_hWnd = 0;
    m_pWindow = this;
    m_pCaptured = NULL;
    m_pUnknown7 = NULL;
    m_pPointed = NULL;
    m_dwUnknown8 = 0;
    m_pToolTip = NULL;
    m_pShadow = NULL;
    m_bShowShadow = TRUE;
    m_dwUnknown9 = 0;
    CUIFWindow::CreateScheme();
}

inline CUIFWindow::~CUIFWindow()
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

/// @unimplemented
inline STDMETHODIMP_(void)
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

    if (m_style & 0x20)
    {
        //FIXME: Create m_pToolTip
    }
    if (m_style & 0x40)
    {
        //FIXME: Create m_pShadow
    }

    return CUIFObject::Initialize();
}

inline STDMETHODIMP_(void)
CUIFWindow::OnUser(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  ;
}

inline STDMETHODIMP_(LRESULT)
CUIFWindow::OnSettingChange(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
}

inline void CUIFWindow::UpdateUI(LPCRECT prc)
{
    if (::IsWindow(m_hWnd))
        ::InvalidateRect(m_hWnd, prc, FALSE);
}

inline STDMETHODIMP_(void)
CUIFWindow::OnCreate(HWND hWnd)
{
}

inline STDMETHODIMP_(void)
CUIFWindow::OnDestroy(HWND hWnd)
{

}

inline STDMETHODIMP_(void)
CUIFWindow::OnNCDestroy(HWND hWnd)
{
}

inline STDMETHODIMP_(void)
CUIFWindow::OnKeyUp(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
}

inline CUIFWindow*
CUIFWindow::GetThis(HWND hWnd)
{
    return (CUIFWindow *)GetWindowLongPtr(hWnd, GWLP_USERDATA);
}

inline void
CUIFWindow::SetThis(HWND hWnd, LONG_PTR dwNewLong)
{
    ::SetWindowLongPtr(hWnd, GWLP_USERDATA, dwNewLong);
}

inline void CUIFWindow::CreateScheme()
{
    if (m_pScheme)
    {
        delete m_pScheme;
        m_pScheme = NULL;
    }

    INT iScheme = 0;
    if (m_style & 0x10000000)
        iScheme = 1;
    else if (m_style & 0x20000000)
        iScheme = 2;
    else if (m_style & 0x40000000)
        iScheme = 3;
    else
        iScheme = 0;

    m_pScheme = cicCreateUIFScheme(iScheme);
    SetScheme(m_pScheme);
}

inline LPCTSTR CUIFWindow::GetClassName()
{
    return TEXT("CiceroUIWndFrame");
}

inline LPCTSTR CUIFWindow::GetWndText()
{
    return TEXT("CiceroUIWndFrame");
}

inline STDMETHODIMP_(DWORD)
CUIFWindow::GetWndStyle()
{
    DWORD ret;

    if (m_style & 1)
        ret = WS_CHILD | WS_CLIPSIBLINGS;
    else
        ret = WS_POPUP | WS_DISABLED;

    if (m_style & 0x10000000)
    {
        ret |= WS_BORDER;
    }
    else
    {
        if (m_style & 8)
            ret |= WS_DLGFRAME;
        else if ((m_style & 0x20000000) || (m_style & 0x10))
            ret |= WS_BORDER;
    }

    return ret;
}

inline STDMETHODIMP_(DWORD)
CUIFWindow::GetWndStyleEx()
{
    DWORD ret = 0;
    if (m_style & 2)
        ret = WS_EX_TOPMOST;
    if (m_style & 4)
        ret |= WS_EX_TOOLWINDOW;
    if (m_style & 0x200)
        ret |= WS_EX_LAYOUTRTL;
    return ret;
}

inline STDMETHODIMP_(HWND)
CUIFWindow::CreateWnd(HWND hwndParent)
{
    DWORD style = GetWndStyle();
    LPCTSTR text = GetWndText();
    LPCTSTR clsName = GetClassName();
    DWORD exstyle = GetWndStyleEx();
    HWND hWnd = CreateWindowEx(exstyle,
                               clsName,
                               text,
                               style,
                               m_nLeft,
                               m_nTop,
                               m_nWidth,
                               m_nHeight,
                               hwndParent,
                               NULL,
                               m_hInst,
                               this);
    if (m_pToolTip)
        m_pToolTip->CreateWnd(hWnd);
    if (m_pShadow)
        m_pShadow->CreateWnd(hWnd);
    return hWnd;
}

inline void CUIFWindow::Show(BOOL bVisible)
{
    if (!IsWindow(m_hWnd))
        return;

    if (bVisible)
    {
        if (m_style & 2)
            ::SetWindowPos(m_hWnd, 0, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
    }
    m_bVisible = bVisible;
    ::ShowWindow(m_hWnd, (bVisible ? SW_SHOWNOACTIVATE : 0));
}

inline STDMETHODIMP_(BOOL)
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

inline void CUIFWindow::SetCaptureObject(CUIFObject *pCaptured)
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

inline STDMETHODIMP_(void)
CUIFWindow::SetCapture(BOOL bSet)
{
    if (bSet)
        ::SetCapture(m_hWnd);
    else
        ::ReleaseCapture();
}

/// @unimplemented
inline void CUIFWindow::SetObjectPointed(CUIFObject *pPointed, POINT pt)
{
    if (pPointed == m_pPointed)
        return;

    if (m_pCaptured)
    {
        if (m_pCaptured == m_pPointed)
        {
            if (m_pPointed->m_bEnable)
                m_pPointed->OnRButtonUp2(pt.x, pt.y); //FIXME
        }
    }
    else if (m_pPointed)
    {
        if ( m_pPointed->m_bEnable )
            m_pPointed->OnRButtonUp2(pt.x, pt.y); //FIXME
    }

    m_pPointed = pPointed;
    if ( m_pCaptured )
    {
        if (m_pCaptured != m_pPointed)
            return;
    }
    else if (!m_pPointed)
    {
        return;
    }

    if (m_pPointed->m_bEnable)
        m_pPointed->OnRButtonDblClk(pt.x, pt.y); //FIXME
}

inline STDMETHODIMP_(void)
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

inline STDMETHODIMP_(void)
CUIFWindow::SetRect(LPCRECT prc)
{
    RECT Rect;
    ::SetRectEmpty(&Rect);

    if (::IsWindow(m_hWnd))
        ::GetClientRect(m_hWnd, &Rect);

    CUIFObject::SetRect(&Rect);
}

inline STDMETHODIMP_(void)
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

inline STDMETHODIMP_(void)
CUIFWindow::GetWindowFrameSize(LPSIZE pSize)
{
    RECT rc;
    ::SetRectEmpty(&rc);

    ClientRectToWindowRect(&rc);
    pSize->cx = (rc.right - rc.left) / 2;
    pSize->cy = (rc.bottom - rc.top) / 2;
}

inline STDMETHODIMP_(void)
CUIFWindow::OnAnimationEnd()
{
    if (m_pShadow && m_bShowShadow)
        m_pShadow->Show(m_bVisible);
}

inline STDMETHODIMP_(void)
CUIFWindow::OnThemeChanged(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    ClearTheme();
}

/// @unimplemented
inline LRESULT
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
            //FIXME
            return 1;
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
            if (m_pToolTip && ::IsWindow(m_pToolTip->m_hWnd))
                ::DestroyWindow(m_pToolTip->m_hWnd);
            if (m_pShadow && ::IsWindow(m_pShadow->m_hWnd))
            {
                ::DestroyWindow(m_pShadow->m_hWnd);
            }
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
            return 0;
        case WM_TIMER:
            //FIXME
            return 0;
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
            //FIXME
            HandleMouseMsg(uMsg, (SHORT)LOWORD(lParam), (SHORT)HIWORD(lParam));
            return 0;
        }
        case WM_KEYUP:
            OnKeyUp(hWnd, wParam, lParam);
            return 0;
        case WM_WINDOWPOSCHANGING:
        {
            WINDOWPOS *pwp = (WINDOWPOS *)lParam;
            if (m_pShadow && (pwp->flags & SWP_HIDEWINDOW))
                m_pShadow->Show(FALSE);
            if (!(pwp->flags & SWP_NOZORDER) && pwp->hwndInsertAfter == m_pShadow->m_hWnd)
                pwp->flags |= SWP_NOZORDER;
            //m_pShadow->OnOwnerWndMoved(!(pwp->flags & SWP_NOSIZE)); //FIXME
            return OnWindowPosChanging(hWnd, WM_WINDOWPOSCHANGING, wParam, lParam);
        }
        case WM_WINDOWPOSCHANGED:
        {
            //WINDOWPOS *pwp = (WINDOWPOS *)lParam;
            //if (m_pShadow) //FIXME
            //    m_pShadow->OnOwnerWndMoved(!(pwp->flags & SWP_NOSIZE));
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
                return 0;
            }
            return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
        }
    }

    return 0;
}

inline LRESULT CALLBACK
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

inline BOOL
CUIFWindow::GetWorkArea(LPCRECT prcWnd, LPRECT prcWorkArea)
{
    if (!(m_style & 0x180))
        return 0;

    HMONITOR hMon = ::MonitorFromRect(prcWnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi;
    mi.cbSize = sizeof(MONITORINFO);
    if (!hMon || !::GetMonitorInfo(hMon, &mi))
    {
        if (m_style & 0x80)
            return ::SystemParametersInfoA(SPI_GETWORKAREA, 0, prcWorkArea, 0);
        if (m_style & 0x100)
        {
            prcWorkArea->top = 0;
            prcWorkArea->left = 0;
            prcWorkArea->right = ::GetSystemMetrics(SM_CXSCREEN);
            prcWorkArea->bottom = ::GetSystemMetrics(SM_CYSCREEN);
            return TRUE;
        }
        return FALSE;
    }

    if (m_style & 0x80)
    {
        *prcWorkArea = mi.rcWork;
        return TRUE;
    }

    if (m_style & 0x100)
    {
        *prcWorkArea = mi.rcMonitor;
        return TRUE;
    }

    return FALSE;
}

/// @unimplemented
inline void
CUIFWindow::AdjustWindowPosition()
{
    //FIXME
}

/// @unimplemented
inline STDMETHODIMP_(void)
CUIFWindow::PaintObject(HDC hDC, LPCRECT prc)
{
    BOOL bGotDC = FALSE;
    if (!hDC)
    {
        hDC = ::GetDC(m_hWnd);
        bGotDC = TRUE;
    }

    LPCRECT pRect = prc;
    if (!pRect)
        pRect = &m_rc;

    HDC hMemDC = ::CreateCompatibleDC(hDC);
    if (!hMemDC)
        return;

    HBITMAP hbmMem = ::CreateCompatibleBitmap(hDC,
                                              pRect->right - pRect->left,
                                              pRect->bottom - pRect->top);
    if (hbmMem)
    {
        HGDIOBJ hbmOld = ::SelectObject(hMemDC, hbmMem);
        ::SetViewportOrgEx(hMemDC, -pRect->left, -pRect->top, NULL);

        if ((FAILED(CUIFTheme::EnsureThemeData(m_hWnd)) ||
             !(m_style & 1) ||
             FAILED(DrawThemeParentBackground(m_hWnd, hMemDC, &m_rc))) &&
            FAILED(DrawThemeBackground(hMemDC, m_dwUnknown2, &m_rc, 0)))
        {
            //if (m_pScheme)
            //    m_pScheme->FillRect(hMemDC, pRect, 22); //FIXME
        }
        CUIFObject::PaintObject(hMemDC, pRect);
        ::BitBlt(hDC,
                 pRect->left, pRect->top,
                 pRect->right - pRect->left, pRect->bottom - pRect->top,
                 hMemDC,
                 pRect->left, pRect->top,
                 SRCCOPY);
        ::SelectObject(hMemDC, hbmOld);
        ::DeleteObject(hbmMem);
    }
    ::DeleteDC(hMemDC);

    if (bGotDC)
        ::ReleaseDC(m_hWnd, hDC);
}

inline STDMETHODIMP_(void)
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

inline STDMETHODIMP_(void)
CUIFWindow::RemoveUIObj(CUIFObject *pRemove)
{
    if (pRemove == m_pCaptured)
        SetCaptureObject(NULL);
    if (pRemove == m_pUnknown7)
    {
        m_pUnknown7 = NULL;
        ::KillTimer(m_hWnd, 0x5461);
    }
    if (pRemove == m_pPointed)
        m_pPointed = NULL;
    CUIFObject::RemoveUIObj(pRemove);
}

inline STDMETHODIMP_(void)
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

inline STDMETHODIMP_(void)
CUIFWindow::OnSetFocus(HWND hWnd)
{
}

/// @unimplemented
inline STDMETHODIMP_(void)
CUIFWindow::OnLButtonDown2(LONG x, LONG y)
{
    //FIXME
}

inline STDMETHODIMP_(void)
CUIFWindow::OnKillFocus(HWND hWnd)
{
}

inline STDMETHODIMP_(void)
CUIFWindow::OnNotify(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
}

/// @unimplemented
inline STDMETHODIMP_(void)
CUIFWindow::OnTimer(HWND hWnd)
{
    //FIXME
}

inline STDMETHODIMP_(void)
CUIFWindow::OnSysColorChange()
{
}

inline STDMETHODIMP_(void)
CUIFWindow::OnEndSession(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
}

inline STDMETHODIMP_(void)
CUIFWindow::OnKeyDown(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
}

inline STDMETHODIMP_(LRESULT)
CUIFWindow::OnActivate(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return 0;
}

inline STDMETHODIMP_(LRESULT)
CUIFWindow::OnWindowPosChanged(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return 0;
}

inline STDMETHODIMP_(LRESULT)
CUIFWindow::OnWindowPosChanging(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return 0;
}

inline STDMETHODIMP_(LRESULT)
CUIFWindow::OnNotifyFormat(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    return 0;
}

inline STDMETHODIMP_(LRESULT)
CUIFWindow::OnShowWindow(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return 0;
}

inline STDMETHODIMP_(LRESULT)
CUIFWindow::OnDisplayChange(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return 0;
}

inline STDMETHODIMP_(LRESULT)
CUIFWindow::OnGetObject(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return 0;
}

inline STDMETHODIMP_(BOOL)
CUIFWindow::OnEraseBkGnd(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return FALSE;
}

inline STDMETHODIMP_(void)
CUIFWindow::OnSetCapture(HWND hWnd, UINT, LONG)
{
}

inline STDMETHODIMP_(void)
CUIFWindow::OnAnimationStart()
{
}
