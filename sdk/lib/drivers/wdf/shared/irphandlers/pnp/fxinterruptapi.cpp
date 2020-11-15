/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxInterruptApi.cpp

Abstract:

    This implements the WDFINTERRUPT API's

Author:




Environment:

    Both kernel and user mode

Revision History:


--*/

#include "pnppriv.hpp"

extern "C" {
// #include "FxInterruptApi.tmh"
}

//
// At this time we are unable to include wdf19.h in the share code, thus for
// now we simply cut and paste the needed structures.
//
typedef struct _WDF_INTERRUPT_CONFIG_V1_9 {
    ULONG              Size;

    //
    // If this interrupt is to be synchronized with other interrupt(s) assigned
    // to the same WDFDEVICE, create a WDFSPINLOCK and assign it to each of the
    // WDFINTERRUPTs config.
    //
    WDFSPINLOCK        SpinLock;

    WDF_TRI_STATE      ShareVector;

    BOOLEAN            FloatingSave;

    //
    // Automatic Serialization of the DpcForIsr
    //
    BOOLEAN            AutomaticSerialization;

    // Event Callbacks
    PFN_WDF_INTERRUPT_ISR         EvtInterruptIsr;

    PFN_WDF_INTERRUPT_DPC         EvtInterruptDpc;

    PFN_WDF_INTERRUPT_ENABLE      EvtInterruptEnable;

    PFN_WDF_INTERRUPT_DISABLE     EvtInterruptDisable;

} WDF_INTERRUPT_CONFIG_V1_9, *PWDF_INTERRUPT_CONFIG_V1_9;

//
// The interrupt config structure has changed post win8-Beta. This is a
// temporary definition to allow beta drivers to load on post-beta builds.
// Note that size of win8-beta and win8-postbeta structure is different only on
// non-x64 platforms, but the fact that size is same on amd64 is harmless because
// the struture gets zero'out by init macro, and the default value of the new
// field is 0 on amd64.
//
typedef struct _WDF_INTERRUPT_CONFIG_V1_11_BETA {
    ULONG              Size;

    //
    // If this interrupt is to be synchronized with other interrupt(s) assigned
    // to the same WDFDEVICE, create a WDFSPINLOCK and assign it to each of the
    // WDFINTERRUPTs config.
    //
    WDFSPINLOCK                     SpinLock;

    WDF_TRI_STATE                   ShareVector;

    BOOLEAN                         FloatingSave;

    //
    // DIRQL handling: automatic serialization of the DpcForIsr/WaitItemForIsr.
    // Passive-level handling: automatic serialization of all callbacks.
    //
    BOOLEAN                         AutomaticSerialization;

    //
    // Event Callbacks
    //
    PFN_WDF_INTERRUPT_ISR           EvtInterruptIsr;
    PFN_WDF_INTERRUPT_DPC           EvtInterruptDpc;
    PFN_WDF_INTERRUPT_ENABLE        EvtInterruptEnable;
    PFN_WDF_INTERRUPT_DISABLE       EvtInterruptDisable;
    PFN_WDF_INTERRUPT_WORKITEM      EvtInterruptWorkItem;

    //
    // These fields are only used when interrupt is created in
    // EvtDevicePrepareHardware callback.
    //
    PCM_PARTIAL_RESOURCE_DESCRIPTOR InterruptRaw;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR InterruptTranslated;

    //
    // Optional passive lock for handling interrupts at passive-level.
    //
    WDFWAITLOCK                     WaitLock;

    //
    // TRUE: handle interrupt at passive-level.
    // FALSE: handle interrupt at DIRQL level. This is the default.
    //
    BOOLEAN                         PassiveHandling;

} WDF_INTERRUPT_CONFIG_V1_11_BETA, *PWDF_INTERRUPT_CONFIG_V1_11_BETA;

