/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Message queues
 * FILE:             subsystems/win32/win32k/ntuser/msgqueue.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
                     Alexandre Julliard
                     Maarten Lankhorst
 * REVISION HISTORY:
 *       06-06-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

static PAGED_LOOKASIDE_LIST MessageLookasideList;
MOUSEMOVEPOINT MouseHistoryOfMoves[64];
INT gcur_count = 0;

/* FUNCTIONS *****************************************************************/

INIT_FUNCTION
NTSTATUS
NTAPI
MsqInitializeImpl(VOID)
{
   ExInitializePagedLookasideList(&MessageLookasideList,
                                  NULL,
                                  NULL,
                                  0,
                                  sizeof(USER_MESSAGE),
                                  TAG_USRMSG,
                                  256);

   return(STATUS_SUCCESS);
}

DWORD FASTCALL UserGetKeyState(DWORD key)
{
   DWORD ret = 0;
   PTHREADINFO pti;
   PUSER_MESSAGE_QUEUE MessageQueue;

   pti = PsGetCurrentThreadWin32Thread();
   MessageQueue = pti->MessageQueue;

   if( key < 0x100 )
   {
       ret = (DWORD)MessageQueue->KeyState[key];
       if (MessageQueue->KeyState[key] & KS_DOWN_BIT)
          ret |= 0xFF00; // If down, windows returns 0xFF80. 
   }
   else
   {
      EngSetLastError(ERROR_INVALID_PARAMETER);
   }
   return ret;
}

/* change the input key state for a given key */
static void set_input_key_state( PUSER_MESSAGE_QUEUE MessageQueue, UCHAR key, BOOL down )
{
    DPRINT("set_input_key_state key:%d, down:%d\n", key, down);

    if (down)
    {
        if (!(MessageQueue->KeyState[key] & KS_DOWN_BIT))
        {
            MessageQueue->KeyState[key] ^= KS_LOCK_BIT;
        }
        MessageQueue->KeyState[key] |= KS_DOWN_BIT;
    }
    else
    {
        MessageQueue->KeyState[key] &= ~KS_DOWN_BIT;
    }
}

/* update the input key state for a keyboard message */
static void update_input_key_state( PUSER_MESSAGE_QUEUE MessageQueue, MSG* msg )
{
    UCHAR key;
    BOOL down = 0;

    DPRINT("update_input_key_state message:%d\n", msg->message);

    switch (msg->message)
    {
    case WM_LBUTTONDOWN:
        down = 1;
        /* fall through */
    case WM_LBUTTONUP:
        set_input_key_state( MessageQueue, VK_LBUTTON, down );
        break;
    case WM_MBUTTONDOWN:
        down = 1;
        /* fall through */
    case WM_MBUTTONUP:
        set_input_key_state( MessageQueue, VK_MBUTTON, down );
        break;
    case WM_RBUTTONDOWN:
        down = 1;
        /* fall through */
    case WM_RBUTTONUP:
        set_input_key_state( MessageQueue, VK_RBUTTON, down );
        break;
    case WM_XBUTTONDOWN:
        down = 1;
        /* fall through */
    case WM_XBUTTONUP:
        if (msg->wParam == XBUTTON1)
            set_input_key_state( MessageQueue, VK_XBUTTON1, down );
        else if (msg->wParam == XBUTTON2)
            set_input_key_state( MessageQueue, VK_XBUTTON2, down );
        break;
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        down = 1;
        /* fall through */
    case WM_KEYUP:
    case WM_SYSKEYUP:
        key = (UCHAR)msg->wParam;
        set_input_key_state( MessageQueue, key, down );
        switch(key)
        {
        case VK_LCONTROL:
        case VK_RCONTROL:
            down = (MessageQueue->KeyState[VK_LCONTROL] | MessageQueue->KeyState[VK_RCONTROL]) & KS_DOWN_BIT;
            set_input_key_state( MessageQueue, VK_CONTROL, down );
            break;
        case VK_LMENU:
        case VK_RMENU:
            down = (MessageQueue->KeyState[VK_LMENU] | MessageQueue->KeyState[VK_RMENU]) & KS_DOWN_BIT;
            set_input_key_state( MessageQueue, VK_MENU, down );
            break;
        case VK_LSHIFT:
        case VK_RSHIFT:
            down = (MessageQueue->KeyState[VK_LSHIFT] | MessageQueue->KeyState[VK_RSHIFT]) & KS_DOWN_BIT;
            set_input_key_state( MessageQueue, VK_SHIFT, down );
            break;
        }
        break;
    }
}

HANDLE FASTCALL
IntMsqSetWakeMask(DWORD WakeMask)
{
   PTHREADINFO Win32Thread;
   PUSER_MESSAGE_QUEUE MessageQueue;
   HANDLE MessageEventHandle;
   DWORD dwFlags = HIWORD(WakeMask);

   Win32Thread = PsGetCurrentThreadWin32Thread();
   if (Win32Thread == NULL || Win32Thread->MessageQueue == NULL)
      return 0;

   MessageQueue = Win32Thread->MessageQueue;
// Win32Thread->pEventQueueServer; IntMsqSetWakeMask returns Win32Thread->hEventQueueClient
   MessageEventHandle = MessageQueue->NewMessagesHandle;

   if (Win32Thread->pcti)
   {
      if ( (Win32Thread->pcti->fsChangeBits & LOWORD(WakeMask)) ||
           ( (dwFlags & MWMO_INPUTAVAILABLE) && (Win32Thread->pcti->fsWakeBits & LOWORD(WakeMask)) ) )
      {
         DPRINT1("Chg 0x%x Wake 0x%x Mask 0x%x\n",Win32Thread->pcti->fsChangeBits, Win32Thread->pcti->fsWakeBits, WakeMask);
         KeSetEvent(MessageQueue->NewMessages, IO_NO_INCREMENT, FALSE); // Wake it up!
         return MessageEventHandle;
      }
   }

   IdlePing();

   return MessageEventHandle;
}

BOOL FASTCALL
IntMsqClearWakeMask(VOID)
{
   PTHREADINFO Win32Thread;

   Win32Thread = PsGetCurrentThreadWin32Thread();
   if (Win32Thread == NULL || Win32Thread->MessageQueue == NULL)
      return FALSE;
   // Very hacky, but that is what they do.
   Win32Thread->pcti->fsWakeBits = 0;

   IdlePong();

   return TRUE;
}

/*
   Due to the uncertainty of knowing what was set in our multilevel message queue,
   and even if the bits are all cleared. The same as cTimers/cPaintsReady.
   I think this is the best solution... (jt) */
VOID FASTCALL
MsqWakeQueue(PUSER_MESSAGE_QUEUE Queue, DWORD MessageBits, BOOL KeyEvent)
{
   PTHREADINFO pti;

   pti = Queue->Thread->Tcb.Win32Thread;
   pti->pcti->fsWakeBits |= MessageBits;
   pti->pcti->fsChangeBits |= MessageBits;

   // Start bit accounting to help clear the main set of bits.
   if (MessageBits & QS_KEY)         Queue->nCntsQBits[QSRosKey]++;
   if (MessageBits & QS_MOUSEMOVE)   Queue->nCntsQBits[QSRosMouseMove]++;
   if (MessageBits & QS_MOUSEBUTTON) Queue->nCntsQBits[QSRosMouseButton]++;
   if (MessageBits & QS_POSTMESSAGE) Queue->nCntsQBits[QSRosPostMessage]++;
   if (MessageBits & QS_SENDMESSAGE) Queue->nCntsQBits[QSRosSendMessage]++;
   if (MessageBits & QS_HOTKEY)      Queue->nCntsQBits[QSRosHotKey]++;

   if (KeyEvent)
      KeSetEvent(Queue->NewMessages, IO_NO_INCREMENT, FALSE);
}

VOID FASTCALL
ClearMsgBitsMask(PUSER_MESSAGE_QUEUE Queue, UINT MessageBits)
{
   PTHREADINFO pti;
   UINT ClrMask = 0;

   pti = Queue->Thread->Tcb.Win32Thread;

   if (MessageBits & QS_KEY)
   {
      if (--Queue->nCntsQBits[QSRosKey] == 0) ClrMask |= QS_KEY;
   }
   if (MessageBits & QS_MOUSEMOVE) // ReactOS hard coded.
   {  // Account for tracking mouse moves..
      if (--Queue->nCntsQBits[QSRosMouseMove] == 0) ClrMask |= QS_MOUSEMOVE;
      // Handle mouse move bits here.
      if (Queue->MouseMoved) ClrMask |= QS_MOUSEMOVE;
   }
   if (MessageBits & QS_MOUSEBUTTON)
   {
      if (--Queue->nCntsQBits[QSRosMouseButton] == 0) ClrMask |= QS_MOUSEBUTTON;
   }
   if (MessageBits & QS_POSTMESSAGE)
   {
      if (--Queue->nCntsQBits[QSRosPostMessage] == 0) ClrMask |= QS_POSTMESSAGE;
   }
   if (MessageBits & QS_TIMER) // ReactOS hard coded.
   {  // Handle timer bits here.
      if ( pti->cTimersReady )
      {
         if (--pti->cTimersReady == 0) ClrMask |= QS_TIMER;
      }
   }
   if (MessageBits & QS_PAINT) // ReactOS hard coded.
   {  // Handle paint bits here.
      if ( pti->cPaintsReady )
      {
         if (--pti->cPaintsReady == 0) ClrMask |= QS_PAINT;
      }
   }
   if (MessageBits & QS_SENDMESSAGE)
   {
      if (--Queue->nCntsQBits[QSRosSendMessage] == 0) ClrMask |= QS_SENDMESSAGE;
   }
   if (MessageBits & QS_HOTKEY)
   {
      if (--Queue->nCntsQBits[QSRosHotKey] == 0) ClrMask |= QS_HOTKEY;
   }

   pti->pcti->fsWakeBits &= ~ClrMask;
   pti->pcti->fsChangeBits &= ~ClrMask;
}

