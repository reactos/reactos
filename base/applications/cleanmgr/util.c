/*
 * PROJECT:         ReactOS Disk Cleanup
 * LICENSE:         GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:         Utility functions
 * COPYRIGHT:       Copyright 2020 Arnav Bhatt (arnavbhatt288 at gmail dot com)
 */

#include "precomp.h"

#define KB 1024
#define MB 1024 * KB
#define GB 1024 * MB
#define TB 1024 * GB

#define ALLOW_FILE_REMOVAL 2
#define DISALLOW_FILE_REMOVAL 0

#define ICON_BIN 2
#define ICON_BLANK 0
#define ICON_FOLDER 1

#define CX_BITMAP 20
#define CY_BITMAP 20

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
    
    HWND hwnd = lpParam;
    
    HWND hProgressBar = NULL;
    WCHAR LoadedString[ARR_MAX_SIZE] = { 0 };
    WCHAR TargetedDir[MAX_PATH] = { 0 };
    SHQUERYRBINFO RecycleBinInfo;
    ZeroMemory(&RecycleBinInfo, sizeof(RecycleBinInfo));
    RecycleBinInfo.cbSize = sizeof(RecycleBinInfo);

    hProgressBar = GetDlgItem(hwnd, IDC_PROGRESS_1);
    
    if (bv.SysDrive)
    {
        GetTempPathW(_countof(TargetedDir), TargetedDir);
        if (PathIsDirectoryW(TargetedDir))
        {
            sz.TempSize = DirSizeFunc(TargetedDir);
            if (sz.TempSize == 1)
            {
                return FALSE;
            }
        }
        StringCbCopyW(wcv.TempDir, _countof(wcv.TempDir), TargetedDir);
        SendMessageW(hProgressBar, PBM_SETPOS, 25, 0);
        LoadStringW(GetModuleHandleW(NULL), IDS_LABEL_TEMP, LoadedString, _countof(LoadedString));
        SetDlgItemTextW(hwnd, IDC_STATIC_INFO, LoadedString);
    }
    
    /* Hardcoding the path temporarily */
    
    StringCchPrintfW(TargetedDir, _countof(TargetedDir), L"%s\\RECYCLED", wcv.DriveLetter);
    if(!PathIsDirectoryW(TargetedDir))
    {
        StringCchPrintfW(TargetedDir, _countof(TargetedDir), L"%s\\RECYCLER", wcv.DriveLetter);
    }

    sz.RecycleBinSize = DirSizeFunc(TargetedDir);
    if (sz.RecycleBinSize == 1)
    {
        return FALSE;
    }

    SendMessageW(hProgressBar, PBM_SETPOS, 50, 0);
    LoadStringW(GetModuleHandleW(NULL), IDS_LABEL_RECYCLE, LoadedString, _countof(LoadedString));
    SetDlgItemTextW(hwnd, IDC_STATIC_INFO, LoadedString);
    
    
    /* Currently disabled because SHQueryRecycleBinW isn't implemented in ReactOS */
    
    /*StringCbCopyW(TargetedDir, _countof(TargetedDir), wcv.DriveLetter);
    StringCbCatW(TargetedDir, _countof(TargetedDir), L"\\");

    if (SHQueryRecycleBinW(TargetedDir, &RecycleBinInfo) == S_OK)
    {
        sz.RecycleBinSize = RecycleBinInfo.i64Size;
        SendMessageW(hProgressBar, PBM_SETPOS, 50, 0);
        SetDlgItemTextW(hwnd, IDC_STATIC_INFO, L"Recycled Files");
    }*/


    for (int i = 0; i < 10; i++)
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
            LoadStringW(GetModuleHandleW(NULL), IDS_LABEL_CHKDSK, LoadedString, _countof(LoadedString));
            SetDlgItemTextW(hwnd, IDC_STATIC_INFO, LoadedString);
        }
        
        else
        {
            break;
        }
    }

    /*if (RegQueryValueExW(hRegKey, L"Settings", 0, NULL, (LPBYTE)&SettingsInfo, &dwSize) == ERROR_SUCCESS)
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
        SendMessageW(hProgressBar, PBM_SETPOS, 100, 0);
        LoadStringW(GetModuleHandleW(NULL), IDS_LABEL_RAPPS, LoadedString, _countof(LoadedString));
        SetDlgItemTextW(hwnd, IDC_STATIC_INFO, LoadedString);
    }
    
    SendMessageW(hwnd, WM_DESTROY, 0, 0);
    return TRUE;
}

DWORD WINAPI FolderRemoval(LPVOID lpParam)
{
    HWND hProgressBar = NULL;
    WCHAR LoadedString[ARR_MAX_SIZE] = { 0 };
    WCHAR TargetedDir[MAX_PATH] = { 0 };
    
    HWND hwnd = lpParam;

    hProgressBar = GetDlgItem(hwnd, IDC_PROGRESS_2);

    if (bv.TempClean && bv.SysDrive)
    {
        CleanRequiredPath(wcv.TempDir);
        
        SendMessageW(hProgressBar, PBM_SETPOS, 25, 0);
        LoadStringW(GetModuleHandleW(NULL), IDS_LABEL_TEMP, LoadedString, _countof(LoadedString));
        SetDlgItemTextW(hwnd, IDC_STATIC_INFO, LoadedString);
    }
    
    if (bv.RecycleClean)
    {   
        StringCbCopyW(TargetedDir, _countof(TargetedDir), wcv.DriveLetter);
        StringCbCatW(TargetedDir, _countof(TargetedDir), L"\\");
        
        SHEmptyRecycleBinW(NULL, TargetedDir, SHERB_NOCONFIRMATION | SHERB_NOPROGRESSUI | SHERB_NOSOUND);
        
        SendMessageW(hProgressBar, PBM_SETPOS, 50, 0);
        LoadStringW(GetModuleHandleW(NULL), IDS_LABEL_RECYCLE, LoadedString, _countof(LoadedString));
        SetDlgItemTextW(hwnd, IDC_STATIC_INFO, LoadedString);
    }
    
    if (bv.ChkDskClean)
    {
        for (int i = 0; i < 10; i++)
        {
            StringCchPrintfW(TargetedDir, _countof(TargetedDir), L"%s\\FOUND.%.3i", wcv.DriveLetter, i);
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

    if (bv.RappsClean)
    {
        CleanRequiredPath(wcv.RappsDir);

        SendMessageW(hProgressBar, PBM_SETPOS, 100, 0);
        LoadStringW(GetModuleHandleW(NULL), IDS_LABEL_RAPPS, LoadedString, _countof(LoadedString));
        SetDlgItemTextW(hwnd, IDC_STATIC_INFO, LoadedString);
    }
    
    SendMessageW(hwnd, WM_DESTROY, 0, 0);

    return TRUE;
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

uint64_t DirSizeFunc(PWCHAR TargetDir)
{
    WIN32_FIND_DATAW DataStruct;
    ZeroMemory(&DataStruct, sizeof(DataStruct));
    LARGE_INTEGER sz;
    ZeroMemory(&sz, sizeof(sz));
    HANDLE HandleDir = NULL;
    uint64_t Size = 0;
    WCHAR TargetedDir[MAX_PATH + 1] = { 0 };
    WCHAR ReTargetDir[MAX_PATH + 1] = { 0 };
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
        if (wcscmp(DataStruct.cFileName, L".") == 0 || wcscmp(DataStruct.cFileName, L"..") == 0)
        {
            continue;
        }

        else if (DataStruct.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            StringCchPrintfW(ReTargetDir, _countof(ReTargetDir), L"%s\\%s", TargetDir, DataStruct.cFileName);
            Size += DirSizeFunc(ReTargetDir);
        }

        else
        {
            sz.LowPart = DataStruct.nFileSizeLow;
            sz.HighPart = DataStruct.nFileSizeHigh;
            Size += sz.QuadPart;
        }
    }
    while (FindNextFileW(HandleDir, &DataStruct));

    FindClose(HandleDir);

    return Size;
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

    if (hSmall == NULL)
    {
        return FALSE;
    }

    for (int index = 0; index < 3; index++)
    {
        hbmIcon = LoadIconW(GetModuleHandleW(NULL), MAKEINTRESOURCE(IDI_BLANK + index));
        ImageList_AddIcon(hSmall, hbmIcon);
    }

    ListView_SetImageList(hList, hSmall, LVSIL_SMALL);
    return TRUE;
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

    if (hUXTheme)
    {
        EnableThemeDialogTexture = 
            (ETDTProc)GetProcAddress(hUXTheme, "EnableThemeDialogTexture");

        if (EnableThemeDialogTexture)
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
    WCHAR LogicalDrives[ARR_MAX_SIZE] = { 0 };
    WCHAR ArgReal[ARR_MAX_SIZE] = { 0 };
    DWORD NumOfDrives = GetLogicalDriveStringsW(_countof(LogicalDrives), LogicalDrives);
    
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
        {
            return FALSE;
        }
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
    
    ValStr = (PWCHAR)malloc(ARR_MAX_SIZE);
    
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

    StringCchPrintfW(ValStr, ARR_MAX_SIZE, L"StateFlags%.4i", Num);

    return ValStr;
}

BOOL DriverunProc(LPWSTR* ArgList, PWCHAR LogicalDrives)
{
    WCHAR DriveArgGathered[ARR_MAX_SIZE] = { 0 };
    WCHAR TempText[ARR_MAX_SIZE] = { 0 };

    StringCbCopyW(DriveArgGathered, _countof(DriveArgGathered), ArgList[2]);

    _wcsupr(DriveArgGathered);

    if (wcslen(DriveArgGathered) == 1)
    {
        StringCbCatW(DriveArgGathered, _countof(DriveArgGathered), L":");
    }

    WCHAR* SingleDrive = LogicalDrives;
    WCHAR RealDrive[ARR_MAX_SIZE] = { 0 };
    while (*SingleDrive)
    {
        if (GetDriveTypeW(SingleDrive) == ONLY_PHYSICAL_DRIVE)
        {
            StringCchCopyW(RealDrive, _countof(RealDrive), SingleDrive);
            RealDrive[wcslen(RealDrive) - 1] = '\0';
            if (wcscmp(DriveArgGathered, RealDrive) == 0)
            {
                StringCbCopyW(wcv.DriveLetter, _countof(wcv.DriveLetter), DriveArgGathered);
                break;
            }
        }
            SingleDrive += wcslen(SingleDrive) + 1;
    }
    if (wcslen(wcv.DriveLetter) == 0)
    {
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
        
    DialogButtonSelect = DialogBoxParamW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDD_SAGERUN), NULL, SagesetDlgProc, 0);
        
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
            StringCchCopyW(wcv.DriveLetter, _countof(wcv.DriveLetter), SingleDrive);
            wcv.DriveLetter[wcslen(wcv.DriveLetter) - 1] = '\0';

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

            ZeroMemory(&wcv.DriveLetter, sizeof(wcv.DriveLetter));
        }
        SingleDrive += wcslen(SingleDrive) + 1;
    }
}

void InitStartDlg(HWND hwnd, HBITMAP hBitmap)
{
    DWORD DwIndex = 0;
    DWORD NumOfDrives = 0;
    HWND hComboCtrl = NULL;
    WCHAR *GatheredDriveLatter = NULL;
    WCHAR LogicalDrives[ARR_MAX_SIZE] = { 0 };
    WCHAR StringComboBox[ARR_MAX_SIZE] = { 0 };

    hComboCtrl = GetDlgItem(hwnd, IDC_DRIVE);

    if (hComboCtrl == NULL)
    {
        MessageBoxW(NULL, L"GetDlgItem() failed!", L"Error", MB_OK | MB_ICONERROR);
        PostMessage(hwnd, WM_CLOSE, 0, 0);
        return;
    }
  
    NumOfDrives = GetLogicalDriveStringsW(_countof(LogicalDrives), LogicalDrives);
    if (NumOfDrives == 0)
    {
        MessageBoxW(NULL, L"GetLogicalDriveStringsW() failed!", L"Error", MB_OK | MB_ICONERROR);
        PostMessage(hwnd, WM_CLOSE, 0, 0);
        return;
    }

    if (NumOfDrives <= _countof(LogicalDrives))
    {
        WCHAR* SingleDrive = LogicalDrives;
        WCHAR RealDrive[ARR_MAX_SIZE] = { 0 };
        WCHAR VolumeName[ARR_MAX_SIZE] = { 0 };
        WCHAR StringComboBox[ARR_MAX_SIZE] = { 0 };
        while (*SingleDrive)
        {
            if (GetDriveTypeW(SingleDrive) == ONLY_PHYSICAL_DRIVE)
            {
                StringCchCopyW(RealDrive, _countof(RealDrive), SingleDrive);
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
                    MessageBoxW(NULL, L"GetVolumeInformationW() failed!", L"Error", MB_OK | MB_ICONERROR);
                    PostMessage(hwnd, WM_CLOSE, 0, 0);
                    return;
                }

                StringCchPrintfW(StringComboBox, _countof(StringComboBox), L"%s (%s)", VolumeName, RealDrive);
                DwIndex = SendMessageW(hComboCtrl, CB_ADDSTRING, 0, (LPARAM)StringComboBox);
                if (SendMessageW(hComboCtrl, CB_SETITEMDATA, DwIndex, (LPARAM)hBitmap) == CB_ERR)
                {
                    MessageBoxW(NULL, L"SendMessageW() failed!", L"Error", MB_OK | MB_ICONERROR);
                    PostMessage(hwnd, WM_CLOSE, 0, 0);
                    return;
                }
            }
            SingleDrive += wcslen(SingleDrive) + 1;
        }
    }
   
    ComboBox_SetCurSel(hComboCtrl, 0);
    SendMessageW(hComboCtrl, CB_GETLBTEXT, (WPARAM)0, (LPARAM)StringComboBox);
    StringComboBox[wcslen(StringComboBox) - 1] = L'\0';
    GatheredDriveLatter = wcsrchr(StringComboBox, L':');
    GatheredDriveLatter--;
    StringCbCopyW(wcv.DriveLetter, _countof(wcv.DriveLetter), GatheredDriveLatter);
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
    {
        DrawFocusRect(lpdis->hDC, &lpdis->rcItem);
    }

    return TRUE;
}

void InitListViewControl(HWND hList)
{
    WCHAR TempList[ARR_MAX_SIZE] = { 0 };
    WCHAR LoadedString[ARR_MAX_SIZE] = { 0 };

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

    StrFormatByteSizeW(sz.ChkDskSize, TempList, _countof(TempList));
    LoadStringW(GetModuleHandleW(NULL), IDS_LABEL_CHKDSK, LoadedString, _countof(LoadedString));
    AddItem(hList, LoadedString, TempList, ICON_BLANK);

    StrFormatByteSizeW(sz.RappsSize, TempList, _countof(TempList));
    LoadStringW(GetModuleHandleW(NULL), IDS_LABEL_RAPPS, LoadedString, _countof(LoadedString));
    AddItem(hList, LoadedString, TempList, ICON_BLANK);

    StrFormatByteSizeW(sz.RecycleBinSize, TempList, _countof(TempList));
    LoadStringW(GetModuleHandleW(NULL), IDS_LABEL_RECYCLE, LoadedString, _countof(LoadedString));
    AddItem(hList, LoadedString, TempList, ICON_BIN);
 
    if (bv.SysDrive)
    {
        StrFormatByteSizeW(sz.TempSize, TempList, _countof(TempList));
        LoadStringW(GetModuleHandleW(NULL), IDS_LABEL_TEMP, LoadedString, _countof(LoadedString));
        AddItem(hList, LoadedString, TempList, ICON_BLANK);
    }

    ListView_SetCheckState(hList, 1, 1);
    ListView_SetItemState(hList, 1, LVIS_SELECTED, LVIS_SELECTED);
}

void InitSagesetListViewControl(HWND hList)
{
    LVCOLUMNW lvC;
    WCHAR LoadedString[ARR_MAX_SIZE] = { 0 };

    ZeroMemory(&lvC, sizeof(lvC));
    lvC.mask = LVCF_WIDTH | LVCF_SUBITEM | LVCF_FMT;
    lvC.cx = 158;
    lvC.cchTextMax = 256;
    lvC.fmt = LVCFMT_RIGHT;
    
    SendMessageW(hList, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);
    
    ListView_InsertColumn(hList, 0, &lvC);
    ListView_InsertColumn(hList, 1, &lvC);

    CreateImageLists(hList);

    LoadStringW(GetModuleHandleW(NULL), IDS_LABEL_CHKDSK, LoadedString, _countof(LoadedString));
    AddItem(hList, LoadedString, L"", ICON_BLANK);

    LoadStringW(GetModuleHandleW(NULL), IDS_LABEL_RAPPS, LoadedString, _countof(LoadedString));
    AddItem(hList, LoadedString, L"", ICON_BLANK);
    
    LoadStringW(GetModuleHandleW(NULL), IDS_LABEL_RECYCLE, LoadedString, _countof(LoadedString));
    AddItem(hList, LoadedString, L"", ICON_BIN);

    LoadStringW(GetModuleHandleW(NULL), IDS_LABEL_TEMP, LoadedString, _countof(LoadedString));
    AddItem(hList, LoadedString, L"", ICON_BLANK);

    ListView_SetCheckState(hList, 1, 1);
    ListView_SetItemState(hList, 1, LVIS_SELECTED, LVIS_SELECTED);
}

BOOL OnCreate(HWND hwnd)
{
    TCITEMW item;

    dv.hTab = GetDlgItem(hwnd, IDC_TAB);
    dv.hChoicePage = CreateDialogW(GetModuleHandleW(NULL), MAKEINTRESOURCE(IDD_CHOICE_PAGE), hwnd, ChoicePageDlgProc);
    EnableDialogTheme(dv.hChoicePage);
    dv.hOptionsPage = CreateDialogW(GetModuleHandleW(NULL), MAKEINTRESOURCE(IDD_OPTIONS_PAGE), hwnd, OptionsPageDlgProc);
    EnableDialogTheme(dv.hOptionsPage);

    ZeroMemory(&item, sizeof(item));
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

    ZeroMemory(&item, sizeof(item));
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
    }
}

long long CheckedItem(int index, HWND hwnd, HWND hList, long long Size)
{
    uint64_t TempSize = sz.TempSize;
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
        }

        else
        {
            bv.TempClean = FALSE;
            Size -= TempSize;
        }
        break;

    case RECYCLE_BIN:
        if (ListView_GetCheckState(hList, index))
        {
            bv.RecycleClean = TRUE;
            Size += RecycleSize;
        }

        else
        {
            bv.RecycleClean = FALSE;
            Size -= RecycleSize;
        }
        break;

    case OLD_CHKDSK_FILES:
        if (ListView_GetCheckState(hList, index))
        {
            bv.ChkDskClean = TRUE;
            Size += DownloadSize;
        }

        else
        {
            bv.ChkDskClean = FALSE;
            Size -= DownloadSize;
        }
        break;

    case RAPPS_FILES:
        if (ListView_GetCheckState(hList, index))
        {
            bv.RappsClean = TRUE;
            Size += RappsSize;
        }

        else
        {
            bv.RappsClean = FALSE;
            Size -= RappsSize;
        }
        break;
    }

    if (Size < 0)
    {
        Size = 0;
        SetTotalSize(Size, IDC_STATIC_SIZE, hwnd);
        return Size;
    }
    
    SetTotalSize(Size, IDC_STATIC_SIZE, hwnd);
    return Size;
}

void SagesetCheckedItem(int index, HWND hwnd, HWND hList)
{
    DIRECTORIES Dir = index;
    switch (Dir)
    {
    case TEMPORARY_FILE:
        bv.TempClean = ListView_GetCheckState(hList, index);
        break;

    case RECYCLE_BIN:
        bv.RecycleClean = ListView_GetCheckState(hList, index);
        break;

    case OLD_CHKDSK_FILES:
        bv.ChkDskClean = ListView_GetCheckState(hList, index);
        break;

    case RAPPS_FILES:
        bv.RappsClean = ListView_GetCheckState(hList, index);
        break;
    }
}

void SetDetails(UINT StringID, UINT ResourceID, HWND hwnd)
{
    WCHAR LoadedString[ARR_MAX_SIZE] = { 0 };

    LoadStringW(GetModuleHandleW(NULL), StringID, LoadedString, _countof(LoadedString));
    SetDlgItemTextW(hwnd, ResourceID, LoadedString);
}

void SetTotalSize(long long Size, UINT ResourceID, HWND hwnd)
{
    WCHAR LoadedString[ARR_MAX_SIZE] = { 0 };

    StrFormatByteSizeW(Size, LoadedString, ARR_MAX_SIZE);
    SetDlgItemTextW(hwnd, ResourceID, LoadedString);
}
