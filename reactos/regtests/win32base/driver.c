/*
 * PROJECT:         ReactOS kernel
 * FILE:            regtests/win32base/driver.c
 * PURPOSE:         Win32 base services regression testing driver
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      06-07-2003  CSH  Created
 */
#define NTOS_MODE_USER
#include <ntos.h>
#include "regtests.h"

PVOID
AllocateMemory(ULONG Size)
{
  return (PVOID) RtlAllocateHeap(RtlGetProcessHeap(), 0, Size);
}


VOID
FreeMemory(PVOID Base)
{
  RtlFreeHeap(RtlGetProcessHeap(), 0, Base);
}


static DWORD WINAPI
DummyThreadMain(LPVOID lpParameter)
{
	return 0;
}


VOID
RunPrivateTests(LPTSTR FileName)
{
  HMODULE hModule;
  HANDLE hEvent;

  hEvent = CreateEventA(
    NULL,
    FALSE,
	FALSE,
    "WinRegTests");
  if (hEvent == NULL)
    {
	  return;
    }

  hModule = GetModuleHandle(FileName);
  if (hModule != NULL) 
    {
      HANDLE hThread;

      /*
       * The module is a core OS component that is already
       * mapped into the current process.
	     * NOTE: This will cause all core OS components that are already mapped
	     * into the process to run their regression tests.
       */
      hThread = CreateThread(NULL, 0, DummyThreadMain, NULL, 0, NULL);
      if (hThread != NULL)
        {
          DWORD ErrorCode;
		      ErrorCode = WaitForSingleObject(hEvent, 5000); /* Wait up to 5 seconds */
	        CloseHandle(hThread);
        }
	}
  else
	{
      hModule = LoadLibrary(FileName);
      if (hModule != NULL) 
        { 
          CloseHandle(hEvent);
          FreeLibrary(hModule); 
        }
    }

  CloseHandle(hEvent);
}


VOID STDCALL
RegTestMain(TestOutputRoutine OutputRoutine, LPSTR TestName)
{
  /*
   * Private module regression tests in components already mapped
   * (ntdll.dll, kernel32.dll, msvcrt.dll)
   */
  /* FIXME: Need to pass TestName to the driver */
  RunPrivateTests(_T("ntdll.dll"));

  /* Other private module regression tests */

  /* Cross-module regression tests */
  InitializeTests();
  RegisterTests();
  PerformTests(OutputRoutine, TestName);
}
