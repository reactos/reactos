/*
 * PROJECT:     ReactOS Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     User32 Control Gallery
 * COPYRIGHT:   Copyright 2026 Mohammad Amin Mollazadeh <madamin@pm.me>
 */

#include "../controls.h"

#define IDC_SIMPLE_COMBO        3001
#define IDC_SORTED_COMBO        3002
#define IDC_DROPDOWN_COMBO      3003
#define IDC_DROPDOWNLIST_COMBO  3004
#define IDC_DISABLED_COMBO      3005
#define IDC_EXTENDEDUI_COMBO    3006
#define IDC_AUTOHSCROLL_COMBO   3007
#define IDC_UPPERCASE_COMBO     3008
#define IDC_LOWERCASE_COMBO     3009
#define IDC_NOINTEGRAL_COMBO    3010

typedef struct _COMBOBOX_PAGE_DATA
{
    HWND hLblSimple;
    HWND hSimpleCombo;
    HWND hLblSorted;
    HWND hSortedCombo;

    HWND hLblDropdown;
    HWND hDropdownCombo;
    HWND hLblDropdownList;
    HWND hDropdownListCombo;
    HWND hLblDisabled;
    HWND hDisabledCombo;
    HWND hLblExtendedUI;
    HWND hExtendedUICombo;

    HWND hLblAutoHScroll;
    HWND hAutoHScrollCombo;
    HWND hLblUppercase;
    HWND hUppercaseCombo;
    HWND hLblLowercase;
    HWND hLowercaseCombo;
    HWND hLblNoIntegral;
    HWND hNoIntegralCombo;
} COMBOBOX_PAGE_DATA, *PCOMBOBOX_PAGE_DATA;

static PCWSTR ComboItems[] = {
    L"Cherry",
    L"Date",
    L"Grape",
    L"Apple",
    L"Banana",
    L"Fig",
    L"Elderberry",
};

static VOID
PopulateComboBox(HWND hCombo, BOOL bSelectFirst)
{
    /* CBS_UPPERCASE and CBS_LOWERCASE require a writable buffer */
    WCHAR ItemBuffer[20];

    for (int i = 0; i < _countof(ComboItems); i++)
    {
        wcscpy(ItemBuffer, ComboItems[i]);
        SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)ItemBuffer);
    }

    if (bSelectFirst)
        SendMessageW(hCombo, CB_SETCURSEL, 0, 0);
}

