/*
 * Message queues related functions
 *
 * Copyright 1993, 1994 Alexandre Julliard
 */

#include <windows.h>
#include <user32/win.h>
#include <user32/queue.h>
#include <user32/debug.h>


HWND GetSysModalWindow(void);

#define MAX_QUEUE_SIZE   120  /* Max. size of a message queue */

static HQUEUE hFirstQueue = 0;
static HQUEUE hExitingQueue = 0;
static HQUEUE hmemSysMsgQueue = 0;
static MESSAGEQUEUE *sysMsgQueue = NULL;

static MESSAGEQUEUE *pMouseQueue = NULL;  /* Queue for last mouse message */
static MESSAGEQUEUE *pKbdQueue = NULL;    /* Queue for last kbd message */

MESSAGEQUEUE *pCursorQueue = NULL; 
MESSAGEQUEUE *pActiveQueue = NULL;

/***********************************************************************
 *	     QUEUE_DumpQueue
 */
void QUEUE_DumpQueue( HQUEUE hQueue )
{
    MESSAGEQUEUE *pq; 

    if (!(pq = (MESSAGEQUEUE*) GlobalLock( hQueue )) ||
        GlobalSize(hQueue) < sizeof(MESSAGEQUEUE)+pq->queueSize*sizeof(QMSG))
    {
        DPRINT( "%04x is not a queue handle\n", hQueue );
        return;
    }

    DPRINT(    "next: %12.4x  Intertask SendMessage:\n"
             "hTask: %11.4x  ----------------------\n"
             "msgSize: %9.4x  hWnd: %10.4x\n"
             "msgCount: %8.4x  msg: %11.4x\n"
             "msgNext: %9.4x  wParam: %8.4x\n"
             "msgFree: %9.4x  lParam: %8.8x\n"
             "qSize: %11.4x  lRet: %10.8x\n"
             "wWinVer: %9.4x  ISMH: %10.4x\n"
             "paints: %10.4x  hSendTask: %5.4x\n"
             "timers: %10.4x  hPrevSend: %5.4x\n"
             "wakeBits: %8.4x\n"
             "wakeMask: %8.4x\n"
             "hCurHook: %8.4x\n",
             pq->next, pq->hTask, pq->msgSize, pq->hWnd, 
             pq->msgCount, pq->msg, pq->nextMessage, pq->wParam,
             pq->nextFreeMessage, (unsigned)pq->lParam, pq->queueSize,
             (unsigned)pq->SendMessageReturn, pq->wWinVersion, pq->InSendMessageHandle,
             pq->wPaintCount, pq->hSendingTask, pq->wTimerCount,
             pq->hPrevSendingTask, pq->wakeBits, pq->wakeMask, pq->hCurHook);
}


/***********************************************************************
 *	     QUEUE_WalkQueues
 */
void QUEUE_WalkQueues(void)
{
    char module[10];
    HQUEUE hQueue = hFirstQueue;

    DPRINT( "Queue Size Msgs Task\n" );
    while (hQueue)
    {
        MESSAGEQUEUE *queue = (MESSAGEQUEUE *)GlobalLock( hQueue );
        if (!queue)
        {
            DPRINT( "Bad queue handle %04x\n", hQueue );
            return;
        }
//        if (!GetModuleFileName( queue->hTask, module, sizeof(module )))
//            strcpy( module, "???" );
//        DPRINT( "%04x %5d %4d %04x %s\n", hQueue, queue->msgSize,
//                 queue->msgCount, queue->hTask, module );
        hQueue = queue->next;
    }
    DPRINT( "\n" );
}


/***********************************************************************
 *	     QUEUE_IsExitingQueue
 */
WINBOOL QUEUE_IsExitingQueue( HQUEUE hQueue )
{
    return (hExitingQueue && (hQueue == hExitingQueue));
}


/***********************************************************************
 *	     QUEUE_SetExitingQueue
 */
void QUEUE_SetExitingQueue( HQUEUE hQueue )
{
    hExitingQueue = hQueue;
}


/***********************************************************************
 *           QUEUE_CreateMsgQueue
 *
 * Creates a message queue. Doesn't link it into queue list!
 */
