/*
 * PROJECT:         ReactOS HDAudio Driver
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Vendor-specific quirks
 * COPYRIGHT:       Copyright 2026 Oleg Dubinskiy <oleg.dubinskiy@reactos.org>
 */

#include "private.h"

#define NDEBUG
#include <debug.h>

static
NTSTATUS
StacGPIOSetup(
    PVOID Node,
    ULONG GPIOData,
    ULONG GPIOEnableMask,
    ULONG GPIODirection)
{
    CFunctionGroupNode *OutNode = (CFunctionGroupNode *)Node;
    ULONG Data, EnableMask, Direction;
    NTSTATUS Status;

    Status = OutNode->GetGPIO(0x01, &Data, &EnableMask, &Direction);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: GetGPIO failed with status %x Node %u\n", Status, 0x01);
        return Status;
    }

    Data = (Data & ~GPIODirection) | (GPIOData & GPIODirection);
    EnableMask |= GPIOEnableMask;
    Direction |= GPIODirection;

    // Configure GPIOx as CMOS
    Status = OutNode->SetCustomVerb(0x01, 0x7e7, 0x0);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: SetCustomVerb failed with %x Node %u\n", Status, 0x01);
        return Status;
    }

    Status = OutNode->SetGPIO(0x01, 1000, Data, EnableMask, Direction);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: SetGPIO failed with status %x Node %u\n", Status, 0x01);
        return Status;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CAdapterCommon::ExecuteVendorSpecificQuirks(
    _In_ PVOID Node)
{
    CFunctionGroupNode *OutNode = (CFunctionGroupNode *)Node;
    NTSTATUS Status;

    // Avoid D3 to keep GPIO up
    Status = OutNode->SetPowerState(m_FunctionGroupStartNode, TRUE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: SetPowerState failed with status %x\n", Status);
        return Status;
    }

    // The external amplifier (EAPD) is switched by GPIO0
    Status = StacGPIOSetup((PVOID)OutNode, 0x1, 0x1, 0x1);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: StacGPIOSetup failed with status %x\n", Status);
        return Status;
    }

    return STATUS_SUCCESS;
}
