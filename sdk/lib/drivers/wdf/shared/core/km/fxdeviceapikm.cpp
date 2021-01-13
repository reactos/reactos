/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxDeviceApiKm.cpp

Abstract:

    This module exposes the "C" interface to the FxDevice object.

Author:



Environment:

    Kernel mode only

Revision History:

--*/

#include "coreprivshared.hpp"
#include "fxiotarget.hpp"

extern "C" {
// #include "FxDeviceApiKm.tmh"
}

//
// extern "C" the entire file
//
extern "C" {

//
// Verifier Functions
//
// Do not specify argument names
FX_DECLARE_VF_FUNCTION_P3(
VOID,
VerifyWdfDeviceWdmDispatchIrp,
    _In_ PWDF_DRIVER_GLOBALS,
    _In_ FxDevice*,
    _In_ WDFCONTEXT
    );

// Do not specify argument names
FX_DECLARE_VF_FUNCTION_P4(
NTSTATUS,
VerifyWdfDeviceWdmDispatchIrpToIoQueue,
    _In_ FxDevice*,
    _In_ MdIrp,
    _In_ FxIoQueue*,
    _In_ ULONG
    );

__drv_maxIRQL(DISPATCH_LEVEL)
WDFDEVICE
STDCALL
WDFEXPORT(WdfWdmDeviceGetWdfDeviceHandle)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PDEVICE_OBJECT DeviceObject
    )
{
    FxPointerNotNull(GetFxDriverGlobals(DriverGlobals), DeviceObject);

    return FxDevice::GetFxDevice(DeviceObject)->GetHandle();
}

__drv_maxIRQL(DISPATCH_LEVEL)
PDEVICE_OBJECT
STDCALL
WDFEXPORT(WdfDeviceWdmGetDeviceObject)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device
    )
{
    FxDeviceBase *pDevice;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         Device,
                         FX_TYPE_DEVICE_BASE,
                         (PVOID*) &pDevice);

    return pDevice->GetDeviceObject();
}

__drv_maxIRQL(DISPATCH_LEVEL)
PDEVICE_OBJECT
STDCALL
WDFEXPORT(WdfDeviceWdmGetAttachedDevice)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device
    )
{
    FxDeviceBase *pDevice;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         Device,
                         FX_TYPE_DEVICE_BASE,
                         (PVOID*) &pDevice);

    return pDevice->GetAttachedDevice();
}


__drv_maxIRQL(DISPATCH_LEVEL)
PDEVICE_OBJECT
STDCALL
WDFEXPORT(WdfDeviceWdmGetPhysicalDevice)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device
    )
{
    FxDeviceBase *pDevice;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         Device,
                         FX_TYPE_DEVICE_BASE,
                         (PVOID*) &pDevice);

    return pDevice->GetPhysicalDevice();
}

__drv_maxIRQL(DISPATCH_LEVEL)
WDFFILEOBJECT
STDCALL
WDFEXPORT(WdfDeviceGetFileObject)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    MdFileObject FileObject
    )
/*++

Routine Description:

    This functions returns the WDFFILEOBJECT corresponding to the WDM fileobject.

Arguments:

    Device - Handle to the device to which the WDM fileobject is related to.

    FileObject - WDM FILE_OBJECT structure.

Return Value:

--*/

