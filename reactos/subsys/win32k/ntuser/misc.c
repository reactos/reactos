/* $Id: misc.c,v 1.24 2003/11/18 23:33:31 weiden Exp $
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
#include <internal/safe.h>
#include <include/error.h>
#include <include/window.h>
#include <include/painting.h>
#include <include/dce.h>
#include <include/mouse.h>
#include <include/winsta.h>
#include <include/caret.h>
#include <include/object.h>

#define NDEBUG
#include <debug.h>

void W32kRegisterPrimitiveMessageQueue() {
  extern PUSER_MESSAGE_QUEUE pmPrimitiveMessageQueue;
  if( !pmPrimitiveMessageQueue ) {
    PW32THREAD pThread;
    pThread = PsGetWin32Thread();
    if( pThread && pThread->MessageQueue ) {
      pmPrimitiveMessageQueue = pThread->MessageQueue;
      DbgPrint( "Installed primitive input queue.\n" );
    }    
  } else {
    DbgPrint( "Alert! Someone is trying to steal the primitive queue.\n" );
  }
}

PUSER_MESSAGE_QUEUE W32kGetPrimitiveMessageQueue() {
  extern PUSER_MESSAGE_QUEUE pmPrimitiveMessageQueue;
  return pmPrimitiveMessageQueue;
}

/*
 * @unimplemented
 */
DWORD
STDCALL
NtUserCallNoParam(DWORD Routine)
{
  DWORD Result = 0;

  switch(Routine)
  {
    case NOPARAM_ROUTINE_REGISTER_PRIMITIVE:
      W32kRegisterPrimitiveMessageQueue();
      Result = (DWORD)TRUE;
      break;
    
    case NOPARAM_ROUTINE_DESTROY_CARET:
      Result = (DWORD)IntDestroyCaret(PsGetCurrentThread()->Win32Thread);
      break;
    
    default:
      DPRINT1("Calling invalid routine number 0x%x in NtUserCallTwoParam\n");
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      break;
  }
  return Result;
}

/*
 * @implemented
 */
DWORD
STDCALL
NtUserCallOneParam(
  DWORD Param,
  DWORD Routine)
{
  NTSTATUS Status;
  DWORD Result = 0;
  PWINSTATION_OBJECT WinStaObject;
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
      
      Result = (DWORD)WindowObject->IDMenu;
      
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
    
    case ONEPARAM_ROUTINE_SWAPMOUSEBUTTON:
      Status = ValidateWindowStationHandle(PROCESS_WINDOW_STATION(),
                                           KernelMode,
                                           0,
                                           &WinStaObject);
      if (!NT_SUCCESS(Status))
        return (DWORD)FALSE;

      Result = (DWORD)IntSwapMouseButton(WinStaObject, (BOOL)Param);

      ObDereferenceObject(WinStaObject);
      return Result;
    
    case ONEPARAM_ROUTINE_SWITCHCARETSHOWING:
      return (DWORD)IntSwitchCaretShowing((PVOID)Param);
    
    case ONEPARAM_ROUTINE_SETCARETBLINKTIME:
      return (DWORD)IntSetCaretBlinkTime((UINT)Param);

  }
  DPRINT1("Calling invalid routine number 0x%x in NtUserCallOneParam()\n Param=0x%x\n", 
          Routine, Param);
  SetLastWin32Error(ERROR_INVALID_PARAMETER);
  return 0;
}


/*
 * @implemented
 */
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
  POINT Pos;
  
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
      
      if(Param2)
      {
        /* set cursor position */
        
        Status = MmCopyFromCaller(&Pos, (PPOINT)Param1, sizeof(POINT));
        if(!NT_SUCCESS(Status))
        {
          ObDereferenceObject(WinStaObject);
          SetLastNtError(Status);
          return FALSE;
        }
        
        CurInfo = &WinStaObject->SystemCursor;
        /* FIXME - check if process has WINSTA_WRITEATTRIBUTES */
        
        //CheckClipCursor(&Pos->x, &Pos->y, CurInfo);  
        if((Pos.x != CurInfo->x) || (Pos.y != CurInfo->y))
        {
          MouseMoveCursor(Pos.x, Pos.y);
        }

      }
      else
      {
        /* get cursor position */
        /* FIXME - check if process has WINSTA_READATTRIBUTES */
        Pos.x = WinStaObject->SystemCursor.x;
        Pos.y = WinStaObject->SystemCursor.y;
        
        Status = MmCopyToCaller((PPOINT)Param1, &Pos, sizeof(POINT));
        if(!NT_SUCCESS(Status))
        {
          ObDereferenceObject(WinStaObject);
          SetLastNtError(Status);
          return FALSE;
        }
        
      }
      
      ObDereferenceObject(WinStaObject);
      
      return (DWORD)TRUE;
    
    case TWOPARAM_ROUTINE_SETCARETPOS:
      return (DWORD)IntSetCaretPos((int)Param1, (int)Param2);
  }
  DPRINT1("Calling invalid routine number 0x%x in NtUserCallTwoParam()\n Param1=0x%x Parm2=0x%x\n",
          Routine, Param1, Param2);
  SetLastWin32Error(ERROR_INVALID_PARAMETER);
  return 0;
}


