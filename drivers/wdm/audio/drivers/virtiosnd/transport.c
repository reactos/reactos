/*
 * PROJECT:     ReactOS VirtIO Sound Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     VirtIO sound PCI transport
 * COPYRIGHT:   Copyright 2026 Ralf Lang <ralf.lang@ralf-lang.de>
 */

#include "virtio_snd_protocol.h"
#include "virtiosnd.h"
#include <stdbool.h>

#ifdef FORCEINLINE
#undef FORCEINLINE
#endif
#if defined(__GNUC__)
#define FORCEINLINE __attribute__((unused))
#else
#define FORCEINLINE
#endif
/* clang-format off */
/* Cannot let clang alphabetically sort this or we hit a bug: virtio_pci.h defines VirtIODevice and struct virtqueue used by VirtIO.h  */
#include <virtio_pci.h>
#include <VirtIO.h>
/* clang-format on */
#undef FORCEINLINE

NTKERNELAPI
PDEVICE_OBJECT
NTAPI
IoGetDeviceAttachmentBaseRef(_In_ PDEVICE_OBJECT DeviceObject);

#define VIRTIO_SND_PCI_VENDOR_ID 0x1AF4
#define VIRTIO_SND_PCI_DEVICE_ID 0x1059
#define VIRTIO_SND_POOL_TAG 'dSnV'
#define VIRTIO_SND_IO_PORT_MAX 0xFFFF
#define VIRTIO_SND_CONTROL_TIMEOUT_MS 2000
#define VIRTIO_SND_TX_SLOT_COUNT 4
#define VIRTIO_SND_EVENT_SLOT_COUNT 16
#define VIRTIO_SND_ISR_QUEUE 0x01

typedef struct _VIRTIO_SND_TX_SLOT
{
    VIRTIO_SND_PCM_XFER Request;
    UCHAR Audio[VIRTIO_SND_PCM_PERIOD_BYTES];
    VIRTIO_SND_PCM_STATUS Response;
    BOOLEAN InFlight;
} VIRTIO_SND_TX_SLOT, *PVIRTIO_SND_TX_SLOT;

typedef struct _VIRTIO_SND_EVENT_SLOT
{
    VIRTIO_SND_EVENT Event;
} VIRTIO_SND_EVENT_SLOT, *PVIRTIO_SND_EVENT_SLOT;

struct _VIRTIO_SND_PLAYBACK
{
    ULONG StreamId;
    ULONG Channels;
    ULONG Features;
    ULONG InFlightCount;
    PKEVENT WakeEvent;
    BOOLEAN Prepared;
    BOOLEAN Started;
    BOOLEAN DeviceActive;
#if defined(VIRTIO_SND_TEST_XRUN_INJECTION)
    volatile LONG TestXrunInjected;
#endif
    PVIRTIO_SND_TX_SLOT Slots[VIRTIO_SND_TX_SLOT_COUNT];
};

static ULONG_PTR VirtioSndTraceNotifyAddress;

typedef struct _VIRTIO_SND_BAR
{
    PHYSICAL_ADDRESS BaseAddress;
    SIZE_T Length;
    PVOID MappedAddress;
    BOOLEAN IsPort;
    BOOLEAN Assigned;
} VIRTIO_SND_BAR, *PVIRTIO_SND_BAR;

typedef struct _VIRTIO_SND_TRANSPORT
{
    ULONG BusNumber;
    PCI_SLOT_NUMBER SlotNumber;
    VIRTIO_SND_BAR Bars[PCI_TYPE0_ADDRESSES];
} VIRTIO_SND_TRANSPORT, *PVIRTIO_SND_TRANSPORT;

typedef struct _VIRTIO_SND_ADAPTER
{
    PDEVICE_OBJECT OwnerDeviceObject;
    VIRTIO_SND_TRANSPORT Transport;
    VirtIODevice Device;
    VIRTIO_SND_CONFIG Config;
    ULONGLONG HostFeatures;
    ULONGLONG DriverFeatures;
    struct virtqueue *Queues[VIRTIO_SND_VQ_MAX];
    PVIRTIO_SND_PCM_INFO PcmInfo;
    ULONG PcmInfoCount;
    PVIRTIO_SND_CHMAP_INFO ChannelMapInfo;
    ULONG ChannelMapInfoCount;
    PVIRTIO_SND_EVENT_SLOT EventSlots;
    ULONG EventSlotCount;
    PVOID OutstandingBuffer;
    BOOLEAN DeviceInitializeAttempted;
    BOOLEAN DeviceInitialized;
    BOOLEAN QueuesInitialized;
    BOOLEAN Started;
    PKINTERRUPT InterruptObject;
    KDPC InterruptDpc;
    KSPIN_LOCK PlaybackLock;
    PKEVENT PlaybackEvent;
    PVIRTIO_SND_PLAYBACK PlaybackContext;
    volatile LONG Stopping;
    LONG PlaybackOpen;
} VIRTIO_SND_ADAPTER, *PVIRTIO_SND_ADAPTER;

static VIRTIO_SND_ADAPTER VirtioSndAdapter;
static KMUTEX VirtioSndLifecycleMutex;
static KEVENT VirtioSndPlaybackQuiesced;
static KEVENT VirtioSndTransportStopped;
static LONG VirtioSndSynchronizationInitialized;

static VOID
VirtioSndInitializeSynchronization(VOID)
{
    if (InterlockedCompareExchange(&VirtioSndSynchronizationInitialized, 1, 0) == 0)
    {
        KeInitializeMutex(&VirtioSndLifecycleMutex, 0);
        KeInitializeEvent(&VirtioSndPlaybackQuiesced, NotificationEvent, TRUE);
        KeInitializeEvent(&VirtioSndTransportStopped, NotificationEvent, TRUE);
        InterlockedExchange(&VirtioSndSynchronizationInitialized, 2);
    }
    else
    {
        while (InterlockedCompareExchange(&VirtioSndSynchronizationInitialized, 0, 0) != 2)
        {
            KeStallExecutionProcessor(1);
        }
    }
}

static VOID
VirtioSndAcquireLifecycleLock(VOID)
{
    KeWaitForSingleObject(&VirtioSndLifecycleMutex, Executive, KernelMode, FALSE, NULL);
}

static VOID
VirtioSndReleaseLifecycleLock(VOID)
{
    KeReleaseMutex(&VirtioSndLifecycleMutex, FALSE);
}

BOOLEAN
VirtioSndTransportStopping(VOID)
{
    return InterlockedCompareExchange(&VirtioSndAdapter.Stopping, 0, 0) != FALSE || !VirtioSndAdapter.Started;
}

VOID
VirtioSndPlaybackWorkerQuiesced(VOID)
{
    KeSetEvent(&VirtioSndPlaybackQuiesced, IO_NO_INCREMENT, FALSE);
}

static VOID NTAPI
VirtioSndInterruptDpc(
    _In_ PKDPC Dpc,
    _In_opt_ PVOID DeferredContext,
    _In_opt_ PVOID SystemArgument1,
    _In_opt_ PVOID SystemArgument2)
{
    PVIRTIO_SND_ADAPTER Adapter = (PVIRTIO_SND_ADAPTER)DeferredContext;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    KeAcquireSpinLockAtDpcLevel(&Adapter->PlaybackLock);
    if (Adapter->PlaybackEvent != NULL)
    {
        KeSetEvent(Adapter->PlaybackEvent, IO_NO_INCREMENT, FALSE);
    }
    KeReleaseSpinLockFromDpcLevel(&Adapter->PlaybackLock);
}

static BOOLEAN NTAPI
VirtioSndInterruptService(_In_ PKINTERRUPT Interrupt, _In_opt_ PVOID ServiceContext)
{
    PVIRTIO_SND_ADAPTER Adapter = (PVIRTIO_SND_ADAPTER)ServiceContext;
    UCHAR InterruptStatus;

    UNREFERENCED_PARAMETER(Interrupt);

    if (!Adapter->DeviceInitialized)
        return FALSE;

    InterruptStatus = virtio_read_isr_status(&Adapter->Device);
    if ((InterruptStatus & (VIRTIO_SND_ISR_QUEUE | VIRTIO_PCI_ISR_CONFIG)) == 0)
        return FALSE;

    if ((InterruptStatus & VIRTIO_SND_ISR_QUEUE) != 0)
        KeInsertQueueDpc(&Adapter->InterruptDpc, NULL, NULL);

    return TRUE;
}

static UCHAR
VirtioSndReadByte(_In_ ULONG_PTR Register)
{
    if (Register > 0xFFFF)
        return READ_REGISTER_UCHAR((PUCHAR)Register);

    return READ_PORT_UCHAR((PUCHAR)Register);
}

static USHORT
VirtioSndReadWord(_In_ ULONG_PTR Register)
{
    if (Register > 0xFFFF)
        return READ_REGISTER_USHORT((PUSHORT)Register);

    return READ_PORT_USHORT((PUSHORT)Register);
}

static ULONG
VirtioSndReadDword(_In_ ULONG_PTR Register)
{
    if (Register > 0xFFFF)
        return READ_REGISTER_ULONG((PULONG)Register);

    return READ_PORT_ULONG((PULONG)Register);
}

static VOID
VirtioSndWriteByte(_In_ ULONG_PTR Register, _In_ UCHAR Value)
{
    if (Register > 0xFFFF)
    {
        WRITE_REGISTER_UCHAR((PUCHAR)Register, Value);
        return;
    }

    WRITE_PORT_UCHAR((PUCHAR)Register, Value);
}

static VOID
VirtioSndWriteWord(_In_ ULONG_PTR Register, _In_ USHORT Value)
{
    BOOLEAN TraceNotify = Register == VirtioSndTraceNotifyAddress;

    if (TraceNotify)
    {
        DbgPrint("virtiosnd: queue notification MMIO write begins at %p value %04x\n", (PVOID)Register, Value);
    }

    if (Register > 0xFFFF)
    {
        WRITE_REGISTER_USHORT((PUSHORT)Register, Value);
    }
    else
    {
        WRITE_PORT_USHORT((PUSHORT)Register, Value);
    }

    if (TraceNotify)
        DbgPrint("virtiosnd: queue notification MMIO write completed\n");
}

static VOID
VirtioSndWriteDword(_In_ ULONG_PTR Register, _In_ ULONG Value)
{
    if (Register > 0xFFFF)
    {
        WRITE_REGISTER_ULONG((PULONG)Register, Value);
        return;
    }

    WRITE_PORT_ULONG((PULONG)Register, Value);
}

static PVOID
VirtioSndAllocateContiguous(_In_ PVOID Context, _In_ size_t Size)
{
    PHYSICAL_ADDRESS LowestAddress = {0};
    PHYSICAL_ADDRESS HighestAddress;
    PHYSICAL_ADDRESS BoundaryAddress = {0};
    PVOID Memory;

    UNREFERENCED_PARAMETER(Context);

    if (Size == 0)
        return NULL;

    HighestAddress.QuadPart = MAXLONGLONG;
    Memory = MmAllocateContiguousMemorySpecifyCache(Size, LowestAddress, HighestAddress, BoundaryAddress, MmCached);
    if (Memory != NULL)
        RtlZeroMemory(Memory, Size);

    return Memory;
}

