/*
 * PROJECT:     uACPI-NT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     HAL power management handshake and interrupt model
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include <uacpint.h>
#include <uacpi/tables.h>
#include <uacpi/acpi.h>
#include <debug.h>

#ifndef ACPI_DRIVER_INTERNAL
#define ACPI_DRIVER_INTERNAL 0x000000A3
#endif

#ifndef ACPI_BIOS_ERROR
#define ACPI_BIOS_ERROR 0x000000A5
#endif

/* Widest GPE enable or status half we snapshot */
#define UACPINT_PM_REG_MAX_BYTES 32

typedef struct _UACPINT_PM_REG
{
    BOOLEAN Present;
    BOOLEAN Mmio;
    PUCHAR  Mapped;     ///< MMIO mapping when Mmio is set
    USHORT  Port;       ///< I/O port when Mmio is clear
    UCHAR   Length;     ///< width in bytes
} UACPINT_PM_REG, *PUACPINT_PM_REG;

typedef enum _UACPINT_PM_REG_INDEX
{
    UacpiNtPm1aStatus,
    UacpiNtPm1bStatus,
    UacpiNtPm1aEnable,
    UacpiNtPm1bEnable,
    UacpiNtPm1aControl,
    UacpiNtPm1bControl,
    UacpiNtGpe0Enable,
    UacpiNtGpe1Enable,
    UacpiNtGpe0Status,
    UacpiNtGpe1Status,
    UacpiNtSmiCommand,
    UacpiNtPmRegCount
} UACPINT_PM_REG_INDEX;

ULONG GlobalAcpiInterruptModel = ACPI_NT_PIC;

static ACPI_DRIVER_HAL_DISPATCH UacpiNtDriverHalTable;

/* HAL PCI config accessors, valid after the handshake */
static PACPI_HAL_PCI_CONFIG UacpiNtHalPciRead;
static PACPI_HAL_PCI_CONFIG UacpiNtHalPciWrite;

/*
 * The HAL sleep path calls back at HIGH_LEVEL with interrupts off, so the
 * callbacks touch the fixed registers directly and read a cached wake set.
 */
static UACPINT_PM_REG UacpiNtPmRegs[UacpiNtPmRegCount];
static UCHAR UacpiNtAcpiEnableValue;
static BOOLEAN UacpiNtPmRegsValid;

/* Armed wake set, captured at PASSIVE_LEVEL */
static UCHAR UacpiNtWakeGpe0[UACPINT_PM_REG_MAX_BYTES];
static UCHAR UacpiNtWakeGpe1[UACPINT_PM_REG_MAX_BYTES];
static USHORT UacpiNtWakePm1Enable;

/* Status latched on resume to identify the wake source */
static UCHAR UacpiNtWakeStatusGpe0[UACPINT_PM_REG_MAX_BYTES];
static UCHAR UacpiNtWakeStatusGpe1[UACPINT_PM_REG_MAX_BYTES];
static USHORT UacpiNtWakeStatusPm1;
static BOOLEAN UacpiNtWakeStatusValid;

static
PACPI_HAL_INIT_POWER_MANAGEMENT
NTAPI
UacpiNtHalFindInitPowerManagement(VOID)
{
    UNICODE_STRING RoutineName;
    PULONG_PTR DispatchTable;

    RtlInitUnicodeString(&RoutineName, L"HalDispatchTable");
    DispatchTable = MmGetSystemRoutineAddress(&RoutineName);
    if (!DispatchTable)
    {
        DPRINT1("uACPI-NT: HalDispatchTable is not exported\n");
        return NULL;
    }

    /* The first field is the table version */
    if (*(PULONG)DispatchTable != ACPI_HAL_DISPATCH_TABLE_VERSION)
    {
        DPRINT1("uACPI-NT: HalDispatchTable version %lu, expected %lu\n",
                *(PULONG)DispatchTable,
                (ULONG)ACPI_HAL_DISPATCH_TABLE_VERSION);
        return NULL;
    }

    return (PACPI_HAL_INIT_POWER_MANAGEMENT)DispatchTable[ACPI_HAL_DISPATCH_INIT_PM_SLOT];
}

