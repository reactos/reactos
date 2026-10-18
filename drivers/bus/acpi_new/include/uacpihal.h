/*
 * PROJECT:     uACPI-NT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     NT HAL integration header
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#pragma once

/* Set by the hal PM handshake. */
typedef enum _ACPI_NT_INTERRUPT_MODEL {
    ACPI_NT_PIC =  0,
    ACPI_NT_APIC = 1,
} ACPI_NT_INTERRUPT_MODEL;

/* Reads "IPCA" in memory */
#define ACPI_DRV_HAL_DISP_SIG 'ACPI'

#if (NTDDI_VERSION >= NTDDI_WIN8)
#define ACPI_DRIVER_HAL_VERSION 2
#else
#define ACPI_DRIVER_HAL_VERSION 1
#endif

typedef
VOID
(NTAPI *PACPI_HAL_INIT_SLEEP_STATES)(
    _In_ PVOID StateData,
    _Out_ PULONG OutInterruptModel);

typedef
ULONG
(NTAPI *PACPI_HAL_PCI_CONFIG)(
    _In_opt_ PVOID Context,
    _In_ ULONG Bus,
    _In_ ULONG Slot,
    _Inout_updates_bytes_(Length) PVOID Buffer,
    _In_ ULONG Offset,
    _In_ ULONG Length);

typedef struct _ACPI_DRIVER_HAL_DISPATCH
{
    ULONG Signature;
    ULONG Version; // 1 (Vista/Win7) or 2 (Win8+)
    PVOID EnableDisableGpeEvents;
    PVOID InitEnableAcpi;
    PVOID GpeEnableWakeEvents;
#if (NTDDI_VERSION >= NTDDI_WIN8)
    PVOID MarkHiberPhase;
#endif
} ACPI_DRIVER_HAL_DISPATCH;

#define ACPI_DRV_HAL_PM_SIG   'HAL '

#if (NTDDI_VERSION >= NTDDI_WIN8)
#define ACPI_HAL_PM_VERSION 4
#else
#define ACPI_HAL_PM_VERSION 3
#endif

typedef struct _ACPI_HAL_PM_DISPATCH
{
    ULONG Signature;            
    ULONG Version;
#if (NTDDI_VERSION >= NTDDI_WIN8)
    PVOID Reserved0;
#else
    PVOID PmTimerInitialize;
    PVOID PmTimerOverflow;
#endif
    PACPI_HAL_INIT_SLEEP_STATES InitializeSleepStates;
    PVOID QueryAcpiFlags;
    PVOID PicStateValid;
    PVOID RestorePicState;
    PACPI_HAL_PCI_CONFIG PciReadConfig;
    PACPI_HAL_PCI_CONFIG PciWriteConfig;
    PVOID QueryApicVersion;
    PVOID SetMaxLegacyPciBus;
    PVOID ValidateVector;
    PVOID GetTable;
    PVOID GetRsdp;
    PVOID GetFacsMapping;
    PVOID GetAllTables;
#if (NTDDI_VERSION >= NTDDI_WIN8)
    PVOID PmRegisterAvailable;
    PVOID PmRegisterRead;
    PVOID PmRegisterWrite;
#endif
} ACPI_HAL_PM_DISPATCH, *PACPI_HAL_PM_DISPATCH;

/* Handed to InitializeSleepStates, one entry per S1 to S5 */
typedef struct _ACPI_HAL_STATE_DATA
{
    UCHAR Supported;
    UCHAR SlpTypA;
    UCHAR SlpTypB;
} ACPI_HAL_STATE_DATA, *PACPI_HAL_STATE_DATA;

typedef
NTSTATUS
(NTAPI *PACPI_HAL_INIT_POWER_MANAGEMENT)(
    _In_ PVOID DriverTable,
    _Out_ PVOID *HalTable);

/* HalInitPowerManagement moved down a slot when Win8 dropped HalIoAssignDriveLetters */
#if (NTDDI_VERSION >= NTDDI_WIN8)
#define ACPI_HAL_DISPATCH_TABLE_VERSION     4
#define ACPI_HAL_DISPATCH_INIT_PM_SLOT      13
#else
#define ACPI_HAL_DISPATCH_TABLE_VERSION     3
#define ACPI_HAL_DISPATCH_INIT_PM_SLOT      14
#endif
