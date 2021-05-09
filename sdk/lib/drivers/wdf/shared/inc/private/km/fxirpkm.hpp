/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxIrpKm.hpp

Abstract:

    This module implements km definitions for FxIrp functions.

Author:



Environment:

    Kernel mode only

Revision History:

--*/

//
// All the functions in this file should use __inline so that KMDF gets
// inlining. FxIrp.hpp does not use __inline on the functions because it
// doesn't work for UMDF (see comments in FxIrp.hpp).
//

#ifndef _FXIRPKM_HPP_
#define _FXIRPKM_HPP_

typedef PIRP MdIrp;

typedef DRIVER_CANCEL MdCancelRoutineType, *MdCancelRoutine;
typedef IO_COMPLETION_ROUTINE MdCompletionRoutineType, *MdCompletionRoutine;
typedef REQUEST_POWER_COMPLETE MdRequestPowerCompleteType, *MdRequestPowerComplete;

typedef
NTSTATUS
(*PFX_COMPLETION_ROUTINE)(
    __in FxDevice *Device,
    __in FxIrp *Irp,
    __in PVOID Context
    );

typedef
VOID
(*PFX_CANCEL_ROUTINE)(
    __in FxDevice *Device,
    __in FxIrp *Irp,
    __in PVOID CancelContext
    );

#include "fxirp.hpp"



__inline
MdIrp
FxIrp::GetIrp(
    VOID
    )
{
    return m_Irp;
}

__inline
MdIrp
FxIrp::SetIrp(
    MdIrp irp
    )
{
    MdIrp old = m_Irp;
    m_Irp = irp;
    return old;
}

__inline
VOID
FxIrp::CompleteRequest(
    __in CCHAR PriorityBoost
    )
{
    IoCompleteRequest(m_Irp, PriorityBoost);
    m_Irp = NULL;
}

__inline
NTSTATUS
FxIrp::CallDriver(
    __in MdDeviceObject DeviceObject
    )
{
    return IoCallDriver(DeviceObject, m_Irp);
}

__inline
NTSTATUS
FxIrp::PoCallDriver(
    __in MdDeviceObject DeviceObject
    )
{
    return ::PoCallDriver(DeviceObject, m_Irp);
}

__inline
VOID
FxIrp::StartNextPowerIrp(
    )
{
    PoStartNextPowerIrp(m_Irp);
}

__inline
MdCompletionRoutine
FxIrp::GetNextCompletionRoutine(
    VOID
    )
{
    return this->GetNextIrpStackLocation()->CompletionRoutine;
}


__inline
VOID
FxIrp::SetCompletionRoutine(
    __in MdCompletionRoutine CompletionRoutine,
    __in PVOID Context,
    __in BOOLEAN InvokeOnSuccess,
    __in BOOLEAN InvokeOnError,
    __in BOOLEAN InvokeOnCancel
    )
{
    IoSetCompletionRoutine(
        m_Irp,
        CompletionRoutine,
        Context,
        InvokeOnSuccess,
        InvokeOnError,
        InvokeOnCancel
        );
}

__inline
VOID
FxIrp::SetCompletionRoutineEx(
    __in MdDeviceObject DeviceObject,
    __in MdCompletionRoutine CompletionRoutine,
    __in PVOID Context,
    __in BOOLEAN InvokeOnSuccess,
    __in BOOLEAN InvokeOnError,
    __in BOOLEAN InvokeOnCancel
    )
{
    if (!NT_SUCCESS(IoSetCompletionRoutineEx(
            DeviceObject,
            m_Irp,
            CompletionRoutine,
            Context,
            InvokeOnSuccess,
            InvokeOnError,
            InvokeOnCancel))) {

        IoSetCompletionRoutine(
            m_Irp,
            CompletionRoutine,
            Context,
            InvokeOnSuccess,
            InvokeOnError,
            InvokeOnCancel
            );
    }
}

__inline
MdCancelRoutine
FxIrp::SetCancelRoutine(
    __in_opt MdCancelRoutine  CancelRoutine
    )
{
    return IoSetCancelRoutine(m_Irp, CancelRoutine);
}

