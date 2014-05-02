/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS runonce.exe
 * FILE:            base/system/runonce/runonce.c
 * PROGRAMMER:      Dmitry Chapyshev (dmitry@reactos.org)
 */

#include "runonce.h"

static
DWORD
WINAPI
StartApplication(LPVOID lpDlg)
{
    HWND hList = GetDlgItem((HWND)lpDlg, IDC_COMP_LIST);
    INT Index, Count = SendMessage(hList, LB_GETCOUNT, 0, 0);
    PROCESS_INFORMATION pi;
    STARTUPINFOW si;
    TCHAR szData[MAX_PATH];

    for (Index = 0; Index < Count; Index++)
    {
        SendMessage(hList, LB_GETTEXT, Index, (LPARAM)szData);

        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_SHOW;

        if (!CreateProcess(NULL, szData, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
            continue;

        WaitForSingleObjectEx(pi.hProcess, INFINITE, TRUE);

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    RegDeleteKey(HKEY_LOCAL_MACHINE,
                 _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce\\Setup"));

    PostMessage((HWND)lpDlg, WM_CLOSE, 0, 0);

    return 0;
}

static
VOID
InitDialog(HWND hDlg)
{
    TCHAR szAppPath[MAX_PATH], szData[MAX_PATH];
    DWORD dwIndex, dwSize, dwType, dwData, dwThreadId;
    HKEY hKey;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce\\Setup"),
                     0,
                     KEY_QUERY_VALUE,
                     &hKey) == ERROR_SUCCESS)
    {
        for (dwIndex = 0; ; dwIndex++)
        {
            dwSize = sizeof(szAppPath);
            dwData = sizeof(szData) / sizeof(TCHAR);

            if (RegEnumValue(hKey,
                             dwIndex,
                             szAppPath,
                             &dwSize,
                             NULL,
                             &dwType,
                             (LPBYTE)szData,
                             &dwData) == ERROR_SUCCESS)
            {
                if (dwType != REG_SZ) continue;

                SendMessage(GetDlgItem(hDlg, IDC_COMP_LIST), LB_ADDSTRING, 0, (LPARAM)szData);
            }
        }

        RegCloseKey(hKey);
    }

    CloseHandle(CreateThread(NULL,
                             0,
                             StartApplication,
                             (LPVOID)hDlg,
                             0,
                             &dwThreadId));
}

static
INT_PTR
CALLBACK
RunOnceDlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
        case WM_INITDIALOG:
            InitDialog(hDlg);
            break;

        case WM_CLOSE:
            EndDialog(hDlg, 0);
            break;
    }

    return 0;
}

INT
WINAPI
_tWinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPTSTR lpCmdLine, INT nCmdShow)
{
    LPCTSTR lpCmd = GetCommandLine();
    TCHAR szAppPath[MAX_PATH], szData[MAX_PATH];
    DWORD dwIndex, dwSize, dwType, dwData;
    PROCESS_INFORMATION pi;
    STARTUPINFOW si;
    BOOL bRunApps = FALSE;
    HKEY hKey;

    while (*lpCmd)
    {
        while (*lpCmd && *lpCmd != _T('/') && *lpCmd != _T('-')) lpCmd++;
        if (!*lpCmd) break;
        if (*++lpCmd == _T('r')) bRunApps = TRUE;
        lpCmd++;
    }

    if (bRunApps)
    {
        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                         _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce"),
                         0,
                         KEY_QUERY_VALUE,
                         &hKey) == ERROR_SUCCESS)
        {
            for (dwIndex = 0; ; dwIndex++)
            {
                dwSize = sizeof(szAppPath);
                dwData = sizeof(szData) / sizeof(TCHAR);

                if (RegEnumValue(hKey,
                                 dwIndex,
                                 szAppPath,
                                 &dwSize,
                                 NULL,
                                 &dwType,
                                 (LPBYTE)szData,
                                 &dwData) == ERROR_SUCCESS)
                {
                    RegDeleteValue(hKey, szAppPath);

                    if (dwType != REG_SZ) continue;

                    ZeroMemory(&si, sizeof(si));
                    si.cb = sizeof(si);
                    si.dwFlags = STARTF_USESHOWWINDOW;
                    si.wShowWindow = SW_SHOW;

                    if (!CreateProcess(NULL, szData, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
                        continue;

                    WaitForSingleObjectEx(pi.hProcess, INFINITE, TRUE);

                    CloseHandle(pi.hProcess);
                    CloseHandle(pi.hThread);
                }
            }

            RegCloseKey(hKey);
        }

        return 1;
    }

    DialogBox(hInst, MAKEINTRESOURCE(IDD_RUNONCE_DLG), NULL, RunOnceDlgProc);

    return 0;
}
