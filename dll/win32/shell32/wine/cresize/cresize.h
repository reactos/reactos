#pragma once

#ifndef _INC_WINDOWS
    #include <windows.h>
#endif
#include <assert.h>
#include "cvector.h" /* Evan Teran's C vector */

/* The layout anchors for cresize_SetLayoutAnchor */
#define LA_TOP_LEFT         0, 0      /* upper left */
#define LA_TOP_CENTER       50, 0     /* upper center */
#define LA_TOP_RIGHT        100, 0    /* upper right */
#define LA_MIDDLE_LEFT      0, 50     /* middle left */
#define LA_MIDDLE_CENTER    50, 50    /* middle center */
#define LA_MIDDLE_RIGHT     100, 50   /* middle right */
#define LA_BOTTOM_LEFT      0, 100    /* lower left */
#define LA_BOTTOM_CENTER    50, 100   /* lower center */
#define LA_BOTTOM_RIGHT     100, 100  /* lower right */

typedef struct CRESIZE_CTRL_LAYOUT
{
    HWND m_hwndCtrl;
    SIZE m_anchor1;
    SIZE m_margin1;
    SIZE m_anchor2;
    SIZE m_margin2;
} CRESIZE_CTRL_LAYOUT;

typedef struct CRESIZE
{
    HWND            m_hwndParent;
    BOOL            m_bResizeEnabled;
    HWND            m_hwndSizeGrip;
    cvector_vector_type(CRESIZE_CTRL_LAYOUT) m_pLayouts;
} CRESIZE;

static __inline CRESIZE_CTRL_LAYOUT *
cresize_FindCtrlHWND(CRESIZE *pResize, HWND hwndCtrl)
{
    CRESIZE_CTRL_LAYOUT *it;
    for (it = cvector_begin(pResize->m_pLayouts);
         it != cvector_end(pResize->m_pLayouts);
         ++it)
    {
        if (it->m_hwndCtrl == hwndCtrl)
            return it;
    }
    return NULL;
}

static __inline CRESIZE_CTRL_LAYOUT *
cresize_FindCtrlID(CRESIZE *pResize, UINT nCtrlID)
{
    HWND hwndCtrl = GetDlgItem(pResize->m_hwndParent, nCtrlID);
    return cresize_FindCtrlHWND(pResize, hwndCtrl);
}

static __inline void
cresize_ModifySystemMenu(CRESIZE *pResize, BOOL bEnableResize)
{
    if (bEnableResize)
    {
        GetSystemMenu(pResize->m_hwndParent, TRUE);
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
cresize_DoLayout(CRESIZE *pResize, HDWP hDwp, const CRESIZE_CTRL_LAYOUT *pLayout,
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

    NewRect.left = pLayout->m_margin1.cx + width * pLayout->m_anchor1.cx / 100;
    NewRect.top = pLayout->m_margin1.cy + height * pLayout->m_anchor1.cy / 100;
    NewRect.right = pLayout->m_margin2.cx + width * pLayout->m_anchor2.cx / 100;
    NewRect.bottom = pLayout->m_margin2.cy + height * pLayout->m_anchor2.cy / 100;

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
    INT i, count;
    HDWP hDwp;

    assert(IsWindow(pResize->m_hwndParent));

    if (prc)
        ClientRect = *prc;
    else
        GetClientRect(pResize->m_hwndParent, &ClientRect);

    count = (INT)cvector_size(pResize->m_pLayouts);
    if (count == 0)
        return;

    hDwp = BeginDeferWindowPos(count);
    if (hDwp == NULL)
        return;

    for (i = 0; i < count; ++i)
    {
        const CRESIZE_CTRL_LAYOUT *pLayout = &pResize->m_pLayouts[i];
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

// NOTE: Please call cresize_SetLayoutAnchor and/or cresize_SetLayoutAnchorByID to set control layouts.
static __inline void
cresize_SetLayoutAnchor(CRESIZE *pResize, HWND hwndCtrl,
                        INT cx1, INT cy1, INT cx2, INT cy2)
{
    RECT ClientRect, ChildRect;
    SIZE margin1, margin2;
    LONG width, height;
    CRESIZE_CTRL_LAYOUT *pLayout, layout;

    assert(IsWindow(pResize->m_hwndParent));

    GetClientRect(pResize->m_hwndParent, &ClientRect);
    GetWindowRect(hwndCtrl, &ChildRect);
    MapWindowPoints(NULL, pResize->m_hwndParent, (LPPOINT)&ChildRect, 2);

    width = ClientRect.right - ClientRect.left;
    height = ClientRect.bottom - ClientRect.top;

    margin1.cx = ChildRect.left - width * cx1 / 100;
    margin1.cy = ChildRect.top - height * cy1 / 100;
    margin2.cx = ChildRect.right - width * cx2 / 100;
    margin2.cy = ChildRect.bottom - height * cy2 / 100;

    pLayout = cresize_FindCtrlHWND(pResize, hwndCtrl);
    if (pLayout)
        return;

    layout.m_hwndCtrl = hwndCtrl;
    layout.m_anchor1.cx = cx1;
    layout.m_anchor1.cy = cy1;
    layout.m_margin1 = margin1;
    layout.m_anchor2.cx = cx2;
    layout.m_anchor2.cy = cy2;
    layout.m_margin2 = margin2;

    cvector_push_back(pResize->m_pLayouts, layout);
}

static __inline void
cresize_SetLayoutAnchorByID(CRESIZE *pResize, UINT nCtrlID,
                            INT cx1, INT cy1, INT cx2, INT cy2)
{
    HWND hwndCtrl = GetDlgItem(pResize->m_hwndParent, nCtrlID);
    cresize_SetLayoutAnchor(pResize, hwndCtrl, cx1, cy1, cx2, cy2);
}

static __inline CRESIZE *
cresize_Create(HWND hwndParent, BOOL bEnableResize)
{
    CRESIZE *pResize = LocalAlloc(LPTR, sizeof(CRESIZE));
    if (pResize == NULL)
        return NULL;

    assert(IsWindow(hwndParent));
    pResize->m_hwndParent = hwndParent;

    /* NOTE: The parent window must have initially WS_THICKFRAME style. */
    assert(GetWindowLongPtrW(hwndParent, GWL_STYLE) & WS_THICKFRAME);

    cresize_EnableResize(pResize, bEnableResize);

    return pResize;
}

static __inline void
cresize_Destroy(CRESIZE *pResize)
{
    cvector_free(pResize->m_pLayouts);
    LocalFree(pResize);
}