//
// Interrupt Configuration Structure
//
typedef struct _WDF_INTERRUPT_CONFIG_V1_11 {
    ULONG              Size;

    //
    // If this interrupt is to be synchronized with other interrupt(s) assigned
    // to the same WDFDEVICE, create a WDFSPINLOCK and assign it to each of the
    // WDFINTERRUPTs config.
    //
    WDFSPINLOCK                     SpinLock;

    WDF_TRI_STATE                   ShareVector;

    BOOLEAN                         FloatingSave;

    //
    // DIRQL handling: automatic serialization of the DpcForIsr/WaitItemForIsr.
    // Passive-level handling: automatic serialization of all callbacks.
    //
    BOOLEAN                         AutomaticSerialization;

    //
    // Event Callbacks
    //
    PFN_WDF_INTERRUPT_ISR           EvtInterruptIsr;
    PFN_WDF_INTERRUPT_DPC           EvtInterruptDpc;
    PFN_WDF_INTERRUPT_ENABLE        EvtInterruptEnable;
    PFN_WDF_INTERRUPT_DISABLE       EvtInterruptDisable;
    PFN_WDF_INTERRUPT_WORKITEM      EvtInterruptWorkItem;

    //
    // These fields are only used when interrupt is created in
    // EvtDevicePrepareHardware callback.
    //
    PCM_PARTIAL_RESOURCE_DESCRIPTOR InterruptRaw;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR InterruptTranslated;

    //
    // Optional passive lock for handling interrupts at passive-level.
    //
    WDFWAITLOCK                     WaitLock;

    //
    // TRUE: handle interrupt at passive-level.
    // FALSE: handle interrupt at DIRQL level. This is the default.
    //
    BOOLEAN                         PassiveHandling;

    //
    // TRUE: Interrupt is reported inactive on explicit power down
    //       instead of disconnecting it.
    // FALSE: Interrupt is disconnected instead of reporting inactive
    //        on explicit power down.
    // DEFAULT: Framework decides the right value.
    //
    WDF_TRI_STATE                   ReportInactiveOnPowerDown;

} WDF_INTERRUPT_CONFIG_V1_11, *PWDF_INTERRUPT_CONFIG_V1_11;

//
// extern "C" the entire file
//
extern "C" {

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
STDCALL
WDFEXPORT(WdfInterruptCreate)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    PWDF_INTERRUPT_CONFIG Configuration,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES Attributes,
    __out
    WDFINTERRUPT* Interrupt
    )

/*++

Routine Description:

    Create an Interrupt object along with its associated DpcForIsr.

Arguments:

    Object      - Handle to a WDFDEVICE or WDFQUEUE to associate the interrupt with.

    pConfiguration - WDF_INTERRUPT_CONFIG structure.

    pAttributes - Optional WDF_OBJECT_ATTRIBUTES to request a context
                  memory allocation, and a DestroyCallback.

    pInterrupt  - Pointer to location to return the resulting WDFINTERRUPT handle.

Returns:

    STATUS_SUCCESS - A WDFINTERRUPT handle has been created.

--*/

{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS      pFxDriverGlobals;
    FxInterrupt*            pFxInterrupt;
    FxDevice*               pDevice;
    FxObject*               pParent;
    NTSTATUS                status;
    WDF_DEVICE_PNP_STATE    devicePnpState;
    ULONG                   expectedConfigSize;
    WDF_INTERRUPT_CONFIG    intConfig;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Device,
                                   FX_TYPE_DEVICE,
                                   (PVOID*)&pDevice,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, Configuration);
    FxPointerNotNull(pFxDriverGlobals, Interrupt);

    if (pFxDriverGlobals->IsVersionGreaterThanOrEqualTo(1,13)) {
        expectedConfigSize = sizeof(WDF_INTERRUPT_CONFIG);
    } else if (pFxDriverGlobals->IsVersionGreaterThanOrEqualTo(1,11)) {
        expectedConfigSize = sizeof(WDF_INTERRUPT_CONFIG_V1_11);
    } else {
        expectedConfigSize = sizeof(WDF_INTERRUPT_CONFIG_V1_9);
    }


    //
    // We could check if Configuration->Size != expectedConfigSize, but
    // logic below allows me to installing old WDF drivers v1.11 on new WDF.
    //
    if (Configuration->Size != sizeof(WDF_INTERRUPT_CONFIG) &&
        Configuration->Size != sizeof(WDF_INTERRUPT_CONFIG_V1_11) &&
        Configuration->Size != sizeof(WDF_INTERRUPT_CONFIG_V1_11_BETA) &&
        Configuration->Size != sizeof(WDF_INTERRUPT_CONFIG_V1_9)) {

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
            "WDF_INTERRUPT_CONFIG size 0x%x incorrect, expected 0x%x",
            Configuration->Size, expectedConfigSize);

        return STATUS_INFO_LENGTH_MISMATCH;
    }

    //
    // Normalize WDF_INTERRUPT_CONFIG structure.
    //
    if (Configuration->Size < sizeof(WDF_INTERRUPT_CONFIG)) {
        //
        // Init new fields to default values.
        //
        WDF_INTERRUPT_CONFIG_INIT(&intConfig,
                                  Configuration->EvtInterruptIsr,
                                  Configuration->EvtInterruptDpc);
        //
        // Copy over existing fields and readjust the struct size.
        //
        RtlCopyMemory(&intConfig, Configuration, Configuration->Size);
        intConfig.Size = sizeof(intConfig);

        //
        // Use new config structure from now on.
        //
        Configuration = &intConfig;
    }

    //
    // Parameter validation.
    //