static
VOID
NTAPI
UacpiNtPmRegDescribe(
    _Out_ PUACPINT_PM_REG Reg,
    _In_ ULONG64 Address,
    _In_ UCHAR AddressSpace,
    _In_ ULONG Offset,
    _In_ ULONG Length)
{
    PHYSICAL_ADDRESS PhysicalAddress;

    RtlZeroMemory(Reg, sizeof(*Reg));
    if (!Address || !Length)
        return;

    Reg->Length = (UCHAR)min(Length, UACPINT_PM_REG_MAX_BYTES);

    /* Anything but system memory is treated as port I/O */
    if (AddressSpace == UACPI_ADDRESS_SPACE_SYSTEM_MEMORY)
    {
        PhysicalAddress.QuadPart = (LONGLONG)(Address + Offset);
        Reg->Mapped = MmMapIoSpace(PhysicalAddress, Reg->Length, MmNonCached);
        if (!Reg->Mapped)
            return;

        Reg->Mmio = TRUE;
    }
    else
    {
        Reg->Port = (USHORT)(Address + Offset);
    }

    Reg->Present = TRUE;
}

/*
 * Describe one register from the FADT, preferring the extended GAS and
 * falling back to the legacy 32 bit port block.
 */
static
VOID
NTAPI
UacpiNtPmRegFromFadt(
    _Out_ PUACPINT_PM_REG Reg,
    _In_ const struct acpi_gas *Gas,
    _In_ ULONG LegacyPort,
    _In_ ULONG Offset,
    _In_ ULONG Length)
{
    if (Gas->address)
        UacpiNtPmRegDescribe(Reg, Gas->address, Gas->address_space_id, Offset, Length);
    else
        UacpiNtPmRegDescribe(Reg, LegacyPort, UACPI_ADDRESS_SPACE_SYSTEM_IO, Offset, Length);
}

static
VOID
NTAPI
UacpiNtPmRegReadBytes(
    _In_ const UACPINT_PM_REG *Reg,
    _Out_writes_bytes_(Reg->Length) PUCHAR Buffer)
{
    ULONG Index;

    for (Index = 0; Index < Reg->Length; Index++)
    {
        if (Reg->Mmio)
            Buffer[Index] = READ_REGISTER_UCHAR(Reg->Mapped + Index);
        else
            Buffer[Index] = READ_PORT_UCHAR((PUCHAR)(ULONG_PTR)(Reg->Port + Index));
    }
}

static
VOID
NTAPI
UacpiNtPmRegWriteBytes(
    _In_ const UACPINT_PM_REG *Reg,
    _In_reads_bytes_(Reg->Length) const UCHAR *Buffer)
{
    ULONG Index;

    for (Index = 0; Index < Reg->Length; Index++)
    {
        if (Reg->Mmio)
            WRITE_REGISTER_UCHAR(Reg->Mapped + Index, Buffer[Index]);
        else
            WRITE_PORT_UCHAR((PUCHAR)(ULONG_PTR)(Reg->Port + Index), Buffer[Index]);
    }
}

/* Little endian read of the low 4 bytes, an absent register reads as 0 */
static
ULONG
NTAPI
UacpiNtPmRegRead(
    _In_ UACPINT_PM_REG_INDEX Which)
{
    const UACPINT_PM_REG *Reg = &UacpiNtPmRegs[Which];
    UCHAR Buffer[UACPINT_PM_REG_MAX_BYTES] = { 0 };
    ULONG Value = 0;
    ULONG Index;

    if (!Reg->Present)
        return 0;

    UacpiNtPmRegReadBytes(Reg, Buffer);
    for (Index = 0; Index < min(Reg->Length, sizeof(Value)); Index++)
        Value |= (ULONG)Buffer[Index] << (Index * 8);

    return Value;
}

static
VOID
NTAPI
UacpiNtPmRegWrite(
    _In_ UACPINT_PM_REG_INDEX Which,
    _In_ ULONG Value)
{
    const UACPINT_PM_REG *Reg = &UacpiNtPmRegs[Which];
    UCHAR Buffer[UACPINT_PM_REG_MAX_BYTES] = { 0 };
    ULONG Index;

    if (!Reg->Present)
        return;

    for (Index = 0; Index < Reg->Length && Index < sizeof(Value); Index++)
        Buffer[Index] = (UCHAR)(Value >> (Index * 8));

    UacpiNtPmRegWriteBytes(Reg, Buffer);
}

