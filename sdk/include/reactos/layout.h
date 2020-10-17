/*
 * PROJECT:     ReactOS headers
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     The layout engine of resizable dialog boxes / windows
 * COPYRIGHT:   Copyright 2020 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */
#pragma once

#include <assert.h>

typedef struct LAYOUT_INFO {
    UINT m_nCtrlID;
    UINT uEdges; /* BF_* flags */
    SIZE m_margin1;
    SIZE m_margin2;
    HWND m_hwndCtrl;
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
    RECT ClientRect;
    SIZE size = { GetSystemMetrics(SM_CXVSCROLL), GetSystemMetrics(SM_CYHSCROLL) };
    const UINT uFlags = SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER;

    GetClientRect(pData->m_hwndParent, &ClientRect);

    if (hDwp)
    {
        hDwp = DeferWindowPos(hDwp, pData->m_hwndGrip, NULL,
                              ClientRect.right - size.cx, ClientRect.bottom - size.cy,
                              size.cx, size.cy, uFlags);
    }
    else
    {
        SetWindowPos(pData->m_hwndGrip, NULL,
                     ClientRect.right - size.cx, ClientRect.bottom - size.cy,
                     size.cx, size.cy, uFlags);
    }
    return hDwp;
}

static __inline void
_layout_ShowGrip(LAYOUT_DATA *pData, BOOL bShow)
{
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
_layout_GetPercents(UINT uEdges, LPRECT prcPercents)
{
    prcPercents->left = (uEdges & BF_LEFT) ? 0 : 100;
    prcPercents->right = (uEdges & BF_RIGHT) ? 100 : 0;
    prcPercents->top = (uEdges & BF_TOP) ? 0 : 100;
    prcPercents->bottom = (uEdges & BF_BOTTOM) ? 100 : 0;
}

static __inline void
LayoutEnableResize(LAYOUT_DATA *pData, BOOL bEnable)
{
    _layout_ShowGrip(pData, bEnable);
    _layout_ModifySystemMenu(pData, bEnable);
}

static __inline HDWP
_layout_DoMoveItem(LAYOUT_DATA *pData, HDWP hDwp, const LAYOUT_INFO *pLayout,
                   const RECT *ClientRect)
{
    RECT ChildRect, NewRect, rcPercents;
    LONG width, height;
    const UINT uFlags = SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREPOSITION;

    if (!GetWindowRect(pLayout->m_hwndCtrl, &ChildRect))
        return hDwp;
    MapWindowPoints(NULL, pData->m_hwndParent, (LPPOINT)&ChildRect, 2);

    width = ClientRect->right - ClientRect->left;
    height = ClientRect->bottom - ClientRect->top;

    _layout_GetPercents(pLayout->uEdges, &rcPercents);
    NewRect.left = pLayout->m_margin1.cx + width * rcPercents.left / 100;
    NewRect.top = pLayout->m_margin1.cy + height * rcPercents.top / 100;
    NewRect.right = pLayout->m_margin2.cx + width * rcPercents.right / 100;
    NewRect.bottom = pLayout->m_margin2.cy + height * rcPercents.bottom / 100;

    if (!EqualRect(&NewRect, &ChildRect))
    {
        hDwp = DeferWindowPos(hDwp, pLayout->m_hwndCtrl, NULL, NewRect.left, NewRect.top,
                              NewRect.right - NewRect.left, NewRect.bottom - NewRect.top,
                              uFlags);
    }
    return hDwp;
}

static __inline void
_layout_ArrangeLayout(LAYOUT_DATA *pData)
{
    RECT ClientRect;
    UINT iItem;
    HDWP hDwp = BeginDeferWindowPos(pData->m_cLayouts + 1);
    if (hDwp == NULL)
        return;

    GetClientRect(pData->m_hwndParent, &ClientRect);

    for (iItem = 0; iItem < pData->m_cLayouts; ++iItem)
        hDwp = _layout_DoMoveItem(pData, hDwp, &pData->m_pLayouts[iItem], &ClientRect);

    hDwp = _layout_MoveGrip(pData, hDwp);
    EndDeferWindowPos(hDwp);
}

// NOTE: Please call LayoutUpdate on parent's WM_SIZE.
static __inline void
LayoutUpdate(HWND ignored1, LAYOUT_DATA *pData, LPCVOID ignored2, UINT ignored3)
{
    UNREFERENCED_PARAMETER(ignored1);
    UNREFERENCED_PARAMETER(ignored2);
    UNREFERENCED_PARAMETER(ignored3);
    if (pData == NULL)
        return;
    assert(IsWindow(pData->m_hwndParent));
    _layout_ArrangeLayout(pData);
}

static __inline void
_layout_InitLayouts(LAYOUT_DATA *pData)
{
    RECT ClientRect, ChildRect, rcPercents;
    LONG width, height;
    UINT iItem;

    GetClientRect(pData->m_hwndParent, &ClientRect);
    width = ClientRect.right - ClientRect.left;
    height = ClientRect.bottom - ClientRect.top;

    for (iItem = 0; iItem < pData->m_cLayouts; ++iItem)
    {
        LAYOUT_INFO *layout = &pData->m_pLayouts[iItem];
        if (layout->m_hwndCtrl == NULL)
        {
            layout->m_hwndCtrl = GetDlgItem(pData->m_hwndParent, layout->m_nCtrlID);
            if (layout->m_hwndCtrl == NULL)
                continue;
        }

        GetWindowRect(layout->m_hwndCtrl, &ChildRect);
        MapWindowPoints(NULL, pData->m_hwndParent, (LPPOINT)&ChildRect, 2);

        _layout_GetPercents(layout->uEdges, &rcPercents);
        layout->m_margin1.cx = ChildRect.left - width * rcPercents.left / 100;
        layout->m_margin1.cy = ChildRect.top - height * rcPercents.top / 100;
        layout->m_margin2.cx = ChildRect.right - width * rcPercents.right / 100;
        layout->m_margin2.cy = ChildRect.bottom - height * rcPercents.bottom / 100;
    }
}

static __inline LAYOUT_DATA *
LayoutInit(HWND hwndParent, const LAYOUT_INFO *pLayouts, UINT cLayouts)
{
    SIZE_T cb;
    LAYOUT_DATA *pData = SHAlloc(sizeof(LAYOUT_DATA));
    if (pData == NULL)
    {
        assert(0);
        return NULL;
    }

    cb = cLayouts * sizeof(LAYOUT_INFO);
    pData->m_cLayouts = cLayouts;
    pData->m_pLayouts = SHAlloc(cb);
    if (pData->m_pLayouts == NULL)
    {
        assert(0);
        SHFree(pData);
        return NULL;
    }
    memcpy(pData->m_pLayouts, pLayouts, cb);

    /* NOTE: The parent window must have initially WS_SIZEBOX style. */
    assert(IsWindow(hwndParent));
    assert(GetWindowLongPtrW(hwndParent, GWL_STYLE) & WS_SIZEBOX);

    pData->m_hwndParent = hwndParent;
    pData->m_hwndGrip = NULL;
    LayoutEnableResize(pData, TRUE);
    _layout_InitLayouts(pData);
    return pData;
}

static __inline void
LayoutDestroy(LAYOUT_DATA *pData)
{
    if (!pData)
        return;
    SHFree(pData->m_pLayouts);
    SHFree(pData);
}