#if (FX_CORE_MODE == FX_CORE_USER_MODE)
    //
    // Ensure access to interrupts is allowed
    //
    FX_VERIFY_WITH_NAME(DRIVER(BadArgument, TODO),
        CHECK(ERROR_STRING_HW_ACCESS_NOT_ALLOWED,
        (pDevice->IsInterruptAccessAllowed() == TRUE)),
        DriverGlobals->DriverName);

    //
    // PassiveHandling member must be set to TRUE. INIT macro does that for
    // UMDF. This check ensures the field is not overriden by caller.
    //
    FX_VERIFY_WITH_NAME(DRIVER(BadArgument, TODO),
        CHECK("PassiveHandling not set to TRUE in WDF_INTERRUPT_CONFIG structure",
        (Configuration->PassiveHandling == TRUE)),
        DriverGlobals->DriverName);
#endif

    if (Configuration->EvtInterruptIsr == NULL) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
            "NULL EvtInterruptIsr in WDF_INTERRUPT_CONFIG structure 0x%p"
            " passed", Configuration);
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Driver can specify a parent only in the AutomaticSerialization case.
    //
    status = FxValidateObjectAttributes(
                        pFxDriverGlobals,
                        Attributes,
                        Configuration->AutomaticSerialization ?
                            FX_VALIDATE_OPTION_NONE_SPECIFIED :
                            FX_VALIDATE_OPTION_PARENT_NOT_ALLOWED);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (Attributes != NULL && Attributes->ParentObject != NULL) {
        FxObjectHandleGetPtr(pFxDriverGlobals,
                             Attributes->ParentObject,
                             FX_TYPE_OBJECT,
                            (PVOID*)&pParent);
    }
    else {
        pParent = (FxObject*)pDevice;
    }

    devicePnpState = pDevice->GetDevicePnpState();

    if (devicePnpState != WdfDevStatePnpInit &&
        0x0 == (pDevice->GetCallbackFlags() &
                    FXDEVICE_CALLBACK_IN_PREPARE_HARDWARE)) {

        status = STATUS_INVALID_DEVICE_STATE;

        DoTraceLevelMessage(
          pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
          "WDFINTERRUPTs can only be created during: "
          "(1) EvtDriverDeviceAdd when the WDFDEVICE %p is initially created, or "
          "(2) EvtDevicePrepareHardware, %!STATUS!", Device, status);

        return status;
    }

    //
    // Interrupts created in EvtDriverDeviceAdd must not specify any
    // CM resources b/c driver doesn't known them yet.
    //
    if (devicePnpState == WdfDevStatePnpInit) {

        if (Configuration->InterruptRaw != NULL ||
            Configuration->InterruptTranslated != NULL) {

            status = STATUS_INVALID_PARAMETER;

            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                "Not NULL InterruptRaw or InterruptTranslated in "
                "WDF_INTERRUPT_CONFIG structure 0x%p passed, %!STATUS!",
                Configuration, status);

            return status;
        }

        //
        // Wake Interrupts can not be created in AddDevice as it is
        // not known if they are actually marked as wake capable
        //
        if (Configuration->CanWakeDevice) {
            status = STATUS_INVALID_PARAMETER;

            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                "CanWakeDevice set in WDF_INTERRUPT_CONFIG structure  0x%p"
                "during EvtDriverDeviceAdd, %!STATUS!",
                Configuration, status);

            return status;
        }
    }

    //
    // Checks for interrupt created in EvtDevicePrepareHardware.
    //
    if (devicePnpState != WdfDevStatePnpInit) {
        //
        // CM resources must be specified.
        //
        if (Configuration->InterruptRaw == NULL ||
            Configuration->InterruptTranslated == NULL) {

            status = STATUS_INVALID_DEVICE_STATE;

            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                "NULL InterruptRaw or InterruptTranslated in "
                "WDF_INTERRUPT_CONFIG structure 0x%p passed, %!STATUS!",
                Configuration, status);

            return status;
        }

        //
        // Share vector must be default.
        //
        if (Configuration->ShareVector != WdfUseDefault) {
            status = STATUS_INVALID_DEVICE_STATE;

            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                "Driver cannot specify ShareVector different from "
                "WdfUseDefault in EvtDevicePrepareHardware callback,"
                "WDF_INTERRUPT_CONFIG structure 0x%p passed, %!STATUS!",
                Configuration, status);

            return status;
        }

    }

    if (Configuration->CanWakeDevice) {
        if (FxInterrupt::_IsWakeHintedInterrupt(
            Configuration->InterruptTranslated->Flags) == FALSE) {

            status = STATUS_INVALID_PARAMETER;
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                "Driver cannot specify an interrupt as CanWakeDevice if "
                "the CM_RESOURCE_INTERRUPT_WAKE_HINT is not present."
                "WDF_INTERRUPT_CONFIG structure 0x%p passed, %!STATUS!",
                Configuration, status);

            return status;
        }

        //
        // Driver is allowed to create wake interrupt only if it
        // is the power policy owner so that the we can wake the
        // stack when the interrupt fires
        //
        if (pDevice->m_PkgPnp->IsPowerPolicyOwner() == FALSE) {
            status = STATUS_INVALID_PARAMETER;
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                "Driver cannot specify an interrupt as CanWakeDevice if "
                "it is not power policy powner. WDF_INTERRUPT_CONFIG "
                "structure 0x%p passed, %!STATUS!",
                Configuration, status);

            return status;
        }

        //
        // Declaring an interrupt as wake capable allows it to
        // take ownership of interrupt from ACPI. It does not
        // make sense for a PDO
        //
        if (pDevice->IsPdo()) {
            status = STATUS_INVALID_PARAMETER;
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                "Driver cannot specify an interrupt as CanWakeDevice for a PDO "
                "WDF_INTERRUPT_CONFIG structure 0x%p passed, %!STATUS!",
                Configuration, status);

            return status;
        }
    }

    if (Configuration->EvtInterruptDpc != NULL &&
        Configuration->EvtInterruptWorkItem != NULL) {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
            "Driver can provide either EvtInterruptDpc or "
            "EvtInterruptWorkItem callback but not both. "
            "WDF_INTERRUPT_CONFIG structure 0x%p passed, %!STATUS!",
            Configuration, status);

        return status;
    }

    if (Configuration->PassiveHandling == FALSE) {
        //
        // DIRQL handling validations.
        //
        if (Configuration->WaitLock) {
            status = STATUS_INVALID_PARAMETER;
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                "Driver cannot specify WaitLock when handling interrupts at "
                "DIRQL, WDF_INTERRUPT_CONFIG structure 0x%p passed, %!STATUS!",
                Configuration, status);

            return status;
        }
        //
        // Wake interrupts should be passive level so that the stack
        // can be sychronously bring the device back to D0 form within
        // the ISR
        //
        if (Configuration->CanWakeDevice) {
            status = STATUS_INVALID_PARAMETER;

            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                "CanWakeDevice set in WDF_INTERRUPT_CONFIG structure for an"
                "interrupt not marked as passive 0x%p, %!STATUS!",
                Configuration, status);

            return status;
        }
    }
    else {
        //
        // Passive-level handling validations.
        //
        if (FxIsPassiveLevelInterruptSupported() == FALSE) {
            status = STATUS_NOT_SUPPORTED;
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                "The current version of Windows does not support the handling "
                "of interrupts at passive-level, WDF_INTERRUPT_CONFIG "
                "structure 0x%p passed, %!STATUS!", Configuration, status);

            return status;
        }

        if (Configuration->SpinLock) {
            status = STATUS_INVALID_PARAMETER;
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                "Driver cannot specify SpinLock when handling interrupts at "
                "passive-level, WDF_INTERRUPT_CONFIG structure 0x%p passed, "
                "%!STATUS!", Configuration, status);

            return status;
        }


       //
       // For UMDF reflector decides whether to handle the interrupt
       // at passive or DIRQL. Driver has no choice. Therefore this check
       // is applicable only for KMDF.
       //
