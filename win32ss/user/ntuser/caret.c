/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS Win32k subsystem
 * PURPOSE:          Caret functions
 * FILE:             win32ss/user/ntuser/caret.c
 * PROGRAMERS:       Thomas Weidenmueller (w3seek@users.sourceforge.net)
 *                   Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include <win32k.h>
DBG_DEFAULT_CHANNEL(UserCaret);

/* FUNCTIONS *****************************************************************/

VOID FASTCALL
co_IntDrawCaret(PWND pWnd, PTHRDCARETINFO CaretInfo)
{
    HDC hdc, hdcMem;
    HBITMAP hbmOld;
    RECT rcClient;
    BOOL bDone = FALSE;

    if (pWnd == NULL)
    {
       TRACE("Null Window!\n");
       return;
    }

    hdc = UserGetDCEx(pWnd, NULL, DCX_USESTYLE);
    if (!hdc)
    {
        ERR("GetDC failed\n");
        return;
    }

    if (pWnd->hrgnUpdate)
    {
       NtGdiSaveDC(hdc);
    }

    IntGetClientRect(pWnd, &rcClient);
    NtGdiIntersectClipRect(hdc,
                           rcClient.left,
                           rcClient.top,
                           rcClient.right,
                           rcClient.bottom);

    if (CaretInfo->Bitmap)
    {
        if (!GreGetBitmapDimension(CaretInfo->Bitmap, &CaretInfo->Size))
        {
            ERR("Failed to get bitmap dimensions\n");
            goto cleanup;
        }

        hdcMem = NtGdiCreateCompatibleDC(hdc);
        if (hdcMem)
        {
            hbmOld = NtGdiSelectBitmap(hdcMem, CaretInfo->Bitmap);
            bDone = NtGdiBitBlt(hdc,
                                CaretInfo->Pos.x,
                                CaretInfo->Pos.y,
                                CaretInfo->Size.cx,
                                CaretInfo->Size.cy,
                                hdcMem,
                                0,
                                0,
                                SRCINVERT,
                                0,
                                0);
            NtGdiSelectBitmap(hdcMem, hbmOld);
            GreDeleteObject(hdcMem);
        }
    }

    if (!bDone)
    {
        NtGdiPatBlt(hdc,
                    CaretInfo->Pos.x,
                    CaretInfo->Pos.y,
                    CaretInfo->Size.cx,
                    CaretInfo->Size.cy,
                    DSTINVERT);
    }

cleanup:
    if (pWnd->hrgnUpdate)
    {
       NtGdiRestoreDC(hdc, -1);
    }

    UserReleaseDC(pWnd, hdc, FALSE);
}

VOID
CALLBACK
CaretSystemTimerProc(HWND hwnd,
                     UINT uMsg,
                     UINT_PTR idEvent,
                     DWORD dwTime)
{
   PTHREADINFO pti;
   PUSER_MESSAGE_QUEUE ThreadQueue;
   PWND pWnd;

   pti = PsGetCurrentThreadWin32Thread();
   ThreadQueue = pti->MessageQueue;

   if (ThreadQueue->CaretInfo.hWnd != hwnd)
   {
      TRACE("Not the same caret window!\n");
      return;
   }

   if (hwnd)
   {
      pWnd = UserGetWindowObject(hwnd);
      if (!pWnd)
      {
         ERR("Caret System Timer Proc has invalid window handle! %p Id: %u\n", hwnd, idEvent);
         return;
      }
   }
   else
   {
      TRACE( "Windowless Caret Timer Running!\n" );
      return;
   }

   switch (idEvent)
   {
      case IDCARETTIMER:
      {
         ThreadQueue->CaretInfo.Showing = (ThreadQueue->CaretInfo.Showing ? 0 : 1);
         co_IntDrawCaret(pWnd, &ThreadQueue->CaretInfo);
      }
   }
   return;
}

static
BOOL FASTCALL
co_IntHideCaret(PTHRDCARETINFO CaretInfo)
{
   PWND pWnd;
   if(CaretInfo->hWnd && CaretInfo->Visible && CaretInfo->Showing)
   {
      pWnd = UserGetWindowObject(CaretInfo->hWnd);
      CaretInfo->Showing = 0;

      co_IntDrawCaret(pWnd, CaretInfo);
      IntNotifyWinEvent(EVENT_OBJECT_HIDE, pWnd, OBJID_CARET, CHILDID_SELF, 0);
      return TRUE;
   }
   return FALSE;
}

