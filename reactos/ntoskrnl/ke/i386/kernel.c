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
 * FILE:            ntoskrnl/ke/kernel.c
 * PURPOSE:         Initializes the kernel
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ke.h>
#include <internal/mm.h>
#include <internal/ps.h>
#include <internal/i386/fpu.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

ULONG KiPcrInitDone = 0;

/* FUNCTIONS *****************************************************************/

VOID 
KeInit1(VOID)
{
   KiCheckFPU();

   KeInitExceptions ();
   KeInitInterrupts ();
}

VOID 
KeInit2(VOID)
{
   PVOID PcrPage;
   
   KeInitDpc();
   KeInitializeBugCheck();
   KeInitializeDispatcher();
   KeInitializeTimerImpl();
   
   /*
    * Initialize the PCR region. 
    * FIXME: This should be per-processor.
    */
   PcrPage = MmAllocPage(0);
   if (PcrPage == NULL)
     {
	DPRINT1("No memory for PCR page\n");
	KeBugCheck(0);
     }
   MmCreateVirtualMapping(NULL,
			  (PVOID)KPCR_BASE,
			  PAGE_READWRITE,
			  (ULONG)PcrPage);
   memset((PVOID)KPCR_BASE, 0, 4096);
   KiPcrInitDone = 1;
}






