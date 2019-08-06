/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Unit Tests for acpi!Bus_PDO_QueryResourceRequirements
 * COPYRIGHT:   Copyright 2017-2020 Thomas Faber (thomas.faber@reactos.org)
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#include <ndk/rtlfuncs.h>
#define UNIT_TEST
#include <acpi.h>

/* Kernel definitions (copied) */
#define IO_RESOURCE_PREFERRED             0x01
#define IO_RESOURCE_DEFAULT               0x02
#define IO_RESOURCE_ALTERNATIVE           0x08

typedef struct _IO_RESOURCE_DESCRIPTOR {
  UCHAR Option;
  UCHAR Type;
  UCHAR ShareDisposition;
  UCHAR Spare1;
  USHORT Flags;
  USHORT Spare2;
  union {
    struct {
      ULONG Length;
      ULONG Alignment;
      PHYSICAL_ADDRESS MinimumAddress;
      PHYSICAL_ADDRESS MaximumAddress;
    } Port;
    struct {
      ULONG Length;
      ULONG Alignment;
      PHYSICAL_ADDRESS MinimumAddress;
      PHYSICAL_ADDRESS MaximumAddress;
    } Memory;
    struct {
      ULONG MinimumVector;
      ULONG MaximumVector;
    } Interrupt;
    struct {
      ULONG MinimumChannel;
      ULONG MaximumChannel;
    } Dma;
    struct {
      ULONG Length;
      ULONG Alignment;
      PHYSICAL_ADDRESS MinimumAddress;
      PHYSICAL_ADDRESS MaximumAddress;
    } Generic;
    struct {
      ULONG Data[3];
    } DevicePrivate;
    struct {
      ULONG Length;
      ULONG MinBusNumber;
      ULONG MaxBusNumber;
      ULONG Reserved;
    } BusNumber;
    struct {
      ULONG Priority;
      ULONG Reserved1;
      ULONG Reserved2;
    } ConfigData;
  } u;
} IO_RESOURCE_DESCRIPTOR, *PIO_RESOURCE_DESCRIPTOR;

typedef struct _IO_RESOURCE_LIST {
  USHORT Version;
  USHORT Revision;
  ULONG Count;
  IO_RESOURCE_DESCRIPTOR Descriptors[1];
} IO_RESOURCE_LIST, *PIO_RESOURCE_LIST;

typedef struct _IO_RESOURCE_REQUIREMENTS_LIST {
  ULONG ListSize;
  INTERFACE_TYPE InterfaceType;
  ULONG BusNumber;
  ULONG SlotNumber;
  ULONG Reserved[3];
  ULONG AlternativeLists;
  IO_RESOURCE_LIST List[1];
} IO_RESOURCE_REQUIREMENTS_LIST, *PIO_RESOURCE_REQUIREMENTS_LIST;

/* Kernel definitions (mock) */
#define PAGED_CODE()
#define DPRINT1(...) do { if (0) DbgPrint(__VA_ARGS__); } while (0)

typedef struct _IRP
{
    IO_STATUS_BLOCK IoStatus;
} IRP, *PIRP;

#define PagedPool 1
static
PVOID
ExAllocatePoolWithTag(ULONG PoolType, SIZE_T NumberOfBytes, ULONG Tag)
{
    PVOID *Mem;

    Mem = HeapAlloc(GetProcessHeap(), 0, NumberOfBytes + 2 * sizeof(PVOID));
    Mem[0] = (PVOID)NumberOfBytes;
    Mem[1] = (PVOID)(ULONG_PTR)Tag;
    return Mem + 2;
}

static
VOID
ExFreePoolWithTag(PVOID MemPtr, ULONG Tag)
{
    PVOID *Mem = MemPtr;

    Mem -= 2;
    ok(Mem[1] == (PVOID)(ULONG_PTR)Tag, "Tag is %lx, expected %p\n", Tag, Mem[1]);
    HeapFree(GetProcessHeap(), 0, Mem);
}

static
SIZE_T
GetPoolAllocSize(PVOID MemPtr)
{
    PVOID *Mem = MemPtr;

    Mem -= 2;
    return (SIZE_T)Mem[0];
}

