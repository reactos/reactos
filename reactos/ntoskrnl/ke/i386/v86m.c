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
KeV86GPF(PKV86M_TRAP_FRAME VTf, PKTRAP_FRAME Tf)
{
  PUCHAR ip;
  PUSHORT sp;

  ip = (PUCHAR)((Tf->Cs & 0xFFFF) * 16 + (Tf->Eip & 0xFFFF));
  sp = (PUSHORT)((Tf->Ss & 0xFFFF) * 16 + (Tf->Esp & 0xFFFF));
   
  DPRINT("KeV86GPF handling %x at %x:%x ss:sp %x:%x Flags %x\n",
	 ip[0], Tf->Cs, Tf->Eip, Tf->Ss, Tf->Esp, VTf->regs->Flags);
 
  switch (ip[0])
    {
      /* sti */
    case 0xFB:
      if (VTf->regs->Flags & KV86M_EMULATE_CLI_STI)
	{
	  Tf->Eip++;
	  VTf->regs->Vif = 1;
	  return(0);
	}
      break;

      /* cli */
    case 0xFA:
      if (VTf->regs->Flags & KV86M_EMULATE_CLI_STI)
	{
	  Tf->Eip++;
	  VTf->regs->Vif = 0;
	  return(0);
	}
      break;

      /* pushf */
    case 0x9C:
      if (VTf->regs->Flags & KV86M_EMULATE_CLI_STI)
	{
	  Tf->Eip++;
	  Tf->Esp = Tf->Esp - 2;
	  sp = sp - 1;
	  sp[0] = Tf->Eflags & 0xFFFF;
	  if (VTf->regs->Vif)
	    {
	      sp[0] = sp[0] | INTERRUPT_FLAG;
	    }
	  return(0);
	}
      break;

      /* popf */
     case 0x9D:
       if (VTf->regs->Flags & KV86M_EMULATE_CLI_STI)
	 {
	   Tf->Eip++;
	   Tf->Eflags = Tf->Eflags & (~0xFFFF);
	   Tf->Eflags = Tf->Eflags | sp[0];
	   if (Tf->Eflags & INTERRUPT_FLAG)
	     {
	       VTf->regs->Vif = 1;
	     }
	   else
	     {
	       VTf->regs->Vif = 0;
	    }
	   Tf->Eflags = Tf->Eflags & (~INTERRUPT_FLAG);
	   Tf->Esp = Tf->Esp + 2;
	   return(0);
	 }
       break;

      /* iret */
    case 0xCF:
      if (VTf->regs->Flags & KV86M_EMULATE_CLI_STI)
	{
	  Tf->Eip = sp[0];
	  Tf->Cs = sp[1];
	  Tf->Eflags = Tf->Eflags & (~0xFFFF);
	  Tf->Eflags = Tf->Eflags | sp[2];
	  if (Tf->Eflags & INTERRUPT_FLAG)
	    {
	      VTf->regs->Vif = 1;
	    }
	  else
	    {
	      VTf->regs->Vif = 0;
	    }
	  Tf->Eflags = Tf->Eflags & (~INTERRUPT_FLAG);
	  Tf->Esp = Tf->Esp + 6;
	  return(0);
	}
      break;

      /* out imm8, al */
    case 0xE6:
      if (VTf->regs->Flags & KV86M_ALLOW_IO_PORT_ACCESS)
	{
	  WRITE_PORT_UCHAR((PUCHAR)(ULONG)ip[1], 
			   Tf->Eax & 0xFF);
	  Tf->Eip = Tf->Eip + 2;
	  return(0);
	}
      break;

      /* out imm8, ax */
    case 0xE7:
      if (VTf->regs->Flags & KV86M_ALLOW_IO_PORT_ACCESS)
	{
	  WRITE_PORT_USHORT((PUSHORT)(ULONG)ip[1], Tf->Eax & 0xFFFF);
	  Tf->Eip = Tf->Eip + 2;
	  return(0);
	}
      break;

      /* out dx, al */
    case 0xEE:
      if (VTf->regs->Flags & KV86M_ALLOW_IO_PORT_ACCESS)
	{
	  WRITE_PORT_UCHAR((PUCHAR)(Tf->Edx & 0xFFFF), Tf->Eax & 0xFF);
	  Tf->Eip = Tf->Eip + 1;
	  return(0);
	}
      break;

      /* out dx, ax */
    case 0xEF:
      if (VTf->regs->Flags & KV86M_ALLOW_IO_PORT_ACCESS)
	{
	  WRITE_PORT_USHORT((PUSHORT)(Tf->Edx & 0xFFFF), Tf->Eax & 0xFFFF);
	  Tf->Eip = Tf->Eip + 1;
	  return(0);
	}
      break;

      /* in al, imm8 */
    case 0xE4:
      if (VTf->regs->Flags & KV86M_ALLOW_IO_PORT_ACCESS)
	{
	  UCHAR v;
	  
	  v = READ_PORT_UCHAR((PUCHAR)(ULONG)ip[1]);
	  Tf->Eax = Tf->Eax & (~0xFF);
	  Tf->Eax = Tf->Eax | v;
	  Tf->Eip = Tf->Eip + 2;
	  return(0);
	}
      break;

      /* in ax, imm8 */
    case 0xE5:
      if (VTf->regs->Flags & KV86M_ALLOW_IO_PORT_ACCESS)
	{
	  USHORT v;

	  v = READ_PORT_USHORT((PUSHORT)(ULONG)ip[1]);
	  Tf->Eax = Tf->Eax & (~0xFFFF);
	  Tf->Eax = Tf->Eax | v;
	  Tf->Eip = Tf->Eip + 2;
	  return(0);
	}
      break;

      /* in al, dx */
    case 0xEC:
      if (VTf->regs->Flags & KV86M_ALLOW_IO_PORT_ACCESS)
	{
	  UCHAR v;

	  v = READ_PORT_UCHAR((PUCHAR)(Tf->Edx & 0xFFFF));
	  Tf->Eax = Tf->Eax & (~0xFF);
	  Tf->Eax = Tf->Eax | v;
	  Tf->Eip = Tf->Eip + 1;
	  return(0);
	}
      break;

      /* in ax, dx */
    case 0xED:
      if (VTf->regs->Flags & KV86M_ALLOW_IO_PORT_ACCESS)
	{
	  USHORT v;

	  v = READ_PORT_USHORT((PUSHORT)(Tf->Edx & 0xFFFF));
	  Tf->Eax = Tf->Eax & (~0xFFFF);
	  Tf->Eax = Tf->Eax | v;
	  Tf->Eip = Tf->Eip + 1;
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
	
	Tf->Esp = Tf->Esp - 6;
	sp = sp - 3;

	sp[0] = (Tf->Eip & 0xFFFF) + 2;
	sp[1] = Tf->Cs & 0xFFFF;
	sp[2] = Tf->Eflags & 0xFFFF;
	if (VTf->regs->Vif == 1)
	  {
	    sp[2] = sp[2] | INTERRUPT_FLAG;
	  }
	DPRINT("sp[0] %x sp[1] %x sp[2] %x\n", sp[0], sp[1], sp[2]);
	Tf->Eip = entry & 0xFFFF;
	Tf->Cs = entry >> 16;
	Tf->Eflags = Tf->Eflags & (~TRAP_FLAG);

	return(0);
      }

      /* FIXME: Also emulate ins and outs */
      /* FIXME: Handle opcode prefixes */
      /* FIXME: Don't allow the BIOS to write to sensitive I/O ports */
    }
   
  DPRINT("V86GPF unhandled\n");
  *VTf->regs->PStatus = STATUS_NONCONTINUABLE_EXCEPTION;
  return(1);
}

