#include <windows.h>
#include <user32/msg.h>
#include <user32/hook.h>
#include <user32/queue.h>
#include <user32/spy.h>
#include <user32/msg.h>
#include <user32/debug.h>

extern int doubleClickSpeed;

HQUEUE hFirstQueue;
HQUEUE hNewQueue;

/**********************************************************************
 *           SetDoubleClickTime   (USER.480)
 */
WINBOOL STDCALL SetDoubleClickTime( UINT interval )
{
    doubleClickSpeed = interval ? interval : 500;
    return TRUE;
}		


/**********************************************************************
 *           GetDoubleClickTime   (USER.239)
 */
UINT STDCALL GetDoubleClickTime(void)
{
    return doubleClickSpeed;
}		

/***********************************************************************
 *         PeekMessageW             Check queue for messages
 *
 * Checks for a message in the thread's queue, filtered as for
 * GetMessage(). Returns immediately whether a message is available
 * or not.
 *
 * Whether a retrieved message is removed from the queue is set by the
 * _wRemoveMsg_ flags, which should be one of the following values:
 *
 *    PM_NOREMOVE    Do not remove the message from the queue. 
 *
 *    PM_REMOVE      Remove the message from the queue.
 *
 * In addition, PM_NOYIELD may be combined into _wRemoveMsg_ to
 * request that the system not yield control during PeekMessage();
 * however applications may not rely on scheduling behavior.
 * 
 * RETURNS
 *
 *  Nonzero if a message is available and is retrieved, zero otherwise.
 *
 * CONFORMANCE
 *
 * ECMA-234, Win
 *
 */
WINBOOL STDCALL PeekMessageA( LPMSG msg, HWND hwnd, UINT first,
                             UINT last, UINT flags )
{
    return MSG_PeekMessage( msg, hwnd, first, last, flags, FALSE );
}


WINBOOL STDCALL PeekMessageW( LPMSG msg, HWND hwnd, UINT first,
                             UINT last, UINT flags )
{
    return MSG_PeekMessage( msg, hwnd, first, last, flags, TRUE );
}

/***********************************************************************
 *          GetMessageW   (USER.274) Retrieve next message
 *
 * GetMessage retrieves the next event from the calling thread's
 * queue and deposits it in *lpmsg.
 *
 * If _hwnd_ is not NULL, only messages for window _hwnd_ and its
 * children as specified by IsChild() are retrieved. If _hwnd_ is NULL
 * all application messages are retrieved.
 *
 * _min_ and _max_ specify the range of messages of interest. If
 * min==max==0, no filtering is performed. Useful examples are
 * WM_KEYFIRST and WM_KEYLAST to retrieve keyboard input, and
 * WM_MOUSEFIRST and WM_MOUSELAST to retrieve mouse input.
 *
 * WM_PAINT messages are not removed from the queue; they remain until
 * processed. Other messages are removed from the queue.
 *
 * RETURNS
 *
 * -1 on error, 0 if message is WM_QUIT, nonzero otherwise.
 *
 * CONFORMANCE
 *
 * ECMA-234, Win
 * 
 */
WINBOOL
STDCALL
GetMessageA(LPMSG lpMsg, HWND hWnd ,
    UINT wMsgFilterMin, UINT wMsgFilterMax)
{
   
    MSG_PeekMessage( lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, PM_REMOVE, FALSE );
		   
    HOOK_CallHooksA( WH_GETMESSAGE, HC_ACTION, 0, (LPARAM)lpMsg );
    return (lpMsg->message != WM_QUIT);
}




WINBOOL
STDCALL
GetMessageW(
    LPMSG lpMsg,
    HWND hWnd ,
    UINT wMsgFilterMin,
    UINT wMsgFilterMax)
{
   
    MSG_PeekMessage( lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, PM_REMOVE, FALSE );

    HOOK_CallHooksW( WH_GETMESSAGE, HC_ACTION, 0, (LPARAM)lpMsg );
    return (lpMsg->message != WM_QUIT);
}


/***********************************************************************
 *           PostMessageA   (USER.419)
 */