{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    NTSTATUS status;
    FxFileObject* pFxFO;
    FxDevice *pDevice;

    pFxDriverGlobals = GetFxDriverGlobals(DriverGlobals);






    pFxFO = NULL;

    //
    // Validate the Device object handle, and get its FxDevice*
    //
    FxObjectHandleGetPtr(pFxDriverGlobals,
                         Device,
                         FX_TYPE_DEVICE,
                         (PVOID*)&pDevice);

    //
    // Call the static GetFileObjectFromWdm function. This will return an error if the
    // WDM fileObject is NULL and the device is not exclusive or the device is
    // configured to have a WDFFILEOBJECT for every open handle.
    //
    status = FxFileObject::_GetFileObjectFromWdm(
        pDevice,
        pDevice->GetFileObjectClass(),
        FileObject,
        &pFxFO
        );

    if (!NT_SUCCESS(status)) {
         DoTraceLevelMessage(
             pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
             "FxFileObject::_GetFileObjectFromWdm returned an error %!STATUS!",
             status);
         return NULL;
    }

    //
    // pFxFO can be NULL if the device is configured with FileObjectClass WdfFileObjectNotRequired.
    //
    return pFxFO != NULL ? pFxFO->GetHandle() : NULL;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
STDCALL
WDFEXPORT(WdfDeviceWdmDispatchPreprocessedIrp)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    MdIrp Irp
    )
{
    FxDevice            *device;
    PFX_DRIVER_GLOBALS  fxDriverGlobals;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Device,
                                   FX_TYPE_DEVICE,
                                   (PVOID*) &device,
                                   &fxDriverGlobals);

    FxPointerNotNull(fxDriverGlobals, Irp);

    //
    // Verifier checks.
    // This API can only be called by the client driver, Cx must call
    // WdfDeviceWdmDispatchIrp from its preprocess callback.
    // Also, Cx must register for a Preprocessor routine using
    // WdfCxDeviceInitAssignWdmIrpPreprocessCallback.
    //
    if (fxDriverGlobals->IsVerificationEnabled(1, 11, OkForDownLevel)) {
        if (device->IsCxInIoPath()) {
            FxDriver* driver = GetFxDriverGlobals(DriverGlobals)->Driver;

            if (IsListEmpty(&device->m_PreprocessInfoListHead) ||
                device->IsCxDriverInIoPath(driver)) {

                DoTraceLevelMessage(
                        fxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                        "This API can only be called by client driver from its "
                        "pre-process IRP callback, STATUS_INVALID_DEVICE_REQUEST");
                FxVerifierDbgBreakPoint(fxDriverGlobals);
            }
        }
    }

    //
    // OK, ready to dispatch IRP.
    //
    return device->DispatchPreprocessedIrp(
                        Irp,
                        device->m_PreprocessInfoListHead.Flink->Flink);
}

VOID
FX_VF_FUNCTION(VerifyWdfDeviceWdmDispatchIrp) (
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals,
    _In_ PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_ FxDevice* device,
    _In_ WDFCONTEXT  DispatchContext
    )
{
    UNREFERENCED_PARAMETER(FxDriverGlobals);
    FxDriver*   driver;
    BOOLEAN     ctxValid;
    PLIST_ENTRY next;
    NTSTATUS    status;

    PAGED_CODE_LOCKED();

    status = STATUS_SUCCESS;
    driver = GetFxDriverGlobals(DriverGlobals)->Driver;
    ctxValid = (PLIST_ENTRY)DispatchContext ==
                &device->m_PreprocessInfoListHead ? TRUE : FALSE;
    //
    // Driver should be a cx.
    //
    if (device->IsCxDriverInIoPath(driver) == FALSE) {
        status = STATUS_INVALID_DEVICE_REQUEST;
        DoTraceLevelMessage(
                device->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIO,
                "This API can only be called by wdf extension driver "
                "from its pre-process IRP callback, %!STATUS!",
                status);
        FxVerifierDbgBreakPoint(device->GetDriverGlobals());
    }

    //
    // Validate DispatchContext.
    //

    for (next = device->m_PreprocessInfoListHead.Flink;
         next != &device->m_PreprocessInfoListHead;
         next = next->Flink) {
        if ((PLIST_ENTRY)DispatchContext == next) {
            ctxValid = TRUE;
            break;
        }
    }

    if (FALSE == ctxValid) {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(
                device->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIO,
                "DispatchContext 0x%p is invalid, %!STATUS!",
                DispatchContext, status);
        FxVerifierDbgBreakPoint(device->GetDriverGlobals());
    }
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
STDCALL
WDFEXPORT(WdfDeviceWdmDispatchIrp)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    MdIrp Irp,
    __in
    WDFCONTEXT DispatchContext
    )

/*++

Routine Description:

    Client driver calls this API from its dispatch callback when it decides to hand the IRP
    back to framework.

    Cx calls this API from (a) its pre-process callback or (b) its dispatch callback when
    it decides to hand the IRP back to the framework.

Arguments:
    Device - WDF Device handle.

    IRP - WDM request.

    DispatchContext - WDF's context (input arg to callback).

Returns:
    IRP's status.

--*/

