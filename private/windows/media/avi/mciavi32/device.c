/******************************************************************************

   Copyright (C) Microsoft Corporation 1985-1995. All rights reserved.

   Title:   device.c - Multimedia Systems Media Control Interface
            driver for AVI.

*****************************************************************************/
#include "graphic.h"
#include "avitask.h"

#define ALIGNULONG(i)     ((i+3)&(~3))                  /* ULONG aligned ! */
#define WIDTHBYTES(i)     ((unsigned)((i+31)&(~31))/8)  /* ULONG aligned ! */
#define DIBWIDTHBYTES(bi) (DWORD)WIDTHBYTES((int)(bi).biWidth * (int)(bi).biBitCount)


// from wownt32.h
VOID (WINAPI * pWOWYield16)(VOID);



#ifdef DEBUG
    #define AssertUserThread(npMCI) 			\
	{  						\
	    DWORD thread = GetCurrentThreadId();	\
	    Assert((npMCI)->hTask != (HTASK)thread);	\
	    Assert(!((npMCI)->hwndDefault) || ((DWORD_PTR)GetWindowTask((npMCI)->hwndDefault) != thread));\
	}
#else
    #define AssertUserThread(npMCI)
#endif

/*
 * send a request to the worker thread, and wait for it to complete,
 * then return the result
 *
 * We must hold the CmdCritSec to stop other threads from making requests.
 *
 * if bDelayedComplete is true, the request is one that has two phases:
 *
 *   phase 1: 	initiating the operation (eg starting play). No other
 *		requests are permitted during this phase, so we hold the
 *		critical section and wait. No yielding of any sort is safe
 *		at this point, since re-entry on the same thread is not
 *		something we can handle well. This means that the worker
 *		thread must not do anything before setting hEventResponse
 *		that could block on our processing a sendmessage
 *
 *   phase 2:	while the play is taking place, we must process messages,
 *		yield to the app and allow other requests (eg stop).
 *		For this, we wait on a second event, timing out and yielding
 *              to the driver 10 times a second.
 *		
 */