__inline
NTSTATUS
STDCALL
FxIrp::_IrpSynchronousCompletion(
    __in MdDeviceObject DeviceObject,
    __in PIRP OriginalIrp,
    __in PVOID Context
    )
{
    FxCREvent* event = (FxCREvent*) Context;

    UNREFERENCED_PARAMETER(DeviceObject);

    if (OriginalIrp->PendingReturned) {
        //
        // No need to propagate the pending returned bit since we are handling
        // the request synchronously
        //
        event->Set();
    }

    return STATUS_MORE_PROCESSING_REQUIRED;
}

__inline
NTSTATUS
FxIrp::SendIrpSynchronously(
    __in MdDeviceObject DeviceObject
    )
{
    NTSTATUS status;
    FxCREvent event;

    SetCompletionRoutine(_IrpSynchronousCompletion,
                         event.GetSelfPointer(),
                         TRUE,
                         TRUE,
                         TRUE);

    status = CallDriver(DeviceObject);

    if (status == STATUS_PENDING) {
        event.EnterCRAndWaitAndLeave();
        status = m_Irp->IoStatus.Status;
    }

    return status;
}

__inline
VOID
FxIrp::CopyToNextIrpStackLocation(
    __in PIO_STACK_LOCATION Stack
    )
{
  PIO_STACK_LOCATION nextIrpSp = IoGetNextIrpStackLocation(m_Irp);

  RtlCopyMemory(nextIrpSp,
          Stack,
          FIELD_OFFSET(IO_STACK_LOCATION, CompletionRoutine)
          );
  nextIrpSp->Control = 0;
}


__inline
VOID
FxIrp::CopyCurrentIrpStackLocationToNext(
    VOID
    )
{
    IoCopyCurrentIrpStackLocationToNext(m_Irp);
}

__inline
VOID
FxIrp::SetNextIrpStackLocation(
    VOID
    )
{
    IoSetNextIrpStackLocation(m_Irp);
}

__inline
UCHAR
FxIrp::GetMajorFunction(
    VOID
    )
{
    return IoGetCurrentIrpStackLocation(m_Irp)->MajorFunction;
}

__inline
UCHAR
FxIrp::GetMinorFunction(
    VOID
    )
{
    return IoGetCurrentIrpStackLocation(m_Irp)->MinorFunction;
}

__inline
UCHAR
FxIrp::GetCurrentStackFlags(
    VOID
    )
{
    return IoGetCurrentIrpStackLocation(m_Irp)->Flags;
}

__inline
MdFileObject
FxIrp::GetCurrentStackFileObject(
    VOID
    )
{
    return IoGetCurrentIrpStackLocation(m_Irp)->FileObject;
}

__inline
KPROCESSOR_MODE
FxIrp::GetRequestorMode(
    VOID
    )
{
    return m_Irp->RequestorMode;
}

__inline
VOID
FxIrp::SetContext(
    __in ULONG Index,
    __in PVOID Value
    )
{
    m_Irp->Tail.Overlay.DriverContext[Index] = Value;
}

__inline
PVOID
FxIrp::GetContext(
    __in ULONG Index
    )
{
    return m_Irp->Tail.Overlay.DriverContext[Index];
}

__inline
VOID
FxIrp::SetFlags(
    __in ULONG Flags
    )
{
    m_Irp->Flags = Flags;
}

__inline
ULONG
FxIrp::GetFlags(
    VOID
    )
{
    return m_Irp->Flags;
}

__inline
PIO_STACK_LOCATION
FxIrp::GetCurrentIrpStackLocation(
    VOID
    )
{
    return IoGetCurrentIrpStackLocation(m_Irp);
}

__inline
PIO_STACK_LOCATION
FxIrp::GetNextIrpStackLocation(
    VOID
    )
{
    return IoGetNextIrpStackLocation(m_Irp);
}

PIO_STACK_LOCATION
__inline
FxIrp::_GetAndClearNextStackLocation(
    __in MdIrp Irp
    )
{
    RtlZeroMemory(IoGetNextIrpStackLocation(Irp),
                  FIELD_OFFSET(IO_STACK_LOCATION, CompletionRoutine));
    return IoGetNextIrpStackLocation(Irp);
}

__inline
VOID
FxIrp::SkipCurrentIrpStackLocation(
    VOID
    )
{






    IoSkipCurrentIrpStackLocation(m_Irp);
}

