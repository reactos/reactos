/*
 * PROJECT:     ReactOS USB Host Controller Extension
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     UCXROOTHUB object and its two virtual endpoints
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "ucx01000.h"

#define NDEBUG
#include <debug.h>

UcxRootHub*
UcxRootHub::FromHandle(
    _In_ UCXROOTHUB Handle)
{
    return UcxGetRootHubContext(Handle);
}

_Must_inspect_result_
NTSTATUS
UcxRootHub::Create(
    _In_ UCXCONTROLLER Controller,
    _In_ PUCX_ROOTHUB_CONFIG Config,
    _In_opt_ PWDF_OBJECT_ATTRIBUTES Attributes,
    _Out_ UCXROOTHUB* RootHub)
{
    WDF_OBJECT_ATTRIBUTES UcxAttributes;
    WDF_OBJECT_ATTRIBUTES DeviceAttributes;
    UcxController* ControllerContext = UcxController::FromHandle(Controller);
    UcxRootHub* Context;
    PVOID DeviceContext;
    WDFOBJECT Object;
    NTSTATUS Status;

    PAGED_CODE();

    *RootHub = NULL;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&UcxAttributes, UcxRootHub);
    UcxAttributes.EvtCleanupCallback = UcxRootHub::EvtCleanup;

    if (Attributes != NULL)
        Attributes->ParentObject = Controller;
    else
        UcxAttributes.ParentObject = Controller;

    Status = UcxCreateObjectWithTwoContexts(Attributes != NULL ? Attributes : &UcxAttributes,
                                            Attributes != NULL ? &UcxAttributes : NULL,
                                            &Object);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Root hub object create failed 0x%lx\n", Status);
        return Status;
    }

    Context = new (UcxGetRootHubContext(Object)) UcxRootHub();
    Context->m_Handle = (UCXROOTHUB)Object;

    /* The same handle doubles as the root hub's UCXUSBDEVICE */
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&DeviceAttributes, UcxUsbDevice);
    Status = WdfObjectAllocateContext(Object, &DeviceAttributes, &DeviceContext);
    if (NT_SUCCESS(Status))
    {
        Context->m_Device = new (DeviceContext) UcxUsbDevice();
        Status = Context->Initialize(ControllerContext, Config);
    }
    else
    {
        DPRINT1("Root hub device context allocation failed 0x%lx\n", Status);
    }

    if (!NT_SUCCESS(Status))
    {
        WdfObjectDelete(Object);
        return Status;
    }

    {
        SpinLockGuard Guard(&ControllerContext->m_TopologyLock);

        ControllerContext->m_RootHub = Context;
        ControllerContext->m_ChildDeviceCount = 1;
    }

    *RootHub = Context->m_Handle;
    DPRINT("Root hub %p created for controller %p\n", Context, ControllerContext);
    return STATUS_SUCCESS;
}

NTSTATUS
UcxRootHub::Initialize(
    _In_ UcxController* Controller,
    _In_ PUCX_ROOTHUB_CONFIG Config)
{
    NTSTATUS Status;

    m_Controller = Controller;
    KeInitializeSpinLock(&m_PdoInfoLock);

    /* The HCD's size is trusted only as far as the structure we know */
    RtlZeroMemory(&m_Config, sizeof(m_Config));
    RtlCopyMemory(&m_Config, Config, min(Config->Size, (ULONG)sizeof(m_Config)));

    /* Speed is set first so the endpoints get the SuperSpeed transfer limits */
    m_Device->InitializeAsRootHub(m_Handle, Controller);

    Status = CreateEndpoint(FALSE);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = CreateEndpoint(TRUE);
    if (!NT_SUCCESS(Status))
        return Status;

    return CreatePdo();
}

