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

DLG_VAR dv;
WCHAR_VAR wcv;
DIRSIZE sz;
BOOL_VAR bv;

#define ETDT_DISABLE        0x00000001
#define ETDT_ENABLE         0x00000002
#define ETDT_USETABTEXTURE  0x00000004
#define ETDT_ENABLETAB      (ETDT_ENABLE  | ETDT_USETABTEXTURE)

typedef HRESULT (WINAPI * ETDTProc) (HWND, DWORD);

DWORD WINAPI SizeCheck(LPVOID lpParam)
{
	HWND HProgressBar = NULL;
	WCHAR TargetedDir[MAX_PATH] = { 0 };
	
	HProgressBar = GetDlgItem(dv.hwndDlg, IDC_PROGRESS_1);
	
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
			StringCbCopyW(wcv.TempDir, _countof(wcv.TempDir), TargetedDir);
			ZeroMemory(&TargetedDir, sizeof(TargetedDir));
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
			SetDlgItemTextW(dv.hwndDlg, IDC_STATIC_INFO, L"Temporary Files");
			StringCbCopyW(wcv.AltTempDir, _countof(wcv.AltTempDir), TargetedDir);
			ZeroMemory(&TargetedDir, sizeof(TargetedDir));
		}

		SendMessageW(HProgressBar, PBM_SETPOS, 25, 0);
		Sleep(150);
	}

	if(FindRecycleBin(wcv.DriveLetter))
	{
		StringCchPrintfW(TargetedDir, _countof(TargetedDir), L"%s\\%s", wcv.DriveLetter, wcv.RecycleBin);
		sz.RecycleBinSize = DirSizeFunc(TargetedDir);
		if (sz.RecycleBinSize == 1)
		{
			return FALSE;
		}
		StringCbCopyW(wcv.RecycleBinDir, _countof(wcv.RecycleBinDir), TargetedDir);
		SetDlgItemTextW(dv.hwndDlg, IDC_STATIC_INFO, L"Recycled Files");
		ZeroMemory(&TargetedDir, sizeof(TargetedDir));
		SendMessageW(HProgressBar, PBM_SETPOS, 50, 0);
		Sleep(150);
	}

	
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
			SetDlgItemTextW(dv.hwndDlg, IDC_STATIC_INFO, L"Old ChkDsk Files");
			ZeroMemory(&TargetedDir, sizeof(TargetedDir));
			SendMessageW(HProgressBar, PBM_SETPOS, 75, 0);
		}
		
		else
			break;
	}
	
	/* Default path of RAPPS to store downloaded files */
	
	SHGetFolderPathW(NULL, CSIDL_PROFILE, NULL, SHGFP_TYPE_CURRENT, TargetedDir);
	StringCbCatW(TargetedDir, _countof(TargetedDir), L"\\My Documents\\RAPPS Downloads");

	OutputDebugStringW(TargetedDir);

	if (PathIsDirectoryW(TargetedDir))
	{
		sz.RappsSize = DirSizeFunc(TargetedDir);
		if (sz.RappsSize == 1)
		{
			return FALSE;
		}
		StringCbCopyW(wcv.RappsDir, _countof(wcv.RappsDir), TargetedDir);
		SetDlgItemTextW(dv.hwndDlg, IDC_STATIC_INFO, L"Temporary RAPPS Files");
		ZeroMemory(&TargetedDir, sizeof(TargetedDir));
		SendMessageW(HProgressBar, PBM_SETPOS, 100, 0);
	}
	
	Sleep(2500);
	
	SendMessageW(dv.hwndDlg, WM_DESTROY, 0, 0);
	return TRUE;
}

