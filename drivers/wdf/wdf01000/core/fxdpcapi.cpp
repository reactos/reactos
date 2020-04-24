#include "wdf.h"



extern "C" {

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfDpcCreate)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDF_DPC_CONFIG Config,
    __in
    PWDF_OBJECT_ATTRIBUTES Attributes,
    __out
    WDFDPC * Dpc
    )

/*++

Routine Description:

    Create a DPC object that will call the supplied function with
    context when it fires. It returns a handle to the WDFDPC object.

Arguments:

    Config     - WDF_DPC_CONFIG structure.

    Attributes - WDF_OBJECT_ATTRIBUTES to set the parent object and to request
                 a context memory allocation,  and a DestroyCallback.

    Dpc - Pointer to location to returnt he resulting WDFDPC handle.

Returns:

    STATUS_SUCCESS - A WDFDPC handle has been created.

Notes:

    The WDFDPC object is deleted either when the DEVICE or QUEUE it is
    associated as its parent with is deleted, or WdfObjectDelete is called.

    If the DPC is used to access WDM objects, a Cleanup callback should
    be registered to allow references to be released.

--*/

{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

__drv_maxIRQL(HIGH_LEVEL)
KDPC*
WDFEXPORT(WdfDpcWdmGetDpc)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDPC Dpc
    )
/*++

Routine Description:

    Return the KDPC* object pointer so that it may be linked into
    a DPC list.

Arguments:

    WDFDPC - Handle to WDFDPC object created with WdfDpcCreate.

Returns:

    KDPC*

--*/

{
    WDFNOTIMPLEMENTED();
    return NULL;
}

__drv_maxIRQL(HIGH_LEVEL)
BOOLEAN
WDFEXPORT(WdfDpcEnqueue)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDPC Dpc
    )

/*++

Routine Description:

    Enqueue the DPC to run at a system determined time

Arguments:

    WDFDPC - Handle to WDFDPC object created with WdfDpcCreate.

Returns:

--*/

{
    WDFNOTIMPLEMENTED();
    return FALSE;
}

__drv_when(Wait == __true, __drv_maxIRQL(PASSIVE_LEVEL))
__drv_when(Wait == __false, __drv_maxIRQL(HIGH_LEVEL))
BOOLEAN
WDFEXPORT(WdfDpcCancel)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDPC Dpc,
    __in
    BOOLEAN Wait
    )

/*++

Routine Description:

    Attempt to cancel the DPC and returns status

Arguments:

    WDFDPC - Handle to WDFDPC object created with WdfDpcCreate.

Returns:

    TRUE  - DPC was cancelled, and was not run

    FALSE - DPC was not cancelled, has run, is running, or will run

--*/

{
    WDFNOTIMPLEMENTED();
    return FALSE;
}

__drv_maxIRQL(HIGH_LEVEL)
WDFOBJECT
WDFEXPORT(WdfDpcGetParentObject)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDPC Dpc
    )

/*++

Routine Description:

    Return the parent parent object supplied to WdfDpcCreate.

Arguments:

    WDFDPC - Handle to WDFDPC object created with WdfDpcCreate.

Returns:

--*/

{
    WDFNOTIMPLEMENTED();
    return NULL;
}

} // extern "C"