/** One of the two endpoints the hub talks to; their queues live on the controller FDO. */
NTSTATUS
UcxRootHub::CreateEndpoint(
    _In_ BOOLEAN Interrupt)
{
    WDF_IO_QUEUE_FORWARD_PROGRESS_POLICY Policy;
    WDF_OBJECT_ATTRIBUTES QueueAttributes;
    WDF_IO_QUEUE_CONFIG QueueConfig;
    UcxEndpointInit Init;
    PUCXENDPOINT_INIT InitPointer = &Init;
    UCXENDPOINT EndpointHandle;
    WDFQUEUE Queue;
    NTSTATUS Status;

    if (Interrupt)
    {
        WDF_IO_QUEUE_CONFIG_INIT(&QueueConfig, WdfIoQueueDispatchSequential);
        QueueConfig.EvtIoInternalDeviceControl = UcxEvtRootHubInterruptTransfer;
        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&QueueAttributes, UcxInterruptQueueState);
    }
    else
    {
        WDF_IO_QUEUE_CONFIG_INIT(&QueueConfig, WdfIoQueueDispatchParallel);
        QueueConfig.Settings.Parallel.NumberOfPresentedRequests =
            m_Config.NumberOfPresentedControlUrbCallbacks;
        QueueConfig.EvtIoInternalDeviceControl = UcxEvtRootHubControlTransfer;
        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&QueueAttributes, UcxControlQueueContext);
    }
    QueueAttributes.ParentObject = m_Handle;

    Status = WdfIoQueueCreate(m_Controller->m_Fdo, &QueueConfig, &QueueAttributes, &Queue);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Root hub %s queue create failed 0x%lx\n", Interrupt ? "interrupt" : "control", Status);
        return Status;
    }

    WDF_IO_QUEUE_FORWARD_PROGRESS_POLICY_DEFAULT_INIT(&Policy, 1);
    Status = WdfIoQueueAssignForwardProgressPolicy(Queue, &Policy);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Root hub %s queue forward progress setup failed 0x%lx\n", Interrupt ? "interrupt" : "control", Status);
        return Status;
    }

    RtlZeroMemory(&Init, sizeof(Init));
    Init.Kind = Interrupt ? UcxEndpointKind::Generic : UcxEndpointKind::Default;
    Init.TransferType = Interrupt ? UcxTransferType::Interrupt : UcxTransferType::Control;
    Init.Device = (UCXUSBDEVICE)m_Handle;
    Init.CreateMachine = FALSE;

    Status = UcxEndpoint::Create((UCXUSBDEVICE)m_Handle, &InitPointer, NULL, &EndpointHandle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Root hub %s endpoint create failed 0x%lx\n", Interrupt ? "interrupt" : "control", Status);
        return Status;
    }

    UcxEndpoint::FromHandle(EndpointHandle)->SetWdfIoQueue(Queue);

    if (Interrupt)
    {
        UcxInterruptQueueState* State = UcxGetInterruptQueueState(Queue);

        KeInitializeSpinLock(&State->PortChangeLock);

        /* Different start values, so the very first transfer always reaches the HCD */
        State->PortChangeGeneration = 1;
        State->PortChangeGenerationProcessed = 0;
        KeInitializeEvent(&State->NoIndicateWakeInProgress, NotificationEvent, TRUE);
        State->RootHub = this;

        m_InterruptQueue = Queue;
        m_InterruptState = State;
        m_InterruptEndpoint = UcxEndpoint::FromHandle(EndpointHandle);
        DPRINT("Root hub %p interrupt endpoint %p\n", this, m_InterruptEndpoint);
    }
    else
    {
        UcxGetControlQueueContext(Queue)->RootHub = this;

        m_ControlQueue = Queue;
        m_ControlEndpoint = UcxEndpoint::FromHandle(EndpointHandle);
        DPRINT("Root hub %p control endpoint %p\n", this, m_ControlEndpoint);
    }

    return STATUS_SUCCESS;
}

VOID
UcxRootHub::EvtCleanup(
    _In_ WDFOBJECT Object)
{
    UcxRootHub* RootHub = UcxGetRootHubContext(Object);
    UcxController* Controller = RootHub->m_Controller;

    if (Controller == NULL)
        return;

    SpinLockGuard Guard(&Controller->m_TopologyLock);

    Controller->m_RootHub = NULL;
    Controller->m_ChildDeviceCount--;
}

