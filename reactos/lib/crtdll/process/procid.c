#include <windows.h>
#include <crtdll/process.h>

int _getpid (void)
{
	//fixme GetCurrentProcessId
        //return (int)GetCurrentProcessId();
	return 1;
}

