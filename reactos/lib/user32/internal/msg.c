/*
 * Message queues related functions
 *
 * Copyright 1993, 1994 Alexandre Julliard
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/types.h>
#include <windows.h>
#include <user32/msg.h>
#include <user32/spy.h>
#include <user32/hook.h>
#include <user32/debug.h>
#include <user32/winpos.h>
#include <user32/queue.h>



UINT doubleClickSpeed = 452;
INT debugSMRL = 0;       /* intertask SendMessage() recursion level */

/***********************************************************************
 *           MSG_CheckFilter
 */
WINBOOL MSG_CheckFilter(WORD uMsg, DWORD filter)
{
   if( filter )
       return (uMsg >= LOWORD(filter) && uMsg <= HIWORD(filter));
   return TRUE;
}

/***********************************************************************
 *           MSG_SendParentNotify
 *
 * Send a WM_PARENTNOTIFY to all ancestors of the given window, unless
 * the window has the WS_EX_NOPARENTNOTIFY style.
 */
void MSG_SendParentNotify(WND* wndPtr, WORD event, WORD idChild, LPARAM lValue)
{
#define lppt ((LPPOINT)&lValue)

    /* pt has to be in the client coordinates of the parent window */

    MapWindowPoints( 0, wndPtr->hwndSelf, lppt, 1 );
    while (wndPtr)
    {
	if (!(wndPtr->dwStyle & WS_CHILD) || (wndPtr->dwExStyle & WS_EX_NOPARENTNOTIFY)) break;
	lppt->x += wndPtr->rectClient.left;
	lppt->y += wndPtr->rectClient.top;
	wndPtr = wndPtr->parent;
	SendMessageW( wndPtr->hwndSelf, WM_PARENTNOTIFY,
			MAKEWPARAM( event, idChild ), lValue );
    }
#undef lppt
}


/***********************************************************************
 *           MSG_TranslateMouseMsg
 *
 * Translate an mouse hardware event into a real mouse message.
 * Return value indicates whether the translated message must be passed
 * to the user, left in the queue, or skipped entirely (in this case
 * HIWORD contains hit test code).
 */
DWORD MSG_TranslateMouseMsg( HWND hTopWnd, DWORD filter, 
				    MSG *msg, WINBOOL remove, WND* pWndScope )
{
    DWORD   dblclk_time_limit = 0;
    UINT     clk_message = 0;
    HWND     clk_hwnd = 0;
    POINT    clk_pos = { 0, 0 };

    WND *pWnd;
    HWND hWnd;
    INT ht, hittest, sendSC = 0;
    UINT message = msg->message;
    POINT screen_pt, pt;
    HANDLE hQ = GetFastQueue();
    MESSAGEQUEUE *queue = (MESSAGEQUEUE *)GlobalLock(hQ);
    WINBOOL eatMsg = FALSE;
    WINBOOL mouseClick = ((message == WM_LBUTTONDOWN) ||
		         (message == WM_RBUTTONDOWN) ||
		         (message == WM_MBUTTONDOWN))?1:0;
    SYSQ_STATUS ret = 0;

      /* Find the window */

    ht = hittest = HTCLIENT;
    hWnd = GetCapture();
    if( !hWnd )
    {
	ht = hittest = WINPOS_WindowFromPoint( pWndScope, msg->pt, &pWnd );
	if( !pWnd ) pWnd = WIN_GetDesktop();
	hWnd = pWnd->hwndSelf;
	sendSC = 1;
    } 
    else 
    {
	pWnd = WIN_FindWndPtr(hWnd);
	ht = EVENT_GetCaptureInfo();
    }

	/* stop if not the right queue */

    if (pWnd->hmemTaskQ != hQ)
    {
        /* Not for the current task */
        if (queue) QUEUE_ClearWakeBit( queue, QS_MOUSE );
        /* Wake up the other task */
        queue = (MESSAGEQUEUE *)GlobalLock( pWnd->hmemTaskQ );
        if (queue) QUEUE_SetWakeBit( queue, QS_MOUSE );
        return SYSQ_MSG_ABANDON;
    }

	/* check if hWnd is within hWndScope */

    if( hTopWnd && hWnd != hTopWnd )
	if( !IsChild(hTopWnd, hWnd) ) return SYSQ_MSG_CONTINUE;

    if( mouseClick )
    {
	/* translate double clicks -
	 * note that ...MOUSEMOVEs can slip in between
	 * ...BUTTONDOWN and ...BUTTONDBLCLK messages */

	if( pWnd->class->style & CS_DBLCLKS || ht != HTCLIENT )
	{
           if ((message == clk_message) && (hWnd == clk_hwnd) &&
               (msg->time - dblclk_time_limit < doubleClickSpeed) &&
               (abs(msg->pt.x - clk_pos.x) < SYSMETRICS_CXDOUBLECLK/2) &&
               (abs(msg->pt.y - clk_pos.y) < SYSMETRICS_CYDOUBLECLK/2))
	   {
	      message += (WM_LBUTTONDBLCLK - WM_LBUTTONDOWN);
	      mouseClick++;   /* == 2 */
	   }
	}
    }
    screen_pt = pt = msg->pt;

    if (hittest != HTCLIENT)
    {
	message += ((INT)WM_NCMOUSEMOVE - WM_MOUSEMOVE);
	msg->wParam = hittest;
    }
    else ScreenToClient( hWnd, &pt );

	/* check message filter */

    if (!MSG_CheckFilter(message, filter)) return SYSQ_MSG_CONTINUE;

    pCursorQueue = queue;

	/* call WH_MOUSE */

    if (HOOK_IsHooked( WH_MOUSE ))
    { 
	MOUSEHOOKSTRUCT *hook = malloc(sizeof(MOUSEHOOKSTRUCT));
	if( hook )
	{
	    hook->pt           = screen_pt;
	    hook->hwnd         = hWnd;
	    hook->wHitTestCode = hittest;
	    hook->dwExtraInfo  = 0;
	    ret = HOOK_CallHooksA( WH_MOUSE, remove ? HC_ACTION : HC_NOREMOVE,
	                            message, (LPARAM)(hook) );
	    free(hook);
	}
	if( ret ) return MAKELONG((INT)SYSQ_MSG_SKIP, hittest);
    }

    if ((hittest == HTERROR) || (hittest == HTNOWHERE)) 
	eatMsg = sendSC = 1;
    else if( remove && mouseClick )
    {
        HWND hwndTop = WIN_GetTopParent( hWnd );

	if( mouseClick == 1 )
	{
	    /* set conditions */
	    dblclk_time_limit = msg->time;
	       clk_message = msg->message;
	       clk_hwnd = hWnd;
	       clk_pos = screen_pt;
	} else 
	    /* got double click - zero them out */
	    dblclk_time_limit = clk_hwnd = 0;

	if( sendSC )
	{
            /* Send the WM_PARENTNOTIFY,
	     * note that even for double/nonclient clicks
	     * notification message is still WM_L/M/RBUTTONDOWN.
	     */

            MSG_SendParentNotify( pWnd, msg->message, 0, MAKELPARAM(screen_pt.x, screen_pt.y) );

            /* Activate the window if needed */

            if (hWnd != GetActiveWindow() && hWnd != GetDesktopWindow())
            {
                LONG ret = SendMessageW( hWnd, WM_MOUSEACTIVATE,(WPARAM) hwndTop,
                                          (LPARAM)MAKELONG( hittest, message ) );

                if ((ret == MA_ACTIVATEANDEAT) || (ret == MA_NOACTIVATEANDEAT))
                         eatMsg = TRUE;

                if (((ret == MA_ACTIVATE) || (ret == MA_ACTIVATEANDEAT)) 
                      && hwndTop != GetActiveWindow() )
                      if (!WINPOS_SetActiveWindow( hwndTop, TRUE , TRUE ))
			 eatMsg = TRUE;
            }
	}
    } else sendSC = (remove && sendSC);

     /* Send the WM_SETCURSOR message */

    if (sendSC)
        SendMessageW( hWnd, WM_SETCURSOR, (WPARAM)hWnd,
                       MAKELONG( hittest, message ));
    if (eatMsg) return MAKELONG( (UINT)SYSQ_MSG_SKIP, hittest);

    msg->hwnd    = hWnd;
    msg->message = message;
    msg->lParam  = MAKELONG( pt.x, pt.y );
    return SYSQ_MSG_ACCEPT;
}


