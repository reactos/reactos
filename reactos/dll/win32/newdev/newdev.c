/*
 * New device installer (newdev.dll)
 *
 * Copyright 2005-2006 Hervé Poussineau (hpoussin@reactos.org)
 *           2005 Christoph von Wittich (Christoph@ActiveVB.de)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define YDEBUG
#include "newdev_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(newdev);

/* Global variables */
HINSTANCE hDllInstance;

static BOOL
SearchDriver(
	IN PDEVINSTDATA DevInstData,
	IN LPCTSTR Directory OPTIONAL,
	IN LPCTSTR InfFile OPTIONAL);

/*
* @implemented
*/
BOOL WINAPI
UpdateDriverForPlugAndPlayDevicesW(
	IN HWND hwndParent,
	IN LPCWSTR HardwareId,
	IN LPCWSTR FullInfPath,
	IN DWORD InstallFlags,
	OUT PBOOL bRebootRequired OPTIONAL)
{
	DEVINSTDATA DevInstData;
	DWORD i;
	LPWSTR Buffer = NULL;
	DWORD BufferSize;
	LPCWSTR CurrentHardwareId; /* Pointer into Buffer */
	BOOL FoundHardwareId, FoundAtLeastOneDevice = FALSE;
	BOOL ret = FALSE;

	DevInstData.hDevInfo = INVALID_HANDLE_VALUE;

	TRACE("UpdateDriverForPlugAndPlayDevicesW(%p %S %S 0x%lx %p)\n",
		hwndParent, HardwareId, FullInfPath, InstallFlags, bRebootRequired);

	/* FIXME: InstallFlags bRebootRequired ignored! */

	/* Check flags */
	if (InstallFlags & ~(INSTALLFLAG_FORCE | INSTALLFLAG_READONLY | INSTALLFLAG_NONINTERACTIVE))
	{
		DPRINT("Unknown flags: 0x%08lx\n", InstallFlags & ~(INSTALLFLAG_FORCE | INSTALLFLAG_READONLY | INSTALLFLAG_NONINTERACTIVE));
		SetLastError(ERROR_INVALID_FLAGS);
		goto cleanup;
	}

	/* Enumerate all devices of the system */
	DevInstData.hDevInfo = SetupDiGetClassDevsW(NULL, NULL, hwndParent, DIGCF_ALLCLASSES | DIGCF_PRESENT);
	if (DevInstData.hDevInfo == INVALID_HANDLE_VALUE)
		goto cleanup;
	DevInstData.devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
	for (i = 0; ; i++)
	{
		if (!SetupDiEnumDeviceInfo(DevInstData.hDevInfo, i, &DevInstData.devInfoData))
		{
			if (GetLastError() != ERROR_NO_MORE_ITEMS)
			{
				TRACE("SetupDiEnumDeviceInfo() failed with error 0x%lx\n", GetLastError());
				goto cleanup;
			}
			/* This error was expected */
			break;
		}

		/* Get Hardware ID */
		HeapFree(GetProcessHeap(), 0, Buffer);
		Buffer = NULL;
		BufferSize = 0;
		while (!SetupDiGetDeviceRegistryPropertyW(
			DevInstData.hDevInfo,
			&DevInstData.devInfoData,
			SPDRP_HARDWAREID,
			NULL,
			(PBYTE)Buffer,
			BufferSize,
			&BufferSize))
		{
			if (GetLastError() == ERROR_FILE_NOT_FOUND)
			{
				Buffer = NULL;
				break;
			}
			else if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
			{
				TRACE("SetupDiGetDeviceRegistryPropertyW() failed with error 0x%lx\n", GetLastError());
				goto cleanup;
			}
			/* This error was expected */
			HeapFree(GetProcessHeap(), 0, Buffer);
			Buffer = HeapAlloc(GetProcessHeap(), 0, BufferSize);
			if (!Buffer)
			{
				TRACE("HeapAlloc() failed\n", GetLastError());
				SetLastError(ERROR_NOT_ENOUGH_MEMORY);
				goto cleanup;
			}
		}
		if (Buffer == NULL)
			continue;

		/* Check if we match the given hardware ID */
		FoundHardwareId = FALSE;
		for (CurrentHardwareId = Buffer; *CurrentHardwareId != UNICODE_NULL; CurrentHardwareId += wcslen(CurrentHardwareId) + 1)
		{
			if (wcscmp(CurrentHardwareId, HardwareId) == 0)
			{
				FoundHardwareId = TRUE;
				break;
			}
		}
		if (!FoundHardwareId)
			continue;

		/* We need to try to update the driver of this device */

		/* Get Instance ID */
		HeapFree(GetProcessHeap(), 0, Buffer);
		Buffer = NULL;
		if (SetupDiGetDeviceInstanceIdW(DevInstData.hDevInfo, &DevInstData.devInfoData, NULL, 0, &BufferSize))
		{
			/* Error, as the output buffer should be too small */
			SetLastError(ERROR_GEN_FAILURE);
			goto cleanup;
		}
		else if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
		{
			TRACE("SetupDiGetDeviceInstanceIdW() failed with error 0x%lx\n", GetLastError());
			goto cleanup;
		}
		else if ((Buffer = HeapAlloc(GetProcessHeap(), 0, BufferSize)) == NULL)
		{
			TRACE("HeapAlloc() failed\n", GetLastError());
			SetLastError(ERROR_NOT_ENOUGH_MEMORY);
			goto cleanup;
		}
		else if (!SetupDiGetDeviceInstanceIdW(DevInstData.hDevInfo, &DevInstData.devInfoData, Buffer, BufferSize, NULL))
		{
			TRACE("SetupDiGetDeviceInstanceIdW() failed with error 0x%lx\n", GetLastError());
			goto cleanup;
		}
		TRACE("Trying to update the driver of %S\n", Buffer);

		/* Search driver in the specified .inf file */
		if (!SearchDriver(&DevInstData, NULL, FullInfPath))
		{
			TRACE("SearchDriver() failed with error 0x%lx\n", GetLastError());
			continue;
		}

		/* FIXME: HACK! We shouldn't check of ERROR_PRIVILEGE_NOT_HELD */
		//if (!InstallCurrentDriver(&DevInstData))
		if (!InstallCurrentDriver(&DevInstData) && GetLastError() != ERROR_PRIVILEGE_NOT_HELD)
		{
			TRACE("InstallCurrentDriver() failed with error 0x%lx\n", GetLastError());
			continue;
		}

		FoundAtLeastOneDevice = TRUE;
	}

	if (FoundAtLeastOneDevice)
	{
		SetLastError(NO_ERROR);
		ret = TRUE;
	}
	else
	{
		TRACE("No device found with HardwareID %S\n", HardwareId);
		SetLastError(ERROR_NO_SUCH_DEVINST);
	}

cleanup:
	if (DevInstData.hDevInfo != INVALID_HANDLE_VALUE)
		SetupDiDestroyDeviceInfoList(DevInstData.hDevInfo);
	HeapFree(GetProcessHeap(), 0, Buffer);
	return ret;
}

