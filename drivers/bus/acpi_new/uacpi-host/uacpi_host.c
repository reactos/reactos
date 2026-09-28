/*
 * PROJECT:     uACPI-NT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     uACPI kernel API on top of ntoskrnl
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include <uacpint.h>
#include <uacpi/kernel_api.h>
#include <uacpi/log.h>
#include <uacpi/tables.h>
#include <uacpi/acpi.h>
#include <arc/arc.h>
#include <reactos/drivers/acpi/acpi.h>
#include "uacpintsci.h"
#include <debug.h>

/* Exported by ntoskrnl but not declared in the WDK */
extern NTSYSAPI PLOADER_PARAMETER_BLOCK KeLoaderBlock;

NTSYSAPI
PCONFIGURATION_COMPONENT_DATA
NTAPI
KeFindConfigurationNextEntry(
    _In_ PCONFIGURATION_COMPONENT_DATA Child,
    _In_ CONFIGURATION_CLASS Class,
    _In_ CONFIGURATION_TYPE Type,
    _In_opt_ PULONG ComponentKey,
    _Inout_ PCONFIGURATION_COMPONENT_DATA *NextLink);

/* uACPI timeout value meaning wait forever */
#define UACPINT_HOST_WAIT_FOREVER           0xFFFF

/* Seconds between reports while an infinite mutex wait is stuck */
#define UACPINT_HOST_MUTEX_REPORT_SECONDS   5

/* Mutexes tracked for the stuck report, a full table leaves the owner unknown */
#define UACPINT_HOST_MUTEX_OWNERS           64

/* MCFG segment windows kept, extra entries are ignored */
#define UACPINT_HOST_MAX_ECAM_WINDOWS       16

/* The ACPI 1.0 part of the RSDP covered by its checksum */
#define UACPINT_HOST_RSDP_V1_LENGTH         20

typedef struct _UACPINT_HOST_PCI_DEVICE
{
    uacpi_pci_address Address;
    PUCHAR            EcamPage;     ///< mapped 4 KB config page, NULL for type 1
} UACPINT_HOST_PCI_DEVICE, *PUACPINT_HOST_PCI_DEVICE;

typedef struct _UACPINT_HOST_ECAM_WINDOW
{
    uacpi_u64 Base;
    uacpi_u16 Segment;
    uacpi_u8  StartBus;
    uacpi_u8  EndBus;
} UACPINT_HOST_ECAM_WINDOW, *PUACPINT_HOST_ECAM_WINDOW;

typedef struct _UACPINT_HOST_MUTEX_OWNER
{
    volatile PVOID Handle;
    volatile PVOID Owner;
} UACPINT_HOST_MUTEX_OWNER, *PUACPINT_HOST_MUTEX_OWNER;

ULONG UacpiNtHostVerbose = 1;

/* QuadPart 0 means no override */
static PHYSICAL_ADDRESS UacpiNtHostRsdpOverridePa;

/* RSDP built around the loader's root table, kept for the driver lifetime */
static struct acpi_rsdp *UacpiNtHostLoaderRsdp;

static UACPINT_HOST_ECAM_WINDOW UacpiNtHostEcam[UACPINT_HOST_MAX_ECAM_WINDOWS];
static ULONG UacpiNtHostEcamCount;
static BOOLEAN UacpiNtHostEcamParsed;

static UACPINT_HOST_MUTEX_OWNER UacpiNtHostMutexOwners[UACPINT_HOST_MUTEX_OWNERS];

static
ULONG
NTAPI
UacpiNtHostFilterLevel(
    _In_ uacpi_log_level Level)
{
    /* Verbose mode makes every interpreter line visible */
    if (UacpiNtHostVerbose)
        return DPFLTR_ERROR_LEVEL;

    switch (Level)
    {
        case UACPI_LOG_ERROR:
            return DPFLTR_ERROR_LEVEL;

        case UACPI_LOG_WARN:
            return DPFLTR_WARNING_LEVEL;

        default:
            return DPFLTR_INFO_LEVEL;
    }
}

void
uacpi_kernel_log(
    _In_ uacpi_log_level Level,
    _In_z_ const uacpi_char *String)
{
    /* uACPI already ends the line */
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, UacpiNtHostFilterLevel(Level), "uACPI: %s", String);
}

VOID
NTAPI
UacpiNtHostRsdpOverride(
    _In_ PHYSICAL_ADDRESS RsdpPhysical)
{
    UacpiNtHostRsdpOverridePa = RsdpPhysical;
}

