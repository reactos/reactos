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

#include <math.h> // For pow()

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


/**
 * @brief
 * Data structure stored for each partition item in the TreeList.
 * (None for disks items.)
 **/
typedef struct _PARTITEM
{
    PPARTENTRY PartEntry; //< Disk region this structure is associated to.
    PVOLENTRY Volume;     //< Associated file system volume if any, or NULL.
    PVOL_CREATE_INFO VolCreate; //< Volume create information (allocated).
} PARTITEM, *PPARTITEM;

/**
 * @brief
 * Dialog context structure used by PartitionDlgProc() and FormatDlgProc(Worker)().
 **/
typedef struct _PARTCREATE_CTX
{
    PPARTITEM PartItem;  //< Partition item info stored in the TreeList.
    ULONG MaxSizeMB;     //< Maximum possible partition size in MB.
    ULONG PartSizeMB;    //< Selected partition size in MB.
    BOOLEAN MBRExtPart;  //< Whether to create an MBR extended partition.
    BOOLEAN ForceFormat; //< Whether to force formatting ('Do not format' option hidden).
} PARTCREATE_CTX, *PPARTCREATE_CTX;

static INT_PTR
CALLBACK
FormatDlgProcWorker(
    _In_ PPARTCREATE_CTX PartCreateCtx,
    _In_ HWND hDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            ULONG Index = 0;
            INT iItem;
            PCWSTR FileSystem;
            PCWSTR DefaultFs;
            PVOLENTRY Volume;
            PVOL_CREATE_INFO VolCreate;

            /* List the well-known file systems. We use the same strings
             * for the displayed FS names and their actual ones. */
            while (GetRegisteredFileSystems(Index++, &FileSystem))
            {
                iItem = SendDlgItemMessageW(hDlg, IDC_FSTYPE, CB_INSERTSTRING, -1, (LPARAM)FileSystem);
                if (iItem != CB_ERR && iItem != CB_ERRSPACE)
                    SendDlgItemMessageW(hDlg, IDC_FSTYPE, CB_SETITEMDATA, iItem, (LPARAM)FileSystem);
            }

            /* Add the 'Do not format' entry if needed */
            if (!PartCreateCtx->ForceFormat)
            {
                WCHAR szText[50];

                LoadStringW(SetupData.hInstance, IDS_VOLUME_NOFORMAT, szText, _countof(szText));
                iItem = SendDlgItemMessageW(hDlg, IDC_FSTYPE, CB_INSERTSTRING, 0, (LPARAM)szText);
                if (iItem != CB_ERR && iItem != CB_ERRSPACE)
                    SendDlgItemMessageW(hDlg, IDC_FSTYPE, CB_SETITEMDATA, iItem, (LPARAM)NULL);
            }

            // FIXME: Read from SetupData.FsType; select the "FAT" FS instead.
            DefaultFs = L"FAT";

            /* Retrieve the selected volume and create information */
            ASSERT(PartCreateCtx->PartItem->Volume == PartCreateCtx->PartItem->PartEntry->Volume);
            Volume = PartCreateCtx->PartItem->PartEntry->Volume;
            VolCreate = PartCreateCtx->PartItem->VolCreate;

            /* Select the existing file system in the list if any,
             * otherwise use the "DefaultFs" */
            if (VolCreate && *VolCreate->FileSystemName)
                FileSystem = VolCreate->FileSystemName;
            else if (Volume && *Volume->Info.FileSystem)
                FileSystem = Volume->Info.FileSystem;
            else
                FileSystem = DefaultFs;

            iItem = SendDlgItemMessageW(hDlg, IDC_FSTYPE, CB_FINDSTRINGEXACT, 0, (LPARAM)FileSystem);
            if (iItem == CB_ERR)
                iItem = 0;
            SendDlgItemMessageW(hDlg, IDC_FSTYPE, CB_SETCURSEL, (WPARAM)iItem, 0);

            /* Check the quick-format option by default as it speeds up formatting */
            if (!VolCreate || VolCreate->QuickFormat)
                CheckDlgButton(hDlg, IDC_CHECK_QUICKFMT, BST_CHECKED);
            else
                CheckDlgButton(hDlg, IDC_CHECK_QUICKFMT, BST_UNCHECKED);

            break;
        }

        case WM_COMMAND:
        {
            //
            // NOTE:
            // - CBN_SELCHANGE sent everytime a combobox list item is selected,
            //   *even if* the list is opened.
            // - CBN_SELENDOK sent only when user finished to select an item
            //   by clicking on it (if the list is opened), or selection changed
            //   (when the list is closed but selection is done with arrow keys).
            //
            if ((HIWORD(wParam) == CBN_SELCHANGE /*|| HIWORD(wParam) == CBN_SELENDOK*/) &&
                (LOWORD(wParam) == IDC_FSTYPE))
            {
                // HWND hWndList = GetDlgItem(hDlg, IDC_FSTYPE);
                PCWSTR FileSystem;
                INT iItem;

                /* Retrieve the selected file system. Use the
                 * item data instead of the displayed string. */
                iItem = SendDlgItemMessageW(hDlg, IDC_FSTYPE, CB_GETCURSEL, 0, 0);
                if (iItem == CB_ERR)
                    iItem = 0; // Default entry
                FileSystem = (PCWSTR)SendDlgItemMessageW(hDlg, IDC_FSTYPE, CB_GETITEMDATA, iItem, 0);
                if (FileSystem == (PCWSTR)CB_ERR)
                    FileSystem = NULL; // Default data

                /* Enable or disable formatting options,
                 * depending on whether we need to format */
                EnableDlgItem(hDlg, IDC_CHECK_QUICKFMT, (FileSystem && *FileSystem));
                break;
            }

            if (HIWORD(wParam) != BN_CLICKED)
                break;

            switch (LOWORD(wParam))
            {
            case IDOK:
            {
                PPARTITEM PartItem = PartCreateCtx->PartItem;
                PVOL_CREATE_INFO VolCreate;
                PCWSTR FileSystem;
                INT iItem;

                /*
                 * Retrieve the formatting options
                 */

                /* Retrieve the selected file system. Use the
                 * item data instead of the displayed string. */
                iItem = SendDlgItemMessageW(hDlg, IDC_FSTYPE, CB_GETCURSEL, 0, 0);
                if (iItem == CB_ERR)
                    iItem = 0; // Default entry
                FileSystem = (PCWSTR)SendDlgItemMessageW(hDlg, IDC_FSTYPE, CB_GETITEMDATA, iItem, 0);
                if (FileSystem == (PCWSTR)CB_ERR)
                    FileSystem = NULL; // Default data

                VolCreate = PartItem->VolCreate;

                /* Check if we need to format */
                if (!FileSystem || !*FileSystem)
                {
                    /* We don't format. If there is an existing
                     * volume-create structure, free it. */
                    if (VolCreate)
                        LocalFree(VolCreate);
                    PartItem->VolCreate = NULL;

                    /* And return */
                    return TRUE;
                }

                /* We will format: allocate and initialize
                 * a volume-create structure if needed */
                if (!VolCreate)
                    VolCreate = LocalAlloc(LPTR, sizeof(*VolCreate));
                if (!VolCreate)
                {
                    DPRINT1("Failed to allocate volume-create structure\n");
                    return TRUE;
                }

                /* Cached input information that will be set to
                 * the FORMAT_VOLUME_INFO structure given to the
                 * 'FSVOLNOTIFY_STARTFORMAT' step */
                // TODO: Think about which values could be defaulted...
                StringCchCopyW(VolCreate->FileSystemName,
                               _countof(VolCreate->FileSystemName),
                               FileSystem);
                VolCreate->MediaFlag = FMIFS_HARDDISK;
                VolCreate->Label = NULL;
                VolCreate->QuickFormat =
                    (IsDlgButtonChecked(hDlg, IDC_CHECK_QUICKFMT) == BST_CHECKED);
                VolCreate->ClusterSize = 0;

                /* Set the volume associated to the new create information */
                VolCreate->Volume = PartItem->Volume;

                /* Associate the new, or update the volume create information */
                PartItem->VolCreate = VolCreate;
                return TRUE;
            }
            }
        }
    }
    return FALSE;
}

