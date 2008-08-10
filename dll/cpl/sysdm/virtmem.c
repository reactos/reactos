/*
 * PROJECT:     ReactOS system properties, control panel applet
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/cpl/sysdm/virtual.c
 * PURPOSE:     Virtual memory control dialog
 * COPYRIGHT:   Copyright 2006 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include "precomp.h"

static BOOL OnSelChange(HWND hwndDlg, PVIRTMEM pVirtMem);
static LPCTSTR lpKey = _T("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management");

static BOOL
ReadPageFileSettings(PVIRTMEM pVirtMem)
{
    HKEY hkey = NULL;
    DWORD dwType;
    DWORD dwDataSize;
    BOOL bRet = FALSE;

    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                       lpKey,
                       0,
                       NULL,
                       REG_OPTION_NON_VOLATILE,
                       KEY_QUERY_VALUE,
                       NULL,
                       &hkey,
                       NULL) == ERROR_SUCCESS)
    {
        if (RegQueryValueEx(hkey,
                            _T("PagingFiles"),
                            NULL,
                            &dwType,
                            NULL,
                            &dwDataSize) == ERROR_SUCCESS)
        {
            pVirtMem->szPagingFiles = (LPTSTR)HeapAlloc(GetProcessHeap(),
                                                        0,
                                                        dwDataSize);
            if (pVirtMem->szPagingFiles != NULL)
            {
                ZeroMemory(pVirtMem->szPagingFiles,
                           dwDataSize);
                if (RegQueryValueEx(hkey,
                                    _T("PagingFiles"),
                                    NULL,
                                    &dwType,
                                    (PBYTE)pVirtMem->szPagingFiles,
                                    &dwDataSize) == ERROR_SUCCESS)
                {
                    bRet = TRUE;
                }
            }
        }
    }

    if (!bRet)
        ShowLastWin32Error(pVirtMem->hSelf);

    if (hkey != NULL)
        RegCloseKey(hkey);

    return bRet;
}


static VOID
GetPageFileSizes(LPTSTR lpPageFiles,
                 LPINT lpInitialSize,
                 LPINT lpMaximumSize)
{
    INT i = 0;

    *lpInitialSize = -1;
    *lpMaximumSize = -1;

    while (*lpPageFiles != _T('\0'))
    {
        if (*lpPageFiles == _T(' '))
        {
            lpPageFiles++;

            switch (i)
            {
                case 0:
                    *lpInitialSize = (INT)_ttoi(lpPageFiles);
                    i = 1;
                    break;

                case 1:
                    *lpMaximumSize = (INT)_ttoi(lpPageFiles);
                    return;
            }
        }

        lpPageFiles++;
    }
}


static VOID
ParseMemSettings(PVIRTMEM pVirtMem)
{
    TCHAR szDrives[1024];    // all drives
    LPTSTR DrivePtr = szDrives;
    TCHAR szDrive[3]; // single drive
    TCHAR szVolume[MAX_PATH];
    TCHAR *szDisplayString;
    INT InitialSize = 0;
    INT MaxSize = 0;
    INT DriveLen;
    INT PgCnt = 0;

    DriveLen = GetLogicalDriveStrings(1023,
                                      szDrives);

    szDisplayString = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (MAX_PATH * 2 + 69) * sizeof(TCHAR));
    if (szDisplayString == NULL)
        return;

    while (DriveLen != 0)
    {
        INT Len;

        Len = lstrlen(DrivePtr) + 1;
        DriveLen -= Len;

        DrivePtr = _tcsupr(DrivePtr);

        /* copy the 'X:' portion */
        lstrcpyn(szDrive, DrivePtr, sizeof(szDrive) / sizeof(TCHAR));

        if (GetDriveType(DrivePtr) == DRIVE_FIXED)
        {
            /* does drive match the one in the registry ? */
            if (!_tcsncmp(pVirtMem->szPagingFiles, szDrive, 2))
            {
                /* FIXME: we only check the first available pagefile in the reg */
                GetPageFileSizes(pVirtMem->szPagingFiles,
                                 &InitialSize,
                                 &MaxSize);

                pVirtMem->Pagefile[PgCnt].InitialValue = InitialSize;
                pVirtMem->Pagefile[PgCnt].MaxValue = MaxSize;
                pVirtMem->Pagefile[PgCnt].bUsed = TRUE;
                lstrcpy(pVirtMem->Pagefile[PgCnt].szDrive, szDrive);
            }
            else
            {
                pVirtMem->Pagefile[PgCnt].InitialValue = 0;
                pVirtMem->Pagefile[PgCnt].MaxValue = 0;
                pVirtMem->Pagefile[PgCnt].bUsed = FALSE;
                lstrcpy(pVirtMem->Pagefile[PgCnt].szDrive, szDrive);
            }

            _tcscpy(szDisplayString, szDrive);
            _tcscat(szDisplayString, _T("\t"));

            /* set a volume label if there is one */
            if (GetVolumeInformation(DrivePtr,
                                     szVolume,
                                     255,
                                     NULL,
                                     NULL,
                                     NULL,
                                     NULL,
                                     0))
            {
                if (szVolume[0] != _T('\0'))
                {
                    TCHAR szVol[MAX_PATH + 2];
                    _stprintf(szVol, _T("[%s]"), szVolume);
                    _tcscat(szDisplayString, szVol);
                }
            }

            if ((InitialSize != 0) || (MaxSize != 0))
            {
                TCHAR szSize[64];

                _stprintf(szSize, _T("%i - %i"), InitialSize, MaxSize);
                _tcscat(szDisplayString, _T("\t"));
                _tcscat(szDisplayString, szSize);
            }

            SendMessage(pVirtMem->hListBox, LB_ADDSTRING, (WPARAM)0, (LPARAM)szDisplayString);
            PgCnt++;
        }

        DrivePtr += Len;
    }

    SendMessage(pVirtMem->hListBox, LB_SETCURSEL, (WPARAM)0, (LPARAM)0);
    HeapFree(GetProcessHeap(), 0, szDisplayString);
    pVirtMem->Count = PgCnt;
    OnSelChange(pVirtMem->hSelf, pVirtMem);
}