static
UCHAR
NTAPI
UacpiNtHostByteSum(
    _In_reads_bytes_(Length) const UCHAR *Buffer,
    _In_ ULONG Length)
{
    UCHAR Sum = 0;
    ULONG Index;

    for (Index = 0; Index < Length; Index++)
        Sum = (UCHAR)(Sum + Buffer[Index]);

    return Sum;
}

static
BOOLEAN
NTAPI
UacpiNtHostIsRsdp(
    _In_reads_bytes_(UACPINT_HOST_RSDP_V1_LENGTH) const UCHAR *Candidate)
{
    if (!RtlEqualMemory(Candidate, "RSD PTR ", 8))
        return FALSE;

    return UacpiNtHostByteSum(Candidate, UACPINT_HOST_RSDP_V1_LENGTH) == 0;
}

/*
 * Look for the RSDP on 16 byte boundaries in [Start, Start + Length).
 * Returns 0 when nothing is found.
 */
static
uacpi_phys_addr
NTAPI
UacpiNtHostScanForRsdp(
    _In_ ULONG_PTR Start,
    _In_ ULONG Length)
{
    PHYSICAL_ADDRESS Physical;
    uacpi_phys_addr Found = 0;
    PUCHAR Mapping;
    ULONG Offset;

    Physical.QuadPart = Start;
    Mapping = MmMapIoSpace(Physical, Length, MmCached);
    if (!Mapping)
        return 0;

    for (Offset = 0; Offset + UACPINT_HOST_RSDP_V1_LENGTH <= Length; Offset += 16)
    {
        if (UacpiNtHostIsRsdp(Mapping + Offset))
        {
            Found = (uacpi_phys_addr)(Start + Offset);
            break;
        }
    }

    MmUnmapIoSpace(Mapping, Length);
    return Found;
}

static
PACPI_BIOS_MULTI_NODE
NTAPI
UacpiNtHostFindLoaderAcpiNode(VOID)
{
    PCONFIGURATION_COMPONENT_DATA Root;
    PCONFIGURATION_COMPONENT_DATA Entry;
    PCONFIGURATION_COMPONENT_DATA Next = NULL;
    PCM_PARTIAL_RESOURCE_LIST ResourceList;
    ULONG MinimumLength;

    if (!KeLoaderBlock || !KeLoaderBlock->ConfigurationRoot)
        return NULL;

    /* The ConfigurationRoot offset is not verified on every release, so sanity check it */
    Root = KeLoaderBlock->ConfigurationRoot;
    if (!MmIsAddressValid(Root) ||
        !MmIsAddressValid((PUCHAR)(Root + 1) - 1) ||
        Root->Parent ||
        Root->ComponentEntry.Class != SystemClass)
    {
        DPRINT1("uACPI-NT: loader ConfigurationRoot %p is not the ARC system node\n", Root);
        return NULL;
    }

    for (;;)
    {
        Entry = KeFindConfigurationNextEntry(Root, AdapterClass, MultiFunctionAdapter, NULL, &Next);
        if (!Entry)
            return NULL;

        if (Entry->ComponentEntry.Identifier &&
            _stricmp(Entry->ComponentEntry.Identifier, "ACPI BIOS") == 0)
        {
            break;
        }

        Next = Entry;
    }

    /* The node sits behind a one entry DeviceSpecific resource list */
    ResourceList = Entry->ConfigurationData;
    MinimumLength = FIELD_OFFSET(CM_PARTIAL_RESOURCE_LIST, PartialDescriptors[1]) +
                    FIELD_OFFSET(ACPI_BIOS_MULTI_NODE, E820Entry);
    if (!ResourceList ||
        Entry->ComponentEntry.ConfigurationDataLength < MinimumLength ||
        ResourceList->Count < 1 ||
        ResourceList->PartialDescriptors[0].Type != CmResourceTypeDeviceSpecific)
    {
        DPRINT1("uACPI-NT: loader ACPI BIOS node has no device specific data\n");
        return NULL;
    }

    return (PACPI_BIOS_MULTI_NODE)(ResourceList + 1);
}

/*
 * Build an RSDP for the root table the loader handed us. This is the only
 * source on UEFI machines. Returns 0 on failure.
 */
