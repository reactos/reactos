/*
 * PROJECT:     ReactOS Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     User32 Control Gallery
 * COPYRIGHT:   Copyright 2026 Mohammad Amin Mollazadeh <madamin@pm.me>
 */

#include "controls.h"

PNAVIGATOR CreateNavigationHost(HINSTANCE hInst, HWND Parent, PPAGE Pages, UINT PageCount)
{
    PPAGE Page;
    PPAGE_HOST PageHost;
    PNAVIGATOR Nav;
    int x, y, cx, cy;

    if (PageCount > PAGE_MAX) return NULL;

    Nav = (PNAVIGATOR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(NAVIGATOR));
    if (!Nav) return NULL;

    Nav->WndParent = Parent;
    GetClientRect(Parent, &Nav->ClientRect);

    x = Nav->ClientRect.left;
    y = Nav->ClientRect.top;
    cx = 140;
    cy = Nav->ClientRect.bottom - Nav->ClientRect.top;
    Nav->WndNavigationList = CreateChild(IDC_NAVIGATION_LIST,
                                         L"LISTBOX", NULL, LBS_NOTIFY,
                                         x, y, cx, cy,
                                         Parent);

    Nav->PageCount = PageCount;
    x += cx;
    cx = Nav->ClientRect.right - Nav->ClientRect.left - cx;
    for (UINT i = 0; i < PageCount; i++)
    {
        Page = &Pages[i];
        SendMessageW(Nav->WndNavigationList, LB_ADDSTRING, 0, (LPARAM)Page->Title);
        PageHost = CreatePageHost(hInst, Nav->WndParent,
                                  x, y, cx, cy,
                                  Page);
        if (!PageHost)
        {
            for (UINT j = 0; j < i; j++)
                DestroyPageHost(Nav->Pages[j]);
            DestroyWindow(Nav->WndNavigationList);
            HeapFree(GetProcessHeap(), 0, Nav);
            return NULL;
        }
        Nav->Pages[i] = PageHost;
    }

    SendMessageW(Nav->WndNavigationList, LB_SETCURSEL, 0, 0);
    Nav->CurrentPage = 0;
    ShowPage(Nav->Pages[0]);

    return Nav;
}

void NavigateTo(PNAVIGATOR Nav, PAGE_ID Id)
{
    HidePage(Nav->Pages[Nav->CurrentPage]);
    ShowPage(Nav->Pages[Id]);
    Nav->CurrentPage = Id;
}

void UpdateSize(PNAVIGATOR Nav)
{
    int x, y, cx, cy;

    GetClientRect(Nav->WndParent, &Nav->ClientRect);

    x = Nav->ClientRect.left;
    y = Nav->ClientRect.top;
    cx = 140;
    cy = Nav->ClientRect.bottom - y;
    SetWindowPos(Nav->WndNavigationList, NULL, x, y, cx, cy, SWP_NOZORDER);

    x += cx;
    cx = Nav->ClientRect.right - x;
    for (UINT i = 0; i < Nav->PageCount; i++)
    {
        if (Nav->Pages[i])
            SetWindowPos(Nav->Pages[i]->Wnd, NULL, x, y, cx, cy, SWP_NOZORDER);
    }
}

void DestroyNavigationHost(PNAVIGATOR Nav)
{
    if (!Nav) return;

    for (UINT i = 0; i < Nav->PageCount; i++)
    {
        if (Nav->Pages[i])
        {
            DestroyPageHost(Nav->Pages[i]);
        }
    }

    DestroyWindow(Nav->WndNavigationList);
    HeapFree(GetProcessHeap(), 0, Nav);
}