WINBOOL STDCALL PostMessageA( HWND hwnd, UINT message, WPARAM wParam,
                             LPARAM lParam )
{
    MSG 	msg;
    WND 	*wndPtr;


 

    msg.hwnd    = hwnd;
    msg.message = message;
    msg.wParam  = wParam;
    msg.lParam  = lParam;
    msg.time    = GetTickCount();
    msg.pt.x    = 0;
    msg.pt.y    = 0;

#ifdef CONFIG_IPC
    if (DDE_PostMessage(&msg))
       return TRUE;
#endif  /* CONFIG_IPC */
    
    if (hwnd == HWND_BROADCAST)
    {
        DPRINT("HWND_BROADCAST !\n");
        for (wndPtr = WIN_GetDesktop()->child; wndPtr; wndPtr = wndPtr->next)
        {
            if (wndPtr->dwStyle & WS_POPUP || wndPtr->dwStyle & WS_CAPTION)
            {
                DPRINT("BROADCAST Message to hWnd=%04x m=%04X w=%04X l=%08lX !\n",
                            wndPtr->hwndSelf, message, wParam, lParam);
                PostMessageA( wndPtr->hwndSelf, message, wParam, lParam );
            }
        }
        DPRINT("End of HWND_BROADCAST !\n");
        return TRUE;
    }

    wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr || !wndPtr->hmemTaskQ) return FALSE;

    return QUEUE_AddMsg( wndPtr->hmemTaskQ, &msg, 0 );
}





/***********************************************************************
 *           PostMessageW   (USER.420)
 */
WINBOOL STDCALL PostMessageW( HWND hwnd, UINT message, WPARAM wParam,
                              LPARAM lParam )
{
     MSG 	msg;
    WND 	*wndPtr;


 

    msg.hwnd    = hwnd;
    msg.message = message;
    msg.wParam  = wParam;
    msg.lParam  = lParam;
    msg.time    = GetTickCount();
    msg.pt.x    = 0;
    msg.pt.y    = 0;

#ifdef CONFIG_IPC
    if (DDE_PostMessage(&msg))
       return TRUE;
#endif  /* CONFIG_IPC */
    
    if (hwnd == HWND_BROADCAST)
    {
        DPRINT("HWND_BROADCAST !\n");
        for (wndPtr = WIN_GetDesktop()->child; wndPtr; wndPtr = wndPtr->next)
        {
            if (wndPtr->dwStyle & WS_POPUP || wndPtr->dwStyle & WS_CAPTION)
            {
                DPRINT("BROADCAST Message to hWnd=%04x m=%04X w=%04X l=%08lX !\n",
                            wndPtr->hwndSelf, message, wParam, lParam);
                PostMessageA( wndPtr->hwndSelf, message, wParam, lParam );
            }
        }
        DPRINT("End of HWND_BROADCAST !\n");
        return TRUE;
    }

    wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr || !wndPtr->hmemTaskQ) return FALSE;

    return QUEUE_AddMsg( wndPtr->hmemTaskQ, &msg, 0 );
}



/***********************************************************************
 *           SendMessageW   (USER.459)  Send Window Message
 *
 *  Sends a message to the window procedure of the specified window.
 *  SendMessage() will not return until the called window procedure
 *  either returns or calls ReplyMessage().
 *
 *  Use PostMessage() to send message and return immediately. A window
 *  procedure may use InSendMessage() to detect
 *  SendMessage()-originated messages.
 *
 *  Applications which communicate via HWND_BROADCAST may use
 *  RegisterWindowMessage() to obtain a unique message to avoid conflicts
 *  with other applications.
 *
 * CONFORMANCE
 * 
 *  ECMA-234, Win 
 */

