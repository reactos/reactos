/* $Id: misc.c,v 1.5 2003/08/17 09:17:04 weiden Exp $
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
#include <include/dce.h>

#define NDEBUG
#include <debug.h>


DWORD
STDCALL
NtUserCallNoParam(
  DWORD Routine)
{
/*
  switch(Routine)
  {
    case 1:
      return 0;
  }
  DPRINT1("Calling invalid routine number 0x%x in NtUserCallNoParam()\n", Routine);
  SetLastWin32Error(ERROR_INVALID_PARAMETER);
*/
  UNIMPLEMENTED
  return 0;
}


DWORD
STDCALL
NtUserCallOneParam(
  DWORD Param,
  DWORD Routine)
{
  DWORD Result = 0;
  PWINDOW_OBJECT WindowObject;
  switch(Routine)
  {
    case ONEPARAM_ROUTINE_GETMENU:
      WindowObject = W32kGetWindowObject((HWND)Param);
      if(!WindowObject)
      {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return FALSE;
      }
      Result = (DWORD)WindowObject->Menu;
      W32kReleaseWindowObject(WindowObject);
      return Result;
      
    case ONEPARAM_ROUTINE_ISWINDOWUNICODE:
      WindowObject = W32kGetWindowObject((HWND)Param);
      if(!WindowObject)
      {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return FALSE;
      }
      Result = WindowObject->Unicode;
      W32kReleaseWindowObject(WindowObject);
      return Result;
      
    case ONEPARAM_ROUTINE_WINDOWFROMDC:
      return (DWORD)W32kWindowFromDC((HDC)Param);

  }
  DPRINT1("Calling invalid routine number 0x%x in NtUserCallOneParam()\n Param=0x%x\n", 
          Routine, Param);
  SetLastWin32Error(ERROR_INVALID_PARAMETER);
  return 0;
}


DWORD
STDCALL
NtUserCallTwoParam(
  DWORD Param1,
  DWORD Param2,
  DWORD Routine)
{
  switch(Routine)
  {
    case TWOPARAM_ROUTINE_ENABLEWINDOW:
	  UNIMPLEMENTED
      return 0;

    case TWOPARAM_ROUTINE_UNKNOWN:
	  UNIMPLEMENTED
	  return 0;

    case TWOPARAM_ROUTINE_SHOWOWNEDPOPUPS:
	  UNIMPLEMENTED
	  return 0;

    case TWOPARAM_ROUTINE_SWITCHTOTHISWINDOW:
	  UNIMPLEMENTED
	  return 0;

    case TWOPARAM_ROUTINE_VALIDATERGN:
      return (DWORD)NtUserValidateRgn((HWND) Param1, (HRGN) Param2);

  }
  DPRINT1("Calling invalid routine number 0x%x in NtUserCallOneParam()\n Param1=0x%x Parm2=0x%x\n",
          Routine, Param1, Param2);
  SetLastWin32Error(ERROR_INVALID_PARAMETER);
  return 0;
}