/***********************************************************************
 *           MSG_TranslateKbdMsg
 *
 * Translate an keyboard hardware event into a real message.
 */
DWORD MSG_TranslateKbdMsg( HWND hTopWnd, DWORD filter,
				  MSG *msg, WINBOOL remove )
{
    WORD message = msg->message;
    HWND hWnd = GetFocus();
    WND *pWnd;

      /* Should check Ctrl-Esc and PrintScreen here */

    if (!hWnd)
    {
	  /* Send the message to the active window instead,  */
	  /* translating messages to their WM_SYS equivalent */

	hWnd = GetActiveWindow();

	if( message < WM_SYSKEYDOWN )
	    message += WM_SYSKEYDOWN - WM_KEYDOWN;
    }
    pWnd = WIN_FindWndPtr( hWnd );
    if (pWnd && (pWnd->hmemTaskQ != GetFastQueue()))
    {
        /* Not for the current task */
        MESSAGEQUEUE *queue = (MESSAGEQUEUE *)GlobalLock( GetFastQueue() );
        if (queue) QUEUE_ClearWakeBit( queue, QS_KEY );
        /* Wake up the other task */
        queue = (MESSAGEQUEUE *)GlobalLock( pWnd->hmemTaskQ );
        if (queue) QUEUE_SetWakeBit( queue, QS_KEY );
        return SYSQ_MSG_ABANDON;
    }

    if (hTopWnd && hWnd != hTopWnd)
	if (!IsChild(hTopWnd, hWnd)) return SYSQ_MSG_CONTINUE;
    if (!MSG_CheckFilter(message, filter)) return SYSQ_MSG_CONTINUE;

    msg->hwnd = hWnd;
    msg->message = message;

    return (HOOK_CallHooksA( WH_KEYBOARD, remove ? HC_ACTION : HC_NOREMOVE,
			      msg->wParam, msg->lParam )
            ? SYSQ_MSG_SKIP : SYSQ_MSG_ACCEPT);
}


/***********************************************************************
 *           MSG_JournalRecordMsg
 *
 * Build an EVENTMSG structure and call JOURNALRECORD hook
 */
