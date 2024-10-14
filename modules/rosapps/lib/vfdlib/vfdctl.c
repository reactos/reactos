/*
	vfdctl.c

	Virtual Floppy Drive for Windows
	Driver control library
	Driver and image control functions

	Copyright (C) 2003-2005 Ken Kato
*/

#ifdef __cplusplus
#pragma message(__FILE__": Compiled as C++ for testing purpose.")
#endif	// __cplusplus

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dbt.h>
#if !defined(__REACTOS__) || defined(_MSC_VER)
#pragma warning (push, 3)
#endif
#include <shlobj.h>
#include <winioctl.h>
#if !defined(__REACTOS__) || defined(_MSC_VER)
#pragma warning (pop)
#endif
#include <stdio.h>

#include "vfdtypes.h"
#include "vfdio.h"
#include "vfdapi.h"
#include "vfdlib.h"
#include "vfdver.h"

#ifndef IOCTL_DISK_GET_LENGTH_INFO
//	Old winioctl.h header doesn't define the following

#define IOCTL_DISK_GET_LENGTH_INFO			CTL_CODE(\
IOCTL_DISK_BASE, 0x0017, METHOD_BUFFERED, FILE_READ_ACCESS)

typedef struct _GET_LENGTH_INFORMATION {
	LARGE_INTEGER	Length;
} GET_LENGTH_INFORMATION, *PGET_LENGTH_INFORMATION;

#endif	// IOCTL_DISK_GET_LENGTH_INFO

//
//	DOS device name (\\.\VirtualFD)
//
#ifndef __REACTOS__
#define VFD_DEVICE_TEMPLATE		"\\\\.\\" VFD_DEVICE_BASENAME "%u"
#else
#define VFD_DEVICE_TEMPLATE		"\\\\.\\" VFD_DEVICE_BASENAME "%lu"
#endif
#define VFD_VOLUME_TEMPLATE		"\\\\.\\%c:"

#define VFD_INSTALL_DIRECTORY	"\\system32\\drivers\\"

#ifdef _DEBUG
#ifndef __REACTOS__
extern ULONG TraceFlags = (ULONG)-1;//0;
extern CHAR *TraceFile	= NULL;
extern ULONG TraceLine	= 0;
#else
ULONG TraceFlags = (ULONG)-1;//0;
CHAR const * TraceFile	= NULL;
ULONG TraceLine	= 0;
#endif
#endif

//
//	broadcast a WM_DEVICECHANGE system message to inform
//	a drive letter creation / removal
//
#define VFD_LINK_CREATED	0
#define VFD_LINK_REMOVED	1

static void VfdBroadcastLink(
	CHAR			cLetter,
	BOOL			bRemoved)
{
	DWORD			receipients;
	DWORD			device_event;
	DEV_BROADCAST_VOLUME params;

	if (!isalpha(cLetter)) {
		VFDTRACE(0,
			("VfdBroadcastLink: invalid parameter"))
		return;
	}

	receipients = BSM_APPLICATIONS;

	device_event = bRemoved ?
		DBT_DEVICEREMOVECOMPLETE : DBT_DEVICEARRIVAL;

	ZeroMemory(&params, sizeof(params));

	params.dbcv_size		= sizeof(params);
	params.dbcv_devicetype	= DBT_DEVTYP_VOLUME;
	params.dbcv_reserved	= 0;
	params.dbcv_unitmask	= (1 << (toupper(cLetter) - 'A'));
	params.dbcv_flags		= 0;

	if (BroadcastSystemMessage(
		BSF_NOHANG | BSF_FORCEIFHUNG | BSF_NOTIMEOUTIFNOTHUNG,
		&receipients,
		WM_DEVICECHANGE,
		device_event,
		(LPARAM)&params) <= 0) {

		VFDTRACE(0,
			("VfdBroadcastLink: BroadcastSystemMessage - %s",
			SystemMessage(GetLastError())));
	}
}

//
//	Broadcast a VFD notify message
//
static __inline void VfdNotify(
	WPARAM			wParam,
	LPARAM			lParam)
{
	//	SendNotifyMessage causes volume locking conflict (I think)
	//	on Windows XP while closing an image with VfdWin
//	SendNotifyMessage(HWND_BROADCAST, uVfdMsg, wParam, lParam);
	PostMessage(HWND_BROADCAST, g_nNotifyMsg, wParam, lParam);
}

#ifdef VFD_EMBED_DRIVER
//
//	Restore the VFD driver file in the system directory
//

static DWORD VfdRestoreDriver(
	PCSTR			sPath)
{
#define FUNC		"VfdRestoreDriver"
	HRSRC			hRes;
	DWORD			size;
	HGLOBAL			hDrv;
	PVOID			pData;
	DWORD			result;
	HANDLE			hFile;
	DWORD			ret;

	//
	//	Prepare driver binary
	//

	// use embedded driver binary

#define S(s) #s
	hRes = FindResource(g_hDllModule,
		S(VFD_DRIVER_NAME_ID), S(VFD_DRIVER_TYPE_ID));
#undef S

	if (hRes == NULL) {
		ret = GetLastError();

		VFDTRACE(0,
			(FUNC ": FindResource - %s",
			SystemMessage(ret)));

		return ret;
	}

	size = SizeofResource(g_hDllModule, hRes);

	if (size == 0) {
		ret = GetLastError();

		VFDTRACE(0,
			(FUNC ": SizeofResource - %s",
			SystemMessage(ret)));

		return ret;
	}

	hDrv = LoadResource(g_hDllModule, hRes);

	if (hDrv == NULL) {
		ret = GetLastError();

		VFDTRACE(0,
			(FUNC ": LoadResource - %s",
			SystemMessage(ret)));

		return ret;
	}

	pData = LockResource(hDrv);

	if (pData == NULL) {
		ret = GetLastError();

		VFDTRACE(0,
			(FUNC ": LockResource - %s",
			SystemMessage(ret)));

		return ret;
	}

	//	create the driver file

	hFile = CreateFile(sPath, GENERIC_WRITE,
		0, NULL, OPEN_ALWAYS, 0, NULL);

	if (hFile == INVALID_HANDLE_VALUE) {
		ret = GetLastError();

		VFDTRACE(0,
			(FUNC ": CreateFile(%s) - %s",
			sPath, SystemMessage(ret)));

		return ret;
	}

	if (!WriteFile(hFile, pData, size, &result, NULL) ||
		size != result) {
		ret = GetLastError();

		VFDTRACE(0,
			(FUNC ": CreateFile - %s",
			SystemMessage(ret)));

		CloseHandle(hFile);
		return ret;
	}

	SetEndOfFile(hFile);
	CloseHandle(hFile);

	return ERROR_SUCCESS;
}
#endif	// VFD_EMBED_DRIVER

//
//	Install the Virtual Floppy Driver
//
DWORD WINAPI VfdInstallDriver(
	PCSTR			sFileName,
	DWORD			nStart)
{
#undef	FUNC
#define FUNC		"VfdInstallDriver"
	SC_HANDLE		hScManager;				// Service Control Manager
	SC_HANDLE		hService = NULL;		// Service (= Driver)
#ifndef VFD_EMBED_DRIVER
	CHAR			file_path[MAX_PATH];
	PSTR			file_name;
#endif	//	VFD_EMBED_DRIVER
	CHAR			system_dir[MAX_PATH];
	PSTR			inst_path;
	DWORD			len;
	DWORD			ret = ERROR_SUCCESS;

#ifdef __REACTOS__
	CHAR			full_file_path[MAX_PATH];
#endif

	//	get SystemRoot directory path

//	len = GetEnvironmentVariable(
//		"SystemRoot", system_dir, sizeof(system_dir));
	len = GetWindowsDirectory(system_dir, sizeof(system_dir));

	if (len == 0 || len > sizeof(system_dir)) {
		VFDTRACE(0,
			(FUNC ": %%SystemRoot%% is empty or too long.\n"));

		return ERROR_BAD_ENVIRONMENT;
	}

	inst_path = &system_dir[len];

#ifdef __REACTOS__
	strcpy(full_file_path, system_dir);
	strcat(full_file_path, VFD_INSTALL_DIRECTORY);
	strcat(full_file_path, VFD_DRIVER_FILENAME);
#endif

#ifdef VFD_EMBED_DRIVER
	//
	//	use embedded driver file
	//
	strcpy(inst_path++, VFD_INSTALL_DIRECTORY VFD_DRIVER_FILENAME);

	ret = VfdRestoreDriver(system_dir);

	if (ret != ERROR_SUCCESS) {
		return ret;
	}

#else	// VFD_EMBED_DRIVER
	//	Prepare driver binary's full path

	if (sFileName == NULL || *sFileName == '\0') {

		// default driver file is vfd.sys in the same directory as executable

		len = GetModuleFileName(
			NULL, file_path, sizeof(file_path));

		if (len == 0) {
			ret = GetLastError();

			VFDTRACE(0,
				(FUNC ": GetModuleFileName - %s",
				SystemMessage(ret)));

			return ret;
		}

		//	search the last '\' character

		while (len > 0 && file_path[len - 1] != '\\') {
			len --;
		}

		//	supply the file name (vfd.sys)

		file_name = &file_path[len];
		strcpy(file_name, VFD_DRIVER_FILENAME);
	}
	else {

		//	ensure that tha path is an absolute full path

		len = GetFullPathName(
			sFileName,
			sizeof(file_path),
			file_path,
			&file_name);

		if (len == 0) {
			ret = GetLastError();

			VFDTRACE(0,
				(FUNC ": GetFullPathName(%s) - %s\n",
				sFileName, SystemMessage(ret)));

			return ret;
		}

		if (GetFileAttributes(file_path) & FILE_ATTRIBUTE_DIRECTORY) {
			//	if the specified path is a directory,
			//	supply the file name (vfd.sys)

			file_name = &file_path[len];
			strcpy(file_name++, "\\" VFD_DRIVER_FILENAME);
		}
	}

#ifdef __REACTOS__
	//      Check install directory & file exist or use full_file_path

	if (GetFileAttributesA(file_path) == INVALID_FILE_ATTRIBUTES) {
		strcpy(file_path, full_file_path);
	}
#endif

	//	Check if the file is a valid Virtual Floppy driver

	ret = VfdCheckDriverFile(file_path, NULL);

	if (ret != ERROR_SUCCESS) {
		VFDTRACE(0,
			(FUNC ": VfdCheckDriverFile(%s)\n", file_path));

		return ret;
	}

	//	if the path is under the system directory, make it relative
	//	to the system directory

	len = strlen(system_dir);

	if (!_strnicmp(file_path, system_dir, len)) {
		inst_path = &file_path[len];

		while (*inst_path == '\\') {
			inst_path++;
		}
	}
	else {
		inst_path = &file_path[0];
	}
#endif	//	VFD_EMBED_DRIVER

	//	Connect to the Service Control Manager

	hScManager = OpenSCManager(
		NULL,							// local machine
		NULL,							// local database
		SC_MANAGER_CREATE_SERVICE);		// access required

	if (hScManager == NULL) {
		ret = GetLastError();

		VFDTRACE(0,
			(FUNC ": OpenSCManager() - %s",
			SystemMessage(ret)));

		goto cleanup;
	}

	//	Create a new service object

	hService = CreateService(
		hScManager,						// service control manager
		VFD_DEVICE_BASENAME,			// internal service name
		VFD_DEVICE_BASENAME,			// display name
		SERVICE_ALL_ACCESS,				// access mode
		SERVICE_KERNEL_DRIVER,			// service type
		nStart,							// service start type
		SERVICE_ERROR_NORMAL,			// start error sevirity
		inst_path,						// service image file path
		NULL,							// service group
		NULL,							// service tag
		NULL,							// service dependency
		NULL,							// use LocalSystem account
		NULL							// password for the account
	);

	if (!hService) {
		// Failed to create a service object
		ret = GetLastError();

		VFDTRACE(0,
			(FUNC ": CreateService() - %s",
			SystemMessage(ret)));

		goto cleanup;
	}

cleanup:
	//	Close the service object handle

	if (hService) {
		CloseServiceHandle(hService);
	}

	//	Close handle to the service control manager.

	if (hScManager) {
		CloseServiceHandle(hScManager);
	}

	if (ret == ERROR_SUCCESS) {
		//	Broadcast the successful operation
		VfdNotify(VFD_OPERATION_INSTALL, 0);
	}
#ifdef VFD_EMBED_DRIVER
	else {
		//	Delete the restored driver file
		DeleteFile(system_dir);
	}
#endif	// VFD_EMBED_DRIVER

	return ret;
}

