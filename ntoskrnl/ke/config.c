/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/config.c
 * PURPOSE:         Configuration Tree Routines
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
PCONFIGURATION_COMPONENT_DATA
NTAPI
INIT_FUNCTION
KeFindConfigurationEntry(IN PCONFIGURATION_COMPONENT_DATA Child,
                         IN CONFIGURATION_CLASS Class,
                         IN CONFIGURATION_TYPE Type,
                         IN PULONG ComponentKey OPTIONAL)
{
    /* Start Search at Root */
    return KeFindConfigurationNextEntry(Child,
                                        Class,
                                        Type,
                                        ComponentKey,
                                        NULL);
}

/*
 * @implemented
 */
PCONFIGURATION_COMPONENT_DATA
NTAPI
INIT_FUNCTION
KeFindConfigurationNextEntry(IN PCONFIGURATION_COMPONENT_DATA Child,
                             IN CONFIGURATION_CLASS Class,
                             IN CONFIGURATION_TYPE Type,
                             IN PULONG ComponentKey OPTIONAL,
                             IN PCONFIGURATION_COMPONENT_DATA *NextLink)
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

    /* Loop the Components until we find a a match */
    while (Child)
    {
        /* Check if we are starting somewhere already */
        if (*NextLink)
        {
            /* If we've found the place where we started, clear and continue */
            if (Child == *NextLink) *NextLink = NULL;
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
        Sibling = Child->Sibling;
        while (Sibling)
        {
            /* Check if we are starting somewhere already */
            if (*NextLink)
            {
                /* If we've found the place where we started, clear and continue */
                if (Sibling == *NextLink) *NextLink = NULL;
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

            /* We've got to check if the Sibling has a Child as well */
            if (Sibling->Child)
            {
                /* We're just going to call ourselves again */
                ReturnEntry = KeFindConfigurationNextEntry(Sibling->Child,
                                                           Class,
                                                           Type,
                                                           ComponentKey,
                                                           NextLink);
                if (ReturnEntry) return ReturnEntry;
            }

            /* Next Sibling */
            Sibling = Sibling->Sibling;
        }

        /* Next Child */
        Child = Child->Child;
    }

    /* If we got here, nothign was found */
    return NULL;
}

