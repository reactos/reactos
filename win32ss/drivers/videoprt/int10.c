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
#include <ndk/halfuncs.h>

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
#ifdef _M_IX86
    CHAR IVTAndBda[1024 + 256];
#endif // _M_IX86

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

#ifdef _M_IX86
    /* Get the real mode IVT and BDA from the kernel */
    Status = NtVdmControl(VdmInitialize, IVTAndBda);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtVdmControl failed (status %x)\n", Status);
        return Status;
    }
#endif // _M_IX86

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

VP_STATUS
NTAPI
IntInt10AllocateBuffer(
    IN PVOID Context,
    OUT PUSHORT Seg,
    OUT PUSHORT Off,
    IN OUT PULONG Length)
{
    NTSTATUS Status;
#ifdef _M_IX86
    PVOID MemoryAddress;
    PKPROCESS CallingProcess = (PKPROCESS)PsGetCurrentProcess();
    KAPC_STATE ApcState;
    SIZE_T Size;

    TRACE_(VIDEOPRT, "IntInt10AllocateBuffer\n");

    IntAttachToCSRSS(&CallingProcess, &ApcState);

    Size = *Length;
    MemoryAddress = (PVOID)0x20000;
    Status = ZwAllocateVirtualMemory(NtCurrentProcess(),
                                     &MemoryAddress,
                                     0,
                                     &Size,
                                     MEM_COMMIT,
                                     PAGE_EXECUTE_READWRITE);
    if (!NT_SUCCESS(Status))
    {
        WARN_(VIDEOPRT, "- ZwAllocateVirtualMemory failed\n");
        IntDetachFromCSRSS(&CallingProcess, &ApcState);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    if (MemoryAddress > (PVOID)(0x100000 - Size))
    {
        ZwFreeVirtualMemory(NtCurrentProcess(),
                            &MemoryAddress,
                            &Size,
                            MEM_RELEASE);
        WARN_(VIDEOPRT, "- Unacceptable memory allocated\n");
        IntDetachFromCSRSS(&CallingProcess, &ApcState);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    *Length = (ULONG)Size;
    *Seg = (USHORT)((ULONG_PTR)MemoryAddress >> 4);
    *Off = (USHORT)((ULONG_PTR)MemoryAddress & 0xF);

    INFO_(VIDEOPRT, "- Segment: %x\n", (ULONG_PTR)MemoryAddress >> 4);
    INFO_(VIDEOPRT, "- Offset: %x\n", (ULONG_PTR)MemoryAddress & 0xF);
    INFO_(VIDEOPRT, "- Length: %x\n", *Length);

    IntDetachFromCSRSS(&CallingProcess, &ApcState);

    return NO_ERROR;
#else
    Status = x86BiosAllocateBuffer(Length, Seg, Off);
    return NT_SUCCESS(Status) ? NO_ERROR : ERROR_NOT_ENOUGH_MEMORY;
#endif
}

VP_STATUS
NTAPI
IntInt10FreeBuffer(
    IN PVOID Context,
    IN USHORT Seg,
    IN USHORT Off)
{
    NTSTATUS Status;
#ifdef _M_IX86
    PVOID MemoryAddress = (PVOID)((ULONG_PTR)(Seg << 4) | Off);
    PKPROCESS CallingProcess = (PKPROCESS)PsGetCurrentProcess();
    KAPC_STATE ApcState;
    SIZE_T Size = 0;

    TRACE_(VIDEOPRT, "IntInt10FreeBuffer\n");
    INFO_(VIDEOPRT, "- Segment: %x\n", Seg);
    INFO_(VIDEOPRT, "- Offset: %x\n", Off);

    IntAttachToCSRSS(&CallingProcess, &ApcState);
    Status = ZwFreeVirtualMemory(NtCurrentProcess(),
                                 &MemoryAddress,
                                 &Size,
                                 MEM_RELEASE);

    IntDetachFromCSRSS(&CallingProcess, &ApcState);

    return Status;
#else
    Status = x86BiosFreeBuffer(Seg, Off);
    return NT_SUCCESS(Status) ? NO_ERROR : ERROR_INVALID_PARAMETER;
#endif
}

VP_STATUS
NTAPI
IntInt10ReadMemory(
    IN PVOID Context,
    IN USHORT Seg,
    IN USHORT Off,
    OUT PVOID Buffer,
    IN ULONG Length)
{
#ifdef _M_IX86
    PKPROCESS CallingProcess = (PKPROCESS)PsGetCurrentProcess();
    KAPC_STATE ApcState;

    TRACE_(VIDEOPRT, "IntInt10ReadMemory\n");
    INFO_(VIDEOPRT, "- Segment: %x\n", Seg);
    INFO_(VIDEOPRT, "- Offset: %x\n", Off);
    INFO_(VIDEOPRT, "- Buffer: %x\n", Buffer);
    INFO_(VIDEOPRT, "- Length: %x\n", Length);

    IntAttachToCSRSS(&CallingProcess, &ApcState);
    RtlCopyMemory(Buffer, (PVOID)((ULONG_PTR)(Seg << 4) | Off), Length);
    IntDetachFromCSRSS(&CallingProcess, &ApcState);

    return NO_ERROR;
#else
    NTSTATUS Status;

    Status = x86BiosReadMemory(Seg, Off, Buffer, Length);
    return NT_SUCCESS(Status) ? NO_ERROR : ERROR_INVALID_PARAMETER;
#endif
}

VP_STATUS
NTAPI
IntInt10WriteMemory(
    IN PVOID Context,
    IN USHORT Seg,
    IN USHORT Off,
    IN PVOID Buffer,
    IN ULONG Length)
{
#ifdef _M_IX86
    PKPROCESS CallingProcess = (PKPROCESS)PsGetCurrentProcess();
    KAPC_STATE ApcState;

    TRACE_(VIDEOPRT, "IntInt10WriteMemory\n");
    INFO_(VIDEOPRT, "- Segment: %x\n", Seg);
    INFO_(VIDEOPRT, "- Offset: %x\n", Off);
    INFO_(VIDEOPRT, "- Buffer: %x\n", Buffer);
    INFO_(VIDEOPRT, "- Length: %x\n", Length);

    IntAttachToCSRSS(&CallingProcess, &ApcState);
    RtlCopyMemory((PVOID)((ULONG_PTR)(Seg << 4) | Off), Buffer, Length);
    IntDetachFromCSRSS(&CallingProcess, &ApcState);

    return NO_ERROR;
#else
    NTSTATUS Status;

    Status = x86BiosWriteMemory(Seg, Off, Buffer, Length);
    return NT_SUCCESS(Status) ? NO_ERROR : ERROR_INVALID_PARAMETER;
#endif
}

VP_STATUS
NTAPI
IntInt10CallBios(
    IN PVOID Context,
    IN OUT PINT10_BIOS_ARGUMENTS BiosArguments)
{
#ifdef _M_AMD64
    X86_BIOS_REGISTERS BiosContext;
#else
    CONTEXT BiosContext;
#endif
    NTSTATUS Status;
    PKPROCESS CallingProcess = (PKPROCESS)PsGetCurrentProcess();
    KAPC_STATE ApcState;

    /* Attach to CSRSS */
    IntAttachToCSRSS(&CallingProcess, &ApcState);

    /* Clear the context */
    RtlZeroMemory(&BiosContext, sizeof(BiosContext));

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
    (void)KeWaitForMutexObject(&VideoPortInt10Mutex,
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL);

#ifdef _M_AMD64
    Status = x86BiosCall(0x10, &BiosContext) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
#else
    Status = Ke386CallBios(0x10, &BiosContext);
#endif

    KeReleaseMutex(&VideoPortInt10Mutex, FALSE);

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

    if (NT_SUCCESS(Status))
    {
        return NO_ERROR;
    }

    return ERROR_INVALID_PARAMETER;
}

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */
VP_STATUS
NTAPI
VideoPortInt10(
    IN PVOID HwDeviceExtension,
    IN PVIDEO_X86_BIOS_ARGUMENTS BiosArguments)
{
    INT10_BIOS_ARGUMENTS Int10BiosArguments;
    VP_STATUS Status;

    if (!CsrProcess)
    {
        return ERROR_INVALID_PARAMETER;
    }

    /* Copy arguments to other format */
    RtlCopyMemory(&Int10BiosArguments, BiosArguments, sizeof(*BiosArguments));
    Int10BiosArguments.SegDs = 0;
    Int10BiosArguments.SegEs = 0;

    /* Do the BIOS call */
    Status = IntInt10CallBios(NULL, &Int10BiosArguments);

    /* Copy results back */
    RtlCopyMemory(BiosArguments, &Int10BiosArguments, sizeof(*BiosArguments));

    return Status;
}
