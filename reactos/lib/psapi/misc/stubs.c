/* $Id: stubs.c,v 1.6 2004/11/02 23:42:49 weiden Exp $ */
#include "precomp.h"

#if 0
/*
 * @unimplemented
 */
BOOL
STDCALL
EnumPageFiles(PENUM_PAGE_CALLBACKW pCallbackRoutine,
              LPVOID lpContext)
{
  SetLastError(ERROR_INVALID_FUNCTION);
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
GetPerformanceInfo(PPERFORMANCE_INFORMATION pPerformanceInformation,
                   DWORD cb)
{
  SetLastError(ERROR_INVALID_FUNCTION);
  return FALSE;
}
#endif


/*
 * @unimplemented
 */
BOOL
STDCALL
GetProcessMemoryInfo(HANDLE Process,
                     PPROCESS_MEMORY_COUNTERS ppsmemCounters,
                     DWORD cb)
{
  SetLastError(ERROR_INVALID_FUNCTION);
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
QueryWorkingSet(HANDLE hProcess,
                PVOID pv,
                DWORD cb)
{
  SetLastError(ERROR_INVALID_FUNCTION);
  return FALSE;
}

/* EOF */