//
//	Configure the Virtual Floppy Driver (change the start method)
//

DWORD WINAPI VfdConfigDriver(
	DWORD			nStart)
{
#undef	FUNC
#define FUNC		"VfdConfigDriver"
	SC_HANDLE		hScManager;				// Service Control Manager
	SC_HANDLE		hService;				// Service (= Driver)
	DWORD			ret = ERROR_SUCCESS;

	//	Connect to the Service Control Manager

	hScManager = OpenSCManager(NULL, NULL, 0);

	if (hScManager == NULL) {
		ret = GetLastError();

		VFDTRACE(0,
			(FUNC ": OpenSCManager() - %s",
			SystemMessage(ret)));

		return ret;
	}

	//	Open the VFD driver entry in the service database

	hService = OpenService(
		hScManager,						// Service control manager
		VFD_DEVICE_BASENAME,			// service name
		SERVICE_CHANGE_CONFIG);			// service access mode

	if (hService == NULL) {
		ret = GetLastError();

		VFDTRACE(0,
			(FUNC ": OpenService(SERVICE_CHANGE_CONFIG) - %s",
			SystemMessage(ret)));

		goto cleanup;
	}

	//	Change the start method of the VFD driver

	if (!ChangeServiceConfig(
			hService,
			SERVICE_NO_CHANGE,
			nStart,
			SERVICE_NO_CHANGE,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL)) {

		ret = GetLastError();

		VFDTRACE(0,
			(FUNC ": ChangeServiceConfig() - %s",
			SystemMessage(ret)));

		goto cleanup;
	}

cleanup:
	//	Close the service object handle

	if (hService) {
		CloseServiceHandle(hService);
	}

	//	Close handle to the service control manager.

	if (hScManager) {
		CloseServiceHandle(hScManager);
	}

	//	Broadcast the successful operation

	if (ret == ERROR_SUCCESS) {
		VfdNotify(VFD_OPERATION_CONFIG, 0);
	}

	return ret;
}

//
//	Remove the Virtual Floppy Driver entry from the service database
//
DWORD WINAPI VfdRemoveDriver()
{
#undef	FUNC
#define FUNC		"VfdRemoveDriver"
	SC_HANDLE		hScManager;				// Service Control Manager
	SC_HANDLE		hService;				// Service (= Driver)
	CHAR			file_path[MAX_PATH];
	DWORD			ret = ERROR_SUCCESS;

	//	Get the current driver path

	ret = VfdGetDriverConfig(file_path, NULL);

	if (ret != ERROR_SUCCESS) {
		return ret;
	}

	//	Connect to the Service Control Manager

	hScManager = OpenSCManager(NULL, NULL, 0);

	if (hScManager == NULL) {
		ret = GetLastError();

		VFDTRACE(0,
			(FUNC ": OpenSCManager() - %s",
			SystemMessage(ret)));

		return ret;
	}

	//	Open the VFD driver entry in the service database

	hService = OpenService(
		hScManager,						// Service control manager
		VFD_DEVICE_BASENAME,			// service name
		DELETE);						// service access mode

	if (hService == NULL) {
		ret = GetLastError();

		VFDTRACE(0,
			(FUNC ": OpenService(DELETE) - %s",
			SystemMessage(ret)));

		goto cleanup;
	}

	//	Remove driver entry from registry

	if (!DeleteService(hService)) {
		ret = GetLastError();

		VFDTRACE(0,
			(FUNC ": DeleteService() - %s",
			SystemMessage(ret)));

		goto cleanup;
	}

cleanup:
	//	Close the service object handle

	if (hService) {
		CloseServiceHandle(hService);
	}

	//	Close handle to the service control manager.

	if (hScManager) {
		CloseServiceHandle(hScManager);
	}

	//	Broadcast the successful operation

	if (ret == ERROR_SUCCESS) {
		VfdNotify(VFD_OPERATION_REMOVE, 0);

#ifdef VFD_EMBED_DRIVER
		//	Remove the driver file
		DeleteFile(file_path);
#endif	//	VFD_EMBED_DRIVER
	}

	return ret;
}

//
//	Start the Virtual Floppy Driver
//
DWORD WINAPI VfdStartDriver(
	PDWORD			pState)
{
#undef	FUNC
#define FUNC		"VfdStartDriver"
	SC_HANDLE		hScManager;			// Service Control Manager
	SC_HANDLE		hService;			// Service (= Driver)
	SERVICE_STATUS	stat;
	DWORD			ret = ERROR_SUCCESS;
	HCURSOR			original;
	int				i;

	if (pState) {
		*pState = 0;
	}

	//	Connect to the Service Control Manager

	hScManager = OpenSCManager(NULL, NULL, 0);

	if (hScManager == NULL) {
		ret = GetLastError();

		VFDTRACE(0,
			(FUNC ": OpenSCManager() - %s",
			SystemMessage(ret)));

		return ret;
	}

	//	show an hourglass cursor

	original = SetCursor(LoadCursor(NULL, IDC_WAIT));

	//	Open the VFD driver entry in the service database

	hService = OpenService(
		hScManager,						// Service control manager
		VFD_DEVICE_BASENAME,			// service name
		SERVICE_START
		| SERVICE_QUERY_STATUS);		// service access mode

	if (hService == NULL) {
		ret = GetLastError();

		VFDTRACE(0,
			(FUNC ": OpenService(SERVICE_START) - %s",
			SystemMessage(ret)));

		goto cleanup;
	}

	//	Start the driver

	if (!StartService(hService, 0, NULL)) {
		ret = GetLastError();

		VFDTRACE(0,
			(FUNC ": StartService() - %s",
			SystemMessage(ret)));

		goto cleanup;
	}

	//	Wait until the driver is properly running

	i = 0;

	for (;;) {
		if (!QueryServiceStatus(hService, &stat)) {
			ret = GetLastError();

			VFDTRACE(0,
				(FUNC ": QueryServiceStatus() - %s",
				SystemMessage(ret)));

			break;
		}

		if (stat.dwCurrentState == SERVICE_RUNNING || ++i == 5) {
			break;
		}

		Sleep(1000);
	}

	if (stat.dwCurrentState == SERVICE_RUNNING) {

		//	Broadcast the successful operation

		if (ret == ERROR_SUCCESS) {
			VfdNotify(VFD_OPERATION_START, 0);
		}

		//	broadcast the arrival of VFD drives
		//	otherwise WinXP explorer doesn't recognize the VFD drives

		for (i = 0; i < VFD_MAXIMUM_DEVICES; i++) {
			HANDLE	hDevice;
			CHAR	letter = 0;

			hDevice = VfdOpenDevice(i);

			if (hDevice != INVALID_HANDLE_VALUE) {

				VfdGetGlobalLink(hDevice, &letter);

				CloseHandle(hDevice);

				if (isalpha(letter)) {
					VfdBroadcastLink(letter, VFD_LINK_CREATED);
					VfdNotify(VFD_OPERATION_SETLINK, i);
				}
			}
			else {
				VFDTRACE(0,
					(FUNC ": VfdOpenDevice(%d) - %s",
					i, SystemMessage(GetLastError())));
			}
		}
	}
	else {
		//	somehow failed to start the driver

		ret = ERROR_SERVICE_NOT_ACTIVE;
	}

	if (pState) {
		*pState = stat.dwCurrentState;
	}

cleanup:
	//	Close the service object handle

	if (hService) {
		CloseServiceHandle(hService);
	}

	//	Close handle to the service control manager.

	if (hScManager) {
		CloseServiceHandle(hScManager);
	}

	//	revert to the original cursor

	SetCursor(original);

	return ret;
}

