/*
 * PROJECT:         EFI Windows Loader
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            freeldr/winldr/conversion.c
 * PURPOSE:         Physical <-> Virtual addressing mode conversions
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES ***************************************************************/

#include <freeldr.h>

//#include <ndk/ldrtypes.h>
#include <debug.h>

/* FUNCTIONS **************************************************************/

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

VOID
List_PaToVa(LIST_ENTRY *ListEntry)
{
	LIST_ENTRY *ListHead = ListEntry;
	LIST_ENTRY *Next = ListEntry->Flink;
	LIST_ENTRY *NextPA;

	//Print(L"\n\nList_Entry: %X, First Next: %X\n", ListEntry, Next);
	//
	// Walk through the whole list
	//
	if (Next != NULL)
	{
		while (Next != PaToVa(ListHead))
		{
			NextPA = VaToPa(Next);
			//Print(L"Current: %X, Flink: %X, Blink: %X\n", Next, NextPA->Flink, NextPA->Blink);

			NextPA->Flink = PaToVa((PVOID)NextPA->Flink);
			NextPA->Blink = PaToVa((PVOID)NextPA->Blink);

			//Print(L"After converting Flink: %X, Blink: %X\n", NextPA->Flink, NextPA->Blink);

			Next = NextPA->Flink;
		}

		//
		// Finally convert first Flink/Blink
		//
		ListEntry->Flink = PaToVa((PVOID)ListEntry->Flink);
		if (ListEntry->Blink)
			ListEntry->Blink = PaToVa((PVOID)ListEntry->Blink);
	}
}

// This function converts only Child->Child, and calls itself for each Sibling
VOID
ConvertConfigToVA(PCONFIGURATION_COMPONENT_DATA Start)
{
	PCONFIGURATION_COMPONENT_DATA Child;
	PCONFIGURATION_COMPONENT_DATA Sibling;

	DPRINTM((DPRINT_WINDOWS, "ConvertConfigToVA(Start 0x%X)\n", Start));
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

		DPRINTM((DPRINT_WINDOWS, "Device 0x%X class %d type %d id '%s', parent %p\n", Child,
			Child->ComponentEntry.Class, Child->ComponentEntry.Type, VaToPa(Child->ComponentEntry.Identifier), Child->Parent));

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

			DPRINTM((DPRINT_WINDOWS, "Device 0x%X class %d type %d id '%s', parent %p\n", Sibling,
				Sibling->ComponentEntry.Class, Sibling->ComponentEntry.Type, VaToPa(Sibling->ComponentEntry.Identifier), Sibling->Parent));

			// Recurse into the Child tree
			if (VaToPa(Sibling->Child) != NULL)
				ConvertConfigToVA(VaToPa(Sibling->Child));

			Sibling = VaToPa(Sibling->Sibling);
		}

		// Go to the next child
		Child = VaToPa(Child->Child);
	}
}
