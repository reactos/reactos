/* $Id: thread.c,v 1.8 2003/11/14 17:13:31 weiden Exp $
 *
 */
#include <windows.h>
#include <msvcrt/errno.h>
#include <msvcrt/process.h>
#include <msvcrt/internal/file.h>


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
