/*
 *  FreeLoader
 *
 *  Copyright (C) 2003  Eric Kohl
 *  Copyright (C) 2006  Colin Finck <mail@colinfinck.de>
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

#include <freeldr.h>

#define NDEBUG
#include <debug.h>

#define HZ (100)
#define CLOCK_TICK_RATE (1193182)
#define LATCH (CLOCK_TICK_RATE / HZ)
#define CALIBRATE_LATCH (5 * LATCH)
#define CALIBRATE_TIME  (5 * 1000020/HZ)

#define MP_FP_SIGNATURE 0x5F504D5F	/* "_MP_" */
#define MP_CT_SIGNATURE 0x504D4350	/* "PCMP" */


typedef struct _MP_FLOATING_POINT_TABLE
{
  ULONG Signature;			/* "_MP_" */
  ULONG PhysicalAddressPointer;
  UCHAR  Length;
  UCHAR  SpecRev;
  UCHAR  Checksum;
  UCHAR  FeatureByte[5];
} PACKED MP_FLOATING_POINT_TABLE, *PMP_FLOATING_POINT_TABLE;


typedef struct _MPS_CONFIG_TABLE_HEADER
{
  ULONG Signature;			/* "PCMP" */
  USHORT BaseTableLength;
  UCHAR  SpecRev;
  UCHAR  Checksum;
  UCHAR  OemIdString[8];
  UCHAR  ProductIdString[12];
  ULONG OemTablePointer;
  USHORT OemTableLength;
  USHORT EntryCount;
  ULONG AddressOfLocalAPIC;
  USHORT ExtendedTableLength;
  UCHAR  ExtendedTableChecksum;
  UCHAR  Reserved;
} PACKED MP_CONFIGURATION_TABLE, *PMP_CONFIGURATION_TABLE;


typedef struct _MP_PROCESSOR_ENTRY
{
  UCHAR  EntryType;
  UCHAR  LocalApicId;
  UCHAR  LocalApicVersion;
  UCHAR  CpuFlags;
  ULONG CpuSignature;
  ULONG FeatureFlags;
  ULONG Reserved1;
  ULONG Reserved2;
} PACKED MP_PROCESSOR_ENTRY, *PMP_PROCESSOR_ENTRY;


/* FUNCTIONS ****************************************************************/

static ULONG
GetCpuSpeed(BOOLEAN DualCoreSpeedMesure)
{
  ULONGLONG Timestamp1;
  ULONGLONG Timestamp2;
  ULONGLONG Diff;
  ULONG     Count = 0;

   /* 
      FIXME 
      if the DualCoreSpeedMesure is true we need 
      use the wrmsr and rdmsr to mesure the speed

      The rdtc are outdate to use if cpu support 
      the wrmsr and rdmsr, see intel doc AP-485
      for more informations and how to use it. 

      Follow code is good on cpu that does not
      support dual core or have a mmx unit or
      more. 
    */


  /* Initialise timer channel 2 */
  /* Set the Gate high, disable speaker */
  WRITE_PORT_UCHAR((PUCHAR)0x61, (READ_PORT_UCHAR((PUCHAR)0x61) & ~0x02) | 0x01);
  WRITE_PORT_UCHAR((PUCHAR)0x43, 0xB0);  /* binary, mode 0, LSB/MSB, ch 2 */
  WRITE_PORT_UCHAR((PUCHAR)0x42, CALIBRATE_LATCH & 0xff); /* LSB */
  WRITE_PORT_UCHAR((PUCHAR)0x42, CALIBRATE_LATCH >> 8); /* MSB */

  /* Read TSC (Time Stamp Counter) */
  Timestamp1 = RDTSC();

  /* Wait */
  do
    {
      Count++;
    }
  while ((READ_PORT_UCHAR((PUCHAR)0x61) & 0x20) == 0);

  /* Read TSC (Time Stamp Counter) again */
  Timestamp2 = RDTSC();

  /* Calculate elapsed time (check for counter overrun) */
  if (Timestamp2 > Timestamp1)
    {
      Diff = Timestamp2 - Timestamp1;
    }
  else
    {
      Diff = Timestamp2 + (((ULONGLONG)-1) - Timestamp1);
    }

  return (ULONG)(Diff / CALIBRATE_TIME);
}


