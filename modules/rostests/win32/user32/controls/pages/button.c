/*
 * PROJECT:     ReactOS Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     User32 Control Gallery
 * COPYRIGHT:   Copyright 2026 Mohammad Amin Mollazadeh <madamin@pm.me>
 */

#include "../controls.h"

#define IDC_NORMAL_BUTTON 2001
#define IDC_DEFAULT_BUTTON 2002
#define IDC_DISABLED_BUTTON 2003

#define IDC_CHECKBOX 2004
#define IDC_AUTOCHECKBOX 2005
#define IDC_THREESTATE_CHECKBOX 2006

#define IDC_RADIOBUTTON1 2007
#define IDC_RADIOBUTTON2 2008
#define IDC_RADIOBUTTON3 2009

#define IDC_IMAGE_BUTTON 2010
#define IDC_ICON_BUTTON 2011

typedef struct _BUTTON_PAGE_DATA
{
    HWND hNormalButton;
    HWND hDefaultButton;
    HWND hDisabledButton;

    HWND hCheckbox;
    HWND hAutoCheckbox;
    HWND hThreeStateCheckbox;

    HWND hRadioButton1;
    HWND hRadioButton2;
    HWND hRadioButton3;

    HWND hImageButton;
    HBITMAP hImage;

    HWND hIconButton;
    HICON hIcon;
} BUTTON_PAGE_DATA, *PBUTTON_PAGE_DATA;

static LRESULT
InitPage(HWND Parent, PBUTTON_PAGE_DATA PageData)
{
    /* Buttons */
    PageData->hNormalButton =
        CreateChild(IDC_NORMAL_BUTTON, L"BUTTON", L"Normal", BS_PUSHBUTTON,
                    20, 20, 100, 30,
                    Parent);
    PageData->hDefaultButton =
        CreateChild(IDC_DEFAULT_BUTTON, L"BUTTON", L"Default", BS_DEFPUSHBUTTON,
                    20, 60, 100, 30,
                    Parent);
    PageData->hDisabledButton =
        CreateChild(IDC_DISABLED_BUTTON, L"BUTTON", L"Disabled",
                    BS_PUSHBUTTON | WS_DISABLED,
                    20, 100, 100, 30,
                    Parent);

    /* Image button */
    PageData->hImageButton =
        CreateChild(IDC_IMAGE_BUTTON, L"BUTTON", L"Image Button", BS_BITMAP,
                    20, 140, 45, 45,
                    Parent);
    PageData->hImage = LoadBitmapW(NULL, MAKEINTRESOURCEW(OBM_TRTYPE));
    SendMessage(PageData->hImageButton, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)PageData->hImage);

    /* Icon button */
    PageData->hIconButton =
        CreateChild(IDC_ICON_BUTTON,
                    L"BUTTON", L"Icon Button", BS_ICON,
                    75, 140, 45, 45,
                    Parent);
    PageData->hIcon = LoadIconW(NULL, MAKEINTRESOURCEW(OIC_WINLOGO));
    SendMessage(PageData->hIconButton, BM_SETIMAGE, IMAGE_ICON, (LPARAM)PageData->hIcon);

    /* Groupbox */
    CreateChild(0, L"BUTTON", L"GroupBox Demo", BS_GROUPBOX,
                20, 200, 200, 150,
                Parent);

    /* Checkboxes */
    PageData->hCheckbox =
        CreateChild(IDC_CHECKBOX, L"BUTTON", L"Checkbox", BS_CHECKBOX,
                    150, 20, 140, 30,
                    Parent);
    PageData->hAutoCheckbox =
        CreateChild(IDC_AUTOCHECKBOX, L"BUTTON", L"Auto Checkbox", BS_AUTOCHECKBOX,
                    150, 60, 140, 30,
                    Parent);
    PageData->hThreeStateCheckbox =
        CreateChild(IDC_THREESTATE_CHECKBOX, L"BUTTON", L"Three-state", BS_AUTO3STATE,
                    150, 100, 140, 30,
                    Parent);

    /* Radio buttons */
    PageData->hRadioButton1 =
        CreateChild(IDC_RADIOBUTTON1, L"BUTTON", L"Radiobutton 1", BS_RADIOBUTTON,
                    300, 20, 140, 30,
                    Parent);
    PageData->hRadioButton2 =
        CreateChild(IDC_RADIOBUTTON2, L"BUTTON", L"Radiobutton 2", BS_RADIOBUTTON,
                    300, 60, 140, 30,
                    Parent);
    PageData->hRadioButton3 =
        CreateChild(IDC_RADIOBUTTON3, L"BUTTON", L"Radiobutton 3", BS_RADIOBUTTON,
                    300, 100, 140, 30,
                    Parent);

    return TRUE;
}