/*
* @implemented
*/
BOOL WINAPI
UpdateDriverForPlugAndPlayDevicesA(
	IN HWND hwndParent,
	IN LPCSTR HardwareId,
	IN LPCSTR FullInfPath,
	IN DWORD InstallFlags,
	OUT PBOOL bRebootRequired OPTIONAL)
{
	BOOL Result;
	LPWSTR HardwareIdW = NULL;
	LPWSTR FullInfPathW = NULL;

	int len = MultiByteToWideChar(CP_ACP, 0, HardwareId, -1, NULL, 0);
	HardwareIdW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
	if (!HardwareIdW)
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return FALSE;
	}
	MultiByteToWideChar(CP_ACP, 0, HardwareId, -1, HardwareIdW, len);

	len = MultiByteToWideChar(CP_ACP, 0, FullInfPath, -1, NULL, 0);
	FullInfPathW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
	if (!FullInfPathW)
	{
		HeapFree(GetProcessHeap(), 0, HardwareIdW);
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return FALSE;
	}
	MultiByteToWideChar(CP_ACP, 0, FullInfPath, -1, FullInfPathW, len);

	Result = UpdateDriverForPlugAndPlayDevicesW(
		hwndParent,
		HardwareIdW,
		FullInfPathW,
		InstallFlags,
		bRebootRequired);

	HeapFree(GetProcessHeap(), 0, HardwareIdW);
	HeapFree(GetProcessHeap(), 0, FullInfPathW);

	return Result;
}

