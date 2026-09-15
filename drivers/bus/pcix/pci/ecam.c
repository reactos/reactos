/*
 * PROJECT:     ReactOS PCI Bus Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Basic ECAM Handling
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <pci.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

#define PCI_ECAM_BUS_COUNT  (PCI_MAX_BRIDGE_NUMBER + 1)
#define PCI_ECAM_BUS_SIZE   (PCI_MAX_DEVICES * PCI_MAX_FUNCTION * PCI_EXTENDED_CONFIG_LENGTH)

BOOLEAN PciEcamVerified = FALSE;

static BOOLEAN PciEcamWindowQueried = FALSE;
static BOOLEAN PciEcamSkipK8Northbridge = FALSE;
static ULONGLONG PciEcamWindowStart;
static ULONG PciEcamWindowBuses;
static PUCHAR PciEcamBusView[PCI_ECAM_BUS_COUNT];

/* FUNCTIONS ******************************************************************/

/* Takes the first window, which is segment 0 since the HAL writes them in segment order */
static
BOOLEAN
NTAPI
PciEcamParseWindow(
    _In_ PKEY_VALUE_PARTIAL_INFORMATION Value)
{
    PIO_RESOURCE_REQUIREMENTS_LIST Requirements;
    PIO_RESOURCE_DESCRIPTOR Window;
    ULONGLONG Start, End, Buses;
    PAGED_CODE();

    if ((Value->Type != REG_RESOURCE_REQUIREMENTS_LIST) ||
        (Value->DataLength < sizeof(IO_RESOURCE_REQUIREMENTS_LIST)))
    {
        return FALSE;
    }

    Requirements = (PIO_RESOURCE_REQUIREMENTS_LIST)Value->Data;
    if (!(Requirements->AlternativeLists) || !(Requirements->List[0].Count)) return FALSE;

    Window = &Requirements->List[0].Descriptors[0];
    if ((Window->Type != CmResourceTypeMemory) &&
        (Window->Type != CmResourceTypeMemoryLarge))
    {
        return FALSE;
    }

    Start = (ULONGLONG)Window->u.Memory.MinimumAddress.QuadPart;
    End = (ULONGLONG)Window->u.Memory.MaximumAddress.QuadPart;
    if (End < Start) return FALSE;

    Buses = (End - Start + 1) / PCI_ECAM_BUS_SIZE;
    if (!(Buses) || (Buses > PCI_ECAM_BUS_COUNT)) return FALSE;

    PciEcamWindowStart = Start;
    PciEcamWindowBuses = (ULONG)Buses;
    return TRUE;
}

