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

TRACEHANDLE
WMIAPI
GetTraceLoggerHandle(
    PVOID Buffer
)
{
    DPRINT1("GetTraceLoggerHandle stub()\n");
    return (TRACEHANDLE)INVALID_HANDLE_VALUE;
}


ULONG
WMIAPI
TraceEvent(
    TRACEHANDLE SessionHandle,
    PEVENT_TRACE_HEADER EventTrace
)
{
    DPRINT1("TraceEvent stub()\n");

    if (!SessionHandle || !EventTrace)
    {
        /* invalid parameters */
        return ERROR_INVALID_PARAMETER;
    }

    if (EventTrace->Size != sizeof(EVENT_TRACE_HEADER))
    {
        /* invalid parameter */
        return ERROR_INVALID_PARAMETER;
    }

    return ERROR_SUCCESS;
}


/* EOF */
