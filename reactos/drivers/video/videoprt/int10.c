/*
 * VideoPort driver
 *
 * Copyright (C) 2002, 2003, 2004 ReactOS Team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; see the file COPYING.LIB.
 * If not, write to the Free Software Foundation,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Id: int10.c,v 1.6 2004/06/18 17:20:49 navaraf Exp $
 */

#include "videoprt.h"
#include "internal/v86m.h"

/* PRIVATE FUNCTIONS **********************************************************/

VP_STATUS STDCALL
IntInt10AllocateBuffer(
   IN PVOID Context,
   OUT PUSHORT Seg,
   OUT PUSHORT Off,
   IN OUT PULONG Length)
{
   PVOID MemoryAddress;
   NTSTATUS Status;
   PEPROCESS CallingProcess; 
   PEPROCESS PrevAttachedProcess; 

   DPRINT("IntInt10AllocateBuffer\n");

   IntAttachToCSRSS(&CallingProcess, &PrevAttachedProcess);

   MemoryAddress = (PVOID)0x20000;
   Status = ZwAllocateVirtualMemory(NtCurrentProcess(), &MemoryAddress, 0,
      Length, MEM_COMMIT, PAGE_EXECUTE_READWRITE);

   if (!NT_SUCCESS(Status))
   {
      DPRINT("- ZwAllocateVirtualMemory failed\n");
      IntDetachFromCSRSS(&CallingProcess, &PrevAttachedProcess);
      return ERROR_NOT_ENOUGH_MEMORY;
   }

   if (MemoryAddress > (PVOID)(0x100000 - *Length))
   {
      ZwFreeVirtualMemory(NtCurrentProcess(), &MemoryAddress, Length,
         MEM_RELEASE);
      DPRINT("- Unacceptable memory allocated\n");
      IntDetachFromCSRSS(&CallingProcess, &PrevAttachedProcess);
      return ERROR_NOT_ENOUGH_MEMORY;
   }

   *Seg = (ULONG)MemoryAddress >> 4;
   *Off = (ULONG)MemoryAddress & 0xFF;

   DPRINT("- Segment: %x\n", (ULONG)MemoryAddress >> 4);
   DPRINT("- Offset: %x\n", (ULONG)MemoryAddress & 0xFF);
   DPRINT("- Length: %x\n", *Length);

   IntDetachFromCSRSS(&CallingProcess, &PrevAttachedProcess);

   return NO_ERROR;
}

VP_STATUS STDCALL
IntInt10FreeBuffer(
   IN PVOID Context,
   IN USHORT Seg,
   IN USHORT Off)
{
   PVOID MemoryAddress = (PVOID)((Seg << 4) + Off);
   NTSTATUS Status;
   PEPROCESS CallingProcess; 
   PEPROCESS PrevAttachedProcess; 

   DPRINT("IntInt10FreeBuffer\n");
   DPRINT("- Segment: %x\n", Seg);
   DPRINT("- Offset: %x\n", Off);

   IntAttachToCSRSS(&CallingProcess, &PrevAttachedProcess);
   Status = ZwFreeVirtualMemory(NtCurrentProcess(), &MemoryAddress, 0,
      MEM_RELEASE);
   IntDetachFromCSRSS(&CallingProcess, &PrevAttachedProcess);

   return Status;
}

VP_STATUS STDCALL
IntInt10ReadMemory(
   IN PVOID Context,
   IN USHORT Seg,
   IN USHORT Off,
   OUT PVOID Buffer,
   IN ULONG Length)
{
   PEPROCESS CallingProcess; 
   PEPROCESS PrevAttachedProcess; 

   DPRINT("IntInt10ReadMemory\n");
   DPRINT("- Segment: %x\n", Seg);
   DPRINT("- Offset: %x\n", Off);
   DPRINT("- Buffer: %x\n", Buffer);
   DPRINT("- Length: %x\n", Length);

   IntAttachToCSRSS(&CallingProcess, &PrevAttachedProcess);
   RtlCopyMemory(Buffer, (PVOID)((Seg << 4) + Off), Length);
   IntDetachFromCSRSS(&CallingProcess, &PrevAttachedProcess);

   return NO_ERROR;
}

VP_STATUS STDCALL
IntInt10WriteMemory(
   IN PVOID Context,
   IN USHORT Seg,
   IN USHORT Off,
   IN PVOID Buffer,
   IN ULONG Length)
{
   PEPROCESS CallingProcess; 
   PEPROCESS PrevAttachedProcess; 

   DPRINT("IntInt10WriteMemory\n");
   DPRINT("- Segment: %x\n", Seg);
   DPRINT("- Offset: %x\n", Off);
   DPRINT("- Buffer: %x\n", Buffer);
   DPRINT("- Length: %x\n", Length);

   IntAttachToCSRSS(&CallingProcess, &PrevAttachedProcess);
   RtlCopyMemory((PVOID)((Seg << 4) + Off), Buffer, Length);
   IntDetachFromCSRSS(&CallingProcess, &PrevAttachedProcess);

   return NO_ERROR;
}

