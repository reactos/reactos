/*
 *  ReactOS applications
 *  Copyright (C) 2004-2008 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS GUI first stage setup application
 * FILE:        base/setup/reactos/drivepage.c
 * PROGRAMMERS: Matthias Kupfer
 *              Dmitry Chapyshev (dmitry@reactos.org)
 */

#include "reactos.h"
#include "resource.h"

/* GLOBALS ******************************************************************/

#define IDS_LIST_COLUMN_FIRST IDS_PARTITION_NAME
#define IDS_LIST_COLUMN_LAST  IDS_PARTITION_TYPE

#define MAX_LIST_COLUMNS (IDS_LIST_COLUMN_LAST - IDS_LIST_COLUMN_FIRST + 1)
static const UINT column_ids[MAX_LIST_COLUMNS] = {IDS_LIST_COLUMN_FIRST, IDS_LIST_COLUMN_FIRST + 1, IDS_LIST_COLUMN_FIRST + 2};
static const INT  column_widths[MAX_LIST_COLUMNS] = {200, 150, 150};
static const INT  column_alignment[MAX_LIST_COLUMNS] = {LVCFMT_LEFT, LVCFMT_LEFT, LVCFMT_LEFT};

/* FUNCTIONS ****************************************************************/

static INT_PTR CALLBACK
MoreOptDlgProc(HWND hwndDlg,
               UINT uMsg,
               WPARAM wParam,
               LPARAM lParam)
{
    PSETUPDATA pSetupData;

    /* Retrieve pointer to the global setup data */
    pSetupData = (PSETUPDATA)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            /* Save pointer to the global setup data */
            pSetupData = (PSETUPDATA)((LPPROPSHEETPAGE)lParam)->lParam;
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (DWORD_PTR)pSetupData);

            CheckDlgButton(hwndDlg, IDC_INSTFREELDR, BST_CHECKED);
            SendMessage(GetDlgItem(hwndDlg, IDC_PATH),
                        WM_SETTEXT,
                        (WPARAM)0,
                        (LPARAM)pSetupData->USetupData.InstallationDirectory);
            break;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDOK:
                    SendMessage(GetDlgItem(hwndDlg, IDC_PATH),
                                WM_GETTEXT,
                                (WPARAM)ARRAYSIZE(pSetupData->USetupData.InstallationDirectory),
                                (LPARAM)pSetupData->USetupData.InstallationDirectory);

                    EndDialog(hwndDlg, IDOK);
                    return TRUE;

                case IDCANCEL:
                    EndDialog(hwndDlg, IDCANCEL);
                    return TRUE;
            }
            break;
    }

    return FALSE;
}

static INT_PTR CALLBACK
PartitionDlgProc(HWND hwndDlg,
                 UINT uMsg,
                 WPARAM wParam,
                 LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            break;
        case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
                case IDOK:
                    EndDialog(hwndDlg, IDOK);
                    return TRUE;
                case IDCANCEL:
                    EndDialog(hwndDlg, IDCANCEL);
                    return TRUE;
            }
        }
    }
    return FALSE;
}


BOOL
CreateListViewColumns(
    IN HINSTANCE hInstance,
    IN HWND hWndListView,
    IN const UINT* pIDs,
    IN const INT* pColsWidth,
    IN const INT* pColsAlign,
    IN UINT nNumOfColumns)
{
    UINT i;
    LVCOLUMN lvC;
    WCHAR szText[50];

    /* Create the columns */
    lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvC.pszText = szText;

    /* Load the column labels from the resource file */
    for (i = 0; i < nNumOfColumns; i++)
    {
        lvC.iSubItem = i;
        lvC.cx = pColsWidth[i];
        lvC.fmt = pColsAlign[i];

        LoadStringW(hInstance, pIDs[i], szText, ARRAYSIZE(szText));

        if (ListView_InsertColumn(hWndListView, i, &lvC) == -1)
            return FALSE;
    }

    return TRUE;
}


INT_PTR
CALLBACK
DriveDlgProc(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    PSETUPDATA pSetupData;
#if 1
    HDEVINFO h;
    HWND hList;
    SP_DEVINFO_DATA DevInfoData;
    DWORD i;
#endif

    /* Retrieve pointer to the global setup data */
    pSetupData = (PSETUPDATA)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            /* Save pointer to the global setup data */
            pSetupData = (PSETUPDATA)((LPPROPSHEETPAGE)lParam)->lParam;
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (DWORD_PTR)pSetupData);

            CreateListViewColumns(pSetupData->hInstance,
                                  GetDlgItem(hwndDlg, IDC_PARTITION),
                                  column_ids,
                                  column_widths,
                                  column_alignment,
                                  MAX_LIST_COLUMNS);

#if 1
            h = SetupDiGetClassDevs(&GUID_DEVCLASS_DISKDRIVE, NULL, NULL, DIGCF_PRESENT);
            if (h != INVALID_HANDLE_VALUE)
            {
                hList =GetDlgItem(hwndDlg, IDC_PARTITION); 
                DevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
                for (i=0; SetupDiEnumDeviceInfo(h, i, &DevInfoData); i++)
                {
                    DWORD DataT;
                    LPTSTR buffer = NULL;
                    DWORD buffersize = 0;

                    while (!SetupDiGetDeviceRegistryProperty(h,
                                                             &DevInfoData,
                                                             SPDRP_DEVICEDESC,
                                                             &DataT,
                                                             (PBYTE)buffer,
                                                             buffersize,
                                                             &buffersize))
                    {
                        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
                        {
                            if (buffer) LocalFree(buffer);
                            buffer = LocalAlloc(LPTR, buffersize * 2);
                        }
                        else
                            break;
                    }
                    if (buffer)
                    {
                        SendMessage(hList, LB_ADDSTRING, (WPARAM) 0, (LPARAM) buffer);
                        LocalFree(buffer);
                    }
                }
                SetupDiDestroyDeviceInfoList(h);
            }
#endif
        }
        break;

        case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
                case IDC_PARTMOREOPTS:
                    DialogBoxParam(pSetupData->hInstance,
                                   MAKEINTRESOURCE(IDD_BOOTOPTIONS),
                                   hwndDlg,
                                   MoreOptDlgProc,
                                   (LPARAM)pSetupData);
                    break;
                case IDC_PARTCREATE:
                    DialogBox(pSetupData->hInstance,
                              MAKEINTRESOURCE(IDD_PARTITION),
                              hwndDlg,
                              PartitionDlgProc);
                    break;
                case IDC_PARTDELETE:
                    break;
            }
            break;
        }

        case WM_NOTIFY:
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;

            switch (lpnm->code)
            {        
                case PSN_SETACTIVE:
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT | PSWIZB_BACK);
                    break;

                case PSN_QUERYCANCEL:
                    SetWindowLongPtr(hwndDlg,
                                     DWLP_MSGRESULT,
                                     MessageBox(GetParent(hwndDlg),
                                                pSetupData->szAbortMessage,
                                                pSetupData->szAbortTitle,
                                                MB_YESNO | MB_ICONQUESTION) != IDYES);
                    return TRUE;

                default:
                    break;
            }
        }
        break;

        default:
            break;

    }

    return FALSE;
}

/* EOF */
