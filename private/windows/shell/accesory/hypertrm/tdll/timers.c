/* timers.c -- Multiplexed timer routines -- run several timers off one
 *					Windows timer.
 *
 *	Copyright 1994 by Hilgraeve, Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 2 $
 *	$Date: 2/05/99 3:21p $
 */

#include <windows.h>
#pragma hdrstop

#include <tdll\stdtyp.h>
#include <tdll\assert.h>
#include <tdll\mc.h>
#include <tdll\session.h>
#include "timers.h"

typedef struct s_timer ST_TIMER;
typedef struct s_timer_mux ST_TIMER_MUX;

struct s_timer_mux
	{
	HWND	  hWnd;
	UINT	  uiID;
	UINT_PTR  uiTimer;
	UINT	  uiLastDuration;
	int 	  fInMuxProc;
	ST_TIMER *pstFirst;
	ST_TIMER *pstCurrent;
	CRITICAL_SECTION critsec;
	};

struct s_timer
	{
	ST_TIMER	 *pstNext;
	ST_TIMER_MUX *pstTimerMux;
	long		  lInterval;
	long		  lLastFired;
	long		  lFireTime;
	void		  *pvData;
	TIMERCALLBACK pfCallback;
	};


void TimerInsert(ST_TIMER_MUX *pstTimerMux, ST_TIMER *pstTimer);
int  TimerSet(ST_TIMER_MUX *pstTimerMux);

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: TimerMuxCreate
 *
 * DESCRIPTION:
 *	Creates a Timer Multiplexer from which any number of individual timers
 *	can be created. A Timer Multiplexer uses only one Windows Timer now
 *	matter how many individual timers are created from it.
 *
 * ARGUMENTS:
 *	pHTM -- A pointer to an HTIMERMUX handle. This handle must be used in
 *			subsequent calls to CreateTimer
 *
 * RETURNS:
 *	TIMER_OK		  if successful
 *	TIMER_NOMEM 	  if there is insufficient memory
 *	TIMER_NOWINTIMER  if there are no Windows timers available
 *	TIMER_ERROR 	  if invalid parameters are passed.
 */
int TimerMuxCreate(const HWND hWnd, const UINT uiID, HTIMERMUX * const pHTM)
	{
	int iReturnVal = TIMER_OK;
	ST_TIMER_MUX *pstTM;

	if (!hWnd || !pHTM)
		{
		assert(FALSE);
		return TIMER_ERROR;
		}

	if ((pstTM = malloc(sizeof(ST_TIMER_MUX))) == NULL)
		{
		iReturnVal = TIMER_NOMEM;
		}
	else
		{
		pstTM->hWnd = hWnd;
		pstTM->uiID = uiID;
		pstTM->uiTimer = 0;
		pstTM->uiLastDuration = 0;
		pstTM->pstFirst = (ST_TIMER *)0;
		pstTM->pstCurrent = (ST_TIMER *)0;
		pstTM->fInMuxProc = FALSE;
		InitializeCriticalSection(&pstTM->critsec);

		iReturnVal = TimerSet(pstTM);

		DbgOutStr("TimerMux handle %#lx created.\r\n", pstTM, 0, 0, 0, 0);
		}

	if (iReturnVal != TIMER_OK)
		{
		(void)TimerMuxDestroy((HTIMERMUX *)&pstTM);
		}

	*pHTM = (HTIMERMUX)pstTM;
	return iReturnVal;
	}
/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: TimerMuxDestroy
 *
 * DESCRIPTION:
 *	Destroys a timer multiplexer and any timers still active
 *
 * ARGUMENTS:
 *	phTM -- Pointer to a timer mux handle of type HTIMERMUX created by
 *			an earlier call to TimerMuxCreate. The value pointed to will
 *			be set to NULL after the TimerMux is destroyed
 *
 * RETURNS:
 *	TIMER_OK
 */
