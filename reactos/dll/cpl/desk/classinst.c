/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Display Control Panel
 * FILE:            lib/cpl/desk/classinst.c
 * PURPOSE:         Display class installer
 *
 * PROGRAMMERS:     Hervé Poussineau (hpoussin@reactos.org)
 */

//#define NDEBUG
#include <debug.h>

#include "desk.h"

DWORD WINAPI
DisplayClassInstaller(
	IN DI_FUNCTION InstallFunction,
	IN HDEVINFO DeviceInfoSet,
	IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL)
{
	SP_DEVINSTALL_PARAMS InstallParams;
	SP_DRVINFO_DATA DriverInfoData;
	HINF hInf = INVALID_HANDLE_VALUE;
	TCHAR SectionName[MAX_PATH];
	TCHAR ServiceName[MAX_SERVICE_NAME_LEN];
	SP_DRVINFO_DETAIL_DATA DriverInfoDetailData;
	HKEY hDriverKey = INVALID_HANDLE_VALUE; /* SetupDiOpenDevRegKey returns INVALID_HANDLE_VALUE in case of error! */
	HKEY hSettingsKey = NULL;
	HKEY hServicesKey = NULL;
	HKEY hServiceKey = NULL;
	HKEY hDeviceSubKey = NULL;
	DWORD disposition;
	BOOL result;
	LONG rc;

	if (InstallFunction != DIF_INSTALLDEVICE)
		return ERROR_DI_DO_DEFAULT;

	/* Set DI_DONOTCALLCONFIGMG flag */
	InstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
	result = SetupDiGetDeviceInstallParams(DeviceInfoSet, DeviceInfoData, &InstallParams);
	if (!result)
	{
		rc = GetLastError();
		DPRINT("SetupDiGetDeviceInstallParams() failed with error 0x%lx\n", rc);
		goto cleanup;
	}

	InstallParams.Flags |= DI_DONOTCALLCONFIGMG;

	result = SetupDiSetDeviceInstallParams(DeviceInfoSet, DeviceInfoData, &InstallParams);
	if (!result)
	{
		rc = GetLastError();
		DPRINT("SetupDiSetDeviceInstallParams() failed with error 0x%lx\n", rc);
		goto cleanup;
	}

	/* Do normal install */
	result = SetupDiInstallDevice(DeviceInfoSet, DeviceInfoData);
	if (!result)
	{
		rc = GetLastError();
		DPRINT("SetupDiInstallDevice() failed with error 0x%lx\n", rc);
		goto cleanup;
	}

	/* Get .inf file name and section name */
	DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
	result = SetupDiGetSelectedDriver(DeviceInfoSet, DeviceInfoData, &DriverInfoData);
	if (!result)
	{
		rc = GetLastError();
		DPRINT("SetupDiGetSelectedDriver() failed with error 0x%lx\n", rc);
		goto cleanup;
	}

	DriverInfoDetailData.cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);
	result = SetupDiGetDriverInfoDetail(
		DeviceInfoSet, DeviceInfoData,
		&DriverInfoData, &DriverInfoDetailData,
		sizeof(SP_DRVINFO_DETAIL_DATA), NULL);
	if (!result && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
	{
		rc = GetLastError();
		DPRINT("SetupDiGetDriverInfoDetail() failed with error 0x%lx\n", rc);
		goto cleanup;
	}

	hInf = SetupOpenInfFile(DriverInfoDetailData.InfFileName, NULL, INF_STYLE_WIN4, NULL);
	if (hInf == INVALID_HANDLE_VALUE)
	{
		rc = GetLastError();
		DPRINT("SetupOpenInfFile() failed with error 0x%lx\n", rc);
		goto cleanup;
	}

	result = SetupDiGetActualSectionToInstall(
		hInf, DriverInfoDetailData.SectionName,
		SectionName, MAX_PATH - _tcslen(_T(".SoftwareSettings")), NULL, NULL);
	if (!result)
	{
		rc = GetLastError();
		DPRINT("SetupDiGetActualSectionToInstall() failed with error 0x%lx\n", rc);
		goto cleanup;
	}
	_tcscat(SectionName, _T(".SoftwareSettings"));

	/* Open driver registry key and create Settings subkey */
	hDriverKey = SetupDiOpenDevRegKey(
		DeviceInfoSet, DeviceInfoData,
		DICS_FLAG_GLOBAL, 0, DIREG_DRV,
		KEY_CREATE_SUB_KEY);
	if (hDriverKey == INVALID_HANDLE_VALUE)
	{
		rc = GetLastError();
		DPRINT("SetupDiOpenDevRegKey() failed with error 0x%lx\n", rc);
		goto cleanup;
	}
	rc = RegCreateKeyEx(
		hDriverKey, L"Settings",
		0, NULL, REG_OPTION_NON_VOLATILE, 
#if _WIN32_WINNT >= 0x502
		KEY_READ | KEY_WRITE,
#else
		KEY_ALL_ACCESS,
#endif
		NULL, &hSettingsKey, &disposition);
	if (rc != ERROR_SUCCESS)
	{
		DPRINT("RegCreateKeyEx() failed with error 0x%lx\n", rc);
		goto cleanup;
	}

	/* Install .SoftwareSettings to Settings subkey */
	result = SetupInstallFromInfSection(
		InstallParams.hwndParent, hInf, SectionName,
		SPINST_REGISTRY, hSettingsKey,
		NULL, 0, NULL, NULL,
		NULL, NULL);
	if (!result)
	{
		rc = GetLastError();
		DPRINT("SetupInstallFromInfSection() failed with error 0x%lx\n", rc);
		goto cleanup;
	}

	/* Get service name and open service registry key */
	result = SetupDiGetDeviceRegistryProperty(
		DeviceInfoSet, DeviceInfoData,
		SPDRP_SERVICE, NULL,
		(PBYTE)ServiceName, MAX_SERVICE_NAME_LEN * sizeof(TCHAR), NULL);
	if (!result)
	{
		rc = GetLastError();
		DPRINT("SetupDiGetDeviceRegistryProperty() failed with error 0x%lx\n", rc);
		goto cleanup;
	}

	rc = RegOpenKeyEx(
		HKEY_LOCAL_MACHINE, _T("SYSTEM\\CurrentControlSet\\Services"),
		0, KEY_ENUMERATE_SUB_KEYS, &hServicesKey);
	if (rc != ERROR_SUCCESS)
	{
		DPRINT("RegOpenKeyEx() failed with error 0x%lx\n", rc);
		goto cleanup;
	}
	rc = RegOpenKeyEx(
		hServicesKey, ServiceName,
		0, KEY_CREATE_SUB_KEY, &hServiceKey);
	if (rc != ERROR_SUCCESS)
	{
		DPRINT("RegOpenKeyEx() failed with error 0x%lx\n", rc);
		goto cleanup;
	}

	/* Create a Device0 subkey (FIXME: do a loop to find a free number?) */
	rc = RegCreateKeyEx(
		hServiceKey, _T("Device0"), 0, NULL,
		REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL,
		&hDeviceSubKey, &disposition);
	if (rc != ERROR_SUCCESS)
	{
		DPRINT("RegCreateKeyEx() failed with error 0x%lx\n", rc);
		goto cleanup;
	}
	if (disposition != REG_CREATED_NEW_KEY)
	{
		rc = ERROR_GEN_FAILURE;
		DPRINT("RegCreateKeyEx() failed\n");
		goto cleanup;
	}

	/* Install SoftwareSettings section */
	/* Yes, we're installing this section for the second time.
	 * We don't want to create a link to Settings subkey */
	result = SetupInstallFromInfSection(
		InstallParams.hwndParent, hInf, SectionName,
		SPINST_REGISTRY, hDeviceSubKey,
		NULL, 0, NULL, NULL,
		NULL, NULL);
	if (!result)
	{
		rc = GetLastError();
		DPRINT("SetupInstallFromInfSection() failed with error 0x%lx\n", rc);
		goto cleanup;
	}

	/* FIXME: install OpenGLSoftwareSettings section */

	rc = ERROR_SUCCESS;

cleanup:
	if (hInf != INVALID_HANDLE_VALUE)
		SetupCloseInfFile(hInf);
	if (hDriverKey != INVALID_HANDLE_VALUE)
	{
		/* SetupDiOpenDevRegKey returns INVALID_HANDLE_VALUE in case of error! */
		RegCloseKey(hDriverKey);
	}
	if (hSettingsKey != NULL)
		RegCloseKey(hSettingsKey);
	if (hServicesKey != NULL)
		RegCloseKey(hServicesKey);
	if (hServiceKey != NULL)
		RegCloseKey(hServiceKey);
	if (hDeviceSubKey != NULL)
		RegCloseKey(hDeviceSubKey);

	return rc;
}
