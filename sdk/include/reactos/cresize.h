/*
 * PROJECT:     ReactOS include
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Resizable dialog box / window
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

/* The layout anchors */
#define LA_TOP_LEFT      0, 0     /* upper left */
#define LA_TOP_CENTER    50, 0    /* upper center */
#define LA_TOP_RIGHT     100, 0   /* upper right */
#define LA_MIDDLE_LEFT   0, 50    /* middle left */
#define LA_MIDDLE_CENTER 50, 50   /* middle center */
#define LA_MIDDLE_RIGHT  100, 50  /* middle right */
#define LA_BOTTOM_LEFT   0, 100   /* lower left */
#define LA_BOTTOM_CENTER 50, 100  /* lower center */
#define LA_BOTTOM_RIGHT  100, 100 /* lower right */

typedef struct CRESIZE_LAYOUT {
    UINT m_nCtrlID;
    LONG m_cx1, m_cy1; /* layout anchor */
    LONG m_cx2, m_cy2; /* layout anchor */
    SIZE m_margin1;
    SIZE m_margin2;
    HWND m_hwndCtrl;
} CRESIZE_LAYOUT;

typedef struct CRESIZE {
    HWND m_hwndParent;
    HWND m_hwndGrip;
    CRESIZE_LAYOUT *m_pLayouts;
    UINT m_cLayouts;
} CRESIZE;

static __inline void
cresize_ModifySystemMenu(CRESIZE *pResize, BOOL bEnableResize)
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
cresize_MoveGrip(CRESIZE *pResize, HDWP hDwp OPTIONAL)
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
cresize_ShowGrip(CRESIZE *pResize, BOOL bShow)
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
    cresize_MoveGrip(pResize, NULL);
    ShowWindow(pResize->m_hwndGrip, SW_SHOWNOACTIVATE);
}

static __inline void
cresize_EnableResize(CRESIZE *pResize, BOOL bEnable)
{
    cresize_ShowGrip(pResize, bEnable);
    cresize_ModifySystemMenu(pResize, bEnable);
}

static __inline HDWP
cresize_DoLayout(CRESIZE *pResize, HDWP hDwp, const CRESIZE_LAYOUT *pLayout,
                 const RECT *ClientRect)
{
    HWND hwndCtrl = pLayout->m_hwndCtrl;
    RECT ChildRect, NewRect;
    LONG width, height;
    const UINT uFlags = SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREPOSITION;

    if (!GetWindowRect(hwndCtrl, &ChildRect))
        return hDwp;
    MapWindowPoints(NULL, pResize->m_hwndParent, (LPPOINT)&ChildRect, 2);

    width = ClientRect->right - ClientRect->left;
    height = ClientRect->bottom - ClientRect->top;

    NewRect.left = pLayout->m_margin1.cx + width * pLayout->m_cx1 / 100;
    NewRect.top = pLayout->m_margin1.cy + height * pLayout->m_cy1 / 100;
    NewRect.right = pLayout->m_margin2.cx + width * pLayout->m_cx2 / 100;
    NewRect.bottom = pLayout->m_margin2.cy + height * pLayout->m_cy2 / 100;

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
cresize_ArrangeLayout(CRESIZE *pResize)
{
    RECT ClientRect;
    UINT iItem;
    HDWP hDwp = BeginDeferWindowPos(pResize->m_cLayouts + 1);
    if (hDwp == NULL)
        return;

    GetClientRect(pResize->m_hwndParent, &ClientRect);

    for (iItem = 0; iItem < pResize->m_cLayouts; ++iItem)
    {
        const CRESIZE_LAYOUT *pLayout = &pResize->m_pLayouts[iItem];
        hDwp = cresize_DoLayout(pResize, hDwp, pLayout, &ClientRect);
    }

    hDwp = cresize_MoveGrip(pResize, hDwp);
    EndDeferWindowPos(hDwp);
}

// NOTE: Please call cresize_OnSize on parent's WM_SIZE.
static __inline void
cresize_OnSize(CRESIZE *pResize)
{
    if (pResize == NULL)
        return;
    assert(IsWindow(pResize->m_hwndParent));
    cresize_ArrangeLayout(pResize);
}

static __inline void
cresize_InitLayouts(CRESIZE *pResize)
{
    RECT ClientRect, ChildRect;
    LONG width, height;
    UINT iItem;

    GetClientRect(pResize->m_hwndParent, &ClientRect);

    for (iItem = 0; iItem < pResize->m_cLayouts; ++iItem)
    {
        CRESIZE_LAYOUT *layout = &pResize->m_pLayouts[iItem];
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

        layout->m_margin1.cx = ChildRect.left - width * layout->m_cx1 / 100;
        layout->m_margin1.cy = ChildRect.top - height * layout->m_cy1 / 100;
        layout->m_margin2.cx = ChildRect.right - width * layout->m_cx2 / 100;
        layout->m_margin2.cy = ChildRect.bottom - height * layout->m_cy2 / 100;
    }
}

static __inline CRESIZE *
cresize_Create(HWND hwndParent, const CRESIZE_LAYOUT *pLayouts, UINT cLayouts)
{
    SIZE_T cb;
    CRESIZE *pResize = SHAlloc(sizeof(CRESIZE));
    if (pResize == NULL)
    {
        assert(0);
        return NULL;
    }

    cb = cLayouts * sizeof(CRESIZE_LAYOUT);
    pResize->m_cLayouts = cLayouts;
    pResize->m_pLayouts = SHAlloc(cb);
    if (pResize->m_pLayouts == NULL)
    {
        assert(0);
        SHFree(pResize);
        return NULL;
    }
    memcpy(pResize->m_pLayouts, pLayouts, cb);

    /* NOTE: The parent window must have initially WS_THICKFRAME style. */
    assert(IsWindow(pResize->m_hwndParent));
    assert(GetWindowLongPtrW(hwndParent, GWL_STYLE) & WS_THICKFRAME);

    pResize->m_hwndParent = hwndParent;
    pResize->m_hwndGrip = NULL;
    cresize_EnableResize(pResize, TRUE);
    cresize_InitLayouts(pResize);
    return pResize;
}

static __inline void
cresize_Destroy(CRESIZE *pResize)
{
    if (!pResize)
        return;
    SHFree(pResize->m_pLayouts);
    SHFree(pResize);
}
