/*
 *  ReactOS kernel
 *  Copyright (C) 2000 David Welch <welch@cwcom.net>
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
 * FILE:            ntoskrnl/ke/i386/v86m.c
 * PURPOSE:         Support for v86 mode
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/v86m.h>
#include <internal/trap.h>
#include <internal/mm.h>
#include <internal/i386/segment.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

#define INTERRUPT_FLAG     (1 << 9)
#define TRAP_FLAG          (1 << 8)

/* FUNCTIONS *****************************************************************/

ULONG
KeV86GPF(struct trap_frame* tf)
{
  PUCHAR ip;
  PUSHORT sp;

  ip = (PUCHAR)((tf->cs & 0xFFFF) * 16 + (tf->eip & 0xFFFF));
  sp = (PUSHORT)((tf->ss0 & 0xFFFF) * 16 + (tf->esp & 0xFFFF));

  switch (ip[0])
    {
      /* Int nn */
    case 0xCD:
      {
	unsigned int inum;
	unsigned int entry;

	inum = ip[1];
	entry = ((unsigned int *)0)[inum];
	
	tf->esp0 = tf->esp0 - 6;
	sp = sp - 6;

	sp[0] = (tf->eip & 0xFFFF) + 2;
	sp[1] = tf->cs & 0xFFFF;
	sp[2] = tf->eflags & 0xFFFF;
	if (tf->regs->Vif == 1)
	  {
	    sp[2] = sp[2] | INTERRUPT_FLAG;
	  }
	tf->eip = entry & 0xFFFF;
	tf->cs = entry >> 16;
	tf->eflags = tf->eflags & (~TRAP_FLAG);

	return(0);
      }
    }

  *tf->regs->PStatus = STATUS_NONCONTINUABLE_EXCEPTION;
  return(1);
}

ULONG
KeV86Exception(struct trap_frame* tf, ULONG address)
{
  PVOID Ip;

  /*
   * Check if we have reached the recovery instruction
   */
  Ip = (PVOID)((tf->cs & 0xFFFF) * 16 + (tf->eip & 0xFFFF));
  if (tf->type == 6 &&
      memcmp(Ip, tf->regs->RecoveryInstruction, 4) == 0 &&
      (tf->cs * 16 + tf->eip) == tf->regs->RecoveryAddress)
    {
      *tf->regs->PStatus = STATUS_SUCCESS;
      return(1);
    }

  /*
   * Handle the exceptions
   */
  switch (tf->type)
    {
      /* Divide error */
    case 0:
      *tf->regs->PStatus = STATUS_NONCONTINUABLE_EXCEPTION;
      return(1);

      /* Single step */
    case 1:
      *tf->regs->PStatus = STATUS_NONCONTINUABLE_EXCEPTION;
      return(1);

      /* NMI */
    case 2:
      *tf->regs->PStatus = STATUS_NONCONTINUABLE_EXCEPTION;
      return(1);

      /* Breakpoint */
    case 3:
      *tf->regs->PStatus = STATUS_NONCONTINUABLE_EXCEPTION;
      return(1);

      /* Overflow */
    case 4:
      *tf->regs->PStatus = STATUS_NONCONTINUABLE_EXCEPTION;
      return(1);

      /* Array bounds check */
    case 5:
      *tf->regs->PStatus = STATUS_NONCONTINUABLE_EXCEPTION;
      return(1);

      /* Invalid opcode */
    case 6:
      *tf->regs->PStatus = STATUS_NONCONTINUABLE_EXCEPTION;
      return(1);

      /* Device not available */
    case 7:
      *tf->regs->PStatus = STATUS_NONCONTINUABLE_EXCEPTION;
      return(1);

      /* Double fault */
    case 8:
      *tf->regs->PStatus = STATUS_NONCONTINUABLE_EXCEPTION;
      return(1);

      /* Intel reserved */
    case 9:
      *tf->regs->PStatus = STATUS_NONCONTINUABLE_EXCEPTION;
      return(1);

      /* Invalid TSS */
    case 10:
      *tf->regs->PStatus = STATUS_NONCONTINUABLE_EXCEPTION;
      return(1);

      /* Segment not present */
    case 11:
      *tf->regs->PStatus = STATUS_NONCONTINUABLE_EXCEPTION;;
      return(1);

      /* Stack fault */
    case 12:
      *tf->regs->PStatus = STATUS_NONCONTINUABLE_EXCEPTION;
      return(1);

      /* General protection fault */
    case 13:
      return(KeV86GPF(tf));

      /* Page fault */
    case 14:
      {
	NTSTATUS Status;

	Status = MmPageFault(USER_CS,
			     &tf->eip,
			     NULL,
			     address,
			     tf->error_code);
	if (!NT_SUCCESS(Status))
	  {
	    *tf->regs->PStatus = STATUS_NONCONTINUABLE_EXCEPTION;
	    return(1);
	  }
	return(0);
      }

      /* Intel reserved */
    case 15:
    case 16:
      *tf->regs->PStatus = STATUS_NONCONTINUABLE_EXCEPTION;
      return(1);
      
      /* Alignment check */
    case 17:
      *tf->regs->PStatus = STATUS_NONCONTINUABLE_EXCEPTION;
      return(1);

    default:
      *tf->regs->PStatus = STATUS_NONCONTINUABLE_EXCEPTION;
      return(1);
    }
}

