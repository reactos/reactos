/*
 * PROJECT:     ReactOS Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     User32 Control Gallery
 * COPYRIGHT:   Copyright 2026 Mohammad Amin Mollazadeh <madamin@pm.me>
 */

#include "../controls.h"

#define IDC_MDI_CLIENT      6000
#define IDC_MDI_NEW         6001
#define IDC_MDI_CASCADE     6002
#define IDC_MDI_TILEHORZ    6003
#define IDC_MDI_TILEVERT    6004
#define IDC_MDI_ARRANGE     6005
#define IDC_MDI_NEXT        6006
#define IDC_MDI_CLOSE       6007

typedef struct _MDI_PAGE_DATA
{
    /* Toolbar Action Buttons */
    HWND hBtnNew;
    HWND hBtnCascade;
    HWND hBtnTileHorz;
    HWND hBtnTileVert;
    HWND hBtnArrange;
    HWND hBtnNext;
    HWND hBtnClose;

    /* MDI Client Area */
    HWND hMdiClient;

    /* Child counter for unique titles */
    UINT ChildCount;
} MDI_PAGE_DATA, *PMDI_PAGE_DATA;

static LRESULT CALLBACK
GalleryMDIChildProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_CREATE:
            /* Custom child window rendering/initialization can go here */
            break;

        default:
            return DefMDIChildProcW(hWnd, msg, wParam, lParam);
    }
    return 0;
}

static VOID
RegisterMDIChildClass(VOID)
{
    WNDCLASSW wc = {0};
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = GalleryMDIChildProc;
    wc.hInstance = GetModuleHandleW(NULL);
    wc.hCursor = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"GalleryMDIChild";

    RegisterClassW(&wc);
}

static HWND
CreateMDIChildWindow(HWND hMdiClient, LPCWSTR szTitle, DWORD dwStyle, int x, int y, int cx, int cy)
{
    MDICREATESTRUCTW mcs = {0};
    mcs.szClass = L"GalleryMDIChild";
    mcs.szTitle = szTitle;
    mcs.hOwner = GetModuleHandleW(NULL);
    mcs.x = x;
    mcs.y = y;
    mcs.cx = cx;
    mcs.cy = cy;
    mcs.style = dwStyle;

    return (HWND)SendMessageW(hMdiClient, WM_MDICREATE, 0, (LPARAM)&mcs);
}

static LRESULT
InitPage(HWND Parent, PMDI_PAGE_DATA PageData)
{
    CLIENTCREATESTRUCT ccs = {0};

    RegisterMDIChildClass();

    /* Action Toolbar Buttons */
    PageData->hBtnNew =
        CreateChild(IDC_MDI_NEW, L"BUTTON", L"New Window",
                    BS_PUSHBUTTON | WS_TABSTOP,
                    0, 0, 0, 0,
                    Parent);
    PageData->hBtnCascade =
        CreateChild(IDC_MDI_CASCADE, L"BUTTON", L"Cascade",
                    BS_PUSHBUTTON | WS_TABSTOP,
                    0, 0, 0, 0,
                    Parent);
    PageData->hBtnTileHorz =
        CreateChild(IDC_MDI_TILEHORZ,
                    L"BUTTON", L"Tile Horz",
                    BS_PUSHBUTTON | WS_TABSTOP,
                    0, 0, 0, 0,
                    Parent);
    PageData->hBtnTileVert =
        CreateChild(IDC_MDI_TILEVERT,
                    L"BUTTON", L"Tile Vert",
                    BS_PUSHBUTTON | WS_TABSTOP,
                    0, 0, 0, 0,
                    Parent);

    PageData->hBtnArrange =
        CreateChild(IDC_MDI_ARRANGE, L"BUTTON", L"Arrange Icons",
                    BS_PUSHBUTTON | WS_TABSTOP,
                    0, 0, 0, 0,
                    Parent);
    PageData->hBtnNext =
        CreateChild(IDC_MDI_NEXT,
                    L"BUTTON", L"Next Window",
                    BS_PUSHBUTTON | WS_TABSTOP,
                    0, 0, 0, 0,
                    Parent);
    PageData->hBtnClose =
        CreateChild(IDC_MDI_CLOSE,
                    L"BUTTON", L"Close Active",
                    BS_PUSHBUTTON | WS_TABSTOP,
                    0, 0, 0, 0,
                    Parent);

    /* Create the native user32 MDICLIENT window */
    ccs.hWindowMenu = NULL;
    ccs.idFirstChild = 5000;

    PageData->hMdiClient =
        CreateWindowExW(WS_EX_CLIENTEDGE,
                        L"MDICLIENT",
                        NULL,
                        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_VSCROLL | WS_HSCROLL,
                        0, 0, 0, 0,
                        Parent,
                        (HMENU)(UINT_PTR)IDC_MDI_CLIENT,
                        GetModuleHandleW(NULL),
                        &ccs);

    /* Populate initial MDI children demonstrating various states */
    PageData->ChildCount = 3;

    CreateMDIChildWindow(PageData->hMdiClient, L"Document 1 (Normal)", 0, 10, 10, 200, 120);
    CreateMDIChildWindow(PageData->hMdiClient, L"Document 2 (Normal)", 0, 40, 40, 200, 120);
    CreateMDIChildWindow(PageData->hMdiClient, L"Document 3 (Minimized)", WS_MINIMIZE, 70, 70, 200, 120);

    return TRUE;
}

