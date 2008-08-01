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

ULONG
WMIAPI
GetTraceEnableFlags(
    TRACEHANDLE TraceHandle
)
{
    DPRINT1("GetTraceEnableFlags stub()\n");
    return 0xFF;
}

UCHAR
WMIAPI
GetTraceEnableLevel(
    TRACEHANDLE TraceHandle
)
{
    DPRINT1("GetTraceEnableLevel stub()\n");
    return 0xFF;
}

ULONG
WMIAPI
UnregisterTraceGuids(
    TRACEHANDLE RegistrationHandle
)
{
    DPRINT1("UnregisterTraceGuids stub()\n");
    return ERROR_SUCCESS;
}

ULONG
WMIAPI
RegisterTraceGuidsA(
    WMIDPREQUEST RequestAddress,
    PVOID RequestContext,
    LPCGUID ControlGuid,
    ULONG GuidCount,
    PTRACE_GUID_REGISTRATION TraceGuidReg,
    LPCSTR MofImagePath,
    LPCSTR MofResourceName,
    PTRACEHANDLE RegistrationHandle
)
{
    DPRINT1("RegisterTraceGuidsA stub()\n");
    return ERROR_SUCCESS;
}

ULONG
WMIAPI
RegisterTraceGuidsW(
    WMIDPREQUEST RequestAddress,
    PVOID RequestContext,
    LPCGUID ControlGuid,
    ULONG GuidCount,
    PTRACE_GUID_REGISTRATION TraceGuidReg,
    LPCWSTR MofImagePath,
    LPCWSTR MofResourceName,
    PTRACEHANDLE RegistrationHandle
)
{
    DPRINT1("RegisterTraceGuidsW stub()\n");
    return ERROR_SUCCESS;
}


/* EOF */
