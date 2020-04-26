/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

_WdfVersionBuild_

Module Name:

    wdfsync.h

Abstract:

    This module contains contains the Windows Driver Framework synchronization
    DDIs.

Environment:

    kernel mode only

Revision History:


--*/

#ifndef _WDFSYNC_H_
#define _WDFSYNC_H_



#if (NTDDI_VERSION >= NTDDI_WIN2K)



//
// WDF Function: WdfObjectAcquireLock
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
VOID
(*PFN_WDFOBJECTACQUIRELOCK)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    __drv_neverHold(WdfObjectLock)
    __drv_acquiresResource(WdfObjectLock)
    WDFOBJECT Object
    );

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
FORCEINLINE
WdfObjectAcquireLock(
    __in
    __drv_neverHold(WdfObjectLock)
    __drv_acquiresResource(WdfObjectLock)
    WDFOBJECT Object
    )
{
    ((PFN_WDFOBJECTACQUIRELOCK) WdfFunctions[WdfObjectAcquireLockTableIndex])(WdfDriverGlobals, Object);
}

//
// WDF Function: WdfObjectReleaseLock
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
VOID
(*PFN_WDFOBJECTRELEASELOCK)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    __drv_mustHold(WdfObjectLock)
    __drv_releasesResource(WdfObjectLock)
    WDFOBJECT Object
    );

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
FORCEINLINE
WdfObjectReleaseLock(
    __in
    __drv_mustHold(WdfObjectLock)
    __drv_releasesResource(WdfObjectLock)
    WDFOBJECT Object
    )
{
    ((PFN_WDFOBJECTRELEASELOCK) WdfFunctions[WdfObjectReleaseLockTableIndex])(WdfDriverGlobals, Object);
}

//
// WDF Function: WdfWaitLockCreate
//
typedef
__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFWAITLOCKCREATE)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES LockAttributes,
    __out
    WDFWAITLOCK* Lock
    );

__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
FORCEINLINE
WdfWaitLockCreate(
    __in_opt
    PWDF_OBJECT_ATTRIBUTES LockAttributes,
    __out
    WDFWAITLOCK* Lock
    )
{
    return ((PFN_WDFWAITLOCKCREATE) WdfFunctions[WdfWaitLockCreateTableIndex])(WdfDriverGlobals, LockAttributes, Lock);
}

//
// WDF Function: WdfWaitLockAcquire
//
typedef
__drv_when(Timeout != 0, __checkReturn)
__drv_when(Timeout == 0, __drv_maxIRQL(PASSIVE_LEVEL))
__drv_when(Timeout != 0 && *Timeout == 0, __drv_maxIRQL(DISPATCH_LEVEL))
__drv_when(Timeout != 0 && *Timeout != 0, __drv_maxIRQL(PASSIVE_LEVEL))
WDFAPI
NTSTATUS
(*PFN_WDFWAITLOCKACQUIRE)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFWAITLOCK Lock,
    __in_opt
    PLONGLONG Timeout
    );

__drv_when(Timeout != 0, __checkReturn)
__drv_when(Timeout == 0, __drv_maxIRQL(PASSIVE_LEVEL))
__drv_when(Timeout != 0 && *Timeout == 0, __drv_maxIRQL(DISPATCH_LEVEL))
__drv_when(Timeout != 0 && *Timeout != 0, __drv_maxIRQL(PASSIVE_LEVEL))
NTSTATUS
FORCEINLINE
WdfWaitLockAcquire(
    __in
    WDFWAITLOCK Lock,
    __in_opt
    PLONGLONG Timeout
    )
{
    return ((PFN_WDFWAITLOCKACQUIRE) WdfFunctions[WdfWaitLockAcquireTableIndex])(WdfDriverGlobals, Lock, Timeout);
}

//
// WDF Function: WdfWaitLockRelease
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
VOID
(*PFN_WDFWAITLOCKRELEASE)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFWAITLOCK Lock
    );

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
FORCEINLINE
WdfWaitLockRelease(
    __in
    WDFWAITLOCK Lock
    )
{
    ((PFN_WDFWAITLOCKRELEASE) WdfFunctions[WdfWaitLockReleaseTableIndex])(WdfDriverGlobals, Lock);
}

//
// WDF Function: WdfSpinLockCreate
//
typedef
__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFSPINLOCKCREATE)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES SpinLockAttributes,
    __out
    WDFSPINLOCK* SpinLock
    );

__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
FORCEINLINE
WdfSpinLockCreate(
    __in_opt
    PWDF_OBJECT_ATTRIBUTES SpinLockAttributes,
    __out
    WDFSPINLOCK* SpinLock
    )
{
    return ((PFN_WDFSPINLOCKCREATE) WdfFunctions[WdfSpinLockCreateTableIndex])(WdfDriverGlobals, SpinLockAttributes, SpinLock);
}

//
// WDF Function: WdfSpinLockAcquire
//
typedef
__drv_raisesIRQL(DISPATCH_LEVEL)
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
VOID
(*PFN_WDFSPINLOCKACQUIRE)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    __drv_savesIRQL
    __drv_neverHold(SpinLockObj)
    __drv_acquiresResource(SpinLockObj)
    WDFSPINLOCK SpinLock
    );

__drv_raisesIRQL(DISPATCH_LEVEL)
__drv_maxIRQL(DISPATCH_LEVEL)
VOID
FORCEINLINE
WdfSpinLockAcquire(
    __in
    __drv_savesIRQL
    __drv_neverHold(SpinLockObj)
    __drv_acquiresResource(SpinLockObj)
    WDFSPINLOCK SpinLock
    )
{
    ((PFN_WDFSPINLOCKACQUIRE) WdfFunctions[WdfSpinLockAcquireTableIndex])(WdfDriverGlobals, SpinLock);
}

//
// WDF Function: WdfSpinLockRelease
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
__drv_minIRQL(DISPATCH_LEVEL)
WDFAPI
VOID
(*PFN_WDFSPINLOCKRELEASE)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    __drv_restoresIRQL
    __drv_mustHold(SpinLockObj)
    __drv_releasesResource(SpinLockObj)
    WDFSPINLOCK SpinLock
    );

__drv_maxIRQL(DISPATCH_LEVEL)
__drv_minIRQL(DISPATCH_LEVEL)
VOID
FORCEINLINE
WdfSpinLockRelease(
    __in
    __drv_restoresIRQL
    __drv_mustHold(SpinLockObj)
    __drv_releasesResource(SpinLockObj)
    WDFSPINLOCK SpinLock
    )
{
    ((PFN_WDFSPINLOCKRELEASE) WdfFunctions[WdfSpinLockReleaseTableIndex])(WdfDriverGlobals, SpinLock);
}



#endif // (NTDDI_VERSION >= NTDDI_WIN2K)


#endif // _WDFSYNC_H_

