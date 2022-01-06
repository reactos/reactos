/*
 * PROJECT:     ReactOS headers
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     The layout engine of resizable dialog boxes / windows
 * COPYRIGHT:   Copyright 2020-2021 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */
#pragma once
#include <assert.h>

typedef struct LAYOUT_INFO {
    UINT m_nCtrlID;
    UINT m_uEdges; /* BF_* flags */
    HWND m_hwndCtrl;
    SIZE m_margin1;
    SIZE m_margin2;
} LAYOUT_INFO;

typedef struct LAYOUT_DATA {
    HWND m_hwndParent;
    HWND m_hwndGrip;
    LAYOUT_INFO *m_pLayouts;
    UINT m_cLayouts;
} LAYOUT_DATA;

static __inline void
_layout_ModifySystemMenu(LAYOUT_DATA *pData, BOOL bEnableResize)
{
    if (bEnableResize)
    {
        GetSystemMenu(pData->m_hwndParent, TRUE); /* revert */
    }
    else
    {
        HMENU hSysMenu = GetSystemMenu(pData->m_hwndParent, FALSE);
        RemoveMenu(hSysMenu, SC_MAXIMIZE, MF_BYCOMMAND);
        RemoveMenu(hSysMenu, SC_SIZE, MF_BYCOMMAND);
        RemoveMenu(hSysMenu, SC_RESTORE, MF_BYCOMMAND);
    }
}

static __inline HDWP
_layout_MoveGrip(LAYOUT_DATA *pData, HDWP hDwp OPTIONAL)
{
    if (!IsWindowVisible(pData->m_hwndGrip))
        return hDwp;

    SIZE size = { GetSystemMetrics(SM_CXVSCROLL), GetSystemMetrics(SM_CYHSCROLL) };
    const UINT uFlags = SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_NOCOPYBITS;
    RECT rcClient;
    GetClientRect(pData->m_hwndParent, &rcClient);

    if (hDwp)
    {
        hDwp = DeferWindowPos(hDwp, pData->m_hwndGrip, NULL,
                              rcClient.right - size.cx, rcClient.bottom - size.cy,
                              size.cx, size.cy, uFlags);
    }
    else
    {
        SetWindowPos(pData->m_hwndGrip, NULL,
                     rcClient.right - size.cx, rcClient.bottom - size.cy,
                     size.cx, size.cy, uFlags);
    }
    return hDwp;
}

static __inline void
LayoutShowGrip(LAYOUT_DATA *pData, BOOL bShow)
{
    UINT uSWP = SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE |
                SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED;
    DWORD style = GetWindowLongPtrW(pData->m_hwndParent, GWL_STYLE);
    DWORD new_style = (bShow ? (style | WS_SIZEBOX) : (style & ~WS_SIZEBOX));
    if (style != new_style)
    {
        SetWindowLongPtrW(pData->m_hwndParent, GWL_STYLE, new_style); /* change style */
        SetWindowPos(pData->m_hwndParent, NULL, 0, 0, 0, 0, uSWP); /* frame changed */
    }

    if (!bShow)
    {
        ShowWindow(pData->m_hwndGrip, SW_HIDE);
        return;
    }

    if (pData->m_hwndGrip == NULL)
    {
        DWORD style = WS_CHILD | WS_CLIPSIBLINGS | SBS_SIZEGRIP;
        pData->m_hwndGrip = CreateWindowExW(0, L"SCROLLBAR", NULL, style,
                                            0, 0, 0, 0, pData->m_hwndParent,
                                            NULL, GetModuleHandleW(NULL), NULL);
    }
    _layout_MoveGrip(pData, NULL);
    ShowWindow(pData->m_hwndGrip, SW_SHOWNOACTIVATE);
}

static __inline void
_layout_GetPercents(LPRECT prcPercents, UINT uEdges)
{
    prcPercents->left = (uEdges & BF_LEFT) ? 0 : 100;
    prcPercents->right = (uEdges & BF_RIGHT) ? 100 : 0;
    prcPercents->top = (uEdges & BF_TOP) ? 0 : 100;
    prcPercents->bottom = (uEdges & BF_BOTTOM) ? 100 : 0;
}

static __inline HDWP
_layout_DoMoveItem(LAYOUT_DATA *pData, HDWP hDwp, const LAYOUT_INFO *pLayout,
                   const RECT *rcClient)
{
    RECT rcChild, NewRect, rcPercents;
    LONG nWidth, nHeight;

    if (!GetWindowRect(pLayout->m_hwndCtrl, &rcChild))
        return hDwp;
    MapWindowPoints(NULL, pData->m_hwndParent, (LPPOINT)&rcChild, 2);

    nWidth = rcClient->right - rcClient->left;
    nHeight = rcClient->bottom - rcClient->top;

    _layout_GetPercents(&rcPercents, pLayout->m_uEdges);
    NewRect.left = pLayout->m_margin1.cx + nWidth * rcPercents.left / 100;
    NewRect.top = pLayout->m_margin1.cy + nHeight * rcPercents.top / 100;
    NewRect.right = pLayout->m_margin2.cx + nWidth * rcPercents.right / 100;
    NewRect.bottom = pLayout->m_margin2.cy + nHeight * rcPercents.bottom / 100;

    if (!EqualRect(&NewRect, &rcChild))
    {
        hDwp = DeferWindowPos(hDwp, pLayout->m_hwndCtrl, NULL, NewRect.left, NewRect.top,
                              NewRect.right - NewRect.left, NewRect.bottom - NewRect.top,
                              SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREPOSITION | SWP_NOCOPYBITS);
    }
    return hDwp;
}

