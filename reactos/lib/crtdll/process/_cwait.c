/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/process/cwait.c
 * PURPOSE:     Waits for a process to exit 
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              04/03/99: Created
 */
#include <windows.h>
#include <crtdll/process.h>
#include <crtdll/errno.h>
#include <crtdll/internal/file.h>

int	_cwait (int* pnStatus, int hProc, int nAction)
{
	nAction = 0;
	if ( WaitForSingleObject((void *)hProc,INFINITE) != WAIT_OBJECT_0 ) {
		__set_errno(ECHILD);
		return -1;
	}

	if ( !GetExitCodeProcess((void *)hProc,pnStatus) )
		return -1;
	return hProc;
}
