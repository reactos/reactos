/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/i386/v86m.c
 * PURPOSE:         Support for v86 mode
 *
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

#define IOPL_FLAG          ((1 << 12) | (1 << 13))
#define INTERRUPT_FLAG     (1 << 9)
#define TRAP_FLAG          (1 << 8)
#define DIRECTION_FLAG     (1 << 10)

#define VALID_FLAGS         (0xDFF)

#define TRAMPOLINE_BASE    (0x10000)

VOID Ki386RetToV86Mode(KV86M_REGISTERS* InRegs,
                  KV86M_REGISTERS* OutRegs);
/* FUNCTIONS *****************************************************************/

ULONG
KeV86GPF(PKV86M_TRAP_FRAME VTf, PKTRAP_FRAME Tf)
{
  PUCHAR ip;
  PUSHORT sp;
  PULONG dsp;
  BOOL BigDataPrefix = FALSE;
  BOOL BigAddressPrefix = FALSE;
  BOOL RepPrefix = FALSE;
  ULONG i = 0;
  BOOL Exit = FALSE;

  ip = (PUCHAR)((Tf->SegCs & 0xFFFF) * 16 + (Tf->Eip & 0xFFFF));
  sp = (PUSHORT)((Tf->HardwareSegSs & 0xFFFF) * 16 + (Tf->HardwareEsp & 0xFFFF));
  dsp = (PULONG)sp;

  DPRINT("KeV86GPF handling %x at %x:%x ss:sp %x:%x Flags %x\n",
	 ip[0], Tf->SegCs, Tf->Eip, Tf->Ss, Tf->HardwareEsp, VTf->regs->Flags);

  while (!Exit)
    {
        //DPRINT1("ip: %lx\n", ip[i]);
      switch (ip[i])
	{
	  /* 32-bit data prefix */
	case 0x66:
	  BigDataPrefix = TRUE;
    	  i++;
	  Tf->Eip++;
	  break;

	  /* 32-bit address prefix */
	case 0x67:
	  BigAddressPrefix = TRUE;
	  i++;
	  Tf->Eip++;
	  break;

	  /* rep prefix */
	case 0xFC:
	  RepPrefix = TRUE;
	  i++;
	  Tf->Eip++;
	  break;

	  /* sti */
	case 0xFB:
	  if (BigDataPrefix || BigAddressPrefix || RepPrefix)
	    {
	      *VTf->regs->PStatus = STATUS_NONCONTINUABLE_EXCEPTION;
	      return(1);
	    }
	  if (VTf->regs->Flags & KV86M_EMULATE_CLI_STI)
	    {
	      Tf->Eip++;
	      VTf->regs->Vif = 1;
	      return(0);
	    }
	  Exit = TRUE;
	  break;

	  /* cli */
	case 0xFA:
	  if (BigDataPrefix || BigAddressPrefix || RepPrefix)
	    {
	      *VTf->regs->PStatus = STATUS_NONCONTINUABLE_EXCEPTION;
	      return(1);
	    }
	  if (VTf->regs->Flags & KV86M_EMULATE_CLI_STI)
	    {
	      Tf->Eip++;
	      VTf->regs->Vif = 0;
	      return(0);
	    }
	  Exit = TRUE;
	  break;

	  /* pushf */
	case 0x9C:
	  if (RepPrefix)
	    {
	      *VTf->regs->PStatus = STATUS_NONCONTINUABLE_EXCEPTION;
	      return(1);
	    }
	  if (VTf->regs->Flags & KV86M_EMULATE_CLI_STI)
	    {
	      Tf->Eip++;
	      if (!BigAddressPrefix)
		{
		  Tf->HardwareEsp = Tf->HardwareEsp - 2;
		  sp = sp - 1;
		  sp[0] = (USHORT)(Tf->EFlags & 0xFFFF);
		  if (VTf->regs->Vif == 1)
		    {
		      sp[0] = (USHORT)(sp[0] | INTERRUPT_FLAG);
		    }
		  else
		    {
		      sp[0] = (USHORT)(sp[0] & (~INTERRUPT_FLAG));
		    }
		}
	      else
		{
		  Tf->HardwareEsp = Tf->HardwareEsp - 4;
		  dsp = dsp - 1;
		  dsp[0] = Tf->EFlags;
		  dsp[0] = dsp[0] & VALID_FLAGS;
		  if (VTf->regs->Vif == 1)
		    {
		      dsp[0] = dsp[0] | INTERRUPT_FLAG;
		    }
		  else
		    {
		      dsp[0] = dsp[0] & (~INTERRUPT_FLAG);
		    }
		}
	      return(0);
	    }
	  Exit = TRUE;
	  break;

	  /* popf */
	case 0x9D:
	  if (RepPrefix)
	    {
	      *VTf->regs->PStatus = STATUS_NONCONTINUABLE_EXCEPTION;
	      return(1);
	    }
	  if (VTf->regs->Flags & KV86M_EMULATE_CLI_STI)
	    {
	      Tf->Eip++;
	      if (!BigAddressPrefix)
		{
		  Tf->EFlags = Tf->EFlags & (~0xFFFF);
		  Tf->EFlags = Tf->EFlags | (sp[0] & VALID_FLAGS);
		  if (Tf->EFlags & INTERRUPT_FLAG)
		    {
		      VTf->regs->Vif = 1;
		    }
		  else
		    {
		      VTf->regs->Vif = 0;
		    }
		  Tf->EFlags = Tf->EFlags | INTERRUPT_FLAG;
		  Tf->HardwareEsp = Tf->HardwareEsp + 2;
		}
	      else
		{
		  Tf->EFlags = Tf->EFlags | (dsp[0] & VALID_FLAGS);
		  if (dsp[0] & INTERRUPT_FLAG)
		    {
		      VTf->regs->Vif = 1;
		    }
		  else
		    {
		      VTf->regs->Vif = 0;
		    }
		  Tf->EFlags = Tf->EFlags | INTERRUPT_FLAG;
		  Tf->HardwareEsp = Tf->HardwareEsp + 2;
		}
	      return(0);
	    }
	  Exit = TRUE;
	  break;

      /* iret */
	case 0xCF:
	  if (RepPrefix)
	    {
	      *VTf->regs->PStatus = STATUS_NONCONTINUABLE_EXCEPTION;
	      return(1);
	    }
	  if (VTf->regs->Flags & KV86M_EMULATE_CLI_STI)
	    {
	      Tf->Eip = sp[0];
	      Tf->SegCs = sp[1];
	      Tf->EFlags = Tf->EFlags & (~0xFFFF);
	      Tf->EFlags = Tf->EFlags | sp[2];
	      if (Tf->EFlags & INTERRUPT_FLAG)
		{
		  VTf->regs->Vif = 1;
		}
	      else
		{
		  VTf->regs->Vif = 0;
		}
	      Tf->EFlags = Tf->EFlags & (~INTERRUPT_FLAG);
	      Tf->HardwareEsp = Tf->HardwareEsp + 6;
	      return(0);
	    }
	  Exit = TRUE;
	  break;

	  /* out imm8, al */
	case 0xE6:
	  if (RepPrefix)
	    {
	      *VTf->regs->PStatus = STATUS_NONCONTINUABLE_EXCEPTION;
	      return(1);
	    }
	  if (VTf->regs->Flags & KV86M_ALLOW_IO_PORT_ACCESS)
	    {
	      DPRINT("outb %d, %x\n", (ULONG)ip[i + 1], Tf->Eax & 0xFF);
	      WRITE_PORT_UCHAR((PUCHAR)(ULONG)ip[i + 1],
			       (UCHAR)(Tf->Eax & 0xFF));
	      Tf->Eip = Tf->Eip + 2;
	      return(0);
	    }
	  Exit = TRUE;
	  break;

	  /* out imm8, ax */
	case 0xE7:
	  if (RepPrefix)
	    {
	      *VTf->regs->PStatus = STATUS_NONCONTINUABLE_EXCEPTION;
	      return(1);
	    }
	  if (VTf->regs->Flags & KV86M_ALLOW_IO_PORT_ACCESS)
	    {
	      if (!BigDataPrefix)
		{
		  DPRINT("outw %d, %x\n", (ULONG)ip[i + 1], Tf->Eax & 0xFFFF);
		  WRITE_PORT_USHORT((PUSHORT)(ULONG)ip[1], (USHORT)(Tf->Eax & 0xFFFF));
		}
	      else
		{
		  DPRINT("outl %d, %x\n", (ULONG)ip[i + 1], Tf->Eax);
		  WRITE_PORT_ULONG((PULONG)(ULONG)ip[1], Tf->Eax);
		}
	      Tf->Eip = Tf->Eip + 2;
	      return(0);
	    }
	  Exit = TRUE;
	  break;

	  /* out dx, al */
	case 0xEE:
	  if (RepPrefix)
	    {
	      *VTf->regs->PStatus = STATUS_NONCONTINUABLE_EXCEPTION;
	      return(1);
	    }
	  if (VTf->regs->Flags & KV86M_ALLOW_IO_PORT_ACCESS)
	    {
	      DPRINT("outb %d, %x\n", Tf->Edx & 0xFFFF, Tf->Eax & 0xFF);
	      WRITE_PORT_UCHAR((PUCHAR)(Tf->Edx & 0xFFFF), (UCHAR)(Tf->Eax & 0xFF));
	      Tf->Eip = Tf->Eip + 1;
	      return(0);
	    }
	  Exit = TRUE;
	  break;

	  /* out dx, ax */
	case 0xEF:
	  if (RepPrefix)
	    {
	      *VTf->regs->PStatus = STATUS_NONCONTINUABLE_EXCEPTION;
	      return(1);
	    }
	  if (VTf->regs->Flags & KV86M_ALLOW_IO_PORT_ACCESS)
	    {
	      if (!BigDataPrefix)
		{
		  DPRINT("outw %d, %x\n", Tf->Edx & 0xFFFF, Tf->Eax & 0xFFFF);
		  WRITE_PORT_USHORT((PUSHORT)(Tf->Edx & 0xFFFF),
				    (USHORT)(Tf->Eax & 0xFFFF));
		}
	      else
		{
		  DPRINT("outl %d, %x\n", Tf->Edx & 0xFFFF, Tf->Eax);
		  WRITE_PORT_ULONG((PULONG)(Tf->Edx & 0xFFFF),
				   Tf->Eax);
		}
	      Tf->Eip = Tf->Eip + 1;
	      return(0);
	    }
	  Exit = TRUE;
	  break;

	  /* in al, imm8 */
	case 0xE4:
	  if (RepPrefix)
	    {
	      *VTf->regs->PStatus = STATUS_NONCONTINUABLE_EXCEPTION;
	      return(1);
	    }
	  if (VTf->regs->Flags & KV86M_ALLOW_IO_PORT_ACCESS)
	    {
	      UCHAR v;

	      v = READ_PORT_UCHAR((PUCHAR)(ULONG)ip[1]);
	      DPRINT("inb %d\t%X\n", (ULONG)ip[1], v);
	      Tf->Eax = Tf->Eax & (~0xFF);
	      Tf->Eax = Tf->Eax | v;
	      Tf->Eip = Tf->Eip + 2;
	      return(0);
	    }
	  Exit = TRUE;
	  break;

	  /* in ax, imm8 */
	case 0xE5:
	  if (RepPrefix)
	    {
	      *VTf->regs->PStatus = STATUS_NONCONTINUABLE_EXCEPTION;
	      return(1);
	    }
	  if (VTf->regs->Flags & KV86M_ALLOW_IO_PORT_ACCESS)
	    {
	      if (!BigDataPrefix)
		{
		  USHORT v;
		  v = READ_PORT_USHORT((PUSHORT)(ULONG)ip[1]);
		  DPRINT("inw %d\t%X\n", (ULONG)ip[1], v);
		  Tf->Eax = Tf->Eax & (~0xFFFF);
		  Tf->Eax = Tf->Eax | v;
		}
	      else
		{
		  ULONG v;
		  v = READ_PORT_ULONG((PULONG)(ULONG)ip[1]);
		  DPRINT("inl %d\t%X\n", (ULONG)ip[1], v);
		  Tf->Eax = v;
		}
	      Tf->Eip = Tf->Eip + 2;
	      return(0);
	    }
	  Exit = TRUE;
	  break;

	  /* in al, dx */
	case 0xEC:
	  if (RepPrefix)
	    {
	      *VTf->regs->PStatus = STATUS_NONCONTINUABLE_EXCEPTION;
	      return(1);
	    }
	  if (VTf->regs->Flags & KV86M_ALLOW_IO_PORT_ACCESS)
	    {
	      UCHAR v;

	      v = READ_PORT_UCHAR((PUCHAR)(Tf->Edx & 0xFFFF));
	      DPRINT("inb %d\t%X\n", Tf->Edx & 0xFFFF, v);
	      Tf->Eax = Tf->Eax & (~0xFF);
	      Tf->Eax = Tf->Eax | v;
	      Tf->Eip = Tf->Eip + 1;
	      return(0);
	    }
	  Exit = TRUE;
	  break;

	  /* in ax, dx */
	case 0xED:
	  if (RepPrefix)
	    {
	      *VTf->regs->PStatus = STATUS_NONCONTINUABLE_EXCEPTION;
	      return(1);
	    }
	  if (VTf->regs->Flags & KV86M_ALLOW_IO_PORT_ACCESS)
	    {
	      if (!BigDataPrefix)
		{
		  USHORT v;

		  v = READ_PORT_USHORT((PUSHORT)(Tf->Edx & 0xFFFF));
		  DPRINT("inw %d\t%X\n", Tf->Edx & 0xFFFF, v);
		  Tf->Eax = Tf->Eax & (~0xFFFF);
		  Tf->Eax = Tf->Eax | v;
		}
	      else
		{
		  ULONG v;

		  v = READ_PORT_ULONG((PULONG)(Tf->Edx & 0xFFFF));
		  DPRINT("inl %d\t%X\n", Tf->Edx & 0xFFFF, v);
		  Tf->Eax = v;
		}
	      Tf->Eip = Tf->Eip + 1;
	      return(0);
	    }
	  Exit = TRUE;
	  break;

      /* outsb */
	case 0x6E:
	  if (VTf->regs->Flags & KV86M_ALLOW_IO_PORT_ACCESS)
	    {
	      ULONG Count;
	      PUCHAR Port;
	      PUCHAR Buffer;
	      ULONG Offset;

	      Count = 1;
	      if (RepPrefix)
		{
		  Count = Tf->Ecx;
		  if (!BigAddressPrefix)
		    {
		      Count = Count & 0xFFFF;
		    }
		}

	      Port = (PUCHAR)(Tf->Edx & 0xFFFF);
	      Offset = Tf->Edi;
	      if (!BigAddressPrefix)
		{
		  Offset = Offset & 0xFFFF;
		}
	      Buffer = (PUCHAR)((Tf->SegEs * 16) + Offset);
	      for (; Count > 0; Count--)
		{
		  WRITE_PORT_UCHAR(Port, *Buffer);
		  if (Tf->EFlags & DIRECTION_FLAG)
		    {
		      Buffer++;
		    }
		  else
		    {
		      Buffer--;
		    }
		}
	      Tf->Eip++;
	      return(0);
	    }
	  Exit = TRUE;
	  break;

	  /* insw/insd */
	case 0x6F:
	  if (VTf->regs->Flags & KV86M_ALLOW_IO_PORT_ACCESS)
	    {
	      ULONG Count;
	      PUCHAR Port;
	      PUSHORT BufferS = NULL;
	      PULONG BufferL = NULL;
	      ULONG Offset;

	      Count = 1;
	      if (RepPrefix)
		{
		  Count = Tf->Ecx;
		  if (!BigAddressPrefix)
		    {
		      Count = Count & 0xFFFF;
		    }
		}

	      Port = (PUCHAR)(Tf->Edx & 0xFFFF);
	      Offset = Tf->Edi;
	      if (!BigAddressPrefix)
		{
		  Offset = Offset & 0xFFFF;
		}
	      if (BigDataPrefix)
		{
		  BufferL = (PULONG)((Tf->SegEs * 16) + Offset);
		}
	      else
		{
		  BufferS = (PUSHORT)((Tf->SegEs * 16) + Offset);
		}
	      for (; Count > 0; Count--)
		{
		  if (BigDataPrefix)
		    {
		      WRITE_PORT_ULONG((PULONG)Port, *BufferL);
		    }
		  else
		    {
		      WRITE_PORT_USHORT((PUSHORT)Port, *BufferS);
		    }
		  if (Tf->EFlags & DIRECTION_FLAG)
		    {
		      if (BigDataPrefix)
			{
			  BufferL++;
			}
		      else
			{
			  BufferS++;
			}
		    }
		  else
		    {
		      if (BigDataPrefix)
			{
			  BufferL--;
			}
		      else
			{
			  BufferS--;
			}
		    }
		}
	      Tf->Eip++;
	      return(0);
	    }
	  Exit = TRUE;
	  break;

      /* insb */
	case 0x6C:
	  if (VTf->regs->Flags & KV86M_ALLOW_IO_PORT_ACCESS)
	    {
	      ULONG Count;
	      PUCHAR Port;
	      PUCHAR Buffer;
	      ULONG Offset;

	      Count = 1;
	      if (RepPrefix)
		{
		  Count = Tf->Ecx;
		  if (!BigAddressPrefix)
		    {
		      Count = Count & 0xFFFF;
		    }
		}

	      Port = (PUCHAR)(Tf->Edx & 0xFFFF);
	      Offset = Tf->Edi;
	      if (!BigAddressPrefix)
		{
		  Offset = Offset & 0xFFFF;
		}
	      Buffer = (PUCHAR)((Tf->SegEs * 16) + Offset);
	      for (; Count > 0; Count--)
		{
		  *Buffer = READ_PORT_UCHAR(Port);
		  if (Tf->EFlags & DIRECTION_FLAG)
		    {
		      Buffer++;
		    }
		  else
		    {
		      Buffer--;
		    }
		}
	      Tf->Eip++;
	      return(0);
	    }
	  Exit = TRUE;
	  break;

	  /* insw/insd */
	case 0x6D:
	  if (VTf->regs->Flags & KV86M_ALLOW_IO_PORT_ACCESS)
	    {
	      ULONG Count;
	      PUCHAR Port;
	      PUSHORT BufferS = NULL;
	      PULONG BufferL = NULL;
	      ULONG Offset;

	      Count = 1;
	      if (RepPrefix)
		{
		  Count = Tf->Ecx;
		  if (!BigAddressPrefix)
		    {
		      Count = Count & 0xFFFF;
		    }
		}

	      Port = (PUCHAR)(Tf->Edx & 0xFFFF);
	      Offset = Tf->Edi;
	      if (!BigAddressPrefix)
		{
		  Offset = Offset & 0xFFFF;
		}
	      if (BigDataPrefix)
		{
		  BufferL = (PULONG)((Tf->SegEs * 16) + Offset);
		}
	      else
		{
		  BufferS = (PUSHORT)((Tf->SegEs * 16) + Offset);
		}
	      for (; Count > 0; Count--)
		{
		  if (BigDataPrefix)
		    {
		      *BufferL = READ_PORT_ULONG((PULONG)Port);
		    }
		  else
		    {
		      *BufferS = READ_PORT_USHORT((PUSHORT)Port);
		    }
		  if (Tf->EFlags & DIRECTION_FLAG)
		    {
		      if (BigDataPrefix)
			{
			  BufferL++;
			}
		      else
			{
			  BufferS++;
			}
		    }
		  else
		    {
		      if (BigDataPrefix)
			{
			  BufferL--;
			}
		      else
			{
			  BufferS--;
			}
		    }
		}
	      Tf->Eip++;
	      return(0);
	    }
	  Exit = TRUE;
	  break;

	  /* Int nn */
	case 0xCD:
	  {
	    unsigned int inum;
	    unsigned int entry;

	    inum = ip[1];
	    entry = ((unsigned int *)0)[inum];

	    Tf->HardwareEsp = Tf->HardwareEsp - 6;
	    sp = sp - 3;

	    sp[0] = (USHORT)((Tf->Eip & 0xFFFF) + 2);
	    sp[1] = (USHORT)(Tf->SegCs & 0xFFFF);
	    sp[2] = (USHORT)(Tf->EFlags & 0xFFFF);
	    if (VTf->regs->Vif == 1)
	      {
		sp[2] = (USHORT)(sp[2] | INTERRUPT_FLAG);
	      }
	    DPRINT("sp[0] %x sp[1] %x sp[2] %x\n", sp[0], sp[1], sp[2]);
	    Tf->Eip = entry & 0xFFFF;
	    Tf->SegCs = entry >> 16;
	    Tf->EFlags = Tf->EFlags & (~TRAP_FLAG);

	    return(0);
	  }
	}

      /* FIXME: Also emulate ins and outs */
      /* FIXME: Handle opcode prefixes */
      /* FIXME: Don't allow the BIOS to write to sensitive I/O ports */
    }

  DPRINT1("V86GPF unhandled (was %x)\n", ip[i]);
  *VTf->regs->PStatus = STATUS_NONCONTINUABLE_EXCEPTION;
  return(1);
}

