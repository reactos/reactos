/*
 * PROJECT:         ReactOS Disk Cleanup
 * LICENSE:         GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:         Utility functions
 * COPYRIGHT:       Copyright 2020 Arnav Bhatt (arnavbhatt288 at gmail dot com)
 */

#include "util.h"

BOOL SystemDrive;
WCHAR DriveLetter[ARR_MAX_SIZE];
WCHAR RappsDir[MAX_PATH];

void AddRequiredItem(HWND hList, UINT StringID, PWCHAR SubString, int ItemIndex)
{
    LVITEMW lvI;
    ZeroMemory(&lvI, sizeof(lvI));

    WCHAR LoadedString[ARR_MAX_SIZE] = { 0 };
    LoadStringW(GetModuleHandleW(NULL), StringID, LoadedString, _countof(LoadedString));

    lvI.mask = LVIF_TEXT | LVIF_IMAGE;
    lvI.cchTextMax = 256;
    lvI.iItem = SendMessage(hList, LVM_GETITEMCOUNT, 0, 0);
    lvI.iSubItem = 0;
    lvI.iImage = ItemIndex;
    lvI.pszText = LoadedString;
    ListView_InsertItem(hList, &lvI);

    if(SubString != NULL)
    {
        lvI.iSubItem = 1;
        lvI.pszText = SubString;    
        ListView_SetItem(hList, &lvI);
    }
}

BOOL ArgCheck(LPWSTR* ArgList, int nArgs)
{
    WCHAR LogicalDrives[ARR_MAX_SIZE] = { 0 };
    WCHAR ArgSpecified[ARR_MAX_SIZE] = { 0 };
    DWORD NumOfDrives = GetLogicalDriveStringsW(sizeof(LogicalDrives), LogicalDrives);

    if (NumOfDrives == 0)
    {
        DPRINT("GetLogicalDriveStringsW(): Failed to get logical drives!\n");
        return FALSE;
    }

    StringCbCopyW(ArgSpecified, sizeof(ArgSpecified), ArgList[1]);
    _wcsupr(ArgSpecified);
    
    if (wcscmp(ArgSpecified, L"/D") == 0 && nArgs == 3)
    {
        if(!DriverunProc(ArgList, LogicalDrives))
        {
            return FALSE;
        }
    }
    else if (wcsstr(ArgSpecified, L"/SAGERUN:") != NULL)
    {
        SagerunProc(nArgs, ArgSpecified, ArgList, LogicalDrives);
        return FALSE;
    }
    else if (wcsstr(ArgSpecified, L"/SAGESET:") != NULL)
    {
        SagesetProc(nArgs, ArgSpecified, ArgList);
        return FALSE;
    }
    else if (wcsstr(ArgSpecified, L"/TUNEUP:") != NULL)
    {
        SagesetProc(nArgs, ArgSpecified, ArgList);
        SagerunProc(nArgs, ArgSpecified, ArgList, LogicalDrives);
        return FALSE;
    }
    else if (wcscmp(ArgSpecified, L"/?") == 0)
    {
        MessageBoxW(NULL, L"cleanmgr /D <drive>", L"Usage", MB_OK);
        return FALSE;
    }
    else
    {
        MessageBoxW(NULL, L"Type cleanmgr /? for help", L"Invalid argument", MB_OK);
        return FALSE;
    }
    
    return TRUE;
}

uint64_t CheckedItem(int ItemIndex, HWND hwnd, HWND hList, long long Size)
{
    DIRECTORIES Dir = ItemIndex;
    uint64_t TempSize = DirectorySizes.TempSize;
    uint64_t RecycleSize = DirectorySizes.RecycleBinSize;
    uint64_t ChkDskSize = DirectorySizes.ChkDskSize;
    uint64_t RappsSize = DirectorySizes.RappsSize;
    switch (Dir)
    {
        case TEMPORARY_FILE:
            if (ListView_GetCheckState(hList, TEMPORARY_FILE))
            {
                CleanDirectories.TempClean = TRUE;
                Size += TempSize;
            }
            else
            {
                CleanDirectories.TempClean = FALSE;
                Size -= TempSize;
            }
            break;

        case RECYCLE_BIN:
            if (ListView_GetCheckState(hList, RECYCLE_BIN))
            {
                CleanDirectories.RecycleClean = TRUE;
                Size += RecycleSize;
            }
            else
            {
                CleanDirectories.RecycleClean = FALSE;
                Size -= RecycleSize;
            }
            break;

        case OLD_CHKDSK_FILES:
            if (ListView_GetCheckState(hList, OLD_CHKDSK_FILES))
            {
                CleanDirectories.ChkDskClean = TRUE;
                Size += ChkDskSize;
            }
            else
            {
                CleanDirectories.ChkDskClean = FALSE;
                Size -= ChkDskSize;
            }
            break;

        case RAPPS_FILES:
            if (ListView_GetCheckState(hList, RAPPS_FILES))
            {
                CleanDirectories.RappsClean = TRUE;
                Size += RappsSize;
            }
            else
            {
                CleanDirectories.RappsClean = FALSE;
                Size -= RappsSize;
            }
            break;
    }

    if (Size < 0)
    {
        Size = 0;
    }
    
    SetTotalSize(Size, IDC_STATIC_SIZE, hwnd);
    return Size;
}

