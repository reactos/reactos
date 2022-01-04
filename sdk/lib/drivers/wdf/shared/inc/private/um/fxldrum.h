/*++

Copyright (c) Microsoft Corporation

Module Name:

    fxldrum.h

Abstract:

    This is the UMDF version of wdfldr.h


--*/
#ifndef __FXLDRUM_H__
#define __FXLDRUM_H__

#define WDF_COMPONENT_NAME(a) L#a

typedef
VOID
(*WDFFUNC)(
    VOID
    );

typedef ULONG WDF_MAJOR_VERSION;
typedef ULONG WDF_MINOR_VERSION;
typedef ULONG WDF_BUILD_NUMBER;

//
// Version container
//
typedef struct _WDF_VERSION {
    WDF_MAJOR_VERSION  Major;
    WDF_MINOR_VERSION  Minor;
    WDF_BUILD_NUMBER   Build;
} WDF_VERSION;

//
// WDF bind information structure.
//
typedef struct _WDF_BIND_INFO {
    ULONG              Size;
    PWCHAR             Component;
    WDF_VERSION        Version;
    ULONG              FuncCount;
    __field_bcount(FuncCount*sizeof(WDFFUNC)) WDFFUNC* FuncTable;





    //
    // This field is not used in UMDF
    //
    PVOID              Module;



} WDF_BIND_INFO, * PWDF_BIND_INFO;

typedef PVOID WDF_COMPONENT_GLOBALS, *PWDF_COMPONENT_GLOBALS;

typedef
NTSTATUS
(*PFNLIBRARYCOMMISSION)(
    VOID
    );

typedef
NTSTATUS
(*PFNLIBRARYDECOMMISSION)(
    VOID
    );

typedef
NTSTATUS
(*PFNLIBRARYREGISTERCLIENT)(
    PWDF_BIND_INFO             Info,
    PWDF_COMPONENT_GLOBALS   * ComponentGlobals,
    PVOID                    * Context
    );

typedef
NTSTATUS
(*PFNLIBRARYUNREGISTERCLIENT)(
    PWDF_BIND_INFO             Info,
    PWDF_COMPONENT_GLOBALS     DriverGlobals
    );


typedef struct _WDF_LIBRARY_INFO {
    ULONG                             Size;
    PFNLIBRARYCOMMISSION              LibraryCommission;
    PFNLIBRARYDECOMMISSION            LibraryDecommission;
    PFNLIBRARYREGISTERCLIENT          LibraryRegisterClient;
    PFNLIBRARYUNREGISTERCLIENT        LibraryUnregisterClient;
    WDF_VERSION                       Version;
} WDF_LIBRARY_INFO, *PWDF_LIBRARY_INFO;

typedef
PWDF_LIBRARY_INFO
(*PFX_GET_LIBRARY_INFO_UM)(
    VOID
    );

//
// Framework 2.x's driver object.
//
typedef struct _DRIVER_OBJECT_UM *PDRIVER_OBJECT_UM;
struct IWudfDeviceStack;
struct IWudfDeviceStack2;
struct IWudfDevice;
struct IWudfIrp;
struct IUnknown;
typedef enum _WDF_DEVICE_IO_BUFFER_RETRIEVAL *PWDF_DEVICE_IO_BUFFER_RETRIEVAL;
typedef enum RdWmiPowerAction;
typedef const GUID *LPCGUID;
typedef UINT64 WUDF_INTERFACE_CONTEXT;
class FxDriver;

//
// Valid flags for use in the DRIVER_OBJECT_UM::Flags field.
//
enum FxDriverObjectUmFlags : USHORT {
    DriverObjectUmFlagsLoggingEnabled = 0x1
};

