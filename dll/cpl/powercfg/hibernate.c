/*
 * PROJECT:         ReactOS Power Configuration Applet
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/cpl/powercfg/hibernate.c
 * PURPOSE:         hibernate tab of applet
 * PROGRAMMERS:     Alexander Wurzinger (Lohnegrim at gmx dot net)
 *                  Johannes Anderwald (johannes.anderwald@reactos.org)
 *                  Martin Rottensteiner
 *                  Dmitry Chapyshev (lentind@yandex.ru)
 */

#include "powercfg.h"

static VOID
Hib_InitDialog(HWND hwndDlg)
{
    SYSTEM_POWER_CAPABILITIES PowerCaps;
    MEMORYSTATUSEX msex;
    TCHAR szTemp[MAX_PATH];
    LPTSTR lpRoot;
    ULARGE_INTEGER FreeBytesAvailable, TotalNumberOfBytes, TotalNumberOfFreeBytes;

    if (GetPwrCapabilities(&PowerCaps))
    {
        CheckDlgButton(hwndDlg,
                   IDC_HIBERNATEFILE,
                   PowerCaps.HiberFilePresent ? BST_CHECKED : BST_UNCHECKED);

        msex.dwLength = sizeof(msex);
        if (!GlobalMemoryStatusEx(&msex))
        {
            return; // FIXME
        }

        if (GetWindowsDirectory(szTemp,MAX_PATH))
            lpRoot = szTemp;
        else
            lpRoot = NULL;

        // Get available space and size of selected volume.
        if (!GetDiskFreeSpaceEx(lpRoot, &FreeBytesAvailable, &TotalNumberOfBytes, &TotalNumberOfFreeBytes))
            TotalNumberOfFreeBytes.QuadPart = 0;

        // Print the free available space into selected volume.
        StrFormatByteSize(TotalNumberOfFreeBytes.QuadPart, szTemp, _countof(szTemp));
        SetDlgItemText(hwndDlg, IDC_FREESPACE, szTemp);

        // Print the amount of space required for hibernation.
        StrFormatByteSize(msex.ullTotalPhys, szTemp, _countof(szTemp));
        SetDlgItemText(hwndDlg, IDC_SPACEFORHIBERNATEFILE, szTemp);

        if (TotalNumberOfFreeBytes.QuadPart < msex.ullTotalPhys && !PowerCaps.HiberFilePresent)
        {
            EnableWindow(GetDlgItem(hwndDlg, IDC_HIBERNATEFILE), FALSE);
            ShowWindow(GetDlgItem(hwndDlg, IDC_TOLESSFREESPACE), TRUE);
        }
        else
        {
            ShowWindow(GetDlgItem(hwndDlg, IDC_TOLESSFREESPACE), FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_HIBERNATEFILE), TRUE);
        }
    }
}

INT_PTR
Hib_SaveData(HWND hwndDlg)
{
    BOOLEAN bHibernate;

    bHibernate = (BOOLEAN)(IsDlgButtonChecked(hwndDlg, IDC_HIBERNATEFILE) == BST_CHECKED);

    if (CallNtPowerInformation(SystemReserveHiberFile, &bHibernate, sizeof(bHibernate), NULL, 0) == STATUS_SUCCESS)
    {
        Hib_InitDialog(hwndDlg);
        return TRUE;
    }

    return FALSE;
}

/* Property page dialog callback */
INT_PTR CALLBACK
HibernateDlgProc(HWND hwndDlg,
                 UINT uMsg,
                 WPARAM wParam,
                 LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            Hib_InitDialog(hwndDlg);
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_HIBERNATEFILE:
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                    break;
            }
            break;

        case WM_NOTIFY:
            if (((LPNMHDR)lParam)->code == (UINT)PSN_APPLY)
            {
                return Hib_SaveData(hwndDlg);
            }
            break;
    }

    return FALSE;
}
