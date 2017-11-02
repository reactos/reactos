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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#pragma once

#include <arc/setupblk.h>

// See freeldr/ntldr/winldr.h
#define TAG_WLDR_DTE 'eDlW'
#define TAG_WLDR_BDE 'dBlW'
#define TAG_WLDR_NAME 'mNlW'


// Some definitions

// FIXME: This one has nothing to do here!!
#define SECTOR_SIZE 512

// Descriptors
#define NUM_GDT 128     // Must be 128
#define NUM_IDT 0x100   // Only 16 are used though. Must be 0x100

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

typedef struct _ARC_DISK_SIGNATURE_EX
{
    ARC_DISK_SIGNATURE DiskSignature;
    CHAR ArcName[MAX_PATH];
} ARC_DISK_SIGNATURE_EX, *PARC_DISK_SIGNATURE_EX;

///////////////////////////////////////////////////////////////////////////////////////
//
// ReactOS Loading Functions
//
///////////////////////////////////////////////////////////////////////////////////////
VOID LoadAndBootWindows(IN OperatingSystemItem* OperatingSystem,
                        IN USHORT OperatingSystemVersion);

VOID
LoadReactOSSetup(IN OperatingSystemItem* OperatingSystem,
                 IN USHORT OperatingSystemVersion);


// conversion.c and conversion.h
PVOID VaToPa(PVOID Va);
PVOID PaToVa(PVOID Pa);
VOID List_PaToVa(_In_ LIST_ENTRY *ListEntry);


// peloader.c
BOOLEAN
WinLdrLoadImage(IN PCHAR FileName,
                TYPE_OF_MEMORY MemoryType,
                OUT PVOID *ImageBasePA);

BOOLEAN
WinLdrAllocateDataTableEntry(IN OUT PLIST_ENTRY ModuleListHead,
                             IN PCCH BaseDllName,
                             IN PCCH FullDllName,
                             IN PVOID BasePA,
                             OUT PLDR_DATA_TABLE_ENTRY *NewEntry);

BOOLEAN
WinLdrScanImportDescriptorTable(IN OUT PLIST_ENTRY ModuleListHead,
                                IN PCCH DirectoryPath,
                                IN PLDR_DATA_TABLE_ENTRY ScanDTE);

BOOLEAN
WinLdrCheckForLoadedDll(IN OUT PLIST_ENTRY ModuleListHead,
                        IN PCH DllName,
                        OUT PLDR_DATA_TABLE_ENTRY *LoadedEntry);


// arch/xxx/winldr.c
BOOLEAN
MempSetupPaging(IN PFN_NUMBER StartPage,
                IN PFN_NUMBER NumberOfPages,
                IN BOOLEAN KernelMapping);

VOID
MempUnmapPage(PFN_NUMBER Page);

VOID
MempDump(VOID);

VOID
WinLdrSetupMachineDependent(PLOADER_PARAMETER_BLOCK LoaderBlock);

VOID
WinLdrSetProcessorContext(VOID);
