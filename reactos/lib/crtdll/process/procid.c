#include <windows.h>
#include <msvcrt/process.h>

int _getpid (void)
{
	//fixme GetCurrentProcessId
        //return (int)GetCurrentProcessId();
	return 1;
}