{
    FxDevice *device;
    NTSTATUS status;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         Device,
                         FX_TYPE_DEVICE,
                         (PVOID*) &device);

    FxPointerNotNull(device->GetDriverGlobals(), Irp);
    FxPointerNotNull(device->GetDriverGlobals(), DispatchContext);

    if ((UCHAR)(ULONG_PTR)DispatchContext & FX_IN_DISPATCH_CALLBACK) {
        //
        // Called from a dispach irp callback.
        //
        DispatchContext =
            (WDFCONTEXT)((ULONG_PTR)DispatchContext & ~FX_IN_DISPATCH_CALLBACK);

        //
        // DispatchContext is validated by DispatchStep1.
        //
        status = device->m_PkgIo->DispatchStep1(Irp, DispatchContext);
    }
    else {
        //
        // Called from a pre-process irp callback.
        //

        //
        // Verifier checks.
        //
        VerifyWdfDeviceWdmDispatchIrp(device->GetDriverGlobals(),
                                      DriverGlobals,
                                      device,
                                      DispatchContext);

        status = device->DispatchPreprocessedIrp(Irp, DispatchContext);
    }

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
STDCALL
WDFEXPORT(WdfDeviceWdmDispatchIrpToIoQueue)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    MdIrp Irp,
    __in
    WDFQUEUE Queue,
    __in
    ULONG Flags
    )
{
    FxIoQueue           *queue;
    FxDevice            *device;
    PFX_DRIVER_GLOBALS  fxDriverGlobals;
    PIO_STACK_LOCATION  stack;
    NTSTATUS            status;
    FxIoInCallerContext* ioInCallerCtx;

    queue = NULL;
    ioInCallerCtx = NULL;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         Device,
                         FX_TYPE_DEVICE,
                         (PVOID*) &device);

    fxDriverGlobals = device->GetDriverGlobals();
    FX_TRACK_DRIVER(fxDriverGlobals);

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         Queue,
                         FX_TYPE_QUEUE,
                         (PVOID*)&queue);

    FxPointerNotNull(fxDriverGlobals, Irp);

    //
    // If the caller is a preprocess routine, the contract for this DDI is just like IoCallDriver.
    // The caller sets up their stack location and then the DDI advances to the next stack
    // location. This means that the caller either has to call IoSkipCurrentIrpStackLocation
    // or IoCopyCurrentIrpStackLocationToNext before calling this DDI.
    //
    if (Flags & WDF_DISPATCH_IRP_TO_IO_QUEUE_PREPROCESSED_IRP) {
        IoSetNextIrpStackLocation(Irp);
    }

    //
    // Verifier checks.
    //
    status = VerifyWdfDeviceWdmDispatchIrpToIoQueue(fxDriverGlobals,
                                                    device,
                                                    Irp,
                                                    queue,
                                                    Flags);
    if(!NT_SUCCESS(status)) {
        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return status;
    }

    //
    // Adjust stack if IRP needs to be forwarded to parent device.
    //
    if (device->m_ParentDevice == queue->GetDevice()) {
        IoCopyCurrentIrpStackLocationToNext(Irp);
        IoSetNextIrpStackLocation(Irp);

        //
        // From now on use new device.
        //
        device = device->m_ParentDevice;

        //
        // Save a pointer to the device object for this request so that it can
        // be used later in completion.
        //
        stack = IoGetCurrentIrpStackLocation(Irp);
        stack->DeviceObject = device->GetDeviceObject();
    }

    //
    // Get in-context caller callback if required.
    //
    if (Flags & WDF_DISPATCH_IRP_TO_IO_QUEUE_INVOKE_INCALLERCTX_CALLBACK) {
        ioInCallerCtx = device->m_PkgIo->GetIoInCallerContextCallback(
                                                queue->GetCxDeviceInfo());
    }

    //
    // DispatchStep2 will convert the IRP into a WDFREQUEST, queue it and if
    // possible dispatch the request to the driver.
    //
    return device->m_PkgIo->DispatchStep2(Irp, ioInCallerCtx, queue);
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
STDCALL
WDFEXPORT(WdfDeviceAddDependentUsageDeviceObject)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    PDEVICE_OBJECT DependentDevice
    )
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxDevice *pDevice;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Device,
                                   FX_TYPE_DEVICE,
                                   (PVOID *) &pDevice,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, DependentDevice);

    return pDevice->m_PkgPnp->AddUsageDevice(DependentDevice);
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
STDCALL
WDFEXPORT(WdfDeviceRemoveDependentUsageDeviceObject)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    PDEVICE_OBJECT DependentDevice
    )
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxDevice *pDevice;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Device,
                                   FX_TYPE_DEVICE,
                                   (PVOID *) &pDevice,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, DependentDevice);

    return pDevice->m_PkgPnp->RemoveUsageDevice(DependentDevice);
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
STDCALL
WDFEXPORT(WdfDeviceAssignMofResourceName)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    PCUNICODE_STRING MofResourceName
    )
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxDevice *pDevice;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Device,
                                   FX_TYPE_DEVICE,
                                   (PVOID *) &pDevice,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, MofResourceName);

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = FxValidateUnicodeString(pFxDriverGlobals, MofResourceName);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (pDevice->m_MofResourceName.Buffer !=  NULL) {
        status = STATUS_INVALID_DEVICE_REQUEST;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "WDFDEVICE %p MofResourceName already assigned, %!STATUS!",
            Device, status);

        return status;
    }

    status = FxDuplicateUnicodeString(pFxDriverGlobals,
                                      MofResourceName,
                                      &pDevice->m_MofResourceName);

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "WDFDEVICE %p couldn't creat duplicate buffer, %!STATUS!",
            Device, status);
    }

    return status;
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
STDCALL
WDFEXPORT(WdfDeviceSetSpecialFileSupport)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    WDF_SPECIAL_FILE_TYPE FileType,
    __in
    BOOLEAN Supported
    )
{
    FxDevice* pDevice;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Device,
                                   FX_TYPE_DEVICE,
                                   (PVOID *) &pDevice,
                                   &pFxDriverGlobals);

    if (FileType < WdfSpecialFilePaging  || FileType >= WdfSpecialFileMax) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "WDFDEVICE 0x%p FileType %d specified is not in valid range",
            Device, FileType);
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return;
    }

    FxObjectHandleGetPtr(pFxDriverGlobals,
                         Device,
                         FX_TYPE_DEVICE,
                         (PVOID*) &pDevice);

    pDevice->m_PkgPnp->SetSpecialFileSupport(FileType, Supported);
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
STDCALL
WDFEXPORT(WdfDeviceIndicateWakeStatus)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    NTSTATUS WaitWakeStatus
    )
{
    NTSTATUS status;
    FxDevice *pDevice;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Device,
                                   FX_TYPE_DEVICE,
                                   (PVOID *) &pDevice,
                                   &pFxDriverGlobals);

    if (Device == NULL ||
        WaitWakeStatus == STATUS_PENDING || WaitWakeStatus == STATUS_CANCELLED) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "NULL WDFDEVICE handle %p or invalid %!STATUS!",
                            Device, WaitWakeStatus);
        return STATUS_INVALID_PARAMETER;
    }

    if (pDevice->m_PkgPnp->m_SharedPower.m_WaitWakeOwner) {
        if (pDevice->m_PkgPnp->PowerIndicateWaitWakeStatus(WaitWakeStatus)) {
            status = STATUS_SUCCESS;
        }
        else {
            //
            // There was no request to complete
            //
            DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                                "WDFDEVICE 0x%p  No request to complete"
                                " STATUS_INVALID_DEVICE_REQUEST",
                                Device);

            status = STATUS_INVALID_DEVICE_REQUEST;
        }
    }
    else {
        //
        // We cannot complete what we do not own
        //
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "WDFDEVICE 0x%p  Not the waitwake owner"
                            " STATUS_INVALID_DEVICE_STATE",
                            Device);

        status = STATUS_INVALID_DEVICE_STATE;
    }

    return status;
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
STDCALL
WDFEXPORT(WdfDeviceSetBusInformationForChildren)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    PPNP_BUS_INFORMATION BusInformation
    )
{
    FxDevice* pDevice;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         Device,
                         FX_TYPE_DEVICE,
                         (PVOID*) &pDevice);

    FxPointerNotNull(pDevice->GetDriverGlobals(), BusInformation);

    pDevice->m_PkgPnp->SetChildBusInformation(BusInformation);
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
STDCALL
WDFEXPORT(WdfDeviceAddRemovalRelationsPhysicalDevice)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    PDEVICE_OBJECT PhysicalDevice
    )
