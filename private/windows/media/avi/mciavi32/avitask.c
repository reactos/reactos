/******************************************************************************

   Copyright (C) Microsoft Corporation 1985-1995. All rights reserved.

   Title:   avitask.c - Background task that actually manipulates AVI files.

*****************************************************************************/
#include "graphic.h"

STATICFN BOOL OnTask_ProcessRequest(NPMCIGRAPHIC npMCI);
STATICFN void OnTask_WinProcRequests(NPMCIGRAPHIC npMCI, BOOL bPlaying);
STATICFN void OnTask_StopTemporarily(NPMCIGRAPHIC npMCI);
STATICFN void OnTask_RestartAgain(NPMCIGRAPHIC npMCI, BOOL bSetEvent);

STATICFN DWORD InternalPlayStart(
    NPMCIGRAPHIC npMCI,
    DWORD dwFlags,
    long lReqTo,
    long lReqFrom,
    DWORD_PTR dwCallback
);

BOOL TryStreamUpdate(
    NPMCIGRAPHIC npMCI,
    DWORD dwFlags,
    HDC hdc,
    LPRECT lprc
);


/*
 * design overview under construction
 *
 * this file contains the core code for the worker thread that manages
 * playback on request from the user's thread. The worker thread also
 * creates a wndproc thread that owns the default playback window.
 */



// set the error flag and signal completion of request
void
TaskReturns(NPMCIGRAPHIC npMCI, DWORD dwErr)
{
    npMCI->dwReturn = dwErr;

    // clear the hEventSend manual-reset event now that we
    // have processed it
    ResetEvent(npMCI->hEventSend);

#ifdef DEBUG
    // make the message invalid
    npMCI->message = 0;
#endif

    // Wake up the thread that made the request
    DPF2(("...[%x] ok", npMCI->hRequestor));
    SetEvent(npMCI->hEventResponse);
}


/*
 * KillWinProcThread:
 *
 * If the winproc thread exists, send a message to the window to cause
 * the thread to destroy the window and terminate.
 */
STATICFN void KillWinProcThread(NPMCIGRAPHIC npMCI)
{
    // kill the winproc thread and wait for it to die
    if (npMCI->hThreadWinproc) {

	INT_PTR bOK = TRUE;

	if (npMCI->hwndDefault) {
	    // must destroy on creating thread
	    bOK = SendMessage(npMCI->hwndDefault, AVIM_DESTROY, 0, 0);
	    if (!bOK) {
		DPF1(("failed to destroy window: %d", GetLastError()));
	    } else {
		Assert(!IsWindow(npMCI->hwndDefault));
	    }
	}

	// wait for winproc thread to destroy itself when the window goes
	if (bOK) {
	    WaitForSingleObject(npMCI->hThreadWinproc, INFINITE);
	}
	CloseHandle(npMCI->hThreadWinproc);
	npMCI->hThreadWinproc = 0;

    }
}


/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api void | mciaviTask |  This function is the background task which plays
 *      AVI files. It is called as a result of the call to mmTaskCreate()
 *      in DeviceOpen(). When this function returns, the task is destroyed.
 *
 * @parm DWORD | dwInst | instance data passed to mmCreateTask - contains
 *      a pointer to an instance data block.
 *
 ***************************************************************************/

void FAR PASCAL _LOADDS mciaviTask(DWORD_PTR dwInst)
{
    NPMCIGRAPHIC npMCI;
    DWORD dwThreadId;
    BOOL bExit = FALSE;

    npMCI = (NPMCIGRAPHIC) dwInst;

    // Set this task's error mode to the same as the parent's.
    SetErrorMode(npMCI->uErrorMode);

    DPF1(("Bkgd Task hTask=%04X\n", GetCurrentTask()));

    /* We must set hTask up before changing the TaskState as the UI */
    /* thread can abort as soon as wTaskState is altered            */
    /* NB: this comment is no longer true.  Since the rewrite of    */
    /* mciavi the UI thread will create the task thread and wait    */
    /* until it is explicitly signalled.                            */
    npMCI->hTask = GetCurrentTask();
    npMCI->wTaskState = TASKIDLE;
    npMCI->dwTaskError = 0;


    // create a critical section to protect window-update code between
    // the worker and the winproc thread
    InitializeCriticalSection(&npMCI->HDCCritSec);
    SetNTFlags(npMCI, NTF_DELETEHDCCRITSEC);

    // create a critical section to protect window-request code between
    // the worker and the winproc thread
    InitializeCriticalSection(&npMCI->WinCritSec);
    SetNTFlags(npMCI, NTF_DELETEWINCRITSEC);

    // create an event to wait on for the winproc thread  to tell us that
    // init is ok.
    npMCI->hEventWinProcOK = CreateEvent(NULL, FALSE, FALSE, NULL);

    // also a second event that the winproc signals when it has
    // requests for us
    npMCI->heWinProcRequest = CreateEvent(NULL, TRUE, FALSE, NULL);

    if (!npMCI->hEventWinProcOK || !npMCI->heWinProcRequest) {

	npMCI->dwTaskError = MCIERR_OUT_OF_MEMORY;
	mciaviTaskCleanup(npMCI);
	// Abort this thread.  Our waiter will wake up when our thread
	// handle is signalled.
	return;
    }


    // create a second background task to create the default window and
    // own the winproc.
#if 0
    if (mmTaskCreate((LPTASKCALLBACK) aviWinProcTask,
		     &npMCI->hThreadWinproc,
		     (DWORD)(UINT)npMCI) == 0)
#else
    if (npMCI->hThreadWinproc = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)aviWinProcTask,
				   (LPVOID)npMCI, 0, &dwThreadId ))
#endif
    {
	// wait to be sure that window can be created
	// hThreadWinproc will be signalled if the thread exits.
	if (WaitForMultipleObjects(2, &npMCI->hThreadWinproc,
		FALSE, INFINITE) == WAIT_OBJECT_0) {

	    // thread aborted
	    npMCI->dwTaskError = MCIERR_CREATEWINDOW;

	    // dont wait for this thread in cleanup phase
	    CloseHandle(npMCI->hThreadWinproc);
	    npMCI->hThreadWinproc = 0;
	
	    mciaviTaskCleanup(npMCI);
            // Abort this thread.  Our waiter will wake up when our thread
            // handle is signalled.
	    return;
	}
    } else {
	// could not create winproc thread
	npMCI->dwTaskError = MCIERR_CREATEWINDOW;
	mciaviTaskCleanup(npMCI);
        // Abort this thread.  Our waiter will wake up when our thread
        // handle is signalled.
	return;
    }


    /* Open the file  */

    // open has now been done on app thread -complete init here.
    if (!OpenFileInit(npMCI)) {
        DPF1(("Failed to complete open of AVI file\n"));
	mciaviTaskCleanup(npMCI);
        // Abort this thread.  Our waiter will wake up when our thread
        // handle is signalled.
	return;
    }


    // signal that the open is complete
    TaskReturns(npMCI, 0);


    // now loop waiting for requests and processing them
    // ProcessRequest returns TRUE when it is time to exit

    // hEventSend is manual-reset so we can poll it during play.
    // it is reset in TaskReturns just before we signal the response
    // event.

    // hEventAllDone is set here for bDelayedComplete requests
    // (eg play+wait) when the entire request is satisfied and
    // the worker thread is back to idle. hEventResponse is set in
    // ProcessMessage when the request itself is complete - eg for play, once
    // play has started the event will be set.

    // we can't handle more than one thread potentially blocking on
    // hEventAllDone at one time, so while one thread has made a request
    // that could block on hEventAllDone, no other such request is permitted
    // from other threads. In other words, while one (user) thread has
    // requested a play+wait, other threads can request stop, but not
    // play + wait.

    while (!bExit) {
	DWORD dwObject;

	npMCI->wTaskState = TASKIDLE;

#ifdef DEBUG
	// A complex assertion.  If we have stopped temporarily, then we
	// do not want to go back to sleep.
	if ((npMCI->dwFlags & MCIAVI_UPDATING)
	    && (WAIT_TIMEOUT
		== WaitForMultipleObjects(IDLEWAITFOR, &npMCI->hEventSend, FALSE, 0))
	) {
	    Assert(!"About to go to sleep when we should be restarting!");
	}
#endif

	// the OLE guys are kind enough to create windows on this thread.
	// so we need to handle sent messages here to avoid deadlocks.
	// same is true of the similar loop in BePaused()

    	do {
#ifdef DEBUG
	    if (npMCI->hWaiter) {
		DPF(("About to call MsgWaitForMultipleObjects while hWaiter=%x\n", npMCI->hWaiter));
	    }
#endif
	    dwObject = MsgWaitForMultipleObjects(IDLEWAITFOR, &npMCI->hEventSend,
			FALSE, INFINITE, QS_SENDMESSAGE);

	    DPF2(("Task woken up, dwObject=%x, hWaiter=%x\n", dwObject, npMCI->hWaiter));

	    if (dwObject == WAIT_OBJECT_0 + IDLEWAITFOR) {
		MSG msg;

		// just a single peekmessage with NOREMOVE will
		// process the inter-thread send and not affect the queue
		PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);
	    }
	} while (dwObject == WAIT_OBJECT_0 + IDLEWAITFOR);

	switch (dwObject) {
	case WAIT_OBJECT_0:
	    // check that the message has actually been set
	    Assert(npMCI->message != 0);

	    if (npMCI->bDelayedComplete) {
		if (npMCI->hWaiter && (npMCI->hWaiter != npMCI->hRequestor)) {
		    TaskReturns(npMCI, MCIERR_DEVICE_NOT_READY);
		    continue;
		} else {
		    DPF2(("Setting hWaiter to %x\n", npMCI->hRequestor));
		    npMCI->hWaiter = npMCI->hRequestor;
		}
	    }
	
	    DPF2(("get %d [%x]...", npMCI->message, npMCI->hRequestor));

	    // we must reset this event here, or OnTask_PeekRequest will
	    // think it is a new request and will process and
	    // potentially complete it!!
	    ResetEvent(npMCI->hEventSend);

	    bExit = OnTask_ProcessRequest(npMCI);

	    break;

	case WAIT_OBJECT_0+1:
	default:
	    //
	    // winproc request to do something to the task - while idle
	    //
#ifdef DEBUG
            if (dwObject != WAIT_OBJECT_0+1) {
                DPF2(("dwObject == %d\n", dwObject));
            }
#endif
	    Assert(dwObject == WAIT_OBJECT_0+1);
	    OnTask_WinProcRequests(npMCI, FALSE);

	    // this request may have resulted in a temporary stop - so we
	    // need to restart
	    if (npMCI->dwFlags & MCIAVI_UPDATING) {
		OnTask_RestartAgain(npMCI, FALSE);
	    }

	}

        // if we have stopped temporarily to restart with new params,
        // then don't signal completion.  However if we did restart
	// and now everything is quiescent, signal any waiter that happens
	// to be around.  This code is common to both the winproc request
	// and user request legs, as it is possible to stop and restart
	// from both winproc and user requests.
        if (npMCI->hWaiter && (!(npMCI->dwFlags & MCIAVI_UPDATING))) {
	    SetEvent(npMCI->hEventAllDone);
        } else {
	    if (npMCI->hWaiter) {
		DPF2(("Would have Set hEventAllDone, but am updating\n"));
	    }
        }

	// QUERY: if we had processed all the requests, and therefore the
	// two events on which we were waiting had been reset, AND
	// MCIAVI_UPDATING is set (due to a temporary stop) then perhaps
	// we ought to call OnTask_RestartAgain here.  This would mean that
	// all the ugly RestartAgain calls within OnTask_ProcessRequest
	// could be removed.

    }

    // be sure to wake him up before cleanup just in case
    if (npMCI->hWaiter) {
	DPF2(("Setting hEventAllDone before closing\n"));
	SetEvent(npMCI->hEventAllDone);
    }

    // close the window and destroy the winproc thread before any other
    // cleanup, so that paints or realizes don't happen during
    // a partially closed state

    KillWinProcThread(npMCI);

    mciaviCloseFile(npMCI);

    mciaviTaskCleanup(npMCI);
    return;

}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api void | mciaviTaskCleanup |  called when the background task
 *      is being destroyed.  This is where critical cleanup goes.
 *
 ***************************************************************************/