static
LRESULT
OnCommand(PPAGE_HOST PageHost, PBUTTON_PAGE_DATA PageData, int Id)
{
    switch (Id)
    {
        case IDC_NORMAL_BUTTON:
            SetWindowText(PageData->hNormalButton, L"Clicked!");
            return TRUE;

        case IDC_DEFAULT_BUTTON:
            SetWindowText(PageData->hDefaultButton, L"Clicked!");
            return TRUE;

        case IDC_DISABLED_BUTTON:
            SetWindowText(PageData->hDisabledButton, L"Clicked!");
            return TRUE;

        case IDC_CHECKBOX:
            SetWindowText(PageData->hCheckbox, L"Clicked!");
            return TRUE;

        case IDC_RADIOBUTTON1:
        case IDC_RADIOBUTTON2:
        case IDC_RADIOBUTTON3:
            CheckRadioButton(PageHost->Wnd, IDC_RADIOBUTTON1, IDC_RADIOBUTTON3, Id);
            return TRUE;

        default:
            return FALSE;
    }
}

static VOID
OnSize(PBUTTON_PAGE_DATA PageData, int cx, int cy)
{
    HDWP hdwp;
    int margin_x = 20;
    int margin_y = 20;
    int gap = 15;
    int col_width;
    int col1_x, col2_x, col3_x;
    int btn_h = 28;

    if (!PageData || !PageData->hNormalButton)
        return;

    /* Calculate proportional column widths */
    col_width = (cx - (margin_x * 2) - (gap * 2)) / 3;
    if (col_width < 90) col_width = 90;

    col1_x = margin_x;
    col2_x = col1_x + col_width + gap;
    col3_x = col2_x + col_width + gap;

    /* Defer window positions for all 12 controls */
    hdwp = BeginDeferWindowPos(12);

#define MOVE_CTRL(hwnd, x, y, w, h) \
    if (hwnd) hdwp = DeferWindowPos(hdwp, hwnd, NULL, x, y, w, h, SWP_NOZORDER)

    /* Column 1: Push Buttons */
    MOVE_CTRL(PageData->hNormalButton,   col1_x, margin_y,                        col_width, btn_h);
    MOVE_CTRL(PageData->hDefaultButton,  col1_x, margin_y + btn_h + gap,          col_width, btn_h);
    MOVE_CTRL(PageData->hDisabledButton, col1_x, margin_y + (btn_h * 2) + (gap * 2), col_width, btn_h);

    /* Column 2: Checkboxes */
    MOVE_CTRL(PageData->hCheckbox,           col2_x, margin_y,                        col_width, btn_h);
    MOVE_CTRL(PageData->hAutoCheckbox,       col2_x, margin_y + btn_h + gap,          col_width, btn_h);
    MOVE_CTRL(PageData->hThreeStateCheckbox, col2_x, margin_y + (btn_h * 2) + (gap * 2), col_width, btn_h);

    /* Column 3: Radio Buttons */
    MOVE_CTRL(PageData->hRadioButton1, col3_x, margin_y,                        col_width, btn_h);
    MOVE_CTRL(PageData->hRadioButton2, col3_x, margin_y + btn_h + gap,          col_width, btn_h);
    MOVE_CTRL(PageData->hRadioButton3, col3_x, margin_y + (btn_h * 2) + (gap * 2), col_width, btn_h);

#undef MOVE_CTRL

    EndDeferWindowPos(hdwp);
}

LRESULT
ButtonPageProc(PPAGE_HOST PageHost, UINT msg, WPARAM wParam, LPARAM lParam)
{
    PBUTTON_PAGE_DATA PageData;
    if (msg == WM_CREATE)
    {
        PageData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(BUTTON_PAGE_DATA));
        PageHost->UserData = PageData;
        return InitPage(PageHost->Wnd, PageData);
    }
    else
    {
        PageData = (PBUTTON_PAGE_DATA)PageHost->UserData;
    }

    switch (msg)
    {
        case WM_COMMAND:
            if (LOWORD(wParam) < IDC_NORMAL_BUTTON || LOWORD(wParam) > IDC_ICON_BUTTON)
                return FALSE;
            return OnCommand(PageHost, PageData, LOWORD(wParam));

        case WM_SIZE:
            OnSize(PageData, LOWORD(lParam), HIWORD(lParam));
            return TRUE;

        default:
            return FALSE;
    }
}
