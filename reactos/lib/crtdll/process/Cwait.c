#include <process.h>
#include <windows.h>

int _cwait( int *termstat, int procHandle, int action )
{
	DWORD RetVal;
	RetVal = WaitForSingleObject((HANDLE)procHandle, INFINITE);
	if (RetVal == WAIT_FAILED || RetVal == WAIT_ABANDONED) {
		//errno = ECHILD;
		return -1;
	}
	if ( RetVal == WAIT_OBJECT_0 ) {
		GetExitCodeProcess((HANDLE)procHandle, termstat);
		return procHandle;
	}


	return -1;
	// WAIT_TIMEOUT
}