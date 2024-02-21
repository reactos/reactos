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

class CCompFrameWindow;

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
    HWND m_hwndDefCompFrame;
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

struct UIComposition
{
    void OnImeStartComposition(CicIMCLock& imcLock, HWND hUIWnd);
    void OnImeCompositionUpdate(CicIMCLock& imcLock);
    void OnImeEndComposition();
    void OnImeSetContext(CicIMCLock& imcLock, HWND hUIWnd, WPARAM wParam, LPARAM lParam);
    void OnPaintTheme(WPARAM wParam);
    void OnDestroy();

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
