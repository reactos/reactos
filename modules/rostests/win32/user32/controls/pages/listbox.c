/*
 * PROJECT:     ReactOS Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     User32 Control Gallery
 * COPYRIGHT:   Copyright 2026 Mohammad Amin Mollazadeh <madamin@pm.me>
 */

#include "../controls.h"

typedef struct _LISTBOX_PAGE_DATA
{
    HWND hLblSingle;
    HWND hSingle;

    HWND hLblMultiple;
    HWND hMultiple;

    HWND hLblExtended;
    HWND hExtended;
} LISTBOX_PAGE_DATA, *PLISTBOX_PAGE_DATA;

static PCWSTR Items[] = {
    L"Cherry",
    L"Date",
    L"Grape",
    L"Apple",
    L"Banana",
    L"Fig",
    L"Elderberry",
};

static VOID
PopulateListBox(HWND hCombo, BOOL bSelectFirst)
{
    for (int i = 0; i < _countof(Items); i++)
    {
        SendMessageW(hCombo, LB_ADDSTRING, 0, (LPARAM)Items[i]);
    }

    if (bSelectFirst)
        SendMessageW(hCombo, LB_SETCURSEL, 0, 0);
}


LRESULT
InitPage(HWND Parent, PLISTBOX_PAGE_DATA PageData)
{
    PageData->hLblSingle =
        CreateChild(0, L"STATIC", L"Single ListBox", SS_LEFT, 20, 10, 200, 15, Parent);
    PageData->hSingle =
        CreateChild(0, L"LISTBOX", NULL, LBS_STANDARD, 20, 30, 200, 100, Parent);
    PopulateListBox(PageData->hSingle, TRUE);

    PageData->hLblMultiple =
        CreateChild(0, L"STATIC", L"Multiple Selection ListBox", SS_LEFT, 20, 150, 200, 15, Parent);
    PageData->hMultiple =
        CreateChild(0, L"LISTBOX", NULL, LBS_STANDARD | LBS_MULTIPLESEL, 20, 170, 200, 100, Parent);
    PopulateListBox(PageData->hMultiple, FALSE);

    PageData->hLblExtended =
        CreateChild(0, L"STATIC", L"Extended ListBox", SS_LEFT, 240, 10, 200, 15, Parent);
    PageData->hExtended =
        CreateChild(0, L"LISTBOX", NULL, LBS_STANDARD | LBS_EXTENDEDSEL, 240, 30, 200, 100, Parent);
    PopulateListBox(PageData->hExtended, FALSE);

    return TRUE;
}

LRESULT
ListBoxPageProc(PPAGE_HOST PageHost, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_CREATE:
        {
            PLISTBOX_PAGE_DATA PageData;

            PageData = HeapAlloc(GetProcessHeap(),
                                 HEAP_ZERO_MEMORY,
                                 sizeof(*PageData));

            if (!PageData)
                return FALSE;

            PageHost->UserData = PageData;
            return InitPage(PageHost->Wnd, PageData);
        }

        case WM_DESTROY:
        {
            PLISTBOX_PAGE_DATA PageData = (PLISTBOX_PAGE_DATA)PageHost->UserData;
            HeapFree(GetProcessHeap(), 0, PageData);
            return TRUE;
        }
    }

    return FALSE;
}
