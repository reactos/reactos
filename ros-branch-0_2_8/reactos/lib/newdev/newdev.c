/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS New devices installation
 * FILE:            lib/newdev/newdev.c
 * PURPOSE:         New devices installation
 *
 * PROGRAMMERS:     Hervé Poussineau (hpoussin@reactos.org)
 */

#include "newdev.h"

BOOL WINAPI
DevInstallW(
	IN HWND Hwnd,
	IN HINSTANCE Handle,
	IN LPCWSTR InstanceId,
	IN INT Show)
{
	HDEVINFO hDevInfo;
	SP_DEVINFO_DATA devInfoData;
	SP_DRVINFO_DATA_W drvInfoData;
	DWORD index;
	BOOL ret;

	DPRINT1("Installing device %S\n", InstanceId);

	hDevInfo = SetupDiCreateDeviceInfoListExW(NULL, NULL, NULL, NULL);
	if (hDevInfo == INVALID_HANDLE_VALUE)
	{
		DPRINT1("SetupDiCreateDeviceInfoListExW() failed with error 0x%lx\n", GetLastError());
		return FALSE;
	}

	devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
	ret = SetupDiOpenDeviceInfoW(
		hDevInfo,
		InstanceId,
		NULL,
		0, /* Open flags */
		&devInfoData);
	if (!ret)
	{
		DPRINT1("SetupDiOpenDeviceInfoW() failed with error 0x%lx\n", GetLastError());
		return FALSE;
	}

	ret = SetupDiBuildDriverInfoList(hDevInfo, &devInfoData, SPDIT_COMPATDRIVER);
	if (!ret)
	{
		DPRINT1("SetupDiBuildDriverInfoList() failed with error 0x%lx\n", GetLastError());
		return FALSE;
	}

#ifndef NDEBUG
	ret = TRUE;
	index = 0;
	drvInfoData.cbSize = sizeof(SP_DRVINFO_DATA_W);
	while (ret)
	{
		ret = SetupDiEnumDriverInfoW(
			hDevInfo,
			&devInfoData,
			SPDIT_COMPATDRIVER,
			index,
			&drvInfoData);
		if (!ret)
		{
			if (GetLastError() != ERROR_NO_MORE_ITEMS)
			{
				DPRINT1("SetupDiEnumDriverInfoW() failed with error 0x%lx\n", GetLastError());
				return FALSE;
			}
			break;
		}
		index++;
		DPRINT1("- %S: %S\n", drvInfoData.MfgName, drvInfoData.Description);
	}
#endif

	ret = SetupDiCallClassInstaller(
		DIF_SELECTBESTCOMPATDRV,
		hDevInfo,
		&devInfoData);
	if (!ret)
	{
		DPRINT1("SetupDiCallClassInstaller(DIF_SELECTBESTCOMPATDRV) failed with error 0x%lx\n", GetLastError());
		return FALSE;
	}

	ret = SetupDiCallClassInstaller(
		DIF_ALLOW_INSTALL,
		hDevInfo,
		&devInfoData);
	if (!ret)
	{
		DPRINT1("SetupDiCallClassInstaller(DIF_ALLOW_INSTALL) failed with error 0x%lx\n", GetLastError());
		return FALSE;
	}

	ret = SetupDiCallClassInstaller(
		DIF_NEWDEVICEWIZARD_PREANALYZE,
		hDevInfo,
		&devInfoData);
	if (!ret)
	{
		DPRINT1("SetupDiCallClassInstaller(DIF_NEWDEVICEWIZARD_PREANALYZE) failed with error 0x%lx\n", GetLastError());
		return FALSE;
	}

	ret = SetupDiCallClassInstaller(
		DIF_NEWDEVICEWIZARD_POSTANALYZE,
		hDevInfo,
		&devInfoData);
	if (!ret)
	{
		DPRINT1("SetupDiCallClassInstaller(DIF_NEWDEVICEWIZARD_POSTANALYZE) failed with error 0x%lx\n", GetLastError());
		return FALSE;
	}

	ret = SetupDiCallClassInstaller(
		DIF_INSTALLDEVICEFILES,
		hDevInfo,
		&devInfoData);
	if (!ret)
	{
		DPRINT1("SetupDiCallClassInstaller(DIF_INSTALLDEVICEFILES) failed with error 0x%lx\n", GetLastError());
		return FALSE;
	}

	ret = SetupDiCallClassInstaller(
		DIF_REGISTER_COINSTALLERS,
		hDevInfo,
		&devInfoData);
	if (!ret)
	{
		DPRINT1("SetupDiCallClassInstaller(DIF_REGISTER_COINSTALLERS) failed with error 0x%lx\n", GetLastError());
		return FALSE;
	}

	ret = SetupDiCallClassInstaller(
		DIF_INSTALLINTERFACES,
		hDevInfo,
		&devInfoData);
	if (!ret)
	{
		DPRINT1("SetupDiCallClassInstaller(DIF_INSTALLINTERFACES) failed with error 0x%lx\n", GetLastError());
		return FALSE;
	}

	ret = SetupDiCallClassInstaller(
		DIF_INSTALLDEVICE,
		hDevInfo,
		&devInfoData);
	if (!ret)
	{
		DPRINT1("SetupDiCallClassInstaller(DIF_INSTALLDEVICE) failed with error 0x%lx\n", GetLastError());
		return FALSE;
	}

	ret = SetupDiCallClassInstaller(
		DIF_NEWDEVICEWIZARD_FINISHINSTALL,
		hDevInfo,
		&devInfoData);
	if (!ret)
	{
		DPRINT1("SetupDiCallClassInstaller(DIF_NEWDEVICEWIZARD_FINISHINSTALL) failed with error 0x%lx\n", GetLastError());
		return FALSE;
	}

	ret = SetupDiCallClassInstaller(
		DIF_DESTROYPRIVATEDATA,
		hDevInfo,
		&devInfoData);
	if (!ret)
	{
		DPRINT1("SetupDiCallClassInstaller(DIF_DESTROYPRIVATEDATA) failed with error 0x%lx\n", GetLastError());
		return FALSE;
	}

	ret = SetupDiDestroyDriverInfoList(hDevInfo, &devInfoData, SPDIT_COMPATDRIVER);
	if (!ret)
	{
		DPRINT1("SetupDiDestroyDriverInfoList() failed with error 0x%lx\n", GetLastError());
		return FALSE;
	}

	ret = SetupDiDestroyDeviceInfoList(hDevInfo);
	if (!ret)
	{
		DPRINT1("SetupDiDestroyDeviceInfoList() failed with error 0x%lx\n", GetLastError());
		return FALSE;
	}

	return TRUE;
}