#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
        if (Configuration->InterruptTranslated != NULL &&
            FxInterrupt::_IsMessageInterrupt(
                Configuration->InterruptTranslated->Flags)) {
            status = STATUS_INVALID_PARAMETER;
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                "Driver cannot specify PassiveHandling for MSI interrupts, "
                "WDF_INTERRUPT_CONFIG structure 0x%p passed, %!STATUS!",
                Configuration, status);

            return status;
        }
#endif
    }

    //
    // Make sure the specified resources are valid.
    // Do this check only under verifier b/c if the driver has a lot of
    // interrupts/resources, this verification can slow down power up.
    // Note that if InterruptRaw is != NULL, it implies that
    // InterruptTranslated is != NULL from the checks above.
    //
    if (pFxDriverGlobals->FxVerifierOn) {
        if (Configuration->InterruptRaw != NULL ) {
            status = pDevice->m_PkgPnp->ValidateInterruptResourceCm(
                                        Configuration->InterruptRaw,
                                        Configuration->InterruptTranslated,
                                        Configuration);
            if (!NT_SUCCESS(status)) {
                return status;
            }
        }
    }

    status = FxInterrupt::_CreateAndInit(
            pFxDriverGlobals,
            pDevice,
            pParent,
            Attributes,
            Configuration,
            &pFxInterrupt);

    if (NT_SUCCESS(status)) {
        *Interrupt = (WDFINTERRUPT) pFxInterrupt->GetObjectHandle();
    }

    return status;
}