/* Directory and InfFile MUST NOT be specified simultaneously */
static BOOL
SearchDriver(
	IN PDEVINSTDATA DevInstData,
	IN LPCTSTR Directory OPTIONAL,
	IN LPCTSTR InfFile OPTIONAL)
{
	SP_DEVINSTALL_PARAMS DevInstallParams = {0,};
	BOOL ret;

	DevInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
	if (!SetupDiGetDeviceInstallParams(DevInstData->hDevInfo, &DevInstData->devInfoData, &DevInstallParams))
	{
		TRACE("SetupDiGetDeviceInstallParams() failed with error 0x%lx\n", GetLastError());
		return FALSE;
	}
	DevInstallParams.FlagsEx |= DI_FLAGSEX_ALLOWEXCLUDEDDRVS;

	if (InfFile)
	{
		DevInstallParams.Flags |= DI_ENUMSINGLEINF;
		_tcsncpy(DevInstallParams.DriverPath, InfFile, MAX_PATH);
	}
	else if (Directory)
	{
		DevInstallParams.Flags &= ~DI_ENUMSINGLEINF;
		_tcsncpy(DevInstallParams.DriverPath, Directory, MAX_PATH);
	}
	else
	{
		DevInstallParams.Flags &= ~DI_ENUMSINGLEINF;
		*DevInstallParams.DriverPath = _T('\0');
	}

	ret = SetupDiSetDeviceInstallParams(
		DevInstData->hDevInfo,
		&DevInstData->devInfoData,
		&DevInstallParams);
	if (!ret)
	{
		TRACE("SetupDiSetDeviceInstallParams() failed with error 0x%lx\n", GetLastError());
		return FALSE;
	}

	ret = SetupDiBuildDriverInfoList(
		DevInstData->hDevInfo,
		&DevInstData->devInfoData,
		SPDIT_COMPATDRIVER);
	if (!ret)
	{
		TRACE("SetupDiBuildDriverInfoList() failed with error 0x%lx\n", GetLastError());
		return FALSE;
	}

	DevInstData->drvInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
	ret = SetupDiEnumDriverInfo(
		DevInstData->hDevInfo,
		&DevInstData->devInfoData,
		SPDIT_COMPATDRIVER,
		0,
		&DevInstData->drvInfoData);
	if (!ret)
	{
		if (GetLastError() == ERROR_NO_MORE_ITEMS)
			return FALSE;
		TRACE("SetupDiEnumDriverInfo() failed with error 0x%lx\n", GetLastError());
		return FALSE;
	}

	return TRUE;
}

static BOOL
IsDots(IN LPCTSTR str)
{
	if(_tcscmp(str, _T(".")) && _tcscmp(str, _T(".."))) return FALSE;
	return TRUE;
}

static LPCTSTR
GetFileExt(IN LPTSTR FileName)
{
	LPCTSTR Dot;

	Dot = _tcsrchr(FileName, _T('.'));
	if (!Dot)
		return _T("");

	return Dot;
}

