#include <windows.h>
#include <crtdll/process.h>

unsigned long __threadid (void)
{
   return GetCurrentThreadId();
}

void *__threadhandle(void)
{
   return GetCurrentThread();
}