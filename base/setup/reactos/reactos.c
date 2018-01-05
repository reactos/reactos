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
 * FILE:        base/setup/reactos/reactos.c
 * PROGRAMMERS: Matthias Kupfer
 *              Dmitry Chapyshev (dmitry@reactos.org)
 */

#include "reactos.h"
#include "resource.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS ******************************************************************/

HANDLE ProcessHeap;
BOOLEAN IsUnattendedSetup = FALSE;
SETUPDATA SetupData;


/* FUNCTIONS ****************************************************************/

static VOID
CenterWindow(HWND hWnd)
{
    HWND hWndParent;
    RECT rcParent;
    RECT rcWindow;

    hWndParent = GetParent(hWnd);
    if (hWndParent == NULL)
        hWndParent = GetDesktopWindow();

    GetWindowRect(hWndParent, &rcParent);
    GetWindowRect(hWnd, &rcWindow);

    SetWindowPos(hWnd,
                 HWND_TOP,
                 ((rcParent.right - rcParent.left) - (rcWindow.right - rcWindow.left)) / 2,
                 ((rcParent.bottom - rcParent.top) - (rcWindow.bottom - rcWindow.top)) / 2,
                 0,
                 0,
                 SWP_NOSIZE);
}

static HFONT
CreateTitleFont(VOID)
{
    NONCLIENTMETRICS ncm;
    LOGFONT LogFont;
    HDC hdc;
    INT FontSize;
    HFONT hFont;

    ncm.cbSize = sizeof(NONCLIENTMETRICS);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);

    LogFont = ncm.lfMessageFont;
    LogFont.lfWeight = FW_BOLD;
    _tcscpy(LogFont.lfFaceName, _T("MS Shell Dlg"));

    hdc = GetDC(NULL);
    FontSize = 12;
    LogFont.lfHeight = 0 - GetDeviceCaps (hdc, LOGPIXELSY) * FontSize / 72;
    hFont = CreateFontIndirect(&LogFont);
    ReleaseDC(NULL, hdc);

    return hFont;
}

INT DisplayError(
    IN HWND hParentWnd OPTIONAL,
    IN UINT uIDTitle,
    IN UINT uIDMessage)
{
    WCHAR message[512], caption[64];

    LoadStringW(SetupData.hInstance, uIDMessage, message, ARRAYSIZE(message));
    LoadStringW(SetupData.hInstance, uIDTitle, caption, ARRAYSIZE(caption));

    return MessageBoxW(hParentWnd, message, caption, MB_OK | MB_ICONERROR);
}