/* Port changes and the status change transfer */

VOID
UcxRootHub::PortChanged()
{
    UcxInterruptQueueState* State = m_InterruptState;
    WDFREQUEST Release = NULL;
    BOOLEAN IndicateWake;

    DPRINT("Root hub %p port change\n", this);

    {
        SpinLockGuard Guard(&State->PortChangeLock);

        State->PortChangeGeneration++;

        /* If the cancel routine already owns the held transfer, it completes it */
        if (State->HeldTransfer != NULL &&
            NT_SUCCESS(WdfRequestUnmarkCancelable(State->HeldTransfer)))
        {
            Release = State->HeldTransfer;
            State->HeldTransfer = NULL;
            State->PortChangeGenerationProcessed = State->PortChangeGeneration;
        }

        IndicateWake = State->IndicateWakeEnabled;
        if (IndicateWake)
            KeClearEvent(&State->NoIndicateWakeInProgress);
    }

    if (IndicateWake)
    {
        /* Raised so a D0 IRP the wake triggers never runs inline on this thread */
        {
            DispatchLevelGuard Raise;

            WdfDeviceIndicateWakeStatus(m_Pdo, STATUS_SUCCESS);
        }
        KeSetEvent(&State->NoIndicateWakeInProgress, IO_NO_INCREMENT, FALSE);
    }

    if (Release == NULL)
        return;

    if (!m_Controller->BlockReset())
    {
        DPRINT1("Root hub %p failing held status change transfer %p, controller resetting\n", this, Release);
        WdfRequestComplete(Release, STATUS_NO_SUCH_DEVICE);
        return;
    }

    m_Config.EvtRootHubInterruptTx(m_Handle, Release);
    m_Controller->UnblockReset();
}

VOID
NTAPI
UcxEvtRootHubHeldTransferCancel(
    _In_ WDFREQUEST Request)
{
    UcxInterruptQueueState* State = UcxGetInterruptQueueState(WdfRequestGetIoQueue(Request));

    {
        SpinLockGuard Guard(&State->PortChangeLock);

        State->HeldTransfer = NULL;
        State->LastTransferCanceled = TRUE;
    }

    DPRINT("Held status change transfer %p canceled\n", Request);
    WdfRequestComplete(Request, STATUS_CANCELLED);
}

/* A transfer marked as nothing changed is held by UCX until a port changes */
VOID
NTAPI
UcxEvtRootHubInterruptTransfer(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode)
{
    UcxInterruptQueueState* State = UcxGetInterruptQueueState(Queue);
    UcxRootHub* RootHub = State->RootHub;
    PURB Urb = (PURB)UcxRequestArgs(Request).Arg1;
    NTSTATUS HoldStatus = STATUS_SUCCESS;
    BOOLEAN Hold;

    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(IoControlCode);

    NT_ASSERT(Urb->UrbHeader.Function == URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER);

    {
        SpinLockGuard Guard(&State->PortChangeLock);

        State->LastTransferCanceled = FALSE;

        Hold = (Urb->UrbHeader.UsbdFlags & UCXHUB_URB_FLAG_PEND_INTERRUPT_TX) != 0 &&
               State->PortChangeGeneration == State->PortChangeGenerationProcessed;

        if (!Hold)
        {
            State->PortChangeGenerationProcessed = State->PortChangeGeneration;
        }
        else
        {
            HoldStatus = WdfRequestMarkCancelableEx(Request, UcxEvtRootHubHeldTransferCancel);
            if (NT_SUCCESS(HoldStatus))
                State->HeldTransfer = Request;
        }
    }

    if (!NT_SUCCESS(HoldStatus))
    {
        DPRINT1("Root hub %p could not hold status change transfer %p 0x%lx\n", RootHub, Request, HoldStatus);
        WdfRequestComplete(Request, HoldStatus);
        return;
    }

    if (Hold)
        return;

    if (!RootHub->m_Controller->BlockReset())
    {
        DPRINT1("Root hub %p failing status change transfer %p, controller resetting\n", RootHub, Request);
        WdfRequestComplete(Request, STATUS_NO_SUCH_DEVICE);
        return;
    }

    RootHub->m_Config.EvtRootHubInterruptTx(RootHub->m_Handle, Request);
    RootHub->m_Controller->UnblockReset();
}

