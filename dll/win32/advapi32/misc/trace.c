/*
 * Advapi32.dll Event Tracing Functions
 */

#include <advapi32.h>

#include <wmistr.h>
#include <evntrace.h>

WINE_DEFAULT_DEBUG_CHANNEL(advapi);
/*
 * @unimplemented
 */
ULONG CDECL
TraceMessage(
    TRACEHANDLE  SessionHandle,
    ULONG        MessageFlags,
    LPCGUID      MessageGuid,
    USHORT       MessageNumber,
    ...)
{
    FIXME("TraceMessage()\n");
    return ERROR_SUCCESS;
}

TRACEHANDLE
WMIAPI
GetTraceLoggerHandle(
    PVOID Buffer
)
{
    FIXME("GetTraceLoggerHandle stub()\n");
    return (TRACEHANDLE)-1;
}


ULONG
WMIAPI
TraceEvent(
    TRACEHANDLE SessionHandle,
    PEVENT_TRACE_HEADER EventTrace
)
{
    FIXME("TraceEvent stub()\n");

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
    FIXME("GetTraceEnableFlags stub()\n");
    return 0xFF;
}

UCHAR
WMIAPI
GetTraceEnableLevel(
    TRACEHANDLE TraceHandle
)
{
    FIXME("GetTraceEnableLevel stub()\n");
    return 0xFF;
}

ULONG
WMIAPI
UnregisterTraceGuids(
    TRACEHANDLE RegistrationHandle
)
{
    FIXME("UnregisterTraceGuids stub()\n");
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
    FIXME("RegisterTraceGuidsA stub()\n");
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
    FIXME("RegisterTraceGuidsW stub()\n");
    return ERROR_SUCCESS;
}

ULONG WINAPI StartTraceW( PTRACEHANDLE pSessionHandle, LPCWSTR SessionName, PEVENT_TRACE_PROPERTIES Properties )
{
    FIXME("(%p, %s, %p) stub\n", pSessionHandle, debugstr_w(SessionName), Properties);
    if (pSessionHandle) *pSessionHandle = 0xcafe4242;
    return ERROR_SUCCESS;
}

ULONG WINAPI StartTraceA( PTRACEHANDLE pSessionHandle, LPCSTR SessionName, PEVENT_TRACE_PROPERTIES Properties )
{
    FIXME("(%p, %s, %p) stub\n", pSessionHandle, debugstr_a(SessionName), Properties);
    if (pSessionHandle) *pSessionHandle = 0xcafe4242;
    return ERROR_SUCCESS;
}

/******************************************************************************
 * ControlTraceW [ADVAPI32.@]
 *
 * Control a givel event trace session
 *
 */
ULONG WINAPI ControlTraceW( TRACEHANDLE hSession, LPCWSTR SessionName, PEVENT_TRACE_PROPERTIES Properties, ULONG control )
{
    FIXME("(%s, %s, %p, %d) stub\n", wine_dbgstr_longlong(hSession), debugstr_w(SessionName), Properties, control);
    return ERROR_SUCCESS;
}

/******************************************************************************
 * ControlTraceA [ADVAPI32.@]
 *
 * See ControlTraceW.
 *
 */
ULONG WINAPI ControlTraceA( TRACEHANDLE hSession, LPCSTR SessionName, PEVENT_TRACE_PROPERTIES Properties, ULONG control )
{
    FIXME("(%s, %s, %p, %d) stub\n", wine_dbgstr_longlong(hSession), debugstr_a(SessionName), Properties, control);
    return ERROR_SUCCESS;
}

/******************************************************************************
 * EnableTrace [ADVAPI32.@]
 */
ULONG WINAPI EnableTrace( ULONG enable, ULONG flag, ULONG level, LPCGUID guid, TRACEHANDLE hSession )
{
    FIXME("(%d, 0x%x, %d, %s, %s): stub\n", enable, flag, level,
            debugstr_guid(guid), wine_dbgstr_longlong(hSession));

    return ERROR_SUCCESS;
}

/******************************************************************************
 * QueryAllTracesW [ADVAPI32.@]
 *
 * Query information for started event trace sessions
 *
 */
ULONG WINAPI QueryAllTracesW( PEVENT_TRACE_PROPERTIES * parray, ULONG arraycount, PULONG psessioncount )
{
    FIXME("(%p, %d, %p) stub\n", parray, arraycount, psessioncount);

    if (psessioncount) *psessioncount = 0;
    return ERROR_SUCCESS;
}

/******************************************************************************
 * QueryAllTracesA [ADVAPI32.@]
 *
 * See QueryAllTracesW.
 */
ULONG WINAPI QueryAllTracesA( PEVENT_TRACE_PROPERTIES * parray, ULONG arraycount, PULONG psessioncount )
{
    FIXME("(%p, %d, %p) stub\n", parray, arraycount, psessioncount);

    if (psessioncount) *psessioncount = 0;
    return ERROR_SUCCESS;
}

/* EOF */