static INT_PTR CALLBACK
StartDlgProc(
    IN HWND hwndDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
    PSETUPDATA pSetupData;

    /* Retrieve pointer to the global setup data */
    pSetupData = (PSETUPDATA)GetWindowLongPtrW(hwndDlg, GWLP_USERDATA);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            /* Save pointer to the global setup data */
            pSetupData = (PSETUPDATA)((LPPROPSHEETPAGE)lParam)->lParam;
            SetWindowLongPtrW(hwndDlg, GWLP_USERDATA, (DWORD_PTR)pSetupData);

            /* Center the wizard window */
            CenterWindow(GetParent(hwndDlg));

            /* Set title font */
            SendDlgItemMessage(hwndDlg,
                               IDC_STARTTITLE,
                               WM_SETFONT,
                               (WPARAM)pSetupData->hTitleFont,
                               (LPARAM)TRUE);
            break;

        case WM_NOTIFY:
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;

            switch (lpnm->code)
            {
                case PSN_SETACTIVE:
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT);
                    break;

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

static INT_PTR CALLBACK
TypeDlgProc(
    IN HWND hwndDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
    PSETUPDATA pSetupData;

    /* Retrieve pointer to the global setup data */
    pSetupData = (PSETUPDATA)GetWindowLongPtrW(hwndDlg, GWLP_USERDATA);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            /* Save pointer to the global setup data */
            pSetupData = (PSETUPDATA)((LPPROPSHEETPAGE)lParam)->lParam;
            SetWindowLongPtrW(hwndDlg, GWLP_USERDATA, (DWORD_PTR)pSetupData);

            /* Check the 'install' radio button */
            CheckDlgButton(hwndDlg, IDC_INSTALL, BST_CHECKED);

            /*
             * Enable the 'update' radio button and text only if we have
             * available NT installations, otherwise disable them.
             */
            if (pSetupData->NtOsInstallsList &&
                GetNumberOfListEntries(pSetupData->NtOsInstallsList) != 0)
            {
                EnableWindow(GetDlgItem(hwndDlg, IDC_UPDATE), TRUE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_UPDATETEXT), TRUE);
            }
            else
            {
                EnableWindow(GetDlgItem(hwndDlg, IDC_UPDATE), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_UPDATETEXT), FALSE);
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
                    SetWindowLongPtrW(hwndDlg,
                                      DWLP_MSGRESULT,
                                      MessageBoxW(GetParent(hwndDlg),
                                                  pSetupData->szAbortMessage,
                                                  pSetupData->szAbortTitle,
                                                  MB_YESNO | MB_ICONQUESTION) != IDYES);
                    return TRUE;

                case PSN_WIZNEXT: /* Set the selected data */
                {
                    pSetupData->RepairUpdateFlag =
                        !(SendMessageW(GetDlgItem(hwndDlg, IDC_INSTALL),
                                       BM_GETCHECK,
                                       0, 0) == BST_CHECKED);

                    /*
                     * Display the existing NT installations page only
                     * if we have more than one available NT installations.
                     */
                    if (pSetupData->NtOsInstallsList &&
                        GetNumberOfListEntries(pSetupData->NtOsInstallsList) > 1)
                    {
                        /* Actually the best would be to dynamically insert the page only when needed */
                        SetWindowLongPtrW(hwndDlg, DWLP_MSGRESULT, IDD_UPDATEREPAIRPAGE);
                    }
                    else
                    {
                        SetWindowLongPtrW(hwndDlg, DWLP_MSGRESULT, IDD_DEVICEPAGE);
                    }

                    return TRUE;
                }

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



typedef VOID
(NTAPI *PGET_ENTRY_DESCRIPTION)(
    IN PGENERIC_LIST_ENTRY Entry,
    OUT PWSTR Buffer,
    IN SIZE_T cchBufferSize);

VOID
InitGenericComboList(
    IN HWND hWndList,
    IN PGENERIC_LIST List,
    IN PGET_ENTRY_DESCRIPTION GetEntryDescriptionProc)
{
    INT Index, CurrentEntryIndex = 0;
    PGENERIC_LIST_ENTRY ListEntry;
    PLIST_ENTRY Entry;
    WCHAR CurrentItemText[256];

    for (Entry = List->ListHead.Flink;
         Entry != &List->ListHead;
         Entry = Entry->Flink)
    {
        ListEntry = CONTAINING_RECORD(Entry, GENERIC_LIST_ENTRY, Entry);

        if (GetEntryDescriptionProc)
        {
            GetEntryDescriptionProc(ListEntry,
                                    CurrentItemText,
                                    ARRAYSIZE(CurrentItemText));
            Index = SendMessageW(hWndList, CB_ADDSTRING, 0, (LPARAM)CurrentItemText);
        }
        else
        {
            Index = SendMessageW(hWndList, CB_ADDSTRING, 0, (LPARAM)L"n/a");
        }

        if (ListEntry == List->CurrentEntry)
            CurrentEntryIndex = Index;

        SendMessageW(hWndList, CB_SETITEMDATA, Index, (LPARAM)ListEntry);
    }

    SendMessageW(hWndList, CB_SETCURSEL, CurrentEntryIndex, 0);
}

INT
GetSelectedComboListItem(
    IN HWND hWndList)
{
    LRESULT Index;

    Index = SendMessageW(hWndList, CB_GETCURSEL, 0, 0);
    if (Index == CB_ERR)
        return CB_ERR;

    // TODO: Update List->CurrentEntry?
    // return SendMessageW(hWndList, CB_GETITEMDATA, (WPARAM)Index, 0);
    return Index;
}

typedef VOID
(NTAPI *PADD_ENTRY_ITEM)(
    IN HWND hWndList,
    IN LVITEM* plvItem,
    IN PGENERIC_LIST_ENTRY Entry,
    IN OUT PWSTR Buffer,
    IN SIZE_T cchBufferSize);

VOID
InitGenericListView(
    IN HWND hWndList,
    IN PGENERIC_LIST List,
    IN PADD_ENTRY_ITEM AddEntryItemProc)
{
    INT CurrentEntryIndex = 0;
    LVITEM lvItem;
    PGENERIC_LIST_ENTRY ListEntry;
    PLIST_ENTRY Entry;
    WCHAR CurrentItemText[256];

    for (Entry = List->ListHead.Flink;
         Entry != &List->ListHead;
         Entry = Entry->Flink)
    {
        ListEntry = CONTAINING_RECORD(Entry, GENERIC_LIST_ENTRY, Entry);

        if (!AddEntryItemProc)
            continue;

        AddEntryItemProc(hWndList,
                         &lvItem,
                         ListEntry,
                         CurrentItemText,
                         ARRAYSIZE(CurrentItemText));

        if (ListEntry == List->CurrentEntry)
            CurrentEntryIndex = lvItem.iItem;
    }

    SendMessageW(hWndList, LVM_ENSUREVISIBLE, CurrentEntryIndex, FALSE);
    ListView_SetItemState(hWndList, CurrentEntryIndex, LVIS_SELECTED, LVIS_SELECTED);
    ListView_SetItemState(hWndList, CurrentEntryIndex, LVIS_FOCUSED, LVIS_FOCUSED);
}


static VOID
NTAPI
GetSettingDescription(
    IN PGENERIC_LIST_ENTRY Entry,
    OUT PWSTR Buffer,
    IN SIZE_T cchBufferSize)
{
    StringCchCopyW(Buffer, cchBufferSize,
                   ((PGENENTRY)GetListEntryData(Entry))->Value);
}

static VOID
NTAPI
AddNTOSInstallationItem(
    IN HWND hWndList,
    IN LVITEM* plvItem,
    IN PGENERIC_LIST_ENTRY Entry,
    IN OUT PWSTR Buffer, // SystemRootPath
    IN SIZE_T cchBufferSize)
{
    PNTOS_INSTALLATION NtOsInstall = (PNTOS_INSTALLATION)GetListEntryData(Entry);
    PPARTENTRY PartEntry = NtOsInstall->PartEntry;

    if (PartEntry && PartEntry->DriveLetter)
    {
        /* We have retrieved a partition that is mounted */
        StringCchPrintfW(Buffer, cchBufferSize,
                         L"%C:%s",
                         PartEntry->DriveLetter,
                         NtOsInstall->PathComponent);
    }
    else
    {
        /* We failed somewhere, just show the NT path */
        StringCchPrintfW(Buffer, cchBufferSize,
                         L"%wZ",
                         &NtOsInstall->SystemNtPath);
    }

    plvItem->mask = LVIF_IMAGE | LVIF_TEXT | LVIF_PARAM;
    plvItem->iItem = 0;
    plvItem->iSubItem = 0;
    plvItem->lParam = (LPARAM)Entry;
    plvItem->pszText = NtOsInstall->InstallationName;

    /* Associate vendor icon */
    if (FindSubStrI(NtOsInstall->VendorName, VENDOR_REACTOS))
    {
        plvItem->mask |= LVIF_IMAGE;
        plvItem->iImage = 0;
    }
    else if (FindSubStrI(NtOsInstall->VendorName, VENDOR_MICROSOFT))
    {
        plvItem->mask |= LVIF_IMAGE;
        plvItem->iImage = 1;
    }

    plvItem->iItem = SendMessageW(hWndList, LVM_INSERTITEMW, 0, (LPARAM)plvItem);

    plvItem->iSubItem = 1;
    plvItem->pszText = Buffer; // SystemRootPath;
    SendMessageW(hWndList, LVM_SETITEMTEXTW, plvItem->iItem, (LPARAM)plvItem);

    plvItem->iSubItem = 2;
    plvItem->pszText = NtOsInstall->VendorName;
    SendMessageW(hWndList, LVM_SETITEMTEXTW, plvItem->iItem, (LPARAM)plvItem);
}


#define IDS_LIST_COLUMN_FIRST IDS_INSTALLATION_NAME
#define IDS_LIST_COLUMN_LAST  IDS_INSTALLATION_VENDOR

#define MAX_LIST_COLUMNS (IDS_LIST_COLUMN_LAST - IDS_LIST_COLUMN_FIRST + 1)
static const UINT column_ids[MAX_LIST_COLUMNS] = {IDS_LIST_COLUMN_FIRST, IDS_LIST_COLUMN_FIRST + 1, IDS_LIST_COLUMN_FIRST + 2};
static const INT  column_widths[MAX_LIST_COLUMNS] = {200, 150, 100};
static const INT  column_alignment[MAX_LIST_COLUMNS] = {LVCFMT_LEFT, LVCFMT_LEFT, LVCFMT_LEFT};

static INT_PTR CALLBACK
UpgradeRepairDlgProc(
    IN HWND hwndDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
    PSETUPDATA pSetupData;
    HWND hList;
    HIMAGELIST hSmall;

    /* Retrieve pointer to the global setup data */
    pSetupData = (PSETUPDATA)GetWindowLongPtrW(hwndDlg, GWLP_USERDATA);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            /* Save pointer to the global setup data */
            pSetupData = (PSETUPDATA)((LPPROPSHEETPAGE)lParam)->lParam;
            SetWindowLongPtrW(hwndDlg, GWLP_USERDATA, (DWORD_PTR)pSetupData);

            hList = GetDlgItem(hwndDlg, IDC_NTOSLIST);

            CreateListViewColumns(pSetupData->hInstance,
                                  hList,
                                  column_ids,
                                  column_widths,
                                  column_alignment,
                                  MAX_LIST_COLUMNS);

            /* Create the ImageList */
            hSmall = ImageList_Create(GetSystemMetrics(SM_CXSMICON),
                                      GetSystemMetrics(SM_CYSMICON),
                                      ILC_COLOR32 | ILC_MASK, // ILC_COLOR24
                                      1, 1);

            /* Add event type icons to the ImageList */
            ImageList_AddIcon(hSmall, LoadIconW(pSetupData->hInstance, MAKEINTRESOURCEW(IDI_ROSICON)));
            ImageList_AddIcon(hSmall, LoadIconW(pSetupData->hInstance, MAKEINTRESOURCEW(IDI_WINICON)));

            /* Assign the ImageList to the List View */
            ListView_SetImageList(hList, hSmall, LVSIL_SMALL);

            InitGenericListView(hList, pSetupData->NtOsInstallsList, AddNTOSInstallationItem);

            break;
        }

        case WM_DESTROY:
        {
            hList = GetDlgItem(hwndDlg, IDC_NTOSLIST);
            hSmall = ListView_GetImageList(hList, LVSIL_SMALL);
            ListView_SetImageList(hList, NULL, LVSIL_SMALL);
            ImageList_Destroy(hSmall);
            return TRUE;
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
                    SetWindowLongPtrW(hwndDlg,
                                      DWLP_MSGRESULT,
                                      MessageBoxW(GetParent(hwndDlg),
                                                  pSetupData->szAbortMessage,
                                                  pSetupData->szAbortTitle,
                                                  MB_YESNO | MB_ICONQUESTION) != IDYES);
                    return TRUE;

                case PSN_WIZNEXT: /* Set the selected data */
                    pSetupData->RepairUpdateFlag =
                        !(SendMessageW(GetDlgItem(hwndDlg, IDC_INSTALL),
                                       BM_GETCHECK,
                                       0, 0) == BST_CHECKED);
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

static INT_PTR CALLBACK
DeviceDlgProc(
    IN HWND hwndDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
    PSETUPDATA pSetupData;
    HWND hList;

    /* Retrieve pointer to the global setup data */
    pSetupData = (PSETUPDATA)GetWindowLongPtrW(hwndDlg, GWLP_USERDATA);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            /* Save pointer to the global setup data */
            pSetupData = (PSETUPDATA)((LPPROPSHEETPAGE)lParam)->lParam;
            SetWindowLongPtrW(hwndDlg, GWLP_USERDATA, (DWORD_PTR)pSetupData);

            hList = GetDlgItem(hwndDlg, IDC_COMPUTER);
            InitGenericComboList(hList, pSetupData->USetupData.ComputerList, GetSettingDescription);

            hList = GetDlgItem(hwndDlg, IDC_DISPLAY);
            InitGenericComboList(hList, pSetupData->USetupData.DisplayList, GetSettingDescription);

            hList = GetDlgItem(hwndDlg, IDC_KEYBOARD);
            InitGenericComboList(hList, pSetupData->USetupData.KeyboardList, GetSettingDescription);

            // hList = GetDlgItem(hwndDlg, IDC_KEYBOARD_LAYOUT);
            // InitGenericComboList(hList, pSetupData->USetupData.LayoutList, GetSettingDescription);

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
                    SetWindowLongPtrW(hwndDlg,
                                      DWLP_MSGRESULT,
                                      MessageBoxW(GetParent(hwndDlg),
                                                  pSetupData->szAbortMessage,
                                                  pSetupData->szAbortTitle,
                                                  MB_YESNO | MB_ICONQUESTION) != IDYES);
                    return TRUE;

                case PSN_WIZNEXT: /* Set the selected data */
                {
                    hList = GetDlgItem(hwndDlg, IDC_COMPUTER);
                    pSetupData->SelectedComputer = GetSelectedComboListItem(hList);

                    hList = GetDlgItem(hwndDlg, IDC_DISPLAY);
                    pSetupData->SelectedDisplay = GetSelectedComboListItem(hList);

                    hList = GetDlgItem(hwndDlg, IDC_KEYBOARD);
                    pSetupData->SelectedKeyboard = GetSelectedComboListItem(hList);

                    return TRUE;
                }

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

static INT_PTR CALLBACK
SummaryDlgProc(
    IN HWND hwndDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
    PSETUPDATA pSetupData;

    /* Retrieve pointer to the global setup data */
    pSetupData = (PSETUPDATA)GetWindowLongPtrW(hwndDlg, GWLP_USERDATA);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            /* Save pointer to the global setup data */
            pSetupData = (PSETUPDATA)((LPPROPSHEETPAGE)lParam)->lParam;
            SetWindowLongPtrW(hwndDlg, GWLP_USERDATA, (DWORD_PTR)pSetupData);
            break;

        case WM_NOTIFY:
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;

            switch (lpnm->code)
            {
                case PSN_SETACTIVE:
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT | PSWIZB_BACK);
                    break;

                case PSN_QUERYCANCEL:
                    SetWindowLongPtrW(hwndDlg,
                                      DWLP_MSGRESULT,
                                      MessageBoxW(GetParent(hwndDlg),
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

static INT_PTR CALLBACK
ProcessDlgProc(
    IN HWND hwndDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
    PSETUPDATA pSetupData;

    /* Retrieve pointer to the global setup data */
    pSetupData = (PSETUPDATA)GetWindowLongPtrW(hwndDlg, GWLP_USERDATA);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            /* Save pointer to the global setup data */
            pSetupData = (PSETUPDATA)((LPPROPSHEETPAGE)lParam)->lParam;
            SetWindowLongPtrW(hwndDlg, GWLP_USERDATA, (DWORD_PTR)pSetupData);
            break;

        case WM_NOTIFY:
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;

            switch (lpnm->code)
            {
                case PSN_SETACTIVE:
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT);
                   // disable all buttons during installation process
                   // PropSheet_SetWizButtons(GetParent(hwndDlg), 0 );
                   break;
                case PSN_QUERYCANCEL:
                    SetWindowLongPtrW(hwndDlg,
                                      DWLP_MSGRESULT,
                                      MessageBoxW(GetParent(hwndDlg),
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

static INT_PTR CALLBACK
RestartDlgProc(
    IN HWND hwndDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
    PSETUPDATA pSetupData;

    /* Retrieve pointer to the global setup data */
    pSetupData = (PSETUPDATA)GetWindowLongPtrW(hwndDlg, GWLP_USERDATA);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            /* Save pointer to the global setup data */
            pSetupData = (PSETUPDATA)((LPPROPSHEETPAGE)lParam)->lParam;
            SetWindowLongPtrW(hwndDlg, GWLP_USERDATA, (DWORD_PTR)pSetupData);

            /* Set title font */
            /*SendDlgItemMessage(hwndDlg,
                                 IDC_STARTTITLE,
                                 WM_SETFONT,
                                 (WPARAM)hTitleFont,
                                 (LPARAM)TRUE);*/
            break;

        case WM_TIMER:
        {
            INT Position;
            HWND hWndProgress;

            hWndProgress = GetDlgItem(hwndDlg, IDC_RESTART_PROGRESS);
            Position = SendMessageW(hWndProgress, PBM_GETPOS, 0, 0);
            if (Position == 300)
            {
                KillTimer(hwndDlg, 1);
                PropSheet_PressButton(GetParent(hwndDlg), PSBTN_FINISH);
            }
            else
            {
                SendMessageW(hWndProgress, PBM_SETPOS, Position + 1, 0);
            }
            return TRUE;
        }

        case WM_DESTROY:
            return TRUE;

        case WM_NOTIFY:
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;

            switch (lpnm->code)
            {
                case PSN_SETACTIVE: // Only "Finish" for closing the App
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_FINISH);
                    SendDlgItemMessage(hwndDlg, IDC_RESTART_PROGRESS, PBM_SETRANGE, 0, MAKELPARAM(0, 300));
                    SendDlgItemMessage(hwndDlg, IDC_RESTART_PROGRESS, PBM_SETPOS, 0, 0);
                    SetTimer(hwndDlg, 1, 50, NULL);
                    break;

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

BOOL LoadSetupData(
    IN OUT PSETUPDATA pSetupData)
{
    BOOL ret = TRUE;
    // INFCONTEXT InfContext;
    // TCHAR tmp[10];
    // DWORD LineLength;
    // LONG Count;

    /* Load the hardware, language and keyboard layout lists */

    pSetupData->USetupData.ComputerList = CreateComputerTypeList(pSetupData->USetupData.SetupInf);
    pSetupData->USetupData.DisplayList = CreateDisplayDriverList(pSetupData->USetupData.SetupInf);
    pSetupData->USetupData.KeyboardList = CreateKeyboardDriverList(pSetupData->USetupData.SetupInf);

    pSetupData->USetupData.LanguageList = CreateLanguageList(pSetupData->USetupData.SetupInf, pSetupData->DefaultLanguage);

    pSetupData->PartitionList = CreatePartitionList();

    pSetupData->NtOsInstallsList = CreateNTOSInstallationsList(pSetupData->PartitionList);
    if (!pSetupData->NtOsInstallsList)
        DPRINT1("Failed to get a list of NTOS installations; continue installation...\n");


    /* new part */
    pSetupData->SelectedLanguageId = pSetupData->DefaultLanguage;
    wcscpy(pSetupData->DefaultLanguage, pSetupData->USetupData.LocaleID);
    pSetupData->USetupData.LanguageId = (LANGID)(wcstol(pSetupData->SelectedLanguageId, NULL, 16) & 0xFFFF);

    pSetupData->USetupData.LayoutList = CreateKeyboardLayoutList(pSetupData->USetupData.SetupInf, pSetupData->SelectedLanguageId, pSetupData->DefaultKBLayout);

#if 0
    // get default for keyboard and language
    pSetupData->DefaultKBLayout = -1;
    pSetupData->DefaultLang = -1;

    // TODO: get defaults from underlaying running system
    if (SetupFindFirstLine(pSetupData->USetupData.SetupInf, _T("NLS"), _T("DefaultLayout"), &InfContext))
    {
        SetupGetStringField(&InfContext, 1, tmp, ARRAYSIZE(tmp), &LineLength);
        for (Count = 0; Count < pSetupData->KbLayoutCount; Count++)
        {
            if (_tcscmp(tmp, pSetupData->pKbLayouts[Count].LayoutId) == 0)
            {
                pSetupData->DefaultKBLayout = Count;
                break;
            }
        }
    }

    if (SetupFindFirstLine(pSetupData->USetupData.SetupInf, _T("NLS"), _T("DefaultLanguage"), &InfContext))
    {
        SetupGetStringField(&InfContext, 1, tmp, ARRAYSIZE(tmp), &LineLength);
        for (Count = 0; Count < pSetupData->LangCount; Count++)
        {
            if (_tcscmp(tmp, pSetupData->pLanguages[Count].LangId) == 0)
            {
                pSetupData->DefaultLang = Count;
                break;
            }
        }
    }
#endif

    return ret;
}

/*
 * Attempts to convert a pure NT file path into a corresponding Win32 path.
 * Adapted from GetInstallSourceWin32() in dll/win32/syssetup/wizard.c
 */
BOOL
ConvertNtPathToWin32Path(
    OUT PWSTR pwszPath,
    IN DWORD cchPathMax,
    IN PCWSTR pwszNTPath)
{
    WCHAR wszDrives[512];
    WCHAR wszNTPath[512]; // MAX_PATH ?
    DWORD cchDrives;
    PWCHAR pwszDrive;

    *pwszPath = UNICODE_NULL;

    cchDrives = GetLogicalDriveStringsW(_countof(wszDrives) - 1, wszDrives);
    if (cchDrives == 0 || cchDrives >= _countof(wszDrives))
    {
        /* Buffer too small or failure */
        DPRINT1("GetLogicalDriveStringsW failed\n");
        return FALSE;
    }

    for (pwszDrive = wszDrives; *pwszDrive; pwszDrive += wcslen(pwszDrive) + 1)
    {
        /* Retrieve the NT path corresponding to the current Win32 DOS path */
        pwszDrive[2] = UNICODE_NULL; // Temporarily remove the backslash
        QueryDosDeviceW(pwszDrive, wszNTPath, _countof(wszNTPath));
        pwszDrive[2] = L'\\';        // Restore the backslash

        wcscat(wszNTPath, L"\\");    // Concat a backslash

        DPRINT1("Testing '%S' --> '%S'\n", pwszDrive, wszNTPath);

        /* Check whether the NT path corresponds to the NT installation source path */
        if (!_wcsnicmp(wszNTPath, pwszNTPath, wcslen(wszNTPath)))
        {
            /* Found it! */
            wsprintf(pwszPath, L"%s%s", // cchPathMax
                     pwszDrive, pwszNTPath + wcslen(wszNTPath));
            DPRINT1("ConvertNtPathToWin32Path: %S\n", pwszPath);
            return TRUE;
        }
    }

    return FALSE;
}

/* Used to enable and disable the shutdown privilege */
/* static */ BOOL
EnablePrivilege(LPCWSTR lpszPrivilegeName, BOOL bEnablePrivilege)
{
    BOOL   Success;
    HANDLE hToken;
    TOKEN_PRIVILEGES tp;

    Success = OpenProcessToken(GetCurrentProcess(),
                               TOKEN_ADJUST_PRIVILEGES,
                               &hToken);
    if (!Success) return Success;

    Success = LookupPrivilegeValueW(NULL,
                                    lpszPrivilegeName,
                                    &tp.Privileges[0].Luid);
    if (!Success) goto Quit;

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Attributes = (bEnablePrivilege ? SE_PRIVILEGE_ENABLED : 0);

    Success = AdjustTokenPrivileges(hToken, FALSE, &tp, 0, NULL, NULL);

Quit:
    CloseHandle(hToken);
    return Success;
}

int WINAPI
_tWinMain(HINSTANCE hInst,
          HINSTANCE hPrevInstance,
          LPTSTR lpszCmdLine,
          int nCmdShow)
{
    ULONG Error;
    INITCOMMONCONTROLSEX iccx;
    PROPSHEETHEADER psh;
    HPROPSHEETPAGE ahpsp[8];
    PROPSHEETPAGE psp = {0};
    UINT nPages = 0;

    ProcessHeap = GetProcessHeap();

    /* Initialize Setup, phase 0 */
    InitializeSetup(&SetupData.USetupData, 0);

    /* Initialize Setup, phase 1 */
    Error = InitializeSetup(&SetupData.USetupData, 1);
    if (Error != ERROR_SUCCESS)
    {
        //
        // TODO: Write an error mapper (much like the MUIDisplayError of USETUP)
        //
        if (Error == ERROR_NO_SOURCE_DRIVE)
            MessageBoxW(NULL, L"GetSourcePaths failed!", L"Error", MB_ICONERROR);
        else if (Error == ERROR_LOAD_TXTSETUPSIF)
            DisplayError(NULL, IDS_CAPTION, IDS_NO_TXTSETUP_SIF);
        else // FIXME!!
            MessageBoxW(NULL, L"Unknown error!", L"Error", MB_ICONERROR);

        goto Quit;
    }

    /* Load extra setup data (HW lists etc...) */
    if (!LoadSetupData(&SetupData))
        goto Quit;

    SetupData.hInstance = hInst;

    CheckUnattendedSetup(&SetupData.USetupData);
    SetupData.bUnattend = IsUnattendedSetup; // FIXME :-)

    /* Cache commonly-used strings */
    LoadStringW(hInst, IDS_ABORTSETUP, SetupData.szAbortMessage, ARRAYSIZE(SetupData.szAbortMessage));
    LoadStringW(hInst, IDS_ABORTSETUP2, SetupData.szAbortTitle, ARRAYSIZE(SetupData.szAbortTitle));

    /* Whenever any of the common controls are used in your app,
     * you must call InitCommonControlsEx() to register the classes
     * for those controls. */
    iccx.dwSize = sizeof(iccx);
    iccx.dwICC = ICC_LISTVIEW_CLASSES | ICC_TREEVIEW_CLASSES /* | ICC_PROGRESS_CLASS */;
    InitCommonControlsEx(&iccx);

    /* Create title font */
    SetupData.hTitleFont = CreateTitleFont();

    if (!SetupData.bUnattend)
    {
        /* Create the Start page, until setup is working */
        // NOTE: What does "until setup is working" mean??
        psp.dwSize = sizeof(PROPSHEETPAGE);
        psp.dwFlags = PSP_DEFAULT | PSP_HIDEHEADER;
        psp.hInstance = hInst;
        psp.lParam = (LPARAM)&SetupData;
        psp.pfnDlgProc = StartDlgProc;
        psp.pszTemplate = MAKEINTRESOURCE(IDD_STARTPAGE);
        ahpsp[nPages++] = CreatePropertySheetPage(&psp);

        /* Create the install type selection page */
        psp.dwSize = sizeof(PROPSHEETPAGE);
        psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
        psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_TYPETITLE);
        psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_TYPESUBTITLE);
        psp.hInstance = hInst;
        psp.lParam = (LPARAM)&SetupData;
        psp.pfnDlgProc = TypeDlgProc;
        psp.pszTemplate = MAKEINTRESOURCE(IDD_TYPEPAGE);
        ahpsp[nPages++] = CreatePropertySheetPage(&psp);

        /* Create the upgrade/repair selection page */
        psp.dwSize = sizeof(PROPSHEETPAGE);
        psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
        psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_TYPETITLE);
        psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_TYPESUBTITLE);
        psp.hInstance = hInst;
        psp.lParam = (LPARAM)&SetupData;
        psp.pfnDlgProc = UpgradeRepairDlgProc;
        psp.pszTemplate = MAKEINTRESOURCE(IDD_UPDATEREPAIRPAGE);
        ahpsp[nPages++] = CreatePropertySheetPage(&psp);

        /* Create the device settings page */
        psp.dwSize = sizeof(PROPSHEETPAGE);
        psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
        psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_DEVICETITLE);
        psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_DEVICESUBTITLE);
        psp.hInstance = hInst;
        psp.lParam = (LPARAM)&SetupData;
        psp.pfnDlgProc = DeviceDlgProc;
        psp.pszTemplate = MAKEINTRESOURCE(IDD_DEVICEPAGE);
        ahpsp[nPages++] = CreatePropertySheetPage(&psp);

        /* Create the install device settings page / boot method / install directory */
        psp.dwSize = sizeof(PROPSHEETPAGE);
        psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
        psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_DRIVETITLE);
        psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_DRIVESUBTITLE);
        psp.hInstance = hInst;
        psp.lParam = (LPARAM)&SetupData;
        psp.pfnDlgProc = DriveDlgProc;
        psp.pszTemplate = MAKEINTRESOURCE(IDD_DRIVEPAGE);
        ahpsp[nPages++] = CreatePropertySheetPage(&psp);

        /* Create the summary page */
        psp.dwSize = sizeof(PROPSHEETPAGE);
        psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
        psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_SUMMARYTITLE);
        psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_SUMMARYSUBTITLE);
        psp.hInstance = hInst;
        psp.lParam = (LPARAM)&SetupData;
        psp.pfnDlgProc = SummaryDlgProc;
        psp.pszTemplate = MAKEINTRESOURCE(IDD_SUMMARYPAGE);
        ahpsp[nPages++] = CreatePropertySheetPage(&psp);
    }

    /* Create the installation progress page */
    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_PROCESSTITLE);
    psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_PROCESSSUBTITLE);
    psp.hInstance = hInst;
    psp.lParam = (LPARAM)&SetupData;
    psp.pfnDlgProc = ProcessDlgProc;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_PROCESSPAGE);
    ahpsp[nPages++] = CreatePropertySheetPage(&psp);

    /* Create the finish-and-reboot page */
    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = PSP_DEFAULT | PSP_HIDEHEADER;
    psp.hInstance = hInst;
    psp.lParam = (LPARAM)&SetupData;
    psp.pfnDlgProc = RestartDlgProc;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_RESTARTPAGE);
    ahpsp[nPages++] = CreatePropertySheetPage(&psp);

    /* Create the property sheet */
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_WIZARD97 | PSH_WATERMARK | PSH_HEADER;
    psh.hInstance = hInst;
    psh.hwndParent = NULL;
    psh.nPages = nPages;
    psh.nStartPage = 0;
    psh.phpage = ahpsp;
    psh.pszbmWatermark = MAKEINTRESOURCE(IDB_WATERMARK);
    psh.pszbmHeader = MAKEINTRESOURCE(IDB_HEADER);

    /* Display the wizard */
    PropertySheet(&psh);

    if (SetupData.hTitleFont)
        DeleteObject(SetupData.hTitleFont);

Quit:
    /* Setup has finished */
    FinishSetup(&SetupData.USetupData);

#if 0 // NOTE: Disabled for testing purposes only!
    EnablePrivilege(SE_SHUTDOWN_NAME, TRUE);
    ExitWindowsEx(EWX_REBOOT, 0);
    EnablePrivilege(SE_SHUTDOWN_NAME, FALSE);
#endif

    return 0;
}

/* EOF */