static LRESULT
InitPage(HWND Parent, PCOMBOBOX_PAGE_DATA PageData)
{
    PageData->hLblSimple =
        CreateChild(IDC_STATIC, L"STATIC", L"Simple Combo:", SS_LEFT, 20, 10, 140, 15, Parent);
    PageData->hSimpleCombo =
        CreateChild(IDC_SIMPLE_COMBO, L"COMBOBOX", L"Simple Combo",
                    CBS_SIMPLE | WS_VSCROLL,
                    20, 30, 140, 320,
                    Parent);
    PopulateComboBox(PageData->hSimpleCombo, TRUE);

    PageData->hLblDropdown =
        CreateChild(IDC_STATIC, L"STATIC", L"Dropdown:", SS_LEFT, 180, 10, 140, 15, Parent);
    PageData->hDropdownCombo =
        CreateChild(IDC_DROPDOWN_COMBO, L"COMBOBOX", L"Editable Text",
                    CBS_DROPDOWN | WS_VSCROLL,
                    180, 30, 140, 140,
                    Parent);
    PopulateComboBox(PageData->hDropdownCombo, FALSE);

    PageData->hLblDropdownList =
        CreateChild(IDC_STATIC, L"STATIC", L"Dropdown List:", SS_LEFT, 180, 60, 140, 15, Parent);
    PageData->hDropdownListCombo =
        CreateChild(IDC_DROPDOWNLIST_COMBO, L"COMBOBOX", L"",
                    CBS_DROPDOWNLIST | WS_VSCROLL,
                    180, 80, 140, 140,
                    Parent);
    PopulateComboBox(PageData->hDropdownListCombo, TRUE);

    PageData->hLblDisabled =
        CreateChild(IDC_STATIC, L"STATIC", L"Disabled:", SS_LEFT, 180, 110, 140, 15, Parent);
    PageData->hDisabledCombo =
        CreateChild(IDC_DISABLED_COMBO, L"COMBOBOX", L"",
                    CBS_DROPDOWNLIST | WS_DISABLED,
                    180, 130, 140, 140,
                    Parent);
    PopulateComboBox(PageData->hDisabledCombo, TRUE);

    PageData->hLblSorted =
        CreateChild(IDC_STATIC, L"STATIC", L"Sorted List:", SS_LEFT, 180, 160, 140, 15, Parent);
    PageData->hSortedCombo =
        CreateChild(IDC_SORTED_COMBO, L"COMBOBOX", L"",
                    CBS_DROPDOWNLIST | CBS_SORT | WS_VSCROLL,
                    180, 180, 140, 140,
                    Parent);
    PopulateComboBox(PageData->hSortedCombo, TRUE);

    PageData->hLblExtendedUI =
        CreateChild(IDC_STATIC, L"STATIC", L"Extended UI:", SS_LEFT, 180, 210, 140, 15, Parent);
    PageData->hExtendedUICombo =
        CreateChild(IDC_EXTENDEDUI_COMBO, L"COMBOBOX", L"",
                    CBS_DROPDOWNLIST | WS_VSCROLL,
                    180, 230, 140, 140,
                    Parent);
    PopulateComboBox(PageData->hExtendedUICombo, TRUE);
    SendMessageW(PageData->hExtendedUICombo, CB_SETEXTENDEDUI, TRUE, 0);

    PageData->hLblAutoHScroll =
        CreateChild(IDC_STATIC, L"STATIC", L"Auto H-Scroll:", SS_LEFT, 180, 260, 140, 15, Parent);
    PageData->hAutoHScrollCombo =
        CreateChild(IDC_AUTOHSCROLL_COMBO, L"COMBOBOX", L"Type a very long string here...",
                    CBS_DROPDOWN | CBS_AUTOHSCROLL | WS_VSCROLL,
                    180, 280, 140, 140,
                    Parent);
    PopulateComboBox(PageData->hAutoHScrollCombo, FALSE);

    PageData->hLblUppercase =
        CreateChild(IDC_STATIC, L"STATIC", L"Uppercase:", SS_LEFT, 340, 10, 140, 15, Parent);
    PageData->hUppercaseCombo =
        CreateChild(IDC_UPPERCASE_COMBO, L"COMBOBOX", L"all caps",
                    CBS_DROPDOWN | CBS_UPPERCASE | WS_VSCROLL,
                    340, 30, 140, 140,
                    Parent);
    PopulateComboBox(PageData->hUppercaseCombo, FALSE);

    PageData->hLblLowercase =
        CreateChild(IDC_STATIC, L"STATIC", L"Lowercase:", SS_LEFT, 340, 60, 140, 15, Parent);
    PageData->hLowercaseCombo =
        CreateChild(IDC_LOWERCASE_COMBO, L"COMBOBOX", L"ALL LOWER",
                    CBS_DROPDOWN | CBS_LOWERCASE | WS_VSCROLL,
                    340, 80, 140, 140,
                    Parent);
    PopulateComboBox(PageData->hLowercaseCombo, FALSE);

    PageData->hLblNoIntegral =
        CreateChild(IDC_STATIC, L"STATIC", L"No Integral Height:", SS_LEFT, 340, 110, 140, 15, Parent);
    PageData->hNoIntegralCombo =
        CreateChild(IDC_NOINTEGRAL_COMBO, L"COMBOBOX", L"",
                    CBS_DROPDOWNLIST | CBS_NOINTEGRALHEIGHT | WS_VSCROLL,
                    340, 130, 140, 65, /* Intentional awkward height */
                    Parent);
    PopulateComboBox(PageData->hNoIntegralCombo, TRUE);

    return TRUE;
}

static
LRESULT
OnCommand(PPAGE_HOST PageHost, PCOMBOBOX_PAGE_DATA PageData, int Id, int NotifyCode)
{
    switch (Id)
    {
        case IDC_SIMPLE_COMBO:
        case IDC_SORTED_COMBO:
        case IDC_DROPDOWN_COMBO:
        case IDC_DROPDOWNLIST_COMBO:
        case IDC_DISABLED_COMBO:
        case IDC_EXTENDEDUI_COMBO:
        case IDC_AUTOHSCROLL_COMBO:
        case IDC_UPPERCASE_COMBO:
        case IDC_LOWERCASE_COMBO:
        case IDC_NOINTEGRAL_COMBO:
            if (NotifyCode == CBN_SELCHANGE)
            {
                return TRUE;
            }
            else if (NotifyCode == CBN_EDITCHANGE)
            {
                return TRUE;
            }
            return FALSE;

        default:
            return FALSE;
    }
}

