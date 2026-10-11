/*
 * PROJECT:     ReactOS SuperSpeed USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Fan out of hub events to the port and device machines
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "usbhub3.h"

#define NDEBUG
#include <debug.h>

/*
 * Every fan out sets its counter before the first post, so an acknowledgement
 * that comes back inside the post cannot reach zero early.
 */

/* Reference tags; only their addresses matter */
static const char HubTagRegistration[] = "hub child registration";
static const char HubTagPnpPower[] = "hub child pnp power";

/* Root ports report depth 0 or nonsense above 5; both mean the deepest tier */
static
ULONG
NTAPI
HubClampTierDepth(
    _In_ ULONG Depth)
{
    return (Depth == 0 || Depth > 5) ? 5 : Depth;
}

VOID
HubFdo::InitializeMux()
{
    RtlZeroMemory(&m_Mux, sizeof(m_Mux));
    KeInitializeSpinLock(&m_Mux.Lock);
    InitializeListHead(&m_Mux.Ports);
    InitializeListHead(&m_Mux.Devices);
}

HubPort*
HubFdo::FindPort(
    _In_ ULONG PortNumber)
{
    PLIST_ENTRY Entry;
    HubPort* Port;

    /* The list only changes in CreateChildPorts */
    for (Entry = m_Mux.Ports.Flink; Entry != &m_Mux.Ports; Entry = Entry->Flink)
    {
        Port = CONTAINING_RECORD(Entry, HubPort, m_HubLink);
        if (Port->Number() == PortNumber)
            return Port;
    }

    return NULL;
}

/** Adds one port to the end of the list; 2.0 ports are created before 3.0 ports. */
static
BOOLEAN
NTAPI
HubAddPort(
    _In_ HubFdo* Hub,
    _In_ const HubPortInfo* Info)
{
    HubPort* Port = HubPort::Create(Hub, Info);

    if (Port == NULL)
    {
        DPRINT1("Hub %p port %u create failed\n", Hub->m_Device, Info->PortNumber);
        return FALSE;
    }

    DPRINT("Hub %p created port %u (%p), protocol 0x%x\n",
           Hub->m_Device,
           Info->PortNumber,
           Port,
           Info->Protocol);

    InsertTailList(&Hub->m_Mux.Ports, &Port->m_HubLink);
    Hub->m_Mux.PortCount++;
    return TRUE;
}

static
VOID
NTAPI
HubTrackPortRange(
    _In_ USHORT PortNumber,
    _Inout_ PUSHORT First,
    _Inout_ PUSHORT Last)
{
    if (PortNumber < *First)
        *First = PortNumber;
    if (PortNumber > *Last)
        *Last = PortNumber;
}

/* The DeviceRemovable bitmap of a hub descriptor; bit n is port n */
static
BOOLEAN
NTAPI
HubDescriptorSaysFixed(
    _In_ const UCHAR* Bitmap,
    _In_ ULONG PortNumber)
{
    return (Bitmap[PortNumber / 8] & (1 << (PortNumber % 8))) != 0;
}

