/*
 *  ReactOS kernel
 *  Copyright (C) 2000  ReactOS Team
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
/*
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/i386/ldt.c
 * PURPOSE:         LDT managment
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>


/* GLOBALS *******************************************************************/

/*
 * Empty LDT shared by every process that doesn't have its own.
 */
STATIC UCHAR KiNullLdt[8];

/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL
NtSetLdtEntries (HANDLE	Thread,
		 ULONG FirstEntry,
		 PULONG	Entries)
{
  return(STATUS_NOT_IMPLEMENTED);
}

VOID
Ki386InitializeLdt(VOID)
{
  PUSHORT Gdt = KeGetCurrentKPCR()->GDT;
  unsigned int base, length;

  /*
   * Set up an a descriptor for the LDT
   */
  base = (unsigned int)&KiNullLdt;
  length = sizeof(KiNullLdt) - 1;
  
  Gdt[(LDT_SELECTOR / 2) + 0] = (length & 0xFFFF);
  Gdt[(LDT_SELECTOR / 2) + 1] = (base & 0xFFFF);
  Gdt[(LDT_SELECTOR / 2) + 2] = ((base & 0xFF0000) >> 16) | 0x8200;
  Gdt[(LDT_SELECTOR / 2) + 3] = ((length & 0xF0000) >> 16) |
    ((base & 0xFF000000) >> 16);
}
