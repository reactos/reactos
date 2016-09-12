#include <precomp.h>

/*
 * @implemented
 */
void** __pxcptinfoptrs(void)
{
    return (void**)&msvcrt_get_thread_data()->xcptinfo;
}