void MSG_JournalRecordMsg( MSG *msg )
{
    EVENTMSG *event = malloc(sizeof(EVENTMSG));
    if (!event) return;
    event->message = msg->message;
    event->time = msg->time;
    if ((msg->message >= WM_KEYFIRST) && (msg->message <= WM_KEYLAST))
    {
        event->paramL = (msg->wParam & 0xFF) | (HIWORD(msg->lParam) << 8);
        event->paramH = msg->lParam & 0x7FFF;  
        if (HIWORD(msg->lParam) & 0x0100)
            event->paramH |= 0x8000;               /* special_key - bit */
        HOOK_CallHooksA( WH_JOURNALRECORD, HC_ACTION, 0,
                          (LPARAM)(event) );
    }
    else if ((msg->message >= WM_MOUSEFIRST) && (msg->message <= WM_MOUSELAST))
    {
        event->paramL = LOWORD(msg->lParam);       /* X pos */
        event->paramH = HIWORD(msg->lParam);       /* Y pos */ 
        ClientToScreen( msg->hwnd, (LPPOINT)&event->paramL );
        HOOK_CallHooksA( WH_JOURNALRECORD, HC_ACTION, 0,
                          (LPARAM)(event) );
    }
    else if ((msg->message >= WM_NCMOUSEFIRST) &&
             (msg->message <= WM_NCMOUSELAST))
    {
        event->paramL = LOWORD(msg->lParam);       /* X pos */
        event->paramH = HIWORD(msg->lParam);       /* Y pos */ 
        event->message += WM_MOUSEMOVE-WM_NCMOUSEMOVE;/* give no info about NC area */
        HOOK_CallHooksA( WH_JOURNALRECORD, HC_ACTION, 0,
                          (LPARAM)(event) );
    }
    free(event);
}

/***********************************************************************
 *          MSG_JournalPlayBackMsg
 *
 * Get an EVENTMSG struct via call JOURNALPLAYBACK hook function 
 */
int MSG_JournalPlayBackMsg(void)
{
 EVENTMSG *tmpMsg;
 long wtime,lParam;
 WORD keyDown,i,wParam,result=0;

 if ( HOOK_IsHooked( WH_JOURNALPLAYBACK ) )
 {
  tmpMsg = malloc(sizeof(EVENTMSG));
  wtime=HOOK_CallHooksA( WH_JOURNALPLAYBACK, HC_GETNEXT, 0,
			  (LPARAM)(tmpMsg));
  /*  DPRINT("Playback wait time =%ld\n",wtime); */
  if (wtime<=0)
  {
   wtime=0;
   if ((tmpMsg->message>= WM_KEYFIRST) && (tmpMsg->message <= WM_KEYLAST))
   {
     wParam=tmpMsg->paramL & 0xFF;
     lParam=MAKELONG(tmpMsg->paramH&0x7ffff,tmpMsg->paramL>>8);
     if (tmpMsg->message == WM_KEYDOWN || tmpMsg->message == WM_SYSKEYDOWN)
     {
       for (keyDown=i=0; i<256 && !keyDown; i++)
          if (InputKeyStateTable[i] & 0x80)
            keyDown++;
       if (!keyDown)
         lParam |= 0x40000000;       
       AsyncKeyStateTable[wParam]=InputKeyStateTable[wParam] |= 0x80;
     }  
     else                                       /* WM_KEYUP, WM_SYSKEYUP */
     {
       lParam |= 0xC0000000;      
       AsyncKeyStateTable[wParam]=InputKeyStateTable[wParam] &= ~0x80;
     }
     if (InputKeyStateTable[VK_MENU] & 0x80)
       lParam |= 0x20000000;     
     if (tmpMsg->paramH & 0x8000)              /*special_key bit*/
       lParam |= 0x01000000;
     hardware_event( tmpMsg->message, wParam, lParam,0, 0, tmpMsg->time, 0 );     
   }
   else
   {
    if ((tmpMsg->message>= WM_MOUSEFIRST) && (tmpMsg->message <= WM_MOUSELAST))
    {
     switch (tmpMsg->message)
     {
      case WM_LBUTTONDOWN:
          MouseButtonsStates[0]=AsyncMouseButtonsStates[0]=TRUE;break;
      case WM_LBUTTONUP:
          MouseButtonsStates[0]=AsyncMouseButtonsStates[0]=FALSE;break;
      case WM_MBUTTONDOWN:
          MouseButtonsStates[1]=AsyncMouseButtonsStates[1]=TRUE;break;
      case WM_MBUTTONUP:
          MouseButtonsStates[1]=AsyncMouseButtonsStates[1]=FALSE;break;
      case WM_RBUTTONDOWN:
          MouseButtonsStates[2]=AsyncMouseButtonsStates[2]=TRUE;break;
      case WM_RBUTTONUP:
          MouseButtonsStates[2]=AsyncMouseButtonsStates[2]=FALSE;break;      
     }
     AsyncKeyStateTable[VK_LBUTTON]= InputKeyStateTable[VK_LBUTTON] = MouseButtonsStates[0] ? 0x80 : 0;
     AsyncKeyStateTable[VK_MBUTTON]= InputKeyStateTable[VK_MBUTTON] = MouseButtonsStates[1] ? 0x80 : 0;
     AsyncKeyStateTable[VK_RBUTTON]= InputKeyStateTable[VK_RBUTTON] = MouseButtonsStates[2] ? 0x80 : 0;
     SetCursorPos(tmpMsg->paramL,tmpMsg->paramH);
     lParam=MAKELONG(tmpMsg->paramL,tmpMsg->paramH);
     wParam=0;             
     if (MouseButtonsStates[0]) wParam |= MK_LBUTTON;
     if (MouseButtonsStates[1]) wParam |= MK_MBUTTON;
     if (MouseButtonsStates[2]) wParam |= MK_RBUTTON;
     hardware_event( tmpMsg->message, wParam, lParam,  
                     tmpMsg->paramL, tmpMsg->paramH, tmpMsg->time, 0 );
    }
   }
   HOOK_CallHooksA( WH_JOURNALPLAYBACK, HC_SKIP, 0,
		     (LPARAM)(tmpMsg));
  }
  else
  {
    if( tmpMsg->message == WM_QUEUESYNC )
        if (HOOK_IsHooked( WH_CBT ))
            HOOK_CallHooksA( WH_CBT, HCBT_QS, 0, 0L);

    result= QS_MOUSE | QS_KEY; /* ? */
  }
  free(tmpMsg);
 }
 return result;
} 

