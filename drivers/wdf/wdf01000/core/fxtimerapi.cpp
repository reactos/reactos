#include "wdf.h"

extern "C" {

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfTimerCreate)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDF_TIMER_CONFIG Config,
    __in
    PWDF_OBJECT_ATTRIBUTES Attributes,
    __out
    WDFTIMER * Timer
    )

/*++

Routine Description:

    Create a TIMER object that will call the supplied function
    when it fires. It returns a handle to the WDFTIMER object.

Arguments:

    Config     - WDF_TIMER_CONFIG structure.

    Attributes - WDF_OBJECT_ATTRIBUTES to set the parent object, to request
                 a context memory allocation and a DestroyCallback.

    Timer - Pointer to location to return the resulting WDFTIMER handle.

Returns:

    STATUS_SUCCESS - A WDFTIMER handle has been created.

Notes:

    The WDFTIMER object is deleted either when the DEVICE or QUEUE it is
    associated with is deleted, or WdfObjectDelete is called.

--*/

{
    WDFNOTIMPLEMENTED();
    return STATUS_UNSUCCESSFUL;
}

__drv_maxIRQL(DISPATCH_LEVEL)
BOOLEAN
WDFEXPORT(WdfTimerStart)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFTIMER Timer,
    __in
    LONGLONG DueTime
    )

/*++

Routine Description:

    Enqueue the TIMER to run at the specified time.

Arguments:

    WDFTIMER - Handle to WDFTIMER object created with WdfTimerCreate.

    DueTime - Time to execute

Returns:

    TRUE if the timer object was in the system's timer queue

--*/

{
    WDFNOTIMPLEMENTED();
    return FALSE;
}

__drv_when(Wait == __true, __drv_maxIRQL(PASSIVE_LEVEL))
__drv_when(Wait == __false, __drv_maxIRQL(DISPATCH_LEVEL))
BOOLEAN
WDFEXPORT(WdfTimerStop)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFTIMER Timer,
    __in
    BOOLEAN  Wait
    )

/*++

Routine Description:

    Stop the TIMER

Arguments:

    WDFTIMER - Handle to WDFTIMER object created with WdfTimerCreate.

Returns:

    TRUE if the timer object was in the system's timer queue

--*/

{
    WDFNOTIMPLEMENTED();
    return FALSE;
}

__drv_maxIRQL(DISPATCH_LEVEL)
WDFOBJECT
WDFEXPORT(WdfTimerGetParentObject)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFTIMER Timer
    )

/*++

Routine Description:

    Return the Parent Object handle supplied to WdfTimerCreate

Arguments:

    WDFTIMER - Handle to WDFTIMER object created with WdfTimerCreate.

Returns:

    Handle to the framework object that is the specified timer object's 
    parent object

--*/

{
    WDFNOTIMPLEMENTED();
    return NULL;
}

} // extern "C"