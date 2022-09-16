#include "intl.h"

#include <shellapi.h>

/* Is there any Japanese input method? */
BOOL HasJapaneseIME(VOID)
{
    WCHAR szImePath[MAX_PATH];
    GetSystemDirectoryW(szImePath, _countof(szImePath));
    lstrcatW(szImePath, L"\\mzimeja.ime");
    return GetFileAttributesW(szImePath) != INVALID_FILE_ATTRIBUTES;
}

/* Property page dialog callback */
INT_PTR CALLBACK
LanguagesPageProc(HWND hwndDlg,
                  UINT uMsg,
                  WPARAM wParam,
                  LPARAM lParam)
{
    SHELLEXECUTEINFOW shInputDll;
    PGLOBALDATA pGlobalData;

    pGlobalData = (PGLOBALDATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            pGlobalData = (PGLOBALDATA)((LPPROPSHEETPAGE)lParam)->lParam;
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pGlobalData);

            if (!pGlobalData->bIsUserAdmin)
            {
                EnableWindow(GetDlgItem(hwndDlg, IDC_INST_FILES_FOR_RTOL_LANG), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_INST_FILES_FOR_ASIAN), FALSE);
            }

            /* EAST ASIAN specific */
            switch (PRIMARYLANGID(GetUserDefaultLangID()))
            {
                case LANG_JAPANESE:
                    if (HasJapaneseIME())
                    {
                        EnableWindow(GetDlgItem(hwndDlg, IDC_INST_FILES_FOR_ASIAN), FALSE);
                        CheckDlgButton(hwndDlg, IDC_INST_FILES_FOR_ASIAN, BST_CHECKED);
                    }
                    break;

                case LANG_CHINESE: /* Not supported yet */
                case LANG_KOREAN: /* Not supported yet */
                default:
                    EnableWindow(GetDlgItem(hwndDlg, IDC_INST_FILES_FOR_ASIAN), FALSE);
                    break;
            }
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                /* If "detail" button pressed */
                case IDC_DETAIL_BUTTON:
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        memset(&shInputDll, 0x0, sizeof(SHELLEXECUTEINFOW));
                        shInputDll.cbSize = sizeof(shInputDll);
                        shInputDll.hwnd = hwndDlg;
                        shInputDll.lpVerb = L"open";
                        shInputDll.lpFile = L"RunDll32.exe";
                        shInputDll.lpParameters = L"shell32.dll,Control_RunDLL input.dll";
                        if (ShellExecuteExW(&shInputDll) == 0)
                        {
                            PrintErrorMsgBox(IDS_ERROR_INPUT_DLL);
                        }
                    }
                    break;

                case IDC_INST_FILES_FOR_ASIAN:
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;
            }
            break;

        case WM_NOTIFY:
            if (((LPNMHDR)lParam)->code == (UINT)PSN_APPLY)
            {
                /* Apply changes */
                if (IsDlgButtonChecked(hwndDlg, IDC_INST_FILES_FOR_ASIAN) == BST_CHECKED)
                {
                    /* EAST ASIAN specific */
                    switch (PRIMARYLANGID(GetUserDefaultLangID()))
                    {
                        case LANG_JAPANESE:
                            if (HasJapaneseIME())
                            {
                                EnableWindow(GetDlgItem(hwndDlg, IDC_INST_FILES_FOR_ASIAN), FALSE);
                                CheckDlgButton(hwndDlg, IDC_INST_FILES_FOR_ASIAN, BST_CHECKED);
                            }
                            else
                            {
                                ShellExecuteW(hwndDlg, NULL, L"rapps.com", L"/INSTALL mzimeja",
                                              NULL, SW_SHOWNORMAL);
                            }
                            break;

                        case LANG_CHINESE: /* Not supported yet */
                        case LANG_KOREAN: /* Not supported yet */
                        default:
                            break;
                    }
                }

                PropSheet_UnChanged(GetParent(hwndDlg), hwndDlg);
            }
            break;
    }
    return FALSE;
}

/* EOF */