BOOLEAN
STDCALL
WDFEXPORT(WdfInterruptQueueDpcForIsr)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFINTERRUPT Interrupt
    )

/*++

Routine Description:

    Queue the DPC for the ISR

Arguments:

    Interrupt - Handle to WDFINTERUPT object created with WdfInterruptCreate.

Returns:

    TRUE - If the DPC has been enqueued.

    FALSE - If the DPC has already been enqueued.

--*/

{
    DDI_ENTRY();

    FxInterrupt* pFxInterrupt;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         Interrupt,
                         FX_TYPE_INTERRUPT,
                         (PVOID*)&pFxInterrupt);

    return pFxInterrupt->QueueDeferredRoutineForIsr();
}

BOOLEAN
STDCALL
WDFEXPORT(WdfInterruptQueueWorkItemForIsr)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFINTERRUPT Interrupt
    )

/*++

Routine Description:

    Queue the interrupt's work-item for the ISR.

Arguments:

    Interrupt - Handle to WDFINTERUPT object created with WdfInterruptCreate.

Returns:

    TRUE - If the work-item has been enqueued.

    FALSE - If the work-item has already been enqueued.


--*/

{
    DDI_ENTRY();

    FxInterrupt*    pFxInterrupt;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         Interrupt,
                         FX_TYPE_INTERRUPT,
                         (PVOID*)&pFxInterrupt);

    return pFxInterrupt->QueueWorkItemForIsr();
}

__drv_maxIRQL(DISPATCH_LEVEL)
BOOLEAN
STDCALL
WDFEXPORT(WdfInterruptSynchronize)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFINTERRUPT Interrupt,
    __in
    PFN_WDF_INTERRUPT_SYNCHRONIZE Callback,
    __in
    WDFCONTEXT Context
    )

/*++

Routine Description:

    Synchronize execution with the interrupt handler

Arguments:

    Interrupt - Handle to WDFINTERUPT object created with WdfInterruptCreate.

    Callback     - PWDF_INTERRUPT_SYNCHRONIZE callback function to invoke

    Context      - Context to pass to the PFN_WDF_INTERRUPT_SYNCHRONIZE callback

Returns:

    BOOLEAN result from user PFN_WDF_INTERRUPT_SYNCHRONIZE callback

--*/

{
    DDI_ENTRY();

    FxInterrupt*    pFxInterrupt;
    NTSTATUS        status;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         Interrupt,
                         FX_TYPE_INTERRUPT,
                         (PVOID*)&pFxInterrupt);

    if (pFxInterrupt->IsPassiveHandling()) {
        status = FxVerifierCheckIrqlLevel(pFxInterrupt->GetDriverGlobals(),
                                          PASSIVE_LEVEL);
        if (!NT_SUCCESS(status)) {
            return FALSE;
        }
    }

    FxPointerNotNull(pFxInterrupt->GetDriverGlobals(), Callback);

    return pFxInterrupt->Synchronize(Callback, Context);
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
STDCALL
WDFEXPORT(WdfInterruptAcquireLock)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    _Requires_lock_not_held_(_Curr_)
    _Acquires_lock_(_Curr_)
    WDFINTERRUPT Interrupt
    )