/* ACPI driver definitions */
typedef struct _PDO_DEVICE_DATA
{
    HANDLE AcpiHandle;
    PWCHAR HardwareIDs;
} PDO_DEVICE_DATA, *PPDO_DEVICE_DATA;

/* ACPICA functions (mock) */
static BOOLEAN AcpiCallExpected;
static ACPI_HANDLE CorrectHandle = &CorrectHandle;
static ACPI_BUFFER CurrentBuffer;
static ACPI_BUFFER PossibleBuffer;

ACPI_STATUS
AcpiGetCurrentResources (
    ACPI_HANDLE             Device,
    ACPI_BUFFER             *RetBuffer)
{
    ok(AcpiCallExpected, "Unexpected call to AcpiGetCurrentResources\n");
    ok(Device == CorrectHandle, "Device = %p, expected %p\n", Device, CorrectHandle);
    if (RetBuffer->Length < CurrentBuffer.Length)
    {
        RetBuffer->Length = CurrentBuffer.Length;
        return AE_BUFFER_OVERFLOW;
    }
    RetBuffer->Length = CurrentBuffer.Length;
    CopyMemory(RetBuffer->Pointer, CurrentBuffer.Pointer, CurrentBuffer.Length);
    return AE_OK;
}

ACPI_STATUS
AcpiGetPossibleResources (
    ACPI_HANDLE             Device,
    ACPI_BUFFER             *RetBuffer)
{
    ok(AcpiCallExpected, "Unexpected call to AcpiGetPossibleResources\n");
    ok(Device == CorrectHandle, "Device = %p, expected %p\n", Device, CorrectHandle);
    if (RetBuffer->Length < PossibleBuffer.Length)
    {
        RetBuffer->Length = PossibleBuffer.Length;
        return AE_BUFFER_OVERFLOW;
    }
    RetBuffer->Length = PossibleBuffer.Length;
    CopyMemory(RetBuffer->Pointer, PossibleBuffer.Pointer, PossibleBuffer.Length);
    return AE_OK;
}

#include "../../../../drivers/bus/acpi/buspdo.c"

/* ACPI_RESOURCE builder helpers */
#define MAKE_IRQ(Resource, _DescriptorLength, _Triggering, _Polarity, _Shareable, _WakeCapable) \
    do {                                                                                        \
    Resource->Data.Irq.DescriptorLength = _DescriptorLength;                                    \
    Resource->Data.Irq.Triggering = _Triggering;                                                \
    Resource->Data.Irq.Polarity = _Polarity;                                                    \
    Resource->Data.Irq.Shareable = _Shareable;                                                  \
    Resource->Data.Irq.WakeCapable = _WakeCapable;                                              \
    } while (0)

/* IO_RESOURCE_DESCRIPTOR expectations */
#define expect_irq(Desc, ExpectedOption, ExpectedShare, ExpectedMin, ExpectedMax)                                                       \
    do {                                                                                                                                \
    ok((Desc)->Option == ExpectedOption, "Desc->Option = %u\n", (Desc)->Option);                                                        \
    ok((Desc)->Type == CmResourceTypeInterrupt, "Desc->Type = %u\n", (Desc)->Type);                                                     \
    ok((Desc)->ShareDisposition == ExpectedShare, "Desc->ShareDisposition = %u\n", (Desc)->ShareDisposition);                           \
    ok((Desc)->u.Interrupt.MinimumVector == ExpectedMin, "Desc->u.Interrupt.MinimumVector = %lu\n", (Desc)->u.Interrupt.MinimumVector); \
    ok((Desc)->u.Interrupt.MaximumVector == ExpectedMax, "Desc->u.Interrupt.MaximumVector = %lu\n", (Desc)->u.Interrupt.MaximumVector); \
    } while (0)

