/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS Win32k subsystem
 * PURPOSE:          Message queues
 * FILE:             subsystems/win32/win32k/ntuser/msgqueue.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
                     Alexandre Julliard
                     Maarten Lankhorst
 */

#include <win32k.h>
DBG_DEFAULT_CHANNEL(UserMsgQ);

/* GLOBALS *******************************************************************/

static PPAGED_LOOKASIDE_LIST pgMessageLookasideList;
PUSER_MESSAGE_QUEUE gpqCursor;

/* FUNCTIONS *****************************************************************/

INIT_FUNCTION
NTSTATUS
NTAPI
MsqInitializeImpl(VOID)
{
    pgMessageLookasideList = ExAllocatePoolWithTag(NonPagedPool, sizeof(PAGED_LOOKASIDE_LIST), TAG_USRMSG);
    if(!pgMessageLookasideList)
        return STATUS_NO_MEMORY;
   ExInitializePagedLookasideList(pgMessageLookasideList,
                                  NULL,
                                  NULL,
                                  0,
                                  sizeof(USER_MESSAGE),
                                  TAG_USRMSG,
                                  256);

   return(STATUS_SUCCESS);
}

PWND FASTCALL
IntTopLevelWindowFromPoint(INT x, INT y)
{
    PWND pWnd, pwndDesktop;

    /* Get the desktop window */
    pwndDesktop = UserGetDesktopWindow();
    if (!pwndDesktop)
        return NULL;

    /* Loop all top level windows */
    for (pWnd = pwndDesktop->spwndChild;
         pWnd != NULL;
         pWnd = pWnd->spwndNext)
    {
        if (pWnd->state2 & WNDS2_INDESTROY || pWnd->state & WNDS_DESTROYED)
        {
            TRACE("The Window is in DESTROY!\n");
            continue;
        }

        if ((pWnd->style & WS_VISIBLE) && IntPtInWindow(pWnd, x, y))
            return pWnd;
    }

    /* Window has not been found */
    return pwndDesktop;
}

PCURICON_OBJECT
FASTCALL
UserSetCursor(
    PCURICON_OBJECT NewCursor,
    BOOL ForceChange)
{
    PCURICON_OBJECT OldCursor;
    HDC hdcScreen;
    PTHREADINFO pti;
    PUSER_MESSAGE_QUEUE MessageQueue;
    PWND pWnd;

    pti = PsGetCurrentThreadWin32Thread();
    MessageQueue = pti->MessageQueue;

    /* Get the screen DC */
    if(!(hdcScreen = IntGetScreenDC()))
    {
        return NULL;
    }

    OldCursor = MessageQueue->CursorObject;

    /* Check if cursors are different */
    if (OldCursor == NewCursor)
        return OldCursor;

    /* Update cursor for this message queue */
    MessageQueue->CursorObject = NewCursor;

    /* If cursor is not visible we have nothing to do */
    if (MessageQueue->iCursorLevel < 0)
        return OldCursor;

    /* Update cursor if this message queue controls it */
    pWnd = IntTopLevelWindowFromPoint(gpsi->ptCursor.x, gpsi->ptCursor.y);
    if (pWnd && pWnd->head.pti->MessageQueue == MessageQueue)
    {
        if (NewCursor)
        {
            /* Call GDI to set the new screen cursor */
#ifdef NEW_CURSORICON
            PCURICON_OBJECT CursorFrame = NewCursor;
            if(NewCursor->CURSORF_flags & CURSORF_ACON)
            {
                FIXME("Should animate the cursor, using only the first frame now.\n");
                CursorFrame = ((PACON)NewCursor)->aspcur[0];
            }
            GreSetPointerShape(hdcScreen,
                               CursorFrame->hbmAlpha ? NULL : NewCursor->hbmMask,
                               CursorFrame->hbmAlpha ? NewCursor->hbmAlpha : NewCursor->hbmColor,
                               CursorFrame->xHotspot,
                               CursorFrame->yHotspot,
                               gpsi->ptCursor.x,
                               gpsi->ptCursor.y,
                               CursorFrame->hbmAlpha ? SPS_ALPHA : 0);
#else
            GreSetPointerShape(hdcScreen,
                               NewCursor->IconInfo.hbmMask,
                               NewCursor->IconInfo.hbmColor,
                               NewCursor->IconInfo.xHotspot,
                               NewCursor->IconInfo.yHotspot,
                               gpsi->ptCursor.x,
                               gpsi->ptCursor.y,
                               0);
#endif
        }
        else /* Note: OldCursor != NewCursor so we have to hide cursor */
        {
            /* Remove the cursor */
            GreMovePointer(hdcScreen, -1, -1);
            TRACE("Removing pointer!\n");
        }
        IntGetSysCursorInfo()->CurrentCursorObject = NewCursor;
    }

    /* Return the old cursor */
    return OldCursor;
}

/* Called from NtUserCallOneParam with Routine ONEPARAM_ROUTINE_SHOWCURSOR
 * User32 macro NtUserShowCursor */
int UserShowCursor(BOOL bShow)
{
    HDC hdcScreen;
    PTHREADINFO pti;
    PUSER_MESSAGE_QUEUE MessageQueue;
    PWND pWnd;

    if (!(hdcScreen = IntGetScreenDC()))
    {
        return -1; /* No mouse */
    }

    pti = PsGetCurrentThreadWin32Thread();
    MessageQueue = pti->MessageQueue;

    /* Update counter */
    MessageQueue->iCursorLevel += bShow ? 1 : -1;
    pti->iCursorLevel += bShow ? 1 : -1;

    /* Check for trivial cases */
    if ((bShow && MessageQueue->iCursorLevel != 0) ||
        (!bShow && MessageQueue->iCursorLevel != -1))
    {
        /* Note: w don't update global info here because it is used only
          internally to check if cursor is visible */
        return MessageQueue->iCursorLevel;
    }

    /* Check if cursor is above window owned by this MessageQueue */
    pWnd = IntTopLevelWindowFromPoint(gpsi->ptCursor.x, gpsi->ptCursor.y);
    if (pWnd && pWnd->head.pti->MessageQueue == MessageQueue)
    {
        if (bShow)
        {
            /* Show the pointer */
            GreMovePointer(hdcScreen, gpsi->ptCursor.x, gpsi->ptCursor.y);
            TRACE("Showing pointer!\n");
        }
        else
        {
            /* Remove the pointer */
            GreMovePointer(hdcScreen, -1, -1);
            TRACE("Removing pointer!\n");
        }

        /* Update global info */
        IntGetSysCursorInfo()->ShowingCursor = MessageQueue->iCursorLevel;
    }

    return MessageQueue->iCursorLevel;
}

DWORD FASTCALL
UserGetKeyState(DWORD dwKey)
{
   DWORD dwRet = 0;
   PTHREADINFO pti;
   PUSER_MESSAGE_QUEUE MessageQueue;

   pti = PsGetCurrentThreadWin32Thread();
   MessageQueue = pti->MessageQueue;

   if (dwKey < 0x100)
   {
       if (IS_KEY_DOWN(MessageQueue->afKeyState, dwKey))
           dwRet |= 0xFF80; // If down, windows returns 0xFF80.
       if (IS_KEY_LOCKED(MessageQueue->afKeyState, dwKey))
           dwRet |= 0x1;
   }
   else
   {
      EngSetLastError(ERROR_INVALID_PARAMETER);
   }
   return dwRet;
}

/* change the input key state for a given key */
static VOID
UpdateKeyState(PUSER_MESSAGE_QUEUE MessageQueue, WORD wVk, BOOL bIsDown)
{
    TRACE("UpdateKeyState wVk: %u, bIsDown: %d\n", wVk, bIsDown);

    if (bIsDown)
    {
        /* If it's first key down event, xor lock bit */
        if (!IS_KEY_DOWN(MessageQueue->afKeyState, wVk))
            SET_KEY_LOCKED(MessageQueue->afKeyState, wVk, !IS_KEY_LOCKED(MessageQueue->afKeyState, wVk));

        SET_KEY_DOWN(MessageQueue->afKeyState, wVk, TRUE);
        MessageQueue->afKeyRecentDown[wVk / 8] |= (1 << (wVk % 8));
    }
    else
        SET_KEY_DOWN(MessageQueue->afKeyState, wVk, FALSE);
}

