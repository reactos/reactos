/* $Id: stubs.c,v 1.5 2004/10/31 01:23:05 weiden Exp $ */
#include <windows.h>
#include <psapi.h>

#if 0
BOOL STDCALL EnumPageFiles(
  PENUM_PAGE_CALLBACKW pCallbackRoutine,
  LPVOID lpContext
)
{
 SetLastError(ERROR_INVALID_FUNCTION);
 return FALSE;
}

BOOL STDCALL GetPerformanceInfo(
  PPERFORMANCE_INFORMATION pPerformanceInformation, 
  DWORD cb 

)
{
 SetLastError(ERROR_INVALID_FUNCTION);
 return FALSE;
}
#endif
BOOL STDCALL GetProcessMemoryInfo(
  HANDLE Process,                          // handle to process
  PPROCESS_MEMORY_COUNTERS ppsmemCounters, // buffer
  DWORD cb                                 // size of buffer
)
{
 SetLastError(ERROR_INVALID_FUNCTION);
 return FALSE;
}

BOOL STDCALL QueryWorkingSet(
  HANDLE hProcess,  // handle to process
  PVOID pv,         // information buffer
  DWORD cb          // size of buffer
)
{
 SetLastError(ERROR_INVALID_FUNCTION);
 return FALSE;
}

/* EOF */
