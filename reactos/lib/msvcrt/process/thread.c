/* $Id: thread.c,v 1.6 2003/07/11 21:58:09 royce Exp $
 *
 */
#include <windows.h>
#include <msvcrt/errno.h>
#include <msvcrt/process.h>


/*
 * @unimplemented
 */
unsigned long _beginthread(
    void (__cdecl *start_address)(void*),
    unsigned stack_size,
    void* arglist)
{
    errno = ENOSYS;
    return (unsigned long)-1;
}

/*
 * @unimplemented
 */
void _endthread(void)
{
}

/* EOF */