#define expect_port(Desc, ExpectedOption, ExpectedShare, ExpectedLength, ExpectedAlign, ExpectedMin, ExpectedMax)                                   \
    do {                                                                                                                                            \
    ok((Desc)->Option == ExpectedOption, "Desc->Option = %u\n", (Desc)->Option);                                                                    \
    ok((Desc)->Type == CmResourceTypePort, "Desc->Type = %u\n", (Desc)->Type);                                                                      \
    ok((Desc)->ShareDisposition == ExpectedShare, "Desc->ShareDisposition = %u\n", (Desc)->ShareDisposition);                                       \
    ok((Desc)->u.Port.Length == ExpectedLength, "Desc->u.Port.Length = %lu\n", (Desc)->u.Port.Length);                                              \
    ok((Desc)->u.Port.Alignment == ExpectedAlign, "Desc->u.Port.Alignment = %lu\n", (Desc)->u.Port.Alignment);                                      \
    ok((Desc)->u.Port.MinimumAddress.QuadPart == ExpectedMin, "Desc->u.Port.MinimumAddress = 0x%I64x\n", (Desc)->u.Port.MinimumAddress.QuadPart);   \
    ok((Desc)->u.Port.MaximumAddress.QuadPart == ExpectedMax, "Desc->u.Port.MaximumAddress = 0x%I64x\n", (Desc)->u.Port.MaximumAddress.QuadPart);   \
    } while (0)

