/* $Id: misc.c,v 1.11 2003/08/24 23:52:29 weiden Exp $
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
#include <ddk/ntddmou.h>
#include <win32k/win32k.h>
#include <win32k/dc.h>
#include <include/error.h>
#include <include/window.h>
#include <include/painting.h>
#include <include/dce.h>
#include <include/mouse.h>
#include <include/winsta.h>

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
      WindowObject = IntGetWindowObject((HWND)Param);
      if(!WindowObject)
      {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return FALSE;
      }
      
      Result = (DWORD)WindowObject->Menu;
      
      IntReleaseWindowObject(WindowObject);
      return Result;
      
    case ONEPARAM_ROUTINE_ISWINDOWUNICODE:
      WindowObject = IntGetWindowObject((HWND)Param);
      if(!WindowObject)
      {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return FALSE;
      }
      Result = WindowObject->Unicode;
      IntReleaseWindowObject(WindowObject);
      return Result;
      
    case ONEPARAM_ROUTINE_WINDOWFROMDC:
      return (DWORD)IntWindowFromDC((HDC)Param);
      
    case ONEPARAM_ROUTINE_GETWNDCONTEXTHLPID:
      WindowObject = IntGetWindowObject((HWND)Param);
      if(!WindowObject)
      {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return FALSE;
      }
      
      Result = WindowObject->ContextHelpId;
      
      IntReleaseWindowObject(WindowObject);
      return Result;

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
  NTSTATUS Status;
  PWINDOW_OBJECT WindowObject;
  PSYSTEM_CURSORINFO CurInfo;
  PWINSTATION_OBJECT WinStaObject;
  PPOINT Pos;
  
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
      
    case TWOPARAM_ROUTINE_SETWNDCONTEXTHLPID:
      WindowObject = IntGetWindowObject((HWND)Param1);
      if(!WindowObject)
      {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return (DWORD)FALSE;
      }
      
      WindowObject->ContextHelpId = Param2;
      
      IntReleaseWindowObject(WindowObject);
      return (DWORD)TRUE;
      
    case TWOPARAM_ROUTINE_CURSORPOSITION:
      if(!Param1)
        return (DWORD)FALSE;
      Status = ValidateWindowStationHandle(PROCESS_WINDOW_STATION(),
                                           KernelMode,
                                           0,
                                           &WinStaObject);
      if (!NT_SUCCESS(Status))
        return (DWORD)FALSE;
        
      Pos = (PPOINT)Param1;
      
      if(Param2)
      {
        /* set cursor position */
        
        CurInfo = &WinStaObject->SystemCursor;
        /* FIXME - check if process has WINSTA_WRITEATTRIBUTES */
        
        CheckClipCursor(&Pos->x, &Pos->y, CurInfo);  
        if((Pos->x != CurInfo->x) || (Pos->y != CurInfo->y))
        {
          MouseMoveCursor(Pos->x, Pos->y);
        }

      }
      else
      {
        /* get cursor position */
        /* FIXME - check if process has WINSTA_READATTRIBUTES */
        Pos->x = WinStaObject->SystemCursor.x;
        Pos->y = WinStaObject->SystemCursor.y;
      }
      
      ObDereferenceObject(WinStaObject);
      
      return (DWORD)TRUE;

  }
  DPRINT1("Calling invalid routine number 0x%x in NtUserCallOneParam()\n Param1=0x%x Parm2=0x%x\n",
          Routine, Param1, Param2);
  SetLastWin32Error(ERROR_INVALID_PARAMETER);
  return 0;
}