/*++

Routine Description:
    Registers a PDO from another non descendant (not verifiable though) pnp
    stack to be reported as also requiring removal when this PDO is removed.

    The PDO could be another device enumerated by this driver.

Arguments:
    Device - this driver's PDO

    PhysicalDevice - PDO for another stack

Return Value:
    NTSTATUS

  --*/
{
    FxDevice* pDevice;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         Device,
                         FX_TYPE_DEVICE,
                         (PVOID*) &pDevice);

    FxPointerNotNull(pDevice->GetDriverGlobals(), PhysicalDevice);

    return pDevice->m_PkgPnp->AddRemovalDevice(PhysicalDevice);
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
STDCALL
WDFEXPORT(WdfDeviceRemoveRemovalRelationsPhysicalDevice)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    PDEVICE_OBJECT PhysicalDevice
    )
/*++

Routine Description:
    Deregisters a PDO from another non descendant (not verifiable though) pnp
    stack to not be reported as also requiring removal when this PDO is removed.

    The PDO could be another device enumerated by this driver.

Arguments:
    Device - this driver's PDO

    PhysicalDevice - PDO for another stack

Return Value:
    None

  --*/
{
    FxDevice* pDevice;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         Device,
                         FX_TYPE_DEVICE,
                         (PVOID*) &pDevice);

    FxPointerNotNull(pDevice->GetDriverGlobals(), PhysicalDevice);

    pDevice->m_PkgPnp->RemoveRemovalDevice(PhysicalDevice);
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
STDCALL
WDFEXPORT(WdfDeviceClearRemovalRelationsDevices)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device
    )
