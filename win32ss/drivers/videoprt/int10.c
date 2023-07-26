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
#include <ndk/mmfuncs.h>

#define NDEBUG
#include <debug.h>

/* GLOBAL VARIABLES ***********************************************************/

#ifdef _M_IX86
/* Use the 32-bit x86 emulator by default, on NT 6.x (Vista+), or on NT 5.x
 * if the HAL has the necessary exports. Otherwise fall back to V86 mode. */
BOOLEAN VideoPortDisableX86Emulator = FALSE;
#endif

KMUTEX VideoPortInt10Mutex;


/* X86 EMULATOR & V86 MODE INITIALIZATION *************************************/

#if (NTDDI_VERSION < NTDDI_VISTA) && (DLL_EXPORT_VERSION < _WIN32_WINNT_VISTA)
/*
 * x86 Emulator callbacks
 */

static BOOLEAN
(NTAPI *X86BiosCall)(
    _In_ ULONG InterruptNumber,
    _Inout_ PX86_BIOS_REGISTERS Registers);

static NTSTATUS
(NTAPI *X86AllocateBuffer)(
    _Inout_ PULONG Size,
    _Out_ PUSHORT Segment,
    _Out_ PUSHORT Offset);

static NTSTATUS
(NTAPI *X86FreeBuffer)(
    _In_ USHORT Segment,
    _In_ USHORT Offset);

static NTSTATUS
(NTAPI *X86ReadMemory)(
    _In_ USHORT Segment,
    _In_ USHORT Offset,
    _Out_writes_bytes_(Size) PVOID Buffer,
    _In_ ULONG Size);

static NTSTATUS
(NTAPI *X86WriteMemory)(
    _In_ USHORT Segment,
    _In_ USHORT Offset,
    _In_reads_bytes_(Size) PVOID Buffer,
    _In_ ULONG Size);

#else // (NTDDI_VERSION >= NTDDI_VISTA) || (DLL_EXPORT_VERSION >= _WIN32_WINNT_VISTA)

#define X86BiosCall         x86BiosCall
#define X86AllocateBuffer   x86BiosAllocateBuffer
#define X86FreeBuffer       x86BiosFreeBuffer
#define X86ReadMemory       x86BiosReadMemory
#define X86WriteMemory      x86BiosWriteMemory

#endif

static BOOLEAN
IntInitializeX86Emu(VOID)
{
#if (NTDDI_VERSION < NTDDI_VISTA) && (DLL_EXPORT_VERSION < _WIN32_WINNT_VISTA)
    UNICODE_STRING ImportName;

    RtlInitUnicodeString(&ImportName, L"x86BiosCall"); // Check also HalInitializeBios ?
    X86BiosCall = MmGetSystemRoutineAddress(&ImportName);
    if (!X86BiosCall)
        return FALSE; /* No emulator available */

    RtlInitUnicodeString(&ImportName, L"x86BiosAllocateBuffer");
    X86AllocateBuffer = MmGetSystemRoutineAddress(&ImportName);
    if (!X86AllocateBuffer)
        return FALSE;

    RtlInitUnicodeString(&ImportName, L"x86BiosFreeBuffer");
    X86FreeBuffer = MmGetSystemRoutineAddress(&ImportName);
    if (!X86FreeBuffer)
        return FALSE;

    RtlInitUnicodeString(&ImportName, L"x86BiosReadMemory");
    X86ReadMemory = MmGetSystemRoutineAddress(&ImportName);
    if (!X86ReadMemory)
        return FALSE;

    RtlInitUnicodeString(&ImportName, L"x86BiosWriteMemory");
    X86WriteMemory = MmGetSystemRoutineAddress(&ImportName);
    if (!X86WriteMemory)
        return FALSE;
#endif

    return TRUE;
}

#ifdef _M_IX86

#define IsLowV86Mem(_Seg, _Off) ((((_Seg) << 4) + (_Off)) < (0xa0000))

/* Those two functions below are there so that CSRSS can't access low mem.
 * Especially, MAKE IT CRASH ON NULL ACCESS */