static LRESULT
OnCommand(PPAGE_HOST PageHost, PMDI_PAGE_DATA PageData, int Id)
{
    HWND hActive;
    WCHAR szTitle[64];

    switch (Id)
    {
        case IDC_MDI_NEW:
            PageData->ChildCount++;
            wsprintfW(szTitle, L"Document %u", PageData->ChildCount);
            CreateMDIChildWindow(PageData->hMdiClient, szTitle, 0, 20, 20, 220, 130);
            return TRUE;

        case IDC_MDI_CASCADE:
            SendMessageW(PageData->hMdiClient, WM_MDICASCADE, 0, 0);
            return TRUE;

        case IDC_MDI_TILEHORZ:
            SendMessageW(PageData->hMdiClient, WM_MDITILE, MDITILE_HORIZONTAL, 0);
            return TRUE;

        case IDC_MDI_TILEVERT:
            SendMessageW(PageData->hMdiClient, WM_MDITILE, MDITILE_VERTICAL, 0);
            return TRUE;

        case IDC_MDI_ARRANGE:
            SendMessageW(PageData->hMdiClient, WM_MDIICONARRANGE, 0, 0);
            return TRUE;

        case IDC_MDI_NEXT:
            hActive = (HWND)SendMessageW(PageData->hMdiClient, WM_MDIGETACTIVE, 0, 0);
            SendMessageW(PageData->hMdiClient, WM_MDINEXT, (WPARAM)hActive, FALSE);
            return TRUE;

        case IDC_MDI_CLOSE:
            hActive = (HWND)SendMessageW(PageData->hMdiClient, WM_MDIGETACTIVE, 0, 0);
            if (hActive)
                SendMessageW(PageData->hMdiClient, WM_MDIDESTROY, (WPARAM)hActive, 0);
            return TRUE;

        default:
            return FALSE;
    }
}

static VOID
OnSize(PMDI_PAGE_DATA PageData, int cx, int cy)
{
    HDWP hdwp;
    int margin_x = 20;
    int btn_w, btn_h = 25;
    int client_w, client_h;

    /* Prevent sizing calculations before controls are created */
    if (!PageData || !PageData->hMdiClient)
        return;

    /* Dynamically calculate toolbar button width to distribute evenly across 4 columns */
    btn_w = (cx - (margin_x * 2) - 30) / 4;
    if (btn_w < 70) btn_w = 70;

    client_w = cx - (margin_x * 2);
    if (client_w < 100) client_w = 100;

    client_h = cy - 85 - 20;
    if (client_h < 100) client_h = 100;

    /* Batch window movements for smooth, flicker-free resizing */
    hdwp = BeginDeferWindowPos(8);

#define MOVE_CTRL(hwnd, x, y, w, h) \
    if (hwnd) hdwp = DeferWindowPos(hdwp, hwnd, NULL, x, y, w, h, SWP_NOZORDER)

    /* Toolbar Row 1 */
    MOVE_CTRL(PageData->hBtnNew,      margin_x,                   15, btn_w, btn_h);
    MOVE_CTRL(PageData->hBtnCascade,  margin_x + btn_w + 10,      15, btn_w, btn_h);
    MOVE_CTRL(PageData->hBtnTileHorz, margin_x + (btn_w + 10)*2,  15, btn_w, btn_h);
    MOVE_CTRL(PageData->hBtnTileVert, margin_x + (btn_w + 10)*3,  15, btn_w, btn_h);

    /* Toolbar Row 2 */
    MOVE_CTRL(PageData->hBtnArrange,  margin_x,                   45, btn_w, btn_h);
    MOVE_CTRL(PageData->hBtnNext,     margin_x + btn_w + 10,      45, btn_w, btn_h);
    MOVE_CTRL(PageData->hBtnClose,    margin_x + (btn_w + 10)*2,  45, btn_w, btn_h);

    /* MDI Workspace Area */
    MOVE_CTRL(PageData->hMdiClient,   margin_x,                   80, client_w, client_h);

#undef MOVE_CTRL

    EndDeferWindowPos(hdwp);
}

LRESULT
MdiPageProc(PPAGE_HOST PageHost, UINT msg, WPARAM wParam, LPARAM lParam)
{
    PMDI_PAGE_DATA PageData;

    if (msg == WM_CREATE)
    {
        PageData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MDI_PAGE_DATA));
        PageHost->UserData = PageData;

        InitPage(PageHost->Wnd, PageData);
        return TRUE;
    }
    else
    {
        PageData = (PMDI_PAGE_DATA)PageHost->UserData;
    }

    switch (msg)
    {
        case WM_SIZE:
            OnSize(PageData, LOWORD(lParam), HIWORD(lParam));
            return 0;

        case WM_COMMAND:
            if (LOWORD(wParam) < IDC_MDI_NEW || LOWORD(wParam) > IDC_MDI_CLOSE)
                return FALSE;
            return OnCommand(PageHost, PageData, LOWORD(wParam));

        default:
            return FALSE;
    }
}
