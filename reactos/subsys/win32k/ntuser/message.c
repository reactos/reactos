/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
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
/* $Id: message.c,v 1.19 2003/05/18 22:07:02 gvg Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Messages
 * FILE:             subsys/win32k/ntuser/message.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISION HISTORY:
 *       06-06-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <win32k/win32k.h>
#include <include/guicheck.h>
#include <include/msgqueue.h>
#include <include/window.h>
#include <include/class.h>
#include <include/error.h>
#include <include/object.h>
#include <include/winsta.h>
#include <include/callback.h>
#include <include/painting.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS FASTCALL
W32kInitMessageImpl(VOID)
{
  return(STATUS_SUCCESS);
}

NTSTATUS FASTCALL
W32kCleanupMessageImpl(VOID)
{
  return(STATUS_SUCCESS);
}


LRESULT STDCALL
NtUserDispatchMessage(CONST MSG* lpMsg)
{
  LRESULT Result;
  PWINDOW_OBJECT WindowObject;
  NTSTATUS Status;

  /* Process timer messages. */
  if (lpMsg->message == WM_TIMER)
    {
      if (lpMsg->lParam)
	{
	  /* FIXME: Call hooks. */

	  /* FIXME: Check for continuing validity of timer. */

	  return(W32kCallWindowProc((WNDPROC)lpMsg->lParam,
				      lpMsg->hwnd,
				      lpMsg->message,
				      lpMsg->wParam,
				      0 /* GetTickCount() */));
	}
    }

  /* Get the window object. */
  Status = 
    ObmReferenceObjectByHandle(PsGetWin32Process()->WindowStation->HandleTable,
			       lpMsg->hwnd,
			       otWindow,
			       (PVOID*)&WindowObject);
  if (!NT_SUCCESS(Status))
    {
      return(0);
    }

  /* FIXME: Call hook procedures. */

  /* Call the window procedure. */
  Result = W32kCallWindowProc(NULL /* WndProc */,
			      lpMsg->hwnd,
			      lpMsg->message,
			      lpMsg->wParam,
			      lpMsg->lParam);

  return(Result);
}

BOOL STDCALL
NtUserGetMessage(LPMSG lpMsg,
		 HWND hWnd,
		 UINT wMsgFilterMin,
		 UINT wMsgFilterMax)
/*
 * FUNCTION: Get a message from the calling thread's message queue.
 * ARGUMENTS:
 *      lpMsg - Pointer to the structure which receives the returned message.
 *      hWnd - Window whose messages are to be retrieved.
 *      wMsgFilterMin - Integer value of the lowest message value to be
 *                      retrieved.
 *      wMsgFilterMax - Integer value of the highest message value to be
 *                      retrieved.
 */
{
  PUSER_MESSAGE_QUEUE ThreadQueue;
  BOOLEAN Present;
  PUSER_MESSAGE Message;
  NTSTATUS Status;

  /* Initialize the thread's win32 state if necessary. */ 
  W32kGuiCheck();

  ThreadQueue = (PUSER_MESSAGE_QUEUE)PsGetWin32Thread()->MessageQueue;

  do
    {
      /* Dispatch sent messages here. */
      while (MsqDispatchOneSentMessage(ThreadQueue));
      
      /* Now look for a quit message. */
      /* FIXME: WINE checks the message number filter here. */
      if (ThreadQueue->QuitPosted)
	{
	  lpMsg->hwnd = hWnd;
	  lpMsg->message = WM_QUIT;
	  lpMsg->wParam = ThreadQueue->QuitExitCode;
	  lpMsg->lParam = 0;
	  ThreadQueue->QuitPosted = FALSE;
	  return(FALSE);
	}

      /* Now check for normal messages. */
      Present = MsqFindMessage(ThreadQueue,
			       FALSE,
			       TRUE,
			       hWnd,
			       wMsgFilterMin,
			       wMsgFilterMax,
			       &Message);
      if (Present)
	{
	  RtlCopyMemory(lpMsg, &Message->Msg, sizeof(MSG));
	  ExFreePool(Message);
	  return(TRUE);
	}

      /* Check for hardware events. */
      Present = MsqFindMessage(ThreadQueue,
			       TRUE,
			       TRUE,
			       hWnd,
			       wMsgFilterMin,
			       wMsgFilterMax,
			       &Message);
      if (Present)
	{
	  RtlCopyMemory(lpMsg, &Message->Msg, sizeof(MSG));
	  ExFreePool(Message);
	  return(TRUE);
	}

      /* Check for sent messages again. */
      while (MsqDispatchOneSentMessage(ThreadQueue));

      /* Check for paint messages. */
      if (ThreadQueue->PaintPosted)
	{
	  PWINDOW_OBJECT WindowObject;

	  lpMsg->hwnd = PaintingFindWinToRepaint(hWnd, PsGetWin32Thread());
	  lpMsg->message = WM_PAINT;
	  lpMsg->wParam = lpMsg->lParam = 0;

	  WindowObject = W32kGetWindowObject(lpMsg->hwnd);
	  if (WindowObject != NULL)
	    {
	      if (WindowObject->Style & WS_MINIMIZE &&
		  (HICON)NtUserGetClassLong(lpMsg->hwnd, GCL_HICON) != NULL)
		{
		  lpMsg->message = WM_PAINTICON;
		  lpMsg->wParam = 1;
		}

	      if (lpMsg->hwnd == NULL || lpMsg->hwnd == hWnd ||
		  W32kIsChildWindow(hWnd, lpMsg->hwnd))
		{
		  if (WindowObject->Flags & WINDOWOBJECT_NEED_INTERNALPAINT &&
		      WindowObject->UpdateRegion == NULL)
		    {
		      WindowObject->Flags &= ~WINDOWOBJECT_NEED_INTERNALPAINT;
		      MsqDecPaintCountQueue(WindowObject->MessageQueue);
		    }
		}
	      W32kReleaseWindowObject(WindowObject);
	    }

	  return(TRUE); 
	}

      /* Nothing found so far. Wait for new messages. */
      Status = MsqWaitForNewMessages(ThreadQueue);
    }
  while (Status >= STATUS_WAIT_0 && Status <= STATUS_WAIT_63);
  return((BOOLEAN)(-1));
}