/* The HAL publishes the MCFG windows here so the arbiters keep them reserved */
static
NTSTATUS
NTAPI
PciEcamQueryWindow(VOID)
{
    PKEY_VALUE_PARTIAL_INFORMATION Value;
    UNICODE_STRING ValueName;
    HANDLE KeyHandle;
    NTSTATUS Status;
    ULONG Size;
    PAGED_CODE();

    if (!PciOpenKey(L"\\Registry\\Machine\\System\\CurrentControlSet"
                    L"\\Control\\Arbiters\\ReservedResources",
                    NULL,
                    KEY_QUERY_VALUE,
                    &KeyHandle,
                    &Status))
    {
        return Status;
    }

    RtlInitUnicodeString(&ValueName, L"MmConfigRange");
    Status = ZwQueryValueKey(KeyHandle,
                             &ValueName,
                             KeyValuePartialInformation,
                             NULL,
                             0,
                             &Size);
    if ((Status != STATUS_BUFFER_TOO_SMALL) && (Status != STATUS_BUFFER_OVERFLOW))
    {
        ZwClose(KeyHandle);
        return NT_SUCCESS(Status) ? STATUS_UNSUCCESSFUL : Status;
    }

    Value = ExAllocatePoolWithTag(PagedPool, Size, PCI_POOL_TAG);
    if (!Value)
    {
        ZwClose(KeyHandle);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = ZwQueryValueKey(KeyHandle,
                             &ValueName,
                             KeyValuePartialInformation,
                             Value,
                             Size,
                             &Size);
    ZwClose(KeyHandle);

    if ((NT_SUCCESS(Status)) && !(PciEcamParseWindow(Value)))
    {
        Status = STATUS_UNSUCCESSFUL;
    }

    ExFreePoolWithTag(Value, 0);
    return Status;
}

#if defined(_M_IX86) || defined(_M_AMD64)
/*
 * K8 northbridge functions on bus 0 are not decoded by the chipset ECAM window.
 * This really should be a hardware quirk so I'm marking this as TODO:
 * However I want to be able to boot until we got a solid system for this!
 */
static
BOOLEAN
NTAPI
PciEcamIsAmdK8(VOID)
{
    INT CpuInfo[4];

    /* "AuthenticAMD" */
    __cpuid(CpuInfo, 0);
    if ((CpuInfo[1] != 0x68747541) ||
        (CpuInfo[3] != 0x69746E65) ||
        (CpuInfo[2] != 0x444D4163))
    {
        return FALSE;
    }

    /* Family 0Fh with no extended family */
    __cpuid(CpuInfo, 1);
    return ((CpuInfo[0] & 0x0FF00F00) == 0x00000F00);
}
#endif

/* Mappings are kept for the life of the driver since config access can happen at any IRQL */
static
PUCHAR
NTAPI
PciEcamMapBus(
    _In_ ULONG Bus)
{
    PHYSICAL_ADDRESS Address;
    PAGED_CODE();

    if (Bus >= PciEcamWindowBuses) return NULL;
    if (PciEcamBusView[Bus]) return PciEcamBusView[Bus];

    Address.QuadPart = (LONGLONG)(PciEcamWindowStart + ((ULONGLONG)Bus * PCI_ECAM_BUS_SIZE));
    PciEcamBusView[Bus] = MmMapIoSpace(Address, PCI_ECAM_BUS_SIZE, MmNonCached);
    if (!PciEcamBusView[Bus])
    {
        DPRINT1("PCI: Unable to map ECAM for bus 0x%lx\n", Bus);
    }

    return PciEcamBusView[Bus];
}

static
VOID
NTAPI
PciEcamDiscardWindow(VOID)
{
    ULONG Bus;
    PAGED_CODE();

    for (Bus = 0; Bus < PciEcamWindowBuses; Bus++)
    {
        if (!PciEcamBusView[Bus]) continue;

        MmUnmapIoSpace(PciEcamBusView[Bus], PCI_ECAM_BUS_SIZE);
        PciEcamBusView[Bus] = NULL;
    }

    PciEcamWindowBuses = 0;
}

static
PUCHAR
NTAPI
PciEcamFunctionBase(
    _In_ ULONG Bus,
    _In_ PCI_SLOT_NUMBER Slot)
{
    ULONG Index;

    if ((Bus >= PciEcamWindowBuses) || !(PciEcamBusView[Bus])) return NULL;

    if ((PciEcamSkipK8Northbridge) &&
        (Bus == 0) &&
        (Slot.u.bits.DeviceNumber >= 0x18))
    {
        return NULL;
    }

    Index = (Slot.u.bits.DeviceNumber * PCI_MAX_FUNCTION) + Slot.u.bits.FunctionNumber;
    return PciEcamBusView[Bus] + (Index * PCI_EXTENDED_CONFIG_LENGTH);
}

/* Picks the same access widths as the HAL does for the legacy mechanism */
static
VOID
NTAPI
PciEcamTransfer(
    _In_ PUCHAR Function,
    _Inout_updates_bytes_(Length) PUCHAR Buffer,
    _In_ ULONG Offset,
    _In_ ULONG Length,
    _In_ BOOLEAN Read)
{
    PUCHAR Register;
    ULONG Width;

    while (Length)
    {
        Register = Function + Offset;

        if ((Offset & 1) || ((Length & 3) == 1))
        {
            Width = sizeof(UCHAR);
            if (Read)
            {
                *Buffer = READ_REGISTER_UCHAR(Register);
            }
            else
            {
                WRITE_REGISTER_UCHAR(Register, *Buffer);
            }
        }
        else if (!(Offset & 3) && !(Length & 3))
        {
            Width = sizeof(ULONG);
            if (Read)
            {
                *(PULONG)Buffer = READ_REGISTER_ULONG((PULONG)Register);
            }
            else
            {
                WRITE_REGISTER_ULONG((PULONG)Register, *(PULONG)Buffer);
            }
        }
        else
        {
            Width = sizeof(USHORT);
            if (Read)
            {
                *(PUSHORT)Buffer = READ_REGISTER_USHORT((PUSHORT)Register);
            }
            else
            {
                WRITE_REGISTER_USHORT((PUSHORT)Register, *(PUSHORT)Buffer);
            }
        }

        Buffer += Width;
        Offset += Width;
        Length -= Width;
    }
}

/* The window is assumed to start at bus 0, so compare IDs through both mechanisms to prove it */
static
NTSTATUS
NTAPI
PciEcamVerifyBus(
    _In_ PPCI_FDO_EXTENSION FdoExtension)
{
    PCI_SLOT_NUMBER Slot;
    PUCHAR Function;
    ULONG Device, LegacyIds, EcamIds, Matched;
    PAGED_CODE();

    Slot.u.AsULONG = 0;
    Matched = 0;

    for (Device = 0; Device < PCI_MAX_DEVICES; Device++)
    {
        Slot.u.bits.DeviceNumber = Device;

        Function = PciEcamFunctionBase(FdoExtension->BaseBus, Slot);
        if (!Function) continue;

        LegacyIds = MAXULONG;
        PciReadSlotConfig(FdoExtension, Slot, &LegacyIds, 0, sizeof(ULONG));
        if (((USHORT)LegacyIds == PCI_INVALID_VENDORID) || !((USHORT)LegacyIds)) continue;

        EcamIds = READ_REGISTER_ULONG((PULONG)Function);
        if (EcamIds != LegacyIds)
        {
            DPRINT1("PCI: ECAM mismatch at %02lx:%02lx.0, read %08lx expected %08lx\n",
                    FdoExtension->BaseBus,
                    Device,
                    EcamIds,
                    LegacyIds);
            return STATUS_DEVICE_CONFIGURATION_ERROR;
        }

        Matched++;
    }

    return Matched ? STATUS_SUCCESS : STATUS_NO_SUCH_DEVICE;
}

/**
 * @brief
 * Maps the bus an FDO decodes into the ECAM window and verifies the
 * window the first time a bus with devices on it is seen.
 *
 * @param[in] FdoExtension
 * The FDO extension of the bus being started.
 */
VOID
NTAPI
PciInitializeEcam(
    _In_ PPCI_FDO_EXTENSION FdoExtension)
{
    NTSTATUS Status;
    PAGED_CODE();

    if (!PciEcamWindowQueried)
    {
        PciEcamWindowQueried = TRUE;

        Status = PciEcamQueryWindow();
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("PCI: No ECAM window available (0x%lx)\n", Status);
            return;
        }

#if defined(_M_IX86) || defined(_M_AMD64)
        PciEcamSkipK8Northbridge = PciEcamIsAmdK8();
#endif
        DPRINT1("PCI: ECAM window at 0x%I64x for 0x%lx buses\n",
                PciEcamWindowStart,
                PciEcamWindowBuses);
    }

    if (!PciEcamMapBus(FdoExtension->BaseBus) || PciEcamVerified) return;

    /* An empty bus proves nothing, so leave it to the next one */
    Status = PciEcamVerifyBus(FdoExtension);
    if (Status == STATUS_DEVICE_CONFIGURATION_ERROR)
    {
        PciEcamDiscardWindow();
    }
    else if (NT_SUCCESS(Status))
    {
        PciEcamVerified = TRUE;
        DPRINT1("PCI: ECAM verified on bus 0x%lx\n", FdoExtension->BaseBus);
    }
}

