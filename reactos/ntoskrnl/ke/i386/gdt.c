/*
 *  ReactOS kernel
 *  Copyright (C) 2000  ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/gdt.c
 * PURPOSE:         GDT managment
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

USHORT KiGdt[11 * 4] = 
{
 0x0, 0x0, 0x0, 0x0,              /* Null */
 0xffff, 0x0, 0x9a00, 0xcf,       /* Kernel CS */
 0xffff, 0x0, 0x9200, 0xcf,       /* Kernel DS */
 0x0, 0x0, 0xfa00, 0xcc,          /* User CS */
 0x0, 0x0, 0xf200, 0xcc,          /* User DS */
 0x0, 0x0, 0x0, 0x0,              /* TSS */
 0x1000, 0xf000, 0x92df, 0xff00,  /* PCR */
 0x1000, 0x0, 0xf200, 0x0,        /* TEB */
 0x0, 0x0, 0x0, 0x0,              /* Reserved */
 0x0, 0x0, 0x0, 0x0,              /* LDT */
 0x0, 0x0, 0x0, 0x0               /* Trap TSS */
};

static KSPIN_LOCK GdtLock;

/* FUNCTIONS *****************************************************************/

VOID 
KeSetBaseGdtSelector(ULONG Entry,
		     PVOID Base)
{
   KIRQL oldIrql;
   
   DPRINT("KeSetBaseGdtSelector(Entry %x, Base %x)\n",
	   Entry, Base);
   
   KeAcquireSpinLock(&GdtLock, &oldIrql);
   
   Entry = (Entry & (~0x3)) / 2;
   
   KiGdt[Entry + 1] = ((ULONG)Base) & 0xffff;
   
   KiGdt[Entry + 2] = KiGdt[Entry + 2] & ~(0xff);
   KiGdt[Entry + 2] = KiGdt[Entry + 2] |
     ((((ULONG)Base) & 0xff0000) >> 16);
   
   KiGdt[Entry + 3] = KiGdt[Entry + 3] & ~(0xff00);
   KiGdt[Entry + 3] = KiGdt[Entry + 3] |
     ((((ULONG)Base) & 0xff000000) >> 16);
   
   DPRINT("%x %x %x %x\n", 
	   KiGdt[Entry + 0],
	   KiGdt[Entry + 1],
	   KiGdt[Entry + 2],
	   KiGdt[Entry + 3]);
   
   KeReleaseSpinLock(&GdtLock, oldIrql);
}

VOID 
KeDumpGdtSelector(ULONG Entry)
{
   USHORT a, b, c, d;
   ULONG RawLimit;
   
   a = KiGdt[Entry*4];
   b = KiGdt[Entry*4 + 1];
   c = KiGdt[Entry*4 + 2];
   d = KiGdt[Entry*4 + 3];
   
   DbgPrint("Base: %x\n", b + ((c & 0xff) * (1 << 16)) +
	    ((d & 0xff00) * (1 << 16)));
   RawLimit = a + ((d & 0xf) * (1 << 16));
   if (d & 0x80)
     {
	DbgPrint("Limit: %x\n", RawLimit * 4096);
     }
   else
     {
	DbgPrint("Limit: %x\n", RawLimit);
     }
   DbgPrint("Accessed: %d\n", (c & 0x100) >> 8);
   DbgPrint("Type: %x\n", (c & 0xe00) >> 9);
   DbgPrint("System: %d\n", (c & 0x1000) >> 12);
   DbgPrint("DPL: %d\n", (c & 0x6000) >> 13);
   DbgPrint("Present: %d\n", (c & 0x8000) >> 15);
   DbgPrint("AVL: %x\n", (d & 0x10) >> 4);
   DbgPrint("D: %d\n", (d & 0x40) >> 6);
   DbgPrint("G: %d\n", (d & 0x80) >> 7);
}

#if 0
VOID KeFreeGdtSelector(ULONG Entry)
/*
 * FUNCTION: Free a gdt selector
 * ARGUMENTS:
 *       Entry = Entry to free
 */
{
   KIRQL oldIrql;
   
   DPRINT("KeFreeGdtSelector(Entry %d)\n",Entry);
   
   if (Entry > (8 + NR_TASKS))
     {
	DPRINT1("Entry too large\n");
	KeBugCheck(0);
     }
   
   KeAcquireSpinLock(&GdtLock, &oldIrql);
   KiGdt[Entry*4] = 0;
   KiGdt[Entry*4 + 1] = 0;
   KiGdt[Entry*4 + 2] = 0;
   KiGdt[Entry*4 + 3] = 0;
   KeReleaseSpinLock(&GdtLock, oldIrql);
}
#endif

#if 0
ULONG 
KeAllocateGdtSelector(ULONG Desc[2])
/*
 * FUNCTION: Allocate a gdt selector
 * ARGUMENTS:
 *      Desc = Contents for descriptor
 * RETURNS: The index of the entry allocated
 */
{
   ULONG i;
   KIRQL oldIrql;
   
   DPRINT("KeAllocateGdtSelector(Desc[0] %x, Desc[1] %x)\n",
	  Desc[0], Desc[1]);
   
   KeAcquireSpinLock(&GdtLock, &oldIrql);   
   for (i=8; i<(8 + NR_TASKS); i++)
     {
	if (KiGdt[i*4] == 0 &&
	    KiGdt[i*4 + 1] == 0 &&
	    KiGdt[i*4 + 2] == 0 &&
	    KiGdt[i*4 + 3] == 0)
	  {
	     ((PULONG)KiGdt)[i*2] = Desc[0];
	     ((PULONG)KiGdt)[i*2 + 1] = Desc[1];
	     KeReleaseSpinLock(&GdtLock, oldIrql);
	     DPRINT("KeAllocateGdtSelector() = %x\n",i);
	     return(i);
	  }
     }
   KeReleaseSpinLock(&GdtLock, oldIrql);
   return(0);
}
#endif
