/*
 * PROJECT:     ReactOS Explorer
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     AppBar implementation
 * COPYRIGHT:   Copyright 2025 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "precomp.h"
#include "appbar.h"

CAppBarManager::CAppBarManager()
{
}

CAppBarManager::~CAppBarManager()
{
}

PAPPBAR_COMMAND
CAppBarManager::GetAppBarMessage(_In_ const COPYDATASTRUCT *pCopyData)
{
    PAPPBAR_COMMAND pData = (PAPPBAR_COMMAND)pCopyData->lpData;

    // For security check
    if (pCopyData->cbData != sizeof(*pData) || pData->dwMagic != 0xBEEFCAFE)
    {
        WARN("Invalid AppBar message\n");
        return NULL;
    }

    return pData;
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
    if (m_hAppBarDPA)
    {
        if (FindAppBar(pData->data.hWnd))
        {
            WARN("Already exists: %p\n", pData->data.hWnd);
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
        pAppBar->hWnd = pData->data.hWnd;
        pAppBar->uEdge = UINT_MAX;
        pAppBar->uCallbackMessage = pData->data.uCallbackMessage;
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

    INT nItems = DPA_GetPtrCount(m_hAppBarDPA);
    while (--nItems >= 0)
    {
        PAPPBAR pAppBar = (PAPPBAR)DPA_GetPtr(m_hAppBarDPA, nItems);
        if (!pAppBar)
            continue;

        if (pAppBar->hWnd == pData->data.hWnd)
        {
            RECT rcOld = pAppBar->rc;
            EliminateAppBar(nItems);
            StuckAppChange(pData->data.hWnd, &rcOld, NULL, FALSE);
        }
    }
}

// ABM_SETPOS
void CAppBarManager::OnAppBarSetPos(_Inout_ PAPPBAR_COMMAND pData)
{
    PAPPBAR pAppBar = FindAppBar(pData->data.hWnd);
    if (!pAppBar)
        return;

    OnAppBarQueryPos(pData);

    PAPPBARDATA pOutput = AppBar_LockOutput(pData);
    if (!pOutput)
        return;

    RECT rcOld = pAppBar->rc, rcNew = pData->data.rc;
    BOOL bChanged = !::EqualRect(&rcOld, &rcNew);

    pAppBar->rc = rcNew;
    pAppBar->uEdge = pData->data.uEdge;

    AppBar_UnLockOutput(pOutput);

    if (bChanged)
        StuckAppChange(pData->data.hWnd, &rcOld, &rcNew, FALSE);
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
BOOL CAppBarManager::AppBarOutsideOf(_In_ const APPBAR *pAppBar1, _In_ const APPBAR *pAppBar2)
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

void CAppBarManager::RedrawDesktop(_In_ HWND hwndDesktop, _Inout_ PRECT prc)
{
    if (!hwndDesktop)
        return;
    ::MapWindowPoints(NULL, hwndDesktop, (POINT*)prc, sizeof(*prc) / sizeof(POINT));
    ::RedrawWindow(hwndDesktop, prc, 0, RDW_ALLCHILDREN | RDW_ERASE | RDW_INVALIDATE);
}
