/*
 * PROJECT:     ReactOS system properties, control panel applet
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/cpl/sysdm/virtual.c
 * PURPOSE:     Virtual memory control dialog
 * COPYRIGHT:   Copyright 2006 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include "precomp.h"

static LPCTSTR lpKey = _T("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management");

static BOOL
ReadPageFileSettings(PVIRTMEM pVirtMem)
{
    HKEY hkey = NULL;
    DWORD dwType;
    DWORD dwDataSize;
    BOOL bRet = FALSE;

    if(RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                      lpKey,
                      0,
                      NULL,
                      REG_OPTION_NON_VOLATILE,
                      KEY_QUERY_VALUE,
                      NULL,
                      &hkey,
                      NULL) == ERROR_SUCCESS)
    {
        if(RegQueryValueEx(hkey,
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
                if(RegQueryValueEx(hkey,
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


static INT
GetPageFileSizes(LPTSTR lpPageFiles)
{
    while (*lpPageFiles != _T('\0'))
    {
        if (*lpPageFiles == _T(' '))
        {
            lpPageFiles++;
            return (INT)_ttoi(lpPageFiles);
        }

        lpPageFiles++;
    }

    return -1;
}


static VOID
ParseMemSettings(PVIRTMEM pVirtMem)
{
    TCHAR szDrives[1024];    // all drives
    LPTSTR DrivePtr = szDrives;
    TCHAR szDrive[MAX_PATH]; // single drive
    TCHAR szVolume[MAX_PATH];
    INT InitialSize = 0;
    INT MaxSize = 0;
    INT DriveLen;
    INT PgCnt = 0;

    DriveLen = GetLogicalDriveStrings(1023,
                                      szDrives);

    while (DriveLen != 0)
    {
        LVITEM Item;
        INT Len;

        Len = lstrlen(DrivePtr) + 1;
        DriveLen -= Len;

        DrivePtr = _tcsupr(DrivePtr);

        /* copy the 'X:' portion */
        lstrcpyn(szDrive, DrivePtr, 3);

        if(GetDriveType(DrivePtr) == DRIVE_FIXED)
        {
            /* does drive match the one in the registry ? */
            if(!_tcsncmp(pVirtMem->szPagingFiles, szDrive, 2))
            {
                /* FIXME: we only check the first available pagefile in the reg */
                InitialSize = GetPageFileSizes(pVirtMem->szPagingFiles);
                MaxSize = GetPageFileSizes(pVirtMem->szPagingFiles);

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

            /* fill out the listview */
            ZeroMemory(&Item, sizeof(Item));
            Item.mask = LVIF_TEXT;
            Item.iItem = ListView_GetItemCount(pVirtMem->hListView);
            Item.pszText = szDrive;
            (void)ListView_InsertItem(pVirtMem->hListView, &Item);

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

                    Item.iSubItem = 1;
                    Item.pszText = szVol;
                    (void)ListView_InsertItem(pVirtMem->hListView, &Item);
                }
            }

            if ((InitialSize != 0) || (MaxSize != 0))
            {
                TCHAR szSize[64];

                _stprintf(szSize, _T("%i - %i"), InitialSize, MaxSize);

                Item.iSubItem = 2;
                Item.pszText = szSize;
                (void)ListView_InsertItem(pVirtMem->hListView, &Item);
            }

            PgCnt++;
        }

        DrivePtr += Len;
    }

    pVirtMem->Count = PgCnt;
}