static VOID
DetectCPU(FRLDRHKEY CpuKey,
	  FRLDRHKEY FpuKey)
{
  WCHAR VendorIdentifier[13];
  WCHAR ProcessorNameString[49];
  CHAR tmpVendorIdentifier[13];
  CHAR tmpProcessorNameString[49];
  WCHAR Identifier[64];
  ULONG FeatureSet;
  FRLDRHKEY CpuInstKey;
  FRLDRHKEY FpuInstKey;
  ULONG eax = 0;
  ULONG ebx = 0;
  ULONG ecx = 0;
  ULONG edx = 0;
  ULONG i;
  ULONG *Ptr;
  LONG Error;
  BOOLEAN SupportTSC = FALSE;
  ULONG CpuSpeed;
  BOOLEAN DualCoreSpeedMesure = FALSE;

  /* Create the CPU instance key */
  Error = RegCreateKey(CpuKey,
		       L"0",
		       &CpuInstKey);
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_HWDETECT, "RegCreateKey() failed (Error %u)\n", (int)Error));
      return;
    }

  /* Create the FPU instance key */
  Error = RegCreateKey(FpuKey,
		       L"0",
		       &FpuInstKey);
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_HWDETECT, "RegCreateKey() failed (Error %u)\n", (int)Error));
      return;
    }

  eax = CpuidSupported();
  if (eax & 1)
    {
      DbgPrint((DPRINT_HWDETECT, "CPUID supported\n"));

      /* Get vendor identifier */
      GetCpuid(0, &eax, &ebx, &ecx, &edx);
      tmpVendorIdentifier[12] = 0;
      Ptr = (ULONG*)&tmpVendorIdentifier[0];
      *Ptr = ebx;
      Ptr++;
      *Ptr = edx;
      Ptr++;
      *Ptr = ecx;
      swprintf(VendorIdentifier, L"%S", tmpVendorIdentifier);

      /* Get Identifier */
      GetCpuid(1, &eax, &ebx, &ecx, &edx);
      swprintf(Identifier,
	       L"x86 Family %u Model %u Stepping %u",
	       (unsigned int)((eax >> 8) & 0x0F),
	       (unsigned int)((eax >> 4) & 0x0F),
	       (unsigned int)(eax & 0x0F));
      FeatureSet = edx;
      
      if ((FeatureSet & 0x10) == 0x10)
        SupportTSC = TRUE;

      if ((FeatureSet & 0x20) == 0x20)
          DualCoreSpeedMesure = TRUE;

      /* Check if Extended CPUID information is supported */
      GetCpuid(0x80000000, &eax, &ebx, &ecx, &edx);

      if(eax >= 0x80000004)
      {
        /* Get Processor Name String */
        tmpProcessorNameString[48] = 0;
        Ptr = (ULONG*)&tmpProcessorNameString[0];

        for (i = 0x80000002; i <= 0x80000004; i++)
        {
          GetCpuid(i, &eax, &ebx, &ecx, &edx);
          *Ptr = eax;
          Ptr++;
          *Ptr = ebx;
          Ptr++;
          *Ptr = ecx;
          Ptr++;
          *Ptr = edx;
          Ptr++;
        }

        swprintf(ProcessorNameString, L"%S", tmpProcessorNameString);


        /* Set 'ProcessorNameString' value (CPU only) */
        DbgPrint((DPRINT_HWDETECT, "Processor Name String: %S\n", ProcessorNameString));

        Error = RegSetValue(CpuInstKey,
		            L"ProcessorNameString",
		            REG_SZ,
		            (PCHAR)ProcessorNameString,
		            (wcslen(ProcessorNameString) + 1) * sizeof(WCHAR));
		    
        if (Error != ERROR_SUCCESS)
          DbgPrint((DPRINT_HWDETECT, "RegSetValue() failed (Error %u)\n", (int)Error));
      }
    }
  else
    {
      DbgPrint((DPRINT_HWDETECT, "CPUID not supported\n"));

      wcscpy(VendorIdentifier, L"Unknown");
      swprintf(Identifier,
	      L"x86 Family %u Model %u Stepping %u",
	      (unsigned int)((eax >> 8) & 0x0F),
	      (unsigned int)((eax >> 4) & 0x0F),
	      (unsigned int)(eax & 0x0F));
      FeatureSet = 0;
    }

  /* Set 'Conmponent Information' value (CPU and FPU) */
  SetComponentInformation(CpuInstKey, 0, 0, 1);
  SetComponentInformation(FpuInstKey, 0, 0, 1);

  /* Set 'FeatureSet' value (CPU only) */
  DbgPrint((DPRINT_HWDETECT, "FeatureSet: %x\n", FeatureSet));

  Error = RegSetValue(CpuInstKey,
		      L"FeatureSet",
		      REG_DWORD,
		      (PCHAR)&FeatureSet,
		      sizeof(ULONG));
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_HWDETECT, "RegSetValue() failed (Error %u)\n", (int)Error));
    }

  /* Set 'Identifier' value (CPU and FPU) */
  DbgPrint((DPRINT_HWDETECT, "Identifier: %S\n", Identifier));

  Error = RegSetValue(CpuInstKey,
		      L"Identifier",
		      REG_SZ,
		      (PCHAR)Identifier,
		      (wcslen(Identifier) + 1) * sizeof(WCHAR));
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_HWDETECT, "RegSetValue() failed (Error %u)\n", (int)Error));
    }

  Error = RegSetValue(FpuInstKey,
		      L"Identifier",
		      REG_SZ,
		      (PCHAR)Identifier,
		      (wcslen(Identifier) + 1) * sizeof(WCHAR));
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_HWDETECT, "RegSetValue() failed (Error %u)\n", (int)Error));
    }

  /* Set 'VendorIdentifier' value (CPU only) */
  DbgPrint((DPRINT_HWDETECT, "Vendor Identifier: %S\n", VendorIdentifier));

  Error = RegSetValue(CpuInstKey,
		      L"VendorIdentifier",
		      REG_SZ,
		      (PCHAR)VendorIdentifier,
		      (wcslen(VendorIdentifier) + 1) * sizeof(WCHAR));
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_HWDETECT, "RegSetValue() failed (Error %u)\n", (int)Error));
    }

  /* FIXME: Set 'Update Signature' value (CPU only) */

  /* FIXME: Set 'Update Status' value (CPU only) */

  /* Set '~MHz' value (CPU only) */
  if (SupportTSC)
    {
      CpuSpeed = GetCpuSpeed(DualCoreSpeedMesure);

      Error = RegSetValue(CpuInstKey,
			  L"~MHz",
			  REG_DWORD,
			  (PCHAR)&CpuSpeed,
			  sizeof(ULONG));
      if (Error != ERROR_SUCCESS)
	{
	  DbgPrint((DPRINT_HWDETECT, "RegSetValue() failed (Error %u)\n", (int)Error));
	}
    }
}


