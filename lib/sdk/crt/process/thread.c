#include <precomp.h>

#if 0
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
#endif
/*
 * @unimplemented
 */
void _endthread(void)
{
}

/* EOF */
