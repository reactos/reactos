#ifndef _WDF17_H_
#define _WDF17_H_

#include <ntddk.h>

//
// Versioning of structures for wdfio.h
//
// 
// This is the structure used to configure an IoQueue and
// register callback events to it.
// 
//
typedef struct _WDF_IO_QUEUE_CONFIG_V1_7 {
    ULONG                                       Size;

    WDF_IO_QUEUE_DISPATCH_TYPE                  DispatchType;

    WDF_TRI_STATE                               PowerManaged;

    BOOLEAN                                     AllowZeroLengthRequests;

    BOOLEAN                                     DefaultQueue;

    PFN_WDF_IO_QUEUE_IO_DEFAULT                 EvtIoDefault;

    PFN_WDF_IO_QUEUE_IO_READ                    EvtIoRead;

    PFN_WDF_IO_QUEUE_IO_WRITE                   EvtIoWrite;

    PFN_WDF_IO_QUEUE_IO_DEVICE_CONTROL          EvtIoDeviceControl;

    PFN_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL EvtIoInternalDeviceControl;

    PFN_WDF_IO_QUEUE_IO_STOP                    EvtIoStop;

    PFN_WDF_IO_QUEUE_IO_RESUME                  EvtIoResume;

    PFN_WDF_IO_QUEUE_IO_CANCELED_ON_QUEUE       EvtIoCanceledOnQueue;

} WDF_IO_QUEUE_CONFIG_V1_7, *PWDF_IO_QUEUE_CONFIG_V1_7;

typedef struct _WDF_TIMER_CONFIG_V1_7 {
    ULONG         Size;

    PFN_WDF_TIMER EvtTimerFunc;

    LONG          Period;

    // 
    // If this is TRUE, the Timer will automatically serialize
    // with the event callback handlers of its Parent Object.
    // 
    // Parent Object's callback constraints should be compatible
    // with the Timer DPC (DISPATCH_LEVEL), or the request will fail.
    // 
    BOOLEAN       AutomaticSerialization;

} WDF_TIMER_CONFIG_V1_7, *PWDF_TIMER_CONFIG_V1_7;

#endif //_WDF17_H_