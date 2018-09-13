/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    config.c

Abstract:

    This module implements the code to find an ARC configuration tree
    entry as constructed by the OS Loader.

Author:

    David N. Cutler (davec) 9-Sep-1991

Environment:

    User mode only.

Revision History:

--*/

#include "ki.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,KeFindConfigurationEntry)
#pragma alloc_text(INIT,KeFindConfigurationNextEntry)
#endif

PCONFIGURATION_COMPONENT_DATA
KeFindConfigurationEntry (
    IN PCONFIGURATION_COMPONENT_DATA Child,
    IN CONFIGURATION_CLASS Class,
    IN CONFIGURATION_TYPE Type,
    IN PULONG Key OPTIONAL
    )
/*++

Routine Description:

    This function search the specified configuration tree and returns a
    pointer to an entry that matches the specified class, type, and key
    parameters.

    This routine is the same as KeFindConfurationEntryNext expect
    that the search is performed from the first entry

    N.B. This routine can only be called during system initialization.

--*/
{
    PCONFIGURATION_COMPONENT_DATA Resume;

    Resume = NULL;
    return KeFindConfigurationNextEntry (Child, Class, Type, Key, &Resume);
}

PCONFIGURATION_COMPONENT_DATA
KeFindConfigurationNextEntry (
    IN PCONFIGURATION_COMPONENT_DATA Child,
    IN CONFIGURATION_CLASS Class,
    IN CONFIGURATION_TYPE Type,
    IN PULONG Key OPTIONAL,
    IN PCONFIGURATION_COMPONENT_DATA *Resume
    )

/*++

Routine Description:

    This function search the specified configuration tree and returns a
    pointer to an entry that matches the specified class, type, and key
    parameters.

    N.B. This routine can only be called during system initialization.

Arguments:

    Child - Supplies an optional pointer to an NT configuration component.

    Class - Supplies the configuration class of the entry to locate.

    Type - Supplies the configuration type of the entry to locate.

    Key - Supplies a pointer to an optional key value to use in locating
        the specified entry.

    Resume - Supplies the last returned entry for which the search
        should resume from.

Return Value:

    If the specified entry is located, then a pointer to the configuration
    entry is returned as the function value. Otherwise, NULL is returned.

--*/

{

    PCONFIGURATION_COMPONENT_DATA Entry;
    ULONG MatchKey;
    ULONG MatchMask;
    PCONFIGURATION_COMPONENT_DATA Sibling;

    //
    // Initialize the match key and mask based on whether the optional key
    // value is specified.
    //

    if (ARGUMENT_PRESENT(Key)) {
        MatchMask = 0xffffffff;
        MatchKey = *Key;

    } else {
        MatchMask = 0;
        MatchKey = 0;
    }

    //
    // Search specified configuration tree for an entry that matches the
    // the specified class, type, and key.
    //

    while (Child != NULL) {
        if (*Resume) {
            //
            // If resume location found, clear resume location and continue
            // search with next entry
            //

            if (Child == *Resume) {
                *Resume = NULL;
            }
        } else {

            //
            // If the class, type, and key match, then return a pointer to
            // the child entry.
            //

            if ((Child->ComponentEntry.Class == Class) &&
                (Child->ComponentEntry.Type == Type) &&
                ((Child->ComponentEntry.Key & MatchMask) == MatchKey)) {
                return Child;
            }
        }

        //
        // If the child has a sibling list, then search the sibling list
        // for an entry that matches the specified class, type, and key.
        //

        Sibling = Child->Sibling;
        while (Sibling != NULL) {
            if (*Resume) {
                //
                // If resume location found, clear resume location and continue
                // search with next entry
                //

                if (Sibling == *Resume) {
                    *Resume = NULL;
                }
            } else {

                //
                // If the class, type, and key match, then return a pointer to
                // the child entry.
                //

                if ((Sibling->ComponentEntry.Class == Class) &&
                    (Sibling->ComponentEntry.Type == Type) &&
                    ((Sibling->ComponentEntry.Key & MatchMask) == MatchKey)) {
                    return Sibling;
                }
            }

            //
            // If the sibling has a child tree, then search the child tree
            // for an entry that matches the specified class, type, and key.
            //

            if (Sibling->Child != NULL) {
               Entry = KeFindConfigurationNextEntry (
                                Sibling->Child,
                                Class,
                                Type,
                                Key,
                                Resume
                                );

               if (Entry != NULL) {
                   return Entry;
               }
            }

            Sibling = Sibling->Sibling;
        }

        Child = Child->Child;
    }

    return NULL;
}
