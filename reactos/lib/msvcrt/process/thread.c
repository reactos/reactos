/* $Id: thread.c,v 1.7 2003/07/16 02:45:24 royce Exp $
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
    __set_errno ( ENOSYS );
    return (unsigned long)-1;
}

/*
 * @unimplemented
 */
void _endthread(void)
{
}

/* EOF */
