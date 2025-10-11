/*
 * PROJECT:     Kernel Mode Device Framework
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Missing headers (wdfcx.h)
 * COPYRIGHT:   2020 Victor Perevertkin (victor.perevertkin@reactos.org)
 */

#ifndef _WDFCX_H_
#define _WDFCX_H_

#include "kmdf/inc/private/wdfldr.h"

typedef BOOLEAN
(NTAPI *PFN_WDFCX_DEVICE_FILE_CREATE) (
    _In_ WDFDEVICE Device,
    _In_ WDFREQUEST Request,
    _In_opt_ WDFFILEOBJECT FileObject);

// _WDFCX_FILEOBJECT_CONFIG_V1_17

typedef struct _WDFCX_FILEOBJECT_CONFIG {
    //
    // Size of this structure in bytes
    //
    ULONG Size;

    //
    // Event callback for create requests
    //
    PFN_WDFCX_DEVICE_FILE_CREATE  EvtCxDeviceFileCreate;

    //
    // Event callback for close requests
    //
    PFN_WDF_FILE_CLOSE   EvtFileClose;

    //
    // Event callback for cleanup requests
    //
    PFN_WDF_FILE_CLEANUP EvtFileCleanup;

    //
    // If WdfTrue, create/cleanup/close file object related requests will be
    // sent down the stack.
    //
    // If WdfFalse, create/cleanup/close will be completed at this location in
    // the device stack.
    //
    // If WdfDefault, behavior depends on device type
    // FDO, PDO, Control:  use the WdfFalse behavior
    // Filter:  use the WdfTrue behavior
    //
    WDF_TRI_STATE AutoForwardCleanupClose;

    //
    // Specify whether framework should create WDFFILEOBJECT and also
    // whether it can FsContexts fields in the WDM fileobject to store
    // WDFFILEOBJECT so that it can avoid table look up and improve perf.
    //
    WDF_FILEOBJECT_CLASS FileObjectClass;

} WDFCX_FILEOBJECT_CONFIG, *PWDFCX_FILEOBJECT_CONFIG;

typedef NTSTATUS
(NTAPI *PFN_WDFCXDEVICE_WDM_IRP_PREPROCESS)(
    _In_ WDFDEVICE Device,
    _Inout_ PIRP Irp,
    _In_ PVOID DispatchContext);

typedef PWDFCXDEVICE_INIT
(NTAPI *PFN_WDFCXDEVICEINITALLOCATE)(
    _In_ PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_ PWDFDEVICE_INIT DeviceInit);

typedef NTSTATUS
(NTAPI *PFN_WDFCXDEVICEINITASSIGNWDMIRPPREPROCESSCALLBACK) (
    _In_ PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_ PWDFCXDEVICE_INIT CxDeviceInit,
    _In_ PFN_WDFCXDEVICE_WDM_IRP_PREPROCESS EvtCxDeviceWdmIrpPreprocess,
    _In_ UCHAR MajorFunction,
    _When_(NumMinorFunctions > 0, _In_reads_bytes_(NumMinorFunctions))
    _When_(NumMinorFunctions == 0, _In_opt_)
    PUCHAR MinorFunctions,
    _In_ ULONG NumMinorFunctions);

typedef VOID
(NTAPI *PFN_WDFCXDEVICEINITSETIOINCALLERCONTEXTCALLBACK)(
    _In_ PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_ PWDFCXDEVICE_INIT CxDeviceInit,
    _In_ PFN_WDF_IO_IN_CALLER_CONTEXT EvtIoInCallerContext);

typedef VOID
(NTAPI *PFN_WDFCXDEVICEINITSETREQUESTATTRIBUTES)(
    _In_ PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_ PWDFCXDEVICE_INIT CxDeviceInit,
    _In_ PWDF_OBJECT_ATTRIBUTES RequestAttributes);

typedef VOID
(NTAPI *PFN_WDFCXDEVICEINITSETFILEOBJECTCONFIG)(
    _In_ PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_ PWDFCXDEVICE_INIT CxDeviceInit,
    _In_ PWDFCX_FILEOBJECT_CONFIG CxFileObjectConfig,
    _In_opt_ PWDF_OBJECT_ATTRIBUTES FileObjectAttributes);

typedef VOID
(NTAPI *PFN_WDFCXVERIFIERKEBUGCHECK)(
    _In_ PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_opt_ WDFOBJECT Object,
    _In_ ULONG BugCheckCode,
    _In_ ULONG_PTR BugCheckParameter1,
    _In_ ULONG_PTR BugCheckParameter2,
    _In_ ULONG_PTR BugCheckParameter3,
    _In_ ULONG_PTR BugCheckParameter4);

typedef WDFIOTARGET
(NTAPI *PFN_WDFDEVICEGETSELFIOTARGET)(
    _In_ PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_ WDFDEVICE Device);

typedef VOID
(NTAPI *PFN_WDFDEVICEINITALLOWSELFIOTARGET)(
    _In_ PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_ PWDFDEVICE_INIT DeviceInit);

typedef NTSTATUS
(NTAPI *PFN_WDFIOTARGETSELFASSIGNDEFAULTIOQUEUE)(
    _In_ PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_ WDFIOTARGET IoTarget,
    _In_ WDFQUEUE Queue);

/* Class Extension support */
typedef NTSTATUS (NTAPI *PFN_WDF_CLASS_EXTENSIONIN_BIND)(
    _In_ PWDF_CLASS_BIND_INFO ClassBindInfo,
    _In_ PWDF_COMPONENT_GLOBALS ComponentGlobals);

typedef VOID (NTAPI *PFN_WDF_CLASS_EXTENSIONIN_UNBIND)(
    _In_ PWDF_CLASS_BIND_INFO ClassBindInfo,
    _In_ PWDF_COMPONENT_GLOBALS ComponentGlobals);

typedef PVOID (NTAPI *PFN_WDF_CLASS_EXPORT)(VOID);

typedef NTSTATUS (NTAPI *PFN_WDF_CLASS_LIBRARY_INITIALIZE)(VOID);

typedef VOID (NTAPI *PFN_WDF_CLASS_LIBRARY_DEINITIALIZE)(VOID);

typedef NTSTATUS (NTAPI *PFN_WDF_CLASS_LIBRARY_BIND_CLIENT)(
    _In_ PWDF_CLASS_BIND_INFO ClassBindInfo,
    _Inout_ PWDF_COMPONENT_GLOBALS* ClientGlobals);

typedef VOID (NTAPI *PFN_WDF_CLASS_LIBRARY_UNBIND_CLIENT)(
    _In_ PWDF_CLASS_BIND_INFO ClassBindInfo,
    _Inout_ PWDF_COMPONENT_GLOBALS* ClientGlobals);

typedef NTSTATUS (NTAPI *PFN_WDF_CLIENT_BIND_CLASS)(
    _In_ PWDF_CLASS_BIND_INFO ClassBindInfo,
    _In_ PWDF_COMPONENT_GLOBALS ComponentGlobals);

typedef VOID (NTAPI *PFN_WDF_CLIENT_UNBIND_CLASS)(
    _In_ PWDF_CLASS_BIND_INFO ClassBindInfo,
    _In_ PWDF_COMPONENT_GLOBALS ComponentGlobals);

#endif // _WDFCX_H_
