/* $Id: stubs.c,v 1.1 2001/11/28 23:35:16 ea Exp $ */
#include <windows.h>
#include <psapi.h>

BOOL STDCALL
EmptyWorkingSet(HANDLE hProcess)
{
	return FALSE;
}

BOOL STDCALL
EnumDeviceDrivers(
    LPVOID *lpImageBase,
    DWORD cb,
    LPDWORD lpcbNeeded)
{
	return FALSE;
}

BOOL STDCALL
EnumPageFiles(
    PENUM_PAGE_CALLBACKW pCallbackRoutine,
    LPVOID lpContext)
{
	return FALSE;
}

BOOL STDCALL EnumProcesses(
  DWORD *lpidProcess,
  DWORD cb,
  DWORD *cbNeeded)
{
	return FALSE;
}

BOOL STDCALL
EnumProcessModules(
  HANDLE hProcess,
  HMODULE *lphModule,
  DWORD cb,
  LPDWORD lpcbNeeded)
{
	return FALSE;
}

DWORD STDCALL
GetDeviceDriverBaseName(
  LPVOID ImageBase,
  LPTSTR lpBaseName,
  DWORD nSize)
{
	return 0;
}

DWORD STDCALL GetDeviceDriverFileName(
  LPVOID ImageBase,
  LPTSTR lpFilename,
  DWORD nSize)
{
	return 0;
}

DWORD STDCALL
GetMappedFileName(
  HANDLE hProcess,
  LPVOID lpv,
  LPTSTR lpFilename,
  DWORD nSize)
{
	return 0;
}

DWORD STDCALL
GetModuleBaseName(
  HANDLE hProcess,
  HMODULE hModule,
  LPTSTR lpBaseName,
  DWORD nSize)
{
	return 0;
}

DWORD STDCALL
GetModuleFileNameEx(
  HANDLE hProcess,
  HMODULE hModule,
  LPTSTR lpFilename,
  DWORD nSize)
{
	return 0;
}

BOOL STDCALL
GetModuleInformation(
  HANDLE hProcess,
  HMODULE hModule,
  LPMODULEINFO lpmodinfo,
  DWORD cb)
{
	return FALSE;
}

BOOL STDCALL
GetPerformanceInfo(
  PPERFORMANCE_INFORMATION pPerformanceInformation, 
  DWORD cb)
{
	return FALSE;
}

BOOL STDCALL
GetProcessMemoryInfo(
  HANDLE Process,
  PPROCESS_MEMORY_COUNTERS ppsmemCounters,
  DWORD cb)
{
	return FALSE;
}

BOOL STDCALL
GetWsChanges(
  HANDLE hProcess,
  PPSAPI_WS_WATCH_INFORMATION lpWatchInfo,
  DWORD cb)
{
	return FALSE;
}

BOOL STDCALL
InitializeProcessForWsWatch(
  HANDLE hProcess)
{
	return FALSE;
}

BOOL STDCALL
QueryWorkingSet(
  HANDLE hProcess,
  PVOID pv,
  DWORD cb)
{
	return FALSE;
}

/* EOF */