static VOID
WritePageFileSettings(PVIRTMEM pVirtMem)
{
    HKEY hk = NULL;
    TCHAR szPagingFiles[2048];
    TCHAR szText[256];
    INT i;
    INT nPos = 0;
    BOOL bErr = TRUE;

    for (i = 0; i < pVirtMem->Count; ++i)
    {
        if (pVirtMem->Pagefile[i].bUsed)
        {
            _stprintf(szText, _T("%s\\pagefile.sys %i %i"),
                      pVirtMem->Pagefile[i].szDrive,
                      pVirtMem->Pagefile[i].InitialValue,
                      pVirtMem->Pagefile[i].MaxValue);

            /* Add it to our overall registry string */
            lstrcat(szPagingFiles + nPos, szText);

            /* Record the position where the next string will start */
            nPos += (INT)lstrlen(szText) + 1;

            /* add another NULL for REG_MULTI_SZ */
            szPagingFiles[nPos] = _T('\0');
            nPos++;
        }
    }

    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                       lpKey,
                       0,
                       NULL,
                       REG_OPTION_NON_VOLATILE,
                       KEY_WRITE,
                       NULL,
                       &hk,
                       NULL) == ERROR_SUCCESS)
    {
        if (RegSetValueEx(hk,
                          _T("PagingFiles"),
                          0,
                          REG_MULTI_SZ,
                          (LPBYTE) szPagingFiles,
                          (DWORD) nPos * sizeof(TCHAR)) == ERROR_SUCCESS)
        {
            bErr = FALSE;
        }

        RegCloseKey(hk);
    }

    if (bErr)
        ShowLastWin32Error(pVirtMem->hSelf);
}


static VOID
SetListBoxColumns(HWND hwndListBox)
{
    const INT tabs[2] = {30, 120};

    SendMessage(hwndListBox, LB_SETTABSTOPS, (WPARAM)2, (LPARAM)&tabs[0]);
}


static VOID
OnNoPagingFile(PVIRTMEM pVirtMem)
{
    /* Disable the page file custom size boxes */
    EnableWindow(GetDlgItem(pVirtMem->hSelf, IDC_INITIALSIZE), FALSE);
    EnableWindow(GetDlgItem(pVirtMem->hSelf, IDC_MAXSIZE), FALSE);
}


static VOID
OnSysManSize(PVIRTMEM pVirtMem)
{
    /* Disable the page file custom size boxes */
    EnableWindow(GetDlgItem(pVirtMem->hSelf, IDC_INITIALSIZE), FALSE);
    EnableWindow(GetDlgItem(pVirtMem->hSelf, IDC_MAXSIZE), FALSE);
}


