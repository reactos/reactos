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
#include <shlwapi.h>

// #include <ntdddisk.h>
#include <ntddstor.h>
#include <ntddscsi.h>

#include "resource.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS ******************************************************************/

#define IDS_LIST_COLUMN_FIRST IDS_PARTITION_NAME
#define IDS_LIST_COLUMN_LAST  IDS_PARTITION_STATUS

#define MAX_LIST_COLUMNS (IDS_LIST_COLUMN_LAST - IDS_LIST_COLUMN_FIRST + 1)
static const UINT column_ids[MAX_LIST_COLUMNS] = {IDS_LIST_COLUMN_FIRST, IDS_LIST_COLUMN_FIRST + 1, IDS_LIST_COLUMN_FIRST + 2, IDS_LIST_COLUMN_FIRST + 3};
static const INT  column_widths[MAX_LIST_COLUMNS] = {200, 90, 60, 60};
static const INT  column_alignment[MAX_LIST_COLUMNS] = {LVCFMT_LEFT, LVCFMT_LEFT, LVCFMT_RIGHT, LVCFMT_RIGHT};

/* FUNCTIONS ****************************************************************/

/**
 * @brief
 * Sanitize a given string in-place, by
 * removing any invalid character found in it.
 **/
static BOOL
DoSanitizeText(
    _Inout_ PWSTR pszSanitized)
{
    PWCHAR pch1, pch2;
    BOOL bSanitized = FALSE;

    for (pch1 = pch2 = pszSanitized; *pch1; ++pch1)
    {
        /* Skip any invalid character found */
        if (!IS_VALID_INSTALL_PATH_CHAR(*pch1))
        {
            bSanitized = TRUE;
            continue;
        }

        /* Copy over the valid ones */
        *pch2 = *pch1;
        ++pch2;
    }
    *pch2 = 0;

    return bSanitized;
}

/**
 * @brief
 * Sanitize in-place any text found in the clipboard.
 **/
static BOOL
DoSanitizeClipboard(
    _In_ HWND hWnd)
{
    HGLOBAL hData;
    LPWSTR pszText, pszSanitized;
    BOOL bSanitized;

    /* Protect read-only edit control from modification */
    if (GetWindowLongPtrW(hWnd, GWL_STYLE) & ES_READONLY)
        return FALSE;

    if (!OpenClipboard(hWnd))
        return FALSE;

    hData = GetClipboardData(CF_UNICODETEXT);
    pszText = GlobalLock(hData);
    if (!pszText)
    {
        CloseClipboard();
        return FALSE;
    }

    pszSanitized = _wcsdup(pszText);
    GlobalUnlock(hData);
    bSanitized = (pszSanitized && DoSanitizeText(pszSanitized));
    if (bSanitized)
    {
        /* Update clipboard text */
        SIZE_T cbData = (wcslen(pszSanitized) + 1) * sizeof(WCHAR);
        hData = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, cbData);
        pszText = GlobalLock(hData);
        if (pszText)
        {
            CopyMemory(pszText, pszSanitized, cbData);
            GlobalUnlock(hData);
            SetClipboardData(CF_UNICODETEXT, hData);
        }
    }
    free(pszSanitized);

    CloseClipboard();
    return bSanitized;
}

static VOID
ShowErrorTip(
    _In_ HWND hEdit)
{
    EDITBALLOONTIP balloon;
    WCHAR szTitle[512];
    WCHAR szText[512];

    /* Load the resources */
    LoadStringW(SetupData.hInstance, IDS_ERROR_INVALID_INSTALLDIR_CHAR_TITLE, szTitle, _countof(szTitle));
    LoadStringW(SetupData.hInstance, IDS_ERROR_INVALID_INSTALLDIR_CHAR, szText, _countof(szText));

    /* Show a warning balloon */
    balloon.cbStruct = sizeof(balloon);
    balloon.pszTitle = szTitle;
    balloon.pszText  = szText;
#if (_WIN32_WINNT < _WIN32_WINNT_VISTA)
    balloon.ttiIcon  = TTI_ERROR;
#else
    balloon.ttiIcon  = TTI_ERROR_LARGE;
#endif

    MessageBeep(MB_ICONERROR);
    Edit_ShowBalloonTip(hEdit, &balloon);

    // NOTE: There is no need to hide it when other keys are pressed;
    // the EDIT control will deal with that itself.
}

