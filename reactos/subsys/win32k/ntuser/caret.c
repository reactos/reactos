/* $Id: caret.c,v 1.12 2004/05/10 17:07:18 weiden Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Caret functions
 * FILE:             subsys/win32k/ntuser/caret.c
 * PROGRAMER:        Thomas Weidenmueller (w3seek@users.sourceforge.net)
 * REVISION HISTORY:
 *       10/15/2003  Created
 */

#include <w32k.h>

#define NDEBUG
#include <debug.h>

#define MIN_CARETBLINKRATE 100
#define MAX_CARETBLINKRATE 10000
#define DEFAULT_CARETBLINKRATE 530
#define CARET_REGKEY L"\\Registry\\User\\.Default\\Control Panel\\Desktop"
#define CARET_VALUENAME L"CursorBlinkRate"

BOOL FASTCALL
IntHideCaret(PTHRDCARETINFO CaretInfo)
{
  if(CaretInfo->hWnd && CaretInfo->Visible && CaretInfo->Showing)
  {
    IntSendMessage(CaretInfo->hWnd, WM_SYSTIMER, IDCARETTIMER, 0);
    CaretInfo->Showing = 0;
    return TRUE;
  }
  return FALSE;
}

BOOL FASTCALL
IntDestroyCaret(PW32THREAD Win32Thread)
{
  PUSER_MESSAGE_QUEUE ThreadQueue;
  ThreadQueue = (PUSER_MESSAGE_QUEUE)Win32Thread->MessageQueue;
  
  if(!ThreadQueue || !ThreadQueue->CaretInfo)
    return FALSE;
  
  IntHideCaret(ThreadQueue->CaretInfo);
  ThreadQueue->CaretInfo->Bitmap = (HBITMAP)0;
  ThreadQueue->CaretInfo->hWnd = (HWND)0;
  ThreadQueue->CaretInfo->Size.cx = ThreadQueue->CaretInfo->Size.cy = 0;
  ThreadQueue->CaretInfo->Showing = 0;
  ThreadQueue->CaretInfo->Visible = 0;
  return TRUE;
}

BOOL FASTCALL
IntSetCaretBlinkTime(UINT uMSeconds)
{
  /* Don't save the new value to the registry! */
  NTSTATUS Status;
  PWINSTATION_OBJECT WinStaObject;
  
  Status = IntValidateWindowStationHandle(PROCESS_WINDOW_STATION(),
	                                  KernelMode,
				          0,
				          &WinStaObject);
  if(!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    return FALSE;
  }
  
  /* windows doesn't do this check */
  if((uMSeconds < MIN_CARETBLINKRATE) || (uMSeconds > MAX_CARETBLINKRATE))
  {
    SetLastWin32Error(ERROR_INVALID_PARAMETER);
    ObDereferenceObject(WinStaObject);
    return FALSE;
  }
  
  WinStaObject->CaretBlinkRate = uMSeconds;
  
  ObDereferenceObject(WinStaObject);
  return TRUE;
}

UINT FASTCALL
IntQueryCaretBlinkRate(VOID)
{
  UNICODE_STRING KeyName, ValueName;
  NTSTATUS Status;
  HANDLE KeyHandle = NULL;
  OBJECT_ATTRIBUTES KeyAttributes;
  PKEY_VALUE_PARTIAL_INFORMATION KeyValuePartialInfo;
  ULONG Length = 0;
  ULONG ResLength = 0;
  ULONG Val = 0;
  
  RtlRosInitUnicodeStringFromLiteral(&KeyName, CARET_REGKEY);
  RtlRosInitUnicodeStringFromLiteral(&ValueName, CARET_VALUENAME);
  
  InitializeObjectAttributes(&KeyAttributes, &KeyName, OBJ_CASE_INSENSITIVE,
                             NULL, NULL);
  
  Status = ZwOpenKey(&KeyHandle, KEY_READ, &KeyAttributes);
  if(!NT_SUCCESS(Status))
  {
    return 0;
  }
  
  Status = ZwQueryValueKey(KeyHandle, &ValueName, KeyValuePartialInformation,
                           0, 0, &ResLength);
  if((Status != STATUS_BUFFER_TOO_SMALL))
  {
    NtClose(KeyHandle);
    return 0;
  }
  
  ResLength += sizeof(KEY_VALUE_PARTIAL_INFORMATION);
  KeyValuePartialInfo = ExAllocatePoolWithTag(PagedPool, ResLength, TAG_STRING);
  Length = ResLength;
  
  if(!KeyValuePartialInfo)
  {
    NtClose(KeyHandle);
    return 0;
  }
  
  Status = ZwQueryValueKey(KeyHandle, &ValueName, KeyValuePartialInformation,
                           (PVOID)KeyValuePartialInfo, Length, &ResLength);
  if(!NT_SUCCESS(Status) || (KeyValuePartialInfo->Type != REG_SZ))
  {
    NtClose(KeyHandle);
    ExFreePool(KeyValuePartialInfo);
    return 0;
  }
  
  ValueName.Length = KeyValuePartialInfo->DataLength;
  ValueName.MaximumLength = KeyValuePartialInfo->DataLength;
  ValueName.Buffer = (PWSTR)KeyValuePartialInfo->Data;
  
  Status = RtlUnicodeStringToInteger(&ValueName, 0, &Val);
  if(!NT_SUCCESS(Status))
  {
    Val = 0;
  }
  
  ExFreePool(KeyValuePartialInfo);
  NtClose(KeyHandle);
  
  return (UINT)Val;
}