HQUEUE QUEUE_CreateMsgQueue( int size )
{
    HQUEUE hQueue;
    MESSAGEQUEUE * msgQueue;
    int queueSize;
    //TDB *pTask = (TDB *)GlobalLock( GetCurrentTask() );

    DPRINT("Creating message queue...\n");

    queueSize = sizeof(MESSAGEQUEUE) + size * sizeof(QMSG);
    if (!(hQueue = GlobalAlloc( GMEM_FIXED | GMEM_ZEROINIT, queueSize )))
        return 0;
    msgQueue = (MESSAGEQUEUE *) GlobalLock( hQueue );
    msgQueue->self        = hQueue;
    msgQueue->msgSize     = sizeof(QMSG);
    msgQueue->queueSize   = size;
    msgQueue->wakeBits    = msgQueue->changeBits = QS_SMPARAMSFREE;
    //msgQueue->wWinVersion = pTask ? pTask->version : 0;
    GlobalUnlock( hQueue );
    return hQueue;
}


/***********************************************************************
 *	     QUEUE_DeleteMsgQueue
 *
 * Unlinks and deletes a message queue.
 *
 * Note: We need to mask asynchronous events to make sure PostMessage works
 * even in the signal handler.
 */
WINBOOL QUEUE_DeleteMsgQueue( HQUEUE hQueue )
{
    MESSAGEQUEUE * msgQueue = (MESSAGEQUEUE*)GlobalLock(hQueue);
    HQUEUE  senderQ;
    HQUEUE *pPrev;

    DPRINT("Deleting message queue %04x\n", hQueue);

    if (!hQueue || !msgQueue)
    {
	DPRINT( "invalid argument.\n");
	return 0;
    }
    if( pCursorQueue == msgQueue ) pCursorQueue = NULL;
    if( pActiveQueue == msgQueue ) pActiveQueue = NULL;

    /* flush sent messages */
    senderQ = msgQueue->hSendingTask;
    while( senderQ )
    {
      MESSAGEQUEUE* sq = (MESSAGEQUEUE*)GlobalLock(senderQ);
      if( !sq ) break;
      sq->SendMessageReturn = 0L;
      QUEUE_SetWakeBit( sq, QS_SMRESULT );
      senderQ = sq->hPrevSendingTask;
    }

    SIGNAL_MaskAsyncEvents( TRUE );

    pPrev = &hFirstQueue;
    while (*pPrev && (*pPrev != hQueue))
    {
        MESSAGEQUEUE *msgQ = (MESSAGEQUEUE*)GlobalLock(*pPrev);
        pPrev = &msgQ->next;
    }
    if (*pPrev) *pPrev = msgQueue->next;
    msgQueue->self = 0;

    SIGNAL_MaskAsyncEvents( FALSE );

    GlobalFree( hQueue );
    return 1;
}


/***********************************************************************
 *           QUEUE_CreateSysMsgQueue
 *
 * Create the system message queue, and set the double-click speed.
 * Must be called only once.
 */
WINBOOL QUEUE_CreateSysMsgQueue( int size )
{
    if (size > MAX_QUEUE_SIZE) size = MAX_QUEUE_SIZE;
    else if (size <= 0) size = 1;
    if (!(hmemSysMsgQueue = QUEUE_CreateMsgQueue( size ))) return FALSE;
    sysMsgQueue = (MESSAGEQUEUE *) GlobalLock( hmemSysMsgQueue );
    return TRUE;
}


/***********************************************************************
 *           QUEUE_GetSysQueue
 */
MESSAGEQUEUE *QUEUE_GetSysQueue(void)
{
    return sysMsgQueue;
}

/***********************************************************************
 *           QUEUE_Signal
 */

void QUEUE_Signal( HTASK hTask )
{
#if 0
    PDB32 *pdb;
    THREAD_ENTRY *entry;

    TDB *pTask = (TDB *)GlobalLock( hTask );
    if ( !pTask ) return;

    /* Wake up thread waiting for message */
    SetEvent( pTask->thdb->event );

    PostEvent( hTask );
#endif
}

