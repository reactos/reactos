/*
 * PROJECT:     ReactOS SuperSpeed USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Device object lifetime, its timer and the device machine hooks
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "usbhub3.h"

#define NDEBUG
#include <debug.h>

/* Reference tags; only their addresses matter */
static const char HubTagDeviceCreate[] = "hub child create";
static const char HubTagDevice[] = "hub child";

/* Device timer durations in ms */
#define DEVICE_ENUM_RETRY_TIME          500
#define DEVICE_POST_RESET_TIME_20       32
#define DEVICE_POST_RESET_TIME_LONG     100
#define DEVICE_POST_RESET_TIME_30       15
#define DEVICE_POST_ADDRESS_TIME        10
#define DEVICE_DUPLICATE_POLL_TIME      500

/* Language the hub asks strings in until the device says otherwise */
#define DEVICE_DEFAULT_LANGUAGE         0x0409

HubChild*
HubChild::FromObject(
    _In_ WDFOBJECT Object)
{
    return HubGetChildContext(Object);
}

USB_DEVICE_SPEED
HubChild::Speed() const
{
    if (m_Kind & DSM_KIND_SUPER_SPEED)
        return UsbSuperSpeed;
    if (m_Kind & DSM_KIND_HIGH_SPEED)
        return UsbHighSpeed;
    if (m_Kind & DSM_KIND_LOW_SPEED)
        return UsbLowSpeed;
    return UsbFullSpeed;
}

/** Deletes a device owned object and drops the device's own reference on it. */
static
VOID
NTAPI
HubDropDeviceObject(
    _Inout_ WDFOBJECT* Object)
{
    if (*Object == NULL)
        return;

    WdfObjectDelete(*Object);
    WdfObjectDereferenceWithTag(*Object, (PVOID)HubTagDevice);
    *Object = NULL;
}

static
VOID
NTAPI
HubFreeIfPresent(
    _In_opt_ PVOID Buffer)
{
    if (Buffer != NULL)
        ExFreePoolWithTag(Buffer, HUB_TAG_DEVICE);
}

static
NTSTATUS
NTAPI
HubCreateDeviceRequest(
    _In_ HubPort* Port,
    _Out_ WDFREQUEST* Request)
{
    WDF_OBJECT_ATTRIBUTES Attributes;
    NTSTATUS Status;

    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
    Attributes.ParentObject = Port->m_Object;

    Status = WdfRequestCreate(&Attributes, WdfDeviceGetIoTarget(Port->m_Hub->m_Device), Request);
    if (NT_SUCCESS(Status))
        WdfObjectReferenceWithTag(*Request, (PVOID)HubTagDevice);

    return Status;
}

