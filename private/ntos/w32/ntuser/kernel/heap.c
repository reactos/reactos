/****************************** Module Header ******************************\
* Module Name: heap.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains kernel-mode heap management code.
*
* History:
* 03-16-95 JimA         Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

NTSTATUS UserCommitDesktopMemory(
    PVOID pBase,
    PVOID *ppCommit,
    PSIZE_T pCommitSize)
{
    PDESKTOPVIEW    pdv;
    DWORD           dwCommitOffset;
    PWINDOWSTATION  pwinsta;
    PDESKTOP        pdesk;
    PBYTE           pUserBase;
    int             dCommit;
    NTSTATUS        Status;
    ETHREAD         *Thread = PsGetCurrentThread();

    /*
     * If this is a system thread, we have no view of the desktop
     * and must map it in.  Fortunately, this does not happen often.
     *
     * We use the Thread variable because IS_SYSTEM_THREAD is a macro
     * that multiply resolves the parameter.
     */
    if (IS_SYSTEM_THREAD(Thread)) {

        /*
         * Find the desktop that owns the section.
         */
        for (pwinsta = grpWinStaList; pwinsta; pwinsta = pwinsta->rpwinstaNext) {
            for (pdesk = pwinsta->rpdeskList; pdesk; pdesk = pdesk->rpdeskNext) {
                if (pdesk->pDeskInfo->pvDesktopBase == pBase)
                    goto FoundIt;
            }
        }
FoundIt:
        if (pwinsta == NULL) {
            RIPMSG3(RIP_ERROR, "UserCommitDesktopMemory failed: pBase %#p, ppCommit %#p, pCommitSize %d",
                    pBase, ppCommit, *pCommitSize);
            return STATUS_NO_MEMORY;
        }

        /*
         * Map the section into the current process and commit the
         * first page of the section.
         */
        dwCommitOffset = (ULONG)((PBYTE)*ppCommit - (PBYTE)pBase);
        Status = CommitReadOnlyMemory(pdesk->hsectionDesktop, pCommitSize,
                    dwCommitOffset, &dCommit);
        if (NT_SUCCESS(Status)) {
            *ppCommit = (PBYTE)*ppCommit + dCommit;
        }
    } else {

        /*
         * Find the current process' view of the desktop
         */
        for (pdv = PpiCurrent()->pdvList; pdv != NULL; pdv = pdv->pdvNext) {
            if (pdv->pdesk->pDeskInfo->pvDesktopBase == pBase)
                break;
        }
        
        /*
         * 254954: If we didn't find a desktop view then map the desktop view
         * to the current process.
         */
        if (pdv == NULL) {
            /*
             * Find the desktop that owns the section.
             */
            for (pwinsta = grpWinStaList; pwinsta; pwinsta = pwinsta->rpwinstaNext) {
                for (pdesk = pwinsta->rpdeskList; pdesk; pdesk = pdesk->rpdeskNext) {
                    if (pdesk->pDeskInfo->pvDesktopBase == pBase)
                        goto FoundTheDesktop;
                }
            }

FoundTheDesktop:
            if (pwinsta == NULL) {
                RIPMSG3(RIP_ERROR, "UserCommitDesktopMemory failed: pBase %#p, ppCommit %#p, pCommitSize %d",
                        pBase, ppCommit, *pCommitSize);
                return STATUS_NO_MEMORY;
            }

            UserAssert(pdesk != NULL);

            /*
             * Map the desktop into the current process
             */
            try {
                MapDesktop(ObOpenHandle, PsGetCurrentProcess(), pdesk, 0, 1);
            } except (W32ExceptionHandler(TRUE, RIP_WARNING)) {
                
                  RIPMSG2(RIP_WARNING, "UserCommitDesktopMemory: Could't map pdesk %#p in ppi %#p",
                        pdesk, PpiCurrent());
                return STATUS_NO_MEMORY;
            }
            
            pdv = GetDesktopView(PpiCurrent(), pdesk);
        }

        UserAssert(pdv != NULL);

        /*
         * Commit the memory
         */
        pUserBase = (PVOID)((PBYTE)*ppCommit - pdv->ulClientDelta);
        Status = ZwAllocateVirtualMemory(NtCurrentProcess(),
                                        &pUserBase,
                                        0,
                                        pCommitSize,
                                        MEM_COMMIT,
                                        PAGE_EXECUTE_READ
                                        );
        if (NT_SUCCESS(Status))
            *ppCommit = (PVOID)((PBYTE)pUserBase + pdv->ulClientDelta);
    }

    return Status;
}