/*++

Routine Description:
    Deregisters all PDOs to not be reported as also requiring removal when this
    PDO is removed.

Arguments:
    Device - this driver's PDO

Return Value:
    None

  --*/
{
    FxDevice* pDevice;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         Device,
                         FX_TYPE_DEVICE,
                         (PVOID*) &pDevice);

    pDevice->m_PkgPnp->ClearRemovalDevicesList();
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
STDCALL
WDFEXPORT(WdfDeviceWdmAssignPowerFrameworkSettings)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    PWDF_POWER_FRAMEWORK_SETTINGS PowerFrameworkSettings
    )
/*++

Routine Description:
    The DDI is invoked by KMDF client drivers for single-component devices to
    specify their power framework settings to KMDF. KMDF uses these settings on
    Win8+ when registering with the power framework.

    On Win7 and older operating systems the power framework is not available, so
    KMDF does nothing.

Arguments:

    Device - Handle to the framework device object for which power framework
      settings are being specified.

    PowerFrameworkSettings - Pointer to a WDF_POWER_FRAMEWORK_SETTINGS structure
      that contains the client driver's power framework settings.

Return Value:
    An NTSTATUS value that denotes success or failure of the DDI

--*/
{
    NTSTATUS status;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxDevice *pDevice;

    //
    // Validate the Device object handle and get its FxDevice. Also get the
    // driver globals pointer.
    //
    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Device,
                                   FX_TYPE_DEVICE,
                                   (PVOID *) &pDevice,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, PowerFrameworkSettings);

    //
    // Only power policy owners should call this DDI
    //
    if (pDevice->m_PkgPnp->IsPowerPolicyOwner() == FALSE) {
        status = STATUS_INVALID_DEVICE_REQUEST;
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "WDFDEVICE 0x%p is not the power policy owner, so the caller cannot"
            " assign power framework settings %!STATUS!", Device, status);
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return status;
    }

    //
    // Validate the Settings parameter
    //
    if (PowerFrameworkSettings->Size != sizeof(WDF_POWER_FRAMEWORK_SETTINGS)) {
        status = STATUS_INFO_LENGTH_MISMATCH;
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "WDFDEVICE 0x%p Expected PowerFrameworkSettings size %d, actual %d,"
            " %!STATUS!",
            Device,
            sizeof(WDF_POWER_FRAMEWORK_SETTINGS),
            PowerFrameworkSettings->Size,
            status);
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return status;
    }

    //
    // If settings for component 0 are specified, make sure it contains at least
    // one F-state.
    //
    if (NULL != PowerFrameworkSettings->Component) {

        if (0 == PowerFrameworkSettings->Component->IdleStateCount) {
            status = STATUS_INVALID_PARAMETER;
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                "WDFDEVICE 0x%p Component settings are specified but "
                "IdleStateCount is 0. %!STATUS!", Device, status);
            FxVerifierDbgBreakPoint(pFxDriverGlobals);
            return status;
        }

        if (NULL == PowerFrameworkSettings->Component->IdleStates) {
            status = STATUS_INVALID_PARAMETER;
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                "WDFDEVICE 0x%p Component settings are specified but IdleStates"
                " is NULL. %!STATUS!", Device, status);
            FxVerifierDbgBreakPoint(pFxDriverGlobals);
            return status;
        }
    }

    //
    // Assign the driver's settings
    //
    status = pDevice->m_PkgPnp->AssignPowerFrameworkSettings(
                                            PowerFrameworkSettings);

    return status;
}

} // extern "C"
