/*
 * PROJECT:     ReactOS Task Manager
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Processor Affinity.
 * COPYRIGHT:   Copyright 1999-2001 Brian Palmer <brianp@reactos.org>
 *              Copyright 2005 Klemens Friedl <frik85@reactos.at>
 */

#include "precomp.h"

HANDLE        hProcessAffinityHandle;

static const DWORD dwCpuTable[] = {
    IDC_CPU0,   IDC_CPU1,   IDC_CPU2,   IDC_CPU3,
    IDC_CPU4,   IDC_CPU5,   IDC_CPU6,   IDC_CPU7,
    IDC_CPU8,   IDC_CPU9,   IDC_CPU10,  IDC_CPU11,
    IDC_CPU12,  IDC_CPU13,  IDC_CPU14,  IDC_CPU15,
    IDC_CPU16,  IDC_CPU17,  IDC_CPU18,  IDC_CPU19,
    IDC_CPU20,  IDC_CPU21,  IDC_CPU22,  IDC_CPU23,
    IDC_CPU24,  IDC_CPU25,  IDC_CPU26,  IDC_CPU27,
    IDC_CPU28,  IDC_CPU29,  IDC_CPU30,  IDC_CPU31,
};

static INT_PTR CALLBACK AffinityDialogWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

void ProcessPage_OnSetAffinity(void)
{
    DWORD    dwProcessId;
    WCHAR    strErrorText[260];
    WCHAR    szTitle[256];

    dwProcessId = GetSelectedProcessId();

    if (dwProcessId == 0)
        return;

    hProcessAffinityHandle = OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_SET_INFORMATION, FALSE, dwProcessId);
    if (!hProcessAffinityHandle) {
        GetLastErrorText(strErrorText, _countof(strErrorText));
        LoadStringW(hInst, IDS_MSG_ACCESSPROCESSAFF, szTitle, _countof(szTitle));
        MessageBoxW(hMainWnd, strErrorText, szTitle, MB_OK|MB_ICONSTOP);
        return;
    }
    DialogBoxW(hInst, MAKEINTRESOURCEW(IDD_AFFINITY_DIALOG), hMainWnd, AffinityDialogWndProc);
    if (hProcessAffinityHandle)    {
        CloseHandle(hProcessAffinityHandle);
        hProcessAffinityHandle = NULL;
    }
}

INT_PTR CALLBACK
AffinityDialogWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    DWORD_PTR  dwProcessAffinityMask = 0;
    DWORD_PTR  dwSystemAffinityMask = 0;
    WCHAR  strErrorText[260];
    WCHAR  szTitle[256];
    BYTE   nCpu;

    switch (message) {
    case WM_INITDIALOG:

        /*
         * Get the current affinity mask for the process and
         * the number of CPUs present in the system
         */
        if (!GetProcessAffinityMask(hProcessAffinityHandle, &dwProcessAffinityMask, &dwSystemAffinityMask))    {
            GetLastErrorText(strErrorText, _countof(strErrorText));
            EndDialog(hDlg, 0);
            LoadStringW(hInst, IDS_MSG_ACCESSPROCESSAFF, szTitle, _countof(szTitle));
            MessageBoxW(hMainWnd, strErrorText, szTitle, MB_OK|MB_ICONSTOP);
        }

        for (nCpu = 0; nCpu < _countof(dwCpuTable); nCpu++) {
            /*
             * Enable a checkbox for each processor present in the system
             */
            if (dwSystemAffinityMask & ((ULONG_PTR)1 << nCpu))
                EnableWindow(GetDlgItem(hDlg, dwCpuTable[nCpu]), TRUE);
            /*
             * Check each checkbox that the current process
             * has affinity with
             */
            if (dwProcessAffinityMask & ((ULONG_PTR)1 << nCpu))
                CheckDlgButton(hDlg, dwCpuTable[nCpu], BST_CHECKED);
        }

        return TRUE;

    case WM_COMMAND:

        /*
         * If the user has cancelled the dialog box
         * then just close it
         */
        if (LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, LOWORD(wParam));
            return TRUE;
        }

        /*
         * The user has clicked OK -- so now we have
         * to adjust the process affinity mask
         */
        if (LOWORD(wParam) == IDOK) {
            for (nCpu = 0; nCpu < _countof(dwCpuTable); nCpu++) {
                /*
                 * First we have to create a mask out of each
                 * checkbox that the user checked.
                 */
                if (IsDlgButtonChecked(hDlg, dwCpuTable[nCpu]))
                    dwProcessAffinityMask |= ((ULONG_PTR)1 << nCpu);
            }

            /*
             * Make sure they are giving the process affinity
             * with at least one processor. I'd hate to see a
             * process that is not in a wait state get deprived
             * of it's cpu time.
             */
            if (!dwProcessAffinityMask) {
                LoadStringW(hInst, IDS_MSG_PROCESSONEPRO, strErrorText, _countof(strErrorText));
                LoadStringW(hInst, IDS_MSG_INVALIDOPTION, szTitle, _countof(szTitle));
                MessageBoxW(hDlg, strErrorText, szTitle, MB_OK|MB_ICONSTOP);
                return TRUE;
            }

            /*
             * Try to set the process affinity
             */
            if (!SetProcessAffinityMask(hProcessAffinityHandle, dwProcessAffinityMask)) {
                GetLastErrorText(strErrorText, _countof(strErrorText));
                EndDialog(hDlg, LOWORD(wParam));
                LoadStringW(hInst, IDS_MSG_ACCESSPROCESSAFF, szTitle, _countof(szTitle));
                MessageBoxW(hMainWnd, strErrorText, szTitle, MB_OK|MB_ICONSTOP);
            }

            EndDialog(hDlg, LOWORD(wParam));
            return TRUE;
        }

        break;
    }

    return 0;
}