void FAR PASCAL mciaviTaskCleanup(NPMCIGRAPHIC npMCI)
{
     npMCI->hTask = 0;


    /* Closing the driver causes any currently saved notifications */
    /* to abort. */

    GraphicDelayedNotify(npMCI, MCI_NOTIFY_ABORTED);

    GdiFlush();

    // if still alive, kill the winproc thread and wait for it to die
    KillWinProcThread(npMCI);

    // free winproc <-> worker thread communication resources
    if (npMCI->hEventWinProcOK) {
	CloseHandle(npMCI->hEventWinProcOK);
    }
    if (npMCI->heWinProcRequest) {
	CloseHandle(npMCI->heWinProcRequest);
    }


    if (TestNTFlags(npMCI, NTF_DELETEWINCRITSEC)) {
        DeleteCriticalSection(&npMCI->WinCritSec);
    }

    if (TestNTFlags(npMCI, NTF_DELETEHDCCRITSEC)) {
        DeleteCriticalSection(&npMCI->HDCCritSec);
    }


    //
    //  call a MSVideo shutdown routine.
    //
}







// task message functions


// utility functions called on worker thread
/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api void | ShowStage | This utility function brings the default stage
 * window to the foreground on play, seek, step and pause commands. It
 * does nothing if the stage window is not the default window
 *
 * @parm NPMCIGRAPHIC | npMCI | near ptr to the instance data
 *
 ***************************************************************************/

void NEAR PASCAL ShowStage(NPMCIGRAPHIC npMCI)
{

    if (!(npMCI->dwFlags & MCIAVI_NEEDTOSHOW))
        return;


    // don't show stage if we are working in response to a winproc
    // update request, as this could cause deadlock and is any case
    // pointless - if the window is now hidden, we can't possibly need
    // to paint it!
    if (npMCI->bDoingWinUpdate) {
	return;
    }


    if ((npMCI->dwFlags & MCIAVI_SHOWVIDEO) &&
	    npMCI->hwndPlayback == npMCI->hwndDefault &&
////////////!(GetWindowLong(npMCI->hwnd, GWL_STYLE) & WS_CHILD) &&
	    (!IsWindowVisible (npMCI->hwndPlayback) ||
		npMCI->hwndPlayback != GetActiveWindow ())) {

		    // if this is our window, then we need to show it
		    // ourselves
		    if (npMCI->hwndDefault == npMCI->hwndPlayback) {
			WinCritCheckOut(npMCI);
			PostMessage(npMCI->hwndPlayback, AVIM_SHOWSTAGE, 0, 0);
		    } else {
			SetWindowPos(npMCI->hwndPlayback, HWND_TOP, 0, 0, 0, 0,
			    SWP_NOMOVE | SWP_NOSIZE |
			    SWP_SHOWWINDOW |
			    (IsWindowVisible(npMCI->hwndPlayback) ? SWP_NOACTIVATE : 0));
		    }
    }

    //
    // if the movie has palette changes we need to make it the active
    // window otherwise the palette animation will not work right
    //
    if ((npMCI->dwFlags & MCIAVI_ANIMATEPALETTE) &&
            !(npMCI->dwFlags & MCIAVI_SEEKING) &&
            !(npMCI->dwFlags & MCIAVI_FULLSCREEN) &&
            !(npMCI->dwFlags & MCIAVI_UPDATING) &&
            npMCI->hwndPlayback == npMCI->hwndDefault &&
            !(GetWindowLong(npMCI->hwndPlayback, GWL_STYLE) & WS_CHILD)) {
        // if this is our window, then we need to show it
        // ourselves
        if (npMCI->hwndDefault == npMCI->hwndPlayback) {

            // force activation even if visible
	    WinCritCheckOut(npMCI);
            PostMessage(npMCI->hwndPlayback, AVIM_SHOWSTAGE, TRUE, 0);
        } else {
            SetActiveWindow(npMCI->hwndPlayback);
        }
    }

    npMCI->dwFlags &= ~(MCIAVI_NEEDTOSHOW);
}


//
// called to save state if we stop temporarily to change something.
// we will restart with OnTask_RestartAgain. Called on worker thread from
// somewhere in aviTaskCheckRequests
STATICFN void OnTask_StopTemporarily(NPMCIGRAPHIC npMCI)
{
    // save old state and flags
    npMCI->oldState = npMCI->wTaskState;
    npMCI->oldTo = npMCI->lTo;
    npMCI->oldFrom = npMCI->lFrom;
    npMCI->oldFlags = npMCI->dwFlags;
    npMCI->oldCallback = (DWORD_PTR) npMCI->hCallback;

    npMCI->dwFlags |= (MCIAVI_UPDATING | MCIAVI_STOP);
    DPF(("StopTemp: OldState=%d, oldTo=%d, oldFrom=%d, oldFlags=%8x\n",
	npMCI->oldState, npMCI->oldTo, npMCI->oldFrom, npMCI->oldFlags));
}


// called from worker thread on completion of a (idle-time) request
// to restart a suspended play function
//
// responsible for signalling hEventResponse once the restart is complete
// (or failed).
STATICFN void OnTask_RestartAgain(NPMCIGRAPHIC npMCI, BOOL bSetEvent)
{
    DWORD dwErr;
    DWORD dwFlags = 0;
    long lFrom;
    UINT wNotify;

    // we're restarting after a temporary stop- so clear the flag.
    // also turn off REPEATING - we might reset this in a moment
    npMCI->dwFlags &= ~(MCIAVI_UPDATING | MCIAVI_REPEATING);

    // Turn on the repeating flag if it was originally set
    npMCI->dwFlags |= (npMCI->oldFlags & MCIAVI_REPEATING);

    if (npMCI->oldFlags & MCIAVI_REVERSE) {
	dwFlags |= MCI_DGV_PLAY_REVERSE;
    }

    switch (npMCI->oldState) {
	case TASKPAUSED:
	    // get to the old From frame and pause when you get there
	    npMCI->dwFlags |= MCIAVI_PAUSE;  // Make sure we end up paused
	    // NOTE: InternalPlayStart no longer clears the PAUSE flag
	    lFrom = npMCI->oldFrom;
	    break;

        case TASKCUEING:

	    // npMCI->dwFlags should still say whether to pause at
	    // end of cueing or play
	    lFrom = npMCI->oldFrom;
	    dwFlags |= MCI_TO;	  // Stop when we get to the right frame
	    break;

        case TASKPLAYING:

	    lFrom = npMCI->lCurrentFrame;
	    dwFlags |= MCI_TO;
	    break;

        default:

	    DPF(("asked to restart to idle (%d) state", npMCI->oldState));
	    if (bSetEvent) {
		TaskReturns(npMCI, 0);
	    }
	    return;
    }


    DPF2(("RestartAgain calling InternalPlayStart, flags=%8x\n",dwFlags));
    dwErr = InternalPlayStart(npMCI, dwFlags,
		npMCI->oldTo, lFrom, npMCI->oldCallback);

    if (bSetEvent && dwErr) {
	TaskReturns(npMCI, dwErr);
    }

    if (!dwErr) {
	wNotify = mciaviPlayFile(npMCI, bSetEvent);

	// if we stopped to pick up new params without actually completing the
	// the play (OnTask_StopTemporarily) then MCIAVI_UPDATING will be set

	if (! (npMCI->dwFlags & MCIAVI_UPDATING)) {
	    // perform any notification
	    if (wNotify != MCI_NOTIFY_FAILURE) {
		GraphicDelayedNotify(npMCI, wNotify);
	    }
	}
    }
}


/***************************************************************************
 *
 *  IsScreenDC() - returns true if the passed DC is a DC to the screen.
 *                 NOTE this checks for a DCOrg != 0, bitmaps always have
 *                 a origin of (0,0)  This will give the wrong info a
 *                 fullscreen DC.
 *
 ***************************************************************************/

#ifndef _WIN32
#define IsScreenDC(hdc)     (GetDCOrg(hdc) != 0L)
#else
INLINE BOOL IsScreenDC(HDC hdc)
{
    return (WindowFromDC(hdc) != NULL);
}
#endif



// called from several task side functions to initiate play
// when stopped. All you need to do is call mciaviPlayFile
// once this function returns
STATICFN DWORD
InternalPlayStart(
    NPMCIGRAPHIC npMCI,
    DWORD dwFlags,
    long lReqTo,
    long lReqFrom,
    DWORD_PTR dwCallback
)
{
    long lTo, lFrom;
    DWORD dwRet;

    if (dwFlags & (MCI_MCIAVI_PLAY_FULLSCREEN | MCI_MCIAVI_PLAY_FULLBY2)) {
	// do nothing here - handled in fullproc
    } else {
	if (!IsWindow(npMCI->hwndPlayback)) {
	    return MCIERR_NO_WINDOW;
	}

	npMCI->dwFlags |= MCIAVI_NEEDTOSHOW;
    }


    /* Range checks : 0 < 'from' <= 'to' <= last frame */

    if (dwFlags & MCI_TO) {
	lTo = lReqTo;

        if (lTo < 0L || lTo > npMCI->lFrames) {
	    return MCIERR_OUTOFRANGE;
	}
    } else {
        if (dwFlags & MCI_DGV_PLAY_REVERSE)
            lTo = 0;
        else
            lTo = npMCI->lFrames;

        dwFlags |= MCI_TO;
    }


    // if no from setting, then get current position
    if (dwFlags & MCI_FROM) {
	lFrom = lReqFrom;

        if (lFrom < 0L || lFrom > npMCI->lFrames) {
	    return MCIERR_OUTOFRANGE;
	}
    } else if (dwRet = InternalGetPosition(npMCI, &lFrom)) {
    	return dwRet;
    }

    /* check 'to' and 'from' relationship.  */
    if (lTo < lFrom)
	dwFlags |= MCI_DGV_PLAY_REVERSE;

    if ((lFrom < lTo) && (dwFlags & MCI_DGV_PLAY_REVERSE)) {
	return MCIERR_OUTOFRANGE;
    }

    /* If the test flag is set, return without doing anything. */
    /* Question: do we have to check for more possible errors? */
    if (dwFlags & MCI_TEST) {
	return 0;
    }


    npMCI->lFrom = lFrom;

	
    if (dwFlags & MCI_DGV_PLAY_REPEAT) {
	/* If from position isn't given, repeat from either the beginning or
	** end of file as appropriate.
	*/
	npMCI->lRepeatFrom =
	    (dwFlags & MCI_FROM) ? lFrom :
		((dwFlags & MCI_DGV_PLAY_REVERSE) ? npMCI->lFrames : 0);
    }


    /* if not already playing, start the task up. */

    /* First, figure out what mode to use. */
    if (dwFlags & (MCI_MCIAVI_PLAY_FULLSCREEN | MCI_MCIAVI_PLAY_FULLBY2)) {
        if (npMCI->rcDest.right - npMCI->rcDest.left >
            npMCI->rcMovie.right - npMCI->rcMovie.left)
	    dwFlags |= MCI_MCIAVI_PLAY_FULLBY2;
	
        if ((dwFlags & MCI_MCIAVI_PLAY_FULLBY2) &&
                (npMCI->rcMovie.right  <= 160) &&
                (npMCI->rcMovie.bottom <= 120)) {
            npMCI->dwFlags |= MCIAVI_ZOOMBY2;
	} else {
	    npMCI->dwFlags &= ~(MCIAVI_ZOOMBY2);
	}


	if ((dwFlags & MCI_WAIT) && !(npMCI->dwFlags & MCIAVI_REPEATING))
	    npMCI->dwFlags |= MCIAVI_NOBREAK;
	else
	    npMCI->dwFlags &= ~(MCIAVI_NOBREAK);
		
	npMCI->dwFlags |= MCIAVI_FULLSCREEN;
    } else {
	npMCI->dwFlags &= ~(MCIAVI_FULLSCREEN);
    }


    // Make sure flags are cleared if they should be
    //npMCI->dwFlags &= ~(MCIAVI_PAUSE | MCIAVI_CUEING | MCIAVI_REVERSE);
    // PAUSE is NOT turned off, otherwise we cannot RestartAgain to a
    // paused state.
    npMCI->dwFlags &= ~(MCIAVI_CUEING | MCIAVI_REVERSE);

    if (dwFlags & MCI_DGV_PLAY_REPEAT) {
	npMCI->dwFlags |= MCIAVI_REPEATING;
    }

    /* Don't set up notify until here, so that the seek won't make it happen*/
    // idle so no current notify
    if (dwFlags & MCI_NOTIFY) {
        GraphicSaveCallback(npMCI, (HANDLE) (UINT_PTR)dwCallback);
    }


    if (lTo > npMCI->lFrames)
        lTo = npMCI->lFrames;

    if (lTo < 0)
        lTo = 0;

    if (dwFlags & MCI_TO)
	npMCI->lTo = lTo;

    DPF2(("InternalPlayStart  Flags=%8x, ReqTo=%d  ReqFrom=%d   To=%d\n",
	    dwFlags, lReqTo, lReqFrom, lTo));

    if (dwFlags & MCI_DGV_PLAY_REVERSE)
	npMCI->dwFlags |= MCIAVI_REVERSE;


    if (npMCI->dwFlags & MCIAVI_NEEDTOSHOW) {
	ShowStage(npMCI);
	//
	// leave this set so the play code knows this is a "real" play
	// coming from the user, not an internal play/stop
	//
	// if the window needs shown we want to do it here if we can
	// not in the background task.
	//
	npMCI->dwFlags |= MCIAVI_NEEDTOSHOW;
    }


    return 0;


}


