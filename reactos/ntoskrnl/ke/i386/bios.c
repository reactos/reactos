/* $Id:$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/i386/bios.c
 * PURPOSE:         Support for calling the BIOS in v86 mode
 * 
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

#define TRAMPOLINE_BASE    (0x10000)

extern VOID Ki386RetToV86Mode(PKV86M_REGISTERS InRegs,
			      PKV86M_REGISTERS OutRegs);

/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL
Ke386CallBios(UCHAR Int, PKV86M_REGISTERS Regs)
{
  PUCHAR Ip;
  KV86M_REGISTERS ORegs;
  NTSTATUS Status;

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





