#include "intl.h"

#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <strsafe.h>

/* What is the uninstallation command line of "ReactOS JPN Package"? */
BOOL GetJapaneseUninstallCmdLine(HWND hwnd, LPWSTR pszCmdLine, SIZE_T cchCmdLine)
{
    HKEY hKey;
    LONG error;
    DWORD dwSize;

    pszCmdLine[0] = UNICODE_NULL;

    error = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                          L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\"
                          L"{80F03D6E-0549-4202-BE81-FF583F56A7A8}_is1",
                          0,
                          KEY_READ,
                          &hKey);
    if (error != ERROR_SUCCESS)
        return FALSE;

    dwSize = cchCmdLine * sizeof(WCHAR);
    error = RegQueryValueExW(hKey, L"UninstallString", NULL, NULL, (LPBYTE)pszCmdLine, &dwSize);
    if (error != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return FALSE;
    }

    pszCmdLine[cchCmdLine - 1] = UNICODE_NULL;
    RegCloseKey(hKey);
    return TRUE;
}

/* Is there any installed "ReactOS JPN Package"? */
BOOL HasJapanesePackage(HWND hwnd)
{
    WCHAR szPath[MAX_PATH];
    return GetJapaneseUninstallCmdLine(hwnd, szPath, _countof(szPath));
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
                            if (GetJapaneseUninstallCmdLine(hwndDlg, szUninstall, _countof(szUninstall)))
                            {
                                /* Go to arguments of command line */
                                PWCHAR pchArgs = PathGetArgsW(szUninstall);
                                if (pchArgs && *pchArgs)
                                {
                                    --pchArgs;
                                    /* pchArgs pointer is inside szUninstall,
                                     * so we have to split both strings */
                                    *pchArgs = UNICODE_NULL;
                                    ++pchArgs;
                                }
                                PathUnquoteSpacesW(szUninstall);

                                /* Uninstall now */
                                ShellExecuteW(hwndDlg, NULL, szUninstall, pchArgs, NULL, SW_SHOWNORMAL);
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
