/*
 * PROJECT:         ReactOS Disk Cleanup
 * LICENSE:         GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:         Utility functions
 * COPYRIGHT:       Copyright 2020 Arnav Bhatt (arnavbhatt288 at gmail dot com)
 */
 
#include "resource.h"
#include "precomp.h"

#define KB 1024
#define MB 1024 * KB
#define GB 1024 * MB
#define TB 1024 * GB

#define ALLOW_FILE_REMOVAL 2
#define DISALLOW_FILE_REMOVAL 3

typedef enum
{
    OLD_CHKDSK_FILES = 0,
    RAPPS_FILES = 1,
    RECYCLE_BIN = 2,
    TEMPORARY_FILE = 3
} DIRECTORIES;

#define ETDT_DISABLE        0x00000001
#define ETDT_ENABLE         0x00000002
#define ETDT_USETABTEXTURE  0x00000004
#define ETDT_ENABLETAB      (ETDT_ENABLE  | ETDT_USETABTEXTURE)

typedef HRESULT (WINAPI * ETDTProc) (HWND, DWORD);

DWORD WINAPI SizeCheck(LPVOID lpParam)
{
    HKEY hRegKey = NULL;
    //SETTINGS_INFO SettingsInfo;
    //ZeroMemory(&SettingsInfo, sizeof(SETTINGS_INFO));
    //DWORD dwSize = sizeof(SettingsInfo);
    
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                      L"Software\\ReactOS\\rapps",
                      0,
                      KEY_QUERY_VALUE,
                      &hRegKey) != ERROR_SUCCESS)
    {
        DPRINT("RegOpenKeyExW(): Failed to open a registry key!\n");
    }
    
    HWND hwnd = (HWND)lpParam;
    
    HWND hProgressBar = NULL;
    WCHAR TargetedDir[MAX_PATH] = { 0 };
    SHQUERYRBINFO RecycleBinInfo;
    ZeroMemory(&RecycleBinInfo, sizeof(RecycleBinInfo));
    RecycleBinInfo.cbSize = sizeof(RecycleBinInfo);

    hProgressBar = GetDlgItem(hwnd, IDC_PROGRESS_1);
    
    if(bv.SysDrive == TRUE)
    {
        SHGetFolderPathW(NULL, CSIDL_WINDOWS, NULL, SHGFP_TYPE_CURRENT, TargetedDir);
        StringCbCatW(TargetedDir, _countof(TargetedDir), L"\\Temp");
        if (PathIsDirectoryW(TargetedDir))
        {
            sz.TempASize = DirSizeFunc(TargetedDir);
            if (sz.TempASize == 1)
            {
                return FALSE;
            }
        }
        SHGetFolderPathW(NULL, CSIDL_PROFILE, NULL, SHGFP_TYPE_CURRENT, TargetedDir);
        StringCbCatW(TargetedDir, _countof(TargetedDir), L"\\Local Settings\\Temp");
        if (PathIsDirectoryW(TargetedDir))
        {
            sz.TempBSize = DirSizeFunc(TargetedDir);
            if (sz.TempBSize == 1)
            {
                return FALSE;
            }
            SendMessageW(hProgressBar, PBM_SETPOS, 25, 0);
            SetDlgItemTextW(hwnd, IDC_STATIC_INFO, L"Temporary Files");
        }
    }
    
    /* Hardcoding the path temporarily */
    
    StringCchPrintfW(TargetedDir, _countof(TargetedDir), L"%s\\RECYCLED", wcv.DriveLetter);
    if(!PathIsDirectoryW(TargetedDir))
    {
        ZeroMemory(&TargetedDir, sizeof(TargetedDir));
        StringCchPrintfW(TargetedDir, _countof(TargetedDir), L"%s\\RECYCLER", wcv.DriveLetter);
    }

    sz.RecycleBinSize = DirSizeFunc(TargetedDir);
    if (sz.TempBSize == 1)
        return FALSE;

    SendMessageW(hProgressBar, PBM_SETPOS, 50, 0);
    SetDlgItemTextW(hwnd, IDC_STATIC_INFO, L"Recycled Files");
    
    
    /* Currently disable because SHQueryRecycleBinW isn't implemented in ReactOS */
    
    /*StringCbCopyW(TargetedDir, _countof(TargetedDir), wcv.DriveLetter);
    StringCbCatW(TargetedDir, _countof(TargetedDir), L"\\");

    if(SHQueryRecycleBinW(TargetedDir, &RecycleBinInfo) == S_OK)
    {
        sz.RecycleBinSize = RecycleBinInfo.i64Size;
        SendMessageW(hProgressBar, PBM_SETPOS, 50, 0);
        SetDlgItemTextW(hwnd, IDC_STATIC_INFO, L"Recycled Files");
    }*/


    for(int i = 0; i < 10; i++)
    {
        StringCchPrintfW(TargetedDir, _countof(TargetedDir), L"%s\\FOUND.%.3i", wcv.DriveLetter, i);
        if (PathIsDirectoryW(TargetedDir))
        {
            sz.ChkDskSize += DirSizeFunc(TargetedDir);
            if (sz.ChkDskSize == 1)
            {
                return FALSE;
            }
            SendMessageW(hProgressBar, PBM_SETPOS, 75, 0);
            SetDlgItemTextW(hwnd, IDC_STATIC_INFO, L"Old ChkDsk Files");
        }
        
        else
            break;
    }

    /*if(RegQueryValueExW(hRegKey, L"Settings", 0, NULL, (LPBYTE)&SettingsInfo, &dwSize) == ERROR_SUCCESS)
        StringCbCopyW(TargetedDir, _countof(TargetedDir), SettingsInfo.szDownloadDir);


    else
    {
        DPRINT("RegQueryValueExW(): Failed to query a registry key!\n");
        SHGetFolderPathW(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, TargetedDir);
        StringCbCatW(TargetedDir, _countof(TargetedDir), L"\\RAPPS Downloads");
    }*/

    SHGetFolderPathW(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, TargetedDir);
    StringCbCatW(TargetedDir, _countof(TargetedDir), L"\\RAPPS Downloads");
    if (PathIsDirectoryW(TargetedDir))
    {
        sz.RappsSize = DirSizeFunc(TargetedDir);
        if (sz.RappsSize == 1)
        {
            return FALSE;
        }
        StringCbCopyW(wcv.RappsDir, _countof(wcv.RappsDir), TargetedDir);
        ZeroMemory(&TargetedDir, sizeof(TargetedDir));
        SendMessageW(hProgressBar, PBM_SETPOS, 100, 0);
        SetDlgItemTextW(hwnd, IDC_STATIC_INFO, L"Temporary RAPPS Files");
    }
    
    SendMessageW(hwnd, WM_DESTROY, 0, 0);
    return TRUE;
}

