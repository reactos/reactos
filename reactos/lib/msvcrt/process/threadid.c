#include <windows.h>
#include <msvcrt/process.h>

unsigned long __threadid (void)
{
   return GetCurrentThreadId();
}

void *__threadhandle(void)
{
   return GetCurrentThread();
}
