/*
 *  FreeLoader
 *
 *  Copyright (C) 2003  Eric Kohl
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
#include <arch.h>
#include <rtl.h>
#include <debug.h>
#include <mm.h>
#include <portio.h>

#include "../../reactos/registry.h"
#include "hardware.h"


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
GetCpuSpeed(VOID)
{
  ULONGLONG Timestamp1;
  ULONGLONG Timestamp2;
  ULONGLONG Diff;

  /* Read TSC (Time Stamp Counter) */
  Timestamp1 = RDTSC();

  /* Wait for 0.1 seconds (= 100 milliseconds = 100000 microseconds)*/
  StallExecutionProcessor(100000);

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

  return (ULONG)(Diff / 100000);
}


static VOID
DetectCPU(FRLDRHKEY CpuKey,
	  FRLDRHKEY FpuKey)
{
  char VendorIdentifier[13];
  char Identifier[64];
  ULONG FeatureSet;
  FRLDRHKEY CpuInstKey;
  FRLDRHKEY FpuInstKey;
  ULONG eax = 0;
  ULONG ebx = 0;
  ULONG ecx = 0;
  ULONG edx = 0;
  ULONG *Ptr;
  LONG Error;
  BOOL SupportTSC = FALSE;
  ULONG CpuSpeed;


  /* Create the CPU instance key */
  Error = RegCreateKey(CpuKey,
		       "0",
		       &CpuInstKey);
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_HWDETECT, "RegCreateKey() failed (Error %u)\n", (int)Error));
      return;
    }

  /* Create the FPU instance key */
  Error = RegCreateKey(FpuKey,
		       "0",
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
      VendorIdentifier[12] = 0;
      Ptr = (ULONG*)&VendorIdentifier[0];
      *Ptr = ebx;
      Ptr++;
      *Ptr = edx;
      Ptr++;
      *Ptr = ecx;

      /* Get Identifier */
      GetCpuid(1, &eax, &ebx, &ecx, &edx);
      sprintf(Identifier,
	      "x86 Family %u Model %u Stepping %u",
	      (unsigned int)((eax >> 8) & 0x0F),
	      (unsigned int)((eax >> 4) & 0x0F),
	      (unsigned int)(eax & 0x0F));
      FeatureSet = edx;
      if (((eax >> 8) & 0x0F) >= 5)
        SupportTSC = TRUE;
    }
  else
    {
      DbgPrint((DPRINT_HWDETECT, "CPUID not supported\n"));

      strcpy(VendorIdentifier, "Unknown");
      sprintf(Identifier,
	      "x86 Family %u Model %u Stepping %u",
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
		      "FeatureSet",
		      REG_DWORD,
		      (PUCHAR)&FeatureSet,
		      sizeof(ULONG));
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_HWDETECT, "RegSetValue() failed (Error %u)\n", (int)Error));
    }

  /* Set 'Identifier' value (CPU and FPU) */
  DbgPrint((DPRINT_HWDETECT, "Identifier: %s\n", Identifier));

  Error = RegSetValue(CpuInstKey,
		      "Identifier",
		      REG_SZ,
		      (PUCHAR)Identifier,
		      strlen(Identifier) + 1);
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_HWDETECT, "RegSetValue() failed (Error %u)\n", (int)Error));
    }

  Error = RegSetValue(FpuInstKey,
		      "Identifier",
		      REG_SZ,
		      (PUCHAR)Identifier,
		      strlen(Identifier) + 1);
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_HWDETECT, "RegSetValue() failed (Error %u)\n", (int)Error));
    }

  /* Set 'VendorIdentifier' value (CPU only) */
  DbgPrint((DPRINT_HWDETECT, "Vendor Identifier: %s\n", VendorIdentifier));

  Error = RegSetValue(CpuInstKey,
		      "VendorIdentifier",
		      REG_SZ,
		      (PUCHAR)VendorIdentifier,
		      strlen(VendorIdentifier) + 1);
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_HWDETECT, "RegSetValue() failed (Error %u)\n", (int)Error));
    }

  /* FIXME: Set 'Update Signature' value (CPU only) */

  /* FIXME: Set 'Update Status' value (CPU only) */

  /* Set '~MHz' value (CPU only) */
  if (SupportTSC)
    {
      CpuSpeed = GetCpuSpeed();

      Error = RegSetValue(CpuInstKey,
			  "~MHz",
			  REG_DWORD,
			  (PUCHAR)&CpuSpeed,
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
  char VendorIdentifier[13];
  char Identifier[64];
  char Buffer[8];
  ULONG FeatureSet;
  FRLDRHKEY CpuInstKey;
  FRLDRHKEY FpuInstKey;
  ULONG eax = 0;
  ULONG ebx = 0;
  ULONG ecx = 0;
  ULONG edx = 0;
  ULONG *Ptr;
  LONG Error;
  ULONG CpuSpeed;

  /* Get processor instance number */
  sprintf(Buffer, "%u", CpuEntry->LocalApicId);

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
  VendorIdentifier[12] = 0;
  Ptr = (ULONG*)&VendorIdentifier[0];
  *Ptr = ebx;
  Ptr++;
  *Ptr = edx;
  Ptr++;
  *Ptr = ecx;

  /* Get 'Identifier' */
  sprintf(Identifier,
	  "x86 Family %u Model %u Stepping %u",
	  (ULONG)((CpuEntry->CpuSignature >> 8) & 0x0F),
	  (ULONG)((CpuEntry->CpuSignature >> 4) & 0x0F),
	  (ULONG)(CpuEntry->CpuSignature & 0x0F));

  /* Get FeatureSet */
  FeatureSet = CpuEntry->FeatureFlags;

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
		      "FeatureSet",
		      REG_DWORD,
		      (PUCHAR)&FeatureSet,
		      sizeof(ULONG));
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_HWDETECT, "RegSetValue() failed (Error %u)\n", (int)Error));
    }

  /* Set 'Identifier' value (CPU and FPU) */
  DbgPrint((DPRINT_HWDETECT, "Identifier: %s\n", Identifier));

  Error = RegSetValue(CpuInstKey,
		      "Identifier",
		      REG_SZ,
		      (PUCHAR)Identifier,
		      strlen(Identifier) + 1);
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_HWDETECT, "RegSetValue() failed (Error %u)\n", (int)Error));
    }

  Error = RegSetValue(FpuInstKey,
		      "Identifier",
		      REG_SZ,
		      (PUCHAR)Identifier,
		      strlen(Identifier) + 1);
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_HWDETECT, "RegSetValue() failed (Error %u)\n", (int)Error));
    }

  /* Set 'VendorIdentifier' value (CPU only) */
  DbgPrint((DPRINT_HWDETECT, "Vendor Identifier: %s\n", VendorIdentifier));

  Error = RegSetValue(CpuInstKey,
		      "VendorIdentifier",
		      REG_SZ,
		      (PUCHAR)VendorIdentifier,
		      strlen(VendorIdentifier) + 1);
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_HWDETECT, "RegSetValue() failed (Error %u)\n", (int)Error));
    }

  /* FIXME: Set 'Update Signature' value (CPU only) */

  /* FIXME: Set 'Update Status' value (CPU only) */

  /* Set '~MHz' value (CPU only) */
  if (((CpuEntry->CpuSignature >> 8) & 0x0F) >= 5)
    {
      CpuSpeed = GetCpuSpeed();

      Error = RegSetValue(CpuInstKey,
			  "~MHz",
			  REG_DWORD,
			  (PUCHAR)&CpuSpeed,
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


static BOOL
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
		       "CentralProcessor",
		       &CpuKey);
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_HWDETECT, "RegCreateKey() failed (Error %u)\n", (int)Error));
      return;
    }

  /* Create the 'FloatingPointProcessor' key */
  Error = RegCreateKey(SystemKey,
		       "FloatingPointProcessor",
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