BOOL FASTCALL
co_IntDestroyCaret(PTHREADINFO Win32Thread)
{
   PUSER_MESSAGE_QUEUE ThreadQueue;
   PWND pWnd;
   ThreadQueue = Win32Thread->MessageQueue;

   if (!ThreadQueue)
      return FALSE;

   pWnd = ValidateHwndNoErr(ThreadQueue->CaretInfo.hWnd);
   co_IntHideCaret(&ThreadQueue->CaretInfo);
   ThreadQueue->CaretInfo.Bitmap = (HBITMAP)0;
   ThreadQueue->CaretInfo.hWnd = (HWND)0;
   ThreadQueue->CaretInfo.Size.cx = ThreadQueue->CaretInfo.Size.cy = 0;
   ThreadQueue->CaretInfo.Showing = 0;
   ThreadQueue->CaretInfo.Visible = 0;
   if (pWnd)
   {
      IntNotifyWinEvent(EVENT_OBJECT_DESTROY, pWnd, OBJID_CARET, CHILDID_SELF, 0);
   }
   return TRUE;
}

BOOL FASTCALL
IntSetCaretBlinkTime(UINT uMSeconds)
{
   /* Don't save the new value to the registry! */

   gpsi->dtCaretBlink = uMSeconds;

   return TRUE;
}

BOOL FASTCALL
co_IntSetCaretPos(int X, int Y)
{
   PTHREADINFO pti;
   PWND pWnd;
   PUSER_MESSAGE_QUEUE ThreadQueue;

   pti = PsGetCurrentThreadWin32Thread();
   ThreadQueue = pti->MessageQueue;

   if(ThreadQueue->CaretInfo.hWnd)
   {
      pWnd = UserGetWindowObject(ThreadQueue->CaretInfo.hWnd);
      if(ThreadQueue->CaretInfo.Pos.x != X || ThreadQueue->CaretInfo.Pos.y != Y)
      {
         co_IntHideCaret(&ThreadQueue->CaretInfo);
         ThreadQueue->CaretInfo.Pos.x = X;
         ThreadQueue->CaretInfo.Pos.y = Y;
         if (ThreadQueue->CaretInfo.Visible)
         {
            ThreadQueue->CaretInfo.Showing = 1;
            co_IntDrawCaret(pWnd, &ThreadQueue->CaretInfo);
         }

         IntSetTimer(pWnd, IDCARETTIMER, gpsi->dtCaretBlink, CaretSystemTimerProc, TMRF_SYSTEM);
         IntNotifyWinEvent(EVENT_OBJECT_LOCATIONCHANGE, pWnd, OBJID_CARET, CHILDID_SELF, 0);
      }
      return TRUE;
   }

   return FALSE;
}

/* Win: zzzHideCaret */
BOOL FASTCALL co_UserHideCaret(PWND Window OPTIONAL)
{
   PTHREADINFO pti;
   PUSER_MESSAGE_QUEUE ThreadQueue;

   if (Window) ASSERT_REFS_CO(Window);

   if(Window && Window->head.pti->pEThread != PsGetCurrentThread())
   {
      EngSetLastError(ERROR_ACCESS_DENIED);
      return FALSE;
   }

   pti = PsGetCurrentThreadWin32Thread();
   ThreadQueue = pti->MessageQueue;

   if(Window && ThreadQueue->CaretInfo.hWnd != UserHMGetHandle(Window))
   {
      EngSetLastError(ERROR_ACCESS_DENIED);
      return FALSE;
   }

   if(ThreadQueue->CaretInfo.Visible)
   {
      PWND pwnd = UserGetWindowObject(ThreadQueue->CaretInfo.hWnd);
      IntKillTimer(pwnd, IDCARETTIMER, TRUE);

      co_IntHideCaret(&ThreadQueue->CaretInfo);
      ThreadQueue->CaretInfo.Visible = 0;
      ThreadQueue->CaretInfo.Showing = 0;
   }

   return TRUE;
}

