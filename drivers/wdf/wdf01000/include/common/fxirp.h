#ifndef _FXIRPKM_H_
#define _FXIRPKM_H_

#include "common/fxwaitlock.h"
#include <ntddk.h>
#include <wdf.h>

class FxDevice;
typedef PIRP MdIrp;

typedef DRIVER_CANCEL MdCancelRoutineType, *MdCancelRoutine;
typedef IO_COMPLETION_ROUTINE MdCompletionRoutineType, *MdCompletionRoutine;
typedef REQUEST_POWER_COMPLETE MdRequestPowerCompleteType, *MdRequestPowerComplete;

class FxIrp {

    friend struct FxAutoIrp;

private:
    MdIrp m_Irp;

public:

    FxIrp() {}

    FxIrp(MdIrp irp) : m_Irp(irp)
    {
    }

    UCHAR
    GetMajorFunction()
    {
        return GetCurrentIrpStackLocation()->MajorFunction;
    }

    UCHAR
    GetMinorFunction()
    {
        return GetCurrentIrpStackLocation()->MinorFunction;
    }

    VOID
    SetStatus(NTSTATUS status)
    {
        m_Irp->IoStatus.Status = status;
    }
            
            
    VOID
    CompleteRequest(CCHAR priority)
    {
        IoCompleteRequest(m_Irp, priority);
    }

    VOID
    SetIrp(MdIrp irp)
    {
        m_Irp = irp;
    }

    MdIrp
    GetIrp()
    {
        return m_Irp;
    }

    NTSTATUS
    GetStatus()
    {
        return m_Irp->IoStatus.Status;
    }

    __inline
    PIO_STACK_LOCATION
    GetCurrentIrpStackLocation(
        VOID
        )
    {
        return IoGetCurrentIrpStackLocation(m_Irp);
    }

    __inline
    DEVICE_RELATION_TYPE
    GetParameterQDRType(
        )
    {
        return (this->GetCurrentIrpStackLocation())->
            Parameters.QueryDeviceRelations.Type;
    }

    __inline
    POWER_STATE_TYPE
    GetParameterPowerType(
        )
    {
        return (this->GetCurrentIrpStackLocation())->Parameters.Power.Type;
    }

    __inline
    SYSTEM_POWER_STATE
    GetParameterPowerStateSystemState(
        )
    {
        return (this->GetCurrentIrpStackLocation())->
            Parameters.Power.State.SystemState;
    }

    __inline
    DEVICE_POWER_STATE
    GetParameterPowerStateDeviceState(
        )
    {
        return (this->GetCurrentIrpStackLocation())->
            Parameters.Power.State.DeviceState;
    }

    __inline
    POWER_STATE
    GetParameterPowerState(
        )
    {
        return (this->GetCurrentIrpStackLocation())->Parameters.Power.State;
    }

    __inline
    VOID
    CopyCurrentIrpStackLocationToNext(
        VOID
        )
    {
        IoCopyCurrentIrpStackLocationToNext(m_Irp);
    }

    __inline
    NTSTATUS
    CallDriver(
        __in MdDeviceObject DeviceObject
        )
    {
        return IoCallDriver(DeviceObject, m_Irp);
    }

    __inline
    VOID
    StartNextPowerIrp(
        )
    {
        PoStartNextPowerIrp(m_Irp);
    }

    __inline
    NTSTATUS
    PoCallDriver(
        __in MdDeviceObject DeviceObject
        )
    {
        return ::PoCallDriver(DeviceObject, m_Irp);
    }

    _Must_inspect_result_
    static
    MdIrp
    AllocateIrp(
        _In_ CCHAR StackSize,
        _In_opt_ FxDevice* Device = NULL
        )
    {
        UNREFERENCED_PARAMETER(Device);
    
        return IoAllocateIrp(StackSize, FALSE);
    }

