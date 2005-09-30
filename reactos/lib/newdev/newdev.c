/*
 * ReactOS New devices installation
 * Copyright (C) 2004 ReactOS Team
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
/*
 * PROJECT:         ReactOS New devices installation
 * FILE:            lib/newdev/newdev.c
 * PURPOSE:         New devices installation
 * PROGRAMMER:      Hervé Poussineau (hpoussin@reactos.org)
 */

#include <windows.h>
#include <setupapi.h>

ULONG DbgPrint(PCH Format,...);
#define UNIMPLEMENTED \
  DbgPrint("NEWDEV:  %s at %s:%d is UNIMPLEMENTED!\n",__FUNCTION__,__FILE__,__LINE__)
#define DPRINT1 DbgPrint("(%s:%d) ", __FILE__, __LINE__), DbgPrint

BOOL
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

	DbgPrint("OK\n");
	DbgPrint("Installing device %S\n", InstanceId);

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