/**
 * @brief
 * Subclass edit window procedure to filter allowed characters
 * for the ReactOS installation directory.
 **/
static LRESULT
CALLBACK
InstallDirEditProc(
    _In_ HWND hWnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    WNDPROC orgEditProc = (WNDPROC)GetWindowLongPtrW(hWnd, GWLP_USERDATA);

    switch (uMsg)
    {
    case WM_UNICHAR:
        if (wParam == UNICODE_NOCHAR)
            return TRUE;
        __fallthrough;

    case WM_IME_CHAR:
    case WM_CHAR:
    {
        WCHAR wch = (WCHAR)wParam;

        /* Let the EDIT control deal with Control characters.
         * It won't emit them as raw data in the text. */
        if (wParam < ' ')
            break;

        /* Ignore Ctrl-Backspace */
        if (wParam == '\x7F')
            return 0;

        /* Protect read-only edit control from modification */
        if (GetWindowLongPtrW(hWnd, GWL_STYLE) & ES_READONLY)
            break;

        if (uMsg == WM_IME_CHAR)
        {
            if (!IsWindowUnicode(hWnd) && HIBYTE(wch) != 0)
            {
                CHAR data[] = {HIBYTE(wch), LOBYTE(wch)};
                MultiByteToWideChar(CP_ACP, 0, data, 2, &wch, 1);
            }
        }

        /* Show an error and ignore input character if it's invalid */
        if (!IS_VALID_INSTALL_PATH_CHAR(wch))
        {
            ShowErrorTip(hWnd);
            return 0;
        }
        break;
    }

    case WM_PASTE:
        /* Verify the text being pasted; if it was sanitized, show an error */
        if (DoSanitizeClipboard(hWnd))
            ShowErrorTip(hWnd);
        break;
    }

    return CallWindowProcW(orgEditProc, hWnd, uMsg, wParam, lParam);
}

