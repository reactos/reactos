/* $Id: misc.c,v 1.1 2003/05/26 18:52:37 gvg Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Misc User funcs
 * FILE:             subsys/win32k/ntuser/misc.c
 * PROGRAMER:        Ge van Geldorp (ge@gse.nl)
 * REVISION HISTORY:
 *       2003/05/22  Created
 */
#include <ddk/ntddk.h>
#include <win32k/win32k.h>
#include <include/error.h>
#include <include/window.h>
#include <include/painting.h>

#define NDEBUG
#include <debug.h>


DWORD
STDCALL
NtUserCallTwoParam(
  DWORD Param1,
  DWORD Param2,
  DWORD Routine)
{
  DWORD Result;

  switch(Routine)
    {
      case TWOPARAM_ROUTINE_ENABLEWINDOW:
	UNIMPLEMENTED
	break;

      case TWOPARAM_ROUTINE_UNKNOWN:
	UNIMPLEMENTED
	break;

      case TWOPARAM_ROUTINE_SHOWOWNEDPOPUPS:
	UNIMPLEMENTED
	break;

      case TWOPARAM_ROUTINE_SWITCHTOTHISWINDOW:
	UNIMPLEMENTED
	break;

      case TWOPARAM_ROUTINE_VALIDATERGN:
	Result = (DWORD) NtUserValidateRgn((HWND) Param1, (HRGN) Param2);
	break;

      default:
	DPRINT1("Calling invalid routine number 0x%x in NtUserCallTwoParam\n");
	SetLastWin32Error(ERROR_INVALID_PARAMETER);
	Result = 0;
	break;
    }

  return Result;
}