void CleanRequiredPath(PCWSTR TempPath)
{
    SHFILEOPSTRUCTW FileStruct;
    ZeroMemory(&FileStruct, sizeof(FileStruct));

    FileStruct.wFunc = FO_DELETE;
    FileStruct.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_NOCONFIRMMKDIR;
    FileStruct.fAnyOperationsAborted = FALSE;
    FileStruct.pFrom = TempPath;

    SHFileOperationW(&FileStruct);
}

BOOL CreateImageLists(HWND hList)
{
    HICON hbmIcon;
    HIMAGELIST hSmall;
    hSmall = ImageList_Create(16, 16, ILC_COLOR16 | ILC_MASK, 3, 3);

    if (hSmall == NULL)
    {
        return FALSE;
    }

    for (int ItemIndex = 0; ItemIndex < 3; ItemIndex++)
    {
        hbmIcon = LoadIconW(GetModuleHandleW(NULL), MAKEINTRESOURCE(IDI_BLANK + ItemIndex));
        ImageList_AddIcon(hSmall, hbmIcon);
    }

    ListView_SetImageList(hList, hSmall, LVSIL_SMALL);
    return TRUE;
}

BOOL DrawItemCombobox(LPARAM lParam)
{
    COLORREF ClrBackground, ClrForeground;
    TEXTMETRIC tm;
    int x, y;
    size_t cch;
    HBITMAP BitmapIcon = NULL;
    WCHAR achTemp[ARR_MAX_SIZE] = { 0 };
    HBITMAP BitmapMask = NULL;

    LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;

    if (lpdis->itemID == -1)
    {
        return FALSE;
    }

    BitmapMask = LoadBitmapW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDB_MASK));
    if (BitmapMask == NULL)
    {
        DPRINT("LoadBitmapW(): Failed to load the mask bitmap!\n");
        return FALSE;
    }

    BitmapIcon = (HBITMAP)lpdis->itemData;
    if (BitmapIcon == NULL)
    {
        DPRINT("LoadBitmapW(): Failed to load BitmapIcon bitmap!\n");
        return FALSE;
    }

    ClrForeground = SetTextColor(lpdis->hDC,
        GetSysColor(lpdis->itemState & ODS_SELECTED ?
            COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT));

    ClrBackground = SetBkColor(lpdis->hDC,
        GetSysColor(lpdis->itemState & ODS_SELECTED ?
            COLOR_HIGHLIGHT : COLOR_WINDOW));

    GetTextMetrics(lpdis->hDC, &tm);
    y = (lpdis->rcItem.bottom + lpdis->rcItem.top - tm.tmHeight) / 2;
    x = LOWORD(GetDialogBaseUnits()) / 4;

    SendMessage(lpdis->hwndItem, CB_GETLBTEXT,
                lpdis->itemID, (LPARAM)achTemp);

    StringCchLength(achTemp, sizeof(achTemp), &cch);

    ExtTextOut(lpdis->hDC, CX_BITMAP + 2 * x, y,
        ETO_CLIPPED | ETO_OPAQUE, &lpdis->rcItem,
        achTemp, (UINT)cch, NULL);

    SetTextColor(lpdis->hDC, ClrForeground);
    SetBkColor(lpdis->hDC, ClrBackground);

    HDC hdc = CreateCompatibleDC(lpdis->hDC);
    if (hdc == NULL)
    {
        DPRINT("CreateCompatibleDC(): Failed to create a valid handle!\n");
        return FALSE;
    }

    SelectObject(hdc, BitmapMask);
    BitBlt(lpdis->hDC, x, lpdis->rcItem.top + 1,
           CX_BITMAP, CY_BITMAP, hdc, 0, 0, SRCAND);

    SelectObject(hdc, BitmapIcon);
    BitBlt(lpdis->hDC, x, lpdis->rcItem.top + 1,
           CX_BITMAP, CY_BITMAP, hdc, 0, 0, SRCPAINT);

    DeleteDC(hdc);

    if (lpdis->itemState & ODS_FOCUS)
    {
        DrawFocusRect(lpdis->hDC, &lpdis->rcItem);
    }
    return TRUE;
}

