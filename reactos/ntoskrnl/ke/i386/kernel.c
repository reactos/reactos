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
   PKPCR KPCR;
   extern USHORT KiGdt[];

   KiCheckFPU();

   KeInitExceptions ();
   KeInitInterrupts ();

   /* Initialize the initial PCR region. We can't allocate a page
      with MmAllocPage() here because MmInit1() has not yet been
      called, so we use a predefined page in low memory */
   KPCR = (PKPCR)KPCR_BASE;
   memset(KPCR, 0, PAGESIZE);
   KPCR->Self = (PKPCR)KPCR_BASE;
   KPCR->Irql = HIGH_LEVEL;
   KPCR->GDT = (PUSHORT)&KiGdt;
   KPCR->IDT = (PUSHORT)&KiIdt;
   KiPcrInitDone = 1;
}

VOID 
KeInit2(VOID)
{
   KeInitDpc();
   KeInitializeBugCheck();
   KeInitializeDispatcher();
   KeInitializeTimerImpl();
}
