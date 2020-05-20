/*
 * PROJECT:     ReactOS Wdf01000 driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Interrupt api functions
 * COPYRIGHT:   Copyright 2020 mrmks04 (mrmks04@yandex.ru)
 */



#include "wdf.h"



extern "C" {

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
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
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

BOOLEAN
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
    WDFNOTIMPLEMENTED();
    return FALSE;
}

__drv_maxIRQL(DISPATCH_LEVEL)
BOOLEAN
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
    WDFNOTIMPLEMENTED();
    return FALSE;
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
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
    WDFNOTIMPLEMENTED();
}

__drv_maxIRQL(DISPATCH_LEVEL + 1)
VOID
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
    WDFNOTIMPLEMENTED();
}

__drv_maxIRQL(PASSIVE_LEVEL)
VOID
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
    WDFNOTIMPLEMENTED();
}

__drv_maxIRQL(PASSIVE_LEVEL)
VOID
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
    WDFNOTIMPLEMENTED();
}

_Must_inspect_result_
struct _KINTERRUPT*
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
    WDFNOTIMPLEMENTED();
    return NULL;
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
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
    WDFNOTIMPLEMENTED();
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
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
    WDFNOTIMPLEMENTED();
}

WDFDEVICE
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
    WDFNOTIMPLEMENTED();
    return NULL;
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
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
    WDFNOTIMPLEMENTED();
}

} // extern "C"
