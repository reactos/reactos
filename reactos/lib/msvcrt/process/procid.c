#include <windows.h>
#include <crtdll/process.h>

int _getpid (void)
{
   return (int)GetCurrentProcessId();
}

