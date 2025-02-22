/*
 * PROJECT:     ReactOS system properties, control panel applet
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/cpl/sysdm/virtmem.c
 * PURPOSE:     Virtual memory control dialog
 * COPYRIGHT:   Copyright 2006 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include "precomp.h"

#ifdef _M_IX86
/* Used for SharedUserData by GetMaxPageFileSize() in the PAE case */
#define NTOS_MODE_USER
#include <ndk/pstypes.h>
#endif

#define NDEBUG
#include <debug.h>

// #define MAX_PAGING_FILES     16  // See also ntoskrnl/include/internal/mm.h
#define MEGABYTE                        (1024 * 1024)

/* Values adapted from smss/pagefile.c, converted in megabytes and rounded-down.
 * Compare to the more "accurate" values from SMSS (and NTOS) in bytes. */

/* Minimum pagefile size: 2 MB, instead of 256 pages (1 MB) */
#define MINIMUM_PAGEFILE_SIZE           2

/* Maximum pagefile sizes for different architectures */
#define MAXIMUM_PAGEFILE_SIZE32         (4UL * 1024 - 1)
#define MAXIMUM_PAGEFILE_SIZE64         (16UL * 1024 * 1024 - 1)

#if defined(_M_IX86)
/* 4095 MB */
    #define MAXIMUM_PAGEFILE_SIZE       MAXIMUM_PAGEFILE_SIZE32
/* PAE uses the same size as x64 */
    #define MAXIMUM_PAGEFILE_SIZE_PAE   MAXIMUM_PAGEFILE_SIZE64
#elif defined (_M_AMD64) || defined(_M_ARM64)
/* Around 16 TB */
    #define MAXIMUM_PAGEFILE_SIZE       MAXIMUM_PAGEFILE_SIZE64
#elif defined (_M_IA64)
/* Around 32 TB */
    #define MAXIMUM_PAGEFILE_SIZE       (32UL * 1024 * 1024 - 1)
#elif defined(_M_ARM)
/* Around 2 GB */
    #if (NTDDI_VERSION >= NTDDI_WINBLUE) // NTDDI_WIN81
    #define MAXIMUM_PAGEFILE_SIZE       (2UL * 1024 - 1)
    #else
/* Around 4 GB */
    #define MAXIMUM_PAGEFILE_SIZE       MAXIMUM_PAGEFILE_SIZE32
    #endif
#else
/* On unknown architectures, default to either one of the 32 or 64 bit sizes */
#pragma message("Unknown architecture")
    #ifdef _WIN64
    #define MAXIMUM_PAGEFILE_SIZE       MAXIMUM_PAGEFILE_SIZE64
    #else
    #define MAXIMUM_PAGEFILE_SIZE       MAXIMUM_PAGEFILE_SIZE32
    #endif
#endif

typedef struct _PAGEFILE
{
    TCHAR  szDrive[3];
    LPTSTR pszVolume;
    INT    OldMinSize;
    INT    OldMaxSize;
    INT    NewMinSize;
    INT    NewMaxSize;
    UINT   FreeSize;
    BOOL   bIsNotFAT;
    BOOL   bUsed;
} PAGEFILE, *PPAGEFILE;

typedef struct _VIRTMEM
{
    HWND   hSelf;
    HWND   hListBox;
    LPTSTR szPagingFiles;
    UINT   Count;
    BOOL   bModified;
    PAGEFILE PageFile[26];
} VIRTMEM, *PVIRTMEM;


static __inline
UINT
GetMaxPageFileSize(
    _In_ PPAGEFILE PageFile)
{
#ifdef _M_IX86
    /* For x86 PAE-enabled systems, where the maximum pagefile size is
     * greater than 4 GB, verify also that the drive's filesystem on which
     * the pagefile is stored can support it. */
    if (SharedUserData->ProcessorFeatures[PF_PAE_ENABLED] && PageFile->bIsNotFAT)
        return min(PageFile->FreeSize, MAXIMUM_PAGEFILE_SIZE_PAE);
#endif
    return min(PageFile->FreeSize, MAXIMUM_PAGEFILE_SIZE);
}

