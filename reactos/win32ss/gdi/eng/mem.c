/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           GDI Driver Memory Management Functions
 * FILE:              subsys/win32k/eng/mem.c
 * PROGRAMER:         Jason Filby
 */

#include <win32k.h>

#define NDEBUG
#include <debug.h>

/*
 * @implemented
 */
PVOID
APIENTRY
EngAllocMem(
    ULONG Flags,
    ULONG cjMemSize,
    ULONG ulTag)
{
    PVOID pvBaseAddress;

    pvBaseAddress = ExAllocatePoolWithTag((Flags & FL_NONPAGED_MEMORY) ?
                                                    NonPagedPool : PagedPool,
                                          cjMemSize,
                                          ulTag);

    if (pvBaseAddress == NULL)
        return NULL;

    if (Flags & FL_ZERO_MEMORY)
        RtlZeroMemory(pvBaseAddress, cjMemSize);

    return pvBaseAddress;
}

/*
 * @implemented
 */
VOID
APIENTRY
EngFreeMem(PVOID pvBaseAddress)
{
    ExFreePool(pvBaseAddress);
}

/*
 * @implemented
 */
PVOID
APIENTRY
EngAllocUserMem(SIZE_T cjSize, ULONG ulTag)
{
    PVOID pvBaseAddress = NULL;
    NTSTATUS Status;

    Status = ZwAllocateVirtualMemory(NtCurrentProcess(),
                                     &pvBaseAddress,
                                     0,
                                     &cjSize,
                                     MEM_COMMIT | MEM_RESERVE,
                                     PAGE_READWRITE);

    if (!NT_SUCCESS(Status))
    {
        return NULL;
    }

    /* TODO: Add allocation info to AVL tree (stored inside W32PROCESS structure) */
    //hSecure = EngSecureMem(pvBaseAddress, cj);

  return pvBaseAddress;
}

/*
 * @implemented
 */
VOID
APIENTRY
EngFreeUserMem(PVOID pvBaseAddress)
{
    SIZE_T cjSize = 0;

    ZwFreeVirtualMemory(NtCurrentProcess(),
                        &pvBaseAddress,
                        &cjSize,
                        MEM_RELEASE);

  /* TODO: Remove allocation info from AVL tree */
}


PVOID
APIENTRY
HackSecureVirtualMemory(
    IN PVOID Address,
    IN SIZE_T Size,
    IN ULONG ProbeMode,
    OUT PVOID *SafeAddress)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PMDL pmdl;
    LOCK_OPERATION Operation;

    if (ProbeMode == PAGE_READONLY) Operation = IoReadAccess;
    else if (ProbeMode == PAGE_READWRITE) Operation = IoModifyAccess;
    else return NULL;

    pmdl = IoAllocateMdl(Address, Size, FALSE, TRUE, NULL);
    if (pmdl == NULL)
    {
        return NULL;
    }

    _SEH2_TRY
    {
        MmProbeAndLockPages(pmdl, UserMode, Operation);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END

    if (!NT_SUCCESS(Status))
    {
        IoFreeMdl(pmdl);
        return NULL;
    }

    *SafeAddress = MmGetSystemAddressForMdlSafe(pmdl, NormalPagePriority);

    if(!*SafeAddress)
    {
        MmUnlockPages(pmdl);
        IoFreeMdl(pmdl);
        return NULL;
    }

    return pmdl;
}

VOID
APIENTRY
HackUnsecureVirtualMemory(
    IN PVOID  SecureHandle)
{
    PMDL pmdl = (PMDL)SecureHandle;

    MmUnlockPages(pmdl);
    IoFreeMdl(pmdl);
}

/*
 * @implemented
 */
HANDLE APIENTRY
EngSecureMem(PVOID Address, ULONG Length)
{
    return (HANDLE)-1; // HACK!!!
    return MmSecureVirtualMemory(Address, Length, PAGE_READWRITE);
}

/*
 * @implemented
 */
VOID APIENTRY
EngUnsecureMem(HANDLE Mem)
{
    if (Mem == (HANDLE)-1) return;  // HACK!!!
    MmUnsecureVirtualMemory((PVOID) Mem);
}

/* EOF */
