/*
 * PROJECT:     ReactOS Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     User32 Control Gallery - IconTitle Page
 * COPYRIGHT:   Copyright 2026 Mohammad Amin Mollazadeh <madamin@pm.me>
 */

#include "../controls.h"

#define IDC_ICONTITLE_OWNER1         9101
#define IDC_ICONTITLE_OWNER2         9102
#define IDC_ICONTITLE_LOG_EDIT       9103
#define IDC_ICONTITLE_BTN_CHANGE_TXT 9104
#define IDC_ICONTITLE_BTN_TOGGLE_ACT 9105
#define IDC_ICONTITLE_BTN_REFRESH    9106
#define IDC_ICONTITLE_BTN_CLEAR      9107

typedef struct _ICONTITLE_PAGE_DATA
{
    /* Left Column: Owner Windows & Popup IconTitles */
    HWND hLabelOwner1, hOwnerWnd1, hIconTitle1;
    HWND hLabelOwner2, hOwnerWnd2, hIconTitle2;

    /* Right Column: Event Log & System Metrics Display */
    HWND hLabelLog, hLogEdit;

    /* Bottom Toolbar */
    HWND hBtnChangeText;
    HWND hBtnToggleActive;
    HWND hBtnRefreshMetrics;
    HWND hBtnClearLog;

    /* State Tracking */
    BOOL bToggleState;
    BOOL bIsActive;
} ICONTITLE_PAGE_DATA, *PICONTITLE_PAGE_DATA;

static VOID
LogIconTitleEvent(HWND hLogEdit, LPCWSTR szText)
{
    int len;

    if (!hLogEdit)
        return;

    len = GetWindowTextLengthW(hLogEdit);
    SendMessageW(hLogEdit, EM_SETSEL, len, len);
    SendMessageW(hLogEdit, EM_REPLACESEL, FALSE, (LPARAM)szText);
    SendMessageW(hLogEdit, EM_REPLACESEL, FALSE, (LPARAM)L"\r\n");
}

static VOID
QueryAndLogSystemMetrics(PICONTITLE_PAGE_DATA PageData)
{
    WCHAR szBuffer[256];
    LOGFONTW lf = {0};
    int cxSpacing, cySpacing;

    cxSpacing = GetSystemMetrics(SM_CXICONSPACING);
    cySpacing = GetSystemMetrics(SM_CYICONSPACING);

    SystemParametersInfoW(SPI_GETICONTITLELOGFONT, sizeof(LOGFONTW), &lf, 0);

    wsprintfW(szBuffer, L"--- System Icon Title Metrics ---\r\n"
                        L"Font Face: %s (%d pt)\r\n"
                        L"Icon Spacing: %d x %d px\r\n"
                        L"---------------------------------",
              lf.lfFaceName,
              -MulDiv(lf.lfHeight, 72, GetDeviceCaps(GetDC(PageData->hLogEdit), LOGPIXELSY)),
              cxSpacing, cySpacing);

    LogIconTitleEvent(PageData->hLogEdit, szBuffer);
}

static VOID
PositionPopupIconTitle(HWND hParentPage, HWND hIconTitle, int x, int y, int w, int h)
{
    POINT pt = { x, y };

    if (!hIconTitle || !IsWindow(hIconTitle))
        return;

    /* Convert page client coordinates to screen coordinates for WS_POPUP window */
    ClientToScreen(hParentPage, &pt);
    SetWindowPos(hIconTitle, NULL, pt.x, pt.y, w, h, SWP_NOZORDER | SWP_NOACTIVATE);
}