static INT_PTR
CALLBACK
MoreOptDlgProc(
    _In_ HWND hDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    PSETUPDATA pSetupData;

    /* Retrieve pointer to the global setup data */
    pSetupData = (PSETUPDATA)GetWindowLongPtrW(hDlg, GWLP_USERDATA);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            HWND hEdit;
            WNDPROC orgEditProc;
            BOOL bIsBIOS;
            UINT uID;
            INT iItem, iCurrent = CB_ERR, iDefault = 0;
            WCHAR szText[50];

            /* Save pointer to the global setup data */
            pSetupData = (PSETUPDATA)lParam;
            SetWindowLongPtrW(hDlg, GWLP_USERDATA, (LONG_PTR)pSetupData);

            /* Subclass the install-dir edit control */
            hEdit = GetDlgItem(hDlg, IDC_PATH);
            orgEditProc = (WNDPROC)GetWindowLongPtrW(hEdit, GWLP_WNDPROC);
            SetWindowLongPtrW(hEdit, GWLP_USERDATA, (LONG_PTR)orgEditProc);
            SetWindowLongPtrW(hEdit, GWLP_WNDPROC, (LONG_PTR)InstallDirEditProc);

            /* Set the current installation directory */
            SetWindowTextW(hEdit, pSetupData->USetupData.InstallationDirectory);


            /* Initialize the list of available bootloader locations */
            bIsBIOS = ((pSetupData->USetupData.ArchType == ARCH_PcAT) ||
                       (pSetupData->USetupData.ArchType == ARCH_NEC98x86));
            for (uID = IDS_BOOTLOADER_NOINST; uID <= IDS_BOOTLOADER_VBRONLY; ++uID)
            {
                if ( ( bIsBIOS && (uID == IDS_BOOTLOADER_SYSTEM)) ||
                     (!bIsBIOS && (uID == IDS_BOOTLOADER_MBRVBR || uID == IDS_BOOTLOADER_VBRONLY)) )
                {
                    continue; // Skip this choice.
                }

                LoadStringW(pSetupData->hInstance, uID, szText, ARRAYSIZE(szText));
                iItem = SendDlgItemMessageW(hDlg, IDC_INSTFREELDR, CB_ADDSTRING, 0, (LPARAM)szText);
                if (iItem != CB_ERR && iItem != CB_ERRSPACE)
                {
                    UINT uBldrLoc = uID - IDS_BOOTLOADER_NOINST
                                        - (bIsBIOS && (uID >= IDS_BOOTLOADER_SYSTEM) ? 1 : 0);
                    SendDlgItemMessageW(hDlg, IDC_INSTFREELDR, CB_SETITEMDATA, iItem, uBldrLoc);

                    /* Find the index of the current and default locations */
                    if (uBldrLoc == pSetupData->USetupData.BootLoaderLocation)
                        iCurrent = iItem;
                    if (uBldrLoc == (IDS_BOOTLOADER_SYSTEM - IDS_BOOTLOADER_NOINST))
                        iDefault = iItem;
                }
            }
            /* Select the current location or fall back to the default one */
            if (iCurrent == CB_ERR)
                iCurrent = iDefault;
            SendDlgItemMessageW(hDlg, IDC_INSTFREELDR, CB_SETCURSEL, iCurrent, 0);

            break;
        }

        case WM_DESTROY:
        {
            /* Unsubclass the edit control */
            HWND hEdit = GetDlgItem(hDlg, IDC_PATH);
            WNDPROC orgEditProc = (WNDPROC)GetWindowLongPtrW(hEdit, GWLP_USERDATA);
            if (orgEditProc) SetWindowLongPtrW(hEdit, GWLP_WNDPROC, (LONG_PTR)orgEditProc);
            break;
        }

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                {
                    HWND hEdit;
                    BOOL bIsValid;
                    WCHAR InstallDir[MAX_PATH];
                    INT iItem;
                    UINT uBldrLoc = CB_ERR;

                    /*
                     * Retrieve the installation path and verify its validity.
                     * Check for the validity of the installation directory and
                     * pop up an error if this is not the case.
                     */
                    hEdit = GetDlgItem(hDlg, IDC_PATH);
                    bIsValid = (GetWindowTextLengthW(hEdit) < _countof(InstallDir)); // && IsValidInstallDirectory(InstallDir);
                    GetWindowTextW(hEdit, InstallDir, _countof(InstallDir));
                    bIsValid = bIsValid && IsValidInstallDirectory(InstallDir);

                    if (!bIsValid)
                    {
                        // ERROR_DIRECTORY_NAME
                        DisplayError(hDlg,
                                     IDS_ERROR_DIRECTORY_NAME_TITLE,
                                     IDS_ERROR_DIRECTORY_NAME);
                        break; // Go back to the dialog.
                    }

                    StringCchCopyW(pSetupData->USetupData.InstallationDirectory,
                                   _countof(pSetupData->USetupData.InstallationDirectory),
                                   InstallDir);

                    /* Retrieve the bootloader location */
                    iItem = SendDlgItemMessageW(hDlg, IDC_INSTFREELDR, CB_GETCURSEL, 0, 0);
                    if (iItem != CB_ERR)
                        uBldrLoc = SendDlgItemMessageW(hDlg, IDC_INSTFREELDR, CB_GETITEMDATA, iItem, 0);
                    if (uBldrLoc == CB_ERR) // Default location: System partition / MBR & VBR
                        uBldrLoc = (IDS_BOOTLOADER_SYSTEM - IDS_BOOTLOADER_NOINST);
                    uBldrLoc = min(max(uBldrLoc, 0), 3);
                    pSetupData->USetupData.BootLoaderLocation = uBldrLoc;

                    EndDialog(hDlg, IDOK);
                    return TRUE;
                }

                case IDCANCEL:
                    EndDialog(hDlg, IDCANCEL);
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
            switch (LOWORD(wParam))
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
CreateTreeListColumns(
    IN HINSTANCE hInstance,
    IN HWND hWndTreeList,
    IN const UINT* pIDs,
    IN const INT* pColsWidth,
    IN const INT* pColsAlign,
    IN UINT nNumOfColumns)
{
    UINT i;
    TLCOLUMN tlC;
    WCHAR szText[50];

    /* Create the columns */
    tlC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    tlC.pszText = szText;

    /* Load the column labels from the resource file */
    for (i = 0; i < nNumOfColumns; i++)
    {
        tlC.iSubItem = i;
        tlC.cx = pColsWidth[i];
        tlC.fmt = pColsAlign[i];

        LoadStringW(hInstance, pIDs[i], szText, ARRAYSIZE(szText));

        if (TreeList_InsertColumn(hWndTreeList, i, &tlC) == -1)
            return FALSE;
    }

    return TRUE;
}

