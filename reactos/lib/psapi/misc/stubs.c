/* $Id: stubs.c,v 1.4 2002/08/31 15:36:56 hyperion Exp $ */
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

BOOL STDCALL GetWsChanges(
  HANDLE hProcess,                         // handle to process
  PPSAPI_WS_WATCH_INFORMATION lpWatchInfo, // buffer
  DWORD cb                                 // size of buffer
)
{
 SetLastError(ERROR_INVALID_FUNCTION);
 return FALSE;
}

BOOL STDCALL InitializeProcessForWsWatch(
  HANDLE hProcess  // handle to process
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