__inline
VOID
FxIrp::MarkIrpPending(
    )
{
    IoMarkIrpPending(m_Irp);
}

__inline
BOOLEAN
FxIrp::PendingReturned(
    )
{
    return m_Irp->PendingReturned;
}

__inline
VOID
FxIrp::PropagatePendingReturned(
    VOID
    )
{
    if (PendingReturned() && m_Irp->CurrentLocation <= m_Irp->StackCount) {
        MarkIrpPending();
    }
}

__inline
VOID
FxIrp::SetStatus(
    __in NTSTATUS Status
    )
{
    m_Irp->IoStatus.Status = Status;
}

__inline
NTSTATUS
FxIrp::GetStatus(
    )
{
    return m_Irp->IoStatus.Status;
}

__inline
BOOLEAN
FxIrp::Cancel(
    VOID
    )
{
    return IoCancelIrp(m_Irp);
}

__inline
VOID
FxIrp::SetCancel(
    __in BOOLEAN Cancel
    )
{
    m_Irp->Cancel = Cancel;
}

__inline
BOOLEAN
FxIrp::IsCanceled(
    )
{
    return m_Irp->Cancel ? TRUE : FALSE;
}

__inline
KIRQL
FxIrp::GetCancelIrql(
    )
{
    return m_Irp->CancelIrql;
}

__inline
VOID
FxIrp::SetInformation(
    ULONG_PTR Information
    )
{
    m_Irp->IoStatus.Information = Information;
}

__inline
ULONG_PTR
FxIrp::GetInformation(
    )
{
    return m_Irp->IoStatus.Information;
}

__inline
CCHAR
FxIrp::GetCurrentIrpStackLocationIndex(
    )
{
    return m_Irp->CurrentLocation;
}

__inline
CCHAR
FxIrp::GetStackCount(
    )
{
    return m_Irp->StackCount;
}

__inline
PLIST_ENTRY
FxIrp::ListEntry(
    )
{
    return &m_Irp->Tail.Overlay.ListEntry;
}

__inline
PVOID
FxIrp::GetSystemBuffer(
    )
{
    return m_Irp->AssociatedIrp.SystemBuffer;
}

__inline
PVOID
FxIrp::GetOutputBuffer(
    )
{
    //
    // In kernel mode, for buffered I/O, the output and input buffers are
    // at same location.
    //
    return GetSystemBuffer();
}

__inline
VOID
FxIrp::SetSystemBuffer(
    __in PVOID Value
    )
{
    m_Irp->AssociatedIrp.SystemBuffer = Value;
}


__inline
PMDL
FxIrp::GetMdl(
    )
{
    return m_Irp->MdlAddress;
}

__inline
PMDL*
FxIrp::GetMdlAddressPointer(
    )
{
    return &m_Irp->MdlAddress;
}

__inline
VOID
FxIrp::SetMdlAddress(
    __in PMDL Value
    )
{
    m_Irp->MdlAddress = Value;
}


__inline
PVOID
FxIrp::GetUserBuffer(
    )
{
    return m_Irp->UserBuffer;
}


__inline
VOID
FxIrp::SetUserBuffer(
    __in PVOID Value
    )
{
    m_Irp->UserBuffer = Value;
}

__inline
VOID
FxIrp::Reuse(
    __in NTSTATUS Status
    )
{
    IoReuseIrp(m_Irp, Status);
}

__inline
VOID
FxIrp::SetMajorFunction(
    __in UCHAR MajorFunction
    )
{
    this->GetNextIrpStackLocation()->MajorFunction = MajorFunction;
}

__inline
VOID
FxIrp::SetMinorFunction(
    __in UCHAR MinorFunction
    )
{
    this->GetNextIrpStackLocation()->MinorFunction = MinorFunction;
}

__inline
SYSTEM_POWER_STATE_CONTEXT
FxIrp::GetParameterPowerSystemPowerStateContext(
    )
{
    return (this->GetCurrentIrpStackLocation())->
        Parameters.Power.SystemPowerStateContext;
}

__inline
POWER_STATE_TYPE
FxIrp::GetParameterPowerType(
    )
{
    return (this->GetCurrentIrpStackLocation())->Parameters.Power.Type;
}

