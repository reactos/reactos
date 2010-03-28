/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 */

#include <oscalls.h>
#define _DECL_DLLMAIN
#include <process.h>

BOOL WINAPI DllEntryPoint (HANDLE, DWORD, LPVOID);

BOOL WINAPI DllEntryPoint (HANDLE hDllHandle __attribute__ ((__unused__)),
                           DWORD dwReason __attribute__ ((__unused__)),
			   LPVOID lpreserved __attribute__ ((__unused__)))
{
  return TRUE;
}