static BOOL
SearchDriverRecursive(
	IN PDEVINSTDATA DevInstData,
	IN LPCTSTR Path)
{
	WIN32_FIND_DATA wfd;
	TCHAR DirPath[MAX_PATH];
	TCHAR FileName[MAX_PATH];
	TCHAR FullPath[MAX_PATH];
	TCHAR LastDirPath[MAX_PATH] = _T("");
	TCHAR PathWithPattern[MAX_PATH];
	BOOL ok = TRUE;
	BOOL retval = FALSE;
	HANDLE hFindFile = INVALID_HANDLE_VALUE;

	_tcscpy(DirPath, Path);

	if (DirPath[_tcsclen(DirPath) - 1] != '\\')
		_tcscat(DirPath, _T("\\"));

	_tcscpy(PathWithPattern, DirPath);
	_tcscat(PathWithPattern, _T("\\*"));

	for (hFindFile = FindFirstFile(PathWithPattern, &wfd);
		ok && hFindFile != INVALID_HANDLE_VALUE;
		ok = FindNextFile(hFindFile, &wfd))
	{

		_tcscpy(FileName, wfd.cFileName);
		if (IsDots(FileName))
			continue;

		if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			/* Recursive search */
			_tcscpy(FullPath, DirPath);
			_tcscat(FullPath, FileName);
			if (SearchDriverRecursive(DevInstData, FullPath))
			{
				retval = TRUE;
				/* We continue the search for a better driver */
			}
		}
		else
		{
			LPCTSTR pszExtension = GetFileExt(FileName);

			if ((_tcsicmp(pszExtension, _T(".inf")) == 0) && (_tcscmp(LastDirPath, DirPath) != 0))
			{
				_tcscpy(LastDirPath, DirPath);

				if (_tcsclen(DirPath) > MAX_PATH)
					/* Path is too long to be searched */
					continue;

				if (SearchDriver(DevInstData, DirPath, NULL))
				{
					retval = TRUE;
					/* We continue the search for a better driver */
				}

			}
		}
	}

	if (hFindFile != INVALID_HANDLE_VALUE)
		FindClose(hFindFile);
	return retval;
}

BOOL
ScanFoldersForDriver(
	IN PDEVINSTDATA DevInstData)
{
	BOOL result;

	/* Search in default location */
	result = SearchDriver(DevInstData, NULL, NULL);

	if (DevInstData->CustomSearchPath)
	{
		/* Search only in specified paths */
		/* We need to check all specified directories to be
		 * sure to find the best driver for the device.
		 */
		LPCTSTR Path;
		for (Path = DevInstData->CustomSearchPath; *Path != '\0'; Path += _tcslen(Path) + 1)
		{
			TRACE("Search driver in %S\n", Path);
			if (_tcslen(Path) == 2 && Path[1] == ':')
			{
				if (SearchDriverRecursive(DevInstData, Path))
					result = TRUE;
			}
			else
			{
				if (SearchDriver(DevInstData, Path, NULL))
					result = TRUE;
			}
		}
	}

	return result;
}

