/*
 * PROJECT:     ReactOS Wdf01000 driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Framework IO target api functions
 * COPYRIGHT:   Copyright 2020 mrmks04 (mrmks04@yandex.ru)
 */


#include "wdf.h"



extern "C" {

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfIoTargetCreate)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES IoTargetAttributes,
    __out
    WDFIOTARGET* IoTarget
    )
/*++

Routine Description:
    Creates a WDFIOTARGET which can be opened upon success.

Arguments:
    Device - the device which will own the target.  The target will be parented
             by the owning device

    IoTargetAttributes - optional attributes to apply to the target

    IoTarget - pointer which will receive the created target handle

Return Value:
    NTSTATUS

  --*/
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfIoTargetOpen)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIOTARGET IoTarget,
    __in
    PWDF_IO_TARGET_OPEN_PARAMS OpenParams
    )
/*++

Routine Description:
    Opens a target.  The target must be in the closed state for an open to
    succeed.  Open is either wrapping an existing PDEVICE_OBJECT + PFILE_OBJECT
    that the client provides or opening a PDEVICE_OBJECT by name.

Arguments:
    IoTarget - Target to be opened

    OpenParams - structure which describes how to open the target

Return Value:
    NTSTATUS

  --*/
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

__drv_maxIRQL(PASSIVE_LEVEL)
VOID
WDFEXPORT(WdfIoTargetCloseForQueryRemove)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIOTARGET IoTarget
    )
/*++

Routine Description:
    Closes a target in response to a query remove notification callback.  This
    will pend all i/o sent after the call returns.

Arguments:
    IoTarget - Target to be closed

Return Value:
    None

  --*/
{
    WDFNOTIMPLEMENTED();
}

__drv_maxIRQL(PASSIVE_LEVEL)
VOID
WDFEXPORT(WdfIoTargetClose)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIOTARGET IoTarget
    )
/*++

Routine Description:
    Closes the target for good.  The target can be in either a query removed or
    opened state.

Arguments:
    IoTarget - target to close

Return Value:
    None

  --*/
{
    WDFNOTIMPLEMENTED();
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfIoTargetStart)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIOTARGET IoTarget
    )
/*++

Routine Description:
    Changes the target's state to started.  In the started state, the target
    can send I/O.

Arguments:
    IoTarget - the target whose state will change

Return Value:
    NTSTATUS

  --*/
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

__drv_when(Action == 3, __drv_maxIRQL(DISPATCH_LEVEL))
__drv_when(Action == 0 || Action == 1 || Action == 2, __drv_maxIRQL(PASSIVE_LEVEL))
VOID
WDFEXPORT(WdfIoTargetStop)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIOTARGET IoTarget,
    __in
    __drv_strictTypeMatch(__drv_typeConst)
    WDF_IO_TARGET_SENT_IO_ACTION Action
    )
/*++

Routine Description:
    This function puts the target into the stopped state.  Depending on the value
    of Action, this function may not return until sent I/O has been canceled and/or
    completed.

Arguments:
    IoTarget - the target whose state is being changed

    Action - what to do with the I/O that is pending in the target already

Return Value:
    None

  --*/
{
    WDFNOTIMPLEMENTED();
}

__drv_maxIRQL(DISPATCH_LEVEL)
WDF_IO_TARGET_STATE
WDFEXPORT(WdfIoTargetGetState)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIOTARGET IoTarget
    )
/*++

Routine Description:
    Returns the current state of the target

Arguments:
    IoTarget - target whose state is being returned

Return Value:
    current target state

  --*/
{
    WDFNOTIMPLEMENTED();
    return WDF_IO_TARGET_STATE::WdfIoTargetStateUndefined;
}

__drv_maxIRQL(DISPATCH_LEVEL)
WDFDEVICE
WDFEXPORT(WdfIoTargetGetDevice)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIOTARGET IoTarget
    )