//
//	Stop the Virtual Floppy Driver
//
DWORD WINAPI VfdStopDriver(
	PDWORD			pState)
{
#undef	FUNC
#define FUNC		"VfdStopDriver"
	SC_HANDLE		hScManager;			// Service Control Manager
	SC_HANDLE		hService;			// Service (= Driver)
	SERVICE_STATUS	stat;
	CHAR			drive_letters[VFD_MAXIMUM_DEVICES];
	DWORD			ret = ERROR_SUCCESS;
	int				i;
	HCURSOR			original;

	if (pState) {
		*pState = 0;
	}

	//	Connect to the Service Control Manager

	hScManager = OpenSCManager(NULL, NULL, 0);

	if (hScManager == NULL) {
		ret = GetLastError();

		VFDTRACE(0,
			(FUNC ": OpenSCManager() - %s",
			SystemMessage(ret)));

		return ret;
	}

	//	Show the hourglass cursor

	original = SetCursor(LoadCursor(NULL, IDC_WAIT));

	//	Open the VFD driver entry in the service database

	hService = OpenService(
		hScManager,						// Service control manager
		VFD_DEVICE_BASENAME,			// service name
		SERVICE_STOP
		| SERVICE_QUERY_STATUS);		// service access mode

	if (hService == NULL) {
		ret = GetLastError();

		VFDTRACE(0,
			(FUNC ": OpenService(SERVICE_STOP) - %s",
			SystemMessage(ret)));

		goto cleanup;
	}

	//	Get assigned drive letters

	for (i = 0; i < VFD_MAXIMUM_DEVICES; i++) {
		HANDLE	hDevice;
		CHAR letter;

		hDevice = VfdOpenDevice(i);

		if (hDevice != INVALID_HANDLE_VALUE) {

			//	remove all session local drive letters

			while (VfdGetLocalLink(hDevice, &letter) == ERROR_SUCCESS &&
				isalpha(letter)) {
				VfdSetLocalLink(hDevice, 0);
			}

			//	store existing persistent drive letters

			VfdGetGlobalLink(hDevice, &drive_letters[i]);

			CloseHandle(hDevice);
		}
		else {
			VFDTRACE(0,
				(FUNC ": VfdOpenDevice(%d) - %s",
				i, SystemMessage(GetLastError())));
		}
	}

	//	Stop the driver

	if (!ControlService(hService, SERVICE_CONTROL_STOP, &stat)) {
		ret = GetLastError();

		VFDTRACE(0,
			(FUNC ": ControlService(SERVICE_CONTROL_STOP) - %s",
			SystemMessage(ret)));

		goto cleanup;
	}

	//	Wait until the driver is stopped

	i = 0;

	while (stat.dwCurrentState != SERVICE_STOPPED && ++i < 5) {
		Sleep(1000);

		if (!QueryServiceStatus(hService, &stat)) {
			ret = GetLastError();

			VFDTRACE(0,
				(FUNC ": QueryServiceStatus() - %s",
				SystemMessage(ret)));

			break;
		}
	}

	if (stat.dwCurrentState != SERVICE_RUNNING) {

		//	broadcast the removal of persistent drive letters

		for (i = 0; i < VFD_MAXIMUM_DEVICES; i++) {
			if (isalpha(drive_letters[i])) {
				VfdBroadcastLink(drive_letters[i], VFD_LINK_REMOVED);
				VfdNotify(VFD_OPERATION_DELLINK, i);
			}
		}
	}

	if (pState) {
		*pState = stat.dwCurrentState;
	}

cleanup:
	//	Close the service object handle

	if (hService) {
		CloseServiceHandle(hService);
	}

	//	Close handle to the service control manager.

	if (hScManager) {
		CloseServiceHandle(hScManager);
	}

	//	Broadcast the successful operation

	if (ret == ERROR_SUCCESS) {
		VfdNotify(VFD_OPERATION_STOP, 0);
	}

	//	revert to the original cursor

	SetCursor(original);

	return ret;
}

//
//	Get the Virtual Floppy Driver configuration
//
DWORD WINAPI VfdGetDriverConfig(
	PSTR			sFileName,
	PDWORD			pStart)
{
#undef	FUNC
#define FUNC		"VfdGetDriverConfig"
	SC_HANDLE		hScManager;				// Service Control Manager
	SC_HANDLE		hService;				// Service (= Driver)
	LPQUERY_SERVICE_CONFIG config = NULL;
	DWORD			result;
	DWORD			ret = ERROR_SUCCESS;

	if (sFileName) {
		ZeroMemory(sFileName, MAX_PATH);
	}

	if (pStart) {
		*pStart = 0;
	}

	//	Connect to the Service Control Manager

	hScManager = OpenSCManager(NULL, NULL, 0);

	if (hScManager == NULL) {
		ret = GetLastError();

		VFDTRACE(0,
			(FUNC ": OpenSCManager() - %s", SystemMessage(ret)));

		return ret;
	}

	//	Open the VFD driver entry in the service database

	hService = OpenService(
		hScManager,						// Service control manager
		VFD_DEVICE_BASENAME,			// service name
		SERVICE_QUERY_CONFIG);			// service access mode

	if (hService == NULL) {
		ret = GetLastError();

		VFDTRACE(0,
			(FUNC ": OpenService(SERVICE_QUERY_CONFIG) - %s",
			SystemMessage(ret)));

		goto cleanup;
	}

	//	Get the length of config information

	if (!QueryServiceConfig(hService, NULL, 0, &result)) {
		ret = GetLastError();

		if (ret == ERROR_INSUFFICIENT_BUFFER) {
			ret = ERROR_SUCCESS;
		}
		else {
			VFDTRACE(0,
				(FUNC ": QueryServiceConfig() - %s",
				SystemMessage(ret)));

			goto cleanup;
		}
	}

	//	allocate a required buffer

	config = (LPQUERY_SERVICE_CONFIG)LocalAlloc(LPTR, result);

	if (config == NULL) {
		ret = GetLastError();

		VFDTRACE(0,
			(FUNC ": LocalAlloc(%lu) - %s\n",
			result, SystemMessage(ret)));

		goto cleanup;
	}

	//	get the config information

	if (!QueryServiceConfig(hService, config, result, &result)) {
		ret = GetLastError();

		VFDTRACE(0,
			(FUNC ": QueryServiceConfig() - %s",
			SystemMessage(ret)));

		goto cleanup;
	}

	//	copy information to output buffer

	if (sFileName) {
		if (strncmp(config->lpBinaryPathName, "\\??\\", 4) == 0) {

			//	driver path is an absolute UNC path
			strncpy(
				sFileName,
				config->lpBinaryPathName + 4,
				MAX_PATH);
		}
		else if (config->lpBinaryPathName[0] == '\\' ||
			(isalpha(config->lpBinaryPathName[0]) &&
			config->lpBinaryPathName[1] == ':')) {

			//	driver path is an absolute path
			strncpy(sFileName,
				config->lpBinaryPathName,
				MAX_PATH);
		}
		else {
			//	driver path is relative to the SystemRoot
//			DWORD len = GetEnvironmentVariable(
//				"SystemRoot", sFileName, MAX_PATH);

			DWORD len = GetWindowsDirectory(sFileName, MAX_PATH);

			if (len == 0 || len > MAX_PATH) {
				VFDTRACE(0,
					(FUNC ": %%SystemRoot%% is empty or too long.\n"));

				ret = ERROR_BAD_ENVIRONMENT;
				goto cleanup;
			}

			sprintf((sFileName + len), "\\%s",
				config->lpBinaryPathName);
		}
	}

	if (pStart) {
		*pStart = config->dwStartType;
	}

cleanup:
	//	Free service config buffer

	if (config) {
		LocalFree(config);
	}

	//	Close the service object handle

	if (hService) {
		CloseServiceHandle(hService);
	}

	//	Close handle to the service control manager.

	if (hScManager) {
		CloseServiceHandle(hScManager);
	}

	return ret;
}

//
//	Get the Virtual Floppy Driver running state
//
DWORD WINAPI VfdGetDriverState(
	PDWORD			pState)
{
#undef	FUNC
#define FUNC		"VfdGetDriverState"
	SC_HANDLE		hScManager = NULL;	// Service Control Manager
	SC_HANDLE		hService = NULL;	// Service (= Driver)
	SERVICE_STATUS	status;
	DWORD			ret = ERROR_SUCCESS;

	if (pState) {
		*pState = 0;
	}

	//	Connect to the Service Control Manager

	hScManager = OpenSCManager(NULL, NULL, 0);

	if (hScManager == NULL) {
		ret = GetLastError();

		VFDTRACE(0,
			(FUNC ": OpenSCManager() - %s",
			SystemMessage(ret)));

		return ret;
	}

	//	Open the VFD driver entry in the service database

	hService = OpenService(
		hScManager,						// Service control manager
		VFD_DEVICE_BASENAME,			// service name
		SERVICE_QUERY_STATUS);			// service access mode

	if (hService == NULL) {

		ret = GetLastError();

		if (ret == ERROR_SERVICE_DOES_NOT_EXIST) {

			if (pState) {
				*pState = VFD_NOT_INSTALLED;
			}

			ret = ERROR_SUCCESS;
		}
		else {
			VFDTRACE(0,
				(FUNC ": OpenService(SERVICE_QUERY_STATUS) - %s",
				SystemMessage(ret)));
		}

		goto cleanup;
	}

	//	Get current driver status

	ZeroMemory(&status, sizeof(status));

	if (!QueryServiceStatus(hService, &status)) {
		ret = GetLastError();

		VFDTRACE(0,
			(FUNC ": QueryServiceStatus() - %s",
			SystemMessage(ret)));

		goto cleanup;
	}

	if (pState) {
		*pState = status.dwCurrentState;
	}

cleanup:
	//	Close the service object handle

	if (hService) {
		CloseServiceHandle(hService);
	}

	//	Close handle to the service control manager.

	if (hScManager) {
		CloseServiceHandle(hScManager);
	}

	return ret;
}

//
//	open a Virtual Floppy drive without showing the "Insert Floppy"
//	dialog when the drive is empty.
//
HANDLE WINAPI VfdOpenDevice(
	ULONG			nTarget)		// either a drive letter or a device number
{
#undef	FUNC
#define FUNC		"VfdOpenDevice"
	CHAR			dev_name[20];
	UINT			err_mode;
	HANDLE			hDevice;

	//	format a device name string

	if (isalpha(nTarget)) {
		//	nTarget is a drive letter
		//	\\.\<x>:
#ifndef __REACTOS__
		sprintf(dev_name, VFD_VOLUME_TEMPLATE, nTarget);
#else
		sprintf(dev_name, VFD_VOLUME_TEMPLATE, (CHAR)nTarget);
#endif
	}
	else if (isdigit(nTarget)) {
		//	nTarget is a device number in character
		//	\\.\VirtualFD<n>
		sprintf(dev_name, VFD_DEVICE_TEMPLATE, nTarget - '0');
	}
	else {
		//	nTarget is a device number value
		//	\\.\VirtualFD<n>
		sprintf(dev_name, VFD_DEVICE_TEMPLATE, nTarget);
	}

	// change error mode in order to avoid "Insert Floppy" dialog

	err_mode = SetErrorMode(SEM_FAILCRITICALERRORS);

	//	open the target drive

	hDevice = CreateFile(
		dev_name,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_NO_BUFFERING,
		NULL);

	//	revert to the original error mode

	SetErrorMode(err_mode);

	if (hDevice != INVALID_HANDLE_VALUE) {

		//	check if the target is a valid VFD drive

		ULONG version;

		if (VfdGetDriverVersion(hDevice, &version) != ERROR_SUCCESS) {

			//	Failed to get the driver version

			CloseHandle(hDevice);
			hDevice = INVALID_HANDLE_VALUE;
		}
		else if ((version & ~0x80000000) !=
			MAKELONG(VFD_DRIVER_MINOR, VFD_DRIVER_MAJOR)) {

			//	the driver version mismatch

//			CloseHandle(hDevice);
//			hDevice = INVALID_HANDLE_VALUE;

			SetLastError(ERROR_REVISION_MISMATCH);
		}
	}
	else {
		VFDTRACE(0,(
			"CreateFile(%s) - %s", dev_name,
			SystemMessage(GetLastError())));;
	}

	return hDevice;
}