START_TEST(Bus_PDO_QueryResourceRequirements)
{
    NTSTATUS Status;
    PDO_DEVICE_DATA DeviceData;
    IRP Irp;
    ACPI_RESOURCE ResourcesBuffer[20];
    ACPI_RESOURCE *Resource;
    PIO_RESOURCE_REQUIREMENTS_LIST ReqList;
    PIO_RESOURCE_LIST ReqList2;

    /* Invalid AcpiHandle */
    AcpiCallExpected = FALSE;
    Irp.IoStatus.Status = STATUS_WAIT_0 + 17;
    DeviceData.AcpiHandle = NULL;
    Status = Bus_PDO_QueryResourceRequirements(&DeviceData, &Irp);
    ok(Status == STATUS_WAIT_0 + 17, "Status = 0x%lx\n", Status);

    /* PCI Bus device */
    AcpiCallExpected = FALSE;
    Irp.IoStatus.Status = STATUS_WAIT_0 + 17;
    DeviceData.AcpiHandle = CorrectHandle;
    DeviceData.HardwareIDs = L"PNP0A03\0";
    Status = Bus_PDO_QueryResourceRequirements(&DeviceData, &Irp);
    ok(Status == STATUS_WAIT_0 + 17, "Status = 0x%lx\n", Status);

    /* PCI Bus device #2 */
    AcpiCallExpected = FALSE;
    Irp.IoStatus.Status = STATUS_WAIT_0 + 17;
    DeviceData.AcpiHandle = CorrectHandle;
    DeviceData.HardwareIDs = L"PNP0A08\0";
    Status = Bus_PDO_QueryResourceRequirements(&DeviceData, &Irp);
    ok(Status == STATUS_WAIT_0 + 17, "Status = 0x%lx\n", Status);

    /* Empty buffer */
    AcpiCallExpected = TRUE;
    Irp.IoStatus.Status = STATUS_WAIT_0 + 17;
    DeviceData.AcpiHandle = CorrectHandle;
    DeviceData.HardwareIDs = L"PNP0501\0";
    Status = Bus_PDO_QueryResourceRequirements(&DeviceData, &Irp);
    ok(Status == STATUS_WAIT_0 + 17, "Status = 0x%lx\n", Status);

    /* Simple single-resource list */
    AcpiCallExpected = TRUE;
    Irp.IoStatus.Status = STATUS_WAIT_0 + 17;
    Irp.IoStatus.Information = 0;
    DeviceData.AcpiHandle = CorrectHandle;
    DeviceData.HardwareIDs = L"PNP0501\0";
    Resource = ResourcesBuffer;
    Resource->Type = ACPI_RESOURCE_TYPE_IRQ;
    Resource->Length = sizeof(*Resource);
    MAKE_IRQ(Resource, 3, ACPI_LEVEL_SENSITIVE, ACPI_ACTIVE_HIGH, ACPI_EXCLUSIVE, ACPI_NOT_WAKE_CAPABLE);
    Resource->Data.Irq.InterruptCount = 1;
    Resource->Data.Irq.Interrupts[0] = 7;
    Resource = ACPI_NEXT_RESOURCE(Resource);
    Resource->Type = ACPI_RESOURCE_TYPE_END_TAG;
    Resource->Length = 0;
    Resource++;
    PossibleBuffer.Pointer = ResourcesBuffer;
    PossibleBuffer.Length = (ULONG_PTR)Resource - (ULONG_PTR)ResourcesBuffer;
    Status = Bus_PDO_QueryResourceRequirements(&DeviceData, &Irp);
    ok(Status == STATUS_SUCCESS, "Status = 0x%lx\n", Status);
    ok(Irp.IoStatus.Status == STATUS_WAIT_0 + 17, "IoStatus.Status = 0x%lx\n", Irp.IoStatus.Status);
    ReqList = (PVOID)Irp.IoStatus.Information;
    ok(ReqList != NULL, "ReqList is NULL\n");
    if (ReqList == NULL)
    {
        skip("No ReqList\n");
        return;
    }
    ok(ReqList->InterfaceType == Internal, "InterfaceType = %u\n", ReqList->InterfaceType);
    ok(ReqList->BusNumber == 0, "BusNumber = %lu\n", ReqList->BusNumber);
    ok(ReqList->SlotNumber == 0, "SlotNumber = %lu\n", ReqList->SlotNumber);
    ok(ReqList->AlternativeLists == 1, "AlternativeLists = %lu\n", ReqList->AlternativeLists);
    ok(ReqList->List[0].Version == 1, "List[0].Version = %u\n", ReqList->List[0].Version);
    ok(ReqList->List[0].Revision == 1, "List[0].Revision = %u\n", ReqList->List[0].Revision);
    ok(ReqList->List[0].Count == 1, "List[0].Count = %lu\n", ReqList->List[0].Count);
    expect_irq(&ReqList->List[0].Descriptors[0], IO_RESOURCE_PREFERRED, CmResourceShareDeviceExclusive, 7, 7);
    ok_int(ReqList->ListSize, GetPoolAllocSize(ReqList));
    ok_int(ReqList->ListSize, (ULONG_PTR)&ReqList->List[0].Descriptors[1] - (ULONG_PTR)ReqList);
    ExFreePoolWithTag(ReqList, 'RpcA');

    /* Two IRQs */
    AcpiCallExpected = TRUE;
    Irp.IoStatus.Status = STATUS_WAIT_0 + 17;
    Irp.IoStatus.Information = 0;
    DeviceData.AcpiHandle = CorrectHandle;
    DeviceData.HardwareIDs = L"PNP0501\0";
    Resource = ResourcesBuffer;
    Resource->Type = ACPI_RESOURCE_TYPE_IRQ;
    Resource->Length = FIELD_OFFSET(ACPI_RESOURCE, Data) + FIELD_OFFSET(ACPI_RESOURCE_IRQ, Interrupts[2]);
    MAKE_IRQ(Resource, 3, ACPI_LEVEL_SENSITIVE, ACPI_ACTIVE_HIGH, ACPI_EXCLUSIVE, ACPI_NOT_WAKE_CAPABLE);
    Resource->Data.Irq.InterruptCount = 2;
    Resource->Data.Irq.Interrupts[0] = 3;
    Resource->Data.Irq.Interrupts[1] = 7;
    Resource = ACPI_NEXT_RESOURCE(Resource);
    Resource->Type = ACPI_RESOURCE_TYPE_END_TAG;
    Resource->Length = 0;
    Resource++;
    PossibleBuffer.Pointer = ResourcesBuffer;
    PossibleBuffer.Length = (ULONG_PTR)Resource - (ULONG_PTR)ResourcesBuffer;
    Status = Bus_PDO_QueryResourceRequirements(&DeviceData, &Irp);
    ok(Status == STATUS_SUCCESS, "Status = 0x%lx\n", Status);
    ok(Irp.IoStatus.Status == STATUS_WAIT_0 + 17, "IoStatus.Status = 0x%lx\n", Irp.IoStatus.Status);
    ReqList = (PVOID)Irp.IoStatus.Information;
    ok(ReqList != NULL, "ReqList is NULL\n");
    if (ReqList == NULL)
    {
        skip("No ReqList\n");
        return;
    }
    ok(ReqList->InterfaceType == Internal, "InterfaceType = %u\n", ReqList->InterfaceType);
    ok(ReqList->BusNumber == 0, "BusNumber = %lu\n", ReqList->BusNumber);
    ok(ReqList->SlotNumber == 0, "SlotNumber = %lu\n", ReqList->SlotNumber);
    ok(ReqList->AlternativeLists == 1, "AlternativeLists = %lu\n", ReqList->AlternativeLists);
    ok(ReqList->List[0].Version == 1, "List[0].Version = %u\n", ReqList->List[0].Version);
    ok(ReqList->List[0].Revision == 1, "List[0].Revision = %u\n", ReqList->List[0].Revision);
    ok(ReqList->List[0].Count == 2, "List[0].Count = %lu\n", ReqList->List[0].Count);
    expect_irq(&ReqList->List[0].Descriptors[0], IO_RESOURCE_PREFERRED, CmResourceShareDeviceExclusive, 3, 3);
    expect_irq(&ReqList->List[0].Descriptors[1], IO_RESOURCE_ALTERNATIVE, CmResourceShareDeviceExclusive, 7, 7);
    ok_int(ReqList->ListSize, GetPoolAllocSize(ReqList));
    ok_int(ReqList->ListSize, (ULONG_PTR)&ReqList->List[0].Descriptors[2] - (ULONG_PTR)ReqList);
    ExFreePoolWithTag(ReqList, 'RpcA');

    /* Port */
    AcpiCallExpected = TRUE;
    Irp.IoStatus.Status = STATUS_WAIT_0 + 17;
    Irp.IoStatus.Information = 0;
    DeviceData.AcpiHandle = CorrectHandle;
    DeviceData.HardwareIDs = L"PNP0501\0";
    Resource = ResourcesBuffer;
    Resource->Type = ACPI_RESOURCE_TYPE_IO;
    Resource->Length = sizeof(*Resource);
    Resource->Data.Io.IoDecode = ACPI_DECODE_16;
    Resource->Data.Io.Alignment = 8;
    Resource->Data.Io.AddressLength = 8;
    Resource->Data.Io.Minimum = 0x3F8;
    Resource->Data.Io.Maximum = 0x3F8;
    Resource = ACPI_NEXT_RESOURCE(Resource);
    Resource->Type = ACPI_RESOURCE_TYPE_END_TAG;
    Resource->Length = 0;
    Resource++;
    PossibleBuffer.Pointer = ResourcesBuffer;
    PossibleBuffer.Length = (ULONG_PTR)Resource - (ULONG_PTR)ResourcesBuffer;
    Status = Bus_PDO_QueryResourceRequirements(&DeviceData, &Irp);
    ok(Status == STATUS_SUCCESS, "Status = 0x%lx\n", Status);
    ok(Irp.IoStatus.Status == STATUS_WAIT_0 + 17, "IoStatus.Status = 0x%lx\n", Irp.IoStatus.Status);
    ReqList = (PVOID)Irp.IoStatus.Information;
    ok(ReqList != NULL, "ReqList is NULL\n");
    if (ReqList == NULL)
    {
        skip("No ReqList\n");
        return;
    }
    ok(ReqList->InterfaceType == Internal, "InterfaceType = %u\n", ReqList->InterfaceType);
    ok(ReqList->BusNumber == 0, "BusNumber = %lu\n", ReqList->BusNumber);
    ok(ReqList->SlotNumber == 0, "SlotNumber = %lu\n", ReqList->SlotNumber);
    ok(ReqList->AlternativeLists == 1, "AlternativeLists = %lu\n", ReqList->AlternativeLists);
    ok(ReqList->List[0].Version == 1, "List[0].Version = %u\n", ReqList->List[0].Version);
    ok(ReqList->List[0].Revision == 1, "List[0].Revision = %u\n", ReqList->List[0].Revision);
    ok(ReqList->List[0].Count == 1, "List[0].Count = %lu\n", ReqList->List[0].Count);
    expect_port(&ReqList->List[0].Descriptors[0], IO_RESOURCE_PREFERRED, CmResourceShareDriverExclusive, 8, 8, 0x3F8, 0x3FF);
    ok_int(ReqList->ListSize, GetPoolAllocSize(ReqList));
    ok_int(ReqList->ListSize, (ULONG_PTR)&ReqList->List[0].Descriptors[1] - (ULONG_PTR)ReqList);
    ExFreePoolWithTag(ReqList, 'RpcA');

    /* Port + two IRQs */
    AcpiCallExpected = TRUE;
    Irp.IoStatus.Status = STATUS_WAIT_0 + 17;
    Irp.IoStatus.Information = 0;
    DeviceData.AcpiHandle = CorrectHandle;
    DeviceData.HardwareIDs = L"PNP0501\0";
    Resource = ResourcesBuffer;
    Resource->Type = ACPI_RESOURCE_TYPE_IO;
    Resource->Length = sizeof(*Resource);
    Resource->Data.Io.IoDecode = ACPI_DECODE_16;
    Resource->Data.Io.Alignment = 8;
    Resource->Data.Io.AddressLength = 8;
    Resource->Data.Io.Minimum = 0x3F8;
    Resource->Data.Io.Maximum = 0x3F8;
    Resource = ACPI_NEXT_RESOURCE(Resource);
    Resource->Type = ACPI_RESOURCE_TYPE_IRQ;
    Resource->Length = FIELD_OFFSET(ACPI_RESOURCE, Data) + FIELD_OFFSET(ACPI_RESOURCE_IRQ, Interrupts[2]);
    MAKE_IRQ(Resource, 3, ACPI_LEVEL_SENSITIVE, ACPI_ACTIVE_HIGH, ACPI_EXCLUSIVE, ACPI_NOT_WAKE_CAPABLE);
    Resource->Data.Irq.InterruptCount = 2;
    Resource->Data.Irq.Interrupts[0] = 3;
    Resource->Data.Irq.Interrupts[1] = 7;
    Resource = ACPI_NEXT_RESOURCE(Resource);
    Resource->Type = ACPI_RESOURCE_TYPE_END_TAG;
    Resource->Length = 0;
    Resource++;
    PossibleBuffer.Pointer = ResourcesBuffer;
    PossibleBuffer.Length = (ULONG_PTR)Resource - (ULONG_PTR)ResourcesBuffer;
    Status = Bus_PDO_QueryResourceRequirements(&DeviceData, &Irp);
    ok(Status == STATUS_SUCCESS, "Status = 0x%lx\n", Status);
    ok(Irp.IoStatus.Status == STATUS_WAIT_0 + 17, "IoStatus.Status = 0x%lx\n", Irp.IoStatus.Status);
    ReqList = (PVOID)Irp.IoStatus.Information;
    ok(ReqList != NULL, "ReqList is NULL\n");
    if (ReqList == NULL)
    {
        skip("No ReqList\n");
        return;
    }
    ok(ReqList->InterfaceType == Internal, "InterfaceType = %u\n", ReqList->InterfaceType);
    ok(ReqList->BusNumber == 0, "BusNumber = %lu\n", ReqList->BusNumber);
    ok(ReqList->SlotNumber == 0, "SlotNumber = %lu\n", ReqList->SlotNumber);
    ok(ReqList->AlternativeLists == 1, "AlternativeLists = %lu\n", ReqList->AlternativeLists);
    ok(ReqList->List[0].Version == 1, "List[0].Version = %u\n", ReqList->List[0].Version);
    ok(ReqList->List[0].Revision == 1, "List[0].Revision = %u\n", ReqList->List[0].Revision);
    ok(ReqList->List[0].Count == 3, "List[0].Count = %lu\n", ReqList->List[0].Count);
    expect_port(&ReqList->List[0].Descriptors[0], IO_RESOURCE_PREFERRED, CmResourceShareDriverExclusive, 8, 8, 0x3F8, 0x3FF);
    expect_irq(&ReqList->List[0].Descriptors[1], IO_RESOURCE_PREFERRED, CmResourceShareDeviceExclusive, 3, 3);
    expect_irq(&ReqList->List[0].Descriptors[2], IO_RESOURCE_ALTERNATIVE, CmResourceShareDeviceExclusive, 7, 7);
    ok_int(ReqList->ListSize, GetPoolAllocSize(ReqList));
    ok_int(ReqList->ListSize, (ULONG_PTR)&ReqList->List[0].Descriptors[3] - (ULONG_PTR)ReqList);
    ExFreePoolWithTag(ReqList, 'RpcA');

    /* Multiple alternatives for ports + IRQs (VMware COM port, simplified) */
    AcpiCallExpected = TRUE;
    Irp.IoStatus.Status = STATUS_WAIT_0 + 17;
    Irp.IoStatus.Information = 0;
    DeviceData.AcpiHandle = CorrectHandle;
    DeviceData.HardwareIDs = L"PNP0501\0";
    Resource = ResourcesBuffer;
    Resource->Type = ACPI_RESOURCE_TYPE_START_DEPENDENT;
    Resource->Length = sizeof(*Resource);

    Resource = ACPI_NEXT_RESOURCE(Resource);
    Resource->Type = ACPI_RESOURCE_TYPE_IO;
    Resource->Length = sizeof(*Resource);
    Resource->Data.Io.IoDecode = ACPI_DECODE_16;
    Resource->Data.Io.Alignment = 8;
    Resource->Data.Io.AddressLength = 8;
    Resource->Data.Io.Minimum = 0x3E8;
    Resource->Data.Io.Maximum = 0x3E8;

    Resource = ACPI_NEXT_RESOURCE(Resource);
    Resource->Type = ACPI_RESOURCE_TYPE_IRQ;
    Resource->Length = FIELD_OFFSET(ACPI_RESOURCE, Data) + FIELD_OFFSET(ACPI_RESOURCE_IRQ, Interrupts[5]);
    MAKE_IRQ(Resource, 3, ACPI_LEVEL_SENSITIVE, ACPI_ACTIVE_HIGH, ACPI_EXCLUSIVE, ACPI_NOT_WAKE_CAPABLE);
    Resource->Data.Irq.InterruptCount = 5;
    Resource->Data.Irq.Interrupts[0] = 3;
    Resource->Data.Irq.Interrupts[1] = 4;
    Resource->Data.Irq.Interrupts[2] = 5;
    Resource->Data.Irq.Interrupts[3] = 6;
    Resource->Data.Irq.Interrupts[4] = 7;

    Resource = ACPI_NEXT_RESOURCE(Resource);
    Resource->Type = ACPI_RESOURCE_TYPE_START_DEPENDENT;
    Resource->Length = sizeof(*Resource);

    Resource = ACPI_NEXT_RESOURCE(Resource);
    Resource->Type = ACPI_RESOURCE_TYPE_IO;
    Resource->Length = sizeof(*Resource);
    Resource->Data.Io.IoDecode = ACPI_DECODE_16;
    Resource->Data.Io.Alignment = 8;
    Resource->Data.Io.AddressLength = 8;
    Resource->Data.Io.Minimum = 0x2E8;
    Resource->Data.Io.Maximum = 0x2E8;

    Resource = ACPI_NEXT_RESOURCE(Resource);
    Resource->Type = ACPI_RESOURCE_TYPE_IRQ;
    Resource->Length = FIELD_OFFSET(ACPI_RESOURCE, Data) + FIELD_OFFSET(ACPI_RESOURCE_IRQ, Interrupts[5]);
    MAKE_IRQ(Resource, 3, ACPI_LEVEL_SENSITIVE, ACPI_ACTIVE_HIGH, ACPI_EXCLUSIVE, ACPI_NOT_WAKE_CAPABLE);
    Resource->Data.Irq.InterruptCount = 5;
    Resource->Data.Irq.Interrupts[0] = 3;
    Resource->Data.Irq.Interrupts[1] = 4;
    Resource->Data.Irq.Interrupts[2] = 5;
    Resource->Data.Irq.Interrupts[3] = 6;
    Resource->Data.Irq.Interrupts[4] = 7;

    Resource = ACPI_NEXT_RESOURCE(Resource);
    Resource->Type = ACPI_RESOURCE_TYPE_END_DEPENDENT;
    Resource->Length = sizeof(*Resource);

    Resource = ACPI_NEXT_RESOURCE(Resource);
    Resource->Type = ACPI_RESOURCE_TYPE_END_TAG;
    Resource->Length = 0;
    Resource++;
    PossibleBuffer.Pointer = ResourcesBuffer;
    PossibleBuffer.Length = (ULONG_PTR)Resource - (ULONG_PTR)ResourcesBuffer;
    Status = Bus_PDO_QueryResourceRequirements(&DeviceData, &Irp);
    ok(Status == STATUS_SUCCESS, "Status = 0x%lx\n", Status);
    ok(Irp.IoStatus.Status == STATUS_WAIT_0 + 17, "IoStatus.Status = 0x%lx\n", Irp.IoStatus.Status);
    ReqList = (PVOID)Irp.IoStatus.Information;
    ok(ReqList != NULL, "ReqList is NULL\n");
    if (ReqList == NULL)
    {
        skip("No ReqList\n");
        return;
    }
    ok(ReqList->InterfaceType == Internal, "InterfaceType = %u\n", ReqList->InterfaceType);
    ok(ReqList->BusNumber == 0, "BusNumber = %lu\n", ReqList->BusNumber);
    ok(ReqList->SlotNumber == 0, "SlotNumber = %lu\n", ReqList->SlotNumber);
    ok(ReqList->AlternativeLists == 2, "AlternativeLists = %lu\n", ReqList->AlternativeLists);
    ok(ReqList->List[0].Version == 1, "List[0].Version = %u\n", ReqList->List[0].Version);
    ok(ReqList->List[0].Revision == 1, "List[0].Revision = %u\n", ReqList->List[0].Revision);
    ok(ReqList->List[0].Count == 6, "List[0].Count = %lu\n", ReqList->List[0].Count);
    expect_port(&ReqList->List[0].Descriptors[0], IO_RESOURCE_PREFERRED, CmResourceShareDriverExclusive, 8, 8, 0x3E8, 0x3EF);
    expect_irq(&ReqList->List[0].Descriptors[1], IO_RESOURCE_PREFERRED, CmResourceShareDeviceExclusive, 3, 3);
    expect_irq(&ReqList->List[0].Descriptors[2], IO_RESOURCE_ALTERNATIVE, CmResourceShareDeviceExclusive, 4, 4);
    expect_irq(&ReqList->List[0].Descriptors[3], IO_RESOURCE_ALTERNATIVE, CmResourceShareDeviceExclusive, 5, 5);
    expect_irq(&ReqList->List[0].Descriptors[4], IO_RESOURCE_ALTERNATIVE, CmResourceShareDeviceExclusive, 6, 6);
    expect_irq(&ReqList->List[0].Descriptors[5], IO_RESOURCE_ALTERNATIVE, CmResourceShareDeviceExclusive, 7, 7);
    ReqList2 = (PVOID)&ReqList->List[0].Descriptors[6];
    if (ReqList->ListSize > (ULONG_PTR)ReqList2 - (ULONG_PTR)ReqList)
    {
        ok(ReqList2->Version == 1, "List[1].Version = %u\n", ReqList->List[0].Version);
        ok(ReqList2->Revision == 1, "List[1].Revision = %u\n", ReqList->List[0].Revision);
        ok(ReqList2->Count == 6, "List[1].Count = %lu\n", ReqList->List[0].Count);
        expect_port(&ReqList2->Descriptors[0], IO_RESOURCE_PREFERRED, CmResourceShareDriverExclusive, 8, 8, 0x2E8, 0x2EF);
        expect_irq(&ReqList2->Descriptors[1], IO_RESOURCE_PREFERRED, CmResourceShareDeviceExclusive, 3, 3);
        expect_irq(&ReqList2->Descriptors[2], IO_RESOURCE_ALTERNATIVE, CmResourceShareDeviceExclusive, 4, 4);
        expect_irq(&ReqList2->Descriptors[3], IO_RESOURCE_ALTERNATIVE, CmResourceShareDeviceExclusive, 5, 5);
        expect_irq(&ReqList2->Descriptors[4], IO_RESOURCE_ALTERNATIVE, CmResourceShareDeviceExclusive, 6, 6);
        expect_irq(&ReqList2->Descriptors[5], IO_RESOURCE_ALTERNATIVE, CmResourceShareDeviceExclusive, 7, 7);
    }
    ok_int(ReqList->ListSize, GetPoolAllocSize(ReqList));
    ok_int(ReqList->ListSize, (ULONG_PTR)&ReqList2->Descriptors[6] - (ULONG_PTR)ReqList);
    ExFreePoolWithTag(ReqList, 'RpcA');
}
