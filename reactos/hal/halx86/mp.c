/* $Id: mp.c,v 1.12 2004/11/28 01:30:01 hbirr Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/hal/x86/mp.c
 * PURPOSE:         Intel MultiProcessor specification support
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 *                  Casper S. Hornstrup (chorns@users.sourceforge.net)
 * NOTES:           Parts adapted from linux SMP code
 * UPDATE HISTORY:
 *     22/05/1998  DW   Created
 *     12/04/2001  CSH  Added MultiProcessor specification support
 */

/* INCLUDES *****************************************************************/

#include <roscfg.h>
#include <ddk/ntddk.h>


#ifdef MP

#include <hal.h>
#include <halirq.h>
#include <mps.h>
#include <apic.h>

#include <internal/ntoskrnl.h>
#include <internal/i386/segment.h>
#include <internal/ke.h>
#include <internal/ps.h>

#endif

#define NDEBUG
#include <internal/debug.h>

#ifdef MP


/*
   Address of area to be used for communication between Application
   Processors (APs) and the BootStrap Processor (BSP)
 */
#define COMMON_AREA  0x2000

#define BIOS_AREA    0x0

typedef struct __attribute__((packed)) _COMMON_AREA_INFO
{
   ULONG Stack;      /* Location of AP stack */
   ULONG Debug[16];  /* For debugging */
} COMMON_AREA_INFO, *PCOMMON_AREA_INFO;

CPU_INFO CPUMap[MAX_CPU];          /* Map of all CPUs in the system */
ULONG CPUCount;                    /* Total number of CPUs */
ULONG OnlineCPUs;                  /* Bitmask of online CPUs */

UCHAR BUSMap[MAX_BUS];             /* Map of all buses in the system */
UCHAR PCIBUSMap[MAX_BUS];          /* Map of all PCI buses in the system */

IOAPIC_INFO IOAPICMap[MAX_IOAPIC]; /* Map of all I/O APICs in the system */
ULONG IOAPICCount;                 /* Number of I/O APICs in the system */

MP_CONFIGURATION_INTSRC IRQMap[MAX_IRQ_SOURCE]; /* Map of all IRQs */
ULONG IRQVectorMap[MAX_IRQ_SOURCE];             /* IRQ to vector map */
ULONG IrqPinMap[MAX_IRQ_SOURCE];		/* IRQ to Pin map */
ULONG IrqApicMap[MAX_IRQ_SOURCE];
ULONG IRQCount;                                 /* Number of IRQs  */

ULONG APICMode;                     /* APIC mode at startup */
ULONG BootCPU;                      /* Bootstrap processor */
PULONG BIOSBase;                    /* Virtual address of BIOS data segment */
PULONG CommonBase;                  /* Virtual address of common area */

extern CHAR *APstart, *APend;
extern VOID (*APflush)(VOID);

extern VOID MpsTimerInterrupt(VOID);
extern VOID MpsErrorInterrupt(VOID);
extern VOID MpsSpuriousInterrupt(VOID);
extern VOID MpsIpiInterrupt(VOID);

#define CMOS_READ(address) ({ \
   WRITE_PORT_UCHAR((PUCHAR)0x70, address)); \
   READ_PORT_UCHAR((PUCHAR)0x71)); \
})

#define CMOS_WRITE(address, value) ({ \
   WRITE_PORT_UCHAR((PUCHAR)0x70, address); \
   WRITE_PORT_UCHAR((PUCHAR)0x71, value); \
})

#endif /* MP */



/* FUNCTIONS *****************************************************************/

#ifdef MP

/* Functions for handling 8259A PICs */

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


VOID Enable8259AIrq(ULONG irq)
{
    ULONG tmp;

    if (irq >= 8) 
    {
       tmp = READ_PORT_UCHAR((PUCHAR)0xA1);
       tmp &= ~(1 << (irq - 8));
       WRITE_PORT_UCHAR((PUCHAR)0xA1, tmp);
    } 
    else 
    {
       tmp = READ_PORT_UCHAR((PUCHAR)0x21);
       tmp &= ~(1 << irq);
       WRITE_PORT_UCHAR((PUCHAR)0x21, tmp);
    }
}


/* Functions for handling I/O APICs */

volatile ULONG IOAPICRead(ULONG Apic, ULONG Offset)
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


VOID IOAPICClearPin(ULONG Apic, ULONG Pin)
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

static VOID IOAPICClear(ULONG Apic)
{
  ULONG Pin;

  for (Pin = 0; Pin < /*IOAPICMap[Apic].EntryCount*/24; Pin++)
  {
    IOAPICClearPin(Apic, Pin);
  }
}

static VOID IOAPICClearAll(VOID)
{
  ULONG Apic;

  for (Apic = 0; Apic < IOAPICCount; Apic++)
  {
    IOAPICClear(Apic);
  }
}

/* This is performance critical and should probably be done in assembler */
VOID IOAPICMaskIrq(ULONG Irq)
{
  IOAPIC_ROUTE_ENTRY Entry;
  ULONG Apic = IrqApicMap[Irq];


  *((PULONG)&Entry) = IOAPICRead(Apic, IOAPIC_REDTBL+2*Irq);
  Entry.mask = 1;
  IOAPICWrite(Apic, IOAPIC_REDTBL+2*Irq, *((PULONG)&Entry));
}


/* This is performance critical and should probably be done in assembler */
VOID IOAPICUnmaskIrq(ULONG Irq)
{
  IOAPIC_ROUTE_ENTRY Entry;
  ULONG Apic = IrqApicMap[Irq];

  *((PULONG)&Entry) = IOAPICRead(Apic, IOAPIC_REDTBL+2*IrqPinMap[Irq]);
  Entry.mask = 0;
  IOAPICWrite(Apic, IOAPIC_REDTBL+2*IrqPinMap[Irq], *((PULONG)&Entry));
}

