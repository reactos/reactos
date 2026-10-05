/*
 * PROJECT:     ReactOS USB Host Controller Extension
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Controller reset machine actions and the device fan out around a reset
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "ucx01000.h"

#define NDEBUG
#include <debug.h>

/** Endpoints collected under the topology lock, posted to after it is dropped. */
struct UcxResetFanOut
{
    LIST_ENTRY Endpoints;
    ULONG Count;
    BOOLEAN MarkDeprogrammed;
};

static
VOID
NTAPI
UcxCollectEndpointsForReset(
    _In_ UcxUsbDevice* Device,
    _In_opt_ PVOID Context)
{
    UcxResetFanOut* FanOut = (UcxResetFanOut*)Context;
    PLIST_ENTRY Entry;
    UcxEndpoint* Endpoint;

    /* Root hub endpoints have no machine and the root hub keeps its programming */
    if (Device->IsRootHub())
        return;

    if (FanOut->MarkDeprogrammed)
        Device->m_DeprogrammedByControllerReset = TRUE;

    for (Entry = Device->m_EndpointList.Flink;
         Entry != &Device->m_EndpointList;
         Entry = Entry->Flink)
    {
        Endpoint = CONTAINING_RECORD(Entry, UcxEndpoint, m_DeviceLink);

        NT_ASSERT(!UcxIsListEntryLinked(&Endpoint->m_ControllerResetLink));
        WdfObjectReference(Endpoint->m_Handle);
        InsertTailList(&FanOut->Endpoints, &Endpoint->m_ControllerResetLink);
        FanOut->Count++;
    }
}

static
VOID
NTAPI
UcxPostToCollectedEndpoints(
    _Inout_ UcxResetFanOut* FanOut,
    _In_ EpEvent Event)
{
    PLIST_ENTRY Entry;
    UcxEndpoint* Endpoint;

    while (!IsListEmpty(&FanOut->Endpoints))
    {
        Entry = RemoveHeadList(&FanOut->Endpoints);
        Endpoint = CONTAINING_RECORD(Entry, UcxEndpoint, m_ControllerResetLink);
        UcxClearListEntry(Entry);

        Endpoint->m_Machine.SmPost(Event);
        WdfObjectDereference(Endpoint->m_Handle);
    }
}

VOID
UcxController::WalkDevices(
    _In_ PFN_UCX_DEVICE_VISITOR Visitor,
    _In_opt_ PVOID Context,
    _In_ BOOLEAN IncludeDisconnected)
{
    SpinLockGuard Guard(&m_TopologyLock);

    m_RootHub->Device()->WalkSubtree(Visitor, Context, IncludeDisconnected);
}

/** Warns every endpoint of the reset; each acknowledges once idle. */
VOID
UcxController::PrepareDevicesForReset()
{
    UcxResetFanOut FanOut;

    InitializeListHead(&FanOut.Endpoints);
    FanOut.Count = 0;
    FanOut.MarkDeprogrammed = TRUE;

    WalkDevices(UcxCollectEndpointsForReset, &FanOut, TRUE);

    /* Set before the first post so a synchronous ack cannot underflow it */
    NT_ASSERT(m_PendingPrepareForReset == 0);
    m_PendingPrepareForReset = FanOut.Count;

    DPRINT("Controller %p preparing %lu endpoints for reset\n", this, FanOut.Count);

    UcxPostToCollectedEndpoints(&FanOut, EpEvent::ControllerResetStarting);

    if (FanOut.Count == 0)
        PostResetEvent(CrEvent::DevicesReady);
}

/* Notifies whatever endpoints exist now, not only the ones that were prepared */
VOID
UcxController::NotifyDevicesResetDone()
{
    UcxResetFanOut FanOut;

    InitializeListHead(&FanOut.Endpoints);
    FanOut.Count = 0;
    FanOut.MarkDeprogrammed = FALSE;

    WalkDevices(UcxCollectEndpointsForReset, &FanOut, TRUE);
    DPRINT("Controller %p reset done, notifying %lu endpoints\n", this, FanOut.Count);
    UcxPostToCollectedEndpoints(&FanOut, EpEvent::ControllerResetDone);
}

VOID
UcxController::PrepareForResetOperationDone()
{
    if (InterlockedDecrement(&m_PendingPrepareForReset) == 0)
        PostResetEvent(CrEvent::DevicesReady);
}

VOID
NTAPI
UcxController::ResetMachinePassiveRoutine(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_opt_ PVOID Context,
    _In_ PUCXHUB_WORKITEM WorkItem)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(WorkItem);

    ((UcxController*)Context)->m_ResetMachine.SmContinueOnPassive();
}

/* Controller reset machine actions */

VOID
ControllerResetMachine::ReferenceController()
{
    WdfObjectReference(m_Controller->m_Handle);
}

VOID
ControllerResetMachine::DereferenceController()
{
    WdfObjectDereference(m_Controller->m_Handle);
}

VOID
ControllerResetMachine::QueuePassiveWork()
{
    m_Controller->m_ResetMachineWorkItem->Enqueue(UcxController::ResetMachinePassiveRoutine,
                                                  m_Controller,
                                                  UcxHubWorkItemDefault);
}

VOID
ControllerResetMachine::HoldDeviceRequests()
{
    WdfIoQueueStop(m_Controller->m_PendDuringResetQueue, NULL, NULL);
}

VOID
ControllerResetMachine::ReleaseDeviceRequests()
{
    WdfIoQueueStart(m_Controller->m_PendDuringResetQueue);
}

VOID
ControllerResetMachine::MarkResetInProgress()
{
    m_Controller->MarkResetInProgress();
}

VOID
ControllerResetMachine::MarkResetFinished()
{
    m_Controller->MarkResetFinished();
}

VOID
ControllerResetMachine::FailRootHubIo()
{
    m_Controller->m_RootHub->FailIo();
}

VOID
ControllerResetMachine::UnblockRootHubTraffic()
{
    m_Controller->m_RootHub->ResumeIo();
}

VOID
ControllerResetMachine::DropResetBlock()
{
    m_Controller->UnblockReset();
}

VOID
ControllerResetMachine::PrepareDevicesForReset()
{
    m_Controller->PrepareDevicesForReset();
}

VOID
ControllerResetMachine::NotifyDevicesResetDone()
{
    m_Controller->NotifyDevicesResetDone();
}

/* The HCD reports back through UcxControllerResetComplete, maybe from inside this call */
VOID
ControllerResetMachine::ResetController()
{
    DPRINT1("Controller %p being reset\n", m_Controller);
    m_Controller->m_Config.EvtControllerReset(m_Controller->m_Handle);
}

VOID
ControllerResetMachine::CompleteHubReset(
    _In_ BOOLEAN Succeeded)
{
    DPRINT("Controller %p completing hub reset request, success %u\n", m_Controller, Succeeded);
    m_Controller->m_RootHub->FinishPortResetRequest(Succeeded);
}

VOID
ControllerResetMachine::SignalPortChange()
{
    m_Controller->m_RootHub->PortChanged();
}

VOID
ControllerResetMachine::AllowRootHubPowerDown()
{
    m_Controller->m_WaitOnResetComplete = TRUE;
    KeSetEvent(&m_Controller->m_RootHubMayExitD0, IO_NO_INCREMENT, FALSE);
}

VOID
ControllerResetMachine::LetResetCompletionProceed()
{
    m_Controller->m_WaitOnResetComplete = FALSE;
}

VOID
ControllerResetMachine::ReleaseResetCompleteWaiter()
{
    KeSetEvent(&m_Controller->m_ResetCompleteProcessed, IO_NO_INCREMENT, FALSE);
}