VOID FASTCALL
MsqIncPaintCountQueue(PUSER_MESSAGE_QUEUE Queue)
{
   PTHREADINFO pti;
   pti = Queue->Thread->Tcb.Win32Thread;
   pti->cPaintsReady++;
   MsqWakeQueue(Queue, QS_PAINT, TRUE);
}

VOID FASTCALL
MsqDecPaintCountQueue(PUSER_MESSAGE_QUEUE Queue)
{
   ClearMsgBitsMask(Queue, QS_PAINT);
}

VOID FASTCALL
MsqPostMouseMove(PUSER_MESSAGE_QUEUE MessageQueue, MSG* Msg)
{
    MessageQueue->MouseMoveMsg = *Msg;
    MessageQueue->MouseMoved = TRUE;
    MsqWakeQueue(MessageQueue, QS_MOUSEMOVE, TRUE);
}

VOID FASTCALL
co_MsqInsertMouseMessage(MSG* Msg, DWORD flags, ULONG_PTR dwExtraInfo, BOOL Hook)
{
   LARGE_INTEGER LargeTickCount;
   MSLLHOOKSTRUCT MouseHookData;
   PDESKTOP pDesk;
   PWND pwnd, pwndDesktop;

   KeQueryTickCount(&LargeTickCount);
   Msg->time = MsqCalculateMessageTime(&LargeTickCount);

   MouseHookData.pt.x = LOWORD(Msg->lParam);
   MouseHookData.pt.y = HIWORD(Msg->lParam);
   switch(Msg->message)
   {
      case WM_MOUSEWHEEL:
         MouseHookData.mouseData = MAKELONG(0, GET_WHEEL_DELTA_WPARAM(Msg->wParam));
         break;
      case WM_XBUTTONDOWN:
      case WM_XBUTTONUP:
      case WM_XBUTTONDBLCLK:
      case WM_NCXBUTTONDOWN:
      case WM_NCXBUTTONUP:
      case WM_NCXBUTTONDBLCLK:
         MouseHookData.mouseData = MAKELONG(0, HIWORD(Msg->wParam));
         break;
      default:
         MouseHookData.mouseData = 0;
         break;
   }

   MouseHookData.flags = flags; // LLMHF_INJECTED
   MouseHookData.time = Msg->time;
   MouseHookData.dwExtraInfo = dwExtraInfo;

   /* If the hook procedure returned non zero, dont send the message */
   if (Hook)
   {
      if (co_HOOK_CallHooks(WH_MOUSE_LL, HC_ACTION, Msg->message, (LPARAM) &MouseHookData))
         return;
   }

   /* Get the desktop window */
   pwndDesktop = UserGetDesktopWindow();
   if(!pwndDesktop)
       return;

   /* Set hit somewhere on the desktop */
   pDesk = pwndDesktop->head.rpdesk;
   pDesk->htEx = HTNOWHERE;
   pDesk->spwndTrack = pwndDesktop;

   /* Check if the mouse is captured */
   Msg->hwnd = IntGetCaptureWindow();
   if(Msg->hwnd != NULL)
   {
       pwnd = UserGetWindowObject(Msg->hwnd);
       if ((pwnd->style & WS_VISIBLE) &&
            IntPtInWindow(pwnd, Msg->pt.x, Msg->pt.y))
       {
          pDesk->htEx = HTCLIENT;
          pDesk->spwndTrack = pwnd;
       }
   }
   else
   {
       /* Loop all top level windows to find which one should receive input */
       for( pwnd = pwndDesktop->spwndChild;
            pwnd != NULL;
            pwnd = pwnd->spwndNext )
       {
           if ( pwnd->state2 & WNDS2_INDESTROY || pwnd->state & WNDS_DESTROYED )
           {
              DPRINT("The Window is in DESTROY!\n");
              continue;
           }

           if((pwnd->style & WS_VISIBLE) &&
              IntPtInWindow(pwnd, Msg->pt.x, Msg->pt.y))
           {
               Msg->hwnd = pwnd->head.h;
               pDesk->htEx = HTCLIENT;
               pDesk->spwndTrack = pwnd;
               break;
           }
       }
   }

   /* Check if we found a window */
   if(Msg->hwnd != NULL && pwnd != NULL)
   {
       if(Msg->message == WM_MOUSEMOVE)
       {
           /* Mouse move is a special case*/
           MsqPostMouseMove(pwnd->head.pti->MessageQueue, Msg);
       }
       else
       {
           DPRINT("Posting mouse message to hwnd=0x%x!\n", UserHMGetHandle(pwnd));
           MsqPostMessage(pwnd->head.pti->MessageQueue, Msg, TRUE, QS_MOUSEBUTTON);
       }
   }

   /* Do GetMouseMovePointsEx FIFO. */
   MouseHistoryOfMoves[gcur_count].x = Msg->pt.x;
   MouseHistoryOfMoves[gcur_count].y = Msg->pt.y;
   MouseHistoryOfMoves[gcur_count].time = Msg->time;
   MouseHistoryOfMoves[gcur_count].dwExtraInfo = dwExtraInfo;
   if (gcur_count++ == 64) gcur_count = 0; // 0 - 63 is 64, FIFO forwards.
}

//
// Note: Only called from input.c.
//
VOID FASTCALL
co_MsqPostKeyboardMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   PUSER_MESSAGE_QUEUE FocusMessageQueue;
   MSG Msg;
   LARGE_INTEGER LargeTickCount;
   KBDLLHOOKSTRUCT KbdHookData;

   DPRINT("MsqPostKeyboardMessage(uMsg 0x%x, wParam 0x%x, lParam 0x%x)\n",
          uMsg, wParam, lParam);

   // Condition may arise when calling MsqPostMessage and waiting for an event.
   ASSERT(UserIsEntered());

   FocusMessageQueue = IntGetFocusMessageQueue();

   Msg.hwnd = 0;

   if (FocusMessageQueue && (FocusMessageQueue->FocusWindow != (HWND)0))
       Msg.hwnd = FocusMessageQueue->FocusWindow;

   Msg.message = uMsg;
   Msg.wParam = wParam;
   Msg.lParam = lParam;

   KeQueryTickCount(&LargeTickCount);
   Msg.time = MsqCalculateMessageTime(&LargeTickCount);

   /* We can't get the Msg.pt point here since we don't know thread
      (and thus the window station) the message will end up in yet. */

   KbdHookData.vkCode = Msg.wParam;
   KbdHookData.scanCode = (Msg.lParam >> 16) & 0xff;
   KbdHookData.flags = (0 == (Msg.lParam & 0x01000000) ? 0 : LLKHF_EXTENDED) |
                       (0 == (Msg.lParam & 0x20000000) ? 0 : LLKHF_ALTDOWN) |
                       (0 == (Msg.lParam & 0x80000000) ? 0 : LLKHF_UP);
   KbdHookData.time = Msg.time;
   KbdHookData.dwExtraInfo = 0;
   if (co_HOOK_CallHooks(WH_KEYBOARD_LL, HC_ACTION, Msg.message, (LPARAM) &KbdHookData))
   {
      DPRINT1("Kbd msg %d wParam %d lParam 0x%08x dropped by WH_KEYBOARD_LL hook\n",
             Msg.message, Msg.wParam, Msg.lParam);
      return;
   }

   if (FocusMessageQueue == NULL)
   {
         DPRINT("No focus message queue\n");
         return;
   }

   if (FocusMessageQueue->FocusWindow != (HWND)0)
   {
         Msg.hwnd = FocusMessageQueue->FocusWindow;
         DPRINT("Msg.hwnd = %x\n", Msg.hwnd);

         FocusMessageQueue->Desktop->pDeskInfo->LastInputWasKbd = TRUE;

         Msg.pt = gpsi->ptCursor;
         MsqPostMessage(FocusMessageQueue, &Msg, TRUE, QS_KEY);
   }
   else
   {
         DPRINT("Invalid focus window handle\n");
   }

   return;
}