/* Reads OR the a and b blocks, writes go to both */
static
ULONG
NTAPI
UacpiNtPm1Read(
    _In_ UACPINT_PM_REG_INDEX BlockA)
{
    return UacpiNtPmRegRead(BlockA) | UacpiNtPmRegRead(BlockA + 1);
}

static
VOID
NTAPI
UacpiNtPm1Write(
    _In_ UACPINT_PM_REG_INDEX BlockA,
    _In_ ULONG Value)
{
    UacpiNtPmRegWrite(BlockA, Value);
    UacpiNtPmRegWrite(BlockA + 1, Value);
}

static
VOID
NTAPI
UacpiNtRestoreWakeGpes(VOID)
{
    UacpiNtPmRegWriteBytes(&UacpiNtPmRegs[UacpiNtGpe0Enable], UacpiNtWakeGpe0);
    UacpiNtPmRegWriteBytes(&UacpiNtPmRegs[UacpiNtGpe1Enable], UacpiNtWakeGpe1);
}

/* Status bits are write one to clear */
static
VOID
NTAPI
UacpiNtClearGpeStatus(
    _In_ UACPINT_PM_REG_INDEX Which)
{
    UCHAR Status[UACPINT_PM_REG_MAX_BYTES] = { 0 };

    UacpiNtPmRegReadBytes(&UacpiNtPmRegs[Which], Status);
    UacpiNtPmRegWriteBytes(&UacpiNtPmRegs[Which], Status);
}

/*
 * Event and GPE blocks keep status in the low half and enable in the high
 * half. Runs once at handshake time.
 */
static
VOID
NTAPI
UacpiNtHalResolveSleepRegisters(VOID)
{
    struct acpi_fadt *Fadt = NULL;
    ULONG EventHalf;
    ULONG ControlLength;
    ULONG Gpe0Half;
    ULONG Gpe1Half;

    if (UacpiNtPmRegsValid)
        return;

    if (uacpi_unlikely_error(uacpi_table_fadt(&Fadt)) || !Fadt)
    {
        DPRINT1("uACPI-NT: no FADT, HAL sleep callbacks do nothing\n");
        return;
    }

    EventHalf = Fadt->pm1_evt_len / 2;
    ControlLength = Fadt->pm1_cnt_len;
    Gpe0Half = Fadt->gpe0_blk_len / 2;
    Gpe1Half = Fadt->gpe1_blk_len / 2;

    UacpiNtPmRegFromFadt(&UacpiNtPmRegs[UacpiNtPm1aStatus], &Fadt->x_pm1a_evt_blk,
                         Fadt->pm1a_evt_blk, 0, EventHalf);
    UacpiNtPmRegFromFadt(&UacpiNtPmRegs[UacpiNtPm1aEnable], &Fadt->x_pm1a_evt_blk,
                         Fadt->pm1a_evt_blk, EventHalf, EventHalf);
    UacpiNtPmRegFromFadt(&UacpiNtPmRegs[UacpiNtPm1bStatus], &Fadt->x_pm1b_evt_blk,
                         Fadt->pm1b_evt_blk, 0, EventHalf);
    UacpiNtPmRegFromFadt(&UacpiNtPmRegs[UacpiNtPm1bEnable], &Fadt->x_pm1b_evt_blk,
                         Fadt->pm1b_evt_blk, EventHalf, EventHalf);

    UacpiNtPmRegFromFadt(&UacpiNtPmRegs[UacpiNtPm1aControl], &Fadt->x_pm1a_cnt_blk,
                         Fadt->pm1a_cnt_blk, 0, ControlLength);
    UacpiNtPmRegFromFadt(&UacpiNtPmRegs[UacpiNtPm1bControl], &Fadt->x_pm1b_cnt_blk,
                         Fadt->pm1b_cnt_blk, 0, ControlLength);

    UacpiNtPmRegFromFadt(&UacpiNtPmRegs[UacpiNtGpe0Status], &Fadt->x_gpe0_blk,
                         Fadt->gpe0_blk, 0, Gpe0Half);
    UacpiNtPmRegFromFadt(&UacpiNtPmRegs[UacpiNtGpe0Enable], &Fadt->x_gpe0_blk,
                         Fadt->gpe0_blk, Gpe0Half, Gpe0Half);
    UacpiNtPmRegFromFadt(&UacpiNtPmRegs[UacpiNtGpe1Status], &Fadt->x_gpe1_blk,
                         Fadt->gpe1_blk, 0, Gpe1Half);
    UacpiNtPmRegFromFadt(&UacpiNtPmRegs[UacpiNtGpe1Enable], &Fadt->x_gpe1_blk,
                         Fadt->gpe1_blk, Gpe1Half, Gpe1Half);

    UacpiNtPmRegDescribe(&UacpiNtPmRegs[UacpiNtSmiCommand],
                         Fadt->smi_cmd,
                         UACPI_ADDRESS_SPACE_SYSTEM_IO,
                         0,
                         1);
    UacpiNtAcpiEnableValue = Fadt->acpi_enable;

    UacpiNtPmRegsValid = TRUE;
}