/***********************************************************************
 *           MSG_PeekHardwareMsg
 *
 * Peek for a hardware message matching the hwnd and message filters.
 */
WINBOOL MSG_PeekHardwareMsg( MSG *msg, HWND hwnd, DWORD filter,
                                   WINBOOL remove )
{
    DWORD status = SYSQ_MSG_ACCEPT;
    MESSAGEQUEUE *sysMsgQueue = QUEUE_GetSysQueue();
    int i, kbd_msg, pos = sysMsgQueue->nextMessage;

    /* FIXME: there has to be a better way to do this */
//    joySendMessages();

    /* If the queue is empty, attempt to fill it */
//&& THREAD_IsWinA( THREAD_Current() )
    if (!sysMsgQueue->msgCount  && EVENT_Pending())
        EVENT_WaitNetEvent( FALSE, FALSE );

    for (i = kbd_msg = 0; i < sysMsgQueue->msgCount; i++, pos++)
    {
        if (pos >= sysMsgQueue->queueSize) pos = 0;
	*msg = sysMsgQueue->messages[pos].msg;

          /* Translate message */

        if ((msg->message >= WM_MOUSEFIRST) && (msg->message <= WM_MOUSELAST))
        {
	    HWND hWndScope = (HWND)sysMsgQueue->messages[pos].extraInfo;
// removed Options.managed &&
	    status = MSG_TranslateMouseMsg(hwnd, filter, msg, remove, 
					  ( IsWindow(hWndScope) ) 
					   ? WIN_FindWndPtr(hWndScope) : WIN_GetDesktop() );
	    kbd_msg = 0;
        }
        else if ((msg->message >= WM_KEYFIRST) && (msg->message <= WM_KEYLAST))
        {
            status = MSG_TranslateKbdMsg(hwnd, filter, msg, remove);
	    kbd_msg = 1;
        }
        else /* Non-standard hardware event */
        {
            HARDWAREHOOKSTRUCT *hook;
            if ( (hook = malloc(sizeof(HARDWAREHOOKSTRUCT))) )
            {
                WINBOOL ret;
                hook->hWnd     = msg->hwnd;
                hook->wMessage = msg->message;
                hook->wParam   = msg->wParam;
                hook->lParam   = msg->lParam;
                ret = HOOK_CallHooksA( WH_HARDWARE,
                                        remove ? HC_ACTION : HC_NOREMOVE,
                                        0, (LPARAM)hook );
                free(hook);
                if (ret) 
		{
		    QUEUE_RemoveMsg( sysMsgQueue, pos );
		    continue;
		}
		status = SYSQ_MSG_ACCEPT; 
            }
        }

	switch (LOWORD(status))
	{
	   case SYSQ_MSG_ACCEPT:
		break;

	   case SYSQ_MSG_SKIP:
                if (HOOK_IsHooked( WH_CBT ))
                {
                   if( kbd_msg )
		       HOOK_CallHooksA( WH_CBT, HCBT_KEYSKIPPED, 
						 msg->wParam, msg->lParam );
		   else
		   {
                       MOUSEHOOKSTRUCT *hook = malloc(sizeof(MOUSEHOOKSTRUCT));
                       if (hook)
                       {
                           hook->pt           = msg->pt;
                           hook->hwnd         = msg->hwnd;
                           hook->wHitTestCode = HIWORD(status);
                           hook->dwExtraInfo  = 0;
                           HOOK_CallHooksA( WH_CBT, HCBT_CLICKSKIPPED ,msg->message,
                                          (LPARAM)hook );
                           free(hook);
                       }
                   }
                }

		if (remove)
		    QUEUE_RemoveMsg( sysMsgQueue, pos );
		/* continue */

	   case SYSQ_MSG_CONTINUE:
		continue;

	   case SYSQ_MSG_ABANDON: 
		return FALSE;
	}

        if (remove)
        {
            if (HOOK_IsHooked( WH_JOURNALRECORD )) MSG_JournalRecordMsg( msg );
            QUEUE_RemoveMsg( sysMsgQueue, pos );
        }
        return TRUE;
    }
    return FALSE;
}

/***********************************************************************
 *           MSG_SendMessage
 *
 * Implementation of an inter-task SendMessage.
 */