UINT FASTCALL
IntGetCaretBlinkTime(VOID)
{
  NTSTATUS Status;
  PWINSTATION_OBJECT WinStaObject;
  UINT Ret;
  
  Status = IntValidateWindowStationHandle(PROCESS_WINDOW_STATION(),
	                                  KernelMode,
				          0,
				          &WinStaObject);
  if(!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    return 0;
  }
  
  Ret = WinStaObject->CaretBlinkRate;
  if(!Ret)
  {
    /* load it from the registry the first call only! */
    Ret = WinStaObject->CaretBlinkRate = IntQueryCaretBlinkRate();
  }
  
  /* windows doesn't do this check */
  if((Ret < MIN_CARETBLINKRATE) || (Ret > MAX_CARETBLINKRATE))
  {
    Ret = DEFAULT_CARETBLINKRATE;
  }
  
  ObDereferenceObject(WinStaObject);
  return Ret;
}

BOOL FASTCALL
IntSetCaretPos(int X, int Y)
{
  PUSER_MESSAGE_QUEUE ThreadQueue;
  ThreadQueue = (PUSER_MESSAGE_QUEUE)PsGetWin32Thread()->MessageQueue;
  
  if(ThreadQueue->CaretInfo->hWnd)
  {
    if(ThreadQueue->CaretInfo->Pos.x != X || ThreadQueue->CaretInfo->Pos.y != Y)
    {
      IntHideCaret(ThreadQueue->CaretInfo);
      ThreadQueue->CaretInfo->Showing = 0;
      ThreadQueue->CaretInfo->Pos.x = X;
      ThreadQueue->CaretInfo->Pos.y = Y;
      IntSendMessage(ThreadQueue->CaretInfo->hWnd, WM_SYSTIMER, IDCARETTIMER, 0);
      IntSetTimer(ThreadQueue->CaretInfo->hWnd, IDCARETTIMER, IntGetCaretBlinkTime(), NULL, TRUE);
    }
    return TRUE;
  }

  return FALSE;
}

BOOL FASTCALL
IntSwitchCaretShowing(PVOID Info)
{
  PUSER_MESSAGE_QUEUE ThreadQueue;
  ThreadQueue = (PUSER_MESSAGE_QUEUE)PsGetWin32Thread()->MessageQueue;
  
  if(ThreadQueue->CaretInfo->hWnd)
  {
    ThreadQueue->CaretInfo->Showing = (ThreadQueue->CaretInfo->Showing ? 0 : 1);
    MmCopyToCaller(Info, ThreadQueue->CaretInfo, sizeof(THRDCARETINFO));
    return TRUE;
  }
  
  return FALSE;
}

VOID FASTCALL
IntDrawCaret(HWND hWnd)
{
  PUSER_MESSAGE_QUEUE ThreadQueue;
  ThreadQueue = (PUSER_MESSAGE_QUEUE)PsGetWin32Thread()->MessageQueue;
  
  if(ThreadQueue->CaretInfo->hWnd && ThreadQueue->CaretInfo->Visible && 
     ThreadQueue->CaretInfo->Showing)
  {
    IntSendMessage(ThreadQueue->CaretInfo->hWnd, WM_SYSTIMER, IDCARETTIMER, 0);
    ThreadQueue->CaretInfo->Showing = 1;
  }
}



