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