static
uacpi_phys_addr
NTAPI
UacpiNtHostRsdpFromLoader(VOID)
{
    PACPI_BIOS_MULTI_NODE Node;
    struct acpi_sdt_hdr *RootTable;
    struct acpi_rsdp *Rsdp;
    PHYSICAL_ADDRESS RootAddress;

    if (UacpiNtHostLoaderRsdp)
        return (uacpi_phys_addr)MmGetPhysicalAddress(UacpiNtHostLoaderRsdp).QuadPart;

    Node = UacpiNtHostFindLoaderAcpiNode();
    if (!Node || Node->RsdtAddress.QuadPart == 0)
        return 0;

    RootAddress = Node->RsdtAddress;
    RootTable = MmMapIoSpace(RootAddress, sizeof(*RootTable), MmNonCached);
    if (!RootTable)
    {
        DPRINT1("uACPI-NT: cannot map the loader root table at %010I64X\n", RootAddress.QuadPart);
        return 0;
    }

    Rsdp = ExAllocatePoolWithTag(NonPagedPool, sizeof(*Rsdp), UACPINT_HOST_POOL_TAG);
    if (!Rsdp)
    {
        MmUnmapIoSpace(RootTable, sizeof(*RootTable));
        return 0;
    }

    RtlZeroMemory(Rsdp, sizeof(*Rsdp));
    RtlCopyMemory(Rsdp->signature, "RSD PTR ", sizeof(Rsdp->signature));
    RtlCopyMemory(Rsdp->oemid, "ROS   ", sizeof(Rsdp->oemid));

    /* The root table signature picks the RSDP revision */
    if (RtlEqualMemory(RootTable->signature, "XSDT", 4))
    {
        Rsdp->revision = 2;
        Rsdp->length = sizeof(*Rsdp);
        Rsdp->xsdt_addr = (uacpi_u64)RootAddress.QuadPart;
    }
    else if (RtlEqualMemory(RootTable->signature, "RSDT", 4) &&
             RootAddress.QuadPart <= MAXULONG)
    {
        Rsdp->revision = 0;
        Rsdp->rsdt_addr = RootAddress.LowPart;
    }
    else
    {
        DPRINT1("uACPI-NT: loader root table at %010I64X is '%.4s', not an RSDT or XSDT\n",
                RootAddress.QuadPart,
                RootTable->signature);
        MmUnmapIoSpace(RootTable, sizeof(*RootTable));
        ExFreePoolWithTag(Rsdp, UACPINT_HOST_POOL_TAG);
        return 0;
    }

    MmUnmapIoSpace(RootTable, sizeof(*RootTable));

    Rsdp->checksum = (uacpi_u8)(0 - UacpiNtHostByteSum((const UCHAR *)Rsdp,
                                                       UACPINT_HOST_RSDP_V1_LENGTH));
    if (Rsdp->revision >= 2)
        Rsdp->extended_checksum = (uacpi_u8)(0 - UacpiNtHostByteSum((const UCHAR *)Rsdp,
                                                                    sizeof(*Rsdp)));

    UacpiNtHostLoaderRsdp = Rsdp;
    DPRINT1("uACPI-NT: RSDP built for the loader %s at %010I64X\n",
            Rsdp->revision >= 2 ? "XSDT" : "RSDT",
            RootAddress.QuadPart);

    return (uacpi_phys_addr)MmGetPhysicalAddress(Rsdp).QuadPart;
}

uacpi_status
uacpi_kernel_get_rsdp(
    _Out_ uacpi_phys_addr *OutRsdpAddress)
{
    PHYSICAL_ADDRESS Physical;
    uacpi_phys_addr Rsdp;
    ULONG_PTR Ebda;
    PUSHORT EbdaSegment;

    if (UacpiNtHostRsdpOverridePa.QuadPart != 0)
    {
        *OutRsdpAddress = (uacpi_phys_addr)UacpiNtHostRsdpOverridePa.QuadPart;
        return UACPI_STATUS_OK;
    }

    Rsdp = UacpiNtHostRsdpFromLoader();

    /* Then the first KB of the EBDA, whose segment is the word at 0x40E */
    if (Rsdp == 0)
    {
        Physical.QuadPart = 0x40E;
        EbdaSegment = MmMapIoSpace(Physical, sizeof(*EbdaSegment), MmCached);
        if (EbdaSegment)
        {
            Ebda = (ULONG_PTR)*EbdaSegment << 4;
            MmUnmapIoSpace(EbdaSegment, sizeof(*EbdaSegment));

            if (Ebda >= 0x400 && Ebda < 0xA0000)
                Rsdp = UacpiNtHostScanForRsdp(Ebda, 0x400);
        }
    }

    /* Then the BIOS read only area */
    if (Rsdp == 0)
        Rsdp = UacpiNtHostScanForRsdp(0xE0000, 0x20000);

    if (Rsdp == 0)
    {
        DPRINT1("uACPI-NT: no RSDP from the loader and none in low memory\n");
        return UACPI_STATUS_NOT_FOUND;
    }

    *OutRsdpAddress = Rsdp;
    return UACPI_STATUS_OK;
}

