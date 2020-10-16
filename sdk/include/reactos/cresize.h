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
    INT m_nCtrlID;
    LONG m_cx1, m_cy1; /* layout anchor */
    LONG m_cx2, m_cy2; /* layout anchor */
    SIZE m_margin1;
    SIZE m_margin2;
    HWND m_hwndCtrl;
} CRESIZE_LAYOUT;

typedef struct CRESIZE {
    HWND m_hwndParent;
    BOOL m_bResizeEnabled;
    HWND m_hwndSizeGrip;
    size_t m_cLayouts;
    CRESIZE_LAYOUT *m_pLayouts;
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

    RedrawWindow(pResize->m_hwndParent, NULL, NULL,
                 RDW_FRAME | RDW_INVALIDATE | RDW_ERASENOW);
    InvalidateRect(pResize->m_hwndParent, NULL, TRUE);
}

static __inline void
cresize_MoveSizeGrip(CRESIZE *pResize)
{
    RECT ClientRect;
    INT cx, cy;

    assert(IsWindow(pResize->m_hwndParent));

    if (!pResize->m_hwndSizeGrip)
        return;

    GetClientRect(pResize->m_hwndParent, &ClientRect);

    cx = GetSystemMetrics(SM_CXVSCROLL);
    cy = GetSystemMetrics(SM_CYHSCROLL);
    SetWindowPos(pResize->m_hwndSizeGrip, NULL,
                 ClientRect.right - cx, ClientRect.bottom - cy,
                 cx, cy, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);
}

static __inline void
cresize_ShowSizeGrip(CRESIZE *pResize, BOOL bShow)
{
    assert(IsWindow(pResize->m_hwndParent));

    if (!bShow)
    {
        if (IsWindow(pResize->m_hwndSizeGrip))
            ShowWindow(pResize->m_hwndSizeGrip, SW_HIDE);
        return;
    }

    if (!IsWindow(pResize->m_hwndSizeGrip))
    {
        DWORD style = WS_CHILD | WS_CLIPSIBLINGS | SBS_SIZEGRIP;
        pResize->m_hwndSizeGrip = CreateWindowExW(
            0, L"SCROLLBAR", NULL, style,
            0, 0, 0, 0, pResize->m_hwndParent,
            NULL, GetModuleHandleW(NULL), NULL);
    }

    cresize_MoveSizeGrip(pResize);
    ShowWindow(pResize->m_hwndSizeGrip, SW_SHOWNOACTIVATE);
}

static __inline void
cresize_EnableResize(CRESIZE *pResize, BOOL bEnableResize)
{
    cresize_ShowSizeGrip(pResize, bEnableResize);
    cresize_ModifySystemMenu(pResize, bEnableResize);
    pResize->m_bResizeEnabled = bEnableResize;
}

static __inline HDWP
cresize_DoLayout(CRESIZE *pResize, HDWP hDwp, const CRESIZE_LAYOUT *pLayout,
                 const RECT *ClientRect)
{
    RECT ChildRect, NewRect;
    HWND hwndCtrl = pLayout->m_hwndCtrl;
    INT width, height;
    const UINT uFlags = SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREPOSITION;

    if (!IsWindow(hwndCtrl))
        return hDwp;

    GetWindowRect(hwndCtrl, &ChildRect);
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
        hDwp = DeferWindowPos(hDwp, hwndCtrl, NULL,
            NewRect.left, NewRect.top, width, height, uFlags);
    }

    InvalidateRect(hwndCtrl, NULL, TRUE);
    return hDwp;
}

static __inline void
cresize_ArrangeLayout(CRESIZE *pResize, const RECT *prc OPTIONAL)
{
    RECT ClientRect;
    size_t i;
    HDWP hDwp;

    assert(IsWindow(pResize->m_hwndParent));

    if (prc)
        ClientRect = *prc;
    else
        GetClientRect(pResize->m_hwndParent, &ClientRect);

    hDwp = BeginDeferWindowPos((INT)pResize->m_cLayouts);
    if (hDwp == NULL)
        return;

    for (i = 0; i < pResize->m_cLayouts; ++i)
    {
        const CRESIZE_LAYOUT *pLayout = &pResize->m_pLayouts[i];
        hDwp = cresize_DoLayout(pResize, hDwp, pLayout, &ClientRect);
    }

    EndDeferWindowPos(hDwp);
}

// NOTE: Please call cresize_OnSize on parent's WM_SIZE.
static __inline void
cresize_OnSize(CRESIZE *pResize, const RECT *prcClient OPTIONAL)
{
    if (pResize == NULL)
        return;
    cresize_ArrangeLayout(pResize, prcClient);
    cresize_MoveSizeGrip(pResize);
}

static __inline void
cresize_InitializeLayouts(CRESIZE *pResize)
{
    RECT ClientRect, ChildRect;
    LONG width, height;
    size_t iItem;
    HWND hwndCtrl;

    assert(IsWindow(pResize->m_hwndParent));
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
        hwndCtrl = layout->m_hwndCtrl;

        GetWindowRect(hwndCtrl, &ChildRect);
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
cresize_Create(HWND hwndParent, const CRESIZE_LAYOUT *pLayouts, size_t cLayouts,
               BOOL bEnableResize)
{
    size_t cb;
    CRESIZE *pResize = SHAlloc(sizeof(CRESIZE));
    if (pResize == NULL)
        return NULL;

    cb = cLayouts * sizeof(CRESIZE_LAYOUT);
    pResize->m_cLayouts = cLayouts;
    pResize->m_pLayouts = SHAlloc(cb);
    if (pResize->m_pLayouts == NULL)
    {
        SHFree(pResize);
        return NULL;
    }
    memcpy(pResize->m_pLayouts, pLayouts, cb);

    /* NOTE: The parent window must have initially WS_THICKFRAME style. */
    assert(IsWindow(hwndParent));
    assert(GetWindowLongPtrW(hwndParent, GWL_STYLE) & WS_THICKFRAME);

    pResize->m_hwndParent = hwndParent;
    pResize->m_bResizeEnabled = FALSE;
    pResize->m_hwndSizeGrip = NULL;
    cresize_EnableResize(pResize, bEnableResize);
    cresize_InitializeLayouts(pResize);

    return pResize;
}

static __inline void
cresize_Destroy(CRESIZE *pResize)
{
    SHFree(pResize->m_pLayouts);
    SHFree(pResize);
}
