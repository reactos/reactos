/*
 * PROJECT:     ReactOS Explorer
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     AppBar implementation
 * COPYRIGHT:   Copyright 2025 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#pragma once

typedef struct tagAPPBAR
{
    HWND hWnd;
    UINT uCallbackMessage;
    UINT uEdge;
    RECT rc;
} APPBAR, *PAPPBAR;

static inline PAPPBARDATAINTEROP
AppBar_LockOutput(_In_ PAPPBAR_COMMAND pData)
{
    return (PAPPBARDATAINTEROP)SHLockShared((HANDLE)pData->hOutput, pData->dwProcessId);
}

static inline VOID
AppBar_UnLockOutput(_Out_ PAPPBARDATAINTEROP pOutput)
{
    SHUnlockShared(pOutput);
}

static inline BOOL
Edge_IsVertical(_In_ UINT uEdge)
{
    return uEdge == ABE_TOP || uEdge == ABE_BOTTOM;
}

// Return value of CAppBarManager::RecomputeWorkArea
enum WORKAREA_TYPE
{
    WORKAREA_NO_TRAY_AREA = 0,
    WORKAREA_IS_NOT_MONITOR = 1,
    WORKAREA_SAME_AS_MONITOR = 2,
};

class CAppBarManager
{
public:
    CAppBarManager();
    virtual ~CAppBarManager();

    LRESULT OnAppBarMessage(_Inout_ PCOPYDATASTRUCT pCopyData);

protected:
    HDPA m_hAppBarDPA; // DPA (Dynamic Pointer Array)

    PAPPBAR FindAppBar(_In_ HWND hwndAppBar) const;
    void EliminateAppBar(_In_ INT iItem);
    void DestroyAppBarDPA();
    void AppBarSubtractRect(_In_ PAPPBAR pAppBar, _Inout_ PRECT prc);
    BOOL AppBarOutsideOf(_In_ const APPBAR *pAppBar1, _In_ const APPBAR *pAppBar2);
    void ComputeHiddenRect(_Inout_ PRECT prc, _In_ UINT uSide);
    PAPPBAR_COMMAND GetAppBarMessage(_Inout_ PCOPYDATASTRUCT pCopyData);
    void GetDockedRect(_Out_ PRECT prcDocked);

    BOOL OnAppBarNew(_In_ const APPBAR_COMMAND *pData);
    void OnAppBarRemove(_In_ const APPBAR_COMMAND *pData);
    void OnAppBarQueryPos(_Inout_ PAPPBAR_COMMAND pData);
    void OnAppBarSetPos(_Inout_ PAPPBAR_COMMAND pData);

    void OnAppBarNotifyAll(
        _In_opt_ HMONITOR hMon,
        _In_opt_ HWND hwndIgnore,
        _In_ DWORD dwNotify,
        _In_opt_ LPARAM lParam);

    WORKAREA_TYPE
    RecomputeWorkArea(
        _In_ const RECT *prcTray,
        _In_ HMONITOR hMonitor,
        _Out_ PRECT prcWorkArea);
    void RecomputeAllWorkareas();

    void StuckAppChange(
        _In_opt_ HWND hwndTarget,
        _In_opt_ const RECT *prcOld,
        _In_opt_ const RECT *prcNew,
        _In_ BOOL bTray);

    void RedrawDesktop(_In_ HWND hwndDesktop, _Inout_ PRECT prc);

    virtual BOOL IsAutoHideState() const = 0;
    virtual BOOL IsHidingState() const = 0;
    virtual HMONITOR GetMonitor() const = 0;
    virtual HMONITOR GetPreviousMonitor() const = 0;
    virtual INT GetPosition() const = 0;
    virtual const RECT* GetTrayRect() = 0;
    virtual HWND GetDesktopWnd() const = 0;

    static BOOL CALLBACK
    MonitorEnumProc(
        _In_ HMONITOR hMonitor,
        _In_ HDC hDC,
        _In_ LPRECT prc,
        _Inout_ LPARAM lParam);
};
