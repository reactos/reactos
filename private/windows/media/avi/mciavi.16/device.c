/******************************************************************************

   Copyright (C) Microsoft Corporation 1985-1991. All rights reserved.

   Title:   device.c - Multimedia Systems Media Control Interface
            driver for AVI.

*****************************************************************************/
#include "graphic.h"
#include "avitask.h"

#define ALIGNULONG(i)     ((i+3)&(~3))                  /* ULONG aligned ! */
#define WIDTHBYTES(i)     ((unsigned)((i+31)&(~31))/8)  /* ULONG aligned ! */
#define DIBWIDTHBYTES(bi) (DWORD)WIDTHBYTES((int)(bi).biWidth * (int)(bi).biBitCount)

#ifdef WIN32
/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api void | TaskWaitComplete | wait for a task to complete
 *
 ***************************************************************************/

void TaskWaitComplete(NPMCIGRAPHIC npMCI)
{
    LONG lCount;

    /*
    ** Release the critical section so that the task can complete!
    */

    lCount = npMCI->lCritRefCount;
    npMCI->lCritRefCount = 0;
    LeaveCriticalSection(&npMCI->CritSec);

    /*
    ** Use the handle given to us when we created the task to wait
    ** for the thread to complete
    */

    WaitForSingleObject(npMCI->hThreadTermination, INFINITE);
    CloseHandle(npMCI->hThreadTermination);

    /*
    ** Restore our critical section state
    */

    EnterCriticalSection(&npMCI->CritSec);
    npMCI->lCritRefCount = lCount;
}
#endif

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api void | TaskWait | wait for a task state
 *      background task.
 *
 ***************************************************************************/

