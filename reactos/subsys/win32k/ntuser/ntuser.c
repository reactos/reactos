/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998 - 2004 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: ntuser.c,v 1.1.4.11 2004/09/24 18:35:40 weiden Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Entry points for user mode calls
 * FILE:             subsys/win32k/ntuser/ntuser.c
 * PROGRAMER:        Thomas Weidenmueller <w3seek@reactos.com>
 * REVISION HISTORY:
 *       06-22-2004  Created
 */
#include <w32k.h>

/*
 * NtUser handling macros
 */

#define BEGIN_NTUSER(ReturnType, ErrorReturn) \
  ReturnType Result, ErrorResult = ErrorReturn

#define BEGIN_NTUSER_NOERR(ReturnType) \
  ReturnType Result

#define END_NTUSER() \
  if(IN_CRITICAL()) \
  { \
    LEAVE_CRITICAL(); \
  } \
  return Result; \
error: \
  return ErrorResult

#define END_NTUSER_NOERR() \
  return Result;

#define NTUSER_USER_OBJECT(Type, Variable) \
  P##Type##_OBJECT Variable = NULL

#define NTUSER_FAIL_ERROR(ErrorCode) \
  if(ErrorCode != 0) \
  { \
    SetLastWin32Error(ErrorCode); \
  } \
  goto error

#define NTUSER_FAIL_INVALID_PARAMETER(Variable, FailCondition) \
  if((Variable) == (FailCondition)) \
  { \
    SetLastWin32Error(ERROR_INVALID_PARAMETER); \
    goto error; \
  }

/* FIXME - Probe SizeMember first!!! */
#define NTUSER_FAIL_INVALID_STRUCT(StructType, StructAddr, SizeMember) \
  if((StructAddr)->SizeMember != sizeof(StructType)) \
  { \
    SetLastWin32Error(ERROR_INVALID_PARAMETER); \
    goto error; \
  }

#define NTUSER_FAIL() \
  goto error

#define NTUSER_FAIL_NTERROR(ErrorCode) \
  SetLastNtError(ErrorCode); \
  goto error

#define BEGIN_BUFFERS() \
  NTSTATUS NtStatus

#define NTUSER_COPY_BUFFER(Dest, Src, Length) \
  Status = MmCopyFromCaller(&(Dest), (Src), (Length))

#define NTUSER_COPY_BUFFER_W32ERROR(Dest, Src, Length, ErrorCode) \
  if(!NT_SUCCESS((NtStatus = MmCopyFromCaller((Dest), (Src), (Length))))) \
  { \
    SetLastWin32Error(ErrorCode); \
    goto error; \
  }

#define NTUSER_COPY_BUFFER_NTERROR(Dest, Src, Length) \
  if(!NT_SUCCESS((NtStatus = MmCopyFromCaller((Dest), (Src), (Length))))) \
  { \
    SetLastNtError(NtStatus); \
    goto error; \
  }

#define NTUSER_COPY_BUFFER_BACK(Dest, Src, Length) \
  NtStatus = MmCopyFromCaller(&(Dest), (Src), (Length))

#define NTUSER_COPY_BUFFER_BACK_W32ERROR(Dest, Src, Length, ErrorCode) \
  if(!NT_SUCCESS((NtStatus = MmCopyToCaller((Dest), (Src), (Length))))) \
  { \
    SetLastWin32Error(ErrorCode); \
    goto error; \
  }

#define NTUSER_COPY_BUFFER_BACK_NTERROR(Dest, Src, Length) \
  if(!NT_SUCCESS((NtStatus = MmCopyToCaller((Dest), (Src), (Length))))) \
  { \
    SetLastNtError(NtStatus); \
    goto error; \
  }

