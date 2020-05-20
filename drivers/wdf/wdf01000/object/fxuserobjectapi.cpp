/*
 * PROJECT:     ReactOS Wdf01000 driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     User object api function
 * COPYRIGHT:   Copyright 2020 mrmks04 (mrmks04@yandex.ru)
 */


#include "wdf.h"



extern "C" {

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfObjectCreate)(
   __in
   PWDF_DRIVER_GLOBALS DriverGlobals,
   __in_opt
   PWDF_OBJECT_ATTRIBUTES Attributes,
   __out
   WDFOBJECT* Object
   )

/*++

Routine Description:

    This creates a general WDF object for use by the device driver.

    It participates in general framework object contracts in that it:

    - Has a handle and a reference count
    - Has Cleanup and Destroy callbacks
    - Supports driver context memory and type
    - Can have child objects
    - Can optionally have a parent object and automatically delete with it

    It is intended to allow a WDF device driver to use this object to
    create its own structures that can participate in frameworks lifetime
    management.

    The device driver can use the objects context memory and type to
    represent its own internal data structures, and can further assign
    device driver specific resources and release them by registering
    for EvtObjectCleanup, and EvtObjectDestroy callbacks.

    The object may be deleted by using the WdfObjectDelete API.

    Since the object is represented by a frameworks handle, it can be
    reference counted, and validated.

    Class drivers may use this object to define framework object handles
    for their types.

Arguments:

    Attributes - WDF_OBJECT_ATTRIBUTES to define a parent object, context memory,
                 Cleanup and Destroy handlers.

Return Value:

    NTSTATUS

--*/

{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

} // extern "C"