DWORD mciaviTaskWait(NPMCIGRAPHIC npMCI, int state, BOOL fMciWait)
{
#ifdef WIN32
    long lCount;
    MSG msg;
#endif
    DWORD	dwStartWaitTime = timeGetTime();
// #define TIMEOUT_VALUE        10000L

#ifndef WIN32
    Assert(npMCI->hTask != GetCurrentTask());
#endif

    /*
    ** either wait for a state (state > 0) or wait for not state (state < 0)
    **
    ** !!! if we want to timeout this is the place to do it.
    **
    ** !!! Should we put up a wait cursor here?
    */
    while (state < 0
        ? (int)npMCI->wTaskState == -state
        : (int)npMCI->wTaskState != state)
    {
        if (!IsTask(npMCI->hTask)) {
            npMCI->dwTaskError = MCIERR_DEVICE_NOT_READY;
            return MCIERR_DEVICE_NOT_READY;
        }

#ifdef WIN32
        /*
         * Critical sections:
         *
         * We hold a critical section around the whole of the
         * WinProc (among other things). The owning thread can re-enter
         * a critical section, but needs to Leave the same number of times.
         *
         * When yielding here, we need to release the critical section. To avoid
         * the problem of multiple entries, EnterCrit is a macro that
         * increments an entry count (protected by the critical section), and
         * only goes one level into the critsec. Correspondingly,
         * LeaveCrit decrements the count and only actually leaves if
         * the count reaches 0.
         *
         * Here, however, we need to actually Enter and Leave regardless
         * of the count, so that we release the critical section
         * to other threads during the yield. Thus we don't use the macro,
         * and we also save/restore the critsec count so someone else
         * getting the critsec while we are yielding will behave correctly.
         */

        lCount = npMCI->lCritRefCount;
        npMCI->lCritRefCount = 0;
        LeaveCriticalSection(&npMCI->CritSec);

       /*
        *  Sleep for > 0 because this thread may be at a higher priority than
        *  then thread actually playing the AVI because of the use of
        *  SetThreadPriorityBackground  and Sleep(0) only relinquishes
        *  the remainder of the time slice if another thread of the SAME
        *  PRIORITY is waiting to run.
        */

        Sleep(10);
#else
//      DirectedYield(npMCI->hTask);
        Yield();
#endif

#ifdef WM_AVISWP
        if (TRUE)
#else
        if (fMciWait)
#endif
        {
#ifdef WIN32
            /*
             * if it's safe to yield, it's safe to poll
             * messages fully. This way, we will be absolutely
             * sure of getting the async size messages etc
             */
            //aviTaskYield();
            // it clearly is not safe at this point, since this
            // yield can cause mci to close the driver, leaving us
            // with nowhere to return to.
            // handling messages for our own window is safe and should have the
            // desired effect.
#ifdef WM_AVISWP
            if (npMCI->hwnd) {
                if (PeekMessage(&msg, npMCI->hwnd, WM_AVISWP, WM_AVISWP, PM_REMOVE))
                    DispatchMessage(&msg);
            }
#endif

#endif

            if (fMciWait && mciDriverYield(npMCI->wDevID)) {
#ifdef WIN32
                EnterCriticalSection(&npMCI->CritSec);
                npMCI->lCritRefCount = lCount;
#endif
		break;
	    }
        } else {
#ifdef TIMEOUT_VALUE	
	    if (timeGetTime() > dwStartWaitTime + TIMEOUT_VALUE) {
		Assert(0);
		npMCI->dwTaskError = MCIERR_DEVICE_NOT_READY;
#ifdef WIN32
                EnterCriticalSection(&npMCI->CritSec);
                npMCI->lCritRefCount = lCount;
#endif
		return MCIERR_DEVICE_NOT_READY;
	    }
#endif	
	}
#ifdef WIN32
        EnterCriticalSection(&npMCI->CritSec);
        npMCI->lCritRefCount = lCount;
#endif
    }
    return 0L;
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api DWORD | mciaviTaskMessage | this function sends a message to the
 *      background task.
 *
 ***************************************************************************/

DWORD mciaviTaskMessage(NPMCIGRAPHIC npMCI, int msg)
{
    if (!IsTask(npMCI->hTask)) {
	npMCI->dwTaskError = MCIERR_DEVICE_NOT_READY;
	return npMCI->dwTaskError;
    }

    if (GetCurrentTask() == npMCI->hTask) {
        mciaviMessage(npMCI, msg);
        return npMCI->dwTaskError;
    }

    if (npMCI->wTaskState == TASKPAUSED) {
        DPF(("Ack! message while PAUSED!\n"));
	return 1; // !!! Real error?
    }

#ifdef DEBUG
    if (npMCI->wTaskState != TASKIDLE) {
        DPF0(("Unknown task state (mciaviTaskMessage) %d\n", npMCI->wTaskState));
    }
    Assert(npMCI->wTaskState == TASKIDLE);
#endif


    if (mciaviTaskWait(npMCI, TASKIDLE, FALSE) != 0) {
        DPF(("Error waiting for TASKIDLE in mciaviTaskMessage\n"));
        return npMCI->dwTaskError;
    }

    npMCI->dwTaskError = 0L;
    npMCI->wTaskState = msg;
    mmTaskSignal(npMCI->hTask);

    /*
    ** wait for the message to kick in
    */
    mciaviTaskWait(npMCI, -msg, FALSE);

    return npMCI->dwTaskError;
}

DWORD NEAR PASCAL StopTemporarily(NPMCIGRAPHIC npMCI, TEMPORARYSTATE FAR * ps)
{
    DWORD   dw;
    HWND    hCallback;

    DPF2(("StopTemporarily: stopping from state %u.\n", npMCI->wTaskState));

    Assert(ps);

    ps->wOldTaskState = npMCI->wTaskState;
    ps->dwFlags = npMCI->dwFlags;
    ps->lTo = npMCI->lTo - (LONG)npMCI->dwBufferedVideo;
    ps->lFrom = npMCI->lFrom;

    //
    // setting MCIAVI_UPDATING will make sure we dont yield or do
    // other wierd things unless we need to.  it is a bad name for the
    // flag I know I am sorry.
    //
    // it means we are stoping temporarily and will be restarted
    // the code will not do silly things like give our wave device
    // away or become the active window.
    //
    npMCI->dwFlags |= MCIAVI_UPDATING;

    /* Hide the delayed notification, if any, so it doesn't happen now. */
    hCallback = npMCI->hCallback;
    npMCI->hCallback = NULL;

    dw = DeviceStop(npMCI, MCI_WAIT);

    /* Restore the notification */
    npMCI->hCallback = hCallback;

    if (dw != 0 ) {
        if (ps->dwFlags & MCIAVI_UPDATING)
            npMCI->dwFlags |= MCIAVI_UPDATING;
        else
            npMCI->dwFlags &= ~MCIAVI_UPDATING;
    }

    DPF2(("StopTemporarily: stopped.\n"));
    return dw;
}

DWORD NEAR PASCAL RestartAgain(NPMCIGRAPHIC npMCI, TEMPORARYSTATE FAR * ps)
{
    DWORD   dw = 0;
    DWORD   dwFlags = 0;

    DPF2(("Restart Again: restarting.\n"));

    Assert(ps);

    if (ps->dwFlags & MCIAVI_REVERSE)
        dwFlags = MCI_DGV_PLAY_REVERSE;

    // !!! Make sure that this will actually cause a repeat in all cases....

    if (ps->dwFlags & MCIAVI_REPEATING)
        npMCI->dwFlags |= MCIAVI_REPEATING;
    else
        npMCI->dwFlags &= ~MCIAVI_REPEATING;

    if (ps->wOldTaskState == TASKPLAYING) {
	/* The only flags that matter at this point are the
	** VGA flags and the wait flag.  If we managed to
	** get a new command, neither is in effect, so it's
	** okay to pass zero for these flags.
	*/
	npMCI->lFrom = npMCI->lCurrentFrame;
	dw = DevicePlay(npMCI, ps->lTo, dwFlags | MCI_TO);
    } else if (ps->wOldTaskState == TASKCUEING) {
	/* Continue whatever we were doing */
	npMCI->lFrom = ps->lFrom;
	dw = DevicePlay(npMCI, ps->lTo, dwFlags | MCI_TO);
    } else if (ps->wOldTaskState == TASKPAUSED) {
	dw = DeviceCue(npMCI, 0, MCI_WAIT);
	npMCI->lTo = ps->lTo;
    } else if (ps->wOldTaskState == TASKIDLE) {
    } else {
	DPF(("Trying to restart: task state %u...\n", ps->wOldTaskState));
        Assert(0);
    }

    //
    // restore this flag so we can yield again.
    //
    if (ps->dwFlags & MCIAVI_UPDATING)
        npMCI->dwFlags |= MCIAVI_UPDATING;
    else
        npMCI->dwFlags &= ~MCIAVI_UPDATING;

    DPF2(("Restart Again: restarted.\n"));
    return dw;
}

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
    if (!(npMCI->dwFlags & MCIAVI_NEEDTOSHOW)) {
        DPF0(("ShowStage returning NEEDTOSHOW is OFF\n"));
        return;
    }

    if ((npMCI->dwFlags & MCIAVI_SHOWVIDEO) &&
	    npMCI->hwnd == npMCI->hwndDefault &&
////////////!(GetWindowLong(npMCI->hwnd, GWL_STYLE) & WS_CHILD) &&
	    (!IsWindowVisible (npMCI->hwnd) ||
		npMCI->hwnd != GetActiveWindow ())) {
#ifdef WM_AVISWP
        // Get the UI thread to do the window positioning
        // This routine can be called on the background task while the main
        // routine is waiting in mciaviTaskWait
        SendMessage(npMCI->hwnd, WM_AVISWP, 0,
                        SWP_NOMOVE | SWP_NOSIZE |
                        SWP_SHOWWINDOW |
                        (IsWindowVisible(npMCI->hwnd) ? SWP_NOACTIVATE : 0));
#else
	SetWindowPos(npMCI->hwnd, HWND_TOP, 0, 0, 0, 0,
			SWP_NOMOVE | SWP_NOSIZE |
			SWP_SHOWWINDOW |
			(IsWindowVisible(npMCI->hwnd) ? SWP_NOACTIVATE : 0));
#endif
    }

    //
    // if the movie has palette changes we need to make it the active
    // window otherwise the palette animation will not work right
    //
    if ((npMCI->dwFlags & MCIAVI_ANIMATEPALETTE) &&
            !(npMCI->dwFlags & MCIAVI_SEEKING) &&
            !(npMCI->dwFlags & MCIAVI_FULLSCREEN) &&
            !(npMCI->dwFlags & MCIAVI_UPDATING) &&
            npMCI->hwnd == npMCI->hwndDefault &&
            !(GetWindowLong(npMCI->hwnd, GWL_STYLE) & WS_CHILD)) {
        SetActiveWindow(npMCI->hwnd);
    }

    npMCI->dwFlags &= ~(MCIAVI_NEEDTOSHOW);
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api DWORD | DeviceOpen | Open an AVI file.
 *
 * @parm NPMCIGRAPHIC | npMCI | Pointer to instance data.
 *
 * @parm LPSTR | lpName | file name.
 *
 * @parm DWORD | dwFlags | Open flags.
 *
 * @rdesc 0 means OK, otherwise mci error
 *
 ***************************************************************************/

DWORD PASCAL DeviceOpen(NPMCIGRAPHIC npMCI, DWORD dwFlags)
{
    DWORD	dwRet = 0L;

    npMCI->wTaskState = TASKBEINGCREATED;

    npMCI->uErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS |
				     SEM_NOOPENFILEERRORBOX);

#ifndef WIN32
    // give our PSP to the task
    npMCI->pspParent = GetCurrentPDB();
#endif

    switch (mmTaskCreate(mciaviTask, &npMCI->hThreadTermination, (DWORD)(UINT)npMCI)) {
	case 0:


	    // Yield to the newly created task until it has
            // had a chance to initialize or fail to initialize

            while (npMCI->wTaskState <= TASKINIT) {

#ifndef WIN32
                Yield();
#else
                /* we have to peekmsg here since the threads are
                 * synchronised. but we don't need to actually
                 * pick up any messages - so limit ourselves to the
                 * avi window ones
                 */
                Sleep(1);
                if (npMCI->hwnd) {
                    MSG msg;

                    if (PeekMessage(&msg, npMCI->hwnd, 0, 0, PM_REMOVE)) {
                        DispatchMessage(&msg);
                    }
                }
#endif

         	if (npMCI->wTaskState != TASKBEINGCREATED && !IsTask(npMCI->hTask))
                    break;
            }

            /*
             * we need to do this peek message again.  We may have never
             * entered the body of the loop above, or if this thread
             * gets very little cpu during the above loop, we might fail to
             * execute the PeekMessage above AFTER the SetWindowPos happens
             * in mciaviOpen. In that case, the swp resizing will not happen
             * until the next getmessage or peekmessage - in that case,
             * it could come after the ShowWindow (bad) or after
             * another size request (much worse).
             *
             * First check the thread opened the device successfully
             */

	    if (!IsTask(npMCI->hTask)) {
                // Task thread failed its initialisation.  Wait for the
                // task to terminate before returning to the user.
                DPF2(("Waiting for task thread to terminate\n"));
#ifdef WIN32
                // On Win32 we must explicitly wait.  On Win16, because this
                // "thread" does not get control back until the task thread
                // releases control the wait is irrelevant and is not used.
                TaskWaitComplete(npMCI);
#endif
		dwRet = npMCI->dwTaskError;
            } else {

                if (npMCI->hwnd) {
                    MSG msg;
                    if (PeekMessage(&msg, npMCI->hwnd, 0, 0, PM_REMOVE)) {
                        DispatchMessage(&msg);
                    }
                }
            }

	    break;
	
	case TASKERR_NOTASKSUPPORT:
	case TASKERR_OUTOFMEMORY:
	default:
            npMCI->hTask = 0;
	    dwRet = MCIERR_OUT_OF_MEMORY;
	    break;
    }

    SetErrorMode(npMCI->uErrorMode);

    return dwRet;
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api DWORD | DeviceStop | Stop an AVI movie.
 *
 * @parm NPMCIGRAPHIC | npMCI | Pointer to instance data.
 *
 * @parm DWORD | dwFlags | Flags.
 *
 * @rdesc 0 means OK, otherwise mci error
 *
 ***************************************************************************/

DWORD PASCAL DeviceStop(NPMCIGRAPHIC npMCI, DWORD dwFlags)
{
    DWORD dw = 0L;

    /* Stop the record or playback if the task is currently playing */

    if (!IsTask(npMCI->hTask))
	return MCIERR_DEVICE_NOT_READY;
	
    if (npMCI->wTaskState == TASKPLAYING || npMCI->wTaskState == TASKPAUSED
                    || npMCI->wTaskState == TASKCUEING
                    || npMCI->wTaskState == TASKSTARTING) {
        /* Set STOP flag - the task watches for this flag to be set. The
        ** STOP flag is cleared by the task when playback has stopped.
	*/
	// Assert(!(npMCI->dwFlags & MCIAVI_STOP));

	npMCI->dwFlags |= MCIAVI_STOP;

        /* Send an extra signal to the task in case it is still
        ** blocked. This will be true if we are paused or if play
        ** has just completed.
	*/

        mmTaskSignal(npMCI->hTask);

	/* Yield until playback is finished and we've really stopped. */
        mciaviTaskWait(npMCI, TASKIDLE, FALSE);
    } else {
#ifdef DEBUG
        if (npMCI->wTaskState != TASKIDLE) {
            DPF0(("Unknown task state (DeviceStop) %d\n", npMCI->wTaskState));
        }
        Assert(npMCI->wTaskState == TASKIDLE);	 // ??? Why ???
#endif
    }

    return dw;
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api DWORD | DevicePause | Pause an AVI movie.
 *
 * @parm NPMCIGRAPHIC | npMCI | Pointer to instance data.
 *
 * @parm DWORD | dwFlags | Flags.
 *
 * @rdesc 0 means OK, otherwise mci error
 *
 ***************************************************************************/

DWORD PASCAL DevicePause(NPMCIGRAPHIC npMCI, DWORD dwFlags)
{
    DWORD dw = 0L;

    // If we're currently seeking, allow that to finish before
    // pausing.  This could potentially lock up the machine for
    // a while, but the alternatives are ugly.
    mciaviTaskWait(npMCI, -TASKCUEING, FALSE);

    // Pause the record or playback if the task is currently playing
    // or recording (BUSY)

    if (npMCI->wTaskState == TASKPAUSED) {
	/* We're already paused at the right place, so
	** that means we did it.  Reset the flag, though, just in
	** case we were about to restart.
	*/
	npMCI->dwFlags |= MCIAVI_PAUSE;
	if (dwFlags & MCI_NOTIFY)
	    GraphicDelayedNotify(npMCI, MCI_NOTIFY_SUCCESSFUL);
    } else if (npMCI->wTaskState == TASKPLAYING) {
	npMCI->dwFlags |= MCIAVI_PAUSE | MCIAVI_WAITING;
	
	/* If the notify flag is set, set a flag which will tell us to
	** send a notification when we actually pause.
	*/
	if (dwFlags & MCI_NOTIFY)
	    npMCI->dwFlags |= MCIAVI_CUEING;
	
        if (dwFlags & MCI_WAIT) {
	    /* We have to wait to actually pause. */
	    mciaviTaskWait(npMCI, -TASKPLAYING, TRUE);
	}
	
	npMCI->dwFlags &= ~(MCIAVI_WAITING);
    } else if (npMCI->wTaskState == TASKIDLE) {
	/* We're stopped.  Put us in paused mode by cueing. */
	npMCI->lTo = npMCI->lCurrentFrame;
	DeviceCue(npMCI, 0, dwFlags);
    } else {
	dw = MCIERR_NONAPPLICABLE_FUNCTION;
    }

    return dw;
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api DWORD | DeviceClose | Close an AVI file.
 *
 * @parm NPMCIGRAPHIC | npMCI | Pointer to instance data.
 *
 * @rdesc 0 means OK, otherwise mci error
 *
 ***************************************************************************/

DWORD PASCAL DeviceClose (NPMCIGRAPHIC npMCI)
{
    DWORD dw = 0L;

    if (npMCI && IsTask(npMCI->hTask)) {
	/* Be sure to stop playing, one way or another... */
	DeviceStop(npMCI, MCI_WAIT);

        // task state is now TASKIDLE and blocked

#ifdef DEBUG
        if (npMCI->wTaskState != TASKIDLE) {
            DPF0(("Unknown task state (DeviceClose) %d\n", npMCI->wTaskState));
        }
        Assert(npMCI->wTaskState == TASKIDLE);
#endif

        // Set task state to TASKCLOSE - this informs the task that it is
        // time to die.

        mciaviTaskMessage(npMCI, TASKCLOSE);
	mciaviTaskWait(npMCI, TASKCLOSED, FALSE);

#ifdef WIN32

        /*
        ** Wait for the thread to complete so the DLL doesn't get unloaded
        ** while it's still executing code in that thread
        */

        TaskWaitComplete(npMCI);

#endif // WIN32
    }

    return dw;
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api DWORD | DevicePlay | Play an AVI movie.
 *
 * @parm NPMCIGRAPHIC | npMCI | Pointer to instance data.
 *
 * @parm DWORD | dwFlags | MCI flags from command.
 *
 * @rdesc 0 means OK, otherwise mci error
 *
 ***************************************************************************/

DWORD PASCAL DevicePlay (NPMCIGRAPHIC npMCI, LONG lPlayTo, DWORD dwFlags)
{
    HWND    hCallback;
    DWORD   dw = 0L;

    if (!IsTask(npMCI->hTask))
	return MCIERR_DEVICE_NOT_READY;
	
    /* if not already playing, start the task up. */

    if (dwFlags & MCI_NOTIFY) {
	/* Hide the delayed notification, if any, so it doesn't happen now. */
	hCallback = npMCI->hCallback;
	npMCI->hCallback = NULL;
    }

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

	// !!! This check is dumb: Some DISPDIB's may support > 320x240....
        if ((npMCI->rcMovie.right > 320) || (npMCI->rcMovie.bottom > 240)) {
	    dw = MCIERR_AVI_TOOBIGFORVGA;
	    goto Exit;
	}

	/* If playing, we have to stop so that we'll get put into DispDib
		mode correctly. */
	dw = DeviceStop(npMCI, MCI_WAIT);
	
	if (dw)
	    goto Exit;

	if ((dwFlags & MCI_WAIT) && !(npMCI->dwFlags & MCIAVI_REPEATING))
	    npMCI->dwFlags |= MCIAVI_NOBREAK;
	else
	    npMCI->dwFlags &= ~(MCIAVI_NOBREAK);
		
	npMCI->dwFlags |= MCIAVI_FULLSCREEN;
    } else {
	npMCI->dwFlags &= ~(MCIAVI_FULLSCREEN);
    }

    if ((npMCI->dwFlags & MCIAVI_SEEKING) && (npMCI->lTo != npMCI->lFrom)) {
	/* We're currently seeking, so we have to restart to get audio
	** to work.
	*/
	DeviceStop(npMCI, MCI_WAIT);
    }

    /* If we're currently seeking, stop so play can begin immediately. */
    if (npMCI->wTaskState == TASKCUEING) {
	DeviceStop(npMCI, MCI_WAIT);
    }

    if (npMCI->wTaskState == TASKPLAYING || npMCI->wTaskState == TASKPAUSED) {
	if (((npMCI->dwFlags & MCIAVI_REVERSE) != 0) !=
		((dwFlags & MCI_DGV_PLAY_REVERSE) != 0))
	    DeviceStop(npMCI, MCI_WAIT);
    }

    // Make sure flags are cleared if they should be
    npMCI->dwFlags &= ~(MCIAVI_PAUSE | MCIAVI_CUEING | MCIAVI_REVERSE);

    if (dwFlags & MCI_DGV_PLAY_REPEAT) {
	npMCI->dwFlags |= MCIAVI_REPEATING;
    }

    if (dwFlags & MCI_NOTIFY) {
	/* Restore the notification */
	npMCI->hCallback = hCallback;
    }

    if (lPlayTo > npMCI->lFrames)
        lPlayTo = npMCI->lFrames;

    if (lPlayTo < 0)
        lPlayTo = 0;

    if (dwFlags & MCI_TO)
	npMCI->lTo = lPlayTo;

    if (dwFlags & MCI_DGV_PLAY_REVERSE)
	npMCI->dwFlags |= MCIAVI_REVERSE;

    npMCI->dwFlags |= MCIAVI_WAITING;

    if (npMCI->dwFlags & MCIAVI_NEEDTOSHOW) {
        ShowStage(npMCI);
        //
        // leave this set so the play code knows this is a "real" play
        // coming from the user, not a interal play/stop
        //
        // if the window needs shown we want to do it here if we can
        // not in the background task.
        //
        npMCI->dwFlags |= MCIAVI_NEEDTOSHOW;
    }

    if (npMCI->wTaskState == TASKPAUSED) {
	/* Wake the task up from pausing */
	mmTaskSignal(npMCI->hTask);
    } else if (npMCI->wTaskState == TASKCUEING ||
	       npMCI->wTaskState == TASKPLAYING) {
    } else {
        /* Tell the task what to do when it wakes up */

        mciaviTaskMessage(npMCI, TASKSTARTING);

	dw = npMCI->dwTaskError;
    }

    if (dwFlags & MCI_WAIT) {
	// yield to playback task until playback completes but don't
	// yield to application - apps must use driveryield to get
	// out of waits.

        mciaviTaskWait(npMCI, TASKIDLE, TRUE);
	
	dw = npMCI->dwTaskError;
    }

    if (dwFlags & (MCI_MCIAVI_PLAY_FULLSCREEN | MCI_MCIAVI_PLAY_FULLBY2)) {
	MSG	msg;
	
	/* Remove stray mouse and keyboard events after DispDib. */
	while (PeekMessage(&msg, NULL, WM_KEYFIRST, WM_KEYLAST,
					PM_NOYIELD | PM_REMOVE) ||
			PeekMessage(&msg, NULL, WM_MOUSEFIRST, WM_MOUSELAST,
					PM_NOYIELD | PM_REMOVE))
	    ;
    }

    npMCI->dwFlags &= ~(MCIAVI_WAITING);
Exit:	
    return dw;
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api DWORD | DeviceResume | Play an AVI movie.
 *
 * @parm NPMCIGRAPHIC | npMCI | Pointer to instance data.
 *
 * @parm DWORD | dwFlags | MCI flags from command.
 *
 * @rdesc 0 means OK, otherwise mci error
 *
 ***************************************************************************/

DWORD PASCAL DeviceResume(NPMCIGRAPHIC npMCI, DWORD dwFlags)
{
    DWORD   dw = 0L;

    if (!IsTask(npMCI->hTask))
	return MCIERR_DEVICE_NOT_READY;
	
    dw = DevicePlay(npMCI, 0, dwFlags |
	    ((npMCI->dwFlags & MCIAVI_REVERSE) ? MCI_DGV_PLAY_REVERSE : 0));

    return dw;
}
/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api DWORD | DeviceCue | Cue an AVI movie for playing.
 *
 * @parm NPMCIGRAPHIC | npMCI | Pointer to instance data.
 *
 * @parm LONG | lTo | Frame to seek to, if MCI_TO set in <p dwFlags>.
 *
 * @parm DWORD | dwFlags | MCI flags from command.
 *
 * @rdesc 0 means OK, otherwise mci error
 *
 ***************************************************************************/

DWORD PASCAL DeviceCue(NPMCIGRAPHIC npMCI, LONG lTo, DWORD dwFlags)
{
    DWORD dw = 0L;
    HWND    hCallback;

    /* if not already playing, start animation and set timer */

    if (!IsTask(npMCI->hTask))
	return MCIERR_DEVICE_NOT_READY;
	
    if (dwFlags & MCI_NOTIFY) {
	/* Hide the delayed notification, if any, so it doesn't happen now. */
	hCallback = npMCI->hCallback;
	npMCI->hCallback = NULL;
    }

    if (npMCI->dwFlags & MCIAVI_SEEKING) {
	/* We're currently seeking, so we have to start again to get audio
	** to work.
	*/
	DeviceStop(npMCI, MCI_WAIT);
    }

    if (dwFlags & MCI_TO) {
	DeviceStop(npMCI, MCI_WAIT);
	npMCI->lFrom = lTo;
    } else if (npMCI->wTaskState == TASKIDLE) {
	npMCI->lFrom = npMCI->lCurrentFrame;
    }

    if (dwFlags & MCI_NOTIFY) {
	/* Restore the notification */
	npMCI->hCallback = hCallback;
    }

    /* If we're ever resumed, we want to go to the end of the file. */
    npMCI->lTo = npMCI->lFrames;

    if (npMCI->wTaskState == TASKPAUSED) {
	/* We're already paused at the right place, so
	** that means we did it.
	*/
	if (dwFlags & MCI_NOTIFY)
	    GraphicDelayedNotify(npMCI, MCI_NOTIFY_SUCCESSFUL);
    } else if (npMCI->wTaskState == TASKIDLE) {
	// !!! Is this the only condition we can do this in?	
	npMCI->dwFlags |= MCIAVI_PAUSE | MCIAVI_CUEING | MCIAVI_WAITING;

        mciaviTaskMessage(npMCI, TASKSTARTING);

        if (dwFlags & MCI_WAIT) {
            mciaviTaskWait(npMCI, -TASKCUEING, TRUE);
	}

	npMCI->dwFlags &= ~(MCIAVI_WAITING);
	
	dw = npMCI->dwTaskError;	
    } else if (npMCI->wTaskState == TASKCUEING) {
	npMCI->dwFlags |= MCIAVI_PAUSE | MCIAVI_CUEING | MCIAVI_WAITING;

        if (dwFlags & MCI_WAIT) {
            mciaviTaskWait(npMCI, -TASKCUEING, TRUE);
	}

	npMCI->dwFlags &= ~(MCIAVI_WAITING);
	
	dw = npMCI->dwTaskError;		
    } else if (npMCI->wTaskState == TASKPLAYING) {
	npMCI->dwFlags |= MCIAVI_PAUSE | MCIAVI_CUEING | MCIAVI_WAITING;

        if (dwFlags & MCI_WAIT) {
            mciaviTaskWait(npMCI, -TASKPLAYING, TRUE);
	}

	npMCI->dwFlags &= ~(MCIAVI_WAITING);
	
	dw = npMCI->dwTaskError;		
    } else {
	dw = MCIERR_NONAPPLICABLE_FUNCTION;
    }

    return dw;
}


/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api DWORD | DeviceSeek | Seek to a position in an AVI movie.
 *
 * @parm NPMCIGRAPHIC | npMCI | Pointer to instance data.
 *
 * @parm LONG | lTo | Frame to seek to.
 *
 * @parm DWORD | dwFlags | MCI flags from command.
 *
 * @rdesc 0 means OK, otherwise mci error
 *
 ***************************************************************************/

DWORD PASCAL DeviceSeek(NPMCIGRAPHIC npMCI, LONG lTo, DWORD dwFlags)
{
    DWORD dw = 0;
    HWND    hCallback;

    DPF3(("DeviceSeek\n"));
    /* The window will be shown by the play code. */

    /* If we can just shorten a previous seek, do it. */
    if ((npMCI->wTaskState == TASKCUEING) &&
	    (npMCI->dwFlags & MCIAVI_SEEKING) &&
	    (npMCI->lCurrentFrame <= lTo) &&
	    (npMCI->lTo >= lTo)) {
	if (lTo != npMCI->lTo) {
	DPF3(("Seeking to %ld instead.\n", lTo));
	}
	npMCI->lTo = lTo;
	return 0L;
    }

    if (dwFlags & MCI_NOTIFY) {
	/* Hide the delayed notification, if any, so it doesn't happen now. */
	hCallback = npMCI->hCallback;
	npMCI->hCallback = NULL;
    }

    /* If playing, stop, so we can seek. */
    dw = DeviceStop(npMCI, MCI_WAIT);

    if (dwFlags & MCI_NOTIFY) {
	/* Restore the notification */
	npMCI->hCallback = hCallback;
    }

    // task state is now TASKIDLE and blocked

    if (npMCI->lCurrentFrame != lTo) {
	npMCI->dwFlags |= MCIAVI_WAITING;

	/* Essentially, we are telling the task: play just frame <lTo>.
	** When it gets there, it will update the screen for us.
	*/
	npMCI->lFrom = npMCI->lTo = lTo;
	mciaviTaskMessage(npMCI, TASKSTARTING);
	if (dwFlags & MCI_WAIT) {
	    mciaviTaskWait(npMCI, -TASKCUEING, TRUE);
	}
	npMCI->dwFlags &= ~(MCIAVI_WAITING);
    } else {
	/* Be sure the window gets shown and the notify gets sent,
	** even though we don't have to do anything.
	*/
	if (npMCI->dwFlags & MCIAVI_NEEDTOSHOW)
	    ShowStage(npMCI);

	if (dwFlags & MCI_NOTIFY)
	    GraphicDelayedNotify(npMCI, MCI_NOTIFY_SUCCESSFUL);	
    }
	
    return dw;
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

static void CheckIfActive(NPMCIGRAPHIC npMCI)
{
    BOOL fActive;
    HWND hwndA;

    //
    //  are we the foreground window?
    //
    //  ??? should the value of <npMCI->fForceBackground> matter?
    //
    //  IMPORTANT:  This does NOT work under NT.  The best that can
    //  be done is to check GetForegroundWindow
    hwndA = GetActiveWindow();

    fActive = (hwndA == npMCI->hwnd) ||
              (GetFocus() == npMCI->hwnd) ||
              (IsChild(hwndA, npMCI->hwnd) && !npMCI->fForceBackground);

    DeviceSetActive(npMCI, fActive);
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api DWORD | DeviceRealize | Updates the frame into the given DC
 *
 * @parm NPMCIGRAPHIC | npMCI | Pointer to instance data block.
 *
 * @parm BOOL | fForceBackground | Realize as background palette?
 *
 * @rdesc 0 means OK, otherwise mci error
 *
 ***************************************************************************/

DWORD PASCAL DeviceRealize(NPMCIGRAPHIC npMCI)
{
    BOOL fGetDC;
    BOOL fPalChanged;
    BOOL fAlreadyDoneThat;

    if (npMCI->dwFlags & MCIAVI_WANTMOVE)
	CheckWindowMove(npMCI, TRUE);

    if (fAlreadyDoneThat = (BOOL)(npMCI->dwFlags & MCIAVI_UPDATING)) {
	DPF(("Re-entering DeviceRealize - but we don't care"));
    }

    if (fGetDC = (npMCI->hdc == NULL)) {
	npMCI->hdc = GetDC(npMCI->hwnd);
    }

    npMCI->dwFlags |= MCIAVI_UPDATING;

    fPalChanged = PrepareDC(npMCI) > 0;

    if (!fAlreadyDoneThat)
        npMCI->dwFlags &= ~MCIAVI_UPDATING;

    if (fGetDC) {
        UnprepareDC(npMCI);
        ReleaseDC(npMCI->hwnd, npMCI->hdc);
	npMCI->hdc = NULL;
    }

    if (fPalChanged)
        InvalidateRect(npMCI->hwnd, &npMCI->rcDest, TRUE);

    CheckIfActive(npMCI);

    return 0L;
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api DWORD | DeviceActivate | is the movie active?
 *
 * @parm NPMCIGRAPHIC | npMCI | Pointer to instance data block.
 *
 * @rdesc 0 means OK, otherwise mci error
 *
 ***************************************************************************/

DWORD PASCAL DeviceSetActive(NPMCIGRAPHIC npMCI, BOOL fActive)
{
    if (fActive)
#ifdef WIN32
        // We must explicitly request a unicode string.  %s will not
        // work as dprintf uses wvsprintfA
        DPF(("**** '%hs' is active.\n", (LPTSTR)npMCI->szFilename));
#else
        DPF(("**** '%s' is active.\n", (LPTSTR)npMCI->szFilename));
#endif

    //
    //  if we are now the foreground "window" try to get the wave
    //  device back (iff it was stolen from us)
    //
    if (fActive && (npMCI->dwFlags & MCIAVI_LOSTAUDIO)) {

        if (StealWaveDevice(npMCI)) {
            Assert(npMCI->dwFlags & MCIAVI_PLAYAUDIO);
            Assert(npMCI->hWave == NULL);

            npMCI->dwFlags &= ~MCIAVI_PLAYAUDIO;
            DeviceMute(npMCI, FALSE);
        }
    }

    return 0;
}

/***************************************************************************
 *
 *  IsScreenDC() - returns true if the passed DC is a DC to the screen.
 *                 NOTE this checks for a DCOrg != 0, bitmaps always have
 *                 a origin of (0,0)  This will give the wrong info a
 *                 fullscreen DC.
 *
 ***************************************************************************/

#ifndef WIN32
#define IsScreenDC(hdc)     (GetDCOrg(hdc) != 0L)
#else
INLINE BOOL IsScreenDC(HDC hdc)
{
    POINT   pt;

    GetDCOrgEx(hdc, &pt);
    return pt.x != 0 && pt.y != 0;
}
#endif

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api DWORD | DeviceUpdate | Updates the frame into the given DC
 *
 * @parm NPMCIGRAPHIC | npMCI | Pointer to instance data block.
 *
 * @parm HDC | hDC | DC to draw frame into.
 *
 * @parm LPRECT | lprc | Update rect.
 *
 * @rdesc 0 means OK, otherwise mci error
 *
 ***************************************************************************/

DWORD PASCAL DeviceUpdate(NPMCIGRAPHIC npMCI, DWORD dwFlags, HDC hdc, LPRECT lprc)
{
    DWORD   dwErr = 0L;
    BOOL    f;
    HDC     hdcSave;
    TEMPORARYSTATE ts;
    HWND    hCallback;
    HCURSOR hcurPrev;
    RECT    rc;
    LONG    lFrameDrawn;

    if (!IsTask(npMCI->hTask)) {
        DPF0(("Returning DEVICE_NOT_READY from DeviceUpdate\n"));
        return MCIERR_DEVICE_NOT_READY;
    }

    if (npMCI->dwFlags & MCIAVI_UPDATING) {
        DPF(("DeviceUpdate has been reentered.\n"));
        Assert(0);
        return MCIERR_DEVICE_NOT_READY;
    }

    if (npMCI->dwFlags & MCIAVI_WANTMOVE)
	CheckWindowMove(npMCI, TRUE);

    //
    // see if we are the active movie now.
    //
    CheckIfActive(npMCI);

    /* Setting this flag insures that the background task doesn't
    ** yield while we're trying to update.
    */
    npMCI->dwFlags |= MCIAVI_UPDATING;

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

    //
    // if we are playing/seeking... (ie have a DC)
    // then realize the palette now. and set the update flag if we just need to
    // paint
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

	UnprepareDC(npMCI);
        PrepareDC(npMCI);  // re-prepare

////////
//////// a update can fail when paused so we may have to stop/restart the task
////////
////////if (npMCI->wTaskState == TASKPAUSED)
////////    mmTaskSignal(npMCI->hTask);

        npMCI->dwFlags &= ~MCIAVI_UPDATING;
        return 0L;
    }

    //////////////////////////////////////////////////////////////////////
    //
    //  when we get here one of the follow applies
    //
    //      1.  we aren't playing/seeking/...
    //
    //      2.  we need to draw into a memory bitmap (not the screen)
    //
    //////////////////////////////////////////////////////////////////////

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
        SaveDC(hdc);
        IntersectClipRect(hdc, lprc->left, lprc->top,
                               lprc->right, lprc->bottom);
    }

    //
    //  Always do an Update, if the update succeeds and we are at the right
    //  frame keep it.
    //
    //  if it fails or the frame is wrong re-draw
    //
    //  we need to do this because even though lFrameDrawn is a valid
    //  frame the draw handler may fail a update anyway (for example
    //  when decompressing to screen) so lFrameDrawn can be bogus and
    //  we do not know it until we try it.
    //

    lFrameDrawn = npMCI->lFrameDrawn;       // save this for compare

    if (npMCI->lFrameDrawn <= npMCI->lCurrentFrame &&
        npMCI->lFrameDrawn >= 0) {

        DPF2(("Update: redrawing frame %ld, current = %ld.\n", npMCI->lFrameDrawn, npMCI->lCurrentFrame));

	/* Save the DC, in case we're playing, but need to update
	** to a memory bitmap.
	*/
	hdcSave = npMCI->hdc;
        npMCI->hdc = hdc;

	/* Realize the palette here, because it will cause strange
        ** things to happen if we do it in the task.
        */
	if (npMCI->dwFlags & MCIAVI_NEEDDRAWBEGIN) {
	    DrawBegin(npMCI, NULL);

	    if (npMCI->lFrameDrawn < npMCI->lVideoStart) {
		npMCI->hdc = hdcSave;
		goto SlowUpdate;
	    }
	}

        PrepareDC(npMCI);        // make sure the palette is in there

	// worker thread must hold critsec round all drawing
        EnterCrit(npMCI);
        f = DoStreamUpdate(npMCI, FALSE);
	LeaveCrit(npMCI);

        UnprepareDC(npMCI);      // be sure to put things back....
        npMCI->hdc = hdcSave;

        if (!f) {
SlowUpdate:
            DPF(("DeviceUpdate failed! invalidating lFrameDrawn\n"));
            npMCI->lFrameDrawn = (- (LONG) npMCI->wEarlyRecords) - 1;
        }
        else if (npMCI->lFrameDrawn >= npMCI->lCurrentFrame-1) {
            goto Exit;
        }
    }

    DPF(("Update: drawn = %ld, current = %ld.\n", npMCI->lFrameDrawn, npMCI->lCurrentFrame));

    //
    // stop everything.
    //
    StopTemporarily(npMCI, &ts);
    Assert(npMCI->hdc == NULL);
    Assert(npMCI->wTaskState == TASKIDLE);

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
    npMCI->hdc = hdc;
    PrepareDC(npMCI);        // make sure the palette is in there

    hcurPrev =  SetCursor(LoadCursor(NULL, IDC_WAIT));

    /* Hide any notification, so it won't get sent... */
    hCallback = npMCI->hCallback;
    npMCI->hCallback = NULL;

    /* Wake the task up, and wait until it quiets down. */
    mciaviTaskMessage(npMCI, TASKSTARTING);
    mciaviTaskWait(npMCI, TASKIDLE, FALSE);
    dwErr = npMCI->dwTaskError;

    npMCI->hCallback = hCallback;

    // We may have just yielded.. so only set the cursor back if we
    // are still the wait cursor.
    if (hcurPrev) {
        hcurPrev = SetCursor(hcurPrev);
        if (hcurPrev != LoadCursor(NULL, IDC_WAIT))
            SetCursor(hcurPrev);
    }

    npMCI->hdc = NULL;

    if (dwErr == 0)
        dwErr = RestartAgain(npMCI,&ts);
Exit:
    if (lprc) {
        RestoreDC(hdc, -1);
    }

    npMCI->dwFlags &= ~(MCIAVI_UPDATING|MCIAVI_UPDATETOMEMORY);

    if (npMCI->dwFlags & MCIAVI_NEEDUPDATE) {
        DPF(("**** we did a DeviceUpdate but still dirty?\n"));
    }

    return dwErr;
}


/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api DWORD | DeviceStatus | Returns the current status
 *
 * @parm NPMCIGRAPHIC | npMCI | Pointer to instance data block.
 *
 * @rdesc Returns value for MCI's return value
 *
 ***************************************************************************/

UINT PASCAL DeviceMode(NPMCIGRAPHIC npMCI)
{
    if (!IsTask(npMCI->hTask)) {
	return MCI_MODE_NOT_READY;
    }

    switch (npMCI->wTaskState) {
	case TASKIDLE:	
	    return MCI_MODE_STOP;
	
	case TASKCUEING:	
	    return MCI_MODE_SEEK;
	
	case TASKPLAYING:	
	    return MCI_MODE_PLAY;
	
	case TASKPAUSED:	
	    return MCI_MODE_PAUSE;
	
	case TASKBEINGCREATED:	
	case TASKINIT:	
	case TASKCLOSE:	
	case TASKSTARTING:	
	case TASKREADINDEX:	
	default:
            DPF(("Unexpected state %d in DeviceMode()\n", npMCI->wTaskState));
	    return MCI_MODE_NOT_READY;
    }
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api DWORD | DevicePosition | Returns the current frame
 *
 * @parm NPMCIGRAPHIC | npMCI | Pointer to instance data block.
 *
 * @parm LPLONG | lpl | returns current frame
 *
 * @rdesc 0 means OK, otherwise mci error
 *
 ***************************************************************************/

DWORD PASCAL DevicePosition(NPMCIGRAPHIC npMCI, LPLONG lpl)
{
    LONG NEAR PASCAL WhatFrameIsItTimeFor(NPMCIGRAPHIC npMCI);
    LONG    l;

    l = npMCI->lCurrentFrame - npMCI->dwBufferedVideo;

#if 0
    if (npMCI->wTaskState == TASKPLAYING &&
			npMCI->wPlaybackAlg != MCIAVI_ALG_INTERLEAVED)
	l = WhatFrameIsItTimeFor(npMCI);
#endif

    if ((npMCI->wTaskState == TASKCUEING) &&
	    !(npMCI->dwFlags & MCIAVI_SEEKING) &&
	    l < npMCI->lRealStart)
	l = npMCI->lRealStart;

    if (l < 0)
	l = 0;

    *lpl = l;

    return 0L;
}


/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api DWORD | DeviceSetWindow | Set window for display
 *
 * @parm NPMCIGRAPHIC | npMCI | Pointer to instance data.
 *
 * @parm HWND | hwnd | Window to display into.
 *
 * @rdesc 0 means OK, otherwise mci error
 *
 * @comm Should this only take effect at time of next play?
 *
 ***************************************************************************/

DWORD PASCAL DeviceSetWindow(NPMCIGRAPHIC npMCI, HWND hwnd)
{
    DWORD	    dw = 0L;
    TEMPORARYSTATE  ts;

    /* Stop play before changing windows. */
    dw = StopTemporarily(npMCI, &ts);

    if (!dw) {
        npMCI->hwnd = hwnd;

        if (ts.wOldTaskState == TASKIDLE) {
#if 0
            DrawBegin(npMCI);
            DrawEnd(npMCI);
#else
	    npMCI->dwFlags |= MCIAVI_NEEDDRAWBEGIN;
	    InvalidateRect(hwnd, &npMCI->rcDest, FALSE);
#endif
        }

	/* Should we update the window here? */

	/* Start playing again in the new window */
	dw = RestartAgain(npMCI, &ts);
    }

    return dw;
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api DWORD | DeviceSpeed | Adjust the playback speed of an AVI movie.
 *
 * @parm NPMCIGRAPHIC | npMCI | Pointer to instance data.
 *
 * @parm DWORD | dwNewSpeed | New speed, where 1000 is 'normal' speed.
 *
 * @rdesc 0 means OK, otherwise mci error
 *
 * @comm If we are currently playing, we stop the device, set our flag,
 *	and start playing again where we left off.  If we were paused,
 *	we end up stopped.  Is this bad?
 *
 ***************************************************************************/

DWORD PASCAL DeviceSetSpeed(NPMCIGRAPHIC npMCI, DWORD dwNewSpeed)
{
    DWORD	dw = 0L;
    TEMPORARYSTATE  ts;

    /* If new speed is the same as the old speed, return. */
    if (dwNewSpeed == npMCI->dwSpeedFactor)
	return 0L;

    // !!! What if we were cueing or paused?

    npMCI->dwSpeedFactor = dwNewSpeed;

    if (npMCI->wTaskState == TASKIDLE)
	return 0L;

    /* We're playing, so we have to adjust the playback rate in
    ** midstream.  If we don't have sound going, this is pretty
    ** easy.  If we do have sound, we either need to speed it up
    ** or slow it down or stop and start over.
    */

    // This code doesn't work, since there are internal variables that
    // need to be updated.  Therefore, just stop and restart, even if there
    // isn't any sound.
#if 0
    /* Figure out how fast we're playing.... */
    npMCI->dwPlayMicroSecPerFrame = muldiv32(npMCI->dwMicroSecPerFrame, 1000L,
						    npMCI->dwSpeedFactor);

    /* If there's no sound, we're done. */
    if ((npMCI->nAudioStreams == 0) ||
            !(npMCI->dwFlags & MCIAVI_PLAYAUDIO))
	return 0L;

    if (npMCI->hWave) {
	/* We could potentially try to do a waveOutSetPlaybackRate() here. */
    }
#endif

    dw = StopTemporarily(npMCI, &ts);

    if (!dw) {
	dw = RestartAgain(npMCI, &ts);
    }

    return dw;
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api DWORD | DeviceMute | Turn AVI sound on/off.
 *
 * @parm NPMCIGRAPHIC | npMCI | Pointer to instance data.
 *
 * @parm BOOL | fMute | TRUE If sound should be turned off, FALSE
 *      if sound should stay on.
 *
 * @rdesc 0 means OK, otherwise mci error
 *
 * @comm If we are currently playing, we stop the device, set our flag,
 *	and start playing again where we left off.  If we were paused,
 *	we end up stopped.  Is this bad?
 *
 ***************************************************************************/

DWORD PASCAL DeviceMute(NPMCIGRAPHIC npMCI, BOOL fMute)
{
    DWORD	dw = 0L;
    TEMPORARYSTATE  ts;

    /* If there's no audio, just return. Should this be an error? */
    if (npMCI->nAudioStreams == 0)
        return 0L;

    /* If the mute state isn't changing, don't do anything. */
    if (npMCI->dwFlags & MCIAVI_PLAYAUDIO) {
	if (!fMute)
	    return 0L;
    } else {
	if (fMute)
	    return 0L;
    }

    /* Stop before changing mute */

    dw = StopTemporarily(npMCI, &ts);

    if (!dw) {

        if (fMute)
            npMCI->dwFlags &= ~MCIAVI_PLAYAUDIO;
        else
            npMCI->dwFlags |= MCIAVI_PLAYAUDIO;

	dw = RestartAgain(npMCI, &ts);
    }

    return dw;
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api DWORD | DeviceSetVolume | Set AVI volume.
 *
 * @parm NPMCIGRAPHIC | npMCI | Pointer to instance data.
 *
 * @parm DWORD | dwVolume | ranges from 0 to 1000.
 *
 * @rdesc 0 means OK, otherwise mci error
 *
 * @comm If we are currently playing, we try to change the volume of the
 *	wave out device.
 *
 ***************************************************************************/

DWORD PASCAL DeviceSetVolume(NPMCIGRAPHIC npMCI, DWORD dwVolume)
{
    DWORD	dw = 0L;
    npMCI->dwVolume = dwVolume;

    npMCI->dwFlags |= MCIAVI_VOLUMESET;

    /* clear flag to emulate volume */;
    npMCI->fEmulatingVolume = FALSE;

    /* If there's no audio, just return. Should this be an error? */
    if (npMCI->nAudioStreams == 0)
	return 0L;

    dw = DeviceMute(npMCI, dwVolume == 0);

    if (npMCI->hWave && dw == 0L) {
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

    return dw;
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api DWORD | DeviceGetVolume | Check the wave output device's current
 *	volume.
 *
 * @parm NPMCIGRAPHIC | npMCI | Pointer to instance data.
 *
 * @rdesc 0 means OK, otherwise mci error
 *
 * @comm The volume is left in npMCI->dwVolume
 *
 * Issue: On devices with global volume control, like an SBPro, how should
 *	things work?
 *
 ***************************************************************************/
DWORD PASCAL DeviceGetVolume(NPMCIGRAPHIC npMCI)
{
    DWORD	dw;
    DWORD	dwVolume;

    if (npMCI->hWave) {
	// Get the current audio volume....
	dw = waveOutMessage(npMCI->hWave, WODM_GETVOLUME,
			    (DWORD) (DWORD FAR *)&dwVolume, 0);

	if (dw == 0) {
returnvolume:
            npMCI->dwVolume = MAKELONG((UINT)muldiv32(LOWORD(dwVolume), 500L, 32768L),
                                       (UINT)muldiv32(HIWORD(dwVolume), 500L, 32768L));
	}
    } else if (!(npMCI->dwFlags & MCIAVI_VOLUMESET)) {
	// We have no device open, and the user hasn't chosen a
	// volume yet.

        //
        // Try to find out what the current "default" volume is.
        //
        // I realy doubt zero is the current volume, try to work
        // with broken cards like the stupid windows sound system.
        //
        dw = waveOutGetVolume((UINT)WAVE_MAPPER, &dwVolume);

        if (dw == 0 && dwVolume != 0)
	    goto returnvolume;

        dw = waveOutGetVolume(0, &dwVolume);

        if (dw == 0 && dwVolume != 0)
	    goto returnvolume;

	return MCIERR_NONAPPLICABLE_FUNCTION;
    }

    return 0;
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api DWORD | DeviceSetAudioStream | Choose which audio stream to use.
 *
 * @parm NPMCIGRAPHIC | npMCI | Pointer to instance data.
 *
 * @parm WORD | wStream | ranges from 1 to the number of streams.
 *
 * @rdesc 0 means OK, otherwise mci error
 *
 ***************************************************************************/

DWORD PASCAL DeviceSetAudioStream(NPMCIGRAPHIC npMCI, UINT wAudioStream)
{
    DWORD	dw = 0L;
    TEMPORARYSTATE  ts;
    int		stream;

    /* If there's no audio, just return. Should this be an error? */

    if (npMCI->nAudioStreams == 0)
        return 0;

    for (stream = 0; stream < npMCI->streams; stream++) {
	if (SH(stream).fccType == streamtypeAUDIO) {
	    --wAudioStream;

	    if (wAudioStream == 0)
		break;
	}
    }

    if (stream == npMCI->nAudioStream)
	return 0;

    Assert(stream < npMCI->streams);

    /* Stop before changing mute */

    dw = StopTemporarily(npMCI, &ts);

    if (!dw) {
        npMCI->psiAudio = SI(stream);
	npMCI->nAudioStream = stream;
	
	dw = RestartAgain(npMCI, &ts);
    }

    return dw;
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api DWORD | DeviceSetVideoStream | Choose which video stream is the
 *  "default".  Also can enable/disable a stream.  this works for both
 *  video and "other" streams.
 *
 * @parm NPMCIGRAPHIC | npMCI | Pointer to instance data.
 *
 * @parm WORD | wStream | ranges from 1 to the number of streams.
 *
 * @rdesc 0 means OK, otherwise mci error
 *
 ***************************************************************************/

DWORD PASCAL DeviceSetVideoStream(NPMCIGRAPHIC npMCI, UINT uStream, BOOL fOn)
{
    DWORD	dw = 0L;
    TEMPORARYSTATE  ts;
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

    if (stream == npMCI->streams)
        return MCIERR_OUTOFRANGE;

    /* Stop before changing */

    dw = StopTemporarily(npMCI, &ts);

    if (!dw) {

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

	dw = RestartAgain(npMCI, &ts);
    }

    return dw;
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
        DPF0(("MapRect: In    [%d %d %d %d]\n", *prcIn));
        DPF0(("MapRect: From  [%d %d %d %d]\n", *prcFrom));
        DPF0(("MapRect: To    [%d %d %d %d]\n", *prcTo));
        prc->left  = prcTo->left + MulDiv(prcIn->left  - prcFrom->left, prcTo->right  - prcTo->left, prcFrom->right  - prcFrom->left);
        prc->top   = prcTo->top  + MulDiv(prcIn->top   - prcFrom->top,  prcTo->bottom - prcTo->top,  prcFrom->bottom - prcFrom->top);
        prc->right = prcTo->left + MulDiv(prcIn->right - prcFrom->left, prcTo->right  - prcTo->left, prcFrom->right  - prcFrom->left);
        prc->bottom= prcTo->top  + MulDiv(prcIn->bottom- prcFrom->top,  prcTo->bottom - prcTo->top,  prcFrom->bottom - prcFrom->top);
        DPF0(("MapRect: OUT   [%d %d %d %d]\n", *prc));
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

	
        DPF0(("SH(%d) rcFrame  [%d %d %d %d]\n", i, SH(i).rcFrame));
        DPF0(("SI(%d) rcSource [%d %d %d %d]\n", i, SI(i)->rcSource));
        DPF0(("SI(%d) rcDest   [%d %d %d %d]\n", i, SI(i)->rcDest));
        DPF0(("np(%d) rcSource [%d %d %d %d]\n", i, npMCI->rcSource));
        DPF0(("np(%d) rcDest   [%d %d %d %d]\n", i, npMCI->rcDest  ));
        DPF0(("\n"));
        IntersectRect(&SI(i)->rcSource, &SH(i).rcFrame, &npMCI->rcSource);
        DPF0(("SI(%d) rcSource [%d %d %d %d]\n", i, SI(i)->rcSource));
        DPF0(("\n"));

        //
        // now map the stream source rect onto the destination
        //
        MapRect(&SI(i)->rcDest, &SI(i)->rcSource, &npMCI->rcSource, &npMCI->rcDest);
	
        DPF0(("SI(%d) rcSource [%d %d %d %d]\n", i, SI(i)->rcSource));
        DPF0(("SI(%d) rcDest   [%d %d %d %d]\n", i, SI(i)->rcDest));
        DPF0(("np(%d) rcSource [%d %d %d %d]\n", i, npMCI->rcSource));
        DPF0(("np(%d) rcDest   [%d %d %d %d]\n", i, npMCI->rcDest  ));
        DPF0(("\n"));

        //
        // make the stream source RECT (rcSource) relative to the
        // stream rect (rcFrame)
        //
        OffsetRect(&SI(i)->rcSource,-SH(i).rcFrame.left,-SH(i).rcFrame.top);
	
        DPF0(("SI(%d) rcSource [%d %d %d %d]\n", i, SI(i)->rcSource));
        DPF0(("SI(%d) rcDest   [%d %d %d %d]\n", i, SI(i)->rcDest));
        DPF0(("np(%d) rcSource [%d %d %d %d]\n", i, npMCI->rcSource));
        DPF0(("np(%d) rcDest   [%d %d %d %d]\n", i, npMCI->rcDest  ));
        DPF0(("\n"));
    }
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api DWORD | DevicePut | Change source or destination rectangle
 *
 * @parm NPMCIGRAPHIC | npMCI | Pointer to instance data.
 *
 * @parm LPRECT | lprc | Pointer to new rectangle to use.
 *
 * @parm DWORD | dwFlags | Flags: will be either MCI_DGV_PUT_DESTINATION
 *	or MCI_DGV_PUT_SOURCE.
 *
 * @rdesc 0 means OK, otherwise mci error
 *
 * @comm
 *	If we end up using a custom stretch buffer, it would go here.
 *
 ***************************************************************************/
DWORD FAR PASCAL DevicePut(NPMCIGRAPHIC npMCI, LPRECT lprc, DWORD dwFlags)
{
    RECT    rc;
    PRECT   prcPut;
    DWORD   dw = 0;

    if (dwFlags & MCI_DGV_PUT_DESTINATION) {
        DPF2(("DevicePut: destination [%d, %d, %d, %d]\n", *lprc));
        prcPut = &npMCI->rcDest;
    } else {
        DPF2(("DevicePut: source [%d, %d, %d, %d]\n", *lprc));
        prcPut = &npMCI->rcSource;

        //
        // make sure source rectangle is in range.
        //
        //  !!!should we return a error, or just fix the rectange???
        //
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
        DPF2(("DevicePut: invalid rectangle [%d, %d, %d, %d]\n", *lprc));
        return MCIERR_OUTOFRANGE;
    }

    /* make sure the rect changed */
    if (EqualRect(prcPut,lprc))
        return 0L;

    InvalidateRect(npMCI->hwnd, &npMCI->rcDest, TRUE);
    rc = *prcPut;           /* save it */
    *prcPut = *lprc;        /* change it */
    InvalidateRect(npMCI->hwnd, &npMCI->rcDest, FALSE);

    /* has both the dest and source been set? */
    if (IsRectEmpty(&npMCI->rcDest) || IsRectEmpty(&npMCI->rcSource))
        return 0L;

    MapStreamRects(npMCI);
    StreamInvalidate(npMCI, NULL);      // invalidate the world

    if (npMCI->wTaskState <= TASKIDLE) {
	DPF2(("DevicePut: Idle, force DrawBegin on update\n"));
	npMCI->dwFlags |= MCIAVI_NEEDDRAWBEGIN;
    }
    else {
	BOOL	fRestart;
	
        //
        //  we dont need to start/stop just begin again.
        //
	DPF2(("DevicePut: Calling DrawBegin()\n"));
	if (!DrawBegin(npMCI, &fRestart)) {
	    return npMCI->dwTaskError;
	}

        if (!DoStreamUpdate(npMCI, FALSE)) {
	    DPF(("Put: Failed update, forcing restart....\n"));
	    npMCI->lFrameDrawn = (- (LONG) npMCI->wEarlyRecords) - 1;
	    fRestart = TRUE;
	}
	
	if (fRestart) {
            TEMPORARYSTATE  ts;

	    DPF2(("DevicePut: Stopping temporarily()\n"));

	    // !!! Set a flag here to prevent any more drawing
	    npMCI->fNoDrawing = TRUE;

            if (StopTemporarily(npMCI, &ts) != 0)
                return npMCI->dwTaskError;

	    // !!! We used to call InitDecompress here...
	    npMCI->dwFlags |= MCIAVI_NEEDDRAWBEGIN;

            RestartAgain(npMCI, &ts);

	    dw = npMCI->dwTaskError;
        }
    }

    return dw;
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api DWORD | DeviceSetPalette | Changes the override palette.
 *
 * @parm NPMCIGRAPHIC | npMCI | Pointer to instance data.
 *
 * @parm HPALETTE | hpal | New palette to use.
 *
 * @rdesc 0 means OK, otherwise mci error
 *
 ***************************************************************************/
DWORD FAR PASCAL DeviceSetPalette(NPMCIGRAPHIC npMCI, HPALETTE hpal)
{
//
//  You might think it is a good idea to allow the app to change the
//  the palette while playing; think again. This will break
//  MagicSchoolBus, and cause us to get into a infinite palette fight.
//
#if 0
    DWORD dw = 0L;
    TEMPORARYSTATE  ts;

    dw = StopTemporarily(npMCI, &ts);

    // Remember this for later.
    npMCI->hpal = hpal;

    if (!dw) {
        npMCI->dwFlags |= MCIAVI_NEEDDRAWBEGIN;
        dw = RestartAgain(npMCI, &ts);
    }

    return dw;
#else
    if (npMCI->hpal != hpal) {
        // Remember this for later.
        npMCI->hpal = hpal;
	// This won't happen until we restart the movie, so effectively, this
	// request for a palette change will be ignored for now.
        npMCI->dwFlags |= MCIAVI_NEEDDRAWBEGIN;
        InvalidateRect(npMCI->hwnd, NULL, TRUE);
    }
    return 0;
#endif
}

#ifndef LOADACTUALLYWORKS
/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api DWORD | DeviceLoad | Load a new AVI movie.
 *
 * @parm NPMCIGRAPHIC | npMCI | Pointer to instance data.
 *
 * @parm DWORD | dwFlags | MCI flags from command.
 *
 * @rdesc 0 means OK, otherwise mci error
 *
 ***************************************************************************/

DWORD PASCAL DeviceLoad(NPMCIGRAPHIC npMCI)
{
    DWORD   dw = 0L;

    if (!IsTask(npMCI->hTask))
	return MCIERR_DEVICE_NOT_READY;
	
    dw = DeviceStop(npMCI, MCI_WAIT);

    // Kill the current file and open a new file...

    mciaviTaskMessage(npMCI, TASKRELOAD);

    dw = npMCI->dwTaskError;

    return dw;
}
#endif
