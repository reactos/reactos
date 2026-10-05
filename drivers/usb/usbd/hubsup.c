/*
 * PROJECT:     ReactOS USB Driver Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Hub number allocator and serial number device list for usbhub3
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#define _USBD_
#include <ntddk.h>
#include <usbdi.h>
#include <usbioctl.h>
#include <drivers/usb3/usbdhub.h>

#define USBD_POOL_TAG 'DBSU'

/* POOL_TYPE value for no-execute nonpaged pool on NT 6.2 and later */
#define USBD_NONPAGED_POOL_NX ((POOL_TYPE)0x200)

/* One record per attached device that reported a serial number */
typedef struct _USBD_SERIAL_RECORD
{
    LIST_ENTRY Link;
    PVOID DeviceContext;
    PVOID HubContext;
    ULONG PortNumber;
    PVOID ConnectorNode;
    USHORT IdVendor;
    USHORT IdProduct;
    BOOLEAN Disconnected;
    ULONG SerialLength;
    PWCHAR Serial;
} USBD_SERIAL_RECORD, *PUSBD_SERIAL_RECORD;

static POOL_TYPE UsbdNonPagedPoolType = NonPagedPool;

static PRTL_BITMAP UsbdHubNumbers;
static KEVENT UsbdHubNumberLock;

static LIST_ENTRY UsbdSerialList = { &UsbdSerialList, &UsbdSerialList };
static KSPIN_LOCK UsbdSerialListLock;

