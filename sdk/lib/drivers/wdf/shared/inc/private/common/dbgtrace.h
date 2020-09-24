/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    DbgTrace.h

Abstract:

    This file can be used to redirect WPP traces to
    debugger.







Author:



Environment:

    Both kernel and user mode

Revision History:

--*/

#if !defined(EVENT_TRACING)







#if !defined(TRACE_LEVEL_NONE)
  #define TRACE_LEVEL_NONE        0
  #define TRACE_LEVEL_CRITICAL    1
  #define TRACE_LEVEL_FATAL       1
  #define TRACE_LEVEL_ERROR       2
  #define TRACE_LEVEL_WARNING     3
  #define TRACE_LEVEL_INFORMATION 4
  #define TRACE_LEVEL_VERBOSE     5
  #define TRACE_LEVEL_RESERVED6   6
  #define TRACE_LEVEL_RESERVED7   7
  #define TRACE_LEVEL_RESERVED8   8
  #define TRACE_LEVEL_RESERVED9   9
#endif


//
// Define Debug Flags
//
#define TRACINGDEVICE         0x00000001
#define TRACINGOBJECT         0x00000002
#define TRACINGAPIERROR       0x00000004
#define TRACINGHANDLE         0x00000008
#define TRACINGPOOL           0x00000010
#define TRACINGERROR          0x00000020
#define TRACINGUSEROBJECT     0x00000040
#define TRACINGREQUEST        0x00000080
#define TRACINGIO             0x00000100
#define TRACINGPNP            0x00000200
#define TRACINGDRIVER         0x00001000
#define TRACINGPNPPOWERSTATES 0x00002000

extern "C" {
void
__cdecl
DoTraceLevelMessage    (
    __in PVOID FxDriverGlobals,
    __in ULONG   DebugPrintLevel,
    __in ULONG   DebugPrintFlag,
    __drv_formatString(FormatMessage)
    __in PCSTR   DebugMessage,
    ...
    );
}

//
// When linking the lib with UMDF framework we don't want these macros
// to be defined since UMDF WPP tracing uses these macros



//
#ifndef UMDF
#define WPP_INIT_TRACING(DriverObject, RegistryPath)
#define WPP_CLEANUP(DriverObject)
#endif

extern ULONG DebugLevel;
extern ULONG DebugFlag;

#endif