__inline
POWER_STATE
FxIrp::GetParameterPowerState(
    )
{
    return (this->GetCurrentIrpStackLocation())->Parameters.Power.State;
}

__inline
DEVICE_POWER_STATE
FxIrp::GetParameterPowerStateDeviceState(
    )
{
    return (this->GetCurrentIrpStackLocation())->
        Parameters.Power.State.DeviceState;
}

__inline
SYSTEM_POWER_STATE
FxIrp::GetParameterPowerStateSystemState(
    )
{
    return (this->GetCurrentIrpStackLocation())->
        Parameters.Power.State.SystemState;
}

__inline
POWER_ACTION
FxIrp::GetParameterPowerShutdownType(
    )
{
    return (this->GetCurrentIrpStackLocation())->
        Parameters.Power.ShutdownType;
}

__inline
DEVICE_RELATION_TYPE
FxIrp::GetParameterQDRType(
    )
{
    return (this->GetCurrentIrpStackLocation())->
        Parameters.QueryDeviceRelations.Type;
}

__inline
VOID
FxIrp::SetParameterQDRType(
    DEVICE_RELATION_TYPE DeviceRelation
	)
{
     this->GetNextIrpStackLocation()->
        Parameters.QueryDeviceRelations.Type = DeviceRelation;
}

__inline
PDEVICE_CAPABILITIES
FxIrp::GetParameterDeviceCapabilities(
    )
{
    return this->GetCurrentIrpStackLocation()->
        Parameters.DeviceCapabilities.Capabilities;
}

__inline
MdDeviceObject
FxIrp::GetDeviceObject(
    VOID
    )
{
  return this->GetCurrentIrpStackLocation()->DeviceObject;
}

__inline
VOID
FxIrp::SetCurrentDeviceObject(
    __in MdDeviceObject DeviceObject
    )
{
  this->GetCurrentIrpStackLocation()->DeviceObject = DeviceObject;
}

__inline
VOID
FxIrp::SetParameterDeviceCapabilities(
    __in PDEVICE_CAPABILITIES DeviceCapabilities
    )
{
    this->GetNextIrpStackLocation()->
        Parameters.DeviceCapabilities.Capabilities = DeviceCapabilities;
}

__inline
LONGLONG
FxIrp::GetParameterWriteByteOffsetQuadPart(
    )
{
    return this->GetCurrentIrpStackLocation()->
        Parameters.Write.ByteOffset.QuadPart;
}

__inline
VOID
FxIrp::SetNextParameterWriteByteOffsetQuadPart(
    __in LONGLONG DeviceOffset
    )
{
    this->GetNextIrpStackLocation()->
        Parameters.Write.ByteOffset.QuadPart = DeviceOffset;
}

__inline
VOID
FxIrp::SetNextParameterWriteLength(
    __in ULONG IoLength
    )
{
    this->GetNextIrpStackLocation()->
        Parameters.Write.Length = IoLength;
}

__inline
PVOID*
FxIrp::GetNextStackParameterOthersArgument1Pointer(
    )
{
    PIO_STACK_LOCATION nextStack;

    nextStack = this->GetNextIrpStackLocation();

    return &nextStack->Parameters.Others.Argument1;
}

__inline
VOID
FxIrp::SetNextStackParameterOthersArgument1(
    __in PVOID Argument1
    )
{
    this->GetNextIrpStackLocation()->
        Parameters.Others.Argument1 = Argument1;
}

__inline
PVOID*
FxIrp::GetNextStackParameterOthersArgument2Pointer(
    )
{
    PIO_STACK_LOCATION nextStack;

    nextStack = this->GetNextIrpStackLocation();

    return &nextStack->Parameters.Others.Argument2;
}

__inline
PVOID*
FxIrp::GetNextStackParameterOthersArgument4Pointer(
    )
{
    PIO_STACK_LOCATION nextStack;

    nextStack = this->GetNextIrpStackLocation();

    return &nextStack->Parameters.Others.Argument4;
}

__inline
PCM_RESOURCE_LIST
FxIrp::GetParameterAllocatedResources(
    )
{
    return this->GetCurrentIrpStackLocation()->
        Parameters.StartDevice.AllocatedResources;
}