BOOL DriverunProc(LPWSTR* ArgList, PWCHAR LogicalDrives)
{
    WCHAR DriveArgGathered[ARR_MAX_SIZE] = { 0 };
    WCHAR TempText[ARR_MAX_SIZE] = { 0 };

    StringCbCopyW(DriveArgGathered, sizeof(DriveArgGathered), ArgList[2]);

    _wcsupr(DriveArgGathered);

    if (wcslen(DriveArgGathered) == 1)
    {
        StringCbCatW(DriveArgGathered, sizeof(DriveArgGathered), L":");
    }

    WCHAR* SingleDrive = LogicalDrives;
    WCHAR RealDrive[ARR_MAX_SIZE] = { 0 };
    while (*SingleDrive)
    {
        if (GetDriveTypeW(SingleDrive) == ONLY_PHYSICAL_DRIVE)
        {
            StringCchCopyW(RealDrive, sizeof(RealDrive), SingleDrive);
            RealDrive[wcslen(RealDrive) - 1] = '\0';
            if (wcscmp(DriveArgGathered, RealDrive) == 0)
            {
                StringCbCopyW(DriveLetter, sizeof(DriveLetter), DriveArgGathered);
                break;
            }
        }
            SingleDrive += wcslen(SingleDrive) + 1;
    }
    if (wcslen(DriveLetter) == 0)
    {
        LoadStringW(GetModuleHandleW(NULL), IDS_ERROR_DRIVE, TempText, _countof(TempText));
        MessageBoxW(NULL, TempText, L"Warning", MB_OK | MB_ICONERROR);
        return FALSE;
    }
    return TRUE;
}

BOOL EnableDialogTheme(HWND hwnd)
{
    HMODULE hUXTheme;
    ETDTProc EnableThemeDialogTexture;

    hUXTheme = LoadLibraryW(L"uxtheme.dll");

    if (hUXTheme)
    {
        EnableThemeDialogTexture = 
            (ETDTProc)GetProcAddress(hUXTheme, "EnableThemeDialogTexture");

        if (EnableThemeDialogTexture)
        {
            EnableThemeDialogTexture(hwnd, ETDT_ENABLETAB);
        }
        else
        {
            FreeLibrary(hUXTheme);
            return FALSE;
        }
    }
    else
    {
        return FALSE;
    }
    return TRUE;
}

DWORD WINAPI FolderRemoval(LPVOID lpParam)
{
    WCHAR LoadedString[ARR_MAX_SIZE] = { 0 };
    WCHAR TargetedDir[MAX_PATH] = { 0 };
    HWND hwnd = lpParam;
    HWND hProgressBar = GetDlgItem(hwnd, IDC_PROGRESS_2);

    if (CleanDirectories.TempClean && SystemDrive)
    {
        CleanRequiredPath(TempDir);

        SendMessageW(hProgressBar, PBM_SETPOS, 25, 0);
        LoadStringW(GetModuleHandleW(NULL), IDS_LABEL_TEMP, LoadedString, _countof(LoadedString));
        SetDlgItemTextW(hwnd, IDC_STATIC_INFO, LoadedString);
    }

    if (CleanDirectories.RecycleClean)
    {   
        StringCbCopyW(TargetedDir, sizeof(TargetedDir), DriveLetter);
        StringCbCatW(TargetedDir, sizeof(TargetedDir), L"\\");

        SHEmptyRecycleBinW(NULL, TargetedDir, SHERB_NOCONFIRMATION | SHERB_NOPROGRESSUI | SHERB_NOSOUND);

        SendMessageW(hProgressBar, PBM_SETPOS, 50, 0);
        LoadStringW(GetModuleHandleW(NULL), IDS_LABEL_RECYCLE, LoadedString, _countof(LoadedString));
        SetDlgItemTextW(hwnd, IDC_STATIC_INFO, LoadedString);
    }

    if (CleanDirectories.ChkDskClean)
    {
        for (int i = 0; i < 10; i++)
        {
            StringCchPrintfW(TargetedDir, sizeof(TargetedDir), L"%s\\FOUND.%.3i", DriveLetter, i);
            if (PathIsDirectoryW(TargetedDir))
            {
                CleanRequiredPath(TargetedDir);
            }
            else
            {
                break;
            }
        }

        SendMessageW(hProgressBar, PBM_SETPOS, 75, 0);
        LoadStringW(GetModuleHandleW(NULL), IDS_LABEL_CHKDSK, LoadedString, _countof(LoadedString));
        SetDlgItemTextW(hwnd, IDC_STATIC_INFO, LoadedString);
    }

    if (CleanDirectories.RappsClean)
    {
        CleanRequiredPath(RappsDir);

        SendMessageW(hProgressBar, PBM_SETPOS, 100, 0);
        LoadStringW(GetModuleHandleW(NULL), IDS_LABEL_RAPPS, LoadedString, _countof(LoadedString));
        SetDlgItemTextW(hwnd, IDC_STATIC_INFO, LoadedString);
    }

    EndDialog(hwnd, 0);
    return TRUE;
}

