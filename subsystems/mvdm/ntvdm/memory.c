/*
 * COPYRIGHT:       GPLv2+ - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            subsystems/mvdm/ntvdm/memory.c
 * PURPOSE:         Memory Management
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

#include "ntvdm.h"

#define NDEBUG
#include <debug.h>

#include "emulator.h"
#include "memory.h"

/* Extra PSDK/NDK Headers */
#include <ndk/mmfuncs.h>

/* PRIVATE VARIABLES **********************************************************/

typedef struct _MEM_HOOK
{
    LIST_ENTRY Entry;
    HANDLE hVdd;
    ULONG Count;

    union
    {
        PVDD_MEMORY_HANDLER VddHandler;

        struct
        {
            PMEMORY_READ_HANDLER  FastReadHandler;
            PMEMORY_WRITE_HANDLER FastWriteHandler;
        };
    };
} MEM_HOOK, *PMEM_HOOK;

static LIST_ENTRY HookList;
static PMEM_HOOK PageTable[TOTAL_PAGES] = { NULL };
static BOOLEAN A20Line = FALSE;

/* PRIVATE FUNCTIONS **********************************************************/

static inline VOID
MemFastMoveMemory(OUT VOID UNALIGNED *Destination,
                  IN const VOID UNALIGNED *Source,
                  IN SIZE_T Length)
{
#if 1
    /*
     * We use a switch here to detect small moves of memory, as these
     * constitute the bulk of our moves.
     * Using RtlMoveMemory for all these small moves would be slow otherwise.
     */
    switch (Length)
    {
        case 0:
            return;

        case sizeof(UCHAR):
            *(PUCHAR)Destination = *(PUCHAR)Source;
            return;

        case sizeof(USHORT):
            *(PUSHORT)Destination = *(PUSHORT)Source;
            return;

        case sizeof(ULONG):
            *(PULONG)Destination = *(PULONG)Source;
            return;

        case sizeof(ULONGLONG):
            *(PULONGLONG)Destination = *(PULONGLONG)Source;
            return;

        default:
#if defined(__GNUC__)
            __builtin_memmove(Destination, Source, Length);
#else
            RtlMoveMemory(Destination, Source, Length);
#endif
    }

#else // defined(_MSC_VER)

    PUCHAR Dest = (PUCHAR)Destination;
    PUCHAR Src  = (PUCHAR)Source;

    SIZE_T Count, NewSize = Length;

    /* Move dword */
    Count   = NewSize >> 2; // NewSize / sizeof(ULONG);
    NewSize = NewSize  & 3; // NewSize % sizeof(ULONG);
    __movsd(Dest, Src, Count);
    Dest += Count << 2; // Count * sizeof(ULONG);
    Src  += Count << 2;

    /* Move word */
    Count   = NewSize >> 1; // NewSize / sizeof(USHORT);
    NewSize = NewSize  & 1; // NewSize % sizeof(USHORT);
    __movsw(Dest, Src, Count);
    Dest += Count << 1; // Count * sizeof(USHORT);
    Src  += Count << 1;

    /* Move byte */
    Count   = NewSize; // NewSize / sizeof(UCHAR);
    // NewSize = NewSize; // NewSize % sizeof(UCHAR);
    __movsb(Dest, Src, Count);

#endif
}

static inline VOID
ReadPage(PMEM_HOOK Hook, ULONG Address, PVOID Buffer, ULONG Size)
{
    if (Hook && !Hook->hVdd && Hook->FastReadHandler)
    {
        Hook->FastReadHandler(Address, REAL_TO_PHYS(Address), Size);
    }

    MemFastMoveMemory(Buffer, REAL_TO_PHYS(Address), Size);
}

static inline VOID
WritePage(PMEM_HOOK Hook, ULONG Address, PVOID Buffer, ULONG Size)
{
    if (!Hook
        || Hook->hVdd
        || !Hook->FastWriteHandler
        || Hook->FastWriteHandler(Address, Buffer, Size))
    {
        MemFastMoveMemory(REAL_TO_PHYS(Address), Buffer, Size);
    }
}

/* PUBLIC FUNCTIONS ***********************************************************/