_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
UacpiNtHalRefreshWakeCache(VOID)
{
    if (!UacpiNtPmRegsValid)
        return;

    RtlZeroMemory(UacpiNtWakeGpe0, sizeof(UacpiNtWakeGpe0));
    RtlZeroMemory(UacpiNtWakeGpe1, sizeof(UacpiNtWakeGpe1));
    UacpiNtPmRegReadBytes(&UacpiNtPmRegs[UacpiNtGpe0Enable], UacpiNtWakeGpe0);
    UacpiNtPmRegReadBytes(&UacpiNtPmRegs[UacpiNtGpe1Enable], UacpiNtWakeGpe1);
    UacpiNtWakePm1Enable = (USHORT)UacpiNtPm1Read(UacpiNtPm1aEnable);
}

/* Zero means pre sleep without wake (S5), nonzero means resume */
static
NTSTATUS
NTAPI
UacpiNtHalEnableDisableGpeEvents(
    _In_ ULONG Enable)
{
    if (!UacpiNtPmRegsValid)
        return STATUS_SUCCESS;

    if (!Enable)
    {
        UacpiNtPmRegWrite(UacpiNtGpe0Enable, 0);
        UacpiNtPmRegWrite(UacpiNtGpe1Enable, 0);
        return STATUS_SUCCESS;
    }

    /* uACPI restores the runtime GPE set later from the S0 IRP */
    RtlZeroMemory(UacpiNtWakeStatusGpe0, sizeof(UacpiNtWakeStatusGpe0));
    RtlZeroMemory(UacpiNtWakeStatusGpe1, sizeof(UacpiNtWakeStatusGpe1));
    UacpiNtPmRegReadBytes(&UacpiNtPmRegs[UacpiNtGpe0Status], UacpiNtWakeStatusGpe0);
    UacpiNtPmRegReadBytes(&UacpiNtPmRegs[UacpiNtGpe1Status], UacpiNtWakeStatusGpe1);
    UacpiNtWakeStatusPm1 = (USHORT)UacpiNtPm1Read(UacpiNtPm1aStatus);
    UacpiNtWakeStatusValid = TRUE;

    UacpiNtRestoreWakeGpes();
    return STATUS_SUCCESS;
}

/* Re-enter ACPI mode after hibernate resume or an aborted sleep */
static
NTSTATUS
NTAPI
UacpiNtHalInitEnableAcpi(
    _In_ ULONG Flags)
{
    ULONG Spins = 0;

    if (!UacpiNtPmRegsValid)
        return STATUS_SUCCESS;

    /* Firmware came back in legacy mode, ask SMI_CMD for ACPI mode and wait ~2s */
    if (!(UacpiNtPm1Read(UacpiNtPm1aControl) & ACPI_PM1_CNT_SCI_EN_MASK) &&
        UacpiNtPmRegs[UacpiNtSmiCommand].Present)
    {
        UacpiNtPmRegWrite(UacpiNtSmiCommand, UacpiNtAcpiEnableValue);
        while (!(UacpiNtPm1Read(UacpiNtPm1aControl) & ACPI_PM1_CNT_SCI_EN_MASK))
        {
            if (++Spins > 200000)
                KeBugCheckEx(ACPI_BIOS_ERROR, 0x11, 6, 0, 0);

            KeStallExecutionProcessor(10);
        }
    }

    UacpiNtPm1Write(UacpiNtPm1aStatus, UacpiNtPm1Read(UacpiNtPm1aStatus));
    UacpiNtPm1Write(UacpiNtPm1aEnable, UacpiNtWakePm1Enable);

    if (Flags)
    {
        UacpiNtClearGpeStatus(UacpiNtGpe0Status);
        UacpiNtClearGpeStatus(UacpiNtGpe1Status);
        UacpiNtRestoreWakeGpes();
    }

    UacpiNtPm1Write(UacpiNtPm1aControl,
                    UacpiNtPm1Read(UacpiNtPm1aControl) &
                    ~(ULONG)(ACPI_PM1_CNT_SLP_EN_MASK | ACPI_PM1_CNT_BM_RLD_MASK));
    return STATUS_SUCCESS;
}