void *
uacpi_kernel_map(
    _In_ uacpi_phys_addr Address,
    _In_ uacpi_size Length)
{
    PHYSICAL_ADDRESS Physical;
    ULONG_PTR PageOffset;
    PUCHAR Mapping;

    /* MmMapIoSpace wants a page aligned base, hand back the original offset */
    PageOffset = (ULONG_PTR)(Address & (PAGE_SIZE - 1));
    Physical.QuadPart = (LONGLONG)(Address - PageOffset);

    Mapping = MmMapIoSpace(Physical, ROUND_TO_PAGES(PageOffset + Length), MmCached);
    if (!Mapping)
        return UACPI_MAP_FAILED;

    return Mapping + PageOffset;
}

void
uacpi_kernel_unmap(
    _In_ void *Address,
    _In_ uacpi_size Length)
{
    ULONG_PTR PageOffset = (ULONG_PTR)Address & (PAGE_SIZE - 1);

    MmUnmapIoSpace((PUCHAR)Address - PageOffset, ROUND_TO_PAGES(PageOffset + Length));
}

void *
uacpi_kernel_alloc(
    _In_ uacpi_size Size)
{
    return ExAllocatePoolWithTag(NonPagedPool, Size, UACPINT_HOST_POOL_TAG);
}

void
uacpi_kernel_free(
    _In_opt_ void *Memory)
{
    /* uACPI may free NULL */
    if (Memory)
        ExFreePoolWithTag(Memory, UACPINT_HOST_POOL_TAG);
}

static
VOID
NTAPI
UacpiNtHostParseMcfg(VOID)
{
    struct acpi_mcfg *Mcfg;
    uacpi_table Table;
    ULONG EntryCount;
    ULONG Index;

    if (UacpiNtHostEcamParsed)
        return;

    UacpiNtHostEcamParsed = TRUE;

    /* No MCFG means type 1 access only */
    if (uacpi_unlikely_error(uacpi_table_find_by_signature("MCFG", &Table)) || !Table.ptr)
        return;

    Mcfg = Table.ptr;
    if (Mcfg->hdr.length > sizeof(*Mcfg))
    {
        EntryCount = (Mcfg->hdr.length - (ULONG)sizeof(*Mcfg)) / (ULONG)sizeof(Mcfg->entries[0]);

        for (Index = 0;
             Index < EntryCount && UacpiNtHostEcamCount < UACPINT_HOST_MAX_ECAM_WINDOWS;
             Index++)
        {
            UacpiNtHostEcam[UacpiNtHostEcamCount].Base = Mcfg->entries[Index].address;
            UacpiNtHostEcam[UacpiNtHostEcamCount].Segment = Mcfg->entries[Index].segment;
            UacpiNtHostEcam[UacpiNtHostEcamCount].StartBus = Mcfg->entries[Index].start_bus;
            UacpiNtHostEcam[UacpiNtHostEcamCount].EndBus = Mcfg->entries[Index].end_bus;
            UacpiNtHostEcamCount++;
        }
    }

    uacpi_table_unref(&Table);
    DPRINT("uACPI-NT: %lu MCFG segment window(s)\n", UacpiNtHostEcamCount);
}

/* Physical address of the function's config page, 0 when no window covers it */
static
uacpi_u64
NTAPI
UacpiNtHostEcamPage(
    _In_ const uacpi_pci_address *Address)
{
    PUACPINT_HOST_ECAM_WINDOW Window;
    ULONG Index;

    UacpiNtHostParseMcfg();

    for (Index = 0; Index < UacpiNtHostEcamCount; Index++)
    {
        Window = &UacpiNtHostEcam[Index];
        if (Window->Segment != Address->segment ||
            Address->bus < Window->StartBus ||
            Address->bus > Window->EndBus)
        {
            continue;
        }

        return Window->Base +
               ((uacpi_u64)(Address->bus - Window->StartBus) << 20) +
               ((uacpi_u64)Address->device << 15) +
               ((uacpi_u64)Address->function << 12);
    }

    return 0;
}

uacpi_status
uacpi_kernel_pci_device_open(
    _In_ uacpi_pci_address Address,
    _Out_ uacpi_handle *OutHandle)
{
    PUACPINT_HOST_PCI_DEVICE Device;
    PHYSICAL_ADDRESS Physical;

    Device = ExAllocatePoolWithTag(NonPagedPool, sizeof(*Device), UACPINT_HOST_POOL_TAG);
    if (!Device)
        return UACPI_STATUS_OUT_OF_MEMORY;

    Device->Address = Address;
    Device->EcamPage = NULL;

    /* Mapping needs PASSIVE_LEVEL, above that fall back to type 1 */
    Physical.QuadPart = (LONGLONG)UacpiNtHostEcamPage(&Address);
    if (Physical.QuadPart != 0 && KeGetCurrentIrql() == PASSIVE_LEVEL)
        Device->EcamPage = MmMapIoSpace(Physical, PAGE_SIZE, MmNonCached);

    *OutHandle = Device;
    return UACPI_STATUS_OK;
}

