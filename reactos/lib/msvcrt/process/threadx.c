/* $Id: threadx.c,v 1.3 2003/07/11 21:58:09 royce Exp $
 *
 */
#include <windows.h>
#include <msvcrt/errno.h>
#include <msvcrt/process.h>


/*
 * @unimplemented
 */
unsigned long _beginthreadex(
    void* security,
    unsigned stack_size,
    unsigned (__stdcall *start_address)(void*),
    void* arglist,
    unsigned initflag,
    unsigned* thrdaddr)
{
  HANDLE NewThread;

  /*
   * Just call the API function. Any CRT specific processing is done in
   * DllMain DLL_THREAD_ATTACH
   */
  NewThread = CreateThread(security, stack_size, start_address, arglist, initflag, thrdaddr);
  if (NULL == NewThread)
    {
    /* FIXME map GetLastError() to errno */
    errno = ENOSYS;
    }

  return (unsigned long) NewThread;
}


/*
 * @implemented
 */
void _endthreadex(unsigned retval)
{
  /*
   * Just call the API function. Any CRT specific processing is done in
   * DllMain DLL_THREAD_DETACH
   */
  ExitThread(retval);
}

/* EOF */