VOID FASTCALL EmulatorReadMemory(PFAST486_STATE State, ULONG Address, PVOID Buffer, ULONG Size)
{
    ULONG i, Offset, Length;
    ULONG FirstPage, LastPage;

    UNREFERENCED_PARAMETER(State);

    /* Mirror 0x000FFFF0 at 0xFFFFFFF0 */
    if (Address >= 0xFFFFFFF0) Address -= 0xFFF00000;

    /* If the A20 line is disabled, mask bit 20 */
    if (!A20Line) Address &= ~(1 << 20);

    if ((Address + Size - 1) >= MAX_ADDRESS)
    {
        ULONG ExtraStart = (Address < MAX_ADDRESS) ? MAX_ADDRESS - Address : 0;

        /* Fill the memory that was above the limit with 0xFF */
        RtlFillMemory((PVOID)((ULONG_PTR)Buffer + ExtraStart), Size - ExtraStart, 0xFF);

        if (Address < MAX_ADDRESS) Size = MAX_ADDRESS - Address;
        else return;
    }

    FirstPage = Address >> 12;
    LastPage = (Address + Size - 1) >> 12;

    if (FirstPage == LastPage)
    {
        ReadPage(PageTable[FirstPage], Address, Buffer, Size);
    }
    else
    {
        for (i = FirstPage; i <= LastPage; i++)
        {
            Offset = (i == FirstPage) ? (Address & (PAGE_SIZE - 1)) : 0;
            Length = ((i == LastPage) ? (Address + Size - (LastPage << 12)) : PAGE_SIZE) - Offset;

            ReadPage(PageTable[i], (i << 12) + Offset, Buffer, Length);
            Buffer = (PVOID)((ULONG_PTR)Buffer + Length);
        }
    }
}

VOID FASTCALL EmulatorWriteMemory(PFAST486_STATE State, ULONG Address, PVOID Buffer, ULONG Size)
{
    ULONG i, Offset, Length;
    ULONG FirstPage, LastPage;

    UNREFERENCED_PARAMETER(State);

    /* If the A20 line is disabled, mask bit 20 */
    if (!A20Line) Address &= ~(1 << 20);

    if (Address >= MAX_ADDRESS) return;
    Size = min(Size, MAX_ADDRESS - Address);

    FirstPage = Address >> 12;
    LastPage = (Address + Size - 1) >> 12;

    if (FirstPage == LastPage)
    {
        WritePage(PageTable[FirstPage], Address, Buffer, Size);
    }
    else
    {
        for (i = FirstPage; i <= LastPage; i++)
        {
            Offset = (i == FirstPage) ? (Address & (PAGE_SIZE - 1)) : 0;
            Length = ((i == LastPage) ? (Address + Size - (LastPage << 12)) : PAGE_SIZE) - Offset;

            WritePage(PageTable[i], (i << 12) + Offset, Buffer, Length);
            Buffer = (PVOID)((ULONG_PTR)Buffer + Length);
        }
    }
}

VOID FASTCALL EmulatorCopyMemory(PFAST486_STATE State, ULONG DestAddress, ULONG SrcAddress, ULONG Size)
{
    /*
     * Guest-to-guest memory copy
     */

    // FIXME: This is a temporary implementation of a more useful functionality
    // which should be a merge of EmulatorReadMemory & EmulatorWriteMemory without
    // any local external buffer.
    // NOTE: Process heap is by default serialized (unless one specifies it shouldn't).
    static BYTE StaticBuffer[8192]; // Smallest static buffer we can use.
    static PVOID HeapBuffer = NULL; // Always-growing heap buffer. Use it in case StaticBuffer is too small.
    static ULONG HeapBufferSize = 0;
    PVOID LocalBuffer;              // Points to either StaticBuffer or HeapBuffer

    if (Size <= sizeof(StaticBuffer))
    {
        /* Use the static buffer */
        LocalBuffer = StaticBuffer;
    }
    else if (/* sizeof(StaticBuffer) <= Size && */ Size <= HeapBufferSize)
    {
        /* Use the heap buffer */
        ASSERT(HeapBufferSize > 0 && HeapBuffer != NULL);
        LocalBuffer = HeapBuffer;
    }
    else // if (Size > HeapBufferSize)
    {
        /* Enlarge the heap buffer and use it */

        if (HeapBuffer == NULL)
        {
            /* First allocation */
            LocalBuffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, Size);
        }
        else
        {
            /* Reallocation */
            LocalBuffer = RtlReAllocateHeap(RtlGetProcessHeap(), 0 /* HEAP_GENERATE_EXCEPTIONS */, HeapBuffer, Size);
        }
        ASSERT(LocalBuffer != NULL); // We must succeed! TODO: Handle it more properly.
        HeapBuffer = LocalBuffer;    // HeapBuffer is now reallocated.
        HeapBufferSize = Size;
    }

    /* Perform memory copy */
    EmulatorReadMemory( State, SrcAddress , LocalBuffer, Size);
    EmulatorWriteMemory(State, DestAddress, LocalBuffer, Size);

    // if (LocalBuffer != StaticBuffer)
    //     RtlFreeHeap(RtlGetProcessHeap(), 0, LocalBuffer);

    // Note that we don't free HeapBuffer since it's an always-growing buffer.
    // It is freed when NTVDM termiantes.
}

