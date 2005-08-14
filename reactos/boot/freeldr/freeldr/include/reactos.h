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


///////////////////////////////////////////////////////////////////////////////////////
//
// ReactOS Loading Functions
//
///////////////////////////////////////////////////////////////////////////////////////
VOID LoadAndBootReactOS(PCHAR OperatingSystemName);

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
BOOL DissectArcPath(CHAR *ArcPath, CHAR *BootPath, ULONG* BootDrive, ULONG* BootPartition);
VOID ConstructArcPath(PCHAR ArcPath, PCHAR SystemFolder, ULONG Disk, ULONG Partition);
ULONG ConvertArcNameToBiosDriveNumber(PCHAR ArcPath);

///////////////////////////////////////////////////////////////////////////////////////
//
// Loader Functions And Definitions
//
///////////////////////////////////////////////////////////////////////////////////////
 
extern LOADER_PARAMETER_BLOCK LoaderBlock; /* Multiboot info structure passed to kernel */
extern char					reactos_kernel_cmdline[255];	// Command line passed to kernel
extern LOADER_MODULE		reactos_modules[64];		// Array to hold boot module info loaded for the kernel
extern char					reactos_module_strings[64][256];	// Array to hold module names
extern unsigned long		reactos_memory_map_descriptor_size;
extern memory_map_t			reactos_memory_map[32];		// Memory map

VOID FASTCALL FrLdrSetupPae(ULONG Magic);
VOID FASTCALL FrLdrSetupPageDirectory(VOID);
VOID FASTCALL FrLdrGetPaeMode(VOID);
BOOL STDCALL FrLdrMapKernel(FILE *KernelImage);
ULONG_PTR STDCALL FrLdrCreateModule(LPSTR ModuleName);
ULONG_PTR STDCALL FrLdrLoadModule(FILE *ModuleImage, LPSTR ModuleName, PULONG ModuleSize);
BOOL STDCALL FrLdrCloseModule(ULONG_PTR ModuleBase, ULONG dwModuleSize);
VOID STDCALL FrLdrStartup(ULONG Magic);
typedef VOID (FASTCALL *ASMCODE)(ULONG Magic, PLOADER_PARAMETER_BLOCK LoaderBlock);

#endif // defined __REACTOS_H