static LPCTSTR lpKey = _T("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management");

static BOOL
ReadPageFileSettings(PVIRTMEM pVirtMem)
{
    BOOL bRet = FALSE;
    HKEY hkey = NULL;
    DWORD dwType;
    DWORD dwDataSize;

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
                                                        HEAP_ZERO_MEMORY,
                                                        dwDataSize);
            if (pVirtMem->szPagingFiles != NULL)
            {
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
    UINT i = 0;

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
    TCHAR szDrives[1024];    // All drives
    LPTSTR DrivePtr = szDrives;
    TCHAR szDrive[3];        // Single drive
    TCHAR szVolume[MAX_PATH + 1];
    TCHAR szFSName[MAX_PATH + 1];
    INT MinSize;
    INT MaxSize;
    INT DriveLen;
    INT Len;
    UINT PgCnt = 0;
    PPAGEFILE PageFile;

    DriveLen = GetLogicalDriveStrings(_countof(szDrives) - 1,
                                      szDrives);

    while (DriveLen != 0)
    {
        Len = lstrlen(DrivePtr) + 1;
        DriveLen -= Len;

        DrivePtr = _tcsupr(DrivePtr);

        /* Copy the 'X:' portion */
        lstrcpyn(szDrive, DrivePtr, _countof(szDrive));

        if (GetDriveType(DrivePtr) == DRIVE_FIXED)
        {
            MinSize = -1;
            MaxSize = -1;

            /* Does drive match the one in the registry ? */
            if (_tcsnicmp(pVirtMem->szPagingFiles, szDrive, 2) == 0)
            {
                GetPageFileSizes(pVirtMem->szPagingFiles,
                                 &MinSize,
                                 &MaxSize);
            }

            PageFile = &pVirtMem->PageFile[PgCnt];
            PageFile->OldMinSize = MinSize;
            PageFile->OldMaxSize = MaxSize;
            PageFile->NewMinSize = MinSize;
            PageFile->NewMaxSize = MaxSize;
            PageFile->bIsNotFAT  = TRUE; /* Suppose this is not a FAT volume */
            PageFile->bUsed = TRUE;
            lstrcpy(PageFile->szDrive, szDrive);

            /* Get the volume label if there is one */
            if (GetVolumeInformation(DrivePtr,
                                     szVolume, _countof(szVolume),
                                     NULL, NULL, NULL,
                                     szFSName, _countof(szFSName)))
            {
                PageFile->pszVolume = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                                (_tcslen(szVolume) + 1) * sizeof(TCHAR));
                if (PageFile->pszVolume != NULL)
                    _tcscpy(PageFile->pszVolume, szVolume);

                /*
                 * Check whether the volume is FAT, which cannot support files
                 * larger than 4GB (for FAT32 and FATX, and less for FAT16).
                 * This will limit the maximum size of the pagefile this volume
                 * can contain (see GetMaxPageFileSize()).
                 */
                PageFile->bIsNotFAT = (_tcsnicmp(szFSName, _T("FAT"), 3) != 0);
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
    BOOL bErr = TRUE;
    HKEY hk = NULL;
    TCHAR szText[256];
    TCHAR szPagingFiles[2048];
    UINT i, nPos = 0;
    PPAGEFILE PageFile;

    for (i = 0; i < pVirtMem->Count; ++i)
    {
        PageFile = &pVirtMem->PageFile[i];

        if (PageFile->bUsed &&
            PageFile->NewMinSize != -1 &&
            PageFile->NewMaxSize != -1)
        {
            _stprintf(szText,
                      _T("%s\\pagefile.sys %i %i"),
                      PageFile->szDrive,
                      PageFile->NewMinSize,
                      PageFile->NewMaxSize);

            /* Add it to our overall registry string */
            lstrcpy(szPagingFiles + nPos, szText);

            /* Record the position where the next string will start */
            nPos += (UINT)lstrlen(szText) + 1;

            /* Add another NULL for REG_MULTI_SZ */
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
                          (LPBYTE)szPagingFiles,
                          (DWORD)nPos * sizeof(TCHAR)) == ERROR_SUCCESS)
        {
            bErr = FALSE;
        }

        RegCloseKey(hk);
    }

    if (bErr == FALSE)
    {
        /* Delete obsolete paging files on the next boot */
        for (i = 0; i < _countof(pVirtMem->PageFile); i++)
        {
            if (pVirtMem->PageFile[i].OldMinSize != -1 &&
                pVirtMem->PageFile[i].NewMinSize == -1)
            {
                _stprintf(szText,
                          _T("%s\\pagefile.sys"),
                          pVirtMem->PageFile[i].szDrive);

                MoveFileEx(szText, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
            }
        }
    }

    if (bErr)
        ShowLastWin32Error(pVirtMem->hSelf);
}


static VOID
SetListBoxColumns(HWND hwndListBox)
{
    RECT rect = {0, 0, 103, 0};
    MapDialogRect(hwndListBox, &rect);

    SendMessage(hwndListBox, LB_SETTABSTOPS, (WPARAM)1, (LPARAM)&rect.right);
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


static BOOL OnSelChange(PVIRTMEM pVirtMem);

static VOID
InitPagefileList(PVIRTMEM pVirtMem)
{
    INT Index;
    UINT i;
    PPAGEFILE PageFile;
    TCHAR szSize[64];
    TCHAR szDisplayString[256];

    for (i = 0; i < _countof(pVirtMem->PageFile); i++)
    {
        PageFile = &pVirtMem->PageFile[i];

        if (PageFile->bUsed)
        {
            if ((PageFile->NewMinSize == -1) &&
                (PageFile->NewMaxSize == -1))
            {
                LoadString(hApplet,
                           IDS_PAGEFILE_NONE,
                           szSize,
                           _countof(szSize));
            }
            else if ((PageFile->NewMinSize == 0) &&
                     (PageFile->NewMaxSize == 0))
            {
                LoadString(hApplet,
                           IDS_PAGEFILE_SYSTEM,
                           szSize,
                           _countof(szSize));
            }
            else
            {
                _stprintf(szSize, _T("%d - %d"),
                          PageFile->NewMinSize,
                          PageFile->NewMaxSize);
            }

            _stprintf(szDisplayString,
                      _T("%s  [%s]\t%s"),
                      PageFile->szDrive,
                      PageFile->pszVolume ? PageFile->pszVolume : _T(""),
                      szSize);

            Index = SendMessage(pVirtMem->hListBox, LB_ADDSTRING, (WPARAM)0, (LPARAM)szDisplayString);
            SendMessage(pVirtMem->hListBox, LB_SETITEMDATA, Index, i);
        }
    }

    SendMessage(pVirtMem->hListBox, LB_SETCURSEL, (WPARAM)0, (LPARAM)0);

    OnSelChange(pVirtMem);
}


static VOID
UpdatePagefileEntry(PVIRTMEM pVirtMem,
                    INT ListIndex,
                    INT DriveIndex)
{
    PPAGEFILE PageFile = &pVirtMem->PageFile[DriveIndex];
    TCHAR szSize[64];
    TCHAR szDisplayString[256];

    if ((PageFile->NewMinSize == -1) &&
        (PageFile->NewMaxSize == -1))
    {
        LoadString(hApplet,
                   IDS_PAGEFILE_NONE,
                   szSize,
                   _countof(szSize));
    }
    else if ((PageFile->NewMinSize == 0) &&
             (PageFile->NewMaxSize == 0))
    {
        LoadString(hApplet,
                   IDS_PAGEFILE_SYSTEM,
                   szSize,
                   _countof(szSize));
    }
    else
    {
        _stprintf(szSize,
                  _T("%d - %d"),
                  PageFile->NewMinSize,
                  PageFile->NewMaxSize);
    }

    _stprintf(szDisplayString,
              _T("%s  [%s]\t%s"),
              PageFile->szDrive,
              PageFile->pszVolume ? PageFile->pszVolume : _T(""),
              szSize);

    SendMessage(pVirtMem->hListBox, LB_DELETESTRING, (WPARAM)ListIndex, 0);
    SendMessage(pVirtMem->hListBox, LB_INSERTSTRING, (WPARAM)ListIndex, (LPARAM)szDisplayString);
    SendMessage(pVirtMem->hListBox, LB_SETCURSEL, (WPARAM)ListIndex, 0);
}


static VOID
OnSet(PVIRTMEM pVirtMem)
{
    INT Index;
    UINT MinSize = -1;
    UINT MaxSize = -1;
    BOOL bTranslated;
    UINT DriveIndex;
    PPAGEFILE PageFile;

    Index = (INT)SendDlgItemMessage(pVirtMem->hSelf,
                                    IDC_PAGEFILELIST,
                                    LB_GETCURSEL,
                                    0,
                                    0);
    if (Index >= 0 && Index < pVirtMem->Count)
    {
        DriveIndex = SendDlgItemMessage(pVirtMem->hSelf,
                                        IDC_PAGEFILELIST,
                                        LB_GETITEMDATA,
                                        (WPARAM)Index,
                                        0);

        PageFile = &pVirtMem->PageFile[DriveIndex];

        /* Check if custom settings are checked */
        if (IsDlgButtonChecked(pVirtMem->hSelf,
                               IDC_CUSTOM) == BST_CHECKED)
        {
            UINT maxPageFileSize;

            MinSize = GetDlgItemInt(pVirtMem->hSelf,
                                    IDC_INITIALSIZE,
                                    &bTranslated,
                                    FALSE);
            if (!bTranslated)
            {
                ResourceMessageBox(hApplet,
                                   NULL,
                                   MB_ICONWARNING | MB_OK,
                                   IDS_MESSAGEBOXTITLE,
                                   IDS_WARNINITIALSIZE);
                return;
            }

            MaxSize = GetDlgItemInt(pVirtMem->hSelf,
                                    IDC_MAXSIZE,
                                    &bTranslated,
                                    FALSE);
            if (!bTranslated)
            {
                ResourceMessageBox(hApplet,
                                   NULL,
                                   MB_ICONWARNING | MB_OK,
                                   IDS_MESSAGEBOXTITLE,
                                   IDS_WARNMAXIMUMSIZE);
                return;
            }

            maxPageFileSize = GetMaxPageFileSize(PageFile);

            /* Check the valid range of the minimum size */
            if ((MinSize < MINIMUM_PAGEFILE_SIZE) ||
                (MinSize > maxPageFileSize))
            {
                ResourceMessageBox(hApplet,
                                   NULL,
                                   MB_ICONWARNING | MB_OK,
                                   IDS_MESSAGEBOXTITLE,
                                   IDS_WARNINITIALRANGE,
                                   maxPageFileSize);
                return;
            }

            /* Check the valid range of the maximum size */
            if ((MaxSize < MinSize) ||
                (MaxSize > maxPageFileSize))
            {
                ResourceMessageBox(hApplet,
                                   NULL,
                                   MB_ICONWARNING | MB_OK,
                                   IDS_MESSAGEBOXTITLE,
                                   IDS_WARNMAXIMUMRANGE,
                                   maxPageFileSize);
                return;
            }

            // TODO: Check how much disk space would remain after
            // storing a pagefile of a certain size. Warn/error out
            // if less than 5 MB would remain.

            PageFile->NewMinSize = MinSize;
            PageFile->NewMaxSize = MaxSize;
            PageFile->bUsed = TRUE;
        }
        else if (IsDlgButtonChecked(pVirtMem->hSelf,
                                    IDC_NOPAGEFILE) == BST_CHECKED)
        {
            /* No pagefile */
            PageFile->NewMinSize = -1;
            PageFile->NewMaxSize = -1;
            PageFile->bUsed = TRUE;
        }
        else
        {
            /* System managed size*/
            PageFile->NewMinSize = 0;
            PageFile->NewMaxSize = 0;
            PageFile->bUsed = TRUE;
        }

        /* Set the modified flag if min or max size has changed */
        if ((PageFile->OldMinSize != PageFile->NewMinSize) ||
            (PageFile->OldMaxSize != PageFile->NewMaxSize))
        {
            pVirtMem->bModified = TRUE;
        }

        UpdatePagefileEntry(pVirtMem, Index, DriveIndex);
    }
}


static BOOL
OnSelChange(PVIRTMEM pVirtMem)
{
    INT Index;
    UINT DriveIndex;
    PPAGEFILE PageFile;
    MEMORYSTATUSEX MemoryStatus;
    ULARGE_INTEGER FreeDiskSpace;
    UINT i, PageFileSizeMb;
    TCHAR szMegabytes[8];
    TCHAR szBuffer[64];
    TCHAR szText[MAX_PATH];
    WIN32_FIND_DATA fdata = {0};
    HANDLE hFind;
    ULARGE_INTEGER pfSize;

    Index = (INT)SendDlgItemMessage(pVirtMem->hSelf,
                                    IDC_PAGEFILELIST,
                                    LB_GETCURSEL,
                                    0,
                                    0);
    if (Index >= 0 && Index < pVirtMem->Count)
    {
        DriveIndex = SendDlgItemMessage(pVirtMem->hSelf,
                                        IDC_PAGEFILELIST,
                                        LB_GETITEMDATA,
                                        (WPARAM)Index,
                                        0);

        PageFile = &pVirtMem->PageFile[DriveIndex];

        LoadString(hApplet,
                   IDS_PAGEFILE_MB,
                   szMegabytes,
                   _countof(szMegabytes));

        /* Set drive letter */
        SetDlgItemText(pVirtMem->hSelf, IDC_DRIVE,
                       PageFile->szDrive);

        /* Set available disk space */
        if (GetDiskFreeSpaceEx(PageFile->szDrive,
                               NULL, NULL, &FreeDiskSpace))
        {
            PageFile->FreeSize = (UINT)(FreeDiskSpace.QuadPart / MEGABYTE);
            _stprintf(szBuffer, szMegabytes, PageFile->FreeSize);
            SetDlgItemText(pVirtMem->hSelf, IDC_SPACEAVAIL, szBuffer);
        }

        if (PageFile->NewMinSize == -1 &&
            PageFile->NewMaxSize == -1)
        {
            /* No pagefile */
            OnNoPagingFile(pVirtMem);
            CheckDlgButton(pVirtMem->hSelf, IDC_NOPAGEFILE, BST_CHECKED);
        }
        else if (PageFile->NewMinSize == 0 &&
                 PageFile->NewMaxSize == 0)
        {
            /* System managed size */
            OnSysManSize(pVirtMem);
            CheckDlgButton(pVirtMem->hSelf, IDC_SYSMANSIZE, BST_CHECKED);
        }
        else
        {
            /* Custom size */

            /* Enable and fill the custom values */
            OnCustom(pVirtMem);

            SetDlgItemInt(pVirtMem->hSelf,
                          IDC_INITIALSIZE,
                          PageFile->NewMinSize,
                          FALSE);

            SetDlgItemInt(pVirtMem->hSelf,
                          IDC_MAXSIZE,
                          PageFile->NewMaxSize,
                          FALSE);

            CheckDlgButton(pVirtMem->hSelf,
                           IDC_CUSTOM,
                           BST_CHECKED);
        }

        /* Set minimum pagefile size */
        _stprintf(szBuffer, szMegabytes, MINIMUM_PAGEFILE_SIZE);
        SetDlgItemText(pVirtMem->hSelf, IDC_MINIMUM, szBuffer);

        /* Set recommended pagefile size */
        MemoryStatus.dwLength = sizeof(MemoryStatus);
        if (GlobalMemoryStatusEx(&MemoryStatus))
        {
            UINT FreeMemMb, RecoMemMb;
            UINT maxPageFileSize = GetMaxPageFileSize(PageFile);

            FreeMemMb = (UINT)(MemoryStatus.ullTotalPhys / MEGABYTE);
            /* The recommended VM size is 150% of free memory */
            RecoMemMb = FreeMemMb + (FreeMemMb / 2);
            if (RecoMemMb > maxPageFileSize)
                RecoMemMb = maxPageFileSize;
            _stprintf(szBuffer, szMegabytes, RecoMemMb);
            SetDlgItemText(pVirtMem->hSelf, IDC_RECOMMENDED, szBuffer);
        }

        /* Set current pagefile size */
        PageFileSizeMb = 0;

        for (i = 0; i < pVirtMem->Count; i++)
        {
            _stprintf(szText,
                      _T("%c:\\pagefile.sys"),
                      pVirtMem->PageFile[i].szDrive[0]);

            hFind = FindFirstFile(szText, &fdata);
            if (hFind == INVALID_HANDLE_VALUE)
            {
                // FIXME: MsgBox error?
                DPRINT1("Unable to read PageFile size: %ls due to error %d\n",
                        szText, GetLastError());
            }
            else
            {
                pfSize.LowPart = fdata.nFileSizeLow;
                pfSize.HighPart = fdata.nFileSizeHigh;
                PageFileSizeMb += pfSize.QuadPart / MEGABYTE;
                FindClose(hFind);
            }
        }

        _stprintf(szBuffer, szMegabytes, PageFileSizeMb);
        SetDlgItemText(pVirtMem->hSelf, IDC_CURRENT, szBuffer);
    }

    return TRUE;
}


static VOID
OnVirtMemDialogOk(PVIRTMEM pVirtMem)
{
    if (pVirtMem->bModified != FALSE)
    {
        ResourceMessageBox(hApplet,
                           NULL,
                           MB_ICONINFORMATION | MB_OK,
                           IDS_MESSAGEBOXTITLE,
                           IDS_INFOREBOOT);

        WritePageFileSettings(pVirtMem);
    }
}


static VOID
OnInitVirtMemDialog(HWND hwnd, PVIRTMEM pVirtMem)
{
    UINT i;
    PPAGEFILE PageFile;

    pVirtMem->hSelf = hwnd;
    pVirtMem->hListBox = GetDlgItem(hwnd, IDC_PAGEFILELIST);
    pVirtMem->bModified = FALSE;

    SetListBoxColumns(pVirtMem->hListBox);

    for (i = 0; i < _countof(pVirtMem->PageFile); i++)
    {
        PageFile = &pVirtMem->PageFile[i];
        PageFile->bUsed = FALSE;
        PageFile->OldMinSize = -1;
        PageFile->OldMaxSize = -1;
        PageFile->NewMinSize = -1;
        PageFile->NewMaxSize = -1;
        PageFile->FreeSize   = 0;
        PageFile->bIsNotFAT  = TRUE; /* Suppose this is not a FAT volume */
    }

    /* Load the pagefile systems from the reg */
    ReadPageFileSettings(pVirtMem);

    /* Parse our settings and set up dialog */
    ParseMemSettings(pVirtMem);

    InitPagefileList(pVirtMem);
}


static VOID
OnDestroy(PVIRTMEM pVirtMem)
{
    UINT i;

    for (i = 0; i < _countof(pVirtMem->PageFile); i++)
    {
        if (pVirtMem->PageFile[i].pszVolume != NULL)
            HeapFree(GetProcessHeap(), 0, pVirtMem->PageFile[i].pszVolume);
    }

    if (pVirtMem->szPagingFiles)
        HeapFree(GetProcessHeap(), 0, pVirtMem->szPagingFiles);

    HeapFree(GetProcessHeap(), 0, pVirtMem);
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
        {
            pVirtMem = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(VIRTMEM));
            if (pVirtMem == NULL)
            {
                EndDialog(hwndDlg, 0);
                return FALSE;
            }

            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pVirtMem);

            OnInitVirtMemDialog(hwndDlg, pVirtMem);
            break;
        }

        case WM_DESTROY:
            OnDestroy(pVirtMem);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDCANCEL:
                    EndDialog(hwndDlg, 0);
                    return TRUE;

                case IDOK:
                    OnVirtMemDialogOk(pVirtMem);
                    EndDialog(hwndDlg, pVirtMem->bModified);
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
                    switch (HIWORD(wParam))
                    {
                        case LBN_SELCHANGE:
                            OnSelChange(pVirtMem);
                            return TRUE;
                    }
                    break;
            }
            break;
    }

    return FALSE;
}
