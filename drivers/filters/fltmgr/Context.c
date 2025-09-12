/*
* PROJECT:         Filesystem Filter Manager
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            drivers/filters/fltmgr/Context.c
* PURPOSE:         Contains context routines
* PROGRAMMERS:     Ged Murphy (gedmurphy@reactos.org)
*/

/* INCLUDES ******************************************************************/

#include "fltmgr.h"
#include "fltmgrint.h"

#define NDEBUG
#include <debug.h>


/* DATA *********************************************************************/

static
BOOLEAN
IsContextTypeValid(
    _In_ FLT_CONTEXT_TYPE ContextType
);

static
NTSTATUS
SetupContextHeader(
    _In_ PFLT_FILTER Filter,
    _In_ PCFLT_CONTEXT_REGISTRATION ContextPtr,
    _Out_ PALLOCATE_CONTEXT_HEADER ContextHeader
);

/* EXPORTED FUNCTIONS ******************************************************/




/* INTERNAL FUNCTIONS ******************************************************/


NTSTATUS
FltpRegisterContexts(_In_ PFLT_FILTER Filter,
                     _In_ const FLT_CONTEXT_REGISTRATION *Context)
{
    PCFLT_CONTEXT_REGISTRATION ContextPtr;
    PALLOCATE_CONTEXT_HEADER ContextHeader, Prev;
    PVOID Buffer;
    ULONG BufferSize = 0;
    USHORT i;
    NTSTATUS Status;

    /* Loop through all entries in the context registration array */
    ContextPtr = Context;
    while (ContextPtr)
    {
        /* Bail if we found the terminator */
        if (ContextPtr->ContextType == FLT_CONTEXT_END)
            break;

        /* Make sure we have a valid context  */
        if (IsContextTypeValid(ContextPtr->ContextType))
        {
            /* Each context is backed by a crtl struct. Reserve space for it */
            BufferSize += sizeof(STREAM_LIST_CTRL);
        }

        /* Move to the next entry */
        ContextPtr++;
    }

    /* Bail if we found no valid registration requests */
    if (BufferSize == 0)
    {
        return STATUS_SUCCESS;
    }

    /* Allocate the pool that'll hold the context crtl structs */
    Buffer = ExAllocatePoolWithTag(NonPagedPool, BufferSize, FM_TAG_CONTEXT_REGISTA);
    if (!Buffer) return STATUS_INSUFFICIENT_RESOURCES;

    RtlZeroMemory(Buffer, BufferSize);

    /* Setup our loop data */
    ContextHeader = Buffer;
    Prev = NULL;
    Status = STATUS_SUCCESS;

    for (i = 0; i < MAX_CONTEXT_TYPES; i++)
    {
        ContextPtr = (PFLT_CONTEXT_REGISTRATION)Context;
        while (ContextPtr)
        {
            /* We don't support variable sized contents yet */
            FLT_ASSERT(ContextPtr->Size != FLT_VARIABLE_SIZED_CONTEXTS);

            /* Bail if we found the terminator */
            if (ContextPtr->ContextType == FLT_CONTEXT_END)
                break;

            /* Size and pooltag are only checked when ContextAllocateCallback is null */
            if (ContextPtr->ContextAllocateCallback == FALSE && ContextPtr->PoolTag == FALSE)
            {
                Status = STATUS_FLT_INVALID_CONTEXT_REGISTRATION;
                goto Quit;
            }

            /* Make sure we have a valid context  */
            if (IsContextTypeValid(ContextPtr->ContextType))
            {
                Status = SetupContextHeader(Filter, ContextPtr, ContextHeader);
                if (NT_SUCCESS(Status))
                {
                    if (Prev)
                    {
                        Prev->Next = ContextHeader;
                    }

                    Filter->SupportedContexts[i] = ContextHeader;
                }
            }

            Prev = ContextHeader;

            /* Move to the next entry */
            ContextPtr++;
        }
    }

Quit:
    if (NT_SUCCESS(Status))
    {
        Filter->SupportedContextsListHead = Buffer;
    }
    else
    {
        ExFreePoolWithTag(Buffer, FM_TAG_CONTEXT_REGISTA);
        //FIXME: Cleanup anything that SetupContextHeader may have allocated
    }

    return Status;
}


/* PRIVATE FUNCTIONS ******************************************************/

static
BOOLEAN
IsContextTypeValid(_In_ FLT_CONTEXT_TYPE ContextType)
{
    switch (ContextType)
    {
    case FLT_VOLUME_CONTEXT:
    case FLT_INSTANCE_CONTEXT:
    case FLT_FILE_CONTEXT:
    case FLT_STREAM_CONTEXT:
    case FLT_STREAMHANDLE_CONTEXT:
    case FLT_TRANSACTION_CONTEXT:
        return TRUE;
    }

    return FALSE;
}

static
NTSTATUS
SetupContextHeader(_In_ PFLT_FILTER Filter,
                   _In_ PCFLT_CONTEXT_REGISTRATION ContextPtr,
                   _Out_ PALLOCATE_CONTEXT_HEADER ContextHeader)
{
    return 0;
}
