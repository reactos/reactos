/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Configuration Tree Routines
 * COPYRIGHT:   Copyright 2005-2006 Alex Ionescu <alex.ionescu@reactos.org>
 *
 * NOTE: This module is shared by both the kernel and the bootloader.
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
PCONFIGURATION_COMPONENT_DATA
NTAPI
KeFindConfigurationNextEntry(
    _In_ PCONFIGURATION_COMPONENT_DATA Child,
    _In_ CONFIGURATION_CLASS Class,
    _In_ CONFIGURATION_TYPE Type,
    _In_opt_ PULONG ComponentKey,
    _Inout_ PCONFIGURATION_COMPONENT_DATA *NextLink)
{
    ULONG Key = 0;
    ULONG Mask = 0;
    PCONFIGURATION_COMPONENT_DATA Sibling;
    PCONFIGURATION_COMPONENT_DATA ReturnEntry;

    /* If we did get a key, then use it instead */
    if (ComponentKey)
    {
        Key = *ComponentKey;
        Mask = -1;
    }

    /* Loop the components until we find a a match */
    for (; Child; Child = Child->Child)
    {
        /* Check if we are starting somewhere already */
        if (*NextLink)
        {
            /* If we've found the place where we started, clear and continue */
            if (Child == *NextLink)
                *NextLink = NULL;
        }
        else
        {
            /* Try to get a match */
            if ((Child->ComponentEntry.Class) == Class &&
                (Child->ComponentEntry.Type) == Type &&
                (Child->ComponentEntry.Key & Mask) == Key)
            {
                /* Match found */
                return Child;
            }
        }

        /* Now we've also got to lookup the siblings */
        for (Sibling = Child->Sibling; Sibling; Sibling = Sibling->Sibling)
        {
            /* Check if we are starting somewhere already */
            if (*NextLink)
            {
                /* If we've found the place where we started, clear and continue */
                if (Sibling == *NextLink)
                    *NextLink = NULL;
            }
            else
            {
                /* Try to get a match */
                if ((Sibling->ComponentEntry.Class == Class) &&
                    (Sibling->ComponentEntry.Type == Type) &&
                    (Sibling->ComponentEntry.Key & Mask) == Key)
                {
                    /* Match found */
                    return Sibling;
                }
            }

            /* We've got to check if the sibling has a child as well */
            if (Sibling->Child)
            {
                /* We're just going to call ourselves again */
                ReturnEntry = KeFindConfigurationNextEntry(Sibling->Child,
                                                           Class,
                                                           Type,
                                                           ComponentKey,
                                                           NextLink);
                if (ReturnEntry)
                    return ReturnEntry;
            }
        }
    }

    /* If we got here, nothing was found */
    return NULL;
}

/*
 * @implemented
 */
PCONFIGURATION_COMPONENT_DATA
NTAPI
KeFindConfigurationEntry(
    _In_ PCONFIGURATION_COMPONENT_DATA Child,
    _In_ CONFIGURATION_CLASS Class,
    _In_ CONFIGURATION_TYPE Type,
    _In_opt_ PULONG ComponentKey)
{
    PCONFIGURATION_COMPONENT_DATA NextLink = NULL;

    /* Start search at the root */
    return KeFindConfigurationNextEntry(Child,
                                        Class,
                                        Type,
                                        ComponentKey,
                                        &NextLink);
}