/* S1 to S4 pre sleep, the wake set was armed at PASSIVE_LEVEL already */
static
NTSTATUS
NTAPI
UacpiNtHalGpeEnableWakeEvents(VOID)
{
    if (UacpiNtPmRegsValid)
        UacpiNtRestoreWakeGpes();

    return STATUS_SUCCESS;
}

#if (NTDDI_VERSION >= NTDDI_WIN8)
/* Our globals are already part of the hibernate image */
static
NTSTATUS
NTAPI
UacpiNtHalMarkHiberPhase(VOID)
{
    return STATUS_SUCCESS;
}
#endif

/* Collect the \_S1 to \_S5 SLP_TYP values for MachineStateInit */
static
VOID
NTAPI
UacpiNtHalCollectSleepStates(
    _Out_writes_(5) PACPI_HAL_STATE_DATA StateData)
{
    CHAR Path[] = "_S0_";
    uacpi_object *Result;
    uacpi_object_array Package;
    uacpi_u64 Value;
    ULONG Index;

    RtlZeroMemory(StateData, 5 * sizeof(*StateData));

    for (Index = 0; Index < 5; Index++)
    {
        Path[2] = (CHAR)('1' + Index);
        Result = NULL;

        if (uacpi_unlikely_error(uacpi_eval(uacpi_namespace_root(), Path, NULL, &Result)) ||
            !Result)
        {
            continue;
        }

        if (uacpi_object_get_type(Result) == UACPI_OBJECT_PACKAGE &&
            uacpi_likely_success(uacpi_object_get_package(Result, &Package)))
        {
            StateData[Index].Supported = 1;

            if (Package.count >= 1 &&
                uacpi_likely_success(uacpi_object_get_integer(Package.objects[0], &Value)))
            {
                StateData[Index].SlpTypA = (UCHAR)Value;
            }

            if (Package.count >= 2 &&
                uacpi_likely_success(uacpi_object_get_integer(Package.objects[1], &Value)))
            {
                StateData[Index].SlpTypB = (UCHAR)Value;
            }
        }

        uacpi_object_unref(Result);
    }
}

