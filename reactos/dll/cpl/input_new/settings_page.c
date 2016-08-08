/*
 * PROJECT:         input.dll
 * FILE:            dll/cpl/input/settings_page.c
 * PURPOSE:         input.dll
 * PROGRAMMER:      Dmitry Chapyshev (dmitry@reactos.org)
 */

#include "input.h"
#include "layout_list.h"
#include "locale_list.h"
#include "input_list.h"


static HICON
CreateLayoutIcon(LPWSTR szLayout)
{
    HDC hdc;
    HDC hdcsrc;
    HBITMAP hBitmap;
    HICON hIcon = NULL;

    hdcsrc = GetDC(NULL);
    hdc = CreateCompatibleDC(hdcsrc);
    hBitmap = CreateCompatibleBitmap(hdcsrc,
                                     GetSystemMetrics(SM_CXSMICON),
                                     GetSystemMetrics(SM_CYSMICON));
    ReleaseDC(NULL, hdcsrc);

    if (hdc && hBitmap)
    {
        HBITMAP hBmpNew;

        hBmpNew = CreateBitmap(GetSystemMetrics(SM_CXSMICON),
                               GetSystemMetrics(SM_CYSMICON),
                               1, 1, NULL);
        if (hBmpNew)
        {
            ICONINFO IconInfo;
            HBITMAP hBmpOld;
            HFONT hFont;
            RECT rect;

            hBmpOld = SelectObject(hdc, hBitmap);
            rect.right = GetSystemMetrics(SM_CXSMICON);
            rect.left = 0;
            rect.bottom = GetSystemMetrics(SM_CYSMICON);
            rect.top = 0;

            SetBkColor(hdc, GetSysColor(COLOR_HIGHLIGHT));
            SetTextColor(hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));

            ExtTextOut(hdc, rect.left, rect.top, ETO_OPAQUE, &rect, L"", 0, NULL);

            hFont = CreateFontW(-11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
                                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                DEFAULT_QUALITY, FF_DONTCARE, L"Tahoma");

            SelectObject(hdc, hFont);
            DrawTextW(hdc, szLayout, 2, &rect, DT_SINGLELINE|DT_CENTER|DT_VCENTER);
            SelectObject(hdc, hBmpNew);

            PatBlt(hdc, 0, 0,
                   GetSystemMetrics(SM_CXSMICON),
                   GetSystemMetrics(SM_CYSMICON),
                   BLACKNESS);

            SelectObject(hdc, hBmpOld);

            IconInfo.hbmColor = hBitmap;
            IconInfo.hbmMask = hBmpNew;
            IconInfo.fIcon = TRUE;

            hIcon = CreateIconIndirect(&IconInfo);

            DeleteObject(hBmpNew);
            DeleteObject(hBmpOld);
            DeleteObject(hFont);
        }
    }

    DeleteDC(hdc);
    DeleteObject(hBitmap);

    return hIcon;
}


static VOID
AddToInputListView(HWND hwndList, INPUT_LIST_NODE *pInputNode)
{
    INT ItemIndex = -1;
    INT ImageIndex = -1;
    LV_ITEM item;
    HIMAGELIST hImageList;

    hImageList = ListView_GetImageList(hwndList, LVSIL_SMALL);

    if (hImageList != NULL)
    {
        HICON hLayoutIcon;

        hLayoutIcon = CreateLayoutIcon(pInputNode->pLocale->pszIndicator);

        if (hLayoutIcon != NULL)
        {
            ImageIndex = ImageList_AddIcon(hImageList, hLayoutIcon);
            DestroyIcon(hLayoutIcon);
        }
    }

    memset(&item, 0, sizeof(LV_ITEM));

    item.mask    = LVIF_TEXT | LVIF_PARAM | LVIF_STATE | LVIF_IMAGE;
    item.pszText = pInputNode->pLocale->pszName;
    item.iItem   = -1;
    item.lParam  = (LPARAM)pInputNode;
    item.iImage  = ImageIndex;

    ItemIndex = ListView_InsertItem(hwndList, &item);

    ListView_SetItemText(hwndList, ItemIndex, 1, pInputNode->pLayout->pszName);
}


