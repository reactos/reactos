/*
 * PROJECT:     ReactOS Explorer
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     AppBar implementation
 * COPYRIGHT:   Copyright 2008 Vincent Povirk for CodeWeavers
 *              Copyright 2025 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "precomp.h"
#include "appbar.h"

CAppBarManager::CAppBarManager()
    : m_hAppBarDPA(NULL)
    , m_ahwndAutoHideBars { 0 }
{
}

CAppBarManager::~CAppBarManager()
{
    DestroyAppBarDPA();
}

PAPPBAR CAppBarManager::FindAppBar(_In_ HWND hwndAppBar) const
{
    if (!m_hAppBarDPA)
        return NULL;

    INT nItems = DPA_GetPtrCount(m_hAppBarDPA);
    while (--nItems >= 0)
    {
        PAPPBAR pAppBar = (PAPPBAR)DPA_GetPtr(m_hAppBarDPA, nItems);
        if (pAppBar && hwndAppBar == pAppBar->hWnd)
            return pAppBar;
    }

    return NULL;
}

void CAppBarManager::EliminateAppBar(_In_ INT iItem)
{
    LocalFree(DPA_GetPtr(m_hAppBarDPA, iItem));
    DPA_DeletePtr(m_hAppBarDPA, iItem);
}

void CAppBarManager::DestroyAppBarDPA()
{
    if (!m_hAppBarDPA)
        return;

    INT nItems = DPA_GetPtrCount(m_hAppBarDPA);
    while (--nItems >= 0)
    {
        ::LocalFree(DPA_GetPtr(m_hAppBarDPA, nItems));
    }

    DPA_Destroy(m_hAppBarDPA);
    m_hAppBarDPA = NULL;
}

// ABM_NEW
BOOL CAppBarManager::OnAppBarNew(_In_ const APPBAR_COMMAND *pData)
{
    HWND hWnd = (HWND)UlongToHandle(pData->abd.hWnd32);

    if (m_hAppBarDPA)
    {
        if (FindAppBar(hWnd))
        {
            ERR("Already exists: %p\n", hWnd);
            return FALSE;
        }
    }
    else
    {
        const UINT c_nGrow = 4;
        m_hAppBarDPA = DPA_Create(c_nGrow);
        if (!m_hAppBarDPA)
        {
            ERR("Out of memory\n");
            return FALSE;
        }
    }

    PAPPBAR pAppBar = (PAPPBAR)::LocalAlloc(LPTR, sizeof(*pAppBar));
    if (pAppBar)
    {
        pAppBar->hWnd = hWnd;
        pAppBar->uEdge = UINT_MAX;
        pAppBar->uCallbackMessage = pData->abd.uCallbackMessage;
        if (DPA_InsertPtr(m_hAppBarDPA, INT_MAX, pAppBar) >= 0)
            return TRUE; // Success!

        ::LocalFree(pAppBar);
    }

    ERR("Out of memory\n");
    return FALSE;
}

// ABM_REMOVE
void CAppBarManager::OnAppBarRemove(_In_ const APPBAR_COMMAND *pData)
{
    if (!m_hAppBarDPA)
        return;

    HWND hWnd = (HWND)UlongToHandle(pData->abd.hWnd32);
    INT nItems = DPA_GetPtrCount(m_hAppBarDPA);
    while (--nItems >= 0)
    {
        PAPPBAR pAppBar = (PAPPBAR)DPA_GetPtr(m_hAppBarDPA, nItems);
        if (!pAppBar)
            continue;

        if (pAppBar->hWnd == hWnd)
        {
            RECT rcOld = pAppBar->rc;
            EliminateAppBar(nItems);
            StuckAppChange(hWnd, &rcOld, NULL, FALSE);
        }
    }
}

// ABM_QUERYPOS
void CAppBarManager::OnAppBarQueryPos(_Inout_ PAPPBAR_COMMAND pData)
{
    HWND hWnd = (HWND)UlongToHandle(pData->abd.hWnd32);
    PAPPBAR pAppBar1 = FindAppBar(hWnd);
    if (!pAppBar1)
    {
        ERR("Not found: %p\n", hWnd);
        return;
    }

    PAPPBARDATAINTEROP pOutput = AppBar_LockOutput(pData);
    if (!pOutput)
    {
        ERR("!pOutput: %d\n", pData->dwProcessId);
        return;
    }
    pOutput->rc = pData->abd.rc;

    if (::IsRectEmpty(&pOutput->rc))
        ERR("IsRectEmpty\n");

    HMONITOR hMon1 = ::MonitorFromRect(&pOutput->rc, MONITOR_DEFAULTTOPRIMARY);
    ASSERT(hMon1 != NULL);

    // Subtract tray rectangle from pOutput->rc if necessary
    if (hMon1 == GetMonitor() && !IsAutoHideState())
    {
        APPBAR dummyAppBar;
        dummyAppBar.uEdge = GetPosition();
        GetDockedRect(&dummyAppBar.rc);
        AppBarSubtractRect(&dummyAppBar, &pOutput->rc);
    }

    // Subtract area from pOutput->rc
    UINT uEdge = pData->abd.uEdge;
    INT nItems = DPA_GetPtrCount(m_hAppBarDPA);
    while (--nItems >= 0)
    {
        PAPPBAR pAppBar2 = (PAPPBAR)DPA_GetPtr(m_hAppBarDPA, nItems);
        if (!pAppBar2 || pAppBar1->hWnd == pAppBar2->hWnd)
            continue;

        if ((Edge_IsVertical(uEdge) || !Edge_IsVertical(pAppBar2->uEdge)) &&
            (pAppBar1->uEdge != uEdge || !AppBarOutsideOf(pAppBar1, pAppBar2)))
        {
            if (pAppBar1->uEdge == uEdge || pAppBar2->uEdge != uEdge)
                continue;
        }

        HMONITOR hMon2 = ::MonitorFromRect(&pAppBar2->rc, MONITOR_DEFAULTTONULL);
        if (hMon1 == hMon2)
            AppBarSubtractRect(pAppBar2, &pOutput->rc);
    }

    AppBar_UnLockOutput(pOutput);
}

// ABM_SETPOS
void CAppBarManager::OnAppBarSetPos(_Inout_ PAPPBAR_COMMAND pData)
{
    HWND hWnd = (HWND)UlongToHandle(pData->abd.hWnd32);
    PAPPBAR pAppBar = FindAppBar(hWnd);
    if (!pAppBar)
        return;

    OnAppBarQueryPos(pData);

    PAPPBARDATAINTEROP pOutput = AppBar_LockOutput(pData);
    if (!pOutput)
        return;

    RECT rcOld = pAppBar->rc, rcNew = pData->abd.rc;
    BOOL bChanged = !::EqualRect(&rcOld, &rcNew);

    pAppBar->rc = rcNew;
    pAppBar->uEdge = pData->abd.uEdge;

    AppBar_UnLockOutput(pOutput);

    if (bChanged)
        StuckAppChange(hWnd, &rcOld, &rcNew, FALSE);
}

void CAppBarManager::OnAppBarNotifyAll(
    _In_opt_ HMONITOR hMon,
    _In_opt_ HWND hwndIgnore,
    _In_ DWORD dwNotify,
    _In_opt_ LPARAM lParam)
{
    TRACE("%p, %p, 0x%X, %p\n", hMon, hwndIgnore, dwNotify, lParam);

    if (!m_hAppBarDPA)
        return;

    INT nItems = DPA_GetPtrCount(m_hAppBarDPA);
    while (--nItems >= 0)
    {
        PAPPBAR pAppBar = (PAPPBAR)DPA_GetPtr(m_hAppBarDPA, nItems);
        if (!pAppBar || pAppBar->hWnd == hwndIgnore)
            continue;

        HWND hwndAppBar = pAppBar->hWnd;
        if (!::IsWindow(hwndAppBar))
        {
            EliminateAppBar(nItems);
            continue;
        }

        if (!hMon || hMon == ::MonitorFromWindow(hwndAppBar, MONITOR_DEFAULTTONULL))
            ::PostMessageW(hwndAppBar, pAppBar->uCallbackMessage, dwNotify, lParam);
    }
}

/// @param pAppBar The target AppBar to subtract.
/// @param prc The rectangle to be subtracted.
void CAppBarManager::AppBarSubtractRect(_In_ PAPPBAR pAppBar, _Inout_ PRECT prc)
{
    switch (pAppBar->uEdge)
    {
        case ABE_LEFT:   prc->left   = max(prc->left, pAppBar->rc.right); break;
        case ABE_TOP:    prc->top    = max(prc->top, pAppBar->rc.bottom); break;
        case ABE_RIGHT:  prc->right  = min(prc->right, pAppBar->rc.left); break;
        case ABE_BOTTOM: prc->bottom = min(prc->bottom, pAppBar->rc.top); break;
        default:
            ASSERT(FALSE);
            break;
    }
}

// Is pAppBar1 outside of pAppBar2?
BOOL CAppBarManager::AppBarOutsideOf(
    _In_ const APPBAR *pAppBar1,
    _In_ const APPBAR *pAppBar2)
{
    if (pAppBar1->uEdge != pAppBar2->uEdge)
        return FALSE;

    switch (pAppBar2->uEdge)
    {
        case ABE_LEFT:   return pAppBar1->rc.left >= pAppBar2->rc.left;
        case ABE_TOP:    return pAppBar1->rc.top >= pAppBar2->rc.top;
        case ABE_RIGHT:  return pAppBar1->rc.right <= pAppBar2->rc.right;
        case ABE_BOTTOM: return pAppBar1->rc.bottom <= pAppBar2->rc.bottom;
        default:
            ASSERT(FALSE);
            return FALSE;
    }
}

/// Get rectangle of the tray window.
/// @param prcDocked The pointer to the rectangle to be received.
void CAppBarManager::GetDockedRect(_Out_ PRECT prcDocked)
{
    *prcDocked = *GetTrayRect();
    if (IsAutoHideState() && IsHidingState())
        ComputeHiddenRect(prcDocked, GetPosition());
}

/// Compute the position and size of the hidden TaskBar.
/// @param prc The rectangle before hiding TaskBar.
/// @param uSide The side of TaskBar (ABE_...).
void CAppBarManager::ComputeHiddenRect(_Inout_ PRECT prc, _In_ UINT uSide)
{
    MONITORINFO mi = { sizeof(mi) };
    HMONITOR hMonitor = ::MonitorFromRect(prc, MONITOR_DEFAULTTONULL);
    if (!::GetMonitorInfoW(hMonitor, &mi))
        return;
    RECT rcMon = mi.rcMonitor;

    INT cxy = Edge_IsVertical(uSide) ? (prc->bottom - prc->top) : (prc->right - prc->left);
    switch (uSide)
    {
        case ABE_LEFT:
            prc->right = rcMon.left + GetSystemMetrics(SM_CXFRAME) / 2;
            prc->left = prc->right - cxy;
            break;
        case ABE_TOP:
            prc->bottom = rcMon.top + GetSystemMetrics(SM_CYFRAME) / 2;
            prc->top = prc->bottom - cxy;
            break;
        case ABE_RIGHT:
            prc->left = rcMon.right - GetSystemMetrics(SM_CXFRAME) / 2;
            prc->right = prc->left + cxy;
            break;
        case ABE_BOTTOM:
            prc->top = rcMon.bottom - GetSystemMetrics(SM_CYFRAME) / 2;
            prc->bottom = prc->top + cxy;
            break;
        default:
            ASSERT(FALSE);
            break;
    }
}

/// This function is called when AppBar and/or TaskBar is being moved, removed, and/or updated.
/// @param hwndTarget The target window. Optional.
/// @param prcOld The old position and size. Optional.
/// @param prcNew The new position and size. Optional.
/// @param bTray TRUE if the tray is being moved.
void
CAppBarManager::StuckAppChange(
    _In_opt_ HWND hwndTarget,
    _In_opt_ const RECT *prcOld,
    _In_opt_ const RECT *prcNew,
    _In_ BOOL bTray)
{
    RECT rcWorkArea1, rcWorkArea2;
    HMONITOR hMon1 = NULL;
    UINT flags = 0;
    enum { SET_WORKAREA_1 = 1, SET_WORKAREA_2 = 2, NEED_SIZING = 4 }; // for flags

    if (prcOld)
    {
        hMon1 = (bTray ? GetPreviousMonitor() : ::MonitorFromRect(prcOld, MONITOR_DEFAULTTONEAREST));
        if (hMon1)
        {
            WORKAREA_TYPE type1 = RecomputeWorkArea(GetTrayRect(), hMon1, &rcWorkArea1);
            if (type1 == WORKAREA_IS_NOT_MONITOR)
                flags = SET_WORKAREA_1;
            if (type1 == WORKAREA_SAME_AS_MONITOR)
                flags = NEED_SIZING;
        }
    }

    if (prcNew)
    {
        HMONITOR hMon2 = ::MonitorFromRect(prcNew, MONITOR_DEFAULTTONULL);
        if (hMon2 && hMon2 != hMon1)
        {
            WORKAREA_TYPE type2 = RecomputeWorkArea(GetTrayRect(), hMon2, &rcWorkArea2);
            if (type2 == WORKAREA_IS_NOT_MONITOR)
                flags |= SET_WORKAREA_2;
            else if (type2 == WORKAREA_SAME_AS_MONITOR && !flags)
                flags = NEED_SIZING;
        }
    }

    if (flags & SET_WORKAREA_1)
    {
        UINT fWinIni = ((flags == SET_WORKAREA_1 && GetDesktopWnd()) ? SPIF_SENDCHANGE : 0);
        ::SystemParametersInfoW(SPI_SETWORKAREA, TRUE, &rcWorkArea1, fWinIni);
        RedrawDesktop(GetDesktopWnd(), &rcWorkArea1);
    }

    if (flags & SET_WORKAREA_2)
    {
        UINT fWinIni = (GetDesktopWnd() ? SPIF_SENDCHANGE : 0);
        ::SystemParametersInfoW(SPI_SETWORKAREA, TRUE, &rcWorkArea2, fWinIni);
        RedrawDesktop(GetDesktopWnd(), &rcWorkArea2);
    }

    if (bTray || flags == NEED_SIZING)
        ::SendMessageW(GetDesktopWnd(), WM_SIZE, 0, 0);

    // Post ABN_POSCHANGED messages to AppBar windows
    OnAppBarNotifyAll(NULL, hwndTarget, ABN_POSCHANGED, TRUE);
}

void CAppBarManager::RedrawDesktop(_In_ HWND hwndDesktop, _Inout_ PRECT prc)
{
    if (!hwndDesktop)
        return;
    ::MapWindowPoints(NULL, hwndDesktop, (POINT*)prc, sizeof(*prc) / sizeof(POINT));
    ::RedrawWindow(hwndDesktop, prc, 0, RDW_ALLCHILDREN | RDW_ERASE | RDW_INVALIDATE);
}

/// Re-compute the work area.
/// @param prcTray The position and size of the tray window
/// @param hMonitor The monitor of the work area to re-compute.
/// @param prcWorkArea The work area to be re-computed.
WORKAREA_TYPE
CAppBarManager::RecomputeWorkArea(
    _In_ const RECT *prcTray,
    _In_ HMONITOR hMonitor,
    _Out_ PRECT prcWorkArea)
{
    MONITORINFO mi = { sizeof(mi) };
    if (!::GetMonitorInfoW(hMonitor, &mi))
        return WORKAREA_NO_TRAY_AREA;

    if (IsAutoHideState())
        *prcWorkArea = mi.rcMonitor;
    else
        ::SubtractRect(prcWorkArea, &mi.rcMonitor, prcTray);

    if (m_hAppBarDPA)
    {
        INT nItems = DPA_GetPtrCount(m_hAppBarDPA);
        while (--nItems >= 0)
        {
            PAPPBAR pAppBar = (PAPPBAR)DPA_GetPtr(m_hAppBarDPA, nItems);
            if (pAppBar && hMonitor == ::MonitorFromRect(&pAppBar->rc, MONITOR_DEFAULTTONULL))
                AppBarSubtractRect(pAppBar, prcWorkArea);
        }
    }

    if (!::EqualRect(prcWorkArea, &mi.rcWork))
        return WORKAREA_IS_NOT_MONITOR;

    if (IsAutoHideState() || ::IsRectEmpty(prcTray))
        return WORKAREA_NO_TRAY_AREA;

    return WORKAREA_SAME_AS_MONITOR;
}

BOOL CALLBACK
CAppBarManager::MonitorEnumProc(
    _In_ HMONITOR hMonitor,
    _In_ HDC hDC,
    _In_ LPRECT prc,
    _Inout_ LPARAM lParam)
{
    CAppBarManager *pThis = (CAppBarManager *)lParam;
    UNREFERENCED_PARAMETER(hDC);

    RECT rcWorkArea;
    if (pThis->RecomputeWorkArea(prc, hMonitor, &rcWorkArea) != WORKAREA_IS_NOT_MONITOR)
        return TRUE;

    HWND hwndDesktop = pThis->GetDesktopWnd();
    ::SystemParametersInfoW(SPI_SETWORKAREA, 0, &rcWorkArea, hwndDesktop ? SPIF_SENDCHANGE : 0);
    pThis->RedrawDesktop(hwndDesktop, &rcWorkArea);
    return TRUE;
}

void CAppBarManager::RecomputeAllWorkareas()
{
    ::EnumDisplayMonitors(NULL, NULL, CAppBarManager::MonitorEnumProc, (LPARAM)this);
}

BOOL CAppBarManager::SetAutoHideBar(_In_ HWND hwndTarget, _In_ BOOL bSetOrReset, _In_ UINT uSide)
{
    ATLASSERT(uSide < _countof(m_ahwndAutoHideBars));
    HWND *phwndAutoHide = &m_ahwndAutoHideBars[uSide];
    if (!IsWindow(*phwndAutoHide))
        *phwndAutoHide = NULL;

    if (bSetOrReset) // Set?
    {
        if (!*phwndAutoHide)
            *phwndAutoHide = hwndTarget;
        return *phwndAutoHide == hwndTarget;
    }
    else // Reset
    {
        if (*phwndAutoHide == hwndTarget)
            *phwndAutoHide = NULL;
        return TRUE;
    }
}

void CAppBarManager::OnAppBarActivationChange2(_In_ HWND hwndNewAutoHide, _In_ UINT uSide)
{
    HWND hwndAutoHideBar = OnAppBarGetAutoHideBar(uSide);
    if (hwndAutoHideBar && hwndAutoHideBar != hwndNewAutoHide)
        ::PostMessageW(GetTrayWnd(), TWM_SETZORDER, (WPARAM)hwndAutoHideBar, uSide);
}

PAPPBAR_COMMAND
CAppBarManager::GetAppBarMessage(_Inout_ PCOPYDATASTRUCT pCopyData)
{
    PAPPBAR_COMMAND pData = (PAPPBAR_COMMAND)pCopyData->lpData;

    if (pCopyData->cbData != sizeof(*pData) || pData->abd.cbSize != sizeof(pData->abd))
    {
        ERR("Invalid AppBar message\n");
        return NULL;
    }

    return pData;
}

// ABM_GETSTATE
UINT CAppBarManager::OnAppBarGetState()
{
    return (IsAutoHideState() ? ABS_AUTOHIDE : 0) | (IsAlwaysOnTop() ? ABS_ALWAYSONTOP : 0);
}

// ABM_GETTASKBARPOS
BOOL CAppBarManager::OnAppBarGetTaskbarPos(_Inout_ PAPPBAR_COMMAND pData)
{
    PAPPBARDATAINTEROP pOutput = AppBar_LockOutput(pData);
    if (!pOutput)
    {
        ERR("!pOutput: %d\n", pData->dwProcessId);
        return FALSE;
    }

    pOutput->rc = *GetTrayRect();
    pOutput->uEdge = GetPosition();

    AppBar_UnLockOutput(pOutput);
    return TRUE;
}

// ABM_ACTIVATE, ABM_WINDOWPOSCHANGED
void CAppBarManager::OnAppBarActivationChange(_In_ const APPBAR_COMMAND *pData)
{
    HWND hWnd = (HWND)UlongToHandle(pData->abd.hWnd32);
    PAPPBAR pAppBar = FindAppBar(hWnd);
    if (!pAppBar)
    {
        ERR("Not found: %p\n", hWnd);
        return;
    }

    HWND hwndAppBar = pAppBar->hWnd;
    for (UINT uSide = ABE_LEFT; uSide <= ABE_BOTTOM; ++uSide)
    {
        if (m_ahwndAutoHideBars[uSide] == hwndAppBar && uSide != pAppBar->uEdge)
            return;
    }

    OnAppBarActivationChange2(hwndAppBar, pAppBar->uEdge);
}

// ABM_GETAUTOHIDEBAR
HWND CAppBarManager::OnAppBarGetAutoHideBar(_In_ UINT uSide)
{
    if (uSide >= _countof(m_ahwndAutoHideBars))
        return NULL;

    if (!::IsWindow(m_ahwndAutoHideBars[uSide]))
        m_ahwndAutoHideBars[uSide] = NULL;
    return m_ahwndAutoHideBars[uSide];
}

// ABM_SETAUTOHIDEBAR
BOOL CAppBarManager::OnAppBarSetAutoHideBar(_In_ const APPBAR_COMMAND *pData)
{
    if (pData->abd.uEdge >= _countof(m_ahwndAutoHideBars))
        return FALSE;
    HWND hwndTarget = (HWND)UlongToHandle(pData->abd.hWnd32);
    return SetAutoHideBar(hwndTarget, (BOOL)pData->abd.lParam64, pData->abd.uEdge);
}

// ABM_SETSTATE
void CAppBarManager::OnAppBarSetState(_In_ UINT uState)
{
    if ((uState & ~(ABS_AUTOHIDE | ABS_ALWAYSONTOP)))
        return;

    SetAutoHideState(!!(uState & ABS_AUTOHIDE));
    UpdateAlwaysOnTop(!!(uState & ABS_ALWAYSONTOP));
}

// WM_COPYDATA TABDMC_APPBAR
LRESULT CAppBarManager::OnAppBarMessage(_Inout_ PCOPYDATASTRUCT pCopyData)
{
    PAPPBAR_COMMAND pData = GetAppBarMessage(pCopyData);
    if (!pData)
        return 0;

    switch (pData->dwMessage)
    {
        case ABM_NEW:
            return OnAppBarNew(pData);
        case ABM_REMOVE:
            OnAppBarRemove(pData);
            break;
        case ABM_QUERYPOS:
            OnAppBarQueryPos(pData);
            break;
        case ABM_SETPOS:
            OnAppBarSetPos(pData);
            break;
        case ABM_GETSTATE:
            return OnAppBarGetState();
        case ABM_GETTASKBARPOS:
            return OnAppBarGetTaskbarPos(pData);
        case ABM_ACTIVATE:
        case ABM_WINDOWPOSCHANGED:
            OnAppBarActivationChange(pData);
            break;
        case ABM_GETAUTOHIDEBAR:
            return (LRESULT)OnAppBarGetAutoHideBar(pData->abd.uEdge);
        case ABM_SETAUTOHIDEBAR:
            return OnAppBarSetAutoHideBar(pData);
        case ABM_SETSTATE:
            OnAppBarSetState((UINT)pData->abd.lParam64);
            break;
        default:
        {
            FIXME("0x%X\n", pData->dwMessage);
            return FALSE;
        }
    }
    return TRUE;
}