/* Hub class requests to the root hub */

/* bmRequestType decoding: recipient bits 0 and 1 only, type bits 5 and 6, direction bit 7 */
#define UCX_RECIPIENT(Type)      ((Type) & 0x03)
#define UCX_REQUEST_TYPE(Type)   (((Type) >> 5) & 0x03)
#define UCX_DIRECTION_IN(Type)   (((Type) & 0x80) != 0)

#define UCX_RECIPIENT_DEVICE     0
#define UCX_RECIPIENT_OTHER      3
#define UCX_TYPE_STANDARD        0
#define UCX_TYPE_CLASS           1

/** TRUE for the seven hub requests with their own callback slot, even if empty. */
static
BOOLEAN
NTAPI
UcxClassifyRootHubRequest(
    _In_ const UCX_ROOTHUB_CONFIG* Config,
    _In_ const USB_DEFAULT_PIPE_SETUP_PACKET* Setup,
    _Out_ PFN_UCX_ROOTHUB_CONTROL_URB* Callback)
{
    UCHAR Recipient = UCX_RECIPIENT(Setup->bmRequestType.B);
    UCHAR Type = UCX_REQUEST_TYPE(Setup->bmRequestType.B);
    BOOLEAN In = UCX_DIRECTION_IN(Setup->bmRequestType.B);
    BOOLEAN ToDevice = (Recipient == UCX_RECIPIENT_DEVICE);
    BOOLEAN ToPort = (Recipient == UCX_RECIPIENT_OTHER);

    *Callback = NULL;

    if (Type == UCX_TYPE_CLASS && (ToDevice || ToPort))
    {
        switch (Setup->bRequest)
        {
            case USB_REQUEST_CLEAR_FEATURE:
                if (In)
                    break;
                *Callback = ToDevice ? Config->EvtRootHubClearHubFeature
                                     : Config->EvtRootHubClearPortFeature;
                return TRUE;

            case USB_REQUEST_GET_STATUS:
                if (!In)
                    break;
                *Callback = ToDevice ? Config->EvtRootHubGetHubStatus
                                     : Config->EvtRootHubGetPortStatus;
                return TRUE;

            case USB_REQUEST_SET_FEATURE:
                if (In)
                    break;
                *Callback = ToDevice ? Config->EvtRootHubSetHubFeature
                                     : Config->EvtRootHubSetPortFeature;
                return TRUE;
        }
    }

    /* Matched as a standard device request, not the hub class form */
    if (Type == UCX_TYPE_STANDARD && ToDevice && In &&
        Setup->bRequest == USB_REQUEST_GET_PORT_ERR_COUNT)
    {
        *Callback = Config->EvtRootHubGetPortErrorCount;
        return TRUE;
    }

    return FALSE;
}

