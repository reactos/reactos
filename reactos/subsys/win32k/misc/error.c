/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Errors
 * FILE:             subsys/win32k/misc/error.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISION HISTORY:
 *       06-06-2001  CSH  Created
 */
#define NTOS_KERNEL_MODE
#include <ntos.h>
#include <include/error.h>


VOID
SetLastNtError(NTSTATUS Status)
{
  SetLastWin32Error(RtlNtStatusToDosError(Status));
}

VOID
SetLastWin32Error(DWORD Status)
{
  PTEB Teb = NtCurrentTeb();

  if (NULL != Teb)
    {
      Teb->LastErrorValue = Status;
    }
}

/* EOF */