DWORD WINAPI FolderRemoval(LPVOID lpParam)
{
	HWND HProgressBar = NULL;
	WCHAR TargetedDir[MAX_PATH] = { 0 };

	SHFILEOPSTRUCTW FileStruct;
	ZeroMemory(&FileStruct, sizeof(FileStruct));

	FileStruct.wFunc = FO_DELETE;
	FileStruct.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_NOCONFIRMMKDIR;
	FileStruct.fAnyOperationsAborted = FALSE;

	HProgressBar = GetDlgItem(dv.hwndDlg, IDC_PROGRESS_2);

	if (bv.TempClean == TRUE && bv.SysDrive == TRUE)
	{
		SendMessageW(HProgressBar, PBM_SETPOS, 25, 0);
		FileStruct.pFrom = NULL;
		FileStruct.pFrom = wcv.TempDir;

		SetDlgItemTextW(dv.hwndDlg, IDC_STATIC_INFO, L"Temporary files");

		SHFileOperationW(&FileStruct);

		FileStruct.pFrom = NULL;
		FileStruct.pFrom = wcv.AltTempDir;

		SHFileOperationW(&FileStruct);
		Sleep(150);
	}
	
	if (bv.RecycleClean == TRUE)
	{
		SendMessageW(HProgressBar, PBM_SETPOS, 50, 0);
		FileStruct.pFrom = NULL;
		FileStruct.pFrom = wcv.RecycleBinDir;

		SetDlgItemTextW(dv.hwndDlg, IDC_STATIC_INFO, L"Recycled files");

		SHFileOperationW(&FileStruct);
		Sleep(150);
	}
	
	if (bv.ChkDskClean == TRUE)
	{
		for(int i = 0; i < 10; i++)
		{
			StringCchPrintfW(TargetedDir, _countof(TargetedDir), L"%s\\FOUND.%.3i", wcv.DriveLetter, i);
			if (PathIsDirectoryW(TargetedDir))
			{
				SendMessageW(HProgressBar, PBM_SETPOS, 75, 0);
				FileStruct.pFrom = NULL;
				FileStruct.pFrom = TargetedDir;

				SetDlgItemTextW(dv.hwndDlg, IDC_STATIC_INFO, L"Old ChkDsk files");

				SHFileOperationW(&FileStruct);
				Sleep(150);
			}
		
			else
				break;
		}
	}

	if (bv.RappsClean == TRUE)
	{
		SendMessageW(HProgressBar, PBM_SETPOS, 100, 0);
		FileStruct.pFrom = NULL;
		FileStruct.pFrom = wcv.RappsDir;

		SetDlgItemTextW(dv.hwndDlg, IDC_STATIC_INFO, L"Temporary RAPPS Files");

		SHFileOperationW(&FileStruct);
	}
	
	SendMessageW(dv.hwndDlg, WM_DESTROY, 0, 0);

	return TRUE;
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
	DWORD DwError = 0;
	WCHAR Err[256] = { 0 };
	
	StringCchPrintfW(TargetedDir, _countof(TargetedDir), L"%s%s", TargetDir, L"\\*");
	HandleDir = FindFirstFileW(TargetedDir, &DataStruct);
	DwError = GetLastError();
	
	if (DwError != ERROR_NO_MORE_FILES && HandleDir == INVALID_HANDLE_VALUE)
	{
		if (DwError == ERROR_PATH_NOT_FOUND)
		{
			return 0;
		}

		else
		{
			FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), Err, 255, NULL);
			MessageBoxW(NULL, Err, L"Error", MB_OK);
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

BOOL FindRecycleBin(PWCHAR TargetDrive)
{
	WIN32_FIND_DATAW DataStruct;
	ZeroMemory(&DataStruct, sizeof(DataStruct));
	HANDLE HandleDir = NULL;
	WCHAR TargetedDrive[1024] = { 0 };
	SHDESCRIPTIONID did;
	ZeroMemory(&did, sizeof(did));
	HRESULT hr;

	StringCchPrintfW(TargetedDrive, _countof(TargetedDrive), L"%s%s", TargetDrive, L"\\*");
	HandleDir = FindFirstFileW(TargetedDrive, &DataStruct);

	do
	{
		if (FILE_ATTRIBUTE_DIRECTORY == (DataStruct.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			if (wcscmp(DataStruct.cFileName, L".") != 0 && wcscmp(DataStruct.cFileName, L"..") != 0)
			{
				WCHAR childSearch[512] = { 0 };
				StringCchPrintfW(childSearch, _countof(childSearch), L"%s\\%s\\*", TargetDrive, DataStruct.cFileName);
				printf("%S\n", childSearch);

				WIN32_FIND_DATAW childData;

				HANDLE childHandle = FindFirstFileW(childSearch, &childData);

				do
				{
					if (FILE_ATTRIBUTE_DIRECTORY == (childData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
					{

						if (wcscmp(childData.cFileName, L".") != 0 && wcscmp(childData.cFileName, L"..") != 0)
						{
							WCHAR fullPath[MAX_PATH] = { 0 };

							StringCchPrintfW(fullPath, _countof(fullPath), L"%s\\%s\\%s", TargetDrive, DataStruct.cFileName, childData.cFileName);
							OutputDebugStringW(fullPath);
							hr = GetFolderCLSID(fullPath, &did);

							if (SUCCEEDED(hr) && IsEqualCLSID(&CLSID_RecycleBin, &did.clsid))
							{
								StringCchCopyW(wcv.RecycleBin, _countof(wcv.RecycleBin), DataStruct.cFileName);
								printf("%S\n", wcv.RecycleBin);

								FindClose(HandleDir);
								FindClose(childHandle);

								return TRUE;
							}
						}
					}
				}

				while (FindNextFileW(childHandle, &childData));

				FindClose(childHandle);
			}
		}
	}

	while (FindNextFileW(HandleDir, &DataStruct));

	FindClose(HandleDir);

	if (wcslen(wcv.RecycleBin) == 0)
	{
		printf("findRecycleBin couldn't find the folder, ignoring....\n");
		return FALSE;
	}

	return TRUE;
}

/* FIX ME: This function doesn't work in ReactOS because SHGDFIL_DESCRIPTIONID isn't implemented yet. */

HRESULT GetFolderCLSID(LPCWSTR pszPath, SHDESCRIPTIONID* pdid)
{
	HRESULT hr;

	LPITEMIDLIST pidl;

	WCHAR logError[MAX_PATH] = { 0 };

	hr = SHParseDisplayName(pszPath, NULL, &pidl, 0, NULL);
	StringCchPrintfW(logError, _countof(logError), L"\nSHParseDisplayName HANDLE hr returned 0x%08x\n", hr);
	OutputDebugStringW(logError);
	if (SUCCEEDED(hr))
	{
		IShellFolder* psf;

		LPCITEMIDLIST pidlChild;

		hr = SHBindToParent(pidl, &IID_IShellFolder, (void**)&psf, &pidlChild);
		if (SUCCEEDED(hr))
		{
			hr = SHGetDataFromIDList(psf, pidlChild, SHGDFIL_DESCRIPTIONID, pdid, sizeof(*pdid));
			StringCchPrintfW(logError, _countof(logError), L"\nSHGetDataFromIDList HANDLE hr returned 0x%08x\n", hr);
			OutputDebugStringW(logError);
			IShellFolder_Release(psf);
		}

		CoTaskMemFree(pidl);
	}

	return hr;
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
	HICON HbmIcon;
	HIMAGELIST HSmall;
	HSmall = ImageList_Create(16, 16, ILC_COLOR16 | ILC_MASK, 3, 3);

	if(HSmall == NULL)
		return FALSE;
	
	for (int index = 0; index < 3; index++)
	{
		HbmIcon = LoadIconW(dv.hInst, MAKEINTRESOURCE(IDI_BLANK + index));
		ImageList_AddIcon(HSmall, HbmIcon);
		DestroyIcon(HbmIcon);
	}

	ListView_SetImageList(hList, HSmall, LVSIL_SMALL);
	return TRUE;
}

PWCHAR SetOptimalUnit(uint64_t Size)
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
	WCHAR loadedString[MAX_PATH] = { 0 };

	LoadStringW(GetModuleHandleW(NULL), StringID, loadedString, _countof(loadedString));
	SetDlgItemTextW(hwnd, ResourceID, loadedString);
	memset(loadedString, 0, sizeof loadedString);
}

void SetTotalSize(long long Size, UINT ResourceID, HWND hwnd)
{
	WCHAR loadedString[MAX_PATH] = { 0 };

	StringCchPrintfW(loadedString, _countof(loadedString), L"%.02lf %s", SetOptimalSize(Size), SetOptimalUnit(Size));
	SetDlgItemTextW(hwnd, ResourceID, loadedString);
	memset(loadedString, 0, sizeof loadedString);
}

BOOL OnCreate(HWND hwnd)
{
    TCITEMW item;

    dv.hTab = GetDlgItem(hwnd, IDC_TAB);
    dv.hChoicePage = CreateDialogW(dv.hInst, MAKEINTRESOURCE(IDD_CHOICE_PAGE), hwnd, ChoicePageDlgProc);
	EnableDialogTheme(dv.hChoicePage);
	dv.hOptionsPage = CreateDialogW(dv.hInst, MAKEINTRESOURCE(IDD_OPTIONS_PAGE), hwnd, OptionsPageDlgProc);
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
    dv.hSagesetPage = CreateDialogW(dv.hInst, MAKEINTRESOURCE(IDD_SAGESET_PAGE), hwnd, SagesetPageDlgProc);
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
		SetDetails(IDS_DETAILS_DOWNLOAD, IDC_STATIC_DETAILS, hwnd);
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
	LRESULT Ret;
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

			Ret = CDRF_SKIPDEFAULT;
			break;
		}

		case CDDS_PREERASE:
		{
			Ret = CDRF_DODEFAULT;
			break;
		}

		default:
			Ret = CDRF_SKIPDEFAULT;
			break;
		}
	}
	else
	{
		Ret = CDRF_DODEFAULT;
	}

	return Ret;
}