DWORD WINAPI GetRemovableDirSize(LPVOID lpParam)
{
    HKEY hRegKey = NULL;
    DWORD ArrSize = MAX_PATH;
    HWND hwnd = lpParam;
    HWND hProgressBar = GetDlgItem(hwnd, IDC_PROGRESS_1);
    WCHAR LoadedString[ARR_MAX_SIZE] = { 0 };
    WCHAR TargetedDir[MAX_PATH] = { 0 };
    /*
    SHQUERYRBINFO RecycleBinInfo;
    ZeroMemory(&RecycleBinInfo, sizeof(RecycleBinInfo));
    RecycleBinInfo.cbSize = sizeof(RecycleBinInfo);*/

    if (SystemDrive)
    {
        SendMessageW(hProgressBar, PBM_SETPOS, 25, 0);
        LoadStringW(GetModuleHandleW(NULL), IDS_LABEL_TEMP, LoadedString, _countof(LoadedString));
        SetDlgItemTextW(hwnd, IDC_STATIC_INFO, LoadedString);

        GetTempPathW(sizeof(TargetedDir), TargetedDir);
        if (PathIsDirectoryW(TargetedDir))
        {
            DirectorySizes.TempSize = GetTargetedDirSize(TargetedDir);
            if (DirectorySizes.TempSize == -1)
            {
                return FALSE;
            }
        }
        StringCbCopyW(TempDir, sizeof(TempDir), TargetedDir);
    }

    SendMessageW(hProgressBar, PBM_SETPOS, 50, 0);
    LoadStringW(GetModuleHandleW(NULL), IDS_LABEL_RECYCLE, LoadedString, _countof(LoadedString));
    SetDlgItemTextW(hwnd, IDC_STATIC_INFO, LoadedString);

    /* Hardcoding the path */
    StringCchPrintfW(TargetedDir, sizeof(TargetedDir), L"%s\\RECYCLED", DriveLetter);
    if (!PathIsDirectoryW(TargetedDir))
    {
        StringCchPrintfW(TargetedDir, sizeof(TargetedDir), L"%s\\RECYCLER", DriveLetter);
    }

    DirectorySizes.RecycleBinSize = GetTargetedDirSize(TargetedDir);
    if (DirectorySizes.RecycleBinSize == -1)
    {
        return FALSE;
    }

    /* Currently disabled because SHQueryRecycleBinW isn't implemented in ReactOS */

    /*StringCbCopyW(TargetedDir, sizeof(TargetedDir), DriveLetter);
    StringCbCatW(TargetedDir, sizeof(TargetedDir), L"\\");

    if (SHQueryRecycleBinW(TargetedDir, &RecycleBinInfo) == S_OK)
    {
        DirectorySizes.RecycleBinSize = RecycleBinInfo.i64Size;
        SendMessageW(hProgressBar, PBM_SETPOS, 50, 0);
        SetDlgItemTextW(hwnd, IDC_STATIC_INFO, L"Recycled Files");
    }*/

    for (int i = 0; i < 10; i++)
    {
        StringCchPrintfW(TargetedDir, sizeof(TargetedDir), L"%s\\FOUND.%.3i", DriveLetter, i);
        if (PathIsDirectoryW(TargetedDir))
        {
            SendMessageW(hProgressBar, PBM_SETPOS, 75, 0);
            LoadStringW(GetModuleHandleW(NULL), IDS_LABEL_CHKDSK, LoadedString, _countof(LoadedString));
            SetDlgItemTextW(hwnd, IDC_STATIC_INFO, LoadedString);

            DirectorySizes.ChkDskSize += GetTargetedDirSize(TargetedDir);
            if (DirectorySizes.ChkDskSize == -1)
            {
                return FALSE;
            }
        }

        else
        {
            break;
        }
    }

    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                      L"Software\\ReactOS\\rapps",
                      0,
                      KEY_QUERY_VALUE,
                      &hRegKey) != ERROR_SUCCESS)
    {
        DPRINT("RegOpenKeyExW(): Failed to open a registry key!\n");
    }

    if (RegQueryValueExW(hRegKey, 
                         L"szDownloadDir", 
                         0, 
                         NULL, 
                         (LPBYTE)&TargetedDir, 
                         &ArrSize) != ERROR_SUCCESS)
    {
        DPRINT("RegQueryValueExW(): Failed to query a registry key!\n");
        SHGetFolderPathW(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, TargetedDir);
        StringCbCatW(TargetedDir, sizeof(TargetedDir), L"\\RAPPS Downloads");
    }

    if (PathIsDirectoryW(TargetedDir))
    {
        SendMessageW(hProgressBar, PBM_SETPOS, 100, 0);
        LoadStringW(GetModuleHandleW(NULL), IDS_LABEL_RAPPS, LoadedString, _countof(LoadedString));
        SetDlgItemTextW(hwnd, IDC_STATIC_INFO, LoadedString);

        DirectorySizes.RappsSize = GetTargetedDirSize(TargetedDir);
        if (DirectorySizes.RappsSize == 1)
        {
            return FALSE;
        }
        StringCbCopyW(RappsDir, sizeof(RappsDir), TargetedDir);
    }

    EndDialog(hwnd, 0);
    return TRUE;
}