BOOL
STDCALL
NtUserCreateCaret(
  HWND hWnd,
  HBITMAP hBitmap,
  int nWidth,
  int nHeight)
{
  PWINDOW_OBJECT WindowObject;
  PUSER_MESSAGE_QUEUE ThreadQueue;
  
  WindowObject = IntGetWindowObject(hWnd);
  if(!WindowObject)
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return FALSE;
  }
  
  if(WindowObject->OwnerThread != PsGetCurrentThread())
  {
    IntReleaseWindowObject(WindowObject);
    SetLastWin32Error(ERROR_ACCESS_DENIED);
    return FALSE;
  }
  
  IntRemoveTimer(hWnd, IDCARETTIMER, PsGetCurrentThreadId(), TRUE);
  
  ThreadQueue = (PUSER_MESSAGE_QUEUE)PsGetWin32Thread()->MessageQueue;
  
  IntHideCaret(ThreadQueue->CaretInfo);
  
  ThreadQueue->CaretInfo->hWnd = hWnd;
  if(hBitmap)
  {
    ThreadQueue->CaretInfo->Bitmap = hBitmap;
    ThreadQueue->CaretInfo->Size.cx = ThreadQueue->CaretInfo->Size.cy = 0;
  }
  else
  {
    ThreadQueue->CaretInfo->Bitmap = (HBITMAP)0;
    ThreadQueue->CaretInfo->Size.cx = nWidth;
    ThreadQueue->CaretInfo->Size.cy = nHeight;
  }
  ThreadQueue->CaretInfo->Visible = 0;
  ThreadQueue->CaretInfo->Showing = 0;
  
  IntReleaseWindowObject(WindowObject);  
  return TRUE;
}

UINT
STDCALL
NtUserGetCaretBlinkTime(VOID)
{
  return IntGetCaretBlinkTime();
}

BOOL
STDCALL
NtUserGetCaretPos(
  LPPOINT lpPoint)
{
  PUSER_MESSAGE_QUEUE ThreadQueue;
  NTSTATUS Status;
  
  ThreadQueue = (PUSER_MESSAGE_QUEUE)PsGetWin32Thread()->MessageQueue;
  
  Status = MmCopyToCaller(lpPoint, &(ThreadQueue->CaretInfo->Pos), sizeof(POINT));
  if(!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    return FALSE;
  }
  
  return TRUE;
}

BOOL
STDCALL
NtUserHideCaret(
  HWND hWnd)
{
  PWINDOW_OBJECT WindowObject;
  PUSER_MESSAGE_QUEUE ThreadQueue;
  
  WindowObject = IntGetWindowObject(hWnd);
  if(!WindowObject)
  {
    SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
    return FALSE;
  }
  
  if(WindowObject->OwnerThread != PsGetCurrentThread())
  {
    IntReleaseWindowObject(WindowObject);
    SetLastWin32Error(ERROR_ACCESS_DENIED);
    return FALSE;
  }
  
  ThreadQueue = (PUSER_MESSAGE_QUEUE)PsGetWin32Thread()->MessageQueue;
  
  if(ThreadQueue->CaretInfo->hWnd != hWnd)
  {
    IntReleaseWindowObject(WindowObject);
    SetLastWin32Error(ERROR_ACCESS_DENIED);
    return FALSE;
  }
  
  if(ThreadQueue->CaretInfo->Visible)
  {
    IntRemoveTimer(hWnd, IDCARETTIMER, PsGetCurrentThreadId(), TRUE);
    
    IntHideCaret(ThreadQueue->CaretInfo);
    ThreadQueue->CaretInfo->Visible = 0;
    ThreadQueue->CaretInfo->Showing = 0;
  }
  
  IntReleaseWindowObject(WindowObject);
  return TRUE;
}

BOOL
STDCALL
NtUserShowCaret(
  HWND hWnd)
{
  PWINDOW_OBJECT WindowObject;
  PUSER_MESSAGE_QUEUE ThreadQueue;
  
  WindowObject = IntGetWindowObject(hWnd);
  if(!WindowObject)
  {
    SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
    return FALSE;
  }
  
  if(WindowObject->OwnerThread != PsGetCurrentThread())
  {
    IntReleaseWindowObject(WindowObject);
    SetLastWin32Error(ERROR_ACCESS_DENIED);
    return FALSE;
  }
  
  ThreadQueue = (PUSER_MESSAGE_QUEUE)PsGetWin32Thread()->MessageQueue;
  
  if(ThreadQueue->CaretInfo->hWnd != hWnd)
  {
    IntReleaseWindowObject(WindowObject);
    SetLastWin32Error(ERROR_ACCESS_DENIED);
    return FALSE;
  }
  
  if(!ThreadQueue->CaretInfo->Visible)
  {
    ThreadQueue->CaretInfo->Visible = 1;
    if(!ThreadQueue->CaretInfo->Showing)
    {
      IntSendMessage(ThreadQueue->CaretInfo->hWnd, WM_SYSTIMER, IDCARETTIMER, 0);
    }
    IntSetTimer(hWnd, IDCARETTIMER, IntGetCaretBlinkTime(), NULL, TRUE);
  }
  
  IntReleaseWindowObject(WindowObject);
  return TRUE;
}
