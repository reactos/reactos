/* $Id$
 *
 * COPYRIGHT:             See COPYING in the top level directory
 * PROJECT:               ReactOS kernel
 * FILE:                  hal/halx86/generic/ioapic.c
 * PURPOSE:               
 * PROGRAMMER:            
 */

/* INCLUDES *****************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *****************************************************************/

MP_CONFIGURATION_INTSRC IRQMap[MAX_IRQ_SOURCE]; /* Map of all IRQs */
ULONG IRQCount = 0;                             /* Number of IRQs  */
ULONG IrqApicMap[MAX_IRQ_SOURCE];

UCHAR BUSMap[MAX_BUS];				/* Map of all buses in the system */
UCHAR PCIBUSMap[MAX_BUS];			/* Map of all PCI buses in the system */

IOAPIC_INFO IOAPICMap[MAX_IOAPIC];		/* Map of all I/O APICs in the system */
ULONG IOAPICCount;				/* Number of I/O APICs in the system */

ULONG IRQVectorMap[MAX_IRQ_SOURCE];             /* IRQ to vector map */

/* EISA interrupts are always polarity zero and can be edge or level
 * trigger depending on the ELCR value.  If an interrupt is listed as
 * EISA conforming in the MP table, that means its trigger type must
 * be read in from the ELCR */

#define default_EISA_trigger(idx)	(EISA_ELCR(IRQMap[idx].SrcBusIrq))
#define default_EISA_polarity(idx)	(0)

/* ISA interrupts are always polarity zero edge triggered,
 * when listed as conforming in the MP table. */

#define default_ISA_trigger(idx)	(0)
#define default_ISA_polarity(idx)	(0)

/* PCI interrupts are always polarity one level triggered,
 * when listed as conforming in the MP table. */

#define default_PCI_trigger(idx)	(1)
#define default_PCI_polarity(idx)	(1)

/* MCA interrupts are always polarity zero level triggered,
 * when listed as conforming in the MP table. */

#define default_MCA_trigger(idx)	(1)
#define default_MCA_polarity(idx)	(0)

/***************************************************************************/

extern VOID Disable8259AIrq(ULONG irq);
ULONG IOAPICRead(ULONG Apic, ULONG Offset);
VOID IOAPICWrite(ULONG Apic, ULONG Offset, ULONG Value);

/* FUNCTIONS ***************************************************************/

/*
 * EISA Edge/Level control register, ELCR
 */
static ULONG EISA_ELCR(ULONG irq)
{
   if (irq < 16) 
   {
      PUCHAR port = (PUCHAR)(0x4d0 + (irq >> 3));
      return (READ_PORT_UCHAR(port) >> (irq & 7)) & 1;
   }
   DPRINT("Broken MPtable reports ISA irq %d\n", irq);
   return 0;
}

static ULONG 
IRQPolarity(ULONG idx)
{
   ULONG bus = IRQMap[idx].SrcBusId;
   ULONG polarity;

   /*
    * Determine IRQ line polarity (high active or low active):
    */
   switch (IRQMap[idx].IrqFlag & 3)
   {
      case 0: /* conforms, ie. bus-type dependent polarity */
         {
            switch (BUSMap[bus])
	    {
	       case MP_BUS_ISA: /* ISA pin */
		  polarity = default_ISA_polarity(idx);
		  break;
		  
	       case MP_BUS_EISA: /* EISA pin */
		  polarity = default_EISA_polarity(idx);
		  break;
		
	       case MP_BUS_PCI: /* PCI pin */
		  polarity = default_PCI_polarity(idx);
		  break;

	       case MP_BUS_MCA: /* MCA pin */
                  polarity = default_MCA_polarity(idx);
		  break;
		
	       default:
		  DPRINT("Broken BIOS!!\n");
		  polarity = 1;
	    }
	 }
	 break;

      case 1: /* high active */
	 polarity = 0;
	 break;

      case 2: /* reserved */
	 DPRINT("Broken BIOS!!\n");
	 polarity = 1;
	 break;

      case 3: /* low active */
         polarity = 1;
	 break;

      default: /* invalid */
	 DPRINT("Broken BIOS!!\n");
	 polarity = 1;
   }
   return polarity;
}

