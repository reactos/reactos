/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/fsrtl/filtrctx.c
 * PURPOSE:         File Stream Filter Context support for File System Drivers
 * PROGRAMMERS:     Pierre Schweitzer (pierre.schweitzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS *********************************************************/

typedef struct _FILE_OBJECT_FILTER_CONTEXTS
{
    FAST_MUTEX FilterContextsMutex;
    LIST_ENTRY FilterContexts;
} FILE_OBJECT_FILTER_CONTEXTS, *PFILE_OBJECT_FILTER_CONTEXTS;

/*
 * @implemented
 */
VOID
NTAPI
FsRtlPTeardownPerFileObjectContexts(IN PFILE_OBJECT FileObject)
{
    PFILE_OBJECT_FILTER_CONTEXTS FOContext = NULL;

    ASSERT(FileObject);

    if (!(FOContext = IoGetFileObjectFilterContext(FileObject)))
    {
        return;
    }

    ASSERT(IoChangeFileObjectFilterContext(FileObject, FOContext, FALSE) == STATUS_SUCCESS);
    ASSERT(IsListEmpty(&(FOContext->FilterContexts)));

    ExFreePoolWithTag(FOContext, 'FOCX');
}


/* PUBLIC FUNCTIONS **********************************************************/

/*++
 * @name FsRtlIsPagingFile
 * @implemented NT 5.2
 *
 *     The FsRtlIsPagingFile routine checks if the FileObject is a Paging File.
 *
 * @param FileObject
 *        A pointer to the File Object to be tested.
 *
 * @return TRUE if the File is a Paging File, FALSE otherwise.
 *
 * @remarks None.
 *
 *--*/
LOGICAL
NTAPI
FsRtlIsPagingFile(IN PFILE_OBJECT FileObject)
{
    return MmIsFileObjectAPagingFile(FileObject);
}

/*
 * @implemented
 */
PFSRTL_PER_FILEOBJECT_CONTEXT
NTAPI
FsRtlLookupPerFileObjectContext(IN PFILE_OBJECT FileObject,
                                IN PVOID OwnerId OPTIONAL,
                                IN PVOID InstanceId OPTIONAL)
{
    PLIST_ENTRY NextEntry;
    PFILE_OBJECT_FILTER_CONTEXTS FOContext = NULL;
    PFSRTL_PER_FILEOBJECT_CONTEXT TmpPerFOContext, PerFOContext = NULL;

    if (!FileObject || !(FOContext = IoGetFileObjectFilterContext(FileObject)))
    {
        return NULL;
    }

    ExAcquireFastMutex(&(FOContext->FilterContextsMutex));

    /* If list is empty, no need to browse it */
    if (!IsListEmpty(&(FOContext->FilterContexts)))
    {
        for (NextEntry = FOContext->FilterContexts.Flink;
             NextEntry != &(FOContext->FilterContexts);
             NextEntry = NextEntry->Flink)
        {
            /* If we don't have any criteria for search, first entry will be enough */
            if (!OwnerId && !InstanceId)
            {
                PerFOContext = (PFSRTL_PER_FILEOBJECT_CONTEXT)NextEntry;
                break;
            }
            /* Else, we've to find something that matches with the parameters. */
            else
            {
                TmpPerFOContext = CONTAINING_RECORD(NextEntry, FSRTL_PER_FILEOBJECT_CONTEXT, Links);
                if ((InstanceId && TmpPerFOContext->InstanceId == InstanceId && TmpPerFOContext->OwnerId == OwnerId) ||
                    (OwnerId && TmpPerFOContext->OwnerId == OwnerId))
                {
                    PerFOContext = TmpPerFOContext;
                    break;
                }
            }
        }
    }

    ExReleaseFastMutex(&(FOContext->FilterContextsMutex));

    return PerFOContext;
}

/*
 * @implemented
 */
PFSRTL_PER_STREAM_CONTEXT
NTAPI
FsRtlLookupPerStreamContextInternal(IN PFSRTL_ADVANCED_FCB_HEADER AdvFcbHeader,
                                    IN PVOID OwnerId OPTIONAL,
                                    IN PVOID InstanceId OPTIONAL)
{
    PLIST_ENTRY NextEntry;
    PFSRTL_PER_STREAM_CONTEXT TmpPerStreamContext, PerStreamContext = NULL;

    ASSERT(AdvFcbHeader);
    ASSERT(FlagOn(AdvFcbHeader->Flags2, FSRTL_FLAG2_SUPPORTS_FILTER_CONTEXTS));

    ExAcquireFastMutex(AdvFcbHeader->FastMutex);

    /* If list is empty, no need to browse it */
    if (!IsListEmpty(&(AdvFcbHeader->FilterContexts)))
    {
        for (NextEntry = AdvFcbHeader->FilterContexts.Flink;
             NextEntry != &(AdvFcbHeader->FilterContexts);
             NextEntry = NextEntry->Flink)
        {
            /* If we don't have any criteria for search, first entry will be enough */
            if (!OwnerId && !InstanceId)
            {
                PerStreamContext = (PFSRTL_PER_STREAM_CONTEXT)NextEntry;
                break;
            }
            /* Else, we've to find something that matches with the parameters. */
            else
            {
                TmpPerStreamContext = CONTAINING_RECORD(NextEntry, FSRTL_PER_STREAM_CONTEXT, Links);
                if ((InstanceId && TmpPerStreamContext->InstanceId == InstanceId && TmpPerStreamContext->OwnerId == OwnerId) ||
                    (OwnerId && TmpPerStreamContext->OwnerId == OwnerId))
                {
                    PerStreamContext = TmpPerStreamContext;
                    break;
                }
            }
        }
    }

    ExReleaseFastMutex(AdvFcbHeader->FastMutex);

    return PerStreamContext;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
FsRtlInsertPerFileObjectContext(IN PFILE_OBJECT FileObject,
                                IN PFSRTL_PER_FILEOBJECT_CONTEXT Ptr)
{
    PFILE_OBJECT_FILTER_CONTEXTS FOContext = NULL;
    NTSTATUS Status;

    if (!FileObject)
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (!(FileObject->Flags & FO_FILE_OBJECT_HAS_EXTENSION))
    {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    /* Get filter contexts */
    FOContext = IoGetFileObjectFilterContext(FileObject);
    if (!FOContext)
    {
        /* If there's none, allocate new structure */
        FOContext = ExAllocatePoolWithTag(NonPagedPool, sizeof(FILE_OBJECT_FILTER_CONTEXTS), 'FOCX');
        if (!FOContext)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        /* Initialize it */
        ExInitializeFastMutex(&(FOContext->FilterContextsMutex));
        InitializeListHead(&(FOContext->FilterContexts));

        /* Set it */
        Status = IoChangeFileObjectFilterContext(FileObject, FOContext, TRUE);
        if (!NT_SUCCESS(Status))
        {
            /* If it fails, it means that someone else has set it in the meanwhile */
            ExFreePoolWithTag(FOContext, 'FOCX');

            /* So, we can get it */
            FOContext = IoGetFileObjectFilterContext(FileObject);
            if (!FOContext)
            {
                /* If we fall down here, something went very bad. This shouldn't happen */
                ASSERT(FALSE);
                return STATUS_UNSUCCESSFUL;
            }
        }
    }

    /* Finally, insert */
    ExAcquireFastMutex(&(FOContext->FilterContextsMutex));
    InsertHeadList(&(FOContext->FilterContexts), &(Ptr->Links));
    ExReleaseFastMutex(&(FOContext->FilterContextsMutex));

    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
FsRtlInsertPerStreamContext(IN PFSRTL_ADVANCED_FCB_HEADER AdvFcbHeader,
                            IN PFSRTL_PER_STREAM_CONTEXT PerStreamContext)
{
    if (!(AdvFcbHeader) || !(AdvFcbHeader->Flags2 & FSRTL_FLAG2_SUPPORTS_FILTER_CONTEXTS))
    {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    ExAcquireFastMutex(AdvFcbHeader->FastMutex);
    InsertHeadList(&(AdvFcbHeader->FilterContexts), &(PerStreamContext->Links));
    ExReleaseFastMutex(AdvFcbHeader->FastMutex);
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
PFSRTL_PER_FILEOBJECT_CONTEXT
NTAPI
FsRtlRemovePerFileObjectContext(IN PFILE_OBJECT FileObject,
                                IN PVOID OwnerId OPTIONAL,
                                IN PVOID InstanceId OPTIONAL)
{
    PLIST_ENTRY NextEntry;
    PFILE_OBJECT_FILTER_CONTEXTS FOContext = NULL;
    PFSRTL_PER_FILEOBJECT_CONTEXT TmpPerFOContext, PerFOContext = NULL;

    if (!FileObject || !(FOContext = IoGetFileObjectFilterContext(FileObject)))
    {
        return NULL;
    }

    ExAcquireFastMutex(&(FOContext->FilterContextsMutex));

    /* If list is empty, no need to browse it */
    if (!IsListEmpty(&(FOContext->FilterContexts)))
    {
        for (NextEntry = FOContext->FilterContexts.Flink;
             NextEntry != &(FOContext->FilterContexts);
             NextEntry = NextEntry->Flink)
        {
            /* If we don't have any criteria for search, first entry will be enough */
            if (!OwnerId && !InstanceId)
            {
                PerFOContext = (PFSRTL_PER_FILEOBJECT_CONTEXT)NextEntry;
                break;
            }
            /* Else, we've to find something that matches with the parameters. */
            else
            {
                TmpPerFOContext = CONTAINING_RECORD(NextEntry, FSRTL_PER_FILEOBJECT_CONTEXT, Links);
                if ((InstanceId && TmpPerFOContext->InstanceId == InstanceId && TmpPerFOContext->OwnerId == OwnerId) ||
                    (OwnerId && TmpPerFOContext->OwnerId == OwnerId))
                {
                    PerFOContext = TmpPerFOContext;
                    break;
                }
            }
        }

        /* Finally remove entry from list */
        if (PerFOContext)
        {
            RemoveEntryList(&(PerFOContext->Links));
        }
    }

    ExReleaseFastMutex(&(FOContext->FilterContextsMutex));

    return PerFOContext;
}

/*
 * @implemented
 */
PFSRTL_PER_STREAM_CONTEXT
NTAPI
FsRtlRemovePerStreamContext(IN PFSRTL_ADVANCED_FCB_HEADER AdvFcbHeader,
                            IN PVOID OwnerId OPTIONAL,
                            IN PVOID InstanceId OPTIONAL)
{
    PLIST_ENTRY NextEntry;
    PFSRTL_PER_STREAM_CONTEXT TmpPerStreamContext, PerStreamContext = NULL;

    if (!(AdvFcbHeader) || !(AdvFcbHeader->Flags2 & FSRTL_FLAG2_SUPPORTS_FILTER_CONTEXTS))
    {
        return NULL;
    }

    ExAcquireFastMutex(AdvFcbHeader->FastMutex);
    /* If list is empty, no need to browse it */
    if (!IsListEmpty(&(AdvFcbHeader->FilterContexts)))
    {
        for (NextEntry = AdvFcbHeader->FilterContexts.Flink;
             NextEntry != &(AdvFcbHeader->FilterContexts);
             NextEntry = NextEntry->Flink)
        {
            /* If we don't have any criteria for search, first entry will be enough */
            if (!OwnerId && !InstanceId)
            {
                PerStreamContext = (PFSRTL_PER_STREAM_CONTEXT)NextEntry;
                break;
            }
            /* Else, we've to find something that matches with the parameters. */
            else
            {
                TmpPerStreamContext = CONTAINING_RECORD(NextEntry, FSRTL_PER_STREAM_CONTEXT, Links);
                if ((InstanceId && TmpPerStreamContext->InstanceId == InstanceId && TmpPerStreamContext->OwnerId == OwnerId) ||
                    (OwnerId && TmpPerStreamContext->OwnerId == OwnerId))
                {
                    PerStreamContext = TmpPerStreamContext;
                    break;
                }
            }
        }

        /* Finally remove entry from list */
        if (PerStreamContext)
        {
            RemoveEntryList(&(PerStreamContext->Links));
        }
    }
    ExReleaseFastMutex(AdvFcbHeader->FastMutex);

    return PerStreamContext;

}

/*
 * @implemented
 */
VOID
NTAPI
FsRtlTeardownPerStreamContexts(IN PFSRTL_ADVANCED_FCB_HEADER AdvFcbHeader)
{
    PLIST_ENTRY NextEntry;
    volatile BOOLEAN IsMutexLocked = FALSE;
    PFSRTL_PER_STREAM_CONTEXT PerStreamContext;

    _SEH2_TRY
    {
        /* Acquire mutex to deal with the list */
        ExAcquireFastMutex(AdvFcbHeader->FastMutex);
        IsMutexLocked = TRUE;

        /* While there are items... */
        while (!IsListEmpty(&(AdvFcbHeader->FilterContexts)))
        {
            /* ...remove one */
            NextEntry = RemoveHeadList(&(AdvFcbHeader->FilterContexts));
            PerStreamContext = CONTAINING_RECORD(NextEntry, FSRTL_PER_STREAM_CONTEXT, Links);

            /* Release mutex before calling callback */
            ExReleaseFastMutex(AdvFcbHeader->FastMutex);
            IsMutexLocked = FALSE;

            /* Call the callback */
            ASSERT(PerStreamContext->FreeCallback);
            (*PerStreamContext->FreeCallback)(PerStreamContext);

            /* Relock the list to continue */
            ExAcquireFastMutex(AdvFcbHeader->FastMutex);
            IsMutexLocked = TRUE;
        }
    }
    _SEH2_FINALLY
    {
        /* If mutex was locked, release */
        if (IsMutexLocked)
        {
            ExReleaseFastMutex(AdvFcbHeader->FastMutex);
        }
    }
    _SEH2_END;
}
