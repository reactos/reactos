/*
 * PROJECT:     ReactOS SuperSpeed USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Idle notification IRP of a child PDO and the idle machine actions
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "usbhub3.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

/* The machine lock serializes everything, so the cancel spin lock is let go at once */
static
VOID
NTAPI
HubIdleCancel(
    _Inout_ PDEVICE_OBJECT DeviceObject,
    _Inout_ _IRQL_uses_cancel_ PIRP Irp)
{
    HubPdo* Pdo = HubGetPdoContext(WdfWdmDeviceGetWdfDeviceHandle(DeviceObject));

    IoReleaseCancelSpinLock(Irp->CancelIrql);
    DPRINT("Idle IRP %p on port %u canceled\n", Irp, Pdo->m_PortNumber);
    Pdo->m_Idle.Post(IdleEvent::RequestCanceled, NULL);
}

/* The client callback runs on this thread, at passive level */
static
VOID
NTAPI
HubIdleWorkItem(
    _In_ WDFWORKITEM WorkItem)
{
    HubPdo* Pdo = HubGetPdoContext(WdfWorkItemGetParentObject(WorkItem));

    Pdo->m_Idle.Post(IdleEvent::WorkItemRan, NULL);
}

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
HubPdoCreateIdleWorkItem(
    _In_ HubPdo* Pdo)
{
    WDF_WORKITEM_CONFIG Config;
    WDF_OBJECT_ATTRIBUTES Attributes;

    PAGED_CODE();

    WDF_WORKITEM_CONFIG_INIT(&Config, HubIdleWorkItem);
    Config.AutomaticSerialization = FALSE;

    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
    Attributes.ParentObject = Pdo->m_Device;

    return WdfWorkItemCreate(&Config, &Attributes, &Pdo->m_IdleWorkItem);
}

/* After a post the IRP may already be completed; only the returned status is valid */
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
NTAPI
HubPdoSubmitIdleRequest(
    _In_ HubPdo* Pdo,
    _In_ PIRP Irp)
{
    PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
    PUSB_IDLE_CALLBACK_INFO Info = (PUSB_IDLE_CALLBACK_INFO)Stack->Parameters.DeviceIoControl.Type3InputBuffer;

    if (Info == NULL || Info->IdleCallback == NULL)
    {
        DPRINT1("Idle IRP %p on port %u has no callback\n", Irp, Pdo->m_PortNumber);
        Irp->IoStatus.Status = STATUS_NO_CALLBACK_ACTIVE;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_NO_CALLBACK_ACTIVE;
    }

    DPRINT("Idle IRP %p submitted on port %u\n", Irp, Pdo->m_PortNumber);
    return Pdo->m_Idle.Post(IdleEvent::RequestSubmitted, Irp);
}

/* Idle machine actions */

BOOLEAN
IdleMachine::SelectiveSuspendSupported()
{
    if (m_Pdo->m_Hub->HasFlag(HubFlag::ParentNoSelectiveSuspend))
    {
        DPRINT("Port %u: parent stack has no selective suspend\n", m_Pdo->m_PortNumber);
        return FALSE;
    }

    return TRUE;
}

VOID
IdleMachine::MarkRequestPending(
    _In_ PIRP Irp)
{
    IoMarkIrpPending(Irp);
}

/* FALSE when the IRP was canceled before and the hub took it back */
BOOLEAN
IdleMachine::ArmCancel(
    _In_ PIRP Irp)
{
    IoSetCancelRoutine(Irp, HubIdleCancel);

    if (!Irp->Cancel)
        return TRUE;

    /* NULL here means the cancel routine already owns it and will post */
    if (IoSetCancelRoutine(Irp, NULL) == NULL)
        return TRUE;

    DPRINT("Idle IRP %p was canceled before it was held\n", Irp);
    return FALSE;
}

/* FALSE when the cancel routine is running or about to */
BOOLEAN
IdleMachine::DisarmCancel(
    _In_ PIRP Irp)
{
    return IoSetCancelRoutine(Irp, NULL) != NULL;
}

/* Enqueuing an already queued work item does nothing */
VOID
IdleMachine::QueueIdleWorkItem()
{
    WdfWorkItemEnqueue(m_Pdo->m_IdleWorkItem);
}

VOID
IdleMachine::InvokeIdleCallback(
    _In_ PIRP Irp)
{
    PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
    PUSB_IDLE_CALLBACK_INFO Info = (PUSB_IDLE_CALLBACK_INFO)Stack->Parameters.DeviceIoControl.Type3InputBuffer;

    DPRINT("Calling idle callback of port %u\n", m_Pdo->m_PortNumber);
    Info->IdleCallback(Info->IdleContext);
}

/* Cancel and power state completions are the normal ends of an idle IRP */
VOID
IdleMachine::CompleteIdleRequest(
    _In_ PIRP Irp,
    _In_ NTSTATUS Status)
{
    if (NT_SUCCESS(Status) || Status == STATUS_CANCELLED || Status == STATUS_POWER_STATE_INVALID)
        DPRINT("Idle IRP %p on port %u completed 0x%lx\n", Irp, m_Pdo->m_PortNumber, Status);
    else
        DPRINT1("Idle IRP %p on port %u failed 0x%lx\n", Irp, m_Pdo->m_PortNumber, Status);

    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}