static VOID 
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

static ULONG IRQPolarity(ULONG idx)
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
				{
					polarity = default_ISA_polarity(idx);
					break;
				}
				case MP_BUS_EISA: /* EISA pin */
				{
					polarity = default_EISA_polarity(idx);
					break;
				}
				case MP_BUS_PCI: /* PCI pin */
				{
					polarity = default_PCI_polarity(idx);
					break;
				}
				case MP_BUS_MCA: /* MCA pin */
				{
					polarity = default_MCA_polarity(idx);
					break;
				}
				default:
				{
					DPRINT("Broken BIOS!!\n");
					polarity = 1;
					break;
				}
			}
			break;
		}
		case 1: /* high active */
		{
			polarity = 0;
			break;
		}
		case 2: /* reserved */
		{
			DPRINT("Broken BIOS!!\n");
			polarity = 1;
			break;
		}
		case 3: /* low active */
		{
			polarity = 1;
			break;
		}
		default: /* invalid */
		{
			DPRINT("Broken BIOS!!\n");
			polarity = 1;
			break;
		}
	}
	return polarity;
}

static ULONG IRQTrigger(ULONG idx)
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
				{
					trigger = default_ISA_trigger(idx);
					break;
				}
				case MP_BUS_EISA: /* EISA pin */
				{
					trigger = default_EISA_trigger(idx);
					break;
				}
				case MP_BUS_PCI: /* PCI pin */
				{
					trigger = default_PCI_trigger(idx);
					break;
				}
				case MP_BUS_MCA: /* MCA pin */
				{
					trigger = default_MCA_trigger(idx);
					break;
				}
				default:
				{
					DPRINT("Broken BIOS!!\n");
					trigger = 1;
					break;
				}
			}
			break;
		}
		case 1: /* edge */
		{
			trigger = 0;
			break;
		}
		case 2: /* reserved */
		{
			DPRINT("Broken BIOS!!\n");
			trigger = 1;
			break;
		}
		case 3: /* level */
		{
			trigger = 1;
			break;
		}
		default: /* invalid */
		{
			DPRINT("Broken BIOS!!\n");
			trigger = 0;
			break;
		}
	}
	return trigger;
}


static ULONG Pin2Irq(ULONG idx,
		     ULONG apic,
		     ULONG pin)
{
	ULONG irq, i;
	ULONG bus = IRQMap[idx].SrcBusId;

	/*
	 * Debugging check, we are in big trouble if this message pops up!
	 */
	if (IRQMap[idx].DstApicInt != pin) {
		DPRINT("broken BIOS or MPTABLE parser, ayiee!!\n");
  }

	switch (BUSMap[bus])
	{
		case MP_BUS_ISA: /* ISA pin */
		case MP_BUS_EISA:
		case MP_BUS_MCA:
		{
			irq = IRQMap[idx].SrcBusIrq;
			break;
		}
		case MP_BUS_PCI: /* PCI pin */
		{
			/*
			 * PCI IRQs are mapped in order
			 */
			i = irq = 0;
			while (i < apic)
				irq += IOAPICMap[i++].EntryCount;
			irq += pin;
			break;
		}
		default:
		{
			DPRINT("Unknown bus type %d.\n",bus);
			irq = 0;
			break;
		}
	}

	return irq;
}


/*
 * Rough estimation of how many shared IRQs there are, can
 * be changed anytime.
 */
#define MAX_PLUS_SHARED_IRQS PIC_IRQS
#define PIN_MAP_SIZE (MAX_PLUS_SHARED_IRQS + PIC_IRQS)

/*
 * This is performance-critical, we want to do it O(1)
 *
 * the indexing order of this array favors 1:1 mappings
 * between pins and IRQs.
 */

static struct irq_pin_list {
	ULONG apic, pin, next;
} irq_2_pin[PIN_MAP_SIZE];

/*
 * The common case is 1:1 IRQ<->pin mappings. Sometimes there are
 * shared ISA-space IRQs, so we have to support them. We are super
 * fast in the common case, and fast for shared ISA-space IRQs.
 */
static VOID AddPinToIrq(ULONG irq,
			ULONG apic,
			ULONG pin)
{
   static ULONG first_free_entry = PIC_IRQS;
   struct irq_pin_list *entry = irq_2_pin + irq;

   while (entry->next)
   {
      entry = irq_2_pin + entry->next;
   }
   
   if (entry->pin != -1) 
   {
      entry->next = first_free_entry;
      entry = irq_2_pin + entry->next;
      if (++first_free_entry >= PIN_MAP_SIZE) 
      {
         DPRINT1("Ohh no!");
	 KEBUGCHECK(0);
      }
   }
   entry->apic = apic;
   entry->pin = pin;
}


/*
 * Find the IRQ entry number of a certain pin.
 */