uint64_t GetTargetedDirSize(PWCHAR SpecifiedDir)
{
    WIN32_FIND_DATAW DataStruct;
    ZeroMemory(&DataStruct, sizeof(DataStruct));
    LARGE_INTEGER DirectoriesSize;
    ZeroMemory(&DirectoriesSize, sizeof(DirectoriesSize));
    HANDLE HandleDir = NULL;
    uint64_t Size = 0;
    WCHAR TargetedDir[MAX_PATH + 1] = { 0 };
    WCHAR ReTargetDir[MAX_PATH + 1] = { 0 };
    DWORD RetError = 0;

    StringCchPrintfW(TargetedDir, sizeof(TargetedDir), L"%s%s", SpecifiedDir, L"\\*");
    HandleDir = FindFirstFileW(TargetedDir, &DataStruct);
    RetError = GetLastError();

    if (RetError != ERROR_NO_MORE_FILES && HandleDir == INVALID_HANDLE_VALUE)
    {
        if (RetError == ERROR_PATH_NOT_FOUND)
        {
            return 0;
        }
        else
        {
            DPRINT("FindFirstFileW(): Failed to search the directory!\n");
            return -1;
        }
    }

    do
    {
        if (wcscmp(DataStruct.cFileName, L".") == 0 || wcscmp(DataStruct.cFileName, L"..") == 0)
        {
            continue;
        }
        else if (DataStruct.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            StringCchPrintfW(ReTargetDir, sizeof(ReTargetDir), L"%s\\%s", SpecifiedDir, DataStruct.cFileName);
            Size += GetTargetedDirSize(ReTargetDir);
        }
        else
        {
            DirectoriesSize.LowPart = DataStruct.nFileSizeLow;
            DirectoriesSize.HighPart = DataStruct.nFileSizeHigh;
            Size += DirectoriesSize.QuadPart;
        }
    }
    while (FindNextFileW(HandleDir, &DataStruct));

    FindClose(HandleDir);
    return Size;
}

PWCHAR GetProperDriveLetter(HWND hComboCtrl, int ItemIndex)
{
    WCHAR *GatheredDriveLetter = NULL;
    WCHAR StringComboBox[ARR_MAX_SIZE] = { 0 };

    SendMessageW(hComboCtrl, CB_GETLBTEXT, (WPARAM)ItemIndex, (LPARAM)StringComboBox);
    StringComboBox[wcslen(StringComboBox) - 1] = L'\0';
    GatheredDriveLetter = wcsrchr(StringComboBox, L':');
    GatheredDriveLetter--;

    return GatheredDriveLetter;
}

PWCHAR GetStageFlag(int nArgs, PWCHAR ArgSpecified, LPWSTR* ArgList)
{
    int Num = 0;
    WCHAR *StrNum = NULL;
    WCHAR *ValStr = NULL;

    ValStr = (PWCHAR)malloc(ARR_MAX_SIZE);

    if (nArgs == 2)
    {
        StrNum = wcsrchr(ArgSpecified, L':');
        StrNum++;
    }
    else if (nArgs == 3)
    {
        StrNum = ArgList[2];
    }

    Num = _wtoi(StrNum);

    StringCchPrintfW(ValStr, ARR_MAX_SIZE, L"StateFlags%.4i", Num);
    return ValStr;
}

BOOL GetStageFlagVal(PWCHAR RegArg, PWCHAR SubKey)
{
    DWORD RetValue = 0;
    DWORD DwordSize = sizeof(DWORD);
    if (RegGetValueW(HKEY_LOCAL_MACHINE,
                     SubKey,
                     RegArg,
                     RRF_RT_DWORD,
                     NULL,
                     (PVOID)&RetValue,
                     &DwordSize) != ERROR_SUCCESS)
    {
        DPRINT("GetStageFlag(): Failed to gather value!\n");
        return FALSE;               
    }

    if (RetValue == ALLOW_FILE_REMOVAL)
    {
        return TRUE;
    }
    return FALSE;
}

void InitListViewControl(HWND hList)
{
    WCHAR TempList[ARR_MAX_SIZE] = { 0 };

    LVCOLUMNW lvC;
    ZeroMemory(&lvC, sizeof(lvC));
    lvC.mask = LVCF_WIDTH | LVCF_SUBITEM | LVCF_FMT;
    lvC.cx = 158;
    lvC.cchTextMax = 256;
    lvC.fmt = LVCFMT_RIGHT;

    SendMessageW(hList, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);

    ListView_InsertColumn(hList, 0, &lvC);
    ListView_InsertColumn(hList, 1, &lvC);

    if (!CreateImageLists(hList))
    {
        DPRINT("CreateImageLists(): Failed to register image list!\n");
        return;
    }

    StrFormatByteSizeW(DirectorySizes.ChkDskSize, TempList, sizeof(TempList));
    AddRequiredItem(hList, IDS_LABEL_CHKDSK, TempList, ICON_BLANK);

    StrFormatByteSizeW(DirectorySizes.RappsSize, TempList, sizeof(TempList));
    AddRequiredItem(hList, IDS_LABEL_RAPPS, TempList, ICON_BLANK);

    StrFormatByteSizeW(DirectorySizes.RecycleBinSize, TempList, sizeof(TempList));
    AddRequiredItem(hList, IDS_LABEL_RECYCLE, TempList, ICON_BIN);

    if (SystemDrive)
    {
        StrFormatByteSizeW(DirectorySizes.TempSize, TempList, sizeof(TempList));
        AddRequiredItem(hList, IDS_LABEL_TEMP, TempList, ICON_BLANK);
    }

    ListView_SetCheckState(hList, 1, 1);
    ListView_SetItemState(hList, 1, LVIS_SELECTED, LVIS_SELECTED);
}