static VOID
UpdateInputListView(HWND hwndList)
{
    INPUT_LIST_NODE *pCurrentInputNode;
    HIMAGELIST hImageList;

    hImageList = ListView_GetImageList(hwndList, LVSIL_SMALL);
    if (hImageList != NULL)
    {
        ImageList_RemoveAll(hImageList);
    }

    ListView_DeleteAllItems(hwndList);

    for (pCurrentInputNode = InputList_Get();
         pCurrentInputNode != NULL;
         pCurrentInputNode = pCurrentInputNode->pNext)
    {
        AddToInputListView(hwndList, pCurrentInputNode);
    }
}


static VOID
OnInitSettingsPage(HWND hwndDlg)
{
    HWND hwndInputList;

    LayoutList_Create();
    LocaleList_Create();
    InputList_Create();

    hwndInputList = GetDlgItem(hwndDlg, IDC_KEYLAYOUT_LIST);

    if (hwndInputList != NULL)
    {
        INPUT_LIST_NODE *pCurrentInputNode;
        WCHAR szBuffer[MAX_STR_LEN];
        HIMAGELIST hLayoutImageList;
        LV_COLUMN column;

        ListView_SetExtendedListViewStyle(hwndInputList, LVS_EX_FULLROWSELECT);

        memset(&column, 0, sizeof(LV_COLUMN));

        column.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

        LoadStringW(hApplet, IDS_LANGUAGE, szBuffer, ARRAYSIZE(szBuffer));
        column.fmt      = LVCFMT_LEFT;
        column.iSubItem = 0;
        column.pszText  = szBuffer;
        column.cx       = 175;
        ListView_InsertColumn(hwndInputList, 0, &column);

        LoadStringW(hApplet, IDS_LAYOUT, szBuffer, ARRAYSIZE(szBuffer));
        column.fmt      = LVCFMT_RIGHT;
        column.cx       = 155;
        column.iSubItem = 1;
        column.pszText  = szBuffer;
        ListView_InsertColumn(hwndInputList, 1, &column);

        hLayoutImageList = ImageList_Create(GetSystemMetrics(SM_CXSMICON),
                                            GetSystemMetrics(SM_CYSMICON),
                                            ILC_COLOR8 | ILC_MASK, 0, 0);
        if (hLayoutImageList != NULL)
        {
            ListView_SetImageList(hwndInputList, hLayoutImageList, LVSIL_SMALL);
        }

        for (pCurrentInputNode = InputList_Get();
             pCurrentInputNode != NULL;
             pCurrentInputNode = pCurrentInputNode->pNext)
        {
            AddToInputListView(hwndInputList, pCurrentInputNode);
        }
    }
}


static VOID
OnDestroySettingsPage(HWND hwndDlg)
{
    HIMAGELIST hImageList;

    LayoutList_Destroy();
    LocaleList_Destroy();
    InputList_Destroy();

    hImageList = ListView_GetImageList(GetDlgItem(hwndDlg, IDC_KEYLAYOUT_LIST),
                                       LVSIL_SMALL);
    if (hImageList != NULL)
    {
        ImageList_Destroy(hImageList);
    }
}


VOID
OnCommandSettingsPage(HWND hwndDlg, WPARAM wParam)
{
    switch (LOWORD(wParam))
    {
        case IDC_ADD_BUTTON:
        {
            if (DialogBoxW(hApplet,
                           MAKEINTRESOURCEW(IDD_ADD),
                           hwndDlg,
                           AddDialogProc) == IDOK)
            {
                UpdateInputListView(GetDlgItem(hwndDlg, IDC_KEYLAYOUT_LIST));
                PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
            }
        }
        break;

        case IDC_REMOVE_BUTTON:
        {
            PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
        }
        break;

        case IDC_PROP_BUTTON:
        {

        }
        break;

        case IDC_SET_DEFAULT:
        {
            PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
        }
        break;

        case IDC_KEY_SET_BTN:
        {

        }
        break;
    }
}


static VOID
OnNotifySettingsPage(HWND hwndDlg, LPARAM lParam)
{
    if (((LPPSHNOTIFY)lParam)->hdr.code == PSN_APPLY)
    {
        InputList_Process();
    }
}


INT_PTR CALLBACK
SettingsPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            OnInitSettingsPage(hwndDlg);
            break;

        case WM_DESTROY:
            OnDestroySettingsPage(hwndDlg);
            break;

        case WM_COMMAND:
            OnCommandSettingsPage(hwndDlg, wParam);
            break;

        case WM_NOTIFY:
            OnNotifySettingsPage(hwndDlg, lParam);
            break;
    }

    return FALSE;
}
