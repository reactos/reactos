/*
 * PROJECT:     uACPI-NT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Application launch button interface (PNP0C32)
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include <uacpint.h>
#include <debug.h>

VOID
NTAPI
UacpiNtApplaunchStart(
    _Inout_ PUACPINT_PDO Pdo)
{
    if (_stricmp(Pdo->Hid, "PNP0C32") != 0)
        return;

    UacpiNtDevIfRegister(Pdo, &GUID_DEVICE_APPLICATIONLAUNCH_BUTTON, "applaunch");
}
