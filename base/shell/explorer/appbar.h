/*
 * PROJECT:     ReactOS Explorer
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     AppBar implementation
 * COPYRIGHT:   Copyright 2025 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#pragma once

typedef struct _APPBAR
{
    HWND hWnd;
    UINT uCallbackMessage;
    RECT rc;
    UINT uEdge;
} APPBAR, *PAPPBAR;

static inline PAPPBARDATA
AppBar_LockOutput(_In_ PAPPBAR_COMMAND pData)
{
    return (PAPPBARDATA)SHLockShared(pData->hOutput, pData->dwProcessId);
}

static inline VOID
AppBar_UnLockOutput(_Out_ PAPPBARDATA pOutput)
{
    SHUnlockShared(pOutput);
}

static inline BOOL
Edge_IsVertical(_In_ UINT uEdge)
{
    return uEdge == ABE_TOP || uEdge == ABE_BOTTOM;
}

class CAppBarManager
{
public:
    CAppBarManager();
    virtual ~CAppBarManager();

    virtual void StuckAppChange(
        _In_opt_ HWND hwndTarget,
        _In_opt_ const RECT *prcOld,
        _In_opt_ const RECT *prcNew,
        _In_ BOOL bFlag) = 0;

protected:
    HDPA m_hAppBarDPA = NULL; // DPA (Dynamic Pointer Array)

    PAPPBAR FindAppBar(_In_ HWND hwndAppBar) const;
    void EliminateAppBar(_In_ INT iItem);
    void DestroyAppBarDPA();
    void AppBarSubtractRect(_In_ PAPPBAR pAppBar, _Inout_ PRECT prc);
    BOOL AppBarOutsideOf(_In_ const APPBAR *pAppBar1, _In_ const APPBAR *pAppBar2);
    void ComputeHiddenRect(_Inout_ PRECT prc, _In_ UINT uSide);

    BOOL OnAppBarNew(_In_ const APPBAR_COMMAND *pData);
    void OnAppBarRemove(_In_ const APPBAR_COMMAND *pData);
    virtual void OnAppBarQueryPos(_Inout_ PAPPBAR_COMMAND pData) = 0;
    void OnAppBarSetPos(_Inout_ PAPPBAR_COMMAND pData);

    void OnAppBarNotifyAll(
        _In_opt_ HMONITOR hMon,
        _In_opt_ HWND hwndIgnore,
        _In_ DWORD dwNotify,
        _In_opt_ LPARAM lParam);

    void RedrawDesktop(_In_ HWND hwndDesktop, _Inout_ PRECT prc);
};
