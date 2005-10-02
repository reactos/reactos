/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS New devices installation
 * FILE:            lib/newdev/stubs.c
 * PURPOSE:         Stubs
 *
 * PROGRAMMERS:     Hervé Poussineau (hpoussin@reactos.org)
 */

#include "newdev.h"

BOOL WINAPI
InstallNewDevice(
	IN HWND hwndParent,
	IN LPGUID ClassGuid OPTIONAL,
	OUT PDWORD Reboot)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_GEN_FAILURE);
	return FALSE;
}

BOOL WINAPI
InstallSelectedDriverW(
	IN HWND hwndParent,
	IN HDEVINFO DeviceInfoSet,
	IN LPCWSTR Reserved,
	IN BOOL Backup,
	OUT PDWORD pReboot)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_GEN_FAILURE);
	return FALSE;
}

BOOL WINAPI
UpdateDriverForPlugAndPlayDevicesA(
	IN HWND hwndParent,
	IN LPCSTR HardwareId,
	IN LPCSTR FullInfPath,
	IN DWORD InstallFlags,
	OUT PBOOL bRebootRequired OPTIONAL)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_GEN_FAILURE);
	return FALSE;
}

BOOL WINAPI
UpdateDriverForPlugAndPlayDevicesW(
	IN HWND hwndParent,
	IN LPCWSTR HardwareId,
	IN LPCWSTR FullInfPath,
	IN DWORD InstallFlags,
	OUT PBOOL bRebootRequired OPTIONAL)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_GEN_FAILURE);
	return FALSE;
}