ULONG
KeV86Exception(ULONG ExceptionNr, PKTRAP_FRAME Tf, ULONG address)
{
  PUCHAR Ip;
  PKV86M_TRAP_FRAME VTf;

  VTf = (PKV86M_TRAP_FRAME)Tf;

  /*
   * Check if we have reached the recovery instruction
   */
  Ip = (PUCHAR)((Tf->Cs & 0xFFFF) * 16 + (Tf->Eip & 0xFFFF));
  if (ExceptionNr != 14)
    {
      DPRINT("ExceptionNr %d Ip[0] %x Ip[1] %x Ip[2] %x Ip[3] %x Tf->Cs %x "
	     "Tf->Eip %x\n", ExceptionNr, Ip[0], Ip[1], Ip[2], Ip[3], Tf->Cs,
	     Tf->Eip);
      DPRINT("VTf %x VTf->regs %x\n", VTf, VTf->regs);
    }
  if (ExceptionNr == 6 &&
      memcmp(Ip, VTf->regs->RecoveryInstruction, 4) == 0 &&
      (Tf->Cs * 16 + Tf->Eip) == VTf->regs->RecoveryAddress)
    {
      *VTf->regs->PStatus = STATUS_SUCCESS;
      return(1);
    }

  /*
   * Handle the exceptions
   */
  switch (ExceptionNr)
    {
      /* Divide error */
    case 0:
      *VTf->regs->PStatus = STATUS_NONCONTINUABLE_EXCEPTION;
      return(1);

      /* Single step */
    case 1:
      *VTf->regs->PStatus = STATUS_NONCONTINUABLE_EXCEPTION;
      return(1);

      /* NMI */
    case 2:
      *VTf->regs->PStatus = STATUS_NONCONTINUABLE_EXCEPTION;
      return(1);

      /* Breakpoint */
    case 3:
      *VTf->regs->PStatus = STATUS_NONCONTINUABLE_EXCEPTION;
      return(1);

      /* Overflow */
    case 4:
      *VTf->regs->PStatus = STATUS_NONCONTINUABLE_EXCEPTION;
      return(1);

      /* Array bounds check */
    case 5:
      *VTf->regs->PStatus = STATUS_NONCONTINUABLE_EXCEPTION;
      return(1);

      /* Invalid opcode */
    case 6:
      *VTf->regs->PStatus = STATUS_NONCONTINUABLE_EXCEPTION;
      return(1);

      /* Device not available */
    case 7:
      *VTf->regs->PStatus = STATUS_NONCONTINUABLE_EXCEPTION;
      return(1);

      /* Double fault */
    case 8:
      *VTf->regs->PStatus = STATUS_NONCONTINUABLE_EXCEPTION;
      return(1);

      /* Intel reserved */
    case 9:
      *VTf->regs->PStatus = STATUS_NONCONTINUABLE_EXCEPTION;
      return(1);

      /* Invalid TSS */
    case 10:
      *VTf->regs->PStatus = STATUS_NONCONTINUABLE_EXCEPTION;
      return(1);

      /* Segment not present */
    case 11:
      *VTf->regs->PStatus = STATUS_NONCONTINUABLE_EXCEPTION;;
      return(1);

      /* Stack fault */
    case 12:
      *VTf->regs->PStatus = STATUS_NONCONTINUABLE_EXCEPTION;
      return(1);

      /* General protection fault */
    case 13:
      return(KeV86GPF(VTf, Tf));

      /* Page fault */
    case 14:
      {
	NTSTATUS Status;

	Status = MmPageFault(USER_CS,
			     &Tf->Eip,
			     NULL,
			     address,
			     Tf->ErrorCode);
	if (!NT_SUCCESS(Status))
	  {
	    DPRINT("V86Exception, halting due to page fault\n");
	    *VTf->regs->PStatus = STATUS_NONCONTINUABLE_EXCEPTION;
	    return(1);
	  }
	return(0);
      }

      /* Intel reserved */
    case 15:
    case 16:
      *VTf->regs->PStatus = STATUS_NONCONTINUABLE_EXCEPTION;
      return(1);
      
      /* Alignment check */
    case 17:
      *VTf->regs->PStatus = STATUS_NONCONTINUABLE_EXCEPTION;
      return(1);

    default:
      *VTf->regs->PStatus = STATUS_NONCONTINUABLE_EXCEPTION;
      return(1);
    }
}