VOID FASTCALL
MsqPostHotKeyMessage(PVOID Thread, HWND hWnd, WPARAM wParam, LPARAM lParam)
{
   PWND Window;
   PTHREADINFO Win32Thread;
   MSG Mesg;
   LARGE_INTEGER LargeTickCount;
   NTSTATUS Status;
   INT id;
   DWORD Type;

   Status = ObReferenceObjectByPointer (Thread,
                                        THREAD_ALL_ACCESS,
                                        PsThreadType,
                                        KernelMode);
   if (!NT_SUCCESS(Status))
      return;

   Win32Thread = ((PETHREAD)Thread)->Tcb.Win32Thread;
   if (Win32Thread == NULL || Win32Thread->MessageQueue == NULL)
   {
      ObDereferenceObject ((PETHREAD)Thread);
      return;
   }

   Window = IntGetWindowObject(hWnd);
   if (!Window)
   {
      ObDereferenceObject ((PETHREAD)Thread);
      return;
   }

   id = wParam; // Check for hot keys unrelated to the hot keys set by RegisterHotKey.

   Mesg.hwnd    = hWnd;
   Mesg.message = id != IDHOT_REACTOS ? WM_HOTKEY : WM_SYSCOMMAND;
   Mesg.wParam  = id != IDHOT_REACTOS ? wParam    : SC_HOTKEY;
   Mesg.lParam  = id != IDHOT_REACTOS ? lParam    : (LPARAM)hWnd;
   Type         = id != IDHOT_REACTOS ? QS_HOTKEY : QS_POSTMESSAGE;
   KeQueryTickCount(&LargeTickCount);
   Mesg.time    = MsqCalculateMessageTime(&LargeTickCount);
   Mesg.pt      = gpsi->ptCursor;
   MsqPostMessage(Window->head.pti->MessageQueue, &Mesg, FALSE, Type);
   UserDereferenceObject(Window);
   ObDereferenceObject (Thread);

}

PUSER_MESSAGE FASTCALL
MsqCreateMessage(LPMSG Msg)
{
   PUSER_MESSAGE Message;

   Message = ExAllocateFromPagedLookasideList(&MessageLookasideList);
   if (!Message)
   {
      return NULL;
   }

   RtlMoveMemory(&Message->Msg, Msg, sizeof(MSG));

   return Message;
}

VOID FASTCALL
MsqDestroyMessage(PUSER_MESSAGE Message)
{
   ExFreeToPagedLookasideList(&MessageLookasideList, Message);
}

BOOLEAN FASTCALL
co_MsqDispatchOneSentMessage(PUSER_MESSAGE_QUEUE MessageQueue)
{
   PUSER_SENT_MESSAGE SaveMsg, Message;
   PLIST_ENTRY Entry;
   PTHREADINFO pti;
   LRESULT Result = 0;

   if (IsListEmpty(&MessageQueue->SentMessagesListHead))
   {
      return(FALSE);
   }

   /* remove it from the list of pending messages */
   Entry = RemoveHeadList(&MessageQueue->SentMessagesListHead);
   Message = CONTAINING_RECORD(Entry, USER_SENT_MESSAGE, ListEntry);

   pti = MessageQueue->Thread->Tcb.Win32Thread;

   SaveMsg = pti->pusmCurrent;
   pti->pusmCurrent = Message;

   // Processing a message sent to it from another thread.
   if ( ( Message->SenderQueue && MessageQueue != Message->SenderQueue) ||
        ( Message->CallBackSenderQueue && MessageQueue != Message->CallBackSenderQueue ))
   {  // most likely, but, to be sure.
      pti->pcti->CTI_flags |= CTI_INSENDMESSAGE; // Let the user know...
   }

   /* insert it to the list of messages that are currently dispatched by this
      message queue */
   InsertTailList(&MessageQueue->LocalDispatchingMessagesHead,
                  &Message->ListEntry);

   ClearMsgBitsMask(MessageQueue, Message->QS_Flags);

   if (Message->HookMessage == MSQ_ISHOOK)
   {  // Direct Hook Call processor
      Result = co_CallHook( Message->Msg.message,     // HookId
                           (INT)(INT_PTR)Message->Msg.hwnd, // Code
                            Message->Msg.wParam,
                            Message->Msg.lParam);
   }
   else if (Message->HookMessage == MSQ_ISEVENT)
   {  // Direct Event Call processor
      Result = co_EVENT_CallEvents( Message->Msg.message,
                                    Message->Msg.hwnd,
                                    Message->Msg.wParam,
                                    Message->Msg.lParam);
   }
   else if ((Message->CompletionCallback)
       && (Message->CallBackSenderQueue == MessageQueue))
   {   /* Call the callback routine */
      if (Message->QS_Flags & QS_SMRESULT)
      {
         co_IntCallSentMessageCallback(Message->CompletionCallback,
                                       Message->Msg.hwnd,
                                       Message->Msg.message,
                                       Message->CompletionCallbackContext,
                                       Message->lResult);
         /* Set callback to NULL to prevent reentry */
         Message->CompletionCallback = NULL;
      }
      else
      {
         /* The message has not been processed yet, reinsert it. */
         RemoveEntryList(&Message->ListEntry);
         InsertTailList(&Message->CallBackSenderQueue->SentMessagesListHead, &Message->ListEntry);
         DPRINT("Callback Message not processed yet. Requeuing the message\n");
         return (FALSE);
      }
   }
   else
   {  /* Call the window procedure. */
      Result = co_IntSendMessage( Message->Msg.hwnd,
                                  Message->Msg.message,
                                  Message->Msg.wParam,
                                  Message->Msg.lParam);
   }

   /* remove the message from the local dispatching list, because it doesn't need
      to be cleaned up on thread termination anymore */
   RemoveEntryList(&Message->ListEntry);

   /* If the message is a callback, insert it in the callback senders MessageQueue */
   if (Message->CompletionCallback)
   {
      if (Message->CallBackSenderQueue)
      {
         Message->lResult = Result;
         Message->QS_Flags |= QS_SMRESULT;

         /* insert it in the callers message queue */
         InsertTailList(&Message->CallBackSenderQueue->SentMessagesListHead, &Message->ListEntry);
         MsqWakeQueue(Message->CallBackSenderQueue, QS_SENDMESSAGE, TRUE);
         IntDereferenceMessageQueue(Message->CallBackSenderQueue);
      }
      return (TRUE);
   }

   /* remove the message from the dispatching list if needed, so lock the sender's message queue */
   if (Message->SenderQueue)
   {
      if (Message->DispatchingListEntry.Flink != NULL)
      {
         /* only remove it from the dispatching list if not already removed by a timeout */
         RemoveEntryList(&Message->DispatchingListEntry);
      }
   }
   /* still keep the sender's message queue locked, so the sender can't exit the
      MsqSendMessage() function (if timed out) */

   if (Message->QS_Flags & QS_SMRESULT)
   {
      Result = Message->lResult;
   }

   /* Let the sender know the result. */
   if (Message->Result != NULL)
   {
      *Message->Result = Result;
   }

   if (Message->HasPackedLParam == TRUE)
   {
      if (Message->Msg.lParam)
         ExFreePool((PVOID)Message->Msg.lParam);
   }

   /* Notify the sender. */
   if (Message->CompletionEvent != NULL)
   {
      KeSetEvent(Message->CompletionEvent, IO_NO_INCREMENT, FALSE);
   }

   /* if the message has a sender */
   if (Message->SenderQueue)
   {
       /* dereference our and the sender's message queue */
      IntDereferenceMessageQueue(Message->SenderQueue);
      IntDereferenceMessageQueue(MessageQueue);
   }

   /* free the message */
   ExFreePoolWithTag(Message, TAG_USRMSG);

   /* do not hangup on the user if this is reentering */
   if (!SaveMsg) pti->pcti->CTI_flags &= ~CTI_INSENDMESSAGE;
   pti->pusmCurrent = SaveMsg;

   return(TRUE);
}