BOOL EnableDialogTheme(HWND hwnd)
{
    HMODULE hUXTheme;
    ETDTProc FnnEnableThemeDialogTexture;

    hUXTheme = LoadLibraryW(L"uxtheme.dll");

    if(hUXTheme)
    {
        FnnEnableThemeDialogTexture = 
            (ETDTProc)GetProcAddress(hUXTheme, "EnableThemeDialogTexture");

        if(FnnEnableThemeDialogTexture)
        {
            FnnEnableThemeDialogTexture(hwnd, ETDT_ENABLETAB);

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
	DWORD cbSize = sizeof(DWORD);
	DWORD RegValue = 0;
	
	if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
						SubKey,
						0,
						KEY_SET_VALUE,
						&hRegKey) != ERROR_SUCCESS)
	{
		MessageBoxW(NULL, L"RegValSet() failed!", L"Warning", MB_OK | MB_ICONWARNING);
	}
	
	if(ArgBool == TRUE)
	{
		RegValue = 2;
	}
	
	if (RegSetValueExW(hRegKey,
						RegArg,
						0,
						REG_DWORD,
						(BYTE*)&RegValue,
						cbSize) != ERROR_SUCCESS)
	{
		MessageBoxW(NULL, L"RegValSet() failed!", L"Warning", MB_OK | MB_ICONERROR);
		return FALSE;
	}
	
	return TRUE;
}

