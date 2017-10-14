/*
 * PROJECT:     ReactOS Storport Driver
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Miniport interface code
 * COPYRIGHT:   Copyright 2017 Eric Kohl (eric.kohl@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include "precomp.h"

#define NDEBUG
#include <debug.h>


/* FUNCTIONS ******************************************************************/

NTSTATUS
MiniportInitialize(
    _In_ PMINIPORT Miniport,
    _In_ PFDO_DEVICE_EXTENSION DeviceExtension,
    _In_ PHW_INITIALIZATION_DATA InitData)
{
    PMINIPORT_DEVICE_EXTENSION MiniportExtension;
    ULONG Size;

    DPRINT1("MiniportInitialize(%p %p %p)\n",
            Miniport, DeviceExtension, InitData);

    Miniport->DeviceExtension = DeviceExtension;
    Miniport->InitData = InitData;

    /* Calculate the miniport device extension size */
    Size = sizeof(MINIPORT_DEVICE_EXTENSION) +
           Miniport->InitData->DeviceExtensionSize;

    /* Allocate and initialize the miniport device extension */
    MiniportExtension = ExAllocatePoolWithTag(NonPagedPool,
                                              Size,
                                              TAG_MINIPORT_DATA);
    if (MiniportExtension == NULL)
        return STATUS_NO_MEMORY;

    RtlZeroMemory(MiniportExtension, Size);

    MiniportExtension->Miniport = Miniport;
    Miniport->MiniportExtension = MiniportExtension;

    return STATUS_SUCCESS;
}


NTSTATUS
MiniportFindAdapter(
    _In_ PMINIPORT Miniport)
{
    BOOLEAN Reserved = FALSE;
    ULONG Result;
    NTSTATUS Status;

    DPRINT1("MiniportFindAdapter(%p)\n", Miniport);

    /* Call the miniport HwFindAdapter routine */
    Result = Miniport->InitData->HwFindAdapter(&Miniport->MiniportExtension->HwDeviceExtension,
                                               NULL,
                                               NULL,
                                               NULL,
                                               NULL,
                                               &Reserved);
    DPRINT1("HwFindAdapter() returned %lu\n", Result);

    switch (Result)
    {
        case SP_RETURN_NOT_FOUND:
            DPRINT1("SP_RETURN_NOT_FOUND\n");
            Status = STATUS_NOT_FOUND;
            break;

        case SP_RETURN_FOUND:
            DPRINT1("SP_RETURN_FOUND\n");
            Status = STATUS_SUCCESS;
            break;

        case SP_RETURN_ERROR:
            DPRINT1("SP_RETURN_ERROR\n");
            Status = STATUS_ADAPTER_HARDWARE_ERROR;
            break;

        case SP_RETURN_BAD_CONFIG:
            DPRINT1("SP_RETURN_BAD_CONFIG\n");
            Status = STATUS_DEVICE_CONFIGURATION_ERROR;
            break;

        default:
            DPRINT1("Unknown result: %lu\n", Result);
            Status = STATUS_INTERNAL_ERROR;
            break;
    }

    return Status;
}


NTSTATUS
MiniportHwInitialize(
    _In_ PMINIPORT Miniport)
{
    NTSTATUS Status;

    DPRINT1("MiniportHwInitialize(%p)\n", Miniport);

    /* Call the miniport HwInitialize routine */
    Status = Miniport->InitData->HwInitialize(&Miniport->MiniportExtension->HwDeviceExtension);
    DPRINT1("HwInitialize() returned 0x%08lx\n", Status);

    return Status;
}

/* EOF */