DWORD WINAPI FolderRemoval(LPVOID lpParam)
{
    HWND hProgressBar = NULL;
    WCHAR TargetedDir[MAX_PATH] = { 0 };
    
    HWND hwnd = (HWND)lpParam;

    hProgressBar = GetDlgItem(hwnd, IDC_PROGRESS_2);

    if (bv.TempClean == TRUE && bv.SysDrive == TRUE)
    {
        SHGetFolderPathW(NULL, CSIDL_WINDOWS, NULL, SHGFP_TYPE_CURRENT, TargetedDir);
        StringCbCatW(TargetedDir, _countof(TargetedDir), L"\\Temp");
        CleanRequiredPath(TargetedDir);
        
        SHGetFolderPathW(NULL, CSIDL_PROFILE, NULL, SHGFP_TYPE_CURRENT, TargetedDir);
        StringCbCatW(TargetedDir, _countof(TargetedDir), L"\\Local Settings\\Temp");
        CleanRequiredPath(TargetedDir);
        
        SendMessageW(hProgressBar, PBM_SETPOS, 25, 0);
        SetDlgItemTextW(hwnd, IDC_STATIC_INFO, L"Temporary files");
    }
    
    if (bv.RecycleClean == TRUE)
    {   
        StringCbCopyW(TargetedDir, _countof(TargetedDir), wcv.DriveLetter);
        StringCbCatW(TargetedDir, _countof(TargetedDir), L"\\");
        
        SHEmptyRecycleBinW(NULL, TargetedDir, SHERB_NOCONFIRMATION | SHERB_NOPROGRESSUI | SHERB_NOSOUND);
        
        SendMessageW(hProgressBar, PBM_SETPOS, 50, 0);
        SetDlgItemTextW(hwnd, IDC_STATIC_INFO, L"Recycled files");
    }
    
    if (bv.ChkDskClean == TRUE)
    {
        for(int i = 0; i < 10; i++)
        {
            StringCchPrintfW(TargetedDir, _countof(TargetedDir), L"%s\\FOUND.%.3i", wcv.DriveLetter, i);
            if (PathIsDirectoryW(TargetedDir))
            {
                CleanRequiredPath(TargetedDir);
            }
        
            else
                break;
        }
        SendMessageW(hProgressBar, PBM_SETPOS, 75, 0);
        SetDlgItemTextW(hwnd, IDC_STATIC_INFO, L"Old ChkDsk files");
    }

    if (bv.RappsClean == TRUE)
    {
        CleanRequiredPath(wcv.RappsDir);

        SendMessageW(hProgressBar, PBM_SETPOS, 100, 0);
        SetDlgItemTextW(hwnd, IDC_STATIC_INFO, L"Temporary RAPPS Files");
    }
    
    SendMessageW(hwnd, WM_DESTROY, 0, 0);

    return TRUE;
}