DWORD RegQuery(PWCHAR RegArg, PWCHAR SubKey)
{
	DWORD RetValue = 0;
	DWORD cbSize = sizeof(DWORD);
	if (RegGetValueW(HKEY_LOCAL_MACHINE,
					SubKey,
					RegArg,
					RRF_RT_DWORD,
					NULL,
					(PVOID)&RetValue,
					&cbSize) != ERROR_SUCCESS)
	{
		MessageBoxW(NULL, L"RegQuery() failed!", L"Warning", MB_OK | MB_ICONERROR);
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
			if (GetDriveTypeW(SingleDrive) == 3)
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
		
	DialogButtonSelect = DialogBoxParamW(dv.hInst, MAKEINTRESOURCEW(IDD_SAGERUN), NULL, SagesetDlgProc, 0);
		
	if(DialogButtonSelect == IDCANCEL)
	{
		return;
	}

	if(!RegValSet(ValStr, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\VolumeCaches\\Old ChkDsk Files", bv.ChkDskClean))
		return;
	
	if(!RegValSet(ValStr, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\VolumeCaches\\Temporary Files", bv.TempClean))
		return;
	
	if(!RegValSet(ValStr, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\VolumeCaches\\RAPPS Files", bv.RappsClean))
		return;
	
	if(!RegValSet(ValStr, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\VolumeCaches\\Recycle Bin", bv.RecycleClean))
	{
		return;
	}
}

void SagerunProc(int nArgs, PWCHAR ArgReal, LPWSTR* ArgList, PWCHAR LogicalDrives)
{
	WCHAR *ValStr = NULL;
	WCHAR* SingleDrive = LogicalDrives;
	INT_PTR DialogButtonSelect;
		
	ValStr = RealStageFlag(nArgs, ArgReal, ArgList);
		
	if(RegQuery(ValStr, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\VolumeCaches\\Old ChkDsk Files") == 2)
		bv.ChkDskClean = TRUE;
		
	if(RegQuery(ValStr, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\VolumeCaches\\Temporary Files") == 2)
		bv.TempClean = TRUE;

	if(RegQuery(ValStr, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\VolumeCaches\\RAPPS Files") == 2)
		bv.RappsClean = TRUE;
		
	if(RegQuery(ValStr, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\VolumeCaches\\Recycle Bin") == 2)
		bv.RecycleClean = TRUE;
		
	while (*SingleDrive)
	{
		if (GetDriveTypeW(SingleDrive) == 3)
		{
			StringCchCopyW(wcv.DriveLetter, MAX_PATH, SingleDrive);
			wcv.DriveLetter[wcslen(wcv.DriveLetter) - 1] = '\0';

			DialogButtonSelect = DialogBoxParamW(dv.hInst, MAKEINTRESOURCEW(IDD_PROGRESS), NULL, ProgressDlgProc, 0);

			if(DialogButtonSelect == IDCANCEL)
			{
				return;
			}
		
			DialogBoxParamW(dv.hInst, MAKEINTRESOURCEW(IDD_PROGRESS_END), NULL, ProgressEndDlgProc, 0);
				
			ZeroMemory(&wcv.DriveLetter, sizeof(wcv.DriveLetter));
		}
		SingleDrive += wcslen(SingleDrive) + 1;
	}
}
