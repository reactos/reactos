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
	
#include <freeldr.h>
#include <rtl.h>
#include <arch.h>
#include <mm.h>
#include <debug.h>
#include <bootmgr.h>
#include <fs.h>

// Variables BootDrive & BootPartition moved to asmcode.S
//U32				BootDrive = 0;							// BIOS boot drive, 0-A:, 1-B:, 0x80-C:, 0x81-D:, etc.
//U32				BootPartition = 0;						// Boot Partition, 1-4

VOID BootMain(VOID)
{

	EnableA20();

#ifdef DEBUG
	DebugInit();
#endif

	DbgPrint((DPRINT_WARNING, "BootMain() called. BootDrive = 0x%x BootPartition = %d\n", BootDrive, BootPartition));

	if (!MmInitializeMemoryManager())
	{
		printf("Press any key to reboot.\n");
		getch();
		return;
	}

	RunLoader();
}