VOID APIENTRY
MsqRemoveWindowMessagesFromQueue(PVOID pWindow)
{
   PUSER_SENT_MESSAGE SentMessage;
   PUSER_MESSAGE PostedMessage;
   PUSER_MESSAGE_QUEUE MessageQueue;
   PLIST_ENTRY CurrentEntry, ListHead;
   PWND Window = pWindow;

   ASSERT(Window);

   MessageQueue = Window->head.pti->MessageQueue;
   ASSERT(MessageQueue);

   /* remove the posted messages for this window */
   CurrentEntry = MessageQueue->PostedMessagesListHead.Flink;
   ListHead = &MessageQueue->PostedMessagesListHead;
   while (CurrentEntry != ListHead)
   {
      PostedMessage = CONTAINING_RECORD(CurrentEntry, USER_MESSAGE,
                                        ListEntry);
      if (PostedMessage->Msg.hwnd == Window->head.h)
      {
         RemoveEntryList(&PostedMessage->ListEntry);
         ClearMsgBitsMask(MessageQueue, PostedMessage->QS_Flags);
         MsqDestroyMessage(PostedMessage);
         CurrentEntry = MessageQueue->PostedMessagesListHead.Flink;
      }
      else
      {
         CurrentEntry = CurrentEntry->Flink;
      }
   }

   /* remove the sent messages for this window */
   CurrentEntry = MessageQueue->SentMessagesListHead.Flink;
   ListHead = &MessageQueue->SentMessagesListHead;
   while (CurrentEntry != ListHead)
   {
      SentMessage = CONTAINING_RECORD(CurrentEntry, USER_SENT_MESSAGE,
                                      ListEntry);
      if(SentMessage->Msg.hwnd == Window->head.h)
      {
         DPRINT("Notify the sender and remove a message from the queue that had not been dispatched\n");

         RemoveEntryList(&SentMessage->ListEntry);
         ClearMsgBitsMask(MessageQueue, SentMessage->QS_Flags);

         /* if it is a callback and this queue is not the sender queue, dereference queue */
         if ((SentMessage->CompletionCallback) && (SentMessage->CallBackSenderQueue != MessageQueue))
         {
            IntDereferenceMessageQueue(SentMessage->CallBackSenderQueue);
         }
         /* Only if the message has a sender was the queue referenced */
         if ((SentMessage->SenderQueue)
            && (SentMessage->DispatchingListEntry.Flink != NULL))
         {
            RemoveEntryList(&SentMessage->DispatchingListEntry);
         }

         /* wake the sender's thread */
         if (SentMessage->CompletionEvent != NULL)
         {
            KeSetEvent(SentMessage->CompletionEvent, IO_NO_INCREMENT, FALSE);
         }

         if (SentMessage->HasPackedLParam == TRUE)
         {
            if (SentMessage->Msg.lParam)
               ExFreePool((PVOID)SentMessage->Msg.lParam);
         }

         /* if the message has a sender */
         if (SentMessage->SenderQueue)
         {
            /* dereference our and the sender's message queue */
            IntDereferenceMessageQueue(MessageQueue);
            IntDereferenceMessageQueue(SentMessage->SenderQueue);
         }

         /* free the message */
         ExFreePoolWithTag(SentMessage, TAG_USRMSG);

         CurrentEntry = MessageQueue->SentMessagesListHead.Flink;
      }
      else
      {
         CurrentEntry = CurrentEntry->Flink;
      }
   }
}

NTSTATUS FASTCALL
co_MsqSendMessage(PUSER_MESSAGE_QUEUE MessageQueue,
                  HWND Wnd, UINT Msg, WPARAM wParam, LPARAM lParam,
                  UINT uTimeout, BOOL Block, INT HookMessage,
                  ULONG_PTR *uResult)
{
   PTHREADINFO pti, ptirec;
   PUSER_SENT_MESSAGE Message;
   KEVENT CompletionEvent;
   NTSTATUS WaitStatus;
   PUSER_MESSAGE_QUEUE ThreadQueue;
   LARGE_INTEGER Timeout;
   PLIST_ENTRY Entry;
   LRESULT Result = 0;   //// Result could be trashed. ////

   if(!(Message = ExAllocatePoolWithTag(PagedPool, sizeof(USER_SENT_MESSAGE), TAG_USRMSG)))
   {
      DPRINT1("MsqSendMessage(): Not enough memory to allocate a message");
      return STATUS_INSUFFICIENT_RESOURCES;
   }

   KeInitializeEvent(&CompletionEvent, NotificationEvent, FALSE);

   pti = PsGetCurrentThreadWin32Thread();
   ThreadQueue = pti->MessageQueue;
   ptirec = MessageQueue->Thread->Tcb.Win32Thread;
   ASSERT(ThreadQueue != MessageQueue);
   ASSERT(ptirec->pcti); // Send must have a client side to receive it!!!!

   /* Don't send from or to a dying thread */
    if (pti->TIF_flags & TIF_INCLEANUP || ptirec->TIF_flags & TIF_INCLEANUP)
    {
        *uResult = -1;
        return STATUS_TIMEOUT;
    }

   Timeout.QuadPart = (LONGLONG) uTimeout * (LONGLONG) -10000;

   /* FIXME - increase reference counter of sender's message queue here */

   Message->Msg.hwnd = Wnd;
   Message->Msg.message = Msg;
   Message->Msg.wParam = wParam;
   Message->Msg.lParam = lParam;
   Message->CompletionEvent = &CompletionEvent;
   Message->Result = &Result;
   Message->lResult = 0;
   Message->QS_Flags = 0;
   Message->SenderQueue = ThreadQueue;
   Message->CallBackSenderQueue = NULL;
   IntReferenceMessageQueue(ThreadQueue);
   Message->CompletionCallback = NULL;
   Message->CompletionCallbackContext = 0;
   Message->HookMessage = HookMessage;
   Message->HasPackedLParam = FALSE;

   IntReferenceMessageQueue(MessageQueue);

   /* add it to the list of pending messages */
   InsertTailList(&ThreadQueue->DispatchingMessagesHead, &Message->DispatchingListEntry);

   /* queue it in the destination's message queue */
   InsertTailList(&MessageQueue->SentMessagesListHead, &Message->ListEntry);

   Message->QS_Flags = QS_SENDMESSAGE;
   MsqWakeQueue(MessageQueue, QS_SENDMESSAGE, TRUE);

   /* we can't access the Message anymore since it could have already been deleted! */

   if(Block)
   {
      UserLeaveCo();

      /* don't process messages sent to the thread */
      WaitStatus = KeWaitForSingleObject(&CompletionEvent, UserRequest, UserMode,
                                         FALSE, (uTimeout ? &Timeout : NULL));

      UserEnterCo();

      if(WaitStatus == STATUS_TIMEOUT)
      {
         /* look up if the message has not yet dispatched, if so
            make sure it can't pass a result and it must not set the completion event anymore */
         Entry = MessageQueue->SentMessagesListHead.Flink;
         while (Entry != &MessageQueue->SentMessagesListHead)
         {
            if ((PUSER_SENT_MESSAGE) CONTAINING_RECORD(Entry, USER_SENT_MESSAGE, ListEntry)
                  == Message)
            {
               /* we can access Message here, it's secure because the message queue is locked
                  and the message is still hasn't been dispatched */
               Message->CompletionEvent = NULL;
               Message->Result = NULL;
               break;
            }
            Entry = Entry->Flink;
         }

         /* remove from the local dispatching list so the other thread knows,
            it can't pass a result and it must not set the completion event anymore */
         Entry = ThreadQueue->DispatchingMessagesHead.Flink;
         while (Entry != &ThreadQueue->DispatchingMessagesHead)
         {
            if ((PUSER_SENT_MESSAGE) CONTAINING_RECORD(Entry, USER_SENT_MESSAGE, DispatchingListEntry)
                  == Message)
            {
               /* we can access Message here, it's secure because the sender's message is locked
                  and the message has definitely not yet been destroyed, otherwise it would
                  have been removed from this list by the dispatching routine right after
               dispatching the message */
               Message->CompletionEvent = NULL;
               Message->Result = NULL;
               RemoveEntryList(&Message->DispatchingListEntry);
               Message->DispatchingListEntry.Flink = NULL;
               break;
            }
            Entry = Entry->Flink;
         }

         DPRINT("MsqSendMessage (blocked) timed out 1\n");
      }
      while (co_MsqDispatchOneSentMessage(ThreadQueue))
         ;
   }
   else
   {
      PVOID WaitObjects[2];

      WaitObjects[0] = &CompletionEvent;
      WaitObjects[1] = ThreadQueue->NewMessages;
      do
      {
         UserLeaveCo();

         WaitStatus = KeWaitForMultipleObjects(2, WaitObjects, WaitAny, UserRequest,
                                               UserMode, FALSE, (uTimeout ? &Timeout : NULL), NULL);

         UserEnterCo();

         if(WaitStatus == STATUS_TIMEOUT)
         {
            /* look up if the message has not yet been dispatched, if so
               make sure it can't pass a result and it must not set the completion event anymore */
            Entry = MessageQueue->SentMessagesListHead.Flink;
            while (Entry != &MessageQueue->SentMessagesListHead)
            {
               if ((PUSER_SENT_MESSAGE) CONTAINING_RECORD(Entry, USER_SENT_MESSAGE, ListEntry)
                     == Message)
               {
                  /* we can access Message here, it's secure because the message queue is locked
                     and the message is still hasn't been dispatched */
                  Message->CompletionEvent = NULL;
                  Message->Result = NULL;
                  break;
               }
               Entry = Entry->Flink;
            }

            /* remove from the local dispatching list so the other thread knows,
               it can't pass a result and it must not set the completion event anymore */
            Entry = ThreadQueue->DispatchingMessagesHead.Flink;
            while (Entry != &ThreadQueue->DispatchingMessagesHead)
            {
               if ((PUSER_SENT_MESSAGE) CONTAINING_RECORD(Entry, USER_SENT_MESSAGE, DispatchingListEntry)
                     == Message)
               {
                  /* we can access Message here, it's secure because the sender's message is locked
                     and the message has definitely not yet been destroyed, otherwise it would
                     have been removed from this list by the dispatching routine right after
                  dispatching the message */
                  Message->CompletionEvent = NULL;
                  Message->Result = NULL;
                  RemoveEntryList(&Message->DispatchingListEntry);
                  Message->DispatchingListEntry.Flink = NULL;
                  break;
               }
               Entry = Entry->Flink;
            }

            DPRINT("MsqSendMessage timed out 2\n");
            break;
         }
         while (co_MsqDispatchOneSentMessage(ThreadQueue))
            ;
      }
      while (NT_SUCCESS(WaitStatus) && STATUS_WAIT_0 != WaitStatus);
   }

   if(WaitStatus != STATUS_TIMEOUT)
      *uResult = (STATUS_WAIT_0 == WaitStatus ? Result : -1);

   return WaitStatus;
}