#define VALIDATE_USER_OBJECT(ObjectType, Handle, Object) \
  if(!(Object = ObmGetObject(PsGetWin32Process()->WindowStation->HandleTable,  \
                             (Handle), ot##ObjectType ))) \
  { \
    LEAVE_CRITICAL(); \
    DPRINT1("%s(): Invalid %s handle: 0x%x !!!\n", __FUNCTION__, #ObjectType , Handle); \
    SetLastWin32Error(ERROR_INVALID_##ObjectType##_HANDLE); \
    goto error; \
  } \

#define VALIDATE_USER_OBJECT_NOERR(ObjectType, Handle, Object) \
  if(!(Object = ObmGetObject(PsGetWin32Process()->WindowStation->HandleTable,  \
                             (Handle), ot##ObjectType ))) \
  { \
    LEAVE_CRITICAL(); \
    DPRINT1("%s(): Invalid %s handle: 0x%x !!!\n", __FUNCTION__, #ObjectType , Handle); \
    goto error; \
  } \

/*
 * Locking macros
 */

#define ENTER_CRITICAL() \
  IntUserEnterCritical()

#define ENTER_CRITICAL_SHARED() \
  IntUserEnterCriticalShared()

#define LEAVE_CRITICAL() \
  IntUserLeaveCritical()

#define IN_CRITICAL() \
  IntUserIsInCritical()


/*
 * User entry points
 */

HDC STDCALL
NtUserBeginPaint(HWND hWnd, PAINTSTRUCT* lPs)
{
  PAINTSTRUCT SafePs;
  NTUSER_USER_OBJECT(WINDOW, Window);
  BEGIN_BUFFERS();
  BEGIN_NTUSER(HDC, NULL);
  
  /* FIXME - what should we do if there's no write access to lPs? */
  
  ENTER_CRITICAL();
  if(hWnd != NULL)
  {
    VALIDATE_USER_OBJECT(WINDOW, hWnd, Window);
  }
  else
  {
    if(!(Window = IntGetDesktopWindow()))
    {
      LEAVE_CRITICAL();
      DPRINT1("GetWindowDC(): Unable to get desktop window!\n");
      NTUSER_FAIL_ERROR(ERROR_ACCESS_DENIED);
    }
  }
  
  Result = IntBeginPaint(Window, &SafePs);
  
  LEAVE_CRITICAL();
  
  /* Apparently windows always sets the last error to ERROR_NOACCESS in case it can't copy
     back the buffer */
  NTUSER_COPY_BUFFER_BACK_W32ERROR(lPs, &SafePs, sizeof(PAINTSTRUCT), ERROR_NOACCESS);
  
  END_NTUSER();
}

DWORD STDCALL
NtUserCallNoParam(DWORD Routine)
{
  BEGIN_NTUSER(DWORD, 0);
  
  switch(Routine)
  {
    case NOPARAM_ROUTINE_REGISTER_PRIMITIVE:
      ENTER_CRITICAL();
      W32kRegisterPrimitiveMessageQueue();
      LEAVE_CRITICAL();
      Result = TRUE;
      break;
    
    case NOPARAM_ROUTINE_DESTROY_CARET:
      ENTER_CRITICAL();
      #if 0
      Result = (DWORD)IntDestroyCaret(PsGetCurrentThread()->Win32Thread);
      #else
      Result = FALSE;
      #endif
      LEAVE_CRITICAL();
      break;
    
    case NOPARAM_ROUTINE_INIT_MESSAGE_PUMP:
      ENTER_CRITICAL();
      Result = (DWORD)IntInitMessagePumpHook();
      LEAVE_CRITICAL();
      break;

    case NOPARAM_ROUTINE_UNINIT_MESSAGE_PUMP:
      ENTER_CRITICAL();
      Result = (DWORD)IntUninitMessagePumpHook();
      LEAVE_CRITICAL();
      break;
    
    case NOPARAM_ROUTINE_GETMESSAGEEXTRAINFO:
      ENTER_CRITICAL_SHARED();
      Result = (DWORD)MsqGetMessageExtraInfo();
      LEAVE_CRITICAL();
      break;
    
    case NOPARAM_ROUTINE_ANYPOPUP:
      ENTER_CRITICAL_SHARED();
      Result = (DWORD)IntAnyPopup();
      LEAVE_CRITICAL();
      break;

    case NOPARAM_ROUTINE_CSRSS_INITIALIZED:
      Result = (DWORD)CsrInit();
      break;
    
    default:
      DPRINT1("Calling invalid routine number 0x%x in NtUserCallNoParam\n", Routine);
      NTUSER_FAIL_ERROR(ERROR_INVALID_PARAMETER);
      break;
  }
  
  END_NTUSER();
}

DWORD STDCALL
NtUserCallOneParam(DWORD Param,
                   DWORD Routine)
{
  NTUSER_USER_OBJECT(WINDOW, Window);
  NTUSER_USER_OBJECT(MENU, Menu);
  BEGIN_NTUSER(DWORD, 0);
  
  switch(Routine)
  {
    case ONEPARAM_ROUTINE_GETMENU:
      ENTER_CRITICAL_SHARED();
      VALIDATE_USER_OBJECT(WINDOW, (HWND)Param, Window);
      Menu = IntGetWindowMenu(Window);
      Result = (DWORD)(Menu ? Menu->Handle : NULL);
      LEAVE_CRITICAL();
      break;

    case ONEPARAM_ROUTINE_ISWINDOWUNICODE:
      ENTER_CRITICAL_SHARED();
      VALIDATE_USER_OBJECT(WINDOW, (HWND)Param, Window);
      Result = (DWORD)IntIsWindowUnicode(Window);
      LEAVE_CRITICAL();
      break;

    case ONEPARAM_ROUTINE_WINDOWFROMDC:
      ENTER_CRITICAL_SHARED();
      Window = IntWindowFromDC((HDC)Param);
      Result = (DWORD)(Window ? Window->Handle : NULL);
      LEAVE_CRITICAL();
      break;

    case ONEPARAM_ROUTINE_GETWNDCONTEXTHLPID:
      ENTER_CRITICAL_SHARED();
      VALIDATE_USER_OBJECT(WINDOW, (HWND)Param, Window);
      Result = Window->ContextHelpId;
      LEAVE_CRITICAL();
      break;

    case ONEPARAM_ROUTINE_GETWINDOWINSTANCE:
      ENTER_CRITICAL_SHARED();
      VALIDATE_USER_OBJECT(WINDOW, (HWND)Param, Window);
      Result = (DWORD)Window->Instance;
      LEAVE_CRITICAL();
      break;

    case ONEPARAM_ROUTINE_ISWINDOWINDESTROY:
      ENTER_CRITICAL_SHARED();
      VALIDATE_USER_OBJECT(WINDOW, (HWND)Param, Window);
      /* FIXME */
      LEAVE_CRITICAL();
      break;

    case ONEPARAM_ROUTINE_SWAPMOUSEBUTTON:
    case ONEPARAM_ROUTINE_SWITCHCARETSHOWING:
    case ONEPARAM_ROUTINE_SETCARETBLINKTIME:
    case ONEPARAM_ROUTINE_ENUMCLIPBOARDFORMATS:
    case ONEPARAM_ROUTINE_SETMESSAGEEXTRAINFO:
    case ONEPARAM_ROUTINE_GETCURSORPOSITION:
    case ONEPARAM_ROUTINE_ENABLEPROCWNDGHSTING:
    default:
      DPRINT1("Calling invalid routine number 0x%x in NtUserCallOneParam\n", Routine);
      NTUSER_FAIL_ERROR(ERROR_INVALID_PARAMETER);
  }
  
  END_NTUSER();
}

DWORD STDCALL
NtUserCallTwoParam(DWORD Param1,
                   DWORD Param2,
                   DWORD Routine)
{
  BEGIN_NTUSER(DWORD, 0);
  
  switch(Routine)
  {
    case TWOPARAM_ROUTINE_SETDCPENCOLOR:
      /* FIXME */
      Result = (DWORD)IntSetDCColor((HDC)Param1, OBJ_PEN, (COLORREF)Param2);
      break;
    
    case TWOPARAM_ROUTINE_SETDCBRUSHCOLOR:
      /* FIXME */
      Result = (DWORD)IntSetDCColor((HDC)Param1, OBJ_BRUSH, (COLORREF)Param2);
      break;
    
    case TWOPARAM_ROUTINE_GETDCCOLOR:
      /* FIXME */
      Result = (DWORD)IntGetDCColor((HDC)Param1, (ULONG)Param2);
      break;
    
    case TWOPARAM_ROUTINE_GETWINDOWRGNBOX:
    {
      RECT SafeRect;
      NTUSER_USER_OBJECT(WINDOW, Window);
      BEGIN_BUFFERS();
      
      ENTER_CRITICAL_SHARED();
      VALIDATE_USER_OBJECT(WINDOW, (HWND)Param1, Window);
      Result = (DWORD)IntGetWindowRgnBox(Window, &SafeRect);
      LEAVE_CRITICAL();
      
      ErrorResult = ERROR; /* return ERROR in case we fail */
      NTUSER_COPY_BUFFER_BACK_NTERROR((PVOID)Param2, &SafeRect, sizeof(RECT));
      break;
    }
    
    case TWOPARAM_ROUTINE_GETWINDOWRGN:
    {
      NTUSER_USER_OBJECT(WINDOW, Window);
      
      ENTER_CRITICAL_SHARED();
      VALIDATE_USER_OBJECT(WINDOW, (HWND)Param1, Window);
      Result = (DWORD)IntGetWindowRgn(Window, (HRGN)Param2);
      LEAVE_CRITICAL();
      break;
    }
    
    case TWOPARAM_ROUTINE_SETMENUBARHEIGHT:
      DPRINT1("TWOPARAM_ROUTINE_SETMENUBARHEIGHT is unimplemented!\n");
      break;
    
    case TWOPARAM_ROUTINE_SETMENUITEMRECT:
      DPRINT1("TWOPARAM_ROUTINE_SETMENUITEMRECT is unimplemented!\n");
      break;
    
    case TWOPARAM_ROUTINE_SETGUITHRDHANDLE:
    {
      PUSER_MESSAGE_QUEUE MsgQueue;
      NTUSER_USER_OBJECT(WINDOW, Window);
      
      ENTER_CRITICAL();
      VALIDATE_USER_OBJECT(WINDOW, (HWND)Param2, Window);
      MsgQueue = PsGetCurrentThread()->Win32Thread->MessageQueue;
      if(Window->MessageQueue != MsgQueue)
      {
        NTUSER_FAIL_ERROR(ERROR_ACCESS_DENIED);
      }
      Result = (DWORD)MsqSetStateWindow(MsgQueue, (ULONG)Param1, Window);
      LEAVE_CRITICAL();
      
      break;
    }
    
    case TWOPARAM_ROUTINE_VALIDATERGN:
    {
      /* FIXME !!!!! REMOVE THIS ROUTINE AND EXPORT NtUserValidateRgn() INSTEAD! */
      Result = NtUserValidateRgn((HWND)Param1, (HRGN)Param2);
      break;
    }
    
    case TWOPARAM_ROUTINE_SETWNDCONTEXTHLPID:
    {
      NTUSER_USER_OBJECT(WINDOW, Window);
      
      ENTER_CRITICAL();
      VALIDATE_USER_OBJECT(WINDOW, (HWND)Param1, Window);
      Window->ContextHelpId = Param2;
      LEAVE_CRITICAL();
      
      Result = (DWORD)TRUE;
      break;
    }
    
    case TWOPARAM_ROUTINE_SETCARETPOS:
      DPRINT1("TWOPARAM_ROUTINE_SETCARETPOS is unimplemented!\n");
      break;
    
    case TWOPARAM_ROUTINE_GETWINDOWINFO:
    {
      WINDOWINFO SafeWi;
      NTUSER_USER_OBJECT(WINDOW, Window);
      BEGIN_BUFFERS();
      
      ENTER_CRITICAL_SHARED();
      VALIDATE_USER_OBJECT(WINDOW, (HWND)Param1, Window);
      Result = (DWORD)IntGetWindowInfo(Window, &SafeWi);
      LEAVE_CRITICAL();
      
      if(Result)
      {
        NTUSER_COPY_BUFFER_BACK_NTERROR((PVOID)Param2, &SafeWi, sizeof(WINDOWINFO));
      }
      break;
    }
    
    case TWOPARAM_ROUTINE_REGISTERLOGONPROC:
      Result = (DWORD)IntRegisterLogonProcess(Param1, (BOOL)Param2);
      break;
    
    default:
      DPRINT1("Calling invalid routine number 0x%x in NtUserCallTwoParam\n", Routine);
      NTUSER_FAIL_ERROR(ERROR_INVALID_PARAMETER);
      break;
  }
  
  END_NTUSER();
}

HANDLE STDCALL
NtUserCreateCursorIconHandle(PICONINFO IconInfo, BOOL Indirect)
{
  ICONINFO SafeInfo;
  PBITMAPOBJ bmp;
  SIZE sz;
  NTUSER_USER_OBJECT(CURSOR, Cursor);
  BEGIN_BUFFERS();
  BEGIN_NTUSER(HANDLE, NULL);
  
  NTUSER_FAIL_INVALID_PARAMETER(IconInfo, NULL);
  
  NTUSER_COPY_BUFFER_NTERROR(&SafeInfo, IconInfo, sizeof(ICONINFO));
  
  if(Indirect)
  {
    /* Copy the bitmaps */
    SafeInfo.hbmMask = BITMAPOBJ_CopyBitmap(SafeInfo.hbmMask);
    SafeInfo.hbmColor = BITMAPOBJ_CopyBitmap(SafeInfo.hbmColor);
  }
  
  /* Calculate the icon/cursor size from bitmap(s) */
  if(SafeInfo.hbmColor && 
     (bmp = BITMAPOBJ_LockBitmap(SafeInfo.hbmColor)))
  {
    sz.cx = bmp->SurfObj.sizlBitmap.cx;
    sz.cy = bmp->SurfObj.sizlBitmap.cy;
    BITMAPOBJ_UnlockBitmap(bmp);
  }
  else if(SafeInfo.hbmMask && 
          (bmp = BITMAPOBJ_LockBitmap(SafeInfo.hbmMask)))
  {
    sz.cx = bmp->SurfObj.sizlBitmap.cx;
    sz.cy = bmp->SurfObj.sizlBitmap.cy / 2;
    BITMAPOBJ_UnlockBitmap(bmp);
  }
  
  ENTER_CRITICAL();
  Cursor = IntCreateCursorObject(&Result);
  if(Cursor != NULL)
  {
    Cursor->IconInfo = SafeInfo;
    Cursor->Size = sz;
  }
  else
  {
    LEAVE_CRITICAL();
    if(Indirect)
    {
      if(SafeInfo.hbmMask)
        NtGdiDeleteObject(SafeInfo.hbmMask);
      if(SafeInfo.hbmColor)
        NtGdiDeleteObject(SafeInfo.hbmColor);
    }
    NTUSER_FAIL();
  }
  LEAVE_CRITICAL();
  
  END_NTUSER();
}

HWND STDCALL
NtUserCreateWindowEx(DWORD dwExStyle,
		     PUNICODE_STRING UnsafeClassName,
		     PUNICODE_STRING UnsafeWindowName,
		     DWORD dwStyle,
		     LONG x,
		     LONG y,
		     LONG nWidth,
		     LONG nHeight,
		     HWND hWndParent,
		     HMENU hMenu,
		     HINSTANCE hInstance,
		     LPVOID lpParam,
		     DWORD dwShowMode,
		     BOOL bUnicodeWindow)
{
  UNICODE_STRING SafeClassName, SafeWindowName;
  NTUSER_USER_OBJECT(WINDOW, Window);
  NTUSER_USER_OBJECT(WINDOW, Parent);
  NTSTATUS Status;
  BEGIN_BUFFERS();
  BEGIN_NTUSER(HWND, NULL);
  
  if(UnsafeWindowName != NULL)
  {
    Status = IntSafeCopyUnicodeString(&SafeWindowName, UnsafeWindowName);
    if(!NT_SUCCESS(Status))
    {
      NTUSER_FAIL_NTERROR(Status);
    }
  }
  else
  {
    RtlInitUnicodeString(&SafeWindowName, NULL);
  }
  
  /* copy class name (NULL-Terminated! for class atom lookup */
  NTUSER_COPY_BUFFER_NTERROR(&SafeClassName, UnsafeClassName, sizeof(UNICODE_STRING));
  if(!IS_ATOM(SafeClassName.Buffer))
  {
    Status = IntSafeCopyUnicodeStringTerminateNULL(&SafeClassName, UnsafeClassName);
    if(!NT_SUCCESS(Status))
    {
      NTUSER_FAIL_NTERROR(Status);
    }
  }
  
  ENTER_CRITICAL();
  if(hWndParent != NULL)
  {
    VALIDATE_USER_OBJECT(WINDOW, hWndParent, Parent);
  }
  Window = IntCreateWindow(dwExStyle,
                           &SafeClassName,
			   &SafeWindowName,
			   dwStyle,
			   x,
			   y,
			   nWidth,
			   nHeight,
			   Parent,
			   NULL, /* FIXME - Menu */
			   0, /* FIXME - WindowID */
			   hInstance,
			   lpParam,
			   dwShowMode,
			   bUnicodeWindow);
  Result = (Window != NULL ? Window->Handle : NULL);
  LEAVE_CRITICAL();
  
  RtlFreeUnicodeString(&SafeWindowName);
  if(!IS_ATOM(SafeClassName.Buffer))
  {
    RtlFreeUnicodeString(&SafeClassName);
  }
  
  END_NTUSER();
}

BOOL STDCALL
NtUserDefSetText(HWND hWnd, PUNICODE_STRING WindowText)
{
  UNICODE_STRING WndText;
  NTUSER_USER_OBJECT(WINDOW, Window);
  BEGIN_BUFFERS();
  BEGIN_NTUSER(BOOL, FALSE);
  
  if(WindowText != NULL)
  {
    NTUSER_COPY_BUFFER_NTERROR(&WndText, WindowText, sizeof(UNICODE_STRING));
    /* FIXME - probe the string */
  }
  else
  {
    RtlInitUnicodeString(&WndText, NULL);
  }
  
  ENTER_CRITICAL();
  VALIDATE_USER_OBJECT(WINDOW, hWnd, Window);
  Result = IntDefSetText(Window, &WndText);
  LEAVE_CRITICAL();
  
  END_NTUSER();
}

BOOL STDCALL
NtUserDereferenceWndProcHandle(WNDPROC wpHandle, WndProcHandle *Data)
{
  BEGIN_NTUSER_NOERR(BOOL);
  
  ENTER_CRITICAL();
  /* FIXME - probe the buffer Data points to */
  Result = IntDereferenceWndProcHandle(wpHandle, Data);
  LEAVE_CRITICAL();
  
  END_NTUSER_NOERR();
}

BOOL STDCALL
NtUserDestroyCursorIcon(HANDLE Handle,
                        DWORD Unknown)
{
  NTUSER_USER_OBJECT(CURSOR, Cursor);
  BEGIN_NTUSER(BOOL, FALSE);
  
  ENTER_CRITICAL();
  VALIDATE_USER_OBJECT(CURSOR, Handle, Cursor);
  Result = IntDestroyCursorObject(Cursor, TRUE);
  LEAVE_CRITICAL();
  
  END_NTUSER();
}

BOOL STDCALL
NtUserDestroyWindow(HWND hWnd)
{
  PW32PROCESS W32Process;
  PETHREAD Thread;
  NTUSER_USER_OBJECT(WINDOW, Window);
  BEGIN_NTUSER(BOOLEAN, FALSE);
  
  Thread = PsGetCurrentThread();
  W32Process = PsGetWin32Process();
  
  ENTER_CRITICAL();
  VALIDATE_USER_OBJECT(WINDOW, hWnd, Window);
  
  /* Check for owner thread and desktop window */
  if(!IntWndBelongsToThread(Window, Thread->Win32Thread) || IntIsDesktopWindow(Window))
  {
    LEAVE_CRITICAL();
    NTUSER_FAIL_ERROR(ERROR_ACCESS_DENIED);
  }
  ASSERT(W32Process);
  ASSERT(Thread->Win32Thread);
  /* FIXME - send messages if the thread is already terminating? */
  Result = IntDestroyWindow(Window, W32Process, Thread->Win32Thread, TRUE);
  
  LEAVE_CRITICAL();
  
  END_NTUSER();
}

LRESULT STDCALL
NtUserDispatchMessage(PNTUSERDISPATCHMESSAGEINFO UnsafeMsgInfo)
{
  NTUSERDISPATCHMESSAGEINFO MsgInfo;
  NTUSER_USER_OBJECT(WINDOW, Window);
  BEGIN_BUFFERS();
  BEGIN_NTUSER(LRESULT, 0);
  
  NTUSER_COPY_BUFFER_NTERROR(&MsgInfo, UnsafeMsgInfo, sizeof(NTUSERDISPATCHMESSAGEINFO));
  
  ENTER_CRITICAL_SHARED();
  if(MsgInfo.Msg.hwnd != NULL)
  {
    /* win doesn't set a last error in this case */
    VALIDATE_USER_OBJECT_NOERR(WINDOW, MsgInfo.Msg.hwnd, Window);
    
    if(!IntWndBelongsToThread(Window, PsGetWin32Thread()))
    {
      LEAVE_CRITICAL();
      DPRINT1("Window doesn't belong to the calling thread!\n");
      MsgInfo.HandledByKernel = TRUE;
      goto finish;
    }
  }
  Result = IntDispatchMessage(&MsgInfo, Window);
  LEAVE_CRITICAL();
  
finish:
  NTUSER_COPY_BUFFER_BACK_NTERROR(UnsafeMsgInfo, &MsgInfo, sizeof(NTUSERDISPATCHMESSAGEINFO));
  
  END_NTUSER();
}

BOOL STDCALL
NtUserDrawIconEx(HDC hdc,           
                 int xLeft,
                 int yTop,
                 HICON hIcon,
                 int cxWidth,
                 int cyWidth,
                 UINT istepIfAniCur,
                 HBRUSH hbrFlickerFreeDraw,
                 UINT diFlags,
                 DWORD Unknown0,
                 DWORD Unknown1)
{
  NTUSER_USER_OBJECT(CURSOR, Cursor);
  BEGIN_NTUSER(BOOL, FALSE);
  
  ENTER_CRITICAL_SHARED();
  VALIDATE_USER_OBJECT(CURSOR, hIcon, Cursor);
  Result = IntDrawIconEx(hdc, xLeft, yTop, Cursor, cxWidth, cyWidth, istepIfAniCur, hbrFlickerFreeDraw, diFlags);
  LEAVE_CRITICAL();
  
  END_NTUSER();
}

BOOL STDCALL
NtUserEndPaint(HWND hWnd, CONST PAINTSTRUCT* lPs)
{
  PAINTSTRUCT SafePs;
  NTUSER_USER_OBJECT(WINDOW, Window);
  BEGIN_BUFFERS();
  BEGIN_NTUSER(BOOL, FALSE);
  
  /* Apparently windows always sets the last error to ERROR_NOACCESS in case it can't copy
     back the buffer */
  NTUSER_COPY_BUFFER_W32ERROR(&SafePs, lPs, sizeof(PAINTSTRUCT), ERROR_NOACCESS);
  
  ENTER_CRITICAL_SHARED();
  if(hWnd != NULL)
  {
    VALIDATE_USER_OBJECT(WINDOW, hWnd, Window);
  }
  else
  {
    if(!(Window = IntGetDesktopWindow()))
    {
      LEAVE_CRITICAL();
      DPRINT1("GetWindowDC(): Unable to get desktop window!\n");
      NTUSER_FAIL_ERROR(ERROR_ACCESS_DENIED);
    }
  }
  
  Result = IntEndPaint(Window, &SafePs);
  
  LEAVE_CRITICAL();
  
  END_NTUSER();
}

HICON STDCALL
NtUserFindExistingCursorIcon(HMODULE hModule,
                             HRSRC hRsrc,
                             LONG cx,
                             LONG cy)
{
  NTUSER_USER_OBJECT(CURSOR, Cursor);
  BEGIN_NTUSER(HICON, (HICON)0);
  
  ENTER_CRITICAL_SHARED();
  Cursor = IntFindExistingCursorObject(hModule, hRsrc, cx, cy);
  if(Cursor == NULL)
  {
     LEAVE_CRITICAL();
     NTUSER_FAIL_ERROR(0);
  }
  Result = Cursor->Handle;
  LEAVE_CRITICAL();
  
  END_NTUSER();
}

HWND STDCALL
NtUserGetActiveWindow(VOID)
{
  NTUSER_USER_OBJECT(WINDOW, Window);
  BEGIN_NTUSER_NOERR(HWND);
  
  ENTER_CRITICAL_SHARED();
  Window = IntGetActiveWindow();
  Result = (Window != NULL ? Window->Handle : NULL);
  LEAVE_CRITICAL();
  
  END_NTUSER_NOERR();
}

HWND STDCALL
NtUserGetAncestor(HWND hWnd, UINT Type)
{
  NTUSER_USER_OBJECT(WINDOW, Window);
  NTUSER_USER_OBJECT(WINDOW, Ancestor);
  BEGIN_NTUSER(HWND, NULL);
  
  switch(Type)
  {
    case GA_PARENT:
    case GA_ROOT:
    case GA_ROOTOWNER:
    {
      ENTER_CRITICAL_SHARED();
      VALIDATE_USER_OBJECT(WINDOW, hWnd, Window);
      Ancestor = IntGetAncestor(Window, Type);
      Result = (Ancestor != NULL ? Ancestor->Handle : NULL);
      LEAVE_CRITICAL();
      break;
    }
    default:
    {
      NTUSER_FAIL_ERROR(ERROR_INVALID_PARAMETER);
      break;
    }
  }
  
  END_NTUSER();
}

HWND STDCALL
NtUserGetCapture(VOID)
{
  NTUSER_USER_OBJECT(WINDOW, Window);
  BEGIN_NTUSER_NOERR(HWND);
  
  ENTER_CRITICAL_SHARED();
  Window = IntGetCaptureWindow();
  Result = (Window != NULL ? Window->Handle : NULL);
  LEAVE_CRITICAL();
  
  END_NTUSER_NOERR();
}

RTL_ATOM STDCALL
NtUserGetClassInfo(HINSTANCE hInstance, LPCWSTR lpClassName, LPWNDCLASSEXW lpWndClassEx,
                   BOOL Ansi, DWORD unknown3)
{
  UNICODE_STRING ClassName;
  BEGIN_NTUSER(RTL_ATOM, 0);
  
  if(!IS_ATOM(lpClassName))
  {
    /* FIXME - probe string */
    #if 1
    if(*lpClassName == L'\0') NTUSER_FAIL();
    #endif
    RtlInitUnicodeString(&ClassName, lpClassName);
  }
  else
  {
    ClassName.Length = 0;
    ClassName.MaximumLength = 0;
    ClassName.Buffer = (LPWSTR)lpClassName;
  }
  
  /* FIXME - probe lpWndClassEx structure */
  
  ENTER_CRITICAL_SHARED();
  Result = IntGetClassInfo(hInstance, &ClassName, lpWndClassEx, Ansi);
  LEAVE_CRITICAL();
  
  END_NTUSER();
}

DWORD STDCALL
NtUserGetClassLong(HWND hWnd, INT Offset, BOOL Ansi)
{
  NTUSER_USER_OBJECT(WINDOW, Window);
  BEGIN_NTUSER(DWORD, 0);
  
  ENTER_CRITICAL_SHARED();
  VALIDATE_USER_OBJECT(WINDOW, hWnd, Window);
  Result = IntGetClassLong(Window, Offset, Ansi);
  LEAVE_CRITICAL();
  
  END_NTUSER();
}

BOOL STDCALL
NtUserGetClientOrigin(HWND hWnd, LPPOINT Point)
{
  POINT ClientOrigin;
  NTUSER_USER_OBJECT(WINDOW, Window);
  BEGIN_BUFFERS();
  BEGIN_NTUSER(BOOL, FALSE);
  
  NTUSER_FAIL_INVALID_PARAMETER(Point, NULL);
  
  ENTER_CRITICAL_SHARED();
  if(hWnd != NULL)
  {
    VALIDATE_USER_OBJECT(WINDOW, hWnd, Window);
  }
  Result = IntGetClientOrigin((hWnd != NULL ? Window : NULL), &ClientOrigin);
  LEAVE_CRITICAL();
  
  NTUSER_COPY_BUFFER_BACK_NTERROR(Point, &ClientOrigin, sizeof(POINT));
  
  END_NTUSER();
}

BOOL STDCALL
NtUserGetClientRect(HWND hWnd, LPRECT Rect)
{
  RECT ClientRect;
  NTUSER_USER_OBJECT(WINDOW, Window);
  BEGIN_BUFFERS();
  BEGIN_NTUSER(BOOL, FALSE);
  
  NTUSER_FAIL_INVALID_PARAMETER(Rect, NULL);
  
  ENTER_CRITICAL_SHARED();
  VALIDATE_USER_OBJECT(WINDOW, hWnd, Window);
  IntGetClientRect(Window, &ClientRect);
  LEAVE_CRITICAL();
  
  NTUSER_COPY_BUFFER_BACK_NTERROR(Rect, &ClientRect, sizeof(RECT));
  
  END_NTUSER();
}

BOOL STDCALL
NtUserGetClipCursor(RECT *lpRect)
{
  RECT SafeRect;
  BEGIN_BUFFERS();
  BEGIN_NTUSER(BOOL, FALSE);
  
  NTUSER_FAIL_INVALID_PARAMETER(lpRect, NULL);
  
  ENTER_CRITICAL_SHARED();
  IntGetClipCursor(&SafeRect);
  LEAVE_CRITICAL();
  
  NTUSER_COPY_BUFFER_BACK_NTERROR(lpRect, &SafeRect, sizeof(RECT));
  
  END_NTUSER();
}

BOOL STDCALL
NtUserGetCursorIconInfo(HANDLE Handle,
                        PICONINFO IconInfo)
{
  ICONINFO SafeInfo;
  NTUSER_USER_OBJECT(CURSOR, Cursor);
  BEGIN_BUFFERS();
  BEGIN_NTUSER(BOOL, FALSE);
  
  NTUSER_FAIL_INVALID_PARAMETER(IconInfo, NULL);
  
  ENTER_CRITICAL_SHARED();
  VALIDATE_USER_OBJECT(CURSOR, Handle, Cursor);
  IntGetCursorIconInfo(Cursor, &SafeInfo);
  LEAVE_CRITICAL();
  
  NTUSER_COPY_BUFFER_BACK_NTERROR(IconInfo, &SafeInfo, sizeof(ICONINFO));
  
  Result = TRUE;
  
  END_NTUSER();
}

BOOL STDCALL
NtUserGetCursorIconSize(HANDLE Handle,
                        BOOL *fIcon,
                        SIZE *Size)
{
  BOOL SafeIsIcon;
  SIZE SafeSize;
  NTUSER_USER_OBJECT(CURSOR, Cursor);
  BEGIN_BUFFERS();
  BEGIN_NTUSER(BOOL, FALSE);
  
  /* FIXME - Does windows check these parameters, too? */
  NTUSER_FAIL_INVALID_PARAMETER(fIcon, NULL);
  NTUSER_FAIL_INVALID_PARAMETER(Size, NULL);
  
  ENTER_CRITICAL_SHARED();
  VALIDATE_USER_OBJECT(CURSOR, Handle, Cursor);
  Result = IntGetCursorIconSize(Cursor, &SafeIsIcon, &SafeSize);
  LEAVE_CRITICAL();
  
  /* Copy back the umode */
  NTUSER_COPY_BUFFER_BACK_NTERROR(fIcon, &SafeIsIcon, sizeof(BOOL));
  NTUSER_COPY_BUFFER_BACK_NTERROR(Size, &SafeSize, sizeof(SIZE));
  
  END_NTUSER();
}

BOOL STDCALL
NtUserGetCursorInfo(
  PCURSORINFO pci)
{
  CURSORINFO SafeCI;
  BEGIN_BUFFERS();
  BEGIN_NTUSER(BOOL, FALSE);
  
  NTUSER_FAIL_INVALID_PARAMETER(pci, NULL);
  NTUSER_FAIL_INVALID_STRUCT(CURSORINFO, pci, cbSize);
  
  ENTER_CRITICAL_SHARED();
  Result = IntGetCursorInfo(&SafeCI);
  LEAVE_CRITICAL();
  
  NTUSER_COPY_BUFFER_BACK_NTERROR(pci, &SafeCI, sizeof(CURSORINFO));
  
  END_NTUSER();
}

HDC STDCALL
NtUserGetDC(HWND hWnd)
{
  NTUSER_USER_OBJECT(WINDOW, Window);
  BEGIN_NTUSER(HDC, NULL);
  
  ENTER_CRITICAL_SHARED();
  if(hWnd != NULL)
  {
    VALIDATE_USER_OBJECT(WINDOW, hWnd, Window);
  }
  else
  {
    if(!(Window = IntGetDesktopWindow()))
    {
      LEAVE_CRITICAL();
      DPRINT1("GetWindowDC(): Unable to get desktop window (process 0x%x)!\n", PsGetCurrentProcessId());
      NTUSER_FAIL_ERROR(ERROR_ACCESS_DENIED);
    }
  }
  
  Result = IntGetDCEx(Window, NULL, (hWnd == NULL ? DCX_CACHE | DCX_WINDOW : DCX_USESTYLE));
  
  LEAVE_CRITICAL();
  
  END_NTUSER();
}

HDC STDCALL
NtUserGetDCEx(HWND hWnd, HANDLE ClipRegion, ULONG Flags)
{
  NTUSER_USER_OBJECT(WINDOW, Window);
  BEGIN_NTUSER(HDC, NULL);
  
  ENTER_CRITICAL_SHARED();
  if(hWnd != NULL)
  {
    VALIDATE_USER_OBJECT(WINDOW, hWnd, Window);
  }
  else
  {
    if(!(Window = IntGetDesktopWindow()))
    {
      LEAVE_CRITICAL();
      DPRINT1("GetWindowDC(): Unable to get desktop window!\n");
      NTUSER_FAIL_ERROR(ERROR_ACCESS_DENIED);
    }
  }
  
  Result = IntGetDCEx(Window, ClipRegion, Flags);
  
  LEAVE_CRITICAL();
  
  END_NTUSER();
}

HWND STDCALL
NtUserGetDesktopWindow()
{
  NTUSER_USER_OBJECT(WINDOW, Window);
  BEGIN_NTUSER_NOERR(HWND);
  
  ENTER_CRITICAL_SHARED();
  Window = IntGetDesktopWindow();
  Result = (Window != NULL ? Window->Handle : NULL);
  LEAVE_CRITICAL();
  
  END_NTUSER_NOERR();
}

HWND STDCALL
NtUserGetForegroundWindow(VOID)
{
  NTUSER_USER_OBJECT(WINDOW, Window);
  BEGIN_NTUSER_NOERR(HWND);
  
  ENTER_CRITICAL_SHARED();
  Window = IntGetForegroundWindow();
  Result = (Window != NULL ? Window->Handle : NULL);
  LEAVE_CRITICAL();
  
  END_NTUSER_NOERR();
}

BOOL STDCALL
NtUserGetMessage(PNTUSERGETMESSAGEINFO UnsafeInfo,
		 HWND Wnd,
		 UINT MsgFilterMin,
		 UINT MsgFilterMax)
{
  NTUSERGETMESSAGEINFO Info;
  USER_MESSAGE Msg;
  BOOL GotMessage;
  NTUSER_USER_OBJECT(WINDOW, Window);
  BEGIN_BUFFERS();
  BEGIN_NTUSER(BOOL, FALSE);
  
  NTUSER_FAIL_INVALID_PARAMETER(UnsafeInfo, NULL);
  
  /* Fix the filter */
  if(MsgFilterMax < MsgFilterMin)
  {
    MsgFilterMin = 0;
    MsgFilterMax = 0;
  }
  
  GotMessage = FALSE;
  
  ENTER_CRITICAL_SHARED();
  if(Wnd != NULL)
  {
    VALIDATE_USER_OBJECT(WINDOW, Wnd, Window);
    /* reference the window so we make sure the object is still valid even
       if it has been deleted while waiting for messages */
    ObmReferenceObject(Window);
  }
  
  /* We can't hold the global lock in here, otherwise other threads would hang */
  LEAVE_CRITICAL();
  
  /* Peek the message from the queue */
  Result = IntGetMessage(&Msg, Window, MsgFilterMin, MsgFilterMax, &GotMessage);
  
  if(Window != NULL)
  {
    /* dereference the window */
    ObmDereferenceObject(Window);
  }
  
  if(GotMessage)
  {
    PMSGMEMORY MsgMemoryEntry;
    UINT Size;
    PVOID UserMem;
    NTSTATUS Status;
    
    MsgCopyKMsgToMsg(&(Info.Msg), &(Msg.Msg));
    DbgPrint("GotMessage for window 0x%x (h: 0x%x)\n", Msg.Msg.Window, (Msg.Msg.Window ? Msg.Msg.Window->Handle : NULL));
    
    /* See if this message type is present in the table */
    MsgMemoryEntry = FindMsgMemory(Info.Msg.message);
    if (NULL == MsgMemoryEntry)
    {
      /* Not present, no copying needed */
      Info.LParamSize = 0;
    }
    else
    {
      /* Determine required size */
      Size = MsgMemorySize(MsgMemoryEntry, Info.Msg.wParam,
                           Info.Msg.lParam);
      /* Allocate required amount of user-mode memory */
      Info.LParamSize = Size;
      UserMem = NULL;
      Status = ZwAllocateVirtualMemory(NtCurrentProcess(), &UserMem, 0,
                                       &Info.LParamSize, MEM_COMMIT, PAGE_READWRITE);
      
      if(!NT_SUCCESS(Status))
      {
        ErrorResult = (BOOL)-1;
	NTUSER_FAIL_NTERROR(Status);
      }
      /* Transfer lParam data to user-mode mem */
      Status = MmCopyToCaller(UserMem, (PVOID) Info.Msg.lParam, Size);
      if(!NT_SUCCESS(Status))
      {
        ZwFreeVirtualMemory(NtCurrentProcess(), (PVOID *) &UserMem,
                            &Info.LParamSize, MEM_DECOMMIT);
        ErrorResult = (BOOL)-1;
	NTUSER_FAIL_NTERROR(Status);
      }
      Info.Msg.lParam = (LPARAM) UserMem;
    }
    if(Msg.FreeLParam && 0 != Msg.Msg.lParam)
    {
      ExFreePool((void *) Msg.Msg.lParam);
    }
    
    /* Copy back the UnsafeInfo structure */
    NTUSER_COPY_BUFFER_BACK_NTERROR(UnsafeInfo, &Info, sizeof(NTUSERGETMESSAGEINFO));
  }
  
  END_NTUSER(); 
}

HWND STDCALL
NtUserGetParent(HWND hWnd)
{
  NTUSER_USER_OBJECT(WINDOW, Window);
  BEGIN_NTUSER(HWND, NULL);
  
  ENTER_CRITICAL_SHARED();
  VALIDATE_USER_OBJECT(WINDOW, hWnd, Window);
  Result = (Window->Parent != NULL ? Window->Parent->Handle : NULL);
  LEAVE_CRITICAL();
  
  END_NTUSER();
}

HANDLE STDCALL
NtUserGetProp(HWND hWnd, ATOM Atom)
{
  PPROPERTY Property;
  NTUSER_USER_OBJECT(WINDOW, Window);
  BEGIN_NTUSER(HANDLE, NULL);

  ENTER_CRITICAL_SHARED();
  VALIDATE_USER_OBJECT(WINDOW, hWnd, Window);
  Property = IntGetProp(Window, Atom);
  Result = (Property != NULL ? Property->Data : NULL);
  LEAVE_CRITICAL();

  END_NTUSER();
}

HWND STDCALL
NtUserGetShellWindow()
{
  NTUSER_USER_OBJECT(WINDOW, ShellWindow);
  BEGIN_NTUSER_NOERR(HWND);
  
  ENTER_CRITICAL_SHARED();
  ShellWindow = IntGetShellWindow();
  Result = (ShellWindow != NULL ? ShellWindow->Handle : NULL);
  LEAVE_CRITICAL();
  
  END_NTUSER_NOERR();
}

INT STDCALL
NtUserGetSystemMetrics(INT Index)
{
  BEGIN_NTUSER(INT, 0);
  
  if(Index < 0)
  {
    /* windows doesn't set a LastError in this case */
    NTUSER_FAIL();
  }
  
  ENTER_CRITICAL_SHARED();
  Result = IntGetSystemMetrics(Index);
  LEAVE_CRITICAL();
   
  END_NTUSER();
}

BOOL STDCALL
NtUserGetUpdateRect(HWND hWnd, LPRECT lpRect, BOOL bErase)
{
  RECT SafeRect;
  NTUSER_USER_OBJECT(WINDOW, Window);
  BEGIN_BUFFERS();
  BEGIN_NTUSER(BOOL, FALSE);
  
  /* FIXME - if bErase == FALSE, should we do ENTER_CRITICAL_SHARED() instead? */
  ENTER_CRITICAL_SHARED();
  VALIDATE_USER_OBJECT(WINDOW, hWnd, Window);
  Result = IntGetUpdateRect(Window, &SafeRect, bErase);
  LEAVE_CRITICAL();
  
  if(lpRect != NULL)
  {
    NTUSER_COPY_BUFFER_BACK_NTERROR(lpRect, &SafeRect, sizeof(RECT));
  }
  
  END_NTUSER();
}

INT STDCALL
NtUserGetUpdateRgn(HWND hWnd, HRGN hRgn, BOOL bErase)
{
  NTUSER_USER_OBJECT(WINDOW, Window);
  BEGIN_NTUSER(INT, ERROR);
  
  /* FIXME - if bErase == FALSE, should we do ENTER_CRITICAL_SHARED() instead? */
  ENTER_CRITICAL_SHARED();
  VALIDATE_USER_OBJECT(WINDOW, hWnd, Window);
  Result = IntGetUpdateRgn(Window, hRgn, bErase);
  LEAVE_CRITICAL();
  
  END_NTUSER();
}

HWND STDCALL
NtUserGetWindow(HWND hWnd, UINT uCmd)
{
  NTUSER_USER_OBJECT(WINDOW, Window);
  NTUSER_USER_OBJECT(WINDOW, Relative);
  BEGIN_NTUSER(HWND, NULL);
  
  ENTER_CRITICAL_SHARED();
  VALIDATE_USER_OBJECT(WINDOW, hWnd, Window);
  Relative = IntGetWindow(Window, uCmd);
  Result = (Relative != NULL ? Relative->Handle : NULL);
  LEAVE_CRITICAL();
  
  END_NTUSER();
}

HDC STDCALL
NtUserGetWindowDC(HWND hWnd)
{
  NTUSER_USER_OBJECT(WINDOW, Window);
  BEGIN_NTUSER(HDC, NULL);
  
  ENTER_CRITICAL_SHARED();
  if(hWnd != NULL)
  {
    VALIDATE_USER_OBJECT(WINDOW, hWnd, Window);
  }
  else
  {
    if(!(Window = IntGetDesktopWindow()))
    {
      LEAVE_CRITICAL();
      DPRINT1("GetWindowDC(): Unable to get desktop window!\n");
      NTUSER_FAIL_ERROR(ERROR_ACCESS_DENIED);
    }
  }
  
  Result = IntGetDCEx(Window, NULL, DCX_USESTYLE | DCX_WINDOW);
  
  LEAVE_CRITICAL();
  
  END_NTUSER();
}

LONG STDCALL
NtUserGetWindowLong(HWND hWnd, INT Index, BOOL Ansi)
{
  NTUSER_USER_OBJECT(WINDOW, Window);
  BEGIN_NTUSER(LONG, 0);
  
  ENTER_CRITICAL_SHARED();
  VALIDATE_USER_OBJECT(WINDOW, hWnd, Window);
  Result = IntGetWindowLong(Window, Index, Ansi);
  LEAVE_CRITICAL();
  
  END_NTUSER();
}

BOOL STDCALL
NtUserGetWindowRect(HWND hWnd, LPRECT Rect)
{
  RECT SafeRect;
  NTUSER_USER_OBJECT(WINDOW, Window);
  BEGIN_BUFFERS();
  BEGIN_NTUSER(BOOL, FALSE);
  
  NTUSER_FAIL_INVALID_PARAMETER(Rect, NULL);
  
  ENTER_CRITICAL_SHARED();
  VALIDATE_USER_OBJECT(WINDOW, hWnd, Window);
  SafeRect = Window->WindowRect;
  LEAVE_CRITICAL();
  
  NTUSER_COPY_BUFFER_BACK_NTERROR(Rect, &SafeRect, sizeof(RECT));
  
  END_NTUSER();
}

DWORD STDCALL
NtUserGetWindowThreadProcessId(HWND hWnd, LPDWORD UnsafePid)
{
  ULONG SafePid;
  NTUSER_USER_OBJECT(WINDOW, Window);
  BEGIN_BUFFERS();
  BEGIN_NTUSER(DWORD, 0);
  
  ENTER_CRITICAL_SHARED();
  VALIDATE_USER_OBJECT(WINDOW, hWnd, Window);
  Result = (DWORD)IntGetWindowThreadProcessId(Window, &SafePid);
  LEAVE_CRITICAL();
  
  if(UnsafePid != NULL)
  {
    NTUSER_COPY_BUFFER_BACK_NTERROR(UnsafePid, &SafePid, sizeof(DWORD));
  }
  
  END_NTUSER();
}

INT STDCALL
NtUserInternalGetWindowText(HWND hWnd, LPWSTR lpString, INT nMaxCount)
{
  NTUSER_USER_OBJECT(WINDOW, Window);
  BEGIN_NTUSER(INT, 0);
  
  if(lpString && (nMaxCount <= 1))
  {
    NTUSER_FAIL_ERROR(ERROR_INVALID_PARAMETER);
  }
  
  /* FIXME - probe the buffer lpString points to */
  
  ENTER_CRITICAL_SHARED();
  VALIDATE_USER_OBJECT(WINDOW, hWnd, Window);
  Result = IntInternalGetWindowText(Window, lpString, nMaxCount);
  LEAVE_CRITICAL();
  
  END_NTUSER();
}

BOOL STDCALL
NtUserInvalidateRect(HWND hWnd, CONST RECT *Rect, BOOL Erase)
{
  RECT SafeRect;
  NTUSER_USER_OBJECT(WINDOW, Window);
  BEGIN_BUFFERS();
  BEGIN_NTUSER(BOOL, FALSE);
  
  if(Rect != NULL)
  {
    NTUSER_COPY_BUFFER_NTERROR(&SafeRect, Rect, sizeof(RECT));
  }
  
  ENTER_CRITICAL();
  if(hWnd != NULL)
  {
    VALIDATE_USER_OBJECT(WINDOW, hWnd, Window);
  }
  
  Result = IntInvalidateRect(Window, (Rect != NULL ? &SafeRect : NULL), Erase);
  
  LEAVE_CRITICAL();
  
  END_NTUSER();
}

BOOL STDCALL
NtUserInvalidateRgn(HWND hWnd, HRGN Rgn, BOOL Erase)
{
  NTUSER_USER_OBJECT(WINDOW, Window);
  BEGIN_NTUSER(BOOL, FALSE);
  
  ENTER_CRITICAL();
  if(hWnd != NULL)
  {
    VALIDATE_USER_OBJECT(WINDOW, hWnd, Window);
  }
  
  Result = IntInvalidateRgn(Window, Rgn, Erase);
  
  LEAVE_CRITICAL();
  
  END_NTUSER();
}

BOOL STDCALL
NtUserPaintDesktop(HDC hDC)
{
  BEGIN_NTUSER_NOERR(BOOL);
  
  ENTER_CRITICAL_SHARED();
  Result = IntPaintDesktop(hDC);
  LEAVE_CRITICAL();
  
  END_NTUSER_NOERR();
}

BOOL STDCALL
NtUserPeekMessage(PNTUSERGETMESSAGEINFO UnsafeInfo,
                  HWND hWnd,
                  UINT MsgFilterMin,
                  UINT MsgFilterMax,
                  UINT RemoveMsg)
{
  NTUSERGETMESSAGEINFO Info;
  USER_MESSAGE Msg;
  
  NTUSER_USER_OBJECT(WINDOW, Window);
  BEGIN_BUFFERS();
  BEGIN_NTUSER(BOOL, FALSE);
  
  NTUSER_FAIL_INVALID_PARAMETER(UnsafeInfo, NULL);
  
  /* Fix the filter */
  if(MsgFilterMax < MsgFilterMin)
  {
    MsgFilterMin = 0;
    MsgFilterMax = 0;
  }
  
  ENTER_CRITICAL_SHARED();
  if(hWnd != NULL)
  {
    VALIDATE_USER_OBJECT(WINDOW, hWnd, Window);
    /* reference the window so we make sure the object is still valid even
       if it has been deleted while waiting for messages */
    ObmReferenceObject(Window);
  }
  
  /* We can't hold the global lock in here, otherwise other threads would hang */
  LEAVE_CRITICAL();
  
  /* Peek the message from the queue */
  Result = IntPeekMessage(&Msg, Window, MsgFilterMin, MsgFilterMax, RemoveMsg);
  
  if(Window != NULL)
  {
    /* dereference the window */
    ObmDereferenceObject(Window);
  }
  
  if (Result)
  {
    PMSGMEMORY MsgMemoryEntry;
    UINT Size;
    PVOID UserMem;
    NTSTATUS Status;
    
    MsgCopyKMsgToMsg(&(Info.Msg), &(Msg.Msg));
    
    /* See if this message type is present in the table */
    MsgMemoryEntry = FindMsgMemory(Info.Msg.message);
    if (NULL == MsgMemoryEntry)
    {
      /* Not present, no copying needed */
      Info.LParamSize = 0;
    }
    else
    {
      /* Determine required size */
      Size = MsgMemorySize(MsgMemoryEntry, Info.Msg.wParam, Info.Msg.lParam);
      /* Allocate required amount of user-mode memory */
      Info.LParamSize = Size;
      UserMem = NULL;
      Status = ZwAllocateVirtualMemory(NtCurrentProcess(), &UserMem, 0,
                                       &Info.LParamSize, MEM_COMMIT, PAGE_READWRITE);
      if(!NT_SUCCESS(Status))
      {
        ErrorResult = (BOOL)-1;
	NTUSER_FAIL_NTERROR(Status);
      }
      /* Transfer lParam data to user-mode mem */
      Status = MmCopyToCaller(UserMem, (PVOID) Info.Msg.lParam, Size);
      if(!NT_SUCCESS(Status))
      {
        ZwFreeVirtualMemory(NtCurrentProcess(), (PVOID *) &UserMem,
                            &Info.LParamSize, MEM_DECOMMIT);
        ErrorResult = (BOOL)-1;
        NTUSER_FAIL_NTERROR(Status);
      }
      Info.Msg.lParam = (LPARAM) UserMem;
    }
    if(Msg.FreeLParam && 0 != Msg.Msg.lParam)
    {
      ExFreePool((void *) Msg.Msg.lParam);
    }
    
    /* Copy back the UnsafeInfo structure */
    NTUSER_COPY_BUFFER_BACK_NTERROR(UnsafeInfo, &Info, sizeof(NTUSERGETMESSAGEINFO));
  }
  
  END_NTUSER();
}

BOOL STDCALL
NtUserPostMessage(HWND Wnd,
		  UINT Msg,
		  WPARAM wParam,
		  LPARAM lParam)
{
  NTUSER_USER_OBJECT(WINDOW, Window);
  BEGIN_NTUSER(LRESULT, 0);

  /* As opposed to NtUserSendMessage(), NtUserPostMessage() handles HWND_BROADCAST in kmode! */

  if(Wnd != NULL || Wnd == HWND_BROADCAST)
  {
    /* Post a single mesage */
    
    ENTER_CRITICAL_SHARED();
    if(Wnd != HWND_BROADCAST)
    {
      VALIDATE_USER_OBJECT(WINDOW, Wnd, Window);
    }
    Result = IntPostMessage(Window,
                            Msg,
                            wParam,
                            lParam);
    LEAVE_CRITICAL();
  }
  else
  {
    /* According to PSDK it behaves the same way as PostThreadMessage() if no
       window is specified. */

    ENTER_CRITICAL_SHARED();
    Result = IntPostThreadMessage(PsGetWin32Thread(),
                                  Msg,
                                  wParam,
                                  lParam);
    LEAVE_CRITICAL();
  }

  END_NTUSER();
}

BOOL STDCALL
NtUserPostThreadMessage(DWORD idThread,
			UINT Msg,
			WPARAM wParam,
			LPARAM lParam)
{
  NTSTATUS Status;
  PETHREAD peThread;
  BEGIN_NTUSER(BOOL, FALSE);
  
  /* lookup the thread */
  Status = PsLookupThreadByThreadId((PVOID)idThread, &peThread);
  if(!NT_SUCCESS(Status))
  {
    NTUSER_FAIL_NTERROR(Status);
  }
  
  if(peThread->Win32Thread == NULL)
  {
    /* we try to send a message to a non-win32 thread... */
    ObDereferenceObject(peThread);
    NTUSER_FAIL_ERROR(ERROR_ACCESS_DENIED); /* FIXME - right error code?! */
  }
  
  /* FIXME - fail if the thread is exiting */
  /* FIXME - fail if the thread does not belong to the same desktop or to a process
             that doesn't have the same local unique identifier (LUID) as the
             calling process. set the last error code to ERROR_INVALID_THREAD_ID */
  
  ENTER_CRITICAL_SHARED();
  Result = IntPostThreadMessage(peThread->Win32Thread,
                                Msg,
                                wParam,
                                lParam);
  LEAVE_CRITICAL();
  
  ObDereferenceObject(peThread);
  END_NTUSER();
}

BOOL STDCALL
NtUserRedrawWindow(HWND hWnd, CONST RECT *lprcUpdate, HRGN hrgnUpdate,
   UINT flags)
{
  NTUSER_USER_OBJECT(WINDOW, Window);
  BEGIN_NTUSER(BOOL, FALSE);
  
  /* FIXME - probe lprcUpdate! */
  
  ENTER_CRITICAL_SHARED();
  if(hWnd != NULL)
  {
    VALIDATE_USER_OBJECT(WINDOW, hWnd, Window);
  }
  else
  {
    if(!(Window = IntGetDesktopWindow()))
    {
      LEAVE_CRITICAL();
      DPRINT1("RedrawWindow(): Unable to get desktop window!\n");
      NTUSER_FAIL_ERROR(ERROR_ACCESS_DENIED);
    }
  }
  Result = IntRedrawWindow(Window, (LPRECT)lprcUpdate, hrgnUpdate, flags);
  LEAVE_CRITICAL();
  
  END_NTUSER();
}

RTL_ATOM STDCALL
NtUserRegisterClassExWOW(CONST WNDCLASSEXW* lpwcx,
                         PUNICODE_STRING ClassName,
                         PUNICODE_STRING ClassNameCopy,
                         PUNICODE_STRING MenuName,
                         WNDPROC wpExtra, /* FIXME: Windows uses this parameter for something different. */
                         DWORD Flags,
                         DWORD Unknown7)
{
  WNDCLASSEXW SafeClass;
  UNICODE_STRING SafeClassName, SafeMenuName;
  NTUSER_USER_OBJECT(CLASS, Class);
  BEGIN_BUFFERS();
  BEGIN_NTUSER(RTL_ATOM, (RTL_ATOM)0);
  
  NTUSER_FAIL_INVALID_PARAMETER(lpwcx, NULL);
  NTUSER_FAIL_INVALID_PARAMETER(ClassName, NULL);
  NTUSER_FAIL_INVALID_PARAMETER(MenuName, NULL);
  
  if(Flags & ~REGISTERCLASS_ALL)
  {
    NTUSER_FAIL_ERROR(ERROR_INVALID_FLAGS);
  }
  
  NTUSER_COPY_BUFFER_NTERROR(&SafeClass, lpwcx, sizeof(WNDCLASSEXW));
  NTUSER_COPY_BUFFER_NTERROR(&SafeClassName, ClassName, sizeof(UNICODE_STRING));
  NTUSER_COPY_BUFFER_NTERROR(&SafeMenuName, MenuName, sizeof(UNICODE_STRING));
  
   /* Deny negative sizes */
  if(SafeClass.cbClsExtra < 0 || SafeClass.cbWndExtra < 0)
  {
    NTUSER_FAIL_ERROR(ERROR_INVALID_PARAMETER);
  }
  
  /* copy the class name string */
  if(!IS_ATOM(SafeClassName.Buffer))
  {
    NtStatus = IntSafeCopyUnicodeString(&SafeClassName, ClassName);
    if(!NT_SUCCESS(NtStatus))
    {
      NTUSER_FAIL_NTERROR(NtStatus);
    }
  }
  
  /* copy the menu name string */
  if(!IS_ATOM(SafeMenuName.Buffer))
  {
    NtStatus = IntSafeCopyUnicodeString(&SafeMenuName, MenuName);
    if(!NT_SUCCESS(NtStatus))
    {
      if(!IS_ATOM(SafeClassName.Buffer))
      {
        RtlFreeUnicodeString(&SafeClassName);
      }
      NTUSER_FAIL_NTERROR(NtStatus);
    }
  }
  
  ENTER_CRITICAL();
  Class = IntRegisterClass(&SafeClass, &SafeClassName, &SafeMenuName, wpExtra, Flags);
  Result = (Class != NULL ? Class->Atom : (RTL_ATOM)0);
  LEAVE_CRITICAL();
  
  if(!IS_ATOM(SafeMenuName.Buffer))
  {
    RtlFreeUnicodeString(&SafeMenuName);
  }
  if(!IS_ATOM(SafeMenuName.Buffer))
  {
    RtlFreeUnicodeString(&SafeMenuName);
  }
  
  END_NTUSER();
}

INT STDCALL
NtUserReleaseDC(HWND hWnd, HDC hDc)
{
  NTUSER_USER_OBJECT(WINDOW, Window);
  BEGIN_NTUSER(INT, 0);
  
  ENTER_CRITICAL_SHARED();
  if(hWnd != NULL)
  {
    VALIDATE_USER_OBJECT(WINDOW, hWnd, Window);
  }
  else
  {
    if(!(Window = IntGetDesktopWindow()))
    {
      LEAVE_CRITICAL();
      DPRINT1("GetWindowDC(): Unable to get desktop window!\n");
      NTUSER_FAIL_ERROR(ERROR_ACCESS_DENIED);
    }
  }
  
  Result = IntReleaseDC(Window, hDc);
  
  LEAVE_CRITICAL();
  
  END_NTUSER();
}

HANDLE STDCALL
NtUserRemoveProp(HWND hWnd, ATOM Atom)
{
  NTUSER_USER_OBJECT(WINDOW, Window);
  BEGIN_NTUSER(HANDLE, NULL);

  ENTER_CRITICAL();
  VALIDATE_USER_OBJECT(WINDOW, hWnd, Window);
  if(!IntRemoveProp(Window, Atom, &Result))
  {
    Result = NULL;
  }
  LEAVE_CRITICAL();

  END_NTUSER();
}

UINT STDCALL
NtUserSendInput(UINT nInputs,
                LPINPUT pInput,
                INT cbSize)
{
  BEGIN_NTUSER(UINT, 0);
  
  /* check parameters */
  NTUSER_FAIL_INVALID_PARAMETER(nInputs, 0);
  NTUSER_FAIL_INVALID_PARAMETER(pInput, NULL);
  NTUSER_FAIL_INVALID_PARAMETER(cbSize, sizeof(INPUT));
  
  /* FIXME - Probe the input buffer */
  
  /* we don't have to do this atomic, message queues are thread-safe */
  Result = IntSendInput(nInputs, pInput, cbSize);
   
  END_NTUSER();
}

LRESULT STDCALL
NtUserSendMessage(HWND Wnd,
		  UINT Msg,
		  WPARAM wParam,
		  LPARAM lParam,
                  PNTUSERSENDMESSAGEINFO UnsafeInfo)
{
  NTUSER_USER_OBJECT(WINDOW, Window);
  BEGIN_NTUSER(LRESULT, 0);

  /* don't handle HWND_BROADCAST in kmode, SendMessage() should obtain a list
     of handles and then call NtUserSendMessage() for each window */
  NTUSER_FAIL_INVALID_PARAMETER(Wnd, HWND_BROADCAST);
  NTUSER_FAIL_INVALID_PARAMETER(Wnd, 0);

  /* FIXME - probe UnsafeInfo */
  
  UnsafeInfo->HandledByKernel = TRUE;
  
  ENTER_CRITICAL_SHARED();

  VALIDATE_USER_OBJECT(WINDOW, Wnd, Window);
  if(Window->MessageQueue->Thread == PsGetCurrentThread())
  {
    /* return to user mode and call the window proc there */
    if(0xFFFF0000 != ((DWORD) Window->WndProcW & 0xFFFF0000))
    {
      if(0xFFFF0000 != ((DWORD) Window->WndProcA & 0xFFFF0000))
      {
        /* Both Unicode and Ansi winprocs are real, see what usermode prefers */
        UnsafeInfo->Proc = (UnsafeInfo->Ansi ? Window->WndProcA : Window->WndProcW);
      }
      else
      {
        /* Real Unicode winproc */
        UnsafeInfo->Ansi = FALSE;
        UnsafeInfo->Proc = Window->WndProcW;
      }
    }
    else
    {
      /* Must have real Ansi winproc */
      UnsafeInfo->Ansi = TRUE;
      UnsafeInfo->Proc = Window->WndProcA;
    }
    LEAVE_CRITICAL();

    UnsafeInfo->HandledByKernel = FALSE;
    return TRUE;
  }
  ObmReferenceObject(Window);
  LEAVE_CRITICAL();

  Result = IntSendMessage(Window,
                          Msg,
                          wParam,
                          lParam);
  ObmDereferenceObject(Window);
  
  END_NTUSER();
}

HWND STDCALL
NtUserSetActiveWindow(HWND hWnd)
{
  NTUSER_USER_OBJECT(WINDOW, Window);
  NTUSER_USER_OBJECT(WINDOW, PrevWindow);
  BEGIN_NTUSER(HWND, NULL);
  
  ENTER_CRITICAL();
  VALIDATE_USER_OBJECT(WINDOW, hWnd, Window);
  PrevWindow = IntSetActiveWindow(Window);
  Result = (PrevWindow != NULL ? PrevWindow->Handle : NULL);
  LEAVE_CRITICAL();
  
  END_NTUSER();
}

HWND STDCALL
NtUserSetCapture(HWND hWnd)
{
  NTUSER_USER_OBJECT(WINDOW, Window);
  NTUSER_USER_OBJECT(WINDOW, PrevWindow);
  BEGIN_NTUSER(HWND, NULL);
  
  ENTER_CRITICAL();
  VALIDATE_USER_OBJECT(WINDOW, hWnd, Window);
  PrevWindow = IntSetCapture(Window);
  Result = (PrevWindow != NULL ? PrevWindow->Handle : NULL);
  LEAVE_CRITICAL();
  
  END_NTUSER();
}

DWORD STDCALL
NtUserSetClassLong(HWND hWnd,
		   INT Offset,
		   LONG dwNewLong,
		   BOOL Ansi)
{
  NTUSER_USER_OBJECT(WINDOW, Window);
  BEGIN_NTUSER(DWORD, 0);
  
  ENTER_CRITICAL();
  VALIDATE_USER_OBJECT(WINDOW, hWnd, Window);
  Result = IntSetClassLong(Window, Offset, dwNewLong, Ansi);
  LEAVE_CRITICAL();
  
  END_NTUSER();
}

HCURSOR STDCALL
NtUserSetCursor(HCURSOR hCursor)
{
  NTUSER_USER_OBJECT(CURSOR, Cursor);
  NTUSER_USER_OBJECT(CURSOR, OldCursor);
  BEGIN_NTUSER(HCURSOR, (HCURSOR)0);
  
  ENTER_CRITICAL_SHARED();
  if(hCursor != (HCURSOR)0)
  {
    VALIDATE_USER_OBJECT(CURSOR, hCursor, Cursor);
  }
  OldCursor = IntSetCursor(Cursor, FALSE);
  if(OldCursor != NULL)
  {
    Result = OldCursor->Handle;
  }
  LEAVE_CRITICAL();
  
  END_NTUSER();
}

BOOL STDCALL
NtUserSetCursorIconData(HANDLE Handle,
                        PBOOL fIcon,
                        POINT *Hotspot,
                        HMODULE hModule,
                        HRSRC hRsrc,
                        HRSRC hGroupRsrc)
{
  BOOL SafefIcon;
  POINT SafeHotspot;
  BEGIN_BUFFERS();
  NTUSER_USER_OBJECT(CURSOR, Cursor);
  BEGIN_NTUSER(BOOL, FALSE);
  
  /* copy the buffers */
  if(fIcon != NULL)
  {
    NTUSER_COPY_BUFFER_NTERROR(&SafefIcon, fIcon, sizeof(BOOL));
  }
  if(Hotspot != NULL)
  {
    NTUSER_COPY_BUFFER_NTERROR(&SafeHotspot, Hotspot, sizeof(POINT));
  }
  
  ENTER_CRITICAL();
  VALIDATE_USER_OBJECT(CURSOR, Handle, Cursor);
  IntSetCursorIconData(Cursor,
                       (fIcon != NULL ? &SafefIcon : NULL),
                       (Hotspot != NULL ? &SafeHotspot : NULL),
                       hModule,
                       hRsrc,
                       hGroupRsrc);
  LEAVE_CRITICAL();
  
  Result = TRUE;
  
  END_NTUSER();
}

HWND STDCALL
NtUserSetFocus(HWND hWnd)
{
  NTUSER_USER_OBJECT(WINDOW, Window);
  NTUSER_USER_OBJECT(WINDOW, PreviousWindow);
  BEGIN_NTUSER(HWND, NULL);
  
  ENTER_CRITICAL();
  if(hWnd != NULL)
  {
    VALIDATE_USER_OBJECT(WINDOW, hWnd, Window);
  }
  PreviousWindow = IntSetFocus(Window);
  Result = (PreviousWindow != NULL ? PreviousWindow->Handle : NULL);
  LEAVE_CRITICAL();
  
  END_NTUSER();
}

HWND STDCALL
NtUserSetParent(HWND hWndChild, HWND hWndNewParent)
{
  NTUSER_USER_OBJECT(WINDOW, ChildWindow);
  NTUSER_USER_OBJECT(WINDOW, NewParentWindow);
  NTUSER_USER_OBJECT(WINDOW, PreviousParentWindow);
  NTUSER_USER_OBJECT(WINDOW, DesktopWindow);
  BEGIN_NTUSER(HWND, NULL);
  
  if(IntIsBroadcastHwnd(hWndChild) || IntIsBroadcastHwnd(hWndNewParent))
  {
    NTUSER_FAIL_ERROR(ERROR_INVALID_PARAMETER);
  }
  
  ENTER_CRITICAL();
  
  VALIDATE_USER_OBJECT(WINDOW, hWndChild, ChildWindow);
  
  /* You can't change the parent of the desktop window! */
  if(!(DesktopWindow = IntGetDesktopWindow()) ||
     ChildWindow == DesktopWindow)
  {
    LEAVE_CRITICAL();
    NTUSER_FAIL_ERROR(ERROR_ACCESS_DENIED);
  }
  
  switch((ULONG)hWndNewParent)
  {
    case 0:
      NewParentWindow = DesktopWindow;
      break;
    
    case (ULONG)HWND_MESSAGE:
      LEAVE_CRITICAL();
      DPRINT1("SetParent doesn't support HWND_MESSAGE yet!");
      NTUSER_FAIL_ERROR(ERROR_CALL_NOT_IMPLEMENTED);
      break;
    
    default:
      VALIDATE_USER_OBJECT(WINDOW, hWndNewParent, NewParentWindow);
      break;
  }
  
  PreviousParentWindow = IntSetParent(ChildWindow, NewParentWindow);
  Result = (PreviousParentWindow != NULL ? PreviousParentWindow->Handle : NULL);
  
  LEAVE_CRITICAL();
  
  END_NTUSER();
}

BOOL STDCALL
NtUserSetProp(HWND hWnd, ATOM Atom, HANDLE Data)
{
  NTUSER_USER_OBJECT(WINDOW, Window);
  BEGIN_NTUSER(BOOL, FALSE);
  
  ENTER_CRITICAL();
  VALIDATE_USER_OBJECT(WINDOW, hWnd, Window);
  Result = IntSetProp(Window, Atom, Data);
  LEAVE_CRITICAL();
  
  END_NTUSER();
}

BOOL STDCALL
NtUserSetShellWindowEx(HWND hwndShell, HWND hwndListView)
{
  NTUSER_USER_OBJECT(WINDOW, ShellWindow);
  NTUSER_USER_OBJECT(WINDOW, ShellListViewWindow);  
  BEGIN_NTUSER(BOOL, FALSE);
  
  /* FIXME - Shouldn't we handle the case that hwndShell and/or hwndListView == NULL? */
  
  ENTER_CRITICAL();
  VALIDATE_USER_OBJECT(WINDOW, hwndShell, ShellWindow);
  if(hwndListView != NULL)
  {
    VALIDATE_USER_OBJECT(WINDOW, hwndListView, ShellListViewWindow);
  }
  
  Result = IntSetShellWindowEx(ShellWindow, ShellListViewWindow);
  LEAVE_CRITICAL();
  
  END_NTUSER();
}

LONG STDCALL
NtUserSetWindowLong(HWND hWnd, INT Index, LONG NewValue, BOOL Ansi)
{
  NTUSER_USER_OBJECT(WINDOW, Window);
  BEGIN_NTUSER(LONG, 0);
  
  ENTER_CRITICAL();
  VALIDATE_USER_OBJECT(WINDOW, hWnd, Window);
  Result = IntSetWindowLong(Window, Index, NewValue, Ansi);
  LEAVE_CRITICAL();
  
  END_NTUSER();
}

BOOL STDCALL
NtUserSetWindowPos(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags)
{
  NTUSER_USER_OBJECT(WINDOW, Window);
  NTUSER_USER_OBJECT(WINDOW, InsertAfterWindow);
  BEGIN_NTUSER(BOOL, FALSE);
  
  ENTER_CRITICAL();
  VALIDATE_USER_OBJECT(WINDOW, hWnd, Window);
  if(hWndInsertAfter == HWND_BOTTOM)
    InsertAfterWindow = WINDOW_BOTTOM;
  else if(hWndInsertAfter == HWND_NOTOPMOST)
    InsertAfterWindow = WINDOW_NOTOPMOST;
  else if(hWndInsertAfter == HWND_TOP)
    InsertAfterWindow = WINDOW_TOP;
  else if(hWndInsertAfter == HWND_TOPMOST)
    InsertAfterWindow = WINDOW_TOPMOST;
  else if(hWndInsertAfter == HWND_MESSAGE)
    InsertAfterWindow = WINDOW_MESSAGE;
  else
  {
    VALIDATE_USER_OBJECT(WINDOW, hWndInsertAfter, InsertAfterWindow);
  }
  Result = WinPosSetWindowPos(Window, InsertAfterWindow, X, Y, cx, cy, uFlags);
  if(hWnd == (HWND)1)
  {
    DbgPrint("SetWindowPos(w): %i,%i,%i,%i\n", Window->WindowRect.left, Window->WindowRect.top, Window->WindowRect.right, Window->WindowRect.bottom);
    DbgPrint("SetWindowPos(c): %i,%i,%i,%i\n", Window->ClientRect.left, Window->ClientRect.top, Window->ClientRect.right, Window->ClientRect.bottom);
  }
  LEAVE_CRITICAL();

  END_NTUSER();
}

WORD STDCALL
NtUserSetWindowWord(HWND hWnd, INT Index, WORD NewValue)
{
  NTUSER_USER_OBJECT(WINDOW, Window);
  BEGIN_NTUSER(WORD, 0);
  
  ENTER_CRITICAL();
  VALIDATE_USER_OBJECT(WINDOW, hWnd, Window);
  Result = IntSetWindowWord(Window, Index, NewValue);
  LEAVE_CRITICAL();
  
  END_NTUSER();
}

BOOL STDCALL
NtUserSystemParametersInfo(UINT uiAction,
                           UINT uiParam,
                           PVOID pvParam,
                           UINT fWinIni)
{
  union
  {
    BOOL Bool;
    UINT Uint;
    RECT Rect;
    LOGFONTW LogFont;
    NONCLIENTMETRICSW NonClientMetrics;
  } SafeBuffer;
  ULONG BufferLength = 0;
  PVOID OriginalParam = pvParam;
  BOOL CopyBufferBack = FALSE;
  BOOL CopyBuffer = FALSE;
  
  BEGIN_BUFFERS();
  BEGIN_NTUSER(BOOL, FALSE);
  
  /* decide what's to do with buffers */
  switch(uiAction)
  {
  /* BOOLs */
    case SPI_GETFONTSMOOTHING:
      BufferLength = sizeof(BOOL);
      CopyBufferBack = TRUE;
      break;
  /* UINTs */
    case SPI_GETFOCUSBORDERHEIGHT:
    case SPI_GETFOCUSBORDERWIDTH:
      BufferLength = sizeof(UINT);
      CopyBufferBack = TRUE;
      break;
  /* RECTs */
    case SPI_SETWORKAREA:
      BufferLength = sizeof(RECT);
      CopyBuffer = TRUE;
      break;
    case SPI_GETWORKAREA:
      BufferLength = sizeof(RECT);
      CopyBufferBack = TRUE;
      break;
  /* LOGFONTWs */
    case SPI_GETICONTITLELOGFONT:
      BufferLength = sizeof(LOGFONTW);
      CopyBufferBack = TRUE;
      break;
  /* NONCLIENTMETRICSWs */
    case SPI_GETNONCLIENTMETRICS:
      BufferLength = sizeof(NONCLIENTMETRICSW);
      CopyBufferBack = TRUE;
      break;  
  }
  
  ASSERT(!CopyBuffer || (CopyBuffer && BufferLength > 0));
  if(CopyBuffer)
  {
    /* We need to copy the buffer, specified in pvParam. Fail if it's NULL */
    NTUSER_FAIL_INVALID_PARAMETER(pvParam, NULL);
    
    /* be smart and make a safe copy of the buffer */
    NTUSER_COPY_BUFFER_NTERROR(&SafeBuffer, pvParam, BufferLength);
    
    /* change the buffer for the internal call */
    pvParam = &SafeBuffer;
  }
  
  ENTER_CRITICAL();
  Result = IntSystemParametersInfo(uiAction, uiParam, pvParam, fWinIni);
  LEAVE_CRITICAL();
  
  ASSERT(!CopyBufferBack || (CopyBufferBack && BufferLength > 0));
  if(CopyBufferBack)
  {
    /* copy the buffer back to the user */
    NTUSER_COPY_BUFFER_BACK_NTERROR(OriginalParam, &SafeBuffer, BufferLength);
  }
  
  END_NTUSER();
}

BOOL STDCALL
NtUserTranslateMessage(LPMSG lpMsg,
		       HKL dwhkl)
{
  KMSG KMsg;
  NTUSER_USER_OBJECT(WINDOW, Window);
  BEGIN_NTUSER(BOOL, FALSE);
  
  /* FIXME - probe lpMsg */
  
  /* FIXME - lock stuff here... */
  ENTER_CRITICAL_SHARED();
  if(lpMsg->hwnd != NULL)
  {
    VALIDATE_USER_OBJECT(WINDOW, lpMsg->hwnd, Window);
  }
  else
  {
    Window = NULL;
  }
  MsgCopyMsgToKMsg(&KMsg, lpMsg, Window);
  /* FIXME - put this outside of the lock? */
  Result = IntTranslateKbdMessage(&KMsg, dwhkl);
  LEAVE_CRITICAL();
  
  END_NTUSER();
}

BOOL STDCALL
NtUserUpdateWindow(HWND hWnd)
{
  NTUSER_USER_OBJECT(WINDOW, Window);
  BEGIN_NTUSER(BOOL, FALSE);

  ENTER_CRITICAL();
  if(hWnd != NULL)
  {
    VALIDATE_USER_OBJECT(WINDOW, hWnd, Window);
  }

  Result = IntUpdateWindow(Window);

  LEAVE_CRITICAL();

  END_NTUSER();
}

BOOL STDCALL
NtUserValidateRect(HWND hWnd, const RECT* Rect)
{
  RECT SafeRect;
  NTUSER_USER_OBJECT(WINDOW, Window);
  BEGIN_BUFFERS();
  BEGIN_NTUSER(BOOL, FALSE);
  
  if(Rect != NULL)
  {
    NTUSER_COPY_BUFFER_NTERROR(&SafeRect, Rect, sizeof(RECT));
  }
  
  ENTER_CRITICAL();
  if(hWnd != NULL)
  {
    VALIDATE_USER_OBJECT(WINDOW, hWnd, Window);
  }
  
  Result = IntValidateRect(Window, (Rect != NULL ? &SafeRect : NULL));
  
  LEAVE_CRITICAL();
  
  END_NTUSER();
}

BOOL STDCALL
NtUserValidateRgn(HWND hWnd, HRGN hRgn)
{
  NTUSER_USER_OBJECT(WINDOW, Window);
  BEGIN_NTUSER(BOOL, FALSE);
  
  ENTER_CRITICAL();
  if(hWnd != NULL)
  {
    VALIDATE_USER_OBJECT(WINDOW, hWnd, Window);
  }
  
  Result = IntValidateRgn(Window, hRgn);
  
  LEAVE_CRITICAL();
  
  END_NTUSER();
}

BOOL STDCALL
NtUserWaitMessage(VOID)
{
  BEGIN_NTUSER_NOERR(BOOL);
  
  /* We're not going to lock anything here, we don't want other threads to hang */
  Result = IntWaitMessage(NULL, 0, 0);
  
  END_NTUSER_NOERR();
}