BOOL
PrepareFoldersToScan(
	IN PDEVINSTDATA DevInstData,
	IN BOOL IncludeRemovableDevices,
	IN BOOL IncludeCustomPath,
	IN HWND hwndCombo OPTIONAL)
{
	TCHAR drive[] = {'?',':',0};
	DWORD dwDrives = 0;
	DWORD i;
	UINT nType;
	DWORD CustomTextLength = 0;
	DWORD LengthNeeded = 0;
	LPTSTR Buffer;

	TRACE("Include removable devices: %s\n", IncludeRemovableDevices ? "yes" : "no");
	TRACE("Include custom path      : %s\n", IncludeCustomPath ? "yes" : "no");

	/* Calculate length needed to store the search paths */
	if (IncludeRemovableDevices)
	{
		dwDrives = GetLogicalDrives();
		for (drive[0] = 'A', i = 1; drive[0] <= 'Z'; drive[0]++, i <<= 1)
		{
			if (dwDrives & i)
			{
				nType = GetDriveType(drive);
				if (nType == DRIVE_REMOVABLE || nType == DRIVE_CDROM)
				{
					LengthNeeded += 3;
				}
			}
		}
	}
	if (IncludeCustomPath)
	{
		CustomTextLength = 1 + ComboBox_GetTextLength(hwndCombo);
		LengthNeeded += CustomTextLength;
	}

	/* Allocate space for search paths */
	HeapFree(GetProcessHeap(), 0, DevInstData->CustomSearchPath);
	DevInstData->CustomSearchPath = Buffer = HeapAlloc(
		GetProcessHeap(),
		0,
		(LengthNeeded + 1) * sizeof(TCHAR));
	if (!Buffer)
	{
		TRACE("HeapAlloc() failed\n");
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return FALSE;
	}

	/* Fill search paths */
	if (IncludeRemovableDevices)
	{
		for (drive[0] = 'A', i = 1; drive[0] <= 'Z'; drive[0]++, i <<= 1)
		{
			if (dwDrives & i)
			{
				nType = GetDriveType(drive);
				if (nType == DRIVE_REMOVABLE || nType == DRIVE_CDROM)
				{
					Buffer += 1 + _stprintf(Buffer, drive);
				}
			}
		}
	}
	if (IncludeCustomPath)
	{
		Buffer += 1 + ComboBox_GetText(hwndCombo, Buffer, CustomTextLength);
	}
	*Buffer = _T('\0');

	return TRUE;
}

BOOL
InstallCurrentDriver(
	IN PDEVINSTDATA DevInstData)
{
	BOOL ret;

	TRACE("Installing driver %S: %S\n", DevInstData->drvInfoData.MfgName, DevInstData->drvInfoData.Description);

	ret = SetupDiCallClassInstaller(
		DIF_SELECTBESTCOMPATDRV,
		DevInstData->hDevInfo,
		&DevInstData->devInfoData);
	if (!ret)
	{
		TRACE("SetupDiCallClassInstaller(DIF_SELECTBESTCOMPATDRV) failed with error 0x%lx\n", GetLastError());
		return FALSE;
	}

	ret = SetupDiCallClassInstaller(
		DIF_ALLOW_INSTALL,
		DevInstData->hDevInfo,
		&DevInstData->devInfoData);
	if (!ret)
	{
		TRACE("SetupDiCallClassInstaller(DIF_ALLOW_INSTALL) failed with error 0x%lx\n", GetLastError());
		return FALSE;
	}

	ret = SetupDiCallClassInstaller(
		DIF_NEWDEVICEWIZARD_PREANALYZE,
		DevInstData->hDevInfo,
		&DevInstData->devInfoData);
	if (!ret)
	{
		TRACE("SetupDiCallClassInstaller(DIF_NEWDEVICEWIZARD_PREANALYZE) failed with error 0x%lx\n", GetLastError());
		return FALSE;
	}

	ret = SetupDiCallClassInstaller(
		DIF_NEWDEVICEWIZARD_POSTANALYZE,
		DevInstData->hDevInfo,
		&DevInstData->devInfoData);
	if (!ret)
	{
		TRACE("SetupDiCallClassInstaller(DIF_NEWDEVICEWIZARD_POSTANALYZE) failed with error 0x%lx\n", GetLastError());
		return FALSE;
	}

	ret = SetupDiCallClassInstaller(
		DIF_INSTALLDEVICEFILES,
		DevInstData->hDevInfo,
		&DevInstData->devInfoData);
	if (!ret)
	{
		TRACE("SetupDiCallClassInstaller(DIF_INSTALLDEVICEFILES) failed with error 0x%lx\n", GetLastError());
		return FALSE;
	}

	ret = SetupDiCallClassInstaller(
		DIF_REGISTER_COINSTALLERS,
		DevInstData->hDevInfo,
		&DevInstData->devInfoData);
	if (!ret)
	{
		TRACE("SetupDiCallClassInstaller(DIF_REGISTER_COINSTALLERS) failed with error 0x%lx\n", GetLastError());
		return FALSE;
	}

	ret = SetupDiCallClassInstaller(
		DIF_INSTALLINTERFACES,
		DevInstData->hDevInfo,
		&DevInstData->devInfoData);
	if (!ret)
	{
		TRACE("SetupDiCallClassInstaller(DIF_INSTALLINTERFACES) failed with error 0x%lx\n", GetLastError());
		return FALSE;
	}

	ret = SetupDiCallClassInstaller(
		DIF_INSTALLDEVICE,
		DevInstData->hDevInfo,
		&DevInstData->devInfoData);
	if (!ret)
	{
		TRACE("SetupDiCallClassInstaller(DIF_INSTALLDEVICE) failed with error 0x%lx\n", GetLastError());
		return FALSE;
	}

	ret = SetupDiCallClassInstaller(
		DIF_NEWDEVICEWIZARD_FINISHINSTALL,
		DevInstData->hDevInfo,
		&DevInstData->devInfoData);
	if (!ret)
	{
		TRACE("SetupDiCallClassInstaller(DIF_NEWDEVICEWIZARD_FINISHINSTALL) failed with error 0x%lx\n", GetLastError());
		return FALSE;
	}

	ret = SetupDiCallClassInstaller(
		DIF_DESTROYPRIVATEDATA,
		DevInstData->hDevInfo,
		&DevInstData->devInfoData);
	if (!ret)
	{
		TRACE("SetupDiCallClassInstaller(DIF_DESTROYPRIVATEDATA) failed with error 0x%lx\n", GetLastError());
		return FALSE;
	}

	return TRUE;
}

