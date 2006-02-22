#include <precomp.h>
#include <process.h>

/*
 * @implemented
 */
int _getpid (void)
{
   return (int)GetCurrentProcessId();
}

