/*
 * PROJECT:         ReactOS kernel
 * FILE:            regtests/regtests/regtests.c
 * PURPOSE:         Regression testing framework
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      23-10-2004  CSH  Created
 */
#include <windows.h>

HMODULE WINAPI
_GetModuleHandleA(LPCSTR lpModuleName)
{
  return GetModuleHandleA(lpModuleName);
}

FARPROC WINAPI
_GetProcAddress(HMODULE hModule,
  LPCSTR lpProcName)
{
  return GetProcAddress(hModule, lpProcName);
}

HINSTANCE WINAPI
_LoadLibraryA(LPCSTR lpLibFileName)
{
  return LoadLibraryA(lpLibFileName);
}

VOID WINAPI
_ExitProcess(UINT uExitCode)
{
  ExitProcess(uExitCode);
}

HANDLE WINAPI
_CreateThread(LPSECURITY_ATTRIBUTES lpThreadAttributes, DWORD dwStackSize,
              LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter,
              DWORD dwCreationFlags, LPDWORD lpThreadId)
{
  return CreateThread(lpThreadAttributes, dwStackSize, lpStartAddress,
                      lpParameter, dwCreationFlags, lpThreadId);
}

WINBOOL WINAPI
_TerminateThread(HANDLE hThread, DWORD dwExitCode)
{
  return TerminateThread(hThread, dwExitCode);
}

DWORD WINAPI
_WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds)
{
  return WaitForSingleObject(hHandle, dwMilliseconds);
}

DWORD WINAPI
_GetLastError()
{
  return GetLastError();
}

VOID WINAPI
_CloseHandle(HANDLE handle)
{
  CloseHandle (handle);
}

BOOL WINAPI
_GetThreadTimes(HANDLE hThread, LPFILETIME lpCreationTime,
	            LPFILETIME lpExitTime, LPFILETIME lpKernelTime,
	            LPFILETIME lpUserTime)
{
  return GetThreadTimes(hThread, lpCreationTime, lpExitTime,
                        lpKernelTime, lpUserTime);
}

BOOL WINAPI
_SetPriorityClass(HANDLE hProcess, DWORD dwPriorityClass)
{
  return SetPriorityClass(hProcess, dwPriorityClass);
}

BOOL WINAPI
_SetThreadPriority(HANDLE hThread, int nPriority)
{
  return SetThreadPriority(hThread, nPriority);
}

HANDLE WINAPI
_GetCurrentProcess()
{
  return GetCurrentProcess();
}

HANDLE WINAPI
_GetCurrentThread()
{
  return GetCurrentThread();
}

VOID WINAPI
_Sleep(DWORD dwMilliseconds)
{
  return Sleep(dwMilliseconds);
}