void InitStageFlagListViewControl(HWND hList)
{
    LVCOLUMNW lvC;

    ZeroMemory(&lvC, sizeof(lvC));
    lvC.mask = LVCF_WIDTH | LVCF_SUBITEM | LVCF_FMT;
    lvC.cx = 158;
    lvC.cchTextMax = 256;
    lvC.fmt = LVCFMT_RIGHT;

    SendMessageW(hList, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);

    ListView_InsertColumn(hList, 0, &lvC);
    ListView_InsertColumn(hList, 1, &lvC);

    CreateImageLists(hList);

    AddRequiredItem(hList, IDS_LABEL_CHKDSK, NULL, ICON_BLANK);
    AddRequiredItem(hList, IDS_LABEL_RAPPS, NULL, ICON_BLANK);
    AddRequiredItem(hList, IDS_LABEL_RECYCLE, NULL, ICON_BIN);
    AddRequiredItem(hList, IDS_LABEL_TEMP, NULL, ICON_BLANK);

    ListView_SetCheckState(hList, 1, 1);
    ListView_SetItemState(hList, 1, LVIS_SELECTED, LVIS_SELECTED);
}

void InitStartDlg(HWND hwnd, HBITMAP hBitmap)
{
    DWORD DwIndex = 0;
    DWORD NumOfDrives = 0;
    HWND hComboCtrl = GetDlgItem(hwnd, IDC_DRIVE);
    WCHAR LogicalDrives[ARR_MAX_SIZE] = { 0 };

    NumOfDrives = GetLogicalDriveStringsW(sizeof(LogicalDrives), LogicalDrives);
    if (NumOfDrives == 0)
    {
        DPRINT("GetLogicalDriveStringsW(): Failed to get logical drives!\n");
        EndDialog(hwnd, IDCANCEL);
        return;
    }

    if (NumOfDrives <= sizeof(LogicalDrives))
    {
        WCHAR* SingleDrive = LogicalDrives;
        WCHAR RealDrive[ARR_MAX_SIZE] = { 0 };
        WCHAR StringComboBox[ARR_MAX_SIZE] = { 0 };
        WCHAR VolumeName[ARR_MAX_SIZE] = { 0 };
        while (*SingleDrive)
        {
            if (GetDriveTypeW(SingleDrive) == ONLY_PHYSICAL_DRIVE)
            {
                StringCchCopyW(RealDrive, sizeof(RealDrive), SingleDrive);
                RealDrive[wcslen(RealDrive) - 1] = '\0';
                if (!GetVolumeInformationW(SingleDrive,
                                      VolumeName,
                                      ARR_MAX_SIZE,
                                      NULL,
                                      NULL,
                                      NULL,
                                      NULL,
                                      0))
                {
                    DPRINT("GetVolumeInformationW(): Failed to gather information of the volume!\n");
                    EndDialog(hwnd, IDCANCEL);
                    return;
                }

                StringCchPrintfW(StringComboBox, sizeof(StringComboBox), L"%s (%s)", VolumeName, RealDrive);
                DwIndex = SendMessageW(hComboCtrl, CB_ADDSTRING, 0, (LPARAM)StringComboBox);
                SendMessageW(hComboCtrl, CB_SETITEMDATA, DwIndex, (LPARAM)hBitmap);
            }
            SingleDrive += wcslen(SingleDrive) + 1;
        }
    }

    ComboBox_SetCurSel(hComboCtrl, 0);
    StringCbCopyW(DriveLetter, sizeof(DriveLetter), GetProperDriveLetter(hComboCtrl, 0));
}

BOOL InitTabControl(HWND hwnd)
{
    TCITEMW InsertItem;
    ZeroMemory(&InsertItem, sizeof(InsertItem));

    DialogHandle.hTab = GetDlgItem(hwnd, IDC_TAB);
    DialogHandle.hChoicePage = CreateDialogW(GetModuleHandleW(NULL), MAKEINTRESOURCE(IDD_CHOICE_PAGE), hwnd, ChoicePageDlgProc);
    EnableDialogTheme(DialogHandle.hChoicePage);
    DialogHandle.hOptionsPage = CreateDialogW(GetModuleHandleW(NULL), MAKEINTRESOURCE(IDD_OPTIONS_PAGE), hwnd, OptionsPageDlgProc);
    EnableDialogTheme(DialogHandle.hOptionsPage);

    InsertItem.mask = TCIF_TEXT;
    InsertItem.pszText = L"Disk Cleanup";
    (void)TabCtrl_InsertItem(DialogHandle.hTab, 0, &InsertItem);
    InsertItem.pszText = L"More Options";
    (void)TabCtrl_InsertItem(DialogHandle.hTab, 1, &InsertItem);

    TabControlSelChange();
    return TRUE;
}

