/*
 * PROJECT:     uACPI-NT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     ACPI fan class device interface
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include <uacpint.h>
#include <debug.h>

VOID
NTAPI
UacpiNtFanStart(
    _Inout_ PUACPINT_PDO Pdo)
{
    if (_stricmp(Pdo->Hid, "PNP0C0B") != 0)
        return;

    UacpiNtDevIfRegister(Pdo, &GUID_DEVICE_FAN, "fan");

    /* A fan is a cooling device as well */
    UacpiNtCoolingIfRegister(Pdo);
}