DWORD
mciaviTaskRequest(
    NPMCIGRAPHIC npMCI,
    UINT message,
    DWORD dwFlags,
    LPARAM lParam,
    LPARAM dwCallback,
    BOOL bDelayedComplete
)
{
    DWORD dwRet;
    MSG msg;

#ifdef _WIN32
    // the gdi request queue is per-thread. We must flush the
    // app thread q here, or updates done to the window at the apps
    // request may appear before updates done by the app itself beforehand
    GdiFlush();
#endif

    // get the critsec that controls sending requests
    EnterCriticalSection(&npMCI->CmdCritSec);

    if (IsBadReadPtr(npMCI, sizeof(MCIGRAPHIC))) {
	// device has been closed beneath us!
	DPF(("help - npMCI has gone away"));
	// not safe to leave critsec or dec count
	return MCIERR_DEVICE_NOT_READY;
    }

    if (npMCI->EntryCount++ > 0) {
        DPF(("re-entering requestor on same thread (SendMessage?)"));
	//DebugBreak();
        npMCI->EntryCount--;
        LeaveCriticalSection(&npMCI->CmdCritSec);

        return MCIERR_DEVICE_NOT_READY;
	//return 0;
    }


    if (!IsTask(npMCI->hTask)) {
	// worker thread has gone away (previous close ?)
	npMCI->EntryCount--;
	LeaveCriticalSection(&npMCI->CmdCritSec);
    	DPF(("worker thread has gone away"));
	return MCIERR_DEVICE_NOT_READY;
    }

    // the response event should not be set yet!
    Assert(WaitForSingleObject(npMCI->hEventResponse, 0) == WAIT_TIMEOUT);


    // write the params
    npMCI->message = message;
    npMCI->dwParamFlags = dwFlags;
    npMCI->lParam = lParam;
    npMCI->dwReqCallback = dwCallback;
    npMCI->bDelayedComplete = bDelayedComplete;

    // we are the requesting task (we will be thrown out if this is
    // bDelayedComplete and there is an outstanding bDelayedComplete
    // from someone else)
    npMCI->hRequestor = GetCurrentTask();

    // signal that there is a request
    SetEvent(npMCI->hEventSend);

    // and wait for the response.
    //
    // in the play-wait case, this wait will complete once the play
    // has started. So at this point, no yields.

    // send-message processing needed for RealizePalette on worker thread
#if 1
    // this could cause re-entry on this thread, and the critical section
    // will not prevent that. Hence the EntryCount checks.
    while (MsgWaitForMultipleObjects(1, &npMCI->hEventResponse, FALSE,
            INFINITE, QS_SENDMESSAGE) != WAIT_OBJECT_0) {
        DPF2(("rec'd sendmessage during wait\n"));

	// this peekmessage allows an inter-thread sendmessage to complete.
	// no message needs to be removed or processed- the range filtering is
	// essentially irrelevant for this.
        PeekMessage(&msg, NULL, WM_QUERYNEWPALETTE, WM_QUERYNEWPALETTE, PM_NOREMOVE);

    }
#else
    WaitForSingleObject(npMCI->hEventResponse, INFINITE);
#endif

    // pick up the return value
    dwRet = npMCI->dwReturn;
    DPF2(("Task returns %d\n", dwRet));

    // release the critsec now that request is all done
    if (--npMCI->EntryCount != 0) {
	DPF(("EntryCount not 0 on exit"));
    }
    LeaveCriticalSection(&npMCI->CmdCritSec);

    // if this is a two-phased operation such as play + wait
    // we must do the yielding wait here
    if (!dwRet && bDelayedComplete) {
	DWORD dw;
	UINT  nYieldInterval = 300;
#ifdef DEBUG
        nYieldInterval = mmGetProfileInt(szIni, TEXT("YieldInterval"), nYieldInterval);
#endif
	do {
	    if (mciDriverYield(npMCI->wDevID)) {

		// app says we must stop now. do this by issuing a stop
		// request and carry on waiting for the play+wait to finish
		mciaviTaskRequest(npMCI, AVI_STOP, 0, 0, 0, FALSE);
	    }

	    dw = WaitForSingleObject(npMCI->hEventAllDone, nYieldInterval);

	    // this peekmessage allows an inter-thread sendmessage to complete.
	    // no message needs to be removed or processed- the range filtering is
	    // essentially irrelevant for this.
	    PeekMessage(&msg, NULL, WM_QUERYNEWPALETTE, WM_QUERYNEWPALETTE, PM_NOREMOVE);

	} while(dw != WAIT_OBJECT_0);

	// until this is cleared, no other task can issue delayed requests
	npMCI->hWaiter = 0;
    }

    return dwRet;
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
    DWORD	dwRet;

    AssertUserThread(npMCI);

    // init the yield proc we will need for wow yielding
    if (IsNTWOW()) {
	if (pWOWYield16 == 0) {

	    HMODULE hmod;

	    hmod = GetModuleHandle(TEXT("wow32.dll"));
	    if (hmod != NULL) {
		(FARPROC)pWOWYield16 = GetProcAddress(hmod, "WOWYield16");
	    }
	}
    }



    // note that DeviceClose *will* be called anyway, even if DeviceOpen
    // fails, so make sure that allocations and events can be cleaned up
    // correctly.

    npMCI->uErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS |
				     SEM_NOOPENFILEERRORBOX);

    // must open the file on this app thread for ole reasons
    if (!mciaviOpenFile(npMCI)) {
	SetErrorMode(npMCI->uErrorMode);
	return npMCI->dwTaskError;
    }
    // OpenFileInit() on worker thread completes this open later.


    // create the communication channel to the worker thread, and
    // then start the thread

    // do this first, so that whenever we call DeviceClose we can always
    // safely do the Delete..
    InitializeCriticalSection(&npMCI->CmdCritSec);
    SetNTFlags(npMCI, NTF_DELETECMDCRITSEC);   // Remember to do the delete
    npMCI->EntryCount = 0;

    // must be manual-reset to allow polling during play.
    npMCI->hEventSend = CreateEvent(NULL, TRUE, FALSE, NULL);

    npMCI->hEventResponse = CreateEvent(NULL, FALSE, FALSE, NULL);
    npMCI->hEventAllDone = CreateEvent(NULL, FALSE, FALSE, NULL);

    if (!npMCI->hEventSend || !npMCI->hEventResponse || !npMCI->hEventAllDone) {

	// cleanup of events actually allocated will be done in DeviceClose
	return MCIERR_OUT_OF_MEMORY;
    }


    // create the worker thread

#if 0
    if (mmTaskCreate(mciaviTask, &npMCI->hThreadTermination,
	    (DWORD)(UINT)npMCI) == 0)