void CleanRequiredPath(PWCHAR TempPath)
{
    SHFILEOPSTRUCTW FileStruct;
    ZeroMemory(&FileStruct, sizeof(FileStruct));

    int ArrLen = wcslen(TempPath) + 2; // required to set 2 nulls at end of argument to SHFileOperation.
    WCHAR* Path = (WCHAR*) malloc(ArrLen);
    memset(Path, 0, ArrLen);
    StringCbCopyW(Path, ArrLen, TempPath);
    
    FileStruct.wFunc = FO_DELETE;
    FileStruct.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_NOCONFIRMMKDIR;
    FileStruct.fAnyOperationsAborted = FALSE;
    FileStruct.pFrom = TempPath;

    SHFileOperationW(&FileStruct);
    free(Path);
}

uint64_t DirSizeFunc(PWCHAR TargetDir)
{
    WIN32_FIND_DATAW DataStruct;
    ZeroMemory(&DataStruct, sizeof(DataStruct));
    LARGE_INTEGER sz;
    ZeroMemory(&sz, sizeof(sz));
    HANDLE HandleDir = NULL;
    uint64_t Size = 0;
    WCHAR TargetedDir[2048] = { 0 };
    WCHAR ReTargetDir[2048] = { 0 };
    DWORD RetError = 0;
    
    StringCchPrintfW(TargetedDir, _countof(TargetedDir), L"%s%s", TargetDir, L"\\*");
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
            MessageBoxW(NULL, L"FindFirstFileW() failed!", L"Error", MB_OK | MB_ICONERROR);
            return 1;
        }
    }

    do
    {
        if (wcscmp(DataStruct.cFileName, L".") != 0 && wcscmp(DataStruct.cFileName, L"..") != 0)
        {
            if (DataStruct.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                StringCchPrintfW(ReTargetDir, _countof(ReTargetDir), L"%s\\%s", TargetDir, DataStruct.cFileName);
                Size = DirSizeFunc(ReTargetDir);
            }

            else
            {
                sz.LowPart = DataStruct.nFileSizeLow;
                sz.HighPart = DataStruct.nFileSizeHigh;
                Size += sz.QuadPart;
            }
        }
    }

    while (FindNextFileW(HandleDir, &DataStruct));

    FindClose(HandleDir);

    return Size;
}

double SetOptimalSize(uint64_t Size)
{
    if ((double)Size >= (GB))
    {
        return (double)Size / (GB);
    }

    else if ((double)Size >= (MB))
    {
        return (double)Size / (MB);
    }

    else if ((double)Size >= (KB))
    {
        return (double)Size / (KB);
    }

    return (double)Size;
}

void AddItem(HWND hList, PWCHAR String, PWCHAR SubString, int iIndex)
{
    LVITEMW lvI;
    ZeroMemory(&lvI, sizeof(lvI));
    
    lvI.mask = LVIF_TEXT | LVIF_IMAGE;
    lvI.cchTextMax = 256;
    lvI.iItem = SendMessage(hList, LVM_GETITEMCOUNT, 0, 0);
    lvI.iSubItem = 0;
    lvI.iImage = iIndex;
    lvI.pszText = String;
    ListView_InsertItem(hList, &lvI);
    lvI.iSubItem = 1;
    lvI.pszText = SubString;    
    ListView_SetItem(hList, &lvI);
}

BOOL CreateImageLists(HWND hList)
{
    HICON hbmIcon;
    HIMAGELIST hSmall;
    hSmall = ImageList_Create(16, 16, ILC_COLOR16 | ILC_MASK, 3, 3);

    if(hSmall == NULL)
        return FALSE;
    
    for (int index = 0; index < 3; index++)
    {
        hbmIcon = LoadIconW(GetModuleHandleW(NULL), MAKEINTRESOURCE(IDI_BLANK + index));
        ImageList_AddIcon(hSmall, hbmIcon);
        DestroyIcon(hbmIcon);
    }

    ListView_SetImageList(hList, hSmall, LVSIL_SMALL);
    return TRUE;
}