void
uacpi_kernel_pci_device_close(
    _In_opt_ uacpi_handle Handle)
{
    PUACPINT_HOST_PCI_DEVICE Device = Handle;

    if (!Device)
        return;

    if (Device->EcamPage)
        MmUnmapIoSpace(Device->EcamPage, PAGE_SIZE);

    ExFreePoolWithTag(Device, UACPINT_HOST_POOL_TAG);
}

static
ULONG
NTAPI
UacpiNtHostPciSlot(
    _In_ const uacpi_pci_address *Address)
{
    PCI_SLOT_NUMBER Slot;

    Slot.u.AsULONG = 0;
    Slot.u.bits.DeviceNumber = Address->device;
    Slot.u.bits.FunctionNumber = Address->function;
    return Slot.u.AsULONG;
}

/* Config space that cannot be reached reads as all ones */
static
uacpi_status
NTAPI
UacpiNtHostPciRead(
    _In_ uacpi_handle Handle,
    _In_ uacpi_size Offset,
    _Out_writes_bytes_(Width) PVOID Value,
    _In_ ULONG Width)
{
    PUACPINT_HOST_PCI_DEVICE Device = Handle;
    PUCHAR Register;
    ULONG BytesRead;

    if (Device->EcamPage && Offset + Width <= PAGE_SIZE)
    {
        Register = Device->EcamPage + Offset;
        switch (Width)
        {
            case sizeof(UCHAR):
                *(PUCHAR)Value = READ_REGISTER_UCHAR(Register);
                break;

            case sizeof(USHORT):
                *(PUSHORT)Value = READ_REGISTER_USHORT((PUSHORT)Register);
                break;

            case sizeof(ULONG):
                *(PULONG)Value = READ_REGISTER_ULONG((PULONG)Register);
                break;

            default:
                RtlFillMemory(Value, Width, 0xFF);
                break;
        }

        return UACPI_STATUS_OK;
    }

    /* Type 1 only reaches segment 0 and the first 256 bytes */
    if (Device->Address.segment != 0 || Offset + Width > 256)
    {
        RtlFillMemory(Value, Width, 0xFF);
        return UACPI_STATUS_OK;
    }

    BytesRead = HalGetBusDataByOffset(PCIConfiguration,
                                      Device->Address.bus,
                                      UacpiNtHostPciSlot(&Device->Address),
                                      Value,
                                      (ULONG)Offset,
                                      Width);
    if (BytesRead != Width)
        RtlFillMemory(Value, Width, 0xFF);

    return UACPI_STATUS_OK;
}

static
uacpi_status
NTAPI
UacpiNtHostPciWrite(
    _In_ uacpi_handle Handle,
    _In_ uacpi_size Offset,
    _In_reads_bytes_(Width) PVOID Value,
    _In_ ULONG Width)
{
    PUACPINT_HOST_PCI_DEVICE Device = Handle;
    PUCHAR Register;

    if (Device->EcamPage && Offset + Width <= PAGE_SIZE)
    {
        Register = Device->EcamPage + Offset;
        switch (Width)
        {
            case sizeof(UCHAR):
                WRITE_REGISTER_UCHAR(Register, *(PUCHAR)Value);
                break;

            case sizeof(USHORT):
                WRITE_REGISTER_USHORT((PUSHORT)Register, *(PUSHORT)Value);
                break;

            case sizeof(ULONG):
                WRITE_REGISTER_ULONG((PULONG)Register, *(PULONG)Value);
                break;

            default:
                break;
        }

        return UACPI_STATUS_OK;
    }

    /* Out of type 1 reach, drop the write */
    if (Device->Address.segment != 0 || Offset + Width > 256)
        return UACPI_STATUS_OK;

    HalSetBusDataByOffset(PCIConfiguration,
                          Device->Address.bus,
                          UacpiNtHostPciSlot(&Device->Address),
                          Value,
                          (ULONG)Offset,
                          Width);
    return UACPI_STATUS_OK;
}

uacpi_status
uacpi_kernel_pci_read8(
    _In_ uacpi_handle Device,
    _In_ uacpi_size Offset,
    _Out_ uacpi_u8 *Value)
{
    return UacpiNtHostPciRead(Device, Offset, Value, sizeof(*Value));
}

uacpi_status
uacpi_kernel_pci_read16(
    _In_ uacpi_handle Device,
    _In_ uacpi_size Offset,
    _Out_ uacpi_u16 *Value)
{
    return UacpiNtHostPciRead(Device, Offset, Value, sizeof(*Value));
}