/*++

Routine Description:

    Begin synchronization to the interrupts IRQL level

Arguments:

    Interrupt - Handle to WDFINTERUPT object created with WdfInterruptCreate.

Returns:

--*/

{
    DDI_ENTRY();

    FxInterrupt*    pFxInterrupt;
    NTSTATUS        status;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         Interrupt,
                         FX_TYPE_INTERRUPT,
                         (PVOID*)&pFxInterrupt);

    if (pFxInterrupt->IsPassiveHandling()) {
        status = FxVerifierCheckIrqlLevel(pFxInterrupt->GetDriverGlobals(),
                                          PASSIVE_LEVEL);
        if (!NT_SUCCESS(status)) {
            return;
        }
    }

    pFxInterrupt->AcquireLock();
}

__drv_maxIRQL(DISPATCH_LEVEL + 1)
VOID
STDCALL
WDFEXPORT(WdfInterruptReleaseLock)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    _Requires_lock_held_(_Curr_)
    _Releases_lock_(_Curr_)
    WDFINTERRUPT Interrupt
    )

/*++

Routine Description:

    End synchronization to the interrupts IRQL level

Arguments:

    Interrupt - Handle to WDFINTERUPT object created with WdfInterruptCreate.

Returns:

--*/

{
    DDI_ENTRY();

    FxInterrupt*    pFxInterrupt;
    NTSTATUS        status;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         Interrupt,
                         FX_TYPE_INTERRUPT,
                         (PVOID*)&pFxInterrupt);

    if (pFxInterrupt->IsPassiveHandling()) {
        status = FxVerifierCheckIrqlLevel(pFxInterrupt->GetDriverGlobals(),
                                          PASSIVE_LEVEL);
        if (!NT_SUCCESS(status)) {
            return;
        }
    }

    pFxInterrupt->ReleaseLock();
}

__drv_maxIRQL(PASSIVE_LEVEL)
VOID
STDCALL
WDFEXPORT(WdfInterruptEnable)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFINTERRUPT Interrupt
    )

/*++

Routine Description:

    Request that the interrupt be enabled in the hardware.

Arguments:

    Interrupt - Handle to WDFINTERUPT object created with WdfInterruptCreate.

Returns:

--*/

{
    DDI_ENTRY();

    FxInterrupt* pFxInterrupt;
    NTSTATUS status;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         Interrupt,
                         FX_TYPE_INTERRUPT,
                         (PVOID*)&pFxInterrupt);

    status = FxVerifierCheckIrqlLevel(pFxInterrupt->GetDriverGlobals(),
                                      PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return;
    }

    //
    // Okay to ignore error status. Called function prints error messages.
    //
    (VOID) pFxInterrupt->ForceReconnect();
}

__drv_maxIRQL(PASSIVE_LEVEL)
VOID
STDCALL
WDFEXPORT(WdfInterruptDisable)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFINTERRUPT Interrupt
    )

/*++

Routine Description:

    Request that the interrupt be disabled in the hardware.

Arguments:

    Interrupt - Handle to WDFINTERUPT object created with WdfInterruptCreate.

Returns:

--*/

{
    DDI_ENTRY();

    FxInterrupt* pFxInterrupt;
    NTSTATUS status;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         Interrupt,
                         FX_TYPE_INTERRUPT,
                         (PVOID*)&pFxInterrupt);

    status = FxVerifierCheckIrqlLevel(pFxInterrupt->GetDriverGlobals(),
                                      PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return;
    }

    //
    // Okay to ignore error status. Called function prints error messages.
    //
    (VOID) pFxInterrupt->ForceDisconnect();
}

_Must_inspect_result_
struct _KINTERRUPT*
STDCALL
WDFEXPORT(WdfInterruptWdmGetInterrupt)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFINTERRUPT Interrupt
    )

/*++

Routine Description:

    Return the WDM _KINTERRUPT*

Arguments:

    Interrupt - Handle to WDFINTERUPT object created with WdfInterruptCreate.

Returns:

--*/

{
    DDI_ENTRY();

    FxInterrupt* pFxInterrupt;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         Interrupt,
                         FX_TYPE_INTERRUPT,
                         (PVOID*)&pFxInterrupt);

    return pFxInterrupt->GetInterruptPtr();
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
STDCALL
WDFEXPORT(WdfInterruptGetInfo)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFINTERRUPT Interrupt,
    __out
    PWDF_INTERRUPT_INFO    Info
    )