static ULONG 
IRQTrigger(ULONG idx)
{
   ULONG bus = IRQMap[idx].SrcBusId;
   ULONG trigger;

   /*
    * Determine IRQ trigger mode (edge or level sensitive):
    */
   switch ((IRQMap[idx].IrqFlag >> 2) & 3)
   {
      case 0: /* conforms, ie. bus-type dependent */
	 {
            switch (BUSMap[bus])
	    {
	       case MP_BUS_ISA: /* ISA pin */
	          trigger = default_ISA_trigger(idx);
		  break;

	       case MP_BUS_EISA: /* EISA pin */
		  trigger = default_EISA_trigger(idx);
		  break;
		
	       case MP_BUS_PCI: /* PCI pin */
		  trigger = default_PCI_trigger(idx);
		  break;

               case MP_BUS_MCA: /* MCA pin */
		  trigger = default_MCA_trigger(idx);
		  break;
		
               default:
                  DPRINT("Broken BIOS!!\n");
		  trigger = 1;
	    }
	 }
	 break;

      case 1: /* edge */
	 trigger = 0;
	 break;

      case 2: /* reserved */
	 DPRINT("Broken BIOS!!\n");
	 trigger = 1;
	 break;

      case 3: /* level */
 	 trigger = 1;
	 break;
	
      default: /* invalid */
	 DPRINT("Broken BIOS!!\n");
	 trigger = 0;					
   }
   return trigger;
}

static ULONG 
Pin2Irq(ULONG idx,
	ULONG apic,
	ULONG pin)
{
   ULONG irq, i;
   ULONG bus = IRQMap[idx].SrcBusId;

   /*
    * Debugging check, we are in big trouble if this message pops up!
    */
   if (IRQMap[idx].DstApicInt != pin) 
   {
      DPRINT("broken BIOS or MPTABLE parser, ayiee!!\n");
   }

   switch (BUSMap[bus])
   {
      case MP_BUS_ISA: /* ISA pin */
      case MP_BUS_EISA:
      case MP_BUS_MCA:
	irq = IRQMap[idx].SrcBusIrq;
	break;

      case MP_BUS_PCI: /* PCI pin */
	 /*
	  * PCI IRQs are mapped in order
	  */
	 i = irq = 0;
	 while (i < apic)
	 {
	    irq += IOAPICMap[i++].EntryCount;
	 }
	 irq += pin;
	 break;
	
      default:
	 DPRINT("Unknown bus type %d.\n",bus);
	 irq = 0;
   }
   return irq;
}

static ULONG 
AssignIrqVector(ULONG irq)
{
#if 0
   static ULONG current_vector = FIRST_DEVICE_VECTOR, vector_offset = 0;
#endif
   ULONG vector;
   /* There may already have been assigned a vector for this IRQ */
   vector = IRQVectorMap[irq];
   if (vector > 0)
   {
      return vector;
   }
#if 0
   if (current_vector > FIRST_SYSTEM_VECTOR) 
   {
      vector_offset++;
      current_vector = FIRST_DEVICE_VECTOR + vector_offset;
   } 
   else if (current_vector == FIRST_SYSTEM_VECTOR) 
   {
      DPRINT1("Ran out of interrupt sources!");
      KEBUGCHECK(0);
   }

   vector = current_vector;
   IRQVectorMap[irq] = vector;
   current_vector += 8;
   return vector;
#else
   vector = IRQ2VECTOR(irq);
   IRQVectorMap[irq] = vector;
   return vector;
#endif
}

/*
 * Find the IRQ entry number of a certain pin.
 */
