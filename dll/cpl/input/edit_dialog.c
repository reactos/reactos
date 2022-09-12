/*
* PROJECT:         input.dll
* FILE:            dll/cpl/input/edit_dialog.c
* PURPOSE:         input.dll
* PROGRAMMER:      Dmitry Chapyshev (dmitry@reactos.org)
*/

#include "input.h"
#include "locale_list.h"
#include "input_list.h"


INT_PTR CALLBACK
EditDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            LAYOUT_LIST_NODE *pCurrentLayout;
            INPUT_LIST_NODE *pInput;
            HWND hwndList;

            pInput = (INPUT_LIST_NODE*) lParam;

            if (pInput == NULL)
                return TRUE;

            SetWindowLongPtrW(hwndDlg, GWLP_USERDATA, (LONG_PTR) pInput);

            SetWindowTextW(GetDlgItem(hwndDlg, IDC_INPUT_LANG_STR), pInput->pLocale->pszName);

            hwndList = GetDlgItem(hwndDlg, IDC_KB_LAYOUT_IME_COMBO);

            for (pCurrentLayout = LayoutList_GetFirst();
                 pCurrentLayout != NULL;
                 pCurrentLayout = pCurrentLayout->pNext)
            {
                INT iItemIndex;

                iItemIndex = ComboBox_AddString(hwndList, pCurrentLayout->pszName);
                ComboBox_SetItemData(hwndList, iItemIndex, pCurrentLayout);
            }

            ComboBox_SelectString(hwndList, 0, pInput->pLayout->pszName);
        }
        break;

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDOK:
                {
                    INPUT_LIST_NODE *pInput;
                    HWND hwndList;

                    hwndList = GetDlgItem(hwndDlg, IDC_KB_LAYOUT_IME_COMBO);

                    pInput = (INPUT_LIST_NODE*) GetWindowLongPtrW(hwndDlg, GWLP_USERDATA);

                    if (pInput != NULL)
                    {
                        LAYOUT_LIST_NODE *pNewLayout;

                        pNewLayout = (LAYOUT_LIST_NODE*)ComboBox_GetItemData(hwndList,
                                                                             ComboBox_GetCurSel(hwndList));
                        if (pNewLayout != NULL)
                        {
                            pInput->pLayout = pNewLayout;
                            pInput->wFlags |= INPUT_LIST_NODE_FLAG_EDITED;
                        }
                    }

                    EndDialog(hwndDlg, LOWORD(wParam));
                }
                break;

                case IDCANCEL:
                {
                    EndDialog(hwndDlg, LOWORD(wParam));
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}
