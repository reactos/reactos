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
#include <winnls.h> // For GetUserDefaultLCID()

#define NTOS_MODE_USER
#include <ndk/obfuncs.h>

#include "resource.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS ******************************************************************/

HANDLE ProcessHeap;
SETUPDATA SetupData;

/* The partition where to perform the installation */
PPARTENTRY InstallPartition = NULL;
// static PVOLENTRY InstallVolume = NULL;
#define InstallVolume (InstallPartition->Volume)

/* The system partition we will actually use */
PPARTENTRY SystemPartition = NULL;
// static PVOLENTRY SystemVolume = NULL;
#define SystemVolume (SystemPartition->Volume)

/* UI elements */
UI_CONTEXT UiContext;


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

/**
 * @brief
 * Create a bold font derived from the provided font.
 **/
static HFONT
CreateBoldFont(
    _In_opt_ HFONT hOrigFont,
    _In_opt_ INT PointSize)
{
    LOGFONTW lf = {0};

    if (hOrigFont)
    {
        GetObjectW(hOrigFont, sizeof(lf), &lf);
    }
    else
    {
        NONCLIENTMETRICSW ncm;
        ncm.cbSize = sizeof(ncm);
        SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);
        lf = ncm.lfMessageFont;
    }

    /* Make the font bold, keeping the other attributes */
    lf.lfWeight = FW_BOLD;

    /* Determine the font height (logical units) if necessary */
    if (PointSize)
    {
        HDC hdc = GetDC(NULL);
        lf.lfHeight = -MulDiv(PointSize, GetDeviceCaps(hdc, LOGPIXELSY), 72);
        // lf.lfWidth = 0;
        ReleaseDC(NULL, hdc);
    }

    return CreateFontIndirect(&lf);
}

static inline HFONT
CreateTitleFont(
    _In_opt_ HFONT hOrigFont)
{
    /* Title font is 12pt bold */
    return CreateBoldFont(hOrigFont, 12);
}

INT
DisplayMessageV(
    _In_opt_ HWND hWnd,
    _In_ UINT uType,
    _In_opt_ PCWSTR pszTitle,
    _In_opt_ PCWSTR pszFormatMessage,
    _In_ va_list args)
{
    INT iRes;
    HINSTANCE hInstance = NULL;
    MSGBOXPARAMSW mb = {0};
    LPWSTR Format;
    size_t MsgLen;
    WCHAR  StaticBuffer[256];
    LPWSTR Buffer = StaticBuffer; // Use the static buffer by default.

    /* We need to retrieve the current module's instance handle if either
     * the title or the format message is specified by a resource ID */
    if ((pszTitle && IS_INTRESOURCE(pszTitle)) || IS_INTRESOURCE(pszFormatMessage))
        hInstance = GetModuleHandleW(NULL); // SetupData.hInstance;

    /* Retrieve the format message string if this is a resource */
    if (pszFormatMessage && IS_INTRESOURCE(pszFormatMessage)) do
    {
        // LoadAllocStringW()
        PCWSTR pStr;

        /* Try to load the string from the resource */
        MsgLen = LoadStringW(hInstance, PtrToUlong(pszFormatMessage), (LPWSTR)&pStr, 0);
        if (MsgLen == 0)
        {
            /* No resource string was found, return NULL */
            Format = NULL;
            break;
        }

        /* Allocate a new buffer, adding a NULL-terminator */
        Format = HeapAlloc(GetProcessHeap(), 0, (MsgLen + 1) * sizeof(WCHAR));
        if (!Format)
        {
            MsgLen = 0;
            break;
        }

        /* Copy the string, NULL-terminated */
        StringCchCopyNW(Format, MsgLen + 1, pStr, MsgLen);
    } while (0);
    else
    {
        Format = (LPWSTR)pszFormatMessage;
    }

    if (Format)
    {
        /*
         * Retrieve the message length. If it is too long, allocate
         * an auxiliary buffer; otherwise use the static buffer.
         * The string is built to be NULL-terminated.
         */
        MsgLen = _vscwprintf(Format, args);
        if (MsgLen >= _countof(StaticBuffer))
        {
            Buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (MsgLen + 1) * sizeof(WCHAR));
            if (!Buffer)
            {
                /* Allocation failed, use the original format string verbatim */
                Buffer = Format;
            }
        }
        if (Buffer != Format)
        {
            /* Do the printf as we use the caller's format string */
            StringCchVPrintfW(Buffer, MsgLen + 1, Format, args);
        }
    }
    else
    {
        Format = (LPWSTR)pszFormatMessage;
        Buffer = Format;
    }

    /* Display the message */
    mb.cbSize = sizeof(mb);
    mb.hwndOwner = hWnd;
    mb.hInstance = hInstance;
    mb.lpszText = Buffer;
    mb.lpszCaption = pszTitle;
    mb.dwStyle = uType;
    mb.dwLanguageId = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);
    iRes = MessageBoxIndirectW(&mb);

    /* Free the buffers if needed */
    if ((Buffer != StaticBuffer) && (Buffer != Format))
        HeapFree(GetProcessHeap(), 0, Buffer);

    if (Format && (Format != pszFormatMessage))
        HeapFree(GetProcessHeap(), 0, Format);

    return iRes;
}

INT
__cdecl
DisplayMessage(
    _In_opt_ HWND hWnd,
    _In_ UINT uType,
    _In_opt_ PCWSTR pszTitle,
    _In_opt_ PCWSTR pszFormatMessage,
    ...)
{
    INT iRes;
    va_list args;

    va_start(args, pszFormatMessage);
    iRes = DisplayMessageV(hWnd, uType, pszTitle, pszFormatMessage, args);
    va_end(args);

    return iRes;
}

INT
__cdecl
DisplayError(
    _In_opt_ HWND hWnd,
    _In_ UINT uIDTitle,
    _In_ UINT uIDMessage,
    ...)
{
    INT iRes;
    va_list args;

    va_start(args, uIDMessage);
    iRes = DisplayMessageV(hWnd, MB_OK | MB_ICONERROR,
                           MAKEINTRESOURCEW(uIDTitle),
                           MAKEINTRESOURCEW(uIDMessage),
                           args);
    va_end(args);

    return iRes;
}

VOID
SetWindowResTextW(
    _In_ HWND hWnd,
    _In_opt_ HINSTANCE hInstance,
    _In_ UINT uID)
{
    WCHAR szText[256];
    LoadStringW(hInstance, uID, szText, _countof(szText));
    SetWindowTextW(hWnd, szText);
}

VOID
SetWindowResPrintfVW(
    _In_ HWND hWnd,
    _In_opt_ HINSTANCE hInstance,
    _In_ UINT uID,
    _In_ va_list args)
{
    WCHAR ResBuffer[256];
    WCHAR szText[256];

    LoadStringW(hInstance, uID, ResBuffer, _countof(ResBuffer));
    StringCchVPrintfW(szText, _countof(szText), ResBuffer, args);
    SetWindowTextW(hWnd, szText);
}

