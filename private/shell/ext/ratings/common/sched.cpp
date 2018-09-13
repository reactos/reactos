/*****************************************************************/
/**		      	Copyright (C) Microsoft Corp., 1994				**/
/*****************************************************************/ 

/* SCHED.C -- Miscellaneous scheduling helpers
 *
 * History:
 *	gregj	10/17/94	created
 */


#include "npcommon.h"
#include <sched.h>

/* FlushInputQueue is a private routine to collect and dispatch all
 * messages in the input queue.  It returns TRUE if a WM_QUIT message
 * was detected in the queue, FALSE otherwise.
 */
BOOL FlushInputQueue(volatile DWORD *pidOtherThread)
{
	MSG msgTemp;
	while (PeekMessage(&msgTemp, NULL, 0, 0, PM_REMOVE)) {
		DispatchMessage(&msgTemp);

		// If we see a WM_QUIT in the queue, we need to do the same
		// sort of thing that a modal dialog does:  break out of our
		// waiting, and repost the WM_QUIT to the queue so that the
		// next message loop up in the app will also see it.  We also
		// post the message to the server thread's queue so that any
		// dialog stack displayed there will be destroyed as well.
		if (msgTemp.message == WM_QUIT) {
			if (pidOtherThread != NULL && *pidOtherThread != NULL) {
				PostThreadMessage(*pidOtherThread, msgTemp.message, msgTemp.wParam, msgTemp.lParam);
			}
			PostQuitMessage((int)msgTemp.wParam);
			return TRUE;
		}
	}
	return FALSE;
}


/* WaitAndYield() waits for the specified object using
 * MsgWaitForMultipleObjects.  If messages are received,
 * they are dispatched and waiting continues.  The return
 * value is the same as from MsgWaitForMultipleObjects.
 */
DWORD WaitAndYield(HANDLE hObject, DWORD dwTimeout, volatile DWORD *pidOtherThread /* = NULL */)
{
	DWORD dwTickCount, dwWakeReason, dwTemp;

	do {
		/* Flush any messages before we wait.  This is because
		 * MsgWaitForMultipleObjects will only return when NEW
		 * messages are put in the queue.
		 */
		if (FlushInputQueue(pidOtherThread)) {
			dwWakeReason = WAIT_TIMEOUT;
			break;
		}

    	// in case we handle messages, we want close to a true timeout
   		if ((dwTimeout != 0) && 
			(dwTimeout != (DWORD)-1)) {
   			// if we can timeout, store the current tick count
    		// every time through
   			dwTickCount = GetTickCount();
		}
		dwWakeReason = MsgWaitForMultipleObjects(1,
												 &hObject,
												 FALSE,
												 dwTimeout,
												 QS_ALLINPUT);
	    // if we got a message, dispatch it, then try again
	    if (dwWakeReason == 1) {
			// if we can timeout, see if we did before processing the message
			// that way, if we haven't timed out yet, we'll get at least one
			// more shot at the event
			if ((dwTimeout != 0) && 
			    (dwTimeout != (DWORD)-1)) {
			    if ((dwTemp = (GetTickCount()-dwTickCount)) >= dwTimeout) {
					// if we timed out, make us drop through
					dwWakeReason = WAIT_TIMEOUT;
				} else {
					// subtract elapsed time from timeout and continue
					// (we don't count time spent dispatching message)
					dwTimeout -= dwTemp;
				}
			}
			if (FlushInputQueue(pidOtherThread)) {
				dwWakeReason = WAIT_TIMEOUT;
				break;
			}
	    }
	} while (dwWakeReason == 1);

	return dwWakeReason;
}


/* WaitAndProcessSends is similar to WaitAndYield, but it only processes
 * SendMessage messages, not input messages.
 */
DWORD WaitAndProcessSends(HANDLE hObject, DWORD dwTimeout)
{
	DWORD dwWakeReason;

	do {
		dwWakeReason = MsgWaitForMultipleObjects(1,
												 &hObject,
												 FALSE,
												 dwTimeout,
												 QS_SENDMESSAGE);
	    // if we got a message, yield, then try again
	    if (dwWakeReason == 1) {
			MSG msgTemp;
			PeekMessage(&msgTemp, NULL, 0, 0, PM_NOREMOVE | PM_NOYIELD);
	    }
	} while (dwWakeReason == 1);

	return dwWakeReason;
}
