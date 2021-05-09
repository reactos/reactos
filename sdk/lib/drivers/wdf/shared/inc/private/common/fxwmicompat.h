/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxWmiCompat.hpp

Abstract:

    Undefines WMI ntos exports used by WPP and redirects them to our own
    functions so that we can build User and Kernel libraries.

Author:



Environment:

    Both kernel and user mode

Revision History:

--*/

#ifndef __FX_WMI_COMPAT_H__
#define __FX_WMI_COMPAT_H__

#include <evntrace.h>

#ifdef WPP_TRACE
#undef WPP_TRACE
#endif

#define WPP_TRACE FxWmiTraceMessage

extern "C"
_Must_inspect_result_
NTSTATUS
FxWmiTraceMessage(
    __in TRACEHANDLE  LoggerHandle,
    __in ULONG        MessageFlags,
    __in LPGUID       MessageGuid,
    __in USHORT       MessageNumber,
         ...
    );

#define WPP_IFR   FxIFR

extern "C"
_Must_inspect_result_
NTSTATUS
FxIFR(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in UCHAR        MessageLevel,
    __in ULONG        MessageFlags,
    __in LPGUID       MessageGuid,
    __in USHORT       MessageNumber,
         ...
    );

#if ((FX_CORE_MODE)==(FX_CORE_USER_MODE))
#undef WmiQueryTraceInformation
#define WmiQueryTraceInformation FxWmiQueryTraceInformation

extern "C"
_Must_inspect_result_
NTSTATUS
FxWmiQueryTraceInformation(
    __in TRACE_INFORMATION_CLASS TraceInformationClass,
    __out_bcount(TraceInformationLength) PVOID TraceInformation,
    __in ULONG TraceInformationLength,
    __out_opt PULONG RequiredLength,
    __in_opt PVOID Buffer
    );

#endif

#endif __FX_WMI_COMPAT_H__