VOID
__cdecl
SetWindowResPrintfW(
    _In_ HWND hWnd,
    _In_opt_ HINSTANCE hInstance,
    _In_ UINT uID,
    ...)
{
    va_list args;

    va_start(args, uID);
    SetWindowResPrintfVW(hWnd, hInstance, uID, args);
    va_end(args);
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
        {
            /* Save pointer to the global setup data */
            pSetupData = (PSETUPDATA)((LPPROPSHEETPAGE)lParam)->lParam;
            SetWindowLongPtrW(hwndDlg, GWLP_USERDATA, (DWORD_PTR)pSetupData);

            /* Set title font */
            SetDlgItemFont(hwndDlg, IDC_STARTTITLE, pSetupData->hTitleFont, TRUE);

            // TEMPTEMP: Set the ReactOS-Alpha information in bold.
            // TODO: Remove once we reach 0.5/Beta :)
            SetDlgItemFont(hwndDlg, IDC_WARNTEXT1, pSetupData->hBoldFont, TRUE);
            SetDlgItemFont(hwndDlg, IDC_WARNTEXT2, pSetupData->hBoldFont, TRUE);
            SetDlgItemFont(hwndDlg, IDC_WARNTEXT3, pSetupData->hBoldFont, TRUE);

            /* Center the wizard window */
            CenterWindow(GetParent(hwndDlg));
            break;
        }

        case WM_NOTIFY:
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;

            switch (lpnm->code)
            {
                case PSN_SETACTIVE:
                {
                    /* Only "Next" and "Cancel" for the first page and hide "Back" */
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT);
                    // PropSheet_ShowWizButtons(GetParent(hwndDlg), 0, PSWIZB_BACK);
                    ShowDlgItem(GetParent(hwndDlg), ID_WIZBACK, SW_HIDE);
                    break;
                }

                case PSN_KILLACTIVE:
                {
                    /* Show "Back" button */
                    // PropSheet_ShowWizButtons(GetParent(hwndDlg), PSWIZB_BACK, PSWIZB_BACK);
                    ShowDlgItem(GetParent(hwndDlg), ID_WIZBACK, SW_SHOW);
                    break;
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

            /* Set the options in bold */
            SetDlgItemFont(hwndDlg, IDC_INSTALL, pSetupData->hBoldFont, TRUE);
            SetDlgItemFont(hwndDlg, IDC_UPDATE, pSetupData->hBoldFont, TRUE);

            /* Check the "Install" radio button */
            CheckDlgButton(hwndDlg, IDC_INSTALL, BST_CHECKED);

            /*
             * Enable the "Update" radio button and text only if we have
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
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
                    break;

                case PSN_QUERYINITIALFOCUS:
                {
                    /* Focus on "Install ReactOS" */
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LONG_PTR)GetDlgItem(hwndDlg, IDC_INSTALL));
                    return TRUE;
                }

                case PSN_QUERYCANCEL:
                {
                    if (DisplayMessage(GetParent(hwndDlg),
                                       MB_YESNO | MB_ICONQUESTION,
                                       MAKEINTRESOURCEW(IDS_ABORTSETUP2),
                                       MAKEINTRESOURCEW(IDS_ABORTSETUP)) == IDYES)
                    {
                        /* Go to the Terminate page */
                        PropSheet_SetCurSelByID(GetParent(hwndDlg), IDD_RESTARTPAGE);
                    }

                    /* Do not close the wizard too soon */
                    SetWindowLongPtrW(hwndDlg, DWLP_MSGRESULT, TRUE);
                    return TRUE;
                }

                case PSN_WIZNEXT: /* Set the selected data */
                {
                    /*
                     * Go update only if we have available NT installations
                     * and we choose to do so.
                     */
                    if (pSetupData->NtOsInstallsList &&
                        GetNumberOfListEntries(pSetupData->NtOsInstallsList) != 0 &&
                        IsDlgButtonChecked(hwndDlg, IDC_UPDATE) == BST_CHECKED)
                    {
                        pSetupData->RepairUpdateFlag = TRUE;

                        /*
                         * Display the existing NT installations page only
                         * if we have more than one available NT installations.
                         */
                        if (GetNumberOfListEntries(pSetupData->NtOsInstallsList) > 1)
                        {
                            /* pSetupData->CurrentInstallation will be set from within IDD_UPDATEREPAIRPAGE */

                            /* Actually the best would be to dynamically insert the page only when needed */
                            SetWindowLongPtrW(hwndDlg, DWLP_MSGRESULT, IDD_UPDATEREPAIRPAGE);
                        }
                        else
                        {
                            /* Retrieve the current installation */
                            pSetupData->CurrentInstallation =
                                (PNTOS_INSTALLATION)GetListEntryData(GetCurrentListEntry(pSetupData->NtOsInstallsList));

                            SetWindowLongPtrW(hwndDlg, DWLP_MSGRESULT, IDD_DEVICEPAGE);
                        }
                    }
                    else
                    {
                        pSetupData->CurrentInstallation = NULL;
                        pSetupData->RepairUpdateFlag = FALSE;
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

PVOID
GetSelectedComboListItem(
    IN HWND hWndList)
{
    INT Index;

    Index = ComboBox_GetCurSel(hWndList);
    if (Index == CB_ERR)
        return NULL;

    return (PVOID)ComboBox_GetItemData(hWndList, Index);
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

    ListView_EnsureVisible(hWndList, CurrentEntryIndex, FALSE);
    ListView_SetItemState(hWndList, CurrentEntryIndex,
                          LVIS_FOCUSED | LVIS_SELECTED,
                          LVIS_FOCUSED | LVIS_SELECTED);
}

PVOID
GetSelectedListViewItem(
    IN HWND hWndList)
{
    INT Index;
    LVITEM item;

    Index = ListView_GetSelectionMark(hWndList);
    if (Index == LB_ERR)
        return NULL;

    item.mask = LVIF_PARAM;
    item.iItem = Index;
    ListView_GetItem(hWndList, &item);

    return (PVOID)item.lParam;
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
    PVOLINFO VolInfo = (NtOsInstall->Volume ? &NtOsInstall->Volume->Info : NULL);

    if (VolInfo && VolInfo->DriveLetter)
    {
        /* We have retrieved a partition that is mounted */
        StringCchPrintfW(Buffer, cchBufferSize,
                         L"%c:%s",
                         VolInfo->DriveLetter,
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

            /*
             * Keep the "Next" button disabled. It will be enabled only
             * when the user selects an installation to upgrade.
             */
            PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK);

            hList = GetDlgItem(hwndDlg, IDC_NTOSLIST);

            ListView_SetExtendedListViewStyleEx(hList, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

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

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_SKIPUPGRADE:
                {
                    /* Skip the upgrade and do the usual new-installation workflow */
                    pSetupData->CurrentInstallation = NULL;
                    pSetupData->RepairUpdateFlag = FALSE;
                    PropSheet_SetCurSelByID(GetParent(hwndDlg), IDD_DEVICEPAGE);
                    return TRUE;
                }
            }
            break;

        case WM_NOTIFY:
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;

            if (lpnm->idFrom == IDC_NTOSLIST && lpnm->code == LVN_ITEMCHANGED)
            {
                LPNMLISTVIEW pnmv = (LPNMLISTVIEW)lParam;

                if (pnmv->uChanged & LVIF_STATE) /* The state has changed */
                {
                    /* The item has been (de)selected */
                    if (pnmv->uNewState & (LVIS_FOCUSED | LVIS_SELECTED))
                    {
                        PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
                    }
                    else
                    {
                        /*
                         * Keep the "Next" button disabled. It will be enabled only
                         * when the user selects an installation to upgrade.
                         */
                        PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK);
                    }
                }

                break;
            }

            switch (lpnm->code)
            {
#if 0
                case PSN_SETACTIVE:
                {
                    /*
                     * Keep the "Next" button disabled. It will be enabled only
                     * when the user selects an installation to upgrade.
                     */
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK);
                    break;
                }
#endif

                case PSN_QUERYINITIALFOCUS:
                {
                    /* Give the focus on and select the first item */
                    hList = GetDlgItem(hwndDlg, IDC_NTOSLIST);
                    ListView_SetItemState(hList, 0, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LONG_PTR)hList);
                    return TRUE;
                }

                case PSN_QUERYCANCEL:
                {
                    if (DisplayMessage(GetParent(hwndDlg),
                                       MB_YESNO | MB_ICONQUESTION,
                                       MAKEINTRESOURCEW(IDS_ABORTSETUP2),
                                       MAKEINTRESOURCEW(IDS_ABORTSETUP)) == IDYES)
                    {
                        /* Go to the Terminate page */
                        PropSheet_SetCurSelByID(GetParent(hwndDlg), IDD_RESTARTPAGE);
                    }

                    /* Do not close the wizard too soon */
                    SetWindowLongPtrW(hwndDlg, DWLP_MSGRESULT, TRUE);
                    return TRUE;
                }

                case PSN_WIZNEXT: /* Set the selected data */
                {
                    /*
                     * Go update only if we have available NT installations
                     * and we choose to do so.
                     */
                    if (!pSetupData->NtOsInstallsList ||
                        GetNumberOfListEntries(pSetupData->NtOsInstallsList) == 0)
                    {
                        pSetupData->CurrentInstallation = NULL;
                        pSetupData->RepairUpdateFlag = FALSE;
                        break;
                    }

                    hList = GetDlgItem(hwndDlg, IDC_NTOSLIST);
                    SetCurrentListEntry(pSetupData->NtOsInstallsList,
                                        GetSelectedListViewItem(hList));

                    /* Retrieve the current installation */
                    pSetupData->CurrentInstallation =
                        (PNTOS_INSTALLATION)GetListEntryData(GetCurrentListEntry(pSetupData->NtOsInstallsList));

                    /* We perform an upgrade */
                    pSetupData->RepairUpdateFlag = TRUE;
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
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
                    break;

                case PSN_QUERYINITIALFOCUS:
                {
                    /* Focus on "Computer" list */
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LONG_PTR)GetDlgItem(hwndDlg, IDC_COMPUTER));
                    return TRUE;
                }

                case PSN_QUERYCANCEL:
                {
                    if (DisplayMessage(GetParent(hwndDlg),
                                       MB_YESNO | MB_ICONQUESTION,
                                       MAKEINTRESOURCEW(IDS_ABORTSETUP2),
                                       MAKEINTRESOURCEW(IDS_ABORTSETUP)) == IDYES)
                    {
                        /* Go to the Terminate page */
                        PropSheet_SetCurSelByID(GetParent(hwndDlg), IDD_RESTARTPAGE);
                    }

                    /* Do not close the wizard too soon */
                    SetWindowLongPtrW(hwndDlg, DWLP_MSGRESULT, TRUE);
                    return TRUE;
                }

                case PSN_WIZNEXT: /* Set the selected data */
                {
                    hList = GetDlgItem(hwndDlg, IDC_COMPUTER);
                    SetCurrentListEntry(pSetupData->USetupData.ComputerList,
                                        GetSelectedComboListItem(hList));

                    hList = GetDlgItem(hwndDlg, IDC_DISPLAY);
                    SetCurrentListEntry(pSetupData->USetupData.DisplayList,
                                        GetSelectedComboListItem(hList));

                    hList = GetDlgItem(hwndDlg, IDC_KEYBOARD);
                    SetCurrentListEntry(pSetupData->USetupData.KeyboardList,
                                        GetSelectedComboListItem(hList));

                    // hList = GetDlgItem(hwndDlg, IDC_KEYBOARD_LAYOUT);
                    // SetCurrentListEntry(pSetupData->USetupData.LayoutList,
                    //                     GetSelectedComboListItem(hList));

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
    static WCHAR szOrgWizNextBtnText[260]; // TODO: Make it dynamic

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
            break;
        }

        case WM_COMMAND:
        {
            if (HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDC_CONFIRM_INSTALL)
            {
                if (IsDlgButtonChecked(hwndDlg, IDC_CONFIRM_INSTALL) == BST_CHECKED)
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
                else
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK);
            }
            break;
        }

        case WM_NOTIFY:
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;

            switch (lpnm->code)
            {
                case PSN_SETACTIVE:
                {
                    WCHAR CurrentItemText[256];

                    ASSERT(InstallPartition);

                    /* Show the current selected settings */

                    // FIXME! Localize
                    if (pSetupData->RepairUpdateFlag)
                    {
                        StringCchPrintfW(CurrentItemText, ARRAYSIZE(CurrentItemText),
                                         L"Upgrading/Repairing \"%s\" from \"%s\"",
                                         pSetupData->CurrentInstallation->InstallationName,
                                         pSetupData->CurrentInstallation->VendorName);
                    }
                    else
                    {
                        StringCchCopyW(CurrentItemText, ARRAYSIZE(CurrentItemText),
                                       L"New ReactOS installation");
                    }
                    SetDlgItemTextW(hwndDlg, IDC_INSTALLTYPE, CurrentItemText);

                    SetDlgItemTextW(hwndDlg, IDC_INSTALLSOURCE, L"n/a");
                    SetDlgItemTextW(hwndDlg, IDC_ARCHITECTURE, L"n/a");

                    GetSettingDescription(GetCurrentListEntry(pSetupData->USetupData.ComputerList),
                                          CurrentItemText,
                                          ARRAYSIZE(CurrentItemText));
                    SetDlgItemTextW(hwndDlg, IDC_COMPUTER, CurrentItemText);

                    GetSettingDescription(GetCurrentListEntry(pSetupData->USetupData.DisplayList),
                                          CurrentItemText,
                                          ARRAYSIZE(CurrentItemText));
                    SetDlgItemTextW(hwndDlg, IDC_DISPLAY, CurrentItemText);

                    GetSettingDescription(GetCurrentListEntry(pSetupData->USetupData.KeyboardList),
                                          CurrentItemText,
                                          ARRAYSIZE(CurrentItemText));
                    SetDlgItemTextW(hwndDlg, IDC_KEYBOARD, CurrentItemText);

                    if (InstallVolume->Info.DriveLetter)
                    {
#if 0
                        StringCchPrintfW(CurrentItemText, ARRAYSIZE(CurrentItemText),
                                         L"%c: \x2014 %wZ",
                                         InstallVolume->Info.DriveLetter,
                                         &pSetupData->USetupData.DestinationRootPath);
#else
                        StringCchPrintfW(CurrentItemText, ARRAYSIZE(CurrentItemText),
                                         L"%c: \x2014 Harddisk %lu, Partition %lu",
                                         InstallVolume->Info.DriveLetter,
                                         InstallPartition->DiskEntry->DiskNumber,
                                         InstallPartition->OnDiskPartitionNumber);
#endif
                    }
                    else
                    {
#if 0
                        StringCchPrintfW(CurrentItemText, ARRAYSIZE(CurrentItemText),
                                         L"%wZ",
                                         &pSetupData->USetupData.DestinationRootPath);
#else
                        StringCchPrintfW(CurrentItemText, ARRAYSIZE(CurrentItemText),
                                         L"Harddisk %lu, Partition %lu",
                                         InstallPartition->DiskEntry->DiskNumber,
                                         InstallPartition->OnDiskPartitionNumber);
#endif
                    }
                    SetDlgItemTextW(hwndDlg, IDC_DESTDRIVE, CurrentItemText);

                    SetDlgItemTextW(hwndDlg, IDC_PATH,
                                    pSetupData->USetupData.InstallationDirectory
                                    /*pSetupData->USetupData.InstallPath.Buffer*/);


                    /* Change the "Next" button text to "Install" */
                    // PropSheet_SetNextText(GetParent(hwndDlg), ...);
                    GetDlgItemTextW(GetParent(hwndDlg), ID_WIZNEXT,
                                    szOrgWizNextBtnText, ARRAYSIZE(szOrgWizNextBtnText));
                    SetWindowResTextW(GetDlgItem(GetParent(hwndDlg), ID_WIZNEXT),
                                      pSetupData->hInstance,
                                      IDS_INSTALLBTN);

                    /*
                     * Keep the "Next" button disabled. It will be enabled only
                     * when the user clicks on the installation approval checkbox.
                     */
                    CheckDlgButton(hwndDlg, IDC_CONFIRM_INSTALL, BST_UNCHECKED);
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK);
                    break;
                }

                case PSN_QUERYINITIALFOCUS:
                {
                    /* Focus on the confirmation check-box */
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LONG_PTR)GetDlgItem(hwndDlg, IDC_CONFIRM_INSTALL));
                    return TRUE;
                }

                case PSN_KILLACTIVE:
                {
                    /* Restore the original "Next" button text */
                    SetDlgItemTextW(GetParent(hwndDlg), ID_WIZNEXT, szOrgWizNextBtnText);
                    break;
                }

                case PSN_QUERYCANCEL:
                {
                    if (DisplayMessage(GetParent(hwndDlg),
                                       MB_YESNO | MB_ICONQUESTION,
                                       MAKEINTRESOURCEW(IDS_ABORTSETUP2),
                                       MAKEINTRESOURCEW(IDS_ABORTSETUP)) == IDYES)
                    {
                        /* Go to the Terminate page */
                        PropSheet_SetCurSelByID(GetParent(hwndDlg), IDD_RESTARTPAGE);
                    }

                    /* Do not close the wizard too soon */
                    SetWindowLongPtrW(hwndDlg, DWLP_MSGRESULT, TRUE);
                    return TRUE;
                }

                default:
                    break;
            }
            break;
        }

        default:
            break;
    }

    return FALSE;
}


