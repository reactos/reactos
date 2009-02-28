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

/* INCLUDES ******************************************************************/

#include <w32k.h>

#define NDEBUG
#include <debug.h>

/* DEFINES *****************************************************************/

#define MIN_CARETBLINKRATE 100
#define MAX_CARETBLINKRATE 10000
#define DEFAULT_CARETBLINKRATE 530
#define CARET_REGKEY L"\\Registry\\User\\.Default\\Control Panel\\Desktop"
#define CARET_VALUENAME L"CursorBlinkRate"

/* FUNCTIONS *****************************************************************/

static
BOOL FASTCALL
co_IntHideCaret(PTHRDCARETINFO CaretInfo)
{
   if(CaretInfo->hWnd && CaretInfo->Visible && CaretInfo->Showing)
   {
      co_IntSendMessage(CaretInfo->hWnd, WM_SYSTIMER, IDCARETTIMER, 0);
      CaretInfo->Showing = 0;
      return TRUE;
   }
   return FALSE;
}

BOOL FASTCALL
co_IntDestroyCaret(PTHREADINFO Win32Thread)
{
   PUSER_MESSAGE_QUEUE ThreadQueue;
   ThreadQueue = (PUSER_MESSAGE_QUEUE)Win32Thread->MessageQueue;

   if(!ThreadQueue || !ThreadQueue->CaretInfo)
      return FALSE;

   co_IntHideCaret(ThreadQueue->CaretInfo);
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
   PTHREADINFO pti = PsGetCurrentThreadWin32Thread();
   PWINSTATION_OBJECT WinStaObject = pti->Desktop->WindowStation;

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

static
UINT FASTCALL
IntQueryCaretBlinkRate(VOID)
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
      ExFreePoolWithTag(KeyValuePartialInfo, TAG_STRING);
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

   ExFreePoolWithTag(KeyValuePartialInfo, TAG_STRING);
   NtClose(KeyHandle);

   return (UINT)Val;
}

static
UINT FASTCALL
IntGetCaretBlinkTime(VOID)
{
   PTHREADINFO pti;
   PWINSTATION_OBJECT WinStaObject;
   UINT Ret;

   pti = PsGetCurrentThreadWin32Thread();
   WinStaObject = pti->Desktop->WindowStation;

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

   return Ret;
}


BOOL FASTCALL
co_IntSetCaretPos(int X, int Y)
{
   PTHREADINFO pti;
   PUSER_MESSAGE_QUEUE ThreadQueue;

   pti = PsGetCurrentThreadWin32Thread();
   ThreadQueue = pti->MessageQueue;

   if(ThreadQueue->CaretInfo->hWnd)
   {
      if(ThreadQueue->CaretInfo->Pos.x != X || ThreadQueue->CaretInfo->Pos.y != Y)
      {
         co_IntHideCaret(ThreadQueue->CaretInfo);
         ThreadQueue->CaretInfo->Showing = 0;
         ThreadQueue->CaretInfo->Pos.x = X;
         ThreadQueue->CaretInfo->Pos.y = Y;
         co_IntSendMessage(ThreadQueue->CaretInfo->hWnd, WM_SYSTIMER, IDCARETTIMER, 0);
         IntSetTimer(ThreadQueue->CaretInfo->hWnd, IDCARETTIMER, IntGetCaretBlinkTime(), NULL, TRUE);
      }
      return TRUE;
   }

   return FALSE;
}

BOOL FASTCALL
IntSwitchCaretShowing(PVOID Info)
{
   PTHREADINFO pti;
   PUSER_MESSAGE_QUEUE ThreadQueue;

   pti = PsGetCurrentThreadWin32Thread();
   ThreadQueue = pti->MessageQueue;

   if(ThreadQueue->CaretInfo->hWnd)
   {
      ThreadQueue->CaretInfo->Showing = (ThreadQueue->CaretInfo->Showing ? 0 : 1);
      MmCopyToCaller(Info, ThreadQueue->CaretInfo, sizeof(THRDCARETINFO));
      return TRUE;
   }

   return FALSE;
}

#if 0 //unused
static
VOID FASTCALL
co_IntDrawCaret(HWND hWnd)
{
   PTHREADINFO pti;
   PUSER_MESSAGE_QUEUE ThreadQueue;

   pti = PsGetCurrentThreadWin32Thread();
   ThreadQueue = pti->MessageQueue;

   if(ThreadQueue->CaretInfo->hWnd && ThreadQueue->CaretInfo->Visible &&
         ThreadQueue->CaretInfo->Showing)
   {
      co_IntSendMessage(ThreadQueue->CaretInfo->hWnd, WM_SYSTIMER, IDCARETTIMER, 0);
      ThreadQueue->CaretInfo->Showing = 1;
   }
}
#endif



