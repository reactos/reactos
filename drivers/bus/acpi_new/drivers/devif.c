/*
 * PROJECT:     uACPI-NT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Shared class device interface registration
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include <uacpint.h>
#include <debug.h>

static
BOOLEAN
NTAPI
UacpiNtEnableInterface(
    _In_ PUACPINT_PDO Pdo,
    _In_ const GUID *InterfaceGuid,
    _In_z_ const char *Label,
    _Out_ PUNICODE_STRING SymbolicLink)
{
    NTSTATUS Status;

    Status = IoRegisterDeviceInterface(Pdo->Shared.Self, InterfaceGuid, NULL, SymbolicLink);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("uACPI-NT: %s interface for %s failed 0x%lx\n", Label, Pdo->Name, Status);
        return FALSE;
    }

    IoSetDeviceInterfaceState(SymbolicLink, TRUE);
    DPRINT("uACPI-NT: %s interface up for %s\n", Label, Pdo->Name);
    return TRUE;
}

static
VOID
NTAPI
UacpiNtDisableInterface(
    _Inout_ PUNICODE_STRING SymbolicLink,
    _Inout_ PBOOLEAN Enabled)
{
    if (!*Enabled)
        return;

    IoSetDeviceInterfaceState(SymbolicLink, FALSE);
    RtlFreeUnicodeString(SymbolicLink);
    *Enabled = FALSE;
}

/* The class interface lives in ClassIfLink so remove can tear it down */
VOID
NTAPI
UacpiNtDevIfRegister(
    _Inout_ PUACPINT_PDO Pdo,
    _In_ const GUID *InterfaceGuid,
    _In_z_ const char *Label)
{
    if (Pdo->ClassIfOn)
        return;

    Pdo->ClassIfOn = UacpiNtEnableInterface(Pdo, InterfaceGuid, Label, &Pdo->ClassIfLink);
}

/* Kept apart from ClassIfLink so one PDO can expose both */
VOID
NTAPI
UacpiNtCoolingIfRegister(
    _Inout_ PUACPINT_PDO Pdo)
{
    if (Pdo->CoolingIfOn)
        return;

    Pdo->CoolingIfOn = UacpiNtEnableInterface(Pdo,
                                              &GUID_DEVINTERFACE_THERMAL_COOLING,
                                              "cooling",
                                              &Pdo->CoolingIfLink);
}

VOID
NTAPI
UacpiNtDevIfRemove(
    _Inout_ PUACPINT_PDO Pdo)
{
    UacpiNtDisableInterface(&Pdo->ClassIfLink, &Pdo->ClassIfOn);
    UacpiNtDisableInterface(&Pdo->CoolingIfLink, &Pdo->CoolingIfOn);
}

/* Called from pdo.c, every start routine skips devices it does not own */
VOID
NTAPI
UacpiNtDriversStartDevice(
    _Inout_ PUACPINT_PDO Pdo)
{
    UacpiNtButtonStart(Pdo);
    UacpiNtProcessorStart(Pdo);
    UacpiNtThermalStart(Pdo);
    UacpiNtFanStart(Pdo);
    UacpiNtApplaunchStart(Pdo);
    UacpiNtEcStart(Pdo);
}

VOID
NTAPI
UacpiNtDriversRemoveDevice(
    _Inout_ PUACPINT_PDO Pdo)
{
    UacpiNtButtonRemove(Pdo);
    UacpiNtThermalRemove(Pdo);
    UacpiNtDevIfRemove(Pdo);
    UacpiNtEcRemove(Pdo);
}