typedef struct _FSVOL_CONTEXT
{
    PSETUPDATA pSetupData;
    // PAGE_NUMBER NextPageOnAbort;
} FSVOL_CONTEXT, *PFSVOL_CONTEXT;

static
BOOLEAN
NTAPI
FormatCallback(
    _In_ CALLBACKCOMMAND Command,
    _In_ ULONG Modifier,
    _In_ PVOID Argument)
{
    switch (Command)
    {
        case PROGRESS:
        {
            PULONG Percent = (PULONG)Argument;
            DPRINT("%lu percent completed\n", *Percent);
            SendMessageW(UiContext.hWndProgress, PBM_SETPOS, *Percent, 0);
            break;
        }

#if 0
        case OUTPUT:
        {
            PTEXTOUTPUT output = (PTEXTOUTPUT)Argument;
            DPRINT("%s\n", output->Output);
            break;
        }
#endif

        case DONE:
        {
#if 0
            PBOOLEAN Success = (PBOOLEAN)Argument;
            if (*Success == FALSE)
            {
                DPRINT("FormatEx was unable to complete successfully.\n\n");
            }
#endif
            DPRINT("Done\n");
            break;
        }

        default:
            DPRINT("Unknown callback %lu\n", (ULONG)Command);
            break;
    }

    return TRUE;
}

static
BOOLEAN
NTAPI
ChkdskCallback(
    _In_ CALLBACKCOMMAND Command,
    _In_ ULONG Modifier,
    _In_ PVOID Argument)
{
    switch (Command)
    {
        default:
            DPRINT("Unknown callback %lu\n", (ULONG)Command);
            break;
    }

    return TRUE;
}

// PFSVOL_CALLBACK
static FSVOL_OP
CALLBACK
FsVolCallback(
    _In_opt_ PVOID Context,
    _In_ FSVOLNOTIFY FormatStatus,
    _In_ ULONG_PTR Param1,
    _In_ ULONG_PTR Param2)
{
    PFSVOL_CONTEXT FsVolContext = (PFSVOL_CONTEXT)Context;

    switch (FormatStatus)
    {
    // FIXME: Deprecate!
    case ChangeSystemPartition:
    {
        // PPARTENTRY SystemPartition = (PPARTENTRY)Param1;

        // FsVolContext->NextPageOnAbort = SELECT_PARTITION_PAGE;
        // if (ChangeSystemPartitionPage(Ir, SystemPartition))
        //     return FSVOL_DOIT;
        return FSVOL_ABORT;
    }

    case FSVOLNOTIFY_PARTITIONERROR:
    {
        switch (Param1)
        {
        case STATUS_PARTITION_FAILURE:
        {
            // ERROR_WRITE_PTABLE
            DisplayError(NULL,
                         0, // Default to "Error"
                         IDS_ERROR_WRITE_PTABLE);
            // FsVolContext->NextPageOnAbort = QUIT_PAGE;
            // TODO: Go back to the partitioning page?
            break;
        }

        case ERROR_SYSTEM_PARTITION_NOT_FOUND:
        {
            /* FIXME: improve the error dialog */
            //
            // Error dialog should say that we cannot find a suitable
            // system partition and create one on the system. At this point,
            // it may be nice to ask the user whether he wants to continue,
            // or use an external drive as the system drive/partition
            // (e.g. floppy, USB drive, etc...)
            //
            DisplayError(NULL,
                         0, // Default to "Error"
                         IDS_ERROR_SYSTEM_PARTITION);
            // FsVolContext->NextPageOnAbort = SELECT_PARTITION_PAGE;
            // TODO: Go back to the partitioning page
            break;
        }

        default:
            break;
        }
        return FSVOL_ABORT;
    }

    case FSVOLNOTIFY_STARTQUEUE:
    case FSVOLNOTIFY_ENDQUEUE:
        // NOTE: If needed, clear progress gauges.
        return FSVOL_DOIT;

    case FSVOLNOTIFY_STARTSUBQUEUE:
    {
        if ((FSVOL_OP)Param1 == FSVOL_FORMAT)
        {
            /*
             * In case we just repair an existing installation, or make
             * an unattended setup without formatting, just go to the
             * filesystem check step.
             */
            if (FsVolContext->pSetupData->RepairUpdateFlag)
                return FSVOL_SKIP; /** HACK!! **/

            if (IsUnattendedSetup && !FsVolContext->pSetupData->USetupData.FormatPartition)
                return FSVOL_SKIP; /** HACK!! **/

            /* Set status text */
            SetDlgItemTextW(UiContext.hwndDlg, IDC_ITEM, L"");
        }
        else
        if ((FSVOL_OP)Param1 == FSVOL_CHECK)
        {
            /* Set status text */
            SetDlgItemTextW(UiContext.hwndDlg, IDC_ITEM, L"");

            /* Filechecking step: set progress marquee style and start it up */
            UiContext.dwPbStyle = GetWindowLongPtrW(UiContext.hWndProgress, GWL_STYLE);
            SetWindowLongPtrW(UiContext.hWndProgress, GWL_STYLE, UiContext.dwPbStyle | PBS_MARQUEE);
            SendMessageW(UiContext.hWndProgress, PBM_SETMARQUEE, TRUE, 0);
        }

        return FSVOL_DOIT;
    }

    case FSVOLNOTIFY_ENDSUBQUEUE:
    {
        if ((FSVOL_OP)Param1 == FSVOL_CHECK)
        {
            /* File-checking finished: stop the progress bar and restore its style */
            SendMessageW(UiContext.hWndProgress, PBM_SETMARQUEE, FALSE, 0);
            SetWindowLongPtrW(UiContext.hWndProgress, GWL_STYLE, UiContext.dwPbStyle);
        }
        return 0;
    }

    case FSVOLNOTIFY_FORMATERROR:
    {
        PFORMAT_VOLUME_INFO FmtInfo = (PFORMAT_VOLUME_INFO)Param1;

        // FIXME: See also FSVOLNOTIFY_PARTITIONERROR
        if (FmtInfo->ErrorStatus == STATUS_PARTITION_FAILURE)
        {
            // ERROR_WRITE_PTABLE
            DisplayError(NULL,
                         0, // Default to "Error"
                         IDS_ERROR_WRITE_PTABLE);
            // FsVolContext->NextPageOnAbort = QUIT_PAGE;
            // TODO: Go back to the partitioning page?
            return FSVOL_ABORT;
        }
        else
        if (FmtInfo->ErrorStatus == STATUS_UNRECOGNIZED_VOLUME)
        {
            /* FIXME: show an error dialog */
            // MUIDisplayError(ERROR_FORMATTING_PARTITION, Ir, POPUP_WAIT_ANY_KEY, PathBuffer);
            DisplayError(NULL,
                         0, // Default to "Error"
                         IDS_ERROR_FORMAT_UNRECOGNIZED_VOLUME);
            // FsVolContext->NextPageOnAbort = QUIT_PAGE;
            return FSVOL_ABORT;
        }
        else
        if (FmtInfo->ErrorStatus == STATUS_NOT_SUPPORTED)
        {
            INT nRet;

            nRet = DisplayMessage(NULL, MB_ICONERROR | MB_OKCANCEL,
                                  NULL, // Default to "Error"
                                  MAKEINTRESOURCEW(IDS_ERROR_COULD_NOT_FORMAT),
                                  FmtInfo->FileSystemName);
            if (nRet == IDCANCEL)
            {
                // FsVolContext->NextPageOnAbort = QUIT_PAGE;
                return FSVOL_ABORT;
            }
            else if (nRet == IDOK)
            {
                return FSVOL_RETRY;
            }
        }
        else if (!NT_SUCCESS(FmtInfo->ErrorStatus))
        {
            ASSERT(*FmtInfo->Volume->Info.DeviceName);

            DPRINT1("FormatPartition() failed with status 0x%08lx\n", FmtInfo->ErrorStatus);

            // ERROR_FORMATTING_PARTITION
            DisplayError(NULL,
                         0, // Default to "Error"
                         IDS_ERROR_FORMATTING_PARTITION,
                         FmtInfo->Volume->Info.DeviceName);
            // FsVolContext->NextPageOnAbort = QUIT_PAGE;
            return FSVOL_ABORT;
        }

        return FSVOL_RETRY;
    }

    case FSVOLNOTIFY_CHECKERROR:
    {
        PCHECK_VOLUME_INFO ChkInfo = (PCHECK_VOLUME_INFO)Param1;

        if (ChkInfo->ErrorStatus == STATUS_NOT_SUPPORTED)
        {
            INT nRet;

            nRet = DisplayMessage(NULL, MB_ICONERROR | MB_OKCANCEL,
                                  NULL, // Default to "Error"
                                  MAKEINTRESOURCEW(IDS_ERROR_COULD_NOT_CHECK),
                                  ChkInfo->Volume->Info.FileSystem);
            if (nRet == IDCANCEL)
            {
                // FsVolContext->NextPageOnAbort = QUIT_PAGE;
                return FSVOL_ABORT;
            }
            else if (nRet == IDOK)
            {
                return FSVOL_SKIP;
            }
        }
        else if (!NT_SUCCESS(ChkInfo->ErrorStatus))
        {
            DPRINT1("ChkdskPartition() failed with status 0x%08lx\n", ChkInfo->ErrorStatus);

            DisplayError(NULL,
                         0, // Default to "Error"
                         IDS_ERROR_CHECKING_PARTITION,
                         ChkInfo->ErrorStatus);
            return FSVOL_SKIP;
        }

        return FSVOL_SKIP;
    }

    case FSVOLNOTIFY_STARTFORMAT:
    {
        PFORMAT_VOLUME_INFO FmtInfo = (PFORMAT_VOLUME_INFO)Param1;
        PVOL_CREATE_INFO VolCreate;

        ASSERT((FSVOL_OP)Param2 == FSVOL_FORMAT);

        /* Find the volume info in the partition TreeList UI.
         * If none, don't format it. */
        VolCreate = FindVolCreateInTreeByVolume(UiContext.hPartList,
                                                FmtInfo->Volume);
        if (!VolCreate)
            return FSVOL_SKIP;
        ASSERT(VolCreate->Volume == FmtInfo->Volume);

        /* If there is no formatting information, skip it */
        if (!*VolCreate->FileSystemName)
            return FSVOL_SKIP;

        ASSERT(*FmtInfo->Volume->Info.DeviceName);

        /* Set status text */
        if (FmtInfo->Volume->Info.DriveLetter)
        {
            SetWindowResPrintfW(GetDlgItem(UiContext.hwndDlg, IDC_ITEM),
                                SetupData.hInstance,
                                IDS_FORMATTING_PROGRESS1, // L"Formatting volume %c: (%s) in %s..."
                                FmtInfo->Volume->Info.DriveLetter,
                                FmtInfo->Volume->Info.DeviceName,
                                VolCreate->FileSystemName);
        }
        else
        {
            SetWindowResPrintfW(GetDlgItem(UiContext.hwndDlg, IDC_ITEM),
                                SetupData.hInstance,
                                IDS_FORMATTING_PROGRESS2, // L"Formatting volume %s in %s..."
                                FmtInfo->Volume->Info.DeviceName,
                                VolCreate->FileSystemName);
        }

        // StartFormat(FmtInfo, FileSystemList->Selected);
        FmtInfo->FileSystemName = VolCreate->FileSystemName;
        FmtInfo->MediaFlag = VolCreate->MediaFlag;
        FmtInfo->Label = VolCreate->Label;
        FmtInfo->QuickFormat = VolCreate->QuickFormat;
        FmtInfo->ClusterSize = VolCreate->ClusterSize;
        FmtInfo->Callback = FormatCallback;

        /* Set up the progress bar */
        SendMessageW(UiContext.hWndProgress,
                     PBM_SETRANGE, 0, MAKELPARAM(0, 100));
        SendMessageW(UiContext.hWndProgress,
                     PBM_SETPOS, 0, 0);

        return FSVOL_DOIT;
    }

    case FSVOLNOTIFY_ENDFORMAT:
    {
        PFORMAT_VOLUME_INFO FmtInfo = (PFORMAT_VOLUME_INFO)Param1;

        // EndFormat(FmtInfo->ErrorStatus);
        if (FmtInfo->FileSystemName)
            *(PWSTR)FmtInfo->FileSystemName = UNICODE_NULL; // FIXME: HACK!

        // /* Reset the file system list */
        // ResetFileSystemList();
        return 0;
    }

    case FSVOLNOTIFY_STARTCHECK:
    {
        PCHECK_VOLUME_INFO ChkInfo = (PCHECK_VOLUME_INFO)Param1;
        PVOL_CREATE_INFO VolCreate;

        ASSERT((FSVOL_OP)Param2 == FSVOL_CHECK);

        /* Find the volume info in the partition TreeList UI.
         * If none, don't check it. */
        VolCreate = FindVolCreateInTreeByVolume(UiContext.hPartList,
                                                ChkInfo->Volume);
        if (!VolCreate)
            return FSVOL_SKIP;
        ASSERT(VolCreate->Volume == ChkInfo->Volume);

        ASSERT(*ChkInfo->Volume->Info.DeviceName);

        /* Set status text */
        if (ChkInfo->Volume->Info.DriveLetter)
        {
            SetWindowResPrintfW(GetDlgItem(UiContext.hwndDlg, IDC_ITEM),
                                SetupData.hInstance,
                                IDS_CHECKING_PROGRESS1, // L"Checking volume %c: (%s)..."
                                ChkInfo->Volume->Info.DriveLetter,
                                ChkInfo->Volume->Info.DeviceName);
        }
        else
        {
            SetWindowResPrintfW(GetDlgItem(UiContext.hwndDlg, IDC_ITEM),
                                SetupData.hInstance,
                                IDS_CHECKING_PROGRESS2, // L"Checking volume %s..."
                                ChkInfo->Volume->Info.DeviceName);
        }

        // StartCheck(ChkInfo);
        // TODO: Think about which values could be defaulted...
        ChkInfo->FixErrors = TRUE;
        ChkInfo->Verbose = FALSE;
        ChkInfo->CheckOnlyIfDirty = TRUE;
        ChkInfo->ScanDrive = FALSE;
        ChkInfo->Callback = ChkdskCallback;

        return FSVOL_DOIT;
    }

    case FSVOLNOTIFY_ENDCHECK:
    {
        // PCHECK_VOLUME_INFO ChkInfo = (PCHECK_VOLUME_INFO)Param1;
        // EndCheck(ChkInfo->ErrorStatus);
        return 0;
    }
    }

    return 0;
}



