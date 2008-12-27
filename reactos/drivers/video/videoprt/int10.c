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
 */

#include "videoprt.h"
#include "internal/i386/v86m.h"

/* PRIVATE FUNCTIONS **********************************************************/

VP_STATUS NTAPI
IntInt10AllocateBuffer(
   IN PVOID Context,
   OUT PUSHORT Seg,
   OUT PUSHORT Off,
   IN OUT PULONG Length)
{
   PVOID MemoryAddress;
   NTSTATUS Status;
   PKPROCESS CallingProcess = (PKPROCESS)PsGetCurrentProcess();
   KAPC_STATE ApcState;

   TRACE_(VIDEOPRT, "IntInt10AllocateBuffer\n");

   IntAttachToCSRSS(&CallingProcess, &ApcState);

   MemoryAddress = (PVOID)0x20000;
   Status = ZwAllocateVirtualMemory(NtCurrentProcess(), &MemoryAddress, 0,
      Length, MEM_COMMIT, PAGE_EXECUTE_READWRITE);

   if (!NT_SUCCESS(Status))
   {
      WARN_(VIDEOPRT, "- ZwAllocateVirtualMemory failed\n");
      IntDetachFromCSRSS(&CallingProcess, &ApcState);
      return ERROR_NOT_ENOUGH_MEMORY;
   }

   if (MemoryAddress > (PVOID)(0x100000 - *Length))
   {
      ZwFreeVirtualMemory(NtCurrentProcess(), &MemoryAddress, Length,
         MEM_RELEASE);
      WARN_(VIDEOPRT, "- Unacceptable memory allocated\n");
      IntDetachFromCSRSS(&CallingProcess, &ApcState);
      return ERROR_NOT_ENOUGH_MEMORY;
   }

   *Seg = (ULONG)MemoryAddress >> 4;
   *Off = (ULONG)MemoryAddress & 0xF;

   INFO_(VIDEOPRT, "- Segment: %x\n", (ULONG)MemoryAddress >> 4);
   INFO_(VIDEOPRT, "- Offset: %x\n", (ULONG)MemoryAddress & 0xF);
   INFO_(VIDEOPRT, "- Length: %x\n", *Length);

   IntDetachFromCSRSS(&CallingProcess, &ApcState);

   return NO_ERROR;
}

VP_STATUS NTAPI
IntInt10FreeBuffer(
   IN PVOID Context,
   IN USHORT Seg,
   IN USHORT Off)
{
   PVOID MemoryAddress = (PVOID)((Seg << 4) | Off);
   NTSTATUS Status;
   PKPROCESS CallingProcess = (PKPROCESS)PsGetCurrentProcess();
   KAPC_STATE ApcState;

   TRACE_(VIDEOPRT, "IntInt10FreeBuffer\n");
   INFO_(VIDEOPRT, "- Segment: %x\n", Seg);
   INFO_(VIDEOPRT, "- Offset: %x\n", Off);

   IntAttachToCSRSS(&CallingProcess, &ApcState);
   Status = ZwFreeVirtualMemory(NtCurrentProcess(), &MemoryAddress, 0,
      MEM_RELEASE);
   IntDetachFromCSRSS(&CallingProcess, &ApcState);

   return Status;
}

VP_STATUS NTAPI
IntInt10ReadMemory(
   IN PVOID Context,
   IN USHORT Seg,
   IN USHORT Off,
   OUT PVOID Buffer,
   IN ULONG Length)
{
   PKPROCESS CallingProcess = (PKPROCESS)PsGetCurrentProcess();
   KAPC_STATE ApcState;

   TRACE_(VIDEOPRT, "IntInt10ReadMemory\n");
   INFO_(VIDEOPRT, "- Segment: %x\n", Seg);
   INFO_(VIDEOPRT, "- Offset: %x\n", Off);
   INFO_(VIDEOPRT, "- Buffer: %x\n", Buffer);
   INFO_(VIDEOPRT, "- Length: %x\n", Length);

   IntAttachToCSRSS(&CallingProcess, &ApcState);
   RtlCopyMemory(Buffer, (PVOID)((Seg << 4) | Off), Length);
   IntDetachFromCSRSS(&CallingProcess, &ApcState);

   return NO_ERROR;
}