/***********************************************************************
 *           QUEUE_Wait
 */
static void QUEUE_Wait( DWORD wait_mask )
{
#if 0
    if ( THREAD_IsWin16( THREAD_Current() ) )
        WaitEvent( 0 );
    else
    {
        DPRINT( "current task is 32-bit, calling SYNC_DoWait\n");
        MsgWaitForMultipleObjects( 0, NULL, FALSE, INFINITE32, wait_mask );
    }
#endif
}


/***********************************************************************
 *           QUEUE_SetWakeBit
 *
 * See "Windows Internals", p.449
 */
void QUEUE_SetWakeBit( MESSAGEQUEUE *queue, WORD bit )
{
    DPRINT("queue = %04x (wm=%04x), bit = %04x\n", 
	                queue->self, queue->wakeMask, bit );

    if (bit & QS_MOUSE) pMouseQueue = queue;
    if (bit & QS_KEY) pKbdQueue = queue;
    queue->changeBits |= bit;
    queue->wakeBits   |= bit;
    if (queue->wakeMask & bit)
    {
        queue->wakeMask = 0;
        QUEUE_Signal( queue->hTask );
    }
}


/***********************************************************************
 *           QUEUE_ClearWakeBit
 */
void QUEUE_ClearWakeBit( MESSAGEQUEUE *queue, WORD bit )
{
    queue->changeBits &= ~bit;
    queue->wakeBits   &= ~bit;
}


/***********************************************************************
 *           QUEUE_WaitBits
 *
 * See "Windows Internals", p.447
 */
void QUEUE_WaitBits( WORD bits )
{
    MESSAGEQUEUE *queue;

    DPRINT("q %04x waiting for %04x\n", GetFastQueue(), bits);

    for (;;)
    {
        if (!(queue = (MESSAGEQUEUE *)GlobalLock( GetFastQueue() ))) return;

        if (queue->changeBits & bits)
        {
            /* One of the bits is set; we can return */
            queue->wakeMask = 0;
            return;
        }
        if (queue->wakeBits & QS_SENDMESSAGE)
        {
            /* Process the sent message immediately */

	    queue->wakeMask = 0;
            QUEUE_ReceiveMessage( queue );
	    continue;				/* nested sm crux */
        }

        queue->wakeMask = bits | QS_SENDMESSAGE;
	if(queue->changeBits & bits) continue;
	
	DPRINT("%04x) wakeMask is %04x, waiting\n", queue->self, queue->wakeMask);

        QUEUE_Wait( queue->wakeMask );
    }
}


/***********************************************************************
 *           QUEUE_ReceiveMessage
 *
 * This routine is called when a sent message is waiting for the queue.
 */
