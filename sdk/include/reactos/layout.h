/*
 * PROJECT:     ReactOS headers
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     The layout engine of resizable dialog boxes / windows
 * COPYRIGHT:   Copyright 2020 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */
#pragma once

#ifndef _WINDEF_
    #include <windef.h>
#endif
#ifndef _WINBASE_
    #include <winbase.h>
#endif
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
layout_ModifySystemMenu(LAYOUT_DATA *pResize, BOOL bEnableResize)
{
    if (bEnableResize)
    {
        GetSystemMenu(pResize->m_hwndParent, TRUE); /* revert */
    }
    else
    {
        HMENU hSysMenu = GetSystemMenu(pResize->m_hwndParent, FALSE);
        RemoveMenu(hSysMenu, SC_MAXIMIZE, MF_BYCOMMAND);
        RemoveMenu(hSysMenu, SC_SIZE, MF_BYCOMMAND);
        RemoveMenu(hSysMenu, SC_RESTORE, MF_BYCOMMAND);
    }
}

static __inline HDWP
layout_MoveGrip(LAYOUT_DATA *pResize, HDWP hDwp OPTIONAL)
{
    RECT ClientRect;
    INT cx, cy;
    const UINT uFlags = SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER;

    GetClientRect(pResize->m_hwndParent, &ClientRect);

    cx = GetSystemMetrics(SM_CXVSCROLL);
    cy = GetSystemMetrics(SM_CYHSCROLL);
    if (hDwp)
    {
        hDwp = DeferWindowPos(hDwp, pResize->m_hwndGrip, NULL,
                              ClientRect.right - cx, ClientRect.bottom - cy,
                              cx, cy, uFlags);
    }
    else
    {
        SetWindowPos(pResize->m_hwndGrip, NULL,
                     ClientRect.right - cx, ClientRect.bottom - cy, cx, cy, uFlags);
    }
    return hDwp;
}

static __inline void
layout_ShowGrip(LAYOUT_DATA *pResize, BOOL bShow)
{
    if (!bShow)
    {
        ShowWindow(pResize->m_hwndGrip, SW_HIDE);
        return;
    }

    if (pResize->m_hwndGrip == NULL)
    {
        DWORD style = WS_CHILD | WS_CLIPSIBLINGS | SBS_SIZEGRIP;
        pResize->m_hwndGrip = CreateWindowExW(0, L"SCROLLBAR", NULL, style,
                                              0, 0, 0, 0, pResize->m_hwndParent,
                                              NULL, GetModuleHandleW(NULL), NULL);
    }
    layout_MoveGrip(pResize, NULL);
    ShowWindow(pResize->m_hwndGrip, SW_SHOWNOACTIVATE);
}

static __inline void
layout_GetPercents(UINT uEdges, LPRECT prcPercents)
{
    prcPercents->left = (uEdges & BF_LEFT) ? 0 : 100;
    prcPercents->right = (uEdges & BF_RIGHT) ? 100 : 0;
    prcPercents->top = (uEdges & BF_TOP) ? 0 : 100;
    prcPercents->bottom = (uEdges & BF_BOTTOM) ? 100 : 0;
}

static __inline void
LayoutEnableResize(LAYOUT_DATA *pResize, BOOL bEnable)
{
    layout_ShowGrip(pResize, bEnable);
    layout_ModifySystemMenu(pResize, bEnable);
}

static __inline HDWP
layout_DoLayout(LAYOUT_DATA *pResize, HDWP hDwp, const LAYOUT_INFO *pLayout,
                 const RECT *ClientRect)
{
    HWND hwndCtrl = pLayout->m_hwndCtrl;
    RECT ChildRect, NewRect, rcPercents;
    LONG width, height;
    const UINT uFlags = SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREPOSITION;

    if (!GetWindowRect(hwndCtrl, &ChildRect))
        return hDwp;
    MapWindowPoints(NULL, pResize->m_hwndParent, (LPPOINT)&ChildRect, 2);

    width = ClientRect->right - ClientRect->left;
    height = ClientRect->bottom - ClientRect->top;

    layout_GetPercents(pLayout->uEdges, &rcPercents);
    NewRect.left = pLayout->m_margin1.cx + width * rcPercents.left / 100;
    NewRect.top = pLayout->m_margin1.cy + height * rcPercents.top / 100;
    NewRect.right = pLayout->m_margin2.cx + width * rcPercents.right / 100;
    NewRect.bottom = pLayout->m_margin2.cy + height * rcPercents.bottom / 100;

    if (!EqualRect(&NewRect, &ChildRect))
    {
        width = NewRect.right - NewRect.left;
        height = NewRect.bottom - NewRect.top;
        hDwp = DeferWindowPos(hDwp, hwndCtrl, NULL, NewRect.left, NewRect.top,
                              width, height, uFlags);
    }
    return hDwp;
}

