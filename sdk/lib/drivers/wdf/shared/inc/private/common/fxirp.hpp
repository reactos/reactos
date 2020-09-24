/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxIrp.hpp

Abstract:

    This module implements a class for handling irps.

Author:



Environment:

    Both kernel and user mode

Revision History:

































































    // A function for when not assigning

    MdIrp
    GetIrp(
        VOID
        );


    VOID
    CompleteRequest(
        __in_opt CCHAR PriorityBoost=IO_NO_INCREMENT
        );


    NTSTATUS
    CallDriver(
        __in MdDeviceObject DeviceObject
        );







    NTSTATUS
    PoCallDriver(
        __in MdDeviceObject DeviceObject
        );


    VOID
    StartNextPowerIrp(
        );

    MdCompletionRoutine
    GetNextCompletionRoutine(
        VOID
        );

    VOID
    SetCompletionRoutine(
        __in MdCompletionRoutine CompletionRoutine,
        __in PVOID Context,
        __in BOOLEAN InvokeOnSuccess = TRUE,
        __in BOOLEAN InvokeOnError = TRUE,
        __in BOOLEAN InvokeOnCancel = TRUE
        );

    VOID
    SetCompletionRoutineEx(
        __in MdDeviceObject DeviceObject,
        __in MdCompletionRoutine CompletionRoutine,
        __in PVOID Context,
        __in BOOLEAN InvokeOnSuccess = TRUE,
        __in BOOLEAN InvokeOnError = TRUE,
        __in BOOLEAN InvokeOnCancel = TRUE
        );

    MdCancelRoutine
    SetCancelRoutine(
        __in_opt MdCancelRoutine  CancelRoutine
        );

    //
    // SendIrpSynchronously achieves synchronous behavior by waiting on an
    // event after submitting the IRP. The event creation can fail in UM, but
    // not in KM. Hence, in UM the return code could either indicate event
    // creation failure or it could indicate the status set on the IRP by the
    // lower driver. In KM, the return code only indicates the status set on
    // the IRP by the lower lower, because event creation cannot fail.
    //
    CHECK_RETURN_IF_USER_MODE
    NTSTATUS
    SendIrpSynchronously(
        __in MdDeviceObject DeviceObject
        );


    VOID
    CopyCurrentIrpStackLocationToNext(
        VOID
        );

    VOID
    CopyToNextIrpStackLocation(
        __in PIO_STACK_LOCATION Stack
        );

    VOID
    SetNextIrpStackLocation(
        VOID
    );

    UCHAR
    GetMajorFunction(
        VOID
        );

    UCHAR
    GetMinorFunction(
        VOID
        );

    UCHAR
    GetCurrentStackFlags(
        VOID
        );

    MdFileObject
    GetCurrentStackFileObject(
        VOID
        );

    KPROCESSOR_MODE
    GetRequestorMode(
        VOID
        );

    VOID
    SetContext(
        __in ULONG Index,
        __in PVOID Value
        );

    VOID
    SetSystemBuffer(
        __in PVOID Value
        );

    VOID
    SetUserBuffer(
        __in PVOID Value
        );

    VOID
    SetMdlAddress(
        __in PMDL Value
        );

    VOID
    SetFlags(
        __in ULONG Flags
        );

    PVOID
    GetContext(
        __in ULONG Index
        );

    ULONG
    GetFlags(
        VOID
        );

    PIO_STACK_LOCATION
    GetCurrentIrpStackLocation(
        VOID
        );


    PIO_STACK_LOCATION
    GetNextIrpStackLocation(
        VOID
        );

    static
    PIO_STACK_LOCATION
    _GetAndClearNextStackLocation(
        __in MdIrp Irp
        );


    VOID
    SkipCurrentIrpStackLocation(
        VOID
        );


    VOID
    MarkIrpPending(
        );


    BOOLEAN
    PendingReturned(
        );


    VOID
    PropagatePendingReturned(
        VOID
        );


    VOID
    SetStatus(
        __in NTSTATUS Status
        );


    NTSTATUS
    GetStatus(
        );


    BOOLEAN
    Cancel(
        VOID
        );

    VOID
    SetCancel(
        __in BOOLEAN Cancel
        );


    BOOLEAN
    IsCanceled(
        );


    KIRQL
    GetCancelIrql(
        );


    VOID
    SetInformation(
        __in ULONG_PTR Information
        );


    ULONG_PTR
    GetInformation(
        );


    CCHAR
    GetCurrentIrpStackLocationIndex(
        );

    CCHAR
    GetStackCount(
        );

    PLIST_ENTRY
    ListEntry(
        );


    PVOID
    GetSystemBuffer(
        );

    PVOID
    GetOutputBuffer(
        );

    PMDL
    GetMdl(
        );

    PMDL*
    GetMdlAddressPointer(
        );

    PVOID
    GetUserBuffer(
        );

    VOID
    Reuse(
        __in NTSTATUS Status = STATUS_SUCCESS
        );

    //
    // Methods for IO_STACK_LOCATION members
    //

    VOID
    SetMajorFunction(
        __in UCHAR MajorFunction
        );


    VOID
    SetMinorFunction(
        __in UCHAR MinorFunction
        );

    //
    // Get Methods for IO_STACK_LOCATION.Parameters.Power
    //

    SYSTEM_POWER_STATE_CONTEXT
    GetParameterPowerSystemPowerStateContext(
        );


    POWER_STATE_TYPE
    GetParameterPowerType(
        );

    POWER_STATE
    GetParameterPowerState(
        );


    DEVICE_POWER_STATE
    GetParameterPowerStateDeviceState(
        );


    SYSTEM_POWER_STATE
    GetParameterPowerStateSystemState(
        );


    POWER_ACTION
    GetParameterPowerShutdownType(
        );


    MdFileObject
    GetFileObject(
        VOID
        );


    //
    // Get/Set Method for IO_STACK_LOCATION.Parameters.QueryDeviceRelations
    //

    DEVICE_RELATION_TYPE
    GetParameterQDRType(
        );

    VOID
    SetParameterQDRType(
        __in DEVICE_RELATION_TYPE DeviceRelation
        );

    //
    // Get/Set Methods for IO_STACK_LOCATION.Parameters.DeviceCapabilities
    //

    PDEVICE_CAPABILITIES
    GetParameterDeviceCapabilities(
        );

    VOID
    SetCurrentDeviceObject(
        __in MdDeviceObject  DeviceObject
        );

    MdDeviceObject
    GetDeviceObject(
        VOID
        );

    VOID
    SetParameterDeviceCapabilities(
        __in PDEVICE_CAPABILITIES DeviceCapabilities
        );


    //
    // Get/Set Methods for IO_STACK_LOCATION.Parameters.Write.ByteOffset.QuadPart
    //

    LONGLONG
    GetParameterWriteByteOffsetQuadPart(
        );


    VOID
    SetNextParameterWriteByteOffsetQuadPart(
        __in LONGLONG DeviceOffset
        );

    //
    // Get/Set Methods for IO_STACK_LOCATION.Parameters.Write.Length
    //

    ULONG
    GetCurrentParameterWriteLength(
        );

    VOID
    SetNextParameterWriteLength(
        __in ULONG IoLength
        );

    PVOID*
    GetNextStackParameterOthersArgument1Pointer(
        );

    VOID
    SetNextStackParameterOthersArgument1(
        __in PVOID Argument1
        );

    PVOID*
    GetNextStackParameterOthersArgument2Pointer(
        );

    PVOID*
    GetNextStackParameterOthersArgument4Pointer(
        );

    //
    // Get/Set Methods for IO_STACK_LOCATION.Parameters.StartDevice
    //

    PCM_RESOURCE_LIST
    GetParameterAllocatedResources(
        );


    VOID
    SetParameterAllocatedResources(
        __in PCM_RESOURCE_LIST AllocatedResources
        );


    PCM_RESOURCE_LIST
    GetParameterAllocatedResourcesTranslated(
        );


    VOID
    SetParameterAllocatedResourcesTranslated(
        __in PCM_RESOURCE_LIST AllocatedResourcesTranslated
        );

    //
    // Get Method for IO_STACK_LOCATION.Parameters.QueryDeviceText
    //

    LCID
    GetParameterQueryDeviceTextLocaleId(
        );


    DEVICE_TEXT_TYPE
    GetParameterQueryDeviceTextType(
        );

    //
    // Get Method for IO_STACK_LOCATION.Parameters.SetLock
    //

    BOOLEAN
    GetParameterSetLockLock(
        );

    //
    // Get Method for IO_STACK_LOCATION.Parameters.QueryId
    //

    BUS_QUERY_ID_TYPE
    GetParameterQueryIdType(
        );

    //
    // Get/Set Methods for IO_STACK_LOCATION.Parameters.QueryInterface
    //

    PINTERFACE
    GetParameterQueryInterfaceInterface(
        );


    const GUID*
    GetParameterQueryInterfaceType(
        );


    USHORT
    GetParameterQueryInterfaceVersion(
        );


    USHORT
    GetParameterQueryInterfaceSize(
        );


    PVOID
    GetParameterQueryInterfaceInterfaceSpecificData(
        );

    VOID
    SetParameterQueryInterfaceInterface(
        __in PINTERFACE Interface
        );

    VOID
    SetParameterQueryInterfaceType(
        __in const GUID* InterfaceType
        );

    VOID
    SetParameterQueryInterfaceVersion(
        __in USHORT Version
        );

    VOID
    SetParameterQueryInterfaceSize(
        __in USHORT Size
        );

    VOID
    SetParameterQueryInterfaceInterfaceSpecificData(
        __in PVOID InterfaceSpecificData
        );

    //
    // Get Method for IO_STACK_LOCATION.Parameters.UsageNotification
    //

    DEVICE_USAGE_NOTIFICATION_TYPE
    GetParameterUsageNotificationType(
        );


    BOOLEAN
    GetParameterUsageNotificationInPath(
        );


    VOID
    SetParameterUsageNotificationInPath(
        __in BOOLEAN InPath
        );


    BOOLEAN
    GetNextStackParameterUsageNotificationInPath(
        );

    ULONG
    GetParameterIoctlCode(
        VOID
        );

    ULONG
    GetParameterIoctlCodeBufferMethod(
        VOID
        );

    ULONG
    GetParameterIoctlInputBufferLength(
        VOID
        );

    ULONG
    GetParameterIoctlOutputBufferLength(
        VOID
        );

    PVOID
    GetParameterIoctlType3InputBuffer(
        VOID
        );

    //
    // Set Methods for IO_STACK_LOCATION.Parameters.DeviceControl members
    //

    VOID
    SetParameterIoctlCode(
        __in ULONG DeviceIoControlCode
        );

    VOID
    SetParameterIoctlInputBufferLength(
        __in ULONG InputBufferLength
        );

    VOID
    SetParameterIoctlOutputBufferLength(
        __in ULONG OutputBufferLength
        );

    VOID
    SetParameterIoctlType3InputBuffer(
        __in PVOID Type3InputBuffer
        );

    ULONG
    GetParameterReadLength(
        VOID
        );

    ULONG
    GetParameterWriteLength(
        VOID
        );

    VOID
    SetNextStackFlags(
        __in UCHAR Flags
        );

    VOID
    SetNextStackFileObject(
        _In_ MdFileObject FileObject
        );

    PVOID
    GetCurrentParametersPointer(
        VOID
        );

    //
    // Methods for IO_STACK_LOCATION
    //

    VOID
    ClearNextStack(
        VOID
        );

    VOID
    ClearNextStackLocation(
        VOID
        );

    VOID
    InitNextStackUsingStack(
        __in FxIrp* Irp
        );

    ULONG
    GetCurrentFlags(
        VOID
        );

    VOID
    FreeIrp(
        VOID
        );

    MdEThread
    GetThread(
        VOID
        );

    BOOLEAN
    Is32bitProcess(
        VOID
        );