VOID FASTCALL
MsqPostMessage(PUSER_MESSAGE_QUEUE MessageQueue, MSG* Msg, BOOLEAN HardwareMessage,
               DWORD MessageBits)
{
   PUSER_MESSAGE Message;

   if(!(Message = MsqCreateMessage(Msg)))
   {
      return;
   }

   if(!HardwareMessage)
   {
       InsertTailList(&MessageQueue->PostedMessagesListHead,
                      &Message->ListEntry);
   }
   else
   {
       InsertTailList(&MessageQueue->HardwareMessagesListHead,
                      &Message->ListEntry);

       update_input_key_state( MessageQueue, Msg );
   }

   Message->QS_Flags = MessageBits;
   MsqWakeQueue(MessageQueue, MessageBits, (MessageBits & QS_TIMER ? FALSE : TRUE));
}

VOID FASTCALL
MsqPostQuitMessage(PUSER_MESSAGE_QUEUE MessageQueue, ULONG ExitCode)
{
   MessageQueue->QuitPosted = TRUE;
   MessageQueue->QuitExitCode = ExitCode;
   MsqWakeQueue(MessageQueue, QS_POSTMESSAGE|QS_ALLPOSTMESSAGE, TRUE);
}

/***********************************************************************
 *           MsqSendParentNotify
 *
 * Send a WM_PARENTNOTIFY to all ancestors of the given window, unless
 * the window has the WS_EX_NOPARENTNOTIFY style.
 */
static void MsqSendParentNotify( PWND pwnd, WORD event, WORD idChild, POINT pt )
{
    PWND pwndDesktop = UserGetWindowObject(IntGetDesktopWindow());

    /* pt has to be in the client coordinates of the parent window */
    pt.x += pwndDesktop->rcClient.left - pwnd->rcClient.left;
    pt.y += pwndDesktop->rcClient.top - pwnd->rcClient.top;

    for (;;)
    {
        PWND pwndParent;

        if (!(pwnd->style & WS_CHILD)) break;
        if (pwnd->ExStyle & WS_EX_NOPARENTNOTIFY) break;
        if (!(pwndParent = IntGetParent(pwnd))) break;
        if (pwndParent == pwndDesktop) break;
        pt.x += pwnd->rcClient.left - pwndParent->rcClient.left;
        pt.y += pwnd->rcClient.top - pwndParent->rcClient.top;

        pwnd = pwndParent;
        co_IntSendMessage( UserHMGetHandle(pwnd), WM_PARENTNOTIFY,
                      MAKEWPARAM( event, idChild ), MAKELPARAM( pt.x, pt.y ) );
    }
}

BOOL co_IntProcessMouseMessage(MSG* msg, BOOL* RemoveMessages, UINT first, UINT last)
{
    MSG clk_msg;
    POINT pt;
    UINT message;
    USHORT hittest;
    EVENTMSG event;
    MOUSEHOOKSTRUCT hook;
    BOOL eatMsg;

    PWND pwndMsg, pwndDesktop;
    PUSER_MESSAGE_QUEUE MessageQueue;
    PTHREADINFO pti;
    PSYSTEM_CURSORINFO CurInfo;
    DECLARE_RETURN(BOOL);

    pti = PsGetCurrentThreadWin32Thread();
    pwndDesktop = UserGetDesktopWindow();
    MessageQueue = pti->MessageQueue;
    CurInfo = IntGetSysCursorInfo();
    pwndMsg = UserGetWindowObject(msg->hwnd);
    clk_msg = MessageQueue->msgDblClk;

    /* find the window to dispatch this mouse message to */
    if (MessageQueue->CaptureWindow)
    {
        hittest = HTCLIENT;
        pwndMsg = IntGetWindowObject(MessageQueue->CaptureWindow);
    }
    else
    {
        pwndMsg = co_WinPosWindowFromPoint(pwndMsg, &msg->pt, &hittest);
    }

    DPRINT("Got mouse message for 0x%x, hittest: 0x%x\n", msg->hwnd, hittest );

    if (pwndMsg == NULL || pwndMsg->head.pti != pti)
    {
        /* Remove and ignore the message */
        *RemoveMessages = TRUE;
        RETURN(FALSE);
    }

    msg->hwnd = UserHMGetHandle(pwndMsg);

#if 0
    if (!check_hwnd_filter( msg, hwnd_filter )) RETURN(FALSE);
#endif

    pt = msg->pt;
    message = msg->message;
    /* Note: windows has no concept of a non-client wheel message */
    if (message != WM_MOUSEWHEEL)
    {
        if (hittest != HTCLIENT)
        {
            message += WM_NCMOUSEMOVE - WM_MOUSEMOVE;
            msg->wParam = hittest;
        }
        else
        {
            /* coordinates don't get translated while tracking a menu */
            /* FIXME: should differentiate popups and top-level menus */
            if (!(MessageQueue->MenuOwner))
            {
                pt.x += pwndDesktop->rcClient.left - pwndMsg->rcClient.left;
                pt.y += pwndDesktop->rcClient.top - pwndMsg->rcClient.top;
            }
        }
    }
    msg->lParam = MAKELONG( pt.x, pt.y );

    /* translate double clicks */

    if ((msg->message == WM_LBUTTONDOWN) ||
        (msg->message == WM_RBUTTONDOWN) ||
        (msg->message == WM_MBUTTONDOWN) ||
        (msg->message == WM_XBUTTONDOWN))
    {
        BOOL update = *RemoveMessages;

        /* translate double clicks -
         * note that ...MOUSEMOVEs can slip in between
         * ...BUTTONDOWN and ...BUTTONDBLCLK messages */

        if ((MessageQueue->MenuOwner || MessageQueue->MoveSize) ||
            hittest != HTCLIENT ||
            (pwndMsg->pcls->style & CS_DBLCLKS))
        {
           if ((msg->message == clk_msg.message) &&
               (msg->hwnd == clk_msg.hwnd) &&
               (msg->wParam == clk_msg.wParam) &&
               (msg->time - clk_msg.time < gspv.iDblClickTime) &&
               (abs(msg->pt.x - clk_msg.pt.x) < UserGetSystemMetrics(SM_CXDOUBLECLK)/2) &&
               (abs(msg->pt.y - clk_msg.pt.y) < UserGetSystemMetrics(SM_CYDOUBLECLK)/2))
           {
               message += (WM_LBUTTONDBLCLK - WM_LBUTTONDOWN);
               if (update)
               {
                   MessageQueue->msgDblClk.message = 0;  /* clear the double click conditions */
                   update = FALSE;
               }
           }
        }

        if (!((first ==  0 && last == 0) || (message >= first || message <= last)))
        {
            DPRINT("Message out of range!!!\n");
            RETURN(FALSE);
        }

        /* update static double click conditions */
        if (update) MessageQueue->msgDblClk = *msg;
    }
    else
    {
        if (!((first ==  0 && last == 0) || (message >= first || message <= last)))
        {
            DPRINT("Message out of range!!!\n");
            RETURN(FALSE);
        }
    }

    if(gspv.bMouseClickLock)
    {
        BOOL IsClkLck = FALSE;

        if(msg->message == WM_LBUTTONUP)
        {
            IsClkLck = ((msg->time - CurInfo->ClickLockTime) >= gspv.dwMouseClickLockTime);
            if (IsClkLck && (!CurInfo->ClickLockActive))
            {
                CurInfo->ClickLockActive = TRUE;
            }
        }
        else if (msg->message == WM_LBUTTONDOWN)
        {
            if (CurInfo->ClickLockActive)
            {
                IsClkLck = TRUE;
                CurInfo->ClickLockActive = FALSE;
            }

            CurInfo->ClickLockTime = msg->time;
        }

        if(IsClkLck)
        {
            /* Remove and ignore the message */
            *RemoveMessages = TRUE;
            RETURN(FALSE);
        }
    }

    /* message is accepted now (but may still get dropped) */

    event.message = msg->message;
    event.time    = msg->time;
    event.hwnd    = msg->hwnd;
    event.paramL  = msg->pt.x;
    event.paramH  = msg->pt.y;
    co_HOOK_CallHooks( WH_JOURNALRECORD, HC_ACTION, 0, (LPARAM)&event );

    hook.pt           = msg->pt;
    hook.hwnd         = msg->hwnd;
    hook.wHitTestCode = hittest;
    hook.dwExtraInfo  = 0/*extra_info*/;
    if (co_HOOK_CallHooks( WH_MOUSE, *RemoveMessages ? HC_ACTION : HC_NOREMOVE,
                        message, (LPARAM)&hook ))
    {
        hook.pt           = msg->pt;
        hook.hwnd         = msg->hwnd;
        hook.wHitTestCode = hittest;
        hook.dwExtraInfo  = 0/*extra_info*/;
        co_HOOK_CallHooks( WH_CBT, HCBT_CLICKSKIPPED, message, (LPARAM)&hook );

        DPRINT1("WH_MOUSE dorpped mouse message!\n");

        /* Remove and skip message */
        *RemoveMessages = TRUE;
        RETURN(FALSE);
    }

    if ((hittest == HTERROR) || (hittest == HTNOWHERE))
    {
        co_IntSendMessage( msg->hwnd, WM_SETCURSOR, (WPARAM)msg->hwnd,
                      MAKELONG( hittest, msg->message ));

        /* Remove and skip message */
        *RemoveMessages = TRUE;
        RETURN(FALSE);
    }

    if ((*RemoveMessages == FALSE) || MessageQueue->CaptureWindow)
    {
        /* Accept the message */
        msg->message = message;
        RETURN(TRUE);
    }

    eatMsg = FALSE;

    if ((msg->message == WM_LBUTTONDOWN) ||
        (msg->message == WM_RBUTTONDOWN) ||
        (msg->message == WM_MBUTTONDOWN) ||
        (msg->message == WM_XBUTTONDOWN))
    {
        /* Send the WM_PARENTNOTIFY,
         * note that even for double/nonclient clicks
         * notification message is still WM_L/M/RBUTTONDOWN.
         */
        MsqSendParentNotify(pwndMsg, msg->message, 0, msg->pt );

        /* Activate the window if needed */

        if (msg->hwnd != UserGetForegroundWindow())
        {
            PWND pwndTop = pwndMsg;
            while (pwndTop)
            {
                if ((pwndTop->style & (WS_POPUP|WS_CHILD)) != WS_CHILD) break;
                pwndTop = IntGetParent( pwndTop );
            }

            if (pwndTop && pwndTop != pwndDesktop)
            {
                LONG ret = co_IntSendMessage( msg->hwnd,
                                              WM_MOUSEACTIVATE,
                                              (WPARAM)UserHMGetHandle(pwndTop),
                                              MAKELONG( hittest, msg->message));
                switch(ret)
                {
                case MA_NOACTIVATEANDEAT:
                    eatMsg = TRUE;
                    /* fall through */
                case MA_NOACTIVATE:
                    break;
                case MA_ACTIVATEANDEAT:
                    eatMsg = TRUE;
                    /* fall through */
                case MA_ACTIVATE:
                case 0:
                    if(!co_IntMouseActivateWindow(pwndMsg)) eatMsg = TRUE;
                    break;
                default:
                    DPRINT1( "unknown WM_MOUSEACTIVATE code %d\n", ret );
                    break;
                }
            }
        }
    }

    /* send the WM_SETCURSOR message */

    /* Windows sends the normal mouse message as the message parameter
       in the WM_SETCURSOR message even if it's non-client mouse message */
    co_IntSendMessage( msg->hwnd, WM_SETCURSOR, (WPARAM)msg->hwnd, MAKELONG( hittest, msg->message ));

    msg->message = message;
    RETURN(!eatMsg);

CLEANUP:
    if(pwndMsg)
        UserDereferenceObject(pwndMsg);

    END_CLEANUP;
}

