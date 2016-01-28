/*
 * PROJECT:         EFI Windows Loader
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            boot/freeldr/freeldr/windows/conversion.c
 * PURPOSE:         Physical <-> Virtual addressing mode conversions
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES ***************************************************************/

#include <freeldr.h>

//#include <ndk/ldrtypes.h>
#include <debug.h>

DBG_DEFAULT_CHANNEL(WINDOWS);

/* FUNCTIONS **************************************************************/

#ifndef _ZOOM2_
/* Arch-specific addresses translation implementation */
PVOID
VaToPa(PVOID Va)
{
    return (PVOID)((ULONG_PTR)Va & ~KSEG0_BASE);
}

PVOID
PaToVa(PVOID Pa)
{
    return (PVOID)((ULONG_PTR)Pa | KSEG0_BASE);
}
#else
PVOID
VaToPa(PVOID Va)
{
    return Va;
}

PVOID
PaToVa(PVOID Pa)
{
    return Pa;
}
#endif

VOID
List_PaToVa(_In_ PLIST_ENTRY ListHeadPa)
{
    PLIST_ENTRY EntryPa, NextPa;

    /* List must be properly initialized */
    ASSERT(ListHeadPa->Flink != 0);
    ASSERT(ListHeadPa->Blink != 0);

    /* Loop the list in physical address space */
    EntryPa = ListHeadPa->Flink;
    while (EntryPa != ListHeadPa)
    {
        /* Save the physical address of the next entry */
        NextPa = EntryPa->Flink;

        /* Convert the addresses of this entry */
        EntryPa->Flink = PaToVa(EntryPa->Flink);
        EntryPa->Blink = PaToVa(EntryPa->Blink);

        /* Go to the next entry */
        EntryPa = NextPa;
    }

    /* Finally convert the list head */
    ListHeadPa->Flink = PaToVa(ListHeadPa->Flink);
    ListHeadPa->Blink = PaToVa(ListHeadPa->Blink);
}

// This function converts only Child->Child, and calls itself for each Sibling
VOID
ConvertConfigToVA(PCONFIGURATION_COMPONENT_DATA Start)
{
    PCONFIGURATION_COMPONENT_DATA Child;
    PCONFIGURATION_COMPONENT_DATA Sibling;

    TRACE("ConvertConfigToVA(Start 0x%X)\n", Start);
    Child = Start;

    while (Child != NULL)
    {
        if (Child->ConfigurationData)
            Child->ConfigurationData = PaToVa(Child->ConfigurationData);

        if (Child->Child)
            Child->Child = PaToVa(Child->Child);

        if (Child->Parent)
            Child->Parent = PaToVa(Child->Parent);

        if (Child->Sibling)
            Child->Sibling = PaToVa(Child->Sibling);

        if (Child->ComponentEntry.Identifier)
            Child->ComponentEntry.Identifier = PaToVa(Child->ComponentEntry.Identifier);

        TRACE("Device 0x%X class %d type %d id '%s', parent %p\n", Child,
            Child->ComponentEntry.Class, Child->ComponentEntry.Type, VaToPa(Child->ComponentEntry.Identifier), Child->Parent);

        // Go through siblings list
        Sibling = VaToPa(Child->Sibling);
        while (Sibling != NULL)
        {
            if (Sibling->ConfigurationData)
                Sibling->ConfigurationData = PaToVa(Sibling->ConfigurationData);

            if (Sibling->Child)
                Sibling->Child = PaToVa(Sibling->Child);

            if (Sibling->Parent)
                Sibling->Parent = PaToVa(Sibling->Parent);

            if (Sibling->Sibling)
                Sibling->Sibling = PaToVa(Sibling->Sibling);

            if (Sibling->ComponentEntry.Identifier)
                Sibling->ComponentEntry.Identifier = PaToVa(Sibling->ComponentEntry.Identifier);

            TRACE("Device 0x%X class %d type %d id '%s', parent %p\n", Sibling,
                Sibling->ComponentEntry.Class, Sibling->ComponentEntry.Type, VaToPa(Sibling->ComponentEntry.Identifier), Sibling->Parent);

            // Recurse into the Child tree
            if (VaToPa(Sibling->Child) != NULL)
                ConvertConfigToVA(VaToPa(Sibling->Child));

            Sibling = VaToPa(Sibling->Sibling);
        }

        // Go to the next child
        Child = VaToPa(Child->Child);
    }
}