PWCHAR FindOptimalUnit(uint64_t Size)
{
    if ((double)Size >= (GB))
    {
        return L"GB";
    }
    
    else if ((double)Size >= (MB))
    {
        return L"MB";
    }
    
    else if ((double)Size >= (KB))
    {
        return L"KB";
    }
    
    return L"Bytes";
}

void SetDetails(UINT StringID, UINT ResourceID, HWND hwnd)
{
    WCHAR LoadedString[MAX_PATH] = { 0 };

    LoadStringW(GetModuleHandleW(NULL), StringID, LoadedString, _countof(LoadedString));
    SetDlgItemTextW(hwnd, ResourceID, LoadedString);
    memset(LoadedString, 0, sizeof LoadedString);
}

void SetTotalSize(long long Size, UINT ResourceID, HWND hwnd)
{
    WCHAR LoadedString[MAX_PATH] = { 0 };

    StringCchPrintfW(LoadedString, _countof(LoadedString), L"%.02lf %s", SetOptimalSize(Size), FindOptimalUnit(Size));
    SetDlgItemTextW(hwnd, ResourceID, LoadedString);
    memset(LoadedString, 0, sizeof LoadedString);
}

BOOL OnCreate(HWND hwnd)
{
    TCITEMW item;

    dv.hTab = GetDlgItem(hwnd, IDC_TAB);
    dv.hChoicePage = CreateDialogW(GetModuleHandleW(NULL), MAKEINTRESOURCE(IDD_CHOICE_PAGE), hwnd, ChoicePageDlgProc);
    EnableDialogTheme(dv.hChoicePage);
    dv.hOptionsPage = CreateDialogW(GetModuleHandleW(NULL), MAKEINTRESOURCE(IDD_OPTIONS_PAGE), hwnd, OptionsPageDlgProc);
    EnableDialogTheme(dv.hOptionsPage);

    memset(&item, 0, sizeof(item));
    item.mask = TCIF_TEXT;
    item.pszText = L"Disk Cleanup";
    (void)TabCtrl_InsertItem(dv.hTab, 0, &item);

    item.pszText = L"More Options";
    (void)TabCtrl_InsertItem(dv.hTab, 1, &item);

    OnTabWndSelChange();

    return TRUE;
}

BOOL OnCreateSageset(HWND hwnd)
{
    TCITEMW item;

    dv.hTab = GetDlgItem(hwnd, IDC_TAB_SAGESET);
    dv.hSagesetPage = CreateDialogW(GetModuleHandleW(NULL), MAKEINTRESOURCE(IDD_SAGESET_PAGE), hwnd, SagesetPageDlgProc);
    EnableDialogTheme(dv.hSagesetPage);

    memset(&item, 0, sizeof(item));
    item.mask = TCIF_TEXT;
    item.pszText = L"Disk Cleanup";
    (void)TabCtrl_InsertItem(dv.hTab, 0, &item);

    ShowWindow(dv.hSagesetPage, SW_SHOW);
    BringWindowToTop(dv.hSagesetPage);

    return TRUE;
}

void OnTabWndSelChange(void)
{
    switch (TabCtrl_GetCurSel(dv.hTab))
    {
        case 0:
            ShowWindow(dv.hChoicePage, SW_SHOW);
            ShowWindow(dv.hOptionsPage, SW_HIDE);
            BringWindowToTop(dv.hChoicePage);
            break;

        case 1:
            ShowWindow(dv.hChoicePage, SW_HIDE);
            ShowWindow(dv.hOptionsPage, SW_SHOW);
            BringWindowToTop(dv.hOptionsPage);
            break;
    }
}

void SelItem(HWND hwnd, int index)
{
    DIRECTORIES Dir = index;
    switch (Dir)
    {
    case TEMPORARY_FILE:
        ShowWindow(GetDlgItem(hwnd, IDC_VIEW_FILES), SW_HIDE);
        SetDetails(IDS_DETAILS_TEMP, IDC_STATIC_DETAILS, hwnd);
        break;

    case RECYCLE_BIN:
        ShowWindow(GetDlgItem(hwnd, IDC_VIEW_FILES), SW_HIDE);
        SetDetails(IDS_DETAILS_RECYCLE, IDC_STATIC_DETAILS, hwnd);
        break;

    case OLD_CHKDSK_FILES:
        ShowWindow(GetDlgItem(hwnd, IDC_VIEW_FILES), SW_HIDE);
        SetDetails(IDS_DETAILS_CHKDSK, IDC_STATIC_DETAILS, hwnd);
        break;

    case RAPPS_FILES:
        ShowWindow(GetDlgItem(hwnd, IDC_VIEW_FILES), SW_SHOW);
        SetDetails(IDS_DETAILS_RAPPS, IDC_STATIC_DETAILS, hwnd);
        break;

    default:
        break;
    }
}