BOOL co_IntProcessKeyboardMessage(MSG* Msg, BOOL* RemoveMessages)
{
    EVENTMSG Event;

    if (Msg->message == WM_KEYDOWN || Msg->message == WM_SYSKEYDOWN ||
        Msg->message == WM_KEYUP || Msg->message == WM_SYSKEYUP)
    {
        switch (Msg->wParam)
        {
            case VK_LSHIFT: case VK_RSHIFT:
                Msg->wParam = VK_SHIFT;
                break;
            case VK_LCONTROL: case VK_RCONTROL:
                Msg->wParam = VK_CONTROL;
                break;
            case VK_LMENU: case VK_RMENU:
                Msg->wParam = VK_MENU;
                break;
        }
    }

    Event.message = Msg->message;
    Event.hwnd    = Msg->hwnd;
    Event.time    = Msg->time;
    Event.paramL  = (Msg->wParam & 0xFF) | (HIWORD(Msg->lParam) << 8);
    Event.paramH  = Msg->lParam & 0x7FFF;
    if (HIWORD(Msg->lParam) & 0x0100) Event.paramH |= 0x8000;
    co_HOOK_CallHooks( WH_JOURNALRECORD, HC_ACTION, 0, (LPARAM)&Event);

    if (co_HOOK_CallHooks( WH_KEYBOARD,
                           *RemoveMessages ? HC_ACTION : HC_NOREMOVE,
                           LOWORD(Msg->wParam),
                           Msg->lParam))
    {
        /* skip this message */
        co_HOOK_CallHooks( WH_CBT,
                           HCBT_KEYSKIPPED,
                           LOWORD(Msg->wParam),
                           Msg->lParam );
        DPRINT1("KeyboardMessage WH_CBT Call Hook return!\n");
        return FALSE;
    }
    return TRUE;
}

BOOL co_IntProcessHardwareMessage(MSG* Msg, BOOL* RemoveMessages, UINT first, UINT last)
{
    if ( IS_MOUSE_MESSAGE(Msg->message))
    {
        return co_IntProcessMouseMessage(Msg, RemoveMessages, first, last);
    }
    else if ( IS_KBD_MESSAGE(Msg->message))
    {
        return co_IntProcessKeyboardMessage(Msg, RemoveMessages);
    }

    return TRUE;
}

BOOL APIENTRY
co_MsqPeekMouseMove(IN PUSER_MESSAGE_QUEUE MessageQueue,
                   IN BOOL Remove,
                   IN PWND Window,
                   IN UINT MsgFilterLow,
                   IN UINT MsgFilterHigh,
                   OUT MSG* pMsg)
{
    BOOL AcceptMessage;
    MSG msg;

    if(!(MessageQueue->MouseMoved))
        return FALSE;

    msg = MessageQueue->MouseMoveMsg;

    AcceptMessage = co_IntProcessMouseMessage(&msg, &Remove, MsgFilterLow, MsgFilterHigh);

    if(AcceptMessage)
        *pMsg = msg;

    if(Remove)
    {
        ClearMsgBitsMask(MessageQueue, QS_MOUSEMOVE);
        MessageQueue->MouseMoved = FALSE;
    }

   return AcceptMessage;
}

/* check whether a message filter contains at least one potential hardware message */
static INT FASTCALL
filter_contains_hw_range( UINT first, UINT last )
{
   /* hardware message ranges are (in numerical order):
    *   WM_NCMOUSEFIRST .. WM_NCMOUSELAST
    *   WM_KEYFIRST .. WM_KEYLAST
    *   WM_MOUSEFIRST .. WM_MOUSELAST
    */
    if (!last) --last;
    if (last < WM_NCMOUSEFIRST) return 0;
    if (first > WM_NCMOUSELAST && last < WM_KEYFIRST) return 0;
    if (first > WM_KEYLAST && last < WM_MOUSEFIRST) return 0;
    if (first > WM_MOUSELAST) return 0;
    return 1;
}

BOOL APIENTRY
co_MsqPeekHardwareMessage(IN PUSER_MESSAGE_QUEUE MessageQueue,
                         IN BOOL Remove,
                         IN PWND Window,
                         IN UINT MsgFilterLow,
                         IN UINT MsgFilterHigh,
                         IN UINT QSflags,
                         OUT MSG* pMsg)
{

    BOOL AcceptMessage;
    PUSER_MESSAGE CurrentMessage;
    PLIST_ENTRY ListHead, CurrentEntry = NULL;
    MSG msg;

    if (!filter_contains_hw_range( MsgFilterLow, MsgFilterHigh )) return FALSE;

    ListHead = &MessageQueue->HardwareMessagesListHead;
    CurrentEntry = ListHead->Flink;

    if (IsListEmpty(CurrentEntry)) return FALSE;

    CurrentMessage = CONTAINING_RECORD(CurrentEntry, USER_MESSAGE,
                                          ListEntry);
    do
    {
        if (IsListEmpty(CurrentEntry)) break;
        if (!CurrentMessage) break;
        CurrentEntry = CurrentMessage->ListEntry.Flink;
/*
 MSDN:
 1: any window that belongs to the current thread, and any messages on the current thread's message queue whose hwnd value is NULL.
 2: retrieves only messages on the current thread's message queue whose hwnd value is NULL.
 3: handle to the window whose messages are to be retrieved.
 */
      if ( ( !Window || // 1
            ( Window == HWND_BOTTOM && CurrentMessage->Msg.hwnd == NULL ) || // 2
            ( Window != HWND_BOTTOM && Window->head.h == CurrentMessage->Msg.hwnd ) ) && // 3
            ( ( ( MsgFilterLow == 0 && MsgFilterHigh == 0 ) && CurrentMessage->QS_Flags & QSflags ) ||
              ( MsgFilterLow <= CurrentMessage->Msg.message && MsgFilterHigh >= CurrentMessage->Msg.message ) ) )
        {
           msg = CurrentMessage->Msg;

           AcceptMessage = co_IntProcessHardwareMessage(&msg, &Remove, MsgFilterLow, MsgFilterHigh);

           if (Remove)
           {
               update_input_key_state(MessageQueue, &msg);
               RemoveEntryList(&CurrentMessage->ListEntry);
               ClearMsgBitsMask(MessageQueue, CurrentMessage->QS_Flags);
               MsqDestroyMessage(CurrentMessage);
           }

           if (AcceptMessage)
           {
              *pMsg = msg;
              return TRUE;
           }
        }
        CurrentMessage = CONTAINING_RECORD(CurrentEntry, USER_MESSAGE,
                                          ListEntry);
    }
    while(CurrentEntry != ListHead);

    return FALSE;
}