LRESULT MSG_SendMessage( HQUEUE hDestQueue, HWND hwnd, UINT msg,
                                WPARAM wParam, LPARAM lParam, WORD flags )
{
    INT	  prevSMRL = debugSMRL;
    QSMCTRL 	  qCtrl = { 0, 1};
    MESSAGEQUEUE *queue, *destQ;

    if (!(queue = (MESSAGEQUEUE*)GlobalLock( GetFastQueue() ))) return 0;
    if (!(destQ = (MESSAGEQUEUE*)GlobalLock( hDestQueue ))) return 0;

    if (
	//IsTaskLocked() || 
	!IsWindow(hwnd)) return 0;

    debugSMRL+=4;
    DPRINT("%*sSM: %s [%04x] (%04x -> %04x)\n", 
		    prevSMRL, "", SPY_GetMsgName(msg), msg, queue->self, hDestQueue );

    if( !(queue->wakeBits & QS_SMPARAMSFREE) )
    {
      DPRINT("\tIntertask SendMessage: sleeping since unreplied SendMessage pending\n");
      QUEUE_WaitBits( QS_SMPARAMSFREE );
    }

    /* resume sending */ 

    queue->hWnd       = hwnd;
    queue->msg        = msg;
    queue->wParam     = LOWORD(wParam);
    queue->wParamHigh = HIWORD(wParam);
    queue->lParam     = lParam;
    queue->hPrevSendingTask = destQ->hSendingTask;
    destQ->hSendingTask = GetFastQueue();

    QUEUE_ClearWakeBit( queue, QS_SMPARAMSFREE );
    queue->flags = (queue->flags & ~(QUEUE_SM_ASCII|QUEUE_SM_UNICODE)) | flags;

    DPRINT("%*ssm: smResultInit = %08x\n", prevSMRL, "", (unsigned)&qCtrl);

    queue->smResultInit = &qCtrl;

    QUEUE_SetWakeBit( destQ, QS_SENDMESSAGE );

    /* perform task switch and wait for the result */

    while( qCtrl.bPending )
    {
      if (!(queue->wakeBits & QS_SMRESULT))
      {
        //if (THREAD_IsWinA( THREAD_Current() )) DirectedYield( destQ->hTask );
        QUEUE_WaitBits( QS_SMRESULT );
	DPRINT("\tsm: have result!\n");
      }
      /* got something */

      DPRINT("%*ssm: smResult = %08x\n", prevSMRL, "", (unsigned)queue->smResult );

      if (queue->smResult) { /* FIXME, smResult should always be set */
        queue->smResult->lResult = queue->SendMessageReturn;
        queue->smResult->bPending = FALSE;
      }
      QUEUE_ClearWakeBit( queue, QS_SMRESULT );

      if( queue->smResult != &qCtrl )
	  DPRINT( "%*ssm: weird scenes inside the goldmine!\n", prevSMRL, "");
    }
    queue->smResultInit = NULL;
    
    DPRINT("%*sSM: [%04x] returning %08lx\n", prevSMRL, "",  msg, qCtrl.lResult);
    debugSMRL-=4;

    return qCtrl.lResult;
}



/***********************************************************************
 *           MSG_PeekMessage
 */
WINBOOL MSG_PeekMessage( LPMSG msg, HWND hwnd, WORD first, WORD last,
                               WORD flags, WINBOOL peek )
{
    int pos, mask;
    MESSAGEQUEUE *msgQueue;
    HQUEUE hQueue;

#ifdef CONFIG_IPC
    DDE_TestDDE(hwnd);	/* do we have dde handling in the window ?*/
    DDE_GetRemoteMessage();
#endif  /* CONFIG_IPC */

    mask = QS_POSTMESSAGE | QS_SENDMESSAGE;  /* Always selected */
    if (first || last)
    {
        if ((first <= WM_KEYLAST) && (last >= WM_KEYFIRST)) mask |= QS_KEY;
        if ( ((first <= WM_MOUSELAST) && (last >= WM_MOUSEFIRST)) ||
             ((first <= WM_NCMOUSELAST) && (last >= WM_NCMOUSEFIRST)) ) mask |= QS_MOUSE;
        if ((first <= WM_TIMER) && (last >= WM_TIMER)) mask |= QS_TIMER;
        if ((first <= WM_SYSTIMER) && (last >= WM_SYSTIMER)) mask |= QS_TIMER;
        if ((first <= WM_PAINT) && (last >= WM_PAINT)) mask |= QS_PAINT;
    }
    else mask |= QS_MOUSE | QS_KEY | QS_TIMER | QS_PAINT;

    //if (IsTaskLocked()) flags |= PM_NOYIELD;

    /* Never yield on Win32 threads */
//    if (!THREAD_IsWinA(THREAD_Current())) 
	flags |= PM_NOYIELD;

    while(1)
    {    
	hQueue   = GetFastQueue();
 //       msgQueue = (MESSAGEQUEUE *)GlobalLock( hQueue );
	msgQueue = (MESSAGEQUEUE *) hQueue;
        if (!msgQueue) return FALSE;
        msgQueue->changeBits = 0;

        /* First handle a message put by SendMessage() */

	while (msgQueue->wakeBits & QS_SENDMESSAGE)
            QUEUE_ReceiveMessage( msgQueue );

        /* Now handle a WM_QUIT message */

        if (msgQueue->wPostQMsg &&
	   (!first || WM_QUIT >= first) && 
	   (!last || WM_QUIT <= last) )
        {
            msg->hwnd    = hwnd;
            msg->message = WM_QUIT;
            msg->wParam  = msgQueue->wExitCode;
            msg->lParam  = 0;
            if (flags & PM_REMOVE) msgQueue->wPostQMsg = 0;
            break;
        }
    
        /* Now find a normal message */

        if (((msgQueue->wakeBits & mask) & QS_POSTMESSAGE) &&
            ((pos = QUEUE_FindMsg( msgQueue, hwnd, first, last )) != -1))
        {
            QMSG *qmsg = &msgQueue->messages[pos];
            *msg = qmsg->msg;
            msgQueue->GetMessageTimeVal      = msg->time;
            msgQueue->GetMessagePosVal       = *(DWORD *)&msg->pt;
            msgQueue->GetMessageExtraInfoVal = qmsg->extraInfo;

            if (flags & PM_REMOVE) QUEUE_RemoveMsg( msgQueue, pos );
            break;
        }

        msgQueue->changeBits |= MSG_JournalPlayBackMsg();

        /* Now find a hardware event */

        if (((msgQueue->wakeBits & mask) & (QS_MOUSE | QS_KEY)) &&
            MSG_PeekHardwareMsg( msg, hwnd, MAKELONG(first,last), flags & PM_REMOVE ))
        {
            /* Got one */
	    msgQueue->GetMessageTimeVal      = msg->time;
	    msgQueue->GetMessagePosVal       = *(DWORD *)&msg->pt;
	    msgQueue->GetMessageExtraInfoVal = 0;  /* Always 0 for now */
            break;
        }

        /* Check again for SendMessage */

 	while (msgQueue->wakeBits & QS_SENDMESSAGE)
            QUEUE_ReceiveMessage( msgQueue );

        /* Now find a WM_PAINT message */

	if ((msgQueue->wakeBits & mask) & QS_PAINT)
	{
	    WND* wndPtr;
	    msg->hwnd = WIN_FindWinToRepaint( hwnd , hQueue );
	    msg->message = WM_PAINT;
	    msg->wParam = 0;
	    msg->lParam = 0;

	    if ((wndPtr = WIN_FindWndPtr(msg->hwnd)))
	    {
                if( wndPtr->dwStyle & WS_MINIMIZE &&
                    wndPtr->class->hIcon )
                {
                    msg->message = WM_PAINTICON;
                    msg->wParam = 1;
                }

                if( !hwnd || msg->hwnd == hwnd || IsChild(hwnd,msg->hwnd) )
                {
                    if( wndPtr->flags & WIN_INTERNAL_PAINT && !wndPtr->hrgnUpdate)
                    {
                        wndPtr->flags &= ~WIN_INTERNAL_PAINT;
                        QUEUE_DecPaintCount( hQueue );
                    }
                    break;
                }
	    }
	}

        /* Check for timer messages, but yield first */

        if (!(flags & PM_NOYIELD))
        {
            UserYield();
            while (msgQueue->wakeBits & QS_SENDMESSAGE)
                QUEUE_ReceiveMessage( msgQueue );
        }
	if ((msgQueue->wakeBits & mask) & QS_TIMER)
	{
	    if (TIMER_GetTimerMsg(msg, hwnd, hQueue, flags & PM_REMOVE)) break;
	}

        if (peek)
        {
            if (!(flags & PM_NOYIELD)) UserYield();
            return FALSE;
        }
        msgQueue->wakeMask = mask;
        QUEUE_WaitBits( mask );
    }

      /* We got a message */
    if (flags & PM_REMOVE)
    {
	WORD message = msg->message;

	if (message == WM_KEYDOWN || message == WM_SYSKEYDOWN)
	{
	    BYTE *p = &QueueKeyStateTable[msg->wParam & 0xff];

	    if (!(*p & 0x80))
		*p ^= 0x01;
	    *p |= 0x80;
	}
	else if (message == WM_KEYUP || message == WM_SYSKEYUP)
	    QueueKeyStateTable[msg->wParam & 0xff] &= ~0x80;
    }
    if (peek) return TRUE;
    else return (msg->message != WM_QUIT);
}

