/*	File: D:\WACKER\tdll\engnthrd.c (Created: 04-Dec-1993)
 *
 *	Copyright 1993 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:39p $
 */

#include <windows.h>

#include "stdtyp.h"
#include "assert.h"
#include "session.h"
#include <tdll\cloop.h>

DWORD WINAPI EngineThread(const HSESSION hSession);

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	CreateEngineThread
 *
 * DESCRIPTION:
 *	Creates the communications thread (What a surpise!).
 *
 * ARGUMENTS:
 *	hSession	- session handle
 *
 * RETURNS:
 *	0=OK, 1=error.
 *
 */
int CreateEngineThread(const HSESSION hSession)
	{
	DWORD dwThreadId;
	HANDLE hThread;

	if (sessQueryEngineThreadHdl(hSession))
		return 1;

	hThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)EngineThread,
		hSession, CREATE_SUSPENDED, &dwThreadId);

	if (hThread == 0)
		{
		assert(FALSE);
		return 1;
		}

	sessSetEngineThreadHdl(hSession, hThread);
	ResumeThread(hThread);
	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	DestoryEngineThread
 *
 * DESCRIPTION:
 *	Terminates the engine thread gracefully
 *
 * ARGUMENTS:
 *	hSession	- hSession
 *
 * RETURNS:
 *	void
 *
 */
void DestroyEngineThread(const HSESSION hSession)
	{
	sessSetEngineThreadHdl(hSession, 0);
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	EngineThread
 *
 * DESCRIPTION:
 *	Creates the Engine thread.
 *
 *	Important:	This function must be declared WINAPI and DWORD return
 *				because it is essentially a callback function.
 *
 * ARGUMENTS:
 *	hSession	- session handle
 *
 * RETURNS:
 *	0
 *
 */
DWORD WINAPI EngineThread(const HSESSION hSession)
	{
	CLoop(sessQueryCLoopHdl(hSession));
	return 0;
	}