private:

    static
    NTSTATUS
    _IrpSynchronousCompletion(
        __in MdDeviceObject DeviceObject,
        __in MdIrp OriginalIrp,
        __in PVOID Context
        );

public:

    _Must_inspect_result_
    static
    MdIrp
    AllocateIrp(
        _In_ CCHAR StackSize,
        _In_opt_ FxDevice* Device = NULL
        );

    static
    MdIrp
    GetIrpFromListEntry(
        __in PLIST_ENTRY Ple
        );

    _Must_inspect_result_
    static
    NTSTATUS
    RequestPowerIrp(
        __in MdDeviceObject  DeviceObject,
        __in UCHAR  MinorFunction,
        __in POWER_STATE  PowerState,
        __in MdRequestPowerComplete  CompletionFunction,
        __in PVOID  Context
        );

    PIO_STATUS_BLOCK
    GetStatusBlock(
        VOID
        );

    PVOID
    GetDriverContext(
        );

    ULONG
    GetDriverContextSize(
        );

    VOID
    CopyParameters(
        _Out_ PWDF_REQUEST_PARAMETERS Parameters
        );

    VOID
    CopyStatus(
        _Out_ PIO_STATUS_BLOCK StatusBlock
        );

    BOOLEAN
    HasStack(
        _In_ UCHAR StackCount
        );

    BOOLEAN
    IsCurrentIrpStackLocationValid(
        VOID
        );

#if (FX_CORE_MODE == FX_CORE_USER_MODE)

    IWudfIoIrp*
    GetIoIrp(
        VOID
        );

    IWudfPnpIrp*
    GetPnpIrp(
        VOID
        );
#endif

};

//
// FxAutoIrp adds value to FxIrp by automatically freeing the associated MdIrp
// when it goes out of scope
//
struct FxAutoIrp : public FxIrp {

    FxAutoIrp(
        __in_opt MdIrp Irp = NULL
        ) :
        FxIrp(Irp)
    {
    }


    ~FxAutoIrp();
};

#endif //  _FXIRP_H_