long long CheckedItem(int index, HWND hwnd, HWND hList, long long Size)
{
    uint64_t TempSize = sz.TempASize + sz.TempBSize;
    uint64_t RecycleSize = sz.RecycleBinSize;
    uint64_t DownloadSize = sz.ChkDskSize;
    uint64_t RappsSize = sz.RappsSize;
    DIRECTORIES Dir = index;
    switch (Dir)
    {
    case TEMPORARY_FILE:
        if (ListView_GetCheckState(hList, index))
        {
            bv.TempClean = TRUE;
            Size += TempSize;
            SetTotalSize(Size, IDC_STATIC_SIZE, hwnd);
        }

        else if (!ListView_GetCheckState(hList, index))
        {
            bv.TempClean = FALSE;
            Size -= TempSize;
            if (0 >= Size)
            {
                SetDlgItemTextW(hwnd, IDC_STATIC_SIZE, L"0.00 Bytes");
                return 0;
            }
            SetTotalSize(Size, IDC_STATIC_SIZE, hwnd);
        }
        return Size;

    case RECYCLE_BIN:
        if (ListView_GetCheckState(hList, index))
        {
            bv.RecycleClean = TRUE;
            Size += RecycleSize;
            SetTotalSize(Size, IDC_STATIC_SIZE, hwnd);
        }

        else
        {
            bv.RecycleClean = FALSE;
            Size -= RecycleSize;
            if (0 >= Size)
            {
                SetDlgItemTextW(hwnd, IDC_STATIC_SIZE, L"0.00 Bytes");
                return 0;
            }
            SetTotalSize(Size, IDC_STATIC_SIZE, hwnd);
        }
        return Size;

    case OLD_CHKDSK_FILES:
        if (ListView_GetCheckState(hList, index))
        {
            bv.ChkDskClean = TRUE;
            Size += DownloadSize;
            SetTotalSize(Size, IDC_STATIC_SIZE, hwnd);
        }

        else
        {
            bv.ChkDskClean = FALSE;
            Size -= DownloadSize;
            if (0 >= Size)
            {
                SetDlgItemTextW(hwnd, IDC_STATIC_SIZE, L"0.00 Bytes");
                return 0;
            }
            SetTotalSize(Size, IDC_STATIC_SIZE, hwnd);
        }
        return Size;

    case RAPPS_FILES:
        if (ListView_GetCheckState(hList, index))
        {
            bv.RappsClean = TRUE;
            Size += RappsSize;
            SetTotalSize(Size, IDC_STATIC_SIZE, hwnd);
        }

        else
        {
            bv.RappsClean = FALSE;
            Size -= RappsSize;
            if (0 >= Size)
            {
                SetDlgItemTextW(hwnd, IDC_STATIC_SIZE, L"0.00 Bytes");
                return 0;
            }
            SetTotalSize(Size, IDC_STATIC_SIZE, hwnd);
        }
        return Size;

    default:
        return 0;
    }
}

