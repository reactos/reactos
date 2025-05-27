/*
 * PROJECT:     ReactOS msctfime.ime
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     User Interface of msctfime.ime
 * COPYRIGHT:   Copyright 2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

class CUIFGripper;
    class CDefCompFrameGripper;
class CUIFToolbarButton;
    class CCompFinalizeButton;
class CUIFWindow;
    class CCompFrameWindow;
        class CCompButtonFrameWindow;
        class CDefCompFrameWindow;

/***********************************************************************/

extern UINT WM_MSIME_SERVICE;
extern UINT WM_MSIME_UIREADY;
extern UINT WM_MSIME_RECONVERTREQUEST;
extern UINT WM_MSIME_RECONVERT;
extern UINT WM_MSIME_DOCUMENTFEED;
extern UINT WM_MSIME_QUERYPOSITION;
extern UINT WM_MSIME_MODEBIAS;
extern UINT WM_MSIME_SHOWIMEPAD;
extern UINT WM_MSIME_MOUSE;
extern UINT WM_MSIME_KEYMAP;

BOOL IsMsImeMessage(_In_ UINT uMsg);
BOOL RegisterMSIMEMessage(VOID);

/***********************************************************************/

class CDefCompFrameGripper : public CUIFGripper
{
public:
    CDefCompFrameWindow *m_pDefCompFrameWindow;

    CDefCompFrameGripper(CDefCompFrameWindow *pDefCompFrameWindow, LPCRECT prc, DWORD style);
};

/***********************************************************************/

class CCompFinalizeButton : public CUIFToolbarButton
{
public:
    CCompFrameWindow *m_pCompFrameWindow;

    CCompFinalizeButton(
        CCompFrameWindow *pParent,
        DWORD nObjectID,
        LPCRECT prc,
        DWORD style,
        DWORD dwButtonFlags,
        LPCWSTR pszText);
    ~CCompFinalizeButton() override;

    STDMETHOD_(void, OnLeftClick)() override;
};

/***********************************************************************/

class CCompFrameWindow : public CUIFWindow
{
public:
    HIMC m_hIMC;

    CCompFrameWindow(HIMC hIMC, DWORD style);
};

/***********************************************************************/

class CCompButtonFrameWindow : public CCompFrameWindow
{
public:
    MARGINS m_Margins;
    CCompFinalizeButton *m_pFinalizeButton;

    CCompButtonFrameWindow(HIMC hIMC, DWORD style);

    void Init();
    void MoveShow(LONG x, LONG y, BOOL bShow);

    STDMETHOD_(void, OnCreate)(HWND hWnd) override;
};

/***********************************************************************/

class CDefCompFrameWindow : public CCompFrameWindow
{
public:
    HWND m_hwndCompStr;
    CDefCompFrameGripper *m_pGripper;
    CCompFinalizeButton *m_pFinalizeButton;
    MARGINS m_Margins;

public:
    CDefCompFrameWindow(HIMC hIMC, DWORD style);
    ~CDefCompFrameWindow() override;

    void Init();
    INT GetGripperWidth();
    void MyScreenToClient(LPPOINT ppt, LPRECT prc);
    void SetCompStrRect(INT nWidth, INT nHeight, BOOL bShow);

    void LoadPosition();
    void SavePosition();

    STDMETHOD_(void, OnCreate)(HWND hWnd) override;
    STDMETHOD_(BOOL, OnSetCursor)(UINT uMsg, LONG x, LONG y) override;
    STDMETHOD_(LRESULT, OnWindowPosChanged)(HWND hWnd, UINT uMsg, WPARAM wParam,
                                            LPARAM lParam) override;
    STDMETHOD_(void, HandleMouseMsg)(UINT uMsg, LONG x, LONG y) override;
};

/***********************************************************************/

struct CPolyText
{
    CicArray<POLYTEXTW> m_PolyTextArray;
    CicArray<DWORD> m_ValueArray;

    HRESULT ShiftPolyText(INT xDelta, INT yDelta);
    POLYTEXTW *GetPolyAt(INT iItem);
    HRESULT RemoveLastLine(BOOL bHorizontal);
    void RemoveAll();
};

/***********************************************************************/

struct COMPWND
{
    HWND m_hWnd;
    CPolyText m_PolyText;
    CicCaret m_Caret;
    DWORD m_dwUnknown57[3];

    void _ClientToScreen(LPRECT prc);
};

/***********************************************************************/

class UIComposition
{
public:
    HWND m_hwndParent;
    BOOL m_bHasCompWnd;
    COMPWND m_CompStrs[4];
    HFONT m_hFont1;
    DWORD m_dwUnknown54;
    HFONT m_hFont2;
    DWORD m_dwUnknown55;
    SIZE m_CaretSize;
    DWORD m_dwUnknown56[2];
    LPWSTR m_strCompStr;
    INT m_cchCompStr;
    BOOL m_bInComposition;
    BOOL m_bHasCompStr;
    CDefCompFrameWindow *m_pDefCompFrameWindow;
    CCompButtonFrameWindow *m_pCompButtonFrameWindow;

public:
    UIComposition(HWND hwndParent);
    virtual ~UIComposition();

    HRESULT CreateDefFrameWnd(HWND hwndParent, HIMC hIMC);
    HRESULT CreateCompButtonWnd(HWND hwndParent, HIMC hIMC);
    HRESULT CreateCompositionWindow(CicIMCLock& imcLock, HWND hwndParent);
    HRESULT DestroyCompositionWindow();

    HRESULT UpdateShowCompWndFlag(CicIMCLock& imcLock, DWORD *pdwCompStrLen);
    HRESULT UpdateFont(CicIMCLock& imcLock);
    HRESULT UpdateCompositionRect(CicIMCLock& imcLock);
    LPWSTR GetCompStrBuffer(INT cchStr);
    INT GetLevelFromIMC(CicIMCLock& imcLock);

    void OnImeStartComposition(CicIMCLock& imcLock, HWND hUIWnd);
    HRESULT OnImeCompositionUpdate(CicIMCLock& imcLock);
    HRESULT OnImeEndComposition();
    HRESULT OnImeNotifySetCompositionWindow(CicIMCLock& imcLock);
    HRESULT OnImeSetContextAfter(CicIMCLock& imcLock);
    void OnImeSetContext(CicIMCLock& imcLock, HWND hUIWnd, WPARAM wParam, LPARAM lParam);
    void OnPaintTheme(WPARAM wParam);
    void OnTimer(HWND hWnd);
    HRESULT OnDestroy();

    static BOOL SendMessageToUI(CicIMCLock& imcLock, WPARAM wParam, LPARAM lParam);
    static BOOL InquireImeUIWndState(CicIMCLock& imcLock);
    static BOOL GetImeUIWndTextExtent(CicIMCLock& imcLock, LPARAM lParam);

    static LRESULT CALLBACK CompWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

/***********************************************************************/

struct UI
{
    HWND m_hWnd;
    UIComposition *m_pComp;

    UI(HWND hWnd);
    virtual ~UI();

    HRESULT _Create();
    void _Destroy();

    static void OnCreate(HWND hWnd);
    static void OnDestroy(HWND hWnd);

    void OnImeSetContext(CicIMCLock& imcLock, WPARAM wParam, LPARAM lParam);
};

/***********************************************************************/

EXTERN_C LRESULT CALLBACK
UIWndProc(
    _In_ HWND hWnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam);

BOOL RegisterImeClass(VOID);
VOID UnregisterImeClass(VOID);