/*++

Routine Description:
    Returns the owning WDFDEVICE for the WDFIOTARGET, this is not necessarily
    the PDEVICE_OBJECT of the target itself.

Arguments:
    IoTarget - the target being retrieved

Return Value:
    a valid WDFDEVICE handle , NULL if there are any problems

  --*/

{
    WDFNOTIMPLEMENTED();
    return NULL;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfIoTargetQueryTargetProperty)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIOTARGET IoTarget,
    __in
    DEVICE_REGISTRY_PROPERTY  DeviceProperty,
    __in
    ULONG BufferLength,
    __drv_when(BufferLength != 0, __out_bcount_part_opt(BufferLength, *ResultLength))
    __drv_when(BufferLength == 0, __out_opt)       
    PVOID PropertyBuffer,  
   __deref_out_range(<=,BufferLength)    
    PULONG ResultLength
    )
/*++

Routine Description:
    Retrieves the requested device property for the given target

Arguments:
    IoTarget - the target whose PDO whose will be queried

    DeviceProperty - the property being queried

    BufferLength - length of PropertyBuffer in bytes

    PropertyBuffer - Buffer which will receive the property being queried

    ResultLength - if STATUS_BUFFER_TOO_SMALL is returned, then this will contain
                   the required length

Return Value:
    NTSTATUS

  --*/
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfIoTargetAllocAndQueryTargetProperty)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIOTARGET IoTarget,
    __in
    DEVICE_REGISTRY_PROPERTY DeviceProperty,
    __in
    __drv_strictTypeMatch(1)
    POOL_TYPE PoolType,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES PropertyMemoryAttributes,
    __out
    WDFMEMORY*  PropertyMemory
    )
/*++

Routine Description:
    Allocates and retrieves the requested device property for the given target

Arguments:
    IoTarget - the target whose PDO whose will be queried

    DeviceProperty - the property being queried

    PoolType - what type of pool to allocate

    PropertyMemoryAttributes - attributes to associate with PropertyMemory

    PropertyMemory - handle which will receive the property buffer

Return Value:
    NTSTATUS

  --*/
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

__drv_maxIRQL(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS
WDFEXPORT(WdfIoTargetQueryForInterface)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIOTARGET IoTarget,
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
    Sends a query interface pnp request to the top of the target's stack.

Arguments:
    IoTarget - the target which is being queried

    InterfaceType - interface type specifier

    Interface - Interface block which will be filled in by the component which
                responds to the query interface

    Size - size in bytes of Interface

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
PDEVICE_OBJECT
WDFEXPORT(WdfIoTargetWdmGetTargetDeviceObject)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIOTARGET IoTarget
    )
/*++

Routine Description:
    Returns the PDEVICE_OBJECT.  This is the device which PIRPs are sent to.
    This is not necessarily the PDEVICE_OBJECT that WDFDEVICE is attached to.

Arguments:
    IoTarget - target whose WDM device object is being returned

Return Value:
    valid PDEVICE_OBJECT or NULL on failure

  --*/
{
    WDFNOTIMPLEMENTED();
    return NULL;
}

__drv_maxIRQL(DISPATCH_LEVEL)
PDEVICE_OBJECT
WDFEXPORT(WdfIoTargetWdmGetTargetPhysicalDevice)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIOTARGET IoTarget
    )
/*++

Routine Description:
    Returns the PDO for the target itself.  This is not necessarily the same
    PDO as the WDFDEVICE that owns the target.  Not all targets have a PDO since
    you can open a legacy non pnp PDEVICE_OBJECT which does not have one.

Arguments:
    IoTarget - target whose PDO is being returned

Return Value:
    A valid PDEVICE_OBJECT or NULL upon success

  --*/
{
    WDFNOTIMPLEMENTED();
    return NULL;
}

__drv_maxIRQL(DISPATCH_LEVEL)
PFILE_OBJECT
WDFEXPORT(WdfIoTargetWdmGetTargetFileObject)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIOTARGET IoTarget
    )