uacpi_status
uacpi_kernel_pci_read32(
    _In_ uacpi_handle Device,
    _In_ uacpi_size Offset,
    _Out_ uacpi_u32 *Value)
{
    return UacpiNtHostPciRead(Device, Offset, Value, sizeof(*Value));
}

uacpi_status
uacpi_kernel_pci_write8(
    _In_ uacpi_handle Device,
    _In_ uacpi_size Offset,
    _In_ uacpi_u8 Value)
{
    return UacpiNtHostPciWrite(Device, Offset, &Value, sizeof(Value));
}

uacpi_status
uacpi_kernel_pci_write16(
    _In_ uacpi_handle Device,
    _In_ uacpi_size Offset,
    _In_ uacpi_u16 Value)
{
    return UacpiNtHostPciWrite(Device, Offset, &Value, sizeof(Value));
}

uacpi_status
uacpi_kernel_pci_write32(
    _In_ uacpi_handle Device,
    _In_ uacpi_size Offset,
    _In_ uacpi_u32 Value)
{
    return UacpiNtHostPciWrite(Device, Offset, &Value, sizeof(Value));
}

/* The I/O handle is simply the port base */
uacpi_status
uacpi_kernel_io_map(
    _In_ uacpi_io_addr Base,
    _In_ uacpi_size Length,
    _Out_ uacpi_handle *OutHandle)
{
    UNREFERENCED_PARAMETER(Length);

    *OutHandle = (uacpi_handle)(ULONG_PTR)Base;
    return UACPI_STATUS_OK;
}

void
uacpi_kernel_io_unmap(
    _In_ uacpi_handle Handle)
{
    UNREFERENCED_PARAMETER(Handle);
}

uacpi_status
uacpi_kernel_io_read8(
    _In_ uacpi_handle Handle,
    _In_ uacpi_size Offset,
    _Out_ uacpi_u8 *Value)
{
    *Value = READ_PORT_UCHAR((PUCHAR)((ULONG_PTR)Handle + Offset));
    return UACPI_STATUS_OK;
}

uacpi_status
uacpi_kernel_io_read16(
    _In_ uacpi_handle Handle,
    _In_ uacpi_size Offset,
    _Out_ uacpi_u16 *Value)
{
    *Value = READ_PORT_USHORT((PUSHORT)((ULONG_PTR)Handle + Offset));
    return UACPI_STATUS_OK;
}

uacpi_status
uacpi_kernel_io_read32(
    _In_ uacpi_handle Handle,
    _In_ uacpi_size Offset,
    _Out_ uacpi_u32 *Value)
{
    *Value = READ_PORT_ULONG((PULONG)((ULONG_PTR)Handle + Offset));
    return UACPI_STATUS_OK;
}

uacpi_status
uacpi_kernel_io_write8(
    _In_ uacpi_handle Handle,
    _In_ uacpi_size Offset,
    _In_ uacpi_u8 Value)
{
    WRITE_PORT_UCHAR((PUCHAR)((ULONG_PTR)Handle + Offset), Value);
    return UACPI_STATUS_OK;
}

uacpi_status
uacpi_kernel_io_write16(
    _In_ uacpi_handle Handle,
    _In_ uacpi_size Offset,
    _In_ uacpi_u16 Value)
{
    WRITE_PORT_USHORT((PUSHORT)((ULONG_PTR)Handle + Offset), Value);
    return UACPI_STATUS_OK;
}

uacpi_status
uacpi_kernel_io_write32(
    _In_ uacpi_handle Handle,
    _In_ uacpi_size Offset,
    _In_ uacpi_u32 Value)
{
    WRITE_PORT_ULONG((PULONG)((ULONG_PTR)Handle + Offset), Value);
    return UACPI_STATUS_OK;
}

uacpi_u64
uacpi_kernel_get_nanoseconds_since_boot(void)
{
    LARGE_INTEGER Frequency;
    ULONGLONG Counter;
    ULONGLONG Ticks;

    Counter = (ULONGLONG)KeQueryPerformanceCounter(&Frequency).QuadPart;
    Ticks = (ULONGLONG)Frequency.QuadPart;
    if (Ticks == 0)
        return 0;

    /* Split the scale so Counter * 1e9 cannot overflow */
    return (Counter / Ticks) * 1000000000ULL + ((Counter % Ticks) * 1000000000ULL) / Ticks;
}

void
uacpi_kernel_stall(
    _In_ uacpi_u8 Microseconds)
{
    KeStallExecutionProcessor(Microseconds);
}

