/*
 * PROJECT:     ReactOS Cicero
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Cicero UIF Library
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#include "cicarray.h"

// This is the Cicero UIF Library, used to support the IME UI interface.
// The Cicero UIF Library implements some GUI parts for IMEs and Language Bar.
// The GUI parts of the UIF Library requires special handling because:
//
// 1. To avoid interfering with IME input, the GUI part should not receive focus.
// 2. The IME popup window has WS_DISABLED style, so it cannot receive mouse messages
//    directly.

class CUIFSystemInfo;
class CUIFTheme;
    class CUIFObject;
        class CUIFWindow;
            class CUIFToolTip;
            class CUIFShadow;
            class CUIFBalloonWindow;
            class CUIFMenu;
        class CUIFButton;
            class CUIFButton2;
                class CUIFToolbarMenuButton;
                class CUIFToolbarButtonElement;
            class CUIFBalloonButton;
        class CUIFToolbarButton;
        class CUIFWndFrame;
        class CUIFGripper;
        class CUIFMenuItem;
            class CUIFMenuItemSeparator;
class CUIFObjectArray;
class CUIFColorTable;
    class CUIFColorTableSys;
    class CUIFColorTableOff10;
class CUIFBitmapDC;
class CUIFIcon;
class CUIFSolidBrush;
class CUIFScheme;

/////////////////////////////////////////////////////////////////////////////

class CUIFSystemInfo : OSVERSIONINFO
{
public:
    static CUIFSystemInfo *s_pSystemInfo;
    DWORD m_cBitsPixels;
    BOOL m_bHighContrast1;
    BOOL m_bHighContrast2;

    CUIFSystemInfo() { }
    void GetSystemMetrics();
    void Initialize();
};

/////////////////////////////////////////////////////////////////////////////

#include <uxtheme.h>
#include <vsstyle.h>

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

class CUIFTheme
{
public:
    LPCWSTR m_pszClassList;
    INT m_iPartId;
    INT m_iStateId;
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
    STDMETHOD_(void, SetActiveTheme)(LPCWSTR pszClassList, INT iPartId, INT iStateId);
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

        ptrdiff_t iItem = Find(pObject);
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
    DWORD m_nObjectID;
    DWORD m_style;
    RECT m_rc;
    BOOL m_bEnable;
    BOOL m_bVisible;
    HFONT m_hFont;
    BOOL m_bHasCustomFont;
    LPWSTR m_pszToolTip;
    DWORD m_dwUnknown4[2]; //FIXME: name and type
    friend class CUIFWindow;
    friend class CUIFToolTip;
    friend class CUIFBalloonWindow;

public:
    CUIFObject(CUIFObject *pParent, DWORD nObjectID, LPCRECT prc, DWORD style);
    virtual ~CUIFObject();

    void StartCapture();
    void EndCapture();
    BOOL IsCapture();
    BOOL IsRTL();
    LRESULT NotifyCommand(WPARAM wParam, LPARAM lParam);
    CUIFObject* ObjectFromPoint(POINT pt);
    void SetScheme(CUIFScheme *scheme);
    void StartTimer(WPARAM wParam);
    void EndTimer();

    STDMETHOD_(BOOL, Initialize)() { return TRUE; }
    STDMETHOD_(void, OnPaint)(HDC hDC);
    STDMETHOD_(void, OnTimer)() { }
    STDMETHOD_(void, OnLButtonDown)(LONG x, LONG y) { }
    STDMETHOD_(void, OnMButtonDown)(LONG x, LONG y) { }
    STDMETHOD_(void, OnRButtonDown)(LONG x, LONG y) { }
    STDMETHOD_(void, OnLButtonUp)(LONG x, LONG y) { }
    STDMETHOD_(void, OnMButtonUp)(LONG x, LONG y) { }
    STDMETHOD_(void, OnRButtonUp)(LONG x, LONG y) { }
    STDMETHOD_(void, OnMouseMove)(LONG x, LONG y) { }
    STDMETHOD_(void, OnMouseIn)(LONG x, LONG y) { }
    STDMETHOD_(void, OnMouseOut)(LONG x, LONG y) { }
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
    STDMETHOD_(LPCWSTR, GetToolTip)() { return m_pszToolTip; }
    STDMETHOD_(LRESULT, OnShowToolTip)() { return 0; }
    STDMETHOD_(void, OnHideToolTip)() { }
    STDMETHOD_(void, DetachWndObj)();
    STDMETHOD_(void, ClearWndObj)();
    STDMETHOD_(BOOL, OnPaintTheme)(HDC hDC) { return FALSE; }
    STDMETHOD_(void, OnPaintNoTheme)(HDC hDC) { }
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

    COLORREF GetColor(INT iColor) const { return m_rgbColors[iColor]; }
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

    COLORREF GetColor(INT iColor) const { return m_rgbColors[iColor]; }
    HBRUSH GetBrush(INT iColor);

    STDMETHOD_(void, InitColor)() override;
    STDMETHOD_(void, InitBrush)() override;
    STDMETHOD_(void, DoneBrush)() override;
};

/////////////////////////////////////////////////////////////////////////////

class CUIFSolidBrush
{
public:
    HBRUSH m_hBrush;

    operator HBRUSH() const { return m_hBrush; }

    CUIFSolidBrush(COLORREF rgbColor)
    {
        m_hBrush = ::CreateSolidBrush(rgbColor);
    }
    ~CUIFSolidBrush()
    {
        if (m_hBrush)
        {
            ::DeleteObject(m_hBrush);
            m_hBrush = NULL;
        }
    }
};

/////////////////////////////////////////////////////////////////////////////

class CUIFIcon
{
public:
    HICON m_hIcon;
    HIMAGELIST m_hImageList;

    CUIFIcon& operator=(HICON hIcon)
    {
        m_hIcon = hIcon;
        if (m_hImageList)
        {
            ImageList_Destroy(m_hImageList);
            m_hImageList = NULL;
        }
        return *this;
    }

    HIMAGELIST GetImageList(BOOL bMirror);
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

    void Uninit(BOOL bKeep = FALSE);

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

HBITMAP cicMirrorBitmap(HBITMAP hBitmap, HBRUSH hbrBack);
HBRUSH cicCreateDitherBrush(VOID);
HBITMAP cicCreateDisabledBitmap(LPCRECT prc, HBITMAP hbmMask, HBRUSH hbr1, HBRUSH hbr2,
                                BOOL bPressed);
HBITMAP cicCreateShadowMaskBmp(LPRECT prc, HBITMAP hbm1, HBITMAP hbm2, HBRUSH hbr1, HBRUSH hbr2);
HBITMAP cicChangeBitmapColor(LPCRECT prc, HBITMAP hbm, COLORREF rgbBack, COLORREF rgbFore);
HBITMAP cicConvertBlackBKGBitmap(LPCRECT prc, HBITMAP hbm1, HBITMAP hbm2, HBRUSH hBrush);
HBITMAP cicCreateMaskBmp(LPCRECT prc, HBITMAP hbm1, HBITMAP hbm2, HBRUSH hbr,
                         COLORREF rgbColor, COLORREF rgbBack);
BOOL cicGetIconBitmaps(HICON hIcon, HBITMAP *hbm1, HBITMAP *hbm2, const SIZE *pSize);
void cicDrawMaskBmpOnDC(HDC hDC, LPCRECT prc, HBITMAP hbmp, HBITMAP hbmpMask);

/////////////////////////////////////////////////////////////////////////////

// Flags for dwDrawFlags
enum
{
    UIF_DRAW_PRESSED = 0x10,
    UIF_DRAW_DISABLED = 0x20,
};

class CUIFScheme
{
public:
    static CUIFColorTableSys *s_pColorTableSys;
    static CUIFColorTableOff10 *s_pColorTableOff10;
    BOOL m_bMirroring;

    CUIFScheme() : m_bMirroring(FALSE) { }
    virtual ~CUIFScheme() { }

    STDMETHOD_(DWORD, GetType)() = 0;
    STDMETHOD_(COLORREF, GetColor)(INT iColor) = 0;
    STDMETHOD_(HBRUSH, GetBrush)(INT iColor) = 0;
    STDMETHOD_(INT, CyMenuItem)(INT cyText) = 0;
    STDMETHOD_(INT, CxSizeFrame)() = 0;
    STDMETHOD_(INT, CySizeFrame)() = 0;
    STDMETHOD_(INT, CxWndBorder)() = 0;
    STDMETHOD_(INT, CyWndBorder)() = 0;
    STDMETHOD_(void, FillRect)(HDC hDC, LPCRECT prc, INT iColor);
    STDMETHOD_(void, FrameRect)(HDC hDC, LPCRECT prc, INT iColor);
    STDMETHOD_(void, DrawSelectionRect)(HDC hDC, LPCRECT prc, int) = 0;
    STDMETHOD_(void, GetCtrlFaceOffset)(DWORD, DWORD dwDrawFlags, LPSIZE pSize) = 0;
    STDMETHOD_(void, DrawCtrlBkgd)(HDC hDC, LPCRECT prc, DWORD dwUnknownFlags, DWORD dwDrawFlags) = 0;
    STDMETHOD_(void, DrawCtrlEdge)(HDC hDC, LPCRECT prc, DWORD dwUnknownFlags, DWORD dwDrawFlags) = 0;
    STDMETHOD_(void, DrawCtrlText)(HDC hDC, LPCRECT prc, LPCWSTR pszText, INT cchText, DWORD dwDrawFlags, BOOL bRight) = 0;
    STDMETHOD_(void, DrawCtrlIcon)(HDC hDC, LPCRECT prc, HICON hIcon, DWORD dwDrawFlags, LPSIZE pSize) = 0;
    STDMETHOD_(void, DrawCtrlBitmap)(HDC hDC, LPCRECT prc, HBITMAP hbm1, HBITMAP hbm2, DWORD dwDrawFlags) = 0;
    STDMETHOD_(void, DrawMenuBitmap)(HDC hDC, LPCRECT prc, HBITMAP hbm1, HBITMAP hbm2, DWORD dwDrawFlags) = 0;
    STDMETHOD_(void, DrawMenuSeparator)(HDC hDC, LPCRECT prc) = 0;
    STDMETHOD_(void, DrawFrameCtrlBkgd)(HDC hDC, LPCRECT prc, DWORD dwUnknownFlags, DWORD dwDrawFlags) = 0;
    STDMETHOD_(void, DrawFrameCtrlEdge)(HDC hDC, LPCRECT prc, DWORD dwUnknownFlags, DWORD dwDrawFlags) = 0;
    STDMETHOD_(void, DrawFrameCtrlIcon)(HDC hDC, LPCRECT prc, HICON hIcon, DWORD dwDrawFlags, LPSIZE pSize) = 0;
    STDMETHOD_(void, DrawFrameCtrlBitmap)(HDC hDC, LPCRECT prc, HBITMAP hbm1, HBITMAP hbm2, DWORD dwDrawFlags) = 0;
    STDMETHOD_(void, DrawWndFrame)(HDC hDC, LPCRECT prc, DWORD type, DWORD unused1, DWORD unused2) = 0;
    STDMETHOD_(void, DrawDragHandle)(HDC hDC, LPCRECT prc, BOOL bVertical) = 0;
    STDMETHOD_(void, DrawSeparator)(HDC hDC, LPCRECT prc, BOOL bVertical) = 0;
};

class CUIFSchemeDef : public CUIFScheme
{
protected:
    DWORD m_dwType;

public:
    CUIFSchemeDef(DWORD dwType) : m_dwType(dwType) { }

    STDMETHOD_(DWORD, GetType)() override;
    STDMETHOD_(COLORREF, GetColor)(INT iColor) override;
    STDMETHOD_(HBRUSH, GetBrush)(INT iColor) override;
    STDMETHOD_(INT, CyMenuItem)(INT cyText) override;
    STDMETHOD_(INT, CxSizeFrame)() override;
    STDMETHOD_(INT, CySizeFrame)() override;
    STDMETHOD_(INT, CxWndBorder)() override { return 1; }
    STDMETHOD_(INT, CyWndBorder)() override { return 1; }
    STDMETHOD_(void, DrawSelectionRect)(HDC hDC, LPCRECT prc, int) override;
    STDMETHOD_(void, GetCtrlFaceOffset)(DWORD dwUnknownFlags, DWORD dwDrawFlags, LPSIZE pSize) override;
    STDMETHOD_(void, DrawCtrlBkgd)(HDC hDC, LPCRECT prc, DWORD dwUnknownFlags, DWORD dwDrawFlags) override;
    STDMETHOD_(void, DrawCtrlEdge)(HDC hDC, LPCRECT prc, DWORD dwUnknownFlags, DWORD dwDrawFlags) override;
    STDMETHOD_(void, DrawCtrlText)(HDC hDC, LPCRECT prc, LPCWSTR pszText, INT cchText, DWORD dwDrawFlags, BOOL bRight) override;
    STDMETHOD_(void, DrawCtrlIcon)(HDC hDC, LPCRECT prc, HICON hIcon, DWORD dwDrawFlags, LPSIZE pSize) override;
    STDMETHOD_(void, DrawCtrlBitmap)(HDC hDC, LPCRECT prc, HBITMAP hbm1, HBITMAP hbm2, DWORD dwDrawFlags) override;
    STDMETHOD_(void, DrawMenuBitmap)(HDC hDC, LPCRECT prc, HBITMAP hbm1, HBITMAP hbm2, DWORD dwDrawFlags) override;
    STDMETHOD_(void, DrawMenuSeparator)(HDC hDC, LPCRECT prc) override;
    STDMETHOD_(void, DrawFrameCtrlBkgd)(HDC hDC, LPCRECT prc, DWORD dwUnknownFlags, DWORD dwDrawFlags) override;
    STDMETHOD_(void, DrawFrameCtrlEdge)(HDC hDC, LPCRECT prc, DWORD dwUnknownFlags, DWORD dwDrawFlags) override;
    STDMETHOD_(void, DrawFrameCtrlIcon)(HDC hDC, LPCRECT prc, HICON hIcon, DWORD dwDrawFlags, LPSIZE pSize) override;
    STDMETHOD_(void, DrawFrameCtrlBitmap)(HDC hDC, LPCRECT prc, HBITMAP hbm1, HBITMAP hbm2, DWORD dwDrawFlags) override;
    STDMETHOD_(void, DrawWndFrame)(HDC hDC, LPCRECT prc, DWORD type, DWORD unused1, DWORD unused2) override;
    STDMETHOD_(void, DrawDragHandle)(HDC hDC, LPCRECT prc, BOOL bVertical) override;
    STDMETHOD_(void, DrawSeparator)(HDC hDC, LPCRECT prc, BOOL bVertical) override;
};

CUIFScheme *cicCreateUIFScheme(DWORD type);

/////////////////////////////////////////////////////////////////////////////

// m_style flags for CUIFWindow
enum
{
    UIF_WINDOW_CHILD = 0x1,
    UIF_WINDOW_TOPMOST = 0x2,
    UIF_WINDOW_TOOLWINDOW = 0x4,
    UIF_WINDOW_DLGFRAME = 0x8,
    UIF_WINDOW_TOOLTIP = 0x20,
    UIF_WINDOW_SHADOW = 0x40,
    UIF_WINDOW_WORKAREA = 0x80,
    UIF_WINDOW_MONITOR = 0x100,
    UIF_WINDOW_LAYOUTRTL = 0x200,
    UIF_WINDOW_NOMOUSEMSG = 0x400,
    UIF_WINDOW_USESCHEME1 = 0x10000000,
    UIF_WINDOW_USESCHEME2 = 0x20000000,
    UIF_WINDOW_USESCHEME3 = 0x40000000,
    UIF_WINDOW_ENABLETHEMED = 0x80000000,
};

class CUIFWindow : public CUIFObject
{
protected:
    INT m_nLeft;
    INT m_nTop;
    INT m_nHeight;
    INT m_nWidth;
    HINSTANCE m_hInst;
    HWND m_hWnd;
    CUIFObject *m_pTimerObject;
    CUIFObject *m_pCaptured;
    CUIFObject *m_pPointed;
    BOOL m_bPointing;
    CUIFWindow *m_pBehindModal;
    CUIFToolTip *m_pToolTip;
    CUIFShadow *m_pShadow;
    BOOL m_bShowShadow;
    friend class CUIFObject;
    friend class CUIFShadow;
    friend class CUIFToolTip;
    friend class CUIFButton;
    friend class CUIFMenu;

public:
    enum { POINTING_TIMER_ID = 0x7982, USER_TIMER_ID = 0x5461 };
    operator HWND() const { return m_hWnd; }
    CUIFWindow(HINSTANCE hInst, DWORD style);
    ~CUIFWindow() override;

    static CUIFWindow* GetThis(HWND hWnd);
    static void SetThis(HWND hWnd, LONG_PTR dwNewLong);

    STDMETHOD_(BOOL, Initialize)() override;
    STDMETHOD_(void, Show)(BOOL bVisible) override;
    STDMETHOD_(void, SetRect)(LPCRECT prc) override;
    STDMETHOD_(void, PaintObject)(HDC hDC, LPCRECT prc) override;
    STDMETHOD_(void, RemoveUIObj)(CUIFObject *pRemove) override;

    void SetCaptureObject(CUIFObject *pCaptured);
    void SetObjectPointed(CUIFObject *pPointed, POINT pt);
    void CreateScheme();
    BOOL GetWorkArea(LPCRECT prcWnd, LPRECT prcWorkArea);
    void AdjustWindowPosition();
    void SetBehindModal(CUIFWindow *pBehindModal);
    void SetTimerObject(CUIFObject *pTimerObject, UINT uElapse);

    static LRESULT CALLBACK WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    STDMETHOD_(LPCTSTR, GetClassName)() { return TEXT("CiceroUIWndFrame"); }
    STDMETHOD_(LPCTSTR, GetWndTitle)() { return TEXT("CiceroUIWndFrame"); }
    STDMETHOD_(DWORD, GetWndStyle)();
    STDMETHOD_(DWORD, GetWndStyleEx)();
    STDMETHOD_(HWND, CreateWnd)(HWND hwndParent);
    STDMETHOD_(void, Move)(INT x, INT y, INT nWidth, INT nHeight);
    STDMETHOD_(BOOL, AnimateWnd)(DWORD dwTime, DWORD dwFlags);
    STDMETHOD_(void, OnObjectMoved)(CUIFObject *pObject);
    STDMETHOD_(void, OnMouseOutFromWindow)(LONG x, LONG y) { }
    STDMETHOD_(void, OnCreate)(HWND hWnd) { }
    STDMETHOD_(void, OnDestroy)(HWND hWnd) { }
    STDMETHOD_(void, OnNCDestroy)(HWND hWnd) { }
    STDMETHOD_(void, OnSetFocus)(HWND hWnd) { }
    STDMETHOD_(void, OnKillFocus)(HWND hWnd) { }
    STDMETHOD_(void, OnNotify)(HWND hWnd, WPARAM wParam, LPARAM lParam) { }
    STDMETHOD_(void, OnTimer)(WPARAM wParam) { }
    STDMETHOD_(void, OnSysColorChange)() { }
    STDMETHOD_(void, OnEndSession)(HWND hWnd, WPARAM wParam, LPARAM lParam) { }
    STDMETHOD_(void, OnKeyDown)(HWND hWnd, WPARAM wParam, LPARAM lParam) { }
    STDMETHOD_(void, OnKeyUp)(HWND, WPARAM wParam, LPARAM lParam) { }
    STDMETHOD_(void, OnUser)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) { }
    STDMETHOD_(LRESULT, OnActivate)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
        { return 0; }
    STDMETHOD_(LRESULT, OnWindowPosChanged)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
        { return 0; }
    STDMETHOD_(LRESULT, OnWindowPosChanging)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
        { return 0; }
    STDMETHOD_(LRESULT, OnNotifyFormat)(HWND hWnd, WPARAM wParam, LPARAM lParam) { return 0; }
    STDMETHOD_(LRESULT, OnShowWindow)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
        { return 0; }
    STDMETHOD_(LRESULT, OnSettingChange)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    STDMETHOD_(LRESULT, OnDisplayChange)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
        { return 0; }
    STDMETHOD_(HRESULT, OnGetObject)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
        { return S_OK; }
    STDMETHOD_(LRESULT, WindowProc)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    STDMETHOD_(BOOL, OnEraseBkGnd)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
        { return FALSE; }
    STDMETHOD_(void, OnThemeChanged)(HWND hWnd, WPARAM wParam, LPARAM lParam);
    STDMETHOD_(void, UpdateUI)(LPCRECT prc);
    STDMETHOD_(void, SetCapture)(int);
    STDMETHOD_(void, ModalMouseNotify)(UINT uMsg, LONG x, LONG y) { }
    STDMETHOD_(void, OnAnimationStart)() { }
    STDMETHOD_(void, OnAnimationEnd)();
    STDMETHOD_(void, HandleMouseMsg)(UINT uMsg, LONG x, LONG y);
    STDMETHOD_(void, ClientRectToWindowRect)(LPRECT pRect);
    STDMETHOD_(void, GetWindowFrameSize)(LPSIZE pSize);
};

/////////////////////////////////////////////////////////////////////////////

class CUIFToolTip : public CUIFWindow
{
protected:
    CUIFWindow *m_pToolTipOwner;
    CUIFObject *m_pToolTipTarget;
    LPWSTR m_pszToolTipText;
    BOOL m_bShowToolTip;
    DWORD m_dwUnknown10; //FIXME: name and type
    LONG m_nDelayTimeType2;
    LONG m_nDelayTimeType3;
    LONG m_nDelayTimeType1;
    RECT m_rcToolTipMargin;
    LONG m_cxToolTipWidth;
    BOOL m_bToolTipHasBkColor;
    BOOL m_bToolTipHasTextColor;
    COLORREF m_rgbToolTipBkColor;
    COLORREF m_rgbToolTipTextColor;
    friend class CUIFObject;
    friend class CTipbarWnd;

public:
    enum { TOOLTIP_TIMER_ID = 0x3216 };
    CUIFToolTip(HINSTANCE hInst, DWORD style, CUIFWindow *pToolTipOwner);
    ~CUIFToolTip() override;

    LONG GetDelayTime(UINT uType);
    void GetMargin(LPRECT prc);
    COLORREF GetTipBkColor();
    COLORREF GetTipTextColor();
    CUIFObject* FindObject(HWND hWnd, POINT pt);

    void ShowTip();
    void HideTip();

    void GetTipWindowSize(LPSIZE pSize);
    void GetTipWindowRect(LPRECT pRect, SIZE toolTipSize, LPCRECT prc);

    void RelayEvent(LPMSG pMsg);

    STDMETHOD_(void, OnPaint)(HDC hDC) override;
    STDMETHOD_(void, Enable)(BOOL bEnable) override;
    STDMETHOD_(void, OnTimer)(WPARAM wParam) override;
};

class CUIFShadow : public CUIFWindow
{
protected:
    CUIFWindow *m_pShadowOwner;
    COLORREF m_rgbShadowColor;
    DWORD m_dwUnknown11[2];
    INT m_xShadowDelta;
    INT m_yShadowDelta;
    BOOL m_bLayerAvailable;

public:
    CUIFShadow(HINSTANCE hInst, DWORD style, CUIFWindow *pShadowOwner);
    ~CUIFShadow() override;

    void InitSettings();
    void InitShadow();
    void AdjustWindowPos();
    void OnOwnerWndMoved(BOOL bDoSize);

    STDMETHOD_(BOOL, Initialize)() override;
    STDMETHOD_(DWORD, GetWndStyleEx)() override;
    STDMETHOD_(void, OnPaint)(HDC hDC) override;
    STDMETHOD_(LRESULT, OnWindowPosChanging)(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) override;
    STDMETHOD_(LRESULT, OnSettingChange)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
    STDMETHOD_(void, Show)(BOOL bVisible) override;
};

/////////////////////////////////////////////////////////////////////////////

// m_style flags for CUIFMenu
enum
{
    UIF_MENU_USE_OFF10 = 0x10000000,
};

class CUIFMenu : public CUIFWindow
{
public:
    CUIFMenu *m_pVisibleSubMenu;
    CUIFMenu *m_pParentMenu;
    CUIFMenuItem *m_pSelectedItem;
    UINT m_nSelectedID;
    CicArray<CUIFMenuItem*> m_MenuItems;
    HFONT m_hMenuFont;
    BOOL m_bModal;
    BOOL m_bHasMargin;
    DWORD m_dwUnknown14;
    LONG m_cxyMargin;
    LONG m_cxMenuExtent;
    friend class CUIFMenuItem;

public:
    CUIFMenu(HINSTANCE hInst, DWORD style, DWORD dwUnknown14);
    ~CUIFMenu() override;

    void CancelMenu();
    void ClearMenuFont();
    CUIFMenuItem* GetNextItem(CUIFMenuItem *pItem);
    CUIFMenuItem* GetPrevItem(CUIFMenuItem *pItem);
    CUIFMenu* GetTopSubMenu();
    BOOL InsertItem(CUIFMenuItem *pItem);
    BOOL InsertSeparator();
    void PostKey(BOOL bUp, WPARAM wParam, LPARAM lParam);
    void SetMenuFont();
    void SetSelectedId(UINT nSelectID);
    void SetSelectedItem(CUIFMenuItem *pItem);
    UINT ShowModalPopup(CUIFWindow *pWindow, LPCRECT prc, BOOL bFlag);
    void ShowSubPopup(CUIFMenu *pSubMenu, LPCRECT prc, BOOL bFlag);

    STDMETHOD_(void, OnKeyDown)(HWND hWnd, WPARAM wParam, LPARAM lParam) override;
    STDMETHOD_(void, HandleMouseMsg)(UINT uMsg, LONG x, LONG y) override;
    STDMETHOD_(void, ModalMouseNotify)(UINT uMsg, LONG x, LONG y) override;

    STDMETHOD_(void, ModalMessageLoop)();
    STDMETHOD_(BOOL, InitShow)(CUIFWindow *pWindow, LPCRECT prc, BOOL bFlag, BOOL bDoAnimation);
    STDMETHOD_(BOOL, UninitShow)();
};

/////////////////////////////////////////////////////////////////////////////

class CUIFMenuItem : public CUIFObject
{
protected:
    UINT m_nMenuItemID;
    LPWSTR m_pszMenuItemLeft;
    UINT m_cchMenuItemLeft;
    LPWSTR m_pszMenuItemRight;
    UINT m_cchMenuItemRight;
    UINT m_nMenuItemVKey;
    UINT m_ichMenuItemPrefix;
    HBITMAP m_hbmColor;
    HBITMAP m_hbmMask;
    BOOL m_bMenuItemChecked;
    BOOL m_bMenuItemForceChecked;
    BOOL m_bMenuItemGrayed;
    BOOL m_bMenuItemDisabled;
    CUIFMenu *m_pMenu;
    CUIFMenu *m_pSubMenu;
    SIZE m_MenuLeftExtent;
    SIZE m_MenuRightExtent;
    friend class CUIFMenu;

    void DrawArrow(HDC hDC, INT x, INT y);
    void DrawBitmapProc(HDC hDC, INT xLeft, INT yTop);
    void DrawCheck(HDC hDC, INT xLeft, INT yTop);
    void DrawUnderline(HDC hDC, INT xText, INT yText, HBRUSH hbr);

public:
    CUIFMenuItem(CUIFMenu *pMenu, BOOL bDisabled = FALSE);
    ~CUIFMenuItem() override;

    BOOL Init(UINT nMenuItemID, LPCWSTR pszText);

    BOOL IsCheck();
    void Check(BOOL bChecked) { m_bMenuItemChecked = bChecked; }
    void Gray(BOOL bGrayed) { m_bMenuItemGrayed = bGrayed; }

    void SetBitmap(HBITMAP hbmColor) { m_hbmColor = hbmColor; }
    void SetBitmapMask(HBITMAP hbmMask);
    void SetSub(CUIFMenu *pSubMenu) { m_pSubMenu = pSubMenu; }

    void ShowSubPopup();

    STDMETHOD_(void, OnLButtonUp)(LONG x, LONG y) override;
    STDMETHOD_(void, OnMouseIn)(LONG x, LONG y) override;
    STDMETHOD_(void, OnPaint)(HDC hDC) override;
    STDMETHOD_(void, OnTimer)() override;

    STDMETHOD_(void, InitMenuExtent)();
    STDMETHOD_(void, OnPaintDef)(HDC hDC);
    STDMETHOD_(void, OnPaintO10)(HDC hDC);
    STDMETHOD_(void, OnUnknownMethod)() { } // FIXME: method name
};

/////////////////////////////////////////////////////////////////////////////

class CUIFMenuItemSeparator : public CUIFMenuItem
{
public:
    CUIFMenuItemSeparator(CUIFMenu *pMenu);

    STDMETHOD_(void, InitMenuExtent)() override;
    STDMETHOD_(void, OnPaintDef)(HDC hDC) override;
    STDMETHOD_(void, OnPaintO10)(HDC hDC) override;
};

/////////////////////////////////////////////////////////////////////////////

// m_style flags for CUIFButton
enum
{
    UIF_BUTTON_H_ALIGN_LEFT = 0,
    UIF_BUTTON_H_ALIGN_CENTER = 0x1,
    UIF_BUTTON_H_ALIGN_RIGHT = 0x2,
    UIF_BUTTON_H_ALIGN_MASK = UIF_BUTTON_H_ALIGN_CENTER | UIF_BUTTON_H_ALIGN_RIGHT,
    UIF_BUTTON_V_ALIGN_TOP = 0,
    UIF_BUTTON_V_ALIGN_MIDDLE = 0x4,
    UIF_BUTTON_V_ALIGN_BOTTOM = 0x8,
    UIF_BUTTON_V_ALIGN_MASK = UIF_BUTTON_V_ALIGN_MIDDLE | UIF_BUTTON_V_ALIGN_BOTTOM,
    UIF_BUTTON_LARGE_ICON = 0x100,
    UIF_BUTTON_VERTICAL = 0x400,
};

class CUIFButton : public CUIFObject
{
protected:
    UINT m_uButtonStatus;
    LPWSTR m_pszButtonText;
    CUIFIcon m_ButtonIcon;
    DWORD m_dwUnknown9;
    HBITMAP m_hbmButton1;
    HBITMAP m_hbmButton2;
    BOOL m_bPressed;
    SIZE m_IconSize;
    SIZE m_TextSize;
    friend class CUIFToolbarButton;

    void DrawBitmapProc(HDC hDC, LPCRECT prc, BOOL bPressed);
    void DrawEdgeProc(HDC hDC, LPCRECT prc, BOOL bPressed);
    void DrawIconProc(HDC hDC, LPRECT prc, BOOL bPressed);
    void DrawTextProc(HDC hDC, LPCRECT prc, BOOL bPressed);

public:
    CUIFButton(CUIFObject *pParent, DWORD nObjectID, LPCRECT prc, DWORD style);
    ~CUIFButton() override;

    void SetIcon(HICON hIcon);
    void SetText(LPCWSTR pszText);

    void GetIconSize(HICON hIcon, LPSIZE pSize);
    void GetTextSize(LPCWSTR pszText, LPSIZE pSize);

    STDMETHOD_(void, Enable)(BOOL bEnable) override;
    STDMETHOD_(void, OnMouseIn)(LONG x, LONG y) override;
    STDMETHOD_(void, OnMouseOut)(LONG x, LONG y) override;
    STDMETHOD_(void, OnLButtonDown)(LONG x, LONG y) override;
    STDMETHOD_(void, OnLButtonUp)(LONG x, LONG y) override;
    STDMETHOD_(void, OnPaintNoTheme)(HDC hDC) override;
    STDMETHOD_(void, SetStatus)(UINT uStatus);
};

/////////////////////////////////////////////////////////////////////////////

class CUIFButton2 : public CUIFButton
{
protected:
    SIZE m_BitmapSize;

public:
    CUIFButton2(CUIFObject *pParent, DWORD nObjectID, LPCRECT prc, DWORD style);
    ~CUIFButton2() override;

    DWORD MakeDrawFlag();
    STDMETHOD_(BOOL, OnPaintTheme)(HDC hDC) override;
    STDMETHOD_(void, OnPaintNoTheme)(HDC hDC) override;
};

/////////////////////////////////////////////////////////////////////////////

class CUIFToolbarMenuButton : public CUIFButton2
{
public:
    CUIFToolbarButton *m_pToolbarButton;

    CUIFToolbarMenuButton(CUIFToolbarButton *pParent, DWORD nObjectID, LPCRECT prc, DWORD style);
    ~CUIFToolbarMenuButton() override;

    STDMETHOD_(void, OnLButtonUp)(LONG x, LONG y) override;
    STDMETHOD_(BOOL, OnSetCursor)(UINT uMsg, LONG x, LONG y) override;
};

/////////////////////////////////////////////////////////////////////////////

class CUIFToolbarButtonElement : public CUIFButton2
{
public:
    CUIFToolbarButton *m_pToolbarButton;

    CUIFToolbarButtonElement(CUIFToolbarButton *pParent, DWORD nObjectID, LPCRECT prc, DWORD style);

    STDMETHOD_(LPCWSTR, GetToolTip)() override;
    STDMETHOD_(void, OnLButtonUp)(LONG x, LONG y) override;
    STDMETHOD_(void, OnRButtonUp)(LONG x, LONG y) override;
};

/////////////////////////////////////////////////////////////////////////////

class CUIFToolbarButton : public CUIFObject
{
public:
    CUIFToolbarButtonElement *m_pToolbarButtonElement;
    CUIFToolbarMenuButton *m_pToolbarMenuButton;
    DWORD m_dwToolbarButtonFlags;
    LPCWSTR m_pszUnknownText;

    CUIFToolbarButton(
        CUIFObject *pParent,
        DWORD nObjectID,
        LPCRECT prc,
        DWORD style,
        DWORD dwToolbarButtonFlags,
        LPCWSTR pszUnknownText);
    ~CUIFToolbarButton() override { }

    BOOL Init();
    HICON GetIcon();
    void SetIcon(HICON hIcon);

    STDMETHOD_(void, ClearWndObj)() override;
    STDMETHOD_(void, DetachWndObj)() override;
    STDMETHOD_(void, Enable)(BOOL bEnable) override;
    STDMETHOD_(LPCWSTR, GetToolTip)() override;
    STDMETHOD_(void, SetActiveTheme)(LPCWSTR pszClassList, INT iPartId, INT iStateId) override;
    STDMETHOD_(void, SetFont)(HFONT hFont) override;
    STDMETHOD_(void, SetRect)(LPCRECT prc) override;
    STDMETHOD_(void, SetToolTip)(LPCWSTR pszToolTip) override;

    STDMETHOD_(void, OnUnknownMouse0)() { }
    STDMETHOD_(void, OnLeftClick)() { }
    STDMETHOD_(void, OnRightClick)() { }
};

/////////////////////////////////////////////////////////////////////////////

// m_style flags for CUIFGripper
enum
{
    UIF_GRIPPER_VERTICAL = 0x1,
};

class CUIFGripper : public CUIFObject
{
protected:
    POINT m_ptGripper;

public:
    CUIFGripper(CUIFObject *pParent, LPCRECT prc, DWORD style);
    ~CUIFGripper() override;

    STDMETHOD_(void, OnMouseMove)(LONG x, LONG y) override;
    STDMETHOD_(void, OnLButtonDown)(LONG x, LONG y) override;
    STDMETHOD_(void, OnLButtonUp)(LONG x, LONG y) override;
    STDMETHOD_(BOOL, OnPaintTheme)(HDC hDC) override;
    STDMETHOD_(void, OnPaintNoTheme)(HDC hDC) override;
    STDMETHOD_(BOOL, OnSetCursor)(UINT uMsg, LONG x, LONG y) override;
    STDMETHOD_(void, SetStyle)(DWORD style) override;
};

/////////////////////////////////////////////////////////////////////////////

class CUIFWndFrame : public CUIFObject
{
protected:
    DWORD m_dwHitTest;
    POINT m_ptHit;
    RECT m_rcWnd;
    INT m_cxFrame;
    INT m_cyFrame;
    INT m_cxMin;
    INT m_cyMin;

public:
    CUIFWndFrame(CUIFObject *pParent, LPCRECT prc, DWORD style);

    void GetFrameSize(LPSIZE pSize);
    DWORD HitTest(LONG x, LONG y);

    STDMETHOD_(void, OnMouseMove)(LONG x, LONG y) override;
    STDMETHOD_(void, OnLButtonDown)(LONG x, LONG y) override;
    STDMETHOD_(void, OnLButtonUp)(LONG x, LONG y) override;
    STDMETHOD_(BOOL, OnPaintTheme)(HDC hDC) override;
    STDMETHOD_(void, OnPaintNoTheme)(HDC hDC) override;
    STDMETHOD_(BOOL, OnSetCursor)(UINT uMsg, LONG x, LONG y) override;
};

/////////////////////////////////////////////////////////////////////////////

class CUIFBalloonButton : public CUIFButton
{
protected:
    UINT m_nCommandID;
    friend class CUIFBalloonWindow;

public:
    CUIFBalloonButton(CUIFObject *pParent, DWORD nObjectID, LPCRECT prc, DWORD style);

    STDMETHOD_(void, OnPaint)(HDC hDC) override;
    void DrawTextProc(HDC hDC, LPCRECT prc, BOOL bPressed);
};

/////////////////////////////////////////////////////////////////////////////

// m_style flags for CUIFBalloonWindow
enum
{
    UIF_BALLOON_WINDOW_OK = 0x10000,
    UIF_BALLOON_WINDOW_YESNO = 0x20000,
    UIF_BALLOON_WINDOW_TYPE_MASK = 0xF0000,
};

class CUIFBalloonWindow : public CUIFWindow
{
protected:
    LPWSTR m_pszBalloonText;
    HRGN m_hRgn;
    RECT m_rcMargin;
    DWORD m_dwUnknown6;
    BOOL m_bHasBkColor;
    BOOL m_bHasTextColor;
    COLORREF m_rgbBkColor;
    COLORREF m_rgbTextColor;
    POINT m_ptTarget;
    RECT m_rcExclude;
    POINT m_ptBalloon;
    DWORD m_dwUnknown7;
    UINT m_nBalloonType;
    DWORD m_dwUnknown8[2];
    UINT m_cButtons;
    WPARAM m_nActionID;
    HWND m_hwndNotif;
    UINT m_uNotifMsg;

public:
    CUIFBalloonWindow(HINSTANCE hInst, DWORD style);
    ~CUIFBalloonWindow() override;

    STDMETHOD_(BOOL, Initialize)() override;
    STDMETHOD_(LPCTSTR, GetClassName)() override { return TEXT("MSIME_PopupMessage"); }
    STDMETHOD_(LPCTSTR, GetWndTitle)() override { return TEXT("MSIME_PopupMessage"); }
    STDMETHOD_(void, OnCreate)(HWND hWnd) override;
    STDMETHOD_(void, OnDestroy)(HWND hWnd) override;
    STDMETHOD_(void, OnKeyDown)(HWND hWnd, WPARAM wParam, LPARAM lParam) override;
    STDMETHOD_(LRESULT, OnObjectNotify)(CUIFObject *pObject, WPARAM wParam, LPARAM lParam) override;
    STDMETHOD_(void, OnPaint)(HDC hDC) override;

    void AddButton(UINT nCommandId);
    void AdjustPos();
    HRGN CreateRegion(LPCRECT prc);
    void DoneWindowRegion();
    CUIFBalloonButton *FindButton(UINT nCommandID);
    CUIFObject *FindUIObject(UINT nObjectID);

    COLORREF GetBalloonBkColor();
    COLORREF GetBalloonTextColor();
    void GetButtonSize(LPSIZE pSize);
    void GetMargin(LPRECT prcMargin);
    void SetExcludeRect(LPCRECT prcExclude);
    void SetTargetPos(POINT ptTarget);
    void SetText(LPCWSTR pszText);

    void InitWindowRegion();
    void LayoutObject();
    void PaintFrameProc(HDC hDC, LPCRECT prc);
    void PaintMessageProc(HDC hDC, LPRECT prc, LPCWSTR pszText);
    void SendNotification(WPARAM wParam);
};

/////////////////////////////////////////////////////////////////////////////

void cicInitUIFLib(void);
void cicDoneUIFLib(void);

void cicInitUIFSys(void);
void cicDoneUIFSys(void);
void cicUpdateUIFSys(void);

void cicInitUIFScheme(void);
void cicUpdateUIFScheme(void);
void cicDoneUIFScheme(void);

void cicGetWorkAreaRect(POINT pt, LPRECT prc);
void cicGetScreenRect(POINT pt, LPRECT prc);
BOOL cicIsFullScreenSize(HWND hWnd);
BOOL cicGetIconSize(HICON hIcon, LPSIZE pSize);

/////////////////////////////////////////////////////////////////////////////