void QUEUE_ReceiveMessage( MESSAGEQUEUE *queue )
{
    MESSAGEQUEUE *senderQ = NULL;
    HQUEUE      prevSender = 0;
    QSMCTRL*      prevCtrlPtr = NULL;
    LRESULT       result = 0;

    DPRINT( "ReceiveMessage, queue %04x\n", queue->self );
    if (!(queue->wakeBits & QS_SENDMESSAGE) ||
        !(senderQ = (MESSAGEQUEUE*)GlobalLock( queue->hSendingTask))) 
	{ DPRINT("\trcm: nothing to do\n"); return; }

    if( !senderQ->hPrevSendingTask )
        QUEUE_ClearWakeBit( queue, QS_SENDMESSAGE );   /* no more sent messages */

    /* Save current state on stack */
    prevSender                 = queue->InSendMessageHandle;
    prevCtrlPtr		       = queue->smResultCurrent;

    /* Remove sending queue from the list */
    queue->InSendMessageHandle = queue->hSendingTask;
    queue->smResultCurrent     = senderQ->smResultInit;
    queue->hSendingTask	       = senderQ->hPrevSendingTask;

    DPRINT( "\trcm: smResultCurrent = %08x, prevCtrl = %08x\n", 
				(unsigned)queue->smResultCurrent, (unsigned)prevCtrlPtr );
    QUEUE_SetWakeBit( senderQ, QS_SMPARAMSFREE );

    DPRINT( "\trcm: calling wndproc - %04x %04x %04x%04x %08x\n",
                senderQ->hWnd, senderQ->msg, senderQ->wParamHigh,
                senderQ->wParam, (unsigned)senderQ->lParam );

    if (IsWindow( senderQ->hWnd ))
    {
        WND *wndPtr = WIN_FindWndPtr( senderQ->hWnd );
        DWORD extraInfo = queue->GetMessageExtraInfoVal;
        queue->GetMessageExtraInfoVal = senderQ->GetMessageExtraInfoVal;

        if (senderQ->flags & QUEUE_SM_ASCII)
        {
            WPARAM wParam = MAKELONG( senderQ->wParam, senderQ->wParamHigh );
            DPRINT( "\trcm: msg is Win32\n" );
            if (senderQ->flags & QUEUE_SM_UNICODE)
                result = CallWindowProcW( wndPtr->winproc,
                                            senderQ->hWnd, senderQ->msg,
                                            wParam, senderQ->lParam );
            else
                result = CallWindowProcA( wndPtr->winproc,
                                            senderQ->hWnd, senderQ->msg,
                                            wParam, senderQ->lParam );
        }
        else  /* Win16 message */
            result = CallWindowProc( (WNDPROC)wndPtr->winproc,
                                       senderQ->hWnd, senderQ->msg,
                                       senderQ->wParam, senderQ->lParam );

        queue->GetMessageExtraInfoVal = extraInfo;  /* Restore extra info */
	DPRINT("\trcm: result =  %08x\n", (unsigned)result );
    }
    else DPRINT( "\trcm: bad hWnd\n");

    /* Return the result to the sender task */
    ReplyMessage( result );

    queue->InSendMessageHandle = prevSender;
    queue->smResultCurrent     = prevCtrlPtr;

    DPRINT("done!\n");
}

/***********************************************************************
 *           QUEUE_FlushMessage
 * 
 * Try to reply to all pending sent messages on exit.
 */
void QUEUE_FlushMessages( HQUEUE hQueue )
{
  MESSAGEQUEUE *queue = (MESSAGEQUEUE*)GlobalLock( hQueue );

  if( queue )
  {
    MESSAGEQUEUE *senderQ = (MESSAGEQUEUE*)GlobalLock( queue->hSendingTask);
    QSMCTRL*      CtrlPtr = queue->smResultCurrent;

    DPRINT("Flushing queue %04x:\n", hQueue );

    while( senderQ )
    {
      if( !CtrlPtr )
	   CtrlPtr = senderQ->smResultInit;

      DPRINT("\tfrom queue %04x, smResult %08x\n", queue->hSendingTask, (unsigned)CtrlPtr );

      if( !(queue->hSendingTask = senderQ->hPrevSendingTask) )
        QUEUE_ClearWakeBit( queue, QS_SENDMESSAGE );

      QUEUE_SetWakeBit( senderQ, QS_SMPARAMSFREE );
      
      queue->smResultCurrent = CtrlPtr;
//      while( senderQ->wakeBits & QS_SMRESULT ) OldYield();

      while( senderQ->wakeBits & QS_SMRESULT ) Sleep(100);

      senderQ->SendMessageReturn = 0;
      senderQ->smResult = queue->smResultCurrent;
      QUEUE_SetWakeBit( senderQ, QS_SMRESULT);

      senderQ = (MESSAGEQUEUE*)GlobalLock( queue->hSendingTask);
      CtrlPtr = NULL;
    }
    queue->InSendMessageHandle = 0;
  }  
}

/***********************************************************************
 *           QUEUE_AddMsg
 *
 * Add a message to the queue. Return FALSE if queue is full.
 */