/*
 * @implemented
 */
DWORD
STDCALL
NtUserSystemParametersInfo(
  UINT uiAction,
  UINT uiParam,
  PVOID pvParam,
  UINT fWinIni)
{
  /* FIXME: This should be obtained from the registry */
  static LOGFONTW CaptionFont =
  { 14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
    0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"" };
/*  { 12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, OEM_CHARSET,
    0, 0, DEFAULT_QUALITY, FF_MODERN, L"Bitstream Vera Sans Bold" };*/
  NTSTATUS Status;
  PWINSTATION_OBJECT WinStaObject;
  
  switch(uiAction)
  {
    case SPI_SETDOUBLECLKWIDTH:
    case SPI_SETDOUBLECLKHEIGHT:
    case SPI_SETDOUBLECLICKTIME:
      {
        Status = ValidateWindowStationHandle(PROCESS_WINDOW_STATION(),
                                             KernelMode,
                                             0,
                                             &WinStaObject);
        if (!NT_SUCCESS(Status))
          return (DWORD)FALSE;
        
        switch(uiAction)
        {
          case SPI_SETDOUBLECLKWIDTH:
            /* FIXME limit the maximum value? */
            WinStaObject->SystemCursor.DblClickWidth = uiParam;
            break;
          case SPI_SETDOUBLECLKHEIGHT:
            /* FIXME limit the maximum value? */
            WinStaObject->SystemCursor.DblClickHeight = uiParam;
            break;
          case SPI_SETDOUBLECLICKTIME:
            /* FIXME limit the maximum time to 1000 ms? */
            WinStaObject->SystemCursor.DblClickSpeed = uiParam;
            break;
        }
        
        /* FIXME save the value to the registry */
        
        ObDereferenceObject(WinStaObject);
        return TRUE;
      }
    case SPI_SETWORKAREA:
      {
        PDESKTOP_OBJECT Desktop = PsGetWin32Thread()->Desktop;
        
        if(!Desktop)
        {
          /* FIXME - Set last error */
          return FALSE;
        }
        
        Status = MmCopyFromCaller(Desktop->WorkArea, (PRECT)pvParam, sizeof(RECT));
        if(!NT_SUCCESS(Status))
        {
          SetLastNtError(Status);
          return FALSE;
        }
        
        return TRUE;
      }
    case SPI_GETWORKAREA:
      {
        PDESKTOP_OBJECT Desktop = PsGetWin32Thread()->Desktop;
        
        if(!Desktop)
        {
          /* FIXME - Set last error */
          return FALSE;
        }
        
        Status = MmCopyToCaller((PRECT)pvParam, Desktop->WorkArea, sizeof(RECT));
        if(!NT_SUCCESS(Status))
        {
          SetLastNtError(Status);
          return FALSE;
        }
        
        return TRUE;
      }
    case SPI_GETICONTITLELOGFONT:
      {
        Status = MmCopyToCaller(pvParam, &CaptionFont, sizeof(CaptionFont));
        if(!NT_SUCCESS(Status))
        {
          SetLastNtError(Status);
          return FALSE;
        }
        return TRUE;
      }
    case SPI_GETNONCLIENTMETRICS:
      {
        /* FIXME - use MmCopyToCaller() !!! */
        LPNONCLIENTMETRICSW pMetrics = (LPNONCLIENTMETRICSW)pvParam;
    
        if (pMetrics->cbSize != sizeof(NONCLIENTMETRICSW) || 
            uiParam != sizeof(NONCLIENTMETRICSW))
        {
          return FALSE;
        }

        memset((char *)pvParam + sizeof(pMetrics->cbSize), 0,
          pMetrics->cbSize - sizeof(pMetrics->cbSize));

        pMetrics->iBorderWidth = 1;
        pMetrics->iScrollWidth = NtUserGetSystemMetrics(SM_CXVSCROLL);
        pMetrics->iScrollHeight = NtUserGetSystemMetrics(SM_CYHSCROLL);
        pMetrics->iCaptionWidth = NtUserGetSystemMetrics(SM_CXSIZE);
        pMetrics->iCaptionHeight = NtUserGetSystemMetrics(SM_CYSIZE);
        memcpy((LPVOID)&(pMetrics->lfCaptionFont), &CaptionFont, sizeof(CaptionFont));
        pMetrics->lfCaptionFont.lfWeight = FW_BOLD;
        pMetrics->iSmCaptionWidth = NtUserGetSystemMetrics(SM_CXSMSIZE);
        pMetrics->iSmCaptionHeight = NtUserGetSystemMetrics(SM_CYSMSIZE);
        memcpy((LPVOID)&(pMetrics->lfSmCaptionFont), &CaptionFont, sizeof(CaptionFont));
        pMetrics->iMenuWidth = NtUserGetSystemMetrics(SM_CXMENUSIZE);
        pMetrics->iMenuHeight = NtUserGetSystemMetrics(SM_CYMENUSIZE);
        memcpy((LPVOID)&(pMetrics->lfMenuFont), &CaptionFont, sizeof(CaptionFont));
        memcpy((LPVOID)&(pMetrics->lfStatusFont), &CaptionFont, sizeof(CaptionFont));
        memcpy((LPVOID)&(pMetrics->lfMessageFont), &CaptionFont, sizeof(CaptionFont));
        return TRUE;
      }
    
  }
  return FALSE;
}

