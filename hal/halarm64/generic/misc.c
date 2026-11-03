/*
 * PROJECT:     ReactOS ARM64 HAL
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Firmware, clock, display and bus services
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* PSCI 1.0 function IDs */
#define PSCI_SYSTEM_OFF     0x84000008
#define PSCI_SYSTEM_RESET   0x84000009

ULONG64
NTAPI
HalpCallFirmware(
    _In_ ULONG64 Function,
    _In_ ULONG64 Argument1,
    _In_ ULONG64 Argument2,
    _In_ ULONG64 Argument3,
    _In_ BOOLEAN UseHvc);

VOID
NTAPI
HalpWaitForInterrupt(VOID);

/* GLOBALS *******************************************************************/

/* Without CMOS the last known good flag only lives as long as the boot */
static BOOLEAN HalpLastKnownGood = TRUE;

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
HalReturnToFirmware(
    _In_ FIRMWARE_REENTRY Action)
{
    BOOLEAN UseHvc;
    ULONG64 Function;

    UseHvc = !!(HalpPlatform.ArmBootFlags & FADT_ARM_PSCI_USE_HVC);
    Function = ((Action == HalHaltRoutine) || (Action == HalPowerDownRoutine)) ?
               PSCI_SYSTEM_OFF : PSCI_SYSTEM_RESET;

    _disable();
    HalpCallFirmware(Function, 0, 0, 0, UseHvc);

    /* PSCI is missing or refused, park the processor */
    for (;;)
        HalpWaitForInterrupt();
}

/**
 * @brief Reports the time of day.
 *
 * ARM64 platforms have no architected RTC and the UEFI runtime services are
 * not mapped yet, so the clock starts from a fixed date.
 */
BOOLEAN
NTAPI
HalQueryRealTimeClock(
    _Out_ PTIME_FIELDS Time)
{
    RtlZeroMemory(Time, sizeof(*Time));
    Time->Year = 2026;
    Time->Month = 1;
    Time->Day = 1;
    Time->Weekday = 4;
    return TRUE;
}

BOOLEAN
NTAPI
HalSetRealTimeClock(
    _In_ PTIME_FIELDS Time)
{
    UNREFERENCED_PARAMETER(Time);
    return TRUE;
}

ARC_STATUS
NTAPI
HalGetEnvironmentVariable(
    _In_ PCH Name,
    _In_ USHORT ValueLength,
    _Out_writes_z_(ValueLength) PCH Value)
{
    if (_stricmp(Name, "LastKnownGood"))
        return ENOENT;

    strncpy(Value, HalpLastKnownGood ? "TRUE" : "FALSE", ValueLength);
    return ESUCCESS;
}

ARC_STATUS
NTAPI
HalSetEnvironmentVariable(
    _In_ PCH Name,
    _In_ PCH Value)
{
    if (_stricmp(Name, "LastKnownGood"))
        return ENOMEM;

    if (!_stricmp(Value, "TRUE"))
        HalpLastKnownGood = TRUE;
    else if (!_stricmp(Value, "FALSE"))
        HalpLastKnownGood = FALSE;
    else
        return ENOMEM;

    return ESUCCESS;
}

BOOLEAN
NTAPI
HalMakeBeep(
    _In_ ULONG Frequency)
{
    UNREFERENCED_PARAMETER(Frequency);
    return FALSE;
}

VOID
NTAPI
HalAcquireDisplayOwnership(
    _In_ PHAL_RESET_DISPLAY_PARAMETERS ResetDisplayParameters)
{
    UNREFERENCED_PARAMETER(ResetDisplayParameters);
}

VOID
NTAPI
HalDisplayString(
    _In_ PCSTR String)
{
    InbvDisplayString(String);
}

VOID
NTAPI
HalQueryDisplayParameters(
    _Out_ PULONG DispSizeX,
    _Out_ PULONG DispSizeY,
    _Out_ PULONG CursorPosX,
    _Out_ PULONG CursorPosY)
{
    UNREFERENCED_PARAMETER(DispSizeX);
    UNREFERENCED_PARAMETER(DispSizeY);
    UNREFERENCED_PARAMETER(CursorPosX);
    UNREFERENCED_PARAMETER(CursorPosY);
}

