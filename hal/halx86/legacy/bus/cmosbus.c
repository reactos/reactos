/*
 * PROJECT:     ReactOS HAL
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     CMOS bus data handlers
 * COPYRIGHT:   Copyright 2023 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <hal.h>

/* PRIVATE FUNCTIONS **********************************************************/

ULONG
NTAPI
HalpcGetCmosData(
    _In_ PBUS_HANDLER BusHandler,
    _In_ PBUS_HANDLER RootHandler,
    _In_ ULONG SlotNumber,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ ULONG Offset,
    _In_ ULONG Length)
{
    UNREFERENCED_PARAMETER(RootHandler);

    /* CMOS reads do not support offsets */
    if (Offset != 0)
        return 0;

    return HalpGetCmosData(BusHandler->BusNumber,
                           SlotNumber,
                           Buffer,
                           Length);
}

ULONG
NTAPI
HalpcSetCmosData(
    _In_ PBUS_HANDLER BusHandler,
    _In_ PBUS_HANDLER RootHandler,
    _In_ ULONG SlotNumber,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Offset,
    _In_ ULONG Length)
{
    UNREFERENCED_PARAMETER(RootHandler);

    /* CMOS writes do not support offsets */
    if (Offset != 0)
        return 0;

    return HalpSetCmosData(BusHandler->BusNumber,
                           SlotNumber,
                           Buffer,
                           Length);
}

/* EOF */