_IRQL_requires_(PASSIVE_LEVEL)
BOOLEAN
HubFdo::CreateChildPorts()
{
    HubPortInfo Info;
    ULONG Count20 = IsRootHub() ? m_RootHubInfo.Info.NumberOf20Ports : 0;
    ULONG Count30 = IsRootHub() ? m_RootHubInfo.Info.NumberOf30Ports : 0;
    ULONG BitmapBytes;
    ULONG Index;
    PLIST_ENTRY Entry;
    PVOID Buffer;
    WDF_OBJECT_ATTRIBUTES Attributes;
    NTSTATUS Status;

    PAGED_CODE();

    if (!IsRootHub())
    {
        if (m_Parent.HubSpeed == UsbSuperSpeed)
            Count30 = m_HubDescriptor.Usb30.bNumberOfPorts;
        else
            Count20 = m_HubDescriptor.Usb20.bNumberOfPorts;
    }

    m_First20Port = 0xFFFF;
    m_Last20Port = 0;

    for (Index = 0; Index < Count20; Index++)
    {
        RtlZeroMemory(&Info, sizeof(Info));
        Info.Properties = (ULONG)PortProperty::Removable;
        Info.Protocol = 0x200;

        if (IsRootHub())
        {
            const ROOTHUB_20PORT_INFO* Root = m_Root20Ports[Index];

            Info.PortNumber = Root->PortNumber;
            if (Root->Removable == TriStateFalse)
                Info.Properties &= ~(ULONG)PortProperty::Removable;
            if (Root->IntegratedHubImplemented == TriStateTrue)
                Info.Properties |= (ULONG)PortProperty::IntegratedHub;
            if (Root->ControllerUsb20HardwareLpmFlags.Flags.L1CapabilitySupported)
                Info.Properties |= (ULONG)PortProperty::Usb20LpmCapable;
            if (Root->ControllerUsb20HardwareLpmFlags.Flags.BeslLpmCapabilitySupported)
                Info.Properties |= (ULONG)PortProperty::BeslCapable;
            Info.TotalHubDepth = HubClampTierDepth(Root->HubDepth);
        }
        else
        {
            Info.PortNumber = (USHORT)(Index + 1);
            if (HubDescriptorSaysFixed(&m_HubDescriptor.Usb20.bRemoveAndPowerMask[0], Info.PortNumber))
                Info.Properties &= ~(ULONG)PortProperty::Removable;
            Info.TotalHubDepth = m_ParentInfo.TotalHubDepth;
        }

        HubTrackPortRange(Info.PortNumber, &m_First20Port, &m_Last20Port);
        if (!HubAddPort(this, &Info))
            goto Cleanup;
    }

    m_First30Port = 0xFFFF;
    m_Last30Port = 0;

    for (Index = 0; Index < Count30; Index++)
    {
        RtlZeroMemory(&Info, sizeof(Info));
        Info.Properties = (ULONG)PortProperty::Removable;
        Info.Protocol = 0x300;

        if (IsRootHub())
        {
            const ROOTHUB_30PORT_INFO_EX* Root = m_Root30Ports[Index];

            Info.PortNumber = Root->Info.PortNumber;
            if (Root->Info.Removable == TriStateFalse)
                Info.Properties &= ~(ULONG)PortProperty::Removable;
            if (Root->Info.DebugCapable == TriStateTrue)
                Info.Properties |= (ULONG)PortProperty::DebugCapable;
            Info.Properties |= (ULONG)PortProperty::EnhancedSuperSpeed;
            if (Root->Info.MinorRevision != 0)
                Info.SspIsochBurstCount = m_SspIsochBurstCount;
            Info.SublinkSpeedAttr = Root->Speeds;
            Info.SublinkSpeedAttrCount = Root->SpeedsCount;
            Info.TotalHubDepth = HubClampTierDepth(Root->Info.HubDepth);
        }
        else
        {
            Info.PortNumber = (USHORT)(Index + 1);
            if (HubDescriptorSaysFixed((const UCHAR*)&m_HubDescriptor.Usb30.DeviceRemovable, Info.PortNumber))
                Info.Properties &= ~(ULONG)PortProperty::Removable;
            if (m_Parent.IsEnhancedSuperSpeed)
            {
                Info.Properties |= (ULONG)PortProperty::EnhancedSuperSpeed;
                Info.SspIsochBurstCount = m_SspIsochBurstCount;
            }
            Info.SublinkSpeedAttr = m_ParentInfo.SublinkSpeedAttr;
            Info.SublinkSpeedAttrCount = m_ParentInfo.SublinkSpeedAttrCount;
            Info.TotalHubDepth = m_ParentInfo.TotalHubDepth;
        }

        HubTrackPortRange(Info.PortNumber, &m_First30Port, &m_Last30Port);
        if (!HubAddPort(this, &Info))
            goto Cleanup;
    }

    m_PortCount = max(m_Last20Port, m_Last30Port);
    if (m_PortCount == 0)
        DPRINT1("Hub %p reports no ports\n", m_Device);
    NT_ASSERT(m_PortCount != 0);

    /* External hubs read the whole interrupt packet into the bitmap; only the allocation is ULONG aligned */
    BitmapBytes = max(2, m_PortCount / 8 + 1);
    BitmapBytes = max(BitmapBytes, m_InterruptMaxPacket);

    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
    Attributes.ParentObject = m_Device;

    Status = WdfMemoryCreate(&Attributes,
                             NonPagedPool,
                             HUB_TAG_HUB,
                             ALIGN_UP_BY(BitmapBytes, sizeof(ULONG)),
                             &m_Interrupt.BitmapMemory,
                             &Buffer);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Hub %p change bitmap allocation failed 0x%lx\n", m_Device, Status);
        goto Cleanup;
    }

    m_Interrupt.BitmapBuffer = (PUCHAR)Buffer;
    m_Interrupt.BitmapMaxBytes = BitmapBytes;

    DPRINT("Hub %p has %lu ports, bitmap %lu bytes\n", m_Device, m_PortCount, BitmapBytes);

    SetFlag(HubFlag::Configured);
    return TRUE;

