/*
 * Timer functions
 *
 * Copyright 1993 Alexandre Julliard
 */

#include <windows.h>
#include <user32/timer.h>
#include <user32/debug.h>




static TIMER TimersArray[NB_TIMERS];

static TIMER * pNextTimer = NULL;  /* Next timer to expire */

  /* Duration from 'time' until expiration of the timer */
#define EXPIRE_TIME(pTimer,time) \
          (((pTimer)->expires <= (time)) ? 0 : (pTimer)->expires - (time))


/***********************************************************************
 *           TIMER_InsertTimer
 *
 * Insert the timer at its place in the chain.
 */
void TIMER_InsertTimer( TIMER * pTimer )
{
    if (!pNextTimer || (pTimer->expires < pNextTimer->expires))
    {
	pTimer->next = pNextTimer;
	pNextTimer = pTimer;
    }
    else
    {
        TIMER * ptr = pNextTimer;	
	while (ptr->next && (pTimer->expires >= ptr->next->expires))
	    ptr = ptr->next;
	pTimer->next = ptr->next;
	ptr->next = pTimer;
    }
}


/***********************************************************************
 *           TIMER_RemoveTimer
 *
 * Remove the timer from the chain.
 */
void TIMER_RemoveTimer( TIMER * pTimer )
{
    TIMER **ppTimer = &pNextTimer;

    while (*ppTimer && (*ppTimer != pTimer)) ppTimer = &(*ppTimer)->next;
    if (*ppTimer) *ppTimer = pTimer->next;
    pTimer->next = NULL;
    if (!pTimer->expires) QUEUE_DecTimerCount( pTimer->hq );
}


/***********************************************************************
 *           TIMER_ClearTimer
 *
 * Clear and remove a timer.
 */
void TIMER_ClearTimer( TIMER * pTimer )
{
    TIMER_RemoveTimer( pTimer );
    pTimer->hwnd    = 0;
    pTimer->msg     = 0;
    pTimer->id      = 0;
    pTimer->timeout = 0;
    //WINPROC_FreeProc( pTimer->proc, WIN_PROC_TIMER );
}


/***********************************************************************
 *           TIMER_SwitchQueue
 */
void TIMER_SwitchQueue( HQUEUE old, HQUEUE newQ )
{
    TIMER * pT = pNextTimer;

    while (pT)
    {
        if (pT->hq == old) pT->hq = newQ;
        pT = pT->next;
    }
}


/***********************************************************************
 *           TIMER_RemoveWindowTimers
 *
 * Remove all timers for a given window.
 */
void TIMER_RemoveWindowTimers( HWND hwnd )
{
    int i;
    TIMER *pTimer;

    for (i = NB_TIMERS, pTimer = TimersArray; i > 0; i--, pTimer++)
	if ((pTimer->hwnd == hwnd) && pTimer->timeout)
            TIMER_ClearTimer( pTimer );
}


/***********************************************************************
 *           TIMER_RemoveQueueTimers
 *
 * Remove all timers for a given queue.
 */
void TIMER_RemoveQueueTimers( HQUEUE hqueue )
{
    int i;
    TIMER *pTimer;

    for (i = NB_TIMERS, pTimer = TimersArray; i > 0; i--, pTimer++)
	if ((pTimer->hq == hqueue) && pTimer->timeout)
            TIMER_ClearTimer( pTimer );
}


/***********************************************************************
 *           TIMER_RestartTimers
 *
 * Restart an expired timer.
 */
void TIMER_RestartTimer( TIMER * pTimer, DWORD curTime )
{
    TIMER_RemoveTimer( pTimer );
    pTimer->expires = curTime + pTimer->timeout;
    TIMER_InsertTimer( pTimer );
}

			       
/***********************************************************************
 *           TIMER_GetNextExpiration
 *
 * Return next timer expiration time, or -1 if none.
 */
LONG TIMER_GetNextExpiration(void)
{
    return pNextTimer ? EXPIRE_TIME( pNextTimer, GetTickCount() ) : -1;
}


/***********************************************************************
 *           TIMER_ExpireTimers
 *
 * Mark expired timers and wake the appropriate queues.
 */
void TIMER_ExpireTimers(void)
{
    TIMER *pTimer = pNextTimer;
    DWORD curTime = GetTickCount();

    while (pTimer && !pTimer->expires)  /* Skip already expired timers */
        pTimer = pTimer->next;
    while (pTimer && (pTimer->expires <= curTime))
    {
        pTimer->expires = 0;
        QUEUE_IncTimerCount( pTimer->hq );
        pTimer = pTimer->next;
    }
}


/***********************************************************************
 *           TIMER_GetTimerMsg
 *
 * Build a message for an expired timer.
 */
