/*
 *  FreeLoader
 *  Copyright (C) 1998-2003  Brian Palmer  <brianp@sginet.com>
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

#ifndef __REACTOS_H
#define __REACTOS_H

/* Base Addres of Kernel in Physical Memory */
#define KERNEL_BASE_PHYS 0x800000

/* Bits to shift to convert a Virtual Address into an Offset in the Page Table */
#define PFN_SHIFT 12

/* Bits to shift to convert a Virtual Address into an Offset in the Page Directory */
#define PDE_SHIFT 22
#define PDE_SHIFT_PAE 18

/* Converts a Physical Address Pointer into a Page Frame Number */
#define PaPtrToPfn(p) \
    (((ULONG_PTR)&p) >> PFN_SHIFT)

/* Converts a Physical Address into a Page Frame Number */
#define PaToPfn(p) \
    ((p) >> PFN_SHIFT)

#define STARTUP_BASE                0xC0000000
#define HYPERSPACE_BASE             0xC0400000
#define HAL_BASE                    0xFFC00000

#define LowMemPageTableIndex        0
#define StartupPageTableIndex       (STARTUP_BASE >> 22)
#define HyperspacePageTableIndex    (HYPERSPACE_BASE >> 22)
#define HalPageTableIndex           (HAL_BASE >> 22)

typedef struct _PAGE_DIRECTORY_X86
{
    HARDWARE_PTE Pde[1024];
} PAGE_DIRECTORY_X86, *PPAGE_DIRECTORY_X86;

///////////////////////////////////////////////////////////////////////////////////////
//
// ReactOS Loading Functions
//
///////////////////////////////////////////////////////////////////////////////////////
VOID LoadAndBootReactOS(PCSTR OperatingSystemName);

///////////////////////////////////////////////////////////////////////////////////////
//
// ReactOS Setup Loader Functions
//
///////////////////////////////////////////////////////////////////////////////////////
VOID ReactOSRunSetupLoader(VOID);

///////////////////////////////////////////////////////////////////////////////////////
//
// ARC Path Functions
//
///////////////////////////////////////////////////////////////////////////////////////
BOOLEAN DissectArcPath(CHAR *ArcPath, CHAR *BootPath, ULONG* BootDrive, ULONG* BootPartition);
VOID ConstructArcPath(PCHAR ArcPath, PCHAR SystemFolder, ULONG Disk, ULONG Partition);
ULONG ConvertArcNameToBiosDriveNumber(PCHAR ArcPath);

///////////////////////////////////////////////////////////////////////////////////////
//
// Loader Functions And Definitions
//
///////////////////////////////////////////////////////////////////////////////////////
extern ROS_LOADER_PARAMETER_BLOCK LoaderBlock; /* Multiboot info structure passed to kernel */
extern char					reactos_kernel_cmdline[255];	// Command line passed to kernel
extern LOADER_MODULE		reactos_modules[64];		// Array to hold boot module info loaded for the kernel
extern char					reactos_module_strings[64][256];	// Array to hold module names
typedef struct _reactos_mem_data {
    unsigned long			memory_map_descriptor_size;
    memory_map_t			memory_map[32];		// Memory map
} reactos_mem_data_t;
extern reactos_mem_data_t reactos_mem_data;
#define reactos_memory_map_descriptor_size reactos_mem_data.memory_map_descriptor_size
#define reactos_memory_map reactos_mem_data.memory_map

VOID FASTCALL FrLdrSetupPae(ULONG Magic);
VOID FASTCALL FrLdrSetupPageDirectory(VOID);
VOID FASTCALL FrLdrGetPaeMode(VOID);
BOOLEAN NTAPI FrLdrMapKernel(FILE *KernelImage);
ULONG_PTR NTAPI FrLdrCreateModule(LPCSTR ModuleName);
ULONG_PTR NTAPI FrLdrLoadModule(FILE *ModuleImage, LPCSTR ModuleName, PULONG ModuleSize);
BOOLEAN NTAPI FrLdrCloseModule(ULONG_PTR ModuleBase, ULONG dwModuleSize);
VOID NTAPI FrLdrStartup(ULONG Magic);
typedef VOID (FASTCALL *ASMCODE)(ULONG Magic, PROS_LOADER_PARAMETER_BLOCK LoaderBlock);

PVOID
NTAPI
FrLdrMapImage(
    IN FILE *Image,
    IN PCHAR ShortName,
    IN ULONG ImageType
);

PVOID
NTAPI
FrLdrLoadImage(
    IN PCHAR szFileName,
    IN INT nPos,
    IN ULONG ImageType
);

#endif // defined __REACTOS_H