LRESULT STDCALL SendMessageA( HWND hwnd, UINT msg, WPARAM wParam,
                               LPARAM lParam )
{
    WND *wndPtr;

    if (hwnd == HWND_BROADCAST || hwnd == HWND_TOPMOST)
    {
 	return MSG_SendMessageInterTask(hwnd,msg,wParam,lParam,FALSE);
    }


    if (!(wndPtr = WIN_FindWndPtr( hwnd )))
    {
        DPRINT( "invalid hwnd %08x\n", hwnd );
        return 0;
    }

    if (HOOK_IsHooked( WH_CALLWNDPROC ))
	MSG_CallWndProcHook( (LPMSG)&hwnd, FALSE);

  
    return MSG_SendMessage(wndPtr,msg,wParam,lParam);
}



LRESULT STDCALL SendMessageW(  HWND hwnd,  UINT msg,  WPARAM wParam, 
	LPARAM lParam   ) 
{
    WND *wndPtr;

    if (hwnd == HWND_BROADCAST || hwnd == HWND_TOPMOST)
    {
 	return MSG_SendMessageInterTask(hwnd,msg,wParam,lParam,TRUE);
    }


    if (!(wndPtr = WIN_FindWndPtr( hwnd )))
    {
        DPRINT( "invalid hwnd %08x\n", hwnd );
        return 0;
    }

    if (HOOK_IsHooked( WH_CALLWNDPROC ))
	MSG_CallWndProcHook( (LPMSG)&hwnd, FALSE);

  
    return MSG_SendMessage(wndPtr,msg,wParam,lParam);
}

/***********************************************************************
 *           TranslateMessage   (USER.556)
 */
WINBOOL STDCALL TranslateMessage( const MSG *msg )
{
    return MSG_DoTranslateMessage( msg->message, msg->hwnd,
                                   msg->wParam, msg->lParam );
}

/***********************************************************************
 *           DispatchMessageA   (USER.141)
 */
LONG STDCALL DispatchMessageA( const MSG* msg )
{
    WND * wndPtr;
    LONG retval;
    int painting;
    
      /* Process timer messages */
    if ((msg->message == WM_TIMER) || (msg->message == WM_SYSTIMER))
    {
	if (msg->lParam)
        {
/*            HOOK_CallHooksA( WH_CALLWNDPROC, HC_ACTION, 0, FIXME ); */
	    return CallWindowProcA( (WNDPROC)msg->lParam, msg->hwnd,
                                   msg->message, msg->wParam, GetTickCount() );
        }
    }

    if (!msg->hwnd) return 0;
    if (!(wndPtr = WIN_FindWndPtr( msg->hwnd ))) return 0;
    if (!wndPtr->winproc) return 0;
    painting = (msg->message == WM_PAINT);
    if (painting) wndPtr->flags |= WIN_NEEDS_BEGINPAINT;
/*    HOOK_CallHooksA( WH_CALLWNDPROC, HC_ACTION, 0, FIXME ); */

    SPY_EnterMessage( SPY_DISPATCHMESSAGE, msg->hwnd, msg->message,
                      msg->wParam, msg->lParam );
    retval = CallWindowProcA( (WNDPROC)wndPtr->winproc,
                                msg->hwnd, msg->message,
                                msg->wParam, msg->lParam );
    SPY_ExitMessage( SPY_RESULT_OK, msg->hwnd, msg->message, retval );

    if (painting && (wndPtr = WIN_FindWndPtr( msg->hwnd )) &&
        (wndPtr->flags & WIN_NEEDS_BEGINPAINT) && wndPtr->hrgnUpdate)
    {
	//ERR(msg, "BeginPaint not called on WM_PAINT for hwnd %04x!\n", 
	//    msg->hwnd);
	wndPtr->flags &= ~WIN_NEEDS_BEGINPAINT;
        /* Validate the update region to avoid infinite WM_PAINT loop */
        ValidateRect( msg->hwnd, NULL );
    }
    return retval;
}


/***********************************************************************
 *           DispatchMessageW   (USER.142)     Process Message
 *
 * Process the message specified in the structure *_msg_.
 *
 * If the lpMsg parameter points to a WM_TIMER message and the
 * parameter of the WM_TIMER message is not NULL, the lParam parameter
 * points to the function that is called instead of the window
 * procedure.
 *  
 * The message must be valid.
 *
 * RETURNS
 *
 *   DispatchMessage() returns the result of the window procedure invoked.
 *
 * CONFORMANCE
 *
 *   ECMA-234, Win 
 *
 */
