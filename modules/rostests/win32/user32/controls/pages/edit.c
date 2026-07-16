/*
 * PROJECT:     ReactOS Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     User32 Control Gallery
 * COPYRIGHT:   Copyright 2026 Mohammad Amin Mollazadeh <madamin@pm.me>
 */

#include "../controls.h"

#define IDC_EDIT_LEFT           3001
#define IDC_EDIT_CENTER         3002
#define IDC_EDIT_RIGHT          3003
#define IDC_EDIT_READONLY       3004
#define IDC_EDIT_DISABLED       3005

#define IDC_EDIT_PASSWORD       3006
#define IDC_EDIT_NUMBER         3007
#define IDC_EDIT_UPPERCASE      3008
#define IDC_EDIT_LOWERCASE      3009
#define IDC_EDIT_MAXTEXT        3010

#define IDC_EDIT_MULTILINE      3011

typedef struct _EDIT_PAGE_DATA
{
    HWND hLblLeft;
    HWND hLeftEdit;
    HWND hLblCenter;
    HWND hCenterEdit;
    HWND hLblRight;
    HWND hRightEdit;
    HWND hLblReadOnly;
    HWND hReadOnlyEdit;
    HWND hLblDisabled;
    HWND hDisabledEdit;

    HWND hLblPassword;
    HWND hPasswordEdit;
    HWND hLblNumber;
    HWND hNumberEdit;
    HWND hLblUppercase;
    HWND hUppercaseEdit;
    HWND hLblLowercase;
    HWND hLowercaseEdit;
    HWND hLblMaxText;
    HWND hMaxTextEdit;

    HWND hLblMultiline;
    HWND hMultilineEdit;
} EDIT_PAGE_DATA, *PEDIT_PAGE_DATA;

static LRESULT
InitPage(HWND Parent, PEDIT_PAGE_DATA PageData)
{
    PageData->hLblLeft =
        CreateChild(IDC_STATIC, L"STATIC", L"Left Aligned:", SS_LEFT, 20, 15, 140, 15, Parent);
    PageData->hLeftEdit =
        CreateChild(IDC_EDIT_LEFT, L"EDIT", L"Left text",
                    WS_BORDER | WS_TABSTOP | ES_LEFT | ES_AUTOHSCROLL,
                    20, 35, 140, 25,
                    Parent);

    PageData->hLblCenter =
        CreateChild(IDC_STATIC, L"STATIC", L"Center Aligned:", SS_LEFT, 20, 85, 140, 15, Parent);
    PageData->hCenterEdit =
        CreateChild(IDC_EDIT_CENTER, L"EDIT", L"Center text",
                    WS_BORDER | WS_TABSTOP | ES_CENTER | ES_AUTOHSCROLL,
                    20, 105, 140, 25,
                    Parent);

    PageData->hLblRight =
        CreateChild(IDC_STATIC, L"STATIC", L"Right Aligned:", SS_LEFT, 20, 155, 140, 15, Parent);
    PageData->hRightEdit =
        CreateChild(IDC_EDIT_RIGHT, L"EDIT", L"Right text",
                    WS_BORDER | WS_TABSTOP | ES_RIGHT | ES_AUTOHSCROLL,
                    20, 175, 140, 25,
                    Parent);

    PageData->hLblReadOnly =
        CreateChild(IDC_STATIC, L"STATIC", L"Read-Only:", SS_LEFT, 20, 225, 140, 15, Parent);
    PageData->hReadOnlyEdit =
        CreateChild(IDC_EDIT_READONLY, L"EDIT", L"Read-only text",
                    WS_BORDER | WS_TABSTOP | ES_LEFT | ES_READONLY | ES_AUTOHSCROLL,
                    20, 245, 140, 25,
                    Parent);

    PageData->hLblDisabled =
        CreateChild(IDC_STATIC, L"STATIC", L"Disabled:", SS_LEFT, 20, 295, 140, 15, Parent);
    PageData->hDisabledEdit =
        CreateChild(IDC_EDIT_DISABLED, L"EDIT", L"Disabled text",
                    WS_BORDER | WS_DISABLED | ES_LEFT | ES_AUTOHSCROLL,
                    20, 315, 140, 25,
                    Parent);

    PageData->hLblPassword =
        CreateChild(IDC_STATIC, L"STATIC", L"Password:", SS_LEFT, 180, 15, 140, 15, Parent);
    PageData->hPasswordEdit =
        CreateChild(IDC_EDIT_PASSWORD, L"EDIT", L"Secret123",
                    WS_BORDER | WS_TABSTOP | ES_LEFT | ES_PASSWORD | ES_AUTOHSCROLL,
                    180, 35, 140, 25,
                    Parent);

    PageData->hLblNumber =
        CreateChild(IDC_STATIC, L"STATIC", L"Number Only:", SS_LEFT, 180, 85, 140, 15, Parent);
    PageData->hNumberEdit =
        CreateChild(IDC_EDIT_NUMBER, L"EDIT", L"123456",
                    WS_BORDER | WS_TABSTOP | ES_LEFT | ES_NUMBER | ES_AUTOHSCROLL,
                    180, 105, 140, 25,
                    Parent);

    PageData->hLblUppercase =
        CreateChild(IDC_STATIC, L"STATIC", L"Uppercase:", SS_LEFT, 180, 155, 140, 15, Parent);
    PageData->hUppercaseEdit =
        CreateChild(IDC_EDIT_UPPERCASE, L"EDIT", L"all caps",
                    WS_BORDER | WS_TABSTOP | ES_LEFT | ES_UPPERCASE | ES_AUTOHSCROLL,
                    180, 175, 140, 25,
                    Parent);

    PageData->hLblLowercase =
        CreateChild(IDC_STATIC, L"STATIC", L"Lowercase:", SS_LEFT, 180, 225, 140, 15, Parent);
    PageData->hLowercaseEdit =
        CreateChild(IDC_EDIT_LOWERCASE, L"EDIT", L"ALL LOWER",
                    WS_BORDER | WS_TABSTOP | ES_LEFT | ES_LOWERCASE | ES_AUTOHSCROLL,
                    180, 245, 140, 25,
                    Parent);

    PageData->hLblMaxText =
        CreateChild(IDC_STATIC, L"STATIC", L"Max Length (5):", SS_LEFT, 180, 295, 140, 15, Parent);
    PageData->hMaxTextEdit =
        CreateChild(IDC_EDIT_MAXTEXT, L"EDIT", L"ABCDE",
                    WS_BORDER | WS_TABSTOP | ES_LEFT | ES_AUTOHSCROLL,
                    180, 315, 140, 25,
                    Parent);
    /* Enforce character limit natively using EM_SETLIMITTEXT */
    SendMessageW(PageData->hMaxTextEdit, EM_SETLIMITTEXT, 5, 0);

    PageData->hLblMultiline =
        CreateChild(IDC_STATIC, L"STATIC", L"Multiline / Scroll:", SS_LEFT, 340, 15, 140, 15, Parent);
    PageData->hMultilineEdit =
        CreateChild(IDC_EDIT_MULTILINE, L"EDIT",
                    L"Line 1: Standard text\r\n"
                    L"Line 2: Multiline editor\r\n"
                    L"Line 3: Auto-scrolling\r\n"
                    L"Line 4: Word wrap disabled\r\n"
                    L"Line 5: Second paragraph...\r\n"
                    L"Line 6: Keep typing to test vertical scrolling!",
                    WS_BORDER | WS_TABSTOP | WS_VSCROLL | WS_HSCROLL |
                    ES_LEFT | ES_MULTILINE | ES_WANTRETURN | ES_AUTOVSCROLL | ES_AUTOHSCROLL,
                    340, 35, 140, 305,
                    Parent);

    return TRUE;
}

