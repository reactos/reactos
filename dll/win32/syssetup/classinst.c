/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/storage/mountmgr/database.c
 * PURPOSE:     Class installers
 * PROGRAMMERS: Copyright 2006 Hervé Poussineau (hpoussin@reactos.org)
 */

#include <windows.h>
#include <setupapi.h>

#define NDEBUG
#include <debug.h>

DWORD WINAPI
KeyboardClassInstaller(
	IN DI_FUNCTION InstallFunction,
	IN HDEVINFO DeviceInfoSet,
	IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL)
{
	switch (InstallFunction)
	{
		default:
			DPRINT("Install function %u ignored\n", InstallFunction);
			return ERROR_DI_DO_DEFAULT;
	}
}

DWORD WINAPI
MouseClassInstaller(
	IN DI_FUNCTION InstallFunction,
	IN HDEVINFO DeviceInfoSet,
	IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL)
{
	switch (InstallFunction)
	{
		default:
			DPRINT("Install function %u ignored\n", InstallFunction);
			return ERROR_DI_DO_DEFAULT;
	}
}