static VOID
SetMpsProcessor(FRLDRHKEY CpuKey,
		FRLDRHKEY FpuKey,
		PMP_PROCESSOR_ENTRY CpuEntry)
{
  WCHAR ProcessorNameString[49];
  WCHAR VendorIdentifier[13];
  CHAR tmpProcessorNameString[49];
  CHAR tmpVendorIdentifier[13];
  WCHAR Identifier[64];
  WCHAR Buffer[8];
  ULONG FeatureSet;
  FRLDRHKEY CpuInstKey;
  FRLDRHKEY FpuInstKey;
  ULONG eax = 0;
  ULONG ebx = 0;
  ULONG ecx = 0;
  ULONG edx = 0;
  ULONG i;
  ULONG *Ptr;
  LONG Error;
  ULONG CpuSpeed;

  /* Get processor instance number */
  swprintf(Buffer, L"%u", CpuEntry->LocalApicId);

  /* Create the CPU instance key */
  Error = RegCreateKey(CpuKey,
		       Buffer,
		       &CpuInstKey);
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_HWDETECT, "RegCreateKey() failed (Error %u)\n", (int)Error));
      return;
    }

  /* Create the FPU instance key */
  Error = RegCreateKey(FpuKey,
		       Buffer,
		       &FpuInstKey);
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_HWDETECT, "RegCreateKey() failed (Error %u)\n", (int)Error));
      return;
    }

  /* Get 'VendorIdentifier' */
  GetCpuid(0, &eax, &ebx, &ecx, &edx);
  tmpVendorIdentifier[12] = 0;
  Ptr = (ULONG*)&tmpVendorIdentifier[0];
  *Ptr = ebx;
  Ptr++;
  *Ptr = edx;
  Ptr++;
  *Ptr = ecx;
  swprintf(VendorIdentifier, L"%S", tmpVendorIdentifier);

  /* Get 'Identifier' */
  swprintf(Identifier,
	  L"x86 Family %u Model %u Stepping %u",
	  (ULONG)((CpuEntry->CpuSignature >> 8) & 0x0F),
	  (ULONG)((CpuEntry->CpuSignature >> 4) & 0x0F),
	  (ULONG)(CpuEntry->CpuSignature & 0x0F));

  /* Get FeatureSet */
  FeatureSet = CpuEntry->FeatureFlags;

  /* Check if Extended CPUID information is supported */
  GetCpuid(0x80000000, &eax, &ebx, &ecx, &edx);

  if(eax >= 0x80000004)
  {
    /* Get 'ProcessorNameString' */
    tmpProcessorNameString[48] = 0;
    Ptr = (ULONG*)&tmpProcessorNameString[0];

    for (i = 0x80000002; i <= 0x80000004; i++)
    {
      GetCpuid(i, &eax, &ebx, &ecx, &edx);
      *Ptr = eax;
      Ptr++;
      *Ptr = ebx;
      Ptr++;
      *Ptr = ecx;
      Ptr++;
      *Ptr = edx;
      Ptr++;
    }

    swprintf(ProcessorNameString, L"%S", tmpProcessorNameString);


    /* Set 'ProcessorNameString' value (CPU only) */
    DbgPrint((DPRINT_HWDETECT, "Processor Name String: %S\n", ProcessorNameString));

    Error = RegSetValue(CpuInstKey,
                        L"ProcessorNameString",
	                      REG_SZ,
		                    (PCHAR)ProcessorNameString,
		                    (wcslen(ProcessorNameString) + 1) * sizeof(WCHAR));
    if (Error != ERROR_SUCCESS)
      DbgPrint((DPRINT_HWDETECT, "RegSetValue() failed (Error %u)\n", (int)Error));
  }

  /* Set 'Configuration Data' value (CPU and FPU) */
  SetComponentInformation(CpuInstKey,
			  0,
			  CpuEntry->LocalApicId,
			  1 << CpuEntry->LocalApicId);

  SetComponentInformation(FpuInstKey,
			  0,
			  CpuEntry->LocalApicId,
			  1 << CpuEntry->LocalApicId);

  /* Set 'FeatureSet' value (CPU only) */
  DbgPrint((DPRINT_HWDETECT, "FeatureSet: %x\n", FeatureSet));

  Error = RegSetValue(CpuInstKey,
		      L"FeatureSet",
		      REG_DWORD,
		      (PCHAR)&FeatureSet,
		      sizeof(ULONG));
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_HWDETECT, "RegSetValue() failed (Error %u)\n", (int)Error));
    }

  /* Set 'Identifier' value (CPU and FPU) */
  DbgPrint((DPRINT_HWDETECT, "Identifier: %S\n", Identifier));

  Error = RegSetValue(CpuInstKey,
		      L"Identifier",
		      REG_SZ,
		      (PCHAR)Identifier,
		      (wcslen(Identifier) + 1) * sizeof(WCHAR));
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_HWDETECT, "RegSetValue() failed (Error %u)\n", (int)Error));
    }

  Error = RegSetValue(FpuInstKey,
		      L"Identifier",
		      REG_SZ,
		      (PCHAR)Identifier,
		      (wcslen(Identifier) + 1) * sizeof(WCHAR));
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_HWDETECT, "RegSetValue() failed (Error %u)\n", (int)Error));
    }

  /* Set 'VendorIdentifier' value (CPU only) */
  DbgPrint((DPRINT_HWDETECT, "Vendor Identifier: %S\n", VendorIdentifier));

  Error = RegSetValue(CpuInstKey,
		      L"VendorIdentifier",
		      REG_SZ,
		      (PCHAR)VendorIdentifier,
		      (wcslen(VendorIdentifier) + 1) * sizeof(WCHAR));
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_HWDETECT, "RegSetValue() failed (Error %u)\n", (int)Error));
    }

  /* FIXME: Set 'Update Signature' value (CPU only) */

  /* FIXME: Set 'Update Status' value (CPU only) */

  /* Set '~MHz' value (CPU only) */

     
  if ((CpuEntry->FeatureFlags  & 0x10) == 0x10)
  {
      if ((CpuEntry->FeatureFlags  & 0x20) == 0x20)
      {
           CpuSpeed = GetCpuSpeed(TRUE);
      }
      else
      {
          DbgPrint((DPRINT_HWDETECT, "Does not support MSR that are need for mesure the speed correct\n", (int)Error));
          CpuSpeed = GetCpuSpeed(FALSE);
      }

      Error = RegSetValue(CpuInstKey,
			  L"~MHz",
			  REG_DWORD,
			  (PCHAR)&CpuSpeed,
			  sizeof(ULONG));
      if (Error != ERROR_SUCCESS)
      {

	     DbgPrint((DPRINT_HWDETECT, "RegSetValue() failed (Error %u)\n", (int)Error));
	  }
  }
}


