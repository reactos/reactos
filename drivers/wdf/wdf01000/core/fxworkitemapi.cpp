/*
 * PROJECT:     ReactOS Wdf01000 driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     WDFWORKITEM api functions
 * COPYRIGHT:   Copyright 2020 mrmks04 (mrmks04@yandex.ru)
 */


#include "wdf.h"


extern "C" {

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfWorkItemCreate)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDF_WORKITEM_CONFIG Config,
    __in
    PWDF_OBJECT_ATTRIBUTES Attributes,
    __out
    WDFWORKITEM* WorkItem
    )

/*++

Routine Description:

    Create a WorkItem object that will call the supplied function with
    context when it fires. It returns a handle to the WDFWORKITEM object.

Arguments:

    Config - Pointer to WDF_WORKITEM_CONFIG structure

    Attributes - WDF_OBJECT_ATTRIBUTES to set the parent object and to request
                 a context memory allocation,  and a DestroyCallback.

    WorkItem - Pointer to the created WDFWORKITEM handle.

Returns:

    STATUS_SUCCESS - A WDFWORKITEM handle has been created.

    The WDFWORKITEM will be automatically deleted when the object it is
    associated with is deleted.

Notes:

    The WDFWORKITEM object is deleted either when the DEVICE or QUEUE it is
    associated with is deleted, or WdfObjectDelete is called.

    If the WDFWORKITEM is used to access WDM objects, a Cleanup callback should
    be registered to allow references to be released.

--*/

{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfWorkItemEnqueue)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFWORKITEM WorkItem
    )
/*++

Routine Description:

    Enqueue a WorkItem to execute.

Arguments:

    WorkItem - Handle to WDFWORKITEM

Returns:

    None

--*/
{
    WDFNOTIMPLEMENTED();
}

__drv_maxIRQL(DISPATCH_LEVEL)
WDFOBJECT
WDFEXPORT(WdfWorkItemGetParentObject)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFWORKITEM WorkItem
    )

/*++

Routine Description:

    Return the Object handle supplied to WdfWorkItemCreate

Arguments:

    WDFWORKITEM - Handle to WDFWORKITEM object created with WdfWorkItemCreate.

Returns:

    Handle to the framework object that is the specified work-item object's 
    parent object.
    
--*/

{
    WDFNOTIMPLEMENTED();
    return NULL;
}

__drv_maxIRQL(PASSIVE_LEVEL)
VOID
WDFEXPORT(WdfWorkItemFlush)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFWORKITEM WorkItem
    )
/*++

Routine Description:

    Wait until any outstanding workitems have completed

Arguments:

    WorkItem - Handle to WDFWORKITEM

Returns:

    None

--*/
{
    WDFNOTIMPLEMENTED();
}

} // extern "C"