static LRESULT
InitPage(HWND Parent, PICONTITLE_PAGE_DATA PageData)
{
    HINSTANCE hInst = (HINSTANCE)GetWindowLongPtrW(Parent, GWLP_HINSTANCE);

    PageData->hLabelOwner1 =
        CreateChild(IDC_STATIC, L"STATIC", L"Icon Owner Window #1:", SS_LEFT, 0, 0, 0, 0, Parent);
    PageData->hOwnerWnd1 =
        CreateChild(IDC_ICONTITLE_OWNER1,
                    L"STATIC",
                    L"My Computer",
                    SS_CENTER | WS_BORDER | WS_TABSTOP,
                    0, 0, 0, 0,
                    Parent);
    PageData->hIconTitle1 =
        CreateWindowExW(0,
                        L"IconTitle",
                        NULL,
                        WS_POPUP | WS_VISIBLE | WS_BORDER | WS_CLIPSIBLINGS,
                        0, 0, 0, 0,
                        PageData->hOwnerWnd1,
                        NULL, hInst, NULL);

    PageData->hLabelOwner2 =
        CreateChild(IDC_STATIC, L"STATIC", L"Icon Owner Window #2:", SS_LEFT, 0, 0, 0, 0, Parent);
    PageData->hOwnerWnd2 =
        CreateChild(IDC_ICONTITLE_OWNER2,
                    L"STATIC",
                    L"Recycle Bin (Double-line wrapped label demo)",
                    SS_CENTER | WS_BORDER | WS_TABSTOP,
                    0, 0, 0, 0,
                    Parent);
    PageData->hIconTitle2 =
        CreateWindowExW(0,
                        L"IconTitle",
                        NULL,
                        WS_POPUP | WS_VISIBLE | WS_BORDER | WS_CLIPSIBLINGS,
                        0, 0, 0, 0,
                        PageData->hOwnerWnd2,
                        NULL, hInst, NULL);

    PageData->hLabelLog =
        CreateChild(IDC_STATIC, L"STATIC", L"IconTitle Event Log:", SS_LEFT, 0, 0, 0, 0, Parent);
    PageData->hLogEdit =
        CreateChild(IDC_ICONTITLE_LOG_EDIT,
                    L"EDIT", L"",
                    WS_BORDER | WS_TABSTOP | WS_VSCROLL |
                    ES_LEFT | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
                    0, 0, 0, 0, Parent);

    PageData->hBtnChangeText =
        CreateChild(IDC_ICONTITLE_BTN_CHANGE_TXT,
                    L"BUTTON", L"Change Title",
                    BS_PUSHBUTTON | WS_TABSTOP,
                    0, 0, 0, 0,
                    Parent);
    PageData->hBtnToggleActive =
        CreateChild(IDC_ICONTITLE_BTN_TOGGLE_ACT,
                    L"BUTTON", L"Toggle Active",
                    BS_PUSHBUTTON | WS_TABSTOP,
                    0, 0, 0, 0,
                    Parent);
    PageData->hBtnRefreshMetrics =
        CreateChild(IDC_ICONTITLE_BTN_REFRESH,
                    L"BUTTON", L"Refresh Metrics",
                    BS_PUSHBUTTON | WS_TABSTOP,
                    0, 0, 0, 0,
                    Parent);
    PageData->hBtnClearLog =
        CreateChild(IDC_ICONTITLE_BTN_CLEAR,
                    L"BUTTON", L"Clear Log",
                    BS_PUSHBUTTON | WS_TABSTOP,
                    0, 0, 0, 0,
                    Parent);

    QueryAndLogSystemMetrics(PageData);

    return TRUE;
}

static LRESULT
OnCommand(PPAGE_HOST PageHost, PICONTITLE_PAGE_DATA PageData, int Id)
{
    switch (Id)
    {
        case IDC_ICONTITLE_BTN_CHANGE_TXT:
            PageData->bToggleState = !PageData->bToggleState;

            if (PageData->bToggleState)
            {
                SetWindowTextW(PageData->hOwnerWnd1, L"Network Neighborhood");
                SetWindowTextW(PageData->hOwnerWnd2, L"Control Panel Settings");
            }
            else
            {
                SetWindowTextW(PageData->hOwnerWnd1, L"My Computer");
                SetWindowTextW(PageData->hOwnerWnd2, L"Recycle Bin (Double-line wrapped label demo)");
            }

            /* Redraw IconTitle controls to reflect updated owner text */
            InvalidateRect(PageData->hIconTitle1, NULL, TRUE);
            InvalidateRect(PageData->hIconTitle2, NULL, TRUE);

            LogIconTitleEvent(PageData->hLogEdit, L"Updated owner window titles & invalidated IconTitles.");
            return TRUE;

        case IDC_ICONTITLE_BTN_TOGGLE_ACT:
            PageData->bIsActive = !PageData->bIsActive;

            /* Send WM_NCACTIVATE to simulate active/inactive title bar state */
            SendMessageW(PageData->hIconTitle1, WM_NCACTIVATE, PageData->bIsActive, 0);
            SendMessageW(PageData->hIconTitle2, WM_NCACTIVATE, PageData->bIsActive, 0);

            LogIconTitleEvent(PageData->hLogEdit,
                              PageData->bIsActive ? L"Set IconTitle state -> ACTIVE" : L"Set IconTitle state -> INACTIVE");
            return TRUE;

        case IDC_ICONTITLE_BTN_REFRESH:
            QueryAndLogSystemMetrics(PageData);
            return TRUE;

        case IDC_ICONTITLE_BTN_CLEAR:
            SetWindowTextW(PageData->hLogEdit, L"");
            return TRUE;

        default:
            return FALSE;
    }
}