/***********************************************************************
 *           MSG_InternalGetMessage
 *
 * GetMessage() function for internal use. Behave like GetMessage(),
 * but also call message filters and optionally send WM_ENTERIDLE messages.
 * 'hwnd' must be the handle of the dialog or menu window.
 * 'code' is the message filter value (MSGF_??? codes).
 */
WINBOOL MSG_InternalGetMessage( MSG *msg, HWND hwnd, HWND hwndOwner,
                               WPARAM code, WORD flags, WINBOOL sendIdle ) 
{
    for (;;)
    {
	if (sendIdle)
	{
	    if (!MSG_PeekMessage( msg, 0, 0, 0, flags, TRUE ))
	    {
		  /* No message present -> send ENTERIDLE and wait */
                if (IsWindow(hwndOwner))
                    SendMessageW( hwndOwner, WM_ENTERIDLE,
                                   code, (LPARAM)hwnd );
		MSG_PeekMessage( msg, 0, 0, 0, flags, FALSE );
	    }
	}
	else  /* Always wait for a message */
	    MSG_PeekMessage( msg, 0, 0, 0, flags, FALSE );

        /* Call message filters */

        if (HOOK_IsHooked( WH_SYSMSGFILTER ) || HOOK_IsHooked( WH_MSGFILTER ))
        {
            MSG *pmsg = malloc(sizeof(MSG));
            if (pmsg)
            {
                WINBOOL ret;
                *pmsg = *msg;
                ret = ((WINBOOL)HOOK_CallHooksA( WH_SYSMSGFILTER, code, 0,
                                                 (LPARAM)(pmsg) ) ||
                       (WINBOOL)HOOK_CallHooksA( WH_MSGFILTER, code, 0,
                                                 (LPARAM)(pmsg) ));
                free(pmsg);
                if (ret)
                {
                    /* Message filtered -> remove it from the queue */
                    /* if it's still there. */
                    if (!(flags & PM_REMOVE))
                        MSG_PeekMessage( msg, 0, 0, 0, PM_REMOVE, TRUE );
                    continue;
                }
            }
        }

        return (msg->message != WM_QUIT);
    }
}

/************************************************************************
 *	     MSG_CallWndProcHook
 */
void  MSG_CallWndProcHook( LPMSG pmsg, WINBOOL bUnicode )
{
   CWPSTRUCT cwp;

   cwp.lParam = pmsg->lParam;
   cwp.wParam = pmsg->wParam;
   cwp.message = pmsg->message;
   cwp.hwnd = pmsg->hwnd;

   if (bUnicode) HOOK_CallHooksW(WH_CALLWNDPROC, HC_ACTION, 1, (LPARAM)&cwp);
   else HOOK_CallHooksA( WH_CALLWNDPROC, HC_ACTION, 1, (LPARAM)&cwp );

   pmsg->lParam = cwp.lParam;
   pmsg->wParam = cwp.wParam;
   pmsg->message = cwp.message;
   pmsg->hwnd = cwp.hwnd;
}

struct accent_char
{
    BYTE ac_accent;
    BYTE ac_char;
    BYTE ac_result;
};

