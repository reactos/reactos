/* $Id: stubs.c,v 1.10 2004/11/06 01:42:04 weiden Exp $ */
#include "precomp.h"

#define NDEBUG
#include <debug.h>


/*
 * @unimplemented
 */
BOOL
STDCALL
GetPerformanceInfo(PPERFORMANCE_INFORMATION pPerformanceInformation,
                   DWORD cb)
{
  DPRINT1("PSAPI: GetPerformanceInfo is UNIMPLEMENTED!\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
GetProcessMemoryInfo(HANDLE Process,
                     PPROCESS_MEMORY_COUNTERS ppsmemCounters,
                     DWORD cb)
{
  DPRINT1("PSAPI: GetProcessMemoryInfo is UNIMPLEMENTED!\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
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
  DPRINT1("PSAPI: QueryWorkingSet is UNIMPLEMENTED!\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/* EOF */