//
// Driver object's basic interface.
//
typedef
NTSTATUS
DRIVER_ADD_DEVICE_UM (
    _In_  PDRIVER_OBJECT_UM         DriverObject,
    _In_  PVOID                     Context,
    _In_  IWudfDeviceStack *        DevStack,
    _In_  LPCWSTR                   KernelDeviceName,
    _In_opt_ HKEY                   hPdoKey,
    _In_  LPCWSTR                   pwszServiceName,
    _In_  LPCWSTR                   pwszDevInstanceID,
    _In_  ULONG                     ulDriverID
    );

typedef DRIVER_ADD_DEVICE_UM *PFN_DRIVER_ADD_DEVICE_UM;

typedef
VOID
DRIVER_DISPATCH_UM (
    _In_ IWudfDevice *             DeviceObject,
    _In_ IWudfIrp *                 Irp,
    _In_opt_ IUnknown *             Context
    );

typedef DRIVER_DISPATCH_UM *PFN_DRIVER_DISPATCH_UM;

typedef
VOID
DRIVER_UNLOAD_UM (
    _In_ PDRIVER_OBJECT_UM          DriverObject
    );

typedef DRIVER_UNLOAD_UM *PFN_DRIVER_UNLOAD_UM;

typedef struct _DRIVER_OBJECT_UM {
    ULONG Size;

    //
    // The following links all of the devices created by a single driver
    // together on a list, and the Flags word provides an extensible flag
    // location for driver objects.
    //
    ULONG Flags;

    //
    // The driver name field is used by the error log thread
    // determine the name of the driver that an I/O request is/was bound.
    //
    UNICODE_STRING DriverName;

    //
    // Store FxDriver
    //
    FxDriver* FxDriver;

    //
    // Host device stack. This field is only valid while initializing
    // the driver, such as during DriverEntry, and is NULL at all other times.
    //
    IWudfDeviceStack2* WudfDevStack;

    //
    // Callback environment for driver workitems.
    //
    TP_CALLBACK_ENVIRON ThreadPoolEnv;

    //
    // Group that tracks thread pool callbacks.
    //
    PTP_CLEANUP_GROUP ThreadPoolGroup;

    //
    // A global number that driver IFR and framework IFR can opt into
    // using as their record ordering number. This allows the framework
    // and driver IFR record lists to be merged and sorted before being
    // displayed.
    //
    LONG IfrSequenceNumber;

    //
    // The following section describes the entry points to this particular
    // driver.  Note that the major function dispatch table must be the last
    // field in the object so that it remains extensible.
    //
    PFN_DRIVER_ADD_DEVICE_UM AddDevice;
    PFN_DRIVER_UNLOAD_UM DriverUnload;
    PFN_DRIVER_DISPATCH_UM MajorFunction[IRP_MJ_MAXIMUM_FUNCTION + 1];

} DRIVER_OBJECT_UM;

typedef struct _CLIENT_INFO {
    //
    // Size of this structure
    //
    ULONG              Size;

    //
    // registry service path of client driver
    //
    PUNICODE_STRING    RegistryPath;

} CLIENT_INFO, *PCLIENT_INFO;

//
// Event name:  WdfCensusEvtLinkClientToCx
//
// Source:      WudfHost (UM loader)
//
// Description: Written when a client is binding to a class extension.
//              WdfVersionBindClass which is called from the client's stub,
//              will load/reference the Cx and add it to the fx library's
//              list of clients. The client driver's class extension list is
//              also updated at that time, which is when this event is written.
//
// Frequency:   Everytime a client driver binds to a class extension.
//
//
#define WDF_CENSUS_EVT_WRITE_LINK_CLIENT_TO_CX(TraceHandle, CxImageName, ClientImageName)        \
            TraceLoggingWrite(TraceHandle,                                     \
                "WdfCensusEvtLinkClientToCx",                                  \
                WDF_TELEMETRY_EVT_KEYWORDS,                                    \
                TraceLoggingWideString(CxImageName,       "CxImageName"),      \
                TraceLoggingWideString(ClientImageName,   "ClientImageName"  ) \
                );

#endif // __FXLDRUM_H__