static const struct accent_char accent_chars[] =
{
/* A good idea should be to read /usr/X11/lib/X11/locale/iso8859-x/Compose */
    {'`', 'A', '\300'},  {'`', 'a', '\340'},
    {'\'', 'A', '\301'}, {'\'', 'a', '\341'},
    {'^', 'A', '\302'},  {'^', 'a', '\342'},
    {'~', 'A', '\303'},  {'~', 'a', '\343'},
    {'"', 'A', '\304'},  {'"', 'a', '\344'},
    {'O', 'A', '\305'},  {'o', 'a', '\345'},
    {'0', 'A', '\305'},  {'0', 'a', '\345'},
    {'A', 'A', '\305'},  {'a', 'a', '\345'},
    {'A', 'E', '\306'},  {'a', 'e', '\346'},
    {',', 'C', '\307'},  {',', 'c', '\347'},
    {'`', 'E', '\310'},  {'`', 'e', '\350'},
    {'\'', 'E', '\311'}, {'\'', 'e', '\351'},
    {'^', 'E', '\312'},  {'^', 'e', '\352'},
    {'"', 'E', '\313'},  {'"', 'e', '\353'},
    {'`', 'I', '\314'},  {'`', 'i', '\354'},
    {'\'', 'I', '\315'}, {'\'', 'i', '\355'},
    {'^', 'I', '\316'},  {'^', 'i', '\356'},
    {'"', 'I', '\317'},  {'"', 'i', '\357'},
    {'-', 'D', '\320'},  {'-', 'd', '\360'},
    {'~', 'N', '\321'},  {'~', 'n', '\361'},
    {'`', 'O', '\322'},  {'`', 'o', '\362'},
    {'\'', 'O', '\323'}, {'\'', 'o', '\363'},
    {'^', 'O', '\324'},  {'^', 'o', '\364'},
    {'~', 'O', '\325'},  {'~', 'o', '\365'},
    {'"', 'O', '\326'},  {'"', 'o', '\366'},
    {'/', 'O', '\330'},  {'/', 'o', '\370'},
    {'`', 'U', '\331'},  {'`', 'u', '\371'},
    {'\'', 'U', '\332'}, {'\'', 'u', '\372'},
    {'^', 'U', '\333'},  {'^', 'u', '\373'},
    {'"', 'U', '\334'},  {'"', 'u', '\374'},
    {'\'', 'Y', '\335'}, {'\'', 'y', '\375'},
    {'T', 'H', '\336'},  {'t', 'h', '\376'},
    {'s', 's', '\337'},  {'"', 'y', '\377'},
    {'s', 'z', '\337'},  {'i', 'j', '\377'},
	/* iso-8859-2 uses this */
    {'<', 'L', '\245'},  {'<', 'l', '\265'},	/* caron */
    {'<', 'S', '\251'},  {'<', 's', '\271'},
    {'<', 'T', '\253'},  {'<', 't', '\273'},
    {'<', 'Z', '\256'},  {'<', 'z', '\276'},
    {'<', 'C', '\310'},  {'<', 'c', '\350'},
    {'<', 'E', '\314'},  {'<', 'e', '\354'},
    {'<', 'D', '\317'},  {'<', 'd', '\357'},
    {'<', 'N', '\322'},  {'<', 'n', '\362'},
    {'<', 'R', '\330'},  {'<', 'r', '\370'},
    {';', 'A', '\241'},  {';', 'a', '\261'},	/* ogonek */
    {';', 'E', '\312'},  {';', 'e', '\332'},
    {'\'', 'Z', '\254'}, {'\'', 'z', '\274'},	/* acute */
    {'\'', 'R', '\300'}, {'\'', 'r', '\340'},
    {'\'', 'L', '\305'}, {'\'', 'l', '\345'},
    {'\'', 'C', '\306'}, {'\'', 'c', '\346'},
    {'\'', 'N', '\321'}, {'\'', 'n', '\361'},
/*  collision whith S, from iso-8859-9 !!! */
    {',', 'S', '\252'},  {',', 's', '\272'},	/* cedilla */
    {',', 'T', '\336'},  {',', 't', '\376'},
    {'.', 'Z', '\257'},  {'.', 'z', '\277'},	/* dot above */
    {'/', 'L', '\243'},  {'/', 'l', '\263'},	/* slash */
    {'/', 'D', '\320'},  {'/', 'd', '\360'},
    {'(', 'A', '\303'},  {'(', 'a', '\343'},	/* breve */
    {'\275', 'O', '\325'}, {'\275', 'o', '\365'},	/* double acute */
    {'\275', 'U', '\334'}, {'\275', 'u', '\374'},
    {'0', 'U', '\332'},  {'0', 'u', '\372'},	/* ring above */
	/* iso-8859-3 uses this */
    {'/', 'H', '\241'},  {'/', 'h', '\261'},	/* slash */
    {'>', 'H', '\246'},  {'>', 'h', '\266'},	/* circumflex */
    {'>', 'J', '\254'},  {'>', 'j', '\274'},
    {'>', 'C', '\306'},  {'>', 'c', '\346'},
    {'>', 'G', '\330'},  {'>', 'g', '\370'},
    {'>', 'S', '\336'},  {'>', 's', '\376'},
/*  collision whith G( from iso-8859-9 !!!   */
    {'(', 'G', '\253'},  {'(', 'g', '\273'},	/* breve */
    {'(', 'U', '\335'},  {'(', 'u', '\375'},
/*  collision whith I. from iso-8859-3 !!!   */
    {'.', 'I', '\251'},  {'.', 'i', '\271'},	/* dot above */
    {'.', 'C', '\305'},  {'.', 'c', '\345'},
    {'.', 'G', '\325'},  {'.', 'g', '\365'},
	/* iso-8859-4 uses this */
    {',', 'R', '\243'},  {',', 'r', '\263'},	/* cedilla */
    {',', 'L', '\246'},  {',', 'l', '\266'},
    {',', 'G', '\253'},  {',', 'g', '\273'},
    {',', 'N', '\321'},  {',', 'n', '\361'},
    {',', 'K', '\323'},  {',', 'k', '\363'},
    {'~', 'I', '\245'},  {'~', 'i', '\265'},	/* tilde */
    {'-', 'E', '\252'},  {'-', 'e', '\272'},	/* macron */
    {'-', 'A', '\300'},  {'-', 'a', '\340'},
    {'-', 'I', '\317'},  {'-', 'i', '\357'},
    {'-', 'O', '\322'},  {'-', 'o', '\362'},
    {'-', 'U', '\336'},  {'-', 'u', '\376'},
    {'/', 'T', '\254'},  {'/', 't', '\274'},	/* slash */
    {'.', 'E', '\314'},  {'.', 'e', '\344'},	/* dot above */
    {';', 'I', '\307'},  {';', 'i', '\347'},	/* ogonek */
    {';', 'U', '\331'},  {';', 'u', '\371'},
	/* iso-8859-9 uses this */
	/* iso-8859-9 has really bad choosen G( S, and I. as they collide
	 * whith the same letters on other iso-8859-x (that is they are on
	 * different places :-( ), if you use turkish uncomment these and
	 * comment out the lines in iso-8859-2 and iso-8859-3 sections
	 * FIXME: should be dynamic according to chosen language
	 *	  if/when Wine has turkish support.  
	 */ 
/*  collision whith G( from iso-8859-3 !!!   */
/*  {'(', 'G', '\320'},  {'(', 'g', '\360'}, */	/* breve */
/*  collision whith S, from iso-8859-2 !!! */
/*  {',', 'S', '\336'},  {',', 's', '\376'}, */	/* cedilla */
/*  collision whith I. from iso-8859-3 !!!   */
/*  {'.', 'I', '\335'},  {'.', 'i', '\375'}, */	/* dot above */
};