    __inline
    PIO_STACK_LOCATION
    GetNextIrpStackLocation(
        VOID
        )
    {
        return IoGetNextIrpStackLocation(m_Irp);
    }

    __inline
    VOID
    ClearNextStack(
        VOID
        )
    {
        PIO_STACK_LOCATION stack;

        stack = this->GetNextIrpStackLocation();
        RtlZeroMemory(stack, sizeof(IO_STACK_LOCATION));
    }

    __inline
    VOID
    SetMajorFunction(
        __in UCHAR MajorFunction
        )
    {
        this->GetNextIrpStackLocation()->MajorFunction = MajorFunction;
    }

    __inline
    VOID
    SetMinorFunction(
        __in UCHAR MinorFunction
        )
    {
        this->GetNextIrpStackLocation()->MinorFunction = MinorFunction;
    }

    __inline
    VOID
    SetParameterQueryInterfaceInterfaceSpecificData(
        __in PVOID InterfaceSpecificData
        )
    {
        this->GetNextIrpStackLocation()->
            Parameters.QueryInterface.InterfaceSpecificData = InterfaceSpecificData;
    }

    __inline
    VOID
    SetCompletionRoutineEx(
        __in MdDeviceObject DeviceObject,
        __in MdCompletionRoutine CompletionRoutine,
        __in PVOID Context,
        __in BOOLEAN InvokeOnSuccess = TRUE,
        __in BOOLEAN InvokeOnError = TRUE,
        __in BOOLEAN InvokeOnCancel = TRUE
        )
    {
        if (!NT_SUCCESS(IoSetCompletionRoutineEx(
                DeviceObject,
                m_Irp,
                CompletionRoutine,
                Context,
                InvokeOnSuccess,
                InvokeOnError,
                InvokeOnCancel)))
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
    }