static INT_PTR
CALLBACK
FormatDlgProc(
    _In_ HWND hDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    PPARTCREATE_CTX PartCreateCtx;

    /* Retrieve dialog context pointer */
    PartCreateCtx = (PPARTCREATE_CTX)GetWindowLongPtrW(hDlg, GWLP_USERDATA);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            /* Save dialog context pointer */
            PartCreateCtx = (PPARTCREATE_CTX)lParam;
            SetWindowLongPtrW(hDlg, GWLP_USERDATA, (LONG_PTR)PartCreateCtx);

            /* We actually want to format, so set the flag */
            PartCreateCtx->ForceFormat = TRUE;
            break;
        }

        case WM_COMMAND:
        {
            if (HIWORD(wParam) != BN_CLICKED)
                break;

            switch (LOWORD(wParam))
            {
            case IDOK:
            {
                /* Retrieve the formatting options */
                FormatDlgProcWorker(PartCreateCtx, hDlg, uMsg, wParam, lParam);
                EndDialog(hDlg, IDOK);
                return TRUE;
            }

            case IDCANCEL:
            {
                EndDialog(hDlg, IDCANCEL);
                return TRUE;
            }
            }
        }
    }

    return FormatDlgProcWorker(PartCreateCtx, hDlg, uMsg, wParam, lParam);
}

static INT_PTR
CALLBACK
PartitionDlgProc(
    _In_ HWND hDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    PPARTCREATE_CTX PartCreateCtx;

    /* Retrieve dialog context pointer */
    PartCreateCtx = (PPARTCREATE_CTX)GetWindowLongPtrW(hDlg, GWLP_USERDATA);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            PPARTENTRY PartEntry;
            PDISKENTRY DiskEntry;
            ULONG MaxSizeMB;

            /* Save dialog context pointer */
            PartCreateCtx = (PPARTCREATE_CTX)lParam;
            SetWindowLongPtrW(hDlg, GWLP_USERDATA, (LONG_PTR)PartCreateCtx);

            /* Retrieve the selected partition */
            PartEntry = PartCreateCtx->PartItem->PartEntry;
            DiskEntry = PartEntry->DiskEntry;

            /* Set the spinner to the maximum size in MB the partition can have */
            MaxSizeMB = PartCreateCtx->MaxSizeMB;
            SendDlgItemMessageW(hDlg, IDC_UPDOWN_PARTSIZE, UDM_SETRANGE32, (WPARAM)1, (LPARAM)MaxSizeMB);
            SendDlgItemMessageW(hDlg, IDC_UPDOWN_PARTSIZE, UDM_SETPOS32, 0, (LPARAM)MaxSizeMB);

            /* Default to regular partition (non-extended on MBR disks) */
            CheckDlgButton(hDlg, IDC_CHECK_MBREXTPART, BST_UNCHECKED);

            /* Also, disable and hide IDC_CHECK_MBREXTPART
             * if space is logical or the disk is not MBR */
            if ((DiskEntry->DiskStyle == PARTITION_STYLE_MBR) &&
                !PartEntry->LogicalPartition)
            {
                ShowDlgItem(hDlg, IDC_CHECK_MBREXTPART, SW_SHOW);
                EnableDlgItem(hDlg, IDC_CHECK_MBREXTPART, TRUE);
            }
            else
            {
                ShowDlgItem(hDlg, IDC_CHECK_MBREXTPART, SW_HIDE);
                EnableDlgItem(hDlg, IDC_CHECK_MBREXTPART, FALSE);
            }

            break;
        }

        case WM_COMMAND:
        {
            if (HIWORD(wParam) != BN_CLICKED)
                break;

            switch (LOWORD(wParam))
            {
            case IDC_CHECK_MBREXTPART:
            {
                /* Check for MBR-extended (container) partition */
                // BST_UNCHECKED or BST_INDETERMINATE => FALSE
                if (IsDlgButtonChecked(hDlg, IDC_CHECK_MBREXTPART) == BST_CHECKED)
                {
                    /* It is, disable formatting options */
                    EnableDlgItem(hDlg, IDC_FS_STATIC, FALSE);
                    EnableDlgItem(hDlg, IDC_FSTYPE, FALSE);
                    EnableDlgItem(hDlg, IDC_CHECK_QUICKFMT, FALSE);
                }
                else
                {
                    /* It is not, re-enable formatting options */
                    EnableDlgItem(hDlg, IDC_FS_STATIC, TRUE);
                    EnableDlgItem(hDlg, IDC_FSTYPE, TRUE);
                    EnableDlgItem(hDlg, IDC_CHECK_QUICKFMT, TRUE);
                }
                break;
            }

            case IDOK:
            {
                /* Collect all the information and return it
                 * to the caller for creating the partition */
                PartCreateCtx->PartSizeMB = (ULONG)SendDlgItemMessageW(hDlg, IDC_UPDOWN_PARTSIZE, UDM_GETPOS32, 0, (LPARAM)NULL);
                PartCreateCtx->PartSizeMB = min(max(PartCreateCtx->PartSizeMB, 1), PartCreateCtx->MaxSizeMB);
                PartCreateCtx->MBRExtPart = (IsDlgButtonChecked(hDlg, IDC_CHECK_MBREXTPART) == BST_CHECKED);

                /* Retrieve the formatting options */
                FormatDlgProcWorker(PartCreateCtx, hDlg, uMsg, wParam, lParam);

                EndDialog(hDlg, IDOK);
                return TRUE;
            }

            case IDCANCEL:
            {
                EndDialog(hDlg, IDCANCEL);
                return TRUE;
            }
            }
        }
    }

    return FormatDlgProcWorker(PartCreateCtx, hDlg, uMsg, wParam, lParam);
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


