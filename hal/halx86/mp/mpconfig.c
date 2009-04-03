/* $Id$
 *
 * COPYRIGHT:             See COPYING in the top level directory
 * PROJECT:               ReactOS kernel
 * FILE:                  hal/halx86/generic/mpconfig.c
 * PURPOSE:               
 * PROGRAMMER:            
 */

/* INCLUDES *****************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ******************************************************************/

MP_FLOATING_POINTER* Mpf = NULL;

/* FUNCTIONS ****************************************************************/

static UCHAR 
MPChecksum(PUCHAR Base,
	   ULONG Size)
/*
 * Checksum an MP configuration block
 */
{
   UCHAR Sum = 0;

   while (Size--)
      Sum += *Base++;

   return Sum;
}

static VOID 
HaliMPIntSrcInfo(PMP_CONFIGURATION_INTSRC m)
{
  DPRINT("Int: type %d, pol %d, trig %d, bus %d,"
         " IRQ %02x, APIC ID %x, APIC INT %02x\n",
         m->IrqType, m->IrqFlag & 3,
         (m->IrqFlag >> 2) & 3, m->SrcBusId,
         m->SrcBusIrq, m->DstApicId, m->DstApicInt);
  if (IRQCount > MAX_IRQ_SOURCE) 
  {
    DPRINT1("Max # of irq sources exceeded!!\n");
    ASSERT(FALSE);
  }

  IRQMap[IRQCount] = *m;
  IRQCount++;
}

PCHAR 
HaliMPFamily(ULONG Family,
	     ULONG Model)
{
   static CHAR str[64];
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


static VOID 
HaliMPProcessorInfo(PMP_CONFIGURATION_PROCESSOR m)
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
//  ApicVersion[m->ApicId] = Ver;
//  BiosCpuApicId[CPUCount] = m->ApicId;
  CPUMap[CPUCount].APICVersion = ver;
  
  CPUCount++;
}

static VOID 
HaliMPBusInfo(PMP_CONFIGURATION_BUS m)
{
  static ULONG CurrentPCIBusId = 0;

  DPRINT("Bus #%d is %.*s\n", m->BusId, 6, m->BusType);

  if (strncmp(m->BusType, BUSTYPE_ISA, sizeof(BUSTYPE_ISA)-1) == 0) 
  {
     BUSMap[m->BusId] = MP_BUS_ISA;
  } 
  else if (strncmp(m->BusType, BUSTYPE_EISA, sizeof(BUSTYPE_EISA)-1) == 0) 
  {
     BUSMap[m->BusId] = MP_BUS_EISA;
  } 
  else if (strncmp(m->BusType, BUSTYPE_PCI, sizeof(BUSTYPE_PCI)-1) == 0) 
  {
     BUSMap[m->BusId] = MP_BUS_PCI;
     PCIBUSMap[m->BusId] = CurrentPCIBusId;
     CurrentPCIBusId++;
  } 
  else if (strncmp(m->BusType, BUSTYPE_MCA, sizeof(BUSTYPE_MCA)-1) == 0) 
  {
     BUSMap[m->BusId] = MP_BUS_MCA;
  } 
  else 
  {
     DPRINT("Unknown bustype %.*s - ignoring\n", 6, m->BusType);
  }
}

static VOID 
HaliMPIOApicInfo(PMP_CONFIGURATION_IOAPIC m)
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
    ASSERT(FALSE);
  }

  IOAPICMap[IOAPICCount].ApicId = m->ApicId;
  IOAPICMap[IOAPICCount].ApicVersion = m->ApicVersion;
  IOAPICMap[IOAPICCount].ApicAddress = m->ApicAddress;
  IOAPICCount++;
}


static VOID 
HaliMPIntLocalInfo(PMP_CONFIGURATION_INTLOCAL m)
{
  DPRINT("Lint: type %d, pol %d, trig %d, bus %d,"
         " IRQ %02x, APIC ID %x, APIC LINT %02x\n",
         m->IrqType, m->SrcBusIrq & 3,
         (m->SrcBusIrq >> 2) & 3, m->SrcBusId,
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
    ASSERT(FALSE);
  }
  if ((m->IrqType == INT_NMI) && (m->DstApicLInt != 1)) 
  {
    DPRINT1("Invalid MP table!\n");
    ASSERT(FALSE);
  }
}


