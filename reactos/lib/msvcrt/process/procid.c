#include <windows.h>
#include <msvcrt/process.h>

int _getpid (void)
{
   return (int)GetCurrentProcessId();
}

