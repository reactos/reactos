/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS New devices installation
 * FILE:            lib/newdev/newdev.c
 * PURPOSE:         New devices installation
 *
 * PROGRAMMERS:     Hervé Poussineau (hpoussin@reactos.org)
 */

#define NDEBUG
#include <debug.h>

#include "newdev.h"

BOOL WINAPI
DevInstallW(
	IN HWND hWndParent,
	IN HINSTANCE hInstance,
	IN LPCWSTR InstanceId,
	IN INT Show)
{
	HDEVINFO hDevInfo;
	SP_DEVINFO_DATA devInfoData;
	DWORD requiredSize;
	DWORD regDataType;
	PBYTE buffer = NULL;
	SP_DRVINFO_DATA drvInfoData;
	BOOL ret;

	devInfoData.cbSize = 0; /* Tell if the devInfoData is valid */

	hDevInfo = SetupDiCreateDeviceInfoListExW(NULL, NULL, NULL, NULL);
	if (hDevInfo == INVALID_HANDLE_VALUE)
	{
		DPRINT("SetupDiCreateDeviceInfoListExW() failed with error 0x%lx\n", GetLastError());
		ret = FALSE;
		goto cleanup;
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
		DPRINT("SetupDiOpenDeviceInfoW() failed with error 0x%lx\n", GetLastError());
		devInfoData.cbSize = 0;
		goto cleanup;
	}

	SetLastError(ERROR_GEN_FAILURE);
	ret = SetupDiGetDeviceRegistryProperty(
		hDevInfo,
		&devInfoData,
		SPDRP_DEVICEDESC,
		&regDataType,
		NULL, 0,
		&requiredSize);
	if (!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER && regDataType == REG_SZ)
	{
		buffer = HeapAlloc(GetProcessHeap(), 0, requiredSize);
		if (!buffer)
		{
			DPRINT("HeapAlloc() failed\n");
			SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		}
		else
		{
			ret = SetupDiGetDeviceRegistryProperty(
				hDevInfo,
				&devInfoData,
				SPDRP_DEVICEDESC,
				&regDataType,
				buffer, requiredSize,
				&requiredSize);
		}
	}
	if (!ret)
	{
		DPRINT("SetupDiGetDeviceRegistryProperty() failed with error 0x%lx\n", GetLastError());
		goto cleanup;
	}

	DPRINT("Installing %s (%S)\n", buffer, InstanceId);

	ret = SetupDiBuildDriverInfoList(hDevInfo, &devInfoData, SPDIT_COMPATDRIVER);
	if (!ret)
	{
		DPRINT("SetupDiBuildDriverInfoList() failed with error 0x%lx\n", GetLastError());
		goto cleanup;
	}

	drvInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
	ret = SetupDiEnumDriverInfo(
		hDevInfo,
		&devInfoData,
		SPDIT_COMPATDRIVER,
		0,
		&drvInfoData);
	if (!ret)
	{
		DPRINT("SetupDiEnumDriverInfo() failed with error 0x%lx\n", GetLastError());
		goto cleanup;
	}
	DPRINT("Installing driver %s: %s\n", drvInfoData.MfgName, drvInfoData.Description);

	ret = SetupDiCallClassInstaller(
		DIF_SELECTBESTCOMPATDRV,
		hDevInfo,
		&devInfoData);
	if (!ret)
	{
		DPRINT("SetupDiCallClassInstaller(DIF_SELECTBESTCOMPATDRV) failed with error 0x%lx\n", GetLastError());
		goto cleanup;
	}

	ret = SetupDiCallClassInstaller(
		DIF_ALLOW_INSTALL,
		hDevInfo,
		&devInfoData);
	if (!ret)
	{
		DPRINT("SetupDiCallClassInstaller(DIF_ALLOW_INSTALL) failed with error 0x%lx\n", GetLastError());
		goto cleanup;
	}

	ret = SetupDiCallClassInstaller(
		DIF_NEWDEVICEWIZARD_PREANALYZE,
		hDevInfo,
		&devInfoData);
	if (!ret)
	{
		DPRINT("SetupDiCallClassInstaller(DIF_NEWDEVICEWIZARD_PREANALYZE) failed with error 0x%lx\n", GetLastError());
		goto cleanup;
	}

	ret = SetupDiCallClassInstaller(
		DIF_NEWDEVICEWIZARD_POSTANALYZE,
		hDevInfo,
		&devInfoData);
	if (!ret)
	{
		DPRINT("SetupDiCallClassInstaller(DIF_NEWDEVICEWIZARD_POSTANALYZE) failed with error 0x%lx\n", GetLastError());
		goto cleanup;
	}

	ret = SetupDiCallClassInstaller(
		DIF_INSTALLDEVICEFILES,
		hDevInfo,
		&devInfoData);
	if (!ret)
	{
		DPRINT("SetupDiCallClassInstaller(DIF_INSTALLDEVICEFILES) failed with error 0x%lx\n", GetLastError());
		goto cleanup;
	}

	ret = SetupDiCallClassInstaller(
		DIF_REGISTER_COINSTALLERS,
		hDevInfo,
		&devInfoData);
	if (!ret)
	{
		DPRINT("SetupDiCallClassInstaller(DIF_REGISTER_COINSTALLERS) failed with error 0x%lx\n", GetLastError());
		goto cleanup;
	}

	ret = SetupDiCallClassInstaller(
		DIF_INSTALLINTERFACES,
		hDevInfo,
		&devInfoData);
	if (!ret)
	{
		DPRINT("SetupDiCallClassInstaller(DIF_INSTALLINTERFACES) failed with error 0x%lx\n", GetLastError());
		goto cleanup;
	}

	ret = SetupDiCallClassInstaller(
		DIF_INSTALLDEVICE,
		hDevInfo,
		&devInfoData);
	if (!ret)
	{
		DPRINT("SetupDiCallClassInstaller(DIF_INSTALLDEVICE) failed with error 0x%lx\n", GetLastError());
		goto cleanup;
	}

	ret = SetupDiCallClassInstaller(
		DIF_NEWDEVICEWIZARD_FINISHINSTALL,
		hDevInfo,
		&devInfoData);
	if (!ret)
	{
		DPRINT("SetupDiCallClassInstaller(DIF_NEWDEVICEWIZARD_FINISHINSTALL) failed with error 0x%lx\n", GetLastError());
		goto cleanup;
	}

	ret = SetupDiCallClassInstaller(
		DIF_DESTROYPRIVATEDATA,
		hDevInfo,
		&devInfoData);
	if (!ret)
	{
		DPRINT("SetupDiCallClassInstaller(DIF_DESTROYPRIVATEDATA) failed with error 0x%lx\n", GetLastError());
		goto cleanup;
	}

cleanup:
	if (devInfoData.cbSize != 0)
	{
		if (!SetupDiDestroyDriverInfoList(hDevInfo, &devInfoData, SPDIT_COMPATDRIVER))
			DPRINT("SetupDiDestroyDriverInfoList() failed with error 0x%lx\n", GetLastError());
	}

	if (hDevInfo != INVALID_HANDLE_VALUE)
	{
		if (!SetupDiDestroyDeviceInfoList(hDevInfo))
			DPRINT("SetupDiDestroyDeviceInfoList() failed with error 0x%lx\n", GetLastError());
	}

	if (buffer)
		HeapFree(GetProcessHeap(), 0, buffer);

	return ret;
}