HTLITEM
TreeListAddItem(
    _In_ HWND hTreeList,
    _In_opt_ HTLITEM hParent,
    _In_opt_ HTLITEM hInsertAfter,
    _In_ LPCWSTR lpText,
    _In_ INT iImage,
    _In_ INT iSelectedImage,
    _In_ LPARAM lParam)
{
    TLINSERTSTRUCTW Insert;

    ZeroMemory(&Insert, sizeof(Insert));

    Insert.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    Insert.hParent = hParent;
    Insert.hInsertAfter = (hInsertAfter ? hInsertAfter : TVI_LAST);
    Insert.item.pszText = (LPWSTR)lpText;
    Insert.item.iImage = iImage;
    Insert.item.iSelectedImage = iSelectedImage;
    Insert.item.lParam = lParam;

    // Insert.item.mask |= TVIF_STATE;
    // Insert.item.stateMask = TVIS_OVERLAYMASK;
    // Insert.item.state = INDEXTOOVERLAYMASK(1);

    return TreeList_InsertItem(hTreeList, &Insert);
}

LPARAM
TreeListGetItemData(
    _In_ HWND hTreeList,
    _In_ HTLITEM hItem)
{
    TLITEMW tlItem;

    tlItem.mask = TVIF_PARAM;
    tlItem.hItem = hItem;

    TreeList_GetItem(hTreeList, &tlItem);

    return tlItem.lParam;
}

static
PPARTITEM
GetItemPartition(
    _In_ HWND hTreeList,
    _In_ HTLITEM hItem)
{
    HTLITEM hParentItem;
    PPARTITEM PartItem;

    hParentItem = TreeList_GetParent(hTreeList, hItem);
    /* May or may not be a PPARTITEM: this is a PPARTITEM only when hParentItem != NULL */
    PartItem = (PPARTITEM)TreeListGetItemData(hTreeList, hItem);
    if (!hParentItem || !PartItem)
        return NULL;

    return PartItem;
}

static
PPARTITEM
GetSelectedPartition(
    _In_ HWND hTreeList,
    _Out_opt_ HTLITEM* phItem)
{
    HTLITEM hItem;
    PPARTITEM PartItem;

    hItem = TreeList_GetSelection(hTreeList);
    if (!hItem)
        return NULL;

    PartItem = GetItemPartition(hTreeList, hItem);
    if (PartItem && phItem)
        *phItem = hItem;

    return PartItem;
}

