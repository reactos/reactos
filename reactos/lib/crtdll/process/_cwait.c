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
	return *pnStatus;
}