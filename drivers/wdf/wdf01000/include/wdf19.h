#ifndef _WDF19_H_
#define _WDF19_H_

#include <ntddk.h>

typedef struct _WDF_PNPPOWER_EVENT_CALLBACKS_V1_9 {
    // 
    // Size of this structure in bytes
    // 
    ULONG Size;

    PFN_WDF_DEVICE_D0_ENTRY                 EvtDeviceD0Entry;

    PFN_WDF_DEVICE_D0_ENTRY_POST_INTERRUPTS_ENABLED EvtDeviceD0EntryPostInterruptsEnabled;

    PFN_WDF_DEVICE_D0_EXIT                  EvtDeviceD0Exit;

    PFN_WDF_DEVICE_D0_EXIT_PRE_INTERRUPTS_DISABLED EvtDeviceD0ExitPreInterruptsDisabled;

    PFN_WDF_DEVICE_PREPARE_HARDWARE         EvtDevicePrepareHardware;

    PFN_WDF_DEVICE_RELEASE_HARDWARE         EvtDeviceReleaseHardware;

    PFN_WDF_DEVICE_SELF_MANAGED_IO_CLEANUP  EvtDeviceSelfManagedIoCleanup;

    PFN_WDF_DEVICE_SELF_MANAGED_IO_FLUSH    EvtDeviceSelfManagedIoFlush;

    PFN_WDF_DEVICE_SELF_MANAGED_IO_INIT     EvtDeviceSelfManagedIoInit;

    PFN_WDF_DEVICE_SELF_MANAGED_IO_SUSPEND  EvtDeviceSelfManagedIoSuspend;

    PFN_WDF_DEVICE_SELF_MANAGED_IO_RESTART  EvtDeviceSelfManagedIoRestart;

    PFN_WDF_DEVICE_SURPRISE_REMOVAL         EvtDeviceSurpriseRemoval;

    PFN_WDF_DEVICE_QUERY_REMOVE             EvtDeviceQueryRemove;

    PFN_WDF_DEVICE_QUERY_STOP               EvtDeviceQueryStop;

    PFN_WDF_DEVICE_USAGE_NOTIFICATION       EvtDeviceUsageNotification;

    PFN_WDF_DEVICE_RELATIONS_QUERY          EvtDeviceRelationsQuery;

} WDF_PNPPOWER_EVENT_CALLBACKS_V1_9, *PWDF_PNPPOWER_EVENT_CALLBACKS_V1_9;

#endif //_WDF19_H_