/* Requests, timer and memory hang off the port object, so the device holds its own references to them */
HubChild*
HubChild::Create(
    _In_ HubPort* Port)
{
    HubFdo* Hub = Port->m_Hub;
    WDF_OBJECT_ATTRIBUTES Attributes;
    WDF_TIMER_CONFIG TimerConfig;
    HubChild* Child;
    WDFOBJECT Object;
    PVOID Payload;
    NTSTATUS Status;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Attributes, HubChild);
    Attributes.ParentObject = Port->m_Object;
    Attributes.ExecutionLevel = WdfExecutionLevelPassive;
    Attributes.EvtDestroyCallback = EvtDestroy;

    Status = WdfObjectCreate(&Attributes, &Object);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Port %u device object creation failed 0x%lx\n", Port->Number(), Status);
        return NULL;
    }

    WdfObjectReferenceWithTag(Port->m_Object, (PVOID)HubTagDeviceCreate);

    Child = new (HubGetChildContext(Object)) HubChild();
    Child->m_Object = Object;
    Child->m_Port = Port;
    Child->m_Hub = Hub;

    if (!Port->HasProperty(PortProperty::Removable))
        Child->SetProperty(ChildProperty::NotRemovable);
    else
        Child->ClearProperty(ChildProperty::NotRemovable);

    DPRINT("Hub %p port %u enumeration starting\n", Hub, Port->Number());

    Status = Child->m_Control.Create(Port->m_Object, WdfDeviceGetIoTarget(Hub->m_Device));
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Device %p control request creation failed 0x%lx\n", Child, Status);
        goto Failed;
    }
    WdfObjectReferenceWithTag(Child->m_Control.Request, (PVOID)HubTagDevice);

    Status = HubCreateDeviceRequest(Port, &Child->m_UcxRequest);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Device %p UCX request creation failed 0x%lx\n", Child, Status);
        goto Failed;
    }

    Child->m_LanguageId = DEVICE_DEFAULT_LANGUAGE;
    Child->m_Kind = 0;
    Child->m_ValidationBitmap.SizeOfBitMap = sizeof(Child->m_ValidationBits) * 8;
    Child->m_ValidationBitmap.Buffer = Child->m_ValidationBits;

    Child->m_Timer.Initialize(TimerFired, Child);

    WDF_TIMER_CONFIG_INIT(&TimerConfig, HubEvtBandwidthRetryTimer);
    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
    Attributes.ParentObject = Port->m_Object;
    Attributes.ExecutionLevel = WdfExecutionLevelPassive;

    Status = WdfTimerCreate(&TimerConfig, &Attributes, &Child->m_BandwidthRetryTimer);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Device %p out of bandwidth timer creation failed 0x%lx\n", Child, Status);
        goto Failed;
    }
    WdfObjectReferenceWithTag(Child->m_BandwidthRetryTimer, (PVOID)HubTagDevice);

    KeInitializeEvent(&Child->m_PnpEvent, NotificationEvent, FALSE);
    KeInitializeEvent(&Child->m_PreStartEvent, NotificationEvent, FALSE);
    KeInitializeEvent(&Child->m_QueryTextEvent, NotificationEvent, FALSE);

    Child->m_WorkItem = Hub->AllocateWorkItem();
    if (Child->m_WorkItem == NULL)
    {
        DPRINT1("Device %p work item allocation failed\n", Child);
        goto Failed;
    }

    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
    Attributes.ParentObject = Port->m_Object;

    Status = WdfMemoryCreate(&Attributes,
                             NonPagedPool,
                             HUB_TAG_DEVICE,
                             sizeof(HubUcxPayload),
                             &Child->m_UcxPayloadMemory,
                             &Payload);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Device %p UCX payload allocation failed 0x%lx\n", Child, Status);
        goto Failed;
    }
    WdfObjectReferenceWithTag(Child->m_UcxPayloadMemory, (PVOID)HubTagDevice);
    Child->m_UcxPayload = (HubUcxPayload*)Payload;

    KeInitializeSpinLock(&Child->m_ConfigLock);
    Child->m_Machine.Initialize(Child);

    Port->m_Child = Child;
    DPRINT("Port %u created device %p\n", Port->Number(), Child);
    return Child;

Failed:
    WdfObjectDelete(Object);
    return NULL;
}

/* At passive level; the timer callback is waited for */
VOID
NTAPI
HubChild::EvtDestroy(
    _In_ WDFOBJECT Object)
{
    HubChild* Child = HubGetChildContext(Object);

    if (Child->m_Port == NULL)
        return;

    Child->m_Hub->FlushAndDeleteWorkItem(&Child->m_WorkItem);

    HubReleaseRegistryState(Child);
    HubIdFreeDeviceIds(Child);
    HubReleaseTransferState(Child);
    HubFreeIfPresent(Child->m_SerialNumber);
    HubFreeIfPresent(Child->m_AlternateSettingFilter);
    HubFreeIfPresent(Child->m_ConfigDescriptor);
    HubFreeIfPresent(Child->m_LanguageIds);
    HubFreeIfPresent(Child->m_ProductString);
    HubFreeIfPresent(Child->m_Bos);
    HubFreeIfPresent(Child->m_EndpointsToEnable);
    HubFreeIfPresent(Child->m_EndpointsToDisable);
    HubFreeIfPresent(Child->m_EndpointsUnchanged);

    HubDropDeviceObject((WDFOBJECT*)&Child->m_Control.Request);
    HubDropDeviceObject((WDFOBJECT*)&Child->m_UcxRequest);

    Child->m_Timer.Cancel();
    KeFlushQueuedDpcs();

    HubDropDeviceObject((WDFOBJECT*)&Child->m_BandwidthRetryTimer);
    HubDropDeviceObject((WDFOBJECT*)&Child->m_UcxPayloadMemory);

    WdfObjectDereferenceWithTag(Child->m_Port->m_Object, (PVOID)HubTagDeviceCreate);
}