/*++

Routine Description:
    Returns the PFILE_OBJECT associated with the target.  Not all targets have
    an underlying file object so NULL is a valid and successful return value.

Arguments:
    IoTarget - the target whose fileobject is being returned

Return Value:
    a valid PFILE_OBJECT or NULL upon success

  --*/
{
    WDFNOTIMPLEMENTED();
    return NULL;
}

__drv_maxIRQL(DISPATCH_LEVEL)
HANDLE
WDFEXPORT(WdfIoTargetWdmGetTargetFileHandle)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIOTARGET IoTarget
    )
/*++

Routine Description:
    Returns the file handle that the target represents. For KMDF, the handle is a kernel
    handle, so it is not tied to any process context. For UMDF it is a Win32 handle opened
    in the host process context. Not all targets have a file handle associated with them,
    so NULL is a valid return value that does not indicate error.

Arguments:
    IoTarget - target whose file handle is being returned

Return Value:
    A valid kernel/win32 handle or NULL

  --*/
{
    WDFNOTIMPLEMENTED();
    return NULL;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfIoTargetSendReadSynchronously)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIOTARGET IoTarget,
    __in_opt
    WDFREQUEST Request,
    __in_opt
    PWDF_MEMORY_DESCRIPTOR OutputBuffer,
    __in_opt
    PLONGLONG DeviceOffset,
    __in_opt
    PWDF_REQUEST_SEND_OPTIONS RequestOptions,
    __out_opt
    PULONG_PTR BytesRead
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfIoTargetFormatRequestForRead)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIOTARGET IoTarget,
    __in
    WDFREQUEST Request,
    __in_opt
    WDFMEMORY OutputBuffer,
    __in_opt
    PWDFMEMORY_OFFSET OutputBufferOffsets,
    __in_opt
    PLONGLONG DeviceOffset
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfIoTargetSendWriteSynchronously)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIOTARGET IoTarget,
    __in_opt
    WDFREQUEST Request,
    __in_opt
    PWDF_MEMORY_DESCRIPTOR InputBuffer,
    __in_opt
    PLONGLONG DeviceOffset,
    __in_opt
    PWDF_REQUEST_SEND_OPTIONS RequestOptions,
    __out_opt
    PULONG_PTR BytesWritten
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfIoTargetFormatRequestForWrite)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIOTARGET IoTarget,
    __in
    WDFREQUEST Request,
    __in_opt
    WDFMEMORY InputBuffer,
    __in_opt
    PWDFMEMORY_OFFSET InputBufferOffsets,
    __in_opt
    PLONGLONG DeviceOffset
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfIoTargetSendIoctlSynchronously)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIOTARGET IoTarget,
    __in_opt
    WDFREQUEST Request,
    __in
    ULONG Ioctl,
    __in_opt
    PWDF_MEMORY_DESCRIPTOR InputBuffer,
    __in_opt
    PWDF_MEMORY_DESCRIPTOR OutputBuffer,
    __in_opt
    PWDF_REQUEST_SEND_OPTIONS RequestOptions,
    __out_opt
    PULONG_PTR BytesReturned
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfIoTargetFormatRequestForIoctl)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIOTARGET IoTarget,
    __in
    WDFREQUEST Request,
    __in
    ULONG Ioctl,
    __in_opt
    WDFMEMORY InputBuffer,
    __in_opt
    PWDFMEMORY_OFFSET InputBufferOffsets,
    __in_opt
    WDFMEMORY OutputBuffer,
    __in_opt
    PWDFMEMORY_OFFSET OutputBufferOffsets
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfIoTargetSendInternalIoctlSynchronously)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIOTARGET IoTarget,
    __in_opt
    WDFREQUEST Request,
    __in
    ULONG Ioctl,
    __in_opt
    PWDF_MEMORY_DESCRIPTOR InputBuffer,
    __in_opt
    PWDF_MEMORY_DESCRIPTOR OutputBuffer,
    __in_opt
    PWDF_REQUEST_SEND_OPTIONS RequestOptions,
    __out_opt
    PULONG_PTR BytesReturned
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfIoTargetFormatRequestForInternalIoctl)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIOTARGET IoTarget,
    __in
    WDFREQUEST Request,
    __in
    ULONG Ioctl,
    __in_opt
    WDFMEMORY InputBuffer,
    __in_opt
    PWDFMEMORY_OFFSET InputBufferOffsets,
    __in_opt
    WDFMEMORY OutputBuffer,
    __in_opt
    PWDFMEMORY_OFFSET OutputBufferOffsets
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfIoTargetSendInternalIoctlOthersSynchronously)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIOTARGET IoTarget,
    __in_opt
    WDFREQUEST Request,
    __in
    ULONG Ioctl,
    __in_opt
    PWDF_MEMORY_DESCRIPTOR OtherArg1,
    __in_opt
    PWDF_MEMORY_DESCRIPTOR OtherArg2,
    __in_opt
    PWDF_MEMORY_DESCRIPTOR OtherArg4,
    __in_opt
    PWDF_REQUEST_SEND_OPTIONS RequestOptions,
    __out_opt
    PULONG_PTR BytesReturned
    )
