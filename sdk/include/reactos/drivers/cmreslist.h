/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Helper functions to parse CM_RESOURCE_LISTs
 * COPYRIGHT:   Copyright 2020 Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include <wdm.h>

//
// Resource list helpers
//

/* Usage note:
 * As there can be only one variable-sized CM_PARTIAL_RESOURCE_DESCRIPTOR in the list (and it must be the last one),
 * a right looping through resources can look like this:
 *
 * PCM_FULL_RESOURCE_DESCRIPTOR FullDesc = &ResourceList->List[0];
 * for (ULONG i = 0; i < ResourceList->Count; i++, FullDesc = CmiGetNextResourceDescriptor(FullDesc))
 * {
 *    for (ULONG j = 0; j < FullDesc->PartialResourceList.Count; j++)
 *    {
 *        PartialDesc = &FullDesc->PartialResourceList.PartialDescriptors[j];
 *        // work with PartialDesc
 *    }
 * }
 */

FORCEINLINE
PCM_PARTIAL_RESOURCE_DESCRIPTOR
CmiGetNextPartialDescriptor(
    _In_ const CM_PARTIAL_RESOURCE_DESCRIPTOR *PartialDescriptor)
{
    const CM_PARTIAL_RESOURCE_DESCRIPTOR *NextDescriptor;

    /* Assume the descriptors are the fixed size ones */
    NextDescriptor = PartialDescriptor + 1;

    /* But check if this is actually a variable-sized descriptor */
    if (PartialDescriptor->Type == CmResourceTypeDeviceSpecific)
    {
        /* Add the size of the variable section as well */
        NextDescriptor = (PCM_PARTIAL_RESOURCE_DESCRIPTOR)((ULONG_PTR)NextDescriptor +
                         PartialDescriptor->u.DeviceSpecificData.DataSize);
        ASSERT(NextDescriptor >= PartialDescriptor + 1);
    }

    /* Now the correct pointer has been computed, return it */
    return (PCM_PARTIAL_RESOURCE_DESCRIPTOR)NextDescriptor;
}

FORCEINLINE
PCM_FULL_RESOURCE_DESCRIPTOR
CmiGetNextResourceDescriptor(
    _In_ const CM_FULL_RESOURCE_DESCRIPTOR *ResourceDescriptor)
{
    const CM_PARTIAL_RESOURCE_DESCRIPTOR *LastPartialDescriptor;

    /* Calculate the location of the last partial descriptor, which can have a
       variable size! */
    LastPartialDescriptor = &ResourceDescriptor->PartialResourceList.PartialDescriptors[
            ResourceDescriptor->PartialResourceList.Count - 1];

    /* Next full resource descriptor follows the last partial descriptor */
    return (PCM_FULL_RESOURCE_DESCRIPTOR)CmiGetNextPartialDescriptor(LastPartialDescriptor);
}