/* update the input key state for a keyboard message */
static VOID
UpdateKeyStateFromMsg(PUSER_MESSAGE_QUEUE MessageQueue, MSG* msg)
{
    UCHAR key;
    BOOL down = FALSE;

    TRACE("UpdateKeyStateFromMsg message:%u\n", msg->message);

    switch (msg->message)
    {
    case WM_LBUTTONDOWN:
        down = TRUE;
        /* fall through */
    case WM_LBUTTONUP:
        UpdateKeyState(MessageQueue, VK_LBUTTON, down);
        break;
    case WM_MBUTTONDOWN:
        down = TRUE;
        /* fall through */
    case WM_MBUTTONUP:
        UpdateKeyState(MessageQueue, VK_MBUTTON, down);
        break;
    case WM_RBUTTONDOWN:
        down = TRUE;
        /* fall through */
    case WM_RBUTTONUP:
        UpdateKeyState(MessageQueue, VK_RBUTTON, down);
        break;
    case WM_XBUTTONDOWN:
        down = TRUE;
        /* fall through */
    case WM_XBUTTONUP:
        if (msg->wParam == XBUTTON1)
            UpdateKeyState(MessageQueue, VK_XBUTTON1, down);
        else if (msg->wParam == XBUTTON2)
            UpdateKeyState(MessageQueue, VK_XBUTTON2, down);
        break;
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        down = TRUE;
        /* fall through */
    case WM_KEYUP:
    case WM_SYSKEYUP:
        key = (UCHAR)msg->wParam;
        UpdateKeyState(MessageQueue, key, down);
        switch(key)
        {
        case VK_LCONTROL:
        case VK_RCONTROL:
            down = IS_KEY_DOWN(MessageQueue->afKeyState, VK_LCONTROL) || IS_KEY_DOWN(MessageQueue->afKeyState, VK_RCONTROL);
            UpdateKeyState(MessageQueue, VK_CONTROL, down);
            break;
        case VK_LMENU:
        case VK_RMENU:
            down = IS_KEY_DOWN(MessageQueue->afKeyState, VK_LMENU) || IS_KEY_DOWN(MessageQueue->afKeyState, VK_RMENU);
            UpdateKeyState(MessageQueue, VK_MENU, down);
            break;
        case VK_LSHIFT:
        case VK_RSHIFT:
            down = IS_KEY_DOWN(MessageQueue->afKeyState, VK_LSHIFT) || IS_KEY_DOWN(MessageQueue->afKeyState, VK_RSHIFT);
            UpdateKeyState(MessageQueue, VK_SHIFT, down);
            break;
        }
        break;
    }
}

