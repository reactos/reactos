/*
    ReactOS Kernel Streaming
    Port Class / Library Init and Cleanup

    Author: Andrew Greenwood

    Notes:
    -
*/

#include <ntddk.h>

/*
 * @implemented
 */
ULONG STDCALL
DllInitialize(ULONG Unknown)
{
    return 0;
}

/*
 * @implemented
 */
ULONG STDCALL
DllUnload(VOID)
{
    return 0;
}