// called at task idle time to initiate a play request -
// the worker thread is NOT busy playing, seeking, cueing or paused
// at this point.
//
// responsible for calling TaskReturns() appropriately.
void
OnTask_Play(NPMCIGRAPHIC npMCI)
{

    DWORD dwRet;
    DWORD dwMCIFlags = npMCI->dwParamFlags;
    LPMCI_DGV_PLAY_PARMS lpPlay = (LPMCI_DGV_PLAY_PARMS)npMCI->lParam;
    long lTo, lFrom;
    UINT wNotify;

    if (lpPlay != NULL) {
	lTo = lpPlay->dwTo;
	lFrom = lpPlay->dwFrom;
    } else {
	dwMCIFlags &= ~(MCI_TO | MCI_FROM);
    }

    npMCI->dwFlags &= ~MCIAVI_REPEATING;

    // need to convert to frames before calling InternalPlayStart
    if (dwMCIFlags & MCI_TO) {
	lTo = ConvertToFrames(npMCI, lTo);
    }
    if (dwMCIFlags & MCI_FROM) {
	lFrom = ConvertToFrames(npMCI, lFrom);
    }

    dwRet = InternalPlayStart(npMCI, dwMCIFlags, lTo, lFrom,
	    	npMCI->dwReqCallback);

    if (dwRet || (dwMCIFlags & MCI_TEST)) {
	TaskReturns(npMCI, dwRet);
	return;
    }

    // actually play the file
    wNotify = mciaviPlayFile(npMCI, TRUE);

    // if we stopped to pick up new params without actually completing the
    // the play (OnTask_StopTemporarily) then MCIAVI_UPDATING will be set

    if (! (npMCI->dwFlags & MCIAVI_UPDATING)) {
	// perform any notification
	if (wNotify != MCI_NOTIFY_FAILURE) {
	    GraphicDelayedNotify(npMCI, wNotify);
	}
    }

    return;
}

//
// called to process a play request when play is actually happening.
// if parameters can be adjusted without stopping the current play,
// returns FALSE. Also if the request is rejected (and hEventResponse
// signalled) because of some error, returns FALSE indicating no need to
// stop. Otherwise returns TRUE, so that OnTask_Play() will
// be called after stopping the current play.
BOOL
OnTask_PlayDuringPlay(NPMCIGRAPHIC npMCI)
{

    DWORD dwFlags = npMCI->dwParamFlags;
    LPMCI_DGV_PLAY_PARMS lpPlay = (LPMCI_DGV_PLAY_PARMS)npMCI->lParam;
    long lTo, lFrom;
    DWORD dwRet;


    // since this is a real play request coming from the user we need
    // to show the stage window
    if (dwFlags & (MCI_MCIAVI_PLAY_FULLSCREEN | MCI_MCIAVI_PLAY_FULLBY2)) {
	// do nothing here - handled in fullproc
    } else {
	npMCI->dwFlags |= MCIAVI_NEEDTOSHOW;
    }

    // can be called with null lpPlay (in the resume case)
    // in this case, to and from will be left unchanged
    // if you pass lpPlay, then to and from will be set to defaults even
    // if you don't set MCI_TO and MCI_FROM

    if (lpPlay == NULL) {
	dwFlags &= ~(MCI_TO | MCI_FROM);
    }


    /* Range checks : 0 < 'from' <= 'to' <= last frame */

    if (dwFlags & MCI_TO) {
	lTo = ConvertToFrames(npMCI, lpPlay->dwTo);

        if (lTo < 0L || lTo > npMCI->lFrames) {
	    TaskReturns(npMCI, MCIERR_OUTOFRANGE);
	    return FALSE;
	}
    } else {
	// don't touch to and from for resume
	if (lpPlay) {
	    if (dwFlags & MCI_DGV_PLAY_REVERSE)
		lTo = 0;
	    else
		lTo = npMCI->lFrames;

	    dwFlags |= MCI_TO;
	} else {
	    lTo = npMCI->lTo;
	}
    }


    // if no from setting, then get current position
    if (dwFlags & MCI_FROM) {
	lFrom = ConvertToFrames(npMCI, lpPlay->dwFrom);

        if (lFrom < 0L || lFrom > npMCI->lFrames) {
	    TaskReturns(npMCI, MCIERR_OUTOFRANGE);
	    return FALSE;
	}
    } else if (dwRet = InternalGetPosition(npMCI, &lFrom)) {
	TaskReturns(npMCI, dwRet);
	return FALSE;
    }

    /* check 'to' and 'from' relationship.  */
    if (lTo < lFrom)
	dwFlags |= MCI_DGV_PLAY_REVERSE;

    if ((lFrom < lTo) && (dwFlags & MCI_DGV_PLAY_REVERSE)) {
	TaskReturns(npMCI, MCIERR_OUTOFRANGE);
	return FALSE;
    }

    /* If the test flag is set, return without doing anything. */
    /* Question: do we have to check for more possible errors? */
    if (dwFlags & MCI_TEST) {
	TaskReturns(npMCI, 0L);
	return FALSE;
    }

    /* We want any previous playing to be aborted if and only if a 'from'
    ** parameter is specified.  If only a new 'to' parameter is specified,
    ** we can just change the 'to' value, and play will stop at the
    ** proper time.
    **
    ** Also abort the play if we have lost the audio.  An explicit play
    ** command has been issued and we should try and get the audio again.
    */

    // if it's new from position or we are seeking to the wrong stop,
    // or we are reversing the direction of play,
    // or we had lost the audio
    // then we need to stop.
    if (   (dwFlags & MCI_FROM)
	|| (dwFlags & (MCI_MCIAVI_PLAY_FULLSCREEN | MCI_MCIAVI_PLAY_FULLBY2))
	|| ((npMCI->dwFlags & MCIAVI_SEEKING) && (npMCI->lTo != lFrom))
	|| (npMCI->wTaskState == TASKCUEING)
	|| (npMCI->dwFlags & MCIAVI_LOSTAUDIO)
	|| (((npMCI->dwFlags & MCIAVI_REVERSE) != 0) != ((dwFlags & MCI_DGV_PLAY_REVERSE) != 0))
	) {

	// we can't continue the play - we have to stop, and then pick up
	// this request again in OnTask_Play().

	// this will abort the current notify
	return TRUE;
    }

    // ok to continue the current play with revised params.

    // set the from position correctly
    npMCI->lFrom = lFrom;


    /* If we're changing the "to" position, abort any pending notify. */
    if (lTo != npMCI->lTo) {
	GraphicDelayedNotify (npMCI, MCI_NOTIFY_ABORTED);
    }
	
    /* Don't set up notify until here, so that the seek won't make it happen*/
    if (dwFlags & MCI_NOTIFY) {
        GraphicSaveCallback(npMCI, (HANDLE) (UINT_PTR)npMCI->dwReqCallback);
    }

    // Make sure flags are cleared if they should be
    npMCI->dwFlags &= ~(MCIAVI_PAUSE | MCIAVI_CUEING | MCIAVI_REVERSE | MCIAVI_LOSEAUDIO);

    /* Set up the 'repeat' flags */
    npMCI->dwFlags &= ~(MCIAVI_REPEATING);

    if (dwFlags & MCI_DGV_PLAY_REPEAT) {
	/* If from position isn't given, repeat from either the beginning or
	** end of file as appropriate.
	**
	** if no lpPlay is given, then don't change repeatfrom
	*/

	if (lpPlay) {
	    npMCI->lRepeatFrom =
		(dwFlags & MCI_FROM) ? lFrom :
		    ((dwFlags & MCI_DGV_PLAY_REVERSE) ? npMCI->lFrames : 0);
	}
	npMCI->dwFlags |= MCIAVI_REPEATING;
    }

    /* if not already playing, start the task up. */

    if (lTo > npMCI->lFrames)
        lTo = npMCI->lFrames;

    if (lTo < 0)
        lTo = 0;

    if (dwFlags & MCI_TO)
	npMCI->lTo = lTo;

    if (dwFlags & MCI_DGV_PLAY_REVERSE)
	npMCI->dwFlags |= MCIAVI_REVERSE;

    if (npMCI->dwFlags & MCIAVI_NEEDTOSHOW) {
	ShowStage(npMCI);
	//
	// leave this set so the play code knows this is a "real" play
	// coming from the user, not an internal play/stop
	//
	// if the window needs shown we want to do it here if we can
	// not in the background task.
	//
	npMCI->dwFlags |= MCIAVI_NEEDTOSHOW;
    }


    //everything adjusted - tell user ok and return to playing
    TaskReturns(npMCI, 0);
    return FALSE;

}

void OnTask_Realize(NPMCIGRAPHIC npMCI)
{
    DWORD dw;

    EnterHDCCrit(npMCI);
    dw = InternalRealize(npMCI);
    LeaveHDCCrit(npMCI);
    TaskReturns(npMCI, dw);

}