#else
    // We do not want the thread id, but CreateThread blows up if we pass
    // a null parameter.  Hence overload dwRet...
    if (npMCI->hThreadTermination = CreateThread(NULL, 0,
					(LPTHREAD_START_ROUTINE)mciaviTask,
					(LPVOID)npMCI, 0, &dwRet))

#endif
    {
	// check that the thread is actually created

	// either hEventResponse will be set, indicating that
	// the thread completed, or hThreadTermination will be
	// set, indicating that the thread aborted.
#if 0
	if (WaitForMultipleObjects(2,
	    &npMCI->hEventResponse, FALSE, INFINITE) == WAIT_OBJECT_0)
	{

	    // task completed ok
	    Assert(IsTask(npMCI->hTask));
	}
#else
	// We must process messages during this phase... IF messages
	// must be processed by this thread before the AVI window can
	// be created.  The most likely case is when a parent window
	// is passed and the parent window belongs to this (the UI)
	// thread.  If no messages need to be processed (i.e. the AVI
	// window being created has no parent) then we could use the
	// simpler code above.  DO THIS LATER.

	UINT n;

	while (WAIT_OBJECT_0+2 <= (n = MsgWaitForMultipleObjects(2,
	    &npMCI->hEventResponse, FALSE, INFINITE, QS_SENDMESSAGE)))
	{

	    MSG msg;
	    if (n!=WAIT_OBJECT_0+2) {
	        DPF0(("MsgWaitForMultipleObjects gave an unexpected return of %d\n", n));
	    }
	    PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);
	    // PeekMessage with PM_NOREMOVE causes the inter thread
	    // sent messages to be processed
	}

	dwRet = 0;
	if (n == WAIT_OBJECT_0) {

	    // task completed ok
	    Assert(IsTask(npMCI->hTask));
	}
#endif
	else {
	    // hThreadTermination has been signalled - abort
	    CloseHandle(npMCI->hThreadTermination);
            npMCI->hThreadTermination = 0;
	    dwRet = npMCI->dwTaskError;
	    Assert(dwRet);
	}
    } else {
        npMCI->hTask = 0;
        dwRet = MCIERR_OUT_OF_MEMORY;
	npMCI->dwTaskError = GetLastError();
    }

    SetErrorMode(npMCI->uErrorMode);

    if (dwRet != 0) {
	// open failed - the necessary cleanup will be done in DeviceClose
	// which will be called after a bad return from DeviceOpen.  In
	// fact graphic.c (which calls DeviceOpen) will call GraphicClose
	// when DeviceOpen fails.  GraphicClose will then call DeviceClose
	// which will delete the cmdCritSec
    }

    return dwRet;
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

	AssertUserThread(npMCI);
	// tell the worker to close and wait for it to happen
	mciaviTaskRequest(npMCI, AVI_CLOSE, 0, 0, 0, FALSE);
    }

    // must wait for thread to exit
    if (npMCI->hThreadTermination != 0) {

        /*
        ** Wait for the thread to complete so the DLL doesn't get unloaded
        ** while it's still executing code in that thread
        */

	// we must allow sendmessage at this point since the winproc thread
	// will block until it can send messages to our thread, and we are
	// waiting for the winproc thread to exit.
	// do not do this between setting hEventSend and receiving hEventResponse
	// though or we could re-enter the Request block and get confused
	// about whether we have seen hEventResponse.

	// we also need to yield in case we are on a wow thread - any
	// interthread sendmessage to another wow thread will block until
	// we yield here allowing other wow threads to run
	
	DWORD dw;

	do {

	    if (pWOWYield16) {
		pWOWYield16();
	    }

	    dw = MsgWaitForMultipleObjects(
		    1,
		    &npMCI->hThreadTermination,
		    FALSE,
		    100,
		    QS_SENDMESSAGE);

	
	    if (dw == WAIT_OBJECT_0 + 1) {

                MSG msg;
                DPF2(("rec'd sendmessage during shutdown wait\n"));

                // just a single peekmessage with NOREMOVE will
                // process the inter-thread send and not affect the queue
                PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);
	    }
        } while (dw != WAIT_OBJECT_0);

	CloseHandle(npMCI->hThreadTermination);
        npMCI->hThreadTermination = 0;

    }

    if (TestNTFlags(npMCI, NTF_DELETECMDCRITSEC)) {
        DeleteCriticalSection(&npMCI->CmdCritSec);
    }
    if (npMCI->hEventSend) {
	CloseHandle(npMCI->hEventSend);
    }
    if (npMCI->hEventAllDone) {
	CloseHandle(npMCI->hEventAllDone);
    }
    if (npMCI->hEventResponse) {
	CloseHandle(npMCI->hEventResponse);
    }

    // uninitialize AVIFile and hence OLE - must be done on app thread
