#ifndef _DBGTRACE_H_
#define _DBGTRACE_H_

#include "common/fxglobals.h"


/**
 * Tracing Definitions:
 */

#if FX_CORE_MODE == FX_CORE_KERNEL_MODE
#define WDF_FX_TRACE_WPPGUID        (544d4c9d,942c,46d5,bf50,df5cd9524a50)
#elif FX_CORE_MODE == FX_CORE_USER_MODE
#define WDF_FX_TRACE_WPPGUID        (485e7de9,0a80,11d8,ad15,505054503030)
#endif

#define WDF_FRAMEWORKS_TRACE_FLAGS                  \
    WPP_DEFINE_WDF_CONTROL_GUID(                    \
        KmdfTraceGuid,                              \
        WDF_FX_TRACE_WPPGUID,                       \
        WPP_DEFINE_BIT(TRACINGFULL)                 \
        WPP_DEFINE_BIT(TRACINGERROR)                \
        WPP_DEFINE_BIT(TRACINGDBGPRINT)             \
        WPP_DEFINE_BIT(TRACINGFRAMEWORKS)           \
        WPP_DEFINE_BIT(TRACINGAPI)                  \
        WPP_DEFINE_BIT(TRACINGAPIERROR)             \
        WPP_DEFINE_BIT(TRACINGRESOURCES)            \
        WPP_DEFINE_BIT(TRACINGLOCKING)              \
        WPP_DEFINE_BIT(TRACINGCONTEXT)              \
        WPP_DEFINE_BIT(TRACINGPOOL)                 \
        WPP_DEFINE_BIT(TRACINGHANDLE)               \
        WPP_DEFINE_BIT(TRACINGPNP)                  \
        WPP_DEFINE_BIT(TRACINGIO)                   \
        WPP_DEFINE_BIT(TRACINGIOTARGET)             \
        WPP_DEFINE_BIT(TRACINGDMA)                  \
        WPP_DEFINE_BIT(TRACINGREQUEST)              \
        WPP_DEFINE_BIT(TRACINGDRIVER)               \
        WPP_DEFINE_BIT(TRACINGDEVICE)               \
        WPP_DEFINE_BIT(TRACINGUSEROBJECT)           \
        WPP_DEFINE_BIT(TRACINGOBJECT)               \
        WPP_DEFINE_BIT(TRACINGPNPPOWERSTATES)       \
        WPP_DEFINE_BIT(TRACINGIFRCAPTURE)           \
        )

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

//Is correct ?
#define TRACINGIOTARGET       0x00004000

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

extern "C" {
extern ULONG DebugLevel;
extern ULONG DebugFlag;
}

#endif


#endif //_DBGTRACE_H_