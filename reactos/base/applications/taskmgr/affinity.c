/*
 *  ReactOS Task Manager
 *
 *  affinity.c
 *
 *  Copyright (C) 1999 - 2001  Brian Palmer  <brianp@reactos.org>
 *                2005         Klemens Friedl <frik85@reactos.at>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <precomp.h>

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
        GetLastErrorText(strErrorText, sizeof(strErrorText) / sizeof(WCHAR));
        LoadStringW(hInst, IDS_MSG_ACCESSPROCESSAFF, szTitle, sizeof(szTitle) / sizeof(WCHAR));
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
            GetLastErrorText(strErrorText, sizeof(strErrorText) / sizeof(WCHAR));
            EndDialog(hDlg, 0);
            LoadStringW(hInst, IDS_MSG_ACCESSPROCESSAFF, szTitle, sizeof(szTitle) / sizeof(WCHAR));
            MessageBoxW(hMainWnd, strErrorText, szTitle, MB_OK|MB_ICONSTOP);
        }

        for (nCpu=0; nCpu<sizeof(dwCpuTable) / sizeof(dwCpuTable[0]); nCpu++) {
            /*
             * Enable a checkbox for each processor present in the system
             */
            if (dwSystemAffinityMask & (1 << nCpu))
                EnableWindow(GetDlgItem(hDlg, dwCpuTable[nCpu]), TRUE);
            /*
             * Check each checkbox that the current process
             * has affinity with
             */
            if (dwProcessAffinityMask & (1 << nCpu))
                SendMessageW(GetDlgItem(hDlg, dwCpuTable[nCpu]), BM_SETCHECK, BST_CHECKED, 0);
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
            for (nCpu=0; nCpu<sizeof(dwCpuTable) / sizeof(dwCpuTable[0]); nCpu++) {
                /*
                 * First we have to create a mask out of each
                 * checkbox that the user checked.
                 */
                if (SendMessageW(GetDlgItem(hDlg, dwCpuTable[nCpu]), BM_GETCHECK, 0, 0))
                    dwProcessAffinityMask |= (1 << nCpu);
            }

            /*
             * Make sure they are giving the process affinity
             * with at least one processor. I'd hate to see a
             * process that is not in a wait state get deprived
             * of it's cpu time.
             */
            if (!dwProcessAffinityMask) {
                LoadStringW(hInst, IDS_MSG_PROCESSONEPRO, strErrorText, sizeof(strErrorText) / sizeof(WCHAR));
                LoadStringW(hInst, IDS_MSG_INVALIDOPTION, szTitle, sizeof(szTitle) / sizeof(WCHAR));
                MessageBoxW(hDlg, strErrorText, szTitle, MB_OK|MB_ICONSTOP);
                return TRUE;
            }

            /*
             * Try to set the process affinity
             */
            if (!SetProcessAffinityMask(hProcessAffinityHandle, dwProcessAffinityMask)) {
                GetLastErrorText(strErrorText, sizeof(strErrorText) / sizeof(WCHAR));
                EndDialog(hDlg, LOWORD(wParam));
                LoadStringW(hInst, IDS_MSG_ACCESSPROCESSAFF, szTitle, sizeof(szTitle) / sizeof(WCHAR));
                MessageBoxW(hMainWnd, strErrorText, szTitle, MB_OK|MB_ICONSTOP);
            }

            EndDialog(hDlg, LOWORD(wParam));
            return TRUE;
        }

        break;
    }

    return 0;
}