static __inline void
_layout_ArrangeLayout(LAYOUT_DATA *pData)
{
    RECT rcClient;
    UINT iItem;
    HDWP hDwp = BeginDeferWindowPos(pData->m_cLayouts + 1);
    if (hDwp == NULL)
        return;

    GetClientRect(pData->m_hwndParent, &rcClient);

    for (iItem = 0; iItem < pData->m_cLayouts; ++iItem)
        hDwp = _layout_DoMoveItem(pData, hDwp, &pData->m_pLayouts[iItem], &rcClient);

    hDwp = _layout_MoveGrip(pData, hDwp);
    EndDeferWindowPos(hDwp);
}

static __inline void
_layout_InitLayouts(LAYOUT_DATA *pData)
{
    RECT rcClient, rcChild, rcPercents;
    LONG nWidth, nHeight;
    UINT iItem;

    GetClientRect(pData->m_hwndParent, &rcClient);
    nWidth = rcClient.right - rcClient.left;
    nHeight = rcClient.bottom - rcClient.top;

    for (iItem = 0; iItem < pData->m_cLayouts; ++iItem)
    {
        LAYOUT_INFO *pInfo = &pData->m_pLayouts[iItem];
        if (pInfo->m_hwndCtrl == NULL)
        {
            pInfo->m_hwndCtrl = GetDlgItem(pData->m_hwndParent, pInfo->m_nCtrlID);
            if (pInfo->m_hwndCtrl == NULL)
                continue;
        }

        GetWindowRect(pInfo->m_hwndCtrl, &rcChild);
        MapWindowPoints(NULL, pData->m_hwndParent, (LPPOINT)&rcChild, 2);

        _layout_GetPercents(&rcPercents, pInfo->m_uEdges);
        pInfo->m_margin1.cx = rcChild.left - nWidth * rcPercents.left / 100;
        pInfo->m_margin1.cy = rcChild.top - nHeight * rcPercents.top / 100;
        pInfo->m_margin2.cx = rcChild.right - nWidth * rcPercents.right / 100;
        pInfo->m_margin2.cy = rcChild.bottom - nHeight * rcPercents.bottom / 100;
    }
}

/* NOTE: Please call LayoutUpdate on parent's WM_SIZE. */
static __inline void
LayoutUpdate(HWND ignored1, LAYOUT_DATA *pData, LPCVOID ignored2, UINT ignored3)
{
    UNREFERENCED_PARAMETER(ignored1);
    UNREFERENCED_PARAMETER(ignored2);
    UNREFERENCED_PARAMETER(ignored3);
    if (pData == NULL || !pData->m_hwndParent)
        return;
    assert(IsWindow(pData->m_hwndParent));
    _layout_ArrangeLayout(pData);
}

static __inline void
LayoutEnableResize(LAYOUT_DATA *pData, BOOL bEnable)
{
    LayoutShowGrip(pData, bEnable);
    _layout_ModifySystemMenu(pData, bEnable);
}

static __inline LAYOUT_DATA *
LayoutInit(HWND hwndParent, const LAYOUT_INFO *pLayouts, INT cLayouts)
{
    BOOL bShowGrip;
    SIZE_T cb;
    LAYOUT_DATA *pData = (LAYOUT_DATA *)HeapAlloc(GetProcessHeap(), 0, sizeof(LAYOUT_DATA));
    if (pData == NULL)
    {
        assert(0);
        return NULL;
    }

    if (cLayouts < 0) /* NOTE: If cLayouts was negative, then don't show size grip */
    {
        cLayouts = -cLayouts;
        bShowGrip = FALSE;
    }
    else
    {
        bShowGrip = TRUE;
    }

    cb = cLayouts * sizeof(LAYOUT_INFO);
    pData->m_cLayouts = cLayouts;
    pData->m_pLayouts = (LAYOUT_INFO *)HeapAlloc(GetProcessHeap(), 0, cb);
    if (pData->m_pLayouts == NULL)
    {
        assert(0);
        HeapFree(GetProcessHeap(), 0, pData);
        return NULL;
    }
    memcpy(pData->m_pLayouts, pLayouts, cb);

    assert(IsWindow(hwndParent));

    pData->m_hwndParent = hwndParent;

    pData->m_hwndGrip = NULL;
    if (bShowGrip)
        LayoutShowGrip(pData, bShowGrip);

    _layout_InitLayouts(pData);
    return pData;
}

static __inline void
LayoutDestroy(LAYOUT_DATA *pData)
{
    if (!pData)
        return;
    HeapFree(GetProcessHeap(), 0, pData->m_pLayouts);
    HeapFree(GetProcessHeap(), 0, pData);
}