VOID EmulatorSetA20(BOOLEAN Enabled)
{
    A20Line = Enabled;
}

BOOLEAN EmulatorGetA20(VOID)
{
    return A20Line;
}

VOID
MemExceptionHandler(ULONG FaultAddress, BOOLEAN Writing)
{
    PMEM_HOOK Hook = PageTable[FaultAddress >> 12];
    DPRINT("The memory at 0x%08X could not be %s.\n", FaultAddress, Writing ? "written" : "read");

    /* Exceptions are only supposed to happen when using VDD-style memory hooks */
    ASSERT(FaultAddress < MAX_ADDRESS && Hook != NULL && Hook->hVdd != NULL);

    /* Call the VDD handler */
    Hook->VddHandler(REAL_TO_PHYS(FaultAddress), (ULONG)Writing);
}

BOOL
MemInstallFastMemoryHook(PVOID Address,
                         ULONG Size,
                         PMEMORY_READ_HANDLER ReadHandler,
                         PMEMORY_WRITE_HANDLER WriteHandler)
{
    PMEM_HOOK Hook;
    ULONG i;
    ULONG FirstPage = (ULONG_PTR)Address >> 12;
    ULONG LastPage = ((ULONG_PTR)Address + Size - 1) >> 12;
    PLIST_ENTRY Pointer;

    /* Make sure none of these pages are already allocated */
    for (i = FirstPage; i <= LastPage; i++)
    {
        if (PageTable[i] != NULL) return FALSE;
    }

    for (Pointer = HookList.Flink; Pointer != &HookList; Pointer = Pointer->Flink)
    {
        Hook = CONTAINING_RECORD(Pointer, MEM_HOOK, Entry);

        if (Hook->hVdd == NULL
            && Hook->FastReadHandler == ReadHandler
            && Hook->FastWriteHandler == WriteHandler)
        {
            break;
        }
    }

    if (Pointer == &HookList)
    {
        /* Create and initialize a new hook entry... */
        Hook = RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof(*Hook));
        if (Hook == NULL) return FALSE;

        Hook->hVdd = NULL;
        Hook->Count = 0;
        Hook->FastReadHandler = ReadHandler;
        Hook->FastWriteHandler = WriteHandler;

        /* ... and add it to the list of hooks */
        InsertTailList(&HookList, &Hook->Entry);
    }

    /* Increase the number of pages this hook has */
    Hook->Count += LastPage - FirstPage + 1;

    /* Add the hook entry to the page table */
    for (i = FirstPage; i <= LastPage; i++) PageTable[i] = Hook;

    return TRUE;
}

BOOL
MemRemoveFastMemoryHook(PVOID Address, ULONG Size)
{
    PMEM_HOOK Hook;
    ULONG i;
    ULONG FirstPage = (ULONG_PTR)Address >> 12;
    ULONG LastPage = ((ULONG_PTR)Address + Size - 1) >> 12;

    if (Size == 0) return FALSE;

    for (i = FirstPage; i <= LastPage; i++)
    {
        Hook = PageTable[i];
        if (Hook == NULL || Hook->hVdd != NULL) continue;

        if (--Hook->Count == 0)
        {
            /* This hook has no more pages */
            RemoveEntryList(&Hook->Entry);
            RtlFreeHeap(RtlGetProcessHeap(), 0, Hook);
        }

        PageTable[i] = NULL;
    }

    return TRUE;
}

