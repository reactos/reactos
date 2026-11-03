/*
 * PROJECT:     ReactOS ARM64 HAL
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Services for the ACPI driver and the HAL PnP device
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <hal.h>
#include "../../halx86/acpi/dispatch.h"
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

/* The GIC hands out interrupt IDs like an APIC system hands out vectors */
ULONG HalpInterruptControllerType = HALP_INTERRUPT_CONTROLLER_APIC;

/* Hardware ID of the HAL device itself */
PWCHAR HalHardwareIdString = L"arm64_up";

/* FUNCTIONS *****************************************************************/

/* Sleep states need the firmware, only the interrupt model is reported */
static
VOID
NTAPI
HalpAcpiReportSleepStates(
    _In_reads_(HAL_ACPI_SX_COUNT) PHAL_ACPI_SX_VALUES SxValues,
    _Out_ PULONG InterruptModel)
{
    UNREFERENCED_PARAMETER(SxValues);
    *InterruptModel = HalpInterruptControllerType;
}

static
ULONG
NTAPI
HalpAcpiPciConfigRead(
    _In_ PBUS_HANDLER RootBusHandler,
    _In_ ULONG BusNumber,
    _In_ PCI_SLOT_NUMBER SlotNumber,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ ULONG Offset,
    _In_ ULONG Length)
{
    UNREFERENCED_PARAMETER(RootBusHandler);
    return HalpPciRawConfig(BusNumber, SlotNumber, Buffer, Offset, Length, FALSE);
}

static
ULONG
NTAPI
HalpAcpiPciConfigWrite(
    _In_ PBUS_HANDLER RootBusHandler,
    _In_ ULONG BusNumber,
    _In_ PCI_SLOT_NUMBER SlotNumber,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Offset,
    _In_ ULONG Length)
{
    UNREFERENCED_PARAMETER(RootBusHandler);
    return HalpPciRawConfig(BusNumber, SlotNumber, Buffer, Offset, Length, TRUE);
}

static const HAL_ACPI_SERVICES HalpAcpiServices =
{
    HAL_ACPI_SERVICES_SIGNATURE,
    HAL_ACPI_SERVICES_VERSION,
    NULL,
    NULL,
    HalpAcpiReportSleepStates,
    NULL,
    NULL,
    NULL,
    HalpAcpiPciConfigRead,
    HalpAcpiPciConfigWrite,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

NTSTATUS
NTAPI
HaliInitPowerManagement(
    _In_ PPM_DISPATCH_TABLE PmDriverDispatchTable,
    _Out_ PPM_DISPATCH_TABLE *PmHalDispatchTable)
{
    UNREFERENCED_PARAMETER(PmDriverDispatchTable);

    PAGED_CODE();

    *PmHalDispatchTable = (PPM_DISPATCH_TABLE)&HalpAcpiServices;
    return STATUS_SUCCESS;
}

/**
 * @brief Builds the resources of the ACPI PDO.
 *
 * Hardware reduced ACPI has no SCI and the interrupt IDs need no grant, so
 * the list stays empty.
 */
NTSTATUS
NTAPI
HalpQueryAcpiResourceRequirements(
    _Out_ PIO_RESOURCE_REQUIREMENTS_LIST *Requirements)
{
    PIO_RESOURCE_REQUIREMENTS_LIST List;
    ULONG Size;

    PAGED_CODE();

    Size = FIELD_OFFSET(IO_RESOURCE_REQUIREMENTS_LIST, List[0].Descriptors);
    List = ExAllocatePoolZero(PagedPool, Size, TAG_HAL);
    if (!List)
        return STATUS_INSUFFICIENT_RESOURCES;

    List->ListSize = Size;
    List->InterfaceType = PNPBus;
    List->BusNumber = -1;
    List->AlternativeLists = 1;
    List->List[0].Version = 1;
    List->List[0].Revision = 1;

    *Requirements = List;
    return STATUS_SUCCESS;
}

/* Only a PIC HAL translates ISA lines */
NTSTATUS
NTAPI
HalpQueryPicLineTranslator(
    _Out_writes_bytes_(Size) PVOID Interface,
    _In_ ULONG Size,
    _Out_ PULONG Length)
{
    UNREFERENCED_PARAMETER(Interface);
    UNREFERENCED_PARAMETER(Size);
    UNREFERENCED_PARAMETER(Length);
    return STATUS_NOT_SUPPORTED;
}

/* EOF */
