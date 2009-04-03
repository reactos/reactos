/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/process/cwait.c
 * PURPOSE:     Waits for a process to exit
 * PROGRAMER:   Ariadne
 * UPDATE HISTORY:
 *              04/03/99: Created
 */

#include <precomp.h>

/*
 * @implemented
 */
int _cwait(int* pnStatus, int hProc, int nAction)
{
    DWORD ExitCode;

	nAction = 0;
	if (WaitForSingleObject((void*)ULongToPtr(hProc), INFINITE) != WAIT_OBJECT_0) {
		__set_errno(ECHILD);
		return -1;
	}

	if (!GetExitCodeProcess((void*)ULongToPtr(hProc), &ExitCode))
		return -1;
	if (pnStatus != NULL)
        *pnStatus = (int)ExitCode;
    CloseHandle((HANDLE)ULongToPtr(hProc));
    return hProc;
}
