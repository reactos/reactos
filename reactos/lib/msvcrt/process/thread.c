/* $Id: thread.c,v 1.5 2002/11/29 15:59:04 robd Exp $
 *
 */
#include <windows.h>
#include <msvcrt/errno.h>
#include <msvcrt/process.h>


unsigned long _beginthread(
    void (__cdecl *start_address)(void*),
    unsigned stack_size,
    void* arglist)
{
    errno = ENOSYS;
    return (unsigned long)-1;
}

void _endthread(void)
{
}

/* EOF */