static VOID
ProtectLowV86Mem(VOID)
{
    /* We pass a non-NULL address so that ZwAllocateVirtualMemory really does it
     * And we truncate one page to get the right range spanned. */
    PVOID BaseAddress = (PVOID)1;
    NTSTATUS Status;
    SIZE_T ViewSize = 0xa0000 - PAGE_SIZE;

    /* We should only do that for CSRSS */
    ASSERT(PsGetCurrentProcess() == (PEPROCESS)CsrProcess);

    /* Commit (again) the pages, but with PAGE_NOACCESS protection */
    Status = ZwAllocateVirtualMemory(NtCurrentProcess(),
                                     &BaseAddress,
                                     0,
                                     &ViewSize,
                                     MEM_COMMIT,
                                     PAGE_NOACCESS);
    ASSERT(NT_SUCCESS(Status));
}

static VOID
UnprotectLowV86Mem(VOID)
{
    /* We pass a non-NULL address so that ZwAllocateVirtualMemory really does it
     * And we truncate one page to get the right range spanned. */
    PVOID BaseAddress = (PVOID)1;
    NTSTATUS Status;
    SIZE_T ViewSize = 0xa0000 - PAGE_SIZE;

    /* We should only do that for CSRSS, for the v86 address space */
    ASSERT(PsGetCurrentProcess() == (PEPROCESS)CsrProcess);

    /* Commit (again) the pages, but with PAGE_READWRITE protection */
    Status = ZwAllocateVirtualMemory(NtCurrentProcess(),
                                     &BaseAddress,
                                     0,
                                     &ViewSize,
                                     MEM_COMMIT,
                                     PAGE_READWRITE);
    ASSERT(NT_SUCCESS(Status));
}

static FORCEINLINE
NTSTATUS
IntInitializeVideoAddressSpace(VOID)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING PhysMemName = RTL_CONSTANT_STRING(L"\\Device\\PhysicalMemory");
    NTSTATUS Status;
    HANDLE PhysMemHandle;
    PVOID BaseAddress;
    LARGE_INTEGER Offset;
    SIZE_T ViewSize;
    CHAR IVTAndBda[1024 + 256];

    /* We should only do that for CSRSS */
    ASSERT(PsGetCurrentProcess() == (PEPROCESS)CsrProcess);

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

    /* Protect the V86 address space after this */
    ProtectLowV86Mem();

    /* Return success */
    return STATUS_SUCCESS;
}

#endif // _M_IX86


/* VideoPortServicesInt10 CALLBACKS *******************************************/

static VP_STATUS
NTAPI
IntInt10AllocateBufferEmu(
    _In_ PVOID Context,
    _Out_ PUSHORT Seg,
    _Out_ PUSHORT Off,
    _Inout_ PULONG Length)
{
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(Context);

    Status = X86AllocateBuffer(Length, Seg, Off);
    return (NT_SUCCESS(Status) ? NO_ERROR : ERROR_NOT_ENOUGH_MEMORY);
}

