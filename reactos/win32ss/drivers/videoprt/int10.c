/*
 * VideoPort driver
 *
 * Copyright (C) 2002, 2003, 2004 ReactOS Team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "videoprt.h"

#include <ndk/kefuncs.h>

#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS **********************************************************/

#if defined(_M_IX86) || defined(_M_AMD64)
NTSTATUS
NTAPI
IntInitializeVideoAddressSpace(VOID)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING PhysMemName = RTL_CONSTANT_STRING(L"\\Device\\PhysicalMemory");
    NTSTATUS Status;
    HANDLE PhysMemHandle;
    PVOID BaseAddress;
    LARGE_INTEGER Offset;
    SIZE_T ViewSize;
    CHAR IVTAndBda[1024+256];

    /* Free the 1MB pre-reserved region. In reality, ReactOS should simply support us mapping the view into the reserved area, but it doesn't. */
    BaseAddress = 0;
    ViewSize = 1024 * 1024;
    Status = ZwFreeVirtualMemory(NtCurrentProcess(),
                                 &BaseAddress,
                                 &ViewSize,
                                 MEM_RELEASE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Couldn't unmap reserved memory (%x)\n", Status);
        return 0;
    }

    /* Open the physical memory section */
    InitializeObjectAttributes(&ObjectAttributes,
                               &PhysMemName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);
    Status = ZwOpenSection(&PhysMemHandle,
                           SECTION_ALL_ACCESS,
                           &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Couldn't open \\Device\\PhysicalMemory\n");
        return Status;
    }

    /* Map the BIOS and device registers into the address space */
    Offset.QuadPart = 0xa0000;
    ViewSize = 0x100000 - 0xa0000;
    BaseAddress = (PVOID)0xa0000;
    Status = ZwMapViewOfSection(PhysMemHandle,
                                NtCurrentProcess(),
                                &BaseAddress,
                                0,
                                ViewSize,
                                &Offset,
                                &ViewSize,
                                ViewUnmap,
                                0,
                                PAGE_EXECUTE_READWRITE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Couldn't map physical memory (%x)\n", Status);
        ZwClose(PhysMemHandle);
        return Status;
    }

    /* Close physical memory section handle */
    ZwClose(PhysMemHandle);

    if (BaseAddress != (PVOID)0xa0000)
    {
        DPRINT1("Couldn't map physical memory at the right address (was %x)\n",
                BaseAddress);
        return STATUS_UNSUCCESSFUL;
    }

    /* Allocate some low memory to use for the non-BIOS
     * parts of the v86 mode address space
     */
    BaseAddress = (PVOID)0x1;
    ViewSize = 0xa0000 - 0x1000;
    Status = ZwAllocateVirtualMemory(NtCurrentProcess(),
                                     &BaseAddress,
                                     0,
                                     &ViewSize,
                                     MEM_RESERVE | MEM_COMMIT,
                                     PAGE_EXECUTE_READWRITE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to allocate virtual memory (Status %x)\n", Status);
        return Status;
    }
    if (BaseAddress != (PVOID)0x0)
    {
        DPRINT1("Failed to allocate virtual memory at right address (was %x)\n",
                BaseAddress);
        return 0;
    }

    /* Get the real mode IVT and BDA from the kernel */
    Status = NtVdmControl(VdmInitialize, IVTAndBda);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtVdmControl failed (status %x)\n", Status);
        return Status;
    }

    /* Return success */
    return STATUS_SUCCESS;
}
#else
NTSTATUS
NTAPI
IntInitializeVideoAddressSpace(VOID)
{
    UNIMPLEMENTED;
    NT_ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}
#endif

#if defined(_M_IX86)
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

   *Seg = (USHORT)((ULONG)MemoryAddress >> 4);
   *Off = (USHORT)((ULONG)MemoryAddress & 0xF);

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
   SIZE_T Size = 0;

   TRACE_(VIDEOPRT, "IntInt10FreeBuffer\n");
   INFO_(VIDEOPRT, "- Segment: %x\n", Seg);
   INFO_(VIDEOPRT, "- Offset: %x\n", Off);

   IntAttachToCSRSS(&CallingProcess, &ApcState);
   Status = ZwFreeVirtualMemory(NtCurrentProcess(), &MemoryAddress, &Size,
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
    BiosArguments->SegDs = (USHORT)BiosContext.SegDs;
    BiosArguments->SegEs = (USHORT)BiosContext.SegEs;

    /* Detach and return status */
    IntDetachFromCSRSS(&CallingProcess, &ApcState);
    if (NT_SUCCESS(Status)) return NO_ERROR;
    return ERROR_INVALID_PARAMETER;
}
#endif

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */

VP_STATUS NTAPI
VideoPortInt10(
    IN PVOID HwDeviceExtension,
    IN PVIDEO_X86_BIOS_ARGUMENTS BiosArguments)
{
#if defined(_M_IX86)
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
    if (NT_SUCCESS(Status)) return NO_ERROR;
    return ERROR_INVALID_PARAMETER;
#else
    /* Not implemented for anything else than X86*/
    DPRINT1("Int10 not available on non-x86!\n");
    return ERROR_INVALID_FUNCTION;
#endif
}