Cleanup:
    while (!IsListEmpty(&m_Mux.Ports))
    {
        Entry = RemoveHeadList(&m_Mux.Ports);
        m_Mux.PortCount--;
        WdfObjectDelete(CONTAINING_RECORD(Entry, HubPort, m_HubLink)->m_Object);
    }

    return FALSE;
}

VOID
HubFdo::PostToAllPorts(
    _In_ PortEvent Event)
{
    PLIST_ENTRY Entry;

    for (Entry = m_Mux.Ports.Flink; Entry != &m_Mux.Ports; Entry = Entry->Flink)
        CONTAINING_RECORD(Entry, HubPort, m_HubLink)->Post(Event);
}

/* Start and every kind of resume: every port owes one power acknowledgement */
VOID
HubFdo::QueuePowerUpToPorts(
    _In_ PortEvent Event)
{
    PLIST_ENTRY Entry;
    HubPort* Port;

    {
        HubSpinLockGuard Guard(&m_Mux.Lock);

        m_Mux.FailDeviceRegistration = FALSE;
        m_Mux.FailInterruptAcquire = FALSE;
    }

    if (m_Mux.PortCount == 0)
    {
        Post(HubEvent::PortsAcquired);
        return;
    }

    m_Mux.OutstandingPnpOperations = m_Mux.PortCount;

    for (Entry = m_Mux.Ports.Flink; Entry != &m_Mux.Ports; Entry = Entry->Flink)
    {
        Port = CONTAINING_RECORD(Entry, HubPort, m_HubLink);
        Port->SetMuxFlag(PortMuxFlag::PnpOperationPending);
        Port->Post(Event);
    }
}

/* Only ports holding a power reference are waited for, but all of them hear it */
VOID
HubFdo::QueueStopToPorts()
{
    PLIST_ENTRY Entry;
    HubPort* Port;
    BOOLEAN NothingToWaitFor;

    if (m_Mux.PortCount == 0)
    {
        Post(HubEvent::PortsReleased);
        return;
    }

    m_Mux.OutstandingPnpOperations = m_Mux.PortPowerReferences;
    NothingToWaitFor = (m_Mux.OutstandingPnpOperations == 0);

    for (Entry = m_Mux.Ports.Flink; Entry != &m_Mux.Ports; Entry = Entry->Flink)
    {
        Port = CONTAINING_RECORD(Entry, HubPort, m_HubLink);
        if (Port->HasMuxFlag(PortMuxFlag::PowerReference))
            Port->SetMuxFlag(PortMuxFlag::PnpOperationPending);
        Port->Post(PortEvent::HubStopping);
    }

    if (NothingToWaitFor)
        Post(HubEvent::PortsReleased);
}

/* Every port is expected to hold a power reference while the hub is up */
VOID
HubFdo::QueueSuspendToPorts()
{
    PLIST_ENTRY Entry;
    HubPort* Port;

    if (m_Mux.PortCount == 0)
    {
        Post(HubEvent::PortsReleased);
        return;
    }

    m_Mux.OutstandingPnpOperations = m_Mux.PortCount;

    for (Entry = m_Mux.Ports.Flink; Entry != &m_Mux.Ports; Entry = Entry->Flink)
    {
        Port = CONTAINING_RECORD(Entry, HubPort, m_HubLink);
        Port->SetMuxFlag(PortMuxFlag::PnpOperationPending);
        Port->Post(PortEvent::HubSuspending);
    }
}

VOID
HubFdo::QueueSurpriseRemovalToPorts()
{
    PostToAllPorts(PortEvent::HubRemoved);

    HubSpinLockGuard Guard(&m_Mux.Lock);
    m_Mux.FailDeviceRegistration = TRUE;
}