typedef struct _COPYCONTEXT
{
    PSETUPDATA pSetupData;
    ULONG TotalOperations;
    ULONG CompletedOperations;
} COPYCONTEXT, *PCOPYCONTEXT;

static UINT
CALLBACK
FileCopyCallback(PVOID Context,
                 UINT Notification,
                 UINT_PTR Param1,
                 UINT_PTR Param2)
{
    PCOPYCONTEXT CopyContext = (PCOPYCONTEXT)Context;
    PFILEPATHS_W FilePathInfo;
    PCWSTR SrcFileName, DstFileName;

    WaitForSingleObject(CopyContext->pSetupData->hHaltInstallEvent, INFINITE);
    if (CopyContext->pSetupData->bStopInstall)
        return FILEOP_ABORT; // Stop committing files

    switch (Notification)
    {
        case SPFILENOTIFY_STARTSUBQUEUE:
        {
            CopyContext->TotalOperations = (ULONG)Param2;
            CopyContext->CompletedOperations = 0;

            /* Set up the progress bar */
            SendMessageW(UiContext.hWndProgress,
                         PBM_SETRANGE, 0,
                         MAKELPARAM(0, CopyContext->TotalOperations));
            SendMessageW(UiContext.hWndProgress,
                         PBM_SETSTEP, 1, 0);
            SendMessageW(UiContext.hWndProgress,
                         PBM_SETPOS, 0, 0);
            break;
        }

        case SPFILENOTIFY_STARTDELETE:
        case SPFILENOTIFY_STARTRENAME:
        case SPFILENOTIFY_STARTCOPY:
        {
            FilePathInfo = (PFILEPATHS_W)Param1;

            if (Notification == SPFILENOTIFY_STARTDELETE)
            {
                /* Display delete message */
                ASSERT(Param2 == FILEOP_DELETE);

                DstFileName = wcsrchr(FilePathInfo->Target, L'\\');
                if (DstFileName) ++DstFileName;
                else DstFileName = FilePathInfo->Target;

                SetWindowResPrintfW(UiContext.hWndItem,
                                    SetupData.hInstance,
                                    IDS_DELETING, // STRING_DELETING
                                    DstFileName);
            }
            else if (Notification == SPFILENOTIFY_STARTRENAME)
            {
                UINT uMsgID;

                /* Display move/rename message */
                ASSERT(Param2 == FILEOP_RENAME);

                SrcFileName = wcsrchr(FilePathInfo->Source, L'\\');
                if (SrcFileName) ++SrcFileName;
                else SrcFileName = FilePathInfo->Source;

                DstFileName = wcsrchr(FilePathInfo->Target, L'\\');
                if (DstFileName) ++DstFileName;
                else DstFileName = FilePathInfo->Target;

                if (!_wcsicmp(SrcFileName, DstFileName))
                    uMsgID = IDS_MOVING; // STRING_MOVING
                else
                    uMsgID = IDS_RENAMING; // STRING_RENAMING
                SetWindowResPrintfW(UiContext.hWndItem,
                                    SetupData.hInstance,
                                    uMsgID,
                                    SrcFileName, DstFileName);
            }
            else if (Notification == SPFILENOTIFY_STARTCOPY)
            {
                /* Display copy message */
                ASSERT(Param2 == FILEOP_COPY);

                DstFileName = wcsrchr(FilePathInfo->Target, L'\\');
                if (DstFileName) ++DstFileName;
                else DstFileName = FilePathInfo->Target;

                SetWindowResPrintfW(UiContext.hWndItem,
                                    SetupData.hInstance,
                                    IDS_COPYING, // STRING_COPYING
                                    DstFileName);
            }
            break;
        }

        case SPFILENOTIFY_COPYERROR:
        {
            FilePathInfo = (PFILEPATHS_W)Param1;

            DPRINT1("An error happened while trying to copy file '%S' (error 0x%08lx), skipping it...\n",
                    FilePathInfo->Target, FilePathInfo->Win32Error);
            return FILEOP_SKIP;
        }

        case SPFILENOTIFY_ENDDELETE:
        case SPFILENOTIFY_ENDRENAME:
        case SPFILENOTIFY_ENDCOPY:
        {
            CopyContext->CompletedOperations++;

            /* SYSREG checkpoint */
            if (CopyContext->TotalOperations >> 1 == CopyContext->CompletedOperations)
                DPRINT1("CHECKPOINT:HALF_COPIED\n");

            SendMessageW(UiContext.hWndProgress, PBM_STEPIT, 0, 0);
            break;
        }
    }

    return FILEOP_DOIT;
}

static VOID
__cdecl
RegistryStatus(IN REGISTRY_STATUS RegStatus, ...)
{
    /* WARNING: Please keep this lookup table in sync with the resources! */
    static const UINT StringIDs[] =
    {
        IDS_REG_DONE,                   /* Success */
        IDS_REG_REGHIVEUPDATE,          /* RegHiveUpdate */
        IDS_REG_IMPORTFILE,             /* ImportRegHive */
        IDS_REG_DISPLAYSETTINGSUPDATE,  /* DisplaySettingsUpdate */
        IDS_REG_LOCALESETTINGSUPDATE,   /* LocaleSettingsUpdate */
        IDS_REG_ADDKBLAYOUTS,           /* KeybLayouts */
        IDS_REG_KEYBOARDSETTINGSUPDATE, /* KeybSettingsUpdate */
        IDS_REG_CODEPAGEINFOUPDATE,     /* CodePageInfoUpdate */
    };

    if (RegStatus < _countof(StringIDs))
    {
        va_list args;
        va_start(args, RegStatus);
        SetWindowResPrintfVW(UiContext.hWndItem, SetupData.hInstance, StringIDs[RegStatus], args);
        va_end(args);
    }
    else
    {
        SetWindowResPrintfW(UiContext.hWndItem, SetupData.hInstance, IDS_REG_UNKNOWN, RegStatus);
    }

    SendMessageW(UiContext.hWndProgress, PBM_STEPIT, 0, 0);
}