/*
* @implemented
*/
BOOL WINAPI
DevInstallW(
	IN HWND hWndParent,
	IN HINSTANCE hInstance,
	IN LPCWSTR InstanceId,
	IN INT Show)
{
	PDEVINSTDATA DevInstData = NULL;
	BOOL ret;
	DWORD config_flags;
	BOOL retval = FALSE;

	if (!IsUserAdmin())
	{
		/* XP kills the process... */
		ExitProcess(ERROR_ACCESS_DENIED);
	}

	DevInstData = HeapAlloc(GetProcessHeap(), 0, sizeof(DEVINSTDATA));
	if (!DevInstData)
	{
		TRACE("HeapAlloc() failed\n");
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		goto cleanup;
	}

	/* Clear devinst data */
	ZeroMemory(DevInstData, sizeof(DEVINSTDATA));
	DevInstData->devInfoData.cbSize = 0; /* Tell if the devInfoData is valid */

	/* Fill devinst data */
	DevInstData->hDevInfo = SetupDiCreateDeviceInfoListExW(NULL, NULL, NULL, NULL);
	if (DevInstData->hDevInfo == INVALID_HANDLE_VALUE)
	{
		TRACE("SetupDiCreateDeviceInfoListExW() failed with error 0x%lx\n", GetLastError());
		goto cleanup;
	}

	DevInstData->devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
	ret = SetupDiOpenDeviceInfoW(
		DevInstData->hDevInfo,
		InstanceId,
		NULL,
		0, /* Open flags */
		&DevInstData->devInfoData);
	if (!ret)
	{
		TRACE("SetupDiOpenDeviceInfoW() failed with error 0x%lx (InstanceId %S)\n", GetLastError(), InstanceId);
		DevInstData->devInfoData.cbSize = 0;
		goto cleanup;
	}

	SetLastError(ERROR_GEN_FAILURE);
	ret = SetupDiGetDeviceRegistryProperty(
		DevInstData->hDevInfo,
		&DevInstData->devInfoData,
		SPDRP_DEVICEDESC,
		&DevInstData->regDataType,
		NULL, 0,
		&DevInstData->requiredSize);

	if (!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER && DevInstData->regDataType == REG_SZ)
	{
		DevInstData->buffer = HeapAlloc(GetProcessHeap(), 0, DevInstData->requiredSize);
		if (!DevInstData->buffer)
		{
			TRACE("HeapAlloc() failed\n");
			SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		}
		else
		{
			ret = SetupDiGetDeviceRegistryProperty(
				DevInstData->hDevInfo,
				&DevInstData->devInfoData,
				SPDRP_DEVICEDESC,
				&DevInstData->regDataType,
				DevInstData->buffer, DevInstData->requiredSize,
				&DevInstData->requiredSize);
		}
	}
	if (!ret)
	{
		TRACE("SetupDiGetDeviceRegistryProperty() failed with error 0x%lx (InstanceId %S)\n", GetLastError(), InstanceId);
		goto cleanup;
	}

	if (SetupDiGetDeviceRegistryProperty(
		DevInstData->hDevInfo,
		&DevInstData->devInfoData,
		SPDRP_CONFIGFLAGS,
		NULL,
		(BYTE *)&config_flags,
		sizeof(config_flags),
		NULL))
	{
		if (config_flags & CONFIGFLAG_FAILEDINSTALL)
		{
			/* The device is disabled */
			retval = TRUE;
			goto cleanup;
		}
	}

	TRACE("Installing %S (%S)\n", DevInstData->buffer, InstanceId);

	/* Search driver in default location and removable devices */
	if (!PrepareFoldersToScan(DevInstData, FALSE, FALSE, NULL))
	{
		TRACE("PrepareFoldersToScan() failed with error 0x%lx\n", GetLastError());
		goto cleanup;
	}
	if (ScanFoldersForDriver(DevInstData))
	{
		/* Driver found ; install it */
		retval = InstallCurrentDriver(DevInstData);
		goto cleanup;
	}
	else if (Show == SW_HIDE)
	{
		/* We can't show the wizard. Fail the install */
		goto cleanup;
	}

	/* Prepare the wizard, and display it */
	retval = DisplayWizard(DevInstData, hWndParent, IDD_WELCOMEPAGE);

cleanup:
	if (DevInstData)
	{
		if (DevInstData->devInfoData.cbSize != 0)
		{
			if (!SetupDiDestroyDriverInfoList(DevInstData->hDevInfo, &DevInstData->devInfoData, SPDIT_COMPATDRIVER))
				TRACE("SetupDiDestroyDriverInfoList() failed with error 0x%lx\n", GetLastError());
		}
		if (DevInstData->hDevInfo != INVALID_HANDLE_VALUE)
		{
			if (!SetupDiDestroyDeviceInfoList(DevInstData->hDevInfo))
				TRACE("SetupDiDestroyDeviceInfoList() failed with error 0x%lx\n", GetLastError());
		}
		HeapFree(GetProcessHeap(), 0, DevInstData->buffer);
		HeapFree(GetProcessHeap(), 0, DevInstData);
	}

	return retval;
}

/*
* @unimplemented
*/
BOOL WINAPI
ClientSideInstallW(
	IN HWND hWndOwner,
	IN DWORD dwUnknownFlags,
	IN LPWSTR lpNamedPipeName)
{
	/* NOTE: pNamedPipeName is in the format:
	 *       "\\.\pipe\PNP_Device_Install_Pipe_0.{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}"
	 */
	FIXME("Stub\n");
	return FALSE;
}

BOOL WINAPI
DllMain(
	IN HINSTANCE hInstance,
	IN DWORD dwReason,
	IN LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		INITCOMMONCONTROLSEX InitControls;

		DisableThreadLibraryCalls(hInstance);

		InitControls.dwSize = sizeof(INITCOMMONCONTROLSEX);
		InitControls.dwICC = ICC_PROGRESS_CLASS;
		InitCommonControlsEx(&InitControls);
		hDllInstance = hInstance;
	}

	return TRUE;
}