static VOID
OnSize(PCOMBOBOX_PAGE_DATA PageData, int cx, int cy)
{
    HDWP hdwp;
    int margin_x = 20;
    int margin_y = 15;
    int gap = 15;
    int label_h = 15;
    int combo_drop_h = 140; /* Dropdown list height when popped open */
    int col_width;
    int col1_x, col2_x, col3_x;
    int row_h = 42; /* Height per label + combobox row */
    int simple_h;

    if (!PageData || !PageData->hSimpleCombo)
        return;

    /* Calculate equal 3-column widths */
    col_width = (cx - (margin_x * 2) - (gap * 2)) / 3;
    if (col_width < 100) col_width = 100;

    col1_x = margin_x;
    col2_x = col1_x + col_width + gap;
    col3_x = col2_x + col_width + gap;

    /* Simple combo list stretches to fit available vertical space */
    simple_h = cy - margin_y - label_h - 5 - margin_y;
    if (simple_h < 120) simple_h = 120;

    hdwp = BeginDeferWindowPos(20);

#define MOVE_CTRL(hwnd, x, y, w, h) \
    if (hwnd) hdwp = DeferWindowPos(hdwp, hwnd, NULL, x, y, w, h, SWP_NOZORDER)

    /* --- COLUMN 1: Simple Combo (Expanded List) --- */
    MOVE_CTRL(PageData->hLblSimple,   col1_x, margin_y, col_width, label_h);
    MOVE_CTRL(PageData->hSimpleCombo, col1_x, margin_y + label_h + 3, col_width, simple_h);

    /* --- COLUMN 2: Standard ComboBox Styles --- */
    /* Row 1: Dropdown */
    MOVE_CTRL(PageData->hLblDropdown,       col2_x, margin_y + (row_h * 0), col_width, label_h);
    MOVE_CTRL(PageData->hDropdownCombo,     col2_x, margin_y + (row_h * 0) + label_h, col_width, combo_drop_h);

    /* Row 2: Dropdown List */
    MOVE_CTRL(PageData->hLblDropdownList,   col2_x, margin_y + (row_h * 1), col_width, label_h);
    MOVE_CTRL(PageData->hDropdownListCombo, col2_x, margin_y + (row_h * 1) + label_h, col_width, combo_drop_h);

    /* Row 3: Disabled */
    MOVE_CTRL(PageData->hLblDisabled,       col2_x, margin_y + (row_h * 2), col_width, label_h);
    MOVE_CTRL(PageData->hDisabledCombo,     col2_x, margin_y + (row_h * 2) + label_h, col_width, combo_drop_h);

    /* Row 4: Sorted List */
    MOVE_CTRL(PageData->hLblSorted,         col2_x, margin_y + (row_h * 3), col_width, label_h);
    MOVE_CTRL(PageData->hSortedCombo,       col2_x, margin_y + (row_h * 3) + label_h, col_width, combo_drop_h);

    /* Row 5: Extended UI */
    MOVE_CTRL(PageData->hLblExtendedUI,     col2_x, margin_y + (row_h * 4), col_width, label_h);
    MOVE_CTRL(PageData->hExtendedUICombo,   col2_x, margin_y + (row_h * 4) + label_h, col_width, combo_drop_h);

    /* Row 6: Auto H-Scroll */
    MOVE_CTRL(PageData->hLblAutoHScroll,    col2_x, margin_y + (row_h * 5), col_width, label_h);
    MOVE_CTRL(PageData->hAutoHScrollCombo,  col2_x, margin_y + (row_h * 5) + label_h, col_width, combo_drop_h);

    /* --- COLUMN 3: Formatting & Behavior --- */
    /* Row 1: Uppercase */
    MOVE_CTRL(PageData->hLblUppercase,      col3_x, margin_y + (row_h * 0), col_width, label_h);
    MOVE_CTRL(PageData->hUppercaseCombo,    col3_x, margin_y + (row_h * 0) + label_h, col_width, combo_drop_h);

    /* Row 2: Lowercase */
    MOVE_CTRL(PageData->hLblLowercase,      col3_x, margin_y + (row_h * 1), col_width, label_h);
    MOVE_CTRL(PageData->hLowercaseCombo,    col3_x, margin_y + (row_h * 1) + label_h, col_width, combo_drop_h);

    /* Row 3: No Integral Height */
    MOVE_CTRL(PageData->hLblNoIntegral,     col3_x, margin_y + (row_h * 2), col_width, label_h);
    MOVE_CTRL(PageData->hNoIntegralCombo,   col3_x, margin_y + (row_h * 2) + label_h, col_width, 65);

#undef MOVE_CTRL

    EndDeferWindowPos(hdwp);
}

LRESULT
ComboPageProc(PPAGE_HOST PageHost, UINT msg, WPARAM wParam, LPARAM lParam)
{
    PCOMBOBOX_PAGE_DATA PageData;
    if (msg == WM_CREATE)
    {
        PageData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(COMBOBOX_PAGE_DATA));
        PageHost->UserData = PageData;
        return InitPage(PageHost->Wnd, PageData);
    }
    else
    {
        PageData = (PCOMBOBOX_PAGE_DATA)PageHost->UserData;
    }

    switch (msg)
    {
        case WM_COMMAND:
            if (LOWORD(wParam) < IDC_SIMPLE_COMBO || LOWORD(wParam) > IDC_NOINTEGRAL_COMBO)
                return FALSE;
            return OnCommand(PageHost, PageData, LOWORD(wParam), HIWORD(wParam));

        case WM_SIZE:
            OnSize(PageData, LOWORD(lParam), HIWORD(lParam));
            return TRUE;

        default:
            return FALSE;
    }
}