static __inline void
layout_ArrangeLayout(LAYOUT_DATA *pResize)
{
    RECT ClientRect;
    UINT iItem;
    HDWP hDwp = BeginDeferWindowPos(pResize->m_cLayouts + 1);
    if (hDwp == NULL)
        return;

    GetClientRect(pResize->m_hwndParent, &ClientRect);

    for (iItem = 0; iItem < pResize->m_cLayouts; ++iItem)
    {
        const LAYOUT_INFO *pLayout = &pResize->m_pLayouts[iItem];
        hDwp = layout_DoLayout(pResize, hDwp, pLayout, &ClientRect);
    }

    hDwp = layout_MoveGrip(pResize, hDwp);
    EndDeferWindowPos(hDwp);
}

// NOTE: Please call LayoutUpdate on parent's WM_SIZE.
static __inline void
LayoutUpdate(HWND ignored1, LAYOUT_DATA *pResize, LPCVOID ignored2, UINT ignored3)
{
    if (pResize == NULL)
        return;
    assert(IsWindow(pResize->m_hwndParent));
    layout_ArrangeLayout(pResize);
}

static __inline void
layout_InitLayouts(LAYOUT_DATA *pResize)
{
    RECT ClientRect, ChildRect, rcPercents;
    LONG width, height;
    UINT iItem;

    GetClientRect(pResize->m_hwndParent, &ClientRect);

    for (iItem = 0; iItem < pResize->m_cLayouts; ++iItem)
    {
        LAYOUT_INFO *layout = &pResize->m_pLayouts[iItem];
        if (layout->m_hwndCtrl == NULL)
        {
            layout->m_hwndCtrl = GetDlgItem(pResize->m_hwndParent, layout->m_nCtrlID);
            if (layout->m_hwndCtrl == NULL)
                continue;
        }

        GetWindowRect(layout->m_hwndCtrl, &ChildRect);
        MapWindowPoints(NULL, pResize->m_hwndParent, (LPPOINT)&ChildRect, 2);

        width = ClientRect.right - ClientRect.left;
        height = ClientRect.bottom - ClientRect.top;

        layout_GetPercents(layout->uEdges, &rcPercents);
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
    LAYOUT_DATA *pResize = SHAlloc(sizeof(LAYOUT_DATA));
    if (pResize == NULL)
    {
        assert(0);
        return NULL;
    }

    cb = cLayouts * sizeof(LAYOUT_INFO);
    pResize->m_cLayouts = cLayouts;
    pResize->m_pLayouts = SHAlloc(cb);
    if (pResize->m_pLayouts == NULL)
    {
        assert(0);
        SHFree(pResize);
        return NULL;
    }
    memcpy(pResize->m_pLayouts, pLayouts, cb);

    /* NOTE: The parent window must have initially WS_SIZEBOX style. */
    assert(IsWindow(hwndParent));
    assert(GetWindowLongPtrW(hwndParent, GWL_STYLE) & WS_SIZEBOX);

    pResize->m_hwndParent = hwndParent;
    pResize->m_hwndGrip = NULL;
    LayoutEnableResize(pResize, TRUE);
    layout_InitLayouts(pResize);
    return pResize;
}

static __inline void
LayoutDestroy(LAYOUT_DATA *pResize)
{
    if (!pResize)
        return;
    SHFree(pResize->m_pLayouts);
    SHFree(pResize);
}
