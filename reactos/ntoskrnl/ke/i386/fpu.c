/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
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
 * FILE:            ntoskrnl/ke/i386/fpu.c
 * PURPOSE:         Handles the FPU
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ke.h>
#include <internal/mm.h>
#include <internal/ps.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

static ULONG HardwareMathSupport;

/* FUNCTIONS *****************************************************************/

VOID 
KiCheckFPU(VOID)
{
   unsigned short int status;
   int cr0;
   
   HardwareMathSupport = 0;
   
   __asm__("movl %%cr0, %0\n\t" : "=a" (cr0));
   /* Set NE and MP. */
   cr0 = cr0 | 0x22;
   /* Clear EM */
   cr0 = cr0 & (~0x4);
   __asm__("movl %0, %%cr0\n\t" : : "a" (cr0));

   __asm__("clts\n\t");
   __asm__("fninit\n\t");
   __asm__("fstsw %0\n\t" : "=a" (status));
   if (status != 0)
     {
	__asm__("movl %%cr0, %0\n\t" : "=a" (cr0));
	/* Set the EM flag in CR0 so any FPU instructions cause a trap. */
	cr0 = cr0 | 0x4;
	__asm__("movl %0, %%cr0\n\t" :
		: "a" (cr0));
	DbgPrint("No FPU detected\n");
	return;
     }
   /* FIXME: Do fsetpm */
   HardwareMathSupport = 1;   
}
