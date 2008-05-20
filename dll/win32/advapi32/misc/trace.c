/*
 * Advapi32.dll Event Tracing Functions
 */

#include <advapi32.h>
#include <debug.h>

/*
 * @unimplemented
 */
ULONG CDECL
TraceMessage(
    HANDLE       SessionHandle,
    ULONG        MessageFlags,
    LPGUID       MessageGuid,
    USHORT       MessageNumber,
    ...)
{
    DPRINT1("TraceMessage()\n");
    return ERROR_SUCCESS;
}

/* EOF */