BOOLEAN
MemQueryMemoryZone(ULONG StartAddress, PULONG Length, PBOOLEAN Hooked)
{
    ULONG Page = StartAddress >> 12;
    if (Page >= TOTAL_PAGES) return FALSE;

    *Length = 0;
    *Hooked = PageTable[Page] != NULL;

    while (Page < TOTAL_PAGES && (PageTable[Page] != NULL) == *Hooked)
    {
        *Length += PAGE_SIZE;
        Page++;
    }

    return TRUE;
}

PBYTE
WINAPI
Sim32pGetVDMPointer(IN ULONG   Address,
                    IN BOOLEAN ProtectedMode)
{
    // FIXME
    UNREFERENCED_PARAMETER(ProtectedMode);

    /*
     * HIWORD(Address) == Segment  (if ProtectedMode == FALSE)
     *                 or Selector (if ProtectedMode == TRUE )
     * LOWORD(Address) == Offset
     */
    return (PBYTE)FAR_POINTER(Address);
}

PBYTE
WINAPI
MGetVdmPointer(IN ULONG   Address,
               IN ULONG   Size,
               IN BOOLEAN ProtectedMode)
{
    UNREFERENCED_PARAMETER(Size);
    return Sim32pGetVDMPointer(Address, ProtectedMode);
}

PVOID
WINAPI
VdmMapFlat(IN USHORT   Segment,
           IN ULONG    Offset,
           IN VDM_MODE Mode)
{
    // FIXME
    UNREFERENCED_PARAMETER(Mode);

    return SEG_OFF_TO_PTR(Segment, Offset);
}

#ifndef VdmFlushCache

BOOL
WINAPI
VdmFlushCache(IN USHORT   Segment,
              IN ULONG    Offset,
              IN ULONG    Size,
              IN VDM_MODE Mode)
{
    // FIXME
    UNIMPLEMENTED;
    return TRUE;
}

#endif

#ifndef VdmUnmapFlat

BOOL
WINAPI
VdmUnmapFlat(IN USHORT   Segment,
             IN ULONG    Offset,
             IN PVOID    Buffer,
             IN VDM_MODE Mode)
{
    // FIXME
    UNIMPLEMENTED;
    return TRUE;
}

#endif

BOOL
WINAPI
VDDInstallMemoryHook(IN HANDLE hVdd,
                     IN PVOID  pStart,
                     IN DWORD  dwCount,
                     IN PVDD_MEMORY_HANDLER MemoryHandler)
{
    NTSTATUS Status;
    PMEM_HOOK Hook;
    ULONG i;
    ULONG FirstPage = (ULONG_PTR)PHYS_TO_REAL(pStart) >> 12;
    ULONG LastPage = ((ULONG_PTR)PHYS_TO_REAL(pStart) + dwCount - 1) >> 12;
    PVOID Address = (PVOID)REAL_TO_PHYS(FirstPage * PAGE_SIZE);
    SIZE_T Size = (LastPage - FirstPage + 1) * PAGE_SIZE;
    PLIST_ENTRY Pointer;

    /* Check validity of the VDD handle */
    if (hVdd == NULL || hVdd == INVALID_HANDLE_VALUE)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (dwCount == 0) return FALSE;

    /* Make sure none of these pages are already allocated */
    for (i = FirstPage; i <= LastPage; i++)
    {
        if (PageTable[i] != NULL) return FALSE;
    }

    for (Pointer = HookList.Flink; Pointer != &HookList; Pointer = Pointer->Flink)
    {
        Hook = CONTAINING_RECORD(Pointer, MEM_HOOK, Entry);
        if (Hook->hVdd == hVdd && Hook->VddHandler == MemoryHandler) break;
    }

    if (Pointer == &HookList)
    {
        /* Create and initialize a new hook entry... */
        Hook = RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof(*Hook));
        if (Hook == NULL)
        {
            SetLastError(ERROR_OUTOFMEMORY);
            return FALSE;
        }

        Hook->hVdd = hVdd;
        Hook->Count = 0;
        Hook->VddHandler = MemoryHandler;

        /* ... and add it to the list of hooks */
        InsertTailList(&HookList, &Hook->Entry);
    }

    /* Decommit the pages */
    Status = NtFreeVirtualMemory(NtCurrentProcess(),
                                 &Address,
                                 &Size,
                                 MEM_DECOMMIT);
    if (!NT_SUCCESS(Status))
    {
        if (Pointer == &HookList)
        {
            RemoveEntryList(&Hook->Entry);
            RtlFreeHeap(RtlGetProcessHeap(), 0, Hook);
        }

        return FALSE;
    }

    /* Increase the number of pages this hook has */
    Hook->Count += LastPage - FirstPage + 1;

    /* Add the hook entry to the page table */
    for (i = FirstPage; i <= LastPage; i++) PageTable[i] = Hook;

    return TRUE;
}