DWORD InternalRealize(NPMCIGRAPHIC npMCI)
{
    BOOL fGetDC;
    BOOL fPalChanged;
#ifndef _WIN32
    BOOL fAlreadyDoneThat;
#endif

    HDCCritCheckIn(npMCI);
    if (npMCI->dwFlags & MCIAVI_WANTMOVE)
	CheckWindowMove(npMCI, TRUE);

#ifndef _WIN32
    if (fAlreadyDoneThat = (BOOL)(npMCI->dwFlags & MCIAVI_UPDATING)) {
	DPF(("Re-entering InternalRealize - but we don't care, npMCI=%8x\n",npMCI));
    }
#endif

    if (!IsTask(npMCI->hTask))
        return(0L);

    if (fGetDC = (npMCI->hdc == NULL)) {
	npMCI->hdc = GetDC(npMCI->hwndPlayback);
        Assert(npMCI->hdc != NULL);
    }

#ifndef _WIN32
    // this only prevents playback window alignment - which is not done
    // for NT anyway
    npMCI->dwFlags |= MCIAVI_UPDATING;
#endif

    fPalChanged = PrepareDC(npMCI) > 0;

#ifndef _WIN32
    if (!fAlreadyDoneThat)
        npMCI->dwFlags &= ~MCIAVI_UPDATING;
#endif

    if (fGetDC) {
        UnprepareDC(npMCI);
        ReleaseDC(npMCI->hwndPlayback, npMCI->hdc);
	npMCI->hdc = NULL;
	HDCCritCheckIn(npMCI);
    }

    if (fPalChanged)
        InvalidateRect(npMCI->hwndPlayback, &npMCI->rcDest, TRUE);

    CheckIfActive(npMCI);

    return 0L;
}



void OnTask_Update(NPMCIGRAPHIC npMCI)
{
    RECT    rc;
    LPMCI_DGV_UPDATE_PARMS lpParms = (LPMCI_DGV_UPDATE_PARMS) npMCI->lParam;
    DWORD dwFlags = npMCI->dwFlags;
    DWORD dwErr;

    rc.left   = lpParms->ptOffset.x;
    rc.top    = lpParms->ptOffset.y;
    rc.right  = lpParms->ptOffset.x + lpParms->ptExtent.x;
    rc.bottom = lpParms->ptOffset.y + lpParms->ptExtent.y;

    dwErr = Internal_Update (npMCI, dwFlags, lpParms->hDC, (dwFlags & MCI_DGV_RECT) ? &rc : NULL);

    //now, where were we ?
    if (!dwErr && (npMCI->dwFlags & MCIAVI_UPDATING)) {
	OnTask_RestartAgain(npMCI, TRUE);
    } else {
	TaskReturns(npMCI, dwErr);
    }
}



BOOL OnTask_UpdateDuringPlay(NPMCIGRAPHIC npMCI)
{
    RECT    userrc, rc;
    LPMCI_DGV_UPDATE_PARMS lpParms = (LPMCI_DGV_UPDATE_PARMS) npMCI->lParam;
    DWORD dwFlags = npMCI->dwFlags;
    HDC hdc = lpParms->hDC;

    userrc.left   = lpParms->ptOffset.x;
    userrc.top    = lpParms->ptOffset.y;
    userrc.right  = lpParms->ptOffset.x + lpParms->ptExtent.x;
    userrc.bottom = lpParms->ptOffset.y + lpParms->ptExtent.y;

    //
    // mark the proper streams dirty, this will set the proper update flags
    //
    if (hdc)
        GetClipBox(hdc, &rc);
    else
        rc = npMCI->rcDest;

    if (dwFlags & MCI_DGV_RECT)
        IntersectRect(&rc, &rc, &userrc);

    StreamInvalidate(npMCI, &rc);

    //
    // if they are drawing to the screen *assume* they wanted to set
    // the MCI_DGV_UPDATE_PAINT flag
    //
    if (IsScreenDC(hdc))
        dwFlags |= MCI_DGV_UPDATE_PAINT;


    // we are playing now (we have a dc). just realize
    // the palette and set the update flag
    // unless we are painting to a memory dc.
    //
    // if we are paused, fall through so we can handle the case where
    // a update fails
    //
    // !!!mabey we should rework this code to do this even if playing?
    //
    if (npMCI->hdc &&
            (dwFlags & MCI_DGV_UPDATE_PAINT) &&
            (npMCI->wTaskState != TASKPAUSED) &&

            //!!! what is this?
            ((npMCI->wTaskState != TASKCUEING) ||
                (npMCI->lCurrentFrame <= 1) ||
                (npMCI->lCurrentFrame > npMCI->lRealStart - 30)) ) {

        Assert(npMCI->wTaskState == TASKPLAYING ||
               npMCI->wTaskState == TASKCUEING);

	EnterHDCCrit(npMCI);
	UnprepareDC(npMCI);
        PrepareDC(npMCI);  // re-prepare
	LeaveHDCCrit(npMCI);

	// all ok - no need for stop.
	TaskReturns(npMCI, 0);
	return FALSE;
    }

    // try to use DoStreamUpdate - if this fails, we need to stop
    if (TryStreamUpdate(npMCI, dwFlags, hdc,
	(dwFlags & MCI_DGV_RECT) ? &userrc : NULL)) {

	    // we are playing and so have an hdc. However, we have just
	    // done a update to another hdc. switching back to the original
	    // hdc without this will fail
	    PrepareDC(npMCI);

	    TaskReturns(npMCI, 0);
	    return FALSE;
    }

    // otherwise we need to stop to do this

    // indicate that we should restart after doing this, and
    // save enough state to do this
    OnTask_StopTemporarily(npMCI);

    return TRUE;
}


// attempt repaint using DoStreamUpdate - if this fails (eg wrong frame)
// then you need to use mciaviPlayFile to do it (to/from same frame)
BOOL
TryStreamUpdate(
    NPMCIGRAPHIC npMCI,
    DWORD dwFlags,
    HDC hdc,
    LPRECT lprc
)
{
    HDC hdcSave;
    BOOL f;

    //
    // are we updating to a memory bitmap?
    //
    if (!(dwFlags & MCI_DGV_UPDATE_PAINT))
        npMCI->dwFlags |= MCIAVI_UPDATETOMEMORY;

    //
    // if we are using a draw device (or are in stupid mode) make sure we seek
    // to the frame we want and dont use the current decompress buffer that
    // may not be correct.
    //
    if ((npMCI->dwFlags & MCIAVI_UPDATETOMEMORY) ||
        (npMCI->dwFlags & MCIAVI_STUPIDMODE)) {
        DPF(("DeviceUpdate: decompress buffer may be bad, ignoring it....\n"));
	npMCI->lFrameDrawn = (- (LONG) npMCI->wEarlyRecords) - 1;
    }

    //
    // honor the passed rect
    //
    if (lprc) {
	Assert(hdc);
        SaveDC(hdc);
        IntersectClipRect(hdc, lprc->left, lprc->top,
                               lprc->right, lprc->bottom);
    }

    //
    //  Always do an Update, if the update succeeds and we are at the right
    //  frame keep it.
    //
    //  if it fails or the frame is wrong need to re-draw using play.
    //
    //  we need to do this because even though lFrameDrawn is a valid
    //  frame the draw handler may fail a update anyway (for example
    //  when decompressing to screen) so lFrameDrawn can be bogus and
    //  we do not know it until we try it.
    //


    if (npMCI->lFrameDrawn <= npMCI->lCurrentFrame &&
        npMCI->lFrameDrawn >= 0) {

        DPF2(("Update: redrawing frame %ld, current = %ld.\n", npMCI->lFrameDrawn, npMCI->lCurrentFrame));

	/* Save the DC, in case we're playing, but need to update
	** to a memory bitmap.
	*/

	// worker thread must hold critsec round all drawing
        EnterHDCCrit(npMCI);
	hdcSave = npMCI->hdc;
        npMCI->hdc = hdc;

	/* Realize the palette here, because it will cause strange
        ** things to happen if we do it in the task.
        */
	if (npMCI->dwFlags & MCIAVI_NEEDDRAWBEGIN) {
	    DrawBegin(npMCI, NULL);

	    if (npMCI->lFrameDrawn < npMCI->lVideoStart) {
		npMCI->hdc = hdcSave;
		HDCCritCheckIn(npMCI);
		npMCI->lFrameDrawn = (- (LONG) npMCI->wEarlyRecords) - 1;
                LeaveHDCCrit(npMCI);
		return FALSE;	// need to use play
	    }
	}

        PrepareDC(npMCI);        // make sure the palette is in there

        f = DoStreamUpdate(npMCI, FALSE);

        UnprepareDC(npMCI);      // be sure to put things back....
	Assert(hdc == npMCI->hdc);
	HDCCritCheckIn(npMCI);
        npMCI->hdc = hdcSave;
	LeaveHDCCrit(npMCI);

        if (!f) {
            DPF(("DeviceUpdate failed! invalidating lFrameDrawn\n"));
            npMCI->lFrameDrawn = (- (LONG) npMCI->wEarlyRecords) - 1;
	    Assert(!lprc);
        }
        else if (npMCI->lFrameDrawn >= npMCI->lCurrentFrame-1) {
	    if (lprc) {
		RestoreDC(hdc, -1);
	    }
	
	    npMCI->dwFlags &= ~(MCIAVI_UPDATING|MCIAVI_UPDATETOMEMORY);
	
	    if (npMCI->dwFlags & MCIAVI_NEEDUPDATE) {
		DPF(("**** we did a DeviceUpdate but still dirty?\n"));
	    }
	
	    return TRUE;
        }
	//return FALSE;  Drop through
    }

    return FALSE;

}

// called in stopped case to paint from OnTask_Update, and
// also on winproc thread (when stopped). Not called during play.

DWORD Internal_Update(NPMCIGRAPHIC npMCI, DWORD dwFlags, HDC hdc, LPRECT lprc)
{
    DWORD   dwErr = 0L;
    HWND    hCallback;
    HCURSOR hcurPrev;
    RECT    rc;
    LONG lFrameDrawn;


    if (npMCI->dwFlags & MCIAVI_WANTMOVE)
	CheckWindowMove(npMCI, TRUE);

    //
    // see if we are the active movie now.
    //
    CheckIfActive(npMCI);

    //
    // mark the proper streams dirty, this will set the proper update flags
    //
    if (hdc)
        GetClipBox(hdc, &rc);
    else
        rc = npMCI->rcDest;

    if (lprc)
        IntersectRect(&rc, &rc, lprc);

    StreamInvalidate(npMCI, &rc);

    //
    // if they are drawing to the screen *assume* they wanted to set
    // the MCI_DGV_UPDATE_PAINT flag
    //
    if (IsScreenDC(hdc))
        dwFlags |= MCI_DGV_UPDATE_PAINT;

    lFrameDrawn = npMCI->lFrameDrawn;       // save this for compare


    // try to use DoStreamUpdate
    if (TryStreamUpdate(npMCI, dwFlags, hdc, lprc)) {
	return 0;
    }

    // no - need to use Play

    // note we are already stopped at this point.


    //
    // the problem this tries to fix is the following:
    // sometimes we are at N+1 but frame N is on the
    // screen, if we now play to N+1 a mismatch will occur
    //
    if (lFrameDrawn >= 0 && lFrameDrawn == npMCI->lCurrentFrame-1)
	npMCI->lFrom = npMCI->lTo = lFrameDrawn;
    else
	npMCI->lFrom = npMCI->lTo = npMCI->lCurrentFrame;

    /* Realize the palette here, because it will cause strange
    ** things to happen if we do it in the task.
    */
    EnterHDCCrit(npMCI);
    npMCI->hdc = hdc;
    PrepareDC(npMCI);        // make sure the palette is in there
    LeaveHDCCrit(npMCI);

    hcurPrev =  SetCursor(LoadCursor(NULL, IDC_WAIT));

    /* Hide any notification, so it won't get sent... */
    hCallback = npMCI->hCallback;
    npMCI->hCallback = NULL;

    mciaviPlayFile(npMCI, FALSE);

    npMCI->hCallback = hCallback;

    // We may have just yielded.. so only set the cursor back if we
    // are still the wait cursor.
    if (hcurPrev) {
        hcurPrev = SetCursor(hcurPrev);
        if (hcurPrev != LoadCursor(NULL, IDC_WAIT))
            SetCursor(hcurPrev);
    }

    //HDCCritCheckIn(npMCI) ??? This is an atomic operation - and
    //                          why are we setting it to NULL here ??
    npMCI->hdc = NULL;

    if (lprc) {
        RestoreDC(hdc, -1);
    }
    npMCI->dwFlags &= ~(MCIAVI_UPDATETOMEMORY);


    if (npMCI->dwFlags & MCIAVI_NEEDUPDATE) {
        DPF(("**** we did a DeviceUpdate but still dirty?\n"));
    }

    return dwErr;
}