/* With no ports this never completes */
VOID
HubFdo::QueueResetToPorts()
{
    PLIST_ENTRY Entry;
    HubPort* Port;

    if (m_Mux.PortCount == 0)
        DPRINT1("Hub %p reset fan out with no ports will not finish\n", m_Device);

    InterlockedExchange(&m_Mux.HubResetInProgress, 1);
    m_Mux.OutstandingResetOperations = m_Mux.PortCount;

    for (Entry = m_Mux.Ports.Flink; Entry != &m_Mux.Ports; Entry = Entry->Flink)
    {
        Port = CONTAINING_RECORD(Entry, HubPort, m_HubLink);
        Port->SetMuxFlag(PortMuxFlag::ResetPending);
        Port->Post(PortEvent::HubResetting);
    }
}

/*
 * Bit 0 of the change bitmap is the hub itself and was handled already.
 * Bits for ports that do not exist are counted, then taken back.
 */
VOID
HubFdo::QueueStatusChangeToPorts()
{
    PLIST_ENTRY Entry;
    HubPort* Port;
    LONG Expected;
    LONG Sent = 0;

    Expected = (LONG)RtlNumberOfSetBits(&m_Interrupt.Bitmap);
    if (Expected == 0)
    {
        m_Interrupt.LastInterruptWasEmpty = TRUE;
        Post(HubEvent::PortsEnableInterrupt);
        return;
    }

    m_Mux.OutstandingPortChanges = Expected;

    for (Entry = m_Mux.Ports.Flink; Entry != &m_Mux.Ports; Entry = Entry->Flink)
    {
        Port = CONTAINING_RECORD(Entry, HubPort, m_HubLink);
        if (Port->Number() >= m_Interrupt.Bitmap.SizeOfBitMap ||
            !RtlCheckBit(&m_Interrupt.Bitmap, Port->Number()))
        {
            continue;
        }

        Port->SetMuxFlag(PortMuxFlag::PortChangePending);
        Port->Post(PortEvent::StatusChanged);
        Sent++;
    }

    if (Sent < Expected &&
        InterlockedExchangeAdd(&m_Mux.OutstandingPortChanges, -(Expected - Sent)) == Expected - Sent)
    {
        Post(HubEvent::PortsEnableInterrupt);
    }
}

/* After resume from selective suspend every port rechecks; no ports never completes */
VOID
HubFdo::QueueFakeStatusChangeToPorts()
{
    PLIST_ENTRY Entry;
    HubPort* Port;

    if (m_Mux.PortCount == 0)
        DPRINT1("Hub %p status recheck with no ports will not finish\n", m_Device);

    m_Mux.OutstandingPortChanges = m_Mux.PortCount;

    for (Entry = m_Mux.Ports.Flink; Entry != &m_Mux.Ports; Entry = Entry->Flink)
    {
        Port = CONTAINING_RECORD(Entry, HubPort, m_HubLink);
        Port->SetMuxFlag(PortMuxFlag::PortChangePending);
        Port->Post(PortEvent::StatusChanged);
    }
}

VOID
HubFdo::AllowHubReset()
{
    HubSpinLockGuard Guard(&m_Mux.Lock);
    m_Mux.HubResetEnabled = TRUE;
}

/** Only the first caller after AllowHubReset gets a reset posted. */
BOOLEAN
HubFdo::RequestHubReset()
{
    BOOLEAN Queue;

    {
        HubSpinLockGuard Guard(&m_Mux.Lock);

        Queue = m_Mux.HubResetEnabled;
        m_Mux.HubResetEnabled = FALSE;
    }

    if (Queue)
        Post(HubEvent::ResetRequested);

    return Queue;
}

/* Interrupt references */

BOOLEAN
HubFdo::PortInterruptRefsReleased()
{
    HubSpinLockGuard Guard(&m_Mux.Lock);
    BOOLEAN Released = (m_Mux.InterruptReferences == 0);

    if (!Released)
        m_Mux.InterruptReleasePending = TRUE;

    m_Mux.FailInterruptAcquire = TRUE;
    return Released;
}

BOOLEAN
HubFdo::TakeInterruptReference(
    _In_ HubPort* Port)
{
    HubSpinLockGuard Guard(&m_Mux.Lock);

    if (m_Mux.FailInterruptAcquire)
        return FALSE;

    Port->SetMuxFlag(PortMuxFlag::InterruptReference);
    m_Mux.InterruptReferences++;
    return TRUE;
}

