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
#include <string.h>

//#define NDEBUG
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
  sp = (PUSHORT)((tf->ss0 & 0xFFFF) * 16 + (tf->esp0 & 0xFFFF));
   
  DPRINT("KeV86GPF handling %x at %x:%x ss:sp %x:%x\n",
	 ip[0], tf->cs, tf->eip, tf->ss0, tf->esp0);
 
  switch (ip[0])
    {
      /* sti */
    case 0xFB:
      if (tf->regs->Flags & KV86M_EMULATE_CLI_STI)
	{
	  tf->eip++;
	  tf->regs->Vif = 1;
	  return(0);
	}
      break;

      /* cli */
    case 0xFA:
      if (tf->regs->Flags & KV86M_EMULATE_CLI_STI)
	{
	  tf->eip++;
	  tf->regs->Vif = 0;
	  return(0);
	}
      break;

      /* pushf */
    case 0x9C:
      if (tf->regs->Flags & KV86M_EMULATE_CLI_STI)
	{
	  tf->eip++;
	  tf->esp0 = tf->esp0 - 2;
	  sp = sp - 1;
	  sp[0] = tf->eflags & 0xFFFF;
	  if (tf->regs->Vif)
	    {
	      sp[0] = sp[0] | INTERRUPT_FLAG;
	    }
	  return(0);
	}
      break;

      /* popf */
     case 0x9D:
       if (tf->regs->Flags & KV86M_EMULATE_CLI_STI)
	 {
	   tf->eip++;
	   tf->eflags = tf->eflags & (~0xFFFF);
	   tf->eflags = tf->eflags | sp[0];
	   if (tf->eflags & INTERRUPT_FLAG)
	     {
	       tf->regs->Vif = 1;
	     }
	   else
	     {
	       tf->regs->Vif = 0;
	    }
	   tf->eflags = tf->eflags & (~INTERRUPT_FLAG);
	   tf->esp0 = tf->esp0 + 2;
	   return(0);
	 }
       break;

      /* iret */
    case 0xCF:
      if (tf->regs->Flags & KV86M_EMULATE_CLI_STI)
	{
	  tf->eip = sp[0];
	  tf->cs = sp[1];
	  tf->eflags = tf->eflags & (~0xFFFF);
	  tf->eflags = tf->eflags | sp[2];
	  if (tf->eflags & INTERRUPT_FLAG)
	    {
	      tf->regs->Vif = 1;
	    }
	  else
	    {
	      tf->regs->Vif = 0;
	    }
	  tf->eflags = tf->eflags & (~INTERRUPT_FLAG);
	  tf->esp0 = tf->esp0 + 6;
	  return(0);
	}
      break;

      /* out imm8, al */
    case 0xE6:
      if (tf->regs->Flags & KV86M_ALLOW_IO_PORT_ACCESS)
	{
	  WRITE_PORT_UCHAR((PUCHAR)(ULONG)ip[1], 
			   tf->eax & 0xFF);
	  tf->eip = tf->eip + 2;
	  return(0);
	}
      break;

      /* out imm8, ax */
    case 0xE7:
      if (tf->regs->Flags & KV86M_ALLOW_IO_PORT_ACCESS)
	{
	  WRITE_PORT_USHORT((PUSHORT)(ULONG)ip[1], tf->eax & 0xFFFF);
	  tf->eip = tf->eip + 2;
	  return(0);
	}
      break;

      /* out dx, al */
    case 0xEE:
      if (tf->regs->Flags & KV86M_ALLOW_IO_PORT_ACCESS)
	{
	  WRITE_PORT_UCHAR((PUCHAR)(tf->edx & 0xFFFF), tf->eax & 0xFF);
	  tf->eip = tf->eip + 1;
	  return(0);
	}
      break;

      /* out dx, ax */
    case 0xEF:
      if (tf->regs->Flags & KV86M_ALLOW_IO_PORT_ACCESS)
	{
	  WRITE_PORT_USHORT((PUSHORT)(tf->edx & 0xFFFF), tf->eax & 0xFFFF);
	  tf->eip = tf->eip + 1;
	  return(0);
	}
      break;

      /* in al, imm8 */
    case 0xE4:
      if (tf->regs->Flags & KV86M_ALLOW_IO_PORT_ACCESS)
	{
	  UCHAR v;
	  
	  v = READ_PORT_UCHAR((PUCHAR)(ULONG)ip[1]);
	  tf->eax = tf->eax & (~0xFF);
	  tf->eax = tf->eax | v;
	  tf->eip = tf->eip + 2;
	  return(0);
	}
      break;

      /* in ax, imm8 */
    case 0xE5:
      if (tf->regs->Flags & KV86M_ALLOW_IO_PORT_ACCESS)
	{
	  USHORT v;

	  v = READ_PORT_USHORT((PUSHORT)(ULONG)ip[1]);
	  tf->eax = tf->eax & (~0xFFFF);
	  tf->eax = tf->eax | v;
	  tf->eip = tf->eip + 2;
	  return(0);
	}
      break;

      /* in al, dx */
    case 0xEC:
      if (tf->regs->Flags & KV86M_ALLOW_IO_PORT_ACCESS)
	{
	  UCHAR v;

	  v = READ_PORT_UCHAR((PUCHAR)(tf->edx & 0xFFFF));
	  tf->eax = tf->eax & (~0xFF);
	  tf->eax = tf->eax | v;
	  tf->eip = tf->eip + 1;
	  return(0);
	}
      break;

      /* in ax, dx */
    case 0xED:
      if (tf->regs->Flags & KV86M_ALLOW_IO_PORT_ACCESS)
	{
	  USHORT v;

	  v = READ_PORT_USHORT((PUSHORT)(tf->edx & 0xFFFF));
	  tf->eax = tf->eax & (~0xFFFF);
	  tf->eax = tf->eax | v;
	  tf->eip = tf->eip + 1;
	  return(0);
	}
      break;

      /* Int nn */
    case 0xCD:
      {
	unsigned int inum;
	unsigned int entry;

	inum = ip[1];
	entry = ((unsigned int *)0)[inum];
	
	tf->esp0 = tf->esp0 - 6;
	sp = sp - 3;

	sp[0] = (tf->eip & 0xFFFF) + 2;
	sp[1] = tf->cs & 0xFFFF;
	sp[2] = tf->eflags & 0xFFFF;
	if (tf->regs->Vif == 1)
	  {
	    sp[2] = sp[2] | INTERRUPT_FLAG;
	  }
	DPRINT("sp[0] %x sp[1] %x sp[2] %x\n", sp[0], sp[1], sp[2]);
	tf->eip = entry & 0xFFFF;
	tf->cs = entry >> 16;
	tf->eflags = tf->eflags & (~TRAP_FLAG);

	return(0);
      }

      /* FIXME: Also emulate ins and outs */
      /* FIXME: Handle opcode prefixes */
      /* FIXME: Don't allow the BIOS to write to sensitive I/O ports */
    }
   
  DPRINT("V86GPF unhandled\n");
  *tf->regs->PStatus = STATUS_NONCONTINUABLE_EXCEPTION;
  return(1);
}

ULONG
KeV86Exception(struct trap_frame* tf, ULONG address)
{
  PUCHAR Ip;

  /*
   * Check if we have reached the recovery instruction
   */
  Ip = (PUCHAR)((tf->cs & 0xFFFF) * 16 + (tf->eip & 0xFFFF));
  DbgPrint("tf->type %d Ip[0] %x Ip[1] %x Ip[2] %x Ip[3] %x tf->cs %x "
	   "tf->eip %x\n", tf->type, Ip[0], Ip[1], Ip[2], Ip[3], tf->cs,
	   tf->eip);
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
	    DPRINT("V86Exception, halting due to page fault\n");
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