//
//	Open a Virtual Floppy Image
//
DWORD WINAPI VfdOpenImage(
	HANDLE			hDevice,
	PCSTR			sFileName,
	VFD_DISKTYPE	nDiskType,
	VFD_MEDIA		nMediaType,
	VFD_FLAGS		nMediaFlags)
{
#undef	FUNC
#define FUNC		"VfdOpenImage"
	PCSTR			prefix;
	CHAR			abspath[MAX_PATH];
	DWORD			name_len;
	DWORD			result;
	DWORD			ret = ERROR_SUCCESS;

	PVFD_IMAGE_INFO	image_info = NULL;
	PUCHAR			image_buf = NULL;
	ULONG			image_size;
	VFD_FILETYPE	file_type;

	//
	//	Check parameters
	//

	if (hDevice == NULL ||
		hDevice == INVALID_HANDLE_VALUE) {
		return ERROR_INVALID_HANDLE;
	}

	if (nMediaType == VFD_MEDIA_NONE ||
		nMediaType >= VFD_MEDIA_MAX) {

		VFDTRACE(0,
			(FUNC ": Invalid MediaType - %u\n", nMediaType));

		return ERROR_INVALID_PARAMETER;
	}


	if (sFileName && *sFileName) {

		//	check file contents and attributes

		HANDLE hFile = CreateFile(sFileName, GENERIC_READ,
			FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

		if (hFile == INVALID_HANDLE_VALUE) {
			ret = GetLastError();

			VFDTRACE(0,
				(FUNC ": CreateFile(%s) - %s",
				sFileName, SystemMessage(ret)));

			return ret;
		}

		//	try extracting image data from zip compressed file

		ExtractZipImage(hFile, &image_buf, &image_size);

		if (image_buf) {

			file_type = VFD_FILETYPE_ZIP;

			//	imz file must be opened in RAM mode

			if (nDiskType == VFD_DISKTYPE_FILE) {

				VFDTRACE(0,
					(FUNC ": %s is a zip compressed file",
					sFileName));

				CloseHandle(hFile);
				ret = ERROR_INVALID_PARAMETER;

				goto exit_func;
			}
		}
		else {

			file_type = VFD_FILETYPE_RAW;

			if (nDiskType == VFD_DISKTYPE_FILE) {

				//	direct image file must not be compressed or encrypted

				BY_HANDLE_FILE_INFORMATION info;

				if (!GetFileInformationByHandle(hFile, &info)) {
					ret = GetLastError();

					VFDTRACE(0,
						(FUNC ": GetFileInformationByHandle - %s",
						SystemMessage(ret)));

					CloseHandle(hFile);

					return ret;
				}

				if (info.dwFileAttributes &
					(FILE_ATTRIBUTE_COMPRESSED | FILE_ATTRIBUTE_ENCRYPTED)) {

					VFDTRACE(0,
						(FUNC ": file is compressed/encrypted"));

					CloseHandle(hFile);

					return ERROR_FILE_ENCRYPTED;
				}

				image_size	= info.nFileSizeLow;
			}
			else {

				// prepare image data for a file based RAM disk

				image_size = GetFileSize(hFile, NULL);

				if (image_size == 0 || image_size == INVALID_FILE_SIZE) {
					ret = GetLastError();

					VFDTRACE(0,
						(FUNC ": GetFileSize - %s",
						SystemMessage(ret)));

					CloseHandle(hFile);

					return ret;
				}

				image_buf = (PUCHAR)LocalAlloc(LPTR, image_size);

				if (image_buf == NULL) {
					ret = GetLastError();

					VFDTRACE(0,
						(FUNC ": LocalAlloc - %s",
						SystemMessage(ret)));

					CloseHandle(hFile);

					return ret;
				}

				if (SetFilePointer(hFile, 0, NULL, FILE_BEGIN) != 0) {
					ret = GetLastError();

					VFDTRACE(0,
						(FUNC ": SetFilePointer - %s",
						SystemMessage(ret)));

					CloseHandle(hFile);

					goto exit_func;
				}

				if (!ReadFile(hFile, image_buf, image_size, &result, NULL) ||
					image_size != result) {

					ret = GetLastError();

					VFDTRACE(0,
						(FUNC ": ReadFile - %s",
						SystemMessage(ret)));

					CloseHandle(hFile);

					goto exit_func;
				}
			}
		}

		CloseHandle(hFile);

		//	Prepare absolute path in the kernel namespace

		if (*sFileName == '\\' && *(sFileName + 1) == '\\') {

			// \\server\share\path\floppy.img

			prefix = "\\??\\UNC";
			sFileName++;			// drip the first '\'
		}
		else {

			//	local path

			PSTR file_part;

			if (GetFullPathName(sFileName,
				sizeof(abspath), abspath, &file_part) == 0) {

				ret =  GetLastError();

				VFDTRACE(0,
					(FUNC ": GetFullPathName(%s) - %s\n",
					sFileName, SystemMessage(ret)));

				goto exit_func;
			}

			prefix = "\\??\\";
			sFileName = abspath;
		}

		name_len = strlen(prefix) + strlen(sFileName);
	}
	else {

		//	filename is not specified -- pure RAM disk

		nDiskType = VFD_DISKTYPE_RAM;
		file_type = VFD_FILETYPE_NONE;

		prefix = NULL;
		name_len = 0;

		//	prepare a FAT formatted RAM image

		image_size = VfdGetMediaSize(nMediaType);

		image_buf = (PUCHAR)LocalAlloc(LPTR, image_size);

		if (image_buf == NULL) {
			ret = GetLastError();

			VFDTRACE(0,
				(FUNC ": LocalAlloc - %s",
				SystemMessage(ret)));

			return ret;
		}

		FormatBufferFat(image_buf, VFD_BYTE_TO_SECTOR(image_size));
	}

	if (image_size < VfdGetMediaSize(nMediaType)) {

		//	image is too small for the specified media type

		VFDTRACE(0,
			(FUNC ": Image is too small for the specified media type\n"));

		ret = ERROR_INVALID_PARAMETER;
		goto exit_func;
	}

	//	prepare VFD_IMAGE_INFO structure

	image_info = (PVFD_IMAGE_INFO)LocalAlloc(LPTR,
		sizeof(VFD_IMAGE_INFO) + name_len + 1);

	if (image_info == NULL) {
		ret = GetLastError();

		VFDTRACE(0,
			(FUNC ": LocalAlloc(%lu) - %s\n",
			sizeof(VFD_IMAGE_INFO) + name_len + 1,
			SystemMessage(ret)));

		goto exit_func;
	}

	ZeroMemory(image_info,
		sizeof(VFD_IMAGE_INFO) + name_len + 1);

	if (name_len) {
		sprintf(image_info->FileName,
			"%s%s", prefix, sFileName);
	}

	image_info->NameLength	= (USHORT)name_len;

	image_info->DiskType	= nDiskType;
	image_info->MediaType	= nMediaType;
	image_info->MediaFlags	= nMediaFlags;
	image_info->FileType	= file_type;
	image_info->ImageSize	= image_size;

	if (nDiskType != VFD_DISKTYPE_FILE) {
		//	protect flag for a RAM disk is set after
		//	initializing the image buffer
		image_info->MediaFlags &= ~VFD_FLAG_WRITE_PROTECTED;
	}

	VFDTRACE(0,
		(FUNC ": Opening file \"%s\" (%lu bytes) %s %s %s %s\n",
		name_len ? image_info->FileName : "<RAM>",
		image_info->ImageSize,
		(file_type == VFD_FILETYPE_ZIP) ? "ZIP image" : "RAW image",
		VfdMediaTypeName(nMediaType),
		(nDiskType == VFD_DISKTYPE_FILE) ? "FILE disk" : "RAM disk",
		(nMediaFlags & VFD_FLAG_WRITE_PROTECTED) ? "Protected" : "Writable"));

	//	Open the image file / create a ram disk

	if (!DeviceIoControl(
		hDevice,
		IOCTL_VFD_OPEN_IMAGE,
		image_info,
		sizeof(VFD_IMAGE_INFO) + name_len,
		NULL,
		0,
		&result,
		NULL))
	{
		ret = GetLastError();

		VFDTRACE(0,
			(FUNC ": DeviceIoControl(IOCTL_VFD_OPEN_FILE) - %s",
			SystemMessage(ret)));

		goto exit_func;
	}

	//	initialize the RAM disk image

	if (nDiskType != VFD_DISKTYPE_FILE) {

		image_size &= ~VFD_SECTOR_ALIGN_MASK;

		if (SetFilePointer(hDevice, 0, NULL, FILE_BEGIN) != 0) {
			ret = GetLastError();

			VFDTRACE(0,
				(FUNC ": SetFilePointer - %s",
				SystemMessage(ret)));

			goto exit_func;
		}

		if (!WriteFile(hDevice, image_buf, image_size, &result, NULL) ||
			image_size != result) {

			ret = GetLastError();

			VFDTRACE(0,
				(FUNC ": WriteFile - %s",
				SystemMessage(ret)));

			goto exit_func;
		}

		if (nMediaFlags & VFD_FLAG_WRITE_PROTECTED) {
			VfdWriteProtect(hDevice, TRUE);
		}

		if (!DeviceIoControl(
			hDevice,
			IOCTL_VFD_RESET_MODIFY,
			NULL,
			0,
			NULL,
			0,
			&result,
			NULL))
		{
			VFDTRACE(0,
				(FUNC ": DeviceIoControl(IOCTL_VFD_RESET_MODIFY) - %s",
				SystemMessage(GetLastError())));
		}
	}

	//	Broadcast the successful operation

	if (ret == ERROR_SUCCESS) {
		ULONG	number;
		CHAR	root[] = "A:\\";

		if (VfdGetDeviceNumber(hDevice, &number) == ERROR_SUCCESS) {
			VfdNotify(VFD_OPERATION_OPEN, number);
		}

		VfdGetGlobalLink(hDevice, &root[0]);

		if (isalpha(root[0])) {
			SHChangeNotify(SHCNE_MEDIAINSERTED, SHCNF_PATH, root, NULL);
		}

		while (VfdGetLocalLink(hDevice, &root[0]) == ERROR_SUCCESS &&
			isalpha(root[0])) {
			SHChangeNotify(SHCNE_MEDIAINSERTED, SHCNF_PATH, root, NULL);
		}
	}

exit_func:
	if (image_info) {
		LocalFree(image_info);
	}

	if (image_buf) {
		LocalFree(image_buf);
	}

	return ret;
}

//
//	Close the virtual floppy Image
//
DWORD WINAPI VfdCloseImage(
	HANDLE			hDevice,
	BOOL			bForce)
{
#undef	FUNC
#define FUNC		"VfdCloseImage"
	DWORD			result;
	DWORD			ret = ERROR_SUCCESS;
	int				retry = 0;

lock_retry:
	if (!DeviceIoControl(
		hDevice,
		FSCTL_LOCK_VOLUME,
		NULL,
		0,
		NULL,
		0,
		&result,
		NULL))
	{
		ret = GetLastError();

		VFDTRACE(0,
			(FUNC ": DeviceIoControl(FSCTL_LOCK_VOLUME) - %s",
			SystemMessage(ret)));

		if (ret != ERROR_ACCESS_DENIED || retry == 5) {
			//	error other than access denied or
			//	operation kept failing for 5 seconds
			return ret;
		}

		if (!bForce) {
			//	error is access denied and
			//	the force flag is not set

			if (retry == 0) {

				//	send the MEDIAREMOVED notification to the shell and
				//	see if the shell releases the target drive

				CHAR root[] = "A:\\";

				VfdGetGlobalLink(hDevice, &root[0]);

				if (isalpha(root[0])) {
					SHChangeNotify(SHCNE_MEDIAREMOVED, SHCNF_PATH, root, NULL);
				}

				while (VfdGetLocalLink(hDevice, &root[0]) == ERROR_SUCCESS &&
					isalpha(root[0])) {
					SHChangeNotify(SHCNE_MEDIAREMOVED, SHCNF_PATH, root, NULL);
				}
			}

			Sleep(1000);
			retry++;

			goto lock_retry;
		}
	}

	ret = ERROR_SUCCESS;

	if (!DeviceIoControl(
		hDevice,
		FSCTL_DISMOUNT_VOLUME,
		NULL,
		0,
		NULL,
		0,
		&result,
		NULL))
	{
		ret = GetLastError();

		VFDTRACE(0,
			(FUNC ": DeviceIoControl(FSCTL_DISMOUNT_VOLUME) - %s",
			SystemMessage(ret)));

		return ret;
	}

	if (!DeviceIoControl(
		hDevice,
		IOCTL_VFD_CLOSE_IMAGE,
		NULL,
		0,
		NULL,
		0,
		&result,
		NULL))
	{
		ret = GetLastError();

		if (ret != ERROR_NOT_READY) {
			VFDTRACE(0,
				(FUNC ": DeviceIoControl(IOCTL_VFD_CLOSE_FILE) - %s",
				SystemMessage(ret)));
		}

		return ret;
	}

	if (!DeviceIoControl(
		hDevice,
		FSCTL_UNLOCK_VOLUME,
		NULL,
		0,
		NULL,
		0,
		&result,
		NULL))
	{
		//	This should not be fatal because the volume is unlocked
		//	when the handle is closed anyway
		VFDTRACE(0,
			(FUNC ": DeviceIoControl(FSCTL_UNLOCK_VOLUME) - %s",
			SystemMessage(GetLastError())));
	}

	//	Broadcast the successful operation
	if (ret == ERROR_SUCCESS) {
		ULONG	number;

		if (VfdGetDeviceNumber(hDevice, &number) == ERROR_SUCCESS) {
			VfdNotify(VFD_OPERATION_CLOSE, number);
		}
	}

	return ret;
}

//
//	Get Virtual Floppy image info
//
DWORD WINAPI VfdGetImageInfo(
	HANDLE			hDevice,
	PSTR			sFileName,
	PVFD_DISKTYPE	pDiskType,
	PVFD_MEDIA		pMediaType,
	PVFD_FLAGS		pMediaFlags,
	PVFD_FILETYPE	pFileType,
	PULONG			pImageSize)
{
#undef	FUNC
#define FUNC		"VfdGetImageInfo"
	PVFD_IMAGE_INFO	image_info;
	DWORD			result;
	DWORD			ret = ERROR_SUCCESS;

	image_info = (PVFD_IMAGE_INFO)LocalAlloc(
		LPTR, sizeof(VFD_IMAGE_INFO) + MAX_PATH);

	if (image_info == NULL) {
		ret = GetLastError();

		VFDTRACE(0,
			(FUNC ": LocalAlloc(%lu) - %s\n",
			sizeof(VFD_IMAGE_INFO) + MAX_PATH, SystemMessage(ret)));

		return ret;
	}

	ZeroMemory(image_info, sizeof(VFD_IMAGE_INFO) + MAX_PATH);

	//	Query file information

	if (!DeviceIoControl(
		hDevice,
		IOCTL_VFD_QUERY_IMAGE,
		NULL,
		0,
		image_info,
		sizeof(VFD_IMAGE_INFO) + MAX_PATH,
		&result,
		NULL))
	{
		ret = GetLastError();

		if (ret != ERROR_MORE_DATA) {
			VFDTRACE(0,
				(FUNC ": DeviceIoControl(IOCTL_VFD_QUERY_FILE) - %s",
				SystemMessage(ret)));

			goto cleanup;
		}
	}

	//	copy obtained information to output buffer

	if (sFileName) {

		//	if filename is too long, clip it

		if (image_info->NameLength >= MAX_PATH) {
			image_info->NameLength = MAX_PATH - 1;
		}

		// ensure the name is properly terminated

		image_info->FileName[image_info->NameLength] = '\0';

		if (strncmp(image_info->FileName, "\\??\\UNC", 7) == 0) {
			*sFileName = '\\';
			strcpy(sFileName + 1, image_info->FileName + 7);
		}
		else if (strncmp(image_info->FileName, "\\??\\", 4) == 0) {
			strcpy(sFileName, image_info->FileName + 4);
		}
		else {
			strcpy(sFileName, image_info->FileName);
		}
	}

	if (pDiskType) {
		*pDiskType = image_info->DiskType;
	}

	if (pMediaType) {
		*pMediaType = image_info->MediaType;
	}

	if (pMediaFlags) {
		*pMediaFlags = image_info->MediaFlags;
	}

	if (pFileType) {
		*pFileType = image_info->FileType;
	}

	if (pImageSize) {
		*pImageSize = image_info->ImageSize;
	}

cleanup:
	if (image_info) {
		LocalFree(image_info);
	}

	return ret;
}

//
//	Get current media state (opened / write protected)
//
DWORD WINAPI VfdGetMediaState(
	HANDLE			hDevice)
{
#undef	FUNC
#define FUNC		"VfdGetMediaState"
	DWORD			result;
	DWORD			ret = ERROR_SUCCESS;

	//	Query file information

	if (!DeviceIoControl(
		hDevice,
		IOCTL_DISK_IS_WRITABLE,
		NULL,
		0,
		NULL,
		0,
		&result,
		NULL))
	{
		ret = GetLastError();

		if (ret != ERROR_NOT_READY) {
			VFDTRACE(0,
				(FUNC ": DeviceIoControl(IOCTL_DISK_IS_WRITABLE) - %s",
				SystemMessage(ret)));
		}
	}

	return ret;
}

//
//	Set or Delete a global drive letter
//
DWORD WINAPI VfdSetGlobalLink(
	HANDLE			hDevice,
	CHAR			cLetter)
{
#undef	FUNC
#define FUNC		"VfdSetGlobalLink"
	CHAR			letter;
	ULONG			number;
	DWORD			result;
	DWORD			ret;

	if (isalpha(cLetter)) {

		//	make sure the drive does not have a drive letter

		letter = 0;

		VfdGetGlobalLink(hDevice, &letter);

		if (isalpha(letter)) {
			VFDTRACE(0,
				(FUNC ": Drive already has a drive letter %c\n", letter));
			return ERROR_ALREADY_ASSIGNED;
		}

		VfdGetLocalLink(hDevice, &letter);

		if (isalpha(letter)) {
			VFDTRACE(0,
				(FUNC ": Drive already has a drive letter %c\n", letter));
			return ERROR_ALREADY_ASSIGNED;
		}

		//	make sure drive letter is not in use

		cLetter = (CHAR)toupper(cLetter);

		if (GetLogicalDrives() & (1 << (cLetter - 'A'))) {
			VFDTRACE(0,
				(FUNC ": Drive letter %c already used\n", cLetter));
			return ERROR_ALREADY_ASSIGNED;
		}

		//	Assign a new drive letter

		if (!DeviceIoControl(
			hDevice,
			IOCTL_VFD_SET_LINK,
			&cLetter,
			sizeof(cLetter),
			NULL,
			0,
			&result,
			NULL))
		{
			ret = GetLastError();

			VFDTRACE(0,
				(FUNC ": DeviceIoControl(IOCTL_VFD_SET_LINK) - %s",
				SystemMessage(ret)));

			return ret;
		}

		//	broadcast system message

		VfdBroadcastLink(cLetter, VFD_LINK_CREATED);

		//	broadcast VFD message

		if (VfdGetDeviceNumber(hDevice, &number) == ERROR_SUCCESS) {
			VfdNotify(VFD_OPERATION_SETLINK, number);
		}

		return ERROR_SUCCESS;
	}
	else if (!cLetter) {

		//	make sure the drive has a global drive letter

		letter = 0;

		VfdGetGlobalLink(hDevice, &letter);

		if (!isalpha(letter)) {
			VFDTRACE(0,
				(FUNC ": Drive does not have a drive letter\n"));
			return ERROR_INVALID_FUNCTION;
		}

		//	Remove drive letters

		if (!DeviceIoControl(
			hDevice,
			IOCTL_VFD_SET_LINK,
			&cLetter,
			sizeof(cLetter),
			NULL,
			0,
			&result,
			NULL))
		{
			ret = GetLastError();

			VFDTRACE(0,
				(FUNC ": DeviceIoControl(IOCTL_VFD_SET_LINK) - %s",
				SystemMessage(ret)));

			return ret;
		}

		//	broadcast system message

		VfdBroadcastLink(letter, VFD_LINK_REMOVED);

		//	broadcast VFD message
		if (VfdGetDeviceNumber(hDevice, &number) == ERROR_SUCCESS) {
			VfdNotify(VFD_OPERATION_DELLINK, number);
		}

		return ERROR_SUCCESS;
	}
	else {
		return ERROR_INVALID_PARAMETER;
	}
}

//
//	Get a global drive letter
//
DWORD WINAPI VfdGetGlobalLink(
	HANDLE			hDevice,
	PCHAR			pLetter)
{
#undef	FUNC
#define FUNC		"VfdGetGlobalLinks"
	DWORD			result;
	DWORD			ret;

	if (!pLetter) {
		return ERROR_INVALID_PARAMETER;
	}

	*pLetter = 0;

	if (!DeviceIoControl(
		hDevice,
		IOCTL_VFD_QUERY_LINK,
		NULL,
		0,
		pLetter,
		sizeof(*pLetter),
		&result,
		NULL))
	{
		ret = GetLastError();

		VFDTRACE(0,
			(FUNC ": DeviceIoControl(IOCTL_VFD_QUERY_LINK) - %s",
			SystemMessage(ret)));

		return ret;
	}

	return ERROR_SUCCESS;
}

//
//	Set or remove a local drive letter
//
DWORD WINAPI VfdSetLocalLink(
	HANDLE			hDevice,
	CHAR			cLetter)
{
#undef	FUNC
#define FUNC		"VfdSetLocalLink"
	CHAR			letter;
	CHAR			dos_name[] = "A:";
	CHAR			dev_name[MAX_PATH];
	ULONG			number;
	DWORD			ret;

	if (isalpha(cLetter)) {

		//	make sure the drive does not have a drive letter

		letter = 0;

		VfdGetGlobalLink(hDevice, &letter);

		if (isalpha(letter)) {
			VFDTRACE(0,
				(FUNC ": Drive already has a drive letter %c\n", letter));
			return ERROR_ALREADY_ASSIGNED;
		}

		VfdGetLocalLink(hDevice, &letter);

		if (isalpha(letter)) {
			VFDTRACE(0,
				(FUNC ": Drive already has a drive letter %c\n", letter));
			return ERROR_ALREADY_ASSIGNED;
		}

		//	make sure drive letters are not in use

		cLetter = (CHAR)toupper(cLetter);

		if (GetLogicalDrives() & (1 << (cLetter - 'A'))) {
			VFDTRACE(0,
				(FUNC ": Drive letter already used\n"));

			return ERROR_ALREADY_ASSIGNED;
		}

		//	get VFD device name

		ret = VfdGetDeviceName(hDevice, dev_name, sizeof(dev_name));

		if (ret != ERROR_SUCCESS) {
			return ret;
		}

		//	assign a drive letter

		dos_name[0] = cLetter;

		if (!DefineDosDevice(DDD_RAW_TARGET_PATH, dos_name, dev_name)) {
			ret = GetLastError();

			VFDTRACE(0,
				(FUNC ": DefineDosDevice(%s,%s) - %s",
				dos_name, dev_name, SystemMessage(ret)));
		}

		if (ret == ERROR_SUCCESS) {
			//	broadcast VFD message

			if (VfdGetDeviceNumber(hDevice, &number) == ERROR_SUCCESS) {
				VfdNotify(VFD_OPERATION_SETLINK, number);
			}
		}

		return ret;
	}
	else if (!cLetter) {

		//	make sure the drive has a local drive letter

		letter = 0;

		VfdGetLocalLink(hDevice, &letter);

		if (!isalpha(letter)) {
			VFDTRACE(0,
				(FUNC ": Drive letter is not assigned to this drive\n"));
			return ERROR_INVALID_FUNCTION;
		}

		//	get VFD device name

		ret = VfdGetDeviceName(hDevice, dev_name, sizeof(dev_name));

		if (ret != ERROR_SUCCESS) {
			return ret;
		}

		//	remove drive letters
#define DDD_FLAGS	(DDD_REMOVE_DEFINITION | DDD_RAW_TARGET_PATH | DDD_EXACT_MATCH_ON_REMOVE)

		dos_name[0] = (CHAR)toupper(letter);

		if (!DefineDosDevice(DDD_FLAGS, dos_name, dev_name)) {
			ret = GetLastError();

			VFDTRACE(0,
				(FUNC ": DefineDosDevice(%s,%s) - %s",
				dos_name, dev_name, SystemMessage(ret)));
		}

		if (ret == ERROR_SUCCESS) {
			//	broadcast VFD message
			if (VfdGetDeviceNumber(hDevice, &number) == ERROR_SUCCESS) {
				VfdNotify(VFD_OPERATION_DELLINK, number);
			}
		}

		return ret;
	}
	else {
		return ERROR_INVALID_PARAMETER;
	}
}

//
//	Get local drive letters
//
DWORD WINAPI VfdGetLocalLink(
	HANDLE			hDevice,
	PCHAR			pLetter)
{
#undef	FUNC
#define FUNC		"VfdGetLocalLinks"
	CHAR			global;
	ULONG			logical;
	CHAR			dos_name[] = "A:";
	CHAR			dev_name[MAX_PATH];
	CHAR			dos_target[MAX_PATH * 2];
	DWORD			ret;

	if (!pLetter) {
		return ERROR_INVALID_PARAMETER;
	}

	//	Get the VFD device name

	ret = VfdGetDeviceName(hDevice, dev_name, sizeof(dev_name));

	if (ret != ERROR_SUCCESS) {
		return ret;
	}

	//	Get global drive letter

	ret = VfdGetGlobalLink(hDevice, &global);

	if (ret != ERROR_SUCCESS) {
		return ret;
	}

	//	Get logical drives

	logical = GetLogicalDrives();

	//	exclude the global drive letter

	if (isalpha(global)) {
		logical &= ~(1 << (toupper(global) - 'A'));
	}

	//	start searching from the next drive letter

	if (isalpha(*pLetter)) {
		dos_name[0] = (CHAR)(toupper(*pLetter) + 1);
		logical >>= (dos_name[0] - 'A');
	}

	//	Check dos device targets

	*pLetter = '\0';

	while (logical) {
		if (logical & 0x01) {
			if (QueryDosDevice(dos_name, dos_target, sizeof(dos_target))) {
				if (_stricmp(dos_target, dev_name) == 0) {
					*pLetter = dos_name[0];
					break;
				}
			}
			else {
				VFDTRACE(0,
					(FUNC ": QueryDosDevice(%s) - %s",
					dos_name, SystemMessage(GetLastError())));
			}
		}
		logical >>= 1;
		dos_name[0]++;
	}

	return ERROR_SUCCESS;
}

//
//	Get the Virtual Floppy device number
//
DWORD WINAPI VfdGetDeviceNumber(
	HANDLE			hDevice,
	PULONG			pNumber)
{
#undef	FUNC
#define FUNC		"VfdGetDeviceNumber"
	DWORD			result;
	DWORD			ret = ERROR_SUCCESS;

	if (!pNumber) {
		return ERROR_INVALID_PARAMETER;
	}

	*pNumber = 0;

	if (!DeviceIoControl(
		hDevice,
		IOCTL_VFD_QUERY_NUMBER,
		NULL,
		0,
		pNumber,
		sizeof(ULONG),
		&result,
		NULL))
	{
		ret = GetLastError();

		VFDTRACE(0,
			(FUNC ": DeviceIoControl(IOCTL_VFD_QUERY_NUMBER) - %s",
			SystemMessage(ret)));
	}

	return ret;
}

//	Get the Virtual Floppy device name

DWORD WINAPI VfdGetDeviceName(
	HANDLE			hDevice,
	PCHAR			pName,
	ULONG			nLength)
{
#undef	FUNC
#define FUNC		"VfdGetDeviceName"
	DWORD			result;
	WCHAR			wname[MAX_PATH];
	DWORD			ret = ERROR_SUCCESS;

	if (!pName || !nLength) {
		return ERROR_INVALID_PARAMETER;
	}

	ZeroMemory(pName, nLength);

	if (!DeviceIoControl(
		hDevice,
		IOCTL_VFD_QUERY_NAME,
		NULL,
		0,
		wname,
		sizeof(wname),
		&result,
		NULL))
	{
		ret = GetLastError();

		VFDTRACE(0,
			(FUNC ": DeviceIoControl(IOCTL_VFD_QUERY_NUMBER) - %s",
			SystemMessage(ret)));
	}

	if (!WideCharToMultiByte(CP_OEMCP, 0, &wname[1],
		wname[0] / sizeof(WCHAR), pName, nLength, NULL, NULL)) {

		ret = GetLastError();

		VFDTRACE(0,
			(FUNC ": WideCharToMultiByte - %s",
			SystemMessage(ret)));
	}

	return ret;
}

//
//	Get Virtual Floppy driver version
//
DWORD WINAPI VfdGetDriverVersion(
	HANDLE			hDevice,
	PULONG			pVersion)
{
#undef	FUNC
#define FUNC		"VfdGetDriverVersion"
	DWORD			result;
	DWORD			ret = ERROR_SUCCESS;

	if (!pVersion) {
		return ERROR_INVALID_PARAMETER;
	}

	*pVersion = '\0';

	if (!DeviceIoControl(
		hDevice,
		IOCTL_VFD_QUERY_VERSION,
		NULL,
		0,
		pVersion,
		sizeof(ULONG),
		&result,
		NULL))
	{
		ret = GetLastError();

		VFDTRACE(0,
			(FUNC ": DeviceIoControl(IOCTL_VFD_QUERY_VERSION) - %s",
			SystemMessage(ret)));
	}

	return ret;
}

//
//	Change the write protect state of the media
//
DWORD WINAPI VfdWriteProtect(
	HANDLE			hDevice,
	BOOL			bProtect)
{
#undef	FUNC
#define FUNC		"VfdWriteProtect"
	DWORD			result;
	DWORD			ret = ERROR_SUCCESS;

	if (!DeviceIoControl(
		hDevice,
		bProtect ? IOCTL_VFD_SET_PROTECT : IOCTL_VFD_CLEAR_PROTECT,
		NULL,
		0,
		NULL,
		0,
		&result,
		NULL))
	{
		ret = GetLastError();

		VFDTRACE(0,
			(FUNC ": DeviceIoControl(IOCTL_VFD_SET_PROTECT) - %s",
			SystemMessage(ret)));
	}

	if (ret == ERROR_SUCCESS) {
		ULONG number;

		if (VfdGetDeviceNumber(hDevice, &number) == ERROR_SUCCESS) {
			VfdNotify(VFD_OPERATION_PROTECT, number);
		}
	}

	return ret;
}

//	Format the current media with FAT12

DWORD WINAPI VfdFormatMedia(
	HANDLE			hDevice)
{
#undef	FUNC
#define FUNC		"VfdFormatMedia"
	DWORD			result;
	DWORD			ret = ERROR_SUCCESS;
	PUCHAR			buf = NULL;
	GET_LENGTH_INFORMATION	length;

	//	Get the media size

	if (!DeviceIoControl(
		hDevice,
		IOCTL_DISK_GET_LENGTH_INFO,
		NULL,
		0,
		&length,
		sizeof(length),
		&result,
		NULL))
	{
		ret = GetLastError();

		VFDTRACE(0,
			(FUNC ": DeviceIoControl(IOCTL_DISK_GET_LENGTH_INFO) - %s",
			SystemMessage(ret)));

		goto exit_func;
	}

	//	Prepare a formatted image buffer

	buf = (PUCHAR)LocalAlloc(LPTR, length.Length.LowPart);

	if (buf == NULL) {
		ret = GetLastError();

		VFDTRACE(0,
			(FUNC ": LocalAlloc - %s",
			SystemMessage(ret)));

		goto exit_func;
	}

	//	format the buffer

	ret = FormatBufferFat(buf,
		VFD_BYTE_TO_SECTOR(length.Length.LowPart));

	if (ret != ERROR_SUCCESS) {
		goto exit_func;
	}

	//	seek the top of the media

	if (SetFilePointer(hDevice, 0, NULL, FILE_BEGIN) != 0) {
		ret = GetLastError();

		VFDTRACE(0,
			(FUNC ": SetFilePointer - %s",
			SystemMessage(ret)));

		goto exit_func;
	}

	//	write the image into the media

	if (!WriteFile(hDevice, buf, length.Length.LowPart, &result, NULL) ||
		result != length.Length.LowPart) {
		ret = GetLastError();

		VFDTRACE(0,
			(FUNC ": WriteFile - %s",
			SystemMessage(ret)));

		goto exit_func;
	}

exit_func:
	//	unlock the target volume
	if (!DeviceIoControl(
		hDevice,
		FSCTL_UNLOCK_VOLUME,
		NULL,
		0,
		NULL,
		0,
		&result,
		NULL))
	{
		VFDTRACE(0,
			(FUNC ": DeviceIoControl(FSCTL_UNLOCK_VOLUME) - %s",
			SystemMessage(GetLastError())));
	}

	//	release the format image buffer
	if (buf) {
		LocalFree(buf);
	}

	return ret;
}

//	Dismount the volume (should be called before Save, Format)

DWORD WINAPI VfdDismountVolume(
	HANDLE			hDevice,
	BOOL			bForce)
{
#undef	FUNC
#define FUNC		"VfdDismountVolume"
	DWORD			result;
	DWORD			ret = ERROR_SUCCESS;

	//	Lock the target volume

	if (!DeviceIoControl(
		hDevice,
		FSCTL_LOCK_VOLUME,
		NULL,
		0,
		NULL,
		0,
		&result,
		NULL))
	{
		ret = GetLastError();

		VFDTRACE(0,
			(FUNC ": DeviceIoControl(FSCTL_LOCK_VOLUME) - %s",
			SystemMessage(ret)));

		if (ret != ERROR_ACCESS_DENIED || !bForce) {
			return ret;
		}
	}

	//	Dismount the target volume

	if (!DeviceIoControl(
		hDevice,
		FSCTL_DISMOUNT_VOLUME,
		NULL,
		0,
		NULL,
		0,
		&result,
		NULL))
	{
		ret = GetLastError();

		VFDTRACE(0,
			(FUNC ": DeviceIoControl(FSCTL_DISMOUNT_VOLUME) - %s",
			SystemMessage(ret)));
	}

	return ret;
}

//	Save the current image into a file

DWORD WINAPI VfdSaveImage(
	HANDLE			hDevice,
	PCSTR			sFileName,
	BOOL			bOverWrite,
	BOOL			bTruncate)
{
#undef	FUNC
#define FUNC		"VfdSaveImage"
	HANDLE			hFile = INVALID_HANDLE_VALUE;
	DWORD			result;
	DWORD			ret = ERROR_SUCCESS;
	PUCHAR			buf = NULL;
	GET_LENGTH_INFORMATION	length;


	ret = ERROR_SUCCESS;

	//	Get the media size

	if (!DeviceIoControl(
		hDevice,
		IOCTL_DISK_GET_LENGTH_INFO,
		NULL,
		0,
		&length,
		sizeof(length),
		&result,
		NULL))
	{
		ret = GetLastError();

		VFDTRACE(0,
			(FUNC ": DeviceIoControl(IOCTL_DISK_GET_LENGTH_INFO) - %s",
			SystemMessage(ret)));

		goto exit_func;
	}

	//	Prepare an intermediate image buffer

	buf = (PUCHAR)LocalAlloc(LPTR, length.Length.LowPart);

	if (buf == NULL) {
		ret = GetLastError();

		VFDTRACE(0,
			(FUNC ": LocalAlloc - %s",
			SystemMessage(ret)));

		goto exit_func;
	}

	//	seek the top of the media

	if (SetFilePointer(hDevice, 0, NULL, FILE_BEGIN) != 0) {
		ret = GetLastError();

		VFDTRACE(0,
			(FUNC ": SetFilePointer - %s",
			SystemMessage(ret)));

		goto exit_func;
	}

	//	read the image data

	if (!ReadFile(hDevice, buf, length.Length.LowPart, &result, NULL) ||
		result != length.Length.LowPart) {
		ret = GetLastError();

		VFDTRACE(0,
			(FUNC ": ReadFile - %s",
			SystemMessage(ret)));

		goto exit_func;
	}

	//	open the destination file

	hFile = CreateFile(sFileName, GENERIC_WRITE, 0, NULL,
		bOverWrite ? OPEN_ALWAYS : CREATE_NEW, 0, NULL);

	if (hFile == INVALID_HANDLE_VALUE) {
		ret = GetLastError();

		VFDTRACE(0,
			(FUNC ": CreateFile - %s",
			SystemMessage(ret)));

		goto exit_func;
	}

	//	seek the top of the file

	if (SetFilePointer(hFile, 0, NULL, FILE_BEGIN) != 0) {
		ret = GetLastError();

		VFDTRACE(0,
			(FUNC ": SetFilePointer - %s",
			SystemMessage(ret)));

		goto exit_func;
	}

	//	write the image data

	if (!WriteFile(hFile, buf, length.Length.LowPart, &result, NULL) ||
		result != length.Length.LowPart) {
		ret = GetLastError();

		VFDTRACE(0,
			(FUNC ": WriteFile - %s",
			SystemMessage(ret)));

		goto exit_func;
	}

	//	truncate the target file

	if (bTruncate && !SetEndOfFile(hFile)) {
		ret = GetLastError();

		VFDTRACE(0,
			(FUNC ": SetEndOfFile - %s",
			SystemMessage(ret)));

		goto exit_func;
	}

	//	reset the media modified flag

	if (!DeviceIoControl(
		hDevice,
		IOCTL_VFD_RESET_MODIFY,
		NULL,
		0,
		NULL,
		0,
		&result,
		NULL))
	{
		VFDTRACE(0,
			(FUNC ": DeviceIoControl(IOCTL_VFD_RESET_MODIFY) - %s",
			SystemMessage(GetLastError())));
	}

exit_func:
	//	unlock the target volume

	if (!DeviceIoControl(
		hDevice,
		FSCTL_UNLOCK_VOLUME,
		NULL,
		0,
		NULL,
		0,
		&result,
		NULL))
	{
		VFDTRACE(0,
			(FUNC ": DeviceIoControl(FSCTL_UNLOCK_VOLUME) - %s",
			SystemMessage(GetLastError())));
	}

	//	release the format image buffer

	if (buf) {
		LocalFree(buf);
	}

	//	close the image file

	if (hFile != INVALID_HANDLE_VALUE) {
		CloseHandle(hFile);
	}

	return ret;
}

//
//	Check if specified file is valid VFD driver
//
DWORD WINAPI VfdCheckDriverFile(
	PCSTR			sFileName,
	PULONG			pFileVersion)
{
#undef	FUNC
#define FUNC			"VfdCheckDriverFile"
	DWORD				result;
	DWORD				dummy;
	PVOID				info;
	VS_FIXEDFILEINFO	*fixedinfo;
	DWORD				ret = ERROR_SUCCESS;
	PSTR				str;

	//	Check parameter

	if (!sFileName || !*sFileName) {
		return ERROR_INVALID_PARAMETER;
	}

	if (pFileVersion) {
		*pFileVersion = 0;
	}

	//	check file existence

	if (GetFileAttributes(sFileName) == INVALID_FILE_ATTRIBUTES) {
		ret = GetLastError();

		VFDTRACE(0,
			(FUNC ": GetFileAttributes - %s\n",
			SystemMessage(ret)));

		return ret;
	}

	//	check file version

	result = GetFileVersionInfoSize((PSTR)sFileName, &dummy);

	if (result == 0) {
		VFDTRACE(0,
			(FUNC ": GetFileVersionInfoSize == 0\n"));

		return ERROR_BAD_DRIVER;
	}

	info = LocalAlloc(LPTR, result);

	if (info == NULL) {
		ret = GetLastError();

		VFDTRACE(0,
			(FUNC ": LocalAlloc(%lu) - %s\n",
			result, SystemMessage(ret)));

		return ret;
	}

	if (!GetFileVersionInfo((PSTR)sFileName, 0, result, info)) {
		ret = GetLastError();

		VFDTRACE(0,
			(FUNC ": GetFileVersionInfo - %s", SystemMessage(ret)));

		goto cleanup;
	}

	result = sizeof(fixedinfo);

	if (!VerQueryValue(info, "\\", (PVOID *)&fixedinfo, (PUINT)&result)) {
		ret = GetLastError();

		VFDTRACE(0,
			(FUNC ": VerQueryValue(\"\\\") - %s", SystemMessage(ret)));

		goto cleanup;
	}

	if (fixedinfo->dwFileOS				!= VOS_NT_WINDOWS32 ||
		fixedinfo->dwFileType			!= VFT_DRV			||
		fixedinfo->dwFileSubtype		!= VFT2_DRV_SYSTEM) {

		VFDTRACE(0,
			(FUNC ": Invalid file type flags\n"));

		ret = ERROR_BAD_DRIVER;

		goto cleanup;
	}

	if (pFileVersion) {
		*pFileVersion = fixedinfo->dwFileVersionMS;

		if (fixedinfo->dwFileFlags & VS_FF_DEBUG) {
			*pFileVersion |= 0x80000000;
		}
	}

	if (!VerQueryValue(info,
		"\\StringFileInfo\\" VFD_VERSIONINFO_LANG "\\OriginalFileName",
		(PVOID *)&str, (PUINT)&result)) {
		ret = GetLastError();

		VFDTRACE(0,
			(FUNC ": VerQueryValue(\"OriginalFileName\") - %s",
			SystemMessage(ret)));

		goto cleanup;
	}

	if (strcmp(str, VFD_DRIVER_FILENAME)) {
		VFDTRACE(0,
			(FUNC ": Invalid original file name\n"));

		ret = ERROR_BAD_DRIVER;

		goto cleanup;
	}

	if (fixedinfo->dwFileVersionMS		!= MAKELONG(VFD_DRIVER_MINOR, VFD_DRIVER_MAJOR) ||
		fixedinfo->dwProductVersionMS	!= MAKELONG(VFD_PRODUCT_MINOR, VFD_PRODUCT_MAJOR)) {

		VFDTRACE(0,
			(FUNC ": Invalid version values - file:%08x, prod: %08x\n",
			fixedinfo->dwFileVersionMS, fixedinfo->dwProductVersionMS));

		ret = ERROR_BAD_DRIVER;

		goto cleanup;
	}

	//	Ensure that the driver binary is located on a local drive
	//	because device driver cannot be started on network drives.

	if (*sFileName == '\\' && *(sFileName + 1) == '\\') {
		//	full path is a UNC path -- \\server\dir\...

		VFDTRACE(0,
			(FUNC ": Driver is located on a network drive\n"));

		return ERROR_NETWORK_ACCESS_DENIED;
	}
	else {
		//	ensure that the drive letter is not a network drive

		CHAR root[] = " :\\";

		root[0] = *sFileName;

		if (GetDriveType(root) == DRIVE_REMOTE) {
			// the drive is a network drive

			VFDTRACE(0,
				(FUNC ": Driver is located on a network drive\n"));

			return ERROR_NETWORK_ACCESS_DENIED;
		}
	}

cleanup:
	LocalFree(info);

	return ret;
}

//
//	check an image file
//
DWORD WINAPI VfdCheckImageFile(
	PCSTR			sFileName,
	PDWORD			pAttributes,
	PVFD_FILETYPE	pFileType,
	PULONG			pImageSize)
{
#undef	FUNC
#define FUNC		"VfdCheckImageFile"
	HANDLE			hFile;
	DWORD			ret = ERROR_SUCCESS;

	if (!sFileName || !*sFileName || !pAttributes || !pImageSize || !pFileType) {
		return ERROR_INVALID_PARAMETER;
	}

	//	get file attributes

	*pAttributes = GetFileAttributes(sFileName);

	if (*pAttributes == INVALID_FILE_ATTRIBUTES) {
		ret = GetLastError();

		if (ret != ERROR_FILE_NOT_FOUND) {
			VFDTRACE(0,
				(FUNC ": GetFileAttributes(%s) - %s\n",
				sFileName, SystemMessage(ret)));
		}

		return ret;
	}

	//	Open the target file

	hFile = CreateFile(sFileName, GENERIC_READ | GENERIC_WRITE,
		0, NULL, OPEN_EXISTING, 0, NULL);

	if (hFile == INVALID_HANDLE_VALUE) {

		//	failed to open

		ret = GetLastError();

		if (ret != ERROR_ACCESS_DENIED) {
			VFDTRACE(0,
				(FUNC ": CreateFile(%s) - %s\n",
				sFileName, SystemMessage(ret)));

			return ret;
		}

		// try opening it read-only

		hFile = CreateFile(sFileName, GENERIC_READ,
			FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

		if (hFile == INVALID_HANDLE_VALUE) {

			// cannot open even read-only

			ret = GetLastError();

			VFDTRACE(0,
				(FUNC ": CreateFile(%s) - %s\n",
				sFileName, SystemMessage(ret)));

			return ret;
		}

		// file can be opened read-only
		*pAttributes |= FILE_ATTRIBUTE_READONLY;
		ret = ERROR_SUCCESS;
	}

	//	check if the image is an IMZ file

	if (ExtractZipInfo(hFile, pImageSize) == ERROR_SUCCESS) {
		*pFileType = VFD_FILETYPE_ZIP;
	}
	else {
		*pImageSize = GetFileSize(hFile, NULL);
		*pFileType	= VFD_FILETYPE_RAW;
	}

	CloseHandle(hFile);

	return ret;
}

//
//	Create a formatted new image file
//
DWORD WINAPI VfdCreateImageFile(
	PCSTR			sFileName,
	VFD_MEDIA		nMediaType,
	VFD_FILETYPE	nFileType,
	BOOL			bOverWrite)
{
#undef	FUNC
#define FUNC		"VfdCreateImageFile"
	HANDLE			hFile;
	ULONG			file_size;
	PUCHAR			image_buf = NULL;
	DWORD			result;
	DWORD			ret = ERROR_SUCCESS;

	if (nFileType != VFD_FILETYPE_RAW) {
		return ERROR_INVALID_PARAMETER;
	}

	file_size = VfdGetMediaSize(nMediaType);

	if (file_size == 0) {
		return ERROR_INVALID_PARAMETER;
	}

	hFile = CreateFile(sFileName, GENERIC_WRITE, 0, NULL,
		bOverWrite ? CREATE_ALWAYS : CREATE_NEW, 0, NULL);

	if (hFile == INVALID_HANDLE_VALUE) {
		ret = GetLastError();

		VFDTRACE(0,
			(FUNC ": CreateFile - %s",
			SystemMessage(ret)));

		return ret;
	}

	image_buf = (PUCHAR)LocalAlloc(LPTR, file_size);

	if (image_buf == NULL) {
		ret = GetLastError();

		VFDTRACE(0,
			(FUNC ": LocalAlloc - %s",
			SystemMessage(ret)));

		goto exit_func;
	}

	FormatBufferFat(image_buf, VFD_BYTE_TO_SECTOR(file_size));

	if (!WriteFile(hFile, image_buf, file_size, &result, NULL) ||
		file_size != result) {

		ret = GetLastError();

		VFDTRACE(0,
			(FUNC ": WriteFile - %s",
			SystemMessage(ret)));

		goto exit_func;
	}

	SetEndOfFile(hFile);

exit_func:
	CloseHandle(hFile);

	if (image_buf) {
		LocalFree(image_buf);
	}

	return ret;
}


//
// choose first available drive letter
//
CHAR WINAPI VfdChooseLetter()
{
	DWORD	logical_drives = GetLogicalDrives();
	CHAR	drive_letter = 'A';

	if (logical_drives == 0) {
		return '\0';
	}

	while (logical_drives & 0x1) {
		logical_drives >>= 1;
		drive_letter++;
	}

	if (drive_letter > 'Z') {
		return '\0';
	}

	return drive_letter;
}

//
//	media type functions
//
static const struct
{
	ULONG	Size;
	PCSTR	Name;
}
media_tbl[VFD_MEDIA_MAX] =
{
	{ 0,						"" },					//	VFD_MEDIA_NONE,
	{ VFD_SECTOR_TO_BYTE(320),	"5.25\" 160KB" },		//	VFD_MEDIA_F5_160
	{ VFD_SECTOR_TO_BYTE(360),	"5.25\" 180KB" },		//	VFD_MEDIA_F5_180
	{ VFD_SECTOR_TO_BYTE(640),	"5.25\" 320KB" },		//	VFD_MEDIA_F5_320
	{ VFD_SECTOR_TO_BYTE(720),	"5.25\" 360KB" },		//	VFD_MEDIA_F5_360
	{ VFD_SECTOR_TO_BYTE(1280),	"3.5\"  640KB" },		//	VFD_MEDIA_F3_640
	{ VFD_SECTOR_TO_BYTE(1280),	"5.25\" 640KB" },		//	VFD_MEDIA_F5_640
	{ VFD_SECTOR_TO_BYTE(1440),	"3.5\"  720KB" },		//	VFD_MEDIA_F3_720
	{ VFD_SECTOR_TO_BYTE(1440),	"5.25\" 720KB" },		//	VFD_MEDIA_F5_720
	{ VFD_SECTOR_TO_BYTE(1640),	"3.5\"  820KB" },		//	VFD_MEDIA_F3_820
	{ VFD_SECTOR_TO_BYTE(2400),	"3.5\"	1.2MB" },		//	VFD_MEDIA_F3_1P2
	{ VFD_SECTOR_TO_BYTE(2400),	"5.25\" 1.2MB" },		//	VFD_MEDIA_F5_1P2
	{ VFD_SECTOR_TO_BYTE(2880),	"3.5\"  1.44MB" },		//	VFD_MEDIA_F3_1P4
	{ VFD_SECTOR_TO_BYTE(3360),	"3.5\"  1.68MB DMF" },	//	VFD_MEDIA_F3_1P6
	{ VFD_SECTOR_TO_BYTE(3444),	"3.5\"  1.72MB DMF" },	//	VFD_MEDIA_F3_1P7
	{ VFD_SECTOR_TO_BYTE(5760),	"3.5\"  2.88MB"}		//	VFD_MEDIA_F3_2P8
};

//	Lookup the largest media to fit in a size

VFD_MEDIA WINAPI VfdLookupMedia(
	ULONG			nSize)
{
	VFD_MEDIA i;

	for (i = 1; i < VFD_MEDIA_MAX; i++) {
		if (nSize < media_tbl[i].Size) {
			break;
		}
	}

	return (--i);
}

//	Get media size (in bytes) of a media type

ULONG WINAPI VfdGetMediaSize(
	VFD_MEDIA		nMediaType)
{
	return nMediaType < VFD_MEDIA_MAX ? media_tbl[nMediaType].Size : 0;
}

//	Get media type name

PCSTR WINAPI VfdMediaTypeName(
	VFD_MEDIA		nMediaType)
{
	return nMediaType < VFD_MEDIA_MAX ? media_tbl[nMediaType].Name : NULL;
}