BOOL
WINAPI
VDDDeInstallMemoryHook(IN HANDLE hVdd,
                       IN PVOID  pStart,
                       IN DWORD  dwCount)
{
    NTSTATUS Status;
    PMEM_HOOK Hook;
    ULONG i;
    ULONG FirstPage = (ULONG_PTR)PHYS_TO_REAL(pStart) >> 12;
    ULONG LastPage = ((ULONG_PTR)PHYS_TO_REAL(pStart) + dwCount - 1) >> 12;
    PVOID Address = (PVOID)REAL_TO_PHYS(FirstPage * PAGE_SIZE);
    SIZE_T Size = (LastPage - FirstPage + 1) * PAGE_SIZE;

    /* Check validity of the VDD handle */
    if (hVdd == NULL || hVdd == INVALID_HANDLE_VALUE)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (dwCount == 0) return FALSE;

    /* Commit the pages */
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &Address,
                                     0,
                                     &Size,
                                     MEM_COMMIT,
                                     PAGE_READWRITE);
    if (!NT_SUCCESS(Status)) return FALSE;

    for (i = FirstPage; i <= LastPage; i++)
    {
        Hook = PageTable[i];
        if (Hook == NULL) continue;

        if (Hook->hVdd != hVdd)
        {
            DPRINT1("VDDDeInstallMemoryHook: Page %u owned by someone else.\n", i);
            continue;
        }

        if (--Hook->Count == 0)
        {
            /* This hook has no more pages */
            RemoveEntryList(&Hook->Entry);
            RtlFreeHeap(RtlGetProcessHeap(), 0, Hook);
        }

        PageTable[i] = NULL;
    }

    return TRUE;
}

BOOL
WINAPI
VDDAllocMem(IN HANDLE hVdd,
            IN PVOID  Address,
            IN ULONG  Size)
{
    NTSTATUS Status;
    PMEM_HOOK Hook;
    ULONG i;
    ULONG FirstPage = (ULONG_PTR)PHYS_TO_REAL(Address) >> 12;
    ULONG LastPage = ((ULONG_PTR)PHYS_TO_REAL(Address) + Size - 1) >> 12;
    SIZE_T RealSize = (LastPage - FirstPage + 1) * PAGE_SIZE;

    /* Check validity of the VDD handle */
    if (hVdd == NULL || hVdd == INVALID_HANDLE_VALUE)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (Size == 0) return FALSE;

    /* Fixup the address */
    Address = (PVOID)REAL_TO_PHYS(FirstPage * PAGE_SIZE);

    /* Be sure that all the region is held by the VDD */
    for (i = FirstPage; i <= LastPage; i++)
    {
        Hook = PageTable[i];
        if (Hook == NULL) return FALSE;

        if (Hook->hVdd != hVdd)
        {
            DPRINT1("VDDAllocMem: Page %u owned by someone else.\n", i);
            return FALSE;
        }
    }

    /* OK, all the range is held by the VDD. Commit the pages. */
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &Address,
                                     0,
                                     &RealSize,
                                     MEM_COMMIT,
                                     PAGE_READWRITE);
    return NT_SUCCESS(Status);
}

