/* $Id: stubs.c,v 1.11 2004/11/06 11:45:47 weiden Exp $ */
#include "precomp.h"

#define NDEBUG
#include <debug.h>


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