PVOL_CREATE_INFO
FindVolCreateInTreeByVolume(
    _In_ HWND hTreeList,
    _In_ PVOLENTRY Volume)
{
    HTLITEM hItem;

    /* Enumerate every cached data in the TreeList, and for each, check
     * whether its corresponding PPARTENTRY is the one we are looking for */
    // for (hItem = TVI_ROOT; hItem; hItem = TreeList_GetNextItem(...)) { }
    hItem = TVI_ROOT;
    while ((hItem = TreeList_GetNextItem(hTreeList, hItem, TVGN_NEXTITEM)))
    {
        PPARTITEM PartItem = GetItemPartition(hTreeList, hItem);
        if (!PartItem || !PartItem->VolCreate /* || !PartItem->Volume */)
            continue;

        if (PartItem->Volume == Volume)
        {
            /* Found it, return the associated volume-create structure */
            return PartItem->VolCreate;
        }
    }

    /* Nothing was found */
    return NULL;
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
    // else if (PartEntry == PartEntry->DiskEntry->ExtendedPartition)
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


static VOID
PrettifySize1(
    _Inout_ PULONGLONG Size,
    _Out_ PCWSTR* Unit)
{
    ULONGLONG DiskSize = *Size;

    if (DiskSize >= 10 * GB) /* 10 GB */
    {
        DiskSize = RoundingDivide(DiskSize, GB);
        *Unit = L"GB"; // MUIGetString(STRING_GB);
    }
    else
    {
        DiskSize = RoundingDivide(DiskSize, MB);
        if (DiskSize == 0)
            DiskSize = 1;
        *Unit = L"MB"; // MUIGetString(STRING_MB);
    }

    *Size = DiskSize;
}

static VOID
PrettifySize2(
    _Inout_ PULONGLONG Size,
    _Out_ PCWSTR* Unit)
{
    ULONGLONG PartSize = *Size;

#if 0
    if (PartSize >= 10 * GB) /* 10 GB */
    {
        PartSize = RoundingDivide(PartSize, GB);
        *Unit = L"GB"; // MUIGetString(STRING_GB);
    }
    else
#endif
    if (PartSize >= 10 * MB) /* 10 MB */
    {
        PartSize = RoundingDivide(PartSize, MB);
        *Unit = L"MB"; // MUIGetString(STRING_MB);
    }
    else
    {
        PartSize = RoundingDivide(PartSize, KB);
        *Unit = L"KB"; // MUIGetString(STRING_KB);
    }

    *Size = PartSize;
}

static
HTLITEM
PrintPartitionData(
    _In_ HWND hWndList,
    _In_ HTLITEM htiParent,
    _In_opt_ HTLITEM hInsertAfter,
    _In_ PPARTENTRY PartEntry)
{
    PDISKENTRY DiskEntry = PartEntry->DiskEntry;
    PVOLINFO VolInfo = (PartEntry->Volume ? &PartEntry->Volume->Info : NULL);
    PPARTITEM PartItem;
    ULONGLONG PartSize;
    HTLITEM htiPart;
    CHAR PartTypeString[32];
    PCHAR PartType = PartTypeString;
    WCHAR LineBuffer[128];

    /* Volume name */
    if (PartEntry->IsPartitioned == FALSE)
    {
        /* Unpartitioned space: Just display the description */
        StringCchPrintfW(LineBuffer, ARRAYSIZE(LineBuffer),
                         // MUIGetString(STRING_UNPSPACE)
                         L"Unpartitioned space");
    }
    else
//
// NOTE: This could be done with the next case.
//
    if ((DiskEntry->DiskStyle == PARTITION_STYLE_MBR) &&
        IsContainerPartition(PartEntry->PartitionType))
    {
        /* Extended partition container: Just display the partition's type */
        StringCchPrintfW(LineBuffer, ARRAYSIZE(LineBuffer),
                         // MUIGetString(STRING_EXTENDED_PARTITION)
                         L"Extended Partition");
    }
    else
    {
        /* Drive letter and partition number */
        StringCchPrintfW(LineBuffer, ARRAYSIZE(LineBuffer),
                         // MUIGetString(STRING_HDDINFOUNK5),
                         L"%s (%c%c)",
                         (VolInfo && *VolInfo->VolumeLabel) ? VolInfo->VolumeLabel : L"Partition",
                         !(VolInfo && VolInfo->DriveLetter) ? L'-' : VolInfo->DriveLetter,
                         !(VolInfo && VolInfo->DriveLetter) ? L'-' : L':');
    }

    /* Allocate and initialize a partition-info structure */
    PartItem = LocalAlloc(LPTR, sizeof(*PartItem));
    if (!PartItem)
    {
        DPRINT1("Failed to allocate partition-info structure\n");
        // return NULL;
        // We'll store a NULL pointer?!
    }

    PartItem->PartEntry = PartEntry;
    PartItem->Volume = PartEntry->Volume;

    htiPart = TreeListAddItem(hWndList, htiParent, hInsertAfter,
                              LineBuffer, 1, 1,
                              (LPARAM)PartItem);

    *LineBuffer = 0;
    if (PartEntry->IsPartitioned)
    {
        PartTypeString[0] = '\0';

        /*
         * If the volume's file system is recognized, display the volume label
         * (if any) and the file system name. Otherwise, display the partition
         * type if it's not a new partition.
         */
        if (VolInfo && *VolInfo->FileSystem &&
            _wcsicmp(VolInfo->FileSystem, L"RAW") != 0)
        {
            // TODO: Group this part together with the similar one
            // from below once the strings are in the same encoding...
            if (PartEntry->New)
            {
                StringCchPrintfA(PartTypeString,
                                 ARRAYSIZE(PartTypeString),
                                 "New (%S)",
                                 VolInfo->FileSystem);
            }
            else
            {
                StringCchPrintfA(PartTypeString,
                                 ARRAYSIZE(PartTypeString),
                                 "%S",
                                 VolInfo->FileSystem);
            }
            PartType = PartTypeString;
        }
        else
        /* Determine partition type */
        if (PartEntry->New)
        {
            /* Use this description if the partition is new and not planned for formatting */
            PartType = "New (Unformatted)"; // MUIGetString(STRING_UNFORMATTED);
        }
        else
        {
            /* If the partition is not new but its file system is not recognized
             * (or is not formatted), use the partition type description. */
            GetPartitionTypeString(PartEntry,
                                   PartTypeString,
                                   ARRAYSIZE(PartTypeString));
            PartType = PartTypeString;
        }
#if 0
        if (!PartType || !*PartType)
        {
            PartType = MUIGetString(STRING_FORMATUNKNOWN);
        }
#endif

        StringCchPrintfW(LineBuffer, ARRAYSIZE(LineBuffer),
                         L"%S",
                         PartType);
    }
    TreeList_SetItemText(hWndList, htiPart, 1, LineBuffer);

    /* Format the partition size */
    PartSize = GetPartEntrySizeInBytes(PartEntry);
    if (!StrFormatByteSizeW(PartSize, LineBuffer, ARRAYSIZE(LineBuffer)))
    {
        /* We failed for whatever reason, do the hardcoded way */
        PCWSTR Unit;
        PrettifySize2(&PartSize, &Unit);
        StringCchPrintfW(LineBuffer, ARRAYSIZE(LineBuffer),
                         L"%6I64u %s",
                         PartSize, Unit);
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


    /* If this is the single extended partition of an MBR disk,
     * recursively display its logical partitions */
    if (PartEntry == DiskEntry->ExtendedPartition)
    {
        PLIST_ENTRY LogicalEntry;
        PPARTENTRY LogicalPartEntry;

        for (LogicalEntry = DiskEntry->LogicalPartListHead.Flink;
             LogicalEntry != &DiskEntry->LogicalPartListHead;
             LogicalEntry = LogicalEntry->Flink)
        {
            LogicalPartEntry = CONTAINING_RECORD(LogicalEntry, PARTENTRY, ListEntry);
            PrintPartitionData(hWndList, htiPart, NULL, LogicalPartEntry);
        }

        /* Expand the extended partition node */
        TreeList_Expand(hWndList, htiPart, TVE_EXPAND);
    }

    return htiPart;
}

/**
 * @brief
 * Called on response to the TVN_DELETEITEM notification sent by the TreeList.
 **/
static
VOID
DeleteTreeItem(
    _In_ HWND hWndList,
    _In_ TLITEMW* ptlItem)
{
    PPARTITEM PartItem;

    /* Code below is equivalent to: PartItem = GetItemPartition(hWndList, ptlItem->hItem);
     * except that we already have the data structure in ptlItem->lParam, so there is
     * no need to call extra helpers as GetItemPartition() does. */
    HTLITEM hParentItem = TreeList_GetParent(hWndList, ptlItem->hItem);
    /* May or may not be a PPARTITEM: this is a PPARTITEM only when hParentItem != NULL */
    PartItem = (PPARTITEM)ptlItem->lParam;
    if (!hParentItem || !PartItem)
        return;

    if (PartItem->VolCreate)
        LocalFree(PartItem->VolCreate);
    LocalFree(PartItem);
}

static
VOID
PrintDiskData(
    _In_ HWND hWndList,
    _In_opt_ HTLITEM hInsertAfter,
    _In_ PDISKENTRY DiskEntry)
{
    BOOL Success;
    HANDLE hDevice;
    PCHAR DiskName = NULL;
    ULONG Length = 0;
    PPARTENTRY PrimaryPartEntry;
    PLIST_ENTRY PrimaryEntry;
    ULONGLONG DiskSize;
    HTLITEM htiDisk;
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

    htiDisk = TreeListAddItem(hWndList, NULL, hInsertAfter,
                              LineBuffer, 0, 0,
                              (LPARAM)DiskEntry);

    /* Disk type: MBR, GPT or RAW (Uninitialized) */
    TreeList_SetItemText(hWndList, htiDisk, 1,
                         DiskEntry->DiskStyle == PARTITION_STYLE_MBR ? L"MBR" :
                         DiskEntry->DiskStyle == PARTITION_STYLE_GPT ? L"GPT" :
                                                                       L"RAW");

    /* Format the disk size */
    DiskSize = GetDiskSizeInBytes(DiskEntry);
    if (!StrFormatByteSizeW(DiskSize, LineBuffer, ARRAYSIZE(LineBuffer)))
    {
        /* We failed for whatever reason, do the hardcoded way */
        PCWSTR Unit;
        PrettifySize1(&DiskSize, &Unit);
        StringCchPrintfW(LineBuffer, ARRAYSIZE(LineBuffer),
                         L"%6I64u %s",
                         DiskSize, Unit);
    }
    TreeList_SetItemText(hWndList, htiDisk, 2, LineBuffer);

    /* Print partition lines */
    for (PrimaryEntry = DiskEntry->PrimaryPartListHead.Flink;
         PrimaryEntry != &DiskEntry->PrimaryPartListHead;
         PrimaryEntry = PrimaryEntry->Flink)
    {
        PrimaryPartEntry = CONTAINING_RECORD(PrimaryEntry, PARTENTRY, ListEntry);

        /* If this is an extended partition, recursively print the logical partitions */
        PrintPartitionData(hWndList, htiDisk, NULL, PrimaryPartEntry);
    }

    /* Expand the disk node */
    TreeList_Expand(hWndList, htiDisk, TVE_EXPAND);
}

static VOID
InitPartitionList(
    _In_ HINSTANCE hInstance,
    _In_ HWND hWndList)
{
    const DWORD dwStyle = TVS_HASBUTTONS | /*TVS_HASLINES | TVS_LINESATROOT |*/ TVS_SHOWSELALWAYS | TVS_FULLROWSELECT;
    const DWORD dwExStyle = TVS_EX_FULLROWMARK /* | TVS_EX_FULLROWITEMS*/;
    HIMAGELIST hSmall;

    /* Set the TreeList styles and fixup the item selection color */
    TreeList_SetStyle(hWndList, TreeList_GetStyle(hWndList) | dwStyle);
    // TreeList_SetStyleEx(hWndList, dwStyle, dwStyle);
    TreeList_SetExtendedStyle(hWndList, dwExStyle);
    TreeList_SetColor(hWndList, TVC_MARK, GetSysColor(COLOR_HIGHLIGHT));

    /* Initialize its columns */
    CreateTreeListColumns(hInstance,
                          hWndList,
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
    ImageList_AddIcon(hSmall, LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_DISKDRIVE)));
    ImageList_AddIcon(hSmall, LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_PARTITION)));

    /* Assign the ImageList to the List View */
    TreeList_SetImageList(hWndList, hSmall, TVSIL_NORMAL);
}