NTSTATUS
NTAPI
DllInitialize(
    _In_ PUNICODE_STRING RegistryPath)
{
    RTL_OSVERSIONINFOW OsVersion;
    PRTL_BITMAP Bitmap;
    SIZE_T BitmapSize;

    UNREFERENCED_PARAMETER(RegistryPath);

    RtlZeroMemory(&OsVersion, sizeof(OsVersion));
    OsVersion.dwOSVersionInfoSize = sizeof(OsVersion);
    if (NT_SUCCESS(RtlGetVersion(&OsVersion)))
    {
        if (OsVersion.dwMajorVersion > 6 ||
            (OsVersion.dwMajorVersion == 6 && OsVersion.dwMinorVersion >= 2))
        {
            UsbdNonPagedPoolType = USBD_NONPAGED_POOL_NX;
        }
    }

    KeInitializeSpinLock(&UsbdSerialListLock);
    InitializeListHead(&UsbdSerialList);
    KeInitializeEvent(&UsbdHubNumberLock, SynchronizationEvent, TRUE);

    /* Without the bitmap every hub simply gets number 0 */
    BitmapSize = sizeof(*Bitmap) + USBD_HUB_NUMBER_COUNT / 8;
    Bitmap = ExAllocatePoolWithTag(PagedPool, BitmapSize, USBD_POOL_TAG);
    if (Bitmap)
    {
        RtlZeroMemory(Bitmap, BitmapSize);
        RtlInitializeBitMap(Bitmap, (PULONG)(Bitmap + 1), USBD_HUB_NUMBER_COUNT);
        RtlSetBits(Bitmap, USBD_HUB_NUMBER_NONE, 1);
        UsbdHubNumbers = Bitmap;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
DllUnload(VOID)
{
    if (UsbdHubNumbers)
    {
        ExFreePoolWithTag(UsbdHubNumbers, USBD_POOL_TAG);
        UsbdHubNumbers = NULL;
    }

    return STATUS_SUCCESS;
}

ULONG
NTAPI
USBD_AllocateHubNumber(VOID)
{
    ULONG Number;

    PAGED_CODE();

    if (!UsbdHubNumbers)
        return USBD_HUB_NUMBER_NONE;

    KeWaitForSingleObject(&UsbdHubNumberLock, Executive, KernelMode, FALSE, NULL);
    Number = RtlFindClearBitsAndSet(UsbdHubNumbers, 1, 0);
    KeSetEvent(&UsbdHubNumberLock, IO_NO_INCREMENT, FALSE);

    if (Number == MAXULONG)
        return USBD_HUB_NUMBER_NONE;

    return Number;
}

VOID
NTAPI
USBD_ReleaseHubNumber(
    _In_ ULONG HubNumber)
{
    PAGED_CODE();

    /* 0 is reserved and anything past the map would land outside the buffer */
    if (!UsbdHubNumbers ||
        HubNumber == USBD_HUB_NUMBER_NONE ||
        HubNumber >= USBD_HUB_NUMBER_COUNT)
    {
        return;
    }

    KeWaitForSingleObject(&UsbdHubNumberLock, Executive, KernelMode, FALSE, NULL);
    RtlClearBits(UsbdHubNumbers, HubNumber, 1);
    KeSetEvent(&UsbdHubNumberLock, IO_NO_INCREMENT, FALSE);
}

static
BOOLEAN
UsbdSerialRecordMatches(
    _In_ PUSBD_SERIAL_RECORD Record,
    _In_ USHORT IdVendor,
    _In_ USHORT IdProduct,
    _In_ PUSB_ID_STRING SerialNumber)
{
    if (Record->IdVendor != IdVendor || Record->IdProduct != IdProduct)
        return FALSE;

    if (Record->SerialLength != SerialNumber->LengthInBytes)
        return FALSE;

    if (Record->SerialLength == 0)
        return TRUE;

    return RtlCompareMemory(Record->Serial,
                            SerialNumber->Buffer,
                            Record->SerialLength) == Record->SerialLength;
}

/* Decides whether a device colliding with an existing record may still be added */
static
ULONG
UsbdClassifySerialCollision(
    _In_ PUSBD_SERIAL_RECORD Existing,
    _In_opt_ PVOID HubContext,
    _In_ ULONG PortNumber,
    _In_opt_ PVOID ConnectorNode)
{
    BOOLEAN SameConnector;
    BOOLEAN SamePort;

    SameConnector = (ConnectorNode != NULL && Existing->ConnectorNode == ConnectorNode);
    SamePort = (HubContext != NULL &&
                Existing->HubContext == HubContext &&
                Existing->PortNumber == PortNumber);

    /* A connected record on the same connector is the other speed's copy that has not left yet */
    if (!Existing->Disconnected)
        return SameConnector ? USBD_GLOBAL_LIST_DUPLICATE_PENDING : USBD_GLOBAL_LIST_DUPLICATE;

    if (SameConnector || SamePort)
        return USBD_GLOBAL_LIST_ADDED;

    return USBD_GLOBAL_LIST_DUPLICATE_PENDING;
}

static
PUSBD_SERIAL_RECORD
UsbdFindSerialRecordByDevice(
    _In_ PVOID DeviceContext)
{
    PLIST_ENTRY Entry;
    PUSBD_SERIAL_RECORD Record;

    for (Entry = UsbdSerialList.Flink; Entry != &UsbdSerialList; Entry = Entry->Flink)
    {
        Record = CONTAINING_RECORD(Entry, USBD_SERIAL_RECORD, Link);
        if (Record->DeviceContext == DeviceContext)
            return Record;
    }

    return NULL;
}

ULONG
NTAPI
USBD_AddDeviceToGlobalList(
    _In_ PVOID DeviceContext,
    _In_opt_ PVOID HubContext,
    _In_ ULONG PortNumber,
    _In_opt_ PVOID ConnectorNode,
    _In_ USHORT IdVendor,
    _In_ USHORT IdProduct,
    _In_ PUSB_ID_STRING SerialNumber)
{
    PLIST_ENTRY Entry;
    PUSBD_SERIAL_RECORD Record;
    PUSBD_SERIAL_RECORD NewRecord;
    ULONG Result = USBD_GLOBAL_LIST_ADDED;
    KIRQL OldIrql;

    KeAcquireSpinLock(&UsbdSerialListLock, &OldIrql);

    /* Only the first record with the same identity decides */
    for (Entry = UsbdSerialList.Flink; Entry != &UsbdSerialList; Entry = Entry->Flink)
    {
        Record = CONTAINING_RECORD(Entry, USBD_SERIAL_RECORD, Link);
        if (UsbdSerialRecordMatches(Record, IdVendor, IdProduct, SerialNumber))
        {
            Result = UsbdClassifySerialCollision(Record, HubContext, PortNumber, ConnectorNode);
            break;
        }
    }

    if (Result != USBD_GLOBAL_LIST_ADDED)
        goto Exit;

    NewRecord = ExAllocatePoolWithTag(UsbdNonPagedPoolType, sizeof(*NewRecord), USBD_POOL_TAG);
    if (!NewRecord)
    {
        Result = USBD_GLOBAL_LIST_NO_MEMORY;
        goto Exit;
    }

    RtlZeroMemory(NewRecord, sizeof(*NewRecord));

    if (SerialNumber->LengthInBytes != 0)
    {
        NewRecord->Serial = ExAllocatePoolWithTag(UsbdNonPagedPoolType,
                                                  SerialNumber->LengthInBytes,
                                                  USBD_POOL_TAG);
        if (!NewRecord->Serial)
        {
            ExFreePoolWithTag(NewRecord, USBD_POOL_TAG);
            Result = USBD_GLOBAL_LIST_NO_MEMORY;
            goto Exit;
        }

        RtlCopyMemory(NewRecord->Serial, SerialNumber->Buffer, SerialNumber->LengthInBytes);
    }

    NewRecord->SerialLength = SerialNumber->LengthInBytes;
    NewRecord->DeviceContext = DeviceContext;
    NewRecord->HubContext = HubContext;
    NewRecord->PortNumber = PortNumber;
    NewRecord->ConnectorNode = ConnectorNode;
    NewRecord->IdVendor = IdVendor;
    NewRecord->IdProduct = IdProduct;
    InsertTailList(&UsbdSerialList, &NewRecord->Link);

Exit:
    KeReleaseSpinLock(&UsbdSerialListLock, OldIrql);
    return Result;
}

VOID
NTAPI
USBD_MarkDeviceAsDisconnected(
    _In_ PVOID DeviceContext)
{
    PUSBD_SERIAL_RECORD Record;
    KIRQL OldIrql;

    KeAcquireSpinLock(&UsbdSerialListLock, &OldIrql);

    Record = UsbdFindSerialRecordByDevice(DeviceContext);
    if (Record)
        Record->Disconnected = TRUE;

    KeReleaseSpinLock(&UsbdSerialListLock, OldIrql);
}

VOID
NTAPI
USBD_RemoveDeviceFromGlobalList(
    _In_ PVOID DeviceContext)
{
    PUSBD_SERIAL_RECORD Record;
    KIRQL OldIrql;

    KeAcquireSpinLock(&UsbdSerialListLock, &OldIrql);

    Record = UsbdFindSerialRecordByDevice(DeviceContext);
    if (Record)
        RemoveEntryList(&Record->Link);

    KeReleaseSpinLock(&UsbdSerialListLock, OldIrql);

    if (!Record)
        return;

    if (Record->Serial)
        ExFreePoolWithTag(Record->Serial, USBD_POOL_TAG);

    ExFreePoolWithTag(Record, USBD_POOL_TAG);
}
