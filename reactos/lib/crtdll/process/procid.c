#include <windows.h>
#include <msvcrt/process.h>

/*
 * @implemented
 */
int _getpid (void)
{
   return (int)GetCurrentProcessId();
}