void
OnTask_PauseDuringPlay(NPMCIGRAPHIC npMCI)
{
    DWORD dwFlags = npMCI->dwParamFlags;
    DPF3(("Pause during play\n"));

    // no pause during cueing
    if (npMCI->wTaskState == TASKCUEING) {
	// leave event sent - wait till later
	return;
    }

    // save the notify
    if (dwFlags & MCI_NOTIFY) {
        GraphicSaveCallback(npMCI, (HANDLE) (UINT_PTR)npMCI->dwReqCallback);
    }

    // what about delayed completion pause ?
    // especially "pause" followed by "pause wait"
    if (dwFlags & MCI_WAIT) {
        // indicate hEventAllDone should be set on Pause, not
        // on idle (ie at final stop)
	npMCI->dwFlags |= MCIAVI_WAITING;
    }

    if (npMCI->wTaskState == TASKPAUSED) {
	// all done already
	if (dwFlags & MCI_NOTIFY) {
	    GraphicDelayedNotify(npMCI, MCI_NOTIFY_SUCCESSFUL);
	}

    } else if (npMCI->wTaskState == TASKPLAYING) {

	// remember to pause
        npMCI->dwFlags |= MCIAVI_PAUSE;

	if (dwFlags & MCI_NOTIFY) {
	    // remember to send a notify when we pause
	    npMCI->dwFlags |= MCIAVI_CUEING;
    	}
    }

    TaskReturns(npMCI, 0);
}

void
OnTask_Cue(NPMCIGRAPHIC npMCI, DWORD dwFlags, long lTo)
{
    UINT wNotify;

    DPF3(("OnTask_Cue: dwFlags=%8x, To=%d\n", dwFlags, lTo));

    GraphicDelayedNotify(npMCI, MCI_NOTIFY_ABORTED);

    if (dwFlags & MCI_NOTIFY) {
        GraphicSaveCallback(npMCI, (HANDLE) (UINT_PTR)npMCI->dwReqCallback);
    }

    /* Clear the 'repeat' flags */
    npMCI->dwFlags &= ~(MCIAVI_REPEATING);


    if (dwFlags & MCI_TO) {
	npMCI->lFrom = lTo;
    } else if (npMCI->wTaskState == TASKIDLE) {
	npMCI->lFrom = npMCI->lCurrentFrame;
    }

    /* If we're ever resumed, we want to go to the end of the file. */
    npMCI->lTo = npMCI->lFrames;

    npMCI->dwFlags |= MCIAVI_PAUSE | MCIAVI_CUEING;

    if (dwFlags & MCI_WAIT) {
	npMCI->dwFlags |= MCIAVI_WAITING;
    }

    wNotify = mciaviPlayFile(npMCI, TRUE);

    // if we stopped to pick up new params without actually completing the
    // the play (OnTask_StopTemporarily) then MCIAVI_UPDATING will be set

    if (! (npMCI->dwFlags & MCIAVI_UPDATING)) {
	// perform any notification
	if (wNotify != MCI_NOTIFY_FAILURE) {
	    GraphicDelayedNotify(npMCI, wNotify);
	}
    }
}



BOOL
OnTask_CueDuringPlay(NPMCIGRAPHIC npMCI)
{
    DWORD dw = 0L;
    DWORD dwFlags = npMCI->dwParamFlags;
    long lTo = (LONG) npMCI->lParam;

    DPF3(("OnTask_CueDuringPlay\n"));

    if (npMCI->dwFlags & MCIAVI_SEEKING) {
	/* We're currently seeking, so we have to start again to get audio
	** to work.
	*/
	return TRUE;
    }


    if (dwFlags & MCI_TO) {
	return TRUE;
    }

    /* Clear the 'repeat' flags */
    npMCI->dwFlags &= ~(MCIAVI_REPEATING);

    GraphicDelayedNotify(npMCI, MCI_NOTIFY_ABORTED);

    if (dwFlags & MCI_NOTIFY) {
        GraphicSaveCallback(npMCI, (HANDLE) (UINT_PTR)npMCI->dwReqCallback);
    }

    /* If we're ever resumed, we want to go to the end of the file. */
    npMCI->lTo = npMCI->lFrames;

    if (npMCI->wTaskState == TASKPAUSED) {
	/* We're already paused at the right place, so
	** that means we did it.
	*/
	if (dwFlags & MCI_NOTIFY)
	    GraphicDelayedNotify(npMCI, MCI_NOTIFY_SUCCESSFUL);

	// complete completed
	TaskReturns(npMCI, 0);

	// delayed completion is also done!
	if (dwFlags & MCI_WAIT) {
	    SetEvent(npMCI->hEventAllDone);
	}

	// don't drop through to the second TaskReturns() below!
	return FALSE;

    } else if ((npMCI->wTaskState == TASKCUEING) ||
	    	 (npMCI->wTaskState == TASKPLAYING)) {

	// ask for pause on completion of cueing/playing,
	// and for notify and hEventAllDone when pause reached

	npMCI->dwFlags |= MCIAVI_PAUSE | MCIAVI_CUEING;

	if (dwFlags & MCI_WAIT) {
	    npMCI->dwFlags |= MCIAVI_WAITING;
	}


    } else {
	TaskReturns (npMCI, MCIERR_NONAPPLICABLE_FUNCTION);
	return FALSE;
    }

    TaskReturns(npMCI, 0);
    return FALSE;
}


void OnTask_Seek(NPMCIGRAPHIC npMCI)
{
    UINT wNotify;
    DWORD dwFlags = npMCI->dwParamFlags;
    long lTo = (long) npMCI->lParam;

    DPF3(("DeviceSeek - to frame %d (CurrentFrame==%d)  Current State is %d\n", lTo, npMCI->lCurrentFrame, npMCI->wTaskState));
    /* The window will be shown by the play code. */

    // task state is now TASKIDLE and blocked

    if (dwFlags & MCI_NOTIFY) {
        GraphicSaveCallback(npMCI, (HANDLE) (UINT_PTR)npMCI->dwReqCallback);
    }

    /* Clear the 'repeat' flags */
    npMCI->dwFlags &= ~(MCIAVI_REPEATING);



    if (npMCI->lCurrentFrame != lTo) {

	/* Essentially, we are telling the task: play just frame <lTo>.
	** When it gets there, it will update the screen for us.
	*/
	npMCI->lFrom = npMCI->lTo = lTo;

	wNotify = mciaviPlayFile(npMCI, TRUE);

	// if we stopped to pick up new params without actually completing the
	// the play (OnTask_StopTemporarily) then MCIAVI_UPDATING will be set

	if (! (npMCI->dwFlags & MCIAVI_UPDATING)) {
	    // perform any notification
	    if (wNotify != MCI_NOTIFY_FAILURE) {
		GraphicDelayedNotify(npMCI, wNotify);
	    }
	}

    } else {
	// task complete
	TaskReturns(npMCI, 0);

	/* Be sure the window gets shown and the notify gets sent,
	** even though we don't have to do anything.
	*/
	if (npMCI->dwFlags & MCIAVI_NEEDTOSHOW)
	    ShowStage(npMCI);

	if (dwFlags & MCI_NOTIFY)
	    GraphicDelayedNotify(npMCI, MCI_NOTIFY_SUCCESSFUL);	
    }
}

OnTask_SeekDuringPlay(NPMCIGRAPHIC npMCI)
{
    long lTo = (long) npMCI->lParam;
    DWORD dwFlags = npMCI->dwParamFlags;


    DPF3(("DeviceSeek - to frame %d (CurrentFrame==%d)  Current State is %d\n", lTo, npMCI->lCurrentFrame, npMCI->wTaskState));
    /* The window will be shown by the play code. */


    /* If we can just shorten a previous seek, do it. */
    if ((npMCI->wTaskState == TASKCUEING) &&
	    (npMCI->dwFlags & MCIAVI_SEEKING) &&
	    (npMCI->lCurrentFrame <= lTo) &&
	    (npMCI->lTo >= lTo)) {

	npMCI->lTo = lTo;

	/* Clear the 'repeat' flags */
	npMCI->dwFlags &= ~(MCIAVI_REPEATING);

	GraphicDelayedNotify (npMCI, MCI_NOTIFY_ABORTED);

	if (dwFlags & MCI_NOTIFY) {
	    GraphicSaveCallback(npMCI, (HANDLE) (UINT_PTR)npMCI->dwReqCallback);
	}

	TaskReturns(npMCI, 0);
	return FALSE;
    }

    // we have to stop to do this seek
    return TRUE;
}


void OnTask_SetWindow(NPMCIGRAPHIC npMCI)
{

    npMCI->hwndPlayback = (HWND) npMCI->lParam;

    npMCI->dwFlags |= MCIAVI_NEEDDRAWBEGIN;
    InvalidateRect(npMCI->hwndPlayback, &npMCI->rcDest, FALSE);

    /* Should we update the window here? */

    /* Start playing again in the new window (if we had to stop) */

    //now, where were we ?
    if (npMCI->dwFlags & MCIAVI_UPDATING) {
	OnTask_RestartAgain(npMCI, TRUE);
    } else {
	TaskReturns(npMCI, 0);
    }
}

void OnTask_SetSpeed(NPMCIGRAPHIC npMCI)
{
    npMCI->dwSpeedFactor = (DWORD)npMCI->lParam;

    // if we stopped to do this, then restart whatever we were doing
    if (npMCI->dwFlags & MCIAVI_UPDATING) {
	OnTask_RestartAgain(npMCI, TRUE);
    } else {
	TaskReturns(npMCI, 0);
    }
}


BOOL
OnTask_SetSpeedDuringPlay(NPMCIGRAPHIC npMCI)
{
    /* If new speed is the same as the old speed, done. */
    if ((DWORD)npMCI->lParam == npMCI->dwSpeedFactor) {
	TaskReturns(npMCI, 0);
	return FALSE;
    }

    // otherwise we have to stop and restart
    OnTask_StopTemporarily(npMCI);
    return TRUE;
}


void OnTask_WaveSteal(NPMCIGRAPHIC npMCI) {

    DPF2(("OnTask_WaveSteal, '%ls' hTask=%04X\n", (LPSTR)npMCI->szFilename, npMCI->hTask));

    // if we stopped to do this, then restart whatever we were doing
    if (npMCI->dwFlags & MCIAVI_UPDATING) {
	// We stopped to do this...
        EnterWinCrit(npMCI);

        // Turn the lose audio flag on so that when we restart we do not
	// try and pick up the wave device.  The flag will be reset in
	// SetUpAudio.

        npMCI->dwFlags |= MCIAVI_LOSEAUDIO;

	// Hint that we would like sound again
        npMCI->dwFlags |= MCIAVI_LOSTAUDIO;

        LeaveWinCrit(npMCI);
        OnTask_RestartAgain(npMCI, TRUE);

    	Assert(!(npMCI->dwFlags & MCIAVI_LOSEAUDIO));
	// The flag has been reset by SetUpAudio

	// By using MCIAVI_LOSEAUDIO we do not have to change the state of
	// the MCIAVI_PLAYAUDIO flag.  This is goodness as that flag controls
	// the mute state - and that is independent of the availability of a
	// wave device, activation and/or deactivation.
    } else {
        TaskReturns(npMCI, 0);
    }
}