static VOID
OnCustom(PVIRTMEM pVirtMem)
{
    /* Enable the page file custom size boxes */
    EnableWindow(GetDlgItem(pVirtMem->hSelf, IDC_INITIALSIZE), TRUE);
    EnableWindow(GetDlgItem(pVirtMem->hSelf, IDC_MAXSIZE), TRUE);
}


static VOID
OnSet(PVIRTMEM pVirtMem)
{
    INT Index;
    UINT Value;
    BOOL bTranslated;

    pVirtMem->bSave = TRUE;

    Index  = (INT)SendDlgItemMessage(pVirtMem->hSelf,
                                     IDC_PAGEFILELIST,
                                     LB_GETCURSEL,
                                     0,
                                     0);
    if (Index < pVirtMem->Count)
    {
        /* check if custom settings are checked */
        if (IsDlgButtonChecked(pVirtMem->hSelf,
                               IDC_CUSTOM) == BST_CHECKED)
        {
            Value = GetDlgItemInt(pVirtMem->hSelf,
                                  IDC_INITIALSIZE,
                                  &bTranslated,
                                  FALSE);
            if (!bTranslated)
            {
                /* FIXME: Show error message instead of setting the edit
                          field to the previous value */
                SetDlgItemInt(pVirtMem->hSelf,
                              IDC_INITIALSIZE,
                              pVirtMem->Pagefile[Index].InitialValue,
                              FALSE);
            }
            else
            {
                pVirtMem->Pagefile[Index].InitialValue = Value;
            }

            Value = GetDlgItemInt(pVirtMem->hSelf,
                                  IDC_MAXSIZE,
                                  &bTranslated,
                                  FALSE);
            if (!bTranslated)
            {
                /* FIXME: Show error message instead of setting the edit
                          field to the previous value */
                SetDlgItemInt(pVirtMem->hSelf,
                              IDC_MAXSIZE,
                              pVirtMem->Pagefile[Index].MaxValue,
                              FALSE);
            }
            else
            {
                pVirtMem->Pagefile[Index].MaxValue = Value;
            }
        }
        else
        {
            /* set sizes to 0 */
            pVirtMem->Pagefile[Index].InitialValue = pVirtMem->Pagefile[Index].MaxValue = 0;

            // check to see if this drive is used for a paging file
            if (IsDlgButtonChecked(pVirtMem->hSelf,
                                   IDC_NOPAGEFILE) == BST_UNCHECKED)
            {
                pVirtMem->Pagefile[Index].bUsed = TRUE;
            }
            else
            {
                pVirtMem->Pagefile[Index].bUsed = FALSE;
            }
        }
    }
}


