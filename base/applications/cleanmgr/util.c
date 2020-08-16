/*
 * PROJECT:         ReactOS Disk Cleanup
 * LICENSE:         GPL - See COPYING in the top level directory
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

DWORD WINAPI sizeCheck(LPVOID lpParam)
{
	HWND hProgressBar = NULL;
	WCHAR targetedDir[MAX_PATH] = { 0 };
	
	hProgressBar = GetDlgItem(dv.hwndDlg, IDC_PROGRESS_1);
	
	if(bv.sysDrive == TRUE)
	{
		SHGetFolderPathW(NULL, CSIDL_WINDOWS, NULL, SHGFP_TYPE_CURRENT, targetedDir);
		StringCbCatW(targetedDir, _countof(targetedDir), L"\\Temp");
		if (PathIsDirectoryW(targetedDir))
		{
			sz.tempASize = dirSizeFunc(targetedDir);
			if (sz.tempASize == 1)
			{
				return FALSE;
			}
			StringCbCopyW(wcv.tempDir, _countof(wcv.tempDir), targetedDir);
			ZeroMemory(&targetedDir, sizeof(targetedDir));
		}
		SHGetFolderPathW(NULL, CSIDL_PROFILE, NULL, SHGFP_TYPE_CURRENT, targetedDir);
		StringCbCatW(targetedDir, _countof(targetedDir), L"\\Local Settings\\Temp");
		if (PathIsDirectoryW(targetedDir))
		{
			sz.tempBSize = dirSizeFunc(targetedDir);
			if (sz.tempBSize == 1)
			{
				return FALSE;
			}
			SetDlgItemTextW(dv.hwndDlg, IDC_STATIC_INFO, L"Temporary Files");
			StringCbCopyW(wcv.alttempDir, _countof(wcv.alttempDir), targetedDir);
			ZeroMemory(&targetedDir, sizeof(targetedDir));
		}

		SendMessageW(hProgressBar, PBM_SETPOS, 25, 0);
		Sleep(150);
	}

	if(findRecycleBin(wcv.driveLetter))
	{
		StringCchPrintfW(targetedDir, _countof(targetedDir), L"%s\\%s", wcv.driveLetter, wcv.recycleBin);
		sz.recyclebinSize = dirSizeFunc(targetedDir);
		if (sz.recyclebinSize == 1)
		{
			return FALSE;
		}
		StringCbCopyW(wcv.recyclebinDir, _countof(wcv.recyclebinDir), targetedDir);
		SetDlgItemTextW(dv.hwndDlg, IDC_STATIC_INFO, L"Recycled Files");
		ZeroMemory(&targetedDir, sizeof(targetedDir));
		SendMessageW(hProgressBar, PBM_SETPOS, 50, 0);
		Sleep(150);
	}

	SHGetFolderPathW(NULL, CSIDL_PROFILE, NULL, SHGFP_TYPE_CURRENT, targetedDir);
	StringCbCatW(targetedDir, _countof(targetedDir), L"\\My Documents\\Downloads");
	if (PathIsDirectoryW(targetedDir))
	{
		sz.downloadedSize = dirSizeFunc(targetedDir);
		if (sz.downloadedSize == 1)
		{
			return FALSE;
		}
		StringCbCopyW(wcv.downloadDir, _countof(wcv.downloadDir), targetedDir);
		SetDlgItemTextW(dv.hwndDlg, IDC_STATIC_INFO, L"Downloaded Files");
		ZeroMemory(&targetedDir, sizeof(targetedDir));
		SendMessageW(hProgressBar, PBM_SETPOS, 75, 0);
		Sleep(150);
	}

	SHGetFolderPathW(NULL, CSIDL_PROFILE, NULL, SHGFP_TYPE_CURRENT, targetedDir);
	StringCbCatW(targetedDir, _countof(targetedDir), L"\\My Documents\\RAPPS Downloads");
	if (PathIsDirectoryW(targetedDir))
	{
		sz.rappsSize = dirSizeFunc(targetedDir);
		if (sz.rappsSize == 1)
		{
			return FALSE;
		}
		StringCbCopyW(wcv.rappsDir, _countof(wcv.rappsDir), targetedDir);
		SetDlgItemTextW(dv.hwndDlg, IDC_STATIC_INFO, L"Downloaded RAPPS Files");
		ZeroMemory(&targetedDir, sizeof(targetedDir));
		SendMessageW(hProgressBar, PBM_SETPOS, 100, 0);
	}
	
	Sleep(2500);
	
	SendMessageW(dv.hwndDlg, WM_DESTROY, 0, 0);
	return TRUE;
}

DWORD WINAPI folderRemoval(LPVOID lpParam)
{
	HWND hProgressBar = NULL;

	SHFILEOPSTRUCTW fileStruct;
	ZeroMemory(&fileStruct, sizeof(fileStruct));

	fileStruct.wFunc = FO_DELETE;
	fileStruct.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_NOCONFIRMMKDIR;
	fileStruct.fAnyOperationsAborted = FALSE;

	hProgressBar = GetDlgItem(dv.hwndDlg, IDC_PROGRESS_2);

	if (bv.tempClean == TRUE && bv.sysDrive == TRUE)
	{
		SendMessageW(hProgressBar, PBM_SETPOS, 25, 0);
		fileStruct.pFrom = NULL;
		fileStruct.pFrom = wcv.tempDir;

		SetDlgItemTextW(dv.hwndDlg, IDC_STATIC_INFO, L"Temporary files");

		SHFileOperationW(&fileStruct);

		fileStruct.pFrom = NULL;
		fileStruct.pFrom = wcv.alttempDir;

		SHFileOperationW(&fileStruct);
		Sleep(150);
	}
	
	if (bv.recycleClean == TRUE)
	{
		SendMessageW(hProgressBar, PBM_SETPOS, 50, 0);
		fileStruct.pFrom = NULL;
		fileStruct.pFrom = wcv.recyclebinDir;

		SetDlgItemTextW(dv.hwndDlg, IDC_STATIC_INFO, L"Recycled files");

		SHFileOperationW(&fileStruct);
		Sleep(150);
	}

	if (bv.downloadClean == TRUE)
	{
		SendMessageW(hProgressBar, PBM_SETPOS, 75, 0);
		fileStruct.pFrom = NULL;
		fileStruct.pFrom = wcv.downloadDir;

		SetDlgItemTextW(dv.hwndDlg, IDC_STATIC_INFO, L"Downloaded files");

		SHFileOperationW(&fileStruct);
		Sleep(150);
	}

	if (bv.rappsClean == TRUE)
	{
		SendMessageW(hProgressBar, PBM_SETPOS, 100, 0);
		fileStruct.pFrom = NULL;
		fileStruct.pFrom = wcv.rappsDir;

		SetDlgItemTextW(dv.hwndDlg, IDC_STATIC_INFO, L"Downloaded RAPPS Files");

		SHFileOperationW(&fileStruct);
	}
	
	SendMessageW(dv.hwndDlg, WM_DESTROY, 0, 0);

	return TRUE;
}

uint64_t dirSizeFunc(PWCHAR targetDir)
{
	WIN32_FIND_DATAW data;
	ZeroMemory(&data, sizeof(data));
	LARGE_INTEGER sz;
	ZeroMemory(&sz, sizeof(sz));
	HANDLE handleDir = NULL;
	uint64_t size = 0;
	WCHAR targetedDir[2048] = { 0 };
	WCHAR retargetDir[2048] = { 0 };
	DWORD dwError = 0;
	wchar_t err[256] = { 0 };
	
	StringCchPrintfW(targetedDir, _countof(targetedDir), L"%s%s", targetDir, L"\\*");
	handleDir = FindFirstFileW(targetedDir, &data);
	dwError = GetLastError();
	
	if (dwError != ERROR_NO_MORE_FILES && handleDir == INVALID_HANDLE_VALUE)
	{
		if (dwError == ERROR_PATH_NOT_FOUND)
		{
			return 0;
		}

		else
		{
			FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), err, 255, NULL);
			MessageBoxW(NULL, err, (LPCWSTR)L"Ok", MB_OK);
			return 1;
		}
	}

	do
	{
		if (wcscmp(data.cFileName, L".") != 0 && wcscmp(data.cFileName, L"..") != 0)
		{
			if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				StringCchPrintfW(retargetDir, _countof(retargetDir), L"%s\\%s", targetDir, data.cFileName);
				size = dirSizeFunc(retargetDir);
			}

			else
			{
				sz.LowPart = data.nFileSizeLow;
				sz.HighPart = data.nFileSizeHigh;
				size += sz.QuadPart;
			}
		}
	}

	while (FindNextFileW(handleDir, &data));

	FindClose(handleDir);

	return size;
}

BOOL findRecycleBin(PWCHAR drive)
{
	WIN32_FIND_DATAW data;
	ZeroMemory(&data, sizeof(data));
	HANDLE handleDir = NULL;
	WCHAR targetedDrive[1024] = { 0 };
	SHDESCRIPTIONID did;
	ZeroMemory(&did, sizeof(did));
	HRESULT hr;

	StringCchPrintfW(targetedDrive, _countof(targetedDrive), L"%s%s", drive, L"\\*");
	handleDir = FindFirstFileW(targetedDrive, &data);

	do
	{
		if (FILE_ATTRIBUTE_DIRECTORY == (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			if (wcscmp(data.cFileName, L".") != 0 && wcscmp(data.cFileName, L"..") != 0)
			{
				WCHAR childSearch[512] = { 0 };
				StringCchPrintfW(childSearch, _countof(childSearch), L"%s\\%s\\*", drive, data.cFileName);
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

							StringCchPrintfW(fullPath, _countof(fullPath), L"%s\\%s\\%s", drive, data.cFileName, childData.cFileName);
							OutputDebugStringW(fullPath);
							hr = GetFolderCLSID(fullPath, &did);

							if (SUCCEEDED(hr) && IsEqualCLSID(&CLSID_RecycleBin, &did.clsid))
							{
								StringCchCopyW(wcv.recycleBin, _countof(wcv.recycleBin), data.cFileName);
								printf("%S\n", wcv.recycleBin);

								FindClose(handleDir);
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

	while (FindNextFileW(handleDir, &data));

	FindClose(handleDir);

	if (wcslen(wcv.recycleBin) == 0)
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


double setOptimalSize(uint64_t size)
{
	if ((double)size >= (GB))
	{
		return (double)size / (GB);
	}

	else if ((double)size >= (MB))
	{
		return (double)size / (MB);
	}

	else if ((double)size >= (KB))
	{
		return (double)size / (KB);
	}

	return (double)size;
}

void addItem(HWND hList, PWCHAR string, PWCHAR subString, int iIndex)
{
	LVITEMW lvI;
	ZeroMemory(&lvI, sizeof(lvI));
	
	lvI.mask = LVIF_TEXT | LVIF_IMAGE;
	lvI.cchTextMax = 256;
	lvI.iItem = SendMessage(hList, LVM_GETITEMCOUNT, 0, 0);
	lvI.iSubItem = 0;
	lvI.iImage = iIndex;
	lvI.pszText = string;
	ListView_InsertItem(hList, &lvI);
	lvI.iSubItem = 1;
	lvI.pszText = subString;	
	ListView_SetItem(hList, &lvI);
}

void createImageLists(HWND hList)
{
	HICON hbmIcon;
	HIMAGELIST hSmall;
	hSmall = ImageList_Create(16, 16, ILC_COLOR16 | ILC_MASK, 3, 3);

	for (int index = 0; index < 3; index++)
	{
		hbmIcon = LoadIconW(dv.hInst, MAKEINTRESOURCE(IDI_BLANK + index));
		ImageList_AddIcon(hSmall, hbmIcon);
		DestroyIcon(hbmIcon);
	}

	ListView_SetImageList(hList, hSmall, LVSIL_SMALL);
}

PWCHAR setOptimalUnit(uint64_t size)
{
	if ((double)size >= (GB))
	{
		return L"GB";
	}
	
	else if ((double)size >= (MB))
	{
		return L"MB";
	}
	
	else if ((double)size >= (KB))
	{
		return L"KB";
	}
	
	return L"Bytes";
}

void setDetails(UINT stringID, UINT resourceID, HWND hwnd)
{
	WCHAR loadedString[MAX_PATH] = { 0 };

	LoadStringW(GetModuleHandleW(NULL), stringID, loadedString, _countof(loadedString));
	SetDlgItemTextW(hwnd, resourceID, loadedString);
	memset(loadedString, 0, sizeof loadedString);
}

void setTotalSize(long long size, UINT resourceID, HWND hwnd)
{
	WCHAR loadedString[MAX_PATH] = { 0 };

	StringCchPrintfW(loadedString, _countof(loadedString), L"%.02lf %s", setOptimalSize(size), setOptimalUnit(size));
	SetDlgItemTextW(hwnd, resourceID, loadedString);
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
	
	if(dv.hOptionsPage == NULL)
	{
		WCHAR tempArr[MAX_PATH] = { 0 };
		StringCchPrintfW(tempArr, _countof(tempArr), L"\nGetLastError %s", GetLastError());
		OutputDebugStringW(tempArr);
		return FALSE;
	}

    memset(&item, 0, sizeof(item));
    item.mask = TCIF_TEXT;
    item.pszText = L"Disk Cleanup";
    (void)TabCtrl_InsertItem(dv.hTab, 0, &item);

	item.pszText = L"More Options";
	(void)TabCtrl_InsertItem(dv.hTab, 1, &item);

    MsConfig_OnTabWndSelChange();

    return TRUE;
}

void MsConfig_OnTabWndSelChange(void)
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

void selItem(HWND hwnd, int index)
{
	DIRECTORIES Dir = index;
	switch (Dir)
	{
	case TEMPORARY_FILE:
		ShowWindow(GetDlgItem(hwnd, IDC_VIEW_FILES), SW_HIDE);
		setDetails(IDS_DETAILS_TEMP, IDC_STATIC_DETAILS, hwnd);
		break;

	case RECYCLE_BIN:
		ShowWindow(GetDlgItem(hwnd, IDC_VIEW_FILES), SW_HIDE);
		setDetails(IDS_DETAILS_RECYCLE, IDC_STATIC_DETAILS, hwnd);
		break;

	case DOWNLOADED_FILES:
		ShowWindow(GetDlgItem(hwnd, IDC_VIEW_FILES), SW_SHOW);
		OutputDebugStringW(wcv.downloadDir);
		setDetails(IDS_DETAILS_DOWNLOAD, IDC_STATIC_DETAILS, hwnd);
		break;

	case RAPPS_FILES:
		ShowWindow(GetDlgItem(hwnd, IDC_VIEW_FILES), SW_HIDE);
		setDetails(IDS_DETAILS_RAPPS, IDC_STATIC_DETAILS, hwnd);
		break;

	default:
		break;
	}
}

long long checkedItem(int index, HWND hwnd, HWND hList, long long size)
{
	uint64_t tempSize = sz.tempASize + sz.tempBSize;
	uint64_t recycleSize = sz.recyclebinSize;
	uint64_t downloadSize = sz.downloadedSize;
	uint64_t rappsSize = sz.rappsSize;
	DIRECTORIES Dir = index;
	switch (Dir)
	{
	case TEMPORARY_FILE:
		if (ListView_GetCheckState(hList, index))
		{
			bv.tempClean = TRUE;
			size += tempSize;
			setTotalSize(size, IDC_STATIC_SIZE, hwnd);
		}

		else
		{
			bv.tempClean = FALSE;
			size -= tempSize;
			if (0 >= size)
			{
				SetDlgItemTextW(hwnd, IDC_STATIC_SIZE, L"0.00 Bytes");
				return 0;
			}
			setTotalSize(size, IDC_STATIC_SIZE, hwnd);
		}
		return size;

	case RECYCLE_BIN:
		if (ListView_GetCheckState(hList, index))
		{
			bv.recycleClean = TRUE;
			size += recycleSize;
			setTotalSize(size, IDC_STATIC_SIZE, hwnd);
		}

		else
		{
			bv.recycleClean = FALSE;
			size -= recycleSize;
			if (0 >= size)
			{
				SetDlgItemTextW(hwnd, IDC_STATIC_SIZE, L"0.00 Bytes");
				return 0;
			}
			setTotalSize(size, IDC_STATIC_SIZE, hwnd);
		}
		return size;

	case DOWNLOADED_FILES:
		if (ListView_GetCheckState(hList, index))
		{
			bv.downloadClean = TRUE;
			size += downloadSize;
			setTotalSize(size, IDC_STATIC_SIZE, hwnd);
		}

		else
		{
			bv.downloadClean = FALSE;
			size -= downloadSize;
			if (0 >= size)
			{
				SetDlgItemTextW(hwnd, IDC_STATIC_SIZE, L"0.00 Bytes");
				return 0;
			}
			setTotalSize(size, IDC_STATIC_SIZE, hwnd);
		}
		return size;

	case RAPPS_FILES:
		if (ListView_GetCheckState(hList, index))
		{
			bv.rappsClean = TRUE;
			size += rappsSize;
			setTotalSize(size, IDC_STATIC_SIZE, hwnd);
		}

		else
		{
			bv.rappsClean = FALSE;
			size -= rappsSize;
			if (0 >= size)
			{
				SetDlgItemTextW(hwnd, IDC_STATIC_SIZE, L"0.00 Bytes");
				return 0;
			}
			setTotalSize(size, IDC_STATIC_SIZE, hwnd);
		}
		return size;

	default:
		return 0;
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
    ETDTProc fnEnableThemeDialogTexture;

    hUXTheme = LoadLibraryW(L"uxtheme.dll");

    if(hUXTheme)
    {
        fnEnableThemeDialogTexture = 
            (ETDTProc)GetProcAddress(hUXTheme, "EnableThemeDialogTexture");

        if(fnEnableThemeDialogTexture)
        {
            fnEnableThemeDialogTexture(hwnd, ETDT_ENABLETAB);

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

BOOL argCheck(LPWSTR* argList, int nArgs)
{
	WCHAR LogicalDrives[MAX_PATH] = { 0 };
	WCHAR driveReal[MAX_PATH] = { 0 };
	WCHAR argReal[MAX_PATH] = { 0 };
	DWORD drives = GetLogicalDriveStringsW(MAX_PATH, LogicalDrives);

	StringCbCopyW(argReal, _countof(argReal), argList[1]);
	_wcsupr(argReal);
	
	if (wcscmp(argReal, L"/D") == 0 && nArgs == 3)
	{
		if (drives == 0)
		{
			MessageBoxW(NULL, L"GetLogicalDriveStringsW() failed!", L"Ok", MB_OK);
			return FALSE;
		}

		StringCbCopyW(driveReal, _countof(driveReal), argList[2]);

		_wcsupr(driveReal);

		if (wcslen(driveReal) == 1)
		{
			StringCbCatW(driveReal, _countof(driveReal), L":");
		}

		if (drives <= MAX_PATH)
		{
			WCHAR* SingleDrive = LogicalDrives;
			WCHAR RealDrive[MAX_PATH] = { 0 };
			while (*SingleDrive)
			{
				if (GetDriveTypeW(SingleDrive) == 3)
				{
					StringCchCopyW(RealDrive, MAX_PATH, SingleDrive);
					RealDrive[wcslen(RealDrive) - 1] = '\0';
					if (wcscmp(driveReal, RealDrive) == 0)
					{
						StringCbCopyW(wcv.driveLetter, _countof(wcv.driveLetter), driveReal);
						break;
					}
					memset(RealDrive, 0, sizeof RealDrive);
				}
				SingleDrive += wcslen(SingleDrive) + 1;
			}
		}
		if (wcslen(wcv.driveLetter) == 0)
		{
			MessageBoxW(NULL, L"Invalid drive! Please try again!", L"Warning", MB_OK | MB_ICONWARNING);
			return FALSE;
		}

	}

	else if (wcscmp(argReal, L"/?") == 0)
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
