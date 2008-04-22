/*
 *  FreeLoader
 *  Copyright (C) 1998-2003  Brian Palmer    <brianp@sginet.com>
 *  Copyright (C) 2006       Aleksey Bragin  <aleksey@reactos.org>
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

#ifndef __WINLDR_H
#define __WINLDR_H


///////////////////////////////////////////////////////////////////////////////////////
//
// ReactOS Loading Functions
//
///////////////////////////////////////////////////////////////////////////////////////
VOID LoadAndBootWindows(PCSTR OperatingSystemName, WORD OperatingSystemVersion);

/* Entry-point to kernel */
typedef VOID (NTAPI *KERNEL_ENTRY_POINT) (PLOADER_PARAMETER_BLOCK LoaderBlock);


// Some definitions
#define SECTOR_SIZE 512

// Descriptors
#define NUM_GDT 128 // Must be 128
#define NUM_IDT 0x100 // only 16 are used though. Must be 0x100

// conversion.c
PVOID VaToPa(PVOID Va);
PVOID PaToVa(PVOID Pa);
VOID List_PaToVa(LIST_ENTRY *ListEntry);
VOID ConvertConfigToVA(PCONFIGURATION_COMPONENT_DATA Start);

// peloader.c
BOOLEAN
WinLdrLoadImage(IN PCHAR FileName,
                TYPE_OF_MEMORY MemoryType,
                OUT PVOID *ImageBasePA);


BOOLEAN
WinLdrAllocateDataTableEntry(IN OUT PLOADER_PARAMETER_BLOCK WinLdrBlock,
                             IN PCCH BaseDllName,
                             IN PCCH FullDllName,
                             IN PVOID BasePA,
                             OUT PLDR_DATA_TABLE_ENTRY *NewEntry);

BOOLEAN
WinLdrScanImportDescriptorTable(IN OUT PLOADER_PARAMETER_BLOCK WinLdrBlock,
                                IN PCCH DirectoryPath,
                                IN PLDR_DATA_TABLE_ENTRY ScanDTE);

// winldr.c
PVOID WinLdrLoadModule(PCSTR ModuleName, ULONG *Size,
                       TYPE_OF_MEMORY MemoryType);

// wlmemory.c
BOOLEAN
WinLdrTurnOnPaging(IN OUT PLOADER_PARAMETER_BLOCK LoaderBlock,
                   ULONG PcrBasePage,
                   ULONG TssBasePage,
                   PVOID GdtIdt);

// wlregistry.c
BOOLEAN WinLdrLoadAndScanSystemHive(IN OUT PLOADER_PARAMETER_BLOCK LoaderBlock,
                                    IN LPCSTR DirectoryPath);


/* FIXME: Should be moved to NDK, and respective ACPI header files */
typedef struct _ACPI_BIOS_DATA
{
    PHYSICAL_ADDRESS RSDTAddress;
    ULONGLONG Count;
    BIOS_MEMORY_MAP MemoryMap[1]; /* Count of BIOS memory map entries */
} ACPI_BIOS_DATA, *PACPI_BIOS_DATA;

#include <pshpack1.h>
typedef struct  /* Root System Descriptor Pointer */
{
	CHAR             signature [8];          /* contains "RSD PTR " */
	UCHAR            checksum;               /* to make sum of struct == 0 */
	CHAR             oem_id [6];             /* OEM identification */
	UCHAR            revision;               /* Must be 0 for 1.0, 2 for 2.0 */
	ULONG            rsdt_physical_address;  /* 32-bit physical address of RSDT */
	ULONG            length;                 /* XSDT Length in bytes including hdr */
	ULONGLONG        xsdt_physical_address;  /* 64-bit physical address of XSDT */
	UCHAR            extended_checksum;      /* Checksum of entire table */
	CHAR             reserved [3];           /* reserved field must be 0 */
} RSDP_DESCRIPTOR, *PRSDP_DESCRIPTOR;
#include <poppack.h>


#endif // defined __WINLDR_H