/*++

Routine Description:

    Return the WDF_INTERRUPT_INFO that describes this
    particular Message Signaled Interrupt MessageID.

Arguments:

    Interrupt - Handle to WDFINTERUPT object created with WdfInterruptCreate.

Returns:
    Nothing

--*/
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS  pFxDriverGlobals    = NULL;
    FxInterrupt*        pFxInterrupt        = NULL;
    ULONG               size                = 0;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Interrupt,
                                   FX_TYPE_INTERRUPT,
                                   (PVOID*)&pFxInterrupt,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, Info);

    if (sizeof(WDF_INTERRUPT_INFO_V1_7) == Info->Size) {
        size = sizeof(WDF_INTERRUPT_INFO_V1_7);
    }
    else if (sizeof(WDF_INTERRUPT_INFO) == Info->Size) {
        size = sizeof(WDF_INTERRUPT_INFO);
    }
    else {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
            "WDF_INTERRUPT_INFO %p Size %d invalid, expected %d",
            Interrupt, Info->Size, sizeof(WDF_INTERRUPT_INFO));
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return;
    }

    RtlCopyMemory(Info, pFxInterrupt->GetInfo(), size);

    Info->Size = size;
}

WDFDEVICE
STDCALL
WDFEXPORT(WdfInterruptGetDevice)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFINTERRUPT Interrupt
    )

/*++

Routine Description:

    Get the device that the interrupt is related to.

Arguments:

    Interrupt - Handle to WDFINTERUPT object created with WdfInterruptCreate.

Returns:

    WDFDEVICE

--*/

{
    DDI_ENTRY();

    FxInterrupt* pFxInterrupt;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         Interrupt,
                         FX_TYPE_INTERRUPT,
                         (PVOID*)&pFxInterrupt);

    return pFxInterrupt->GetDevice()->GetHandle();
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
STDCALL
WDFEXPORT(WdfInterruptSetPolicy)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFINTERRUPT Interrupt,
    __in
    WDF_INTERRUPT_POLICY Policy,
    __in
    WDF_INTERRUPT_PRIORITY Priority,
    __in
    KAFFINITY TargetProcessorSet
    )

/*++

Routine Description:

    Interrupts have attributes that a driver might want to influence.  This
    routine tells the Framework to tell the PnP manager what the driver would
    prefer.

Arguments:

    Interrupt - Handle to WDFINTERUPT object created with WdfInterruptCreate.

Returns:

    NTSTATUS

--*/

{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS  pFxDriverGlobals = NULL;
    FxInterrupt*        pFxInterrupt     = NULL;
    GROUP_AFFINITY      processorSet;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Interrupt,
                                   FX_TYPE_INTERRUPT,
                                   (PVOID*)&pFxInterrupt,
                                   &pFxDriverGlobals);

    if (Policy < WdfIrqPolicyMachineDefault ||
        Policy > WdfIrqPolicySpreadMessagesAcrossAllProcessors) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
            "Policy %d is out of range", Policy);
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return;
    }

    if (Priority < WdfIrqPriorityLow || Priority > WdfIrqPriorityHigh) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
            "Priority %d is out of range", Priority);
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return;
    }

    RtlZeroMemory(&processorSet, sizeof(processorSet));
    processorSet.Mask = TargetProcessorSet;

    pFxInterrupt->SetPolicy(Policy, Priority, &processorSet);
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
STDCALL
WDFEXPORT(WdfInterruptSetExtendedPolicy)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFINTERRUPT Interrupt,
    __in
    PWDF_INTERRUPT_EXTENDED_POLICY PolicyAndGroup
    )

/*++

Routine Description:

    Interrupts have attributes that a driver might want to influence.  This
    routine tells the Framework to tell the PnP manager what the driver would
    prefer.

Arguments:

    Interrupt - Handle to WDFINTERUPT object created with WdfInterruptCreate.

    PolicyAndGroup - Driver preference for policy, priority and group affinity.

Returns:

    NTSTATUS

--*/