ULONG
NTAPI
KeV86Exception(ULONG ExceptionNr, PKTRAP_FRAME Tf, ULONG address)
{
    PUCHAR Ip;
    PKV86M_TRAP_FRAME VTf;

    VTf = (PKV86M_TRAP_FRAME)Tf;

    /*
    * Check if we have reached the recovery instruction
    */
    Ip = (PUCHAR)((Tf->SegCs & 0xFFFF) * 16 + (Tf->Eip & 0xFFFF));

    if (ExceptionNr == 6 &&
        memcmp(Ip, VTf->regs->RecoveryInstruction, 4) == 0 &&
        (Tf->SegCs * 16 + Tf->Eip) == VTf->regs->RecoveryAddress)
    {
        *VTf->regs->PStatus = STATUS_SUCCESS;
        return(1);
    }

    ASSERT(ExceptionNr == 13);
    return(KeV86GPF(VTf, Tf));
}

NTSTATUS STDCALL
Ke386CallBios(ULONG Int, PCONTEXT regs)
{
  PUCHAR Ip;
  KV86M_REGISTERS ORegs;
  NTSTATUS Status;
  PKV86M_REGISTERS Regs = (PKV86M_REGISTERS)regs;

  /*
   * Set up a trampoline for executing the BIOS interrupt
   */
  Ip = (PUCHAR)TRAMPOLINE_BASE;
  Ip[0] = 0xCD;              /* int XX */
  Ip[1] = Int;
  Ip[2] = 0x63;              /* arpl ax, ax */
  Ip[3] = 0xC0;
  Ip[4] = 0x90;              /* nop */
  Ip[5] = 0x90;              /* nop */

  /*
   * Munge the registers
   */
  Regs->Eip = 0;
  Regs->Cs = TRAMPOLINE_BASE / 16;
  Regs->Esp = 0xFFFF;
  Regs->Ss = TRAMPOLINE_BASE / 16;
  Regs->Eflags = (1 << 1) | (1 << 17) | (1 << 9);     /* VM, IF */
  Regs->RecoveryAddress = TRAMPOLINE_BASE + 2;
  Regs->RecoveryInstruction[0] = 0x63;       /* arpl ax, ax */
  Regs->RecoveryInstruction[1] = 0xC0;
  Regs->RecoveryInstruction[2] = 0x90;       /* nop */
  Regs->RecoveryInstruction[3] = 0x90;       /* nop */
  Regs->Flags = KV86M_EMULATE_CLI_STI | KV86M_ALLOW_IO_PORT_ACCESS;
  Regs->Vif = 1;
  Regs->PStatus = &Status;

  /*
   * Execute the BIOS interrupt
   */
  Ki386RetToV86Mode(Regs, &ORegs);

  /*
   * Copy the return values back to the caller
   */
  memcpy(Regs, &ORegs, sizeof(KV86M_REGISTERS));

  return(Status);
}