/*++

Routine Description:
    Sends an internal IOCTL to the target synchronously.  Since all 3 buffers can
    be used, we cannot overload WdfIoTargetSendInternalIoctlSynchronously since
    it can only take 2 buffers.

Arguments:
    IoTarget - the target to which the request will be sent

    Request - optional.  If specified, the request's PIRP will be used to send
              the i/o to the target.

    Ioctl - internal ioctl value to send

    OtherArg1
    OtherArg2
    OtherArg4 - arguments to use in the stack locations's Others field.  There
                is no OtherArg3 because 3 is where the IOCTL value is written.
                All buffers are optional.

    RequestOptions - optional.  If specified, the timeout indicated will be used
                     if the request exceeds the timeout.

    BytesReturned - the number of bytes returned by the target

Return Value:
    NTSTATUS

  --*/
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfIoTargetFormatRequestForInternalIoctlOthers)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIOTARGET IoTarget,
    __in
    WDFREQUEST Request,
    __in
    ULONG Ioctl,
    __in_opt
    WDFMEMORY OtherArg1,
    __in_opt
    PWDFMEMORY_OFFSET OtherArg1Offsets,
    __in_opt
    WDFMEMORY OtherArg2,
    __in_opt
    PWDFMEMORY_OFFSET OtherArg2Offsets,
    __in_opt
    WDFMEMORY OtherArg4,
    __in_opt
    PWDFMEMORY_OFFSET OtherArg4Offsets
    )
/*++

Routine Description:
    Formats an internal IOCTL so that it can sent to the target.  Since all 3
    buffers can be used, we cannot overload
    WdfIoTargetFormatRequestForInternalIoctlOthers since it can only take 2 buffers.

    Upon success, this will take a reference on each of the WDFMEMORY handles that
    are passed in.  This reference will be released when one of the following
    occurs:
    1)  the request is completed through WdfRequestComplete
    2)  the request is reused through WdfRequestReuse
    3)  the request is reformatted through any target format DDI

Arguments:
    IoTarget - the target to which the request will be sent

    Request - the request to be formatted

    Ioctl - internal ioctl value to send

    OtherArg1
    OtherArg2
    OtherArg4 - arguments to use in the stack locations's Others field.  There
                is no OtherArg3 because 3 is where the IOCTL value is written.
                All buffers are optional

    OterhArgXOffsets - offset into each buffer which can override the starting
                       offset of the buffer.  Length does not matter since
                       there is no way of generically describing the length of
                       each of the 3 buffers in the PIRP

Return Value:
    NTSTATUS

  --*/
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

} // extern "C"