HANDLE FASTCALL
IntMsqSetWakeMask(DWORD WakeMask)
{
   PTHREADINFO Win32Thread;
   HANDLE MessageEventHandle;
   DWORD dwFlags = HIWORD(WakeMask);

   Win32Thread = PsGetCurrentThreadWin32Thread();
   if (Win32Thread == NULL || Win32Thread->MessageQueue == NULL)
      return 0;

// Win32Thread->pEventQueueServer; IntMsqSetWakeMask returns Win32Thread->hEventQueueClient
   MessageEventHandle = Win32Thread->hEventQueueClient;

   if (Win32Thread->pcti)
   {
      if ( (Win32Thread->pcti->fsChangeBits & LOWORD(WakeMask)) ||
           ( (dwFlags & MWMO_INPUTAVAILABLE) && (Win32Thread->pcti->fsWakeBits & LOWORD(WakeMask)) ) )
      {
         ERR("Chg 0x%x Wake 0x%x Mask 0x%x\n",Win32Thread->pcti->fsChangeBits, Win32Thread->pcti->fsWakeBits, WakeMask);
         KeSetEvent(Win32Thread->pEventQueueServer, IO_NO_INCREMENT, FALSE); // Wake it up!
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
MsqWakeQueue(PTHREADINFO pti, DWORD MessageBits, BOOL KeyEvent)
{
   PUSER_MESSAGE_QUEUE Queue;

   Queue = pti->MessageQueue;

   if (Queue->QF_flags & QF_INDESTROY)
   {
      ERR("This Message Queue is in Destroy!\n");
   }
   pti->pcti->fsWakeBits |= MessageBits;
   pti->pcti->fsChangeBits |= MessageBits;

   // Start bit accounting to help clear the main set of bits.
   if (MessageBits & QS_KEY)
   {
      pti->nCntsQBits[QSRosKey]++;
   }
   if (MessageBits & QS_MOUSE)
   {
      if (MessageBits & QS_MOUSEMOVE)   pti->nCntsQBits[QSRosMouseMove]++;
      if (MessageBits & QS_MOUSEBUTTON) pti->nCntsQBits[QSRosMouseButton]++;
   }
   if (MessageBits & QS_POSTMESSAGE) pti->nCntsQBits[QSRosPostMessage]++;
   if (MessageBits & QS_SENDMESSAGE) pti->nCntsQBits[QSRosSendMessage]++;
   if (MessageBits & QS_HOTKEY)      pti->nCntsQBits[QSRosHotKey]++;
   if (MessageBits & QS_EVENT)       pti->nCntsQBits[QSRosEvent]++;

   if (KeyEvent)
      KeSetEvent(pti->pEventQueueServer, IO_NO_INCREMENT, FALSE);
}

VOID FASTCALL
ClearMsgBitsMask(PTHREADINFO pti, UINT MessageBits)
{
   PUSER_MESSAGE_QUEUE Queue;
   UINT ClrMask = 0;

   Queue = pti->MessageQueue;

   if (MessageBits & QS_KEY)
   {
      if (--pti->nCntsQBits[QSRosKey] == 0) ClrMask |= QS_KEY;
   }
   if (MessageBits & QS_MOUSEMOVE) // ReactOS hard coded.
   {  // Account for tracking mouse moves..
      if (pti->nCntsQBits[QSRosMouseMove]) 
      {
         pti->nCntsQBits[QSRosMouseMove] = 0; // Throttle down count. Up to > 3:1 entries are ignored.
      }
      // Handle mouse move bits here.
      if (Queue->MouseMoved)
      {
         ClrMask |= QS_MOUSEMOVE;
         Queue->MouseMoved = FALSE;
      }
   }
   if (MessageBits & QS_MOUSEBUTTON)
   {
      if (--pti->nCntsQBits[QSRosMouseButton] == 0) ClrMask |= QS_MOUSEBUTTON;
   }
   if (MessageBits & QS_POSTMESSAGE)
   {
      if (--pti->nCntsQBits[QSRosPostMessage] == 0) ClrMask |= QS_POSTMESSAGE;
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
      if (--pti->nCntsQBits[QSRosSendMessage] == 0) ClrMask |= QS_SENDMESSAGE;
   }
   if (MessageBits & QS_HOTKEY)
   {
      if (--pti->nCntsQBits[QSRosHotKey] == 0) ClrMask |= QS_HOTKEY;
   }
   if (MessageBits & QS_EVENT)
   {
      if (--pti->nCntsQBits[QSRosEvent] == 0) ClrMask |= QS_EVENT;
   }

   pti->pcti->fsWakeBits &= ~ClrMask;
   pti->pcti->fsChangeBits &= ~ClrMask;
}

VOID FASTCALL
MsqIncPaintCountQueue(PTHREADINFO pti)
{
   pti->cPaintsReady++;
   MsqWakeQueue(pti, QS_PAINT, TRUE);
}

VOID FASTCALL
MsqDecPaintCountQueue(PTHREADINFO pti)
{
   ClearMsgBitsMask(pti, QS_PAINT);
}

/*
    Post Mouse Move.
 */
VOID FASTCALL
MsqPostMouseMove(PTHREADINFO pti, MSG* Msg)
{
    PUSER_MESSAGE Message;
    PLIST_ENTRY ListHead;
    PUSER_MESSAGE_QUEUE MessageQueue = pti->MessageQueue;

    ListHead = &MessageQueue->HardwareMessagesListHead;

    MessageQueue->MouseMoved = TRUE;

    if (!IsListEmpty(ListHead->Flink))
    {  // Look at the end of the list,
       Message = CONTAINING_RECORD(ListHead->Blink, USER_MESSAGE, ListEntry);
       // If the mouse move message is existing,
       if (Message->Msg.message == WM_MOUSEMOVE)
       {
          TRACE("Post Old MM Message in Q\n");
          Message->Msg = *Msg; // Overwrite the message with updated data!
          MsqWakeQueue(pti, QS_MOUSEMOVE, TRUE);
          return;
       }
    }
    TRACE("Post New MM Message to Q\n");
    MsqPostMessage(pti, Msg, TRUE, QS_MOUSEMOVE, 0);
}

VOID FASTCALL
co_MsqInsertMouseMessage(MSG* Msg, DWORD flags, ULONG_PTR dwExtraInfo, BOOL Hook)
{
   LARGE_INTEGER LargeTickCount;
   MSLLHOOKSTRUCT MouseHookData;
//   PDESKTOP pDesk;
   PWND pwnd, pwndDesktop;
   HDC hdcScreen;
   PTHREADINFO pti;
   PUSER_MESSAGE_QUEUE MessageQueue;
   PSYSTEM_CURSORINFO CurInfo;

   KeQueryTickCount(&LargeTickCount);
   Msg->time = MsqCalculateMessageTime(&LargeTickCount);

   MouseHookData.pt.x = LOWORD(Msg->lParam);
   MouseHookData.pt.y = HIWORD(Msg->lParam);
   switch (Msg->message)
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
   if (!pwndDesktop) return;
//   pDesk = pwndDesktop->head.rpdesk;

   /* Check if the mouse is captured */
   Msg->hwnd = IntGetCaptureWindow();
   if (Msg->hwnd != NULL)
   {
       pwnd = UserGetWindowObject(Msg->hwnd);
   }
   else
   {
       pwnd = IntTopLevelWindowFromPoint(Msg->pt.x, Msg->pt.y);
       if (pwnd) Msg->hwnd = pwnd->head.h;
   }

   hdcScreen = IntGetScreenDC();
   CurInfo = IntGetSysCursorInfo();

   /* Check if we found a window */
   if (Msg->hwnd != NULL && pwnd != NULL)
   {
       pti = pwnd->head.pti;
       MessageQueue = pti->MessageQueue;

       if (MessageQueue->QF_flags & QF_INDESTROY)
       {
          ERR("Mouse is over a Window with a Dead Message Queue!\n");
          return;
       }

       MessageQueue->ptiMouse = pti;

       if (Msg->message == WM_MOUSEMOVE)
       {
          /* Check if cursor should be visible */
           if(hdcScreen &&
              MessageQueue->CursorObject &&
              MessageQueue->iCursorLevel >= 0)
           {
               /* Check if shape has changed */
               if(CurInfo->CurrentCursorObject != MessageQueue->CursorObject)
               {
                   /* Call GDI to set the new screen cursor */
#ifdef NEW_CURSORICON
                    GreSetPointerShape(hdcScreen,
                                       MessageQueue->CursorObject->hbmAlpha ?
                                           NULL : MessageQueue->CursorObject->hbmMask,
                                       MessageQueue->CursorObject->hbmAlpha ?
                                           MessageQueue->CursorObject->hbmAlpha : MessageQueue->CursorObject->hbmColor,
                                       MessageQueue->CursorObject->xHotspot,
                                       MessageQueue->CursorObject->yHotspot,
                                       gpsi->ptCursor.x,
                                       gpsi->ptCursor.y,
                                       MessageQueue->CursorObject->hbmAlpha ? SPS_ALPHA : 0);
#else
                    GreSetPointerShape(hdcScreen,
                                      MessageQueue->CursorObject->IconInfo.hbmMask,
                                      MessageQueue->CursorObject->IconInfo.hbmColor,
                                      MessageQueue->CursorObject->IconInfo.xHotspot,
                                      MessageQueue->CursorObject->IconInfo.yHotspot,
                                      gpsi->ptCursor.x,
                                      gpsi->ptCursor.y,
                                      0);
#endif
               } else
                   GreMovePointer(hdcScreen, Msg->pt.x, Msg->pt.y);
           }
           /* Check if w have to hide cursor */
           else if (CurInfo->ShowingCursor >= 0)
               GreMovePointer(hdcScreen, -1, -1);

           /* Update global cursor info */
           CurInfo->ShowingCursor = MessageQueue->iCursorLevel;
           CurInfo->CurrentCursorObject = MessageQueue->CursorObject;
           gpqCursor = MessageQueue;

           /* Mouse move is a special case */
           MsqPostMouseMove(pti, Msg);
       }
       else
       {
           if (!IntGetCaptureWindow())
           {
             // ERR("ptiLastInput is set\n");
             // ptiLastInput = pti; // Once this is set during Reboot or Shutdown, this prevents the exit window having foreground.
             // Find all the Move Mouse calls and fix mouse set active focus issues......
           }
           TRACE("Posting mouse message to hwnd=%p!\n", UserHMGetHandle(pwnd));
           MsqPostMessage(pti, Msg, TRUE, QS_MOUSEBUTTON, 0);
       }
   }
   else if (hdcScreen)
   {
       /* always show cursor on background; FIXME: set default pointer */
       GreMovePointer(hdcScreen, Msg->pt.x, Msg->pt.y);
       CurInfo->ShowingCursor = 0;
   }
}

PUSER_MESSAGE FASTCALL
MsqCreateMessage(LPMSG Msg)
{
   PUSER_MESSAGE Message;

   Message = ExAllocateFromPagedLookasideList(pgMessageLookasideList);
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
   ExFreeToPagedLookasideList(pgMessageLookasideList, Message);
}

BOOLEAN FASTCALL
co_MsqDispatchOneSentMessage(PTHREADINFO pti)
{
   PUSER_SENT_MESSAGE SaveMsg, Message;
   PLIST_ENTRY Entry;
   BOOL Ret;
   LRESULT Result = 0;

   if (IsListEmpty(&pti->SentMessagesListHead))
   {
      return(FALSE);
   }

   /* remove it from the list of pending messages */
   Entry = RemoveHeadList(&pti->SentMessagesListHead);
   Message = CONTAINING_RECORD(Entry, USER_SENT_MESSAGE, ListEntry);

   SaveMsg = pti->pusmCurrent;
   pti->pusmCurrent = Message;

   // Processing a message sent to it from another thread.
   if ( ( Message->ptiSender && pti != Message->ptiSender) ||
        ( Message->ptiCallBackSender && pti != Message->ptiCallBackSender ))
   {  // most likely, but, to be sure.
      pti->pcti->CTI_flags |= CTI_INSENDMESSAGE; // Let the user know...
   }

   /* insert it to the list of messages that are currently dispatched by this
      message queue */
   InsertTailList(&pti->LocalDispatchingMessagesHead,
                  &Message->ListEntry);

   ClearMsgBitsMask(pti, Message->QS_Flags);

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
   else if(Message->HookMessage == MSQ_INJECTMODULE)
   {
       Result = IntLoadHookModule(Message->Msg.message,
                                  (HHOOK)Message->Msg.lParam,
                                  Message->Msg.wParam);
   }
   else if ((Message->CompletionCallback) &&
            (Message->ptiCallBackSender == pti))
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
         InsertTailList(&Message->ptiCallBackSender->SentMessagesListHead, &Message->ListEntry);
         TRACE("Callback Message not processed yet. Requeuing the message\n");
         Ret = FALSE;
         goto Exit;
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
      if (Message->ptiCallBackSender)
      {
         Message->lResult = Result;
         Message->QS_Flags |= QS_SMRESULT;

         /* insert it in the callers message queue */
         InsertTailList(&Message->ptiCallBackSender->SentMessagesListHead, &Message->ListEntry);
         MsqWakeQueue(Message->ptiCallBackSender, QS_SENDMESSAGE, TRUE);
      }
      Ret = TRUE;
      goto Exit;
   }

   /* remove the message from the dispatching list if needed, so lock the sender's message queue */
   if (Message->ptiSender)
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

   if (Message->HasPackedLParam)
   {
      if (Message->Msg.lParam)
         ExFreePool((PVOID)Message->Msg.lParam);
   }

   /* Notify the sender. */
   if (Message->CompletionEvent != NULL)
   {
      KeSetEvent(Message->CompletionEvent, IO_NO_INCREMENT, FALSE);
   }

   /* free the message */
   ExFreePoolWithTag(Message, TAG_USRMSG);
   Ret = TRUE;
Exit:
   /* do not hangup on the user if this is reentering */
   if (!SaveMsg) pti->pcti->CTI_flags &= ~CTI_INSENDMESSAGE;
   pti->pusmCurrent = SaveMsg;

   return Ret;
}

VOID APIENTRY
MsqRemoveWindowMessagesFromQueue(PWND Window)
{
   PTHREADINFO pti;
   PUSER_SENT_MESSAGE SentMessage;
   PUSER_MESSAGE PostedMessage;
   PLIST_ENTRY CurrentEntry, ListHead;

   ASSERT(Window);

   pti = Window->head.pti;

   /* remove the posted messages for this window */
   CurrentEntry = pti->PostedMessagesListHead.Flink;
   ListHead = &pti->PostedMessagesListHead;
   while (CurrentEntry != ListHead)
   {
      PostedMessage = CONTAINING_RECORD(CurrentEntry, USER_MESSAGE,
                                        ListEntry);
      if (PostedMessage->Msg.hwnd == Window->head.h)
      {
         RemoveEntryList(&PostedMessage->ListEntry);
         ClearMsgBitsMask(pti, PostedMessage->QS_Flags);
         MsqDestroyMessage(PostedMessage);
         CurrentEntry = pti->PostedMessagesListHead.Flink;
      }
      else
      {
         CurrentEntry = CurrentEntry->Flink;
      }
   }

   /* remove the sent messages for this window */
   CurrentEntry = pti->SentMessagesListHead.Flink;
   ListHead = &pti->SentMessagesListHead;
   while (CurrentEntry != ListHead)
   {
      SentMessage = CONTAINING_RECORD(CurrentEntry, USER_SENT_MESSAGE,
                                      ListEntry);
      if(SentMessage->Msg.hwnd == Window->head.h)
      {
         TRACE("Notify the sender and remove a message from the queue that had not been dispatched\n");

         RemoveEntryList(&SentMessage->ListEntry);
         ClearMsgBitsMask(pti, SentMessage->QS_Flags);

         /* Only if the message has a sender was the queue referenced */
         if ((SentMessage->ptiSender)
            && (SentMessage->DispatchingListEntry.Flink != NULL))
         {
            RemoveEntryList(&SentMessage->DispatchingListEntry);
         }

         /* wake the sender's thread */
         if (SentMessage->CompletionEvent != NULL)
         {
            KeSetEvent(SentMessage->CompletionEvent, IO_NO_INCREMENT, FALSE);
         }

         if (SentMessage->HasPackedLParam)
         {
            if (SentMessage->Msg.lParam)
               ExFreePool((PVOID)SentMessage->Msg.lParam);
         }

         /* free the message */
         ExFreePoolWithTag(SentMessage, TAG_USRMSG);

         CurrentEntry = pti->SentMessagesListHead.Flink;
      }
      else
      {
         CurrentEntry = CurrentEntry->Flink;
      }
   }
}

BOOL FASTCALL
co_MsqSendMessageAsync(PTHREADINFO ptiReceiver,
                       HWND hwnd,
                       UINT Msg,
                       WPARAM wParam,
                       LPARAM lParam,
                       SENDASYNCPROC CompletionCallback,
                       ULONG_PTR CompletionCallbackContext,
                       BOOL HasPackedLParam,
                       INT HookMessage)
{

    PTHREADINFO ptiSender;
    PUSER_SENT_MESSAGE Message;

    if(!(Message = ExAllocatePoolWithTag(NonPagedPool, sizeof(USER_SENT_MESSAGE), TAG_USRMSG)))
    {
        ERR("MsqSendMessage(): Not enough memory to allocate a message");
        return FALSE;
    }

    ptiSender = PsGetCurrentThreadWin32Thread();

    Message->Msg.hwnd = hwnd;
    Message->Msg.message = Msg;
    Message->Msg.wParam = wParam;
    Message->Msg.lParam = lParam;
    Message->CompletionEvent = NULL;
    Message->Result = 0;
    Message->lResult = 0;
    Message->ptiReceiver = ptiReceiver;
    Message->ptiSender = NULL;
    Message->ptiCallBackSender = ptiSender;
    Message->DispatchingListEntry.Flink = NULL;
    Message->CompletionCallback = CompletionCallback;
    Message->CompletionCallbackContext = CompletionCallbackContext;
    Message->HookMessage = HookMessage;
    Message->HasPackedLParam = HasPackedLParam;
    Message->QS_Flags = QS_SENDMESSAGE;

    InsertTailList(&ptiReceiver->SentMessagesListHead, &Message->ListEntry);
    MsqWakeQueue(ptiReceiver, QS_SENDMESSAGE, TRUE);

    return TRUE;
}

NTSTATUS FASTCALL
co_MsqSendMessage(PTHREADINFO ptirec,
                  HWND Wnd, UINT Msg, WPARAM wParam, LPARAM lParam,
                  UINT uTimeout, BOOL Block, INT HookMessage,
                  ULONG_PTR *uResult)
{
   PTHREADINFO pti;
   PUSER_SENT_MESSAGE Message;
   KEVENT CompletionEvent;
   NTSTATUS WaitStatus;
   LARGE_INTEGER Timeout;
   PLIST_ENTRY Entry;
   PWND pWnd;
   LRESULT Result = 0;   //// Result could be trashed. ////

   pti = PsGetCurrentThreadWin32Thread();
   ASSERT(pti != ptirec);
   ASSERT(ptirec->pcti); // Send must have a client side to receive it!!!!

   /* Don't send from or to a dying thread */
    if (pti->TIF_flags & TIF_INCLEANUP || ptirec->TIF_flags & TIF_INCLEANUP)
    {
        // Unless we are dying and need to tell our parents.
        if (pti->TIF_flags & TIF_INCLEANUP && !(ptirec->TIF_flags & TIF_INCLEANUP))
        {
           // Parent notify is the big one. Fire and forget!
           TRACE("Send message from dying thread %d\n",Msg);
           co_MsqSendMessageAsync(ptirec, Wnd, Msg, wParam, lParam, NULL, 0, FALSE, HookMessage);
        }
        if (uResult) *uResult = -1;
        TRACE("MsqSM: Msg %d Current pti %lu or Rec pti %lu\n", Msg, pti->TIF_flags & TIF_INCLEANUP, ptirec->TIF_flags & TIF_INCLEANUP);
        return STATUS_UNSUCCESSFUL;
    }

   if ( HookMessage == MSQ_NORMAL )
   {
      pWnd = ValidateHwndNoErr(Wnd);

      // These can not cross International Border lines!
      if ( pti->ppi != ptirec->ppi && pWnd )
      {
         switch(Msg)
         {
             // Handle the special case when working with password transfers across bordering processes.
             case EM_GETLINE:
             case EM_SETPASSWORDCHAR:
             case WM_GETTEXT:
                // Look for edit controls setup for passwords.
                if ( gpsi->atomSysClass[ICLS_EDIT] == pWnd->pcls->atomClassName && // Use atomNVClassName.
                     pWnd->style & ES_PASSWORD )
                {
                   if (uResult) *uResult = -1;
                   ERR("Running across the border without a passport!\n");
                   EngSetLastError(ERROR_ACCESS_DENIED);
                   return STATUS_UNSUCCESSFUL;
                }
                break;
             case WM_NOTIFY:
                if (uResult) *uResult = -1;
                ERR("Running across the border without a passport!\n");
                return STATUS_UNSUCCESSFUL;
         }
      }

      // These can not cross State lines!
      if ( Msg == WM_CREATE || Msg == WM_NCCREATE )
      {
         if (uResult) *uResult = -1;
         ERR("Can not tell the other State we have Create!\n");
         return STATUS_UNSUCCESSFUL;
      }
   }

   if(!(Message = ExAllocatePoolWithTag(PagedPool, sizeof(USER_SENT_MESSAGE), TAG_USRMSG)))
   {
      ERR("MsqSendMessage(): Not enough memory to allocate a message");
      return STATUS_INSUFFICIENT_RESOURCES;
   }

   KeInitializeEvent(&CompletionEvent, NotificationEvent, FALSE);

   Timeout.QuadPart = (LONGLONG) uTimeout * (LONGLONG) -10000;

   /* FIXME: Increase reference counter of sender's message queue here */

   Message->Msg.hwnd = Wnd;
   Message->Msg.message = Msg;
   Message->Msg.wParam = wParam;
   Message->Msg.lParam = lParam;
   Message->CompletionEvent = &CompletionEvent;
   Message->Result = &Result;
   Message->lResult = 0;
   Message->QS_Flags = 0;
   Message->ptiReceiver = ptirec;
   Message->ptiSender = pti;
   Message->ptiCallBackSender = NULL;
   Message->CompletionCallback = NULL;
   Message->CompletionCallbackContext = 0;
   Message->HookMessage = HookMessage;
   Message->HasPackedLParam = FALSE;

   /* Add it to the list of pending messages */
   InsertTailList(&pti->DispatchingMessagesHead, &Message->DispatchingListEntry);

   /* Queue it in the destination's message queue */
   InsertTailList(&ptirec->SentMessagesListHead, &Message->ListEntry);

   Message->QS_Flags = QS_SENDMESSAGE;
   MsqWakeQueue(ptirec, QS_SENDMESSAGE, TRUE);

   /* We can't access the Message anymore since it could have already been deleted! */

   if(Block)
   {
      UserLeaveCo();

      /* Don't process messages sent to the thread */
      WaitStatus = KeWaitForSingleObject(&CompletionEvent, UserRequest, UserMode,
                                         FALSE, (uTimeout ? &Timeout : NULL));

      UserEnterCo();

      if(WaitStatus == STATUS_TIMEOUT)
      {
         /* Look up if the message has not yet dispatched, if so
            make sure it can't pass a result and it must not set the completion event anymore */
         Entry = ptirec->SentMessagesListHead.Flink;
         while (Entry != &ptirec->SentMessagesListHead)
         {
            if ((PUSER_SENT_MESSAGE) CONTAINING_RECORD(Entry, USER_SENT_MESSAGE, ListEntry)
                  == Message)
            {
               /* We can access Message here, it's secure because the message queue is locked
                  and the message is still hasn't been dispatched */
               Message->CompletionEvent = NULL;
               Message->Result = NULL;
               break;
            }
            Entry = Entry->Flink;
         }

         /* Remove from the local dispatching list so the other thread knows,
            it can't pass a result and it must not set the completion event anymore */
         Entry = pti->DispatchingMessagesHead.Flink;
         while (Entry != &pti->DispatchingMessagesHead)
         {
            if ((PUSER_SENT_MESSAGE) CONTAINING_RECORD(Entry, USER_SENT_MESSAGE, DispatchingListEntry)
                  == Message)
            {
               /* We can access Message here, it's secure because the sender's message is locked
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

         TRACE("MsqSendMessage (blocked) timed out 1\n");
      }
      while (co_MsqDispatchOneSentMessage(ptirec))
         ;
   }
   else
   {
      PVOID WaitObjects[3];

      WaitObjects[0] = &CompletionEvent;       // Wait 0
      WaitObjects[1] = pti->pEventQueueServer; // Wait 1
      WaitObjects[2] = ptirec->pEThread;       // Wait 2

      do
      {
         UserLeaveCo();

         WaitStatus = KeWaitForMultipleObjects(3, WaitObjects, WaitAny, UserRequest,
                                               UserMode, FALSE, (uTimeout ? &Timeout : NULL), NULL);

         UserEnterCo();

         if(WaitStatus == STATUS_TIMEOUT)
         {
            /* Look up if the message has not yet been dispatched, if so
               make sure it can't pass a result and it must not set the completion event anymore */
            Entry = ptirec->SentMessagesListHead.Flink;
            while (Entry != &ptirec->SentMessagesListHead)
            {
               if ((PUSER_SENT_MESSAGE) CONTAINING_RECORD(Entry, USER_SENT_MESSAGE, ListEntry)
                     == Message)
               {
                  /* We can access Message here, it's secure because the message queue is locked
                     and the message is still hasn't been dispatched */
                  Message->CompletionEvent = NULL;
                  Message->Result = NULL;
                  break;
               }
               Entry = Entry->Flink;
            }

            /* Remove from the local dispatching list so the other thread knows,
               it can't pass a result and it must not set the completion event anymore */
            Entry = pti->DispatchingMessagesHead.Flink;
            while (Entry != &pti->DispatchingMessagesHead)
            {
               if ((PUSER_SENT_MESSAGE) CONTAINING_RECORD(Entry, USER_SENT_MESSAGE, DispatchingListEntry)
                     == Message)
               {
                  /* We can access Message here, it's secure because the sender's message is locked
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

            TRACE("MsqSendMessage timed out 2\n");
            break;
         }
         // Receiving thread passed on and left us hanging with issues still pending.
         if ( WaitStatus == STATUS_WAIT_2 )
         {
            ERR("Receiving Thread woken up dead!\n");
            Entry = pti->DispatchingMessagesHead.Flink;
            while (Entry != &pti->DispatchingMessagesHead)
            {
               if ((PUSER_SENT_MESSAGE) CONTAINING_RECORD(Entry, USER_SENT_MESSAGE, DispatchingListEntry)
                     == Message)
               {
                  Message->CompletionEvent = NULL;
                  Message->Result = NULL;
                  RemoveEntryList(&Message->DispatchingListEntry);
                  Message->DispatchingListEntry.Flink = NULL;
                  break;
               }
               Entry = Entry->Flink;
            }
         }
         while (co_MsqDispatchOneSentMessage(pti))
            ;
      }
      while (NT_SUCCESS(WaitStatus) && WaitStatus == STATUS_WAIT_1);
   }

   if(WaitStatus != STATUS_TIMEOUT)
      if (uResult) *uResult = (STATUS_WAIT_0 == WaitStatus ? Result : -1);

   return WaitStatus;
}

VOID FASTCALL
MsqPostMessage(PTHREADINFO pti,
               MSG* Msg,
               BOOLEAN HardwareMessage,
               DWORD MessageBits,
               DWORD dwQEvent)
{
   PUSER_MESSAGE Message;
   PUSER_MESSAGE_QUEUE MessageQueue;

   if ( pti->TIF_flags & TIF_INCLEANUP || pti->MessageQueue->QF_flags & QF_INDESTROY )
   {
      ERR("Post Msg; Thread or Q is Dead!\n");
      return;
   }

   if(!(Message = MsqCreateMessage(Msg)))
   {
      return;
   }

   MessageQueue = pti->MessageQueue;

   if (dwQEvent)
   {
       ERR("Post Msg; System Qeued Event Message!\n");
       InsertHeadList(&pti->PostedMessagesListHead,
                      &Message->ListEntry);
   }
   else if (!HardwareMessage)
   {
       InsertTailList(&pti->PostedMessagesListHead,
                      &Message->ListEntry);
   }
   else
   {
       InsertTailList(&MessageQueue->HardwareMessagesListHead,
                      &Message->ListEntry);
   }

   if (Msg->message == WM_HOTKEY) MessageBits |= QS_HOTKEY; // Justin Case, just set it.
   Message->dwQEvent = dwQEvent;
   Message->QS_Flags = MessageBits;
   Message->pti = pti;
   MsqWakeQueue(pti, MessageBits, TRUE);
}

VOID FASTCALL
MsqPostQuitMessage(PTHREADINFO pti, ULONG ExitCode)
{
   pti->QuitPosted = TRUE;
   pti->exitCode = ExitCode;
   MsqWakeQueue(pti, QS_POSTMESSAGE|QS_ALLPOSTMESSAGE, TRUE);
}

/***********************************************************************
 *           MsqSendParentNotify
 *
 * Send a WM_PARENTNOTIFY to all ancestors of the given window, unless
 * the window has the WS_EX_NOPARENTNOTIFY style.
 */
static void MsqSendParentNotify( PWND pwnd, WORD event, WORD idChild, POINT pt )
{
    PWND pwndDesktop = UserGetDesktopWindow();

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

VOID
FASTCALL
IntTrackMouseMove(PWND pwndTrack, PDESKTOP pDesk, PMSG msg, USHORT hittest)
{
//   PWND pwndTrack = IntChildrenWindowFromPoint(pwndMsg, msg->pt.x, msg->pt.y);
//   hittest = (USHORT)GetNCHitEx(pwndTrack, msg->pt); /// @todo WTF is this???

   if ( pDesk->spwndTrack != pwndTrack || // Change with tracking window or
        msg->message != WM_MOUSEMOVE   || // Mouse click changes or
        pDesk->htEx != hittest)           // Change in current hit test states.
   {
      TRACE("ITMM: Track Mouse Move!\n");

      /* Handle only the changing window track and mouse move across a border. */
      if ( pDesk->spwndTrack != pwndTrack ||
          (pDesk->htEx == HTCLIENT) ^ (hittest == HTCLIENT) )
      {
         TRACE("ITMM: Another Wnd %d or Across Border %d\n",
              pDesk->spwndTrack != pwndTrack,(pDesk->htEx == HTCLIENT) ^ (hittest == HTCLIENT));

         if ( pDesk->dwDTFlags & DF_TME_LEAVE )
            UserPostMessage( UserHMGetHandle(pDesk->spwndTrack),
                            (pDesk->htEx != HTCLIENT) ? WM_NCMOUSELEAVE : WM_MOUSELEAVE,
                             0, 0);

         if ( pDesk->dwDTFlags & DF_TME_HOVER )
            IntKillTimer(pDesk->spwndTrack, ID_EVENT_SYSTIMER_MOUSEHOVER, TRUE);

         /* Clear the flags to sign a change. */
         pDesk->dwDTFlags &= ~(DF_TME_LEAVE|DF_TME_HOVER);
      }
      /* Set the Track window and hit test. */
      pDesk->spwndTrack = pwndTrack;
      pDesk->htEx = hittest;
   }

   /* Reset, Same Track window, Hover set and Mouse Clicks or Clobbered Hover box. */
   if ( pDesk->spwndTrack == pwndTrack &&
       ( msg->message != WM_MOUSEMOVE || !RECTL_bPointInRect(&pDesk->rcMouseHover, msg->pt.x, msg->pt.y)) &&
        pDesk->dwDTFlags & DF_TME_HOVER )
   {
      TRACE("ITMM: Reset Hover points!\n");
      // Restart timer for the hover period.
      IntSetTimer(pDesk->spwndTrack, ID_EVENT_SYSTIMER_MOUSEHOVER, pDesk->dwMouseHoverTime, SystemTimerProc, TMRF_SYSTEM);
      // Reset desktop mouse hover from the system default hover rectangle.
      RECTL_vSetRect(&pDesk->rcMouseHover,
                      msg->pt.x - gspv.iMouseHoverWidth  / 2,
                      msg->pt.y - gspv.iMouseHoverHeight / 2,
                      msg->pt.x + gspv.iMouseHoverWidth  / 2,
                      msg->pt.y + gspv.iMouseHoverHeight / 2);
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
    BOOL eatMsg = FALSE;

    PWND pwndMsg, pwndDesktop;
    PUSER_MESSAGE_QUEUE MessageQueue;
    PTHREADINFO pti;
    PSYSTEM_CURSORINFO CurInfo;
    PDESKTOP pDesk;
    DECLARE_RETURN(BOOL);

    pti = PsGetCurrentThreadWin32Thread();
    pwndDesktop = UserGetDesktopWindow();
    MessageQueue = pti->MessageQueue;
    CurInfo = IntGetSysCursorInfo();
    pwndMsg = ValidateHwndNoErr(msg->hwnd);
    clk_msg = MessageQueue->msgDblClk;
    pDesk = pwndDesktop->head.rpdesk;

    /* find the window to dispatch this mouse message to */
    if (MessageQueue->spwndCapture)
    {
        hittest = HTCLIENT;
        pwndMsg = MessageQueue->spwndCapture;
        if (pwndMsg) UserReferenceObject(pwndMsg);
    }
    else
    {
        pwndMsg = co_WinPosWindowFromPoint(NULL, &msg->pt, &hittest, FALSE);//TRUE);
    }

    TRACE("Got mouse message for %p, hittest: 0x%x\n", msg->hwnd, hittest);

    if (pwndMsg == NULL || pwndMsg->head.pti != pti)
    {
        /* Remove and ignore the message */
        *RemoveMessages = TRUE;
        RETURN(FALSE);
    }

    if ( MessageQueue == gpqCursor ) // Cursor must use the same Queue!
    {
       IntTrackMouseMove(pwndMsg, pDesk, msg, hittest);
    }
    else
    {
       ERR("Not the same cursor!\n");
    }

    msg->hwnd = UserHMGetHandle(pwndMsg);

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
               ((msg->time - clk_msg.time) < (ULONG)gspv.iDblClickTime) &&
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
            TRACE("Message out of range!!!\n");
            RETURN(FALSE);
        }

        /* update static double click conditions */
        if (update) MessageQueue->msgDblClk = *msg;
    }
    else
    {
        if (!((first ==  0 && last == 0) || (message >= first || message <= last)))
        {
            TRACE("Message out of range!!!\n");
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
    hook.dwExtraInfo  = 0 /* extra_info */ ;
    if (co_HOOK_CallHooks( WH_MOUSE, *RemoveMessages ? HC_ACTION : HC_NOREMOVE,
                        message, (LPARAM)&hook ))
    {
        hook.pt           = msg->pt;
        hook.hwnd         = msg->hwnd;
        hook.wHitTestCode = hittest;
        hook.dwExtraInfo  = 0 /* extra_info */ ;
        co_HOOK_CallHooks( WH_CBT, HCBT_CLICKSKIPPED, message, (LPARAM)&hook );

        ERR("WH_MOUSE dropped mouse message!\n");

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

    if ((*RemoveMessages == FALSE) || MessageQueue->spwndCapture)
    {
        /* Accept the message */
        msg->message = message;
        RETURN(TRUE);
    }

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

        if (pwndMsg != MessageQueue->spwndActive)
        {
            PWND pwndTop = pwndMsg;
            pwndTop = IntGetNonChildAncestor(pwndTop);

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
                    if (!co_IntMouseActivateWindow( pwndTop )) eatMsg = TRUE;
                    break;
                default:
                    ERR( "unknown WM_MOUSEACTIVATE code %d\n", ret );
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
        ERR("KeyboardMessage WH_CBT Call Hook return!\n");
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
co_MsqPeekHardwareMessage(IN PTHREADINFO pti,
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
    BOOL Ret = FALSE;
    PUSER_MESSAGE_QUEUE MessageQueue = pti->MessageQueue;

    if (!filter_contains_hw_range( MsgFilterLow, MsgFilterHigh )) return FALSE;

    ListHead = &MessageQueue->HardwareMessagesListHead;
    CurrentEntry = ListHead->Flink;

    if (IsListEmpty(CurrentEntry)) return FALSE;

    if (!MessageQueue->ptiSysLock)
    {
       MessageQueue->ptiSysLock = pti;
       pti->pcti->CTI_flags |= CTI_THREADSYSLOCK;
    }

    if (MessageQueue->ptiSysLock != pti)
    {
       ERR("MsqPeekHardwareMessage: Thread Q is locked to another pti!\n");
       return FALSE;
    }

    CurrentMessage = CONTAINING_RECORD(CurrentEntry, USER_MESSAGE,
                                          ListEntry);
    do
    {
        if (IsListEmpty(CurrentEntry)) break;
        if (!CurrentMessage) break;
        CurrentEntry = CurrentMessage->ListEntry.Flink;
        if (!CurrentEntry) break; //// Fix CORE-6734 reported crash.
/*
 MSDN:
 1: any window that belongs to the current thread, and any messages on the current thread's message queue whose hwnd value is NULL.
 2: retrieves only messages on the current thread's message queue whose hwnd value is NULL.
 3: handle to the window whose messages are to be retrieved.
 */
      if ( ( !Window || // 1
            ( Window == PWND_BOTTOM && CurrentMessage->Msg.hwnd == NULL ) || // 2
            ( Window != PWND_BOTTOM && Window->head.h == CurrentMessage->Msg.hwnd ) ) && // 3
            ( ( ( MsgFilterLow == 0 && MsgFilterHigh == 0 ) && CurrentMessage->QS_Flags & QSflags ) ||
              ( MsgFilterLow <= CurrentMessage->Msg.message && MsgFilterHigh >= CurrentMessage->Msg.message ) ) )
        {
           msg = CurrentMessage->Msg;

           UpdateKeyStateFromMsg(MessageQueue, &msg);
           AcceptMessage = co_IntProcessHardwareMessage(&msg, &Remove, MsgFilterLow, MsgFilterHigh);

           if (Remove)
           {
               RemoveEntryList(&CurrentMessage->ListEntry);
               ClearMsgBitsMask(pti, CurrentMessage->QS_Flags);
               MsqDestroyMessage(CurrentMessage);
           }

           if (AcceptMessage)
           {
              *pMsg = msg;
              Ret = TRUE;
              break;
           }
        }
        CurrentMessage = CONTAINING_RECORD(CurrentEntry, USER_MESSAGE, ListEntry);
    }
    while(CurrentEntry != ListHead);

    MessageQueue->ptiSysLock = NULL;
    pti->pcti->CTI_flags &= ~CTI_THREADSYSLOCK;
    return Ret;
}

BOOLEAN APIENTRY
MsqPeekMessage(IN PTHREADINFO pti,
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
   BOOL Ret = FALSE;

   CurrentEntry = pti->PostedMessagesListHead.Flink;
   ListHead = &pti->PostedMessagesListHead;

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
            ( Window == PWND_BOTTOM && CurrentMessage->Msg.hwnd == NULL ) || // 2
            ( Window != PWND_BOTTOM && Window->head.h == CurrentMessage->Msg.hwnd ) ) && // 3
            ( ( ( MsgFilterLow == 0 && MsgFilterHigh == 0 ) && CurrentMessage->QS_Flags & QSflags ) ||
              ( MsgFilterLow <= CurrentMessage->Msg.message && MsgFilterHigh >= CurrentMessage->Msg.message ) ) )
      {
         *Message = CurrentMessage->Msg;

         if (Remove)
         {
             RemoveEntryList(&CurrentMessage->ListEntry);
             ClearMsgBitsMask(pti, CurrentMessage->QS_Flags);
             MsqDestroyMessage(CurrentMessage);
         }
         Ret = TRUE;
         break;
      }
      CurrentMessage = CONTAINING_RECORD(CurrentEntry, USER_MESSAGE,
                                         ListEntry);
   }
   while (CurrentEntry != ListHead);

   return Ret;
}

NTSTATUS FASTCALL
co_MsqWaitForNewMessages(PTHREADINFO pti, PWND WndFilter,
                         UINT MsgFilterMin, UINT MsgFilterMax)
{
   NTSTATUS ret;
   UserLeaveCo();
   ret = KeWaitForSingleObject( pti->pEventQueueServer,
                                UserRequest,
                                UserMode,
                                FALSE,
                                NULL );
   UserEnterCo();
   return ret;
}

BOOL FASTCALL
MsqIsHung(PTHREADINFO pti)
{
   LARGE_INTEGER LargeTickCount;

   KeQueryTickCount(&LargeTickCount);
   return ((LargeTickCount.u.LowPart - pti->timeLast) > MSQ_HUNG);
}

VOID
CALLBACK
HungAppSysTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
   DoTheScreenSaver();
   TRACE("HungAppSysTimerProc\n");
   // Process list of windows that are hung and waiting.
}

BOOLEAN FASTCALL
MsqInitializeMessageQueue(PTHREADINFO pti, PUSER_MESSAGE_QUEUE MessageQueue)
{
   MessageQueue->CaretInfo = (PTHRDCARETINFO)(MessageQueue + 1);
   InitializeListHead(&MessageQueue->HardwareMessagesListHead); // Keep here!
   MessageQueue->spwndFocus = NULL;
   MessageQueue->iCursorLevel = 0;
   MessageQueue->CursorObject = NULL;
   RtlCopyMemory(MessageQueue->afKeyState, gafAsyncKeyState, sizeof(gafAsyncKeyState));
   MessageQueue->ptiMouse = pti;
   MessageQueue->ptiKeyboard = pti;
   MessageQueue->cThreads++;

   return TRUE;
}

VOID FASTCALL
MsqCleanupThreadMsgs(PTHREADINFO pti)
{
   PLIST_ENTRY CurrentEntry;
   PUSER_MESSAGE CurrentMessage;
   PUSER_SENT_MESSAGE CurrentSentMessage;

   /* cleanup posted messages */
   while (!IsListEmpty(&pti->PostedMessagesListHead))
   {
      CurrentEntry = RemoveHeadList(&pti->PostedMessagesListHead);
      CurrentMessage = CONTAINING_RECORD(CurrentEntry, USER_MESSAGE,
                                         ListEntry);
      MsqDestroyMessage(CurrentMessage);
   }

   /* remove the messages that have not yet been dispatched */
   while (!IsListEmpty(&pti->SentMessagesListHead))
   {
      CurrentEntry = RemoveHeadList(&pti->SentMessagesListHead);
      CurrentSentMessage = CONTAINING_RECORD(CurrentEntry, USER_SENT_MESSAGE,
                                             ListEntry);

      TRACE("Notify the sender and remove a message from the queue that had not been dispatched\n");
      /* Only if the message has a sender was the message in the DispatchingList */
      if ((CurrentSentMessage->ptiSender)
         && (CurrentSentMessage->DispatchingListEntry.Flink != NULL))
      {
         RemoveEntryList(&CurrentSentMessage->DispatchingListEntry);
      }

      /* wake the sender's thread */
      if (CurrentSentMessage->CompletionEvent != NULL)
      {
         KeSetEvent(CurrentSentMessage->CompletionEvent, IO_NO_INCREMENT, FALSE);
      }

      if (CurrentSentMessage->HasPackedLParam)
      {
         if (CurrentSentMessage->Msg.lParam)
            ExFreePool((PVOID)CurrentSentMessage->Msg.lParam);
      }

      /* free the message */
      ExFreePool(CurrentSentMessage);
   }

   /* notify senders of dispatching messages. This needs to be cleaned up if e.g.
      ExitThread() was called in a SendMessage() umode callback */
   while (!IsListEmpty(&pti->LocalDispatchingMessagesHead))
   {
      CurrentEntry = RemoveHeadList(&pti->LocalDispatchingMessagesHead);
      CurrentSentMessage = CONTAINING_RECORD(CurrentEntry, USER_SENT_MESSAGE,
                                             ListEntry);

      /* remove the message from the dispatching list */
      if(CurrentSentMessage->DispatchingListEntry.Flink != NULL)
      {
         RemoveEntryList(&CurrentSentMessage->DispatchingListEntry);
      }

      TRACE("Notify the sender, the thread has been terminated while dispatching a message!\n");

      /* wake the sender's thread */
      if (CurrentSentMessage->CompletionEvent != NULL)
      {
         KeSetEvent(CurrentSentMessage->CompletionEvent, IO_NO_INCREMENT, FALSE);
      }

      if (CurrentSentMessage->HasPackedLParam)
      {
         if (CurrentSentMessage->Msg.lParam)
            ExFreePool((PVOID)CurrentSentMessage->Msg.lParam);
      }

      /* free the message */
      ExFreePool(CurrentSentMessage);
   }

   /* tell other threads not to bother returning any info to us */
   while (! IsListEmpty(&pti->DispatchingMessagesHead))
   {
      CurrentEntry = RemoveHeadList(&pti->DispatchingMessagesHead);
      CurrentSentMessage = CONTAINING_RECORD(CurrentEntry, USER_SENT_MESSAGE,
                                             DispatchingListEntry);
      CurrentSentMessage->CompletionEvent = NULL;
      CurrentSentMessage->Result = NULL;

      /* do NOT dereference our message queue as it might get attempted to be
         locked later */
   }

   // Clear it all out.
   if (pti->pcti)
   {
       pti->pcti->fsWakeBits = 0;
       pti->pcti->fsChangeBits = 0;
   }

   pti->nCntsQBits[QSRosKey] = 0;
   pti->nCntsQBits[QSRosMouseMove] = 0;
   pti->nCntsQBits[QSRosMouseButton] = 0;
   pti->nCntsQBits[QSRosPostMessage] = 0;
   pti->nCntsQBits[QSRosSendMessage] = 0;
   pti->nCntsQBits[QSRosHotKey] = 0;
   pti->nCntsQBits[QSRosEvent] = 0;
}

VOID FASTCALL
MsqCleanupMessageQueue(PTHREADINFO pti)
{
   PUSER_MESSAGE_QUEUE MessageQueue;

   MessageQueue = pti->MessageQueue;
   MessageQueue->cThreads--;

   if (MessageQueue->cThreads)
   {
      if (MessageQueue->ptiSysLock == pti) MessageQueue->ptiSysLock = NULL;
   }

   if (MessageQueue->CursorObject)
   {
       PCURICON_OBJECT pCursor = MessageQueue->CursorObject;

       /* Change to another cursor if we going to dereference current one
          Note: we can't use UserSetCursor because it uses current thread
                message queue instead of queue given for cleanup */
       if (IntGetSysCursorInfo()->CurrentCursorObject == pCursor)
       {
           HDC hdcScreen;

           /* Get the screen DC */
           hdcScreen = IntGetScreenDC();
           if (hdcScreen)
               GreMovePointer(hdcScreen, -1, -1);
           IntGetSysCursorInfo()->CurrentCursorObject = NULL;
       }

       TRACE("DereferenceObject pCursor\n");
       UserDereferenceObject(pCursor);
   }

   if (gpqForeground == MessageQueue)
   {
      IntSetFocusMessageQueue(NULL);
   }
   if (gpqForegroundPrev == MessageQueue)
   {
      gpqForegroundPrev = NULL;
   }
   if (gpqCursor == MessageQueue)
   {
      gpqCursor = NULL;
   }
}

PUSER_MESSAGE_QUEUE FASTCALL
MsqCreateMessageQueue(PTHREADINFO pti)
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
   if (!MsqInitializeMessageQueue(pti, MessageQueue))
   {
      IntDereferenceMessageQueue(MessageQueue);
      return NULL;
   }

   return MessageQueue;
}

VOID FASTCALL
MsqDestroyMessageQueue(PTHREADINFO pti)
{
   PDESKTOP desk;
   PUSER_MESSAGE_QUEUE MessageQueue = pti->MessageQueue;

   MessageQueue->QF_flags |= QF_INDESTROY;

   /* remove the message queue from any desktops */
   if ((desk = InterlockedExchangePointer((PVOID*)&MessageQueue->Desktop, 0)))
   {
      (void)InterlockedExchangePointer((PVOID*)&desk->ActiveMessageQueue, 0);
      IntDereferenceMessageQueue(MessageQueue);
   }

   /* clean it up */
   MsqCleanupMessageQueue(pti);

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
   if (Message->ptiSender || Message->CompletionCallback)
   {
      Message->lResult = lResult;
      Message->QS_Flags |= QS_SMRESULT;
   // See co_MsqDispatchOneSentMessage, change bits already accounted for and cleared and this msg is going away..
   }
   return TRUE;
}

HWND FASTCALL
MsqSetStateWindow(PTHREADINFO pti, ULONG Type, HWND hWnd)
{
   HWND Prev;
   PUSER_MESSAGE_QUEUE MessageQueue;

   MessageQueue = pti->MessageQueue;

   switch(Type)
   {
      case MSQ_STATE_CAPTURE:
         Prev = MessageQueue->spwndCapture ? UserHMGetHandle(MessageQueue->spwndCapture) : 0;
         MessageQueue->spwndCapture = ValidateHwndNoErr(hWnd);
         return Prev;
      case MSQ_STATE_ACTIVE:
         Prev = MessageQueue->spwndActive ? UserHMGetHandle(MessageQueue->spwndActive) : 0;
         MessageQueue->spwndActive = ValidateHwndNoErr(hWnd);
         return Prev;
      case MSQ_STATE_FOCUS:
         Prev = MessageQueue->spwndFocus ? UserHMGetHandle(MessageQueue->spwndFocus) : 0;
         MessageQueue->spwndFocus = ValidateHwndNoErr(hWnd);
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

   UserEnterShared();

   Ret = UserGetKeyState(key);

   UserLeave();

   return (SHORT)Ret;
}


DWORD
APIENTRY
NtUserGetKeyboardState(LPBYTE lpKeyState)
{
   DWORD i, ret = TRUE;
   PTHREADINFO pti;
   PUSER_MESSAGE_QUEUE MessageQueue;

   UserEnterShared();

   pti = PsGetCurrentThreadWin32Thread();
   MessageQueue = pti->MessageQueue;

   _SEH2_TRY
   {
       /* Probe and copy key state to an array */
       ProbeForWrite(lpKeyState, 256 * sizeof(BYTE), 1);
       for (i = 0; i < 256; ++i)
       {
           lpKeyState[i] = 0;
           if (IS_KEY_DOWN(MessageQueue->afKeyState, i))
               lpKeyState[i] |= KS_DOWN_BIT;
           if (IS_KEY_LOCKED(MessageQueue->afKeyState, i))
               lpKeyState[i] |= KS_LOCK_BIT;
       }
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
NtUserSetKeyboardState(LPBYTE pKeyState)
{
   UINT i;
   BOOL bRet = TRUE;
   PTHREADINFO pti;
   PUSER_MESSAGE_QUEUE MessageQueue;

   UserEnterExclusive();

   pti = PsGetCurrentThreadWin32Thread();
   MessageQueue = pti->MessageQueue;

   _SEH2_TRY
   {
       ProbeForRead(pKeyState, 256 * sizeof(BYTE), 1);
       for (i = 0; i < 256; ++i)
       {
            SET_KEY_DOWN(MessageQueue->afKeyState, i, pKeyState[i] & KS_DOWN_BIT);
            SET_KEY_LOCKED(MessageQueue->afKeyState, i, pKeyState[i] & KS_LOCK_BIT);
       }
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
       SetLastNtError(_SEH2_GetExceptionCode());
       bRet = FALSE;
   }
   _SEH2_END;

   UserLeave();

   return bRet;
}

/* EOF */