void OnTask_WaveReturn(NPMCIGRAPHIC npMCI) {

    // Turn off the flag that caused us to get called.
    // Note: if the audio device is still unavailable, this flag will get
    // turned on again when we fail to open the device.
    npMCI->dwFlags &= ~MCIAVI_LOSTAUDIO;

    DPF2(("OnTask_WaveReturn... pick up the audio\n"));
    // if we stopped to do this, then restart whatever we were doing
    if (npMCI->dwFlags & MCIAVI_UPDATING) {
	OnTask_RestartAgain(npMCI, TRUE);
    } else {
	TaskReturns(npMCI, 0);
    }
}

BOOL OnTask_WaveStealDuringPlay(NPMCIGRAPHIC npMCI) {

    DPF2(("OnTask_WaveStealDuringPlay, '%ls' hTask=%04X\n", (LPSTR)npMCI->szFilename, npMCI->hTask));
    /* If we do not have the audio, just return. */
    if (npMCI->hWave == 0) {
	TaskReturns(npMCI, 0);
        return FALSE;
    }

    /* Stop before changing sound status */
    OnTask_StopTemporarily(npMCI);
    return(TRUE);
}

/*
 * A wave device may have become available.  Stop and try to pick it up.
 */

BOOL OnTask_WaveReturnDuringPlay(NPMCIGRAPHIC npMCI) {


    /* If there's no audio, just return. */
    if (npMCI->nAudioStreams == 0) {
        npMCI->dwFlags &= ~MCIAVI_LOSTAUDIO;
	TaskReturns(npMCI, 0);
        return FALSE;
    }

    /* Stop before changing sound status */
    OnTask_StopTemporarily(npMCI);
    return(TRUE);
}

BOOL
OnTask_MuteDuringPlay(NPMCIGRAPHIC npMCI)
{
    // If npMCI->lParam is TRUE, this means that we are to mute the
    // device - hence turn off the PLAYAUDIO flag.

    DWORD fPlayAudio = (DWORD)((BOOL) npMCI->lParam ? 0 : MCIAVI_PLAYAUDIO);

    /* If there's no audio, just return. Should this be an error? */
    if (npMCI->nAudioStreams == 0) {
	TaskReturns(npMCI, 0);
        return FALSE;
    }

    /* If the mute state isn't changing, don't do anything. */
    if ( (npMCI->dwFlags & MCIAVI_PLAYAUDIO) == fPlayAudio) {
        TaskReturns(npMCI, 0);
        return FALSE;
    }

    DPF2(("DeviceMute, fPlayAudio = %x, npMCI=%8x\n", fPlayAudio, npMCI));

    /* Stop before changing mute */
    OnTask_StopTemporarily(npMCI);

    return TRUE;
}


void
OnTask_Mute(NPMCIGRAPHIC npMCI)
{

    // If npMCI->lParam is TRUE, this means that we are to mute the
    // device - hence turn off the PLAYAUDIO flag.
    // We do not bother to check a change in state.  That is only
    // relevant if we are already playing when we only want to stop
    // for a change in state.

    BOOL fMute = (BOOL)npMCI->lParam;

    /* If there's no audio, just return. Should this be an error? */
    if (npMCI->nAudioStreams != 0) {

	EnterWinCrit(npMCI);
        if (fMute)
            npMCI->dwFlags &= ~MCIAVI_PLAYAUDIO;
        else
            npMCI->dwFlags |= MCIAVI_PLAYAUDIO;
    	LeaveWinCrit(npMCI);
    }

    // if we stopped to do this, then restart whatever we were doing
    if (npMCI->dwFlags & MCIAVI_UPDATING) {
	OnTask_RestartAgain(npMCI, TRUE);
    } else {
	TaskReturns(npMCI, 0);
    }
}


// all access to the hWave *must* be restricted to the thread that created
// the wave device. So even getting the volume must be done on the
// worker thread only
//
// this function gets the current volume setting and stores it in
// npMCI->dwVolume

DWORD
InternalGetVolume(NPMCIGRAPHIC npMCI)
{
    DWORD	dw;
    DWORD	dwVolume;

    if (npMCI->hWave) {
	// Get the current audio volume....
	dw = waveOutMessage(npMCI->hWave, WODM_GETVOLUME,
			    (DWORD_PTR) (DWORD FAR *)&dwVolume, 0);

    } else if (!(npMCI->dwFlags & MCIAVI_VOLUMESET)) {
	// We have no device open, and the user hasn't chosen a
	// volume yet.

        //
        // Try to find out what the current "default" volume is.
        //
        // I really doubt zero is the current volume, try to work
        // with broken cards like the stupid windows sound system.
        //
        dw = waveOutGetVolume((HWAVEOUT)(UINT)WAVE_MAPPER, &dwVolume);

        if ((dw != 0) || (dwVolume != 0)) {

	    dw = waveOutGetVolume((HWAVEOUT)0, &dwVolume);
	}

	// don't accept default volume of 0
	if ((dwVolume == 0) && (dw == 0)) {
	    dw = MCIERR_NONAPPLICABLE_FUNCTION;
	}

    }
    if (dw == 0) {
	npMCI->dwVolume = MAKELONG((UINT)muldiv32(LOWORD(dwVolume), 500L, 32768L),
				   (UINT)muldiv32(HIWORD(dwVolume), 500L, 32768L));
    }
    return dw;

}

DWORD
InternalSetVolume(NPMCIGRAPHIC npMCI, DWORD dwVolume)
{
    DWORD dw = 0;

    npMCI->dwVolume = dwVolume;

    EnterWinCrit(npMCI);
    npMCI->dwFlags |= MCIAVI_VOLUMESET;
    LeaveWinCrit(npMCI);

    /* clear flag to emulate volume */;
    npMCI->fEmulatingVolume = FALSE;

    /* If there's no audio, just return. Should this be an error? */
    if (npMCI->nAudioStreams != 0) {

	if (npMCI->hWave) {
	    WORD	wLeft;
	    WORD	wRight;

	    if (LOWORD(dwVolume) >= 1000)
		wLeft = 0xFFFF;
	    else
		wLeft = (WORD) muldiv32(LOWORD(dwVolume), 32768L, 500L);

	    if (HIWORD(dwVolume) >= 1000)
		wRight = 0xFFFF;
	    else
		wRight = (WORD) muldiv32(HIWORD(dwVolume), 32768L, 500L);

	    // !!! Support left and right volume?
	    dw = waveOutMessage(npMCI->hWave, WODM_SETVOLUME,
				MAKELONG(wLeft, wRight), 0);

	    if (dw != MMSYSERR_NOERROR && LOWORD(dwVolume) != 500) {
		npMCI->fEmulatingVolume = TRUE;
		BuildVolumeTable(npMCI);
	    }

	    dw = 0;
	}
    }
    return dw;
}

INLINE void
OnTask_SetVolume(NPMCIGRAPHIC npMCI)
{
    DWORD dwVolume = (DWORD) npMCI->lParam;

    TaskReturns(npMCI, InternalSetVolume(npMCI, dwVolume));
}

void OnTask_SetAudioStream(NPMCIGRAPHIC npMCI)
{
    UINT wAudioStream = npMCI->dwParamFlags;
    int		stream;

    /* If there's no audio, we're done. Should this be an error? */

    if (npMCI->nAudioStreams != 0) {

	for (stream = 0; stream < npMCI->streams; stream++) {
	    if (SH(stream).fccType == streamtypeAUDIO) {
		--wAudioStream;

		if (wAudioStream == 0)
		    break;
	    }
	}

	Assert(stream < npMCI->streams);

	npMCI->psiAudio = SI(stream);
	npMCI->nAudioStream = stream;
    }

    // if we stopped to do this, then restart whatever we were doing
    if (npMCI->dwFlags & MCIAVI_UPDATING) {
	OnTask_RestartAgain(npMCI, TRUE);
    } else {
	TaskReturns(npMCI, 0);
    }
}

void
OnTask_SetVideoStream(NPMCIGRAPHIC npMCI)
{
    UINT uStream = npMCI->dwParamFlags;
    BOOL fOn = (BOOL) npMCI->lParam;
    DWORD	dw = 0L;
    int         stream;
    STREAMINFO *psi;

    //
    // find the Nth non-audio, non-error stream
    //
    for (stream = 0; stream < npMCI->streams; stream++) {

        psi = SI(stream);

        if (psi->sh.fccType == streamtypeAUDIO)
            continue;

        if (psi->dwFlags & STREAM_ERROR)
            continue;

        if (--uStream == 0)
            break;
    }

    if (stream == npMCI->streams) {
        dw = MCIERR_OUTOFRANGE;
    } else {


        if (fOn)
            psi->dwFlags |= STREAM_ENABLED;
        else
            psi->dwFlags &= ~STREAM_ENABLED;

        if (fOn && psi->sh.fccType == streamtypeVIDEO) {
            //!!! should we change the master timebase?
            DOUT("Setting main video stream\n");
#if 0
//
//  the master video stream is too special cased to change!
//
            npMCI->psiVideo = psi;
            npMCI->nVideoStream = stream;
#endif
        }

        if (!fOn && npMCI->nVideoStream == stream) {
            DOUT("Turning off main video stream\n");
            npMCI->dwFlags &= ~MCIAVI_SHOWVIDEO;
        }

        //
        //  now we turn MCIAVI_SHOWVIDEO off if no video/other streams
        //  are enabled.
        //
        npMCI->dwFlags &= ~MCIAVI_SHOWVIDEO;    // assume off.

        for (stream = 0; stream < npMCI->streams; stream++) {

            psi = SI(stream);

            if (psi->sh.fccType == streamtypeAUDIO)
                continue;

            if (psi->dwFlags & STREAM_ERROR)
                continue;

            if (!(psi->dwFlags & STREAM_ENABLED))
                continue;

            // at least one stream is enabled show "video"
            npMCI->dwFlags |= MCIAVI_SHOWVIDEO;
        }

        if (!(npMCI->dwFlags & MCIAVI_SHOWVIDEO))
            DOUT("All streams off\n");
    }

    // if we stopped to do this, then restart whatever we were doing
    if ( (dw == 0) && (npMCI->dwFlags & MCIAVI_UPDATING)) {
	OnTask_RestartAgain(npMCI, TRUE);
    } else {
	TaskReturns(npMCI, dw);
    }

}

/***************************************************************************
 *
 ***************************************************************************/

static void MapRect(RECT *prc, RECT*prcIn, RECT *prcFrom, RECT *prcTo)
{
    if (IsRectEmpty(prcFrom)) {
        SetRectEmpty(prc);
    }
    else {
        prc->left  = prcTo->left + MulDiv(prcIn->left  - prcFrom->left, prcTo->right  - prcTo->left, prcFrom->right  - prcFrom->left);
        prc->top   = prcTo->top  + MulDiv(prcIn->top   - prcFrom->top,  prcTo->bottom - prcTo->top,  prcFrom->bottom - prcFrom->top);
        prc->right = prcTo->left + MulDiv(prcIn->right - prcFrom->left, prcTo->right  - prcTo->left, prcFrom->right  - prcFrom->left);
        prc->bottom= prcTo->top  + MulDiv(prcIn->bottom- prcFrom->top,  prcTo->bottom - prcTo->top,  prcFrom->bottom - prcFrom->top);
    }
}

/***************************************************************************
 *
 ***************************************************************************/

static void MapStreamRects(NPMCIGRAPHIC npMCI)
{
    int i;

    //
    //  now set the source and dest rects for each stream.
    //
    for (i=0; i<npMCI->streams; i++)
    {
        //
        // make sure the stream rect is in bounds
        //

        IntersectRect(&SI(i)->rcSource, &SH(i).rcFrame, &npMCI->rcSource);

        //
        // now map the stream source rect onto the destination
        //
        MapRect(&SI(i)->rcDest, &SI(i)->rcSource, &npMCI->rcSource, &npMCI->rcDest);
	
        //
        // make the stream source RECT (rcSource) relative to the
        // stream rect (rcFrame)
        //
        OffsetRect(&SI(i)->rcSource,-SH(i).rcFrame.left,-SH(i).rcFrame.top);
	
    }
}