__inline
VOID
FxIrp::SetParameterAllocatedResources(
    __in PCM_RESOURCE_LIST AllocatedResources
    )
{
    this->GetNextIrpStackLocation()->
        Parameters.StartDevice.AllocatedResources = AllocatedResources;
}

__inline
PCM_RESOURCE_LIST
FxIrp::GetParameterAllocatedResourcesTranslated(
    )
{
    return this->GetCurrentIrpStackLocation()->
        Parameters.StartDevice.AllocatedResourcesTranslated;
}

__inline
VOID
FxIrp::SetParameterAllocatedResourcesTranslated(
    __in PCM_RESOURCE_LIST AllocatedResourcesTranslated
    )
{
    this->GetNextIrpStackLocation()->
        Parameters.StartDevice.AllocatedResourcesTranslated =
            AllocatedResourcesTranslated;
}

__inline
LCID
FxIrp::GetParameterQueryDeviceTextLocaleId(
    )
{
    return this->GetCurrentIrpStackLocation()->
        Parameters.QueryDeviceText.LocaleId;
}

__inline
DEVICE_TEXT_TYPE
FxIrp::GetParameterQueryDeviceTextType(
    )
{
    return this->GetCurrentIrpStackLocation()->
        Parameters.QueryDeviceText.DeviceTextType;
}

__inline
BOOLEAN
FxIrp::GetParameterSetLockLock(
    )
{
    return this->GetCurrentIrpStackLocation()->Parameters.SetLock.Lock;
}

__inline
BUS_QUERY_ID_TYPE
FxIrp::GetParameterQueryIdType(
    )
{
    return this->GetCurrentIrpStackLocation()->Parameters.QueryId.IdType;
}

__inline
PINTERFACE
FxIrp::GetParameterQueryInterfaceInterface(
    )
{
    return this->GetCurrentIrpStackLocation()->
        Parameters.QueryInterface.Interface;
}

__inline
const GUID*
FxIrp::GetParameterQueryInterfaceType(
    )
{
    return this->GetCurrentIrpStackLocation()->
        Parameters.QueryInterface.InterfaceType;
}

__inline
MdFileObject
FxIrp::GetFileObject(
    VOID
    )
{
    return this->GetCurrentIrpStackLocation()->FileObject;
}

__inline
USHORT
FxIrp::GetParameterQueryInterfaceVersion(
    )
{
    return this->GetCurrentIrpStackLocation()->Parameters.QueryInterface.Version;
}

__inline
USHORT
FxIrp::GetParameterQueryInterfaceSize(
    )
{
    return this->GetCurrentIrpStackLocation()->Parameters.QueryInterface.Size;
}

__inline
PVOID
FxIrp::GetParameterQueryInterfaceInterfaceSpecificData(
    )
{
    return this->GetCurrentIrpStackLocation()->
        Parameters.QueryInterface.InterfaceSpecificData;
}

__inline
DEVICE_USAGE_NOTIFICATION_TYPE
FxIrp::GetParameterUsageNotificationType(
    )
{
    return this->GetCurrentIrpStackLocation()->
        Parameters.UsageNotification.Type;
}

__inline
BOOLEAN
FxIrp::GetParameterUsageNotificationInPath(
    )
{
    return this->GetCurrentIrpStackLocation()->
        Parameters.UsageNotification.InPath;
}

__inline
VOID
FxIrp::SetParameterUsageNotificationInPath(
    __in BOOLEAN InPath
    )
{
    this->GetNextIrpStackLocation()->
        Parameters.UsageNotification.InPath = InPath;
}

__inline
BOOLEAN
FxIrp::GetNextStackParameterUsageNotificationInPath(
    )
{
    return this->GetNextIrpStackLocation()->
        Parameters.UsageNotification.InPath;
}



__inline
ULONG
FxIrp::GetParameterIoctlCode(
    VOID
    )
{
    return this->GetCurrentIrpStackLocation()->
        Parameters.DeviceIoControl.IoControlCode;
}

__inline
ULONG
FxIrp::GetParameterIoctlCodeBufferMethod(
    VOID
    )
{
    return METHOD_FROM_CTL_CODE(GetParameterIoctlCode());
}

