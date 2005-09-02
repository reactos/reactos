/* $Id$
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
co_IntHideCaret(PTHRDCARETINFO CaretInfo)
{
   ASSERT_LOCK(ulExclusive);
   
   if(CaretInfo->hWnd && CaretInfo->Visible && CaretInfo->Showing)
   {
      co_UserSendMessage(CaretInfo->hWnd, WM_SYSTIMER, IDCARETTIMER, 0);
      CaretInfo->Showing = 0;
      return TRUE;
   }
   return FALSE;
}

BOOL FASTCALL
co_UserDestroyCaret(PW32THREAD Win32Thread /* unused param!!!*/)
{
   PW32THREAD W32Thread;

   ASSERT_LOCK(ulExclusive);
   
   W32Thread = PsGetWin32Thread();

   //FIXME: can W32Thread be NULL?
   if(!W32Thread)
      return FALSE;

   co_IntHideCaret(&W32Thread->Queue.Input->CaretInfo);
   RtlZeroMemory(&W32Thread->Queue.Input->CaretInfo, sizeof(THRDCARETINFO));
   return TRUE;
}

BOOL FASTCALL
UserSetCaretBlinkTime(UINT uMSeconds)
{
   /* Don't save the new value to the registry! */
   PWINSTATION_OBJECT WinStaObject = UserGetCurrentWinSta();

   ASSERT_LOCK(ulExclusive);

   /* windows doesn't do this check */
   if((uMSeconds < MIN_CARETBLINKRATE) || (uMSeconds > MAX_CARETBLINKRATE))
   {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      ObDereferenceObject(WinStaObject);
      return FALSE;
   }

   WinStaObject->CaretBlinkRate = uMSeconds;

   return TRUE;
}

