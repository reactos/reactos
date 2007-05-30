/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Configuration of networkd devices
 * FILE:            lib/netcfgx/netcfgx.c
 * PURPOSE:         Network devices installer
 *
 * PROGRAMMERS:     Hervé Poussineau (hpoussin@reactos.org)
 */

#define NDEBUG
#include <debug.h>

#include "netcfgx.h"

/* Append a REG_SZ to an existing REG_MULTI_SZ string in the registry.
 * If the value doesn't exist, create it.
 * Returns ERROR_SUCCESS if success. Othewise, returns an error code
 */
static LONG
AppendStringToMultiSZ(
	IN HKEY hKey,
	IN PCWSTR ValueName,
	IN PCWSTR ValueToAppend)
{
	PWSTR Buffer = NULL;
	DWORD dwRegType;
	DWORD dwRequired, dwLength;
	DWORD dwTmp;
	LONG rc;

	rc = RegQueryValueExW(
		hKey,
		ValueName,
		NULL,
		&dwRegType,
		NULL,
		&dwRequired);
	if (rc != ERROR_FILE_NOT_FOUND)
	{
		if (rc != ERROR_SUCCESS)
			goto cleanup;
		if (dwRegType != REG_MULTI_SZ)
		{
			rc = ERROR_GEN_FAILURE;
			goto cleanup;
		}

		dwTmp = dwLength = dwRequired + wcslen(ValueToAppend) * sizeof(WCHAR) + sizeof(UNICODE_NULL);
		Buffer = HeapAlloc(GetProcessHeap(), 0, dwLength);
		if (!Buffer)
		{
			rc = ERROR_NOT_ENOUGH_MEMORY;
			goto cleanup;
		}
		rc = RegQueryValueExW(
			hKey,
			ValueName,
			NULL,
			NULL,
			(BYTE*)Buffer,
			&dwTmp);
		if (rc != ERROR_SUCCESS)
			goto cleanup;
	}
	else
	{
		dwRequired = sizeof(WCHAR);
		dwLength = wcslen(ValueToAppend) * sizeof(WCHAR) + 2 * sizeof(UNICODE_NULL);
		Buffer = HeapAlloc(GetProcessHeap(), 0, dwLength);
		if (!Buffer)
		{
			rc = ERROR_NOT_ENOUGH_MEMORY;
			goto cleanup;
		}
	}

	/* Append the value */
	wcscpy(&Buffer[dwRequired / sizeof(WCHAR) - 1], ValueToAppend);
	/* Terminate the REG_MULTI_SZ string */
	Buffer[dwLength / sizeof(WCHAR) - 1] = UNICODE_NULL;

	rc = RegSetValueExW(
		hKey,
		ValueName,
		0,
		REG_MULTI_SZ,
		(const BYTE*)Buffer,
		dwLength);

cleanup:
	HeapFree(GetProcessHeap(), 0, Buffer);
	return rc;
}

/* Install a section of a .inf file
 * Returns TRUE if success, FALSE if failure. Error code can
 * be retrieved with GetLastError()
 */
static BOOL
InstallInfSection(
	IN HWND hWnd,
	IN LPCWSTR InfFile,
	IN LPCWSTR InfSection)
{
	WCHAR Buffer[MAX_PATH];
	HINF hInf = INVALID_HANDLE_VALUE;
	UINT BufferSize;
	PVOID Context = NULL;
	BOOL ret = FALSE;

	/* Get Windows directory */
	BufferSize = MAX_PATH - 5 - wcslen(InfFile);
	if (GetWindowsDirectoryW(Buffer, BufferSize) > BufferSize)
	{
		/* Function failed */
		SetLastError(ERROR_GEN_FAILURE);
		goto cleanup;
	}
	/* We have enough space to add some information in the buffer */
	if (Buffer[wcslen(Buffer) - 1] != '\\')
		wcscat(Buffer, L"\\");
	wcscat(Buffer, L"Inf\\");
	wcscat(Buffer, InfFile);

	/* Install specified section */
	hInf = SetupOpenInfFileW(Buffer, NULL, INF_STYLE_WIN4, NULL);
	if (hInf == INVALID_HANDLE_VALUE)
		goto cleanup;

	Context = SetupInitDefaultQueueCallback(hWnd);
	if (Context == NULL)
		goto cleanup;

	ret = SetupInstallFromInfSectionW(
		hWnd, hInf,
		InfSection, SPINST_ALL,
		NULL, NULL, SP_COPY_NEWER,
		SetupDefaultQueueCallbackW, Context,
		NULL, NULL);

cleanup:
	if (Context)
		SetupTermDefaultQueueCallback(Context);
	if (hInf != INVALID_HANDLE_VALUE)
		SetupCloseInfFile(hInf);
	return ret;
}

