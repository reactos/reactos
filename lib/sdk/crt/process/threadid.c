#include <precomp.h>
#include <process.h>


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
uintptr_t __threadhandle()
{
   return (uintptr_t)GetCurrentThread();
}
