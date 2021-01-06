/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxTelemetryCommon.hpp

Abstract:

    This is header file for telemetry methods common to all WDF components

Author:



Environment:

     Both kernel and user mode

Revision History:

Notes:

--*/

#pragma once

//
// The TraceLogging infrastructure calls EtwSetInformation API that is not available on Win7.
// Setting TLG_HAVE_EVENT_SET_INFORMATION, modifies the behavior of TraceLoggingProvider.h.
// The value 2 indicates that the trace logging infra would find "EtwSetInformation" via
// MmGetSystemRoutineAddress. This allows our code to be backwards compatible to Win7
//
#define TLG_HAVE_EVENT_SET_INFORMATION 2
// #include <traceloggingprovider.h>
// #include <telemetry\MicrosoftTelemetry.h>

// WDF01000.sys
#define KMDF_FX_TRACE_LOGGING_PROVIDER_NAME    "Microsoft.Wdf.KMDF.Fx"

// WUDFX0200.dll
#define UMDF_FX_TRACE_LOGGING_PROVIDER_NAME    "Microsoft.Wdf.UMDF.Fx"

// WudfHost.exe
#define UMDF_HOST_TRACE_LOGGING_PROVIDER_NAME  "Microsoft.Wdf.UMDF.Host"

// WdfLdr.sys
#define KMDF_LDR_TRACE_LOGGING_PROVIDER_NAME   "Microsoft.Wdf.KMDF.Ldr"

// WudfSvc.dll
#define UMDF_DM_TRACE_LOGGING_PROVIDER_NAME    "Microsoft.Wdf.UMDF.Dm"

// Common telemetry related keyword used across all telemetry events
#define WDF_TELEMETRY_EVT_KEYWORDS TraceLoggingKeyword(MICROSOFT_KEYWORD_TELEMETRY)
