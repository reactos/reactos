/*
 * PROJECT:     ReactOS Hardware Abstraction Layer
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Tables exchanged with the ACPI driver by HalInitPowerManagement
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#pragma once

/* Values the driver evaluated from \_S1 through \_S5, one entry per state */
typedef struct _HAL_ACPI_SX_VALUES
{
    BOOLEAN Present;
    UCHAR SlpTypA;
    UCHAR SlpTypB;
} HAL_ACPI_SX_VALUES, *PHAL_ACPI_SX_VALUES;

C_ASSERT(sizeof(HAL_ACPI_SX_VALUES) == 3);

#define HAL_ACPI_SX_COUNT               5

/* Services the HAL hands to the driver */
#define HAL_ACPI_SERVICES_SIGNATURE     'HAL '
#define HAL_ACPI_SERVICES_VERSION       3

typedef struct _HAL_ACPI_SERVICES
{
    ULONG Signature;
    ULONG Version;

    VOID (NTAPI *PmTimerConfigure)(
        _In_ ULONG TimerPort,
        _In_ BOOLEAN TimerValExt);

    VOID (NTAPI *PmTimerOverflow)(
        VOID);

    VOID (NTAPI *ReportSleepStates)(
        _In_reads_(HAL_ACPI_SX_COUNT) PHAL_ACPI_SX_VALUES SxValues,
        _Out_ PULONG InterruptModel);

    ULONG (NTAPI *CapabilityFlags)(
        VOID);

    BOOLEAN (NTAPI *InterruptControllerIntact)(
        VOID);

    VOID (NTAPI *InterruptControllerRestore)(
        VOID);

    ULONG (NTAPI *PciConfigRead)(
        _In_ PBUS_HANDLER RootBusHandler,
        _In_ ULONG BusNumber,
        _In_ PCI_SLOT_NUMBER SlotNumber,
        _Out_writes_bytes_(Length) PVOID Buffer,
        _In_ ULONG Offset,
        _In_ ULONG Length);

    ULONG (NTAPI *PciConfigWrite)(
        _In_ PBUS_HANDLER RootBusHandler,
        _In_ ULONG BusNumber,
        _In_ PCI_SLOT_NUMBER SlotNumber,
        _In_reads_bytes_(Length) PVOID Buffer,
        _In_ ULONG Offset,
        _In_ ULONG Length);

    ULONG (NTAPI *IoApicVersion)(
        _In_ ULONG InterruptBase);

    VOID (NTAPI *LegacyPciBusLimit)(
        _In_ ULONG BusNumber);

    BOOLEAN (NTAPI *InterruptInputValid)(
        _In_ ULONG Input);

    PVOID (NTAPI *FindTable)(
        _In_ ULONG Signature,
        _In_opt_ PCSTR OemId,
        _In_opt_ PCSTR OemTableId);

    PVOID (NTAPI *RootPointer)(
        VOID);

    PVOID (NTAPI *FacsTable)(
        VOID);

    PVOID (NTAPI *TableList)(
        VOID);
} HAL_ACPI_SERVICES, *PHAL_ACPI_SERVICES;

C_ASSERT(FIELD_OFFSET(HAL_ACPI_SERVICES, PmTimerConfigure) == 2 * sizeof(ULONG));
C_ASSERT(FIELD_OFFSET(HAL_ACPI_SERVICES, TableList) ==
         2 * sizeof(ULONG) + 14 * sizeof(PVOID));

/* Callbacks the driver hands to the HAL. Version 2 appends an entry this HAL does not use. */
#define ACPI_DRIVER_CALLBACKS_SIGNATURE 'ACPI'

typedef struct _ACPI_DRIVER_CALLBACKS
{
    ULONG Signature;
    ULONG Version;

    NTSTATUS (NTAPI *SciGpeControl)(
        _In_ ULONG Enable);

    NTSTATUS (NTAPI *AcpiModeEnable)(
        _In_ ULONG Flags);

    NTSTATUS (NTAPI *WakeGpeArm)(
        VOID);
} ACPI_DRIVER_CALLBACKS, *PACPI_DRIVER_CALLBACKS;

/* EOF */