BOOL FASTCALL co_UserHideCaret(PWINDOW_OBJECT Window OPTIONAL)
{
   PTHREADINFO pti;
   PUSER_MESSAGE_QUEUE ThreadQueue;

   if (Window) ASSERT_REFS_CO(Window);

   if(Window && Window->OwnerThread != PsGetCurrentThread())
   {
      SetLastWin32Error(ERROR_ACCESS_DENIED);
      return FALSE;
   }

   pti = PsGetCurrentThreadWin32Thread();
   ThreadQueue = pti->MessageQueue;

   if(Window && ThreadQueue->CaretInfo->hWnd != Window->hSelf)
   {
      SetLastWin32Error(ERROR_ACCESS_DENIED);
      return FALSE;
   }

   if(ThreadQueue->CaretInfo->Visible)
   {
      IntKillTimer(ThreadQueue->CaretInfo->hWnd, IDCARETTIMER, TRUE);

      co_IntHideCaret(ThreadQueue->CaretInfo);
      ThreadQueue->CaretInfo->Visible = 0;
      ThreadQueue->CaretInfo->Showing = 0;
   }

   return TRUE;
}


BOOL FASTCALL co_UserShowCaret(PWINDOW_OBJECT Window OPTIONAL)
{
   PTHREADINFO pti;
   PUSER_MESSAGE_QUEUE ThreadQueue;

   if (Window) ASSERT_REFS_CO(Window);

   if(Window && Window->OwnerThread != PsGetCurrentThread())
   {
      SetLastWin32Error(ERROR_ACCESS_DENIED);
      return FALSE;
   }

   pti = PsGetCurrentThreadWin32Thread();
   ThreadQueue = pti->MessageQueue;

   if(Window && ThreadQueue->CaretInfo->hWnd != Window->hSelf)
   {
      SetLastWin32Error(ERROR_ACCESS_DENIED);
      return FALSE;
   }

   if(!ThreadQueue->CaretInfo->Visible)
   {
      ThreadQueue->CaretInfo->Visible = 1;
      if(!ThreadQueue->CaretInfo->Showing)
      {
         co_IntSendMessage(ThreadQueue->CaretInfo->hWnd, WM_SYSTIMER, IDCARETTIMER, 0);
      }
      IntSetTimer(ThreadQueue->CaretInfo->hWnd, IDCARETTIMER, IntGetCaretBlinkTime(), NULL, TRUE);
   }

   return TRUE;
}


/* SYSCALLS *****************************************************************/

BOOL
APIENTRY
NtUserCreateCaret(
   HWND hWnd,
   HBITMAP hBitmap,
   int nWidth,
   int nHeight)
{
   PWINDOW_OBJECT Window;
   PTHREADINFO pti;
   PUSER_MESSAGE_QUEUE ThreadQueue;
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserCreateCaret\n");
   UserEnterExclusive();

   if(!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN(FALSE);
   }

   if(Window->OwnerThread != PsGetCurrentThread())
   {
      SetLastWin32Error(ERROR_ACCESS_DENIED);
      RETURN(FALSE);
   }

   pti = PsGetCurrentThreadWin32Thread();
   ThreadQueue = pti->MessageQueue;

   if (ThreadQueue->CaretInfo->Visible)
   {
      IntKillTimer(hWnd, IDCARETTIMER, TRUE);
      co_IntHideCaret(ThreadQueue->CaretInfo);
   }

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

   RETURN(TRUE);

CLEANUP:
   DPRINT("Leave NtUserCreateCaret, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

UINT
APIENTRY
NtUserGetCaretBlinkTime(VOID)
{
   DECLARE_RETURN(UINT);

   DPRINT("Enter NtUserGetCaretBlinkTime\n");
   UserEnterShared();

   RETURN(IntGetCaretBlinkTime());

CLEANUP:
   DPRINT("Leave NtUserGetCaretBlinkTime, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

BOOL
APIENTRY
NtUserGetCaretPos(
   LPPOINT lpPoint)
{
   PTHREADINFO pti;
   PUSER_MESSAGE_QUEUE ThreadQueue;
   NTSTATUS Status;
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserGetCaretPos\n");
   UserEnterShared();

   pti = PsGetCurrentThreadWin32Thread();
   ThreadQueue = pti->MessageQueue;

   Status = MmCopyToCaller(lpPoint, &(ThreadQueue->CaretInfo->Pos), sizeof(POINT));
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
APIENTRY
NtUserShowCaret(HWND hWnd OPTIONAL)
{
   PWINDOW_OBJECT Window = NULL;
   USER_REFERENCE_ENTRY Ref;
   DECLARE_RETURN(BOOL);
   BOOL ret;

   DPRINT("Enter NtUserShowCaret\n");
   UserEnterExclusive();

   if(hWnd && !(Window = UserGetWindowObject(hWnd)))
   {
      RETURN(FALSE);
   }

   if (Window) UserRefObjectCo(Window, &Ref);

   ret = co_UserShowCaret(Window);

   if (Window) UserDerefObjectCo(Window);

   RETURN(ret);

CLEANUP:
   DPRINT("Leave NtUserShowCaret, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

BOOL
APIENTRY
NtUserHideCaret(HWND hWnd OPTIONAL)
{
   PWINDOW_OBJECT Window = NULL;
   USER_REFERENCE_ENTRY Ref;
   DECLARE_RETURN(BOOL);
   BOOL ret;

   DPRINT("Enter NtUserHideCaret\n");
   UserEnterExclusive();

   if(hWnd && !(Window = UserGetWindowObject(hWnd)))
   {
      RETURN(FALSE);
   }

   if (Window) UserRefObjectCo(Window, &Ref);

   ret = co_UserHideCaret(Window);

   if (Window) UserDerefObjectCo(Window);

   RETURN(ret);

CLEANUP:
   DPRINT("Leave NtUserHideCaret, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}