static VOID
WritePageFileSettings(PVIRTMEM pVirtMem)
{
    HKEY hk = NULL;
    TCHAR szPagingFiles[2048];
    INT i;
    INT nPos = 0;
    BOOL bErr = TRUE;

    for(i = 0; i < pVirtMem->Count; ++i)
    {
        if(pVirtMem->Pagefile[i].bUsed)
        {
            TCHAR szText[256];

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

    if(RegCreateKeyEx(HKEY_LOCAL_MACHINE,
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
                          (DWORD) nPos * sizeof(TCHAR)))
        {
            bErr = FALSE;
        }

        RegCloseKey(hk);
    }

    if (bErr)
        ShowLastWin32Error(pVirtMem->hSelf);
}


static VOID
SetListViewColumns(HWND hwndListView)
{
    RECT rect;
    LV_COLUMN lvc;

    GetClientRect(hwndListView, &rect);

    (void)ListView_SetExtendedListViewStyle(hwndListView,
                                            LVS_EX_FULLROWSELECT);

    ZeroMemory(&lvc, sizeof(lvc));
    lvc.mask = LVCF_SUBITEM | LVCF_WIDTH  | LVCF_FMT;
    lvc.fmt = LVCFMT_LEFT;

    lvc.cx = (INT)((rect.right - rect.left) * 0.1);
    lvc.iSubItem = 0;
    (void)ListView_InsertColumn(hwndListView, 0, &lvc);

    lvc.cx = (INT)((rect.right - rect.left) * 0.3);
    lvc.iSubItem = 1;
    (void)ListView_InsertColumn(hwndListView, 1, &lvc);

    lvc.cx = (INT)((rect.right - rect.left) * 0.6);
    lvc.iSubItem = 2;
    (void)ListView_InsertColumn(hwndListView, 2, &lvc);
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

    pVirtMem->bSave = TRUE;

    Index  = (INT)SendDlgItemMessage(pVirtMem->hSelf,
                                     IDC_PAGEFILELIST,
                                     LB_GETCURSEL,
                                     0,
                                     0);

    if(Index < pVirtMem->Count)
    {
        TCHAR szText[255];

        /* check if custom settings are checked */
        if(SendDlgItemMessage(pVirtMem->hSelf,
                              IDC_CUSTOM,
                              BM_GETCHECK,
                              0,
                              0) == BST_CHECKED)
        {
            SendDlgItemMessage(pVirtMem->hSelf,
                               IDC_INITIALSIZE,
                               WM_GETTEXT,
                               254,
                               (LPARAM)szText);
            pVirtMem->Pagefile[Index].InitialValue = _ttoi(szText);

            SendDlgItemMessage(pVirtMem->hSelf,
                               IDC_MAXSIZE,
                               WM_GETTEXT,
                               254,
                               (LPARAM)szText);
            pVirtMem->Pagefile[Index].MaxValue = _ttoi(szText);
        }
        else
        {
            /* set sizes to 0 */
            pVirtMem->Pagefile[Index].InitialValue = pVirtMem->Pagefile[Index].MaxValue = 0;

            // check to see if this drive is used for a paging file
            if (SendDlgItemMessage(pVirtMem->hSelf,
                                   IDC_NOPAGEFILE,
                                   BM_GETCHECK,
                                   0,
                                   0) == BST_UNCHECKED)
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
OnSelChange(PVIRTMEM pVirtMem,
            LPNMLISTVIEW pnmv)
{
    TCHAR szCustVals[255];
    INT Index;

    UNREFERENCED_PARAMETER(pnmv);

    Index = (INT)SendDlgItemMessage(pVirtMem->hSelf,
                                    IDC_PAGEFILELIST,
                                    LB_GETCURSEL,
                                    0,
                                    0);

    if(Index < pVirtMem->Count)
    {

        if(pVirtMem->Pagefile[Index].InitialValue  != 0 &&
           pVirtMem->Pagefile[Index].MaxValue != 0)
        {
            /* enable and fill the custom values */
            EnableWindow(GetDlgItem(pVirtMem->hSelf, IDC_MAXSIZE), TRUE);
            EnableWindow(GetDlgItem(pVirtMem->hSelf, IDC_INITIALSIZE), TRUE);

            _itot(pVirtMem->Pagefile[Index].InitialValue , szCustVals, 10);
            SendDlgItemMessage(pVirtMem->hSelf,
                               IDC_INITIALSIZE,
                               WM_SETTEXT,
                               0,
                               (LPARAM)szCustVals);

            _itot(pVirtMem->Pagefile[Index].MaxValue, szCustVals, 10);
            SendDlgItemMessage(pVirtMem->hSelf,
                               IDC_MAXSIZE,
                               WM_SETTEXT,
                               0,
                               (LPARAM)szCustVals);

            SendDlgItemMessage(pVirtMem->hSelf,
                               IDC_CUSTOM,
                               BM_SETCHECK,
                               1,
                               0);
        }
        else
        {
            /* It's not a custom value */
            EnableWindow(GetDlgItem(pVirtMem->hSelf, IDC_MAXSIZE), FALSE);
            EnableWindow(GetDlgItem(pVirtMem->hSelf, IDC_INITIALSIZE), FALSE);

            /* is it system managed */
            if(pVirtMem->Pagefile[Index].bUsed)
            {
                SendDlgItemMessage(pVirtMem->hSelf,
                                   IDC_SYSMANSIZE,
                                   BM_SETCHECK,
                                   1,
                                   0);
            }
            else
            {
                SendDlgItemMessage(pVirtMem->hSelf,
                                   IDC_NOPAGEFILE,
                                   BM_SETCHECK,
                                   1,
                                   0);
            }
        }
    }

    return TRUE;
}


static VOID
OnOk(PVIRTMEM pVirtMem)
{
    if(pVirtMem->bSave == TRUE)
    {
        WritePageFileSettings(pVirtMem);
    }

    if (pVirtMem->szPagingFiles)
        HeapFree(GetProcessHeap(),
                 0,
                 pVirtMem->szPagingFiles);

    HeapFree(GetProcessHeap(),
             0,
             pVirtMem);
}


static VOID
OnCancel(PVIRTMEM pVirtMem)
{
    if (pVirtMem->szPagingFiles)
        HeapFree(GetProcessHeap(),
                 0,
                 pVirtMem->szPagingFiles);

    HeapFree(GetProcessHeap(),
             0,
             pVirtMem);
}


static PVIRTMEM
OnInitDialog(HWND hwnd)
{
    PVIRTMEM pVirtMem = (PVIRTMEM)HeapAlloc(GetProcessHeap(),
                                            HEAP_ZERO_MEMORY,
                                            sizeof(VIRTMEM));
    if (pVirtMem == NULL)
    {
        EndDialog(hwnd, 0);
    }

    pVirtMem->hSelf = hwnd;
    pVirtMem->hListView = GetDlgItem(hwnd, IDC_PAGEFILELIST);
    pVirtMem->bSave = FALSE;

    SetListViewColumns(pVirtMem->hListView);

    /* Load the pagefile systems from the reg */
    if (ReadPageFileSettings(pVirtMem))
    {
        /* Parse our settings and set up dialog */
        ParseMemSettings(pVirtMem);
    }

    return pVirtMem;
}


INT_PTR CALLBACK
VirtMemDlgProc(HWND hwndDlg,
               UINT uMsg,
               WPARAM wParam,
               LPARAM lParam)
{
    /* there can only be one instance of this dialog */
    static PVIRTMEM pVirtMem = NULL;

    UNREFERENCED_PARAMETER(lParam);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            pVirtMem = OnInitDialog(hwndDlg);
            break;

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDCANCEL:
                    OnCancel(pVirtMem);
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
            }
        }
        break;

        case WM_NOTIFY:
        {
            LPNMHDR pnmhdr = (LPNMHDR)lParam;

            switch (pnmhdr->code)
            {
                case LVN_ITEMCHANGED:
                {
                    LPNMLISTVIEW pnmv = (LPNMLISTVIEW) lParam;

                    OnSelChange(pVirtMem, pnmv);

                }
            }
        }
        break;
    }

    return FALSE;
}