/* Win: zzzShowCaret */
BOOL FASTCALL co_UserShowCaret(PWND Window OPTIONAL)
{
   PTHREADINFO pti;
   PUSER_MESSAGE_QUEUE ThreadQueue;
   PWND pWnd = NULL;

   if (Window) ASSERT_REFS_CO(Window);

   if(Window && Window->head.pti->pEThread != PsGetCurrentThread())
   {
      EngSetLastError(ERROR_ACCESS_DENIED);
      return FALSE;
   }

   pti = PsGetCurrentThreadWin32Thread();
   ThreadQueue = pti->MessageQueue;

   if(Window && ThreadQueue->CaretInfo.hWnd != UserHMGetHandle(Window))
   {
      EngSetLastError(ERROR_ACCESS_DENIED);
      return FALSE;
   }

   if (!ThreadQueue->CaretInfo.Visible)
   {
      ThreadQueue->CaretInfo.Visible = 1;
      pWnd = ValidateHwndNoErr(ThreadQueue->CaretInfo.hWnd);
      if (!ThreadQueue->CaretInfo.Showing && pWnd)
      {
         IntNotifyWinEvent(EVENT_OBJECT_SHOW, pWnd, OBJID_CARET, OBJID_CARET, 0);
      }
      if ((INT)gpsi->dtCaretBlink > 0)
      {
          IntSetTimer(pWnd, IDCARETTIMER, gpsi->dtCaretBlink, CaretSystemTimerProc, TMRF_SYSTEM);
      }
      else if (ThreadQueue->CaretInfo.Visible)
      {
          ThreadQueue->CaretInfo.Showing = 1;
          co_IntDrawCaret(pWnd, &ThreadQueue->CaretInfo);
      }
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
   PWND Window;
   PTHREADINFO pti;
   PUSER_MESSAGE_QUEUE ThreadQueue;
   BOOL Ret = FALSE;

   TRACE("Enter NtUserCreateCaret\n");
   UserEnterExclusive();

   if(!(Window = UserGetWindowObject(hWnd)))
   {
      goto Exit; // Return FALSE
   }

   if(Window->head.pti->pEThread != PsGetCurrentThread())
   {
      EngSetLastError(ERROR_ACCESS_DENIED);
      goto Exit; // Return FALSE
   }

   pti = PsGetCurrentThreadWin32Thread();
   ThreadQueue = pti->MessageQueue;

   if (ThreadQueue->CaretInfo.Visible)
   {
      IntKillTimer(Window, IDCARETTIMER, TRUE);
      co_IntHideCaret(&ThreadQueue->CaretInfo);
   }

   ThreadQueue->CaretInfo.hWnd = hWnd;
   if(hBitmap)
   {
      ThreadQueue->CaretInfo.Bitmap = hBitmap;
      ThreadQueue->CaretInfo.Size.cx = ThreadQueue->CaretInfo.Size.cy = 0;
   }
   else
   {
      if (nWidth == 0)
      {
          nWidth = UserGetSystemMetrics(SM_CXBORDER);
      }
      if (nHeight == 0)
      {
          nHeight = UserGetSystemMetrics(SM_CYBORDER);
      }
      ThreadQueue->CaretInfo.Bitmap = (HBITMAP)0;
      ThreadQueue->CaretInfo.Size.cx = nWidth;
      ThreadQueue->CaretInfo.Size.cy = nHeight;
   }
   ThreadQueue->CaretInfo.Visible = 0;
   ThreadQueue->CaretInfo.Showing = 0;

   IntSetTimer(Window, IDCARETTIMER, gpsi->dtCaretBlink, CaretSystemTimerProc, TMRF_SYSTEM);

   IntNotifyWinEvent(EVENT_OBJECT_CREATE, Window, OBJID_CARET, CHILDID_SELF, 0);

   Ret = TRUE;

Exit:
   TRACE("Leave NtUserCreateCaret, ret=%i\n", Ret);
   UserLeave();
   return Ret;
}

UINT
APIENTRY
NtUserGetCaretBlinkTime(VOID)
{
   UINT ret;

   UserEnterShared();

   ret = gpsi->dtCaretBlink;

   UserLeave();

   return ret;
}

BOOL
APIENTRY
NtUserGetCaretPos(
   LPPOINT lpPoint)
{
   PTHREADINFO pti;
   PUSER_MESSAGE_QUEUE ThreadQueue;
   NTSTATUS Status;
   BOOL Ret = TRUE;

   TRACE("Enter NtUserGetCaretPos\n");
   UserEnterShared();

   pti = PsGetCurrentThreadWin32Thread();
   ThreadQueue = pti->MessageQueue;

   Status = MmCopyToCaller(lpPoint, &ThreadQueue->CaretInfo.Pos, sizeof(POINT));
   if(!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      Ret = FALSE;
   }

   TRACE("Leave NtUserGetCaretPos, ret=%i\n", Ret);
   UserLeave();
   return Ret;
}

BOOL
APIENTRY
NtUserShowCaret(HWND hWnd OPTIONAL)
{
   PWND Window = NULL;
   USER_REFERENCE_ENTRY Ref;
   BOOL ret = FALSE;

   TRACE("Enter NtUserShowCaret\n");
   UserEnterExclusive();

   if(hWnd && !(Window = UserGetWindowObject(hWnd)))
   {
      goto Exit; // Return FALSE
   }

   if (Window) UserRefObjectCo(Window, &Ref);

   ret = co_UserShowCaret(Window);

   if (Window) UserDerefObjectCo(Window);

Exit:
   TRACE("Leave NtUserShowCaret, ret=%i\n", ret);
   UserLeave();
   return ret;
}

BOOL
APIENTRY
NtUserHideCaret(HWND hWnd OPTIONAL)
{
   PWND Window = NULL;
   USER_REFERENCE_ENTRY Ref;
   BOOL ret = FALSE;

   TRACE("Enter NtUserHideCaret\n");
   UserEnterExclusive();

   if(hWnd && !(Window = UserGetWindowObject(hWnd)))
   {
      goto Exit; // Return FALSE
   }

   if (Window) UserRefObjectCo(Window, &Ref);

   ret = co_UserHideCaret(Window);

   if (Window) UserDerefObjectCo(Window);

Exit:
   TRACE("Leave NtUserHideCaret, ret=%i\n", ret);
   UserLeave();
   return ret;
}