UINT FASTCALL
UserQueryCaretBlinkRate(VOID)
{
   UNICODE_STRING KeyName = RTL_CONSTANT_STRING(CARET_REGKEY);
   UNICODE_STRING ValueName = RTL_CONSTANT_STRING(CARET_VALUENAME);
   NTSTATUS Status;
   HANDLE KeyHandle = NULL;
   OBJECT_ATTRIBUTES KeyAttributes;
   PKEY_VALUE_PARTIAL_INFORMATION KeyValuePartialInfo;
   ULONG Length = 0;
   ULONG ResLength = 0;
   ULONG Val = 0;

   ASSERT_LOCK(ulShared);

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
UserGetCaretBlinkTime(VOID)
{
   PWINSTATION_OBJECT WinStaObject;
   UINT Ret;

   ASSERT_LOCK(ulExclusive);
   
   WinStaObject = UserGetCurrentWinSta();

   Ret = WinStaObject->CaretBlinkRate;
   if(!Ret)
   {
      /* load it from the registry the first call only! */
      Ret = WinStaObject->CaretBlinkRate = UserQueryCaretBlinkRate();
   }

   /* windows doesn't do this check */
   if((Ret < MIN_CARETBLINKRATE) || (Ret > MAX_CARETBLINKRATE))
   {
      Ret = DEFAULT_CARETBLINKRATE;
   }

   return Ret;
}

BOOL FASTCALL
co_UserSetCaretPos(int X, int Y)
{
   PUSER_THREAD_INPUT Input;

   ASSERT_LOCK(ulExclusive);

   Input = UserGetCurrentQueue()->Input;

   if(Input->CaretInfo.hWnd)
   {
      if(Input->CaretInfo.Pos.x != X || Input->CaretInfo.Pos.y != Y)
      {
         co_IntHideCaret(&Input->CaretInfo);
         Input->CaretInfo.Showing = 0;
         Input->CaretInfo.Pos.x = X;
         Input->CaretInfo.Pos.y = Y;
         co_UserSendMessage(Input->CaretInfo.hWnd, WM_SYSTIMER, IDCARETTIMER, 0);
         UserSetTimer(GetWnd(Input->CaretInfo.hWnd), IDCARETTIMER, UserGetCaretBlinkTime(), NULL, TRUE);
      }
      return TRUE;
   }

   return FALSE;
}

BOOL FASTCALL
UserSwitchCaretShowing(PTHRDCARETINFO Info)
{
   PUSER_THREAD_INPUT Input;
   
   ASSERT_LOCK(ulExclusive);

   Input = UserGetCurrentQueue()->Input;

   if(Input->CaretInfo.hWnd)
   {
      Input->CaretInfo.Showing = !Input->CaretInfo.Showing;
      //FIXME: internal. no SEH here!
      MmCopyToCaller(Info, &Input->CaretInfo, sizeof(THRDCARETINFO));
      //*Info = Input->CaretInfo;
      return TRUE;
   }

   return FALSE;
}


#if 0
/* FIXME: UNUSED FUNCTION!!!!!! WHY????? */
VOID FASTCALL
co_UserDrawCaret(HWND hWnd /* FIXME: Unused param?!?! */)
{
   PUSER_THREAD_INPUT Input;

   Input = UserGetCurrentQueue()->Input;

   if(Input->CaretInfo.hWnd && Input->CaretInfo.Visible &&
         Input->CaretInfo.Showing)
   {
      co_UserSendMessage(Input->CaretInfo.hWnd, WM_SYSTIMER, IDCARETTIMER, 0);
      Input->CaretInfo.Showing = 1;
   }
}
#endif


BOOL
STDCALL
NtUserCreateCaret(
   HWND hWnd,
   HBITMAP hBitmap,
   int nWidth,
   int nHeight)
{
   PWINDOW_OBJECT Window;
   PUSER_THREAD_INPUT Input;
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserCreateCaret\n");
   UserEnterExclusive();

   if(!(Window = UserGetWindowObject(hWnd)))
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      RETURN(FALSE);
   }

   if(Window->Queue != UserGetCurrentQueue())
   {
      SetLastWin32Error(ERROR_ACCESS_DENIED);
      RETURN(FALSE);
   }

   Input = UserGetCurrentQueue()->Input;

   if (Input->CaretInfo.Visible)
   {
      UserKillTimer(Window, IDCARETTIMER, TRUE);
      co_IntHideCaret(&Input->CaretInfo);
   }

   Input->CaretInfo.hWnd = hWnd;
   if(hBitmap)
   {
      Input->CaretInfo.Bitmap = hBitmap;
      Input->CaretInfo.Size.cx = Input->CaretInfo.Size.cy = 0;
   }
   else
   {
      Input->CaretInfo.Bitmap = (HBITMAP)0;
      Input->CaretInfo.Size.cx = nWidth;
      Input->CaretInfo.Size.cy = nHeight;
   }
   Input->CaretInfo.Visible = 0;
   Input->CaretInfo.Showing = 0;

   RETURN(TRUE);

CLEANUP:
   DPRINT("Leave NtUserCreateCaret, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

UINT
STDCALL
NtUserGetCaretBlinkTime(VOID)
{
   DECLARE_RETURN(UINT);

   DPRINT("Enter NtUserGetCaretBlinkTime\n");
   UserEnterExclusive();

   RETURN(UserGetCaretBlinkTime());

CLEANUP:
   DPRINT("Leave NtUserGetCaretBlinkTime, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


BOOL
STDCALL
NtUserGetCaretPos(
   LPPOINT lpPoint)
{
   PUSER_THREAD_INPUT Input;
   NTSTATUS Status;
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserGetCaretPos\n");
   UserEnterShared();

   Input = UserGetCurrentQueue()->Input;

   Status = MmCopyToCaller(lpPoint, &(Input->CaretInfo.Pos), sizeof(POINT));
   if(!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      RETURN(FALSE);
   }

   RETURN(TRUE);

CLEANUP:
   DPRINT("Leave NtUserGetCaretPos, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


BOOL
STDCALL
NtUserHideCaret(
   HWND hWnd)
{
   PWINDOW_OBJECT Window;
   PUSER_MESSAGE_QUEUE Queue;
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserHideCaret\n");
   UserEnterExclusive();

   if(!(Window = UserGetWindowObject(hWnd)))
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      RETURN(FALSE);
   }

   Queue = UserGetCurrentQueue();

   if(Window->Queue != Queue)
   {
      SetLastWin32Error(ERROR_ACCESS_DENIED);
      RETURN(FALSE);
   }

   if(Queue->Input->CaretInfo.hWnd != hWnd)
   {
      SetLastWin32Error(ERROR_ACCESS_DENIED);
      RETURN(FALSE);
   }

   if(Queue->Input->CaretInfo.Visible)
   {
      UserKillTimer(Window, IDCARETTIMER, TRUE);

      co_IntHideCaret(&Queue->Input->CaretInfo);
      Queue->Input->CaretInfo.Visible = 0;
      Queue->Input->CaretInfo.Showing = 0;
   }

   RETURN(TRUE);

CLEANUP:
   DPRINT("Leave NtUserHideCaret, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}



BOOL FASTCALL
co_UserHideCaret(PWINDOW_OBJECT Wnd)
{
   PUSER_MESSAGE_QUEUE Queue;
   
   ASSERT(Wnd);
   ASSERT_LOCK(ulExclusive);

   Queue = UserGetCurrentQueue();
   
   if(Wnd->Queue != Queue)
   {
      SetLastWin32Error(ERROR_ACCESS_DENIED);
      return(FALSE);
   }

   if(Queue->Input->CaretInfo.hWnd != Wnd->hSelf)
   {
      SetLastWin32Error(ERROR_ACCESS_DENIED);
      return(FALSE);
   }

   if(Queue->Input->CaretInfo.Visible)
   {
      UserKillTimer(Wnd, IDCARETTIMER, TRUE);

      //FIXME: fix IntHideCaret vs. NtHideCaret vs. UserHideCaret mess
      co_IntHideCaret(&Queue->Input->CaretInfo);
      Queue->Input->CaretInfo.Visible = 0;
      Queue->Input->CaretInfo.Showing = 0;
   }

   return(TRUE);
}



BOOL
STDCALL
NtUserShowCaret(HWND hWnd)
{
   PWINDOW_OBJECT Wnd;
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserShowCaret\n");
   UserEnterExclusive();

   if(!(Wnd = UserGetWindowObject(hWnd)))
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      RETURN(FALSE);
   }

   RETURN(co_UserShowCaret(Wnd));

CLEANUP:
   DPRINT("Leave NtUserShowCaret, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}



BOOL FASTCALL
co_UserShowCaret(PWINDOW_OBJECT Wnd)
{
   PUSER_MESSAGE_QUEUE Queue;

   ASSERT(Wnd);
   ASSERT_LOCK(ulExclusive);

   Queue = UserGetCurrentQueue();

   if(Wnd->Queue != Queue)
   {
      SetLastWin32Error(ERROR_ACCESS_DENIED);
      return(FALSE);
   }

   if(Queue->Input->CaretInfo.hWnd != Wnd->hSelf)
   {
      SetLastWin32Error(ERROR_ACCESS_DENIED);
      return(FALSE);
   }

   if(!Queue->Input->CaretInfo.Visible)
   {
      Queue->Input->CaretInfo.Visible = 1;
      if(!Queue->Input->CaretInfo.Showing)
      {
         co_UserSendMessage(Queue->Input->CaretInfo.hWnd, WM_SYSTIMER, IDCARETTIMER, 0);
      }
      UserSetTimer(Wnd, IDCARETTIMER, UserGetCaretBlinkTime(), NULL, TRUE);
   }

   return(TRUE);
}