static BOOL
OnSelChange(HWND hwndDlg, PVIRTMEM pVirtMem)
{
    TCHAR szBuffer[64];
    MEMORYSTATUSEX MemoryStatus;
    ULARGE_INTEGER FreeBytes;
    DWORDLONG FreeMemory;
    INT Index;
    INT i;
    INT FileSize;

    Index = (INT)SendDlgItemMessage(hwndDlg,
                                    IDC_PAGEFILELIST,
                                    LB_GETCURSEL,
                                    0,
                                    0);
    if (Index < pVirtMem->Count)
    {
        /* Set drive letter */
        SetDlgItemText(hwndDlg, IDC_DRIVE,
                       pVirtMem->Pagefile[Index].szDrive);

        /* Set available disk space */
        if (GetDiskFreeSpaceEx(pVirtMem->Pagefile[Index].szDrive,
                               NULL, NULL, &FreeBytes))
        {
            _stprintf(szBuffer, _T("%I64u MB"), FreeBytes.QuadPart / (1024 * 1024));
            SetDlgItemText(hwndDlg, IDC_SPACEAVAIL, szBuffer);
        }

        if (pVirtMem->Pagefile[Index].InitialValue  != 0 &&
            pVirtMem->Pagefile[Index].MaxValue != 0)
        {
            /* enable and fill the custom values */
            EnableWindow(GetDlgItem(pVirtMem->hSelf, IDC_MAXSIZE), TRUE);
            EnableWindow(GetDlgItem(pVirtMem->hSelf, IDC_INITIALSIZE), TRUE);

            SetDlgItemInt(pVirtMem->hSelf,
                          IDC_INITIALSIZE,
                          pVirtMem->Pagefile[Index].InitialValue,
                          FALSE);

            SetDlgItemInt(pVirtMem->hSelf,
                          IDC_MAXSIZE,
                          pVirtMem->Pagefile[Index].MaxValue,
                          FALSE);

            CheckDlgButton(pVirtMem->hSelf,
                           IDC_CUSTOM,
                           BST_CHECKED);
        }
        else
        {
            /* It's not a custom value */
            EnableWindow(GetDlgItem(pVirtMem->hSelf, IDC_MAXSIZE), FALSE);
            EnableWindow(GetDlgItem(pVirtMem->hSelf, IDC_INITIALSIZE), FALSE);

            /* is it system managed */
            if (pVirtMem->Pagefile[Index].bUsed)
            {
                CheckDlgButton(pVirtMem->hSelf,
                               IDC_SYSMANSIZE,
                               BST_CHECKED);
            }
            else
            {
                CheckDlgButton(pVirtMem->hSelf,
                               IDC_NOPAGEFILE,
                               BST_CHECKED);
            }
        }

        /* Set minimum pagefile size */
        SetDlgItemText(hwndDlg, IDC_MINIMUM, _T("2 MB"));

        /* Set recommended pagefile size */
        MemoryStatus.dwLength = sizeof(MEMORYSTATUSEX);
        if (GlobalMemoryStatusEx(&MemoryStatus))
        {
            FreeMemory = MemoryStatus.ullTotalPhys / (1024 * 1024);
            _stprintf(szBuffer, _T("%I64u MB"), FreeMemory + (FreeMemory / 2));
            SetDlgItemText(hwndDlg, IDC_RECOMMENDED, szBuffer);
        }

        /* Set current pagefile size */
        FileSize = 0;
        for (i = 0; i < 26; i++)
        {
            FileSize += pVirtMem->Pagefile[i].InitialValue;
        }
        _stprintf(szBuffer, _T("%u MB"), FileSize);
        SetDlgItemText(hwndDlg, IDC_CURRENT, szBuffer);
    }

    return TRUE;
}


static VOID
OnOk(PVIRTMEM pVirtMem)
{
    if (pVirtMem->bSave == TRUE)
    {
        WritePageFileSettings(pVirtMem);
    }
}


static VOID
OnInitDialog(HWND hwnd, PVIRTMEM pVirtMem)
{
    pVirtMem->hSelf = hwnd;
    pVirtMem->hListBox = GetDlgItem(hwnd, IDC_PAGEFILELIST);
    pVirtMem->bSave = FALSE;

    SetListBoxColumns(pVirtMem->hListBox);

    /* Load the pagefile systems from the reg */
    if (ReadPageFileSettings(pVirtMem))
    {
        /* Parse our settings and set up dialog */
        ParseMemSettings(pVirtMem);
    }
}


INT_PTR CALLBACK
VirtMemDlgProc(HWND hwndDlg,
               UINT uMsg,
               WPARAM wParam,
               LPARAM lParam)
{
    PVIRTMEM pVirtMem;

    UNREFERENCED_PARAMETER(lParam);

    pVirtMem = (PVIRTMEM)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            pVirtMem = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(VIRTMEM));
            if (pVirtMem == NULL)
            {
                EndDialog(hwndDlg, 0);
                return FALSE;
            }

            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pVirtMem);

            OnInitDialog(hwndDlg, pVirtMem);
            break;

        case WM_DESTROY:
            if (pVirtMem->szPagingFiles)
                HeapFree(GetProcessHeap(), 0,
                         pVirtMem->szPagingFiles);
            HeapFree(GetProcessHeap(), 0, pVirtMem);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDCANCEL:
                    EndDialog(hwndDlg, 0);
                    return TRUE;

                case IDOK:
                    OnOk(pVirtMem);
                    EndDialog(hwndDlg, 0);
                    return TRUE;

                case IDC_NOPAGEFILE:
                    OnNoPagingFile(pVirtMem);
                    return TRUE;

                case IDC_SYSMANSIZE:
                    OnSysManSize(pVirtMem);
                    return TRUE;

                case IDC_CUSTOM:
                    OnCustom(pVirtMem);
                    return TRUE;

                case IDC_SET:
                    OnSet(pVirtMem);
                    return TRUE;

                case IDC_PAGEFILELIST:
                    switch HIWORD(wParam)
                    {
                        case LBN_SELCHANGE:
                            OnSelChange(hwndDlg, pVirtMem);
                            return TRUE;
                    }
                    break;
            }
            break;
    }

    return FALSE;
}
