#ifndef _WDF10_H_
#define _WDF10_H_

#include <ntddk.h>

//
// Since C does not have strong type checking we must invent our own
//
typedef struct _WDF_OBJECT_CONTEXT_TYPE_INFO_V1_0 {
    //
    // The size of this structure in bytes
    //
    ULONG Size;

    //
    // String representation of the context's type name, i.e. "DEVICE_CONTEXT"
    //
    PCHAR ContextName;

    //
    // The size of the context in bytes.  This will be the size of the context
    // associated with the handle unless
    // WDF_OBJECT_ATTRIBUTES::ContextSizeOverride is specified.
    //
    size_t ContextSize;

} WDF_OBJECT_CONTEXT_TYPE_INFO_V1_0, *PWDF_OBJECT_CONTEXT_TYPE_INFO_V1_0;

//
// Versioning of structures for wdfdriver.h
//
typedef struct _WDF_DRIVER_CONFIG_V1_0 {
    //
    // Size of this structure in bytes
    //
    ULONG Size;

    //
    // Event callbacks
    //
    PFN_WDF_DRIVER_DEVICE_ADD EvtDriverDeviceAdd;

    PFN_WDF_DRIVER_UNLOAD    EvtDriverUnload;

    //
    // Combination of WDF_DRIVER_INIT_FLAGS values
    //
    ULONG DriverInitFlags;

} WDF_DRIVER_CONFIG_V1_0, *PWDF_DRIVER_CONFIG_V1_0;

#endif //_WDF10_H_