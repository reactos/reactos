/* $Id: caret.c,v 1.2 2003/10/16 22:07:37 weiden Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Caret functions
 * FILE:             subsys/win32k/ntuser/caret.c
 * PROGRAMER:        Thomas Weidenmueller (w3seek@users.sourceforge.net)
 * REVISION HISTORY:
 *       10/15/2003  Created
 */

#include <win32k/win32k.h>
#include <internal/safe.h>
#include <include/error.h>
#include <include/window.h>
#include <include/caret.h>
#include <include/timer.h>

#define NDEBUG
#include <debug.h>

BOOL FASTCALL
IntDestroyCaret(PW32THREAD Win32Thread)
{
  PTHRDCARETINFO CaretInfo = ThrdCaretInfo(Win32Thread);
  RtlZeroMemory(CaretInfo, sizeof(THRDCARETINFO));
  return TRUE;
}

BOOL FASTCALL
IntSetCaretBlinkTime(UINT uMSeconds)
{
  return FALSE;
}

UINT FASTCALL
IntGetCaretBlinkTime(VOID)
{
  return 500;
}

BOOL FASTCALL
IntSetCaretPos(int X, int Y)
{
  PTHRDCARETINFO CaretInfo = ThrdCaretInfo(PsGetCurrentThread()->Win32Thread);
  
  if(CaretInfo->hWnd)
  {
    if(CaretInfo->Pos.x != X || CaretInfo->Pos.y != Y)
    {
      CaretInfo->Pos.x = X;
      CaretInfo->Pos.y = Y;
      IntSetTimer(CaretInfo->hWnd, IDCARETTIMER, IntGetCaretBlinkTime(), NULL, TRUE);
    }
    return TRUE;
  }

  return FALSE;
}

BOOL FASTCALL
IntSwitchCaretShowing(VOID)
{
  PTHRDCARETINFO CaretInfo = ThrdCaretInfo(PsGetCurrentThread()->Win32Thread);
  
  if(CaretInfo->hWnd)
  {
    CaretInfo->Showing = (CaretInfo->Showing ? 0 : 1);
    return TRUE;
  }
  
  return FALSE;
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
  PTHRDCARETINFO CaretInfo;
  
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
  
  CaretInfo = ThrdCaretInfo(WindowObject->OwnerThread->Win32Thread);
  CaretInfo->hWnd = hWnd;
  if(hBitmap)
  {
    CaretInfo->Bitmap = hBitmap;
    CaretInfo->Size.cx = CaretInfo->Size.cy = 0;
  }
  else
  {
    CaretInfo->Bitmap = (HBITMAP)0;
    CaretInfo->Size.cx = nWidth;
    CaretInfo->Size.cy = nHeight;
  }
  CaretInfo->Pos.x = CaretInfo->Pos.y = 0;
  CaretInfo->Visible = 0;
  CaretInfo->Showing = 0;
  
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
  PTHRDCARETINFO CaretInfo;
  NTSTATUS Status;
  
  CaretInfo = ThrdCaretInfo(PsGetCurrentThread()->Win32Thread);
  
  Status = MmCopyToCaller(lpPoint, &(CaretInfo->Pos), sizeof(POINT));
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
  PTHRDCARETINFO CaretInfo;
  
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
  
  CaretInfo = ThrdCaretInfo(PsGetCurrentThread()->Win32Thread);
  
  if(CaretInfo->hWnd != hWnd)
  {
    IntReleaseWindowObject(WindowObject);
    SetLastWin32Error(ERROR_ACCESS_DENIED);
    return FALSE;
  }
  
  if(CaretInfo->Visible)
  {
    IntRemoveTimer(hWnd, IDCARETTIMER, PsGetCurrentThreadId(), TRUE);
    if(CaretInfo->Showing)
    {
      /* FIXME send another WM_SYSTIMER message to hide the cursor */
    }
    CaretInfo->Visible = 0;
    CaretInfo->Showing = 0;
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
  PTHRDCARETINFO CaretInfo;
  
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
  
  CaretInfo = ThrdCaretInfo(PsGetCurrentThread()->Win32Thread);
  
  if(CaretInfo->hWnd != hWnd)
  {
    IntReleaseWindowObject(WindowObject);
    SetLastWin32Error(ERROR_ACCESS_DENIED);
    return FALSE;
  }
  
  if(!CaretInfo->Visible)
  {
    CaretInfo->Visible = 1;
    CaretInfo->Showing = 0;
    IntSetTimer(hWnd, IDCARETTIMER, IntGetCaretBlinkTime(), NULL, TRUE);
  }
  
  IntReleaseWindowObject(WindowObject);
  return TRUE;
}

