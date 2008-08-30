#define _DECL_DLLMAIN
#define _WINDOWS_
#include <windows.h>
#include <process.h>

BOOL WINAPI DllMain(HANDLE hDllHandle, DWORD dwReason, LPVOID lpreserved)
{
  return TRUE;
}
