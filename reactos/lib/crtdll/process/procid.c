#include <windows.h>
#include <crtdll/process.h>

int _getpid (void)
{
	printf("get current processid\n");
        //return (int)GetCurrentProcessId();
}

