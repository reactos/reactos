/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/i386/gdt.c
 * PURPOSE:         GDT managment
 *
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

KGDTENTRY KiBootGdt[11] =
{
    {0x0000, 0x0000, {{0x00, 0x00, 0x00, 0x00}}},              /* Null */
    {0xffff, 0x0000, {{0x00, 0x9a, 0xcf, 0x00}}},       /* Kernel CS */
    {0xffff, 0x0000, {{0x00, 0x92, 0xcf, 0x00}}},       /* Kernel DS */
    {0xffff, 0x0000, {{0x00, 0xfa, 0xcf, 0x00}}},       /* User CS */
    {0xffff, 0x0000, {{0x00, 0xf2, 0xcf, 0x00}}},       /* User DS */
    {0x0000, 0x0000, {{0x00, 0x00, 0x00, 0x00}}},              /* TSS */
    {0x0fff, 0x0000, {{0x00, 0x92, 0x00, 0xff}}},  /* PCR */
    {0x0fff, 0x0000, {{0x00, 0xf2, 0x00, 0x00}}},        /* TEB */
    {0x0000, 0x0000, {{0x00, 0x00, 0x00, 0x00}}},             /* Reserved */
    {0x0000, 0x0000, {{0x00, 0x00, 0x00, 0x00}}},             /* LDT */
    {0x0000, 0x0000, {{0x00, 0x00, 0x00, 0x00}}}             /* Trap TSS */
};

KDESCRIPTOR KiGdtDescriptor = {sizeof(KiBootGdt), (ULONG)KiBootGdt};

static KSPIN_LOCK GdtLock;

/* FUNCTIONS *****************************************************************/

/*
 * @unimplemented
 */
NTSTATUS STDCALL
KeI386FlatToGdtSelector(
	IN ULONG	Base,
	IN USHORT	Length,
	IN USHORT	Selector
)
{
	UNIMPLEMENTED;
	return 0;
}

/*
 * @unimplemented
 */
NTSTATUS STDCALL
KeI386ReleaseGdtSelectors(
	OUT PULONG SelArray,
	IN ULONG NumOfSelectors
)
{
	UNIMPLEMENTED;
	return 0;
}

/*
 * @unimplemented
 */
NTSTATUS STDCALL
KeI386AllocateGdtSelectors(
	OUT PULONG SelArray,
	IN ULONG NumOfSelectors
)
{
	UNIMPLEMENTED;
	return 0;
}

VOID
KeSetBaseGdtSelector(ULONG Entry,
		     PVOID Base)
{
   KIRQL oldIrql;
   PUSHORT Gdt;

   DPRINT("KeSetBaseGdtSelector(Entry %x, Base %x)\n",
	   Entry, Base);

   KeAcquireSpinLock(&GdtLock, &oldIrql);

   Gdt = KeGetCurrentKPCR()->GDT;
   Entry = (Entry & (~0x3)) / 2;

   Gdt[Entry + 1] = (USHORT)(((ULONG)Base) & 0xffff);

   Gdt[Entry + 2] = Gdt[Entry + 2] & ~(0xff);
   Gdt[Entry + 2] = (USHORT)(Gdt[Entry + 2] |
     ((((ULONG)Base) & 0xff0000) >> 16));

   Gdt[Entry + 3] = Gdt[Entry + 3] & ~(0xff00);
   Gdt[Entry + 3] = (USHORT)(Gdt[Entry + 3] |
     ((((ULONG)Base) & 0xff000000) >> 16));

   DPRINT("%x %x %x %x\n",
	   Gdt[Entry + 0],
	   Gdt[Entry + 1],
	   Gdt[Entry + 2],
	   Gdt[Entry + 3]);

   KeReleaseSpinLock(&GdtLock, oldIrql);
}

VOID
KeSetGdtSelector(ULONG Entry,
                 ULONG Value1,
                 ULONG Value2)
{
   KIRQL oldIrql;
   PULONG Gdt;

   DPRINT("KeSetGdtSelector(Entry %x, Value1 %x, Value2 %x)\n",
	   Entry, Value1, Value2);

   KeAcquireSpinLock(&GdtLock, &oldIrql);

   Gdt = (PULONG) KeGetCurrentKPCR()->GDT;
   Entry = (Entry & (~0x3)) / 4;

   Gdt[Entry] = Value1;
   Gdt[Entry + 1] = Value2;

   DPRINT("%x %x\n",
	   Gdt[Entry + 0],
	   Gdt[Entry + 1]);

   KeReleaseSpinLock(&GdtLock, oldIrql);
}

/* EOF */