VOID
HubFdo::DropInterruptReference(
    _In_ HubPort* Port)
{
    BOOLEAN Released = FALSE;

    {
        HubSpinLockGuard Guard(&m_Mux.Lock);

        if (!Port->HasMuxFlag(PortMuxFlag::InterruptReference))
            DPRINT1("Port %u dropped an interrupt reference it did not hold\n", Port->Number());
        NT_ASSERT(Port->HasMuxFlag(PortMuxFlag::InterruptReference));
        Port->ClearMuxFlag(PortMuxFlag::InterruptReference);

        if (m_Mux.InterruptReferences == 0)
            DPRINT1("Hub %p interrupt references released too many times\n", m_Device);
        NT_ASSERT(m_Mux.InterruptReferences != 0);
        if (m_Mux.InterruptReferences != 0)
            m_Mux.InterruptReferences--;

        if (m_Mux.InterruptReferences == 0 && m_Mux.InterruptReleasePending)
        {
            m_Mux.InterruptReleasePending = FALSE;
            Released = TRUE;
        }
    }

    if (Released)
        Post(HubEvent::PortsInterruptReleased);
}

/* Port acknowledgements, interlocked without the mux lock */

VOID
HubFdo::TakePortPowerReference(
    _In_ HubPort* Port)
{
    Port->ClearMuxFlag(PortMuxFlag::PnpOperationPending);
    Port->SetMuxFlag(PortMuxFlag::PowerReference);
    InterlockedIncrement(&m_Mux.PortPowerReferences);

    if (InterlockedDecrement(&m_Mux.OutstandingPnpOperations) == 0)
        Post(HubEvent::PortsAcquired);
}

VOID
HubFdo::DropPortPowerReference(
    _In_ HubPort* Port)
{
    Port->ClearMuxFlag(PortMuxFlag::PnpOperationPending);
    Port->ClearMuxFlag(PortMuxFlag::PowerReference);
    InterlockedDecrement(&m_Mux.PortPowerReferences);

    if (InterlockedDecrement(&m_Mux.OutstandingPnpOperations) == 0)
        Post(HubEvent::PortsReleased);

    /* Taken when a root port answered with an operation still pending */
    if (Port->HasFlag(PortFlag::PendingHubPowerReference))
    {
        WdfDeviceResumeIdle(m_Device);
        Port->ClearFlag(PortFlag::PendingHubPowerReference);
    }
}

VOID
HubFdo::DropHubResetReference(
    _In_ HubPort* Port)
{
    Port->ClearMuxFlag(PortMuxFlag::ResetPending);

    if (InterlockedDecrement(&m_Mux.OutstandingResetOperations) == 0)
        Post(HubEvent::PortsResetReleased);
}

VOID
HubFdo::ResumeInterruptTransfer(
    _In_ HubPort* Port)
{
    Port->ClearMuxFlag(PortMuxFlag::PortChangePending);

    if (InterlockedDecrement(&m_Mux.OutstandingPortChanges) == 0)
        Post(HubEvent::PortsEnableInterrupt);
}

/* Devices */

/** A new device starts out holding a power reference. */
BOOLEAN
HubFdo::RegisterDevice(
    _In_ HubChild* Child)
{
    {
        HubSpinLockGuard Guard(&m_Mux.Lock);

        if (m_Mux.FailDeviceRegistration)
        {
            DPRINT("Hub %p refused device %p, hub is going down\n", m_Device, Child);
            return FALSE;
        }

        Child->m_MuxFlags |= (ULONG)ChildMuxFlag::PowerReference;
        m_Mux.DevicePowerReferences++;
        InsertTailList(&m_Mux.Devices, &Child->m_HubLink);
    }

    WdfObjectReferenceWithTag(Child->m_Object, (PVOID)HubTagRegistration);
    return TRUE;
}

/* Posts the completion of whatever device fan out is running */
VOID
HubFdo::FinishDeviceFanOut()
{
    HubDeviceFanOut FanOut = m_Mux.DeviceFanOut;

    m_Mux.DeviceFanOut = HubDeviceFanOut::None;

    switch (FanOut)
    {
        case HubDeviceFanOut::PowerUp:
            Post(HubEvent::DevicesAcquired);
            break;

        case HubDeviceFanOut::PowerDown:
            Post(HubEvent::DevicesReleased);
            break;

        case HubDeviceFanOut::StopWhileSuspended:
            Post(HubEvent::DevicesStoppedAfterSuspend);
            break;

        default:
            DPRINT1("Hub %p device fan out ended with none running\n", m_Device);
            NT_ASSERT(FALSE);
            break;
    }
}