UINT
STDCALL
NtUserGetDoubleClickTime(VOID)
{
  UINT Result;
  NTSTATUS Status;
  PWINSTATION_OBJECT WinStaObject;
  
  Status = ValidateWindowStationHandle(PROCESS_WINDOW_STATION(),
                                       KernelMode,
                                       0,
                                       &WinStaObject);
  if (!NT_SUCCESS(Status))
    return (DWORD)FALSE;

  Result = WinStaObject->SystemCursor.DblClickSpeed;
      
  ObDereferenceObject(WinStaObject);
  return Result;
}

BOOL
STDCALL
NtUserGetGUIThreadInfo(
  DWORD idThread,
  LPGUITHREADINFO lpgui)
{
  NTSTATUS Status;
  PTHRDCARETINFO CaretInfo;
  GUITHREADINFO SafeGui;
  PDESKTOP_OBJECT Desktop;
  PUSER_MESSAGE_QUEUE MsgQueue;
  PETHREAD Thread = NULL;
  
  Status = MmCopyFromCaller(&SafeGui, lpgui, sizeof(DWORD));
  if(!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    return FALSE;
  }
  
  if(SafeGui.cbSize != sizeof(GUITHREADINFO))
  {
    SetLastWin32Error(ERROR_INVALID_PARAMETER);
    return FALSE;
  }
  
  if(idThread)
  {
    Status = PsLookupThreadByThreadId((PVOID)idThread, &Thread);
    if(!NT_SUCCESS(Status))
    {
      SetLastWin32Error(ERROR_ACCESS_DENIED);
      return FALSE;
    }
    Desktop = Thread->Win32Thread->Desktop;
  }
  else
  {
    /* get the foreground thread */
    PW32THREAD W32Thread = PsGetCurrentThread()->Win32Thread;
    Desktop = W32Thread->Desktop;
    if(Desktop)
    {
      MsgQueue = Desktop->ActiveMessageQueue;
      if(MsgQueue)
      {
        Thread = MsgQueue->Thread;
      }
    }
  }
  
  if(!Thread || !Desktop)
  {
    SetLastWin32Error(ERROR_ACCESS_DENIED);
    return FALSE;
  }
  
  CaretInfo = ThrdCaretInfo(Thread->Win32Thread);
  MsgQueue = (PUSER_MESSAGE_QUEUE)Desktop->ActiveMessageQueue;
  
  SafeGui.flags = (CaretInfo->Visible ? GUI_CARETBLINKING : 0);
  /* FIXME add flags GUI_16BITTASK, GUI_INMENUMODE, GUI_INMOVESIZE,
                     GUI_POPUPMENUMODE, GUI_SYSTEMMENUMODE */
  
  SafeGui.hwndActive = MsgQueue->ActiveWindow;
  SafeGui.hwndFocus = MsgQueue->FocusWindow;
  SafeGui.hwndCapture = MsgQueue->CaptureWindow;
  SafeGui.hwndMenuOwner = 0; /* FIXME */
  SafeGui.hwndMoveSize = 0;  /* FIXME */
  SafeGui.hwndCaret = CaretInfo->hWnd;
  
  SafeGui.rcCaret.left = CaretInfo->Pos.x;
  SafeGui.rcCaret.top = CaretInfo->Pos.y;
  SafeGui.rcCaret.right = SafeGui.rcCaret.left + CaretInfo->Size.cx;
  SafeGui.rcCaret.bottom = SafeGui.rcCaret.top + CaretInfo->Size.cy;
  
  Status = MmCopyToCaller(lpgui, &SafeGui, sizeof(GUITHREADINFO));
  if(!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    return FALSE;
  }

  return TRUE;
}