BOOLEAN APIENTRY
MsqPeekMessage(IN PUSER_MESSAGE_QUEUE MessageQueue,
                  IN BOOLEAN Remove,
                  IN PWND Window,
                  IN UINT MsgFilterLow,
                  IN UINT MsgFilterHigh,
                  IN UINT QSflags,
                  OUT PMSG Message)
{
   PLIST_ENTRY CurrentEntry;
   PUSER_MESSAGE CurrentMessage;
   PLIST_ENTRY ListHead;

   CurrentEntry = MessageQueue->PostedMessagesListHead.Flink;
   ListHead = &MessageQueue->PostedMessagesListHead;

   if (IsListEmpty(CurrentEntry)) return FALSE;

   CurrentMessage = CONTAINING_RECORD(CurrentEntry, USER_MESSAGE,
                                         ListEntry);
   do
   {
      if (IsListEmpty(CurrentEntry)) break;
      if (!CurrentMessage) break;
      CurrentEntry = CurrentEntry->Flink;
/*
 MSDN:
 1: any window that belongs to the current thread, and any messages on the current thread's message queue whose hwnd value is NULL.
 2: retrieves only messages on the current thread's message queue whose hwnd value is NULL.
 3: handle to the window whose messages are to be retrieved.
 */
      if ( ( !Window || // 1
            ( Window == HWND_BOTTOM && CurrentMessage->Msg.hwnd == NULL ) || // 2
            ( Window != HWND_BOTTOM && Window->head.h == CurrentMessage->Msg.hwnd ) ) && // 3
            ( ( ( MsgFilterLow == 0 && MsgFilterHigh == 0 ) && CurrentMessage->QS_Flags & QSflags ) ||
              ( MsgFilterLow <= CurrentMessage->Msg.message && MsgFilterHigh >= CurrentMessage->Msg.message ) ) )
      {
         *Message = CurrentMessage->Msg;

         if (Remove)
         {
             RemoveEntryList(&CurrentMessage->ListEntry);
             ClearMsgBitsMask(MessageQueue, CurrentMessage->QS_Flags);
             MsqDestroyMessage(CurrentMessage);
         }
         return(TRUE);
      }
      CurrentMessage = CONTAINING_RECORD(CurrentEntry, USER_MESSAGE,
                                         ListEntry);
   }
   while (CurrentEntry != ListHead);

   return(FALSE);
}

NTSTATUS FASTCALL
co_MsqWaitForNewMessages(PUSER_MESSAGE_QUEUE MessageQueue, PWND WndFilter,
                         UINT MsgFilterMin, UINT MsgFilterMax)
{
   NTSTATUS ret;
   UserLeaveCo();
   ret = KeWaitForSingleObject( MessageQueue->NewMessages,
                                UserRequest,
                                UserMode,
                                FALSE,
                                NULL );
   UserEnterCo();
   return ret;
}

BOOL FASTCALL
MsqIsHung(PUSER_MESSAGE_QUEUE MessageQueue)
{
   LARGE_INTEGER LargeTickCount;

   KeQueryTickCount(&LargeTickCount);
   return ((LargeTickCount.u.LowPart - MessageQueue->LastMsgRead) > MSQ_HUNG);
}

VOID
CALLBACK
HungAppSysTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
   //DoTheScreenSaver();
   DPRINT("HungAppSysTimerProc\n");
   // Process list of windows that are hung and waiting.
}

BOOLEAN FASTCALL
MsqInitializeMessageQueue(struct _ETHREAD *Thread, PUSER_MESSAGE_QUEUE MessageQueue)
{
   LARGE_INTEGER LargeTickCount;
   NTSTATUS Status;

   MessageQueue->Thread = Thread;
   MessageQueue->CaretInfo = (PTHRDCARETINFO)(MessageQueue + 1);
   InitializeListHead(&MessageQueue->PostedMessagesListHead);
   InitializeListHead(&MessageQueue->SentMessagesListHead);
   InitializeListHead(&MessageQueue->HardwareMessagesListHead);
   InitializeListHead(&MessageQueue->DispatchingMessagesHead);
   InitializeListHead(&MessageQueue->LocalDispatchingMessagesHead);
   KeInitializeMutex(&MessageQueue->HardwareLock, 0);
   MessageQueue->QuitPosted = FALSE;
   MessageQueue->QuitExitCode = 0;
   KeQueryTickCount(&LargeTickCount);
   MessageQueue->LastMsgRead = LargeTickCount.u.LowPart;
   MessageQueue->FocusWindow = NULL;
   MessageQueue->NewMessagesHandle = NULL;

   Status = ZwCreateEvent(&MessageQueue->NewMessagesHandle, EVENT_ALL_ACCESS,
                          NULL, SynchronizationEvent, FALSE);
   if (!NT_SUCCESS(Status))
   {
      return FALSE;
   }

   Status = ObReferenceObjectByHandle(MessageQueue->NewMessagesHandle, 0,
                                      ExEventObjectType, KernelMode,
                                      (PVOID*)&MessageQueue->NewMessages, NULL);
   if (!NT_SUCCESS(Status))
   {
      ZwClose(MessageQueue->NewMessagesHandle);
      MessageQueue->NewMessagesHandle = NULL;
      return FALSE;
   }

   return TRUE;
}

VOID FASTCALL
MsqCleanupMessageQueue(PUSER_MESSAGE_QUEUE MessageQueue)
{
   PLIST_ENTRY CurrentEntry;
   PUSER_MESSAGE CurrentMessage;
   PUSER_SENT_MESSAGE CurrentSentMessage;
   PTHREADINFO pti;

   pti = MessageQueue->Thread->Tcb.Win32Thread;


   /* cleanup posted messages */
   while (!IsListEmpty(&MessageQueue->PostedMessagesListHead))
   {
      CurrentEntry = RemoveHeadList(&MessageQueue->PostedMessagesListHead);
      CurrentMessage = CONTAINING_RECORD(CurrentEntry, USER_MESSAGE,
                                         ListEntry);
      MsqDestroyMessage(CurrentMessage);
   }

   /* remove the messages that have not yet been dispatched */
   while (!IsListEmpty(&MessageQueue->SentMessagesListHead))
   {
      CurrentEntry = RemoveHeadList(&MessageQueue->SentMessagesListHead);
      CurrentSentMessage = CONTAINING_RECORD(CurrentEntry, USER_SENT_MESSAGE,
                                             ListEntry);

      /* if it is a callback and this queue is not the sender queue, dereference queue */
      if ((CurrentSentMessage->CompletionCallback) && (CurrentSentMessage->CallBackSenderQueue != MessageQueue))
      {
         IntDereferenceMessageQueue(CurrentSentMessage->CallBackSenderQueue);
      }

      DPRINT("Notify the sender and remove a message from the queue that had not been dispatched\n");
      /* Only if the message has a sender was the message in the DispatchingList */
      if ((CurrentSentMessage->SenderQueue)
         && (CurrentSentMessage->DispatchingListEntry.Flink != NULL))
      {
         RemoveEntryList(&CurrentSentMessage->DispatchingListEntry);
      }

      /* wake the sender's thread */
      if (CurrentSentMessage->CompletionEvent != NULL)
      {
         KeSetEvent(CurrentSentMessage->CompletionEvent, IO_NO_INCREMENT, FALSE);
      }

      if (CurrentSentMessage->HasPackedLParam == TRUE)
      {
         if (CurrentSentMessage->Msg.lParam)
            ExFreePool((PVOID)CurrentSentMessage->Msg.lParam);
      }

      /* if the message has a sender */
      if (CurrentSentMessage->SenderQueue)
      {
         /* dereference our and the sender's message queue */
         IntDereferenceMessageQueue(MessageQueue);
         IntDereferenceMessageQueue(CurrentSentMessage->SenderQueue);
      }

      /* free the message */
      ExFreePool(CurrentSentMessage);
   }

   /* notify senders of dispatching messages. This needs to be cleaned up if e.g.
      ExitThread() was called in a SendMessage() umode callback */
   while (!IsListEmpty(&MessageQueue->LocalDispatchingMessagesHead))
   {
      CurrentEntry = RemoveHeadList(&MessageQueue->LocalDispatchingMessagesHead);
      CurrentSentMessage = CONTAINING_RECORD(CurrentEntry, USER_SENT_MESSAGE,
                                             ListEntry);

      /* if it is a callback and this queue is not the sender queue, dereference queue */
      if ((CurrentSentMessage->CompletionCallback) && (CurrentSentMessage->CallBackSenderQueue != MessageQueue))
      {
         IntDereferenceMessageQueue(CurrentSentMessage->CallBackSenderQueue);
      }

      /* remove the message from the dispatching list */
      if(CurrentSentMessage->DispatchingListEntry.Flink != NULL)
      {
         RemoveEntryList(&CurrentSentMessage->DispatchingListEntry);
      }

      DPRINT("Notify the sender, the thread has been terminated while dispatching a message!\n");

      /* wake the sender's thread */
      if (CurrentSentMessage->CompletionEvent != NULL)
      {
         KeSetEvent(CurrentSentMessage->CompletionEvent, IO_NO_INCREMENT, FALSE);
      }

      if (CurrentSentMessage->HasPackedLParam == TRUE)
      {
         if (CurrentSentMessage->Msg.lParam)
            ExFreePool((PVOID)CurrentSentMessage->Msg.lParam);
      }

      /* if the message has a sender */
      if (CurrentSentMessage->SenderQueue)
      {
         /* dereference our and the sender's message queue */
         IntDereferenceMessageQueue(MessageQueue);
         IntDereferenceMessageQueue(CurrentSentMessage->SenderQueue);
      }

      /* free the message */
      ExFreePool(CurrentSentMessage);
   }

   /* tell other threads not to bother returning any info to us */
   while (! IsListEmpty(&MessageQueue->DispatchingMessagesHead))
   {
      CurrentEntry = RemoveHeadList(&MessageQueue->DispatchingMessagesHead);
      CurrentSentMessage = CONTAINING_RECORD(CurrentEntry, USER_SENT_MESSAGE,
                                             DispatchingListEntry);
      CurrentSentMessage->CompletionEvent = NULL;
      CurrentSentMessage->Result = NULL;

      /* do NOT dereference our message queue as it might get attempted to be
         locked later */
   }

   // Clear it all out.
   pti->pcti->fsWakeBits = 0;
   pti->pcti->fsChangeBits = 0;

   MessageQueue->nCntsQBits[QSRosKey] = 0;
   MessageQueue->nCntsQBits[QSRosMouseMove] = 0;
   MessageQueue->nCntsQBits[QSRosMouseButton] = 0;
   MessageQueue->nCntsQBits[QSRosPostMessage] = 0;
   MessageQueue->nCntsQBits[QSRosSendMessage] = 0;
   MessageQueue->nCntsQBits[QSRosHotKey] = 0;
}

