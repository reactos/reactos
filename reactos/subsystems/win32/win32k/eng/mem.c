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
PVOID APIENTRY
EngAllocMem(ULONG Flags,
	    ULONG MemSize,
	    ULONG Tag)
{
    PVOID newMem;

    newMem = ExAllocatePoolWithTag((Flags & FL_NONPAGED_MEMORY) ? NonPagedPool : PagedPool,
                                   MemSize,
                                   Tag);

    if (newMem == NULL)
        return NULL;

    if (Flags & FL_ZERO_MEMORY)
        RtlZeroMemory(newMem, MemSize);

    return newMem;
}

/*
 * @implemented
 */
VOID APIENTRY
EngFreeMem(PVOID Mem)
{
  ExFreePool(Mem);
}

/*
 * @implemented
 */
PVOID APIENTRY
EngAllocUserMem(SIZE_T cj, ULONG Tag)
{
  PVOID NewMem = NULL;
  NTSTATUS Status;
  SIZE_T MemSize = cj;

  Status = ZwAllocateVirtualMemory(NtCurrentProcess(), &NewMem, 0, &MemSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

  if (! NT_SUCCESS(Status))
    {
      return NULL;
    }

  /* TODO: Add allocation info to AVL tree (stored inside W32PROCESS structure) */
  //hSecure = EngSecureMem(NewMem, cj);

  return NewMem;
}

/*
 * @implemented
 */
VOID APIENTRY
EngFreeUserMem(PVOID pv)
{
  PVOID BaseAddress = pv;
  SIZE_T MemSize = 0;

  ZwFreeVirtualMemory(NtCurrentProcess(), &BaseAddress, &MemSize, MEM_RELEASE);

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
	PMDL mdl;
	LOCK_OPERATION Operation;

	if (ProbeMode == PAGE_READONLY) Operation = IoReadAccess;
	else if (ProbeMode == PAGE_READWRITE) Operation = IoModifyAccess;
	else return NULL;

	mdl = IoAllocateMdl(Address, Size, FALSE, TRUE, NULL);
	if (mdl == NULL)
	{
		return NULL;
	}

	_SEH2_TRY
	{
		MmProbeAndLockPages(mdl, UserMode, Operation);
	}
	_SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
		Status = _SEH2_GetExceptionCode();
	}
	_SEH2_END

	if (!NT_SUCCESS(Status))
	{
		IoFreeMdl(mdl);
		return NULL;
	}

	*SafeAddress = MmGetSystemAddressForMdlSafe(mdl, NormalPagePriority);

	if(!*SafeAddress)
	{
		MmUnlockPages(mdl);
		IoFreeMdl(mdl);
		return NULL;           
	}

	return mdl;
}

VOID
APIENTRY
HackUnsecureVirtualMemory(
	IN PVOID  SecureHandle)
{
	PMDL mdl = (PMDL)SecureHandle;

	MmUnlockPages(mdl);
	IoFreeMdl(mdl);  
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