//
// try to set the dest or source rect without stopping play.
// called both at stop time and at play time
//
// returns TRUE if stop needed, or else FALSE if all handled.
// lpdwErr is set to a non-zero error if any error occured (in which case
// FALSE will be returned.
//
BOOL
TryPutRect(NPMCIGRAPHIC npMCI, DWORD dwFlags, LPRECT lprc, LPDWORD lpdwErr)
{

    RECT    rc;
    PRECT   prcPut;
    DWORD   dw = 0;


    // assume no error
    *lpdwErr = 0;

    if (dwFlags & MCI_DGV_PUT_DESTINATION) {
        DPF2(("DevicePut: destination [%d, %d, %d, %d]\n", *lprc));
        prcPut = &npMCI->rcDest;
    } else {
        DPF2(("DevicePut: source [%d, %d, %d, %d]\n", *lprc));
        prcPut = &npMCI->rcSource;

        //
        // make sure source rectangle is in range.
        //
        //  !!!should we return a error, or just fix the rectangle???
        //
	// ?? Why do we use an intermediate structure?
        rc = npMCI->rcMovie;
        IntersectRect(lprc, &rc, lprc);     // fix up the passed rect.
    }

    //
    // check for a bogus rect. either a NULL or inverted rect is considered
    // invalid.
    //
    // !!!NOTE we should handle a inverted rect (mirrored stretch)
    //
    if (lprc->left >= lprc->right ||
        lprc->top  >= lprc->bottom) {
		
	// this is fine if there are no video streams
	if (npMCI->nVideoStreams <= 0) {
	    // no video so all ok
	    return FALSE;
	}

        DPF2(("DevicePut: invalid rectangle [%d, %d, %d, %d]\n", *lprc));
	*lpdwErr = MCIERR_OUTOFRANGE;
	return FALSE;
    }

    /* make sure the rect changed */
    if (EqualRect(prcPut,lprc)) {
	return FALSE;
    }

    InvalidateRect(npMCI->hwndPlayback, &npMCI->rcDest, TRUE);
    rc = *prcPut;           /* save it */
    *prcPut = *lprc;        /* change it */
    InvalidateRect(npMCI->hwndPlayback, &npMCI->rcDest, FALSE);

    /* have both the dest and source been set? */
    if (IsRectEmpty(&npMCI->rcDest) || IsRectEmpty(&npMCI->rcSource)) {
	return FALSE;
    }

    MapStreamRects(npMCI);
    StreamInvalidate(npMCI, NULL);      // invalidate the world

    if (npMCI->wTaskState == TASKIDLE) {
	DPF2(("TryPutRect: Idle, force DrawBegin on update\n"));
	npMCI->dwFlags |= MCIAVI_NEEDDRAWBEGIN;
    }
    else {
	BOOL	fRestart;
	
        //
        //  we dont need to start/stop just begin again.
        //
	DPF2(("TryPutRect: Calling DrawBegin()\n"));
	if (!DrawBegin(npMCI, &fRestart)) {
	    *lpdwErr =  npMCI->dwTaskError;
	    return FALSE;
	}

        if (!DoStreamUpdate(npMCI, FALSE)) {
	    DPF(("TryPutRect: Failed update, forcing restart....\n"));
	    npMCI->lFrameDrawn = (- (LONG) npMCI->wEarlyRecords) - 1;
	    fRestart = TRUE;
	}
	
	if (fRestart) {
	    // restart needed
	    return TRUE;
        }
    }

    // all ok
    return FALSE;
}

void
OnTask_Put(NPMCIGRAPHIC npMCI)
{

    DWORD dwFlags = npMCI->dwParamFlags;
    LPRECT lprc = (LPRECT) npMCI->lParam;
    DWORD dw = 0;
	
	//If the user is doing an MCI_PUT to set the rectangle we should
	//stop any previous requests to set the rectangle.
	npMCI->dwWinProcRequests &= ~WINPROC_RESETDEST;

    if (TryPutRect(npMCI, dwFlags, lprc, &dw)) {

	// what to do now? It says we need to stop, but we
	// are stopped.
	TaskReturns(npMCI, MCIERR_DEVICE_NOT_READY);
	return;
    }

    // if we stopped to do this, then restart whatever we were doing
    if ((dw == 0) && (npMCI->dwFlags & MCIAVI_UPDATING)) {

	// !!! We used to call InitDecompress here...
	npMCI->dwFlags |= MCIAVI_NEEDDRAWBEGIN;

	OnTask_RestartAgain(npMCI, TRUE);
    } else {
	TaskReturns(npMCI, dw);
    }
}

BOOL
OnTask_PutDuringPlay(NPMCIGRAPHIC npMCI)
{
    DWORD dwFlags = npMCI->dwParamFlags;
    LPRECT lprc = (LPRECT) npMCI->lParam;
    DWORD dw = 0;
	

    if (TryPutRect(npMCI, dwFlags, lprc, &dw)) {

	// need to stop to handle this one.

	// !!! Set a flag here to prevent any more drawing
	npMCI->fNoDrawing = TRUE;

	OnTask_StopTemporarily(npMCI);
	return TRUE;
    }

    // handled ok or error - no stop needed
    TaskReturns(npMCI, dw);
    return FALSE;
}

void OnTask_Palette(NPMCIGRAPHIC npMCI)
{
    HPALETTE hpal = (HPALETTE)npMCI->lParam;

    // Remember this for later.

    npMCI->hpal = hpal;

    npMCI->dwFlags |= MCIAVI_NEEDDRAWBEGIN;
    InvalidateRect(npMCI->hwndPlayback, NULL, TRUE);

    // !!! Do we need to stop and restart here?
    // Answer: probably not, because if they care about the palette not being
    // messed up, they probably never allowed us to be shown at all.

    TaskReturns(npMCI, 0);
    return;
}

void OnTask_PaletteColor(NPMCIGRAPHIC npMCI)
{
    DWORD index = (DWORD)npMCI->lParam;
    DWORD color = (DWORD)npMCI->dwParamFlags;

    // !!! Do we need to stop and restart here?
    // Answer: probably not, because if they care about the palette not being
    // messed up, they probably never allowed us to be shown at all.
    // Note: chicago code does stop... but they stop for most things.
    //	(it would be cleaner to stop and restart...)

    // Pound the new color into the old format.
    ((DWORD FAR *) ((BYTE FAR *) npMCI->pbiFormat +
		   npMCI->pbiFormat->biSize))[index] = color;

    ((DWORD FAR *) npMCI->argb)[index] = color;

    npMCI->dwFlags |= MCIAVI_NEEDDRAWBEGIN;
    InvalidateRect(npMCI->hwndPlayback, NULL, TRUE);
    TaskReturns(npMCI, 0);
    return;
}


/*
 * OnTask_ProcessRequest
 *
 * Process a request on the worker thread. Set hEventResponse if completed
 * in error, or once the request has been completed (in the case of async
 * requests such as play, set the event once play has begun ok
 *
 * return TRUE if it is time for the thread to exit, or else false.
 *
 */
BOOL
OnTask_ProcessRequest(NPMCIGRAPHIC npMCI)
{
    switch(npMCI->message) {

    case AVI_CLOSE:
	// release the requesting thread so he can go and wait on the
	// worker thread exit
	TaskReturns(npMCI, 0);

	// now go and exit
	return TRUE;

    case AVI_RESUME:
	// same as play, except that repeat and reverse flags need
	// to be set on worker thread
	npMCI->dwParamFlags |=
	    ((npMCI->dwFlags & MCIAVI_REVERSE)? MCI_DGV_PLAY_REVERSE : 0);
	// fall through
    case AVI_PLAY:
	OnTask_Play(npMCI);
	break;

    case AVI_STOP:
	npMCI->dwFlags &= ~MCIAVI_REPEATING;  // Give the wave device away
	TaskReturns(npMCI, 0);
	break;

    case AVI_REALIZE:
	OnTask_Realize(npMCI);
	break;

    case AVI_UPDATE:
	OnTask_Update(npMCI);
	break;

    case AVI_PAUSE:
	// not playing, so same as cue to current frame
	OnTask_Cue(npMCI, npMCI->dwParamFlags | MCI_TO, npMCI->lCurrentFrame);
	break;

    case AVI_CUE:
	OnTask_Cue(npMCI, npMCI->dwParamFlags, (LONG) npMCI->lParam);
	break;

    case AVI_SEEK:
	OnTask_Seek(npMCI);
	break;

    case AVI_WINDOW:
	OnTask_SetWindow(npMCI);
	break;

    case AVI_MUTE:
	OnTask_Mute(npMCI);
	break;

    case AVI_SETSPEED:
	OnTask_SetSpeed(npMCI);
	break;

    case AVI_SETVOLUME:
	OnTask_SetVolume(npMCI);
	break;

    case AVI_GETVOLUME:
	TaskReturns(npMCI, InternalGetVolume(npMCI));
	break;

    case AVI_AUDIOSTREAM:
	OnTask_SetAudioStream(npMCI);
	break;

    case AVI_VIDEOSTREAM:
	OnTask_SetVideoStream(npMCI);
	break;

    case AVI_PUT:
	OnTask_Put(npMCI);
	break;

    case AVI_PALETTE:
	OnTask_Palette(npMCI);
	break;

    case AVI_PALETTECOLOR:
	OnTask_PaletteColor(npMCI);
	break;

    case AVI_WAVESTEAL:
	OnTask_WaveSteal(npMCI);
	break;

    case AVI_WAVERETURN:
	OnTask_WaveReturn(npMCI);
	break;

    default:
	TaskReturns(npMCI, MCIERR_UNSUPPORTED_FUNCTION);
	break;
    }
    return FALSE;
}


// OnTask_PeekRequest
//
// called from aviTaskCheckRequests() to process a message at play time.
// if the message requires a stop, then this function returns TRUE and
// leaves the message unprocessed.
//
// otherwise the message is fully processed. This must include resetting
// hEventSend
//
INLINE STATICFN BOOL
OnTask_PeekRequest(NPMCIGRAPHIC npMCI)
{
    switch(npMCI->message) {

// always need to stop
    case AVI_CLOSE:
    case AVI_STOP:
	npMCI->dwFlags &= ~MCIAVI_REPEATING;  // Give the wave device away
	return TRUE;


// may need to stop

    case AVI_RESUME:
	// same as play, except that repeat and reverse flags need
	// to be set on worker thread
	npMCI->dwParamFlags |=
	    ((npMCI->dwFlags & MCIAVI_REPEATING)? MCI_DGV_PLAY_REPEAT : 0) |
	    ((npMCI->dwFlags & MCIAVI_REVERSE)? MCI_DGV_PLAY_REVERSE : 0);
	// fall through
    case AVI_PLAY:
	return OnTask_PlayDuringPlay(npMCI);

    case AVI_UPDATE:
	return OnTask_UpdateDuringPlay(npMCI);

    case AVI_SEEK:
	return OnTask_SeekDuringPlay(npMCI);

    case AVI_CUE:
	return OnTask_CueDuringPlay(npMCI);

    case AVI_MUTE:
	return OnTask_MuteDuringPlay(npMCI);

    case AVI_WAVESTEAL:
	return OnTask_WaveStealDuringPlay(npMCI);

    case AVI_WAVERETURN:
	return OnTask_WaveReturnDuringPlay(npMCI);

    case AVI_SETSPEED:
	return OnTask_SetSpeedDuringPlay(npMCI);

    case AVI_PUT:
	return OnTask_PutDuringPlay(npMCI);


// need temporary stop
    case AVI_WINDOW:
    case AVI_AUDIOSTREAM:
    case AVI_VIDEOSTREAM:
	OnTask_StopTemporarily(npMCI);
	return TRUE;


// never need to stop
    case AVI_REALIZE:
	OnTask_Realize(npMCI);
	break;

    case AVI_PAUSE:
	OnTask_PauseDuringPlay(npMCI);
	break;

    case AVI_SETVOLUME:
	OnTask_SetVolume(npMCI);
	break;

    case AVI_GETVOLUME:
	TaskReturns(npMCI, InternalGetVolume(npMCI));
	break;

    case AVI_PALETTE:
	OnTask_Palette(npMCI);
	break;

    case AVI_PALETTECOLOR:
        OnTask_PaletteColor(npMCI);
        break;

    default:
	TaskReturns(npMCI, MCIERR_UNSUPPORTED_FUNCTION);
	break;
    }
    return FALSE;
}