PUSER_MESSAGE_QUEUE FASTCALL
MsqCreateMessageQueue(struct _ETHREAD *Thread)
{
   PUSER_MESSAGE_QUEUE MessageQueue;

   MessageQueue = (PUSER_MESSAGE_QUEUE)ExAllocatePoolWithTag(NonPagedPool,
                  sizeof(USER_MESSAGE_QUEUE) + sizeof(THRDCARETINFO),
                  USERTAG_Q);

   if (!MessageQueue)
   {
      return NULL;
   }

   RtlZeroMemory(MessageQueue, sizeof(USER_MESSAGE_QUEUE) + sizeof(THRDCARETINFO));
   /* hold at least one reference until it'll be destroyed */
   IntReferenceMessageQueue(MessageQueue);
   /* initialize the queue */
   if (!MsqInitializeMessageQueue(Thread, MessageQueue))
   {
      IntDereferenceMessageQueue(MessageQueue);
      return NULL;
   }

   return MessageQueue;
}

VOID FASTCALL
MsqDestroyMessageQueue(PUSER_MESSAGE_QUEUE MessageQueue)
{
   PDESKTOP desk;

   MessageQueue->QF_flags |= QF_INDESTROY;

   /* remove the message queue from any desktops */
   if ((desk = InterlockedExchangePointer((PVOID*)&MessageQueue->Desktop, 0)))
   {
      (void)InterlockedExchangePointer((PVOID*)&desk->ActiveMessageQueue, 0);
      IntDereferenceMessageQueue(MessageQueue);
   }

   /* clean it up */
   MsqCleanupMessageQueue(MessageQueue);

   if (MessageQueue->NewMessagesHandle != NULL)
      ZwClose(MessageQueue->NewMessagesHandle);
   MessageQueue->NewMessagesHandle = NULL;
   /* decrease the reference counter, if it hits zero, the queue will be freed */
   IntDereferenceMessageQueue(MessageQueue);
}

LPARAM FASTCALL
MsqSetMessageExtraInfo(LPARAM lParam)
{
   LPARAM Ret;
   PTHREADINFO pti;
   PUSER_MESSAGE_QUEUE MessageQueue;

   pti = PsGetCurrentThreadWin32Thread();
   MessageQueue = pti->MessageQueue;
   if(!MessageQueue)
   {
      return 0;
   }

   Ret = MessageQueue->ExtraInfo;
   MessageQueue->ExtraInfo = lParam;

   return Ret;
}

LPARAM FASTCALL
MsqGetMessageExtraInfo(VOID)
{
   PTHREADINFO pti;
   PUSER_MESSAGE_QUEUE MessageQueue;

   pti = PsGetCurrentThreadWin32Thread();
   MessageQueue = pti->MessageQueue;
   if(!MessageQueue)
   {
      return 0;
   }

   return MessageQueue->ExtraInfo;
}

// ReplyMessage is called by the thread receiving the window message.
BOOL FASTCALL
co_MsqReplyMessage( LRESULT lResult )
{
   PUSER_SENT_MESSAGE Message;
   PTHREADINFO pti;

   pti = PsGetCurrentThreadWin32Thread();
   Message = pti->pusmCurrent;

   if (!Message) return FALSE;

   if (Message->QS_Flags & QS_SMRESULT) return FALSE;

   //     SendMessageXxx    || Callback msg and not a notify msg
   if (Message->SenderQueue || Message->CompletionCallback)
   {
      Message->lResult = lResult;
      Message->QS_Flags |= QS_SMRESULT;
   // See co_MsqDispatchOneSentMessage, change bits already accounted for and cleared and this msg is going away..
   }
   return TRUE;
}

HWND FASTCALL
MsqSetStateWindow(PUSER_MESSAGE_QUEUE MessageQueue, ULONG Type, HWND hWnd)
{
   HWND Prev;

   switch(Type)
   {
      case MSQ_STATE_CAPTURE:
         Prev = MessageQueue->CaptureWindow;
         MessageQueue->CaptureWindow = hWnd;
         return Prev;
      case MSQ_STATE_ACTIVE:
         Prev = MessageQueue->ActiveWindow;
         MessageQueue->ActiveWindow = hWnd;
         return Prev;
      case MSQ_STATE_FOCUS:
         Prev = MessageQueue->FocusWindow;
         MessageQueue->FocusWindow = hWnd;
         return Prev;
      case MSQ_STATE_MENUOWNER:
         Prev = MessageQueue->MenuOwner;
         MessageQueue->MenuOwner = hWnd;
         return Prev;
      case MSQ_STATE_MOVESIZE:
         Prev = MessageQueue->MoveSize;
         MessageQueue->MoveSize = hWnd;
         return Prev;
      case MSQ_STATE_CARET:
         ASSERT(MessageQueue->CaretInfo);
         Prev = MessageQueue->CaretInfo->hWnd;
         MessageQueue->CaretInfo->hWnd = hWnd;
         return Prev;
   }

   return NULL;
}

SHORT
APIENTRY
NtUserGetKeyState(INT key)
{
   DWORD Ret;

   UserEnterExclusive();

   Ret = UserGetKeyState(key);

   UserLeave();

   return Ret;
}


DWORD
APIENTRY
NtUserGetKeyboardState(LPBYTE lpKeyState)
{
   DWORD ret = TRUE;
   PTHREADINFO pti;
   PUSER_MESSAGE_QUEUE MessageQueue;

   UserEnterShared();

   pti = PsGetCurrentThreadWin32Thread();
   MessageQueue = pti->MessageQueue;

   _SEH2_TRY
   {
       ProbeForWrite(lpKeyState,sizeof(MessageQueue->KeyState) ,1);
       RtlCopyMemory(lpKeyState,MessageQueue->KeyState,sizeof(MessageQueue->KeyState));
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
       SetLastNtError(_SEH2_GetExceptionCode());
       ret = FALSE;
   }
   _SEH2_END;

   UserLeave();

   return ret;
}

BOOL
APIENTRY
NtUserSetKeyboardState(LPBYTE lpKeyState)
{
   DWORD ret = TRUE;
   PTHREADINFO pti;
   PUSER_MESSAGE_QUEUE MessageQueue;

   UserEnterExclusive();

   pti = PsGetCurrentThreadWin32Thread();
   MessageQueue = pti->MessageQueue;

   _SEH2_TRY
   {
       ProbeForRead(lpKeyState,sizeof(MessageQueue->KeyState) ,1);
       RtlCopyMemory(MessageQueue->KeyState,lpKeyState,sizeof(MessageQueue->KeyState));
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
       SetLastNtError(_SEH2_GetExceptionCode());
       ret = FALSE;
   }
   _SEH2_END;

   UserLeave();

   return ret;
}


/* EOF */
