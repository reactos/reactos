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


VOID STDCALL
RegTestMain()
{
  InitializeTests();
  RegisterTests();
  PerformTests();
}
