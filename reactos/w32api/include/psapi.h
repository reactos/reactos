/*
	psapi.h - Include file for PSAPI.DLL APIs

	Written by Mumit Khan <khan@nanotech.wisc.edu>

	This file is part of a free library for the Win32 API.

	NOTE: This strictly does not belong in the Win32 API since it's
	really part of Platform SDK. However,GDB needs it and we might
	as well provide it here.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

*/
#ifndef _PSAPI_H
#define _PSAPI_H
#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef RC_INVOKED

typedef struct _MODULEINFO {
	LPVOID lpBaseOfDll;
	DWORD SizeOfImage;
	LPVOID EntryPoint;
} MODULEINFO,*LPMODULEINFO;

typedef struct _PSAPI_WS_WATCH_INFORMATION {
	LPVOID FaultingPc;
	LPVOID FaultingVa;
} PSAPI_WS_WATCH_INFORMATION,*PPSAPI_WS_WATCH_INFORMATION;

typedef struct _PROCESS_MEMORY_COUNTERS {
	DWORD cb;
	DWORD PageFaultCount;
	DWORD PeakWorkingSetSize;
	DWORD WorkingSetSize;
	DWORD QuotaPeakPagedPoolUsage;
	DWORD QuotaPagedPoolUsage;
	DWORD QuotaPeakNonPagedPoolUsage;
	DWORD QuotaNonPagedPoolUsage;
	DWORD PagefileUsage;
	DWORD PeakPagefileUsage;
} PROCESS_MEMORY_COUNTERS,*PPROCESS_MEMORY_COUNTERS;

/* Grouped by application,not in alphabetical order. */
BOOL WINAPI EnumProcesses(DWORD *,DWORD,DWORD *);
BOOL WINAPI EnumProcessModules(HANDLE,HMODULE *,DWORD,LPDWORD);
DWORD WINAPI GetModuleBaseNameA(HANDLE,HMODULE,LPSTR,DWORD);
DWORD WINAPI GetModuleBaseNameW(HANDLE,HMODULE,LPWSTR,DWORD);
DWORD WINAPI GetModuleFileNameExA(HANDLE,HMODULE,LPSTR,DWORD);
DWORD WINAPI GetModuleFileNameExW(HANDLE,HMODULE,LPWSTR,DWORD);
BOOL WINAPI GetModuleInformation(HANDLE,HMODULE,LPMODULEINFO,DWORD);
BOOL WINAPI EmptyWorkingSet(HANDLE);
BOOL WINAPI QueryWorkingSet(HANDLE,PVOID,DWORD);
BOOL WINAPI InitializeProcessForWsWatch(HANDLE);
BOOL WINAPI GetWsChanges(HANDLE,PPSAPI_WS_WATCH_INFORMATION,DWORD);
DWORD WINAPI GetMappedFileNameW(HANDLE,LPVOID,LPWSTR,DWORD);
DWORD WINAPI GetMappedFileNameA(HANDLE,LPVOID,LPSTR,DWORD);
BOOL WINAPI EnumDeviceDrivers(LPVOID *,DWORD,LPDWORD);
DWORD WINAPI GetDeviceDriverBaseNameA(LPVOID,LPSTR,DWORD);
DWORD WINAPI GetDeviceDriverBaseNameW(LPVOID,LPWSTR,DWORD);
DWORD WINAPI GetDeviceDriverFileNameA(LPVOID,LPSTR,DWORD);
DWORD WINAPI GetDeviceDriverFileNameW(LPVOID,LPWSTR,DWORD);
BOOL WINAPI GetProcessMemoryInfo(HANDLE,PPROCESS_MEMORY_COUNTERS,DWORD);

#endif /* not RC_INVOKED */

#ifdef UNICODE
#define GetModuleBaseName GetModuleBaseNameW
#define GetModuleFileNameEx GetModuleFileNameExW
#define GetMappedFilenameEx GetMappedFilenameExW
#define GetDeviceDriverBaseName GetDeviceDriverBaseNameW
#define GetDeviceDriverFileName GetDeviceDriverFileNameW
#else
#define GetModuleBaseName GetModuleBaseNameA
#define GetModuleFileNameEx GetModuleFileNameExA
#define GetMappedFilenameEx GetMappedFilenameExA
#define GetDeviceDriverBaseName GetDeviceDriverBaseNameA
#define GetDeviceDriverFileName GetDeviceDriverFileNameA
#endif

#ifdef __cplusplus
}
#endif

#endif /* _PSAPI_H */