static VOID
DrawPartitionList(
    _In_ HWND hWndList,
    _In_ PPARTLIST List)
{
    PLIST_ENTRY Entry;
    PDISKENTRY DiskEntry;

    /* Clear the list first */
    TreeList_DeleteAllItems(hWndList);

    /* Insert all the detected disks and partitions */
    for (Entry = List->DiskListHead.Flink;
         Entry != &List->DiskListHead;
         Entry = Entry->Flink)
    {
        DiskEntry = CONTAINING_RECORD(Entry, DISKENTRY, ListEntry);

        /* Print disk entry */
        PrintDiskData(hWndList, NULL, DiskEntry);
    }

    /* Select the first item */
    // TreeList_SetFocusItem(hWndList, 1, 1);
    TreeList_SelectItem(hWndList, 1);
}

static VOID
CleanupPartitionList(
    _In_ HWND hWndList)
{
    HIMAGELIST hSmall;

    /* Cleanup all the items. Their cached data will be automatically deleted
     * on response to the TVN_DELETEITEM notification sent by the TreeList
     * by DeleteTreeItem() */
    TreeList_DeleteAllItems(hWndList);

    /* And cleanup the imagelist */
    hSmall = TreeList_GetImageList(hWndList, TVSIL_NORMAL);
    TreeList_SetImageList(hWndList, NULL, TVSIL_NORMAL);
    ImageList_Destroy(hSmall);
}


/**
 * @brief
 * Create a partition in the selected disk region in the partition list,
 * and update the partition list UI.
 **/
static BOOLEAN
DoCreatePartition(
    _In_ HWND hList,
    _In_ PPARTLIST List,
    _Inout_ HTLITEM* phItem,
    _Inout_opt_ PPARTITEM* pPartItem,
    _In_opt_ ULONGLONG SizeBytes,
    _In_opt_ ULONG_PTR PartitionInfo)
{
    BOOLEAN Success;
    PPARTITEM PartItem;
    PPARTENTRY PartEntry;
    HTLITEM hParentItem;
    HTLITEM hInsertAfter;
    PPARTENTRY NextPart;

    PartItem = (pPartItem ? *pPartItem : GetItemPartition(hList, *phItem));
    if (!PartItem)
    {
        /* We must have a disk region... */
        ASSERT(FALSE);
        return FALSE;
    }
    PartEntry = PartItem->PartEntry;
#if 0
    if (PartEntry->IsPartitioned)
    {
        /* Don't create a partition when one already exists */
        ASSERT(FALSE);
        return FALSE;
    }
    ASSERT(!PartEntry->Volume);
#endif

    Success = CreatePartition(List,
                              PartEntry,
                              SizeBytes,
                              PartitionInfo);
    if (!Success)
        return Success;

    /* Retrieve the parent item (disk or MBR extended partition) */
    hParentItem = TreeList_GetParent(hList, *phItem);
    hInsertAfter = TreeList_GetPrevSibling(hList, *phItem);
    if (!hInsertAfter)
        hInsertAfter = TVI_FIRST;

    /*
     * The current entry has been recreated and maybe split
     * in two: new partition and remaining unused space.
     * Thus, recreate the current entry.
     *
     * NOTE: Since we create a partition we don't care
     * about its previous PartItem, so it can be deleted.
     */
    {
    /* Cache out the original item's volume create info */
    PVOL_CREATE_INFO VolCreate = PartItem->VolCreate;
    PartItem->VolCreate = NULL;

    /* Remove the current item */
    TreeList_DeleteItem(hList, *phItem);

    /* Recreate the entry */
    *phItem = PrintPartitionData(hList, hParentItem, hInsertAfter, PartEntry);

    /* Retrieve the new PartItem and restore the volume create
     * information associated to the newly-created partition */
    PartItem = GetItemPartition(hList, *phItem);
    ASSERT(PartItem);

    /* Update the volume associated to the create information */
    if (VolCreate)
        VolCreate->Volume = PartItem->Volume;

    /* Restore the volume create information */
    PartItem->VolCreate = VolCreate;

    if (pPartItem)
        *pPartItem = PartItem;
    }

    /* Add also the following unused space, if any */
    // NextPart = GetAdjDiskRegion(NULL, PartEntry, ENUM_REGION_NEXT);
    NextPart = GetAdjUnpartitionedEntry(PartEntry, TRUE);
    if (NextPart /*&& !NextPart->IsPartitioned*/)
        PrintPartitionData(hList, hParentItem, *phItem, NextPart);

    /* Give the focus on and select the created partition */
    // TreeList_SetFocusItem(hList, 1, 1);
    TreeList_SelectItem(hList, *phItem);

    return TRUE;
}

/**
 * @brief
 * Delete the selected partition in the partition list,
 * and update the partition list UI.
 **/
static BOOLEAN
DoDeletePartition(
    _In_ HWND hList,
    _In_ PPARTLIST List,
    _Inout_ HTLITEM* phItem,
    _In_ PPARTITEM PartItem)
{
    PPARTENTRY PartEntry = PartItem->PartEntry;
    PPARTENTRY PrevPart, NextPart;
    BOOLEAN PrevIsPartitioned, NextIsPartitioned;
    BOOLEAN Success;

    HTLITEM hParentItem;
    HTLITEM hInsertAfter;
    HTLITEM hAdjItem;
    PPARTITEM AdjPartItem;

#if 0
    if (!PartEntry->IsPartitioned)
    {
        /* Don't delete an unpartitioned disk region */
        ASSERT(FALSE);
        return FALSE;
    }
#endif

    /*
     * Determine the nature of the previous and next disk regions,
     * in order to know what to do on the corresponding tree items
     * after the selected partition has been deleted.
     *
     * NOTE: Don't reference these pointers after DeletePartition(),
     * since these disk regions might have been coalesced/reallocated.
     * However we can check whether they were NULL or not.
     */
    // PrevPart = GetAdjDiskRegion(NULL, PartEntry, ENUM_REGION_PREV);
    // NextPart = GetAdjDiskRegion(NULL, PartEntry, ENUM_REGION_NEXT);
    PrevPart = GetAdjUnpartitionedEntry(PartEntry, FALSE);
    NextPart = GetAdjUnpartitionedEntry(PartEntry, TRUE);

    PrevIsPartitioned = (PrevPart && PrevPart->IsPartitioned);
    NextIsPartitioned = (NextPart && NextPart->IsPartitioned);

    ASSERT(PartEntry->IsPartitioned); // The current partition must be partitioned.
    Success = DeletePartition(List,
                              PartEntry,
                              &PartEntry);
    if (!Success)
        return Success;

    /* Retrieve the parent item (disk or MBR extended partition) */
    hParentItem = TreeList_GetParent(hList, *phItem);

    /* If previous sibling isn't partitioned, remove it */
    if (PrevPart && !PrevIsPartitioned)
    {
        hAdjItem = TreeList_GetPrevSibling(hList, *phItem);
        ASSERT(hAdjItem); // TODO: Investigate
        if (hAdjItem)
        {
            AdjPartItem = GetItemPartition(hList, hAdjItem);
            ASSERT(AdjPartItem && (AdjPartItem->PartEntry == PrevPart));
            AdjPartItem = NULL;
            TreeList_DeleteItem(hList, hAdjItem);
        }
    }
    /* If next sibling isn't partitioned, remove it */
    if (NextPart && !NextIsPartitioned)
    {
        hAdjItem = TreeList_GetNextSibling(hList, *phItem);
        ASSERT(hAdjItem); // TODO: Investigate
        if (hAdjItem)
        {
            AdjPartItem = GetItemPartition(hList, hAdjItem);
            ASSERT(AdjPartItem && (AdjPartItem->PartEntry == NextPart));
            AdjPartItem = NULL;
            TreeList_DeleteItem(hList, hAdjItem);
        }
    }

    /* We are going to insert the updated entry after
     * either, the original previous sibling (if it is
     * partitioned), or the second-previous sibling
     * (if the first one was already removed, see above) */
    hInsertAfter = TreeList_GetPrevSibling(hList, *phItem);
    if (!hInsertAfter)
        hInsertAfter = TVI_FIRST;

    /* Remove the current item */
    TreeList_DeleteItem(hList, *phItem);

    /* Add back the "new" unpartitioned space */
    *phItem = PrintPartitionData(hList, hParentItem, hInsertAfter, PartEntry);

    /* Give the focus on and select the unpartitioned space */
    // TreeList_SetFocusItem(hList, 1, 1);
    TreeList_SelectItem(hList, *phItem);

    return TRUE;
}