static VOID
VirtioSndFreeContiguous(_In_ PVOID Context, _In_ PVOID Memory)
{
    UNREFERENCED_PARAMETER(Context);

    if (Memory != NULL)
        MmFreeContiguousMemory(Memory);
}

static ULONGLONG
VirtioSndGetPhysicalAddress(_In_ PVOID Context, _In_ PVOID VirtualAddress)
{
    UNREFERENCED_PARAMETER(Context);

    return MmGetPhysicalAddress(VirtualAddress).QuadPart;
}

static PVOID
VirtioSndAllocateNonpaged(_In_ PVOID Context, _In_ size_t Size)
{
    PVOID Memory;

    UNREFERENCED_PARAMETER(Context);

    if (Size == 0)
        return NULL;

    Memory = ExAllocatePoolWithTag(NonPagedPool, Size, VIRTIO_SND_POOL_TAG);
    if (Memory != NULL)
        RtlZeroMemory(Memory, Size);

    return Memory;
}

static VOID
VirtioSndFreeNonpaged(_In_ PVOID Context, _In_ PVOID Memory)
{
    UNREFERENCED_PARAMETER(Context);

    if (Memory != NULL)
        ExFreePoolWithTag(Memory, VIRTIO_SND_POOL_TAG);
}

static NTSTATUS
VirtioSndSubmitEventSlot(_Inout_ PVIRTIO_SND_ADAPTER Adapter, _Inout_ PVIRTIO_SND_EVENT_SLOT Slot)
{
    struct scatterlist ScatterList;
    INT AddResult;

    ScatterList.physAddr = MmGetPhysicalAddress(&Slot->Event);
    ScatterList.length = sizeof(Slot->Event);

    AddResult = virtqueue_add_buf(Adapter->Queues[VIRTIO_SND_VQ_EVENT], &ScatterList, 0, 1, Slot, NULL, 0);
    if (AddResult < 0)
    {
        DbgPrint("virtiosnd: Failed to post event buffer (%d)\n", AddResult);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS
VirtioSndInitializeEventQueue(_Inout_ PVIRTIO_SND_ADAPTER Adapter)
{
    ULONG QueueSize;
    ULONG SlotIndex;

    QueueSize = virtio_get_queue_size(Adapter->Queues[VIRTIO_SND_VQ_EVENT]);
    Adapter->EventSlotCount = min(QueueSize, VIRTIO_SND_EVENT_SLOT_COUNT);
    if (Adapter->EventSlotCount == 0)
        return STATUS_DEVICE_CONFIGURATION_ERROR;

    Adapter->EventSlots = VirtioSndAllocateContiguous(NULL, Adapter->EventSlotCount * sizeof(*Adapter->EventSlots));
    if (Adapter->EventSlots == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    for (SlotIndex = 0; SlotIndex < Adapter->EventSlotCount; ++SlotIndex)
    {
        NTSTATUS Status = VirtioSndSubmitEventSlot(Adapter, &Adapter->EventSlots[SlotIndex]);
        if (!NT_SUCCESS(Status))
            return Status;
    }

    return STATUS_SUCCESS;
}

static VOID
VirtioSndHandleEvent(
    _In_opt_ PVIRTIO_SND_PLAYBACK Playback,
    _In_ const VIRTIO_SND_EVENT *Event,
    _Inout_ PULONG XrunCount,
    _In_ BOOLEAN RequireXrunFeature)
{
    if (Event->Header.Code == VIRTIO_SND_EVT_PCM_XRUN)
    {
        if (Playback != NULL && (!RequireXrunFeature || (Playback->Features & VIRTIO_SND_PCM_F_EVT_XRUNS) != 0) &&
            Event->Data == Playback->StreamId)
        {
            ++*XrunCount;
        }
        else
        {
            DbgPrint("virtiosnd: Ignoring PCM XRUN event for stream %lu\n", Event->Data);
        }
    }
    else
    {
        DbgPrint("virtiosnd: Ignoring event %08lx data %lu\n", Event->Header.Code, Event->Data);
    }
}

static NTSTATUS
VirtioSndDrainEventQueue(
    _Inout_ PVIRTIO_SND_ADAPTER Adapter,
    _In_opt_ PVIRTIO_SND_PLAYBACK Playback,
    _Out_ PULONG XrunCount)
{
    struct virtqueue *EventQueue = Adapter->Queues[VIRTIO_SND_VQ_EVENT];

    *XrunCount = 0;
    if (EventQueue == NULL)
        return STATUS_DEVICE_NOT_READY;

    for (;;)
    {
        PVIRTIO_SND_EVENT_SLOT Slot;
        unsigned int UsedLength;
        BOOLEAN Reposted = FALSE;

        while ((Slot = (PVIRTIO_SND_EVENT_SLOT)virtqueue_get_buf(EventQueue, &UsedLength)) != NULL)
        {
            if (UsedLength < sizeof(Slot->Event))
            {
                DbgPrint("virtiosnd: Short event response (%u bytes)\n", UsedLength);
            }
            else
            {
                VirtioSndHandleEvent(Playback, &Slot->Event, XrunCount, TRUE);
            }

            RtlZeroMemory(&Slot->Event, sizeof(Slot->Event));
            if (!NT_SUCCESS(VirtioSndSubmitEventSlot(Adapter, Slot)))
                return STATUS_INSUFFICIENT_RESOURCES;
            Reposted = TRUE;
        }

        if (Reposted && virtqueue_kick_prepare(EventQueue))
            virtqueue_notify(EventQueue);

        if (virtqueue_enable_cb(EventQueue))
            break;
    }

    return STATUS_SUCCESS;
}

static INT
VirtioSndReadPciConfig(_In_ PVIRTIO_SND_TRANSPORT Transport, _In_ INT Offset, _Out_ PVOID Buffer, _In_ ULONG Length)
{
    ULONG BytesRead;

    BytesRead = HalGetBusDataByOffset(
        PCIConfiguration, Transport->BusNumber, Transport->SlotNumber.u.AsULONG, Buffer, (ULONG)Offset, Length);

    return (BytesRead == Length) ? 0 : -1;
}

static INT
VirtioSndReadPciConfigByte(_In_ PVOID Context, _In_ INT Offset, _Out_ UCHAR *Value)
{
    return VirtioSndReadPciConfig(Context, Offset, Value, sizeof(*Value));
}

static INT
VirtioSndReadPciConfigWord(_In_ PVOID Context, _In_ INT Offset, _Out_ USHORT *Value)
{
    return VirtioSndReadPciConfig(Context, Offset, Value, sizeof(*Value));
}

static INT
VirtioSndReadPciConfigDword(_In_ PVOID Context, _In_ INT Offset, _Out_ ULONG *Value)
{
    return VirtioSndReadPciConfig(Context, Offset, Value, sizeof(*Value));
}

static size_t
VirtioSndGetBarLength(_In_ PVOID Context, _In_ INT BarIndex)
{
    PVIRTIO_SND_TRANSPORT Transport = Context;

    if (BarIndex < 0 || BarIndex >= PCI_TYPE0_ADDRESSES || !Transport->Bars[BarIndex].Assigned)
    {
        return 0;
    }

    return Transport->Bars[BarIndex].Length;
}

static PVOID
VirtioSndMapBar(_In_ PVOID Context, _In_ INT BarIndex, _In_ size_t Offset, _In_ size_t Length)
{
    PVIRTIO_SND_TRANSPORT Transport = Context;
    PVIRTIO_SND_BAR Bar;

    if (BarIndex < 0 || BarIndex >= PCI_TYPE0_ADDRESSES)
        return NULL;

    Bar = &Transport->Bars[BarIndex];
    if (!Bar->Assigned || Offset > Bar->Length || Length > Bar->Length - Offset)
        return NULL;

    if (Bar->IsPort)
    {
        ULONGLONG PortAddress = Bar->BaseAddress.QuadPart + Offset;

        if (PortAddress > VIRTIO_SND_IO_PORT_MAX)
            return NULL;

        return (PVOID)(ULONG_PTR)PortAddress;
    }

    if (Bar->MappedAddress == NULL)
    {
        Bar->MappedAddress = MmMapIoSpace(Bar->BaseAddress, Bar->Length, MmNonCached);
        if (Bar->MappedAddress == NULL)
            return NULL;
    }

    return (PUCHAR)Bar->MappedAddress + Offset;
}

static USHORT
VirtioSndGetMsixVector(_In_ PVOID Context, _In_ INT QueueIndex)
{
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(QueueIndex);

    return VIRTIO_MSI_NO_VECTOR;
}

static VOID
VirtioSndSleep(_In_ PVOID Context, _In_ UINT Milliseconds)
{
    LARGE_INTEGER Interval;

    UNREFERENCED_PARAMETER(Context);

    if (Milliseconds == 0)
        return;

    Interval.QuadPart = -((LONGLONG)Milliseconds * 10000);
    KeDelayExecutionThread(KernelMode, FALSE, &Interval);
}

static const VirtIOSystemOps VirtioSndSystemOps = {VirtioSndReadByte,           VirtioSndReadWord,
                                                   VirtioSndReadDword,          VirtioSndWriteByte,
                                                   VirtioSndWriteWord,          VirtioSndWriteDword,
                                                   VirtioSndAllocateContiguous, VirtioSndFreeContiguous,
                                                   VirtioSndGetPhysicalAddress, VirtioSndAllocateNonpaged,
                                                   VirtioSndFreeNonpaged,       VirtioSndReadPciConfigByte,
                                                   VirtioSndReadPciConfigWord,  VirtioSndReadPciConfigDword,
                                                   VirtioSndGetBarLength,       VirtioSndMapBar,
                                                   VirtioSndGetMsixVector,      VirtioSndSleep};

static PCM_PARTIAL_RESOURCE_DESCRIPTOR
VirtioSndFindBarResource(
    _In_ PRESOURCELIST ResourceList,
    _In_ CM_RESOURCE_TYPE ResourceType,
    _In_ ULONGLONG BaseAddress)
{
    ULONG Index;
    ULONG Count = ResourceList->lpVtbl->NumberOfEntriesOfType(ResourceList, ResourceType);

    for (Index = 0; Index < Count; ++Index)
    {
        PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor =
            ResourceList->lpVtbl->FindTranslatedEntry(ResourceList, ResourceType, Index);
        ULONGLONG ResourceAddress;

        if (Descriptor == NULL)
            continue;

        if (ResourceType == CmResourceTypePort)
            ResourceAddress = Descriptor->u.Port.Start.QuadPart;
        else
            ResourceAddress = Descriptor->u.Memory.Start.QuadPart;

        if (ResourceAddress == BaseAddress)
            return Descriptor;
    }

    return NULL;
}

static NTSTATUS
VirtioSndCaptureBars(
    _In_ PRESOURCELIST ResourceList,
    _In_ PPCI_COMMON_HEADER PciHeader,
    _Inout_ PVIRTIO_SND_TRANSPORT Transport)
{
    ULONG BarIndex;
    ULONG FoundBars = 0;

    for (BarIndex = 0; BarIndex < PCI_TYPE0_ADDRESSES; ++BarIndex)
    {
        ULONG CurrentBarIndex = BarIndex;
        ULONG RawBar = PciHeader->u.type0.BaseAddresses[BarIndex];
        ULONGLONG BaseAddress;
        BOOLEAN IsPort;
        CM_RESOURCE_TYPE ResourceType;
        PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor;
        PVIRTIO_SND_BAR Bar;

        if (RawBar == 0 || RawBar == MAXULONG)
            continue;

        IsPort = (RawBar & PCI_ADDRESS_IO_SPACE) != 0;
        ResourceType = IsPort ? CmResourceTypePort : CmResourceTypeMemory;
        if (IsPort)
        {
            BaseAddress = RawBar & ~0x3UL;
        }
        else
        {
            ULONG MemoryType = RawBar & PCI_ADDRESS_MEMORY_TYPE_MASK;

            if (MemoryType == 0x4)
            {
                if (BarIndex + 1 >= PCI_TYPE0_ADDRESSES)
                    return STATUS_DEVICE_CONFIGURATION_ERROR;

                BaseAddress = ((ULONGLONG)PciHeader->u.type0.BaseAddresses[BarIndex + 1] << 32) | (RawBar & ~0xFUL);
                ++BarIndex;
            }
            else if (MemoryType == 0 || MemoryType == 0x2)
            {
                BaseAddress = RawBar & ~0xFUL;
            }
            else
            {
                continue;
            }
        }

        Descriptor = VirtioSndFindBarResource(ResourceList, ResourceType, BaseAddress);
        if (Descriptor == NULL)
        {
            DbgPrint(
                "virtiosnd: BAR %lu base %08lx%08lx (%s) has no translated resource\n", CurrentBarIndex,
                (ULONG)(BaseAddress >> 32), (ULONG)BaseAddress, IsPort ? "port" : "memory");
            continue;
        }

        Bar = &Transport->Bars[CurrentBarIndex];
        Bar->BaseAddress = IsPort ? Descriptor->u.Port.Start : Descriptor->u.Memory.Start;
        Bar->Length = IsPort ? Descriptor->u.Port.Length : Descriptor->u.Memory.Length;
        Bar->IsPort = IsPort;
        Bar->Assigned = (Bar->Length != 0);
        if (Bar->Assigned)
        {
            ++FoundBars;
            DbgPrint(
                "virtiosnd: BAR %lu assigned %s base %08lx%08lx length %lu\n", CurrentBarIndex,
                IsPort ? "port" : "memory", (ULONG)(Bar->BaseAddress.QuadPart >> 32), (ULONG)Bar->BaseAddress.QuadPart,
                (ULONG)Bar->Length);
        }
    }

    return FoundBars ? STATUS_SUCCESS : STATUS_DEVICE_CONFIGURATION_ERROR;
}

static NTSTATUS
VirtioSndGetPciSlotNumber(_In_ PDEVICE_OBJECT PhysicalDeviceObject, _Out_ PPCI_SLOT_NUMBER SlotNumber)
{
    ULONG Address;
    ULONG BytesReturned;
    ULONG DeviceNumber;
    ULONG FunctionNumber;
    WCHAR LocationInformation[128];
    PWCHAR DeviceText;
    PWCHAR FunctionText;
    UNICODE_STRING NumberString;
    NTSTATUS Status;

    Status =
        IoGetDeviceProperty(PhysicalDeviceObject, DevicePropertyAddress, sizeof(Address), &Address, &BytesReturned);
    if (NT_SUCCESS(Status))
    {
        DeviceNumber = (Address >> 16) & 0xFFFF;
        FunctionNumber = Address & 0xFFFF;
    }
    else
    {
        DbgPrint("virtiosnd: PCI address property unavailable (%08lx). Parsing location information\n", (ULONG)Status);

        RtlZeroMemory(LocationInformation, sizeof(LocationInformation));
        Status = IoGetDeviceProperty(
            PhysicalDeviceObject, DevicePropertyLocationInformation, sizeof(LocationInformation), LocationInformation,
            &BytesReturned);
        if (!NT_SUCCESS(Status))
        {
            DbgPrint("virtiosnd: PCI location information query failed with status %08lx\n", (ULONG)Status);
            return Status;
        }

        LocationInformation[RTL_NUMBER_OF(LocationInformation) - 1] = UNICODE_NULL;
        DeviceText = wcsstr(LocationInformation, L"Device ");
        FunctionText = wcsstr(LocationInformation, L"Function ");
        if (DeviceText == NULL || FunctionText == NULL)
            return STATUS_DEVICE_CONFIGURATION_ERROR;

        RtlInitUnicodeString(&NumberString, DeviceText + RTL_NUMBER_OF(L"Device ") - 1);
        Status = RtlUnicodeStringToInteger(&NumberString, 10, &DeviceNumber);
        if (!NT_SUCCESS(Status))
            return Status;

        RtlInitUnicodeString(&NumberString, FunctionText + RTL_NUMBER_OF(L"Function ") - 1);
        Status = RtlUnicodeStringToInteger(&NumberString, 10, &FunctionNumber);
        if (!NT_SUCCESS(Status))
            return Status;
    }

    if (DeviceNumber > 0x1F || FunctionNumber > 0x7)
        return STATUS_DEVICE_CONFIGURATION_ERROR;

    RtlZeroMemory(SlotNumber, sizeof(*SlotNumber));
    SlotNumber->u.bits.DeviceNumber = DeviceNumber;
    SlotNumber->u.bits.FunctionNumber = FunctionNumber;
    return STATUS_SUCCESS;
}

static NTSTATUS
VirtioSndInitializePciResources(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PRESOURCELIST ResourceList,
    _Out_ PVIRTIO_SND_TRANSPORT Transport)
{
    PDEVICE_OBJECT PhysicalDeviceObject;
    PCM_RESOURCE_LIST TranslatedResources;
    PCI_COMMON_HEADER PciHeader;
    ULONG BytesReturned;
    NTSTATUS Status;

    RtlZeroMemory(Transport, sizeof(*Transport));
    PhysicalDeviceObject = IoGetDeviceAttachmentBaseRef(DeviceObject);
    if (PhysicalDeviceObject == NULL)
        return STATUS_INVALID_DEVICE_STATE;

    TranslatedResources = ResourceList->lpVtbl->TranslatedList(ResourceList);
    if (TranslatedResources == NULL || TranslatedResources->Count == 0)
    {
        DbgPrint("virtiosnd: Translated PCI resource list is empty\n");
        Status = STATUS_DEVICE_CONFIGURATION_ERROR;
        goto Exit;
    }

    Transport->BusNumber = TranslatedResources->List[0].BusNumber;
    Status = VirtioSndGetPciSlotNumber(PhysicalDeviceObject, &Transport->SlotNumber);
    if (!NT_SUCCESS(Status))
        goto Exit;

    DbgPrint(
        "virtiosnd: using PCI bus %lu slot %08lx from assigned resources\n", Transport->BusNumber,
        Transport->SlotNumber.u.AsULONG);
    RtlZeroMemory(&PciHeader, sizeof(PciHeader));
    BytesReturned = HalGetBusDataByOffset(
        PCIConfiguration, Transport->BusNumber, Transport->SlotNumber.u.AsULONG, &PciHeader, 0, sizeof(PciHeader));
    if (BytesReturned != sizeof(PciHeader))
    {
        DbgPrint(
            "virtiosnd: PCI config read failed (bus %lu slot %08lx bytes %lu)\n", Transport->BusNumber,
            Transport->SlotNumber.u.AsULONG, BytesReturned);
        Status = STATUS_DEVICE_CONFIGURATION_ERROR;
        goto Exit;
    }

    if (PciHeader.VendorID != VIRTIO_SND_PCI_VENDOR_ID || PciHeader.DeviceID != VIRTIO_SND_PCI_DEVICE_ID)
    {
        DbgPrint(
            "virtiosnd: unexpected PCI ID %04x:%04x (bus %lu slot %08lx)\n", PciHeader.VendorID, PciHeader.DeviceID,
            Transport->BusNumber, Transport->SlotNumber.u.AsULONG);
        Status = STATUS_NO_SUCH_DEVICE;
        goto Exit;
    }

    if ((PciHeader.HeaderType & ~PCI_MULTIFUNCTION) != PCI_DEVICE_TYPE)
    {
        DbgPrint("virtiosnd: unsupported PCI header type %02x\n", PciHeader.HeaderType);
        Status = STATUS_DEVICE_CONFIGURATION_ERROR;
        goto Exit;
    }

    Status = VirtioSndCaptureBars(ResourceList, &PciHeader, Transport);
    if (!NT_SUCCESS(Status))
        DbgPrint("virtiosnd: BAR/resource matching failed with status %08lx\n", (ULONG)Status);

Exit:
    ObDereferenceObject(PhysicalDeviceObject);
    return Status;
}

static VOID
VirtioSndReleasePciResources(_Inout_ PVIRTIO_SND_TRANSPORT Transport)
{
    ULONG BarIndex;

    for (BarIndex = 0; BarIndex < PCI_TYPE0_ADDRESSES; ++BarIndex)
    {
        PVIRTIO_SND_BAR Bar = &Transport->Bars[BarIndex];

        if (Bar->MappedAddress != NULL)
        {
            MmUnmapIoSpace(Bar->MappedAddress, Bar->Length);
            Bar->MappedAddress = NULL;
        }
    }
}

static NTSTATUS
VirtioSndConnectInterrupt(_Inout_ PVIRTIO_SND_ADAPTER Adapter, _In_ PRESOURCELIST ResourceList)
{
    PCM_PARTIAL_RESOURCE_DESCRIPTOR InterruptResource;
    KINTERRUPT_MODE InterruptMode;

    InterruptResource = ResourceList->lpVtbl->FindTranslatedEntry(ResourceList, CmResourceTypeInterrupt, 0);
    if (InterruptResource == NULL)
    {
        DbgPrint("virtiosnd: No translated interrupt resource was assigned\n");
        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    InterruptMode = (InterruptResource->Flags & CM_RESOURCE_INTERRUPT_LATCHED) != 0 ? Latched : LevelSensitive;

    return IoConnectInterrupt(
        &Adapter->InterruptObject, VirtioSndInterruptService, Adapter, NULL, InterruptResource->u.Interrupt.Vector,
        (KIRQL)InterruptResource->u.Interrupt.Level, (KIRQL)InterruptResource->u.Interrupt.Level, InterruptMode,
        InterruptResource->ShareDisposition == CmResourceShareShared, InterruptResource->u.Interrupt.Affinity, FALSE);
}

static VOID
VirtioSndShutdownAdapter(_Inout_ PVIRTIO_SND_ADAPTER Adapter)
{
    InterlockedExchange(&Adapter->Stopping, TRUE);

    if (Adapter->QueuesInitialized)
    {
        ULONG QueueIndex;
        for (QueueIndex = 0; QueueIndex < VIRTIO_SND_VQ_MAX; ++QueueIndex)
        {
            if (Adapter->Queues[QueueIndex] != NULL)
                virtqueue_disable_cb(Adapter->Queues[QueueIndex]);
        }
    }

    if (Adapter->InterruptObject != NULL)
    {
        IoDisconnectInterrupt(Adapter->InterruptObject);
        Adapter->InterruptObject = NULL;
        KeRemoveQueueDpc(&Adapter->InterruptDpc);
        KeFlushQueuedDpcs();
    }

    if (Adapter->PlaybackContext != NULL)
    {
        Adapter->PlaybackContext->DeviceActive = FALSE;
        Adapter->PlaybackContext->Prepared = FALSE;
        Adapter->PlaybackContext->Started = FALSE;
        Adapter->PlaybackContext->WakeEvent = NULL;
    }

    if (Adapter->DeviceInitialized)
        virtio_device_reset(&Adapter->Device);

    if (Adapter->QueuesInitialized)
        virtio_delete_queues(&Adapter->Device);

    if (Adapter->OutstandingBuffer != NULL)
        VirtioSndFreeContiguous(NULL, Adapter->OutstandingBuffer);

    if (Adapter->PcmInfo != NULL)
        VirtioSndFreeNonpaged(NULL, Adapter->PcmInfo);

    if (Adapter->ChannelMapInfo != NULL)
        VirtioSndFreeNonpaged(NULL, Adapter->ChannelMapInfo);

    if (Adapter->EventSlots != NULL)
        VirtioSndFreeContiguous(NULL, Adapter->EventSlots);

    if (Adapter->DeviceInitializeAttempted)
        virtio_device_shutdown(&Adapter->Device);

    VirtioSndReleasePciResources(&Adapter->Transport);
    RtlZeroMemory(Adapter, sizeof(*Adapter));
}

VOID
VirtioSndStopTransport(_In_opt_ PDEVICE_OBJECT DeviceObject)
{
    PVIRTIO_SND_ADAPTER Adapter = &VirtioSndAdapter;
    PKEVENT WakeEvent = NULL;
    BOOLEAN WaitForPlayback = FALSE;

    VirtioSndInitializeSynchronization();
    VirtioSndAcquireLifecycleLock();

    if (Adapter->OwnerDeviceObject == NULL || (DeviceObject != NULL && Adapter->OwnerDeviceObject != DeviceObject))
    {
        VirtioSndReleaseLifecycleLock();
        return;
    }

    InterlockedExchange(&Adapter->Stopping, TRUE);
    if (Adapter->PlaybackContext != NULL)
    {
        WakeEvent = Adapter->PlaybackContext->WakeEvent;
        WaitForPlayback = !KeReadStateEvent(&VirtioSndPlaybackQuiesced);
    }

    if (WakeEvent != NULL)
        KeSetEvent(WakeEvent, IO_NO_INCREMENT, FALSE);
    VirtioSndReleaseLifecycleLock();

    if (WaitForPlayback)
    {
        KeWaitForSingleObject(&VirtioSndPlaybackQuiesced, Executive, KernelMode, FALSE, NULL);
    }

    VirtioSndAcquireLifecycleLock();
    if (Adapter->OwnerDeviceObject != NULL && (DeviceObject == NULL || Adapter->OwnerDeviceObject == DeviceObject))
    {
        VirtioSndShutdownAdapter(Adapter);
    }
    VirtioSndReleaseLifecycleLock();
    KeSetEvent(&VirtioSndTransportStopped, IO_NO_INCREMENT, FALSE);
}

static NTSTATUS
VirtioSndQueryPcmInfo(_Inout_ PVIRTIO_SND_ADAPTER Adapter)
{
    const SIZE_T StatusOffset = sizeof(VIRTIO_SND_QUERY_INFO);
    const SIZE_T InfoOffset = (sizeof(VIRTIO_SND_QUERY_INFO) + sizeof(VIRTIO_SND_HDR) + 7) & ~(SIZE_T)7;
    struct virtqueue *ControlQueue = Adapter->Queues[VIRTIO_SND_VQ_CONTROL];
    ULONG StreamCount = Adapter->Config.Streams;
    SIZE_T InfoLength;
    SIZE_T BufferLength;
    PVOID Buffer;
    PVIRTIO_SND_QUERY_INFO Request;
    PULONG ResponseStatus;
    PVIRTIO_SND_PCM_INFO StreamInfo;
    struct scatterlist ScatterList[3];
    PHYSICAL_ADDRESS PhysicalAddress;
    PVOID CompletedBuffer = NULL;
    unsigned int UsedLength = 0;
    ULONG Retry;
    INT AddResult;
    NTSTATUS Status = STATUS_SUCCESS;

    if (StreamCount == 0)
        return STATUS_SUCCESS;

    if (ControlQueue == NULL)
        return STATUS_INVALID_DEVICE_STATE;

    if ((SIZE_T)StreamCount > (MAXULONG - InfoOffset) / sizeof(VIRTIO_SND_PCM_INFO))
    {
        return STATUS_INTEGER_OVERFLOW;
    }

    InfoLength = (SIZE_T)StreamCount * sizeof(VIRTIO_SND_PCM_INFO);
    BufferLength = InfoOffset + InfoLength;
    DbgPrint(
        "virtiosnd: Querying PCM info for %lu streams (record %lu bytes, buffer %lu bytes)\n", StreamCount,
        (ULONG)sizeof(VIRTIO_SND_PCM_INFO), (ULONG)BufferLength);
    Buffer = VirtioSndAllocateContiguous(NULL, BufferLength);
    if (Buffer == NULL)
    {
        DbgPrint("virtiosnd: Contiguous PCM query buffer allocation failed\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Request = (PVIRTIO_SND_QUERY_INFO)Buffer;
    Request->Header.Code = VIRTIO_SND_R_PCM_INFO;
    Request->StartId = 0;
    Request->Count = StreamCount;
    Request->Size = sizeof(VIRTIO_SND_PCM_INFO);

    virtqueue_disable_cb(ControlQueue);
    DbgPrint("virtiosnd: Control queue interrupts disabled for polling\n");

    ResponseStatus = (PULONG)((PUCHAR)Buffer + StatusOffset);
    *ResponseStatus = MAXULONG;
    StreamInfo = (PVIRTIO_SND_PCM_INFO)((PUCHAR)Buffer + InfoOffset);

    PhysicalAddress = MmGetPhysicalAddress(Buffer);
    ScatterList[0].physAddr = PhysicalAddress;
    ScatterList[0].length = sizeof(*Request);
    ScatterList[1].physAddr = PhysicalAddress;
    ScatterList[1].physAddr.QuadPart += StatusOffset;
    ScatterList[1].length = sizeof(*ResponseStatus);
    ScatterList[2].physAddr = PhysicalAddress;
    ScatterList[2].physAddr.QuadPart += InfoOffset;
    ScatterList[2].length = (ULONG)InfoLength;

    AddResult = virtqueue_add_buf(ControlQueue, ScatterList, 1, 2, Buffer, NULL, 0);
    if (AddResult < 0)
    {
        DbgPrint("virtiosnd: Failed to enqueue PCM info request (%d)\n", AddResult);
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    DbgPrint(
        "virtiosnd: PCM info request enqueued. Kick prepare (notify %p callback %p)\n", ControlQueue->notification_addr,
        ControlQueue->notification_cb);
    if (virtqueue_kick_prepare(ControlQueue))
    {
        DbgPrint("virtiosnd: Control queue kick requires notification\n");
        VirtioSndTraceNotifyAddress = (ULONG_PTR)ControlQueue->notification_addr;
        virtqueue_notify(ControlQueue);
        VirtioSndTraceNotifyAddress = 0;
        DbgPrint("virtiosnd: Control queue notification returned\n");
    }
    else
    {
        DbgPrint("virtiosnd: Control queue notification suppressed\n");
    }
    DbgPrint("virtiosnd: Control queue kick completed\n");
    for (Retry = 0; Retry < VIRTIO_SND_CONTROL_TIMEOUT_MS; ++Retry)
    {
        CompletedBuffer = virtqueue_get_buf(ControlQueue, &UsedLength);
        if (CompletedBuffer != NULL)
            break;

        VirtioSndSleep(NULL, 1);
    }

    if (CompletedBuffer == NULL)
    {
        DbgPrint("virtiosnd: PCM info request timed out\n");
        Adapter->OutstandingBuffer = Buffer;
        return STATUS_IO_TIMEOUT;
    }

    if (CompletedBuffer != Buffer)
    {
        DbgPrint("virtiosnd: Unexpected PCM info completion token %p\n", CompletedBuffer);
        Adapter->OutstandingBuffer = Buffer;
        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    if (UsedLength < sizeof(*ResponseStatus) + InfoLength)
    {
        DbgPrint(
            "virtiosnd: Short PCM info response (%lu bytes, expected %lu)\n", (ULONG)UsedLength,
            (ULONG)(sizeof(*ResponseStatus) + InfoLength));
        Status = STATUS_DEVICE_CONFIGURATION_ERROR;
        goto Exit;
    }

    DbgPrint("virtiosnd: PCM info response received (%u bytes, status %08lx)\n", UsedLength, *ResponseStatus);
    if (*ResponseStatus != VIRTIO_SND_S_OK)
    {
        DbgPrint("virtiosnd: PCM info request returned Status %08lx\n", *ResponseStatus);
        Status = STATUS_UNSUCCESSFUL;
        goto Exit;
    }

    for (Retry = 0; Retry < StreamCount; ++Retry)
    {
        PVIRTIO_SND_PCM_INFO Info = &StreamInfo[Retry];

        if (Info->Direction > 1 || Info->ChannelsMin == 0 || Info->ChannelsMin > Info->ChannelsMax)
        {
            DbgPrint(
                "virtiosnd: Invalid PCM info for stream %lu (direction %u channels %u-%u)\n", Retry, Info->Direction,
                Info->ChannelsMin, Info->ChannelsMax);
            Status = STATUS_DEVICE_CONFIGURATION_ERROR;
            goto Exit;
        }

        DbgPrint(
            "virtiosnd: PCM Stream %lu nid %lu direction %u channels %u-%u features %08lx "
            "formats %08lx%08lx rates %08lx%08lx\n",
            Retry, Info->Header.HdaFunctionNodeId, Info->Direction, Info->ChannelsMin, Info->ChannelsMax,
            Info->Features, (ULONG)(Info->Formats >> 32), (ULONG)Info->Formats, (ULONG)(Info->Rates >> 32),
            (ULONG)Info->Rates);
    }

    Adapter->PcmInfo = VirtioSndAllocateNonpaged(NULL, InfoLength);
    if (Adapter->PcmInfo == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    RtlCopyMemory(Adapter->PcmInfo, StreamInfo, InfoLength);
    Adapter->PcmInfoCount = StreamCount;

Exit:
    VirtioSndFreeContiguous(NULL, Buffer);
    return Status;
}

static NTSTATUS
VirtioSndQueryChannelMaps(_Inout_ PVIRTIO_SND_ADAPTER Adapter)
{
    const SIZE_T StatusOffset = sizeof(VIRTIO_SND_QUERY_INFO);
    const SIZE_T InfoOffset = (sizeof(VIRTIO_SND_QUERY_INFO) + sizeof(VIRTIO_SND_HDR) + 7) & ~(SIZE_T)7;
    struct virtqueue *ControlQueue = Adapter->Queues[VIRTIO_SND_VQ_CONTROL];
    ULONG MapCount = Adapter->Config.ChannelMaps;
    SIZE_T InfoLength;
    SIZE_T BufferLength;
    PVOID Buffer;
    PVIRTIO_SND_QUERY_INFO Request;
    PULONG ResponseStatus;
    PVIRTIO_SND_CHMAP_INFO MapInfo;
    struct scatterlist ScatterList[3];
    PHYSICAL_ADDRESS PhysicalAddress;
    PVOID CompletedBuffer = NULL;
    unsigned int UsedLength = 0;
    ULONG Retry;
    ULONG PositionIndex;
    INT AddResult;
    NTSTATUS Status = STATUS_SUCCESS;

    if (MapCount == 0)
        return STATUS_SUCCESS;
    if (ControlQueue == NULL)
        return STATUS_INVALID_DEVICE_STATE;
    if ((SIZE_T)MapCount > (MAXULONG - InfoOffset) / sizeof(VIRTIO_SND_CHMAP_INFO))
    {
        return STATUS_INTEGER_OVERFLOW;
    }

    InfoLength = (SIZE_T)MapCount * sizeof(VIRTIO_SND_CHMAP_INFO);
    BufferLength = InfoOffset + InfoLength;
    Buffer = VirtioSndAllocateContiguous(NULL, BufferLength);
    if (Buffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    Request = (PVIRTIO_SND_QUERY_INFO)Buffer;
    Request->Header.Code = VIRTIO_SND_R_CHMAP_INFO;
    Request->StartId = 0;
    Request->Count = MapCount;
    Request->Size = sizeof(VIRTIO_SND_CHMAP_INFO);

    ResponseStatus = (PULONG)((PUCHAR)Buffer + StatusOffset);
    *ResponseStatus = MAXULONG;
    MapInfo = (PVIRTIO_SND_CHMAP_INFO)((PUCHAR)Buffer + InfoOffset);

    PhysicalAddress = MmGetPhysicalAddress(Buffer);
    ScatterList[0].physAddr = PhysicalAddress;
    ScatterList[0].length = sizeof(*Request);
    ScatterList[1].physAddr = PhysicalAddress;
    ScatterList[1].physAddr.QuadPart += StatusOffset;
    ScatterList[1].length = sizeof(*ResponseStatus);
    ScatterList[2].physAddr = PhysicalAddress;
    ScatterList[2].physAddr.QuadPart += InfoOffset;
    ScatterList[2].length = (ULONG)InfoLength;

    virtqueue_disable_cb(ControlQueue);
    AddResult = virtqueue_add_buf(ControlQueue, ScatterList, 1, 2, Buffer, NULL, 0);
    if (AddResult < 0)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    if (virtqueue_kick_prepare(ControlQueue))
        virtqueue_notify(ControlQueue);

    for (Retry = 0; Retry < VIRTIO_SND_CONTROL_TIMEOUT_MS; ++Retry)
    {
        CompletedBuffer = virtqueue_get_buf(ControlQueue, &UsedLength);
        if (CompletedBuffer != NULL)
            break;
        VirtioSndSleep(NULL, 1);
    }

    if (CompletedBuffer == NULL)
    {
        Adapter->OutstandingBuffer = Buffer;
        return STATUS_IO_TIMEOUT;
    }
    if (CompletedBuffer != Buffer)
    {
        Adapter->OutstandingBuffer = Buffer;
        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }
    if (UsedLength < sizeof(*ResponseStatus))
    {
        Status = STATUS_DEVICE_CONFIGURATION_ERROR;
        goto Exit;
    }
    if (*ResponseStatus == VIRTIO_SND_S_NOT_SUPP)
    {
        Status = STATUS_NOT_SUPPORTED;
        goto Exit;
    }
    if (*ResponseStatus != VIRTIO_SND_S_OK)
    {
        Status = STATUS_UNSUCCESSFUL;
        goto Exit;
    }
    if (UsedLength < sizeof(*ResponseStatus) + InfoLength)
    {
        Status = STATUS_DEVICE_CONFIGURATION_ERROR;
        goto Exit;
    }

    for (Retry = 0; Retry < MapCount; ++Retry)
    {
        PVIRTIO_SND_CHMAP_INFO Info = &MapInfo[Retry];

        if (Info->Direction > VIRTIO_SND_D_INPUT || Info->Channels == 0 || Info->Channels > VIRTIO_SND_CHMAP_MAX_SIZE)
        {
            Status = STATUS_DEVICE_CONFIGURATION_ERROR;
            goto Exit;
        }

        for (PositionIndex = 0; PositionIndex < Info->Channels; ++PositionIndex)
        {
            if (Info->Positions[PositionIndex] > VIRTIO_SND_CHMAP_BRC)
            {
                Status = STATUS_DEVICE_CONFIGURATION_ERROR;
                goto Exit;
            }
        }

        DbgPrint(
            "virtiosnd: Channel map %lu nid %lu direction %u channels %u positions %u,%u,%u,%u\n", Retry,
            Info->Header.HdaFunctionNodeId, Info->Direction, Info->Channels, Info->Positions[0], Info->Positions[1],
            Info->Positions[2], Info->Positions[3]);
    }

    Adapter->ChannelMapInfo = VirtioSndAllocateNonpaged(NULL, InfoLength);
    if (Adapter->ChannelMapInfo == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    RtlCopyMemory(Adapter->ChannelMapInfo, MapInfo, InfoLength);
    Adapter->ChannelMapInfoCount = MapCount;

Exit:
    VirtioSndFreeContiguous(NULL, Buffer);
    if (!NT_SUCCESS(Status))
    {
        DbgPrint("virtiosnd: Channel-map query failed (%08lx)\n", (ULONG)Status);
    }
    return Status;
}

NTSTATUS
VirtioSndGetPlaybackChannelMap(_In_ ULONG Channels, _Out_writes_bytes_(Channels) PUCHAR Positions)
{
    PVIRTIO_SND_ADAPTER Adapter = &VirtioSndAdapter;
    PVIRTIO_SND_CHMAP_INFO Match = NULL;
    ULONG MapIndex;

    PAGED_CODE();

    if (Channels == 0 || Channels > VIRTIO_SND_CHMAP_MAX_SIZE || Positions == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }
    if (!Adapter->Started || VirtioSndTransportStopping())
        return STATUS_DEVICE_NOT_READY;
    if (Adapter->ChannelMapInfo == NULL)
        return STATUS_NOT_SUPPORTED;

    for (MapIndex = 0; MapIndex < Adapter->ChannelMapInfoCount; ++MapIndex)
    {
        PVIRTIO_SND_CHMAP_INFO Candidate = &Adapter->ChannelMapInfo[MapIndex];

        if (Candidate->Direction != VIRTIO_SND_D_OUTPUT || Candidate->Channels != Channels)
        {
            continue;
        }

        if (Match != NULL && RtlCompareMemory(Match->Positions, Candidate->Positions, Channels) != Channels)
        {
            return STATUS_NO_MATCH;
        }
        Match = Candidate;
    }

    if (Match == NULL)
        return STATUS_NOT_SUPPORTED;

    RtlCopyMemory(Positions, Match->Positions, Channels);
    return STATUS_SUCCESS;
}

NTSTATUS
VirtioSndGetPlaybackRates(_In_ ULONG Channels, _In_ ULONG BitsPerSample, _Out_ ULONGLONG *Rates)
{
    PVIRTIO_SND_ADAPTER Adapter = &VirtioSndAdapter;
    ULONG Format;
    ULONGLONG FormatBit;
    ULONG StreamId;
    NTSTATUS Status = STATUS_NOT_SUPPORTED;

    PAGED_CODE();

    if (Rates == NULL || Channels == 0 || Channels > 2)
        return STATUS_INVALID_PARAMETER;

    if (BitsPerSample == 16)
        Format = VIRTIO_SND_PCM_FMT_S16;
    else if (BitsPerSample == 32)
        Format = VIRTIO_SND_PCM_FMT_S32;
    else
        return STATUS_NOT_SUPPORTED;

    FormatBit = (ULONGLONG)1 << Format;
    *Rates = 0;
    VirtioSndInitializeSynchronization();
    VirtioSndAcquireLifecycleLock();
    if (!Adapter->Started || VirtioSndTransportStopping())
    {
        Status = STATUS_DEVICE_NOT_READY;
        goto Exit;
    }

    for (StreamId = 0; StreamId < Adapter->PcmInfoCount; ++StreamId)
    {
        PVIRTIO_SND_PCM_INFO Info = &Adapter->PcmInfo[StreamId];

        if (Info->Direction == VIRTIO_SND_D_OUTPUT && (Info->Formats & FormatBit) != 0 &&
            Info->ChannelsMin <= Channels && Info->ChannelsMax >= Channels)
        {
            *Rates |= Info->Rates;
        }
    }

    if (*Rates != 0)
        Status = STATUS_SUCCESS;

Exit:
    VirtioSndReleaseLifecycleLock();
    return Status;
}

static NTSTATUS
VirtioSndGetRateIndex(_In_ ULONG SampleRate, _Out_ PULONG RateIndex)
{
    if (RateIndex == NULL)
        return STATUS_INVALID_PARAMETER;

    switch (SampleRate)
    {
        case 44100:
            *RateIndex = VIRTIO_SND_PCM_RATE_44100;
            return STATUS_SUCCESS;
        case 48000:
            *RateIndex = VIRTIO_SND_PCM_RATE_48000;
            return STATUS_SUCCESS;
        case 96000:
            *RateIndex = VIRTIO_SND_PCM_RATE_96000;
            return STATUS_SUCCESS;
        default:
            return STATUS_NOT_SUPPORTED;
    }
}

static NTSTATUS
VirtioSndSubmitControlRequest(
    _Inout_ PVIRTIO_SND_ADAPTER Adapter,
    _In_reads_bytes_(RequestLength) PVOID Request,
    _In_ SIZE_T RequestLength)
{
    struct virtqueue *ControlQueue = Adapter->Queues[VIRTIO_SND_VQ_CONTROL];
    struct scatterlist ScatterList[2];
    PHYSICAL_ADDRESS PhysicalAddress;
    PVOID Buffer;
    PULONG ResponseStatus;
    PVOID CompletedBuffer = NULL;
    SIZE_T BufferLength;
    ULONG Retry;
    unsigned int UsedLength = 0;
    INT AddResult;
    NTSTATUS Status = STATUS_SUCCESS;

    if (ControlQueue == NULL || Request == NULL || RequestLength == 0)
        return STATUS_INVALID_PARAMETER;

    if (RequestLength > MAXULONG - sizeof(*ResponseStatus))
        return STATUS_INTEGER_OVERFLOW;

    BufferLength = RequestLength + sizeof(*ResponseStatus);
    Buffer = VirtioSndAllocateContiguous(NULL, BufferLength);
    if (Buffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlCopyMemory(Buffer, Request, RequestLength);
    ResponseStatus = (PULONG)((PUCHAR)Buffer + RequestLength);
    *ResponseStatus = MAXULONG;

    PhysicalAddress = MmGetPhysicalAddress(Buffer);
    ScatterList[0].physAddr = PhysicalAddress;
    ScatterList[0].length = (ULONG)RequestLength;
    ScatterList[1].physAddr = PhysicalAddress;
    ScatterList[1].physAddr.QuadPart += RequestLength;
    ScatterList[1].length = sizeof(*ResponseStatus);

    virtqueue_disable_cb(ControlQueue);
    AddResult = virtqueue_add_buf(ControlQueue, ScatterList, 1, 1, Buffer, NULL, 0);
    if (AddResult < 0)
    {
        DbgPrint(
            "virtiosnd: failed to enqueue control request (%08lx, %d)\n", ((PVIRTIO_SND_HDR)Request)->Code, AddResult);
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    if (virtqueue_kick_prepare(ControlQueue))
        virtqueue_notify(ControlQueue);

    for (Retry = 0; Retry < VIRTIO_SND_CONTROL_TIMEOUT_MS; ++Retry)
    {
        CompletedBuffer = virtqueue_get_buf(ControlQueue, &UsedLength);
        if (CompletedBuffer != NULL)
            break;

        VirtioSndSleep(NULL, 1);
    }

    if (CompletedBuffer == NULL)
    {
        DbgPrint("virtiosnd: Control request %08lx timed out\n", ((PVIRTIO_SND_HDR)Request)->Code);
        Adapter->OutstandingBuffer = Buffer;
        return STATUS_IO_TIMEOUT;
    }

    if (CompletedBuffer != Buffer)
    {
        DbgPrint("virtiosnd: Unexpected control completion token %p\n", CompletedBuffer);
        Adapter->OutstandingBuffer = Buffer;
        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    if (UsedLength < sizeof(*ResponseStatus))
    {
        DbgPrint(
            "virtiosnd: Short control response for request %08lx (%lu bytes)\n", ((PVIRTIO_SND_HDR)Request)->Code,
            UsedLength);
        Status = STATUS_DEVICE_CONFIGURATION_ERROR;
        goto Exit;
    }

    if (*ResponseStatus == VIRTIO_SND_S_NOT_SUPP)
        Status = STATUS_NOT_SUPPORTED;
    else if (*ResponseStatus != VIRTIO_SND_S_OK)
        Status = STATUS_UNSUCCESSFUL;

    DbgPrint(
        "virtiosnd: Control request %08lx completed with device status %08lx (NTSTATUS %08lx)\n",
        ((PVIRTIO_SND_HDR)Request)->Code, *ResponseStatus, (ULONG)Status);

Exit:
    VirtioSndFreeContiguous(NULL, Buffer);
    return Status;
}

static NTSTATUS
VirtioSndSendPcmCommand(_Inout_ PVIRTIO_SND_ADAPTER Adapter, _In_ ULONG Code, _In_ ULONG StreamId)
{
    VIRTIO_SND_PCM_HDR Request;

    RtlZeroMemory(&Request, sizeof(Request));
    Request.Header.Code = Code;
    Request.StreamId = StreamId;

    return VirtioSndSubmitControlRequest(Adapter, &Request, sizeof(Request));
}

NTSTATUS
VirtioSndOpenPlayback(
    _In_ ULONG Channels,
    _In_ ULONG BitsPerSample,
    _In_ ULONG SampleRate,
    _In_ PKEVENT WakeEvent,
    _Out_ PVIRTIO_SND_PLAYBACK *Playback)
{
    PVIRTIO_SND_ADAPTER Adapter = &VirtioSndAdapter;
    PVIRTIO_SND_PLAYBACK Context;
    VIRTIO_SND_PCM_SET_PARAMS Request;
    ULONG StreamId;
    ULONG RateIndex;
    ULONG Format;
    ULONG SlotIndex;
    ULONG StreamFeatures = 0;
    KIRQL OldIrql;
    NTSTATUS Status;

    PAGED_CODE();

    if (Playback == NULL || WakeEvent == NULL || (Channels != 1 && Channels != 2) ||
        (BitsPerSample != 16 && BitsPerSample != 32))
        return STATUS_INVALID_PARAMETER;

    Format = BitsPerSample == 16 ? VIRTIO_SND_PCM_FMT_S16 : VIRTIO_SND_PCM_FMT_S32;

    Status = VirtioSndGetRateIndex(SampleRate, &RateIndex);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = STATUS_NOT_SUPPORTED;
    *Playback = NULL;
    VirtioSndInitializeSynchronization();
    VirtioSndAcquireLifecycleLock();
    if (!Adapter->Started || VirtioSndTransportStopping() || Adapter->Queues[VIRTIO_SND_VQ_TX] == NULL)
    {
        Status = STATUS_DEVICE_NOT_READY;
        goto Failure;
    }

    if (InterlockedCompareExchange(&Adapter->PlaybackOpen, 1, 0) != 0)
    {
        Status = STATUS_DEVICE_BUSY;
        goto Failure;
    }

    for (StreamId = 0; StreamId < Adapter->PcmInfoCount; ++StreamId)
    {
        PVIRTIO_SND_PCM_INFO Info = &Adapter->PcmInfo[StreamId];
        ULONGLONG FormatBit = (ULONGLONG)1 << Format;
        ULONGLONG RateBit = (ULONGLONG)1 << RateIndex;

        if (Info->Direction == 0 && (Info->Formats & FormatBit) != 0 && (Info->Rates & RateBit) != 0 &&
            Info->ChannelsMin <= Channels && Info->ChannelsMax >= Channels)
        {
            StreamFeatures = Info->Features & VIRTIO_SND_PCM_F_EVT_XRUNS;
            break;
        }
    }

    if (StreamId == Adapter->PcmInfoCount)
        goto Failure;

    Context = VirtioSndAllocateNonpaged(NULL, sizeof(*Context));
    if (Context == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Failure;
    }

    Context->StreamId = StreamId;
    Context->Channels = Channels;
    Context->Features = StreamFeatures;
    Context->WakeEvent = WakeEvent;
    for (SlotIndex = 0; SlotIndex < VIRTIO_SND_TX_SLOT_COUNT; ++SlotIndex)
    {
        Context->Slots[SlotIndex] = VirtioSndAllocateContiguous(NULL, sizeof(VIRTIO_SND_TX_SLOT));
        if (Context->Slots[SlotIndex] == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto ContextFailure;
        }

        Context->Slots[SlotIndex]->Request.StreamId = StreamId;
    }

    RtlZeroMemory(&Request, sizeof(Request));
    Request.Header.Header.Code = VIRTIO_SND_R_PCM_SET_PARAMS;
    Request.Header.StreamId = StreamId;
    Request.BufferBytes = VIRTIO_SND_PCM_BUFFER_BYTES;
    Request.PeriodBytes = VIRTIO_SND_PCM_PERIOD_BYTES;
    Request.Features = StreamFeatures;
    Request.Channels = (UCHAR)Channels;
    Request.Format = (UCHAR)Format;
    Request.Rate = (UCHAR)RateIndex;

    Status = VirtioSndSubmitControlRequest(Adapter, &Request, sizeof(Request));
    if (!NT_SUCCESS(Status))
        goto ContextFailure;

    Status = VirtioSndSendPcmCommand(Adapter, VIRTIO_SND_R_PCM_PREPARE, StreamId);
    if (!NT_SUCCESS(Status))
    {
        if (Status != STATUS_IO_TIMEOUT)
        {
            NTSTATUS ReleaseStatus = VirtioSndSendPcmCommand(Adapter, VIRTIO_SND_R_PCM_RELEASE, StreamId);
            if (!NT_SUCCESS(ReleaseStatus))
                DbgPrint("virtiosnd: PCM Release after prepare failure returned %08lx\n", (ULONG)ReleaseStatus);
        }
        goto ContextFailure;
    }

    Context->Prepared = TRUE;
    virtqueue_disable_cb(Adapter->Queues[VIRTIO_SND_VQ_TX]);
    Context->DeviceActive = TRUE;
    KeAcquireSpinLock(&Adapter->PlaybackLock, &OldIrql);
    Adapter->PlaybackContext = Context;
    Adapter->PlaybackEvent = WakeEvent;
    KeReleaseSpinLock(&Adapter->PlaybackLock, OldIrql);
    KeClearEvent(&VirtioSndPlaybackQuiesced);
    *Playback = Context;
    DbgPrint(
        "virtiosnd: Prepared %lu Hz S%lu playback stream %lu (%lu channels)\n", SampleRate, BitsPerSample, StreamId,
        Channels);
    KeSetEvent(WakeEvent, IO_NO_INCREMENT, FALSE);
    VirtioSndReleaseLifecycleLock();
    return STATUS_SUCCESS;

ContextFailure:
    for (SlotIndex = 0; SlotIndex < VIRTIO_SND_TX_SLOT_COUNT; ++SlotIndex)
        VirtioSndFreeContiguous(NULL, Context->Slots[SlotIndex]);
    VirtioSndFreeNonpaged(NULL, Context);
Failure:
    InterlockedExchange(&Adapter->PlaybackOpen, 0);
    VirtioSndReleaseLifecycleLock();
    return Status;
}

NTSTATUS
VirtioSndStartPlayback(_Inout_ PVIRTIO_SND_PLAYBACK Playback)
{
    NTSTATUS Status;

    PAGED_CODE();

    if (Playback == NULL || !Playback->Prepared)
        return STATUS_INVALID_DEVICE_STATE;
    if (!Playback->DeviceActive || VirtioSndTransportStopping() || !VirtioSndAdapter.Started)
        return STATUS_DEVICE_NOT_READY;
    if (Playback->Started)
        return STATUS_SUCCESS;

    Status = VirtioSndSendPcmCommand(&VirtioSndAdapter, VIRTIO_SND_R_PCM_START, Playback->StreamId);
    if (NT_SUCCESS(Status))
    {
        Playback->Started = TRUE;
        if (!virtqueue_enable_cb(VirtioSndAdapter.Queues[VIRTIO_SND_VQ_TX]))
        {
            KeSetEvent(Playback->WakeEvent, IO_NO_INCREMENT, FALSE);
        }
        DbgPrint("virtiosnd: Playback started on stream %lu\n", Playback->StreamId);
    }
    return Status;
}

static NTSTATUS
VirtioSndGetCompletedTxSlot(
    _Inout_ PVIRTIO_SND_PLAYBACK Playback,
    _Out_ PVIRTIO_SND_TX_SLOT *CompletedSlot,
    _Out_ unsigned int *UsedLength)
{
    struct virtqueue *TxQueue = VirtioSndAdapter.Queues[VIRTIO_SND_VQ_TX];
    PVIRTIO_SND_TX_SLOT Slot;
    ULONG SlotIndex;

    *CompletedSlot = NULL;
    Slot = (PVIRTIO_SND_TX_SLOT)virtqueue_get_buf(TxQueue, UsedLength);
    if (Slot == NULL)
        return STATUS_NO_MORE_ENTRIES;

    for (SlotIndex = 0; SlotIndex < VIRTIO_SND_TX_SLOT_COUNT; ++SlotIndex)
    {
        if (Playback->Slots[SlotIndex] == Slot)
        {
            if (!Slot->InFlight)
                return STATUS_DEVICE_CONFIGURATION_ERROR;

            Slot->InFlight = FALSE;
            --Playback->InFlightCount;
            *CompletedSlot = Slot;
            return STATUS_SUCCESS;
        }
    }

    DbgPrint("virtiosnd: Unexpected PCM TX completion token %p\n", Slot);
    return STATUS_DEVICE_CONFIGURATION_ERROR;
}

NTSTATUS
VirtioSndGetPlaybackCompletion(
    _Inout_ PVIRTIO_SND_PLAYBACK Playback,
    _Out_ PULONG DeviceStatus,
    _Out_ PULONG LatencyBytes)
{
    PVIRTIO_SND_TX_SLOT Slot;
    unsigned int UsedLength = 0;
    NTSTATUS Status;

    if (Playback == NULL || DeviceStatus == NULL || LatencyBytes == NULL)
        return STATUS_INVALID_PARAMETER;

    Status = VirtioSndGetCompletedTxSlot(Playback, &Slot, &UsedLength);
    if (!NT_SUCCESS(Status))
        return Status;
    if (UsedLength < sizeof(Slot->Response))
        return STATUS_DEVICE_CONFIGURATION_ERROR;

    *DeviceStatus = Slot->Response.Status;
    *LatencyBytes = Slot->Response.LatencyBytes;
    return STATUS_SUCCESS;
}

NTSTATUS
VirtioSndProcessPlaybackEvents(_Inout_ PVIRTIO_SND_PLAYBACK Playback, _Out_ PULONG XrunCount)
{
    NTSTATUS Status;

    if (Playback == NULL || XrunCount == NULL)
        return STATUS_INVALID_PARAMETER;

    Status = VirtioSndDrainEventQueue(&VirtioSndAdapter, Playback, XrunCount);
    if (!NT_SUCCESS(Status))
        return Status;

#if defined(VIRTIO_SND_TEST_XRUN_INJECTION)
    if (Playback->InFlightCount != 0 && InterlockedCompareExchange(&Playback->TestXrunInjected, TRUE, FALSE) == FALSE)
    {
        VIRTIO_SND_EVENT TestEvent;

        TestEvent.Header.Code = VIRTIO_SND_EVT_PCM_XRUN;
        TestEvent.Data = Playback->StreamId;
        DbgPrint("virtiosnd: Injecting a synthetic PCM XRUN for stream %lu\n", Playback->StreamId);
        VirtioSndHandleEvent(Playback, &TestEvent, XrunCount, FALSE);
    }
#endif

    return STATUS_SUCCESS;
}

BOOLEAN
VirtioSndArmPlaybackCompletion(_Inout_ PVIRTIO_SND_PLAYBACK Playback)
{
    struct virtqueue *TxQueue = VirtioSndAdapter.Queues[VIRTIO_SND_VQ_TX];

    if (Playback == NULL || !Playback->DeviceActive || !Playback->Started || TxQueue == NULL ||
        VirtioSndTransportStopping())
    {
        return FALSE;
    }

    return virtqueue_enable_cb(TxQueue);
}

BOOLEAN
VirtioSndPlaybackIsActive(_In_ PVIRTIO_SND_PLAYBACK Playback)
{
    return Playback != NULL && Playback->DeviceActive && VirtioSndAdapter.PlaybackContext == Playback &&
           VirtioSndAdapter.Started && !VirtioSndTransportStopping();
}

NTSTATUS
VirtioSndQueuePlayback(_Inout_ PVIRTIO_SND_PLAYBACK Playback, _In_reads_bytes_(Length) PVOID Audio, _In_ ULONG Length)
{
    PVIRTIO_SND_ADAPTER Adapter = &VirtioSndAdapter;
    struct virtqueue *TxQueue = Adapter->Queues[VIRTIO_SND_VQ_TX];
    struct scatterlist ScatterList[3];
    PVIRTIO_SND_TX_SLOT Slot = NULL;
    PHYSICAL_ADDRESS PhysicalAddress;
    ULONG SlotIndex;
    INT AddResult;

    if (Playback == NULL || Audio == NULL || Length != VIRTIO_SND_PCM_PERIOD_BYTES || !Playback->DeviceActive ||
        VirtioSndTransportStopping() || !Playback->Started || TxQueue == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    for (SlotIndex = 0; SlotIndex < VIRTIO_SND_TX_SLOT_COUNT; ++SlotIndex)
    {
        if (!Playback->Slots[SlotIndex]->InFlight)
        {
            Slot = Playback->Slots[SlotIndex];
            break;
        }
    }
    if (Slot == NULL)
        return STATUS_DEVICE_BUSY;

    RtlCopyMemory(Slot->Audio, Audio, Length);
    RtlZeroMemory(&Slot->Response, sizeof(Slot->Response));
    Slot->Response.Status = MAXULONG;

    PhysicalAddress = MmGetPhysicalAddress(Slot);
    ScatterList[0].physAddr = PhysicalAddress;
    ScatterList[0].length = sizeof(Slot->Request);
    ScatterList[1].physAddr = PhysicalAddress;
    ScatterList[1].physAddr.QuadPart += FIELD_OFFSET(VIRTIO_SND_TX_SLOT, Audio);
    ScatterList[1].length = Length;
    ScatterList[2].physAddr = PhysicalAddress;
    ScatterList[2].physAddr.QuadPart += FIELD_OFFSET(VIRTIO_SND_TX_SLOT, Response);
    ScatterList[2].length = sizeof(Slot->Response);

    AddResult = virtqueue_add_buf(TxQueue, ScatterList, 2, 1, Slot, NULL, 0);
    if (AddResult < 0)
        return STATUS_DEVICE_BUSY;

    Slot->InFlight = TRUE;
    ++Playback->InFlightCount;
    if (virtqueue_kick_prepare(TxQueue))
        virtqueue_notify(TxQueue);
    return STATUS_SUCCESS;
}

NTSTATUS
VirtioSndStopPlayback(_Inout_ PVIRTIO_SND_PLAYBACK Playback)
{
    ULONG Retry;
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    if (Playback == NULL)
        return STATUS_INVALID_PARAMETER;

    if (!Playback->DeviceActive)
        return STATUS_DEVICE_NOT_READY;
    virtqueue_disable_cb(VirtioSndAdapter.Queues[VIRTIO_SND_VQ_TX]);

    for (Retry = 0; Playback->InFlightCount != 0 && Retry < VIRTIO_SND_CONTROL_TIMEOUT_MS; ++Retry)
    {
        PVIRTIO_SND_TX_SLOT Slot;
        unsigned int UsedLength = 0;
        Status = VirtioSndGetCompletedTxSlot(Playback, &Slot, &UsedLength);
        if (Status == STATUS_NO_MORE_ENTRIES)
            VirtioSndSleep(NULL, 1);
        else if (!NT_SUCCESS(Status))
            return Status;
    }

    if (Playback->InFlightCount != 0)
    {
        DbgPrint("virtiosnd: Timed out draining PCM TX queue\n");
        return STATUS_IO_TIMEOUT;
    }

    if (Playback->Started)
    {
        Status = VirtioSndSendPcmCommand(&VirtioSndAdapter, VIRTIO_SND_R_PCM_STOP, Playback->StreamId);
        if (!NT_SUCCESS(Status))
            return Status;
        Playback->Started = FALSE;
    }

    return STATUS_SUCCESS;
}

VOID
VirtioSndClosePlayback(_Inout_ PVIRTIO_SND_PLAYBACK Playback)
{
    PVIRTIO_SND_ADAPTER Adapter = &VirtioSndAdapter;
    ULONG SlotIndex;
    NTSTATUS Status;
    KIRQL OldIrql;
    BOOLEAN OwnsAdapterContext;

    PAGED_CODE();

    if (Playback == NULL)
        return;

    VirtioSndInitializeSynchronization();

Retry:
    VirtioSndAcquireLifecycleLock();
    if (Playback->DeviceActive && VirtioSndTransportStopping())
    {
        VirtioSndReleaseLifecycleLock();
        KeWaitForSingleObject(&VirtioSndTransportStopped, Executive, KernelMode, FALSE, NULL);
        goto Retry;
    }

    if (Playback->DeviceActive && Adapter->QueuesInitialized)
    {
        Status = VirtioSndStopPlayback(Playback);
        if (!NT_SUCCESS(Status))
        {
            DbgPrint(
                "virtiosnd: Playback stop failed (%08lx). Resetting device before freeing DMA "
                "buffers\n",
                (ULONG)Status);
            virtio_device_reset(&Adapter->Device);
            if (Adapter->QueuesInitialized)
            {
                virtio_delete_queues(&Adapter->Device);
                Adapter->QueuesInitialized = FALSE;
            }
            Adapter->Started = FALSE;
        }

        if (Playback->Prepared && Adapter->QueuesInitialized)
        {
            Status = VirtioSndSendPcmCommand(Adapter, VIRTIO_SND_R_PCM_RELEASE, Playback->StreamId);
            if (!NT_SUCCESS(Status))
                DbgPrint("virtiosnd: PCM Release failed with status %08lx\n", (ULONG)Status);
        }
    }

    Playback->DeviceActive = FALSE;
    OwnsAdapterContext = Adapter->PlaybackContext == Playback;
    if (OwnsAdapterContext)
    {
        KeAcquireSpinLock(&Adapter->PlaybackLock, &OldIrql);
        Adapter->PlaybackContext = NULL;
        Adapter->PlaybackEvent = NULL;
        KeReleaseSpinLock(&Adapter->PlaybackLock, OldIrql);
    }
    for (SlotIndex = 0; SlotIndex < VIRTIO_SND_TX_SLOT_COUNT; ++SlotIndex)
        VirtioSndFreeContiguous(NULL, Playback->Slots[SlotIndex]);
    VirtioSndFreeNonpaged(NULL, Playback);
    if (OwnsAdapterContext)
        InterlockedExchange(&Adapter->PlaybackOpen, 0);
    VirtioSndReleaseLifecycleLock();
}

static NTSTATUS
VirtioSndProbePcmParameters(_Inout_ PVIRTIO_SND_ADAPTER Adapter)
{
    ULONG StreamId;
    ULONG PlaybackStream = MAXULONG;
    UCHAR Channels = 0;
    VIRTIO_SND_PCM_SET_PARAMS Request;
    NTSTATUS Status;

    for (StreamId = 0; StreamId < Adapter->PcmInfoCount; ++StreamId)
    {
        PVIRTIO_SND_PCM_INFO Info = &Adapter->PcmInfo[StreamId];
        ULONGLONG FormatBit = (ULONGLONG)1 << VIRTIO_SND_PCM_FMT_S16;
        ULONGLONG RateBit = (ULONGLONG)1 << VIRTIO_SND_PCM_RATE_48000;

        if (Info->Direction != 0 || (Info->Formats & FormatBit) == 0 || (Info->Rates & RateBit) == 0 ||
            Info->ChannelsMin == 0 || Info->ChannelsMin > 2 || Info->ChannelsMax < Info->ChannelsMin)
        {
            continue;
        }

        PlaybackStream = StreamId;
        Channels = Info->ChannelsMax >= 2 ? 2 : 1;
        break;
    }

    if (PlaybackStream == MAXULONG)
    {
        DbgPrint("virtiosnd: No playback stream supports signed 16-bit 48 kHz PCM\n");
        return STATUS_NOT_SUPPORTED;
    }

    RtlZeroMemory(&Request, sizeof(Request));
    Request.Header.Header.Code = VIRTIO_SND_R_PCM_SET_PARAMS;
    Request.Header.StreamId = PlaybackStream;
    Request.BufferBytes = VIRTIO_SND_PCM_BUFFER_BYTES;
    Request.PeriodBytes = VIRTIO_SND_PCM_PERIOD_BYTES;
    Request.Channels = Channels;
    Request.Format = VIRTIO_SND_PCM_FMT_S16;
    Request.Rate = VIRTIO_SND_PCM_RATE_48000;

    DbgPrint(
        "virtiosnd: Probing 48 kHz S16 playback on stream %lu, %u channels, buffer %lu period "
        "%lu\n",
        PlaybackStream, Channels, Request.BufferBytes, Request.PeriodBytes);

    Status = VirtioSndSubmitControlRequest(Adapter, &Request, sizeof(Request));
    if (!NT_SUCCESS(Status))
        return Status;

    Status = VirtioSndSendPcmCommand(Adapter, VIRTIO_SND_R_PCM_PREPARE, PlaybackStream);
    if (!NT_SUCCESS(Status))
    {
        if (Status != STATUS_IO_TIMEOUT)
        {
            NTSTATUS ReleaseStatus = VirtioSndSendPcmCommand(Adapter, VIRTIO_SND_R_PCM_RELEASE, PlaybackStream);
            if (!NT_SUCCESS(ReleaseStatus))
            {
                DbgPrint("virtiosnd: PCM Release after prepare failure returned %08lx\n", (ULONG)ReleaseStatus);
                return STATUS_DEVICE_CONFIGURATION_ERROR;
            }
        }

        return Status;
    }

    Status = VirtioSndSendPcmCommand(Adapter, VIRTIO_SND_R_PCM_RELEASE, PlaybackStream);
    if (!NT_SUCCESS(Status))
    {
        DbgPrint("virtiosnd: PCM Release after probe failed with status %08lx\n", (ULONG)Status);
        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
VirtioSndStartTransport(_In_ PDEVICE_OBJECT DeviceObject, _In_ PRESOURCELIST ResourceList)
{
    PVIRTIO_SND_ADAPTER Adapter = &VirtioSndAdapter;
    ULONGLONG DriverFeatures = 0;
    ULONG QueueIndex;
    NTSTATUS Status;

    PAGED_CODE();

    VirtioSndInitializeSynchronization();
    VirtioSndAcquireLifecycleLock();

    if (Adapter->Started || Adapter->PlaybackContext != NULL)
    {
        VirtioSndReleaseLifecycleLock();
        return STATUS_DEVICE_BUSY;
    }

    if (Adapter->OwnerDeviceObject != NULL && Adapter->OwnerDeviceObject != DeviceObject)
    {
        DbgPrint("virtiosnd: This prototype already owns another device\n");
        VirtioSndReleaseLifecycleLock();
        return STATUS_DEVICE_BUSY;
    }

    VirtioSndShutdownAdapter(Adapter);
    Adapter->OwnerDeviceObject = DeviceObject;
    KeInitializeSpinLock(&Adapter->PlaybackLock);
    KeInitializeDpc(&Adapter->InterruptDpc, VirtioSndInterruptDpc, Adapter);
    InterlockedExchange(&Adapter->Stopping, FALSE);
    KeClearEvent(&VirtioSndTransportStopped);

    Status = VirtioSndInitializePciResources(DeviceObject, ResourceList, &Adapter->Transport);
    if (!NT_SUCCESS(Status))
    {
        DbgPrint("virtiosnd: PCI Resource initialization failed with status %08lx\n", (ULONG)Status);
        goto Failure;
    }

    Adapter->DeviceInitializeAttempted = TRUE;
    Status = virtio_device_initialize(&Adapter->Device, &VirtioSndSystemOps, &Adapter->Transport, FALSE);
    if (!NT_SUCCESS(Status))
    {
        DbgPrint("virtiosnd: VirtIO Transport initialization failed with status %08lx\n", (ULONG)Status);
        goto Failure;
    }

    Adapter->DeviceInitialized = TRUE;
    Adapter->HostFeatures = virtio_get_features(&Adapter->Device);
    if (virtio_is_feature_enabled(Adapter->HostFeatures, VIRTIO_F_VERSION_1))
        virtio_feature_enable(DriverFeatures, VIRTIO_F_VERSION_1);
    Adapter->DriverFeatures = DriverFeatures;

    Status = virtio_set_features(&Adapter->Device, Adapter->DriverFeatures);
    if (!NT_SUCCESS(Status))
    {
        DbgPrint(
            "virtiosnd: feature negotiation failed (host %08lx%08lx driver %08lx%08lx status "
            "%08lx)\n",
            (ULONG)(Adapter->HostFeatures >> 32), (ULONG)Adapter->HostFeatures, (ULONG)(Adapter->DriverFeatures >> 32),
            (ULONG)Adapter->DriverFeatures, (ULONG)Status);
        goto Failure;
    }

    if (Adapter->Device.config == NULL || Adapter->Device.config_len < sizeof(Adapter->Config))
    {
        DbgPrint(
            "virtiosnd: Invalid device config (address %p length %lu expected %lu)\n", Adapter->Device.config,
            (ULONG)Adapter->Device.config_len, (ULONG)sizeof(Adapter->Config));
        Status = STATUS_DEVICE_CONFIGURATION_ERROR;
        goto Failure;
    }

    RtlZeroMemory(&Adapter->Config, sizeof(Adapter->Config));
    virtio_get_config(&Adapter->Device, 0, &Adapter->Config, sizeof(Adapter->Config));

    DbgPrint(
        "virtiosnd: Transport ready. Features %08lx%08lx, jacks %lu, streams %lu, channel "
        "maps %lu, controls %lu\n",
        (ULONG)(Adapter->HostFeatures >> 32), (ULONG)Adapter->HostFeatures, Adapter->Config.Jacks,
        Adapter->Config.Streams, Adapter->Config.ChannelMaps, Adapter->Config.Controls);

    Status = virtio_find_queues(&Adapter->Device, VIRTIO_SND_VQ_MAX, Adapter->Queues);
    if (!NT_SUCCESS(Status))
    {
        DbgPrint("virtiosnd: Virtqueue setup failed with status %08lx\n", (ULONG)Status);
        goto Failure;
    }
    Adapter->QueuesInitialized = TRUE;

    for (QueueIndex = 0; QueueIndex < VIRTIO_SND_VQ_MAX; ++QueueIndex)
    {
        virtqueue_disable_cb(Adapter->Queues[QueueIndex]);
        DbgPrint(
            "virtiosnd: Queue %lu ready with %lu descriptors\n", QueueIndex,
            virtio_get_queue_size(Adapter->Queues[QueueIndex]));
    }

    Status = VirtioSndInitializeEventQueue(Adapter);
    if (!NT_SUCCESS(Status))
    {
        DbgPrint("virtiosnd: Event queue setup failed with status %08lx\n", (ULONG)Status);
        goto Failure;
    }

    Status = VirtioSndConnectInterrupt(Adapter, ResourceList);
    if (!NT_SUCCESS(Status))
    {
        DbgPrint("virtiosnd: Interrupt connection failed with status %08lx\n", (ULONG)Status);
        goto Failure;
    }

    virtio_device_ready(&Adapter->Device);
    DbgPrint("virtiosnd: Device marked ready. Starting PCM info query\n");
    if (virtqueue_kick_prepare(Adapter->Queues[VIRTIO_SND_VQ_EVENT]))
        virtqueue_notify(Adapter->Queues[VIRTIO_SND_VQ_EVENT]);
    {
        ULONG InitialXrunCount;
        Status = VirtioSndDrainEventQueue(Adapter, NULL, &InitialXrunCount);
        if (!NT_SUCCESS(Status))
        {
            DbgPrint("virtiosnd: Event queue initialization failed with status %08lx\n", (ULONG)Status);
            goto Failure;
        }
    }

    Status = VirtioSndQueryPcmInfo(Adapter);
    if (!NT_SUCCESS(Status))
    {
        DbgPrint("virtiosnd: PCM Info query failed with status %08lx\n", (ULONG)Status);
        goto Failure;
    }

    Status = VirtioSndQueryChannelMaps(Adapter);
    if (!NT_SUCCESS(Status))
    {
        DbgPrint("virtiosnd: Channel-map query failed with status %08lx\n", (ULONG)Status);
        goto Failure;
    }

    Status = VirtioSndProbePcmParameters(Adapter);
    if (Status == STATUS_NOT_SUPPORTED)
    {
        DbgPrint("virtiosnd: Optional 48 kHz PCM setup probe is not supported\n");
    }
    else if (!NT_SUCCESS(Status))
    {
        DbgPrint("virtiosnd: 48 kHz PCM Setup probe failed with status %08lx\n", (ULONG)Status);
        goto Failure;
    }

    Adapter->Started = TRUE;
    DbgPrint("virtiosnd: adapter and queues retained after StartDevice\n");
    VirtioSndReleaseLifecycleLock();
    return STATUS_SUCCESS;

Failure:
    VirtioSndShutdownAdapter(Adapter);
    KeSetEvent(&VirtioSndTransportStopped, IO_NO_INCREMENT, FALSE);
    VirtioSndReleaseLifecycleLock();
    return Status;
}