void
uacpi_kernel_sleep(
    _In_ uacpi_u64 Milliseconds)
{
    LARGE_INTEGER Interval;

    Interval.QuadPart = -(LONGLONG)(Milliseconds * 10000ULL);
    KeDelayExecutionThread(KernelMode, FALSE, &Interval);
}

/* Waits above APC_LEVEL are a caller bug, AML has to run at PASSIVE_LEVEL */
static
VOID
NTAPI
UacpiNtHostCheckWaitIrql(
    _In_z_ PCSTR Operation,
    _In_ uacpi_u16 Timeout)
{
    KIRQL Irql = KeGetCurrentIrql();

    if (Irql > APC_LEVEL)
    {
        DPRINT1("uACPI-NT: %s at IRQL %u (timeout %u), AML needs PASSIVE_LEVEL\n",
                Operation,
                (ULONG)Irql,
                (ULONG)Timeout);
    }
}

/* NULL means an infinite wait, a zero timeout polls */
static
PLARGE_INTEGER
NTAPI
UacpiNtHostWaitTimeout(
    _In_ uacpi_u16 Timeout,
    _Out_ PLARGE_INTEGER Storage)
{
    if (Timeout == UACPINT_HOST_WAIT_FOREVER)
        return NULL;

    Storage->QuadPart = -(LONGLONG)((ULONGLONG)Timeout * 10000ULL);
    return Storage;
}

/* AML mutexes are binary, non recursive and releasable from any thread */
uacpi_handle
uacpi_kernel_create_mutex(void)
{
    PKSEMAPHORE Semaphore;

    Semaphore = ExAllocatePoolWithTag(NonPagedPool, sizeof(*Semaphore), UACPINT_HOST_POOL_TAG);
    if (Semaphore)
        KeInitializeSemaphore(Semaphore, 1, 1);

    return Semaphore;
}

void
uacpi_kernel_free_mutex(
    _In_opt_ uacpi_handle Handle)
{
    if (Handle)
        ExFreePoolWithTag(Handle, UACPINT_HOST_POOL_TAG);
}

static
VOID
NTAPI
UacpiNtHostMutexSetOwner(
    _In_ uacpi_handle Handle,
    _In_opt_ PVOID Owner)
{
    ULONG Index;

    for (Index = 0; Index < UACPINT_HOST_MUTEX_OWNERS; Index++)
    {
        if (UacpiNtHostMutexOwners[Index].Handle == Handle)
        {
            UacpiNtHostMutexOwners[Index].Owner = Owner;
            return;
        }
    }

    /* A release of an untracked mutex has nothing to record */
    if (!Owner)
        return;

    for (Index = 0; Index < UACPINT_HOST_MUTEX_OWNERS; Index++)
    {
        if (!InterlockedCompareExchangePointer(&UacpiNtHostMutexOwners[Index].Handle, Handle, NULL))
        {
            UacpiNtHostMutexOwners[Index].Owner = Owner;
            return;
        }
    }
}

static
PVOID
NTAPI
UacpiNtHostMutexGetOwner(
    _In_ uacpi_handle Handle)
{
    ULONG Index;

    for (Index = 0; Index < UACPINT_HOST_MUTEX_OWNERS; Index++)
    {
        if (UacpiNtHostMutexOwners[Index].Handle == Handle)
            return UacpiNtHostMutexOwners[Index].Owner;
    }

    return NULL;
}

uacpi_status
uacpi_kernel_acquire_mutex(
    _In_ uacpi_handle Handle,
    _In_ uacpi_u16 Timeout)
{
    LARGE_INTEGER Interval;
    NTSTATUS Status;
    ULONG Waited = 0;

    UacpiNtHostCheckWaitIrql("acquire_mutex", Timeout);

    if (Timeout == UACPINT_HOST_WAIT_FOREVER)
    {
        /* Poll first, then wait in slices so a stuck wait gets reported */
        Interval.QuadPart = 0;
        Status = KeWaitForSingleObject(Handle, Executive, KernelMode, FALSE, &Interval);

        while (Status == STATUS_TIMEOUT)
        {
            Interval.QuadPart = -((LONGLONG)UACPINT_HOST_MUTEX_REPORT_SECONDS * 10 * 1000 * 1000);
            Status = KeWaitForSingleObject(Handle, Executive, KernelMode, FALSE, &Interval);
            if (Status != STATUS_TIMEOUT)
                break;

            Waited += UACPINT_HOST_MUTEX_REPORT_SECONDS;
            DPRINT1("uACPI-NT: mutex %p still not acquired after %lu s "
                    "(waiter %p, IRQL %u, owner %p)\n",
                    Handle,
                    Waited,
                    PsGetCurrentThread(),
                    (ULONG)KeGetCurrentIrql(),
                    UacpiNtHostMutexGetOwner(Handle));
        }
    }
    else
    {
        Status = KeWaitForSingleObject(Handle,
                                       Executive,
                                       KernelMode,
                                       FALSE,
                                       UacpiNtHostWaitTimeout(Timeout, &Interval));
    }

    if (Status == STATUS_SUCCESS)
    {
        UacpiNtHostMutexSetOwner(Handle, PsGetCurrentThread());
        return UACPI_STATUS_OK;
    }

    if (Status == STATUS_TIMEOUT)
        return UACPI_STATUS_TIMEOUT;

    return UACPI_STATUS_INTERNAL_ERROR;
}