static BOOLEAN
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
       
       DPRINT1("Bad MP configuration block signature: %c%c%c%c\n",
		pc[0], pc[1], pc[2], pc[3]);
       KeBugCheckEx(0, pc[0], pc[1], pc[2], pc[3]);
       return FALSE;
     }

   if (MPChecksum((PUCHAR)Table, Table->Length))
     {
       DPRINT1("Bad MP configuration block checksum\n");
       ASSERT(FALSE);
       return FALSE;
     }

   if (Table->Specification != 0x01 && Table->Specification != 0x04)
     {
       DPRINT1("Bad MP configuration table version (%d)\n",
	       Table->Specification);
       ASSERT(FALSE);
       return FALSE;
     }

   if (Table->LocalAPICAddress != APIC_DEFAULT_BASE)
     {
       DPRINT1("APIC base address is at 0x%X. I cannot handle non-standard adresses\n", 
	       Table->LocalAPICAddress);
       ASSERT(FALSE);
       return FALSE;
     }

   DPRINT("Oem: %.*s, ProductId: %.*s\n", 8, Table->Oem, 12, Table->ProductId);
   DPRINT("APIC at: %08x\n", Table->LocalAPICAddress);


   Entry = (PUCHAR)((ULONG_PTR)Table + sizeof(MP_CONFIGURATION_TABLE));
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
	 DPRINT1("Unknown entry in MPC table\n");
	 ASSERT(FALSE);
	 return FALSE;
       }
   }
   return TRUE;
}

static VOID 
HaliConstructDefaultIOIrqMPTable(ULONG Type)
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

static VOID 
HaliConstructDefaultISAMPTable(ULONG Type)
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


static BOOLEAN
HaliScanForMPConfigTable(ULONG Base,
			 ULONG Size)
{
/*
   PARAMETERS:
      Base = Base address of region
      Size = Length of region to check
   RETURNS:
      TRUE if a valid MP configuration table was found
 */

   PULONG bp = (PULONG)Base;
   MP_FLOATING_POINTER* mpf;
   UCHAR Checksum;

   while (Size > 0)
   {
      mpf = (MP_FLOATING_POINTER*)bp;
      if (mpf->Signature == MPF_SIGNATURE)
      {
	 Checksum = MPChecksum((PUCHAR)bp, 16);
	 DPRINT("Found MPF signature at %x, checksum %x\n", bp, Checksum);
         if (Checksum == 0 &&
	     mpf->Length == 1)
         {
            DPRINT("Intel MultiProcessor Specification v1.%d compliant system.\n",
                   mpf->Specification);

            if (mpf->Feature2 & FEATURE2_IMCRP) 
	    {
               DPRINT("Running in IMCR and PIC compatibility mode.\n");
            } 
	    else 
	    {
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
                  DPRINT("Unknown standard configuration %d\n", mpf->Feature1);
                  return FALSE;
            }
            Mpf = mpf; 
            return TRUE;
         }
      }
      bp += 4;
      Size -= 16;
   }
   return FALSE;
}

static BOOLEAN
HaliGetSmpConfig(VOID)
{
   if (Mpf == NULL)
   {
      return FALSE;
   }

   if (Mpf->Feature2 & FEATURE2_IMCRP)
   {
      DPRINT("Running in IMCR and PIC compatibility mode.\n");
      APICMode = amPIC;
   }
   else
   {
      DPRINT("Running in Virtual Wire compatibility mode.\n");
      APICMode = amVWIRE;
   }

   if (Mpf->Feature1 == 0 && Mpf->Address)
   {
      if(!HaliReadMPConfigTable((PMP_CONFIGURATION_TABLE)Mpf->Address))
      {
         DPRINT("BIOS bug, MP table errors detected!...\n");
	 DPRINT("... disabling SMP support. (tell your hw vendor)\n");
	 return FALSE;
      }
      if (IRQCount == 0)
      {
         MP_CONFIGURATION_BUS bus;

         DPRINT("BIOS bug, no explicit IRQ entries, using default mptable. (tell your hw vendor)\n");

         bus.BusId = 1;
	 memcpy(bus.BusType, "ISA   ", 6);
         HaliMPBusInfo(&bus);
	 HaliConstructDefaultIOIrqMPTable(bus.BusId);
      }

   } 
   else if(Mpf->Feature1 != 0)
   {
      HaliConstructDefaultISAMPTable(Mpf->Feature1);
   }
   else
   {
      ASSERT(FALSE);
   }
   return TRUE;
}    

BOOLEAN 
HaliFindSmpConfig(VOID)
{
   /*
     Scan the system memory for an MP configuration table
       1) Scan the first KB of system base memory
       2) Scan the last KB of system base memory
       3) Scan the BIOS ROM address space between 0F0000h and 0FFFFFh
       4) Scan the first KB from the Extended BIOS Data Area
   */

   if (!HaliScanForMPConfigTable(0x0, 0x400)) 
   {
      if (!HaliScanForMPConfigTable(0x9FC00, 0x400)) 
      {
         if (!HaliScanForMPConfigTable(0xF0000, 0x10000)) 
         {
            if (!HaliScanForMPConfigTable(*((PUSHORT)0x040E) << 4, 0x400)) 
	    {
               DPRINT("No multiprocessor compliant system found.\n");
               return FALSE;
            }
         }
      }
   }

   if (HaliGetSmpConfig())
   {
      return TRUE;
   }
   else
   {
      DPRINT("No MP config table found\n");
      return FALSE;
   }

}

/* EOF */
