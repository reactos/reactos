/*****************************************************************/
/**				 MPR Client/Server DLL Header File				**/
/**		      	Copyright (C) Microsoft Corp., 1994				**/
/*****************************************************************/ 

/* SCHED.H -- Header file for miscellaneous common scheduling primitives.
 *
 * History:
 *	gregj	10/17/94	created
 */


#ifndef _INC_SCHED
#define _INC_SCHED

#ifndef RC_INVOKED
#ifdef __cplusplus
extern "C" {
#endif

/* WaitAndYield processes all input messages.  WaitAndProcessSends only
 * processes SendMessages.
 *
 * WaitAndYield takes an optional parameter which is the ID of another
 * thread concerned with the waiting.  If it's not NULL, WM_QUIT messages
 * will be posted to that thread's queue when they are seen in the message
 * loop.
 */
DWORD WaitAndYield(HANDLE hObject, DWORD dwTimeout, volatile DWORD *pidOtherThread = NULL);
DWORD WaitAndProcessSends(HANDLE hObject, DWORD dwTimeout);

#ifdef __cplusplus
};	/* extern "C" */
#endif

#endif	/* RC_INVOKED */

#endif	/* _INC_SCHED */