    static
    __inline
    NTSTATUS
    FxIrp::_IrpSynchronousCompletion(
        __in MdDeviceObject DeviceObject,
        __in PIRP OriginalIrp,
        __in PVOID Context
        )
    {
        FxCREvent* event = (FxCREvent*) Context;

        UNREFERENCED_PARAMETER(DeviceObject);

        if (OriginalIrp->PendingReturned)
        {
            //
            // No need to propagate the pending returned bit since we are handling
            // the request synchronously
            //
            event->Set();
        }

        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    __inline
    VOID
    SetCompletionRoutine(
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
    NTSTATUS
    SendIrpSynchronously(
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

        if (status == STATUS_PENDING)
        {
            event.EnterCRAndWaitAndLeave();
            status = m_Irp->IoStatus.Status;
        }

        return status;
    }

    __inline
    VOID
    SkipCurrentIrpStackLocation(
        VOID
        )
    {
        IoSkipCurrentIrpStackLocation(m_Irp);
    }

    __inline
    VOID
    SetInformation(
        ULONG_PTR Information
        )
    {
        m_Irp->IoStatus.Information = Information;
    }

    __inline
    PVOID
    GetDriverContext(
        VOID
        )
    {
        return m_Irp->Tail.Overlay.DriverContext;
    }

    __inline
    ULONG
    GetDriverContextSize(
        VOID
        )
    {
        return sizeof(m_Irp->Tail.Overlay.DriverContext);
    }

    __inline
    PIO_STATUS_BLOCK
    GetStatusBlock(
        VOID
        )
    {
        return &m_Irp->IoStatus;
    }

    __inline
    VOID
    CopyStatus(
        _Out_ PIO_STATUS_BLOCK StatusBlock
        )
    {
        RtlCopyMemory(StatusBlock,
                      GetStatusBlock(),
                      sizeof(*StatusBlock));
    }

    __inline
    VOID
    MarkIrpPending(
        )
    {
        IoMarkIrpPending(m_Irp);
    }

    __inline
    ULONG_PTR
    GetInformation(
        )
    {
        return m_Irp->IoStatus.Information;
    }

    __inline
    VOID
    SetContext(
        __in ULONG Index,
        __in PVOID Value
        )
    {
        m_Irp->Tail.Overlay.DriverContext[Index] = Value;
    }

    __inline
    PLIST_ENTRY
    ListEntry(
        )
    {
        return &m_Irp->Tail.Overlay.ListEntry;
    }

    __inline
    MdCancelRoutine 
    SetCancelRoutine(
        __in_opt MdCancelRoutine  CancelRoutine
        )
    {
        return IoSetCancelRoutine(m_Irp, CancelRoutine);
    }

    __inline
    BOOLEAN
    IsCanceled(
        )
    {
        return m_Irp->Cancel ? TRUE : FALSE;
    }

    __inline
    KIRQL
    GetCancelIrql(
        )
    {
        return m_Irp->CancelIrql;
    }

    __inline
    PVOID
    GetContext(
        __in ULONG Index
        )
    {
        return m_Irp->Tail.Overlay.DriverContext[Index];
    }

    __inline
    MdFileObject
    GetFileObject(
        VOID
        )
    {
        return this->GetCurrentIrpStackLocation()->FileObject;
    }

    __inline
    ULONG
    GetParameterReadLength(
        VOID
        )
    {
      return this->GetCurrentIrpStackLocation()->Parameters.Read.Length;
    }

    __inline
    ULONG
    GetParameterWriteLength(
        VOID
        )
    {
      return this->GetCurrentIrpStackLocation()->Parameters.Write.Length;
    }

    __inline
    ULONG
    GetParameterIoctlOutputBufferLength(
        VOID
        )
    {
        return this->GetCurrentIrpStackLocation()->
            Parameters.DeviceIoControl.OutputBufferLength;
    }

    __inline
    ULONG
    GetParameterIoctlInputBufferLength(
        VOID
        )
    {
        return this->GetCurrentIrpStackLocation()->
            Parameters.DeviceIoControl.InputBufferLength;
    }

    __inline
    ULONG
    GetParameterIoctlCode(
        VOID
        )
    {
        return this->GetCurrentIrpStackLocation()->
            Parameters.DeviceIoControl.IoControlCode;
    }

    __inline
    static
    MdIrp
    GetIrpFromListEntry(
        __in PLIST_ENTRY Ple
        )
    {
        return CONTAINING_RECORD(Ple, IRP, Tail.Overlay.ListEntry);
    }

    __inline
    KPROCESSOR_MODE
    GetRequestorMode(
        VOID
        )
    {
        return m_Irp->RequestorMode;
    }

    __inline
    PVOID
    GetUserBuffer(
        )
    {
        return m_Irp->UserBuffer;
    }

    __inline
    PVOID
    GetSystemBuffer(
        )
    {
        return m_Irp->AssociatedIrp.SystemBuffer;
    }

    __inline
    PVOID
    GetParameterIoctlType3InputBuffer(
        VOID
        )
    {
        return this->GetCurrentIrpStackLocation()->
                   Parameters.DeviceIoControl.Type3InputBuffer;
    }

    __inline
    PVOID
    GetOutputBuffer(
        )
    {
        //
        // In kernel mode, for buffered I/O, the output and input buffers are 
        // at same location.
        //
        return GetSystemBuffer();
    }

    __inline
    ULONG
    GetParameterIoctlCodeBufferMethod(
        VOID
        )
    {
        return METHOD_FROM_CTL_CODE(GetParameterIoctlCode());
    }

    __inline
    BOOLEAN
    PendingReturned(
        )
    {
        return m_Irp->PendingReturned;
    }

    __inline
    VOID
    PropagatePendingReturned(
        VOID
        )
    {
        if (PendingReturned() && m_Irp->CurrentLocation <= m_Irp->StackCount)
        {
            MarkIrpPending();
        }
    }

    __inline
    VOID
    FreeIrp(
        VOID
        )
    {
        IoFreeIrp(m_Irp);
    }

    __inline
    VOID
    SetFlags(
        __in ULONG Flags
        )
    {
        m_Irp->Flags = Flags;
    }

    __inline
    ULONG
    GetFlags(
        VOID
        )
    {
        return m_Irp->Flags;
    }

    __inline
    MdDeviceObject
    GetDeviceObject(
        VOID
        )
    {
      return this->GetCurrentIrpStackLocation()->DeviceObject;
    }

    __inline
    POWER_ACTION
    GetParameterPowerShutdownType(
        )
    {
        return (this->GetCurrentIrpStackLocation())->
            Parameters.Power.ShutdownType;
    }

    __inline
    PCM_RESOURCE_LIST
    GetParameterAllocatedResources(
        )
    {
        return this->GetCurrentIrpStackLocation()->
            Parameters.StartDevice.AllocatedResources;
    }

    __inline
    PCM_RESOURCE_LIST
    GetParameterAllocatedResourcesTranslated(
        )
    {
        return this->GetCurrentIrpStackLocation()->
            Parameters.StartDevice.AllocatedResourcesTranslated;
    }

    __inline
    VOID
    SetParameterAllocatedResources(
        __in PCM_RESOURCE_LIST AllocatedResources
        )
    {
        this->GetNextIrpStackLocation()->
            Parameters.StartDevice.AllocatedResources = AllocatedResources;
    }

    __inline
    VOID
    SetParameterAllocatedResourcesTranslated(
        __in PCM_RESOURCE_LIST AllocatedResourcesTranslated
        )
    {
        this->GetNextIrpStackLocation()->
            Parameters.StartDevice.AllocatedResourcesTranslated = 
                AllocatedResourcesTranslated;
    }

    __inline
    VOID
    SetParameterDeviceCapabilities(
        __in PDEVICE_CAPABILITIES DeviceCapabilities
        )
    {
        this->GetNextIrpStackLocation()->
            Parameters.DeviceCapabilities.Capabilities = DeviceCapabilities;
    }

    __inline
    PDEVICE_CAPABILITIES
    GetParameterDeviceCapabilities(
        )
    {
        return this->GetCurrentIrpStackLocation()->
            Parameters.DeviceCapabilities.Capabilities;
    }

    __inline
    PMDL
    GetMdl(
        )
    {
        return m_Irp->MdlAddress;
    }

    __inline
    CCHAR
    GetCurrentIrpStackLocationIndex(
        )
    {
        return m_Irp->CurrentLocation;
    }

    __inline
    CCHAR
    GetStackCount(
        )
    {
        return m_Irp->StackCount;
    }

    __inline
    BOOLEAN
    IsCurrentIrpStackLocationValid(
        VOID
        )
    {
        return (GetCurrentIrpStackLocationIndex() <= GetStackCount());
    }

    #if (NTDDI_VERSION >= NTDDI_VISTA) 
    __inline
    SYSTEM_POWER_STATE_CONTEXT
    GetParameterPowerSystemPowerStateContext(
        )
    {
        return (this->GetCurrentIrpStackLocation())->
            Parameters.Power.SystemPowerStateContext;
    }
    #endif

    _Must_inspect_result_
    __inline
    static
    NTSTATUS
    RequestPowerIrp(
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
    PVOID
    GetCurrentParametersPointer(
        VOID
        )
    {
        return &(this->GetCurrentIrpStackLocation())->Parameters;
    }

    __inline
    VOID
    CopyParameters(
        _Out_ PWDF_REQUEST_PARAMETERS Parameters
        )
    {
        RtlMoveMemory(&Parameters->Parameters,
                      GetCurrentParametersPointer(),
                      sizeof(Parameters->Parameters));
    }

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

    
    __inline
    ~FxAutoIrp()
    {
        if (m_Irp != NULL)
        {
            IoFreeIrp(m_Irp);
        }
    }    
};

#endif //_FXIRPKM_H_