/* The completion event follows the running fan out, not the device's power reference */
VOID
HubFdo::UnregisterDevice(
    _In_ HubChild* Child)
{
    BOOLEAN Completed = FALSE;

    Child->ClearState(ChildState::ActivityIdSet);

    {
        HubSpinLockGuard Guard(&m_Mux.Lock);

        if (Child->m_MuxFlags & (ULONG)ChildMuxFlag::PowerReference)
        {
            m_Mux.DevicePowerReferences--;
            Child->m_MuxFlags &= ~(ULONG)ChildMuxFlag::PowerReference;
        }

        if (Child->m_MuxFlags & (ULONG)ChildMuxFlag::PnpOperationPending)
        {
            Child->m_MuxFlags &= ~(ULONG)ChildMuxFlag::PnpOperationPending;
            if (InterlockedDecrement(&m_Mux.OutstandingPnpOperations) == 0)
                Completed = TRUE;
        }

        RemoveEntryList(&Child->m_HubLink);
        HubClearListEntry(&Child->m_HubLink);
    }

    WdfObjectDereferenceWithTag(Child->m_Object, (PVOID)HubTagRegistration);

    if (Completed)
        FinishDeviceFanOut();
}

/*
 * Stop and suspend wait only for devices holding a power reference; every
 * registered device still hears the event.
 */
VOID
HubFdo::QueuePowerDownToDevices(
    _In_ DsmEvent Event,
    _In_ HubDeviceFanOut FanOut)
{
    LIST_ENTRY FanOutList;
    PLIST_ENTRY Entry;
    HubChild* Child;
    LONG Counted = 0;
    BOOLEAN Any;

    InitializeListHead(&FanOutList);

    {
        HubSpinLockGuard Guard(&m_Mux.Lock);

        m_Mux.FailDeviceRegistration = TRUE;

        for (Entry = m_Mux.Devices.Flink; Entry != &m_Mux.Devices; Entry = Entry->Flink)
        {
            Child = CONTAINING_RECORD(Entry, HubChild, m_HubLink);

            if (FanOut == HubDeviceFanOut::StopWhileSuspended ||
                (Child->m_MuxFlags & (ULONG)ChildMuxFlag::PowerReference))
            {
                Child->m_MuxFlags |= (ULONG)ChildMuxFlag::PnpOperationPending;
                Counted++;
            }

            WdfObjectReferenceWithTag(Child->m_Object, (PVOID)HubTagPnpPower);
            InsertTailList(&FanOutList, &Child->m_FanOutLink);
        }

        m_Mux.OutstandingPnpOperations = Counted;
        m_Mux.DeviceFanOut = (Counted != 0) ? FanOut : HubDeviceFanOut::None;
    }

    Any = !IsListEmpty(&FanOutList);

    while (!IsListEmpty(&FanOutList))
    {
        Entry = RemoveHeadList(&FanOutList);
        HubClearListEntry(Entry);
        Child = CONTAINING_RECORD(Entry, HubChild, m_FanOutLink);

        Child->Post(Event);
        WdfObjectDereferenceWithTag(Child->m_Object, (PVOID)HubTagPnpPower);
    }

    if (!Any || Counted == 0)
        Post(FanOut == HubDeviceFanOut::StopWhileSuspended ? HubEvent::DevicesStoppedAfterSuspend
                                                         : HubEvent::DevicesReleased);
}

