/*
 * PROJECT:     Authentication Package DLL
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/msv1_0/usercontext.c
 * PURPOSE:     manage user mode contexts (create, destroy, reference)
 * COPYRIGHT:   Copyright 2019 Andreas Maier <staubim@quantentunnel.de>
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(msv1_0);

// user mode context and lock
RTL_RESOURCE UsrContextLock;
LIST_ENTRY UsrContextList;

VOID
NtlmUsrContextInitialize(VOID)
{
    // initialize globals we only need in user mode
    // it's init once and freed (from windows) if
    // then process is terminated
    RtlInitializeResource(&UsrContextLock);
    InitializeListHead(&UsrContextList);
}

PNTLMSSP_CONTEXT_USR
NtlmUsrReferenceContext(
    IN LSA_SEC_HANDLE ContextHandle,
    IN BOOL IncRefCount)
{
    PLIST_ENTRY ContextEntry;
    PNTLMSSP_CONTEXT_USR ContextUsr;
    PNTLMSSP_CONTEXT_USR FoundContext = NULL;

    if (!RtlAcquireResourceShared(&UsrContextLock, TRUE))
    {
        ERR("RtlAcquireResourceShared failed\n");
        return NULL;
    }

    for (ContextEntry = UsrContextList.Flink;
         ContextEntry != &UsrContextList;
         ContextEntry = ContextEntry->Flink)
    {
        ContextUsr = CONTAINING_RECORD(ContextEntry, NTLMSSP_CONTEXT_USR, Entry);

        if (ContextUsr->ContextHandle == ContextHandle)
        {
            if (IncRefCount)
                InterlockedIncrement(&ContextUsr->Hdr->RefCount);
            FoundContext = ContextUsr;
            break;
        }
    }
    RtlReleaseResource(&UsrContextLock);

    return FoundContext;
}

VOID
NtlmUsrDereferenceContext(
    IN PNTLMSSP_CONTEXT_USR Context)
{
    NTSTATUS Status;

    if (InterlockedDecrement(&Context->Hdr->RefCount) > 0)
        return;

    // free entry
    Status = RtlAcquireResourceExclusive(&UsrContextLock, TRUE);
    if (!NT_SUCCESS(Status))
    {
        ERR("RtlAcquireResourceShared failed\n");
        return;
    }
    RemoveEntryList(&Context->Entry);
    RtlReleaseResource(&UsrContextLock);

    NtlmFree(Context->Hdr, FALSE);
    NtlmFree(Context, FALSE);
}

NTSTATUS NTAPI
UsrSpInitUserModeContext(
    IN LSA_SEC_HANDLE ContextHandle,
    IN PSecBuffer PackedContext)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PNTLMSSP_CONTEXT_HDR ContextPtr;
    PNTLMSSP_CONTEXT_USR UsrContext;
    BOOL UsrContextLockHeld = FALSE;

    fdTRACE("UsrSpInitUserModeContext(%p %p)\n",
          ContextHandle, PackedContext);

    if (PackedContext->BufferType != SECBUFFER_EMPTY)
    {
        fdTRACE("BufferType is invalid\n");
        return SEC_E_INVALID_TOKEN;
    }

    if ((PackedContext->cbBuffer != sizeof(NTLMSSP_CONTEXT_SVR)) &&
        (PackedContext->cbBuffer != sizeof(NTLMSSP_CONTEXT_CLI)))
    {
        fdTRACE("invalid context size\n");
        return STATUS_INVALID_PARAMETER_2;
    }

    if (!RtlAcquireResourceExclusive(&UsrContextLock, TRUE))
    {
        ERR("RtlAcquireResourceShared failed\n");
        return STATUS_LOCK_NOT_GRANTED;
    }
    UsrContextLockHeld = TRUE;

    // check if this context is already initialized
    UsrContext = NtlmUsrReferenceContext(ContextHandle, TRUE);
    if (UsrContext != NULL)
    {
        ULONG RefCount;
        NtlmUsrDereferenceContext(UsrContext);

        TRACE("Context already exists - updating!\n");
        RefCount = UsrContext->Hdr->RefCount;
        // copy context data (this is not the same as windows does)
        // its unsecure, however it will work for the first step
        ContextPtr = PackedContext->pvBuffer;
        RtlCopyMemory(UsrContext->Hdr, ContextPtr, PackedContext->cbBuffer);

        // restore old refcount
        UsrContext->Hdr->RefCount = RefCount;

        Status = STATUS_SUCCESS;
        goto done;
    }

    // allocate new user context
    UsrContext = NtlmAllocate(sizeof(NTLMSSP_CONTEXT_USR), FALSE);
    UsrContext->Hdr = NtlmAllocate(PackedContext->cbBuffer, FALSE);

    // copy context data (this is not the same as windows does)
    // its unsecure, however it will work for the first step
    ContextPtr = PackedContext->pvBuffer;
    RtlCopyMemory(UsrContext->Hdr, ContextPtr, PackedContext->cbBuffer);

    // insert into list
    InsertHeadList(&UsrContextList, &UsrContext->Entry);
    UsrContext->ContextHandle = ContextHandle;
    UsrContext->Hdr->RefCount = 1;
    //FIXME: do we need the original ProcID
    Status = STATUS_SUCCESS;

done:
    if (UsrContextLockHeld)
        RtlReleaseResource(&UsrContextLock);
    return Status;
}