DWORD
STDCALL
NtUserMessageCall(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4,
  DWORD Unknown5,
  DWORD Unknown6)
{
  UNIMPLEMENTED

  return 0;
}

BOOL STDCALL
NtUserPeekMessage(LPMSG lpMsg,
  HWND hWnd,
  UINT wMsgFilterMin,
  UINT wMsgFilterMax,
  UINT wRemoveMsg)
/*
 * FUNCTION: Get a message from the calling thread's message queue.
 * ARGUMENTS:
 *      lpMsg - Pointer to the structure which receives the returned message.
 *      hWnd - Window whose messages are to be retrieved.
 *      wMsgFilterMin - Integer value of the lowest message value to be
 *                      retrieved.
 *      wMsgFilterMax - Integer value of the highest message value to be
 *                      retrieved.
 *      wRemoveMsg - Specificies whether or not to remove messages from the queue after processing
 */
{
  PUSER_MESSAGE_QUEUE ThreadQueue;
  BOOLEAN Present;
  PUSER_MESSAGE Message;
  BOOLEAN RemoveMessages;

  /* Initialize the thread's win32 state if necessary. */ 
  W32kGuiCheck();

  ThreadQueue = (PUSER_MESSAGE_QUEUE)PsGetWin32Thread()->MessageQueue;

  /* Inspect wRemoveMsg flags */
  /* FIXME: The only flag we process is PM_REMOVE - processing of others must still be implemented */
  RemoveMessages = wRemoveMsg & PM_REMOVE;

  /* Dispatch sent messages here. */
  while (MsqDispatchOneSentMessage(ThreadQueue));
      
  /* Now look for a quit message. */
  /* FIXME: WINE checks the message number filter here. */
  if (ThreadQueue->QuitPosted)
  {
	  lpMsg->hwnd = hWnd;
	  lpMsg->message = WM_QUIT;
	  lpMsg->wParam = ThreadQueue->QuitExitCode;
	  lpMsg->lParam = 0;
	  ThreadQueue->QuitPosted = FALSE;
	  return(FALSE);
  }

  /* Now check for normal messages. */
  Present = MsqFindMessage(ThreadQueue,
       FALSE,
       RemoveMessages,
       hWnd,
       wMsgFilterMin,
       wMsgFilterMax,
       &Message);
  if (Present)
  {
	  RtlCopyMemory(lpMsg, &Message->Msg, sizeof(MSG));
	  ExFreePool(Message);
	  return(TRUE);
  }

  /* Check for hardware events. */
  Present = MsqFindMessage(ThreadQueue,
       TRUE,
       RemoveMessages,
       hWnd,
       wMsgFilterMin,
       wMsgFilterMax,
       &Message);
  if (Present)
  {
	  RtlCopyMemory(lpMsg, &Message->Msg, sizeof(MSG));
	  ExFreePool(Message);
	  return(TRUE);
  }

  /* Check for sent messages again. */
  while (MsqDispatchOneSentMessage(ThreadQueue));

  /* Check for paint messages. */

  /* Check for paint messages. */
  if (ThreadQueue->PaintPosted)
    {
      PWINDOW_OBJECT WindowObject;

      lpMsg->hwnd = PaintingFindWinToRepaint(hWnd, PsGetWin32Thread());
      lpMsg->message = WM_PAINT;
      lpMsg->wParam = lpMsg->lParam = 0;

      WindowObject = W32kGetWindowObject(lpMsg->hwnd);
      if (WindowObject != NULL)
	{
	  if (WindowObject->Style & WS_MINIMIZE &&
	      (HICON)NtUserGetClassLong(lpMsg->hwnd, GCL_HICON) != NULL)
	    {
	      lpMsg->message = WM_PAINTICON;
	      lpMsg->wParam = 1;
	    }

	  if (lpMsg->hwnd == NULL || lpMsg->hwnd == hWnd ||
	      W32kIsChildWindow(hWnd, lpMsg->hwnd))
	    {
	      if (WindowObject->Flags & WINDOWOBJECT_NEED_INTERNALPAINT &&
		  WindowObject->UpdateRegion == NULL)
		{
		  WindowObject->Flags &= ~WINDOWOBJECT_NEED_INTERNALPAINT;
		  MsqDecPaintCountQueue(WindowObject->MessageQueue);
		}
	    }
	  W32kReleaseWindowObject(WindowObject);
	}

      return(TRUE); 
    }

  return FALSE;
}

