/* $Id: message.c,v 1.6 2002/06/06 17:50:16 jfilby Exp $
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

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS
W32kInitMessageImpl(VOID)
{
  return(STATUS_SUCCESS);
}

NTSTATUS
W32kCleanupMessageImpl(VOID)
{
  return(STATUS_SUCCESS);
}


LRESULT STDCALL
NtUserDispatchMessage(LPMSG lpMsg)
{
  LRESULT Result;

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

  /* 
   * FIXME: Check for valid window handle, valid window and valid window 
   * proc. 
   */

  /* FIXME: Check for paint message. */

  Result = W32kCallWindowProc(NULL /* WndProc */,
			      lpMsg->hwnd,
			      lpMsg->message,
			      lpMsg->wParam,
			      lpMsg->lParam);

  /* FIXME: Check for paint message. */

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
      /* FIXME: Dispatch sent messages here. */
      
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

      /* FIXME: Check for sent messages again. */

      /* FIXME: Check for paint messages. */

      /* Nothing found so far. Wait for new messages. */
      Status = MsqWaitForNewMessages(ThreadQueue);
    }
  while (Status == STATUS_WAIT_0);
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
  NTSTATUS Status;
  BOOLEAN RemoveMessages;

  /* Initialize the thread's win32 state if necessary. */ 
  W32kGuiCheck();

  ThreadQueue = (PUSER_MESSAGE_QUEUE)PsGetWin32Thread()->MessageQueue;

  /* Inspect wRemoveMsg flags */
  /* FIXME: The only flag we process is PM_REMOVE - processing of others must still be implemented */
  RemoveMessages = wRemoveMsg & PM_REMOVE;

  /* FIXME: Dispatch sent messages here. */
      
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

  /* FIXME: Check for sent messages again. */

  /* FIXME: Check for paint messages. */

  return((BOOLEAN)(-1));
}

BOOL STDCALL
NtUserPostMessage(HWND hWnd,
		  UINT Msg,
		  WPARAM wParam,
		  LPARAM lParam)
{
  UNIMPLEMENTED;
    
  return 0;
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

BOOL STDCALL
NtUserSendMessage(HWND hWnd,
		  UINT Msg,
		  WPARAM Wparam,
		  LPARAM lParam)
{
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
  
  return 0;
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