VP_STATUS NTAPI
IntInt10WriteMemory(
   IN PVOID Context,
   IN USHORT Seg,
   IN USHORT Off,
   IN PVOID Buffer,
   IN ULONG Length)
{
   PKPROCESS CallingProcess = (PKPROCESS)PsGetCurrentProcess();
   KAPC_STATE ApcState;

   TRACE_(VIDEOPRT, "IntInt10WriteMemory\n");
   INFO_(VIDEOPRT, "- Segment: %x\n", Seg);
   INFO_(VIDEOPRT, "- Offset: %x\n", Off);
   INFO_(VIDEOPRT, "- Buffer: %x\n", Buffer);
   INFO_(VIDEOPRT, "- Length: %x\n", Length);

   IntAttachToCSRSS(&CallingProcess, &ApcState);
   RtlCopyMemory((PVOID)((Seg << 4) | Off), Buffer, Length);
   IntDetachFromCSRSS(&CallingProcess, &ApcState);

   return NO_ERROR;
}

VP_STATUS
NTAPI
IntInt10CallBios(
    IN PVOID Context,
    IN OUT PINT10_BIOS_ARGUMENTS BiosArguments)
{
    CONTEXT BiosContext;
    NTSTATUS Status;
    PKPROCESS CallingProcess = (PKPROCESS)PsGetCurrentProcess();
    KAPC_STATE ApcState;

    /* Attach to CSRSS */
    IntAttachToCSRSS(&CallingProcess, &ApcState);

    /* Clear the context */
    RtlZeroMemory(&BiosContext, sizeof(CONTEXT));

    /* Fill out the bios arguments */
    BiosContext.Eax = BiosArguments->Eax;
    BiosContext.Ebx = BiosArguments->Ebx;
    BiosContext.Ecx = BiosArguments->Ecx;
    BiosContext.Edx = BiosArguments->Edx;
    BiosContext.Esi = BiosArguments->Esi;
    BiosContext.Edi = BiosArguments->Edi;
    BiosContext.Ebp = BiosArguments->Ebp;
    BiosContext.SegDs = BiosArguments->SegDs;
    BiosContext.SegEs = BiosArguments->SegEs;

    /* Do the ROM BIOS call */
    Status = Ke386CallBios(0x10, &BiosContext);

    /* Return the arguments */
    BiosArguments->Eax = BiosContext.Eax;
    BiosArguments->Ebx = BiosContext.Ebx;
    BiosArguments->Ecx = BiosContext.Ecx;
    BiosArguments->Edx = BiosContext.Edx;
    BiosArguments->Esi = BiosContext.Esi;
    BiosArguments->Edi = BiosContext.Edi;
    BiosArguments->Ebp = BiosContext.Ebp;
    BiosArguments->SegDs = BiosContext.SegDs;
    BiosArguments->SegEs = BiosContext.SegEs;

    /* Detach and return status */
    IntDetachFromCSRSS(&CallingProcess, &ApcState);
    return Status;
}

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */

VP_STATUS NTAPI
VideoPortInt10(
    IN PVOID HwDeviceExtension,
    IN PVIDEO_X86_BIOS_ARGUMENTS BiosArguments)
{
    CONTEXT BiosContext;
    NTSTATUS Status;
    PKPROCESS CallingProcess = (PKPROCESS)PsGetCurrentProcess();
    KAPC_STATE ApcState;

    if (!CsrssInitialized)
    {
       return ERROR_INVALID_PARAMETER;
    }

    /* Attach to CSRSS */
    IntAttachToCSRSS(&CallingProcess, &ApcState);

    /* Clear the context */
    RtlZeroMemory(&BiosContext, sizeof(CONTEXT));

    /* Fill out the bios arguments */
    BiosContext.Eax = BiosArguments->Eax;
    BiosContext.Ebx = BiosArguments->Ebx;
    BiosContext.Ecx = BiosArguments->Ecx;
    BiosContext.Edx = BiosArguments->Edx;
    BiosContext.Esi = BiosArguments->Esi;
    BiosContext.Edi = BiosArguments->Edi;
    BiosContext.Ebp = BiosArguments->Ebp;

    /* Do the ROM BIOS call */
    Status = Ke386CallBios(0x10, &BiosContext);

    /* Return the arguments */
    BiosArguments->Eax = BiosContext.Eax;
    BiosArguments->Ebx = BiosContext.Ebx;
    BiosArguments->Ecx = BiosContext.Ecx;
    BiosArguments->Edx = BiosContext.Edx;
    BiosArguments->Esi = BiosContext.Esi;
    BiosArguments->Edi = BiosContext.Edi;
    BiosArguments->Ebp = BiosContext.Ebp;

    /* Detach from CSRSS */
    IntDetachFromCSRSS(&CallingProcess, &ApcState);

    return Status;
}
