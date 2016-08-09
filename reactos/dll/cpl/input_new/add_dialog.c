/*
* PROJECT:         input.dll
* FILE:            dll/cpl/input/add_dialog.c
* PURPOSE:         input.dll
* PROGRAMMER:      Dmitry Chapyshev (dmitry@reactos.org)
*/

#include "input.h"
#include "locale_list.h"
#include "layout_list.h"
#include "input_list.h"


static DWORD
GetDefaultLayoutForLocale(DWORD dwLocaleId)
{
    DWORD dwResult = 0;
    HINF hIntlInf;

    hIntlInf = SetupOpenInfFileW(L"intl.inf", NULL, INF_STYLE_WIN4, NULL);
    if (hIntlInf != INVALID_HANDLE_VALUE)
    {
        WCHAR szLangID[MAX_STR_LEN];
        INFCONTEXT InfContext;

        StringCchPrintfW(szLangID, ARRAYSIZE(szLangID), L"%08X", dwLocaleId);

        if (SetupFindFirstLineW(hIntlInf, L"Locales", szLangID, &InfContext))
        {
            if (SetupGetFieldCount(&InfContext) >= 5)
            {
                WCHAR szField[MAX_STR_LEN];

                if (SetupGetStringFieldW(&InfContext, 5, szField, ARRAYSIZE(szField), NULL))
                {
                    if (wcslen(szField) == 13) // like 0409:00000409 (13 chars)
                    {
                        WCHAR *pszSeparator = L":";
                        WCHAR *pszToken;

                        pszToken = wcstok(szField, pszSeparator);
                        if (pszToken != NULL)
                            pszToken = wcstok(NULL, pszSeparator);

                        if (pszToken != NULL)
                        {
                            dwResult = DWORDfromString(pszToken);
                        }
                    }
                }
            }
        }

        SetupCloseInfFile(hIntlInf);
    }

    return dwResult;
}


static VOID
OnInitAddDialog(HWND hwndDlg)
{
    HWND hwndLocaleCombo = GetDlgItem(hwndDlg, IDC_INPUT_LANG_COMBO);
    HWND hwndLayoutCombo = GetDlgItem(hwndDlg, IDC_KEYBOARD_LO_COMBO);
    LOCALE_LIST_NODE *pCurrentLocale;
    LAYOUT_LIST_NODE *pCurrentLayout;
    DWORD dwDefaultLocaleId;
    DWORD dwDefaultLayoutId;
    INT iItemIndex;

    dwDefaultLocaleId = GetSystemDefaultLCID();

    for (pCurrentLocale = LocaleList_GetFirst();
         pCurrentLocale != NULL;
         pCurrentLocale = pCurrentLocale->pNext)
    {
        iItemIndex = ComboBox_AddString(hwndLocaleCombo, pCurrentLocale->pszName);
        ComboBox_SetItemData(hwndLocaleCombo, iItemIndex, pCurrentLocale);

        if (pCurrentLocale->dwId == dwDefaultLocaleId)
        {
            ComboBox_SetCurSel(hwndLocaleCombo, iItemIndex);
        }
    }

    dwDefaultLayoutId = GetDefaultLayoutForLocale(dwDefaultLocaleId);

    for (pCurrentLayout = LayoutList_GetFirst();
         pCurrentLayout != NULL;
         pCurrentLayout = pCurrentLayout->pNext)
    {
        iItemIndex = ComboBox_AddString(hwndLayoutCombo, pCurrentLayout->pszName);
        ComboBox_SetItemData(hwndLayoutCombo, iItemIndex, pCurrentLayout);

        if (pCurrentLayout->dwId == dwDefaultLayoutId)
        {
            ComboBox_SetCurSel(hwndLayoutCombo, iItemIndex);
        }
    }

    if (dwDefaultLayoutId == 0)
        ComboBox_SetCurSel(hwndLayoutCombo, 0);
}


static VOID
OnCommandAddDialog(HWND hwndDlg, WPARAM wParam)
{
    switch (LOWORD(wParam))
    {
        case IDOK:
        {
            HWND hwndLocaleCombo = GetDlgItem(hwndDlg, IDC_INPUT_LANG_COMBO);
            HWND hwndLayoutCombo = GetDlgItem(hwndDlg, IDC_KEYBOARD_LO_COMBO);
            LOCALE_LIST_NODE *pCurrentLocale;
            LAYOUT_LIST_NODE *pCurrentLayout;

            pCurrentLocale = (LOCALE_LIST_NODE*)ComboBox_GetItemData(hwndLocaleCombo,
                                                                     ComboBox_GetCurSel(hwndLocaleCombo));
            pCurrentLayout = (LAYOUT_LIST_NODE*)ComboBox_GetItemData(hwndLayoutCombo,
                                                                     ComboBox_GetCurSel(hwndLayoutCombo));

            InputList_Add(pCurrentLocale, pCurrentLayout);

            EndDialog(hwndDlg, LOWORD(wParam));
        }
        break;

        case IDCANCEL:
        {
            EndDialog(hwndDlg, LOWORD(wParam));
        }
        break;

        case IDC_INPUT_LANG_COMBO:
        {
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                HWND hwndLocaleCombo = GetDlgItem(hwndDlg, IDC_INPUT_LANG_COMBO);
                HWND hwndLayoutCombo = GetDlgItem(hwndDlg, IDC_KEYBOARD_LO_COMBO);
                LOCALE_LIST_NODE *pCurrentLocale;

                pCurrentLocale = (LOCALE_LIST_NODE*)ComboBox_GetItemData(hwndLocaleCombo,
                                                                         ComboBox_GetCurSel(hwndLocaleCombo));
                if (pCurrentLocale != NULL)
                {
                    DWORD dwLayoutId;
                    INT iIndex;
                    INT iCount;

                    dwLayoutId = GetDefaultLayoutForLocale(pCurrentLocale->dwId);

                    iCount = ComboBox_GetCount(hwndLayoutCombo);

                    for (iIndex = 0; iIndex < iCount; iIndex++)
                    {
                        LAYOUT_LIST_NODE *pCurrentLayout;

                        pCurrentLayout = (LAYOUT_LIST_NODE*)ComboBox_GetItemData(hwndLayoutCombo, iIndex);

                        if (pCurrentLayout != NULL && pCurrentLayout->dwId == dwLayoutId)
                        {
                            ComboBox_SetCurSel(hwndLayoutCombo, iIndex);
                            break;
                        }
                    }
                }
            }
        }
        break;
    }
}


INT_PTR CALLBACK
AddDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            OnInitAddDialog(hwndDlg);
            break;

        case WM_COMMAND:
            OnCommandAddDialog(hwndDlg, wParam);
            break;

        case WM_DESTROY:
            break;
    }

    return FALSE;
}