static PMP_FLOATING_POINT_TABLE
GetMpFloatingPointTable(VOID)
{
  PMP_FLOATING_POINT_TABLE FpTable;
  char *Ptr;
  UCHAR Sum;
  ULONG Length;
  ULONG i;

  FpTable = (PMP_FLOATING_POINT_TABLE)0xF0000;
  while ((ULONG)FpTable < 0x100000)
    {
      if (FpTable->Signature == MP_FP_SIGNATURE)
	{
	  Length = FpTable->Length * 0x10;
	  Ptr = (char *)FpTable;
	  Sum = 0;
	  for (i = 0; i < Length; i++)
	    {
	      Sum += Ptr[i];
	    }
	  DbgPrint((DPRINT_HWDETECT,
		    "Checksum: %u\n",
		    Sum));

	  if (Sum != 0)
	    {
	      DbgPrint((DPRINT_HWDETECT,
			"Invalid MP floating point checksum: %u\n",
			Sum));
	      return NULL;
	    }

	  return FpTable;
	}

      FpTable = (PMP_FLOATING_POINT_TABLE)((ULONG)FpTable + 0x10);
    }

  return NULL;
}


static PMP_CONFIGURATION_TABLE
GetMpConfigurationTable(PMP_FLOATING_POINT_TABLE FpTable)
{
  PMP_CONFIGURATION_TABLE ConfigTable;
  char *Ptr;
  UCHAR Sum;
  ULONG Length;
  ULONG i;

  if (FpTable->FeatureByte[0] != 0 ||
      FpTable->PhysicalAddressPointer == 0)
    return NULL;

  ConfigTable = (PMP_CONFIGURATION_TABLE)FpTable->PhysicalAddressPointer;
  if (ConfigTable->Signature != MP_CT_SIGNATURE)
    return NULL;

  DbgPrint((DPRINT_HWDETECT,
	    "MP Configuration Table at: %x\n",
	    (ULONG)ConfigTable));

  /* Calculate base table checksum */
  Length = ConfigTable->BaseTableLength;
  Ptr = (char *)ConfigTable;
  Sum = 0;
  for (i = 0; i < Length; i++)
    {
      Sum += Ptr[i];
    }
  DbgPrint((DPRINT_HWDETECT,
	    "MP Configuration Table base checksum: %u\n",
	    Sum));

  if (Sum != 0)
    {
      DbgPrint((DPRINT_HWDETECT,
		"Invalid MP Configuration Table base checksum: %u\n",
		Sum));
      return NULL;
    }

  if (ConfigTable->ExtendedTableLength != 0)
    {
      /* FIXME: Check extended table */
    }

  return ConfigTable;
}


