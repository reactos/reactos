/*
 * PROJECT:     uACPI-NT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     ACPI processor class device interface
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include <uacpint.h>
#include <debug.h>

/* Covers legacy Processor() objects and ACPI0007 devices */
VOID
NTAPI
UacpiNtProcessorStart(
    _Inout_ PUACPINT_PDO Pdo)
{
    BOOLEAN CanThrottle;

    if (!Pdo->IsProcessor && _stricmp(Pdo->Hid, "ACPI0007") != 0)
        return;

    UacpiNtDevIfRegister(Pdo, &GUID_DEVICE_PROCESSOR, "processor");

    /* Throttling or P-states make the processor a cooling device */
    CanThrottle = UacpiNtNodeHasChild(Pdo->Node, "_PTC") ||
                  UacpiNtNodeHasChild(Pdo->Node, "_TSS") ||
                  UacpiNtNodeHasChild(Pdo->Node, "_PSS");
    if (CanThrottle)
        UacpiNtCoolingIfRegister(Pdo);
}
