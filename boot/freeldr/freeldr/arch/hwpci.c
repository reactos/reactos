/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 *              or MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     PCI bus detection routines
 * COPYRIGHT:   Copyright 2007 Aleksey Bragin <aleksey@reactos.org>
 *              Copyright 2009 Hervé Poussineau <hpoussin@reactos.org>
 *              Copyright 2026 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#include <freeldr.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(HWDETECT);

VOID
DetectPciBus(
    _In_ PCONFIGURATION_COMPONENT_DATA SystemKey,
    _Inout_ PULONG BusNumber,
    _In_ DETECT_PCI_BUS MachDetectPciBus)
{
    PCI_REGISTRY_INFO BusData;
    PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;
    PCONFIGURATION_COMPONENT_DATA BusKey;
    ULONG Size;
    ULONG i;

    /* Detect the PCI buses */
    if (!MachDetectPciBus(SystemKey, BusNumber, &BusData))
        return;

    /* Report PCI buses */
    for (i = 0; i < (ULONG)BusData.NoBuses; i++)
    {
        /* Check if this is the first bus */
        if (i == 0)
        {
            /* Set 'Configuration Data' value */
            Size = FIELD_OFFSET(CM_PARTIAL_RESOURCE_LIST, PartialDescriptors[1]) +
                   sizeof(BusData);
            PartialResourceList = FrLdrHeapAlloc(Size, TAG_HW_RESOURCE_LIST);
            if (!PartialResourceList)
            {
                ERR("Failed to allocate resource descriptor! Ignoring remaining PCI buses (i = %lu, NoBuses = %lu)\n",
                    i, (ULONG)BusData.NoBuses);
                return;
            }

            /* Initialize resource descriptor */
            RtlZeroMemory(PartialResourceList, Size);
            PartialResourceList->Version = 1;
            PartialResourceList->Revision = 1;
            PartialResourceList->Count = 1;

            PartialDescriptor = &PartialResourceList->PartialDescriptors[0];
            PartialDescriptor->Type = CmResourceTypeDeviceSpecific;
            PartialDescriptor->ShareDisposition = CmResourceShareUndetermined;
            PartialDescriptor->u.DeviceSpecificData.DataSize = sizeof(BusData);

            RtlCopyMemory(&PartialResourceList->PartialDescriptors[1],
                          &BusData, sizeof(BusData));
        }
        else
        {
            /* Set 'Configuration Data' value */
            Size = FIELD_OFFSET(CM_PARTIAL_RESOURCE_LIST, PartialDescriptors);
            PartialResourceList = FrLdrHeapAlloc(Size, TAG_HW_RESOURCE_LIST);
            if (!PartialResourceList)
            {
                ERR("Failed to allocate resource descriptor! Ignoring remaining PCI buses (i = %lu, NoBuses = %lu)\n",
                    i, (ULONG)BusData.NoBuses);
                return;
            }

            /* Initialize resource descriptor */
            RtlZeroMemory(PartialResourceList, Size);
        }

        /* Create the bus key */
        FldrCreateComponentKey(SystemKey,
                               AdapterClass,
                               MultiFunctionAdapter,
                               0,
                               0,
                               0xFFFFFFFF,
                               "PCI",
                               PartialResourceList,
                               Size,
                               &BusKey);

        /* Increment bus number */
        (*BusNumber)++;
    }
}

/* EOF */