/* Add default services for network cards */
static DWORD
InstallAdditionalServices(
	IN HWND hWnd)
{
	BOOL ret;

	/* Install TCP/IP protocol */
	ret = InstallInfSection(
		hWnd,
		L"nettcpip.inf",
		L"MS_TCPIP.PrimaryInstall");
	if (!ret && GetLastError() != ERROR_FILE_NOT_FOUND)
	{
		DPRINT("InstallInfSection() failed with error 0x%lx\n", GetLastError());
		return GetLastError();
	}

	/* You can add here more clients (SMB...) and services (DHCP server...) */

	return ERROR_SUCCESS;
}

DWORD WINAPI
NetClassInstaller(
	IN DI_FUNCTION InstallFunction,
	IN HDEVINFO DeviceInfoSet,
	IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL)
{
	RPC_STATUS RpcStatus;
	UUID Uuid;
	LPWSTR InstanceId = NULL;
	LPWSTR UuidRpcString = NULL;
	LPWSTR UuidString = NULL;
	LPWSTR DeviceName = NULL;
	LPWSTR ExportName = NULL;
	LONG rc;
	DWORD dwShowIcon, dwLength;
	HKEY hKey = NULL;
	HKEY hLinkageKey = NULL;
	HKEY hNetworkKey = NULL;
	HKEY hConnectionKey = NULL;
	SP_DEVINSTALL_PARAMS_W installParams;
	
	if (InstallFunction != DIF_INSTALLDEVICE)
		return ERROR_DI_DO_DEFAULT;

	DPRINT("%lu %p %p\n", InstallFunction, DeviceInfoSet, DeviceInfoData);

	/* Get Instance ID */
	if (SetupDiGetDeviceInstanceIdW(DeviceInfoSet, DeviceInfoData, NULL, 0, &dwLength))
	{
		DPRINT("SetupDiGetDeviceInstanceIdW() returned TRUE. FALSE expected\n");
		rc = ERROR_GEN_FAILURE;
		goto cleanup;
	}
	InstanceId = HeapAlloc(GetProcessHeap(), 0, dwLength);
	if (!InstanceId)
	{
		DPRINT("HeapAlloc() failed\n");
		rc = ERROR_NOT_ENOUGH_MEMORY;
		goto cleanup;
	}
	if (!SetupDiGetDeviceInstanceIdW(DeviceInfoSet, DeviceInfoData, InstanceId, dwLength, NULL))
	{
		rc = GetLastError();
		DPRINT("SetupDiGetDeviceInstanceIdW() failed with error 0x%lx\n", rc);
		goto cleanup;
	}

	/* Create a new UUID */
	RpcStatus = UuidCreate(&Uuid);
	if (RpcStatus != RPC_S_OK && RpcStatus != RPC_S_UUID_LOCAL_ONLY)
	{
		DPRINT("UuidCreate() failed with RPC status 0x%lx\n", RpcStatus);
		rc = ERROR_GEN_FAILURE;
		goto cleanup;
	}
	RpcStatus = UuidToStringW(&Uuid, &UuidRpcString);
	if (RpcStatus != RPC_S_OK)
	{
		DPRINT("UuidToStringW() failed with RPC status 0x%lx\n", RpcStatus);
		rc = ERROR_GEN_FAILURE;
		goto cleanup;
	}

	/* Add curly braces around Uuid */
	UuidString = HeapAlloc(GetProcessHeap(), 0, (2 + wcslen(UuidRpcString)) * sizeof(WCHAR) + sizeof(UNICODE_NULL));
	if (!UuidString)
	{
		DPRINT("HeapAlloc() failed\n");
		rc = ERROR_NOT_ENOUGH_MEMORY;
		goto cleanup;
	}
	wcscpy(UuidString, L"{");
	wcscat(UuidString, UuidRpcString);
	wcscat(UuidString, L"}");

	/* Create device name */
	DeviceName = HeapAlloc(GetProcessHeap(), 0, (wcslen(L"\\Device\\") + wcslen(UuidString)) * sizeof(WCHAR) + sizeof(UNICODE_NULL));
	if (!DeviceName)
	{
		DPRINT("HeapAlloc() failed\n");
		rc = ERROR_NOT_ENOUGH_MEMORY;
		goto cleanup;
	}
	wcscpy(DeviceName, L"\\Device\\");
	wcscat(DeviceName, UuidString);
	
	/* Create export name */
	ExportName = HeapAlloc(GetProcessHeap(), 0, (wcslen(L"\\Device\\Tcpip_") + wcslen(UuidString)) * sizeof(WCHAR) + sizeof(UNICODE_NULL));
	if (!ExportName)
	{
		DPRINT("HeapAlloc() failed\n");
		rc = ERROR_NOT_ENOUGH_MEMORY;
		goto cleanup;
	}
	wcscpy(ExportName, L"\\Device\\Tcpip_");
	wcscat(ExportName, UuidString);

	/* Write Tcpip parameters in new service Key */
	rc = RegCreateKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services", 0, NULL, REG_OPTION_NON_VOLATILE, 0, NULL, &hKey, NULL);
	if (rc != ERROR_SUCCESS)
	{
		DPRINT("RegCreateKeyExW() failed with error 0x%lx\n", rc);
		goto cleanup;
	}
	rc = RegCreateKeyExW(hKey, UuidString, 0, NULL, REG_OPTION_NON_VOLATILE, 0, NULL, &hNetworkKey, NULL);
	if (rc != ERROR_SUCCESS)
	{
		DPRINT("RegCreateKeyExW() failed with error 0x%lx\n", rc);
		goto cleanup;
	}
	RegCloseKey(hKey);
	hKey = NULL;
	rc = RegCreateKeyExW(hNetworkKey, L"Parameters\\Tcpip", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, NULL);
	if (rc != ERROR_SUCCESS)
	{
		DPRINT("RegCreateKeyExW() failed with error 0x%lx\n", rc);
		goto cleanup;
	}
	RegCloseKey(hNetworkKey);
	hNetworkKey = NULL;
	rc = RegSetValueExW(hKey, L"DefaultGateway", 0, REG_SZ, (const BYTE*)L"0.0.0.0", (wcslen(L"0.0.0.0") + 1) * sizeof(WCHAR));
	if (rc != ERROR_SUCCESS)
	{
		DPRINT("RegSetValueExW() failed with error 0x%lx\n", rc);
		goto cleanup;
	}
	rc = RegSetValueExW(hKey, L"IPAddress", 0, REG_SZ, (const BYTE*)L"0.0.0.0", (wcslen(L"0.0.0.0") + 1) * sizeof(WCHAR));
	if (rc != ERROR_SUCCESS)
	{
		DPRINT("RegSetValueExW() failed with error 0x%lx\n", rc);
		goto cleanup;
	}
	rc = RegSetValueExW(hKey, L"SubnetMask", 0, REG_SZ, (const BYTE*)L"0.0.0.0", (wcslen(L"0.0.0.0") + 1) * sizeof(WCHAR));
	if (rc != ERROR_SUCCESS)
	{
		DPRINT("RegSetValueExW() failed with error 0x%lx\n", rc);
		goto cleanup;
	}
	RegCloseKey(hKey);
	hKey = NULL;

	/* Write 'Linkage' key in hardware key */
#if _WIN32_WINNT >= 0x502
	hKey = SetupDiOpenDevRegKey(DeviceInfoSet, DeviceInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DRV, KEY_READ | KEY_WRITE);
#else
	hKey = SetupDiOpenDevRegKey(DeviceInfoSet, DeviceInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DRV, KEY_ALL_ACCESS);
#endif
	if (hKey == INVALID_HANDLE_VALUE && GetLastError() == ERROR_FILE_NOT_FOUND)
		hKey = SetupDiCreateDevRegKeyW(DeviceInfoSet, DeviceInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DRV, NULL, NULL);
	if (hKey == INVALID_HANDLE_VALUE)
	{
		hKey = NULL;
		rc = GetLastError();
		DPRINT("SetupDiCreateDevRegKeyW() failed with error 0x%lx\n", rc);
		goto cleanup;
	}
	rc = RegSetValueExW(hKey, L"NetCfgInstanceId", 0, REG_SZ, (const BYTE*)UuidString, (wcslen(UuidString) + 1) * sizeof(WCHAR));
	if (rc != ERROR_SUCCESS)
	{
		DPRINT("RegSetValueExW() failed with error 0x%lx\n", rc);
		goto cleanup;
	}
	rc = RegCreateKeyExW(hKey, L"Linkage", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hLinkageKey, NULL);
	if (rc != ERROR_SUCCESS)
	{
		DPRINT("RegCreateKeyExW() failed with error 0x%lx\n", rc);
		goto cleanup;
	}
	rc = RegSetValueExW(hLinkageKey, L"Export", 0, REG_SZ, (const BYTE*)DeviceName, (wcslen(DeviceName) + 1) * sizeof(WCHAR));
	if (rc != ERROR_SUCCESS)
	{
		DPRINT("RegSetValueExW() failed with error 0x%lx\n", rc);
		goto cleanup;
	}
	rc = RegSetValueExW(hLinkageKey, L"RootDevice", 0, REG_SZ, (const BYTE*)UuidString, (wcslen(UuidString) + 1) * sizeof(WCHAR));
	if (rc != ERROR_SUCCESS)
	{
		DPRINT("RegSetValueExW() failed with error 0x%lx\n", rc);
		goto cleanup;
	}
	rc = RegSetValueExW(hLinkageKey, L"UpperBind", 0, REG_SZ, (const BYTE*)L"Tcpip", (wcslen(L"Tcpip") + 1) * sizeof(WCHAR));
	if (rc != ERROR_SUCCESS)
	{
		DPRINT("RegSetValueExW() failed with error 0x%lx\n", rc);
		goto cleanup;
	}
	RegCloseKey(hKey);
	hKey = NULL;

	/* Write connection information in network subkey */
	rc = RegCreateKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Network\\{4D36E972-E325-11CE-BFC1-08002BE10318}", 0, NULL, REG_OPTION_NON_VOLATILE, 0, NULL, &hNetworkKey, NULL);
	if (rc != ERROR_SUCCESS)
	{
		DPRINT("RegCreateKeyExW() failed with error 0x%lx\n", rc);
		goto cleanup;
	}
	rc = RegCreateKeyExW(hNetworkKey, UuidString, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_CREATE_SUB_KEY, NULL, &hKey, NULL);
	if (rc != ERROR_SUCCESS)
	{
		DPRINT("RegCreateKeyExW() failed with error 0x%lx\n", rc);
		goto cleanup;
	}
	rc = RegCreateKeyExW(hKey, L"Connection", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hConnectionKey, NULL);
	RegCloseKey(hKey);
	hKey = NULL;
	if (rc != ERROR_SUCCESS)
	{
		DPRINT("RegCreateKeyExW() failed with error 0x%lx\n", rc);
		goto cleanup;
	}
	rc = RegSetValueExW(hConnectionKey, L"Name", 0, REG_SZ, (const BYTE*)L"Network connection", (wcslen(L"Network connection") + 1) * sizeof(WCHAR));
	if (rc != ERROR_SUCCESS)
	{
		DPRINT("RegSetValueExW() failed with error 0x%lx\n", rc);
		goto cleanup;
	}
	rc = RegSetValueExW(hConnectionKey, L"PnpInstanceId", 0, REG_SZ, (const BYTE*)InstanceId, (wcslen(InstanceId) + 1) * sizeof(WCHAR));
	if (rc != ERROR_SUCCESS)
	{
		DPRINT("RegSetValueExW() failed with error 0x%lx\n", rc);
		goto cleanup;
	}
	dwShowIcon = 1;
	rc = RegSetValueExW(hConnectionKey, L"ShowIcon", 0, REG_DWORD, (const BYTE*)&dwShowIcon, sizeof(dwShowIcon));
	if (rc != ERROR_SUCCESS)
	{
		DPRINT("RegSetValueExW() failed with error 0x%lx\n", rc);
		goto cleanup;
	}

	/* Write linkage information in Tcpip service */
	rc = RegCreateKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Linkage", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_QUERY_VALUE | KEY_SET_VALUE, NULL, &hKey, NULL);
	if (rc != ERROR_SUCCESS)
	{
		DPRINT("RegCreateKeyExW() failed with error 0x%lx\n", rc);
		goto cleanup;
	}
	rc = AppendStringToMultiSZ(hKey, L"Bind", DeviceName);
	if (rc != ERROR_SUCCESS)
	{
		DPRINT("AppendStringToMultiSZ() failed with error 0x%lx\n", rc);
		goto cleanup;
	}
	rc = AppendStringToMultiSZ(hKey, L"Export", ExportName);
	if (rc != ERROR_SUCCESS)
	{
		DPRINT("AppendStringToMultiSZ() failed with error 0x%lx\n", rc);
		goto cleanup;
	}
	rc = AppendStringToMultiSZ(hKey, L"Route", UuidString);
	if (rc != ERROR_SUCCESS)
	{
		DPRINT("AppendStringToMultiSZ() failed with error 0x%lx\n", rc);
		goto cleanup;
	}

	/* Install additionnal services */
	/* FIXME: do it only if it is a network adapter! */
	rc = InstallAdditionalServices(NULL);
	if (rc != ERROR_SUCCESS)
	{
		DPRINT("InstallAdditionalServices() failed with error 0x%lx\n", rc);
		goto cleanup;
	}

	/* HACK: hpoussin, Dec 2005. TCP/IP driver is not able to manage devices
	 * which are installed after its startup. So, we have to reboot to take
	 * this new netcard into account.
	 */
	/* Should we reboot? */
	installParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS_W);
	if (!SetupDiGetDeviceInstallParamsW(
		DeviceInfoSet,
		DeviceInfoData,
		&installParams))
	{
		rc = GetLastError();
		DPRINT("SetupDiGetDeviceInstallParams() failed with error 0x%lx\n", rc);
		goto cleanup;
	}
	installParams.Flags |= DI_NEEDRESTART;
	if (!SetupDiSetDeviceInstallParamsW(
		DeviceInfoSet,
		DeviceInfoData,
		&installParams))
	{
		rc = GetLastError();
		DPRINT("SetupDiSetDeviceInstallParams() failed with error 0x%lx\n", rc);
		goto cleanup;
	}
	rc = ERROR_SUCCESS;

cleanup:
	if (UuidRpcString != NULL)
		RpcStringFreeW(&UuidRpcString);
	HeapFree(GetProcessHeap(), 0, InstanceId);
	HeapFree(GetProcessHeap(), 0, UuidString);
	HeapFree(GetProcessHeap(), 0, DeviceName);
	HeapFree(GetProcessHeap(), 0, ExportName);
	if (hKey != NULL)
		RegCloseKey(hKey);
	if (hLinkageKey != NULL)
		RegCloseKey(hLinkageKey);
	if (hNetworkKey != NULL)
		RegCloseKey(hNetworkKey);
	if (hConnectionKey != NULL)
		RegCloseKey(hConnectionKey);

	if (rc == ERROR_SUCCESS)
		rc = ERROR_DI_DO_DEFAULT;
	DPRINT("Returning 0x%lx\n", rc);
	return rc;
}