/***********************************************************************
 *           MSG_DoTranslateMessage
 *
 * Implementation of TranslateMessage.
 *
 * TranslateMessage translates virtual-key messages into character-messages,
 * as follows :
 * WM_KEYDOWN/WM_KEYUP combinations produce a WM_CHAR or WM_DEADCHAR message.
 * ditto replacing WM_* with WM_SYS*
 * This produces WM_CHAR messages only for keys mapped to ASCII characters
 * by the keyboard driver.
 */
WINBOOL MSG_DoTranslateMessage( UINT message, HWND hwnd,
                                      WPARAM wParam, LPARAM lParam )
{
    int dead_char = 0;
    BYTE wp[2];
    
    if (message != WM_MOUSEMOVE && message != WM_TIMER)
        DPRINT( "(%s, %04X, %08lX)\n",
		     SPY_GetMsgName(message), wParam, lParam );
    if(message >= WM_KEYFIRST && message <= WM_KEYLAST)
       DPRINT( "(%s, %04X, %08lX)\n",
		     SPY_GetMsgName(message), wParam, lParam );

    if ((message != WM_KEYDOWN) && (message != WM_SYSKEYDOWN))	return FALSE;

   DPRINT( "Translating key %04X, scancode %04X\n",
                 wParam, HIWORD(lParam) );

    /* FIXME : should handle ToAscii yielding 2 */
    switch (ToAscii(wParam, HIWORD(lParam),
                      QueueKeyStateTable,(LPWORD)wp, 0)) 
    {
    case 1 :
        message = (message == WM_KEYDOWN) ? WM_CHAR : WM_SYSCHAR;
        /* Should dead chars handling go in ToAscii ? */
        if (dead_char)
        {
            int i;

            if (wp[0] == ' ') wp[0] =  dead_char;
            if (dead_char == 0xa2) dead_char = '(';
            else if (dead_char == 0xa8) dead_char = '"';
	    else if (dead_char == 0xb2) dead_char = ';';
            else if (dead_char == 0xb4) dead_char = '\'';
            else if (dead_char == 0xb7) dead_char = '<';
            else if (dead_char == 0xb8) dead_char = ',';
            else if (dead_char == 0xff) dead_char = '.';
            for (i = 0; i < sizeof(accent_chars)/sizeof(accent_chars[0]); i++)
                if ((accent_chars[i].ac_accent == dead_char) &&
                    (accent_chars[i].ac_char == wp[0]))
                {
                    wp[0] = accent_chars[i].ac_result;
                    break;
                }
            dead_char = 0;
        }
       DPRINT( "1 -> PostMessage(%s)\n", SPY_GetMsgName(message));
        PostMessageA( hwnd, message, wp[0], lParam );
        return TRUE;

    case -1 :
        message = (message == WM_KEYDOWN) ? WM_DEADCHAR : WM_SYSDEADCHAR;
        dead_char = wp[0];
       DPRINT( "-1 -> PostMessage(%s)\n",
                     SPY_GetMsgName(message));
        PostMessageA( hwnd, message, wp[0], lParam );
        return TRUE;
    }
    return FALSE;
}


HTASK GetCurrentTask(void)
{
    return (HTASK)-2;
}

//FIXME
int init = 0;
MESSAGEQUEUE Queue;
HQUEUE hThreadQ = &Queue;;

HQUEUE  GetThreadQueue( DWORD thread )
{
	if ( init == 0 ) {
		init = 1;
		memset(&Queue,0,sizeof(MESSAGEQUEUE));
	}
	return hThreadQ;
}



HQUEUE  SetThreadQueue( DWORD thread, HQUEUE hQueue )
{
    HQUEUE oldQueue = hThreadQ;
    hThreadQ = hQueue;
    return oldQueue;
}

HANDLE  GetFastQueue( void )
{
    return hThreadQ;
}

int UserYield(void)
{
	return SuspendThread(GetCurrentThread());
}