VOID
NTAPI
UcxEvtRootHubControlTransfer(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode)
{
    UcxRootHub* RootHub = UcxGetControlQueueContext(Queue)->RootHub;
    PURB Urb = (PURB)UcxRequestArgs(Request).Arg1;
    PUSB_DEFAULT_PIPE_SETUP_PACKET Setup;
    PFN_UCX_ROOTHUB_CONTROL_URB Callback;

    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(IoControlCode);

    if (!RootHub->m_Controller->BlockReset())
    {
        DPRINT1("Root hub %p failing control transfer %p, controller resetting\n", RootHub, Request);
        WdfRequestComplete(Request, STATUS_NO_SUCH_DEVICE);
        return;
    }

    /* Same offset in the plain and the _EX control transfer URBs */
    Setup = (PUSB_DEFAULT_PIPE_SETUP_PACKET)Urb->UrbControlTransfer.SetupPacket;

    /* A recognized hub request with no callback is not offered to EvtRootHubControlUrb */
    if (!UcxClassifyRootHubRequest(&RootHub->m_Config, Setup, &Callback))
        Callback = RootHub->m_Config.EvtRootHubControlUrb;

    if (Callback != NULL)
        Callback(RootHub->m_Handle, Request);

    RootHub->m_Controller->UnblockReset();

    if (Callback == NULL)
    {
        DPRINT1("Root hub %p has no callback for request 0x%x type 0x%x\n",
                RootHub, Setup->bRequest, Setup->bmRequestType.B);
        NT_ASSERT(FALSE);
        WdfRequestComplete(Request, STATUS_NOT_SUPPORTED);
    }
}

/* Controller reset hooks */

/* Queued requests fail and new ones are refused until ResumeIo */
VOID
UcxRootHub::FailIo()
{
    DPRINT("Root hub %p failing I/O for controller reset\n", this);
    WdfIoQueuePurge(m_InterruptQueue, NULL, NULL);
    WdfIoQueuePurge(m_ControlQueue, NULL, NULL);
}

VOID
UcxRootHub::ResumeIo()
{
    {
        SpinLockGuard Guard(&m_InterruptState->PortChangeLock);

        /* The next status change transfer goes to the HCD even if flagged */
        m_InterruptState->PortChangeGeneration++;
    }

    DPRINT("Root hub %p resuming I/O\n", this);
    WdfIoQueueStart(m_InterruptQueue);
    WdfIoQueueStart(m_ControlQueue);
}

VOID
UcxRootHub::FinishPortResetRequest(
    _In_ BOOLEAN Succeeded)
{
    PIRP Irp = m_PendingAsyncReset;
    PUCXHUB_RESET_FLAGS Flags;

    m_PendingAsyncReset = NULL;

    Flags = (PUCXHUB_RESET_FLAGS)IoGetCurrentIrpStackLocation(Irp)->Parameters.Others.Argument1;
    Flags->AsUlong = 0;

    if (InterlockedExchange(&m_Controller->m_DeviceContextsLost, 0) != 0)
        Flags->HostContextLost = 1;

    m_Controller->m_RootHubResetSeen = 0;

    if (!Succeeded)
        DPRINT1("Root hub %p async reset IRP %p failed\n", this, Irp);

    Irp->IoStatus.Status = Succeeded ? STATUS_SUCCESS : STATUS_NO_SUCH_DEVICE;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}

/* ROOTHUB_GET_INFO, called with a reset reference held */
VOID
UcxRootHub::DispatchGetInfo(
    _In_ WDFREQUEST Request,
    _Inout_ PUCXHUB_ROOTHUB_INFO Info)
{
    /* The HCD only knows ROOTHUB_INFO; the completion routine restores the size */
    Info->Info.Size = sizeof(Info->Info);
    Info->InterruptPipe = m_InterruptEndpoint->m_Pipe.Handle();

    DPRINT("Root hub %p GetInfo request %p to HCD\n", this, Request);
    m_Config.EvtRootHubGetInfo(m_Handle, Request);
}

/** Hands out the root hub's interface name with a reference the caller drops. */
NTSTATUS
UcxRootHub::ReferenceSymbolicName(
    _Out_ WDFSTRING* Name)
{
    SpinLockGuard Guard(&m_PdoInfoLock);

    if (!m_PdoStarted || m_SymbolicName == NULL)
    {
        *Name = NULL;
        return STATUS_UNSUCCESSFUL;
    }

    WdfObjectReferenceWithTag(m_SymbolicName, (PVOID)UCX_POOL_TAG);
    *Name = m_SymbolicName;
    return STATUS_SUCCESS;
}