static ULONG IOAPICGetIrqEntry(ULONG apic,
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


static ULONG AssignIrqVector(ULONG irq)
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
  if (current_vector > FIRST_SYSTEM_VECTOR) {
      vector_offset++;
	  current_vector = FIRST_DEVICE_VECTOR + vector_offset;
  } else if (current_vector == FIRST_SYSTEM_VECTOR) {
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


VOID IOAPICSetupIrqs(VOID)
{
   IOAPIC_ROUTE_ENTRY entry;
   ULONG apic, pin, idx, irq, first_notcon = 1, vector;

   DPRINT("Init IO_APIC IRQs\n");

   for (apic = 0; apic < IOAPICCount; apic++) 
   {
      for (pin = 0; pin < IOAPICMap[apic].EntryCount; pin++) 
      {
         /*
	  * add it to the IO-APIC irq-routing table
	  */
	 memset(&entry,0,sizeof(entry));

	 entry.delivery_mode = APIC_DM_LOWEST;
	 entry.dest_mode = 1;  /* logical delivery */
	 entry.mask = 1;       /* disable IRQ */
#if 0
	 /*
	  * FIXME:
	  *   Some drivers are not able to deal with more than one cpu.
	  */
	 entry.dest.logical.logical_dest = OnlineCPUs;
#else
	 entry.dest.logical.logical_dest = 1 << 0;
#endif
	 idx = IOAPICGetIrqEntry(apic,pin,INT_VECTORED);
	 if (idx == -1) 
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

         entry.trigger = IRQTrigger(idx);
	 entry.polarity = IRQPolarity(idx);

	 if (entry.trigger) 
	 {
	    entry.trigger = 1;
	    entry.mask = 1; // disable
#if 0
	    entry.dest.logical.logical_dest = OnlineCPUs;
#else
	    entry.dest.logical.logical_dest = 1 << 0;
#endif
	 }

	 irq = Pin2Irq(idx, apic, pin);
	 AddPinToIrq(irq, apic, pin);

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

	 IrqPinMap[irq] = pin;
	 IrqApicMap[irq] = apic;

	 DPRINT("Vector %x, Pin %x, Irq %x\n", vector, pin, irq);
      }
   }
}


static VOID IOAPICEnable(VOID)
{
   ULONG i, tmp;

   for (i = 0; i < PIN_MAP_SIZE; i++) 
   {
      irq_2_pin[i].pin = -1;
      irq_2_pin[i].next = 0;
   }

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

#if 0
static VOID IOAPICDisable(VOID)
{
   /*
    * Clear the IO-APIC before rebooting
    */
   IOAPICClearAll();
   APICDisable();
}
#endif


static VOID IOAPICSetup(VOID)
{
  IOAPICEnable();
  IOAPICSetupIds();
  APICSyncArbIDs();
  IOAPICSetupIrqs();
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
   DbgPrint("IRQ to pin mappings:\n");
   for (i = 0; i < PIC_IRQS; i++) 
   {
      struct irq_pin_list *entry = irq_2_pin + i;
      if (entry->pin < 0)
      {
         continue;
      }
      DbgPrint("IRQ%d ", i);
      for (;;) 
      {
	 DbgPrint("-> %d", entry->pin);
	 if (!entry->next)
	 {
            break;
	 }
	 entry = irq_2_pin + entry->next;
      }
      if (i % 2) 
      {
         DbgPrint("\n");
      } 
      else 
      {
         DbgPrint("        ");
      }
   }

   DbgPrint(".................................... done.\n");
}



/* Functions for handling local APICs */

ULONG Read8254Timer(VOID)
{
	ULONG Count;

	WRITE_PORT_UCHAR((PUCHAR)0x43, 0x00);
	Count = READ_PORT_UCHAR((PUCHAR)0x40);
	Count |= READ_PORT_UCHAR((PUCHAR)0x40) << 8;

	return Count;
}

VOID WaitFor8254Wraparound(VOID)
{
   ULONG CurCount, PrevCount = ~0;
   LONG Delta;

   CurCount = Read8254Timer();
   do 
   {
      PrevCount = CurCount;
      CurCount = Read8254Timer();
      Delta = CurCount - PrevCount;

      /*
       * This limit for delta seems arbitrary, but it isn't, it's
       * slightly above the level of error a buggy Mercury/Neptune
       * chipset timer can cause.
       */

   } 
   while (Delta < 300);
}

#define HZ (100)
#define APIC_DIVISOR (16)

VOID APICSetupLVTT(ULONG ClockTicks)
{
   ULONG tmp;

   tmp = GET_APIC_VERSION(APICRead(APIC_VER));
   if (!APIC_INTEGRATED(tmp))
   {
      tmp = SET_APIC_TIMER_BASE(APIC_TIMER_BASE_DIV) | APIC_LVT_PERIODIC | LOCAL_TIMER_VECTOR;;
   }
   else
   {
      /* Periodic timer */
      tmp = APIC_LVT_PERIODIC | LOCAL_TIMER_VECTOR;;
   }
   APICWrite(APIC_LVTT, tmp);

   tmp = APICRead(APIC_TDCR);
   tmp &= ~(APIC_TDCR_1 | APIC_TIMER_BASE_DIV);
   tmp |= APIC_TDCR_16;
   APICWrite(APIC_TDCR, tmp);
   APICWrite(APIC_ICRT, ClockTicks / APIC_DIVISOR);
}


VOID APICCalibrateTimer(ULONG CPU)
{
   ULARGE_INTEGER t1, t2;
   LONG tt1, tt2;

   DPRINT("Calibrating APIC timer for CPU %d\n", CPU);

   APICSetupLVTT(1000000000);

   /*
    * The timer chip counts down to zero. Let's wait
    * for a wraparound to start exact measurement:
    * (the current tick might have been already half done)
    */
   WaitFor8254Wraparound();

   /*
    * We wrapped around just now. Let's start
    */
   ReadPentiumClock(&t1);
   tt1 = APICRead(APIC_CCRT);

   WaitFor8254Wraparound();


   tt2 = APICRead(APIC_CCRT);
   ReadPentiumClock(&t2);

   CPUMap[CPU].BusSpeed = (HZ * (long)(tt1 - tt2) * APIC_DIVISOR);
   CPUMap[CPU].CoreSpeed = (HZ * (t2.QuadPart - t1.QuadPart));

   /* Setup timer for normal operation */
// APICSetupLVTT((CPUMap[CPU].BusSpeed / 1000000) * 100);    // 100ns
   APICSetupLVTT((CPUMap[CPU].BusSpeed / 1000000) * 10000);  // 10ms
// APICSetupLVTT((CPUMap[CPU].BusSpeed / 1000000) * 100000); // 100ms

   DPRINT("CPU clock speed is %ld.%04ld MHz.\n",
	  CPUMap[CPU].CoreSpeed/1000000,
	  CPUMap[CPU].CoreSpeed%1000000);

   DPRINT("Host bus clock speed is %ld.%04ld MHz.\n",
	  CPUMap[CPU].BusSpeed/1000000,
	  CPUMap[CPU].BusSpeed%1000000);
}

VOID 
SetInterruptGate(ULONG index, ULONG address)
{
  IDT_DESCRIPTOR *idt;

  idt = (IDT_DESCRIPTOR*)((ULONG)KeGetCurrentKPCR()->IDT + index * sizeof(IDT_DESCRIPTOR));
  idt->a = (((ULONG)address)&0xffff) + (KERNEL_CS << 16);
  idt->b = 0x8e00 + (((ULONG)address)&0xffff0000);
}

#endif /* MP */

VOID STDCALL
HalInitializeProcessor(ULONG ProcessorNumber,
		       PVOID /*PLOADER_PARAMETER_BLOCK*/ LoaderBlock)
{
#ifdef MP
   ULONG CPU;
#endif

   DPRINT("HalInitializeProcessor(%x %x)\n", ProcessorNumber, LoaderBlock);

#ifdef MP
   CPU = ThisCPU();
   if (OnlineCPUs & (1 << CPU))
   {
      KEBUGCHECK(0);
   }

   if (ProcessorNumber == 0)
   {
      HaliInitBSP();
   }
   else
   {
      APICSetup();

      DPRINT("CPU %d says it is now booted.\n", CPU);
 
      APICCalibrateTimer(CPU);
   }

   /* This processor is now booted */
   CPUMap[CPU].Flags |= CPU_ENABLED;
   OnlineCPUs |= (1 << CPU);

   /* Setup busy waiting */
   HalpCalibrateStallExecution();
#endif
}

BOOLEAN STDCALL
HalAllProcessorsStarted (VOID)
{
    DPRINT("HalAllProcessorsStarted()\n");

#ifdef MP

    ULONG CPUs = 0, i;
    for (i = 0; i < 32; i++)
    {
       if (OnlineCPUs & (1 << i))
       {
          CPUs++;
       }
    }
    if (CPUs > CPUCount)
    {
       KEBUGCHECK(0);
    }
    else if (CPUs == CPUCount)
    {
       IOAPICSetup();
       return TRUE;
    }
    return FALSE;

#else /* MP */

    return TRUE;

#endif /* MP */
}

BOOLEAN STDCALL 
HalStartNextProcessor(ULONG Unknown1,
		      ULONG ProcessorStack)
{
#ifdef MP
   PCOMMON_AREA_INFO Common;
   ULONG StartupCount;
   ULONG DeliveryStatus = 0;
   ULONG AcceptStatus = 0;
   ULONG CPU, i, j;
   ULONG tmp, maxlvt;

   DPRINT("HalStartNextProcessor(%x %x)\n", Unknown1, ProcessorStack);

   for (CPU = 0; CPU < CPUCount; CPU++)
   {
      if (!(OnlineCPUs & (1<<CPU)))
      {
	 break;
      }
   }

   if (CPU >= CPUCount)
   {
      KEBUGCHECK(0);
   }

   DPRINT1("Attempting to boot CPU %d\n", CPU);

   /* Send INIT IPI */

   APICSendIPI(CPU, APIC_DM_INIT|APIC_ICR0_LEVEL_ASSERT);
 
   KeStallExecutionProcessor(200);

   /* Deassert INIT */

   APICSendIPI(CPU, APIC_DM_INIT|APIC_ICR0_LEVEL_DEASSERT);

   if (APIC_INTEGRATED(CPUMap[CPU].APICVersion)) 
   {
      /* Clear APIC errors */
      APICWrite(APIC_ESR, 0);
      tmp = (APICRead(APIC_ESR) & APIC_ESR_MASK);
   }

   Common = (PCOMMON_AREA_INFO)CommonBase;

   /* Write the location of the AP stack */
   Common->Stack = (ULONG)ProcessorStack;

   DPRINT("CPU %d got stack at 0x%X\n", CPU, Common->Stack);
#if 0
   for (j = 0; j < 16; j++) 
   {
      Common->Debug[j] = 0;
   }
#endif

   maxlvt = APICGetMaxLVT();

   /* Is this a local APIC or an 82489DX? */
   StartupCount = (APIC_INTEGRATED(CPUMap[CPU].APICVersion)) ? 2 : 0;

   for (i = 1; i <= StartupCount; i++)
   {
      /* It's a local APIC, so send STARTUP IPI */
      DPRINT("Sending startup signal %d\n", i);
      /* Clear errors */
      APICWrite(APIC_ESR, 0);
      APICRead(APIC_ESR);

      APICSendIPI(CPU, APIC_DM_STARTUP | ((COMMON_AREA + PAGE_SIZE) >> 12)|APIC_ICR0_LEVEL_DEASSERT);

      /* Wait up to 10ms for IPI to be delivered */
      j = 0;
      do 
      {
         KeStallExecutionProcessor(10);

         /* Check Delivery Status */
         DeliveryStatus = APICRead(APIC_ICR0) & APIC_ICR0_DS;

         j++;
      } while ((DeliveryStatus) && (j < 1000));

      KeStallExecutionProcessor(200);

      /*
       * Due to the Pentium erratum 3AP.
       */
      if (maxlvt > 3) 
      {
        APICRead(APIC_SIVR);
        APICWrite(APIC_ESR, 0);
      }

      AcceptStatus = APICRead(APIC_ESR) & APIC_ESR_MASK;

      if (DeliveryStatus || AcceptStatus) 
      {
         break;
      }
   }

   if (DeliveryStatus) 
   {
      DPRINT("STARTUP IPI for CPU %d was never delivered.\n", CPU);
   }

   if (AcceptStatus) 
   {
      DPRINT("STARTUP IPI for CPU %d was never accepted.\n", CPU);
   }

   if (!(DeliveryStatus || AcceptStatus)) 
   {

      /* Wait no more than 5 seconds for processor to boot */
      DPRINT("Waiting for 5 seconds for CPU %d to boot\n", CPU);

      /* Wait no more than 5 seconds */
      for (j = 0; j < 50000; j++) 
      {
         if (CPUMap[CPU].Flags & CPU_ENABLED)
	 {
            break;
	 }
         KeStallExecutionProcessor(100);
      }
   }

   if (CPUMap[CPU].Flags & CPU_ENABLED) 
   {
      DbgPrint("CPU %d is now running\n", CPU);
   } 
   else 
   {
      DbgPrint("Initialization of CPU %d failed\n", CPU);
   }

#if 0
   DPRINT("Debug bytes are:\n");

   for (j = 0; j < 4; j++) 
   {
      DPRINT("0x%08X 0x%08X 0x%08X 0x%08X.\n",
             Common->Debug[j*4+0],
             Common->Debug[j*4+1],
             Common->Debug[j*4+2],
             Common->Debug[j*4+3]);
   }
#endif

#endif /* MP */
   return TRUE;
}


#ifdef MP

ULONG MPChecksum(PUCHAR Base,
                 ULONG Size)
/*
 *	Checksum an MP configuration block
 */
{
   ULONG Sum = 0;

   while (Size--)
      Sum += *Base++;

   return((UCHAR)Sum);
}


PCHAR HaliMPFamily(ULONG Family,
		   ULONG Model)
{
   static CHAR str[32];
   static PCHAR CPUs[] =
   {
      "80486DX", "80486DX",
      "80486SX", "80486DX/2 or 80487",
      "80486SL", "Intel5X2(tm)",
      "Unknown", "Unknown",
      "80486DX/4"
   };
   if (Family == 0x6)
      return ("Pentium(tm) Pro");
   if (Family == 0x5)
      return ("Pentium(tm)");
   if (Family == 0x0F && Model == 0x0F)
      return("Special controller");
   if (Family == 0x0F && Model == 0x00)
      return("Pentium 4(tm)");
   if (Family == 0x04 && Model < 9)
      return CPUs[Model];
   sprintf(str, "Unknown CPU with family ID %ld and model ID %ld", Family, Model);
   return str;
}

static VOID HaliMPProcessorInfo(PMP_CONFIGURATION_PROCESSOR m)
{
  ULONG ver;

  if (!(m->CpuFlags & CPU_FLAG_ENABLED))
    return;

  DPRINT("Processor #%d %s APIC version %d\n",
         m->ApicId,
         HaliMPFamily((m->FeatureFlags & CPU_FAMILY_MASK) >> 8,
                      (m->FeatureFlags & CPU_MODEL_MASK) >> 4),
         m->ApicVersion);

  if (m->FeatureFlags & (1 << 0))
    DPRINT("    Floating point unit present.\n");
  if (m->FeatureFlags & (1 << 7))
    DPRINT("    Machine Exception supported.\n");
  if (m->FeatureFlags & (1 << 8))
    DPRINT("    64 bit compare & exchange supported.\n");
  if (m->FeatureFlags & (1 << 9))
    DPRINT("    Internal APIC present.\n");
  if (m->FeatureFlags & (1 << 11))
    DPRINT("    SEP present.\n");
  if (m->FeatureFlags & (1 << 12))
    DPRINT("    MTRR present.\n");
  if (m->FeatureFlags & (1 << 13))
    DPRINT("    PGE  present.\n");
  if (m->FeatureFlags & (1 << 14))
    DPRINT("    MCA  present.\n");
  if (m->FeatureFlags & (1 << 15))
    DPRINT("    CMOV  present.\n");
  if (m->FeatureFlags & (1 << 16))
    DPRINT("    PAT  present.\n");
  if (m->FeatureFlags & (1 << 17))
    DPRINT("    PSE  present.\n");
  if (m->FeatureFlags & (1 << 18))
    DPRINT("    PSN  present.\n");
  if (m->FeatureFlags & (1 << 19))
    DPRINT("    Cache Line Flush Instruction present.\n");
  /* 20 Reserved */
  if (m->FeatureFlags & (1 << 21))
    DPRINT("    Debug Trace and EMON Store present.\n");
  if (m->FeatureFlags & (1 << 22))
    DPRINT("    ACPI Thermal Throttle Registers present.\n");
  if (m->FeatureFlags & (1 << 23))
    DPRINT("    MMX  present.\n");
  if (m->FeatureFlags & (1 << 24))
    DPRINT("    FXSR  present.\n");
  if (m->FeatureFlags & (1 << 25))
    DPRINT("    XMM  present.\n");
  if (m->FeatureFlags & (1 << 26))
    DPRINT("    Willamette New Instructions present.\n");
  if (m->FeatureFlags & (1 << 27))
    DPRINT("    Self Snoop present.\n");
  /* 28 Reserved */
  if (m->FeatureFlags & (1 << 29))
    DPRINT("    Thermal Monitor present.\n");
  /* 30, 31 Reserved */

  CPUMap[CPUCount].APICId = m->ApicId;

  CPUMap[CPUCount].Flags = CPU_USABLE;

  if (m->CpuFlags & CPU_FLAG_BSP) 
  {
    DPRINT("    Bootup CPU\n");
    CPUMap[CPUCount].Flags |= CPU_BSP;
    BootCPU = m->ApicId;
  }

  if (m->ApicId > MAX_CPU) 
  {
    DPRINT("Processor #%d INVALID. (Max ID: %d).\n", m->ApicId, MAX_CPU);
    return;
  }
  ver = m->ApicVersion;

  /*
  * Validate version
  */
  if (ver == 0x0) 
  {
    DPRINT("BIOS bug, APIC version is 0 for CPU#%d! fixing up to 0x10. (tell your hw vendor)\n", m->ApicId);
    ver = 0x10;
  }
  CPUMap[CPUCount].APICVersion = ver;
  
  CPUCount++;
}

static VOID HaliMPBusInfo(PMP_CONFIGURATION_BUS m)
{
  static ULONG CurrentPCIBusId = 0;
  CHAR str[7];

  memcpy(str, m->BusType, 6);
  str[6] = 0;
  DPRINT("Bus #%d is %s\n", m->BusId, str);

  if (strncmp(str, BUSTYPE_ISA, sizeof(BUSTYPE_ISA)-1) == 0) 
  {
     BUSMap[m->BusId] = MP_BUS_ISA;
  } 
  else if (strncmp(str, BUSTYPE_EISA, sizeof(BUSTYPE_EISA)-1) == 0) 
  {
     BUSMap[m->BusId] = MP_BUS_EISA;
  } 
  else if (strncmp(str, BUSTYPE_PCI, sizeof(BUSTYPE_PCI)-1) == 0) 
  {
     BUSMap[m->BusId] = MP_BUS_PCI;
     PCIBUSMap[m->BusId] = CurrentPCIBusId;
     CurrentPCIBusId++;
  } 
  else if (strncmp(str, BUSTYPE_MCA, sizeof(BUSTYPE_MCA)-1) == 0) 
  {
     BUSMap[m->BusId] = MP_BUS_MCA;
  } 
  else 
  {
     DPRINT("Unknown bustype %s - ignoring\n", str);
  }
}

static VOID HaliMPIOApicInfo(PMP_CONFIGURATION_IOAPIC m)
{
  if (!(m->ApicFlags & CPU_FLAG_ENABLED))
    return;

  DPRINT("I/O APIC #%d Version %d at 0x%lX.\n",
         m->ApicId, m->ApicVersion, m->ApicAddress);
  if (IOAPICCount > MAX_IOAPIC) 
  {
    DPRINT("Max # of I/O APICs (%d) exceeded (found %d).\n",
           MAX_IOAPIC, IOAPICCount);
    DPRINT1("Recompile with bigger MAX_IOAPIC!.\n");
    KEBUGCHECK(0);
  }
  IOAPICMap[IOAPICCount].ApicId = m->ApicId;
  IOAPICMap[IOAPICCount].ApicVersion = m->ApicVersion;
  IOAPICMap[IOAPICCount].ApicAddress = m->ApicAddress;
  IOAPICCount++;
}

static VOID HaliMPIntSrcInfo(PMP_CONFIGURATION_INTSRC m)
{
  DPRINT("Int: type %d, pol %d, trig %d, bus %d,"
         " IRQ %02x, APIC ID %x, APIC INT %02x\n",
         m->IrqType, m->IrqFlag & 3,
         (m->IrqFlag >> 2) & 3, m->SrcBusId,
         m->SrcBusIrq, m->DstApicId, m->DstApicInt);
  if (IRQCount > MAX_IRQ_SOURCE) 
  {
    DPRINT1("Max # of irq sources exceeded!!\n");
    KEBUGCHECK(0);
  }

  IRQMap[IRQCount] = *m;
  IRQCount++;
}

static VOID HaliMPIntLocalInfo(PMP_CONFIGURATION_INTLOCAL m)
{
  DPRINT("Lint: type %d, pol %d, trig %d, bus %d,"
         " IRQ %02x, APIC ID %x, APIC LINT %02x\n",
         m->IrqType, m->IrqFlag & 3,
         (m->IrqFlag >> 2) & 3, m->SrcBusId,
          m->SrcBusIrq, m->DstApicId, m->DstApicLInt);
  /*
   * Well it seems all SMP boards in existence
   * use ExtINT/LVT1 == LINT0 and
   * NMI/LVT2 == LINT1 - the following check
   * will show us if this assumptions is false.
   * Until then we do not have to add baggage.
   */
  if ((m->IrqType == INT_EXTINT) && (m->DstApicLInt != 0)) 
  {
    DPRINT1("Invalid MP table!\n");
    KEBUGCHECK(0);
  }
  if ((m->IrqType == INT_NMI) && (m->DstApicLInt != 1)) 
  {
    DPRINT1("Invalid MP table!\n");
    KEBUGCHECK(0);
  }
}

VOID
HaliReadMPConfigTable(PMP_CONFIGURATION_TABLE Table)
/*
   PARAMETERS:
      Table = Pointer to MP configuration table
 */
{
   PUCHAR Entry;
   ULONG Count;

   if (Table->Signature != MPC_SIGNATURE)
     {
       PUCHAR pc = (PUCHAR)&Table->Signature;
       
       DbgPrint("Bad MP configuration block signature: %c%c%c%c\n", 
		pc[0], pc[1], pc[2], pc[3]);
       KEBUGCHECK(0);
       return;
     }

   if (MPChecksum((PUCHAR)Table, Table->Length))
     {
       DbgPrint("Bad MP configuration block checksum\n");
       KEBUGCHECK(0);
       return;
     }

   if (Table->Specification < 0x04)
     {
       DbgPrint("Bad MP configuration table version (%d)\n",
		Table->Specification);
       KEBUGCHECK(0);
       return;
     }

   if (Table->LocalAPICAddress != APIC_DEFAULT_BASE)
     {
       DbgPrint("APIC base address is at 0x%X. " \
		"I cannot handle non-standard adresses\n", Table->LocalAPICAddress);
       KEBUGCHECK(0);
     }

   Entry = (PUCHAR)((PVOID)Table + sizeof(MP_CONFIGURATION_TABLE));
   Count = 0;
   while (Count < (Table->Length - sizeof(MP_CONFIGURATION_TABLE)))
   {
     /* Switch on type */
     switch (*Entry)
       {
       case MPCTE_PROCESSOR:
         {
	   HaliMPProcessorInfo((PMP_CONFIGURATION_PROCESSOR)Entry);
	   Entry += sizeof(MP_CONFIGURATION_PROCESSOR);
	   Count += sizeof(MP_CONFIGURATION_PROCESSOR);
	   break;
	 }
       case MPCTE_BUS:
	 {
	   HaliMPBusInfo((PMP_CONFIGURATION_BUS)Entry);
	   Entry += sizeof(MP_CONFIGURATION_BUS);
	   Count += sizeof(MP_CONFIGURATION_BUS);
	   break;
	 }
       case MPCTE_IOAPIC:
	 {
	   HaliMPIOApicInfo((PMP_CONFIGURATION_IOAPIC)Entry);
	   Entry += sizeof(MP_CONFIGURATION_IOAPIC);
	   Count += sizeof(MP_CONFIGURATION_IOAPIC);
	   break;
	 }
       case MPCTE_INTSRC:
	 {
	   HaliMPIntSrcInfo((PMP_CONFIGURATION_INTSRC)Entry);
	   Entry += sizeof(MP_CONFIGURATION_INTSRC);
	   Count += sizeof(MP_CONFIGURATION_INTSRC);
	   break;
	 }
       case MPCTE_LINTSRC:
	 {
	   HaliMPIntLocalInfo((PMP_CONFIGURATION_INTLOCAL)Entry);
	   Entry += sizeof(MP_CONFIGURATION_INTLOCAL);
	   Count += sizeof(MP_CONFIGURATION_INTLOCAL);
	   break;
	 }
       default:
	 DbgPrint("Unknown entry in MPC table\n");
	 KEBUGCHECK(0);
       }
   }
}

static VOID HaliConstructDefaultIOIrqMPTable(ULONG Type)
{
	MP_CONFIGURATION_INTSRC intsrc;
	ULONG i;

	intsrc.Type = MPCTE_INTSRC;
	intsrc.IrqFlag = 0;			/* conforming */
	intsrc.SrcBusId = 0;
	intsrc.DstApicId = IOAPICMap[0].ApicId;

	intsrc.IrqType = INT_VECTORED;
	for (i = 0; i < 16; i++) {
		switch (Type) {
		case 2:
			if (i == 0 || i == 13)
				continue;	/* IRQ0 & IRQ13 not connected */
			/* Fall through */
		default:
			if (i == 2)
				continue;	/* IRQ2 is never connected */
		}

		intsrc.SrcBusIrq = i;
		intsrc.DstApicInt = i ? i : 2; /* IRQ0 to INTIN2 */
		HaliMPIntSrcInfo(&intsrc);
	}

	intsrc.IrqType = INT_EXTINT;
	intsrc.SrcBusIrq = 0;
	intsrc.DstApicInt = 0; /* 8259A to INTIN0 */
	HaliMPIntSrcInfo(&intsrc);
}

static VOID HaliConstructDefaultISAMPTable(ULONG Type)
{
  MP_CONFIGURATION_PROCESSOR processor;
  MP_CONFIGURATION_BUS bus;
  MP_CONFIGURATION_IOAPIC ioapic;
  MP_CONFIGURATION_INTLOCAL lintsrc;
  ULONG linttypes[2] = { INT_EXTINT, INT_NMI };
  ULONG i;

  /*
   * 2 CPUs, numbered 0 & 1.
   */
  processor.Type = MPCTE_PROCESSOR;
  /* Either an integrated APIC or a discrete 82489DX. */
  processor.ApicVersion = Type > 4 ? 0x10 : 0x01;
  processor.CpuFlags = CPU_FLAG_ENABLED | CPU_FLAG_BSP;
  /* FIXME: Get this from the bootstrap processor */
  processor.CpuSignature = 0;
  processor.FeatureFlags = 0;
  processor.Reserved[0] = 0;
  processor.Reserved[1] = 0;
  for (i = 0; i < 2; i++) 
  {
    processor.ApicId = i;
    HaliMPProcessorInfo(&processor);
    processor.CpuFlags &= ~CPU_FLAG_BSP;
  }

  bus.Type = MPCTE_BUS;
  bus.BusId = 0;
  switch (Type) 
  {
    default:
    DPRINT("Unknown standard configuration %d\n", Type);
      /* Fall through */
    case 1:
    case 5:
      memcpy(bus.BusType, "ISA   ", 6);
      break;
    case 2:
    case 6:
    case 3:
      memcpy(bus.BusType, "EISA  ", 6);
      break;
    case 4:
    case 7:
      memcpy(bus.BusType, "MCA   ", 6);
  }
  HaliMPBusInfo(&bus);
  if (Type > 4) 
  {
    bus.Type = MPCTE_BUS;
    bus.BusId = 1;
    memcpy(bus.BusType, "PCI   ", 6);
    HaliMPBusInfo(&bus);
  }

  ioapic.Type = MPCTE_IOAPIC;
  ioapic.ApicId = 2;
  ioapic.ApicVersion = Type > 4 ? 0x10 : 0x01;
  ioapic.ApicFlags = MP_IOAPIC_USABLE;
  ioapic.ApicAddress = IOAPIC_DEFAULT_BASE;
  HaliMPIOApicInfo(&ioapic);

  /*
   * We set up most of the low 16 IO-APIC pins according to MPS rules.
   */
  HaliConstructDefaultIOIrqMPTable(Type);

  lintsrc.Type = MPCTE_LINTSRC;
  lintsrc.IrqType = 0;
  lintsrc.IrqFlag = 0;  /* conforming */
  lintsrc.SrcBusId = 0;
  lintsrc.SrcBusIrq = 0;
  lintsrc.DstApicId = MP_APIC_ALL;
  for (i = 0; i < 2; i++) 
  {
    lintsrc.IrqType = linttypes[i];
    lintsrc.DstApicLInt = i;
    HaliMPIntLocalInfo(&lintsrc);
  }
}


BOOLEAN
HaliScanForMPConfigTable(ULONG Base,
			 ULONG Size)
/*
   PARAMETERS:
      Base = Base address of region
      Size = Length of region to check
   RETURNS:
      TRUE if a valid MP configuration table was found
 */
{
   PULONG bp = (PULONG)Base;
   MP_FLOATING_POINTER* mpf;

   while (Size > 0)
   {
      if (*bp == MPF_SIGNATURE)
      {
	DbgPrint("Found MPF signature at %x, checksum %x\n", bp,
		 MPChecksum((PUCHAR)bp, 16));
         if (MPChecksum((PUCHAR)bp, 16) == 0)
         {
            mpf = (MP_FLOATING_POINTER*)bp;

            DbgPrint("Intel MultiProcessor Specification v1.%d compliant system.\n",
              mpf->Specification);

            if (mpf->Feature2 & FEATURE2_IMCRP) {
               APICMode = amPIC;
               DPRINT("Running in IMCR and PIC compatibility mode.\n");
            } else {
               APICMode = amVWIRE;
               DPRINT("Running in Virtual Wire compatibility mode.\n");
				    }

            switch (mpf->Feature1)
            {
               case 0:
                  /* Non standard configuration */
                  break;
               case 1:
                  DPRINT("ISA\n");
                  break;
               case 2:
                  DPRINT("EISA with no IRQ8 chaining\n");
                  break;
               case 3:
                  DPRINT("EISA\n");
                  break;
               case 4:
                  DPRINT("MCA\n");
                  break;
               case 5:
                  DPRINT("ISA and PCI\n");
                  break;
               case 6:
                  DPRINT("EISA and PCI\n");
                  break;
               case 7:
                  DPRINT("MCA and PCI\n");
                  break;
               default:
                  DbgPrint("Unknown standard configuration %d\n", mpf->Feature1);
                  return FALSE;
            }

            CPUCount = 0;
            IOAPICCount = 0;
            IRQCount = 0;

            if ((mpf->Feature1 == 0) && (mpf->Address)) {
              HaliReadMPConfigTable((PMP_CONFIGURATION_TABLE)mpf->Address);
            } else {
              HaliConstructDefaultISAMPTable(mpf->Feature1);
            }

            return TRUE;
         }
      }
      bp += 4;
      Size -= 16;
   }
   return FALSE;
}

VOID
HalpInitMPS(VOID)
{
   USHORT EBDA;
   static BOOLEAN MPSInitialized = FALSE;


   /* Only initialize MP system once. Once called the first time,
      each subsequent call is part of the initialization sequence
			for an application processor. */

   DPRINT("HalpInitMPS()\n");


   if (MPSInitialized) 
   {
      KEBUGCHECK(0);
   }

   MPSInitialized = TRUE;

   /*
     Scan the system memory for an MP configuration table
       1) Scan the first KB of system base memory
       2) Scan the last KB of system base memory
       3) Scan the BIOS ROM address space between 0F0000h and 0FFFFFh
       4) Scan the Extended BIOS Data Area
   */

   if (!HaliScanForMPConfigTable(0x0, 0x400)) 
   {
      if (!HaliScanForMPConfigTable(0x9FC00, 0x400)) 
      {
         if (!HaliScanForMPConfigTable(0xF0000, 0x10000)) 
         {
            EBDA = *((PUSHORT)0x040E);
            EBDA <<= 4;
            if (!HaliScanForMPConfigTable((ULONG)EBDA, 0x1000)) 
	    {
               DbgPrint("No multiprocessor compliant system found.\n");
               KEBUGCHECK(0);
            }
         }
      }
   }

   /* Setup IRQ to vector translation map */
   memset(&IRQVectorMap, 0, sizeof(IRQVectorMap));

   /* Setup interrupt handlers */
   SetInterruptGate(LOCAL_TIMER_VECTOR, (ULONG)MpsTimerInterrupt);
   SetInterruptGate(ERROR_VECTOR, (ULONG)MpsErrorInterrupt);
   SetInterruptGate(SPURIOUS_VECTOR, (ULONG)MpsSpuriousInterrupt);
   SetInterruptGate(IPI_VECTOR, (ULONG)MpsIpiInterrupt);

}

VOID
HaliReconfigurePciInterrupts(VOID)
{
   ULONG i;

   for (i = 0; i < IRQCount; i++)
   {
      if (BUSMap[IRQMap[i].SrcBusId] == MP_BUS_PCI)
      {
         DPRINT("%02x: IrqType %02x, IrqFlag %02x, SrcBusId %02x, SrcBusIrq %02x, DstApicId %02x, DstApicInt %02x\n",
	        i, IRQMap[i].IrqType, IRQMap[i].IrqFlag, IRQMap[i].SrcBusId, 
	        IRQMap[i].SrcBusIrq, IRQMap[i].DstApicId, IRQMap[i].DstApicInt);

	 if(1 != HalSetBusDataByOffset(PCIConfiguration, IRQMap[i].SrcBusId, (IRQMap[i].SrcBusIrq >> 2) & 0x1f, &IRQMap[i].DstApicInt, 0x3c /*PCI_INTERRUPT_LINE*/, 1))
	 {
	    CHECKPOINT;
	 }
      }
   }

}

#endif /* MP */

/* EOF */