LONG STDCALL DispatchMessageW( const MSG* msg )
{
    WND * wndPtr;
    LONG retval;
    int painting;
    
      /* Process timer messages */
    if ((msg->message == WM_TIMER) || (msg->message == WM_SYSTIMER))
    {
	if (msg->lParam)
        {
/*            HOOK_CallHooksW( WH_CALLWNDPROC, HC_ACTION, 0, FIXME ); */
	    return CallWindowProcW( (WNDPROC)msg->lParam, msg->hwnd,
                                   msg->message, msg->wParam, GetTickCount() );
        }
    }

    if (!msg->hwnd) return 0;
    if (!(wndPtr = WIN_FindWndPtr( msg->hwnd ))) return 0;
    if (!wndPtr->winproc) return 0;
    painting = (msg->message == WM_PAINT);
    if (painting) wndPtr->flags |= WIN_NEEDS_BEGINPAINT;
/*    HOOK_CallHooksW( WH_CALLWNDPROC, HC_ACTION, 0, FIXME ); */

    SPY_EnterMessage( SPY_DISPATCHMESSAGE, msg->hwnd, msg->message,
                      msg->wParam, msg->lParam );
    retval = CallWindowProcW( (WNDPROC)wndPtr->winproc,
                                msg->hwnd, msg->message,
                                msg->wParam, msg->lParam );
    SPY_ExitMessage( SPY_RESULT_OK, msg->hwnd, msg->message, retval );

    if (painting && (wndPtr = WIN_FindWndPtr( msg->hwnd )) &&
        (wndPtr->flags & WIN_NEEDS_BEGINPAINT) && wndPtr->hrgnUpdate)
    {
	//ERR(msg, "BeginPaint not called on WM_PAINT for hwnd %04x!\n", 
	//    msg->hwnd);
	wndPtr->flags &= ~WIN_NEEDS_BEGINPAINT;
        /* Validate the update region to avoid infinite WM_PAINT loop */
        ValidateRect( msg->hwnd, NULL );
    }
    return retval;
}



/**********************************************************************
 *           PostThreadMessageA    (USER.422)
 *
 * BUGS
 *
 *  Thread-local message queues are not supported.
 * 
 */
WINBOOL STDCALL PostThreadMessageA(DWORD idThread , UINT message,
                                   WPARAM wParam, LPARAM lParam )
{
	return FALSE;
}

/**********************************************************************
 *           PostThreadMessageW    (USER.423)
 *
 * BUGS
 *
 *  Thread-local message queues are not supported.
 * 
 */
WINBOOL STDCALL PostThreadMessageW(DWORD idThread , UINT message,
                                   WPARAM wParam, LPARAM lParam )
{
	return FALSE;
}

/***********************************************************************
 *           SendMessageTimeoutA   (USER.457)
 */
LRESULT STDCALL SendMessageTimeoutA( HWND hwnd, UINT msg, WPARAM wParam,
				      LPARAM lParam, UINT flags,
				      UINT timeout, LPDWORD resultp)
{
 // DPRINT( "(...): semistub\n");
  return SendMessageA (hwnd, msg, wParam, lParam);
}


/***********************************************************************
 *           SendMessageTimeoutW   (USER.458)
 */
LRESULT STDCALL SendMessageTimeoutW( HWND hwnd, UINT msg, WPARAM wParam,
				      LPARAM lParam, UINT flags,
				      UINT timeout, LPDWORD resultp)
{
//  DPRINT( "(...): semistub\n");
  return SendMessageW (hwnd, msg, wParam, lParam);
}


/***********************************************************************
 *  WaitMessage    (USER.112) (USER.578)  Suspend thread pending messages
 *
 * WaitMessage() suspends a thread until events appear in the thread's
 * queue.
 *
 * BUGS
 *
 * Is supposed to return WINBOOL under Win.
 *
 * Thread-local message queues are not supported.
 *
 * CONFORMANCE
 *
 * ECMA-234, Win
 * 
 */