// unused
VOID
DisplayStuffUsingWin32Setup(HWND hwndDlg)
{
    HDEVINFO h;
    HWND hList;
    SP_DEVINFO_DATA DevInfoData;
    DWORD i;

    h = SetupDiGetClassDevs(&GUID_DEVCLASS_DISKDRIVE, NULL, NULL, DIGCF_PRESENT);
    if (h == INVALID_HANDLE_VALUE)
        return;

    hList = GetDlgItem(hwndDlg, IDC_PARTITION);
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
            {
                return;
            }
        }
        if (buffer)
        {
            SendMessageW(hList, LB_ADDSTRING, (WPARAM)0, (LPARAM)buffer);
            LocalFree(buffer);
        }
    }
    SetupDiDestroyDeviceInfoList(h);
}


HTLITEM
TreeListAddItem(IN HWND hTreeList,
                IN HTLITEM hParent,
                IN LPWSTR lpText,
                IN INT iImage,
                IN INT iSelectedImage,
                IN LPARAM lParam)
{
    TL_INSERTSTRUCTW Insert;

    ZeroMemory(&Insert, sizeof(Insert));

    Insert.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    Insert.hInsertAfter = TVI_LAST;
    Insert.hParent = hParent;
    Insert.item.pszText = lpText;
    Insert.item.iImage = iImage;
    Insert.item.iSelectedImage = iSelectedImage;
    Insert.item.lParam = lParam;

    // Insert.item.mask |= TVIF_STATE;
    // Insert.item.stateMask = TVIS_OVERLAYMASK;
    // Insert.item.state = INDEXTOOVERLAYMASK(1);

    return TreeList_InsertItem(hTreeList, &Insert);
}


VOID
GetPartitionTypeString(
    IN PPARTENTRY PartEntry,
    OUT PSTR strBuffer,
    IN ULONG cchBuffer)
{
    if (PartEntry->PartitionType == PARTITION_ENTRY_UNUSED)
    {
        StringCchCopyA(strBuffer, cchBuffer,
                       "Unused" /* MUIGetString(STRING_FORMATUNUSED) */);
    }
    else if (IsContainerPartition(PartEntry->PartitionType))
    {
        StringCchCopyA(strBuffer, cchBuffer,
                       "Extended Partition" /* MUIGetString(STRING_EXTENDED_PARTITION) */);
    }
    else
    {
        UINT i;

        /* Do the table lookup */
        if (PartEntry->DiskEntry->DiskStyle == PARTITION_STYLE_MBR)
        {
            for (i = 0; i < ARRAYSIZE(MbrPartitionTypes); ++i)
            {
                if (PartEntry->PartitionType == MbrPartitionTypes[i].Type)
                {
                    StringCchCopyA(strBuffer, cchBuffer,
                                   MbrPartitionTypes[i].Description);
                    return;
                }
            }
        }
#if 0 // TODO: GPT support!
        else if (PartEntry->DiskEntry->DiskStyle == PARTITION_STYLE_GPT)
        {
            for (i = 0; i < ARRAYSIZE(GptPartitionTypes); ++i)
            {
                if (IsEqualPartitionType(PartEntry->PartitionType,
                                         GptPartitionTypes[i].Guid))
                {
                    StringCchCopyA(strBuffer, cchBuffer,
                                   GptPartitionTypes[i].Description);
                    return;
                }
            }
        }
#endif

        /* We are here because the partition type is unknown */
        if (cchBuffer > 0) *strBuffer = '\0';
    }

    if ((cchBuffer > 0) && (*strBuffer == '\0'))
    {
        StringCchPrintfA(strBuffer, cchBuffer,
                         // MUIGetString(STRING_PARTTYPE),
                         "Type 0x%02x",
                         PartEntry->PartitionType);
    }
}

