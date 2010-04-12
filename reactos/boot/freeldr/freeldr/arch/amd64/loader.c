/*
 *  FreeLoader
 *  Copyright (C) 2008 - 2009 Timo Kreuzer (timo.kreuzer@reactor.org)
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
#define _NTSYSTEM_
#include <freeldr.h>

#define NDEBUG
#include <debug.h>

/* Page Directory and Tables for non-PAE Systems */
extern ULONG_PTR NextModuleBase;
extern ULONG_PTR KernelBase;
ULONG_PTR GdtBase, IdtBase, TssBase;
extern ROS_KERNEL_ENTRY_POINT KernelEntryPoint;

PPAGE_DIRECTORY_AMD64 pPML4;
PVOID pIdt, pGdt;

/* FUNCTIONS *****************************************************************/

void
EnableA20()
{
    /* Already done */
}

/*++
 * FrLdrStartup
 * INTERNAL
 *
 *     Prepares the system for loading the Kernel.
 *
 * Params:
 *     Magic - Multiboot Magic
 *
 * Returns:
 *     None.
 *
 * Remarks:
 *     None.
 *
 *--*/
VOID
NTAPI
FrLdrStartup(ULONG Magic)
{
	DbgPrint("ReactOS loader is unsupported! Halting.\n", KernelEntryPoint);
    for(;;);
}

