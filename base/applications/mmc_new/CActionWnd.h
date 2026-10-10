/*
 * PROJECT:     ReactOS Management Console
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Action window class
 * COPYRIGHT:   Copyright 2026 Eric Kohl (eric.kohl@reactos.org)
 */

#pragma once

class CActionWnd :
    public CWindowImpl<CActionWnd>
{
private:

public:

public:

    BEGIN_MSG_MAP(CActionWnd)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_PAINT, OnPaint)
    END_MSG_MAP()

    static CWndClassInfo& GetWndClassInfo()
    {
        static CWndClassInfo wc =
        {
            {
                /* cbSize= */sizeof(WNDCLASSEX),
                /* style= */0,
                /* lpfnWndProc= */StartWindowProc,
                /* cbClsExtra= */0,
                /* cbWndExtra= */0,
                /* hInstance= */NULL,
                /* hIcon= */ NULL, //LoadIcon(_AtlBaseModule.GetModuleInstance(), MAKEINTRESOURCE(IDI_MAINAPP)),
                /* hCursor= */NULL,
                /* hbrBackground= */(HBRUSH)(COLOR_BTNFACE + 1),
                /* lpszMenuName= */NULL,
                /* lpszClassName= */L"MMCActionWindow",
                /* hIconSm= */NULL //LoadIcon(_AtlBaseModule.GetModuleInstance(), MAKEINTRESOURCE(IDI_MAINAPP))
            },
            NULL, NULL, IDC_ARROW, TRUE, 0, L""
        };
        return wc;
    }

    static LPCTSTR GetWndClassName()
    {
        return GetWndClassInfo().m_wc.lpszClassName;
    }

private:

public:
    CActionWnd();
    ~CActionWnd();

    LRESULT OnCreate(UINT nMessage, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDestroy(UINT nMessage, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnPaint(UINT nMessage, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

};

