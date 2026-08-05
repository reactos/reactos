/*
 * PROJECT:     ReactOS Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     User32 Control Gallery
 * COPYRIGHT:   Copyright 2026 Mohammad Amin Mollazadeh <madamin@pm.me>
 */

#pragma once

#define OEMRESOURCE
#include <windows.h>

typedef enum _PAGES
{
    PAGE_BUTTONS,
    PAGE_COMBO,
    PAGE_EDIT,
    PAGE_ICONTITLE,
    PAGE_LISTBOX,
    PAGE_MDI,
    PAGE_SCROLLBAR,
    PAGE_STATIC,
    PAGE_MAX
} PAGE_ID;

typedef struct _PAGE_HOST* PPAGE_HOST;
typedef LRESULT (*PFN_PAGE_PROC)(PPAGE_HOST PageHost, UINT msg, WPARAM wParam, LPARAM lParam);

typedef struct _PAGE
{
    PAGE_ID Id;
    LPCWSTR Title;
    PFN_PAGE_PROC PageProc;
} PAGE, *PPAGE;

typedef struct _PAGE_HOST
{
    HWND Wnd;
    HWND WndParent;
    PPAGE PageData;
    PVOID UserData;
} PAGE_HOST, *PPAGE_HOST;

typedef struct _NAVIGATOR
{
    HWND WndParent;
    RECT ClientRect;
    HWND WndNavigationList;
    UINT CurrentPage;
    UINT PageCount;
    PPAGE_HOST Pages[PAGE_MAX];
} NAVIGATOR, *PNAVIGATOR;

/* Helper function */
static inline
HWND
CreateChild(
    int Id,
    LPCTSTR ClassName,
    LPCTSTR Text,
    DWORD Style,
    int x,
    int y,
    int cx,
    int cy,
    HWND Parent)
{
    return CreateWindowEx(
        0,
        ClassName,
        Text,
        WS_CHILD | WS_VISIBLE | Style,
        x, y, cx, cy,
        Parent,
        (HMENU)(INT_PTR)Id,
        GetModuleHandle(NULL),
        NULL);
}

/* navigation */
#define IDC_NAVIGATION_LIST 1000
#define IDC_NAVIGATION_PAGE_HOST 1001

PNAVIGATOR CreateNavigationHost(HINSTANCE hInst, HWND Parent, PPAGE Pages, UINT PageCount);
void NavigateTo(PNAVIGATOR Nav, PAGE_ID Id);
void UpdateSize(PNAVIGATOR Nav);
void DestroyNavigationHost(PNAVIGATOR Nav);

PPAGE_HOST CreatePageHost(HINSTANCE hInst, HWND Parent, int x, int y, int cx, int cy, PPAGE PageData);
LRESULT CallPageProc(PPAGE_HOST PageHost, UINT msg, WPARAM wParam, LPARAM lParam);
void ShowPage(PPAGE_HOST PageHost);
void HidePage(PPAGE_HOST PageHost);
void DestroyPageHost(PPAGE_HOST PageHost);

/* pages */
LRESULT ButtonPageProc(PPAGE_HOST PageHost, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT EditPageProc(PPAGE_HOST PageHost, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT ComboPageProc(PPAGE_HOST PageHost, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT IconTitlePageProc(PPAGE_HOST PageHost, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT ListBoxPageProc(PPAGE_HOST PageHost, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT MdiPageProc(PPAGE_HOST PageHost, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT ScrollBarPageProc(PPAGE_HOST PageHost, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT StaticPageProc(PPAGE_HOST PageHost, UINT msg, WPARAM wParam, LPARAM lParam);

#define IDC_STATIC -1