static VOID
OnSize(PEDIT_PAGE_DATA PageData, int cx, int cy)
{
    HDWP hdwp;
    int margin_x = 20;
    int margin_y = 15;
    int gap = 20;
    int col_fixed_w = 140;

    int col1_x = margin_x;
    int col2_x = col1_x + col_fixed_w + gap;
    int col3_x = col2_x + col_fixed_w + gap;

    /* Column 3 stretches to fill remaining width */
    int col3_w = cx - col3_x - margin_x;
    if (col3_w < 100) col3_w = 100;

    int label_h = 15;

    /* Multiline stretches to fill remaining height */
    int multi_h = cy - margin_y - label_h - 5 - margin_y;
    if (multi_h < 100) multi_h = 100;

    if (!PageData || !PageData->hLeftEdit)
        return;

    hdwp = BeginDeferWindowPos(22);

#define MOVE_CTRL(hwnd, x, y, w, h) \
    if (hwnd) hdwp = DeferWindowPos(hdwp, hwnd, NULL, x, y, w, h, SWP_NOZORDER)

    /* --- COLUMN 3 --- */
    /* Multiline / Scroll */
    MOVE_CTRL(PageData->hLblMultiline,  col3_x, margin_y, col3_w, label_h);
    MOVE_CTRL(PageData->hMultilineEdit, col3_x, margin_y + 20, col3_w, multi_h);

#undef MOVE_CTRL

    EndDeferWindowPos(hdwp);
}

static
LRESULT
OnCommand(PPAGE_HOST PageHost, PEDIT_PAGE_DATA PageData, int Id, int NotifyCode)
{
    switch (Id)
    {
        case IDC_EDIT_LEFT:
        case IDC_EDIT_CENTER:
        case IDC_EDIT_RIGHT:
        case IDC_EDIT_READONLY:
        case IDC_EDIT_DISABLED:
        case IDC_EDIT_PASSWORD:
        case IDC_EDIT_NUMBER:
        case IDC_EDIT_UPPERCASE:
        case IDC_EDIT_LOWERCASE:
        case IDC_EDIT_MAXTEXT:
        case IDC_EDIT_MULTILINE:
            switch (NotifyCode)
            {
                case EN_CHANGE:
                    return TRUE;
                case EN_UPDATE:
                    return TRUE;
                case EN_MAXTEXT:
                    return TRUE;
                default:
                    return FALSE;
            }

        default:
            return FALSE;
    }
}

LRESULT
EditPageProc(PPAGE_HOST PageHost, UINT msg, WPARAM wParam, LPARAM lParam)
{
    PEDIT_PAGE_DATA PageData;
    if (msg == WM_CREATE)
    {
        PageData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(EDIT_PAGE_DATA));
        PageHost->UserData = PageData;
        return InitPage(PageHost->Wnd, PageData);
    }
    else
    {
        PageData = (PEDIT_PAGE_DATA)PageHost->UserData;
    }

    switch (msg)
    {
        case WM_COMMAND:
            if (LOWORD(wParam) < IDC_EDIT_LEFT || LOWORD(wParam) > IDC_EDIT_MULTILINE)
                return FALSE;
            return OnCommand(PageHost, PageData, LOWORD(wParam), HIWORD(wParam));

        case WM_SIZE:
            OnSize(PageData, LOWORD(lParam), HIWORD(lParam));
            return TRUE;

        default:
            return FALSE;
    }
}