__inline
ULONG
FxIrp::GetParameterIoctlOutputBufferLength(
    VOID
    )
{
    return this->GetCurrentIrpStackLocation()->
        Parameters.DeviceIoControl.OutputBufferLength;
}

__inline
ULONG
FxIrp::GetParameterIoctlInputBufferLength(
    VOID
    )
{
    return this->GetCurrentIrpStackLocation()->
        Parameters.DeviceIoControl.InputBufferLength;
}

__inline
VOID
FxIrp::SetParameterIoctlCode(
    __in ULONG DeviceIoControlCode
    )
{
    this->GetNextIrpStackLocation()->
        Parameters.DeviceIoControl.IoControlCode = DeviceIoControlCode;
}

__inline
VOID
FxIrp::SetParameterIoctlInputBufferLength(
    __in ULONG InputBufferLength
    )
{
    this->GetNextIrpStackLocation()->
        Parameters.DeviceIoControl.InputBufferLength = InputBufferLength;
}

__inline
VOID
FxIrp::SetParameterIoctlOutputBufferLength(
    __in ULONG OutputBufferLength
    )
{
    this->GetNextIrpStackLocation()->
        Parameters.DeviceIoControl.OutputBufferLength = OutputBufferLength;
}

__inline
VOID
FxIrp::SetParameterIoctlType3InputBuffer(
    __in PVOID Type3InputBuffer
    )
{
    this->GetNextIrpStackLocation()->
        Parameters.DeviceIoControl.Type3InputBuffer = Type3InputBuffer;
}

__inline
PVOID
FxIrp::GetParameterIoctlType3InputBuffer(
    VOID
    )
{
    return this->GetCurrentIrpStackLocation()->
               Parameters.DeviceIoControl.Type3InputBuffer;
}

__inline
VOID
FxIrp::SetParameterQueryInterfaceInterface(
    __in PINTERFACE Interface
    )
{
    this->GetNextIrpStackLocation()->
        Parameters.QueryInterface.Interface = Interface;
}

__inline
VOID
FxIrp::SetParameterQueryInterfaceType(
    __in const GUID* InterfaceType
    )
{
    this->GetNextIrpStackLocation()->
        Parameters.QueryInterface.InterfaceType = InterfaceType;
}

__inline
VOID
FxIrp::SetParameterQueryInterfaceVersion(
    __in USHORT Version
    )
{
    this->GetNextIrpStackLocation()->Parameters.QueryInterface.Version = Version;
}

__inline
VOID
FxIrp::SetParameterQueryInterfaceSize(
    __in USHORT Size
    )
{
    this->GetNextIrpStackLocation()->Parameters.QueryInterface.Size = Size;
}

__inline
VOID
FxIrp::SetParameterQueryInterfaceInterfaceSpecificData(
    __in PVOID InterfaceSpecificData
    )
{
    this->GetNextIrpStackLocation()->
        Parameters.QueryInterface.InterfaceSpecificData = InterfaceSpecificData;
}

__inline
VOID
FxIrp::SetNextStackFlags(
    __in UCHAR Flags
    )
{
    this->GetNextIrpStackLocation()->Flags = Flags;
}

__inline
VOID
FxIrp::SetNextStackFileObject(
    _In_ MdFileObject FileObject
    )
{
    this->GetNextIrpStackLocation()->FileObject = FileObject;
}


__inline
VOID
FxIrp::ClearNextStack(
    VOID
    )
{
    PIO_STACK_LOCATION stack;

    stack = this->GetNextIrpStackLocation();
    RtlZeroMemory(stack, sizeof(IO_STACK_LOCATION));
}




__inline
VOID
FxIrp::ClearNextStackLocation(
    VOID
    )
{
    RtlZeroMemory(this->GetNextIrpStackLocation(),
                  FIELD_OFFSET(IO_STACK_LOCATION, CompletionRoutine));
}

__inline
VOID
FxIrp::InitNextStackUsingStack(
    __in FxIrp* Irp
    )
{
    PIO_STACK_LOCATION srcStack, destStack;

    srcStack = Irp->GetCurrentIrpStackLocation();
    destStack = this->GetNextIrpStackLocation();

    *destStack = *srcStack;
}