WINBOOL
STDCALL
WaitMessage(VOID)
{
    QUEUE_WaitBits( QS_ALLINPUT );
    return TRUE;
}

/***********************************************************************
 *           MsgWaitForMultipleObjects    (USER.400)
 */
DWORD STDCALL MsgWaitForMultipleObjects( DWORD nCount, HANDLE *pHandles,
                                        WINBOOL fWaitAll, DWORD dwMilliseconds,
                                        DWORD dwWakeMask )
{

}


/***********************************************************************
 *           RegisterWindowMessageA   (USER.437)
 */
UINT
STDCALL
RegisterWindowMessageA(
    LPCSTR lpString)
{
    return GlobalAddAtomA( lpString );
}


/***********************************************************************
 *           RegisterWindowMessageW   (USER.438)
 */
UINT
STDCALL
RegisterWindowMessageW(
    LPCWSTR lpString)
{
    return GlobalAddAtomW( lpString );
}

/***********************************************************************
 *           InSendMessage    (USER.0)
 */
WINBOOL STDCALL InSendMessage(void)
{
    MESSAGEQUEUE *queue;

    if (!(queue = (MESSAGEQUEUE *)GlobalLock( GetFastQueue() )))
        return 0;
    return (WINBOOL)queue->InSendMessageHandle;
}

/***********************************************************************
 *           BroadcastSystemMessage    (USER.12)
 */
LONG STDCALL BroadcastSystemMessage(
	DWORD dwFlags,LPDWORD recipients,UINT uMessage,WPARAM wParam,
	LPARAM lParam
) {
	DPRINT("(%08lx,%08lx,%08x,%08x,%08lx): stub!\n",
	      dwFlags,*recipients,uMessage,wParam,lParam
	);
	return 0;
}

/***********************************************************************
 *           SendNotifyMessageA    (USER.460)
 * FIXME
 *  The message sended with PostMessage has to be put in the queue
 *  with a higher priority as the other "Posted" messages.
 *  QUEUE_AddMsg has to be modifyed.
 */