void SagesetCheckedItem(int index, HWND hwnd, HWND hList)
{
    DIRECTORIES Dir = index;
    switch (Dir)
    {
    case TEMPORARY_FILE:
        if (ListView_GetCheckState(hList, index))
        {
            bv.TempClean = TRUE;
        }

        else
        {
            bv.TempClean = FALSE;
        }
        break;

    case RECYCLE_BIN:
        if (ListView_GetCheckState(hList, index))
        {
            bv.RecycleClean = TRUE;
        }

        else
        {
            bv.RecycleClean = FALSE;
        }
        break;

    case OLD_CHKDSK_FILES:
        if (ListView_GetCheckState(hList, index))
        {
            bv.ChkDskClean = TRUE;
        }

        else
        {
            bv.ChkDskClean = FALSE;
        }
        break;

    case RAPPS_FILES:
        if (ListView_GetCheckState(hList, index))
        {
            bv.RappsClean = TRUE;
        }

        else
        {
            bv.RappsClean = FALSE;
        }
        break;
    }
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

BOOL EnableDialogTheme(HWND hwnd)
{
    HMODULE hUXTheme;
    ETDTProc EnableThemeDialogTexture;

    hUXTheme = LoadLibraryW(L"uxtheme.dll");

    if(hUXTheme)
    {
        EnableThemeDialogTexture = 
            (ETDTProc)GetProcAddress(hUXTheme, "EnableThemeDialogTexture");

        if(EnableThemeDialogTexture)
        {
            EnableThemeDialogTexture(hwnd, ETDT_ENABLETAB);

            FreeLibrary(hUXTheme);
            return TRUE;
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
}

BOOL ArgCheck(LPWSTR* ArgList, int nArgs)
{
    WCHAR LogicalDrives[MAX_PATH] = { 0 };
    WCHAR ArgReal[MAX_PATH] = { 0 };
    DWORD NumOfDrives = GetLogicalDriveStringsW(MAX_PATH, LogicalDrives);
    
    if (NumOfDrives == 0)
    {
        MessageBoxW(NULL, L"GetLogicalDriveStringsW() failed!", L"Error", MB_OK);
        return FALSE;
    }

    StringCbCopyW(ArgReal, _countof(ArgReal), ArgList[1]);
    _wcsupr(ArgReal);
    
    if (wcscmp(ArgReal, L"/D") == 0 && nArgs == 3)
    {
        if(!DriverunProc(ArgList, LogicalDrives))
            return FALSE;

    }

    else if (wcsstr(ArgReal, L"/SAGERUN:") != NULL)
    {
        SagerunProc(nArgs, ArgReal, ArgList, LogicalDrives);
        return FALSE;
    }
    
    else if (wcsstr(ArgReal, L"/SAGESET:") != NULL)
    {
        SagesetProc(nArgs, ArgReal, ArgList);
        return FALSE;
    }
    
    else if (wcsstr(ArgReal, L"/TUNEUP:") != NULL)
    {
        SagesetProc(nArgs, ArgReal, ArgList);
        SagerunProc(nArgs, ArgReal, ArgList, LogicalDrives);
        return FALSE;
    }
    
    
    else if (wcscmp(ArgReal, L"/?") == 0)
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

BOOL RegValSet(PWCHAR RegArg, PWCHAR SubKey, BOOL ArgBool)
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
        DPRINT("RegValSet(): Failed to open the key!\n");
        return FALSE;
    }
    
    if(ArgBool == TRUE)
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
        DPRINT("RegValSet(): Failed to set value!\n");
        return FALSE;
    }
    
    return TRUE;
}

DWORD RegQuery(PWCHAR RegArg, PWCHAR SubKey)
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
        DPRINT("RegQuery(): Failed to gather value!\n");
        return 0;               
    }
    
    return RetValue;
}

PWCHAR RealStageFlag(int nArgs, PWCHAR ArgReal, LPWSTR* ArgList)
{
    WCHAR *StrNum = NULL;
    WCHAR *ValStr = NULL;
    
    ValStr = (PWCHAR)malloc(MAX_PATH);
    
    int Num = 0;
    
    if (nArgs == 2)
    {
        StrNum = wcsrchr(ArgReal, L':');
        StrNum++;
    }
        
    else if (nArgs == 3)
    {
        StrNum = ArgList[2];
    }
        
    Num = _wtoi(StrNum);
        
    StringCchPrintfW(ValStr, MAX_PATH, L"StateFlags%.4i", Num);
    
    return ValStr;
}

BOOL DriverunProc(LPWSTR* ArgList, PWCHAR LogicalDrives)
{
    WCHAR DriveReal[MAX_PATH] = { 0 };
    WCHAR TempText[MAX_PATH] = { 0 };
    DWORD NumOfDrives = GetLogicalDriveStringsW(MAX_PATH, LogicalDrives);

    StringCbCopyW(DriveReal, _countof(DriveReal), ArgList[2]);

    _wcsupr(DriveReal);

    if (wcslen(DriveReal) == 1)
    {
        StringCbCatW(DriveReal, _countof(DriveReal), L":");
    }

    if (NumOfDrives <= MAX_PATH)
    {
        WCHAR* SingleDrive = LogicalDrives;
        WCHAR RealDrive[MAX_PATH] = { 0 };
        while (*SingleDrive)
        {
            if (GetDriveTypeW(SingleDrive) == ONLY_PHYSICAL_DRIVE)
            {
                StringCchCopyW(RealDrive, MAX_PATH, SingleDrive);
                RealDrive[wcslen(RealDrive) - 1] = '\0';
                if (wcscmp(DriveReal, RealDrive) == 0)
                {
                    StringCbCopyW(wcv.DriveLetter, _countof(wcv.DriveLetter), DriveReal);
                    break;
                }
                memset(RealDrive, 0, sizeof RealDrive);
            }
            SingleDrive += wcslen(SingleDrive) + 1;
        }
        }
    if (wcslen(wcv.DriveLetter) == 0)
    {
        ZeroMemory(&TempText, sizeof(TempText));
        LoadStringW(GetModuleHandleW(NULL), IDS_ERROR_DRIVE, TempText, _countof(TempText));
        MessageBoxW(NULL, TempText, L"Warning", MB_OK | MB_ICONERROR);
        return FALSE;
    }
    
    return TRUE;
}

