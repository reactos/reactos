/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/i386/ldt.c
 * PURPOSE:         LDT managment
 *
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

static KSPIN_LOCK LdtLock;
static KSPIN_LOCK GdtLock;

/* FUNCTIONS *****************************************************************/

VOID
KeSetBaseGdtSelector(ULONG Entry,
		     PVOID Base)
{
   KIRQL oldIrql;
   PUSHORT Gdt;

   DPRINT("KeSetBaseGdtSelector(Entry %x, Base %x)\n",
	   Entry, Base);

   KeAcquireSpinLock(&GdtLock, &oldIrql);

   Gdt = (PUSHORT)KeGetPcr()->GDT;
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

   Gdt = (PULONG) KeGetPcr()->GDT;
   Entry = (Entry & (~0x3)) / 4;

   Gdt[Entry] = Value1;
   Gdt[Entry + 1] = Value2;

   DPRINT("%x %x\n",
	   Gdt[Entry + 0],
	   Gdt[Entry + 1]);

   KeReleaseSpinLock(&GdtLock, oldIrql);
}

BOOLEAN PspIsDescriptorValid(PLDT_ENTRY ldt_entry)
{
  ULONG Base, SegLimit;
  /*
     Allow invalid descriptors.
  */
  if(!ldt_entry->HighWord.Bits.Type &&
     !ldt_entry->HighWord.Bits.Dpl)
       return TRUE;

  /* eliminate system descriptors and code segments other than
     execute and execute/read and DPL<3 descriptors */
  if(!(ldt_entry->HighWord.Bits.Type & 0x10) ||
     (ldt_entry->HighWord.Bits.Type & 0x8 &&
      ldt_entry->HighWord.Bits.Type & 0x4) ||
     ldt_entry->HighWord.Bits.Dpl != 3 ||
     ldt_entry->HighWord.Bits.Reserved_0) return FALSE;

  if(!ldt_entry->HighWord.Bits.Pres) return TRUE;

  Base=ldt_entry->BaseLow | (ldt_entry->HighWord.Bytes.BaseMid << 16) |
             (ldt_entry->HighWord.Bytes.BaseHi << 24);

  SegLimit=ldt_entry->LimitLow |
                 (ldt_entry->HighWord.Bits.LimitHi << 16);

  if(ldt_entry->HighWord.Bits.Type & 0x4)
  {
    SegLimit=(ldt_entry->HighWord.Bits.Default_Big) ? -1 : (USHORT)-1;

  } else if(ldt_entry->HighWord.Bits.Granularity)
  {
    SegLimit=(SegLimit << 12) | 0xfff;
  }

  if ((Base + SegLimit > (ULONG_PTR) MmHighestUserAddress) ||
      (Base > Base+SegLimit))
  {
    DPRINT1("WARNING: Windows would mark this descriptor invalid!");
  }

  /*
     Certain "DOS32" programs expect to be able to create DPMI selectors
     that wrap the address space.  Windows NT does not allow user-created
     selectors to reach into kernel memory.  However, there is no security
     risk in allowing it; the page table will prevent access anyway.
  */  
  return (/*(Base + SegLimit > (ULONG_PTR) MmHighestUserAddress) ||
          (Base > Base+SegLimit) ? FALSE : TRUE*/ TRUE);
}

NTSTATUS STDCALL
NtSetLdtEntries (ULONG Selector1,
		 LDT_ENTRY LdtEntry1,
		 ULONG Selector2,
		 LDT_ENTRY LdtEntry2)
{
  KIRQL oldIrql;
  ULONG NewLdtSize = sizeof(LDT_ENTRY);
  PUSHORT LdtDescriptor;
  ULONG LdtBase;
  ULONG LdtLimit;

  if((Selector1 & ~0xffff) || (Selector2 & ~0xffff)) return STATUS_INVALID_LDT_DESCRIPTOR;

  Selector1 &= ~0x7;
  Selector2 &= ~0x7;

  if((Selector1 && !PspIsDescriptorValid(&LdtEntry1)) ||
     (Selector2 && !PspIsDescriptorValid(&LdtEntry2))) return STATUS_INVALID_LDT_DESCRIPTOR;
  if(!(Selector1 || Selector2)) return STATUS_SUCCESS;

  NewLdtSize += (Selector1 >= Selector2) ? Selector1 : Selector2;

  KeAcquireSpinLock(&LdtLock, &oldIrql);

  LdtDescriptor = (PUSHORT) &PsGetCurrentProcess()->Pcb.LdtDescriptor;
  LdtBase = LdtDescriptor[1] |
                  ((LdtDescriptor[2] & 0xff) << 16) |
                  ((LdtDescriptor[3] & ~0xff) << 16);
  LdtLimit = LdtDescriptor[0] |
                   ((LdtDescriptor[3] & 0xf) << 16);

  if(LdtLimit < (NewLdtSize - 1))
  {
    /* allocate new ldt, copy old one there, set gdt ldt entry to new
       values and load ldtr register and free old ldt */

    ULONG NewLdtBase = (ULONG) ExAllocatePool(NonPagedPool,
                                              NewLdtSize);

    if(!NewLdtBase)
    {
      KeReleaseSpinLock(&LdtLock, oldIrql);
      return STATUS_INSUFFICIENT_RESOURCES;
    }

    if(LdtBase)
    {
      memcpy((PVOID) NewLdtBase, (PVOID) LdtBase, LdtLimit+1);
    }

    LdtDescriptor[0] = (USHORT)((--NewLdtSize) & 0xffff);
    LdtDescriptor[1] = (USHORT)(NewLdtBase & 0xffff);
    LdtDescriptor[2] = (USHORT)(((NewLdtBase & 0xff0000) >> 16) | 0x8200);
    LdtDescriptor[3] = (USHORT)(((NewLdtSize & 0xf0000) >> 16) |
                                ((NewLdtBase & 0xff000000) >> 16));

    KeSetGdtSelector(KGDT_LDT,
                     ((PULONG) LdtDescriptor)[0],
                     ((PULONG) LdtDescriptor)[1]);

    Ke386SetLocalDescriptorTable(KGDT_LDT);

    if(LdtBase)
    {
      ExFreePool((PVOID) LdtBase);
    }

    LdtBase = NewLdtBase;
  }

  if(Selector1)
  {
    memcpy((char*)LdtBase + Selector1,
           &LdtEntry1,
           sizeof(LDT_ENTRY));
  }

  if(Selector2)
  {
    memcpy((char*)LdtBase + Selector2,
           &LdtEntry2,
           sizeof(LDT_ENTRY));
  }

  KeReleaseSpinLock(&LdtLock, oldIrql);
  return STATUS_SUCCESS;
}