WINBOOL STDCALL SendNotifyMessageA(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{	WINBOOL ret = TRUE;
	DPRINT("(%04x,%08x,%08x,%08lx) not complete\n",
	      hwnd, msg, wParam, lParam);
	      
	if ( GetCurrentThreadId() == GetWindowThreadProcessId ( hwnd, NULL))
	{	ret=SendMessageA ( hwnd, msg, wParam, lParam );
	}
	else
	{	PostMessageA ( hwnd, msg, wParam, lParam );
	}
	return ret;
}

/***********************************************************************
 *           SendNotifyMessageW    (USER.461)
 * FIXME
 *  The message sended with PostMessage has to be put in the queue
 *  with a higher priority as the other "Posted" messages.
 *  QUEUE_AddMsg has to be modifyed.
 */
WINBOOL STDCALL SendNotifyMessageW(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{       WINBOOL ret = TRUE;
	DPRINT("(%04x,%08x,%08x,%08lx) not complete\n",
	      hwnd, msg, wParam, lParam);

	if ( GetCurrentThreadId() == GetWindowThreadProcessId ( hwnd, NULL))
	{       ret=SendMessageW ( hwnd, msg, wParam, lParam );
	}
	else
	{       PostMessageW ( hwnd, msg, wParam, lParam );
	}
	return ret;
}

/***********************************************************************
 *           SendMessageCallBackA
 * FIXME: It's like PostMessage. The callback gets called when the message
 * is processed. We have to modify the message processing for a exact
 * implementation...
 */
WINBOOL
STDCALL
SendMessageCallbackA(
    HWND hWnd,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam,
    SENDASYNCPROC lpResultCallBack,
    DWORD dwData)
{	

	if ( hWnd == HWND_BROADCAST)
	{	PostMessageA( hWnd, Msg, wParam, lParam);
	//	DPRINT("Broadcast: Callback will not be called!\n");
		return TRUE;
	}
	(lpResultCallBack)( hWnd, Msg, dwData, SendMessageA ( hWnd, Msg, wParam, lParam ));
		return TRUE;
}

/***********************************************************************
 *           ReplyMessage   (USER.115)
 */
WINBOOL STDCALL ReplyMessage( LRESULT result )
{
    MESSAGEQUEUE *senderQ;
    MESSAGEQUEUE *queue;

    if (!(queue = (MESSAGEQUEUE*)GlobalLock( GetFastQueue() ))) return;

    DPRINT("ReplyMessage, queue %04x\n", queue->self);

    while( (senderQ = (MESSAGEQUEUE*)GlobalLock( queue->InSendMessageHandle)))
    {
      DPRINT("\trpm: replying to %04x (%04x -> %04x)\n",
                          queue->msg, queue->self, senderQ->self);

      if( queue->wakeBits & QS_SENDMESSAGE )
      {
	QUEUE_ReceiveMessage( queue );
	continue; /* ReceiveMessage() already called us */
      }

      if(!(senderQ->wakeBits & QS_SMRESULT) ) break;
//      if (THREAD_IsWinA(THREAD_Current())) OldYield();
    } 
    if( !senderQ ) { DPRINT("\trpm: done\n"); return; }

    senderQ->SendMessageReturn = result;
    DPRINT("\trpm: smResult = %08x, result = %08lx\n", 
			(unsigned)queue->smResultCurrent, result );

    senderQ->smResult = queue->smResultCurrent;
    queue->InSendMessageHandle = 0;

    QUEUE_SetWakeBit( senderQ, QS_SMRESULT );
//    if (THREAD_IsWinA(THREAD_Current())) DirectedYield( senderQ->hTask );
}




/***********************************************************************
 *           PostQuitMessage   (USER.421)
 *
 * PostQuitMessage() posts a message to the system requesting an
 * application to terminate execution. As a result of this function,
 * the WM_QUIT message is posted to the application, and
 * PostQuitMessage() returns immediately.  The exitCode parameter
 * specifies an application-defined exit code, which appears in the
 * _wParam_ parameter of the WM_QUIT message posted to the application.  
 *
 * CONFORMANCE
 *
 *  ECMA-234, Win
 */
void STDCALL PostQuitMessage( INT exitCode )
{
    MESSAGEQUEUE *queue;

    if (!(queue = (MESSAGEQUEUE *)GlobalLock( GetFastQueue() ))) return;
    queue->wPostQMsg = TRUE;
    queue->wExitCode = (WORD)exitCode;
}




/***********************************************************************
 *           GetWindowThreadProcessId   (USER.313)
 */
DWORD STDCALL GetWindowThreadProcessId( HWND hwnd, LPDWORD process )
{
	return 0;
}





/***********************************************************************
 *           SetMessageQueue   (USER.494)
 */
WINBOOL STDCALL SetMessageQueue( INT size )
{

    HQUEUE hQueue, hNewQueue;
    MESSAGEQUEUE *queuePtr;

    DPRINT("task %04x size %i\n", GetCurrentTask(), size); 

    if ((size > MAX_QUEUE_SIZE) || (size <= 0)) return FALSE;

    if( !(hNewQueue = QUEUE_CreateMsgQueue( size ))) 
    {
	DPRINT( "failed!\n");
	return FALSE;
    }
    queuePtr = (MESSAGEQUEUE *)GlobalLock( hNewQueue );

    SIGNAL_MaskAsyncEvents( TRUE );

    /* Copy data and free the old message queue */
    if ((hQueue = GetThreadQueue(0)) != 0) 
    {
       MESSAGEQUEUE *oldQ = (MESSAGEQUEUE *)GlobalLock( hQueue );
       memcpy( &queuePtr->wParamHigh, &oldQ->wParamHigh,
			(int)oldQ->messages - (int)(&oldQ->wParamHigh) );
       HOOK_ResetQueueHooks( hNewQueue );
       if( WIN_GetDesktop()->hmemTaskQ == hQueue )
	   WIN_GetDesktop()->hmemTaskQ = hNewQueue;
       WIN_ResetQueueWindows( WIN_GetDesktop(), hQueue, hNewQueue );
//       CLIPBOARD_ResetLock( hQueue, hNewQueue );
       QUEUE_DeleteMsgQueue( hQueue );
    }

    /* Link new queue into list */
    queuePtr->hTask = GetCurrentTask();
    queuePtr->next  = hFirstQueue;
    hFirstQueue = hNewQueue;
    
    if( !queuePtr->next ) pCursorQueue = queuePtr;
    SetThreadQueue( 0, hNewQueue );
    
    SIGNAL_MaskAsyncEvents( FALSE );
    return TRUE;


// win32 dynamically expands the message queue
}



/***********************************************************************
 *           GetQueueStatus   (USER.283)
 */
DWORD STDCALL GetQueueStatus( UINT flags )
{
    MESSAGEQUEUE *queue;
    DWORD ret;

    if (!(queue = (MESSAGEQUEUE *)GlobalLock( GetFastQueue() ))) return 0;
    ret = MAKELONG( queue->changeBits, queue->wakeBits );
    queue->changeBits = 0;
    return ret & MAKELONG( flags, flags );
}


/***********************************************************************
 *           WaitForInputIdle   (USER.577)
 */
DWORD STDCALL WaitForInputIdle (HANDLE hProcess, DWORD dwTimeOut)
{
 // FIXME (msg, "(hProcess=%d, dwTimeOut=%ld): stub\n", hProcess, dwTimeOut);

  return WAIT_TIMEOUT;
}


/***********************************************************************
 *           GetInputState   (USER.244)
 */
WINBOOL STDCALL GetInputState(void)
{
    MESSAGEQUEUE *queue;

    if (!(queue = (MESSAGEQUEUE *)GlobalLock( GetFastQueue() )))
        return FALSE;
    return queue->wakeBits & (QS_KEY | QS_MOUSEBUTTON);
}



/***********************************************************************
 *           GetMessagePos   (USER.272)
 * 
 * The GetMessagePos() function returns a long value representing a
 * cursor position, in screen coordinates, when the last message
 * retrieved by the GetMessage() function occurs. The x-coordinate is
 * in the low-order word of the return value, the y-coordinate is in
 * the high-order word. The application can use the MAKEPOINT()
 * macro to obtain a POINT structure from the return value. 
 *
 * For the current cursor position, use GetCursorPos().
 *
 * RETURNS
 *
 * Cursor position of last message on success, zero on failure.
 *
 * CONFORMANCE
 *
 * ECMA-234, Win
 *
 */
DWORD STDCALL GetMessagePos(void)
{
    MESSAGEQUEUE *queue;

    if (!(queue = (MESSAGEQUEUE *)GlobalLock( GetFastQueue() ))) return 0;
    return queue->GetMessagePosVal;
}


/***********************************************************************
 *           GetMessageTime   (USER.273)
 *
 * GetMessageTime() returns the message time for the last message
 * retrieved by the function. The time is measured in milliseconds with
 * the same offset as GetTickCount().
 *
 * Since the tick count wraps, this is only useful for moderately short
 * relative time comparisons.
 *
 * RETURNS
 *
 * Time of last message on success, zero on failure.
 *
 * CONFORMANCE
 *
 * ECMA-234, Win
 *  
 */
LONG STDCALL GetMessageTime(void)
{
    MESSAGEQUEUE *queue;

    if (!(queue = (MESSAGEQUEUE *)GlobalLock( GetFastQueue() ))) return 0;
    return queue->GetMessageTimeVal;
}


/***********************************************************************
 *           GetMessageExtraInfo  (USER.271)
 */
LONG STDCALL GetMessageExtraInfo(void)
{
    MESSAGEQUEUE *queue;

    if (!(queue = (MESSAGEQUEUE *)GlobalLock( GetFastQueue() ))) return 0;
    return queue->GetMessageExtraInfoVal;
}


WINBOOL STDCALL MessageBeep( UINT uType )
{
    return Beep(uType ,0);
}