NTSTATUS
NTAPI
UacpiNtHalPmHandshake(VOID)
{
    ACPI_HAL_STATE_DATA StateData[5];
    PACPI_HAL_INIT_POWER_MANAGEMENT InitPowerManagement;
    PACPI_HAL_PM_DISPATCH HalTable = NULL;
    ULONG Model = MAXULONG;
    uacpi_status UacpiStatus;
    NTSTATUS Status;

    PAGED_CODE();

    UacpiNtHalResolveSleepRegisters();
    UacpiNtHalCollectSleepStates(StateData);

    RtlZeroMemory(&UacpiNtDriverHalTable, sizeof(UacpiNtDriverHalTable));
    UacpiNtDriverHalTable.Signature = ACPI_DRV_HAL_DISP_SIG;
    UacpiNtDriverHalTable.Version = ACPI_DRIVER_HAL_VERSION;
    UacpiNtDriverHalTable.EnableDisableGpeEvents = (PVOID)(ULONG_PTR)UacpiNtHalEnableDisableGpeEvents;
    UacpiNtDriverHalTable.InitEnableAcpi = (PVOID)(ULONG_PTR)UacpiNtHalInitEnableAcpi;
    UacpiNtDriverHalTable.GpeEnableWakeEvents = (PVOID)(ULONG_PTR)UacpiNtHalGpeEnableWakeEvents;
#if (NTDDI_VERSION >= NTDDI_WIN8)
    UacpiNtDriverHalTable.MarkHiberPhase = (PVOID)(ULONG_PTR)UacpiNtHalMarkHiberPhase;
#endif

    /* There is no PIC fallback, a failed handshake is fatal */
    InitPowerManagement = UacpiNtHalFindInitPowerManagement();
    if (!InitPowerManagement)
    {
        DPRINT1("uACPI-NT: HalInitPowerManagement not found\n");
        KeBugCheckEx(ACPI_BIOS_ERROR, 0x11, 0, 0, 0);
    }

    Status = InitPowerManagement(&UacpiNtDriverHalTable, (PVOID *)&HalTable);
    if (!NT_SUCCESS(Status) || !HalTable)
    {
        DPRINT1("uACPI-NT: HalInitPowerManagement failed 0x%lx\n", Status);
        KeBugCheckEx(ACPI_BIOS_ERROR, 0x11, 1, (ULONG_PTR)Status, 0);
    }

    /* A version mismatch means this build targets a different HAL */
    if (HalTable->Signature != ACPI_DRV_HAL_PM_SIG ||
        HalTable->Version != ACPI_HAL_PM_VERSION ||
        !HalTable->MachineStateInit ||
        !HalTable->PciReadConfig ||
        !HalTable->PciWriteConfig)
    {
        DPRINT1("uACPI-NT: unexpected HAL PM table, signature 0x%08lx version %lu, expected %lu\n",
                HalTable->Signature,
                HalTable->Version,
                (ULONG)ACPI_HAL_PM_VERSION);
        KeBugCheckEx(ACPI_BIOS_ERROR,
                     0x11,
                     2,
                     (ULONG_PTR)HalTable->Signature,
                     HalTable->Version);
    }

    UacpiNtHalPciRead = HalTable->PciReadConfig;
    UacpiNtHalPciWrite = HalTable->PciWriteConfig;

    /* The HAL reports the interrupt model and takes over the sleep path */
    HalTable->MachineStateInit(StateData, &Model);

    DPRINT("uACPI-NT: HAL PM table version %lu, model %lu, S1-S5 {%u,%u,%u,%u,%u}\n",
           HalTable->Version,
           Model,
           StateData[0].Supported,
           StateData[1].Supported,
           StateData[2].Supported,
           StateData[3].Supported,
           StateData[4].Supported);

    /* An untouched sentinel means the slot or its arity is wrong */
    if (Model == MAXULONG)
    {
        DPRINT1("uACPI-NT: MachineStateInit did not report an interrupt model\n");
        KeBugCheckEx(ACPI_DRIVER_INTERNAL,
                     0x12,
                     (ULONG_PTR)HalTable->MachineStateInit,
                     HalTable->Version,
                     0);
    }

    if (Model != ACPI_NT_PIC && Model != ACPI_NT_APIC)
    {
        DPRINT1("uACPI-NT: interrupt model %lu is not supported\n", Model);
        KeBugCheckEx(ACPI_DRIVER_INTERNAL, 0x13, Model, 0, 0);
    }

    GlobalAcpiInterruptModel = Model;
    DPRINT("uACPI-NT: interrupt model %s\n", Model == ACPI_NT_APIC ? "APIC" : "PIC");

    /* A missing \_PIC is not an error */
    UacpiStatus = uacpi_set_interrupt_model(Model == ACPI_NT_APIC ?
                                            UACPI_INTERRUPT_MODEL_IOAPIC :
                                            UACPI_INTERRUPT_MODEL_PIC);
    if (uacpi_unlikely_error(UacpiStatus))
    {
        DPRINT1("uACPI-NT: \\_PIC(%lu) failed: %s\n",
                Model,
                uacpi_status_to_string(UacpiStatus));
    }

    return STATUS_SUCCESS;
}

/* Returns the byte count moved, zero before the handshake */
ULONG
NTAPI
UacpiNtHalPciReadConfig(
    _In_opt_ PVOID Context,
    _In_ ULONG Bus,
    _In_ ULONG Slot,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ ULONG Offset,
    _In_ ULONG Length)
{
    if (!UacpiNtHalPciRead)
        return 0;

    return UacpiNtHalPciRead(Context, Bus, Slot, Buffer, Offset, Length);
}

ULONG
NTAPI
UacpiNtHalPciWriteConfig(
    _In_opt_ PVOID Context,
    _In_ ULONG Bus,
    _In_ ULONG Slot,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Offset,
    _In_ ULONG Length)
{
    if (!UacpiNtHalPciWrite)
        return 0;

    return UacpiNtHalPciWrite(Context, Bus, Slot, Buffer, Offset, Length);
}
