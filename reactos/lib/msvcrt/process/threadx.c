/* $Id: threadx.c,v 1.1 2002/11/29 16:00:21 robd Exp $
 *
 */
#include <windows.h>
#include <msvcrt/errno.h>
#include <msvcrt/process.h>


unsigned long _beginthreadex(
    void* security,
    unsigned stack_size,
    unsigned (__stdcall *start_address)(void*),
    void* arglist,
    unsigned initflag,
    unsigned* thrdaddr)
{
    errno = ENOSYS;
    return (unsigned long)-1;
}


void _endthreadex(unsigned retval)
{
}

/* EOF */
