/*
 * ntdll.dll Event Tracing Functions
 */

#include <ntdll.h>

#include <wmistr.h>
#include <evntrace.h>

#define NDEBUG
#include <debug.h>

#define FIXME DPRINT1

ULONG
NTAPI
EtwRegisterTraceGuidsW(
    _In_ WMIDPREQUEST RequestAddress,
    _In_opt_ PVOID RequestContext,
    _In_ LPCGUID ControlGuid,
    _In_ ULONG GuidCount,
    _In_reads_opt_(GuidCount) PTRACE_GUID_REGISTRATION TraceGuidReg,
    _In_opt_ LPCWSTR MofImagePath,
    _In_opt_ LPCWSTR MofResourceName,
    _Out_ PTRACEHANDLE RegistrationHandle
);

/*
 * @unimplemented
 */
ULONG CDECL
EtwTraceMessage(
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
NTAPI
EtwGetTraceLoggerHandle(
    PVOID Buffer
)
{
    FIXME("EtwGetTraceLoggerHandle stub()\n");
    return (TRACEHANDLE)-1;
}


ULONG
NTAPI
EtwTraceEvent(
    TRACEHANDLE SessionHandle,
    PEVENT_TRACE_HEADER EventTrace
)
{
    FIXME("EtwTraceEvent stub()\n");

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
NTAPI
EtwGetTraceEnableFlags(
    TRACEHANDLE TraceHandle
)
{
    FIXME("EtwGetTraceEnableFlags stub()\n");
    return 0xFF;
}

UCHAR
NTAPI
EtwGetTraceEnableLevel(
    TRACEHANDLE TraceHandle
)
{
    FIXME("EtwGetTraceEnableLevel stub()\n");
    return 0xFF;
}

/******************************************************************************
 * EtwUnregisterTraceGuids (NTDLL.@)
 */
ULONG
NTAPI
EtwUnregisterTraceGuids(
    _In_ TRACEHANDLE RegistrationHandle
)
{
    if (!RegistrationHandle)
        return ERROR_INVALID_PARAMETER;

    FIXME("EtwUnregisterTraceGuids(%I64x) stub\n", RegistrationHandle);

    return ERROR_SUCCESS;
}

/******************************************************************************
 * EtwRegisterTraceGuidsA (NTDLL.@)
 */
ULONG
NTAPI
EtwRegisterTraceGuidsA(
    _In_ WMIDPREQUEST RequestAddress,
    _In_opt_ PVOID RequestContext,
    _In_ LPCGUID ControlGuid,
    _In_ ULONG GuidCount,
    _In_reads_opt_(GuidCount) PTRACE_GUID_REGISTRATION TraceGuidReg,
    _In_opt_ LPCSTR MofImagePath,
    _In_opt_ LPCSTR MofResourceName,
    _Out_ PTRACEHANDLE RegistrationHandle
)
{
    if (!RequestContext || !ControlGuid || !RegistrationHandle)
        return ERROR_INVALID_PARAMETER;

#if 0
    // TODO: Should be NULL on XPsp2+... Test if error or simply ignored.
    if (MofImagePath || MofResourceName)
        return ERROR_INVALID_PARAMETER;
#endif

    FIXME("EtwRegisterTraceGuidsA(%p, %p, %p, %lu, %p, %p, %p, %p) stub\n",
          RequestAddress, RequestContext, ControlGuid, GuidCount, TraceGuidReg,
          MofImagePath, MofResourceName, RegistrationHandle);

    return EtwRegisterTraceGuidsW(RequestAddress,
                                  RequestContext,
                                  ControlGuid,
                                  GuidCount,
                                  TraceGuidReg,
                                  NULL,
                                  NULL,
                                  RegistrationHandle);
}

/******************************************************************************
 * EtwRegisterTraceGuidsW (NTDLL.@)
 *
 * Register an event trace provider and the event trace classes that it uses
 * to generate events.
 *
 * PARAMS
 *  RequestAddress     [I]   ControlCallback function
 *  RequestContext     [I]   Optional provider-defined context
 *  ControlGuid        [I]   GUID of the registering provider
 *  GuidCount          [I]   Number of elements in the TraceGuidReg array
 *  TraceGuidReg       [I/O] Array of TRACE_GUID_REGISTRATION structures
 *  MofImagePath       [I]   not supported, set to NULL
 *  MofResourceName    [I]   not supported, set to NULL
 *  RegistrationHandle [O]   Provider's registration handle
 *
 * RETURNS
 *  Success: ERROR_SUCCESS
 *  Failure: System error code
 */
ULONG
NTAPI
EtwRegisterTraceGuidsW(
    _In_ WMIDPREQUEST RequestAddress,
    _In_opt_ PVOID RequestContext,
    _In_ LPCGUID ControlGuid,
    _In_ ULONG GuidCount,
    _In_reads_opt_(GuidCount) PTRACE_GUID_REGISTRATION TraceGuidReg,
    _In_opt_ LPCWSTR MofImagePath,
    _In_opt_ LPCWSTR MofResourceName,
    _Out_ PTRACEHANDLE RegistrationHandle
)
{
    if (!RequestContext || !ControlGuid || !RegistrationHandle)
        return ERROR_INVALID_PARAMETER;

#if 0
    // TODO: Should be NULL on XPsp2+... Test if error or simply ignored.
    if (MofImagePath || MofResourceName)
        return ERROR_INVALID_PARAMETER;
#endif

    FIXME("EtwRegisterTraceGuidsW(%p, %p, %p, %lu, %p, %p, %p, %p) stub\n",
          RequestAddress, RequestContext, ControlGuid, GuidCount, TraceGuidReg,
          MofImagePath, MofResourceName, RegistrationHandle);

    if (TraceGuidReg)
    {
        ULONG i;
        for (i = 0; i < GuidCount; i++)
        {
            FIXME("  register trace class %p\n", TraceGuidReg[i].Guid);
            TraceGuidReg[i].RegHandle = UlongToPtr(0xdeadbeef);
        }
    }

    *RegistrationHandle = (TRACEHANDLE)0xdeadbeef;

    return ERROR_SUCCESS;
}

ULONG WINAPI EtwStartTraceW( PTRACEHANDLE pSessionHandle, LPCWSTR SessionName, PEVENT_TRACE_PROPERTIES Properties )
{
    FIXME("(%p, %s, %p) stub\n", pSessionHandle, SessionName, Properties);
    if (pSessionHandle) *pSessionHandle = 0xcafe4242;
    return ERROR_SUCCESS;
}

ULONG WINAPI EtwStartTraceA( PTRACEHANDLE pSessionHandle, LPCSTR SessionName, PEVENT_TRACE_PROPERTIES Properties )
{
    FIXME("(%p, %s, %p) stub\n", pSessionHandle, SessionName, Properties);
    if (pSessionHandle) *pSessionHandle = 0xcafe4242;
    return ERROR_SUCCESS;
}

/******************************************************************************
 * EtwControlTraceW [NTDLL.@]
 *
 * Control a givel event trace session
 *
 */
ULONG WINAPI EtwControlTraceW( TRACEHANDLE hSession, LPCWSTR SessionName, PEVENT_TRACE_PROPERTIES Properties, ULONG control )
{
    FIXME("(%I64x, %s, %p, %d) stub\n", hSession, SessionName, Properties, control);
    return ERROR_SUCCESS;
}

/******************************************************************************
 * EtwControlTraceA [NTDLL.@]
 *
 * See ControlTraceW.
 *
 */
ULONG WINAPI EtwControlTraceA( TRACEHANDLE hSession, LPCSTR SessionName, PEVENT_TRACE_PROPERTIES Properties, ULONG control )
{
    FIXME("(%I64x, %s, %p, %d) stub\n", hSession, SessionName, Properties, control);
    return ERROR_SUCCESS;
}

/******************************************************************************
 * EtwEnableTrace [NTDLL.@]
 */
ULONG WINAPI EtwEnableTrace( ULONG enable, ULONG flag, ULONG level, LPCGUID guid, TRACEHANDLE hSession )
{
    FIXME("(%d, 0x%x, %d, %p, %I64x): stub\n", enable, flag, level,
            guid, hSession);

    return ERROR_SUCCESS;
}

/******************************************************************************
 * EtwQueryAllTracesW [NTDLL.@]
 *
 * Query information for started event trace sessions
 *
 */
ULONG WINAPI EtwQueryAllTracesW( PEVENT_TRACE_PROPERTIES * parray, ULONG arraycount, PULONG psessioncount )
{
    FIXME("(%p, %d, %p) stub\n", parray, arraycount, psessioncount);

    if (psessioncount) *psessioncount = 0;
    return ERROR_SUCCESS;
}

/******************************************************************************
 * QueryAllTracesA [NTDLL.@]
 *
 * See EtwQueryAllTracesA.
 */
ULONG WINAPI EtwQueryAllTracesA( PEVENT_TRACE_PROPERTIES * parray, ULONG arraycount, PULONG psessioncount )
{
    FIXME("(%p, %d, %p) stub\n", parray, arraycount, psessioncount);

    if (psessioncount) *psessioncount = 0;
    return ERROR_SUCCESS;
}

/******************************************************************************
 * EtwFlushTraceA [NTDLL.@]
 *
 */
ULONG WINAPI EtwFlushTraceA( TRACEHANDLE hSession, LPCSTR SessionName, PEVENT_TRACE_PROPERTIES Properties )
{
    return EtwControlTraceA( hSession, SessionName, Properties, EVENT_TRACE_CONTROL_FLUSH );
}

/******************************************************************************
 * EtwFlushTraceW [NTDLL.@]
 *
 */
ULONG WINAPI EtwFlushTraceW( TRACEHANDLE hSession, LPCWSTR SessionName, PEVENT_TRACE_PROPERTIES Properties )
{
    return EtwControlTraceW( hSession, SessionName, Properties, EVENT_TRACE_CONTROL_FLUSH );
}

/******************************************************************************
 * EtwQueryTraceA [NTDLL.@]
 *
 */
ULONG WINAPI EtwQueryTraceA( TRACEHANDLE hSession, LPCSTR SessionName, PEVENT_TRACE_PROPERTIES Properties )
{
    return EtwControlTraceA( hSession, SessionName, Properties, EVENT_TRACE_CONTROL_QUERY );
}

/******************************************************************************
 * EtwQueryTraceW [NTDLL.@]
 *
 */
ULONG WINAPI EtwQueryTraceW( TRACEHANDLE hSession, LPCWSTR SessionName, PEVENT_TRACE_PROPERTIES Properties )
{
    return EtwControlTraceW( hSession, SessionName, Properties, EVENT_TRACE_CONTROL_QUERY );
}

/******************************************************************************
 * EtwStopTraceA [NTDLL.@]
 *
 */
ULONG WINAPI EtwStopTraceA( TRACEHANDLE hSession, LPCSTR SessionName, PEVENT_TRACE_PROPERTIES Properties )
{
    return EtwControlTraceA( hSession, SessionName, Properties, EVENT_TRACE_CONTROL_STOP );
}

/******************************************************************************
 * EtwStopTraceW [NTDLL.@]
 *
 */
ULONG WINAPI EtwStopTraceW( TRACEHANDLE hSession, LPCWSTR SessionName, PEVENT_TRACE_PROPERTIES Properties )
{
    return EtwControlTraceW( hSession, SessionName, Properties, EVENT_TRACE_CONTROL_STOP );
}

/******************************************************************************
 * EtwUpdateTraceA [NTDLL.@]
 *
 */
ULONG WINAPI EtwUpdateTraceA( TRACEHANDLE hSession, LPCSTR SessionName, PEVENT_TRACE_PROPERTIES Properties )
{
    return EtwControlTraceA( hSession, SessionName, Properties, EVENT_TRACE_CONTROL_UPDATE );
}

/******************************************************************************
 * EtwUpdateTraceW [NTDLL.@]
 *
 */
ULONG WINAPI EtwUpdateTraceW( TRACEHANDLE hSession, LPCWSTR SessionName, PEVENT_TRACE_PROPERTIES Properties )
{
    return EtwControlTraceW( hSession, SessionName, Properties, EVENT_TRACE_CONTROL_UPDATE );
}

/* EOF */
