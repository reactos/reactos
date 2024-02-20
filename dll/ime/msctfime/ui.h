/*
 * PROJECT:     ReactOS msctfime.ime
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     User Interface of msctfime.ime
 * COPYRIGHT:   Copyright 2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

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