{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS      pFxDriverGlobals    = NULL;
    FxInterrupt*            pFxInterrupt        = NULL;
    PGROUP_AFFINITY         processorSet        = NULL;
    WDF_INTERRUPT_POLICY    policy;
    WDF_INTERRUPT_PRIORITY  priority;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Interrupt,
                                   FX_TYPE_INTERRUPT,
                                   (PVOID*)&pFxInterrupt,
                                   &pFxDriverGlobals);

    if (PolicyAndGroup->Size != sizeof(WDF_INTERRUPT_EXTENDED_POLICY)) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
            "WDF_INTERRUPT_EXTENDED_POLICY %p Size %d invalid, expected %d",
            PolicyAndGroup,
            PolicyAndGroup->Size,
            sizeof(WDF_INTERRUPT_EXTENDED_POLICY));
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return;
    }

    policy = PolicyAndGroup->Policy;
    priority = PolicyAndGroup->Priority;
    processorSet = &PolicyAndGroup->TargetProcessorSetAndGroup;

    if (policy < WdfIrqPolicyMachineDefault ||
        policy > WdfIrqPolicySpreadMessagesAcrossAllProcessors) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
            "Policy %d is out of range", policy);
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return;
    }

    if (priority < WdfIrqPriorityLow || priority > WdfIrqPriorityHigh) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
            "Priority %d is out of range", priority);
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return;
    }

    if (processorSet->Reserved[0] != 0 || processorSet->Reserved[1] != 0 ||
        processorSet->Reserved[2] != 0) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
            "TargetProcessorSet's reserved fields are not zero");
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return;
    }

    pFxInterrupt->SetPolicy(policy, priority, processorSet);
}

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
_Post_satisfies_(return == 1 || return == 0)
BOOLEAN
STDCALL
WDFEXPORT(WdfInterruptTryToAcquireLock)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    _Requires_lock_not_held_(_Curr_)
    _When_(return!=0, _Acquires_lock_(_Curr_))
    WDFINTERRUPT Interrupt
    )

/*++

Routine Description:

    Try to acquire the interrupt's passive-lock.

Arguments:

    Interrupt - Handle to WDFINTERUPT object created with WdfInterruptCreate.

Returns:

    TRUE:  function was able to successfully acquire the lock.

    FALSE: function was unable to acquire the lock.

--*/

{
    DDI_ENTRY();

    FxInterrupt*    pFxInterrupt;
    NTSTATUS        status;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         Interrupt,
                         FX_TYPE_INTERRUPT,
                         (PVOID*)&pFxInterrupt);

    if (pFxInterrupt->GetDriverGlobals()->FxVerifierOn) {
        if (pFxInterrupt->IsPassiveHandling() == FALSE) {
            DoTraceLevelMessage(
                pFxInterrupt->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                "WDFINTERRUPT %p is handled at DIRQL. This API is not "
                "available for DIQRL interrupt handling.",
                Interrupt);
            FxVerifierDbgBreakPoint(pFxInterrupt->GetDriverGlobals());
            return FALSE;
        }

        status = FxVerifierCheckIrqlLevel(pFxInterrupt->GetDriverGlobals(),
                                          PASSIVE_LEVEL);
        if (!NT_SUCCESS(status)) {
            return FALSE;
        }
    }

    return pFxInterrupt->TryToAcquireLock();
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
STDCALL
WDFEXPORT(WdfInterruptReportActive)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFINTERRUPT Interrupt
    )

/*++

Routine Description:

    This DDI informs the system that the interrupt is active and expecting
    interrupt requests on the associated lines. This is typically called after
    having reported the lines as inactive via WdfInterruptReportInactive.
    The DDI doesn't do anything if this DDI is called before
    WdfInterruptReportInactive while the interrupt is connected.

Arguments:

    Interrupt - Handle to WDFINTERUPT object created with WdfInterruptCreate.

Returns:

    VOID

--*/
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS  pFxDriverGlobals = NULL;
    FxInterrupt*        pFxInterrupt     = NULL;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Interrupt,
                                   FX_TYPE_INTERRUPT,
                                   (PVOID*)&pFxInterrupt,
                                   &pFxDriverGlobals);

    pFxInterrupt->ReportActive();

    return;
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
STDCALL
WDFEXPORT(WdfInterruptReportInactive)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFINTERRUPT Interrupt
    )

/*++

Routine Description:

    This DDI informs the system that the the interrupt is no longer active
    and is not expecting interrupt requests on the associated lines.

Arguments:

    Interrupt - Handle to WDFINTERUPT object created with WdfInterruptCreate.

Returns:

    VOID

--*/
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS  pFxDriverGlobals = NULL;
    FxInterrupt*        pFxInterrupt     = NULL;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Interrupt,
                                   FX_TYPE_INTERRUPT,
                                   (PVOID*)&pFxInterrupt,
                                   &pFxDriverGlobals);

    pFxInterrupt->ReportInactive();

    return;
}

} // extern "C"