BOOL InitStageFlagTabControl(HWND hwnd)
{
    TCITEMW InsertItem;
    ZeroMemory(&InsertItem, sizeof(InsertItem));

    DialogHandle.hTab = GetDlgItem(hwnd, IDC_TAB_SAGESET);
    DialogHandle.hSagesetPage = CreateDialogW(GetModuleHandleW(NULL), MAKEINTRESOURCE(IDD_STAGEFLAG_PAGE), hwnd, SetStageFlagPageDlgProc);
    EnableDialogTheme(DialogHandle.hSagesetPage);

    InsertItem.mask = TCIF_TEXT;
    InsertItem.pszText = L"Disk Cleanup";
    (void)TabCtrl_InsertItem(DialogHandle.hTab, 0, &InsertItem);

    ShowWindow(DialogHandle.hSagesetPage, SW_SHOW);
    BringWindowToTop(DialogHandle.hSagesetPage);
    return TRUE;
}

void TabControlSelChange(void)
{
    switch (TabCtrl_GetCurSel(DialogHandle.hTab))
    {
        case 0:
            ShowWindow(DialogHandle.hChoicePage, SW_SHOW);
            ShowWindow(DialogHandle.hOptionsPage, SW_HIDE);
            BringWindowToTop(DialogHandle.hChoicePage);
            break;

        case 1:
            ShowWindow(DialogHandle.hChoicePage, SW_HIDE);
            ShowWindow(DialogHandle.hOptionsPage, SW_SHOW);
            BringWindowToTop(DialogHandle.hOptionsPage);
            break;
    }
}

void SagerunProc(int nArgs, PWCHAR ArgSpecified, LPWSTR* ArgList, PWCHAR LogicalDrives)
{
    WCHAR *ValStr = NULL;
    WCHAR* SingleDrive = LogicalDrives;
    INT_PTR DialogButtonSelect;

    ValStr = GetStageFlag(nArgs, ArgSpecified, ArgList);

    CleanDirectories.ChkDskClean = GetStageFlagVal(ValStr, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\VolumeCaches\\Old ChkDsk Files");
    CleanDirectories.TempClean = GetStageFlagVal(ValStr, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\VolumeCaches\\Temporary Files");
    CleanDirectories.RappsClean = GetStageFlagVal(ValStr, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\VolumeCaches\\RAPPS Files");
    CleanDirectories.RecycleClean = GetStageFlagVal(ValStr, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\VolumeCaches\\Recycle Bin");

    while (*SingleDrive)
    {
        if (GetDriveTypeW(SingleDrive) == ONLY_PHYSICAL_DRIVE)
        {
            StringCchCopyW(DriveLetter, sizeof(DriveLetter), SingleDrive);
            DriveLetter[wcslen(DriveLetter) - 1] = '\0';

            DialogButtonSelect = DialogBoxParamW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDD_PROGRESS), NULL, ProgressDlgProc, 0);

            if (DialogButtonSelect == IDCANCEL)
            {
                return;
            }

            DialogButtonSelect = DialogBoxParamW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDD_PROGRESS_END), NULL, ProgressEndDlgProc, 0);

            if (DialogButtonSelect == IDCANCEL)
            {
                return;
            }

            ZeroMemory(&DriveLetter, sizeof(DriveLetter));
        }
        SingleDrive += wcslen(SingleDrive) + 1;
    }
}

void SagesetProc(int nArgs, PWCHAR ArgSpecified, LPWSTR* ArgList)
{
    WCHAR *ValStr = GetStageFlag(nArgs, ArgSpecified, ArgList);
    INT_PTR DialogButtonSelect;

    DialogButtonSelect = DialogBoxParamW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDD_STAGEFLAG), NULL, SetStageFlagDlgProc, 0);

    if (DialogButtonSelect == IDCANCEL)
    {
        free(ValStr);
        return;
    }

    if (!SetStageFlagVal(ValStr,
                         L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\VolumeCaches\\Old ChkDsk Files",
                         CleanDirectories.ChkDskClean))
    {
        DPRINT("SetStageFlagVal(): Failed to set a registry value!\n");
    }

    if (!SetStageFlagVal(ValStr,
                         L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\VolumeCaches\\Temporary Files",
                         CleanDirectories.TempClean))
    {
        DPRINT("SetStageFlagVal(): Failed to set a registry value!\n");
    }

    if (!SetStageFlagVal(ValStr,
                         L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\VolumeCaches\\RAPPS Files",
                         CleanDirectories.RappsClean))
    {
        DPRINT("SetStageFlagVal(): Failed to set a registry value!\n");
    }

    if (!SetStageFlagVal(ValStr,
                         L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\VolumeCaches\\Recycle Bin",
                         CleanDirectories.RecycleClean))
    {
        DPRINT("SetStageFlagVal(): Failed to set a registry value!\n");
    }
    free(ValStr);
}

