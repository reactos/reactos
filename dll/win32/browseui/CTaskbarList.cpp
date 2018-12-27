/*
 * PROJECT:     browseui
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     ITaskbarList implementation
 * COPYRIGHT:   Copyright 2018 Mark Jansen (mark.jansen@reactos.org)
 */

#include "precomp.h"


/***********************************************************************
 *   ITaskbarList2 implementation
 */

#define TWM_GETTASKSWITCH (WM_USER + 236)

CTaskbarList::CTaskbarList()
    : m_hTaskWnd(NULL)
{
    m_ShellHookMsg = RegisterWindowMessageW(L"SHELLHOOK");
}

CTaskbarList::~CTaskbarList()
{
}

HWND CTaskbarList::TaskWnd()
{
    HWND hTrayWnd;
    if (m_hTaskWnd && ::IsWindow(m_hTaskWnd))
        return m_hTaskWnd;

    hTrayWnd = FindWindowW(L"Shell_TrayWnd", NULL);
    if (hTrayWnd)
    {
        m_hTaskWnd = (HWND)SendMessageW(hTrayWnd, TWM_GETTASKSWITCH, 0L, 0L);
    }
    return m_hTaskWnd;
}

void CTaskbarList::SendTaskWndShellHook(WPARAM wParam, HWND hWnd)
{
    HWND hTaskWnd = TaskWnd();
    if (hTaskWnd && m_ShellHookMsg)
        ::SendMessageW(hTaskWnd, m_ShellHookMsg, wParam, (LPARAM)hWnd);
}


HRESULT WINAPI CTaskbarList::MarkFullscreenWindow(HWND hwnd, BOOL fFullscreen)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}


/***********************************************************************
 *   ITaskbarList implementation
 */

HRESULT WINAPI CTaskbarList::HrInit()
{
    if (m_ShellHookMsg == NULL)
        return E_OUTOFMEMORY;

    if (!TaskWnd())
        return E_HANDLE;

    return S_OK;
}

HRESULT WINAPI CTaskbarList::AddTab(HWND hwnd)
{
    SendTaskWndShellHook(HSHELL_WINDOWCREATED, hwnd);
    return S_OK;
}

HRESULT WINAPI CTaskbarList::DeleteTab(HWND hwnd)
{
    SendTaskWndShellHook(HSHELL_WINDOWDESTROYED, hwnd);
    return S_OK;
}

HRESULT WINAPI CTaskbarList::ActivateTab(HWND hwnd)
{
    SendTaskWndShellHook(HSHELL_WINDOWACTIVATED, hwnd);
    return S_OK;
}

HRESULT WINAPI CTaskbarList::SetActiveAlt(HWND hwnd)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

