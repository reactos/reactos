#ifndef _WDF11_H_
#define _WDF11_H_

#include <ntddk.h>

//
// Versioning of structures for wdfdriver.h
//
typedef struct _WDF_DRIVER_CONFIG_V1_1 {
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

} WDF_DRIVER_CONFIG_V1_1, *PWDF_DRIVER_CONFIG_V1_1;

#endif //_WDF11_H_