BOOL
WINAPI
VDDFreeMem(IN HANDLE hVdd,
           IN PVOID  Address,
           IN ULONG  Size)
{
    NTSTATUS Status;
    PMEM_HOOK Hook;
    ULONG i;
    ULONG FirstPage = (ULONG_PTR)PHYS_TO_REAL(Address) >> 12;
    ULONG LastPage = ((ULONG_PTR)PHYS_TO_REAL(Address) + Size - 1) >> 12;
    SIZE_T RealSize = (LastPage - FirstPage + 1) * PAGE_SIZE;

    /* Check validity of the VDD handle */
    if (hVdd == NULL || hVdd == INVALID_HANDLE_VALUE)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (Size == 0) return FALSE;

    /* Fixup the address */
    Address = (PVOID)REAL_TO_PHYS(FirstPage * PAGE_SIZE);

    /* Be sure that all the region is held by the VDD */
    for (i = FirstPage; i <= LastPage; i++)
    {
        Hook = PageTable[i];
        if (Hook == NULL) return FALSE;

        if (Hook->hVdd != hVdd)
        {
            DPRINT1("VDDFreeMem: Page %u owned by someone else.\n", i);
            return FALSE;
        }
    }

    /* OK, all the range is held by the VDD. Decommit the pages. */
    Status = NtFreeVirtualMemory(NtCurrentProcess(),
                                 &Address,
                                 &RealSize,
                                 MEM_DECOMMIT);
    return NT_SUCCESS(Status);
}

BOOL
WINAPI
VDDIncludeMem(IN HANDLE hVdd,
              IN PVOID  Address,
              IN ULONG  Size)
{
    // FIXME
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
WINAPI
VDDExcludeMem(IN HANDLE hVdd,
              IN PVOID  Address,
              IN ULONG  Size)
{
    // FIXME
    UNIMPLEMENTED;
    return FALSE;
}



BOOLEAN
MemInitialize(VOID)
{
    NTSTATUS Status;
    SIZE_T MemorySize = MAX_ADDRESS; // See: kernel32/client/vdm.c!BaseGetVdmConfigInfo

    InitializeListHead(&HookList);

#ifndef STANDALONE

    /*
     * The reserved region starts from the very first page.
     * We need to commit the reserved first 16 MB virtual address.
     *
     * NOTE: NULL has another signification for NtAllocateVirtualMemory.
     */
    BaseAddress = (PVOID)1;

    /*
     * Since to get NULL, we allocated from 0x1, account for this.
     * See also: kernel32/client/proc.c!CreateProcessInternalW
     */
    MemorySize -= 1;

#else

    /* Allocate it anywhere */
    BaseAddress = NULL;

#endif

    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &BaseAddress,
                                     0,
                                     &MemorySize,
#ifndef STANDALONE
                                     MEM_COMMIT,
#else
                                     MEM_RESERVE | MEM_COMMIT,
#endif
                                     PAGE_EXECUTE_READWRITE);
    if (!NT_SUCCESS(Status))
    {
        wprintf(L"FATAL: Failed to commit VDM memory, Status 0x%08lx\n", Status);
        return FALSE;
    }

#ifndef STANDALONE
    ASSERT(BaseAddress == NULL);
#endif

    /*
     * For diagnostics purposes, we fill the memory with INT 0x03 codes
     * so that if a program wants to execute random code in memory, we can
     * retrieve the exact CS:IP where the problem happens.
     */
    RtlFillMemory(BaseAddress, MAX_ADDRESS, 0xCC);
    return TRUE;
}

VOID
MemCleanup(VOID)
{
    NTSTATUS Status;
    SIZE_T MemorySize = MAX_ADDRESS;
    PLIST_ENTRY Pointer;

    while (!IsListEmpty(&HookList))
    {
        Pointer = RemoveHeadList(&HookList);
        RtlFreeHeap(RtlGetProcessHeap(), 0, CONTAINING_RECORD(Pointer, MEM_HOOK, Entry));
    }

    /* Decommit the VDM memory */
    Status = NtFreeVirtualMemory(NtCurrentProcess(),
                                 &BaseAddress,
                                 &MemorySize,
#ifndef STANDALONE
                                 MEM_DECOMMIT
#else
                                 MEM_RELEASE
#endif
                                 );
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NTVDM: Failed to decommit VDM memory, Status 0x%08lx\n", Status);
    }
}

/* EOF */