void
uacpi_kernel_release_mutex(
    _In_ uacpi_handle Handle)
{
    UacpiNtHostMutexSetOwner(Handle, NULL);
    KeReleaseSemaphore(Handle, IO_NO_INCREMENT, 1, FALSE);
}

/* uACPI events are counting semaphores */
uacpi_handle
uacpi_kernel_create_event(void)
{
    PKSEMAPHORE Semaphore;

    Semaphore = ExAllocatePoolWithTag(NonPagedPool, sizeof(*Semaphore), UACPINT_HOST_POOL_TAG);
    if (Semaphore)
        KeInitializeSemaphore(Semaphore, 0, MAXLONG);

    return Semaphore;
}

void
uacpi_kernel_free_event(
    _In_opt_ uacpi_handle Handle)
{
    if (Handle)
        ExFreePoolWithTag(Handle, UACPINT_HOST_POOL_TAG);
}

uacpi_bool
uacpi_kernel_wait_for_event(
    _In_ uacpi_handle Handle,
    _In_ uacpi_u16 Timeout)
{
    LARGE_INTEGER Interval;
    NTSTATUS Status;

    UacpiNtHostCheckWaitIrql("wait_for_event", Timeout);

    Status = KeWaitForSingleObject(Handle,
                                   Executive,
                                   KernelMode,
                                   FALSE,
                                   UacpiNtHostWaitTimeout(Timeout, &Interval));

    return (Status == STATUS_SUCCESS) ? UACPI_TRUE : UACPI_FALSE;
}

void
uacpi_kernel_reset_event(
    _In_ uacpi_handle Handle)
{
    LARGE_INTEGER Interval;

    /* Consume the whole count with polling waits */
    Interval.QuadPart = 0;
    while (KeWaitForSingleObject(Handle, Executive, KernelMode, FALSE, &Interval) == STATUS_SUCCESS)
        NOTHING;
}

uacpi_thread_id
uacpi_kernel_get_thread_id(void)
{
    /* Unique per thread and never UACPI_THREAD_ID_NONE */
    return (uacpi_thread_id)KeGetCurrentThread();
}

uacpi_interrupt_state
uacpi_kernel_disable_interrupts(void)
{
    KIRQL OldIrql;

    KeRaiseIrql(HIGH_LEVEL, &OldIrql);
    return (uacpi_interrupt_state)OldIrql;
}

void
uacpi_kernel_restore_interrupts(
    _In_ uacpi_interrupt_state State)
{
    KeLowerIrql((KIRQL)State);
}

uacpi_handle
uacpi_kernel_create_spinlock(void)
{
    PKSPIN_LOCK SpinLock;

    SpinLock = ExAllocatePoolWithTag(NonPagedPool, sizeof(*SpinLock), UACPINT_HOST_POOL_TAG);
    if (SpinLock)
        KeInitializeSpinLock(SpinLock);

    return SpinLock;
}

void
uacpi_kernel_free_spinlock(
    _In_opt_ uacpi_handle Handle)
{
    if (Handle)
        ExFreePoolWithTag(Handle, UACPINT_HOST_POOL_TAG);
}

/* AML Breakpoint and Fatal are only logged, Fatal does not bugcheck */
uacpi_status
uacpi_kernel_handle_firmware_request(
    _In_ uacpi_firmware_request *Request)
{
    switch (Request->type)
    {
        case UACPI_FIRMWARE_REQUEST_TYPE_BREAKPOINT:
            DPRINT1("uACPI-NT: AML Breakpoint (context %p)\n", Request->breakpoint.ctx);
            break;

        case UACPI_FIRMWARE_REQUEST_TYPE_FATAL:
            DPRINT1("uACPI-NT: AML Fatal type 0x%X code 0x%X argument 0x%I64X\n",
                    Request->fatal.type,
                    Request->fatal.code,
                    Request->fatal.arg);
            break;

        default:
            break;
    }

    return UACPI_STATUS_OK;
}