INT_PTR
CALLBACK
DriveDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
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
            SetWindowLongPtrW(hwndDlg, GWLP_USERDATA, (LONG_PTR)pSetupData);

            /* Initially hide and disable all partitioning buttons */
            ShowDlgItem(hwndDlg, IDC_INITDISK, SW_HIDE);
            EnableDlgItem(hwndDlg, IDC_INITDISK, FALSE);
            ShowDlgItem(hwndDlg, IDC_PARTCREATE, SW_HIDE);
            EnableDlgItem(hwndDlg, IDC_PARTCREATE, FALSE);
            ShowDlgItem(hwndDlg, IDC_PARTFORMAT, SW_HIDE);
            EnableDlgItem(hwndDlg, IDC_PARTFORMAT, FALSE);
            ShowDlgItem(hwndDlg, IDC_PARTDELETE, SW_HIDE);
            EnableDlgItem(hwndDlg, IDC_PARTDELETE, FALSE);

            /* Initialize the partitions list */
            hList = GetDlgItem(hwndDlg, IDC_PARTITION);
            UiContext.hPartList = hList;
            InitPartitionList(pSetupData->hInstance, hList);
            DrawPartitionList(hList, pSetupData->PartitionList);

            // HACK: Wine "kwality" code doesn't still implement
            // PSN_QUERYINITIALFOCUS so we "emulate" its call there...
            {
            PSHNOTIFY pshn = {{hwndDlg, GetWindowLong(hwndDlg, GWL_ID), PSN_QUERYINITIALFOCUS}, (LPARAM)hList};
            SendMessageW(hwndDlg, WM_NOTIFY, (WPARAM)pshn.hdr.idFrom, (LPARAM)&pshn);
            }
            break;
        }

        case WM_DESTROY:
        {
            hList = GetDlgItem(hwndDlg, IDC_PARTITION);
            ASSERT(UiContext.hPartList == hList);
            UiContext.hPartList = NULL;
            CleanupPartitionList(hList);
            return TRUE;
        }

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDC_PARTMOREOPTS:
                {
                    DialogBoxParamW(pSetupData->hInstance,
                                    MAKEINTRESOURCEW(IDD_ADVINSTOPTS),
                                    hwndDlg,
                                    MoreOptDlgProc,
                                    (LPARAM)pSetupData);
                    break;
                }

                case IDC_INITDISK:
                {
                    // TODO: Implement disk partitioning initialization
                    break;
                }

                case IDC_PARTCREATE:
                {
                    HTLITEM hItem;
                    PPARTITEM PartItem;
                    PPARTENTRY PartEntry;
                    ULONGLONG PartSize;
                    ULONGLONG MaxPartSize;
                    ULONG MaxSizeMB;
                    INT_PTR ret;
                    PARTCREATE_CTX PartCreateCtx = {0};

                    hList = GetDlgItem(hwndDlg, IDC_PARTITION);
                    PartItem = GetSelectedPartition(hList, &hItem);
                    if (!PartItem)
                    {
                        /* If the button was clicked, an empty disk
                         * region should have been selected first */
                        ASSERT(FALSE);
                        break;
                    }
                    PartEntry = PartItem->PartEntry;
                    if (PartEntry->IsPartitioned)
                    {
                        /* Don't create a partition when one already exists */
                        ASSERT(FALSE);
                        break;
                    }
                    ASSERT(!PartEntry->Volume);

                    /* Get the partition info stored in the TreeList */
                    PartCreateCtx.PartItem = PartItem;

                    /* Retrieve the maximum size in MB (rounded up) the partition can have */
                    MaxPartSize = GetPartEntrySizeInBytes(PartEntry);
                    MaxSizeMB = (ULONG)RoundingDivide(MaxPartSize, MB);
                    PartCreateCtx.MaxSizeMB = MaxSizeMB;

                    /* Don't force formatting by default */
                    PartCreateCtx.ForceFormat = FALSE;

                    /* Show the partitioning dialog */
                    ret = DialogBoxParamW(pSetupData->hInstance,
                                          MAKEINTRESOURCEW(IDD_PARTITION),
                                          hwndDlg,
                                          PartitionDlgProc,
                                          (LPARAM)&PartCreateCtx);
                    if (ret != IDOK)
                        break;

                    /*
                     * If the input size, given in MB, specifies the maximum partition
                     * size, it may slightly under- or over-estimate the latter due to
                     * rounding error. In this case, use all of the unpartitioned space.
                     * Otherwise, directly convert the size to bytes.
                     */
                    PartSize = PartCreateCtx.PartSizeMB;
                    if (PartSize == MaxSizeMB)
                        PartSize = MaxPartSize;
                    else // if (PartSize < MaxSizeMB)
                        PartSize *= MB;

                    ASSERT(PartSize <= MaxPartSize);

                    if (!DoCreatePartition(hList, pSetupData->PartitionList,
                                           &hItem, &PartItem,
                                           PartSize,
                                           !PartCreateCtx.MBRExtPart
                                               ? 0 : PARTITION_EXTENDED))
                    {
                        DisplayError(GetParent(hwndDlg),
                                     IDS_ERROR_CREATE_PARTITION_TITLE,
                                     IDS_ERROR_CREATE_PARTITION);
                    }

                    break;
                }

                case IDC_PARTFORMAT:
                {
                    HTLITEM hItem;
                    PPARTITEM PartItem;
                    PPARTENTRY PartEntry;
                    INT_PTR ret;
                    PARTCREATE_CTX PartCreateCtx = {0};

                    hList = GetDlgItem(hwndDlg, IDC_PARTITION);
                    PartItem = GetSelectedPartition(hList, &hItem);
                    if (!PartItem)
                    {
                        /* If the button was clicked, an empty disk
                         * region should have been selected first */
                        ASSERT(FALSE);
                        break;
                    }
                    PartEntry = PartItem->PartEntry;
                    if (!PartEntry->Volume)
                    {
                        /* Don't format an unformattable partition */
                        ASSERT(FALSE);
                        break;
                    }

                    /* Show the formatting dialog */
                    PartCreateCtx.PartItem = PartItem;
                    ret = DialogBoxParamW(pSetupData->hInstance,
                                          MAKEINTRESOURCEW(IDD_FORMAT),
                                          hwndDlg,
                                          FormatDlgProc,
                                          (LPARAM)&PartCreateCtx);
                    DBG_UNREFERENCED_PARAMETER(ret);
                    break;
                }

                case IDC_PARTDELETE:
                {
                    PPARTITEM PartItem;
                    PPARTENTRY PartEntry;
                    HTLITEM hItem;
                    UINT uIDWarnMsg;

                    hList = GetDlgItem(hwndDlg, IDC_PARTITION);
                    PartItem = GetSelectedPartition(hList, &hItem);
                    if (!PartItem)
                    {
                        // If the button was clicked, a partition
                        // should have been selected first...
                        ASSERT(FALSE);
                        break;
                    }
                    PartEntry = PartItem->PartEntry;
                    if (!PartEntry->IsPartitioned)
                    {
                        /* Don't delete an unpartitioned disk region */
                        ASSERT(FALSE);
                        break;
                    }

                    /* Choose the correct warning message to display:
                     * MBR-extended (container) vs. standard partition */
                    if (PartEntry == PartEntry->DiskEntry->ExtendedPartition)
                        uIDWarnMsg = IDS_WARN_DELETE_MBR_EXTENDED_PARTITION;
                    else
                        uIDWarnMsg = IDS_WARN_DELETE_PARTITION;

                    /* If the user really wants to delete the partition... */
                    if (DisplayMessage(GetParent(hwndDlg),
                                       MB_YESNO | MB_DEFBUTTON2 | MB_ICONWARNING,
                                       MAKEINTRESOURCEW(IDS_WARN_DELETE_PARTITION_TITLE),
                                       MAKEINTRESOURCEW(uIDWarnMsg)) == IDYES)
                    {
                        /* ... make it so! */
                        if (!DoDeletePartition(hList, pSetupData->PartitionList,
                                               &hItem, PartItem))
                        {
                            // TODO: Show error if partition couldn't be deleted?
                        }
                    }

                    break;
                }
            }
            break;
        }

        case WM_NOTIFY:
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;

            // On Vista+ we can use TVN_ITEMCHANGED instead, with NMTVITEMCHANGE* pointer
            if (lpnm->idFrom == IDC_PARTITION && lpnm->code == TVN_SELCHANGED)
            {
                LPNMTREEVIEW pnmv = (LPNMTREEVIEW)lParam;

                // if (!(pnmv->uChanged & TVIF_STATE)) /* The state has changed */
                if (!(pnmv->itemNew.mask & TVIF_STATE))
                    break;

                /* The item has been (de)selected */
                // if (pnmv->uNewState & TVIS_SELECTED)
                if (pnmv->itemNew.state & TVIS_SELECTED)
                {
                    HTLITEM hParentItem = TreeList_GetParent(lpnm->hwndFrom, pnmv->itemNew.hItem);
                    /* May or may not be a PPARTENTRY: this is a PPARTENTRY only when hParentItem != NULL */

                    if (!hParentItem)
                    {
                        /* Hard disk */
                        PDISKENTRY DiskEntry = (PDISKENTRY)pnmv->itemNew.lParam;
                        ASSERT(DiskEntry);

                        /* Show the "Initialize" disk button and hide and disable the others */
                        ShowDlgItem(hwndDlg, IDC_INITDISK, SW_SHOW);

#if 0 // FIXME: Init disk not implemented yet!
                        EnableDlgItem(hwndDlg, IDC_INITDISK,
                                      DiskEntry->DiskStyle == PARTITION_STYLE_RAW);
#else
                        EnableDlgItem(hwndDlg, IDC_INITDISK, FALSE);
#endif

                        ShowDlgItem(hwndDlg, IDC_PARTCREATE, SW_HIDE);
                        EnableDlgItem(hwndDlg, IDC_PARTCREATE, FALSE);

                        ShowDlgItem(hwndDlg, IDC_PARTFORMAT, SW_HIDE);
                        EnableDlgItem(hwndDlg, IDC_PARTFORMAT, FALSE);

                        ShowDlgItem(hwndDlg, IDC_PARTDELETE, SW_HIDE);
                        EnableDlgItem(hwndDlg, IDC_PARTDELETE, FALSE);

                        /* Disable the "Next" button */
                        goto DisableWizNext;
                    }
                    else
                    {
                        /* Partition or unpartitioned space */
                        PPARTITEM PartItem = (PPARTITEM)pnmv->itemNew.lParam;
                        PPARTENTRY PartEntry;
                        ASSERT(PartItem);
                        PartEntry = PartItem->PartEntry;
                        ASSERT(PartEntry);

                        /* Hide and disable the "Initialize" disk button */
                        ShowDlgItem(hwndDlg, IDC_INITDISK, SW_HIDE);
                        EnableDlgItem(hwndDlg, IDC_INITDISK, FALSE);

                        if (!PartEntry->IsPartitioned)
                        {
                            /* Show and enable the "Create" partition button */
                            ShowDlgItem(hwndDlg, IDC_PARTCREATE, SW_SHOW);
                            EnableDlgItem(hwndDlg, IDC_PARTCREATE, TRUE);

                            /* Hide and disable the "Format" button */
                            ShowDlgItem(hwndDlg, IDC_PARTFORMAT, SW_HIDE);
                            EnableDlgItem(hwndDlg, IDC_PARTFORMAT, FALSE);
                        }
                        else
                        {
                            /* Hide and disable the "Create" partition button */
                            ShowDlgItem(hwndDlg, IDC_PARTCREATE, SW_HIDE);
                            EnableDlgItem(hwndDlg, IDC_PARTCREATE, FALSE);

                            /* Show the "Format" button, but enable or disable it if a formattable volume is present */
                            ShowDlgItem(hwndDlg, IDC_PARTFORMAT, SW_SHOW);
                            EnableDlgItem(hwndDlg, IDC_PARTFORMAT, !!PartEntry->Volume);
                        }

                        /* Show the "Delete" partition button, but enable or disable it if the disk region is partitioned */
                        ShowDlgItem(hwndDlg, IDC_PARTDELETE, SW_SHOW);
                        EnableDlgItem(hwndDlg, IDC_PARTDELETE, PartEntry->IsPartitioned);

                        /*
                         * Enable the "Next" button if:
                         *
                         * 1. the selected disk region is partitioned:
                         *    it can either have a volume attached (and be either
                         *    formatted or ready to be formatted),
                         *    or it's not yet formatted (the installer will prompt
                         *    for formatting parameters).
                         *
                         * 2. Or, the selected disk region is not partitioned but
                         *    can be partitioned according to the disk's partitioning
                         *    scheme (the installer will auto-partition the region
                         *    and prompt for formatting parameters).
                         *
                         * In all other cases, the "Next" button is disabled.
                         */

                        // TODO: In the future: first test needs to be augmented with:
                        // (... && PartEntry->Volume->IsSimpleVolume)
                        if ((PartEntry->IsPartitioned && PartEntry->Volume) ||
                            (!PartEntry->IsPartitioned && (PartitionCreationChecks(PartEntry) == NOT_AN_ERROR)))
                        {
                            // ASSERT(PartEntry != PartEntry->DiskEntry->ExtendedPartition);
                            ASSERT(!IsContainerPartition(PartEntry->PartitionType));
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
                    /* Keep the "Next" button disabled. It will be enabled
                     * only when the user selects a valid partition. */
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK);
                }

                break;
            }

            if (lpnm->idFrom == IDC_PARTITION && lpnm->code == TVN_DELETEITEM)
            {
                /* Deleting an item from the partition list */
                LPNMTREEVIEW pnmv = (LPNMTREEVIEW)lParam;
                DeleteTreeItem(lpnm->hwndFrom, (TLITEMW*)&pnmv->itemOld);
                break;
            }

            switch (lpnm->code)
            {
                case PSN_SETACTIVE:
                {
                    /* Keep the "Next" button disabled. It will be enabled
                     * only when the user selects a valid partition. */
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK);
                    break;
                }

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
                    if (DisplayMessage(GetParent(hwndDlg),
                                       MB_YESNO | MB_ICONQUESTION,
                                       MAKEINTRESOURCEW(IDS_ABORTSETUP2),
                                       MAKEINTRESOURCEW(IDS_ABORTSETUP)) == IDYES)
                    {
                        /* Go to the Terminate page */
                        PropSheet_SetCurSelByID(GetParent(hwndDlg), IDD_FINISHPAGE);
                    }

                    /* Do not close the wizard too soon */
                    SetWindowLongPtrW(hwndDlg, DWLP_MSGRESULT, TRUE);
                    return TRUE;
                }

                case PSN_WIZNEXT: /* Set the selected data */
                {
                    HTLITEM hItem;
                    PPARTITEM PartItem;
                    PPARTENTRY PartEntry;

                    hList = GetDlgItem(hwndDlg, IDC_PARTITION);
                    PartItem = GetSelectedPartition(hList, &hItem);
                    if (!PartItem)
                    {
                        /* Fail and don't continue the installation */
                        SetWindowLongPtrW(hwndDlg, DWLP_MSGRESULT, -1);
                        return TRUE;
                    }
                    PartEntry = PartItem->PartEntry;
                    ASSERT(PartEntry);

                    /*
                     * Check whether the user wants to install ReactOS on a disk that
                     * is not recognized by the computer's firmware and if so, display
                     * a warning since such disks may not be bootable.
                     */
                    if (PartEntry->DiskEntry->MediaType == FixedMedia &&
                        !PartEntry->DiskEntry->BiosFound)
                    {
                        INT nRet;

                        nRet = DisplayMessage(hwndDlg,
                                              MB_OKCANCEL | MB_ICONWARNING,
                                              L"Warning",
                                              L"The disk you have selected for installing ReactOS\n"
                                              L"is not visible by the firmware of your computer,\n"
                                              L"and so may not be bootable.\n"
                                              L"\nClick on OK to continue anyway."
                                              L"\nClick on CANCEL to go back to the partitions list.");
                        if (nRet != IDOK)
                        {
                            /* Fail and don't continue the installation */
                            SetWindowLongPtrW(hwndDlg, DWLP_MSGRESULT, -1);
                            return TRUE;
                        }
                    }

                    /* If this is an empty region, auto-create the partition if conditions are OK */
                    if (!PartEntry->IsPartitioned)
                    {
                        ULONG Error;

                        Error = PartitionCreationChecks(PartEntry);
                        if (Error != NOT_AN_ERROR)
                        {
                            // MUIDisplayError(Error, Ir, POPUP_WAIT_ANY_KEY);
                            DisplayMessage(hwndDlg, MB_OK | MB_ICONERROR, NULL,
                                           L"Could not create a partition on the selected disk region");

                            /* Fail and don't continue the installation */
                            SetWindowLongPtrW(hwndDlg, DWLP_MSGRESULT, -1);
                            return TRUE;
                        }

                        /* Automatically create the partition on the whole empty space;
                         * it will be formatted later with default parameters */
                        if (!DoCreatePartition(hList, pSetupData->PartitionList,
                                               &hItem, &PartItem,
                                               0ULL,
                                               0))
                        {
                            DisplayError(GetParent(hwndDlg),
                                         IDS_ERROR_CREATE_PARTITION_TITLE,
                                         IDS_ERROR_CREATE_PARTITION);

                            /* Fail and don't continue the installation */
                            SetWindowLongPtrW(hwndDlg, DWLP_MSGRESULT, -1);
                            return TRUE;
                        }
                        /* Update PartEntry */
                        PartEntry = PartItem->PartEntry;
                    }

                    ASSERT(PartEntry->IsPartitioned);
                    // ASSERT(PartEntry != PartEntry->DiskEntry->ExtendedPartition);
                    ASSERT(!IsContainerPartition(PartEntry->PartitionType));
                    ASSERT(PartEntry->Volume);

#if 0 // TODO: Implement!
                    if (!IsPartitionLargeEnough(PartEntry))
                    {
                        MUIDisplayError(ERROR_INSUFFICIENT_PARTITION_SIZE, Ir, POPUP_WAIT_ANY_KEY,
                                        USetupData.RequiredPartitionDiskSpace);

                        /* Fail and don't continue the installation */
                        SetWindowLongPtrW(hwndDlg, DWLP_MSGRESULT, -1);
                        return TRUE;
                    }
#endif

                    /* Force formatting only if the partition doesn't have a volume (may or may not be formatted) */
                    if (PartEntry->Volume &&
                        ((PartEntry->Volume->FormatState == Formatted) ||
                         (PartItem->VolCreate && *PartItem->VolCreate->FileSystemName)))
                    {
                        /*NOTHING*/;
                    }
                    else /* Request formatting of the selected region if it's not already formatted */
                    {
                        INT_PTR ret;
                        PARTCREATE_CTX PartCreateCtx = {0};

                        /* Show the formatting dialog */
                        PartCreateCtx.PartItem = PartItem;
                        ret = DialogBoxParamW(pSetupData->hInstance,
                                              MAKEINTRESOURCEW(IDD_FORMAT),
                                              hwndDlg,
                                              FormatDlgProc,
                                              (LPARAM)&PartCreateCtx);

                        /* If the user refuses to format the partition,
                         * fail and don't continue the installation */
                        if (ret != IDOK)
                        {
                            SetWindowLongPtrW(hwndDlg, DWLP_MSGRESULT, -1);
                            return TRUE;
                        }

                        /* The partition will be formatted */
                    }

                    InstallPartition = PartEntry;
                    break;
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

/* EOF */