static ULONG 
IOAPICGetIrqEntry(ULONG apic,
		  ULONG pin,
		  ULONG type)
{
   ULONG i;

   for (i = 0; i < IRQCount; i++)
   {
      if (IRQMap[i].IrqType == type &&
	  (IRQMap[i].DstApicId == IOAPICMap[apic].ApicId || IRQMap[i].DstApicId == MP_APIC_ALL) &&
	  IRQMap[i].DstApicInt == pin)
      {
         return i;
      }
   }
   return -1;
}


VOID 
IOAPICSetupIrqs(VOID)
{
   IOAPIC_ROUTE_ENTRY entry;
   ULONG apic, pin, idx, irq, first_notcon = 1, vector, trigger;

   DPRINT("Init IO_APIC IRQs\n");

   /* Setup IRQ to vector translation map */
   memset(&IRQVectorMap, 0, sizeof(IRQVectorMap));

   for (apic = 0; apic < IOAPICCount; apic++) 
   {
      for (pin = 0; pin < IOAPICMap[apic].EntryCount; pin++) 
      {
         /*
	  * add it to the IO-APIC irq-routing table
	  */
	 memset(&entry,0,sizeof(entry));

	 entry.delivery_mode = (APIC_DM_LOWEST >> 8);
	 entry.dest_mode = 1;  /* logical delivery */
	 entry.mask = 1;       /* disable IRQ */
         entry.dest.logical.logical_dest = 0;

	 idx = IOAPICGetIrqEntry(apic,pin,INT_VECTORED);
	 if (idx == (ULONG)-1)
	 {
	    if (first_notcon) 
	    {
	       DPRINT(" IO-APIC (apicid-pin) %d-%d\n", IOAPICMap[apic].ApicId, pin);
	       first_notcon = 0;
	    } 
	    else 
	    {
	       DPRINT(", %d-%d\n", IOAPICMap[apic].ApicId, pin);
            }
	    continue;
	 }

         trigger = IRQTrigger(idx);
	 entry.polarity = IRQPolarity(idx);

	 if (trigger) 
	 {
	    entry.trigger = 1;
	 }

	 irq = Pin2Irq(idx, apic, pin);

  	 vector = AssignIrqVector(irq);
	 entry.vector = vector;

	 DPRINT("vector 0x%.08x assigned to irq 0x%.02x\n", vector, irq);

         if (irq == 0)
         {
            /* Mask timer IRQ */
            entry.mask = 1;
         }

         if ((apic == 0) && (irq < 16))
	 {
	    Disable8259AIrq(irq);
	 }
         IOAPICWrite(apic, IOAPIC_REDTBL+2*pin+1, *(((PULONG)&entry)+1));
	 IOAPICWrite(apic, IOAPIC_REDTBL+2*pin, *(((PULONG)&entry)+0));

	 IrqApicMap[irq] = apic;

	 DPRINT("Vector %x, Pin %x, Irq %x\n", vector, pin, irq);
      }
   }
}

static VOID 
IOAPICClearPin(ULONG Apic, ULONG Pin)
{
   IOAPIC_ROUTE_ENTRY Entry;

   DPRINT("IOAPICClearPin(Apic %d, Pin %d\n", Apic, Pin);
   /*
    * Disable it in the IO-APIC irq-routing table
    */
   memset(&Entry, 0, sizeof(Entry));
   Entry.mask = 1;

   IOAPICWrite(Apic, IOAPIC_REDTBL + 2 * Pin, *(((PULONG)&Entry) + 0));
   IOAPICWrite(Apic, IOAPIC_REDTBL + 1 + 2 * Pin, *(((PULONG)&Entry) + 1));
}

static VOID 
IOAPICClear(ULONG Apic)
{
   ULONG Pin;

   for (Pin = 0; Pin < /*IOAPICMap[Apic].EntryCount*/24; Pin++)
   {
     IOAPICClearPin(Apic, Pin);
   }
}

