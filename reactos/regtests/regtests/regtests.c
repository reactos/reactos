/*
 * PROJECT:         ReactOS kernel
 * FILE:            regtests/regtests/regtests.c
 * PURPOSE:         Regression testing framework
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      23-10-2004  CSH  Created
 */
#include <windows.h>

HMODULE STDCALL
_GetModuleHandleA(LPCSTR lpModuleName)
{
  return GetModuleHandleA(lpModuleName);
}

FARPROC STDCALL
_GetProcAddress(HMODULE hModule,
  LPCSTR lpProcName)
{
  return GetProcAddress(hModule, lpProcName);
}

HINSTANCE STDCALL
_LoadLibraryA(LPCSTR lpLibFileName)
{
  return LoadLibraryA(lpLibFileName);
}

VOID STDCALL
_ExitProcess(UINT uExitCode)
{
  ExitProcess(uExitCode);
}

HANDLE STDCALL
_CreateThread(LPSECURITY_ATTRIBUTES lpThreadAttributes, DWORD dwStackSize,
              LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter,
              DWORD dwCreationFlags, LPDWORD lpThreadId)
{
  return CreateThread(lpThreadAttributes, dwStackSize, lpStartAddress,
                      lpParameter, dwCreationFlags, lpThreadId);
}

WINBOOL STDCALL
_TerminateThread(HANDLE hThread, DWORD dwExitCode)
{
  return TerminateThread(hThread, dwExitCode);
}

DWORD STDCALL
_WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds)
{
  return WaitForSingleObject(hHandle, dwMilliseconds);
}

DWORD STDCALL
_GetLastError()
{
  return GetLastError();
}

VOID STDCALL
_CloseHandle(HANDLE handle)
{
  CloseHandle (handle);
}

BOOL STDCALL
_GetThreadTimes(HANDLE hThread, LPFILETIME lpCreationTime,
	            LPFILETIME lpExitTime, LPFILETIME lpKernelTime,
	            LPFILETIME lpUserTime)
{
  return GetThreadTimes(hThread, lpCreationTime, lpExitTime,
                        lpKernelTime, lpUserTime);
}

BOOL STDCALL
_SetPriorityClass(HANDLE hProcess, DWORD dwPriorityClass)
{
  return SetPriorityClass(hProcess, dwPriorityClass);
}

BOOL STDCALL
_SetThreadPriority(HANDLE hThread, int nPriority)
{
  return SetThreadPriority(hThread, nPriority);
}

HANDLE STDCALL
_GetCurrentProcess()
{
  return GetCurrentProcess();
}

HANDLE STDCALL
_GetCurrentThread()
{
  return GetCurrentThread();
}

VOID STDCALL
_Sleep(DWORD dwMilliseconds)
{
  return Sleep(dwMilliseconds);
}