WINBOOL QUEUE_AddMsg( HQUEUE hQueue, MSG * msg, DWORD extraInfo )
{
    int pos;
    MESSAGEQUEUE *msgQueue;

    SIGNAL_MaskAsyncEvents( TRUE );

    if (!(msgQueue = (MESSAGEQUEUE *)GlobalLock( hQueue ))) return FALSE;
    pos = msgQueue->nextFreeMessage;

      /* Check if queue is full */
    if ((pos == msgQueue->nextMessage) && (msgQueue->msgCount > 0))
    {
	SIGNAL_MaskAsyncEvents( FALSE );
        DPRINT("Queue is full!\n" );
        return FALSE;
    }

      /* Store message */
    msgQueue->messages[pos].msg = *msg;
    msgQueue->messages[pos].extraInfo = extraInfo;
    if (pos < msgQueue->queueSize-1) pos++;
    else pos = 0;
    msgQueue->nextFreeMessage = pos;
    msgQueue->msgCount++;

    SIGNAL_MaskAsyncEvents( FALSE );

    QUEUE_SetWakeBit( msgQueue, QS_POSTMESSAGE );
    return TRUE;
}


/***********************************************************************
 *           QUEUE_FindMsg
 *
 * Find a message matching the given parameters. Return -1 if none available.
 */
int QUEUE_FindMsg( MESSAGEQUEUE * msgQueue, HWND hwnd, int first, int last )
{
    int i, pos = msgQueue->nextMessage;

    DPRINT("hwnd=%04x pos=%d\n", hwnd, pos );

    if (!msgQueue->msgCount) return -1;
    if (!hwnd && !first && !last) return pos;
        
    for (i = 0; i < msgQueue->msgCount; i++)
    {
	MSG * msg = &msgQueue->messages[pos].msg;

	if (!hwnd || (msg->hwnd == hwnd))
	{
	    if (!first && !last) return pos;
	    if ((msg->message >= first) && (msg->message <= last)) return pos;
	}
	if (pos < msgQueue->queueSize-1) pos++;
	else pos = 0;
    }
    return -1;
}


/***********************************************************************
 *           QUEUE_RemoveMsg
 *
 * Remove a message from the queue (pos must be a valid position).
 */
void QUEUE_RemoveMsg( MESSAGEQUEUE * msgQueue, int pos )
{
    SIGNAL_MaskAsyncEvents( TRUE );

    if (pos >= msgQueue->nextMessage)
    {
	for ( ; pos > msgQueue->nextMessage; pos--)
	    msgQueue->messages[pos] = msgQueue->messages[pos-1];
	msgQueue->nextMessage++;
	if (msgQueue->nextMessage >= msgQueue->queueSize)
	    msgQueue->nextMessage = 0;
    }
    else
    {
	for ( ; pos < msgQueue->nextFreeMessage; pos++)
	    msgQueue->messages[pos] = msgQueue->messages[pos+1];
	if (msgQueue->nextFreeMessage) msgQueue->nextFreeMessage--;
	else msgQueue->nextFreeMessage = msgQueue->queueSize-1;
    }
    msgQueue->msgCount--;
    if (!msgQueue->msgCount) msgQueue->wakeBits &= ~QS_POSTMESSAGE;

    SIGNAL_MaskAsyncEvents( FALSE );
}


/***********************************************************************
 *           QUEUE_WakeSomeone
 *
 * Wake a queue upon reception of a hardware event.
 */
static void QUEUE_WakeSomeone( UINT message )
{
    WND*	  wndPtr = NULL;
    WORD          wakeBit;
    HWND hwnd;
    MESSAGEQUEUE *queue = pCursorQueue;

    if( (message >= WM_KEYFIRST) && (message <= WM_KEYLAST) )
    {
       wakeBit = QS_KEY;
       if( pActiveQueue ) queue = pActiveQueue;
    }
    else 
    {
       wakeBit = (message == WM_MOUSEMOVE) ? QS_MOUSEMOVE : QS_MOUSEBUTTON;
       if( (hwnd = GetCapture()) )
	 if( (wndPtr = WIN_FindWndPtr( hwnd )) ) 
	   queue = (MESSAGEQUEUE *)GlobalLock( wndPtr->hmemTaskQ );
    }

    if( (hwnd = GetSysModalWindow()) )
      if( (wndPtr = WIN_FindWndPtr( hwnd )) )
        queue = (MESSAGEQUEUE *)GlobalLock( wndPtr->hmemTaskQ );

    if( !queue ) 
    {
      queue = GlobalLock( hFirstQueue );
      while( queue )
      {
        if (queue->wakeMask & wakeBit) break;
        queue = GlobalLock( queue->next );
      }
      if( !queue )
      { 
        DPRINT( "couldn't find queue\n"); 
        return; 
      }
    }

    QUEUE_SetWakeBit( queue, wakeBit );
}


