/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    vfpriv.hpp

Abstract:

    common header file for verifier

Author:



Environment:

    user/kernel mode

Revision History:

--*/

#if FX_CORE_MODE==FX_CORE_KERNEL_MODE
#include "VfPrivKm.hpp"
#else if ((FX_CORE_MODE)==(FX_CORE_USER_MODE))
#include "VfPrivUm.hpp"
#endif

extern "C" {
#include "FxDynamics.h"
}
#include "vfeventhooks.hpp"


#define FX_ENHANCED_VERIFIER_SECTION_NAME       WDF_FX_VF_SECTION_NAME

#define GET_CONTEXT(_objectHandle, _type)  \
    (_type *)VfWdfObjectGetTypedContext(_objectHandle, WDF_GET_CONTEXT_TYPE_INFO(_type));

#define SET_HOOK_IF_CALLBACK_PRESENT(Source, Target, Name)  \
    if ((Source)-> ## Name != NULL) {      \
        (Target)-> ## Name = Vf ## Name;   \
    }

typedef struct _VF_HOOK_PROCESS_INFO {
    //
    // Return status of the DDI of called by hook routine.
    // this will be returned by stub if it does not call the DDI (since
    // hook already called.
    //
    ULONG DdiCallStatus;

    //
    // Whether kmdf lib needs to be called after hook functin returns
    //
    BOOLEAN DonotCallKmdfLib;

} VF_HOOK_PROCESS_INFO, *PVF_HOOK_PROCESS_INFO;

typedef struct _VF_COMMON_CONTEXT_HEADER {

    PWDF_DRIVER_GLOBALS DriverGlobals;

} VF_COMMON_CONTEXT_HEADER, *PVF_COMMON_CONTEXT_HEADER;

typedef struct _VF_WDFDEVICECREATE_CONTEXT {

    VF_COMMON_CONTEXT_HEADER CommonHeader;

    WDF_PNPPOWER_EVENT_CALLBACKS PnpPowerEventCallbacksOriginal;

} VF_WDFDEVICECREATE_CONTEXT, *PVF_WDFDEVICECREATE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE(VF_WDFDEVICECREATE_CONTEXT);

typedef struct _VF_WDFIOQUEUECREATE_CONTEXT {

    VF_COMMON_CONTEXT_HEADER CommonHeader;

    WDF_IO_QUEUE_CONFIG IoQueueConfigOriginal;

} VF_WDFIOQUEUECREATE_CONTEXT, *PVF_WDFIOQUEUECREATE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE(VF_WDFIOQUEUECREATE_CONTEXT);

extern "C" {

_Must_inspect_result_
PVOID
FASTCALL
VfWdfObjectGetTypedContext(
    __in
    WDFOBJECT Handle,
    __in
    PCWDF_OBJECT_CONTEXT_TYPE_INFO TypeInfo
    );

_Must_inspect_result_
NTSTATUS
VfAllocateContext(
    __in PWDF_DRIVER_GLOBALS DriverGlobals,
    __in PWDF_OBJECT_ATTRIBUTES Attributes,
    __out PVOID* ContextHeader
    );

_Must_inspect_result_
NTSTATUS
VfAddContextToHandle(
    __in PVOID ContextHeader,
    __in PWDF_OBJECT_ATTRIBUTES Attributes,
    __in WDFOBJECT Handle,
    __out_opt PVOID* Context
    );

_Must_inspect_result_
NTSTATUS
AddEventHooksWdfDeviceCreate(
    __inout PVF_HOOK_PROCESS_INFO HookProcessInfo,
    __in PWDF_DRIVER_GLOBALS DriverGlobals,
    __in PWDFDEVICE_INIT* DeviceInit,
    __in PWDF_OBJECT_ATTRIBUTES DeviceAttributes,
    __out WDFDEVICE* Device
    );

_Must_inspect_result_
NTSTATUS
AddEventHooksWdfIoQueueCreate(
    __inout PVF_HOOK_PROCESS_INFO HookProcessInfo,
    __in PWDF_DRIVER_GLOBALS DriverGlobals,
    __in WDFDEVICE Device,
    __in PWDF_IO_QUEUE_CONFIG Config,
    __in PWDF_OBJECT_ATTRIBUTES QueueAttributes,
    __out WDFQUEUE* Queue
    );

} // extern "C"
