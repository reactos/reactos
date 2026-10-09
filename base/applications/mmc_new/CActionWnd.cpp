/*
 * PROJECT:     ReactOS Management Console
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Action window class
 * COPYRIGHT:   Copyright 2026 Eric Kohl (eric.kohl@reactos.org)
 */

#include "precomp.h"

CActionWnd::CActionWnd()
{
}

CActionWnd::~CActionWnd()
{
}

LRESULT
CActionWnd::OnCreate(UINT nMessage, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    return 0;
}

LRESULT
CActionWnd::OnDestroy(UINT nMessage, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    return 0;
}

LRESULT
CActionWnd::OnPaint(UINT nMessage, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(&ps);

    FillRect(hdc, &ps.rcPaint, (HBRUSH) (COLOR_WINDOW+1));

    EndPaint(&ps);

    return 0;
}