/**
 * @brief Unmaps every bus mapped through the ECAM window.
 */
VOID
NTAPI
PciReleaseEcam(VOID)
{
    PAGED_CODE();

    /* Config access falls back to the legacy mechanism before the mappings go away */
    PciEcamVerified = FALSE;
    PciEcamDiscardWindow();
    PciEcamWindowQueried = FALSE;
}

/**
 * @brief
 * Performs a configuration space access through the ECAM window.
 *
 * @return
 * FALSE if the window cannot serve the access, in which case the
 * caller falls back to the legacy mechanism.
 */
BOOLEAN
NTAPI
PciEcamReadWriteConfig(
    _In_ ULONG Bus,
    _In_ PCI_SLOT_NUMBER Slot,
    _Inout_updates_bytes_(Length) PVOID Buffer,
    _In_ ULONG Offset,
    _In_ ULONG Length,
    _In_ BOOLEAN Read)
{
    PUCHAR Function;

    if (!PciEcamVerified) return FALSE;

    if ((Offset >= PCI_EXTENDED_CONFIG_LENGTH) ||
        (Length > (PCI_EXTENDED_CONFIG_LENGTH - Offset)))
    {
        return FALSE;
    }

    Function = PciEcamFunctionBase(Bus, Slot);
    if (!Function) return FALSE;

    PciEcamTransfer(Function, Buffer, Offset, Length, Read);
    return TRUE;
}

/* EOF */
