#define _DECL_DLLMAIN
#include <windows.h>
#include <process.h>

BOOL WINAPI DllEntryPoint(HANDLE hDllHandle, DWORD dwReason, LPVOID lpreserved)
{
  return TRUE;
}