VOID
NTAPI
HalSetDisplayParameters(
    _In_ ULONG CursorPosX,
    _In_ ULONG CursorPosY)
{
    UNREFERENCED_PARAMETER(CursorPosX);
    UNREFERENCED_PARAMETER(CursorPosY);
}

/* Configuration space belongs to the PCI driver through ECAM, legacy access finds nothing */
ULONG
NTAPI
HalGetBusDataByOffset(
    _In_ BUS_DATA_TYPE BusDataType,
    _In_ ULONG BusNumber,
    _In_ ULONG SlotNumber,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ ULONG Offset,
    _In_ ULONG Length)
{
    UNREFERENCED_PARAMETER(BusNumber);
    UNREFERENCED_PARAMETER(SlotNumber);
    UNREFERENCED_PARAMETER(Offset);

    if ((BusDataType == PCIConfiguration) && (Length >= sizeof(USHORT)))
    {
        *(PUSHORT)Buffer = PCI_INVALID_VENDORID;
        return sizeof(USHORT);
    }

    return 0;
}

ULONG
NTAPI
HalGetBusData(
    _In_ BUS_DATA_TYPE BusDataType,
    _In_ ULONG BusNumber,
    _In_ ULONG SlotNumber,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length)
{
    return HalGetBusDataByOffset(BusDataType, BusNumber, SlotNumber, Buffer, 0, Length);
}

ULONG
NTAPI
HalSetBusDataByOffset(
    _In_ BUS_DATA_TYPE BusDataType,
    _In_ ULONG BusNumber,
    _In_ ULONG SlotNumber,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Offset,
    _In_ ULONG Length)
{
    UNREFERENCED_PARAMETER(BusDataType);
    UNREFERENCED_PARAMETER(BusNumber);
    UNREFERENCED_PARAMETER(SlotNumber);
    UNREFERENCED_PARAMETER(Buffer);
    UNREFERENCED_PARAMETER(Offset);
    UNREFERENCED_PARAMETER(Length);
    return 0;
}

ULONG
NTAPI
HalSetBusData(
    _In_ BUS_DATA_TYPE BusDataType,
    _In_ ULONG BusNumber,
    _In_ ULONG SlotNumber,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length)
{
    return HalSetBusDataByOffset(BusDataType, BusNumber, SlotNumber, Buffer, 0, Length);
}

/* Devices sit in the physical address space as is */
BOOLEAN
NTAPI
HalTranslateBusAddress(
    _In_ INTERFACE_TYPE InterfaceType,
    _In_ ULONG BusNumber,
    _In_ PHYSICAL_ADDRESS BusAddress,
    _Inout_ PULONG AddressSpace,
    _Out_ PPHYSICAL_ADDRESS TranslatedAddress)
{
    UNREFERENCED_PARAMETER(InterfaceType);
    UNREFERENCED_PARAMETER(BusNumber);

    *AddressSpace = 0;
    *TranslatedAddress = BusAddress;
    return TRUE;
}

NTSTATUS
NTAPI
HalAssignSlotResources(
    _In_ PUNICODE_STRING RegistryPath,
    _In_opt_ PUNICODE_STRING DriverClassName,
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ INTERFACE_TYPE BusType,
    _In_ ULONG BusNumber,
    _In_ ULONG SlotNumber,
    _Inout_ PCM_RESOURCE_LIST *AllocatedResources)
{
    UNREFERENCED_PARAMETER(RegistryPath);
    UNREFERENCED_PARAMETER(DriverClassName);
    UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(BusType);
    UNREFERENCED_PARAMETER(BusNumber);
    UNREFERENCED_PARAMETER(SlotNumber);
    UNREFERENCED_PARAMETER(AllocatedResources);
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
HalAdjustResourceList(
    _Inout_ PIO_RESOURCE_REQUIREMENTS_LIST *ResourceList)
{
    UNREFERENCED_PARAMETER(ResourceList);
    return STATUS_SUCCESS;
}
