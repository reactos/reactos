#include <windows.h>
#include <msvcrt/process.h>

/*
 * @implemented
 */
unsigned long __threadid (void)
{
   return GetCurrentThreadId();
}

/*
 * @implemented
 */
void *__threadhandle(void)
{
   return GetCurrentThread();
}
