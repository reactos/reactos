/* $Id$ */
#include "precomp.h"

#define NDEBUG
#include <debug.h>


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