VP_STATUS STDCALL
IntInt10CallBios(
   IN PVOID Context,
   IN OUT PINT10_BIOS_ARGUMENTS BiosArguments)
{
   KV86M_REGISTERS Regs;
   NTSTATUS Status;
   PEPROCESS CallingProcess; 
   PEPROCESS PrevAttachedProcess; 

   DPRINT("IntInt10CallBios\n");

   IntAttachToCSRSS(&CallingProcess, &PrevAttachedProcess);

   memset(&Regs, 0, sizeof(Regs));
   DPRINT("- Input register Eax: %x\n", BiosArguments->Eax);
   Regs.Eax = BiosArguments->Eax;
   DPRINT("- Input register Ebx: %x\n", BiosArguments->Ebx);
   Regs.Ebx = BiosArguments->Ebx;
   DPRINT("- Input register Ecx: %x\n", BiosArguments->Ecx);
   Regs.Ecx = BiosArguments->Ecx;
   DPRINT("- Input register Edx: %x\n", BiosArguments->Edx);
   Regs.Edx = BiosArguments->Edx;
   DPRINT("- Input register Esi: %x\n", BiosArguments->Esi);
   Regs.Esi = BiosArguments->Esi;
   DPRINT("- Input register Edi: %x\n", BiosArguments->Edi);
   Regs.Edi = BiosArguments->Edi;
   DPRINT("- Input register Ebp: %x\n", BiosArguments->Ebp);
   Regs.Ebp = BiosArguments->Ebp;
   DPRINT("- Input register SegDs: %x\n", BiosArguments->SegDs);
   Regs.Ds = BiosArguments->SegDs;
   DPRINT("- Input register SegEs: %x\n", BiosArguments->SegEs);
   Regs.Es = BiosArguments->SegEs;
   Status = Ke386CallBios(0x10, &Regs);
   BiosArguments->Eax = Regs.Eax;
   BiosArguments->Ebx = Regs.Ebx;
   BiosArguments->Ecx = Regs.Ecx;
   BiosArguments->Edx = Regs.Edx;
   BiosArguments->Esi = Regs.Esi;
   BiosArguments->Edi = Regs.Edi;
   BiosArguments->Ebp = Regs.Ebp;
   BiosArguments->SegDs = Regs.Ds;
   BiosArguments->SegEs = Regs.Es;

   IntDetachFromCSRSS(&CallingProcess, &PrevAttachedProcess);

   return Status;
}

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */

VP_STATUS STDCALL
VideoPortInt10(
   IN PVOID HwDeviceExtension,
   IN PVIDEO_X86_BIOS_ARGUMENTS BiosArguments)
{
   KV86M_REGISTERS Regs;
   NTSTATUS Status;
   PEPROCESS CallingProcess; 
   PEPROCESS PrevAttachedProcess; 

   DPRINT("VideoPortInt10\n");

   IntAttachToCSRSS(&CallingProcess, &PrevAttachedProcess);

   memset(&Regs, 0, sizeof(Regs));
   DPRINT("- Input register Eax: %x\n", BiosArguments->Eax);
   Regs.Eax = BiosArguments->Eax;
   DPRINT("- Input register Ebx: %x\n", BiosArguments->Ebx);
   Regs.Ebx = BiosArguments->Ebx;
   DPRINT("- Input register Ecx: %x\n", BiosArguments->Ecx);
   Regs.Ecx = BiosArguments->Ecx;
   DPRINT("- Input register Edx: %x\n", BiosArguments->Edx);
   Regs.Edx = BiosArguments->Edx;
   DPRINT("- Input register Esi: %x\n", BiosArguments->Esi);
   Regs.Esi = BiosArguments->Esi;
   DPRINT("- Input register Edi: %x\n", BiosArguments->Edi);
   Regs.Edi = BiosArguments->Edi;
   DPRINT("- Input register Ebp: %x\n", BiosArguments->Ebp);
   Regs.Ebp = BiosArguments->Ebp;
   Status = Ke386CallBios(0x10, &Regs);
   BiosArguments->Eax = Regs.Eax;
   BiosArguments->Ebx = Regs.Ebx;
   BiosArguments->Ecx = Regs.Ecx;
   BiosArguments->Edx = Regs.Edx;
   BiosArguments->Esi = Regs.Esi;
   BiosArguments->Edi = Regs.Edi;
   BiosArguments->Ebp = Regs.Ebp;

   IntDetachFromCSRSS(&CallingProcess, &PrevAttachedProcess);

   return Status;
}
