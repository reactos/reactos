/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxTrace.h

Abstract:

    This module contains the private tracing definitions

Author:




Environment:

    Both kernel and user mode

Revision History:


--*/

#ifndef _FXTRACE_H_
#define _FXTRACE_H_

typedef struct _WDF_DRIVER_CONFIG *PWDF_DRIVER_CONFIG;

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
        )

#define WPP_CONTROL_GUIDS \
    WDF_FRAMEWORKS_TRACE_FLAGS









//#define WPP_DEBUG(args) DbgPrint args , DbgPrint("\n")

#define WPP_GLOBALS_LEVEL_FLAGS_LOGGER(globals,lvl,flags) WPP_LEVEL_LOGGER(flags)
#define WPP_GLOBALS_LEVEL_FLAGS_ENABLED(globals,lvl,flags) (WPP_LEVEL_ENABLED(flags) && WPP_CONTROL(WPP_BIT_ ## flags).Level >= lvl)

#define IFR_GLOBALS_LEVEL_FLAGS_FILTER(globals,lvl,flags) (lvl < TRACE_LEVEL_VERBOSE || globals->FxVerboseOn)

//
// These are pure enums (one and only one value)
//
// begin_wpp config
// CUSTOM_TYPE(WdfDmaEnablerCallback, ItemListLong(FxEvtDmaEnablerInvalid,FxEvtDmaEnablerFill,FxEvtDmaEnablerFlush,FxEvtDmaEnablerEnable,FxEvtDmaEnablerDisable,FxEvtDmaEnablerSelfManagedIoStart,FxEvtDmaEnablerSelfManagedIoStop),"s");
// CUSTOM_TYPE(IRPMJ, ItemListByte(IRP_MJ_CREATE,IRP_MJ_CREATE_NAMED_PIPE,IRP_MJ_CLOSE,IRP_MJ_READ,IRP_MJ_WRITE,IRP_MJ_QUERY_INFORMATION,IRP_MJ_SET_INFORMATION,IRP_MJ_QUERY_EA,IRP_MJ_SET_EA,IRP_MJ_FLUSH_BUFFERS,IRP_MJ_QUERY_VOLUME_INFORMATION,IRP_MJ_SET_VOLUME_INFORMATION,IRP_MJ_DIRECTORY_CONTROL,IRP_MJ_FILE_SYSTEM_CONTROL,IRP_MJ_DEVICE_CONTROL,IRP_MJ_INTERNAL_DEVICE_CONTROL,IRP_MJ_SHUTDOWN,IRP_MJ_LOCK_CONTROL,IRP_MJ_CLEANUP,IRP_MJ_CREATE_MAILSLOT,IRP_MJ_QUERY_SECURITY,IRP_MJ_SET_SECURITY,IRP_MJ_POWER,IRP_MJ_SYSTEM_CONTROL,IRP_MJ_DEVICE_CHANGE,IRP_MJ_QUERY_QUOTA,IRP_MJ_SET_QUOTA,IRP_MJ_PNP));
// end_wpp


_Must_inspect_result_
NTSTATUS
FxTraceInitialize(
    VOID
    );

VOID
TraceUninitialize(
    VOID
    );

VOID
FxIFRStart(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in PCUNICODE_STRING RegistryPath,
    __in MdDriverObject DriverObject
    );

VOID
FxIFRStop(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    );

#endif // _FXTRACE_H