VOID
NTAPI
HubChild::TimerFired(
    _In_ PVOID Context)
{
    ((HubChild*)Context)->Post(DsmEvent::TimerFired);
}

VOID
NTAPI
HubChild::MachineWorkItem(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_opt_ PVOID Context,
    _In_ PUCXHUB_WORKITEM WorkItem)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(WorkItem);

    ((HubChild*)Context)->m_Machine.SmContinueOnPassive();
}

/* Device machine engine hooks */

VOID
DeviceMachine::ReferenceDevice()
{
    WdfObjectReference(m_Device->m_Object);
}

VOID
DeviceMachine::DereferenceDevice()
{
    HubDereferenceDeferred(m_Device->m_Object);
}

VOID
DeviceMachine::QueuePassiveWork()
{
    m_Device->m_Hub->EnqueueWorkItem(m_Device->m_WorkItem,
                                     HubChild::MachineWorkItem,
                                     m_Device,
                                     m_Device->m_NeedsForwardProgress);
}

BOOLEAN
DeviceMachine::CancelStateTimer()
{
    return m_Device->m_Timer.Cancel();
}

BOOLEAN
DeviceMachine::StopTimer()
{
    return m_Device->m_Timer.Cancel();
}

ULONG
DeviceMachine::DeviceKind()
{
    return m_Device->m_Kind;
}

VOID
DeviceMachine::MarkAttachFailed()
{
    m_Device->ClearState(ChildState::AttachSucceeded);
}

VOID
DeviceMachine::MarkAttachSucceeded()
{
    m_Device->SetState(ChildState::AttachSucceeded);
}

VOID
DeviceMachine::ReferencePort()
{
    WdfObjectReferenceWithTag(m_Device->m_Port->m_Object, (PVOID)HubTagDevice);
}

VOID
DeviceMachine::DereferencePort()
{
    WdfObjectDereferenceWithTag(m_Device->m_Port->m_Object, (PVOID)HubTagDevice);
}

/* Device timers */

VOID
DeviceMachine::StartRetryTimer()
{
    DPRINT1("Device %p retrying enumeration in %lu ms\n", m_Device, (ULONG)DEVICE_ENUM_RETRY_TIME);
    m_Device->m_Timer.Start(DEVICE_ENUM_RETRY_TIME);
}

VOID
DeviceMachine::StartPostResetTimer()
{
    m_Device->m_Timer.Start(DEVICE_POST_RESET_TIME_20);
}

VOID
DeviceMachine::StartLongPostResetTimer()
{
    m_Device->m_Timer.Start(DEVICE_POST_RESET_TIME_LONG);
}

VOID
DeviceMachine::StartSuperSpeedPostResetTimer()
{
    m_Device->m_Timer.Start(DEVICE_POST_RESET_TIME_30);
}

VOID
DeviceMachine::ArmPostAddressTimer()
{
    m_Device->m_Timer.Start(DEVICE_POST_ADDRESS_TIME);
}

VOID
DeviceMachine::ArmDuplicateDeviceTimer()
{
    m_Device->m_Timer.Start(DEVICE_DUPLICATE_POLL_TIME);
}
