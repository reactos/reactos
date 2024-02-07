/*
 * PROJECT:     ReactOS Explorer
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Show Desktop tray button header file
 * COPYRIGHT:   Copyright 2006-2007 Thomas Weidenmueller <w3seek@reactos.org>
 *              Copyright 2018-2022 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 *              Copyright 2023 Ethan Rodensky <splitwirez@gmail.com>
 */

#pragma once

#include "precomp.h"
#include <commoncontrols.h>
#include <uxtheme.h>

// This window class name is CONFIRMED on Win10 by WinHier.
static const WCHAR szTrayShowDesktopButton[] = L"TrayShowDesktopButtonWClass";
#define TSDB_CLICK (WM_USER + 100)

// The 'Show Desktop' button at edge of taskbar
class CTrayShowDesktopButton :
    public CWindowImpl<CTrayShowDesktopButton, CWindow, CControlWinTraits>
{
    LONG m_nClickedTime;
    HTHEME m_hTheme;
    HTHEME m_hFallbackTheme;
    MARGINS m_ContentMargins;
    SIZE m_inset;
    HICON m_icon;
    SIZE m_szIcon;
    BOOL m_highContrastMode;
    BOOL m_drawWithDedicatedBackground;
    BOOL m_bHovering;

public:
    BOOL m_bPressed;
    BOOL IsHorizontal;

    DECLARE_WND_CLASS_EX(szTrayShowDesktopButton, CS_HREDRAW | CS_VREDRAW, COLOR_3DFACE)

    BEGIN_MSG_MAP(CTrayShowDesktopButton)
        MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
        MESSAGE_HANDLER(WM_NCLBUTTONDOWN, OnLButtonDown)
        MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUp)
        MESSAGE_HANDLER(WM_NCLBUTTONUP, OnLButtonUp)
        MESSAGE_HANDLER(WM_SETTINGCHANGE, OnSettingChanged)
        MESSAGE_HANDLER(WM_THEMECHANGED, OnThemeChanged)
        MESSAGE_HANDLER(WM_WINDOWPOSCHANGED, OnWindowPosChanged)
        MESSAGE_HANDLER(WM_PAINT, OnPaint)
        MESSAGE_HANDLER(WM_TIMER, OnTimer)
        MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(TSDB_CLICK, OnClick)
    END_MSG_MAP()

    CTrayShowDesktopButton();
    // This function is called from OnPaint and parent.
    BOOL GetTaskbar(OUT HWND *taskbarWnd);
    VOID OnDraw(HDC hdc, LPRECT prc);
    INT WidthOrHeight() const;
    HRESULT DoCreate(HWND hwndParent);
    LRESULT OnClick(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    VOID Click();
    LRESULT OnLButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnLButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSettingChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnThemeChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnWindowPosChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    VOID EnsureWindowTheme(BOOL setTheme);
    LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    BOOL PtInButton(LPPOINT pt);
    VOID StartHovering();
    LRESULT OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};