static VOID 
IOAPICClearAll(VOID)
{
   ULONG Apic;

   for (Apic = 0; Apic < IOAPICCount; Apic++)
   {
      IOAPICClear(Apic);
   }
}

VOID 
IOAPICEnable(VOID)
{
   ULONG i, tmp;

   /* Setup IRQ to vector translation map */
   memset(&IRQVectorMap, 0, sizeof(IRQVectorMap));

   /*
    * The number of IO-APIC IRQ registers (== #pins):
    */
   for (i = 0; i < IOAPICCount; i++) 
   {
      tmp = IOAPICRead(i, IOAPIC_VER);
      IOAPICMap[i].EntryCount = GET_IOAPIC_MRE(tmp) + 1;
   }

   /*
    * Do not trust the IO-APIC being empty at bootup
    */
   IOAPICClearAll();
}

VOID 
IOAPICSetupIds(VOID)
{
  ULONG tmp, apic, i;
  UCHAR old_id;
  
  /*
   * Set the IOAPIC ID to the value stored in the MPC table.
   */
  for (apic = 0; apic < IOAPICCount; apic++) 
  {
    
    /* Read the register 0 value */
    tmp = IOAPICRead(apic, IOAPIC_ID);
    
    old_id = IOAPICMap[apic].ApicId;
    
    if (IOAPICMap[apic].ApicId >= 0xf) 
    {
      DPRINT1("BIOS bug, IO-APIC#%d ID is %d in the MPC table!...\n",
	      apic, IOAPICMap[apic].ApicId);
      DPRINT1("... fixing up to %d. (tell your hw vendor)\n", 
	      GET_IOAPIC_ID(tmp));
      IOAPICMap[apic].ApicId = GET_IOAPIC_ID(tmp);
    }
    
    /*
     * We need to adjust the IRQ routing table
     * if the ID changed.
     */
    if (old_id != IOAPICMap[apic].ApicId)
    {
      for (i = 0; i < IRQCount; i++)
      {
	if (IRQMap[i].DstApicId == old_id)
	{
	  IRQMap[i].DstApicId = IOAPICMap[apic].ApicId;
	}
      }
    }
    
    /*
     * Read the right value from the MPC table and
     * write it into the ID register.
     */
    DPRINT("Changing IO-APIC physical APIC ID to %d\n",
	   IOAPICMap[apic].ApicId);
    
    tmp &= ~IOAPIC_ID_MASK;
    tmp |= SET_IOAPIC_ID(IOAPICMap[apic].ApicId);

    IOAPICWrite(apic, IOAPIC_ID, tmp);
    
    /*
     * Sanity check
     */
    tmp = IOAPICRead(apic, 0);
    if (GET_IOAPIC_ID(tmp) != IOAPICMap[apic].ApicId) 
    {
      DPRINT1("Could not set I/O APIC ID!\n");
      KEBUGCHECK(0);
    }
  }
}

/* This is performance critical and should probably be done in assembler */
VOID IOAPICMaskIrq(ULONG Irq)
{
   IOAPIC_ROUTE_ENTRY Entry;
   ULONG Apic = IrqApicMap[Irq];

   *(((PULONG)&Entry)+0) = IOAPICRead(Apic, IOAPIC_REDTBL+2*Irq);
   *(((PULONG)&Entry)+1) = IOAPICRead(Apic, IOAPIC_REDTBL+2*Irq+1);
   Entry.dest.logical.logical_dest &= ~(1 << KeGetCurrentProcessorNumber());
   if (Entry.dest.logical.logical_dest == 0)
   {
      Entry.mask = 1;
   }
   IOAPICWrite(Apic, IOAPIC_REDTBL+2*Irq+1, *(((PULONG)&Entry)+1));
   IOAPICWrite(Apic, IOAPIC_REDTBL+2*Irq, *(((PULONG)&Entry)+0));
}