void StageFlagCheckedItem(int ItemIndex, HWND hwnd, HWND hList)
{
    DIRECTORIES Dir = ItemIndex;
    switch (Dir)
    {
        case TEMPORARY_FILE:
            CleanDirectories.TempClean = ListView_GetCheckState(hList, ItemIndex);
            break;

        case RECYCLE_BIN:
            CleanDirectories.RecycleClean = ListView_GetCheckState(hList, ItemIndex);
            break;

        case OLD_CHKDSK_FILES:
            CleanDirectories.ChkDskClean = ListView_GetCheckState(hList, ItemIndex);
            break;

        case RAPPS_FILES:
            CleanDirectories.RappsClean = ListView_GetCheckState(hList, ItemIndex);
            break;
    }
}

void SelItem(HWND hwnd, int ItemIndex)
{
    DIRECTORIES Dir = ItemIndex;
    HWND hButton = GetDlgItem(hwnd, IDC_VIEW_FILES);

    switch (Dir)
    {
        case TEMPORARY_FILE:
            ShowWindow(hButton, SW_HIDE);
            SetDetails(IDS_DETAILS_TEMP, IDC_STATIC_DETAILS, hwnd);
            break;

        case RECYCLE_BIN:
            ShowWindow(hButton, SW_HIDE);
            SetDetails(IDS_DETAILS_RECYCLE, IDC_STATIC_DETAILS, hwnd);
            break;

        case OLD_CHKDSK_FILES:
            ShowWindow(hButton, SW_HIDE);
            SetDetails(IDS_DETAILS_CHKDSK, IDC_STATIC_DETAILS, hwnd);
            break;

        case RAPPS_FILES:
            ShowWindow(hButton, SW_SHOW);
            SetDetails(IDS_DETAILS_RAPPS, IDC_STATIC_DETAILS, hwnd);
            break;
    }
}

void SetDetails(UINT StringID, UINT ResourceID, HWND hwnd)
{
    WCHAR LoadedString[ARR_MAX_SIZE] = { 0 };

    LoadStringW(GetModuleHandleW(NULL), StringID, LoadedString, _countof(LoadedString));
    SetDlgItemTextW(hwnd, ResourceID, LoadedString);
}

BOOL SetStageFlagVal(PWCHAR RegArg, PWCHAR SubKey, BOOL ArgBool)
{
    HKEY hRegKey = NULL;
    DWORD DwordSize = sizeof(DWORD);
    DWORD RegValue = DISALLOW_FILE_REMOVAL;

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                      SubKey,
                      0,
                      KEY_SET_VALUE,
                      &hRegKey) != ERROR_SUCCESS)
    {
        DPRINT("SetStageFlagVal(): Failed to open the key!\n");
        return FALSE;
    }

    if (ArgBool)
    {
        RegValue = ALLOW_FILE_REMOVAL;
    }

    if (RegSetValueExW(hRegKey,
                       RegArg,
                       0,
                       REG_DWORD,
                       (BYTE*)&RegValue,
                       DwordSize) != ERROR_SUCCESS)
    {
        DPRINT("SetStageFlagVal(): Failed to set value!\n");
        return FALSE;
    }
    return TRUE;
}

void SetTotalSize(uint64_t Size, UINT ResourceID, HWND hwnd)
{
    WCHAR LoadedString[ARR_MAX_SIZE] = { 0 };

    StrFormatByteSizeW(Size, LoadedString, ARR_MAX_SIZE);
    SetDlgItemTextW(hwnd, ResourceID, LoadedString);
}

LRESULT APIENTRY ThemeHandler(HWND hDlg, NMCUSTOMDRAW* pNmDraw)
{
    HTHEME hTheme;
    HWND hDlgButtonCtrl;
    LRESULT RetValue;
    INT iState = PBS_NORMAL;

    hDlgButtonCtrl = pNmDraw->hdr.hwndFrom;
    hTheme = GetWindowTheme(hDlgButtonCtrl);

    if (hTheme)
    {
        switch (pNmDraw->dwDrawStage)
        {
            case CDDS_PREPAINT:
            {
                if (pNmDraw->uItemState & CDIS_DEFAULT)
                {
                    iState = PBS_DEFAULTED;
                }
                else if (pNmDraw->uItemState & CDIS_SELECTED)
                {
                    iState = PBS_PRESSED;
                }
                else if (pNmDraw->uItemState & CDIS_HOT)
                {
                    iState = PBS_HOT;
                }

                if (IsThemeBackgroundPartiallyTransparent(hTheme, BP_PUSHBUTTON, iState))
                {
                    DrawThemeParentBackground(hDlgButtonCtrl, pNmDraw->hdc, &pNmDraw->rc);
                }

                DrawThemeBackground(hTheme, pNmDraw->hdc, BP_PUSHBUTTON, iState, &pNmDraw->rc, NULL);

                RetValue = CDRF_SKIPDEFAULT;
                break;
            }

            case CDDS_PREERASE:
            {
                RetValue = CDRF_DODEFAULT;
                break;
            }

            default:
                RetValue = CDRF_SKIPDEFAULT;
                break;
        }
    }
    else
    {
        RetValue = CDRF_DODEFAULT;
    }
    return RetValue;
}