static BOOLEAN
DetectMps(FRLDRHKEY CpuKey,
	  FRLDRHKEY FpuKey)
{
  PMP_FLOATING_POINT_TABLE FpTable;
  PMP_CONFIGURATION_TABLE ConfigTable;
  PMP_PROCESSOR_ENTRY CpuEntry;
  char *Ptr;
  ULONG Offset;

  /* Get floating point table */
  FpTable = GetMpFloatingPointTable();
  if (FpTable == NULL)
    return FALSE;

  DbgPrint((DPRINT_HWDETECT,
	    "MP Floating Point Table at: %x\n",
	    (ULONG)FpTable));

  if (FpTable->FeatureByte[0] == 0)
    {
      /* Get configuration table */
      ConfigTable = GetMpConfigurationTable(FpTable);
      if (ConfigTable == NULL)
	{
	  DbgPrint((DPRINT_HWDETECT,
		    "Failed to find the MP Configuration Table\n"));
	  return FALSE;
	}

      Offset = sizeof(MP_CONFIGURATION_TABLE);
      while (Offset < ConfigTable->BaseTableLength)
	{
	  Ptr = (char*)((ULONG)ConfigTable + Offset);

	  switch (*Ptr)
	    {
	      case 0:
		CpuEntry = (PMP_PROCESSOR_ENTRY)Ptr;

		DbgPrint((DPRINT_HWDETECT, "Processor Entry\n"));
		DbgPrint((DPRINT_HWDETECT,
			  "APIC Id %u  APIC Version %u  Flags %x  Signature %x  Feature %x\n",
			  CpuEntry->LocalApicId,
			  CpuEntry->LocalApicVersion,
			  CpuEntry->CpuFlags,
			  CpuEntry->CpuSignature,
			  CpuEntry->FeatureFlags));
		DbgPrint((DPRINT_HWDETECT,
			  "Processor %u: x86 Family %u Model %u Stepping %u\n",
			  CpuEntry->LocalApicId,
			  (ULONG)((CpuEntry->CpuSignature >> 8) & 0x0F),
			  (ULONG)((CpuEntry->CpuSignature >> 4) & 0x0F),
			  (ULONG)(CpuEntry->CpuSignature & 0x0F)));

		SetMpsProcessor(CpuKey, FpuKey, CpuEntry);
		Offset += 0x14;
		break;

	      case 1:
		DbgPrint((DPRINT_HWDETECT, "Bus Entry\n"));
		Offset += 0x08;
		break;

	      case 2:
		DbgPrint((DPRINT_HWDETECT, "I/0 APIC Entry\n"));
		Offset += 0x08;
		break;

	      case 3:
		DbgPrint((DPRINT_HWDETECT, "I/0 Interrupt Assignment Entry\n"));
		Offset += 0x08;
		break;

	      case 4:
		DbgPrint((DPRINT_HWDETECT, "Local Interrupt Assignment Entry\n"));
		Offset += 0x08;
		break;

	      default:
		DbgPrint((DPRINT_HWDETECT, "Unknown Entry %u\n",(ULONG)*Ptr));
		return FALSE;
	    }
	}
    }
  else
    {
      DbgPrint((DPRINT_HWDETECT,
	       "Unsupported MPS configuration: %x\n",
	       FpTable->FeatureByte[0]));

      /* FIXME: Identify default configurations */

      return FALSE;
    }

  return TRUE;
}



VOID
DetectCPUs(FRLDRHKEY SystemKey)
{
  FRLDRHKEY CpuKey;
  FRLDRHKEY FpuKey;
  LONG Error;

  /* Create the 'CentralProcessor' key */
  Error = RegCreateKey(SystemKey,
		       L"CentralProcessor",
		       &CpuKey);
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_HWDETECT, "RegCreateKey() failed (Error %u)\n", (int)Error));
      return;
    }

  /* Create the 'FloatingPointProcessor' key */
  Error = RegCreateKey(SystemKey,
		       L"FloatingPointProcessor",
		       &FpuKey);
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_HWDETECT, "RegCreateKey() failed (Error %u)\n", (int)Error));
      return;
    }

  /* Detect CPUs */
  if (!DetectMps(CpuKey, FpuKey))
    {
      DetectCPU(CpuKey, FpuKey);
    }
}

/* EOF */
