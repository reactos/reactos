#include "intl.h"

#include <shellapi.h>
#include <shlobj.h>
#include <strsafe.h>

/* Where is the uninstaller of "ReactOS JPN Package"? */
BOOL GetJapaneseUninstallPath(HWND hwnd, LPWSTR pszPath, SIZE_T cchPath)
{
    SHGetSpecialFolderPathW(hwnd, pszPath, CSIDL_PROGRAM_FILES, FALSE);
    StringCchCatW(pszPath, cchPath, L"\\ReactOS JPN Package\\unins000.exe");
    if (GetFileAttributesW(pszPath) != INVALID_FILE_ATTRIBUTES)
        return TRUE;

    SHGetSpecialFolderPathW(hwnd, pszPath, CSIDL_PROGRAM_FILESX86, FALSE);
    StringCchCatW(pszPath, cchPath, L"\\ReactOS JPN Package\\unins000.exe");
    if (GetFileAttributesW(pszPath) != INVALID_FILE_ATTRIBUTES)
        return TRUE;

    pszPath[0] = UNICODE_NULL;
    return FALSE;
}

/* Is there any "ReactOS JPN Package"? */
BOOL HasJapanesePackage(HWND hwnd)
{
    WCHAR szPath[MAX_PATH];
    return GetJapaneseUninstallPath(hwnd, szPath, _countof(szPath));
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
                    if (HasJapanesePackage(hwndDlg))
                    {
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
            if (((LPNMHDR)lParam)->code == (UINT)PSN_APPLY) /* Apply changes */
            {
                /* EAST ASIAN specific */
                switch (PRIMARYLANGID(GetUserDefaultLangID()))
                {
                    case LANG_JAPANESE:
                        if (IsDlgButtonChecked(hwndDlg, IDC_INST_FILES_FOR_ASIAN) == BST_CHECKED)
                        {
                            if (!HasJapanesePackage(hwndDlg))
                            {
                                /* Install now */
                                ShellExecuteW(hwndDlg, NULL, L"rapps.exe", L"/INSTALL jpn-package",
                                              NULL, SW_SHOWNORMAL);
                            }
                        }
                        else
                        {
                            WCHAR szUninstall[MAX_PATH];
                            if (GetJapaneseUninstallPath(hwndDlg, szUninstall, _countof(szUninstall)))
                            {
                                /* Uninstall now */
                                ShellExecuteW(hwndDlg, NULL, szUninstall, L"", NULL, SW_SHOWNORMAL);
                            }
                        }
                        break;

                    case LANG_CHINESE: /* Not supported yet */
                    case LANG_KOREAN: /* Not supported yet */
                    default:
                        break;
                }

                PropSheet_UnChanged(GetParent(hwndDlg), hwndDlg);
            }
            break;
    }
    return FALSE;
}

/* EOF */
