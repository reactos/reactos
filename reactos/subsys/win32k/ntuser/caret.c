/* $Id: caret.c,v 1.4 2003/11/22 01:49:39 rcampbell Exp $
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
#include <include/callback.h>

#define NDEBUG
#include <debug.h>

BOOL FASTCALL
IntHideCaret(PTHRDCARETINFO CaretInfo)
{
  if(CaretInfo->hWnd && CaretInfo->Visible && CaretInfo->Showing)
  {
    IntCallWindowProc(NULL, CaretInfo->hWnd, WM_SYSTIMER, IDCARETTIMER, 0);
    CaretInfo->Showing = 0;
    return TRUE;
  }
  return FALSE;
}

BOOL FASTCALL
IntDestroyCaret(PW32THREAD Win32Thread)
{
  PTHRDCARETINFO CaretInfo = ThrdCaretInfo(Win32Thread);
  IntHideCaret(CaretInfo);
  RtlZeroMemory(CaretInfo, sizeof(THRDCARETINFO));
  return TRUE;
}

BOOL FASTCALL
IntSetCaretBlinkTime(UINT uMSeconds)
{
  return FALSE;
}

UINT
IntGetCaretBlinkTime(VOID)
{
	HKEY hKey;
	BYTE dwValue[4];
	DWORD dwType;
	DWORD dwLen;


	if (RegOpenKeyEx( HKEY_CURRENT_USER, "Control Panel\\Desktop", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
	{
	
		if (RegQueryValueEx(hKey,"CursorBlinkRate",NULL,&dwType,dwValue,&dwLen) == ERROR_SUCCESS)
		{
			return atoi( (char *)dwValue );
		}
	}
	return 0;
}

BOOL FASTCALL
IntSetCaretPos(int X, int Y)
{
  PTHRDCARETINFO CaretInfo = ThrdCaretInfo(PsGetCurrentThread()->Win32Thread);
  
  if(CaretInfo->hWnd)
  {
    if(CaretInfo->Pos.x != X || CaretInfo->Pos.y != Y)
    {
      IntHideCaret(CaretInfo);
      CaretInfo->Showing = 0;
      CaretInfo->Pos.x = X;
      CaretInfo->Pos.y = Y;
      IntCallWindowProc(NULL, CaretInfo->hWnd, WM_SYSTIMER, IDCARETTIMER, 0);
      IntSetTimer(CaretInfo->hWnd, IDCARETTIMER, IntGetCaretBlinkTime(), NULL, TRUE);
    }
    return TRUE;
  }

  return FALSE;
}

BOOL FASTCALL
IntSwitchCaretShowing(PVOID Info)
{
  PTHRDCARETINFO CaretInfo = ThrdCaretInfo(PsGetCurrentThread()->Win32Thread);
  
  if(CaretInfo->hWnd)
  {
    CaretInfo->Showing = (CaretInfo->Showing ? 0 : 1);
    MmCopyToCaller(Info, CaretInfo, sizeof(THRDCARETINFO));
    return TRUE;
  }
  
  return FALSE;
}

VOID FASTCALL
IntDrawCaret(HWND hWnd)
{
  PTHRDCARETINFO CaretInfo = ThrdCaretInfo(PsGetCurrentThread()->Win32Thread);
  
  if(CaretInfo->hWnd && CaretInfo->Visible && CaretInfo->Showing)
  {
    IntCallWindowProc(NULL, CaretInfo->hWnd, WM_SYSTIMER, IDCARETTIMER, 0);
    CaretInfo->Showing = 1;
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
  
  IntHideCaret(CaretInfo);
  
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
    
    IntHideCaret(CaretInfo);
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
    if(!CaretInfo->Showing)
    {
      IntCallWindowProc(NULL, CaretInfo->hWnd, WM_SYSTIMER, IDCARETTIMER, 0);
    }
    IntSetTimer(hWnd, IDCARETTIMER, IntGetCaretBlinkTime(), NULL, TRUE);
  }
  
  IntReleaseWindowObject(WindowObject);
  return TRUE;
}

