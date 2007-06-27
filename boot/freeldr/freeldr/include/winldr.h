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
typedef
VOID
NTAPI
(*KERNEL_ENTRY_POINT) (PLOADER_PARAMETER_BLOCK LoaderBlock);


// Some definitions
#define SECTOR_SIZE 512

// Descriptors
#define NUM_GDT 28 //15. The kernel wants 0xD8 as a last GDT entry offset
#define NUM_IDT 0x100 // only 16 are used though

// conversion.c
PVOID VaToPa(PVOID Va);
PVOID PaToVa(PVOID Pa);
VOID List_PaToVa(LIST_ENTRY *ListEntry);
VOID ConvertConfigToVA(PCONFIGURATION_COMPONENT_DATA Start);

// peloader.c
BOOLEAN
WinLdrLoadImage(IN PCHAR FileName,
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

// wlmemory.c
BOOLEAN
WinLdrTurnOnPaging(IN OUT PLOADER_PARAMETER_BLOCK LoaderBlock,
                   ULONG PcrBasePage,
                   ULONG TssBasePage,
                   PVOID GdtIdt);

// wlregistry.c
BOOLEAN WinLdrLoadAndScanSystemHive(IN OUT PLOADER_PARAMETER_BLOCK LoaderBlock,
                                    IN LPCSTR DirectoryPath);


#endif // defined __WINLDR_H