/**
 * @brief
 * Enables or disables the Cancel and the Close title-bar
 * property-sheet window buttons.
 **/
VOID
PropSheet_SetCloseCancel(
    _In_ HWND hWndWiz,
    _In_ BOOL Enable)
{
    EnableDlgItem(hWndWiz, IDCANCEL, Enable);
    // ShowDlgItem(hWndWiz, IDCANCEL, Enable ? SW_SHOW : SW_HIDE);
    EnableMenuItem(GetSystemMenu(hWndWiz, FALSE),
                   SC_CLOSE,
                   MF_BYCOMMAND | (Enable ? MF_ENABLED : MF_GRAYED));
}

static DWORD
WINAPI
PrepareAndDoCopyThread(
    IN LPVOID Param)
{
    PSETUPDATA pSetupData;
    HWND hwndDlg = (HWND)Param;
    HWND hWndProgress;
    LONG_PTR dwStyle;
    ERROR_NUMBER ErrorNumber;
    BOOLEAN Success;
    NTSTATUS Status;
    FSVOL_CONTEXT FsVolContext;
    COPYCONTEXT CopyContext;
    WCHAR PathBuffer[RTL_NUMBER_OF_FIELD(PARTENTRY, DeviceName) + 1];

    /* Retrieve pointer to the global setup data */
    pSetupData = (PSETUPDATA)GetWindowLongPtrW(hwndDlg, GWLP_USERDATA);

    /* Get the progress handle */
    hWndProgress = GetDlgItem(hwndDlg, IDC_PROCESSPROGRESS);

    /* Setup global UI context */
    UiContext.hwndDlg = hwndDlg;
    UiContext.hWndItem = GetDlgItem(hwndDlg, IDC_ITEM);
    UiContext.hWndProgress = hWndProgress;
    UiContext.dwPbStyle = 0;


    /* Disable the Close/Cancel buttons during all partition operations */
    // TODO: Consider, alternatively, to just show an info-box saying
    // that the installation process cannot be canceled at this stage?
    // PropSheet_SetWizButtons(GetParent(hwndDlg), 0);
    PropSheet_SetCloseCancel(GetParent(hwndDlg), FALSE);


    /*
     * Find/Set the system partition, and apply all pending partition operations.
     */

    /* Create context for the volume/partition operations */
    FsVolContext.pSetupData = pSetupData;

    /* Set status text */
    SetWindowResTextW(GetDlgItem(hwndDlg, IDC_ACTIVITY),
                      pSetupData->hInstance,
                      IDS_CONFIG_SYSTEM_PARTITION);
    SetDlgItemTextW(hwndDlg, IDC_ITEM, L"");

    /* Find or set the active system partition before starting formatting */
    Success = InitSystemPartition(pSetupData->PartitionList,
                                  InstallPartition,
                                  &SystemPartition,
                                  FsVolCallback,
                                  &FsVolContext);
    // if (!Success)
    //     return FsVolContext.NextPageOnAbort;
    //
    // FIXME?? If cannot use any system partition, install FreeLdr on floppy / removable media??
    //
    if (!Success)
    {
        /* Display an error if an unexpected failure happened */
        MessageBoxW(GetParent(hwndDlg), L"Failed to find or set the system partition!", L"Error", MB_ICONERROR);

        /* Re-enable the Close/Cancel buttons */
        PropSheet_SetCloseCancel(GetParent(hwndDlg), TRUE);

        /*
         * We failed due to an unexpected error, keep on the copy page to view the current state,
         * but enable the "Next" button to allow the user to continue to the terminate page.
         */
        PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT);
        return 1;
    }


    /* Set status text */
    SetWindowResTextW(GetDlgItem(hwndDlg, IDC_ACTIVITY),
                      pSetupData->hInstance,
                      IDS_PREPARE_PARTITIONS);
    SetDlgItemTextW(hwndDlg, IDC_ITEM, L"");

    /* Apply all pending operations on partitions: formatting and checking */
    Success = FsVolCommitOpsQueue(pSetupData->PartitionList,
                                  SystemVolume,
                                  InstallVolume,
                                  FsVolCallback,
                                  &FsVolContext);
    if (!Success)
    {
        /* Display an error if an unexpected failure happened */
        MessageBoxW(GetParent(hwndDlg), L"Failed to prepare the partitions!", L"Error", MB_ICONERROR);

        /* Re-enable the Close/Cancel buttons */
        PropSheet_SetCloseCancel(GetParent(hwndDlg), TRUE);

        /*
         * We failed due to an unexpected error, keep on the copy page to view the current state,
         * but enable the "Next" button to allow the user to continue to the terminate page.
         */
        PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT);
        return 1;
    }


    /* Re-enable the Close/Cancel buttons */
    PropSheet_SetCloseCancel(GetParent(hwndDlg), TRUE);



    /* Re-calculate the final destination paths */
    ASSERT(InstallPartition);
    Status = InitDestinationPaths(&pSetupData->USetupData,
                                  NULL, // pSetupData->USetupData.InstallationDirectory,
                                  InstallVolume);
    if (!NT_SUCCESS(Status))
    {
        DisplayMessage(GetParent(hwndDlg), MB_ICONERROR, L"Error", L"InitDestinationPaths() failed with status 0x%08lx\n", Status);

        /*
         * We failed due to an unexpected error, keep on the copy page to view the current state,
         * but enable the "Next" button to allow the user to continue to the terminate page.
         */
        PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT);
        return 1;
    }



    /*
     * Preparation of the list of files to be copied
     */

    /* Set status text */
    SetWindowResTextW(GetDlgItem(hwndDlg, IDC_ACTIVITY),
                      pSetupData->hInstance,
                      IDS_PREPARE_FILES);
    SetDlgItemTextW(hwndDlg, IDC_ITEM, L"");

    /* Set progress marquee style and start it up */
    dwStyle = GetWindowLongPtrW(hWndProgress, GWL_STYLE);
    SetWindowLongPtrW(hWndProgress, GWL_STYLE, dwStyle | PBS_MARQUEE);
    SendMessageW(hWndProgress, PBM_SETMARQUEE, TRUE, 0);

    /* Prepare the list of files */
    /* ErrorNumber = */ Success = PrepareFileCopy(&pSetupData->USetupData, NULL);

    /* Stop progress and restore its style */
    SendMessageW(hWndProgress, PBM_SETMARQUEE, FALSE, 0);
    SetWindowLongPtrW(hWndProgress, GWL_STYLE, dwStyle);

    if (/*ErrorNumber != ERROR_SUCCESS*/ !Success)
    {
        /* Display an error only if an unexpected failure happened, and not because the user cancelled the installation */
        if (!pSetupData->bStopInstall)
            MessageBoxW(GetParent(hwndDlg), L"Failed to prepare the list of files!", L"Error", MB_ICONERROR);

        /*
         * If we failed due to an unexpected error, keep on the copy page to view the current state,
         * but enable the "Next" button to allow the user to continue to the terminate page.
         * Otherwise we have been cancelled by the user, who has already switched to the Terminate page.
         */
        if (!pSetupData->bStopInstall)
            PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT);
        return 1;
    }


    /*
     * Perform the file copy
     */

    /* Set status text */
    SetWindowResTextW(GetDlgItem(hwndDlg, IDC_ACTIVITY),
                      pSetupData->hInstance,
                      IDS_COPYING_FILES);
    SetDlgItemTextW(hwndDlg, IDC_ITEM, L"");

    /* Create context for the copy process */
    CopyContext.pSetupData = pSetupData;
    CopyContext.TotalOperations = 0;
    CopyContext.CompletedOperations = 0;

    /* Do the file copying - The callback handles whether or not we should stop file copying */
    if (!DoFileCopy(&pSetupData->USetupData, FileCopyCallback, &CopyContext))
    {
        /* Display an error only if an unexpected failure happened, and not because the user cancelled the installation */
        if (!pSetupData->bStopInstall)
            MessageBoxW(GetParent(hwndDlg), L"Failed to copy the files!", L"Error", MB_ICONERROR);

        /*
         * If we failed due to an unexpected error, keep on the copy page to view the current state,
         * but enable the "Next" button to allow the user to continue to the terminate page.
         * Otherwise we have been cancelled by the user, who has already switched to the Terminate page.
         */
        if (!pSetupData->bStopInstall)
            PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT);
        return 1;
    }

    // /* Set status text */
    // SetWindowResTextW(GetDlgItem(hwndDlg, IDC_ACTIVITY),
    //                   pSetupData->hInstance,
    //                   IDS_INSTALL_FINALIZE);
    // SetDlgItemTextW(hwndDlg, IDC_ITEM, L"");

    /* Create the $winnt$.inf file */
    InstallSetupInfFile(&pSetupData->USetupData);


    /*
     * Create or update the registry hives
     */

    /* Set status text */
    SetWindowResTextW(GetDlgItem(hwndDlg, IDC_ACTIVITY),
                      pSetupData->hInstance,
                      pSetupData->RepairUpdateFlag ? IDS_UPDATE_REGISTRY
                                                   : IDS_CREATE_REGISTRY);
    SetDlgItemTextW(hwndDlg, IDC_ITEM, L"");

    /* Set up the progress bar */
    SendMessageW(hWndProgress,
                 PBM_SETRANGE, 0,
                 MAKELPARAM(0, 8)); // FIXME: hardcoded number of steps, see StringIDs[] array in RegistryStatus()
    SendMessageW(hWndProgress,
                 PBM_SETSTEP, 1, 0);
    SendMessageW(hWndProgress,
                 PBM_SETPOS, 0, 0);

    ErrorNumber = UpdateRegistry(&pSetupData->USetupData,
                                 pSetupData->RepairUpdateFlag,
                                 pSetupData->PartitionList,
                                 InstallVolume->Info.DriveLetter,
                                 pSetupData->SelectedLanguageId,
                                 RegistryStatus,
                                 NULL /* SubstSettings */);
    DBG_UNREFERENCED_PARAMETER(ErrorNumber);
    SendMessageW(UiContext.hWndProgress, PBM_SETPOS, 100, 0);

    /*
     * And finally, install the bootloader
     */

    /* Set status text */
    SetWindowResTextW(GetDlgItem(hwndDlg, IDC_ACTIVITY),
                      pSetupData->hInstance,
                      IDS_INSTALL_BOOTLOADER);
    SetDlgItemTextW(hwndDlg, IDC_ITEM, L"");

    RtlFreeUnicodeString(&pSetupData->USetupData.SystemRootPath);
    StringCchPrintfW(PathBuffer, _countof(PathBuffer),
                     L"%s\\", SystemPartition->DeviceName);
    RtlCreateUnicodeString(&pSetupData->USetupData.SystemRootPath, PathBuffer);
    DPRINT1("SystemRootPath: %wZ\n", &pSetupData->USetupData.SystemRootPath);

    switch (pSetupData->USetupData.BootLoaderLocation)
    {
        /* Install on removable disk */
        case 1:
        {
            // TODO: So far SETUP only supports the 1st floppy.
            // Use a simple UI like comdlg32's DlgDirList* to show
            // a list of drives that the user could select.
            static const UNICODE_STRING FloppyDrive = RTL_CONSTANT_STRING(L"\\Device\\Floppy0\\");
            static const WCHAR DriveLetter = L'A';

            INT nRet;
        RetryCancel:
            nRet = DisplayMessage(GetParent(hwndDlg),
                                  MB_ICONINFORMATION | MB_OKCANCEL,
                                  L"Bootloader installation",
                                  L"Please insert a blank floppy disk in drive %c: .\n"
                                  L"All data in the floppy disk will be erased!\n"
                                  L"\nClick on OK to continue."
                                  L"\nClick on CANCEL to skip bootloader installation.",
                                  DriveLetter);
            if (nRet != IDOK)
                break; /* Skip installation */

        Retry:
            Status = InstallBootcodeToRemovable(pSetupData->USetupData.ArchType,
                                                &FloppyDrive,
                                                &pSetupData->USetupData.SourceRootPath,
                                                &pSetupData->USetupData.DestinationArcPath);
            if (Status == STATUS_SUCCESS)
                break; /* Successful installation */

            if (Status == STATUS_DEVICE_NOT_READY)
            {
                // ERROR_NO_FLOPPY
                nRet = DisplayMessage(GetParent(hwndDlg),
                                      MB_ICONWARNING | MB_RETRYCANCEL,
                                      NULL, // Default to "Error"
                                      L"No disk detected in drive %c: .",
                                      DriveLetter);
                if (nRet == IDRETRY)
                    goto Retry;
            }
            else if ((Status == ERROR_WRITE_BOOT) ||
                     (Status == ERROR_INSTALL_BOOTCODE))
            {
                /* Error when writing the boot code */
                DisplayError(GetParent(hwndDlg),
                             0, // Default to "Error"
                             IDS_ERROR_INSTALL_BOOTCODE_REMOVABLE);
            }
            else if (!NT_SUCCESS(Status))
            {
                /* Any other NTSTATUS failure code */
                DPRINT1("InstallBootcodeToRemovable() failed: Status 0x%lx\n", Status);
                DisplayError(GetParent(hwndDlg),
                             0, // Default to "Error"
                             IDS_ERROR_BOOTLDR_FAILED,
                             Status);
            }
            goto RetryCancel;
        }

        /* Install on hard-disk */
        case 2: // System partition / MBR and VBR (on BIOS-based PC)
        case 3: // VBR only (on BIOS-based PC)
        {
            /* Copy FreeLoader to the disk and save the boot entries */
            Status = InstallBootManagerAndBootEntries(
                        pSetupData->USetupData.ArchType,
                        &pSetupData->USetupData.SystemRootPath,
                        &pSetupData->USetupData.SourceRootPath,
                        &pSetupData->USetupData.DestinationArcPath,
                        (pSetupData->USetupData.BootLoaderLocation == 2)
                           ? 1 /* Install MBR and VBR */
                           : 0 /* Install VBR only */);
            if (Status == STATUS_SUCCESS)
                break; /* Successful installation */

            if (Status == ERROR_WRITE_BOOT)
            {
                /* Error when writing the VBR */
                DisplayError(GetParent(hwndDlg),
                             0, // Default to "Error"
                             IDS_ERROR_WRITE_BOOT,
                             SystemVolume->Info.FileSystem);
            }
            else if (Status == ERROR_INSTALL_BOOTCODE)
            {
                /* Error when writing the MBR */
                DisplayError(GetParent(hwndDlg),
                             0, // Default to "Error"
                             IDS_ERROR_INSTALL_BOOTCODE,
                             L"MBR");
            }
            else if (Status == STATUS_NOT_SUPPORTED)
            {
                DisplayError(GetParent(hwndDlg),
                             0, // Default to "Error"
                             IDS_ERROR_BOOTLDR_ARCH_UNSUPPORTED);
            }
            else if (!NT_SUCCESS(Status))
            {
                /* Any other NTSTATUS failure code */
                DPRINT1("InstallBootManagerAndBootEntries() failed: Status 0x%lx\n", Status);
                DisplayError(GetParent(hwndDlg),
                             0, // Default to "Error"
                             IDS_ERROR_BOOTLDR_FAILED,
                             Status);
            }
            break;
        }

        /* Skip installation */
        case 0:
        default:
            break;
    }


    /* We are done! Switch to the Terminate page */
    PropSheet_SetCurSelByID(GetParent(hwndDlg), IDD_RESTARTPAGE);
    return 0;
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
        {
            /* Save pointer to the global setup data */
            pSetupData = (PSETUPDATA)((LPPROPSHEETPAGE)lParam)->lParam;
            SetWindowLongPtrW(hwndDlg, GWLP_USERDATA, (DWORD_PTR)pSetupData);

            /* Reset status text */
            SetDlgItemTextW(hwndDlg, IDC_ACTIVITY, L"");
            SetDlgItemTextW(hwndDlg, IDC_ITEM, L"");
            break;
        }

        case WM_NOTIFY:
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;

            switch (lpnm->code)
            {
                case PSN_SETACTIVE:
                {
                    /* Create the file-copy halt (manual-reset) event */
                    pSetupData->hHaltInstallEvent = CreateEventW(NULL, TRUE, TRUE, NULL);
                    if (!pSetupData->hHaltInstallEvent)
                        break;
                    pSetupData->bStopInstall = FALSE;

                    /* Start the prepare-and-copy files thread */
                    pSetupData->hInstallThread =
                        CreateThread(NULL, 0,
                                     PrepareAndDoCopyThread,
                                     (PVOID)hwndDlg,
                                     CREATE_SUSPENDED,
                                     NULL);
                    if (!pSetupData->hInstallThread)
                    {
                        CloseHandle(pSetupData->hHaltInstallEvent);
                        pSetupData->hHaltInstallEvent = NULL;

                        MessageBoxW(GetParent(hwndDlg), L"Cannot create the prepare-and-copy files thread!", L"Error", MB_ICONERROR);
                        break;
                    }

                    /* Disable all buttons during installation process - buttons will be reenabled by the installation thread */
                    PropSheet_SetWizButtons(GetParent(hwndDlg), 0);

                    /* Resume the installation thread */
                    ResumeThread(pSetupData->hInstallThread);

                    break;
                }

                case PSN_QUERYCANCEL:
                {
                    /* Halt the on-going file copy */
                    ResetEvent(pSetupData->hHaltInstallEvent);

                    if (DisplayMessage(GetParent(hwndDlg),
                                       MB_YESNO | MB_ICONQUESTION,
                                       MAKEINTRESOURCEW(IDS_ABORTSETUP2),
                                       MAKEINTRESOURCEW(IDS_ABORTSETUP)) == IDYES)
                    {
                        /* Stop the file copy thread */
                        pSetupData->bStopInstall = TRUE;
                        SetEvent(pSetupData->hHaltInstallEvent);

#if 0
                        /* Wait for any pending installation */
                        WaitForSingleObject(pSetupData->hInstallThread, INFINITE);
                        CloseHandle(pSetupData->hInstallThread);
                        pSetupData->hInstallThread = NULL;
                        CloseHandle(pSetupData->hHaltInstallEvent);
                        pSetupData->hHaltInstallEvent = NULL;
#endif

                        // TODO: Unwind installation?!

                        /* Go to the Terminate page */
                        PropSheet_SetCurSelByID(GetParent(hwndDlg), IDD_RESTARTPAGE);
                    }
                    else
                    {
                        /* We don't stop installation, resume file copy */
                        SetEvent(pSetupData->hHaltInstallEvent);
                    }

                    /* Do not close the wizard too soon */
                    SetWindowLongPtrW(hwndDlg, DWLP_MSGRESULT, TRUE);
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
            SetDlgItemFont(hwndDlg, IDC_FINISHTITLE, pSetupData->hTitleFont, TRUE);
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
                case PSN_SETACTIVE:
                {
                    /* Only "Finish" for closing the wizard */
                    ShowDlgItem(GetParent(hwndDlg), IDCANCEL, SW_HIDE);
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_FINISH);

                    /* Set up the reboot progress bar */
                    SendDlgItemMessage(hwndDlg, IDC_RESTART_PROGRESS, PBM_SETRANGE, 0, MAKELPARAM(0, 300));
                    SendDlgItemMessage(hwndDlg, IDC_RESTART_PROGRESS, PBM_SETPOS, 0, 0);
                    SetTimer(hwndDlg, 1, 50, NULL);
                    break;
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

BOOL LoadSetupData(
    IN OUT PSETUPDATA pSetupData)
{
    pSetupData->PartitionList = CreatePartitionList();
    if (!pSetupData->PartitionList)
    {
        DPRINT1("Could not enumerate available disks; failing installation\n");
        return FALSE;
    }

    pSetupData->NtOsInstallsList = CreateNTOSInstallationsList(pSetupData->PartitionList);
    if (!pSetupData->NtOsInstallsList)
        DPRINT1("Failed to get a list of NTOS installations; continue installation...\n");

    /* Load the hardware, language and keyboard layout lists */

    pSetupData->USetupData.ComputerList = CreateComputerTypeList(pSetupData->USetupData.SetupInf);
    pSetupData->USetupData.DisplayList = CreateDisplayDriverList(pSetupData->USetupData.SetupInf);
    pSetupData->USetupData.KeyboardList = CreateKeyboardDriverList(pSetupData->USetupData.SetupInf);

    pSetupData->USetupData.LanguageList = CreateLanguageList(pSetupData->USetupData.SetupInf, pSetupData->DefaultLanguage);

    /* If not unattended, overwrite language and locale with
     * the current ones of the running ReactOS instance */
    if (!IsUnattendedSetup)
    {
        LCID LocaleID = GetUserDefaultLCID();

        StringCchPrintfW(pSetupData->DefaultLanguage,
                         _countof(pSetupData->DefaultLanguage),
                         L"%08lx", LocaleID);

        StringCchPrintfW(pSetupData->USetupData.LocaleID,
                         _countof(pSetupData->USetupData.LocaleID),
                         L"%08lx", LocaleID);
    }

    /* new part */
    pSetupData->SelectedLanguageId = pSetupData->DefaultLanguage;
    wcscpy(pSetupData->DefaultLanguage, pSetupData->USetupData.LocaleID); // FIXME: In principle, only when unattended.
    pSetupData->USetupData.LanguageId = (LANGID)(wcstol(pSetupData->SelectedLanguageId, NULL, 16) & 0xFFFF);

    pSetupData->USetupData.LayoutList = CreateKeyboardLayoutList(pSetupData->USetupData.SetupInf,
                                                                 pSetupData->SelectedLanguageId,
                                                                 pSetupData->DefaultKBLayout);

    /* If not unattended, overwrite keyboard layout with
     * the current one of the running ReactOS instance */
    if (!IsUnattendedSetup)
    {
        C_ASSERT(_countof(pSetupData->DefaultKBLayout) >= KL_NAMELENGTH);
        /* If the call fails, keep the default already stored in the buffer */
        GetKeyboardLayoutNameW(pSetupData->DefaultKBLayout);
    }

    /* Change the default entries in the language and keyboard layout lists */
    {
    PGENERIC_LIST LanguageList = pSetupData->USetupData.LanguageList;
    PGENERIC_LIST LayoutList = pSetupData->USetupData.LayoutList;
    PGENERIC_LIST_ENTRY ListEntry;

    /* Search for default language */
    for (ListEntry = GetFirstListEntry(LanguageList); ListEntry;
         ListEntry = GetNextListEntry(ListEntry))
    {
        PCWSTR LocaleId = ((PGENENTRY)GetListEntryData(ListEntry))->Id;
        if (!_wcsicmp(pSetupData->DefaultLanguage, LocaleId))
        {
            DPRINT("found %S in LanguageList\n", LocaleId);
            SetCurrentListEntry(LanguageList, ListEntry);
            break;
        }
    }

    /* Search for default layout */
    for (ListEntry = GetFirstListEntry(LayoutList); ListEntry;
         ListEntry = GetNextListEntry(ListEntry))
    {
        PCWSTR pszLayoutId = ((PGENENTRY)GetListEntryData(ListEntry))->Id;
        if (!_wcsicmp(pSetupData->DefaultKBLayout, pszLayoutId))
        {
            DPRINT("Found %S in LayoutList\n", pszLayoutId);
            SetCurrentListEntry(LayoutList, ListEntry);
            break;
        }
    }
    }

    return TRUE;
}

VOID
InitNtToWin32PathMappingList(
    IN OUT PNT_WIN32_PATH_MAPPING_LIST MappingList)
{
    InitializeListHead(&MappingList->List);
    MappingList->MappingsCount = 0;
}

VOID
FreeNtToWin32PathMappingList(
    IN OUT PNT_WIN32_PATH_MAPPING_LIST MappingList)
{
    PLIST_ENTRY ListEntry;
    PVOID Entry;

    while (!IsListEmpty(&MappingList->List))
    {
        ListEntry = RemoveHeadList(&MappingList->List);
        Entry = (PVOID)CONTAINING_RECORD(ListEntry, NT_WIN32_PATH_MAPPING, ListEntry);
        HeapFree(ProcessHeap, 0, Entry);
    }

    MappingList->MappingsCount = 0;
}

/*
 * Attempts to convert a pure NT file path into a corresponding Win32 path.
 * Adapted from GetInstallSourceWin32() in dll/win32/syssetup/wizard.c
 */
BOOL
ConvertNtPathToWin32Path(
    IN OUT PNT_WIN32_PATH_MAPPING_LIST MappingList,
    OUT PWSTR pwszPath,
    IN DWORD cchPathMax,
    IN PCWSTR pwszNTPath)
{
    BOOL FoundDrive = FALSE, RetryOnce = FALSE;
    PLIST_ENTRY ListEntry;
    PNT_WIN32_PATH_MAPPING Entry;
    PCWSTR pwszNtPathToMap = pwszNTPath;
    PCWSTR pwszRemaining = NULL;
    DWORD cchDrives;
    PWCHAR pwszDrive;
    WCHAR wszDrives[512];
    WCHAR wszNTPath[MAX_PATH];
    WCHAR TargetPath[MAX_PATH];

    *pwszPath = UNICODE_NULL;

    /*
     * We find first a mapping inside the MappingList. If one is found, use it
     * to build the Win32 path. If there is none, we need to create one by
     * checking the Win32 drives (and possibly NT symlinks too).
     * In case of success, add the newly found mapping to the list and use it
     * to build the Win32 path.
     */

    for (ListEntry = MappingList->List.Flink;
         ListEntry != &MappingList->List;
         ListEntry = ListEntry->Flink)
    {
        Entry = CONTAINING_RECORD(ListEntry, NT_WIN32_PATH_MAPPING, ListEntry);

        DPRINT("Testing '%S' --> '%S'\n", Entry->Win32Path, Entry->NtPath);

        /* Check whether the queried NT path prefixes the user-provided NT path */
        FoundDrive = !_wcsnicmp(pwszNtPathToMap, Entry->NtPath, wcslen(Entry->NtPath));
        if (FoundDrive)
        {
            /* Found it! */

            /* Set the pointers and go build the Win32 path */
            pwszDrive = Entry->Win32Path;
            pwszRemaining = pwszNTPath + wcslen(Entry->NtPath);
            goto Quit;
        }
    }

    /*
     * No mapping exists for this path yet: try to find one now.
     */

    /* Retrieve the mounted drives (available drive letters) */
    cchDrives = GetLogicalDriveStringsW(_countof(wszDrives) - 1, wszDrives);
    if (cchDrives == 0 || cchDrives >= _countof(wszDrives))
    {
        /* Buffer too small or failure */
        DPRINT1("ConvertNtPathToWin32Path: GetLogicalDriveStringsW failed\n");
        return FALSE;
    }

/* We go back there once if RetryOnce == TRUE */
Retry:

    /* Enumerate the mounted drives */
    for (pwszDrive = wszDrives; *pwszDrive; pwszDrive += wcslen(pwszDrive) + 1)
    {
        /* Retrieve the NT path corresponding to the current Win32 DOS path */
        pwszDrive[2] = UNICODE_NULL; // Temporarily remove the backslash
        QueryDosDeviceW(pwszDrive, wszNTPath, _countof(wszNTPath));
        pwszDrive[2] = L'\\';        // Restore the backslash

        DPRINT("Testing '%S' --> '%S'\n", pwszDrive, wszNTPath);

        /* Check whether the queried NT path prefixes the user-provided NT path */
        FoundDrive = !_wcsnicmp(pwszNtPathToMap, wszNTPath, wcslen(wszNTPath));
        if (!FoundDrive)
        {
            PWCHAR ptr, ptr2;

            /*
             * Check whether this was a network share that has a drive letter,
             * but the user-provided NT path points to this share without
             * mentioning the drive letter.
             *
             * The format is: \Device\<network_redirector>\;X:<data>\share\path
             * The corresponding drive letter is 'X'.
             * A system-provided network redirector (LanManRedirector or Mup)
             * or a 3rd-party one may be used.
             *
             * We check whether the user-provided NT path has the form:
             * \Device\<network_redirector>\<data>\share\path
             * as it obviously did not have the full form (the previous check
             * would have been OK otherwise).
             */
            if (!_wcsnicmp(wszNTPath, L"\\Device\\", _countof(L"\\Device\\")-1) &&
                (ptr = wcschr(wszNTPath + _countof(L"\\Device\\")-1, L'\\')) &&
                wcslen(++ptr) >= 3 && ptr[0] == L';' && ptr[2] == L':')
            {
                /*
                 * Normally the specified drive letter should correspond
                 * to the one used for the mapping. But we will ignore
                 * if it happens not to be the case.
                 */
                if (pwszDrive[0] != ptr[1])
                {
                    DPRINT1("Peculiar: expected network share drive letter %C different from actual one %C\n",
                            pwszDrive[0], ptr[1]);
                }

                /* Remove the drive letter from the NT network share path */
                ptr2 = ptr + 3;
                /* Swallow as many possible consecutive backslashes as there could be */
                while (*ptr2 == L'\\') ++ptr2;

                memmove(ptr, ptr2, (wcslen(ptr2) + 1) * sizeof(WCHAR));

                /* Now do the check again */
                FoundDrive = !_wcsnicmp(pwszNtPathToMap, wszNTPath, wcslen(wszNTPath));
            }
        }
        if (FoundDrive)
        {
            /* Found it! */

            pwszDrive[2] = UNICODE_NULL; // Remove the backslash

            if (pwszNtPathToMap == pwszNTPath)
            {
                ASSERT(!RetryOnce && pwszNTPath != TargetPath);
                pwszRemaining = pwszNTPath + wcslen(wszNTPath);
            }
            break;
        }
    }

    if (FoundDrive)
    {
        /* A mapping was found, add it to the cache */
        Entry = HeapAlloc(ProcessHeap, HEAP_ZERO_MEMORY, sizeof(*Entry));
        if (!Entry)
        {
            DPRINT1("ConvertNtPathToWin32Path: Cannot allocate memory\n");
            return FALSE;
        }
        StringCchCopyNW(Entry->NtPath, _countof(Entry->NtPath),
                        pwszNTPath, pwszRemaining - pwszNTPath);
        StringCchCopyW(Entry->Win32Path, _countof(Entry->Win32Path), pwszDrive);

        /* Insert it as the most recent entry */
        InsertHeadList(&MappingList->List, &Entry->ListEntry);
        MappingList->MappingsCount++;

        /* Set the pointers and go build the Win32 path */
        pwszDrive = Entry->Win32Path;
        goto Quit;
    }

    /*
     * We failed, perhaps because the beginning of the NT path used a symlink.
     * Try to see whether this is the case by attempting to resolve it.
     * If the symlink resolution gives nothing, or we already failed once,
     * there is no hope in converting the path to Win32.
     * Otherwise, symlink resolution succeeds but we need to recheck again
     * the drives list.
     */

    /*
     * In theory we would have to parse each element in the NT path and going
     * until finding a symlink object (otherwise we would fail straight away).
     * However here we can use guessing instead, since we know which kind of
     * NT paths we are likely to manipulate: \Device\HarddiskX\PartitionY\ and
     * the like (including \Device\HarddiskVolumeX\) and the other ones that
     * are supported in setuplib\utils\arcname.c .
     *
     * But actually, all the supported names in arcname.c are real devices,
     * and only \Device\HarddiskX\PartitionY\ may refer to a symlink, so we
     * just check for it.
     */
    if (!RetryOnce && !FoundDrive)
    {
        ULONG DiskNumber, PartitionNumber;
        INT Length;

        NTSTATUS Status;
        OBJECT_ATTRIBUTES ObjectAttributes;
        HANDLE LinkHandle;
        UNICODE_STRING SymLink, Target;

        if (swscanf(pwszNTPath, L"\\Device\\Harddisk%lu\\Partition%lu%n",
                    &DiskNumber, &PartitionNumber, &Length) != 2)
        {
            /* Definitively not a recognized path, bail out */
            return FALSE;
        }

        /* Check whether \Device\HarddiskX\PartitionY is a symlink */
        RtlInitEmptyUnicodeString(&SymLink, (PWCHAR)pwszNTPath, Length * sizeof(WCHAR));
        SymLink.Length = SymLink.MaximumLength;

        InitializeObjectAttributes(&ObjectAttributes,
                                   &SymLink,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);
        Status = NtOpenSymbolicLinkObject(&LinkHandle,
                                          SYMBOLIC_LINK_QUERY,
                                          &ObjectAttributes);
        if (!NT_SUCCESS(Status))
        {
            /* Not a symlink, or something else happened: bail out */
            DPRINT1("ConvertNtPathToWin32Path: NtOpenSymbolicLinkObject(%wZ) failed, Status 0x%08lx\n",
                    &SymLink, Status);
            return FALSE;
        }

        *TargetPath = UNICODE_NULL;
        RtlInitEmptyUnicodeString(&Target, TargetPath, sizeof(TargetPath));

        /* Resolve the link and close its handle */
        Status = NtQuerySymbolicLinkObject(LinkHandle, &Target, NULL);
        NtClose(LinkHandle);

        /* Check for success */
        if (!NT_SUCCESS(Status))
        {
            /* Not a symlink, or something else happened: bail out */
            DPRINT1("ConvertNtPathToWin32Path: NtQuerySymbolicLinkObject(%wZ) failed, Status 0x%08lx\n",
                    &SymLink, Status);
            return FALSE;
        }

        /* Set the pointers */
        pwszRemaining = pwszNTPath + Length;
        pwszNtPathToMap = TargetPath; // Point to our local buffer

        /* Retry once */
        RetryOnce = TRUE;
        goto Retry;
    }

    ASSERT(!FoundDrive);

Quit:
    if (FoundDrive)
    {
        StringCchPrintfW(pwszPath, cchPathMax,
                         L"%s%s",
                         pwszDrive,
                         pwszRemaining);
        DPRINT("ConvertNtPathToWin32Path: %S\n", pwszPath);
        return TRUE;
    }

    return FALSE;
}

/* Used to enable and disable the shutdown privilege */
/* static */ BOOL
EnablePrivilege(
    IN LPCWSTR lpszPrivilegeName,
    IN BOOL bEnablePrivilege)
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

/* Copied from HotkeyThread() in dll/win32/syssetup/install.c */
static DWORD CALLBACK
HotkeyThread(LPVOID Parameter)
{
    ATOM hotkey;
    MSG msg;

    DPRINT("HotkeyThread start\n");

    hotkey = GlobalAddAtomW(L"Setup Shift+F10 Hotkey");
    if (!RegisterHotKey(NULL, hotkey, MOD_SHIFT, VK_F10))
        DPRINT1("RegisterHotKey failed with %lu\n", GetLastError());

    while (GetMessageW(&msg, NULL, 0, 0))
    {
        if (msg.hwnd == NULL && msg.message == WM_HOTKEY && msg.wParam == hotkey)
        {
            WCHAR CmdLine[] = L"cmd.exe"; // CreateProcess can modify this buffer.
            STARTUPINFOW si = { sizeof(si) };
            PROCESS_INFORMATION pi;

            if (CreateProcessW(NULL,
                               CmdLine,
                               NULL,
                               NULL,
                               FALSE,
                               CREATE_NEW_CONSOLE,
                               NULL,
                               NULL,
                               &si,
                               &pi))
            {
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
            }
            else
            {
                DPRINT1("Failed to launch command prompt: %lu\n", GetLastError());
            }
        }
    }

    UnregisterHotKey(NULL, hotkey);
    GlobalDeleteAtom(hotkey);

    DPRINT("HotkeyThread terminate\n");
    return 0;
}

int WINAPI
_tWinMain(HINSTANCE hInst,
          HINSTANCE hPrevInstance,
          LPTSTR lpszCmdLine,
          int nCmdShow)
{
    ULONG Error;
    HANDLE hHotkeyThread;
    INITCOMMONCONTROLSEX iccx;
    PROPSHEETHEADER psh;
    HPROPSHEETPAGE ahpsp[8];
    PROPSHEETPAGE psp = {0};
    UINT nPages = 0;

    ProcessHeap = GetProcessHeap();

    SetupData.hInstance = hInst;
    SetupData.hInstallThread = NULL;
    SetupData.hHaltInstallEvent = NULL;
    SetupData.bStopInstall = FALSE;

    /* Initialize the NT to Win32 path prefix mapping list */
    InitNtToWin32PathMappingList(&SetupData.MappingList);

    /* Initialize Setup */
    Error = InitializeSetup(&SetupData.USetupData, NULL,
                            &SpFileExports, &SpInfExports);
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

    /* Retrieve any supplemental options from the unattend file */
    CheckUnattendedSetup(&SetupData.USetupData);
    SetupData.bUnattend = IsUnattendedSetup; // FIXME :-)

    /* Load extra setup data (HW lists etc...) */
    if (!LoadSetupData(&SetupData))
        goto Quit;

    hHotkeyThread = CreateThread(NULL, 0, HotkeyThread, NULL, 0, NULL);

    /* Whenever any of the common controls are used in your app,
     * you must call InitCommonControlsEx() to register the classes
     * for those controls. */
    iccx.dwSize = sizeof(iccx);
    iccx.dwICC = ICC_LISTVIEW_CLASSES | ICC_TREEVIEW_CLASSES | ICC_PROGRESS_CLASS;
    InitCommonControlsEx(&iccx);

    /* Register the TreeList control */
    // RegisterTreeListClass(hInst);
    TreeListRegister(hInst);

    /* Create the title and bold fonts */
    SetupData.hTitleFont = CreateTitleFont(NULL);
    SetupData.hBoldFont  = CreateBoldFont(NULL, 0);

    if (!SetupData.bUnattend)
    {
        /* Create the Start page, until setup is working */
        // NOTE: What does "until setup is working" mean??
        psp.dwSize = sizeof(PROPSHEETPAGE);
        psp.dwFlags = PSP_DEFAULT | PSP_HIDEHEADER;
        psp.hInstance = hInst;
        psp.lParam = (LPARAM)&SetupData;
        psp.pfnDlgProc = StartDlgProc;
        psp.pszTemplate = MAKEINTRESOURCEW(IDD_STARTPAGE);
        ahpsp[nPages++] = CreatePropertySheetPage(&psp);

        /* Create the install type selection page */
        psp.dwSize = sizeof(PROPSHEETPAGE);
        psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
        psp.pszHeaderTitle = MAKEINTRESOURCEW(IDS_TYPETITLE);
        psp.pszHeaderSubTitle = MAKEINTRESOURCEW(IDS_TYPESUBTITLE);
        psp.hInstance = hInst;
        psp.lParam = (LPARAM)&SetupData;
        psp.pfnDlgProc = TypeDlgProc;
        psp.pszTemplate = MAKEINTRESOURCEW(IDD_TYPEPAGE);
        ahpsp[nPages++] = CreatePropertySheetPage(&psp);

        /* Create the upgrade/repair selection page */
        psp.dwSize = sizeof(PROPSHEETPAGE);
        psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
        psp.pszHeaderTitle = MAKEINTRESOURCEW(IDS_UPDATETITLE);
        psp.pszHeaderSubTitle = MAKEINTRESOURCEW(IDS_UPDATESUBTITLE);
        psp.hInstance = hInst;
        psp.lParam = (LPARAM)&SetupData;
        psp.pfnDlgProc = UpgradeRepairDlgProc;
        psp.pszTemplate = MAKEINTRESOURCEW(IDD_UPDATEREPAIRPAGE);
        ahpsp[nPages++] = CreatePropertySheetPage(&psp);

        /* Create the device settings page */
        psp.dwSize = sizeof(PROPSHEETPAGE);
        psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
        psp.pszHeaderTitle = MAKEINTRESOURCEW(IDS_DEVICETITLE);
        psp.pszHeaderSubTitle = MAKEINTRESOURCEW(IDS_DEVICESUBTITLE);
        psp.hInstance = hInst;
        psp.lParam = (LPARAM)&SetupData;
        psp.pfnDlgProc = DeviceDlgProc;
        psp.pszTemplate = MAKEINTRESOURCEW(IDD_DEVICEPAGE);
        ahpsp[nPages++] = CreatePropertySheetPage(&psp);

        /* Create the install device settings page / boot method / install directory */
        psp.dwSize = sizeof(PROPSHEETPAGE);
        psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
        psp.pszHeaderTitle = MAKEINTRESOURCEW(IDS_DRIVETITLE);
        psp.pszHeaderSubTitle = MAKEINTRESOURCEW(IDS_DRIVESUBTITLE);
        psp.hInstance = hInst;
        psp.lParam = (LPARAM)&SetupData;
        psp.pfnDlgProc = DriveDlgProc;
        psp.pszTemplate = MAKEINTRESOURCEW(IDD_DRIVEPAGE);
        ahpsp[nPages++] = CreatePropertySheetPage(&psp);

        /* Create the summary page */
        psp.dwSize = sizeof(PROPSHEETPAGE);
        psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
        psp.pszHeaderTitle = MAKEINTRESOURCEW(IDS_SUMMARYTITLE);
        psp.pszHeaderSubTitle = MAKEINTRESOURCEW(IDS_SUMMARYSUBTITLE);
        psp.hInstance = hInst;
        psp.lParam = (LPARAM)&SetupData;
        psp.pfnDlgProc = SummaryDlgProc;
        psp.pszTemplate = MAKEINTRESOURCEW(IDD_SUMMARYPAGE);
        ahpsp[nPages++] = CreatePropertySheetPage(&psp);
    }

    /* Create the installation progress page */
    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    psp.pszHeaderTitle = MAKEINTRESOURCEW(IDS_PROCESSTITLE);
    psp.pszHeaderSubTitle = MAKEINTRESOURCEW(IDS_PROCESSSUBTITLE);
    psp.hInstance = hInst;
    psp.lParam = (LPARAM)&SetupData;
    psp.pfnDlgProc = ProcessDlgProc;
    psp.pszTemplate = MAKEINTRESOURCEW(IDD_PROCESSPAGE);
    ahpsp[nPages++] = CreatePropertySheetPage(&psp);

    /* Create the finish-and-reboot page */
    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = PSP_DEFAULT | PSP_HIDEHEADER;
    psp.hInstance = hInst;
    psp.lParam = (LPARAM)&SetupData;
    psp.pfnDlgProc = RestartDlgProc;
    psp.pszTemplate = MAKEINTRESOURCEW(IDD_RESTARTPAGE);
    ahpsp[nPages++] = CreatePropertySheetPage(&psp);

    /* Create the property sheet */
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_WIZARD97 | PSH_WATERMARK | PSH_HEADER;
    psh.hInstance = hInst;
    psh.hwndParent = NULL;
    psh.nPages = nPages;
    psh.nStartPage = 0;
    psh.phpage = ahpsp;
    psh.pszbmWatermark = MAKEINTRESOURCEW(IDB_WATERMARK);
    psh.pszbmHeader = MAKEINTRESOURCEW(IDB_HEADER);

    /* Display the wizard */
    PropertySheet(&psh);

    /* Wait for any pending installation */
    WaitForSingleObject(SetupData.hInstallThread, INFINITE);
    CloseHandle(SetupData.hInstallThread);
    SetupData.hInstallThread = NULL;
    CloseHandle(SetupData.hHaltInstallEvent);
    SetupData.hHaltInstallEvent = NULL;

    if (SetupData.hBoldFont)
        DeleteFont(SetupData.hBoldFont);
    if (SetupData.hTitleFont)
        DeleteFont(SetupData.hTitleFont);

    /* Unregister the TreeList control */
    // UnregisterTreeListClass(hInst);
    TreeListUnregister(hInst);

    if (hHotkeyThread)
    {
        PostThreadMessageW(GetThreadId(hHotkeyThread), WM_QUIT, 0, 0);
        CloseHandle(hHotkeyThread);
    }

Quit:
    /* Setup has finished */
    FinishSetup(&SetupData.USetupData);

    /* Free the NT to Win32 path prefix mapping list */
    FreeNtToWin32PathMappingList(&SetupData.MappingList);

#if 0 // NOTE: Disabled for testing purposes only!
    EnablePrivilege(SE_SHUTDOWN_NAME, TRUE);
    ExitWindowsEx(EWX_REBOOT, 0);
    EnablePrivilege(SE_SHUTDOWN_NAME, FALSE);
#endif

    return 0;
}

/* EOF */
