#include <windows.h>
#define NTOS_MODE_USER
#include <ntos.h>
#include "regtests.h"

static PVOID
GetFunction(LPSTR FileName,
  LPSTR FunctionName)
{
  HMODULE hModule;
  PVOID Function;

  hModule = GetModuleHandleA(FileName);
  if (hModule != NULL) 
    {
      Function = GetProcAddress(hModule, FunctionName);
    }
  else
	  {
      hModule = LoadLibraryA(FileName);
      if (hModule != NULL)
        {
          Function = GetProcAddress(hModule, FunctionName);
          //FreeLibrary(hModule);
        }
    }
  return Function;
}

typedef PVOID STDCALL (*RTL_ALLOCATE_HEAP)(PVOID a1, ULONG a2, ULONG a3);

PVOID STDCALL
RtlAllocateHeap(PVOID a1,
  ULONG a2,
  ULONG a3)
{
  RTL_ALLOCATE_HEAP p;
  p = GetFunction("ntdll.dll", "RtlAllocateHeap");
  return p(a1, a2, a3);
}

BOOLEAN STDCALL
RtlFreeHeap(
  HANDLE heap,
  ULONG flags,
  PVOID ptr)
{
  return TRUE;
}
