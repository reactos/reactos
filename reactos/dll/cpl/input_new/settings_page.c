/*
 * PROJECT:         input.dll
 * FILE:            dll/cpl/input/settings_page.c
 * PURPOSE:         input.dll
 * PROGRAMMER:      Dmitry Chapyshev (dmitry@reactos.org)
 */

#include "input.h"
#include "layout_list.h"
#include "locale_list.h"


static HICON
CreateLayoutIcon(LPWSTR szLayout)
{
    HDC hdc, hdcsrc;
    HBITMAP hBitmap, hBmpNew, hBmpOld;
    RECT rect;
    HFONT hFont = NULL;
    ICONINFO IconInfo;
    HICON hIcon = NULL;

    hdcsrc = GetDC(NULL);
    hdc = CreateCompatibleDC(hdcsrc);
    hBitmap = CreateCompatibleBitmap(hdcsrc,
                                     GetSystemMetrics(SM_CXSMICON),
                                     GetSystemMetrics(SM_CYSMICON));
    ReleaseDC(NULL, hdcsrc);

    if (hdc && hBitmap)
    {
        hBmpNew = CreateBitmap(GetSystemMetrics(SM_CXSMICON),
                               GetSystemMetrics(SM_CYSMICON),
                               1, 1, NULL);
        if (hBmpNew)
        {
            hBmpOld = SelectObject(hdc, hBitmap);
            rect.right = GetSystemMetrics(SM_CXSMICON);
            rect.left = 0;
            rect.bottom = GetSystemMetrics(SM_CYSMICON);
            rect.top = 0;

            SetBkColor(hdc, GetSysColor(COLOR_HIGHLIGHT));
            SetTextColor(hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));

            ExtTextOut(hdc, rect.left, rect.top, ETO_OPAQUE, &rect, L"", 0, NULL);

            hFont = CreateFont(-11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
                               OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                               DEFAULT_QUALITY, FF_DONTCARE, L"Tahoma");

            SelectObject(hdc, hFont);
            DrawText(hdc, szLayout, 2, &rect, DT_SINGLELINE|DT_CENTER|DT_VCENTER);
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
AddToLayoutListView(HWND hwndList,
                    WCHAR *pszLanguageName,
                    WCHAR *pszLayoutName,
                    WCHAR *pszIndicatorTitle)
{
    INT ItemIndex = -1;
    INT ImageIndex = -1;
    LV_ITEM item;
    HIMAGELIST hImageList;

    hImageList = ListView_GetImageList(hwndList, LVSIL_SMALL);
    if (hImageList != NULL)
    {
        HICON hLayoutIcon;

        hLayoutIcon = CreateLayoutIcon(pszIndicatorTitle);
        if (hLayoutIcon != NULL)
        {
            ImageIndex = ImageList_AddIcon(hImageList, hLayoutIcon);
            DestroyIcon(hLayoutIcon);
        }
    }

    memset(&item, 0, sizeof(LV_ITEM));

    item.mask    = LVIF_TEXT | LVIF_PARAM | LVIF_STATE | LVIF_IMAGE;
    item.pszText = pszLanguageName;
    item.iItem   = -1;
    item.lParam  = (LPARAM)NULL;
    item.iImage  = ImageIndex;

    ItemIndex = ListView_InsertItem(hwndList, &item);

    ListView_SetItemText(hwndList, ItemIndex, 1, pszLayoutName);
}


static VOID
OnInitSettingsPage(HWND hwndDlg)
{
    WCHAR szBuffer[MAX_STR_LEN];
    LV_COLUMN column;
    HWND hLayoutList;
    HIMAGELIST hLayoutImageList;
    INT iLayoutCount;
    HKL *pActiveLayoutList;

    hLayoutList = GetDlgItem(hwndDlg, IDC_KEYLAYOUT_LIST);

    ListView_SetExtendedListViewStyle(hLayoutList, LVS_EX_FULLROWSELECT);

    memset(&column, 0, sizeof(LV_COLUMN));

    column.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

    LoadString(hApplet, IDS_LANGUAGE, szBuffer, ARRAYSIZE(szBuffer));
    column.fmt      = LVCFMT_LEFT;
    column.iSubItem = 0;
    column.pszText  = szBuffer;
    column.cx       = 175;
    ListView_InsertColumn(hLayoutList, 0, &column);

    LoadString(hApplet, IDS_LAYOUT, szBuffer, ARRAYSIZE(szBuffer));
    column.fmt      = LVCFMT_RIGHT;
    column.cx       = 155;
    column.iSubItem = 1;
    column.pszText  = szBuffer;
    ListView_InsertColumn(hLayoutList, 1, &column);

    hLayoutImageList = ImageList_Create(GetSystemMetrics(SM_CXSMICON),
                                        GetSystemMetrics(SM_CYSMICON),
                                        ILC_COLOR8 | ILC_MASK, 0, 0);
    if (hLayoutImageList != NULL)
    {
        ListView_SetImageList(hLayoutList, hLayoutImageList, LVSIL_SMALL);
    }

    LayoutList_Create();
    LocaleList_Create();

    iLayoutCount = GetKeyboardLayoutList(0, NULL);
    pActiveLayoutList = (HKL*) malloc(iLayoutCount * sizeof(HKL));
    if (pActiveLayoutList != NULL)
    {
        if (GetKeyboardLayoutList(iLayoutCount, pActiveLayoutList) > 0)
        {
            INT iIndex;

            for (iIndex = 0; iIndex < iLayoutCount; iIndex++)
            {
                LOCALE_LIST_NODE *CurrentLocale;

                for (CurrentLocale = LocaleList_Get();
                     CurrentLocale != NULL;
                     CurrentLocale = CurrentLocale->pNext)
                {
                    if (LOWORD(CurrentLocale->dwId) == LOWORD(pActiveLayoutList[iIndex]))
                    {
                        AddToLayoutListView(hLayoutList,
                                            CurrentLocale->pszName,
                                            LayoutList_GetNameByHkl(pActiveLayoutList[iIndex]),
                                            CurrentLocale->pszIndicator);
                    }
                }
            }
        }

        free(pActiveLayoutList);
    }
}


static VOID
OnDestroySettingsPage(HWND hwndDlg)
{
    HIMAGELIST hImageList;

    LayoutList_Destroy();
    LocaleList_Destroy();

    hImageList = ListView_GetImageList(GetDlgItem(hwndDlg, IDC_KEYLAYOUT_LIST),
                                       LVSIL_SMALL);
    if (hImageList != NULL)
    {
        ImageList_Destroy(hImageList);
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
        {
            
        }
        break;
    }

    return FALSE;
}