/* This is performance critical and should probably be done in assembler */
VOID IOAPICUnmaskIrq(ULONG Irq)
{
   IOAPIC_ROUTE_ENTRY Entry;
   ULONG Apic = IrqApicMap[Irq];

   *(((PULONG)&Entry)+0) = IOAPICRead(Apic, IOAPIC_REDTBL+2*Irq);
   *(((PULONG)&Entry)+1) = IOAPICRead(Apic, IOAPIC_REDTBL+2*Irq+1);
   Entry.dest.logical.logical_dest |= 1 << KeGetCurrentProcessorNumber();
   Entry.mask = 0;
   IOAPICWrite(Apic, IOAPIC_REDTBL+2*Irq+1, *(((PULONG)&Entry)+1));
   IOAPICWrite(Apic, IOAPIC_REDTBL+2*Irq, *(((PULONG)&Entry)+0));
}

VOID IOAPICDump(VOID)
{
   ULONG apic, i;
   ULONG reg0, reg1, reg2=0;

   DbgPrint("Number of MP IRQ sources: %d.\n", IRQCount);
   for (i = 0; i < IOAPICCount; i++) 
   {
      DbgPrint("Number of IO-APIC #%d registers: %d.\n",
	       IOAPICMap[i].ApicId,
               IOAPICMap[i].EntryCount);
   }

   /*
    * We are a bit conservative about what we expect.  We have to
    * know about every hardware change ASAP.
    */
   DbgPrint("Testing the IO APIC.......................\n");

   for (apic = 0; apic < IOAPICCount; apic++) 
   {
      reg0 = IOAPICRead(apic, IOAPIC_ID);
      reg1 = IOAPICRead(apic, IOAPIC_VER);
      if (GET_IOAPIC_VERSION(reg1) >= 0x10) 
      {
         reg2 = IOAPICRead(apic, IOAPIC_ARB);
      }

      DbgPrint("\n");
      DbgPrint("IO APIC #%d......\n", IOAPICMap[apic].ApicId);
      DbgPrint(".... register #00: %08X\n", reg0);
      DbgPrint(".......    : physical APIC id: %02X\n", GET_IOAPIC_ID(reg0));
      if (reg0 & 0xF0FFFFFF) 
      {
         DbgPrint("  WARNING: Unexpected IO-APIC\n");
      }

      DbgPrint(".... register #01: %08X\n", reg1);
      i = GET_IOAPIC_MRE(reg1);

      DbgPrint(".......     : max redirection entries: %04X\n", i);
      if ((i != 0x0f) &&    /* older (Neptune) boards */
	  (i != 0x17) &&    /* typical ISA+PCI boards */
	  (i != 0x1b) &&    /* Compaq Proliant boards */
	  (i != 0x1f) &&    /* dual Xeon boards */
          (i != 0x22) &&   /* bigger Xeon boards */
	  (i != 0x2E) &&
	  (i != 0x3F)) 
      {
         DbgPrint("  WARNING: Unexpected IO-APIC\n");
      }

      i = GET_IOAPIC_VERSION(reg1);
      DbgPrint(".......     : IO APIC version: %04X\n", i);
      if ((i != 0x01) &&    /* 82489DX IO-APICs */
	  (i != 0x10) &&    /* oldest IO-APICs */
	  (i != 0x11) &&    /* Pentium/Pro IO-APICs */
	  (i != 0x13))	    /* Xeon IO-APICs */
      {
         DbgPrint("  WARNING: Unexpected IO-APIC\n");
      }

      if (reg1 & 0xFF00FF00) 
      {
         DbgPrint("  WARNING: Unexpected IO-APIC\n");
      }

      if (GET_IOAPIC_VERSION(reg1) >= 0x10) 
      {
	 DbgPrint(".... register #02: %08X\n", reg2);
	 DbgPrint(".......     : arbitration: %02X\n",
	          GET_IOAPIC_ARB(reg2));
  	 if (reg2 & 0xF0FFFFFF) 
	 {
            DbgPrint("  WARNING: Unexpected IO-APIC\n");
         }
      }

      DbgPrint(".... IRQ redirection table:\n");
      DbgPrint(" NR Log Phy Mask Trig IRR Pol"
	       " Stat Dest Deli Vect:   \n");

      for (i = 0; i <= GET_IOAPIC_MRE(reg1); i++) 
      {
         IOAPIC_ROUTE_ENTRY entry;

	 *(((PULONG)&entry)+0) = IOAPICRead(apic, 0x10+i*2);
	 *(((PULONG)&entry)+1) = IOAPICRead(apic, 0x11+i*2);

	 DbgPrint(" %02x %03X %02X  ",
		  i,
		  entry.dest.logical.logical_dest,
		  entry.dest.physical.physical_dest);

         DbgPrint("%C    %C    %1d  %C    %C    %C     %03X    %02X\n",
		  (entry.mask == 0) ? 'U' : 'M',            // Unmasked/masked
		  (entry.trigger == 0) ? 'E' : 'L',         // Edge/level sensitive
		  entry.irr,
		  (entry.polarity == 0) ? 'H' : 'L',        // Active high/active low
		  (entry.delivery_status == 0) ? 'I' : 'S', // Idle / send pending
		  (entry.dest_mode == 0) ? 'P' : 'L',       // Physical logical
		  entry.delivery_mode,
		  entry.vector);
      }
   }

   DbgPrint(".................................... done.\n");
}