void SagesetProc(int nArgs, PWCHAR ArgReal, LPWSTR* ArgList)
{
    WCHAR *ValStr = NULL;
    INT_PTR DialogButtonSelect;
        
    ValStr = RealStageFlag(nArgs, ArgReal, ArgList);
        
    DialogButtonSelect = DialogBoxParamW(NULL, MAKEINTRESOURCEW(IDD_SAGERUN), NULL, SagesetDlgProc, 0);
        
    if (DialogButtonSelect == IDCANCEL)
        return;

    if (!RegValSet(ValStr, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\VolumeCaches\\Old ChkDsk Files", bv.ChkDskClean))
        return;
    
    if (!RegValSet(ValStr, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\VolumeCaches\\Temporary Files", bv.TempClean))
        return;
    
    if (!RegValSet(ValStr, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\VolumeCaches\\RAPPS Files", bv.RappsClean))
        return;
    
    if (!RegValSet(ValStr, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\VolumeCaches\\Recycle Bin", bv.RecycleClean))
        return;
}

void SagerunProc(int nArgs, PWCHAR ArgReal, LPWSTR* ArgList, PWCHAR LogicalDrives)
{
    WCHAR *ValStr = NULL;
    WCHAR* SingleDrive = LogicalDrives;
    INT_PTR DialogButtonSelect;
        
    ValStr = RealStageFlag(nArgs, ArgReal, ArgList);
        
    if (RegQuery(ValStr, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\VolumeCaches\\Old ChkDsk Files") == ALLOW_FILE_REMOVAL)
        bv.ChkDskClean = TRUE;
        
    if (RegQuery(ValStr, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\VolumeCaches\\Temporary Files") == ALLOW_FILE_REMOVAL)
        bv.TempClean = TRUE;

    if (RegQuery(ValStr, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\VolumeCaches\\RAPPS Files") == ALLOW_FILE_REMOVAL)
        bv.RappsClean = TRUE;
        
    if (RegQuery(ValStr, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\VolumeCaches\\Recycle Bin") == ALLOW_FILE_REMOVAL)
        bv.RecycleClean = TRUE;
        
    while (*SingleDrive)
    {
        if (GetDriveTypeW(SingleDrive) == ONLY_PHYSICAL_DRIVE)
        {
            StringCchCopyW(wcv.DriveLetter, MAX_PATH, SingleDrive);
            wcv.DriveLetter[wcslen(wcv.DriveLetter) - 1] = '\0';

            DialogButtonSelect = DialogBoxParamW(NULL, MAKEINTRESOURCEW(IDD_PROGRESS), NULL, ProgressDlgProc, 0);

            if(DialogButtonSelect == IDCANCEL)
                return;
        
            DialogBoxParamW(NULL, MAKEINTRESOURCEW(IDD_PROGRESS_END), NULL, ProgressEndDlgProc, 0);
                
            ZeroMemory(&wcv.DriveLetter, sizeof(wcv.DriveLetter));
        }
        SingleDrive += wcslen(SingleDrive) + 1;
    }
}

void InitListViewControl(HWND hList)
{
    WCHAR TempList[MAX_PATH] = { 0 };

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
        MessageBoxW(NULL, L"CreateImageLists() failed!", L"Error", MB_OK | MB_ICONERROR);
    }

    StringCchPrintfW(TempList, _countof(TempList), L"%.02lf %s", SetOptimalSize(sz.ChkDskSize), FindOptimalUnit(sz.ChkDskSize));
    AddItem(hList, L"Old ChkDsk Files", TempList, 0);

    StringCchPrintfW(TempList, _countof(TempList), L"%.02lf %s", SetOptimalSize(sz.RappsSize), FindOptimalUnit(sz.RappsSize));
    AddItem(hList, L"RAPPS Files", TempList, 0);
 
    StringCchPrintfW(TempList, _countof(TempList), L"%.02lf %s", SetOptimalSize(sz.RecycleBinSize), FindOptimalUnit(sz.RecycleBinSize));
    AddItem(hList, L"Recycle Bin", TempList, 2);
 
    if (bv.SysDrive == TRUE)
    {
        StringCchPrintfW(TempList, _countof(TempList), L"%.02lf %s", SetOptimalSize(sz.TempASize + sz.TempBSize), FindOptimalUnit(sz.TempASize + sz.TempBSize));
        AddItem(hList, L"Temporary Files", TempList, 0);
    }

    ListView_SetCheckState(hList, 1, 1);
    ListView_SetItemState(hList, 1, LVIS_SELECTED, LVIS_SELECTED);
}

void InitSagesetListViewControl(HWND hList)
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

    AddItem(hList, L"Old ChkDsk Files", L"", 0);

    AddItem(hList, L"RAPPS Files", L"", 0);
        
    AddItem(hList, L"Temporary Files", L"", 0);
        
    AddItem(hList, L"Recycle Bin", L"", 2);

    ListView_SetCheckState(hList, 1, 1);
    ListView_SetItemState(hList, 1, LVIS_SELECTED, LVIS_SELECTED);
}

void InitStartDlg(HWND hwnd, HBITMAP hBitmap)
{
    DWORD DwIndex = 0;
    DWORD NumOfDrives = 0;
    HWND hComboCtrl = NULL;
    WCHAR LogicalDrives[MAX_PATH] = { 0 };

    hComboCtrl = GetDlgItem(hwnd, IDC_DRIVE);

    if(hComboCtrl == NULL)
    {
        MessageBoxW(NULL, L"GetDlgItem() failed!", L"Error", MB_OK | MB_ICONERROR);
        PostMessage(hwnd, WM_CLOSE, 0, 0);
        return;
    }
  
    NumOfDrives = GetLogicalDriveStringsW(MAX_PATH, LogicalDrives);
    if (NumOfDrives == 0)
    {
        MessageBoxW(NULL, L"GetLogicalDriveStringsW() failed!", L"Error", MB_OK | MB_ICONERROR);
        PostMessage(hwnd, WM_CLOSE, 0, 0);
        return;
    }

    if (NumOfDrives <= MAX_PATH)
    {
        WCHAR* SingleDrive = LogicalDrives;
        WCHAR RealDrive[MAX_PATH] = { 0 };
        while (*SingleDrive)
        {
            if (GetDriveTypeW(SingleDrive) == ONLY_PHYSICAL_DRIVE)
            {
                StringCchCopyW(RealDrive, MAX_PATH, SingleDrive);
                RealDrive[wcslen(RealDrive) - 1] = '\0';
                DwIndex = SendMessageW(hComboCtrl, CB_ADDSTRING, 0, (LPARAM)RealDrive);
                if (SendMessageW(hComboCtrl, CB_SETITEMDATA, DwIndex, (LPARAM)hBitmap) == CB_ERR)
                {
                    MessageBoxW(NULL, L"SendMessageW() failed!", L"Error", MB_OK | MB_ICONERROR);
                    PostMessage(hwnd, WM_CLOSE, 0, 0);
                    return;
                }
                memset(RealDrive, 0, sizeof RealDrive);
            }
            SingleDrive += wcslen(SingleDrive) + 1;
        }
    }
   
    ComboBox_SetCurSel(hComboCtrl, 0);
    SendMessageW(hComboCtrl, CB_GETLBTEXT, (WPARAM)0, (LPARAM)wcv.DriveLetter);
}

BOOL DrawItemCombobox(LPARAM lParam)
{
    COLORREF ClrBackground, ClrForeground;
    TEXTMETRIC tm;
    int x, y;
    size_t cch;
    HBITMAP BitmapIcon = NULL;
    WCHAR achTemp[256] = { 0 };
    HBITMAP BitmapMask = NULL;

    LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;

    if (lpdis->itemID == -1)
        return FALSE;

    BitmapMask = LoadBitmapW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDB_MASK));
 
   if(BitmapMask == NULL)
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

    StringCchLength(achTemp, _countof(achTemp), &cch);

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
        DrawFocusRect(lpdis->hDC, &lpdis->rcItem);

    return TRUE;
}