/* Devices that registered while the hub was down are not stopped and are skipped */
VOID
HubFdo::QueuePowerUpToDevices(
    _In_ DsmEvent Event)
{
    LIST_ENTRY FanOutList;
    PLIST_ENTRY Entry;
    HubChild* Child;
    LONG Counted = 0;

    InitializeListHead(&FanOutList);

    {
        HubSpinLockGuard Guard(&m_Mux.Lock);

        for (Entry = m_Mux.Devices.Flink; Entry != &m_Mux.Devices; Entry = Entry->Flink)
        {
            Child = CONTAINING_RECORD(Entry, HubChild, m_HubLink);
            if (!(Child->m_MuxFlags & (ULONG)ChildMuxFlag::Stopped))
                continue;

            Child->m_MuxFlags &= ~(ULONG)ChildMuxFlag::Stopped;
            Child->m_MuxFlags |= (ULONG)ChildMuxFlag::PnpOperationPending;
            WdfObjectReferenceWithTag(Child->m_Object, (PVOID)HubTagPnpPower);
            InsertTailList(&FanOutList, &Child->m_FanOutLink);
            Counted++;
        }

        m_Mux.OutstandingPnpOperations = Counted;
        m_Mux.DeviceFanOut = (Counted != 0) ? HubDeviceFanOut::PowerUp : HubDeviceFanOut::None;
    }

    if (Counted == 0)
    {
        Post(HubEvent::DevicesAcquired);
        return;
    }

    while (!IsListEmpty(&FanOutList))
    {
        Entry = RemoveHeadList(&FanOutList);
        HubClearListEntry(Entry);
        Child = CONTAINING_RECORD(Entry, HubChild, m_FanOutLink);

        Child->Post(Event);
        WdfObjectDereferenceWithTag(Child->m_Object, (PVOID)HubTagPnpPower);
    }
}

/* Device acknowledgements; the completion is posted after the lock is dropped */

VOID
HubFdo::ReferenceDevicePower(
    _In_ HubChild* Child)
{
    BOOLEAN Completed;

    {
        HubSpinLockGuard Guard(&m_Mux.Lock);

        Child->m_MuxFlags &= ~((ULONG)ChildMuxFlag::PnpOperationPending | (ULONG)ChildMuxFlag::Stopped);
        Child->m_MuxFlags |= (ULONG)ChildMuxFlag::PowerReference;
        m_Mux.DevicePowerReferences++;
        Completed = (InterlockedDecrement(&m_Mux.OutstandingPnpOperations) == 0);
    }

    if (Completed)
        FinishDeviceFanOut();
}

VOID
HubFdo::DereferenceDevicePower(
    _In_ HubChild* Child)
{
    BOOLEAN Completed;

    {
        HubSpinLockGuard Guard(&m_Mux.Lock);

        Child->m_MuxFlags &= ~((ULONG)ChildMuxFlag::PnpOperationPending | (ULONG)ChildMuxFlag::PowerReference);
        Child->m_MuxFlags |= (ULONG)ChildMuxFlag::Stopped;
        m_Mux.DevicePowerReferences--;
        Completed = (InterlockedDecrement(&m_Mux.OutstandingPnpOperations) == 0);
    }

    if (Completed)
        FinishDeviceFanOut();
}

VOID
HubFdo::ConfirmStopWhileSuspended(
    _In_ HubChild* Child)
{
    BOOLEAN Completed;

    {
        HubSpinLockGuard Guard(&m_Mux.Lock);

        Child->m_MuxFlags &= ~(ULONG)ChildMuxFlag::PnpOperationPending;
        Child->m_MuxFlags |= (ULONG)ChildMuxFlag::Stopped;
        Completed = (InterlockedDecrement(&m_Mux.OutstandingPnpOperations) == 0);
    }

    if (Completed)
        FinishDeviceFanOut();
}

/* The controller lost every device's programming on resume or hub reset */
VOID
HubFdo::MarkDevicesProgrammingLost()
{
    PLIST_ENTRY Entry;

    HubSpinLockGuard Guard(&m_Mux.Lock);

    for (Entry = m_Mux.Devices.Flink; Entry != &m_Mux.Devices; Entry = Entry->Flink)
        CONTAINING_RECORD(Entry, HubChild, m_HubLink)->SetState(ChildState::ReprogrammingPending);
}

/* Hub machine actions */

BOOLEAN
HubMachine::CreateChildPorts()
{
    return m_Hub->CreateChildPorts();
}

BOOLEAN
HubMachine::PortInterruptRefsReleased()
{
    return m_Hub->PortInterruptRefsReleased();
}

VOID
HubMachine::AllowHubReset()
{
    m_Hub->AllowHubReset();
}

VOID
HubMachine::QueueStartToPorts()
{
    InterlockedExchange(&m_Hub->m_Mux.HubResetInProgress, 0);
    m_Hub->QueuePowerUpToPorts(PortEvent::HubStarted);
}