VOID
HaliReconfigurePciInterrupts(VOID)
{
   ULONG i;

   for (i = 0; i < IRQCount; i++)
   {
      if (BUSMap[IRQMap[i].SrcBusId] == MP_BUS_PCI)
      {
         DPRINT("%02x: IrqType %02x, IrqFlag %02x, SrcBusId %02x, SrcBusIrq %02x"
	        ", DstApicId %02x, DstApicInt %02x\n",
	        i, IRQMap[i].IrqType, IRQMap[i].IrqFlag, IRQMap[i].SrcBusId, 
	        IRQMap[i].SrcBusIrq, IRQMap[i].DstApicId, IRQMap[i].DstApicInt);

	 if(1 != HalSetBusDataByOffset(PCIConfiguration, 
	                               IRQMap[i].SrcBusId, 
				       (IRQMap[i].SrcBusIrq >> 2) & 0x1f, 
				       &IRQMap[i].DstApicInt, 
				       0x3c /*PCI_INTERRUPT_LINE*/, 
				       1))
	 {
	    CHECKPOINT;
	 }
      }
   }
}

VOID Disable8259AIrq(ULONG irq)
{
    ULONG tmp;

    if (irq >= 8) 
    {
       tmp = READ_PORT_UCHAR((PUCHAR)0xA1);
       tmp |= (1 << (irq - 8));
       WRITE_PORT_UCHAR((PUCHAR)0xA1, tmp);
    } 
    else 
    {
       tmp = READ_PORT_UCHAR((PUCHAR)0x21);
       tmp |= (1 << irq);
       WRITE_PORT_UCHAR((PUCHAR)0x21, tmp);
    }
}

ULONG IOAPICRead(ULONG Apic, ULONG Offset)
{
  PULONG Base;

  Base = (PULONG)IOAPICMap[Apic].ApicAddress;
  *Base = Offset;
  return *((PULONG)((ULONG)Base + IOAPIC_IOWIN));
}

VOID IOAPICWrite(ULONG Apic, ULONG Offset, ULONG Value)
{
  PULONG Base;

  Base = (PULONG)IOAPICMap[Apic].ApicAddress;
  *Base = Offset;
  *((PULONG)((ULONG)Base + IOAPIC_IOWIN)) = Value;
}

/* EOF */
