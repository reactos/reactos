/*
 * PROJECT:     uACPI-NT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     uACPI interpreter bring up and teardown
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include <uacpint.h>
#include <uacpi/notify.h>
#include <uacpi/tables.h>
#include <uacpi/registers.h>
#include <uacpi/acpi.h>
#include <debug.h>

/* Runs at DISPATCH_LEVEL from the SCI DPC or at PASSIVE_LEVEL from a work item */
static
uacpi_status
UacpiNtGlobalNotify(
    _In_opt_ uacpi_handle Context,
    _In_ uacpi_namespace_node *Node,
    _In_ uacpi_u64 Value)
{
    uacpi_object_name Name = uacpi_namespace_node_name(Node);

    UNREFERENCED_PARAMETER(Context);

    DPRINT("uACPI-NT: Notify(%.4s, 0x%I64X)\n", Name.text, Value);
    UacpiNtRouteNotify(Node, (ULONG)Value);
    return UACPI_STATUS_OK;
}

static
VOID
NTAPI
UacpiNtInstallEventHandlers(
    _In_ PUACPINT_FDO Fdo)
{
    uacpi_status UacpiStatus;

    /* A handler on the root sees notifies for every node */
    UacpiStatus = uacpi_install_notify_handler(uacpi_namespace_root(), UacpiNtGlobalNotify, NULL);
    if (uacpi_unlikely_error(UacpiStatus))
        DPRINT1("uACPI-NT: global Notify handler failed: %s\n", uacpi_status_to_string(UacpiStatus));
    else
        DPRINT("uACPI-NT: global Notify handler installed\n");

    /* The resource hub must be bound before enumeration, failure is not fatal */
    UacpiNtConnectResourceHub();

    UacpiNtFixedButtonInit(Fdo);
}

/* Status bits of the fixed events, written back to clear them */
#define UACPINT_PM1_STATUS_BITS (ACPI_PM1_STS_TMR_STS_MASK |                                         ACPI_PM1_STS_BM_STS_MASK |                                          ACPI_PM1_STS_GBL_STS_MASK |                                         ACPI_PM1_STS_PWRBTN_STS_MASK |                                      ACPI_PM1_STS_SLPBTN_STS_MASK |                                      ACPI_PM1_STS_RTC_STS_MASK |                                         ACPI_PM1_STS_PCIEXP_WAKE_STS_MASK |                                  ACPI_PM1_STS_WAKE_STS_MASK)

/* Clears one FADT GPE block, half of the block is status and each byte covers 8 GPEs */
static
VOID
NTAPI
UacpiNtClearGpeBlock(
    _In_ BOOLEAN Present,
    _In_ UCHAR BlockLength,
    _In_ ULONG Base)
{
    ULONG Count;
    ULONG Index;

    if (!Present || BlockLength < 2)
        return;

    Count = (BlockLength / 2) * 8;
    for (Index = 0; Index < Count; Index++)
        uacpi_clear_gpe(NULL, (uacpi_u16)(Base + Index));
}

uacpi_status
NTAPI
UacpiNtClearAllEvents(VOID)
{
    struct acpi_fadt *Fadt;
    uacpi_status UacpiStatus;

    UacpiStatus = uacpi_table_fadt(&Fadt);
    if (uacpi_unlikely_error(UacpiStatus) || !Fadt)
        return UacpiStatus;

    /* Hardware reduced platforms have no fixed events or FADT GPE blocks */
    if (Fadt->flags & ACPI_HW_REDUCED_ACPI)
        return UACPI_STATUS_OK;

    UacpiStatus = uacpi_write_register(UACPI_REGISTER_PM1_STS, UACPINT_PM1_STATUS_BITS);
    if (uacpi_unlikely_error(UacpiStatus))
        return UacpiStatus;

    UacpiNtClearGpeBlock(Fadt->x_gpe0_blk.address || Fadt->gpe0_blk, Fadt->gpe0_blk_len, 0);
    UacpiNtClearGpeBlock(Fadt->x_gpe1_blk.address || Fadt->gpe1_blk, Fadt->gpe1_blk_len, Fadt->gpe1_base);
    return UACPI_STATUS_OK;
}

/* Masks every GPE and clears pending events so a level SCI cannot storm */
static
VOID
NTAPI
UacpiNtQuiesceEvents(VOID)
{
    uacpi_status UacpiStatus;

    UacpiStatus = uacpi_disable_all_gpes();
    if (uacpi_unlikely_error(UacpiStatus))
        DPRINT1("uACPI-NT: disabling GPEs failed: %s\n", uacpi_status_to_string(UacpiStatus));

    UacpiStatus = UacpiNtClearAllEvents();
    if (uacpi_unlikely_error(UacpiStatus))
        DPRINT1("uACPI-NT: clearing events failed: %s\n", uacpi_status_to_string(UacpiStatus));
}

NTSTATUS
NTAPI
UacpiNtBringUpInterpreter(
    _In_ PUACPINT_FDO Fdo)
{
    uacpi_status UacpiStatus;
    NTSTATUS Status;

    PAGED_CODE();

    /* Enter ACPI mode and honor _OSI */
    UacpiStatus = uacpi_initialize(0);
    if (uacpi_unlikely_error(UacpiStatus))
    {
        DPRINT1("uACPI-NT: uacpi_initialize failed: %s\n", uacpi_status_to_string(UacpiStatus));
        return STATUS_UNSUCCESSFUL;
    }

    /* Loads the DSDT and SSDTs, the SCI is recorded here */
    UacpiStatus = uacpi_namespace_load();
    if (uacpi_unlikely_error(UacpiStatus))
    {
        DPRINT1("uACPI-NT: uacpi_namespace_load failed: %s\n", uacpi_status_to_string(UacpiStatus));
        return STATUS_UNSUCCESSFUL;
    }

    UacpiNtHalPmHandshake();
    UacpiNtIrqLibInitialize();
    UacpiNtQuiesceEvents();

    /* The SCI needs IrqLib and has to be up before any GPE is enabled */
    Status = UacpiNtHostConnectSci();
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("uACPI-NT: SCI connect failed 0x%lx\n", Status);
        return Status;
    }

    /* _INI methods often read the EC */
    UacpiNtEcInitialize();

    UacpiStatus = uacpi_namespace_initialize();
    if (uacpi_unlikely_error(UacpiStatus))
    {
        DPRINT1("uACPI-NT: uacpi_namespace_initialize failed: %s\n", uacpi_status_to_string(UacpiStatus));
        return STATUS_UNSUCCESSFUL;
    }

    UacpiNtPlatformOscNegotiate();
    UacpiNtPowerResInit();

    /* Handlers go in first, a GPE firing during install can deadlock */
    UacpiNtInstallEventHandlers(Fdo);

    /* Wake GPEs are marked per device on WAIT_WAKE, not here */
    UacpiStatus = uacpi_finalize_gpe_initialization();
    if (uacpi_unlikely_error(UacpiStatus))
    {
        DPRINT1("uACPI-NT: uacpi_finalize_gpe_initialization failed: %s\n",
                uacpi_status_to_string(UacpiStatus));
    }

    Fdo->InterpreterReady = TRUE;
    DPRINT("uACPI-NT: interpreter up\n");
    return STATUS_SUCCESS;
}

/* uACPI cannot undo uacpi_initialize, only mark it down */
VOID
NTAPI
UacpiNtTearDownInterpreter(
    _In_ PUACPINT_FDO Fdo)
{
    UacpiNtMsiDiagDisarm();
    Fdo->InterpreterReady = FALSE;
}