static
HTLITEM
PrintPartitionData(
    IN HWND hWndList,
    IN PPARTLIST List,
    IN HTLITEM htiParent,
    IN PDISKENTRY DiskEntry,
    IN PPARTENTRY PartEntry)
{
    PVOLINFO VolInfo = (PartEntry->Volume ? &PartEntry->Volume->Info : NULL);
    LARGE_INTEGER PartSize;
    HTLITEM htiPart;
    CHAR PartTypeString[32];
    PCHAR PartType = PartTypeString;
    WCHAR LineBuffer[128];

    /* Volume name */
    if (PartEntry->IsPartitioned == FALSE)
    {
        StringCchPrintfW(LineBuffer, ARRAYSIZE(LineBuffer),
                         // MUIGetString(STRING_UNPSPACE),
                         L"Unpartitioned space");
    }
    else
    {
        StringCchPrintfW(LineBuffer, ARRAYSIZE(LineBuffer),
                         // MUIGetString(STRING_HDDINFOUNK5),
                         L"%s (%c%c)",
                         (VolInfo && *VolInfo->VolumeLabel) ? VolInfo->VolumeLabel : L"Partition",
                         !(VolInfo && VolInfo->DriveLetter) ? L'-' : VolInfo->DriveLetter,
                         !(VolInfo && VolInfo->DriveLetter) ? L'-' : L':');
    }

    htiPart = TreeListAddItem(hWndList, htiParent, LineBuffer,
                              1, 1,
                              (LPARAM)PartEntry);

    /* Determine partition type */
    *LineBuffer = 0;
    if (PartEntry->IsPartitioned)
    {
        PartTypeString[0] = '\0';
        if (PartEntry->New == TRUE)
        {
            PartType = "New (Unformatted)"; // MUIGetString(STRING_UNFORMATTED);
        }
        else if (PartEntry->IsPartitioned == TRUE)
        {
            GetPartitionTypeString(PartEntry,
                                   PartTypeString,
                                   ARRAYSIZE(PartTypeString));
            PartType = PartTypeString;
        }

        StringCchPrintfW(LineBuffer, ARRAYSIZE(LineBuffer),
                         L"%S",
                         PartType);
    }
    TreeList_SetItemText(hWndList, htiPart, 1, LineBuffer);

    /* Format the disk size in KBs, MBs, etc... */
    PartSize.QuadPart = PartEntry->SectorCount.QuadPart * DiskEntry->BytesPerSector;
    if (StrFormatByteSizeW(PartSize.QuadPart, LineBuffer, ARRAYSIZE(LineBuffer)) == NULL)
    {
        /* We failed for whatever reason, do the hardcoded way */
        PWCHAR Unit;

#if 0
        if (PartSize.QuadPart >= 10 * GB) /* 10 GB */
        {
            PartSize.QuadPart = RoundingDivide(PartSize.QuadPart, GB);
            // Unit = MUIGetString(STRING_GB);
            Unit = L"GB";
        }
        else
#endif
        if (PartSize.QuadPart >= 10 * MB) /* 10 MB */
        {
            PartSize.QuadPart = RoundingDivide(PartSize.QuadPart, MB);
            // Unit = MUIGetString(STRING_MB);
            Unit = L"MB";
        }
        else
        {
            PartSize.QuadPart = RoundingDivide(PartSize.QuadPart, KB);
            // Unit = MUIGetString(STRING_KB);
            Unit = L"KB";
        }

        StringCchPrintfW(LineBuffer, ARRAYSIZE(LineBuffer),
                         L"%6lu %s",
                         PartSize.u.LowPart,
                         Unit);
    }
    TreeList_SetItemText(hWndList, htiPart, 2, LineBuffer);

    /* Volume status */
    *LineBuffer = 0;
    if (PartEntry->IsPartitioned)
    {
        StringCchPrintfW(LineBuffer, ARRAYSIZE(LineBuffer),
                         // MUIGetString(STRING_HDDINFOUNK5),
                         PartEntry->BootIndicator ? L"Active" : L"");
    }
    TreeList_SetItemText(hWndList, htiPart, 3, LineBuffer);

    return htiPart;
}