VOID
HubMachine::QueueResumeToPorts()
{
    m_Hub->QueuePowerUpToPorts(PortEvent::HubResumed);
}

VOID
HubMachine::QueueResumeInS0ToPorts()
{
    m_Hub->QueuePowerUpToPorts(PortEvent::HubResumedInS0);
}

VOID
HubMachine::QueueResumeWithResetToPorts()
{
    m_Hub->QueuePowerUpToPorts(PortEvent::HubResumedWithReset);
}

VOID
HubMachine::QueueStopToPorts()
{
    m_Hub->QueueStopToPorts();
}

VOID
HubMachine::QueueSuspendToPorts()
{
    m_Hub->QueueSuspendToPorts();
}

VOID
HubMachine::QueueSurpriseRemovalToPorts()
{
    m_Hub->QueueSurpriseRemovalToPorts();
}

VOID
HubMachine::QueueResetToPorts()
{
    m_Hub->QueueResetToPorts();
}

VOID
HubMachine::QueueStatusChangeToPorts()
{
    m_Hub->QueueStatusChangeToPorts();
}

VOID
HubMachine::QueueFakeStatusChangeToPorts()
{
    m_Hub->QueueFakeStatusChangeToPorts();
}

VOID
HubMachine::QueueStopToDevices()
{
    m_Hub->QueuePowerDownToDevices(DsmEvent::HubStopping, HubDeviceFanOut::PowerDown);
}

VOID
HubMachine::QueueSuspendToDevices()
{
    m_Hub->QueuePowerDownToDevices(DsmEvent::HubSuspending, HubDeviceFanOut::PowerDown);
}

VOID
HubMachine::QueueStopAfterSuspendToDevices()
{
    m_Hub->QueuePowerDownToDevices(DsmEvent::HubStoppingAfterSuspend, HubDeviceFanOut::StopWhileSuspended);
}

VOID
HubMachine::QueueStartToDevices()
{
    m_Hub->QueuePowerUpToDevices(DsmEvent::HubStarted);
}

VOID
HubMachine::QueueResumeToDevices()
{
    m_Hub->QueuePowerUpToDevices(DsmEvent::HubResumed);
}

VOID
HubMachine::QueueResumeInS0ToDevices()
{
    m_Hub->QueuePowerUpToDevices(DsmEvent::HubResumedInS0);
}

VOID
HubMachine::QueueResumeWithResetToDevices()
{
    m_Hub->QueuePowerUpToDevices(DsmEvent::HubResumedWithReset);
}

/* Port machine actions */

VOID
PortMachine::TakePortPowerReference()
{
    m_Port->m_Hub->TakePortPowerReference(m_Port);
}

VOID
PortMachine::DropPortPowerReference()
{
    m_Port->m_Hub->DropPortPowerReference(m_Port);
}

VOID
PortMachine::DropHubResetReference()
{
    m_Port->m_Hub->DropHubResetReference(m_Port);
}

BOOLEAN
PortMachine::TakeInterruptReference()
{
    return m_Port->m_Hub->TakeInterruptReference(m_Port);
}

VOID
PortMachine::DropInterruptReference()
{
    m_Port->m_Hub->DropInterruptReference(m_Port);
}

VOID
PortMachine::RequestHubReset()
{
    m_Port->m_Hub->RequestHubReset();
}

VOID
PortMachine::ResumeInterruptTransfer()
{
    m_Port->m_Hub->ResumeInterruptTransfer(m_Port);
}

BOOLEAN
PortMachine::HubArmedForWake()
{
    return m_Port->m_Hub->HasFlag(HubFlag::ArmedForWake);
}

/* Device machine actions */

BOOLEAN
DeviceMachine::RegisterWithHub()
{
    return m_Device->m_Hub->RegisterDevice(m_Device);
}

VOID
DeviceMachine::UnregisterFromHub()
{
    m_Device->m_Hub->UnregisterDevice(m_Device);
}

VOID
DeviceMachine::ReferenceDevicePower()
{
    m_Device->m_Hub->ReferenceDevicePower(m_Device);
}

VOID
DeviceMachine::DereferenceDevicePower()
{
    m_Device->m_Hub->DereferenceDevicePower(m_Device);
}

VOID
DeviceMachine::ConfirmStopWhileSuspended()
{
    m_Device->m_Hub->ConfirmStopWhileSuspended(m_Device);
}