int TimerMuxDestroy(HTIMERMUX * const phTM)
	{
	ST_TIMER *pstTimer;
	ST_TIMER_MUX *pstTimerMux;
	assert(phTM);

	pstTimerMux = (ST_TIMER_MUX *)*phTM;

	if (pstTimerMux)
		{
		EnterCriticalSection(&pstTimerMux->critsec);
		while (pstTimerMux->pstFirst)
			{
			pstTimer = pstTimerMux->pstFirst;
			(void)TimerDestroy((HTIMER *)&pstTimer);
			}
		if (pstTimerMux->uiTimer)
			{
			DbgOutStr("KillTimer (timers.c)\r\n",0,0,0,0,0);
			(void)KillTimer(pstTimerMux->hWnd, pstTimerMux->uiID);
			}
		DeleteCriticalSection(&pstTimerMux->critsec);
		free(pstTimerMux);
		pstTimerMux = NULL;
		DbgOutStr("TimerMux handle 0x%lx destroyed.\r\n", pstTimerMux, 0, 0, 0, 0);
		}
	*phTM = (HTIMERMUX)0;
	return TIMER_OK;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: TimerCreate
 *
 * DESCRIPTION:
 *	Creates a timer that will call a registered call back function at
 *	regular intervals specified in milliseconds. Once created, a timer
 *	can be destroyed by calling TimerDestroy(). TimerDestroy can be called
 *	from within the timer callback procedure.
 *
 * ARGUMENTS:
 *	hTM 	   -- A timer multiplexer handle returned from a call to
 *					TimerMuxCreate
 *	phTimer    -- Pointer to a variable of type HTIMER to receive the
 *					handle of the new timer.
 *	lInterval -- The timer interval in milliseconds. The callback function
 *					will be called repeatedly at roughly this interval until
 *					the timer is destroyed. The timer will operate at a minimum
 *					resolution depending on system capabilities. In Windows 3.x,
 *					the minimum resolution is 55 msecs. Due to the operation
 *					of the underlying Windows timer function, any interval may
 *					be extended an arbitrary amount of time.
 *	pfCallback -- Pointer to a function to be called after each interval.
 *					This function should be of type TIMER_CALLBACK. The
 *					value passed should be the result of calling
 *					MakeProcInstance on the actual callback function.
 *
 *					This function should accept two arguments: a void ptr
 *					value which will contain any value supplied in the
 *					pvData argument described below; and an unsigned long
 *					which will be set to the actual duration of the most
 *					recent interval in milliseconds.
 *	pvData	   -- Can contain any arbitrary data. This value will be
 *					associated with the timer created and will be passed as
 *					an argument when the callback funtion is called.
 *
 * RETURNS:
 *	TIMER_OK		 if timer could be created.
 *	TIMER_NOMEM 	 if there was insufficient memory
 *	TIMER_NOWINTIMER if no Windows timers are available
 *	TIMER_ERROR 	 if parameters were invalid (also generates an assert)
 */
int TimerCreate(const HTIMERMUX 	hTM,
					  HTIMER		* const phTimer,
					  long			lInterval,
				const TIMERCALLBACK pfCallback,
					  void			*pvData)
	{
	ST_TIMER_MUX * const pstTimerMux = (ST_TIMER_MUX *)hTM;
	ST_TIMER *pstTimer = (ST_TIMER *)0;
	int iReturnVal = TIMER_OK;

	assert(pstTimerMux && pfCallback);

	EnterCriticalSection(&pstTimerMux->critsec);
	// Guard against a zero interval
	if (lInterval == 0L)
		++lInterval;

	if (!pstTimerMux || !pfCallback)
		iReturnVal = TIMER_ERROR;
	else if ((pstTimer = malloc(sizeof *pstTimer)) == NULL)
		iReturnVal = TIMER_NOMEM;
	else
		{
		pstTimer->pstNext	  = (ST_TIMER *)0;
		pstTimer->pstTimerMux = pstTimerMux;
		pstTimer->lInterval  = lInterval;
		pstTimer->lLastFired =	(long)GetTickCount();
		pstTimer->lFireTime  = pstTimer->lLastFired + lInterval;
		pstTimer->pvData	  = pvData;
		pstTimer->pfCallback  = pfCallback;

		TimerInsert(pstTimerMux, pstTimer);

		// Following code caused problems when called from a thread other
		//	than the main thread
		// if ((iReturnVal = TimerSet(pstTimerMux)) != TIMER_OK)
		//	   (void)TimerDestroy((HTIMER *)&pstTimer);

		PostMessage(pstTimerMux->hWnd, WM_FAKE_TIMER, 0, 0);

		DbgOutStr("Timer handle %#lx (%#lx) created.\r\n", pstTimer, pstTimerMux, 0, 0, 0);
		}

	if (phTimer)
		*phTimer = (HTIMER)pstTimer;

	LeaveCriticalSection(&pstTimerMux->critsec);
	return iReturnVal;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: TimerDestroy
 *
 * DESCRIPTION:
 *	Destroys a timer created with TimerCreate. This routine can be called
 *	to destroy a timer from within its own callback functionl
 *
 * ARGUMENTS:
 *	hTimer -- A timer handle returned from a call to TimerCreate.
 *
 * RETURNS:
 *	TIMER_OK  if the timer is found and destroyed.
 *	TIMER_ERROR if the handle could not be found.
 */
int TimerDestroy(HTIMER * const phTimer)
	{
	int 		  iReturnVal = TIMER_OK;
	ST_TIMER	  stDummy;
	ST_TIMER	 *pstTimer = (ST_TIMER *)*phTimer;
	ST_TIMER	 *pstScan;
	ST_TIMER	 *pstFound;
	ST_TIMER_MUX *pstTimerMux;

	assert(phTimer);

	if (pstTimer)
		{
		// Get pointer to parent struct
		pstTimerMux = pstTimer->pstTimerMux;
		EnterCriticalSection(&pstTimerMux->critsec);

		// If a timer is being destroyed from within its own callback, it
		//	 has already been removed from the timer chain. Setting
		//	 pstTimerMux->pstCurrent to NULL will prevent it from being
		//	 rescheduled.
		if (pstTimer == pstTimerMux->pstCurrent)
			{
			free(pstTimer);
			pstTimer = NULL;
			DbgOutStr("Timer destroyed 0x%lx\r\n", (LONG)pstTimer, 0, 0, 0, 0);
			*phTimer = (HTIMER)0;
			pstTimerMux->pstCurrent = (ST_TIMER *)0;
			}

		else
			{
			// Set up dummy node at head of list to avoid a bunch of
			//	 special cases
			stDummy.pstNext = pstTimerMux->pstFirst;
			pstScan = &stDummy;

			// Scan through list for match, maintaining pointer to the
			//	 node BEFORE
			while ((pstFound = pstScan->pstNext) != (ST_TIMER *)0)
				{
				if (pstFound == pstTimer)
					break;
				pstScan = pstFound;
				}

			// pstFound will be NULL if timer was not in list, otherwise
			//	pstFound is the node to remove and pstScan is the node
			//	prior to it
			if (!pstFound)
				iReturnVal = TIMER_ERROR;
			else
				{
				pstScan->pstNext = pstFound->pstNext;
				DbgOutStr("Timer handle 0x%lx destroyed.\r\n", pstFound, 0, 0, 0, 0);
				free(pstFound);
				pstFound = NULL;

				// If we just destroyed a timer from within its own callback,
				//	leave a sign so the timer proc will know
				if (pstFound == pstTimerMux->pstCurrent)
					pstTimerMux->pstCurrent = (ST_TIMER *)0;

				// Remove dummy node from start of list
				pstTimerMux->pstFirst = stDummy.pstNext;
				*phTimer = (HTIMER)0;
				}
			}
		LeaveCriticalSection(&pstTimerMux->critsec);
		}

	return iReturnVal;
	}



/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: TimerMuxProc
 *
 * DESCRIPTION:
 *	This function should be called by the window proc of the window whose
 *	handle was passed to TimerMuxCreate when a WM_TIMER message is received.
 *	It uses one Windows timer to control any number of individual multiplexed
 *	timers.
 *
 * ARGUMENTS:
 *	hTM -- The timer mux handle returned by TimerMuxCreate.
 *
 * RETURNS:
 *	nothing
 */
void TimerMuxProc(const HTIMERMUX hTM)
	{
	ST_TIMER *pstScan;
	ST_TIMER_MUX * const pstTimerMux = (ST_TIMER_MUX *)hTM;
	long lNow;

	// Callbacks to timer procs to the printing routines can take a
	// long time because of paper-out, etc.  Since the AbortProc in
	// the printer routines yields via a message loop, it is possible
	// (read probable) that we can recursively enter this routine.
	// The fInMuxProc flag guards against such an event. - mrw

	if (!pstTimerMux->fInMuxProc)
		pstTimerMux->fInMuxProc = TRUE;

	else
		return;

	EnterCriticalSection(&pstTimerMux->critsec);
	lNow = (long)GetTickCount();
	DbgOutStr("%ld ", lNow, 0, 0, 0, 0);

	// In the following routine, note that the node associated with the
	//	current call back is NOT linked into the timer chain during the
	//	call back. This allows TimerDestroy to be called on a timer from
	//	within its own call back. It is also OK to call TimerCreate from
	//	within call backs.

	// Since timer ticks can be delayed, more than one event may have expired
	pstScan = pstTimerMux->pstFirst;
	while (pstScan && lNow > pstScan->lFireTime)
		{
		// Keep track of which timer is being called
		pstTimerMux->pstCurrent = pstScan;

		// Remove current node from list (will be added back in later if
		//	not destroyed)
		pstTimerMux->pstFirst = pstScan->pstNext;

		// Give up the critical section while doing the callback so
		// a lengthy call back won't delay any other threads
		LeaveCriticalSection(&pstTimerMux->critsec);

		// Activate the call back function
		(*pstScan->pfCallback)(pstScan->pvData, lNow - pstScan->lLastFired);
		EnterCriticalSection(&pstTimerMux->critsec);

		lNow = (long)GetTickCount();
		DbgOutStr("%ld ", lNow, 0, 0, 0, 0);

		// If timer was destroyed during callback, pstTimerMux->pstCurrent will have
		//	been sent to NULL; otherwise reschedule this timer
		if ((pstScan = pstTimerMux->pstCurrent) != (ST_TIMER *)0)
			{
			DbgOutStr("Reschedule ", 0, 0, 0, 0, 0);
			// Reschedule timer
			pstScan->lLastFired = lNow;
			pstScan->lFireTime = lNow + pstScan->lInterval;

			// link this timer back into the list
			TimerInsert(pstTimerMux, pstScan);
			pstTimerMux->pstCurrent = (ST_TIMER *)0;
			}

		// First node on list is always the next one due to fire
		pstScan = pstTimerMux->pstFirst;
		}

	(void)TimerSet(pstTimerMux);

	pstTimerMux->fInMuxProc = FALSE;
	LeaveCriticalSection(&pstTimerMux->critsec);
	}

//			INTERNAL ROUTINES


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: TimerInsert
 *
 * DESCRIPTION:
 *	Links a timer control node into the linked list of all multiplexed timers.
 *	The list is maintained in order by when the node is due to fire.
 *
 * ARGUMENTS:
 *	pstTimerMux -- Handle to the timer multiplexer.
 *	pstTimer	-- Pointer to a node to be inserted.
 *
 * RETURNS:
 *	nothing
 */
void TimerInsert(ST_TIMER_MUX *pstTimerMux,
				 ST_TIMER *pstTimer)
	{
	ST_TIMER *pstScan;

	pstScan = pstTimerMux->pstFirst;

	// If there are no other nodes in the list or if the new timer is
	//	 scheduled before the first one in the list, link new one in first
	if (!pstScan || pstTimer->lFireTime < pstScan->lFireTime)
		{
		pstTimer->pstNext = pstScan;
		pstTimerMux->pstFirst = pstTimer;
		}
	else
		{
		// Insert sorted by lFireTime
		while (pstScan->pstNext &&
				pstScan->pstNext->lFireTime < pstTimer->lFireTime)
			{
			pstScan = pstScan->pstNext;
			}

		// Link into chain
		pstTimer->pstNext = pstScan->pstNext;
		pstScan->pstNext = pstTimer;
		}
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: TimerSet
 *
 * DESCRIPTION:
 *	Sets up a Windows timer using SetTimer to fire when the next multiplexed
 *	timer needs attention. Since the Window timer operates with an interval
 *	specified in a USHORT, there are times when the timer may have to be
 *	set to go off before the next required interval. If there are no
 *	multiplexed timers to be serviced, the timer is set to its maximum time
 *	anyway. By keeping one timer whether we need it or not, we guarantee
 *	that one will be available when we DO need it.
 *
 * ARGUMENTS:
 *	none
 *
 * RETURNS:
 *	TIMER_OK if successful.
 *	TIMER_NOWINTIMER if no Windows timers were available
 */
int TimerSet(ST_TIMER_MUX *pstTimerMux)
	{
	UINT uiDuration = 100000;
	int  iReturnVal = TIMER_OK;
	long lTickCount;

	if (pstTimerMux->pstFirst)
		{
		lTickCount = (long)GetTickCount();

		if (pstTimerMux->pstFirst->lFireTime <= lTickCount)
			uiDuration = 1; 	// Timer has already expired
		else
			uiDuration = (UINT)(pstTimerMux->pstFirst->lFireTime - lTickCount);
		}

	if (pstTimerMux->uiTimer == 0 || uiDuration != pstTimerMux->uiLastDuration)
		{
		// if (pstTimerMux->uiTimer != 0)
		//	   {
		//	   DbgOutStr("KillTimer (timers.c)\r\n",0,0,0,0,0);
		//	   fResult = KillTimer(pstTimerMux->hWnd, pstTimerMux->uiID);
		//	   assert(fResult);
		//	   }

		pstTimerMux->uiTimer =
			SetTimer(pstTimerMux->hWnd, pstTimerMux->uiID, uiDuration, NULL);

		DbgOutStr("SetTimer (timers.c)\r\n", 0,0,0,0,0);

		if (pstTimerMux->uiTimer == 0)
			{
			iReturnVal = TIMER_NOWINTIMER;
			}

		pstTimerMux->uiLastDuration = uiDuration;
		}
	return (iReturnVal);
	}

//	End of timers.c
