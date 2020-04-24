#include "wdf.h"



extern "C" {


_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFAPI
WDFEXPORT(WdfWaitLockCreate)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
     __in_opt
    PWDF_OBJECT_ATTRIBUTES LockAttributes,
    __out
    WDFWAITLOCK* Lock
    )
/*++

Routine Description:
    Creates a lock object which can be acquired at PASSIVE_LEVEL and will return
    to the caller at PASSIVE_LEVEL once acquired.

Arguments:
    LockAttributes - generic attributes to be associated with the created lock

    Lock - pointer to receive the newly created lock

Return Value:
    NTSTATUS

  --*/
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

__drv_when(Timeout != 0, _Must_inspect_result_)
__drv_when(Timeout == 0, __drv_maxIRQL(PASSIVE_LEVEL))
__drv_when(Timeout != 0 && *Timeout == 0, __drv_maxIRQL(DISPATCH_LEVEL))
__drv_when(Timeout != 0 && *Timeout != 0, __drv_maxIRQL(PASSIVE_LEVEL))
NTSTATUS
WDFAPI
WDFEXPORT(WdfWaitLockAcquire)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFWAITLOCK Lock,
    __in_opt
    PLONGLONG Timeout
    )
/*++

Routine Description:
    Attempts to acquire the lock object.  If a non NULL timeout is provided, the
    attempt to acquire the lock may fail if it cannot be acquired in the
    specified time.

Arguments:
    Lock - the lock to acquire

    Timeout - optional timeout in acquiring the lock.  If calling at an IRQL >=
              DISPATCH_LEVEL, then this parameter is not NULL (and should more
              then likely be zero)

Return Value:
    STATUS_TIMEOUT if a timeout was provided and the lock could not be acquired
    in the specified time, otherwise STATUS_SUCCESS

  --*/
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFAPI
WDFEXPORT(WdfWaitLockRelease)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFWAITLOCK Lock
    )
/*++

Routine Description:
    Releases a previously acquired wait lock

Arguments:
    Lock - the lock to release

  --*/
{
    WDFNOTIMPLEMENTED();
}

} // extern "C"
