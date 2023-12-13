/*
 * PROJECT:     ReactOS CTF Monitor
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Cicero TIP Bar loader window
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

class CLoaderWnd
{
public:
    HWND m_hWnd;
    static BOOL s_bUninitedSystem;
    static BOOL s_bWndClassRegistered;

    CLoaderWnd() : m_hWnd(NULL) { }
    ~CLoaderWnd() { }

    BOOL Init();
    HWND CreateWnd();

protected:
    static LRESULT CALLBACK
    WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};
