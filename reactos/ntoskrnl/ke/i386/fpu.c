/* $Id: fpu.c,v 1.13 2004/08/15 16:39:05 chorns Exp $
 *
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

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

ULONG HardwareMathSupport;

/* FUNCTIONS *****************************************************************/

VOID INIT_FUNCTION
KiCheckFPU(VOID)
{
   unsigned short int status;
   int cr0_;
   
   HardwareMathSupport = 0;
   
#if defined(__GNUC__)
   __asm__("movl %%cr0, %0\n\t" : "=a" (cr0_));
   /* Set NE and MP. */
   cr0_ = cr0_ | 0x22;
   /* Clear EM */
   cr0_ = cr0_ & (~0x4);
   __asm__("movl %0, %%cr0\n\t" : : "a" (cr0_));

   __asm__("clts\n\t");
   __asm__("fninit\n\t");
   __asm__("fstsw %0\n\t" : "=a" (status));
   if (status != 0)
     {
	__asm__("movl %%cr0, %0\n\t" : "=a" (cr0_));
	/* Set the EM flag in CR0 so any FPU instructions cause a trap. */
	cr0_ = cr0_ | 0x4;
	__asm__("movl %0, %%cr0\n\t" :
		: "a" (cr0_));
	return;
     }
   /* fsetpm for i287, ignored by i387 */
   __asm__(".byte 0xDB, 0xE4\n\t");
#elif defined(_MSC_VER)
   __asm mov eax, cr0;
   __asm mov cr0_, eax;
   cr0_ |= 0x22;	/* Set NE and MP. */
   cr0_ &= ~0x4;	/* Clear EM */
   __asm
   {
	   mov eax, cr0_;
	   mov cr0, eax;
	   clts;
	   fninit;
	   fstsw status
   }
   if (status != 0)
     {
	__asm mov eax, cr0_;
	__asm or eax, 4; /* Set the EM flag in CR0 so any FPU instructions cause a trap. */
	__asm mov cr0, eax;
	return;
     }
   /* fsetpm for i287, ignored by i387 */
   __asm _emit 0xDB __asm _emit 0xe4
//   __asm fsetpm;
//   __asm__(".byte 0xDB, 0xE4\n\t");
#else
#error Unknown compiler for inline assembler
#endif

   HardwareMathSupport = 1;
}

/* This is a rather naive implementation of Ke(Save/Restore)FloatingPointState
   which will not work for WDM drivers. Please feel free to improve */

#define FPU_STATE_SIZE 108

NTSTATUS STDCALL
KeSaveFloatingPointState(OUT PKFLOATING_SAVE Save)
{
  char *FpState;

  FpState = ExAllocatePool(PagedPool, FPU_STATE_SIZE);
  if (NULL == FpState)
    {
      return STATUS_INSUFFICIENT_RESOURCES;
    }
  *((PVOID *) Save) = FpState;

#if defined(__GNUC__)
  __asm__("fsave %0\n\t" : "=m" (*FpState));
#elif defined(_MSC_VER)
  __asm mov eax, FpState;
  __asm fsave [eax];
#else
#error Unknown compiler for inline assembler
#endif

  return STATUS_SUCCESS;
}

NTSTATUS STDCALL
KeRestoreFloatingPointState(IN PKFLOATING_SAVE Save)
{
  char *FpState = *((PVOID *) Save);

#if defined(__GNUC__)
  __asm__("frstor %0\n\t" : "=m" (*FpState));
#elif defined(_MSC_VER)
  __asm mov eax, FpState;
  __asm frstor [eax];
#else
#error Unknown compiler for inline assembler
#endif

  ExFreePool(FpState);

  return STATUS_SUCCESS;
}