#ifdef _M_IX86
static VP_STATUS
NTAPI
IntInt10AllocateBufferV86(
    _In_ PVOID Context,
    _Out_ PUSHORT Seg,
    _Out_ PUSHORT Off,
    _Inout_ PULONG Length)
{
    NTSTATUS Status;
    PVOID MemoryAddress;
    PKPROCESS CallingProcess;
    KAPC_STATE ApcState;
    SIZE_T Size;

    UNREFERENCED_PARAMETER(Context);

    /* Perform the call in the CSRSS context */
    if (!IntAttachToCSRSS(&CallingProcess, &ApcState))
        return ERROR_INVALID_PARAMETER;

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
        IntDetachFromCSRSS(CallingProcess, &ApcState);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    if (MemoryAddress > (PVOID)(0x100000 - Size))
    {
        ZwFreeVirtualMemory(NtCurrentProcess(),
                            &MemoryAddress,
                            &Size,
                            MEM_RELEASE);
        WARN_(VIDEOPRT, "- Unacceptable memory allocated\n");
        IntDetachFromCSRSS(CallingProcess, &ApcState);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    IntDetachFromCSRSS(CallingProcess, &ApcState);

    *Length = (ULONG)Size;
    *Seg = (USHORT)((ULONG_PTR)MemoryAddress >> 4);
    *Off = (USHORT)((ULONG_PTR)MemoryAddress & 0xF);

    return NO_ERROR;
}

static PINT10_ALLOCATE_BUFFER pInt10AllocateBuffer;
#else
#define pInt10AllocateBuffer IntInt10AllocateBufferEmu
#endif // _M_IX86

VP_STATUS
NTAPI
IntInt10AllocateBuffer(
    _In_ PVOID Context,
    _Out_ PUSHORT Seg,
    _Out_ PUSHORT Off,
    _Inout_ PULONG Length)
{
    VP_STATUS Status;

    TRACE_(VIDEOPRT, "IntInt10AllocateBuffer\n");

    Status = pInt10AllocateBuffer(Context, Seg, Off, Length);
    if (Status == NO_ERROR)
    {
        INFO_(VIDEOPRT, "- Segment: 0x%x\n", *Seg);
        INFO_(VIDEOPRT, "- Offset : 0x%x\n", *Off);
        INFO_(VIDEOPRT, "- Length : 0x%x\n", *Length);
    }
    return Status;
}


static VP_STATUS
NTAPI
IntInt10FreeBufferEmu(
    _In_ PVOID Context,
    _In_ USHORT Seg,
    _In_ USHORT Off)
{
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(Context);

    Status = X86FreeBuffer(Seg, Off);
    return (NT_SUCCESS(Status) ? NO_ERROR : ERROR_INVALID_PARAMETER);
}

#ifdef _M_IX86
static VP_STATUS
NTAPI
IntInt10FreeBufferV86(
    _In_ PVOID Context,
    _In_ USHORT Seg,
    _In_ USHORT Off)
{
    NTSTATUS Status;
    PVOID MemoryAddress = (PVOID)((ULONG_PTR)(Seg << 4) | Off);
    PKPROCESS CallingProcess;
    KAPC_STATE ApcState;
    SIZE_T Size = 0;

    UNREFERENCED_PARAMETER(Context);

    /* Perform the call in the CSRSS context */
    if (!IntAttachToCSRSS(&CallingProcess, &ApcState))
        return ERROR_INVALID_PARAMETER;

    Status = ZwFreeVirtualMemory(NtCurrentProcess(),
                                 &MemoryAddress,
                                 &Size,
                                 MEM_RELEASE);

    IntDetachFromCSRSS(CallingProcess, &ApcState);

    return Status;
}

static PINT10_FREE_BUFFER pInt10FreeBuffer;
#else
#define pInt10FreeBuffer IntInt10FreeBufferEmu
#endif // _M_IX86

VP_STATUS
NTAPI
IntInt10FreeBuffer(
    _In_ PVOID Context,
    _In_ USHORT Seg,
    _In_ USHORT Off)
{
    TRACE_(VIDEOPRT, "IntInt10FreeBuffer\n");
    INFO_(VIDEOPRT, "- Segment: 0x%x\n", Seg);
    INFO_(VIDEOPRT, "- Offset : 0x%x\n", Off);

    return pInt10FreeBuffer(Context, Seg, Off);
}


static VP_STATUS
NTAPI
IntInt10ReadMemoryEmu(
    _In_ PVOID Context,
    _In_ USHORT Seg,
    _In_ USHORT Off,
    _Out_ PVOID Buffer,
    _In_ ULONG Length)
{
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(Context);

    Status = X86ReadMemory(Seg, Off, Buffer, Length);
    return (NT_SUCCESS(Status) ? NO_ERROR : ERROR_INVALID_PARAMETER);
}

#ifdef _M_IX86
static VP_STATUS
NTAPI
IntInt10ReadMemoryV86(
    _In_ PVOID Context,
    _In_ USHORT Seg,
    _In_ USHORT Off,
    _Out_ PVOID Buffer,
    _In_ ULONG Length)
{
    PKPROCESS CallingProcess;
    KAPC_STATE ApcState;

    UNREFERENCED_PARAMETER(Context);

    /* Perform the call in the CSRSS context */
    if (!IntAttachToCSRSS(&CallingProcess, &ApcState))
        return ERROR_INVALID_PARAMETER;

    if (IsLowV86Mem(Seg, Off))
        UnprotectLowV86Mem();
    RtlCopyMemory(Buffer, (PVOID)((ULONG_PTR)(Seg << 4) | Off), Length);
    if (IsLowV86Mem(Seg, Off))
        ProtectLowV86Mem();

    IntDetachFromCSRSS(CallingProcess, &ApcState);

    return NO_ERROR;
}

static PINT10_READ_MEMORY pInt10ReadMemory;
#else
#define pInt10ReadMemory IntInt10ReadMemoryEmu
#endif // _M_IX86

VP_STATUS
NTAPI
IntInt10ReadMemory(
    _In_ PVOID Context,
    _In_ USHORT Seg,
    _In_ USHORT Off,
    _Out_ PVOID Buffer,
    _In_ ULONG Length)
{
    TRACE_(VIDEOPRT, "IntInt10ReadMemory\n");
    INFO_(VIDEOPRT, "- Segment: 0x%x\n", Seg);
    INFO_(VIDEOPRT, "- Offset : 0x%x\n", Off);
    INFO_(VIDEOPRT, "- Buffer : 0x%x\n", Buffer);
    INFO_(VIDEOPRT, "- Length : 0x%x\n", Length);

    return pInt10ReadMemory(Context, Seg, Off, Buffer, Length);
}


static VP_STATUS
NTAPI
IntInt10WriteMemoryEmu(
    _In_ PVOID Context,
    _In_ USHORT Seg,
    _In_ USHORT Off,
    _In_ PVOID Buffer,
    _In_ ULONG Length)
{
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(Context);

    Status = X86WriteMemory(Seg, Off, Buffer, Length);
    return (NT_SUCCESS(Status) ? NO_ERROR : ERROR_INVALID_PARAMETER);
}

#ifdef _M_IX86
static VP_STATUS
NTAPI
IntInt10WriteMemoryV86(
    _In_ PVOID Context,
    _In_ USHORT Seg,
    _In_ USHORT Off,
    _In_ PVOID Buffer,
    _In_ ULONG Length)
{
    PKPROCESS CallingProcess;
    KAPC_STATE ApcState;

    UNREFERENCED_PARAMETER(Context);

    /* Perform the call in the CSRSS context */
    if (!IntAttachToCSRSS(&CallingProcess, &ApcState))
        return ERROR_INVALID_PARAMETER;

    if (IsLowV86Mem(Seg, Off))
        UnprotectLowV86Mem();
    RtlCopyMemory((PVOID)((ULONG_PTR)(Seg << 4) | Off), Buffer, Length);
    if (IsLowV86Mem(Seg, Off))
        ProtectLowV86Mem();

    IntDetachFromCSRSS(CallingProcess, &ApcState);

    return NO_ERROR;
}

static PINT10_WRITE_MEMORY pInt10WriteMemory;
#else
#define pInt10WriteMemory IntInt10WriteMemoryEmu
#endif // _M_IX86

VP_STATUS
NTAPI
IntInt10WriteMemory(
    _In_ PVOID Context,
    _In_ USHORT Seg,
    _In_ USHORT Off,
    _In_ PVOID Buffer,
    _In_ ULONG Length)
{
    TRACE_(VIDEOPRT, "IntInt10WriteMemory\n");
    INFO_(VIDEOPRT, "- Segment: 0x%x\n", Seg);
    INFO_(VIDEOPRT, "- Offset : 0x%x\n", Off);
    INFO_(VIDEOPRT, "- Buffer : 0x%x\n", Buffer);
    INFO_(VIDEOPRT, "- Length : 0x%x\n", Length);

    return pInt10WriteMemory(Context, Seg, Off, Buffer, Length);
}


static VP_STATUS
NTAPI
IntInt10CallBiosEmu(
    _In_ PVOID Context,
    _Inout_ PINT10_BIOS_ARGUMENTS BiosArguments)
{
    X86_BIOS_REGISTERS BiosContext;
    BOOLEAN Success;

    UNREFERENCED_PARAMETER(Context);

    /* Clear the context and fill out the BIOS arguments */
    RtlZeroMemory(&BiosContext, sizeof(BiosContext));
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

    Success = X86BiosCall(0x10, &BiosContext);

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

    return (Success ? NO_ERROR : ERROR_INVALID_PARAMETER);
}

#ifdef _M_IX86
static VP_STATUS
NTAPI
IntInt10CallBiosV86(
    _In_ PVOID Context,
    _Inout_ PINT10_BIOS_ARGUMENTS BiosArguments)
{
    CONTEXT BiosContext;
    NTSTATUS Status;
    PKPROCESS CallingProcess;
    KAPC_STATE ApcState;

    UNREFERENCED_PARAMETER(Context);

    /* Clear the context and fill out the BIOS arguments */
    RtlZeroMemory(&BiosContext, sizeof(BiosContext));
    BiosContext.Eax = BiosArguments->Eax;
    BiosContext.Ebx = BiosArguments->Ebx;
    BiosContext.Ecx = BiosArguments->Ecx;
    BiosContext.Edx = BiosArguments->Edx;
    BiosContext.Esi = BiosArguments->Esi;
    BiosContext.Edi = BiosArguments->Edi;
    BiosContext.Ebp = BiosArguments->Ebp;
    BiosContext.SegDs = BiosArguments->SegDs;
    BiosContext.SegEs = BiosArguments->SegEs;

    /* Perform the call in the CSRSS context */
    if (!IntAttachToCSRSS(&CallingProcess, &ApcState))
        return ERROR_INVALID_PARAMETER;

    /* Do the ROM BIOS call */
    (void)KeWaitForMutexObject(&VideoPortInt10Mutex,
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL);

    /* The kernel needs access here */
    UnprotectLowV86Mem();

    /* Invoke the V86 monitor under SEH, as it can raise exceptions */
    _SEH2_TRY
    {
        Status = Ke386CallBios(0x10, &BiosContext);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    ProtectLowV86Mem();

    KeReleaseMutex(&VideoPortInt10Mutex, FALSE);

    IntDetachFromCSRSS(CallingProcess, &ApcState);

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

    return (NT_SUCCESS(Status) ? NO_ERROR : ERROR_INVALID_PARAMETER);
}

static PINT10_CALL_BIOS pInt10CallBios;
#else
#define pInt10CallBios IntInt10CallBiosEmu
#endif // _M_IX86

VP_STATUS
NTAPI
IntInt10CallBios(
    _In_ PVOID Context,
    _Inout_ PINT10_BIOS_ARGUMENTS BiosArguments)
{
    return pInt10CallBios(Context, BiosArguments);
}


/* PRIVATE FUNCTIONS **********************************************************/

NTSTATUS
IntInitializeInt10(VOID)
{
#ifdef _M_IX86
    /* We should only do that for CSRSS */
    ASSERT(PsGetCurrentProcess() == (PEPROCESS)CsrProcess);

    /* Initialize the x86 emulator if necessary, otherwise fall back to V86 mode */
    if (!VideoPortDisableX86Emulator)
    {
        /* Use the emulation routines */
        pInt10AllocateBuffer = IntInt10AllocateBufferEmu;
        pInt10FreeBuffer = IntInt10FreeBufferEmu;
        pInt10ReadMemory = IntInt10ReadMemoryEmu;
        pInt10WriteMemory = IntInt10WriteMemoryEmu;
        pInt10CallBios = IntInt10CallBiosEmu;

        if (IntInitializeX86Emu())
            return STATUS_SUCCESS;
        DPRINT1("Could not initialize the x86 emulator; falling back to V86 mode\n");
        VideoPortDisableX86Emulator = TRUE;
    }

    /* Fall back to the V86 routines */
    pInt10AllocateBuffer = IntInt10AllocateBufferV86;
    pInt10FreeBuffer = IntInt10FreeBufferV86;
    pInt10ReadMemory = IntInt10ReadMemoryV86;
    pInt10WriteMemory = IntInt10WriteMemoryV86;
    pInt10CallBios = IntInt10CallBiosV86;

    return IntInitializeVideoAddressSpace();
#else
    /* Initialize the x86 emulator */
    return (IntInitializeX86Emu() ? STATUS_SUCCESS : STATUS_NOT_SUPPORTED);
#endif
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
    VP_STATUS Status;
    INT10_BIOS_ARGUMENTS Int10BiosArguments;

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