#ifdef USEAVIFILE
    //
    // we must do this so COMPOBJ will shut down right.
    //
    FreeAVIFile(npMCI);
#endif



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
 * @parm LPMCI_DGV_PLAY_PARMS | lpPlay | Parameters for the play message.
 *
 * @rdesc 0 means OK, otherwise mci error
 *
 ***************************************************************************/

DWORD PASCAL
DevicePlay(
    NPMCIGRAPHIC npMCI,
    DWORD dwFlags,
    LPMCI_DGV_PLAY_PARMS lpPlay,
    LPARAM dwCallback
)
{
    BOOL bWait = FALSE;
    DWORD dwErr;

    if (!IsTask(npMCI->hTask))
	return MCIERR_DEVICE_NOT_READY;

    // all handled by the worker thread
    AssertUserThread(npMCI);

    if (dwFlags & MCI_WAIT) {
	bWait = TRUE;
    }
    dwErr =  mciaviTaskRequest(npMCI,
	      AVI_PLAY, dwFlags, (LPARAM) lpPlay, dwCallback, bWait);

    if (dwFlags & (MCI_MCIAVI_PLAY_FULLSCREEN | MCI_MCIAVI_PLAY_FULLBY2)) {
	MSG	msg;
	
        DPF(("DevicePlay, removing stray messages\n"));
	/* Remove stray mouse and keyboard events after DispDib. */
	while (PeekMessage(&msg, NULL, WM_KEYFIRST, WM_KEYLAST,
					PM_NOYIELD | PM_REMOVE) ||
			PeekMessage(&msg, NULL, WM_MOUSEFIRST, WM_MOUSELAST,
					PM_NOYIELD | PM_REMOVE))
	    ;
    }

    return dwErr;
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
    BOOL bWait = FALSE;

    if (!IsTask(npMCI->hTask))
	return MCIERR_DEVICE_NOT_READY;

    // all handled by the worker thread
    AssertUserThread(npMCI);

    return mciaviTaskRequest(npMCI, AVI_REALIZE, 0, 0, 0, FALSE);

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

    if (!IsTask(npMCI->hTask)) {
        DPF0(("DeviceStop called on a dead task, npMCI=%8x\n", npMCI));
	return MCIERR_DEVICE_NOT_READY;
    }
	
    AssertUserThread(npMCI);
    return mciaviTaskRequest(npMCI, AVI_STOP, 0, 0, 0, FALSE);
}


/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api DWORD | DeviceUpdate | Updates the frame into the given DC
 *
 * @parm NPMCIGRAPHIC | npMCI | Pointer to instance data block.
 *
 * @rdesc 0 means OK, otherwise mci error
 *
 ***************************************************************************/