BOOL STDCALL
NtUserPostMessage(HWND hWnd,
		  UINT Msg,
		  WPARAM wParam,
		  LPARAM lParam)
{
  PUSER_MESSAGE_QUEUE ThreadQueue;

  switch (Msg)
  {
    case WM_NULL:
      break;

    case WM_QUIT:
      ThreadQueue = (PUSER_MESSAGE_QUEUE)PsGetWin32Thread()->MessageQueue;
      ThreadQueue->QuitPosted = TRUE;
      ThreadQueue->QuitExitCode = wParam;
      break;

    default:
      DPRINT1("Unhandled message: %u\n", Msg);
      return FALSE;
  }

  return TRUE;
}

BOOL STDCALL
NtUserPostThreadMessage(DWORD idThread,
			UINT Msg,
			WPARAM wParam,
			LPARAM lParam)
{
  UNIMPLEMENTED;

  return 0;
}

DWORD STDCALL
NtUserQuerySendMessage(DWORD Unknown0)
{
  UNIMPLEMENTED;

  return 0;
}

LRESULT STDCALL
W32kSendMessage(HWND hWnd,
		UINT Msg,
		WPARAM wParam,
		LPARAM lParam,
		BOOL KernelMessage)
{
  LRESULT Result;
  NTSTATUS Status;
  PWINDOW_OBJECT Window;

  /* FIXME: Check for a broadcast or topmost destination. */

  /* FIXME: Call hooks. */

  Status = 
    ObmReferenceObjectByHandle(PsGetWin32Process()->WindowStation->HandleTable,
			       hWnd,
			       otWindow,
			       (PVOID*)&Window);
  if (!NT_SUCCESS(Status))
    {
      return(FALSE);
    }

  /* FIXME: Check for an exiting window. */

  if (NULL != PsGetWin32Thread() &&
      Window->MessageQueue == PsGetWin32Thread()->MessageQueue)
    {
      if (KernelMessage)
	{
	  Result = W32kCallTrampolineWindowProc(NULL, hWnd, Msg, wParam,
						lParam);
	  return(Result);
	}
      else
	{
	  Result = W32kCallWindowProc(NULL, hWnd, Msg, wParam, lParam);
	  return(Result);
	}
    }
  else
    {
      PUSER_SENT_MESSAGE Message;
      PKEVENT CompletionEvent;

      CompletionEvent = ExAllocatePool(NonPagedPool, sizeof(KEVENT));
      KeInitializeEvent(CompletionEvent, NotificationEvent, FALSE);

      Message = ExAllocatePool(NonPagedPool, sizeof(USER_SENT_MESSAGE));
      Message->Msg.hwnd = hWnd;
      Message->Msg.message = Msg;
      Message->Msg.wParam = wParam;
      Message->Msg.lParam = lParam;
      Message->CompletionEvent = CompletionEvent;
      Message->Result = &Result;
      Message->CompletionQueue = NULL;
      Message->CompletionCallback = NULL;
      MsqSendMessage(Window->MessageQueue, Message);

      ObmDereferenceObject(Window);
      Status = KeWaitForSingleObject(CompletionEvent,
				     UserRequest,
				     UserMode,
				     FALSE,
				     NULL);
      if (Status == STATUS_WAIT_0)
	{
	  return(Result);
	}
      else
	{
	  return(FALSE);
	}
    }
}

LRESULT STDCALL
NtUserSendMessage(HWND hWnd,
		  UINT Msg,
		  WPARAM wParam,
		  LPARAM lParam)
{
  return(W32kSendMessage(hWnd, Msg, wParam, lParam, FALSE));
}

BOOL STDCALL
NtUserSendMessageCallback(HWND hWnd,
			  UINT Msg,
			  WPARAM wParam,
			  LPARAM lParam,
			  SENDASYNCPROC lpCallBack,
			  ULONG_PTR dwData)
{
  UNIMPLEMENTED;

  return(0);
}

BOOL STDCALL
NtUserSendNotifyMessage(HWND hWnd,
			UINT Msg,
			WPARAM wParam,
			LPARAM lParam)
{
  UNIMPLEMENTED;

  return 0;
}

BOOL STDCALL
NtUserWaitMessage(VOID)
{
  UNIMPLEMENTED;

  return 0;
}

/* EOF */