/*
 * This routine is called from the IDLE loop and at key points while
 * playing.  If it is possible to service the request, the state of the
 * device is updated and the request flag is cleared.
 *
 * If the request cannot be handled now (e.g. while playing) the flag
 * is not set and we will be called again (e.g. when idle).
 *
 * If we need to stop to service the request (i.e. to regain the sound
 * device) we return TRUE.  In all other cases we return FALSE.  The
 * return value is only checked if we are actually playing.
 */

STATICFN void OnTask_WinProcRequests(NPMCIGRAPHIC npMCI, BOOL bPlaying)
{

    DWORD requests;
    EnterWinCrit(npMCI);

    // grab the request bits now, so we don't need to hold the
    // critsec while servicing them.
    // any that are not cleared will be or-ed back in at the end
    requests = npMCI->dwWinProcRequests;
    npMCI->dwWinProcRequests = 0;
    LeaveWinCrit(npMCI);


    if (requests & WINPROC_STOP) {
	requests &= ~WINPROC_STOP;
	npMCI->dwFlags |= MCIAVI_STOP;
    }

    if (requests & WINPROC_MUTE) {
	if (bPlaying) {
	    OnTask_StopTemporarily(npMCI);
	} else {
	    // toggle audio flag
	    npMCI->dwFlags ^= MCIAVI_PLAYAUDIO;
	    requests &= ~WINPROC_MUTE;
	}
    }

    if (requests & WINPROC_SOUND) {
	// We might be able to pick up the sound.  This is only of interest
	// if we are currently playing, do not have a sound device, and want
	// the audio.
	if (bPlaying && (NULL == npMCI->hWave) && (MCIAVI_PLAYAUDIO & npMCI->dwFlags)) {
	    OnTask_StopTemporarily(npMCI);
	} else {
	    // We have finished this request.  Make sure we try and
	    // get sound when we restart
	    requests &= ~WINPROC_SOUND;
	    npMCI->dwFlags &= ~MCIAVI_LOSEAUDIO;
	}
    }

#ifdef REMOTESTEAL
    if (requests & WINPROC_SILENT) {
	extern HWND hwndWantAudio;
	DPF2(("WINPROC_SILENT request made, bPlaying=%x\n", bPlaying));
	// If we are playing, and we have a wave device, stop.
	// When we are recalled, we will start again without the wave device.
	if (bPlaying && npMCI->hWave) {
	    OnTask_StopTemporarily(npMCI);
	    // Stopping will cause the wave device to be released, which
	    // means a message will be posted to whoever wanted the wave
	    // device
	} else {
	    // If we are playing, we do not have a wave device, and we
	    // do not want to stop.
	    // Otherwise we want to lose our wave device.
	    // Either way, we will finish with WINPROC_SILENT on this pass
	    requests &= ~WINPROC_SILENT;
	    hwndWantAudio = 0;  // In case we did not have to stop
	    if (!bPlaying) {
		// Remember we lost audio, and start again without audio
		npMCI->dwFlags |= MCIAVI_LOSTAUDIO;
		npMCI->dwFlags |= MCIAVI_LOSEAUDIO;
	    }
	}
    }

#endif

    if (requests & WINPROC_RESETDEST) {

	RECT rc;
	DWORD dw;

	if (npMCI->hwndPlayback &&
	    npMCI->hwndPlayback == npMCI->hwndDefault &&
	    (npMCI->dwOptionFlags & MCIAVIO_STRETCHTOWINDOW)) {
		GetClientRect(npMCI->hwndPlayback, &rc);
	} else if (npMCI->streams > 0) {
	    rc = npMCI->rcMovie;

	    if (npMCI->dwOptionFlags & MCIAVIO_ZOOMBY2) {
		rc.right *= 2;
		rc.bottom *= 2;
	    }
	}

	if (TryPutRect(npMCI, MCI_DGV_PUT_DESTINATION, &rc, &dw) && bPlaying) {
	    OnTask_StopTemporarily(npMCI);
	} else {
	    requests &= ~WINPROC_RESETDEST;
	}
    }

    if (requests & WINPROC_ACTIVE) {

	// We are being made active.  The only extra work we must do
	// is grab the wave device - if we have ever lost it.

	// If we are playing, and we do not have the audio, and we want
	// the audio...
	if (bPlaying
	    && (npMCI->hWave == 0)
            && (npMCI->dwFlags & MCIAVI_PLAYAUDIO)) {

	    // Let's try and make the sound active by stealing the wave device
	    // Must stop before trying to reset the sound
	    if (StealWaveDevice(npMCI)) {

		OnTask_StopTemporarily(npMCI);
		// Force ourselves to be called again.  Doing it this way
		// means that we will be recalled.  We cannot rely on
		// WINPROC_ACTIVE staying around.  A deactivation could
		// cause the flag to be cleared
	        requests |= WINPROC_SOUND;
	    }
	} else {
	    // We had not lost the wave device...
	    // OR we are playing silently, and so there is no point
	    // in trying to steal it.
	    // We are finished.
	}

	// Clear WINPROC_ACTIVE - all processing done.
	// Note: we might have set WINPROC_SOUND, which will cause this
	// routine to be recalled.  Once recalled, then playing can restart
	requests &= ~ WINPROC_ACTIVE;

    } else {  // We never have both INACTIVE and ACTIVE at the same time
        if (requests & WINPROC_INACTIVE) {
	    //!!!need to support this
	    requests &= ~WINPROC_INACTIVE;
        }
    }

    EnterWinCrit(npMCI);     // Do we really need this one here??

    if (requests & WINPROC_UPDATE) {
	if (bPlaying) {
	    npMCI->dwFlags |= MCIAVI_NEEDUPDATE;
	} else {

	    HDC hdc;

	    // don't do this if the window is now hidden
	    // or showstage will be called with the critsec and deadlock
	    if (IsWindowVisible(npMCI->hwndPlayback)) {
		EnterHDCCrit(npMCI);
		npMCI->bDoingWinUpdate = TRUE;

		hdc = GetDC(npMCI->hwndPlayback);
		Assert(hdc);
		Internal_Update(npMCI, MCI_DGV_UPDATE_PAINT, hdc, NULL);
		ReleaseDC(npMCI->hwndPlayback, hdc);

		npMCI->bDoingWinUpdate = FALSE;
		LeaveHDCCrit(npMCI);
	    }
	}
	requests &= ~WINPROC_UPDATE;
    }

    if (requests & WINPROC_REALIZE) {
	EnterHDCCrit(npMCI);
	InternalRealize(npMCI);
	LeaveHDCCrit(npMCI);
	requests &= ~ WINPROC_REALIZE;
    }

    // or back the bits we didn't clear
    npMCI->dwWinProcRequests |= requests;

    // if we processed all the bits (and no new bits were set)
    if (! npMCI->dwWinProcRequests) {
	ResetEvent(npMCI->heWinProcRequest);
    }

    LeaveWinCrit(npMCI);
}


/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api void| aviTaskCheckRequests | called on the worker thread at least once per
 *
 * frame. We use this to check for requests from the user thread.
 *
 *
 ***************************************************************************/

void NEAR PASCAL aviTaskCheckRequests(NPMCIGRAPHIC npMCI)
{
    HANDLE hWaiter;

    if (WaitForSingleObject(npMCI->hEventSend, 0) == WAIT_OBJECT_0) {

	// there is a request

	Assert(npMCI->message != 0);

        // check the waiter

        // if this is an async request with wait, we need to set hWaiter
        // so that hEventAllDone is set correctly. If we stop
        // and process this message in the idle loop, then we don't want to
        // set hWaiter here, or EventAllDone could be signalled when we
        // stop - before we've even started this request.

        // so we need to check the validity (no waiting if another thread is
        // waiting) and pick up the waiter and bDelayed while the critsec
        // is still held, but only set hWaiter if the request was processed
        // here.

        // no - that leaves a timing window when hWaiter is not set and the
        // critsec is not held. Set hWaiter, but be prepared to unset it
        // if we postpone processing during the idle loop (in which case,
        // the waiter will hold the critsec until we have stopped).

        hWaiter = npMCI->hWaiter;

        if (npMCI->bDelayedComplete) {

            if (npMCI->hWaiter && (npMCI->hWaiter != npMCI->hRequestor)) {
                TaskReturns(npMCI, MCIERR_DEVICE_NOT_READY);
                return;
            } else {
		DPF2(("Replacing hWaiter in aviTaskCheckRequests... was %x, now %x\n", npMCI->hWaiter, npMCI->hRequestor));
                npMCI->hWaiter = npMCI->hRequestor;
            }
        }

	DPF2(("peek %d [%x] ...", npMCI->message, npMCI->hRequestor));

	if (OnTask_PeekRequest(npMCI)) {
	    // we need to stop

	    // must be set on WORKER THREAD ONLY
	    npMCI->dwFlags |= MCIAVI_STOP;
	    DPF2(("Need to stop - replacing hWaiter (was %x, now %x)\n", npMCI->hWaiter, hWaiter));

            // replace hWaiter so idle loop does not set hEventAllDone for
            // a request he has not yet started.
            npMCI->hWaiter = hWaiter;
	}
	// else the request has already been dealt with
    }

    // did the winproc have any requests
    if (WaitForSingleObject(npMCI->heWinProcRequest, 0) == WAIT_OBJECT_0) {

	//
	// We have a request from the window thread.  Go process it
	//
	OnTask_WinProcRequests(npMCI, TRUE);
    }
}


/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api DWORD | CheckIfActive | check to see if we are the active movie
 *
 * @parm NPMCIGRAPHIC | npMCI | Pointer to instance data block.
 *
 * @rdesc 0 means OK, otherwise mci error
 *
 ***************************************************************************/

void CheckIfActive(NPMCIGRAPHIC npMCI)
{
    BOOL fActive;
    HWND hwndA;


    if (!IsTask(npMCI->hTask)) return;

    //
    //  are we the foreground window?
    //
    //  ??? should the value of <npMCI->fForceBackground> matter?
    //
    //  IMPORTANT:  This does NOT work under NT.  The best that can
    //  be done is to check GetForegroundWindow
#ifndef _WIN32
    hwndA = GetActiveWindow();

    fActive = (hwndA == npMCI->hwndPlayback) ||
              (GetFocus() == npMCI->hwndPlayback) ||
              (IsWindow(hwndA) && IsChild(hwndA, npMCI->hwndPlayback) && !npMCI->fForceBackground);
#else
    hwndA = GetForegroundWindow();

    fActive = (hwndA == npMCI->hwndPlayback) ||
              (IsWindow(hwndA) && IsChild(hwndA, npMCI->hwndPlayback) && !npMCI->fForceBackground);
#endif

    DeviceSetActive(npMCI, fActive);
}