DWORD PASCAL DeviceUpdate(
    NPMCIGRAPHIC npMCI,
    DWORD dwFlags,
    LPMCI_DGV_UPDATE_PARMS lpParms)
{

    if (!IsTask(npMCI->hTask))
        return MCIERR_DEVICE_NOT_READY;


    AssertUserThread(npMCI);
    return mciaviTaskRequest(npMCI, AVI_UPDATE, dwFlags, (LPARAM) lpParms, 0, FALSE);


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

DWORD PASCAL DevicePause(NPMCIGRAPHIC npMCI, DWORD dwFlags, LPARAM dwCallback)
{
    if (!IsTask(npMCI->hTask))
        return MCIERR_DEVICE_NOT_READY;

    AssertUserThread(npMCI);
    return mciaviTaskRequest(npMCI, AVI_PAUSE, dwFlags, 0, dwCallback,
	    (dwFlags & MCI_WAIT));

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

DWORD PASCAL DeviceCue(NPMCIGRAPHIC npMCI, LONG lTo, DWORD dwFlags, LPARAM dwCallback)
{

    if (!IsTask(npMCI->hTask))
        return MCIERR_DEVICE_NOT_READY;

    AssertUserThread(npMCI);
    return mciaviTaskRequest(npMCI, AVI_CUE, dwFlags, lTo, dwCallback,
	    (dwFlags & MCI_WAIT));

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

DWORD PASCAL DeviceResume(NPMCIGRAPHIC npMCI, DWORD dwFlags, LPARAM dwCallback)
{
    DWORD   dw = 0L;

    BOOL bWait = FALSE;

    if (!IsTask(npMCI->hTask))
	return MCIERR_DEVICE_NOT_READY;

    // all handled by the worker thread
    AssertUserThread(npMCI);

    if (dwFlags & MCI_WAIT) {
	bWait = TRUE;
    }
    return  mciaviTaskRequest(npMCI,
	      AVI_RESUME, dwFlags, 0, dwCallback, bWait);
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

DWORD PASCAL DeviceSeek(NPMCIGRAPHIC npMCI, LONG lTo, DWORD dwFlags, LPARAM dwCallback)
{
    if (!IsTask(npMCI->hTask))
        return MCIERR_DEVICE_NOT_READY;


    AssertUserThread(npMCI);
    return mciaviTaskRequest(npMCI, AVI_SEEK, dwFlags, lTo, dwCallback,
	    (dwFlags & MCI_WAIT));
}



/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api DWORD | DeviceSetActive | is the movie active?
 *
 * @parm NPMCIGRAPHIC | npMCI | Pointer to instance data block.
 *
 * @rdesc 0 means OK, otherwise mci error
 *
 ***************************************************************************/

DWORD PASCAL DeviceSetActive(NPMCIGRAPHIC npMCI, BOOL fActive)
{
    // We cannot call   AssertUserThread(npMCI);
    // This routine is called on the winproc thread, as well as the user
    // thread.

    if (fActive)
        // We must explicitly request a unicode string.  %s will not
        // work as dprintf uses wvsprintfA
        DPF(("**** '%ls' is active.\n", (LPTSTR)npMCI->szFilename));

    return 0;
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

    // there is no point in synchronizing with the worker thread for
    // this since the task state will be transient anyway.
    // just grab a snapshot and return that.
    AssertUserThread(npMCI);

    switch (npMCI->wTaskState) {
	case TASKIDLE:	
	    return MCI_MODE_STOP;
	
        case TASKCUEING:	

	    // problem: some apps (notably mplayer) will be surprised to
	    // get MCI_MODE_SEEK immediately after issuing a PLAY command.
	    // on win-16 the yielding model meant that the app would not
	    // normally get control back until after the play proper had
	    // started and so would never see the cueing state.

	    // to avoid this confusion (and the bugs that arise from it), we
	    // never return MCI_MODE_SEEK: we report this mode as playing.
	    // this is often what would be seen on win-16 anyway (even in the
	    // case of a PLAY command ?).

            // Except... for apps that really do seek this can fool them into
            // thinking that they are playing.  So... we modify the algorithm
            // to return MODE_SEEK if lTo==lFrom (why is obvious) OR if
            // lRealStart==lTo.  This latter is because if you seek in mplayer
            // by dragging the thumb the image is only updated every key frame.
            // lRealStart is updated to this key frame while seeking

            //DPF0(("F: %8x, To=%d,  From=%d  lReal=%d lDrawn=%d Current=%d\n",
            //        npMCI->dwFlags, npMCI->lTo, npMCI->lFrom, npMCI->lRealStart, npMCI->lFrameDrawn, npMCI->lCurrentFrame));
            if ((npMCI->lTo == npMCI->lFrom)
                || (npMCI->lTo == npMCI->lRealStart)) {
                return(MCI_MODE_SEEK);
            }
	    return MCI_MODE_PLAY;
	
	case TASKSTARTING:	// ready? of course we're ready
	case TASKPLAYING:	
	    return MCI_MODE_PLAY;
	
	case TASKPAUSED:	
	    return MCI_MODE_PAUSE;
	
	default:
            DPF(("Unexpected state %d in DeviceMode()\n", npMCI->wTaskState));
            // fall through to the known states
	//case TASKBEINGCREATED:	
	//case TASKINIT:	
	case TASKCLOSE:	
	//case TASKREADINDEX:	
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

    // read a snapshot of the current state without
    // synchronising with the worker thread!

    AssertUserThread(npMCI);
    return InternalGetPosition(npMCI, lpl);
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
    if (!IsTask(npMCI->hTask))
        return MCIERR_DEVICE_NOT_READY;


    AssertUserThread(npMCI);
    return mciaviTaskRequest(npMCI, AVI_WINDOW, 0, (LPARAM) hwnd, 0, FALSE);
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
 *      ** It is if you paused to change the speed then try and resume **
 *
 ***************************************************************************/

DWORD PASCAL DeviceSetSpeed(NPMCIGRAPHIC npMCI, DWORD dwNewSpeed)
{

    if (!IsTask(npMCI->hTask))
        return MCIERR_DEVICE_NOT_READY;


    AssertUserThread(npMCI);
    return mciaviTaskRequest(npMCI, AVI_SETSPEED, 0, (LPARAM) dwNewSpeed, 0, FALSE);
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

    if (!IsTask(npMCI->hTask))
        return MCIERR_DEVICE_NOT_READY;

    AssertUserThread(npMCI);
    return mciaviTaskRequest(npMCI, AVI_MUTE, 0, (LPARAM) fMute, 0, FALSE);
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

    // switch audio off completely if setting volume level to 0, and back on
    // again if not.
    dw = DeviceMute(npMCI, (dwVolume == 0));
    if (dw != 0) {
	return dw;
    }

    AssertUserThread(npMCI);
    return mciaviTaskRequest(npMCI, AVI_SETVOLUME, 0, (LPARAM) dwVolume, 0, FALSE);
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
    // all reference to the hWave *must* be done on the worker thread

    AssertUserThread(npMCI);
    return mciaviTaskRequest(npMCI, AVI_GETVOLUME, 0, 0, 0, FALSE);
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
    AssertUserThread(npMCI);
    return mciaviTaskRequest(npMCI, AVI_AUDIOSTREAM, wAudioStream, 0, 0, FALSE);
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
    AssertUserThread(npMCI);
    return mciaviTaskRequest(npMCI, AVI_AUDIOSTREAM, uStream, (BOOL)fOn, 0, FALSE);
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
    AssertUserThread(npMCI);
    return mciaviTaskRequest(npMCI, AVI_PUT, dwFlags, (LPARAM)lprc, 0, FALSE);
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
    AssertUserThread(npMCI);
    return mciaviTaskRequest(npMCI, AVI_PALETTE, 0, (LPARAM) hpal, 0, FALSE);
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api DWORD | DeviceSetPaletteColor | Changes the a single color
 *	in the movie's palette.
 *
 * @parm NPMCIGRAPHIC | npMCI | Pointer to instance data.
 *
 * @parm DWORD | index | color index to change
 *
 * @parm DWORD | color | color value to use
 *
 * @rdesc 0 means OK, otherwise mci error
 *
 ***************************************************************************/
DWORD FAR PASCAL DeviceSetPaletteColor(NPMCIGRAPHIC npMCI, DWORD index, DWORD color)
{

    AssertUserThread(npMCI);
    return mciaviTaskRequest(npMCI, AVI_PALETTECOLOR, color, (LPARAM) index, 0, FALSE);
}

//
// user-thread version of ResetDestRect - note that there is a similar
// winproc-thread-only version in window.c
//
void FAR PASCAL ResetDestRect(NPMCIGRAPHIC npMCI, BOOL fUseDefaultSizing)
{
    RECT    rc;

    /* WM_SIZE messages (on NT at least) are sometimes sent
     * during CreateWindow processing (eg if the initial window size
     * is not CW_DEFAULT). Some fields in npMCI are only filled in
     * after CreateWindow has returned. So there is a danger that at this
     * point some fields are not valid.
     */

    if (npMCI->hwndPlayback &&
        npMCI->hwndPlayback == npMCI->hwndDefault &&
        (npMCI->dwOptionFlags & MCIAVIO_STRETCHTOWINDOW)) {
        GetClientRect(npMCI->hwndPlayback, &rc);
    }

    // Only allow ZOOMBY2 and fixed % defaults for our default playback window
    else if ((npMCI->streams > 0) && (npMCI->hwndPlayback == npMCI->hwndDefault)) {
        rc = npMCI->rcMovie;

		if (fUseDefaultSizing)
			AlterRectUsingDefaults(npMCI, &rc);
    }
    else {
        return;
    }

    if (!IsRectEmpty(&rc)) {
		DevicePut(npMCI, &rc, MCI_DGV_PUT_DESTINATION);
    }
}