static
VOID
PrintDiskData(
    IN HWND hWndList,
    IN PPARTLIST List,
    IN PDISKENTRY DiskEntry)
{
    BOOL Success;
    HANDLE hDevice;
    PCHAR DiskName = NULL;
    ULONG Length = 0;
    PPARTENTRY PrimaryPartEntry, LogicalPartEntry;
    PLIST_ENTRY PrimaryEntry, LogicalEntry;
    ULARGE_INTEGER DiskSize;
    HTLITEM htiDisk, htiPart;
    WCHAR LineBuffer[128];
    UCHAR outBuf[512];

    StringCchPrintfW(LineBuffer, ARRAYSIZE(LineBuffer),
                     // L"\\Device\\Harddisk%lu\\Partition%lu",
                     L"\\\\.\\PhysicalDrive%lu",
                     DiskEntry->DiskNumber);

    hDevice = CreateFileW(
                LineBuffer,                         // device interface name
                GENERIC_READ /*| GENERIC_WRITE*/,   // dwDesiredAccess
                FILE_SHARE_READ | FILE_SHARE_WRITE, // dwShareMode
                NULL,                               // lpSecurityAttributes
                OPEN_EXISTING,                      // dwCreationDistribution
                0,                                  // dwFlagsAndAttributes
                NULL                                // hTemplateFile
                );
    if (hDevice != INVALID_HANDLE_VALUE)
    {
        STORAGE_PROPERTY_QUERY Query;

        Query.PropertyId = StorageDeviceProperty;
        Query.QueryType  = PropertyStandardQuery;

        Success = DeviceIoControl(hDevice,
                                  IOCTL_STORAGE_QUERY_PROPERTY,
                                  &Query,
                                  sizeof(Query),
                                  &outBuf,
                                  sizeof(outBuf),
                                  &Length,
                                  NULL);
        if (Success)
        {
            PSTORAGE_DEVICE_DESCRIPTOR devDesc = (PSTORAGE_DEVICE_DESCRIPTOR)outBuf;
            if (devDesc->ProductIdOffset)
            {
                DiskName = (PCHAR)&outBuf[devDesc->ProductIdOffset];
                Length -= devDesc->ProductIdOffset;
                DiskName[min(Length, strlen(DiskName))] = 0;
                // ( i = devDesc->ProductIdOffset; p[i] != 0 && i < Length; i++ )
            }
        }

        CloseHandle(hDevice);
    }

    if (DiskName && *DiskName)
    {
        if (DiskEntry->DriverName.Length > 0)
        {
            StringCchPrintfW(LineBuffer, ARRAYSIZE(LineBuffer),
                             // MUIGetString(STRING_HDDINFO_1),
                             L"Harddisk %lu (%S) (Port=%hu, Bus=%hu, Id=%hu) on %wZ",
                             DiskEntry->DiskNumber,
                             DiskName,
                             DiskEntry->Port,
                             DiskEntry->Bus,
                             DiskEntry->Id,
                             &DiskEntry->DriverName);
        }
        else
        {
            StringCchPrintfW(LineBuffer, ARRAYSIZE(LineBuffer),
                             // MUIGetString(STRING_HDDINFO_2),
                             L"Harddisk %lu (%S) (Port=%hu, Bus=%hu, Id=%hu)",
                             DiskEntry->DiskNumber,
                             DiskName,
                             DiskEntry->Port,
                             DiskEntry->Bus,
                             DiskEntry->Id);
        }
    }
    else
    {
        if (DiskEntry->DriverName.Length > 0)
        {
            StringCchPrintfW(LineBuffer, ARRAYSIZE(LineBuffer),
                             // MUIGetString(STRING_HDDINFO_1),
                             L"Harddisk %lu (Port=%hu, Bus=%hu, Id=%hu) on %wZ",
                             DiskEntry->DiskNumber,
                             DiskEntry->Port,
                             DiskEntry->Bus,
                             DiskEntry->Id,
                             &DiskEntry->DriverName);
        }
        else
        {
            StringCchPrintfW(LineBuffer, ARRAYSIZE(LineBuffer),
                             // MUIGetString(STRING_HDDINFO_2),
                             L"Harddisk %lu (Port=%hu, Bus=%hu, Id=%hu)",
                             DiskEntry->DiskNumber,
                             DiskEntry->Port,
                             DiskEntry->Bus,
                             DiskEntry->Id);
        }
    }

    htiDisk = TreeListAddItem(hWndList, NULL, LineBuffer,
                              0, 0,
                              (LPARAM)DiskEntry);

    /* Disk type: MBR, GPT or RAW (Uninitialized) */
    TreeList_SetItemText(hWndList, htiDisk, 1,
                         DiskEntry->DiskStyle == PARTITION_STYLE_MBR ? L"MBR" :
                         DiskEntry->DiskStyle == PARTITION_STYLE_GPT ? L"GPT" :
                                                                       L"RAW");

    /* Format the disk size in KBs, MBs, etc... */
    DiskSize.QuadPart = DiskEntry->SectorCount.QuadPart * DiskEntry->BytesPerSector;
    if (StrFormatByteSizeW(DiskSize.QuadPart, LineBuffer, ARRAYSIZE(LineBuffer)) == NULL)
    {
        /* We failed for whatever reason, do the hardcoded way */
        PWCHAR Unit;

        if (DiskSize.QuadPart >= 10 * GB) /* 10 GB */
        {
            DiskSize.QuadPart = RoundingDivide(DiskSize.QuadPart, GB);
            // Unit = MUIGetString(STRING_GB);
            Unit = L"GB";
        }
        else
        {
            DiskSize.QuadPart = RoundingDivide(DiskSize.QuadPart, MB);
            if (DiskSize.QuadPart == 0)
                DiskSize.QuadPart = 1;
            // Unit = MUIGetString(STRING_MB);
            Unit = L"MB";
        }

        StringCchPrintfW(LineBuffer, ARRAYSIZE(LineBuffer),
                         L"%6lu %s",
                         DiskSize.u.LowPart,
                         Unit);
    }
    TreeList_SetItemText(hWndList, htiDisk, 2, LineBuffer);


    /* Print partition lines */
    for (PrimaryEntry = DiskEntry->PrimaryPartListHead.Flink;
         PrimaryEntry != &DiskEntry->PrimaryPartListHead;
         PrimaryEntry = PrimaryEntry->Flink)
    {
        PrimaryPartEntry = CONTAINING_RECORD(PrimaryEntry, PARTENTRY, ListEntry);

        htiPart = PrintPartitionData(hWndList, List, htiDisk,
                                     DiskEntry, PrimaryPartEntry);

        if (IsContainerPartition(PrimaryPartEntry->PartitionType))
        {
            for (LogicalEntry = DiskEntry->LogicalPartListHead.Flink;
                 LogicalEntry != &DiskEntry->LogicalPartListHead;
                 LogicalEntry = LogicalEntry->Flink)
            {
                LogicalPartEntry = CONTAINING_RECORD(LogicalEntry, PARTENTRY, ListEntry);

                PrintPartitionData(hWndList, List, htiPart,
                                   DiskEntry, LogicalPartEntry);
            }

            /* Expand the extended partition node */
            TreeList_Expand(hWndList, htiPart, TVE_EXPAND);
        }
    }

    /* Expand the disk node */
    TreeList_Expand(hWndList, htiDisk, TVE_EXPAND);
}

