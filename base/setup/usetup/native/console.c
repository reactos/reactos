#include <usetup.h>

BOOLEAN
NATIVE_InitConsole(VOID)
{
    return (BOOLEAN)AllocConsole();
}