_Must_inspect_result_
__inline
MdIrp
FxIrp::AllocateIrp(
    _In_ CCHAR StackSize,
    _In_opt_ FxDevice* Device
    )
{
    UNREFERENCED_PARAMETER(Device);

    return IoAllocateIrp(StackSize, FALSE);
}

__inline
MdIrp
FxIrp::GetIrpFromListEntry(
    __in PLIST_ENTRY Ple
    )
{
    return CONTAINING_RECORD(Ple, IRP, Tail.Overlay.ListEntry);
}

__inline
ULONG
FxIrp::GetParameterReadLength(
    VOID
    )
{
  return this->GetCurrentIrpStackLocation()->Parameters.Read.Length;
}

__inline
ULONG
FxIrp::GetParameterWriteLength(
    VOID
    )
{
  return this->GetCurrentIrpStackLocation()->Parameters.Write.Length;
}

_Must_inspect_result_
__inline
NTSTATUS
FxIrp::RequestPowerIrp(
    __in MdDeviceObject  DeviceObject,
    __in UCHAR  MinorFunction,
    __in POWER_STATE  PowerState,
    __in MdRequestPowerComplete  CompletionFunction,
    __in PVOID  Context
    )
{
    //
    // Prefast enforces that NULL is passed for IRP parameter (last parameter)
    // since the IRP might complete before the function returns.
    //
    return PoRequestPowerIrp(
        DeviceObject,
        MinorFunction,
        PowerState,
        CompletionFunction,
        Context,
        NULL);
}

__inline
ULONG
FxIrp::GetCurrentFlags(
    VOID
    )
{
    return (this->GetCurrentIrpStackLocation())->Flags;
}

__inline
PVOID
FxIrp::GetCurrentParametersPointer(
    VOID
    )
{
    return &(this->GetCurrentIrpStackLocation())->Parameters;
}

__inline
MdEThread
FxIrp::GetThread(
    VOID
    )
{
    return  m_Irp->Tail.Overlay.Thread;
}

__inline
BOOLEAN
FxIrp::Is32bitProcess(
    VOID
    )
{

#if defined(_WIN64)

#if BUILD_WOW64_ENABLED

    return IoIs32bitProcess(m_Irp);

#else // BUILD_WOW64_ENABLED

    return FALSE;

#endif // BUILD_WOW64_ENABLED

#else // defined(_WIN64)

    return TRUE;

#endif // defined(_WIN64)

}

__inline
VOID
FxIrp::FreeIrp(
    VOID
    )
{
    IoFreeIrp(m_Irp);
}

__inline
PIO_STATUS_BLOCK
FxIrp::GetStatusBlock(
    VOID
    )
{
    return &m_Irp->IoStatus;
}

__inline
PVOID
FxIrp::GetDriverContext(
    VOID
    )
{
    return m_Irp->Tail.Overlay.DriverContext;
}

__inline
ULONG
FxIrp::GetDriverContextSize(
    VOID
    )
{
    return sizeof(m_Irp->Tail.Overlay.DriverContext);
}

__inline
VOID
FxIrp::CopyParameters(
    _Out_ PWDF_REQUEST_PARAMETERS Parameters
    )
{
    RtlMoveMemory(&Parameters->Parameters,
                  GetCurrentParametersPointer(),
                  sizeof(Parameters->Parameters));
}

__inline
VOID
FxIrp::CopyStatus(
    _Out_ PIO_STATUS_BLOCK StatusBlock
    )
{
    RtlCopyMemory(StatusBlock,
                  GetStatusBlock(),
                  sizeof(*StatusBlock));
}

__inline
BOOLEAN
FxIrp::HasStack(
    _In_ UCHAR StackCount
    )
{
    return (GetCurrentIrpStackLocationIndex() >= StackCount);
}

__inline
BOOLEAN
FxIrp::IsCurrentIrpStackLocationValid(
    VOID
    )
{
    return (GetCurrentIrpStackLocationIndex() <= GetStackCount());
}

__inline
FxAutoIrp::~FxAutoIrp()
{
    if (m_Irp != NULL) {
        IoFreeIrp(m_Irp);
    }
}

#endif // _FXIRPKM_HPP