VOID
DrawPartitionList(
    IN HWND hWndList,
    IN PPARTLIST List)
{
    PLIST_ENTRY Entry;
    PDISKENTRY DiskEntry;

    for (Entry = List->DiskListHead.Flink;
         Entry != &List->DiskListHead;
         Entry = Entry->Flink)
    {
        DiskEntry = CONTAINING_RECORD(Entry, DISKENTRY, ListEntry);

        /* Print disk entry */
        PrintDiskData(hWndList, List, DiskEntry);
    }
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
             * when the user selects a valid partition.
             */
            PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK);

            hList = GetDlgItem(hwndDlg, IDC_PARTITION);

            TreeList_SetExtendedStyleEx(hList, TVS_EX_FULLROWMARK, TVS_EX_FULLROWMARK);
            // TreeList_SetExtendedStyleEx(hList, TVS_EX_FULLROWITEMS, TVS_EX_FULLROWITEMS);

            CreateTreeListColumns(pSetupData->hInstance,
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
            ImageList_AddIcon(hSmall, LoadIconW(pSetupData->hInstance, MAKEINTRESOURCEW(IDI_DISKDRIVE)));
            ImageList_AddIcon(hSmall, LoadIconW(pSetupData->hInstance, MAKEINTRESOURCEW(IDI_PARTITION)));

            /* Assign the ImageList to the List View */
            TreeList_SetImageList(hList, hSmall, TVSIL_NORMAL);

            // DisplayStuffUsingWin32Setup(hwndDlg);
            DrawPartitionList(hList, pSetupData->PartitionList);
            break;
        }

        case WM_DESTROY:
        {
            hList = GetDlgItem(hwndDlg, IDC_PARTITION);
            hSmall = TreeList_GetImageList(hList, TVSIL_NORMAL);
            TreeList_SetImageList(hList, NULL, TVSIL_NORMAL);
            ImageList_Destroy(hSmall);
            return TRUE;
        }

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDC_PARTMOREOPTS:
                    DialogBoxParamW(pSetupData->hInstance,
                                    MAKEINTRESOURCEW(IDD_ADVINSTOPTS),
                                    hwndDlg,
                                    MoreOptDlgProc,
                                    (LPARAM)pSetupData);
                    break;

                case IDC_PARTCREATE:
                    DialogBoxW(pSetupData->hInstance,
                               MAKEINTRESOURCEW(IDD_PARTITION),
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

            // On Vista+ we can use TVN_ITEMCHANGED instead, with  NMTVITEMCHANGE* pointer
            if (lpnm->idFrom == IDC_PARTITION && lpnm->code == TVN_SELCHANGED)
            {
                LPNMTREEVIEW pnmv = (LPNMTREEVIEW)lParam;

                // if (pnmv->uChanged & TVIF_STATE) /* The state has changed */
                if (pnmv->itemNew.mask & TVIF_STATE)
                {
                    /* The item has been (de)selected */
                    // if (pnmv->uNewState & TVIS_SELECTED)
                    if (pnmv->itemNew.state & TVIS_SELECTED)
                    {
                        HTLITEM hParentItem = TreeList_GetParent(lpnm->hwndFrom, pnmv->itemNew.hItem);
                        /* May or may not be a PPARTENTRY: this is a PPARTENTRY only when hParentItem != NULL */
                        PPARTENTRY PartEntry = (PPARTENTRY)pnmv->itemNew.lParam;

                        if (!hParentItem || !PartEntry)
                        {
                            EnableWindow(GetDlgItem(hwndDlg, IDC_PARTCREATE), TRUE);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_PARTDELETE), FALSE);
                            goto DisableWizNext;
                        }
                        else // if (hParentItem && PartEntry)
                        {
                            EnableWindow(GetDlgItem(hwndDlg, IDC_PARTCREATE), !PartEntry->IsPartitioned);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_PARTDELETE),  PartEntry->IsPartitioned);

                            if (PartEntry->IsPartitioned &&
                                !IsContainerPartition(PartEntry->PartitionType) /* alternatively: PartEntry->PartitionNumber != 0 */ &&
                                PartEntry->Volume && // !PartEntry->Volume->New &&
                                (PartEntry->Volume->FormatState == Formatted))
                            {
                                PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
                            }
                            else
                            {
                                goto DisableWizNext;
                            }
                        }
                    }
                    else
                    {
DisableWizNext:
                        /*
                         * Keep the "Next" button disabled. It will be enabled only
                         * when the user selects a valid partition.
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
                     * when the user selects a valid partition.
                     */
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK);
                    break;
                }
