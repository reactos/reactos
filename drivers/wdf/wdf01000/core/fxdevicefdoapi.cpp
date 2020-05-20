/*
 * PROJECT:     ReactOS Wdf01000 driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     FDO api functions
 * COPYRIGHT:   Copyright 2020 mrmks04 (mrmks04@yandex.ru)
 */


#include "wdf.h"


extern "C" {

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfFdoQueryForInterface)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Fdo,
    __in
    LPCGUID InterfaceType,
    __out
    PINTERFACE Interface,
    __in
    USHORT Size,
    __in
    USHORT Version,
    __in_opt
    PVOID InterfaceSpecificData
    )
/*++

Routine Description:
    Sends a query interface pnp request to the top of the stack (which means that
    the request will travel through this device before going down the stack).

Arguments:
    Fdo - the device stack which is being queried

    InterfaceType - interface type specifier

    Interface - Interface block which will be filled in by the component which
                responds to the query interface

    Size - size in bytes of Interfce

    Version - version of InterfaceType being requested

    InterfaceSpecificData - Additional data associated with Interface

Return Value:
    NTSTATUS

  --*/
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

__drv_maxIRQL(DISPATCH_LEVEL)
WDFCHILDLIST
WDFEXPORT(WdfFdoGetDefaultChildList)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Fdo
    )
/*++

Routine Description:
    Returns the default dynamic child list associated with the FDO.   For a
    valid handle to be returned, the driver must have configured the default
    list in its device init phase.

Arguments:
    Fdo - the FDO being asked to return the handle

Return Value:
    a valid handle value or NULL

  --*/
{
    WDFNOTIMPLEMENTED();
    return NULL;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfFdoAddStaticChild)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Fdo,
    __in
    WDFDEVICE Child
    )
/*++

Routine Description:
    Adds a statically enumerated child to the static device list.

Arguments:
    Fdo - the parent of the child

    Child - the child to add

Return Value:
    NTSTATUS

  --*/

{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfFdoLockStaticChildListForIteration)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Fdo
    )
/*++

Routine Description:
    Locks the static child list for iteration.  Any adds or removes that occur
    while the list is locked are pended until it is unlocked.  Locking can
    be nested.  When the unlock count is zero, the changes are evaluated.

Arguments:
    Fdo - the parent who owns the list of static children

Return Value:
    None

  --*/

{
    WDFNOTIMPLEMENTED();
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
WDFDEVICE
WDFEXPORT(WdfFdoRetrieveNextStaticChild)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Fdo,
    __in
    WDFDEVICE PreviousChild,
    __in
    ULONG Flags
    )
/*++

Routine Description:
    Returns the next static child in the list based on the PreviousChild and
    Flags values.

Arguments:
    Fdo - the parent which owns the static child list

    PreviousChild - The child returned on the last call to this DDI.  If NULL,
        returns the first static child specified by the Flags criteria

    Flags - combination of values from WDF_RETRIEVE_CHILD_FLAGS.
        WdfRetrievePresentChildren - reports children who have been reported as
                                     present to pnp
        WdfRetrieveMissingChildren = reports children who have been reported as
                                     missing to pnp
        WdfRetrievePendingChildren = reports children who have been added to the
                                     list, but not yet reported to pnp

Return Value:
    next WDFDEVICE handle representing a static child or NULL

  --*/
{
    WDFNOTIMPLEMENTED();
    return NULL;
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfFdoUnlockStaticChildListFromIteration)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Fdo
    )
/*++

Routine Description:
    Unlocks the static child list from iteration.  Upon the last unlock, any
    pended modifications to the list will be applied.

Arguments:
    Fdo - the parent who owns the static child list

Return Value:
    None

  --*/
{
    WDFNOTIMPLEMENTED();
}

} // extern "C"
