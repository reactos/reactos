
#include <msvcrt/internal/rterror.h>

/*
 * @implemented
 */
void _purecall(void)
{
    _amsg_exit(_RT_PUREVIRT);
}