#endif

                case PSN_QUERYINITIALFOCUS:
                {
                    /* Give the focus on and select the first item */
                    hList = GetDlgItem(hwndDlg, IDC_PARTITION);
                    // TreeList_SetFocusItem(hList, 1, 1);
                    TreeList_SelectItem(hList, 1);
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LONG_PTR)hList);
                    return TRUE;
                }

                case PSN_QUERYCANCEL:
                {
                    if (MessageBoxW(GetParent(hwndDlg),
                                    pSetupData->szAbortMessage,
                                    pSetupData->szAbortTitle,
                                    MB_YESNO | MB_ICONQUESTION) == IDYES)
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
                    NTSTATUS Status;

                    /****/
                    // FIXME: This is my test disk encoding!
                    DISKENTRY DiskEntry;
                    PARTENTRY PartEntry;
                    DiskEntry.DiskNumber = 0;
                    DiskEntry.HwDiskNumber = 0;
                    DiskEntry.HwFixedDiskNumber = 0;
                    PartEntry.DiskEntry = &DiskEntry;
                    PartEntry.PartitionNumber = 1; // 4;
                    PartEntry.Volume = NULL;
                    /****/

                    Status = InitDestinationPaths(&pSetupData->USetupData,
                                                  NULL, // pSetupData->USetupData.InstallationDirectory,
                                                  PartEntry.Volume);

                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT1("InitDestinationPaths() failed with status 0x%08lx\n", Status);
                    }

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

/* EOF */