static VOID
OnSize(HWND hWndPage, PICONTITLE_PAGE_DATA PageData, int cx, int cy)
{
    HDWP hdwp;
    int margin_x = 20;
    int margin_y = 15;
    int gap = 10;
    int col_width;
    int col1_x, col2_x;
    int toolbar_y, toolbar_h = 28;
    int btn_w;
    int log_h;

    if (!PageData || !PageData->hLogEdit)
        return;

    /* Bottom Toolbar Calculations */
    toolbar_y = cy - margin_y - toolbar_h;
    btn_w = (cx - (margin_x * 2) - (gap * 3)) / 4;
    if (btn_w < 60) btn_w = 60;

    /* Column Calculations */
    col_width = (cx - (margin_x * 3)) / 2;
    if (col_width < 100) col_width = 100;

    col1_x = margin_x;
    col2_x = col1_x + col_width + margin_x;

    log_h = toolbar_y - margin_y - 35;
    if (log_h < 80) log_h = 80;

    /* Defer window positions for child controls */
    hdwp = BeginDeferWindowPos(8);

#define MOVE_CTRL(hwnd, x, y, w, h) \
    if (hwnd) hdwp = DeferWindowPos(hdwp, hwnd, NULL, x, y, w, h, SWP_NOZORDER)

    /* Column 1: Owner Static Labels & Windows */
    MOVE_CTRL(PageData->hLabelOwner1, col1_x, 15,  col_width, 15);
    MOVE_CTRL(PageData->hOwnerWnd1,   col1_x, 35,  col_width, 22);

    MOVE_CTRL(PageData->hLabelOwner2, col1_x, 95,  col_width, 15);
    MOVE_CTRL(PageData->hOwnerWnd2,   col1_x, 115, col_width, 22);

    /* Column 2: Log Display */
    MOVE_CTRL(PageData->hLabelLog,    col2_x, 15,  col_width, 15);
    MOVE_CTRL(PageData->hLogEdit,     col2_x, 35,  col_width, log_h);

    /* Bottom Toolbar */
    MOVE_CTRL(PageData->hBtnChangeText,   margin_x,                     toolbar_y, btn_w, toolbar_h);
    MOVE_CTRL(PageData->hBtnToggleActive, margin_x + btn_w + gap,       toolbar_y, btn_w, toolbar_h);
    MOVE_CTRL(PageData->hBtnRefreshMetrics,margin_x + (btn_w + gap) * 2, toolbar_y, btn_w, toolbar_h);
    MOVE_CTRL(PageData->hBtnClearLog,      margin_x + (btn_w + gap) * 3, toolbar_y, btn_w, toolbar_h);

#undef MOVE_CTRL

    EndDeferWindowPos(hdwp);

    /* Position WS_POPUP IconTitle windows using screen coordinates */
    PositionPopupIconTitle(hWndPage, PageData->hIconTitle1, col1_x, 62, col_width, 25);
    PositionPopupIconTitle(hWndPage, PageData->hIconTitle2, col1_x, 142, col_width, 40);
}

LRESULT
IconTitlePageProc(PPAGE_HOST PageHost, UINT msg, WPARAM wParam, LPARAM lParam)
{
    PICONTITLE_PAGE_DATA PageData;

    if (msg == WM_CREATE)
    {
        PageData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(ICONTITLE_PAGE_DATA));
        PageHost->UserData = PageData;

        InitPage(PageHost->Wnd, PageData);
        return TRUE;
    }
    else
    {
        PageData = (PICONTITLE_PAGE_DATA)PageHost->UserData;
    }

    switch (msg)
    {
        case WM_SHOWWINDOW:
            /* Show or hide popup windows when page tab visibility changes */
            if (PageData)
            {
                BOOL bShow = (BOOL)wParam;
                if (PageData->hIconTitle1) ShowWindow(PageData->hIconTitle1, bShow ? SW_SHOW : SW_HIDE);
                if (PageData->hIconTitle2) ShowWindow(PageData->hIconTitle2, bShow ? SW_SHOW : SW_HIDE);
            }
            return 0;

        case WM_COMMAND:
            return OnCommand(PageHost, PageData, LOWORD(wParam));

        case WM_SIZE:
            OnSize(PageHost->Wnd, PageData, LOWORD(lParam), HIWORD(lParam));
            return 0;

        case WM_DESTROY:
            /* Clean up popup windows */
            if (PageData)
            {
                if (PageData->hIconTitle1) DestroyWindow(PageData->hIconTitle1);
                if (PageData->hIconTitle2) DestroyWindow(PageData->hIconTitle2);
            }
            return 0;

        default:
            return FALSE;
    }
}
