/*
 * PROJECT:     VerOS NT System Libraries
 * LICENSE:     GNU Lesser General Public License, version 2.1 or later (LGPL-2.1-or-later)
 * FILE:        dll/ntdll/etw/trace.c
 * PURPOSE:     ntdll Event Tracing stubs and helpers
 * COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@verosnt.com>
 * PROGRAMMERS: Ryan Wilson
 *
 * EtwEventRegister writes a non-zero REGHANDLE because the generated
 * -stub returned ERROR_SUCCESS without filling *RegHandle. Guest serial
 * output/ntdll-apps/serial-App-20261116T054003Z.log shows
 * EtwEventSetInformation(0x0, ...) after that stub, then a user AV in
 * firefox.exe. Public docs: EventRegister out-parameter RegHandle
 * (learn.microsoft.com evntprov EventRegister). Sentinel value matches
 * EtwStartTraceW in this file (0xcafe4242).
 */

#include <ntdll.h>

#include <wmistr.h>
#include <evntrace.h>
#include <evntprov.h>

#define NDEBUG
#include <debug.h>

#define FIXME DPRINT1

/* Same sentinel EtwStartTraceW already writes for a session handle. */
#define ETW_EVENT_REG_SENTINEL ((REGHANDLE)0xcafe4242ULL)

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

ULONG
NTAPI
EtwUnregisterTraceGuids(
    TRACEHANDLE RegistrationHandle
)
{
    FIXME("EtwUnregisterTraceGuids stub()\n");
    return ERROR_SUCCESS;
}

ULONG
NTAPI
EtwRegisterTraceGuidsA(
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
    FIXME("EtwRegisterTraceGuidsA stub()\n");
    return ERROR_SUCCESS;
}

ULONG
NTAPI
EtwRegisterTraceGuidsW(
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
    FIXME("EtwRegisterTraceGuidsW stub()\n");
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

/*
 * EtwEventRegister / related provider APIs.
 * No-op registration: fill *RegHandle so callers do not keep a null handle
 * after ERROR_SUCCESS (see file header).
 */
ULONG
NTAPI
EtwEventRegister(
    LPCGUID ProviderId,
    PENABLECALLBACK EnableCallback,
    PVOID CallbackContext,
    PREGHANDLE RegHandle)
{
    UNREFERENCED_PARAMETER(EnableCallback);
    UNREFERENCED_PARAMETER(CallbackContext);

    if (!ProviderId || !RegHandle)
    {
        return ERROR_INVALID_PARAMETER;
    }

    *RegHandle = ETW_EVENT_REG_SENTINEL;
    return ERROR_SUCCESS;
}

ULONG
NTAPI
EtwEventUnregister(
    REGHANDLE RegHandle)
{
    UNREFERENCED_PARAMETER(RegHandle);
    return ERROR_SUCCESS;
}

ULONG
NTAPI
EtwEventSetInformation(
    REGHANDLE RegHandle,
    ULONG InformationClass,
    PVOID EventInformation,
    ULONG InformationLength)
{
    UNREFERENCED_PARAMETER(RegHandle);
    UNREFERENCED_PARAMETER(InformationClass);
    UNREFERENCED_PARAMETER(EventInformation);
    UNREFERENCED_PARAMETER(InformationLength);
    return ERROR_SUCCESS;
}

ULONG
NTAPI
EtwEventWrite(
    REGHANDLE RegHandle,
    PCEVENT_DESCRIPTOR EventDescriptor,
    ULONG UserDataCount,
    PEVENT_DATA_DESCRIPTOR UserData)
{
    UNREFERENCED_PARAMETER(RegHandle);
    UNREFERENCED_PARAMETER(EventDescriptor);
    UNREFERENCED_PARAMETER(UserDataCount);
    UNREFERENCED_PARAMETER(UserData);
    return ERROR_SUCCESS;
}

ULONG
NTAPI
EtwEventWriteTransfer(
    REGHANDLE RegHandle,
    PCEVENT_DESCRIPTOR EventDescriptor,
    LPCGUID ActivityId,
    LPCGUID RelatedActivityId,
    ULONG UserDataCount,
    PEVENT_DATA_DESCRIPTOR UserData)
{
    UNREFERENCED_PARAMETER(RegHandle);
    UNREFERENCED_PARAMETER(EventDescriptor);
    UNREFERENCED_PARAMETER(ActivityId);
    UNREFERENCED_PARAMETER(RelatedActivityId);
    UNREFERENCED_PARAMETER(UserDataCount);
    UNREFERENCED_PARAMETER(UserData);
    return ERROR_SUCCESS;
}

ULONG
NTAPI
EtwEventActivityIdControl(
    ULONG ControlCode,
    LPGUID ActivityId)
{
    UNREFERENCED_PARAMETER(ControlCode);
    UNREFERENCED_PARAMETER(ActivityId);
    return ERROR_SUCCESS;
}

/* EOF */