NTSTATUS UserCommitSharedMemory(
    PVOID  pBase,
    PVOID *ppCommit,
    PSIZE_T pCommitSize)
{
    ULONG_PTR   ulClientDelta;
    DWORD       dwCommitOffset;
    PBYTE       pUserBase;
    NTSTATUS    Status;
    PEPROCESS   Process;
    int         dCommit;

#if DBG
    if (pBase != Win32HeapGetHandle(gpvSharedAlloc)) {
        RIPMSG0(RIP_WARNING, "pBase != gpvSharedAlloc");
    }
#else
    UNREFERENCED_PARAMETER(pBase);
#endif

    Process = PsGetCurrentProcess();

    ValidateProcessSessionId(Process);

    if (Process->Win32Process == NULL ||
        ((PPROCESSINFO)Process->Win32Process)->pClientBase == NULL) {

        dwCommitOffset = (ULONG)((PBYTE)*ppCommit - (PBYTE)gpvSharedBase);
        Status = CommitReadOnlyMemory(
                ghSectionShared, pCommitSize, dwCommitOffset, &dCommit);

        if (NT_SUCCESS(Status)) {
            *ppCommit = (PBYTE) *ppCommit + dCommit;
        }
    } else {

        /*
         * Commit the memory
         */
        ulClientDelta = (ULONG_PTR)((PBYTE)gpvSharedBase - (PBYTE)(PpiCurrent()->pClientBase));
        pUserBase = (PVOID)((PBYTE)*ppCommit - ulClientDelta);
        Status = ZwAllocateVirtualMemory(
                         NtCurrentProcess(),
                         &pUserBase,
                         0,
                         pCommitSize,
                         MEM_COMMIT,
                         PAGE_EXECUTE_READ);
        if (NT_SUCCESS(Status)) {
            *ppCommit = (PVOID)((PBYTE)pUserBase + ulClientDelta);
        }
    }

    return Status;
}

PWIN32HEAP UserCreateHeap(
    HANDLE                      hSection,
    ULONG                       ulViewOffset,
    PVOID                       pvBaseAddress,
    DWORD                       dwSize,
    PRTL_HEAP_COMMIT_ROUTINE    pfnCommit)
{
    PVOID pUserBase;
    SIZE_T ulViewSize;
    LARGE_INTEGER liOffset;
    PEPROCESS Process = PsGetCurrentProcess();
    RTL_HEAP_PARAMETERS HeapParams;
    NTSTATUS Status;
    ULONG HeapFlags;

    /*
     * Map the section into the current process and commit the
     * first page of the section.
     */
    ulViewSize        = 0;
    liOffset.LowPart  = ulViewOffset;
    liOffset.HighPart = 0;
    pUserBase         = NULL;

    Status = MmMapViewOfSection(
                    hSection,
                    Process,
                    &pUserBase,
                    0,
                    PAGE_SIZE,
                    &liOffset,
                    &ulViewSize,
                    ViewUnmap,
                    SEC_NO_CHANGE,
                    PAGE_EXECUTE_READ);

    if (!NT_SUCCESS(Status))
        return NULL;

    MmUnmapViewOfSection(Process, pUserBase);

    /*
     * We now have a committed page to create the heap in.
     */
    RtlZeroMemory(&HeapParams, sizeof(HeapParams));

    HeapParams.Length         = sizeof(HeapParams);
    HeapParams.InitialCommit  = PAGE_SIZE;
    HeapParams.InitialReserve = dwSize;
    HeapParams.CommitRoutine  = pfnCommit;

    UserAssert(HeapParams.InitialCommit < dwSize);
    
    
    HeapFlags = HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY;

#if DBG
    HeapFlags |= HEAP_TAIL_CHECKING_ENABLED;
#endif // DBG

    return Win32HeapCreate("UH_HEAD",
                           "UH_TAIL",
                           HeapFlags,
                           pvBaseAddress,
                           dwSize,
                           PAGE_SIZE,
                           NULL,
                           &HeapParams);
}