/***********************************************************************
 *           hardware_event
 *
 * Add an event to the system message queue.
 * Note: the position is relative to the desktop window.
 */
void hardware_event( WORD message, WORD wParam, LONG lParam,
		     int xPos, int yPos, DWORD time, DWORD extraInfo )
{
    MSG *msg;
    int pos;

    if (!sysMsgQueue) return;
    pos = sysMsgQueue->nextFreeMessage;

      /* Merge with previous event if possible */

    if ((message == WM_MOUSEMOVE) && sysMsgQueue->msgCount)
    {
        if (pos > 0) pos--;
        else pos = sysMsgQueue->queueSize - 1;
	msg = &sysMsgQueue->messages[pos].msg;
	if ((msg->message == message) && (msg->wParam == wParam))
            sysMsgQueue->msgCount--;  /* Merge events */
        else
            pos = sysMsgQueue->nextFreeMessage;  /* Don't merge */
    }

      /* Check if queue is full */

    if ((pos == sysMsgQueue->nextMessage) && sysMsgQueue->msgCount)
    {
        /* Queue is full, beep (but not on every mouse motion...) */
        if (message != WM_MOUSEMOVE) MessageBeep(0);
        return;
    }

      /* Store message */

    msg = &sysMsgQueue->messages[pos].msg;
    msg->hwnd    = 0;
    msg->message = message;
    msg->wParam  = wParam;
    msg->lParam  = lParam;
    msg->time    = time;
    msg->pt.x    = xPos & 0xffff;
    msg->pt.y    = yPos & 0xffff;
    sysMsgQueue->messages[pos].extraInfo = extraInfo;
    if (pos < sysMsgQueue->queueSize - 1) pos++;
    else pos = 0;
    sysMsgQueue->nextFreeMessage = pos;
    sysMsgQueue->msgCount++;
    QUEUE_WakeSomeone( message );
}

		    
/***********************************************************************
 *	     QUEUE_GetQueueTask
 */
HTASK QUEUE_GetQueueTask( HQUEUE hQueue )
{
    MESSAGEQUEUE *queue = GlobalLock( hQueue );
    return (queue) ? queue->hTask : 0 ;
}


/***********************************************************************
 *           QUEUE_IncPaintCount
 */
void QUEUE_IncPaintCount( HQUEUE hQueue )
{
    MESSAGEQUEUE *queue;

    if (!(queue = (MESSAGEQUEUE *)GlobalLock( hQueue ))) return;
    queue->wPaintCount++;
    QUEUE_SetWakeBit( queue, QS_PAINT );
}


/***********************************************************************
 *           QUEUE_DecPaintCount
 */
void QUEUE_DecPaintCount( HQUEUE hQueue )
{
    MESSAGEQUEUE *queue;

    if (!(queue = (MESSAGEQUEUE *)GlobalLock( hQueue ))) return;
    queue->wPaintCount--;
    if (!queue->wPaintCount) queue->wakeBits &= ~QS_PAINT;
}


/***********************************************************************
 *           QUEUE_IncTimerCount
 */
void QUEUE_IncTimerCount( HQUEUE hQueue )
{
    MESSAGEQUEUE *queue;

    if (!(queue = (MESSAGEQUEUE *)GlobalLock( hQueue ))) return;
    queue->wTimerCount++;
    QUEUE_SetWakeBit( queue, QS_TIMER );
}


/***********************************************************************
 *           QUEUE_DecTimerCount
 */
void QUEUE_DecTimerCount( HQUEUE hQueue )
{
    MESSAGEQUEUE *queue;

    if (!(queue = (MESSAGEQUEUE *)GlobalLock( hQueue ))) return;
    queue->wTimerCount--;
    if (!queue->wTimerCount) queue->wakeBits &= ~QS_TIMER;
}