WINBOOL TIMER_GetTimerMsg( MSG *msg, HWND hwnd,
                          HQUEUE hQueue,WINBOOL remove )
{
    TIMER *pTimer = pNextTimer;
    DWORD curTime = GetTickCount();

    if (hwnd)  /* Find first timer for this window */
	while (pTimer && (pTimer->hwnd != hwnd)) pTimer = pTimer->next;
    else   /* Find first timer for this queue */
	while (pTimer && (pTimer->hq != hQueue)) pTimer = pTimer->next;

    if (!pTimer || (pTimer->expires > curTime)) return FALSE; /* No timer */
    if (remove)	TIMER_RestartTimer( pTimer, curTime );  /* Restart it */

    DPRINT( "Timer expired: %04x, %04x, %04x, %08lx\n", 
		   pTimer->hwnd, pTimer->msg, pTimer->id, (DWORD)pTimer->proc);

      /* Build the message */
    msg->hwnd    = (HWND)pTimer->hwnd;
    msg->message = pTimer->msg;
    msg->wParam  = (UINT)pTimer->id;
    msg->lParam  = (LONG)pTimer->proc;
    msg->time    = curTime;
    return TRUE;
}


/***********************************************************************
 *           TIMER_SetTimer
 */
UINT TIMER_SetTimer( HWND hwnd, UINT id, UINT timeout,
                              TIMERPROC proc, WINBOOL sys )
{
    int i;
    TIMER * pTimer;

    int type = 0;

    if (!timeout) return 0;

      /* Check if there's already a timer with the same hwnd and id */

    for (i = 0, pTimer = TimersArray; i < NB_TIMERS; i++, pTimer++)
        if ((pTimer->hwnd == hwnd) && (pTimer->id == id) &&
            (pTimer->timeout != 0))
        {
              /* Got one: set new values and return */
            TIMER_RemoveTimer( pTimer );
            pTimer->timeout = timeout;
  //          WINPROC_FreeProc( pTimer->proc, WIN_PROC_TIMER );
            pTimer->proc = (HWINDOWPROC)0;
            if (proc)  {
		pTimer->proc = proc;
		//WINPROC_SetProc( &pTimer->proc, proc, type,WIN_PROC_TIMER );
	    }
            pTimer->expires = GetTickCount() + timeout;
            TIMER_InsertTimer( pTimer );
            return id;
        }

      /* Find a free timer */
    
    for (i = 0, pTimer = TimersArray; i < NB_TIMERS; i++, pTimer++)
	if (!pTimer->timeout) break;

    if (i >= NB_TIMERS) return 0;
    if (!sys && (i >= NB_TIMERS-NB_RESERVED_TIMERS)) return 0;
    if (!hwnd) id = i + 1;
    
      /* Add the timer */

    pTimer->hwnd    = hwnd;
    pTimer->hq	    = (hwnd) ? GetThreadQueue( GetWindowThreadProcessId( hwnd, NULL ) )
			     : GetFastQueue( );
    pTimer->msg     = sys ? WM_SYSTIMER : WM_TIMER;
    pTimer->id      = id;
    pTimer->timeout = timeout;
    pTimer->expires = GetTickCount() + timeout;
    pTimer->proc    = (HWINDOWPROC)0;
    if (proc) {
	pTimer->proc = proc;
	// WINPROC_SetProc( &pTimer->proc, proc, type, WIN_PROC_TIMER );
    }
    DPRINT( "Timer added: %p, %04x, %04x, %04x, %08lx\n", 
		   pTimer, pTimer->hwnd, pTimer->msg, pTimer->id,
                   (DWORD)pTimer->proc );
    TIMER_InsertTimer( pTimer );
    if (!id) return TRUE;
    else return id;
}


/***********************************************************************
 *           TIMER_KillTimer
 */
WINBOOL TIMER_KillTimer( HWND hwnd, UINT id,WINBOOL sys )
{
    int i;
    TIMER * pTimer;
    
    /* Find the timer */
    
    for (i = 0, pTimer = TimersArray; i < NB_TIMERS; i++, pTimer++)
	if ((pTimer->hwnd == hwnd) && (pTimer->id == id) &&
	    (pTimer->timeout != 0)) break;
    if (i >= NB_TIMERS) return FALSE;
    if (!sys && (i >= NB_TIMERS-NB_RESERVED_TIMERS)) return FALSE;
    if (!sys && (pTimer->msg != WM_TIMER)) return FALSE;
    else if (sys && (pTimer->msg != WM_SYSTIMER)) return FALSE;    

    /* Delete the timer */

    TIMER_ClearTimer( pTimer );
    return TRUE;
}




